"""
Albion's stained glass, made from the designs (plain Python: numpy, OpenCV, PIL):

    python windows/Scripts/albion_glass.py [tristram] [west] [east]

Bradford's own Tristram and Isolde panels (Morris, Marshall, Faulkner & Co., 1862) have no open photographs (the board:
"photographs to ask of Bradford"). What is open are the designers' cartoons and studies for them (Birmingham Museums,
CC0; assets/albion/glass): Hughes's Birth of Tristram, Burne-Jones's Madness of Tristram and Tomb of Tristram and Isoude,
drawn as the glaziers needed them, with the leads marked in brown; Rossetti's study for the Love Potion; Morris's study.
From these the panels are glazed as the firm glazed its cartoons: each piece of pot-metal glass cut along the leads, its
colour chosen from the firm's palette of the 1860s (ruby, a deep blue, greens, a golden yellow stain, murrey, white),
the cartoon's line and shading fired on as brown-black grisaille, the glass streaked and seeded. Where no cartoon is open,
a panel is glazed from the nearest open design (flagged in assets/albion/glass/STANDINS.md). The side lancets: Philip
Webb's birds and Morris's flowers in diamond quarries (west), and Chaucer's Good Women after Burne-Jones's designs (east).

Output (SourceArt/Albion, imported by albion_materials.py): RGBA, RGB the glass's transmittance (sRGB), A the lead.
- T_albion_tristram.png: 6 × 2 panels (T1 … T6 over T7 … T12), each 1.6 × 1.7 m at 640 px a metre.
- T_albion_lancets_west.png, T_albion_lancets_east.png: 6 × 1 lancets (1.42 × 4.01 m), north to south.
"""
import json
import math
import os
import sys

import cv2
import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
SRC = os.path.join(REPO, "assets", "albion", "glass")
OUT = os.path.normpath(os.path.join(HERE, "..", "SourceArt", "Albion"))
PPM = 640                       # pixels a metre

# Morris & Co.'s glass of the 1860s (sRGB, as transmitted light reads).
PALETTE = {
    "ruby": (150, 22, 30), "blue": (28, 52, 128), "sky": (84, 120, 176), "green": (48, 104, 52), "olive": (118, 128, 52),
    "yellow": (214, 162, 44), "murrey": (104, 44, 84), "white": (226, 222, 196), "flesh": (236, 206, 176), "brown": (132, 86, 44),
}


def load_gray(path, crop):
    im = Image.open(path).convert("RGB")
    w, h = im.size
    x0, y0, x1, y1 = crop
    im = im.crop((int(x0 * w), int(y0 * h), int(x1 * w), int(y1 * h)))
    return np.asarray(im).astype(np.float32) / 255.0


def lead_from_brown(rgb, size):
    """The leads a cartoon marks in brown ink: warm and dark strokes, thick (a pencil line is neither), closed."""
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    lum = (r + g + b) / 3.0
    m = (((r - b) > 0.10) & (lum < 0.40)).astype(np.uint8)
    m = cv2.morphologyEx(m, cv2.MORPH_OPEN, np.ones((2, 2), np.uint8))
    m = cv2.resize(m * 255, size, interpolation=cv2.INTER_AREA)
    m = (m > 70).astype(np.uint8)
    m = cv2.morphologyEx(m, cv2.MORPH_CLOSE, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (11, 11)))
    # Thin to a lead's width (6 mm): the skeleton, drawn again at a steady width.
    sk = cv2.ximgproc.thinning(m * 255) if hasattr(cv2, "ximgproc") else cv2.erode(m, np.ones((3, 3), np.uint8)) * 255
    return sk > 0


def lead_from_drawing(rgb, size):
    """A pencil study has no leads marked: lead round the drawing's masses (the figures, their garments), as the
    glazier would cut them from its cartoon."""
    gray = cv2.resize(rgb.mean(axis=2), size, interpolation=cv2.INTER_AREA)
    paper = cv2.GaussianBlur(gray, (0, 0), 25)
    ink = np.clip((paper - gray) * 3.2, 0, 1)
    mass = cv2.GaussianBlur(ink, (0, 0), 6) > 0.08
    mass = cv2.morphologyEx(mass.astype(np.uint8), cv2.MORPH_CLOSE, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (21, 21)))
    edges = cv2.morphologyEx(mass, cv2.MORPH_GRADIENT, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))) > 0
    # And along the drawing's strongest lines (the folds), thinned to a lead.
    strong = cv2.GaussianBlur(ink, (0, 0), 1.5) > 0.55
    strong = cv2.morphologyEx(strong.astype(np.uint8), cv2.MORPH_OPEN, np.ones((2, 2), np.uint8)) > 0
    return edges | strong


def cutlines(regions_mask, max_piece_px, seed):
    """Extra leads in pieces too big to cut in one: a Voronoi of the region, as a glazier breaks a background."""
    h, w = regions_mask.shape
    rng = np.random.default_rng(seed)
    n = int(regions_mask.sum() / (max_piece_px ** 2)) + 1
    pts = np.stack([rng.random(n * 3) * w, rng.random(n * 3) * h], axis=1)
    pts = pts[[regions_mask[int(p[1]) % h, int(p[0]) % w] for p in pts]][:n]
    if len(pts) < 2:
        return np.zeros_like(regions_mask)
    sub = cv2.Subdiv2D((0, 0, w, h))
    for p in pts:
        sub.insert((float(min(p[0], w - 1)), float(min(p[1], h - 1))))
    facets, _ = sub.getVoronoiFacetList([])
    lines = np.zeros((h, w), np.uint8)
    for f in facets:
        cv2.polylines(lines, [np.int32(f)], True, 255, 1)
    return (lines > 0) & regions_mask


def neighbours(lab, n):
    """Which pieces touch which (across one lead)."""
    k = np.ones((9, 9), np.uint8)
    adj = [set() for _ in range(n)]
    for i in range(1, n):
        m = (lab == i).astype(np.uint8)
        if m.sum() == 0:
            continue
        ring = cv2.dilate(m, k) > 0
        for j in np.unique(lab[ring]):
            if j != i and j > 0:
                adj[i].add(int(j))
    return adj


def glaze(rgb, lead, size, seed, scheme):
    """Colour each piece, fire the cartoon's lines on it, and texture the glass. Returns RGBA uint8."""
    w, h = size
    gray = cv2.resize(rgb.mean(axis=2), size, interpolation=cv2.INTER_AREA)
    paper = cv2.GaussianBlur(gray, (0, 0), 25)
    ink = np.clip((paper - gray) * 1.7, 0, 0.8)
    edge = np.zeros((h, w), bool)
    edge[:5, :] = edge[-5:, :] = edge[:, :5] = edge[:, -5:] = True
    leads = lead | edge
    leads = cv2.dilate(leads.astype(np.uint8), cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (4, 4))) > 0
    # The figures: where the cartoon is drawn densely (and what its leads enclose); the rest is the ground behind them.
    mass = cv2.GaussianBlur(ink, (0, 0), 7) > 0.09
    mass = cv2.morphologyEx(mass.astype(np.uint8), cv2.MORPH_CLOSE, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (31, 31))) > 0
    mass = cv2.morphologyEx(mass.astype(np.uint8), cv2.MORPH_OPEN, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (15, 15))) > 0
    ground = ~mass
    outline = cv2.morphologyEx(mass.astype(np.uint8), cv2.MORPH_GRADIENT, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))) > 0
    leads |= outline
    # Cut what is too big: figure pieces at about 12 cm, the ground at about 16 cm.
    n0, lab0 = cv2.connectedComponents((~leads).astype(np.uint8), connectivity=4)
    bigf = np.zeros((h, w), bool)
    for i in range(1, n0):
        m = lab0 == i
        if m.sum() > (0.13 * PPM) ** 2 and (m & mass).sum() > 0.5 * m.sum():
            bigf |= m
    leads |= cv2.dilate(cutlines(bigf, 0.12 * PPM, seed).astype(np.uint8), np.ones((3, 3), np.uint8)) > 0
    leads |= cv2.dilate(cutlines(ground & ~leads, 0.16 * PPM, seed + 7).astype(np.uint8), np.ones((3, 3), np.uint8)) > 0
    n, lab = cv2.connectedComponents((~leads).astype(np.uint8), connectivity=4)
    rng = np.random.default_rng(seed)
    adj = neighbours(lab, n)
    info = {}
    for i in range(1, n):
        m = lab == i
        a = int(m.sum())
        if a == 0:
            continue
        ys, xs = np.nonzero(m)
        info[i] = (a, xs.mean() / w, ys.mean() / h, float(ink[m].mean()), bool(ground[m].mean() > 0.5))
    choice = {}
    for i in sorted(info, key=lambda k: -info[k][0]):
        a, cx, cy, detail, is_ground = info[i]
        if is_ground:
            if cy < 0.2:
                choice[i] = "sky" if rng.random() < 0.7 else "blue"
            elif cy > 0.78:
                choice[i] = "green" if rng.random() < 0.65 else "olive"
            else:
                choice[i] = "white" if detail > 0.05 or rng.random() < 0.4 else ("green" if rng.random() < 0.5 else "sky")
            continue
        taken = {choice[j] for j in adj[i] if j in choice}
        choice[i] = scheme(cx, cy, a / (w * h), detail, rng, taken)
    col = np.zeros((h, w, 3), np.float32)
    for i, c in choice.items():
        col[lab == i] = np.array(PALETTE[c], np.float32) / 255.0 * (1.0 + rng.normal(0, 0.04))
    # Glass: streaky pot metal, a slow mottle, seeds.
    streak = cv2.GaussianBlur(np.random.default_rng(seed + 1).standard_normal((h, w)).astype(np.float32), (0, 0), sigmaX=2.0, sigmaY=18.0)
    streak = streak / (streak.std() + 1e-6)
    mottle = cv2.GaussianBlur(np.random.default_rng(seed + 2).standard_normal((h, w)).astype(np.float32), (0, 0), 30)
    mottle /= mottle.std() + 1e-6
    col *= (1.0 + 0.05 * streak[..., None] + 0.07 * mottle[..., None])
    seeds = cv2.dilate((np.random.default_rng(seed + 3).random((h, w)) > 0.9985).astype(np.uint8), np.ones((2, 2), np.uint8)) > 0
    col = np.where(seeds[..., None], col * 1.12, col)
    # Fired paint: the cartoon's line and shading as brown-black trace and matt (lighter on the ground's quarries).
    k = np.where(ground, ink * 0.7, ink)[..., None]
    col = col * (1 - k) + np.array([0.08, 0.06, 0.04], np.float32) * k
    dist = cv2.distanceTransform((~leads).astype(np.uint8), cv2.DIST_L2, 3)
    col *= (0.86 + 0.14 * np.clip(dist / 8.0, 0, 1))[..., None]
    out = np.zeros((h, w, 4), np.uint8)
    out[..., :3] = np.clip(col * 255, 0, 255).astype(np.uint8)
    out[..., 3] = (leads * 255).astype(np.uint8)
    return out


def scheme_figures(bias):
    """The figures' pieces: faces and hands in white (flesh painted), hair and shadows deep, garments in the palette,
    no two touching pieces the same."""
    garments = ["ruby", "blue", "green", "yellow", "murrey", "olive", "white"]

    def f(cx, cy, frac, detail, rng, taken):
        if frac < 0.004 and detail < 0.22:
            return "flesh"
        if detail > 0.32 and frac < 0.01:
            return "brown"
        opts = [g for g in garments if g not in taken] or garments
        w = np.array([bias.get(g, 1.0) for g in opts], float)
        return str(rng.choice(opts, p=w / w.sum()))
    return f


# The twelve panels (the Details board): source, crop (fractions of the photograph: the paper inside its mount), how the
# leads are found (brown: marked on the cartoon), and a colour bias. None: a stand-in from the nearest open design.
PANELS = [
    ("T1 The Birth of Sir Tristram (Hughes)", "tristram-cartoon-birth-of-tristram_hughes_bmt.jpg", (0.06, 0.05, 0.94, 0.90), {"ruby": 2, "green": 1.5}),
    ("T2 The Fight between Sir Tristram and Sir Marhaus (Rossetti)", None, None, {"blue": 2}),
    ("T3 The Departure of Tristram and Isoude from Ireland (Prinsep)", None, None, {"sky": 2}),
    ("T4 Tristram and Isoude drink the Love Potion (Rossetti)", "tristram-study-love-potion_rossetti_bmt.jpg", (0.05, 0.04, 0.95, 0.96), {"ruby": 2, "yellow": 1.5}),
    ("T5 The Marriage of Tristram and Isoude les Blanches Mains (Burne-Jones)", None, None, {"murrey": 2}),
    ("T6 The Madness of Tristram (Burne-Jones)", "tristram-cartoon-madness-of-tristram_burne-jones_bmt.jpg", (0.09, 0.08, 0.915, 0.93), {"green": 2}),
    ("T7 The Attempted Suicide of La Belle Isoude (Burne-Jones)", None, None, {"blue": 1.5}),
    ("T8 The Recognition of Tristram by La Belle Isoude (Morris or Burne-Jones)", None, None, {"yellow": 2}),
    ("T9 At the Court of King Arthur (Morris)", None, None, {"ruby": 1.5}),
    ("T10 King Mark slays Tristram (Madox Brown)", None, None, {"ruby": 2}),
    ("T11 The Tomb of Tristram and Isoude (Burne-Jones)", "tristram-cartoon-tomb-of-tristram_burne-jones_bmt.jpg", (0.08, 0.05, 0.92, 0.91), {"murrey": 1.5, "blue": 1.5}),
    ("T12 Queen Guenevere and Isoude les Blanches Mains (Morris)", "tristram-study-composition_morris_bmt.jpg", (0.04, 0.04, 0.96, 0.96), {"yellow": 1.5}),
]


def tristram():
    pw, ph = int(1.6 * PPM), int(1.7 * PPM)
    atlas = np.zeros((2 * ph, 6 * pw, 4), np.uint8)
    have = [p for p in PANELS if p[1] and os.path.exists(os.path.join(SRC, p[1]))]
    notes = []
    for k, (title, fn, crop, bias) in enumerate(PANELS):
        src = (fn, crop) if fn and os.path.exists(os.path.join(SRC, fn)) else None
        if src is None:
            # A stand-in: the nearest open design, mirrored so no two panels read the same.
            alt = have[k % len(have)]
            src = (alt[1], alt[2])
            notes.append(f"- {title}: no open design; stood in by the glazing of {alt[0]}, mirrored.")
        rgb = load_gray(os.path.join(SRC, src[0]), src[1])
        if src[0] != fn:
            rgb = rgb[:, ::-1]
        lead = lead_from_brown(rgb, (pw, ph))
        if lead.mean() < 0.02:
            lead = lead_from_drawing(rgb, (pw, ph))
        tile = glaze(rgb, lead, (pw, ph), 100 + k, scheme_figures(bias))
        r, c = (0, k) if k < 6 else (1, k - 6)
        atlas[r * ph:(r + 1) * ph, c * pw:(c + 1) * pw] = tile
    os.makedirs(OUT, exist_ok=True)
    Image.fromarray(atlas, "RGBA").save(os.path.join(OUT, "T_albion_tristram.png"))
    with open(os.path.join(SRC, "STANDINS.md"), "w", encoding="utf-8") as f:
        f.write("# Albion's stained glass: what is the design, what is a stand-in\n\n"
                "Bradford Museums publish no open photographs of the Tristram and Isolde panels; the panels are glazed by "
                "Scripts/albion_glass.py from the designers' open cartoons and studies (Birmingham Museums, CC0).\n\n"
                + "\n".join(notes) + "\n")
    print(f"tristram: {len(PANELS) - len(notes)} panels from their designs, {len(notes)} stand-ins")


# ============================================================================================ the lancets

LW, LH = int(1.42 * PPM), int(4.01 * PPM)


def lancet_shape():
    """The lancet's opening (1.42 wide, sill 0, springing 3.2, head 4.01; arcs of 0.808) in atlas pixels: True inside."""
    ys, xs = np.mgrid[0:LH, 0:LW].astype(np.float32)
    x = (xs + 0.5) / PPM - 0.71
    z = 4.01 - (ys + 0.5) / PPM
    half = 0.7
    spring = 4.0 - math.sqrt(0.808 ** 2 - (0.808 - 0.7) ** 2)
    off = 0.808 - 0.7
    inside = np.where(z < spring, np.abs(x) < half, np.sqrt((np.abs(x) + off) ** 2 + (z - spring) ** 2) < 0.808)
    return inside & (z > 0.0)


def quarries(seed, flowers):
    """Webb's birds and Morris's flowers in diamond quarries (white glass, silver stain, grisaille), a coloured border."""
    rng = np.random.default_rng(seed)
    inside = lancet_shape()
    ys, xs = np.mgrid[0:LH, 0:LW].astype(np.float32)
    x, z = (xs + 0.5) / PPM - 0.71, 4.01 - (ys + 0.5) / PPM
    col = np.zeros((LH, LW, 3), np.float32)
    col[:] = np.array(PALETTE["white"]) / 255.0 * np.array([0.98, 1.0, 0.94])
    # Border 7 cm of ruby and blue lengths, a white fillet inside.
    dist_in = cv2.distanceTransform(inside.astype(np.uint8), cv2.DIST_L2, 5) / PPM
    border = dist_in < 0.07
    fillet = (dist_in >= 0.07) & (dist_in < 0.085)
    seg = np.floor((z + np.abs(x)) / 0.22).astype(int)
    bc = np.where((seg % 2 == 0)[..., None], np.array(PALETTE["ruby"]) / 255.0, np.array(PALETTE["blue"]) / 255.0)
    col = np.where(border[..., None], bc, col)
    col = np.where(fillet[..., None], np.array(PALETTE["yellow"]) / 255.0, col)
    # Diamond quarries (13 cm × 18 cm), leaded.
    a = (x / 0.13 + z / 0.18)
    b = (x / 0.13 - z / 0.18)
    grid = (np.abs(a - np.round(a)) < 0.035) | (np.abs(b - np.round(b)) < 0.035)
    lead = (grid & (dist_in > 0.085)) | (np.abs(dist_in - 0.07) < 0.004) | (np.abs(dist_in - 0.085) < 0.004) | (border & (np.abs(np.mod((z + np.abs(x)) / 0.22, 1.0)) < 0.02))
    lead |= ~inside
    # In the quarries: a painted flower or bird in every third, silver-stained yellow here and there.
    ia, ib = np.floor(a).astype(int), np.floor(b).astype(int)
    fa, fb = a - ia - 0.5, b - ib - 0.5
    cell = (ia * 7 + ib * 13) % 3 == 0
    motif = np.zeros((LH, LW), np.float32)
    r = np.sqrt(fa * fa + fb * fb)
    th = np.arctan2(fb, fa)
    kind = (ia * 5 + ib * 3 + seed) % 4
    # A flower: petals and a stem; a bird: a body, a head, a tail (Webb's quarry birds are small and plump).
    petals = (r < 0.18 + 0.07 * np.cos(5 * th)) & (r > 0.05)
    bird = ((((fa - 0.02) / 0.22) ** 2 + (fb / 0.12) ** 2) < 1) | ((((fa + 0.2) / 0.07) ** 2 + ((fb - 0.07) / 0.07) ** 2) < 1) | \
           ((fa > 0.15) & (fa < 0.36) & (np.abs(fb + (fa - 0.15) * 0.4) < 0.04))
    shape = np.where(kind < 2 if flowers else kind < 1, petals, bird)
    motif = (cell & shape & (dist_in > 0.1)).astype(np.float32)
    outline = cv2.morphologyEx(motif, cv2.MORPH_GRADIENT, np.ones((3, 3), np.uint8))
    stain = motif * ((kind == 0) | (kind == 2))
    col = np.where((stain > 0)[..., None], col * np.array([1.0, 0.82, 0.38]), col)
    col = col * (1 - 0.75 * outline[..., None])
    col = col * (1 - 0.18 * (motif * (1 - stain))[..., None])
    return col, lead


def good_woman(name, robe, source, crop, seed):
    """A Good Woman after Burne-Jones: the figure glazed from his design, under a canopy, on a blue ground of vine."""
    inside = lancet_shape()
    col = np.zeros((LH, LW, 3), np.float32)
    col[:] = np.array(PALETTE["blue"]) / 255.0
    lead = ~inside
    ys, xs = np.mgrid[0:LH, 0:LW].astype(np.float32)
    x, z = (xs + 0.5) / PPM - 0.71, 4.01 - (ys + 0.5) / PPM
    # The figure's panel: 0.3 … 3.3 m up, the full width less the border.
    fz0, fz1, fw = 0.35, 3.25, 1.10
    if source and os.path.exists(os.path.join(SRC, source)):
        rgb = load_gray(os.path.join(SRC, source), crop)
        h, w = int((fz1 - fz0) * PPM), int(fw * PPM)
        fl = lead_from_brown(rgb, (w, h)) if False else np.zeros((h, w), bool)
        gray = cv2.resize(rgb.mean(axis=2), (w, h), interpolation=cv2.INTER_AREA)
        paper = cv2.GaussianBlur(gray, (0, 0), 20)
        ink = np.clip((paper - gray) * 3.5, 0, 1)
        # The figure's silhouette from the drawing: where the ink is dense, filled.
        fig = cv2.GaussianBlur((ink > 0.12).astype(np.float32), (0, 0), 9) > 0.18
        fig = cv2.morphologyEx(fig.astype(np.uint8), cv2.MORPH_CLOSE, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (25, 25))) > 0
        # Robe below the shoulders, flesh and hair above.
        yy = np.mgrid[0:h, 0:w][0] / h
        robe_c = np.array(PALETTE[robe]) / 255.0
        face_c = np.array(PALETTE["flesh"]) / 255.0
        pc = np.where((yy < 0.16)[..., None], face_c, robe_c)
        pc = np.where(fig[..., None], pc, np.array(PALETTE["blue"]) / 255.0)
        pc = pc * (1 - np.clip(ink * 1.1, 0, 0.9))[..., None]
        # Leads round the figure and across the robe's folds.
        edges = cv2.morphologyEx(fig.astype(np.uint8), cv2.MORPH_GRADIENT, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))) > 0
        cuts = cutlines(fig, 0.16 * PPM, seed) | cutlines(~fig, 0.2 * PPM, seed + 1)
        fl = edges | (cv2.dilate(cuts.astype(np.uint8), np.ones((3, 3), np.uint8)) > 0)
        y0, x0 = int((4.01 - fz1) * PPM), int((1.42 - fw) / 2 * PPM)
        col[y0:y0 + h, x0:x0 + w] = pc
        lead[y0:y0 + h, x0:x0 + w] |= fl
    # The canopy over her (white and yellow stain) and a name scroll at her feet.
    canopy = (z > 3.3) & inside
    col = np.where(canopy[..., None], np.array(PALETTE["white"]) / 255.0 * np.array([1.0, 0.9, 0.6]), col)
    lead |= (np.abs(z - 3.3) < 0.006) | (np.abs(z - 0.33) < 0.006)
    scroll = (z < 0.33) & inside
    col = np.where(scroll[..., None], np.array(PALETTE["white"]) / 255.0, col)
    dist_in = cv2.distanceTransform(inside.astype(np.uint8), cv2.DIST_L2, 5) / PPM
    border = dist_in < 0.06
    col = np.where(border[..., None], np.array(PALETTE["ruby"]) / 255.0, col)
    lead |= np.abs(dist_in - 0.06) < 0.004
    lead = cv2.dilate(lead.astype(np.uint8), cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (4, 4))) > 0
    streak = cv2.GaussianBlur(np.random.default_rng(seed).standard_normal((LH, LW)).astype(np.float32), (0, 0), sigmaX=2.0, sigmaY=16.0)
    col = col * (1 + 0.06 * (streak / (streak.std() + 1e-6)))[..., None]
    return col, lead


# Morris & Co.'s Good Women glass of 1863 (the actual panels: roundel portraits by Burne-Jones on Morris's painted flower
# quarries; Birmingham Museums, CC0): (file, crop inside the panel's frame).
GOOD_WOMEN_GLASS = [("goodwomen-glass-dorigen.jpg", (0.03, 0.02, 0.97, 0.98)), ("goodwomen-glass-constance.jpg", (0.03, 0.02, 0.97, 0.98)),
                    ("goodwomen-glass-penelope.jpg", (0.03, 0.02, 0.97, 0.98))]


def morris_lancet(source, crop, roundel, seed):
    """A lancet of Morris's flower quarries from the real panel's photograph: with its roundel (a Good Woman) at the middle,
    or its quarry rows alone; the rows above and below repeat the panel's own, in a ruby and blue border."""
    inside = lancet_shape()
    dist_in = cv2.distanceTransform(inside.astype(np.uint8), cv2.DIST_L2, 5) / PPM
    ys, xs = np.mgrid[0:LH, 0:LW].astype(np.float32)
    x, z = (xs + 0.5) / PPM - 0.71, 4.01 - (ys + 0.5) / PPM
    b = int(0.085 * PPM)
    wi = LW - 2 * b
    rgb = load_gray(os.path.join(SRC, source), crop)
    hp = int(round(rgb.shape[0] * wi / rgb.shape[1]))
    panel = cv2.resize(np.ascontiguousarray(rgb), (wi, hp), interpolation=cv2.INTER_LANCZOS4)
    blur = cv2.GaussianBlur(panel, (0, 0), 1.5)
    panel = np.clip(panel + 0.5 * (panel - blur), 0, 1)
    row = hp / 6.0                                     # the panel is six rows of quarries
    top, bot = panel[:int(round(row))], panel[int(round(5 * row)):]   # (one row each: the roundel reaches into the second)
    field = np.zeros((LH, wi, 3), np.float32)
    if roundel:
        y0 = int((4.01 - 2.05) * PPM) - hp // 2      # the roundel's centre 2.05 m up
        field[max(0, y0):y0 + hp] = panel[max(0, -y0):LH - y0][:max(0, min(hp, LH - y0))]
        y = y0
        while y > 0:                                   # upward: the panel's top rows again
            y -= top.shape[0]
            a0 = max(0, y)
            field[a0:y + top.shape[0]] = top[a0 - y:]
        y = y0 + hp
        while y < LH:                                  # downward: its bottom rows
            n = min(bot.shape[0], LH - y)
            field[y:y + n] = bot[:n]
            y += bot.shape[0]
    else:
        y, k = 0, 0
        while y < LH:
            strip = top if k % 2 == 0 else bot
            n = min(strip.shape[0], LH - y)
            field[y:y + n] = strip[:n]
            y += strip.shape[0]
            k += 1
    col = np.zeros((LH, LW, 3), np.float32)
    col[:, b:b + wi] = field
    lum = col @ np.array([0.2126, 0.7152, 0.0722], np.float32)
    lead = (lum < 0.13) & (dist_in > 0.07)
    # The border: ruby and blue lengths, a white fillet, as the west's.
    border = dist_in < 0.07
    fillet = (dist_in >= 0.07) & (dist_in < 0.085)
    seg = np.floor((z + np.abs(x)) / 0.22).astype(int)
    bc = np.where((seg % 2 == 0)[..., None], np.array(PALETTE["ruby"]) / 255.0, np.array(PALETTE["blue"]) / 255.0)
    col = np.where(border[..., None], bc, col)
    col = np.where(fillet[..., None], np.array(PALETTE["white"]) / 255.0, col)
    lead |= (np.abs(dist_in - 0.07) < 0.004) | (np.abs(dist_in - 0.085) < 0.004) | (border & (np.abs(np.mod((z + np.abs(x)) / 0.22, 1.0)) < 0.02))
    lead |= ~inside
    streak = cv2.GaussianBlur(np.random.default_rng(seed).standard_normal((LH, LW)).astype(np.float32), (0, 0), sigmaX=1.5, sigmaY=14.0)
    col = col * (1 + 0.03 * (streak / (streak.std() + 1e-6)))[..., None]
    lead = cv2.dilate(lead.astype(np.uint8), np.ones((2, 2), np.uint8)) > 0
    return col, lead


def lancets():
    west = np.zeros((LH, 6 * LW, 4), np.uint8)
    for b in range(6):
        col, lead = quarries(11 + b, flowers=b % 2 == 0)
        west[:, b * LW:(b + 1) * LW, :3] = np.clip(col * 255, 0, 255).astype(np.uint8)
        west[:, b * LW:(b + 1) * LW, 3] = (lead * 255).astype(np.uint8)
    Image.fromarray(west, "RGBA").save(os.path.join(OUT, "T_albion_lancets_west.png"))
    east = np.zeros((LH, 6 * LW, 4), np.uint8)
    # Morris & Co.'s Good Women (1863), the real panels: Dorigen, Constance, Penelope in the first, third and fifth lancets
    # from the north, Morris's flower quarries alone between them.
    for b in range(6):
        src, crop = GOOD_WOMEN_GLASS[(b // 2) % 3] if b % 2 == 0 else GOOD_WOMEN_GLASS[(b // 2 + 1) % 3]
        col, lead = morris_lancet(src, crop, b % 2 == 0, 31 + b)
        east[:, b * LW:(b + 1) * LW, :3] = np.clip(col * 255, 0, 255).astype(np.uint8)
        east[:, b * LW:(b + 1) * LW, 3] = (lead * 255).astype(np.uint8)
    Image.fromarray(east, "RGBA").save(os.path.join(OUT, "T_albion_lancets_east.png"))
    print("lancets: west (Webb and Morris quarries), east (Morris & Co.'s Good Women, 1863)")


if __name__ == "__main__":
    what = set(sys.argv[1:]) or {"tristram", "west", "east"}
    if "tristram" in what:
        import albion_glass_band   # the zones glazing (the older tristram() above glazed by guesswork)
        albion_glass_band.band()
    if what & {"west", "east"}:
        lancets()
