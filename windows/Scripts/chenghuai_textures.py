"""
Chenghuai's painted and lettered textures (plan/proposals/chenghuai: Materials, Names and Section boards), drawn with PIL
into windows/SourceArt/Textures (materials.texture_asset imports them; chenghuai.py makes the instances):

    python windows/Scripts/chenghuai_textures.py

- T_ch_suhua.png: the Suzhou-style painting (苏式彩画) of the beams, an atlas of 8 rows × 4096 × 512 (ChenghuaiPaint.h):
  rows 0-3 the eave lintel and board of a bay with its arched panel (包袱) holding a small landscape (cropped from Wang
  Ximeng's Thousand Li, public domain) inside layered 'smoke-cloud' borders (烟云), the end bands (箍头) with beads and
  the scroll clasps (卡子); row 4 a plain painted beam; row 5 a purlin's band; row 6 the round rafters' heads (the
  'dragon's eye', 龙眼); row 7 the flying rafters' heads (a gilt 万 on green). Azurite blue, malachite green, iron red,
  ochre gold lines, the black and white of the linework; faded, dusted and crazed as sixty-year-old paint is.
- T_ch_plaques.png: the house's plaques and couplet (Names board), black lacquer boards with raised gilt characters in a
  gilt frame, read right to left; the discs of the green screen door (正中莊齋) and of the gate's door pins (吉祥如意); the
  screen wall's carved brick heart (澄懷觀道) in its moulded frame with corner flowers (岔角).

Characters in KaiTi (C:/Windows/Fonts/simkai.ttf), which has every traditional form the names need.
"""
import math
import os
import random

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

Image.MAX_IMAGE_PIXELS = None
HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(REPO, "windows", "SourceArt", "Textures")
PAINTINGS = os.path.join(REPO, "assets", "paintings_hires")
FONT = "C:/Windows/Fonts/simkai.ttf"

# The palette (aged mineral pigments, sRGB): a scholar's house, restrained (the renderings' dark green and slate beams),
# the pigments sixty years faded and dusted, not a palace's fresh blue and green.
BLUE = (22, 44, 76)         # azurite, a deep blue (round 8: deeper still, the references' azurite in sun)
BLUE_L = (78, 100, 112)
GREEN = (18, 56, 54)        # malachite, a deep teal (round 8: the references' blue-green, not mint, in sun)
GREEN_L = (84, 116, 100)
RED = (112, 50, 40)         # iron red (铁红) / 章丹, dulled: accents only since round 4
GOLD = (178, 136, 48)       # leaf gold over raised putty lines (沥粉贴金, round 8): saturated, so M_Ch_Stone's Inlay makes it metal
BLACK = (24, 22, 20)
WHITE = (214, 208, 192)     # the linework's white, dusted
CREAM = (220, 208, 180)


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def age(img, seed, dust=0.07, fade=0.1, edges=True):
    """Sixty years on a beam: a slow fade towards the lime ground, dust, fine crazing and a few small losses."""
    rnd = random.Random(seed)
    w, h = img.size
    base = img.convert("RGB")
    # Fade towards a pale grey-buff, unevenly.
    veil = Image.new("RGB", (w, h), (170, 160, 140))
    mask = Image.effect_noise((max(8, w // 64), max(8, h // 64)), 60).resize((w, h), Image.BICUBIC)
    mask = mask.point(lambda v: int(max(0, min(255, (v - 40) * fade * 3.0))))
    base = Image.composite(veil, base, mask)
    # Crazing: a network of fine dark lines.
    craze = Image.new("L", (w, h), 0)
    d = ImageDraw.Draw(craze)
    for _ in range(int(w * h / 9000)):
        x, y = rnd.uniform(0, w), rnd.uniform(0, h)
        a = rnd.uniform(0, math.pi)
        L = rnd.uniform(8, 40)
        d.line([(x, y), (x + L * math.cos(a), y + L * math.sin(a) * 0.5)], fill=rnd.randint(40, 90), width=1)
    base = Image.composite(Image.new("RGB", (w, h), (60, 52, 44)), base, craze.filter(ImageFilter.GaussianBlur(0.4)))
    # Dust and grime.
    grime = Image.effect_noise((w, h), 30).filter(ImageFilter.GaussianBlur(1.2))
    grime = grime.point(lambda v: int(max(0, min(255, (v - 128) * dust * 4 + 128))))
    base = ImageChops.multiply(base, Image.merge("RGB", (grime, grime, grime)).point(lambda v: int(v * 1.0 + 70) if v < 185 else 255))
    # Wear where it happens: the paint rubbed thin along the arrises (a face's top and bottom edges), the lime ground
    # showing through in a ragged band, not scattered dots.
    if edges:
        ew = max(4, h // 40)   # (round 7: wider, the arrises' wear read at a distance)
        band = Image.new("L", (w, h), 0)
        db = ImageDraw.Draw(band)
        db.rectangle([0, 0, w, ew], fill=255)
        db.rectangle([0, h - ew, w, h], fill=255)
        rag = Image.effect_noise((w, h), 90).point(lambda v: 255 if v > 150 else 0)
        band = ImageChops.multiply(band.filter(ImageFilter.GaussianBlur(ew * 0.8)), rag).filter(ImageFilter.GaussianBlur(0.8))
        base = Image.composite(Image.new("RGB", (w, h), (176, 166, 146)), base, band.point(lambda v: int(v * 0.8)))
        # Chips: small flakes of the paint gone to the lime ground and the wood under it, near the arrises.
        chips = Image.new("L", (w, h), 0)
        dc = ImageDraw.Draw(chips)
        for _ in range(int(w / 40)):
            x = rnd.uniform(0, w)
            y = rnd.choice((rnd.uniform(0, h * 0.12), rnd.uniform(h * 0.88, h)))
            r = rnd.uniform(2, 7)
            dc.ellipse([x - r * 1.6, y - r, x + r * 1.6, y + r], fill=255)
        base = Image.composite(Image.new("RGB", (w, h), (120, 92, 70)), base, chips.filter(ImageFilter.GaussianBlur(0.7)))
    return base


def ink(im, colour=0.25):
    """A landscape as a painter would put it in a 包袱 panel: ink and a breath of colour on a pale ground."""
    g = im.convert("L").convert("RGB")
    return Image.blend(g, im.convert("RGB"), colour)


def landscape_crops():
    """Small landscapes for the panels: Ni Zan's river bank (ink), and the Thousand Li's hills over water, brought to ink
    and light colour (the renderings: 'a single white semicircular panel with a small ink landscape')."""
    out = []
    nz = os.path.join(PAINTINGS, "chinese-ni-zan-rongxi.jpg")
    if os.path.exists(nz):
        im = Image.open(nz)
        w, h = im.size
        for box in ((0.05, 0.62, 0.95, 0.95), (0.1, 0.5, 0.95, 0.76), (0.0, 0.66, 0.7, 0.98)):
            c = im.crop((int(w * box[0]), int(h * box[1]), int(w * box[2]), int(h * box[3])))
            out.append(ink(c, 0.1).resize((1200, 560), Image.LANCZOS))
    paths = [os.path.join(PAINTINGS, f"chinese-thousand-li-{k}.jpg") for k in (1, 2, 3, 4)]
    for k, p in enumerate(paths):
        if not os.path.exists(p):
            continue
        im = Image.open(p)
        w, h = im.size
        for j in range(3):
            x0 = int(w * (0.12 + 0.3 * j))
            box = (x0, int(h * 0.3), x0 + int(h * 1.5), int(h * 0.98))
            out.append(ink(im.crop(box), 0.35).resize((1200, 560), Image.LANCZOS))
    return out


def beads(d, x0, y0, x1, y1, n, c0, c1):
    """A band of 'connected beads' (连珠) between two lines, across its width x0 … x1 (vertical band)."""
    h = (y1 - y0) / n
    r = min(h * 0.42, (x1 - x0) * 0.42)
    cx = 0.5 * (x0 + x1)
    for i in range(n):
        cy = y0 + h * (i + 0.5)
        d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=c0 if i % 2 == 0 else c1, outline=BLACK)


def scroll_clasp(d, cx, y0, y1, colour, flip):
    """A scrollwork clasp (卡子, round 6): two pairs of thin C-scrolls curling away from a short stem, gold as outline
    only; the old heavy clasp read as a printed logo."""
    h = y1 - y0
    w = max(3, int(h * 0.045))   # (round 9: bolder, to read at the court's distance)
    cy = 0.5 * (y0 + y1)
    for sx in (-1, 1):
        for sy in (-1, 1):
            r = h * 0.17
            ox, oy = cx + sx * r * 0.9, cy + sy * r * 0.85
            start = {(1, 1): 180, (1, -1): 90, (-1, 1): 270, (-1, -1): 0}[(sx, sy)]
            d.arc([ox - r, oy - r, ox + r, oy + r], start, start + 270, fill=colour, width=w)
            r2 = r * 0.45
            d.arc([ox - r2, oy - r2, ox + r2, oy + r2], start + 90, start + 360, fill=colour, width=w)
    d.line([(cx - h * 0.05, cy), (cx + h * 0.05, cy)], fill=colour, width=w)


def clasp(d, cx, y0, y1, colour, flip):
    """A scroll clasp (卡子): two mirrored C-scrolls meeting at a stem, drawn with thick strokes."""
    h = y1 - y0
    s = -1 if flip else 1
    for k in (-1, 1):
        cy = 0.5 * (y0 + y1) + k * h * 0.2
        r = h * 0.18
        d.arc([cx - r, cy - r, cx + r, cy + r], 90 - 90 * s if k < 0 else 270 - 90 * s, 270 - 90 * s if k < 0 else 450 - 90 * s, fill=colour, width=int(h * 0.06))
        d.arc([cx - r * 0.55, cy - r * 0.55, cx + r * 0.55, cy + r * 0.55], 0, 360, fill=colour, width=int(h * 0.04))
    d.line([(cx, y0 + h * 0.08), (cx, y1 - h * 0.08)], fill=colour, width=int(h * 0.05))


def fret(d, x0, y0, x1, y1, cell, colour, width):
    """A key fret (回纹) filling a rectangle: square spirals, cell by cell, in one line colour."""
    nx = max(1, int((x1 - x0) // cell))
    ny = max(1, int((y1 - y0) // cell))
    cw, ch = (x1 - x0) / nx, (y1 - y0) / ny
    for i in range(nx):
        for j in range(ny):
            ax, ay = x0 + i * cw, y0 + j * ch
            l, t, r, b = ax + cw * 0.08, ay + ch * 0.08, ax + cw * 0.92, ay + ch * 0.92
            sx, sy = (r - l) / 6.0, (b - t) / 6.0
            pts = [(l, b), (l, t), (r, t), (r, b), (l + sx, b),
                   (l + sx, t + sy), (r - sx, t + sy), (r - sx, b - sy), (l + 2 * sx, b - sy),
                   (l + 2 * sx, t + 2 * sy), (r - 2 * sx, t + 2 * sy), (r - 2 * sx, b - 2 * sy)]
            d.line(pts, fill=colour, width=width, joint="curve")


def brocade(d, x0, y0, x1, y1, cell, line, dot):
    """A brocade (锦纹) of interlocking diamonds with a dot at each heart: the 找头's fill."""
    nx = max(1, int((x1 - x0) // cell))
    ny = max(1, int((y1 - y0) // (cell * 0.6)))
    cw, ch = (x1 - x0) / nx, (y1 - y0) / ny
    for i in range(nx + 1):
        for j in range(ny + 1):
            if (i + j) % 2:
                continue
            cx, cy = x0 + i * cw, y0 + j * ch
            d.line([(cx, cy - ch), (cx + cw, cy), (cx, cy + ch), (cx - cw, cy), (cx, cy - ch)], fill=line, width=3)
            r = min(cw, ch) * 0.16
            d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=dot)


def hoop(d, x0, x1, H, main, other, flip=False):
    """The end band (箍头): layered stripes, dark to light, black and white linework, beads in the middle."""
    w = x1 - x0
    order = [(0.00, 0.06, BLACK), (0.06, 0.10, WHITE), (0.10, 0.34, other), (0.34, 0.66, main), (0.66, 0.90, other),
             (0.90, 0.94, WHITE), (0.94, 1.0, BLACK)]
    for a, b, c in order:
        xa, xb = (x0 + w * a, x0 + w * b) if not flip else (x1 - w * b, x1 - w * a)
        d.rectangle([xa, 0, xb, H], fill=c)
    beads(d, x0 + w * 0.4, 16, x0 + w * 0.6, H - 16, 11, WHITE, lerp(main, WHITE, 0.4))
    for x in (x0 + w * 0.1, x0 + w * 0.9, x0 + w * 0.34, x0 + w * 0.66):
        d.line([(x, 0), (x, H)], fill=GOLD, width=7)


def wrapper(img, cx, pw, ph, crop, ground, other):
    """The arched panel (包袱): a half-ellipse hanging from the top, its border the layered 'smoke clouds' (烟云), a small
    ink landscape inside."""
    W, H = img.size
    d = ImageDraw.Draw(img)
    rings = [(18, BLACK), (14, GOLD), (14, other), (12, lerp(other, WHITE, 0.3)), (10, lerp(other, WHITE, 0.55)),
             (8, lerp(other, WHITE, 0.8)), (14, GOLD)]
    grow = sum(r for r, _ in rings) + 22
    d.pieslice([cx - pw / 2 - grow, -ph - grow, cx + pw / 2 + grow, ph + grow], 0, 180, fill=lerp(ground, BLACK, 0.3))
    off = grow
    for r, c in rings:
        d.pieslice([cx - pw / 2 - off, -ph - off, cx + pw / 2 + off, ph + off], 0, 180, fill=c)
        off -= r
    panel = Image.new("L", (W, H), 0)
    ImageDraw.Draw(panel).pieslice([cx - pw / 2, -ph, cx + pw / 2, ph], 0, 180, fill=255)
    paper = lerp(CREAM, (150, 140, 118), 0.25)
    g = Image.new("RGB", (W, H), paper)
    if crop is not None:
        c = crop.resize((int(pw), int(ph)), Image.LANCZOS)
        c = Image.blend(c, Image.new("RGB", c.size, paper), 0.3)
        g.paste(c, (int(cx - pw / 2), 0))
    img.paste(g, (0, 0), panel)
    ImageDraw.Draw(img).arc([cx - pw / 2, -ph, cx + pw / 2, ph], 0, 180, fill=BLACK, width=4)


def beam_row(style, crop, seed, band_top=0.42):
    """One bay's lintel and board (virtual 2970 × 512, stretched to 4096; one bay per span): the board (top 42 %) in dark
    green or blue with a key fret, the lintel with brocade in the 找头, layered 箍头 at both ends, scroll clasps, and the
    big 包袱 in the middle. (Round 4: the references' muted, dense under-eave painting; the flat red board and flat
    lintels read as stripes.)"""
    W, H = 2970, 512
    lintel_col = BLUE if style % 2 == 0 else GREEN
    other = GREEN if style % 2 == 0 else BLUE
    img = Image.new("RGB", (W, H), lintel_col)
    d = ImageDraw.Draw(img)
    board_h = int(H * band_top)
    d.rectangle([0, 0, W, board_h], fill=lerp(other, BLACK, 0.15))
    fret(d, 0, 14, W, board_h - 14, board_h - 28, GOLD, 7)   # (round 9: the board's fret in gold, as the references' dense gilt)
    brocade(d, 0, board_h + 18, W, H - 18, 30, lerp(lintel_col, WHITE, 0.16), lerp(GOLD, BLACK, 0.25))
    for y, c, wd in ((board_h + 3, BLACK, 4), (board_h + 10, GOLD, 8), (H - 12, GOLD, 8), (H - 4, BLACK, 4), (4, BLACK, 4), (board_h - 7, GOLD, 7)):
        d.line([(0, y), (W, y)], fill=c, width=wd)
    band = int(W * 0.07)
    hoop(d, 0, band, H, lintel_col, other)
    hoop(d, W - band, W, H, lintel_col, other, flip=True)
    for cx, flip in ((W * 0.15, False), (W * 0.85, True)):
        d.rectangle([cx - 90, board_h + 14, cx + 90, H - 14], fill=lintel_col, outline=GOLD, width=3)
        scroll_clasp(d, cx, board_h + 26, H - 26, GOLD, flip)
    wrapper(img, W * 0.5, W * 0.36, H * 0.9, crop, lintel_col, other)
    img = age(img, seed, dust=0.05, fade=0.02)
    return img.resize((4096, 512), Image.LANCZOS)


def plain_row(seed, crop=None):
    """A painted beam (interior beams, backs, undersides): 箍头 at both ends, brocade between, a small 包袱 in the middle,
    so each span reads as one painted panel (round 4; it was blue and green halves with two clasps)."""
    W, H = 2970, 512
    img = Image.new("RGB", (W, H), GREEN)
    d = ImageDraw.Draw(img)
    d.rectangle([0, H * 0.5, W, H], fill=BLUE)
    brocade(d, 0, 18, W, H * 0.5 - 4, 30, lerp(GREEN, WHITE, 0.16), lerp(GOLD, BLACK, 0.25))
    brocade(d, 0, H * 0.5 + 4, W, H - 18, 30, lerp(BLUE, WHITE, 0.16), lerp(GOLD, BLACK, 0.25))
    d.line([(0, H * 0.5), (W, H * 0.5)], fill=BLACK, width=4)
    band = int(W * 0.07)
    hoop(d, 0, band, H, BLUE, GREEN)
    hoop(d, W - band, W, H, BLUE, GREEN, flip=True)
    for cx, flip in ((W * 0.17, False), (W * 0.83, True)):
        d.rectangle([cx - 86, 16, cx + 86, H - 16], fill=GREEN, outline=GOLD, width=3)
        scroll_clasp(d, cx, 30, H - 30, GOLD, flip)
    wrapper(img, W * 0.5, W * 0.3, H * 0.86, crop, BLUE, GREEN)
    d = ImageDraw.Draw(img)
    d.line([(0, 4), (W, 4)], fill=BLACK, width=6)
    d.line([(0, H - 4), (W, H - 4)], fill=BLACK, width=6)
    return age(img, seed, dust=0.05, fade=0.02).resize((4096, 512), Image.LANCZOS)


def purlin_row(seed):
    """A purlin's band, repeated every 3.1 m along it: blue with a key fret, a layered end band at the joint."""
    W, H = 2970, 512
    img = Image.new("RGB", (W, H), BLUE)
    d = ImageDraw.Draw(img)
    fret(d, 0, 30, W, H - 30, 112, GOLD, 8)
    hoop(d, 0, 150, H, GREEN, BLUE)
    hoop(d, W - 150, W, H, GREEN, BLUE, flip=True)
    return age(img, seed, dust=0.05, fade=0.02).resize((4096, 512), Image.LANCZOS)


def rafter_heads(seed):
    """Row 6: eight round rafters' heads; row 7: four flying rafters' heads (then blanks)."""
    row6 = Image.new("RGB", (4096, 512), BLUE)
    row7 = Image.new("RGB", (4096, 512), GREEN)
    for k in range(8):
        cell = Image.new("RGB", (512, 512), BLUE)
        d = ImageDraw.Draw(cell)
        # (Round 6: the references' round rafter ends carry a flower roundel (柿子花/宝珠): a gold-ringed blue disc, eight
        # petals in light blue and white round a gold heart; the old 'dragon's eye' rings read as targets.)
        d.ellipse([10, 10, 502, 502], fill=BLUE, outline=BLACK, width=18)
        d.ellipse([40, 40, 472, 472], outline=GOLD, width=14)
        for j in range(8):
            a0 = math.radians(j * 45 + (k % 2) * 22.5)
            cx, cy = 256 + 120 * math.cos(a0), 256 + 120 * math.sin(a0)
            d.ellipse([cx - 62, cy - 62, cx + 62, cy + 62], fill=BLUE_L, outline=BLACK, width=6)
            d.ellipse([cx - 30, cy - 30, cx + 30, cy + 30], fill=WHITE)
        d.ellipse([256 - 64, 256 - 64, 256 + 64, 256 + 64], fill=GOLD, outline=BLACK, width=6)
        row6.paste(age(cell, seed + k, dust=0.1, fade=0.15, edges=False), (512 * k, 0))
    font = ImageFont.truetype(FONT, 380)
    for k in range(8):
        cell = Image.new("RGB", (512, 512), GREEN)
        d = ImageDraw.Draw(cell)
        d.rectangle([0, 0, 511, 511], outline=BLACK, width=22)
        if k < 4:
            d.text((256, 262), "卍", font=font, fill=GOLD, anchor="mm")
        row7.paste(age(cell, seed + 20 + k, dust=0.1, fade=0.15, edges=False), (512 * k, 0))
    return row6, row7


def raised_gold(img):
    """沥粉贴金 (round 8): the gold lines stand on raised putty. The beams' material makes the gold metal (Inlay); the
    relief is drawn into the paint as a painter's light would fall on it: a lit edge up and left of each gold line, a
    soft shadow down and right, so the lines read proud from the court."""
    import numpy as np
    a = np.asarray(img, np.float32)
    r, b = a[..., 0], a[..., 2]
    gold = np.clip(((r - b) / np.maximum(r, 5.0) - 0.5) * 6.0, 0, 1) * (r > 90)
    g = Image.fromarray((gold * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(1.0))
    m = np.asarray(g, np.float32) / 255.0
    sh = np.roll(np.roll(m, 3, 0), 3, 1) - m          # down-right of the line, off it
    hi = np.roll(np.roll(m, -2, 0), -2, 1) - m        # up-left
    sh = np.clip(sh, 0, 1)[..., None]; hi = np.clip(hi, 0, 1)[..., None]
    a = a * (1.0 - 0.55 * sh) + (255.0 - a) * 0.25 * hi
    return Image.fromarray(np.clip(a, 0, 255).astype(np.uint8))


def make_suhua():
    crops = landscape_crops()
    rows = []
    for r in range(4):
        rows.append(beam_row(r, crops[r % len(crops)] if crops else None, 100 + r))
    rows.append(plain_row(200, crops[4 % len(crops)] if crops else None))
    rows.append(purlin_row(300))
    r6, r7 = rafter_heads(400)
    rows += [r6, r7]
    atlas = Image.new("RGB", (4096, 4096))
    for i, r in enumerate(rows):
        atlas.paste(r, (0, 512 * i))
    atlas = raised_gold(atlas)
    p = os.path.join(OUT, "T_ch_suhua.png")
    atlas.save(p)
    print("wrote", p)


# ------------------------------------------------------------------------------------------------ plaques

GILT = (212, 172, 86)
LACQUER = (22, 19, 17)


def gilt_text(size, text, box, vertical=False, colour=GILT, weight=1.0):
    """Raised gilt characters: the letters, a highlight up-left and a shadow down-right, so they read as carved relief."""
    w, h = size
    layer = Image.new("L", (w, h), 0)
    d = ImageDraw.Draw(layer)
    n = len(text)
    x0, y0, x1, y1 = box
    if vertical:
        step = (y1 - y0) / n
        fs = int(min(step * 0.86, (x1 - x0) * 0.86) * weight)
        font = ImageFont.truetype(FONT, fs)
        for i, ch in enumerate(text):
            d.text((0.5 * (x0 + x1), y0 + step * (i + 0.5)), ch, font=font, fill=255, anchor="mm")
    else:
        step = (x1 - x0) / n
        fs = int(min(step * 0.86, (y1 - y0) * 0.86) * weight)
        font = ImageFont.truetype(FONT, fs)
        for i, ch in enumerate(text):
            d.text((x0 + step * (i + 0.5), 0.5 * (y0 + y1)), ch, font=font, fill=255, anchor="mm")
    # Thicken a touch (carved strokes are fuller than a font's).
    layer = layer.filter(ImageFilter.MaxFilter(3))
    return layer


def emboss(base, mask, colour, depth=4):
    """Puts raised letters (mask) on the base: the colour, lit from the upper left."""
    w, h = base.size
    hi = ImageChops.offset(mask, -depth // 2, -depth // 2)
    lo = ImageChops.offset(mask, depth, depth)
    shadow = ImageChops.subtract(lo, mask).filter(ImageFilter.GaussianBlur(1.5))
    out = Image.composite(Image.new("RGB", (w, h), (0, 0, 0)), base, shadow.point(lambda v: int(v * 0.7)))
    out = Image.composite(Image.new("RGB", (w, h), colour), out, mask)
    edge = ImageChops.subtract(mask, hi).filter(ImageFilter.GaussianBlur(0.8))
    out = Image.composite(Image.new("RGB", (w, h), lerp(colour, (255, 240, 200), 0.5)), out, edge.point(lambda v: int(v * 0.6)))
    return out


def board(w, h, text, vertical=False, seed=0):
    """A black lacquer board with a gilt frame and raised gilt characters."""
    img = Image.new("RGB", (w, h), LACQUER)
    d = ImageDraw.Draw(img)
    m = int(min(w, h) * 0.07)
    d.rectangle([0, 0, w - 1, h - 1], fill=(92, 60, 30))
    d.rectangle([m * 0.35, m * 0.35, w - m * 0.35, h - m * 0.35], fill=GILT)
    d.rectangle([m, m, w - m, h - m], fill=LACQUER)
    d.rectangle([m + 10, m + 10, w - m - 10, h - m - 10], outline=lerp(GILT, LACQUER, 0.4), width=4)
    mask = gilt_text((w, h), text, (m * 1.6, m * 1.6, w - m * 1.6, h - m * 1.6), vertical=vertical)
    img = emboss(img, mask, GILT)
    return age(img, seed, dust=0.05, fade=0.04, edges=False)


def disc(size, char, ground, colour, seed):
    img = Image.new("RGB", (size, size), ground)
    d = ImageDraw.Draw(img)
    d.ellipse([6, 6, size - 6, size - 6], fill=ground, outline=GILT, width=18)
    mask = gilt_text((size, size), char, (60, 60, size - 60, size - 60))
    img = emboss(img, mask, colour)
    return age(img, seed, dust=0.05, fade=0.05, edges=False)


def flower_disc(size, kind, ground, seed):
    """A carved gilt flower on a lacquered disc: 0 plum (five round petals), 1 lotus (layered pointed petals),
    2 chrysanthemum (many narrow petals), 3 a rosette."""
    img = Image.new("RGB", (size, size), ground)
    d = ImageDraw.Draw(img)
    d.ellipse([6, 6, size - 6, size - 6], fill=ground, outline=GILT, width=18)
    mask = Image.new("L", (size, size), 0)
    m = ImageDraw.Draw(mask)
    c = size / 2
    def petal(angle, r0, r1, width):
        pts = []
        for t in range(0, 21):
            u = t / 20.0
            rr = r0 + (r1 - r0) * u
            w = width * math.sin(math.pi * min(1.0, u * 1.1)) * (1.0 - 0.3 * u)
            pts.append((rr, w))
        poly = [(c + r * math.cos(angle) - w * math.sin(angle), c + r * math.sin(angle) + w * math.cos(angle)) for r, w in pts]
        poly += [(c + r * math.cos(angle) + w * math.sin(angle), c + r * math.sin(angle) - w * math.cos(angle)) for r, w in reversed(pts)]
        m.polygon(poly, fill=255)
    if kind == 0:
        for k in range(5):
            a = 2 * math.pi * k / 5 - math.pi / 2
            x, y = c + 105 * math.cos(a), c + 105 * math.sin(a)
            m.ellipse([x - 82, y - 82, x + 82, y + 82], fill=255)
        m.ellipse([c - 50, c - 50, c + 50, c + 50], fill=0)
        for k in range(12):
            a = 2 * math.pi * k / 12
            m.ellipse([c + 34 * math.cos(a) - 7, c + 34 * math.sin(a) - 7, c + 34 * math.cos(a) + 7, c + 34 * math.sin(a) + 7], fill=255)
    elif kind == 1:
        for k in range(8):
            petal(2 * math.pi * k / 8 - math.pi / 2, 40, 200, 52)
        for k in range(8):
            petal(2 * math.pi * (k + 0.5) / 8 - math.pi / 2, 30, 130, 34)
        m.ellipse([c - 45, c - 45, c + 45, c + 45], fill=255)
    elif kind == 2:
        for k in range(28):
            petal(2 * math.pi * k / 28, 30, 205, 16)
        for k in range(18):
            petal(2 * math.pi * (k + 0.5) / 18, 20, 120, 13)
    else:
        for k in range(12):
            petal(2 * math.pi * k / 12, 60, 195, 30)
        m.ellipse([c - 62, c - 62, c + 62, c + 62], fill=255)
        m.ellipse([c - 40, c - 40, c + 40, c + 40], fill=0)
        m.ellipse([c - 22, c - 22, c + 22, c + 22], fill=255)
    img = emboss(img, mask, GILT, depth=6)
    return age(img, seed, dust=0.05, fade=0.05, edges=False)


def screen_heart(size, seed):
    """The screen wall's heart: rubbed grey brick set diagonally, a moulded frame, corner flowers, 澄懷觀道 raised."""
    GREY = (132, 133, 129)
    img = Image.new("RGB", (size, size), GREY)
    d = ImageDraw.Draw(img)
    # Diagonal square bricks (方砖心), hairline joints.
    step = size / 6
    for k in range(-8, 16):
        d.line([(k * step, 0), (k * step + size, size)], fill=(118, 119, 115), width=3)
        d.line([(k * step, 0), (k * step - size, size)], fill=(118, 119, 115), width=3)
    # The moulded frame (线枋子).
    for i, c in enumerate(((104, 105, 101), (150, 151, 146), (120, 121, 117))):
        o = 18 + 18 * i
        d.rectangle([o, o, size - o, size - o], outline=c, width=16)
    # Corner flowers (岔角): a quarter rosette in each corner.
    for (cx, cy, a0) in ((80, 80, 0), (size - 80, 80, 90), (size - 80, size - 80, 180), (80, size - 80, 270)):
        for r in (150, 115, 80):
            d.pieslice([cx - r, cy - r, cx + r, cy + r], a0, a0 + 90, outline=(100, 101, 97), width=6)
    # The characters, two columns read from the right: 澄懷 | 觀道.
    mask = Image.new("L", (size, size), 0)
    m1 = gilt_text((size, size), "澄懷", (size * 0.52, size * 0.14, size * 0.86, size * 0.86), vertical=True, weight=1.05)
    m2 = gilt_text((size, size), "觀道", (size * 0.14, size * 0.14, size * 0.48, size * 0.86), vertical=True, weight=1.05)
    mask = ImageChops.lighter(m1, m2)
    img = emboss(img, mask, (150, 151, 147), depth=8)
    return age(img, seed, dust=0.12, fade=0.02, edges=False)


def make_plaques():
    atlas = Image.new("RGB", (4096, 4096), LACQUER)
    # Rows 0-2: horizontal boards (read right to left: the first character on the right).
    names = ["臨池", "天青", "昌南", "清閟", "停雲", "舒卷", "臥遊", "林泉", "知魚", "見山", "遊目", "澄懷堂"]
    for i, n in enumerate(names):
        b = board(1024, 512, n[::-1], seed=500 + i)
        atlas.paste(b, (1024 * (i % 4), 512 * (i // 4)))
    # Row 3: discs for the doors, carved flowers only (no characters on any door): the screen door's (red, gilt) and the
    # door pins' (green, gilt): plum, lotus, chrysanthemum and a rosette, the four seasons.
    for i in range(4):
        atlas.paste(flower_disc(512, i, (150, 52, 38), 600 + i), (512 * i, 1536))
        atlas.paste(flower_disc(512, i, (44, 86, 74), 610 + i), (512 * (4 + i), 1536))
    # Rows 4-5: the couplet (right: 行到水窮處; left: 坐看雲起時), the screen wall's heart.
    atlas.paste(board(256, 1024, "行到水窮處", vertical=True, seed=700), (0, 2048))
    atlas.paste(board(256, 1024, "坐看雲起時", vertical=True, seed=701), (256, 2048))
    atlas.paste(screen_heart(1024, 800), (1024, 2048))
    p = os.path.join(OUT, "T_ch_plaques.png")
    atlas.save(p)
    print("wrote", p)


# The twelve 书条石 in the garden walk's wall (遊目): 0.9 × 0.32 m bluestone slabs, each cut with a stretch of the
# house's own calligraphy at its true size, read right to left along the walk (Wang Xizhi's Lanting first).
SHUTIAO_CELL = (1360, 484)          # 0.9 × 0.32 m at about 1510 px/m
SHUTIAO_GRID = (3, 4)
SHUTIAO = [
    # (image file(s) joined left to right, its height in m, the stone's slice: right edge as a fraction from the right)
    (["chinese-orchid-pavilion.jpg"], 0.245, 0.0),
    (["chinese-yan-zhenqing-nephew.jpg"], 0.282, 0.0),
    (["chinese-huaisu-autobiography-1.jpg", "chinese-huaisu-autobiography-2.jpg"], 0.283, 0.0),
    (["chinese-huaisu-autobiography-1.jpg", "chinese-huaisu-autobiography-2.jpg"], 0.283, 0.12),
    (["chinese-huaisu-autobiography-1.jpg", "chinese-huaisu-autobiography-2.jpg"], 0.283, 0.24),
    (["chinese-huaisu-autobiography-1.jpg", "chinese-huaisu-autobiography-2.jpg"], 0.283, 0.5),
    (["chinese-huaisu-autobiography-1.jpg", "chinese-huaisu-autobiography-2.jpg"], 0.283, 0.75),
    (["chinese-su-shi-cold-food.jpg"], 0.342, 0.0),
    (["chinese-su-shi-cold-food.jpg"], 0.342, 0.45),
    (["chinese-mi-fu-shu-su.jpg"], 0.278, 0.0),
    (["chinese-mi-fu-shu-su.jpg"], 0.278, 0.34),
    (["chinese-mi-fu-shu-su.jpg"], 0.278, 0.68),
]


def engraved(src, seed):
    """Calligraphy cut into bluestone (阴刻): the ink (from the red channel, so seals drop out) becomes V-cut grooves,
    their floors paler with old lime dust, their upper walls in shadow, lower walls catching the light; the stone
    honed, a little mottled, weathered at the edges."""
    import numpy as np
    from scipy import ndimage
    rng = np.random.default_rng(seed)
    a = np.asarray(src.convert("RGB"), np.float32) / 255.0
    r = a[:, :, 0]
    bg, ink = np.percentile(r, 70), np.percentile(r, 1.5)
    m = np.clip((bg - r) / max(bg - ink, 1e-3), 0, 1)
    m = np.clip((m - 0.25) / 0.5, 0, 1)
    m = ndimage.gaussian_filter(m, 1.2)
    h, w = m.shape
    stone = np.array([0.33, 0.345, 0.35], np.float32)
    mott = ndimage.gaussian_filter(rng.standard_normal((h // 3 + 1, w // 3 + 1)), 2)
    mott = ndimage.zoom(mott, 3, order=1)[:h, :w]
    fine = rng.standard_normal((h, w)) * 0.012
    base = stone[None, None, :] * (1 + 0.025 * mott[:, :, None] / (mott.std() + 1e-6)) + fine[:, :, None]
    gy = np.gradient(ndimage.gaussian_filter(m, 2.0), axis=0)
    shade = np.clip(-gy * 6.0, -0.25, 0.25)
    groove = base * (1 + 0.28 * m[:, :, None]) + 0.05 * m[:, :, None]
    out = groove + shade[:, :, None] * 0.5
    return Image.fromarray((np.clip(out, 0, 1) * 255).astype("uint8"))


def make_shutiao():
    cw, ch = SHUTIAO_CELL
    atlas = Image.new("RGB", (cw * SHUTIAO_GRID[0], ch * SHUTIAO_GRID[1]), (84, 88, 89))
    art = os.path.join(REPO, "assets", "paintings")
    for k, (files, height, frac) in enumerate(SHUTIAO):
        ims = [Image.open(os.path.join(art, f)).convert("RGB") for f in files]
        hh = min(i.height for i in ims)
        ims = [i.resize((round(i.width * hh / i.height), hh)) for i in ims]
        strip = Image.new("RGB", (sum(i.width for i in ims), hh))
        x = 0
        for i in ims:
            strip.paste(i, (x, 0))
            x += i.width
        px_per_m = hh / height
        # The text sits within the stone's 0.3 m face with a 1 cm margin; the slice is 0.86 m of the scroll.
        sw = int(0.86 * px_per_m)
        x1 = strip.width - int(frac * strip.width)
        x0 = max(0, x1 - sw)
        piece = strip.crop((x0, 0, x1, hh))
        face_h = int(ch * 0.30 / 0.32)
        text_h = int(face_h * min(1.0, height / 0.30))
        piece = piece.resize((max(1, int(piece.width * text_h / hh)), text_h), Image.LANCZOS)
        cell = Image.new("RGB", (cw, ch), (0, 0, 0))
        ground = Image.new("RGB", (cw, ch), (int(0.33 * 255), int(0.345 * 255), int(0.35 * 255)))
        # The ground around the text: the paper's own tone (its 70th percentile), so the sheet's edge isn't cut.
        import numpy as np
        pa = np.asarray(piece, np.float32).reshape(-1, 3)
        paper = tuple(int(v) for v in np.percentile(pa, 70, axis=0))
        canvas = Image.new("RGB", (cw, ch), paper)
        canvas.paste(piece, (cw - piece.width - int(0.02 * cw), (ch - text_h) // 2))
        cell = engraved(canvas, 900 + k)
        atlas.paste(cell, ((k % SHUTIAO_GRID[0]) * cw, (k // SHUTIAO_GRID[0]) * ch))
    p = os.path.join(OUT, "T_ch_shutiao.png")
    atlas.save(p)
    print("wrote", p)


if __name__ == "__main__":
    import sys
    os.makedirs(OUT, exist_ok=True)
    only = sys.argv[1:]
    if not only or "suhua" in only:
        make_suhua()
    if not only or "plaques" in only:
        make_plaques()
    if not only or "shutiao" in only:
        make_shutiao()
