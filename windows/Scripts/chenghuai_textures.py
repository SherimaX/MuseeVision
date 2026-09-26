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
BLUE = (40, 60, 76)         # azurite, faded to slate
BLUE_L = (86, 108, 120)
GREEN = (42, 76, 64)        # malachite, dark
GREEN_L = (92, 124, 108)
RED = (118, 54, 42)         # iron red (铁红) / 章丹 board, dulled
GOLD = (168, 138, 80)       # ochre-gold linework (matt)
BLACK = (28, 26, 24)
WHITE = (232, 226, 210)
CREAM = (228, 216, 188)


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
        ew = max(3, h // 60)
        band = Image.new("L", (w, h), 0)
        db = ImageDraw.Draw(band)
        db.rectangle([0, 0, w, ew], fill=255)
        db.rectangle([0, h - ew, w, h], fill=255)
        rag = Image.effect_noise((w, h), 90).point(lambda v: 255 if v > 150 else 0)
        band = ImageChops.multiply(band.filter(ImageFilter.GaussianBlur(ew * 0.8)), rag).filter(ImageFilter.GaussianBlur(0.8))
        base = Image.composite(Image.new("RGB", (w, h), (176, 166, 146)), base, band.point(lambda v: int(v * 0.55)))
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


def beam_row(style, crop, seed, band_top=0.42):
    """One bay's lintel and board (virtual 2970 × 512, stretched to 4096): the board (top 42 %) and the lintel under it."""
    W, H = 2970, 512
    img = Image.new("RGB", (W, H), GREEN)
    d = ImageDraw.Draw(img)
    board_h = int(H * band_top)
    lintel_col = GREEN if style % 2 == 0 else BLUE
    lintel_light = GREEN_L if style % 2 == 0 else BLUE_L
    other = BLUE if style % 2 == 0 else GREEN
    d.rectangle([0, 0, W, board_h], fill=RED)
    d.rectangle([0, board_h, W, H], fill=lintel_col)
    # The lintel's edges: a black line and a white one (the 'eyebrow' lines).
    for y, c in ((board_h + 4, BLACK), (board_h + 10, WHITE), (H - 12, WHITE), (H - 5, BLACK)):
        d.line([(0, y), (W, y)], fill=c, width=4)
    # The end bands (箍头), each end: a dark band with beads, gold lines either side.
    band = int(W * 0.055)
    for x0 in (0, W - band):
        d.rectangle([x0, 0, x0 + band, H], fill=other)
        beads(d, x0 + band * 0.28, 14, x0 + band * 0.72, H - 14, 9, WHITE, lintel_light)
        for x in (x0 + 6, x0 + band - 6):
            d.line([(x, 0), (x, H)], fill=GOLD, width=6)
        # A second, thinner band inside (副箍头).
        xi = x0 + band + 18 if x0 == 0 else x0 - 60
        d.rectangle([xi, 0, xi + 42, H], fill=other)
        d.line([(xi + 21, 0), (xi + 21, H)], fill=WHITE, width=5)
    # The clasps (卡子) in the zones between the bands and the panel.
    for cx, flip in ((W * 0.13, False), (W * 0.87, True)):
        clasp(d, cx, board_h + 20, H - 20, GOLD if style % 2 == 0 else WHITE, flip)
        clasp(d, cx, 14, board_h - 14, lintel_light, flip)
    # The arched panel (包袱): a half-ellipse hanging from the top, its borders the layered 'smoke clouds' (烟云) in five
    # steps from dark to light, the backing (托子) outside them.
    pw = W * 0.38
    cx = W * 0.5
    ph = H * 0.9
    rings = [(24, BLACK), (18, other), (14, lerp(other, WHITE, 0.3)), (12, lerp(other, WHITE, 0.55)), (10, lerp(other, WHITE, 0.8)), (8, GOLD)]
    grow = sum(r for r, _ in rings) + 26
    # Backing.
    d.pieslice([cx - pw / 2 - grow, -ph - grow, cx + pw / 2 + grow, ph + grow], 0, 180, fill=lerp(RED, BLACK, 0.2) if style % 2 == 0 else lerp(GREEN, BLACK, 0.2))
    off = grow
    for r, c in rings:
        d.pieslice([cx - pw / 2 - off, -ph - off, cx + pw / 2 + off, ph + off], 0, 180, fill=c)
        off -= r
    # The panel's ground and its painting.
    panel = Image.new("L", (W, H), 0)
    ImageDraw.Draw(panel).pieslice([cx - pw / 2, -ph, cx + pw / 2, ph], 0, 180, fill=255)
    ground = Image.new("RGB", (W, H), CREAM)
    if crop is not None:
        c = crop.resize((int(pw), int(ph)), Image.LANCZOS)
        # Paler, as mineral paint on a lime ground (not a photograph).
        c = Image.blend(c, Image.new("RGB", c.size, CREAM), 0.35)
        ground.paste(c, (int(cx - pw / 2), 0))
    img = Image.composite(ground, img, panel)
    d = ImageDraw.Draw(img)
    d.arc([cx - pw / 2, -ph, cx + pw / 2, ph], 0, 180, fill=BLACK, width=5)
    img = age(img, seed, fade=0.035)
    return img.resize((4096, 512), Image.LANCZOS)


def plain_row(seed):
    W, H = 2970, 512
    img = Image.new("RGB", (W, H), GREEN)
    d = ImageDraw.Draw(img)
    d.rectangle([0, H * 0.5, W, H], fill=BLUE)
    for x0 in (0, W - 160):
        d.rectangle([x0, 0, x0 + 160, H], fill=BLUE)
        beads(d, x0 + 45, 16, x0 + 115, H - 16, 9, WHITE, GREEN_L)
        d.line([(x0 + 8, 0), (x0 + 8, H)], fill=GOLD, width=6)
        d.line([(x0 + 152, 0), (x0 + 152, H)], fill=GOLD, width=6)
    for cx, flip in ((W * 0.2, False), (W * 0.8, True)):
        clasp(d, cx, 20, H - 20, GOLD, flip)
    d.line([(0, 6), (W, 6)], fill=BLACK, width=6)
    d.line([(0, H - 6), (W, H - 6)], fill=BLACK, width=6)
    return age(img, seed, fade=0.035).resize((4096, 512), Image.LANCZOS)


def purlin_row(seed):
    """A purlin's band, repeated every 3.1 m along it: blue with a brocade of small squares, an end band at the joint."""
    W, H = 2970, 512
    img = Image.new("RGB", (W, H), BLUE)
    d = ImageDraw.Draw(img)
    for x in range(0, W, 60):
        for y in range(40, H - 40, 60):
            d.rectangle([x + 18, y + 18, x + 42, y + 42], outline=BLUE_L, width=4)
    for x0 in (0, W - 120):
        d.rectangle([x0, 0, x0 + 120, H], fill=GREEN)
        d.line([(x0 + 60, 0), (x0 + 60, H)], fill=WHITE, width=10)
        d.line([(x0 + 6, 0), (x0 + 6, H)], fill=GOLD, width=5)
        d.line([(x0 + 114, 0), (x0 + 114, H)], fill=GOLD, width=5)
    return age(img, seed, fade=0.035).resize((4096, 512), Image.LANCZOS)


def rafter_heads(seed):
    """Row 6: eight round rafters' heads; row 7: four flying rafters' heads (then blanks)."""
    row6 = Image.new("RGB", (4096, 512), BLUE)
    row7 = Image.new("RGB", (4096, 512), GREEN)
    for k in range(8):
        cell = Image.new("RGB", (512, 512), BLUE)
        d = ImageDraw.Draw(cell)
        # The dragon's eye: blue, a white ring, a dark pupil; the rim black.
        d.ellipse([10, 10, 502, 502], fill=BLUE, outline=BLACK, width=18)
        r1 = 170 + (k % 3) * 8
        d.ellipse([256 - r1, 256 - r1, 256 + r1, 256 + r1], outline=WHITE, width=34)
        d.ellipse([256 - 70, 256 - 70, 256 + 70, 256 + 70], fill=BLACK)
        d.ellipse([256 - 26, 256 - 26, 256 + 26, 256 + 26], fill=WHITE)
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


def make_suhua():
    crops = landscape_crops()
    rows = []
    for r in range(4):
        rows.append(beam_row(r, crops[r % len(crops)] if crops else None, 100 + r))
    rows.append(plain_row(200))
    rows.append(purlin_row(300))
    r6, r7 = rafter_heads(400)
    rows += [r6, r7]
    atlas = Image.new("RGB", (4096, 4096))
    for i, r in enumerate(rows):
        atlas.paste(r, (0, 512 * i))
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
