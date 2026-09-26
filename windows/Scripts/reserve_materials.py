"""
The Reserve's own materials (AReserveStructure's); native.py makes them before it places the rooms, or run on its own:

    UnrealEditor-Cmd.exe MuseeVision.uproject -run=pythonscript -script="<repo>/windows/Scripts/reserve_materials.py"

- MI_Brick_Vault, on M_StoneRelief_Welded: the Reserve's brick. The same brick as materials.py's MI_Brick_Coursed (its
  spec is read on every run, so the two stay one brick), on a copy of M_StoneRelief whose Nanite displacement is scaled
  by the mesh's vertex colour R. AReserveStructure writes R = 0 on every crease (the groins, the bands' and piers'
  arrises) and on the vault's open edges, 1 elsewhere, so the brick keeps its relief but the two sides of a crease
  are never pushed apart (displaced each along its own normal they opened hairline cracks, lit from behind: the
  Reserve's "streak").
- M_GlobeLamp: a pendant's lit globe of cased opal glass. Emissive, in nits:
  the lamp seen through the opal, so a hot core where the view passes nearest the lamp (CoreNits) falling to the rim
  (RimNits) as pow(cos, CoreExponent); 600 + 5400 cos^7 averages 1800 nits over the disc, a 3400 lm lamp in a 17"
  ball. Cloudy by ±9 % (6 cm) and ±6 % (18 cm), darker at the neck and shoulder (the socket's shadow, the thicker
  glass), a little warmer at the rim, in the lamp's own black-body colour (Kelvin, 2900 K, native.py's lamps too).
  Shown through a camera-like highlight roll-off (RimExposed, RollOff; see make_globe_lamp), so at the Reserve's
  exposure the core clips and the rest keeps its gradient instead of the whole ball clipping to a disc. Over it the
  casing's glossy skin (lit, opaque: white opal under F0 0.04 at roughness 0.04), so the ball reflects the vault and
  the other lamps through the opaque Lumen reflections; the light it lets out is the emission × (1 − Fresnel). (A
  separate thin-translucent outer shell gave the same reflections through the front-layer translucency passes,
  2 ms at 4K; MI_GlobeGlass, which it used, is no longer made.)
- M_Cable: the pendants' cloth-covered flex, dark brown.
- MI_Reserve_Linen (on M_Fabric): the picture screens' faces (AReserveRacks), natural unbleached linen stretched on
  boards: the F1 slubbed weave photographed, laid on the screens' UV0 (metres) rather than the world, so the weave rides
  with a screen as it glides; the fibres' fuzz (Cloth), matte.
- M_PictureLens: the lens under a picture light's trough, lit opal in the lamp's colour (2900 K) through the same
  camera-like roll-off as the globes (it just clips at the room's exposure).
"""
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import musee_paths as P  # noqa: E402

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

LAMP_KELVIN = 2900.0
GLOBE = dict(CoreNits=6000.0, RimNits=600.0, CoreExponent=7.0, CloudAmount=0.09, CloudScaleCm=6.0, DriftAmount=0.06,
             NeckDim=0.35, ShoulderDim=0.45, LampOffset=0.0, Kelvin=LAMP_KELVIN, RimExposed=0.9, RollOff=0.7)
RIM_TINT = (1.0, 0.91, 0.78)
OPAL = (0.80, 0.78, 0.74)            # the opal's own white, seen when other lamps light it
SKIN_ROUGHNESS = 0.04
WELD_SHARPNESS = 8.0                 # the relief is full an eighth of a cell away from a crease


def log(msg):
    unreal.log(f"[reserve_materials] {msg}")


def warn(msg):
    unreal.log_warning(f"[reserve_materials] {msg}")


def fresh_material(name):
    path = f"{P.MATERIALS}/{name}"
    if EAL.does_asset_exist(path):
        m = unreal.load_asset(path)
        for _ in range(10):
            if MEL.get_num_material_expressions(m) == 0:
                break
            MEL.delete_all_material_expressions(m)
            for e in list(MEL.get_material_expressions(m)):
                MEL.delete_material_expression(m, e)
        return m
    return TOOLS.create_asset(name, P.MATERIALS, unreal.Material, unreal.MaterialFactoryNew())


class G:
    """A few expression helpers (inputs: expressions, (expression, output), or numbers)."""

    def __init__(self, m):
        self.m, self.n = m, 0

    def node(self, cls, **props):
        e = MEL.create_material_expression(self.m, cls, -1800 + (self.n // 25) * 260, -1200 + (self.n % 25) * 100)
        self.n += 1
        for k, v in props.items():
            e.set_editor_property(k, v)
        return e

    def link(self, src, dst, pin):
        e, out = src if isinstance(src, tuple) else (src, "")
        if not MEL.connect_material_expressions(e, out, dst, pin):
            warn(f"{self.m.get_name()}: could not connect {e.get_class().get_name()}[{out}] to {dst.get_class().get_name()}[{pin}]")

    def val(self, v):
        if isinstance(v, (int, float)):
            return self.node(unreal.MaterialExpressionConstant, r=float(v))
        if isinstance(v, (list, tuple)) and len(v) == 3 and all(isinstance(x, (int, float)) for x in v):
            return self.node(unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(*[float(x) for x in v], 1.0))
        return v

    def op(self, cls, a, b):
        e = self.node(cls)
        self.link(self.val(a), e, "A")
        self.link(self.val(b), e, "B")
        return e

    def mul(self, a, b):
        return self.op(unreal.MaterialExpressionMultiply, a, b)

    def add(self, a, b):
        return self.op(unreal.MaterialExpressionAdd, a, b)

    def sub(self, a, b):
        return self.op(unreal.MaterialExpressionSubtract, a, b)

    def div(self, a, b):
        return self.op(unreal.MaterialExpressionDivide, a, b)

    def dot(self, a, b):
        return self.op(unreal.MaterialExpressionDotProduct, a, b)

    def one(self, cls, x, pin=""):
        e = self.node(cls)
        self.link(self.val(x), e, pin)
        return e

    def sat(self, x):
        return self.one(unreal.MaterialExpressionSaturate, x)

    def normalize(self, x):
        return self.one(unreal.MaterialExpressionNormalize, x, "VectorInput")

    def sqrt(self, x):
        return self.one(unreal.MaterialExpressionSquareRoot, x)

    def mask(self, x, ch):
        e = self.node(unreal.MaterialExpressionComponentMask, r="R" in ch, g="G" in ch, b="B" in ch, a="A" in ch)
        self.link(x, e, "")
        return e

    def power(self, base, exponent):
        e = self.node(unreal.MaterialExpressionPower)
        self.link(self.val(base), e, "Base")
        x = self.val(exponent)
        # The exponent's pin shows as "Exp" (the property is Exponent, overriding ConstExponent).
        if not any(MEL.connect_material_expressions(x, "", e, pin) for pin in ("Exp", "Exponent")):
            warn(f"{self.m.get_name()}: could not connect a Power's exponent")
        return e

    def lerp(self, a, b, t):
        e = self.node(unreal.MaterialExpressionLinearInterpolate)
        for pin, v in (("A", a), ("B", b), ("Alpha", t)):
            self.link(self.val(v), e, pin)
        return e

    def smoothstep(self, lo, hi, x):
        e = self.node(unreal.MaterialExpressionSmoothStep)
        for pin, v in (("Min", lo), ("Max", hi), ("Value", x)):
            self.link(self.val(v), e, pin)
        return e

    def scalar(self, name, v):
        return self.node(unreal.MaterialExpressionScalarParameter, parameter_name=name, default_value=float(v))

    def vector(self, name, c):
        return self.node(unreal.MaterialExpressionVectorParameter, parameter_name=name,
                         default_value=unreal.LinearColor(c[0], c[1], c[2], 1.0))

    def noise(self, pos, scale, levels=3):
        e = self.node(unreal.MaterialExpressionNoise)
        for k, v in (("scale", float(scale)), ("levels", int(levels)), ("quality", 1), ("output_min", -1.0),
                     ("output_max", 1.0), ("tiling", False), ("turbulence", False)):
            try:
                e.set_editor_property(k, v)
            except Exception:  # noqa: BLE001
                pass
        self.link(pos, e, "")   # Position is the first input
        return e


def usages(m, names=("MATUSAGE_NANITE", "MATUSAGE_STATIC_MESH", "MATUSAGE_INSTANCED_STATIC_MESHES")):
    for u in names:
        v = getattr(unreal.MaterialUsage, u, None)
        if v is not None:
            try:
                MEL.set_base_material_usage(m, v, True)
            except Exception as e:  # noqa: BLE001
                warn(f"{m.get_name()}: usage {u}: {e}")


def finish(m):
    errors = MEL.recompile_material(m)
    EAL.save_loaded_asset(m, only_if_is_dirty=False)
    log(f"{m.get_name()}: {MEL.get_num_material_expressions(m)} expressions{', errors ' + str(list(errors)) if errors else ''}")
    return m


def make_globe_lamp():
    m = fresh_material("M_GlobeLamp")
    for prop, value in (("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT), ("blend_mode", unreal.BlendMode.BLEND_OPAQUE),
                        ("two_sided", False)):
        try:
            if m.get_editor_property(prop) != value:
                m.set_editor_property(prop, value)
        except Exception as e:  # noqa: BLE001
            warn(f"M_GlobeLamp: {prop}: {e}")
    usages(m)
    g = G(m)
    p = {k: g.scalar(k, v) for k, v in GLOBE.items()}
    n = g.normalize(g.node(unreal.MaterialExpressionVertexNormalWS))
    v = g.node(unreal.MaterialExpressionCameraVectorWS)
    uv = g.node(unreal.MaterialExpressionTextureCoordinate)
    neck = g.mask(uv, "G")
    # The lamp at the centre (LampOffset × R above it): the view ray through this point passes nearest it where
    # -V points at it. For the ball P = C + N R, so the direction to the lamp is normalize(-N + up · offset).
    to_lamp = g.normalize(g.add(g.mul(n, -1.0), g.mul(g.val((0.0, 0.0, 1.0)), p["LampOffset"])))
    core = g.sat(g.dot(g.mul(v, -1.0), to_lamp))
    lum = g.add(p["RimNits"], g.mul(g.sub(p["CoreNits"], p["RimNits"]), g.power(g.add(core, 1e-4), p["CoreExponent"])))
    # Opal is never even: milky clouds (6 cm) and a slower drift (18 cm), fixed to the glass.
    wp = g.node(unreal.MaterialExpressionWorldPosition)
    cloud = g.add(1.0, g.add(g.mul(g.noise(g.div(wp, p["CloudScaleCm"]), 1.0, 3), p["CloudAmount"]),
                             g.mul(g.noise(g.div(wp, g.mul(p["CloudScaleCm"], 3.0)), 1.0, 2), p["DriftAmount"])))
    # Darker at the neck (thick glass in the gallery's shadow) and towards it over the shoulder.
    shoulder = g.lerp(1.0, p["ShoulderDim"], g.smoothstep(0.80, 0.97, g.mask(n, "B")))
    dim = g.mul(g.lerp(1.0, p["NeckDim"], neck), shoulder)
    # Warmer at the rim, where the light has come through more opal.
    ndv = g.sat(g.dot(n, v))
    tint = g.lerp(g.vector("RimTint", RIM_TINT), g.val((1.0, 1.0, 1.0)), g.sqrt(ndv))
    # The lamp's black-body colour at unit luminance (the same Planck colour as the lights' Temperature).
    bb = g.one(unreal.MaterialExpressionBlackBody, p["Kelvin"], "Temp")
    colour = g.div(bb, g.dot(bb, g.val((0.2126, 0.7152, 0.0722))))
    nits = g.mul(g.mul(g.mul(lum, cloud), dim), g.scalar("LampScale", 1.0))
    # A camera's highlight roll-off: at the room's exposure (EyeAdaptation) 600 nits is ~40x the lamplit brick and the
    # whole ball would clip to a flat disc, as in a single exposure (a photograph of a lit globe keeps its gradient
    # only through its highlight compression). Above the rim's level the luminance is compressed in log space
    # (RollOff), with the rim put at RimExposed (pre-tonemap units: the core just clips, the rim stays below white),
    # never brighter than the physical value (so under a brighter exposure the globe is simply itself).
    e = g.op(unreal.MaterialExpressionMax, g.node(unreal.MaterialExpressionEyeAdaptation), 1e-6)
    shown = g.div(g.mul(p["RimExposed"], g.power(g.div(nits, p["RimNits"]), p["RollOff"])), e)
    shown = g.op(unreal.MaterialExpressionMin, nits, shown)
    # The casing's skin reflects F (Schlick, F0 0.04) and lets 1 − F of the opal's light out.
    fres = g.node(unreal.MaterialExpressionFresnel, exponent=5.0, base_reflect_fraction=0.04)
    glow = g.mul(g.mul(g.mul(colour, tint), shown), g.sub(1.0, fres))
    MEL.connect_material_property(g.vector("Opal", OPAL), "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(g.scalar("SkinRoughness", SKIN_ROUGHNESS), "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(g.val(0.5), "", unreal.MaterialProperty.MP_SPECULAR)
    MEL.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    return finish(m)


def make_cable():
    m = fresh_material("M_Cable")
    usages(m)
    g = G(m)
    MEL.connect_material_property(g.val((0.022, 0.018, 0.015)), "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(g.val(0.72), "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(g.val(0.35), "", unreal.MaterialProperty.MP_SPECULAR)
    return finish(m)


def make_brick_vault():
    """M_StoneRelief_Welded (materials.build_stone's relief master, its height × saturate(8 · vertex colour R)) and
    MI_Brick_Vault (MI_Brick_Coursed's spec on it)."""
    import materials as M   # noqa: E402 - materials.py's masters and the brick's spec
    import pbr              # noqa: E402
    original = pbr.layer

    def layer(g, *args, **kwargs):
        out = original(g, *args, **kwargs)
        if out.get("height") is not None:
            vc = g.node(unreal.MaterialExpressionVertexColor)
            out["height"] = g.mul(out["height"], g.sat(g.mul((vc, "R"), WELD_SHARPNESS)))
        return out

    textures = {}
    spec = dict(M.NAMED["MI_Brick_Coursed"])
    for t in spec["textures"].values():
        if not str(t).startswith("PBR:"):
            textures[t] = M.texture_asset(t)
    default_tex = M.texture_asset("gravel") or unreal.load_asset("/Engine/EngineResources/DefaultTexture")
    pbr.layer = layer
    try:
        master = M.build_stone(default_tex, "M_StoneRelief_Welded", relief=True)
    finally:
        pbr.layer = original
    M.PARAMS["M_StoneRelief_Welded"] = M.parameter_names(master)
    spec["parent"] = "M_StoneRelief_Welded"
    mi = M.material_instance("MI_Brick_Vault", P.MATERIALS, master, spec, textures)
    for w in M.WARNINGS:
        warn(w)
    log(f"MI_Brick_Vault on M_StoneRelief_Welded ({'made' if mi else 'FAILED'})")
    return mi


# Reserve screens: the linen of the picture screens and their picture lights' lenses (AReserveRacks).
LINEN = 0xB2A690            # natural unbleached linen (sRGB), a shade under the Salon's case linen: the lamps warm it
LENS_NITS = 1500.0


def make_screen_linen():
    import materials as M   # noqa: E402 - the museum's fabric master and the F1 weave
    fabric = unreal.load_asset(f"{P.MATERIALS}/M_Fabric")
    if fabric is None:
        warn("MI_Reserve_Linen: no M_Fabric")
        return None
    M.PARAMS["M_Fabric"] = M.parameter_names(fabric)
    c = M.lin(LINEN)
    # The master's own slubs, weave and mottle are world-projected (they would swim on a gliding screen): off; the
    # photographed weave on UV0 carries the texture instead, with its slubs.
    spec = M.with_photo(M.spec("M_Fabric", {"Roughness": 0.86, "SheenAmount": 0.32, "SlubAmount": 0.0, "WeaveAmount": 0.0,
                                            "MottleAmount": 0.0, "Specular": 0.32},
                               {"BaseColor": c, "SheenTint": M.cmix(c, (0.95, 0.93, 0.9), 0.4)}),
                        "F1", dict(colour_amount=0.6, contrast=0.85, normal=0.75, rough_influence=0.4, scale=0.30, use_uv0=True),
                        # The weave (a 2 mm repeat) beat against the pixels at grazing angles (moiré): it fades to its
                        # average once a pixel's footprint nears it (pbr.layer's detail fade).
                        {"PBRDetailPeriod": 0.002}, switches={"PBRDetail": True})
    mi = M.material_instance("MI_Reserve_Linen", P.MATERIALS, fabric, spec, {})
    for w in M.WARNINGS:
        warn(w)
    log(f"MI_Reserve_Linen on M_Fabric ({'made' if mi else 'FAILED'})")
    return mi


def make_picture_lens():
    m = fresh_material("M_PictureLens")
    usages(m)
    g = G(m)
    bb = g.one(unreal.MaterialExpressionBlackBody, g.scalar("Kelvin", LAMP_KELVIN), "Temp")
    colour = g.div(bb, g.dot(bb, g.val((0.2126, 0.7152, 0.0722))))
    e = g.op(unreal.MaterialExpressionMax, g.node(unreal.MaterialExpressionEyeAdaptation), 1e-6)
    nits = g.scalar("Nits", LENS_NITS)
    shown = g.op(unreal.MaterialExpressionMin, nits, g.div(g.scalar("Exposed", 1.1), e))
    MEL.connect_material_property(g.vector("Opal", OPAL), "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(g.val(0.3), "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(g.mul(colour, shown), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    return finish(m)


def make_all():
    for fn in (make_brick_vault, make_globe_lamp, make_cable, make_screen_linen, make_picture_lens):
        try:
            fn()
        except Exception as e:  # noqa: BLE001 - one failing must not stop the others (AReserveStructure falls back)
            warn(f"{fn.__name__} failed: {e}")


if __name__ == "__main__":
    make_all()
