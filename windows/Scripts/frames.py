"""
Frame the paintings (AMuseeFrame, C++): carved gilt frames in the Salon (and on the Reserve's
racks and the Starry Night), thin dark frames with white mats in the Hall of Light. Run after
import_wing.py (a re-import replaces the frames with the rest of the wing); idempotent:

    UnrealEditor-Cmd.exe MuseeVision.uproject -run=pythonscript -script="Scripts/frames.py"
    ... -script="Scripts/frames.py --rebuild-materials"     (remake M_Frame_Gilt / _Wood_Dark / _Mat)

For every painting the USD brought in (an actor with "Canvas" and "Frame" children, from
MuseumScene.hangFramed) it measures the canvas, hides the imported box frame, and places an
AMuseeFrame attached to the painting, styled by wing and work:

- Salon, Reserve: SalonGilt; CabinetGilt below 0.4 m across; widths capped so neighbours don't
  touch, and 10 cm kept clear above a Salon frame for the picture light when a work hangs above it;
- Élan: the Starry Night keeps a slim frame (SalonGilt, 6 cm), backed (it stands on a glass stele,
  as Impression, Sunrise does in the Salon);
- Hall of Light: Photograph. The autochromes (which had a frame) get a white mat to 1.0 x 0.9 m;
  the prints (on pale 1.0 x 0.9 m mounts with a bronze edge, no frame) get a thin dark frame round
  the mount, which serves as their mat, and the bronze edge is hidden;
- the Chinese Wing's scrolls and the Water Lilies are left as they are (unframed).

The gilt is /Game/Museum/Materials/M_Gilt_Aged when it exists; this script makes a fallback,
M_Frame_Gilt (aged gold: dark in the recesses, bright and burnished on the raised ornament, the red
bole rubbed through on the highest edges; from the frame's vertex colours), and M_Frame_Wood_Dark
and M_Frame_Mat if they are missing.
"""
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import musee_paths as P  # noqa: E402

MEL = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

FRAME_CLASS = "/Script/MuseeVision.MuseeFrame"
FRAME_TAG = "musee.frame"

# Works on glass steles: their backs show through the glass, so the frame gets a backboard.
ON_STELES = {"monet_impression_sunrise", "starry_night"}
# Fixed moulding widths (m): the Starry Night's slim frame.
FIXED_WIDTH = {"starry_night": 0.06}
# Smaller than this across (m): a cabinet frame.
CABINET_BELOW = 0.40
# Per wing, a factor on the default gilt width (the Reserve's racks hang works close together).
WING_WIDTH_SCALE = {"Reserve": 0.8}
# Wings and works left unframed.
UNFRAMED_WINGS = {"ChineseWing"}
UNFRAMED_NAMES = ("water_lilies",)
# The Hall of Light's mounts (m): the prints' pale panels, and the autochromes' mats to match.
HOL_MOUNT = (1.0, 0.9)
FRAME_HOL_PRINTS = True
PHOTO_WIDTH = 0.022
# Clearances (m): between neighbouring frames, and above a Salon frame for its picture light.
NEIGHBOUR_GAP = 0.04
PICTURE_LIGHT_CLEAR = 0.10
MIN_WIDTH = {"SalonGilt": 0.06, "CabinetGilt": 0.04, "Photograph": 0.015}


def log(msg):
    unreal.log(f"[frames] {msg}")


def warn(msg):
    unreal.log_warning(f"[frames] {msg}")


def tag_value(actor, prefix):
    for t in actor.tags:
        s = str(t)
        if s.startswith(prefix):
            return s[len(prefix):]
    return None


def set_prop(obj, name, value):
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception as e:  # noqa: BLE001
        warn(f"{obj.get_name()}: {name} not set ({e})")
        return False


# --------------------------------------------------------------------------- materials

def new_material(name):
    """A material to (re)build: an existing one is emptied in place, so what refers to it keeps it."""
    path = f"{P.MATERIALS}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        m = unreal.load_asset(path)
        MEL.delete_all_material_expressions(m)
        return m
    return tools.create_asset(name, P.MATERIALS, unreal.Material, unreal.MaterialFactoryNew())


def ex(mat, cls, x, y, **props):
    e = MEL.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        set_prop(e, k, v)
    return e


def link(a, a_out, b, b_in):
    if not MEL.connect_material_expressions(a, a_out, b, b_in):
        warn(f"could not connect {a.get_name()}.{a_out or 'out'} to {b.get_name()}.{b_in or 'in'}")


def op(mat, cls, x, y, a, b=None, a_out="", b_out="", **props):
    """A two-input node (Add, Subtract, Multiply…): b may be a node or, via props, a constant."""
    e = ex(mat, cls, x, y, **props)
    link(a, a_out, e, "A")
    if b is not None:
        link(b, b_out, e, "B")
    return e


def one_input(mat, cls, x, y, a, a_out=""):
    e = ex(mat, cls, x, y)
    link(a, a_out, e, "")
    return e


def lerp(mat, x, y, a=None, b=None, alpha=None, a_out="", b_out="", alpha_out="", **props):
    e = ex(mat, unreal.MaterialExpressionLinearInterpolate, x, y, **props)
    if a is not None:
        link(a, a_out, e, "A")
    if b is not None:
        link(b, b_out, e, "B")
    if alpha is not None:
        link(alpha, alpha_out, e, "Alpha")
    return e


def colour(mat, rgb, x, y):
    return ex(mat, unreal.MaterialExpressionConstant3Vector, x, y, constant=unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))


def noise(mat, x, y, scale, levels, position=None):
    e = ex(mat, unreal.MaterialExpressionNoise, x, y, scale=scale, levels=levels, output_min=0.0, output_max=1.0)
    fn = getattr(getattr(unreal, "NoiseFunction", None), "NOISEFUNCTION_GRADIENT_ALU", None)
    if fn is not None:
        set_prop(e, "noise_function", fn)
    if position is not None:
        link(position, "", e, "Position")
    return e


def saturate(mat, x, y, a):
    return one_input(mat, unreal.MaterialExpressionSaturate, x, y, a)


def make_gilt():
    """
    M_Frame_Gilt: aged water gilding from the frame's vertex colours (R exposure, G sanded frieze,
    B carved ornament). Recesses go dark and brown with old grime and roughen; the raised ornament
    stays bright and burnished; on the highest edges the gold is rubbed through to the red bole.
    """
    m = new_material("M_Frame_Gilt")
    vc = ex(m, unreal.MaterialExpressionVertexColor, -1800, 0)
    coarse = noise(m, -1800, 300, 0.07, 3)          # grime patches, ~15 cm
    fine = noise(m, -1800, 500, 1.4, 2)             # rubbing and tooling, a few mm

    # dirt = saturate((1 − exposure) × 1.35 + (coarse − ½) × 0.5 − 0.15)
    hollow = one_input(m, unreal.MaterialExpressionOneMinus, -1550, 0, vc, "R")
    hollow2 = op(m, unreal.MaterialExpressionMultiply, -1400, 0, hollow, const_b=1.35)
    patch = op(m, unreal.MaterialExpressionSubtract, -1550, 300, coarse, const_b=0.5)
    patch2 = op(m, unreal.MaterialExpressionMultiply, -1400, 300, patch, const_b=0.5)
    dirt0 = op(m, unreal.MaterialExpressionAdd, -1250, 100, hollow2, patch2)
    dirt1 = op(m, unreal.MaterialExpressionSubtract, -1100, 100, dirt0, const_b=0.15)
    dirt = saturate(m, -950, 100, dirt1)

    # wear = saturate((exposure − 0.8) × 5) × saturate((fine − 0.55) × 5) × 0.85: bole on the high edges
    high = op(m, unreal.MaterialExpressionSubtract, -1550, 650, vc, a_out="R", const_b=0.8)
    high2 = saturate(m, -1250, 650, op(m, unreal.MaterialExpressionMultiply, -1400, 650, high, const_b=5.0))
    rub = op(m, unreal.MaterialExpressionSubtract, -1550, 800, fine, const_b=0.55)
    rub2 = saturate(m, -1250, 800, op(m, unreal.MaterialExpressionMultiply, -1400, 800, rub, const_b=5.0))
    wear0 = op(m, unreal.MaterialExpressionMultiply, -1100, 700, high2, rub2)
    wear = op(m, unreal.MaterialExpressionMultiply, -950, 700, wear0, const_b=0.85)

    # Colour: rich warm gold, down to brown grime in the hollows, a touch deeper on the sanded frieze.
    gold = colour(m, (0.95, 0.64, 0.27), -950, -350)
    grime = colour(m, (0.26, 0.16, 0.06), -950, -200)
    dirt_amount = op(m, unreal.MaterialExpressionMultiply, -800, 100, dirt, const_b=0.8)
    gilt = lerp(m, -650, -250, gold, grime, dirt_amount)
    sanded = lerp(m, -650, -80, alpha=vc, alpha_out="G", const_a=1.0, const_b=0.82)
    gilt2 = op(m, unreal.MaterialExpressionMultiply, -500, -200, gilt, sanded)
    bole = colour(m, (0.30, 0.10, 0.045), -650, 60)
    base = lerp(m, -300, -150, gilt2, bole, wear)
    MEL.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    metal = one_input(m, unreal.MaterialExpressionOneMinus, -300, 150, wear)
    MEL.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)

    # Roughness: burnished highs 0.2, grimy hollows 0.5, the sanded frieze and the bole matte.
    rough0 = lerp(m, -800, 350, alpha=dirt, const_a=0.2, const_b=0.5)
    sand_r = op(m, unreal.MaterialExpressionMultiply, -800, 480, vc, a_out="G", const_b=0.18)
    tool = op(m, unreal.MaterialExpressionSubtract, -950, 560, fine, const_b=0.5)
    tool2 = op(m, unreal.MaterialExpressionMultiply, -800, 560, tool, const_b=0.08)
    wear_r = op(m, unreal.MaterialExpressionMultiply, -800, 640, wear, const_b=0.35)
    r1 = op(m, unreal.MaterialExpressionAdd, -650, 400, rough0, sand_r)
    r2 = op(m, unreal.MaterialExpressionAdd, -500, 450, r1, tool2)
    r3 = op(m, unreal.MaterialExpressionAdd, -350, 500, r2, wear_r)
    MEL.connect_material_property(saturate(m, -200, 500, r3), "", unreal.MaterialProperty.MP_ROUGHNESS)

    # Ambient occlusion from the same exposure, for the deep cove under the ornament.
    ao = lerp(m, -300, 700, alpha=vc, alpha_out="R", const_a=0.55, const_b=1.0)
    MEL.connect_material_property(ao, "", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    MEL.recompile_material(m)
    return m


def make_wood():
    """M_Frame_Wood_Dark: dark oak with a faint grain along the moulding (UV U runs along it, in metres)."""
    m = new_material("M_Frame_Wood_Dark")
    uv = ex(m, unreal.MaterialExpressionTextureCoordinate, -1200, 0)
    stretch = ex(m, unreal.MaterialExpressionConstant2Vector, -1200, 150, r=3.0, g=90.0)
    grain_uv = op(m, unreal.MaterialExpressionMultiply, -1000, 50, uv, stretch)
    zero = ex(m, unreal.MaterialExpressionConstant, -1000, 200, r=0.0)
    pos = op(m, unreal.MaterialExpressionAppendVector, -850, 100, grain_uv, zero)
    grain = noise(m, -700, 100, 1.0, 3, position=pos)
    base = lerp(m, -450, 0, colour(m, (0.030, 0.019, 0.012), -700, -150), colour(m, (0.060, 0.038, 0.024), -700, -50), grain)
    MEL.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = lerp(m, -450, 200, alpha=grain, const_a=0.42, const_b=0.6)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = ex(m, unreal.MaterialExpressionConstant, -450, 350, r=0.4)
    MEL.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    MEL.recompile_material(m)
    return m


def make_mat_board():
    """M_Frame_Mat: white museum board (a warm white, matte)."""
    m = new_material("M_Frame_Mat")
    MEL.connect_material_property(colour(m, (0.86, 0.84, 0.78), -400, 0), "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(ex(m, unreal.MaterialExpressionConstant, -400, 150, r=0.92), "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(ex(m, unreal.MaterialExpressionConstant, -400, 250, r=0.35), "", unreal.MaterialProperty.MP_SPECULAR)
    MEL.recompile_material(m)
    return m


def make_linen():
    """M_Frame_Linen: the back of a canvas, unbleached linen: warm greige, a fine weave, matte."""
    m = new_material("M_Frame_Linen")
    uv = ex(m, unreal.MaterialExpressionTextureCoordinate, -1200, 0)
    weave_scale = ex(m, unreal.MaterialExpressionConstant2Vector, -1200, 150, r=160.0, g=160.0)
    w_uv = op(m, unreal.MaterialExpressionMultiply, -1000, 50, uv, weave_scale)
    zero = ex(m, unreal.MaterialExpressionConstant, -1000, 200, r=0.0)
    pos = op(m, unreal.MaterialExpressionAppendVector, -850, 100, w_uv, zero)
    weave = noise(m, -700, 100, 1.0, 2, position=pos)
    base = lerp(m, -450, 0, colour(m, (0.36, 0.30, 0.21), -700, -150), colour(m, (0.47, 0.40, 0.29), -700, -50), weave)
    MEL.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(ex(m, unreal.MaterialExpressionConstant, -400, 200, r=0.9), "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(ex(m, unreal.MaterialExpressionConstant, -400, 300, r=0.3), "", unreal.MaterialProperty.MP_SPECULAR)
    MEL.recompile_material(m)
    return m


def make_pine():
    """M_Frame_Pine: the stretcher bars and keys, pale planed pine gone honey with age, a faint grain."""
    m = new_material("M_Frame_Pine")
    pos = ex(m, unreal.MaterialExpressionWorldPosition, -1200, 100)
    grain = noise(m, -700, 100, 0.05, 3, position=pos)
    base = lerp(m, -450, 0, colour(m, (0.42, 0.28, 0.14), -700, -150), colour(m, (0.55, 0.39, 0.21), -700, -50), grain)
    MEL.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(ex(m, unreal.MaterialExpressionConstant, -400, 200, r=0.75), "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(ex(m, unreal.MaterialExpressionConstant, -400, 300, r=0.35), "", unreal.MaterialProperty.MP_SPECULAR)
    MEL.recompile_material(m)
    return m


def make_materials(force):
    """Make the frames' own materials if missing (or when forced); M_Gilt_Aged is someone else's."""
    for name, maker in (("M_Frame_Gilt", make_gilt), ("M_Frame_Wood_Dark", make_wood), ("M_Frame_Mat", make_mat_board),
                        ("M_Frame_Linen", make_linen), ("M_Frame_Pine", make_pine)):
        if unreal.EditorAssetLibrary.does_asset_exist(f"{P.MATERIALS}/{name}") and not force:
            continue
        try:
            m = maker()
            unreal.EditorAssetLibrary.save_loaded_asset(m, only_if_is_dirty=False)
            log(f"made {name}")
        except Exception as e:  # noqa: BLE001
            warn(f"{name} not made ({e}); the frames will use what they find")
    aged = unreal.EditorAssetLibrary.does_asset_exist(f"{P.MATERIALS}/M_Gilt_Aged")
    log("gilt: " + ("M_Gilt_Aged" if aged else "M_Frame_Gilt (M_Gilt_Aged not there yet; re-run this script when it is)"))


# --------------------------------------------------------------------------- the paintings

def hide(actor, why):
    """Hidden in game and in the editor, no collision; the components too, so showing the building
    again (MuseeWorld::SetBuildingHidden) doesn't bring it back."""
    for method, value in (("set_actor_hidden_in_game", True), ("set_actor_enable_collision", False),
                          ("set_is_temporarily_hidden_in_editor", True)):
        try:
            getattr(actor, method)(value)
        except Exception as e:  # noqa: BLE001
            warn(f"{actor.get_actor_label()}: {method} failed ({e})")
    for comp in actor.get_components_by_class(unreal.PrimitiveComponent):
        try:
            comp.set_visibility(False, True)
        except Exception as e:  # noqa: BLE001
            warn(f"{actor.get_actor_label()}: set_visibility failed ({e})")
        try:
            comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        except Exception as e:  # noqa: BLE001
            warn(f"{actor.get_actor_label()}: set_collision_enabled failed ({e})")
    log(f"  hid {actor.get_actor_label()} ({why})")


def child_role(child):
    """'Canvas' or 'Frame' for the painting's imported children (by prim path, else by label)."""
    prim = tag_value(child, "prim:") or ""
    leaf = prim.rsplit("/", 1)[-1] if prim else child.get_actor_label()
    for role in ("Canvas", "Frame"):
        if leaf == role or leaf.startswith(role + "_"):
            return role
    return None


def bounds(actor):
    origin, extent = actor.get_actor_bounds(False)
    return (origin.x, origin.y, origin.z), (extent.x, extent.y, extent.z)


def find_around(candidates, canvas):
    """The candidate whose bounds hold the canvas's centre (in the wall's plane) and sit just behind it."""
    (cx, cy, cz), (ex_, ey, ez) = bounds(canvas)
    thin = min(range(3), key=lambda i: (ex_, ey, ez)[i])
    c = (cx, cy, cz)
    best, best_d = None, None
    for a in candidates:
        o, e = bounds(a)
        if all(abs(c[i] - o[i]) <= e[i] + 5.0 for i in range(3) if i != thin) and abs(c[thin] - o[thin]) < 15.0:
            d = sum((c[i] - o[i]) ** 2 for i in range(3))
            if best is None or d < best_d:
                best, best_d = a, d
    return best


def gather(eas, frame_cls):
    """Paintings: {path: dict(actor, name, wing, prim, canvas, frame)}, the level's AMuseeFrames, and actors by wing."""
    actors = eas.get_all_level_actors()
    kids = {}
    frames = []
    by_wing = {}
    for a in actors:
        if a.get_class() == frame_cls:
            frames.append(a)
            continue
        wing = tag_value(a, "musee.wing:")
        if wing:
            by_wing.setdefault(wing, []).append(a)
        parent = a.get_attach_parent_actor()
        if parent is not None:
            kids.setdefault(parent.get_path_name(), []).append(a)
    paintings = {}
    for a in actors:
        children = kids.get(a.get_path_name(), [])
        roles = {}
        for c in children:
            r = child_role(c)
            if r and r not in roles:
                roles[r] = c
        if "Canvas" not in roles:
            continue
        prim = tag_value(a, "prim:") or ""
        paintings[a.get_path_name()] = {
            "actor": a,
            "name": prim.rsplit("/", 1)[-1] if prim else a.get_actor_label(),
            "wing": tag_value(a, "musee.wing:") or "",
            "prim": prim,
            "canvas": roles["Canvas"],
            "frame": roles.get("Frame"),
        }
    return paintings, frames, by_wing


def style_for(p, sight_w, sight_h):
    """(style, fixed width or 0, mat width or −1, mat outer size or None, backboard) or None to leave unframed."""
    wing, name = p["wing"], p["name"].lower()
    if wing in UNFRAMED_WINGS or any(n in name for n in UNFRAMED_NAMES):
        return None
    back = True   # every work shows its own back (AMuseeFrame bBackPanel): linen and stretcher, or a backing board
    if wing == "HallOfLight":
        if p["frame"] is not None:
            return ("Photograph", PHOTO_WIDTH, -1.0, HOL_MOUNT, back)   # an autochrome: mat to the mounts' size
        return ("Photograph", PHOTO_WIDTH, 0.0, None, back)             # a print: framed round its mount
    if p["frame"] is None:
        return None
    width = FIXED_WIDTH.get(name, 0.0)
    style = "CabinetGilt" if max(sight_w, sight_h) < CABINET_BELOW and not width else "SalonGilt"
    return (style, width, -1.0, None, back)


def configure(frame, plan):
    style, width, mat_width, mat_outer, back = plan
    outer = unreal.Vector2D(mat_outer[0], mat_outer[1]) if mat_outer else unreal.Vector2D(0.0, 0.0)
    return frame.configure(style, float(width), float(mat_width), outer, bool(back))


def vec(v):
    return (v.x, v.y, v.z)


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cap_widths(placed):
    """Narrow frames that would crowd a neighbour on the same wall (and keep the picture light's room)."""
    info = []
    for item in placed:
        f = item["frame"]
        mats = f.get_resolved_mat_widths()
        info.append({
            "loc": tuple(c / 100.0 for c in vec(f.get_actor_location())),
            "fwd": vec(f.get_actor_forward_vector()),
            "right": vec(f.get_actor_right_vector()),
            "up": vec(f.get_actor_up_vector()),
            "half": (0.5 * f.get_editor_property("sight_width") + mats.x, 0.5 * f.get_editor_property("sight_height") + mats.y),
            "cap": None,
        })
    for i, a in enumerate(info):
        for j, b in enumerate(info):
            if i == j or dot(a["fwd"], b["fwd"]) < 0.99:
                continue
            d = tuple(b["loc"][k] - a["loc"][k] for k in range(3))
            if abs(dot(d, a["fwd"])) > 0.15:
                continue
            dy, dz = dot(d, a["right"]), dot(d, a["up"])
            gy = abs(dy) - (a["half"][0] + b["half"][0])
            gz = abs(dz) - (a["half"][1] + b["half"][1])
            cap = None
            if gy > 0 and gz < 0:        # side by side
                cap = (gy - NEIGHBOUR_GAP) / 2.0
            elif gz > 0 and gy < 0:      # one above the other: the lower one's picture light needs room
                clear = PICTURE_LIGHT_CLEAR if placed[i]["painting"]["wing"] == "Salon" else 0.0
                cap = (gz - NEIGHBOUR_GAP - clear) / 2.0
            if cap is not None:
                a["cap"] = cap if a["cap"] is None else min(a["cap"], cap)
    capped = 0
    for item, a in zip(placed, info):
        f, plan = item["frame"], item["plan"]
        style = plan[0]
        width = f.get_resolved_frame_width()
        scale = WING_WIDTH_SCALE.get(item["painting"]["wing"], 1.0) if not plan[1] else 1.0
        target = width * scale
        if a["cap"] is not None and a["cap"] < target:
            target = max(MIN_WIDTH.get(style, 0.02), a["cap"])
            warn(f"  {item['painting']['name']}: frame narrowed to {target * 100:.1f} cm for its neighbours")
        if abs(target - width) > 1e-4:
            item["plan"] = (style, target) + tuple(plan[2:])
            configure(f, item["plan"])
            capped += 1
    return capped


def frame_all():
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    frame_cls = unreal.load_class(None, FRAME_CLASS)
    if frame_cls is None:
        raise RuntimeError(f"{FRAME_CLASS} not found: build the MuseeVision module first")
    paintings, frames, by_wing = gather(eas, frame_cls)
    existing = {}
    for f in frames:
        parent = f.get_attach_parent_actor()
        key = parent.get_path_name() if parent else None
        if key in paintings and key not in existing:
            existing[key] = f
        else:
            eas.destroy_actor(f)   # a duplicate, or its painting is gone

    hol_mounts = [a for a in by_wing.get("HallOfLight", []) if a.get_actor_label().startswith("Mount")]
    hol_edges = [a for a in by_wing.get("HallOfLight", []) if a.get_actor_label().startswith("Edge")]
    placed = []
    counts = {}
    skipped = 0
    for key, p in sorted(paintings.items(), key=lambda kv: kv[1]["name"]):
        painting, canvas, old = p["actor"], p["canvas"], p["frame"]
        mount = None
        if p["wing"] == "HallOfLight" and old is None:
            if not FRAME_HOL_PRINTS:
                skipped += 1
                continue
            mount = find_around(hol_mounts, canvas)
            if mount is None:
                skipped += 1          # the stereo stones' views, anything else without a mount
                continue
        elif old is None:
            skipped += 1              # unframed in the Swift build (scrolls, the Water Lilies)
            continue

        # Size the frame to the canvas first: the style depends on the size.
        (cx, cy, cz), _ = bounds(canvas)
        frame = existing.get(key)
        if frame is None:
            frame = eas.spawn_actor_from_class(frame_cls, unreal.Vector(cx, cy, cz), unreal.Rotator(0, 0, 0))
        if mount is not None:
            ok = frame.fit_to_canvas(canvas, mount, mount)
        else:
            ok = frame.fit_to_canvas(canvas, old, None)
        if not ok:
            warn(f"{p['name']}: canvas not measured; left as it was")
            if key not in existing:
                eas.destroy_actor(frame)
            skipped += 1
            continue
        plan = style_for(p, frame.get_editor_property("sight_width"), frame.get_editor_property("sight_height"))
        if plan is None:
            eas.destroy_actor(frame)   # left unframed (and any frame from an earlier run goes)
            skipped += 1
            continue
        if not configure(frame, plan):
            warn(f"{p['name']}: style {plan[0]} unknown to AMuseeFrame")
        frame.set_actor_label(f"{p['name']} frame")
        try:
            frame.attach_to_actor(painting, "", unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD,
                                  unreal.AttachmentRule.KEEP_WORLD, False)
        except Exception as e:  # noqa: BLE001
            warn(f"{p['name']}: not attached ({e})")
        frame.tags = [unreal.Name("musee.building"), unreal.Name(f"musee.wing:{p['wing']}"), unreal.Name(FRAME_TAG),
                      unreal.Name(f"frame.of:{p['prim']}"), unreal.Name(f"frame.style:{plan[0]}")]
        if old is not None:
            hide(old, "the imported box frame")
        if mount is not None:
            edge = find_around(hol_edges, mount)
            if edge is not None:
                hide(edge, "the bronze edge; the new frame goes round the mount")
        placed.append({"painting": p, "frame": frame, "plan": plan})
        counts[(p["wing"], plan[0])] = counts.get((p["wing"], plan[0]), 0) + 1

    narrowed = cap_widths(placed)
    for item in placed:
        f, p = item["frame"], item["painting"]
        size = f.get_outer_size()
        top = f.get_top_edge_world()
        log(f"  {p['wing']}/{p['name']}: {item['plan'][0]} {f.get_resolved_frame_width() * 100:.1f} cm, "
            f"sight {f.get_editor_property('sight_width'):.2f} x {f.get_editor_property('sight_height'):.2f} m, "
            f"outside {size.x:.2f} x {size.y:.2f} m, top edge at z {top.z:.0f} cm")
    for (wing, style), n in sorted(counts.items()):
        log(f"{wing}: {n} {style}")
    log(f"{len(placed)} frames placed ({len(existing)} updated), {narrowed} re-sized for neighbours, "
        f"{skipped} canvases left unframed")


def main(args):
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not unreal.EditorAssetLibrary.does_asset_exist(P.MAP_PATH):
        raise RuntimeError(f"{P.MAP_PATH} is missing: run Scripts/setup_project.py and import_wing.py first")
    les.load_level(P.MAP_PATH)
    make_materials("--rebuild-materials" in args)
    frame_all()
    les.save_current_level()
    unreal.EditorAssetLibrary.save_directory(P.MUSEUM_CONTENT, only_if_is_dirty=True, recursive=True)
    log("saved")


if __name__ == "__main__":
    main(sys.argv[1:])
