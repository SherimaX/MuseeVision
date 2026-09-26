"""
Chenghuai's turned vessels (Source/MuseeVision/Chenghuai/ChenghuaiVessel.h): the side halls' pieces that the Rooms board
leaves as "loan to select", chosen from the Metropolitan Museum's open-access collection (CC0), each modelled from its
museum photograph (assets/reference/chenghuai-ref-<name>.jpg):

  * the profile: the photo's silhouette (GrabCut), row by row, measured against the catalogue's size, the camera's
    slight elevation taken out (the lip's ellipse gives it);
  * the glaze and decoration: the front 140° of the photo unwrapped onto the lathe's UV (U round, V along the profile
    by length, as ChenghuaiVessel.cpp turns it), each point sampled where it shows in the photo (its section's ellipse),
    gone round twice with a cross-fade; the studio's highlights taken out; the unseen inside and underside given the
    glaze's and the body's own colours from the photo.

Plain Python (numpy, OpenCV, PIL):
    python chenghuai_vessels.py          # writes SourceArt/Textures/T_ch_v_<name>.png and ch_vessels.json (profiles)
"""
import json
import math
import os

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(HERE, "..", "SourceArt", "Textures")
PROFILES = os.path.join(OUT, "ch_vessels.json")
TEX_W, TEX_H = 2048, 1024

# name: the Met object, its size (m: "h" height, or "d" diameter), open (a bowl or pot: the inside shows) or closed,
# the wall's thickness, and the room it goes to.
VESSELS = {
    "ding": dict(mono=True, met=42461, title="Basin with lotus decoration", h=0.115, d=0.248, open=True, t=0.004, room="Tianqing"),
    "jun": dict(mono=True, met=50242, title="Flower pot", d=0.203, open=True, t=0.008, room="Tianqing"),
    "guan": dict(mono=True, met=52611, title="Vase", h=0.34, d=0.216, open=False, t=0.006, room="Tianqing"),
    "jian": dict(mono=True, met=48117, title="Tea bowl", h=0.073, d=0.121, open=True, t=0.005, room="Tianqing"),
    "longquan": dict(mono=True, met=51050, title="Bottle vase", h=0.146, d=0.083, open=False, t=0.005, room="Tianqing"),
    "xuande": dict(met=39666, title="Jar with dragon", h=0.483, d=0.483, open=False, t=0.008, room="Changnan"),
    "wucai": dict(met=42549, title="Jar with carp in lotus pond", h=0.232, open=False, t=0.006, room="Changnan"),
    "falangcai": dict(met=42319, title="Bowl with flowers", h=0.054, d=0.114, open=True, t=0.003, room="Changnan"),
}


def ref_path(name):
    return os.path.join(REPO, "assets", "reference", f"chenghuai-ref-{name}.jpg")


def silhouette(img):
    """The vessel's mask: GrabCut from a rectangle inside the frame's margins, cleaned to its largest part, holes filled."""
    import cv2
    import numpy as np
    from scipy import ndimage
    h, w = img.shape[:2]
    s = 900.0 / max(h, w)
    small = cv2.resize(img, (int(w * s), int(h * s)), interpolation=cv2.INTER_AREA)
    mask = np.zeros(small.shape[:2], np.uint8)
    rect = (int(small.shape[1] * 0.04), int(small.shape[0] * 0.03), int(small.shape[1] * 0.92), int(small.shape[0] * 0.95))
    bg, fg = np.zeros((1, 65), np.float64), np.zeros((1, 65), np.float64)
    cv2.grabCut(small, mask, rect, bg, fg, 8, cv2.GC_INIT_WITH_RECT)
    m = ((mask == cv2.GC_FGD) | (mask == cv2.GC_PR_FGD)).astype(np.uint8)
    m = cv2.resize(m, (w, h), interpolation=cv2.INTER_NEAREST)
    lab, n = ndimage.label(m)
    if n > 1:
        sizes = ndimage.sum(m, lab, range(1, n + 1))
        m = (lab == 1 + int(np.argmax(sizes))).astype(np.uint8)
    m = ndimage.binary_fill_holes(ndimage.binary_opening(m, iterations=3)).astype(np.uint8)
    return m


def measure(name, spec):
    """From the photo: each row's centre and half-width; the lip's and the foot's rows; the scale; the elevation."""
    import numpy as np
    import cv2
    img = cv2.imread(ref_path(name))[:, :, ::-1]
    k = 1600.0 / max(img.shape[:2])
    img = cv2.resize(img, (int(img.shape[1] * k), int(img.shape[0] * k)), interpolation=cv2.INTER_AREA)
    m = silhouette(img)
    rows = np.where(m.any(1))[0]
    top, bot = int(rows[0]), int(rows[-1])
    # The vessel is turned: one axis for all rows (the upper rows', clear of any cast shadow), each row's half-width
    # the nearer of its two edges (a shadow on the floor widens one side only).
    edges = {}
    for y in range(top, bot + 1):
        xs = np.where(m[y])[0]
        edges[y] = (xs[0], xs[-1])
    upper = [0.5 * (edges[y][0] + edges[y][1]) for y in range(top, top + max(3, int(0.3 * (bot - top))))]
    axis = float(np.median(upper))
    cx = np.full(m.shape[0], axis)
    hw = np.zeros(m.shape[0])
    for y in range(top, bot + 1):
        hw[y] = max(0.0, min(axis - edges[y][0], edges[y][1] - axis))
    solid = [y for y in range(top, bot + 1) if hw[y] > 0.15 * hw[top:bot + 1].max()]
    bot = max(solid)
    # The mask made symmetric (for the sampling's inside test).
    xx = np.arange(m.shape[1])[None, :]
    m = ((np.abs(xx - axis) <= hw[:, None]) & m.astype(bool)).astype(np.uint8)
    # The lip: the widest row near the top (its ellipse's major axis); its minor half-axis is lip_row - top.
    if spec["open"]:
        # A bowl's rim is its widest section near the top: the ellipse's major axis row; its minor half-axis above.
        band = max(4, int(0.12 * (bot - top)))
        lip_row = top + int(np.argmax(hw[top:top + band]))
        b_lip = max(lip_row - top, 1)
        sin_e = min(0.35, b_lip / max(hw[lip_row], 1.0))
    else:
        # A jar's or vase's mouth is narrower than its body: take the camera a little above (the Met's usual 7°).
        sin_e = math.sin(math.radians(7.0))
        lip_row = top + int(round(hw[top + 2] * sin_e))
    cos_e = math.sqrt(1.0 - sin_e * sin_e)
    # The foot: the silhouette's bottom is the foot ring's near edge, its section's centre b_foot above.
    foot_hw = np.median(hw[max(top, bot - 6):bot + 1])
    foot_row = bot - foot_hw * sin_e
    height_px = (foot_row - lip_row) / cos_e
    # The scale from the diameter when the catalogue gives it (the silhouette's width is the surest measure).
    if "d" in spec:
        ppm = 2.0 * hw[top:bot + 1].max() / spec["d"]
    else:
        ppm = height_px / spec["h"]
    return dict(img=img.astype(np.float32) / 255.0, mask=m, cx=cx, hw=hw, top=top, bot=bot, lip_row=lip_row, foot_row=foot_row,
                sin_e=sin_e, cos_e=cos_e, ppm=ppm, height=height_px / ppm)


def outer_profile(M, n=40):
    """(r, z) up the outer wall from the foot's edge to the lip, metres. A row at height z is the section whose centre
    projects there: row = foot_row - z·ppm·cos e; its half-width is r·ppm (the silhouette's width is the section's)."""
    import numpy as np
    pts = []
    H = M["height"]
    for k in range(n + 1):
        z = H * k / n
        y = M["foot_row"] - z * M["ppm"] * M["cos_e"]
        # The silhouette's widest extent at that height is the section's radius, but its row is shifted down by the
        # ellipse: search a little below for the half-width of this section.
        y0 = int(round(y))
        y1 = int(min(M["bot"], round(y + M["hw"][int(np.clip(y0, M["top"], M["bot"]))] * M["sin_e"] * 0.0)))
        yy = int(np.clip(max(y0, y1), M["top"], M["bot"]))
        pts.append((float(M["hw"][yy] / M["ppm"]), float(z)))
    # Smooth a little (the mask's steps).
    r = np.array([p[0] for p in pts])
    r[1:-1] = 0.25 * r[:-2] + 0.5 * r[1:-1] + 0.25 * r[2:]
    return [(float(r[i]), pts[i][1]) for i in range(len(pts))]


def full_profile(spec, outer):
    """The walk the C++ turns: the foot's centre, out under the foot, up the outer wall, over the lip, down inside."""
    t = spec["t"]
    r0 = outer[0][0]
    zl = outer[-1][1]
    rl = outer[-1][0]
    walk = [(0.0, 0.004), (max(r0 - 0.006, 0.001), 0.004), (max(r0 - 0.004, 0.002), 0.0), (r0, 0.0)]
    walk += [p for p in outer[1:]]
    walk += [(rl - 0.5 * t, zl + 0.35 * t), (rl - t, zl)]
    if spec["open"]:
        # Down the inside at the wall's thickness to the well.
        inner = [(max(r - t, 0.001), z) for (r, z) in reversed(outer[2:-1]) if z > 2.2 * t]
        walk += inner
        walk += [(0.0, max(2.2 * t, outer[1][1]))]
    else:
        # A short way down the neck, then closed (never seen).
        walk += [(rl - t, max(zl - 0.03, zl * 0.8)), (0.0, max(zl - 0.03, zl * 0.8))]
    return walk


def arc(walk):
    s = [0.0]
    for a, b in zip(walk, walk[1:]):
        s.append(s[-1] + math.dist(a, b))
    return [x / s[-1] for x in s]


def unwrap(name, spec):
    import numpy as np
    from scipy import ndimage
    M = measure(name, spec)
    outer = outer_profile(M)
    if "h" in spec and "d" in spec:
        # Both sizes known: the heights follow the catalogue (the camera's elevation is the uncertain measure).
        k = spec["h"] / outer[-1][1]
        outer = [(r, z * k) for r, z in outer]
        M["zscale"] = k
    else:
        M["zscale"] = 1.0
    walk = full_profile(spec, outer)
    vs = arc(walk)
    img = M["img"]
    H_, W_ = img.shape[:2]
    # The studio's highlights: pixels well above their neighbourhood's median, replaced by it.
    lum = img.mean(2)
    small = lum[::4, ::4]
    med = ndimage.zoom(ndimage.median_filter(small, size=7), 4, order=1)[:lum.shape[0], :lum.shape[1]]
    hot = (lum - med) > 0.1
    hot = ndimage.binary_dilation(hot, iterations=3) & M["mask"].astype(bool)
    if hot.any():
        for c in range(3):
            ch = img[:, :, c]
            cm_ = ndimage.zoom(ndimage.median_filter(ch[::4, ::4], size=9), 4, order=1)[:ch.shape[0], :ch.shape[1]]
            ch[hot] = cm_[hot]
    lip_i = len(outer) + 3          # the walk's index of the lip (after the 4 foot points and the outer wall's)
    out = np.zeros((TEX_H, TEX_W, 3), np.float32)
    u = (np.arange(TEX_W) + 0.5) / TEX_W
    t = u * 2 * np.pi
    SPAN, FADE = math.radians(120.0), math.radians(30.0)
    P = SPAN - FADE
    xa = (t % np.pi) / np.pi * P
    phi = -SPAN / 2 + xa
    phi_next = phi + P
    w_next = np.clip(1.0 - xa / FADE, 0, 1)

    maskf = M["mask"].astype(np.float32)

    def sample(r, z, ph, with_mask=False):
        z = z / M["zscale"]
        yc = M["foot_row"] - z * M["ppm"] * M["cos_e"]
        cxr = M["cx"][int(np.clip(round(yc), M["top"], M["bot"]))]
        x = cxr + 0.97 * r * M["ppm"] * np.sin(ph)
        y = yc + r * M["ppm"] * M["sin_e"] * np.cos(ph)
        yy, xx = np.clip(y, 0, H_ - 1), np.clip(x, 0, W_ - 1)
        rgb = np.stack([ndimage.map_coordinates(img[:, :, c], [yy, xx], order=1) for c in range(3)], -1)
        if with_mask:
            return rgb, ndimage.map_coordinates(maskf, [yy, xx], order=1)
        return rgb

    outer_rows = []
    for row in range(TEX_H):
        v = (row + 0.5) / TEX_H
        i = max(0, min(len(vs) - 2, next((k for k in range(len(vs) - 1) if vs[k] <= v <= vs[k + 1]), len(vs) - 2)))
        f = (v - vs[i]) / max(vs[i + 1] - vs[i], 1e-9)
        r = walk[i][0] + f * (walk[i + 1][0] - walk[i][0])
        z = walk[i][1] + f * (walk[i + 1][1] - walk[i][1])
        if 3 <= i < lip_i:
            (a, ma), (b, mb) = sample(r, max(z, 0.0), phi, True), sample(r, max(z, 0.0), phi_next, True)
            row_rgb = a * (1 - w_next[:, None]) + b * w_next[:, None]
            inside = (ma * (1 - w_next) + mb * w_next) > 0.5
            if inside.sum() > 8 and not inside.all():
                row_rgb[~inside] = np.median(row_rgb[inside], axis=0)   # sampled off the vessel: its own row's colour
            out[row] = row_rgb
            outer_rows.append(row)
    lo, hi = outer_rows[0], outer_rows[-1]
    # The studio's light taken out: each row's slow change round the vessel (the sides darker, a lit band) divided away,
    # keeping the decoration's own contrast; on the monochromes, the big reflections too.
    mono = spec.get("mono", False)
    if mono:
        seg = out[lo:hi + 1]
        l2 = seg.mean(2)
        med = np.median(l2, axis=1, keepdims=True)
        hotm = np.clip((l2 - med * 1.06) / (med * 0.1 + 1e-3), 0, 1)
        hotm = ndimage.gaussian_filter(ndimage.maximum_filter(hotm, 9), 5.0)
        medc = np.median(seg, axis=1, keepdims=True)
        out[lo:hi + 1] = seg * (1 - hotm[:, :, None]) + medc * hotm[:, :, None]
    lum = out[lo:hi + 1].mean(2)
    smooth = ndimage.gaussian_filter1d(lum, TEX_W / (40.0 if mono else 14.0), axis=1, mode="wrap")
    smooth = ndimage.gaussian_filter1d(smooth, 3.0, axis=0)
    target = np.median(smooth, axis=1, keepdims=True)
    out[lo:hi + 1] *= np.clip(target / np.maximum(smooth, 1e-3), 0.7, 1.5)[:, :, None]
    mid = out[lo + (hi - lo) // 4: hi - (hi - lo) // 5].reshape(-1, 3)
    if mono:
        body = np.median(mid, axis=0)
    else:
        # A painted piece's inside is its white ground: the brightest tenth of the outside (short of the reflections).
        l3 = mid.mean(1)
        sel = (l3 > np.percentile(l3, 85)) & (l3 < np.percentile(l3, 97))
        body = np.median(mid[sel], axis=0)
    foot_rgb = np.median(out[lo:lo + max(3, (hi - lo) // 20)].reshape(-1, 3), axis=0)
    out[:lo] = foot_rgb                              # under the foot: the bare body
    out[hi + 1:] = body                              # inside: the glaze's own colour
    # The inside a little mottled (glaze pools deeper toward the well), never flat.
    rng = np.random.default_rng(M["top"])
    noise = ndimage.gaussian_filter(rng.standard_normal((TEX_H // 8, TEX_W // 8)), 2.0)
    noise = ndimage.zoom(noise, 8, order=1)[:TEX_H, :TEX_W]
    out[hi + 1:] *= (1.0 + 0.03 * noise[hi + 1:, :, None] / (noise.std() + 1e-6))
    return walk, np.clip(out, 0, 1), M


# The court's lived-in things (Rules board: 天棚 鱼缸 石榴树): the fish bowl on the axis and the pomegranates' tubs,
# turned from drawn profiles, their glazes painted (no photograph: common garden wares).
COURT = {
    # A glazed stoneware fish bowl (鱼缸), 1.1 m across, 0.66 high, a rolled rim; green glaze thin over the brown body
    # at the rim and the foot.
    "fishbowl": dict(profile=[(0.0, 0.01), (0.34, 0.01), (0.36, 0.0), (0.4, 0.0), (0.42, 0.04), (0.5, 0.2), (0.55, 0.42), (0.56, 0.58),
                              (0.575, 0.63), (0.585, 0.655), (0.57, 0.668), (0.545, 0.66), (0.535, 0.6), (0.52, 0.42), (0.46, 0.18),
                              (0.36, 0.06), (0.0, 0.05)], glaze=(62, 74, 60), rim=(88, 72, 52), foot=(100, 82, 62), height=0.668, open=True),
    # A pomegranate's tub: a dark-glazed pot 0.62 across and 0.46 high.
    "tub": dict(profile=[(0.0, 0.01), (0.2, 0.01), (0.21, 0.0), (0.24, 0.0), (0.25, 0.03), (0.28, 0.2), (0.305, 0.4), (0.315, 0.445),
                         (0.31, 0.46), (0.29, 0.455), (0.285, 0.4), (0.26, 0.22), (0.23, 0.06), (0.0, 0.05)],
                glaze=(58, 44, 36), rim=(96, 72, 50), foot=(118, 90, 64), height=0.46, open=True),
}


def paint_court(name, spec):
    """A plain glaze: mottled, pooling darker low on the wall, thin (the body showing) at the rim and the foot."""
    import numpy as np
    from scipy import ndimage
    rng = np.random.default_rng(len(name) * 7)
    walk = spec["profile"]
    vs = arc(walk)
    lip = max(range(len(walk)), key=lambda i: (walk[i][1], walk[i][0]))
    out = np.zeros((TEX_H // 2, TEX_W // 2, 3), np.float32)
    H, W = out.shape[:2]
    g = np.array(spec["glaze"], np.float32) / 255
    mott = ndimage.gaussian_filter(rng.standard_normal((H // 4, W // 4)), 2.0)
    mott = ndimage.zoom(mott, 4, order=1)[:H, :W]
    for r in range(H):
        v = (r + 0.5) / H
        k = min(range(len(vs) - 1), key=lambda i: 0 if vs[i] <= v <= vs[i + 1] else min(abs(vs[i] - v), abs(vs[i + 1] - v)))
        col = g.copy()
        if k < 4:
            col = np.array(spec["foot"], np.float32) / 255
        elif abs(k - lip) <= 1:
            col = np.array(spec["rim"], np.float32) / 255
        out[r] = col * (1 + 0.08 * mott[r:r + 1].T[:, 0][:, None] / (mott.std() + 1e-6)) if False else col[None, :] * (1 + 0.08 * mott[r][:, None] / (mott.std() + 1e-6))
    return walk, np.clip(out, 0, 1)


def main():
    from PIL import Image
    os.makedirs(OUT, exist_ok=True)
    profiles = {}
    for name, spec in VESSELS.items():
        walk, tex, M = unwrap(name, spec)
        Image.fromarray((tex * 255).astype("uint8")).save(os.path.join(OUT, f"T_ch_v_{name}.png"))
        profiles[name] = dict(profile=[[round(r, 5), round(z, 5)] for r, z in walk], height=round(M["height"], 4),
                              radius=round(max(r for r, _ in walk), 4), elevation=round(math.degrees(math.asin(M["sin_e"])), 1),
                              **{k: v for k, v in spec.items()})
        print(name, "h %.3f (catalogue %s)  r %.3f  elev %.1f  points %d" % (M["height"], spec.get("h"), profiles[name]["radius"],
                                                                         profiles[name]["elevation"], len(walk)))
        # A check sheet: the photo with the measured silhouette's outline, and the texture.
        import numpy as np
        ov = (M["img"] * 255).astype("uint8").copy()
        edge = M["mask"] ^ np.pad(M["mask"], 1)[1:-1, 2:]
        ov[edge.astype(bool)] = (255, 0, 0)
        a = Image.fromarray(ov); a.thumbnail((600, 600))
        b = Image.fromarray((tex * 255).astype("uint8")).resize((1200, 600))
        sheet = Image.new("RGB", (1800, 600), (40, 40, 40)); sheet.paste(a, (0, 0)); sheet.paste(b, (600, 0))
        sheet.save(os.path.join(os.environ.get("TEMP", "."), f"ch_vessel_{name}.jpg"))
    for name, spec in COURT.items():
        walk, tex = paint_court(name, spec)
        Image.fromarray((tex * 255).astype("uint8")).save(os.path.join(OUT, f"T_ch_v_{name}.png"))
        profiles[name] = dict(profile=[[round(r, 5), round(z, 5)] for r, z in walk], height=spec["height"],
                              radius=round(max(r for r, _ in walk), 4), elevation=0.0, open=True)
        print(name, "painted")
    with open(PROFILES, "w", encoding="utf-8") as f:
        json.dump(profiles, f, indent=1)


if __name__ == "__main__":
    main()
