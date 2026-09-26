"""
Flicker check: a burst of frames from a camera that doesn't move; anything that changes between them is flicker
(z-fighting, Lumen or ray-traced noise that boils, MegaLights noise, thin geometry shimmering, translucency
popping, exposure hunting). Writes a heatmap per view and prints a score.

Take the burst with MuseeDo, e.g. for view <id> at pose x,y,feet,yaw,pitch (hold still ~8 s first so Lumen settles):
    cmd:musee.Hour 14@20;tp:x,y,feet@20.1;look:yaw,pitch@20.2;
    shot:fl-<id>-0@28;shot:fl-<id>-1@28.4;shot:fl-<id>-2@28.8; ... (8 frames, 0.4 s apart)
then:
    python flicker_check.py <MuseeDo dir> <id> [<id> ...]

Raw score = 99.5th percentile of the per-pixel temporal standard deviation of luminance (0–255 scale), with animated
things (water, plants, the sky) still counted: judge the heatmap, not only the number. Rough guide: < 2 steady;
2–5 check the hot spots; > 5 visible flicker. The verdict uses the beyond-jitter score (the raw score minus the upscaler's
sub-pixel edge jitter), so edge-dense views aren't mistaken for flicker. Output: <dir>/flicker/<id>_heat.jpg (hot = flicker) and
<id>_sheet.jpg (frame 0 | heat overlay).
"""
import glob
import os
import re
import sys

import numpy as np
from PIL import Image


def check(folder, vid):
    # Only this view's frames: "fl-reserve-*" would also match fl-reserve-rack-* and stack three views.
    pattern = re.compile(rf"fl-{re.escape(vid)}-(\d+)\.png$")
    frames = sorted((p for p in glob.glob(os.path.join(folder, f"fl-{vid}-*.png")) if pattern.search(os.path.basename(p))),
                    key=lambda p: int(pattern.search(os.path.basename(p)).group(1)))
    if len(frames) < 3:
        print(f"{vid}: need at least 3 frames, found {len(frames)}")
        return None
    stack = np.stack([np.asarray(Image.open(f).convert("L"), dtype=np.float32) for f in frames])
    std = stack.std(axis=0)
    score = float(np.percentile(std, 99.5))
    # Jitter-tolerant score: how far each frame falls outside the mean image's own 3x3 neighbourhood range. The
    # upscaler (DLSS/TSR) jitters high-contrast edges by a sub-pixel each frame; that stays inside the range and
    # scores ~0, while real flicker (z-fighting, boiling, sparkles) falls outside it.
    mean = stack.mean(axis=0)
    pad = np.pad(mean, 1, mode="edge")
    shifts = [pad[1 + dy:1 + dy + mean.shape[0], 1 + dx:1 + dx + mean.shape[1]] for dy in (-1, 0, 1) for dx in (-1, 0, 1)]
    lo, hi = np.min(shifts, axis=0), np.max(shifts, axis=0)
    beyond = np.maximum(stack - hi, 0) + np.maximum(lo - stack, 0)
    jitter_free = float(np.percentile(beyond.max(axis=0), 99.5))
    out = os.path.join(folder, "flicker")
    os.makedirs(out, exist_ok=True)
    heat = np.clip(std * 25.0, 0, 255).astype(np.uint8)            # std 10 -> full red
    heat_rgb = np.stack([heat, np.zeros_like(heat), np.zeros_like(heat)], axis=-1)
    Image.fromarray(heat_rgb).save(os.path.join(out, f"{vid}_heat.jpg"), quality=90)
    base = Image.open(frames[0]).convert("RGB")
    overlay = Image.blend(base, Image.fromarray(heat_rgb).resize(base.size), 0.55)
    w, h = base.size
    sheet = Image.new("RGB", (w * 2, h))
    sheet.paste(base, (0, 0))
    sheet.paste(overlay, (w, 0))
    sheet.save(os.path.join(out, f"{vid}_sheet.jpg"), quality=88)
    hot = float((std > 5).mean() * 100)
    verdict = "steady" if jitter_free < 2 else ("check hot spots" if jitter_free < 5 else "FLICKER")
    print(f"{vid}: score {score:.2f} (raw), beyond-jitter {jitter_free:.2f}, {hot:.2f}% of pixels > 5, "
          f"{len(frames)} frames -> {verdict}")
    return score


if __name__ == "__main__":
    d = sys.argv[1]
    for v in sys.argv[2:]:
        check(d, v)
