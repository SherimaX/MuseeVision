import CoreGraphics
import Foundation
import RealityKit
import simd

/// The sky seen through the eye, the glass hall and the open garden, and — inside Élan's
/// Sphere — all round you. Everything here is at infinity: it follows the camera.
@MainActor
final class SkySystem {
    let root = Entity()
    private let dome = ModelEntity()
    private let stars: Entity
    private let moon = ModelEntity()
    private var lastBand = Int.min
    private var lastMoonKey = -1
    private var timer: Float = 999
    /// When true (inside the Sphere) the whole view is the night sky, day or not.
    var sphereMode = false { didSet { if oldValue != sphereMode { timer = 999 } } }
    private(set) var sunAltitudeDegrees: Float = 30
    private(set) var moonState: (dir: SIMD3<Float>, altitude: Float, illuminated: Float, waxing: Bool)?

    init() {
        root.name = "Sky"
        var b = MeshBuilder()
        // Inverted sphere; v runs 1 (zenith) → 0 (nadir) to match the gradient texture.
        let r: Float = 900, seg = 48, rings = 24
        for j in 0..<rings {
            for i in 0..<seg {
                func p(_ i: Int, _ j: Int) -> SIMD3<Float> {
                    let el = Float.pi / 2 - Float.pi * Float(j) / Float(rings)
                    let az = 2 * Float.pi * Float(i) / Float(seg)
                    return [r * cos(el) * cos(az), r * sin(el), r * cos(el) * sin(az)]
                }
                let a = p(i, j), bb = p(i + 1, j), c = p(i + 1, j + 1), d = p(i, j + 1)
                let v0 = 1 - Float(j) / Float(rings), v1 = 1 - Float(j + 1) / Float(rings)
                let u0 = Float(i) / Float(seg), u1 = Float(i + 1) / Float(seg)
                b.quad(a, bb, c, d, normals: -normalize(a), -normalize(bb), -normalize(c), -normalize(d),
                       uv: ([u0, v0], [u1, v0], [u1, v1], [u0, v1]))
            }
        }
        dome.model = ModelComponent(mesh: b.mesh(name: "sky"), materials: [Mat.glow(0x9FBCD8)])
        dome.name = "Sky dome"
        root.addChild(dome)
        dome.components.set(DynamicLightShadowComponent(castsShadow: false))
        stars = StarField.make(radius: 600)
        for c in stars.children { c.components.set(DynamicLightShadowComponent(castsShadow: false)) }
        root.addChild(stars)
        moon.model = ModelComponent(mesh: .generatePlane(width: 1, height: 1), materials: [Mat.glow(0xFFFFFF)])
        moon.name = "Moon"
        moon.components.set(DynamicLightShadowComponent(castsShadow: false))
        root.addChild(moon)
    }

    func update(dt: Float, eye: SIMD3<Float>, observer: Observer) {
        root.position = eye
        timer += dt
        if timer > 20 {
            timer = 0
            let now = Date()
            let sun = Ephemeris.sun(now, observer)
            sunAltitudeDegrees = sun.altitude * 180 / .pi
            let band = sphereMode ? -99 : (sunAltitudeDegrees >= 8 ? 2 : (sunAltitudeDegrees >= -4 ? 1 : (sunAltitudeDegrees >= -12 ? 0 : -1)))
            if band != lastBand {
                lastBand = band
                let alt: Float = band == 2 ? 30 : (band == 1 ? 0 : (band == 0 ? -8 : -30))
                if band == -99 {
                    // Inside the Sphere the image is sky all round, below your feet too.
                    dome.model?.materials = [Mat.glow(0x0E141C)]
                } else {
                    var m = UnlitMaterial()
                    m.color = .init(tint: .white, texture: .init(Textures.resource(Textures.skyDome(sunAltitude: alt))))
                    dome.model?.materials = [m]
                }
            }
            stars.isEnabled = sphereMode || sunAltitudeDegrees < -6
            stars.orientation = simd_quatf(Ephemeris.skyRotation(now, observer))
            let mo = Ephemeris.moon(now, observer)
            moonState = mo
            let key = Int(mo.illuminated * 40) * 2 + (mo.waxing ? 1 : 0)
            if key != lastMoonKey {
                lastMoonKey = key
                var m = UnlitMaterial()
                m.color = .init(tint: .white, texture: .init(Textures.resource(Textures.moonDisc(illuminated: mo.illuminated, waxing: mo.waxing))))
                m.blending = .transparent(opacity: .init(floatLiteral: 1))
                moon.model?.materials = [m]
            }
            moon.isEnabled = mo.altitude > -0.02 || sphereMode
            let d: Float = 500
            moon.position = mo.dir * d
            // Face the eye; the moon looks about 1.3° across (a little larger than life).
            moon.look(at: .zero, from: moon.position, relativeTo: root)
            moon.orientation *= simd_quatf(angle: .pi, axis: [0, 1, 0])
            let size = d * 0.023
            moon.scale = [size, size, size]
        }
    }
}

extension MuseumScene {
    func buildSky() {
        let sky = SkySystem()
        root.addChild(sky.root)
        skySystem = sky
        updaters.append { [weak self] dt, eye in
            guard let self else { return }
            sky.update(dt: dt, eye: eye, observer: self.observer)
        }
    }
}
