import CoreGraphics
import Foundation
import RealityKit
import simd

/// The Hall of Light, east of the Rotunda (board: HallOfLight). A glass hall 30 × 9 m
/// (x 11 → 41, y ±4.5) under a shallow steel-and-glass lattice vault (R 5.3 m about h 2.2,
/// crown 7.5 m), between a meadow of birches (north) and a flowering orchard (south).
/// One photograph per 3 m bay on each glass wall, walking forward in time; four stereo stones.
enum HallOfLightPlan {
    static let x0: Float = 10.26            // where the glass meets the Rotunda drum (r 11.2) at |y| 4.5
    static let x1: Float = 39.90            // where it meets the Atrium drum (r 14.8 about (54, 0))
    static let halfWidth: Float = 4.5
    static let wallHeight: Float = 5.0
    static let vault = (radius: Float(5.3), centreY: Float(2.2), halfAngle: Float(58.1 * .pi / 180))
    static let posts: [Float] = [11, 14, 17, 20, 23, 26, 29, 32, 35, 38]
    static let stones: [SIMD2<Float>] = [[17, -2.4], [23, 2.4], [29, -2.4], [35, 2.4]]
    static let birches: [SIMD2<Float>] = [[14, -8], [16.5, -11.5], [21, -7.2], [24.5, -11.2], [28, -8.3], [32, -11.6],
                                          [35.5, -7.6], [38.5, -11]]
    static let orchard: [SIMD2<Float>] = [[14.5, 7.8], [20.5, 7.8], [26.5, 7.8], [32.5, 7.8], [38, 7.8],
                                          [16, 11.6], [22, 11.6], [28, 11.6], [34, 11.6], [39.5, 11.6]]

    /// The prints, bay by bay (x centre), north wall then south wall. Bay 10 is shifted to
    /// 38.95 so it clears the Atrium's 0.8 m wall. Ids are image files / catalogue entries.
    struct Print { let bay: Int; let x: Float; let north: Bool; let id: String; let autochrome: Bool }
    static var prints: [Print] {
        let xs: [Float] = [12.5, 15.5, 18.5, 21.5, 24.5, 27.5, 30.5, 33.5, 36.5, 38.95]
        var out: [Print] = []
        for (i, pair) in HallOfLightHang.order.enumerated() {
            out.append(Print(bay: i + 1, x: xs[i], north: true, id: pair.north, autochrome: i >= 8))
            out.append(Print(bay: i + 1, x: xs[i], north: false, id: pair.south, autochrome: i >= 8))
        }
        return out
    }
}

extension MuseumScene {
    func buildHallOfLight() {
        typealias H = HallOfLightPlan
        let hw = H.halfWidth
        let steel = Mat.metal(0x3B3A36, roughness: 0.45)
        let glass = Mat.glass(0xA9CFCB, opacity: 0.14)

        // Floor and the stone plinths the glass stands on.
        var f = MeshBuilder()
        f.floorRect(x0: H.x0 - 0.3, x1: H.x1 + 0.3, z0: -hw, z1: hw, y: -0.003, up: true, tile: 2.4)
        add(f, Mat.polishedStone(.travertine, tint: 0xF4EEE4, seed: 6),
            name: "Hall of Light floor")
        var plinth = MeshBuilder()
        for side: Float in [-1, 1] {
            plinth.box(min: [H.x0, 0, side * hw - 0.12], max: [H.x1, 0.3, side * hw + 0.12])
        }
        add(plinth, Mat.matte(0xCFC6B8), name: "Hall of Light plinths")

        // Glass walls, posts, eaves.
        var g = MeshBuilder()
        for side: Float in [-1, 1] {
            let z = side * hw
            g.quad([H.x0, 0.3, z], [H.x1, 0.3, z], [H.x1, H.wallHeight, z], [H.x0, H.wallHeight, z], normal: [0, 0, -side])
            collision.add([H.x0, z], [H.x1, z], occludes: false, y1: 8)
        }
        var frame = MeshBuilder()
        for x in H.posts {
            for side: Float in [-1, 1] {
                frame.box(min: [x - 0.045, 0, side * hw - 0.075], max: [x + 0.045, H.wallHeight, side * hw + 0.075])
            }
        }
        for side: Float in [-1, 1] {
            frame.box(min: [H.x0, H.wallHeight - 0.08, side * hw - 0.06], max: [H.x1, H.wallHeight + 0.02, side * hw + 0.06])
        }

        // The lattice vault: glass on a circular arc, with arches every 1.5 m, nine longitudinal
        // members and a diagonal in each panel.
        let V = H.vault
        func arcPoint(_ x: Float, _ theta: Float) -> SIMD3<Float> {
            [x, V.centreY + V.radius * cos(theta), V.radius * sin(theta)]
        }
        let thetas = (0...8).map { -V.halfAngle + 2 * V.halfAngle * Float($0) / 8 }
        let fine = (0...32).map { -V.halfAngle + 2 * V.halfAngle * Float($0) / 32 }
        for (t0, t1) in zip(fine, fine.dropFirst()) {
            let n0 = -normalize(arcPoint(0, t0) - [0, V.centreY, 0]), n1 = -normalize(arcPoint(0, t1) - [0, V.centreY, 0])
            g.quad(arcPoint(H.x0, t0), arcPoint(H.x1, t0), arcPoint(H.x1, t1), arcPoint(H.x0, t1),
                   normals: [0, n0.y, n0.z], [0, n0.y, n0.z], [0, n1.y, n1.z], [0, n1.y, n1.z])
        }
        add(g, glass, name: "Hall of Light glass")
        func member(_ a: SIMD3<Float>, _ b: SIMD3<Float>, _ w: Float = 0.05) {
            let d = b - a, len = simd_length(d)
            guard len > 1e-3 else { return }
            var local = MeshBuilder()
            local.box(min: [-w / 2, -w / 2, 0], max: [w / 2, w / 2, len])
            let q = simd_quatf(from: [0, 0, 1], to: d / len)
            frame.append(local, transform: { q.act($0) + a }, normalTransform: { q.act($0) })
        }
        var arches: [Float] = []
        var x: Float = 11
        while x <= H.x1 { arches.append(x); x += 1.5 }
        for ax in arches {
            for (t0, t1) in zip(fine, fine.dropFirst()) { member(arcPoint(ax, t0), arcPoint(ax, t1)) }
        }
        for t in thetas { member(arcPoint(H.x0, t), arcPoint(H.x1, t)) }
        for (a0, a1) in zip(arches, arches.dropFirst()) {
            for (t0, t1) in zip(thetas, thetas.dropFirst()) {
                member(arcPoint(a0, t0), arcPoint(a1, t1), 0.03)
                member(arcPoint(a0, t1), arcPoint(a1, t0), 0.03)
            }
        }
        add(frame, steel, name: "Hall of Light lattice")

        // Daylight in the hall (the glass lets the sky in; two soft spots stand for it).
        for lx: Float in [18, 32] {
            addLight(spot(at: [lx, 7.2, 0], looking: [lx, 0, 0], colour: 0xFFF8EE, intensity: 110_000, inner: 60, outer: 89, radius: 20))
        }

        // The photographs, one per bay on each wall, on slim bronze-edged panels (autochromes
        // set into the glass itself, lit by the garden behind).
        for p in H.prints {
            let side: Float = p.north ? -1 : 1
            let facing = SIMD3<Float>(0, 0, -side)
            let z = side * (p.autochrome ? (hw - 0.02) : 4.38)
            let size = HallOfLightHang.imageSize(p.id, maxW: 0.78, maxH: 0.68)
            if !p.autochrome {
                // Pale mount 1.0 × 0.9 m with a bronze edge; the print centred on it.
                var mount = MeshBuilder()
                mount.box(min: [p.x - 0.5, 1.15, z - side * 0.03], max: [p.x + 0.5, 2.05, z])
                add(mount, Mat.matte(0xF2EDE3, roughness: 0.8), name: "Mount \(p.id)")
                var edge = MeshBuilder()
                for (a, b) in [(SIMD3<Float>(p.x - 0.51, 1.14, 0), SIMD3<Float>(p.x + 0.51, 1.16, 0)),
                               ([p.x - 0.51, 2.04, 0], [p.x + 0.51, 2.06, 0]),
                               ([p.x - 0.51, 1.14, 0], [p.x - 0.49, 2.06, 0]), ([p.x + 0.49, 1.14, 0], [p.x + 0.51, 2.06, 0])] {
                    edge.box(min: [a.x, a.y, z - side * 0.035], max: [b.x, b.y, z + side * 0.005])
                }
                add(edge, Mat.metal(0x5A4632, roughness: 0.4), name: "Edge \(p.id)")
            }
            hangFramed(id: "hol-\(p.bay)-\(p.north ? "n" : "s")", artworkID: p.id, image: p.id,
                       wall: [p.x, 1.6, p.autochrome ? z : z - side * 0.034], facing: facing, width: size.x, height: size.y,
                       frame: p.autochrome ? (0.03, 0.02, 0x7E5C25, true) : (0, 0.002, 0, false),
                       tint: p.autochrome ? 0xFFFFFF : 0xF4F1EA)
        }

        // The four stereo stones, each carrying an 1850s stereograph that "moves" in 3D:
        // the left and right views alternate, so the depth shows as a gentle rocking.
        var stones = MeshBuilder()
        for (i, c) in H.stones.enumerated() {
            stones.box(min: [c.x - 0.35, 0, c.y - 0.35], max: [c.x + 0.35, 1.0, c.y + 0.35])
            collision.addRect(x0: c.x - 0.35, x1: c.x + 0.35, z0: c.y - 0.35, z1: c.y + 0.35, occludes: false, y1: 1.0)
            let faceSouth = c.y < 0
            let f = SIMD3<Float>(0, 0, faceSouth ? 1 : -1)
            buildStereograph(id: "stereo-\(i + 1)", centre: [c.x, 1.32, c.y], facing: f)
        }
        add(stones, Mat.matte(0xCFC6B8, roughness: 0.7), name: "Stereo stones")

        buildHallGardens()
    }

    /// A stereo card on a tilted bronze reader above a stone; the two halves alternate.
    func buildStereograph(id: String, centre c: SIMD3<Float>, facing f: SIMD3<Float>) {
        let holder = Entity()
        holder.name = id
        holder.position = c
        holder.orientation = simd_quatf(angle: atan2(f.x, f.z), axis: [0, 1, 0]) * simd_quatf(angle: -0.5, axis: [1, 0, 0])
        building.addChild(holder)
        var plate = MeshBuilder()
        plate.box(min: [-0.34, -0.2, -0.03], max: [0.34, 0.2, 0])
        holder.addChild(ModelEntity(mesh: plate.mesh(name: "reader"), materials: [Mat.metal(0x5A4632, roughness: 0.4)]))
        // Two views of the card, each showing one half; they swap four times a second.
        var views: [ModelEntity] = []
        for half in 0..<2 {
            var q = MeshBuilder()
            let u0: Float = half == 0 ? 0 : 0.5, u1: Float = half == 0 ? 0.5 : 1
            q.quad([-0.3, -0.16, 0.004], [0.3, -0.16, 0.004], [0.3, 0.16, 0.004], [-0.3, 0.16, 0.004], normal: [0, 0, 1],
                   uv: ([u0, 0], [u1, 0], [u1, 1], [u0, 1]))
            let e = ModelEntity(mesh: q.mesh(name: "\(id)-\(half)"), materials: [placeholder])
            holder.addChild(e)
            addImageSlot(e, image: id, maxPixels: 1600, at: c)
            views.append(e)
        }
        var t: Float = 0
        updaters.append { dt, eye in
            guard simd_distance(eye, c) < 12 else { return }
            t += dt
            let right = Int(t * 4) % 2 == 1
            views[0].isEnabled = !right
            views[1].isEnabled = right
        }
        targets.append(PickTarget(artworkID: id, detail: "A stereograph: the two views alternate, so you see its depth.",
                                  hit: { o, d in Self.rayBox(o, d, min: c - [0.4, 0.3, 0.4], max: c + [0.4, 0.3, 0.4]) }))
    }
}

/// Which photograph hangs where, bay by bay (north wall, south wall), walking forward in time.
/// Board: Talbot in bay 1, Atkins's cyanotypes in bay 2, Nadar and Muybridge among the albumen
/// prints of bays 3–8, Lumière autochromes in bays 9–10. The unnamed bays are filled with
/// landmark photographs of the same years (see README).
enum HallOfLightHang {
    static var order: [(north: String, south: String)] = [
        ("photo-talbot-leaf", "photo-talbot-open-door"),                               // B1 · Talbot, c.1840–44
        ("photo-atkins-algae", "photo-atkins-algae-2"),                                // B2 · cyanotypes, 1843
        ("photo-hill-adamson-newhaven-fishwives", "photo-fenton-valley-shadow-of-death"), // B3 · 1840s–1855
        ("photo-nadar-bernhardt", "photo-le-gray-great-wave"),                        // B4 · Nadar c.1864; Le Gray 1857
        ("photo-marville-rue-traversine", "photo-cameron-julia-jackson"),             // B5 · 1860s
        ("photo-muybridge-horse", "photo-osullivan-canyon-de-chelly"),                // B6 · Muybridge 1878; 1873
        ("photo-marey-bird-in-flight", "photo-atget-organ-grinder"),                  // B7 · 1880s–1890s
        ("photo-stieglitz-steerage", "photo-hine-sadie-pfeifer"),                     // B8 · 1907–08
        ("photo-lumiere-autochrome", "photo-lumiere-autochrome-4"),                   // B9 · autochromes, c.1907–12
        ("photo-lumiere-autochrome-3", "photo-lumiere-autochrome-2"),                 // B10 · autochromes, 1913–14
    ]

    /// Fits an image inside maxW × maxH at its true proportions (read from the file's header).
    static func imageSize(_ id: String, maxW: Float, maxH: Float) -> SIMD2<Float> {
        let aspect = ImageInfo.aspect(id) ?? 1.3
        return aspect > maxW / maxH ? [maxW, maxW / aspect] : [maxH * aspect, maxH]
    }
}

extension MuseumScene {
    func buildHallGardens() {
        typealias H = HallOfLightPlan
        let season = Season.now(latitude: observer.latitude)
        // Ground of both gardens (the meadow to the north is a little lusher).
        buildOutdoorGround()
        // Hedges on the long outer sides.
        var hedge = MeshBuilder()
        for side: Float in [-1, 1] {
            hedge.box(min: [11, 0, side * 14 - 0.2], max: [41, 1.5, side * 14 + 0.2])
        }
        add(hedge, Mat.matte(season == .winter ? 0x55664A : 0x5F7A4E, roughness: 0.95), name: "Hedges")

        // North: birches with their white trunks.
        let birchCrown: UInt32? = [.spring: 0xA8BF7E, .summer: 0x8DA56A, .autumn: 0xD8B650, .winter: nil][season] ?? nil
        for (i, p) in H.birches.enumerated() {
            Garden.tree(in: self, at: [p.x, 0, p.y], trunkHeight: 6.0, trunkRadius: 0.1, trunkColour: 0xD8D2C4,
                        crownCentre: 6.8, crownRadii: [1.9, 2.5, 1.9], crownColour: birchCrown, blobs: 6,
                        seed: UInt64(100 + i), branches: season == .winter ? 7 : 3)
        }
        // South: the flowering orchard.
        let fruitCrown: UInt32? = [.spring: 0xEBCFCB, .summer: 0x7F9A5E, .autumn: 0xB79A55, .winter: nil][season] ?? nil
        for (i, p) in H.orchard.enumerated() {
            Garden.tree(in: self, at: [p.x, 0, p.y], trunkHeight: 3.6, trunkRadius: 0.1, trunkColour: 0x6B5A45,
                        crownCentre: 4.9, crownRadii: [2.0, 1.8, 2.0], crownColour: fruitCrown, blobs: 5,
                        seed: UInt64(200 + i), branches: 5,
                        flowerColour: season == .spring ? 0xFBF4F0 : (season == .autumn ? 0xC8563A : nil))
        }
        // Meadow flowers and spring bulbs, from the plotted points; seed heads in winter.
        for bed in HallOfLightPlanting.beds {
            let isMeadow = bed.garden == "north"
            switch (season, isMeadow) {
            case (.winter, true):
                Garden.flowers(in: self, points: bed.points, colour: 0x8B7355, height: 0.5...0.9, headRadius: 0.04,
                               seed: UInt64(bed.colour), name: "Seed heads")
            case (.winter, false), (.autumn, false), (.summer, false):
                continue   // bulbs are over
            case (.autumn, true):
                Garden.flowers(in: self, points: Array(bed.points.prefix(bed.points.count / 2)), colour: bed.colour,
                               height: 0.4...0.8, seed: UInt64(bed.colour), name: "Meadow flowers")
            default:
                Garden.flowers(in: self, points: bed.points, colour: bed.colour,
                               height: isMeadow ? 0.4...0.9 : 0.2...0.35, headRadius: isMeadow ? 0.06 : 0.07,
                               seed: UInt64(bed.colour), name: isMeadow ? "Meadow flowers" : "Bulbs")
            }
        }
    }
}
