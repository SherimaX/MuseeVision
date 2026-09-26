"""
The bake measured on a scratch copy of the Museum map, which is never saved here:

    UnrealEditor-Cmd.exe MuseeVision.uproject -run=pythonscript -script="<repo>/windows/Scripts/bake_test.py --copy"
    UnrealEditor-Cmd.exe MuseeVision.uproject -run=pythonscript -script="<repo>/windows/Scripts/bake_test.py --bake"

--copy   /Game/Maps/Museum duplicated to /Game/Maps/BakeTest (replacing an earlier copy), unbaked
--bake   /Game/Maps/BakeTest baked (bake.py) and saved
(both, in that order, without an option)

Then run the game on /Game/Maps/BakeTest (UnrealEditor.exe MuseeVision.uproject /Game/Maps/BakeTest -game ...)
before and after the bake. Only /Game/Maps/BakeTest and /Game/Museum/Baked are written (and any
material the bake marks for Nanite).
"""
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import bake  # noqa: E402
import musee_paths as P  # noqa: E402

SCRATCH = "/Game/Maps/BakeTest"
EAL = unreal.EditorAssetLibrary


def log(msg):
    unreal.log(f"[bake_test] {msg}")


def current_map():
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    return world.get_path_name().split(".")[0] if world else ""


def save_scratch(les):
    here = current_map()
    if here != SCRATCH:
        raise RuntimeError(f"refusing to save {here}: only {SCRATCH} is saved here")
    les.save_current_level()
    log(f"{SCRATCH} saved")


def copy():
    if EAL.does_asset_exist(SCRATCH):
        if not EAL.delete_asset(SCRATCH):
            raise RuntimeError(f"could not replace {SCRATCH}")
    if not EAL.duplicate_asset(P.MAP_PATH, SCRATCH):
        raise RuntimeError(f"could not copy {P.MAP_PATH} to {SCRATCH}")
    if not EAL.save_asset(SCRATCH, only_if_is_dirty=False):
        raise RuntimeError(f"could not save {SCRATCH}")
    log(f"{P.MAP_PATH} copied to {SCRATCH}")


def bake_scratch():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not les.load_level(SCRATCH) or current_map() != SCRATCH:
        raise RuntimeError(f"could not open {SCRATCH}")
    bake.bake()
    save_scratch(les)


def main(argv):
    do_copy = "--copy" in argv or "--bake" not in argv
    do_bake = "--bake" in argv or "--copy" not in argv
    if do_copy:
        copy()
    if do_bake:
        bake_scratch()


if __name__ == "__main__":
    main(sys.argv[1:])
