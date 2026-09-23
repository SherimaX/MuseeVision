import Foundation
import simd

/// A tiny writer for USD's text format (.usda). Layers are built as trees of prim specs and
/// written out in one go; export.sh then turns the big ones into binary .usdc with usdcat.
enum USDA {
    /// A valid USD identifier from any name: accents folded ("Élan" → "Elan"), spaces and
    /// punctuation to "_", other characters (e.g. Chinese) spelled as u+hex, never a leading digit.
    static func identifier(_ s: String) -> String {
        let folded = s.folding(options: .diacriticInsensitive, locale: Locale(identifier: "en_US_POSIX"))
        var out = ""
        for u in folded.unicodeScalars {
            switch u.value {
            case 0x30...0x39, 0x41...0x5A, 0x61...0x7A: out.unicodeScalars.append(u)
            case 0...0x7F: out += "_"
            default: out += "u" + String(u.value, radix: 16, uppercase: true)
            }
        }
        while out.contains("__") { out = out.replacingOccurrences(of: "__", with: "_") }
        out = out.trimmingCharacters(in: CharacterSet(charactersIn: "_"))
        if out.isEmpty { out = "prim" }
        if let f = out.unicodeScalars.first, (0x30...0x39).contains(f.value) { out = "_" + out }
        return out
    }

    static func f(_ x: Float) -> String {
        if x == 0 { return "0" }   // also folds −0
        if x.isNaN || x.isInfinite { return "0" }
        let s = x.description
        return s.hasSuffix(".0") ? String(s.dropLast(2)) : s
    }

    static func f(_ x: Double) -> String { f(Float(x)) }

    static func v2(_ v: SIMD2<Float>) -> String { "(\(f(v.x)), \(f(v.y)))" }
    static func v3(_ v: SIMD3<Float>) -> String { "(\(f(v.x)), \(f(v.y)), \(f(v.z)))" }

    static func string(_ s: String) -> String {
        "\"" + s.replacingOccurrences(of: "\\", with: "\\\\").replacingOccurrences(of: "\"", with: "\\\"")
            .replacingOccurrences(of: "\n", with: "\\n") + "\""
    }

    /// Joins array items, breaking lines every `perLine` items to keep the text readable.
    static func array<T>(_ items: [T], perLine: Int = 12, indent: String, _ format: (T) -> String) -> String {
        guard !items.isEmpty else { return "[]" }
        var out = "["
        for (i, item) in items.enumerated() {
            if i > 0 { out += ", " }
            if i > 0 && i % perLine == 0 { out += "\n" + indent + "    " }
            out += format(item)
        }
        return out + "]"
    }
}

/// One prim in one layer: a `def` (the layer defines it) or an `over` (it only holds a descendant).
final class PrimSpec {
    let name: String
    var specifier = "over"
    var typeName = ""
    var metadata: [String] = []
    var properties: [String] = []
    private(set) var children: [PrimSpec] = []
    private var index: [String: PrimSpec] = [:]

    init(_ name: String) { self.name = name }

    func child(_ name: String) -> PrimSpec {
        if let c = index[name] { return c }
        let c = PrimSpec(name)
        children.append(c)
        index[name] = c
        return c
    }

    func write(to out: inout String, indent: String) {
        out += indent + specifier + (typeName.isEmpty ? "" : " " + typeName) + " " + USDA.string(name)
        if !metadata.isEmpty {
            out += " (\n"
            for m in metadata { out += indent + "    " + m.replacingOccurrences(of: "\n", with: "\n" + indent + "    ") + "\n" }
            out += indent + ")"
        }
        out += "\n" + indent + "{\n"
        for p in properties { out += indent + "    " + p.replacingOccurrences(of: "\n", with: "\n" + indent + "    ") + "\n" }
        for (i, c) in children.enumerated() {
            if i > 0 || !properties.isEmpty { out += "\n" }
            c.write(to: &out, indent: indent + "    ")
        }
        out += indent + "}\n"
    }

    /// Number of prims this spec and its descendants define.
    var definedCount: Int { (specifier == "def" ? 1 : 0) + children.reduce(0) { $0 + $1.definedCount } }
}

/// A layer file: stage metadata plus a tree of prim specs.
final class LayerFile {
    /// Path relative to the usd/ folder, with the extension it ends up with (.usda or .usdc).
    let path: String
    var doc: String
    var subLayers: [String] = []
    var defaultPrim = "Museum"
    var extraMetadata: [String] = []
    let root = PrimSpec("")

    init(path: String, doc: String) { self.path = path; self.doc = doc }

    /// The spec at a prim path (names below the pseudo-root), made as `over`s where missing.
    func spec(_ path: [String]) -> PrimSpec {
        var s = root
        for name in path { s = s.child(name) }
        return s
    }

    var definedPrims: Int { root.definedCount }

    func text() -> String {
        var out = "#usda 1.0\n(\n"
        out += "    defaultPrim = \(USDA.string(defaultPrim))\n"
        out += "    doc = \(USDA.string(doc))\n"
        out += "    metersPerUnit = 1\n"
        for m in extraMetadata { out += "    " + m + "\n" }
        if !subLayers.isEmpty {
            out += "    subLayers = [\n"
            out += subLayers.map { "        @\($0)@" }.joined(separator: ",\n") + "\n"
            out += "    ]\n"
        }
        out += "    upAxis = \"Y\"\n)\n\n"
        for (i, c) in root.children.enumerated() {
            if i > 0 { out += "\n" }
            c.write(to: &out, indent: "")
        }
        return out
    }
}
