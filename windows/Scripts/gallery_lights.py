"""
Light the works as a museum does (run after relight.py; idempotent):

    UnrealEditor-Cmd.exe MuseeVision.uproject -run=pythonscript -script="Scripts/gallery_lights.py"

Museums light the art above the room: warm (about 3000 K), high-CRI accent spots from the
ceiling, aimed at about 30° off the wall so the light rakes gently and the frame's shadow falls
short, with the illuminance set by conservation: about 150–200 lux on oil paintings (up to about
325), about 50 lux on works on paper, photographs and textiles, more on stone and bronze. By day
controlled daylight through the laylights fills the rooms; at night the laylights dim and these
spots carry the galleries. Here each work gets its own spot, sized to it, with its intensity set in
candela for the target illuminance.
"""
import importlib
import math
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import musee_paths as P  # noqa: E402

ACCENT_TAG = "musee.accent"
# Neutral white (the user: 3000 K read too warm); high-CRI museum LED at 4000 K sits between the
# laylights' daylight and a warm lamp.
COLOUR_TEMPERATURE = 4000.0

# Target illuminance on the work (lux). A little above conservation levels for paintings so they
# still read against daylight in a game; works on paper stay low.
LUX_PAINTING = 300.0
LUX_PAPER = 70.0          # photographs, scrolls
LUX_OBJECT = 150.0        # ceramics
LUX_SALON_PAINTING = 650.0   # Salon interior: the paintings the brightest thing in view, the room in half-light
LUX_SCULPTURE = 500.0

# How high above a painting's centre the spot hangs, per wing (m): under the Salon's vaults, the
# Hall of Light's glass and the Chinese cloister's lower roof.
MOUNT_ABOVE = {"Salon": 4.5, "HallOfLight": 2.4, "ChineseWing": 1.5, "Elan": 2.5, "Rotunda": 3.0, "SculptureHall": 3.5,
               "Albion": 3.0}   # (Albion: from the aisles' iron, over the picture rail)
AIM_FROM_WALL_DEGREES = 30.0
SCULPTURE_COURT_CENTRE = (0.0, -22.5)   # plan m: the middle of the Sculpture Hall's court

# The Salon as the Met's paintings galleries: no fixture next to the art (the bronze picture lights
# are off: PICTURE_LIGHTS), each painting lit from high up by a concealed spot, the vault washed
# from its coves, and the laylights over all.
LUX_SALON_FILL = 150.0
PICTURE_LIGHT_LUMENS_PER_M = 900.0
FREE_STANDING = {"monet-impression-sunrise", "van-gogh-starry-night"}   # on glass steles
SALON_BAYS = [(0, 11.4), (12.6, 23.4), (24.6, 35.4), (36.6, 47.4), (48.6, 60.0)]   # salon data x, m
SALON_HALF_WIDTH = 7.0
SALON_SPRINGING = 6.5
COVE_LUMENS_PER_M = 220.0   # Salon interior: the vault in half-light (cove light at half, the coffers in shade)
PICTURE_LIGHTS = False


def log(msg):
    unreal.log(f"[gallery_lights] {msg}")


def tag_value(actor, prefix):
    for t in actor.tags:
        s = str(t)
        if s.startswith(prefix):
            return s[len(prefix):]
    return None


def child(actor, label):
    for c in actor.get_attached_actors():
        if c.get_actor_label() == label:
            return c
    return None


def spot(eas, wing, work, at, target, radius_m, lux, label):
    """A neutral-white accent spot at `at` (cm) aimed at `target` (cm), its cone covering radius_m there."""
    d = unreal.Vector(target.x - at.x, target.y - at.y, target.z - at.z)
    dist_m = math.sqrt(d.x * d.x + d.y * d.y + d.z * d.z) / 100.0
    rot = unreal.MathLibrary.find_look_at_rotation(at, target)
    a = eas.spawn_actor_from_class(unreal.SpotLight, at, rot)
    a.set_actor_label(label)
    a.tags = [unreal.Name(ACCENT_TAG), unreal.Name("musee.building"), unreal.Name(f"musee.wing:{wing}"), unreal.Name(f"accent:{work}")]
    c = a.spot_light_component
    # Soft-edged: the beam falls off from a small core to just past the frame, over the wall washers'
    # even light, so a painting is lifted without a disc of light round it.
    outer = math.degrees(math.atan2(radius_m, dist_m)) + 5.0
    c.set_editor_property("outer_cone_angle", outer)
    c.set_editor_property("inner_cone_angle", outer * 0.3)
    c.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    # E = I cos θ / d²: the beam meets a wall at 60° from its normal (cos 0.5), an object head-on.
    incidence = 0.5 if radius_m < 5 and lux != LUX_SCULPTURE else 0.85
    c.set_editor_property("intensity", lux * dist_m * dist_m / incidence)
    c.set_editor_property("use_temperature", True)
    c.set_editor_property("temperature", COLOUR_TEMPERATURE)
    c.set_editor_property("attenuation_radius", (dist_m + radius_m + 2.0) * 100.0)
    c.set_editor_property("source_radius", 10.0)
    c.set_editor_property("soft_source_radius", 10.0)
    c.set_editor_property("cast_shadows", True)
    return a


def tagged(actor, wing, work):
    actor.tags = [unreal.Name(ACCENT_TAG), unreal.Name("musee.building"), unreal.Name(f"musee.wing:{wing}"), unreal.Name(f"accent:{work}")]


def picture_light(eas, wing, work, canvas_origin, canvas_extent, f):
    """A bronze bar on a short arm above the frame, and a warm rect light washing down over the canvas."""
    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    bronze = unreal.load_asset(f"{P.MATERIALS}/M_Gilt_Aged") or unreal.load_asset(f"{P.MATERIALS}/M_Gilt")
    width_m = max(canvas_extent.x, canvas_extent.y) * 2 / 100.0
    half_h_m = canvas_extent.z / 100.0
    bar_m = min(1.4, max(0.35, width_m * 0.55))
    yaw = math.degrees(math.atan2(f.y, f.x))
    top = unreal.Vector(canvas_origin.x, canvas_origin.y, canvas_origin.z + (half_h_m + 0.16) * 100)
    bar_at = unreal.Vector(top.x + f.x * 16, top.y + f.y * 16, top.z)
    for size, at in [((0.06, bar_m, 0.035), bar_at),                                          # the bar
                     ((0.16, 0.02, 0.02), unreal.Vector(top.x + f.x * 8, top.y + f.y * 8, top.z + 1))]:   # the arm
        a = eas.spawn_actor_from_class(unreal.StaticMeshActor, at, unreal.Rotator(0, 0, yaw))
        a.static_mesh_component.set_static_mesh(cube)
        if bronze:
            a.static_mesh_component.set_material(0, bronze)
        a.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        a.set_actor_scale3d(unreal.Vector(*size))
        a.set_actor_label(f"Picture light {work}")
        tagged(a, wing, work)
    aim = unreal.Vector(canvas_origin.x, canvas_origin.y, canvas_origin.z - half_h_m * 30)
    light = eas.spawn_actor_from_class(unreal.RectLight, unreal.Vector(bar_at.x, bar_at.y, bar_at.z - 3),
                                       unreal.MathLibrary.find_look_at_rotation(bar_at, aim))
    c = light.rect_light_component
    c.set_editor_property("source_width", bar_m * 100)
    c.set_editor_property("source_height", 3.0)
    c.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
    c.set_editor_property("intensity", PICTURE_LIGHT_LUMENS_PER_M * bar_m)
    c.set_editor_property("use_temperature", True)
    c.set_editor_property("temperature", COLOUR_TEMPERATURE)
    c.set_editor_property("attenuation_radius", (half_h_m * 2 + 1.5) * 100)
    c.set_editor_property("barn_door_angle", 35.0)
    c.set_editor_property("barn_door_length", 8.0)
    light.set_actor_label(f"Picture light {work}")
    tagged(light, wing, work)


# The native Salon's light slots (ASalonStructure.GetLightLines): lumens per metre by kind.
# Only the vault coves: no light lines along the silk, next to the paintings.
SLOT_LUMENS_PER_M = {"VaultCove": COVE_LUMENS_PER_M}
WASHER_OUT = 2.5            # m from the wall into the room
WASHER_HEIGHT = 6.4         # m, at the vault's springing
WASHER_AIM_HEIGHT = 2.6     # m, the middle of the hang
WASHER_LUMENS_PER_M = 300.0   # Salon interior: the silk about a third of the paintings (the Light board)


def salon_light_lines(eas):
    """
    A warm LED line in each of the native Salon's slots: the vault coves (hidden behind their lip),
    the slot under each bay's cornice and the one in its skirting (rendering 01). False if there is
    no native Salon in the map.
    """
    cls = unreal.load_class(None, "/Script/MuseeVision.SalonStructure")
    salon = next((a for a in eas.get_all_level_actors() if cls and a.get_class() == cls), None)
    if not salon:
        return False
    made = 0
    for line in salon.get_light_lines():
        kind = str(line.get_editor_property("kind"))
        per_m = SLOT_LUMENS_PER_M.get(kind)
        if not per_m:
            continue
        a, b = line.get_editor_property("start"), line.get_editor_property("end")
        facing = line.get_editor_property("facing")
        length = (b - a).length() / 100.0
        if length < 0.2:
            continue
        mid = (a + b) * 0.5
        light = eas.spawn_actor_from_class(unreal.RectLight, mid, unreal.MathLibrary.find_look_at_rotation(mid, mid + facing))
        c = light.rect_light_component
        c.set_editor_property("source_width", length * 100)
        c.set_editor_property("source_height", 3.0)
        c.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
        c.set_editor_property("intensity", per_m * length)
        c.set_editor_property("use_temperature", True)
        c.set_editor_property("temperature", COLOUR_TEMPERATURE)
        c.set_editor_property("attenuation_radius", 1200.0)
        c.set_editor_property("cast_shadows", kind == "VaultCove")
        light.set_actor_label(f"Salon {kind} {made + 1}")
        tagged(light, "Salon", f"slot-{kind}-{made + 1}")
        made += 1
    # Wall washers, as the Met lights its paintings galleries: for each silk wall, a linear light high
    # in the room (at the vault's springing, 2.5 m out from the wall, far from the art) aimed down at
    # the wall's middle, so the silk is washed evenly and the accent spots only lift the paintings.
    washers = 0
    for line in salon.get_light_lines():
        if str(line.get_editor_property("kind")) != "SilkTop":
            continue
        a, b = line.get_editor_property("start"), line.get_editor_property("end")
        facing = line.get_editor_property("facing")
        into_room = unreal.Vector(-facing.x, -facing.y, 0.0)
        n = into_room.length()
        length = (b - a).length() / 100.0
        if n < 1e-3 or length < 0.5:
            continue
        into_room = unreal.Vector(into_room.x / n, into_room.y / n, 0.0)
        mid = (a + b) * 0.5
        at = unreal.Vector(mid.x + into_room.x * WASHER_OUT * 100, mid.y + into_room.y * WASHER_OUT * 100, WASHER_HEIGHT * 100)
        target = unreal.Vector(mid.x, mid.y, WASHER_AIM_HEIGHT * 100)
        light = eas.spawn_actor_from_class(unreal.RectLight, at, unreal.MathLibrary.find_look_at_rotation(at, target))
        c = light.rect_light_component
        c.set_editor_property("source_width", length * 100)
        c.set_editor_property("source_height", 8.0)
        c.set_editor_property("barn_door_angle", 50.0)
        c.set_editor_property("barn_door_length", 20.0)
        c.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
        c.set_editor_property("intensity", WASHER_LUMENS_PER_M * length)
        c.set_editor_property("use_temperature", True)
        c.set_editor_property("temperature", COLOUR_TEMPERATURE)
        c.set_editor_property("attenuation_radius", 1200.0)
        light.set_actor_label(f"Salon wall washer {washers + 1}")
        tagged(light, "Salon", f"washer-{washers + 1}")
        washers += 1
    log(f"{made} light lines in the native Salon's slots, {washers} wall washers")
    return True


def salon_coves(eas):
    """A warm line of light along both long walls of each bay, at the springing, facing the vault."""
    if salon_light_lines(eas):
        return
    for i, (d0, d1) in enumerate(SALON_BAYS):
        x0, x1 = -14 - d0, -14 - d1
        length = abs(x1 - x0) - 1.0
        for side in (-1, 1):   # north, south
            at = unreal.Vector((x0 + x1) * 50, side * (SALON_HALF_WIDTH - 0.25) * 100, (SALON_SPRINGING + 0.05) * 100)
            yaw = -90.0 if side > 0 else 90.0   # face the axis
            light = eas.spawn_actor_from_class(unreal.RectLight, at, unreal.Rotator(0, 72, yaw))
            c = light.rect_light_component
            c.set_editor_property("source_width", length * 100)
            c.set_editor_property("source_height", 4.0)
            c.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
            c.set_editor_property("intensity", COVE_LUMENS_PER_M * length)
            c.set_editor_property("use_temperature", True)
            c.set_editor_property("temperature", COLOUR_TEMPERATURE)
            c.set_editor_property("attenuation_radius", 1200.0)
            light.set_actor_label(f"Cove bay {i + 1} {'south' if side > 0 else 'north'}")
            tagged(light, "Salon", f"cove-{i + 1}")


def main():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    les.load_level(P.MAP_PATH)

    old = [a for a in eas.get_all_level_actors() if ACCENT_TAG in [str(t) for t in a.tags]]
    for a in old:
        eas.destroy_actor(a)

    counts = {}
    for actor in eas.get_all_level_actors():
        work = tag_value(actor, "work:")
        wing = tag_value(actor, "musee.wing:")
        if not work or not wing or wing in ("Reserve", "ClassicalHall", "Chenghuai"):   # the Classical Hall lights its own
            # (Chenghuai: its works are lit by their cases' canopies and the paper windows, AChenghuaiStructure's lamps)
            continue   # the Reserve's works are in store: the easel has its own light
        if any(str(t).startswith("part:reserve_rack_work") for t in actor.tags):
            continue
        if "musee.retired" in [str(t) for t in actor.tags]:
            continue   # taken out of the museum (relight.py): nothing to light
        if "musee.daylit" in [str(t) for t in actor.tags]:
            continue   # Salon interior: the willows round the oval are lit by the velarium's daylight alone (as Monet wanted)

        canvas = child(actor, "Canvas")
        tags = [str(t) for t in actor.tags]
        if not canvas and "musee.albion.wall" in tags:
            # Albion: a work hung flat on the wall (AAlbionWork, facing its +X): lit like a canvas, a textile or paper lower.
            origin, extent = actor.get_actor_bounds(False)
            fv = actor.get_actor_forward_vector()
            f = unreal.Vector(fv.x, fv.y, 0.0)
            up = MOUNT_ABOVE.get(wing, 2.5)
            out = up * math.tan(math.radians(AIM_FROM_WALL_DEGREES))
            at = unreal.Vector(origin.x + f.x * out * 100, origin.y + f.y * out * 100, origin.z + up * 100)
            at = importlib.import_module("albion_place").mount(origin, f)   # Albion: on its iron, where its fitting is
            size_m = max(extent.x, extent.y, extent.z) / 100.0 * 2
            paper = "musee.paper" in tags or "musee.textile" in tags
            spot(eas, wing, work, at, origin, 0.55 * size_m + 0.25, LUX_PAPER if paper else LUX_PAINTING, f"Accent {work}")
            counts["pictures on paper" if paper else "paintings"] = counts.get("pictures on paper" if paper else "paintings", 0) + 1
            continue
        if canvas:
            # A picture: its origin is on the wall, its canvas a few cm in front, so that is its facing.
            origin, extent = canvas.get_actor_bounds(False)
            wall = actor.get_actor_location()
            f = unreal.Vector(origin.x - wall.x, origin.y - wall.y, 0.0)
            length = math.hypot(f.x, f.y)
            if length < 0.1:
                continue
            f = unreal.Vector(f.x / length, f.y / length, 0.0)
            up = MOUNT_ABOVE.get(wing, 2.5)
            out = up * math.tan(math.radians(AIM_FROM_WALL_DEGREES))
            at = unreal.Vector(origin.x + f.x * out * 100, origin.y + f.y * out * 100, origin.z + up * 100)
            size_m = max(extent.x, extent.y, extent.z) / 100.0 * 2
            paper = wing in ("HallOfLight", "ChineseWing")
            picture_lit = PICTURE_LIGHTS and wing == "Salon" and work not in FREE_STANDING
            lux = LUX_PAPER if paper else (LUX_SALON_FILL if picture_lit else (LUX_SALON_PAINTING if wing == "Salon" else LUX_PAINTING))
            spot(eas, wing, work, at, origin, 0.55 * size_m + 0.25, lux, f"Accent {work}")
            if picture_lit:
                picture_light(eas, wing, work, origin, extent, f)
            counts["pictures on paper" if paper else "paintings"] = counts.get("pictures on paper" if paper else "paintings", 0) + 1
            continue

        # An object: a sculpture scan, or a ceramic. Key light from above, towards the room.
        origin, extent = actor.get_actor_bounds(False)
        if extent.z <= 0:
            continue
        # Key light from the room's side of the work: in the Sculpture Hall from the court's centre (the
        # Rotunda's centre put lights for works by the south wall outside the court).
        toward = unreal.Vector(SCULPTURE_COURT_CENTRE[0] * 100 - origin.x, SCULPTURE_COURT_CENTRE[1] * 100 - origin.y, 0.0)             if wing == "SculptureHall" else unreal.Vector(1.0, 0.0, 0.0)
        length = math.hypot(toward.x, toward.y) or 1.0
        toward = unreal.Vector(toward.x / length, toward.y / length, 0.0)
        sculpture = wing in ("SculptureHall", "Salon") and "musee.paper" not in [str(t) for t in actor.tags]   # (the 1874 case: paper)
        up = MOUNT_ABOVE.get(wing, 2.5) + (1.0 if sculpture else 0.0)
        out = up * 0.8
        target = unreal.Vector(origin.x, origin.y, origin.z + extent.z * 0.2)
        at = unreal.Vector(origin.x + toward.x * out * 100, origin.y + toward.y * out * 100, target.z + up * 100)
        if wing == "Albion":   # Albion: the cases' objects lit from the arcade's girder, where the spot's fitting is
            at = importlib.import_module("albion_place").mount(target, unreal.Vector(1.0, 0.0, 0.0))
        radius_m = max(extent.x, extent.y, extent.z) / 100.0 * 1.1 + 0.2
        lux = LUX_PAPER if "musee.paper" in [str(t) for t in actor.tags] else (LUX_SCULPTURE if sculpture else LUX_OBJECT)
        spot(eas, wing, work, at, target, radius_m, lux, f"Accent {work}")
        counts["sculptures" if sculpture else "objects"] = counts.get("sculptures" if sculpture else "objects", 0) + 1

    salon_coves(eas)
    les.save_current_level()
    log(", ".join(f"{v} {k}" for k, v in sorted(counts.items())) + f" lit (replaced {len(old)})")


if __name__ == "__main__":
    main()
