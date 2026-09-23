import Metal
import RealityKit

#if canImport(UIKit)
import UIKit
typealias PlatformColor = UIColor
#else
import AppKit
typealias PlatformColor = NSColor
#endif

extension PlatformColor {
    convenience init(hex: UInt32, alpha: CGFloat = 1) {
        self.init(red: CGFloat((hex >> 16) & 0xFF) / 255, green: CGFloat((hex >> 8) & 0xFF) / 255,
                  blue: CGFloat(hex & 0xFF) / 255, alpha: alpha)
    }
}

/// The museum's palette of materials (colours from the boards).
@MainActor
enum Mat {
    static let stone: UInt32 = 0xEFE8DB       // pilasters, piers, arches, vaults
    static let moulding: UInt32 = 0xE4DBCB    // cornices and skirting
    static let gilt: UInt32 = 0xC9A266
    static let bronze: UInt32 = 0x6B4E2E

    static var repeatSampler: MaterialParameters.Texture.Sampler = {
        let d = MTLSamplerDescriptor()
        d.sAddressMode = .repeat
        d.tAddressMode = .repeat
        d.minFilter = .linear
        d.magFilter = .linear
        d.mipFilter = .linear
        d.maxAnisotropy = 8
        return MaterialParameters.Texture.Sampler(d)
    }()

    static func matte(_ hex: UInt32, roughness: Float = 0.88) -> RealityKit.Material {
        var m = PhysicallyBasedMaterial()
        m.baseColor = .init(tint: PlatformColor(hex: hex))
        m.roughness = .init(floatLiteral: roughness)
        m.metallic = .init(floatLiteral: 0)
        return m
    }

    static func textured(_ tex: TextureResource, tint: UInt32 = 0xFFFFFF, roughness: Float = 0.8,
                         metallic: Float = 0) -> RealityKit.Material {
        var m = PhysicallyBasedMaterial()
        m.baseColor = .init(tint: PlatformColor(hex: tint), texture: .init(tex, sampler: repeatSampler))
        m.roughness = .init(floatLiteral: roughness)
        m.metallic = .init(floatLiteral: metallic)
        return m
    }

    static func metal(_ hex: UInt32, roughness: Float) -> RealityKit.Material {
        var m = PhysicallyBasedMaterial()
        m.baseColor = .init(tint: PlatformColor(hex: hex))
        m.roughness = .init(floatLiteral: roughness)
        m.metallic = .init(floatLiteral: 0.9)
        return m
    }

    static func glass(_ hex: UInt32 = 0xDCEDEC, opacity: Float = 0.18) -> RealityKit.Material {
        var m = PhysicallyBasedMaterial()
        m.baseColor = .init(tint: PlatformColor(hex: hex))
        m.roughness = .init(floatLiteral: 0.05)
        m.metallic = .init(floatLiteral: 0)
        m.blending = .transparent(opacity: .init(floatLiteral: opacity))
        m.faceCulling = .none
        return m
    }

    /// Self-lit surface (paintings, skylights, the velarium): shows its colours as if well lit.
    static func glow(_ tex: TextureResource, tint: UInt32 = 0xFFFFFF, repeating: Bool = false) -> RealityKit.Material {
        var m = UnlitMaterial()
        m.color = .init(tint: PlatformColor(hex: tint),
                        texture: repeating ? .init(tex, sampler: repeatSampler) : .init(tex))
        return m
    }

    static func glow(_ hex: UInt32) -> RealityKit.Material {
        UnlitMaterial(color: PlatformColor(hex: hex))
    }
}
