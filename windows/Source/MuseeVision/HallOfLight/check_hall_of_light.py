"""
A Python port of HallOfLightStructure.cpp's geometry (HallOfLightBuild), and the checks it must pass:

    python check_hall_of_light.py            (add --no-rays to skip the light-leak test)

1. Closed solids: every member, stone and stem is a closed 2-manifold (its edges, welded at 0.05 mm,
   each used once in each direction), with a positive volume (its normals face out).
2. Winding: every triangle's face normal agrees with its vertices' normals.
3. The floor and the glass: their open edges lie only where they run into a solid (under the plinths,
   inside the drums' walls, on the sun clock's and the Atrium floor's edges): no gaps, no T-junctions.
4. Z-fighting: no face lies within 1 cm of a parallel face of another part (or of the neighbours the
   hall meets: sun clock, Ground, the drums, the Atrium's coping and doors, the prints' mounts), unless
   they share their vertices.
5. Leaks: rays from inside the hall in every direction all end on a surface (glass counts: that is
   the sky), with the Rotunda and the Atrium taken as closed drums.
6. UVs are in metres (a triangle's UV area matches its area on its plane's axes to within 50 %).

Keep it in step with the C++ (same numbers, same order of construction).
"""
import math
import sys
from collections import defaultdict

import numpy as np

# ------------------------------------------------------------------------------------------------
# MuseePlan

ROT_RADIUS, ROT_WALL = 10.0, 1.2
ROT_DOOR_HALF, ROT_DOOR_SPRING = 1.5, 4.5
ELAN_X, ELAN_Y, ELAN_R, ELAN_WALL, ELAN_BASE = 54.0, 0.0, 14.0, 0.8, 6.0
ELAN_DOOR_HALF, ELAN_DOOR_H = 2.0, 4.5

HW, EAVES_H, PLINTH_H = 4.5, 5.0, 0.3
VR, VC = 5.3, 2.2
TRANSOM_H = 3.2
FIRST_POST, POST_STEP, POSTS = 11.0, 3.0, 10
FIRST_ARCH, ARCH_STEP, ARCHES = 12.5, 1.5, 19
ROT_OUTER = ROT_RADIUS + ROT_WALL
SUN_R, SUN_SEG = 10.3, 128
ATR_OUTER, ATR_DRUM = ELAN_R + ELAN_WALL, ELAN_R
ATR_FLOOR_R, ATR_FLOOR_SEG = 14.3, 96
STEREO = [(17, -2.4), (23, 2.4), (29, -2.4), (35, 2.4)]
STEREO_R, STEREO_TOP, READER_H = 0.35, 1.0, 1.32
VIEWING = [14, 20, 26, 32, 38]
PRINT_BAYS = [12.5, 15.5, 18.5, 21.5, 24.5, 27.5, 30.5, 33.5, 36.5, 38.95]
FIRST_AUTOCHROME = 8
MOUNT_HALF, MOUNT_BOTTOM, MOUNT_TOP, MOUNT_BACK = 0.5, 1.15, 2.05, 4.38

# HallOfLightBuild
EMBED, SEAT, PLINTH_EMBED, KERB_EMBED = 0.028, 0.014, 0.04, 0.055
DRUM_GAP = 0.012
PURLIN_DRUM_GAP = DRUM_GAP + 0.012
GLASS_FOOT = PLINTH_H - 0.01
POST_FOOT = PLINTH_H - 0.02
SQUARE_STEPS = 8
BAR_CLEAR = 0.025
STRIPS_PER_PANEL = 6
LATHE_SEG, ARCH_SEG, DISC_SEG = 64, 48, 96
FLOOR_EDGE = HW - 0.16
FLOOR_WEST_X, FLOOR_EAST_X = 11.3, 39.1
STONE_CELL, STONE_DISC, STONE_RING = 0.6, 0.40, 0.45
IN_ROT_WALL, IN_ATR_WALL = 10.7, 14.6
EAVES_BOTTOM, EAVES_TOP = 4.86, 5.10
POST_TOP = 4.90
TRANSOM_HALF = 0.03
SHOE_TOP = 0.34
PI = math.pi


def mix(a, b, t):
    return a * (1.0 - t) + b * t


def cross2(a, b):
    return a[0] * b[1] - a[1] * b[0]


def rot_face_x(y):
    return math.sqrt(ROT_OUTER * ROT_OUTER - y * y)


def atr_face_x(y, drum):
    ra = ATR_DRUM if drum else ATR_OUTER
    return ELAN_X - math.sqrt(ra * ra - y * y)


def west_end(y):
    return rot_face_x(y) - EMBED


def east_end(y, drum):
    return atr_face_x(y, True) - PURLIN_DRUM_GAP if drum else atr_face_x(y, False) + EMBED


def glass_east_end(y, drum):
    return atr_face_x(y, drum) + EMBED


def vault_point(x, theta, radius):
    return np.array([x, radius * math.sin(theta), VC + radius * math.cos(theta)])


def spring_angle():
    return math.asin(HW / VR)


def coping_angle():
    return math.acos((ELAN_BASE - VC) / VR)


def purlin_angles():
    s, c = spring_angle(), coping_angle()
    a = [-s] + [-c + 2.0 * c * k / 6.0 for k in range(7)] + [s]
    return a


def strip_angles():
    p = purlin_angles()
    a = []
    for k in range(len(p) - 1):
        for j in range(STRIPS_PER_PANEL):
            a.append(mix(p[k], p[k + 1], j / STRIPS_PER_PANEL))
    a.append(p[-1])
    return a


def between(t0, t1, steps):
    return [mix(t0, t1, i / steps) for i in range(steps + 1)]


def post_x(k):
    return FIRST_POST + POST_STEP * k


def arch_x(k):
    return FIRST_ARCH + ARCH_STEP * k


# ------------------------------------------------------------------------------------------------
# Mesh data (SalonKit::FMeshData, in metres)

class Mesh:
    def __init__(self, name):
        self.name = name
        self.p, self.n, self.uv, self.tris = [], [], [], []
        self.solids = []          # (label, first triangle, end triangle)
        self._open = None

    def vertex(self, p, n, uv):
        n = np.asarray(n, float)
        ln = np.linalg.norm(n)
        self.n.append(n / ln if ln > 0 else n)
        self.uv.append((float(uv[0]), float(uv[1])))
        self.p.append(np.asarray(p, float))
        return len(self.p) - 1

    def tri(self, a, b, c):
        pa, pb, pc = self.p[a], self.p[b], self.p[c]
        x = np.cross(pb - pa, pc - pa)
        if np.dot(x, x) * 1e8 < 1e-10:
            return
        if np.dot(x, self.n[a] + self.n[b] + self.n[c]) < 0.0:
            self.tris.append((a, b, c))
        else:
            self.tris.append((a, c, b))

    def quad(self, a, b, c, d):
        self.tri(a, b, c)
        self.tri(a, c, d)

    def begin(self, label):
        self._open = (label, len(self.tris))

    def end(self):
        label, first = self._open
        self.solids.append((label, first, len(self.tris)))
        self._open = None


def polygon_area(p):
    return 0.5 * sum(cross2(p[i], p[(i + 1) % len(p)]) for i in range(len(p)))


def triangulate(poly):
    out = []
    count = len(poly)
    if count < 3:
        return out
    orient = 1.0 if polygon_area(poly) > 0.0 else -1.0
    left = list(range(count))

    def sub(a, b):
        return (a[0] - b[0], a[1] - b[1])

    def turning(a, b, c):
        return cross2(sub(poly[b], poly[a]), sub(poly[c], poly[a])) * orient

    def inside(q, a, b, c):
        return (cross2(sub(poly[b], poly[a]), sub(q, poly[a])) * orient >= -1e-12
                and cross2(sub(poly[c], poly[b]), sub(q, poly[b])) * orient >= -1e-12
                and cross2(sub(poly[a], poly[c]), sub(q, poly[c])) * orient >= -1e-12)

    while len(left) > 3:
        ear = None
        for i in range(len(left)):
            a, b, c = left[(i - 1) % len(left)], left[i], left[(i + 1) % len(left)]
            if turning(a, b, c) <= 1e-12:
                continue
            if any(o not in (a, b, c) and inside(poly[o], a, b, c) for o in left):
                continue
            ear = i
            break
        if ear is None:
            print(f"  ! triangulate: stuck with {len(left)} points")
            break
        out += [left[(ear - 1) % len(left)], left[ear], left[(ear + 1) % len(left)]]
        left.pop(ear)
    if len(left) == 3 and turning(*left) > 1e-12:
        out += left
    return out


def solid(m, section, ts, place, cap_start=True, cap_end=True, label=None):
    """HallOfLightBuild::Solid. place(t, (a, b)) -> np.array."""
    own = label is not None
    if own:
        m.begin(label)
    npnt = len(section)
    orient = 1.0 if polygon_area(section) > 0.0 else -1.0
    step = 1e-6 * max(1.0, abs(ts[-1] - ts[0]))
    v0 = 0.0
    for j in range(npnt):
        a, b = section[j], section[(j + 1) % npnt]
        d = (b[0] - a[0], b[1] - a[1])
        ln = math.hypot(*d)
        out = (d[1] / ln * orient, -d[0] / ln * orient)
        mid = ((a[0] + b[0]) / 2, (a[1] + b[1]) / 2)
        base = len(m.p)
        prev_a = prev_b = None
        ua = ub = 0.0
        for i, t in enumerate(ts):
            pa, pb = place(t, a), place(t, b)
            if i > 0:
                ua += np.linalg.norm(pa - prev_a)
                ub += np.linalg.norm(pb - prev_b)
            along = place(t + step, mid) - place(t - step, mid)
            hint = place(t, (mid[0] + out[0] * 1e-4, mid[1] + out[1] * 1e-4)) - place(t, mid)
            nrm = np.cross(along, pb - pa)
            if np.dot(nrm, nrm) <= 1e-30:
                nrm = hint
            elif np.dot(nrm, hint) < 0:
                nrm = -nrm
            m.vertex(pa, nrm, (ua, v0))
            m.vertex(pb, nrm, (ub, v0 + ln))
            prev_a, prev_b = pa, pb
        for i in range(len(ts) - 1):
            k = base + 2 * i
            m.quad(k, k + 2, k + 3, k + 1)
        v0 += ln
    tris = triangulate(section)
    cen = (sum(q[0] for q in section) / npnt, sum(q[1] for q in section) / npnt)
    for is_end in (False, True):
        if (is_end and not cap_end) or (not is_end and not cap_start):
            continue
        t = ts[-1] if is_end else ts[0]
        nrm = (place(t + step, cen) - place(t - step, cen)) * (1.0 if is_end else -1.0)
        base = len(m.p)
        for q in section:
            m.vertex(place(t, q), nrm, q)
        for k in range(0, len(tris) - 2, 3):
            m.tri(base + tris[k], base + tris[k + 1], base + tris[k + 2])
    if own:
        m.end()


def rect(a0, a1, b0, b1):
    return [(a0, b0), (a1, b0), (a1, b1), (a0, b1)]


def i_section(hl, hw_, hh, b0, b1, b2, b3):
    return [(-hl, b0), (hl, b0), (hl, b1), (hw_, b1), (hw_, b2), (hh, b2), (hh, b3), (-hh, b3),
            (-hh, b2), (-hw_, b2), (-hw_, b1), (-hl, b1)]


def lathe(m, cx, cy, section, label):
    solid(m, section, between(0.0, 2.0 * PI, LATHE_SEG),
          lambda t, q: np.array([cx + q[0] * math.cos(t), cy + q[0] * math.sin(t), q[1]]), False, False, label)


def along_side(m, side, section, from_x, to_x, label):
    def place(t, q):
        y = side * (HW - q[0])
        return np.array([mix(from_x(y), to_x(y), t), y, q[1]])
    solid(m, section, [0.0, 1.0], place, label=label)


def upright(m, side, x, section, z0, z1, label):
    solid(m, section, [0.0, 1.0], lambda t, q: np.array([x + q[0], side * (HW - q[1]), mix(z0, z1, t)]), label=label)


# ------------------------------------------------------------------------------------------------
# The parts

class Parts:
    def __init__(self):
        for name in ("floor", "viewing", "rings", "stone", "mouldings", "frame", "lattice", "glass", "bronze"):
            setattr(self, name, Mesh(name))


def build_floor(out):
    up = (0, 0, 1)

    def plane(m, poly):
        tris = triangulate(poly)
        base = len(m.p)
        for p in poly:
            m.vertex((p[0], p[1], 0.0), up, p)
        for k in range(0, len(tris) - 2, 3):
            m.tri(base + tris[k], base + tris[k + 1], base + tris[k + 2])

    step = 2 * PI / SUN_SEG
    span = math.ceil(math.asin(ROT_DOOR_HALF / SUN_R) / step) + 1
    xin = math.sqrt(IN_ROT_WALL ** 2 - FLOOR_EDGE ** 2)
    west = [(math.cos(i * step) * SUN_R, math.sin(i * step) * SUN_R) for i in range(-span, span + 1)]
    west += [(xin, FLOOR_EDGE), (FLOOR_WEST_X, FLOOR_EDGE), (FLOOR_WEST_X, STONE_CELL), (FLOOR_WEST_X, -STONE_CELL),
             (FLOOR_WEST_X, -FLOOR_EDGE), (xin, -FLOOR_EDGE)]
    plane(out.floor, west)

    step = 2 * PI / ATR_FLOOR_SEG
    half = ATR_FLOOR_SEG // 2
    span = math.ceil(math.asin(ELAN_DOOR_HALF / ATR_FLOOR_R) / step) + 1
    xin = ELAN_X - math.sqrt(IN_ATR_WALL ** 2 - FLOOR_EDGE ** 2)
    east = [(FLOOR_EAST_X, -FLOOR_EDGE), (FLOOR_EAST_X, -STONE_CELL), (FLOOR_EAST_X, STONE_CELL), (FLOOR_EAST_X, FLOOR_EDGE),
            (xin, FLOOR_EDGE)]

    def poly(i):
        return (ELAN_X + math.cos(i * step) * ATR_FLOOR_R, ELAN_Y + math.sin(i * step) * ATR_FLOOR_R)
    for i in range(half - span, half + span + 1):
        east.append(poly(i))
        if i == half + span:
            break
        p, q = poly(i), poly(i + 1)
        for y in (ELAN_DOOR_HALF, -ELAN_DOOR_HALF):
            dp, dq = p[1] - ELAN_Y - y, q[1] - ELAN_Y - y
            if dp * dq < 0:
                t = dp / (dp - dq)
                east.append((p[0] + (q[0] - p[0]) * t, p[1] + (q[1] - p[1]) * t))
    east.append((xin, -FLOOR_EDGE))
    plane(out.floor, east)

    def square_loop(sx):
        corners = [(STONE_CELL, STONE_CELL), (-STONE_CELL, STONE_CELL), (-STONE_CELL, -STONE_CELL), (STONE_CELL, -STONE_CELL)]
        loop = []
        for c in range(4):
            a, b = corners[c], corners[(c + 1) % 4]
            for k in range(SQUARE_STEPS):
                t = k / SQUARE_STEPS
                loop.append((sx + a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t))
        return loop
    square_points = [q for sx in VIEWING for q in square_loop(sx)]

    def cell(x0, x1, y0, y1):
        corners = [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]
        poly = []
        for c in range(4):
            fr, to = corners[c], corners[(c + 1) % 4]
            poly.append(fr)
            d = (to[0] - fr[0], to[1] - fr[1])
            l2 = d[0] ** 2 + d[1] ** 2
            on = []
            for q in square_points:
                t = ((q[0] - fr[0]) * d[0] + (q[1] - fr[1]) * d[1]) / l2
                if 1e-9 < t < 1 - 1e-9 and abs(cross2(d, (q[0] - fr[0], q[1] - fr[1]))) < 1e-9:
                    on.append((t, q))
            on.sort()
            poly += [q for _, q in on]
        return poly

    xs = [FLOOR_WEST_X, FLOOR_EAST_X]
    for sx in VIEWING:
        xs += [sx - STONE_CELL, sx + STONE_CELL]
    xs.sort()
    ys = [-FLOOR_EDGE, -STONE_CELL, STONE_CELL, FLOOR_EDGE]
    for i in range(len(xs) - 1):
        for j in range(len(ys) - 1):
            x0, x1, y0, y1 = xs[i], xs[i + 1], ys[j], ys[j + 1]
            stone = any(abs(0.5 * (x0 + x1) - sx) < STONE_CELL and abs(0.5 * (y0 + y1)) < STONE_CELL for sx in VIEWING)
            if not stone:
                plane(out.floor, cell(x0, x1, y0, y1))
    for s in VIEWING:
        def circle(radius, k):
            t = 2.0 * PI * (k % DISC_SEG) / DISC_SEG
            return (s + radius * math.cos(t), radius * math.sin(t), 0.0)
        vs = out.viewing
        vs.begin(f"viewing stone {s}")
        hub = vs.vertex((s, 0, 0), up, (s, 0))
        for k in range(DISC_SEG):
            p = circle(STONE_DISC, k)
            vs.vertex(p, up, p[:2])
        for k in range(DISC_SEG):
            vs.tri(hub, hub + 1 + k, hub + 1 + (k + 1) % DISC_SEG)
        vs.end()
        rg = out.rings
        base = len(rg.p)
        for k in range(DISC_SEG):
            p, q = circle(STONE_DISC, k), circle(STONE_RING, k)
            rg.vertex(p, up, p[:2])
            rg.vertex(q, up, q[:2])
        for k in range(DISC_SEG):
            a, b = base + 2 * k, base + 2 * ((k + 1) % DISC_SEG)
            rg.quad(a, b, b + 1, a + 1)
        square, ring = [], []
        for q in square_loop(s):
            a = math.atan2(q[1], q[0] - s)
            if a < 0:
                a += 2 * PI
            square.append((a, out.floor.vertex((q[0], q[1], 0.0), up, q)))
        for k in range(DISC_SEG):
            p = circle(STONE_RING, k)
            ring.append((2 * PI * k / DISC_SEG, out.floor.vertex(p, up, p[:2])))
        square.sort(key=lambda e: e[0])
        square.append((square[0][0] + 2 * PI, square[0][1]))
        ring.append((ring[0][0] + 2 * PI, ring[0][1]))
        i = j = 0
        while i + 1 < len(square) or j + 1 < len(ring):
            if j + 1 >= len(ring) or (i + 1 < len(square) and square[i + 1][0] <= ring[j + 1][0]):
                out.floor.tri(square[i][1], square[i + 1][1], ring[j][1])
                i += 1
            else:
                out.floor.tri(square[i][1], ring[j + 1][1], ring[j][1])
                j += 1


def build_plinths(out):
    plinth = [(-0.18, -0.10), (0.18, -0.10), (0.18, 0.29), (0.17, 0.30), (-0.17, 0.30), (-0.18, 0.29)]
    kerb = [(-0.30, -0.12), (-0.17, -0.12), (-0.17, 0.06), (-0.29, 0.06), (-0.30, 0.05)]
    for side in (-1, 1):
        along_side(out.stone, side, plinth, lambda y: rot_face_x(y) - PLINTH_EMBED, lambda y: atr_face_x(y, False) + PLINTH_EMBED,
                   f"plinth {side}")
        along_side(out.stone, side, kerb, lambda y: rot_face_x(y) - KERB_EMBED, lambda y: atr_face_x(y, False) + KERB_EMBED,
                   f"planter edge {side}")


def build_glass(out):
    m = out.glass
    c, s = coping_angle(), spring_angle()
    th = strip_angles()

    def inward(theta):
        return (0.0, -math.sin(theta), -math.cos(theta))

    def add(x, theta):
        return m.vertex(vault_point(x, theta, VR), inward(theta), (x, VR * theta))

    for i in range(len(th) - 1):
        t0, t1 = th[i], th[i + 1]
        drum = abs(0.5 * (t0 + t1)) < c

        def west_at(theta):
            return west_end(VR * math.sin(theta))

        def east_at(theta):
            return glass_east_end(VR * math.sin(theta), drum)
        w0, e0, e1, w1 = add(west_at(t0), t0), add(east_at(t0), t0), add(east_at(t1), t1), add(west_at(t1), t1)
        split0 = drum and abs(abs(t0) - c) < 1e-9
        split1 = drum and abs(abs(t1) - c) < 1e-9
        if split0:
            mid = add(glass_east_end(VR * math.sin(t0), False), t0)
            m.tri(e1, w1, w0)
            m.tri(e1, w0, mid)
            m.tri(e1, mid, e0)
        elif split1:
            mid = add(glass_east_end(VR * math.sin(t1), False), t1)
            m.tri(e0, w0, w1)
            m.tri(e0, w1, mid)
            m.tri(e0, mid, e1)
        else:
            m.quad(w0, e0, e1, w1)
    for side in (-1, 1):
        theta = side * s
        top_w = vault_point(west_end(VR * math.sin(theta)), theta, VR)
        top_e = vault_point(glass_east_end(VR * math.sin(theta), False), theta, VR)
        inn = (0.0, -side, 0.0)
        bot_w = np.array([top_w[0], top_w[1], GLASS_FOOT])
        bot_e = np.array([top_e[0], top_e[1], GLASS_FOOT])

        def wall_uv(p):
            return (p[0] * side, -p[2])
        a, b = m.vertex(bot_w, inn, wall_uv(bot_w)), m.vertex(bot_e, inn, wall_uv(bot_e))
        cc, d = m.vertex(top_e, inn, wall_uv(top_e)), m.vertex(top_w, inn, wall_uv(top_w))
        m.quad(a, b, cc, d)


def build_wall_frame(out):
    m = out.frame
    post = i_section(0.03, 0.01, 0.03, -0.03, -0.01, 0.14, 0.155)
    transom = rect(0.012, 0.11, TRANSOM_H - TRANSOM_HALF, TRANSOM_H + TRANSOM_HALF)
    shoe = rect(-0.025, 0.025, GLASS_FOOT, SHOE_TOP)
    eaves = [(-0.10, EAVES_BOTTOM), (0.20, EAVES_BOTTOM), (0.20, 5.05), (0.15, EAVES_TOP), (-0.10, EAVES_TOP)]
    end_post = rect(-SEAT, 0.07, -0.04, 0.12)
    for side in (-1, 1):
        for k in range(POSTS):
            upright(m, side, post_x(k), post, POST_FOOT, POST_TOP, f"post {k} {side}")
        along_side(m, side, transom, west_end, lambda y: east_end(y, False), f"transom {side}")
        along_side(m, side, shoe, west_end, lambda y: east_end(y, False), f"shoe {side}")
        along_side(m, side, eaves, west_end, lambda y: east_end(y, False), f"eaves {side}")

        def west_post(t, q, side=side):
            y = side * (HW - q[1])
            return np.array([rot_face_x(y) + q[0], y, mix(POST_FOOT, POST_TOP, t)])

        def east_post(t, q, side=side):
            y = side * (HW - q[1])
            return np.array([atr_face_x(y, False) - q[0], y, mix(POST_FOOT, POST_TOP, t)])
        solid(m, end_post, [0.0, 1.0], west_post, label=f"end post W {side}")
        solid(m, end_post, [0.0, 1.0], east_post, label=f"end post E {side}")


def build_lattice(out):
    m = out.lattice
    c, s = coping_angle(), spring_angle()
    strips = strip_angles()
    purlins = purlin_angles()
    arch = i_section(0.035, 0.012, 0.035, -0.15, -0.13, 0.025, 0.045)
    for k in range(ARCHES):
        x = arch_x(k)
        solid(m, arch, strips, lambda theta, q, x=x: vault_point(x + q[0], theta, VR + q[1]), label=f"arch {x}")
    purlin = rect(-0.03, 0.03, -0.12, 0.015)
    for k in range(1, len(purlins) - 1):
        theta = purlins[k]
        drum = abs(theta) < c + 1e-9
        u = np.array([0.0, math.sin(theta), math.cos(theta)])
        across = np.array([0.0, math.cos(theta), -math.sin(theta)])

        def place(t, q, u=u, across=across, drum=drum):
            p = np.array([0.0, 0.0, VC]) + u * (VR + q[1]) + across * q[0]
            return np.array([mix(west_end(p[1]), east_end(p[1], drum), t), p[1], p[2]])
        solid(m, purlin, [0.0, 1.0], place, label=f"purlin {k}")
    bar_a, bar_b = rect(-0.015, 0.015, -0.08, -0.02), rect(-0.015, 0.015, -0.09, -0.03)

    def diagonal(bar, x0, t0, x1, t1, label):
        def place(t, q):
            x, theta = mix(x0, x1, t), mix(t0, t1, t)
            tangent = np.array([x1 - x0, VR * math.cos(theta) * (t1 - t0), -VR * math.sin(theta) * (t1 - t0)])
            tangent /= np.linalg.norm(tangent)
            u = np.array([0.0, math.sin(theta), math.cos(theta)])
            side = np.cross(u, tangent)
            side /= np.linalg.norm(side)
            return vault_point(x, theta, VR + q[1]) + side * q[0]
        trim = BAR_CLEAR / math.sqrt((x1 - x0) ** 2 + (VR * (t1 - t0)) ** 2)
        solid(m, bar, between(trim, 1.0 - trim, 8), place, label=label)
    for i in range(ARCHES - 1):
        for k in range(len(purlins) - 1):
            diagonal(bar_a, arch_x(i), purlins[k], arch_x(i + 1), purlins[k + 1], f"bar A {i} {k}")
            diagonal(bar_b, arch_x(i), purlins[k + 1], arch_x(i + 1), purlins[k], f"bar B {i} {k}")
    collar = rect(-SEAT, 0.08, -0.11, -0.012)
    drum_collar = rect(DRUM_GAP, 0.08, -0.11, -0.012)

    def west_collar(theta, q):
        p = vault_point(0.0, theta, VR + q[1])
        return np.array([rot_face_x(p[1]) + q[0], p[1], p[2]])
    solid(m, collar, strips, west_collar, label="collar W")

    def east_collar(section, fr, to, drum, label):
        ts = [t for t in strips if fr - 1e-9 <= t <= to + 1e-9]

        def place(theta, q):
            p = vault_point(0.0, theta, VR + q[1])
            return np.array([atr_face_x(p[1], drum) - q[0], p[1], p[2]])
        solid(m, section, ts, place, label=label)
    east_collar(collar, -s, -c, False, "collar E north")
    east_collar(drum_collar, -c, c, True, "collar E drum")
    east_collar(collar, c, s, False, "collar E south")


def build_door_surrounds(out):
    section = [(0.29, -0.015), (0.012, -0.015), (0.012, 0.024), (0.10, 0.030), (0.10, 0.040), (0.19, 0.046), (0.19, 0.054),
               (0.205, 0.054)]
    for k in range(1, 7):
        a = PI - 0.5 * PI * k / 6
        section.append((0.245 + 0.04 * math.cos(a), 0.054 + 0.04 * math.sin(a)))
    section.append((0.29, 0.094))
    floor = -0.01
    half, spring = ROT_DOOR_HALF, ROT_DOOR_SPRING
    ts = [-1.0] + between(0.0, PI, ARCH_SEG) + [PI + 1.0]

    def rot_place(t, q):
        reach = half + q[0]
        if t < 0.0:
            lateral, z = -reach, mix(floor, spring, t + 1.0)
        elif t > PI:
            lateral, z = reach, mix(spring, floor, t - PI)
        else:
            lateral, z = -reach * math.cos(t), spring + reach * math.sin(t)
        radius = ROT_OUTER + q[1]
        return np.array([math.sqrt(radius * radius - lateral * lateral), lateral, z])
    solid(out.mouldings, section, ts, rot_place, label="architrave Rotunda")

    half2, head = ELAN_DOOR_HALF, ELAN_DOOR_H

    def on_base(lateral, z, proud):
        radius = ATR_OUTER + proud
        return np.array([ELAN_X - math.sqrt(radius * radius - lateral * lateral), ELAN_Y + lateral, z])
    m = out.mouldings
    m.begin("architrave Atrium")
    solid(m, section, [0.0, 1.0], lambda t, q: on_base(-(half2 + q[0]), mix(floor, head + q[0], t), q[1]), True, False)
    solid(m, section, [0.0, 1.0], lambda t, q: on_base(mix(-(half2 + q[0]), half2 + q[0], t), head + q[0], q[1]), False, False)
    solid(m, section, [0.0, 1.0], lambda t, q: on_base(half2 + q[0], mix(head + q[0], floor, t), q[1]), False, True)
    m.end()


def reader_facing(y):
    return 1.0 if y < 0.0 else -1.0


def build_stereo_stones(out):
    r0, top = STEREO_R, STEREO_TOP
    drum = [(0.0, -0.01), (r0, -0.01), (r0, top - 0.015), (r0 - 0.015, top), (0.0, top)]
    stem = [(0.0, top - 0.01), (0.015, top - 0.01), (0.015, READER_H), (0.0, READER_H)]
    for x, y in STEREO:
        lathe(out.stone, x, y, drum, f"stereo stone {x}")
        lathe(out.bronze, x, y - 0.016 * reader_facing(y), stem, f"reader stem {x}")


def build_stands(out):
    stem_bar = rect(-0.015, 0.015, 0.098, 0.110)
    foot = rect(-0.035, 0.035, 0.080, 0.128)
    glazing_bar = rect(-0.0125, 0.0125, 0.035, 0.055)
    stem_top = MOUNT_TOP - 0.05
    for side in (-1, 1):
        for b, x in enumerate(PRINT_BAYS):
            if b < FIRST_AUTOCHROME:
                for dx in (-0.30, 0.30):
                    upright(out.bronze, side, x + dx, stem_bar, PLINTH_H + 0.01, stem_top, f"stand stem {x + dx} {side}")
                    upright(out.bronze, side, x + dx, foot, PLINTH_H - 0.02, PLINTH_H + 0.02, f"stand foot {x + dx} {side}")
            else:
                for dx in (-0.56, 0.56):
                    upright(out.bronze, side, x + dx, glazing_bar, GLASS_FOOT, TRANSOM_H, f"glazing bar {x + dx} {side}")
        for k in range(POSTS):
            upright(out.bronze, side, post_x(k), rect(-0.05, 0.05, 0.17, 0.19), 0.11, 0.17, f"louvre {k} {side}")


def build_threshold(out):
    saddle = [(-0.035, -0.012), (0.035, -0.012), (0.035, 0.008), (0.031, 0.012), (-0.031, 0.012), (-0.035, 0.008)]
    reach = math.asin((ROT_DOOR_HALF + 0.015) / SUN_R)

    def place(phi, q):
        radius = SUN_R + q[0]
        return np.array([radius * math.cos(phi), radius * math.sin(phi), q[1]])
    solid(out.bronze, saddle, between(-reach, reach, 24), place, label="threshold")


def build_all():
    parts = Parts()
    build_floor(parts)
    build_plinths(parts)
    build_glass(parts)
    build_wall_frame(parts)
    build_lattice(parts)
    build_door_surrounds(parts)
    build_stereo_stones(parts)
    build_stands(parts)
    build_threshold(parts)
    return parts


# ------------------------------------------------------------------------------------------------
# Neighbours (as the map has them) for the proximity and leak checks

def cylinder(m, cx, cy, r, z0, z1, segs, inward, label, a0=0.0, a1=2 * PI):
    m.begin(label)
    base = len(m.p)
    for k in range(segs + 1):
        a = a0 + (a1 - a0) * k / segs
        d = np.array([math.cos(a), math.sin(a), 0.0])
        nrm = -d if inward else d
        m.vertex((cx + r * d[0], cy + r * d[1], z0), nrm, (0, 0))
        m.vertex((cx + r * d[0], cy + r * d[1], z1), nrm, (0, 0))
    for k in range(segs):
        a = base + 2 * k
        m.quad(a, a + 2, a + 3, a + 1)
    m.end()


def disc(m, cx, cy, r, z, segs, label, up=True):
    m.begin(label)
    hub = m.vertex((cx, cy, z), (0, 0, 1 if up else -1), (0, 0))
    for k in range(segs):
        a = 2 * PI * k / segs
        m.vertex((cx + r * math.cos(a), cy + r * math.sin(a), z), (0, 0, 1 if up else -1), (0, 0))
    for k in range(segs):
        m.tri(hub, hub + 1 + k, hub + 1 + (k + 1) % segs)
    m.end()


def box(m, lo, hi, label):
    m.begin(label)
    lo, hi = np.array(lo, float), np.array(hi, float)
    for axis in range(3):
        for sgn in (-1, 1):
            nrm = np.zeros(3)
            nrm[axis] = sgn
            u, v = [a for a in range(3) if a != axis]
            pts = []
            for cu, cv in ((0, 0), (1, 0), (1, 1), (0, 1)):
                p = np.zeros(3)
                p[axis] = hi[axis] if sgn > 0 else lo[axis]
                p[u] = hi[u] if cu else lo[u]
                p[v] = hi[v] if cv else lo[v]
                pts.append(m.vertex(p, nrm, (0, 0)))
            m.quad(*pts)
    m.end()


def neighbours():
    m = Mesh("neighbours")
    disc(m, 0, 0, SUN_R, 0.0, SUN_SEG, "sun clock")
    m.begin("Ground")
    g = [m.vertex(p, (0, 0, 1), (0, 0)) for p in ((-110, -40, -0.03), (80, -40, -0.03), (80, 45, -0.03), (-110, 45, -0.03))]
    m.quad(*g)
    m.end()
    # The drums as the map has them (their door openings ignored: the tests stay on the hall's side).
    cylinder(m, 0, 0, ROT_OUTER, 0.0, 11.0, 320, False, "Rotunda drum")
    cylinder(m, ELAN_X, ELAN_Y, ATR_OUTER, 0.0, ELAN_BASE, 192, False, "Atrium base")
    cylinder(m, ELAN_X, ELAN_Y, ATR_DRUM, ELAN_BASE, 22.0, 192, False, "Atrium drum glass")
    # The Atrium's floor to r 14.3 and its coping (h 6, r 14–14.8).
    disc(m, ELAN_X, ELAN_Y, ATR_FLOOR_R, 0.0, ATR_FLOOR_SEG, "Atrium floor")
    m.begin("Atrium coping")
    base = len(m.p)
    for k in range(193):
        a = 2 * PI * k / 192
        m.vertex((ELAN_X + ATR_DRUM * math.cos(a), ELAN_Y + ATR_DRUM * math.sin(a), ELAN_BASE), (0, 0, 1), (0, 0))
        m.vertex((ELAN_X + ATR_OUTER * math.cos(a), ELAN_Y + ATR_OUTER * math.sin(a), ELAN_BASE), (0, 0, 1), (0, 0))
    for k in range(192):
        a = base + 2 * k
        m.quad(a, a + 2, a + 3, a + 1)
    m.end()
    # The doors' reveals: the Rotunda's jambs (lateral ±1.5, r 10.04–11.2) and the Atrium's (±2, 4.5 high) and lintel.
    for side in (-1, 1):
        y = side * ROT_DOOR_HALF
        box(m, (math.sqrt(10.04 ** 2 - y * y), y - 0.0 if side > 0 else y - 0.3, -0.5),
            (math.sqrt(ROT_OUTER ** 2 - y * y), y + 0.3 if side > 0 else y, ROT_DOOR_SPRING), f"Rotunda jamb {side}")
        y = side * ELAN_DOOR_HALF
        box(m, (ELAN_X - math.sqrt(ATR_OUTER ** 2 - y * y), y if side > 0 else y - 0.3, -0.5),
            (ELAN_X - math.sqrt(ATR_DRUM ** 2 - y * y), y + 0.3 if side > 0 else y, ELAN_DOOR_H), f"Atrium jamb {side}")
    # The prints' mounts (imported, kept): 3 cm panels, their backs at |y| 4.38.
    for side in (-1, 1):
        for b, x in enumerate(PRINT_BAYS[:FIRST_AUTOCHROME]):
            y0, y1 = sorted((side * MOUNT_BACK, side * (MOUNT_BACK - 0.03)))
            box(m, (x - MOUNT_HALF, y0, MOUNT_BOTTOM), (x + MOUNT_HALF, y1, MOUNT_TOP), f"mount {b} {side}")
    return m


# ------------------------------------------------------------------------------------------------
# Checks

def arrays(mesh, t0=0, t1=None):
    p = np.array(mesh.p) if mesh.p else np.zeros((0, 3))
    t = np.array(mesh.tris[t0:t1], int) if mesh.tris[t0:t1] else np.zeros((0, 3), int)
    return p, t


def weld_key(p):
    return tuple(np.round(np.asarray(p) / 5e-5).astype(np.int64))


def check_solids(parts):
    bad = 0
    count = 0
    for mesh in (parts.viewing, parts.stone, parts.mouldings, parts.frame, parts.lattice, parts.bronze):
        p, _ = arrays(mesh)
        keys = [weld_key(q) for q in p]
        for label, t0, t1 in mesh.solids:
            count += 1
            tris = mesh.tris[t0:t1]
            edges = defaultdict(int)
            vol = 0.0
            for a, b, c in tris:
                ka, kb, kc = keys[a], keys[b], keys[c]
                for u, v in ((ka, kb), (kb, kc), (kc, ka)):
                    edges[(u, v)] += 1
                vol -= np.dot(p[a], np.cross(p[b], p[c])) / 6.0   # clockwise: Unreal's front faces
            open_edges = [e for e, n in edges.items() if edges.get((e[1], e[0]), 0) != n or n != 1]
            flat = label.startswith("viewing stone")
            if flat:
                continue
            if open_edges or vol <= 0:
                bad += 1
                if bad <= 12:
                    e = open_edges[0] if open_edges else None
                    where = (np.array(e[0]) * 5e-5).round(3) if e else ""
                    print(f"  ! {mesh.name}/{label}: {len(open_edges)} unpaired edges {where}, volume {vol:.5f}")
    print(f"1. closed solids: {count - bad} of {count} closed with outward normals" + ("" if not bad else f" ({bad} BAD)"))
    return bad == 0


def check_winding(parts):
    bad = total = 0
    for mesh in vars(parts).values():
        p, t = arrays(mesh)
        if not len(t):
            continue
        n = np.array(mesh.n)
        fn = -np.cross(p[t[:, 1]] - p[t[:, 0]], p[t[:, 2]] - p[t[:, 0]])   # clockwise front faces
        vn = n[t[:, 0]] + n[t[:, 1]] + n[t[:, 2]]
        d = np.einsum("ij,ij->i", fn, vn)
        bad += int((d <= 0).sum())
        # A vertex normal more than 80° from its face's: a shading error.
        fnu = fn / np.linalg.norm(fn, axis=1)[:, None]
        for k in range(3):
            bad += int((np.einsum("ij,ij->i", fnu, n[t[:, k]]) < math.cos(math.radians(80))).sum())
        total += len(t)
    print(f"2. winding: {total} triangles, {bad} disagree with their normals")
    return bad == 0


def open_boundary(mesh_list):
    edges = defaultdict(int)
    pos = {}
    for mesh in mesh_list:
        p, t = arrays(mesh)
        keys = [weld_key(q) for q in p]
        for a, b, c in t:
            for u, v in ((a, b), (b, c), (c, a)):
                ku, kv = keys[u], keys[v]
                pos[ku], pos[kv] = p[u], p[v]
                edges[(ku, kv)] += 1
    out = []
    for (u, v), n in edges.items():
        if edges.get((v, u), 0) == 0:
            out.append((pos[u], pos[v]))
        elif n > 1 or edges[(v, u)] > 1:
            out.append((pos[u], pos[v]))
    return out


def rot_r(p):
    return math.hypot(p[0], p[1])


def atr_r(p):
    return math.hypot(p[0] - ELAN_X, p[1] - ELAN_Y)


def buried_floor(a, b):
    m = (a + b) / 2
    ok = []
    for p in (a, b, m):
        y = abs(p[1])
        r, ra = rot_r(p), atr_r(p)
        in_rot_wall = (ROT_RADIUS + 0.04 < r < ROT_OUTER and y > ROT_DOOR_HALF) or abs(r - SUN_R) < 0.012
        in_atr_wall = (ATR_DRUM < ra < ATR_OUTER and y > ELAN_DOOR_HALF) or abs(ra - ATR_FLOOR_R) < 0.012
        on_polygon_edge = r <= SUN_R + 1e-6 or ra <= ATR_FLOOR_R + 1e-6
        ok.append(y >= HW - 0.18 or in_rot_wall or in_atr_wall or on_polygon_edge)
    return all(ok)


def buried_glass(a, b):
    m = (a + b) / 2
    for p in (a, b, m):
        if p[2] <= PLINTH_H - 0.005:
            continue                                    # in the plinth
        r, ra = rot_r(p), atr_r(p)
        if ROT_OUTER - 0.03 < r < ROT_OUTER:
            continue                                    # 2 cm into the Rotunda's drum
        if p[2] < ELAN_BASE + 1e-6 and ATR_OUTER - 0.03 < ra < ATR_OUTER:
            continue                                    # into the Atrium's base
        if p[2] >= ELAN_BASE - 1e-6 and ATR_DRUM - 0.03 < ra < ATR_DRUM:
            continue                                    # through the drum's glass
        if abs(p[2] - ELAN_BASE) < 1e-6 and ATR_DRUM - 0.03 < ra < ATR_OUTER:
            continue                                    # on the coping (under the coping purlin)
        return False
    return True


def check_surfaces(parts):
    ok = True
    floor_open = open_boundary([parts.floor, parts.viewing, parts.rings])
    bad = [e for e in floor_open if not buried_floor(*e)]
    print(f"3a. floor: {len(floor_open)} open edges, {len(bad)} not buried in a solid or on a neighbour's edge")
    for a, b in bad[:8]:
        print(f"   ! {a.round(3)} - {b.round(3)}")
    ok &= not bad
    glass_open = open_boundary([parts.glass])
    bad = [e for e in glass_open if not buried_glass(*e)]
    print(f"3b. glass: {len(glass_open)} open edges, {len(bad)} not buried")
    for a, b in bad[:8]:
        print(f"   ! {a.round(3)} - {b.round(3)}")
    ok &= not bad
    # Coverage: the floor covers the hall (and the doors' reveals) wherever it is open to view.
    tris = []
    for mesh in (parts.floor, parts.viewing, parts.rings):
        p, t = arrays(mesh)
        tris += [p[x][:, :2] for x in t]
    tri = np.array(tris)
    rng = np.random.default_rng(1)
    missing = 0
    samples = 0
    for _ in range(6000):
        x, y = rng.uniform(9.9, 40.5), rng.uniform(-HW + 0.18, HW - 0.18)
        r, ra = math.hypot(x, y), math.hypot(x - ELAN_X, y)
        if r < SUN_R - 1e-6 or ra < ATR_FLOOR_R - 1e-6:
            continue                                     # the neighbours' floors
        if (ROT_RADIUS < r < ROT_OUTER and abs(y) > ROT_DOOR_HALF) or r < ROT_RADIUS:
            continue                                     # under the Rotunda's wall
        if (ATR_DRUM < ra < ATR_OUTER and abs(y) > ELAN_DOOR_HALF) or ra < ATR_DRUM:
            continue
        samples += 1
        a, b, c = tri[:, 0], tri[:, 1], tri[:, 2]
        q = np.array([x, y])

        def side(u, v):
            return (v[:, 0] - u[:, 0]) * (q[1] - u[:, 1]) - (v[:, 1] - u[:, 1]) * (q[0] - u[:, 0])
        s1, s2, s3 = side(a, b), side(b, c), side(c, a)
        inside = ((s1 >= -1e-12) & (s2 >= -1e-12) & (s3 >= -1e-12)) | ((s1 <= 1e-12) & (s2 <= 1e-12) & (s3 <= 1e-12))
        if not inside.any():
            missing += 1
            if missing <= 5:
                print(f"   ! floor missing at ({x:.3f}, {y:.3f})")
    print(f"3c. floor coverage: {samples} points in the hall and the reveals, {missing} uncovered")
    ok &= missing == 0
    return ok


def all_parts(parts, extra):
    """(label, part, positions, triangles) for every solid, surface and neighbour."""
    out = []
    for mesh in list(vars(parts).values()) + [extra]:
        p, _ = arrays(mesh)
        covered = set()
        for label, t0, t1 in mesh.solids:
            out.append((f"{mesh.name}/{label}", p, np.array(mesh.tris[t0:t1], int)))
            covered.update(range(t0, t1))
        rest = [t for i, t in enumerate(mesh.tris) if i not in covered]
        if rest:
            out.append((f"{mesh.name}", p, np.array(rest, int)))
    return out


def check_proximity(parts, extra):
    """Faces within 1 cm of a parallel face of another part (unless they share their vertices)."""
    items = all_parts(parts, extra)
    allp, alln, alltri, owner = [], [], [], []
    for idx, (label, p, t) in enumerate(items):
        if not len(t):
            continue
        a, b, c = p[t[:, 0]], p[t[:, 1]], p[t[:, 2]]
        fn = np.cross(b - a, c - a)
        ln = np.linalg.norm(fn, axis=1)
        keep = ln > 1e-12
        allp.append(np.stack([a, b, c], 1)[keep])
        alln.append(fn[keep] / ln[keep][:, None])
        owner.append(np.full(keep.sum(), idx))
    P = np.concatenate(allp)
    N = np.concatenate(alln)
    O = np.concatenate(owner)
    # Glass surfaces: the wall glass meets the vault's first strip at the springing (sharing vertices) — fine.
    cell = 0.3
    lo = P.min(axis=(0, 1))
    grid = defaultdict(list)
    tmin = np.floor((P.min(axis=1) - lo - 0.011) / cell).astype(int)
    tmax = np.floor((P.max(axis=1) - lo + 0.011) / cell).astype(int)
    for i in range(len(P)):
        if (tmax[i] - tmin[i]).prod() > 4000:
            continue   # the Ground and the big glass: tested from the other side
        for ix in range(tmin[i][0], tmax[i][0] + 1):
            for iy in range(tmin[i][1], tmax[i][1] + 1):
                for iz in range(tmin[i][2], tmax[i][2] + 1):
                    grid[(ix, iy, iz)].append(i)
    big = [i for i in range(len(P)) if (tmax[i] - tmin[i]).prod() > 4000]
    # Samples: each triangle's centroid and three points towards its corners.
    w = np.array([[1 / 3, 1 / 3, 1 / 3], [0.7, 0.15, 0.15], [0.15, 0.7, 0.15], [0.15, 0.15, 0.7]])
    hits = defaultdict(int)
    examples = {}
    for i in range(len(P)):
        samples = w @ P[i]
        cands = set(big)
        for s in samples:
            k = tuple(np.floor((s - lo) / cell).astype(int))
            cands.update(grid.get(k, ()))
        cands = [j for j in cands if O[j] != O[i]]
        if not cands:
            continue
        cands = np.array(cands)
        par = np.abs(N[cands] @ N[i]) > 0.995
        cands = cands[par]
        if not len(cands):
            continue
        for s in samples:
            d = np.einsum("ij,ij->i", s - P[cands, 0], N[cands])
            near = np.abs(d) < 0.0099
            for j in cands[near]:
                q = s - N[j] * np.dot(s - P[j, 0], N[j])
                a, b, c = P[j]
                # Inside triangle j (inset 0.5 mm, so touching edges don't count)?
                v0, v1, v2 = c - a, b - a, q - a
                d00, d01, d02, d11, d12 = v0 @ v0, v0 @ v1, v0 @ v2, v1 @ v1, v1 @ v2
                den = d00 * d11 - d01 * d01
                if den <= 0:
                    continue
                u = (d11 * d02 - d01 * d12) / den
                v = (d00 * d12 - d01 * d02) / den
                e = 0.0005 / max(math.sqrt(min(d00, d11)), 1e-6)
                if u > e and v > e and u + v < 1 - e:
                    # Sharing all their vertices (the same surface split between two parts)?
                    if all(np.min(np.linalg.norm(P[i] - vtx, axis=1)) < 1e-6 for vtx in P[j]):
                        continue
                    key = tuple(sorted((items[O[i]][0], items[O[j]][0])))
                    hits[key] += 1
                    examples.setdefault(key, (s.round(4), float(d[list(cands).index(j)])))
    nice = {}
    for (a, b), n in hits.items():
        ka = (a.split("/")[0] + "/" + " ".join(a.split("/")[1].split()[:2])) if "/" in a else a
        kb = (b.split("/")[0] + "/" + " ".join(b.split("/")[1].split()[:2])) if "/" in b else b
        nice.setdefault((ka, kb), [0, examples[(a, b)]])[0] += n
    print(f"4. z-fighting: {len(nice)} pairs of parts with faces closer than 1 cm")
    for (a, b), (n, ex) in sorted(nice.items(), key=lambda kv: -kv[1][0])[:25]:
        print(f"   ! {a}  ~  {b}: {n} samples, e.g. at {ex[0]} ({ex[1] * 1000:.1f} mm)")
    return not nice


def check_leaks(parts, extra, rays=6000):
    """Rays from inside the hall must all end on a surface (the drums are taken as closed)."""
    tris = []
    for mesh in list(vars(parts).values()) + [extra]:
        p, t = arrays(mesh)
        if len(t):
            tris.append(np.stack([p[t[:, 0]], p[t[:, 1]], p[t[:, 2]]], 1))
    T = np.concatenate(tris)
    # Close the drums: their tops (the Rotunda to 11 m, the Atrium's glass to 22 m) and the sky over the Atrium.
    caps = []
    for cx, r, z in ((0.0, ROT_OUTER, 11.0), (ELAN_X, ATR_OUTER, 22.0)):
        for k in range(96):
            a0, a1 = 2 * PI * k / 96, 2 * PI * (k + 1) / 96
            caps.append([[cx, 0, z], [cx + r * math.cos(a0), r * math.sin(a0), z], [cx + r * math.cos(a1), r * math.sin(a1), z]])
    T = np.concatenate([T, np.array(caps)])
    a, e1, e2 = T[:, 0], T[:, 1] - T[:, 0], T[:, 2] - T[:, 0]
    rng = np.random.default_rng(7)
    leaks = []
    for k in range(rays):
        o = np.array([rng.uniform(11.4, 39.0), rng.uniform(-4.2, 4.2), rng.uniform(0.05, 7.0)])
        if o[2] > VC + math.sqrt(max(0.0, (VR - 0.2) ** 2 - o[1] ** 2)):
            continue
        d = rng.normal(size=3)
        d /= np.linalg.norm(d)
        pv = np.cross(d, e2)
        det = np.einsum("ij,ij->i", e1, pv)
        ok = np.abs(det) > 1e-14
        inv = np.where(ok, 1.0 / np.where(ok, det, 1.0), 0.0)
        tv = o - a
        u = np.einsum("ij,ij->i", tv, pv) * inv
        qv = np.cross(tv, e1)
        v = (qv @ d) * inv
        t = np.einsum("ij,ij->i", e2, qv) * inv
        hit = ok & (u >= -1e-9) & (v >= -1e-9) & (u + v <= 1 + 1e-9) & (t > 1e-6)
        if not hit.any():
            leaks.append((o.round(3), d.round(3)))
    print(f"5. leaks: {rays} rays from inside the hall, {len(leaks)} escaped")
    for o, d in leaks[:6]:
        print(f"   ! from {o} towards {d}")
    return not leaks


def check_uvs(parts):
    bad = total = 0
    for mesh in vars(parts).values():
        p, t = arrays(mesh)
        if not len(t):
            continue
        uv = np.array(mesh.uv)
        area = 0.5 * np.linalg.norm(np.cross(p[t[:, 1]] - p[t[:, 0]], p[t[:, 2]] - p[t[:, 0]]), axis=1)
        d1, d2 = uv[t[:, 1]] - uv[t[:, 0]], uv[t[:, 2]] - uv[t[:, 0]]
        uva = 0.5 * np.abs(d1[:, 0] * d2[:, 1] - d1[:, 1] * d2[:, 0])
        big = area > 1e-6
        ratio = uva[big] / area[big]
        bad += int(((ratio < 0.5) | (ratio > 1.5)).sum())
        total += int(big.sum())
    print(f"6. UVs in metres: {total} triangles, {bad} whose UV area is off by more than 50 %")
    return bad == 0


def summary(parts):
    for name, mesh in vars(parts).items():
        print(f"   {name:10s} {len(mesh.tris):7d} triangles, {len(mesh.p):7d} vertices, {len(mesh.solids):4d} solids")


def main():
    parts = build_all()
    summary(parts)
    extra = neighbours()
    results = [check_solids(parts), check_winding(parts), check_surfaces(parts), check_proximity(parts, extra), check_uvs(parts)]
    if "--no-rays" not in sys.argv:
        results.append(check_leaks(parts, extra))
    print("ALL PASS" if all(results) else "FAILED")
    return 0 if all(results) else 1


if __name__ == "__main__":
    sys.exit(main())
