import CoreGraphics
import Foundation
import RealityKit
import simd

/// Section II · Élan (boards: FutureAtrium, ElanSquare, ElanSphere). The Atrium: a 28 m circle
/// of twelve bays under misty glass, centred (54, 0). At its centre a glass car joins the
/// square earth below (the Square, floor at −9 m, numbered by the Lo Shu) to the stars above
/// (the Sphere, Ø 150 m, its centre 101 m up). The Starry Night on a glass stele marks the car.
enum ElanPlan {
    static let centre = SIMD2<Float>(54, 0)
    static let radius: Float = 14
    static let wall: Float = 0.8
    static let baseHeight: Float = 6
    static let ringHeight: Float = 22
    static let ringRadius: Float = 4.2
    static let door = (width: Float(4), height: Float(4.5))
    static let car = (radius: Float(2.2), height: Float(2.6), doorHalfAngle: Float(24) * Float.pi / 180)
    static let waitRing: Float = 3.0
    static let squareFloor: Float = -9
    static let squareCeiling: Float = -1.5
    static let sphereCentreY: Float = 101
    static let sphereRadius: Float = 75
    static let topFloor: Float = 99.4               // eyes at 101: the centre
    static let home: Float = 22                      // where the car waits, in the throat
    static let speed: Float = 1.5
    static let accel: Float = 0.75
    static let starryNight = (canvas: SIMD3<Float>(50.10, 1.38, -1.90), stele: SIMD2<Float>(50.30, -1.90))

    /// Rib curves (radius, height): quadratic Béziers for the upper (glass) and lower faces.
    static func ribUpper(_ t: Float) -> SIMD2<Float> { bezier(t, [14.0, 6.0], [13.07, 19.0], [4.2, 22.0]) }
    static func ribLower(_ t: Float) -> SIMD2<Float> { bezier(t, [13.42, 6.0], [12.37, 18.5], [4.2, 21.42]) }
    static func bezier(_ t: Float, _ a: SIMD2<Float>, _ b: SIMD2<Float>, _ c: SIMD2<Float>) -> SIMD2<Float> {
        (1 - t) * (1 - t) * a + 2 * (1 - t) * t * b + t * t * c
    }

    /// World point at plan angle θ (from east towards south) and radius r about the centre.
    static func at(_ theta: Float, _ r: Float) -> SIMD2<Float> { centre + [r * cos(theta), r * sin(theta)] }
    /// Plan angle for a compass bearing (clockwise from north).
    static func theta(bearing b: Float) -> Float { (b - 90) * .pi / 180 }
}

/// The glass elevator: where the car is, where it is going, its doors.
@MainActor
final class Elevator {
    let root = Entity()          // car, mast: outside the building (visible in the Sphere)
    let car = Entity()
    var y: Float = ElanPlan.home
    var v: Float = 0
    var target: Float?
    var doors: Float = 0         // 0 closed … 1 open
    var doorsTarget: Float = 0
    var visitedSquare = false
    var dim: Float = 0           // glass fading at the top
    var leaveTimer: Float = 0
    var glassParts: [ModelEntity] = []
    var glassOpacity: [ObjectIdentifier: Float] = [:]
    var appliedDim: Float = 0
    var doorPanels: [(entity: Entity, side: Float)] = []
    var landingDoors: [(entity: Entity, side: Float, level: Float)] = []
    var glow: ModelEntity?
    let mast = Entity()
    let iris = Entity()
    var moving: Bool { target != nil }
    func at(_ level: Float) -> Bool { target == nil && abs(y - level) < 0.01 }
}

extension MuseumScene {
    func buildElan() {
        buildAtrium()
        buildSquare()
        buildElevator()
    }

    // MARK: - The Atrium

    private func buildAtrium() {
        typealias E = ElanPlan
        let c = E.centre
        // Travertine floor, an annulus round the car shaft.
        var floor = MeshBuilder()
        let segs = 96
        let radii: [Float] = [2.3, 3.0, 5, 8, 11, 14.3]
        for (r0, r1) in zip(radii, radii.dropFirst()) {
            for i in 0..<segs {
                let a0 = 2 * Float.pi * Float(i) / Float(segs), a1 = 2 * Float.pi * Float(i + 1) / Float(segs)
                let p = { (r: Float, a: Float) -> SIMD3<Float> in [c.x + r * cos(a), 0, c.y + r * sin(a)] }
                let uv = { (q: SIMD3<Float>) -> SIMD2<Float> in [q.x / 1.5, q.z / 1.5] }
                let q00 = p(r0, a0), q10 = p(r0, a1), q11 = p(r1, a1), q01 = p(r1, a0)
                floor.quad(q00, q10, q11, q01, normal: [0, 1, 0], uv: (uv(q00), uv(q10), uv(q11), uv(q01)))
            }
        }
        add(floor, Mat.textured(Textures.resource(Textures.stoneSlab(base: 0xF3EEE5, joint: 0xDDD3C2)), roughness: 0.6),
            name: "Atrium floor")
        // The bronze ring where you wait, and the waiting mark on the axis.
        var ring = MeshBuilder()
        for i in 0..<segs {
            let a0 = 2 * Float.pi * Float(i) / Float(segs), a1 = 2 * Float.pi * Float(i + 1) / Float(segs)
            let p = { (r: Float, a: Float) -> SIMD3<Float> in [c.x + r * cos(a), 0.003, c.y + r * sin(a)] }
            ring.quad(p(2.94, a0), p(2.94, a1), p(3.06, a1), p(3.06, a0), normal: [0, 1, 0])
        }
        let mark = SIMD2<Float>(50.69, 0)
        for i in 0..<24 {
            let a0 = Float.pi / 2 + Float.pi * Float(i) / 24, a1 = Float.pi / 2 + Float.pi * Float(i + 1) / 24
            let p = { (r: Float, a: Float) -> SIMD3<Float> in [mark.x + r * cos(a), 0.004, mark.y + r * sin(a)] }
            ring.quad(p(0.38, a0), p(0.38, a1), p(0.46, a1), p(0.46, a0), normal: [0, 1, 0])
        }
        ring.ellipseDisc(center: [50.35, 0.004, 0], a: 0.14, b: 0.14, up: true, segments: 16)
        add(ring, Mat.metal(0xC9A266, roughness: 0.3), name: "Bronze ring")

        // The stone base: a 0.8 m wall, 6 m high, with the one 4 m door to the Hall (west).
        let loop = Poly.circle(c, r: E.radius, from: 0, to: 2 * .pi + 0.01, segments: 192)
        let base = WallRun(points: loop, inside: .right, height: E.baseHeight, thickness: E.wall,
                           openings: [WallOpening(center: E.radius * .pi, width: E.door.width, spring: E.door.height, arched: false)])
        var wall = MeshBuilder()
        wall.wall(base)
        add(wall, Mat.matte(0xE4DBCB, roughness: 0.85), name: "Atrium stone base")
        collision.add(run: base)
        var coping = MeshBuilder()
        for i in 0..<192 {
            let a0 = 2 * Float.pi * Float(i) / 192, a1 = 2 * Float.pi * Float(i + 1) / 192
            let p = { (r: Float, a: Float) -> SIMD3<Float> in [c.x + r * cos(a), E.baseHeight, c.y + r * sin(a)] }
            coping.quad(p(E.radius, a0), p(E.radius, a1), p(E.radius + E.wall, a1), p(E.radius + E.wall, a0), normal: [0, 1, 0])
        }
        add(coping, Mat.matte(0xD8CDB9), name: "Atrium coping")

        // Twelve stone pilasters on the bay boundaries, carrying twelve dark ribs to the ring at 22 m.
        var pil = MeshBuilder()
        var ribs = MeshBuilder()
        for k in 0..<12 {
            let th = E.theta(bearing: 15 + 30 * Float(k))
            let radial = SIMD3<Float>(cos(th), 0, sin(th)), tangent = SIMD3<Float>(-sin(th), 0, cos(th))
            let foot = E.at(th, E.radius - 0.15)
            pil.box(center: [foot.x, E.baseHeight / 2, foot.y], size: [0.6, E.baseHeight, 1.6], yaw: -th)
            collision.addPolyline([E.at(th, E.radius) - SIMD2<Float>(tangent.x, tangent.z) * 0.8,
                                   E.at(th, E.radius - 0.3) - SIMD2<Float>(tangent.x, tangent.z) * 0.8,
                                   E.at(th, E.radius - 0.3) + SIMD2<Float>(tangent.x, tangent.z) * 0.8,
                                   E.at(th, E.radius) + SIMD2<Float>(tangent.x, tangent.z) * 0.8], y1: 6)
            // Rib: sweep a 0.6 m-wide band between the upper and lower curves.
            let n = 24
            let cw: Float = 0.3
            for i in 0..<n {
                let t0 = Float(i) / Float(n), t1 = Float(i + 1) / Float(n)
                let u0 = E.ribUpper(t0), u1 = E.ribUpper(t1), l0 = E.ribLower(t0), l1 = E.ribLower(t1)
                func P(_ q: SIMD2<Float>, _ s: Float) -> SIMD3<Float> {
                    [c.x, 0, c.y] + radial * q.x + [0, q.y, 0] + tangent * (s * cw)
                }
                for s: Float in [-1, 1] {
                    ribs.quad(P(u0, s), P(u1, s), P(l1, s), P(l0, s), normal: tangent * s)
                }
                let dn = normalize(SIMD2<Float>(l1.y - l0.y, -(l1.x - l0.x)))
                ribs.quad(P(l0, -1), P(l1, -1), P(l1, 1), P(l0, 1), normal: normalize(radial * -abs(dn.x) + [0, -abs(dn.y), 0]))
                let up = normalize(SIMD2<Float>(-(u1.y - u0.y), u1.x - u0.x))
                ribs.quad(P(u0, -1), P(u1, -1), P(u1, 1), P(u0, 1), normal: normalize(radial * abs(up.x) + [0, abs(up.y), 0]))
            }
        }
        add(pil, Mat.matte(0xE4DBCB, roughness: 0.85), name: "Atrium pilasters")
        // The compression ring at 22 m and the throat collar up to the Sphere's south pole.
        ribs.lathe([[E.ringRadius, 21.4], [E.ringRadius + 0.6, 21.4], [E.ringRadius + 0.6, 22.0], [E.ringRadius, 22.0], [E.ringRadius, 21.4]],
                   center: [c.x, 0, c.y], segments: 48, inside: true)
        add(ribs, Mat.metal(0x1E1C19, roughness: 0.5), name: "Atrium ribs")

        // Misty glass: the ribs' upper curve turned about the axis, a luminous haze that hides the Sphere.
        let profile = (0...24).map { E.ribUpper(Float($0) / 24) }
        var glass = MeshBuilder()
        glass.lathe(profile, center: [c.x, 0, c.y], segments: 96, inside: true)
        glass.lathe([[E.ringRadius, 22.0], [E.ringRadius, 27.0]], center: [c.x, 0, c.y], segments: 48, inside: true)
        glass.uvs = glass.uvs.map { $0 * SIMD2<Float>(72, 14) }
        var misty = UnlitMaterial()
        misty.color = .init(tint: PlatformColor(hex: 0xEEF3F1), texture: .init(Textures.resource(Textures.frit()), sampler: Mat.repeatSampler))
        add(glass, misty, name: "Misty glass")
        // The iris at the south pole (opens round the car as it passes).
        var irisMesh = MeshBuilder()
        irisMesh.ellipseDisc(center: [c.x, 26.9, c.y], a: E.ringRadius, b: E.ringRadius, up: false)
        let irisEntity = ModelEntity(mesh: irisMesh.mesh(name: "iris"), materials: [Mat.glow(0xE3ECEA)])
        irisEntity.name = "Iris"
        building.addChild(irisEntity)
        irisProxy = irisEntity

        // Light: the soft even haze of the misty roof.
        addLight(spot(at: [c.x, 20, c.y + 6], looking: [c.x, 0, c.y + 4], colour: 0xFFF7EC, intensity: 160_000, inner: 70, outer: 89, radius: 34))
        addLight(spot(at: [c.x, 20, c.y - 6], looking: [c.x, 0, c.y - 4], colour: 0xFFF7EC, intensity: 160_000, inner: 70, outer: 89, radius: 34))

        // The Starry Night on its glass stele, north of the axis, facing west to the Hall.
        let st = E.starryNight
        var stele = MeshBuilder()
        stele.box(min: [st.stele.x - 0.14, 0, st.stele.y - 0.75], max: [st.stele.x + 0.14, 1.9, st.stele.y + 0.75])
        add(stele, Mat.glass(0xBFE0E0, opacity: 0.25), name: "Starry Night stele")
        collision.addRect(x0: st.stele.x - 0.14, x1: st.stele.x + 0.14, z0: st.stele.y - 0.75, z1: st.stele.y + 0.75,
                          occludes: false, y1: 1.9)
        hangFramed(id: "starry-night", artworkID: "van-gogh-starry-night", image: "van-gogh-starry-night",
                   wall: st.canvas + [0.015, 0, 0], facing: [-1, 0, 0], width: 0.92, height: 0.74,
                   frame: (0.02, 0.025, 0x7E5C25, true))

        // Eleven bays for new art, still to be planned: a small bronze letter at the foot of each.
        let letters = ["A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K"]
        for (i, letter) in letters.enumerated() {
            let th = E.theta(bearing: 300 + 30 * Float(i))
            let p = E.at(th, E.radius - 0.9)
            let plate = ModelEntity(mesh: .generatePlane(width: 0.5, depth: 0.5),
                                    materials: [Mat.glow(Textures.resource(Textures.floorNumeral(letter, dark: false)))])
            plate.position = [p.x, 0.004, p.y]
            plate.orientation = simd_quatf(angle: -th - .pi / 2, axis: [0, 1, 0])
            building.addChild(plate)
        }
    }

    // MARK: - The Square (level −1)

    private func buildSquare() {
        typealias E = ElanPlan
        let c = E.centre
        let root = Entity()
        root.name = "The Square"
        root.position = [0, E.squareFloor, 0]
        building.addChild(root)
        let x0 = c.x - 14, x1 = c.x + 14, z0 = c.y - 14, z1 = c.y + 14
        let h = E.squareCeiling - E.squareFloor   // 7.5
        let earth = Mat.textured(Textures.resource(Textures.rammedEarth()), roughness: 0.95)

        // Floors: pale in the cross of galleries, dark in the corner rooms; a clear glass disc at 5.
        let g = (a: c.x - 4.667, b: c.x + 4.667)
        var light = MeshBuilder(), dark = MeshBuilder()
        light.floorCells(x0: x0, x1: x1, z0: z0, z1: z1, y: 0, cell: 0.4667, tile: 1.4) { p in
            let corner = (p.x < g.a || p.x > g.b) && (p.y < c.y - 4.667 || p.y > c.y + 4.667)
            return !corner && simd_distance(p, c) > 4.9
        }
        dark.floorCells(x0: x0, x1: x1, z0: z0, z1: z1, y: 0, cell: 0.4667, tile: 1.4) { p in
            (p.x < g.a || p.x > g.b) && (p.y < c.y - 4.667 || p.y > c.y + 4.667)
        }
        // Fill the ring between the cells and the glass disc exactly.
        for i in 0..<64 {
            let a0 = 2 * Float.pi * Float(i) / 64, a1 = 2 * Float.pi * Float(i + 1) / 64
            let p = { (r: Float, a: Float) -> SIMD3<Float> in [c.x + r * cos(a), -0.001, c.y + r * sin(a)] }
            light.quad(p(4.2, a0), p(4.2, a1), p(4.9, a1), p(4.9, a0), normal: [0, 1, 0])
        }
        add(light, Mat.textured(Textures.resource(Textures.stoneSlab(base: 0xF1E9DC, joint: 0xDCD0BC)), roughness: 0.8),
            name: "Square floor", to: root)
        add(dark, Mat.matte(0x2E2924, roughness: 0.9), name: "Dark room floors", to: root)
        var glassFloor = MeshBuilder()
        glassFloor.ellipseDisc(center: [c.x, 0.002, c.y], a: 4.2, b: 4.2, up: true)
        add(glassFloor, Mat.glass(0xCFE3E1, opacity: 0.18), name: "Glass floor", to: root)
        floors.add("The Square", .rect(x0: x0, x1: x1, z0: z0, z1: z1), height: E.squareFloor,
                   holes: [.ellipse(c: c, a: E.car.radius + 0.05, b: E.car.radius + 0.05)])

        // The strata pit under the glass: soil, clay, chalk, bedrock (and an ammonite in the chalk).
        var pit = MeshBuilder()
        pit.lathe([[4.2, -7], [4.2, 0]], center: [c.x, 0, c.y], segments: 48)
        let pitEntity = add(pit, Mat.textured(Textures.resource(Textures.strata()), roughness: 0.95), name: "Strata", to: root)
        pitEntity?.scale = [1, 1, 1]
        // Turn the lathe inside out: we look at it from within.
        var pitInside = MeshBuilder()
        for i in 0..<48 {
            let a0 = 2 * Float.pi * Float(i) / 48, a1 = 2 * Float.pi * Float(i + 1) / 48
            let p = { (y: Float, a: Float) -> SIMD3<Float> in [c.x + 4.2 * cos(a), y, c.y + 4.2 * sin(a)] }
            let n = { (a: Float) -> SIMD3<Float> in [-cos(a), 0, -sin(a)] }
            pitInside.quad(p(-7, a0), p(-7, a1), p(0, a1), p(0, a0), normals: n(a0), n(a1), n(a1), n(a0),
                           uv: ([Float(i) / 12, 0], [Float(i + 1) / 12, 0], [Float(i + 1) / 12, 1], [Float(i) / 12, 1]))
        }
        pitInside.ellipseDisc(center: [c.x, -7, c.y], a: 4.2, b: 4.2, up: true)
        pitEntity?.removeFromParent()
        add(pitInside, Mat.textured(Textures.resource(Textures.strata()), roughness: 0.95), name: "Strata pit", to: root)

        // Rammed-earth walls: the outer square and the partitions of the four dark rooms.
        let outer = WallRun(points: [[x0, z1], [x0, z0], [x1, z0], [x1, z1], [x0 - 0.01, z1]], inside: .right, height: h, thickness: 1.2)
        var walls = MeshBuilder()
        walls.wall(outer)
        collision.add(run: outer, base: E.squareFloor)
        let a = g.a, b = g.b, n = c.y - 4.667, s = c.y + 4.667
        func door(_ at: Float) -> [WallOpening] { [WallOpening(center: at, width: 2.6, spring: 3.0, arched: false)] }
        // Partitions (0.6 m, centred on the grid lines): doorways form a clockwise pinwheel.
        let parts: [(SIMD2<Float>, SIMD2<Float>, [WallOpening])] = [
            ([a, z0], [a, n], door(-10.633 - z0 + 1.3)),       // W1: 6 opens E into 1
            ([x0, n], [a, n], []),                              // W2
            ([b, z0], [b, n], []),                              // W3
            ([b, n], [x1, n], door(62.033 - b + 1.3)),          // W4: 8 opens S into 3
            ([b, s], [x1, s], []),                              // W5
            ([b, s], [b, z1], door(8.033 - s + 1.3)),           // W6: 4 opens W into 9
            ([a, s], [a, z1], []),                              // W7
            ([x0, s], [a, s], door(43.367 - x0 + 1.3)),         // W8: 2 opens N into 7
        ]
        for (p0, p1, openings) in parts {
            let dir = normalize(p1 - p0)
            let side = SIMD2<Float>(dir.y, -dir.x) * 0.3
            // Build as a thin wall: the run on one face, thickness 0.6 to the other.
            let run = WallRun(points: [p0 + side, p1 + side], inside: .left, height: h, thickness: 0.6, openings: openings)
            walls.wall(run)
            collision.add(run: run, base: E.squareFloor)
        }
        // Columns at the grid crossings, with their capitals.
        for (px, pz) in [(a, n), (b, n), (a, s), (b, s)] {
            walls.box(min: [px - 0.6, 0, pz - 0.6], max: [px + 0.6, h, pz + 0.6])
            collision.addRect(x0: px - 0.6, x1: px + 0.6, z0: pz - 0.6, z1: pz + 0.6, y0: E.squareFloor - 0.5, y1: E.squareCeiling)
        }
        add(walls, earth, name: "Rammed earth", to: root)
        var caps = MeshBuilder()
        for (px, pz) in [(a, n), (b, n), (a, s), (b, s)] {
            caps.box(min: [px - 1.4, h - 0.7, pz - 1.4], max: [px + 1.4, h, pz + 1.4])
        }
        add(caps, Mat.matte(0xCFC6B8), name: "Capitals", to: root)

        // Ceiling with the shaft opening and the ring of light tracing the Atrium's circle above.
        var ceil = MeshBuilder()
        ceil.floorCells(x0: x0, x1: x1, z0: z0, z1: z1, y: h, cell: 0.4667, up: false) { p in
            simd_distance(p, c) > 2.8
        }
        for i in 0..<64 {
            let a0 = 2 * Float.pi * Float(i) / 64, a1 = 2 * Float.pi * Float(i + 1) / 64
            let p = { (r: Float, ang: Float) -> SIMD3<Float> in [c.x + r * cos(ang), h, c.y + r * sin(ang)] }
            ceil.quad(p(2.3, a0), p(2.3, a1), p(3.0, a1), p(3.0, a0), normal: [0, -1, 0])
        }
        add(ceil, Mat.matte(0xCFC6B8), name: "Square ceiling", to: root)
        var lightRing = MeshBuilder()
        for i in 0..<160 {
            let a0 = 2 * Float.pi * Float(i) / 160, a1 = 2 * Float.pi * Float(i + 1) / 160
            let p = { (r: Float, ang: Float) -> SIMD3<Float> in [c.x + r * cos(ang), h - 0.02, c.y + r * sin(ang)] }
            lightRing.quad(p(13.8, a0), p(13.8, a1), p(14.0, a1), p(14.0, a0), normal: [0, -1, 0])
        }
        add(lightRing, Mat.glow(0xF4E9C8), name: "Ring of light", to: root)

        // The Lo Shu numbers cast in bronze in each floor (north up: 6 1 8 / 7 5 3 / 2 9 4).
        let grid: [[String]] = [["6", "1", "8"], ["7", "5", "3"], ["2", "9", "4"]]
        for (row, line) in grid.enumerated() {
            for (col, num) in line.enumerated() {
                let isDark = (row != 1) && (col != 1)
                var p = SIMD2<Float>(c.x - 9.333 + 9.333 * Float(col), c.y - 9.333 + 9.333 * Float(row))
                if num == "5" { p = [c.x, c.y - 3.2] }
                let plate = ModelEntity(mesh: .generatePlane(width: 1.2, depth: 1.2),
                                        materials: [Mat.glow(Textures.resource(Textures.floorNumeral(num, dark: isDark)))])
                plate.position = [p.x, 0.006, p.y]
                root.addChild(plate)
            }
        }

        // Light: daylight down the shaft onto 5; the ring of light; the dark rooms stay dark.
        addLight(spot(at: [c.x, -1.0, c.y], looking: [c.x, E.squareFloor, c.y], colour: 0xFFF1D6, intensity: 60_000,
                      inner: 18, outer: 30, radius: 14))
        for (dx, dz) in [(Float(0), Float(-9.3)), (0, 9.3), (-9.3, 0), (9.3, 0)] {
            addLight(spot(at: [c.x + dx, -1.8, c.y + dz], looking: [c.x + dx, E.squareFloor, c.y + dz], colour: 0xF4E9C8,
                          intensity: 30_000, inner: 55, outer: 88, radius: 12))
        }
    }

    // MARK: - The glass elevator

    private func buildElevator() {
        typealias E = ElanPlan
        let el = Elevator()
        elevator = el
        let c = E.centre
        el.root.name = "Élan elevator"
        root.addChild(el.root)
        el.car.position = [c.x, E.home, c.y]
        el.root.addChild(el.car)

        let R = E.car.radius, H = E.car.height, gap = E.car.doorHalfAngle
        let doorTheta: Float = .pi   // west
        // Car walls (glass) with the door gap; frame rings; roof; frosted floor; rail.
        var glassMesh = MeshBuilder()
        let n = 64
        for i in 0..<n {
            let a0 = 2 * Float.pi * Float(i) / Float(n), a1 = 2 * Float.pi * Float(i + 1) / Float(n)
            let mid = (a0 + a1) / 2
            if abs(remainder(mid - doorTheta, 2 * .pi)) < gap { continue }
            let p = { (a: Float, y: Float) -> SIMD3<Float> in [R * cos(a), y, R * sin(a)] }
            let nn = { (a: Float) -> SIMD3<Float> in [cos(a), 0, sin(a)] }
            glassMesh.quad(p(a0, 0.05), p(a1, 0.05), p(a1, H), p(a0, H), normals: nn(a0), nn(a1), nn(a1), nn(a0))
        }
        glassMesh.ellipseDisc(center: [0, H, 0], a: R, b: R, up: true)
        let glassEntity = ModelEntity(mesh: glassMesh.mesh(name: "car glass"), materials: [Mat.glass(0xBFE0E0, opacity: 0.22)])
        el.car.addChild(glassEntity)
        el.glassParts.append(glassEntity)
        el.glassOpacity[ObjectIdentifier(glassEntity)] = 0.22
        var frameMesh = MeshBuilder()
        frameMesh.lathe([[R - 0.04, H - 0.08], [R + 0.03, H - 0.08], [R + 0.03, H], [R - 0.04, H]], center: .zero, segments: 64, inside: true)
        let frameEntity = ModelEntity(mesh: frameMesh.mesh(name: "car frame"), materials: [Mat.metal(0x2C6E73, roughness: 0.4)])
        el.car.addChild(frameEntity)
        el.glassParts.append(frameEntity)
        var floorMesh = MeshBuilder()
        floorMesh.ellipseDisc(center: [0, 0.02, 0], a: R, b: R, up: true)
        floorMesh.lathe([[R + 0.03, -0.1], [R + 0.03, 0.05]], center: .zero, segments: 64)
        el.car.addChild(ModelEntity(mesh: floorMesh.mesh(name: "car floor"), materials: [Mat.glass(0xDDE8E6, opacity: 0.55)]))
        var rail = MeshBuilder()
        rail.lathe([[R - 0.12, 1.02], [R - 0.08, 1.02], [R - 0.08, 1.08], [R - 0.12, 1.08], [R - 0.12, 1.02]], center: .zero, segments: 64)
        el.car.addChild(ModelEntity(mesh: rail.mesh(name: "rail"), materials: [Mat.glass(0xEFF4F3, opacity: 0.7)]))
        var sleeve = MeshBuilder()
        sleeve.lathe([[0.19, 0.02], [0.19, 0.06]], center: .zero, segments: 16)
        sleeve.ellipseDisc(center: [0, 0.06, 0], a: 0.19, b: 0.19, up: true, segments: 16)
        el.car.addChild(ModelEntity(mesh: sleeve.mesh(name: "sleeve"), materials: [Mat.metal(0x2C6E73, roughness: 0.4)]))
        // Glow when it moves.
        var glowMesh = MeshBuilder()
        glowMesh.lathe([[R + 0.04, 0.0], [R + 0.04, H]], center: .zero, segments: 64)
        var glowMat = UnlitMaterial(color: PlatformColor(hex: 0x7CC3C5))
        glowMat.blending = .transparent(opacity: 0.18)
        let glow = ModelEntity(mesh: glowMesh.mesh(name: "glow"), materials: [glowMat])
        glow.isEnabled = false
        el.car.addChild(glow)
        el.glow = glow
        // Two curved sliding door leaves.
        for side: Float in [-1, 1] {
            var leaf = MeshBuilder()
            let a0 = doorTheta, a1 = doorTheta + side * gap
            for i in 0..<8 {
                let t0 = a0 + (a1 - a0) * Float(i) / 8, t1 = a0 + (a1 - a0) * Float(i + 1) / 8
                let p = { (a: Float, y: Float) -> SIMD3<Float> in [(R - 0.03) * cos(a), y, (R - 0.03) * sin(a)] }
                leaf.quad(p(t0, 0.05), p(t1, 0.05), p(t1, 2.3), p(t0, 2.3), normal: [cos(t0), 0, sin(t0)])
            }
            let e = ModelEntity(mesh: leaf.mesh(name: "car door"), materials: [Mat.glass(0xCFE8E8, opacity: 0.3)])
            el.car.addChild(e)
            el.doorPanels.append((e, side))
            el.glassParts.append(e)
            el.glassOpacity[ObjectIdentifier(e)] = 0.3
        }
        // Landing screens (clear glass, 2.7 m) with sliding doors, at the Atrium and the Square.
        for level: Float in [0, E.squareFloor] {
            var screen = MeshBuilder()
            for i in 0..<n {
                let a0 = 2 * Float.pi * Float(i) / Float(n), a1 = 2 * Float.pi * Float(i + 1) / Float(n)
                if abs(remainder((a0 + a1) / 2 - doorTheta, 2 * .pi)) < gap { continue }
                let p = { (a: Float, y: Float) -> SIMD3<Float> in [c.x + 2.3 * cos(a), level + y, c.y + 2.3 * sin(a)] }
                screen.quad(p(a0, 0), p(a1, 0), p(a1, 2.7), p(a0, 2.7), normal: [cos(a0), 0, sin(a0)])
            }
            add(screen, Mat.glass(0xD8ECEC, opacity: 0.12), name: "Landing screen")
            collision.addCircle(c, r: 2.3, segments: 48, occludes: false, y0: level - 0.5, y1: level + 2.7,
                                gap: (from: doorTheta - gap, to: doorTheta + gap))
            for side: Float in [-1, 1] {
                var leaf = MeshBuilder()
                let a0 = doorTheta, a1 = doorTheta + side * gap
                for i in 0..<8 {
                    let t0 = a0 + (a1 - a0) * Float(i) / 8, t1 = a0 + (a1 - a0) * Float(i + 1) / 8
                    let p = { (a: Float, y: Float) -> SIMD3<Float> in [2.32 * cos(a), y, 2.32 * sin(a)] }
                    leaf.quad(p(t0, 0), p(t1, 0), p(t1, 2.6), p(t0, 2.6), normal: [cos(t0), 0, sin(t0)])
                }
                let e = ModelEntity(mesh: leaf.mesh(name: "landing door"), materials: [Mat.glass(0xD8ECEC, opacity: 0.2)])
                e.position = [c.x, level, c.y]
                building.addChild(e)
                el.landingDoors.append((e, side, level))
            }
            // The landing door gap is solid unless the car is here with its doors open.
            let lv = level
            collision.add(E.at(doorTheta - gap, 2.3), E.at(doorTheta + gap, 2.3), occludes: false, y0: level - 0.5, y1: level + 2.7,
                          active: { [weak el] in !(el?.at(lv) == true && (el?.doors ?? 0) > 0.95) })
        }
        // The car's own walls hold you in while it moves (door gap solid unless open).
        collision.addCircle(c, r: R - 0.02, segments: 48, occludes: false, y0: -100, y1: 200,
                            gap: (from: doorTheta - gap, to: doorTheta + gap),
                            active: { [weak self, weak el] in
                                guard let self, let el else { return false }
                                return abs(self.visitorFeet - el.y) < 1.2 && simd_distance(self.visitorPlan, c) < R
                            })
        collision.add(E.at(doorTheta - gap, R - 0.02), E.at(doorTheta + gap, R - 0.02), occludes: false, y0: -100, y1: 200,
                      active: { [weak self, weak el] in
                          guard let self, let el else { return false }
                          return abs(self.visitorFeet - el.y) < 1.2 && simd_distance(self.visitorPlan, c) < R && el.doors < 0.95
                      })
        floors.add("Car", .ellipse(c: c, a: R + 0.1, b: R + 0.1)) { [weak el] _ in el?.y ?? 0 }

        // The bronze mast inside the Sphere (from the south pole to the car's stop).
        let mastMesh = MeshResource.generateCylinder(height: E.topFloor - 26, radius: 0.2)
        let mastModel = ModelEntity(mesh: mastMesh, materials: [Mat.metal(0xC9A266, roughness: 0.3)])
        mastModel.position = [c.x, 26 + (E.topFloor - 26) / 2, c.y]
        el.mast.addChild(mastModel)
        el.mast.isEnabled = false
        el.root.addChild(el.mast)

        // Tap the car (from the Atrium) to call it.
        targets.append(PickTarget(artworkID: nil, hit: { [weak el] o, d in
            guard let el else { return nil }
            let p = el.car.position(relativeTo: nil)
            return Self.rayBox(o, d, min: p + [-R, 0, -R], max: p + [R, H, R])
        }, action: { [weak self] in self?.perform("elevator.call") }))

        actionHandlers["elevator.call"] = { [weak self, weak el] in
            guard let self, let el, abs(self.visitorFeet) < 0.5 else { return }
            if !el.at(0) {
                el.target = 0
                self.message = "The car glows down through the misty glass. The wait is part of the visit."
            }
        }
        actionHandlers["elevator.square"] = { [weak self, weak el] in
            el?.target = E.squareFloor
            self?.message = "Down to the earth first…"
        }
        actionHandlers["elevator.atrium"] = { [weak self, weak el] in
            el?.target = 0
            self?.message = nil
        }
        actionHandlers["elevator.sphere"] = { [weak self, weak el] in
            el?.target = E.topFloor
            self?.message = "…then up to the stars. A steady 1.5 metres a second."
        }
        promptProviders.append { [weak self, weak el] eye in
            guard let self, let el else { return [] }
            let inCar = simd_distance(self.visitorPlan, c) < R && abs(self.visitorFeet - el.y) < 0.3
            if inCar {
                guard !el.moving, el.doors > 0.95 || el.at(E.topFloor) else { return [] }
                if el.at(0) {
                    return el.visitedSquare
                        ? [ActionPrompt(id: "elevator.sphere", title: "Up to the Sphere", symbol: "arrow.up"),
                           ActionPrompt(id: "elevator.square", title: "Down to the Square", symbol: "arrow.down")]
                        : [ActionPrompt(id: "elevator.square", title: "Down to the Square", symbol: "arrow.down")]
                }
                if el.at(E.squareFloor) { return [ActionPrompt(id: "elevator.atrium", title: "Up to the Atrium", symbol: "arrow.up")] }
                if el.at(E.topFloor), el.dim > 0.99 { return [ActionPrompt(id: "elevator.atrium", title: "Down to the Atrium", symbol: "arrow.down")] }
                return []
            }
            // On the bronze ring at the Atrium, with no car here: call it.
            let d = simd_distance(self.visitorPlan, c)
            if abs(self.visitorFeet) < 0.3, d < E.waitRing + 0.6, d > 2.35, !el.at(0), el.target != 0 {
                return [ActionPrompt(id: "elevator.call", title: "Call the car", symbol: "bell")]
            }
            return []
        }
        updaters.append { [weak self] dt, eye in self?.updateElevator(dt: dt, eye: eye) }
    }

    private func updateElevator(dt: Float, eye: SIMD3<Float>) {
        guard let el = elevator else { return }
        typealias E = ElanPlan
        let c = E.centre
        let inCar = simd_distance(visitorPlan, c) < E.car.radius && abs(visitorFeet - el.y) < 0.6
        // Doors first: close before moving.
        if el.target != nil { el.doorsTarget = 0 }
        if el.doors != el.doorsTarget {
            let step = dt / 1.5
            el.doors = el.doorsTarget > el.doors ? min(el.doorsTarget, el.doors + step) : max(el.doorsTarget, el.doors - step)
        }
        if let target = el.target, el.doors <= 0.001 {
            // Undim the glass before descending from the top.
            if el.dim > 0 {
                el.dim = max(0, el.dim - dt / 2)
            } else {
                let dist = target - el.y
                let dir: Float = dist > 0 ? 1 : -1
                let brake = sqrt(2 * E.accel * abs(dist))
                el.v = min(E.speed, min(brake, abs(el.v) + E.accel * dt)) * dir
                var step = el.v * dt
                if abs(step) >= abs(dist) || abs(dist) < 0.005 { step = dist }
                el.y += step
                if abs(target - el.y) < 0.005 {
                    el.y = target
                    el.v = 0
                    el.target = nil
                    if target == E.squareFloor { el.visitedSquare = true }
                    if target == 0 || target == E.squareFloor { el.doorsTarget = 1 }
                    if target == E.topFloor { message = "The centre of the Sphere: the sky over you, now." }
                    else if message?.hasPrefix("The car glows") == true || message?.hasPrefix("Down to") == true
                                || message?.hasPrefix("…then") == true || message?.hasPrefix("The centre") == true { message = nil }
                }
            }
        }
        // At the top the glass dims to a rail and a floor.
        if el.at(E.topFloor) { el.dim = min(1, el.dim + dt / 3) }
        // Left behind at the Atrium: the car closes and goes home to the throat.
        if el.at(0), el.doors > 0.95, !inCar, simd_distance(visitorPlan, c) > E.waitRing + 0.5 || abs(visitorFeet) > 0.5 {
            el.leaveTimer += dt
            if el.leaveTimer > 4 { el.target = E.home; el.leaveTimer = 0 }
        } else {
            el.leaveTimer = 0
        }
        // Apply.
        el.car.position = [c.x, el.y, c.y]
        el.glow?.isEnabled = el.target != nil
        let open = el.doors * el.doors * (3 - 2 * el.doors)
        for (leaf, side) in el.doorPanels {
            leaf.orientation = simd_quatf(angle: -side * open * E.car.doorHalfAngle * 2, axis: [0, 1, 0])
        }
        for (leaf, side, level) in el.landingDoors {
            let o: Float = abs(el.y - level) < 0.01 ? open : 0
            leaf.orientation = simd_quatf(angle: -side * o * E.car.doorHalfAngle * 2, axis: [0, 1, 0])
        }
        // Fade the glass (not the rail or the floor) by its own opacity, only while it changes.
        if abs(el.dim - el.appliedDim) > 0.02 || (el.dim == 0 && el.appliedDim != 0) || (el.dim >= 1 && el.appliedDim < 1) {
            el.appliedDim = el.dim
            for part in el.glassParts {
                part.isEnabled = el.dim < 0.98
                if part.isEnabled, let base = el.glassOpacity[ObjectIdentifier(part)] {
                    part.model?.materials = [Mat.glass(0xBFE0E0, opacity: base * (1 - el.dim))]
                }
            }
        }
        // Inside the Sphere: the building is gone, only the sky, the mast and the car remain.
        let inSphere = inCar && el.y > 26
        building.isEnabled = !inSphere
        el.mast.isEnabled = inSphere
        skySystem?.sphereMode = inSphere
        irisProxy?.isEnabled = !(el.y + E.car.height > 24 && el.y < 27)
    }
}

extension Textures {
    /// Misty glass: a fine frit of dots.
    static func frit() -> CGImage {
        draw(width: 64, height: 64) { ctx in
            ctx.setFillColor(cg(0xF4F7F6))
            ctx.fill(CGRect(x: 0, y: 0, width: 64, height: 64))
            ctx.setFillColor(cg(0xDCE6E6))
            for x in stride(from: 4, to: 64, by: 16) {
                for y in stride(from: 4, to: 64, by: 16) {
                    ctx.fillEllipse(in: CGRect(x: x, y: y, width: 6, height: 6))
                }
            }
        }
    }

    /// Rammed earth in visible layers.
    static func rammedEarth() -> CGImage {
        draw(width: 256, height: 256) { ctx in
            var rng = SplitMix(seed: 21)
            var y: CGFloat = 0
            while y < 256 {
                let h = rng.next(10, 28)
                let shades: [UInt32] = [0xA07F5A, 0x9A7853, 0xA88763, 0x957350, 0xAD8B67]
                ctx.setFillColor(cg(shades[Int(rng.nextUInt() % 5)]))
                ctx.fill(CGRect(x: 0, y: y, width: 256, height: h))
                ctx.setFillColor(cg(0x8A6B48, 0.6))
                ctx.fill(CGRect(x: 0, y: y, width: 256, height: 1.5))
                y += h
            }
        }
    }

    /// Soil, clay, chalk and bedrock (top to bottom over 7 m), with an ammonite in the chalk.
    static func strata() -> CGImage {
        draw(width: 256, height: 512) { ctx in
            // v = 0 at the pit bottom (−7 m), 1 at the glass (0 m); CG y up matches.
            let px: CGFloat = 512 / 7
            let bands: [(from: CGFloat, to: CGFloat, c: UInt32)] = [
                (0, 7 - 4.8, 0x8F877C), (7 - 4.8, 7 - 3.6, 0xDCD2BE), (7 - 3.6, 7 - 2.2, 0xC8B08A), (7 - 2.2, 7, 0xB89A74),
            ]
            for b in bands {
                ctx.setFillColor(cg(b.c))
                ctx.fill(CGRect(x: 0, y: b.from * px, width: 256, height: (b.to - b.from) * px))
            }
            var rng = SplitMix(seed: 9)
            for _ in 0..<300 {
                ctx.setFillColor(cg(0x000000, CGFloat(rng.next(0.03, 0.09))))
                ctx.fillEllipse(in: CGRect(x: rng.next(0, 256), y: rng.next(0, 512), width: rng.next(2, 7), height: rng.next(2, 5)))
            }
            // Ammonite.
            ctx.setStrokeColor(cg(0x9D9280))
            ctx.setLineWidth(3)
            let cx: CGFloat = 60, cy = (7 - 4.1) * px
            let spiral = CGMutablePath()
            for i in 0..<120 {
                let a = CGFloat(i) * 0.18
                let r = 2 + a * 2.6
                let p = CGPoint(x: cx + r * cos(a), y: cy + r * sin(a))
                if i == 0 { spiral.move(to: p) } else { spiral.addLine(to: p) }
            }
            ctx.addPath(spiral)
            ctx.strokePath()
        }
    }

    /// A bronze numeral (or letter) cast in the floor.
    static func floorNumeral(_ s: String, dark: Bool) -> CGImage {
        draw(width: 256, height: 256) { ctx in
            ctx.setFillColor(dark ? cg(0x2E2924) : cg(0xF1E9DC))
            ctx.fill(CGRect(x: 0, y: 0, width: 256, height: 256))
            ctx.translateBy(x: 128, y: 128)
            text(s, in: ctx, font: "Didot", size: 200, color: dark ? cg(0xC9A266) : cg(0x7E5C25))
        }
    }
}
