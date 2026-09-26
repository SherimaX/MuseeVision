"""
The Hall of Light's four stereographs, as they sit on their bronze readers: T_stereo_1.png … T_stereo_4.png
in SourceArt/Textures.

Each reader's picture is a 0.60 × 0.32 m quad whose UVs take the left half of its image (u 0 to 0.5: on the
iPhone two quads, one per half, alternated to show the depth). Given the plain card it showed the left view
stretched to 1.875:1 and off centre. Here the whole card, both views, undistorted and centred on its own
mount colour, fills a 1.875:1 panel, and the image is that panel twice side by side, so either quad shows it
(relight.py points painting_stereo_N at T_stereo_N).

    python windows/Scripts/stereo_cards.py
"""
import os
from PIL import Image, ImageStat

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "..", "..", "assets", "paintings")
OUT = os.path.join(HERE, "..", "SourceArt", "Textures")
ASPECT = 0.60 / 0.32
PANEL_H = 1024
MARGIN = 0.04   # of the panel's height, round the card


def panel(card):
    W, H = round(PANEL_H * ASPECT), PANEL_H
    # The card's own edge colour (a strip round it) for the mount it sits on.
    edge = [card.crop(b) for b in ((0, 0, card.width, 8), (0, card.height - 8, card.width, card.height))]
    colour = tuple(int(sum(ImageStat.Stat(e).median[c] for e in edge) / len(edge)) for c in range(3))
    img = Image.new("RGB", (W, H), colour)
    s = min((W - 2 * MARGIN * H) / card.width, (H - 2 * MARGIN * H) / card.height)
    c = card.resize((round(card.width * s), round(card.height * s)), Image.LANCZOS)
    img.paste(c, ((W - c.width) // 2, (H - c.height) // 2))
    return img


def main():
    for i in range(1, 5):
        p = panel(Image.open(os.path.join(SRC, f"stereo-{i}.jpg")).convert("RGB"))
        twice = Image.new("RGB", (p.width * 2, p.height))
        twice.paste(p, (0, 0))
        twice.paste(p, (p.width, 0))
        twice.save(os.path.join(OUT, f"T_stereo_{i}.png"))
    print(f"4 stereo cards in {os.path.abspath(OUT)}")


if __name__ == "__main__":
    main()
