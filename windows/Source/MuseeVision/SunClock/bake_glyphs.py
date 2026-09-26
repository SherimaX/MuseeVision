"""
Bakes the sun clock's bronze capitals into SunClockGlyphs.h (run from anywhere; needs fontTools):

    python bake_glyphs.py

The letters are Centaur's capitals (Bruce Rogers, after Jenson; C:/Windows/Fonts/CENTAUR.TTF), the
closest Windows face to the Trajan inscription: A E H I M N O R S U for HORAS NON NUMERO NISI
SERENAS, and I V X for the hour numerals. Each glyph is flattened (quadratic curves split until they
stray less than 1/600 of the cap height), scaled so the cap height is 1 (baseline 0), its contours
wound with the solid on the left (outers anticlockwise, counters clockwise, y up), and triangulated
once (ear clipping, counters bridged in). ASunClock insets the top face along each point's mitre
vector for the bevel, so the same triangles serve the inset top; the script checks that no triangle
flips at the bevels used (up to 0.016 of the cap height).
"""
import math
import os

from fontTools.pens.basePen import BasePen
from fontTools.ttLib import TTFont

FONT = "C:/Windows/Fonts/CENTAUR.TTF"
LETTERS = "AEHIMNORSUVX"
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "SunClockGlyphs.h")
TOLERANCE = 1 / 600          # of the cap height
MAX_BEVEL = 0.016            # of the cap height: the numerals' bevel
SMOOTH_DEGREES = 28          # a turn smaller than this is a curve (smooth normals)
MITRE_LIMIT = 2.5


class FlatPen(BasePen):
    def __init__(self, glyphset, tol):
        super().__init__(glyphset)
        self.tol = tol
        self.contours = []
        self.cur = None

    def _moveTo(self, p):
        self.cur = [p]

    def _lineTo(self, p):
        self.cur.append(p)

    def _qCurveToOne(self, p1, p2):
        p0 = self.cur[-1]
        dx, dy = p0[0] - 2 * p1[0] + p2[0], p0[1] - 2 * p1[1] + p2[1]
        n = max(1, math.ceil(math.sqrt(math.hypot(dx, dy) / (4 * self.tol))))
        for i in range(1, n + 1):
            t = i / n
            u = 1 - t
            self.cur.append((u * u * p0[0] + 2 * u * t * p1[0] + t * t * p2[0],
                             u * u * p0[1] + 2 * u * t * p1[1] + t * t * p2[1]))

    def _curveToOne(self, p1, p2, p3):
        p0 = self.cur[-1]
        n = 16
        for i in range(1, n + 1):
            t = i / n
            u = 1 - t
            self.cur.append(tuple(u ** 3 * a + 3 * u * u * t * b + 3 * u * t * t * c + t ** 3 * d
                                  for a, b, c, d in zip(p0, p1, p2, p3)))

    def _closePath(self):
        if self.cur and len(self.cur) > 2:
            self.contours.append(self.cur)
        self.cur = None

    _endPath = _closePath


def area(c):
    return 0.5 * sum(c[i][0] * c[(i + 1) % len(c)][1] - c[(i + 1) % len(c)][0] * c[i][1] for i in range(len(c)))


def clean(c, eps):
    """Drops repeated points and points in line with their neighbours."""
    out = []
    for p in c:
        if not out or math.dist(out[-1], p) > eps:
            out.append(p)
    if len(out) > 1 and math.dist(out[0], out[-1]) <= eps:
        out.pop()
    changed = True
    while changed and len(out) > 3:
        changed = False
        for i in range(len(out)):
            a, b, c2 = out[i - 1], out[i], out[(i + 1) % len(out)]
            cross = (b[0] - a[0]) * (c2[1] - b[1]) - (b[1] - a[1]) * (c2[0] - b[0])
            if abs(cross) < 1e-9 * max(1e-12, math.dist(a, b) * math.dist(b, c2)) or abs(cross) < eps * eps * 1e-3:
                out.pop(i)
                changed = True
                break
    return out


def inside(pt, poly):
    x, y = pt
    hit = False
    for i in range(len(poly)):
        (x0, y0), (x1, y1) = poly[i], poly[(i + 1) % len(poly)]
        if (y0 > y) != (y1 > y) and x < x0 + (y - y0) * (x1 - x0) / (y1 - y0):
            hit = not hit
    return hit


def cross(o, a, b):
    return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])


def in_triangle(p, a, b, c):
    return cross(a, b, p) >= -1e-12 and cross(b, c, p) >= -1e-12 and cross(c, a, p) >= -1e-12


def bridge(outer, hole):
    """Merges a clockwise hole into an anticlockwise outer (lists of (index, point))."""
    m = max(range(len(hole)), key=lambda i: hole[i][1][0])
    mx, my = hole[m][1]
    best, best_x = None, math.inf
    for i in range(len(outer)):
        (_, a), (_, b) = outer[i], outer[(i + 1) % len(outer)]
        if (a[1] - my) * (b[1] - my) > 0 or a[1] == b[1]:
            continue
        x = a[0] + (my - a[1]) * (b[0] - a[0]) / (b[1] - a[1])
        if mx <= x < best_x:
            best_x, best = x, i
    i0, i1 = best, (best + 1) % len(outer)
    p = i0 if outer[i0][1][0] > outer[i1][1][0] else i1
    ix = (best_x, my)
    # A reflex vertex inside the triangle (M, I, P) would block the bridge: take the one nearest in angle.
    cands = []
    for j, (_, q) in enumerate(outer):
        if j == p:
            continue
        if in_triangle(q, (mx, my), ix, outer[p][1]) or in_triangle(q, (mx, my), outer[p][1], ix):
            prev, nxt = outer[j - 1][1], outer[(j + 1) % len(outer)][1]
            if cross(prev, q, nxt) <= 0:
                cands.append((abs(math.atan2(q[1] - my, q[0] - mx)), math.dist(q, (mx, my)), j))
    if cands:
        p = min(cands)[2]
    return outer[:p + 1] + hole[m:] + hole[:m + 1] + outer[p:]


def earclip(poly):
    """Triangles (as original indices) of a simple anticlockwise polygon [(index, point)]."""
    idx = list(range(len(poly)))
    tris = []
    guard = 0
    while len(idx) > 3 and guard < 100000:
        guard += 1
        n = len(idx)
        cut = False
        for k in range(n):
            ia, ib, ic = idx[k - 1], idx[k], idx[(k + 1) % n]
            a, b, c = poly[ia][1], poly[ib][1], poly[ic][1]
            if cross(a, b, c) <= 1e-14:
                continue
            blocked = False
            for j in idx:
                if j in (ia, ib, ic):
                    continue
                q = poly[j][1]
                if q in (a, b, c):
                    continue
                if in_triangle(q, a, b, c):
                    blocked = True
                    break
            if blocked:
                continue
            tris.append((poly[ia][0], poly[ib][0], poly[ic][0]))
            idx.pop(k)
            cut = True
            break
        if not cut:
            # Degenerate leftovers: drop the flattest vertex.
            k = min(range(n), key=lambda k: abs(cross(poly[idx[k - 1]][1], poly[idx[k]][1], poly[idx[(k + 1) % n]][1])))
            idx.pop(k)
    if len(idx) == 3:
        a, b, c = (poly[i][1] for i in idx)
        if cross(a, b, c) > 1e-14:
            tris.append(tuple(poly[i][0] for i in idx))
    return tris


def delaunay_flips(points, tris, fixed):
    """Lawson flips of every edge that isn't an outline edge, until the triangulation is Delaunay."""
    tris = [list(t) for t in tris]

    def incircle(a, b, c, d):
        ax, ay = a[0] - d[0], a[1] - d[1]
        bx, by = b[0] - d[0], b[1] - d[1]
        cx, cy = c[0] - d[0], c[1] - d[1]
        return (ax * ax + ay * ay) * (bx * cy - cx * by) - (bx * bx + by * by) * (ax * cy - cx * ay) + \
            (cx * cx + cy * cy) * (ax * by - bx * ay)

    for _ in range(200):
        edges = {}
        for ti, t in enumerate(tris):
            for k in range(3):
                a, b = t[k], t[(k + 1) % 3]
                edges.setdefault((min(a, b), max(a, b)), []).append(ti)
        flipped = 0
        done = set()
        for (a, b), ts in edges.items():
            if len(ts) != 2 or (a, b) in fixed or ts[0] in done or ts[1] in done:
                continue
            t0, t1 = tris[ts[0]], tris[ts[1]]
            c = next(v for v in t0 if v not in (a, b))
            d = next(v for v in t1 if v not in (a, b))
            # Orient t0 as (a, b, c) anticlockwise.
            p = [points[v] for v in (a, b, c)]
            if cross(*p) < 0:
                a, b = b, a
            if incircle(points[a], points[b], points[c], points[d]) <= 1e-18:
                continue
            # The flip must keep both triangles anticlockwise (the quad convex).
            if cross(points[c], points[a], points[d]) <= 1e-14 or cross(points[d], points[b], points[c]) <= 1e-14:
                continue
            tris[ts[0]] = [c, a, d]
            tris[ts[1]] = [d, b, c]
            done.update(ts)
            flipped += 1
        if not flipped:
            break
    return [tuple(t) for t in tris]


def glyph(font, ch, cap):
    gs = font.getGlyphSet()
    name = font.getBestCmap()[ord(ch)]
    pen = FlatPen(gs, TOLERANCE * cap)
    gs[name].draw(pen)
    contours = [[(x / cap, y / cap) for x, y in c] for c in pen.contours]
    contours = [clean(c, 1e-4) for c in contours]
    contours = [c for c in contours if len(c) >= 3 and abs(area(c)) > 1e-6]
    # Outer or counter by nesting depth; wind outers anticlockwise, counters clockwise.
    depth = [sum(inside(c[0], o) for j, o in enumerate(contours) if j != i) for i, c in enumerate(contours)]
    for i, c in enumerate(contours):
        want_ccw = depth[i] % 2 == 0
        if (area(c) > 0) != want_ccw:
            c.reverse()
    points, starts = [], []
    for c in contours:
        starts.append(len(points))
        points.extend(c)
    starts.append(len(points))
    # Triangulate each outer with the counters directly inside it.
    tris = []
    for i, c in enumerate(contours):
        if depth[i] % 2:
            continue
        poly = [(starts[i] + k, p) for k, p in enumerate(c)]
        holes = [j for j in range(len(contours)) if depth[j] == depth[i] + 1 and inside(contours[j][0], c)]
        holes.sort(key=lambda j: -max(p[0] for p in contours[j]))
        for j in holes:
            poly = bridge(poly, [(starts[j] + k, p) for k, p in enumerate(contours[j])])
        tris += earclip(poly)
    fixed = set()
    for ci in range(len(contours)):
        s, e = starts[ci], starts[ci + 1]
        for k in range(s, e):
            a, b = k, s + (k + 1 - s) % (e - s)
            fixed.add((min(a, b), max(a, b)))
    tris = delaunay_flips(points, tris, fixed)
    # Mitre vectors into the solid (left of each edge) and smooth flags.
    mitres, smooth = [], []
    for ci in range(len(contours)):
        s, e = starts[ci], starts[ci + 1]
        n = e - s
        for k in range(n):
            p0, p1, p2 = points[s + (k - 1) % n], points[s + k], points[s + (k + 1) % n]
            e0 = (p1[0] - p0[0], p1[1] - p0[1])
            e1 = (p2[0] - p1[0], p2[1] - p1[1])
            l0, l1 = math.hypot(*e0), math.hypot(*e1)
            n0 = (-e0[1] / l0, e0[0] / l0)
            n1 = (-e1[1] / l1, e1[0] / l1)
            d = 1 + n0[0] * n1[0] + n0[1] * n1[1]
            m = ((n0[0] + n1[0]) / max(d, 1e-6), (n0[1] + n1[1]) / max(d, 1e-6))
            ml = math.hypot(*m)
            if ml > MITRE_LIMIT:
                m = (m[0] * MITRE_LIMIT / ml, m[1] * MITRE_LIMIT / ml)
            # Where the stroke is thin (a serif's tip), the bevel narrows: the point may move at most
            # 0.4 of the way to the outline across from it.
            ml = math.hypot(*m)
            ux, uy = m[0] / ml, m[1] / ml
            reach = math.inf
            for cj in range(len(contours)):
                s2, e2 = starts[cj], starts[cj + 1]
                for q in range(s2, e2):
                    q2 = s2 + (q + 1 - s2) % (e2 - s2)
                    if s + k in (q, q2):
                        continue
                    a, b = points[q], points[q2]
                    ex, ey = b[0] - a[0], b[1] - a[1]
                    den = ux * ey - uy * ex
                    if abs(den) < 1e-12:
                        continue
                    t = ((a[0] - p1[0]) * ey - (a[1] - p1[1]) * ex) / den
                    v = ((a[0] - p1[0]) * uy - (a[1] - p1[1]) * ux) / den
                    if t > 1e-9 and -1e-9 <= v <= 1 + 1e-9:
                        reach = min(reach, t)
            scale = min(1.0, 0.4 * reach / (MAX_BEVEL * ml))
            m = (m[0] * scale, m[1] * scale)
            mitres.append(m)
            turn = math.degrees(math.acos(max(-1.0, min(1.0, (e0[0] * e1[0] + e0[1] * e1[1]) / (l0 * l1)))))
            smooth.append(turn < SMOOTH_DEGREES)
    # At the sharpest serif tips a triangle can still turn over: narrow the bevel at its points until
    # none does.
    for _ in range(40):
        q = [(p[0] + MAX_BEVEL * m[0], p[1] + MAX_BEVEL * m[1]) for p, m in zip(points, mitres)]
        bad = {v for t in tris if cross(q[t[0]], q[t[1]], q[t[2]]) <= 1e-9 for v in t}
        if not bad:
            break
        for v in bad:
            mitres[v] = (mitres[v][0] * 0.7, mitres[v][1] * 0.7)
    # Check: no triangle flips on the inset top.
    flips = 0
    for b in (0.008, MAX_BEVEL):
        q = [(p[0] + b * m[0], p[1] + b * m[1]) for p, m in zip(points, mitres)]
        flips = max(flips, sum(1 for a, bb, c in tris if cross(q[a], q[bb], q[c]) <= 0))
    tri_area = sum(cross(points[a], points[b], points[c]) / 2 for a, b, c in tris)
    shape_area = sum(area(c) for c in contours)
    return {"points": points, "starts": starts, "tris": tris, "mitres": mitres, "smooth": smooth,
            "advance": gs[name].width / cap, "flips": flips, "area_error": abs(tri_area - shape_area)}


def main():
    font = TTFont(FONT)
    gs = font.getGlyphSet()
    name = font.getBestCmap()[ord("H")]
    from fontTools.pens.boundsPen import BoundsPen
    bp = BoundsPen(gs)
    gs[name].draw(bp)
    cap = bp.bounds[3]
    glyphs = {ch: glyph(font, ch, cap) for ch in LETTERS}
    lines = [
        "// Generated by bake_glyphs.py from Centaur (C:/Windows/Fonts/CENTAUR.TTF): do not edit.",
        "// Capitals in units of the cap height (baseline 0, cap height 1, x from the pen position);",
        "// contours with the solid on their left; triangles over the glyph's points; mitre vectors",
        "// into the solid for the bevel; smooth flags where the outline is a curve.",
        "#pragma once",
        "",
        "#include \"CoreMinimal.h\"",
        "",
        "namespace SunClockGlyphs",
        "{",
        "\tstruct FGlyph { TCHAR Char; float Advance; int32 FirstContour, NumContours, FirstTri, NumTris; };",
        "",
    ]
    all_pts, all_mitre, all_smooth, all_starts, all_tris, table = [], [], [], [], [], []
    for ch in LETTERS:
        g = glyphs[ch]
        base = len(all_pts)
        first_contour = len(all_starts)
        for s in g["starts"][:-1]:
            all_starts.append(base + s)
        all_pts += g["points"]
        all_mitre += g["mitres"]
        all_smooth += g["smooth"]
        first_tri = len(all_tris)
        all_tris += [(base + a, base + b, base + c) for a, b, c in g["tris"]]
        table.append((ch, g["advance"], first_contour, len(g["starts"]) - 1, first_tri, len(g["tris"])))
        print(f"{ch}: {len(g['points'])} points, {len(g['starts']) - 1} contours, {len(g['tris'])} triangles, "
              f"advance {g['advance']:.3f}, flips at the bevel {g['flips']}, area error {g['area_error']:.2e}")
    all_starts.append(len(all_pts))

    def floats(vals, per=8):
        out = []
        for i in range(0, len(vals), per):
            out.append("\t\t" + ", ".join(f"{v:.5f}f" for v in vals[i:i + per]) + ",")
        return out

    lines.append(f"\tinline constexpr int32 NumPoints = {len(all_pts)};")
    lines.append("\t/** Each contour's first point; one more entry closes the last. */")
    lines.append("\tinline constexpr int32 ContourStart[] = {")
    for i in range(0, len(all_starts), 16):
        lines.append("\t\t" + ", ".join(str(v) for v in all_starts[i:i + 16]) + ",")
    lines.append("\t};")
    lines.append("\t/** x, y of each point. */")
    lines.append("\tinline constexpr float Point[] = {")
    lines += floats([v for p in all_pts for v in p])
    lines.append("\t};")
    lines.append("\t/** The mitre vector into the solid at each point (a unit bevel moves the point this far). */")
    lines.append("\tinline constexpr float Mitre[] = {")
    lines += floats([v for p in all_mitre for v in p])
    lines.append("\t};")
    lines.append("\tinline constexpr uint8 Smooth[] = {")
    for i in range(0, len(all_smooth), 32):
        lines.append("\t\t" + ", ".join("1" if s else "0" for s in all_smooth[i:i + 32]) + ",")
    lines.append("\t};")
    lines.append("\tinline constexpr int32 Tri[] = {")
    flat = [v for t in all_tris for v in t]
    for i in range(0, len(flat), 24):
        lines.append("\t\t" + ", ".join(str(v) for v in flat[i:i + 24]) + ",")
    lines.append("\t};")
    lines.append("\tinline constexpr FGlyph Glyphs[] = {")
    for ch, adv, fc, nc, ft, nt in table:
        lines.append(f"\t\t{{TEXT('{ch}'), {adv:.5f}f, {fc}, {nc}, {ft}, {nt}}},")
    lines.append("\t};")
    lines.append("")
    lines.append("\tinline const FGlyph* Find(TCHAR C)")
    lines.append("\t{")
    lines.append("\t\tfor (const FGlyph& G : Glyphs) { if (G.Char == C) { return &G; } }")
    lines.append("\t\treturn nullptr;")
    lines.append("\t}")
    lines.append("}")
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    print(f"wrote {OUT}: {len(all_pts)} points, {len(all_tris)} triangles")


if __name__ == "__main__":
    main()
