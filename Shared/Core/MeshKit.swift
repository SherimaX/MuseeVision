import Foundation
import RealityKit
import simd

/// How finely curved architecture is tessellated. 1 on the phone and headset (arches 48
/// segments per semicircle, the Rotunda's dome 336 round); the macOS USD exporter builds at 3
/// for Unreal (Nanite, ray tracing, close cameras). `MUSEE_TESSELLATION=<n>` overrides either.
/// Surfaces that meet use the same counts, so raising it never opens seams.
enum Tessellation {
    static let quality: Int = {
        // At most 4: finer, the first arch station would come within 0.1 mm of the jamb.
        if let s = ProcessInfo.processInfo.environment["MUSEE_TESSELLATION"], let q = Int(s), q >= 1 { return min(q, 4) }
        #if os(macOS)
        return 3
        #else
        return 1
        #endif
    }()

    /// A phone segment count scaled by the quality.
    static func segments(_ phone: Int) -> Int { phone * quality }

    /// Segments per semicircle of an arched or round head (and of the niches and passage vaults
    /// that continue it).
    static var arch: Int { segments(48) }
}

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

    /// |d| clamped to the half-width. Within 0.06 mm of a jamb counts as on it: stations are
    /// rounded to 0.1 mm, and a jamb station a hair inside would otherwise start the arc 1.4 cm up
    /// (the arc is vertical there) and open a slit beside a niche.
    func lateral(_ d: Float) -> Float {
        let r = width / 2, dd = min(abs(d), r)
        return r - dd < 6e-5 ? r : dd
    }

    /// Height of the underside of the opening's head at lateral offset `d` from its centre.
    func head(at d: Float) -> Float {
        let r = width / 2
        guard arched || round else { return spring }
        let dd = lateral(d)
        return spring + sqrt(max(0, r * r - dd * dd))
    }

    /// Height of the bottom of the opening at lateral offset `d` (the sill; curved for round ones).
    func bottom(at d: Float) -> Float {
        guard round else { return sill }
        let r = width / 2
        let dd = lateral(d)
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

    /// True when the polyline ends where it starts (a room's closed loop).
    var isClosed: Bool { points.count > 2 && simd_distance(points[0], points[points.count - 1]) < 1e-3 }

    /// The segment containing arc length s and the fraction along it (zero-length segments skipped).
    private func locate(_ s: Float) -> (i: Int, t: Float) {
        // Binary search for the segment containing s.
        var lo = 0, hi = cumulative.count - 1
        let s = min(max(s, 0), length)
        while hi - lo > 1 {
            let mid = (lo + hi) / 2
            if cumulative[mid] <= s { lo = mid } else { hi = mid }
        }
        var i = lo
        while i < points.count - 2 && cumulative[i + 1] - cumulative[i] <= 0 { i += 1 }
        let l = max(cumulative[i + 1] - cumulative[i], 1e-6)
        return (i, min(max((s - cumulative[i]) / l, 0), 1))
    }

    /// Position and outward normal at arc length s.
    func sample(_ s: Float) -> (p: SIMD2<Float>, out: SIMD2<Float>, dir: SIMD2<Float>) {
        let (i, t) = locate(s)
        let a = points[i], b = points[i + 1]
        let l = max(cumulative[i + 1] - cumulative[i], 1e-6)
        let dir = (b - a) / l
        return (a + (b - a) * t, outward(dir), dir)
    }

    /// Outward normal of segment i, if it has length.
    private func segmentOut(_ i: Int) -> SIMD2<Float>? {
        guard i >= 0, i < points.count - 1 else { return nil }
        let d = points[i + 1] - points[i], l = simd_length(d)
        return l > 1e-6 ? outward(d / l) : nil
    }

    /// Outward normals of the two segments meeting at vertex v (wrapping round a closed loop).
    private func corner(_ v: Int) -> (SIMD2<Float>, SIMD2<Float>)? {
        let n = points.count - 1
        var p = v - 1, q = v
        if isClosed {
            if p < 0 { p = n - 1 }
            if q >= n { q = 0 }
        }
        guard let a = segmentOut(p), let b = segmentOut(q) else { return nil }
        return (a, b)
    }

    /// Outward normal at s for shading, on the segment that contains `m` (a strip's midpoint):
    /// blended across vertices where the polyline turns by less than 20° (a polygonised curve),
    /// so curved walls shade smoothly while real corners stay sharp.
    func smoothOut(_ s: Float, on m: Float) -> SIMD2<Float> {
        let i = locate(m).i
        guard let own = segmentOut(i) else { return sample(s).out }
        let l = max(cumulative[i + 1] - cumulative[i], 1e-6)
        let t = min(max((s - cumulative[i]) / l, 0), 1)
        func atVertex(_ v: Int) -> SIMD2<Float> {
            guard let c = corner(v), dot(c.0, c.1) > 0.94 else { return own }
            return normalize(c.0 + c.1)
        }
        let n = atVertex(i) * (1 - t) + atVertex(i + 1) * t
        return simd_length(n) > 1e-6 ? normalize(n) : own
    }

    /// Offset from the inner face to the outer face per metre of thickness at s: the outward
    /// normal, mitred at polyline vertices so outer corners and curves close exactly.
    func outerOffset(_ s: Float) -> SIMD2<Float> {
        let (i, t) = locate(s)
        let l = cumulative[i + 1] - cumulative[i]
        let v = t * l < 2e-4 ? i : ((1 - t) * l < 2e-4 ? i + 1 : -1)
        guard v >= 0, let c = corner(v), simd_length(c.0 + c.1) > 1e-3 else { return sample(s).out }
        let u = normalize(c.0 + c.1)
        return u / max(0.25, dot(u, c.1))
    }

    /// Arc-length stations along the run: every polyline vertex, every `step` metres, the exact
    /// edges of every opening, and equal-angle stations round arched and round heads (which
    /// replace the even steps inside them, so the wall faces, reveals and niches share one
    /// set of vertices along the arc).
    func stations(step: Float = 0.12) -> [Float] {
        func inHead(_ x: Float) -> Bool {
            openings.contains { ($0.arched || $0.round) && abs(x - $0.center) < $0.width / 2 - 1e-4 }
        }
        var s: [Float] = [0]
        var acc: Float = 0
        for (a, b) in zip(points, points.dropFirst()) {
            let l = simd_distance(a, b)
            let n = max(1, Int(ceil(l / step)))
            // Polyline vertices always stay.
            for i in 1...n where i == n || !inHead(acc + l * Float(i) / Float(n)) {
                s.append(acc + l * Float(i) / Float(n))
            }
            acc += l
        }
        for o in openings {
            s.append(o.center - o.width / 2)
            s.append(o.center + o.width / 2)
            // Equal angles round the head: equal steps along the run left the arc coarse at the
            // springing, where the old wall face and reveal parted by up to 3 cm.
            if o.arched || o.round {
                let n = Tessellation.arch
                for i in 1..<n { s.append(o.center - o.width / 2 * cos(Float.pi * Float(i) / Float(n))) }
            }
        }
        // Both ends exactly. (Filtering the rounded list by `<= length` could drop the last
        // station when it rounded up, leaving the run's last strip unbuilt.)
        let inner = Set(s.map(Self.snap)).filter { $0 > 0 && $0 < acc }
        return [0] + inner.sorted() + [acc]
    }

    /// Stations are rounded to 0.1 mm; openings find their edges among them with this.
    static func snap(_ s: Float) -> Float { (s * 10000).rounded() / 10000 }
}

extension MeshBuilder {
    /// Builds both faces of a wall run plus the reveals (tunnels) of its doors and the
    /// half-round recesses of its niches.
    mutating func wall(_ run: WallRun, faces: (inner: Bool, outer: Bool) = (true, true), reveals: Bool = true) {
        let st = run.stations()
        let h = run.height
        let t = run.thickness
        // (Stations are rounded to 0.1 mm, so any positive gap is a real strip; the old 1e-4
        // threshold could drop a 0.1 mm strip and leave a crack.)
        for (s0, s1) in zip(st, st.dropFirst()) where s1 - s0 > 1e-6 {
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
            // Smooth normals on curved runs, taken on this strip's own segment (so the strip
            // before a corner no longer shades with the next wall's normal).
            let na = run.smoothOut(s0, on: sm), nb = run.smoothOut(s1, on: sm)
            let inN = SIMD3<Float>(-na.x, 0, -na.y)
            let inNb = SIMD3<Float>(-nb.x, 0, -nb.y)
            if faces.inner {
                for (i, j) in zip(ia, ib) {
                    let p0 = SIMD3<Float>(a.p.x, i.0, a.p.y), p1 = SIMD3<Float>(b.p.x, j.0, b.p.y)
                    let p2 = SIMD3<Float>(b.p.x, j.1, b.p.y), p3 = SIMD3<Float>(a.p.x, i.1, a.p.y)
                    quad(p0, p1, p2, p3, normals: inN, inNb, inNb, inN,
                         uv: ([s0, i.0], [s1, j.0], [s1, j.1], [s0, i.1]))
                }
            }
            if faces.outer {
                // Mitred at vertices: outer corners close square instead of chamfering.
                let ao = a.p + run.outerOffset(s0) * t, bo = b.p + run.outerOffset(s1) * t
                for (i, j) in zip(oa, ob) {
                    let p0 = SIMD3<Float>(ao.x, i.0, ao.y), p1 = SIMD3<Float>(bo.x, j.0, bo.y)
                    let p2 = SIMD3<Float>(bo.x, j.1, bo.y), p3 = SIMD3<Float>(ao.x, i.1, ao.y)
                    quad(p0, p1, p2, p3, normals: -inN, -inNb, -inNb, -inN,
                         uv: ([s0, i.0], [s1, j.0], [s1, j.1], [s0, i.1]))
                }
            }
        }
        for o in run.openings {
            if o.through {
                if reveals { reveal(run, o, stations: st, outer: faces.outer) }
            } else {
                niche(run, o, stations: st)
            }
        }
    }

    /// The jambs, head (and sill) of a door, from the inner face through to the outer face. Its
    /// outline runs through the wall's own stations, so it shares every vertex with the hole in
    /// each face: no slit where the arc meets the jambs, no light through the wall. The jambs
    /// stay parallel to the opening's axis (as the passages beyond them are); on a curved wall a
    /// return in the outer face closes the gap to that face's wider hole.
    private mutating func reveal(_ run: WallRun, _ o: WallOpening, stations st: [Float], outer: Bool) {
        let c = run.sample(o.center)
        let axis = SIMD3<Float>(c.out.x, 0, c.out.y)
        let tangent = SIMD3<Float>(c.dir.x, 0, c.dir.y)
        let lo = WallRun.snap(o.center - o.width / 2), hi = WallRun.snap(o.center + o.width / 2)
        let span = st.filter { $0 >= lo && $0 <= hi }
        guard span.count >= 2 else { return }
        func head(_ s: Float) -> Float { min(o.head(at: s - o.center), run.height) }
        // Outline as (station, height): up the left jamb, over the head, down the right jamb, then
        // back along the sill, or round the lower half of a round opening.
        var outline: [(s: Float, y: Float)]
        if o.round {
            outline = span.map { (s: $0, y: head($0)) } + span.reversed().map { (s: $0, y: o.bottom(at: $0 - o.center)) }
        } else {
            outline = [(s: span[0], y: o.sill)] + span.map { (s: $0, y: head($0)) } + [(s: span[span.count - 1], y: o.sill)]
            if o.sill > 0 { outline += span.reversed().dropFirst().map { (s: $0, y: o.sill) } }
        }
        var pts: [(s: Float, y: Float)] = []
        for q in outline where pts.last.map({ abs($0.s - q.s) > 1e-6 || abs($0.y - q.y) > 1e-6 }) ?? true {
            pts.append(q)
        }
        let closed = pts.count > 2 && abs(pts[0].s - pts[pts.count - 1].s) < 1e-6 && abs(pts[0].y - pts[pts.count - 1].y) < 1e-6
        if closed { pts.removeLast() }
        let count = closed ? pts.count : pts.count - 1
        guard count > 0 else { return }
        // Each edge's normal in (along the run, up), facing into the opening.
        let centre2 = SIMD2<Float>(0, o.round ? o.spring : (o.sill + o.top) / 2)
        let edgeN: [SIMD2<Float>] = (0..<count).map { e in
            let u = pts[e], v = pts[(e + 1) % pts.count]
            var n2 = SIMD2<Float>(-(v.y - u.y), v.s - u.s)
            let l = simd_length(n2)
            n2 = l > 1e-9 ? n2 / l : [0, -1]
            let mid = SIMD2<Float>((u.s + v.s) / 2 - o.center, (u.y + v.y) / 2)
            return dot(n2, centre2 - mid) < 0 ? -n2 : n2
        }
        // Vertex normals: smooth round the arc (and into the jambs it is tangent to), sharp at corners.
        func blend(_ e: Int, _ f: Int?) -> SIMD3<Float> {
            var n2 = edgeN[e]
            if let f, dot(edgeN[e], edgeN[f]) > 0.77 { n2 = normalize(edgeN[e] + edgeN[f]) }
            return normalize(tangent * n2.x + SIMD3<Float>(0, n2.y, 0))
        }
        let t = run.thickness
        // Where this wall draws no outer face, another run's face meets the reveal: overlap it by 2 mm.
        let over: Float = outer ? 0 : 0.002
        func inner(_ q: (s: Float, y: Float)) -> SIMD3<Float> {
            let p = run.sample(q.s).p
            return [p.x, q.y, p.y]
        }
        func outerVertex(_ q: (s: Float, y: Float)) -> SIMD3<Float> {
            let p = run.sample(q.s).p + run.outerOffset(q.s) * t
            return [p.x, q.y, p.y]
        }
        func end(_ q: (s: Float, y: Float)) -> SIMD3<Float> {
            inner(q) + axis * (dot(outerVertex(q) - inner(q), axis) + over)
        }
        for e in 0..<count {
            let u = pts[e], v = pts[(e + 1) % pts.count]
            // A round opening cut off by the floor has no soffit along the floor (it would z-fight it).
            if u.y < 1e-4 && v.y < 1e-4 { continue }
            var prev: Int? = e - 1, next: Int? = e + 1
            if e == 0 { prev = closed ? count - 1 : nil }
            if e + 1 == count { next = closed ? 0 : nil }
            let nu = blend(e, prev), nv = blend(e, next)
            let iu = inner(u), iv = inner(v), eu = end(u), ev = end(v)
            quad(iu, iv, ev, eu, normals: nu, nv, nv, nu)
            // The return on a curved wall's outer face.
            let ou = outerVertex(u), ov = outerVertex(v)
            if outer && max(simd_distance(ou, eu), simd_distance(ov, ev)) > 1e-3 {
                quad(eu, ev, ov, ou, normal: axis)
            }
        }
    }

    /// A half-round niche with a quarter-sphere head, recessed into the wall. Built in slices at
    /// the wall's own stations, so its front edge is exactly the edge of the hole in the wall
    /// face. (The old one stood on the tangent plane at the niche's centre, which on the curved
    /// Rotunda drum left a slit along each jamb, open to the garden.)
    private mutating func niche(_ run: WallRun, _ o: WallOpening, stations st: [Float]) {
        let c = run.sample(o.center)
        let out = SIMD3<Float>(c.out.x, 0, c.out.y)
        let tan = SIMD3<Float>(c.dir.x, 0, c.dir.y)
        let up = SIMD3<Float>(0, 1, 0)
        let r = o.width / 2
        let lo = WallRun.snap(o.center - o.width / 2), hi = WallRun.snap(o.center + o.width / 2)
        let span = st.filter { $0 >= lo && $0 <= hi }
        guard span.count >= 2 else { return }
        let m = max(2, Tessellation.arch / 2)
        // The point `z` deep behind the wall face at station s, at height y.
        func P(_ s: Float, _ z: Float, _ y: Float) -> SIMD3<Float> {
            let p = run.sample(s).p
            return SIMD3<Float>(p.x, y, p.y) + out * z
        }
        // Normal facing into the niche, from (lateral d, depth z, height above the springing).
        func N(_ d: Float, _ z: Float, _ y: Float) -> SIMD3<Float> {
            let v = tan * d + out * z + up * y
            return simd_length(v) > 1e-6 ? -normalize(v) : -out
        }
        for (s0, s1) in zip(span, span.dropFirst()) where s1 - s0 > 1e-6 {
            let d0 = (s0 < o.center ? -1 : 1) * o.lateral(s0 - o.center)
            let d1 = (s1 < o.center ? -1 : 1) * o.lateral(s1 - o.center)
            let z0 = sqrt(max(0, r * r - d0 * d0)), z1 = sqrt(max(0, r * r - d1 * d1))
            // Half-cylinder back and the sill.
            quad(P(s0, z0, o.sill), P(s1, z1, o.sill), P(s1, z1, o.spring), P(s0, z0, o.spring),
                 normals: N(d0, z0, 0), N(d1, z1, 0), N(d1, z1, 0), N(d0, z0, 0))
            quad(P(s0, 0, o.sill), P(s1, 0, o.sill), P(s1, z1, o.sill), P(s0, z0, o.sill), normal: up)
            // Quarter-sphere head, in the same slices: from the arch in the wall face back and
            // down to the springing, where it meets the cylinder.
            func H(_ s: Float, _ d: Float, _ rho: Float, _ f: Float) -> (p: SIMD3<Float>, n: SIMD3<Float>) {
                let z = rho * sin(f), y = rho * cos(f)
                return (P(s, z, o.spring + y), N(d, z, y))
            }
            for j in 0..<m {
                let f0 = Float.pi / 2 * Float(j) / Float(m), f1 = Float.pi / 2 * Float(j + 1) / Float(m)
                let a = H(s0, d0, z0, f0), b = H(s1, d1, z1, f0), e = H(s1, d1, z1, f1), g = H(s0, d0, z0, f1)
                quad(a.p, b.p, e.p, g.p, normals: a.n, b.n, e.n, g.n)
            }
        }
    }

    /// A moulding band (cornice or skirting) running along the room side of a wall,
    /// interrupted where openings cut through its height range (its ends closed there).
    mutating func band(_ run: WallRun, from y0: Float, to y1: Float, depth: Float) {
        let st = run.stations(step: 0.25)
        func cut(_ sm: Float) -> Bool {
            run.openings.contains(where: { abs(sm - $0.center) < $0.width / 2 && $0.sill < y1 && $0.top > y0 })
        }
        // The band's end at s, facing along the run (f = 1) or back (f = -1), towards the opening.
        func cap(_ s: Float, facing f: Float) {
            let a = run.sample(s)
            let fa = a.p - a.out * depth
            quad([a.p.x, y0, a.p.y], [fa.x, y0, fa.y], [fa.x, y1, fa.y], [a.p.x, y1, a.p.y],
                 normal: SIMD3<Float>(a.dir.x, 0, a.dir.y) * f)
        }
        var wasCut: Bool?
        for (s0, s1) in zip(st, st.dropFirst()) where s1 - s0 > 1e-6 {
            let sm = (s0 + s1) / 2
            let isCut = cut(sm)
            if let wasCut, wasCut != isCut { cap(s0, facing: isCut ? 1 : -1) }
            wasCut = isCut
            if isCut { continue }
            let a = run.sample(s0), b = run.sample(s1)
            let sa = run.smoothOut(s0, on: sm), sb = run.smoothOut(s1, on: sm)
            let na = SIMD3<Float>(-sa.x, 0, -sa.y), nb = SIMD3<Float>(-sb.x, 0, -sb.y)
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
    /// `arc` gives the angles round the vault (0…π) when it must meet other surfaces vertex for
    /// vertex (coffers, lunettes); by default they are even.
    mutating func barrelVault(x0: Float, x1: Float, radius r: Float, spring: Float,
                              holes: [(x0: Float, x1: Float, halfWidth: Float)], tile: Float, arc: [Float]? = nil) {
        var xs: [Float] = []
        let n = Int(ceil(abs(x1 - x0) / 0.6))
        for i in 0...n { xs.append(x0 + (x1 - x0) * Float(i) / Float(n)) }
        for h in holes { xs.append(h.x0); xs.append(h.x1) }
        xs = Array(Set(xs)).sorted()
        let even = Tessellation.segments(48)
        var angles: [Float] = arc ?? (0...even).map { Float.pi * Float($0) / Float(even) }
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
        let n = Tessellation.arch
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
    /// `arc`: the angles (0…π) of the vault it closes, so their edges share vertices.
    mutating func lunette(x: Float, radius r: Float, spring: Float, facing nx: Float, arc: [Float]? = nil) {
        let n = Tessellation.segments(48)
        let angles = arc ?? (0...n).map { Float.pi * Float($0) / Float(n) }
        let c = SIMD3<Float>(x, spring, 0)
        for (a0, a1) in zip(angles, angles.dropFirst()) {
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

    /// A solid curved bench along a centreline polyline, w wide and h tall, as one piece: its sides
    /// are offset with mitres at the vertices (a box per segment left wedge-shaped gaps along the
    /// outer edge), shade smoothly along the curve and have chamfered top edges; the ends are cut
    /// square at the polyline's ends (as on look target 03), so the plan dimensions are unchanged.
    mutating func curvedBench(_ pts: [SIMD2<Float>], width w: Float, height h: Float) {
        guard pts.count >= 2 else { return }
        let hw = w / 2
        let ch = min(0.03, hw / 4, h / 4)          // chamfer on the top edges
        // Left normal of each segment; at each vertex the smooth normal and the mitred offset.
        var segN: [SIMD2<Float>] = []
        for i in 0..<(pts.count - 1) {
            let d = pts[i + 1] - pts[i], l = simd_length(d)
            segN.append(l > 1e-6 ? SIMD2<Float>(-d.y, d.x) / l : (segN.last ?? [0, 1]))
        }
        var nrm: [SIMD2<Float>] = [], off: [SIMD2<Float>] = []
        for i in pts.indices {
            let n0 = segN[max(0, i - 1)], n1 = segN[min(segN.count - 1, i)]
            let m = simd_length(n0 + n1) > 1e-4 ? normalize(n0 + n1) : n1
            nrm.append(m)
            off.append(m / max(0.3, dot(m, n1)))
        }
        // The section, from the foot of the left side over the top to the foot of the right side
        // (lateral, height), with each edge's normal (lateral, up).
        let section: [SIMD2<Float>] = [[hw, 0], [hw, h - ch], [hw - ch, h], [-(hw - ch), h], [-hw, h - ch], [-hw, 0]]
        let edgeN: [SIMD2<Float>] = [[1, 0], normalize(SIMD2<Float>(1, 1)), [0, 1], normalize(SIMD2<Float>(-1, 1)), [-1, 0]]
        func X(_ i: Int, _ q: SIMD2<Float>) -> SIMD3<Float> { let p = pts[i] + off[i] * q.x; return [p.x, q.y, p.y] }
        func N(_ i: Int, _ n: SIMD2<Float>) -> SIMD3<Float> { normalize(SIMD3<Float>(nrm[i].x * n.x, n.y, nrm[i].y * n.x)) }
        for i in 0..<(pts.count - 1) {
            for k in 0..<(section.count - 1) {
                quad(X(i, section[k]), X(i + 1, section[k]), X(i + 1, section[k + 1]), X(i, section[k + 1]),
                     normals: N(i, edgeN[k]), N(i + 1, edgeN[k]), N(i + 1, edgeN[k]), N(i, edgeN[k]))
            }
        }
        // Square ends: the section itself, facing out along the first and last segments.
        func end(_ i: Int, outward b: SIMD2<Float>) {
            let c = X(i, [0, h / 2]), bn = SIMD3<Float>(b.x, 0, b.y)
            for k in 0..<section.count {
                tri(c, X(i, section[k]), X(i, section[(k + 1) % section.count]), bn, bn, bn)
            }
        }
        // (A segment's direction is (n.y, −n.x) for its left normal n.)
        end(0, outward: [-segN[0].y, segN[0].x])
        end(pts.count - 1, outward: [segN[segN.count - 1].y, -segN[segN.count - 1].x])
    }

    /// Vertical flat shape (door leaf) filling an opening outline, in the plane through
    /// `origin` spanned by `tangent` and up, facing `normal`.
    mutating func doorLeaf(origin o: SIMD3<Float>, tangent t: SIMD3<Float>, normal n: SIMD3<Float>,
                           width w: Float, spring: Float, arched: Bool) {
        let r = w / 2
        var outline: [SIMD2<Float>] = [[-r, 0], [r, 0], [r, spring]]
        if arched {
            let n = Tessellation.arch
            for i in 1..<n {
                let a = Float.pi * Float(i) / Float(n)
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
        // Outward normal of each profile edge in (r, h). Where two edges turn by less than 35° the
        // normals are blended at their shared point, so curved profiles (the Sphere, the vases)
        // shade smoothly instead of in flat bands; lips and corners stay sharp.
        let edgeN: [SIMD2<Float>] = (0..<(profile.count - 1)).map { k in
            let d = profile[k + 1] - profile[k], l = simd_length(d)
            return l > 1e-9 ? SIMD2<Float>(d.y, -d.x) / l : [0, 1]
        }
        func blend(_ k: Int, _ j: Int) -> SIMD2<Float> {
            guard j >= 0, j < edgeN.count, dot(edgeN[k], edgeN[j]) > 0.82 else { return edgeN[k] }
            return normalize(edgeN[k] + edgeN[j])
        }
        for k in 0..<(profile.count - 1) {
            let a = profile[k], b = profile[k + 1]
            let na = blend(k, k - 1), nb = blend(k, k + 1)
            for i in 0..<segments {
                let t0 = 2 * Float.pi * Float(i) / Float(segments), t1 = 2 * Float.pi * Float(i + 1) / Float(segments)
                func p(_ q: SIMD2<Float>, _ t: Float) -> SIMD3<Float> { c + [q.x * cos(t), q.y, q.x * sin(t)] }
                func n(_ n2: SIMD2<Float>, _ t: Float) -> SIMD3<Float> { [n2.x * cos(t), n2.y, n2.x * sin(t)] }
                let v0 = Float(k) / Float(profile.count - 1), v1 = Float(k + 1) / Float(profile.count - 1)
                let u0 = Float(i) / Float(segments), u1 = Float(i + 1) / Float(segments)
                quad(p(a, t0), p(a, t1), p(b, t1), p(b, t0), normals: n(na, t0), n(na, t1), n(nb, t1), n(nb, t0),
                     uv: ([u0, v0], [u1, v0], [u1, v1], [u0, v1]))
                if inside {
                    quad(p(a, t0), p(a, t1), p(b, t1), p(b, t0), normals: -n(na, t0), -n(na, t1), -n(nb, t1), -n(nb, t0),
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
