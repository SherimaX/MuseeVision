#if DEBUG
import CoreGraphics
import ImageIO
import Foundation
import RealityKit
import simd

/// Perf-lab only (branch perf-lab): scene statistics and iOS 27 rendering-feature probes.
@MainActor
enum Lab {
    static var enabled: Bool { ProcessInfo.processInfo.arguments.contains("-lab") }
    static var stats: Bool { ProcessInfo.processInfo.arguments.contains("-stats") }
    static var effects: Bool { ProcessInfo.processInfo.arguments.contains("-effects") }
    static func on(_ f: String) -> Bool {
        let a = ProcessInfo.processInfo.arguments
        return a.contains("-lab-all") || a.contains("-lab-" + f)
    }

    /// Prints and appends to Documents/lab.log (stdout isn't captured by simctl here).
    static func L(_ line: String) {
        print(line)
        let url = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0].appendingPathComponent("lab.log")
        let data = Data((line + "\n").utf8)
        if let h = try? FileHandle(forWritingTo: url) { h.seekToEndOfFile(); h.write(data); try? h.close() }
        else { try? data.write(to: url) }
    }

    // MARK: Scene statistics

    static func printStats(_ root: Entity) {
        var models = 0, parts = 0, tris = 0, verts = 0, unlit = 0, pbr = 0, transparent = 0, lights = 0, entities = 0
        var meshBytes = 0
        var area: Double = 0, areaUnlit: Double = 0
        var biggest: [(String, Int)] = []
        func walk(_ e: Entity) {
            entities += 1
            if e.components.has(SpotLightComponent.self) || e.components.has(PointLightComponent.self)
                || e.components.has(DirectionalLightComponent.self) { lights += 1 }
            if let m = e.components[ModelComponent.self] {
                models += 1
                var t = 0
                for model in m.mesh.contents.models {
                    for p in model.parts {
                        parts += 1
                        let v = p.positions.count
                        verts += v
                        let i = p.triangleIndices?.count ?? 0
                        t += i / 3
                        // positions + normals (12 B each) + uv (8 B) + 4 B index per corner
                        meshBytes += v * 32 + i * 4
                        // World-space surface area (architecture only: skip scans and the sky).
                        let scan = e.name.hasPrefix("rodin") || e.name.hasPrefix("carpeaux") || e.name.hasPrefix("bourdelle")
                            || e.name.hasPrefix("degas") || e.name.hasPrefix("rude") || e.name.hasPrefix("chinese-sancai")
                        if !scan, let tri = p.triangleIndices { let idx = Array(tri)
                            let pos = Array(p.positions)
                            let mtx = e.transformMatrix(relativeTo: nil)
                            func w(_ k: UInt32) -> SIMD3<Float> { let q = mtx * SIMD4<Float>(pos[Int(k)], 1); return [q.x, q.y, q.z] }
                            var a: Double = 0
                            for t in stride(from: 0, to: idx.count - 2, by: 3) {
                                let A = w(idx[t]), B = w(idx[t + 1]), C = w(idx[t + 2])
                                a += Double(simd_length(simd_cross(B - A, C - A))) / 2
                            }
                            if a < 200_000 { area += a } // ignore the sky dome/sphere shells
                            if a < 200_000, m.materials.contains(where: { $0 is UnlitMaterial }) { areaUnlit += a }
                        }
                    }
                }
                tris += t
                biggest.append((e.name.isEmpty ? "(unnamed)" : e.name, t))
                for mat in m.materials {
                    if mat is UnlitMaterial { unlit += 1 }
                    if let p = mat as? PhysicallyBasedMaterial {
                        pbr += 1
                        if case .transparent = p.blending { transparent += 1 }
                    }
                }
            }
            for c in e.children { walk(c) }
        }
        walk(root)
        L("[lab] entities=\(entities) models=\(models) parts=\(parts) triangles=\(tris) vertices=\(verts) meshMB≈\(meshBytes / 1_048_576)")
        L("[lab] surface area (architecture, lit+unlit) ≈ \(Int(area)) m², of which unlit ≈ \(Int(areaUnlit)) m²")
        L("[lab] materials: pbr=\(pbr) unlit=\(unlit) transparentPBR=\(transparent) lights=\(lights)")
        for (n, t) in biggest.sorted(by: { $0.1 > $1.1 }).prefix(15) { L("[lab]   \(t) tris  \(n)") }
    }

    /// Texture memory the current streamer would hold at a few eye positions (RGBA8 + mips vs ASTC 4×4).
    static func paintingMemory(_ scene: MuseumScene) {
        let eyes: [(String, SIMD3<Float>)] = [("Rotunda", [0, 1.6, 0]), ("Salon east", [-20, 1.6, 0]), ("Salon mid", [-40, 1.6, 0]),
                                              ("Salon west", [-60, 1.6, 0]), ("Chinese", [0, 1.6, 25]), ("Hall of Light", [25, 1.6, 0])]
        var dims: [Int: (Int, Int)] = [:]
        for (i, slot) in scene.paintingSlots.enumerated() {
            guard let url = Bundle.main.url(forResource: slot.image, withExtension: "jpg", subdirectory: "paintings"),
                  let src = CGImageSourceCreateWithURL(url as CFURL, nil),
                  let props = CGImageSourceCopyPropertiesAtIndex(src, 0, nil) as? [CFString: Any],
                  let w = props[kCGImagePropertyPixelWidth] as? Int, let h = props[kCGImagePropertyPixelHeight] as? Int else { continue }
            let k = Double(slot.maxPixels) / Double(max(w, h))
            dims[i] = k < 1 ? (Int(Double(w) * k), Int(Double(h) * k)) : (w, h)
        }
        L("[lab] painting slots=\(scene.paintingSlots.count) with images=\(dims.count)")
        for (name, eye) in eyes {
            var n = 0, texels = 0
            for (i, slot) in scene.paintingSlots.enumerated() where simd_distance(slot.position(), eye) < 34 {
                if let d = dims[i] { n += 1; texels += d.0 * d.1 }
            }
            let rgba = Double(texels) * 4 * 4 / 3 / 1_048_576, astc = Double(texels) * 1 * 4 / 3 / 1_048_576
            L("[lab] \(name): \(n) images loaded, RGBA8+mips ≈ \(Int(rgba)) MB, ASTC4x4 ≈ \(Int(astc)) MB")
        }
    }

    // MARK: Export probe

    /// Tries RealityKit's own writer on the whole museum (to the app's Documents folder).
    static func export(_ root: Entity) async {
        let docs = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        for name in ["museum.usdz", "museum.reality"] {
            let url = docs.appendingPathComponent(name)
            let t0 = Date()
            do {
                try await root.write(to: url)
                let size = (try? FileManager.default.attributesOfItem(atPath: url.path)[.size] as? Int) ?? -1
                L("[lab] export \(name): ok, \(size / 1_048_576) MB in \(Date().timeIntervalSince(t0)) s → \(url.path)")
            } catch {
                L("[lab] export \(name) failed: \(error)")
            }
        }
    }

    // MARK: iOS 27 feature probes

    /// Adds test objects on the Rotunda's north side (look from -pose "0 3 0 -8").
    static func install(in root: Entity) {
        guard #available(iOS 27.0, *) else { L("[lab] iOS 27 APIs unavailable"); return }
        let lab = Entity()
        lab.name = "Lab"
        root.addChild(lab)

        // 1. Scene-wide tone mapping and bloom (components on the root).
        if on("post") {
        root.components.set(ToneMappingComponent(exposure: 0.0))
        root.components.set(BloomComponent(scope: .unbounded))
        var bloom = BloomOptionsComponent()
        bloom.strength = 0.35
        bloom.threshold = 1.0
        root.components.set(bloom)
        L("[lab] tone mapping + bloom components set")
        }

        // 2. A lightmapped panel (final shaded colour bake, sampled through texture-coordinate set 1).
        if on("lm") { do {
            let atlas = gradientAtlas()
            let tex = try TextureResource.texture2DArray(slices: [atlas], options: .init(semantic: .color, mipmapsMode: .allocateAndGenerateAll))
            var ref = LightmapResource.AtlasReference()
            ref.atlasTextureIndex = 0
            ref.atlasTextureSlice = 0
            ref.uvOffset = [0, 0]
            ref.uvScale = [1, 1]
            let part = try LightmapResource.MeshPartLightmapDescriptor(
                bakeDescriptor: .finalShadedColor(.init(sourceAtlasReference: ref)))
            let entityDesc = try LightmapResource.EntityLightmapDescriptor(perPartData: [part])
            let part2 = try LightmapResource.MeshPartLightmapDescriptor(
                bakeDescriptor: .indirectDiffuseIrradiance(.init(sourceAtlasReference: ref)))
            let entityDesc2 = try LightmapResource.EntityLightmapDescriptor(perPartData: [part2])
            let resource = try LightmapResource(atlasTextures: [tex], perEntityData: [entityDesc, entityDesc2])

            let beauty = ModelEntity(mesh: panelMesh(uv1Crop: true), materials: [LightmapComponent.FinalShadedColorBakeMaterial()])
            beauty.name = "Lab lightmap beauty"
            beauty.position = [-1.6, 0.2, -3.5]
            // Same bake as indirect irradiance on an ordinary PBR material.
            var pbr = PhysicallyBasedMaterial()
            pbr.baseColor = .init(tint: .white)
            pbr.roughness = 0.6
            let irr = ModelEntity(mesh: panelMesh(), materials: [pbr])
            irr.name = "Lab lightmap irradiance"
            irr.position = [1.6, 0.2, -3.5]
            let holder = Entity()
            holder.name = "Lab lightmap holder"
            holder.addChild(beauty)
            holder.addChild(irr)
            lab.addChild(holder)
            var lc = LightmapComponent(resource: resource)
            lc.entityIndexInLightmapResource = [beauty: 0, irr: 1]
            holder.components.set(lc)
            L("[lab] LightmapComponent set (bakeTypes=\(resource.bakeTypes), entities=\(resource.entityCount))")
        } catch {
            L("[lab] lightmap failed: \(error)")
        } }

        // 3. A soft-shadowed spot light over a box on the floor.
        var box = MeshBuilder()
        box.box(min: [-0.3, 0, -0.3], max: [0.3, 1.2, 0.3])
        let block = ModelEntity(mesh: box.mesh(name: "Lab block"), materials: [Mat.matte(0xD8CDB8)])
        block.position = [0, 0, -5.5]
        lab.addChild(block)
        let spot = Entity()
        spot.name = "Lab soft spot"
        var sc = SpotLightComponent(color: .white, intensity: 60_000, innerAngleInDegrees: 30, outerAngleInDegrees: 60, attenuationRadius: 12)
        sc.attenuationFalloffExponent = 2
        spot.components.set(sc)
        var sh = SpotLightComponent.Shadow()
        sh.quality = .high
        sh.lightSize = 0.6
        spot.components.set(sh)
        spot.position = [1.2, 5.0, -4.8]
        spot.look(at: [0, 0, -5.5], from: spot.position, relativeTo: nil)
        if on("shadow") { lab.addChild(spot); L("[lab] soft-shadow spot light set") }

        // 4. A glossy, subsurface marble sphere with a local (box-projected) reflection probe.
        var marble = PhysicallyBasedMaterial()
        marble.baseColor = .init(tint: PlatformColor(hex: 0xF0ECE4))
        marble.roughness = 0.15
        marble.subsurfaceWeight = 0.6
        let ball = ModelEntity(mesh: .generateSphere(radius: 0.45), materials: [marble])
        ball.position = [-1.2, 0.45, -5.5]
        if on("sss") { lab.addChild(ball); L("[lab] subsurface material set") }
    }

    /// A 2 × 2 m upright panel with UV0 (0…1) and UV1 (the lightmap set, 0…1).
    static func panelMesh(uv1Crop: Bool = false) -> MeshResource {
        var d = MeshDescriptor(name: "lab panel")
        let p: [SIMD3<Float>] = [[-1, 0, 0], [1, 0, 0], [1, 2, 0], [-1, 2, 0]]
        d.positions = MeshBuffers.Positions(p)
        d.normals = MeshBuffers.Normals(Array(repeating: [0, 0, 1], count: 4))
        let uv: [SIMD2<Float>] = [[0, 0], [1, 0], [1, 1], [0, 1]]
        d.textureCoordinates = MeshBuffers.TextureCoordinates(uv)
        // uv1Crop: UV1 covers only the centre half of the atlas, so a UV0 read would look different.
        let uv1 = uv1Crop ? uv.map { $0 * 0.5 + 0.25 } : uv
        if #available(iOS 27.0, *) { d.textureCoordinates1 = MeshBuffers.TextureCoordinates(uv1) }
        d.primitives = .triangles([0, 1, 2, 0, 2, 3])
        return try! MeshResource.generate(from: [d])
    }

    /// A stand-in "bake": a warm sun pool fading into cool shade, with a hard shadow bar.
    static func gradientAtlas() -> CGImage {
        Textures.draw(width: 256, height: 256) { ctx in
            let cs = CGColorSpaceCreateDeviceRGB()
            let g = CGGradient(colorsSpace: cs, colors: [Textures.cg(0xFFE2A8), Textures.cg(0x39414F)] as CFArray, locations: [0, 1])!
            ctx.drawRadialGradient(g, startCenter: CGPoint(x: 90, y: 90), startRadius: 0, endCenter: CGPoint(x: 90, y: 90), endRadius: 220, options: [.drawsAfterEndLocation])
            ctx.setFillColor(Textures.cg(0x1A1A22, 0.7))
            ctx.fill(CGRect(x: 150, y: 0, width: 24, height: 256))
        }
    }
}
#endif
