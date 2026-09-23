import Foundation
import RealityKit
import simd

/// Small procedural-mesh toolkit. Everything the museum is made of (walls with arched
/// openings, domes, vaults, rings, ribbons) is assembled here from triangles, so the
/// architecture needs no external 3D assets. Platform-neutral: shared by iOS and visionOS.
struct MeshBuilder {
    var positions: [SIMD3<Float>] = []
    var normals: [SIMD3<Float>] = []
    var uvs: [SIMD2<Float>] = []
    var indices: [UInt32] = []

    var isEmpty: Bool { indices.isEmpty }

    /// Adds a triangle, flipping the winding so its front face points along `n`.
    mutating func tri(_ a: SIMD3<Float>, _ b: SIMD3<Float>, _ c: SIMD3<Float>,
                      _ na: SIMD3<Float>, _ nb: SIMD3<Float>, _ nc: SIMD3<Float>,
                      _ ua: SIMD2<Float> = .zero, _ ub: SIMD2<Float> = .zero, _ uc: SIMD2<Float> = .zero) {
        let base = UInt32(positions.count)
        positions += [a, b, c]
        normals += [na, nb, nc]
        uvs += [ua, ub, uc]
        let face = cross(b - a, c - a)
        if dot(face, na + nb + nc) >= 0 {
            indices += [base, base + 1, base + 2]
        } else {
            indices += [base, base + 2, base + 1]
        }
    }

    /// Adds a quad a-b-c-d (in order around its edge) with one normal.
    mutating func quad(_ a: SIMD3<Float>, _ b: SIMD3<Float>, _ c: SIMD3<Float>, _ d: SIMD3<Float>,
                       normal n: SIMD3<Float>,
                       uv: (SIMD2<Float>, SIMD2<Float>, SIMD2<Float>, SIMD2<Float>) = (.zero, .zero, .zero, .zero)) {
        tri(a, b, c, n, n, n, uv.0, uv.1, uv.2)
        tri(a, c, d, n, n, n, uv.0, uv.2, uv.3)
    }

    /// Quad with per-vertex normals (for curved surfaces).
    mutating func quad(_ a: SIMD3<Float>, _ b: SIMD3<Float>, _ c: SIMD3<Float>, _ d: SIMD3<Float>,
                       normals na: SIMD3<Float>, _ nb: SIMD3<Float>, _ nc: SIMD3<Float>, _ nd: SIMD3<Float>,
                       uv: (SIMD2<Float>, SIMD2<Float>, SIMD2<Float>, SIMD2<Float>) = (.zero, .zero, .zero, .zero)) {
        tri(a, b, c, na, nb, nc, uv.0, uv.1, uv.2)
        tri(a, c, d, na, nc, nd, uv.0, uv.2, uv.3)
    }

    /// Axis-aligned box given its min and max corners (all six faces, outward normals).
    mutating func box(min a: SIMD3<Float>, max b: SIMD3<Float>) {
        let lo = simd_min(a, b), hi = simd_max(a, b)
        let x0 = lo.x, y0 = lo.y, z0 = lo.z, x1 = hi.x, y1 = hi.y, z1 = hi.z
        quad([x0, y0, z1], [x1, y0, z1], [x1, y1, z1], [x0, y1, z1], normal: [0, 0, 1])
        quad([x1, y0, z0], [x0, y0, z0], [x0, y1, z0], [x1, y1, z0], normal: [0, 0, -1])
        quad([x1, y0, z1], [x1, y0, z0], [x1, y1, z0], [x1, y1, z1], normal: [1, 0, 0])
        quad([x0, y0, z0], [x0, y0, z1], [x0, y1, z1], [x0, y1, z0], normal: [-1, 0, 0])
        quad([x0, y1, z1], [x1, y1, z1], [x1, y1, z0], [x0, y1, z0], normal: [0, 1, 0])
        quad([x0, y0, z0], [x1, y0, z0], [x1, y0, z1], [x0, y0, z1], normal: [0, -1, 0])
    }

    /// Box of size `size` centred at `c`, rotated about the vertical axis by `yaw`.
    mutating func box(center c: SIMD3<Float>, size s: SIMD3<Float>, yaw: Float) {
        var local = MeshBuilder()
        local.box(min: -s / 2, max: s / 2)
        let q = simd_quatf(angle: yaw, axis: [0, 1, 0])
        append(local, transform: { q.act($0) + c }, normalTransform: { q.act($0) })
    }

    mutating func append(_ other: MeshBuilder,
                         transform: (SIMD3<Float>) -> SIMD3<Float> = { $0 },
                         normalTransform: (SIMD3<Float>) -> SIMD3<Float> = { $0 }) {
        let base = UInt32(positions.count)
        positions += other.positions.map(transform)
        normals += other.normals.map(normalTransform)
        uvs += other.uvs
        indices += other.indices.map { $0 + base }
    }

    @MainActor
    func mesh(name: String = "mesh") -> MeshResource {
        var d = MeshDescriptor(name: name)
        d.positions = MeshBuffers.Positions(positions)
        d.normals = MeshBuffers.Normals(normals.map { simd_length($0) > 0 ? normalize($0) : [0, 1, 0] })
        d.textureCoordinates = MeshBuffers.TextureCoordinates(uvs)
        d.primitives = .triangles(indices)
        // Procedural meshes are always well formed; a failure here is a programming error.
        return try! MeshResource.generate(from: [d])
    }
}

// MARK: - Walls with openings

/// An opening in a wall run: a door (goes through the wall) or a niche (a recess).
struct WallOpening {
    /// Arc length along the run to the centre of the opening, in metres.
    var center: Float
    var width: Float
    /// Height where the jambs stop and the arch begins.
    var spring: Float
    /// Semicircular arched head (radius = width / 2); otherwise a flat head at `spring`.
    var arched: Bool = true
    /// Height of the bottom of the opening (0 for doors).
    var sill: Float = 0
    /// Doors pass through the wall; niches are half-round recesses of depth width / 2.
    var through: Bool = true

    /// A round (moon-gate) opening: a circle of diameter `width` centred `spring` above the floor.
    var round: Bool = false

    var top: Float { round ? spring + width / 2 : (arched ? spring + width / 2 : spring) }

    /// Height of the underside of the opening's head at lateral offset `d` from its centre.
    func head(at d: Float) -> Float {
        let r = width / 2
        guard arched || round else { return spring }
        let dd = min(abs(d), r)
        return spring + sqrt(max(0, r * r - dd * dd))
    }

    /// Height of the bottom of the opening at lateral offset `d` (the sill; curved for round ones).
    func bottom(at d: Float) -> Float {
        guard round else { return sill }
        let r = width / 2
        let dd = min(abs(d), r)
        return max(0, spring - sqrt(max(0, r * r - dd * dd)))
    }
}

/// A wall described by a polyline of its inner (room-side) face in plan coordinates
/// (x east, z south). The room lies on the `inside` side of the direction of travel.
struct WallRun {
    enum Side { case left, right }
    let points: [SIMD2<Float>]
    let inside: Side
    let height: Float
    let thickness: Float
    var openings: [WallOpening]
    /// Cumulative arc length at each point, for fast sampling.
    private let cumulative: [Float]

    init(points: [SIMD2<Float>], inside: Side, height: Float, thickness: Float, openings: [WallOpening] = []) {
        self.points = points
        self.inside = inside
        self.height = height
        self.thickness = thickness
        self.openings = openings
        var c: [Float] = [0]
        for (a, b) in zip(points, points.dropFirst()) { c.append(c.last! + simd_distance(a, b)) }
        cumulative = c
    }

    /// Unit vector pointing out of the room (into the wall) for a segment direction.
    func outward(_ dir: SIMD2<Float>) -> SIMD2<Float> {
        // Seen from above with x east and z south, the left of travel is (dir.z, -dir.x).
        let left = SIMD2<Float>(dir.y, -dir.x)
        return inside == .left ? -left : left
    }

    var length: Float { cumulative.last ?? 0 }

    /// Position and outward normal at arc length s.
    func sample(_ s: Float) -> (p: SIMD2<Float>, out: SIMD2<Float>, dir: SIMD2<Float>) {
        // Binary search for the segment containing s.
        var lo = 0, hi = cumulative.count - 1
        let s = min(max(s, 0), length)
        while hi - lo > 1 {
            let mid = (lo + hi) / 2
            if cumulative[mid] <= s { lo = mid } else { hi = mid }
        }
        var i = lo
        while i < points.count - 2 && cumulative[i + 1] - cumulative[i] <= 0 { i += 1 }
        let a = points[i], b = points[i + 1]
        let l = max(cumulative[i + 1] - cumulative[i], 1e-6)
        let dir = (b - a) / l
        let t = min(max((s - cumulative[i]) / l, 0), 1)
        return (a + (b - a) * t, outward(dir), dir)
    }

    /// Arc-length stations along the run: every polyline vertex, every `step` metres,
    /// and the exact edges of every opening.
    func stations(step: Float = 0.12) -> [Float] {
        var s: [Float] = [0]
        var acc: Float = 0
        for (a, b) in zip(points, points.dropFirst()) {
            let l = simd_distance(a, b)
            let n = max(1, Int(ceil(l / step)))
            for i in 1...n { s.append(acc + l * Float(i) / Float(n)) }
            acc += l
        }
        for o in openings {
            s.append(o.center - o.width / 2)
            s.append(o.center + o.width / 2)
            // Extra stations across arched heads for a smooth curve.
            if o.arched || o.round {
                for i in 1..<32 { s.append(o.center - o.width / 2 + o.width * Float(i) / 32) }
            }
        }
        return Array(Set(s.map { ($0 * 10000).rounded() / 10000 })).filter { $0 >= 0 && $0 <= acc }.sorted()
    }
}

extension MeshBuilder {
    /// Builds both faces of a wall run plus the reveals (tunnels) of its doors and the
    /// half-round recesses of its niches.
    mutating func wall(_ run: WallRun, faces: (inner: Bool, outer: Bool) = (true, true), reveals: Bool = true) {
        let st = run.stations()
        let h = run.height
        let t = run.thickness
        for (s0, s1) in zip(st, st.dropFirst()) where s1 - s0 > 1e-4 {
            let a = run.sample(s0), b = run.sample(s1)
            let sm = (s0 + s1) / 2
            let op = run.openings.first { abs(sm - $0.center) < $0.width / 2 }
            // Solid height intervals at each end of the strip.
            func intervals(_ s: Float, forOuter: Bool) -> [(Float, Float)] {
                guard let o = op, !(forOuter && !o.through) else { return [(0, h)] }
                var r: [(Float, Float)] = []
                let b = o.bottom(at: s - o.center)
                if b > 0 || o.round { r.append((0, b)) }
                r.append((min(o.head(at: s - o.center), h), h))
                return r
            }
            let ia = intervals(s0, forOuter: false), ib = intervals(s1, forOuter: false)
            let oa = intervals(s0, forOuter: true), ob = intervals(s1, forOuter: true)
            let inN = SIMD3<Float>(-a.out.x, 0, -a.out.y)
            let inNb = SIMD3<Float>(-b.out.x, 0, -b.out.y)
            if faces.inner {
                for (i, j) in zip(ia, ib) {
                    let p0 = SIMD3<Float>(a.p.x, i.0, a.p.y), p1 = SIMD3<Float>(b.p.x, j.0, b.p.y)
                    let p2 = SIMD3<Float>(b.p.x, j.1, b.p.y), p3 = SIMD3<Float>(a.p.x, i.1, a.p.y)
                    quad(p0, p1, p2, p3, normals: inN, inNb, inNb, inN,
                         uv: ([s0, i.0], [s1, j.0], [s1, j.1], [s0, i.1]))
                }
            }
            if faces.outer {
                let ao = a.p + a.out * t, bo = b.p + b.out * t
                for (i, j) in zip(oa, ob) {
                    let p0 = SIMD3<Float>(ao.x, i.0, ao.y), p1 = SIMD3<Float>(bo.x, j.0, bo.y)
                    let p2 = SIMD3<Float>(bo.x, j.1, bo.y), p3 = SIMD3<Float>(ao.x, i.1, ao.y)
                    quad(p0, p1, p2, p3, normals: -inN, -inNb, -inNb, -inN,
                         uv: ([s0, i.0], [s1, j.0], [s1, j.1], [s0, i.1]))
                }
            }
        }
        for o in run.openings {
            if o.through { if reveals { reveal(run, o) } } else { niche(run, o) }
        }
    }

    /// The jambs, soffit (and sill) of a door, from the inner face through to the outer face.
    private mutating func reveal(_ run: WallRun, _ o: WallOpening) {
        let c = run.sample(o.center)
        let out = SIMD3<Float>(c.out.x, 0, c.out.y)
        let r = o.width / 2
        // Outline in (lateral d, height) going left jamb up, over the head, right jamb down.
        var outline: [SIMD2<Float>] = [[-r, o.sill]]
        if o.round {
            outline = (0...64).map { i in
                let a = Float.pi + 2 * Float.pi * Float(i) / 64
                return [r * cos(a), max(0, o.spring + r * sin(a))]
            }
        } else if o.arched {
            outline.append([-r, o.spring])
            for i in 1..<32 {
                let a = Float.pi - Float.pi * Float(i) / 32
                outline.append([r * cos(a), o.spring + r * sin(a)])
            }
            outline.append([r, o.spring])
        } else {
            outline.append([-r, o.spring]); outline.append([r, o.spring])
        }
        if !o.round {
            outline.append([r, o.sill])
            if o.sill > 0 { outline.append([-r, o.sill]) }
        }
        let centre2 = SIMD2<Float>(0, (o.sill + o.top) / 2)
        for (u, v) in zip(outline, outline.dropFirst()) {
            let pu = run.sample(o.center + u.x), pv = run.sample(o.center + v.x)
            let edge = v - u
            var n2 = SIMD2<Float>(-edge.y, edge.x)
            if dot(n2, centre2 - (u + v) / 2) < 0 { n2 = -n2 }
            let tangent = SIMD3<Float>(c.dir.x, 0, c.dir.y)
            let n = normalize(tangent * n2.x + SIMD3<Float>(0, 1, 0) * n2.y)
            let a0 = SIMD3<Float>(pu.p.x, u.y, pu.p.y) - out * 0.01
            let b0 = SIMD3<Float>(pv.p.x, v.y, pv.p.y) - out * 0.01
            let depth = out * (run.thickness + 0.02)
            quad(a0, b0, b0 + depth, a0 + depth, normal: n)
        }
    }

    /// A half-round niche with a quarter-sphere head, recessed into the wall.
    private mutating func niche(_ run: WallRun, _ o: WallOpening) {
        let c = run.sample(o.center)
        let out = SIMD3<Float>(c.out.x, 0, c.out.y)
        let tan = SIMD3<Float>(c.dir.x, 0, c.dir.y)
        let base = SIMD3<Float>(c.p.x, 0, c.p.y) - out * 0.005
        let r = o.width / 2
        let n = 24
        for i in 0..<n {
            let a0 = Float.pi * Float(i) / Float(n), a1 = Float.pi * Float(i + 1) / Float(n)
            // Directions in the horizontal plane from the niche axis: from -tan through out to +tan.
            let d0 = -tan * cos(a0) + out * sin(a0), d1 = -tan * cos(a1) + out * sin(a1)
            // Cylinder part.
            let p0 = base + d0 * r + [0, o.sill, 0], p1 = base + d1 * r + [0, o.sill, 0]
            let q0 = base + d0 * r + [0, o.spring, 0], q1 = base + d1 * r + [0, o.spring, 0]
            quad(p0, p1, q1, q0, normals: -d0, -d1, -d1, -d0)
            // Sill (half disc).
            let sc = base + [0, o.sill, 0]
            tri(sc, p0, p1, [0, 1, 0], [0, 1, 0], [0, 1, 0])
            // Quarter-sphere head.
            let m = 12
            for j in 0..<m {
                let e0 = Float.pi / 2 * Float(j) / Float(m), e1 = Float.pi / 2 * Float(j + 1) / Float(m)
                func pt(_ d: SIMD3<Float>, _ e: Float) -> SIMD3<Float> {
                    base + [0, o.spring, 0] + d * (r * cos(e)) + [0, r * sin(e), 0]
                }
                let hc = base + [0, o.spring, 0]
                let v00 = pt(d0, e0), v10 = pt(d1, e0), v11 = pt(d1, e1), v01 = pt(d0, e1)
                quad(v00, v10, v11, v01, normals: normalize(hc - v00), normalize(hc - v10),
                     normalize(hc - v11), normalize(hc - v01))
            }
        }
    }

    /// A moulding band (cornice or skirting) running along the room side of a wall,
    /// interrupted where openings cut through its height range.
    mutating func band(_ run: WallRun, from y0: Float, to y1: Float, depth: Float) {
        let st = run.stations(step: 0.25)
        for (s0, s1) in zip(st, st.dropFirst()) where s1 - s0 > 1e-4 {
            let sm = (s0 + s1) / 2
            if run.openings.contains(where: { abs(sm - $0.center) < $0.width / 2 && $0.sill < y1 && $0.top > y0 }) {
                continue
            }
            let a = run.sample(s0), b = run.sample(s1)
            let na = SIMD3<Float>(-a.out.x, 0, -a.out.y), nb = SIMD3<Float>(-b.out.x, 0, -b.out.y)
            let fa = a.p - a.out * depth, fb = b.p - b.out * depth
            let A0 = SIMD3<Float>(fa.x, y0, fa.y), B0 = SIMD3<Float>(fb.x, y0, fb.y)
            let A1 = SIMD3<Float>(fa.x, y1, fa.y), B1 = SIMD3<Float>(fb.x, y1, fb.y)
            let W0a = SIMD3<Float>(a.p.x, y0, a.p.y), W0b = SIMD3<Float>(b.p.x, y0, b.p.y)
            let W1a = SIMD3<Float>(a.p.x, y1, a.p.y), W1b = SIMD3<Float>(b.p.x, y1, b.p.y)
            quad(A0, B0, B1, A1, normals: na, nb, nb, na)
            quad(W1a, W1b, B1, A1, normal: [0, 1, 0])
            quad(W0a, W0b, B0, A0, normal: [0, -1, 0])
        }
    }
}

// MARK: - Curved surfaces

extension MeshBuilder {
    /// Inner surface of a spherical dome band, seen from below. Elevation angles in radians
    /// (0 = springing line). UVs: u wraps `uRepeat` times around, v spans the band `vRepeat` times.
    mutating func domeBand(center c: SIMD3<Float>, radius r: Float, from e0: Float, to e1: Float,
                           segments: Int = 112, rings: Int = 12, uRepeat: Float = 1, vRepeat: Float = 1) {
        for i in 0..<segments {
            let a0 = 2 * Float.pi * Float(i) / Float(segments)
            let a1 = 2 * Float.pi * Float(i + 1) / Float(segments)
            for j in 0..<rings {
                let f0 = Float(j) / Float(rings), f1 = Float(j + 1) / Float(rings)
                let el0 = e0 + (e1 - e0) * f0, el1 = e0 + (e1 - e0) * f1
                func p(_ a: Float, _ e: Float) -> SIMD3<Float> {
                    c + [r * cos(e) * cos(a), r * sin(e), r * cos(e) * sin(a)]
                }
                let v00 = p(a0, el0), v10 = p(a1, el0), v11 = p(a1, el1), v01 = p(a0, el1)
                let u0 = uRepeat * Float(i) / Float(segments), u1 = uRepeat * Float(i + 1) / Float(segments)
                quad(v00, v10, v11, v01,
                     normals: normalize(c - v00), normalize(c - v10), normalize(c - v11), normalize(c - v01),
                     uv: ([u0, f0 * vRepeat], [u1, f0 * vRepeat], [u1, f1 * vRepeat], [u0, f1 * vRepeat]))
            }
        }
    }

    /// A barrel vault (half cylinder) along x from x0 to x1, axis at height `spring`,
    /// inner surface facing down. Cells whose centre falls in any `holes` rectangle
    /// (x range, |z| half-width) are left open for skylights. UVs in metres / `tile`.
    mutating func barrelVault(x0: Float, x1: Float, radius r: Float, spring: Float,
                              holes: [(x0: Float, x1: Float, halfWidth: Float)], tile: Float) {
        var xs: [Float] = []
        let n = Int(ceil(abs(x1 - x0) / 0.6))
        for i in 0...n { xs.append(x0 + (x1 - x0) * Float(i) / Float(n)) }
        for h in holes { xs.append(h.x0); xs.append(h.x1) }
        xs = Array(Set(xs)).sorted()
        var angles: [Float] = (0...48).map { Float.pi * Float($0) / 48 }
        for h in holes { let a = acos(h.halfWidth / r); angles.append(a); angles.append(Float.pi - a) }
        angles = Array(Set(angles)).sorted()
        for (xa, xb) in zip(xs, xs.dropFirst()) {
            for (a0, a1) in zip(angles, angles.dropFirst()) {
                let xm = (xa + xb) / 2, zm = r * cos((a0 + a1) / 2)
                if holes.contains(where: { xm > min($0.x0, $0.x1) && xm < max($0.x0, $0.x1) && abs(zm) < $0.halfWidth }) {
                    continue
                }
                func p(_ x: Float, _ a: Float) -> SIMD3<Float> { [x, spring + r * sin(a), r * cos(a)] }
                func n(_ a: Float) -> SIMD3<Float> { [0, -sin(a), -cos(a)] }
                let arc0 = r * a0 / tile, arc1 = r * a1 / tile
                quad(p(xa, a0), p(xb, a0), p(xb, a1), p(xa, a1), normals: n(a0), n(a0), n(a1), n(a1),
                     uv: ([xa / tile, arc0], [xb / tile, arc0], [xb / tile, arc1], [xa / tile, arc1]))
            }
        }
    }

    /// A transverse arch: the half-annulus between an intrados of radius `rIn` and the
    /// vault of radius `rOut`, springing at `spring`, `depth` thick along x, centred at x.
    mutating func transverseArch(x: Float, depth: Float, rIn: Float, rOut: Float, spring: Float) {
        let n = 48
        let xa = x - depth / 2, xb = x + depth / 2
        for i in 0..<n {
            let a0 = Float.pi * Float(i) / Float(n), a1 = Float.pi * Float(i + 1) / Float(n)
            func p(_ x: Float, _ r: Float, _ a: Float) -> SIMD3<Float> { [x, spring + r * sin(a), r * cos(a)] }
            // Faces towards +x and -x.
            quad(p(xb, rIn, a0), p(xb, rOut, a0), p(xb, rOut, a1), p(xb, rIn, a1), normal: [1, 0, 0])
            quad(p(xa, rIn, a0), p(xa, rOut, a0), p(xa, rOut, a1), p(xa, rIn, a1), normal: [-1, 0, 0])
            // Intrados (underside), facing the axis.
            let n0 = SIMD3<Float>(0, -sin(a0), -cos(a0)), n1 = SIMD3<Float>(0, -sin(a1), -cos(a1))
            quad(p(xa, rIn, a0), p(xb, rIn, a0), p(xb, rIn, a1), p(xa, rIn, a1), normals: n0, n0, n1, n1)
        }
    }

    /// Flat half-disc (lunette) of radius r in the plane x = const, above height `spring`.
    mutating func lunette(x: Float, radius r: Float, spring: Float, facing nx: Float) {
        let n = 48
        let c = SIMD3<Float>(x, spring, 0)
        for i in 0..<n {
            let a0 = Float.pi * Float(i) / Float(n), a1 = Float.pi * Float(i + 1) / Float(n)
            tri(c, c + [0, r * sin(a0), r * cos(a0)], c + [0, r * sin(a1), r * cos(a1)],
                [nx, 0, 0], [nx, 0, 0], [nx, 0, 0])
        }
    }

    /// Horizontal elliptical disc (floor or ceiling) with UVs in metres / tile.
    mutating func ellipseDisc(center c: SIMD3<Float>, a: Float, b: Float, up: Bool, tile: Float = 1, segments: Int = 96) {
        let n: SIMD3<Float> = up ? [0, 1, 0] : [0, -1, 0]
        for i in 0..<segments {
            let t0 = 2 * Float.pi * Float(i) / Float(segments), t1 = 2 * Float.pi * Float(i + 1) / Float(segments)
            let p0 = c + [a * cos(t0), 0, b * sin(t0)], p1 = c + [a * cos(t1), 0, b * sin(t1)]
            tri(c, p0, p1, n, n, n, [c.x / tile, c.z / tile], [p0.x / tile, p0.z / tile], [p1.x / tile, p1.z / tile])
        }
    }

    /// Axis-aligned horizontal rectangle (floor or ceiling) with UVs in metres / tile.
    mutating func floorRect(x0: Float, x1: Float, z0: Float, z1: Float, y: Float, up: Bool, tile: Float = 1) {
        let n: SIMD3<Float> = up ? [0, 1, 0] : [0, -1, 0]
        quad([x0, y, z0], [x1, y, z0], [x1, y, z1], [x0, y, z1], normal: n,
             uv: ([x0 / tile, z0 / tile], [x1 / tile, z0 / tile], [x1 / tile, z1 / tile], [x0 / tile, z1 / tile]))
    }

    /// A vertical ribbon along a polyline (e.g. a curved panel) from y0 to y1, facing `side`.
    /// UV u runs 0→1 along the ribbon's length, v 0 (bottom) → 1 (top) — note RealityKit's
    /// texture v runs upward.
    mutating func ribbon(_ pts: [SIMD2<Float>], y0: Float, y1: Float, facing normalFor: (SIMD2<Float>) -> SIMD2<Float>) {
        var lengths: [Float] = [0]
        for (a, b) in zip(pts, pts.dropFirst()) { lengths.append(lengths.last! + simd_distance(a, b)) }
        let total = lengths.last!
        for i in 0..<(pts.count - 1) {
            let a = pts[i], b = pts[i + 1]
            let na = normalFor(a), nb = normalFor(b)
            let u0 = lengths[i] / total, u1 = lengths[i + 1] / total
            quad([a.x, y0, a.y], [b.x, y0, b.y], [b.x, y1, b.y], [a.x, y1, a.y],
                 normals: [na.x, 0, na.y], [nb.x, 0, nb.y], [nb.x, 0, nb.y], [na.x, 0, na.y],
                 uv: ([u0, 0], [u1, 0], [u1, 1], [u0, 1]))
        }
    }

    /// A solid curved bench: a band of width w along a centreline polyline, h tall.
    mutating func curvedBench(_ pts: [SIMD2<Float>], width w: Float, height h: Float) {
        for i in 0..<(pts.count - 1) {
            let a = pts[i], b = pts[i + 1]
            let d = normalize(b - a)
            let side = SIMD2<Float>(-d.y, d.x) * (w / 2)
            let al = a + side, ar = a - side, bl = b + side, br = b - side
            let ns = SIMD3<Float>(side.x, 0, side.y) / (w / 2)
            quad([al.x, h, al.y], [bl.x, h, bl.y], [br.x, h, br.y], [ar.x, h, ar.y], normal: [0, 1, 0])
            quad([al.x, 0, al.y], [bl.x, 0, bl.y], [bl.x, h, bl.y], [al.x, h, al.y], normal: ns)
            quad([ar.x, 0, ar.y], [br.x, 0, br.y], [br.x, h, br.y], [ar.x, h, ar.y], normal: -ns)
            if i == 0 {
                quad([al.x, 0, al.y], [ar.x, 0, ar.y], [ar.x, h, ar.y], [al.x, h, al.y], normal: [-d.x, 0, -d.y])
            }
            if i == pts.count - 2 {
                quad([bl.x, 0, bl.y], [br.x, 0, br.y], [br.x, h, br.y], [bl.x, h, bl.y], normal: [d.x, 0, d.y])
            }
        }
    }

    /// Vertical flat shape (door leaf) filling an opening outline, in the plane through
    /// `origin` spanned by `tangent` and up, facing `normal`.
    mutating func doorLeaf(origin o: SIMD3<Float>, tangent t: SIMD3<Float>, normal n: SIMD3<Float>,
                           width w: Float, spring: Float, arched: Bool) {
        let r = w / 2
        var outline: [SIMD2<Float>] = [[-r, 0], [r, 0], [r, spring]]
        if arched {
            for i in 1..<32 {
                let a = Float.pi * Float(i) / 32
                outline.append([r * cos(a), spring + r * sin(a)])
            }
        }
        outline.append([-r, spring])
        let centre = SIMD2<Float>(0, spring * 0.6)
        func p(_ q: SIMD2<Float>) -> SIMD3<Float> { o + t * q.x + [0, q.y, 0] }
        for (a, b) in zip(outline, outline.dropFirst() + [outline[0]]) {
            tri(p(centre), p(a), p(b), n, n, n,
                [0.5 + centre.x / w, centre.y / (spring + r)], [0.5 + a.x / w, a.y / (spring + r)], [0.5 + b.x / w, b.y / (spring + r)])
        }
    }
}

// MARK: - Polyline helpers

enum Poly {
    /// Points on an ellipse centred at c with semi-axes a (x) and b (z), from angle t0 to t1.
    static func ellipse(_ c: SIMD2<Float>, a: Float, b: Float, from t0: Float, to t1: Float, segments: Int) -> [SIMD2<Float>] {
        (0...segments).map { i in
            let t = t0 + (t1 - t0) * Float(i) / Float(segments)
            return c + [a * cos(t), b * sin(t)]
        }
    }

    static func circle(_ c: SIMD2<Float>, r: Float, from t0: Float, to t1: Float, segments: Int) -> [SIMD2<Float>] {
        ellipse(c, a: r, b: r, from: t0, to: t1, segments: segments)
    }

    static func arcLength(_ pts: [SIMD2<Float>]) -> Float {
        zip(pts, pts.dropFirst()).reduce(0) { $0 + simd_distance($1.0, $1.1) }
    }
}

// MARK: - Solids of revolution and blobs

extension MeshBuilder {
    /// An ellipsoid (for tree crowns, lily pads, pebbles). UVs: u around, v up.
    mutating func ellipsoid(center c: SIMD3<Float>, radii r: SIMD3<Float>, segments: Int = 16, rings: Int = 10) {
        for j in 0..<rings {
            let p0 = -Float.pi / 2 + Float.pi * Float(j) / Float(rings)
            let p1 = -Float.pi / 2 + Float.pi * Float(j + 1) / Float(rings)
            for i in 0..<segments {
                let a0 = 2 * Float.pi * Float(i) / Float(segments), a1 = 2 * Float.pi * Float(i + 1) / Float(segments)
                func unit(_ a: Float, _ p: Float) -> SIMD3<Float> { [cos(p) * cos(a), sin(p), cos(p) * sin(a)] }
                func pt(_ u: SIMD3<Float>) -> SIMD3<Float> { c + u * r }
                func nrm(_ u: SIMD3<Float>) -> SIMD3<Float> { normalize(u / r) }
                let u00 = unit(a0, p0), u10 = unit(a1, p0), u11 = unit(a1, p1), u01 = unit(a0, p1)
                quad(pt(u00), pt(u10), pt(u11), pt(u01), normals: nrm(u00), nrm(u10), nrm(u11), nrm(u01),
                     uv: ([Float(i) / Float(segments), Float(j) / Float(rings)], [Float(i + 1) / Float(segments), Float(j) / Float(rings)],
                          [Float(i + 1) / Float(segments), Float(j + 1) / Float(rings)], [Float(i) / Float(segments), Float(j + 1) / Float(rings)]))
            }
        }
    }

    /// A lathe: a profile of (radius, height) points turned about a vertical axis at `c`.
    /// Outward normals; `inside` adds the inner surface too (for open vessels).
    mutating func lathe(_ profile: [SIMD2<Float>], center c: SIMD3<Float>, segments: Int = 40, inside: Bool = false) {
        for k in 0..<(profile.count - 1) {
            let a = profile[k], b = profile[k + 1]
            let d = b - a
            let n2 = normalize(SIMD2<Float>(d.y, -d.x))   // outward in (r, h)
            for i in 0..<segments {
                let t0 = 2 * Float.pi * Float(i) / Float(segments), t1 = 2 * Float.pi * Float(i + 1) / Float(segments)
                func p(_ q: SIMD2<Float>, _ t: Float) -> SIMD3<Float> { c + [q.x * cos(t), q.y, q.x * sin(t)] }
                func n(_ t: Float) -> SIMD3<Float> { [n2.x * cos(t), n2.y, n2.x * sin(t)] }
                let v0 = Float(k) / Float(profile.count - 1), v1 = Float(k + 1) / Float(profile.count - 1)
                let u0 = Float(i) / Float(segments), u1 = Float(i + 1) / Float(segments)
                quad(p(a, t0), p(a, t1), p(b, t1), p(b, t0), normals: n(t0), n(t1), n(t1), n(t0),
                     uv: ([u0, v0], [u1, v0], [u1, v1], [u0, v1]))
                if inside {
                    quad(p(a, t0), p(a, t1), p(b, t1), p(b, t0), normals: -n(t0), -n(t1), -n(t1), -n(t0),
                         uv: ([u0, v0], [u1, v0], [u1, v1], [u0, v1]))
                }
            }
        }
    }

    /// A tapered round stem between two points (trunks, branches, bamboo culms).
    mutating func stem(from a: SIMD3<Float>, to b: SIMD3<Float>, r0: Float, r1: Float, segments: Int = 8) {
        let d = b - a, len = simd_length(d)
        guard len > 1e-4 else { return }
        var local = MeshBuilder()
        local.lathe([[r0, 0], [r1, len]], center: .zero, segments: segments)
        let q = simd_quatf(from: [0, 1, 0], to: d / len)
        append(local, transform: { q.act($0) + a }, normalTransform: { q.act($0) })
    }
}

extension MeshBuilder {
    /// A horizontal floor made of square cells inside a bounding rectangle, keeping only the
    /// cells whose centre passes `include` (for floors with holes). UVs in metres / tile.
    mutating func floorCells(x0: Float, x1: Float, z0: Float, z1: Float, y: Float, cell: Float, tile: Float = 1,
                             up: Bool = true, include: (SIMD2<Float>) -> Bool) {
        let nx = Int(ceil((x1 - x0) / cell)), nz = Int(ceil((z1 - z0) / cell))
        for i in 0..<nx {
            for j in 0..<nz {
                let ax = x0 + Float(i) * cell, bx = min(x1, ax + cell)
                let az = z0 + Float(j) * cell, bz = min(z1, az + cell)
                guard include([(ax + bx) / 2, (az + bz) / 2]) else { continue }
                floorRect(x0: ax, x1: bx, z0: az, z1: bz, y: y, up: up, tile: tile)
            }
        }
    }
}
