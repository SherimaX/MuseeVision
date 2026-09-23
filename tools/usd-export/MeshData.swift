import Foundation
import RealityKit
import simd

/// One triangle mesh ready for USD: indexed (welded) points with vertex normals and UVs.
struct MeshData {
    var points: [SIMD3<Float>] = []
    var normals: [SIMD3<Float>] = []
    var uvs: [SIMD2<Float>] = []
    var indices: [Int32] = []
    var materialIndex = 0
    /// How the UVs were made: kept (image mapped), scaled to metres, box-projected, or mixed.
    var uvMode = "none"
    /// Metres per original UV unit (u, v), when the original UVs were scaled.
    var uvScale: SIMD2<Float> = [1, 1]

    var triangleCount: Int { indices.count / 3 }

    var bounds: (min: SIMD3<Float>, max: SIMD3<Float>) {
        guard let first = points.first else { return (.zero, .zero) }
        var lo = first, hi = first
        for p in points { lo = simd_min(lo, p); hi = simd_max(hi, p) }
        return (lo, hi)
    }

    /// The parts of a RealityKit mesh (one per instance × part), with instance transforms baked in.
    static func parts(of mesh: MeshResource) -> [(model: String, part: MeshResource.Part, transform: float4x4)] {
        let contents = mesh.contents
        var out: [(String, MeshResource.Part, float4x4)] = []
        for instance in contents.instances {
            guard let model = contents.models[instance.model] else { continue }
            for part in model.parts { out.append((model.id, part, instance.transform)) }
        }
        return out
    }

    /// Reads a part. `keepUVs` keeps the original texture coordinates (for image-mapped
    /// materials such as paintings and lettering); otherwise UV0 is made to be in metres.
    static func make(_ part: MeshResource.Part, transform: float4x4, keepUVs: Bool) -> MeshData {
        let pos = part.positions.elements
        let nor = part.normals?.elements
        let tex = part.textureCoordinates?.elements
        let idx: [UInt32] = part.triangleIndices?.elements ?? Array(0..<UInt32(pos.count))
        let identity = transform == matrix_identity_float4x4
        let normalMatrix = simd_transpose(simd_inverse(simd_float3x3(
            SIMD3(transform.columns.0.x, transform.columns.0.y, transform.columns.0.z),
            SIMD3(transform.columns.1.x, transform.columns.1.y, transform.columns.1.z),
            SIMD3(transform.columns.2.x, transform.columns.2.y, transform.columns.2.z))))
        func p(_ i: UInt32) -> SIMD3<Float> {
            let v = pos[Int(i)]
            if identity { return v }
            let w = transform * SIMD4(v, 1)
            return SIMD3(w.x, w.y, w.z)
        }
        func n(_ i: UInt32, face: SIMD3<Float>) -> SIMD3<Float> {
            guard let nor else { return face }
            let v = identity ? nor[Int(i)] : normalMatrix * nor[Int(i)]
            let l = simd_length(v)
            return l > 1e-8 ? v / l : face
        }

        var d = MeshData()
        d.materialIndex = part.materialIndex
        let triCount = idx.count / 3
        // Corners: position, normal, uv.
        var cp = [SIMD3<Float>](), cn = [SIMD3<Float>](), cu = [SIMD2<Float>]()
        cp.reserveCapacity(idx.count); cn.reserveCapacity(idx.count); cu.reserveCapacity(idx.count)
        var faces = [SIMD3<Float>]()
        faces.reserveCapacity(triCount)
        for t in 0..<triCount {
            let a = idx[3 * t], b = idx[3 * t + 1], c = idx[3 * t + 2]
            let pa = p(a), pb = p(b), pc = p(c)
            var face = cross(pb - pa, pc - pa)
            let fl = simd_length(face)
            face = fl > 1e-12 ? face / fl : [0, 1, 0]
            faces.append(face)
            cp += [pa, pb, pc]
            cn += [n(a, face: face), n(b, face: face), n(c, face: face)]
            if let tex { cu += [tex[Int(a)], tex[Int(b)], tex[Int(c)]] } else { cu += [.zero, .zero, .zero] }
        }

        if keepUVs {
            d.uvMode = "image"
        } else {
            // Metres per UV unit along u and v for each triangle whose UVs are not degenerate.
            var su: [Float] = [], sv: [Float] = []
            var good = [Bool](repeating: false, count: triCount)
            for t in 0..<triCount {
                let e1 = cp[3 * t + 1] - cp[3 * t], e2 = cp[3 * t + 2] - cp[3 * t]
                let d1 = cu[3 * t + 1] - cu[3 * t], d2 = cu[3 * t + 2] - cu[3 * t]
                let det = d1.x * d2.y - d1.y * d2.x
                let area = simd_length(cross(e1, e2))
                guard abs(det) > 1e-10, area > 1e-10 else { continue }
                // [e1 e2] = J [d1 d2]  →  J = [e1 e2] · inverse([d1 d2]).
                let ju = (e1 * d2.y - e2 * d1.y) / det
                let jv = (e2 * d1.x - e1 * d2.x) / det
                let lu = simd_length(ju), lv = simd_length(jv)
                guard lu.isFinite, lv.isFinite, lu > 1e-6, lv > 1e-6 else { continue }
                good[t] = true
                su.append(lu); sv.append(lv)
            }
            func median(_ a: [Float]) -> Float {
                let s = a.sorted()
                return s.isEmpty ? 1 : s[s.count / 2]
            }
            func snap(_ x: Float) -> Float {
                if abs(x - 1) < 2e-3 { return 1 }
                return (x * 10000).rounded() / 10000
            }
            let scale = SIMD2<Float>(snap(median(su)), snap(median(sv)))
            d.uvScale = scale
            let goodCount = su.count
            d.uvMode = goodCount == triCount ? (scale == [1, 1] ? "metres" : "scaled") : (goodCount == 0 ? "box" : "mixed")
            for t in 0..<triCount {
                if good[t] {
                    for k in 0..<3 { cu[3 * t + k] *= scale }
                } else {
                    for k in 0..<3 { cu[3 * t + k] = boxUV(cp[3 * t + k], faces[t]) }
                }
            }
        }

        // Weld identical corners.
        struct Key: Hashable { let a: SIMD4<Int32>; let b: SIMD4<Int32> }
        func q(_ x: Float, _ s: Float) -> Int32 { Int32(clamping: Int((x * s).rounded())) }
        var map: [Key: Int32] = [:]
        map.reserveCapacity(cp.count)
        d.indices.reserveCapacity(cp.count)
        for i in 0..<cp.count {
            let P = cp[i], N = cn[i], U = cu[i]
            let key = Key(a: [q(P.x, 1e5), q(P.y, 1e5), q(P.z, 1e5), q(N.x, 1e4)],
                          b: [q(N.y, 1e4), q(N.z, 1e4), q(U.x, 1e5), q(U.y, 1e5)])
            if let j = map[key] {
                d.indices.append(j)
            } else {
                let j = Int32(d.points.count)
                map[key] = j
                d.points.append(P); d.normals.append(N); d.uvs.append(U)
                d.indices.append(j)
            }
        }
        return d
    }

    /// Box projection in metres by the face's dominant axis, oriented so the texture reads
    /// upright on walls and north-up on floors.
    static func boxUV(_ p: SIMD3<Float>, _ n: SIMD3<Float>) -> SIMD2<Float> {
        let a = abs(n)
        if a.y >= a.x && a.y >= a.z { return n.y >= 0 ? [p.x, -p.z] : [p.x, p.z] }
        if a.x >= a.z { return n.x >= 0 ? [-p.z, p.y] : [p.z, p.y] }
        return n.z >= 0 ? [p.x, p.y] : [-p.x, p.y]
    }

    /// True when the part's own UVs look like an image mapping: most triangles have UV area and
    /// every coordinate lies within the unit square (give or take 5%, as the sun clock bleeds past its image).
    static func looksImageMapped(_ part: MeshResource.Part) -> Bool {
        guard let tex = part.textureCoordinates?.elements, !tex.isEmpty else { return false }
        let idx: [UInt32] = part.triangleIndices?.elements ?? Array(0..<UInt32(tex.count))
        var lo = SIMD2<Float>(repeating: .infinity), hi = SIMD2<Float>(repeating: -.infinity)
        for t in tex { lo = simd_min(lo, t); hi = simd_max(hi, t) }
        guard lo.x > -0.05, lo.y > -0.05, hi.x < 1.05, hi.y < 1.05 else { return false }
        var good = 0
        let tris = idx.count / 3
        for t in 0..<tris {
            let a = tex[Int(idx[3 * t])], b = tex[Int(idx[3 * t + 1])], c = tex[Int(idx[3 * t + 2])]
            let d1 = b - a, d2 = c - a
            if abs(d1.x * d2.y - d1.y * d2.x) > 1e-10 { good += 1 }
        }
        return tris > 0 && good * 10 >= tris * 9
    }
}
