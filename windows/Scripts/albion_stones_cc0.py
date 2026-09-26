"""
Albion's ten column stones from photographed CC0 sets (ambientCG), in place of albion_textures.py's drawn ones: each set is
fetched (2K), graded in linear light to the stone's own colour (the real columns, scratch refs sheet-stones), its roughness
brought into a polished shaft's range, and written as SourceArt/PBR/AL_ST_<stone>/ with its statistics in albion_stats.json.
One repeat is once round a 0.5 m shaft (1.5708 m), so the wrap has no seam.

    python windows/Scripts/albion_stones_cc0.py [Stone ...]

Credits: SourceArt/PBR/AL_ST_*/SET.txt (ambientCG, CC0 1.0).
"""
import io
import json
import os
import sys

import numpy as np
from PIL import Image

import pbr_fetch

HERE = os.path.dirname(os.path.abspath(__file__))
PBR = os.path.normpath(os.path.join(HERE, "..", "SourceArt", "PBR"))
STATS = os.path.join(PBR, "albion_stats.json")
SCALE = 1.5708

# stone: (ambientCG set, the stone's colour (sRGB), polished roughness range, saturation, what)
STONES = {
    "Ashburton": ("Marble011", 0x7E6A62, (0.05, 0.12), 1.15, "Ashburton marble, Devon: grey-pink, red-veined"),
    "Frosterley": ("Marble017", 0x2B2927, (0.05, 0.10), 1.0, "Frosterley marble, Weardale: black, pale coral"),
    "HoptonWood": ("Marble015", 0xD6C8AC, (0.08, 0.16), 1.0, "Hopton Wood stone, Derbyshire: cream"),
    "Iona": ("Marble001", 0xD8DACF, (0.05, 0.11), 1.1, "Iona marble: white, green-streaked"),
    "Mona": ("Marble009", 0x33503A, (0.05, 0.10), 1.1, "Mona marble, Anglesey: green"),
    "Peterhead": ("Granite006A", 0x9A6A5D, (0.06, 0.12), 0.8, "Peterhead granite: red"),
    "Purbeck": ("Marble023", 0x4A443B, (0.06, 0.12), 0.9, "Purbeck marble, Dorset: dark brown-grey"),
    "Rubislaw": ("Granite002A", 0xAEABA5, (0.06, 0.12), 1.0, "Rubislaw granite, Aberdeen: grey"),
    "Serpentine": ("Marble013", 0x3E2E27, (0.04, 0.09), 1.3, "Lizard serpentine, Cornwall: dark red-green"),
    "Tiree": ("Marble010", 0xC49A90, (0.05, 0.11), 1.15, "Tiree marble: pink"),
}


def lin(c):
    s = np.array([(c >> 16) & 255, (c >> 8) & 255, c & 255], np.float32) / 255.0
    return np.where(s <= 0.04045, s / 12.92, ((s + 0.055) / 1.055) ** 2.4)


def to_lin(a):
    return np.where(a <= 0.04045, a / 12.92, ((a + 0.055) / 1.055) ** 2.4)


def to_srgb8(l):
    l = np.clip(l, 0, 1)
    s = np.where(l <= 0.0031308, l * 12.92, 1.055 * np.power(l, 1 / 2.4) - 0.055)
    return (s * 255 + 0.5).astype(np.uint8)


def build(name):
    asset, colour, (r0, r1), sat, what = STONES[name]
    maps = pbr_fetch.ambientcg(asset, "2K")
    load = lambda k, m: Image.open(io.BytesIO(maps[k])).convert(m)  # noqa: E731
    c = to_lin(np.asarray(load("color", "RGB")).astype(np.float32) / 255.0)
    size = (c.shape[1], c.shape[0])
    # Grade: saturation about the luminance, then a per-channel gain to the stone's mean.
    y = (c @ np.array([0.2126, 0.7152, 0.0722], np.float32))[..., None]
    c = y + (c - y) * sat
    c = np.clip(c, 0, None) * (lin(colour) / np.maximum(c.reshape(-1, 3).mean(axis=0), 1e-4))
    d = os.path.join(PBR, "AL_ST_" + name)
    os.makedirs(d, exist_ok=True)
    Image.fromarray(to_srgb8(c)).save(os.path.join(d, "Color.png"))
    load("normal", "RGB").resize(size).save(os.path.join(d, "Normal.png"))
    rough = np.asarray(load("rough", "L").resize(size)).astype(np.float32) / 255.0 if "rough" in maps else np.full(size[::-1], 0.5, np.float32)
    rn = (rough - rough.min()) / max(1e-6, rough.max() - rough.min())
    rough = r0 + (r1 - r0) * rn
    ao = np.full_like(rough, 1.0)
    orm = np.stack([ao, rough, np.zeros_like(rough)], axis=-1)
    Image.fromarray((orm * 255 + 0.5).astype(np.uint8)).save(os.path.join(d, "ORM.png"))
    if "height" in maps:
        load("height", "L").resize(size).save(os.path.join(d, "Height.png"))
    with open(os.path.join(d, "SET.txt"), "w", encoding="utf-8") as f:
        f.write(f"AL_ST_{name}\tambientcg\t{asset}\t2K\t{SCALE}\t{what} (graded; ambientCG CC0 1.0, https://ambientcg.com/view?id={asset})\n")
    st = {"color_mean": [round(float(max(m, 1e-4)), 4) for m in c.reshape(-1, 3).mean(axis=0)], "rough_mean": round(float(rough.mean()), 4),
          "scale": SCALE, "has_height": "height" in maps}
    print(f"AL_ST_{name} <- {asset}: mean {st['color_mean']} rough {st['rough_mean']}")
    return st


def main(names):
    with open(STATS, encoding="utf-8") as f:
        stats = json.load(f)
    for n in names or list(STONES):
        stats["AL_ST_" + n] = build(n)
    with open(STATS, "w", encoding="utf-8") as f:
        json.dump(stats, f, indent=1)


if __name__ == "__main__":
    main(sys.argv[1:])
