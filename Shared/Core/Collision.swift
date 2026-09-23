import simd

/// Walls, benches and furniture flattened to 2D segments on the floor plan, each standing
/// between two heights. The visitor is a vertical capsule (a circle on plan, from their feet
/// to 1.8 m above them) that slides along any segment their body overlaps.
struct CollisionWorld {
    struct Segment {
        var a: SIMD2<Float>
        var b: SIMD2<Float>
        /// Extra thickness (half-width) of the obstacle, e.g. for benches.
        var radius: Float = 0
        /// Whether the segment hides things behind it (walls do, benches and glass don't).
        var occludes: Bool = true
        /// Vertical extent of the obstacle.
        var y0: Float = -0.5
        var y1: Float = 30
        /// Optional switch (e.g. a door that is sometimes open); nil = always solid.
        var active: (() -> Bool)? = nil
    }

    var segments: [Segment] = []
    static let bodyHeight: Float = 1.8

    mutating func add(_ a: SIMD2<Float>, _ b: SIMD2<Float>, radius: Float = 0, occludes: Bool = true,
                      y0: Float = -0.5, y1: Float = 30, active: (() -> Bool)? = nil) {
        segments.append(Segment(a: a, b: b, radius: radius, occludes: occludes, y0: y0, y1: y1, active: active))
    }

    mutating func addPolyline(_ pts: [SIMD2<Float>], radius: Float = 0, occludes: Bool = true,
                              y0: Float = -0.5, y1: Float = 30, active: (() -> Bool)? = nil) {
        for (a, b) in zip(pts, pts.dropFirst()) {
            add(a, b, radius: radius, occludes: occludes, y0: y0, y1: y1, active: active)
        }
    }

    /// Axis-aligned rectangle outline.
    mutating func addRect(x0: Float, x1: Float, z0: Float, z1: Float, occludes: Bool = true,
                          y0: Float = -0.5, y1: Float = 30) {
        let p = [SIMD2<Float>(x0, z0), [x1, z0], [x1, z1], [x0, z1], [x0, z0]]
        addPolyline(p, occludes: occludes, y0: y0, y1: y1)
    }

    /// Circle outline (e.g. a column), optionally leaving an arc open.
    mutating func addCircle(_ c: SIMD2<Float>, r: Float, segments n: Int = 24, occludes: Bool = true,
                            y0: Float = -0.5, y1: Float = 30, gap: (from: Float, to: Float)? = nil,
                            active: (() -> Bool)? = nil) {
        for i in 0..<n {
            let t0 = 2 * Float.pi * Float(i) / Float(n), t1 = 2 * Float.pi * Float(i + 1) / Float(n)
            if let g = gap {
                let tm = (t0 + t1) / 2
                if Self.angleWithin(tm, g.from, g.to) { continue }
            }
            add(c + [r * cos(t0), r * sin(t0)], c + [r * cos(t1), r * sin(t1)], occludes: occludes,
                y0: y0, y1: y1, active: active)
        }
    }

    static func angleWithin(_ t: Float, _ a: Float, _ b: Float) -> Bool {
        func norm(_ x: Float) -> Float { var y = x.truncatingRemainder(dividingBy: 2 * .pi); if y < 0 { y += 2 * .pi }; return y }
        let tt = norm(t - a), span = norm(b - a)
        return tt <= span
    }

    /// Adds the inner face of a wall run, leaving gaps at through-openings; niches are solid.
    mutating func add(run: WallRun, base: Float = 0) {
        let st = run.stations(step: 0.25)
        let y0 = base - 0.5, y1 = base + run.height
        for (s0, s1) in zip(st, st.dropFirst()) where s1 - s0 > 1e-4 {
            let sm = (s0 + s1) / 2
            if run.openings.contains(where: { o in
                let half = o.round ? sqrt(max(0, pow(o.width / 2, 2) - pow(o.spring - 0.3, 2))) : o.width / 2
                return o.through && o.sill < 0.3 && abs(sm - o.center) < half
            }) {
                continue
            }
            add(run.sample(s0).p, run.sample(s1).p, y0: y0, y1: y1)
        }
        for o in run.openings where o.through {
            // Round openings: the walkable width is where the sill is low.
            let half = o.round ? sqrt(max(0, pow(o.width / 2, 2) - pow(o.spring - 0.3, 2))) : o.width / 2
            for side: Float in [-1, 1] {
                let s = run.sample(o.center + side * half)
                add(s.p, s.p + s.out * run.thickness, y0: y0, y1: y1)
            }
        }
    }

    private static func closest(_ p: SIMD2<Float>, _ a: SIMD2<Float>, _ b: SIMD2<Float>) -> SIMD2<Float> {
        let ab = b - a
        let l2 = simd_length_squared(ab)
        if l2 < 1e-10 { return a }
        let t = simd_clamp(dot(p - a, ab) / l2, 0, 1)
        return a + ab * t
    }

    private func blocks(_ s: Segment, feet: Float) -> Bool {
        guard s.y1 > feet + 0.3, s.y0 < feet + Self.bodyHeight else { return false }
        return s.active?() ?? true
    }

    /// Moves a circle of radius r from p by delta, sliding along obstacles at the body's height.
    func move(from p: SIMD2<Float>, by delta: SIMD2<Float>, radius r: Float, feet: Float) -> SIMD2<Float> {
        let steps = max(1, Int(ceil(simd_length(delta) / 0.08)))
        let local = segments.filter { s in
            blocks(s, feet: feet) && Self.segmentNear(s, p, reach: simd_length(delta) + r + s.radius + 0.5)
        }
        var q = p
        for _ in 0..<steps {
            q += delta / Float(steps)
            for _ in 0..<4 {
                var pushed = false
                for s in local {
                    let c = CollisionWorld.closest(q, s.a, s.b)
                    let d = q - c
                    let dist = simd_length(d)
                    let minDist = r + s.radius
                    if dist < minDist {
                        let n = dist > 1e-5 ? d / dist : SIMD2<Float>(-(s.b - s.a).y, (s.b - s.a).x).normalizedOrZero
                        q = c + n * minDist
                        pushed = true
                    }
                }
                if !pushed { break }
            }
        }
        return q
    }

    private static func segmentNear(_ s: Segment, _ p: SIMD2<Float>, reach: Float) -> Bool {
        simd_distance(closest(p, s.a, s.b), p) < reach
    }

    /// Distance along a 2D ray to the first occluding segment at eye height, if any.
    func firstHit(origin o: SIMD2<Float>, direction d: SIMD2<Float>, maxDistance: Float, eyeY: Float) -> Float? {
        var best: Float?
        for s in segments where s.occludes && s.y0 < eyeY && s.y1 > eyeY && (s.active?() ?? true) {
            let e = s.b - s.a
            let denom = d.x * e.y - d.y * e.x
            if abs(denom) < 1e-8 { continue }
            let w = s.a - o
            let t = (w.x * e.y - w.y * e.x) / denom
            let u = (w.x * d.y - w.y * d.x) / denom
            if t > 0.05, t < maxDistance, u >= 0, u <= 1 {
                if best == nil || t < best! { best = t }
            }
        }
        return best
    }
}

/// Where the floor is. Every walkable surface is a region on plan with a height (constant
/// or sloping, like a stair). The visitor stands on the highest floor within a step of their
/// feet; a move onto no floor at all (a drop, a stairwell edge, the sky) is refused.
struct FloorWorld {
    enum Shape {
        case rect(x0: Float, x1: Float, z0: Float, z1: Float)
        case ellipse(c: SIMD2<Float>, a: Float, b: Float)
        case polygon([SIMD2<Float>])
        case everywhere

        func contains(_ p: SIMD2<Float>) -> Bool {
            switch self {
            case let .rect(x0, x1, z0, z1):
                return p.x >= min(x0, x1) && p.x <= max(x0, x1) && p.y >= min(z0, z1) && p.y <= max(z0, z1)
            case let .ellipse(c, a, b):
                let d = p - c
                return (d.x * d.x) / (a * a) + (d.y * d.y) / (b * b) <= 1
            case let .polygon(pts):
                var inside = false
                var j = pts.count - 1
                for i in 0..<pts.count {
                    let a = pts[i], b = pts[j]
                    if (a.y > p.y) != (b.y > p.y), p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x { inside.toggle() }
                    j = i
                }
                return inside
            case .everywhere:
                return true
            }
        }
    }

    struct Region {
        var name: String
        var shape: Shape
        var holes: [Shape] = []
        var height: (SIMD2<Float>) -> Float
        var active: (() -> Bool)? = nil
    }

    var regions: [Region] = []
    static let step: Float = 0.45

    mutating func add(_ name: String, _ shape: Shape, height h: Float, holes: [Shape] = [], active: (() -> Bool)? = nil) {
        regions.append(Region(name: name, shape: shape, holes: holes, height: { _ in h }, active: active))
    }

    mutating func add(_ name: String, _ shape: Shape, holes: [Shape] = [], active: (() -> Bool)? = nil,
                      height: @escaping (SIMD2<Float>) -> Float) {
        regions.append(Region(name: name, shape: shape, holes: holes, height: height, active: active))
    }

    /// The floor under p for someone whose feet are at `feet`, or nil if there is none in reach.
    func floor(at p: SIMD2<Float>, feet: Float) -> Float? {
        var best: Float?
        for r in regions where (r.active?() ?? true) && r.shape.contains(p) && !r.holes.contains(where: { $0.contains(p) }) {
            let h = r.height(p)
            guard h <= feet + Self.step, h >= feet - Self.step else { continue }
            if best == nil || h > best! { best = h }
        }
        return best
    }
}

extension SIMD2 where Scalar == Float {
    var normalizedOrZero: SIMD2<Float> {
        let l = simd_length(self)
        return l > 1e-8 ? self / l : .zero
    }
}
