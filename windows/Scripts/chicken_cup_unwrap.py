# Unwraps the National Palace Museum's photograph of the Chenghua chicken cup (assets/reference/chinese-ref-chicken-cup.jpg,
# CC BY 4.0, NPM Open Data) onto the Chinese Wing model's lathe UVs (Shared/Wings/ChineseWing.swift: v by profile point,
# 10 points, the outer wall from point 1 (foot) to 5 (lip)), for windows/SourceArt/Textures/T_chicken_cup.png.
# The photo's camera: about 9 degrees above the rim (the rim's ellipse 860 x 138 px), 104.9 px/cm, the axis at x 532,
# the foot's front at y 578; the cup photographs 6.5 % taller than the model's 3.4 cm.
import os
import numpy as np
from PIL import Image
from scipy import ndimage

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
src = np.asarray(Image.open(os.path.join(REPO, "assets", "reference", "chinese-ref-chicken-cup.jpg")).convert("RGB")).astype(np.float32) / 255
H, W, _ = src.shape
CX, S, SIN = 532.0, 430.0 / 4.1, 0.1605
COS = np.sqrt(1 - SIN ** 2)
HSCALE = 1.065
CY0 = 578.0 - 1.9 * S * SIN                      # the axis at the foot (h 0)
prof = [(0, 0), (1.9, 0), (2.1, 0.4), (2.6, 0.5), (3.4, 1.6), (4.1, 3.4), (3.9, 3.4), (3.2, 1.8), (2.4, 0.8), (0, 0.8)]
N = len(prof) - 1
OW, OH = 2048, 1024
glaze = np.array([0xF4, 0xF3, 0xEC], np.float32) / 255
out = np.tile(glaze, (OH, OW, 1))
u = (np.arange(OW) + 0.5) / OW
t = u * 2 * np.pi
SPAN, FADE = np.radians(130.0), np.radians(20.0)  # the front 130 degrees go round twice (the cup has two families)
P = SPAN - FADE
half = (t % np.pi) / np.pi
xa = half * P
phi = -SPAN / 2 + xa
phi_next = phi + P
w_next = np.clip(1.0 - xa / FADE, 0, 1)
for r in range(OH):
    k = (1.0 - (r + 0.5) / OH) * N
    if k < 1.0 or k > 5.0:
        continue                                  # the foot's underside, the rim's top and the inside: plain glaze
    i = min(int(k), N - 1); f = k - i
    R = prof[i][0] + (prof[i + 1][0] - prof[i][0]) * f
    h = (prof[i][1] + (prof[i + 1][1] - prof[i][1]) * f) * HSCALE
    def sample(ph):
        x = CX + S * R * np.sin(ph)
        y = CY0 - S * COS * h + S * SIN * R * np.cos(ph)
        return np.stack([ndimage.map_coordinates(src[:, :, c], [np.clip(y, 0, H - 1), np.clip(x, 0, W - 1)], order=1) for c in range(3)], -1)
    out[r] = sample(phi) * (1 - w_next[:, None]) + sample(phi_next) * w_next[:, None]
# Flatten the studio light: divide by the glaze's own white (a large closing of the luminance) on the painted rows.
L = out.mean(2)
white = ndimage.gaussian_filter(ndimage.grey_closing(L, size=(41, 81)), 20)
white = np.maximum(white, 0.35)
flat = np.clip(out / white[..., None] * glaze, 0, 1)
Image.fromarray((flat * 255 + 0.5).astype(np.uint8)).save(os.path.join(HERE, "..", "SourceArt", "Textures", "T_chicken_cup.png"))
print("ok")
