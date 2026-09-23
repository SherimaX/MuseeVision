import CoreGraphics
import Foundation
import RealityKit
import simd

/// The Sculpture Hall, north of the Rotunda (board: Sculpture). One top-lit court 16.8 m
/// square, centre (0, −22.5): travertine floor, warm-white walls to 10 m, a laylight under a
/// glass roof. Ten works from Rude to Bourdelle; The Gates of Hell close the axis 30 m from
/// the sun rosette. Nothing stands on the axis.
enum SculptureHallPlan {
    static let x0: Float = -8.4, x1: Float = 8.4, z0: Float = -30.9, z1: Float = -14.1
    static let wallHeight: Float = 10
    static let wall: Float = 0.6
    static let door = (width: Float(4), spring: Float(4.5))           // crown 6.5 m, as the Rotunda's N door
    static let passage = (x: Float(2), z0: Float(-10.6), z1: Float(-13.5))
    static let laylight = (x0: Float(-7), x1: Float(7), z0: Float(-29.7), z1: Float(-15.3))

    struct Work {
        let number: Int
        let id: String
        let centre: SIMD2<Float>
        let plinth: SIMD3<Float>          // width (E–W), height, depth (N–S); height 0 = none
        let height: Float
        let facing: SIMD2<Float>
        let material: Material
        enum Material { case bronze, darkBronze, marble, stone, plaster, gilt }
    }

    static let works: [Work] = [
        Work(number: 1, id: "rude-marseillaise", centre: [-6.40, -16.30], plinth: [0.6, 1.3, 0.6], height: 0.7, facing: [1, 0], material: .plaster),
        Work(number: 2, id: "carpeaux-la-danse", centre: [-7.70, -21.10], plinth: [1.4, 0.8, 3.0], height: 4.20, facing: [1, 0], material: .stone),
        Work(number: 3, id: "rodin-age-of-bronze", centre: [-6.40, -25.50], plinth: [0.9, 0.9, 0.9], height: 1.78, facing: [1, 0], material: .bronze),
        Work(number: 4, id: "rodin-gates-of-hell", centre: [0, -30.45], plinth: [0, 0, 0], height: 6.35, facing: [0, 1], material: .darkBronze),
        Work(number: 5, id: "rodin-thinker", centre: [-3.60, -26.90], plinth: [1.2, 1.2, 1.2], height: 1.80, facing: [0, 1], material: .bronze),
        Work(number: 6, id: "rodin-kiss", centre: [3.60, -26.90], plinth: [1.3, 0.5, 1.1], height: 1.81, facing: [0, 1], material: .marble),
        Work(number: 7, id: "rodin-burghers-of-calais", centre: [5.20, -22.70], plinth: [2.8, 0.12, 2.6], height: 2.02, facing: [-1, 0], material: .bronze),
        Work(number: 8, id: "rodin-balzac", centre: [6.40, -19.10], plinth: [1.2, 0.3, 1.2], height: 2.82, facing: [-1, 0], material: .darkBronze),
        Work(number: 9, id: "rodin-walking-man", centre: [6.40, -15.90], plinth: [1.0, 0.3, 1.0], height: 2.13, facing: [-1, 0], material: .bronze),
        Work(number: 10, id: "bourdelle-heracles-archer", centre: [3.90, -17.30], plinth: [1.6, 0.5, 1.2], height: 2.48, facing: [1, 0], material: .gilt),
    ]
}

extension MuseumScene {
    func buildSculptureHall() {
        typealias P = SculptureHallPlan
        // Court walls: a loop of inner faces from the SW corner, the 4 m door in the south wall.
        let loop: [SIMD2<Float>] = [[P.x0, P.z1], [P.x0, P.z0], [P.x1, P.z0], [P.x1, P.z1], [P.x0 - 0.01, P.z1]]
        // Arc length from the start to the door centre: west + north + east walls + 8.4 m along the south.
        let s = (P.z1 - P.z0) * 2 + (P.x1 - P.x0) + P.x1
        let court = WallRun(points: loop, inside: .right, height: P.wallHeight, thickness: P.wall,
                            openings: [WallOpening(center: s, width: P.door.width, spring: P.door.spring)])
        var w = MeshBuilder()
        w.wall(court)
        add(w, Mat.honedStone(.travertine, tint: 0xFFFBF3, seed: 5), name: "Sculpture court walls")
        var m = MeshBuilder()
        m.band(court, from: 0, to: 0.18, depth: 0.03)
        m.band(court, from: 3.18, to: 3.22, depth: 0.01)
        m.band(court, from: 6.38, to: 6.42, depth: 0.01)
        m.band(court, from: 9.3, to: 9.65, depth: 0.25)
        add(m, Mat.matte(0xE4DBCB), name: "Sculpture court mouldings")
        collision.add(run: court)
        contactShade(court)

        // Passage from the Rotunda: 4 m wide under a barrel vault continuing the door's arch.
        barrelPassage(alongZ: true, fixed: 0, from: P.passage.z0, to: P.passage.z1 - 0.01, halfWidth: P.passage.x,
                      spring: P.door.spring, material: Mat.matte(0xF1EBDF), name: "Sculpture passage")

        // Travertine floor.
        var f = MeshBuilder()
        f.floorRect(x0: P.x0, x1: P.x1, z0: P.z0, z1: P.z1, y: 0, up: true, tile: 2.4)
        f.floorRect(x0: -P.passage.x, x1: P.passage.x, z0: P.z1, z1: P.passage.z0 + 0.8, y: -0.002, up: true, tile: 1.2)
        add(f, Mat.polishedStone(.travertine, polish: 0.7, seed: 4),
            name: "Sculpture court floor")

        // Ceiling with its laylight (a glowing diffuser under the glass roof).
        var ceil = MeshBuilder()
        let L = P.laylight
        ceil.floorRect(x0: P.x0, x1: P.x1, z0: P.z0, z1: L.z0, y: 9.65, up: false)
        ceil.floorRect(x0: P.x0, x1: P.x1, z0: L.z1, z1: P.z1, y: 9.65, up: false)
        ceil.floorRect(x0: P.x0, x1: L.x0, z0: L.z0, z1: L.z1, y: 9.65, up: false)
        ceil.floorRect(x0: L.x1, x1: P.x1, z0: L.z0, z1: L.z1, y: 9.65, up: false)
        // Reveal round the laylight.
        for (a, b) in [(SIMD2<Float>(L.x0, L.z0), SIMD2<Float>(L.x1, L.z0)), ([L.x1, L.z0], [L.x1, L.z1]),
                       ([L.x1, L.z1], [L.x0, L.z1]), ([L.x0, L.z1], [L.x0, L.z0])] {
            let n = normalize(SIMD2<Float>(-(b - a).y, (b - a).x))
            ceil.quad([a.x, 9.65, a.y], [b.x, 9.65, b.y], [b.x, 9.97, b.y], [a.x, 9.97, a.y], normal: [n.x, 0, n.y])
        }
        add(ceil, Mat.matte(0xE4DBCB), name: "Sculpture ceiling")
        var lay = MeshBuilder()
        lay.floorRect(x0: L.x0, x1: L.x1, z0: L.z0, z1: L.z1, y: 9.97, up: false, tile: 1.4)
        add(lay, Mat.glow(lightGridTexture, tint: 0xFFF3DA, repeating: true), name: "Sculpture laylight")
        addLight(withShadow(spot(at: [0, 9.8, -22.5], looking: [0, 0, -22.5], colour: 0xFFF1D6, intensity: 150_000,
                                 inner: 55, outer: 89, radius: 26), softness: 2.5))

        // The bench opposite The Dance.
        var bench = MeshBuilder()
        bench.box(min: [-4.9, 0, -22.3], max: [-4.4, 0.45, -19.9])
        add(bench, Mat.matte(0xE4DBCB, roughness: 0.7), name: "Sculpture bench")
        collision.addRect(x0: -4.9, x1: -4.4, z0: -22.3, z1: -19.9, occludes: false, y1: 0.45)

        // The ten works on their plinths.
        var plinths = MeshBuilder()
        for work in P.works {
            let c = work.centre
            let pl = work.plinth
            if pl.y > 0 {
                plinths.box(min: [c.x - pl.x / 2, 0, c.y - pl.z / 2], max: [c.x + pl.x / 2, pl.y, c.y + pl.z / 2])
                collision.addRect(x0: c.x - pl.x / 2, x1: c.x + pl.x / 2, z0: c.y - pl.z / 2, z1: c.y + pl.z / 2,
                                  occludes: false, y1: pl.y + work.height)
            }
            let base = SIMD3<Float>(c.x, pl.y, c.y)
            let mat: RealityKit.Material
            switch work.material {
            case .bronze: mat = Mat.metal(0x6B5A45, roughness: 0.5)
            case .darkBronze: mat = Mat.metal(0x4A3B2C, roughness: 0.55)
            case .marble: mat = Mat.matte(0xF7F3EA, roughness: 0.35)
            case .stone: mat = Mat.matte(0xE9E1D2, roughness: 0.8)
            case .plaster: mat = Mat.matte(0xEDE6D8, roughness: 0.85)
            case .gilt: mat = Mat.metal(0xC9A266, roughness: 0.3)
            }
            let footprint: SIMD2<Float> = work.number == 4 ? [4.0, 0.9] : [max(0.5, pl.x * 0.8), max(0.5, pl.z * 0.8)]
            placeSculpture(id: work.id, at: base, facing: work.facing, height: work.height, material: mat, pickBox: nil,
                           standInFootprint: footprint)
            if pl.y <= 0 || work.number == 7 {
                // The Gates stand on the floor against the wall; the Burghers stand among you.
                let half = work.number == 4 ? SIMD2<Float>(2.0, 0.45) : SIMD2<Float>(1.4, 1.3)
                collision.addRect(x0: c.x - half.x, x1: c.x + half.x, z0: c.y - half.y, z1: c.y + half.y, occludes: false,
                                  y1: work.height)
            }
        }
        add(plinths, Mat.matte(0xCFC6B8, roughness: 0.75), name: "Sculpture plinths")
    }

    /// A straight passage between two parallel walls under a semicircular barrel vault.
    /// `alongZ`: the passage runs north–south at x = fixed; otherwise east–west at z = fixed.
    func barrelPassage(alongZ: Bool, fixed: Float, from a: Float, to b: Float, halfWidth hw: Float, spring: Float,
                       material: RealityKit.Material, name: String, floorY: Float = 0, addCollision: Bool = true) {
        var m = MeshBuilder()
        func P(_ along: Float, _ lateral: Float, _ y: Float) -> SIMD3<Float> {
            alongZ ? [fixed + lateral, y, along] : [along, y, fixed + lateral]
        }
        func N(_ lateral: Float, _ y: Float) -> SIMD3<Float> { alongZ ? [lateral, y, 0] : [0, y, lateral] }
        // Side walls, facing into the passage.
        for side: Float in [-1, 1] {
            m.quad(P(a, side * hw, floorY), P(b, side * hw, floorY), P(b, side * hw, floorY + spring), P(a, side * hw, floorY + spring),
                   normal: N(-side, 0))
        }
        for i in 0..<32 {
            let t0 = Float.pi * Float(i) / 32, t1 = Float.pi * Float(i + 1) / 32
            let l0 = hw * cos(t0), y0 = floorY + spring + hw * sin(t0)
            let l1 = hw * cos(t1), y1 = floorY + spring + hw * sin(t1)
            m.quad(P(a, l0, y0), P(b, l0, y0), P(b, l1, y1), P(a, l1, y1),
                   normals: N(-cos(t0), -sin(t0)), N(-cos(t0), -sin(t0)), N(-cos(t1), -sin(t1)), N(-cos(t1), -sin(t1)))
        }
        add(m, material, name: name)
        if addCollision {
            for side: Float in [-1, 1] {
                let p0 = P(a, side * hw, 0), p1 = P(b, side * hw, 0)
                collision.add([p0.x, p0.z], [p1.x, p1.z], y0: floorY - 0.5, y1: floorY + spring + hw)
            }
        }
    }
}
