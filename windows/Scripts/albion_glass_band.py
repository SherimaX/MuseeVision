"""
The south screen's Tristram and Isolde band (albion_glass.py tristram runs this): twelve panels (Morris, Marshall,
Faulkner & Co., 1862; Bradford Museums). Each is the open photograph of the panel itself (assets/albion/glass/
tristram-panel-*.jpg; public domain, faithful copies of the 1862 glass): its colours are the glass's own, its black the
leads. A panel whose photograph is missing is glazed from the designers' open cartoons and studies (Birmingham Museums, CC0) in the panels' own manner, as the real ones
show it: figures in white glass and silver-stain yellow with a few pot-metal colours, on a ground of deep green and
brown painted with foliage, the drawing fired on in brown-black, the whole framed by a border of cream quarries.
STANDINS.md says which is which.
"""
import os

import cv2
import numpy as np
from PIL import Image

import albion_glass as G

GLASS = {  # sRGB, as transmitted light reads (the photographs' own glass)
    "white": (226, 214, 180), "stain": (222, 170, 62), "ruby": (150, 30, 28), "green": (62, 100, 42), "brown": (120, 72, 38),
    "blue": (40, 62, 120), "murrey": (96, 40, 62), "ground": (30, 42, 24), "ground2": (46, 34, 22),
}

# (title, source, crop (x0, y0, x1, y1), mirrored, own: the source is the panel's own design or photograph)
PANELS = [
    ("T1 The Birth of Sir Tristram", "tristram-panel-T1-birth.jpg", (0.0, 0.0, 1.0, 1.0), False, True),
    ("T2 The Fight between Sir Tristram and Sir Marhaus", "tristram-panel-T2-marhaus.jpg", (0.0, 0.0, 1.0, 1.0), False, True),
    ("T3 The Departure of Tristram and Isoude from Ireland", "tristram-panel-T3-departure.jpg", (0.0, 0.0, 1.0, 1.0), False, True),
    ("T4 Tristram and Isoude drink the Love Potion", "tristram-panel-T4-love-potion.jpg", (0.0, 0.0, 1.0, 1.0), False, True),
    ("T5 The Marriage of Tristram and Isoude les Blanches Mains", "tristram-panel-T5-marriage.jpg", (0.0, 0.0, 1.0, 1.0), False, True),
    ("T6 The Madness of Tristram", "tristram-panel-T6-madness.jpg", (0.0, 0.0, 1.0, 1.0), False, True),
    ("T7 The Attempted Suicide of La Belle Isoude", "tristram-panel-T7-attempted-suicide.jpg", (0.0, 0.0, 1.0, 1.0), False, True),
    ("T8 The Recognition of Tristram by La Belle Isoude", "tristram-panel-T8-recognition.jpg", (0.0, 0.0, 1.0, 1.0), False, True),
    ("T9 At the Court of King Arthur", "tristram-panel-T9-arthurs-court.jpg", (0.0, 0.13, 1.0, 1.0), False, True),
    ("T10 King Mark slays Tristram", "tristram-panel-T10-king-mark-slays.jpg", (0.0, 0.12, 1.0, 1.0), False, True),
    ("T11 The Tomb of Tristram and Isoude", "tristram-panel-T11-tomb.jpg", (0.0, 0.0, 1.0, 1.0), False, True),
    ("T12 Queen Guenevere and Isoude les Blanches Mains", "tristram-panel-T12-guenevere-and-isoude.jpg", (0.0, 0.0, 1.0, 1.0), False, True),
]
# Where a panel's photograph is missing, the cartoon it is glazed from instead (Birmingham Museums, CC0).
CARTOONS = {
    "T1": ("tristram-cartoon-birth-of-tristram_hughes_bmt.jpg", (0.085, 0.07, 0.935, 0.895)),
    "T6": ("tristram-cartoon-madness-of-tristram_burne-jones_bmt.jpg", (0.09, 0.08, 0.915, 0.93)),
    "T11": ("tristram-cartoon-tomb-of-tristram_burne-jones_bmt.jpg", (0.08, 0.05, 0.92, 0.91)),
    "T4": ("tristram-study-love-potion_rossetti_bmt.jpg", (0.05, 0.04, 0.95, 0.96)),
    "T12": ("tristram-study-composition_morris_bmt.jpg", (0.04, 0.04, 0.96, 0.96)),
}
BORDER = 0.07    # m: the quarry border


def lin(rgb8):
    s = np.asarray(rgb8, np.float32) / 255.0
    return np.where(s <= 0.04045, s / 12.92, ((s + 0.055) / 1.055) ** 2.4)


def photo_panel(path, crop, size, mirror):
    """A photograph of the panel: its colour as it is; its leads (and the paint's blackest strokes) the alpha."""
    rgb = G.load_gray(path, crop)
    if mirror:
        rgb = rgb[:, ::-1]
    src = np.ascontiguousarray(rgb)
    rgb = cv2.resize(src, size, interpolation=cv2.INTER_AREA if src.shape[1] >= size[0] else cv2.INTER_LANCZOS4)
    if src.shape[1] < size[0]:
        # (Most of the photographs are ~670 px for 1.6 m: an unsharp mask after the upscale, and the pot metal's own
        # streaks and seed so the enlargement doesn't read as soft.)
        blur = cv2.GaussianBlur(rgb, (0, 0), 2.0)
        rgb = np.clip(rgb + 0.6 * (rgb - blur), 0, 1)
        rng = np.random.default_rng(int(size[0]) + len(path))
        streak = cv2.GaussianBlur(rng.standard_normal(rgb.shape[:2]).astype(np.float32), (0, 0), sigmaX=1.5, sigmaY=14.0)
        streak /= streak.std() + 1e-6
        rgb = np.clip(rgb * (1.0 + 0.035 * streak[..., None]), 0, 1)
    lum = rgb @ np.array([0.2126, 0.7152, 0.0722], np.float32)
    lead = np.clip((0.085 - lum) / 0.05, 0, 1)
    # Lift the photograph's print contrast a little back toward glass (the scans are dark in the midtones).
    rgb = np.clip(rgb * 1.08, 0, 1) ** 0.92
    out = np.zeros((size[1], size[0], 4), np.uint8)
    out[..., :3] = (rgb * 255 + 0.5).astype(np.uint8)
    out[..., 3] = (lead * 255 + 0.5).astype(np.uint8)
    return out


def glazed_panel(path, crop, size, mirror, seed):
    """A panel glazed from its cartoon in the Bradford panels' manner."""
    w, h = size
    rgb = G.load_gray(path, crop)
    if mirror:
        rgb = np.ascontiguousarray(rgb[:, ::-1])
    gray = cv2.resize(rgb.mean(axis=2), size, interpolation=cv2.INTER_AREA)
    g8 = (np.clip(gray, 0, 1) * 255).astype(np.uint8)
    g8 = cv2.createCLAHE(clipLimit=3.0, tileGridSize=(8, 8)).apply(g8)
    g = g8.astype(np.float32) / 255.0
    paper = cv2.GaussianBlur(g, (0, 0), 20)
    ink = np.clip((paper - g) * 2.6, 0, 1)                                    # the drawing
    lines = ink > 0.18
    # Figure or ground: where the drawing is dense (drapery, faces, hands) the figures; where it is empty, the ground.
    dens = cv2.GaussianBlur(lines.astype(np.float32), (0, 0), 0.05 * G.PPM)
    t = max(0.04, float(np.percentile(dens, 45)))
    figure = cv2.morphologyEx((dens > t).astype(np.uint8), cv2.MORPH_CLOSE, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (31, 31))) > 0
    figure = cv2.morphologyEx(figure.astype(np.uint8), cv2.MORPH_OPEN, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (25, 25))) > 0
    # The leads: the cartoon's own (brown ink), the figures' outlines, and the glazier's cuts in big pieces.
    lead = G.lead_from_brown(rgb, size)
    if lead.mean() < 0.01:
        lead = G.lead_from_drawing(rgb, size)
    edge = cv2.morphologyEx(figure.astype(np.uint8), cv2.MORPH_GRADIENT, np.ones((3, 3), np.uint8)) > 0
    leads = lead | edge
    for k, m in enumerate((figure & ~leads, ~figure & ~leads)):
        leads |= G.cutlines(m, (0.13 if k == 0 else 0.18) * G.PPM, seed + 7 * k)
    b = int(BORDER * G.PPM)
    frame = np.zeros((h, w), bool)
    frame[b - 3:b + 3, b:w - b] = frame[h - b - 3:h - b + 3, b:w - b] = True
    frame[b:h - b, b - 3:b + 3] = frame[b:h - b, w - b - 3:w - b + 3] = True
    leads |= frame
    leads = cv2.dilate(leads.astype(np.uint8), cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))) > 0
    n, lab = cv2.connectedComponents((~leads).astype(np.uint8), connectivity=4)
    # Each piece's glass: the figures' from a garment-sized field (neighbouring pieces share a colour, as a robe's do),
    # mostly white and stain; the ground's deep green or brown.
    rng = np.random.default_rng(seed)
    field = cv2.GaussianBlur(rng.standard_normal((h, w)).astype(np.float32), (0, 0), 0.16 * G.PPM)
    field = (field - field.mean()) / (field.std() + 1e-6)
    field2 = cv2.GaussianBlur(rng.standard_normal((h, w)).astype(np.float32), (0, 0), 0.10 * G.PPM)
    field2 = (field2 - field2.mean()) / (field2.std() + 1e-6)
    idx = lab.ravel()
    cnt = np.bincount(idx, minlength=n).astype(np.float32) + 1e-6
    fig_frac = np.bincount(idx, weights=figure.ravel().astype(np.float32), minlength=n) / cnt
    f1 = np.bincount(idx, weights=field.ravel(), minlength=n) / cnt
    f2 = np.bincount(idx, weights=field2.ravel(), minlength=n) / cnt
    tone = np.bincount(idx, weights=g.ravel(), minlength=n) / cnt
    # The piece's glass by the cartoon's own tone there (its hatching is the painter's shadow: the glazier's darker
    # glass): the densely hatched ground deep green or brown; the half-tones stain, brown or ruby; the paper white.
    tone_s = cv2.GaussianBlur(g, (0, 0), 0.03 * G.PPM)
    ptone = np.bincount(idx, weights=tone_s.ravel(), minlength=n) / cnt
    lo, hi = np.percentile(tone_s, 30), np.percentile(tone_s, 75)
    names = []
    for i in range(n):
        tt = (ptone[i] - lo) / max(1e-3, hi - lo)
        if tt < 0.0:
            c = "ground" if f1[i] < 0.6 else "ground2"
        elif tt < 0.55:
            c = "ruby" if f1[i] > 1.1 else ("blue" if f1[i] < -1.2 else ("brown" if f2[i] < -0.3 else ("green" if f2[i] > 1.0 else "stain")))
        else:
            c = "stain" if f2[i] > 1.2 else "white"
        names.append(c)
    pal = np.array([lin(GLASS[c]) for c in names], np.float32)
    shade = rng.normal(1.0, 0.07, n).astype(np.float32)
    col = pal[lab] * shade[lab][..., None]
    # The quarry border: cream quarries, 0.14 m, between the panel's edge and the frame lead.
    border = np.zeros((h, w), bool)
    border[:b, :] = border[-b:, :] = border[:, :b] = border[:, -b:] = True
    q = int(0.14 * G.PPM)
    yy, xx = np.mgrid[0:h, 0:w]
    along = np.where((yy < b) | (yy >= h - b), xx, yy)
    quarry_lead = border & ((along % q) < 5)
    col[border] = lin(GLASS["white"]) * 0.95
    leads |= quarry_lead
    # Pot metal: streaks; the paint fired on: the drawing in brown-black, a matt wash over the ground's foliage.
    streak = cv2.GaussianBlur(rng.standard_normal((h, w)).astype(np.float32), (0, 0), sigmaX=2.0, sigmaY=16.0)
    streak /= streak.std() + 1e-6
    col *= (1.0 + 0.05 * streak[..., None])
    ground = (tone_s < lo) & ~border
    wash = np.clip(0.35 + 0.5 * (1.0 - g), 0, 0.85)
    col[ground] *= (1.0 - wash[ground])[..., None]
    # The matting: a thin wash of the paint over all but the lights, darker where the cartoon shades (the photographs'
    # glass is never paper-white).
    tn = np.clip((tone_s - lo) / max(1e-3, hi - lo), 0, 1)
    col[~border] *= (0.55 + 0.45 * tn[~border])[..., None]
    paint = np.clip(ink * 1.9, 0, 0.95)
    paint[border] = 0.0
    col = col * (1.0 - paint[..., None]) + lin((24, 18, 12)) * paint[..., None]
    dist = cv2.distanceTransform((~leads).astype(np.uint8), cv2.DIST_L2, 3)
    col *= (0.84 + 0.16 * np.clip(dist / 8.0, 0, 1))[..., None]
    s = np.clip(col, 0, 1)
    s = np.where(s <= 0.0031308, s * 12.92, 1.055 * np.power(s, 1 / 2.4) - 0.055)
    out = np.zeros((h, w, 4), np.uint8)
    out[..., :3] = (s * 255 + 0.5).astype(np.uint8)
    out[..., 3] = (leads * 255).astype(np.uint8)
    return out


def band():
    pw, ph = int(1.6 * G.PPM), int(1.7 * G.PPM)
    atlas = np.zeros((2 * ph, 6 * pw, 4), np.uint8)
    notes = []
    for k, (title, fn, crop, mirror, own) in enumerate(PANELS):
        path = os.path.join(G.SRC, fn)
        key = title.split()[0]
        if not os.path.exists(path) and key in CARTOONS:
            fn, crop = CARTOONS[key]
            path = os.path.join(G.SRC, fn)
        if "-panel-" in fn:
            tile = photo_panel(path, crop, (pw, ph), mirror)
            notes.append(f"- {title}: the panel itself (photograph, {fn}).")
        else:
            tile = glazed_panel(path, crop, (pw, ph), mirror, 100 + k)
            what = "its own design" if own else "a stand-in: another Tristram design"
            notes.append(f"- {title}: glazed from {what} ({fn}{', mirrored' if mirror else ''}); colours ours.")
        r, c = (0, k) if k < 6 else (1, k - 6)
        atlas[r * ph:(r + 1) * ph, c * pw:(c + 1) * pw] = tile
    os.makedirs(G.OUT, exist_ok=True)
    Image.fromarray(atlas, "RGBA").save(os.path.join(G.OUT, "T_albion_tristram.png"))
    with open(os.path.join(G.SRC, "STANDINS.md"), "w", encoding="utf-8") as f:
        f.write("# Albion's Tristram and Isolde band: what is the panel, what is a stand-in\n\n"
                "Written by windows/Scripts/albion_glass_band.py. Photographs of the panels themselves are used where open "
                "ones exist; the rest are glazed from the designers' open cartoons and studies (Birmingham Museums, CC0) in "
                "the panels' manner.\n\n" + "\n".join(notes) + "\n")
    print("tristram: " + "; ".join(n.split(":")[0][2:] + (" photo" if "photograph" in n else " glazed") for n in notes))


if __name__ == "__main__":
    band()
