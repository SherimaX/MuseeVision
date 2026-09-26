"""
T_water_ripples.png (SourceArt/Textures): a tileable normal map (DirectX) of still water's surface, for M_Water.

A sum of capillary–gravity waves on a periodic domain (integer wave vectors, so it tiles), with the spectrum of
light air over a sheltered pond: short waves (2–20 cm) with random phases and directions, amplitude falling as
k^-1.9, a little stretched along one wind direction. The height field's slopes give the normals.

    python windows/Scripts/water_ripples.py
"""
import math
import os
import random

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "SourceArt", "Textures", "T_water_ripples.png")
N = 512          # pixels across one tile (the material repeats it every 0.6 m and 1.7 m)
WAVES = 260


def main():
    rnd = random.Random(7)
    waves = []
    for _ in range(WAVES):
        # integer wave numbers (cycles per tile): 3 … 30 per 0.6 m tile → 2 … 20 cm wavelengths
        k = rnd.uniform(3, 30)
        a = rnd.gauss(0.35, 0.9)                      # direction: mostly along the light air
        kx, ky = round(k * math.cos(a)), round(k * math.sin(a))
        if kx == 0 and ky == 0:
            continue
        kk = math.hypot(kx, ky)
        amp = kk ** -1.9
        waves.append((kx, ky, amp, rnd.uniform(0, 2 * math.pi)))
    # slopes: dh/dx = sum A k_x 2π/N cos(...)
    sx = [[0.0] * N for _ in range(N)]
    sy = [[0.0] * N for _ in range(N)]
    for kx, ky, amp, ph in waves:
        cx, cy = amp * kx, amp * ky
        for y in range(N):
            base = 2 * math.pi * ky * y / N + ph
            row_x, row_y = sx[y], sy[y]
            for x in range(N):
                c = math.cos(2 * math.pi * kx * x / N + base)
                row_x[x] += cx * c
                row_y[x] += cy * c
    peak = max(max(abs(v) for v in r) for r in sx + sy) or 1.0
    img = Image.new("RGB", (N, N))
    px = img.load()
    for y in range(N):
        for x in range(N):
            dx, dy = sx[y][x] / peak, sy[y][x] / peak
            nx, ny, nz = -dx, dy, 1.0                  # DirectX: green towards the image's top
            l = math.sqrt(nx * nx + ny * ny + nz * nz)
            px[x, y] = tuple(int(round((v / l * 0.5 + 0.5) * 255)) for v in (nx, ny, nz))
    img.save(OUT)
    print(f"{OUT}: {len(waves)} waves")


if __name__ == "__main__":
    main()
