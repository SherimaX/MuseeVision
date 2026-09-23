import CoreGraphics
import Foundation
import ImageIO
import RealityKit
import simd
#if canImport(AppKit)
import AppKit
#endif

/// What the exporter can read from a RealityKit material. Two materials with the same fields
/// become one USD material.
struct MaterialInfo: Hashable {
    enum Kind: String { case pbr, unlit, painting, other }
    var kind: Kind
    /// sRGB, 0…1, rounded to 8 bits.
    var tint: SIMD4<Float> = [1, 1, 1, 1]
    /// The texture's name (see TextureNames), the painting's image id, or nil.
    var texture: String?
    var roughness: Float = 1
    var metallic: Float = 0
    var opacity: Float = 1
    var clearcoat: Float = 0
    var clearcoatRoughness: Float = 0
    var normalMap = false

    var tintHex: String {
        let c = simd_clamp(tint, .zero, SIMD4(repeating: 1)) * 255
        return String(format: "%02X%02X%02X", Int(c.x.rounded()), Int(c.y.rounded()), Int(c.z.rounded()))
    }

    /// matte / metal / glass / glow: the family a colour-only material is named by.
    var family: String {
        switch kind {
        case .unlit: return "glow"
        case .painting: return "painting"
        case .other: return "material"
        case .pbr: return opacity < 0.999 ? "glass" : (metallic >= 0.5 ? "metal" : "matte")
        }
    }
}

/// Collects the distinct materials, names them stably, and writes Materials.usda and the
/// textures they use.
@MainActor
final class MaterialTable {
    private(set) var infos: [MaterialInfo] = []
    private var index: [MaterialInfo: Int] = [:]
    private(set) var names: [String] = []
    /// Texture images by texture name (drawn in code; written out as PNG).
    private var images: [String: CGImage] = [:]
    private var unnamedTextures: [ObjectIdentifier: String] = [:]
    /// Metres per texture repeat, gathered from the meshes that use each material.
    private var tiling: [Int: [SIMD2<Float>]] = [:]
    private var imageMapped: Set<Int> = []
    private var doubleSided: Set<Int> = []

    static let root = "/Museum/Materials"

    func path(_ i: Int) -> String { Self.root + "/" + names[i] }

    // MARK: Reading RealityKit materials

    func info(_ m: RealityKit.Material) -> MaterialInfo {
        if let p = m as? PhysicallyBasedMaterial {
            var i = MaterialInfo(kind: .pbr)
            i.tint = rgba(p.baseColor.tint)
            i.texture = p.baseColor.texture.map { textureName($0.resource) }
            i.roughness = round3(p.roughness.scale)
            i.metallic = round3(p.metallic.scale)
            if case .transparent(let o) = p.blending { i.opacity = round3(o.scale) }
            i.clearcoat = round3(p.clearcoat.scale)
            i.clearcoatRoughness = i.clearcoat > 0 ? round3(p.clearcoatRoughness.scale) : 0
            i.normalMap = p.normal.texture != nil
            return i
        }
        if let u = m as? UnlitMaterial {
            var i = MaterialInfo(kind: .unlit)
            i.tint = rgba(u.color.tint)
            i.texture = u.color.texture.map { textureName($0.resource) }
            if case .transparent(let o) = u.blending { i.opacity = round3(o.scale) }
            i.roughness = 1
            return i
        }
        return MaterialInfo(kind: .other, texture: String(describing: type(of: m)))
    }

    static func isDoubleSided(_ m: RealityKit.Material) -> Bool {
        if let p = m as? PhysicallyBasedMaterial { return p.faceCulling == .none }
        if let u = m as? UnlitMaterial { return u.faceCulling == .none }
        return false
    }

    private func round3(_ x: Float) -> Float { (x * 1000).rounded() / 1000 }

    private func rgba(_ c: PlatformColor) -> SIMD4<Float> {
        #if canImport(AppKit)
        let s = c.usingColorSpace(.sRGB) ?? c
        let v = SIMD4<Float>(Float(s.redComponent), Float(s.greenComponent), Float(s.blueComponent), Float(s.alphaComponent))
        #else
        var r: CGFloat = 0, g: CGFloat = 0, b: CGFloat = 0, a: CGFloat = 0
        c.getRed(&r, green: &g, blue: &b, alpha: &a)
        let v = SIMD4<Float>(Float(r), Float(g), Float(b), Float(a))
        #endif
        return (simd_clamp(v, .zero, SIMD4(repeating: 1)) * 255).rounded(.toNearestOrEven) / 255
    }

    private func textureName(_ t: TextureResource) -> String {
        if let named = TextureNames.lookup(t) {
            images[named.name] = images[named.name] ?? named.image
            return named.name
        }
        if let n = unnamedTextures[ObjectIdentifier(t)] { return n }
        let n = "texture_\(unnamedTextures.count + 1)"
        unnamedTextures[ObjectIdentifier(t)] = n
        return n
    }

    // MARK: Registering uses

    func add(_ i: MaterialInfo) -> Int {
        if let k = index[i] { return k }
        infos.append(i)
        index[i] = infos.count - 1
        return infos.count - 1
    }

    func noteUse(_ k: Int, mesh: MeshData, doubleSided ds: Bool) {
        if mesh.uvMode == "image" { imageMapped.insert(k) } else if mesh.uvMode != "box" { tiling[k, default: []].append(mesh.uvScale) }
        if ds { doubleSided.insert(k) }
    }

    // MARK: Names

    /// Names every material: paintings by image, textured ones by texture, the rest by colour
    /// (a palette name where there is one). Materials that would share a name get suffixes from
    /// what tells them apart (colour, roughness, opacity…), so names depend only on content.
    func finalizeNames() {
        var base = infos.map(baseName)
        var groups: [String: [Int]] = [:]
        for (k, b) in base.enumerated() { groups[b, default: []].append(k) }
        for (b, members) in groups where members.count > 1 {
            let descriptors: [(Int) -> String] = [
                { self.infos[$0].tintHex },
                { "r" + String(Int((self.infos[$0].roughness * 100).rounded())) },
                { "o" + String(Int((self.infos[$0].opacity * 100).rounded())) },
                { "m" + String(Int((self.infos[$0].metallic * 100).rounded())) },
                { "c" + String(Int((self.infos[$0].clearcoat * 100).rounded())) },
                { self.infos[$0].kind.rawValue },
                { self.infos[$0].normalMap ? "n" : "flat" },
            ]
            var chosen: [(Int) -> String] = []
            var names: [String] = members.map { _ in b }
            for d in descriptors {
                let values = Set(members.map(d))
                guard values.count > 1 else { continue }
                chosen.append(d)
                names = members.map { m in ([b] + chosen.map { $0(m) }).joined(separator: "_") }
                if Set(names).count == members.count { break }
            }
            if Set(names).count < members.count {
                // Still ambiguous: number them in a content order.
                let order = members.sorted { String(describing: infos[$0]) < String(describing: infos[$1]) }
                for (n, m) in order.enumerated() { names[members.firstIndex(of: m)!] += "_\(n + 1)" }
            }
            for (m, n) in zip(members, names) { base[m] = n }
        }
        names = base
    }

    private func baseName(_ i: MaterialInfo) -> String {
        if i.kind == .painting, let t = i.texture { return USDA.identifier("painting_" + t) }
        if let t = i.texture { return USDA.identifier(t) }
        if let p = Palette.names[i.family + ":" + i.tintHex] { return p }
        return i.family + "_" + i.tintHex
    }

    // MARK: Writing

    static func linear(_ c: Float) -> Float { c <= 0.04045 ? c / 12.92 : pow((c + 0.055) / 1.055, 2.4) }

    func linearRGB(_ i: MaterialInfo) -> SIMD3<Float> {
        SIMD3(Self.linear(i.tint.x), Self.linear(i.tint.y), Self.linear(i.tint.z)).rounded4
    }

    /// Writes Materials.usda into `layer` and the PNGs into `textureDir`. Returns the texture files written.
    func write(into layer: LayerFile, textureDir: URL, used: Set<Int>) throws -> [String] {
        let scope = layer.spec(["Museum", "Materials"])
        scope.specifier = "def"
        scope.typeName = "Scope"
        var written: [String] = []
        try FileManager.default.createDirectory(at: textureDir, withIntermediateDirectories: true)
        for k in used.sorted(by: { names[$0] < names[$1] }) {
            let i = infos[k]
            let name = names[k]
            let m = scope.child(name)
            m.specifier = "def"
            m.typeName = "Material"
            let base = Self.root + "/" + name
            m.properties.append("token outputs:surface.connect = <\(base)/Surface.outputs:surface>")
            let source = i.kind == .painting ? "painting" : (i.kind == .unlit ? "UnlitMaterial" : (i.kind == .pbr ? "PhysicallyBasedMaterial" : (i.texture ?? "other")))
            m.properties.append("custom string museevision:source = \(USDA.string(source))")
            if i.kind != .painting && i.kind != .other {
                m.properties.append("custom string museevision:tint = \(USDA.string("#" + i.tintHex))")
            }
            if i.kind == .unlit { m.properties.append("custom bool museevision:unlit = 1") }
            if doubleSided.contains(k) { m.properties.append("custom bool museevision:doubleSided = 1") }

            let surface = m.child("Surface")
            surface.specifier = "def"
            surface.typeName = "Shader"
            var sp = ["uniform token info:id = \"UsdPreviewSurface\""]
            let colour = linearRGB(i)

            // A texture: a painting's image in assets/, or a drawn texture written as PNG.
            var file: String?
            if i.kind == .painting, let id = i.texture {
                file = "../assets/paintings/\(id).jpg"
            } else if let t = i.texture, let img = images[t] {
                let fileName = USDA.identifier(t) + ".png"
                let url = textureDir.appendingPathComponent(fileName)
                if !written.contains(fileName) {
                    try writePNG(img, to: url)
                    written.append(fileName)
                }
                file = "./textures/" + fileName
            }
            let transparent = i.opacity < 0.999
            if let file {
                let uv = m.child("UV")
                uv.specifier = "def"; uv.typeName = "Shader"
                uv.properties = ["uniform token info:id = \"UsdPrimvarReader_float2\"",
                                 "string inputs:varname = \"st\"",
                                 "float2 outputs:result"]
                var stSource = "\(base)/UV.outputs:result"
                // Tiled textures: UV0 is in metres, so scale it back to one repeat per tile.
                if !imageMapped.contains(k), let tiles = tiling[k], !tiles.isEmpty {
                    let sorted = tiles.sorted { $0.x * $0.y < $1.x * $1.y }
                    let t = sorted[sorted.count / 2]
                    if t != [1, 1] {
                        let place = m.child("Tiling")
                        place.specifier = "def"; place.typeName = "Shader"
                        place.properties = ["uniform token info:id = \"UsdTransform2d\"",
                                            "float2 inputs:in.connect = <\(base)/UV.outputs:result>",
                                            "float2 inputs:scale = \(USDA.v2(SIMD2(1 / t.x, 1 / t.y).rounded4))",
                                            "float2 outputs:result"]
                        stSource = "\(base)/Tiling.outputs:result"
                    }
                }
                let tex = m.child("Image")
                tex.specifier = "def"; tex.typeName = "Shader"
                let wrap = imageMapped.contains(k) ? "clamp" : "repeat"
                tex.properties = ["uniform token info:id = \"UsdUVTexture\"",
                                  "asset inputs:file = @\(file)@",
                                  "float2 inputs:st.connect = <\(stSource)>",
                                  "token inputs:sourceColorSpace = \"sRGB\"",
                                  "float4 inputs:scale = (\(USDA.f(colour.x)), \(USDA.f(colour.y)), \(USDA.f(colour.z)), 1)",
                                  "token inputs:wrapS = \"\(wrap)\"",
                                  "token inputs:wrapT = \"\(wrap)\"",
                                  "float outputs:a",
                                  "float3 outputs:rgb"]
                sp.append("color3f inputs:diffuseColor.connect = <\(base)/Image.outputs:rgb>")
                if i.kind == .unlit { sp.append("color3f inputs:emissiveColor.connect = <\(base)/Image.outputs:rgb>") }
                if transparent && i.kind == .unlit {
                    sp.append("float inputs:opacity.connect = <\(base)/Image.outputs:a>")
                }
            } else {
                sp.append("color3f inputs:diffuseColor = \(USDA.v3(colour))")
                if i.kind == .unlit { sp.append("color3f inputs:emissiveColor = \(USDA.v3(colour))") }
            }
            if !(transparent && i.kind == .unlit && file != nil) {
                sp.append("float inputs:opacity = \(USDA.f(i.opacity))")
            }
            sp.append("float inputs:roughness = \(USDA.f(i.kind == .painting ? 0.6 : i.roughness))")
            sp.append("float inputs:metallic = \(USDA.f(i.metallic))")
            if i.clearcoat > 0 {
                sp.append("float inputs:clearcoat = \(USDA.f(i.clearcoat))")
                sp.append("float inputs:clearcoatRoughness = \(USDA.f(i.clearcoatRoughness))")
            }
            sp.append("token outputs:surface")
            surface.properties = sp
        }
        return written
    }

    private func writePNG(_ image: CGImage, to url: URL) throws {
        guard let dest = CGImageDestinationCreateWithURL(url as CFURL, "public.png" as CFString, 1, nil) else {
            throw ExportError("cannot write \(url.path)")
        }
        CGImageDestinationAddImage(dest, image, nil)
        guard CGImageDestinationFinalize(dest) else { throw ExportError("cannot write \(url.path)") }
    }
}

extension SIMD3 where Scalar == Float {
    var rounded4: SIMD3<Float> { (self * 10000).rounded(.toNearestOrEven) / 10000 }
}

extension SIMD2 where Scalar == Float {
    var rounded4: SIMD2<Float> { (self * 10000).rounded(.toNearestOrEven) / 10000 }
}

struct ExportError: Error, CustomStringConvertible {
    let description: String
    init(_ d: String) { description = d }
}
