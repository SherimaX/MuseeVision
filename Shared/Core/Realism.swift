import CoreGraphics
import Foundation
import ImageIO
import RealityKit
import simd

/// The first realism pass: stone with veining and relief, polished floors, an interior light
/// probe, shadows and contact shadows. Everything is still generated in code.
enum Stone {
    enum Kind { case travertine, marble, darkStone }

    /// A height field for a stone tile of `slabs` × `slabs` slabs (joints recessed, pores, veins),
    /// returned with its albedo so both match.
    static func tile(_ kind: Kind, px: Int = 512, slabs: Int = 2, seed: UInt64) -> (albedo: CGImage, normal: CGImage) {
        let key = "stone-v2-\(kind)-\(px)-\(slabs)-\(seed)"
        if let a = DiskCache.image(key + "-a"), let n = DiskCache.image(key + "-n") { return (a, n) }
        let made = generate(kind, px: px, slabs: slabs, seed: seed)
        DiskCache.store(made.albedo, key + "-a")
        DiskCache.store(made.normal, key + "-n")
        return made
    }

    private static func generate(_ kind: Kind, px: Int, slabs: Int, seed: UInt64) -> (albedo: CGImage, normal: CGImage) {
        var rng = SplitMix(seed: seed)
        let n = px
        var height = [Float](repeating: 0.5, count: n * n)
        var colour = [SIMD3<Float>](repeating: .zero, count: n * n)
        let base: SIMD3<Float>, alt: SIMD3<Float>, vein: SIMD3<Float>
        switch kind {
        case .travertine: base = [0.945, 0.915, 0.862]; alt = [0.905, 0.868, 0.802]; vein = [0.82, 0.77, 0.69]
        case .marble: base = [0.945, 0.935, 0.915]; alt = [0.905, 0.895, 0.875]; vein = [0.62, 0.60, 0.58]
        case .darkStone: base = [0.42, 0.41, 0.40]; alt = [0.36, 0.355, 0.35]; vein = [0.55, 0.54, 0.52]
        }
        // Smooth value noise.
        func lattice(_ cells: Int) -> [Float] { (0..<(cells * cells)).map { _ in Float(rng.next(0, 1)) } }
        func sample(_ g: [Float], _ cells: Int, _ x: Float, _ y: Float) -> Float {
            let fx = x * Float(cells), fy = y * Float(cells)
            let x0 = Int(floor(fx)) % cells, y0 = Int(floor(fy)) % cells
            let x1 = (x0 + 1) % cells, y1 = (y0 + 1) % cells
            let tx = fx - floor(fx), ty = fy - floor(fy)
            let sx = tx * tx * (3 - 2 * tx), sy = ty * ty * (3 - 2 * ty)
            let a = g[y0 * cells + x0], b = g[y0 * cells + x1], c = g[y1 * cells + x0], d = g[y1 * cells + x1]
            return (a * (1 - sx) + b * sx) * (1 - sy) + (c * (1 - sx) + d * sx) * sy
        }
        let n8 = lattice(8), n24 = lattice(24), n64 = lattice(64), n160 = lattice(160)
        let slabPx = n / slabs
        for y in 0..<n {
            for x in 0..<n {
                let u = Float(x) / Float(n), v = Float(y) / Float(n)
                let big = sample(n8, 8, u, v), mid = sample(n24, 24, u, v), fine = sample(n64, 64, u, v), grain = sample(n160, 160, u, v)
                var c: SIMD3<Float>
                var h: Float = 0.6
                switch kind {
                case .travertine:
                    // Horizontal banding with pores.
                    let band = sin((v * 18 + big * 3 + mid) * .pi) * 0.5 + 0.5
                    c = simd_mix(base, alt, SIMD3(repeating: band * 0.55 + grain * 0.2))
                    if fine > 0.86 && grain > 0.7 { c *= 0.93; h -= 0.15 }
                case .marble, .darkStone:
                    let t = abs(sin((u * 3 + v * 5 + big * 4 + mid * 1.5) * .pi))
                    let veinMask = pow(max(0, 1 - t * 12), 2) * (0.5 + big)
                    c = simd_mix(simd_mix(base, alt, SIMD3(repeating: mid * 0.6)), vein, SIMD3(repeating: min(1, veinMask)))
                    h -= veinMask * 0.05
                }
                // Joints between slabs.
                let jx = x % slabPx, jy = y % slabPx
                if jx < 2 || jy < 2 { c *= slabs == 1 ? 0.93 : 0.84; h = 0.2 }
                colour[y * n + x] = c
                height[y * n + x] = h + grain * 0.04
            }
        }
        let albedo = image(n) { x, y in colour[y * n + x] }
        let normal = image(n) { x, y in
            let l = height[y * n + (x + n - 1) % n], r = height[y * n + (x + 1) % n]
            let d = height[((y + n - 1) % n) * n + x], u = height[((y + 1) % n) * n + x]
            let nrm = normalize(SIMD3<Float>((l - r) * 3, (d - u) * 3, 1))
            return nrm * 0.5 + 0.5
        }
        return (albedo, normal)
    }

    private static func image(_ n: Int, _ f: (Int, Int) -> SIMD3<Float>) -> CGImage {
        var bytes = [UInt8](repeating: 255, count: n * n * 4)
        for y in 0..<n {
            for x in 0..<n {
                let c = simd_clamp(f(x, y), .zero, SIMD3(repeating: 1))
                let i = (y * n + x) * 4
                bytes[i] = UInt8(c.x * 255); bytes[i + 1] = UInt8(c.y * 255); bytes[i + 2] = UInt8(c.z * 255)
            }
        }
        let provider = CGDataProvider(data: Data(bytes) as CFData)!
        return CGImage(width: n, height: n, bitsPerComponent: 8, bitsPerPixel: 32, bytesPerRow: n * 4,
                       space: CGColorSpace(name: CGColorSpace.sRGB)!, bitmapInfo: CGBitmapInfo(rawValue: CGImageAlphaInfo.noneSkipLast.rawValue),
                       provider: provider, decode: nil, shouldInterpolate: true, intent: .defaultIntent)!
    }
}

@MainActor
extension Mat {
    private static var cache: [String: RealityKit.Material] = [:]

    /// Polished stone floor: veined albedo, joint relief, and a glossy clear coat that picks up
    /// the room's light. `tile` is the size in metres that one texture covers (2 × 2 slabs).
    static func polishedStone(_ kind: Stone.Kind, tint: UInt32 = 0xFFFFFF, polish: Float = 1, seed: UInt64 = 1) -> RealityKit.Material {
        let key = "\(kind)-\(tint)-\(polish)-\(seed)"
        if let m = cache[key] { return m }
        let t = Stone.tile(kind, seed: seed)
        var m = PhysicallyBasedMaterial()
        m.baseColor = .init(tint: PlatformColor(hex: tint),
                            texture: .init(Textures.resource(t.albedo), sampler: repeatSampler))
        m.normal = .init(texture: .init(try! TextureResource(image: t.normal, withName: nil,
                                                             options: .init(semantic: .normal, mipmapsMode: .allocateAndGenerateAll)),
                                        sampler: repeatSampler))
        m.roughness = .init(floatLiteral: kind == .travertine ? 0.55 : 0.4)
        m.metallic = .init(floatLiteral: 0)
        m.clearcoat = .init(floatLiteral: 0.9 * polish)
        m.clearcoatRoughness = .init(floatLiteral: 0.06 + (1 - polish) * 0.3)
        cache[key] = m
        return m
    }

    /// Honed stone for walls and vaults: the same stone, matte, with relief.
    static func honedStone(_ kind: Stone.Kind, tint: UInt32 = 0xFFFFFF, seed: UInt64 = 2) -> RealityKit.Material {
        let key = "honed-\(kind)-\(tint)-\(seed)"
        if let m = cache[key] { return m }
        let t = Stone.tile(kind, px: 512, slabs: 1, seed: seed)
        var m = PhysicallyBasedMaterial()
        m.baseColor = .init(tint: PlatformColor(hex: tint), texture: .init(Textures.resource(t.albedo), sampler: repeatSampler))
        m.normal = .init(texture: .init(try! TextureResource(image: t.normal, withName: nil,
                                                             options: .init(semantic: .normal, mipmapsMode: .allocateAndGenerateAll)),
                                        sampler: repeatSampler))
        m.roughness = .init(floatLiteral: 0.8)
        m.metallic = .init(floatLiteral: 0)
        cache[key] = m
        return m
    }
}

extension Textures {
    /// An interior light probe: warm stone all round, bright daylight overhead, a few bright
    /// openings. Lights the museum and gives polished surfaces something to reflect.
    static func interiorProbe() -> CGImage {
        draw(width: 1024, height: 512) { ctx in
            let cs = CGColorSpace(name: CGColorSpace.sRGB)!
            let g = CGGradient(colorsSpace: cs, colors: [cg(0xFFFDF7), cg(0xF6EEDF), cg(0xE6D9C3), cg(0xD2C3AA), cg(0xB9A98F)] as CFArray,
                               locations: [0, 0.22, 0.48, 0.62, 1])!
            ctx.drawLinearGradient(g, start: CGPoint(x: 0, y: 512), end: CGPoint(x: 0, y: 0), options: [])
            // Bright openings (doors, skylights) along the horizon and overhead.
            ctx.setFillColor(cg(0xFFFFFF, 0.9))
            for i in 0..<4 {
                let x = CGFloat(i) * 256 + 110
                ctx.fill(CGRect(x: x, y: 250, width: 36, height: 70))
            }
            ctx.setFillColor(cg(0xFFFFFF))
            ctx.fillEllipse(in: CGRect(x: 380, y: 470, width: 260, height: 60))
            // Darker picture band.
            ctx.setFillColor(cg(0x8C6A55, 0.25))
            ctx.fill(CGRect(x: 0, y: 230, width: 1024, height: 16))
        }
    }
}

extension MuseumScene {
    /// Contact shadows under a free-standing object.
    func groundShadow(_ e: Entity) {
        e.components.set(GroundingShadowComponent(castsShadow: true))
    }

    /// Gives a spot light a soft shadow map.
    /// `softness` is the light's apparent size in metres (iOS 27 soft shadows).
    func withShadow(_ e: Entity, softness: Float = 0.4) -> Entity {
        var s = SpotLightComponent.Shadow()
        s.depthBias = 1.5
        if #available(iOS 27.0, visionOS 27.0, *) {
            s.quality = .high
            s.lightSize = softness
        }
        e.components.set(s)
        return e
    }
}

// MARK: - Coffers as real geometry

extension MeshBuilder {
    /// Coffers on a curved surface. `P(u, v, depth)` maps grid coordinates to a point, pushed
    /// `depth` metres away from the viewer (into the vault). Each cell gets a rib frame, a
    /// stepped bevel and a recessed panel. `centre` is a point on the viewer's side, used to
    /// face the normals inwards. `skip` leaves cells open (skylights).
    mutating func coffers(us: [Float], vs: [Float], depth: Float, rib: Float = 0.15, centre: (SIMD3<Float>) -> SIMD3<Float>,
                          skip: (Int, Int) -> Bool = { _, _ in false },
                          _ P: (Float, Float, Float) -> SIMD3<Float>) {
        func face(_ a: SIMD3<Float>, _ b: SIMD3<Float>, _ c: SIMD3<Float>, _ d: SIMD3<Float>) {
            var n = cross(b - a, d - a)
            if simd_length(n) < 1e-9 { n = cross(c - b, a - b) }
            n = simd_length(n) > 1e-9 ? normalize(n) : [0, -1, 0]
            let mid = (a + b + c + d) / 4
            if dot(n, centre(mid) - mid) < 0 { n = -n }
            quad(a, b, c, d, normal: n)
        }
        for i in 0..<(us.count - 1) {
            for j in 0..<(vs.count - 1) where !skip(i, j) {
                let u0 = us[i], u1 = us[i + 1], v0 = vs[j], v1 = vs[j + 1]
                let du = u1 - u0, dv = v1 - v0
                // Rings of the frame: outer cell edge, opening, step, panel.
                let insets: [(Float, Float)] = [(0, 0), (rib, 0), (rib + 0.08, 0.45), (rib + 0.14, 1)]
                var rings: [[SIMD3<Float>]] = []
                for (f, d) in insets {
                    let a0 = u0 + du * f, a1 = u1 - du * f, b0 = v0 + dv * f, b1 = v1 - dv * f
                    rings.append([P(a0, b0, d * depth), P(a1, b0, d * depth), P(a1, b1, d * depth), P(a0, b1, d * depth)])
                }
                for k in 0..<3 {
                    let o = rings[k], n = rings[k + 1]
                    for e in 0..<4 {
                        let e1 = (e + 1) % 4
                        face(o[e], o[e1], n[e1], n[e])
                    }
                }
                let p = rings[3]
                face(p[0], p[1], p[2], p[3])
            }
        }
    }
}

// MARK: - Contact shading (soft darkening where walls meet floors)

extension Textures {
    static func contactGradient() -> CGImage {
        draw(width: 8, height: 64) { ctx in
            let cs = CGColorSpace(name: CGColorSpace.sRGB)!
            let g = CGGradient(colorsSpace: cs, colors: [cg(0x1A140C, 0.42), cg(0x1A140C, 0.12), cg(0x1A140C, 0)] as CFArray,
                               locations: [0, 0.35, 1])!
            ctx.drawLinearGradient(g, start: CGPoint(x: 0, y: 0), end: CGPoint(x: 0, y: 64), options: [])
        }
    }
}

extension MuseumScene {
    /// Soft darkening on the floor and the foot of the wall along a wall run (skips door openings).
    func contactShade(_ run: WallRun, base: Float = 0, width: Float = 0.45, parent: Entity? = nil) {
        var b = MeshBuilder()
        let st = run.stations(step: 0.3)
        for (s0, s1) in zip(st, st.dropFirst()) where s1 - s0 > 1e-4 {
            let sm = (s0 + s1) / 2
            if run.openings.contains(where: { $0.through && $0.bottom(at: sm - $0.center) < 0.3 && abs(sm - $0.center) < $0.width / 2 }) {
                continue
            }
            let a = run.sample(s0), c = run.sample(s1)
            let ia = a.p - a.out * width, ic = c.p - c.out * width
            let y = base + 0.004
            // Floor strip: v 0 at the wall → 1 into the room.
            b.quad([a.p.x, y, a.p.y], [c.p.x, y, c.p.y], [ic.x, y, ic.y], [ia.x, y, ia.y], normal: [0, 1, 0],
                   uv: ([0, 1], [1, 1], [1, 0], [0, 0]))
            // Wall strip: v 0 at the floor → 1 up the wall.
            let na = SIMD3<Float>(-a.out.x, 0, -a.out.y)
            let wa = a.p - a.out * 0.003, wc = c.p - c.out * 0.003
            b.quad([wa.x, base, wa.y], [wc.x, base, wc.y], [wc.x, base + width * 0.8, wc.y], [wa.x, base + width * 0.8, wa.y],
                   normal: na, uv: ([0, 1], [1, 1], [1, 0], [0, 0]))
        }
        var m = UnlitMaterial()
        m.color = .init(tint: .white, texture: .init(contactTexture))
        m.blending = .transparent(opacity: .init(floatLiteral: 1))
        add(b, m, name: "Contact shade", to: parent)
    }
}

/// Generated textures are cached on disk so they are only computed on the first launch.
enum DiskCache {
    static var folder: URL {
        let url = FileManager.default.urls(for: .cachesDirectory, in: .userDomainMask)[0].appendingPathComponent("GeneratedTextures")
        try? FileManager.default.createDirectory(at: url, withIntermediateDirectories: true)
        return url
    }

    static func image(_ key: String) -> CGImage? {
        let url = folder.appendingPathComponent(key + ".png")
        guard let src = CGImageSourceCreateWithURL(url as CFURL, nil) else { return nil }
        return CGImageSourceCreateImageAtIndex(src, 0, nil)
    }

    static func store(_ image: CGImage, _ key: String) {
        let url = folder.appendingPathComponent(key + ".png")
        guard let dest = CGImageDestinationCreateWithURL(url as CFURL, "public.png" as CFString, 1, nil) else { return }
        CGImageDestinationAddImage(dest, image, nil)
        CGImageDestinationFinalize(dest)
    }
}
