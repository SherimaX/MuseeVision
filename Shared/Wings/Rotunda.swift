import CoreGraphics
import Foundation
import RealityKit
import simd

extension MuseumScene {
    // MARK: - Rotunda

    func buildRotunda() {
        typealias R = Plan.Rotunda
        let deg = Float.pi / 180
        // Floor: the sun clock, drawn as one texture over the whole Ø 20 m floor.
        var floor = MeshBuilder()
        let n = 128
        for i in 0..<n {
            let a0 = 2 * Float.pi * Float(i) / Float(n), a1 = 2 * Float.pi * Float(i + 1) / Float(n)
            let r: Float = R.radius + 0.3
            let p0 = SIMD3<Float>(r * cos(a0), 0, r * sin(a0)), p1 = SIMD3<Float>(r * cos(a1), 0, r * sin(a1))
            func uv(_ p: SIMD3<Float>) -> SIMD2<Float> { [(p.x + 10) / 20, (10 - p.z) / 20] }
            floor.tri(.zero, p0, p1, [0, 1, 0], [0, 1, 0], [0, 1, 0], uv(.zero), uv(p0), uv(p1))
        }
        var floorMat = PhysicallyBasedMaterial()
        floorMat.baseColor = .init(tint: .white, texture: .init(Textures.resource(Textures.sunClockFloor())))
        floorMat.roughness = .init(floatLiteral: 0.5)
        floorMat.metallic = .init(floatLiteral: 0)
        floorMat.clearcoat = .init(floatLiteral: 0.9)
        floorMat.clearcoatRoughness = .init(floatLiteral: 0.07)
        add(floor, floorMat, name: "Sun clock")

        // Drum wall: a loop starting at 22.5° so no opening straddles the seam. Plan angle θ
        // is measured from east towards south (x = cos θ, z = sin θ).
        let start = 22.5 * deg
        let loop = Poly.circle(.zero, r: R.radius, from: start, to: start + 2 * .pi + 0.01, segments: 256)
        func s(_ theta: Float) -> Float { R.radius * (theta - start) }
        var openings: [WallOpening] = [
            WallOpening(center: s(90 * deg), width: 4, spring: R.doorSpring),    // S · Chinese Wing
            WallOpening(center: s(180 * deg), width: 3, spring: R.doorSpring),   // W · Salon (open)
            WallOpening(center: s(270 * deg), width: 4, spring: R.doorSpring),   // N · Sculpture Hall
            WallOpening(center: s(360 * deg), width: 3, spring: R.doorSpring),   // E · Hall of Light
        ]
        for a in [45, 135, 225, 315] {
            openings.append(WallOpening(center: s(Float(a) * deg), width: R.nicheWidth, spring: R.nicheSpring,
                                        sill: R.nicheSill, through: false))
        }
        let drum = WallRun(points: loop, inside: .right, height: R.drumHeight, thickness: R.wall, openings: openings)
        var wall = MeshBuilder()
        wall.wall(drum)
        add(wall, Mat.honedStone(.travertine, tint: 0xFFFBF3, seed: 3), name: "Drum")
        var moulding = MeshBuilder()
        moulding.band(drum, from: 9.55, to: 10.0, depth: 0.32)
        moulding.band(drum, from: 0, to: 0.18, depth: 0.04)
        add(moulding, Mat.matte(Mat.moulding), name: "Drum mouldings")
        collision.add(run: drum)
        contactShade(drum)

        // Sixteen pilasters framing the doors and niches.
        var pil = MeshBuilder()
        for k in 0..<R.pilasters {
            let a = (11.25 + 22.5 * Float(k)) * deg
            let r = R.radius - R.pilasterDepth / 2 + 0.02
            let c = SIMD3<Float>(r * cos(a), R.pilasterHeight / 2, r * sin(a))
            pil.box(center: c, size: [R.pilasterDepth, R.pilasterHeight, R.pilasterWidth], yaw: -a)
            let cap = SIMD3<Float>((r - 0.06) * cos(a), R.pilasterHeight - 0.2, (r - 0.06) * sin(a))
            pil.box(center: cap, size: [R.pilasterDepth + 0.12, 0.4, R.pilasterWidth + 0.16], yaw: -a)
            let base = SIMD3<Float>((r - 0.04) * cos(a), 0.15, (r - 0.04) * sin(a))
            pil.box(center: base, size: [R.pilasterDepth + 0.08, 0.3, R.pilasterWidth + 0.1], yaw: -a)
            // Collision outline of the pilaster.
            let radial = SIMD2<Float>(cos(a), sin(a)), tangent = SIMD2<Float>(-sin(a), cos(a))
            let inner = radial * (R.radius - R.pilasterDepth - 0.04)
            let outer = radial * R.radius
            let hw = R.pilasterWidth / 2 + 0.05
            collision.addPolyline([outer - tangent * hw, inner - tangent * hw, inner + tangent * hw, outer + tangent * hw])
        }
        add(pil, Mat.honedStone(.travertine, tint: 0xFFF9EE), name: "Pilasters")

        // Dome: 5 rings × 28 coffers between plain bands, up to the Ø 5 m eye.
        let centre = SIMD3<Float>(0, R.drumHeight, 0)
        let eyeEl = acos(R.oculusRadius / R.radius)
        var plain = MeshBuilder()
        plain.domeBand(center: centre, radius: R.radius, from: 0, to: 4 * deg, rings: 2)
        plain.domeBand(center: centre, radius: R.radius, from: 64 * deg, to: eyeEl, rings: 4)
        add(plain, Mat.matte(Mat.stone), name: "Dome")
        // 5 rings × 28 real coffers, each a rib frame, a stepped bevel and a recessed panel.
        var coffers = MeshBuilder()
        let us = (0...R.coffers.perRing).map { 2 * Float.pi * Float($0) / Float(R.coffers.perRing) }
        let vs = (0...R.coffers.rings).map { (4 + 60 * Float($0) / Float(R.coffers.rings)) * deg }
        coffers.coffers(us: us, vs: vs, depth: 0.45, rib: 0.13, centre: { _ in centre }) { a, e, d in
            let r = R.radius + d
            return centre + SIMD3<Float>(r * cos(e) * cos(a), r * sin(e), r * cos(e) * sin(a))
        }
        add(coffers, Mat.honedStone(.travertine, tint: 0xFFFAF0, seed: 9), name: "Dome coffers")

        // The eye: a short curb, then the steel-and-glass lattice with its bronze node.
        let eyeY = R.drumHeight + R.radius * sin(eyeEl)
        var curb = MeshBuilder()
        let ring = Poly.circle(.zero, r: R.oculusRadius, from: 0, to: 2 * .pi, segments: 64)
        curb.ribbon(ring, y0: eyeY - 0.05, y1: R.latticeHeight, facing: { -$0 / simd_length($0) })
        add(curb, Mat.matte(Mat.stone), name: "Oculus curb")
        var lattice = MeshBuilder()
        for dirDeg in [0, 60, 120] {
            let a = Float(dirDeg) * deg
            let d = SIMD2<Float>(cos(a), sin(a)), nrm = SIMD2<Float>(-sin(a), cos(a))
            for k in -3...3 {
                let off = Float(k) * 0.8
                let half = sqrt(max(0, R.oculusRadius * R.oculusRadius - off * off))
                if half < 0.1 { continue }
                let c2 = nrm * off
                lattice.box(center: [c2.x, R.latticeHeight, c2.y], size: [2 * half, 0.1, 0.05], yaw: -a)
                _ = d
            }
        }
        lattice.ribbon(ring, y0: R.latticeHeight - 0.06, y1: R.latticeHeight + 0.06, facing: { -$0 / simd_length($0) })
        add(lattice, Mat.metal(0x3C3F42, roughness: 0.4), name: "Lattice")
        var glass = MeshBuilder()
        glass.ellipseDisc(center: [0, R.latticeHeight + 0.06, 0], a: R.oculusRadius, b: R.oculusRadius, up: false)
        add(glass, Mat.glass(0xE3EFEF, opacity: 0.12), name: "Oculus glass")
        let node = ModelEntity(mesh: .generateSphere(radius: 0.12), materials: [Mat.metal(0x7E5C25, roughness: 0.3)])
        node.position = [0, R.latticeHeight - 0.05, 0]
        node.name = "Bronze node"
        building.addChild(node)

        // Sun patch and the node's point of shadow on the clock (positioned by updateSun).
        sunPatch.model = ModelComponent(mesh: .generatePlane(width: 1, depth: 1, cornerRadius: 0.5),
                                        materials: [Mat.glow(0xFFF3D0)])
        sunPatch.components.set(OpacityComponent(opacity: 0.22))
        sunPatch.name = "Sun patch"
        building.addChild(sunPatch)
        shadowPoint.model = ModelComponent(mesh: .generatePlane(width: 0.22, depth: 0.22, cornerRadius: 0.11),
                                           materials: [Mat.glow(0x2A2016)])
        shadowPoint.name = "Point of shadow"
        building.addChild(shadowPoint)

        // Daylight through the eye, and the idealised sun that keeps the hour on the clock.
        addLight(withShadow(spot(at: [0, 19.5, 0], looking: .zero, colour: 0xFFF7EA, intensity: 140_000, inner: 40, outer: 80, radius: 30), softness: 1.2))
        // The sun: one directional light for the whole museum, following the Rotunda's idealised
        // sun, so the eye and every skylight let in a pool of sunlight with real shadows (the
        // lattice and the bronze node draw their own shadow on the clock).
        sunLight.name = "Sun"
        sunLight.components.set(DirectionalLightComponent(color: PlatformColor(hex: 0xFFEFD6), intensity: 9_000))
        sunLight.components.set(DirectionalLightComponent.Shadow(shadowProjection: .automatic(maximumDistance: 60), depthBias: 2.0,
                                                                  cullMode: MaterialParameterTypes.FaceCulling.none))
        root.addChild(sunLight)
        updateSun(Self.now())
        var sunTimer: Float = 0
        updaters.append { [weak self] dt, _ in
            sunTimer += dt
            if sunTimer > 20 { sunTimer = 0; self?.updateSun(Self.now()) }
        }
    }

    // MARK: - Passage from the Rotunda to Bay 1

    func buildPassage() {
        typealias P = Plan.Passage
        let x0: Float = -10.6, x1 = P.x1
        let hw = P.halfWidth
        let spring = Plan.Rotunda.doorSpring
        var b = MeshBuilder()
        let north = WallRun(points: [[x0, -hw], [x1, -hw]], inside: .left, height: spring, thickness: 0.3)
        let south = WallRun(points: [[x0, hw], [x1, hw]], inside: .right, height: spring, thickness: 0.3)
        b.wall(north, faces: (true, false))
        b.wall(south, faces: (true, false))
        // Arched ceiling continuing the Rotunda door's arch.
        for i in 0..<32 {
            let a0 = Float.pi * Float(i) / 32, a1 = Float.pi * Float(i + 1) / 32
            func p(_ x: Float, _ a: Float) -> SIMD3<Float> { [x, spring + hw * sin(a), hw * cos(a)] }
            let n0 = SIMD3<Float>(0, -sin(a0), -cos(a0)), n1 = SIMD3<Float>(0, -sin(a1), -cos(a1))
            b.quad(p(x0, a0), p(x1, a0), p(x1, a1), p(x0, a1), normals: n0, n0, n1, n1)
        }
        add(b, Mat.matte(0xF1EBDF), name: "Passage")
        collision.addPolyline([[-10.9, -hw], [x1, -hw]])
        collision.addPolyline([[-10.9, hw], [x1, hw]])
        var f = MeshBuilder()
        f.floorRect(x0: x1 - 0.7, x1: -9.8, z0: -hw, z1: hw, y: -0.002, up: true, tile: 1.2)
        add(f, Mat.polishedStone(.travertine), name: "Passage floor")
    }

    func updateSun(_ date: Date) {
        let cal = Calendar.current
        let comps = cal.dateComponents([.hour, .minute], from: date)
        let hour = Float(comps.hour ?? 12) + Float(comps.minute ?? 0) / 60
        let day = hour >= 6 && hour <= 18
        sunLight.isEnabled = day
        shadowPoint.isEnabled = false
        sunPatch.isEnabled = false
        guard day else { return }
        // Angle from west (VI) through north (XII) to east (VI); the point of shadow sits 6.78 m out.
        let phi = (hour - 6) * 15 * .pi / 180
        let r: Float = 6.78
        let p = SIMD3<Float>(-cos(phi) * r, 0, -sin(phi) * r)
        let node = SIMD3<Float>(0, Plan.Rotunda.latticeHeight, 0)
        sunLight.position = node
        sunLight.look(at: p, from: node, relativeTo: nil)
    }
}
