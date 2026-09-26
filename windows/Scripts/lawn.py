"""
The grounds' mown lawn (Source/MuseeVision/Nature/MuseeLawn.*): real blades of grass over the site's ground; and the
grounds' clipped hedges (Source/MuseeVision/Nature/MuseeHedge.*, planted by AMuseeLandscape): real box and yew leaves.

    UnrealEditor-Cmd.exe MuseeVision.uproject -run=pythonscript -script="<repo>/windows/Scripts/apply_all.py lawn[:PARTS]"

PARTS (comma-separated; all four by default):
    material   M_NatureLawn and MI_Lawn_Blade (nature.make_lawn_material), and M_Lawn's tone under the blades
    meshes     the Nanite patches of blades in /Game/Museum/Nature/Lawn (UMuseeLawnLibrary.build_lawn_meshes)
    hedges     M_NatureHedge and its instances (nature.make_hedge_materials), the Nanite leaf modules in
               /Game/Museum/Nature/Hedge (UMuseeLawnLibrary.build_hedge_meshes); the Hall of Light's imported plain
               hedge boxes hidden (the landscape grows yew there); the landscape's hedge cores rebuilt (rebaked if baked)
    survey     the AMuseeLawn actor at the origin (placed if missing) surveys where the lawn is: box overlaps against
               everything the visitor collides with, so run it after anything on the grounds moves (after hedges)
    plants     (only when named) the grounds' bamboo regrown at the landscape's leaf density

It changes no native room but the landscape's hedge cores, so a baked map stays baked (the lawn and the hedges' leaves
are instanced and never baked: musee.nobake).
"""
import os
import re
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import musee_paths as P  # noqa: E402

LAWN_TAG = "musee.lawn"
# M_Lawn under the blades: the soil and thatch down in the sward, darker and greener than the photograph's mean, so the
# ground between the blades reads as the sward's depth; far off, where no blades grow, it carries the lawn alone
# (materials.py NAMED["M_Lawn"] holds the same value).
LAWN_GROUND = (0.05, 0.072, 0.026)
# The survey's clearances (m): see AMuseeLawn.Margin and HedgeMargin.
MARGIN = 0.025
HEDGE_MARGIN = 0.06
HALL_OF_LIGHT_HEDGES = re.compile(r"^/Museum/HallOfLight/Hedges(_\d+)?$")


def log(msg):
    unreal.log(f"[lawn] {msg}")


def ground_tone():
    path = f"{P.MATERIALS}/M_Lawn"
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log_warning(f"[lawn] {path} is missing (materials.py makes it)")
        return
    mi = unreal.load_asset(path)
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(mi, "BaseColor", unreal.LinearColor(*LAWN_GROUND, 1.0))
    unreal.MaterialEditingLibrary.update_material_instance(mi)
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    log(f"M_Lawn's tone {LAWN_GROUND}")


def find_or_place():
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for a in eas.get_all_level_actors():
        if isinstance(a, unreal.MuseeLawn):
            return a
    a = eas.spawn_actor_from_class(unreal.MuseeLawn, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    a.set_actor_label("Grounds_Lawn")
    a.set_folder_path("Exterior")
    log("placed the lawn")
    return a


def hedges():
    """The hedges' materials and leaf modules; the Hall of Light's plain boxes hidden; the landscape's cores rebuilt."""
    import nature
    nature.make_hedge_materials()
    n = unreal.MuseeLawnLibrary.build_hedge_meshes()
    log(f"hedge modules: {n} packages saved")
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    hidden = 0
    for a in eas.get_all_level_actors():
        prim = nature.tag_value(a, "prim:")
        if prim and HALL_OF_LIGHT_HEDGES.match(prim):
            nature.hide_stand_in(a)
            hidden += 1
    log(f"the Hall of Light's imported hedges: {hidden} actors hidden")
    for a in eas.get_all_level_actors():
        if not isinstance(a, unreal.MuseeLandscape):
            continue
        baked = unreal.MuseeBakeLibrary.is_baked(a)
        if baked:
            unreal.MuseeBakeLibrary.unbake_actor(a)
        # (Setting a property reruns the construction: the new cores.)
        a.set_editor_property("preview_hedges_in_editor", True)
        if baked:
            unreal.MuseeBakeLibrary.bake_actor(a, "")
        log(f"{a.get_actor_label()}: hedge cores rebuilt{' and rebaked' if baked else ''}, {a.get_triangle_count():,} triangles")


def plants():
    """The grounds' bamboo regrown with the landscape's leaf density (AMuseeLandscape.get_tree_placements), without
    re-placing every plant (nature.py does that)."""
    import nature
    wanted = {t.name: t for t in unreal.MuseeLandscape.get_tree_placements() if t.name.startswith("Grounds_Bamboo")}
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    n = 0
    for a in eas.get_all_level_actors():
        t = wanted.get(nature.tag_value(a, "nature:") or "")
        if t is None:
            continue
        baked = unreal.MuseeBakeLibrary.is_baked(a)
        if baked:
            unreal.MuseeBakeLibrary.unbake_actor(a)
        a.set_editor_property("leaf_density", t.leaf_density)
        a.regrow()
        if baked:
            unreal.MuseeBakeLibrary.bake_actor(a, "")
        n += 1
        log(f"{t.name}: leaf density {t.leaf_density:.2f}, {a.get_editor_property('triangle_count'):,} triangles")
    log(f"{n} bamboo clumps regrown")


def main(args=None, levels=None):
    parts = set(args or []) or {"material", "meshes", "hedges", "survey"}
    if "plants" in parts:
        plants()
        if levels is not None:
            levels.save_current_level()
    if "material" in parts:
        import nature
        nature.make_lawn_material()
        ground_tone()
    if "meshes" in parts:
        n = unreal.MuseeLawnLibrary.build_lawn_meshes()
        log(f"{n} packages saved")
    if "hedges" in parts:
        hedges()
        if levels is not None:
            levels.save_current_level()
    if "survey" in parts:
        lawn = find_or_place()
        tags = [str(t) for t in lawn.get_editor_property("tags")]
        for t in ("musee.nobake", "musee.exterior", LAWN_TAG):
            if t not in tags:
                tags.append(t)
        lawn.set_editor_property("tags", [unreal.Name(t) for t in tags])
        lawn.set_editor_property("margin", MARGIN)
        lawn.set_editor_property("hedge_margin", HEDGE_MARGIN)
        n = lawn.survey()
        log(f"survey: {n} patches; {lawn.describe()}")
        if levels is not None:
            levels.save_current_level()


if __name__ == "__main__":
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.load_level(P.MAP_PATH)
    main(sys.argv[1:], les)
