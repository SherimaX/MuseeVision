"""
Albion (England 1848-1898) on the Rotunda's south door, in place of the Chinese Wing (native.py calls place() after the
rooms: AAlbionStructure and AAlbionSky are native.py ROOMS, which retire every imported /Museum/ChineseWing prim).
Here: the Chinese Wing's native actor goes (its code stays, Source/MuseeVision/ChineseWing), and whatever else was the
wing's and is not an imported prim (its garden's plants, spawned by nature.py, which no longer places them); the Kelmscott
Chaucer on its lectern; the hang (albion_hang.py place: the works, their frames).

    (in the editor, inside apply_all's native step)
"""
import importlib
import math

import unreal

NATIVE_TAG = "musee.native"
BOOK_TAG = "musee.albion.book"
# Plan metres (AlbionPlan.h: LecternX, LecternY).
LECTERN = (12.2, 20.2)


def log(msg):
    unreal.log(f"[albion_place] {msg}")


def albion_present():
    return hasattr(unreal, "AlbionStructure")


def retire_chinese_wing(eas):
    gone = 0
    cls = unreal.load_class(None, "/Script/MuseeVision.ChineseWingStructure")
    for a in eas.get_all_level_actors():
        tags = [str(t) for t in a.tags]
        imported = any(t.startswith("prim:") for t in tags)
        if (cls and a.get_class() == cls) or ("musee.wing:ChineseWing" in tags and not imported):
            eas.destroy_actor(a)
            gone += 1
    log(f"the Chinese Wing: {gone} native actors removed (its imported prims are retired by native.py)")


def place_book(eas):
    for a in eas.get_all_level_actors():
        if BOOK_TAG in [str(t) for t in a.tags]:
            eas.destroy_actor(a)
    book = eas.spawn_actor_from_class(unreal.AlbionChaucer, unreal.Vector(LECTERN[0] * 100.0, LECTERN[1] * 100.0, 0.0), unreal.Rotator(0, 0, 0))
    book.set_actor_label("Albion Kelmscott Chaucer (native)")
    book.tags = [unreal.Name("musee.building"), unreal.Name("musee.wing:Albion"), unreal.Name(NATIVE_TAG), unreal.Name(BOOK_TAG),
                 unreal.Name("work:albion-kelmscott-chaucer")]
    log("the Kelmscott Chaucer on its lectern")


# The accent spots' places on the iron (AlbionPlan.h; AlbionIron.cpp), plan metres. A light is a fitting you can see:
# AAlbionLamps builds a projector, its yoke and clamp at each Albion accent spot, so each must sit where there is iron.
ARCADE_X = 6.0
GIRDER_FACE = 0.45           # the spot's axis beside the box girder (0.35 m half-width, its flange angles, the clamp)
GIRDER_HANG = 9.30           # below the girder's bottom flange (9.70) by the yoke and its stem
NAVE_R, NAVE_OFF = 7.0833333, 1.0833333      # the nave's glass arcs (their centres 1.083 m across the axis)
AISLE_R, AISLE_OFF, AISLE_X = 5.38, 1.38, 10.0
RIB_UNDER = 0.80             # under the glass: below the main rib's inner flange (0.41) by the yoke
SPRING = 10.0


def glass_z(x):
    """The vaults' glass over plan x (the transverse section)."""
    ax = abs(x)
    if ax <= ARCADE_X:
        return SPRING + math.sqrt(max(0.0, NAVE_R ** 2 - (ax + NAVE_OFF) ** 2))
    d = abs(ax - AISLE_X)
    return SPRING + math.sqrt(max(0.0, AISLE_R ** 2 - (d + AISLE_OFF) ** 2))


def mount(target, facing):
    """Where a work's accent spot hangs on Albion's iron (world cm), for a work at `target` (world cm) facing `facing`.
    Side walls' works and the cases' objects: from the arcade's box girder on their aisle's side (about 45° down onto the
    wall, so the varnish's reflection falls to the floor short of a viewer). End walls' works: from the next line's main
    rib, over the work (about 25° off the vertical)."""
    x, y = target.x / 100.0, target.y / 100.0
    if abs(facing.y) > 0.7:
        line_y = 47.2 if facing.y < 0 else 23.2          # the south wall's works face north; the north wall's south
        return unreal.Vector(x * 100.0, line_y * 100.0, (glass_z(x) - RIB_UNDER) * 100.0)
    side = 1.0 if x > 0 else -1.0
    return unreal.Vector(side * (ARCADE_X + GIRDER_FACE) * 100.0, y * 100.0, GIRDER_HANG * 100.0)


def place(eas):
    if not albion_present():
        log("no AlbionStructure class: build the editor first")
        return
    retire_chinese_wing(eas)
    place_book(eas)
    importlib.import_module("albion_hang").unreal_place()
