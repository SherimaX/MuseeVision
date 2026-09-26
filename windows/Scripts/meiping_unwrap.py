# Unwraps the Commons photo of the Nanjing meiping (Xiao_He_meiping.jpg, CC BY-SA 3.0, in the working
# directory as meiping_full.jpg) onto the Chinese Wing model's lathe UVs (Shared/Wings/ChineseWing.swift, v by
# profile point): writes mei_tex.png, which goes to windows/SourceArt/Textures/T_meiping.png.
from PIL import Image; import numpy as np, cv2
from scipy import ndimage
src = cv2.imread('meiping_full.jpg')[:, :, ::-1].astype(np.float32) / 255.0
H, W, _ = src.shape
# The glass's reflection: a thin vertical bright line; inpaint it away.
g = src.mean(2)
# (a hairline near x 785 from the lip to the foot, drifting a little: traced row by row)
hp = g - ndimage.uniform_filter1d(g, 21, axis=1)
line = np.zeros(g.shape, bool)
ys, xs = [], []
for y in range(100, 2480, 4):
    seg = hp[y, 760:810]
    if seg.max() > 0.05:
        ys.append(y); xs.append(760 + int(np.argmax(seg)))
fit = np.polyfit(ys, xs, 2)
for y in range(110, 2480):
    x = int(round(np.polyval(fit, y)))
    line[y, x - 6:x + 7] = True
for y in range(100, 700):                              # near the lip the line bends off the fit: follow it
    seg = hp[y, 770:830]
    if seg.max() > 0.03:
        x = 770 + int(np.argmax(seg)); line[y - 2:y + 3, x - 6:x + 7] = True
print('line px', line.sum())
src8 = (src * 255).astype(np.uint8)
src8 = cv2.inpaint(src8[:, :, ::-1].copy(), line.astype(np.uint8), 5, cv2.INPAINT_TELEA)[:, :, ::-1]
src = src8.astype(np.float32) / 255.0
# Geometry (measured): lip top y 130, foot y 2472 (44 cm); photo radius = 0.87 x the model's; axis x 890 at y 720, 949 at y 2295.
FOOT, PXCM, K = 2472.0, (2472.0 - 130.0) / 44.0, 0.87
prof = [(0, 0), (6.5, 0), (7.2, 4), (9.2, 12), (12.2, 22), (13.8, 30), (13.2, 35), (9.5, 39), (4.0, 41), (3.0, 42), (3.6, 42.6), (3.6, 44), (0, 44)]
N = len(prof) - 1
OW, OH = 2048, 2048
out = np.zeros((OH, OW, 3), np.float32)
u = (np.arange(OW) + 0.5) / OW
t = u * 2 * np.pi
# The photo shows one side: its front 140 degrees (the least foreshortened) go round twice, each copy
# cross-faded into the next over 20 degrees of the photo, so no seam and no mirror image.
SPAN = np.radians(140.0); FADE = np.radians(20.0)
half = (t % np.pi) / np.pi                            # 0..1 along each copy
P = SPAN - FADE                                        # each copy's own share of the photo
xa = half * P                                          # 0..P along the photo's strip
phi = -SPAN / 2 + xa
phi_next = phi + P                                     # the strip's last FADE, blended into each copy's start
w_next = np.clip(1.0 - xa / FADE, 0, 1)
for r in range(OH):
    v = 1.0 - (r + 0.5) / OH
    k = v * N
    i = min(int(k), N - 1); f = k - i
    R = prof[i][0] + (prof[i + 1][0] - prof[i][0]) * f
    h = prof[i][1] + (prof[i + 1][1] - prof[i][1]) * f
    y = FOOT - h * PXCM
    cx = 890 + (949 - 890) * (y - 720) / (2295 - 720)
    def sample(ph):
        x = cx + K * R * PXCM * np.sin(ph)
        yy = np.clip(np.full_like(x, y), 0, H - 1); xx = np.clip(x, 0, W - 1)
        return np.stack([ndimage.map_coordinates(src[:, :, c], [yy, xx], order=1) for c in range(3)], -1)
    a, b = sample(phi), sample(phi_next)
    out[r] = a * (1 - w_next[:, None]) + b * w_next[:, None]
# Flatten the photo's light: divide by the glaze's own white level (a large closing of the luminance).
L = out.mean(2)
white = ndimage.gaussian_filter(ndimage.grey_closing(L, size=(121, 121)), 40)
white = np.maximum(white, np.percentile(L, 70))
glaze = np.array([0xF1, 0xF0, 0xE8]) / 255.0
flat = np.clip(out / white[..., None] * glaze, 0, 1)
# Rows outside the painted body (the base underside, the lip's top and inside): plain glaze.
cv2.imwrite('mei_tex.png', (flat[:, :, ::-1] * 255).astype(np.uint8))
print('ok')
