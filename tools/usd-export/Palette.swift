/// Names for the colour-only materials, keyed by family and sRGB colour ("matte:EFE8DB").
/// A colour not listed here is named by family and hex ("matte_A1B2C3"). If a colour in the
/// Swift build changes, change its key here too, or its material is renamed on re-export
/// (and Unreal loses the material assigned to it).
enum Palette {
    static let names: [String: String] = [
        // Stone, plaster and mouldings.
        "matte:EFE8DB": "stone",                 // Mat.stone: dome, curbs, lunettes
        "matte:E4DBCB": "moulding",              // Mat.moulding: cornices, skirting, plinths
        "matte:F1EBDF": "plaster",               // passages
        "matte:F1EDE4": "plaster_oval",
        "matte:ECE7DE": "plaster_vestibule",
        "matte:FBF9F4": "whitewash",             // Chinese Wing walls
        "matte:D9CDBA": "plaster_shaft",
        "matte:CFC6B8": "stone_plinth",
        "matte:CFC6B6": "stone_stair",
        "matte:D8CDB9": "stone_coping",
        "matte:DCCFB9": "stone_bench",
        "matte:EAE0CE": "stone_basin",
        "matte:B8B2A6": "stone_grey",            // moon gate surround
        "matte:77726B": "roof_coping",
        "matte:1E1C19": "roof_ridge",
        "matte:E9DFCD": "plinth_cream",
        "matte:F4EFE5": "lantern_white",
        // Salon wall colours (Plan.Salon bays, the Manet cabinet).
        "matte:7A3B2E": "wall_bay_1",
        "matte:5D6B75": "wall_bay_2",
        "matte:8C9A7B": "wall_bay_3",
        "matte:B08A80": "wall_bay_4",
        "matte:B79A62": "wall_bay_5",
        "matte:2F3A33": "wall_cabinet",
        "matte:2E2924": "floor_dark",            // the Square's dark rooms
        // Timber, silk, paper, furniture.
        "matte:4A3A2C": "timber_dark",
        "matte:9C8A74": "seat_rail",
        "matte:5E574D": "cord",
        "matte:C9BB98": "silk_mount",
        "matte:DDCFAE": "silk_mount_pale",
        "matte:E4D8BA": "silk_bed",
        "matte:D9CDB0": "roller_ivory",
        "matte:D9C9B0": "plan_chest",
        "matte:F2EDE3": "photo_mount",
        // Sculpture and ceramics.
        "matte:F7F3EA": "marble_carrara",        // The Kiss
        "matte:E9E1D2": "stone_facade",          // La Danse (Opéra façade copy)
        "matte:EDE6D8": "plaster_cast",          // Rude's head
        "matte:A9C4C0": "celadon_ru",
        "matte:C58B3C": "sancai_amber",
        "matte:E6D6B4": "sancai_cream",
        "matte:6E8B4F": "sancai_green",
        "matte:CBB58A": "sancai_base",
        "metal:6B5A45": "bronze",                // Rodin's bronzes
        "metal:4A3B2C": "bronze_dark",
        "metal:6B5236": "bronze_warm",
        "metal:6B4E2E": "bronze_palette",        // Mat.bronze
        "metal:5A4632": "bronze_brown",          // readers, easel, stele foot, photo edges
        "metal:7E5C25": "bronze_gold",           // node, frames, case rings
        "metal:8A6A3E": "bronze_light",          // the pond's posts
        "metal:C9A266": "gilt",                  // Mat.gilt: frames, handrails, mast
        "metal:E0B347": "gold_lily",
        // Steel and glass.
        "metal:3C3F42": "steel_lattice",
        "metal:3B3A36": "steel_lattice_hall",
        "metal:1E1C19": "steel_dark",            // the Atrium's ribs
        "metal:2C6E73": "steel_teal",            // the car's frame
        "glass:A9CFCB": "glass_hall",
        "glass:BFE0E0": "glass_teal",
        "glass:CFE3E1": "glass_floor",
        "glass:CFE6E5": "glass_stele",
        "glass:CFE8E8": "glass_car_door",
        "glass:D8ECEC": "glass_landing",
        "glass:DDE8E6": "glass_frosted",
        "glass:E3EFEF": "glass_oculus",
        "glass:E9F2F1": "glass_vitrine",
        "glass:EEF3F2": "glass_case",
        "glass:EFF4F3": "glass_rail",
        "glass:E8EEEC": "glass_stand_in",        // a missing scan's stand-in
        // Water and planting.
        "matte:3F5E5A": "water_pond",
        "metal:2F4A4A": "water_garden",
        "matte:5E8A4E": "lily_pad",
        "matte:F2C9D2": "lily_flower",
        "matte:E3A9B5": "lotus_flower",
        "matte:8FA27F": "foliage_green",
        "matte:6F8466": "foliage_dark",
        "matte:8DA56A": "foliage_birch",
        "matte:7F9A5E": "foliage_orchard",
        "matte:5F7A4E": "hedge",
        "matte:5E7456": "bamboo",
        "matte:6F8456": "flower_stem",
        "matte:A08AB0": "flower_violet",
        "matte:D9A5A0": "flower_pink",
        "matte:C9826A": "flower_terracotta",
        "matte:E6C77A": "flower_yellow",
        "matte:D8D2C4": "bark_birch",
        "matte:6B5A45": "bark_orchard",
        "matte:5A4632": "bark_brown",
        "matte:8E897F": "pond_bank",
        "matte:B4AFA4": "taihu_rock",
        "matte:6F6A61": "rock_hollow",
        // Light and self-lit surfaces.
        "glow:FFE3A0": "glow_rim_light",
        "glow:E9B75A": "glow_lily_halo",
        "glow:7CC3C5": "glow_car",
        "glow:E3ECEA": "glow_iris",
        "glow:F4E9C8": "glow_ring_of_light",
        "glow:F4F0E4": "moon_reflection",
        "glow:E4DBCB": "veil",
        "glow:FFF3D0": "sun_patch",
        "glow:2A2016": "point_of_shadow",
        "glow:B8AE9C": "painting_placeholder",
        "glow:9FBCD8": "sky_day",
        "glow:0E141C": "sky_night",
        "glow:FFFFFF": "moon",
    ]
}
