import CoreGraphics
import Foundation
import RealityKit
import simd

extension MuseumScene {
    // MARK: - Salon Impression

    func buildSalon() {
        typealias S = Plan.Salon
        let hw = S.halfWidth
        // Floor.
        var floor = MeshBuilder()
        floor.floorRect(x0: S.farWallX, x1: S.endWallX, z0: -hw, z1: hw, y: 0, up: true, tile: 2.4)
        add(floor, Mat.polishedStone(.travertine), name: "Salon floor")

        // Long walls, one colour per bay; each run spans from pier centre to pier centre.
        let breaks: [Float] = [S.endWallX] + S.piers + [S.farWallX]
        var mouldings = MeshBuilder()
        for (i, bay) in S.bays.enumerated() {
            let xa = breaks[i], xb = breaks[i + 1]
            var north = WallRun(points: [[xa, -hw], [xb, -hw]], inside: .left, height: S.wallHeight, thickness: S.wall)
            if bay.number == 1 {
                north.openings = [WallOpening(center: xa - S.Cabinet.doorCentreX, width: S.door.width, spring: S.door.spring)]
            }
            let south = WallRun(points: [[xa, hw], [xb, hw]], inside: .right, height: S.wallHeight, thickness: S.wall)
            var b = MeshBuilder()
            b.wall(north, faces: (true, false))
            b.wall(south, faces: (true, false))
            add(b, Mat.matte(bay.colour), name: "Bay \(bay.number) walls")
            for run in [north, south] {
                contactShade(run)
                mouldings.band(run, from: 6.2, to: 6.6, depth: 0.35)
                mouldings.band(run, from: 0, to: 0.15, depth: 0.03)
                collision.add(run: run)
            }
        }
        // End walls with their doors: Bay 1 (from the Rotunda) and Bay 5 (to the oval).
        let door = { (c: Float) in WallOpening(center: c, width: S.door.width, spring: S.door.spring) }
        let endWall = WallRun(points: [[S.endWallX, hw], [S.endWallX, -hw]], inside: .left, height: S.wallHeight,
                              thickness: S.wall, openings: [door(hw)])
        let farWall = WallRun(points: [[S.farWallX, -hw], [S.farWallX, hw]], inside: .left, height: S.wallHeight,
                              thickness: S.wall, openings: [door(hw)])
        var e1 = MeshBuilder(); e1.wall(endWall)
        add(e1, Mat.matte(S.bays[0].colour), name: "Bay 1 end wall")
        var e5 = MeshBuilder(); e5.wall(farWall)
        add(e5, Mat.matte(S.bays[4].colour), name: "Bay 5 end wall")
        for run in [endWall, farWall] {
            contactShade(run)
            mouldings.band(run, from: 6.2, to: 6.6, depth: 0.35)
            mouldings.band(run, from: 0, to: 0.15, depth: 0.03)
            collision.add(run: run)
        }
        add(mouldings, Mat.matte(Mat.moulding), name: "Salon mouldings")

        // Piers and transverse arches (9 m clear, springing at 6.5 m).
        var piers = MeshBuilder()
        for x in S.piers {
            let t = S.pierThickness / 2
            piers.box(min: [x - t, 0, -hw], max: [x + t, S.wallHeight, -hw + S.pierDepth])
            piers.box(min: [x - t, 0, hw - S.pierDepth], max: [x + t, S.wallHeight, hw])
            piers.box(min: [x - t - 0.08, S.wallHeight - 0.35, -hw], max: [x + t + 0.08, S.wallHeight, -hw + S.pierDepth + 0.08])
            piers.box(min: [x - t - 0.08, S.wallHeight - 0.35, hw - S.pierDepth - 0.08], max: [x + t + 0.08, S.wallHeight, hw])
            piers.transverseArch(x: x, depth: S.pierThickness, rIn: S.archIntrados, rOut: S.vaultRadius + 0.05,
                                 spring: S.wallHeight)
            collision.addRect(x0: x - t, x1: x + t, z0: -hw, z1: -hw + S.pierDepth)
            collision.addRect(x0: x - t, x1: x + t, z0: hw - S.pierDepth, z1: hw)
        }
        add(piers, Mat.honedStone(.travertine, tint: 0xFFF9EE), name: "Piers and arches")

        // Coffered barrel vault with a light-grid skylight over each bay.
        // The coffered barrel vault: real coffers over each bay (skylight cells left open),
        // plain vault over the piers and the 0.6 m strips at each end of a bay.
        var vault = MeshBuilder()
        let R = S.vaultRadius
        let hole = acos(S.skylightHalfWidth / R)
        let side = (0...8).map { hole * Float($0) / 8 }
        let angles = side + [Float.pi / 2] + side.reversed().map { Float.pi - $0 }
        for bay in S.bays {
            let xa = bay.skylight.x0, xb = bay.skylight.x1
            let n = 8
            let xs = (0...n).map { xa + (xb - xa) * Float($0) / Float(n) }
            vault.coffers(us: xs, vs: angles, depth: 0.32, rib: 0.14,
                          centre: { p in [p.x, S.wallHeight, 0] },
                          skip: { _, j in j == 8 || j == 9 }) { x, a, d in
                [x, S.wallHeight + (R + d) * sin(a), (R + d) * cos(a)]
            }
        }
        // Plain strips: bay ends and the pier zones.
        var strips: [(Float, Float)] = []
        let edges: [Float] = [S.endWallX] + S.bays.flatMap { [$0.skylight.x0, $0.skylight.x1] } + [S.farWallX]
        for k in stride(from: 0, to: edges.count - 1, by: 2) { strips.append((edges[k], edges[k + 1])) }
        for (a, b) in strips where abs(a - b) > 1e-3 {
            vault.barrelVault(x0: a, x1: b, radius: R, spring: S.wallHeight, holes: [], tile: 1.2)
        }
        add(vault, Mat.honedStone(.travertine, tint: 0xFFFAF0, seed: 10), name: "Vault")
        var lunettes = MeshBuilder()
        lunettes.lunette(x: S.endWallX - 0.001, radius: S.vaultRadius, spring: S.wallHeight, facing: -1)
        lunettes.lunette(x: S.farWallX + 0.001, radius: S.vaultRadius, spring: S.wallHeight, facing: 1)
        add(lunettes, Mat.matte(Mat.stone), name: "Lunettes")

        let crown = S.wallHeight + sqrt(S.vaultRadius * S.vaultRadius - S.skylightHalfWidth * S.skylightHalfWidth)
        let top: Float = 14.8
        var lantern = MeshBuilder()
        var grid = MeshBuilder()
        for bay in S.bays {
            let xa = bay.skylight.x0, xb = bay.skylight.x1, h = S.skylightHalfWidth
            lantern.quad([xa, crown - 0.05, -h], [xb, crown - 0.05, -h], [xb, top, -h], [xa, top, -h], normal: [0, 0, 1])
            lantern.quad([xa, crown - 0.05, h], [xb, crown - 0.05, h], [xb, top, h], [xa, top, h], normal: [0, 0, -1])
            lantern.quad([xa, crown - 0.05, -h], [xa, crown - 0.05, h], [xa, top, h], [xa, top, -h], normal: [-1, 0, 0])
            lantern.quad([xb, crown - 0.05, -h], [xb, crown - 0.05, h], [xb, top, h], [xb, top, -h], normal: [1, 0, 0])
            grid.floorRect(x0: xb, x1: xa, z0: -h, z1: h, y: top, up: false, tile: 0.6)
        }
        add(lantern, Mat.matte(0xF4EFE5), name: "Skylight lanterns")
        add(grid, Mat.glow(Textures.resource(Textures.lightGrid()), repeating: true), name: "Light-grid skylights")
    }

    // MARK: - Manet cabinet

    func buildCabinet() {
        typealias C = Plan.Salon.Cabinet
        let east = WallRun(points: [[C.x0, C.z1], [C.x0, C.z0]], inside: .left, height: C.height, thickness: 0.4)
        let north = WallRun(points: [[C.x0, C.z0], [C.x1, C.z0]], inside: .left, height: C.height, thickness: 0.4)
        let west = WallRun(points: [[C.x1, C.z0], [C.x1, C.z1]], inside: .left, height: C.height, thickness: 0.4)
        // The cabinet side of the Salon's north wall, with the door.
        let south = WallRun(points: [[C.x1, C.z1], [C.x0, C.z1]], inside: .left, height: C.height, thickness: 0.6,
                            openings: [WallOpening(center: C.doorCentreX - C.x1, width: Plan.Salon.door.width,
                                                   spring: Plan.Salon.door.spring)])
        var b = MeshBuilder()
        var m = MeshBuilder()
        for run in [east, north, west] {
            b.wall(run, faces: (true, false))
            collision.add(run: run)
        }
        b.wall(south, faces: (true, false), reveals: false)
        for run in [east, north, west, south] {
            m.band(run, from: C.height - 0.4, to: C.height, depth: 0.25)
            m.band(run, from: 0, to: 0.15, depth: 0.03)
        }
        add(b, Mat.matte(C.colour), name: "Cabinet walls")
        add(m, Mat.matte(Mat.moulding), name: "Cabinet mouldings")
        var f = MeshBuilder()
        f.floorRect(x0: C.x1, x1: C.x0, z0: C.z0, z1: C.z1 + 0.62, y: 0, up: true, tile: 2.4)
        add(f, Mat.polishedStone(.travertine), name: "Cabinet floor")
        var ceiling = MeshBuilder()
        ceiling.floorRect(x0: C.x1, x1: C.x0, z0: C.z0, z1: C.z1, y: C.height, up: false)
        add(ceiling, Mat.matte(Mat.stone), name: "Cabinet ceiling")
        var laylight = MeshBuilder()
        let cx = (C.x0 + C.x1) / 2, cz = (C.z0 + C.z1) / 2
        laylight.floorRect(x0: cx - 2, x1: cx + 2, z0: cz - 2, z1: cz + 2, y: C.height - 0.01, up: false, tile: 0.5)
        add(laylight, Mat.glow(Textures.resource(Textures.lightGrid()), tint: 0xF2EEE6, repeating: true), name: "Cabinet laylight")
    }

    // MARK: - Nymphéas oval

    func buildOval() {
        typealias O = Plan.Oval
        let c = O.centre
        // Passage from Bay 5 (3 m wide, arched like the Salon doors).
        let hw: Float = 1.5
        let spring = Plan.Salon.door.spring
        var p = MeshBuilder()
        let px0 = Plan.Salon.farWallX - 0.55, px1: Float = -75.9
        p.wall(WallRun(points: [[px0, -hw], [px1, -hw]], inside: .left, height: spring, thickness: 0.2), faces: (true, false))
        p.wall(WallRun(points: [[px0, hw], [px1, hw]], inside: .right, height: spring, thickness: 0.2), faces: (true, false))
        for i in 0..<32 {
            let a0 = Float.pi * Float(i) / 32, a1 = Float.pi * Float(i + 1) / 32
            func q(_ x: Float, _ a: Float) -> SIMD3<Float> { [x, spring + hw * sin(a), hw * cos(a)] }
            let n0 = SIMD3<Float>(0, -sin(a0), -cos(a0)), n1 = SIMD3<Float>(0, -sin(a1), -cos(a1))
            p.quad(q(px0, a0), q(px1, a0), q(px1, a1), q(px0, a1), normals: n0, n0, n1, n1)
        }
        add(p, Mat.matte(O.colour), name: "Oval passage")
        collision.addPolyline([[px0, -hw], [px1, -hw]])
        collision.addPolyline([[px0, hw], [px1, hw]])
        var pf = MeshBuilder()
        pf.floorRect(x0: -76.2, x1: Plan.Salon.farWallX, z0: -hw, z1: hw, y: 0, up: true, tile: 2.4)
        add(pf, Mat.polishedStone(.travertine), name: "Oval passage floor")

        // The oval wall: a loop from the west apex, so the door (east apex) sits mid-run.
        // (The loop overlaps itself by a hair so no seam shows at the west apex.)
        let loop = Poly.ellipse(c, a: O.a, b: O.b, from: .pi, to: 3 * .pi + 0.01, segments: 400)
        let half = Poly.arcLength(Poly.ellipse(c, a: O.a, b: O.b, from: .pi, to: 2 * .pi, segments: 400))
        let run = WallRun(points: loop, inside: .right, height: O.height, thickness: O.wall,
                          openings: [WallOpening(center: half, width: 3, spring: spring)])
        var w = MeshBuilder()
        w.wall(run)
        add(w, Mat.matte(O.colour, roughness: 0.92), name: "Oval wall")
        var m = MeshBuilder()
        m.band(run, from: O.height - 0.3, to: O.height, depth: 0.2)
        m.band(run, from: 0, to: 0.15, depth: 0.03)
        add(m, Mat.matte(Mat.moulding), name: "Oval mouldings")
        collision.add(run: run)
        contactShade(run)

        var floor = MeshBuilder()
        // The floor has an opening over the stair shaft, covered by the pond until it lifts.
        let fa = O.a + 0.2, fb = O.b + 0.2
        // Grid aligned to the opening's edges so the hole is exact.
        let o = PondPlan.openingRect
        let gx0 = o.x0 - 0.3 * ceil((o.x0 - (c.x - fa)) / 0.3), gz0 = o.z0 - 0.3 * ceil((o.z0 - (c.y - fb)) / 0.3)
        floor.floorCells(x0: gx0, x1: c.x + fa, z0: gz0, z1: c.y + fb, y: 0, cell: 0.3, tile: 2.4) { p in
            let d = p - c
            return (d.x * d.x) / (fa * fa) + (d.y * d.y) / (fb * fb) <= 1 && !PondPlan.opening.contains(p)
        }
        add(floor, Mat.polishedStone(.travertine), name: "Oval floor")

        // Velarium: a softly glowing fabric dome over the whole oval.
        var vel = MeshBuilder()
        let rings = 12, segs = 96
        for j in 0..<rings {
            for i in 0..<segs {
                func pt(_ rho: Float, _ t: Float) -> SIMD3<Float> {
                    [c.x + O.a * rho * cos(t), O.height + O.velariumRise * (1 - rho * rho), c.y + O.b * rho * sin(t)]
                }
                let r0 = Float(j) / Float(rings), r1 = Float(j + 1) / Float(rings)
                let t0 = 2 * Float.pi * Float(i) / Float(segs), t1 = 2 * Float.pi * Float(i + 1) / Float(segs)
                let a = pt(r0, t0), b = pt(r0, t1), cc = pt(r1, t1), d = pt(r1, t0)
                let uv = { (v: SIMD3<Float>) in SIMD2<Float>(v.x / 8, v.z / 8) }
                vel.quad(a, b, cc, d, normal: [0, -1, 0], uv: (uv(a), uv(b), uv(cc), uv(d)))
            }
        }
        add(vel, Mat.glow(Textures.resource(Textures.velarium()), repeating: true), name: "Velarium")

        part("Reserve") { buildPond() }   // the lifting pond belongs with the Reserve below it

        // Four curved stone benches facing the panels.
        var benches = MeshBuilder()
        for bench in O.benches {
            benches.curvedBench(bench, width: O.benchWidth, height: O.benchHeight)
            collision.addPolyline(bench, radius: O.benchWidth / 2, occludes: false)
        }
        add(benches, Mat.matte(0xDCCFB9, roughness: 0.75), name: "Benches")
    }

    // MARK: - The hang

    func buildHang() {
        for canvas in Hang.canvases {
            let f = SIMD3<Float>(canvas.facing.x, 0, canvas.facing.y)
            hangFramed(id: canvas.id, artworkID: canvas.artworkID, image: canvas.image,
                       wall: [canvas.position.x, canvas.centreHeight, canvas.position.y], facing: f,
                       width: canvas.width, height: canvas.height)
        }

        // The glass stele that carries Impression, Sunrise.
        typealias St = Plan.Salon.Stele
        var stele = MeshBuilder()
        stele.box(min: [St.x - 0.025, 0.08, St.z - St.width / 2], max: [St.x + 0.025, 0.08 + St.height, St.z + St.width / 2])
        add(stele, Mat.glass(0xCFE6E5, opacity: 0.22), name: "Glass stele")
        var foot = MeshBuilder()
        foot.box(min: [St.x - 0.2, 0, St.z - 0.7], max: [St.x + 0.2, 0.08, St.z + 0.7])
        add(foot, Mat.metal(0x5A4632, roughness: 0.45), name: "Stele foot")
        collision.addRect(x0: St.x - 0.2, x1: St.x + 0.2, z0: St.z - 0.7, z1: St.z + 0.7, occludes: false, y1: 2.5)

        // Degas's Little Dancer on her plinth in a glass vitrine (Bay 2).
        let d = Plan.Salon.dancerPlinth
        var plinth = MeshBuilder()
        plinth.box(min: [d.x - 0.4, 0, d.y - 0.4], max: [d.x + 0.4, 1.0, d.y + 0.4])
        add(plinth, Mat.matte(0xE9DFCD, roughness: 0.7), name: "Little Dancer plinth")
        var vitrine = MeshBuilder()
        vitrine.box(min: [d.x - 0.38, 1.0, d.y - 0.38], max: [d.x + 0.38, 2.1, d.y + 0.38])
        add(vitrine, Mat.glass(0xE9F2F1, opacity: 0.12), name: "Little Dancer vitrine")
        placeSculpture(id: "degas-little-dancer", at: [d.x, 1.0, d.y], facing: [1, 0], height: 0.99,
                       material: Mat.metal(0x6B5236, roughness: 0.5), pickBox: nil)
        collision.addRect(x0: d.x - 0.4, x1: d.x + 0.4, z0: d.y - 0.4, z1: d.y + 0.4, occludes: false, y1: 2.1)
        targets.append(PickTarget(artworkID: Hang.dancerArtworkID, hit: { o, dir in
            Self.rayBox(o, dir, min: [d.x - 0.4, 0, d.y - 0.4], max: [d.x + 0.4, 2.1, d.y + 0.4])
        }))

        // The Water Lilies panels on the oval's curved wall, at true proportion, 2 m tall,
        // 0.7 m above the floor, centred on the arcs drawn on the plan.
        typealias O = Plan.Oval
        let a = O.a - 0.03, b = O.b - 0.03
        for panel in Hang.lilyPanels {
            let arc = Poly.ellipse(O.centre, a: a, b: b, from: panel.to, to: panel.from, segments: 200)
            let full = Poly.arcLength(arc)
            let want = min(full, O.panelHeight * panel.aspect)
            // Trim the arc symmetrically to the panel's true width.
            let trim = (full - want) / 2
            var pts: [SIMD2<Float>] = []
            var acc: Float = 0
            for (p, q) in zip(arc, arc.dropFirst()) {
                let l = simd_distance(p, q)
                if acc + l >= trim && acc <= full - trim { if pts.isEmpty { pts.append(p) }; pts.append(q) }
                acc += l
            }
            var ribbon = MeshBuilder()
            let centre = O.centre
            ribbon.ribbon(pts, y0: O.panelBottom, y1: O.panelBottom + O.panelHeight,
                          facing: { -normalize(($0 - centre) / [a * a, b * b]) })
            let e = ModelEntity(mesh: ribbon.mesh(name: panel.id), materials: [placeholder])
            e.name = panel.id
            building.addChild(e)
            let mid = pts[pts.count / 2]
            addImageSlot(e, image: panel.image, maxPixels: 4096, at: [mid.x, 1.7, mid.y])
            let t0 = min(panel.from, panel.to), t1 = max(panel.from, panel.to)
            targets.append(PickTarget(artworkID: Hang.lilyArtworkID, detail: panel.title, hit: { o, d in
                Self.rayOvalPanel(o, d, centre: centre, a: a, b: b, t0: t0, t1: t1,
                                  y0: O.panelBottom, y1: O.panelBottom + O.panelHeight)
            }))
        }
    }

    // MARK: - Light

    func buildSalonLights() {
        // Daylight from each bay's skylight.
        for bay in Plan.Salon.bays {
            let x = (bay.x0 + bay.x1) / 2
            addLight(withShadow(spot(at: [x, 13.2, 0], looking: [x, 0, 0], intensity: 90_000), softness: 1.0))
        }
        // The oval under its velarium.
        addLight(spot(at: [Plan.Oval.centre.x, 5.8, 0], looking: [Plan.Oval.centre.x, 0, 0], colour: 0xFFF6E6,
                      intensity: 70_000, inner: 60, outer: 89, radius: 18))
        // The Manet cabinet's laylight.
        let C = Plan.Salon.Cabinet.self
        let cc = SIMD3<Float>((C.x0 + C.x1) / 2, 0, (C.z0 + C.z1) / 2)
        addLight(spot(at: cc + [0, 5.3, 0], looking: cc, colour: 0xFFF1DC, intensity: 40_000, inner: 60, outer: 88, radius: 12))
    }
}
