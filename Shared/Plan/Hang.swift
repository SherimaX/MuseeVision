import simd

/// One hung canvas: which artwork, which image file, where it hangs and how big it is.
struct HungCanvas: Identifiable {
    let id: String            // unique per canvas (Rouen has five)
    let number: Int           // number on the SalonPlan / SalonHang boards
    let artworkID: String     // id in artworks.json
    let image: String         // file name (without .jpg) in assets/paintings
    /// Centre of the picture on its wall, in plan coordinates (x, z).
    let position: SIMD2<Float>
    /// Unit vector out of the wall, into the room (the way the picture faces).
    let facing: SIMD2<Float>
    let width: Float          // metres
    let height: Float
    /// Height of the picture's centre above the floor. Boards: 1.55 m to centre; large works
    /// sit 0.6 m above the floor.
    var centreHeight: Float { max(Plan.Salon.hangCentre, Plan.Salon.minimumBottom + height / 2) }
}

/// A Water Lilies panel on the oval wall, between two ellipse angles (radians, measured
/// from the east apex, positive towards the south).
struct LilyPanel: Identifiable {
    let id: String
    let title: String
    let image: String
    let from: Float
    let to: Float
    /// True proportions of the panel (width / height), from the image.
    let aspect: Float
}

/// The hang of Salon Impression — positions from the SalonPlan board (lines at true width
/// along each wall), sizes from the SalonHang board (cm). 37 works, 44 pieces.
enum Hang {
    private static let north = SIMD2<Float>(0, 1)    // on a north wall, facing south
    private static let south = SIMD2<Float>(0, -1)
    private static let east = SIMD2<Float>(1, 0)     // facing east
    private static let west = SIMD2<Float>(-1, 0)
    private static let nWall: Float = -Plan.Salon.halfWidth
    private static let sWall: Float = Plan.Salon.halfWidth

    private static func on(_ n: Int, _ id: String, x: Float, z: Float, _ f: SIMD2<Float>, w: Float, h: Float,
                           image: String? = nil, suffix: String = "") -> HungCanvas {
        HungCanvas(id: id + suffix, number: n, artworkID: id, image: image ?? id, position: [x, z], facing: f,
                   width: w, height: h)
    }

    static let canvases: [HungCanvas] = {
        let cab = Plan.Salon.Cabinet.self
        var c: [HungCanvas] = [
            // Bay 1 · The Birth of Impressionism · oxblood
            on(1, "monet-impression-sunrise", x: Plan.Salon.Stele.paintingX, z: 1.4, east, w: 0.63, h: 0.48),
            on(2, "bazille-family-reunion", x: -14, z: -4.0, west, w: 2.30, h: 1.52),
            on(3, "monet-women-in-the-garden", x: -19.70, z: sWall, south, w: 2.05, h: 2.55),
            on(4, "sisley-flood-port-marly", x: -16.20, z: sWall, south, w: 0.81, h: 0.60),
            on(5, "pissarro-hoarfrost", x: -23.20, z: sWall, south, w: 0.93, h: 0.65),
            on(6, "monet-the-magpie", x: -16.40, z: nWall, north, w: 1.30, h: 0.89),
            on(7, "monet-terrace-sainte-adresse", x: -23.40, z: nWall, north, w: 1.30, h: 0.98),
            // Cabinet · Around the Group: Manet · cabinet green
            on(8, "manet-luncheon-on-the-grass", x: -20.0, z: cab.z0, north, w: 2.65, h: 2.08),
            on(9, "manet-olympia", x: cab.x0, z: -11.6, west, w: 1.90, h: 1.30),
            on(10, "manet-bar-folies-bergere", x: cab.x1, z: -11.6, east, w: 1.30, h: 0.96),
            // Bay 2 · Modern Paris · Paris grey
            on(11, "pissarro-boulevard-montmartre-winter", x: -28.60, z: nWall, north, w: 0.81, h: 0.65),
            on(12, "caillebotte-paris-street-rainy-day", x: -32.00, z: nWall, north, w: 2.76, h: 2.12),
            on(13, "pissarro-boulevard-montmartre-night", x: -35.40, z: nWall, north, w: 0.65, h: 0.53),
            on(14, "monet-gare-saint-lazare", x: -28.20, z: sWall, south, w: 1.04, h: 0.75),
            on(15, "caillebotte-floor-scrapers", x: -30.80, z: sWall, south, w: 1.47, h: 1.02),
            on(16, "degas-absinthe", x: -33.40, z: sWall, south, w: 0.69, h: 0.92),
            on(17, "degas-dance-class", x: -35.60, z: sWall, south, w: 0.77, h: 0.84),
            // (18, Little Dancer, stands on a plinth — see Plan.Salon.dancerPlinth)
            // Bay 3 · Leisure and Light · sage
            on(19, "monet-la-grenouillere", x: -40.80, z: nWall, north, w: 1.00, h: 0.75),
            on(20, "renoir-boating-party", x: -44.00, z: nWall, north, w: 1.73, h: 1.30),
            on(21, "monet-woman-with-a-parasol", x: -47.20, z: nWall, north, w: 0.81, h: 1.00),
            on(22, "renoir-dance-at-bougival", x: -40.80, z: sWall, south, w: 0.98, h: 1.82),
            on(23, "renoir-bal-du-moulin", x: -44.00, z: sWall, south, w: 1.75, h: 1.31),
            on(24, "monet-poppy-field", x: -47.20, z: sWall, south, w: 0.65, h: 0.50),
            // Bay 4 · Women Impressionists · dusty rose
            on(25, "morisot-harbor-lorient", x: -51.90, z: nWall, north, w: 0.73, h: 0.44),
            on(26, "morisot-summers-day", x: -53.60, z: nWall, north, w: 0.75, h: 0.46),
            on(27, "morisot-the-cradle", x: -56.00, z: nWall, north, w: 0.46, h: 0.56),
            on(28, "morisot-dining-room", x: -58.20, z: nWall, north, w: 0.50, h: 0.61),
            on(29, "cassatt-childs-bath", x: -60.00, z: nWall, north, w: 0.66, h: 1.00),
            on(30, "renoir-la-loge", x: -52.40, z: sWall, south, w: 0.64, h: 0.80),
            on(31, "cassatt-in-the-loge", x: -53.60, z: sWall, south, w: 0.66, h: 0.81),
            on(32, "cassatt-blue-armchair", x: -56.40, z: sWall, south, w: 1.30, h: 0.90),
            on(33, "cassatt-boating-party", x: -59.40, z: sWall, south, w: 1.17, h: 0.90),
            // Bay 5 · Monet's Series · ochre
            on(34, "monet-stacks-of-wheat", x: -66.00, z: sWall, south, w: 1.00, h: 0.60),
            on(35, "monet-bridge-water-lilies", x: -70.60, z: sWall, south, w: 0.74, h: 0.93),
        ]
        // 36 · Five Rouen Cathedrals in a row on Bay 5's north wall, c. 100 × 65 cm each.
        for (i, x) in [-66.1, -67.2, -68.3, -69.4, -70.5].enumerated() {
            c.append(on(36, "monet-rouen-cathedral", x: Float(x), z: nWall, north, w: 0.65, h: 1.00,
                        image: "monet-rouen-cathedral-\(i + 1)", suffix: "-\(i + 1)"))
        }
        return c
    }()

    /// 37 · The Water Lilies cycle on the oval's curved walls, in the order you meet them
    /// walking round from the door: Morning (NE), The Clouds (NW), Green Reflections (SW),
    /// Setting Sun (SE). Angle ranges from the plan's arcs.
    static let lilyPanels: [LilyPanel] = {
        let d = Float.pi / 180
        return [
            LilyPanel(id: "morning", title: "Morning", image: "monet-water-lilies-morning", from: -18 * d, to: -86 * d, aspect: 6.39),
            LilyPanel(id: "clouds", title: "The Clouds", image: "monet-water-lilies-clouds", from: -94 * d, to: -178 * d, aspect: 6.63),
            LilyPanel(id: "green-reflections", title: "Green Reflections", image: "monet-water-lilies-green-reflections", from: 178 * d, to: 94 * d, aspect: 4.34),
            LilyPanel(id: "setting-sun", title: "Setting Sun", image: "monet-water-lilies-setting-sun", from: 86 * d, to: 18 * d, aspect: 3.02),
        ]
    }()
    static let lilyArtworkID = "monet-water-lilies-orangerie"
    static let dancerArtworkID = "degas-little-dancer"
}
