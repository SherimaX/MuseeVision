"""
Run several of the map scripts in one editor session: the engine starts once, the Museum map loads
once and is saved once, instead of once per script.

    UnrealEditor-Cmd.exe MuseeVision.uproject -run=pythonscript -script="<repo>/windows/Scripts/apply_all.py <steps>"

Steps run in the canonical order below, whatever order they are given in:

    unbake               bake.py --unbake (the native actors' procedural geometry back, for editing)
    art[:ID1,ID2]        art.py reimport (the prepared high-resolution images over the textures; all without a list)
    materials            materials.py (the master materials; recompiles shaders, so only when they change)
    glass                setup_project.make_glass (M_Glass rebuilt in place)
    exposure             exposure.py (metering and the compensation curve)
    relight[:W1,W2]      relight.py for those wings (all wings without a list)
    journeys[:PARTS]     journeys.py (Élan Cube: its materials, textures and journey meshes; all parts without a list)
    salon                salon_interior.py make (Salon interior: its materials, the willows' and documents' textures)
    native               native.py (the native rooms, the Reserve's lamps, the Atrium's ribs)
    classical            classical_art.py place (the casts on the Classical Hall's spots)
    frames               frames.py
    lights               gallery_lights.py (accent spots, the Salon's cove lines)
    nature[:ARGS]        nature.py, ARGS as its own options joined by commas (e.g. nature:--term,3)
    lawn[:PARTS]         lawn.py: the grounds' mown lawn (material, meshes, survey; all without a list); no unbake
    textures             texture_budget.py (every colour image BC7-compressed, mipped and streamed; also with bake)
    bake[:FOLDER]        bake.py: the native actors baked into Nanite static meshes (in /Game/Museum/Baked,
                         or FOLDER); re-runnable, it replaces the meshes and components
    all                  every step above but unbake and bake, relight for all wings

A baked actor doesn't rebuild, so when the map is baked and relight, native, frames, lights or nature
run, the map is unbaked first and baked again after them (bake is added).

e.g.  apply_all.py lights native      apply_all.py relight:Salon,Elan lights      apply_all.py bake

Each script also still runs on its own. Here each one is handed a LevelEditorSubsystem whose
load_level does nothing when the map is already open, and whose save_current_level is held until
the end, when the map is saved once. Asset saves (materials, textures, the plants' materials) are
the scripts' own and happen as before.
"""
import importlib
import os
import sys
import time

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import musee_paths as P  # noqa: E402

ORDER = ["unbake", "art", "glass", "materials", "exposure", "relight", "journeys", "salon", "native", "classical", "frames", "lights", "nature", "lawn", "textures", "bake"]
# Not in "all": the bake is a finishing step (a baked actor doesn't show edits to its code until rebaked).
NOT_IN_ALL = {"unbake", "bake"}
# Steps that change the native actors: a baked map is unbaked before them and baked again after.
CHANGES_NATIVE = {"relight", "native", "frames", "lights", "nature"}


def log(msg):
    unreal.log(f"[apply_all] {msg}")


class _OneSessionLevels:
    """A LevelEditorSubsystem that loads the map once and defers saving to the runner."""

    def __init__(self, real):
        self._real = real
        self.save_requested = False

    def load_level(self, path):
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        current = world.get_path_name().split(".")[0] if world else ""
        if current == path.split(".")[0]:
            return True
        return self._real.load_level(path)

    def save_current_level(self):
        self.save_requested = True
        return True

    def __getattr__(self, name):
        return getattr(self._real, name)


def _run_step(name, args, levels):
    if name == "unbake":
        importlib.import_module("bake").unbake()
        levels.save_requested = True
    elif name == "bake":
        importlib.import_module("bake").bake(args[0] if args else importlib.import_module("bake").FOLDER)
        levels.save_requested = True
    elif name == "art":
        importlib.import_module("art").reimport(set(args) or None)
    elif name == "materials":
        importlib.import_module("materials").main()
    elif name == "glass":
        sp = importlib.import_module("setup_project")
        sp.make_glass()
        sp.make_water()
        sp.make_misty()
        sp.make_stars()
    elif name == "exposure":
        importlib.import_module("exposure").main()
    elif name == "relight":
        importlib.import_module("relight").relight(args or None)
        levels.save_requested = True
    elif name == "journeys":   # Élan Cube
        importlib.import_module("journeys").main(args)
    elif name == "salon":   # Salon interior: its materials and textures (salon_interior.make); native.py places its pieces
        importlib.import_module("salon_interior").make()
    elif name == "native":
        importlib.import_module("native").main()
    elif name == "classical":
        importlib.import_module("classical_art").place()
    elif name == "frames":
        importlib.import_module("frames").main(args)
    elif name == "lights":
        importlib.import_module("gallery_lights").main()
    elif name == "nature":
        importlib.import_module("nature").main(args)
    elif name == "lawn":
        importlib.import_module("lawn").main(args, levels)
    elif name == "textures":   # the colour images BC7, mipped and streamed (texture_budget.py)
        importlib.import_module("texture_budget").fix()


def parse(argv):
    steps = {}
    for a in argv:
        if a.startswith("-"):
            continue
        name, _, rest = a.partition(":")
        if name == "all":
            steps.update({s: [] for s in ORDER if s not in NOT_IN_ALL})
            continue
        if name not in ORDER:
            raise ValueError(f"unknown step {name}; one of {ORDER + ['all']}")
        steps[name] = [x for x in rest.split(",") if x] if rest else []
    return [(s, steps[s]) for s in ORDER if s in steps]


def main(argv):
    steps = parse(argv)
    if not steps:
        log(__doc__)
        return
    real = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    levels = _OneSessionLevels(real)
    original = unreal.get_editor_subsystem

    def get_editor_subsystem(cls):
        return levels if cls == unreal.LevelEditorSubsystem else original(cls)

    unreal.get_editor_subsystem = get_editor_subsystem
    start = time.time()
    try:
        levels.load_level(P.MAP_PATH)
        log(f"map loaded in {time.time() - start:.0f} s")
        names = [s for s, _ in steps]
        if CHANGES_NATIVE & set(names) and "unbake" not in names and importlib.import_module("bake").baked_actors():
            log("the map is baked: unbaking it first, baking it again after")
            steps = [("unbake", [])] + steps + ([] if "bake" in names else [("bake", [])])
        if "bake" in [s for s, _ in steps] and "textures" not in [s for s, _ in steps]:
            # Every bake (and so every package) first gets the images into their GPU budget.
            steps.insert([s for s, _ in steps].index("bake"), ("textures", []))
        for name, args in steps:
            t = time.time()
            _run_step(name, args, levels)
            log(f"{name}{':' + ','.join(args) if args else ''} done in {time.time() - t:.0f} s")
    finally:
        unreal.get_editor_subsystem = original
    if levels.save_requested:
        t = time.time()
        real.save_current_level()
        log(f"map saved in {time.time() - t:.0f} s")
    log(f"{len(steps)} steps in {time.time() - start:.0f} s")


if __name__ == "__main__":
    main(sys.argv[1:])
