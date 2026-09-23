import Foundation
import RealityKit
import simd

/// Walks the built museum and lays it out as USD layers: one per wing, plus the sculpture scans,
/// the materials, and two optional layers (runtime helpers and the sky).
@MainActor
final class Exporter {
    struct Stats {
        var triangles = 0
        var meshes = 0
        var lights = 0
        var materials = Set<Int>()
        var lo = SIMD3<Float>(repeating: .infinity)
        var hi = SIMD3<Float>(repeating: -.infinity)
        mutating func include(_ p: SIMD3<Float>) { lo = simd_min(lo, p); hi = simd_max(hi, p) }
    }

    let scene: MuseumScene
    let repo: URL
    let materials = MaterialTable()
    private(set) var layers: [String: LayerFile] = [:]
    private(set) var layerOrder: [String] = []
    /// Geometry files of the sculpture scans (usd/sculptures/<id>.usdc).
    private(set) var scanFiles: [LayerFile] = []
    private(set) var stats: [String: Stats] = [:]
    private(set) var wingOrder: [String] = []
    private var used = Set<Int>()
    private(set) var warnings: [String] = []

    private var slots: [ObjectIdentifier: MuseumScene.PaintingSlot] = [:]
    private var roles: [ObjectIdentifier: String] = [:]
    private let scanIDs: Set<String>
    private let credits: [String: [String: String]]
    /// Pending mesh bindings (spec, material index), resolved once names are final.
    private var bindings: [(PrimSpec, Int)] = []

    /// Runtime-only helpers, kept out of the wings (see layer "Helpers").
    static let helperNames: Set<String> = ["Contact shade", "Sun patch", "Point of shadow"]

    init(scene: MuseumScene, repo: URL) {
        self.scene = scene
        self.repo = repo
        let sculptures = repo.appendingPathComponent("assets/sculptures")
        let files = (try? FileManager.default.contentsOfDirectory(atPath: sculptures.path)) ?? []
        scanIDs = Set(files.filter { $0.hasSuffix(".mvm") }.map { String($0.dropLast(4)) })
        credits = Self.readCredits(sculptures.appendingPathComponent("CREDITS.md"))
        for s in scene.paintingSlots { slots[ObjectIdentifier(s.entity)] = s }
        tagMovingParts()
    }

    // MARK: Moving parts

    private func tag(_ e: Entity?, _ role: String) {
        if let e { roles[ObjectIdentifier(e)] = role }
    }

    private func tagMovingParts() {
        if let pond = scene.pondLift {
            tag(pond.pond, "pond")
            pond.posts.forEach { tag($0, "pond_post") }
            pond.rimSegments.forEach { tag($0, "pond_rim_light") }
            tag(pond.halo, "pond_halo")
        }
        scene.reserveRacks.forEach { tag($0.entity, "reserve_rack") }
        scene.reserveWorks.forEach { tag($0.entity, "reserve_rack_work") }
        tag(scene.easelLight, "easel_light")
        if let el = scene.elevator {
            tag(el.root, "elevator")
            tag(el.car, "elevator_car")
            tag(el.mast, "elevator_mast")
            tag(el.glow, "elevator_car_glow")
            el.doorPanels.forEach { tag($0.entity, "elevator_car_door") }
            el.landingDoors.forEach { tag($0.entity, "elevator_landing_door") }
        }
        tag(scene.irisProxy, "elevator_iris")
        tag(scene.chickenCup?.entity, "chicken_cup")
        tag(scene.sunLight, "sun")
        tag(scene.sunPatch, "sun_clock_marker")
        tag(scene.shadowPoint, "sun_clock_marker")
        for name in ["Veil north", "Veil south"] { scene.building.descendants(named: name).forEach { tag($0, "handscroll_veil") } }
        for name in ["Roller north", "Roller south"] { scene.building.descendants(named: name).forEach { tag($0, "handscroll_roller") } }
        for e in scene.building.descendants(named: "Crown") { tag(e, "tree_crown") }
        // The stereographs' two alternating views.
        func walk(_ e: Entity) {
            if let id = e.components[ModelComponent.self]?.mesh.contents.models.first?.id,
               id.hasPrefix("stereo-"), e.parent?.name.hasPrefix("stereo-") == true, id != "reader" {
                tag(e, "stereograph_view")
            }
            e.children.forEach(walk)
        }
        walk(scene.building)
    }

    // MARK: Layers

    func layer(_ key: String) -> LayerFile {
        if let l = layers[key] { return l }
        let path: String, doc: String
        switch key {
        case "Sculptures":
            path = "sculptures/Sculptures.usda"
            doc = "Musée Vision: the sculpture scans in place (geometry in the <id>.usdc files beside this one)."
        case "Helpers":
            path = "extras/Helpers.usdc"
            doc = "Musée Vision: runtime-only helpers of the iPhone build (contact shading, sun-clock markers). Not in museum.usda; add it as a sublayer to see them."
        case "Sky":
            path = "extras/Sky.usdc"
            doc = "Musée Vision: the iPhone build's sky dome and star field (they follow the camera there). Not in museum.usda."
        default:
            path = "wings/\(key).usdc"
            doc = "Musée Vision: \(key). Exported from the Swift builder; do not edit by hand."
        }
        let l = LayerFile(path: path, doc: doc)
        layers[key] = l
        layerOrder.append(key)
        return l
    }

    // MARK: Walk

    func run() {
        var seen = Set<ObjectIdentifier>()
        var parts = scene.parts
        // Anything not claimed by a part (built after init) goes to "Misc", with a warning.
        let claimed = Set(parts.flatMap { $0.entities }.map(ObjectIdentifier.init))
        let loose = (scene.building.children.map { $0 } + scene.root.children.map { $0 })
            .filter { $0 !== scene.building && !claimed.contains(ObjectIdentifier($0)) }
        if !loose.isEmpty {
            warnings.append("\(loose.count) top-level entities belong to no part; exported under Misc")
            parts.append(("Misc", loose))
        }
        // One layer per wing, in visit order; a part built inside another (the pond, inside the
        // Salon's oval) joins its wing.
        let order = ["Rotunda", "Salon", "Reserve", "SculptureHall", "ChineseWing", "HallOfLight", "Elan", "Sky"]
        var merged: [(name: String, entities: [Entity])] = []
        for (name, entities) in parts {
            if let i = merged.firstIndex(where: { $0.name == name }) { merged[i].entities += entities } else { merged.append((name, entities)) }
        }
        merged.sort { (order.firstIndex(of: $0.name) ?? order.count) < (order.firstIndex(of: $1.name) ?? order.count) }
        for (partName, entities) in merged {
            let wing = USDA.identifier(partName)
            wingOrder.append(partName)
            let key = partName == "Sky" ? "Sky" : wing
            let l = layer(key)
            let spec = l.spec(["Museum", wing])
            spec.specifier = "def"
            spec.typeName = "Xform"
            if !spec.metadata.contains(where: { $0.hasPrefix("kind") }) { spec.metadata.append("kind = \"group\"") }
            var names = Set<String>()
            for e in entities where !seen.contains(ObjectIdentifier(e)) {
                seen.insert(ObjectIdentifier(e))
                visit(e, path: ["Museum", wing, unique(baseName(e), &names)], key: key, wing: partName)
            }
        }
        materials.finalizeNames()
        for (spec, k) in bindings {
            spec.properties.append("rel material:binding = <\(materials.path(k))>")
        }
    }

    private func unique(_ base: String, _ used: inout Set<String>) -> String {
        var name = base, n = 2
        while used.contains(name) { name = "\(base)_\(n)"; n += 1 }
        used.insert(name)
        return name
    }

    private func baseName(_ e: Entity) -> String {
        if !e.name.isEmpty { return USDA.identifier(e.name) }
        if slots[ObjectIdentifier(e)] != nil { return "Canvas" }
        if let id = e.components[ModelComponent.self]?.mesh.contents.models.first?.id, id != "MeshModel", !id.isEmpty {
            return USDA.identifier(id)
        }
        if e.components.has(SpotLightComponent.self) { return "SpotLight" }
        if e.components.has(DirectionalLightComponent.self) { return "DirectionalLight" }
        if e.components.has(PointLightComponent.self) { return "PointLight" }
        if e.components.has(ModelComponent.self) { return "Model" }
        return "Group"
    }

    private func visit(_ e: Entity, path: [String], key inherited: String, wing: String) {
        var key = inherited
        if Self.helperNames.contains(e.name) || roles[ObjectIdentifier(e)] == "sun_clock_marker" { key = "Helpers" }
        let model = e.components[ModelComponent.self]
        let parts = model.map { MeshData.parts(of: $0.mesh) } ?? []
        let scanID = parts.count == 1 && scanIDs.contains(parts[0].model) ? parts[0].model : nil
        if scanID != nil { key = "Sculptures" }
        let l = layer(key)
        let spec = l.spec(path)
        spec.specifier = "def"

        // Transform, visibility, notes.
        writeTransform(e.transform, into: spec)
        if !e.isEnabled {
            spec.properties.append("token visibility = \"invisible\"")
            spec.properties.append("custom bool museevision:enabledAtStart = 0")
        }
        if !e.name.isEmpty { spec.properties.append("custom string museevision:entity = \(USDA.string(e.name))") }
        if let role = roles[ObjectIdentifier(e)] {
            spec.properties.append("custom string museevision:movingPart = \(USDA.string(role))")
        }
        if let o = e.components[OpacityComponent.self] {
            spec.properties.append("custom float museevision:opacity = \(USDA.f(o.opacity))")
        }
        let world = e.transformMatrix(relativeTo: nil)

        // Lights.
        if writeLight(e, spec: spec) {
            stats[wing, default: Stats()].lights += 1
            stats[wing, default: Stats()].include(SIMD3(world.columns.3.x, world.columns.3.y, world.columns.3.z))
        }

        // Geometry.
        let slot = slots[ObjectIdentifier(e)]
        var childNames = Set<String>()
        let singleMesh = parts.count == 1 && e.children.isEmpty && spec.typeName.isEmpty
        for p in parts {
            let material: RealityKit.Material? = model.flatMap { p.part.materialIndex < $0.materials.count ? $0.materials[p.part.materialIndex] : $0.materials.first }
            var info = material.map(materials.info) ?? MaterialInfo(kind: .other)
            if let slot {
                info = MaterialInfo(kind: .painting)
                info.texture = slot.image
                info.tint = SIMD4(Float((slot.tint >> 16) & 0xFF) / 255, Float((slot.tint >> 8) & 0xFF) / 255, Float(slot.tint & 0xFF) / 255, 1)
                info.roughness = 0.6
            }
            let keep = slot != nil || (info.texture != nil && MeshData.looksImageMapped(p.part))
            let data = MeshData.make(p.part, transform: p.transform, keepUVs: keep)
            let ds = material.map(MaterialTable.isDoubleSided) ?? false
            let k = materials.add(info)
            materials.noteUse(k, mesh: data, doubleSided: ds)
            used.insert(k)
            stats[wing, default: Stats()].materials.insert(k)
            stats[wing, default: Stats()].triangles += data.triangleCount
            stats[wing, default: Stats()].meshes += 1
            let b = data.bounds
            // Bounds are for checking against the plan, so the 800 m grass plane stays out of them.
            for c in 0..<8 where e.name != "Ground" {
                let q = SIMD3<Float>(c & 1 == 0 ? b.min.x : b.max.x, c & 2 == 0 ? b.min.y : b.max.y, c & 4 == 0 ? b.min.z : b.max.z)
                let w = world * SIMD4(q, 1)
                stats[wing, default: Stats()].include(SIMD3(w.x, w.y, w.z))
            }

            let meshSpec: PrimSpec
            if singleMesh {
                meshSpec = spec
            } else {
                spec.typeName = spec.typeName.isEmpty ? "Xform" : spec.typeName
                meshSpec = spec.child(unique("Geom", &childNames))
                meshSpec.specifier = "def"
            }
            meshSpec.typeName = "Mesh"
            meshSpec.metadata.append("prepend apiSchemas = [\"MaterialBindingAPI\"]")
            if ds { meshSpec.properties.append("uniform bool doubleSided = 1") }
            bindings.append((meshSpec, k))
            if let slot { meshSpec.properties.append("custom string museevision:image = \(USDA.string(slot.image))") }
            if let scanID {
                writeScan(scanID, data: data, spec: meshSpec)
            } else {
                meshSpec.properties += Self.meshProperties(data)
            }
        }
        if spec.typeName.isEmpty { spec.typeName = "Xform" }

        for c in e.children {
            visit(c, path: path + [unique(baseName(c), &childNames)], key: key, wing: wing)
        }
    }

    private func writeTransform(_ t: Transform, into spec: PrimSpec) {
        var order: [String] = []
        if t.translation != .zero {
            spec.properties.append("float3 xformOp:translate = \(USDA.v3(t.translation))")
            order.append("\"xformOp:translate\"")
        }
        let q = t.rotation
        if abs(q.real - 1) > 1e-7 || simd_length(q.imag) > 1e-7 {
            let n = q.normalized
            spec.properties.append("quatf xformOp:orient = (\(USDA.f(n.real)), \(USDA.f(n.imag.x)), \(USDA.f(n.imag.y)), \(USDA.f(n.imag.z)))")
            order.append("\"xformOp:orient\"")
        }
        if t.scale != SIMD3(repeating: 1) {
            spec.properties.append("float3 xformOp:scale = \(USDA.v3(t.scale))")
            order.append("\"xformOp:scale\"")
        }
        if !order.isEmpty { spec.properties.append("uniform token[] xformOpOrder = [\(order.joined(separator: ", "))]") }
    }

    /// Spot lights become SphereLights with ShapingAPI; the sun a DistantLight. Returns true if written.
    private func writeLight(_ e: Entity, spec: PrimSpec) -> Bool {
        func colour(_ c: PlatformColor) -> String {
            let s = c.usingColorSpace(.sRGB) ?? c
            let v = SIMD3<Float>(MaterialTable.linear(Float(s.redComponent)), MaterialTable.linear(Float(s.greenComponent)),
                                 MaterialTable.linear(Float(s.blueComponent)))
            return USDA.v3(v.rounded4)
        }
        if let s = e.components[SpotLightComponent.self] {
            spec.typeName = "SphereLight"
            spec.metadata.append("prepend apiSchemas = [\"ShapingAPI\"]")
            // RealityKit gives the flux in lumens. A USD sphere light's intensity is the luminance
            // of its surface (nits): flux / (4π² r²) for a small sphere of radius r.
            let r: Float = 0.05
            let nits = s.intensity / (4 * .pi * .pi * r * r)
            spec.properties += [
                "color3f inputs:color = \(colour(s.color))",
                "float inputs:intensity = \(USDA.f(nits.rounded()))",
                "float inputs:radius = \(USDA.f(r))",
                "float inputs:shaping:cone:angle = \(USDA.f(Self.coneHalfAngle(outer: s.outerAngleInDegrees)))",
                "float inputs:shaping:cone:softness = \(USDA.f(((s.outerAngleInDegrees - s.innerAngleInDegrees) / max(s.outerAngleInDegrees, 1e-3) * 1000).rounded() / 1000))",
                "custom float museevision:lumens = \(USDA.f(s.intensity))",
                "custom float museevision:innerAngleDegrees = \(USDA.f(s.innerAngleInDegrees))",
                "custom float museevision:outerAngleDegrees = \(USDA.f(s.outerAngleInDegrees))",
                "custom float museevision:attenuationRadius = \(USDA.f(s.attenuationRadius))",
                "custom bool museevision:castsShadow = \(e.components.has(SpotLightComponent.Shadow.self) ? 1 : 0)",
            ]
            return true
        }
        if let d = e.components[DirectionalLightComponent.self] {
            spec.typeName = "DistantLight"
            spec.properties += [
                "color3f inputs:color = \(colour(d.color))",
                "float inputs:intensity = \(USDA.f(d.intensity))",
                "float inputs:angle = 0.53",
                "custom float museevision:lux = \(USDA.f(d.intensity))",
                "custom bool museevision:castsShadow = \(e.components.has(DirectionalLightComponent.Shadow.self) ? 1 : 0)",
                "custom string museevision:note = \"The iPhone build's idealised sun at the export date and time; it moves with the hour.\"",
            ]
            return true
        }
        if let p = e.components[PointLightComponent.self] {
            spec.typeName = "SphereLight"
            let r: Float = 0.05
            spec.properties += [
                "color3f inputs:color = \(colour(p.color))",
                "float inputs:intensity = \(USDA.f((p.intensity / (4 * .pi * .pi * r * r)).rounded()))",
                "float inputs:radius = \(USDA.f(r))",
                "custom float museevision:lumens = \(USDA.f(p.intensity))",
                "custom float museevision:attenuationRadius = \(USDA.f(p.attenuationRadius))",
            ]
            return true
        }
        return false
    }

    /// RealityKit's spot angles are the full cone (measured: outer 60° lights a pool of radius tan 30° at 1 m);
    /// USD's shaping:cone:angle is the half-angle.
    static var coneAnglesAreFull = true
    static func coneHalfAngle(outer: Float) -> Float { coneAnglesAreFull ? outer / 2 : outer }

    static func meshProperties(_ d: MeshData) -> [String] {
        let b = d.bounds
        let big = Int.max
        return [
            "float3[] extent = [\(USDA.v3(b.min)), \(USDA.v3(b.max))]",
            "int[] faceVertexCounts = " + USDA.array(Array(repeating: 3, count: d.triangleCount), perLine: big, indent: "") { String($0) },
            "int[] faceVertexIndices = " + USDA.array(d.indices, perLine: big, indent: "") { String($0) },
            "normal3f[] normals = " + USDA.array(d.normals, perLine: big, indent: "", USDA.v3) + " (\n    interpolation = \"vertex\"\n)",
            "point3f[] points = " + USDA.array(d.points, perLine: big, indent: "", USDA.v3),
            "texCoord2f[] primvars:st = " + USDA.array(d.uvs, perLine: big, indent: "", USDA.v2) + " (\n    interpolation = \"vertex\"\n)",
            "uniform token subdivisionScheme = \"none\"",
        ]
    }

    // MARK: Sculptures

    private func writeScan(_ id: String, data: MeshData, spec: PrimSpec) {
        let file = LayerFile(path: "sculptures/\(id).usdc",
                             doc: "Musée Vision: decimated scan \(id) (from assets/sculptures/\(id).mvm). UV0 is a box projection in metres.")
        file.defaultPrim = "Scan"
        let mesh = file.spec(["Scan"])
        mesh.specifier = "def"
        mesh.typeName = "Mesh"
        mesh.properties = Self.meshProperties(data)
        scanFiles.append(file)
        spec.metadata.append("prepend references = @./\(id).usdc@")
        var fields: [(String, String)] = [("workId", id), ("scanFile", "assets/sculptures/\(id).mvm")]
        if let c = credits[id] {
            fields += [("work", c["work"] ?? ""), ("scanSource", c["source"] ?? ""), ("scanSourcePage", c["page"] ?? ""),
                       ("scanLicence", c["licence"] ?? ""), ("scanTriangles", c["triangles"] ?? "")]
        } else {
            warnings.append("no CREDITS.md row for \(id)")
        }
        let dict = fields.map { "        string \($0.0) = \(USDA.string($0.1))" }.joined(separator: "\n")
        spec.metadata.append("customData = {\n    dictionary museevision = {\n\(dict)\n    }\n}")
        for (k, v) in fields { spec.properties.append("custom string museevision:\(k) = \(USDA.string(v))") }
    }

    /// Rows of the table in assets/sculptures/CREDITS.md, by work id.
    static func readCredits(_ url: URL) -> [String: [String: String]] {
        guard let text = try? String(contentsOf: url, encoding: .utf8) else { return [:] }
        var out: [String: [String: String]] = [:]
        for line in text.split(separator: "\n") where line.hasPrefix("| ") && line.contains(".mvm") {
            let cells = line.split(separator: "|", omittingEmptySubsequences: false).map { $0.trimmingCharacters(in: .whitespaces) }
            // | file | work | author/source | page | licence | triangles | notes |
            guard cells.count >= 8 else { continue }
            let id = cells[1].replacingOccurrences(of: ".mvm", with: "")
            out[id] = ["work": cells[2], "source": cells[3], "page": cells[4], "licence": cells[5], "triangles": cells[6]]
        }
        return out
    }

    // MARK: Output

    var usedMaterials: Set<Int> { used }
}
