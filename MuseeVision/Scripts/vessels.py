"""Derives Chenghuai's painted and monochrome vessels from their photographs (order K9).

    python MuseeVision/Scripts/vessels.py [--write] [--sheets <folder>]

For each vessel the collection holds photographs of (assets/ceramics/<work>/, credited in its CREDITS.md), this finds:

- the camera of each photograph: a pinhole whose distance, height, pitch and focal length make the rim's back edge
  and the foot's front edge (two circles of the vessel, seen as ellipses) and the rim's and foot's widths fall where
  the photograph shows them, with the vessel's height the museum's. A photograph that shows perspective (the foot's
  ellipse rounder than the rim's) is corrected by it; one taken from far off fits a far camera;
- the vessel's section: the silhouette's edge against its plain ground, each height's ring turned until its image
  meets that edge through the camera, scaled so the vessel's height is the museum's; its widest diameter is checked
  against the museum's where one is published;
- the angle each further view was taken from, by registering its picture of the decoration, unwrapped round the
  vessel's axis, against the first view's where the two overlap, its camera the one its own silhouette fixes; a view is
  kept only where its camera fits its silhouette and its painting lies on the views it overlaps in every band of height
  (else it is listed as unused, with the numbers), and a view that overlaps none of the first view's is unverified;
- for a monochrome glaze, its colour in CIELAB, from the lit glaze of its photograph relative to the photograph's own
  white.

The underside and the inside, which no silhouette shows, are a wall's thickness in from the outside: a thin-potted
vessel's its lip's, read on its photograph; any other's Data/Art/Ware.json's rule. Every section is checked to cross
itself nowhere and to keep its wall.
Everything is deterministic: the same photographs give the same numbers. With --write the results replace the
vessels' entries in Data/Art/Vessels.json; without it they are printed. With --sheets, a check picture of each view,
its fitted rings drawn over it, is written to that folder.

Needs Python 3 with numpy, scipy and OpenCV (cv2). The photographs are read from the collection (MUSEE_COLLECTION, or
the owner's assets folder), never written.
"""
import argparse
import json
import math
import os
import sys

import cv2
import numpy as np
from scipy.optimize import least_squares

HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT = os.path.dirname(HERE)
COLLECTION = os.environ.get('MUSEE_COLLECTION', r'C:\Users\anan9\Desktop\Projects\Musee Vision\assets')
RECORD = os.path.join(PROJECT, 'Data', 'Art', 'Vessels.json')
WARE = os.path.join(PROJECT, 'Data', 'Art', 'Ware.json')

# ---------------------------------------------------------------------------------------------------------------------
# The vessels. Sizes are the museums' records; each view names its photograph (under assets/), how its silhouette is
# read, and whether it is the view the section is derived from.
# ---------------------------------------------------------------------------------------------------------------------
MET = 'The Metropolitan Museum of Art'
# The meiping's side A carries its camera's EXIF: a Canon EOS 600D at 48 mm, 5728 px per inch on its sensor
# (48 mm x 5728 / 25.4 = 10825 px, the file being the sensor's full 3456 x 5184).
MEIPING_A_FOCAL = 48.0 * 5728.176795580111 / 25.4
NPM = 'National Palace Museum, Taipei'
VESSELS = [
    dict(work='chinese-xuande-jar', painted=True,
         museum=dict(heightMeters=0.483, diameterMeters=0.483,
                     source=MET + ', 37.191.1, Jar with dragon: H. 48.3 cm, Diam. 48.3 cm (the order K9 table)'),
         # the neck's step and the rolled lip keep their turns
         views=[dict(image='ceramics/chinese-xuande-jar/DP-32251-001.jpg', profile=True, fine=[(0.88, 1.0)]),
                dict(image='ceramics/chinese-xuande-jar/MET_DT5083.jpg')],
         bands=[dict(name="the foot's lotus panels", rows=(0.0, 0.16), shortest=8.0),
                dict(name="the neck's cloud scroll", rows=(0.89, 0.98), shortest=30.0)]),
    dict(work='chinese-wucai-jar', painted=True,
         museum=dict(heightMeters=0.232, source=MET + ', 17.127.2, Jar with carp in lotus pond: H. 23.2 cm (no diameter published)'),
         views=[dict(image='ceramics/chinese-wucai-jar/DP342697.jpg', profile=True, measure=dict(side='both'), rows=(0.12, 1.0)),
                dict(image='ceramics/chinese-wucai-jar/DP342698.jpg', same_camera=True, measure=dict(side='both')),
                dict(image='ceramics/chinese-wucai-jar/DP342700.jpg', same_camera=True, measure=dict(side='both'))],
         bands=[dict(name="the foot's cloud collar", rows=(0.0, 0.16), shortest=15.0),
                dict(name="the shoulder's lappets", rows=(0.82, 0.92), shortest=10.0),
                dict(name="the neck's scroll", rows=(0.93, 1.0), shortest=6.0)]),
    dict(work='chinese-falangcai-bowl', painted=True,
         museum=dict(heightMeters=0.054, diameterMeters=0.114, footMeters=0.044,
                     source=MET + ', 65.86.12, Bowl with flowers: H. 5.4 cm, Diam. 11.4 cm, foot 4.4 cm (the order K9 table)'),
         views=[dict(image='ceramics/chinese-falangcai-bowl/MET_DT223854.jpg', profile=True,
                     measure=dict(lip_rows=0.35, grabcut=False, thresh=8, foot_span=(-0.5, 0.5), window_bottom=20),
                     fixed=dict(lip='diameter', foot='foot'), rows=(0.28, 0.96))],
         # thin-potted: the lit lip at the rim's right-hand turn, between the ground and the shaded inside
         lip=dict(rows=(930, 935), columns=(3318, 3338)),
         unused=[dict(image='assets/ceramics/chinese-falangcai-bowl/MET_201714.jpg',
                      why=('a black-and-white photograph, which carries no colour. It shows what no colour view does: the '
                           "bowl's inside painted with a peony and flowering branches, and a chrysanthemum on the far side "
                           'of the outside. With no colour photograph of them, the inside is shown in the glaze and the far '
                           'side as the glaze or its bands allow: the real bowl is painted there'))],
         bands=[]),
    dict(work='chinese-chicken-cup', painted=True,
         museum=dict(heightMeters=0.037, diameterMeters=0.082, footMeters=0.041,
                     source=NPM + ', 故瓷005190, chicken cup in doucai enamels: H. 3.7 cm, mouth 8.2 cm, foot 4.1 cm (the order K9 table)'),
         views=[dict(image='ceramics/chinese-chicken-cup/Porcelain_chicken_cup_in_doucai_painted_enamels.tif', profile=True,
                     # the lower wall's right-hand edge, pale enamel against the grey ground and the cup's soft cast
                     # shadow, traced by its colour down to row 1700, 3 mm above the foot
                     measure=dict(lip_rows=0.35, side='right', window_bottom=40, window_side=25, foot_span=(-0.6, 0.6),
                                  edge=dict(chroma=6.0, rows=(1400, 1700))),
                     fixed=dict(lip='diameter', foot='foot'), rows=(0.09, 1.0),
                     # the cup's foot sits in its own soft shadow: its bottom and the rim's top on the axis, by eye
                     axis_rows=(1036, 1803))],
         # eggshell-thin: the lit lip at the rim's right-hand turn, between the ground and the shaded inside
         lip=dict(rows=(1138, 1144), columns=(2204, 2215)),
         unused=[dict(image='assets/ceramics/chinese-chicken-cup/鬥彩雞缸杯.png', why='the same photograph as the TIFF, reduced')],
         bands=[dict(name="the rim's double line", rows=(0.76, 0.92)), dict(name="the foot's double line", rows=(0.0, 0.10))]),
    dict(work='chinese-meiping', painted=True,
         museum=dict(heightMeters=0.356, diameterMeters=0.22,
                     source='Victoria and Albert Museum, C.8-1952, meiping with scenes from The West Chamber: H. 35.6 cm, Diam. 22 cm (the order K9 table)'),
         views=[dict(image='ceramics/chinese-meiping/Meiping_Vase_with_Narrative_Scenes,_Jingdezhen,_Yuan,_1320-1350_(9847628326).jpg',
                     profile=True, fixed=dict(focal=MEIPING_A_FOCAL), fine=[(0.93, 1.0)]),
                ],
         # Side B (WLA_vanda_Vase_6.jpg) is read but not composed. Close, soft and lit from its case, its silhouette is
         # lost at the mouth in a highlight's bloom and at the foot in the shelf's reflection: fitted with its EXIF focal
         # length (30 mm on a Canon EOS 40D: 2022 px at the file's size) and its rim and foot read by eye (rows 108 and
         # 1415), its camera still misses the known section's sides by 24 px rms, and registered round to the right
         # (the willow at A's right edge and B's left) its painting lies on A's at a correlation of 0.34, under the 0.45 a
         # genuine overlap reaches. Composed, it would double the figures where the two sides meet.
         unused=[dict(image='assets/ceramics/chinese-meiping/WLA_vanda_Vase_6.jpg',
                      why=('its camera could not be fixed: fitted with its EXIF focal length, its silhouette misses the section '
                           'by 24 px rms and its painting lies on side A at a correlation of 0.34, under the 0.45 of a genuine '
                           'overlap; composed, it would double the figures where the sides meet'))],
         bands=[dict(name="the base's lotus panels", rows=(0.0, 0.28), shortest=30.0), dict(name="the lower diaper band", rows=(0.28, 0.35)),
                dict(name="the shoulder's peony scroll", rows=(0.78, 0.92), shortest=15.0)]),
    dict(work='chinese-peachbloom', painted=False,
         museum=dict(heightMeters=0.197, source=MET + ', 14.40.377, Vase with peach-bloom glaze: H. 19.7 cm (no diameter published)'),
         # the two fillets at the neck's foot, the neck and the everted lip, crisp against the dark ground
         views=[dict(image='ceramics/chinese-peachbloom/DP246426.jpg', profile=True, fine=[(0.70, 1.0)])],
         # thin-potted: the white lip at the rim's left-hand turn, between the ground and the dark inside
         lip=dict(rows=(713, 716), columns=(1280, 1299)),
         bands=[]),
]

# The view registration and the unwrapped pictures: the grid round the axis and up the outside.
AZIMUTH_STEPS = 720
HEIGHT_STEPS = 240
# A view contributes where it sees the surface at least this squarely (the cosine of the angle between the surface's
# normal and the line of sight): beyond 60 degrees a photograph's pixel covers twice the surface it does face on.
SQUARENESS_CUT = 0.5


# ---------------------------------------------------------------------------------------------------------------------
# Reading the photographs
# ---------------------------------------------------------------------------------------------------------------------
def load(rel):
    data = np.fromfile(os.path.join(COLLECTION, rel), dtype=np.uint8)
    im = cv2.imdecode(data, cv2.IMREAD_COLOR)
    if im is None:
        raise SystemExit(f'{rel} cannot be read as an image')
    return im


def background(small, band=0.08):
    """The plain ground: per row, a straight line between the medians of the image's left and right margins."""
    h, w = small.shape[:2]
    lab = cv2.cvtColor(small, cv2.COLOR_BGR2LAB).astype(np.float32)
    m = max(2, int(w * band))
    left = np.median(lab[:, :m], axis=1)
    right = np.median(lab[:, -m:], axis=1)
    t = (np.arange(w) + 0.5) / w
    return lab, left[:, None, :] * (1 - t)[None, :, None] + right[:, None, :] * t[None, :, None]


def mask_of(im, work_px, thresh, grabcut, shadow):
    """The vessel against its ground: pixels far from the ground's colour, grown and cleaned by GrabCut. Where a view
    names a shadow (a chroma and the share of the vessel's height below which it lies), a cast shadow is ground: the
    ground darkened, its hue unchanged."""
    h, w = im.shape[:2]
    s = work_px / max(h, w)
    small = cv2.resize(im, (int(round(w * s)), int(round(h * s))), interpolation=cv2.INTER_AREA)
    lab, bg = background(small)
    rough = (np.sqrt(((lab - bg) ** 2).sum(axis=2)) > thresh).astype(np.uint8)
    cast = np.zeros(rough.shape, bool)
    if shadow:
        chroma, below = shadow
        ys = np.where(rough.any(axis=1))[0]
        start = int(ys[0] + below * (ys[-1] - ys[0]))
        cast[start:] = ((np.sqrt(((lab[..., 1:] - bg[..., 1:]) ** 2).sum(axis=2)) < chroma) & (lab[..., 0] < bg[..., 0] - 3))[start:]
        rough[cast] = 0
    rough = cv2.morphologyEx(rough, cv2.MORPH_OPEN, np.ones((5, 5), np.uint8))
    n, labl, stats, _ = cv2.connectedComponentsWithStats(rough)
    k = 1 + int(np.argmax(stats[1:, cv2.CC_STAT_AREA]))
    x, y, ww, hh = stats[k, :4]
    if grabcut:
        gc = np.full(rough.shape, cv2.GC_BGD, np.uint8)
        gc[max(0, y - 12):y + hh + 12, max(0, x - 12):x + ww + 12] = cv2.GC_PR_BGD
        gc[labl == k] = cv2.GC_PR_FGD
        gc[cv2.erode((labl == k).astype(np.uint8), np.ones((15, 15), np.uint8)) > 0] = cv2.GC_FGD
        gc[cast] = cv2.GC_BGD
        cv2.setRNGSeed(0)
        cv2.grabCut(small, gc, None, np.zeros((1, 65)), np.zeros((1, 65)), 5, cv2.GC_INIT_WITH_MASK)
        fg = np.where((gc == cv2.GC_FGD) | (gc == cv2.GC_PR_FGD), 255, 0).astype(np.uint8)
    else:
        fg = cv2.morphologyEx(rough * 255, cv2.MORPH_CLOSE, np.ones((9, 9), np.uint8))
    n, labl, stats, _ = cv2.connectedComponentsWithStats(fg)
    k = 1 + int(np.argmax(stats[1:, cv2.CC_STAT_AREA]))
    fg = np.where(labl == k, 255, 0).astype(np.uint8)
    flood = fg.copy()
    cv2.floodFill(flood, np.zeros((fg.shape[0] + 2, fg.shape[1] + 2), np.uint8), (0, 0), 255)
    return small, fg | cv2.bitwise_not(flood), s


def steps(im, sigma=1.2):
    """The photograph's horizontal and vertical colour steps (Sobel magnitudes in CIELAB)."""
    lab = cv2.GaussianBlur(cv2.cvtColor(im, cv2.COLOR_BGR2LAB).astype(np.float32), (0, 0), sigma)
    gx = sum(cv2.Sobel(lab[..., c], cv2.CV_32F, 1, 0, ksize=3) ** 2 for c in range(3))
    gy = sum(cv2.Sobel(lab[..., c], cv2.CV_32F, 0, 1, ksize=3) ** 2 for c in range(3))
    return np.sqrt(gx), np.sqrt(gy)


# A side's edge is the outermost step within the window at least this share of the strongest there: a painted line just
# inside the silhouette steps harder than the pale glaze against the ground, but lies inside it.
OUTERMOST_SHARE = 0.5


def snap(g, lines, guesses, window, along_rows, outward=0):
    """Moves each rough edge to the strongest step within the window (to a fraction of a pixel); where `outward` is -1 or
    +1 (a left or a right side), to the outermost step that way at least OUTERMOST_SHARE of the strongest."""
    out = np.full(len(lines), np.nan)
    limit = g.shape[1] if along_rows else g.shape[0]
    for i, (line, guess) in enumerate(zip(lines, guesses)):
        if np.isnan(guess):
            continue
        lo, hi = max(1, int(guess - window)), min(limit - 1, int(guess + window))
        seg = g[int(line), lo:hi] if along_rows else g[lo:hi, int(line)]
        if len(seg) < 3:
            continue
        k = int(np.argmax(seg))
        if outward:
            strong = seg >= OUTERMOST_SHARE * seg[k]
            peaks = np.where(strong[1:-1] & (seg[1:-1] >= seg[:-2]) & (seg[1:-1] >= seg[2:]))[0] + 1
            if len(peaks):
                k = int(peaks[0] if outward < 0 else peaks[-1])
        if 0 < k < len(seg) - 1:
            a, b, c = seg[k - 1], seg[k], seg[k + 1]
            den = a - 2 * b + c
            k = k + (0.5 * (a - c) / den if den != 0 else 0.0)
        out[i] = lo + k
    return out


def chroma_edge(im, rows, side, chroma, run=6, ground=300):
    """Where the vessel meets its ground along each of these rows, from the ground inward: the first run of pixels whose
    colour stands off the ground's by more than `chroma` in a* and b* (CIELAB). A cast shadow darkens the ground but
    leaves its hue, so the edge is found through it where a mask of lightness and colour takes the shadow for the
    vessel."""
    lab = cv2.cvtColor(cv2.GaussianBlur(im, (0, 0), 1.5), cv2.COLOR_BGR2LAB).astype(np.float32)
    out = np.full(len(rows), np.nan)
    for i, row in enumerate(rows):
        line = lab[int(row), :, 1:] if side == 'right' else lab[int(row), ::-1, 1:]
        off = np.sqrt(((line - np.median(line[-ground:], axis=0)) ** 2).sum(axis=1)) > chroma
        runs = np.where(np.convolve(off.astype(int), np.ones(run, int), 'valid') == run)[0]
        if len(runs):
            x = runs[-1] + run
            out[i] = x if side == 'right' else im.shape[1] - x
    return out


# The rows (of the photograph reduced to work_px) over which a silhouette's edge is taken as their median.
EDGE_MEDIAN_ROWS = 5


def measure(rel, work_px=1400, thresh=14.0, grabcut=True, shadow=None, window=6, window_side=None, window_bottom=None, side='left',
            lip_rows=0.03, lip_span=0.85, foot_span=(-0.9, 0.2), edge=None):
    """The silhouette of one photograph, in its own pixels: the axis's column, the half width at each row (from the
    clean side, the side away from the cast shadow), the rim's back edge and the foot's front edge, and the rim's and
    foot's half widths. Where the view names an edge (a chroma and a span of its rows), the clean side's edge on those
    rows is traced by colour from the ground inward (chroma_edge)."""
    im = load(rel)
    small, fg, s = mask_of(im, work_px, thresh, grabcut, shadow)
    rows = np.where(fg.any(axis=1))[0]
    L = np.full(fg.shape[0], np.nan); R = np.full(fg.shape[0], np.nan)
    for r in rows:
        c = np.where(fg[r])[0]
        L[r], R[r] = c[0], c[-1] + 1
    T = np.full(fg.shape[1], np.nan); B = np.full(fg.shape[1], np.nan)
    for c in np.where(fg.any(axis=0))[0]:
        r = np.where(fg[:, c])[0]
        T[c], B[c] = r[0], r[-1] + 1
    top, bot = rows[0], rows[-1] + 1
    tall = bot - top
    band = np.arange(top + int(0.2 * tall), top + int(0.6 * tall))
    uc = float(np.median((L[band] + R[band]) / 2))
    rough_hw = np.fmin(uc - L, R - uc)
    hl = np.nanmax(rough_hw[top: top + max(3, int(lip_rows * tall))])
    hf = np.nanmax(rough_hw[bot - max(3, int(0.05 * tall)): bot])
    k = 1.0 / s
    gx, gy = steps(im)
    centres_r = (np.arange(len(L)) + 0.5) * k
    centres_c = (np.arange(len(T)) + 0.5) * k
    ws = (window if window_side is None else window_side) * k
    wb = (window if window_bottom is None else window_bottom) * k
    L = snap(gx, centres_r, L * k, ws, True, outward=-1)
    R = snap(gx, centres_r, R * k, ws, True, outward=1)
    T = snap(gy, centres_c, T * k, window * k, False)
    B = snap(gy, centres_c, B * k, wb, False)
    ucf = uc * k
    if edge:
        span = (centres_r >= edge['rows'][0]) & (centres_r <= edge['rows'][1])
        traced = chroma_edge(im, centres_r[span], side, edge['chroma'])
        if side == 'right':
            R[span] = np.where(np.isnan(traced), R[span], traced)
        else:
            L[span] = np.where(np.isnan(traced), L[span], traced)
    hw = np.fmin(ucf - L, R - ucf) if side == 'both' else (ucf - L if side == 'left' else R - ucf)
    # a soft edge's strongest step can jump a few pixels from row to row: the median of five rows keeps the edge's own
    # turns (a fillet spans several) and drops the jumps
    from scipy.ndimage import median_filter
    known = ~np.isnan(hw)
    hw[known] = median_filter(hw[known], size=EDGE_MEDIAN_ROWS, mode='nearest')
    st = (np.abs(centres_c - ucf) < lip_span * hl * k) & ~np.isnan(T)
    sb = (centres_c - ucf > foot_span[0] * hf * k) & (centres_c - ucf < foot_span[1] * hf * k) & ~np.isnan(B)
    return dict(rel=rel, im=im, uc=ucf, rows=centres_r, hw=hw, top=top * k, bot=bot * k,
                top_u=centres_c[st], top_v=T[st], bot_u=centres_c[sb], bot_v=B[sb], hl=hl * k, hf=hf * k)


# ---------------------------------------------------------------------------------------------------------------------
# The camera
# ---------------------------------------------------------------------------------------------------------------------
def project(P, cam):
    """Image position (u right, v down, pixels) and depth of vessel-frame points P (metres: x the vessel's right, y its
    front, z up from the foot). The camera stands at bearing `azimuth` from the front towards the right, `distance` from
    the axis and `height` above the foot's plane, pitched down by `pitch`; `focal` in pixels; (u0, v0) its optical
    axis's image."""
    az = cam.get('azimuth', 0.0)
    ca, sa = math.cos(az), math.sin(az)
    X = P[..., 0] * ca - P[..., 1] * sa
    Y = P[..., 0] * sa + P[..., 1] * ca
    dy, dz = Y - cam['distance'], P[..., 2] - cam['height']
    cp, sp = math.cos(cam['pitch']), math.sin(cam['pitch'])
    depth = -dy * cp - dz * sp
    up = -dy * sp + dz * cp
    return cam['u0'] + cam['focal'] * X / depth, cam['v0'] - cam['focal'] * up / depth, depth


def ring(r, z, t):
    return np.stack([r * np.cos(t), r * np.sin(t), np.full_like(t, z)], axis=-1)


def edge_residual(cam, r, z, us, vs, back):
    """How far the photograph's edge (us, vs) lies from the ring's image: its back half (the rim seen from above) or its
    front half (the foot)."""
    t = np.linspace(np.pi, 2 * np.pi, 721) if back else np.linspace(0, np.pi, 721)
    u, v, _ = project(ring(r, z, t), cam)
    o = np.argsort(u)
    d = np.interp(us, u[o], v[o], left=np.nan, right=np.nan) - vs
    return np.where(np.isnan(d), 30.0, d)


def extreme_left(cam, r, z):
    """Each ring's leftmost image point (the silhouette's edge at its height)."""
    t = np.linspace(0, 2 * np.pi, 361)
    P = np.stack([r[:, None] * np.cos(t)[None], r[:, None] * np.sin(t)[None], np.repeat(z[:, None], len(t), 1)], -1)
    u, v, _ = project(P, cam)
    i = np.argmin(u, axis=1)
    return u[np.arange(len(r)), i], v[np.arange(len(r)), i]


def half_width(cam, r, z):
    u, _, _ = project(ring(r, z, np.linspace(0, 2 * np.pi, 720, endpoint=False)), cam)
    return 0.5 * (u.max() - u.min())


def fit_camera(m, H, h, radii=None, lip=None, foot=None, focal=None, max_diameter=None, profile_rows=(0.0, 1.0), known=None):
    """The camera, and the rim's and foot's radii, that put the rim's back edge, the foot's front edge and their widths
    where the photograph shows them. Known radii (the museum's lip or foot, or both from a derived section) and a known
    focal length hold them; a published widest diameter holds the section derived through the camera."""
    k0 = (m['bot'] - m['top']) / H
    use_foot_width = foot is None and radii is None
    if known is not None:
        radii = (float(known[1][-1]), float(known[1][0]))

    def unpack(x):
        D = math.exp(x[0])
        cam = dict(distance=D, height=x[1], pitch=x[2], focal=math.exp(x[3]) * D, u0=m['uc'], v0=h / 2)
        rl, rf = radii if radii else (math.exp(x[4]), math.exp(x[5]))
        return cam, rl, rf

    ok = ~np.isnan(m['hw'])

    def residual(x):
        cam, rl, rf = unpack(x)
        a = edge_residual(cam, rl, H, m['top_u'], m['top_v'], True)
        b = edge_residual(cam, rf, 0.0, m['bot_u'], m['bot_v'], False)
        # a further view's foot is often lost in its own cast shadow: its known section's sides stand in for it, unless
        # the view reads its foot by eye (axis_rows)
        parts = [a / math.sqrt(len(a))] + ([] if known is not None and not m.get('by_eye') else [b / math.sqrt(len(b))])
        if known is not None:
            # a known section's rings must meet the silhouette's clean side all the way up
            zk, rk = known
            u, v = extreme_left(cam, rk, zk)
            parts.append((m['uc'] - u - np.interp(v, m['rows'][ok], m['hw'][ok])) / math.sqrt(len(zk)) * 2)
        extra = [half_width(cam, rl, H) - m['hl']]
        if use_foot_width:
            extra.append(half_width(cam, rf, 0.0) - m['hf'])
        if lip is not None:
            extra.append((rl - lip) * k0)
        if foot is not None:
            extra.append((rf - foot) * k0)
        if focal is not None:
            extra.append((cam['focal'] - focal) / focal * 1000.0)
        if max_diameter is not None:
            zs, r = section_through(m, cam, H, rl, rf, 60, 5, profile_rows)
            extra.append((2 * r.max() - max_diameter) * k0)
        return np.concatenate(parts + [np.array(extra)])

    best = None
    # with a known focal length the camera's distance is near the one at which the vessel spans its pixels
    starts = (0.8, 1.5, 3.0, 6.0, 12.0) if focal is None else tuple(focal * H / (m['bot'] - m['top']) * f for f in (0.8, 1.0, 1.25))
    for D0 in starts:
        for e0 in (-0.1, 0.05, 0.15, 0.3, 0.5):
            x0 = [math.log(D0), H * 0.5 + D0 * math.tan(e0), e0, math.log(k0)]
            if not radii:
                x0 += [math.log(m['hl'] / k0), math.log(m['hf'] / k0)]
            r = least_squares(residual, x0, x_scale='jac', max_nfev=600)
            if 0.2 < math.exp(r.x[0]) < 100 and (best is None or r.cost < best.cost):
                best = r
    cam, rl, rf = unpack(best.x)
    return cam, rl, rf, float(np.sqrt(np.mean(best.fun ** 2)))


# Where a section keeps its fine turns (a fillet, a rolled lip, a neck's step), its edge is smoothed over this many
# samples (a sample a row of the photograph): the edge's own noise, a pixel or two.
FINE_SMOOTHING = 2.0


def section_through(m, cam, H, rl, rf, n, iters, profile_rows=(0.0, 1.0), lip_ring=False, fine=()):
    """The outside's radius at n heights from the foot to the lip: each height's ring turned until its leftmost (or
    rightmost) image point meets the silhouette's edge. Heights outside profile_rows (fractions of the height whose edge
    is clean) are bridged by a cubic from the last clean ring to the fitted foot or rim, matching its slope. The foot's
    ring is the fitted foot's; the lip's is the silhouette's own, unless the museum's mouth holds it (lip_ring), so the
    rim's outer edge stays where the photograph shows it. The edge's noise is smoothed over a hundredth of the samples:
    against the ground a pale glaze's edge is faint, and a painted line just inside it can pull the edge in by a few
    pixels; within `fine` (spans of the height, fractions, where the view's edge is crisp and the vessel turns sharply)
    over FINE_SMOOTHING samples only, so its fillets and lip keep their shape."""
    rows, hw = m['rows'], m['hw']
    ok = ~np.isnan(hw)
    zs = np.linspace(0.0, H, n)
    t = np.linspace(0, 2 * np.pi, 361)
    r = np.interp(m['bot'] - (m['bot'] - m['top']) * zs / H, rows[ok], hw[ok]) / cam['focal'] * cam['distance']
    # the silhouette closes to a point under the foot and over the rim: start no ring narrower than a third of the
    # fitted foot's or rim's
    r = np.maximum(r, np.minimum(rf, rl) / 3.0)
    for _ in range(iters):
        P = np.stack([r[:, None] * np.cos(t)[None], r[:, None] * np.sin(t)[None], np.repeat(zs[:, None], len(t), 1)], -1)
        u, v, _ = project(P, cam)
        i = np.argmin(u, axis=1)
        ul, vl = u[np.arange(n), i], v[np.arange(n), i]
        observed = np.interp(vl, rows[ok], hw[ok])
        r = r * np.clip(observed / np.maximum(m['uc'] - ul, 1e-6), 0.5, 2.0)
    # the unclean heights are bridged before the edge is smoothed, so nothing of their edge is smoothed into the clean
    lo, hi = profile_rows
    if lo > 0.0:
        r = bridge(zs, r, lo * H, 0.0, rf, below=True)
    if hi < 1.0:
        r = bridge(zs, r, hi * H, H, rl, below=False)
    # the silhouette's edge is found to a fraction of a pixel, but not without noise: smooth over a hundredth of the height
    from scipy.ndimage import gaussian_filter1d
    body = gaussian_filter1d(r, sigma=max(1.0, 0.01 * n), mode='nearest')
    crisp = gaussian_filter1d(r, sigma=FINE_SMOOTHING, mode='nearest')
    share = np.zeros(n)
    for lo, hi in fine:
        share = np.maximum(share, ((zs >= lo * H) & (zs <= hi * H)).astype(float))
    # the two meet over a hundredth of the height, so the section doesn't step where they meet
    share = gaussian_filter1d(share, sigma=max(1.0, 0.01 * n), mode='nearest') if fine else share
    r = share * crisp + (1.0 - share) * body
    if lip_ring:
        r[-1] = rl
    # the foot's own ring is the ellipse the camera was fitted to
    r[0] = rf
    return zs, np.maximum(r, 0.0)


def pixel_rows(m):
    """How many image rows the vessel spans: the section is sampled as finely as its photograph resolves it."""
    return int(min(max(m['bot'] - m['top'], 400), 3000))


def bridge(zs, r, z_clean, z_end, r_end, below):
    """A cubic from the clean ring at z_clean (its value and slope, fitted by a line to the clean rings over the next
    fiftieth of the height, on the clean side alone) to r_end at z_end, reaching it square (the slope a foot or lip
    ring's own edge has)."""
    i = int(np.searchsorted(zs, z_clean))
    i = min(max(i, 2), len(zs) - 3)
    reach = max(3, int(0.02 * len(zs)))
    side = slice(i, min(len(zs), i + reach)) if below else slice(max(0, i - reach + 1), i + 1)
    slope, intercept = np.polyfit(zs[side], r[side], 1)
    z0, r0 = zs[i], slope * zs[i] + intercept
    span = z_end - z0
    out = r.copy()
    sel = zs <= z0 if below else zs >= z0
    s = (zs[sel] - z0) / span
    h00, h10, h01 = 2 * s ** 3 - 3 * s ** 2 + 1, s ** 3 - 2 * s ** 2 + s, -2 * s ** 3 + 3 * s ** 2
    out[sel] = h00 * r0 + h10 * span * slope + h01 * r_end
    return out


# ---------------------------------------------------------------------------------------------------------------------
# The wall
# ---------------------------------------------------------------------------------------------------------------------
def wall_of(vessel, cam, ware, lip):
    """The wall a vessel is drawn with, and how it was found. A thin-potted one's is its lip's thickness as its
    photograph shows it where the rim turns at the photograph's side, edge on: the lit lip's width in pixels between the
    ground and the shaded inside (read on the photograph's pixel grid, its rows and columns in the vessel's table),
    through the camera's scale there. Any other's is Data/Art/Ware.json's rule."""
    read = vessel.get('lip')
    if read:
        px = read['columns'][1] - read['columns'][0]
        t = px * cam['distance'] / cam['focal']
        return t, ("the wall its lip's thickness in %s, %d px (columns %d..%d on rows %d..%d, the lit lip between the "
                   "ground and the shaded inside) at %.4f mm a pixel: %.2f mm" % (
                       vessel['views'][0]['image'].split('/')[-1], px, read['columns'][0], read['columns'][1], read['rows'][0],
                       read['rows'][1], 1000 * cam['distance'] / cam['focal'], 1000 * t))
    t = min(ware['wallMeters'], ware['wallOfLip'] * lip)
    return t, "the wall Data/Art/Ware.json's, %.1f mm" % (1000 * t)


# ---------------------------------------------------------------------------------------------------------------------
# The closed section
# ---------------------------------------------------------------------------------------------------------------------
def simplified(points, tolerance):
    """The indices of a polyline's points that keep it within `tolerance` of itself (Ramer-Douglas-Peucker): few points
    where it runs straight, many where it turns sharply, so a fillet or a rolled lip keeps its shape."""
    keep = np.zeros(len(points), bool)
    keep[0] = keep[-1] = True
    stack = [(0, len(points) - 1)]
    while stack:
        a, b = stack.pop()
        if b <= a + 1:
            continue
        run = points[b] - points[a]
        length = np.hypot(*run)
        rel = points[a + 1:b] - points[a]
        d = np.abs(rel[:, 0] * run[1] - rel[:, 1] * run[0]) / length if length > 0 else np.hypot(rel[:, 0], rel[:, 1])
        k = int(np.argmax(d))
        if d[k] > tolerance:
            keep[a + 1 + k] = True
            stack += [(a, a + 1 + k), (a + 1 + k, b)]
    return np.where(keep)[0]


def crossing(p, q, a, b):
    """Where segment pq crosses segment ab (strictly inside both), or None."""
    d = (q[0] - p[0]) * (b[1] - a[1]) - (q[1] - p[1]) * (b[0] - a[0])
    if abs(d) < 1e-15:
        return None
    s = ((a[0] - p[0]) * (b[1] - a[1]) - (a[1] - p[1]) * (b[0] - a[0])) / d
    u = ((a[0] - p[0]) * (q[1] - p[1]) - (a[1] - p[1]) * (q[0] - p[0])) / d
    if 1e-9 < s < 1 - 1e-9 and 1e-9 < u < 1 - 1e-9:
        return (p[0] + s * (q[0] - p[0]), p[1] + s * (q[1] - p[1]))
    return None


def untangled(points):
    """A polyline with its loops cut out: where it crosses itself (an inward offset through a hollow tighter than the
    offset), the part between the crossings is dropped and the crossing kept."""
    pts = [tuple(p) for p in points]
    i = 0
    while i < len(pts) - 1:
        for j in range(len(pts) - 2, i + 1, -1):
            x = crossing(pts[i], pts[i + 1], pts[j], pts[j + 1])
            if x is not None:
                pts = pts[:i + 1] + [x] + pts[j + 1:]
                break
        i += 1
    return np.array(pts)


def offset_inward(rz, t):
    """The outside's polyline [radius, height] moved a wall's thickness t in along its normal, its loops cut out."""
    tangent = np.gradient(rz, axis=0)
    tangent /= np.maximum(np.hypot(tangent[:, 0], tangent[:, 1]), 1e-12)[:, None]
    # the solid lies on the section's left, so the outward normal is the tangent turned right: (dz, -dr)
    normal = np.stack([tangent[:, 1], -tangent[:, 0]], 1)
    return untangled(rz - t * normal)


def section_faults(section, first, last, t):
    """What is wrong with a closed section: two of its segments crossing (a surface through a surface), or its wall
    anywhere thinner than half the wall it was drawn with (an inside point nearer the outside than that)."""
    faults = []
    pts = [tuple(p) for p in section]
    n = len(pts)
    for i in range(n - 1):
        for j in range(i + 2, n - 1):
            if crossing(pts[i], pts[i + 1], pts[j], pts[j + 1]) is not None:
                faults.append('segments %d and %d cross' % (i, j))
    outer = np.array(pts[:last + 1])
    for k in range(last + 2, n - 1):
        p = np.array(pts[k])
        a, b = outer[:-1], outer[1:]
        ab = b - a
        s = np.clip(((p - a) * ab).sum(1) / np.maximum((ab * ab).sum(1), 1e-18), 0, 1)
        d = np.hypot(*(a + s[:, None] * ab - p).T).min()
        if d < 0.5 * t:
            faults.append('the wall at inside point %d is %.2f mm, under half the %.2f mm drawn' % (k, 1000 * d, 1000 * t))
    return faults


def closed_section(zs, r, t, ware, open_form, tolerance):
    """[radius, height] from the axis under the foot, out along the underside, up the outside, over the lip and down
    the inside to the axis, the solid on the left; and the indices of the outside's first and last points.
    - The outside is the derived profile, kept within `tolerance` of itself (simplified), to the lip's shoulder half a
      wall under the top; the lip turns over it, square to the wall, to the top.
    - The wall is t thick: the foot ring as wide as the wall and the base recessed by it; the inside is the outside moved
      t in along its normal (its loops cut out), so the wall is t thick everywhere; an open form's runs down to a floor a
      wall thick over the recess, a closed form's down its neck Data/Art/Ware.json's neck depth to a floor.
    Raises if the section crosses itself or its wall thins under half of t."""
    H = zs[-1]
    # the lip's cap: centred half a wall in from the outside's top, square to the wall there
    slope = (r[-1] - np.interp(H - t, zs, r)) / t
    along = np.array([slope, 1.0]) / math.hypot(slope, 1.0)
    out = np.array([along[1], -along[0]])
    top = H - 0.5 * t * (along[1] - out[1])
    keep = zs < top
    rz = np.vstack([np.stack([r[keep], zs[keep]], 1), [np.interp(top, zs, r), top]])
    shoulder = rz[-1]
    apex = shoulder - 0.5 * t * out + 0.5 * t * along
    inner_top = shoulder - t * out
    outside = rz[simplified(rz, tolerance)]
    rf = outside[0][0]
    under = [[0.0, t], [rf - t, t], [rf - t, 0.0]]
    floor = 2 * t if open_form else H - min(ware['neckDepthOfHeight'] * H, ware['neckDepthMeters'])
    # the inside from under the lip down to the floor, where it turns across to the axis
    offset = offset_inward(rz, t)[::-1]
    inner = [inner_top]
    for a, b in zip(offset[:-1], offset[1:]):
        # only ever downwards: where the offset turns back up (under a flared lip, over a fillet tighter than the wall)
        # it is left out, so the inside runs down clear of itself
        if b[1] >= inner[-1][1]:
            continue
        if b[1] <= floor:
            inner.append(a + (a[1] - floor) / (a[1] - b[1]) * (b - a) if floor < a[1] < inner[-1][1] else np.array([inner[-1][0], floor]))
            break
        inner.append(b)
    inner = np.array(inner)
    if inner[-1][1] > floor:
        inner = np.vstack([inner, [inner[-1][0], floor]])
    inner[:, 0] = np.maximum(inner[:, 0], 0.0)
    inner = inner[simplified(inner, tolerance)]
    inside = [[float(a), float(b)] for a, b in inner] + [[0.0, float(floor)]]
    section = under + [[float(a), float(b)] for a, b in outside] + [[float(apex[0]), float(apex[1])]] + inside
    first = len(under)
    last = first + len(outside) - 1
    section = [[round(a, 5), round(b, 5)] for a, b in section]
    faults = section_faults(section, first, last, t)
    if faults:
        raise SystemExit('the closed section is not a solid: ' + '; '.join(faults[:6]))
    return section, first, last + 1


# ---------------------------------------------------------------------------------------------------------------------
# Views of the decoration
# ---------------------------------------------------------------------------------------------------------------------
def unwrap(view, cam, zs, r):
    """The photograph's picture of the outside, unwrapped: rows up the height, columns round from the front towards
    the right; and how squarely the view sees each cell (0 where it doesn't)."""
    im = view['im']
    phi = (np.arange(AZIMUTH_STEPS) + 0.5) / AZIMUTH_STEPS * 2 * np.pi
    z = (np.arange(HEIGHT_STEPS) + 0.5) / HEIGHT_STEPS * zs[-1]
    rr = np.interp(z, zs, r)
    slope = np.gradient(rr, z)
    nr, nz = 1 / np.sqrt(1 + slope ** 2), -slope / np.sqrt(1 + slope ** 2)
    # azimuth phi: 0 at the front (+y), increasing towards the right (+x)
    P = np.stack([rr[:, None] * np.sin(phi)[None], rr[:, None] * np.cos(phi)[None], np.repeat(z[:, None], AZIMUTH_STEPS, 1)], -1)
    N = np.stack([nr[:, None] * np.sin(phi)[None], nr[:, None] * np.cos(phi)[None], np.repeat(nz[:, None], AZIMUTH_STEPS, 1)], -1)
    u, v, depth = project(P, cam)
    az = cam.get('azimuth', 0.0)
    C = np.array([cam['distance'] * math.sin(az), cam['distance'] * math.cos(az), cam['height']])
    sight = C[None, None, :] - P
    sight /= np.linalg.norm(sight, axis=-1, keepdims=True)
    square = np.clip((N * sight).sum(-1), 0, 1)
    pic = cv2.remap(im, (u - 0.5).astype(np.float32), (v - 0.5).astype(np.float32), cv2.INTER_LINEAR, borderMode=cv2.BORDER_CONSTANT)
    inside = (u > 0) & (u < im.shape[1]) & (v > 0) & (v < im.shape[0])
    square[~inside] = 0
    return pic, square


def detail(pic):
    """The decoration's own detail for registration: lightness band-passed (3 to 15 cells), so shading, exposure and a
    photograph's grain don't count."""
    g = cv2.cvtColor(pic, cv2.COLOR_BGR2LAB)[..., 0].astype(np.float32)
    return cv2.GaussianBlur(g, (0, 0), 3) - cv2.GaussianBlur(g, (0, 0), 15)


# Registration looks further round than composition does: to 78 degrees off face on, where a view's picture is
# stretched fivefold but its coarse detail still lines up with a neighbour's.
REGISTRATION_CUT = 0.2


def weights(square, cut=None):
    cut = REGISTRATION_CUT if cut is None else cut
    return np.clip((square - cut) / (1 - cut), 0, 1)


def correlation(da, wa, db, wb):
    w = wa * wb
    tot = w.sum()
    if tot < 0.02 * w.size:
        return -1.0
    mx, my = (w * da).sum() / tot, (w * db).sum() / tot
    return float((w * (da - mx) * (db - my)).sum() / math.sqrt((w * (da - mx) ** 2).sum() * (w * (db - my) ** 2).sum() + 1e-9))


def register(base, other, rise=24):
    """The turn (radians, towards the right) and the rise (cells up the height) that best lay other's unwrapped
    picture on base's, by normalised correlation of their detail where both see squarely; and that correlation."""
    (pa, sa), (pb, sb) = base, other
    da, db, wa, wb = detail(pa), detail(pb), weights(sa), weights(sb)
    best = (-2.0, 0, 0)
    for dv in range(-rise, rise + 1, 2):
        dbv, wbv = np.roll(db, dv, axis=0), np.roll(wb, dv, axis=0)
        if dv > 0:
            wbv[:dv] = 0
        elif dv < 0:
            wbv[dv:] = 0
        for shift in range(0, AZIMUTH_STEPS, 2):
            c = correlation(da, wa, np.roll(dbv, shift, axis=1), np.roll(wbv, shift, axis=1))
            if c > best[0]:
                best = (c, shift, dv)
    return best[1] / AZIMUTH_STEPS * 2 * np.pi, best[2], best[0]


def refine_turn(base, view, cam, zs, r, reach=3.0, step=0.1):
    """The further view's turn refined so its picture of the decoration lies best on the first view's (degrees within
    reach of the registered turn, in steps), its camera otherwise the one its own silhouette fixed: a camera moved off
    its silhouette to make its painting fit would put the painting where the vessel isn't."""
    da, wa = detail(base[0]), weights(base[1])
    best = (-2.0, cam['azimuth'])
    for d in np.arange(-reach, reach + 0.5 * step, step):
        c = dict(cam, azimuth=cam['azimuth'] + math.radians(d))
        pb, sb = unwrap(view, c, zs, r)
        k = correlation(da, wa, detail(pb), weights(sb))
        if k > best[0]:
            best = (k, c['azimuth'])
    return dict(cam, azimuth=best[1] % (2 * np.pi)), best[0]


# A further view's camera must fit its own silhouette within this (rms over its clean sides and its rim's back edge, in
# metres at the vessel through its camera): a camera that misses it puts the painting where the vessel isn't. A hand-
# thrown vessel is not quite round, and its sides seen from another bearing stray from the one section by a millimetre
# or so; a camera moved off its silhouette to fit the painting strays by centimetres.
SILHOUETTE_LIMIT = 0.002
# Where two views overlap, their paintings must lie on each other within this in every band of height they share
# (registered band by band, turn and rise): further apart, composed, the painting would be doubled.
AGREEMENT_METERS = 0.005
AGREEMENT_BANDS = 5


def silhouette_rms(m, cam, H, zs, r, foot, rows=(0.0, 1.0)):
    """How far the section's image through a camera strays from the photograph's silhouette (rms, pixels): its sides
    at 40 heights of the clean edge, the rim's back edge, and for the view the section was derived from its foot's
    front edge (a further view's foot is often lost in its own shadow)."""
    # the turned surface looks the same from every bearing: its silhouette is the camera's at bearing 0
    cam = dict(cam, azimuth=0.0)
    ok = ~np.isnan(m['hw'])
    lo, hi = max(0.15, rows[0]), min(0.92, rows[1])
    pick = np.linspace(lo * (len(zs) - 1), hi * (len(zs) - 1), 40).astype(int)
    u, v = extreme_left(cam, r[pick], zs[pick])

    def middle(us, vs):
        # an ellipse's ends run steeply, where a hair's error across is many pixels down: its middle four fifths
        span = us.max() - us.min() if len(us) else 0.0
        sel = np.abs(us - 0.5 * (us.max() + us.min())) <= 0.4 * span if len(us) else np.zeros(0, bool)
        return us[sel], vs[sel]
    parts = [m['uc'] - u - np.interp(v, m['rows'][ok], m['hw'][ok]), edge_residual(cam, r[-1], H, *middle(m['top_u'], m['top_v']), True)]
    if foot:
        parts.append(edge_residual(cam, r[0], 0.0, *middle(m['bot_u'], m['bot_v']), False))
    return float(np.sqrt(np.mean(np.concatenate(parts) ** 2)))


def agreement(a, b, H, reach=0.04, turn_cells=8):
    """How two placed views' paintings lie on each other band by band up the height: for each band both see, the rise
    (metres) and turn (degrees) that lay b's best on a's, their correlation there and the share of the band both see.
    Views placed right agree at no rise and no turn in every band."""
    da, db, wa, wb = detail(a[0]), detail(b[0]), weights(a[1]), weights(b[1])
    cells = int(math.ceil(reach / (H / HEIGHT_STEPS)))
    out = []
    for k in range(AGREEMENT_BANDS):
        lo, hi = k * HEIGHT_STEPS // AGREEMENT_BANDS, (k + 1) * HEIGHT_STEPS // AGREEMENT_BANDS
        shared = float((wa[lo:hi] * wb[lo:hi]).mean())
        best = (-2.0, 0, 0)
        for turn in range(-turn_cells, turn_cells + 1):
            dbt, wbt = np.roll(db, turn, 1), np.roll(wb, turn, 1)
            for dv in range(-cells, cells + 1):
                dbv, wbv = np.roll(dbt, dv, 0), np.roll(wbt, dv, 0)
                if dv > 0:
                    wbv[:dv] = 0
                elif dv < 0:
                    wbv[dv:] = 0
                c = correlation(da[lo:hi], wa[lo:hi], dbv[lo:hi], wbv[lo:hi])
                if c > best[0]:
                    best = (c, dv, turn)
        out.append(dict(rows=(lo / HEIGHT_STEPS, hi / HEIGHT_STEPS), rise=best[1] * H / HEIGHT_STEPS,
                        turn=best[2] * 360.0 / AZIMUTH_STEPS, correlation=best[0], shared=shared))
    return out


def disagreement(bands):
    """The bands in which two views' paintings don't lie on each other: registered at a rise past AGREEMENT_METERS
    where their paintings correlate as a genuine overlap does (GENUINE): a plain band, or a sliver seen by both, has
    nothing to register."""
    return [b for b in bands if b['correlation'] >= GENUINE and abs(b['rise']) > AGREEMENT_METERS]


def describe(bands):
    return ', '.join('%.1f-%.1f H %+.1f cm (correlation %.2f)' % (b['rows'][0], b['rows'][1], 100 * b['rise'], b['correlation'])
                     for b in bands if b['correlation'] >= GENUINE)


# Two views of the same stretch of painting correlate at 0.5 or better once registered (the Xuande jar's pair, 0.76);
# a repeat of the design elsewhere round the vessel, fish for fish, at 0.4 or less. A turn is scored by how far each
# overlap's correlation stands above the line between them, weighted by the overlap.
GENUINE = 0.45
MIN_OVERLAP = 0.005


def landscape(a, b):
    """For each turn of b against a (in cells, two at a time): their correlation and their overlap."""
    da, db, wa, wb = detail(a[0]), detail(b[0]), weights(a[1]), weights(b[1])
    C = np.full(AZIMUTH_STEPS, -1.0)
    O = np.zeros(AZIMUTH_STEPS)
    for shift in range(0, AZIMUTH_STEPS, 2):
        rb, rw = np.roll(db, shift, axis=1), np.roll(wb, shift, axis=1)
        O[shift] = float((wa * rw).mean())
        if O[shift] >= MIN_OVERLAP:
            C[shift] = correlation(da, wa, rb, rw)
    return C, O


def score(C, O, shift):
    shift %= AZIMUTH_STEPS
    return O[shift] * (C[shift] - GENUINE) if O[shift] >= MIN_OVERLAP else 0.0


def place(base, fitted, zs, r):
    """Each further view's turn round the vessel, and how it was found. One view is registered on the first by turn and
    rise, and its turn refined on the painting (its camera is its silhouette's). A turntable series is placed jointly:
    every pair's correlation is scored (GENUINE), and the turns that score best together win, so a fish is not mistaken
    for its neighbour. A view is 'registered' where a chain of genuine overlaps ties it to the first view. Views whose
    painting shares nothing with the first's are 'unverified': their turn against the first is not in the photographs;
    as a group (keeping the turns their own overlaps give them) they are set where they stand furthest from every other
    view, as a turntable series is photographed round the vessel. Returns, per view, its camera, its placement and its
    overlaps with the others [(index, share, correlation)]."""
    pics = [unwrap(mv, cv, zs, r) for _, mv, cv in fitted]
    if len(fitted) == 1 and not fitted[0][0].get('same_camera'):
        v, mv, cv = fitted[0]
        turn, _, _ = register(base, pics[0])
        cv['azimuth'] = turn
        cv, corr = refine_turn(base, mv, cv, zs, r)
        C, O = landscape(base, unwrap(mv, cv, zs, r))
        return [(cv, 'registered', [(0, float(O[0]), corr)])]
    allp = [base] + pics
    n = len(allp)
    land = {(i, j): landscape(allp[i], allp[j]) for i in range(n) for j in range(i + 1, n)}
    grid = range(0, AZIMUTH_STEPS, 2)
    best = (-1e9, None)
    import itertools
    for turns in itertools.product(grid, repeat=n - 1):
        t = (0,) + turns
        total = sum(score(*land[(i, j)], t[j] - t[i]) for i in range(n) for j in range(i + 1, n))
        if total > best[0]:
            best = (total, t)
    t = list(best[1])

    def pair(i, j):
        (C, O), shift = land[(min(i, j), max(i, j))], (t[max(i, j)] - t[min(i, j)]) % AZIMUTH_STEPS
        return float(O[shift]), float(C[shift])

    # the views tied to the first by genuine overlaps; the rest in groups of their own
    group = list(range(n))

    def root(i):
        while group[i] != i:
            i = group[i]
        return i
    for i in range(n):
        for j in range(i + 1, n):
            share, corr = pair(i, j)
            if share >= MIN_OVERLAP and corr >= GENUINE:
                group[root(j)] = root(i)
    for g in sorted({root(i) for i in range(n)} - {root(0)}):
        members = [i for i in range(n) if root(i) == g]
        others = [i for i in range(n) if root(i) != g]

        def least_gap(delta):
            gaps = [(t[i] + delta - t[j]) % AZIMUTH_STEPS for i in members for j in others]
            return min(min(d, AZIMUTH_STEPS - d) for d in gaps)
        delta = max(range(0, AZIMUTH_STEPS, 2), key=lambda d: (least_gap(d), -d))
        for i in members:
            t[i] = (t[i] + delta) % AZIMUTH_STEPS
    out = []
    for k, (v, mv, cv) in enumerate(fitted):
        cv['azimuth'] = t[k + 1] / AZIMUTH_STEPS * 2 * np.pi
        overlaps = [(i,) + pair(i, k + 1) for i in range(n) if i != k + 1 and pair(i, k + 1)[0] >= MIN_OVERLAP]
        out.append((cv, 'registered' if root(k + 1) == root(0) else 'unverified', overlaps))
    return out


def band_period(unwrapped, H, rows, shortest=6.0, longest=120.0):
    """A band's repeat round the vessel (degrees, a whole number to the turn): the turn at which its painting, as the
    placed views show it, best matches itself (autocorrelation of its detail along the band, where it is seen)."""
    z = (np.arange(HEIGHT_STEPS) + 0.5) / HEIGHT_STEPS * H
    band = (z >= rows[0] * H) & (z <= rows[1] * H)
    signal = np.zeros(AZIMUTH_STEPS)
    seen = np.zeros(AZIMUTH_STEPS)
    for pic, square in unwrapped:
        d = detail(pic)[band]
        w = np.clip((square[band] - SQUARENESS_CUT) / (1 - SQUARENESS_CUT), 0, 1)
        signal += (d * w).sum(axis=0)
        seen += w.sum(axis=0)
    s = np.where(seen > 0, signal / np.maximum(seen, 1e-9), 0.0)
    best = (-2.0, 360.0)
    # a lag is trusted only where the band is seen over at least half as much of the turn both ways as it is at all:
    # a lag near the seen arc's own width compares its two ends alone
    whole = (seen * seen).sum()
    for lag in range(int(shortest / 360 * AZIMUTH_STEPS), int(longest / 360 * AZIMUTH_STEPS) + 1):
        w = seen * np.roll(seen, lag)
        if w.sum() < 0.5 * whole:
            continue
        a, b = s, np.roll(s, lag)
        ma, mb = (w * a).sum() / w.sum(), (w * b).sum() / w.sum()
        c = (w * (a - ma) * (b - mb)).sum() / math.sqrt((w * (a - ma) ** 2).sum() * (w * (b - mb) ** 2).sum() + 1e-12)
        if c > best[0]:
            best = (c, lag * 360.0 / AZIMUTH_STEPS)
    repeats = max(1, int(round(360.0 / best[1])))
    return 360.0 / repeats, float(best[0])


def coverage(views):
    """The share of the outside some view sees at least as squarely as the cut."""
    seen = np.zeros((HEIGHT_STEPS, AZIMUTH_STEPS), bool)
    for pic, square in views:
        seen |= square >= SQUARENESS_CUT
    return float(seen.mean())


# ---------------------------------------------------------------------------------------------------------------------
# A glaze's colour
# ---------------------------------------------------------------------------------------------------------------------
def glaze_colour(view, cam, zs, r):
    """The glaze's CIELAB (D65) from the photograph: the median of the cells the camera sees within 30 degrees of face
    on, clear of the highlights (the brightest tenth), relative to the photograph's white (its 99.5th percentile of
    lightness over the whole frame, taken as L* 100)."""
    pic, square = unwrap(view, cam, zs, r)
    lab = cv2.cvtColor(pic, cv2.COLOR_BGR2LAB).astype(np.float32)
    L = lab[..., 0] * 100 / 255
    sel = square > math.cos(math.radians(30))
    cut = np.percentile(L[sel], 90)
    sel &= L < cut
    white = np.percentile(cv2.cvtColor(view['im'], cv2.COLOR_BGR2LAB)[..., 0].astype(np.float32) * 100 / 255, 99.5)
    Lm = float(np.median(L[sel])) * 100 / white
    a = float(np.median(lab[..., 1][sel]) - 128)
    b = float(np.median(lab[..., 2][sel]) - 128)
    spread = float(np.std(np.log(np.maximum(L[sel], 1))))
    return [round(float(Lm), 1), round(a, 1), round(b, 1)], round(spread, 3), round(float(white), 1)


# ---------------------------------------------------------------------------------------------------------------------
def fit_view(v, H, museum, zs=None, r=None):
    m = measure(v['image'], **v.get('measure', {}))
    if 'axis_rows' in v:
        # where the photograph's own edges are lost (a highlight's bloom over the rim, a shelf's reflection under the
        # foot), the rim's top and the foot's bottom on the axis, read from the photograph by eye
        top, bottom = v['axis_rows']
        m.update(top_u=np.array([m['uc'] - 1.0, m['uc'], m['uc'] + 1.0]), top_v=np.full(3, float(top)),
                 bot_u=np.array([m['uc'] - 1.0, m['uc'], m['uc'] + 1.0]), bot_v=np.full(3, float(bottom)), by_eye=True)
    h = m['im'].shape[0]
    fixed = v.get('fixed', {})
    kw = {}
    if fixed.get('lip') == 'diameter':
        kw['lip'] = museum['diameterMeters'] / 2
    if fixed.get('foot') == 'foot':
        kw['foot'] = museum['footMeters'] / 2
    if fixed.get('max') == 'diameter':
        kw['max_diameter'] = museum['diameterMeters']
    if 'focal' in fixed:
        kw['focal'] = fixed['focal']
    if zs is not None:
        lo, hi = v.get('sides', (0.15, 0.92))
        pick = np.linspace(lo * (len(zs) - 1), hi * (len(zs) - 1), 40).astype(int)
        kw['known'] = (zs[pick], r[pick])
    rows = v.get('rows', (0.0, 1.0))
    cam, rl, rf, rms = fit_camera(m, H, h, profile_rows=rows, **kw)
    return m, cam, rl, rf, rms


def sheet(folder, name, m, cam, zs, r):
    im = m['im'].copy()
    t = np.linspace(0, 2 * np.pi, 720)
    for z, x in list(zip(zs, r))[::max(1, len(zs) // 16)] + [(zs[-1], r[-1]), (zs[0], r[0])]:
        u, v, _ = project(ring(x, z, t), cam)
        cv2.polylines(im, [np.stack([u, v], -1).astype(np.int32)], True, (0, 0, 255), max(1, im.shape[0] // 1000))
    s = 1200 / max(im.shape[:2])
    cv2.imwrite(os.path.join(folder, name), cv2.resize(im, None, fx=s, fy=s, interpolation=cv2.INTER_AREA))


# The section's outside is kept to within this of the derived profile (or two pixels of its photograph, its edge's
# noise, if coarser):
# enough points to keep a fillet or a rolled lip, few where it runs straight.
SECTION_TOLERANCE = 0.0001
SECTION_PIXELS = 2.0


def derive(vessel, ware, sheets):
    H = vessel['museum']['heightMeters']
    museum = vessel['museum']
    views = vessel['views']
    first = next(v for v in views if v.get('profile'))
    rows = first.get('rows', (0.0, 1.0))
    m, cam, rl, rf, rms = fit_view(first, H, museum)
    lip_ring = first.get('fixed', {}).get('lip') == 'diameter'
    zs, r = section_through(m, cam, H, rl, rf, pixel_rows(m), 12, rows, lip_ring=lip_ring, fine=first.get('fine', ()))
    out = dict(work=vessel['work'], zs=zs, r=r, views=[], checks=[], unused=list(vessel.get('unused', [])))
    out['checks'].append('widest diameter %.1f cm (museum %s)' % (200 * r.max(), '%.1f cm' % (100 * museum['diameterMeters']) if 'diameterMeters' in museum else 'none published'))
    if 'footMeters' in museum:
        out['checks'].append('foot %.1f cm (museum %.1f cm)' % (200 * r[0], 100 * museum['footMeters']))
    if not lip_ring:
        out['checks'].append("the rim's outer radius %.1f mm from the silhouette (the rim's fitted ellipse %.1f)" % (1000 * r[-1], 1000 * rl))
    # the foot's edge counts where the view's silhouette is clean down to it
    out['views'].append(dict(view=first, m=m, cam=cam, rms=silhouette_rms(m, cam, H, zs, r, rows[0] == 0.0, rows), placement='fitted', overlaps=[]))
    if sheets:
        sheet(sheets, vessel['work'] + '-0.jpg', m, cam, zs, r)
    unwrapped = [unwrap(m, cam, zs, r)]
    fitted = []
    for v in (vv for vv in views if vv is not first):
        if v.get('same_camera'):
            # a series taken on a turntable: the first view's camera, the vessel set down where this photograph shows it
            # (its axis's column, and its foot and rim as far up or down the frame as they stand in it)
            mv = measure(v['image'], **v.get('measure', {}))
            cv = dict(cam, u0=mv['uc'], v0=cam['v0'] + 0.5 * (mv['top'] + mv['bot'] - m['top'] - m['bot']))
        else:
            mv, cv, _, _, _ = fit_view(v, H, museum, zs, r)
        fitted.append([v, mv, cv])
    placed = place(unwrapped[0], fitted, zs, r) if fitted else []
    pics = [unwrap(mv, cv, zs, r) for (_, mv, _), (cv, _, _) in zip(fitted, placed)]
    allp = unwrapped + pics
    for k, ((v, mv, _), (cv, placement, overlaps)) in enumerate(zip(fitted, placed)):
        fit = silhouette_rms(mv, cv, H, zs, r, False, v.get('rows', (0.0, 1.0)))
        fit_meters = fit * cv['distance'] / cv['focal']
        # every view it overlaps must agree with it band by band
        wrong = []
        for i, share, corr in overlaps:
            bands = agreement(allp[i], allp[k + 1], H)
            if disagreement(bands):
                wrong.append((i, bands))
        name = v['image'].split('/')[-1]
        if fit_meters > SILHOUETTE_LIMIT or wrong:
            why = []
            if fit_meters > SILHOUETTE_LIMIT:
                why.append('its camera misses its own silhouette by %.1f px rms, %.1f mm at the vessel (over %.0f)' % (
                    fit, 1000 * fit_meters, 1000 * SILHOUETTE_LIMIT))
            for i, bands in wrong:
                why.append("placed on %s's painting, its painting lies off it band by band up the height (%s; over %.1f cm)" % (
                    allp_name(views, first, fitted, i), describe(bands), 100 * AGREEMENT_METERS))
            out['unused'].append(dict(image='assets/' + v['image'], why='; '.join(why) + ': composed, it would double the painting'))
            print('  %s left out: %s' % (name, '; '.join(why)))
            continue
        out['views'].append(dict(view=v, m=mv, cam=cv, rms=fit, placement=placement,
                                 overlaps=[(allp_name(views, first, fitted, i), share, corr) for i, share, corr in overlaps]))
        unwrapped.append(allp[k + 1])
        if sheets:
            sheet(sheets, vessel['work'] + '-%d.jpg' % (k + 1), mv, cv, zs, r)
    out['coverage'] = coverage(unwrapped)
    out['periods'] = [band_period(unwrapped, H, b['rows'], b.get('shortest', 6.0)) for b in vessel['bands']]
    out['unwrapped'] = unwrapped
    if not vessel['painted']:
        out['glaze'] = glaze_colour(m, cam, zs, r)
    open_form = r[-1] >= 0.95 * r.max()
    t, how = wall_of(vessel, cam, ware, r[-1])
    tolerance = max(SECTION_TOLERANCE, SECTION_PIXELS * cam['distance'] / cam['focal'])
    out['section'], out['first'], out['last'] = closed_section(zs, r, t, ware, open_form, tolerance)
    out['open'] = open_form
    out['wall'], out['wall_how'] = t, how
    return out


def allp_name(views, first, fitted, i):
    """The photograph of the i-th placed view (0 the first)."""
    return (first if i == 0 else fitted[i - 1][0])['image'].split('/')[-1]


def record_of(vessel, d):
    museum = vessel['museum']
    views = []
    for v in d['views']:
        c = v['cam']
        view = dict(image='assets/' + v['view']['image'], azimuthDegrees=round(math.degrees(c.get('azimuth', 0.0)), 3),
                    distanceMeters=round(c['distance'], 5), heightMeters=round(c['height'], 5),
                    pitchDegrees=round(math.degrees(c['pitch']), 4), focalPixels=round(c['focal'], 2),
                    axisPixels=[round(c['u0'], 2), round(c['v0'], 2)], fitPixels=round(v['rms'], 2), placement=v['placement'])
        if v['overlaps']:
            view['overlaps'] = [dict(image=name, share=round(share, 4), correlation=round(corr, 3)) for name, share, corr in v['overlaps']]
        views.append(view)
    first = d['views'][0]
    derivation = (
        "Section derived from %s: the silhouette against its plain ground, its edges on the photograph's strongest "
        "steps; the camera (%.2f m off, %.2f m up, pitched %.1f deg, focal %.0f px) fitted so the rim's back edge, the "
        "foot's front edge and their widths fall where the photograph shows them (the section's image strays from the "
        "silhouette by %.1f px rms), which corrects the photograph's perspective; each height's ring turned until its "
        "image meets the silhouette, a ring for each row of the photograph, scaled so the height is the museum's %.1f cm; "
        "the outside kept within %.2f mm of it, so its fillets and its lip keep their shape. Check: %s. Underside and "
        "inside a wall's thickness in from the outside, %s. MuseeVision/Scripts/vessels.py." % (
            first['view']['image'].split('/')[-1], first['cam']['distance'], first['cam']['height'],
            math.degrees(first['cam']['pitch']), first['cam']['focal'], first['rms'], 100 * museum['heightMeters'],
            1000 * max(SECTION_TOLERANCE, SECTION_PIXELS * first['cam']['distance'] / first['cam']['focal']), '; '.join(d['checks']),
            d['wall_how']))
    if vessel['painted']:
        placed = [v for v in d['views'][1:]]
        derivation += (' Decoration: each further view placed by registering its painting, unwrapped round the axis, on '
                       "the others' where they overlap, its camera its own silhouette's (fitPixels), and kept only where "
                       "its painting lies on theirs within %.1f cm in every band of height they share; a view whose "
                       "painting overlaps none of the first view's is placed unverified. The views composed see %.0f%% "
                       "of the outside within %.0f degrees of face on; where none does, each band's own repeat is carried "
                       "round from where it is best seen, and elsewhere the glaze is left." % (
                           100 * AGREEMENT_METERS, 100 * d['coverage'], math.degrees(math.acos(SQUARENESS_CUT))))
        if any(v['placement'] == 'unverified' for v in placed):
            derivation += (' Unverified: ' + '; '.join(
                '%s at %.0f deg' % (v['view']['image'].split('/')[-1], math.degrees(v['cam']['azimuth'])) for v in placed
                if v['placement'] == 'unverified') + ': their painting shares nothing with the first view\'s, so their '
                'turn against it is not in the photographs; they keep the turns their own overlaps give them and stand '
                'as far from every other view as the turn allows.')
        derivation += vessel.get('note', '')
    rec = dict(work=vessel['work'], source=museum['source'], grade='derived', derivation=derivation,
               museum={k: v for k, v in museum.items() if k != 'source'}, section=d['section'],
               outside=[d['first'], d['last']], wallMeters=round(d['wall'], 5))
    if vessel['painted']:
        rec['views'] = views
        rec['coverage'] = round(d['coverage'], 3)
        rec['bands'] = [dict(name=b['name'], fromMeters=round(b['rows'][0] * museum['heightMeters'], 4),
                             toMeters=round(b['rows'][1] * museum['heightMeters'], 4), periodDegrees=p)
                        for b, (p, _) in zip(vessel['bands'], d['periods'])]
        rec['unused'] = d['unused']
    return rec


# ---------------------------------------------------------------------------------------------------------------------
# The Ru basin: an oval with no side view
# ---------------------------------------------------------------------------------------------------------------------
# The National Palace Museum photographs its narcissus basin only from above at about 28 degrees, and its base from
# below: no silhouette shows its section edge on, and its pale glaze on the grey ground defeats a mask. Its section is
# the museum's four sizes joined by the proportions the oblique photograph shows at the basin's left end, read by eye
# on a pixel grid of 汝窯_青瓷無紋水仙盆.png at full size (2829 x 2122): the rim's outer edge at (480, 1035), the wall
# meeting the base's flange at (630, 1390), the flange's face at x 648 from 1400 to 1495, and the nearest foot below it
# from 1490 to 1560. As shares of the 530 px from rim to sole (the photograph's foreshortening falls on all alike): the
# feet 0.13, the flange 0.20, the wall 0.67. The flange's face is the museum's foot (19.3 x 12.9 cm); the wall steps in
# from it by 18 px at the flange's top, 2.2 mm at the rim's scale (about 8100 px a metre: 934 px to the 11.5 cm
# half-axis). The wall's flare, its inset from the rim at shares of its height, is read along the same edge.
# The feet are four cloud-scroll feet, each a long curved foot hugging a corner of the base: on the base photograph
# (北宋汝窯青瓷無紋水仙盆_底部.jpg, 3066 x 2281, taken from under it and all but square to it) the flange's outer edge,
# the museum's 19.3 x 12.9 cm foot, spans columns 800..2390 and rows 620..1700 (centre 1595, 1160). The upper right foot
# is traced by eye on its pixel grid, round from its outer end along the long side, over the corner and down the short
# side, and back along its inner edge (FOOT_OUTLINE); the other three are its mirror images, as the four are in the
# photograph. The base's plan is a rounded oblong, a superellipse of exponent 3 through the traced foot's outer edge
# (FOOT_PLAN), where the basin turned round and pressed to its mouth is an ellipse: each point of the outline keeps its
# share of the way from the centre to the base's edge in its direction, so the feet hug the turned base's edge as they
# hug the real one's.
FOOT_OUTLINE = [(1925.0, 647.5), (2045.0, 650.0), (2100.0, 656.0), (2150.0, 670.0), (2195.0, 695.0), (2230.0, 725.0),
                (2250.0, 760.0), (2270.0, 772.5), (2317.5, 857.5), (2315.0, 870.0), (2260.0, 900.0), (2170.0, 805.0),
                (2130.0, 775.0), (2090.0, 750.0), (2045.0, 737.5), (1925.0, 712.5), (1920.0, 700.0)]
FOOT_FLANGE = dict(columns=(800, 2390), rows=(620, 1700))
FOOT_PLAN = 3.0
RU = dict(
    work='chinese-ru-basin',
    museum=dict(heightMeters=0.069, mouthAxesMeters=[0.230, 0.164], footAxesMeters=[0.193, 0.129],
                source=NPM + ', 故瓷017851, narcissus basin, Ru ware: H. 6.9 cm; mouth 23.0 x 16.4 cm; foot 19.3 x 12.9 cm (the order K9 table)'),
    image='ceramics/chinese-ru-basin/汝窯_青瓷無紋水仙盆.png',
    shares=dict(feet=0.13, flange=0.20),
    step_in=0.0022,
    wall=[(0.0, 0.0), (0.27, 0.0041), (0.49, 0.0081), (0.71, 0.0126), (0.86, 0.0161), (0.95, 0.0186), (0.99, 0.0204)],
    glaze_region=(1100, 1350, 1500, 1500),
)


def ru_feet(pressed_half_axes):
    """The four feet's outlines in the basin's plan (metres, x across, y front to back), from FOOT_OUTLINE: each point's
    share of the way to the real base's edge (a superellipse of FOOT_PLAN) kept on the turned base's (an ellipse of
    these half-axes), and mirrored to the other three corners."""
    (c0, c1), (r0, r1) = FOOT_FLANGE['columns'], FOOT_FLANGE['rows']
    cx, cy, a, b = 0.5 * (c0 + c1), 0.5 * (r0 + r1), 0.5 * (c1 - c0), 0.5 * (r1 - r0)
    one = []
    for x, y in FOOT_OUTLINE:
        X, Y = (x - cx) / a, (cy - y) / b
        share = (abs(X) ** FOOT_PLAN + abs(Y) ** FOOT_PLAN) ** (1.0 / FOOT_PLAN)
        k = share / math.hypot(X, Y)
        one.append((X * k * pressed_half_axes[0], Y * k * pressed_half_axes[1]))
    feet = []
    for sx, sy in ((1, 1), (-1, 1), (-1, -1), (1, -1)):
        outline = [[round(sx * x, 5), round(sy * y, 5)] for x, y in one]
        # every outline anticlockwise in the plan, its solid inside (a mirror image runs the other way round)
        area = sum(a[0] * b[1] - b[0] * a[1] for a, b in zip(outline, outline[1:] + outline[:1]))
        feet.append(dict(outline=outline if area > 0 else outline[::-1]))
    return feet


def derive_ru(ware):
    """The Ru basin's section in the round, its long half-axis the radius (Art presses it to the mouth's short axis),
    its feet, and its glaze's colour from its photograph's lit outside wall, face on."""
    H = RU['museum']['heightMeters']
    a = RU['museum']['mouthAxesMeters'][0] / 2
    foot = RU['museum']['footAxesMeters'][0] / 2
    hf = RU['shares']['feet'] * H
    zf = hf + RU['shares']['flange'] * H
    wall_bottom = foot - RU['step_in']
    outside = [[foot - 0.004, hf], [foot, hf + 0.003], [foot, zf - 0.002], [wall_bottom, zf]]
    for share, inset in RU['wall'][1:]:
        z = zf + share * (H - zf)
        r = wall_bottom + (a - wall_bottom) * inset / RU['wall'][-1][1]
        outside.append([r, z])
    t = min(ware['wallMeters'], ware['wallOfLip'] * a)
    lip = [[a, H - 0.5 * t], [a - 0.5 * t, H], [a - t, H - 0.5 * t]]
    inside = [[r - t, z] for r, z in reversed(outside[4:-1])] + [[wall_bottom - t, zf], [wall_bottom - t, hf + t], [0.0, hf + t]]
    section = [[0.0, hf]] + outside[:-1] + lip + inside
    first, last = 1, len(outside)
    im = load(RU['image'])
    lab = cv2.cvtColor(im, cv2.COLOR_BGR2LAB).astype(np.float32)
    white = np.percentile(lab[..., 0] * 100 / 255, 99.5)
    x0, y0, x1, y1 = RU['glaze_region']
    region = lab[y0:y1, x0:x1].reshape(-1, 3)
    glaze = [round(float(np.median(region[:, 0]) * 100 / 255 * 100 / white), 1), round(float(np.median(region[:, 1]) - 128), 1),
             round(float(np.median(region[:, 2]) - 128), 1)]
    spread = round(float(np.std(np.log(region[:, 0] + 1.0))), 3)
    mouth = RU['museum']['mouthAxesMeters']
    pressed_foot = [2 * foot, 2 * foot * mouth[1] / mouth[0]]
    offset_foot = mouth[1] - (mouth[0] - RU['museum']['footAxesMeters'][0])
    derivation = (
        "Section derived from the museum's sizes and the proportions the oblique photograph %s shows at the basin's "
        "left end, read by eye on its pixel grid (MuseeVision/Scripts/vessels.py, RU): feet %.2f, flange %.2f and wall "
        "%.2f of the height; the flange's face the museum's foot, the wall stepping in %.1f mm above it and flaring to "
        "the mouth; the wall Data/Art/Ware.json's. No photograph shows the section edge on, so no camera is fitted: the "
        "photograph's foreshortening falls on every height alike and cancels in the shares. Turned in the round on the "
        "long half-axis and pressed front to back to the mouth's %.1f x %.1f cm, its foot comes to %.1f x %.1f cm against "
        "the museum's %.1f x %.1f (a wall that flared as far at the ends as at the sides would give %.1f across the short "
        "axis). Four cloud-scroll feet traced from the base photograph %s, each hugging a corner of the base, %.1f mm high. Glaze colour from the lit outside wall, face on (pixels %d..%d x "
        "%d..%d), relative to the photograph's white." % (
            RU['image'].split('/')[-1], RU['shares']['feet'], RU['shares']['flange'],
            1 - RU['shares']['feet'] - RU['shares']['flange'], 1000 * RU['step_in'], 100 * mouth[0], 100 * mouth[1],
            100 * pressed_foot[0], 100 * pressed_foot[1], 100 * RU['museum']['footAxesMeters'][0],
            100 * RU['museum']['footAxesMeters'][1], 100 * offset_foot, '北宋汝窯青瓷無紋水仙盆_底部.jpg', 1000 * hf, x0, x1, y0, y1))
    section = [[round(r, 5), round(z, 5)] for r, z in section]
    faults = section_faults(section, first, last, t)
    if faults:
        raise SystemExit('the Ru basin\'s section is not a solid: ' + '; '.join(faults[:6]))
    rec = dict(work=RU['work'], source=RU['museum']['source'], grade='derived', derivation=derivation,
               museum={k: v for k, v in RU['museum'].items() if k != 'source'},
               section=section, outside=[first, last + 1], wallMeters=round(t, 5),
               feet=dict(heightMeters=round(hf, 5), each=ru_feet((foot, foot * mouth[1] / mouth[0]))))
    return rec, glaze, spread, pressed_foot


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--write', action='store_true')
    ap.add_argument('--sheets')
    ap.add_argument('--only')
    args = ap.parse_args()
    sys.stdout.reconfigure(encoding='utf-8')
    ware = json.load(open(WARE, encoding='utf-8'))
    results = []
    for vessel in VESSELS:
        if args.only and vessel['work'] != args.only:
            continue
        d = derive(vessel, ware, args.sheets)
        results.append((vessel, d))
        print(vessel['work'])
        for v in d['views']:
            c = v['cam']
            print('  %s: camera %.2f m off, %.3f m up, pitch %.1f deg, focal %.0f px, azimuth %.1f deg, silhouette %.2f px, %s%s' % (
                v['view']['image'].split('/')[-1], c['distance'], c['height'], math.degrees(c['pitch']), c['focal'],
                math.degrees(c.get('azimuth', 0.0)), v['rms'], v['placement'],
                ''.join('; on %s %.1f%% shared, correlation %.2f' % (n, 100 * s_, k) for n, s_, k in v['overlaps'])))
        print('  ' + '; '.join(d['checks']) + '; coverage %.2f; %s form; %s; section %d points' % (
            d['coverage'], 'open' if d['open'] else 'closed', d['wall_how'], len(d['section'])))
        for b, (p, c) in zip(vessel['bands'], d.get('periods', [])):
            print('  band %s: repeats every %.2f deg (%d round), self-correlation %.2f' % (b['name'], p, round(360 / p), c))
        if 'glaze' in d:
            print('  glaze CIELAB', d['glaze'])
    ru = None
    if not args.only or args.only == RU['work']:
        ru, glaze, spread, pressed_foot = derive_ru(ware)
        print(RU['work'])
        print('  foot pressed to the mouth: %.1f x %.1f cm (museum 19.3 x 12.9); glaze CIELAB %s, spread %.3f' % (
            100 * pressed_foot[0], 100 * pressed_foot[1], glaze, spread))
    if args.write:
        record = json.load(open(RECORD, encoding='utf-8'))
        records = [record_of(vessel, d) for vessel, d in results] + ([ru] if ru else [])
        for rec in records:
            record['vessels'] = [v for v in record['vessels'] if v['work'] != rec['work']] + [rec]
        with open(RECORD, 'w', encoding='utf-8', newline='\n') as f:
            json.dump(record, f, ensure_ascii=False, indent=2)
            f.write('\n')


if __name__ == '__main__':
    main()
