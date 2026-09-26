"""
The museum's final materials (Unreal is the source of truth): five master materials, the named
instances other code uses, one instance per material the wings were imported with, and the swap of
every imported component onto them. usd/ is only read, for the imported materials' names and values.

    UnrealEditor-Cmd.exe MuseeVision.uproject -run=pythonscript -script="Scripts/materials.py" -AllowCommandletRendering

(-AllowCommandletRendering lets the commandlet compile shaders, so the log reports each master's
instruction count and checks the instance permutations; without it the assets are the same, but a
commandlet compiles no shaders and reports no counts.)

Run after setup_project.py and import_wing.py, and again after any re-import (the swap is made on the
level's components). Idempotent: the masters are rebuilt in place (instances and components keep
pointing at them), the instances are updated in place, the components are re-pointed. The log ends
with "done"; any "[materials]" warning above it is a real problem (the instance values are read
back and checked).

Masters (/Game/Museum/Materials), all opaque, built from engine expressions only:

- M_Stone (clear coat): stone drawn procedurally in world space, so neighbouring meshes line up and
  nothing repeats. A box projection picks the course direction from the vertex normal: walls, piers
  and curved drums get ashlar courses (CourseHeight × BlockLength, the length varying per course,
  staggered joints); floors and ceilings get a slab grid (FloorSlab). Fine joints (JointWidth,
  JointDarkness; WallJoints / FloorJoints turn them off), per-block tone, strata stretched along the
  courses (veins on floors, VeinAmount for marble), small pores along the strata, a large-scale tone
  drift, and roughness / clear coat for polished floors. Brick is the same pattern at brick size
  (FloorAspect for the vault soffits, JointColor for the mortar, AltColor per brick), and the
  Reserve's dark polished concrete is large sawcut panels. No drawn texture is used for stone;
  UseTexture multiplies in an image (rammed earth, the sun clock, glazed ceramics), and Inlay makes
  the sun clock's bronze lines metal.
- M_Metal: gilt, bronze, patinated bronze, steel; colour and roughness drift, a dark patina.
- M_Fabric: silk wall covering; horizontal slubs, warp and weft threads (faded before they
  shimmer), a linen mottle, a soft Fresnel sheen.
- M_Plaster: matte plaster and mouldings with a gentle drift and a fine grain.
- M_Wood: timber with a grain along the course direction and figure lines.

Named instances: M_Travertine_Honed, M_Travertine_Polished, M_Marble_Polished, M_Plaster_Coffer,
M_Plaster_Moulding, M_Gilt_Aged; the sun clock's M_SunClock_Marble, M_SunClock_MarbleAlt, M_Marble_Rosso,
M_Marble_Nero and M_SunClock_Drum. M_Gilt, M_RibSteel, M_Glass, M_MistyGlass, M_Daylit, M_IrisGlow and
MPC_Musee (setup_project.py) are not touched.

Per imported material: /Game/Museum/Materials/USD/MI_<name>, tuned to the renderings (LOOK_TARGETS.md)
from the values in usd/Materials.usda (tint, roughness, metallic, clear coat, tiling). Paintings, glass, the sky-lit panels (relight.py) and every
self-lit (museevision:unlit) material keep what they have; so does planting and water (no rule).
"""
import os
import re
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import musee_paths as P  # noqa: E402
import pbr  # noqa: E402

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

TEXTURES = P.MUSEUM_CONTENT + "/Textures"
USD_MATERIALS = P.MATERIALS + "/USD"
# Made by setup_project.py (and read by the C++): never rebuilt here.
PROTECTED = {"M_Gilt", "M_RibSteel", "M_Glass", "M_MistyGlass", "M_Daylit", "M_IrisGlow", "MPC_Musee"}

WARNINGS = []


def log(msg):
    unreal.log(f"[materials] {msg}")


def warn(msg):
    WARNINGS.append(msg)
    unreal.log_warning(f"[materials] {msg}")


# --------------------------------------------------------------------------- colours

def linear(c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def lin(h):
    """sRGB hex (0xE4DBCB or "#E4DBCB") → linear (r, g, b)."""
    if isinstance(h, str):
        h = int(h.lstrip("#"), 16)
    return (linear(((h >> 16) & 255) / 255), linear(((h >> 8) & 255) / 255), linear((h & 255) / 255))


def cmul(a, b):
    return tuple(x * y for x, y in zip(a, b))


def cscale(a, k):
    return tuple(min(1.0, x * k) for x in a)


def cmix(a, b, t):
    return tuple(x + (y - x) * t for x, y in zip(a, b))


def soft_tint(h, amount=0.25):
    """A near-white export tint at a quarter of its strength (it multiplied a drawn texture on the
    phone); enough to tell the wings' stone apart without pushing it towards yellow."""
    return cmix((1.0, 1.0, 1.0), lin(h), amount)


def linear_colour(c):
    return unreal.LinearColor(c[0], c[1], c[2], 1.0)


# --------------------------------------------------------------------------- small helpers

def set_prop(obj, prop, value, what=None):
    try:
        obj.set_editor_property(prop, value)
        return True
    except Exception as e:  # noqa: BLE001 - one bad name must not stop the run
        warn(f"{what or obj.get_name()}: {prop} not set ({e})")
        return False


# Expression properties are set without change notifications: each notification recompiles the
# whole material (hundreds of compiles per master, some of a half-built graph). finish() sends
# one notification on the material before the real compile.
QUIET = getattr(getattr(unreal, "PropertyAccessChangeNotifyMode", None), "NEVER", None)


def set_quiet(obj, prop, value, what=None):
    if QUIET is None:
        return set_prop(obj, prop, value, what)
    try:
        obj.set_editor_property(prop, value, QUIET)
        return True
    except Exception as e:  # noqa: BLE001
        warn(f"{what or obj.get_name()}: {prop} not set ({e})")
        return False


def enum_value(enum_cls, *names):
    """An enum entry by one of its Python names, tolerant of how the generator split the words."""
    if enum_cls is None:
        return None
    for n in names:
        if hasattr(enum_cls, n):
            return getattr(enum_cls, n)
    wanted = {n.replace("_", "").upper() for n in names}
    for attr in dir(enum_cls):
        if attr.replace("_", "").upper() in wanted:
            return getattr(enum_cls, attr)
    warn(f"{getattr(enum_cls, '__name__', enum_cls)}: none of {names}")
    return None


NOISE_FUNCTIONS = {
    # ~16 instructions per level, one volume-texture fetch; repeats every 16 units (hidden below).
    "tex3d": ("NOISEFUNCTION_GRADIENT_TEX3D", "NOISEFUNCTION_GRADIENT_TEX_3D", "NOISEFUNCTION_GradientTex3D"),
    "gradient": ("NOISEFUNCTION_GRADIENT_ALU", "NOISEFUNCTION_GradientALU"),
    "value": ("NOISEFUNCTION_VALUE_ALU", "NOISEFUNCTION_ValueALU"),
}


# --------------------------------------------------------------------------- graph building

class Graph:
    """Expressions in one material. Inputs are expressions, (expression, output name), or numbers."""

    def __init__(self, material):
        self.m = material
        self.name = material.get_name()
        self.count = 0

    def node(self, cls, **props):
        x = -3600 + (self.count // 32) * 300
        y = -1800 + (self.count % 32) * 115
        self.count += 1
        e = MEL.create_material_expression(self.m, cls, x, y)
        for k, v in props.items():
            set_quiet(e, k, v, f"{self.name}.{getattr(cls, '__name__', cls)}")
        return e

    def link(self, src, dst, pin=""):
        e, out = src if isinstance(src, tuple) else (src, "")
        if not MEL.connect_material_expressions(e, out, dst, pin):
            warn(f"{self.name}: could not connect {e.get_class().get_name()}[{out}] to "
                 f"{dst.get_class().get_name()}[{pin}]")

    def pins(self, node, items):
        for pin, v, const in items:
            if v is None:
                continue
            if isinstance(v, (int, float)):
                if const:
                    set_quiet(node, const, float(v), f"{self.name}.{node.get_class().get_name()}")
                else:
                    self.link(self.const(v), node, pin)
            else:
                self.link(v, node, pin)
        return node

    # arithmetic
    def binary(self, cls, a, b):
        return self.pins(self.node(cls), [("A", a, "const_a"), ("B", b, "const_b")])

    def add(self, a, b):
        return self.binary(unreal.MaterialExpressionAdd, a, b)

    def sub(self, a, b):
        return self.binary(unreal.MaterialExpressionSubtract, a, b)

    def mul(self, a, b):
        return self.binary(unreal.MaterialExpressionMultiply, a, b)

    def div(self, a, b):
        return self.binary(unreal.MaterialExpressionDivide, a, b)

    def min(self, a, b):
        return self.binary(unreal.MaterialExpressionMin, a, b)

    def max(self, a, b):
        return self.binary(unreal.MaterialExpressionMax, a, b)

    def append(self, a, b):
        return self.pins(self.node(unreal.MaterialExpressionAppendVector), [("A", a, None), ("B", b, None)])

    def lerp(self, a, b, t):
        return self.pins(self.node(unreal.MaterialExpressionLinearInterpolate),
                         [("A", a, "const_a"), ("B", b, "const_b"), ("Alpha", t, "const_alpha")])

    def unary(self, cls, x, **props):
        n = self.node(cls, **props)
        self.link(x, n, "")
        return n

    def frac(self, x):
        return self.unary(unreal.MaterialExpressionFrac, x)

    def floor(self, x):
        return self.unary(unreal.MaterialExpressionFloor, x)

    def abs(self, x):
        return self.unary(unreal.MaterialExpressionAbs, x)

    def sat(self, x):
        return self.unary(unreal.MaterialExpressionSaturate, x)

    def one_minus(self, x):
        return self.unary(unreal.MaterialExpressionOneMinus, x)

    def sin(self, x):
        return self.unary(unreal.MaterialExpressionSine, x, period=6.2831853)

    def mask(self, x, channels):
        return self.unary(unreal.MaterialExpressionComponentMask, x,
                          r="R" in channels, g="G" in channels, b="B" in channels, a="A" in channels)

    def step(self, edge, x):
        """1 where x >= edge."""
        return self.pins(self.node(unreal.MaterialExpressionStep), [("Y", edge, "const_y"), ("X", x, "const_x")])

    def smoothstep(self, lo, hi, v):
        return self.pins(self.node(unreal.MaterialExpressionSmoothStep),
                         [("Min", lo, "const_min"), ("Max", hi, "const_max"), ("Value", v, "const_value")])

    def hash(self, x):
        """A cheap per-cell random value in 0..1 (x: a cell id, not an integer multiple of 2π)."""
        return self.frac(self.mul(self.sin(x), 43758.5453))

    # sources
    def const(self, v):
        return self.node(unreal.MaterialExpressionConstant, r=float(v))

    def scalar(self, name, default, group="", lo=None, hi=None):
        e = self.node(unreal.MaterialExpressionScalarParameter, parameter_name=name, default_value=float(default))
        if group:
            set_quiet(e, "group", group, f"{self.name}.{name}")
        if lo is not None and hi is not None:
            try:
                e.set_editor_property("slider_min", float(lo), QUIET)
                e.set_editor_property("slider_max", float(hi), QUIET)
            except Exception:  # noqa: BLE001 - cosmetic
                pass
        return e

    def vector(self, name, colour, group=""):
        e = self.node(unreal.MaterialExpressionVectorParameter, parameter_name=name, default_value=linear_colour(colour))
        if group:
            set_quiet(e, "group", group, f"{self.name}.{name}")
        return e

    def switch(self, name, default, if_true, if_false, group="Switches"):
        n = self.node(unreal.MaterialExpressionStaticSwitchParameter, parameter_name=name, default_value=bool(default))
        set_quiet(n, "group", group, f"{self.name}.{name}")
        self.pins(n, [("True", if_true, None), ("False", if_false, None)])
        return n

    def texture(self, name, default, uv, group="Texture"):
        e = self.node(unreal.MaterialExpressionTextureSampleParameter2D, parameter_name=name)
        if default:
            set_quiet(e, "texture", default, f"{self.name}.{name}")
        st = enum_value(getattr(unreal, "MaterialSamplerType", None), "SAMPLERTYPE_COLOR")
        if st is not None:
            set_quiet(e, "sampler_type", st, f"{self.name}.{name}")
        set_quiet(e, "group", group, f"{self.name}.{name}")
        self.link(uv, e, "UVs")
        return e

    def noise(self, position, levels=3, function="tex3d", out_min=-1.0, out_max=1.0, scale=1.0, level_scale=2.0):
        n = self.node(unreal.MaterialExpressionNoise)
        fn = enum_value(getattr(unreal, "NoiseFunction", None), *NOISE_FUNCTIONS[function])
        if fn is not None:
            set_quiet(n, "noise_function", fn, f"{self.name}.Noise")
        for k, v in (("scale", float(scale)), ("levels", int(levels)), ("quality", 1), ("turbulence", False),
                     ("output_min", float(out_min)), ("output_max", float(out_max)),
                     ("level_scale", float(level_scale)), ("tiling", False)):
            set_quiet(n, k, v, f"{self.name}.Noise")
        if position is not None:
            self.link(position, n, "")   # Position is the first input
        return n

    def output(self, **attributes):
        """Everything goes through Make Material Attributes (the only Python route to clear coat)."""
        mk = self.node(unreal.MaterialExpressionMakeMaterialAttributes)
        self.pins(mk, [(pin, v, None) for pin, v in attributes.items()])
        if not MEL.connect_material_property(mk, "", unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES):
            warn(f"{self.name}: could not connect the material attributes")
        return mk


def world_metres(g):
    """
    Absolute world position in metres as plain floats: frac() demotes Unreal's large-world
    coordinates, with a ±512 m window around the origin (the museum is far smaller).
    """
    wp = g.node(unreal.MaterialExpressionWorldPosition)
    return g.sub(g.mul(g.frac(g.add(g.mul(wp, 0.01 / 1024.0), 0.5)), 1024.0), 512.0)


def dither(g):
    """A per-pixel random number in 0..1 that changes every frame (interleaved gradient noise): the upscaler's
    temporal filter averages what it picks, as it averages the jittered samples it is built on. Constant in
    ray-traced hits (they have no pixel), which then take one choice."""
    sp = g.node(unreal.MaterialExpressionScreenPosition)
    frame = g.floor(g.mul(g.frac(g.mul(g.node(unreal.MaterialExpressionTime), 60.0 / 64.0)), 64.0))   # 60 patterns a second
    pix = g.add((sp, "PixelPosition"), g.mul(frame, 5.588238))
    dot = g.add(g.mul(g.mask(pix, "R"), 0.06711056), g.mul(g.mask(pix, "G"), 0.00583715))
    return g.frac(g.mul(g.frac(dot), 52.9829189))


def box_projection(g, p, grid_offset=None, sharpness=8.0):
    """
    The world projection every master uses: u runs along the course (horizontal, along the wall), v up the wall,
    both in metres; floors and ceilings use x and y. The axis follows the vertex normal. Where two axes meet on a
    curved face (a drum, a cove, a moulding, a basin) each pixel takes one of them at random, weighted |n|^sharpness
    (ProjectionSharpness), and the temporal filter blends them: no hard seam where the axis changes. (A hard switch at
    |n.z| = 0.7 or at 45° cut the oval's cove facet by facet into a dashed line, and every drum four times.)
    Returns {u, v, floor, facing_x, t_u, t_v}: t_u, t_v the world directions of +u and +v.
    """
    px, py, pz = g.mask(p, "R"), g.mask(p, "G"), g.mask(p, "B")
    n = g.node(unreal.MaterialExpressionVertexNormalWS)
    sharp = g.scalar("ProjectionSharpness", sharpness, "Projection", 1, 64)
    w = g.pins(g.node(unreal.MaterialExpressionPower), [("Base", g.max(g.abs(n), 1e-4), None), ("Exp", sharp, None)])
    wx, wy, wz = g.mask(w, "R"), g.mask(w, "G"), g.mask(w, "B")
    pick = g.mul(dither(g), g.add(g.add(wx, wy), wz))
    floor = g.step(pick, wz)                                            # 1: the floor / ceiling axis
    facing_x = g.mul(g.one_minus(floor), g.step(g.sub(pick, wz), wx))    # 1: the wall faces ±x (it runs along y)
    u_wall = g.lerp(px, py, facing_x)
    if grid_offset is not None:
        px, py = g.add(px, grid_offset), g.add(py, grid_offset)
    u = g.lerp(u_wall, px, floor)
    v = g.lerp(pz, py, floor)
    x_ax, y_ax, z_ax = pbr.vec3(g, 1, 0, 0), pbr.vec3(g, 0, 1, 0), pbr.vec3(g, 0, 0, 1)
    t_u = g.lerp(g.lerp(x_ax, y_ax, facing_x), x_ax, floor)
    t_v = g.lerp(z_ax, y_ax, floor)
    return {"u": u, "v": v, "floor": floor, "facing_x": facing_x, "t_u": t_u, "t_v": t_v, "p": p, "n": n}


# The floor levels (m) the museum's rooms stand on: the Square (-9), the Classical Hall (-6), the Reserve (-5.8), the
# ground floor (0), the Sphere's platform (20.42). Height above the level below a point = how far up a wall it is.
FLOOR_LEVELS = (-9.0, -6.0, -5.8, 0.0, 20.42)


def height_above_floor(g, z):
    level = g.const(FLOOR_LEVELS[0] - 100.0)
    for lv in FLOOR_LEVELS:
        level = g.lerp(level, float(lv), g.step(lv - 0.03, z))
    return g.sub(z, level)


def grime(g, proj, c, rough, foot_default=0.0, dust_default=0.0):
    """
    Where a real room gets dirty (the audit, 2.5): the foot of every wall, plinth and pier (0-40 cm, darker from the
    mop and shoes, broken up), and dust on surfaces that face up above the floor (ledges, plinth tops, cornices).
    Returns (colour, roughness)."""
    n = proj["n"]
    z = g.mask(proj["p"], "B")
    h = height_above_floor(g, z)
    nz = g.mask(n, "B")
    vertical = g.one_minus(g.smoothstep(0.35, 0.7, g.abs(nz)))
    breakup = g.noise(g.div(proj["p"], 0.23), levels=2, out_min=0.0, out_max=1.0)
    foot = g.mul(g.mul(g.one_minus(g.smoothstep(0.0, 0.4, g.add(h, g.mul(breakup, 0.08)))), vertical),
                 g.scalar("FootDirt", foot_default, "Grime", 0, 0.3))
    c = g.mul(c, g.one_minus(foot))
    up = g.mul(g.mul(g.smoothstep(0.6, 0.9, nz), g.smoothstep(0.25, 0.4, h)), g.add(0.6, g.mul(breakup, 0.4)))
    dust = g.mul(up, g.scalar("DustUp", dust_default, "Grime", 0, 0.3))
    c = g.lerp(c, pbr.vec3(g, 0.42, 0.40, 0.37), dust)
    rough = g.sat(g.add(g.add(rough, g.mul(foot, 0.6)), g.mul(dust, 2.5)))
    return c, rough


def waviness(g, proj, scale_default, degrees_default, group="Surface"):
    """The macro normal of a real surface: slow undulation (a honed slab's 0.02-0.08° over half a metre; trowelled plaster
    1-3 mm over 20-50 cm; hand-laid brick). One fetch of the library's WV map (pbr_fetch.make_waviness: a tileable random
    height field, four wavelengths across the tile, its slope normalised) laid in the projection's plane, so Waviness is
    the RMS slope in degrees and WaveScale the wavelength in metres. Returns (world offset vector, a matching -1..1 value
    for a roughness wobble). Behind the Macro switch: no cost where off."""
    wv = pbr.textures("WV").get("Normal")
    uv = g.div(g.append(proj["u"], g.mul(proj["v"], -1.0)), g.mul(g.scalar("WaveScale", scale_default, group, 0.05, 5), 4.0))
    s_ = pbr.sample(g, "WaveNormal", wv, uv, "SAMPLERTYPE_NORMAL")
    a_ = g.mask(s_, "R")
    b_ = g.mul(g.mask(s_, "G"), -1.0)                   # DirectX green points down the image: -v
    in_plane = g.add(g.mul(proj["t_u"], a_), g.mul(proj["t_v"], b_))
    tilt = g.mul(in_plane, g.mul(g.scalar("Waviness", degrees_default, group, 0, 3), 0.01745 * 4.0))
    return tilt, g.mul(a_, 4.0)


# --------------------------------------------------------------------------- masters

def clear_graph(m):
    """Every expression out (delete_all_material_expressions leaves some behind in 5.8)."""
    for _ in range(10):
        if MEL.get_num_material_expressions(m) == 0:
            break
        MEL.delete_all_material_expressions(m)
        for e in list(MEL.get_material_expressions(m)):
            MEL.delete_material_expression(m, e)
    left = MEL.get_num_material_expressions(m)
    if left:
        warn(f"{m.get_name()}: {left} old expressions could not be deleted")
    for prop in (unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES, unreal.MaterialProperty.MP_BASE_COLOR):
        try:
            MEL.disconnect_material_property(m, prop)
        except Exception:  # noqa: BLE001
            pass


def master(name, shading_model):
    path = f"{P.MATERIALS}/{name}"
    if EAL.does_asset_exist(path):
        m = unreal.load_asset(path)
        if not isinstance(m, unreal.Material):
            raise RuntimeError(f"{path} exists and is not a Material")
        clear_graph(m)
        fresh = False
    else:
        m = TOOLS.create_asset(name, P.MATERIALS, unreal.Material, unreal.MaterialFactoryNew())
        fresh = True
    # Each property change recompiles the material, and a compile of a half-built graph evaluates
    # the old graph's cached parameters: only what differs is set (on a re-run, nothing is).
    # normal_curvature_to_roughness: geometric specular antialiasing. A highlight on a curve thinner than a pixel (a
    # gilt rosette, a fillet, a rail's top, a column's arris) jumped from pixel to pixel with the upscaler's jitter
    # (the flicker check's hot spots); the curvature under the pixel now widens it into the roughness it really spans.
    for prop, value in (("blend_mode", unreal.BlendMode.BLEND_OPAQUE), ("two_sided", False),
                        ("shading_model", shading_model), ("use_material_attributes", True),
                        ("normal_curvature_to_roughness", True)):
        try:
            if m.get_editor_property(prop) == value:
                continue
        except Exception:  # noqa: BLE001
            pass
        set_prop(m, prop, value)
    if not fresh:
        return m          # the usages below persist from the first build
    for usage in ("MATUSAGE_NANITE", "MATUSAGE_STATIC_MESH", "MATUSAGE_INSTANCED_STATIC_MESHES"):
        u = enum_value(getattr(unreal, "MaterialUsage", None), usage)
        if u is not None:
            try:
                MEL.set_base_material_usage(m, u, True)
            except Exception as e:  # noqa: BLE001
                warn(f"{name}: usage {usage} not set ({e})")
    return m


def shader_stats(material):
    """(pixel instructions, samplers), compiling if needed; (0, 0) when this process compiles no
    shaders (a commandlet without -AllowCommandletRendering)."""
    try:
        s = MEL.get_statistics(material)
        return s.num_pixel_shader_instructions, s.num_samplers
    except Exception:  # noqa: BLE001
        return 0, 0


def finish(m):
    try:
        MEL.layout_material_expressions(m)
    except Exception:  # noqa: BLE001 - cosmetic
        pass
    if MEL.get_material_property_input_node(m, unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES) is None:
        warn(f"{m.get_name()}: nothing drives the material attributes")
    # A property change refreshes the material's cached expression data (its referenced textures)
    # before the compile; otherwise the first compile of a rebuilt graph looks up stale textures.
    set_prop(m, "use_material_attributes", True)
    errors = list(MEL.recompile_material(m) or [])
    for e in errors:
        warn(f"{m.get_name()}: compile error: {e}")
    EAL.save_loaded_asset(m, only_if_is_dirty=False)
    ps, samplers = shader_stats(m)
    if ps:
        log(f"{m.get_name()}: {MEL.get_num_material_expressions(m)} expressions, {ps} pixel-shader instructions "
            f"(base pass, incl. lighting), {samplers} samplers, {len(errors)} errors")
    else:
        log(f"{m.get_name()}: {MEL.get_num_material_expressions(m)} expressions, {len(errors)} errors (no instruction "
            f"count: shaders aren't compiled in a commandlet without -AllowCommandletRendering)")


# Honed travertine, the masters' defaults: warm ivory (the Main board's travertine #E9DFCD, a shade
# down so Lumen's bounce doesn't wash the rooms out).
TRAVERTINE = 0xE6DDCF
BRICK_BUFF = 0xB49C7E          # the spec's buff brick (#B89A76) at its saturation cap, 0.30
TRAVERTINE_POLISHED = 0xE8E0D2
MARBLE = 0xEEECE7
MARBLE_VEIN = 0xA9A49B


def build_stone(tex_default, name="M_Stone", relief=False, sss=False):
    """M_Stone; M_StoneRelief is the same stone with Nanite tessellation displacing the photograph's height
    (brick with recessed joints, pebbles, gravel, rock).

    What makes one slab of real stone read differently from the next (the photoreal audit, 2.1-2.10), each a parameter:
    - joints that show in the light, not only in the colour: a chamfered arris each side (JointBevel, a highlight and
      a shadow line under grazing light), a groove with no polish (JointRough, the coat off) and no sky in it (JointAO),
      still legible at a distance (JointMinVisible);
    - each slab its own tone and hue (BlockTone, BlockHue), and its own tilt (SlabTilt, degrees: the reflections kink at
      every joint), with a slow waviness inside it (Waviness over WaveScale, the Macro switch);
    - polish that varies (RoughWear: a 0.5 m and a 5 m pattern), and a duller trafficked floor (TrafficWear);
    - the travertine's voids (the photograph's 1 - AO): open and dark on walls (VoidOpen, VoidDarkness), filled with a
      duller filler on floors (FillerRough);
    - grime where it collects (FootDirt, DustUp), and the stone's own reflectance (Specular 0.6 = calcite, F0 0.048).
    M_StoneSSS (sss) is the statuary marble: light enters it and comes out warmer and softer (Jensen's marble: a mean
    free path of 0.85 / 0.56 / 0.40 cm in R / G / B), the subsurface model in place of the coat.
    """
    m = master(name, unreal.MaterialShadingModel.MSM_SUBSURFACE if sss else unreal.MaterialShadingModel.MSM_CLEAR_COAT)
    g = Graph(m)
    p = world_metres(g)
    proj = box_projection(g, p, g.scalar("GridOffset", 0.6, "Coursing"), sharpness=16.0)
    u, v, floor = proj["u"], proj["v"], proj["floor"]
    # A pixel's width on the surface in metres (the geometric mean of the position's screen derivatives: right at any
    # resolution and field of view; the old depth x 0.0008 made the joints 2.4 times too thin at 4K, where they shimmered).
    fx = g.unary(unreal.MaterialExpressionLength, g.unary(unreal.MaterialExpressionDDX, p))
    fy = g.unary(unreal.MaterialExpressionLength, g.unary(unreal.MaterialExpressionDDY, p))
    pixel = g.unary(unreal.MaterialExpressionSquareRoot, g.mul(fx, fy))

    # Coursing: rows of CourseHeight up the wall (FloorSlab on floors); blocks along the row.
    course = g.scalar("CourseHeight", 0.6, "Coursing", 0.05, 1.5)
    slab = g.scalar("FloorSlab", 1.2, "Coursing", 0.05, 3.0)
    lv = g.lerp(course, slab, floor)
    vr = g.add(g.div(v, lv), g.scalar("CourseOffset", 0.0, "Coursing"))
    row = g.floor(vr)
    fv = g.frac(vr)
    row_rand = g.hash(g.mul(row, 91.3458))
    jitter = g.mul(g.sub(row_rand, 0.5), g.mul(g.scalar("BlockJitter", 0.33, "Coursing", 0, 0.6), 2.0))
    lu_wall = g.mul(g.scalar("BlockLength", 1.8, "Coursing", 0.1, 4.0), g.add(jitter, 1.0))
    # Floors: FloorSlab rows, slabs FloorAspect times as long (1 = square; 3 = brick on a soffit).
    lu = g.lerp(lu_wall, g.mul(slab, g.scalar("FloorAspect", 1.0, "Coursing", 0.5, 4)), floor)
    shift_wall = g.add(g.mul(row, g.scalar("Stagger", 0.5, "Coursing", 0, 1)),
                       g.mul(row_rand, g.scalar("StaggerJitter", 0.2, "Coursing", 0, 1)))
    shift = g.lerp(shift_wall, g.mul(row, g.scalar("FloorStagger", 0.0, "Coursing", 0, 1)), floor)
    ur = g.add(g.div(u, lu), shift)
    col = g.floor(ur)
    fu = g.frac(ur)
    du = g.mul(g.min(fu, g.one_minus(fu)), lu)          # metres to the nearest head joint
    dv = g.mul(g.min(fv, g.one_minus(fv)), lv)          # … and to the nearest bed joint
    d = g.min(du, dv)

    # Joints: a groove JointWidth wide, widened to a pixel and a half with distance so it never shimmers, and kept
    # legible (at least JointMinVisible of its strength) where it is thinner than that: dirt and shadow keep a real
    # joint visible from across a room.
    joints_on = g.lerp(g.scalar("WallJoints", 1.0, "Joints", 0, 1), g.scalar("FloorJoints", 1.0, "Joints", 0, 1), floor)
    width = g.scalar("JointWidth", 0.003, "Joints", 0.0005, 0.02)
    width_seen = g.max(width, g.mul(pixel, 2.0))       # never under two pixels: a thinner line flickers with the jitter
    half = g.mul(width_seen, 0.5)
    seen = g.max(g.div(width, width_seen), g.scalar("JointMinVisible", 0.6, "Joints", 0, 1))
    joint = g.mul(g.mul(g.one_minus(g.smoothstep(g.mul(half, 0.35), half, d)), seen), joints_on)
    cell_rand = g.hash(g.add(g.add(g.mul(col, 78.233), g.mul(row, 12.9898)), 0.5))
    cell = g.mul(cell_rand, joints_on)      # a block's own seed; none where there are no joints
    cell_rand2 = g.frac(g.add(g.mul(cell_rand, 13.37), 0.21))
    cell_rand3 = g.frac(g.add(g.mul(cell_rand, 7.919), 0.53))

    # Large-scale drift of tone and polish.
    macro = g.noise(g.div(p, g.scalar("MacroScale", 5.0, "Variation", 0.5, 20)), levels=3)

    # Strata along the courses (travertine), or veins (marble; FloorVeinScale on floors). Each block
    # takes its own slice of the noise.
    along = g.lerp(g.scalar("StrataAlong", 1.6, "Strata", 0.05, 5), g.scalar("FloorVeinScale", 0.7, "Strata", 0.05, 5), floor)
    across = g.lerp(g.scalar("StrataAcross", 0.04, "Strata", 0.005, 5), g.scalar("FloorVeinScale", 0.7, "Strata", 0.05, 5), floor)
    strata = g.noise(g.append(g.append(g.div(u, along), g.div(v, across)), g.mul(cell, 7.13)), levels=4)

    # Pores: small, stretched along the strata, faded to their average tone with distance.
    pore_size = g.scalar("PoreSize", 0.006, "Pores", 0.001, 0.05)
    pore_along = g.mul(pore_size, g.lerp(g.scalar("PoreStretch", 4.0, "Pores", 1, 10), 1.5, floor))
    pn = g.noise(g.append(g.append(g.div(u, pore_along), g.div(v, pore_size)), g.add(g.mul(cell, 3.7), 0.37)), levels=1)
    pore_amount = g.scalar("PoreAmount", 0.5, "Pores", 0, 1)
    pore_near = g.mul(g.sat(g.mul(g.sub(pn, g.scalar("PoreThreshold", 0.4, "Pores", -1, 1)), 8.0)), pore_amount)
    pore = g.lerp(pore_near, g.mul(pore_amount, 0.06), g.smoothstep(0.0032, 0.012, pixel))

    # Colour: the macro drift, each block its own tone and a breath of hue (BlockHue: warmer or cooler by up to
    # ±BlockHue in red against blue), the strata.
    tone = g.add(g.add(g.add(1.0, g.mul(macro, g.scalar("MacroVariation", 0.05, "Variation", 0, 0.5))),
                       g.mul(g.mul(g.sub(cell_rand, 0.5), g.scalar("BlockTone", 0.05, "Variation", 0, 0.4)), joints_on)),
                 g.mul(strata, g.scalar("StrataAmount", 0.07, "Strata", 0, 0.5)))
    hue = g.mul(g.mul(g.mul(g.sub(cell_rand3, 0.5), 2.0), g.scalar("BlockHue", 0.0, "Variation", 0, 0.2)), joints_on)
    hue_tint = g.append(g.append(g.add(1.0, hue), 1.0), g.sub(1.0, hue))
    # AltColor: each block (brick) leans towards a second colour by its own amount.
    base = g.lerp(g.vector("BaseColor", lin(TRAVERTINE), "Colour"), g.vector("AltColor", lin(0x8E4E38), "Colour"),
                  g.mul(g.mul(cell_rand2, g.scalar("AltAmount", 0.0, "Colour", 0, 1)), joints_on))
    c = g.mul(g.mul(base, tone), hue_tint)
    vein = g.mul(g.sat(g.one_minus(g.mul(g.abs(strata), g.scalar("VeinSharpness", 18.0, "Strata", 1, 60)))),
                 g.scalar("VeinAmount", 0.0, "Strata", 0, 1))
    c = g.lerp(c, g.vector("VeinColor", lin(MARBLE_VEIN), "Colour"), vein)
    c = g.lerp(c, g.mul(c, g.scalar("PoreDarkness", 0.72, "Pores", 0, 1.5)), pore)
    # Joints: darker stone, or a mortar colour (JointColorAmount 1, brick).
    joint_col = g.lerp(g.mul(c, g.scalar("JointDarkness", 0.86, "Joints", 0, 2)),
                       g.vector("JointColor", lin(0xC9B8A6), "Joints"), g.scalar("JointColorAmount", 0.0, "Joints", 0, 1))
    c = g.lerp(c, joint_col, joint)

    # Optional image (UV0, in metres per TexScale): brick, rammed earth, the sun clock, ceramics.
    tex_scale = g.scalar("TexScale", 1.0, "Texture", 0.01, 10)
    uv = g.div(g.node(unreal.MaterialExpressionTextureCoordinate),
               g.append(tex_scale, g.mul(tex_scale, g.scalar("TexAspect", 1.0, "Texture", 0.1, 10))))
    # TexBlockShift: each block (between the joints) takes its own slice of the image, as ashlar cut from one
    # quarry does, so a photographed stone doesn't repeat block after block.
    uv = g.add(uv, g.mul(g.append(cell_rand, cell_rand2), g.mul(g.scalar("TexBlockShift", 0.0, "Texture", 0, 1), joints_on)))
    tex = g.texture("Texture", tex_default, uv)
    c = g.switch("UseTexture", False, g.mul(c, (tex, "RGB")), c)

    # Polish: the base, the macro drift, the pores, and RoughWear's two scales of wear (0.5 m patches, 5 m drift).
    rv = g.scalar("RoughnessVariation", 0.06, "Surface", 0, 0.3)
    wear_fine = g.noise(g.div(p, 0.45), levels=2)
    wear = g.mul(g.add(g.mul(wear_fine, 0.6), g.mul(macro, 0.4)), g.scalar("RoughWear", 0.0, "Surface", 0, 0.4))
    # Traffic: the floor's middle stretches duller (a lane's worth of the 5 m pattern).
    traffic = g.mul(g.mul(g.smoothstep(-0.15, 0.5, macro), floor), g.scalar("TrafficWear", 0.0, "Surface", 0, 1))
    rough = g.add(g.add(g.add(g.scalar("Roughness", 0.62, "Surface", 0, 1), g.mul(macro, rv)), g.mul(pore, 0.25)),
                  g.add(wear, g.mul(traffic, 0.12)))
    rough = g.sat(g.add(rough, g.mul(joint, g.scalar("JointRough", 0.35, "Joints", 0, 1))))

    # Inlay: the image's saturated (bronze) lines become metal. Everything that reads the image
    # sits behind the switches, so the plain stone permutation samples no texture at all.
    tr, tb = (tex, "R"), (tex, "B")
    inlay_mask = g.sat(g.mul(g.sub(g.div(g.sub(tr, tb), g.max(tr, 0.02)), 0.45), 5.0))
    c = g.switch("Inlay", False, g.lerp(c, g.sat(g.mul((tex, "RGB"), g.scalar("InlayBoost", 1.8, "Texture", 1, 4))), inlay_mask), c)
    rough = g.switch("Inlay", False, g.lerp(rough, g.scalar("InlayRoughness", 0.22, "Texture", 0, 1), inlay_mask), rough)
    inlay = g.switch("Inlay", False, inlay_mask, 0.0)

    # The photographed layer (pbr.py): colour variation, relief, polish, occlusion; each block its own slice.
    photo = pbr.layer(g, proj, cells=(cell_rand, cell_rand2, joints_on), relief=relief)
    c = g.mul(c, photo["colour"])
    # The stone's voids (1 - AO of the photograph): open on walls (dark: the light doesn't get in), filled on floors
    # (the filler's duller polish shows through the coat).
    void = g.sat(g.mul(g.one_minus(photo["ao_raw"]), 1.43))
    void_open = g.mul(g.scalar("VoidOpen", 0.0, "Voids", 0, 1), g.one_minus(floor))      # floors are filled
    c = g.lerp(c, g.mul(c, g.scalar("VoidDarkness", 0.55, "Voids", 0, 1)), g.mul(void, void_open))
    filler = g.mul(g.mul(void, g.scalar("FillerRough", 0.0, "Voids", 0, 0.5)), g.one_minus(void_open))
    rough = g.sat(g.add(g.add(rough, photo["rough"]), filler))

    # The coat: off in the joints, the pores and the filler, thinner where the floor is trafficked.
    clear = g.mul(g.scalar("ClearCoat", 0.0, "Surface", 0, 1),
                  g.mul(g.mul(g.one_minus(g.max(joint, g.mul(pore, 0.6))), g.one_minus(g.mul(filler, 4.0))),
                        g.one_minus(g.mul(traffic, 0.35))))
    clear_rough = g.sat(g.add(g.add(g.scalar("ClearCoatRoughness", 0.06, "Surface", 0, 1), g.mul(macro, g.mul(rv, 0.3))),
                              g.add(g.mul(photo["rough"], 0.5), g.add(g.mul(wear, 0.6), g.mul(traffic, 0.1)))))
    c, rough = grime(g, proj, c, rough)

    # The macro normal: each slab tilted (SlabTilt degrees, a random direction in its plane), a slow waviness inside it,
    # and the arrises of the joints chamfered (JointBevel wide, faded out once the chamfer is under a pixel and a half).
    tilt_amt = g.mul(g.scalar("SlabTilt", 0.0, "Surface", 0, 2), 0.01745)
    tilt = g.mul(g.add(g.mul(proj["t_u"], g.sub(cell_rand2, 0.5)), g.mul(proj["t_v"], g.sub(cell_rand3, 0.5))),
                 g.mul(g.mul(tilt_amt, 2.0), joints_on))
    wave, wave_n = waviness(g, proj, 0.5, 0.0)
    rough = g.switch("Macro", False, g.sat(g.add(rough, g.mul(wave_n, g.mul(g.scalar("RoughWear", 0.0, "Surface", 0, 0.4), 0.5)))), rough)
    bevel = g.scalar("JointBevel", 0.0, "Joints", 0, 0.02)
    near_u = g.step(du, dv)                                  # 1: the nearest joint is a head joint (across u)
    toward = g.lerp(g.mul(proj["t_v"], g.sub(g.mul(g.step(0.5, fv), 2.0), 1.0)),
                    g.mul(proj["t_u"], g.sub(g.mul(g.step(0.5, fu), 2.0), 1.0)), near_u)
    in_arris = g.mul(g.step(half, d), g.one_minus(g.smoothstep(g.mul(bevel, 0.6), g.add(bevel, 0.0005), g.sub(d, half))))
    arris_fade = g.sat(g.div(bevel, g.mul(pixel, 1.5)))
    arris = g.mul(toward, g.mul(g.mul(g.mul(in_arris, arris_fade), joints_on), 0.8))
    macro_n = g.add(tilt, arris)
    macro_n = g.switch("Macro", False, g.add(macro_n, wave), macro_n)
    normal = g.unary(unreal.MaterialExpressionNormalize, g.add(photo["normal"], g.mul(macro_n, photo["side"])))
    ao = g.mul(photo["ao"], g.lerp(1.0, g.scalar("JointAO", 0.4, "Joints", 0, 1), joint))

    pbr.world_normal_material(m)
    out = dict(BaseColor=c, Metallic=inlay, Roughness=rough, ClearCoat=clear, ClearCoatRoughness=clear_rough,
               Specular=g.scalar("Specular", 0.5, "Surface", 0, 1), Normal=normal, AmbientOcclusion=ao)
    if relief:
        pbr.relief_material(m)
        out["Displacement"] = photo["height"]
    if sss:
        del out["ClearCoat"], out["ClearCoatRoughness"]
        out["SubsurfaceColor"] = g.mul(c, g.vector("SSSColor", (1.0, 0.82, 0.66), "Subsurface"))
        out["Opacity"] = g.scalar("SSSOpacity", 0.45, "Subsurface", 0, 1)      # lower: the light travels further in
    g.output(**out)
    finish(m)
    return m


def build_metal(tex_default):
    m = master("M_Metal", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    g = Graph(m)
    p = world_metres(g)
    n = g.noise(g.div(p, g.scalar("NoiseScale", 0.25, "Variation", 0.01, 5)), levels=4)
    var = g.scalar("Variation", 0.1, "Variation", 0, 0.5)
    c = g.mul(g.vector("BaseColor", lin(0xD6AE68), "Colour"), g.add(g.mul(n, var), 1.0))
    patina = g.mul(g.smoothstep(0.1, 0.55, g.mul(n, -1.0)), g.scalar("PatinaAmount", 0.0, "Patina", 0, 1))
    c = g.lerp(c, g.vector("PatinaColor", lin(0x3F4636), "Patina"), patina)
    tex_scale = g.scalar("TexScale", 1.0, "Texture", 0.01, 10)
    uv = g.div(g.node(unreal.MaterialExpressionTextureCoordinate),
               g.append(tex_scale, g.mul(tex_scale, g.scalar("TexAspect", 1.0, "Texture", 0.1, 10))))
    tex = g.texture("Texture", tex_default, uv)
    c = g.switch("UseTexture", False, g.mul(c, (tex, "RGB")), c)
    metallic = g.mul(g.scalar("Metallic", 1.0, "Surface", 0, 1), g.one_minus(g.mul(patina, 0.6)))
    rough = g.sat(g.add(g.add(g.scalar("Roughness", 0.3, "Surface", 0, 1), g.mul(n, g.mul(var, 0.5))), g.mul(patina, 0.35)))
    proj = box_projection(g, p)
    photo = pbr.layer(g, proj)
    c = g.mul(c, photo["colour"])
    rough = g.sat(g.add(rough, photo["rough"]))
    # A patinated set's own metal mask (PBRMetalAmount): bare metal where its metalness is, patina elsewhere.
    metallic = g.lerp(metallic, g.mul(metallic, photo["metal"]), g.scalar("PBRMetalAmount", 0.0, "Photo", 0, 1))
    # Brushed metal: the scratches run along the photograph's +u (the brushing on UV0), so its highlight is drawn out
    # across them (Anisotropy < 0 stretches it along the bitangent). Brushed bronze -0.6, gilt and patina 0.
    pbr.world_normal_material(m)
    g.output(BaseColor=c, Metallic=metallic, Roughness=rough, Normal=photo["normal"], AmbientOcclusion=photo["ao"],
             Anisotropy=g.scalar("Anisotropy", 0.0, "Surface", -1, 1), Tangent=photo["tangent"])
    finish(m)
    return m


def build_fabric():
    # Cloth: a fuzz layer of fibres over the weave (Substrate's Fuzz; the legacy Cloth model without it), in place of
    # a Fresnel glow painted into the colour.
    m = master("M_Fabric", unreal.MaterialShadingModel.MSM_CLOTH)
    g = Graph(m)
    p = world_metres(g)
    proj = box_projection(g, p)
    u, v = proj["u"], proj["v"]
    pixel = g.mul(g.node(unreal.MaterialExpressionPixelDepth), 0.01 * 0.0008)   # ≈ a pixel's width in metres
    thread = g.scalar("ThreadSize", 0.0025, "Weave", 0.0005, 0.02)
    # Slubs: horizontal threads of uneven thickness (shantung silk). The third coordinate drifts so
    # the noise's 16-unit repeat never lines up.
    slub_pos = g.append(g.append(g.div(u, g.scalar("SlubLength", 0.5, "Weave", 0.05, 3)), g.div(v, thread)),
                        g.add(g.mul(u, 0.05), g.mul(v, 3.1)))
    slub = g.noise(slub_pos, levels=2)
    # The weave: warp and weft threads (noise stretched along each), faded out before a thread
    # gets smaller than a pixel (no moiré); a linen mottle carries the texture further away.
    long_thread = g.mul(thread, 8.0)
    warp = g.noise(g.append(g.append(g.div(u, long_thread), g.div(v, thread)), 0.19), levels=1)
    weft = g.noise(g.append(g.append(g.div(u, thread), g.div(v, long_thread)), 0.73), levels=1)
    weave_fade = g.one_minus(g.smoothstep(g.mul(thread, 0.35), g.mul(thread, 1.2), pixel))
    weave = g.mul(g.mul(g.add(warp, weft), 0.5), weave_fade)
    mottle = g.noise(g.div(p, g.scalar("MottleSize", 0.02, "Weave", 0.002, 0.2)), levels=2)
    tone = g.add(g.add(g.add(1.0, g.mul(slub, g.scalar("SlubAmount", 0.08, "Weave", 0, 0.5))),
                       g.mul(weave, g.scalar("WeaveAmount", 0.08, "Weave", 0, 0.5))),
                 g.mul(mottle, g.scalar("MottleAmount", 0.04, "Weave", 0, 0.3)))
    c = g.mul(g.vector("BaseColor", lin(0x7A3B2E), "Colour"), tone)
    # The fibres: SheenAmount x 2 of fuzz (silk 0.5, velvet cord 0.9, felt 0.8, leather 0.1), coloured between the
    # cloth and SheenTint (the light a fibre scatters is paler than the dyed weave).
    fuzz = g.sat(g.mul(g.scalar("SheenAmount", 0.3, "Sheen", 0, 1), 2.0))
    fuzz_colour = g.lerp(c, g.vector("SheenTint", (0.9, 0.87, 0.82), "Sheen"), 0.5)
    rough = g.sat(g.sub(g.sub(g.scalar("Roughness", 0.62, "Surface", 0, 1), g.mul(slub, 0.06)), g.mul(weave, 0.05)))
    photo = pbr.layer(g, proj)
    c = g.mul(c, photo["colour"])
    rough = g.sat(g.add(rough, photo["rough"]))
    pbr.world_normal_material(m)
    g.output(BaseColor=c, Roughness=rough, Specular=g.scalar("Specular", 0.35, "Surface", 0, 1), Metallic=0.0,
             Normal=photo["normal"], AmbientOcclusion=photo["ao"], SubsurfaceColor=fuzz_colour, ClearCoat=fuzz)   # ClearCoat = the Cloth pin
    finish(m)
    return m


def build_plaster():
    m = master("M_Plaster", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    g = Graph(m)
    p = world_metres(g)
    n = g.noise(g.div(p, g.scalar("NoiseScale", 2.0, "Variation", 0.1, 10)), levels=5, level_scale=2.5)
    grain = g.noise(g.div(p, 0.004), levels=1)
    c = g.mul(g.vector("BaseColor", lin(0xEFE8DB), "Colour"),
              g.add(g.add(1.0, g.mul(n, g.scalar("Variation", 0.03, "Variation", 0, 0.3))),
                    g.mul(grain, g.scalar("GrainAmount", 0.02, "Variation", 0, 0.2))))
    rough = g.sat(g.add(g.scalar("Roughness", 0.85, "Surface", 0, 1), g.mul(n, 0.04)))
    # A soft projection blend: plaster has no joints to line up, and its coves and mouldings curve everywhere.
    proj = box_projection(g, p, sharpness=4.0)
    photo = pbr.layer(g, proj)
    c = g.mul(c, photo["colour"])
    rough = g.sat(g.add(rough, photo["rough"]))
    c, rough = grime(g, proj, c, rough)
    # Hand-trowelled lime is never flat: 1-3 mm over 20-50 cm (Waviness degrees, the Macro switch).
    wave, _ = waviness(g, proj, 0.35, 0.0)
    normal = g.switch("Macro", False, g.unary(unreal.MaterialExpressionNormalize, g.add(photo["normal"], g.mul(wave, photo["side"]))),
                      photo["normal"])
    pbr.world_normal_material(m)
    g.output(BaseColor=c, Roughness=rough, Metallic=0.0, Specular=g.scalar("Specular", 0.5, "Surface", 0, 1), Normal=normal,
             AmbientOcclusion=photo["ao"])
    finish(m)
    return m


def build_glaze():
    """
    M_Glaze: Tang sancai lead glaze, after the reference horse (assets/reference/chinese-ref-sancai-horse.jpg): a deep amber
    ground run through with long darker manganese-brown streaks where the glaze flowed down in the kiln; green and cream
    only high on the body (the trappings, the mane), in drips; below GlazeLine the glaze stops in a ragged edge and the
    buff earthenware shows, matt. World space (m), so it needs no UVs (the hand-built models have none to trust); the
    heights are the instance's, for the piece where it stands.
    """
    m = master("M_Glaze", unreal.MaterialShadingModel.MSM_CLEAR_COAT)
    g = Graph(m)
    p = world_metres(g)
    x, y, z = g.mask(p, "R"), g.mask(p, "G"), g.mask(p, "B")
    def drawn(width, length, offset):
        return g.append(g.append(g.div(x, width), g.div(y, width)), g.add(g.div(z, length), offset))
    wobble = g.noise(g.div(p, 0.03), levels=2)
    # The amber, streaked darker.
    streak = g.noise(drawn(g.scalar("StreakWidth", 0.018, "Glaze", 0.002, 0.2), g.scalar("StreakLength", 0.14, "Glaze", 0.01, 1), 0.0), levels=4)
    dark = g.mul(g.smoothstep(0.05, 0.65, g.add(streak, g.mul(wobble, 0.15))), g.scalar("StreakAmount", 0.75, "Glaze", 0, 1))
    tone = g.add(1.0, g.mul(g.noise(g.div(p, 0.12), levels=3), 0.1))
    c = g.lerp(g.mul(g.vector("BaseColor", lin(0xB4661C), "Colour"), tone), g.vector("StreakColor", lin(0x4A200B), "Colour"), dark)
    # Green and cream drips, only above their heights (their edges run down too).
    soft = g.scalar("Bleed", 0.1, "Glaze", 0.01, 0.5)
    def drips(seed, low, threshold, amount):
        n = g.add(g.noise(drawn(0.035, 0.1, seed), levels=3), g.mul(wobble, 0.2))
        high = g.smoothstep(low, g.add(low, 0.06), g.add(z, g.mul(n, 0.05)))
        return g.mul(g.mul(g.smoothstep(threshold, g.add(threshold, soft), n), high), amount)
    green = drips(11.0, g.scalar("GreenLow", 1.3, "Glaze", -10, 100), g.scalar("GreenThreshold", 0.15, "Glaze", -1, 1),
                  g.scalar("GreenAmount", 0.9, "Glaze", 0, 1))
    cream = drips(23.0, g.scalar("CreamLow", 1.45, "Glaze", -10, 100), g.scalar("CreamThreshold", 0.2, "Glaze", -1, 1),
                  g.scalar("CreamAmount", 0.9, "Glaze", 0, 1))
    c = g.lerp(c, g.vector("GreenColor", lin(0x3F6E25), "Colour"), green)
    c = g.lerp(c, g.vector("CreamColor", lin(0xE6D9BC), "Colour"), cream)
    # Where the glaze stops: bare buff earthenware below a ragged line.
    bare = g.one_minus(g.smoothstep(g.scalar("GlazeLine", 1.1, "Glaze", -10, 100),
                                     g.add(g.scalar("GlazeLine", 1.1, "Glaze", -10, 100), 0.015),
                                     g.add(z, g.mul(g.noise(drawn(0.02, 0.03, 5.0), levels=3), 0.035))))
    c = g.lerp(c, g.mul(g.vector("BodyColor", lin(0xD8CCB8), "Colour"), g.add(1.0, g.mul(wobble, 0.06))), bare)
    rough = g.lerp(g.sat(g.add(g.scalar("Roughness", 0.14, "Surface", 0, 1), g.mul(wobble, 0.03))), 0.8, bare)
    clear = g.mul(g.scalar("ClearCoat", 0.7, "Surface", 0, 1), g.one_minus(bare))
    g.output(BaseColor=c, Roughness=rough, Metallic=0.0, ClearCoat=clear,
             ClearCoatRoughness=g.scalar("ClearCoatRoughness", 0.07, "Surface", 0, 1))
    finish(m)
    return m


def build_wood():
    """
    M_Wood: the museum's oak (W2, a flat-sawn European oak veneer, 1.83 m) made up as solid timber is: in boards
    BoardWidth wide across the grain, each its own slice of the veneer (mirrored and shifted), its own tone (BoardTone,
    ±12 % in real oak) and hue (BoardHue), with a hairline glue joint (BoardJoint). The grain runs along the member:
    UV0 on the native furniture and timber (U along the piece), the world projection elsewhere. The veneer's roughness
    carries the open pores (dull in the pore bands, satin between); its normal the grain relief; anisotropy along the
    grain gives the sheen that moves across it (Anisotropy < 0). Oil or wax: Specular 0.45 (F0 0.036). A lacquer is a
    real second layer: ClearCoat (the 栗壳色 chestnut lacquer of the Chinese timber, F0 0.04 over the wood), its own
    ClearCoatRoughness with a little orange peel. The drawn grain (GrainAmount, FigureAmount) stays for instances
    without a photograph.
    """
    m = master("M_Wood", unreal.MaterialShadingModel.MSM_CLEAR_COAT)
    g = Graph(m)
    p = world_metres(g)
    proj = box_projection(g, p)
    u, v = proj["u"], proj["v"]
    pixel = g.mul(g.node(unreal.MaterialExpressionPixelDepth), 0.01 * 0.0008)
    # The boards: across the grain. The grain runs along U on UV0 (along the member), along u in the world projection.
    uv0 = g.node(unreal.MaterialExpressionTextureCoordinate)
    along = g.switch("PBRUseUV0", False, g.mask(uv0, "R"), u, "Photo")
    across = g.switch("PBRUseUV0", False, g.mask(uv0, "G"), v, "Photo")
    bw = g.scalar("BoardWidth", 0.14, "Boards", 0.03, 1.0)
    br = g.div(across, bw)
    board = g.floor(br)
    # Boards end every BoardLength (staggered), as solid timber is jointed.
    bl = g.scalar("BoardLength", 2.4, "Boards", 0.3, 10)
    lr = g.add(g.div(along, bl), g.mul(g.hash(g.mul(board, 17.13)), 1.0))
    piece = g.floor(lr)
    board_rand = g.hash(g.add(g.mul(board, 41.37), g.mul(piece, 7.77)))
    board_rand2 = g.frac(g.add(g.mul(board_rand, 13.37), 0.21))
    board_rand3 = g.frac(g.add(g.mul(board_rand, 5.71), 0.67))
    fb = g.frac(br)
    d = g.min(g.mul(g.min(fb, g.one_minus(fb)), bw), g.mul(g.min(g.frac(lr), g.one_minus(g.frac(lr))), bl))
    joint = g.mul(g.one_minus(g.smoothstep(0.0002, g.max(0.0004, pixel), d)), g.scalar("BoardJoint", 0.0, "Boards", 0, 1))

    gpos = g.append(g.append(g.div(u, g.scalar("GrainAlong", 0.8, "Grain", 0.05, 5)),
                             g.div(v, g.scalar("GrainAcross", 0.012, "Grain", 0.001, 0.2))),
                    g.mul(g.add(u, v), 0.071))
    grain = g.noise(gpos, levels=3)
    figure = g.mul(g.smoothstep(0.75, 1.0, g.frac(g.mul(grain, 3.0))), g.scalar("FigureAmount", 0.3, "Grain", 0, 1))
    macro = g.noise(g.div(p, 1.5), levels=2)
    tone = g.add(g.add(g.add(1.0, g.mul(grain, g.mul(g.scalar("GrainAmount", 0.25, "Grain", 0, 1), 0.5))),
                       g.mul(macro, g.scalar("Variation", 0.06, "Grain", 0, 0.3))),
                 g.mul(g.sub(board_rand, 0.5), g.mul(g.scalar("BoardTone", 0.0, "Boards", 0, 0.5), 2.0)))
    hue = g.mul(g.mul(g.sub(board_rand3, 0.5), 2.0), g.scalar("BoardHue", 0.0, "Boards", 0, 0.2))
    hue_tint = g.append(g.append(g.add(1.0, hue), 1.0), g.sub(1.0, g.mul(hue, 1.5)))
    c = g.mul(g.mul(g.mul(g.vector("BaseColor", lin(0x6B5236), "Colour"), tone), hue_tint), g.one_minus(g.mul(figure, 0.5)))
    c = g.lerp(c, g.mul(c, 0.55), joint)
    rough = g.sat(g.add(g.add(g.add(g.scalar("Roughness", 0.55, "Surface", 0, 1), g.mul(figure, 0.08)), g.mul(macro, 0.03)),
                        g.mul(g.sub(board_rand2, 0.5), 0.08)))
    photo = pbr.layer(g, proj, cells=(board_rand, board_rand2, g.const(1.0)))
    c = g.mul(c, photo["colour"])
    rough = g.sat(g.add(g.add(rough, photo["rough"]), g.mul(joint, 0.3)))
    c, rough = grime(g, proj, c, rough)
    # The lacquer: its own smooth layer (orange peel: a slow ripple in its roughness), off in the joints; worn thinner
    # where hands and seats rub (the macro drift's high side, LacquerWear).
    peel = g.noise(g.div(p, 0.004), levels=1)
    worn = g.mul(g.smoothstep(0.2, 0.7, macro), g.scalar("LacquerWear", 0.0, "Surface", 0, 1))
    clear = g.mul(g.mul(g.scalar("ClearCoat", 0.0, "Surface", 0, 1), g.one_minus(joint)), g.one_minus(worn))
    clear_rough = g.sat(g.add(g.add(g.scalar("ClearCoatRoughness", 0.25, "Surface", 0, 1), g.mul(peel, 0.03)), g.mul(worn, 0.15)))
    ao = g.mul(photo["ao"], g.lerp(1.0, 0.5, joint))
    pbr.world_normal_material(m)
    g.output(BaseColor=c, Roughness=rough, Metallic=0.0, Specular=g.scalar("Specular", 0.45, "Surface", 0, 1),
             Normal=photo["normal"], AmbientOcclusion=ao, ClearCoat=clear, ClearCoatRoughness=clear_rough,
             Anisotropy=g.scalar("Anisotropy", 0.0, "Surface", -1, 1), Tangent=photo["tangent"])
    finish(m)
    return m


# --------------------------------------------------------------------------- the export's materials

def parse_materials(path):
    """usd/Materials.usda → {name: info}. Plain text, as the exporter writes it (MaterialTable.swift)."""
    text = open(path, encoding="utf-8").read()
    heads = list(re.finditer(r'def Material "([^"]+)"', text))
    out = {}
    for i, h in enumerate(heads):
        body = text[h.end(): heads[i + 1].start() if i + 1 < len(heads) else len(text)]

        def get(pattern, default=None, cast=str):
            mm = re.search(pattern, body)
            return cast(mm.group(1)) if mm else default

        scale = re.search(r"float2 inputs:scale = \(([-\d.e]+),\s*([-\d.e]+)\)", body)
        tex = get(r"asset inputs:file = @([^@]+)@")
        out[h.group(1)] = {
            "source": get(r'museevision:source = "([^"]*)"', ""),
            "tint": get(r'museevision:tint = "#([0-9A-Fa-f]{6})"'),
            "unlit": "museevision:unlit = 1" in body,
            "double_sided": "museevision:doubleSided = 1" in body,
            "roughness": get(r"float inputs:roughness = ([\d.]+)", 0.8, float),
            "metallic": get(r"float inputs:metallic = ([\d.]+)", 0.0, float),
            "opacity": get(r"float inputs:opacity = ([\d.]+)", 1.0, float),
            "clearcoat": get(r"float inputs:clearcoat = ([\d.]+)", 0.0, float),
            "clearcoat_roughness": get(r"float inputs:clearcoatRoughness = ([\d.]+)", 0.0, float),
            "texture": os.path.splitext(os.path.basename(tex))[0] if tex else None,
            # Metres per repeat (the exporter scaled metres back to its tiling); None = 1 m or image-mapped.
            "tile": (round(1.0 / float(scale.group(1)), 3), round(1.0 / float(scale.group(2)), 3)) if scale else None,
        }
    return out


SKIP_PREFIXES = ("painting_", "glass_", "light_grid", "frit", "velarium", "veil")
PLANTING = ("foliage_", "hedge", "flower_", "lily_", "lotus_", "grass_", "bamboo", "bark_")

# Stone without coursing (single blocks, sculpture, rock).
NO_JOINTS = {"WallJoints": 0.0, "FloorJoints": 0.0}
# An image with its own pattern: the procedural stone only drifts the tone.
IMAGE = dict(NO_JOINTS, StrataAmount=0.0, PoreAmount=0.0, VeinAmount=0.0, BlockTone=0.0, MacroVariation=0.07)
# Glazed ceramics: the glaze is the clear coat.
GLAZE = dict(IMAGE, MacroVariation=0.015, RoughnessVariation=0.02, ClearCoat=0.8, ClearCoatRoughness=0.06)
ROCK = dict(NO_JOINTS, StrataAmount=0.12, StrataAlong=0.5, StrataAcross=0.08, FloorVeinScale=0.3, PoreSize=0.02,
            PoreStretch=2.0, PoreAmount=0.7, PoreThreshold=0.3, MacroVariation=0.12, MacroScale=1.0)
# The Reserve's brick (04): warm tan running bond, pale mortar, each brick its own shade; the
# vault soffits (floor-facing) get the same bond through FloorSlab / FloorAspect.
BRICK = {"CourseHeight": 0.075, "BlockLength": 0.225, "BlockJitter": 0.0, "Stagger": 0.5, "StaggerJitter": 0.0,
         "FloorSlab": 0.075, "FloorAspect": 3.0, "FloorStagger": 0.5, "GridOffset": 0.0,
         "WallJoints": 1.0, "FloorJoints": 1.0, "JointWidth": 0.01, "JointColorAmount": 1.0,
         "BlockTone": 0.3, "AltAmount": 0.6, "MacroVariation": 0.08, "MacroScale": 2.0,
         "StrataAmount": 0.05, "StrataAlong": 0.12, "StrataAcross": 0.03, "FloorVeinScale": 0.05, "VeinAmount": 0.0,
         "PoreSize": 0.003, "PoreStretch": 1.5, "PoreAmount": 0.35, "PoreThreshold": 0.35, "PoreDarkness": 0.75,
         "Roughness": 0.85, "RoughnessVariation": 0.05, "ClearCoat": 0.0}
# The Reserve's floor (04): dark polished concrete, large sawcut panels, a cloudy trowelled
# mottle with pale aggregate specks, soft reflections of the pendants.
CONCRETE = {"FloorSlab": 1.5, "FloorAspect": 1.0, "FloorStagger": 0.0, "GridOffset": 0.0, "CourseHeight": 1.5,
            "BlockLength": 1.5, "BlockJitter": 0.0, "WallJoints": 1.0, "FloorJoints": 1.0, "JointWidth": 0.004,
            "JointDarkness": 0.55, "BlockTone": 0.08, "MacroVariation": 0.14, "MacroScale": 1.5,
            "StrataAmount": 0.1, "StrataAlong": 0.4, "StrataAcross": 0.4, "FloorVeinScale": 0.4, "VeinAmount": 0.0,
            "PoreSize": 0.0025, "PoreStretch": 1.0, "PoreAmount": 0.3, "PoreThreshold": 0.45, "PoreDarkness": 1.5,
            "Roughness": 0.5, "RoughnessVariation": 0.1, "ClearCoat": 0.35, "ClearCoatRoughness": 0.22}
# Limestone: travertine's coursing, quieter strata, fewer pores.
LIMESTONE = {"StrataAmount": 0.04, "PoreAmount": 0.25, "PoreThreshold": 0.5}


# The Chinese court's ground (a Suzhou garden's grey granite and pebbles).
SUZHOU_PAVING = {"EFE8DB": "8F8C85"}
SUZHOU_GRAVEL = 0x8A877F


def spec(parent, scalars=None, vectors=None, textures=None, switches=None, two_sided=False, photo=None):
    """photo: a set key, or (key, {pbr.instance_values keywords}): the photographed layer from the museum's library."""
    s = {"parent": parent, "scalars": dict(scalars or {}), "vectors": dict(vectors or {}),
         "textures": dict(textures or {}), "switches": dict(switches or {}), "two_sided": two_sided}
    if photo:
        key, kw = photo if isinstance(photo, tuple) else (photo, {})
        ps, pv, pt, psw = pbr.instance_values(key, **kw)
        for k, v in ps.items():
            s["scalars"].setdefault(k, v)
        for k, v in pv.items():
            s["vectors"].setdefault(k, v)
        s["textures"].update(pt)
        s["switches"].update(psw)
    return s


# One cell the size of the room: no world-grid changes in the veins (the sun clock's slabs are geometry).
UNBROKEN = {"FloorSlab": 60.0, "CourseHeight": 60.0, "BlockLength": 60.0, "BlockJitter": 0.0}
# The sun clock's polished marble: M_Marble_Polished's stone, without its joints.
SUNCLOCK_MARBLE = dict(NO_JOINTS, **UNBROKEN, Roughness=0.26, RoughnessVariation=0.04, ClearCoat=0.7,
                       ClearCoatRoughness=0.14, MacroVariation=0.03, BlockTone=0.0, StrataAlong=0.9, StrataAcross=0.9,
                       FloorVeinScale=0.9, StrataAmount=0.03, VeinAmount=0.13, VeinSharpness=26.0, PoreAmount=0.0)
# (Sparse, fine, light-grey veins: under the bronze dial heavier veining read as a map, fighting the lines.)

# The named instances other code refers to.
NAMED = {
    "M_Travertine_Honed": spec("M_Stone", {
        "Roughness": 0.62, "RoughnessVariation": 0.06, "ClearCoat": 0.0,
        "CourseHeight": 0.6, "BlockLength": 1.8, "BlockJitter": 0.33, "Stagger": 0.5, "StaggerJitter": 0.2,
        "FloorSlab": 1.2, "WallJoints": 1.0, "FloorJoints": 1.0, "JointWidth": 0.003, "JointDarkness": 0.86,
        "MacroScale": 5.0, "MacroVariation": 0.05, "BlockTone": 0.05,
        "StrataAlong": 1.6, "StrataAcross": 0.04, "StrataAmount": 0.07, "FloorVeinScale": 0.7, "VeinAmount": 0.0,
        "PoreSize": 0.006, "PoreStretch": 4.0, "PoreAmount": 0.5, "PoreThreshold": 0.4, "PoreDarkness": 0.72,
    }, {"BaseColor": lin(TRAVERTINE)}),
    # The lawn round the museum (AMuseeGround): a photographed summer lawn (T_lawn, ambientCG's Grass 004, CC0) every
    # 1.6 m, its tone drifting over 9 m so the repeat doesn't show from the garden; matt.
    "M_Lawn": spec("M_Stone", dict(IMAGE, Roughness=0.92, RoughnessVariation=0.04, ClearCoat=0.0, MacroScale=9.0,
                                   MacroVariation=0.16, TexScale=1.6, TexAspect=1.0),
                   {"BaseColor": lin(0xFFFFFF)}, {"Texture": "lawn"}, {"UseTexture": True}),
    # The Sculpture Hall's walls: vein-cut Roman travertine ashlar (T_travertine_veincut, ambientCG's Travertine 009,
    # CC0) in 0.6 m courses of 1.2 m blocks, each block its own slice of the stone, honed, fine dark joints.
    "M_Travertine_Ashlar": spec("M_Stone", dict(IMAGE, Roughness=0.58, RoughnessVariation=0.05, ClearCoat=0.0,
                                                CourseHeight=0.6, BlockLength=1.2, BlockJitter=0.25, Stagger=0.5,
                                                StaggerJitter=0.15, FloorSlab=1.2, WallJoints=1.0, FloorJoints=1.0,
                                                JointWidth=0.004, JointDarkness=0.62, BlockTone=0.04,
                                                MacroScale=5.0, MacroVariation=0.04, TexScale=1.2, TexAspect=1.0,
                                                TexBlockShift=1.0),
                                {"BaseColor": lin(0xE1D6C4)},
                                photo=("S1", dict(block_shift=1.0, contrast=0.7, normal=0.6, rough_influence=0.5))),
    # Filled, polished travertine floors: large slabs, fine joints, crisp clear coat for Lumen.
    "M_Travertine_Polished": spec("M_Stone", {
        # Honed-polished, not a mirror (renderings 01, 02: soft reflections of the piers).
        "Roughness": 0.34, "RoughnessVariation": 0.06, "ClearCoat": 0.6, "ClearCoatRoughness": 0.16,
        "FloorSlab": 0.8, "FloorAspect": 1.5, "FloorStagger": 0.0, "GridOffset": 0.0,
        "WallJoints": 1.0, "FloorJoints": 1.0, "JointWidth": 0.002,
        "JointDarkness": 0.88, "MacroVariation": 0.05, "BlockTone": 0.05,
        "StrataAlong": 1.6, "StrataAcross": 0.04, "StrataAmount": 0.08, "FloorVeinScale": 0.6,
        "PoreSize": 0.005, "PoreAmount": 0.3, "PoreThreshold": 0.45, "PoreDarkness": 0.8,
        # The photograph (S1, Travertine 009 at 4K) carries the veins and pores now: the drawn ones only whisper.
        "StrataAmount": 0.02, "PoreAmount": 0.08,
    }, {"BaseColor": lin(TRAVERTINE_POLISHED)},
        photo=("S1", dict(block_shift=1.0, contrast=0.85, normal=0.35, rough_influence=0.3))),
    "M_Marble_Polished": spec("M_Stone", {
        # Polished, not glassy: the rendering's floor holds a soft reflection of the pilasters, not a mirror.
        "Roughness": 0.28, "RoughnessVariation": 0.05, "ClearCoat": 0.7, "ClearCoatRoughness": 0.15,
        "FloorSlab": 1.2, "FloorStagger": 0.0, "WallJoints": 1.0, "FloorJoints": 1.0, "JointWidth": 0.002,
        "JointDarkness": 0.9, "MacroVariation": 0.03, "BlockTone": 0.03,
        "StrataAlong": 0.9, "StrataAcross": 0.9, "FloorVeinScale": 0.9, "StrataAmount": 0.03,
        "VeinAmount": 0.3, "VeinSharpness": 18.0, "PoreAmount": 0.0,
    }, {"BaseColor": lin(MARBLE), "VeinColor": lin(MARBLE_VEIN)}),
    # The Rotunda's walls and pilasters (rendering 05): cream marble ashlar, its grey crackle veins
    # from T_marble_crackle (stone_textures.py) over the travertine coursing's joints.
    "M_Marble_Wall": spec("M_Stone", {
        "Roughness": 0.45, "RoughnessVariation": 0.06, "ClearCoat": 0.25, "ClearCoatRoughness": 0.3,
        "CourseHeight": 1.2, "BlockLength": 1.9, "BlockJitter": 0.3, "Stagger": 0.5, "StaggerJitter": 0.2,
        "FloorSlab": 1.2, "WallJoints": 1.0, "FloorJoints": 1.0, "JointWidth": 0.003, "JointDarkness": 0.8,
        "MacroScale": 4.0, "MacroVariation": 0.06, "BlockTone": 0.06,
        "StrataAmount": 0.0, "VeinAmount": 0.0, "PoreAmount": 0.08, "PoreThreshold": 0.6,
        "TexScale": 2.0, "TexAspect": 1.0,
    }, {"BaseColor": lin(0xECE4D6)}, {"Texture": "marble_crackle"}, {"UseTexture": True}),
    # The Square (Élan level -1, ASquareStructure). Rammed earth in lifts: T_rammed_earth_lifts
    # (stone_textures.py) is the albedo, one image the room's 7.5 m high by 2.8 m of wall (the actor's
    # UV0: U = x + y, V = 7.5 m - the height), matte; the stone only drifts its tone a little.
    "M_RammedEarth": spec("M_Stone", dict(IMAGE, MacroVariation=0.05, MacroScale=4.0, Roughness=0.92, RoughnessVariation=0.04,
                                          ClearCoat=0.0, TexScale=2.8, TexAspect=7.5 / 2.8),
                          {"BaseColor": lin(0xFFFFFF)}, {"Texture": "rammed_earth_lifts"}, {"UseTexture": True}),
    # The strata round the pit under the glass: T_strata_pit is laid out for the pit (R 4.2 m, 7 m deep):
    # once round (2π × 4.2 m) across, the wall's 7 m and then its floor's 7 m down.
    "M_Strata": spec("M_Stone", dict(IMAGE, MacroVariation=0.03, Roughness=0.95, RoughnessVariation=0.03, ClearCoat=0.0,
                                     TexScale=26.389378, TexAspect=14.0 / 26.389378),
                     {"BaseColor": lin(0xFFFFFF)}, {"Texture": "strata_pit"}, {"UseTexture": True}),
    # The pale stone of the cross: honed limestone in 2.0 × 1.4 m slabs, the joints on the walls' lines
    # (x 40, y -14).
    # A photographed cream stone (T_limestone_honed, ambientCG's Marble 014, CC0), each slab its own slice, honed
    # with a soft sheen: the rammed earth and the pit's rim reflect in it, blurred (the procedural stone read flat).
    "M_SquareFloor": spec("M_Stone", dict(IMAGE, Roughness=0.36, RoughnessVariation=0.06, ClearCoat=0.3,
                                          ClearCoatRoughness=0.22, FloorSlab=1.4, FloorAspect=2.0 / 1.4, FloorStagger=0.0,
                                          GridOffset=0.0, WallJoints=1.0, FloorJoints=1.0, JointWidth=0.003,
                                          JointDarkness=0.8, MacroScale=5.0, MacroVariation=0.04, BlockTone=0.03,
                                          TexScale=2.2, TexAspect=1.0, TexBlockShift=1.0),
                          {"BaseColor": lin(0xFFFFFF)}, {"Texture": "limestone_honed"}, {"UseTexture": True}),
    # The dark rooms: honed dark stone in metre slabs (on the walls' lines), lighter joints.
    "M_SquareFloorDark": spec("M_Stone", dict(LIMESTONE, Roughness=0.45, RoughnessVariation=0.06, ClearCoat=0.15,
                                              ClearCoatRoughness=0.3, FloorSlab=1.0, FloorAspect=1.0, FloorStagger=0.0,
                                              GridOffset=0.0, WallJoints=1.0, FloorJoints=1.0, JointWidth=0.003,
                                              JointDarkness=1.3, PoreDarkness=1.2, MacroVariation=0.05, BlockTone=0.06),
                              {"BaseColor": lin(0x2E2924)}),
    # The ceiling slab and the capitals: warm grey lime plaster.
    "M_SquareCeiling": spec("M_Plaster", {"Roughness": 0.9, "Variation": 0.03, "GrainAmount": 0.02},
                            {"BaseColor": lin(0xD6CDBF)}),
    # The Classical Hall's casts (SMK's Royal Cast Collection): fine warm-white plaster with the soft,
    # slightly waxy sheen of an old cast (a thin shellac), never chalk-flat.
    "M_Plaster_Cast": spec("M_Plaster", {"Roughness": 0.52, "Variation": 0.025, "GrainAmount": 0.012},
                           {"BaseColor": lin(0xEDE6D9)}),
    "M_Plaster_Coffer": spec("M_Plaster", {"Roughness": 0.85, "Variation": 0.03, "GrainAmount": 0.0},
                             {"BaseColor": lin(0xEFE8DB)},
                             photo=("P2", dict(colour_amount=0.5, contrast=0.5, normal=0.35, rough_influence=0.3))),
    "M_Plaster_Moulding": spec("M_Plaster", {"Roughness": 0.8, "Variation": 0.03, "GrainAmount": 0.02},
                               {"BaseColor": lin(0xE4DBCB)}),
    # The Reserve's brick (AReserveStructure lays UV0 along the courses, metres): buff stock brick in a pale lime
    # mortar, 75 mm courses (B1 at 1.95 m per repeat), the brick faces 8 mm proud of the joints (Nanite
    # tessellation); a slow 5 m drift in tone so the 60 m nave never repeats.
    "MI_Brick_Coursed": spec("M_StoneRelief", dict(IMAGE, Roughness=0.86, RoughnessVariation=0.03, ClearCoat=0.0,
                                                    MacroScale=5.0, MacroVariation=0.05),
                             {"BaseColor": lin(BRICK_BUFF)},
                             photo=("B1", dict(use_uv0=True, normal=1.0, rough_influence=0.6, relief_cm=0.8, contrast=0.9))),
    # Frames and handrails: warm, reflective, a little worn.
    # Carved, gilded frames: small-scale burnish and dirt so the gold never reads as flat yellow,
    # the red-brown bole showing in the hollows.
    "M_Gilt_Aged": spec("M_Metal", {"Roughness": 0.28, "Metallic": 1.0, "Variation": 0.18, "NoiseScale": 0.05,
                                    "PatinaAmount": 0.18},
                        {"BaseColor": lin(0xD6AE68), "PatinaColor": lin(0x5A3A22)}),
    # The sun clock (ASunClock, Source/MuseeVision/SunClock): its slabs are the geometry's own (concentric
    # and radial, in two tones), so the stone draws no joints and has one cell the size of the room (the
    # veins run on unbroken instead of changing at the world grid's lines). The drum moves: its wall
    # takes its crackle from its own UVs and has no world-space coursing to slide over it.
    "M_SunClock_Marble": spec("M_Stone", dict(SUNCLOCK_MARBLE), {"BaseColor": lin(MARBLE), "VeinColor": lin(0xBDB8AF)}),
    "M_SunClock_MarbleAlt": spec("M_Stone", dict(SUNCLOCK_MARBLE, GridOffset=7.3, FloorVeinScale=1.25, MacroScale=3.0),
                                 {"BaseColor": lin(0xE8E3D9), "VeinColor": lin(0xB9B3A9)}),
    # A thin band of rosso (Rosso Levanto: deep red-brown, pale veins) and one of nero (Nero Marquina).
    "M_Marble_Rosso": spec("M_Stone", dict(SUNCLOCK_MARBLE, StrataAmount=0.10, MacroVariation=0.06, VeinAmount=0.35,
                                           VeinSharpness=12.0, FloorVeinScale=0.6, Roughness=0.22),
                           {"BaseColor": lin(0x8C3B2E), "VeinColor": lin(0xD8C6B6)}),
    "M_Marble_Nero": spec("M_Stone", dict(SUNCLOCK_MARBLE, StrataAmount=0.05, VeinAmount=0.5, VeinSharpness=26.0,
                                          FloorVeinScale=0.7, Roughness=0.2, ClearCoat=0.8, ClearCoatRoughness=0.1),
                          {"BaseColor": lin(0x1D1C1B), "VeinColor": lin(0xD5D1CA)}),
    "M_SunClock_Drum": spec("M_Stone", dict(NO_JOINTS, **UNBROKEN, Roughness=0.42, RoughnessVariation=0.06, ClearCoat=0.3,
                                            ClearCoatRoughness=0.28, MacroScale=4.0, MacroVariation=0.05, BlockTone=0.0,
                                            StrataAmount=0.0, VeinAmount=0.0, PoreAmount=0.08, PoreThreshold=0.6,
                                            TexScale=2.0, TexAspect=1.0),
                            {"BaseColor": lin(0xECE4D6)}, {"Texture": "marble_crackle"}, {"UseTexture": True}),
    # The classical hall (AClassicalHallStructure, Source/MuseeVision/ClassicalHall): the coloured marbles of its opus
    # sectile floors, column shafts, wall panels and dado, after the Pantheon's floor and the Braccio Nuovo's columns.
    # Each is a full-colour image from stone_textures.py (BaseColor white), polished; the floor's pieces are the geometry's
    # own, so these draw no joints of their own (the dado alone keeps a slab joint every 1.2 m).
    "M_Porphyry": spec("M_Stone", dict(IMAGE, Roughness=0.2, RoughnessVariation=0.04, ClearCoat=0.75, ClearCoatRoughness=0.1,
                                       TexScale=1.0, TexAspect=1.0),
                       {"BaseColor": lin(0xFFFFFF)}, {"Texture": "porphyry"}, {"UseTexture": True}),
    "M_Marble_VerdeAntico": spec("M_Stone", dict(IMAGE, Roughness=0.22, RoughnessVariation=0.05, ClearCoat=0.7,
                                                 ClearCoatRoughness=0.12, TexScale=1.5, TexAspect=1.0),
                                 {"BaseColor": lin(0xFFFFFF)}, {"Texture": "verde_antico"}, {"UseTexture": True}),
    "M_Marble_GialloAntico": spec("M_Stone", dict(IMAGE, Roughness=0.22, RoughnessVariation=0.05, ClearCoat=0.65,
                                                  ClearCoatRoughness=0.12, TexScale=1.5, TexAspect=1.0),
                                  {"BaseColor": lin(0xFFFFFF)}, {"Texture": "giallo_antico"}, {"UseTexture": True}),
    "M_Marble_Pavonazzetto": spec("M_Stone", dict(IMAGE, Roughness=0.24, RoughnessVariation=0.05, ClearCoat=0.6,
                                                  ClearCoatRoughness=0.14, TexScale=1.5, TexAspect=1.0),
                                  {"BaseColor": lin(0xFFFFFF)}, {"Texture": "pavonazzetto"}, {"UseTexture": True}),
    "M_Marble_RossoAntico": spec("M_Stone", dict(IMAGE, WallJoints=1.0, FloorJoints=0.0, CourseHeight=1.0, BlockLength=1.2,
                                                 BlockJitter=0.0, Stagger=0.0, StaggerJitter=0.0, GridOffset=0.0,
                                                 JointWidth=0.002, JointDarkness=0.75, Roughness=0.25,
                                                 RoughnessVariation=0.05, ClearCoat=0.6, ClearCoatRoughness=0.14,
                                                 TexScale=1.0, TexAspect=1.0),
                                 {"BaseColor": lin(0xFFFFFF)}, {"Texture": "rosso_antico"}, {"UseTexture": True}),
    # Its carved white marble (bases, capitals, entablatures, frames, plinths): honed statuary, no coursing, faint veins.
    "M_Marble_Carved": spec("M_Stone", dict(NO_JOINTS, Roughness=0.36, RoughnessVariation=0.05, ClearCoat=0.25,
                                            ClearCoatRoughness=0.3, MacroScale=3.0, MacroVariation=0.03, BlockTone=0.0,
                                            StrataAmount=0.02, StrataAlong=0.8, StrataAcross=0.8, FloorVeinScale=0.8,
                                            VeinAmount=0.12, VeinSharpness=16.0, PoreAmount=0.0),
                            {"BaseColor": lin(0xECE8E0), "VeinColor": lin(0xB5AEA3)}),
    # The inside of its niches: a muted Pompeian red lime stucco behind the white statues.
    "M_Stucco_Pompeian": spec("M_Plaster", {"Roughness": 0.9, "Variation": 0.05, "GrainAmount": 0.03},
                              {"BaseColor": lin(0x8E4B3D)}),
}

# Each master's parameter names by kind ("scalar", "vector", "texture", "switch"), read back from
# the built masters in main().

# --------------------------------------------------------------------------- the material language

# The museum's materials as one work (the materials spec, Part 1): four families that run through every room
# (the stone, the white, one bronze, one oak), each era changing the finish, not the material; the Rotunda is the
# anchor. Every surface takes the photographed layer of its family's set (pbr.py; SourceArt/PBR): colour variation
# about the albedo the spec sets (BaseColor), relief, polish, occlusion. PBR keywords: see pbr.instance_values.

def with_photo(spec_, key, kw=None, scalars=None, vectors=None, drop_texture=False, parent=None, switches=None):
    """spec_ with set `key`'s photographed layer (and overrides); drop_texture: the photograph replaces the image."""
    ps, pv, pt, psw = pbr.instance_values(key, **(kw or {}))
    out = {"parent": parent or spec_["parent"], "scalars": dict(spec_["scalars"]), "vectors": dict(spec_["vectors"]),
           "textures": dict(spec_["textures"]), "switches": dict(spec_["switches"]), "two_sided": spec_["two_sided"]}
    if drop_texture:
        out["textures"].pop("Texture", None)
        out["switches"].pop("UseTexture", None)
    out["scalars"].update(ps)
    out["scalars"].update(scalars or {})
    out["vectors"].update(pv)
    out["vectors"].update(vectors or {})
    out["textures"].update(pt)
    out["switches"].update(psw)
    out["switches"].update(switches or {})
    return out


def merged(*dicts, **kw):
    out = {}
    for d in dicts:
        out.update(d)
    out.update(kw)
    return out


QUIET_DRAWN = {"StrataAmount": 0.02, "PoreAmount": 0.12, "VeinAmount": 0.0}   # the photograph carries the stone now
RELIEF_ONLY = dict(colour_amount=0.0)                                          # keep the image, add relief and polish

LANGUAGE = {
    # A · the stone. Classical honed, Modern polished, Future glass-smooth; the Rotunda's own stones keep their
    # albedo and gain only relief and polish.
    "M_Travertine_Honed": ("S1", dict(block_shift=1.0, contrast=0.6, normal=0.5, rough_influence=0.5), QUIET_DRAWN,
                           {"BaseColor": lin(0xE1D6C4)}, False),
    # The Rotunda's walls and the sun clock's drum: Marble014's own photographed veins (the drawn crackle, a polygon net
    # of lines, read as cracked paint at arm's length), each block its own slice; the drum on its UV0, as it turns.
    "M_Marble_Wall": ("S2", dict(block_shift=1.0, contrast=0.5, normal=0.3, rough_influence=0.3), {"TexBlockShift": 0.0},
                      {"BaseColor": lin(0xEAE1D1)}, True),
    "M_SunClock_Drum": ("S2", dict(contrast=0.5, normal=0.25, rough_influence=0.3, use_uv0=True), {},
                        {"BaseColor": lin(0xEAE1D1)}, True),
    "M_SunClock_Marble": ("S4", dict(RELIEF_ONLY, normal=0.1, rough_influence=0.25), {}, {}, False),   # marble's, not travertine's voids
    "M_SunClock_MarbleAlt": ("S4", dict(RELIEF_ONLY, normal=0.1, rough_influence=0.25), {}, {}, False),   # marble's, not travertine's voids
    "M_Marble_Rosso": ("S4", dict(RELIEF_ONLY, normal=0.2, rough_influence=0.2), {}, {}, False),
    "M_Marble_Nero": ("S4", dict(RELIEF_ONLY, normal=0.2, rough_influence=0.2), {}, {}, False),
    "M_Marble_Polished": ("S4", dict(block_shift=1.0, contrast=0.8, normal=0.3, rough_influence=0.3), {"VeinAmount": 0.08}, {}, False),
    "M_Marble_Carved": ("S4", dict(contrast=0.7, normal=0.3, rough_influence=0.3), {"VeinAmount": 0.04},
                        {"BaseColor": lin(0xECE8E0)}, False),
    # The Classical Hall's coloured marbles: real verde and giallo; the generated porphyry, pavonazzetto and
    # rosso gain the statuary marble's relief and polish.
    "M_Marble_VerdeAntico": ("S6", dict(contrast=0.9, normal=0.3, rough_influence=0.3), {}, {"BaseColor": lin(0x2F3A30)}, True),
    "M_Marble_GialloAntico": ("S7", dict(contrast=1.3, normal=0.3, rough_influence=0.3), {}, {"BaseColor": lin(0xD8C59C)}, True),
    "M_Porphyry": ("S4", dict(RELIEF_ONLY, normal=0.25, rough_influence=0.2), {}, {}, False),
    # Pavonazzetto: white Phrygian marble with violet veins, from the real statuary marble's photograph (the
    # generated image read as a cow's hide): its veins pushed up, the stone a breath violet.
    "M_Marble_Pavonazzetto": ("S4", dict(contrast=1.5, normal=0.3, rough_influence=0.2), {}, {"BaseColor": lin(0xEBE3E8)}, True),
    "M_Marble_RossoAntico": ("S4", dict(RELIEF_ONLY, normal=0.25, rough_influence=0.2), {}, {}, False),
    # The Square: the Rotunda's cream marble underfoot, honed; the earth walls gain the limewash's relief.
    "M_SquareFloor": ("S2", dict(block_shift=1.0, contrast=0.9, normal=0.3, rough_influence=0.4), {},
                      {"BaseColor": lin(0xE8DFCF)}, True),
    "M_SquareFloorDark": ("G5", dict(block_shift=1.0, normal=0.6, rough_influence=0.5), {"StrataAmount": 0.0, "PoreAmount": 0.0},
                          {"BaseColor": lin(0x2E2924)}, False),
    "M_RammedEarth": ("P3", dict(RELIEF_ONLY, normal=0.6, rough_influence=0.3), {}, {}, False),
    # B · the white: lime plaster and stucco, matt; the colour only a breath, the grain in the light.
    "M_Plaster_Moulding": ("P2", dict(colour_amount=0.5, contrast=0.5, normal=0.3, rough_influence=0.3), {"GrainAmount": 0.0}, {}, False),
    "M_Plaster_Cast": ("P2", dict(colour_amount=0.3, contrast=0.5, normal=0.2, rough_influence=0.3), {"GrainAmount": 0.0}, {}, False),
    "M_SquareCeiling": ("P1", dict(colour_amount=0.4, contrast=0.5, normal=0.4, rough_influence=0.3), {"GrainAmount": 0.0}, {}, False),
    "M_Stucco_Pompeian": ("P1", dict(colour_amount=0.4, contrast=0.6, normal=0.4, rough_influence=0.3), {"GrainAmount": 0.0}, {}, False),
    # C · the bronze: aged gilt on the frames and the Classical Hall.
    "M_Gilt_Aged": ("M3", dict(colour_amount=0.5, normal=0.3, rough_influence=0.5), {}, {}, False),
    # The grounds: the lawn with its relief and occlusion (Grass004's full set, 1.4 m).
    # A mown lawn's albedo is darker and less saturated than the photograph's mean (0.09, 0.12, 0.045 linear); a shade
    # darker again under the blades (AMuseeLawn, lawn.py sets the same), so the ground between them reads as the sward's depth.
    "M_Lawn": ("L1", dict(normal=0.8, rough_influence=0.5, scale=1.4, contrast=0.8), {"MacroVariation": 0.1},
               {"BaseColor": (0.05, 0.072, 0.026)}, True),
}
# III · the Future's floor: the Rotunda's own cream marble (S2), glass-smooth (the Atrium's rings of slabs are
# geometry: no drawn joints). The Atrium floor and the Sphere's platform (AAtriumBaseStructure, ElanKit).
NAMED["M_Elan_Mirror"] = with_photo(spec("M_Stone", dict(NO_JOINTS, **UNBROKEN, Roughness=0.09, RoughnessVariation=0.02,
                                                          ClearCoat=1.0, ClearCoatRoughness=0.04, MacroScale=6.0,
                                                          MacroVariation=0.02, StrataAmount=0.0, VeinAmount=0.0,
                                                          PoreAmount=0.0, BlockTone=0.0),
                                         {"BaseColor": lin(0xEAE2D4)}),
                                    "S2", dict(contrast=0.9, normal=0.15, rough_influence=0.2))
for _name, (_key, _kw, _sc, _vec, _drop) in LANGUAGE.items():
    if _name in NAMED:
        NAMED[_name] = with_photo(NAMED[_name], _key, _kw, _sc, _vec, _drop)

# The oak as solid timber (M_Wood): boards 12 cm wide, each its own slice and tone (±12 %, a breath of hue), one
# board per member where the member is a single timber (BoardLength 40: the native kits start each member's U at its
# own multiple of 40 m); oiled (Specular 0.45), the pores dull in the veneer's roughness, the sheen drawn across the
# grain (Anisotropy). The veneer's grain runs up its image: PBRSwapUV lays it along U.
OAK_BOARDS = {"GrainAmount": 0.0, "FigureAmount": 0.0, "Variation": 0.03, "BoardWidth": 0.12, "BoardLength": 40.0,
              "BoardTone": 0.12, "BoardHue": 0.03, "BoardJoint": 0.35, "Anisotropy": -0.3, "Specular": 0.45, "ClearCoat": 0.0,
              "PBRBlockShift": 1.0}
OAK_PHOTO = dict(contrast=0.9, normal=0.7, rough_influence=0.9, use_uv0=True, swap_uv=True, block_shift=1.0)
# The Chinese timber (栗壳色): the same oak under a chestnut lacquer, a real second layer (F0 0.04, R 0.25, orange
# peel), worn thinner where hands and backs rub; the grain reads only in grazing light.
CHESTNUT_LACQUER = dict(OAK_BOARDS, Roughness=0.5, BoardWidth=1.0, BoardTone=0.07, BoardJoint=0.0, Anisotropy=-0.15,
                        ClearCoat=0.5, ClearCoatRoughness=0.12, LacquerWear=0.2)

# ---- The core palette as named instances (furniture and thresholds task; AMuseeFurniture, the Élan's collar) ----
# One bronze alloy (F0 #C9A266) in its three states, the one oak in two finishes, the leather and the felt. The oak and
# the brushed bronze read UV0 (metres, U along the grain / the brushing), which AMuseeFurniture lays along each piece.
NAMED.update({
    "M_Bronze_Brushed": spec("M_Metal", {"Roughness": 0.30, "Metallic": 1.0, "Variation": 0.05, "NoiseScale": 0.3,
                                         "PatinaAmount": 0.0},
                             {"BaseColor": lin(0xC9A266)},
                             photo=("M2", dict(colour_amount=0.3, normal=0.6, rough_influence=0.5, use_uv0=True))),
    "M_Bronze_Patina": spec("M_Metal", {"Roughness": 0.50, "Metallic": 1.0, "Variation": 0.12, "NoiseScale": 0.08,
                                        "PatinaAmount": 0.25},
                            {"BaseColor": lin(0x6B5A45), "PatinaColor": lin(0x3A4232)},
                            photo=("M1", dict(colour_amount=0.6, normal=0.5, rough_influence=0.5, metal_amount=1.0))),
    "M_Gilt_Clean": spec("M_Metal", {"Roughness": 0.17, "Metallic": 1.0, "Variation": 0.04, "NoiseScale": 0.1,
                                     "PatinaAmount": 0.0},
                         {"BaseColor": lin(0xE0BC78)},
                         photo=("M4", dict(colour_amount=0.3, normal=0.25, rough_influence=0.3))),
    "M_Oak_Fumed": spec("M_Wood", dict(OAK_BOARDS, Roughness=0.52), {"BaseColor": lin(0x6B5236)}, photo=("W2", OAK_PHOTO)),
    "M_Oak_Natural": spec("M_Wood", dict(OAK_BOARDS, Roughness=0.52), {"BaseColor": lin(0xA07A52)}, photo=("W2", OAK_PHOTO)),
    "M_Leather_Cognac": spec("M_Fabric", {"Roughness": 0.45, "Specular": 0.5, "SheenAmount": 0.05, "SlubAmount": 0.0,
                                          "WeaveAmount": 0.0, "MottleAmount": 0.03, "MottleSize": 0.05},
                             {"BaseColor": lin(0x6B3F24)},
                             photo=("F3", dict(colour_amount=0.5, contrast=0.8, normal=0.5, rough_influence=0.5))),
    "M_Felt": spec("M_Fabric", {"Roughness": 0.95, "Specular": 0.3, "SheenAmount": 0.4, "SlubAmount": 0.0,
                                "WeaveAmount": 0.0, "MottleAmount": 0.03},
                   {"BaseColor": lin(0x4E5A4C)},
                   photo=("F2", dict(colour_amount=0.5, contrast=0.7, normal=0.6, rough_influence=0.4))),
})
# ---- The physics of each finish (the photoreal audit's plan, items 3-5, 9, 10) ----
# Joints that read in the light, each slab its own tone, hue and tilt, polish that wears, the coat only where a real
# coat is (polished floors 0.35-0.45, the Élan's mirror 1.0; honed stone none), the voids open on walls and filled on
# floors, grime where it collects, the stone's own F0 (calcite: Specular 0.6). Degrees for SlabTilt and Waviness.
STONE_FLOOR = {"SlabTilt": 0.15, "Waviness": 0.05, "WaveScale": 0.5, "RoughWear": 0.1, "TrafficWear": 0.8,
               "BlockTone": 0.2, "BlockHue": 0.02, "JointBevel": 0.0015, "JointMinVisible": 0.8, "Specular": 0.6}
STONE_WALL = {"SlabTilt": 0.3, "Waviness": 0.08, "WaveScale": 0.6, "RoughWear": 0.08, "BlockTone": 0.22, "BlockHue": 0.025,
              "JointBevel": 0.003, "JointMinVisible": 0.65, "Specular": 0.6, "FootDirt": 0.05, "DustUp": 0.04}
MACRO = {"Macro": True}
PHYSICS = {
    # A · travertine: honed walls with open voids, and the same stone filled on its floors.
    "M_Travertine_Honed": (dict(STONE_WALL, TrafficWear=0.6, Roughness=0.6, JointWidth=0.004, JointDarkness=0.72,
                                BlockTone=0.2, VoidOpen=1.0, VoidDarkness=0.5, FillerRough=0.12, NormalStrength=0.9,
                                FloorNormalScale=0.2, PBRContrast=0.55), MACRO),
    "M_Travertine_Ashlar": (dict(STONE_WALL, BlockTone=0.26, JointWidth=0.004, JointDarkness=0.6, VoidOpen=1.0,
                                 VoidDarkness=0.5, NormalStrength=1.0, PBRContrast=0.6), MACRO),
    "M_Travertine_Polished": (dict(STONE_FLOOR, Roughness=0.32, ClearCoat=0.85, ClearCoatRoughness=0.08, JointWidth=0.0025,
                                   JointDarkness=0.58, FillerRough=0.12, NormalStrength=0.12, PBRRoughInfluence=0.35,
                                   PBRContrast=0.75, Specular=0.3), MACRO),
    # The marbles: polished floors 0.45 coat, honed walls and carving none.
    "M_Marble_Polished": (dict(STONE_FLOOR, Roughness=0.24, ClearCoat=0.9, ClearCoatRoughness=0.06, JointWidth=0.0015,
                               JointDarkness=0.65, BlockTone=0.12, Specular=0.3), MACRO),
    "M_Marble_Wall": ({"ClearCoat": 0.0, "Roughness": 0.42, "SlabTilt": 0.2, "Waviness": 0.06, "RoughWear": 0.06,
                       "BlockTone": 0.08, "JointBevel": 0.002, "JointDarkness": 0.82, "JointMinVisible": 0.45,
                       "FootDirt": 0.04, "DustUp": 0.03, "Specular": 0.6}, MACRO),
    "M_SunClock_Marble": ({"ClearCoat": 0.4, "ClearCoatRoughness": 0.12, "Waviness": 0.05, "RoughWear": 0.08,
                           "TrafficWear": 0.6, "Specular": 0.6}, MACRO),
    "M_SunClock_MarbleAlt": ({"ClearCoat": 0.4, "ClearCoatRoughness": 0.12, "Waviness": 0.05, "RoughWear": 0.08,
                              "TrafficWear": 0.6, "Specular": 0.6}, MACRO),
    "M_Marble_Rosso": ({"ClearCoat": 0.4, "ClearCoatRoughness": 0.12, "Waviness": 0.05, "RoughWear": 0.08, "Specular": 0.6}, MACRO),
    "M_Marble_Nero": ({"ClearCoat": 0.45, "ClearCoatRoughness": 0.1, "Waviness": 0.05, "RoughWear": 0.08, "Specular": 0.6}, MACRO),
    # The drum turns: nothing world-space may slide over it, so no macro; honed, no coat.
    "M_SunClock_Drum": ({"ClearCoat": 0.0, "Roughness": 0.42, "Specular": 0.6}, {}),
    "M_Porphyry": ({"ClearCoat": 0.45, "ClearCoatRoughness": 0.12, "Waviness": 0.05, "RoughWear": 0.06, "Specular": 0.6}, MACRO),
    "M_Marble_VerdeAntico": ({"ClearCoat": 0.45, "ClearCoatRoughness": 0.12, "Waviness": 0.05, "RoughWear": 0.06,
                              "Specular": 0.6}, MACRO),
    "M_Marble_GialloAntico": ({"ClearCoat": 0.45, "ClearCoatRoughness": 0.12, "Waviness": 0.05, "RoughWear": 0.06,
                               "Specular": 0.6}, MACRO),
    "M_Marble_Pavonazzetto": ({"ClearCoat": 0.4, "ClearCoatRoughness": 0.14, "Waviness": 0.05, "RoughWear": 0.06,
                               "Specular": 0.6}, MACRO),
    "M_Marble_RossoAntico": ({"ClearCoat": 0.4, "ClearCoatRoughness": 0.14, "Waviness": 0.06, "RoughWear": 0.06,
                              "SlabTilt": 0.2, "JointBevel": 0.002, "Specular": 0.6, "FootDirt": 0.04}, MACRO),
    "M_Marble_Carved": ({"ClearCoat": 0.0, "Roughness": 0.4, "DustUp": 0.05, "FootDirt": 0.03, "Specular": 0.6}, {}),
    # III · the one mirror: a polished floor is still not optically flat.
    "M_Elan_Mirror": ({"Waviness": 0.02, "WaveScale": 0.7, "RoughWear": 0.015, "Specular": 0.6}, MACRO),
    "M_SquareFloor": (dict(STONE_FLOOR, ClearCoat=0.15, ClearCoatRoughness=0.22, JointDarkness=0.72), MACRO),
    "M_SquareFloorDark": (dict(STONE_FLOOR, ClearCoat=0.1, ClearCoatRoughness=0.3, Specular=0.56), MACRO),
    # B · lime: trowelled undulation, grime at the foot, dust on the ledges.
    "M_Plaster_Coffer": ({"Waviness": 0.6, "WaveScale": 0.35}, MACRO),
    "M_Plaster_Moulding": ({"DustUp": 0.05, "FootDirt": 0.03}, {}),
    "M_SquareCeiling": ({"Waviness": 0.6}, MACRO),
    "M_Stucco_Pompeian": ({"Waviness": 0.6, "FootDirt": 0.04}, MACRO),
    # The brick: hand-laid (a course is never a plane), grime at its foot.
    "MI_Brick_Coursed": ({"Waviness": 0.3, "WaveScale": 0.4, "FootDirt": 0.06}, MACRO),
    # C · bronze: brushed anisotropic (0.6), the gilt and patina not.
    "M_Bronze_Brushed": ({"Anisotropy": -0.6}, {}),
}
# Polished stone (the texture audit, 2026-09-26): under Substrate the legacy clear coat is a top layer of F0 0.04 whose
# coverage is ClearCoat, so a coat of 0.4-0.45 halved the sharp reflection of every polished floor and column (the
# Pantheon's and the Louvre's floors hold the columns and windows clearly). The polish is the coat, nearly whole (M_Stone
# takes it off in the joints and the pores and thins it in the traffic lanes); beneath it the stone keeps its softer
# sheen at a lower F0 (Specular 0.3), so the two together come to calcite's ~0.05 at normal incidence.
POLISH = {"ClearCoat": 0.88, "ClearCoatRoughness": 0.07, "Specular": 0.3}
for _name in ("M_SunClock_Marble", "M_SunClock_MarbleAlt", "M_Marble_Rosso", "M_Marble_Nero", "M_Porphyry",
              "M_Marble_VerdeAntico", "M_Marble_GialloAntico", "M_Marble_Pavonazzetto", "M_Marble_RossoAntico"):
    PHYSICS[_name][0].update(POLISH)
for _name, (_sc, _sw) in PHYSICS.items():
    if _name in NAMED:
        NAMED[_name]["scalars"].update(_sc)
        NAMED[_name]["switches"].update(_sw)

# The seats' honed travertine (a bench's slab, its plinths): one stone each, so no drawn slab or course joints.
_solid = {k: (dict(v) if isinstance(v, dict) else v) for k, v in NAMED["M_Travertine_Honed"].items()}
_solid["scalars"].update(NO_JOINTS, BlockTone=0.0, PBRBlockShift=0.0, Roughness=0.52)
NAMED["M_Travertine_Solid"] = _solid

# Saved instances in place of the rooms' run-time ones (a material made at play keeps a component procedural: no Nanite,
# no Lumen card, so no bounce from it). The Hall of Light's statuary bronze (its floor's inlays, the stereo's stand),
# the stele feet's brushed brass (the Salon's and the Atrium's), the Atrium floor's joints (the marble a shade darker,
# unpolished in the groove).
NAMED["M_Bronze_Statuary"] = spec("M_Metal", {"Roughness": 0.4, "Metallic": 1.0, "Variation": 0.08, "PatinaAmount": 0.05},
                                  {"BaseColor": (0.10, 0.061, 0.032), "PatinaColor": lin(0x3A4232)},
                                  photo=("M1", dict(colour_amount=0.6, normal=0.5, rough_influence=0.5, metal_amount=1.0)))
NAMED["M_Brass_Brushed"] = spec("M_Metal", {"Roughness": 0.34, "Metallic": 1.0, "Variation": 0.04, "PatinaAmount": 0.0,
                                            "Anisotropy": -0.5},
                                {"BaseColor": (0.72, 0.52, 0.25)},
                                photo=("M2", dict(colour_amount=0.3, normal=0.6, rough_influence=0.5)))
NAMED["M_Elan_Mirror_Joint"] = spec("M_Elan_Mirror", {"Roughness": 0.3, "ClearCoat": 0.3, "ClearCoatRoughness": 0.14},
                                    {"BaseColor": cscale(lin(0xEAE2D4), 0.8)})
# ---- end of the core palette ----


def language(name, info, s):
    """The imported materials' instances in the language (the rule's spec, with its family's photograph)."""
    if not isinstance(s, dict):
        return s
    tint = info["tint"] or "FFFFFF"
    grey = sum(c * w for c, w in zip(lin(tint), (0.2126, 0.7152, 0.0722)))
    if name in ("stone_slab_EFE6DA", "stone_slab_F6EFE4"):          # the Reserve's ground concrete
        return with_photo(s, "C1", dict(block_shift=1.0, contrast=0.8, normal=0.3, rough_influence=0.5),
                          merged(QUIET_DRAWN, STONE_FLOOR, Roughness=0.35, ClearCoat=0.55, ClearCoatRoughness=0.07, RoughWear=0.06,
                               TrafficWear=0.3, Waviness=0.1, WaveScale=0.8, BlockTone=0.08, BlockHue=0.0, SlabTilt=0.1,
                               JointBevel=0.001, JointDarkness=1.25, Specular=0.5),
                          drop_texture=True, switches=MACRO)
    if name == "brick":                                              # the long stair's shaft: the Reserve's brick
        return with_photo(s, "B1", dict(normal=1.0, rough_influence=0.6, relief_cm=0.6, contrast=0.9),
                          dict(IMAGE, Roughness=0.86, ClearCoat=0.0, Waviness=0.3, WaveScale=0.4, FootDirt=0.06),
                          {"BaseColor": lin(BRICK_BUFF)}, parent="M_StoneRelief", switches=MACRO)
    if name.startswith("stone_slab_"):                               # Suzhou granite paving (grey, not the photo's pink)
        # Flamed granite set in sand: more lippage than a gallery floor, no coat, granite's F0 0.045.
        return with_photo(s, "G1", dict(block_shift=1.0, colour_amount=0.6, contrast=0.6, normal=0.6, rough_influence=0.5),
                          merged(QUIET_DRAWN, STONE_FLOOR, SlabTilt=0.4, JointBevel=0.003, BlockTone=0.12, BlockHue=0.0,
                               RoughWear=0.06, Specular=0.56, ClearCoat=0.0), switches=MACRO)
    if name == "gravel":
        return with_photo(s, "G3", dict(normal=1.0, relief_cm=1.2, contrast=0.9, rough_influence=0.4),
                          dict(IMAGE, Roughness=0.9), drop_texture=True, parent="M_StoneRelief")
    if name == "roof_tiles":
        return with_photo(s, "K1", dict(RELIEF_ONLY, normal=0.5, rough_influence=0.3))
    if name in ("roof_coping", "roof_ridge"):
        return with_photo(s, "K1", dict(contrast=0.7, normal=0.5, rough_influence=0.3), QUIET_DRAWN)
    if name in ("taihu_rock", "rock_hollow", "pond_bank"):
        colour = {"taihu_rock": 0xA9A69E, "pond_bank": 0x8E897F}.get(name)
        return with_photo(s, "G4", dict(contrast=0.9, normal=0.8, rough_influence=0.4), {"StrataAmount": 0.03, "PoreAmount": 0.2},
                          {"BaseColor": lin(colour)} if colour else {})
    if name == "marble_carrara":                                     # the statues: statuary marble's grain, faint
        return with_photo(s, "S4", dict(colour_amount=0.5, contrast=0.6, normal=0.2, rough_influence=0.3))
    if name == "floor_dark":
        return with_photo(s, "G5", dict(block_shift=1.0, normal=0.6, rough_influence=0.5), QUIET_DRAWN)
    if name == "stone" or name.startswith("stone_"):
        # The museum's stone (travertine, honed); the Chinese Wing's grey stone is Suzhou granite.
        if grey < 0.45 or name.startswith("stone_plinth"):     # the ceramics' drums: Suzhou granite, honed
            return with_photo(s, "G1", dict(block_shift=1.0, colour_amount=0.6, contrast=0.6, normal=0.5, rough_influence=0.5), QUIET_DRAWN,
                              {"BaseColor": lin(0xA8A59E)} if name.startswith("stone_plinth") else None)
        return with_photo(s, "S1", dict(block_shift=1.0, contrast=0.55, normal=0.9, rough_influence=0.5, floor_normal=0.2),
                          merged(QUIET_DRAWN, STONE_WALL, VoidOpen=1.0, VoidDarkness=0.5, FillerRough=0.12, ClearCoat=0.0,
                               JointDarkness=0.72), switches=MACRO)
    if name in ("bronze", "bronze_dark", "bronze_warm"):            # patinated: the Classical bronze
        return with_photo(s, "M1", dict(colour_amount=0.6, normal=0.5, rough_influence=0.5, metal_amount=1.0))
    if name.startswith(("bronze", "steel_")) and not name.startswith("bronze_gold"):   # brushed: the Modern bronze
        return with_photo(s, "M2", dict(colour_amount=0.3, normal=0.6, rough_influence=0.5), {"Anisotropy": -0.5})
    if name.startswith(("wall_bay_", "wall_cabinet", "silk_")) or name == "cord":       # silk on the walls, mounts
        return with_photo(s, "F1", dict(colour_amount=0.35, contrast=0.6, normal=0.6, rough_influence=0.4, scale=0.3))
    # Lime: trowelled undulation (Waviness °), rain and mop grime at the foot, dust on what faces up.
    if name.startswith("whitewash"):
        return with_photo(s, "P3", dict(colour_amount=0.4, contrast=0.3, normal=0.5, rough_influence=0.3),
                          {"GrainAmount": 0.0, "Waviness": 0.8, "FootDirt": 0.05, "DustUp": 0.03}, switches=MACRO)
    if name.startswith("plaster_oval"):
        return with_photo(s, "P1", dict(colour_amount=0.3, contrast=0.25, normal=0.2, rough_influence=0.3),
                          {"GrainAmount": 0.0, "Waviness": 0.5, "WaveScale": 0.5, "FootDirt": 0.03, "DustUp": 0.02}, switches=MACRO)
    if name.startswith("plaster"):
        return with_photo(s, "P1", dict(colour_amount=0.4, contrast=0.4, normal=0.35, rough_influence=0.3),
                          {"GrainAmount": 0.0, "Waviness": 0.8, "FootDirt": 0.04, "DustUp": 0.03}, switches=MACRO)
    if name.startswith("timber_") or name in ("seat_rail", "plan_chest"):
        # D · the oak (W2, along each member on UV0: the native kits lay U along the piece); the Chinese timber is the
        # same oak in chestnut lacquer (栗壳色), the seat rails' lacquer worn by backs and hands.
        chestnut = name.startswith("timber_dark") or name == "seat_rail"
        if chestnut:
            sc = dict(CHESTNUT_LACQUER, LacquerWear=0.5 if name == "seat_rail" else 0.2)
            return with_photo(s, "W2", dict(OAK_PHOTO, contrast=0.45, normal=0.3, rough_influence=0.4), sc,
                              {"BaseColor": lin(0x4A3326)})
        return with_photo(s, "W2", OAK_PHOTO, dict(OAK_BOARDS, Roughness=0.52, BoardWidth=0.16), {"BaseColor": lin(0x6B5236)})
    return s

PARAMS = {}


def parameter_names(material):
    out = {}
    for kind, fn in (("scalar", "get_scalar_parameter_names"), ("vector", "get_vector_parameter_names"),
                     ("texture", "get_texture_parameter_names"), ("switch", "get_static_switch_parameter_names")):
        try:
            out[kind] = {str(n) for n in getattr(MEL, fn)(material)}
        except Exception as e:  # noqa: BLE001
            warn(f"{material.get_name()}: {kind} parameter names not read ({e})")
    return out


def root_of(parent):
    while parent in NAMED:
        parent = NAMED[parent]["parent"]
    return parent


def textured(info, extra=None):
    """Scalars and the texture for an image multiplied into a master (UV0 in metres per repeat)."""
    tile = info["tile"] or (1.0, 1.0)
    s = {"TexScale": tile[0], "TexAspect": tile[1] / tile[0]}
    s.update(extra or {})
    return s


def rule(name, info):
    """The instance for one USD material (in the material language), or a string saying why it keeps its own."""
    return language(name, info, base_rule(name, info))


def base_rule(name, info):
    """The instance for one USD material: a spec, or a string saying why it keeps its own."""
    if name.startswith(SKIP_PREFIXES):
        return "handled elsewhere"
    if info["unlit"]:
        return "self-lit"
    if info["opacity"] < 0.999:
        return "translucent"
    if name.startswith(PLANTING) or name.startswith("water_"):
        return None

    r, metal, ds = info["roughness"], info["metallic"], info["double_sided"]
    tint = info["tint"] or "FFFFFF"
    cc, ccr = info["clearcoat"], info["clearcoat_roughness"]
    tile = info["tile"]

    # Stone.
    if name.startswith("travertine_honed"):
        return spec("M_Travertine_Honed", {}, {"BaseColor": cmul(lin(TRAVERTINE), soft_tint(tint))}, two_sided=ds)
    if name.startswith("travertine_polished"):
        # The export's full polish (c90) is M_Travertine_Polished's; a lesser one (the Sculpture
        # court's c63) keeps its own coat.
        # A real polished floor's coat is 0.35-0.45 (the export's 0.63-0.9 made every floor one uniform mirror).
        s = {} if cc >= 0.8 else {"ClearCoat": min(cc, 0.45), "ClearCoatRoughness": max(ccr, 0.1)}
        return spec("M_Travertine_Polished", s, {"BaseColor": cmul(lin(TRAVERTINE_POLISHED), soft_tint(tint))}, two_sided=ds)
    if name == "marble_polished":
        s = {}
        if tile:
            s["FloorSlab"] = tile[0] / 2
        return spec("M_Marble_Polished", s, two_sided=ds)
    if name == "marble_carrara":
        # Statuary marble, honed: no coat (the audit: F0 of calcite, Specular 0.6).
        return spec("M_StoneSSS", dict(NO_JOINTS, Roughness=0.32, StrataAmount=0.02, Specular=0.6,
                                    StrataAlong=0.35, StrataAcross=0.35, FloorVeinScale=0.35, VeinAmount=0.12,
                                    PoreAmount=0.0, BlockTone=0.0, MacroVariation=0.02),
                    {"BaseColor": cscale(lin(tint), 0.95), "VeinColor": lin(0xB5B0A8)}, two_sided=ds)
    if name in ("stone_slab_EFE6DA", "stone_slab_F6EFE4"):
        # Only the Reserve's floor and viewing aisle use these: dark polished concrete (04).
        # Mid-grey polished concrete (rendering 04), its trowelled cloud from T_concrete_mottle.
        # Darker and more polished than the rest of CONCRETE: 04's floor is a charcoal grey that holds the
        # pendants and piers in soft, pooled reflections.
        return spec("M_Stone", dict(CONCRETE, TexScale=3.0, TexAspect=1.0, Roughness=0.34, ClearCoat=0.6,
                                    ClearCoatRoughness=0.12),
                    {"BaseColor": lin(0x4C4A47 if name.endswith("EFE6DA") else 0x484643)},
                    {"Texture": "concrete_mottle"}, {"UseTexture": True}, two_sided=ds)
    if name == "brick":
        # A soft orange-tan (rendering 04), not a saturated red: the pendants' warm light adds the rest.
        return spec("M_Stone", BRICK, {"BaseColor": lin(0xB0907A), "AltColor": lin(0x94725E),
                                       "JointColor": lin(0xC4B6A4)}, two_sided=ds)
    if name.startswith("stone_slab_"):
        # The Chinese Wing's slabs: one slab per repeat of the old texture, its colour in the name.
        colour = name[len("stone_slab_"):]
        # The cloister and vestibule paving: grey granite, as in Suzhou's gardens, not pale stone - it
        # frames the whitewashed walls instead of joining them in one glare under the open sky.
        colour = SUZHOU_PAVING.get(colour, colour)
        return spec("M_Stone", dict(LIMESTONE, Roughness=min(r, 0.7), FloorSlab=tile[0] if tile else 1.0, JointWidth=0.003),
                    {"BaseColor": cscale(lin(colour), 0.95)}, two_sided=ds)
    if name == "sun_clock_floor":
        # The Rotunda's dial: the drawn image (hour lines, numerals, the inscription, the gilt sun) under a
        # soft polished coat. The bronze is the image's own dark bronze, not metal: shaded as metal, the
        # lines picked up the bright walls and vanished into the stone.
        return spec("M_Stone", textured(info, dict(IMAGE, MacroVariation=0.03, Roughness=0.32, ClearCoat=0.85,
                                                   ClearCoatRoughness=0.09, RoughWear=0.06, TrafficWear=0.6, Specular=0.3)),
                    {"BaseColor": lin(tint)}, {"Texture": info["texture"]}, {"UseTexture": True}, ds)
    if name in ("rammed_earth", "strata", "gravel", "roof_tiles"):
        if name == "gravel":
            tint = SUZHOU_GRAVEL   # grey river pebbles, not pale grit
        return spec("M_Stone", textured(info, dict(IMAGE, Roughness=r)), {"BaseColor": lin(tint)},
                    {"Texture": info["texture"]}, {"UseTexture": True}, ds)
    if name in ("meiping", "peachbloom", "chicken_cup"):
        return spec("M_Stone", textured(info, dict(GLAZE, Roughness=max(r, 0.2))), {"BaseColor": lin(tint)},
                    {"Texture": info["texture"]}, {"UseTexture": True}, ds)
    if name.startswith("sancai_"):
        # Three-colour splashed glaze over the model's amber (M_Glaze), not one flat colour.
        # The horse stands on a plinth 0.9 m up and is 0.7 m tall: the glaze stops at mid-leg, green on the trappings'
        # height, cream on the mane and crest.
        return spec("M_Glaze", {"Roughness": max(0.1, min(r, 0.2)), "GlazeLine": 1.1, "GreenLow": 1.37, "CreamLow": 1.44,
                                "GreenThreshold": 0.32, "StreakLength": 0.24, "StreakWidth": 0.014, "StreakAmount": 0.85},
                    {"BaseColor": lin(0x985318), "StreakColor": lin(0x391706)}, two_sided=ds)
    if name.startswith("celadon_"):
        return spec("M_Stone", dict(GLAZE, Roughness=r), {"BaseColor": lin(tint)}, two_sided=ds)
    if name in ("taihu_rock", "rock_hollow", "pond_bank"):
        return spec("M_Stone", dict(ROCK, Roughness=r), {"BaseColor": lin(tint)}, two_sided=ds)
    if name in ("roof_coping", "roof_ridge"):
        return spec("M_Stone", dict(NO_JOINTS, **LIMESTONE, Roughness=r), {"BaseColor": lin(tint)}, two_sided=ds)
    if name == "floor_dark":
        # The Square's dark rooms: honed dark stone, slabs with lighter joints.
        return spec("M_Stone", dict(LIMESTONE, Roughness=0.5, FloorSlab=1.0, JointDarkness=1.3, PoreDarkness=1.2),
                    {"BaseColor": lin(tint)}, two_sided=ds)
    if name == "stone" or name.startswith("stone_"):
        single = name.startswith(("stone_basin", "stone_bench", "stone_plinth", "stone_facade"))
        s = dict(LIMESTONE, Roughness=min(r, 0.75))
        if single:
            s.update(NO_JOINTS)
        return spec("M_Stone", s, {"BaseColor": cscale(lin(tint), 0.95)}, two_sided=ds)

    # Metal.
    if name.startswith("gilt"):
        return spec("M_Gilt_Aged", {"Roughness": max(0.2, r - 0.07), "PatinaAmount": 0.12},
                    {"BaseColor": lin(0xDDB672)}, two_sided=ds)
    if name.startswith("bronze_gold"):
        return spec("M_Gilt_Aged", {"Roughness": r, "PatinaAmount": 0.1}, {"BaseColor": cscale(lin(tint), 1.25)}, two_sided=ds)
    if name == "gold_lily":
        return spec("M_Metal", {"Roughness": r, "Metallic": 1.0, "Variation": 0.08, "PatinaAmount": 0.0},
                    {"BaseColor": lin(tint)}, switches={"UseTexture": False}, two_sided=ds)
    if name in ("bronze", "bronze_dark", "bronze_warm"):
        # Rodin's bronzes: a dark, uneven patina.
        return spec("M_Metal", {"Roughness": r, "Metallic": 1.0, "Variation": 0.15, "NoiseScale": 0.08,
                                "PatinaAmount": 0.3},
                    {"BaseColor": lin(tint), "PatinaColor": lin(0x3A4232)}, switches={"UseTexture": False}, two_sided=ds)
    if name.startswith("bronze"):
        return spec("M_Metal", {"Roughness": r, "Metallic": 1.0, "Variation": 0.1, "NoiseScale": 0.15, "PatinaAmount": 0.1},
                    {"BaseColor": lin(tint), "PatinaColor": lin(0x3A4232)}, switches={"UseTexture": False}, two_sided=ds)
    if name.startswith("steel_"):
        return spec("M_Metal", {"Roughness": r, "Metallic": 1.0, "Variation": 0.05, "NoiseScale": 0.5, "PatinaAmount": 0.0},
                    {"BaseColor": lin(tint)}, switches={"UseTexture": False}, two_sided=ds)
    if name == "brass_mesh":
        return spec("M_Metal", textured(info, {"Roughness": r, "Metallic": 0.85, "Variation": 0.06, "PatinaAmount": 0.0}),
                    {"BaseColor": lin(tint)}, {"Texture": info["texture"]}, {"UseTexture": True}, ds)

    # Fabric.
    if name.startswith(("wall_bay_", "wall_cabinet", "silk_")) or name == "cord":
        colour = lin(tint)
        if name == "wall_bay_2":
            # 02's silk is a neutral grey; the export's slate leans blue.
            grey = 0.2126 * colour[0] + 0.7152 * colour[1] + 0.0722 * colour[2]
            colour = cmix(colour, (grey, grey, grey), 0.45)
        s = {"Roughness": 0.65, "SheenAmount": 0.25, "SlubAmount": 0.07, "WeaveAmount": 0.1, "MottleAmount": 0.04}
        if name == "cord":
            s.update(Roughness=0.8, SheenAmount=0.45, SlubAmount=0.03)
        return spec("M_Fabric", s, {"BaseColor": colour, "SheenTint": cmix(colour, (0.9, 0.88, 0.84), 0.4)}, two_sided=ds)

    # Plaster.
    if name.startswith("moulding"):
        return spec("M_Plaster_Moulding", {"Roughness": min(r, 0.85)}, {"BaseColor": lin(tint)}, two_sided=ds)
    if name.startswith(("plaster", "whitewash", "lantern_white", "photo_mount", "plinth_cream", "roller_ivory")):
        s = {"Roughness": min(r, 0.9), "Variation": 0.03, "GrainAmount": 0.02}
        if name.startswith("plaster_oval"):
            # The Nymphéas oval (03): lime-white, softly clouded by the trowel.
            s.update(Variation=0.045, NoiseScale=1.5, GrainAmount=0.015)
        if name == "roller_ivory":
            s["Roughness"] = 0.5
        return spec("M_Plaster", s, {"BaseColor": cscale(lin(tint), 0.95 if name == "whitewash" else 1.0)}, two_sided=ds)

    # Wood.
    if name.startswith("timber_") or name in ("seat_rail", "plan_chest"):
        grain = {"plan_chest": 0.08, "seat_rail": 0.25}.get(name, 0.35)
        return spec("M_Wood", {"Roughness": r, "GrainAmount": grain, "FigureAmount": grain},
                    {"BaseColor": lin(tint)}, two_sided=ds)

    # Colours the palette doesn't name yet (matte_<hex>, metal_<hex>).
    if name.startswith("matte_"):
        return spec("M_Plaster", {"Roughness": r}, {"BaseColor": lin(tint)}, two_sided=ds)
    if name.startswith("metal_"):
        return spec("M_Metal", {"Roughness": r, "Metallic": max(metal, 0.9)}, {"BaseColor": lin(tint)},
                    switches={"UseTexture": False}, two_sided=ds)
    return None


# --------------------------------------------------------------------------- assets

SOURCE_ART = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "SourceArt", "Textures")


def texture_asset(name):
    """
    T_<name> in /Game/Museum/Textures: from windows/SourceArt/Textures (stone_textures.py; imported
    again each run, so a regenerated image arrives), or from usd/textures once.
    """
    path = f"{TEXTURES}/T_{name}"
    art = os.path.join(SOURCE_ART, "T_" + name + ".png")
    if os.path.exists(art) or not EAL.does_asset_exist(path):
        png = art if os.path.exists(art) else os.path.join(P.USD_DIR, "textures", name + ".png")
        if not os.path.exists(png):
            warn(f"texture {png} missing")
            return None
        task = unreal.AssetImportTask()
        task.filename = png
        task.destination_path = TEXTURES
        task.destination_name = "T_" + name
        task.automated = True
        task.replace_existing = True
        task.save = True
        TOOLS.import_asset_tasks([task])
    tex = unreal.load_asset(path) if EAL.does_asset_exist(path) else None
    if tex is None:
        for p in EAL.list_assets(TEXTURES, recursive=False):
            if p.split(".")[0].rsplit("/", 1)[-1] in (name, "T_" + name):
                tex = unreal.load_asset(p)
                break
    if tex is None:
        warn(f"texture {name} not imported")
        return None
    # Colour images, sampled as Color (a greyscale import would not match the sampler).
    try:
        if tex.get_editor_property("compression_settings") != unreal.TextureCompressionSettings.TC_DEFAULT or \
                not tex.get_editor_property("srgb"):
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
            tex.set_editor_property("srgb", True)
            EAL.save_loaded_asset(tex)
    except Exception as e:  # noqa: BLE001
        warn(f"T_{name}: compression not checked ({e})")
    return tex


def material_instance(name, folder, parent_asset, s, textures):
    path = f"{folder}/{name}"
    if EAL.does_asset_exist(path):
        mi = unreal.load_asset(path)
        if not isinstance(mi, unreal.MaterialInstanceConstant):
            warn(f"{path} exists and is not a material instance: left alone")
            return None
    else:
        mi = TOOLS.create_asset(name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, parent_asset)
    MEL.clear_all_material_instance_parameters(mi)
    # The library's setters always return False (UE 5.8), so each value is checked against the
    # master's parameter names and read back after the update below.
    known = PARAMS.get(root_of(s["parent"]), {})

    def exists(kind, k):
        if kind in known and k not in known[kind]:
            warn(f"{name}: {root_of(s['parent'])} has no {kind} parameter {k}")
            return False
        return True

    for k, v in s["scalars"].items():
        if exists("scalar", k):
            MEL.set_material_instance_scalar_parameter_value(mi, k, float(v))
    for k, v in s["vectors"].items():
        if exists("vector", k):
            MEL.set_material_instance_vector_parameter_value(mi, k, linear_colour(v))
    for k, v in s["textures"].items():
        tex = textures.get(v) or pbr.resolve(v)
        if tex is None:
            warn(f"{name}: texture {v} missing")
        elif exists("texture", k):
            MEL.set_material_instance_texture_parameter_value(mi, k, tex)
    # Only the switches turned on are set (so instances without them share the master's shaders).
    # Clearing is meant to drop the others, but in 5.8 a switch set on an earlier run can survive it
    # (the sun clock kept Inlay after its rule dropped it): any still on that shouldn't be is set off.
    # Switches the parent instance turns on are inherited: they count as wanted (else they'd be switched off here).
    inherited, parent = {}, s["parent"]
    chain = []
    while parent in NAMED:
        chain.append(NAMED[parent]["switches"])
        parent = NAMED[parent]["parent"]
    for sw in reversed(chain):
        inherited.update(sw)
    inherited.update(s["switches"])
    wanted_on = {k for k, v in inherited.items() if v}
    for k in known.get("switch", ()):
        if k not in wanted_on and MEL.get_material_instance_static_switch_parameter_value(mi, k):
            try:
                MEL.set_material_instance_static_switch_parameter_value(
                    mi, k, False, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER, False)
            except Exception:  # noqa: BLE001 - older signature
                MEL.set_material_instance_static_switch_parameter_value(mi, k, False)
    for k, v in s["switches"].items():
        if not v or not exists("switch", k):
            continue
        try:
            MEL.set_material_instance_static_switch_parameter_value(
                mi, k, bool(v), unreal.MaterialParameterAssociation.GLOBAL_PARAMETER, False)
        except Exception:  # noqa: BLE001 - older signature
            try:
                MEL.set_material_instance_static_switch_parameter_value(mi, k, bool(v))
            except Exception as e:  # noqa: BLE001
                warn(f"{name}: switch {k} not set ({e})")
    try:
        o = mi.get_editor_property("base_property_overrides")
        o.set_editor_property("override_two_sided", bool(s["two_sided"]))
        o.set_editor_property("two_sided", bool(s["two_sided"]))
        mi.set_editor_property("base_property_overrides", o)
    except Exception as e:  # noqa: BLE001
        if s["two_sided"]:
            warn(f"{name}: two-sided not set ({e})")
    MEL.update_material_instance(mi)
    verify_instance(name, mi, s, textures)
    EAL.save_loaded_asset(mi, only_if_is_dirty=False)
    return mi


def verify_instance(name, mi, s, textures):
    for k, v in s["scalars"].items():
        got = MEL.get_material_instance_scalar_parameter_value(mi, k)
        if abs(got - float(v)) > 1e-4:
            warn(f"{name}: scalar {k} is {got}, wanted {v}")
    for k, v in s["vectors"].items():
        got = MEL.get_material_instance_vector_parameter_value(mi, k)
        if max(abs(got.r - v[0]), abs(got.g - v[1]), abs(got.b - v[2])) > 1e-4:
            warn(f"{name}: vector {k} is {got}, wanted {v}")
    for k, v in s["textures"].items():
        got = MEL.get_material_instance_texture_parameter_value(mi, k)
        tex = textures.get(v) or pbr.resolve(v)
        if tex is not None and (got is None or got.get_path_name() != tex.get_path_name()):
            warn(f"{name}: texture {k} is {got}, wanted {tex.get_path_name()}")
    for k, v in s["switches"].items():
        if bool(MEL.get_material_instance_static_switch_parameter_value(mi, k)) != bool(v):
            warn(f"{name}: switch {k} is not {v}")


# --------------------------------------------------------------------------- the swap

def usd_key(material_name, usd_names):
    """The USD material a level material stands for: Interchange's copy (maybe with a digit added,
    "glass_oculus1") or one of ours (MI_<name>)."""
    if material_name.startswith("MI_") and material_name[3:] in usd_names:
        return material_name[3:]
    n = material_name
    while n not in usd_names and n and n[-1].isdigit():
        n = n[:-1]
    return n if n in usd_names else None


def swap(instances, usd_names, reasons):
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = comps = slots = 0
    unmapped = {}
    for actor in eas.get_all_level_actors():
        if not any(str(t).startswith("musee.wing:") for t in actor.tags):
            continue
        actors += 1
        for comp in actor.get_components_by_class(unreal.StaticMeshComponent):
            changed = False
            for i, m in enumerate(comp.get_materials()):
                if not m:
                    continue
                key = usd_key(m.get_name(), usd_names)
                if key is None:
                    continue
                target = instances.get(key)
                if target is None:
                    if reasons.get(key) is None:
                        unmapped[key] = unmapped.get(key, 0) + 1
                    continue
                if m.get_path_name() != target.get_path_name():
                    comp.set_material(i, target)
                    slots += 1
                    changed = True
            if changed:
                comps += 1
    return actors, comps, slots, unmapped


# --------------------------------------------------------------------------- water

# The imported ponds' water → the museum's water (setup_project.make_water: Single Layer Water).
WATER_SWAP = {"water_pond": "MI_Water_Pond", "water_garden": "MI_Water_Garden"}


def water_pass():
    """Every imported pond surface on its Single Layer Water instance, on a mesh that isn't Nanite (the
    engine draws that water on ordinary meshes only)."""
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    targets = {k: unreal.load_asset(f"{P.MATERIALS}/{v}") for k, v in WATER_SWAP.items()
               if EAL.does_asset_exist(f"{P.MATERIALS}/{v}")}
    if not targets:
        warn("water: M_Water's instances missing (apply_all glass makes them)")
        return
    changed, meshes = 0, set()
    for actor in eas.get_all_level_actors():
        for comp in actor.get_components_by_class(unreal.StaticMeshComponent):
            for i, m in enumerate(comp.get_materials()):
                key = m and usd_key(m.get_name(), set(WATER_SWAP))
                if not key:
                    continue
                if m.get_path_name() != targets[key].get_path_name():
                    comp.set_material(i, targets[key])
                    changed += 1
                mesh = comp.static_mesh
                if mesh and mesh.get_path_name() not in meshes:
                    meshes.add(mesh.get_path_name())
                    try:
                        ns = mesh.get_editor_property("nanite_settings")
                        if ns.enabled:
                            ns.enabled = False
                            mesh.set_editor_property("nanite_settings", ns)
                            EAL.save_loaded_asset(mesh)
                            log(f"water: {mesh.get_name()} taken off Nanite")
                    except Exception as e:  # noqa: BLE001
                        warn(f"water: {mesh.get_name()} Nanite not checked ({e})")
    log(f"water: {changed} slots on the museum's water, {len(meshes)} pond meshes")


# --------------------------------------------------------------------------- main

def main():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if EAL.does_asset_exist(P.MAP_PATH):
        les.load_level(P.MAP_PATH)
    else:
        warn(f"{P.MAP_PATH} missing: materials only (run setup_project.py and import_wing.py for the swap)")

    usd = parse_materials(os.path.join(P.USD_DIR, "Materials.usda"))
    specs, reasons = {}, {}
    for name, info in sorted(usd.items()):
        r = rule(name, info)
        if isinstance(r, dict):
            specs[name] = r
        else:
            reasons[name] = r
    log(f"{len(usd)} USD materials: {len(specs)} with a rule, "
        f"{sum(1 for r in reasons.values() if r)} kept (paintings, glass, sky-lit, self-lit), "
        f"{sum(1 for r in reasons.values() if r is None)} without a rule")

    # Textures: the images some instances multiply in, plus a default for the texture parameters.
    wanted = sorted({t for s in list(specs.values()) + list(NAMED.values()) for t in s["textures"].values()
                     if not t.startswith("PBR:")} | {"gravel"})
    textures = {t: texture_asset(t) for t in wanted}
    default_tex = textures.get("gravel") or next((t for t in textures.values() if t), None) or \
        unreal.load_asset("/Engine/EngineResources/DefaultTexture")

    masters = {
        "M_Stone": build_stone(default_tex),
        "M_StoneRelief": build_stone(default_tex, "M_StoneRelief", relief=True),
        "M_StoneSSS": build_stone(default_tex, "M_StoneSSS", sss=True),
        "M_Metal": build_metal(default_tex),
        "M_Fabric": build_fabric(),
        "M_Plaster": build_plaster(),
        "M_Wood": build_wood(),
        "M_Glaze": build_glaze(),
    }
    for k, m in masters.items():
        PARAMS[k] = parameter_names(m)
    parents = dict(masters)
    for name, s in NAMED.items():
        if name in PROTECTED:
            continue
        mi = material_instance(name, P.MATERIALS, parents[s["parent"]], s, textures)
        if mi:
            parents[name] = mi
    log(f"named instances: {', '.join(n for n in NAMED if n in parents)}")

    instances = {}
    for name, s in specs.items():
        parent = parents.get(s["parent"])
        if parent is None:
            warn(f"{name}: parent {s['parent']} missing")
            continue
        mi = material_instance("MI_" + name, USD_MATERIALS, parent, s, textures)
        if mi:
            instances[name] = mi
    by_parent = {}
    for name, s in specs.items():
        by_parent.setdefault(root_of(s["parent"]), []).append(name)
    for k in sorted(by_parent):
        log(f"{k}: {len(by_parent[k])} instances")
    # The static permutations (image, inlay, two-sided) compile on their own: check each kind once.
    if shader_stats(masters["M_Stone"])[0]:
        seen = set()
        for name, s in sorted(specs.items()):
            kind = (root_of(s["parent"]), tuple(sorted(k for k, v in s["switches"].items() if v)), s["two_sided"])
            if kind in seen or name not in instances or not (kind[1] or kind[2]):
                continue
            seen.add(kind)
            ps, samplers = shader_stats(instances[name])
            if ps:
                log(f"permutation {kind[0]} {'+'.join(kind[1]) or '-'}{' two-sided' if kind[2] else ''} "
                    f"(MI_{name}): {ps} pixel-shader instructions, {samplers} samplers")
            else:
                warn(f"permutation of {kind[0]} for MI_{name} did not compile")

    if EAL.does_asset_exist(P.MAP_PATH):
        actors, comps, slots, unmapped = swap(instances, set(usd), reasons)
        log(f"swap: {actors} imported actors, {comps} components changed ({slots} material slots)")
        if unmapped:
            log("in the level without a rule (kept as imported): " +
                ", ".join(f"{k} ×{v}" for k, v in sorted(unmapped.items())))
        water_pass()
        les.save_current_level()

    no_rule = sorted(n for n, r in reasons.items() if r is None)
    log(f"materials mapped: {len(instances)}; no rule: {', '.join(no_rule) if no_rule else 'none'}")
    EAL.save_directory(P.MATERIALS, only_if_is_dirty=True, recursive=True)
    EAL.save_directory(TEXTURES, only_if_is_dirty=True, recursive=True)
    if WARNINGS:
        log(f"{len(WARNINGS)} warnings (see above)")
    log("done")


if __name__ == "__main__":
    main()
