"""
Put the native architecture in the Museum map (safe to re-run; run after import_wing.py and relight.py):

    UnrealEditor-Cmd.exe MuseeVision.uproject -run=pythonscript -script="<repo>/windows/Scripts/native.py"

The Unreal build is the museum's source of truth. Where a room has been rebuilt natively in C++
(Source/MuseeVision/<Room>/<Room>Structure), its actor is placed here at the origin and the
imported pieces it replaces are retired (relight.retire: hidden, no collision, off the building list).
Rooms: the Rotunda (ARotundaStructure), the Salon with the Manet cabinet and the Nymphéas oval
(ASalonStructure), the Square under the Atrium (ASquareStructure) and the Reserve (AReserveStructure,
with its pendants and vault uplights lit, and AReserveRacks, which arranges the export's works for its racks).
The Élan's structure and elevator are placed by import_wing.py.
"""
import importlib
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import musee_paths as P  # noqa: E402
import relight  # noqa: E402
import setup_project as S  # noqa: E402

NATIVE_TAG = "musee.native"

# Class → (wing, the imported prims it replaces).
ROOMS = {
    "/Script/MuseeVision.RotundaStructure": ("Rotunda", [
        "/Museum/Rotunda/Drum", "/Museum/Rotunda/Drum_mouldings", "/Museum/Rotunda/Pilasters",
        "/Museum/Rotunda/Dome", "/Museum/Rotunda/Dome_coffers", "/Museum/Rotunda/Oculus_curb",
        "/Museum/Rotunda/Lattice", "/Museum/Rotunda/Oculus_glass", "/Museum/Rotunda/Bronze_node",
        # The passages out of the W and N doors are the Rotunda's now (to x −13.4 and y −13.5).
        "/Museum/Salon/Passage", "/Museum/SculptureHall/Sculpture_passage",
    ]),
    # The Salon, the Manet cabinet and the Nymphéas oval (its benches, velarium and walls); the list
    # is the class's own (ASalonStructure::GetReplacedImportPrims), read when the class loads.
    "/Script/MuseeVision.SalonStructure": ("Salon", "class"),
    # Élan Cube: the Cube (Élan, level −1; replaces the Square, see retire_square): light-field panels in a concrete box,
    # the glass shaft through the ground, the ceiling iris's ring. It keeps the Square's imported prims retired.
    "/Script/MuseeVision.CubeStructure": ("Elan", "class", [], (5400.0, 0.0, 0.0)),
    # Élan's exterior: a tall domed hall (plinth, glass drum with fins, entablature, latticed pearl dome,
    # lantern), placed at the Atrium's centre.
    "/Script/MuseeVision.ElanExteriorStructure": ("Elan", [], [], (5400.0, 0.0, 0.0)),
    # The Hall of Light: the glazed garden hall (steel arches, pearl lattice, plinths, print stands).
    "/Script/MuseeVision.HallOfLightStructure": ("HallOfLight", "class"),
    # The Rotunda's sun clock: the bronze-and-marble dial that rises as a drum over the spiral stair.
    "/Script/MuseeVision.SunClock": ("Rotunda", "class"),
    # Albion (England 1848-1898) on the south door, in place of the Chinese Wing (whose code stays in ChineseWing/ and
    # whose imported prims, all of them, Albion's class retires): the court of iron and glass, its porch, its outside;
    # and its sky (London's weather on the glass, the stained glass's colours on the floor). albion_place.py removes the
    # Chinese Wing's native actor and places the Kelmscott Chaucer and the hang.
    "/Script/MuseeVision.AlbionStructure": ("Albion", "class"),
    "/Script/MuseeVision.AlbionSky": ("Albion", []),
    # Albion's night light: twelve electroliers in the arcades' bays and one in the porch, lit at dusk (AAlbionLamps).
    "/Script/MuseeVision.AlbionLamps": ("Albion", []),
    # The exterior: a Beaux-Arts dress round the stone wings (podium, Ionic order, portico, balustrades,
    # the Rotunda's dome cover) and the grounds (parvis, canal, hedges, walks, lamps); Source/MuseeVision/Facade.
    "/Script/MuseeVision.MuseeFacadeStructure": ("Exterior", "class"),
    "/Script/MuseeVision.MuseeLandscape": ("Exterior", "class"),
    # The site's ground, with the opening over the sun clock's stair shaft.
    "/Script/MuseeVision.MuseeGround": ("HallOfLight", "class"),
    # The Classical Hall below the Rotunda: the Braccio Nuovo-like gallery and the domed tribune.
    "/Script/MuseeVision.ClassicalHallStructure": ("ClassicalHall", []),
    # The Atrium's floor, stone base, cove, bay letters and the Starry Night's glass stele.
    "/Script/MuseeVision.AtriumBaseStructure": ("Elan", "class"),
    # The long stair from the Nymphéas oval's pond down to the Reserve, its shaft and handrails.
    "/Script/MuseeVision.ReserveStairStructure": ("Reserve", "class"),
    # Chenghuai 澄懷 on the Rotunda's north door, in place of the Sculpture Hall (whose code stays in SculptureHall/; its
    # imported prims, all of them, Chenghuai's class retires; chenghuai.native_place removes its native actor and lights
    # and places the works): the three-court house, its Suzhou garden, their cases, lamps and water.
    "/Script/MuseeVision.ChenghuaiStructure": ("Chenghuai", "class"),
    # The Reserve: the brick undercroft (walls, vault, piers, floor, plan chests, easel) and its sliding racks (the
    # export's rack frames and unlettered racks retired; the lettered racks' works, the pastels and the easel's
    # painting placed for the layout by AReserveRacks.ArrangeImported).
    "/Script/MuseeVision.ReserveStructure": ("Reserve", "class"),
    "/Script/MuseeVision.ReserveRacks": ("Reserve", "class"),
    # The seats (Salon banquettes, the oval's pond benches, the Sculpture Hall's and the Chinese terrace's) and the bronze
    # thresholds at every doorway (Source/MuseeVision/Furniture); they replace nothing imported (the rooms retire theirs).
    "/Script/MuseeVision.MuseeFurniture": ("Furniture", []),
    # Salon interior: the velvet benches beside the line (bays 2-5), and the lanterns' sky (the weather from Giverny, the
    # diffusers' and the velarium's light). The case, the willows and the pond's kerb are placed by salon_interior.place.
    "/Script/MuseeVision.SalonBenches": ("Salon", []),
    "/Script/MuseeVision.SalonSky": ("Salon", []),
}

# Materials set on a native room after it is placed, by class (property → asset in /Game/Museum/Materials).
MATERIAL_OVERRIDES = {
    # The Rotunda's walls and pilasters: cream marble with grey crackle veins (rendering 05).
    "/Script/MuseeVision.RotundaStructure": {"wall_material": "M_Marble_Wall", "pilaster_material": "M_Marble_Wall"},
    # The Salon's viewing stones: their flush rings in brushed bronze, the thresholds' metal (materials spec 3.2).
    "/Script/MuseeVision.SalonStructure": {"bronze_material": "M_Bronze_Brushed"},
    # The site's lawn: photographed grass (materials.py), not the export's flat green.
    "/Script/MuseeVision.MuseeGround": {"ground_material": "M_Lawn"},
}

# The Atrium's drum glass between the ribs: pearl, a little under the roof's 900 nits (not grey).
ELAN_DRUM_NITS = 750.0   # (07's membrane glows silver-white: 500 read grey)
# The ribs' and lattice's faint luminosity by day (nits; the roof is 900).
ELAN_RIB_NITS = 140.0

# The Reserve: an opal globe on each bay's pendant with a lamp inside it, the nave's (on the axis) larger and brighter
# than the aisles'. The lamp's colour is the globe's own (reserve_materials.LAMP_KELVIN, a warm incandescent 2900 K);
# its source is a sphere of 0.8 R (the lit opal is the source), and the bronze gallery over the neck shadows it, so
# the vault over a globe gets the gallery's soft cut-off rather than a uniform wash.
PENDANT_LUMENS = 3400.0
AISLE_PENDANT_LUMENS = 2600.0
PENDANT_KELVIN = 2900.0
PENDANT_SOURCE = 0.8                     # × the globe's radius
# The vault uplights (rendering 04): a small can on each corner of every pier's impost, one up each groin
# (AReserveStructure.get_uplight_transforms: the fittings are the actor's). They cast shadows (non-shadowing ones lit
# the void behind the vault through its seams: the streak), with a small source (a 45 mm lens).
UPLIGHT_LUMENS = 320.0
UPLIGHT_KELVIN = PENDANT_KELVIN
UPLIGHT_SOURCE_CM = 1.5
PENDANT_TAG = "musee.pendant"
# The Reserve's white balance: a camera in a room lit only by 2900 K lamps is balanced part of the way towards them
# (the eye adapts too), so the lamplight reads warm, not orange, and the lime mortar reads cream. A post-process
# volume over the Reserve (priority over the museum's 6600 K); tagged as an exposure zone so exposure.py leaves it be.
RESERVE_WHITE_K = 4300.0
RESERVE_BOX_CM = ((-7460.0, -1260.0, -600.0), (-1340.0, 1260.0, -5.0))
# The export's three aisle spots (plan x −66, −52, −36, 3.6 m over the floor) give way to the pendants.
RESERVE_AISLE_SPOTS_X = (-66.0, -52.0, -36.0)
RESERVE_FLOOR = -5.8
# The export's warm spot down the long stair (plan x −89.8, h −0.6): AReserveStairStructure lights it now.
STAIR_SPOT = (-89.8, 0.0, -0.6)
# The easel's spot (the export's, part:easel_light): high in the nave's east bay, off the axis, on the easel's painting.
EASEL_SPOT_AT = (-19.6, 1.9, -1.55)
EASEL_SPOT_LUMENS = 1400.0
EASEL_WORK = "caillebotte-pont-europe"


def log(msg):
    unreal.log(f"[native] {msg}")


def make_reserve_materials():
    """M_TealStrip (the glowing strips beside the racks), M_OpalGlobe (the Élan lantern's and the grounds' globes) and
    the Reserve's own (reserve_materials.py: its brick, the globes' opal and glass, the cable)."""
    try:
        import reserve_materials
        reserve_materials.make_all()
    except Exception as e:  # noqa: BLE001 - the Reserve falls back to MI_Brick_Coursed and M_OpalGlobe
        unreal.log_warning(f"[native] reserve_materials: {e}")
    made = []
    for name, colour, nits in (("M_TealStrip", unreal.LinearColor(0.10, 0.75, 0.85, 1), 60.0),
                               ("M_OpalGlobe", unreal.LinearColor(1.0, 0.82, 0.62, 1), 2500.0)):
        if unreal.EditorAssetLibrary.does_asset_exist(f"{P.MATERIALS}/{name}"):
            continue
        m = S.new_material(P.MATERIALS, name)
        m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        glow = S.multiply(m, S.constant3(m, colour, -600, 0), S.scalar_param(m, "Luminance", nits, -600, 200), -300, 0)
        S.MEL.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        S.MEL.recompile_material(m)
        unreal.EditorAssetLibrary.save_loaded_asset(m)
        made.append(name)
    if made:
        log(f"made {', '.join(made)}")


def make_rib_pearl():
    """M_RibPearl: the Atrium's ribs, pearl-white painted steel (rendering 07), not dark steel."""
    name = "M_RibPearl"
    if unreal.EditorAssetLibrary.does_asset_exist(f"{P.MATERIALS}/{name}"):
        return unreal.load_asset(f"{P.MATERIALS}/{name}")
    m = S.new_material(P.MATERIALS, name)
    m.set_editor_property("two_sided", True)
    S.MEL.connect_material_property(S.constant3(m, S.hex_colour(0xECE9E3)), "", unreal.MaterialProperty.MP_BASE_COLOR)
    for prop, value, y in ((unreal.MaterialProperty.MP_ROUGHNESS, 0.32, 150), (unreal.MaterialProperty.MP_SPECULAR, 0.55, 250)):
        c = S.MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, y)
        c.set_editor_property("r", value)
        S.MEL.connect_material_property(c, "", prop)
    S.MEL.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m)
    log(f"made {name}")
    return m


def make_rib_pearl_glow():
    """
    M_RibPearlGlow: the Atrium's ribs and lattice, pearl-white steel with a faint luminosity that follows
    the daylight (MPC_Musee "Daylight", none at night). Against the 900-nit roof, lit steel alone reads
    grey from below (its undersides see only the floor): rendering 07's ribs are pearl-white. Rebuilt in
    place on every run.
    """
    name = "M_RibPearlGlow"
    m = S.new_material(P.MATERIALS, name)
    m.set_editor_property("two_sided", True)
    colour = S.constant3(m, S.hex_colour(0xECE9E3), -700, 0)
    S.MEL.connect_material_property(colour, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = S.MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 150)
    rough.set_editor_property("r", 0.32)
    S.MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mpc = unreal.load_asset(f"{P.MATERIALS}/MPC_Musee")
    nits = S.multiply(m, colour, S.scalar_param(m, "Luminance", ELAN_RIB_NITS, -700, 250), -450, 0)
    glow = S.multiply(m, nits, S.daylight_factor(m, mpc, 0.0), -200, 0)
    S.MEL.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    S.MEL.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m)
    return m


def dress_elan(eas):
    make_rib_pearl()
    rib = make_rib_pearl_glow()
    cls = unreal.load_class(None, "/Script/MuseeVision.ElanStructure")
    if cls and not any(a.get_class() == cls for a in eas.get_all_level_actors()):
        # (as import_wing.place_elan_structure: at the Atrium's centre)
        a = eas.spawn_actor_from_class(cls, unreal.Vector(5400, 0, 0), unreal.Rotator(0, 0, 0))
        a.set_actor_label("Elan_drum_and_Sphere")
        a.tags = [unreal.Name("musee.building"), unreal.Name("musee.wing:Elan"),
                  unreal.Name("prim:/Museum/Elan/Drum_and_Sphere")]
        log("Élan: the drum and the Sphere placed again")
    for a in eas.get_all_level_actors():
        if cls and a.get_class() == cls and rib:
            a.set_editor_property("rib_material", rib)   # an edit: the actor rebuilds itself
            a.set_editor_property("drum_nits", ELAN_DRUM_NITS)
            log("Élan: ribs in pearl-white steel")


# The temporary square strata pit (before ASquareStructure) is removed wherever it was placed.
STRATA_TAG = "musee.strata_pit"


# Élan Cube: the Square (ASquareStructure) is retired; its code stays, its actor goes.
def retire_square(eas):
    cls = unreal.load_class(None, "/Script/MuseeVision.SquareStructure")
    for a in eas.get_all_level_actors():
        if cls and a.get_class() == cls:
            eas.destroy_actor(a)
            log("Élan: the Square's actor removed (the Cube replaces it)")


def remove_old_strata_pit(eas):
    for a in eas.get_all_level_actors():
        if STRATA_TAG in [str(t) for t in a.tags]:
            eas.destroy_actor(a)


def place_rooms(eas):
    placed = []
    by_prim = {}
    for a in eas.get_all_level_actors():
        path = relight.tag_value(a, "prim:")
        if path:
            by_prim.setdefault(path, []).append(a)
    for class_path, room in ROOMS.items():
        wing, retired, extra = room[0], room[1], (room[2] if len(room) > 2 else [])
        cls = unreal.load_class(None, class_path)
        if not cls:
            log(f"no class {class_path}: build the editor first")
            continue
        for a in eas.get_all_level_actors():
            if a.get_class() == cls:
                eas.destroy_actor(a)
        if retired in ("class", "class+"):
            # The room's own list (GetReplacedImportPrims), plus any extra prims.
            retired = [str(p) for p in getattr(unreal, class_path.rsplit(".", 1)[1]).get_replaced_import_prims()] + list(extra)
        at = room[3] if len(room) > 3 else (0.0, 0.0, 0.0)
        actor = eas.spawn_actor_from_class(cls, unreal.Vector(*at), unreal.Rotator(0, 0, 0))
        for prop, material in MATERIAL_OVERRIDES.get(class_path, {}).items():
            asset = unreal.load_asset(f"{P.MATERIALS}/{material}")
            if asset:
                actor.set_editor_property(prop, asset)
        actor.set_actor_label(f"{wing} (native)" if not at[0] else f"{wing} exterior (native)")
        actor.tags = [unreal.Name("musee.building"), unreal.Name(f"musee.wing:{wing}"), unreal.Name(NATIVE_TAG)]
        placed.append(actor)
        missing = []
        for path in retired:
            found = [a for p, actors in by_prim.items() if p == path or p.startswith(path + "/") for a in actors]
            if not found:
                missing.append(path)
            for a in found:
                relight.retire(a)
        log(f"{wing}: placed, {len(retired) - len(missing)} imported pieces retired"
            + (f"; not found: {missing}" if missing else ""))
    return placed


def light_reserve(eas, reserve, racks=None):
    for a in eas.get_all_level_actors():
        if PENDANT_TAG in [str(t) for t in a.tags]:
            eas.destroy_actor(a)
    positions = reserve.get_pendant_positions() if reserve else []
    for i, at in enumerate(positions):
        nave = abs(at.y) < 100.0
        light = eas.spawn_actor_from_class(unreal.PointLight, at, unreal.Rotator(0, 0, 0))
        c = light.point_light_component
        c.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
        c.set_editor_property("intensity", PENDANT_LUMENS if nave else AISLE_PENDANT_LUMENS)
        c.set_editor_property("use_temperature", True)
        c.set_editor_property("temperature", PENDANT_KELVIN)
        radius = 22.0 if nave else 17.0   # AReserveStructure::NaveGlobeRadius, AisleGlobeRadius (cm)
        c.set_editor_property("source_radius", PENDANT_SOURCE * radius)
        c.set_editor_property("soft_source_radius", 0.0)
        c.set_editor_property("cast_shadows", True)
        c.set_editor_property("attenuation_radius", 1600.0 if nave else 1100.0)
        c.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
        light.set_actor_label(f"Reserve pendant {i + 1}")
        light.tags = [unreal.Name(PENDANT_TAG), unreal.Name("musee.building"), unreal.Name("musee.wing:Reserve"),
                      unreal.Name(NATIVE_TAG)]
    uplights = unreal.ReserveStructure.get_uplight_transforms() if reserve else []
    xf = reserve.get_actor_transform() if reserve else None
    for k, t in enumerate(uplights):
        t = unreal.MathLibrary.compose_transforms(t, xf)
        light = eas.spawn_actor_from_class(unreal.SpotLight, t.translation, t.rotation.rotator())
        c = light.spot_light_component
        c.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
        c.set_editor_property("intensity", UPLIGHT_LUMENS)
        c.set_editor_property("use_temperature", True)
        c.set_editor_property("temperature", UPLIGHT_KELVIN)
        c.set_editor_property("inner_cone_angle", 15.0)
        c.set_editor_property("outer_cone_angle", 62.0)
        c.set_editor_property("source_radius", UPLIGHT_SOURCE_CM)
        c.set_editor_property("soft_source_radius", 0.0)
        c.set_editor_property("attenuation_radius", 900.0)
        c.set_editor_property("cast_shadows", True)
        c.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
        loc = t.translation
        light.set_actor_label(f"Reserve vault uplight {loc.x / 100:.1f} {loc.y / 100:+.1f} {k % 4}")
        light.tags = [unreal.Name(PENDANT_TAG), unreal.Name("musee.building"),
                      unreal.Name("musee.wing:Reserve"), unreal.Name(NATIVE_TAG)]
    # The export's aisle spots the pendants replace, its spot down the stair (the stair lights itself), and the
    # easel's spot placed on the easel's painting.
    removed = moved = 0
    easel_target = unreal.ReserveRacks.easel_placement(EASEL_WORK) if racks else None
    for a in eas.get_all_level_actors():
        if relight.tag_value(a, "musee.wing:") != "Reserve" or not a.get_components_by_class(unreal.SpotLightComponent):
            continue
        tags = [str(t) for t in a.tags]
        if NATIVE_TAG in tags:
            continue
        loc = a.get_actor_location()
        x, y, h = loc.x / 100.0, loc.y / 100.0, loc.z / 100.0
        stair_spot = abs(x - STAIR_SPOT[0]) < 0.5 and abs(y - STAIR_SPOT[1]) < 0.5 and abs(h - STAIR_SPOT[2]) < 0.5
        if stair_spot or (abs(y) < 0.5 and abs(h - (RESERVE_FLOOR + 3.6)) < 0.5 and any(abs(x - s) < 0.6 for s in RESERVE_AISLE_SPOTS_X)):
            eas.destroy_actor(a)
            removed += 1
        elif "part:easel_light" in tags and easel_target is not None:
            new = unreal.Vector(*(v * 100.0 for v in EASEL_SPOT_AT))
            a.set_actor_location(new, False, False)
            a.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(new, easel_target), False)
            c = a.get_components_by_class(unreal.SpotLightComponent)[0]
            c.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
            c.set_editor_property("intensity", EASEL_SPOT_LUMENS)
            c.set_editor_property("use_temperature", True)
            c.set_editor_property("temperature", PENDANT_KELVIN)
            c.set_editor_property("inner_cone_angle", 11.0)
            c.set_editor_property("outer_cone_angle", 19.0)
            c.set_editor_property("attenuation_radius", 1500.0)
            moved += 1
    lo, hi = RESERVE_BOX_CM
    centre = unreal.Vector((lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, (lo[2] + hi[2]) / 2)
    ppv = eas.spawn_actor_from_class(unreal.PostProcessVolume, centre, unreal.Rotator(0, 0, 0))
    ppv.set_actor_label("Reserve white balance")
    ppv.tags = [unreal.Name(PENDANT_TAG), unreal.Name(NATIVE_TAG), unreal.Name("musee.exposure_zone")]
    ppv.set_editor_property("unbound", False)
    ppv.set_editor_property("priority", 20.0)
    ppv.set_editor_property("blend_radius", 150.0)
    ppv.set_actor_scale3d(unreal.Vector((hi[0] - lo[0]) / 200.0, (hi[1] - lo[1]) / 200.0, (hi[2] - lo[2]) / 200.0))
    st = ppv.settings
    st.set_editor_property("override_white_temp", True)
    st.set_editor_property("white_temp", RESERVE_WHITE_K)
    ppv.set_editor_property("settings", st)
    log(f"Reserve: {len(positions)} pendants lit ({PENDANT_LUMENS:.0f} / {AISLE_PENDANT_LUMENS:.0f} lm, {PENDANT_KELVIN:.0f} K), "
        f"{len(uplights)} shadowing vault uplights, {removed} export spots removed, {moved} easel spot placed")


def main():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    make_reserve_materials()
    les.load_level(P.MAP_PATH)
    placed = place_rooms(eas)
    reserve = next((a for a in placed if a.get_class().get_name() == "ReserveStructure"), None)
    racks = next((a for a in placed if a.get_class().get_name() == "ReserveRacks"), None)
    if racks:
        log(f"Reserve: {racks.arrange_imported()} imported pieces arranged for the racks")
    light_reserve(eas, reserve, racks)
    dress_elan(eas)
    remove_old_strata_pit(eas)
    retire_square(eas)   # Élan Cube
    try:   # Albion: the Chinese Wing's native actor out, the Kelmscott Chaucer and the hang in
        importlib.import_module("albion_place").place(eas)
    except Exception as e:  # noqa: BLE001 - the rest of the map is placed either way
        unreal.log_warning(f"[native] albion_place.place: {e}")
    try:   # Salon interior: the 1874 case, the willows round the oval, the pond's kerb (and what they replace)
        importlib.import_module("salon_interior").place(eas)
    except Exception as e:  # noqa: BLE001 - the rest of the map is placed either way
        unreal.log_warning(f"[native] salon_interior.place: {e}")
    try:   # Chenghuai: the Sculpture Hall's native actor and lights out, the works, mounts and plants in
        importlib.import_module("chenghuai").native_place(eas)
    except Exception as e:  # noqa: BLE001 - the rest of the map is placed either way
        unreal.log_warning(f"[native] chenghuai.native_place: {e}")
    les.save_current_level()


if __name__ == "__main__":
    main()
