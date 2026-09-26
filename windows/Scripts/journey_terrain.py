"""
The Matterhorn journey's land (Élan Cube, journey I): swisstopo's open data (Swiss OGD, free use with the source named,
since 1 March 2021) and the Copernicus DEM beyond, prepared for MuseeJourneyLibrary (the editor builds the meshes) and
journeys.py (it imports the images).

    python windows/Scripts/journey_terrain.py fetch     download (STAC API, no account) into SourceArt/Journeys/Matterhorn/raw
    python windows/Scripts/journey_terrain.py build     tiles, images and places into SourceArt/Journeys/Matterhorn/terrain

Data:
- swissALTI3D (2 m, and 0.5 m round the places): the terrain model, © swisstopo.
- SWISSIMAGE (2 m, and 10 cm round the places): orthophotos, © swisstopo. De-lit here (the sun of the flight divided out,
  its cast shadows found on the terrain) so the journey's own sun can light them.
- Copernicus DEM GLO-30 (30 m) for the land beyond the 24 × 18 km box (Mont Blanc, Monte Rosa, the Weisshorn):
  © DLR e.V. 2010–2014 and © Airbus Defence and Space GmbH 2014–2018, provided under COPERNICUS by the European Union and ESA.

Frame: LV95 metres (E, N) and heights above the sea; the Earth's curvature (with refraction, k 0.13) is taken off every
height relative to the summit, so the far peaks stand as low as they do from there. Every tile is a grid of vertices on
exact metre lines; neighbours of different spacing are stitched (the finer edge follows the coarser), so no cracks.

Tile file (.bin, little endian): b'MHT1', int32 nx, int32 ny, float64 west E, float64 north N, float32 spacing, then
float32 heights, ny rows from north to south, nx each from west to east.
"""
import json
import math
import os
import sys
import urllib.request

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
BASE = os.path.normpath(os.path.join(HERE, "..", "SourceArt", "Journeys", "Matterhorn"))
RAW = os.path.join(BASE, "raw")
OUT = os.path.join(BASE, "terrain")

# The near box (2 m data), km lines.
E0, E1, N0, N1 = 2608000, 2632000, 1083000, 1101000
# The far land (Copernicus), on a 50 m grid whose lines meet the box's edges.
FAR_E0, FAR_E1, FAR_N0, FAR_N1, FAR_S = 2545000, 2665000, 1045000, 1135000, 50.0
FAR_TILE = 400   # cells
R_EFF = 6371000.0 / (1.0 - 0.13)

# The places (LV95 E, N; the eye's height is the ground's (or the water's) plus the car's floor and 1.6 m).
SUMMIT = (2617044.5, 1091674.0)
PLACES = {
    "riffelsee": {"E": 2625095.0, "N": 1092485.0, "look": SUMMIT},
    "glacier": {"E": 2621643.0, "N": 1089796.0, "look": SUMMIT},
    # Beside the Hörnli hut, off the annex's south-east face, where the path onto the ridge begins.
    "hornli": {"E": 2618471.1, "N": 1092298.3, "look": SUMMIT},
    # Off the crest by the Solvay hut (on the crest at 2617404.6, 1091942.6), 6 m out over the east face (the face falls
    # away under the glass floor; the hut and the fixed rope beside).
    "solvay": {"E": 2617409.8, "N": 1091939.6, "look": SUMMIT, "floor": 4001.5},
    "summit": {"E": SUMMIT[0], "N": SUMMIT[1], "look": None},
}
# The buildings (E, N of their centre, the bearing of their long axis, the ground floor's rise over the terrain there):
# the Hörnli hut (the old Hotel Belvedere's centre; SWISSIMAGE 2017) and the Solvay hut on the crest.
BUILDINGS = {"hornli": (2618474.5, 1092319.5, 45.0, 0.3), "solvay": (2617404.6, 1091942.6, 31.0, 0.6),
             "cross": (2616969.5, 1091695.0, 106.0, 0.0)}   # the iron cross on the Italian summit, its arms along the ridge
# Tiles with 0.5 m terrain and 10 cm images.
HIRES = ["2616-1091", "2616-1092", "2617-1091", "2617-1092", "2618-1092", "2624-1092", "2625-1092", "2625-1093"]
HIRES_PX = 4096


def lv95_to_wgs(E, N):
    y = (E - 2600000) / 1e6
    x = (N - 1200000) / 1e6
    lon = 2.6779094 + 4.728982 * y + 0.791484 * y * x + 0.1306 * y * x * x - 0.0436 * y ** 3
    lat = 16.9023892 + 3.238272 * x - 0.270978 * y * y - 0.002528 * x * x - 0.0447 * y * y * x - 0.0140 * x ** 3
    return lat * 100 / 36, lon * 100 / 36


# ---------------------------------------------------------------------------------------------- fetch

STAC = "https://data.geo.admin.ch/api/stac/v0.9/collections/{c}/items?bbox={b}&limit=100"


def _items(coll, e0, n0, e1, n1):
    la0, lo0 = lv95_to_wgs(e0, n0)
    la1, lo1 = lv95_to_wgs(e1, n1)
    url = STAC.format(c=coll, b=f"{lo0},{la0},{lo1},{la1}")
    out = []
    while url:
        d = json.load(urllib.request.urlopen(url, timeout=60))
        out += d["features"]
        url = next((l["href"] for l in d.get("links", []) if l["rel"] == "next"), None)
    return out


def _fetch(coll, gsd, e0, n0, e1, n1, want=None):
    import concurrent.futures as cf
    best = {}
    for f in _items(coll, e0 + 10, n0 + 10, e1 - 10, n1 - 10):
        _, year, tile = f["id"].split("_")[:3]
        if tile not in best or year > best[tile][0]:
            best[tile] = (year, f)
    jobs = []
    for tile, (year, f) in best.items():
        E, N = map(int, tile.split("-"))
        if not (e0 // 1000 <= E < e1 // 1000 and n0 // 1000 <= N < n1 // 1000) or (want and tile not in want):
            continue
        for k, a in f["assets"].items():
            if k.endswith(".tif") and abs(float(a.get("eo:gsd", 0)) - gsd) < 1e-6:
                jobs.append((a["href"], os.path.join(RAW, "swiss", f"{coll.split('.')[-1]}_{gsd}_{tile}.tif")))

    def get(job):
        url, path = job
        if not os.path.exists(path):
            urllib.request.urlretrieve(url, path + ".part")
            os.replace(path + ".part", path)

    with cf.ThreadPoolExecutor(8) as ex:
        list(ex.map(get, jobs))
    print(coll, gsd, len(jobs), "files")


def fetch():
    os.makedirs(os.path.join(RAW, "swiss"), exist_ok=True)
    os.makedirs(os.path.join(RAW, "cop"), exist_ok=True)
    _fetch("ch.swisstopo.swissalti3d", 2.0, E0, N0, E1, N1)
    _fetch("ch.swisstopo.swissimage-dop10", 2.0, E0, N0, E1, N1)
    want = set(HIRES)
    _fetch("ch.swisstopo.swissalti3d", 0.5, E0, N0, E1, N1, want)
    _fetch("ch.swisstopo.swissimage-dop10", 0.1, E0, N0, E1, N1, want)
    for lat in (45, 46):
        for lon in (6, 7, 8):
            n = f"Copernicus_DSM_COG_10_N{lat}_00_E{lon:03d}_00_DEM"
            p = os.path.join(RAW, "cop", n + ".tif")
            if not os.path.exists(p):
                urllib.request.urlretrieve(f"https://copernicus-dem-30m.s3.amazonaws.com/{n}/{n}.tif", p)
    print("fetched")


# ---------------------------------------------------------------------------------------------- data

def _read(path):
    import cv2
    return cv2.imread(path, cv2.IMREAD_UNCHANGED)


def near_dem(cop):
    """
    The 2 m terrain over the box: (rows north→south, cols west→east), pixel centres at (E0 + 1 + 2i, N1 - 1 - 2j). Where
    swissALTI3D stops (Italy, south-west of the ridge's foot) the Copernicus DEM, brought to the swiss heights along the
    border by a smooth offset.
    """
    import cv2
    H = np.full(((N1 - N0) // 2, (E1 - E0) // 2), np.nan, np.float32)
    for e in range(E0 // 1000, E1 // 1000):
        for n in range(N0 // 1000, N1 // 1000):
            path = os.path.join(RAW, "swiss", f"swissalti3d_2.0_{e}-{n}.tif")
            if not os.path.exists(path):
                continue
            d = _read(path)
            r0, c0 = (N1 // 1000 - n - 1) * 500, (e - E0 // 1000) * 500
            H[r0:r0 + 500, c0:c0 + 500] = np.where(d < -100, np.nan, d)
    bad = np.isnan(H)
    if bad.any():
        print("near DEM: from Copernicus,", int(bad.sum()), "pixels")
        cols = E0 + 1 + 2 * np.arange(H.shape[1], dtype=np.float64)
        rows = N1 - 1 - 2 * np.arange(H.shape[0], dtype=np.float64)
        GE, GN = np.meshgrid(cols, rows)
        C = cop(GE, GN)
        # The offset (swiss − Copernicus) where both are, spread smoothly over the gap (normalized convolution, 400 m).
        small = 8
        diff = np.where(bad, 0, H - C)[::small, ::small]
        w = (~bad)[::small, ::small].astype(np.float32)
        sig = 400 / (2 * small)
        db = cv2.GaussianBlur(diff.astype(np.float32) * w, (0, 0), sig)
        wb = cv2.GaussianBlur(w, (0, 0), sig)
        off = np.where(wb > 1e-4, db / np.maximum(wb, 1e-4), 0)
        off = cv2.resize(off.astype(np.float32), (H.shape[1], H.shape[0]), interpolation=cv2.INTER_LINEAR)
        H = np.where(bad, C + off, H).astype(np.float32)
    return H, bad


def near_image():
    img = np.zeros(((N1 - N0) // 2, (E1 - E0) // 2, 3), np.uint8)
    for e in range(E0 // 1000, E1 // 1000):
        for n in range(N0 // 1000, N1 // 1000):
            path = os.path.join(RAW, "swiss", f"swissimage-dop10_2.0_{e}-{n}.tif")
            if not os.path.exists(path):
                continue
            d = _read(path)
            r0, c0 = (N1 // 1000 - n - 1) * 500, (e - E0 // 1000) * 500
            img[r0:r0 + 500, c0:c0 + 500] = d[:, :, :3]
    return img   # BGR


def copernicus():
    """The Copernicus mosaic over 45–47°N, 6–9°E (1″), rows from 47°N down, and a sampler in LV95."""
    import cv2
    M = np.zeros((7200, 10800), np.float32)
    for lat in (45, 46):
        for lon in (6, 7, 8):
            d = _read(os.path.join(RAW, "cop", f"Copernicus_DSM_COG_10_N{lat}_00_E{lon:03d}_00_DEM.tif"))
            r0, c0 = (46 - lat) * 3600, (lon - 6) * 3600
            M[r0:r0 + 3600, c0:c0 + 3600] = d[:3600, :3600]

    def sample(E, N):
        lat, lon = lv95_to_wgs(E, N)
        r = ((47.0 - lat) * 3600.0 - 0.5).astype(np.float32)
        c = ((lon - 6.0) * 3600.0 - 0.5).astype(np.float32)
        return cv2.remap(M, c, r, cv2.INTER_LINEAR, borderMode=cv2.BORDER_REPLICATE)
    return sample


def bilinear(H, E, N, west, north, s):
    """Sample a grid of pixel centres (west + s/2 + s·i, north − s/2 − s·j) at points (E, N)."""
    import cv2
    c = ((E - west) / s - 0.5).astype(np.float32)
    r = ((north - N) / s - 0.5).astype(np.float32)
    return cv2.remap(H, c, r, cv2.INTER_LINEAR, borderMode=cv2.BORDER_REPLICATE)


def curvature(E, N):
    return ((E - SUMMIT[0]) ** 2 + (N - SUMMIT[1]) ** 2) / (2.0 * R_EFF)


# ---------------------------------------------------------------------------------------------- de-lighting

def normals(H, s):
    gy, gx = np.gradient(H.astype(np.float64), s)   # rows go south: dH/dN = -gy
    n = np.dstack([-gx, gy, np.ones_like(gx)])
    return n / np.linalg.norm(n, axis=2, keepdims=True)


def sun_vector(az, el):
    a, e = math.radians(az), math.radians(el)
    return np.array([math.sin(a) * math.cos(e), math.cos(a) * math.cos(e), math.sin(e)])


def cast_shadow(H, s, az, el):
    """1 where the sun (azimuth, elevation) is hidden by the terrain: rotate so the sun lies along +x, sweep a running max."""
    import cv2
    h, w = H.shape
    ang = 90.0 - az   # the direction to the sun in image coordinates (x east, y north up the image)
    M = cv2.getRotationMatrix2D((w / 2, h / 2), -ang, 1.0)
    diag = int(math.hypot(w, h)) + 4
    M[0, 2] += (diag - w) / 2
    M[1, 2] += (diag - h) / 2
    R = cv2.warpAffine(H, M, (diag, diag), flags=cv2.INTER_LINEAR, borderValue=-1e4)
    t = math.tan(math.radians(el)) * s
    x = np.arange(diag, dtype=np.float32)[None, :]
    a = R - x * (-t)    # a height relative to the sun ray through each column (rays rise towards +x, the sun)
    # shadowed if any point further towards the sun stands above the ray: max over k > j of (h_k − (k − j)·t) > h_j
    b = R - x * t
    run = np.maximum.accumulate(b[:, ::-1], axis=1)[:, ::-1]
    run = np.concatenate([run[:, 1:], np.full((diag, 1), -1e9, np.float32)], axis=1)
    shade = (run > b + 0.5).astype(np.float32)
    Minv = cv2.invertAffineTransform(M)
    back = cv2.warpAffine(shade, Minv, (w, h), flags=cv2.INTER_LINEAR, borderValue=0)
    return back


def delight(img_bgr, H, s, fit=None, extra_shadow=None):
    """The flight's sun divided out of an orthophoto (BGR uint8) over the terrain H (spacing s): returns BGR float 0..1."""
    import cv2
    img = img_bgr.astype(np.float32) / 255.0
    lin = img ** 2.2
    lum = lin @ np.array([0.0722, 0.7152, 0.2126], np.float32)
    N = normals(H, s)
    if fit is None:
        # The sun of the flight: the direction whose Lambert term best explains the image's light on the rock and the
        # scree (a middle band of brightness; the snow's albedo would swamp it).
        rng = np.random.default_rng(1)
        idx = rng.integers(0, lum.size, 400000)
        l = lum.ravel()[idx]
        nn = N.reshape(-1, 3)[idx]
        keep = (l > 0.01) & (l < 0.35)
        best = (-2, None)
        for az in range(90, 271, 5):
            for el in range(25, 71, 5):
                d = np.clip(nn[keep] @ sun_vector(az, el), 0, None)
                c = np.corrcoef(d, l[keep])[0, 1]
                if c > best[0]:
                    best = (c, (az, el))
        az, el = best[1]
        print(f"de-light: the flight's sun at azimuth {az}°, elevation {el}° (r {best[0]:.2f})")
        fit = {"az": az, "el": el}
    L = sun_vector(fit["az"], fit["el"])
    cos = np.clip(N @ L, 0, None)
    sh = cast_shadow(H, s, fit["az"], fit["el"])
    if extra_shadow is not None:
        sh = np.maximum(sh, extra_shadow)
    sh = cv2.GaussianBlur(sh, (0, 0), 1.0)
    direct = cos * (1 - sh)
    # The sky's share and its colour, from the snow (white: what differs between its lit and its shadowed parts is the
    # light). Lit snow: bright, unclipped, facing the sun; shadowed snow: blue (the sky's light alone). With the sun's
    # light 1 on a lit face at cos θ: shadow / lit = sky / (sky + cos θ), per channel.
    if "sky" not in fit:
        mx, mn = img.max(axis=2), img.min(axis=2)
        lit = (mn > 0.75) & (mx < 0.985) & (direct > 0.6)
        dark = (img[:, :, 0] > img[:, :, 2] * 1.25) & (img[:, :, 0] > 0.35) & (lum < 0.25) & (direct < 0.1)
        if lit.sum() > 1000 and dark.sum() > 1000:
            r = np.mean(lin[dark], axis=0) / np.mean(lin[lit], axis=0)
            c = float(np.median(cos[lit]))
            a = r * c / (1.0 - r)
        else:
            a = np.array([0.43, 0.18, 0.09], np.float32)   # B, G, R
        fit["sky"] = [float(x) for x in np.clip(a, 0.05, 0.8)]
        print("de-light: sky share (B, G, R)", np.round(a, 3), int(lit.sum()), int(dark.sum()))
    sky = np.array(fit["sky"], np.float32)
    # Light at each pixel relative to a flat sunlit field: sky + sun·cosθ, per channel.
    flat = sky + math.sin(math.radians(fit["el"]))
    light = sky[None, None, :] + direct[:, :, None]
    light = light / flat[None, None, :]
    alb = lin / np.maximum(light, 0.12)
    alb = np.clip(alb, 0, 1) ** (1 / 2.2)
    return alb, fit, sh


# ---------------------------------------------------------------------------------------------- tiles

def write_tile(path, west, north, s, Z):
    ny, nx = Z.shape
    with open(path, "wb") as f:
        f.write(b"MHT1")
        f.write(np.array([nx, ny], np.int32).tobytes())
        f.write(np.array([west, north], np.float64).tobytes())
        f.write(np.array([s], np.float32).tobytes())
        f.write(Z.astype(np.float32).tobytes())


def lod_of(e, n):
    """The spacing of the km tile (e, n): 2 m near the ridge's places and the lake, 4 m on the mountain, 8 m else."""
    cx, cy = e * 1000 + 500, n * 1000 + 500
    def dist(p):
        return max(abs(cx - p[0]), abs(cy - p[1])) - 500   # to the tile's nearest edge (Chebyshev)
    near = min(dist((PLACES[k]["E"], PLACES[k]["N"])) for k in ("summit", "solvay", "hornli"))
    lake = dist((PLACES["riffelsee"]["E"], PLACES["riffelsee"]["N"]))
    if near < 1200 or lake < 600:
        return 2
    if near < 3500 or lake < 1500:
        return 4
    return 8


def build():
    import cv2
    os.makedirs(OUT, exist_ok=True)
    os.makedirs(os.path.join(OUT, "tiles"), exist_ok=True)
    os.makedirs(os.path.join(OUT, "images"), exist_ok=True)
    print("near terrain")
    cop = copernicus()
    H, missing = near_dem(cop)
    # Smoothed copies for the coarser tiles (no aliasing).
    Hs = {2: H, 4: cv2.GaussianBlur(H, (0, 0), 0.8), 8: cv2.GaussianBlur(H, (0, 0), 1.6)}
    tiles = []
    ecount, ncount = (E1 - E0) // 1000, (N1 - N0) // 1000
    lods = {(e, n): lod_of(e, n) for e in range(E0 // 1000, E1 // 1000) for n in range(N0 // 1000, N1 // 1000)}

    # The far land first (the near tiles' outer edges follow it).
    print("far terrain")
    fe = np.arange(FAR_E0, FAR_E1 + 1, FAR_S)
    fn = np.arange(FAR_N1, FAR_N0 - 1, -FAR_S)
    FE, FN = np.meshgrid(fe, fn)
    F = cop(FE, FN)
    # Inside and near the box the swiss terrain: an offset that brings the Copernicus heights to it, faded out 3 km beyond.
    inside = (FE >= E0) & (FE <= E1) & (FN >= N0) & (FN <= N1)
    S = bilinear(H, np.clip(FE, E0 + 1, E1 - 1).astype(np.float32), np.clip(FN, N0 + 1, N1 - 1).astype(np.float32), E0, N1, 2.0)
    off = np.where(inside, S - F, 0).astype(np.float32)
    w = inside.astype(np.float32)
    k = int(3000 / FAR_S)
    offb = cv2.GaussianBlur(off, (0, 0), k / 2)
    wb = cv2.GaussianBlur(w, (0, 0), k / 2)
    fade = np.clip(1.0 - np.maximum.reduce([E0 - FE, FE - E1, N0 - FN, FN - N1, np.zeros_like(FE)]) / 3000.0, 0, 1)
    F = np.where(inside, S, F + np.where(wb > 1e-3, offb / np.maximum(wb, 1e-3), 0) * fade)
    F = F - curvature(FE, FN)
    far_z = F.astype(np.float32)
    for tj in range(0, len(fn) - 1, FAR_TILE):
        for ti in range(0, len(fe) - 1, FAR_TILE):
            Z = far_z[tj:tj + FAR_TILE + 1, ti:ti + FAR_TILE + 1]
            west, north = fe[ti], fn[tj]
            name = f"far_{ti // FAR_TILE}_{tj // FAR_TILE}"
            write_tile(os.path.join(OUT, "tiles", name + ".bin"), west, north, FAR_S, Z)
            tiles.append({"name": name, "kind": "far", "west": float(west), "north": float(north), "spacing": FAR_S,
                          "nx": int(Z.shape[1]), "ny": int(Z.shape[0]), "hole": [E0, N0, E1, N1]})

    def far_at(E, N):
        return bilinear(far_z, E, N, FAR_E0 - FAR_S / 2, FAR_N1 + FAR_S / 2, FAR_S)

    print("near tiles")
    for (e, n), s in lods.items():
        west, north = e * 1000.0, (n + 1) * 1000.0
        m = int(1000 / s)
        ge = west + np.arange(m + 1) * s
        gn = north - np.arange(m + 1) * s
        GE, GN = np.meshgrid(ge, gn)
        Z = bilinear(Hs[s], GE.astype(np.float32), GN.astype(np.float32), E0, N1, 2.0) - curvature(GE, GN)
        # Stitch each edge to a coarser neighbour (or, at the box's edge, to the far land's 50 m).
        for side, (de, dn) in {"w": (-1, 0), "e": (1, 0), "n": (0, 1), "s": (0, -1)}.items():
            nb = lods.get((e + de, n + dn))
            cs = nb if nb is not None else FAR_S
            if cs <= s:
                continue
            if side in "we":
                col = 0 if side == "w" else m
                Ecol = ge[col]
                coarse_n = np.arange(north, north - 1000 - 1e-6, -cs)
                if nb is None:
                    cz = far_at(np.full(coarse_n.shape, Ecol, np.float32), coarse_n.astype(np.float32)).ravel()
                else:
                    cz = (bilinear(Hs[cs], np.full(coarse_n.shape, Ecol, np.float32), coarse_n.astype(np.float32), E0, N1, 2.0)
                          .ravel() - curvature(Ecol, coarse_n))
                Z[:, col] = np.interp(-gn, -coarse_n, cz)
            else:
                row = 0 if side == "n" else m
                Nrow = gn[row]
                coarse_e = np.arange(west, west + 1000 + 1e-6, cs)
                if nb is None:
                    cz = far_at(coarse_e.astype(np.float32), np.full(coarse_e.shape, Nrow, np.float32)).ravel()
                else:
                    cz = (bilinear(Hs[cs], coarse_e.astype(np.float32), np.full(coarse_e.shape, Nrow, np.float32), E0, N1, 2.0)
                          .ravel() - curvature(coarse_e, Nrow))
                Z[row, :] = np.interp(ge, coarse_e, cz)
        name = f"near_{e}_{n}"
        write_tile(os.path.join(OUT, "tiles", name + ".bin"), west, north, float(s), Z)
        tiles.append({"name": name, "kind": "near", "west": west, "north": north, "spacing": float(s),
                      "nx": m + 1, "ny": m + 1, "block": block_of(e, n)})

    print("images")
    img = near_image()
    alb, fit, shadow2 = delight(img, H, 2.0)
    for bx in range(3):
        for by in range(3):
            bw, bn = E0 + bx * 8000, N1 - by * 8000
            r0, c0 = by * 4000, bx * 4000
            part = np.zeros((4000, 4000, 4), np.float32)
            src = alb[r0:r0 + 4000, c0:c0 + 4000]
            part[:src.shape[0], :src.shape[1], :3] = src
            part[:src.shape[0], :src.shape[1], 3] = 1.0 - missing[r0:r0 + 4000, c0:c0 + 4000]
            part = cv2.resize(part, (4096, 4096), interpolation=cv2.INTER_AREA)
            cv2.imwrite(os.path.join(OUT, "images", f"T_MH_Ortho_{bx}{by}.png"), (np.clip(part, 0, 1) * 255 + 0.5).astype(np.uint8))
    blocks = [{"name": f"T_MH_Ortho_{bx}{by}", "west": E0 + bx * 8000, "north": N1 - by * 8000, "size": 8000.0}
              for bx in range(3) for by in range(3)]

    print("high-resolution tiles")
    hires = []
    for t in HIRES:
        e, n = map(int, t.split("-"))
        h5 = _read(os.path.join(RAW, "swiss", f"swissalti3d_0.5_{t}.tif")).astype(np.float32)
        im = _read(os.path.join(RAW, "swiss", f"swissimage-dop10_0.1_{t}.tif"))
        if h5 is None or im is None:
            print("missing", t)
            continue
        im = cv2.resize(im[:, :, :3], (HIRES_PX, HIRES_PX), interpolation=cv2.INTER_AREA)
        hz = cv2.resize(h5, (HIRES_PX, HIRES_PX), interpolation=cv2.INTER_LINEAR)
        r0, c0 = (N1 // 1000 - n - 1) * 500, (e - E0 // 1000) * 500
        big = cv2.resize(shadow2[r0:r0 + 500, c0:c0 + 500], (HIRES_PX, HIRES_PX), interpolation=cv2.INTER_LINEAR)
        a, _, _ = delight(im, hz, 1000.0 / HIRES_PX, dict(fit), big)
        cv2.imwrite(os.path.join(OUT, "images", f"T_MH_Hires_{e}_{n}.png"), (np.clip(a, 0, 1) * 255 + 0.5).astype(np.uint8))
        hires.append({"name": f"T_MH_Hires_{e}_{n}", "west": e * 1000.0, "north": (n + 1) * 1000.0, "size": 1000.0})
        # The 0.5 m terrain for patches round the places (300 m square), written as its own tiles.
    patches = []
    for key, p in PLACES.items():
        if key == "glacier":
            continue
        cx, cy = round(p["E"] / 50) * 50, round(p["N"] / 50) * 50
        west, north = cx - 150.0, cy + 150.0
        GE, GN = np.meshgrid(west + np.arange(601) * 0.5, north - np.arange(601) * 0.5)
        Z = hires_height(GE, GN, H) - curvature(GE, GN)
        name = f"patch_{key}"
        write_tile(os.path.join(OUT, "tiles", name + ".bin"), west, north, 0.5, Z.astype(np.float32))
        patches.append({"name": name, "kind": "patch", "west": west, "north": north, "spacing": 0.5, "nx": 601, "ny": 601,
                        "block": block_of(int(cx // 1000), int(cy // 1000))})
    tiles += patches

    # The places: their ground (or the lake's surface) and the turn that puts their view north.
    places = {}
    for key, p in PLACES.items():
        g = float(hires_height(np.array([[p["E"]]]), np.array([[p["N"]]]), H)[0, 0])
        yaw = 0.0
        if p["look"]:
            dE, dN = p["look"][0] - p["E"], p["look"][1] - p["N"]
            bearing = math.degrees(math.atan2(dE, dN)) % 360
            yaw = (360.0 - bearing) % 360
        places[key] = {"E": p["E"], "N": p["N"], "ground": g, "ground_curved": g - float(curvature(p["E"], p["N"])), "yaw": yaw}
        print(f"{key}: ground {g:.1f} m, yaw {yaw:.1f}")
    with open(os.path.join(OUT, "terrain.json"), "w", encoding="utf-8") as f:
        json.dump({"tiles": tiles, "blocks": blocks, "hires": hires, "places": places, "fit": fit,
                   "summit": SUMMIT, "r_eff": R_EFF}, f, indent=1)
    print(len(tiles), "tiles written")


_HIRES_CACHE = {}


def hires_height(GE, GN, H):
    """The 0.5 m terrain where there is some, else the 2 m."""
    import cv2
    out = bilinear(H, GE.astype(np.float32), GN.astype(np.float32), E0, N1, 2.0).astype(np.float64)
    for t in HIRES:
        e, n = map(int, t.split("-"))
        if t not in _HIRES_CACHE:
            _HIRES_CACHE[t] = _read(os.path.join(RAW, "swiss", f"swissalti3d_0.5_{t}.tif")).astype(np.float32)
        inside = (GE >= e * 1000) & (GE < e * 1000 + 1000) & (GN > n * 1000) & (GN <= n * 1000 + 1000)
        if inside.any():
            v = bilinear(_HIRES_CACHE[t], GE.astype(np.float32), GN.astype(np.float32), e * 1000.0, n * 1000.0 + 1000, 0.5)
            out = np.where(inside, v, out)
    return out


def block_of(e, n):
    bx = (e * 1000 - E0) // 8000
    by = (N1 - (n + 1) * 1000) // 8000
    return f"T_MH_Ortho_{bx}{by}"


def places():
    """The lake's outline and the car's height at each place (its floor clear of the ground under all of it), added to terrain.json."""
    import cv2
    with open(os.path.join(OUT, "terrain.json"), encoding="utf-8") as f:
        T = json.load(f)
    H = np.zeros((1, 1), np.float32)
    # Riffelsee: the flat water at 2756.6 m in the 0.5 m terrain round the lake.
    e0, n1 = 2624800.0, 1092700.0
    GE, GN = np.meshgrid(e0 + np.arange(1000) * 0.5, n1 - np.arange(800) * 0.5)
    Z = hires_height(GE, GN, near_dem_cached())
    level = float(np.median(Z[(np.abs(GE - 2625065) < 20) & (np.abs(GN - 1092482) < 10)]))
    gy, gx = np.gradient(Z, 0.5)
    water = ((np.abs(Z - level) < 0.12) & (np.hypot(gx, gy) < 0.03)).astype(np.uint8)
    water = cv2.morphologyEx(water, cv2.MORPH_CLOSE, np.ones((5, 5), np.uint8))
    n, lab, stats, cent = cv2.connectedComponentsWithStats(water)
    k = 1 + int(np.argmax(stats[1:, cv2.CC_STAT_AREA]))
    mask = (lab == k).astype(np.uint8)
    cs, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    c = max(cs, key=cv2.contourArea)
    c = cv2.approxPolyDP(c, 1.0, True)[:, 0, :]
    poly = [[float(e0 + x * 0.5), float(n1 - y * 0.5)] for x, y in c]
    print(f"Riffelsee: level {level:.2f} m, {len(poly)} points, {int(mask.sum() * 0.25)} m2")
    T["lake"] = {"level": level, "level_curved": level - float(curvature(np.array(2625065.0), np.array(1092482.0))), "outline": poly}
    # The places, from PLACES (they may have moved since the tiles were built).
    for key, p in PLACES.items():
        g = float(hires_height(np.array([[p["E"]]]), np.array([[p["N"]]]), near_dem_cached())[0, 0])
        yaw = 0.0
        if p["look"]:
            bearing = math.degrees(math.atan2(p["look"][0] - p["E"], p["look"][1] - p["N"])) % 360
            yaw = (360.0 - bearing) % 360
        T["places"][key] = {"E": p["E"], "N": p["N"], "ground": g, "ground_curved": g - float(curvature(p["E"], p["N"])), "yaw": yaw}
    # The car: its floor 0.3 m over the highest ground under its 2.4 m radius (and over the lake, 0.3 m over the water).
    for key, p in T["places"].items():
        GE, GN = np.meshgrid(p["E"] + np.arange(-6, 7) * 0.4, p["N"] + np.arange(-6, 7) * 0.4)
        inside = np.hypot(GE - p["E"], GN - p["N"]) <= 2.45
        z = hires_height(GE, GN, near_dem_cached())
        top = float(z[inside].max())
        p["floor"] = max(top, p["ground"]) + 0.3
        want = PLACES[key].get("floor")
        if want is not None:
            if want < top + 0.2:
                print(f"{key}: the floor asked for ({want:.1f}) is under the ground ({top:.1f}): kept above it")
            p["floor"] = max(want, top + 0.2)
        p["floor_curved"] = p["floor"] - float(curvature(np.array(p["E"]), np.array(p["N"])))
        print(f"{key}: ground {p['ground']:.1f}, highest under the car {top:.1f}, floor {p['floor']:.1f}, yaw {p['yaw']:.1f}")
    # The Hörnli ridge's crest, from the hut to the summit: every 2 m along the line between them, the highest point
    # across a 60 m window, smoothed (for the rope teams and the fixed ropes).
    H = near_dem_cached()
    hut = np.array([2618472.0, 1092316.0])
    top = np.array(SUMMIT)
    d = top - hut
    L = float(np.linalg.norm(d))
    u = d / L
    perp = np.array([-u[1], u[0]])
    crest = []
    for st in np.arange(0.0, L, 2.0):
        c = hut + u * st
        offs = np.arange(-30.0, 30.0, 0.5)
        pts = c[None, :] + offs[:, None] * perp[None, :]
        z = hires_height(pts[:, 0:1], pts[:, 1:2], H)[:, 0]
        k = int(np.argmax(z))
        crest.append([float(pts[k, 0]), float(pts[k, 1]), float(z[k])])
    crest = np.array(crest)
    from scipy.ndimage import uniform_filter1d
    crest[:, 0] = uniform_filter1d(crest[:, 0], 5)
    crest[:, 1] = uniform_filter1d(crest[:, 1], 5)
    crest[:, 2] = hires_height(crest[:, 0:1], crest[:, 1:2], H)[:, 0]
    T["crest"] = [[float(e), float(n), float(z - curvature(np.array(e), np.array(n)))] for e, n, z in crest]
    print(f"crest: {len(crest)} points, {crest[0, 2]:.0f} to {crest[-1, 2]:.0f} m")
    # Zermatt's lights: points over the valley floor round the village (below 1,720 m, gentle), denser at its heart.
    rng = np.random.default_rng(1865)
    zc = np.array([2624350.0, 1096150.0])
    pts = zc + rng.normal(0, 1, (9000, 2)) * np.array([420.0, 700.0])
    zz = hires_height(pts[:, 0:1], pts[:, 1:2], H)[:, 0]
    gx = hires_height(pts[:, 0:1] + 5, pts[:, 1:2], H)[:, 0] - zz
    gy = hires_height(pts[:, 0:1], pts[:, 1:2] + 5, H)[:, 0] - zz
    ok = (zz < 1720) & (np.hypot(gx, gy) / 5 < 0.45)
    lights = [[float(e), float(n), float(z - curvature(np.array(e), np.array(n)) + 3.0)] for (e, n), z in zip(pts[ok][:1800], zz[ok][:1800])]
    T["lights"] = lights
    print(f"Zermatt: {len(lights)} lights")
    # Boulders round the places (the terrain models smooth them away): on rock and scree and meadow, not on snow, ice or
    # water, slopes under 42°; many small, few large (a power law), 3–90 m from the car.
    import cv2
    rng = np.random.default_rng(4478)
    boulders = []
    lake_poly = np.array(T["lake"]["outline"]) if "lake" in T else None
    for key in ("riffelsee", "hornli", "solvay", "summit"):
        p = T["places"][key]
        n_want, got = 320, 0
        for _ in range(8000):
            if got >= n_want:
                break
            a = rng.uniform(0, 2 * math.pi)
            r = math.sqrt(rng.uniform(3.2 ** 2, 90.0 ** 2))
            E, N = p["E"] + r * math.cos(a), p["N"] + r * math.sin(a)
            if lake_poly is not None and cv2.pointPolygonTest(lake_poly.astype(np.float32), (float(E), float(N)), True) > -1.5:
                continue
            z = float(hires_height(np.array([[E]]), np.array([[N]]), H)[0, 0])
            zx = float(hires_height(np.array([[E + 1.0]]), np.array([[N]]), H)[0, 0]) - z
            zy = float(hires_height(np.array([[E]]), np.array([[N + 1.0]]), H)[0, 0]) - z
            if math.degrees(math.atan(math.hypot(zx, zy))) > 42:
                continue
            e, n = int(E // 1000), int(N // 1000)
            img = _hires_image(e, n)
            if img is not None:
                px = int((E - e * 1000) / 1000 * img.shape[1])
                py = int(((n + 1) * 1000 - N) / 1000 * img.shape[0])
                b, g, rr = img[min(max(py, 0), img.shape[0] - 1), min(max(px, 0), img.shape[1] - 1)] / 255.0
                lum = 0.2126 * rr + 0.7152 * g + 0.0722 * b
                sat = (max(rr, g, b) - min(rr, g, b)) / (max(rr, g, b) + 1e-3)
                if lum > 0.62 and sat < 0.2:
                    continue
            size = float(np.clip(0.25 * (1.0 - rng.uniform()) ** -0.55, 0.25, 3.2))
            boulders.append([key, E, N, z - float(curvature(np.array(E), np.array(N))), float(rng.uniform(0, 360)), size, int(rng.integers(0, 6))])
            got += 1
    T["boulders"] = boulders
    print(f"boulders: {len(boulders)}")
    # The buildings: where they stand (their ground floor on the terrain at their centre) and how they are turned.
    T["buildings"] = {}
    for key, (E, N, bearing, rise) in BUILDINGS.items():
        g = float(hires_height(np.array([[E]]), np.array([[N]]), near_dem_cached())[0, 0]) + rise
        T["buildings"][key] = {"E": E, "N": N, "z": g - float(curvature(np.array(E), np.array(N))), "bearing": bearing}
        print(f"building {key}: ground floor {g:.1f} m")
    with open(os.path.join(OUT, "terrain.json"), "w", encoding="utf-8") as f:
        json.dump(T, f, indent=1)


_HIRES_IMG = {}


def _hires_image(e, n):
    import cv2
    k = (e, n)
    if k not in _HIRES_IMG:
        path = os.path.join(OUT, "images", f"T_MH_Hires_{e}_{n}.png")
        _HIRES_IMG[k] = cv2.imread(path) if os.path.exists(path) else None
    return _HIRES_IMG[k]


def cpp():
    """Source/MuseeVision/Cube/MatterhornData.inl: the places and the lake for the game (it can't read SourceArt)."""
    with open(os.path.join(OUT, "terrain.json"), encoding="utf-8") as f:
        T = json.load(f)
    L = ["// Generated by Scripts/journey_terrain.py cpp from SourceArt/Journeys/Matterhorn/terrain/terrain.json. Do not edit.",
         "#pragma once", "", "namespace MatterhornData", "{",
         "	struct FPlaceData { const TCHAR* Key; double E, N, Floor; };",
         f"	constexpr double SummitE = {SUMMIT[0]:.2f}, SummitN = {SUMMIT[1]:.2f};",
         f"	constexpr double REff = {R_EFF:.1f};",
         "	// The car's floor at each place (curvature taken off as in the tiles), metres."]
    L.append("	static const FPlaceData Places[] = {")
    for k, p in T["places"].items():
        L.append(f'		{{TEXT("{k}"), {p["E"]:.2f}, {p["N"]:.2f}, {p.get("floor_curved", p["ground_curved"] + 0.3):.3f}}},')
    L.append("	};")
    lake = T.get("lake")
    if lake:
        L.append(f"	constexpr double LakeLevel = {lake['level_curved']:.3f};")
        L.append("	static const double LakeOutline[][2] = {")
        for e, n in lake["outline"]:
            L.append(f"		{{{e:.2f}, {n:.2f}}},")
        L.append("	};")
    L.append("	// The Hörnli ridge's crest (E, N, z curved), hut to summit, every 2 m.")
    L.append("	static const double Crest[][3] = {")
    for e, n, z in T.get("crest", []):
        L.append(f"		{{{e:.2f}, {n:.2f}, {z:.2f}}},")
    L.append("	};")
    L.append("	// Zermatt's lights at night (E, N, z curved).")
    L.append("	static const float Lights[][3] = {")
    for e, n, z in T.get("lights", []):
        L.append(f"		{{{e:.1f}f, {n:.1f}f, {z:.1f}f}},")
    L.append("	};")
    L.append("	// Boulders round the places: E, N, z (curved), yaw, size (m), shape.")
    L.append("	struct FBoulderData { double E, N, Z; float Yaw, Size; int32 Shape; };")
    L.append("	static const FBoulderData Boulders[] = {")
    for key, E, N, z, yaw, size, shape in T.get("boulders", []):
        L.append(f"		{{{E:.2f}, {N:.2f}, {z:.2f}, {yaw:.1f}f, {size:.2f}f, {shape}}},")
    L.append("	};")
    L.append("	struct FBuildingData { const TCHAR* Key; double E, N, Z, Bearing; };")
    L.append("	static const FBuildingData Buildings[] = {")
    for k, b in T.get("buildings", {}).items():
        L.append(f'		{{TEXT("{k}"), {b["E"]:.2f}, {b["N"]:.2f}, {b["z"]:.3f}, {b["bearing"]:.1f}}},')
    L.append("	};")
    L.append("}")
    path = os.path.join(HERE, "..", "Source", "MuseeVision", "Cube", "MatterhornData.inl")
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(L) + "\n")
    print("wrote", os.path.normpath(path))


_NEAR = None


def near_dem_cached():
    global _NEAR
    if _NEAR is None:
        _NEAR, _ = near_dem(copernicus())
    return _NEAR


if __name__ == "__main__":
    {"fetch": fetch, "build": build, "places": places, "cpp": cpp}[sys.argv[1] if len(sys.argv) > 1 else "build"]()
