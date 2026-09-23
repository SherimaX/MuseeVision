import CoreGraphics
import CoreText
import ImageIO
import Foundation
import RealityKit
import simd

/// Textures drawn in code with Core Graphics: the Rotunda's sun clock, stone slabs,
/// coffers, the light-grid skylight, the velarium and the sky.
enum Textures {
    // MARK: Drawing helpers

    static func draw(width: Int, height: Int, _ body: (CGContext) -> Void) -> CGImage {
        let cs = CGColorSpace(name: CGColorSpace.sRGB)!
        let ctx = CGContext(data: nil, width: width, height: height, bitsPerComponent: 8, bytesPerRow: 0,
                            space: cs, bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue)!
        ctx.setAllowsAntialiasing(true)
        ctx.setShouldAntialias(true)
        ctx.interpolationQuality = .high
        body(ctx)
        return ctx.makeImage()!
    }

    static func cg(_ hex: UInt32, _ alpha: CGFloat = 1) -> CGColor {
        CGColor(srgbRed: CGFloat((hex >> 16) & 0xFF) / 255, green: CGFloat((hex >> 8) & 0xFF) / 255,
                blue: CGFloat(hex & 0xFF) / 255, alpha: alpha)
    }

    @MainActor
    static func resource(_ image: CGImage, color: Bool = true) -> TextureResource {
        try! TextureResource(image: image, withName: nil,
                             options: .init(semantic: color ? .color : .raw, mipmapsMode: .allocateAndGenerateAll))
    }

    /// Draws a string centred at the origin of the current transform (y up in the context).
    static func text(_ s: String, in ctx: CGContext, font: String, size: CGFloat, color: CGColor, tracking: CGFloat = 0) {
        let f = CTFontCreateWithName(font as CFString, size, nil)
        let attrs: [NSAttributedString.Key: Any] = [
            NSAttributedString.Key(kCTFontAttributeName as String): f,
            NSAttributedString.Key(kCTForegroundColorAttributeName as String): color,
            NSAttributedString.Key(kCTKernAttributeName as String): tracking,
        ]
        let line = CTLineCreateWithAttributedString(NSAttributedString(string: s, attributes: attrs))
        let b = CTLineGetBoundsWithOptions(line, .useOpticalBounds)
        ctx.textPosition = CGPoint(x: -b.width / 2 - b.minX, y: -b.height / 2 - b.minY)
        CTLineDraw(line, ctx)
    }

    // MARK: The Rotunda floor: a Ø 14 m bronze sun clock in a Ø 20 m travertine floor

    /// Texture for the whole Rotunda floor (20 × 20 m). Board: Rotunda plan, 15 px/m.
    /// Numerals VI–XII–VI across the north half, gilt meridian to XII, motto on the south arc,
    /// gilt sun rosette at the centre.
    static func sunClockFloor(size px: Int = 2048) -> CGImage {
        draw(width: px, height: px) { ctx in
            let s = CGFloat(px) / 20.0   // pixels per metre
            let c = CGFloat(px) / 2
            // Image y runs up; plan north (-z) must be at the top of the texture as mapped
            // on the floor (v = 0 at z = +10, v = 1 at z = -10), so plan north = +y here.
            ctx.setFillColor(cg(0xE9E1D1))
            ctx.fill(CGRect(x: 0, y: 0, width: px, height: px))
            ctx.translateBy(x: c, y: c)
            // Travertine rings and radial joints outside the clock.
            ctx.setStrokeColor(cg(0xD8CDB9))
            ctx.setLineWidth(0.02 * s)
            for r in stride(from: 8.9, through: 10.0, by: 1.1) {
                ctx.strokeEllipse(in: CGRect(x: -r * s, y: -r * s, width: 2 * r * s, height: 2 * r * s))
            }
            for i in 0..<32 {
                let a = CGFloat(i) * .pi / 16
                ctx.move(to: CGPoint(x: cos(a) * 8.9 * s, y: sin(a) * 8.9 * s))
                ctx.addLine(to: CGPoint(x: cos(a) * 10.2 * s, y: sin(a) * 10.2 * s))
            }
            ctx.strokePath()
            // The clock face: pale polished stone disc Ø 14 m with bronze rings.
            ctx.setFillColor(cg(0xF3ECDD))
            ctx.fillEllipse(in: CGRect(x: -7 * s, y: -7 * s, width: 14 * s, height: 14 * s))
            // Subtle concentric slab joints inside the clock.
            ctx.setStrokeColor(cg(0xE3D8C4))
            ctx.setLineWidth(0.015 * s)
            for r in stride(from: 1.6, through: 6.2, by: 1.15) {
                ctx.strokeEllipse(in: CGRect(x: -r * s, y: -r * s, width: 2 * r * s, height: 2 * r * s))
            }
            let bronze = cg(0x7E5C25)
            let gilt = cg(0xC9A24E)
            ctx.setStrokeColor(bronze)
            ctx.setLineWidth(0.17 * s)
            ctx.strokeEllipse(in: CGRect(x: -7 * s, y: -7 * s, width: 14 * s, height: 14 * s))
            ctx.setLineWidth(0.05 * s)
            let r2: CGFloat = 98.2 / 15
            ctx.strokeEllipse(in: CGRect(x: -r2 * s, y: -r2 * s, width: 2 * r2 * s, height: 2 * r2 * s))
            // Hour lines and numerals VI (west) … XII (north) … VI (east).
            let numerals = ["VI", "VII", "VIII", "IX", "X", "XI", "XII", "I", "II", "III", "IV", "V", "VI"]
            for (i, n) in numerals.enumerated() {
                // Angle measured in the image (x east, y north): VI west = 180°, XII north = 90°.
                let a = CGFloat.pi - CGFloat(i) * .pi / 12
                ctx.setStrokeColor(bronze)
                ctx.setLineWidth(0.1 * s)
                ctx.move(to: CGPoint(x: cos(a) * r2 * s, y: sin(a) * r2 * s))
                ctx.addLine(to: CGPoint(x: cos(a) * 7.45 * s, y: sin(a) * 7.45 * s))
                ctx.strokePath()
                // Half-hour ticks.
                if i < 12 {
                    let h = a - .pi / 24
                    ctx.setLineWidth(0.04 * s)
                    ctx.move(to: CGPoint(x: cos(h) * 6.75 * s, y: sin(h) * 6.75 * s))
                    ctx.addLine(to: CGPoint(x: cos(h) * 7.0 * s, y: sin(h) * 7.0 * s))
                    ctx.strokePath()
                }
                ctx.saveGState()
                ctx.translateBy(x: cos(a) * 8.0 * s, y: sin(a) * 8.0 * s)
                ctx.rotate(by: a - .pi / 2)
                text(n, in: ctx, font: "Didot", size: 0.62 * s, color: bronze)
                ctx.restoreGState()
            }
            // Motto around the south arc, read from the centre (letters upright towards the centre).
            let motto = Array("HORAS · NON · NUMERO · NISI · SERENAS")
            let span: CGFloat = 136 * .pi / 180
            for (i, ch) in motto.enumerated() where ch != " " {
                let f = CGFloat(i) / CGFloat(motto.count - 1)
                let a = -.pi / 2 - span / 2 + span * (1 - f)   // west → east along the south
                ctx.saveGState()
                ctx.translateBy(x: cos(a) * 8.2 * s, y: sin(a) * 8.2 * s)
                ctx.rotate(by: a + .pi / 2)
                text(String(ch), in: ctx, font: "Didot", size: 0.4 * s, color: bronze)
                ctx.restoreGState()
            }
            // Gilt meridian from the sun to XII.
            ctx.setStrokeColor(gilt)
            ctx.setLineWidth(0.23 * s)
            ctx.move(to: .zero)
            ctx.addLine(to: CGPoint(x: 0, y: 7.0 * s))
            ctx.strokePath()
            // The gilt sun rosette where every visit begins: 16 rays, r ≈ 0.95 m.
            let rays = 16
            let path = CGMutablePath()
            for i in 0..<(rays * 2) {
                let a = CGFloat(i) * .pi / CGFloat(rays) + .pi / 2
                let r: CGFloat = (i % 2 == 0 ? 0.95 : 0.42) * s
                let p = CGPoint(x: cos(a) * r, y: sin(a) * r)
                if i == 0 { path.move(to: p) } else { path.addLine(to: p) }
            }
            path.closeSubpath()
            ctx.addPath(path)
            ctx.setFillColor(gilt)
            ctx.fillPath()
            ctx.setFillColor(cg(0xE4C77A))
            ctx.fillEllipse(in: CGRect(x: -0.3 * s, y: -0.3 * s, width: 0.6 * s, height: 0.6 * s))
            ctx.setFillColor(gilt)
            ctx.fillEllipse(in: CGRect(x: -0.16 * s, y: -0.16 * s, width: 0.32 * s, height: 0.32 * s))
        }
    }

    // MARK: Tiles

    /// One 1.2 m stone slab with a joint on two edges (repeats seamlessly).
    static func stoneSlab(base: UInt32 = 0xF4EFE5, joint: UInt32 = 0xDCD3C3) -> CGImage {
        draw(width: 256, height: 256) { ctx in
            ctx.setFillColor(cg(base))
            ctx.fill(CGRect(x: 0, y: 0, width: 256, height: 256))
            // A little veining so the floor reads as stone, not paper.
            var rng = SplitMix(seed: 7)
            for _ in 0..<60 {
                ctx.setFillColor(cg(joint, CGFloat(rng.next(0.03, 0.08))))
                let x = rng.next(0, 256), y = rng.next(0, 256), w = rng.next(8, 70), h = rng.next(2, 8)
                ctx.fillEllipse(in: CGRect(x: x, y: y, width: w, height: h))
            }
            ctx.setFillColor(cg(joint))
            ctx.fill(CGRect(x: 0, y: 0, width: 256, height: 2))
            ctx.fill(CGRect(x: 0, y: 0, width: 2, height: 256))
        }
    }

    /// One coffer: a recessed square panel with shaded bevels (for dome and vault).
    static func coffer(base: UInt32 = 0xEFE8DB) -> CGImage {
        draw(width: 256, height: 256) { ctx in
            ctx.setFillColor(cg(base))
            ctx.fill(CGRect(x: 0, y: 0, width: 256, height: 256))
            let rib: CGFloat = 26, bevel: CGFloat = 30
            // Bevels: lit from below (you look up at them), top darker.
            let outer = CGRect(x: rib, y: rib, width: 256 - 2 * rib, height: 256 - 2 * rib)
            let inner = outer.insetBy(dx: bevel, dy: bevel)
            func trap(_ pts: [CGPoint], _ color: CGColor) {
                ctx.setFillColor(color)
                ctx.addLines(between: pts)
                ctx.closePath()
                ctx.fillPath()
            }
            trap([CGPoint(x: outer.minX, y: outer.minY), CGPoint(x: outer.maxX, y: outer.minY),
                  CGPoint(x: inner.maxX, y: inner.minY), CGPoint(x: inner.minX, y: inner.minY)], cg(0xD9CFBD))
            trap([CGPoint(x: outer.minX, y: outer.maxY), CGPoint(x: outer.maxX, y: outer.maxY),
                  CGPoint(x: inner.maxX, y: inner.maxY), CGPoint(x: inner.minX, y: inner.maxY)], cg(0xF7F2E8))
            trap([CGPoint(x: outer.minX, y: outer.minY), CGPoint(x: outer.minX, y: outer.maxY),
                  CGPoint(x: inner.minX, y: inner.maxY), CGPoint(x: inner.minX, y: inner.minY)], cg(0xE3DACA))
            trap([CGPoint(x: outer.maxX, y: outer.minY), CGPoint(x: outer.maxX, y: outer.maxY),
                  CGPoint(x: inner.maxX, y: inner.maxY), CGPoint(x: inner.maxX, y: inner.minY)], cg(0xE8E0D1))
            ctx.setFillColor(cg(0xE6DDCC))
            ctx.fill(inner)
            // A small gilt rosette in each coffer.
            ctx.setFillColor(cg(0xC9A266, 0.8))
            ctx.fillEllipse(in: CGRect(x: 128 - 9, y: 128 - 9, width: 18, height: 18))
        }
    }

    /// The light-grid skylight: glowing panes behind a fine grid.
    static func lightGrid() -> CGImage {
        draw(width: 256, height: 256) { ctx in
            ctx.setFillColor(cg(0xFFF9EC))
            ctx.fill(CGRect(x: 0, y: 0, width: 256, height: 256))
            ctx.setFillColor(cg(0xB9B1A4))
            ctx.fill(CGRect(x: 0, y: 0, width: 256, height: 6))
            ctx.fill(CGRect(x: 0, y: 0, width: 6, height: 256))
        }
    }

    /// Fabric velarium with soft clouds (the oval's ceiling).
    static func velarium() -> CGImage {
        draw(width: 1024, height: 1024) { ctx in
            ctx.setFillColor(cg(0xFFF6E2))
            ctx.fill(CGRect(x: 0, y: 0, width: 1024, height: 1024))
            var rng = SplitMix(seed: 42)
            for _ in 0..<70 {
                let x = rng.next(0, 1024), y = rng.next(0, 1024), r = rng.next(40, 160)
                ctx.setFillColor(cg(0xFFFFFF, CGFloat(rng.next(0.10, 0.28))))
                ctx.fillEllipse(in: CGRect(x: x - r, y: y - r * 0.6, width: 2 * r, height: 1.2 * r))
            }
            ctx.setFillColor(cg(0xE8DCC4, 0.35))
            for i in stride(from: 0, to: 1024, by: 64) {
                ctx.fill(CGRect(x: i, y: 0, width: 2, height: 1024))
            }
        }
    }

    /// Equirectangular sky used as the environment (lighting and what you see through
    /// the oculus and skylights): pale sky above, warm stone below.
    static func skyEquirect() -> CGImage {
        draw(width: 1024, height: 512) { ctx in
            let cs = CGColorSpace(name: CGColorSpace.sRGB)!
            let colors = [cg(0xAFC4D6), cg(0xE2E4E0), cg(0xF6EFE2), cg(0xDCCFB8), cg(0x8F8270)] as CFArray
            let grad = CGGradient(colorsSpace: cs, colors: colors, locations: [0, 0.3, 0.5, 0.56, 1])!
            // Image y = 0 at the bottom in CG; the top row of the equirect is the zenith.
            ctx.drawLinearGradient(grad, start: CGPoint(x: 0, y: 512), end: CGPoint(x: 0, y: 0), options: [])
            var rng = SplitMix(seed: 3)
            for _ in 0..<50 {
                let x = rng.next(0, 1024), y = rng.next(330, 500), r = rng.next(20, 70)
                ctx.setFillColor(cg(0xFFFFFF, CGFloat(rng.next(0.15, 0.4))))
                ctx.fillEllipse(in: CGRect(x: x - r, y: y - r * 0.35, width: 2 * r, height: 0.7 * r))
            }
        }
    }

    /// Bronze door with a small engraved name plate.
    static func bronzeDoor(label: String) -> CGImage {
        draw(width: 512, height: 1024) { ctx in
            let cs = CGColorSpace(name: CGColorSpace.sRGB)!
            let grad = CGGradient(colorsSpace: cs, colors: [cg(0x4A3620), cg(0x6B4E2E), cg(0x4A3620)] as CFArray,
                                  locations: [0, 0.5, 1])!
            ctx.drawLinearGradient(grad, start: .zero, end: CGPoint(x: 512, y: 0), options: [])
            // Panels.
            ctx.setStrokeColor(cg(0x2E2114))
            ctx.setLineWidth(6)
            for col in 0..<2 {
                for row in 0..<3 {
                    let x = CGFloat(col) * 256 + 36, y = CGFloat(row) * 230 + 60
                    ctx.stroke(CGRect(x: x, y: y, width: 184, height: 190))
                }
            }
            ctx.setFillColor(cg(0x241910))
            ctx.fill(CGRect(x: 253, y: 0, width: 6, height: 1024))
            // Name plate.
            ctx.setFillColor(cg(0xC9A266))
            ctx.fill(CGRect(x: 106, y: 760, width: 300, height: 70))
            ctx.saveGState()
            ctx.translateBy(x: 256, y: 795)
            text(label, in: ctx, font: "Didot", size: 30, color: cg(0x2E2114), tracking: 3)
            ctx.restoreGState()
        }
    }

    // MARK: Sky

    /// A soft round star sprite (white, alpha falloff).
    static func starSprite() -> CGImage {
        draw(width: 64, height: 64) { ctx in
            let cs = CGColorSpace(name: CGColorSpace.sRGB)!
            let g = CGGradient(colorsSpace: cs, colors: [cg(0xFFFFFF, 1), cg(0xFFFFFF, 0.55), cg(0xFFFFFF, 0)] as CFArray,
                               locations: [0, 0.25, 1])!
            ctx.drawRadialGradient(g, startCenter: CGPoint(x: 32, y: 32), startRadius: 0,
                                   endCenter: CGPoint(x: 32, y: 32), endRadius: 32, options: [])
        }
    }

    /// The moon's disc with its phase (lit fraction 0…1; waxing = lit on the right as seen
    /// from the northern hemisphere).
    static func moonDisc(illuminated k: Float, waxing: Bool) -> CGImage {
        draw(width: 256, height: 256) { ctx in
            let r: CGFloat = 110
            ctx.translateBy(x: 128, y: 128)
            let cs = CGColorSpace(name: CGColorSpace.sRGB)!
            let halo = CGGradient(colorsSpace: cs, colors: [cg(0xFFF8E6, 0.35), cg(0xFFF8E6, 0)] as CFArray, locations: [0, 1])!
            ctx.drawRadialGradient(halo, startCenter: .zero, startRadius: r * 0.9, endCenter: .zero, endRadius: 128, options: [])
            // Earthshine on the dark side.
            ctx.setFillColor(cg(0x3A3F4A, 0.9))
            ctx.fillEllipse(in: CGRect(x: -r, y: -r, width: 2 * r, height: 2 * r))
            // Lit part: a half disc plus/minus an ellipse whose width depends on the phase.
            let lit = CGMutablePath()
            let side: CGFloat = waxing ? 1 : -1
            lit.addArc(center: .zero, radius: r, startAngle: -.pi / 2, endAngle: .pi / 2, clockwise: side < 0)
            let w = r * CGFloat(1 - 2 * k)   // terminator half-width: +r new, −r full
            let steps = 40
            for i in 0...steps {
                let a = CGFloat.pi / 2 - CGFloat.pi * CGFloat(i) / CGFloat(steps)
                lit.addLine(to: CGPoint(x: side * w * cos(a), y: r * sin(a)))
            }
            lit.closeSubpath()
            ctx.addPath(lit)
            ctx.setFillColor(cg(0xF4F0E4))
            ctx.fillPath()
            // A few maria.
            ctx.setFillColor(cg(0x9A9A92, 0.35))
            for (x, y, rr) in [(-30.0, 30.0, 26.0), (20.0, 40.0, 18.0), (10.0, -10.0, 30.0), (-40.0, -30.0, 16.0)] {
                ctx.fillEllipse(in: CGRect(x: x - rr, y: y - rr, width: 2 * rr, height: 2 * rr))
            }
        }
    }

    /// Sky dome gradient for a given sun altitude (degrees): day, dusk, night.
    static func skyDome(sunAltitude alt: Float) -> CGImage {
        let (zenith, mid, horizon): (UInt32, UInt32, UInt32)
        switch alt {
        case 8...: (zenith, mid, horizon) = (0x7FA6CC, 0xB9D0E2, 0xEEF0EA)
        case -4..<8: (zenith, mid, horizon) = (0x3D5478, 0xA38BA0, 0xF2C49A)
        case -12 ..< -4: (zenith, mid, horizon) = (0x141D33, 0x2B3553, 0x6A5D6E)
        default: (zenith, mid, horizon) = (0x05070D, 0x0A0F1C, 0x161B28)
        }
        return draw(width: 64, height: 512) { ctx in
            let cs = CGColorSpace(name: CGColorSpace.sRGB)!
            let g = CGGradient(colorsSpace: cs, colors: [cg(zenith), cg(mid), cg(horizon), cg(0x3A3428)] as CFArray,
                               locations: [0, 0.3, 0.5, 0.56])!
            ctx.drawLinearGradient(g, start: CGPoint(x: 0, y: 512), end: CGPoint(x: 0, y: 0), options: [])
        }
    }
}


/// Tiny deterministic random generator for texture details.
struct SplitMix {
    var state: UInt64
    init(seed: UInt64) { state = seed }
    mutating func nextUInt() -> UInt64 {
        state &+= 0x9E3779B97F4A7C15
        var z = state
        z = (z ^ (z >> 30)) &* 0xBF58476D1CE4E5B9
        z = (z ^ (z >> 27)) &* 0x94D049BB133111EB
        return z ^ (z >> 31)
    }
    mutating func next(_ lo: Double, _ hi: Double) -> CGFloat {
        CGFloat(lo + (hi - lo) * Double(nextUInt() % 1_000_000) / 1_000_000)
    }
}

/// Reads an image file's pixel size from its header (cheap) to get its proportions.
enum ImageInfo {
    static func aspect(_ name: String) -> Float? {
        guard let url = Bundle.main.url(forResource: name, withExtension: "jpg", subdirectory: "paintings")
                ?? Bundle.main.url(forResource: name, withExtension: "jpg"),
              let src = CGImageSourceCreateWithURL(url as CFURL, nil),
              let props = CGImageSourceCopyPropertiesAtIndex(src, 0, nil) as? [CFString: Any],
              let w = props[kCGImagePropertyPixelWidth] as? Int, let h = props[kCGImagePropertyPixelHeight] as? Int, h > 0
        else { return nil }
        return Float(w) / Float(h)
    }
}

extension Textures {
    /// Warm cellar brick, laid in stretcher bond (one tile = 0.9 × 0.6 m).
    static func brick() -> CGImage {
        draw(width: 256, height: 256) { ctx in
            ctx.setFillColor(cg(0xC9B8A4))
            ctx.fill(CGRect(x: 0, y: 0, width: 256, height: 256))
            var rng = SplitMix(seed: 5)
            let rows = 8, cols = 4
            let bh = 256.0 / Double(rows), bw = 256.0 / Double(cols)
            for r in 0..<rows {
                for c in 0...cols {
                    let off = r % 2 == 0 ? 0 : bw / 2
                    let x = Double(c) * bw - off, y = Double(r) * bh
                    let shades: [UInt32] = [0xB98E73, 0xA97E63, 0xC49A7C, 0xB08468, 0x9F7659]
                    ctx.setFillColor(cg(shades[Int(rng.nextUInt() % 5)]))
                    ctx.fill(CGRect(x: x + 2, y: y + 2, width: bw - 4, height: bh - 4))
                }
            }
        }
    }

    /// Brass mesh for the Reserve's racks: dark ground, fine wires.
    static func brassMesh() -> CGImage {
        draw(width: 128, height: 128) { ctx in
            ctx.setFillColor(cg(0x5E5246))
            ctx.fill(CGRect(x: 0, y: 0, width: 128, height: 128))
            ctx.setFillColor(cg(0x9A8660))
            for i in stride(from: 0, to: 128, by: 16) {
                ctx.fill(CGRect(x: i, y: 0, width: 2, height: 128))
                ctx.fill(CGRect(x: 0, y: i, width: 128, height: 2))
            }
        }
    }
}
