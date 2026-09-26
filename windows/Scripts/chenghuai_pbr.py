"""
Chenghuai's photographed PBR sets (CC0: ambientCG, Poly Haven), fetched into windows/SourceArt/PBR/CH_<name>/ in the
library's layout (Color.png sRGB, Normal.png DirectX, ORM.png = AO / roughness / metal, Height.png), with their
statistics in SourceArt/PBR/chenghuai_stats.json (chenghuai.py adds them to pbr.py's table, as Albion does) and their
credits in SourceArt/PBR/CREDITS_chenghuai.md. Plain Python (PIL, numpy, OpenCV):

    python windows/Scripts/chenghuai_pbr.py [NAME ...] [--derive]

The sets (each checked by eye and by its normal's tilt before use, LESSONS §2):
- CH_BRICK      Bricks061: reduction-fired grey brick in lime-pointed courses (淌白), about 6.9 cm a course;
- CH_BRICKRUB   derived from it: the same bricks rubbed and laid dry (干摆, 丝缝): the joints filled with the brick's own
                face (inpainted) and left as a hairline, 0.6 mm deep;
- CH_PAVING     PavingStones112: square grey pavers, photogrammetry, moss and dirt in the joints (the courts' 方砖);
- CH_LIME       white_plaster_02: mottled white lime render (粉墙);
- CH_TAIHU      Rock042L: weathered grey limestone, photogrammetry (the rockery, the standing rock);
- CH_LICHEN     Rock046L: grey stone with lichen (the roofs' fired clay as it weathers: tiles, ridges);
- CH_MOSSGROUND Ground037: damp earth under moss and litter, as under a Suzhou garden's trees and in the courts' beds;
- CH_NANMU      rosewood_veneer_02: a fine straight grain with a quiet wavy figure (its tone only), tinted to nanmu's gold by the
                instance (dark_wood's coarse streaks read as bamboo slats);
- CH_CRAZE      derived from mud_cracked_dry_riverbed_002's relief: the fine crazing (断纹) of old lacquer over its
                lime-and-hemp ground, cells 5-20 mm, the cracks 0.1-0.3 mm and dirty.
"""
import io
import json
import os
import sys
import urllib.request
import zipfile

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.normpath(os.path.join(HERE, "..", "SourceArt", "PBR"))
STATS = os.path.join(OUT, "chenghuai_stats.json")
CREDITS = os.path.join(OUT, "CREDITS_chenghuai.md")
UA = {"User-Agent": "MuseeVision texture fetch (github.com/SherimaX/MuseeVision)"}

# name: (source, asset, resolution, metres per repeat, what it stands for)
SETS = {
    "CH_BRICK": ("ambientcg", "Bricks061", "2K", 1.10, "grey brick, lime-pointed courses (淌白)"),
    "CH_PAVING": ("ambientcg", "PavingStones112", "2K", 2.10, "square grey pavers, moss in the joints (the courts' 方砖)"),
    "CH_LIME": ("polyhaven", "white_plaster_02", "2k", 2.0, "white lime render (粉墙)"),
    "CH_TAIHU": ("ambientcg", "Rock042L", "2K", 2.4, "weathered grey limestone (太湖石, the rockery)"),
    "CH_LICHEN": ("ambientcg", "Rock046L", "2K", 2.0, "grey stone with lichen (the roofs' weathered clay)"),
    "CH_NANMU": ("polyhaven", "rosewood_veneer_02", "2k", 1.0, "fine straight-grained veneer, a quiet wavy figure (nanmu's)"),
    "CH_MOSSGROUND": ("ambientcg", "Ground037", "2K", 2.0, "damp earth under moss, leaf litter (the garden's ground and the courts' beds)"),
    "CH_CRAZESRC": ("polyhaven", "mud_cracked_dry_riverbed_002", "2k", 2.0, "crack network (source of CH_CRAZE)"),
}
DERIVED = {
    "CH_BRICKRUB": ("CH_BRICK", 1.10, "rubbed dry-laid grey brick (干摆, 丝缝): Bricks061's joints filled flush, a hairline left"),
    "CH_CRAZE": ("CH_CRAZESRC", 0.30, "fine crazing of old lacquer (断纹), from mud_cracked_dry_riverbed_002's relief"),
}


def get(url):
    with urllib.request.urlopen(urllib.request.Request(url, headers=UA), timeout=300) as r:
        return r.read()


def ambientcg(asset, res):
    z = zipfile.ZipFile(io.BytesIO(get(f"https://ambientcg.com/get?file={asset}_{res}-PNG.zip")))
    maps = {}
    for n in z.namelist():
        low = n.lower()
        for key, tag in (("color", "_color."), ("normal", "_normaldx."), ("rough", "_roughness."), ("ao", "_ambientocclusion."),
                         ("height", "_displacement.")):
            if tag in low:
                maps[key] = z.read(n)
    return maps


def polyhaven(asset, res):
    files = json.loads(get(f"https://api.polyhaven.com/files/{asset}"))
    maps = {}
    for key, names in (("color", ("Diffuse", "diff", "col")), ("normal", ("nor_dx",)), ("rough", ("Rough", "rough")), ("ao", ("AO", "ao")),
                       ("height", ("Displacement", "disp"))):
        for n in names:
            if n in files and res in files[n]:
                f = files[n][res]
                url = (f.get("png") or f.get("jpg"))["url"]
                maps[key] = get(url)
                break
    return maps


def load(data, mode):
    return Image.open(io.BytesIO(data)).convert(mode)


def fetch(name):
    src, asset, res, scale, what = SETS[name]
    d = os.path.join(OUT, name)
    if os.path.exists(os.path.join(d, "ORM.png")):
        print(f"{name} {asset}: already here")
        return
    os.makedirs(d, exist_ok=True)
    maps = ambientcg(asset, res) if src == "ambientcg" else polyhaven(asset, res)
    if "color" not in maps or "normal" not in maps:
        raise RuntimeError(f"{name} {asset}: missing colour or normal ({sorted(maps)})")
    color = load(maps["color"], "RGB")
    size = color.size
    color.save(os.path.join(d, "Color.png"))
    load(maps["normal"], "RGB").resize(size).save(os.path.join(d, "Normal.png"))
    grey = lambda k, v: load(maps[k], "L").resize(size) if k in maps else Image.new("L", size, v)  # noqa: E731
    Image.merge("RGB", (grey("ao", 255), grey("rough", 160), Image.new("L", size, 0))).save(os.path.join(d, "ORM.png"))
    if "height" in maps:
        h = Image.open(io.BytesIO(maps["height"]))
        h = np.asarray(h, np.float32)
        if h.ndim == 3:
            h = h[..., 0]
        h = (h - h.min()) / max(1e-6, h.max() - h.min())
        Image.fromarray((h * 255 + 0.5).astype(np.uint8)).resize(size).save(os.path.join(d, "Height.png"))
    if name == "CH_NANMU":
        # The grain's tone only (luminance): the instance gives it nanmu's gold (a red wood's own hue ratios, blue near 0,
        # would speckle the tint).
        c = np.asarray(color, np.float32) / 255.0
        y = (c ** 2.2) @ np.array([0.2126, 0.7152, 0.0722], np.float32)
        Image.fromarray((np.clip(y ** (1 / 2.2), 0, 1) * 255 + 0.5).astype(np.uint8)).convert("RGB").save(os.path.join(d, "Color.png"))
    write_set(name, src, asset, res, scale, what)
    print(f"{name} {asset} {res}: {size[0]}x{size[1]}, maps {sorted(maps)}")


def write_set(name, src, asset, res, scale, what):
    with open(os.path.join(OUT, name, "SET.txt"), "w", encoding="utf-8") as f:
        f.write(f"{name}\t{src}\t{asset}\t{res}\t{scale}\t{what}\n")


def arr(path, mode="RGB"):
    return np.asarray(Image.open(path).convert(mode), np.float32) / 255.0


def save8(a, path):
    Image.fromarray((np.clip(a, 0, 1) * 255 + 0.5).astype(np.uint8)).save(path)


def normal_from_height(h_mm, mm_per_px):
    import cv2
    p = np.pad(h_mm.astype(np.float32), 2, mode="wrap")     # the sets tile
    gx = cv2.Sobel(p, cv2.CV_32F, 1, 0, ksize=3)[2:-2, 2:-2] / (8 * mm_per_px)
    gy = cv2.Sobel(p, cv2.CV_32F, 0, 1, ksize=3)[2:-2, 2:-2] / (8 * mm_per_px)
    n = np.dstack([-gx, -gy, np.ones_like(gx)])   # DirectX: green towards the image's bottom
    return n / np.linalg.norm(n, axis=2, keepdims=True)


def despeckle(img, mask=None, thresh=0.10, desat=0.0):
    """Small pale specks (lime splashes, grit catching the flash) at a texel or three read as white sparkle at a distance
    (the user's rule: no white specks): each pixel much paler than its 7 px median takes the median. `mask` limits it
    (e.g. to the brick faces); `desat` pulls the colour towards its own grey (reduction-fired brick is neutral)."""
    import cv2
    a = img.astype(np.float32)
    med = cv2.medianBlur(img.astype(np.uint8), 7).astype(np.float32)
    lum, lmed = a.mean(2), med.mean(2)
    spot = (lum - lmed) > thresh * 255
    if mask is not None:
        spot &= mask
    spot = cv2.dilate(spot.astype(np.uint8), np.ones((3, 3), np.uint8)) > 0
    a[spot] = med[spot]
    if desat:
        g = a.mean(2, keepdims=True)
        a = a + (g - a) * desat
    return np.clip(a, 0, 255).astype(np.uint8), float(spot.mean())


def derive_brickrub():
    """干摆 / 丝缝: the joints of Bricks061 (its height's low channels) filled with the bricks' own faces (inpainted), a
    hairline joint 1-1.5 mm left, 0.6 mm deep; the relief of the faces kept."""
    import cv2
    src = os.path.join(OUT, "CH_BRICK")
    d = os.path.join(OUT, "CH_BRICKRUB")
    os.makedirs(d, exist_ok=True)
    orig = os.path.join(src, "source", "Color.png")      # the set's own colour, kept (Color.png is despeckled below)
    if not os.path.exists(orig):
        os.makedirs(os.path.dirname(orig), exist_ok=True)
        os.replace(os.path.join(src, "Color.png"), orig)
    col = (arr(orig) * 255).astype(np.uint8)
    h = arr(os.path.join(src, "Height.png"), "L")
    orm = arr(os.path.join(src, "ORM.png"))
    n_px = h.shape[0]
    mm = SETS["CH_BRICK"][3] * 1000.0 / n_px
    lum = col.astype(np.float32).mean(2) / 255.0
    # The lime joints: pale and low (the height's lower part); the lime smears on the faces go with them.
    joint = ((lum > 0.66) | (h < 0.42)).astype(np.uint8)
    joint = cv2.morphologyEx(joint, cv2.MORPH_OPEN, np.ones((3, 3), np.uint8))
    joint = cv2.morphologyEx(joint, cv2.MORPH_CLOSE, np.ones((5, 5), np.uint8))
    grow = cv2.dilate(joint, np.ones((7, 7), np.uint8))
    fill = cv2.inpaint(col, grow * 255, 9, cv2.INPAINT_TELEA).astype(np.float32)
    # The inpaint is smooth: give the filled joints the faces' own grain, borrowed from half a course up or down (a
    # brick's middle), so no smooth band shows where a joint was.
    course = int(round(n_px / 16))
    detail = col.astype(np.float32) - cv2.GaussianBlur(col.astype(np.float32), (0, 0), 3)
    for shift in (course // 2, -course // 2, course):
        src_ok = np.roll(grow, shift, axis=0) == 0
        take = (grow > 0) & src_ok
        fill[take] += np.roll(detail, shift, axis=0)[take]
        grow_left = (grow > 0) & ~src_ok
        if not grow_left.any():
            break
    fill = np.clip(fill, 0, 255).astype(np.uint8)
    fill, specks = despeckle(fill, None, 0.08, desat=0.55)
    # The pointed brick (CH_BRICK) keeps its lime joints but loses the specks on its faces and the beige cast.
    pointed, _ = despeckle(col, grow == 0, 0.10, desat=0.4)
    Image.fromarray(pointed).save(os.path.join(src, "Color.png"))
    # The hairline: the joints' skeleton (the long runs only: the smears on the faces leave short bits, dropped).
    skel = cv2.ximgproc.thinning(joint * 255) > 0
    n_lab, lab, st, _ = cv2.connectedComponentsWithStats(skel.astype(np.uint8), connectivity=8)
    keep = np.zeros(n_lab, bool)
    keep[1:] = np.maximum(st[1:, cv2.CC_STAT_WIDTH], st[1:, cv2.CC_STAT_HEIGHT]) > 60
    ridge = keep[lab]
    line = cv2.GaussianBlur(ridge.astype(np.float32), (0, 0), 0.8)
    line = np.clip(line * 1.8, 0, 1)
    c = fill.astype(np.float32) / 255.0 * (1.0 - 0.28 * line[..., None])
    hh = cv2.inpaint((h * 255).astype(np.uint8), grow * 255, 6, cv2.INPAINT_TELEA).astype(np.float32) / 255.0
    h_mm = hh * 1.2 - line * 0.6
    nrm = normal_from_height(cv2.GaussianBlur(h_mm, (0, 0), 0.8), mm)
    rough = cv2.inpaint((orm[..., 1] * 255).astype(np.uint8), grow * 255, 6, cv2.INPAINT_TELEA).astype(np.float32) / 255.0
    rough = np.clip(rough + 0.15 * line, 0, 1)
    ao = 1.0 - 0.45 * line
    save8(c, os.path.join(d, "Color.png"))
    save8(nrm * 0.5 + 0.5, os.path.join(d, "Normal.png"))
    Image.merge("RGB", [Image.fromarray((np.clip(x, 0, 1) * 255 + 0.5).astype(np.uint8)) for x in (ao, rough, np.zeros_like(ao))]).save(
        os.path.join(d, "ORM.png"))
    save8((h_mm - h_mm.min()) / max(1e-6, h_mm.max() - h_mm.min()), os.path.join(d, "Height.png"))
    write_set("CH_BRICKRUB", "derived", "Bricks061 (ambientCG)", "2K", DERIVED["CH_BRICKRUB"][1], DERIVED["CH_BRICKRUB"][2])
    print(f"CH_BRICKRUB: joints {joint.mean() * 100:.1f} % of the face filled, hairline {line.mean() * 100:.2f} %, specks {specks * 100:.2f} %")


def derive_craze():
    """断纹: the crack network of a dried riverbed's mud (its relief), made the scale of old lacquer's crazing: the cracks
    thinned to hairlines (0.1-0.3 mm at 0.3 m a repeat), a little dirt in them, the cells each a breath of their own
    tone and polish (faded unevenly), a faint cupping."""
    import cv2
    src = os.path.join(OUT, "CH_CRAZESRC")
    d = os.path.join(OUT, "CH_CRAZE")
    os.makedirs(d, exist_ok=True)
    h = arr(os.path.join(src, "Height.png"), "L")
    n_px = h.shape[0]
    mm = DERIVED["CH_CRAZE"][1] * 1000.0 / n_px
    local = cv2.GaussianBlur(h, (0, 0), 9)
    crack = (h < local - 0.035).astype(np.uint8)
    crack = cv2.morphologyEx(crack, cv2.MORPH_CLOSE, np.ones((5, 5), np.uint8))
    crack = cv2.morphologyEx(crack, cv2.MORPH_OPEN, np.ones((2, 2), np.uint8))
    ridge = cv2.ximgproc.thinning(crack * 255) > 0
    line = cv2.GaussianBlur(ridge.astype(np.float32), (0, 0), 0.6)
    line = np.clip(line * 2.0, 0, 1)
    # The cells: their own tone (a faded or darker patch) and a faint cupping towards the cracks.
    _, cells = cv2.connectedComponents((1 - cv2.dilate(crack, np.ones((3, 3), np.uint8))).astype(np.uint8))
    rng = np.random.default_rng(5)
    tone = rng.normal(0.0, 1.0, cells.max() + 1).astype(np.float32)[cells]
    tone = cv2.GaussianBlur(tone, (0, 0), 1.2) * 0.5
    slow = cv2.GaussianBlur(rng.standard_normal(h.shape).astype(np.float32), (0, 0), n_px / 12)
    slow /= slow.std() + 1e-6
    cup = cv2.GaussianBlur(line, (0, 0), 6)
    colour = 1.0 + 0.035 * tone + 0.05 * slow - 0.22 * line
    h_mm = -0.05 * line - 0.02 * cup
    nrm = normal_from_height(cv2.GaussianBlur(h_mm, (0, 0), 0.7), mm)
    rough = 0.45 + 0.04 * tone + 0.06 * slow + 0.25 * line
    ao = 1.0 - 0.5 * line
    c = np.repeat(np.clip(colour * 0.5, 0, 1)[..., None], 3, axis=2)
    save8(c, os.path.join(d, "Color.png"))
    save8(nrm * 0.5 + 0.5, os.path.join(d, "Normal.png"))
    Image.merge("RGB", [Image.fromarray((np.clip(x, 0, 1) * 255 + 0.5).astype(np.uint8)) for x in (ao, rough, np.zeros_like(ao))]).save(
        os.path.join(d, "ORM.png"))
    save8((h_mm - h_mm.min()) / max(1e-6, h_mm.max() - h_mm.min()), os.path.join(d, "Height.png"))
    write_set("CH_CRAZE", "derived", "mud_cracked_dry_riverbed_002 (Poly Haven)", "2k", DERIVED["CH_CRAZE"][1], DERIVED["CH_CRAZE"][2])
    print(f"CH_CRAZE: {cells.max()} cells, crack lines {line.mean() * 100:.2f} % of the face")


def srgb_to_linear(c):
    c = c / 255.0
    return np.where(c > 0.04045, ((c + 0.055) / 1.055) ** 2.4, c / 12.92)


def tilt(name):
    n = arr(os.path.join(OUT, name, "Normal.png")) * 2 - 1
    return float(np.degrees(np.arctan2(np.hypot(n[..., 0], n[..., 1]), np.maximum(n[..., 2], 1e-3))).mean())


def stats():
    out = {}
    for name in list(SETS) + list(DERIVED):
        d = os.path.join(OUT, name)
        if not os.path.exists(os.path.join(d, "Color.png")):
            continue
        c = np.asarray(Image.open(os.path.join(d, "Color.png")).convert("RGB").resize((256, 256), Image.BILINEAR), np.float32)
        mean = srgb_to_linear(c).reshape(-1, 3).mean(0)
        g = np.asarray(Image.open(os.path.join(d, "ORM.png")).convert("RGB").resize((256, 256), Image.BILINEAR), np.float32)[..., 1] / 255.0
        scale = SETS[name][3] if name in SETS else DERIVED[name][1]
        out[name] = {"color_mean": [round(float(v), 4) for v in mean], "rough_mean": round(float(g.mean()), 4), "scale": scale,
                     "has_height": os.path.exists(os.path.join(d, "Height.png")), "tilt_deg": round(tilt(name), 2)}
    with open(STATS, "w", encoding="utf-8") as f:
        json.dump(out, f, indent=1)
    for k, v in out.items():
        print(f"{k}: mean {v['color_mean']} rough {v['rough_mean']} tilt {v['tilt_deg']} deg")


def credits():
    rows = []
    for name, (src, asset, res, scale, what) in SETS.items():
        url = f"https://ambientcg.com/view?id={asset}" if src == "ambientcg" else f"https://polyhaven.com/a/{asset}"
        rows.append(f"| {name} | {asset} ({'ambientCG' if src == 'ambientcg' else 'Poly Haven'}, {res}) | {what} | {scale} m | {url} | CC0 1.0 |")
    for name, (base, scale, what) in DERIVED.items():
        rows.append(f"| {name} | derived from {base} (chenghuai_pbr.py) | {what} | {scale} m | as {base} | CC0 1.0 |")
    with open(CREDITS, "w", encoding="utf-8") as f:
        f.write("# Chenghuai's PBR sets: credits\n\nFetched and derived by `windows/Scripts/chenghuai_pbr.py`. All CC0 1.0 (public "
                "domain).\n\n| key | set | stands for | scale | source | licence |\n|---|---|---|---|---|---|\n" + "\n".join(rows) + "\n")


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    for k in args or list(SETS):
        if k in SETS:
            try:
                fetch(k)
            except Exception as e:  # noqa: BLE001
                print(f"{k}: FAILED {e}")
    if "--derive" in sys.argv or not os.path.exists(os.path.join(OUT, "CH_BRICKRUB", "ORM.png")):
        derive_brickrub()
    if "--derive" in sys.argv or not os.path.exists(os.path.join(OUT, "CH_CRAZE", "ORM.png")):
        derive_craze()
    credits()
    stats()
