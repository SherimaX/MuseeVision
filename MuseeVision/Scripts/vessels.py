"""Derives Chenghuai's painted and monochrome vessels from their photographs (order K9).

    python MuseeVision/Scripts/vessels.py [--write] [--sheets <folder>] [--only <work>] [--selftest]

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
         # The neck's clouds are read on DP-32251-001 unwrapped round the jar (half a millimetre a row) between the neck's
         # lower double line (from 43.66 cm) and its upper (to 48.06 cm). Below the neck, the shoulder carries the reign mark
         # once, over the front (`once`): a band reaching down onto it would carry the mark round the jar with its repeat.
         # The lotus panels' tips reach 9.27 cm up, above the band's 7.73: taken to them, the band's measured repeat is
         # trusted, at 15.1 to 15.4 degrees (23.3 to 23.8 panels), which settles neither 23 nor 24.
         bands=[dict(name="the foot's lotus panels", rows=(0.0, 0.16), count=23,
                     read="23 panels: on DP-32251-001 seven panels' tips stand 15.6 degrees apart round the jar"),
                dict(name="the neck's cloud scroll", rows=(0.904, 0.995), count=8,
                     read="8 clouds: on DP-32251-001 three of the neck's clouds stand 44 degrees apart")],
         once=[dict(name='the reign mark', rows=(0.8882, 0.8954), degrees=(-9.0, 9.5),
                    read=("宣德年製 in a line on the shoulder under the neck, read on DP-32251-001 unwrapped: 42.90 to 43.25 cm up, "
                          "from 9.0 degrees left of the front to 9.5 right; the Met's jar carries it once"))]),
    dict(work='chinese-wucai-jar', painted=True,
         museum=dict(heightMeters=0.232, source=MET + ', 17.127.2, Jar with carp in lotus pond: H. 23.2 cm (no diameter published)'),
         # below 0.12 of the height the jar's sides are lost against the grey ground in its own soft shadow: its foot and
         # the collar's lower line are measured on the bottom view instead (ring), and the foot's front edge on the axis
         # is read by eye where its unglazed rim meets the ground (row 3193: the shadow takes the mask 44 px lower)
         views=[dict(image='ceramics/chinese-wucai-jar/DP342697.jpg', profile=True, measure=dict(side='both'), rows=(0.12, 1.0),
                     foot_row=3193, fixed=dict(foot='ring')),
                dict(image='ceramics/chinese-wucai-jar/DP342698.jpg', same_camera=True, measure=dict(side='both')),
                dict(image='ceramics/chinese-wucai-jar/DP342700.jpg', same_camera=True, measure=dict(side='both'))],
         # the bottom, photographed from under the jar along its axis: its cloud collar and the pond above it run all
         # round, between the collar's lower (blue) and upper (red) painted lines. It places the side views round the jar
         # and measures the foot and the collar's lower line, which the side views lose in the shadow.
         ring=dict(image='ceramics/chinese-wucai-jar/DP342699.jpg',
                   lower=dict(hue='blue', rows=(0.02, 0.08), window=(0.56, 0.68), thick=(2, 14)),
                   upper=dict(hue='red', rows=(0.12, 0.20), window=(0.64, 0.80), thick=(5, 30)),
                   foot=dict(hue='unglazed')),
         bands=[dict(name="the foot's cloud collar", rows=(0.0, 0.16),
                     glaze=('13 clouds, which DP342699 shows all round the jar, their colours running yellow, blue, red, '
                            'green four clouds at a time with one red cloud more: no whole number of repeats carries them '
                            'round, so where no side view sees the collar it is left in the glaze')),
                dict(name="the shoulder's lappets", rows=(0.82, 0.92), shortest=10.0),
                dict(name="the neck's scroll", rows=(0.93, 1.0), shortest=3.0)]),
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
         bands=[dict(name="the rim's double line", rows=(0.76, 0.92), plain=True), dict(name="the foot's double line", rows=(0.0, 0.10), plain=True)]),
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
         # side B's panels, to settle their count with side A's: its two whole panels' emblems' rings (the small circles
         # under them) read by eye on its pixel grid, on row 1250 at columns 450 and 655, and turned into bearings through
         # the cameras its silhouette allows, fitted with its EXIF focal length to the section's upper and lower sides
         side_b=dict(image='ceramics/chinese-meiping/WLA_vanda_Vase_6.jpg', focal=2022.0, axis_rows=(108, 1415),
                     band="the base's lotus panels", row=1250, columns=(450, 655), sides=((0.15, 0.92), (0.05, 0.5))),
         bands=[dict(name="the base's lotus panels", rows=(0.0, 0.28), count=7,
                     read=("7 panels: on side A its three panels' emblems stand 49 and 51.5 degrees apart round the vase and "
                           "its front panel's dividers 55 (7 panels are 51.4 degrees each, 6 would be 60)")),
                dict(name="the lower diaper band", rows=(0.28, 0.35)),
                dict(name="the shoulder's peony scroll", rows=(0.78, 0.92), shortest=15.0)]),
    dict(work='chinese-peachbloom', painted=False,
         museum=dict(heightMeters=0.197, source=MET + ', 14.40.377, Vase with peach-bloom glaze: H. 19.7 cm (no diameter published)'),
         # the two fillets at the neck's foot, the neck and the everted lip, crisp against the dark ground
         views=[dict(image='ceramics/chinese-peachbloom/DP246426.jpg', profile=True, fine=[(0.70, 1.0)])],
         # thin-potted: the white lip at the rim's left-hand turn, between the ground and the dark inside
         lip=dict(rows=(713, 716), columns=(1280, 1299)),
         bands=[]),
]

# The view registration and the unwrapped pictures (Data/Art/Vessels.json's `registration`, which Art's check of each
# view's placement measures by too): the grid round the axis and up the outside.
_RECORD = json.load(open(RECORD, encoding='utf-8'))
REGISTRATION = _RECORD['registration']
COMPOSITION = _RECORD['composition']
AZIMUTH_STEPS = REGISTRATION['azimuthSteps']
HEIGHT_STEPS = REGISTRATION['heightSteps']
# A band's repeat, a glaze's colour and a side view's collar on the bottom view's ring are measured where a view sees
# the surface at least this squarely (the cosine of the angle between the surface's normal and the line of sight):
# beyond 60 degrees a photograph's pixel covers twice the surface it does face on, too coarse to measure by. (Art
# composes from every view wherever it sees the surface: Data/Art/Vessels.json's `composition`.)
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


def section_through(m, cam, H, rl, rf, n, iters, profile_rows=(0.0, 1.0), lip_ring=False, fine=(), through=()):
    """The outside's radius at n heights from the foot to the lip: each height's ring turned until its leftmost (or
    rightmost) image point meets the silhouette's edge. Heights outside profile_rows (fractions of the height whose edge
    is clean) are bridged by a cubic from the last clean ring to the fitted foot or rim, matching its slope, or where
    other photographs measure rings there (`through`, [(height, radius)] below the clean heights, the foot's among them),
    by a monotone cubic through them and the clean rings. The foot's
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
        r = bridge(zs, r, lo * H, 0.0, rf, below=True, through=through)
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


def bridge(zs, r, z_clean, z_end, r_end, below, through=()):
    """A cubic from the clean ring at z_clean (its value and slope, fitted by a line to the clean rings over the next
    fiftieth of the height, on the clean side alone) to r_end at z_end, reaching it square (the slope a foot or lip
    ring's own edge has); or, given rings measured beyond the clean ones (through), a monotone cubic through them and the
    clean rings over the next fiftieth of the height."""
    i = int(np.searchsorted(zs, z_clean))
    i = min(max(i, 2), len(zs) - 3)
    reach = max(3, int(0.02 * len(zs)))
    side = slice(i, min(len(zs), i + reach)) if below else slice(max(0, i - reach + 1), i + 1)
    if through:
        from scipy.interpolate import PchipInterpolator
        known = sorted(through)
        pz = np.array([z for z, _ in known] + list(zs[side]) if below else list(zs[side]) + [z for z, _ in known])
        pr = np.array([x for _, x in known] + list(r[side]) if below else list(r[side]) + [x for _, x in known])
        out = r.copy()
        sel = zs <= zs[i] if below else zs >= zs[i]
        out[sel] = PchipInterpolator(pz, pr)(zs[sel])
        return out
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
    """The decoration's own detail for registration: lightness band-passed (the record's fineCells to coarseCells), so
    shading, exposure and a photograph's grain don't count."""
    g = cv2.cvtColor(pic, cv2.COLOR_BGR2LAB)[..., 0].astype(np.float32)
    return cv2.GaussianBlur(g, (0, 0), REGISTRATION['fineCells']) - cv2.GaussianBlur(g, (0, 0), REGISTRATION['coarseCells'])


# Registration looks as far round as its detail still lines up with a neighbour's: to 78 degrees off face on, where a
# view's picture is stretched fivefold (the record's registration squarenessCut).
REGISTRATION_CUT = REGISTRATION['squarenessCut']


def weights(square, cut=None):
    cut = REGISTRATION_CUT if cut is None else cut
    return np.clip((square - cut) / (1 - cut), 0, 1)


def correlation(da, wa, db, wb):
    w = wa * wb
    tot = w.sum()
    if tot < REGISTRATION['leastShare'] * w.size:
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


# Where two placed views overlap, the composition blends them (Data/Art/Vessels.json's composition blendMeters) only
# where their paintings lie on each other within AGREEMENT_METERS; further apart, blended, the painting shows twice (the
# wucai jar's carp with two heads), so each view keeps its own side of the overlap, meeting the other at a seam. How far
# apart they lie is measured as the composition weighs them (each by the square of how squarely it sees the surface):
# blocks of OFFSET_BLOCK cells over the overlap, each laid on the other view by the rise and turn (within OFFSET_REACH
# cells) at which its detail correlates best; a block whose painting lies on the other's at OFFSET_TRUST or better (the
# same stretch of painting, not a repeat or the plain glaze) gives its offset, and the overlap's is their median.
OFFSET_BLOCK = 24
OFFSET_REACH = 10
OFFSET_TRUST = 0.8
OFFSET_LEAST = 0.01


def weighted_correlation(da, wa, db, wb):
    """The correlation of two pictures' detail, each weighted, where both are (-1 where they share no weight)."""
    w = wa * wb
    tot = w.sum()
    if tot <= 1e-12:
        return -1.0
    mx, my = (w * da).sum() / tot, (w * db).sum() / tot
    return float((w * (da - mx) * (db - my)).sum() / math.sqrt((w * (da - mx) ** 2).sum() * (w * (db - my) ** 2).sum() + 1e-9))


def offset_of(a, b, zs, r):
    """How far apart two placed views' paintings lie where both lend them (metres): the median of their trusted blocks'
    offsets, and how many blocks were trusted; None where no block is (too slight an overlap, or plain glaze)."""
    da, db = detail(a[0]), detail(b[0])
    wa, wb = a[1] ** 2, b[1] ** 2
    ring_r = np.interp((np.arange(HEIGHT_STEPS) + 0.5) / HEIGHT_STEPS * zs[-1], zs, r)
    cell_z, cell_t = zs[-1] / HEIGHT_STEPS, 2 * np.pi / AZIMUTH_STEPS
    found = []
    for r0 in range(OFFSET_REACH, HEIGHT_STEPS - OFFSET_BLOCK - OFFSET_REACH, OFFSET_BLOCK // 2):
        rows = slice(r0, r0 + OFFSET_BLOCK)
        for c0 in range(0, AZIMUTH_STEPS, OFFSET_BLOCK // 2):
            cols = np.arange(c0, c0 + OFFSET_BLOCK) % AZIMUTH_STEPS
            A, WA = da[rows][:, cols], wa[rows][:, cols]
            if (WA * wb[rows][:, cols]).mean() < OFFSET_LEAST:
                continue
            best = (-2.0, 0, 0)
            for dv in range(-OFFSET_REACH, OFFSET_REACH + 1):
                moved = slice(r0 + dv, r0 + dv + OFFSET_BLOCK)
                for dc in range(-OFFSET_REACH, OFFSET_REACH + 1):
                    cc = (cols + dc) % AZIMUTH_STEPS
                    c = weighted_correlation(A, WA, db[moved][:, cc], wb[moved][:, cc])
                    if c > best[0]:
                        best = (c, dv, dc)
            if best[0] >= OFFSET_TRUST:
                found.append(math.hypot(best[1] * cell_z, best[2] * cell_t * ring_r[r0 + OFFSET_BLOCK // 2]))
    return (float(np.median(found)) if found else None), len(found)


def describe(bands):
    return ', '.join('%.1f-%.1f H %+.1f cm (correlation %.2f)' % (b['rows'][0], b['rows'][1], 100 * b['rise'], b['correlation'])
                     for b in bands if b['correlation'] >= GENUINE)


# Two views of the same stretch of painting correlate at 0.5 or better once registered (the Xuande jar's pair, 0.76);
# a repeat of the design elsewhere round the vessel, fish for fish, at 0.4 or less. A turn is scored by how far each
# overlap's correlation stands above the line between them (the record's registration genuine), weighted by the overlap.
GENUINE = REGISTRATION['genuine']
MIN_OVERLAP = 0.005


def landscape(a, b, cut=None):
    """For each turn of b against a (in cells, two at a time): their correlation and their overlap, each view weighted
    from the cut (the registration's, unless given)."""
    da, db, wa, wb = detail(a[0]), detail(b[0]), weights(a[1], cut), weights(b[1], cut)
    C = np.full(AZIMUTH_STEPS, -1.0)
    O = np.zeros(AZIMUTH_STEPS)
    for shift in range(0, AZIMUTH_STEPS, 2):
        rb, rw = np.roll(db, shift, axis=1), np.roll(wb, shift, axis=1)
        O[shift] = float((wa * rw).mean())
        if O[shift] >= MIN_OVERLAP:
            C[shift] = correlation(da, wa, rb, rw)
    return C, O


# A placed view's overlaps with the others are recorded as the composition sees them: wherever both see the surface at
# all (the record's registration overlapCut), so a view along the axis, which sees the vessel obliquely, has its own.
OVERLAP_CUT = REGISTRATION['overlapCut']


def overlap_of(a, b):
    """Two placed views' overlap as recorded: their share of the outside and their correlation there (-1 where too
    slight to correlate), at no further turn."""
    C, O = landscape(a, b, OVERLAP_CUT)
    return float(O[0]), float(C[0])


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
        return [(cv, 'registered', [(0,) + overlap_of(base, unwrap(mv, cv, zs, r))])]
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
    placed = [base] + [unwrap(mv, cv, zs, r) for _, mv, cv in fitted]
    for k, (v, mv, cv) in enumerate(fitted):
        seen = [(i,) + overlap_of(placed[i], placed[k + 1]) for i in range(n) if i != k + 1]
        overlaps = [o for o in seen if o[1] >= MIN_OVERLAP and o[2] > -1.0]
        out.append((cv, 'registered' if root(k + 1) == root(0) else 'unverified', overlaps))
    return out


# ---------------------------------------------------------------------------------------------------------------------
# A view along the axis: a vessel's bottom, photographed from under it
# ---------------------------------------------------------------------------------------------------------------------
# A painted line by its hue in CIELAB (a*, b* about 0; L* 0..255 as OpenCV gives it): red enamel, cobalt blue, and the
# foot's unglazed body (its iron-brown rim).
LINE_HUES = {
    'red': lambda L, a, b: (a > 8) & (b < 1.3 * a) & (b > -0.5 * a) & (L < 190),
    'blue': lambda L, a, b: (b < -6) & (np.abs(a) < 12) & (L < 200),
    'unglazed': lambda L, a, b: (b > 12) & (a > 3) & (b > 1.3 * a),
}
RING_ANGLES = 720
# The ground's cast shadow: the ground darkened with its hue unchanged (within this of its a* and b*).
RING_SHADOW_CHROMA = 4.0
# The vessel's texture against the smooth ground: its lightness's spread (L*, 0..255) over a square of so many pixels.
RING_TEXTURE = 4.0
RING_TEXTURE_PX = 9
# A painted line's circle is searched for within RING_SEARCH px of the middle, to RING_STEP px, and its ring fitted to
# the strokes within RING_BAND px of it.
RING_SEARCH = 80
RING_STEP = 4
RING_BAND = 15
RING_SECTORS = 180


def ring_terms(t):
    return np.stack([np.ones_like(t), np.cos(t), np.sin(t), np.cos(2 * t), np.sin(2 * t)], 1)


def robust_ring(t, rho):
    """A ring's image about a centre, its radius at each angle as a mean and first and second harmonics (a ring off the
    centre, and slightly oval), fitted to candidate points: those more than 2.5 times the fit's spread off it (another
    line's stroke, a cloud's outline) are dropped, again and again, the first passes wide. Returns the harmonics, the
    spread (pixels) and the share of the candidates kept."""
    M = ring_terms(t)
    keep = np.ones(len(t), bool)
    for step in range(30):
        coef = np.linalg.lstsq(M[keep], rho[keep], rcond=None)[0]
        off = rho - M @ coef
        spread = max(3.0, 1.4826 * float(np.median(np.abs(off[keep])))) if step > 2 else 0.05 * float(np.median(rho))
        keep = np.abs(off) < 2.5 * spread
    return coef, spread, float(keep.mean())


def polar(img, centre, radii):
    t = (np.arange(RING_ANGLES) + 0.5) / RING_ANGLES * 2 * np.pi
    X = centre[0] + radii[:, None] * np.cos(t)[None]
    Y = centre[1] + radii[:, None] * np.sin(t)[None]
    return t, cv2.remap(img, X.astype(np.float32), Y.astype(np.float32), cv2.INTER_LINEAR)


def runs_of(on):
    """The runs of True along a column: [(start, end)]."""
    d = np.diff(np.concatenate([[0], on.astype(int), [0]]))
    return list(zip(np.where(d == 1)[0], np.where(d == -1)[0]))


def read_ring(spec):
    """The bottom view's rings, in its pixels about the silhouette's centre: the silhouette (the widest ring), the
    collar's lower and upper painted lines (each angle's runs of the line's hue, of the line's thickness, within its
    window of the silhouette's radius), and the foot's outer edge (the outermost edge of the unglazed rim inside the lower
    line). Each is a robust_ring."""
    im = load(spec['image'])
    # the middle: the deepest point of the vessel against its ground, its cast shadow (the ground darkened, its hue
    # unchanged) taken for ground
    small, fg, s = mask_of(im, 1400, 14.0, True, (RING_SHADOW_CHROMA, 0.0))
    depth = cv2.distanceTransform((fg > 0).astype(np.uint8), cv2.DIST_L2, 5)
    centre = np.array(np.unravel_index(np.argmax(depth), depth.shape)[::-1], float) / s
    lab = cv2.cvtColor(im, cv2.COLOR_BGR2LAB).astype(np.float32)
    # the silhouette: along each ray from the middle, the outer end of the painted and glazed vessel's last run of
    # texture (its lightness varying by more than RING_TEXTURE over RING_TEXTURE_PX), where the plain ground and the
    # shadow on it are smooth
    L = lab[..., 0]
    k = (RING_TEXTURE_PX, RING_TEXTURE_PX)
    texture = np.sqrt(np.maximum(cv2.blur(L * L, k) - cv2.blur(L, k) ** 2, 0))
    rs = np.arange(0.0, 0.6 * max(im.shape[:2]), 1.0)
    t, tex = polar(texture, centre, rs)
    tt, outer = [], []
    for a in range(RING_ANGLES):
        runs = [(p, q) for p, q in runs_of(tex[:, a] > RING_TEXTURE) if q - p >= 2 * RING_TEXTURE_PX]
        if runs:
            tt.append(t[a])
            outer.append(rs[runs[-1][1] - 1])
    rings = dict(centre=centre, silhouette=robust_ring(np.array(tt), np.array(outer)))
    R = rings['silhouette'][0][0]
    for name in ('lower', 'upper'):
        line = spec[name]
        radii = np.arange(line['window'][0] * R, line['window'][1] * R)
        t, pl = polar(lab, centre, radii)
        on = LINE_HUES[line['hue']](pl[..., 0], pl[..., 1] - 128, pl[..., 2] - 128)
        tt, rr = [], []
        for k in range(RING_ANGLES):
            for a, b in runs_of(on[:, k]):
                if line['thick'][0] <= b - a <= line['thick'][1]:
                    tt.append(t[k])
                    rr.append(radii[(a + b) // 2])
        tt, rr = np.array(tt), np.array(rr)
        # a line runs all the way round, where a cloud's or a flower's strokes in its hue come and go: the circle that
        # strokes cover the most of (the most of RING_SECTORS sectors round it; its centre within RING_SEARCH px of the
        # middle, to RING_STEP px) is the line, and its ring is fitted to the strokes within RING_BAND px of that circle
        px, py = rr * np.cos(tt), rr * np.sin(tt)
        best = (-1, 0.0, 0.0, 0.0)
        for dx in np.arange(-RING_SEARCH, RING_SEARCH + 1, RING_STEP):
            for dy in np.arange(-RING_SEARCH, RING_SEARCH + 1, RING_STEP):
                d = np.hypot(px - dx, py - dy)
                sector = ((np.arctan2(py - dy, px - dx) + np.pi) / (2 * np.pi) * RING_SECTORS).astype(int) % RING_SECTORS
                cells = np.unique((d / RING_STEP).astype(int) * RING_SECTORS + sector)
                cover = np.bincount(cells // RING_SECTORS)
                if cover.max() > best[0]:
                    best = (cover.max(), dx, dy, (np.argmax(cover) + 0.5) * RING_STEP)
        _, dx, dy, radius = best
        near = np.abs(np.hypot(px - dx, py - dy) - radius) < RING_BAND
        rings[name] = robust_ring(tt[near], rr[near])
    lower = ring_terms(t) @ rings['lower'][0]
    radii = np.arange(0.4 * R, lower.max())
    t, pl = polar(lab, centre, radii)
    on = LINE_HUES[spec['foot']['hue']](pl[..., 0], pl[..., 1] - 128, pl[..., 2] - 128)
    tt, rr = [], []
    for k in range(RING_ANGLES):
        ends = [radii[min(b, len(radii) - 1)] for a, b in runs_of(on[:, k]) if b - a >= 3]
        ends = [e for e in ends if e < lower[k] - 3]
        if ends:
            tt.append(t[k])
            rr.append(max(ends))
    rings['foot'] = robust_ring(np.array(tt), np.array(rr))
    rings['im'] = im
    return rings


def line_row(pic, square, hue, rows):
    """The row of an unwrapped side view (up the height) on which a painted line runs: of rows within `rows` (fractions of
    the height), the one on which most of what the view sees squarely is the line's hue."""
    lab = cv2.cvtColor(pic, cv2.COLOR_BGR2LAB).astype(np.float32)
    on = LINE_HUES[hue](lab[..., 0], lab[..., 1] - 128, lab[..., 2] - 128)
    w = square > 0.7
    share = (on & w).sum(1) / np.maximum(w.sum(1), 1)
    lo, hi = int(rows[0] * HEIGHT_STEPS), int(rows[1] * HEIGHT_STEPS)
    return lo + int(np.argmax(share[lo:hi])) + 0.5


def ring_camera(rings, zs, r, z_upper):
    """The bottom view's camera, a pinhole on the axis under the foot (its rings are round to a few per cent and centred
    within a few pixels of each other: the view is along the axis): its distance below the collar's upper line and its
    focal length, from the upper line's radius at its height on the section and the silhouette's, the ring whose image is
    widest."""
    from scipy.optimize import brentq
    rho_upper, rho_sil = rings['upper'][0][0], rings['silhouette'][0][0]
    r_upper = float(np.interp(z_upper, zs, r))

    def widest(D):
        f = rho_upper * D / r_upper
        return float(np.max(f * r / (D + zs - z_upper))) - rho_sil
    D = brentq(widest, 0.3, 50.0)
    return D, rho_upper * D / r_upper


def ring_radius(rho, z, D, f, z_upper):
    """The radius (metres) of a ring whose image about the axis is rho pixels, at height z, through the bottom camera."""
    return rho * (D + z - z_upper) / f


# The rows the collar (between its lines) and the pond above it are registered on: the collar in RING_COLLAR_ROWS shares
# of the way between its lines (which each view finds on its own rows), the pond a cell of height at a time over
# RING_POND metres.
RING_COLLAR_ROWS = 28
RING_POND = 0.05


def ring_grid(rings, zs, r, H, D, f, z_upper):
    """The bottom view laid out as the side views are: columns round the axis (its own angle, from the image's +x
    towards +y), rows the collar's shares of the way from its lower line to its upper, then the pond a cell at a time
    above the upper line, each ring's image scaled from the upper line's by the camera."""
    t = (np.arange(AZIMUTH_STEPS) + 0.5) / AZIMUTH_STEPS * 2 * np.pi
    M = ring_terms(t)
    lower, upper = M @ rings['lower'][0], M @ rings['upper'][0]
    u = (np.arange(RING_COLLAR_ROWS) + 0.5) / RING_COLLAR_ROWS
    zp = z_upper + (np.arange(int(round(RING_POND / (H / HEIGHT_STEPS)))) + 0.5) * H / HEIGHT_STEPS
    r_upper = np.interp(z_upper, zs, r)
    scale = (np.interp(zp, zs, r) / (D + zp - z_upper)) / (r_upper / D)
    rho = np.vstack([lower[None] + u[:, None] * (upper - lower)[None], upper[None] * scale[:, None]])
    c = rings['centre']
    X, Y = c[0] + rho * np.cos(t)[None], c[1] + rho * np.sin(t)[None]
    return cv2.remap(rings['im'], X.astype(np.float32), Y.astype(np.float32), cv2.INTER_LINEAR), len(zp)


def side_grid(pic, square, lower_row, upper_row, pond_rows):
    """A side view's unwrapped picture on the ring grid's rows: the collar between its own lines, then the pond."""
    rows = np.concatenate([lower_row + (np.arange(RING_COLLAR_ROWS) + 0.5) / RING_COLLAR_ROWS * (upper_row - lower_row),
                           upper_row + 0.5 + np.arange(pond_rows)]) - 0.5
    mx = np.tile(np.arange(AZIMUTH_STEPS, dtype=np.float32), (len(rows), 1))
    my = np.repeat(rows[:, None].astype(np.float32), AZIMUTH_STEPS, 1)
    return cv2.remap(pic, mx, my, cv2.INTER_LINEAR), cv2.remap(square.astype(np.float32), mx, my, cv2.INTER_LINEAR)


# Registration on the ring is by the painting's colours, not its lightness: the bottom view is lit and exposed unlike the
# side views. Each cell is a class (red, yellow, green or blue enamel, where its chroma is at least RING_CHROMA of the
# strongest within RING_NEAR cells round, or a dark stroke, darker than RING_DARK of the white there), softened over
# RING_SOFT cells.
RING_CHROMA = 0.3
RING_DARK = 0.6
RING_SOFT = 2.0
RING_NEAR = 60


def colour_classes(pic, valid=None):
    from scipy.ndimage import uniform_filter1d
    lab = cv2.cvtColor(pic, cv2.COLOR_BGR2LAB).astype(np.float32)
    L, a, b = lab[..., 0], lab[..., 1] - 128, lab[..., 2] - 128
    C, h = np.hypot(a, b), np.degrees(np.arctan2(b, a)) % 360
    Cm, Lm = C.copy(), L.copy()
    if valid is not None:
        Cm[~valid], Lm[~valid] = np.nan, np.nan
    with np.errstate(all='ignore'):
        import warnings
        with warnings.catch_warnings():
            warnings.simplefilter('ignore')
            c95 = np.nan_to_num(np.nanpercentile(Cm, 95, axis=0), nan=1.0)
            w95 = np.nan_to_num(np.nanpercentile(Lm, 95, axis=0), nan=200.0)
    c95 = uniform_filter1d(c95, RING_NEAR, mode='wrap')
    w95 = uniform_filter1d(w95, RING_NEAR, mode='wrap')
    chroma = C / np.maximum(c95, 5)[None] > RING_CHROMA
    classes = [chroma & ((h < 55) | (h >= 345)), chroma & (h >= 55) & (h < 105), chroma & (h >= 105) & (h < 200),
               chroma & (h >= 200) & (h < 345), ~chroma & (L / np.maximum(w95, 50)[None] < RING_DARK)]
    return [cv2.GaussianBlur(k.astype(np.float32), (0, 0), RING_SOFT) for k in classes]


def ring_landscape(grid, pic, square):
    """How a side view's collar and pond lie on the bottom view's at each turn of the ring (cells): the mean correlation of
    their colour classes where the side view sees squarely."""
    seen = square > SQUARENESS_CUT
    ks, kb = colour_classes(pic, seen), colour_classes(grid)
    w = seen.astype(np.float32)
    one = np.ones_like(w)
    return np.array([np.mean([correlation(a, w, np.roll(b, k, 1), one) for a, b in zip(ks, kb)]) for k in range(AZIMUTH_STEPS)])


def ring_peak(L):
    """The ring's angle (radians) under a view's front, from its landscape: the best turn, refined to the middle of the
    cells round it within 5 per cent of it (a broad peak's middle); and its correlation and the next best peak's, further
    than 10 degrees from it."""
    n = AZIMUTH_STEPS
    k = int(np.argmax(L))
    top = L[k]
    lo = hi = k
    while L[(lo - 1) % n] >= 0.95 * top and hi - lo < n // 8:
        lo -= 1
    while L[(hi + 1) % n] >= 0.95 * top and hi - lo < n // 8:
        hi += 1
    cells = np.arange(lo, hi + 1)
    mid = float((cells * L[cells % n]).sum() / L[cells % n].sum())
    peaks = [i for i in range(n) if L[i] >= L[i - 1] and L[i] >= L[(i + 1) % n] and min(abs(i - k), n - abs(i - k)) > n // 36]
    runner = max((L[i] for i in peaks), default=float('nan'))
    # a turn of k cells lays the view's column c on the ring's c - k: the ring's angle at the view's front is -k
    return (-mid / n * 2 * np.pi) % (2 * np.pi), float(top), float(runner)


# A band's repeat is measured on its painting's colour, lightly smoothed (BAND_SMOOTH cells) with the view's shading
# taken off (its trend over BAND_SHADING cells), both along the band: a plain line running round leaves nothing. A turn
# is trusted where the band is seen at both ends of it over at least BAND_OVERLAP of what is seen at all, its painting
# then lies on itself at a correlation of BAND_TRUST or more, and the correlation peaks there (not at the search's
# bounds). Of peaks within BAND_HARMONIC of the best, the shortest is the repeat (twice the repeat matches as well).
BAND_SMOOTH = 1.0
BAND_SHADING = 40.0
BAND_OVERLAP = 0.2
BAND_TRUST = 0.5
BAND_HARMONIC = 0.05


def along(x, w, sigma):
    """x smoothed along its rows (round the vessel, wrapping) where w sees it, weighted by w."""
    pad = int(4 * sigma) + 1
    xp = np.concatenate([x[:, -pad:], x, x[:, :pad]], 1)
    wp = np.concatenate([w[:, -pad:], w, w[:, :pad]], 1)
    k = (2 * pad + 1, 1)
    bx = cv2.GaussianBlur((xp * wp).astype(np.float32), k, sigma)[:, pad:-pad]
    bw = cv2.GaussianBlur(wp.astype(np.float32), k, sigma)[:, pad:-pad]
    return bx / np.maximum(bw, 1e-6)


def band_repeat(unwrapped, H, rows, shortest=6.0, longest=180.0):
    """A band's repeat round the vessel as the placed views show it: the turn (degrees, to a tenth of a cell) at which its
    painting best lies on itself, and that correlation; None for the turn where no turn is trusted (BAND_TRUST)."""
    z = (np.arange(HEIGHT_STEPS) + 0.5) / HEIGHT_STEPS * H
    band = (z >= rows[0] * H) & (z <= rows[1] * H)
    colour = [np.zeros((band.sum(), AZIMUTH_STEPS)) for _ in range(3)]
    seen = np.zeros((band.sum(), AZIMUTH_STEPS))
    for pic, square in unwrapped:
        lab = cv2.cvtColor(pic, cv2.COLOR_BGR2LAB).astype(np.float32)[band]
        w = np.clip((square[band] - SQUARENESS_CUT) / (1 - SQUARENESS_CUT), 0, 1)
        for k in range(3):
            colour[k] += lab[..., k] * w
        seen += w
    detail = []
    for c in colour:
        c = c / np.maximum(seen, 1e-9)
        detail.append(along(c, seen, BAND_SMOOTH) - along(c, seen, BAND_SHADING))
    whole = (seen * seen).sum()
    C = np.full(AZIMUTH_STEPS // 2 + 1, -2.0)
    for lag in range(1, len(C)):
        w = seen * np.roll(seen, lag, 1)
        if w.sum() < BAND_OVERLAP * whole:
            continue
        num = da = db = 0.0
        for d in detail:
            a, b = d, np.roll(d, lag, 1)
            ma, mb = (w * a).sum() / w.sum(), (w * b).sum() / w.sum()
            num += (w * (a - ma) * (b - mb)).sum()
            da += (w * (a - ma) ** 2).sum()
            db += (w * (b - mb) ** 2).sum()
        C[lag] = num / math.sqrt(da * db + 1e-12)
    lo = max(1, int(math.ceil(shortest / 360 * AZIMUTH_STEPS)))
    hi = min(len(C) - 1, int(longest / 360 * AZIMUTH_STEPS))
    peaks = [k for k in range(lo + 1, hi) if C[k] > -2 and C[k - 1] > -2 and C[k + 1] > -2 and C[k] > C[k - 1] and C[k] >= C[k + 1]]
    if not peaks:
        return None, float('nan')
    top = max(C[k] for k in peaks)
    k = min(k for k in peaks if C[k] >= top - BAND_HARMONIC)
    # the peak's turn between cells, from the parabola through it and its neighbours
    a, b, c = C[k - 1], C[k], C[k + 1]
    den = a - 2 * b + c
    turn = (k + (0.5 * (a - c) / den if den != 0 else 0.0)) * 360.0 / AZIMUTH_STEPS
    return (turn if b >= BAND_TRUST else None), float(b)


# A count is settled where the measured repeat, if trusted, gives the same whole number of repeats a turn to within this
# share of one; a measured repeat alone is trusted as a count where it comes so near a whole number. Halfway between two
# counts, the photographs choose neither.
BAND_WHOLE = 0.25


def band_of(band, unwrapped, H, also=None):
    """A band's repeat and how it was found: a count read by eye on the photographs where its measured repeat, if
    trusted, gives the same count (within BAND_WHOLE of a repeat), a plain line running round unbroken (carried round as
    itself, PLAIN_PERIOD at a time), or the measured repeat where it is trusted and near a whole count; otherwise None,
    with why, and the band is left in the glaze where no view sees it. `also`: further evidence on the count, said
    beside it."""
    turn, corr = band_repeat(unwrapped, H, band['rows'], band.get('shortest', 6.0))
    measured = ('measured repeat %.1f deg (self-correlation %.2f)' % (turn, corr) if turn is not None else
                'no measured repeat trusted (best self-correlation %.2f, under %.2f)' % (corr, BAND_TRUST))
    extra = '; ' + also if also else ''
    left = ': where no view sees it, it is left in the glaze'
    if band.get('count'):
        if turn is not None and abs(360.0 / turn - band['count']) > BAND_WHOLE:
            return None, ('not settled: read by eye, %s; but the %s gives %.2f repeats a turn, %.2f of a repeat off %d%s%s'
                          % (band['read'], measured, 360.0 / turn, abs(360.0 / turn - band['count']), band['count'], extra, left))
        return 360.0 / band['count'], 'read by eye: %s; %s%s' % (band['read'], measured, extra)
    if band.get('plain'):
        return PLAIN_PERIOD, 'a plain line running unbroken round, carried round as itself'
    if band.get('glaze'):
        return None, band['glaze']
    if turn is None:
        return None, measured + extra + left
    n = max(1, int(round(360.0 / turn)))
    if abs(360.0 / turn - n) > BAND_WHOLE:
        return None, '%s, %.2f repeats a turn, no whole count%s%s' % (measured, 360.0 / turn, extra, left)
    return 360.0 / n, '%s, %d round%s' % (measured, n, extra)


# A plain line's repeat: any turn carries it, and the finer the turn the more of what the views see of it is averaged.
PLAIN_PERIOD = 2.0


def sees(square, cam, fit, zs, r):
    """Where a view lends its colour as Art composes (Data/Art/Vessels.json's composition): wherever it sees the surface,
    obliquely too, the place inside the silhouette by its camera's miss and edgePixels (a place that faces the camera
    by the cosine s lies about s^2 / 2 of its ring's radius inside it). Art also leaves out what another part of the
    vessel hides, which none of these vessels' views has."""
    z = (np.arange(HEIGHT_STEPS) + 0.5) / HEIGHT_STEPS * zs[-1]
    rr = np.interp(z, zs, r)
    eye = np.array([cam['distance'] * math.sin(cam.get('azimuth', 0.0)), cam['distance'] * math.cos(cam.get('azimuth', 0.0)), cam['height']])
    far = np.linalg.norm(eye - np.array([0.0, 0.0, 0.5 * zs[-1]]))
    inside = 0.5 * square ** 2 * (rr * cam['focal'] / far)[:, None]
    return (square > 0) & (inside >= fit + COMPOSITION['edgePixels'])


def coverage(views, sights):
    """The share of the outside some view lends its colour to (sees)."""
    seen = np.zeros((HEIGHT_STEPS, AZIMUTH_STEPS), bool)
    for (pic, square), (cam, fit, zs, r) in zip(views, sights):
        seen |= sees(square, cam, fit, zs, r)
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
def fit_view(v, H, museum, zs=None, r=None, ring_foot=None):
    m = measure(v['image'], **v.get('measure', {}))
    if 'foot_row' in v:
        # the foot's front edge on the axis, read from the photograph by eye where its own shadow takes the mask lower
        m.update(bot_u=np.array([m['uc'] - 1.0, m['uc'], m['uc'] + 1.0]), bot_v=np.full(3, float(v['foot_row'])))
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
    if fixed.get('foot') == 'ring' and ring_foot is not None:
        # the foot's radius as a view along the axis measures it
        kw['foot'] = ring_foot
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


def lower_from_ring(spec, rings, m, cam, zs, r, H):
    """What the bottom view measures below a side view's clean heights: the collar's lower and upper lines' heights (found
    on the first view, unwrapped), the bottom camera (fitted to the upper line and the silhouette on the section), and
    through it the radii of the lower line's ring and the foot's."""
    pic, sq = unwrap(m, cam, zs, r)
    z_lower = line_row(pic, sq, spec['lower']['hue'], spec['lower']['rows']) / HEIGHT_STEPS * H
    z_upper = line_row(pic, sq, spec['upper']['hue'], spec['upper']['rows']) / HEIGHT_STEPS * H
    D, f = ring_camera(rings, zs, r, z_upper)
    return dict(z_lower=z_lower, z_upper=z_upper, D=D, f=f, lower=ring_radius(rings['lower'][0][0], z_lower, D, f, z_upper),
                foot=ring_radius(rings['foot'][0][0], 0.0, D, f, z_upper))


def bottom_camera(rings, low, first_turn):
    """The bottom view's camera in the side views' terms: the pinhole on the axis that measured the foot (low['D'] under
    the collar's upper line, focal low['f']), pitched up 90 degrees, its optical axis the rings' centre (a pixel's centre
    at +0.5), and turned so the ring's angle first_turn (image x towards image y) lies under the first view's front. Such
    a camera sees the point at bearing phi at the ring's angle phi - azimuth - 90 deg, so its bearing is -first_turn -
    90 deg."""
    c = rings['centre']
    return dict(azimuth=(-first_turn - 0.5 * np.pi) % (2 * np.pi), distance=0.0, height=low['z_upper'] - low['D'], pitch=-0.5 * np.pi,
                focal=low['f'], u0=float(c[0]) + 0.5, v0=float(c[1]) + 0.5)


def bottom_fit(rings):
    """How far the bottom view's painted rings stray from their fitted ellipses (px, the largest spread): its camera's
    miss, as a side view's silhouette's is."""
    return max(rings[name][1] for name in ('lower', 'upper', 'foot'))


def place_on_ring(spec, rings, low, first_pic, fitted, zs, r, H):
    """Each view's bearing from the bottom view, which sees the collar and the pond all round: every side view's collar
    (between its own lines) and pond registered on the ring by their colours (ring_landscape), its bearing the ring's
    angle under its front less the first view's. Returns the views' placements as place() does (their overlaps' indices
    counting the bottom view last), each view's registration (its ring angle, correlation, and the next best peak's), and
    the bottom view placed (its camera, its fit, its picture and its overlaps)."""
    grid, pond = ring_grid(rings, zs, r, H, low['D'], low['f'], low['z_upper'])
    pics = [first_pic] + [unwrap(mv, dict(cv, azimuth=0.0), zs, r) for _, mv, cv in fitted]
    reg = []
    for pic, sq in pics:
        lower = line_row(pic, sq, spec['lower']['hue'], spec['lower']['rows'])
        upper = line_row(pic, sq, spec['upper']['hue'], spec['upper']['rows'])
        sp, ss = side_grid(pic, sq, lower, upper, pond)
        reg.append(ring_peak(ring_landscape(grid, sp, ss)))
    turns = [(reg[k + 1][0] - reg[0][0]) % (2 * np.pi) for k in range(len(fitted))]
    cam_b = bottom_camera(rings, low, reg[0][0])
    pic_b = unwrap(dict(im=rings['im']), cam_b, zs, r)
    allp = [first_pic] + [unwrap(mv, dict(cv, azimuth=turns[k]), zs, r) for k, (_, mv, cv) in enumerate(fitted)] + [pic_b]
    out = []
    for k, (v, mv, cv) in enumerate(fitted):
        cv['azimuth'] = turns[k]
        overlaps = []
        for i in range(len(allp)):
            if i == k + 1:
                continue
            # the pictures are placed already: they lie on each other at no further turn (an overlap too slight to
            # correlate, under correlation()'s share, is not listed)
            share, corr = overlap_of(allp[i], allp[k + 1])
            if share >= MIN_OVERLAP and corr > -1.0:
                overlaps.append((i, share, corr))
        out.append((cv, 'registered', overlaps))
    b = len(allp) - 1
    bottom_overlaps = []
    for i in range(b):
        share, corr = overlap_of(allp[i], pic_b)
        if share >= MIN_OVERLAP and corr > -1.0:
            bottom_overlaps.append((i, share, corr))
    bottom = dict(cam=cam_b, fit=bottom_fit(rings), pic=pic_b, overlaps=bottom_overlaps, turn=reg[0][0])
    return out, reg, bottom


def bearing_at(cam, zs, r, u, v, reach=0.4):
    """The bearing (degrees from the camera's front towards the right) of the outside's point a photograph shows at the
    pixel (u, v) (centres at +0.5), on the near side, within `reach` of the height: the height whose ring's image passes
    nearest it, and that ring's angle there."""
    best = None
    t = np.radians(np.linspace(-90.0, 90.0, 3601))
    for z in np.linspace(0.0, reach * zs[-1], 801):
        rr = float(np.interp(z, zs, r))
        P = np.stack([rr * np.sin(t), rr * np.cos(t), np.full_like(t, z)], -1)
        pu, pv, _ = project(P, dict(cam, azimuth=0.0))
        i = int(np.argmin(np.abs(pu - u)))
        if abs(pu[i] - u) < 1.0 and (best is None or abs(pv[i] - v) < best[0]):
            best = (abs(pv[i] - v), math.degrees(t[i]))
    return best[1] if best else float('nan')


def side_b_counts(spec, H, museum, zs, r):
    """What a photograph whose camera can't be fixed says of a band's count: for each camera its silhouette allows (fitted
    with its known focal length to the section's sides over each span of height), its two whole panels' turn apart and
    that camera's miss. Returns {band name: the evidence, said}."""
    turns = []
    for sides in spec['sides']:
        v = dict(image=spec['image'], fixed=dict(focal=spec['focal']), axis_rows=spec['axis_rows'], sides=sides)
        m, cam, _, _, rms = fit_view(v, H, museum, zs, r)
        a, b = (bearing_at(cam, zs, r, c + 0.5, spec['row'] + 0.5) for c in spec['columns'])
        turns.append((abs(b - a), rms))
    said = ("side B (%s), whose camera can't be fixed, gives no count: its two whole panels' emblems stand %s apart through "
            "the cameras its silhouette allows (%s px rms)" % (
                spec['image'].split('/')[-1], ' and '.join('%.0f deg' % t for t, _ in turns), ' and '.join('%.0f' % e for _, e in turns)))
    return {spec['band']: said}


def widest_of(section):
    """A stored section's widest diameter: twice its widest point's radius, over every point (as Art's SizeFault and
    Musee.Art.PaintedVessel measure it)."""
    return 2 * max(p[0] for p in section)


def departure_of(first, H, museum, widest, rms):
    """Where the section's widest diameter departs from the museum's by more than DIAMETER_TOLERANCE of it: what a camera
    held to the museum's diameter makes of the photograph (the widest it reaches and how far its section's image strays
    from the silhouette), so the record says why the photograph's proportion is kept."""
    published = museum.get('diameterMeters')
    if not published or abs(widest - published) <= DIAMETER_TOLERANCE * published:
        return None
    held = dict(first, fixed=dict(first.get('fixed', {}), max='diameter'))
    rows = first.get('rows', (0.0, 1.0))
    m, cam, rl, rf, _ = fit_view(held, H, museum)
    zs, r = section_through(m, cam, H, rl, rf, pixel_rows(m), 12, rows, fine=first.get('fine', ()))
    miss = silhouette_rms(m, cam, H, zs, r, rows[0] == 0.0, rows)
    return ("its photograph gives %.1f cm across at the museum's %.1f cm height, %.1f%% %s the museum's %.1f cm; held to "
            "the museum's diameter, the camera's best comes to %.1f cm across and its section's image misses the silhouette "
            "by %.1f px rms (against %.1f): no camera puts the museum's diameter on the photograph, so the section keeps the "
            "photograph's proportion" % (100 * widest, 100 * H, 100 * abs(widest - published) / published,
                                         'under' if widest < published else 'over', 100 * published, 200 * r.max(), miss, rms))


# How far a derived section's widest diameter may stand from its museum's before its record must say why (a departure):
# the Data/Art/Vessels.json record's diameterTolerance, which make checks too.
DIAMETER_TOLERANCE = json.load(open(RECORD, encoding='utf-8'))['diameterTolerance']


def derive(vessel, ware, sheets):
    H = vessel['museum']['heightMeters']
    museum = vessel['museum']
    views = vessel['views']
    first = next(v for v in views if v.get('profile'))
    rows = first.get('rows', (0.0, 1.0))
    m, cam, rl, rf, rms = fit_view(first, H, museum)
    lip_ring = first.get('fixed', {}).get('lip') == 'diameter'
    zs, r = section_through(m, cam, H, rl, rf, pixel_rows(m), 12, rows, lip_ring=lip_ring, fine=first.get('fine', ()))
    out = dict(work=vessel['work'], views=[], checks=[], unused=list(vessel.get('unused', [])))
    spec = vessel.get('ring')
    if spec:
        # the bottom view measures the foot and the collar's lower line, which the side views lose: the camera is held to
        # that foot and the section runs through both, and again with the section they give
        rings = read_ring(spec)
        for _ in range(2):
            low = lower_from_ring(spec, rings, m, cam, zs, r, H)
            m, cam, rl, rf, rms = fit_view(first, H, museum, ring_foot=low['foot'])
            zs, r = section_through(m, cam, H, rl, rf, pixel_rows(m), 12, rows, lip_ring=lip_ring, fine=first.get('fine', ()),
                                    through=[(0.0, low['foot']), (low['z_lower'], low['lower'])])
        low = lower_from_ring(spec, rings, m, cam, zs, r, H)
        out['ring'] = dict(low, rings=rings)
    out['zs'], out['r'] = zs, r
    if not lip_ring:
        out['checks'].append("the rim's outer radius %.1f mm from the silhouette (the rim's fitted ellipse %.1f)" % (1000 * r[-1], 1000 * rl))
    # the foot's edge counts where the view's silhouette is clean down to it
    out['views'].append(dict(view=first, m=m, cam=cam, rms=silhouette_rms(m, cam, H, zs, r, rows[0] == 0.0, rows), placement='fitted', overlaps=[]))
    if sheets:
        sheet(sheets, vessel['work'] + '-0.jpg', m, cam, zs, r)
    unwrapped = [unwrap(m, cam, zs, r)]
    fitted = []
    m0 = measure(first['image'], **first.get('measure', {}))
    for v in (vv for vv in views if vv is not first):
        if v.get('same_camera'):
            # a series taken on a turntable: the first view's camera, the vessel set down where this photograph shows it
            # (its axis's column, and its foot and rim as far up or down the frame as its mask shows them against the
            # first view's)
            mv = measure(v['image'], **v.get('measure', {}))
            cv = dict(cam, u0=mv['uc'], v0=cam['v0'] + 0.5 * (mv['top'] + mv['bot'] - m0['top'] - m0['bot']))
        else:
            mv, cv, _, _, _ = fit_view(v, H, museum, zs, r)
        fitted.append([v, mv, cv])
    bottom = None
    if spec and fitted:
        placed, out['ring_reg'], bottom = place_on_ring(spec, rings, low, unwrapped[0], fitted, zs, r, H)
    else:
        placed = place(unwrapped[0], fitted, zs, r) if fitted else []
    pics = [unwrap(mv, cv, zs, r) for (_, mv, _), (cv, _, _) in zip(fitted, placed)]
    allp = unwrapped + pics
    names = [first['image'].split('/')[-1]] + [v['image'].split('/')[-1] for v, _, _ in fitted]
    sights = [(cam, out['views'][0]['rms'], zs, r)]
    if bottom:
        # the bottom view is composed as the side views are, where its painting lies on theirs band by band
        allp.append(bottom['pic'])
        names.append(spec['image'].split('/')[-1])
        wrong = [(i, bands) for i, bands in ((i, agreement(allp[i], bottom['pic'], H)) for i, _, _ in bottom['overlaps']) if disagreement(bands)]
        if wrong:
            why = "placed by its rings, its painting lies off %s band by band up the height (%s; over %.1f cm)" % (
                ', '.join(names[i] for i, _ in wrong), '; '.join(describe(bands) for _, bands in wrong), 100 * AGREEMENT_METERS)
            out['unused'].append(dict(image='assets/' + spec['image'], why=why + ': composed, it would double the painting'))
            print('  %s left out: %s' % (names[-1], why))
            b = len(allp) - 1
            placed = [(cv, placement, [o for o in overlaps if o[0] != b]) for cv, placement, overlaps in placed]
            bottom = None
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
                    names[i], describe(bands), 100 * AGREEMENT_METERS))
            out['unused'].append(dict(image='assets/' + v['image'], why='; '.join(why) + ': composed, it would double the painting'))
            print('  %s left out: %s' % (name, '; '.join(why)))
            continue
        out['views'].append(dict(view=v, m=mv, cam=cv, rms=fit, placement=placement,
                                 overlaps=[(names[i], share, corr, offset_of(allp[i], allp[k + 1], zs, r)[0]) for i, share, corr in overlaps]))
        unwrapped.append(allp[k + 1])
        sights.append((cv, fit, zs, r))
        if sheets:
            sheet(sheets, vessel['work'] + '-%d.jpg' % (k + 1), mv, cv, zs, r)
    if bottom:
        out['views'].append(dict(view=dict(image=spec['image']), m=None, cam=bottom['cam'], rms=bottom['fit'], placement='registered',
                                 overlaps=[(names[i], share, corr, offset_of(allp[i], bottom['pic'], zs, r)[0])
                                           for i, share, corr in bottom['overlaps']]))
        out['bottom'] = bottom
        unwrapped.append(bottom['pic'])
        sights.append((bottom['cam'], bottom['fit'], zs, r))
    out['coverage'] = coverage(unwrapped, sights)
    also = {}
    if vessel.get('side_b'):
        also = side_b_counts(vessel['side_b'], H, museum, zs, r)
    for b in vessel['bands']:
        for m in vessel.get('once', []):
            if b['rows'][0] < m['rows'][1] and m['rows'][0] < b['rows'][1]:
                raise SystemExit("%s: %s (%.4f to %.4f m) reaches over %s (%.4f to %.4f m), which the vessel carries once: its "
                                 "repeat would carry it round" % (vessel['work'], b['name'], b['rows'][0] * H, b['rows'][1] * H,
                                                                  m['name'], m['rows'][0] * H, m['rows'][1] * H))
    out['bands'] = [band_of(b, unwrapped, H, also.get(b['name'])) for b in vessel['bands']]
    out['unwrapped'] = unwrapped
    if not vessel['painted']:
        out['glaze'] = glaze_colour(m, cam, zs, r)
    open_form = r[-1] >= 0.95 * r.max()
    t, how = wall_of(vessel, cam, ware, r[-1])
    tolerance = max(SECTION_TOLERANCE, SECTION_PIXELS * cam['distance'] / cam['focal'])
    out['section'], out['first'], out['last'] = closed_section(zs, r, t, ware, open_form, tolerance)
    # the checks are of the section as it is stored, as Art measures it: its widest point anywhere (a flared lip's turn
    # too), and its foot's outer edge (the outside's first point)
    outside = out['section'][out['first']:out['last']]
    widest = widest_of(out['section'])
    out['widest'] = widest
    checks = ['widest diameter %.2f cm (museum %s)' % (100 * widest, '%.1f cm' % (100 * museum['diameterMeters']) if 'diameterMeters' in museum else 'none published')]
    if 'footMeters' in museum:
        checks.append('foot %.2f cm (museum %.1f cm)' % (200 * outside[0][0], 100 * museum['footMeters']))
    out['checks'] = checks + out['checks']
    out['departure'] = departure_of(first, H, museum, widest, out['views'][0]['rms'])
    out['open'] = open_form
    out['wall'], out['wall_how'] = t, how
    return out


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
            view['overlaps'] = []
            for name, share, corr, offset in v['overlaps']:
                o = dict(image=name, share=round(share, 4), correlation=round(corr, 3))
                if offset is not None:
                    o['offsetMeters'] = round(offset, 4)
                view['overlaps'].append(o)
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
    if d.get('departure'):
        derivation += ' Departure: %s.' % d['departure']
    ring = d.get('ring')
    if ring:
        rr = ring['rings']
        derivation += (
            " Below %.0f%% of the height the side view's edge is lost in the jar's own shadow: there the section runs through "
            "the rings the bottom view %s measures, a pinhole on the axis %.3f m under the collar's upper line with a focal "
            "length of %.0f px (fitted to that line's ring, %.1f px about the axis, at its height %.1f mm on the section, "
            "and to the silhouette's, %.1f px): the collar's lower line at %.1f mm up, %.1f mm from the axis (its ring %.1f "
            "px, spread %.1f), and the foot's outer edge %.1f mm (its unglazed rim's ring %.1f px, spread %.1f); the camera "
            "is held to that foot, and the foot's front edge on the axis read by eye (row %d), where the shadow takes the "
            "mask lower." % (
                100 * first['view'].get('rows', (0.0, 1.0))[0], vessel['ring']['image'].split('/')[-1], ring['D'], ring['f'],
                rr['upper'][0][0], 1000 * ring['z_upper'], rr['silhouette'][0][0], 1000 * ring['z_lower'], 1000 * ring['lower'],
                rr['lower'][0][0], rr['lower'][1], 1000 * ring['foot'], rr['foot'][0][0], rr['foot'][1],
                first['view'].get('foot_row', 0)))
    if vessel['painted']:
        placed = [v for v in d['views'][1:]]
        if ring:
            reg = d['ring_reg']
            derivation += (
                ' Decoration: each view placed round the jar by the bottom view, which sees the collar and the pond above it '
                'all round: each side view\'s collar (between its own lines) and pond laid on the ring by their colours '
                '(red, yellow, green and blue enamel and dark strokes), its bearing the ring\'s angle under its front less the '
                'first view\'s (' + '; '.join(
                    '%s under %.1f deg of the ring, correlation %.2f against %.2f at the next best turn' % (
                        v['view']['image'].split('/')[-1], math.degrees(g[0]), g[1], g[2]) for v, g in zip(d['views'], reg)) +
                '); its camera its own silhouette\'s (fitPixels), and kept only where its painting lies on the views it '
                'overlaps within %.1f cm in every band of height they share.' % (100 * AGREEMENT_METERS))
            if d.get('bottom'):
                g = d['bottom']
                derivation += (
                    ' The bottom view is composed too, as it sees the collar and the pond obliquely from under the jar: its '
                    'camera the pinhole on the axis that measures the foot (pitched up 90 deg), turned so the first view\'s '
                    'front lies under %.1f deg of its ring (bearing %.1f deg), its fit its painted rings\' largest spread '
                    '(%.1f px).' % (math.degrees(g['turn']), math.degrees(g['cam']['azimuth']), g['fit']))
        else:
            derivation += (' Decoration: each further view placed by registering its painting, unwrapped round the axis, on '
                           "the others' where they overlap, its camera its own silhouette's (fitPixels), and kept only where "
                           "its painting lies on theirs within %.1f cm in every band of height they share; a view whose "
                           "painting overlaps none of the first view's is placed unverified." % (100 * AGREEMENT_METERS))
        derivation += (" The views composed see %.0f%% of the outside (all they see of it, obliquely too, inside their "
                       "silhouettes by their cameras' misses and %.0f px); where none does, a band whose repeat is known is "
                       "carried round from where it is seen (its `reading`), and elsewhere the glaze is left." % (
                           100 * d['coverage'], COMPOSITION['edgePixels']))
        seamed = [(v['view']['image'].split('/')[-1], name, offset) for v in d['views'] for name, _, _, offset in v['overlaps']
                  if offset is not None and offset > COMPOSITION['blendMeters']]
        blended = [(v['view']['image'].split('/')[-1], name, offset) for v in d['views'] for name, _, _, offset in v['overlaps']
                   if offset is not None and offset <= COMPOSITION['blendMeters']]
        if seamed or blended:
            derivation += (" Where two views overlap, their paintings' offset is measured as the composition weighs them "
                           '(the median of the blocks that lie on the other view at a correlation of %.1f or better): %s. %s' % (
                               OFFSET_TRUST, '; '.join('%s on %s %.1f mm' % (a, b, 1000 * o) for a, b, o in seamed + blended),
                               ('Further apart than %.1f cm, blended they would show the painting twice, so each keeps its own side '
                                'of the overlap and they meet at a seam run where their paintings agree best, cross-faded over %.0f '
                                'mm.' % (100 * COMPOSITION['blendMeters'], 1000 * COMPOSITION['seamMeters'])) if seamed else
                               'Within %.1f cm, they are blended.' % (100 * COMPOSITION['blendMeters'])))
        left = [(b['name'], how) for b, (p, how) in zip(vessel['bands'], d['bands']) if p is None]
        if left:
            derivation += ' Left in the glaze where no view sees them: ' + '; '.join('%s (%s)' % (n, how) for n, how in left) + '.'
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
    if d.get('departure'):
        rec['departure'] = d['departure']
    if vessel['painted']:
        rec['views'] = views
        rec['coverage'] = round(d['coverage'], 3)
        rec['bands'] = [dict(name=b['name'], fromMeters=round(b['rows'][0] * museum['heightMeters'], 4),
                             toMeters=round(b['rows'][1] * museum['heightMeters'], 4), periodDegrees=p, reading=how)
                        for b, (p, how) in zip(vessel['bands'], d['bands']) if p is not None]
        rec['unused'] = list(d['unused'])
        if vessel.get('once'):
            rec['once'] = [dict(name=m['name'], fromMeters=round(m['rows'][0] * museum['heightMeters'], 4),
                                toMeters=round(m['rows'][1] * museum['heightMeters'], 4), fromDegrees=m['degrees'][0],
                                toDegrees=m['degrees'][1], reading=m['read']) for m in vessel['once']]
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
# The feet are four cloud-scroll feet, each a long curved foot hugging a corner of the base. The upper right foot is
# traced by eye on the pixel grid of the base photograph (北宋汝窯青瓷無紋水仙盆_底部.jpg, 3066 x 2281, taken from under the
# basin and all but square to it), round from its outer end along the long side, over the corner and down the short side,
# and back along its inner edge (FOOT_OUTLINE); the other three are its mirror images, as the four are in the photograph.
#
# The basin's plan is read on its photographs, not assumed (RU_PLAN). Its base: the flange's outer edge, where the pale
# flange meets the darker wall seen beyond it, traced along rays from the base's middle (inward from the wall's band, to
# where the lightness first rises PLAN_RISE over the wall's), and a superellipse |x/a|^n + |y/b|^n = 1 fitted to it,
# robustly: the rays where the feet hide the edge are left out as they stray. Its mouth: the white lip, traced on the
# oblique photograph as the brightest thin line near a rough oval, and a superellipse of the museum's mouth, 23.0 x 16.4
# cm, fitted to it through a pinhole (its distance, elevation, focal length, optical axis, roll and yaw fitted too). Each
# outline is kept in shares of its half-axes (the lip's traced points carried back through the fitted camera onto the
# lip's plane), for Art's check of the turned mesh against the photographs.
FOOT_OUTLINE = [(1925.0, 647.5), (2045.0, 650.0), (2100.0, 656.0), (2150.0, 670.0), (2195.0, 695.0), (2230.0, 725.0),
                (2250.0, 760.0), (2270.0, 772.5), (2317.5, 857.5), (2315.0, 870.0), (2260.0, 900.0), (2170.0, 805.0),
                (2130.0, 775.0), (2090.0, 750.0), (2045.0, 737.5), (1925.0, 712.5), (1920.0, 700.0)]
RU_PLAN = dict(
    base='ceramics/chinese-ru-basin/北宋汝窯青瓷無紋水仙盆_底部.jpg',
    # the base's middle and half-axes, rough, read by eye: where the rays start
    base_rough=(1575.0, 1160.0, 830.0, 555.0),
    lip='ceramics/chinese-ru-basin/汝窯_青瓷無紋水仙盆.png',
    # the lip's rough oval on the oblique photograph, read by eye: its middle and half-axes
    lip_rough=(1415.0, 1026.0, 928.0, 320.0),
)
# A ray's wall is its lightness's median over the first 8 px inside the wall's band (from PLAN_WALL_SHARE beyond the
# rough oval); the flange's edge, where the lightness first rises PLAN_RISE above it, at the half-way level.
PLAN_WALL_SHARE = 1.07
PLAN_RISE = 12.0
# The lip is the brightest thin line (a 15 px top-hat of the lightness, at least LIP_LEAST) within LIP_REACH of the rough oval.
LIP_LEAST = 6.0
LIP_REACH = 0.1
# A fit leaves out the traced points that stray from it more than PLAN_OUTLIER times the median stray (and 2 px), again
# and again (PLAN_ROUNDS): the rays the feet hide, a speck of glaze.
PLAN_OUTLIER = 3.0
PLAN_ROUNDS = 5
# Each outline is kept at so many points, evenly round.
PLAN_POINTS = 72


def superellipse_share(X, Y, n):
    """How far a point lies to a superellipse of exponent n and half-axes 1 along its ray from the middle (1 on it)."""
    return (np.abs(X) ** n + np.abs(Y) ** n) ** (1.0 / n)


def fit_superellipse(x, y, q0):
    """A superellipse's middle, half-axes and exponent fitted to traced points (pixels, y down): each point's stray along
    its ray from the middle, robustly; again leaving out the points that stray (PLAN_OUTLIER). Returns (cx, cy, a, b, n),
    the kept points' rms stray and which were kept."""
    keep = np.ones(len(x), bool)
    q = np.array(q0, float)

    def stray(q, xs, ys):
        cx, cy, a, b, n = q
        X, Y = (xs - cx) / a, (ys - cy) / b
        return (1 - 1 / superellipse_share(X, Y, n)) * np.hypot(xs - cx, ys - cy)
    for _ in range(PLAN_ROUNDS):
        q = least_squares(stray, q, args=(x[keep], y[keep]), loss='soft_l1', f_scale=2.0).x
        d = np.abs(stray(q, x, y))
        keep = d < PLAN_OUTLIER * np.median(d) + 2.0
    return q, float(np.sqrt(np.mean(stray(q, x[keep], y[keep]) ** 2))), keep


def even_outline(X, Y):
    """Points [X, Y] (shares of the half-axes) thinned to PLAN_POINTS, evenly by their angle round the middle."""
    t = np.arctan2(Y, X) % (2 * np.pi)
    out = []
    for k in range(PLAN_POINTS):
        lo, hi = 2 * np.pi * k / PLAN_POINTS, 2 * np.pi * (k + 1) / PLAN_POINTS
        sel = np.where((t >= lo) & (t < hi))[0]
        if len(sel):
            i = sel[np.argmin(np.abs(t[sel] - 0.5 * (lo + hi)))]
            out.append([round(float(X[i]), 4), round(float(Y[i]), 4)])
    return out


def base_plan():
    """The base's plan from the base photograph: the flange's outer edge traced and fitted (fit_superellipse). Returns the
    fit (cx, cy, a, b, n in its pixels), its rms stray, how many rays were kept of how many, and its outline in shares."""
    im = load(RU_PLAN['base'])
    L = cv2.GaussianBlur(cv2.cvtColor(im, cv2.COLOR_BGR2LAB)[..., 0].astype(np.float32), (0, 0), 2.0)
    cx, cy, a, b = RU_PLAN['base_rough']
    pts = []
    step = 0.5
    for k in range(720):
        t = math.radians((k + 0.5) / 2)
        c, s = math.cos(t), math.sin(t)
        r_out = PLAN_WALL_SHARE / math.hypot(c / a, s / b)
        rs = np.arange(r_out, 0.6 * r_out, -step)
        xs, ys = cx + rs * c, cy - rs * s
        vals = cv2.remap(L, xs.astype(np.float32)[None], ys.astype(np.float32)[None], cv2.INTER_LINEAR)[0]
        wall = float(np.median(vals[:int(8 / step)]))
        above = np.where(vals > wall + PLAN_RISE)[0]
        if not len(above):
            continue
        i = int(above[0])
        inner = vals[min(len(vals) - 1, i + int(6 / step))]
        mid = 0.5 * (wall + max(inner, wall + PLAN_RISE))
        j = i
        while j > 0 and vals[j - 1] > mid:
            j -= 1
        while j < len(vals) - 1 and vals[j] < mid:
            j += 1
        pts.append((xs[j], ys[j]))
    pts = np.array(pts)
    q, rms, keep = fit_superellipse(pts[:, 0], pts[:, 1], (cx, cy, a, b, 2.5))
    X, Y = (pts[keep, 0] - q[0]) / q[2], (q[1] - pts[keep, 1]) / q[3]
    return q, rms, int(keep.sum()), len(pts), even_outline(X, Y)


def lip_camera_curve(q, A, B, t):
    """The lip's image through the oblique photograph's pinhole: a superellipse of half-axes A, B and exponent n on the
    lip's plane, yawed, seen from D off at elevation e (in front of it, looking at its middle), focal f, optical axis
    (u0, v0), rolled."""
    D, e, f, u0, v0, roll, yaw, n = q
    s = superellipse_share(np.cos(t), np.sin(t), n) ** -1
    X, Y = A * np.cos(t) * s, B * np.sin(t) * s
    X, Y = X * math.cos(yaw) - Y * math.sin(yaw), X * math.sin(yaw) + Y * math.cos(yaw)
    ce, se = math.cos(e), math.sin(e)
    C = np.array([0.0, -D * ce, D * se])
    fwd, right = np.array([0.0, ce, -se]), np.array([1.0, 0.0, 0.0])
    up = np.cross(right, fwd)
    P = np.stack([X, Y, np.zeros_like(X)], 1) - C
    u, v = f * (P @ right) / (P @ fwd), -f * (P @ up) / (P @ fwd)
    cr, sr = math.cos(roll), math.sin(roll)
    return np.stack([u0 + u * cr - v * sr, v0 + u * sr + v * cr], 1)


def lip_to_plan(q, px):
    """Pixels of the oblique photograph carried back through its fitted pinhole onto the lip's plane, un-yawed: metres."""
    D, e, f, u0, v0, roll, yaw, n = q
    cr, sr = math.cos(roll), math.sin(roll)
    du, dv = px[:, 0] - u0, px[:, 1] - v0
    u, v = du * cr + dv * sr, -du * sr + dv * cr
    ce, se = math.cos(e), math.sin(e)
    C = np.array([0.0, -D * ce, D * se])
    fwd, right = np.array([0.0, ce, -se]), np.array([1.0, 0.0, 0.0])
    up = np.cross(right, fwd)
    d = right[None] * (u / f)[:, None] - up[None] * (v / f)[:, None] + fwd[None]
    t = -C[2] / d[:, 2]
    P = C[None] + d * t[:, None]
    return P[:, 0] * math.cos(-yaw) - P[:, 1] * math.sin(-yaw), P[:, 0] * math.sin(-yaw) + P[:, 1] * math.cos(-yaw)


def mouth_plan(A, B):
    """The mouth's plan from the oblique photograph: the white lip traced and a superellipse of the museum's mouth fitted
    to it through a pinhole. Returns the camera and exponent fitted, its rms stray (px), the points kept of those traced,
    its outline in shares of the half-axes, and the points kept (pixels)."""
    from scipy.spatial import cKDTree
    im = load(RU_PLAN['lip'])
    L = cv2.cvtColor(im, cv2.COLOR_BGR2LAB)[..., 0]
    hat = cv2.GaussianBlur(cv2.morphologyEx(L, cv2.MORPH_TOPHAT, np.ones((15, 15), np.uint8)).astype(np.float32), (0, 0), 1.5)
    cx, cy, a, b = RU_PLAN['lip_rough']
    pts = []
    for k in range(720):
        t = math.radians((k + 0.5) / 2)
        c, s = math.cos(t), math.sin(t)
        r0 = 1 / math.hypot(c / a, s / b)
        rs = np.arange((1 - LIP_REACH) * r0, (1 + LIP_REACH) * r0, 0.5)
        xs, ys = cx + rs * c, cy - rs * s
        vals = cv2.remap(hat, xs.astype(np.float32)[None], ys.astype(np.float32)[None], cv2.INTER_LINEAR)[0]
        i = int(np.argmax(vals))
        if vals[i] > LIP_LEAST:
            pts.append((xs[i], ys[i]))
    pts = np.array(pts)
    t = np.linspace(0, 2 * np.pi, 4000, endpoint=False)

    def stray(q, P):
        return cKDTree(lip_camera_curve(q, A, B, t)).query(P)[0]
    # a first guess: 1.5 m off, 28 degrees up (the museum's view), the focal length that gives the rough oval's width
    q = np.array([1.5, math.radians(28.0), 1.5 * a / A, cx, cy, 0.0, 0.0, 2.5])
    keep = np.ones(len(pts), bool)
    for _ in range(PLAN_ROUNDS):
        q = least_squares(stray, q, args=(pts[keep],), loss='soft_l1', f_scale=3.0,
                          x_scale=[0.5, 0.1, 1000.0, 50.0, 50.0, 0.05, 0.05, 0.5]).x
        d = stray(q, pts)
        keep = d < max(6.0, PLAN_OUTLIER * np.median(d))
    rms = float(np.sqrt(np.mean(stray(q, pts[keep]) ** 2)))
    X, Y = lip_to_plan(q, pts[keep])
    return q, rms, int(keep.sum()), len(pts), even_outline(X / A, Y / B), pts[keep]


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


# The underside meets the flange's face over a rounded arris this wide: the base photograph shows the pale underside
# running out to the flange's edge, where the feet stand.
RU_ARRIS = 0.001


def ru_feet(half_axes, base, underside):
    """The four feet's outlines in the basin's plan (metres, x across, y front to back), from FOOT_OUTLINE: each point in
    shares of the base photograph's fitted flange (base: its middle, half-axes and exponent in its pixels), laid on the
    turned base's half-axes, which Art presses to the same superellipse; a point the photograph shows beyond the
    underside's edge (its share, `underside`, of the flange's) is brought in to it along its ray, as a foot stands under
    the base; and mirrored to the other three corners. Returns the feet and the furthest share the tracing reached."""
    cx, cy, a, b, n = base
    one = []
    furthest = 0.0
    for x, y in FOOT_OUTLINE:
        X, Y = (x - cx) / a, (cy - y) / b
        share = float(superellipse_share(X, Y, n))
        furthest = max(furthest, share)
        k = min(1.0, underside / share)
        one.append((X * k * half_axes[0], Y * k * half_axes[1]))
    feet = []
    for sx, sy in ((1, 1), (-1, 1), (-1, -1), (1, -1)):
        outline = [[round(sx * x, 5), round(sy * y, 5)] for x, y in one]
        # every outline anticlockwise in the plan, its solid inside (a mirror image runs the other way round)
        area = sum(a[0] * b[1] - b[0] * a[1] for a, b in zip(outline, outline[1:] + outline[:1]))
        feet.append(dict(outline=outline if area > 0 else outline[::-1]))
    return feet, furthest


def lip_misfit(q, pts, mouth, n):
    """How far the lip's kept points (pixels) stray (rms, px) from the best superellipse of exponent n through the
    oblique photograph's pinhole, its camera refitted: what another plan would make of the photograph."""
    from scipy.spatial import cKDTree
    A, B = mouth[0] / 2, mouth[1] / 2
    t = np.linspace(0, 2 * np.pi, 4000, endpoint=False)
    fixed = least_squares(lambda qq: cKDTree(lip_camera_curve(list(qq) + [n], A, B, t)).query(pts)[0], q[:7], loss='soft_l1',
                          f_scale=3.0, x_scale=[0.5, 0.1, 1000.0, 50.0, 50.0, 0.05, 0.05])
    return float(np.sqrt(np.mean(cKDTree(lip_camera_curve(list(fixed.x) + [n], A, B, t)).query(pts)[0] ** 2)))


def derive_ru(ware):
    """The Ru basin's section in the round, its long half-axis the radius (Art presses it front to back to the foot's
    oval at the flange's face and the mouth's at the lip, evenly between: Vessel/VesselSection.h, PressOf), its feet,
    and its glaze's colour from its photograph's lit outside wall, face on."""
    H = RU['museum']['heightMeters']
    a = RU['museum']['mouthAxesMeters'][0] / 2
    foot = RU['museum']['footAxesMeters'][0] / 2
    hf = RU['shares']['feet'] * H
    zf = hf + RU['shares']['flange'] * H
    wall_bottom = foot - RU['step_in']
    outside = [[foot - RU_ARRIS, hf], [foot, hf + RU_ARRIS], [foot, zf - 0.002], [wall_bottom, zf]]
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
    foot_axes = RU['museum']['footAxesMeters']
    # a pressed round of the mouth's ratio all the way down, and a wall flaring as far at the ends as at the sides: the
    # two readings the museum's foot lies between
    uniform_foot = 2 * foot * mouth[1] / mouth[0]
    offset_foot = mouth[1] - (mouth[0] - foot_axes[0])
    pressed_foot = [2 * foot, 2 * foot * foot_axes[1] / foot_axes[0]]
    base, base_rms, base_kept, base_rays, base_outline = base_plan()
    lip_q, lip_rms, lip_kept, lip_traced, lip_outline, lip_points = mouth_plan(mouth[0] / 2, mouth[1] / 2)
    feet, furthest = ru_feet((foot, 0.5 * foot_axes[1]), base, (foot - RU_ARRIS) / foot)
    derivation = (
        "Section derived from the museum's sizes and the proportions the oblique photograph %s shows at the basin's "
        "left end, read by eye on its pixel grid (MuseeVision/Scripts/vessels.py, RU): feet %.2f, flange %.2f and wall "
        "%.2f of the height; the flange's face the museum's foot, the wall stepping in %.1f mm above it and flaring to "
        "the mouth; the wall Data/Art/Ware.json's. No photograph shows the section edge on, so no camera is fitted: the "
        "photograph's foreshortening falls on every height alike and cancels in the shares. Turned in the round on the "
        "long half-axis and pressed front to back, as a potter presses a thrown round to an oval: at the lip to the "
        "mouth's %.1f x %.1f cm and at the flange's face to the foot's %.1f x %.1f, the pressing running evenly up the "
        "wall between them (pressed to the mouth's ratio all the way down, the foot would be %.1f across its short axis; a "
        "wall that flared as far at the ends as at the sides would give %.1f; the museum's %.1f lies between, and no "
        "photograph shows how the wall's flare changes round the oval). Its plan is a rounded oblong, as its photographs "
        "show it: a superellipse |x/a|^n + |y/b|^n = 1, pressed so at every height. At the flange's face n = %.2f, fitted "
        "to the flange's outer edge on the base photograph %s (%d of %d rays; the rest are where the feet hide the edge; "
        "%.1f px rms, the flange %.0f x %.0f px, %.3f to 1 against the museum's %.3f); at the lip n = %.2f, fitted to the "
        "white lip on %s through a pinhole %.2f m off, %.1f deg up (%d of %d points, %.1f px rms); an ellipse (n = 2) "
        "misses the lip by %.1f px rms, and n = 3 by %.1f. The exponent runs evenly up the wall between them. The "
        "underside runs out to the flange's face over a %.0f mm arris. Four cloud-scroll feet traced from the base "
        "photograph, each hugging a corner of the base, %.1f mm high, laid in shares of the fitted flange (the tracing "
        "reaches %.3f of it at the corners, where the feet's own faces show beyond the flange's edge; a foot stands under "
        "the base, so they are brought within its underside). Glaze colour from the lit outside wall, face on (pixels "
        "%d..%d x %d..%d), relative to the photograph's white." % (
            RU['image'].split('/')[-1], RU['shares']['feet'], RU['shares']['flange'],
            1 - RU['shares']['feet'] - RU['shares']['flange'], 1000 * RU['step_in'], 100 * mouth[0], 100 * mouth[1],
            100 * foot_axes[0], 100 * foot_axes[1], 100 * uniform_foot, 100 * offset_foot, 100 * foot_axes[1],
            base[4], RU_PLAN['base'].split('/')[-1], base_kept, base_rays, base_rms, 2 * base[2], 2 * base[3], base[2] / base[3],
            foot_axes[0] / foot_axes[1], lip_q[7], RU_PLAN['lip'].split('/')[-1], lip_q[0], math.degrees(lip_q[1]), lip_kept,
            lip_traced, lip_rms, lip_misfit(lip_q, lip_points, mouth, 2.0), lip_misfit(lip_q, lip_points, mouth, 3.0), 1000 * RU_ARRIS, 1000 * hf,
            furthest, x0, x1, y0, y1))
    section = [[round(r, 5), round(z, 5)] for r, z in section]
    faults = section_faults(section, first, last, t)
    if faults:
        raise SystemExit('the Ru basin\'s section is not a solid: ' + '; '.join(faults[:6]))
    rec = dict(work=RU['work'], source=RU['museum']['source'], grade='derived', derivation=derivation,
               museum={k: v for k, v in RU['museum'].items() if k != 'source'},
               section=section, outside=[first, last + 1], wallMeters=round(t, 5),
               feet=dict(heightMeters=round(hf, 5), each=feet),
               plan=dict(footExponent=round(float(base[4]), 3), mouthExponent=round(float(lip_q[7]), 3),
                         footOutline=base_outline, mouthOutline=lip_outline))
    return rec, glaze, spread, pressed_foot


# ---------------------------------------------------------------------------------------------------------------------
# Self-test: the repeat and the ring's registration on pictures whose answers are known
# ---------------------------------------------------------------------------------------------------------------------
def seen_from_front(half_width_degrees=55.0):
    """How squarely a view facing the front sees each cell: the cosine of the angle round from it, nothing past the given
    half width."""
    phi = (np.arange(AZIMUTH_STEPS) + 0.5) / AZIMUTH_STEPS * 360.0
    off = (phi + 180.0) % 360.0 - 180.0
    square = np.where(np.abs(off) <= half_width_degrees, np.cos(np.radians(off)), 0.0)
    return np.repeat(square[None], HEIGHT_STEPS, 0)


def panels(count, rng):
    """A band of `count` panels round the vessel, each a dark divider at its edge and an emblem in its middle, on white;
    hand-painted, each a little different."""
    pic = np.full((HEIGHT_STEPS, AZIMUTH_STEPS, 3), 235, np.uint8)
    width = AZIMUTH_STEPS / count
    for k in range(count):
        edge = int(round(k * width))
        pic[:, edge:edge + 6] = (120, 60, 40)
        mid = int(round((k + 0.5) * width))
        size = 8 + int(rng.integers(0, 3))
        pic[HEIGHT_STEPS // 3:2 * HEIGHT_STEPS // 3, mid - size:mid + size] = (140, 70, 50)
    return pic


def selftest():
    """Checks band_repeat and the ring's registration where the answer is known. Returns the failures."""
    rng = np.random.default_rng(0)
    fails = []
    square = seen_from_front()
    # seven panels seen over 110 degrees: the repeat is a panel, 51.4 degrees, though only two fit in the seen arc
    turn, corr = band_repeat([(panels(7, rng), square)], 1.0, (0.0, 1.0), 30.0)
    if turn is None or abs(turn - 360.0 / 7) > 1.0:
        fails.append('seven panels seen over 110 degrees repeat every %s degrees (correlation %.2f), not 51.4' % (turn, corr))
    # noise has no repeat to trust
    noise = rng.integers(0, 255, (HEIGHT_STEPS, AZIMUTH_STEPS, 3)).astype(np.uint8)
    turn, corr = band_repeat([(noise, square)], 1.0, (0.0, 1.0), 6.0)
    if turn is not None:
        fails.append('noise repeats every %.1f degrees (correlation %.2f)' % (turn, corr))
    # a ring of coloured strokes, and a side view of it turned 100 degrees: registered, the ring's angle under the view's
    # front is 100 degrees
    rows = RING_COLLAR_ROWS + 20
    ring = np.full((rows, AZIMUTH_STEPS, 3), 235, np.uint8)
    colours = [(40, 40, 200), (40, 200, 220), (40, 160, 40), (200, 60, 40)]
    for k in range(60):
        c = int(rng.integers(0, AZIMUTH_STEPS - 12))
        r0 = int(rng.integers(0, rows - 8))
        ring[r0:r0 + 8, c:c + 12] = colours[k % 4]
    # a count read by eye stands where the measured repeat gives it, and not where the repeat falls between two counts
    square55 = seen_from_front()
    p, how = band_of(dict(name='seven', rows=(0.0, 1.0), count=7, read='7'), [(panels(7, rng), square55)], 1.0)
    if p is None or abs(p - 360.0 / 7) > 1e-9:
        fails.append('seven panels read and measured as seven are not carried round as seven (%s)' % how)
    halfway = np.full((HEIGHT_STEPS, AZIMUTH_STEPS, 3), 235, np.uint8)
    for k in range(13):
        edge = int(round(k * AZIMUTH_STEPS / 6.5))
        halfway[:, edge % AZIMUTH_STEPS:(edge + 6) % AZIMUTH_STEPS or AZIMUTH_STEPS] = (120, 60, 40)
    turn, corr = band_repeat([(halfway, square55)], 1.0, (0.0, 1.0), 30.0)
    p, how = band_of(dict(name='between', rows=(0.0, 1.0), count=7, read='7'), [(halfway, square55)], 1.0)
    if turn is None or p is not None:
        fails.append('a band repeating 6.5 times a turn (measured %s), read as 7, is carried round as %s (%s)' % (
            turn, p and round(360.0 / p), how))
    # the record's stated widest diameters are its sections' as stored, the lip's turn too
    for rec in json.load(open(RECORD, encoding='utf-8'))['vessels']:
        said = rec.get('derivation', '')
        if 'widest diameter ' in said:
            stated = float(said.split('widest diameter ')[1].split(' cm')[0])
            if abs(stated - 100 * widest_of(rec['section'])) > 0.005:
                fails.append("%s's derivation says its widest diameter is %.2f cm, its section %.2f" % (
                    rec['work'], stated, 100 * widest_of(rec['section'])))
    # two views of one painting: laid 8 mm apart round a 0.1 m cylinder their offset is measured so; laid true, none
    zs_c, r_c = np.array([0.0, 0.2]), np.array([0.1, 0.1])
    painting = np.full((HEIGHT_STEPS, AZIMUTH_STEPS, 3), 235, np.uint8)
    for k in range(400):
        c, r0 = int(rng.integers(0, AZIMUTH_STEPS - 10)), int(rng.integers(0, HEIGHT_STEPS - 10))
        painting[r0:r0 + int(rng.integers(3, 10)), c:c + int(rng.integers(3, 10))] = colours[k % 4]
    shift = int(round(0.008 / (2 * np.pi * 0.1 / AZIMUTH_STEPS)))
    both = np.full((HEIGHT_STEPS, AZIMUTH_STEPS), 0.7)
    moved, _ = offset_of((painting, both), (np.roll(painting, shift, 1), both), zs_c, r_c)
    true, _ = offset_of((painting, both), (painting, both), zs_c, r_c)
    if moved is None or abs(moved - 0.008) > 0.001 or true is None or true > 0.0005:
        fails.append('two views of a painting laid 8 mm apart are measured %s apart, and laid true %s' % (moved, true))
    # a band never reaches over what a vessel carries once (the Xuande jar's reign mark)
    for vessel in VESSELS:
        for band in vessel['bands']:
            for m in vessel.get('once', []):
                if band['rows'][0] < m['rows'][1] and m['rows'][0] < band['rows'][1]:
                    fails.append("%s's %s reaches over %s, which it carries once" % (vessel['work'], band['name'], m['name']))
    # a superellipse traced with noise gives back its exponent
    t = np.linspace(0, 2 * np.pi, 720, endpoint=False)
    sq = superellipse_share(np.cos(t), np.sin(t), 2.35) ** -1
    x = 1500 + 800 * np.cos(t) * sq + rng.normal(0, 1.0, len(t))
    y = 1100 - 530 * np.sin(t) * sq + rng.normal(0, 1.0, len(t))
    q, rms, _ = fit_superellipse(x, y, (1480, 1120, 780, 520, 2.8))
    if abs(q[4] - 2.35) > 0.03:
        fails.append('a superellipse of exponent 2.35 is fitted at %.3f' % q[4])
    turn = 200
    side = np.roll(ring, -turn, 1)
    angle, top, runner = ring_peak(ring_landscape(ring, side, seen_from_front()[:rows]))
    if abs(math.degrees(angle) - turn * 360.0 / AZIMUTH_STEPS) > 1.0:
        fails.append('a side view turned %.0f degrees is registered at %.1f' % (turn * 360.0 / AZIMUTH_STEPS, math.degrees(angle)))
    return fails


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--write', action='store_true')
    ap.add_argument('--sheets')
    ap.add_argument('--only')
    ap.add_argument('--selftest', action='store_true')
    args = ap.parse_args()
    sys.stdout.reconfigure(encoding='utf-8')
    if args.selftest:
        fails = selftest()
        print('; '.join(fails) if fails else 'selftest: the repeat and the ring registration find what they were given')
        sys.exit(1 if fails else 0)
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
                ''.join('; on %s %.1f%% shared, correlation %.2f, %s' % (n, 100 * s_, k, 'offset %.1f mm' % (1000 * o) if o is not None else 'no offset measured')
                        for n, s_, k, o in v['overlaps'])))
        print('  ' + '; '.join(d['checks']) + '; coverage %.2f; %s form; %s; section %d points' % (
            d['coverage'], 'open' if d['open'] else 'closed', d['wall_how'], len(d['section'])))
        for b, (p, how) in zip(vessel['bands'], d.get('bands', [])):
            print('  band %s: %s; %s' % (b['name'], 'repeats every %.2f deg (%d round)' % (p, round(360 / p)) if p else 'left in the glaze', how))
        if d.get('departure'):
            print('  departure: ' + d['departure'])
        if d.get('ring'):
            g = d['ring']
            print('  ring: camera %.3f m under the upper line, focal %.0f px; lower line %.1f mm up at %.1f mm; foot %.1f mm; upper line %.1f mm up' % (
                g['D'], g['f'], 1000 * g['z_lower'], 1000 * g['lower'], 1000 * g['foot'], 1000 * g['z_upper']))
            for v, reg in zip(d['views'], d.get('ring_reg', [])):
                print('  ring: %s under %.1f deg, correlation %.2f (next %.2f)' % (v['view']['image'].split('/')[-1], math.degrees(reg[0]), reg[1], reg[2]))
        if 'glaze' in d:
            print('  glaze CIELAB', d['glaze'])
    ru = None
    if not args.only or args.only == RU['work']:
        ru, glaze, spread, pressed_foot = derive_ru(ware)
        print(RU['work'])
        print('  foot pressed to its own oval: %.1f x %.1f cm (museum 19.3 x 12.9); glaze CIELAB %s, spread %.3f' % (
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
