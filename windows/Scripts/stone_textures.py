"""
Detail images for the stone masters, generated here (numpy + Pillow, outside Unreal), seamless:

    python windows/Scripts/stone_textures.py

writes windows/SourceArt/Textures/*.png, which materials.py imports and multiplies into its stones
(UseTexture, UV0 in metres, TexScale metres per repeat). They carry what the procedural masters
lacked and the renderings have: a stone's own character.

- T_marble_crackle (2 m): the Rotunda's cream marble (rendering 05): a crackle of fine grey veins
  round cells 25-40 cm across, a finer set in patches, over a soft cloud.
- T_concrete_mottle (3 m): the Reserve's polished concrete floor (rendering 04): a trowelled
  cloud, darker burnish marks, fine pale aggregate.

- T_porphyry (1 m), T_verde_antico (1.5 m), T_pavonazzetto (2 m), T_giallo_antico (1.5 m), T_rosso_antico
  (1 m): the classical hall's coloured marbles, full-colour images (their materials' BaseColor is white).

Each is a multiplier near 1 (white = the material's own colour), built from periodic noise
(filtered in the Fourier domain), so it tiles without seams.
"""
import os

import numpy as np
from PIL import Image

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "SourceArt", "Textures")


def fbm(n, beta, seed, lo=2.0, hi=None):
    """Periodic noise with a 1/f^beta spectrum between wavelengths n/lo and n/hi pixels, normalised to ±1."""
    rng = np.random.default_rng(seed)
    f = np.fft.fftfreq(n) * n
    k = np.sqrt(f[:, None] ** 2 + f[None, :] ** 2)
    amp = np.where(k > 0, 1.0 / np.maximum(k, 1e-6) ** beta, 0.0)
    amp[k < lo] = 0.0
    if hi:
        amp[k > hi] = 0.0
    spec = amp * np.exp(2j * np.pi * rng.random((n, n)))
    field = np.real(np.fft.ifft2(spec))
    return field / (np.abs(field).max() + 1e-9)


def veins(field, levels, width):
    """Thin lines along level sets of a field: a meandering network, as marble's."""
    out = np.zeros_like(field)
    grad = np.hypot(*np.gradient(field)) + 1e-6
    for level in levels:
        d = np.abs(field - level) / grad          # distance to the level set, in pixels
        out = np.maximum(out, np.exp(-(d / width) ** 2))
    return out


def cell_edges(n, cells, seed, warp):
    """Distance (pixels) to the nearest edge of a periodic Voronoi pattern, its points warped: crackle."""
    from scipy.spatial import cKDTree
    rng = np.random.default_rng(seed)
    # Points no closer than 60 % of their mean spacing (periodic), so no cell collapses to a sliver.
    spacing = 0.6 * n / np.sqrt(cells)
    pts = []
    while len(pts) < cells:
        p = rng.random(2) * n
        if all(np.hypot(*((p - q + n / 2) % n - n / 2)) >= spacing for q in pts):
            pts.append(p)
    pts = np.array(pts)
    yy, xx = np.mgrid[0:n, 0:n].astype(np.float64)
    wx, wy = warp
    q = np.stack([(yy + wy) % n, (xx + wx) % n], axis=-1).reshape(-1, 2)
    d, _ = cKDTree(pts, boxsize=n).query(q, k=2)
    return ((d[:, 1] - d[:, 0]) / 2.0).reshape(n, n)


def marble_crackle(n=2048):
    # Voronoi cells some 25-40 cm across (2048 px = 2 m), their edges bent by noise so they wander and
    # kink like the rendering's veins, a second finer set inside some cells, and a soft cloud.
    w1 = (fbm(n, 2.0, 11, lo=2, hi=24) * 70, fbm(n, 2.0, 12, lo=2, hi=24) * 70)
    w2 = (fbm(n, 1.8, 13, lo=3, hi=40) * 35, fbm(n, 1.8, 14, lo=3, hi=40) * 35)
    main = np.exp(-(cell_edges(n, 38, 15, w1) / 1.0) ** 2)
    fine = np.exp(-(cell_edges(n, 160, 16, w2) / 0.9) ** 2)
    halo = np.exp(-(cell_edges(n, 38, 15, w1) / 7.0) ** 2) * 0.18       # the veins' soft shadows
    # Veins fade in and out along their length; the fine set shows only in patches.
    fade = np.clip(0.35 + 0.8 * fbm(n, 1.5, 17, lo=2, hi=40), 0.0, 1.0)
    patches = np.clip(fbm(n, 1.8, 18, lo=2, hi=16) * 1.6, 0.0, 1.0)
    v = np.clip(main * fade * 0.8 + fine * patches * 0.45 + halo * fade, 0.0, 1.0)
    cloud = fbm(n, 1.8, 19, lo=1.5, hi=30) * 0.04 + fbm(n, 1.2, 20, lo=40, hi=400) * 0.012
    grey = np.array([0.64, 0.62, 0.60])                        # the veins' grey, slightly warm
    base = 1.0 + cloud[..., None] * np.array([1.0, 0.97, 0.92])
    # Soft grey whispers, not cracks (rendering 05): at most about a fifth darker than the stone.
    img = base * (1.0 - v[..., None] * (1.0 - grey) * 0.42)
    return np.clip(img * 0.97, 0.0, 1.0)


def concrete_mottle(n=2048):
    cloud = fbm(n, 1.9, 21, lo=1.5, hi=25) * 0.10 + fbm(n, 1.5, 22, lo=20, hi=200) * 0.04
    burnish = np.clip(fbm(n, 2.1, 23, lo=2, hi=12), 0.25, 1.0) - 0.25   # darker trowel marks
    rng = np.random.default_rng(24)
    specks = (rng.random((n, n)) > 0.9965).astype(float)
    specks = np.maximum(specks, np.roll(specks, 1, 0) * 0.5)
    img = 1.0 + cloud - burnish * 0.12 + specks * 0.18
    img = np.repeat(img[..., None], 3, axis=2) * np.array([1.0, 0.995, 0.985])
    return np.clip(img * 0.93, 0.0, 1.0)


# --------------------------------------------------------------------------- the Square (Élan, level -1)

def fbm_rect(ny, nx, height_m, width_m, seed, lam_min, lam_max, beta=1.8, stretch=1.0):
    """
    Periodic noise on an ny x nx image covering height_m x width_m metres, normalised to ±1: a 1/f^beta
    spectrum between wavelengths lam_min and lam_max (m). stretch > 1 draws the features that much
    longer horizontally than vertically (layers, streaks).
    """
    rng = np.random.default_rng(seed)
    ky = np.fft.fftfreq(ny) * ny / height_m            # cycles per metre
    kx = np.fft.fftfreq(nx) * nx / width_m
    k = np.sqrt((kx[None, :] * stretch) ** 2 + ky[:, None] ** 2)
    amp = np.where(k > 0, 1.0 / np.maximum(k, 1e-9) ** beta, 0.0)
    amp[(k < 1.0 / lam_max) | (k > 1.0 / lam_min)] = 0.0
    spec = amp * np.exp(2j * np.pi * rng.random((ny, nx)))
    field = np.real(np.fft.ifft2(spec))
    return field / (np.abs(field).max() + 1e-12)


def srgb(h):
    return np.array([(h >> 16) & 255, (h >> 8) & 255, h & 255], dtype=np.float64) / 255.0


def stamp_pebbles(img, rng, count, size_mm, mpp, colours, where=None, flat=(1.0, 1.7), shade=0.12, alpha=0.9,
                  mute_max=0.0):
    """
    Rounded stones pressed into the surface, drawn with wrap-around (the image tiles): ellipses of
    size_mm = (min, max) (a power law, many small, few large), flattened by `flat` (rammed: lying
    down), each its own colour from `colours`, lit from above (a lighter top, a thin shadow under).
    mpp = (metres per pixel down, across). where(y, x) -> 0..1 thins them out by position.
    """
    ny, nx, _ = img.shape
    lo, hi = size_mm
    for _ in range(count):
        # Power law: d = lo * (hi/lo)^u^2.2 puts most stones near the small end.
        d = lo * (hi / lo) ** (rng.random() ** 2.2) / 1000.0
        cy, cx = rng.random() * ny, rng.random() * nx
        if where is not None and rng.random() > where(cy, cx):
            continue
        f = flat[0] + (flat[1] - flat[0]) * rng.random()
        ry_px, rx_px = d / 2 / f / mpp[0], d / 2 / mpp[1]
        ang = (rng.random() - 0.5) * 0.6                 # nearly horizontal
        r = int(np.ceil(max(ry_px, rx_px))) + 2
        ys = np.arange(int(cy) - r, int(cy) + r + 1)
        xs = np.arange(int(cx) - r, int(cx) + r + 1)
        yy, xx = np.meshgrid(ys - cy, xs - cx, indexing="ij")
        ca, sa = np.cos(ang), np.sin(ang)
        u = (xx * ca + yy * sa) / max(rx_px, 0.5)
        v = (-xx * sa + yy * ca) / max(ry_px, 0.5)
        q = u * u + v * v
        inside = np.clip((1.0 - q) * max(rx_px, ry_px) * 0.9 + 0.5, 0.0, 1.0)   # antialiased edge
        if not inside.any():
            continue
        # Irregular, not a clean ellipse: the outline wobbles with the angle.
        phi = np.arctan2(v, u)
        p1, p2 = rng.random(2) * 6.283
        q = q * (1.0 + 0.16 * np.sin(3 * phi + p1) + 0.09 * np.sin(5 * phi + p2)) ** 2
        inside = np.clip((1.0 - q) * max(rx_px, ry_px) * 0.9 + 0.5, 0.0, 1.0)
        col = colours[rng.integers(len(colours))] * (0.9 + 0.2 * rng.random())
        light = 1.0 + shade * np.clip(-v, -1, 1) * np.sqrt(np.clip(1 - q, 0, 1))  # the top catches the light
        under = np.clip(1.0 - np.abs(q - 1.15) * 4.0, 0.0, 1.0) * (v > 0)        # a thin shadow below it
        yi, xi = ys % ny, xs % nx
        patch = img[np.ix_(yi, xi)]
        # Half buried in the earth: some take on the matrix's colour.
        mute = rng.random() * mute_max
        col = col * (1 - mute) + patch.reshape(-1, 3).mean(axis=0) * mute
        stone = col[None, None, :] * light[..., None]
        a = (inside * alpha)[..., None]
        patch = patch * (1 - a) + stone * a
        patch *= (1.0 - 0.25 * under[..., None] * (1 - inside[..., None]))
        img[np.ix_(yi, xi)] = patch


def rammed_earth(ny=4096, nx=2048, height_m=7.5, width_m=2.8, seed=31):
    """
    The Square's rammed earth (Élan level -1), as Martin Rauch's walls and Herzog & de Meuron's
    Ricola: compacted horizontal lifts 5-15 cm thick with ragged edges, grouped in broader bands
    that drift between warm ochre, umber, sand and grey; each lift denser and finer at its top,
    coarser at its foot, with a thin compaction line under it; a lighter trass-lime erosion line
    every half metre or so; sand grain, mottling, pebbles and small voids. Matte.

    One image covers the full wall height (7.5 m, image top = the ceiling) by 2.8 m of wall, so the
    layering never repeats up a wall; it tiles across (and wraps top to bottom). This is the albedo
    itself (sRGB): the material's BaseColor stays near white. UV0: U along the wall, V = 7.5 m - the
    height above the floor (metres), TexScale 2.8, TexAspect 7.5/2.8.
    """
    rng = np.random.default_rng(seed)
    mpp = (height_m / ny, width_m / nx)
    y = (np.arange(ny) + 0.5) * mpp[0]                   # metres down from the top

    # Lifts: 5-15 cm (mostly 7-12), scaled to fill the height exactly.
    lifts = []
    while sum(lifts) < height_m:
        lifts.append(0.05 + 0.10 * rng.beta(2.2, 2.6))
    lifts = np.array(lifts) * height_m / sum(lifts)
    tops = np.concatenate([[0.0], np.cumsum(lifts)[:-1]])
    n = len(lifts)

    # Bands of 3-9 lifts: each band's colour a step of a slow walk through the earth's palette
    # (ochre, umber, sand, warm grey; now and then a darker or a redder band).
    palette = [srgb(h) for h in (0xAB8B67, 0x8F745C, 0x9F9080, 0xB49C7E, 0x7A634F, 0x9B9283, 0xA27C5E)]
    weights = np.array([0.25, 0.2, 0.15, 0.15, 0.07, 0.1, 0.08])
    lift_col = np.zeros((n, 3))
    soft = np.zeros(n)                                   # how diffuse each lift's top edge is (m)
    lime = np.zeros(n, dtype=bool)
    i, prev = 0, None
    since_lime = 0.0
    while i < n:
        k = int(rng.integers(3, 10))
        choice = rng.choice(len(palette), p=weights / weights.sum())
        base = palette[choice] if prev is None else 0.75 * palette[choice] + 0.25 * prev
        prev = base
        for j in range(i, min(n, i + k)):
            lift_col[j] = base * (1.0 + rng.normal(0, 0.022)) + rng.normal(0, 0.005, 3)
            soft[j] = 0.002 + 0.018 * rng.random() ** 1.5
            since_lime += lifts[j]
            if since_lime > 0.45 + 0.25 * rng.random():
                lime[j] = True
                since_lime = 0.0
        i += k

    # Where each pixel falls: the lift boundaries undulate along the wall (a centimetre or so over
    # half a metre to a few metres; each boundary its own) and are ragged at the scale of the grain.
    wave = fbm_rect(ny, nx, height_m, width_m, seed + 1, 0.05, 0.6, beta=1.6, stretch=3.0) * 0.016
    rag = fbm_rect(ny, nx, height_m, width_m, seed + 2, 0.003, 0.04, beta=1.1, stretch=1.3) * 0.004
    yd = (y[:, None] + wave + rag) % height_m
    idx = np.clip(np.searchsorted(tops, yd, side="right") - 1, 0, n - 1)
    t = (yd - tops[idx]) / lifts[idx]                    # 0 at a lift's top, 1 at its foot
    top_dist = lifts[idx] * t                            # metres below the lift's top
    foot = lifts[idx] * (1.0 - t)                        # metres above its foot

    # Each lift's colour drifts along the wall (the mix was never quite even), and its top edge
    # blends into the lift above over a few millimetres (sharp for some, diffuse for others).
    drift = fbm_rect(ny, nx, height_m, width_m, seed + 8, 0.05, 0.9, beta=1.4, stretch=3.0)
    clumps = fbm_rect(ny, nx, height_m, width_m, seed + 9, 0.02, 0.25, beta=1.5, stretch=1.2)
    img = lift_col[idx] * (1.0 + 0.07 * drift + 0.05 * clumps)[..., None]
    above = lift_col[(idx - 1) % n]
    blend = np.clip(0.5 - top_dist / (2 * soft[idx]), 0.0, 0.5)
    img = img * (1 - blend[..., None]) + above * blend[..., None]
    # Within a lift: denser and a little lighter at the top, coarser and darker at the foot.
    img *= (1.03 - 0.06 * t)[..., None]
    # The compaction line under each lift: a few mm, darker, broken.
    brk = fbm_rect(ny, nx, height_m, width_m, seed + 3, 0.01, 0.3, stretch=3.0)
    line = np.exp(-(foot / 0.003) ** 2) * np.clip(0.4 + 0.9 * brk, 0, 1)
    img *= (1.0 - 0.07 * line)[..., None]
    # Erosion lines of trass-lime (every half metre or so): 5-9 mm, pale warm grey, broken.
    lime_w = 0.0065 + 0.0025 * fbm_rect(ny, nx, height_m, width_m, seed + 4, 0.03, 0.6, stretch=3.0)
    lime_px = lime[idx] & (foot < lime_w)
    lime_col = srgb(0xBDB3A4) * (1.0 + 0.06 * brk)[..., None]
    img = np.where(lime_px[..., None], img * 0.45 + lime_col * 0.55, img)

    # The matrix: mottling stretched along the lifts, a cloud, sand grain; dark and pale grains.
    mottle = fbm_rect(ny, nx, height_m, width_m, seed + 5, 0.02, 0.5, beta=1.6, stretch=2.5)
    cloud = fbm_rect(ny, nx, height_m, width_m, seed + 6, 0.3, 2.8, beta=1.5, stretch=2.0)
    grain = fbm_rect(ny, nx, height_m, width_m, seed + 7, 0.002, 0.01, beta=0.5)
    img *= (1.0 + 0.06 * mottle + 0.04 * cloud + 0.1 * grain)[..., None]
    # Grit and pits: the rammed surface is granular, never smooth.
    specks = rng.random((ny, nx))
    pits = (specks > 0.986) | ((np.roll(specks, 1, axis=1) > 0.996) & (specks > 0.6))
    img = np.where(pits[..., None], img * (0.6 + 0.25 * rng.random((ny, nx, 1))), img)       # dark grit and pits
    img = np.where((specks < 0.006)[..., None], img * 1.12 + 0.05, img)                      # quartz sand

    # Gravel and pebbles, more towards each lift's foot (the coarse settles), and small voids.
    stones = [srgb(h) for h in (0x8B8781, 0x9C968C, 0xC4BDB1, 0x6E655C, 0x94705A, 0xAE977A, 0x5C554F, 0x7D7468)]

    def lower_half(cy, cx):
        return 0.3 + 0.7 * float(t[int(cy) % ny, int(cx) % nx])

    area = height_m * width_m
    stamp_pebbles(img, rng, int(3200 * area), (2.0, 7.0), mpp, stones, lower_half, alpha=0.7, mute_max=0.6)
    stamp_pebbles(img, rng, int(420 * area), (7.0, 20.0), mpp, stones, lower_half, alpha=0.85, mute_max=0.5)
    stamp_pebbles(img, rng, int(40 * area), (20.0, 45.0), mpp, stones, lower_half, alpha=0.9, mute_max=0.35)
    voids = [srgb(0x4A3F36)]
    stamp_pebbles(img, rng, int(260 * area), (1.5, 5.0), mpp, voids, None, flat=(1.0, 1.5), shade=-0.2, alpha=0.5)
    return np.clip(img, 0.0, 1.0)


def strata_pit(ny=2048, nx=4096, depth_m=7.0, round_m=2 * np.pi * 4.2, floor_m=8.4, seed=47):
    """
    The strata round the Square's pit (Élan level -1), seen through its glass floor: soil, clay,
    chalk with an ammonite, bedrock. The pit is round, R 4.2 m and 7 m deep; this image is laid out
    for it, not tiled:

    - the top half is the wall unrolled: across, once round (2π × 4.2 m, seamless); down, the 7 m
      from the glass to the bottom, top row at the glass: topsoil with roots, stony subsoil, mottled
      clay, white chalk with rows of flint and the ammonite, bedded grey bedrock with joints;
    - the bottom half is the pit's floor seen from above: fractured bedrock and a little gravel, an
      8.4 m square (the floor's diameter) at the left, drawn at the scale the UVs give it.

    UV0 on the wall: U = the arc length (m) from east (towards south), V = the depth (m); on the
    floor, U = plan x - (the centre's x - 4.2), V = 7 + (plan y - the centre's y + 4.2) × 7 / 8.4.
    The material's TexScale is 2π × 4.2 and its TexAspect 14 / (2π × 4.2), so the image spans one
    round and 14 m of V. The albedo itself (sRGB).
    """
    rng = np.random.default_rng(seed)
    half = ny // 2
    wall_h = half
    mpp = (depth_m / wall_h, round_m / nx)
    d = (np.arange(wall_h) + 0.5) * mpp[0]                     # depth, m
    u = (np.arange(nx) + 0.5) * mpp[1]                         # round the pit, m

    def noise(seed_, lam_min, lam_max, beta=1.7, stretch=1.0):
        return fbm_rect(wall_h, nx, depth_m, round_m, seed_, lam_min, lam_max, beta, stretch)

    # The horizons' boundaries: each undulates round the pit (long waves) and is ragged in detail.
    def boundary(mean, amp, rag, s):
        line = fbm_rect(1, nx, 1.0, round_m, s, 0.5, round_m, beta=1.8)[0] * amp
        line = line + fbm_rect(1, nx, 1.0, round_m, s + 1, 0.02, 0.4, beta=1.3)[0] * rag
        return mean + line

    b_top = boundary(0.38, 0.06, 0.02, seed + 1)      # topsoil / subsoil
    b_clay = boundary(2.2, 0.12, 0.03, seed + 3)      # subsoil / clay
    b_chalk = boundary(3.6, 0.07, 0.015, seed + 5)    # clay / chalk
    b_rock = boundary(4.8, 0.05, 0.01, seed + 7)      # chalk / bedrock
    D = d[:, None] + 0.0 * u[None, :]

    # Topsoil: dark humus, crumbly.
    crumb = noise(seed + 10, 0.005, 0.08, beta=1.2)
    soil_top = srgb(0x4B3A2C) * (1 + 0.12 * crumb)[..., None]
    # Subsoil: brown, lightening and yellowing with depth, iron mottles (stones below).
    t_sub = np.clip((D - b_top) / (b_clay - b_top), 0, 1)
    sub = srgb(0x7A5E45) * (1 - t_sub)[..., None] + srgb(0x9A7B57) * t_sub[..., None]
    sub *= (1 + 0.08 * noise(seed + 11, 0.01, 0.3, stretch=2.0) + 0.05 * crumb)[..., None]
    iron = np.clip(noise(seed + 12, 0.03, 0.4, beta=1.6) * 2.2 - 1.1, 0, 1)
    sub = sub * (1 - 0.35 * iron[..., None]) + srgb(0xA86A3A) * (0.35 * iron[..., None])
    # Clay: dense ochre mottled with blue-grey, thin sandy laminae.
    mott = np.clip(noise(seed + 13, 0.12, 2.0, beta=2.2, stretch=2.0) * 1.3, -1, 1)
    mott = np.sign(mott) * np.abs(mott) ** 0.7
    clay = srgb(0xAA7A4E) * (0.5 + 0.5 * mott)[..., None] + srgb(0x938D82) * (0.5 - 0.5 * mott)[..., None]
    lam = noise(seed + 14, 0.008, 0.12, beta=1.2, stretch=10.0)          # faint silty laminae
    clay *= (1 + 0.05 * lam + 0.04 * noise(seed + 15, 0.004, 0.05))[..., None]
    # Chalk: white-cream, faint bedding every 30-40 cm, soft grain.
    chalk = srgb(0xDDD5C3) * (1 + 0.035 * noise(seed + 16, 0.005, 0.5, beta=1.3) + 0.02 * np.sin(2 * np.pi * D / 0.34)
                              + 0.03 * noise(seed + 20, 0.2, 2.0, stretch=3.0))[..., None]
    stain = np.clip(noise(seed + 21, 0.1, 1.5, beta=2.0) * 2 - 0.8, 0, 1)
    chalk = chalk * (1 - 0.25 * stain[..., None]) + srgb(0xC9B894) * (0.25 * stain[..., None])
    # Bedrock: grey limestone in beds of 20-50 cm with dark partings and a few vertical joints.
    beds = [4.8]
    while beds[-1] < depth_m + 0.5:
        beds.append(beds[-1] + 0.25 + 0.4 * rng.random())
    beds = np.array(beds)
    bend = 0.025 * noise(seed + 17, 0.3, 4.0, stretch=4.0)
    bed_i = np.clip(np.searchsorted(beds, D + bend) - 1, 0, len(beds) - 1)
    bed_tone = 1 + 0.05 * rng.normal(size=len(beds))
    rock = srgb(0x8B857C) * bed_tone[bed_i][..., None]
    rock *= (1 + 0.1 * noise(seed + 18, 0.004, 0.25, beta=1.2) + 0.07 * noise(seed + 19, 0.2, 3.0, stretch=2.0)
             + 0.06 * noise(seed + 25, 0.006, 0.08, beta=1.0, stretch=8.0))[..., None]     # laminae within the beds
    grit = rng.random(D.shape)
    rock = np.where((grit > 0.992)[..., None], rock * 0.7, rock)
    rock = np.where((grit < 0.004)[..., None], rock * 1.2 + 0.03, rock)
    rust = np.clip(noise(seed + 22, 0.1, 1.5, beta=2.0) * 2 - 1.0, 0, 1)
    rock = rock * (1 - 0.3 * rust[..., None]) + srgb(0x8E6E52) * (0.3 * rust[..., None])
    dist_bed = np.full(D.shape, 1e9)
    for b in beds:
        dist_bed = np.minimum(dist_bed, np.abs(D + bend - b))
    parting_w = 0.004 + 0.004 * np.clip(noise(seed + 23, 0.05, 1.0, stretch=4.0), -1, 1)
    parting = np.exp(-(dist_bed / np.maximum(parting_w, 0.001)) ** 2) * np.clip(0.6 + 0.6 * noise(seed + 24, 0.05, 1.5, stretch=3.0), 0, 1)
    rock *= (1 - 0.28 * parting)[..., None]
    joints = np.zeros(D.shape)
    for _ in range(5):
        x0 = rng.random() * round_m
        b0 = int(rng.integers(0, len(beds) - 1))
        top, bot = beds[b0], beds[min(b0 + 1 + int(rng.integers(0, 3)), len(beds) - 1)]
        dx = ((u[None, :] - x0 - 0.05 * np.sin(D * 3.0 + x0)) + round_m / 2) % round_m - round_m / 2
        joints = np.maximum(joints, np.exp(-(dx / 0.004) ** 2) * ((D > top) & (D < bot)))
    rock *= (1 - 0.22 * joints)[..., None]

    # The horizons together, blended over a few centimetres at each boundary.
    def over(a, b, edge, width):
        w = np.clip((D - edge) / width + 0.5, 0, 1)[..., None]
        return a * (1 - w) + b * w

    img = over(soil_top, sub, b_top, 0.08)
    img = over(img, clay, b_clay, 0.1)
    img = over(img, chalk, b_chalk, 0.03)
    img = over(img, rock, b_rock, 0.015)

    # Roots in the topsoil (a few reaching down into the subsoil): thin wandering lines.
    for _ in range(90):
        x, y = rng.random() * nx, (0.02 + rng.random() * 0.5) / mpp[0]
        ang = np.pi / 2 + rng.normal(0, 0.5)
        length = int((0.05 + 0.4 * rng.random() ** 2) / mpp[0])
        col = srgb(0x8A7056) if rng.random() < 0.6 else srgb(0x2E231B)
        for _ in range(length):
            ang += rng.normal(0, 0.12)
            x, y = x + np.cos(ang) * 0.7, y + np.sin(ang) * 0.7
            yi, xi = int(y), int(x) % nx
            if 0 <= yi < wall_h:
                img[yi, xi] = img[yi, xi] * 0.4 + col * 0.6

    # Stones in the subsoil (a scatter elsewhere in the soil), flints in the chalk's bedding planes.
    def in_sub(cy, cx):
        dd, i = cy * mpp[0], int(cx) % nx
        if b_top[i] + 0.05 < dd < b_clay[i] - 0.05:
            return 1.0
        return 0.1 if dd < b_top[i] else 0.0

    stones = [srgb(h) for h in (0x8C867C, 0x9E968A, 0x6B635A, 0xB9AE9C, 0x8A6F55, 0x7C746A)]
    stamp_pebbles(img, rng, 5200, (6.0, 30.0), mpp, stones, in_sub, flat=(1.0, 1.5), alpha=0.9, mute_max=0.3)
    stamp_pebbles(img, rng, 700, (30.0, 90.0), mpp, stones, in_sub, flat=(1.0, 1.4), alpha=0.95, mute_max=0.2)
    for row in (3.82, 4.55):
        for _ in range(int(round_m / 0.9)):
            cx = rng.random() * round_m
            if abs(((cx - np.radians(20) * 4.2) + round_m / 2) % round_m - round_m / 2) < 0.45:
                continue                                  # keep clear of the ammonite
            _flint(img, rng, (row + 0.03 * rng.normal()) / mpp[0], cx / mpp[1], 0.05 + 0.13 * rng.random() ** 1.5, mpp)

    # The ammonite, in the chalk: 45 cm across, on the east wall (20° south of east), which faces
    # the car's door on the west.
    _ammonite(img, 4.15 / mpp[0], (np.radians(20) * 4.2) / mpp[1], 0.45, mpp)

    # The pit's floor (the bottom half): fractured bedrock and gravel, seen from above.
    fh = ny - half
    fmpp = (floor_m / fh, round_m / nx)
    fs = fh                                                   # the floor's square, fs x fs px over 8.4 m
    warp = (fbm(fs, 2.0, seed + 34, lo=2, hi=20) * 25, fbm(fs, 2.0, seed + 35, lo=2, hi=20) * 25)
    edge = cell_edges(fs, 45, seed + 36, warp)              # blocks about 1.2 m across
    cracks = np.exp(-(edge / 1.6) ** 2) * np.clip(0.5 + 0.8 * fbm(fs, 1.6, seed + 37, lo=2, hi=40), 0, 1)
    tone = 1 + 0.08 * fbm(fs, 1.3, seed + 30, lo=6, hi=300) + 0.06 * fbm(fs, 1.8, seed + 31, lo=1.5, hi=12)
    sq = srgb(0x86807A) * tone[..., None] * (1 - 0.5 * cracks)[..., None]
    # Widen to the image's scale across (the floor's U is in metres like the wall's).
    cols = int(round(floor_m / fmpp[1]))
    xi = (np.arange(nx) % cols) * fs // cols
    fimg = np.ascontiguousarray(sq[:, xi])
    stamp_pebbles(fimg, rng, 3000, (5.0, 40.0), fmpp, stones + [srgb(0xD8D0C0)], None, flat=(1.0, 1.3), alpha=0.9, mute_max=0.3)
    return np.clip(np.concatenate([img, fimg], axis=0), 0.0, 1.0)


def _flint(img, rng, cy, cx, size, mpp):
    """A flint nodule: an irregular dark grey-black lump in a white cortex (cy, cx in pixels)."""
    ny, nx, _ = img.shape
    ry, rx = size * (0.45 + 0.2 * rng.random()) / mpp[0], size / mpp[1]
    r = int(max(ry, rx)) + 3
    ys, xs = np.arange(int(cy) - r, int(cy) + r + 1), np.arange(int(cx) - r, int(cx) + r + 1)
    yy, xx = np.meshgrid(ys - cy, xs - cx, indexing="ij")
    phi = np.arctan2(yy / ry, xx / rx)
    p = rng.random(3) * 6.283
    q = np.sqrt((yy / ry) ** 2 + (xx / rx) ** 2) * (1 + 0.22 * np.sin(2 * phi + p[0]) + 0.12 * np.sin(3 * phi + p[1])
                                                    + 0.08 * np.sin(5 * phi + p[2]))
    core = np.clip((1 - q) * 6, 0, 1)
    cortex = np.clip((1.1 - q) * 6, 0, 1) - core
    yi, xi = np.clip(ys, 0, ny - 1), xs % nx
    patch = img[np.ix_(yi, xi)]
    patch = patch * (1 - 0.7 * cortex[..., None]) + srgb(0xEAE4D8) * (0.7 * cortex[..., None])
    dark = srgb(0x3A3836) * (1.0 + 0.35 * rng.random()) * (1 + 0.2 * np.clip(-yy / ry, -1, 1))[..., None]
    patch = patch * (1 - core[..., None]) + dark * core[..., None]
    img[np.ix_(yi, xi)] = patch


def _ammonite(img, cy, cx, diameter, mpp):
    """An ammonite in relief (cy, cx in pixels): an evolute logarithmic spiral of ribbed whorls, lit from above."""
    ny, nx, _ = img.shape
    R = diameter / 2
    r = int(R / min(mpp)) + 4
    ys, xs = np.arange(int(cy) - r, int(cy) + r + 1), np.arange(int(cx) - r, int(cx) + r + 1)
    yy, xx = np.meshgrid((ys - cy) * mpp[0], (xs - cx) * mpp[1], indexing="ij")    # metres
    rad = np.hypot(yy, xx)
    phi = np.arctan2(yy, xx)
    k = 1.75                                    # each whorl this much wider than the last
    g = np.log(k) / (2 * np.pi)
    whorls = 4.3
    a = R / np.exp(g * 2 * np.pi * whorls)
    turn = (np.log(np.maximum(rad, 1e-6) / a) / g - phi) / (2 * np.pi)
    n = np.floor(turn)
    frac = turn - n                             # 0 at the inner suture of this whorl, 1 at its outer
    theta = phi + 2 * np.pi * n                 # angle along the shell from the centre
    inside = theta < 2 * np.pi * whorls         # the last whorl ends at the aperture
    body = (rad < R * 1.001) & (rad > a * 3) & inside
    # A rounded whorl, its outer flank lower (it overlaps the next), fine ribs that curve forward.
    prof = np.sin(np.pi * np.clip(frac, 0, 1)) ** 0.7
    ribs = np.abs(np.sin(theta * 13 + 1.8 * frac))
    ribs = 1 - 0.22 * (1 - ribs) ** 3
    h = np.where(body, prof * ribs * 0.02 * np.exp(g * theta * 0.5), 0.0)     # metres of relief
    gy = np.gradient(h, mpp[0], axis=0)
    gx = np.gradient(h, mpp[1], axis=1)
    lit = np.clip(1.0 - 1.6 * gy - 0.5 * gx, 0.55, 1.45)                        # light from above, a little left
    col = srgb(0xC5AC84) * (0.85 + 0.15 * prof)[..., None] * lit[..., None]
    col *= (1 - 0.3 * np.exp(-(np.minimum(frac, 1 - frac) / 0.06) ** 2))[..., None]   # the sutures
    fade = np.clip((R - rad) / (0.02 * R), 0, 1) * body
    yi, xi = np.clip(ys, 0, ny - 1), xs % nx
    patch = img[np.ix_(yi, xi)]
    patch = patch * (1 - fade[..., None]) + col * fade[..., None]
    img[np.ix_(yi, xi)] = patch


# --------------------------------------------------------------------------- the classical hall's coloured marbles

# Full-colour images (the materials' BaseColor is white): the stones of the Pantheon's floor and the Braccio Nuovo's
# columns, for AClassicalHallStructure's opus sectile, shafts, panels and dado. Each tiles; 2048 px.

def _mix(a, b, t):
    return a * (1.0 - t[..., None]) + b * t[..., None]


def porphyry(n=2048):
    """
    Red porphyry (1 m): a deep purple-red groundmass, softly clouded, crowded with small pale pink feldspar crystals,
    1-6 mm, sharp-edged (a thresholded fine field, so they come in clusters and angular bits), and a few dark specks.
    """
    ground = srgb(0x5E1D22)
    cloud = fbm(n, 1.8, 101, lo=2, hi=60) * 0.07 + fbm(n, 1.4, 102, lo=60, hi=400) * 0.03
    img = ground[None, None, :] * (1.0 + cloud[..., None])
    s = n / 2048.0
    crystals = fbm(n, 0.4, 103, lo=110 * s, hi=700 * s)   # features of 3-18 px at 0.5 mm/px: crystals of 1-6 mm
    sizes = fbm(n, 1.5, 104, lo=4, hi=40)                 # denser here and there
    mask = np.clip((crystals - (0.22 - 0.07 * sizes)) * 12.0, 0.0, 1.0)
    pale = srgb(0xD6ABA4) * (1.0 + fbm(n, 1.0, 105, lo=200, hi=800)[..., None] * 0.08)
    img = _mix(img, pale, mask * 0.85)
    dark = np.clip((fbm(n, 0.5, 106, lo=150 * s, hi=700 * s) - 0.45) * 10.0, 0.0, 1.0)
    img = _mix(img, srgb(0x2A0C10), dark * 0.7)
    return np.clip(img, 0.0, 1.0)


def verde_antico(n=2048):
    """
    Verde antico (1.5 m): a breccia of dark green serpentine clasts, some lighter and bluish, in a network of white
    calcite veins; the clasts' edges broken by a finer crackle.
    """
    w = (fbm(n, 2.0, 111, lo=2, hi=30) * 60, fbm(n, 2.0, 112, lo=2, hi=30) * 60)
    edges = cell_edges(n, 90, 113, w)
    # The clasts' shades drift from dark to a lighter, bluer green.
    shade = fbm(n, 2.2, 115, lo=2, hi=18)
    dark, light = srgb(0x1C2A22), srgb(0x3E5E4C)
    img = _mix(np.broadcast_to(dark, (n, n, 3)).copy(), np.broadcast_to(light, (n, n, 3)), np.clip(shade * 0.7 + 0.4, 0.0, 1.0))
    img *= (1.0 + fbm(n, 1.2, 116, lo=40, hi=500)[..., None] * 0.10)
    veins_main = np.exp(-(edges / 2.2) ** 2)
    fine = np.exp(-(cell_edges(n, 400, 117, (fbm(n, 1.8, 118, lo=4, hi=60) * 25, fbm(n, 1.8, 119, lo=4, hi=60) * 25)) / 1.0) ** 2)
    patches = np.clip(fbm(n, 1.8, 120, lo=2, hi=12) * 1.5 + 0.2, 0.0, 1.0)
    white = srgb(0xDDE0D6)
    img = _mix(img, white, np.clip(veins_main * 0.95 + fine * patches * 0.5, 0.0, 1.0))
    return np.clip(img, 0.0, 1.0)


def pavonazzetto(n=2048, m=2.0):
    """
    Pavonazzetto (2 m), after the Colosseum's fluted shaft, the Altemps spiral column and the Calp slab: ivory clasts in
    wandering grey-violet veins that run from hairlines to broad pools, granular and darkest in their cores, their
    edges crinkled at the grain's scale (not smooth cell edges, which read as cracks); a finer vein network in patches.
    """
    F = lambda seed, lo, hi, beta=1.9, st=1.6: fbm_rect(n, n, m, m, seed, lo, hi, beta=beta, stretch=st)
    yy, xx = np.mgrid[0:n, 0:n] / n
    def warped(field, wx, wy):
        iy = ((yy + wy) % 1.0 * n).astype(int); ix = ((xx + wx) % 1.0 * n).astype(int)
        return field[iy, ix]
    # Two warps: a broad one that makes the veins wander, a fine one (5–40 mm) that crinkles their edges.
    wx = F(201, 0.08, 1.0) * 0.16 + F(241, 0.005, 0.04, beta=1.3, st=1.0) * 0.006
    wy = F(202, 0.08, 1.0) * 0.16 + F(242, 0.005, 0.04, beta=1.3, st=1.0) * 0.006
    ridge1 = 1.0 - np.abs(warped(F(203, 0.04, 0.9, beta=2.1), wx, wy))
    ridge2 = 1.0 - np.abs(warped(F(243, 0.015, 0.3, beta=2.0), wx * 1.3, wy * 1.3))
    width1 = 0.04 + 0.15 * np.clip(F(204, 0.1, 1.2) * 1.0 + 0.45, 0, 1) ** 1.6
    width2 = 0.03 + 0.08 * np.clip(F(244, 0.1, 1.0) + 0.3, 0, 1)
    patch2 = np.clip(F(245, 0.15, 1.2) * 1.8, 0, 1)
    b1 = np.clip((ridge1 - (1.0 - width1)) / (width1 * 0.22), 0.0, 1.0)
    b2 = np.clip((ridge2 - (1.0 - width2)) / (width2 * 0.3), 0.0, 1.0) * patch2
    band = np.maximum(b1 * np.clip(0.9 + 0.4 * F(206, 0.15, 1.5), 0.0, 1.0), b2 * 0.85) * 0.82
    grain = F(209, 0.0015, 0.015, beta=0.9, st=1.0)
    # The clasts: ivory to cream, faintly clouded, a few flushed lilac.
    img = np.broadcast_to(srgb(0xEFE9DE), (n, n, 3)).copy()
    img = _mix(img, srgb(0xE4D8C3), np.clip(F(207, 0.05, 0.8) * 0.9 + 0.3, 0, 1) * 0.55)
    img = _mix(img, srgb(0xDACDD0), np.clip(F(229, 0.1, 0.6) * 1.5 - 0.4, 0, 1) * 0.45)
    img *= 1.0 + grain[..., None] * 0.02
    # A narrow lilac-grey stain along the veins.
    halo = np.clip((ridge1 - (1.0 - width1 * 1.6)) / (width1 * 0.6), 0.0, 1.0) * (1 - b1)
    img = _mix(img, srgb(0xC2B3B8), halo * 0.22)
    # The veins: granular violet-grey, darkest in the broad cores, paler grains among them.
    core = np.clip((ridge1 - (1.0 - width1 * 0.45)) / (width1 * 0.3), 0, 1)
    vein = _mix(np.broadcast_to(srgb(0xA08F99), (n, n, 3)).copy(), srgb(0x6E5B69), np.clip(0.3 + core * 0.5 + grain * 0.8, 0, 1))
    vein = _mix(vein, srgb(0xAFA2A9), np.clip(-grain * 2.2 - 0.4, 0, 1) * 0.8)
    img = _mix(img, vein, band)
    specks = np.clip((F(210, 0.0015, 0.008, beta=0.6, st=1.0) - 0.5) * 5, 0, 1) * core
    img = _mix(img, srgb(0xF2EEE8), specks * 0.8)
    hair = veins(F(211, 0.05, 0.7, beta=2.0), [-0.35, 0.3], 0.9) * np.clip(F(212, 0.1, 1.0) + 0.05, 0, 1)
    img = _mix(img, srgb(0x857380), hair * 0.35 * (1 - band))
    return np.clip(img, 0, 1)


def giallo_antico(n=2048):
    """
    Giallo antico (1.5 m): warm yellow Numidian marble, clouded from pale straw to deep gold, with a web of thin ochre-red
    veins and a few broader rusty ones.
    """
    straw, gold = srgb(0xE3C382), srgb(0xC3923F)
    t = np.clip(fbm(n, 1.9, 131, lo=2, hi=40) * 0.8 + 0.45, 0.0, 1.0)
    img = _mix(np.broadcast_to(straw, (n, n, 3)).copy(), np.broadcast_to(gold, (n, n, 3)), t)
    img *= (1.0 + fbm(n, 1.2, 132, lo=40, hi=600)[..., None] * 0.05)
    # A web of thin veins round warped cells, fading in and out; a few broad rusty ones.
    w = (fbm(n, 2.0, 133, lo=2, hi=28) * 90, fbm(n, 2.0, 136, lo=2, hi=28) * 90)
    web = np.exp(-(cell_edges(n, 60, 137, w) / 1.4) ** 2)
    fade = np.clip(fbm(n, 1.6, 134, lo=2, hi=24) * 1.2 + 0.25, 0.0, 1.0)
    img = _mix(img, srgb(0x9B5A2E), web * fade * 0.6)
    halo = np.exp(-(cell_edges(n, 60, 137, w) / 9.0) ** 2) * fade * 0.18
    img = _mix(img, srgb(0xB27437), halo)
    broad = np.exp(-(cell_edges(n, 9, 135, (w[0] * 2.0, w[1] * 2.0)) / 5.0) ** 2) * np.clip(fbm(n, 1.5, 138, lo=2, hi=10) + 0.2, 0.0, 1.0)
    img = _mix(img, srgb(0x8C4526), broad * 0.3)
    return np.clip(img, 0.0, 1.0)


def rosso_antico(n=2048):
    """Rosso antico (1 m): a dense dark red, faintly clouded, with fine black veinlets and a few pale hairlines."""
    red = srgb(0x6D2821)
    img = red[None, None, :] * (1.0 + fbm(n, 1.8, 141, lo=2, hi=50)[..., None] * 0.08 + fbm(n, 1.0, 142, lo=80, hi=700)[..., None] * 0.03)
    w = (fbm(n, 2.0, 143, lo=2, hi=40) * 50, fbm(n, 2.0, 146, lo=2, hi=40) * 50)
    dark = np.exp(-(cell_edges(n, 70, 147, w) / 1.0) ** 2) * np.clip(fbm(n, 1.6, 148, lo=2, hi=24) + 0.35, 0.0, 1.0)
    img = _mix(img, srgb(0x250A08), dark * 0.55)
    pale = np.exp(-(cell_edges(n, 12, 144, (w[0] * 2, w[1] * 2)) / 0.8) ** 2) * np.clip(fbm(n, 1.5, 145, lo=2, hi=16), 0.0, 1.0)
    img = _mix(img, srgb(0xB88578), pale * 0.45)
    return np.clip(img, 0.0, 1.0)


def peachbloom(n=1024):
    """
    Kangxi peachbloom glaze (the Met's 14.40.377, assets/reference/chinese-ref-peachbloom.jpg): a crimson-pink clouded
    lighter and deeper, fine dark red flecks, and a few faint grey-green patches where the copper greened.
    """
    img = np.broadcast_to(srgb(0xB05A56), (n, n, 3)).copy()
    c1, c2 = fbm(n, 2.0, 501, lo=1, hi=12), fbm(n, 1.6, 502, lo=2, hi=40)
    img = _mix(img, srgb(0xC7766E), np.clip(c1 * 1.2 + 0.1, 0, 1) * 0.55)
    img = _mix(img, srgb(0x8E3E3F), np.clip(-c1 * 1.3 - 0.1, 0, 1) * 0.5)
    img *= (1 + c2[..., None] * 0.04)
    rng = np.random.default_rng(503)
    specks = np.zeros((n, n))
    yy, xx = np.ogrid[-3:4, -3:4]
    for _ in range(900):
        y, x = rng.integers(0, n, 2)
        r = rng.uniform(0.6, 1.8)
        ys, xs = (np.arange(y - 3, y + 4) % n)[:, None], (np.arange(x - 3, x + 4) % n)[None, :]
        specks[ys, xs] = np.maximum(specks[ys, xs], np.exp(-(xx ** 2 + yy ** 2) / (2 * r * r)) * rng.uniform(0.3, 0.9))
    img = _mix(img, srgb(0x6E2A2E), specks * 0.7)
    green = np.clip((fbm(n, 1.0, 504, lo=20, hi=200) - 0.62) * 6, 0, 1) * np.clip(fbm(n, 1.5, 505, lo=2, hi=10) + 0.1, 0, 1)
    return np.clip(_mix(img, srgb(0x8C8C74), green * 0.35), 0, 1)


def save(name, img):
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, name + ".png")
    Image.fromarray((img * 255.0 + 0.5).astype(np.uint8), "RGB").save(path, optimize=True)
    print(f"{path}: mean {img.mean():.3f}, min {img.min():.3f}")


if __name__ == "__main__":
    save("T_marble_crackle", marble_crackle())
    save("T_concrete_mottle", concrete_mottle())
    save("T_rammed_earth_lifts", rammed_earth())
    save("T_strata_pit", strata_pit())
    # The classical hall's coloured marbles (AClassicalHallStructure).
    save("T_porphyry", porphyry())
    save("T_verde_antico", verde_antico())
    save("T_pavonazzetto", pavonazzetto())
    save("T_giallo_antico", giallo_antico())
    save("T_rosso_antico", rosso_antico())
    save("T_peachbloom", peachbloom())
