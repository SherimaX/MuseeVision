import CoreGraphics
import Foundation
import RealityKit
import simd

/// Where the visitor is on Earth (for the real sky, the moon and the solar terms).
/// Defaults to Paris until the device reports a location.
struct Observer {
    /// Until the device reports where it is, guess from the time zone: its offset gives the
    /// longitude (15° an hour); the latitude is a temperate 40° on the right side of the equator.
    var latitude: Double = Observer.guessLatitude()
    var longitude: Double = Double(TimeZone.current.secondsFromGMT()) / 240

    static func guessLatitude() -> Double {
        let id = TimeZone.current.identifier
        let south = ["Australia", "Pacific/Auckland", "America/Sao_Paulo", "America/Argentina", "America/Santiago",
                     "Africa/Johannesburg", "America/Montevideo", "America/Lima"]
        return south.contains(where: { id.hasPrefix($0) }) ? -34 : 40
    }
}

/// Low-precision ephemerides (after Meeus and the Astronomical Almanac's short formulas):
/// good to a fraction of a degree — plenty for a sky seen by eye.
enum Ephemeris {
    static let deg = Double.pi / 180

    static func julianDay(_ date: Date) -> Double { date.timeIntervalSince1970 / 86400 + 2440587.5 }

    /// Days since J2000.0.
    static func n(_ date: Date) -> Double { julianDay(date) - 2451545.0 }

    static func obliquity(_ n: Double) -> Double { (23.439 - 0.0000004 * n) * deg }

    /// Sun's apparent ecliptic longitude (radians).
    static func sunLongitude(_ date: Date) -> Double {
        let n = n(date)
        let L = (280.460 + 0.9856474 * n).truncatingRemainder(dividingBy: 360)
        let g = (357.528 + 0.9856003 * n) * deg
        return (L + 1.915 * sin(g) + 0.020 * sin(2 * g)) * deg
    }

    /// Moon's ecliptic longitude and latitude (radians), about ±1°.
    static func moonEcliptic(_ date: Date) -> (lon: Double, lat: Double) {
        let n = n(date)
        let L = 218.316 + 13.176396 * n
        let M = (134.963 + 13.064993 * n) * deg
        let F = (93.272 + 13.229350 * n) * deg
        let D = (297.850 + 12.190749 * n) * deg
        let lon = L + 6.289 * sin(M) - 1.274 * sin(M - 2 * D) + 0.658 * sin(2 * D) + 0.214 * sin(2 * M)
        let lat = 5.128 * sin(F)
        return (lon * deg, lat * deg)
    }

    static func equatorial(lon: Double, lat: Double, _ date: Date) -> (ra: Double, dec: Double) {
        let e = obliquity(n(date))
        let ra = atan2(sin(lon) * cos(e) - tan(lat) * sin(e), cos(lon))
        let dec = asin(sin(lat) * cos(e) + cos(lat) * sin(e) * sin(lon))
        return (ra, dec)
    }

    /// Local sidereal time (radians).
    static func localSiderealTime(_ date: Date, longitude: Double) -> Double {
        let gmst = 280.46061837 + 360.98564736629 * n(date)
        return ((gmst + longitude).truncatingRemainder(dividingBy: 360)) * deg
    }

    /// Rotation taking equatorial unit vectors (x to the vernal equinox, z to the north celestial
    /// pole) into world space (x east, y up, z south).
    static func skyRotation(_ date: Date, _ o: Observer) -> simd_float3x3 {
        let lst = Float(localSiderealTime(date, longitude: o.longitude))
        let phi = Float(o.latitude * deg)
        // e' = Rz(−LST)·c gives (cos δ cos H, −cos δ sin H, sin δ) with H the hour angle.
        let rz = simd_float3x3(columns: ([cos(lst), -sin(lst), 0], [sin(lst), cos(lst), 0], [0, 0, 1]))
        // Horizon frame: east = b, up = cos φ a + sin φ c, south (world +z) = sin φ a − cos φ c.
        let m = simd_float3x3(rows: [[0, 1, 0], [cos(phi), 0, sin(phi)], [sin(phi), 0, -cos(phi)]])
        return m * rz
    }

    static func equatorialVector(ra: Double, dec: Double) -> SIMD3<Float> {
        [Float(cos(dec) * cos(ra)), Float(cos(dec) * sin(ra)), Float(sin(dec))]
    }

    /// World direction of the sun and its altitude (radians).
    static func sun(_ date: Date, _ o: Observer) -> (dir: SIMD3<Float>, altitude: Float) {
        let eq = equatorial(lon: sunLongitude(date), lat: 0, date)
        let d = skyRotation(date, o) * equatorialVector(ra: eq.ra, dec: eq.dec)
        return (d, asin(d.y))
    }

    /// World direction of the moon, its illuminated fraction and whether it is waxing.
    static func moon(_ date: Date, _ o: Observer) -> (dir: SIMD3<Float>, altitude: Float, illuminated: Float, waxing: Bool) {
        let m = moonEcliptic(date)
        let eq = equatorial(lon: m.lon, lat: m.lat, date)
        let d = skyRotation(date, o) * equatorialVector(ra: eq.ra, dec: eq.dec)
        var elong = (m.lon - sunLongitude(date)).truncatingRemainder(dividingBy: 2 * .pi)
        if elong < 0 { elong += 2 * .pi }
        return (d, asin(d.y), Float((1 - cos(elong)) / 2), elong < .pi)
    }

    // MARK: The 24 solar terms (節氣), every 15° of the sun's longitude from Lichun at 315°.

    static let solarTerms: [(hanzi: String, pinyin: String, english: String)] = [
        ("立春", "Lìchūn", "Start of Spring"), ("雨水", "Yǔshuǐ", "Rain Water"), ("驚蟄", "Jīngzhé", "Awakening of Insects"),
        ("春分", "Chūnfēn", "Spring Equinox"), ("清明", "Qīngmíng", "Pure Brightness"), ("穀雨", "Gǔyǔ", "Grain Rain"),
        ("立夏", "Lìxià", "Start of Summer"), ("小滿", "Xiǎomǎn", "Grain Buds"), ("芒種", "Mángzhòng", "Grain in Ear"),
        ("夏至", "Xiàzhì", "Summer Solstice"), ("小暑", "Xiǎoshǔ", "Minor Heat"), ("大暑", "Dàshǔ", "Major Heat"),
        ("立秋", "Lìqiū", "Start of Autumn"), ("處暑", "Chǔshǔ", "End of Heat"), ("白露", "Báilù", "White Dew"),
        ("秋分", "Qiūfēn", "Autumn Equinox"), ("寒露", "Hánlù", "Cold Dew"), ("霜降", "Shuāngjiàng", "Frost's Descent"),
        ("立冬", "Lìdōng", "Start of Winter"), ("小雪", "Xiǎoxuě", "Minor Snow"), ("大雪", "Dàxuě", "Major Snow"),
        ("冬至", "Dōngzhì", "Winter Solstice"), ("小寒", "Xiǎohán", "Minor Cold"), ("大寒", "Dàhán", "Major Cold"),
    ]

    /// Index 0…23 of the current solar term (0 = Lichun).
    static func solarTerm(_ date: Date) -> Int {
        var lon = sunLongitude(date) / deg - 315
        lon = lon.truncatingRemainder(dividingBy: 360)
        if lon < 0 { lon += 360 }
        return Int(lon / 15) % 24
    }
}

/// The stars of the Yale Bright Star Catalogue as small glowing sprites on a sphere around
/// the eye. Stars are at infinity: the field follows the camera, only its rotation changes.
@MainActor
enum StarField {
    struct Star { var ra: Float; var dec: Float; var mag: Float; var bv: Float }

    static func load() -> [Star] {
        guard let url = Bundle.main.url(forResource: "stars", withExtension: "bin", subdirectory: "sky")
                ?? Bundle.main.url(forResource: "stars", withExtension: "bin"),
              let data = try? Data(contentsOf: url), data.count > 8 else { return [] }
        return data.withUnsafeBytes { raw -> [Star] in
            let count = Int(raw.loadUnaligned(fromByteOffset: 4, as: UInt32.self))
            var out: [Star] = []
            out.reserveCapacity(count)
            for i in 0..<count {
                let o = 8 + i * 16
                guard o + 16 <= raw.count else { break }
                out.append(Star(ra: raw.loadUnaligned(fromByteOffset: o, as: Float.self),
                                dec: raw.loadUnaligned(fromByteOffset: o + 4, as: Float.self),
                                mag: raw.loadUnaligned(fromByteOffset: o + 8, as: Float.self),
                                bv: raw.loadUnaligned(fromByteOffset: o + 12, as: Float.self)))
            }
            return out
        }
    }

    /// Colour of a star from its B−V index (blue-white to orange).
    static func colour(_ bv: Float) -> UInt32 {
        switch bv {
        case ..<0.0: return 0xAFC8FF
        case ..<0.3: return 0xDCE6FF
        case ..<0.6: return 0xFFF8EC
        case ..<1.0: return 0xFFE7C2
        case ..<1.4: return 0xFFD29A
        default: return 0xFFBC7A
        }
    }

    /// Builds the field: a few meshes (one per colour), each star a small square facing the
    /// centre, sized by brightness. `radius` should be well inside the camera's far plane.
    static func make(radius r: Float = 180) -> Entity {
        let root = Entity()
        root.name = "Star field"
        let stars = load()
        let sprite = Textures.resource(Textures.starSprite())
        var bins: [UInt32: MeshBuilder] = [:]
        for s in stars where s.mag < 6.5 {
            let v = SIMD3<Float>(cos(s.dec) * cos(s.ra), cos(s.dec) * sin(s.ra), sin(s.dec))
            // Angular size: bright stars larger (a sprite ~0.2°–1.1° across).
            let size = r * (0.0035 + 0.017 * pow(max(0, 6.5 - s.mag) / 8, 1.6))
            let up: SIMD3<Float> = abs(v.z) > 0.95 ? [1, 0, 0] : [0, 0, 1]
            let t1 = normalize(cross(up, v)) * size / 2, t2 = normalize(cross(v, t1)) * size / 2
            let c = v * r
            let n = -v
            bins[colour(s.bv), default: MeshBuilder()].quad(c - t1 - t2, c + t1 - t2, c + t1 + t2, c - t1 + t2, normal: n,
                                                            uv: ([0, 0], [1, 0], [1, 1], [0, 1]))
        }
        for (hex, b) in bins {
            var m = UnlitMaterial()
            m.color = .init(tint: PlatformColor(hex: hex), texture: .init(sprite))
            m.blending = .transparent(opacity: .init(floatLiteral: 1))
            let e = ModelEntity(mesh: b.mesh(name: "stars"), materials: [m])
            root.addChild(e)
        }
        return root
    }
}
