import CoreGraphics
import Foundation
import RealityKit

/// Names for the textures drawn in code, so the USD exporter can name the materials that use
/// them ("brick", "travertine_honed", "letter_plate_A") and write the images out. It records
/// nothing unless `enabled` is set, which only the exporter does; the app never pays for it.
enum TextureNames {
    nonisolated(unsafe) static var enabled = false
    private static let lock = NSLock()
    // Strong references, so an identifier is never reused while the export runs.
    nonisolated(unsafe) private static var images: [ObjectIdentifier: (image: CGImage, name: String)] = [:]
    nonisolated(unsafe) private static var textures: [ObjectIdentifier: (texture: TextureResource, name: String, image: CGImage)] = [:]

    /// Remembers the name of a drawn image: the drawing function's `#function` (e.g. "brick()"),
    /// turned into snake case, or an explicit name ("letter_plate_A"), used as it is.
    static func note(_ image: CGImage, function: String) {
        guard enabled else { return }
        note(image, name: function.contains("(") ? snakeCase(String(function.prefix { $0 != "(" })) : function)
    }

    static func note(_ image: CGImage, name: String) {
        guard enabled else { return }
        lock.lock(); defer { lock.unlock() }
        images[ObjectIdentifier(image)] = (image, name)
    }

    /// Links a texture to the image it was made from.
    static func note(_ texture: TextureResource, from image: CGImage) {
        guard enabled else { return }
        lock.lock(); defer { lock.unlock() }
        guard let named = images[ObjectIdentifier(image)] else { return }
        textures[ObjectIdentifier(texture)] = (texture, named.name, image)
    }

    static func lookup(_ texture: TextureResource) -> (name: String, image: CGImage)? {
        lock.lock(); defer { lock.unlock() }
        return textures[ObjectIdentifier(texture)].map { ($0.name, $0.image) }
    }

    static func snakeCase(_ s: String) -> String {
        var out = ""
        for ch in s {
            if ch.isUppercase, !out.isEmpty { out += "_" }
            out += ch.lowercased()
        }
        return out
    }
}
