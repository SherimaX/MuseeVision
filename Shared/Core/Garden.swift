import CoreGraphics
import Foundation
import RealityKit
import simd

/// The four seasons, from the 24 solar terms at the visitor's date (hemisphere-aware).
enum Season: Int {
    case spring, summer, autumn, winter

    static func now(_ date: Date = MuseumScene.now(), latitude: Double) -> Season {
        var term = Ephemeris.solarTerm(date)
        if latitude < 0 { term = (term + 12) % 24 }
        return Season(rawValue: term / 6) ?? .spring
    }
}

/// Planting kit shared by the gardens: trees, flowers, grass and hedges, in low-poly forms
/// that read at a glance and cost little on a phone.
@MainActor
enum Garden {
    static func grassTexture(base: UInt32 = 0x8FA36E) -> TextureResource {
        Textures.resource(Textures.draw(width: 256, height: 256, name: "grass_" + String(format: "%06X", base)) { ctx in
            ctx.setFillColor(Textures.cg(base))
            ctx.fill(CGRect(x: 0, y: 0, width: 256, height: 256))
            var rng = SplitMix(seed: 11)
            for _ in 0..<900 {
                let shade: UInt32 = [0x7C9160, 0xA2B47F, 0x6F8456, 0xB3C08F][Int(rng.nextUInt() % 4)]
                ctx.setFillColor(Textures.cg(shade, 0.55))
                let x = rng.next(0, 256), y = rng.next(0, 256)
                ctx.fill(CGRect(x: x, y: y, width: 1.5, height: rng.next(3, 9)))
            }
        })
    }

    /// Adds a tree: trunk (and a few branches) plus crown blobs; the crown sways gently.
    /// Returns the crown entity so callers can animate or recolour it.
    @discardableResult
    static func tree(in scene: MuseumScene, at p: SIMD3<Float>, trunkHeight: Float, trunkRadius: Float,
                     trunkColour: UInt32, crownCentre: Float, crownRadii: SIMD3<Float>, crownColour: UInt32?,
                     blobs: Int = 5, seed: UInt64, branches: Int = 4, flowerColour: UInt32? = nil) -> Entity {
        var trunk = MeshBuilder()
        var rng = SplitMix(seed: seed)
        trunk.stem(from: p, to: p + [0, trunkHeight, 0], r0: trunkRadius, r1: trunkRadius * 0.6)
        for i in 0..<branches {
            let a = Float(i) * 2.4 + Float(rng.next(0, 1))
            let h = trunkHeight * Float(rng.next(0.55, 0.95))
            let dir = normalize(SIMD3<Float>(cos(a), Float(rng.next(0.6, 1.2)), sin(a)))
            let len = max(crownRadii.x, crownRadii.z) * Float(rng.next(0.6, 1.0))
            trunk.stem(from: p + [0, h, 0], to: p + [0, h, 0] + dir * len, r0: trunkRadius * 0.45, r1: trunkRadius * 0.12, segments: 6)
        }
        scene.add(trunk, Mat.matte(trunkColour, roughness: 0.9), name: "Trunk")
        let crown = Entity()
        crown.name = "Crown"
        crown.position = p + [0, crownCentre, 0]
        scene.building.addChild(crown)
        if let crownColour {
            var b = MeshBuilder()
            for i in 0..<blobs {
                let off = SIMD3<Float>(Float(rng.next(-0.45, 0.45)) * crownRadii.x, Float(rng.next(-0.35, 0.35)) * crownRadii.y,
                                       Float(rng.next(-0.45, 0.45)) * crownRadii.z)
                let s = Float(rng.next(0.55, 0.8))
                b.ellipsoid(center: i == 0 ? .zero : off, radii: crownRadii * (i == 0 ? 0.8 : s), segments: 12, rings: 8)
            }
            crown.addChild(ModelEntity(mesh: b.mesh(name: "crown"), materials: [Mat.matte(crownColour, roughness: 0.95)]))
            if let flowerColour {
                var fl = MeshBuilder()
                for _ in 0..<60 {
                    let u = normalize(SIMD3<Float>(Float(rng.next(-1, 1)), Float(rng.next(-0.6, 1)), Float(rng.next(-1, 1))))
                    fl.ellipsoid(center: u * crownRadii * 0.92, radii: [0.09, 0.07, 0.09], segments: 5, rings: 3)
                }
                crown.addChild(ModelEntity(mesh: fl.mesh(name: "blossom"), materials: [Mat.matte(flowerColour, roughness: 0.8)]))
            }
        }
        // Wind: a slow sway, different for every tree.
        let phase = Float(rng.next(0, 6.28))
        var t: Float = 0
        let base = crown.orientation
        scene.updaters.append { dt, eye in
            guard simd_distance(eye, p) < 60 else { return }
            t += dt
            crown.orientation = base * simd_quatf(angle: 0.025 * sin(t * 0.9 + phase), axis: normalize([cos(phase), 0, sin(phase)]))
        }
        return crown
    }

    /// A scatter of flowers (a small head on a stem) at the given points.
    static func flowers(in scene: MuseumScene, points: [SIMD2<Float>], colour: UInt32, height: ClosedRange<Float>,
                        headRadius: Float = 0.06, seed: UInt64, name: String) {
        var heads = MeshBuilder(), stems = MeshBuilder()
        var rng = SplitMix(seed: seed)
        for p in points {
            let h = Float(rng.next(Double(height.lowerBound), Double(height.upperBound)))
            let lean = SIMD3<Float>(Float(rng.next(-0.08, 0.08)), 0, Float(rng.next(-0.08, 0.08)))
            let top = SIMD3<Float>(p.x, h, p.y) + lean
            stems.stem(from: [p.x, 0, p.y], to: top, r0: 0.008, r1: 0.006, segments: 4)
            heads.ellipsoid(center: top, radii: [headRadius, headRadius * 0.7, headRadius], segments: 6, rings: 4)
        }
        scene.add(stems, Mat.matte(0x6F8456, roughness: 0.9), name: name + " stems")
        scene.add(heads, Mat.matte(colour, roughness: 0.7), name: name)
    }
}

extension MuseumScene {
    /// The ground round the whole museum: a meadow to the horizon, just under the floors.
    func buildOutdoorGround() {
        // Holes where something sinks below the ground: the Chinese pond, the lily pond's stair
        // opening and the Atrium's car shaft (their own floors cover the rest).
        let holes: [(x0: Float, x1: Float, z0: Float, z1: Float)] = [(-4, 5, 19, 23), (-91, -82, -2, 2), (51, 57, -3, 3)]
        let bx0: Float = -110, bx1: Float = 80, bz0: Float = -40, bz1: Float = 45
        var g = MeshBuilder()
        g.floorCells(x0: bx0, x1: bx1, z0: bz0, z1: bz1, y: -0.03, cell: 1, tile: 2) { p in
            !holes.contains { p.x > $0.x0 && p.x < $0.x1 && p.y > $0.z0 && p.y < $0.z1 }
        }
        g.floorRect(x0: -400, x1: 400, z0: -400, z1: bz0, y: -0.03, up: true, tile: 2)
        g.floorRect(x0: -400, x1: 400, z0: bz1, z1: 400, y: -0.03, up: true, tile: 2)
        g.floorRect(x0: -400, x1: bx0, z0: bz0, z1: bz1, y: -0.03, up: true, tile: 2)
        g.floorRect(x0: bx1, x1: 400, z0: bz0, z1: bz1, y: -0.03, up: true, tile: 2)
        add(g, Mat.textured(Garden.grassTexture(base: 0x8A9E6A), roughness: 0.95), name: "Ground")
    }
}
