"""
Relight the imported museum for Lumen (run after import_wing.py; import_wing.py calls it per wing):

    UnrealEditor-Cmd.exe MuseeVision.uproject -run=pythonscript -script="Scripts/relight.py"

On the iPhone, spot lights stood in for daylight: under the eye, the skylights, the laylights and
the glass hall. Here the real sun and sky come in through those openings, and the skylights,
laylights and velarium glow with the sky (M_Daylit, following "Daylight" in MPC_Musee), so those
spot lights go. The rest are lamps (the Nymphéas oval, the Reserve, the cloister, the Élan shaft
and the Square's ring, the Atrium's haze): they are rebuilt from their USD attributes (lumens,
range, shadows) as soft area lights instead of the importer's 5 cm points with a 10 m range.
Glass gets a thin, clear M_Glass; the display cases lose their glass; the stereo stones show one
view each.
"""
import math
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import musee_paths as P  # noqa: E402
import setup_project as S  # noqa: E402

from pxr import Usd  # noqa: E402

# The iPhone's stand-ins for daylight; Lumen brings the real thing through these openings.
DAYLIGHT_STAND_INS = {
    "/Museum/Rotunda/SpotLight",                                    # under the eye
    "/Museum/Salon/SpotLight", "/Museum/Salon/SpotLight_2", "/Museum/Salon/SpotLight_3",
    "/Museum/Salon/SpotLight_4", "/Museum/Salon/SpotLight_5",       # under the five skylights
    "/Museum/Salon/SpotLight_7",                                    # the Manet cabinet's laylight
    "/Museum/SculptureHall/SpotLight",                              # the laylight
    "/Museum/HallOfLight/SpotLight", "/Museum/HallOfLight/SpotLight_2",   # the glass hall
    "/Museum/ChineseWing/SpotLight_4",                              # over the garden
}

# The export's lumens are the phone's (tuned for RealityKit): this brings them to real lamps
# (the Reserve's 18,000 → about 1,100 lm).
LUMENS_SCALE = 0.06
SOURCE_RADIUS_CM = 15.0

# Sky-lit materials of the export → their native instances (setup_project.py).
DAYLIT = {
    "light_grid_FFFFFF": "MI_light_grid_FFFFFF",
    "light_grid_F2EEE6": "MI_light_grid_F2EEE6",
    "light_grid_FFF3DA": "MI_light_grid_FFF3DA",
    "velarium": "MI_velarium",
}

# The same panels by prim (so a mesh whose material went missing is still found).
DAYLIT_PRIMS = {
    "/Museum/Salon/Light_grid_skylights": "MI_light_grid_FFFFFF",
    "/Museum/Salon/Cabinet_laylight": "MI_light_grid_F2EEE6",
    "/Museum/SculptureHall/Sculpture_laylight": "MI_light_grid_FFF3DA",
    "/Museum/Salon/Velarium": "MI_velarium",
}


def log(msg):
    unreal.log(f"[relight] {msg}")


# The stereo stones carry both views of each stereograph in the same place (on the iPhone they
# alternate); here they would fight over the same pixels, so the right-eye views are hidden.
STEREO_RIGHT_VIEWS = {f"/Museum/HallOfLight/stereo_{i}/Canvas_2" for i in range(1, 5)}

# The Sculpture Hall stands empty for now: its ten scans (Rodin and his time) were not good enough to show.
# Their plinths go with them (ASculptureHallStructure), and so their accent lights (gallery_lights.py skips
# retired works).
EMPTY_ROOM = "/Museum/SculptureHall/"

# The Thousand Li handscroll lies fully open. On the iPhone a veil covered what was not yet unrolled
# and followed the visitor; here it would sit frozen 5 mm over the silk and flicker against it. So the
# veils go and the two rollers sit at the scroll's ends (plan y 17.54 north, 29.46 south, in cm below).
HANDSCROLL_VEILS = {"/Museum/ChineseWing/Veil_north", "/Museum/ChineseWing/Veil_south"}
HANDSCROLL_ROLLERS = {"/Museum/ChineseWing/Roller_north": 1754.0 - 4.0, "/Museum/ChineseWing/Roller_south": 2946.0 + 4.0}

# No protective glass round the works (sculptures, ceramics): the cases' glass goes. The glass
# steles that carry paintings (glass_stele) stay.
CASE_GLASS = ("glass_case", "glass_vitrine")
# And what held the glass: the bronze rings that capped the ceramics' cases in the Chinese Wing.
CASE_PARTS = ("/Museum/ChineseWing/Case_ring", "/Museum/ChineseWing/Cup_ring")   # the cup's ring read as a black line on the plinth


def retire(actor):
    """
    Take an imported piece out of the museum for good: hidden in game and in the editor, no collision,
    and off the musee.building list, so leaving the Sphere (which shows the building again) leaves it out.
    """
    actor.set_actor_hidden_in_game(True)
    actor.set_actor_enable_collision(False)
    actor.set_is_temporarily_hidden_in_editor(True)
    tags = [t for t in actor.tags if str(t) != "musee.building"]
    if "musee.retired" not in [str(t) for t in tags]:
        tags.append(unreal.Name("musee.retired"))
    actor.tags = tags


def repair_canvases(eas, wings=None):
    """
    A painting whose image Unreal's decoder rejected (musee_paths.BAD_JPEGS, re-encoded to PNG on import)
    can keep the importer's white placeholder as its colour: point it at its imported texture
    (painting_<name> → <name-with-hyphens> in the wing's Textures).
    """
    L = unreal.MaterialEditingLibrary
    fixed = []
    for actor in eas.get_all_level_actors():
        wing = tag_value(actor, "musee.wing:")
        # Every painting_ material, whatever its prim (…/Canvas, the handscroll's …/Canvas_2 to _5, the Nymphéas
        # oval's four panels …/clouds, …/morning …): the handscroll lost two images and Clouds its own.
        if wing is None or (wings and wing not in wings):
            continue
        for comp in actor.get_components_by_class(unreal.StaticMeshComponent):
            for m in comp.get_materials():
                if not isinstance(m, unreal.MaterialInstanceConstant) or not m.get_name().startswith("painting_"):
                    continue
                if m.get_name().startswith("painting_stereo_"):
                    # The stereo readers: the whole card, centred (stereo_cards.py), not the plain card's
                    # left half stretched across the reader.
                    import materials as MAT
                    card = MAT.texture_asset("stereo_" + m.get_name()[len("painting_stereo_"):])
                    if card and L.get_material_instance_texture_parameter_value(m, "BaseColorTexture") != card:
                        L.set_material_instance_texture_parameter_value(m, "BaseColorTexture", card)
                        unreal.EditorAssetLibrary.save_loaded_asset(m)
                        fixed.append(m.get_name())
                    continue
                tex = L.get_material_instance_texture_parameter_value(m, "BaseColorTexture")
                if tex and tex.get_name() != "WhiteSquareTexture":
                    continue
                name = m.get_name()[len("painting_"):].replace("_", "-")
                image = unreal.load_asset(f"{P.MUSEUM_CONTENT}/{wing}/{wing}/Textures/{name}")
                if image:
                    L.set_material_instance_texture_parameter_value(m, "BaseColorTexture", image)
                    unreal.EditorAssetLibrary.save_loaded_asset(m)
                    fixed.append(name)
    if fixed:
        log(f"paintings re-linked to their images: {', '.join(sorted(set(fixed)))}")


def is_case_glass(actor):
    comps = actor.get_components_by_class(unreal.StaticMeshComponent)
    names = [m.get_name() for c in comps for m in c.get_materials() if m]
    return bool(names) and all(n.startswith(CASE_GLASS) for n in names)


def glass_instances(stage):
    """
    MI_<glass> for every glass of the export: M_Glass (thin, clear, a little tinted, reflecting
    like real glass rather than a mirror) with the export's tint and opacity.
    """
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mel = unreal.MaterialEditingLibrary
    parent = unreal.load_asset(f"{P.MATERIALS}/M_Glass")
    out = {}
    materials = stage.GetPrimAtPath("/Museum/Materials")
    for mat in (materials.GetChildren() if materials else []):
        name = mat.GetName()
        if not name.startswith("glass_"):
            continue
        shader = mat.GetChild("Surface")
        colour = attr(shader, "inputs:diffuseColor", (0.8, 0.9, 0.9))
        opacity = attr(shader, "inputs:opacity", 0.15)
        path = f"{P.MATERIALS}/MI_{name}"
        mi = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else \
            tools.create_asset(f"MI_{name}", P.MATERIALS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, parent)
        mel.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(colour[0], colour[1], colour[2], 1))
        # The phone's opacity stood in for everything glass does; real glass lets most light through.
        mel.set_material_instance_scalar_parameter_value(mi, "Opacity", min(0.4, float(opacity) * 0.35))
        unreal.EditorAssetLibrary.save_loaded_asset(mi)
        out[name] = mi
    return out


# Roofs over the laylit wings. The phone never needed them; here the sun would slip through the
# gaps round the skylight panels and draw lines on the floor. As in a real museum, daylight reaches
# these rooms only through the laylights (which glow with the sky); the Rotunda's eye and the Hall
# of Light's glass still let the sun in. (x0, x1, y0, y1, z) in plan metres, from the wings' bounds,
# kept clear of the Rotunda's dome.
ROOFS = {
    # (The Salon's roof is the native ASalonStructure's own, open only over its lanterns.)
    # (The Sculpture Hall's roof is ASculptureHallStructure's own hipped glass roof, sealed but for the glass.)
}
ROOF_TAG = "musee.roof"

# The laylights and the velarium light the rooms under them. Their glow (M_Daylit) reaches the room
# only through Lumen's bounce, which falls well short of a real laylight's (the Nymphéas walls came
# out 3.6 stops under the velarium instead of about 1.3), so each gets a rect light of its size
# behind it: candela = luminance × area × fill. MuseeSky scales them with the daylight in play.
# Prim → (luminance in nits as in setup_project.py, colour, fill: the panel's share of its box).
LAYLIGHTS = {
    # (Salon interior: the Nymphéas velarium's light is ASalonSky's now, following Giverny's weather.)
    "/Museum/Salon/Cabinet_laylight": (2000.0, "F2EEE6", 1.0),
    # (Chenghuai: the Sculpture Hall's laylight went with the hall when Chenghuai took the north door.)
}
LAYLIGHT_TAG = "musee.laylight"
# (The Atrium's roof lights are AElanStructure's own rect-light components, under its glazed roof.)
LAYLIGHT_FILL = 0.85   # the panel's own glow still bounces a little
# Where the native Salon (ASalonStructure) moved a panel: its inset from the imported panel's box
# (m, each side) and the height of the light (m). The Nymphéas velarium sits on the cove's rim at
# 6.35 m, 1.45 m in from the wall.
LAYLIGHT_NATIVE = {"/Museum/Salon/Velarium": (1.45, 6.30),
                   # ASculptureHallStructure: just inside the pearl bars under the diffuser (9.90 m).
                   "/Museum/SculptureHall/Sculpture_laylight": (0.05, 9.89)}


def add_laylights(wings=None):
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    S.set_laylight_floor()
    # Only this function's own rect lights: other actors carry the tag too (AElanStructure, whose
    # roof lights MuseeSky dims), and must stay.
    for a in eas.get_all_level_actors():
        if isinstance(a, unreal.RectLight) and LAYLIGHT_TAG in [str(t) for t in a.tags]                 and (not wings or tag_value(a, "musee.wing:") in wings):
            eas.destroy_actor(a)
    made = 0
    for a in eas.get_all_level_actors():
        path = tag_value(a, "prim:") or ""
        wing = tag_value(a, "musee.wing:")
        if path not in LAYLIGHTS or (wings and wing not in wings) or LAYLIGHT_TAG in [str(t) for t in a.tags]:
            continue
        nits, colour, fill = LAYLIGHTS[path]
        origin, extent = a.get_actor_bounds(False)
        width, depth = extent.x * 2, extent.y * 2
        at = unreal.Vector(origin.x, origin.y, origin.z - extent.z - 5.0)
        if path in LAYLIGHT_NATIVE and "musee.retired" in [str(t) for t in a.tags]:
            inset, z = LAYLIGHT_NATIVE[path]
            width, depth = width - 2 * inset * 100.0, depth - 2 * inset * 100.0
            at = unreal.Vector(origin.x, origin.y, z * 100.0)
        light = eas.spawn_actor_from_class(unreal.RectLight, at, unreal.Rotator(0, -90, 0))
        c = light.rect_light_component
        c.set_editor_property("source_width", width)
        c.set_editor_property("source_height", depth)
        c.set_editor_property("barn_door_angle", 88.0)
        c.set_editor_property("barn_door_length", 0.0)
        c.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
        c.set_editor_property("intensity", nits * (width / 100.0) * (depth / 100.0) * fill * LAYLIGHT_FILL)
        c.set_editor_property("light_color", unreal.Color(int(colour[4:6], 16), int(colour[2:4], 16), int(colour[0:2], 16), 255))
        c.set_editor_property("attenuation_radius", 4000.0)
        c.set_editor_property("cast_shadows", True)
        c.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
        light.set_actor_label(f"Laylight {path.rsplit('/', 1)[-1]}")
        light.tags = [unreal.Name(LAYLIGHT_TAG), unreal.Name(f"laylight.night:{S.LAYLIGHT_FLOOR}"), unreal.Name("musee.building"),
                      unreal.Name(f"musee.wing:{wing}")]
        made += 1
    log(f"{made} laylight area lights")


def add_roofs(wings=None):
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for a in eas.get_all_level_actors():
        if ROOF_TAG in [str(t) for t in a.tags] and (not wings or tag_value(a, "musee.wing:") in wings):
            eas.destroy_actor(a)
    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    roof_material = unreal.load_asset(f"{P.MATERIALS}/M_RibSteel")
    for wing, (x0, x1, y0, y1, z) in ROOFS.items():
        if wings and wing not in wings:
            continue
        centre = unreal.Vector((x0 + x1) * 50, (y0 + y1) * 50, (z + 0.2) * 100)
        roof = eas.spawn_actor_from_class(unreal.StaticMeshActor, centre, unreal.Rotator(0, 0, 0))
        roof.set_actor_label(f"{wing}_roof")
        comp = roof.static_mesh_component
        comp.set_static_mesh(cube)
        if roof_material:
            comp.set_material(0, roof_material)
        # The engine cube is 1 m: 0.4 m thick, over the whole wing.
        roof.set_actor_scale3d(unreal.Vector(x1 - x0, y1 - y0, 0.4))
        roof.tags = [unreal.Name(ROOF_TAG), unreal.Name("musee.building"), unreal.Name(f"musee.wing:{wing}")]
    log("roofs over the laylit wings")


def tag_value(actor, prefix):
    for t in actor.tags:
        s = str(t)
        if s.startswith(prefix):
            return s[len(prefix):]
    return None


def attr(prim, name, default=None):
    a = prim.GetAttribute(name) if prim else None
    return a.Get() if a and a.HasValue() else default


def relight(wings=None):
    stage = Usd.Stage.Open(os.path.join(P.USD_DIR, "museum.usda"))
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    daylit = {k: unreal.load_asset(f"{P.MATERIALS}/{v}") for k, v in DAYLIT.items()}
    daylit.update(glass_instances(stage))
    # Meshes already on a native instance (a second run) keep theirs, refreshed.
    daylit.update({mi.get_name(): mi for mi in list(daylit.values()) if mi})
    removed = rebuilt = panels = cases = 0
    for actor in eas.get_all_level_actors():
        wing = tag_value(actor, "musee.wing:")
        if wing is None or (wings and wing not in wings):
            continue
        tags = [str(t) for t in actor.tags]
        if "musee.accent" in tags or LAYLIGHT_TAG in tags or "musee.native" in tags or "musee.retired" in tags:
            continue   # gallery_lights.py's accent spots, the laylights' area lights, native rooms and their lamps
        path = tag_value(actor, "prim:") or ""
        if path in STEREO_RIGHT_VIEWS:
            retire(actor)
            continue
        if path.startswith(EMPTY_ROOM) and tag_value(actor, "work:"):
            retire(actor)   # the Sculpture Hall stands empty for now
            continue

        if path in HANDSCROLL_VEILS:
            retire(actor)
            continue
        roller = next((y for p, y in HANDSCROLL_ROLLERS.items() if path.startswith(p + "/")), None)
        if roller is not None:
            origin, _ = actor.get_actor_bounds(False)
            loc = actor.get_actor_location()
            actor.set_actor_location(unreal.Vector(loc.x, loc.y + (roller - origin.y), loc.z), False, False)
            continue

        if path.startswith(CASE_PARTS):
            retire(actor)
            cases += 1
            continue

        if is_case_glass(actor):
            retire(actor)
            cases += 1
            continue

        lights = actor.get_components_by_class(unreal.LocalLightComponent)
        if lights:
            if path in DAYLIGHT_STAND_INS:
                eas.destroy_actor(actor)
                removed += 1
                continue
            prim = stage.GetPrimAtPath(path)
            lumens = attr(prim, "museevision:lumens")
            for light in lights:
                if lumens:
                    light.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
                    light.set_editor_property("intensity", lumens * LUMENS_SCALE)
                rng = attr(prim, "museevision:attenuationRadius")
                if rng:
                    light.set_editor_property("attenuation_radius", rng * 100.0)
                light.set_editor_property("source_radius", SOURCE_RADIUS_CM)
                light.set_editor_property("soft_source_radius", SOURCE_RADIUS_CM)
                light.set_editor_property("cast_shadows", bool(attr(prim, "museevision:castsShadow", True)))
                if isinstance(light, unreal.SpotLightComponent):
                    inner = attr(prim, "museevision:innerAngleDegrees")
                    outer = attr(prim, "museevision:outerAngleDegrees")
                    if inner and outer:
                        # RealityKit's angles are the full cone; Unreal's are half.
                        light.set_editor_property("inner_cone_angle", inner / 2)
                        light.set_editor_property("outer_cone_angle", outer / 2)
            rebuilt += 1
            continue

        if path in DAYLIT_PRIMS:
            mi = unreal.load_asset(f"{P.MATERIALS}/{DAYLIT_PRIMS[path]}")
            for comp in actor.get_components_by_class(unreal.StaticMeshComponent):
                for i in range(comp.get_num_materials()):
                    comp.set_material(i, mi)
                    panels += 1
            continue

        for comp in actor.get_components_by_class(unreal.StaticMeshComponent):
            for i, m in enumerate(comp.get_materials()):
                if not m:
                    continue
                # Interchange may add a digit to a name ("glass_oculus1"): drop them one at a time.
                name = m.get_name()
                while name not in daylit and name and name[-1].isdigit():
                    name = name[:-1]
                if name in daylit and daylit[name]:
                    comp.set_material(i, daylit[name])
                    panels += 1
    add_roofs(wings)
    add_laylights(wings)
    repair_canvases(eas, wings)
    log(f"removed {removed} daylight stand-ins, rebuilt {rebuilt} lamps, {panels} panels and panes re-materialled, "
        f"{cases} display cases' glass removed")


if __name__ == "__main__":
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.load_level(P.MAP_PATH)
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    relight(args or None)
    les.save_current_level()
