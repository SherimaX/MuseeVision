import simd

/// Every dimension of the building, taken from the boards on the Musée Vision design
/// canvas (MasterPlan, Rotunda, SalonPlan, SalonHang, SalonDetails).
///
/// Plan convention (as on the boards): metres, Rotunda centre = origin, x east, y south.
/// In RealityKit the plan's y becomes z (so south = +z, north = -z) and height is y.
/// The Salon sheets are drawn in "salon data" coordinates (x measured west from the inner
/// face of Bay 1's end wall) and mirrored east–west on screen; `Plan.salonX` converts.
enum Plan {
    /// Salon data x (metres west of Bay 1's end wall) → world x.
    static func salonX(_ d: Float) -> Float { -14 - d }

    static let eyeHeight: Float = 1.6

    // MARK: Rotunda (board: Rotunda — plan at 15 px/m, section at 30 px/m)
    enum Rotunda {
        static let radius: Float = 10            // Ø 20 m drum
        static let wall: Float = 1.2             // drum wall thickness
        static let drumHeight: Float = 10        // springing of the dome
        static let oculusRadius: Float = 2.5     // Ø 5 m eye
        static let latticeHeight: Float = 20.3   // steel-and-glass lattice above the eye
        static let clockRadius: Float = 7        // Ø 14 m sun clock
        static let pilasters = 16
        static let pilasterWidth: Float = 0.9
        static let pilasterDepth: Float = 0.45
        static let pilasterHeight: Float = 9.47
        /// Doors (clear width in the drum wall): W and E 3 m, N and S 4 m, arched,
        /// springing at 4.5 m.
        static let doorSpring: Float = 4.5
        /// Niches on the diagonals: 2 m wide, sill 2.0 m, springing 4.67 m.
        static let nicheWidth: Float = 2.0
        static let nicheSill: Float = 2.0
        static let nicheSpring: Float = 4.67
        static let coffers = (rings: 5, perRing: 28)
    }

    // MARK: Salon Impression (boards: SalonPlan at 21 px/m, SalonDetails section at 36 px/m)
    enum Salon {
        static let endWallX: Float = -14         // inner face of Bay 1's end wall
        static let farWallX: Float = -74         // inner face of Bay 5's end wall (salon data 60 m)
        static let halfWidth: Float = 7          // bays are 14 m wide
        static let wall: Float = 0.6
        static let wallHeight: Float = 6.5       // walls to the vault springing
        static let vaultRadius: Float = 7        // coffered barrel vault, crown 13.5 m
        static let skylightHalfWidth: Float = 1.5
        static let archIntrados: Float = 4.5     // transverse arches: 9 m clear
        static let pierDepth: Float = 2.5
        static let pierThickness: Float = 1.2
        static let door = (width: Float(3), spring: Float(3.3))   // arched, top 4.8 m
        static let hangCentre: Float = 1.55
        static let minimumBottom: Float = 0.6

        struct Bay {
            let number: Int
            let x0: Float, x1: Float            // world x, east (x0) to west (x1)
            let colour: UInt32
            let name: String
            let skylight: (x0: Float, x1: Float)
        }
        /// Bay extents: Bay 1 and Bay 5 are 11.4 m, Bays 2–4 10.8 m, piers 1.2 m.
        static let bays: [Bay] = [
            Bay(number: 1, x0: salonX(0), x1: salonX(11.4), colour: 0x7A3B2E, name: "The Birth of Impressionism",
                skylight: (salonX(0.6), salonX(10.8))),
            Bay(number: 2, x0: salonX(12.6), x1: salonX(23.4), colour: 0x5D6B75, name: "Modern Paris",
                skylight: (salonX(13.2), salonX(22.8))),
            Bay(number: 3, x0: salonX(24.6), x1: salonX(35.4), colour: 0x8C9A7B, name: "Leisure and Light",
                skylight: (salonX(25.2), salonX(34.8))),
            Bay(number: 4, x0: salonX(36.6), x1: salonX(47.4), colour: 0xB08A80, name: "Women Impressionists",
                skylight: (salonX(37.2), salonX(46.8))),
            Bay(number: 5, x0: salonX(48.6), x1: salonX(60), colour: 0xB79A62, name: "Monet's Series",
                skylight: (salonX(49.2), salonX(59.4))),
        ]
        /// Pier centres between bays (salon data 12, 24, 36, 48).
        static let piers: [Float] = [12, 24, 36, 48].map { salonX($0) }

        /// Manet cabinet off Bay 1's north side: 8 × 8 m, salon data x 2–10, plan y −15.6 to −7.6.
        enum Cabinet {
            static let x0: Float = salonX(2), x1: Float = salonX(10)
            static let z0: Float = -15.6, z1: Float = -7.6
            static let height: Float = 5.5
            static let colour: UInt32 = 0x2F3A33
            static let doorCentreX: Float = salonX(6)   // opening salon data 4.5–7.5
        }

        /// Glass stele carrying Impression, Sunrise: 1.2 × 2.4 m low-iron glass, 1.4 m south
        /// of the axis, the painting on its east face.
        enum Stele {
            static let x: Float = -20.15
            static let z: Float = 1.4
            static let width: Float = 1.2
            static let height: Float = 2.4
            static let paintingX: Float = -20.062
        }

        /// Degas's Little Dancer: plinth 0.8 m square (sculpture scans left out for now).
        static let dancerPlinth = SIMD2<Float>(-34.5, 3.4)
    }

    // MARK: Passage Rotunda → Salon (3 m wide, from the drum to Bay 1's end wall)
    enum Passage {
        static let halfWidth: Float = 1.5
        static let x0: Float = -10.9, x1: Float = -13.4
    }

    // MARK: Nymphéas oval (SalonPlan, SalonHang oval elevation at 22 px/m, SalonDetails)
    enum Oval {
        static let centre = SIMD2<Float>(salonX(72.5), 0)   // x = −86.5
        static let a: Float = 11.0, b: Float = 7.5            // inner face (wall centreline 11.3 × 7.8)
        static let wall: Float = 0.6
        static let height: Float = 5.0
        static let velariumRise: Float = 1.0
        static let colour: UInt32 = 0xF1EDE4
        static let panelBottom: Float = 0.7
        static let panelHeight: Float = 2.0
        static let passageX0: Float = -74.6, passageX1: Float = -76.0
        /// Pond: 8.4 × 4.6 m basin with a 0.3 m stone rim.
        static let pond = (a: Float(4.2), b: Float(2.3), rim: Float(0.3), height: Float(0.45))
        /// Lily pads (x, z, radius) and which carry a flower, from the plan.
        static let lilies: [(x: Float, z: Float, r: Float, flower: Bool)] = [
            (-83.60, -0.60, 0.45, true), (-84.60, 0.90, 0.38, false), (-85.90, -1.20, 0.42, false),
            (-86.90, 0.50, 0.50, true), (-88.10, -0.70, 0.36, false), (-89.10, 0.80, 0.44, true),
            (-89.60, -0.40, 0.30, false), (-83.20, 0.50, 0.30, false), (-85.60, 1.40, 0.30, true),
            (-87.60, 1.50, 0.28, false),
        ]
        /// Four curved stone benches facing the panels (centrelines from the plan), 0.55 m wide.
        static let benches: [[SIMD2<Float>]] = [
            [[-91.58, 1.16], [-91.40, 1.44], [-91.18, 1.70], [-90.92, 1.95], [-90.64, 2.19], [-90.32, 2.40], [-89.97, 2.60], [-89.60, 2.79], [-89.20, 2.94], [-88.78, 3.08], [-88.35, 3.20]],
            [[-84.65, 3.20], [-84.22, 3.08], [-83.80, 2.94], [-83.40, 2.79], [-83.03, 2.60], [-82.68, 2.40], [-82.36, 2.19], [-82.08, 1.95], [-81.82, 1.70], [-81.60, 1.44], [-81.42, 1.16]],
            [[-81.42, -1.16], [-81.60, -1.44], [-81.82, -1.70], [-82.08, -1.95], [-82.36, -2.19], [-82.68, -2.40], [-83.03, -2.60], [-83.40, -2.79], [-83.80, -2.94], [-84.22, -3.08], [-84.65, -3.20]],
            [[-88.35, -3.20], [-88.78, -3.08], [-89.20, -2.94], [-89.60, -2.79], [-89.97, -2.60], [-90.32, -2.40], [-90.64, -2.19], [-90.92, -1.95], [-91.18, -1.70], [-91.40, -1.44], [-91.58, -1.16]],
        ]
        static let benchWidth: Float = 0.55
        static let benchHeight: Float = 0.45
    }
}
