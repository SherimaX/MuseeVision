"""
Albion's own texture sets (plan/proposals/albion), made here because no CC0 set is close enough: plain Python (numpy,
PIL, scipy, OpenCV), outside Unreal.

    python windows/Scripts/albion_textures.py [tile] [border] [tablets] [stones]      (all without arguments)

Each set is written as the museum's PBR library writes them (SourceArt/PBR/<KEY>/Color.png, Normal.png (DirectX),
ORM.png (AO, roughness, metal), Height.png), and its statistics to SourceArt/PBR/albion_stats.json (albion_materials.py
adds them to pbr.py's table at run time; pbr_fetch.py's own stats.json is left alone). Every set tiles.

- AL_TILE: the nave's and porch's encaustic floor after Minton and Pugin (6-inch tiles laid on the diagonal, red, buff and
  black): 2 × 2 encaustic tiles make a roundel (a ring, a quatrefoil, a flower, fleurons in the spandrels), each group
  framed by a line of plain black tiles with a buff square where the lines cross. 2 mm joints of dark grout, each tile
  its own tone, a worn arris, chips, the inlay's own slightly different surface. One repeat is 2.586 m square (4 groups of
  three tiles along each diagonal).
- AL_BORDER: the black border round the fields: a 1½-inch black fillet, a row of 6-inch encaustic border tiles (a running
  trefoil in buff on red), a buff fillet, a chequer of 3-inch black and red squares; 0.3048 m square (two border tiles),
  its outer edge along the image's bottom.
- AL_RUSKIN, AL_NAME: the two inscribed tablets, V-cut letters in Centaur capitals (Bruce Rogers's Jenson roman: the
  face Morris's Golden type came from): "REJECTING NOTHING · SELECTING NOTHING · SCORNING NOTHING" (Ruskin, Modern
  Painters I, 1843) and "ALBION / 1848 · 1898".
- AL_ST_<stone>: the ten polished British stones of the columns (the Details board), each drawn from what the stone is:
  serpentine's mottled green-black and veins, the granites' crystals, Purbeck's and Frosterley's fossils, Hopton Wood's
  cream, Ashburton's red and white veins, Iona's green streaks, Tiree's pink with its dark flecks, Mona's green with
  white veins and red. 1.571 m square (once round a 0.5 m shaft).
"""
import json
import math
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont
from scipy import ndimage
from scipy.spatial import cKDTree

HERE = os.path.dirname(os.path.abspath(__file__))
PBR = os.path.normpath(os.path.join(HERE, "..", "SourceArt", "PBR"))
STATS = os.path.join(PBR, "albion_stats.json")
FONT = r"C:\Windows\Fonts\CENTAUR.TTF"


def srgb(c):
    """sRGB hex → linear float triple."""
    c = np.array([(c >> 16) & 255, (c >> 8) & 255, c & 255], dtype=np.float64) / 255.0
    return np.where(c > 0.04045, ((c + 0.055) / 1.055) ** 2.4, c / 12.92)


def to_srgb8(lin):
    lin = np.clip(lin, 0.0, 1.0)
    s = np.where(lin > 0.0031308, 1.055 * np.power(lin, 1 / 2.4) - 0.055, lin * 12.92)
    return (np.clip(s, 0, 1) * 255 + 0.5).astype(np.uint8)


def periodic_noise(n, scale_px, seed, octaves=4, persistence=0.5):
    """Tileable fractal noise in [-1, 1]: white noise filtered to features of about scale_px, octaves finer."""
    rng = np.random.default_rng(seed)
    fy = np.fft.fftfreq(n)[:, None]
    fx = np.fft.fftfreq(n)[None, :]
    f = np.sqrt(fx * fx + fy * fy)
    out = np.zeros((n, n))
    amp, total = 1.0, 0.0
    s = scale_px
    for _ in range(octaves):
        w = rng.standard_normal((n, n))
        f0 = 1.0 / max(s, 1.0)
        band = np.exp(-((f / f0) ** 2)) * (1 - np.exp(-((f / (f0 * 0.25)) ** 2)))
        layer = np.real(np.fft.ifft2(np.fft.fft2(w) * band))
        layer /= np.std(layer) + 1e-9
        out += amp * layer
        total += amp
        amp *= persistence
        s *= 0.5
    out /= total
    return np.clip(out / 2.5, -1, 1)


def voronoi(n, count, seed, jitter=1.0):
    """A tileable Voronoi: (cell id, distance to own site, distance to the border) per pixel."""
    rng = np.random.default_rng(seed)
    pts = rng.random((count, 2)) * n
    tiles = np.concatenate([pts + np.array([dx, dy]) * n for dx in (-1, 0, 1) for dy in (-1, 0, 1)])
    ids = np.tile(np.arange(count), 9)
    tree = cKDTree(tiles)
    yy, xx = np.mgrid[0:n, 0:n]
    q = np.stack([xx.ravel() + 0.5, yy.ravel() + 0.5], axis=1)
    d, i = tree.query(q, k=2)
    cell = ids[i[:, 0]].reshape(n, n)
    d1 = d[:, 0].reshape(n, n)
    edge = (d[:, 1] - d[:, 0]).reshape(n, n) * 0.5
    return cell, d1, edge


def normal_from_height(h, strength_px):
    """DirectX normal map (green down) from a height field in pixels' units × strength, wrapping at the edges."""
    gx = (np.roll(h, -1, axis=1) - np.roll(h, 1, axis=1)) * 0.5 * strength_px
    gy = (np.roll(h, -1, axis=0) - np.roll(h, 1, axis=0)) * 0.5 * strength_px
    nx, ny, nz = -gx, gy, np.ones_like(h)       # DirectX: +y of the normal points to the image's top: green = -dh/dy (image down)
    l = np.sqrt(nx * nx + ny * ny + nz * nz)
    n = np.stack([nx / l, -ny / l, nz / l], axis=-1)
    return ((n * 0.5 + 0.5) * 255 + 0.5).astype(np.uint8)


def write_set(key, colour_lin, height, rough, ao=None, strength=1.0, scale=1.0, describe=""):
    """Writes a set and returns its statistics (linear colour mean, mean roughness)."""
    d = os.path.join(PBR, key)
    os.makedirs(d, exist_ok=True)
    Image.fromarray(to_srgb8(colour_lin)).save(os.path.join(d, "Color.png"))
    Image.fromarray(normal_from_height(height, strength)).save(os.path.join(d, "Normal.png"))
    if ao is None:
        ao = np.ones_like(rough)
    orm = np.stack([ao, rough, np.zeros_like(rough)], axis=-1)
    Image.fromarray((np.clip(orm, 0, 1) * 255 + 0.5).astype(np.uint8)).save(os.path.join(d, "ORM.png"))
    hn = (height - height.min()) / max(1e-9, height.max() - height.min())
    Image.fromarray((hn * 255 + 0.5).astype(np.uint8), "L").save(os.path.join(d, "Height.png"))
    with open(os.path.join(d, "SET.txt"), "w", encoding="utf-8") as f:
        f.write(f"{key}\tgenerated\talbion_textures.py\t{colour_lin.shape[1]}\t{scale}\t{describe}\n")
    mean = [float(np.mean(colour_lin[..., i])) for i in range(3)]
    st = {"color_mean": [round(max(m, 1e-4), 4) for m in mean], "rough_mean": round(float(np.mean(rough)), 4), "scale": scale,
          "has_height": True}
    print(f"{key}: mean {st['color_mean']} rough {st['rough_mean']} scale {scale}")
    return st


# ======================================================================================== the encaustic tiles

P6 = 0.1524                   # a six-inch tile
TILE_REPEAT = 4 * 3 * P6 * math.sqrt(2.0)   # 2.586 m: four groups of three tiles along each diagonal
# (Muted, as Minton's fired clays are after a century of wax and feet: a brown-red, a greyed buff, a soft black.)
RED, BUFF, BLACK, GROUT = srgb(0x6E3B30), srgb(0xB29C7A), srgb(0x2B2826), srgb(0x4A443D)


def motif_quarter(u, v, ci, cj):
    """
    One encaustic tile of the 2 × 2 roundel; (u, v) in the tile (0 … 1), (ci, cj) the roundel's centre corner. Returns
    the inlay index per pixel: 0 red ground, 1 buff, 2 black.
    """
    du, dv = u - ci, v - cj
    r = np.sqrt(du * du + dv * dv)
    th = np.arctan2(dv, du)
    out = np.zeros(u.shape, dtype=np.int8)
    # The outer ring (buff, a black line inside it) and a beaded edge of buff dots outside it.
    out[(r > 0.86) & (r < 0.95)] = 1
    out[(r > 0.83) & (r <= 0.86)] = 2
    dots = ((r > 0.99) & (r < 1.05)) & (np.cos(th * 24) > 0.55)
    out[dots] = 1
    # The quatrefoil: four lobes, buff, outlined black.
    q = 0.52 + 0.16 * np.cos(4 * th)
    out[(r < q) & (r > q - 0.03)] = 2
    out[(r < q - 0.03) & (r > 0.24)] = 1
    # A small trefoil at each lobe's tip, in red within the buff.
    tip = (np.abs(np.cos(4 * th)) > 0.985) & (np.abs(r - 0.56) < 0.035)
    out[tip & (np.cos(4 * th) > 0)] = 0
    # The flower in the middle: four petals of red, a black eye.
    petal = 0.22 * np.abs(np.cos(2 * th)) ** 0.7
    out[(r < petal)] = 0
    out[r < 0.07] = 2
    # Fleurons in the spandrels (towards the tile's far corner): a trefoil in buff.
    fu, fv = (1 - ci) - u, (1 - cj) - v            # from the far corner
    fr = np.sqrt(fu * fu + fv * fv)
    fth = np.arctan2(fv * np.sign(1 - 2 * cj + 1e-9), fu * np.sign(1 - 2 * ci + 1e-9))
    tre = fr < (0.20 + 0.07 * np.cos(3 * (fth - math.pi / 4)))
    out[tre & (fr > 0.035)] = 1
    out[(fr < 0.035)] = 2
    return out


def tile_field(n=4096):
    S = TILE_REPEAT
    px = S / n
    colour = np.zeros((n, n, 3))
    height = np.zeros((n, n))
    rough = np.zeros((n, n))
    ao = np.ones((n, n))
    rng = np.random.default_rng(7)
    tone = rng.normal(0.0, 0.055, (64, 64))           # per tile (indexed by lattice cell mod 64)
    hue = rng.normal(0.0, 0.02, (64, 64))
    tilt = rng.normal(0.0, 1.0, (64, 64, 2))
    wear = rng.random((64, 64))
    grain = periodic_noise(n, 3.0, 11, octaves=3)
    blotch = periodic_noise(n, 80.0, 12, octaves=3)
    chipn = periodic_noise(n, 6.0, 13, octaves=2)
    mottle = periodic_noise(n, 8.0, 14, octaves=3)
    speck = periodic_noise(n, 0.9, 15, octaves=1)
    for y0 in range(0, n, 512):
        yy, xx = np.mgrid[y0:y0 + 512, 0:n].astype(np.float64)
        X = (xx + 0.5) * px
        Y = (yy + 0.5) * px
        a = (X + Y) / (P6 * math.sqrt(2.0))
        b = (Y - X) / (P6 * math.sqrt(2.0))
        ia, ib = np.floor(a).astype(np.int64), np.floor(b).astype(np.int64)
        u, v = a - ia, b - ib
        i3, j3 = np.mod(ia, 3), np.mod(ib, 3)
        kind = np.full(u.shape, 0, dtype=np.int8)     # the ink index
        motif = (i3 < 2) & (j3 < 2)
        ci = np.where(i3 == 0, 1.0, 0.0)
        cj = np.where(j3 == 0, 1.0, 0.0)
        m = motif_quarter(u, v, ci, cj)
        kind = np.where(motif, m, kind)
        line = ~motif
        cross = (i3 == 2) & (j3 == 2)
        # The lines between the roundels: plain tiles, black and a darker red by turns.
        alt = np.mod(ia + ib, 2) == 0
        kind = np.where(line & ~cross, np.where(alt, 2, 3), kind)
        # The crossings: buff squares with a small red diamond.
        dmd = (np.abs(u - 0.5) + np.abs(v - 0.5)) < 0.22
        kind = np.where(cross, np.where(dmd, 0, 1), kind)
        inks = np.stack([RED, BUFF, BLACK, RED * 0.78])
        c = inks[kind]
        # The clay: a mottle of the body (5 mm), grog specks, and the inlay's edge a little soft where it was pressed.
        mot = mottle[y0:y0 + 512][..., None]
        spk = speck[y0:y0 + 512]
        c = c * (1.0 + 0.05 * mot)
        c = np.where((spk > 0.72)[..., None], c * 0.82, np.where((spk < -0.74)[..., None], c * 1.12, c))
        # Each tile its own firing: tone and a breath of hue; the inlay (buff) a touch paler and duller.
        ta, tb = np.mod(ia, 64), np.mod(ib, 64)
        t = tone[ta, tb][..., None]
        h = hue[ta, tb][..., None]
        c = c * (1.0 + t) * (1.0 + np.concatenate([h, np.zeros_like(h), -h], axis=-1))
        g = grain[y0:y0 + 512]
        bl = blotch[y0:y0 + 512]
        c = c * (1.0 + 0.05 * g[..., None] + 0.05 * bl[..., None])
        # Joints: 2 mm, dark grout, recessed 1 mm; a worn, rounded arris 1.5 mm; chips at some corners.
        edge = np.minimum(np.minimum(u, 1 - u), np.minimum(v, 1 - v)) * P6       # metres to the tile's edge
        joint = edge < 0.001
        arris = np.clip((edge - 0.001) / 0.0015, 0, 1)
        chip = (edge < 0.004) & (chipn[y0:y0 + 512] > 0.62)
        surf = 0.0012 * np.sin(np.pi * np.clip(u, 0, 1)) * np.sin(np.pi * np.clip(v, 0, 1))   # a slight dome
        tl = tilt[ta, tb]
        surf = surf + 0.00025 * ((u - 0.5) * tl[..., 0] + (v - 0.5) * tl[..., 1])
        hgt = np.where(joint, -0.001, surf - 0.0006 * (1 - arris) ** 2 - np.where(chip, 0.0012, 0.0))
        inlay = ((kind == 1) | (kind == 2)) & motif
        hgt = hgt - 0.00005 * inlay
        c = np.where(joint[..., None], GROUT * (1.0 + 0.1 * g[..., None]), c)
        c = np.where(chip[..., None] & ~joint[..., None], c * 0.8 + srgb(0x9C8470) * 0.2, c)
        # Roughness: waxed and worn (0.42 … 0.55), each tile its own, the inlay a little duller, the grout matt.
        r = 0.45 + 0.08 * (wear[ta, tb] - 0.5) + 0.05 * inlay + 0.04 * g
        r = np.where(joint, 0.9, np.where(chip, 0.8, r))
        colour[y0:y0 + 512] = c
        height[y0:y0 + 512] = hgt
        rough[y0:y0 + 512] = r
        ao[y0:y0 + 512] = np.where(joint, 0.45, 0.75 + 0.25 * arris)
    return write_set("AL_TILE", colour, height / px, rough, ao, strength=1.0, scale=round(TILE_REPEAT, 4),
                     describe="encaustic tiles on the diagonal, after Minton and Pugin")


def border(n=2048):
    """0.3048 m square: the band across V (the image's bottom row = the band's outer edge), two border tiles along U."""
    S = 0.3048
    px = S / n
    yy, xx = np.mgrid[0:n, 0:n].astype(np.float64)
    U = (xx + 0.5) * px                      # along the band
    V = (n - (yy + 0.5)) * px                # across it, from the outer edge
    colour = np.zeros((n, n, 3))
    grain = periodic_noise(n, 3.0, 21, 3)
    joints = np.zeros((n, n), dtype=bool)
    r = np.full((n, n), 0.47)
    # Zones across the band.
    fillet1 = V < 0.038
    enc = (V >= 0.038) & (V < 0.1904)
    fillet2 = (V >= 0.1904) & (V < 0.2285)
    chq = V >= 0.2285
    colour[fillet1] = BLACK
    colour[fillet2] = BUFF
    # The encaustic border tiles: a running vine in buff on red, a trefoil leaf at each crest and trough, berries in
    # the hollows, a buff line along each edge, all outlined as the inlay is.
    tu = np.mod(U, P6) / P6
    tv = (V - 0.038) / P6
    k = np.zeros((n, n), dtype=np.int8)
    stem = 0.5 + 0.2 * np.sin(2 * np.pi * tu)
    k[np.abs(tv - stem) < 0.035] = 1
    for cx, cy, sgn in ((0.25, 0.70, 1.0), (0.75, 0.30, -1.0)):
        for ang in (-50.0, 0.0, 50.0):
            a_ = math.radians(90.0 * sgn + ang)
            lx, ly = cx + 0.11 * math.cos(a_), cy + 0.11 * math.sin(a_)
            k[((tu - lx) ** 2 + ((tv - ly) * 1.0) ** 2) < 0.052 ** 2] = 1
        k[((tu - cx) ** 2 + (tv - cy) ** 2) < 0.02 ** 2] = 2
    for cx, cy in ((0.25, 0.28), (0.75, 0.72), (0.0, 0.5), (1.0, 0.5)):
        for dx in (-0.045, 0.045):
            k[((tu - cx - dx) ** 2 + (tv - cy) ** 2) < 0.028 ** 2] = 1
    k[(np.abs(tv - 0.12) < 0.018) | (np.abs(tv - 0.88) < 0.018)] = 1
    k[(tv < 0.06) | (tv > 0.94)] = 2
    ink = np.stack([RED, BUFF, BLACK])[k]
    colour[enc] = ink[enc]
    # The chequer: 3-inch squares, black and red.
    cu = np.floor(U / (P6 / 2)).astype(int)
    cv = np.floor((V - 0.2285) / (P6 / 2)).astype(int)
    chk = np.where((cu + cv) % 2 == 0, 0, 1)
    colour[chq] = np.stack([BLACK, RED])[chk][chq]
    # Joints between every tile.
    def jline(val, pitch, offset=0.0):
        m = np.mod(val - offset, pitch)
        return np.minimum(m, pitch - m) < 0.001
    joints |= (np.abs(V - 0.038) < 0.001) | (np.abs(V - 0.1904) < 0.001) | (np.abs(V - 0.2285) < 0.001)
    joints |= (fillet1 | fillet2) & jline(U, P6 / 2)
    joints |= enc & jline(U, P6)
    joints |= chq & jline(U, P6 / 2)
    rng = np.random.default_rng(3)
    colour = colour * (1.0 + 0.05 * grain[..., None])
    colour = np.where(joints[..., None], GROUT, colour)
    h = np.where(joints, -0.001, 0.0) / px
    r = np.where(joints, 0.9, r + 0.04 * grain)
    ao = np.where(joints, 0.45, 1.0)
    return write_set("AL_BORDER", colour, h, r, ao, strength=1.0, scale=S, describe="the black border of the encaustic fields")


# ======================================================================================== the tablets

def tablet(key, width_m, height_m, lines, px_per_m=880, describe=""):
    W, H = int(width_m * px_per_m), int(height_m * px_per_m)
    img = Image.new("L", (W, H), 0)
    d = ImageDraw.Draw(img)
    for text, size_m, y_m, track in lines:
        font = ImageFont.truetype(FONT, int(size_m * px_per_m))
        # Tracked capitals, centred.
        widths = [d.textlength(ch, font=font) for ch in text]
        total = sum(widths) + track * px_per_m * (len(text) - 1)
        x = (W - total) / 2
        for ch, w in zip(text, widths):
            if ch == "·":
                # Centaur has no interpunct: a small cut lozenge at the middle of the capitals' height.
                cx, cy, rr = x + w * 0.5, y_m * px_per_m - size_m * px_per_m * 0.33, size_m * px_per_m * 0.07
                d.polygon([(cx, cy - rr * 1.3), (cx + rr, cy), (cx, cy + rr * 1.3), (cx - rr, cy)], fill=255)
            else:
                d.text((x, y_m * px_per_m), ch, font=font, fill=255, anchor="ls")
            x += w + track * px_per_m
    mask = np.asarray(img).astype(np.float64) / 255.0
    inside = mask > 0.5
    # V-cut: depth grows with the distance in from the letter's edge, to a chisel's line.
    dist = ndimage.distance_transform_edt(inside)
    depth_px = 0.006 * px_per_m
    cut = np.clip(dist / max(1.0, dist.max() * 0.5), 0, 1)
    hgt = -np.minimum(dist, depth_px) / depth_px * 0.006 * px_per_m * 0.9
    reps = (H // 1024 + 1, W // 1024 + 1)
    stone = np.tile(periodic_noise(1024, 60.0, 31, 4), reps)[:H, :W]
    fine = np.tile(periodic_noise(1024, 2.0, 32, 2), reps)[:H, :W]
    base = srgb(0xD8CBB2)
    col = base[None, None, :] * (1.0 + 0.04 * stone[..., None] + 0.02 * fine[..., None])
    # The cuts hold a little dust: slightly darker, as they do.
    col = col * (1.0 - 0.12 * inside[..., None] * (0.5 + 0.5 * cut[..., None]))
    rough = 0.62 + 0.05 * fine + 0.1 * inside
    ao = 1.0 - 0.35 * inside * cut
    hgt = hgt + 0.3 * fine
    return write_set(key, col, hgt, rough, ao, strength=1.0, scale=1.0, describe=describe)


# ======================================================================================== the ten stones

def stones(n=2048):
    S = 1.5708     # m: once round a 0.5 m shaft
    ppm = n / S
    out = {}

    def finish(key, col, rough, relief=None, describe=""):
        h = relief if relief is not None else periodic_noise(n, 2.0, sum(map(ord, key)), 2) * 0.3
        out[key] = write_set(key, col, h, rough, None, strength=0.6, scale=S, describe=describe)

    def mix(*pairs):
        acc = 0.0
        for w, c in pairs:
            acc = acc + w[..., None] * c[None, None, :]
        return acc

    one = np.ones((n, n))
    # A · Lizard serpentine: dark green-black, mottled with dull red, a net of pale green veins; highly polished.
    m1 = periodic_noise(n, 90, 101, 5)
    m2 = periodic_noise(n, 30, 102, 4)
    veins = np.exp(-np.abs(periodic_noise(n, 140, 103, 4) * 14.0))
    veins2 = np.exp(-np.abs(periodic_noise(n, 60, 104, 3) * 22.0))
    red = np.clip((m1 - 0.15) * 2.5, 0, 1) * np.clip(m2 * 0.5 + 0.7, 0, 1)
    col = mix((one, srgb(0x243326)), (red * 0.8, srgb(0x5A2A22) - srgb(0x243326)))
    col = col * (1 + 0.25 * m2[..., None])
    col = col + (veins * 0.7 + veins2 * 0.35)[..., None] * (srgb(0x87A383) - col)
    finish("AL_ST_Serpentine", col, 0.07 + 0.03 * veins + 0.02 * m2, describe="Lizard serpentine, Cornwall")

    # B · Peterhead granite: pink-red feldspar crystals (0.5 … 1.5 cm), glassy grey quartz, black biotite.
    def granite(key, feld, feld2, quartz, mica, size_mm, frac_q, frac_m, seed, describe):
        count = int((S * 1000 / size_mm) ** 2 * 1.2)
        cell, d1, edge = voronoi(n, count, seed)
        rng = np.random.default_rng(seed + 1)
        kind = rng.random(count)
        shade = rng.normal(0, 0.12, count)
        k = kind[cell]
        c = np.where((k < frac_m)[..., None], mica[None, None, :],
                     np.where((k < frac_m + frac_q)[..., None], quartz[None, None, :],
                              np.where((k < frac_m + frac_q + (1 - frac_m - frac_q) * 0.35)[..., None], feld2[None, None, :], feld[None, None, :])))
        c = c * (1 + shade[cell][..., None])
        tex = periodic_noise(n, 1.5, seed + 2, 2)
        c = c * (1 + 0.08 * tex[..., None])
        # Cleavage lines across the feldspars; crystal boundaries darker.
        c = c * (1 - 0.25 * np.exp(-edge / 1.2)[..., None])
        r = np.where(k < frac_m, 0.16, np.where(k < frac_m + frac_q, 0.05, 0.08)) + 0.02 * tex
        relief = -np.exp(-edge / 1.0) * 0.6 + np.where(k < frac_m, -0.3, 0.0)
        out[key] = write_set(key, c, relief, r, None, strength=0.5, scale=S, describe=describe)

    granite("AL_ST_Peterhead", srgb(0xB86A58), srgb(0xC98A74), srgb(0x8C8580), srgb(0x1C1A19), 9.0, 0.25, 0.08, 201,
            "Peterhead granite, Aberdeenshire")
    # C · Rubislaw granite: grey, finer: white feldspar, grey quartz, black mica.
    granite("AL_ST_Rubislaw", srgb(0xB9B6B0), srgb(0xCFCBC4), srgb(0x7F7D7A), srgb(0x1E1E1E), 4.0, 0.3, 0.14, 301,
            "Rubislaw granite, Aberdeen")

    # Fossil marbles: a matrix with shells (Purbeck: Viviparus snails, whorled sections) or corals (Frosterley).
    def fossils(key, matrix, shell, rim, count, rmin, rmax, seed, coral=False, describe=""):
        rng = np.random.default_rng(seed)
        mat = periodic_noise(n, 70, seed + 1, 4)
        col = matrix[None, None, :] * (1 + 0.15 * mat[..., None])
        yy, xx = np.mgrid[0:n, 0:n]
        rough = 0.08 + 0.02 * mat
        relief = np.zeros((n, n))
        for _ in range(count):
            cx, cy = rng.random() * n, rng.random() * n
            rr = rng.uniform(rmin, rmax) * ppm
            ang = rng.random() * math.pi
            stretch = rng.uniform(1.0, 2.2) if coral else rng.uniform(1.0, 1.4)
            x0, x1 = int(cx - rr * stretch - 2), int(cx + rr * stretch + 3)
            y0, y1 = int(cy - rr * stretch - 2), int(cy + rr * stretch + 3)
            ys = np.arange(y0, y1)
            xs = np.arange(x0, x1)
            Y, X = np.meshgrid(ys, xs, indexing="ij")
            dx, dy = X - cx, Y - cy
            ca, sa = math.cos(ang), math.sin(ang)
            ex = (dx * ca + dy * sa) / stretch
            ey = -dx * sa + dy * ca
            r = np.sqrt(ex * ex + ey * ey)
            th = np.arctan2(ey, ex)
            inside = r < rr
            if not inside.any():
                continue
            if coral:
                # A cup coral's section: radiating septa inside a wall, a darker axis.
                val = (0.55 + 0.45 * (np.cos(th * 28) > 0.3)) * (r / rr > 0.2)
                wall = (r > rr * 0.88)
            else:
                # A snail's whorled section: a spiral of shell, calcite-filled chambers.
                spiral = np.mod(th / (2 * math.pi) + r / (rr * 0.33), 1.0)
                val = 0.4 + 0.6 * (spiral < 0.25)
                wall = (r > rr * 0.9)
            c_here = np.where(wall[..., None], rim[None, None, :], shell[None, None, :] * val[..., None] + matrix[None, None, :] * (1 - val[..., None]) * 0.3)
            Ym, Xm = np.mod(Y, n), np.mod(X, n)
            col[Ym[inside], Xm[inside]] = c_here[inside]
            rough[Ym[inside], Xm[inside]] = 0.11
            relief[Ym[inside], Xm[inside]] -= 0.2 * (1 - val[inside])
        finish(key, col, rough, relief, describe)

    fossils("AL_ST_Purbeck", srgb(0x4F4A42), srgb(0x9A8F7E), srgb(0x2D2A26), 1500, 0.004, 0.009, 401,
            describe="Purbeck marble, Dorset (Viviparus shells)")
    fossils("AL_ST_Frosterley", srgb(0x1E1D1C), srgb(0xB8B4AC), srgb(0x5A5752), 90, 0.008, 0.02, 501, coral=True,
            describe="Frosterley marble, County Durham (Dibunophyllum corals)")

    # F · Hopton Wood: cream-buff fine limestone, faint crinoid fragments, soft mottling.
    m = periodic_noise(n, 60, 601, 5)
    specks = periodic_noise(n, 1.8, 602, 2)
    col = srgb(0xD7C9AC)[None, None, :] * (1 + 0.06 * m[..., None] - 0.05 * (specks > 0.55)[..., None])
    finish("AL_ST_HoptonWood", col, 0.10 + 0.03 * m, describe="Hopton Wood stone, Derbyshire")

    # G · Ashburton: grey-mauve, a breccia of angular clasts with red and white veins.
    cell, d1, edge = voronoi(n, 180, 701)
    rng = np.random.default_rng(702)
    tones = rng.normal(0, 0.12, 180)
    base = srgb(0x6D6466)[None, None, :] * (1 + tones[cell][..., None])
    base = base * (1 + 0.1 * periodic_noise(n, 25, 703, 4)[..., None])
    red_v = np.exp(-edge / 3.0)
    white_v = np.exp(-np.abs(periodic_noise(n, 120, 704, 4)) * 30.0)
    col = base + red_v[..., None] * (srgb(0x9B4540) - base) * 0.9
    col = col + white_v[..., None] * (srgb(0xE3DDD5) - col) * 0.85
    finish("AL_ST_Ashburton", col, 0.08 + 0.05 * white_v, describe="Ashburton marble, Devon")

    # H · Iona marble: white with streaks of green serpentine, drawn out along the bedding.
    yy, xx = np.mgrid[0:n, 0:n]
    warp = periodic_noise(n, 200, 801, 3) * 80
    band = periodic_noise(n, 35, 802, 4)
    streak = np.clip((np.sin((yy + warp) / n * 2 * math.pi * 3 + band * 3) - 0.55) * 3.0, 0, 1) * np.clip(periodic_noise(n, 90, 803, 3) + 0.4, 0, 1)
    col = srgb(0xE7E3DA)[None, None, :] * (1 + 0.03 * band[..., None])
    col = col + streak[..., None] * (srgb(0x6E8A67) - col) * 0.85
    finish("AL_ST_Iona", col, 0.08 + 0.03 * streak, describe="Iona marble, Inner Hebrides")

    # I · Tiree marble: rose pink with dark green flecks (augite), a salmon cloud.
    m = periodic_noise(n, 70, 901, 4)
    fl = periodic_noise(n, 1.6, 902, 2)
    flecks = fl > 0.62
    col = srgb(0xD5A596)[None, None, :] * (1 + 0.12 * m[..., None])
    col = np.where(flecks[..., None], srgb(0x3B5543)[None, None, :], col)
    finish("AL_ST_Tiree", col, 0.08 + 0.05 * flecks, describe="Tiree marble, Inner Hebrides")

    # J · Mona marble: green-black serpentinite, white calcite veins, patches of red.
    m = periodic_noise(n, 80, 1001, 5)
    v1 = np.exp(-np.abs(periodic_noise(n, 160, 1002, 4)) * 25.0)
    v2 = np.exp(-np.abs(periodic_noise(n, 50, 1003, 3)) * 35.0)
    redp = np.clip((periodic_noise(n, 110, 1004, 4) - 0.35) * 3.0, 0, 1)
    col = srgb(0x26302A)[None, None, :] * (1 + 0.3 * m[..., None])
    col = col + redp[..., None] * (srgb(0x6E322C) - col) * 0.8
    col = col + (v1 * 0.9 + v2 * 0.5)[..., None] * (srgb(0xE0DCD2) - col)
    finish("AL_ST_Mona", col, 0.07 + 0.04 * v1, describe="Mona marble, Anglesey")
    return out


def main(args):
    stats = {}
    if os.path.exists(STATS):
        with open(STATS, encoding="utf-8") as f:
            stats = json.load(f)
    todo = set(args) or {"tile", "border", "tablets"}   # the stones are albion_stones_cc0.py's now (photographed); "stones" draws them
    if "tile" in todo:
        stats["AL_TILE"] = tile_field()
    if "border" in todo:
        stats["AL_BORDER"] = border()
    if "tablets" in todo:
        stats["AL_RUSKIN"] = tablet("AL_RUSKIN", 9.16, 0.96, [("REJECTING NOTHING · SELECTING NOTHING", 0.26, 0.40, 0.035),
                                                             ("SCORNING NOTHING", 0.26, 0.80, 0.035)],
                                    describe="Ruskin, Modern Painters I (1843), V-cut in Centaur capitals")
        stats["AL_NAME"] = tablet("AL_NAME", 5.16, 1.56, [("ALBION", 0.62, 0.86, 0.10), ("1848 · 1898", 0.20, 1.30, 0.05)],
                                  describe="ALBION 1848 · 1898, V-cut")
    if "stones" in todo:
        stats.update(stones())
    with open(STATS, "w", encoding="utf-8") as f:
        json.dump(stats, f, indent=1)


if __name__ == "__main__":
    main(sys.argv[1:])
