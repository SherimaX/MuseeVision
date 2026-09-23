import CoreGraphics
import Foundation
import RealityKit
import simd

/// The Chinese Wing, 四時園 the Garden of the Four Seasons (board: ChineseWing). South of the
/// Rotunda: a vestibule with the solar-term stele, a moon gate Ø 3.6 m, an open moon terrace,
/// and a whitewashed cloister on three sides of an 11 m garden (pond, Taihu rock, plum,
/// osmanthus, bamboo). The garden keeps the season; the moon lies in the pond.
enum ChinesePlan {
    static let x0: Float = -10, x1: Float = 10, z0: Float = 13.5, z1: Float = 33.5
    static let wallHeight: Float = 6
    static let wall: Float = 0.6
    static let vestibule = (halfWidth: Float(2.4), z0: Float(10.75), z1: Float(12.9), ceiling: Float(4.4))
    static let moonGate = (radius: Float(1.8), centreHeight: Float(1.55))
    static let columns: [SIMD2<Float>] = {
        var c: [SIMD2<Float>] = []
        for y: Float in [15.8, 18.0, 20.2, 22.4, 24.6, 26.8, 29.0] { c.append([-5.5, y]); c.append([5.5, y]) }
        for x: Float in [-3.3, -1.1, 1.1, 3.3] { c.append([x, 29.0]) }
        return c
    }()
    /// Roof profile (distance from the outer wall → top and underside heights).
    static let roofD: [Float] = [0, 0.32, 0.64, 0.96, 1.28, 1.60, 1.91, 2.23, 2.55, 2.87, 3.19, 3.51, 3.83, 4.15, 4.47, 4.78, 5.10]
    static let roofTop: [Float] = [5.40, 5.22, 5.03, 4.85, 4.68, 4.51, 4.36, 4.21, 4.07, 3.95, 3.84, 3.75, 3.67, 3.61, 3.58, 3.57, 3.60]
    static let roofUnder: [Float] = [5.04, 4.87, 4.69, 4.52, 4.36, 4.20, 4.06, 3.92, 3.80, 3.68, 3.58, 3.50, 3.43, 3.38, 3.36, 3.36, 3.40]
    static func interp(_ d: Float, _ ys: [Float]) -> Float {
        let dd = simd_clamp(d, 0, 5.10)
        for i in 0..<(roofD.count - 1) where dd <= roofD[i + 1] {
            let t = (dd - roofD[i]) / (roofD[i + 1] - roofD[i])
            return ys[i] + (ys[i + 1] - ys[i]) * t
        }
        return ys.last!
    }
    static let pond: [SIMD2<Float>] = [[3.66, 20.90], [3.23, 21.21], [2.66, 21.53], [2.47, 21.90], [2.22, 22.22], [1.56, 22.34], [0.76, 22.39],
        [0.00, 22.48], [-0.75, 22.50], [-1.47, 22.31], [-2.10, 22.05], [-2.56, 21.83], [-2.92, 21.57], [-3.26, 21.25], [-3.47, 20.90],
        [-3.50, 20.54], [-3.30, 20.20], [-2.74, 19.93], [-2.03, 19.72], [-1.35, 19.59], [-0.65, 19.52], [0.05, 19.44], [0.73, 19.41],
        [1.39, 19.42], [2.00, 19.53], [2.53, 19.83], [2.99, 20.20], [3.45, 20.55]]
    static let edgingStones: [SIMD2<Float>] = [[3.66, 20.90], [2.66, 21.53], [2.22, 22.22], [0.76, 22.39], [-0.75, 22.50], [-2.10, 22.05],
        [-2.92, 21.57], [-3.47, 20.90], [-3.30, 20.20], [-2.03, 19.72], [-0.65, 19.52], [0.73, 19.41], [2.00, 19.53], [2.99, 20.20]]
    static let lotus: [(p: SIMD2<Float>, d: Float, flower: Float)] = [([-1.90, 20.60], 0.76, 0.30), ([-1.10, 21.50], 0.64, 0),
        ([1.30, 20.40], 0.80, 0), ([2.00, 21.30], 0.60, 0.24), ([0.90, 21.60], 0.52, 0)]
    static let rock = SIMD2<Float>(0.08, 25.26)
    static let rockSilhouette: [SIMD2<Float>] = [[24.30, 0], [24.50, 0.93], [24.36, 1.77], [24.70, 2.52], [24.50, 3.27], [24.90, 4.08],
        [25.30, 3.78], [25.62, 4.20], [26.02, 3.36], [25.78, 2.60], [26.20, 1.85], [25.98, 0.93], [26.30, 0]]
    static let plum = SIMD2<Float>(3.60, 20.10)
    static let osmanthus = SIMD2<Float>(-3.49, 26.70)
    static let bambooNW: [SIMD2<Float>] = [[-3.09, 14.18], [-2.83, 14.05], [-2.72, 13.82], [-5.10, 13.80], [-4.17, 13.93], [-3.68, 13.95],
        [-3.00, 14.12], [-2.73, 13.82], [-2.55, 13.83], [-3.56, 14.15], [-3.49, 13.76], [-3.17, 14.20], [-2.66, 14.01], [-3.48, 14.03],
        [-3.38, 14.05], [-3.79, 14.03]]
    static let bambooNE: [SIMD2<Float>] = [[3.99, 13.91], [4.00, 13.80], [4.62, 14.11], [4.20, 13.78], [3.85, 13.86], [3.15, 14.17],
        [5.09, 13.72], [4.74, 14.00], [3.49, 13.84], [4.25, 13.93], [4.28, 14.03], [2.85, 14.08], [5.05, 14.18], [4.10, 13.72],
        [2.51, 13.77], [4.95, 13.85]]
    /// The SE clump, moved 0.5 m north-west so it clears the eaves (see README).
    static let bambooSE: [SIMD2<Float>] = [[5.09, 28.55], [4.78, 27.77], [4.30, 28.33], [4.77, 28.46], [4.67, 28.48], [4.57, 28.52],
        [5.03, 27.98], [4.84, 28.35], [4.49, 28.18], [5.10, 27.77], [5.10, 28.36], [5.15, 27.87], [4.39, 28.52], [5.10, 28.03],
        [4.40, 27.68], [4.93, 27.81], [5.25, 28.22], [4.50, 28.32], [4.66, 28.70], [5.21, 28.12], [4.95, 28.38], [5.10, 28.77]]
        .map { $0 - [0.4, 0.4] }
    static let scrolls: [(id: String, x: Float, w: Float, h: Float)] = [
        ("chinese-fan-kuan-travelers", -7.5, 1.033, 2.063), ("chinese-guo-xi-early-spring", -4.5, 1.081, 1.583),
        ("chinese-li-tang-wind-in-pines", -1.5, 1.398, 1.887), ("chinese-wang-meng-qingbian", 1.5, 0.422, 1.406),
        ("chinese-ni-zan-rongxi", 4.5, 0.355, 0.747), ("chinese-shen-zhou-lushan", 7.5, 0.981, 1.938),
    ]
    static let ceramicsX: Float = -8.2
    static let ceramics: [(id: String, y: Float)] = [("chinese-sancai-horse", 18.50), ("chinese-ru-basin", 21.25),
        ("chinese-meiping", 24.00), ("chinese-chicken-cup", 26.75), ("chinese-peachbloom", 29.50)]
    static let handscroll = (x0: Float(9.0), x1: Float(9.9), y0: Float(16.8), y1: Float(30.2), silkX: Float(9.45),
                             silkW: Float(0.515), start: Float(29.46), end: Float(17.54), window: Float(1.5))
}

extension MuseumScene {
    func buildChineseWing() {
        typealias C = ChinesePlan
        let term = Ephemeris.solarTerm(Self.now())
        let t = observer.latitude < 0 ? (term + 12) % 24 : term
        let whitewash = Mat.matte(0xFBF9F4, roughness: 0.92)
        let timber = Mat.matte(0x4A3A2C, roughness: 0.7)
        let stone = Mat.matte(0xCFC6B8, roughness: 0.75)
        let paving = Mat.textured(Textures.resource(Textures.stoneSlab(base: 0xEFE8DB, joint: 0xE4DBCB)), roughness: 0.7)

        // Vestibule: 4.8 m wide, flat ceiling at 4.4 m; a plaster panel closes the Rotunda's arch above it.
        let V = C.vestibule
        var vest = MeshBuilder()
        for side: Float in [-1, 1] {
            vest.quad([side * V.halfWidth, 0, V.z0], [side * V.halfWidth, 0, V.z1], [side * V.halfWidth, V.ceiling, V.z1],
                      [side * V.halfWidth, V.ceiling, V.z0], normal: [-side, 0, 0])
            collision.add([side * V.halfWidth, V.z0 - 0.4], [side * V.halfWidth, V.z1], y1: V.ceiling)
        }
        vest.floorRect(x0: -V.halfWidth, x1: V.halfWidth, z0: V.z0, z1: V.z1, y: V.ceiling, up: false)
        vest.quad([-2.1, V.ceiling, 11.05], [2.1, V.ceiling, 11.05], [2.1, 6.7, 11.05], [-2.1, 6.7, 11.05], normal: [0, 0, -1])
        add(vest, Mat.matte(0xECE7DE, roughness: 0.9), name: "Vestibule")
        var vf = MeshBuilder()
        vf.floorRect(x0: -V.halfWidth, x1: V.halfWidth, z0: 10.0, z1: C.z0, y: -0.002, up: true, tile: 1)
        add(vf, paving, name: "Vestibule floor")

        // The solar-term stele on the west wall, facing east.
        var stele = MeshBuilder()
        stele.box(min: [-2.35, 0, 11.5], max: [-2.05, 1.5, 12.3])
        add(stele, stone, name: "Solar-term stele")
        collision.addRect(x0: -2.35, x1: -2.05, z0: 11.5, z1: 12.3, occludes: false, y1: 1.5)
        let inscription = ModelEntity(mesh: .generatePlane(width: 0.72, height: 1.36),
                                      materials: [Mat.glow(Textures.resource(Textures.steleInscription(term: term)), tint: 0xE6E0D6)])
        inscription.name = "Stele inscription"
        inscription.position = [-2.045, 0.75, 11.9]
        inscription.orientation = simd_quatf(angle: .pi / 2, axis: [0, 1, 0])
        building.addChild(inscription)
        let termInfo = Ephemeris.solarTerms[term]
        targets.append(PickTarget(artworkID: "chinese-solar-term-stele",
                                  detail: "Today: \(termInfo.hanzi) \(termInfo.pinyin) · \(termInfo.english)",
                                  hit: { o, d in Self.rayBox(o, d, min: [-2.35, 0, 11.5], max: [-2.05, 1.5, 12.3]) }))

        // Perimeter walls, whitewashed, with the moon gate in the north wall.
        let loop: [SIMD2<Float>] = [[C.x0, C.z0], [C.x1, C.z0], [C.x1, C.z1], [C.x0, C.z1], [C.x0, C.z0 - 0.01]]
        let walls = WallRun(points: loop, inside: .right, height: C.wallHeight, thickness: C.wall,
                            openings: [WallOpening(center: 10, width: 2 * C.moonGate.radius, spring: C.moonGate.centreHeight, round: true)])
        var w = MeshBuilder()
        w.wall(walls)
        add(w, whitewash, name: "Chinese Wing walls")
        collision.add(run: walls)
        // Tiled coping on the wall tops.
        var coping = MeshBuilder()
        let copes: [(SIMD3<Float>, SIMD3<Float>)] = [
            ([C.x0 - 0.75, 6, C.z0 - 0.75], [C.x1 + 0.75, 6.3, C.z0 + 0.15]), ([C.x0 - 0.75, 6, C.z1 - 0.15], [C.x1 + 0.75, 6.3, C.z1 + 0.75]),
            ([C.x0 - 0.75, 6, C.z0 - 0.75], [C.x0 + 0.15, 6.3, C.z1 + 0.75]), ([C.x1 - 0.15, 6, C.z0 - 0.75], [C.x1 + 0.75, 6.3, C.z1 + 0.75]),
        ]
        for (a, b) in copes { coping.box(min: a, max: b) }
        add(coping, Mat.matte(0x77726B, roughness: 0.8), name: "Coping")
        var ridge = MeshBuilder()
        ridge.box(min: [C.x0 - 0.45, 6.3, C.z0 - 0.45], max: [C.x1 + 0.45, 6.45, C.z0 - 0.15])
        ridge.box(min: [C.x0 - 0.45, 6.3, C.z1 + 0.15], max: [C.x1 + 0.45, 6.45, C.z1 + 0.45])
        ridge.box(min: [C.x0 - 0.45, 6.3, C.z0 - 0.45], max: [C.x0 - 0.15, 6.45, C.z1 + 0.45])
        ridge.box(min: [C.x1 + 0.15, 6.3, C.z0 - 0.45], max: [C.x1 + 0.45, 6.45, C.z1 + 0.45])
        add(ridge, Mat.matte(0x1E1C19), name: "Coping ridge")
        // The gate's grey surround on the vestibule face, and the plaque 四時園 above it.
        var surround = MeshBuilder()
        for i in 0..<64 {
            let a0 = 2 * Float.pi * Float(i) / 64, a1 = 2 * Float.pi * Float(i + 1) / 64
            let p = { (r: Float, a: Float) -> SIMD3<Float> in [r * cos(a), max(0, C.moonGate.centreHeight + r * sin(a)), 12.89] }
            surround.quad(p(1.8, a0), p(1.8, a1), p(1.92, a1), p(1.92, a0), normal: [0, 0, -1])
        }
        add(surround, Mat.matte(0xB8B2A6), name: "Moon gate surround")
        let plaque = ModelEntity(mesh: .generatePlane(width: 1.61, height: 0.53),
                                 materials: [Mat.glow(Textures.resource(Textures.plaque()), tint: 0xF0EBE2)])
        plaque.name = "Plaque"
        plaque.position = [0, 4.0, 12.88]
        plaque.orientation = simd_quatf(angle: .pi, axis: [0, 1, 0])
        building.addChild(plaque)
        targets.append(PickTarget(artworkID: "chinese-plaque-si-shi-yuan", hit: { o, d in
            Self.rayBox(o, d, min: [-0.8, 3.73, 12.8], max: [0.8, 4.27, 12.95])
        }))

        // Floors: paving in the walks and on the terrace, gravel in the garden.
        var pave = MeshBuilder()
        pave.floorCells(x0: C.x0, x1: C.x1, z0: C.z0, z1: C.z1, y: 0, cell: 0.5, tile: 1) { p in
            !(p.x > -5.5 && p.x < 5.5 && p.y > 18 && p.y < 29)
        }
        add(pave, paving, name: "Cloister paving")
        var gravel = MeshBuilder()
        gravel.floorCells(x0: -5.5, x1: 5.5, z0: 18, z1: 29, y: 0, cell: 0.25, tile: 1) { p in
            !FloorWorld.Shape.polygon(C.pond).contains(p)
        }
        add(gravel, Mat.textured(Textures.resource(Textures.gravel()), roughness: 0.95), name: "Gravel")

        buildChineseCloister(timber: timber, stone: stone)
        buildChineseGarden(term: t, stone: stone)
        buildChineseExhibits(timber: timber, stone: stone)

        // Soft light under the eaves and on the scrolls; the garden is open to the sky.
        addLight(spot(at: [0, 5.5, 31.5], looking: [0, 1.2, 33.3], colour: 0xFFE9C8, intensity: 45_000, inner: 55, outer: 85, radius: 10))
        addLight(spot(at: [-7.5, 5.0, 23], looking: [-8.2, 0, 23], colour: 0xFFF1DE, intensity: 40_000, inner: 60, outer: 88, radius: 14))
        addLight(spot(at: [7.7, 5.0, 23], looking: [9.4, 0, 23], colour: 0xFFF1DE, intensity: 40_000, inner: 60, outer: 88, radius: 14))
        addLight(spot(at: [0, 9, 22], looking: [0, 0, 23], colour: 0xFFF8EE, intensity: 90_000, inner: 60, outer: 89, radius: 20))
    }

    // MARK: Cloister

    private func buildChineseCloister(timber: RealityKit.Material, stone: RealityKit.Material) {
        typealias C = ChinesePlan
        var cols = MeshBuilder()
        for c in C.columns {
            cols.stem(from: [c.x, 0, c.y], to: [c.x, 3.0, c.y], r0: 0.15, r1: 0.15, segments: 12)
            collision.addCircle(c, r: 0.16, segments: 10, occludes: false, y1: 3.3)
        }
        // Eave beams on the column lines.
        cols.box(min: [-5.625, 3.0, C.z0], max: [-5.375, 3.3, 29.125])
        cols.box(min: [5.375, 3.0, C.z0], max: [5.625, 3.3, 29.125])
        cols.box(min: [-5.5, 3.0, 28.875], max: [5.5, 3.3, 29.125])
        cols.box(min: [C.x0, 3.0, 28.875], max: [-5.5, 3.3, 29.125])
        cols.box(min: [5.5, 3.0, 28.875], max: [C.x1, 3.3, 29.125])
        add(cols, timber, name: "Colonnade")
        // Seat rails between the columns (the first bay left open on both sides for the route).
        var rails = MeshBuilder()
        for x: Float in [-5.5, 5.5] {
            rails.box(min: [x - 0.06, 0.3, 18.0], max: [x + 0.06, 0.45, 29.0])
            collision.add([x, 18.0], [x, 29.0], radius: 0.08, occludes: false, y1: 0.45)
        }
        rails.box(min: [-5.5, 0.3, 28.94], max: [5.5, 0.45, 29.06])
        collision.add([-5.5, 29.0], [5.5, 29.0], radius: 0.08, occludes: false, y1: 0.45)
        for i in 0..<14 {
            let y = 18.4 + Float(i) * 0.8
            for x: Float in [-5.5, 5.5] { rails.box(min: [x - 0.04, 0, y - 0.04], max: [x + 0.04, 0.3, y + 0.04]) }
        }
        add(rails, Mat.matte(0x9C8A74, roughness: 0.7), name: "Seat rails")

        // The single-slope curved tile roofs of the three walks, as one surface: at any point the
        // nearest outer wall of a walk sets the height, so they meet in valleys at the corners.
        func dist(_ p: SIMD2<Float>) -> Float? {
            var d: [Float] = []
            if p.x < -4.9 { d.append(p.x - C.x0) }
            if p.x > 4.9 { d.append(C.x1 - p.x) }
            if p.y > 28.4 { d.append(C.z1 - p.y) }
            return d.min()
        }
        var top = MeshBuilder(), under = MeshBuilder()
        let cell: Float = 0.32
        var x = C.x0
        while x < C.x1 - 1e-3 {
            var z = C.z0
            while z < C.z1 - 1e-3 {
                let x1 = min(C.x1, x + cell), z1 = min(C.z1, z + cell)
                let corners: [SIMD2<Float>] = [[x, z], [x1, z], [x1, z1], [x, z1]]
                if let _ = dist([(x + x1) / 2, (z + z1) / 2]), corners.allSatisfy({ dist($0) != nil || true }) {
                    let dT = corners.map { C.interp(dist($0) ?? 5.1, C.roofTop) }
                    let dU = corners.map { C.interp(dist($0) ?? 5.1, C.roofUnder) }
                    let pT = zip(corners, dT).map { SIMD3<Float>($0.x, $1, $0.y) }
                    let pU = zip(corners, dU).map { SIMD3<Float>($0.x, $1, $0.y) }
                    var n = normalize(cross(pT[3] - pT[0], pT[1] - pT[0]))
                    if n.y < 0 { n = -n }
                    top.quad(pT[0], pT[1], pT[2], pT[3], normal: n,
                             uv: ([x / 0.32, z / 0.32], [x1 / 0.32, z / 0.32], [x1 / 0.32, z1 / 0.32], [x / 0.32, z1 / 0.32]))
                    under.quad(pU[0], pU[1], pU[2], pU[3], normal: -n)
                }
                z += cell
            }
            x += cell
        }
        // Eave tips.
        let tipT = C.interp(5.1, C.roofTop), tipU = C.interp(5.1, C.roofUnder)
        for side: Float in [-1, 1] {
            top.quad([side * 4.9, tipU, C.z0], [side * 4.9, tipU, 28.4], [side * 4.9, tipT, 28.4], [side * 4.9, tipT, C.z0], normal: [-side, 0, 0])
        }
        top.quad([-4.9, tipU, 28.4], [4.9, tipU, 28.4], [4.9, tipT, 28.4], [-4.9, tipT, 28.4], normal: [0, 0, -1])
        add(top, Mat.textured(Textures.resource(Textures.roofTiles()), roughness: 0.8), name: "Tile roofs")
        add(under, timber, name: "Roof boarding")
    }

    // MARK: Garden

    private func buildChineseGarden(term t: Int, stone: RealityKit.Material) {
        typealias C = ChinesePlan
        let snow = (19...23).contains(t)
        // The pond: water 0.3 m down, sloping banks, edging stones, lotus in season.
        var bank = MeshBuilder()
        let n = C.pond.count
        let centroid = C.pond.reduce(SIMD2<Float>.zero, +) / Float(n)
        for i in 0..<n {
            let a = C.pond[i], b = C.pond[(i + 1) % n]
            let ia = a + normalize(centroid - a) * 0.12, ib = b + normalize(centroid - b) * 0.12
            bank.quad([a.x, 0, a.y], [b.x, 0, b.y], [ib.x, -0.32, ib.y], [ia.x, -0.32, ia.y], normal: [0, 0.5, 0])
        }
        add(bank, Mat.matte(0x8E897F, roughness: 0.9), name: "Pond bank")
        var water = MeshBuilder()
        for i in 0..<n {
            let a = C.pond[i] + normalize(centroid - C.pond[i]) * 0.12, b = C.pond[(i + 1) % n] + normalize(centroid - C.pond[(i + 1) % n]) * 0.12
            water.tri([centroid.x, -0.3, centroid.y], [a.x, -0.3, a.y], [b.x, -0.3, b.y], [0, 1, 0], [0, 1, 0], [0, 1, 0])
        }
        var waterMat = PhysicallyBasedMaterial()
        waterMat.baseColor = .init(tint: PlatformColor(hex: 0x2F4A4A))
        waterMat.roughness = .init(floatLiteral: 0.03)
        waterMat.metallic = .init(floatLiteral: 0.85)
        add(water, waterMat, name: "Garden pond")
        collision.addPolyline(C.pond + [C.pond[0]], radius: 0.05, occludes: false, y1: 0.3)
        var stones = MeshBuilder()
        for (i, s) in C.edgingStones.enumerated() {
            let next = C.edgingStones[(i + 1) % C.edgingStones.count]
            let yaw = -atan2(next.y - s.y, next.x - s.x)
            stones.box(center: [s.x, 0.0, s.y], size: [0.6, 0.15, 0.4], yaw: yaw)
        }
        add(stones, stone, name: "Edging stones")
        // Lotus: pads from the start of summer, flowers in high summer, withered in early winter.
        if (6...19).contains(t) {
            var pads = MeshBuilder(), flowers = MeshBuilder()
            for l in C.lotus {
                pads.ellipseDisc(center: [l.p.x, -0.29, l.p.y], a: l.d / 2, b: l.d / 2, up: true, segments: 20)
                if (8...13).contains(t), l.flower > 0 {
                    flowers.ellipsoid(center: [l.p.x, 0.25, l.p.y], radii: [l.flower / 2, l.flower / 2.5, l.flower / 2], segments: 8, rings: 6)
                    pads.stem(from: [l.p.x, -0.3, l.p.y], to: [l.p.x, 0.2, l.p.y], r0: 0.01, r1: 0.01, segments: 4)
                }
            }
            add(pads, Mat.matte((16...19).contains(t) ? 0x8A7A5A : 0x8FA27F, roughness: 0.6), name: "Lotus pads")
            add(flowers, Mat.matte(0xE3A9B5, roughness: 0.6), name: "Lotus flowers")
        }
        // The Taihu rock on the axis: tall, waisted, twin-peaked, pitted.
        var rock = MeshBuilder()
        var rng = SplitMix(seed: 77)
        let sil = C.rockSilhouette
        let rings = 18, segs = 16
        func width(_ h: Float) -> (Float, Float) {
            // N–S extent at height h from the section silhouette (front and back edges).
            let front = sil.prefix(7), back = sil.suffix(7).reversed()
            func interp(_ pts: [SIMD2<Float>]) -> Float {
                for (a, b) in zip(pts, pts.dropFirst()) where (h >= a.y && h <= b.y) || (h <= a.y && h >= b.y) {
                    let t = (h - a.y) / max(1e-3, b.y - a.y)
                    return a.x + (b.x - a.x) * t
                }
                return pts.last!.x
            }
            return (interp(Array(front)), interp(Array(back)))
        }
        var ringsPts: [[SIMD3<Float>]] = []
        for j in 0...rings {
            let h = 4.1 * Float(j) / Float(rings)
            let (f, b) = width(h)
            let cy = (f + b) / 2, ry = max(0.08, (b - f) / 2)
            let rx = ry * 1.05
            var ring: [SIMD3<Float>] = []
            for i in 0..<segs {
                let a = 2 * Float.pi * Float(i) / Float(segs)
                let jitter = 1 + Float(rng.next(-0.18, 0.18))
                ring.append([C.rock.x + rx * cos(a) * jitter, h, cy + ry * sin(a) * jitter])
            }
            ringsPts.append(ring)
        }
        // Smooth shading: each vertex's normal points out from its ring's centre.
        func rn(_ j: Int, _ i: Int) -> SIMD3<Float> {
            let ring = ringsPts[j]
            let centre = ring.reduce(SIMD3<Float>.zero, +) / Float(ring.count)
            var v = ring[i] - centre
            v.y = 0.15
            return normalize(v)
        }
        for j in 0..<rings {
            for i in 0..<segs {
                let i1 = (i + 1) % segs
                rock.quad(ringsPts[j][i], ringsPts[j][i1], ringsPts[j + 1][i1], ringsPts[j + 1][i],
                          normals: rn(j, i), rn(j, i1), rn(j + 1, i1), rn(j + 1, i))
            }
        }
        let topRing = ringsPts[rings]
        let apex = topRing.reduce(SIMD3<Float>.zero, +) / Float(segs) + [0, 0.1, 0]
        for i in 0..<segs { rock.tri(topRing[i], topRing[(i + 1) % segs], apex, [0, 1, 0], [0, 1, 0], [0, 1, 0]) }
        add(rock, Mat.matte(0xB4AFA4, roughness: 0.95), name: "Taihu rock")
        var hollows = MeshBuilder()
        for (y, h, ww, hh) in [(Float(25.14), Float(3.11), Float(0.32), Float(0.50)), (25.56, 1.98, 0.28, 0.76), (24.96, 1.51, 0.20, 0.42),
                               (25.70, 3.36, 0.16, 0.42)] {
            let (f, b) = width(h)
            let r = max(0.1, (b - f) / 2)
            for side: Float in [-1, 1] {
                hollows.ellipsoid(center: [C.rock.x + side * r * 0.98, h, y], radii: [0.05, hh / 2, ww / 2], segments: 8, rings: 6)
            }
        }
        add(hollows, Mat.matte(0x6F6A61), name: "Rock hollows")
        collision.addCircle(C.rock + [0, 0.05], r: 1.0, segments: 16, occludes: false, y1: 4.2)
        if snow {
            var cap = MeshBuilder()
            cap.ellipsoid(center: [C.rock.x, 4.1, 25.5], radii: [0.45, 0.12, 0.45], segments: 12, rings: 6)
            cap.ellipsoid(center: [C.rock.x, 3.8, 25.0], radii: [0.35, 0.1, 0.35], segments: 12, rings: 6)
            add(cap, Mat.matte(0xFAFBFD, roughness: 0.6), name: "Snow on the rock")
        }

        // The plum: gnarled and leaning; blossom at Lichun, leaves in summer, bare in winter.
        let plumCrown: UInt32? = (0...2).contains(t) ? 0xEAD9D6 : ((3...14).contains(t) ? 0x8DA56A : ((15...17).contains(t) ? 0xC9A45C : nil))
        Garden.tree(in: self, at: [C.plum.x, 0, C.plum.y], trunkHeight: 2.9, trunkRadius: 0.09, trunkColour: 0x4A3A2C, crownCentre: 2.8,
                    crownRadii: [1.05, 0.6, 1.05], crownColour: plumCrown, blobs: 4, seed: 301, branches: 7,
                    flowerColour: (0...2).contains(t) ? 0xE3A9B5 : ((22...23).contains(t) ? 0xB0343A : nil))
        collision.addCircle(C.plum, r: 0.2, segments: 8, occludes: false, y1: 3)
        // Osmanthus: evergreen, gold flowers around Qiufen.
        Garden.tree(in: self, at: [C.osmanthus.x, 0, C.osmanthus.y], trunkHeight: 1.6, trunkRadius: 0.1, trunkColour: 0x5A4632,
                    crownCentre: 2.7, crownRadii: [1.4, 1.4, 1.4], crownColour: 0x6F8466, blobs: 6, seed: 302, branches: 3,
                    flowerColour: (14...16).contains(t) ? 0xD9A93B : nil)
        collision.addCircle(C.osmanthus, r: 0.25, segments: 8, occludes: false, y1: 3)
        // Bamboo: evergreen culms, 4.7–6.1 m, leaves from 2 m up.
        var culms = MeshBuilder(), leaves = MeshBuilder()
        var brng = SplitMix(seed: 88)
        for p in C.bambooNW + C.bambooNE + C.bambooSE {
            let h = Float(brng.next(4.7, 6.1))
            let lean = SIMD3<Float>(Float(brng.next(-0.25, 0.25)), 0, Float(brng.next(-0.25, 0.25)))
            culms.stem(from: [p.x, 0, p.y], to: SIMD3<Float>(p.x, h, p.y) + lean, r0: 0.03, r1: 0.02, segments: 5)
            for k in 0..<6 {
                let hh = 2.2 + Float(k) * (h - 2.4) / 6
                let at = SIMD3<Float>(p.x, hh, p.y) + lean * (hh / h)
                leaves.ellipsoid(center: at + [Float(brng.next(-0.25, 0.25)), 0, Float(brng.next(-0.25, 0.25))],
                                 radii: [0.35, 0.12, 0.35], segments: 6, rings: 3)
            }
        }
        add(culms, Mat.matte(0x5E7456, roughness: 0.6), name: "Bamboo culms")
        add(leaves, Mat.matte(snow ? 0xB9C4AE : 0x8FA27F, roughness: 0.9), name: "Bamboo leaves")
        var beds = MeshBuilder()
        beds.box(min: [-5.3, 0, 13.5], max: [-2.3, 0.15, 14.35])
        beds.box(min: [2.3, 0, 13.5], max: [5.3, 0.15, 14.35])
        add(beds, Mat.matte(0x6F8466), name: "Bamboo beds")
        collision.addRect(x0: -5.3, x1: -2.3, z0: 13.5, z1: 14.35, occludes: false, y1: 6)
        collision.addRect(x0: 2.3, x1: 5.3, z0: 13.5, z1: 14.35, occludes: false, y1: 6)
        collision.addCircle([4.4, 27.8], r: 0.55, segments: 10, occludes: false, y1: 6)
        // Two stone benches on the terrace's south edge.
        var benches = MeshBuilder()
        for x0: Float in [-3.7, 1.9] {
            benches.box(min: [x0, 0, 17.05], max: [x0 + 1.8, 0.45, 17.5])
            collision.addRect(x0: x0, x1: x0 + 1.8, z0: 17.05, z1: 17.5, occludes: false, y1: 0.45)
        }
        add(benches, stone, name: "Terrace benches")
        // Orchids at Chunfen, chrysanthemums at Shuangjiang (pots at the foot of the beds and rails).
        if (2...5).contains(t) || (16...19).contains(t) {
            let pts: [SIMD2<Float>] = [[-2.0, 14.6], [-1.6, 14.6], [1.6, 14.6], [2.0, 14.6], [-5.0, 17.7], [5.0, 17.7]]
            var pots = MeshBuilder()
            for p in pts { pots.lathe([[0.12, 0], [0.16, 0.25], [0.17, 0.27]], center: [p.x, 0, p.y], segments: 12) }
            add(pots, Mat.matte(0x6B7A7E, roughness: 0.6), name: "Pots")
            Garden.flowers(in: self, points: pts.flatMap { p in (0..<5).map { p + [Float($0) * 0.04 - 0.08, Float($0 % 2) * 0.05] } },
                           colour: (2...5).contains(t) ? 0xE8D8EE : 0xE6B84A, height: 0.4...0.6, headRadius: 0.05,
                           seed: 5, name: (2...5).contains(t) ? "Orchids" : "Chrysanthemums")
        }
        if snow {
            var snowRoof = MeshBuilder()
            snowRoof.floorCells(x0: -5.5, x1: 5.5, z0: 18, z1: 29, y: 0.01, cell: 0.5, tile: 1) { p in
                !FloorWorld.Shape.polygon(ChinesePlan.pond).contains(p)
            }
            add(snowRoof, Mat.matte(0xF4F6F8, roughness: 0.7), name: "Snow on the gravel")
        }

        // The moon in the pond: its reflection, as seen from where you stand, when the moon is up.
        let reflection = ModelEntity(mesh: .generatePlane(width: 1, depth: 1, cornerRadius: 0.5), materials: [Mat.glow(0xF4F0E4)])
        reflection.name = "Moon in the pond"
        reflection.isEnabled = false
        building.addChild(reflection)
        var timer: Float = 0
        updaters.append { [weak self] dt, eye in
            guard let self else { return }
            timer += dt
            guard timer > 0.1 else { return }
            timer = 0
            guard eye.z > 12, eye.z < 34, abs(eye.x) < 10, eye.y > -1, eye.y < 3,
                  let sky = self.skySystem, let moon = sky.moonState, moon.altitude > 0.05, sky.sunAltitudeDegrees < -4
            else { reflection.isEnabled = false; return }
            // Mirror the moon's direction in the water plane (h −0.3) and find where you see it.
            let dir = SIMD3<Float>(moon.dir.x, -moon.dir.y, moon.dir.z)
            let tHit = (-0.3 - eye.y) / dir.y
            let p = eye + dir * tHit
            guard tHit > 0, FloorWorld.Shape.polygon(ChinesePlan.pond).contains([p.x, p.z]) else { reflection.isEnabled = false; return }
            reflection.isEnabled = true
            reflection.position = [p.x, -0.295, p.z]
            let size = tHit * 0.023 * (0.5 + moon.illuminated * 0.5)
            reflection.scale = [size, 1, size / max(0.2, sin(moon.altitude))]
            reflection.orientation = simd_quatf(angle: -atan2(eye.z - p.z, eye.x - p.x), axis: [0, 1, 0])
        }
    }

    // MARK: Exhibits

    private func buildChineseExhibits(timber: RealityKit.Material, stone: RealityKit.Material) {
        typealias C = ChinesePlan
        let caseBody = Mat.matte(0xE4DBCB, roughness: 0.7)
        let bronzeTrim = Mat.metal(0x7E5C25, roughness: 0.4)
        let glass = Mat.glass(0xEEF3F2, opacity: 0.06)

        // 1 · The Orchid Pavilion Preface, flat in a table case in the NW corner.
        var orchid = MeshBuilder()
        orchid.box(min: [-9.3, 0, 13.9], max: [-7.3, 0.85, 14.7])
        add(orchid, caseBody, name: "Orchid case")
        var og = MeshBuilder()
        og.box(min: [-9.28, 0.85, 13.92], max: [-7.32, 0.9, 14.68])
        add(og, glass, name: "Orchid case glass")
        collision.addRect(x0: -9.3, x1: -7.3, z0: 13.9, z1: 14.7, occludes: false, y1: 0.9)
        let mount = ModelEntity(mesh: .generatePlane(width: 1.6, depth: 0.36), materials: [Mat.matte(0xDDCFAE)])
        mount.name = "Orchid Preface mount"
        mount.position = [-8.3, 0.851, 14.35]
        building.addChild(mount)
        // The tracing, read from the south: the characters' tops point north.
        let tracing = ModelEntity(mesh: .generatePlane(width: 0.699, depth: 0.245), materials: [placeholder])
        tracing.position = [-8.3, 0.853, 14.35]
        building.addChild(tracing)
        addImageSlot(tracing, image: "chinese-orchid-pavilion", maxPixels: 4096, at: [-8.3, 0.85, 14.35])
        targets.append(PickTarget(artworkID: "chinese-orchid-pavilion", hit: { o, d in
            Self.rayBox(o, d, min: [-9.3, 0, 13.9], max: [-7.3, 0.95, 14.7])
        }))

        // 2–6 · Five ceramics on drum plinths under glass (the chicken cup uncovered: you may hold it).
        for (i, item) in C.ceramics.enumerated() {
            let p = SIMD3<Float>(C.ceramicsX, 0, item.y)
            var plinth = MeshBuilder()
            plinth.lathe([[0.45, 0], [0.45, 0.9]], center: p, segments: 32)
            plinth.ellipseDisc(center: p + [0, 0.9, 0], a: 0.45, b: 0.45, up: true, segments: 32)
            add(plinth, caseBody, name: "Ceramic plinth")
            collision.addCircle([p.x, p.z], r: 0.56, segments: 16, occludes: false, y1: 2.0)
            let isCup = item.id == "chinese-chicken-cup"
            if !isCup {
                var cyl = MeshBuilder()
                let top: Float = i == 0 ? 2.0 : 1.6
                cyl.lathe([[0.55, 0.9], [0.55, top]], center: p, segments: 40)
                cyl.ellipseDisc(center: p + [0, top, 0], a: 0.55, b: 0.55, up: true, segments: 40)
                add(cyl, glass, name: "Ceramic case")
                var ring = MeshBuilder()
                ring.lathe([[0.55, top - 0.02], [0.56, top - 0.02], [0.56, top], [0.55, top]], center: p, segments: 40)
                add(ring, bronzeTrim, name: "Case ring")
            } else {
                var ring = MeshBuilder()
                ring.lathe([[0.085, 0.9], [0.09, 0.9], [0.09, 0.902], [0.085, 0.902]], center: p, segments: 24)
                add(ring, bronzeTrim, name: "Cup ring")
            }
            let object = buildCeramic(item.id, at: p + [0, 0.9, 0])
            if isCup { chickenCup = (object, object.position) }
            let bounds = object.visualBounds(relativeTo: nil)
            let lo = bounds.min - SIMD3<Float>(repeating: 0.1), hi = bounds.max + SIMD3<Float>(repeating: 0.1)
            let hit: (SIMD3<Float>, SIMD3<Float>) -> Float? = { [weak object] o, d in
                guard let object else { return nil }
                if isCup {
                    let c: SIMD3<Float> = object.position(relativeTo: nil) + SIMD3<Float>(0, 0.03, 0)
                    return MuseumScene.raySphere(o, d, centre: c, radius: 0.12)
                }
                return MuseumScene.rayBox(o, d, min: lo, max: hi)
            }
            var target = PickTarget(artworkID: item.id, detail: isCup ? "The one you may hold. Tap it again to put it back." : nil, hit: hit)
            if isCup { target.action = { [weak self] in self?.toggleCup() } }
            targets.append(target)
        }
        updaters.append { [weak self] dt, eye in self?.updateCup(dt: dt, eye: eye) }

        // 7–12 · Six hanging scrolls on cords from one rail at 3.45 m on the south wall, centres at 1.6 m.
        var rail = MeshBuilder()
        rail.box(min: [-9.6, 3.43, 33.44], max: [9.6, 3.47, 33.5])
        var cords = MeshBuilder()
        var sticks = MeshBuilder()
        for s in C.scrolls {
            let mw = s.w + 0.2, top = 1.6 + s.h / 2 + max(0.3, s.h * 0.2), bottom = 1.6 - s.h / 2 - max(0.16, s.h * 0.12)
            let silk = ModelEntity(mesh: .generatePlane(width: mw, height: top - bottom), materials: [Mat.matte(0xC9BB98, roughness: 0.85)])
            silk.name = "Mount " + s.id
            silk.position = [s.x, (top + bottom) / 2, 33.47]
            silk.orientation = simd_quatf(angle: .pi, axis: [0, 1, 0])
            building.addChild(silk)
            hangFramed(id: s.id, artworkID: s.id, image: s.id, wall: [s.x, 1.6, 33.465], facing: [0, 0, -1], width: s.w, height: s.h,
                       frame: (0, 0.002, 0, false), tint: 0xEDE7DA)
            sticks.box(min: [s.x - mw / 2, top, 33.43], max: [s.x + mw / 2, top + 0.07, 33.49])
            sticks.stem(from: [s.x - mw / 2 - 0.06, bottom - 0.045, 33.44], to: [s.x + mw / 2 + 0.06, bottom - 0.045, 33.44],
                        r0: 0.045, r1: 0.045, segments: 10)
            cords.stem(from: [s.x, 3.45, 33.45], to: [s.x - mw / 2 + 0.05, top + 0.07, 33.46], r0: 0.004, r1: 0.004, segments: 4)
            cords.stem(from: [s.x, 3.45, 33.45], to: [s.x + mw / 2 - 0.05, top + 0.07, 33.46], r0: 0.004, r1: 0.004, segments: 4)
        }
        add(rail, timber, name: "Picture rail")
        add(sticks, timber, name: "Scroll sticks")
        add(cords, Mat.matte(0x5E574D), name: "Cords")

        // 13 · The Thousand Li handscroll at full length in a 13.4 m case; an arm's length open at a time.
        let H = C.handscroll
        var caseMesh = MeshBuilder()
        caseMesh.box(min: [H.x0, 0, H.y0], max: [H.x1, 0.8, H.y1])
        add(caseMesh, caseBody, name: "Handscroll case")
        var bed = MeshBuilder()
        bed.floorRect(x0: H.x0 + 0.05, x1: H.x1 - 0.05, z0: H.y0 + 0.05, z1: H.y1 - 0.05, y: 0.8, up: true)
        add(bed, Mat.matte(0xE4D8BA), name: "Scroll bed")
        var lid = MeshBuilder()
        lid.box(min: [H.x0 + 0.05, 0.88, H.y0 + 0.05], max: [H.x1 - 0.05, 0.9, H.y1 - 0.05])
        add(lid, glass, name: "Handscroll glass")
        var trim = MeshBuilder()
        trim.box(min: [H.x0, 0.8, H.y0], max: [H.x0 + 0.03, 0.9, H.y1])
        add(trim, bronzeTrim, name: "Case trim")
        collision.addRect(x0: H.x0, x1: H.x1, z0: H.y0, z1: H.y1, occludes: false, y1: 0.9)
        // Four tiles of the scroll, north (the image's left, its end) to south (its right, its beginning).
        let length = H.start - H.end
        for k in 0..<4 {
            let za = H.end + length * Float(k) / 4, zb = H.end + length * Float(k + 1) / 4
            var q = MeshBuilder()
            // Top of the painting to the east: image up = +x, image right = +z (south).
            q.quad([H.silkX - H.silkW / 2, 0.82, za], [H.silkX - H.silkW / 2, 0.82, zb], [H.silkX + H.silkW / 2, 0.82, zb],
                   [H.silkX + H.silkW / 2, 0.82, za], normal: [0, 1, 0], uv: ([0, 0], [1, 0], [1, 1], [0, 1]))
            let e = ModelEntity(mesh: q.mesh(name: "thousand li \(k + 1)"), materials: [placeholder])
            building.addChild(e)
            addImageSlot(e, image: "chinese-thousand-li-\(k + 1)", maxPixels: 4096, at: [H.silkX, 1.0, (za + zb) / 2], tint: 0xF2EDE2)
        }
        // The veil over the parts not yet open (north, "to come") and already seen (south), and the two rollers.
        var veilMat = UnlitMaterial(color: PlatformColor(hex: 0xE4DBCB))
        veilMat.blending = .transparent(opacity: 0.72)
        let veilNorth = ModelEntity(mesh: .generatePlane(width: H.silkW + 0.04, depth: 1), materials: [veilMat])
        let veilSouth = ModelEntity(mesh: .generatePlane(width: H.silkW + 0.04, depth: 1), materials: [veilMat])
        veilNorth.name = "Veil north"
        veilSouth.name = "Veil south"
        var roller = MeshBuilder()
        roller.stem(from: [-0.3, 0, 0], to: [0.3, 0, 0], r0: 0.0365, r1: 0.0365, segments: 12)
        var rod = MeshBuilder()
        rod.stem(from: [-0.324, 0, 0], to: [0.324, 0, 0], r0: 0.0165, r1: 0.0165, segments: 8)
        let rollers: [Entity] = (0..<2).map { i in
            let e = Entity()
            e.name = i == 0 ? "Roller north" : "Roller south"
            e.addChild(ModelEntity(mesh: roller.mesh(name: "roller"), materials: [Mat.matte(0xD9CDB0)]))
            e.addChild(ModelEntity(mesh: rod.mesh(name: "rod"), materials: [timber]))
            return e
        }
        for e in [veilNorth, veilSouth] as [Entity] + rollers { building.addChild(e) }
        var windowCentre: Float = H.start - H.window / 2
        let lo = H.end + H.window / 2, hi = H.start - H.window / 2
        func place() {
            let a = windowCentre - H.window / 2, b = windowCentre + H.window / 2
            veilNorth.position = [H.silkX, 0.825, (H.end + a) / 2]
            veilNorth.scale = [1, 1, max(0.001, a - H.end)]
            veilSouth.position = [H.silkX, 0.825, (b + H.start) / 2]
            veilSouth.scale = [1, 1, max(0.001, H.start - b)]
            rollers[0].position = [H.silkX, 0.857, a]
            rollers[1].position = [H.silkX, 0.857, b]
            let turn = windowCentre / (Float.pi * 0.073) * 2 * .pi
            rollers[0].orientation = simd_quatf(angle: turn, axis: [1, 0, 0])
            rollers[1].orientation = simd_quatf(angle: turn, axis: [1, 0, 0])
        }
        place()
        updaters.append { dt, eye in
            // It follows you as you walk beside it on the east walk; otherwise it rests at the beginning.
            let beside = eye.x > 5.5 && eye.x < 9.0 && eye.z > H.y0 - 1 && eye.z < H.y1 + 1 && abs(eye.y - 1.6) < 1
            let goal = beside ? simd_clamp(eye.z, lo, hi) : hi
            let diff = goal - windowCentre
            guard abs(diff) > 0.001 else { return }
            let step = simd_clamp(diff * min(1, dt / 0.4), -0.8 * dt, 0.8 * dt)
            windowCentre += abs(step) > abs(diff) ? diff : step
            place()
        }
        targets.append(PickTarget(artworkID: "chinese-thousand-li",
                                  detail: "It opens an arm's length at a time, right to left, rolling on as you walk north.",
                                  hit: { o, d in Self.rayBox(o, d, min: [H.x0, 0, H.y0], max: [H.x1, 0.95, H.y1]) }))
    }

    /// A ceramic at true size, turned on a lathe from the board's profiles (the horse from its scan
    /// or, failing that, modelled from the board's drawing).
    private func buildCeramic(_ id: String, at base: SIMD3<Float>) -> Entity {
        let e = Entity()
        e.name = id
        e.position = base
        building.addChild(e)
        func cm(_ pts: [(Float, Float)]) -> [SIMD2<Float>] { pts.map { [$0.0 / 100, $0.1 / 100] } }
        switch id {
        case "chinese-ru-basin":
            var b = MeshBuilder()
            b.lathe(cm([(0, 0.8), (8.5, 0.8), (10.2, 1.0), (11.6, 6.2), (11.7, 6.7), (11.0, 6.7), (10.5, 1.5), (0, 1.5)]), center: .zero, segments: 40)
            for (dx, dz) in [(Float(-0.07), Float(-0.04)), (0.07, -0.04), (-0.07, 0.04), (0.07, 0.04)] {
                b.ellipsoid(center: [dx, 0.004, dz], radii: [0.012, 0.008, 0.012], segments: 6, rings: 4)
            }
            let m = ModelEntity(mesh: b.mesh(name: id), materials: [Mat.matte(0xA9C4C0, roughness: 0.3)])
            m.scale = [1, 1, 0.7]
            e.addChild(m)
        case "chinese-meiping":
            var b = MeshBuilder()
            b.lathe(cm([(0, 0), (6.5, 0), (7.2, 4), (9.2, 12), (12.2, 22), (13.8, 30), (13.2, 35), (9.5, 39), (4.0, 41), (3.0, 42),
                        (3.6, 42.6), (3.6, 44), (0, 44)]), center: .zero, segments: 48)
            var m = PhysicallyBasedMaterial()
            m.baseColor = .init(tint: .white, texture: .init(Textures.resource(Textures.meiping())))
            m.roughness = .init(floatLiteral: 0.25)
            e.addChild(ModelEntity(mesh: b.mesh(name: id), materials: [m]))
        case "chinese-chicken-cup":
            var b = MeshBuilder()
            b.lathe(cm([(0, 0), (1.9, 0), (2.1, 0.4), (2.6, 0.5), (3.4, 1.6), (4.1, 3.4), (3.9, 3.4), (3.2, 1.8), (2.4, 0.8), (0, 0.8)]),
                    center: .zero, segments: 40)
            var m = PhysicallyBasedMaterial()
            m.baseColor = .init(tint: .white, texture: .init(Textures.resource(Textures.chickenCup())))
            m.roughness = .init(floatLiteral: 0.25)
            e.addChild(ModelEntity(mesh: b.mesh(name: id), materials: [m]))
        case "chinese-peachbloom":
            var b = MeshBuilder()
            b.lathe(cm([(0, 0), (2.6, 0), (2.9, 1), (4.4, 5), (5.0, 9), (4.4, 13), (2.8, 16), (1.6, 17.6), (1.5, 18.8), (1.9, 20), (1.6, 20),
                        (0, 19.5)]), center: .zero, segments: 40)
            var m = PhysicallyBasedMaterial()
            m.baseColor = .init(tint: .white, texture: .init(Textures.resource(Textures.peachbloom())))
            m.roughness = .init(floatLiteral: 0.2)
            e.addChild(ModelEntity(mesh: b.mesh(name: id), materials: [m]))
        default: // the sancai horse
            if let scan = Self.loadScan("chinese-sancai-horse") {
                let m = ModelEntity(mesh: scan, materials: [Mat.matte(0xC58B3C, roughness: 0.35)])
                m.orientation = simd_quatf(angle: -.pi / 2, axis: [0, 1, 0])   // head north, side-on to the walk
                e.addChild(m)
            } else {
                e.addChild(Self.sancaiHorse())
            }
        }
        return e
    }

    /// The Tang horse, modelled from the board's drawing (body, legs, arched neck, saddle cloth).
    static func sancaiHorse() -> Entity {
        let root = Entity()
        var amber = MeshBuilder(), cream = MeshBuilder(), green = MeshBuilder(), base = MeshBuilder()
        base.box(min: [-0.03, 0, -0.38], max: [0.03 + 0.1, 0.03, 0.38])
        base = MeshBuilder()
        base.box(min: [-0.11, 0, -0.38], max: [0.11, 0.03, 0.38])
        // Head end towards −z (north); body along z.
        amber.ellipsoid(center: [0, 0.41, 0], radii: [0.11, 0.11, 0.27], segments: 20, rings: 12)
        for z: Float in [-0.22, -0.14, 0.15, 0.23] {
            for x: Float in [-0.06, 0.06] {
                cream.box(min: [x - 0.02, 0.03, z - 0.02], max: [x + 0.02, 0.34, z + 0.02])
            }
        }
        amber.stem(from: [0, 0.45, -0.2], to: [0, 0.6, -0.3], r0: 0.07, r1: 0.055, segments: 12)
        amber.stem(from: [0, 0.6, -0.3], to: [0, 0.66, -0.36], r0: 0.055, r1: 0.045, segments: 12)
        amber.ellipsoid(center: [0, 0.63, -0.4], radii: [0.045, 0.05, 0.09], segments: 12, rings: 8)
        cream.stem(from: [0, 0.62, -0.26], to: [0, 0.69, -0.34], r0: 0.02, r1: 0.015, segments: 6)
        cream.stem(from: [0, 0.44, 0.25], to: [0, 0.3, 0.33], r0: 0.03, r1: 0.015, segments: 8)
        green.ellipsoid(center: [0, 0.5, 0.02], radii: [0.12, 0.05, 0.13], segments: 14, rings: 8)
        root.addChild(ModelEntity(mesh: amber.mesh(name: "horse"), materials: [Mat.matte(0xC58B3C, roughness: 0.3)]))
        root.addChild(ModelEntity(mesh: cream.mesh(name: "horse legs"), materials: [Mat.matte(0xE6D6B4, roughness: 0.35)]))
        root.addChild(ModelEntity(mesh: green.mesh(name: "saddle"), materials: [Mat.matte(0x6E8B4F, roughness: 0.3)]))
        root.addChild(ModelEntity(mesh: base.mesh(name: "base"), materials: [Mat.matte(0xCBB58A, roughness: 0.6)]))
        return root
    }

    // MARK: The chicken cup you may hold

    func toggleCup() {
        guard let cup = chickenCup else { return }
        cupHeld.toggle()
        message = cupHeld ? "The Chenghua chicken cup, in your hand. Tap it to put it back." : nil
        _ = cup
    }

    private func updateCup(dt: Float, eye: SIMD3<Float>) {
        guard let (cup, home) = chickenCup else { return }
        let target: SIMD3<Float>
        if cupHeld {
            let f = normalize(SIMD3<Float>(visitorForward.x, 0, visitorForward.z))
            target = eye + f * 0.45 + [0, -0.18, 0]
            cup.orientation *= simd_quatf(angle: dt * 0.5, axis: [0, 1, 0])
            if simd_distance(SIMD2<Float>(eye.x, eye.z), [ChinesePlan.ceramicsX, 26.75]) > 5 {
                cupHeld = false
                message = nil
            }
        } else {
            target = home
        }
        let p = cup.position
        cup.position = p + (target - p) * min(1, dt * 6)
    }
}

extension Textures {
    /// The stele: today's solar term in two large gilt characters, with its pinyin and English.
    static func steleInscription(term: Int) -> CGImage {
        let t = Ephemeris.solarTerms[term]
        return draw(width: 360, height: 680, name: "stele_inscription") { ctx in
            ctx.setFillColor(cg(0xCFC6B8))
            ctx.fill(CGRect(x: 0, y: 0, width: 360, height: 680))
            ctx.setStrokeColor(cg(0xB7AD9C))
            ctx.setLineWidth(6)
            ctx.stroke(CGRect(x: 20, y: 20, width: 320, height: 640))
            let chars = Array(t.hanzi)
            for (i, ch) in chars.enumerated() {
                ctx.saveGState()
                ctx.translateBy(x: 180, y: 520 - CGFloat(i) * 170)
                text(String(ch), in: ctx, font: "Songti TC", size: 140, color: cg(0xB8924A))
                ctx.restoreGState()
            }
            ctx.saveGState(); ctx.translateBy(x: 180, y: 150)
            text(t.pinyin.uppercased(), in: ctx, font: "Didot", size: 34, color: cg(0x5E574D), tracking: 3)
            ctx.restoreGState()
            ctx.saveGState(); ctx.translateBy(x: 180, y: 100)
            text(t.english.uppercased(), in: ctx, font: "Avenir Next", size: 20, color: cg(0x5E574D), tracking: 2)
            ctx.restoreGState()
        }
    }

    /// The plaque 四時園, read right to left (四 on the right as you face it).
    static func plaque() -> CGImage {
        draw(width: 644, height: 212) { ctx in
            ctx.setFillColor(cg(0x7E5C25))
            ctx.fill(CGRect(x: 0, y: 0, width: 644, height: 212))
            ctx.setFillColor(cg(0x3B332B))
            ctx.fill(CGRect(x: 8, y: 8, width: 628, height: 196))
            for (i, ch) in ["園", "時", "四"].enumerated() {
                ctx.saveGState()
                ctx.translateBy(x: 170 + CGFloat(i) * 152, y: 106)
                text(ch, in: ctx, font: "Songti TC", size: 118, color: cg(0xD9B77A))
                ctx.restoreGState()
            }
        }
    }

    static func gravel() -> CGImage {
        draw(width: 256, height: 256) { ctx in
            ctx.setFillColor(cg(0xE9E4D8))
            ctx.fill(CGRect(x: 0, y: 0, width: 256, height: 256))
            var rng = SplitMix(seed: 31)
            for _ in 0..<1400 {
                ctx.setFillColor(cg([0xD2CBBB, 0xC6BFAE, 0xF3EFE6][Int(rng.nextUInt() % 3)]))
                let r = rng.next(1, 3.5)
                ctx.fillEllipse(in: CGRect(x: rng.next(0, 256), y: rng.next(0, 256), width: r * 1.3, height: r))
            }
        }
    }

    static func roofTiles() -> CGImage {
        draw(width: 128, height: 128) { ctx in
            ctx.setFillColor(cg(0x8E8982))
            ctx.fill(CGRect(x: 0, y: 0, width: 128, height: 128))
            ctx.setFillColor(cg(0x77726B))
            ctx.fill(CGRect(x: 0, y: 0, width: 128, height: 14))
            ctx.fill(CGRect(x: 0, y: 0, width: 14, height: 128))
        }
    }

    /// Yuan blue-and-white: cobalt bands at the foot and shoulder, a painted scene between.
    static func meiping() -> CGImage {
        draw(width: 512, height: 512) { ctx in
            ctx.setFillColor(cg(0xF4F2EB))
            ctx.fill(CGRect(x: 0, y: 0, width: 512, height: 512))
            let blue = cg(0x3C5B8C)
            // v runs up the profile (0 foot … 1 lip): bands at 3–6 cm and 33–37 cm of 44.
            ctx.setFillColor(blue)
            ctx.fill(CGRect(x: 0, y: 512 * 3 / 44, width: 512, height: 512 * 3 / 44))
            ctx.fill(CGRect(x: 0, y: 512 * 33 / 44, width: 512, height: 512 * 4 / 44))
            ctx.setFillColor(cg(0xF4F2EB))
            for i in 0..<16 {
                ctx.fillEllipse(in: CGRect(x: CGFloat(i) * 32 + 6, y: 512 * 33.8 / 44, width: 20, height: 22))
            }
            // The scene: a rider, a pine, the moon, hills (loose brush strokes).
            ctx.setStrokeColor(blue)
            ctx.setLineWidth(5)
            ctx.setLineCap(.round)
            var rng = SplitMix(seed: 12)
            for _ in 0..<60 {
                let x = rng.next(10, 500), y = rng.next(512 * 8 / 44, 512 * 31 / 44)
                ctx.move(to: CGPoint(x: x, y: y))
                ctx.addQuadCurve(to: CGPoint(x: x + rng.next(-40, 40), y: y + rng.next(-18, 18)),
                                 control: CGPoint(x: x + rng.next(-30, 30), y: y + rng.next(-30, 30)))
            }
            ctx.strokePath()
            ctx.strokeEllipse(in: CGRect(x: 380, y: 512 * 26 / 44, width: 42, height: 42))
        }
    }

    static func chickenCup() -> CGImage {
        draw(width: 256, height: 128) { ctx in
            ctx.setFillColor(cg(0xF6F3EA))
            ctx.fill(CGRect(x: 0, y: 0, width: 256, height: 128))
            var rng = SplitMix(seed: 4)
            for i in 0..<6 {
                let x = CGFloat(i) * 42 + 10
                ctx.setFillColor(cg(0xC0493A))
                ctx.fillEllipse(in: CGRect(x: x, y: 50, width: 18, height: 12))
                ctx.setFillColor(cg(0xD9A93B))
                ctx.fillEllipse(in: CGRect(x: x + 14, y: 44, width: 8, height: 8))
                ctx.setFillColor(cg(0x5E8A55))
                ctx.fill(CGRect(x: x + rng.next(-6, 20), y: 30, width: 3, height: 22))
            }
            ctx.setStrokeColor(cg(0x3C5B8C))
            ctx.setLineWidth(2)
            ctx.move(to: CGPoint(x: 0, y: 108)); ctx.addLine(to: CGPoint(x: 256, y: 108))
            ctx.move(to: CGPoint(x: 0, y: 20)); ctx.addLine(to: CGPoint(x: 256, y: 20))
            ctx.strokePath()
        }
    }

    static func peachbloom() -> CGImage {
        draw(width: 256, height: 256) { ctx in
            ctx.setFillColor(cg(0xC07A70))
            ctx.fill(CGRect(x: 0, y: 0, width: 256, height: 256))
            var rng = SplitMix(seed: 8)
            for _ in 0..<120 {
                ctx.setFillColor(cg([0xA9605A, 0xD39A8C, 0x8E6A5A][Int(rng.nextUInt() % 3)], 0.4))
                let r = rng.next(4, 18)
                ctx.fillEllipse(in: CGRect(x: rng.next(0, 256), y: rng.next(0, 256), width: r, height: r * 0.8))
            }
        }
    }
}
