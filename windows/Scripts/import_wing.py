"""
Import one or more wings of usd/ into the Museum map, as ordinary Unreal assets (WINDOWS.md).

    UnrealEditor-Cmd.exe MuseeVision.uproject -run=pythonscript -script="Scripts/import_wing.py Rotunda"

For each wing it writes a small wrapper layer in Import/ (the wing's layer plus only the materials it
uses), imports it with Interchange into /Game/Museum/<Wing> and the level, then:

- tags every actor from its USD prim: "musee.building", "musee.wing:<Wing>", "prim:<path>",
  "part:<movingPart>" (museevision:movingPart) and "work:<id>" (a work for the placards);
- hides what the app disables at the start (museevision:enabledAtStart = 0, or invisible);
- turns on Nanite for opaque meshes and uses the mesh itself for collision (solid walls, stairs);
- drops the imported sun (the Sky actor keeps the Rotunda's sun clock).

Re-running a wing replaces what it imported before. Run Scripts/setup_project.py first.
"""
import os
import subprocess
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import musee_paths as P  # noqa: E402

from pxr import Sdf, Usd, UsdGeom, UsdShade  # noqa: E402

# Prims of the Élan layer that predate the Sphere of R 14 m: the old rib vault, its misty glass and
# the iris at 26.9 m. AElanStructure builds the drum, the Sphere, the ribs and the iris natively.
ELAN_SUPERSEDED = ["/Museum/Elan/Misty_glass", "/Museum/Elan/Atrium_ribs", "/Museum/Elan/Iris"]


def log(msg):
    unreal.log(f"[import_wing] {msg}")


# --------------------------------------------------------------------------- the wrapper layer

def materials_bound(stage, wing_path):
    """Paths of the materials bound anywhere under the wing (and by its scans)."""
    used = set()
    for prim in Usd.PrimRange(stage.GetPrimAtPath(wing_path)):
        rel = prim.GetRelationship("material:binding")
        if rel:
            for t in rel.GetTargets():
                used.add(t)
    return used


def readable_image(path):
    """The image itself, or a PNG re-encoding of it if Unreal's JPEG decoder rejects it."""
    if os.path.basename(path) not in P.BAD_JPEGS:
        return path
    out_dir = os.path.join(P.IMPORT_DIR, "images")
    os.makedirs(out_dir, exist_ok=True)
    out = os.path.join(out_dir, os.path.splitext(os.path.basename(path))[0] + ".png")
    if not os.path.exists(out):
        # Windows' own decoder reads these; save them losslessly.
        script = ("Add-Type -AssemblyName System.Drawing; "
                  f"$i = [System.Drawing.Image]::FromFile('{path}'); $i.Save('{out}', [System.Drawing.Imaging.ImageFormat]::Png); $i.Dispose()")
        subprocess.run(["powershell", "-NoProfile", "-NonInteractive", "-Command", script], check=True)
        log(f"re-encoded {os.path.basename(path)} as PNG")
    return out


def absolutize_assets(layer, anchor_dir):
    """Make every asset path in the layer absolute (the specs move out of usd/)."""
    def fix(path):
        spec = layer.GetObjectAtPath(path)
        if isinstance(spec, Sdf.AttributeSpec) and spec.typeName == Sdf.ValueTypeNames.Asset and spec.default:
            p = spec.default.path
            if p and not os.path.isabs(p):
                full = readable_image(os.path.normpath(os.path.join(anchor_dir, p)))
                spec.default = Sdf.AssetPath(full.replace("\\", "/"))
    layer.Traverse(Sdf.Path.absoluteRootPath, fix)


def write_wrapper(wing):
    """Import/<Wing>.usda: the wing layer plus Import/<Wing>_materials.usda, with /Museum defined."""
    os.makedirs(P.IMPORT_DIR, exist_ok=True)
    full = Usd.Stage.Open(os.path.join(P.USD_DIR, "museum.usda"))
    wing_path = f"/Museum/{wing}"
    used = materials_bound(full, wing_path)

    src = Sdf.Layer.FindOrOpen(os.path.join(P.USD_DIR, "Materials.usda"))
    mats = Sdf.Layer.CreateNew(os.path.join(P.IMPORT_DIR, f"{wing}_materials.usda"))
    mats.defaultPrim = "Museum"
    Sdf.CreatePrimInLayer(mats, "/Museum/Materials").specifier = Sdf.SpecifierOver
    mats.GetPrimAtPath("/Museum").specifier = Sdf.SpecifierOver
    mats.GetPrimAtPath("/Museum/Materials").specifier = Sdf.SpecifierDef
    mats.GetPrimAtPath("/Museum/Materials").typeName = "Scope"
    for m in sorted(used, key=str):
        if src.GetPrimAtPath(m):
            Sdf.CopySpec(src, m, mats, m)
    absolutize_assets(mats, P.USD_DIR)
    mats.Save()

    layers = [os.path.join(P.USD_DIR, "wings", f"{wing}.usdc")]
    # The scans that stand in this wing, copied from usd/sculptures/Sculptures.usda as they are.
    scans = scan_prims(full, wing_path)
    if scans:
        layers.append(write_scans(wing, scans))
    layers.append(os.path.join(P.IMPORT_DIR, f"{wing}_materials.usda"))
    wrapper = os.path.join(P.IMPORT_DIR, f"{wing}.usda")
    rel = lambda p: os.path.relpath(p, P.IMPORT_DIR).replace("\\", "/")  # noqa: E731
    sub = ", ".join(f"@{rel(l)}@" for l in layers)
    body = [
        "#usda 1.0",
        "(",
        '    defaultPrim = "Museum"',
        "    metersPerUnit = 1",
        '    upAxis = "Y"',
        f"    subLayers = [{sub}]",
        ")",
        "",
        'def Xform "Museum" (',
        '    kind = "assembly"',
        ")",
        "{",
        "}",
    ]
    open(wrapper, "w", encoding="utf-8").write("\n".join(body) + "\n")
    return wrapper, used


def scan_prims(stage, wing_path):
    return [p for p in Usd.PrimRange(stage.GetPrimAtPath(wing_path))
            if p.GetAttribute("museevision:scanFile") and p.GetAttribute("museevision:scanFile").HasValue()]


def write_scans(wing, scans):
    """Import/<Wing>_scans.usda: the wing's scan prims, with their geometry references made absolute."""
    src_path = os.path.join(P.USD_DIR, "sculptures", "Sculptures.usda")
    src = Sdf.Layer.FindOrOpen(src_path)
    out_path = os.path.join(P.IMPORT_DIR, f"{wing}_scans.usda")
    out = Sdf.Layer.CreateNew(out_path)
    out.defaultPrim = "Museum"
    for prim in scans:
        path = prim.GetPath()
        if not src.GetPrimAtPath(path):
            continue
        # Parents as overs (/Museum is defined by the wrapper), then the scan's own spec.
        Sdf.CreatePrimInLayer(out, path.GetParentPath())
        Sdf.CopySpec(src, path, out, path)
        spec = out.GetPrimAtPath(path)
        items = [Sdf.Reference(os.path.normpath(os.path.join(os.path.dirname(src_path), r.assetPath)).replace("\\", "/"),
                               r.primPath, r.layerOffset) for r in spec.referenceList.prependedItems]
        spec.referenceList.prependedItems = items
    out.Save()
    return out_path


# --------------------------------------------------------------------------- the import

def prim_path_of(actor, root_label):
    """/Museum/… from the actor's attach chain (labels are the prim names)."""
    names = []
    a = actor
    while a is not None:
        names.append(a.get_actor_label())
        a = a.get_attach_parent_actor()
    names.reverse()
    # The chain starts at the import's root actor (named after the file), then Museum/….
    if names and names[0] == root_label and len(names) > 1 and names[1] == "Museum":
        names = names[1:]
    return "/" + "/".join(names)


def remove_previous(wing):
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    tag = f"musee.wing:{wing}"
    old = [a for a in eas.get_all_level_actors() if tag in [str(t) for t in a.tags]]
    for a in old:
        eas.destroy_actor(a)
    if old:
        log(f"{wing}: removed {len(old)} actors from the previous import")


def is_translucent(stage, material_path):
    prim = stage.GetPrimAtPath(material_path)
    if not prim:
        return False
    for p in Usd.PrimRange(prim):
        a = p.GetAttribute("inputs:opacity")
        if a and a.HasValue() and a.Get() is not None and a.Get() < 0.999:
            return True
    return False


def material_is_translucent(material, translucent_usd):
    """From Unreal's own blend mode, else from the USD material (Interchange may add a digit)."""
    try:
        return material.get_blend_mode() not in (unreal.BlendMode.BLEND_OPAQUE, unreal.BlendMode.BLEND_MASKED)
    except Exception:
        name = material.get_name().rstrip("0123456789")
        return f"/Museum/Materials/{material.get_name()}" in translucent_usd or f"/Museum/Materials/{name}" in translucent_usd


def import_wing(wing):
    wrapper, used = write_wrapper(wing)
    stage = Usd.Stage.Open(wrapper)
    content = f"{P.MUSEUM_CONTENT}/{wing}"
    remove_previous(wing)

    mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
    params = unreal.ImportAssetParameters()
    params.is_automated = True
    params.replace_existing = True
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    before = set(a.get_path_name() for a in eas.get_all_level_actors())
    ok = mgr.import_scene(content, unreal.InterchangeManager.create_source_data(wrapper), params)
    mgr.wait_until_all_tasks_done(False)
    if not ok:
        raise RuntimeError(f"{wing}: Interchange import failed")
    new = [a for a in eas.get_all_level_actors() if a.get_path_name() not in before]
    log(f"{wing}: {len(new)} actors imported into {content}")

    root_label = os.path.splitext(os.path.basename(wrapper))[0]
    translucent = {str(m) for m in used if is_translucent(stage, m)}
    meshes = {}
    hidden = 0
    removed = 0
    for actor in new:
        path = prim_path_of(actor, root_label)
        prim = stage.GetPrimAtPath(path)
        tags = [unreal.Name("musee.building"), unreal.Name(f"musee.wing:{wing}"), unreal.Name(f"prim:{path}")]
        if prim:
            part = prim.GetAttribute("museevision:movingPart")
            if part and part.HasValue():
                tags.append(unreal.Name(f"part:{part.Get()}"))
            work = prim.GetAttribute("museevision:workId")
            entity = prim.GetAttribute("museevision:entity")
            if work and work.HasValue():
                tags.append(unreal.Name(f"work:{work.Get()}"))
            elif entity and entity.HasValue() and "-" in entity.Get() and " " not in entity.Get():
                # Paintings, photographs, ceramics: the entity is the catalogue id.
                tags.append(unreal.Name(f"work:{entity.Get()}"))
        actor.tags = tags

        # The imported sun: the Sky actor keeps the Rotunda's sun clock instead.
        if isinstance(actor, unreal.DirectionalLight):
            eas.destroy_actor(actor)
            removed += 1
            continue
        if wing == "Elan" and path in ELAN_SUPERSEDED:
            eas.destroy_actor(actor)
            removed += 1
            continue

        enabled = True
        p = prim
        while p and p.IsValid() and str(p.GetPath()) != "/":
            en = p.GetAttribute("museevision:enabledAtStart")
            if en and en.HasValue() and not en.Get():
                enabled = False
            if p.IsA(UsdGeom.Imageable) and UsdGeom.Imageable(p).GetVisibilityAttr().Get() == UsdGeom.Tokens.invisible:
                enabled = False
            p = p.GetParent()
        if not enabled:
            actor.set_actor_hidden_in_game(True)
            actor.set_actor_enable_collision(False)
            actor.set_is_temporarily_hidden_in_editor(True)
            hidden += 1

        for comp in actor.get_components_by_class(unreal.StaticMeshComponent):
            sm = comp.static_mesh
            if sm:
                glassy = any(material_is_translucent(m, translucent) for m in comp.get_materials() if m)
                meshes[sm.get_path_name()] = (sm, glassy)
                if glassy:
                    # Glass stays solid to walk into but lets the look trace through to what is behind.
                    comp.set_collision_response_to_channel(unreal.CollisionChannel.ECC_VISIBILITY, unreal.CollisionResponseType.ECR_IGNORE)

        # Élan's Atrium lights stay unshadowed, as on the iPhone, so the Sphere over them doesn't
        # change the floor's light.
        if wing == "Elan" and isinstance(actor, unreal.SpotLight) and path.startswith("/Museum/Elan/SpotLight"):
            loc = actor.get_actor_location()
            if loc.z > 1500:
                actor.spot_light_component.set_cast_shadows(False)

    for path, (sm, glassy) in meshes.items():
        ns = sm.get_editor_property("nanite_settings")
        ns.enabled = not glassy
        sm.set_editor_property("nanite_settings", ns)
        body = sm.get_editor_property("body_setup")
        if body:
            body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        unreal.EditorAssetLibrary.save_loaded_asset(sm, only_if_is_dirty=False)
    log(f"{wing}: {len(meshes)} meshes ({sum(1 for _, g in meshes.values() if g)} glass), {hidden} hidden at start, {removed} removed")

    if wing == "Elan":
        place_elan_structure()
    # Lumen does the daylight: drop the phone's stand-ins, rebuild the lamps, light the skylights.
    import relight
    relight.relight([wing])


def place_elan_structure():
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    structure = unreal.load_class(None, "/Script/MuseeVision.ElanStructure")
    elevator = unreal.load_class(None, "/Script/MuseeVision.ElanElevator")
    for a in eas.get_all_level_actors():
        if a.get_class() in (structure, elevator):
            eas.destroy_actor(a)
    a = eas.spawn_actor_from_class(structure, unreal.Vector(5400, 0, 0), unreal.Rotator(0, 0, 0))
    a.set_actor_label("Elan_drum_and_Sphere")
    a.tags = [unreal.Name("musee.building"), unreal.Name("musee.wing:Elan"), unreal.Name("prim:/Museum/Elan/Drum_and_Sphere")]
    # The elevator is not part of the building: it stays when the building is hidden in the Sphere.
    e = eas.spawn_actor_from_class(elevator, unreal.Vector(5400, 0, 0), unreal.Rotator(0, 0, 0))
    e.set_actor_label("Elan_elevator_controller")
    log("Elan: placed the drum, the Sphere and the ribs (R 14 m, centre 22 m up), and the elevator")


def main(wings):
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not unreal.EditorAssetLibrary.does_asset_exist(P.MAP_PATH):
        raise RuntimeError(f"{P.MAP_PATH} is missing: run Scripts/setup_project.py first")
    les.load_level(P.MAP_PATH)
    for wing in wings:
        if wing not in P.WINGS:
            raise ValueError(f"unknown wing {wing}; one of {P.WINGS}")
        import_wing(wing)
        # Save as we go: a failure in a later wing keeps the ones before it.
        les.save_current_level()
    unreal.EditorAssetLibrary.save_directory(P.MUSEUM_CONTENT, only_if_is_dirty=True, recursive=True)
    log("saved")


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    main(P.WINGS if args == ["all"] else (args or ["Rotunda"]))
