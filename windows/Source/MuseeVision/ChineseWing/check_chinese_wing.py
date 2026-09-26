"""
Checks AChineseWingStructure's geometry, written by `musee.ChineseWing.WriteObj` (or
AChineseWingStructure::WriteGeometryObj): OBJ in cm, groups "part|assembly|solid|closed".

  - every closed solid (and every assembly of open runs, e.g. the roof slab across its sections) is watertight:
    each edge used by exactly two triangles, in opposite directions (after welding positions to 0.01 mm);
  - triangles are wound as Unreal's front faces for their normals (cross(b-a, c-a) away from the normal) and closed
    solids face outwards (positive volume);
  - no two surfaces face the same way less than 1 cm apart where they overlap (z-fighting).

  python check_chinese_wing.py ChineseWing.obj [--no-fight]

Write the OBJ from a game run with -ExecCmds="musee.ChineseWing.WriteObj PATH" (musee.ChineseWing.Preview places the
wing for that run and hides what it replaces), or in the editor with unreal.ChineseWingStructure.write_geometry_obj(PATH).
A pair of faces closer than 1 cm is ignored where the rear one is buried inside another closed solid.
"""
import math
import sys
from collections import defaultdict

import numpy as np


def load(path):
    V, N = [], []
    groups = []   # (part, assembly, solid, closed, [faces])
    part = None
    cur = None
    with open(path, encoding="utf-8") as f:
        for line in f:
            if line.startswith("v "):
                V.append([float(x) for x in line.split()[1:4]])
            elif line.startswith("vn "):
                N.append([float(x) for x in line.split()[1:4]])
            elif line.startswith("o "):
                part = line[2:].strip()
            elif line.startswith("g "):
                p, asm, solid, closed = line[2:].strip().split("|")
                cur = [p, asm, solid, closed == "1", []]
                groups.append(cur)
            elif line.startswith("f "):
                idx = [int(t.split("/")[0]) - 1 for t in line.split()[1:4]]
                cur[4].append(idx)
    return np.array(V), np.array(N), groups


def weld(V, tol=1e-3):
    q = np.round(V / tol).astype(np.int64)
    _, inv = np.unique(q, axis=0, return_inverse=True)
    return inv.reshape(-1)


def check_closed(name, tris, W, V, expect_closed=True, report=5):
    """tris: (n,3) original indices; W: weld map. Returns problems."""
    t = W[tris]
    good = (t[:, 0] != t[:, 1]) & (t[:, 1] != t[:, 2]) & (t[:, 0] != t[:, 2])
    t = t[good]
    tr = tris[good]
    edges = defaultdict(list)
    for k, (a, b, c) in enumerate(t):
        for u, v in ((a, b), (b, c), (c, a)):
            edges[(min(u, v), max(u, v))].append((u, v, k))
    boundary, nonmanifold, flipped = [], [], []
    for e, uses in edges.items():
        if len(uses) == 1:
            boundary.append(e)
        elif len(uses) > 2:
            # Allow pairs of opposite uses (e.g. two solids touching along an edge inside one run).
            fwd = sum(1 for u, v, _ in uses if u == e[0])
            if fwd * 2 != len(uses):
                nonmanifold.append(e)
        else:
            if uses[0][0] == uses[1][0]:
                flipped.append(e)
    # Signed volume with the front-face normals (Unreal: front = -cross).
    P = V[tr]
    vol = -np.einsum("ij,ij->i", P[:, 0], np.cross(P[:, 1], P[:, 2])).sum() / 6.0
    return boundary, nonmanifold, flipped, vol, len(t)


def rep_points(W, V, edges, n=4):
    # a representative position (cm) for some edges
    first = {}
    for i, w in enumerate(W):
        if w not in first:
            first[w] = i
    out = []
    for e in edges[:n]:
        a, b = V[first[e[0]]], V[first[e[1]]]
        out.append(tuple(np.round((a + b) / 200.0, 3)))   # metres
    return out


def tri_clip_area(A, B):
    """Area of the intersection of two 2D triangles (Sutherland-Hodgman)."""
    def clip(poly, a, b):
        out = []
        def side(p):
            return (b[0] - a[0]) * (p[1] - a[1]) - (b[1] - a[1]) * (p[0] - a[0])
        n = len(poly)
        for i in range(n):
            p, q = poly[i], poly[(i + 1) % n]
            sp, sq = side(p), side(q)
            if sp >= 0:
                out.append(p)
            if (sp >= 0) != (sq >= 0):
                t = sp / (sp - sq)
                out.append((p[0] + t * (q[0] - p[0]), p[1] + t * (q[1] - p[1])))
        return out
    def area(poly):
        return 0.5 * sum(poly[i][0] * poly[(i + 1) % len(poly)][1] - poly[(i + 1) % len(poly)][0] * poly[i][1] for i in range(len(poly)))
    if area(B) < 0:
        B = [B[0], B[2], B[1]]
    poly = [tuple(p) for p in A]
    if area(poly) < 0:
        poly = [poly[0], poly[2], poly[1]]
    for i in range(3):
        poly = clip(poly, B[i], B[(i + 1) % 3])
        if len(poly) < 3:
            return 0.0, None
    return abs(area(poly)), (sum(p[0] for p in poly) / len(poly), sum(p[1] for p in poly) / len(poly))


def fights(V, tris, owner, gap=1.0, min_area=1.0):
    """Same-facing overlapping triangles closer than `gap` cm (area > min_area cm2)."""
    P = V[tris]
    n = np.cross(P[:, 1] - P[:, 0], P[:, 2] - P[:, 0])
    ln = np.linalg.norm(n, axis=1)
    ok = ln > 1e-6
    n[ok] /= ln[ok][:, None]
    n = -n   # Unreal's front faces
    off = np.einsum("ij,ij->i", n, P[:, 0])
    key_n = np.round(n * 10).astype(int)
    buckets = defaultdict(list)
    for i in np.nonzero(ok)[0]:
        buckets[(key_n[i, 0], key_n[i, 1], key_n[i, 2], int(math.floor(off[i] / gap)))].append(i)
    found = []
    seen = set()
    for key, items in buckets.items():
        cand = list(items)
        nb = buckets.get((key[0], key[1], key[2], key[3] + 1))
        if nb:
            cand = cand + nb
        if len(cand) < 2:
            continue
        ax = int(np.argmax(np.abs(n[items[0]])))
        keep = [a for a in range(3) if a != ax]
        grid = defaultdict(list)
        for i in cand:
            lo = P[i][:, keep].min(axis=0)
            hi = P[i][:, keep].max(axis=0)
            for gx in range(int(lo[0] // 50), int(hi[0] // 50) + 1):
                for gy in range(int(lo[1] // 50), int(hi[1] // 50) + 1):
                    grid[(gx, gy)].append(i)
        for cell in grid.values():
            if len(cell) < 2:
                continue
            for x in range(len(cell)):
                i = cell[x]
                for y in range(x + 1, len(cell)):
                    j = cell[y]
                    if (i, j) in seen:
                        continue
                    seen.add((i, j))
                    if np.dot(n[i], n[j]) < 0.9995 or abs(off[i] - off[j]) > gap - 0.01:
                        continue
                    # Each triangle's corners within `gap` of the other's plane.
                    if np.abs(P[j] @ n[i] - off[i]).max() > gap or np.abs(P[i] @ n[j] - off[j]).max() > gap:
                        continue
                    if len(set(tris[i]) & set(tris[j])) >= 2:
                        continue
                    a, c2 = tri_clip_area(P[i][:, keep], P[j][:, keep])
                    if a > min_area:
                        p3 = np.zeros(3)
                        p3[keep[0]], p3[keep[1]] = c2
                        p3[ax] = (off[i] - n[i][keep[0]] * c2[0] - n[i][keep[1]] * c2[1]) / n[i][ax]
                        found.append((i, j, a, abs(off[i] - off[j]), p3))
    return found


class Solids:
    """Point-in-solid over the closed runs (ray parity along a skewed direction)."""

    def __init__(self, V, groups):
        self.items = []
        for k, g in enumerate(groups):
            if not g[3] or not g[4]:
                continue
            T = V[np.array(g[4])]
            self.items.append((k, T.reshape(-1, 3).min(axis=0) - 0.01, T.reshape(-1, 3).max(axis=0) + 0.01, T))
        asm = defaultdict(list)
        for k, g in enumerate(groups):
            if not g[3] and g[1] != 'Ground':
                asm[g[1]].extend(g[4])
        for name, faces in asm.items():
            T = V[np.array(faces)]
            self.items.append((-1, T.reshape(-1, 3).min(axis=0) - 0.01, T.reshape(-1, 3).max(axis=0) + 0.01, T))
        self.lo = np.array([i[1] for i in self.items])
        self.hi = np.array([i[2] for i in self.items])

    def inside(self, p, skip=()):
        d = np.array([0.0123, 0.0171, 1.0])
        d /= np.linalg.norm(d)
        hits = np.nonzero(np.all((self.lo <= p) & (self.hi >= p), axis=1))[0]
        for h in hits:
            k, _, _, T = self.items[h]
            if k in skip:
                continue
            a, b, c = T[:, 0], T[:, 1], T[:, 2]
            e1, e2 = b - a, c - a
            pv = np.cross(d, e2)
            det = np.einsum("ij,ij->i", e1, pv)
            ok = np.abs(det) > 1e-12
            inv = np.zeros_like(det)
            inv[ok] = 1.0 / det[ok]
            tv = p - a
            u = np.einsum("ij,ij->i", tv, pv) * inv
            qv = np.cross(tv, e1)
            v = (qv @ d) * inv
            t = np.einsum("ij,ij->i", e2, qv) * inv
            n = np.count_nonzero(ok & (u >= 0) & (v >= 0) & (u + v <= 1) & (t > 1e-9))
            if n % 2 == 1:
                return True
        return False


def main():
    path = sys.argv[1]
    V, N, groups = load(path)
    W = weld(V)
    print(f"{len(V)} vertices, {sum(len(g[4]) for g in groups)} triangles, {len(groups)} runs")
    per_part = defaultdict(int)
    for g in groups:
        per_part[g[0]] += len(g[4])
    for p, c in per_part.items():
        print(f"  {p}: {c} triangles")

    # Winding against normals.
    all_tris = np.array([f for g in groups for f in g[4]])
    P = V[all_tris]
    geo = np.cross(P[:, 1] - P[:, 0], P[:, 2] - P[:, 0])
    nav = N[all_tris].sum(axis=1)
    big = np.linalg.norm(geo, axis=1) > 1e-6
    bad = (np.einsum("ij,ij->i", geo, nav) > 0) & big
    print(f"winding against the normals: {bad.sum()} triangles wound the wrong way")

    # Solids.
    problems = 0
    by_asm = defaultdict(list)
    for g in groups:
        if g[3]:
            b, nm, fl, vol, nt = check_closed(g[2], np.array(g[4]), W, V)
            if b or nm or fl or vol <= 0:
                problems += 1
                if problems <= 25:
                    print(f"  OPEN/BAD {g[0]}:{g[2]}: {len(b)} boundary, {len(nm)} non-manifold, {len(fl)} flipped, volume {vol / 1e6:.4f} m3"
                          f" at {rep_points(W, V, b or nm or fl)}")
        else:
            by_asm[g[1]].append(g)
    closed_runs = sum(1 for g in groups if g[3])
    print(f"closed solids: {closed_runs - problems} of {closed_runs} watertight and outward")
    for asm, gs in by_asm.items():
        tris = np.array([f for g in gs for f in g[4]])
        b, nm, fl, vol, nt = check_closed(asm, tris, W, V)
        print(f"assembly {asm!r} ({len(gs)} runs, {nt} tris): {len(b)} boundary edges, {len(nm)} non-manifold, {len(fl)} flipped,"
              f" volume {vol / 1e6:.3f} m3; boundary samples {rep_points(W, V, b, 6)}")
        if asm == "Ground" and b:
            # Boundary edges strictly inside the court's garden or on the seams are cracks.
            first = {}
            for i, w in enumerate(W):
                first.setdefault(w, i)
            inner = 0
            for e in b:
                m = (V[first[e[0]]] + V[first[e[1]]]) / 200.0
                if -5.6 < m[0] < 5.6 and 17.9 < m[1] < 29.1:
                    inner += 1
            print(f"    ground boundary edges inside the garden (cracks): {inner}")

    if "--no-fight" not in sys.argv:
        owner = np.concatenate([[k] * len(g[4]) for k, g in enumerate(groups)])
        found = fights(V, all_tris, owner)
        print(f"z-fighting candidates (same facing, < 1 cm apart, overlap > 1 cm2): {len(found)}")
        gidx = np.concatenate([[k] * len(g[4]) for k, g in enumerate(groups)])
        solids = Solids(V, groups)
        P3 = V[all_tris]
        nrm = -np.cross(P3[:, 1] - P3[:, 0], P3[:, 2] - P3[:, 0])
        nrm /= np.maximum(np.linalg.norm(nrm, axis=1), 1e-12)[:, None]
        visible = []
        for i, j, a, d, p3 in found:
            # The overlap is hidden if a point just in front of both faces is inside some other closed solid,
            # or the faces belong to the invisible guard.
            if groups[gidx[i]][0] == "PondGuard":
                continue
            oi, oj = P3[i][0] @ nrm[i], P3[j][0] @ nrm[i]
            back = min(oi, oj)
            c = p3 + nrm[i] * ((back - oi) + max(abs(oi - oj) / 2, 0.1))
            rear = gidx[i] if oi <= oj else gidx[j]
            if solids.inside(c, skip=(rear,)):
                continue
            visible.append((i, j, a, d))
        print(f"  of which visible (not inside another solid): {len(visible)}")
        found = visible
        kinds = defaultdict(lambda: [0, 0.0, None, 0.0])
        for i, j, a, d in found:
            gi, gj = groups[gidx[i]], groups[gidx[j]]
            key = tuple(sorted([f"{gi[0]}:{gi[2]}", f"{gj[0]}:{gj[2]}"]))
            k = kinds[key]
            k[0] += 1
            k[1] += a
            if k[2] is None or a > k[3]:
                k[2] = (np.round(V[all_tris[i]].mean(axis=0) / 100.0, 2), round(d, 2), gidx[i] == gidx[j])
                k[3] = a
        for key, (cnt, area, sample, _) in sorted(kinds.items(), key=lambda x: -x[1][1])[:40]:
            print(f"  {key[0]} vs {key[1]}: {cnt} pairs, {area:.0f} cm2, e.g. at {sample[0]} {sample[1]} cm apart{' (same solid)' if sample[2] else ''}")


if __name__ == "__main__":
    main()


