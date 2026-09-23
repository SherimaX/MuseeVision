import Foundation
import RealityKit
import simd

/// museum-usd: builds the whole museum exactly as the app does (Shared/), then writes it as USD.
///
///   museum-usd --repo <repository> --out <usd folder> [--stage <scratch folder>] [--date 2026-06-21T12:00:00Z]
///
/// Run it through tools/usd-export/export.sh, which compiles it and validates the result.
@main
struct MuseumUSD {
    @MainActor
    static func main() {
        do {
            try run()
        } catch {
            FileHandle.standardError.write(Data("museum-usd: \(error)\n".utf8))
            exit(1)
        }
    }

    @MainActor
    static func run() throws {
        var args = Array(CommandLine.arguments.dropFirst())
        func value(_ flag: String) -> String? {
            guard let i = args.firstIndex(of: flag), i + 1 < args.count else { return nil }
            let v = args[i + 1]
            args.removeSubrange(i...(i + 1))
            return v
        }
        guard let repoPath = value("--repo") else { throw ExportError("usage: museum-usd --repo <repo> --out <usd dir> [--stage <dir>] [--date ISO8601]") }
        let repo = URL(fileURLWithPath: repoPath).standardizedFileURL
        let out = URL(fileURLWithPath: value("--out") ?? repo.appendingPathComponent("usd").path).standardizedFileURL
        let stage = URL(fileURLWithPath: value("--stage") ?? NSTemporaryDirectory() + "museum-usd-stage").standardizedFileURL
        let dateText = value("--date") ?? "2026-06-21T12:00:00Z"
        guard let date = ISO8601DateFormatter().date(from: dateText) else { throw ExportError("bad --date \(dateText)") }

        // A fixed clock, place and time zone: the gardens follow the season, the Chinese Wing the
        // solar term, the Rotunda's sun the hour. Same inputs, same USD.
        setenv("TZ", "UTC", 1)
        NSTimeZone.default = TimeZone(identifier: "UTC")!
        MuseumScene.fixedDate = date
        MuseumResources.folders = ["assets/paintings", "assets/sculptures", "assets/sky", "data", "assets"]
            .map { repo.appendingPathComponent($0) }
        TextureNames.enabled = true

        let t0 = Date()
        let scene = MuseumScene()
        log("built the museum in \(String(format: "%.1f", Date().timeIntervalSince(t0))) s")

        let exporter = Exporter(scene: scene, repo: repo)
        exporter.run()

        // Stage everything as .usda, then convert the layers meant to be binary with usdcat.
        let fm = FileManager.default
        try? fm.removeItem(at: stage)
        try fm.createDirectory(at: stage, withIntermediateDirectories: true)

        // Materials and textures.
        let materialsLayer = LayerFile(path: "Materials.usda",
                                       doc: "Musée Vision: one UsdPreviewSurface material per distinct material of the Swift build. Names are stable across re-exports.")
        let textures = try exporter.materials.write(into: materialsLayer, textureDir: stage.appendingPathComponent("textures"),
                                                    used: exporter.usedMaterials)

        // The root layer: the wings, the sculptures and the materials. Extras stay out.
        let wingKeys = exporter.layerOrder.filter { !["Sculptures", "Helpers", "Sky"].contains($0) }
        let root = LayerFile(path: "museum.usda",
                             doc: "Musée Vision, the whole museum, exported from the Swift builder (tools/usd-export). Metres, Y up, the Rotunda's centre at the origin, x east, z south (plan y). See usd/README.md.")
        root.extraMetadata = ["customLayerData = {\n        string museevision_clock = \(USDA.string(dateText))\n        string museevision_frame = \"metres; +X east, +Y up, +Z south (plan y); origin at the centre of the Rotunda\"\n    }"]
        root.subLayers = (exporter.layers["Sculptures"] != nil ? ["./sculptures/Sculptures.usda"] : [])
            + wingKeys.map { "./" + exporter.layers[$0]!.path } + ["./Materials.usda"]
        let museum = root.spec(["Museum"])
        museum.specifier = "def"
        museum.typeName = "Xform"
        museum.metadata = ["kind = \"assembly\""]
        let childOrder = wingKeys + ["Materials"]
        museum.properties = ["reorder nameChildren = [\(childOrder.map { USDA.string($0) }.joined(separator: ", "))]"]

        var all: [LayerFile] = [root, materialsLayer] + exporter.layerOrder.compactMap { exporter.layers[$0] } + exporter.scanFiles
        all.sort { $0.path < $1.path }
        for layer in all {
            let staged = stage.appendingPathComponent(layer.path.replacingOccurrences(of: ".usdc", with: ".usda"))
            try fm.createDirectory(at: staged.deletingLastPathComponent(), withIntermediateDirectories: true)
            try layer.text().write(to: staged, atomically: true, encoding: .utf8)
        }

        // Replace the generated parts of the output folder (README.md and anything else stays).
        try fm.createDirectory(at: out, withIntermediateDirectories: true)
        for generated in ["museum.usda", "Materials.usda", "summary.json", "wings", "extras", "sculptures", "textures"] {
            try? fm.removeItem(at: out.appendingPathComponent(generated))
        }
        let usdcat = try tool("usdcat")
        for layer in all {
            let dest = out.appendingPathComponent(layer.path)
            try fm.createDirectory(at: dest.deletingLastPathComponent(), withIntermediateDirectories: true)
            let staged = stage.appendingPathComponent(layer.path.replacingOccurrences(of: ".usdc", with: ".usda"))
            if layer.path.hasSuffix(".usdc") {
                try runTool(usdcat, [staged.path, "-o", dest.path])
            } else {
                try fm.copyItem(at: staged, to: dest)
            }
        }
        if !textures.isEmpty {
            try fm.createDirectory(at: out.appendingPathComponent("textures"), withIntermediateDirectories: true)
            for t in textures {
                try fm.copyItem(at: stage.appendingPathComponent("textures/" + t), to: out.appendingPathComponent("textures/" + t))
            }
        }

        // Summary: per wing counts and bounds (metres, RealityKit frame).
        var wings: [[String: Any]] = []
        var lines: [String] = []
        for name in exporter.wingOrder {
            guard let s = exporter.stats[name] else { continue }
            let key = name == "Sky" ? "Sky" : USDA.identifier(name)
            let layer = exporter.layers[key]
            let prims = layer?.definedPrims ?? 0
            func r(_ v: SIMD3<Float>) -> [Double] { [v.x, v.y, v.z].map { (Double($0) * 100).rounded() / 100 } }
            var entry: [String: Any] = ["wing": name, "layer": layer?.path ?? "", "primsInLayer": prims, "meshes": s.meshes,
                                        "triangles": s.triangles, "materials": s.materials.count, "lights": s.lights]
            if s.lo.x.isFinite { entry["boundsMin"] = r(s.lo); entry["boundsMax"] = r(s.hi) }
            wings.append(entry)
            lines.append(String(format: "%-14@ %7d tris %5d meshes %4d prims %4d mats %3d lights  x %7.1f…%7.1f  y %6.1f…%6.1f  z %7.1f…%7.1f",
                                name as NSString, s.triangles, s.meshes, prims, s.materials.count, s.lights,
                                s.lo.x, s.hi.x, s.lo.y, s.hi.y, s.lo.z, s.hi.z))
        }
        if let sculptures = exporter.layers["Sculptures"] {
            lines.append("Sculptures layer: \(sculptures.definedPrims) prims, \(exporter.scanFiles.count) scan files")
        }
        let summary: [String: Any] = [
            "generator": "tools/usd-export (museum-usd)",
            "clock": dateText,
            "frame": "metres, +X east, +Y up, +Z south (plan y), origin at the Rotunda's centre",
            "boundsNote": "world-space bounds of each wing's meshes, without the 800 m grass plane (HallOfLight/Ground)",
            "wings": wings,
            "materials": exporter.materials.names.enumerated().filter { exporter.usedMaterials.contains($0.offset) }.map(\.element).sorted(),
            "textures": textures.sorted(),
            "warnings": exporter.warnings,
        ]
        let json = try JSONSerialization.data(withJSONObject: summary, options: [.prettyPrinted, .sortedKeys])
        try json.write(to: out.appendingPathComponent("summary.json"))
        for l in lines { log(l) }
        log("\(exporter.usedMaterials.count) materials, \(textures.count) textures")
        for w in exporter.warnings { log("warning: \(w)") }
        log("wrote \(out.path)")
    }

    static func log(_ s: String) { print(s) }

    /// Finds a USD command-line tool on PATH (/usr/bin on macOS, or the bin of a usd-core venv).
    static func tool(_ name: String) throws -> String {
        for dir in (ProcessInfo.processInfo.environment["PATH"] ?? "/usr/bin").split(separator: ":") {
            let p = "\(dir)/\(name)"
            if FileManager.default.isExecutableFile(atPath: p) { return p }
        }
        throw ExportError("\(name) not found. It ships with macOS (/usr/bin); elsewhere `pip install usd-core` and put its bin on PATH.")
    }

    static func runTool(_ path: String, _ arguments: [String]) throws {
        let p = Process()
        p.executableURL = URL(fileURLWithPath: path)
        p.arguments = arguments
        try p.run()
        p.waitUntilExit()
        guard p.terminationStatus == 0 else { throw ExportError("\(path) \(arguments.joined(separator: " ")) failed") }
    }
}
