import CoreGraphics
import Foundation
import RealityKit
import simd

/// The lily pond that lifts, the long stair and the Reserve (board: SalonReserve).
/// Tap the golden lily: a line of warm light runs round the rim. Tap it again: the whole pond
/// rises 2.6 m on four bronze posts, and a 36-step stair drops away east under the oval floor to
/// the Reserve, a 60 m brick-vaulted cellar beneath the Salon, where racks glide out and a
/// painting can be sent to the viewing easel.
enum PondPlan {
    static let centre = SIMD2<Float>(-86.5, 0)
    static let lift: Float = 2.6
    static let liftDuration: Float = 5
    /// The opening in the oval floor: the part of the pond's footprint over the 2.4 m shaft.
    static let openingRect = (x0: Float(-90.4), x1: Float(-82.9), z0: Float(-1.2), z1: Float(1.2))
    static var opening: FloorWorld.Shape { .rect(x0: openingRect.x0, x1: openingRect.x1, z0: openingRect.z0, z1: openingRect.z1) }
    static let posts: [SIMD2<Float>] = [[-83.5, -1.35], [-83.5, 1.35], [-89.5, -1.35], [-89.5, 1.35]]
    static let goldenLily = SIMD2<Float>(-86.9, 0.5)
    static let basin = (a: Float(4.2), b: Float(2.3), rim: Float(0.3), height: Float(0.45), water: Float(0.38))
    // The stair: 36 risers of 0.1611 m, goings of 0.3389 m, from x −90.0 down to −5.8 m at −78.14.
    static let stairTop: Float = -90.0
    static let going: Float = 0.3389
    static let steps = 36
    static let stairBottom: Float = -78.14
    static let shaftHalfWidth: Float = 1.2
    static let shaftEnd: Float = -74.0

    static func stairHeight(_ x: Float) -> Float {
        -ReservePlan.depth * simd_clamp((x - stairTop) / (stairBottom - stairTop), 0, 1)
    }
}

enum ReservePlan {
    static let depth: Float = 5.8                 // floor at h −5.8
    static let x0: Float = -74, x1: Float = -14, halfWidth: Float = 7
    static let clear: Float = 5.0                 // floor to the Salon slab's soffit (h −0.8)
    static let aisle: Float = 1.4
    static let columnXs: [Float] = [-20, -26, -32, -38, -44, -50, -56, -62, -68]
    static let columnY: Float = 1.8
    static let springing: Float = 1.9
    static let rackPitch: Float = 1.8
    static let rack = (length: Float(4.2), height: Float(3.0), thickness: Float(0.26), glide: Float(2.4))
    /// Lettered racks (north side) and their works: (letter, x, [(id, w, h, centre y at home)]).
    static let lettered: [(letter: String, x: Float, works: [(id: String, w: Float, h: Float, y: Float)])] = [
        ("A", -71.6, [("caillebotte-pont-europe", 1.80, 1.25, -5.050), ("caillebotte-young-man-window", 0.81, 1.16, -3.445)]),
        ("B", -69.8, [("degas-place-de-la-concorde", 1.18, 0.78, -5.360), ("degas-orchestra-opera", 0.46, 0.57, -4.240)]),
        ("C", -66.2, [("renoir-cirque-fernando", 0.99, 1.31, -5.455), ("renoir-two-sisters", 0.81, 1.01, -4.255),
                      ("renoir-the-swing", 0.73, 0.92, -3.185)]),
        ("D", -64.4, [("pissarro-red-roofs", 0.66, 0.55, -5.620), ("pissarro-avenue-opera-snow", 0.81, 0.65, -4.585)]),
        ("E", -62.6, [("sisley-bridge-villeneuve", 0.65, 0.50, -5.625), ("sisley-snow-louveciennes", 0.51, 0.61, -4.745)]),
        ("F", -60.8, []),
    ]
    static let easel = SIMD2<Float>(-15.0, 0)
    static let chests: [(x0: Float, x1: Float, id: String, w: Float, h: Float)] = [
        (-17.10, -15.20, "degas-the-star", 0.42, 0.58), (-19.20, -17.40, "degas-the-tub", 0.83, 0.60),
    ]
}

/// The pond's moving parts.
@MainActor
final class PondLift {
    enum State { case down, armed, rising, up, lowering }
    var state: State = .down
    var height: Float = 0
    var armedTimer: Float = 0
    var rimReveal: Float = 0
    let pond = Entity()
    var posts: [Entity] = []
    var rimSegments: [Entity] = []
    let halo = ModelEntity()
}

/// A rack in the Reserve and whether it is pulled out.
@MainActor
final class ReserveRack {
    let entity = Entity()
    let north: Bool
    let x: Float
    var offset: Float = 0          // 0 home … glide
    var target: Float = 0
    init(north: Bool, x: Float) { self.north = north; self.x = x }
    var homeCentreY: Float { north ? -4.1 : 4.1 }
    var direction: Float { north ? 1 : -1 }
}

extension MuseumScene {
    /// Floor holes at level 0 (the pond's opening; the Atrium's car shaft).
    func groundHoles() -> [FloorWorld.Shape] {
        [PondPlan.opening, .ellipse(c: ElanPlan.centre, a: ElanPlan.car.radius + 0.05, b: ElanPlan.car.radius + 0.05)]
    }

    // MARK: - The pond

    func buildPond() {
        typealias P = PondPlan
        let lift = PondLift()
        pondLift = lift
        let c = P.centre
        let B = P.basin
        lift.pond.name = "Lily pond"
        building.addChild(lift.pond)

        // Basin, water and lilies, as one body (built at floor level; the entity rises).
        let outer = Poly.ellipse(c, a: B.a, b: B.b, from: 0, to: 2 * .pi, segments: 96)
        let inner = Poly.ellipse(c, a: B.a - B.rim, b: B.b - B.rim, from: 0, to: 2 * .pi, segments: 96)
        var basin = MeshBuilder()
        basin.ribbon(outer, y0: 0, y1: B.height, facing: { normalize(($0 - c) / [B.a * B.a, B.b * B.b]) })
        basin.ribbon(inner, y0: 0.2, y1: B.height, facing: { -normalize(($0 - c) / [B.a * B.a, B.b * B.b]) })
        for (a, b) in zip(zip(outer, inner), zip(outer.dropFirst(), inner.dropFirst())) {
            basin.quad([a.0.x, B.height, a.0.y], [b.0.x, B.height, b.0.y], [b.1.x, B.height, b.1.y],
                       [a.1.x, B.height, a.1.y], normal: [0, 1, 0])
        }
        // Underside, seen once the pond is up.
        basin.ellipseDisc(center: [c.x, 0.001, c.y], a: B.a, b: B.b, up: false)
        add(basin, Mat.matte(0xEAE0CE, roughness: 0.7), name: "Pond basin", to: lift.pond)
        var water = MeshBuilder()
        water.ellipseDisc(center: [c.x, B.water, c.y], a: B.a - B.rim, b: B.b - B.rim, up: true)
        var waterMat = PhysicallyBasedMaterial()
        waterMat.baseColor = .init(tint: PlatformColor(hex: 0x3F5E5A))
        waterMat.roughness = .init(floatLiteral: 0.08)
        waterMat.metallic = .init(floatLiteral: 0.3)
        add(water, waterMat, name: "Water", to: lift.pond)
        var pads = MeshBuilder(), pink = MeshBuilder(), gold = MeshBuilder()
        for l in Plan.Oval.lilies {
            let n = 24
            let ctr = SIMD3<Float>(l.x, B.water + 0.005, l.z)
            for i in 0..<n {
                let a0 = 0.35 + (2 * Float.pi - 0.7) * Float(i) / Float(n)
                let a1 = 0.35 + (2 * Float.pi - 0.7) * Float(i + 1) / Float(n)
                pads.tri(ctr, ctr + [l.r * cos(a0), 0, l.r * sin(a0)], ctr + [l.r * cos(a1), 0, l.r * sin(a1)],
                         [0, 1, 0], [0, 1, 0], [0, 1, 0])
            }
            let isGold = abs(l.x - P.goldenLily.x) < 0.01 && abs(l.z - P.goldenLily.y) < 0.01
            if l.flower || isGold {
                for k in 0..<8 {
                    let a = Float(k) * .pi / 4
                    let r: Float = isGold ? 0.2 : l.r * 0.4
                    let petal = SIMD3<Float>(l.x + cos(a) * r * 0.5, B.water + 0.06, l.z + sin(a) * r * 0.5)
                    if isGold {
                        gold.box(center: petal, size: [r, 0.07, r * 0.45], yaw: -a)
                    } else {
                        pink.box(center: petal, size: [r, 0.07, r * 0.45], yaw: -a)
                    }
                }
            }
        }
        add(pads, Mat.matte(0x5E8A4E, roughness: 0.5), name: "Lily pads", to: lift.pond)
        add(pink, Mat.matte(0xF2C9D2, roughness: 0.6), name: "Lily flowers", to: lift.pond)
        add(gold, Mat.metal(0xE0B347, roughness: 0.25), name: "The golden lily", to: lift.pond)

        // The halo round the golden lily and the line of warm light that runs round the rim.
        var ring = MeshBuilder()
        let hl = SIMD3<Float>(P.goldenLily.x, B.water + 0.01, P.goldenLily.y)
        for i in 0..<40 {
            let a0 = 2 * Float.pi * Float(i) / 40, a1 = 2 * Float.pi * Float(i + 1) / 40
            ring.quad(hl + [0.36 * cos(a0), 0, 0.36 * sin(a0)], hl + [0.36 * cos(a1), 0, 0.36 * sin(a1)],
                      hl + [0.44 * cos(a1), 0, 0.44 * sin(a1)], hl + [0.44 * cos(a0), 0, 0.44 * sin(a0)], normal: [0, 1, 0])
        }
        var haloMat = UnlitMaterial(color: PlatformColor(hex: 0xE9B75A))
        haloMat.blending = .transparent(opacity: 0.8)
        lift.halo.model = ModelComponent(mesh: ring.mesh(name: "halo"), materials: [haloMat])
        lift.halo.isEnabled = false
        lift.pond.addChild(lift.halo)
        // Rim light: 32 segments that light one after another, starting nearest the lily.
        let rimPts = Poly.ellipse(c, a: B.a + 0.01, b: B.b + 0.01, from: .pi * 0.9, to: .pi * 0.9 + 2 * .pi, segments: 128)
        for k in 0..<32 {
            var seg = MeshBuilder()
            for j in (k * 4)..<(k * 4 + 4) {
                let a = rimPts[j], b = rimPts[j + 1]
                let na = normalize((a - c) / [B.a * B.a, B.b * B.b]), nb = normalize((b - c) / [B.a * B.a, B.b * B.b])
                seg.quad([a.x, B.height - 0.05, a.y], [b.x, B.height - 0.05, b.y], [b.x, B.height + 0.005, b.y],
                         [a.x, B.height + 0.005, a.y], normals: [na.x, 0, na.y], [nb.x, 0, nb.y], [nb.x, 0, nb.y], [na.x, 0, na.y])
                seg.quad([a.x, B.height + 0.006, a.y], [b.x, B.height + 0.006, b.y],
                         [b.x - nb.x * 0.05, B.height + 0.006, b.y - nb.y * 0.05],
                         [a.x - na.x * 0.05, B.height + 0.006, a.y - na.y * 0.05], normal: [0, 1, 0])
            }
            let e = ModelEntity(mesh: seg.mesh(name: "rim light"), materials: [Mat.glow(0xFFE3A0)])
            e.isEnabled = false
            lift.pond.addChild(e)
            lift.rimSegments.append(e)
        }

        // Four bronze posts that telescope up with the pond.
        for p in P.posts {
            let post = ModelEntity(mesh: .generateCylinder(height: 1, radius: 0.075), materials: [Mat.metal(0x8A6A3E, roughness: 0.4)])
            post.position = [p.x, 0, p.y]
            post.scale = [1, 0.001, 1]
            post.isEnabled = false
            building.addChild(post)
            lift.posts.append(post)
            collision.addCircle(p, r: 0.12, segments: 8, occludes: false, y0: -0.5, y1: 3, active: { [weak lift] in (lift?.height ?? 0) > 0.05 })
        }
        // The basin blocks the way until it has risen clear of your head.
        collision.addPolyline(outer, radius: 0.02, occludes: false, y0: -0.5, y1: 0.45,
                              active: { [weak lift] in (lift?.height ?? 0) < 1.9 })

        // The golden lily: tap to wake it, tap again to lift; tap once the pond is up to lower it.
        targets.append(PickTarget(artworkID: nil, hit: { [weak lift] o, d in
            let h = lift?.height ?? 0
            return Self.raySphere(o, d, centre: [P.goldenLily.x, B.water + 0.1 + h, P.goldenLily.y], radius: 0.45)
        }, action: { [weak self] in self?.tapGoldenLily() }))

        // A warm glow from below and a wash down the stair.
        addLight(spot(at: [-89.8, -0.6, 0], looking: [-80, -5.8, 0], colour: 0xFFD99A, intensity: 30_000, inner: 30, outer: 60, radius: 18))

        buildStair()

        updaters.append { [weak self] dt, eye in self?.updatePond(dt: dt, eye: eye) }
    }

    func tapGoldenLily() {
        guard let lift = pondLift else { return }
        switch lift.state {
        case .down:
            lift.state = .armed
            lift.armedTimer = 5
            lift.rimReveal = 0
            lift.halo.isEnabled = true
            message = "A line of light runs round the rim. Tap the golden lily again to lift the pond."
        case .armed:
            lift.state = .rising
            message = "The pond rises on four bronze posts…"
        case .up:
            if visitorFeet > -0.05 && !PondPlan.opening.contains(visitorPlan) {
                lift.state = .lowering
                message = nil
            }
        case .rising, .lowering:
            break
        }
    }

    private func updatePond(dt: Float, eye: SIMD3<Float>) {
        guard let lift = pondLift else { return }
        let P = PondPlan.self
        switch lift.state {
        case .armed:
            lift.rimReveal = min(1, lift.rimReveal + dt)
            lift.armedTimer -= dt
            if lift.armedTimer <= 0 {
                lift.state = .down
                lift.halo.isEnabled = false
                message = nil
            }
        case .rising:
            lift.height = min(P.lift, lift.height + dt * P.lift / P.liftDuration)
            if lift.height >= P.lift {
                lift.state = .up
                message = "Beneath the pond, a long stair leads down to the Reserve."
            }
        case .lowering:
            lift.height = max(0, lift.height - dt * P.lift / P.liftDuration)
            if lift.height <= 0 {
                lift.state = .down
                lift.halo.isEnabled = false
                lift.rimReveal = 0
            }
        case .up:
            if visitorFeet < -3, message?.hasPrefix("Beneath") == true { message = nil }
            // It settles again once you have left the oval (never while you are below).
            let away = simd_distance(SIMD2<Float>(eye.x, eye.z), P.centre) > 16
            if visitorFeet > -0.05 && away {
                lift.state = .lowering
                if message?.hasPrefix("Beneath") == true { message = nil }
            }
        case .down:
            lift.rimReveal = 0
        }
        // Ease the motion.
        let t = lift.height / P.lift
        let eased = t * t * (3 - 2 * t) * P.lift
        lift.pond.position.y = eased
        for post in lift.posts {
            post.isEnabled = eased > 0.02
            post.scale = [1, max(0.001, eased), 1]
            post.position.y = eased / 2
        }
        let lit = lift.state == .down ? 0 : Int(lift.rimReveal * 32)
        for (k, seg) in lift.rimSegments.enumerated() { seg.isEnabled = k < lit }
    }

    // MARK: - The long stair

    private func buildStair() {
        typealias P = PondPlan
        let hw = P.shaftHalfWidth
        let depth = ReservePlan.depth
        let riser = depth / Float(P.steps)
        var steps = MeshBuilder()
        // Top landing, then 36 treads, then the bottom landing (built as solid steps).
        steps.floorRect(x0: P.openingRect.x0, x1: P.stairTop, z0: -hw, z1: hw, y: 0.0, up: true, tile: 1)
        for i in 0..<P.steps {
            let xa = P.stairTop + P.going * Float(i), xb = xa + P.going
            let top = -riser * Float(i + 1)
            steps.box(min: [xa, top - 0.25, -hw], max: [xb, top, hw])
            steps.quad([xa, top, -hw], [xa, top, hw], [xa, top + riser, hw], [xa, top + riser, -hw], normal: [-1, 0, 0])
        }
        steps.floorRect(x0: P.stairBottom, x1: P.shaftEnd + 0.3, z0: -hw, z1: hw, y: -depth, up: true, tile: 1)
        add(steps, Mat.matte(0xCFC6B6, roughness: 0.7), name: "The long stair")

        // Shaft walls, end wall, ceiling (the underside of the oval floor), and the slab edge.
        var shaft = MeshBuilder()
        for side: Float in [-1, 1] {
            let z = side * hw
            shaft.quad([P.openingRect.x0, -depth - 0.3, z], [P.shaftEnd, -depth - 0.3, z], [P.shaftEnd, 0, z], [P.openingRect.x0, 0, z],
                       normal: [0, 0, -side])
        }
        shaft.quad([P.openingRect.x0, -depth, -hw], [P.openingRect.x0, -depth, hw], [P.openingRect.x0, 0, hw],
                   [P.openingRect.x0, 0, -hw], normal: [1, 0, 0])
        shaft.floorRect(x0: P.openingRect.x1, x1: P.shaftEnd, z0: -hw, z1: hw, y: -0.4, up: false)
        shaft.quad([P.openingRect.x1, -0.4, -hw], [P.openingRect.x1, -0.4, hw], [P.openingRect.x1, 0, hw],
                   [P.openingRect.x1, 0, -hw], normal: [-1, 0, 0])
        add(shaft, Mat.matte(0xD9CDBA, roughness: 0.85), name: "Stair shaft")
        // Brass handrails at 0.9 m above the nosings, on both walls.
        var rails = MeshBuilder()
        for side: Float in [-1, 1] {
            let z = side * (hw - 0.06)
            // The rails begin where they pass below the oval floor, so nothing shows under the lowered pond.
            let startX = P.stairTop + (0.95 / depth) * (P.stairBottom - P.stairTop)
            rails.stem(from: [startX, -0.05, z], to: [P.stairBottom, 0.9 - depth, z], r0: 0.025, r1: 0.025)
            rails.stem(from: [P.stairBottom, 0.9 - depth, z], to: [P.shaftEnd, 0.9 - depth, z], r0: 0.025, r1: 0.025)
        }
        add(rails, Mat.metal(0xC9A266, roughness: 0.3), name: "Handrails")

        // Walking: the stair is a slope from the top landing to the bottom; its walls hold you in.
        floors.add("Stair", .rect(x0: P.openingRect.x0, x1: P.shaftEnd + 0.01, z0: -hw, z1: hw)) { p in PondPlan.stairHeight(p.x) }
        for side: Float in [-1, 1] {
            collision.add([P.openingRect.x0, side * hw], [P.shaftEnd, side * hw], y0: -depth - 0.5, y1: -0.02)
        }
        collision.add([P.openingRect.x0, -hw], [P.openingRect.x0, hw], y0: -depth - 0.5, y1: -0.02)
    }

    // MARK: - The Reserve

    func buildReserve() {
        typealias R = ReservePlan
        let base = Entity()
        base.name = "The Reserve"
        base.position = [0, -R.depth, 0]
        building.addChild(base)
        let brick = Mat.textured(Textures.resource(Textures.brick()), roughness: 0.9)

        // Brick walls with the one opening, from the stair (arched to the aisle vault).
        let loop: [SIMD2<Float>] = [[R.x0, R.halfWidth], [R.x0, -R.halfWidth], [R.x1, -R.halfWidth], [R.x1, R.halfWidth],
                                    [R.x0 - 0.01, R.halfWidth]]
        let run = WallRun(points: loop, inside: .right, height: R.clear, thickness: 0.6,
                          openings: [WallOpening(center: R.halfWidth, width: 2.4, spring: 2.0)])
        var walls = MeshBuilder()
        walls.wall(run)
        add(walls, brick, name: "Reserve walls", to: base)
        collision.add(run: run, base: -R.depth)

        // Floor with the lighter viewing aisle.
        var slab = MeshBuilder()
        slab.floorRect(x0: R.x0, x1: R.x1, z0: -R.halfWidth, z1: -R.aisle, y: 0, up: true, tile: 1.2)
        slab.floorRect(x0: R.x0, x1: R.x1, z0: R.aisle, z1: R.halfWidth, y: 0, up: true, tile: 1.2)
        add(slab, Mat.textured(Textures.resource(Textures.stoneSlab(base: 0xEFE6DA, joint: 0xD8CCB9)), roughness: 0.75),
            name: "Reserve floor", to: base)
        var runner = MeshBuilder()
        runner.floorRect(x0: R.x0, x1: R.x1, z0: -R.aisle, z1: R.aisle, y: 0.002, up: true, tile: 1.2)
        add(runner, Mat.textured(Textures.resource(Textures.stoneSlab(base: 0xF6EFE4, joint: 0xE3D8C6)), roughness: 0.7),
            name: "Viewing aisle", to: base)
        floors.add("Reserve", .rect(x0: R.x0, x1: R.x1, z0: -R.halfWidth, z1: R.halfWidth), height: -R.depth)

        // The vault: brick bays across the width, with a barrel along the aisle (their union).
        func ceiling(_ x: Float, _ z: Float) -> Float {
            let bayCentre = ((x - R.x0) / 6).rounded(.down) * 6 + R.x0 + 3
            let dx = x - bayCentre
            var h = R.springing + sqrt(max(0, 9 - dx * dx))
            if abs(z) < 1.8 { h = max(h, R.springing + sqrt(max(0, 3.24 - z * z))) }
            return min(h, R.clear)
        }
        var vault = MeshBuilder()
        let cell: Float = 0.25
        var gx = R.x0
        while gx < R.x1 - 1e-3 {
            var gz = -R.halfWidth
            while gz < R.halfWidth - 1e-3 {
                let x0 = gx, x1 = min(R.x1, gx + cell), z0 = gz, z1 = min(R.halfWidth, gz + cell)
                let p00 = SIMD3<Float>(x0, ceiling(x0, z0), z0), p10 = SIMD3<Float>(x1, ceiling(x1, z0), z0)
                let p11 = SIMD3<Float>(x1, ceiling(x1, z1), z1), p01 = SIMD3<Float>(x0, ceiling(x0, z1), z1)
                var n = normalize(cross(p01 - p00, p10 - p00))
                if n.y > 0 { n = -n }
                vault.quad(p00, p10, p11, p01, normal: n,
                           uv: ([x0 / 0.9, (z0 + p00.y) / 0.6], [x1 / 0.9, (z0 + p10.y) / 0.6], [x1 / 0.9, (z1 + p11.y) / 0.6],
                                [x0 / 0.9, (z1 + p01.y) / 0.6]))
                gz += cell
            }
            gx += cell
        }
        add(vault, brick, name: "Reserve vault", to: base)

        // Brick columns flanking the aisle.
        var cols = MeshBuilder()
        for x in R.columnXs {
            for side: Float in [-1, 1] {
                cols.box(min: [x - 0.25, 0, side * R.columnY - 0.25], max: [x + 0.25, R.springing + 0.1, side * R.columnY + 0.25])
                collision.addRect(x0: x - 0.25, x1: x + 0.25, z0: side * R.columnY - 0.25, z1: side * R.columnY + 0.25,
                                  y0: -R.depth - 0.5, y1: -R.depth + R.springing)
            }
        }
        add(cols, brick, name: "Reserve columns", to: base)

        buildRacks(base: base)
        buildEasel(base: base)

        // Warm, low light: the easel's spot is the brightest; a few lamps along the aisle.
        for x: Float in [-66, -52, -36] {
            addLight(spot(at: [x, -R.depth + 3.6, 0], looking: [x, -R.depth, 0], colour: 0xFFD99A, intensity: 18_000,
                          inner: 50, outer: 85, radius: 10))
        }
    }

    private func buildRacks(base: Entity) {
        typealias R = ReservePlan
        let meshMat = Mat.textured(Textures.resource(Textures.brassMesh()), roughness: 0.6, metallic: 0.3)
        let brass = Mat.metal(0xC9A266, roughness: 0.35)
        var racks: [ReserveRack] = []
        for k in 0..<32 where ![9, 19, 29].contains(k) {
            let x = -15.8 - R.rackPitch * Float(k)
            for north in [true, false] {
                // The two plan chests take the first two south slots (56 racks in all; see README).
                if !north && k < 2 { continue }
                let rack = ReserveRack(north: north, x: x)
                rack.entity.position = [x, 0, rack.homeCentreY]
                base.addChild(rack.entity)
                let L = R.rack
                var mesh = MeshBuilder()
                mesh.box(min: [-L.thickness / 2 + 0.03, 0.12, -L.length / 2 + 0.05], max: [L.thickness / 2 - 0.03, L.height - 0.05, L.length / 2 - 0.05])
                rack.entity.addChild(ModelEntity(mesh: mesh.mesh(name: "rack mesh"), materials: [meshMat]))
                var frame = MeshBuilder()
                frame.box(min: [-L.thickness / 2, 0, -L.length / 2], max: [L.thickness / 2, 0.12, L.length / 2])
                frame.box(min: [-L.thickness / 2, L.height - 0.06, -L.length / 2], max: [L.thickness / 2, L.height, L.length / 2])
                frame.box(min: [-L.thickness / 2, 0, -L.length / 2], max: [L.thickness / 2, L.height, -L.length / 2 + 0.06])
                frame.box(min: [-L.thickness / 2, 0, L.length / 2 - 0.06], max: [L.thickness / 2, L.height, L.length / 2])
                // Handle at the aisle end.
                let hz = north ? L.length / 2 + 0.05 : -L.length / 2 - 0.05
                frame.box(min: [-0.07, 0.8, hz - 0.025], max: [0.07, 1.3, hz + 0.025])
                rack.entity.addChild(ModelEntity(mesh: frame.mesh(name: "rack frame"), materials: [brass]))
                racks.append(rack)
            }
        }
        reserveRacks = racks
        // Letter plates and the works on racks A–E (west face).
        for rack in racks where rack.north {
            guard let lettered = R.lettered.first(where: { abs($0.x - rack.x) < 0.01 }) else { continue }
            actionHandlers["rack.\(lettered.letter)"] = { [weak self, weak rack] in
                guard let self, let rack else { return }
                let out = rack.target == 0
                for other in self.reserveRacks where other !== rack { other.target = 0 }
                rack.target = out ? ReservePlan.rack.glide : 0
            }
            let plate = ModelEntity(mesh: .generatePlane(width: 0.18, height: 0.18),
                                    materials: [Mat.glow(Textures.resource(Textures.letterPlate(lettered.letter)))])
            plate.position = [-R.rack.thickness / 2 - 0.004, 1.55, R.rack.length / 2 - 0.2]
            plate.orientation = simd_quatf(angle: -.pi / 2, axis: [0, 1, 0])
            rack.entity.addChild(plate)
            for work in lettered.works {
                let local = SIMD3<Float>(-0.15, 1.55, work.y - rack.homeCentreY)
                let e = hangFramed(id: "reserve-\(work.id)", artworkID: work.id, image: work.id,
                                   wall: local, facing: [-1, 0, 0], width: work.w, height: work.h,
                                   frame: (0.03, 0.03, 0xC9A266, true), parent: rack.entity)
                reserveWorks.append((entity: e, rack: rack, local: local, id: work.id))
                let index = reserveWorks.count - 1
                targets[targets.count - 1].action = { [weak self] in self?.sendToEasel(index) }
                actionHandlers["easel.\(work.id)"] = { [weak self] in self?.sendToEasel(index) }
            }
        }
        // Tap a rack (its handle end) to glide it out; tap again to push it home.
        for rack in racks {
            targets.append(PickTarget(artworkID: nil, hit: { [weak rack] o, d in
                guard let rack else { return nil }
                let c = rack.entity.position(relativeTo: nil)
                let L = ReservePlan.rack
                return Self.rayBox(o, d, min: c + [-L.thickness / 2, 0, -L.length / 2], max: c + [L.thickness / 2, L.height, L.length / 2])
            }, action: { [weak self, weak rack] in
                guard let self, let rack else { return }
                let out = rack.target == 0
                for other in self.reserveRacks where other !== rack { other.target = 0 }
                rack.target = out ? ReservePlan.rack.glide : 0
            }))
            // Collision: the rack where it stands (home or out).
            for (out, active) in [(false, { [weak rack] in (rack?.offset ?? 0) < 1.2 }), (true, { [weak rack] in (rack?.offset ?? 0) >= 1.2 })] {
                let cy = rack.homeCentreY + (out ? rack.direction * R.rack.glide : 0)
                let t = R.rack.thickness / 2 + 0.05, l = R.rack.length / 2
                let pts: [SIMD2<Float>] = [[rack.x - t, cy - l], [rack.x + t, cy - l], [rack.x + t, cy + l], [rack.x - t, cy + l], [rack.x - t, cy - l]]
                collision.addPolyline(pts, occludes: false, y0: -R.depth - 0.5, y1: -R.depth + R.rack.height, active: active)
            }
        }
        updaters.append { [weak self] dt, eye in
            guard let self, eye.y < -2 else { return }
            for rack in self.reserveRacks where rack.offset != rack.target {
                let step = dt * R.rack.glide / 1.5
                rack.offset = rack.target > rack.offset ? min(rack.target, rack.offset + step) : max(rack.target, rack.offset - step)
                rack.entity.position.z = rack.homeCentreY + rack.direction * rack.offset
            }
            self.updateCarry(dt: dt)
        }
    }

    private func buildEasel(base: Entity) {
        typealias R = ReservePlan
        let e = R.easel
        var easel = MeshBuilder()
        easel.stem(from: [e.x + 0.1, 0, e.y - 0.5], to: [e.x - 0.1, 1.9, e.y - 0.1], r0: 0.035, r1: 0.03, segments: 6)
        easel.stem(from: [e.x + 0.1, 0, e.y + 0.5], to: [e.x - 0.1, 1.9, e.y + 0.1], r0: 0.035, r1: 0.03, segments: 6)
        easel.stem(from: [e.x + 0.35, 0, e.y], to: [e.x - 0.05, 1.7, e.y], r0: 0.03, r1: 0.025, segments: 6)
        easel.box(min: [e.x - 0.18, 0.82, e.y - 0.95], max: [e.x + 0.02, 0.86, e.y + 0.95])
        add(easel, Mat.metal(0x5A4632, roughness: 0.5), name: "Viewing easel", to: base)
        collision.addRect(x0: e.x - 0.3, x1: e.x + 0.4, z0: e.y - 0.6, z1: e.y + 0.6, occludes: false,
                          y0: -R.depth - 0.5, y1: -R.depth + 1.9)
        easelLight.components.set(SpotLightComponent(color: PlatformColor(hex: 0xFFD99A), intensity: 25_000,
                                                     innerAngleInDegrees: 12, outerAngleInDegrees: 22, attenuationRadius: 10))
        easelLight.position = [-17.2, -1.2, 0]
        easelLight.look(at: [-15.25, -4.25, 0], from: easelLight.position, relativeTo: nil)
        addLight(easelLight, always: false)

        // The plan chests: Degas's pastels lie flat under glass, in low light.
        for chest in R.chests {
            var box = MeshBuilder()
            box.box(min: [chest.x0, 0, 3.2], max: [chest.x1, 0.85, 4.4])
            add(box, Mat.matte(0xD9C9B0, roughness: 0.6), name: "Plan chest", to: base)
            var glass = MeshBuilder()
            glass.box(min: [chest.x0 + 0.05, 0.85, 3.25], max: [chest.x1 - 0.05, 0.9, 4.35])
            add(glass, Mat.glass(0xE9F2F1, opacity: 0.15), name: "Chest glass", to: base)
            collision.addRect(x0: chest.x0, x1: chest.x1, z0: 3.2, z1: 4.4, occludes: false, y0: -R.depth - 0.5, y1: -R.depth + 0.9)
            let cx = (chest.x0 + chest.x1) / 2
            let work = ModelEntity(mesh: .generatePlane(width: chest.w, depth: chest.h), materials: [placeholder])
            work.position = [cx, 0.855, 3.8]
            base.addChild(work)
            addImageSlot(work, image: chest.id, maxPixels: 1200, at: [cx, -R.depth + 0.9, 3.8], tint: 0xB9B2A6)
            let lo = SIMD3<Float>(cx - chest.w / 2, -R.depth + 0.8, 3.8 - chest.h / 2)
            let hi = SIMD3<Float>(cx + chest.w / 2, -R.depth + 0.95, 3.8 + chest.h / 2)
            targets.append(PickTarget(artworkID: chest.id, detail: "A pastel: it lies flat, in low light.",
                                      hit: { o, d in Self.rayBox(o, d, min: lo, max: hi) }))
        }
    }

    // MARK: Sending a painting to the easel

    func sendToEasel(_ index: Int) {
        if let current = easelWork {
            carry = (index: current, toEasel: false, t: 0, from: reserveWorks[current].entity.transformMatrix(relativeTo: nil))
            easelWork = nil
            if current == index { return }
            pendingEasel = index
            return
        }
        let w = reserveWorks[index]
        let world = w.entity.transformMatrix(relativeTo: nil)
        w.entity.removeFromParent()
        building.addChild(w.entity)
        w.entity.setTransformMatrix(world, relativeTo: nil)
        carry = (index: index, toEasel: true, t: 0, from: world)
        message = "On its way to the viewing easel…"
    }

    private func updateCarry(dt: Float) {
        guard var c = carry else { return }
        let duration: Float = 6
        c.t = min(1, c.t + dt / duration)
        carry = c
        let w = reserveWorks[c.index]
        let from = Transform(matrix: c.from)
        var to = Transform()
        if c.toEasel {
            to.translation = [-15.26, -ReservePlan.depth + 1.55, 0]
            to.rotation = simd_quatf(angle: -.pi / 2, axis: [0, 1, 0])
        } else {
            to = Transform(matrix: w.rack.entity.transformMatrix(relativeTo: nil) * Transform(translation: w.local).matrix)
            to.rotation = simd_quatf(angle: -.pi / 2, axis: [0, 1, 0])
        }
        let s = c.t * c.t * (3 - 2 * c.t)
        var p = simd_mix(from.translation, to.translation, SIMD3<Float>(repeating: s))
        p.y += sin(c.t * .pi) * 0.4
        w.entity.position = p
        w.entity.orientation = simd_slerp(from.rotation, to.rotation, s)
        if c.t >= 1 {
            carry = nil
            if c.toEasel {
                easelWork = c.index
                message = nil
            } else {
                w.entity.removeFromParent()
                w.rack.entity.addChild(w.entity)
                w.entity.position = w.local
                w.entity.orientation = simd_quatf(angle: -.pi / 2, axis: [0, 1, 0])
                if let next = pendingEasel {
                    pendingEasel = nil
                    sendToEasel(next)
                }
            }
        }
    }
}

extension Textures {
    /// A small brass plate with a rack letter.
    static func letterPlate(_ s: String) -> CGImage {
        draw(width: 128, height: 128) { ctx in
            ctx.setFillColor(cg(0xC9A266))
            ctx.fill(CGRect(x: 0, y: 0, width: 128, height: 128))
            ctx.translateBy(x: 64, y: 64)
            text(s, in: ctx, font: "Didot", size: 84, color: cg(0x2E2114))
        }
    }
}
