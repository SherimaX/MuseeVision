"""
The Chinese Wing's lettering in clerical script (隸書, the Ministry of Education's font, SourceArt/Fonts), as a
Suzhou garden cuts and gilds it:

- the solar-term stele (the vestibule): the term in two large gilt characters on the stone, framed, and
  nothing else; T_stele_00.png … T_stele_23.png (0 = 立春, as MuseeEphemeris::SolarTerm). The game shows
  today's (AMuseeGameMode::ShowTodaysSolarTerm);
- the plaque over the moon gate, 四時園, read right to left, gilt on dark lacquer in a bronze-brown frame:
  T_plaque.png.

The font lacks 蟄 (驚蟄): it is set from its own 執 over 虫, as the character is built.

    python windows/Scripts/stele_inscriptions.py
"""
import os
from PIL import Image, ImageDraw, ImageFilter, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "SourceArt", "Textures")
FONT = os.path.join(HERE, "..", "SourceArt", "Fonts", "MoeLI-3.0.ttf")
TERMS = ["立春", "雨水", "驚蟄", "春分", "清明", "穀雨", "立夏", "小滿", "芒種", "夏至", "小暑", "大暑",
         "立秋", "處暑", "白露", "秋分", "寒露", "霜降", "立冬", "小雪", "大雪", "冬至", "小寒", "大寒"]
GILT, GILT_SHADE = (0xC4, 0x9A, 0x4E), (0x7A, 0x5E, 0x2C)


def glyph(ch, size):
    """One character as a white-on-black mask, trimmed to its ink."""
    font = ImageFont.truetype(FONT, size)
    if ch == "蟄":
        top, bottom = glyph("執", size), glyph("虫", size)
        w = max(top.width, bottom.width)
        top = top.resize((w, int(top.height * 0.55)))
        bottom = bottom.resize((int(w * 0.92), int(bottom.height * 0.5)))
        m = Image.new("L", (w, top.height + bottom.height - size // 40))
        m.paste(top, (0, 0))
        m.paste(bottom, ((w - bottom.width) // 2, top.height - size // 40), bottom)
        return m
    m = Image.new("L", (size * 2, size * 2))
    ImageDraw.Draw(m).text((size // 2, size // 2), ch, font=font, fill=255)
    return m.crop(m.getbbox())


def carve(img, mask, x, y):
    """Set a gilt character into the stone: a shadowed incised edge, then the gilt."""
    shade = mask.filter(ImageFilter.GaussianBlur(max(1, mask.width // 160)))
    img.paste(Image.new("RGB", mask.size, GILT_SHADE), (x + mask.width // 200 + 1, y + mask.width // 200 + 1), shade)
    img.paste(Image.new("RGB", mask.size, GILT), (x, y), mask)


def stele(term):
    W, H = 720, 1360
    img = Image.new("RGB", (W, H), (0xCF, 0xC6, 0xB8))
    d = ImageDraw.Draw(img)
    d.rectangle([40, 40, W - 41, H - 41], outline=(0xB2, 0xA8, 0x97), width=10)
    d.rectangle([62, 62, W - 63, H - 63], outline=(0xBD, 0xB3, 0xA3), width=3)
    # Both characters at one size, each centred in its half of the stone (clerical script is broad and low).
    size = 540
    cell = (H - 200) // 2
    for k, ch in enumerate(TERMS[term]):
        c = glyph(ch, size)
        cy = 100 + cell * k + cell // 2
        carve(img, c, (W - c.width) // 2, cy - c.height // 2)
    return img


def plaque():
    W, H = 1932, 636
    img = Image.new("RGB", (W, H), (0x6E, 0x4F, 0x22))
    d = ImageDraw.Draw(img)
    d.rectangle([24, 24, W - 25, H - 25], fill=(0x2C, 0x25, 0x1F))
    d.rectangle([48, 48, W - 49, H - 49], outline=(0x8A, 0x6A, 0x33), width=4)
    # One size, natural proportions, each centred in its third; read right to left (四 on the right).
    for k, ch in enumerate("園時四"):
        c = glyph(ch, 440)
        cx = 80 + (W - 160) * (2 * k + 1) / 6
        carve(img, c, int(cx - c.width / 2), (H - c.height) // 2)
    return img


def main():
    for i in range(24):
        stele(i).save(os.path.join(OUT, f"T_stele_{i:02d}.png"))
    plaque().save(os.path.join(OUT, "T_plaque.png"))
    print(f"24 stele inscriptions and the plaque in {os.path.abspath(OUT)}")


if __name__ == "__main__":
    main()
