"""
Bake the native procedural architecture into Nanite static meshes (safe to re-run):

    UnrealEditor-Cmd.exe MuseeVision.uproject -run=pythonscript -script="<repo>/windows/Scripts/bake.py [--unbake] [--folder=/Game/Museum/Baked] [--map=/Game/Maps/Museum]"

or, with other steps in one editor session, `apply_all.py ... bake` (see apply_all.py).

The rooms, frames and plants are C++ actors built of procedural mesh components, which get no Nanite,
no mesh distance field and no Lumen surface cards. UMuseeBakeLibrary (Source/MuseeVisionEditor) turns
each procedural component of a bakeable actor into static meshes saved in <folder>/<actor label>/
(Nanite for the opaque sections; glass apart, without Nanite; hidden collision apart, never drawn),
puts static mesh components in its place on the same actor (tagged musee.baked), and empties and
hides the procedural one. Rebaking replaces them; --unbake puts the procedural geometry back.

Never baked: actors that tick or that the visitor uses (the elevator, the pond lift, the sky), plants
that follow today's season, and anything tagged musee.nobake (the Atrium's misty glass and iris, the
Square's light ring). Source/MuseeVision/Geometry/MuseeBake.h has the guard a structure needs so it
doesn't rebuild over its bake.

Edits to a baked actor don't show until it is rebaked: apply_all.py unbakes the map before any step
that changes the native actors and bakes it again after.
"""
import os
import sys
import time

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import musee_paths as P  # noqa: E402

FOLDER = P.MUSEUM_CONTENT + "/Baked"
BAKED_TAG = "musee.baked"


def log(msg):
    unreal.log(f"[bake] {msg}")


def library():
    lib = getattr(unreal, "MuseeBakeLibrary", None)
    if lib is None:
        raise RuntimeError("MuseeBakeLibrary is missing: build the MuseeVisionEditor target first")
    return lib


def baked_actors():
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    lib = getattr(unreal, "MuseeBakeLibrary", None)
    if lib is not None:
        return [a for a in eas.get_all_level_actors() if lib.is_baked(a)]
    # (without the editor module: by the tag alone)
    return [a for a in eas.get_all_level_actors() if BAKED_TAG in [str(t) for t in a.tags]]


def bake(folder=FOLDER):
    """Bake (or rebake) every bakeable actor of the open map. The caller saves the map."""
    start = time.time()
    made = library().bake_all(folder)
    log(f"{len(baked_actors())} actors baked, {made} static mesh components, in {time.time() - start:.0f} s "
        f"(details: LogMuseeBake in the log)")
    return made


def unbake():
    """Put the procedural geometry back on every baked actor of the open map. The caller saves the map."""
    count = library().unbake_all()
    log(f"{count} actors unbaked")
    return count


def main(argv):
    folder = FOLDER
    map_path = P.MAP_PATH
    undo = False
    for a in argv:
        if a.startswith("--folder="):
            folder = a.split("=", 1)[1]
        elif a.startswith("--map="):
            map_path = a.split("=", 1)[1]
        elif a == "--unbake":
            undo = True
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.load_level(map_path)
    if undo:
        unbake()
    else:
        bake(folder)
    les.save_current_level()


if __name__ == "__main__":
    main(sys.argv[1:])
