"""
The museum's PBR texture library (CC0: ambientCG, Poly Haven), fetched into windows/SourceArt/PBR/<Key>/ as
Color.png (sRGB), Normal.png (DirectX), ORM.png (R ambient occlusion, G roughness, B metalness; linear) and
Height.png (linear, for Nanite displacement). The keys and sets are the materials spec's (the material
language: four families, S stone, P plaster, M metal, W wood, F fabric, B brick, C concrete, G ground, K clay).

    python windows/Scripts/pbr_fetch.py [KEY ...]      (all when none; a set already fetched is skipped)

Credits: windows/SourceArt/PBR/CREDITS.md (written by this script).
"""
import io
import os
import sys
import urllib.request
import zipfile

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "SourceArt", "PBR")
UA = {"User-Agent": "MuseeVision texture fetch (github.com/SherimaX/MuseeVision)"}

# key: (source, asset id, resolution, what it stands for, metres per repeat)
SETS = {
    "S1": ("ambientcg", "Travertine009", "4K", "vein-cut Roman travertine: the museum stone", 1.20),
    "S2": ("ambientcg", "Marble014", "4K", "cream crackle marble: the Rotunda's and Élan's", 2.20),
    "S3": ("polyhaven", "marble_01", "2k", "weathered honed limestone: the façade and grounds", 1.50),
    "S4": ("ambientcg", "Marble021", "2K", "white statuary marble", 1.50),
    "S6": ("ambientcg", "Marble009", "2K", "verde antico", 1.00),
    "S7": ("ambientcg", "Marble026", "2K", "giallo antico (base)", 1.50),
    "P1": ("ambientcg", "Plaster001", "2K", "trowelled lime plaster", 2.00),
    "P2": ("polyhaven", "white_stucco", "2k", "fine stucco: coffers, mouldings", 2.00),
    "P3": ("ambientcg", "Plaster003", "2K", "limewash over brick", 2.00),
    "M1": ("ambientcg", "Metal015", "2K", "patinated bronze", 1.00),
    "M2": ("ambientcg", "Metal009", "2K", "brushed metal (bronze tint)", 0.50),
    "M3": ("ambientcg", "Metal048C", "2K", "aged gilt", 0.50),
    "M4": ("ambientcg", "Metal048A", "2K", "clean gilt", 0.50),
    "M5": ("ambientcg", "Metal029", "2K", "satin lacquer orange peel (pearl steel)", 0.50),
    "W1": ("polyhaven", "oak_wood_planks", "2k", "European oak floor planks (superseded by W2: its painted plank joints)", 1.20),
    # The museum's oak: a flat-sawn European oak veneer (cathedral figure, open pores in its roughness), no joints; the
    # boards are drawn by M_Wood, each its own slice and tone.
    "W2": ("polyhaven", "oak_veneer_01", "4k", "European oak, flat-sawn veneer: the museum's timber", 1.83),
    "F1": ("ambientcg", "Fabric036", "2K", "slubbed silk/linen weave", 0.30),
    "F2": ("ambientcg", "Fabric034", "2K", "wool felt", 0.50),
    "F3": ("ambientcg", "Leather037", "2K", "smooth aniline leather", 0.50),
    "B1": ("polyhaven", "yellow_brick", "4k", "buff stock brick, lime mortar", 1.95),
    "C1": ("polyhaven", "concrete_floor_01", "2k", "ground polished concrete", 2.00),
    "G1": ("ambientcg", "Granite002B", "2K", "Suzhou grey granite", 0.60),
    "G2": ("ambientcg", "Rocks023", "2K", "pebble mosaic", 1.00),
    "G3": ("ambientcg", "Gravel022", "2K", "river gravel", 1.50),
    "G4": ("ambientcg", "Rock026", "2K", "grey limestone rock (Taihu)", 2.00),
    "G5": ("ambientcg", "Rock032", "2K", "dark stone: pond and basin beds", 1.50),
    "K1": ("ambientcg", "Concrete017", "2K", "grey fired clay", 1.00),
    "L1": ("ambientcg", "Grass004", "2K", "lawn", 1.40),
}


def get(url):
    with urllib.request.urlopen(urllib.request.Request(url, headers=UA), timeout=300) as r:
        return r.read()


def load(data, mode):
    return Image.open(io.BytesIO(data)).convert(mode)


def ambientcg(asset, res):
    z = zipfile.ZipFile(io.BytesIO(get(f"https://ambientcg.com/get?file={asset}_{res}-JPG.zip")))
    maps = {}
    for n in z.namelist():
        low = n.lower()
        for key, tag in (("color", "_color."), ("normal", "_normaldx."), ("rough", "_roughness."), ("ao", "_ambientocclusion."),
                         ("metal", "_metalness."), ("height", "_displacement.")):
            if tag in low:
                maps[key] = z.read(n)
    return maps


def polyhaven(asset, res):
    maps = {}
    base = f"https://dl.polyhaven.org/file/ph-assets/Textures/jpg/{res}/{asset}/{asset}"
    for key, tag in (("color", "diff"), ("normal", "nor_dx"), ("rough", "rough"), ("ao", "ao"), ("height", "disp")):
        try:
            maps[key] = get(f"{base}_{tag}_{res}.jpg")
        except Exception:  # noqa: BLE001  (not every set has every map)
            pass
    return maps


def fetch(key):
    src, asset, res, what, scale = SETS[key]
    d = os.path.join(OUT, key)
    if os.path.exists(os.path.join(d, "ORM.png")):
        print(f"{key} {asset}: already here")
        return
    os.makedirs(d, exist_ok=True)
    maps = ambientcg(asset, res) if src == "ambientcg" else polyhaven(asset, res)
    if "color" not in maps or "normal" not in maps:
        raise RuntimeError(f"{key} {asset}: missing colour or normal ({sorted(maps)})")
    color = load(maps["color"], "RGB")
    size = color.size
    color.save(os.path.join(d, "Color.png"))
    load(maps["normal"], "RGB").resize(size).save(os.path.join(d, "Normal.png"))
    grey = lambda k, v: load(maps[k], "L").resize(size) if k in maps else Image.new("L", size, v)  # noqa: E731
    Image.merge("RGB", (grey("ao", 255), grey("rough", 160), grey("metal", 0))).save(os.path.join(d, "ORM.png"))
    if "height" in maps:
        grey("height", 128).save(os.path.join(d, "Height.png"))
    with open(os.path.join(d, "SET.txt"), "w", encoding="utf-8") as f:
        f.write(f"{key}\t{src}\t{asset}\t{res}\t{scale}\t{what}\n")
    print(f"{key} {asset} {res}: {size[0]}x{size[1]}, maps {sorted(maps)}")


def _blur(x, sigma_px, sigma_y=None):
    """Gaussian blur that wraps (the sets tile)."""
    import cv2
    import numpy as np
    sy = sigma_px if sigma_y is None else sigma_y
    p = int(max(sigma_px, sy) * 3) + 1
    y = np.pad(x, p, mode="wrap")
    return cv2.GaussianBlur(y, (0, 0), sigmaX=sigma_px, sigmaY=sy)[p:-p, p:-p]


def derive_travertine(key="S1", depth_mm=2.5):
    """
    Travertine009 is a polished, filled slab: its normal is flat and its height empty, so on its own it can't give the
    stone relief. Its colour still records where the voids are: vein-cut travertine's pores are the thin dark lenses
    along the bedding. They are found as darkness against the 15 mm neighbourhood, joined along the bedding into lenses
    2-30 mm long (about 4 % of the face, as on honed unfilled travertine), and written as
    - Height: the voids 2.5 mm deep under a faint honed relief from the stone's own tone (harder bands stand proud);
    - Normal (DirectX): from that height;
    - ORM: AO 1 - 0.7 x void (M_Stone reads 1 - AO as the void / filler mask), roughness 0.5 honed with the voids 0.9
      (walls: open; floors: filled, where the filler reads through the polish), metal 0.
    The shipped maps are kept in <key>/source/.
    """
    import cv2
    import numpy as np
    d = os.path.join(OUT, key)
    src = os.path.join(d, "source")
    os.makedirs(src, exist_ok=True)
    for kind in ("Normal", "ORM", "Height"):
        if os.path.exists(os.path.join(d, f"{kind}.png")) and not os.path.exists(os.path.join(src, f"{kind}.png")):
            os.replace(os.path.join(d, f"{kind}.png"), os.path.join(src, f"{kind}.png"))
    if not os.path.exists(os.path.join(src, "Color.png")):           # the photograph as shipped, kept once
        Image.open(os.path.join(d, "Color.png")).save(os.path.join(src, "Color.png"))
    c = np.asarray(Image.open(os.path.join(src, "Color.png")).convert("RGB"), np.float32) / 255
    n_px = c.shape[0]
    mm = SETS[key][4] * 1000.0 / n_px            # millimetres per pixel
    lum = ((c ** 2.2) @ np.array([0.2126, 0.7152, 0.0722], np.float32)) ** (1 / 2.2)
    local = _blur(lum, 15.0 / mm)
    dark = (local - lum) / np.maximum(local, 0.05)
    void = np.clip((dark - 0.045) / 0.03, 0, 1)
    void = _blur(void, 3.0 / mm, 0.7 / mm)       # along the bedding (image x)
    void = np.clip((void - 0.18) / 0.22, 0, 1)
    # Break the long ones into lenses: a noise stretched along the bedding (4 cm x 6 mm) keeps about half of each line.
    rng = np.random.default_rng(7)
    lens = _blur(rng.standard_normal(void.shape).astype(np.float32), 40.0 / mm, 6.0 / mm)
    lens = (lens - lens.mean()) / (lens.std() + 1e-6)
    void = void * np.clip((lens + 0.7) / 0.5, 0, 1)
    void = _blur(void, 0.35 / mm)
    # A real void is a lens a few centimetres long. Where the photograph's dark lines run on for decimetres (one wavy
    # vein crosses the whole 1.2 m slab), the lenses joined up into one open crack that every block and slab then
    # showed again: the "darkest vein in every block". Such runs (joined across 6 x 2 mm gaps, wider than 4-8 cm) keep
    # only short lenses (1-3 cm, about a third of their length), and the photograph's dark line is filled there with
    # the stone's tone around it.
    import cv2
    m = (void > 0.25).astype(np.uint8)
    joined = cv2.dilate(m, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (int(6 / mm) | 1, int(2 / mm) | 1)))
    count, labels, st, _ = cv2.connectedComponentsWithStats(joined, 8)
    width_mm = st[:, cv2.CC_STAT_WIDTH] * mm
    runs = np.clip((width_mm - 40.0) / 40.0, 0, 1).astype(np.float32)
    runs[0] = 0.0
    long_run = _blur(runs[labels] * (joined > 0), 2.0 / mm)
    chop = _blur(np.random.default_rng(3).standard_normal(void.shape).astype(np.float32), 9.0 / mm, 3.0 / mm)
    chop = np.clip((chop / (chop.std() + 1e-6) - 0.45) / 0.35, 0, 1)
    void = void * (1.0 - long_run * (1.0 - chop))
    filled = long_run * (1.0 - chop)                  # where a long dark line was taken out
    lift = np.clip(local / np.maximum(lum, 0.05), 1.0, 1.6)
    c = c * (1.0 + (lift[..., None] - 1.0) * filled[..., None])
    c = _tame_bands(c, mm)
    Image.fromarray((np.clip(c, 0, 1) * 255 + 0.5).astype(np.uint8)).save(os.path.join(d, "Color.png"))
    print(f"{key}: {int((width_mm[1:] > 60).sum())} long vein runs broken into lenses; colour's broad bands tamed")
    fine = _blur(lum, 0.5 / mm) - _blur(lum, 6.0 / mm)
    h = 1.0 - void + fine * 0.6
    h = (h - h.min()) / (h.max() - h.min())
    h_mm = h * depth_mm
    gx = cv2.Sobel(h_mm, cv2.CV_32F, 1, 0, ksize=3, borderType=cv2.BORDER_REFLECT) / (8 * mm)
    gy = cv2.Sobel(h_mm, cv2.CV_32F, 0, 1, ksize=3, borderType=cv2.BORDER_REFLECT) / (8 * mm)
    nrm = np.dstack([-gx, -gy, np.ones_like(gx)])  # DirectX: green is the component towards the image's bottom
    nrm /= np.linalg.norm(nrm, axis=2, keepdims=True)
    to8 = lambda a: (np.clip(a, 0, 1) * 255 + 0.5).astype(np.uint8)  # noqa: E731
    Image.fromarray(to8(nrm * 0.5 + 0.5)).save(os.path.join(d, "Normal.png"))
    rough = 0.5 + (fine * -1.5) + void * 0.4
    Image.merge("RGB", [Image.fromarray(to8(1.0 - 0.7 * void)), Image.fromarray(to8(rough)),
                        Image.new("L", (n_px, n_px), 0)]).save(os.path.join(d, "ORM.png"))
    Image.fromarray(to8(h)).save(os.path.join(d, "Height.png"))
    tilt = np.hypot(nrm[..., 0], nrm[..., 1])
    print(f"{key}: travertine voids derived: {void.mean() * 100:.1f} % of the face, tilt mean {tilt.mean():.3f}")


def _tame_bands(c, mm, strength=0.7, along_mm=100.0, across_mm=6.0):
    """
    Travertine009's photograph is one 1.2 m slab with a few broad bands, the darkest a third of the way down: every block
    and slab takes its own slice of it, but each still carried the same band. The bands are the tone averaged along the
    bedding (10 cm) over a few millimetres across it; the colour (sRGB 0-1 array) is divided by that tone ^ strength, so
    the broad bands fade to a breath while the fine veining, the pores and the voids stay. M_Stone's BlockTone and
    BlockHue give back a real wall's variation, at random per block instead of the one band repeated.
    """
    import numpy as np
    lin = np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)
    lum = lin @ np.array([0.2126, 0.7152, 0.0722], np.float32)
    band = _blur(lum, along_mm / mm, across_mm / mm) / lum.mean()
    out = lin * np.clip(band ** -strength, 0.75, 1.35)[..., None]
    return np.where(out <= 0.0031308, out * 12.92, 1.055 * np.power(np.clip(out, 0, 1), 1 / 2.4) - 0.055)


def make_waviness(key="WV", size=1024):
    """
    WV/Normal.png: the macro undulation every master can lay over its surface (M_Stone's slabs, trowelled plaster,
    hand-laid brick), as a tileable normal map: a smooth random height field with four wavelengths across the tile,
    its slope normalised (RMS 1) and stored at a quarter (the material scales it to degrees). One texture fetch in place
    of an ALU Perlin gradient (~100 instructions a pixel).
    """
    import numpy as np
    rng = np.random.default_rng(11)
    f = np.fft.fftfreq(size)
    fx, fy = np.meshgrid(f, f)
    k = np.hypot(fx, fy) * size                       # cycles per tile
    spectrum = np.exp(-((k - 4.0) / 2.5) ** 2) + 0.35 * np.exp(-((k - 11.0) / 4.0) ** 2)
    h = np.real(np.fft.ifft2(np.fft.fft2(rng.standard_normal((size, size))) * spectrum))
    gx = np.real(np.fft.ifft2(np.fft.fft2(h) * (2j * np.pi * fx)))
    gy = np.real(np.fft.ifft2(np.fft.fft2(h) * (2j * np.pi * fy)))
    rms = np.sqrt((gx ** 2 + gy ** 2).mean() / 2)
    gx, gy = gx / rms * 0.25, gy / rms * 0.25
    nrm = np.dstack([-gx, -gy, np.sqrt(np.clip(1 - gx ** 2 - gy ** 2, 0, 1))])   # DirectX
    d = os.path.join(OUT, key)
    os.makedirs(d, exist_ok=True)
    Image.fromarray((np.clip(nrm * 0.5 + 0.5, 0, 1) * 255 + 0.5).astype(np.uint8)).save(os.path.join(d, "Normal.png"))
    with open(os.path.join(d, "SET.txt"), "w", encoding="utf-8") as fh:
        fh.write("\t".join((key, "generated", "make_waviness", str(size), "-", "macro undulation (tileable normal)")) + "\n")
    print(f"{key}: waviness normal written ({size}^2)")


DERIVED = {"S1": derive_travertine}


NOTES = """
## Derived maps

- **S1**: Normal, ORM and Height are derived from Travertine009's own colour by `derive_travertine()`. The shipped maps
  (flat: a polished, filled slab) are kept in `S1/source/`. The voids are the dark lenses along the bedding, 2.5 mm
  deep, about 4 % of the face. Same licence (CC0 1.0).

## Checked and not used (2026-09-25)

- ambientCG Tiles139, Tiles142, Tiles143, Tiles144: limestone tiles with 33 cm joints baked in; normals flat (mean
  tilt 0.006-0.014); Tiles139's height is only a synthetic ramp per tile.
- Poly Haven floor_tiles_02 and floor_tiles_04: worn tiles with their own joints (floor_tiles_04's normal map is
  biased, mean tilt 0.30).
- Poly Haven plastered_wall_02: a good trowelled grain, but a panel groove every 0.55 m (new stitch lines).
- Poly Haven oak_veneer_02 to 05, red_oak_veneer, mocha_oak_veneer, white_oak_veneer, silver_oak_veneer_01, and
  ambientCG Wood049: paler, repeating every 0.5-1 m, or weathered sawn wood. oak_veneer_01 (W2) has the figure and
  the pore bands.
"""


def credits():
    rows = []
    for key in sorted(SETS):
        src, asset, res, what, scale = SETS[key]
        url = f"https://ambientcg.com/view?id={asset}" if src == "ambientcg" else f"https://polyhaven.com/a/{asset}"
        rows.append(f"| {key} | {asset} ({'ambientCG' if src == 'ambientcg' else 'Poly Haven'}, {res}) | {what} | {scale} m | {url} | CC0 1.0 |")
    with open(os.path.join(OUT, "CREDITS.md"), "w", encoding="utf-8") as f:
        f.write("# PBR texture library: credits\n\nFetched by `windows/Scripts/pbr_fetch.py`. All CC0 1.0 (public domain).\n\n"
                "| key | set | stands for | scale | source | licence |\n|---|---|---|---|---|---|\n" + "\n".join(rows) + "\n" + NOTES)



def srgb_to_linear(c):
    c = c / 255.0
    return ((c + 0.055) / 1.055) ** 2.4 if c > 0.04045 else c / 12.92


def stats():
    """SourceArt/PBR/stats.json: each set's mean colour (linear) and mean roughness, so an instance can bring
    the photograph to the spec's albedo (BaseColor / mean) and keep its roughness where the spec wants it."""
    import json
    out = {}
    for key in sorted(SETS):
        d = os.path.join(OUT, key)
        if not os.path.exists(os.path.join(d, "Color.png")):
            continue
        c = Image.open(os.path.join(d, "Color.png")).convert("RGB").resize((256, 256), Image.BILINEAR)
        px = list(c.getdata())
        mean = [sum(srgb_to_linear(p[i]) for p in px) / len(px) for i in range(3)]
        orm = Image.open(os.path.join(d, "ORM.png")).convert("RGB").resize((256, 256), Image.BILINEAR)
        g = [p[1] / 255.0 for p in orm.getdata()]
        out[key] = {"color_mean": [round(v, 4) for v in mean], "rough_mean": round(sum(g) / len(g), 4),
                    "scale": SETS[key][4], "has_height": os.path.exists(os.path.join(d, "Height.png"))}
    with open(os.path.join(OUT, "stats.json"), "w", encoding="utf-8") as f:
        json.dump(out, f, indent=1)
    print(f"stats for {len(out)} sets")


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    for k in args or list(SETS):
        try:
            fetch(k)
        except Exception as e:  # noqa: BLE001
            print(f"{k}: FAILED {e}")
    if not os.path.exists(os.path.join(OUT, "WV", "Normal.png")) or "--wave" in sys.argv:
        make_waviness()
    # Derived relief (--derive re-derives even when it is already there).
    for k, fn in DERIVED.items():
        if (not args or k in args) and ("--derive" in sys.argv or not os.path.exists(os.path.join(OUT, k, "source"))):
            fn(k)
    credits()
    stats()
