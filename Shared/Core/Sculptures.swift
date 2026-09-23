import Foundation
import RealityKit
import simd

/// Loads the sculpture scans (decimated, in the app's small "MVM1" mesh format — see
/// assets/sculptures/CREDITS.md) and places them. A work without a scan shows a frosted
/// glass stand-in of its size, so the room still reads.
extension MuseumScene {
    static func loadScan(_ id: String) -> MeshResource? {
        guard let url = Bundle.main.url(forResource: id, withExtension: "mvm", subdirectory: "sculptures")
                ?? Bundle.main.url(forResource: id, withExtension: "mvm"),
              let data = try? Data(contentsOf: url), data.count > 12 else { return nil }
        return data.withUnsafeBytes { raw -> MeshResource? in
            guard raw.loadUnaligned(fromByteOffset: 0, as: UInt32.self) == 0x314D_564D else { return nil } // "MVM1"
            let vc = Int(raw.loadUnaligned(fromByteOffset: 4, as: UInt32.self))
            let tc = Int(raw.loadUnaligned(fromByteOffset: 8, as: UInt32.self))
            let need = 12 + vc * 24 + tc * 12
            guard raw.count >= need else { return nil }
            var pos = [SIMD3<Float>](repeating: .zero, count: vc)
            var nor = [SIMD3<Float>](repeating: .zero, count: vc)
            var idx = [UInt32](repeating: 0, count: tc * 3)
            var o = 12
            for i in 0..<vc {
                pos[i] = [raw.loadUnaligned(fromByteOffset: o, as: Float.self), raw.loadUnaligned(fromByteOffset: o + 4, as: Float.self),
                          raw.loadUnaligned(fromByteOffset: o + 8, as: Float.self)]
                o += 12
            }
            for i in 0..<vc {
                nor[i] = [raw.loadUnaligned(fromByteOffset: o, as: Float.self), raw.loadUnaligned(fromByteOffset: o + 4, as: Float.self),
                          raw.loadUnaligned(fromByteOffset: o + 8, as: Float.self)]
                o += 12
            }
            for i in 0..<(tc * 3) {
                idx[i] = raw.loadUnaligned(fromByteOffset: o, as: UInt32.self)
                o += 4
            }
            var d = MeshDescriptor(name: id)
            d.positions = MeshBuffers.Positions(pos)
            d.normals = MeshBuffers.Normals(nor)
            d.primitives = .triangles(idx)
            return try? MeshResource.generate(from: [d])
        }
    }

    /// Places a scanned work with its base at `at`, its front towards `facing` (plan x, z).
    /// Returns true if a scan was found. Adds a pick target on its bounding box unless one is given.
    @discardableResult
    func placeSculpture(id: String, at base: SIMD3<Float>, facing f: SIMD2<Float>, height: Float,
                        material: RealityKit.Material, pickBox: (SIMD3<Float>, SIMD3<Float>)?,
                        standInFootprint: SIMD2<Float> = [0.8, 0.8], detail: String? = nil) -> Bool {
        let yaw = atan2(f.x, f.y)
        if let mesh = Self.loadScan(id) {
            // Scans are open shells in places (backs, cut plinths): draw both sides.
            var mat = material
            if var pbr = mat as? PhysicallyBasedMaterial { pbr.faceCulling = .none; mat = pbr }
            let e = ModelEntity(mesh: mesh, materials: [mat])
            e.name = id
            e.position = base
            e.orientation = simd_quatf(angle: yaw, axis: [0, 1, 0])
            groundShadow(e)
            building.addChild(e)
            if pickBox == nil {
                let b = e.visualBounds(relativeTo: nil)
                targets.append(PickTarget(artworkID: id, detail: detail, hit: { o, d in Self.rayBox(o, d, min: b.min, max: b.max) }))
            }
            return true
        }
        // Stand-in: a frosted glass volume of the work's size, with a faint bronze base line.
        var b = MeshBuilder()
        let w = standInFootprint.x / 2, dpt = standInFootprint.y / 2
        b.box(min: [-w, 0, -dpt], max: [w, height, dpt])
        let e = ModelEntity(mesh: b.mesh(name: id + " stand-in"), materials: [Mat.glass(0xE8EEEC, opacity: 0.16)])
        e.position = base
        e.orientation = simd_quatf(angle: yaw, axis: [0, 1, 0])
        building.addChild(e)
        if pickBox == nil {
            let bb = e.visualBounds(relativeTo: nil)
            targets.append(PickTarget(artworkID: id, detail: detail ?? "The 3D scan is not in the app yet.",
                                      hit: { o, d in Self.rayBox(o, d, min: bb.min, max: bb.max) }))
        }
        return false
    }
}
