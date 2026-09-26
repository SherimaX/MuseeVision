"""
The Élan Cube (Source/MuseeVision/Cube): its materials, its texture sets and its journeys' assets, in /Game/Museum/Journeys.

    UnrealEditor-Cmd MuseeVision.uproject -run=pythonscript -script="<repo>/windows/Scripts/journeys.py [parts]"
    (or apply_all.py journeys[:parts])

Parts (without a list: cube, the room's own; the rest are the parked journeys'; "all" for every one):
    cube        MPC_Cube, M_CubePanel, M_CubeStrata, M_CubeMast, M_CubeMuon, M_CubeFrost and the strata's textures
    matterhorn  journey I's land: its materials, then the Nanite tiles from journey_terrain.py's output (minutes)
    mhmat       the land's materials only (M_MH_Terrain and its instances, the lake)
    mhthings    journey I's things (JourneyGen: crevasse, huts, boulders, summit cross, climbers) and their materials
    sea         journey II: its materials, then SeaGen's things (beds, corals, fishes, mantas, mangroves, karst)
    seamat      journey II's materials only
    sealight    the Cube's first piece, II · Sea Light: M_SeaLight_Cells and the cells' mesh (SeaLight.h)
    mhready     the marker that lets the car offer journey I (made only when the journey is presentable)
    seaready    the same for journey II

The texture sets are the Cube's own (SourceArt/Journeys/PBR, fetched by journey_fetch.py; CC0 ambientCG), imported as
pbr.py imports the museum's library: colour sRGB, normal BC5, ORM linear with the normal's variance folded into the
roughness, height grayscale.
"""
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import musee_paths as P  # noqa: E402
import pbr  # noqa: E402

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

HERE = os.path.dirname(os.path.abspath(__file__))
LIBRARY = os.path.join(HERE, "..", "SourceArt", "Journeys", "PBR")
ROOT = P.MUSEUM_CONTENT + "/Journeys"
MATS = ROOT + "/Materials"
TEX = ROOT + "/Textures"


def log(msg):
    unreal.log(f"[journeys] {msg}")


def warn(msg):
    unreal.log_warning(f"[journeys] {msg}")


# ---------------------------------------------------------------------------------------------- assets

def material(name, folder=MATS):
    """The material, emptied in place (what refers to it keeps it)."""
    path = f"{folder}/{name}"
    if EAL.does_asset_exist(path):
        m = unreal.load_asset(path)
        for _ in range(10):
            if MEL.get_num_material_expressions(m) == 0:
                break
            MEL.delete_all_material_expressions(m)
            for e in list(MEL.get_material_expressions(m)):
                MEL.delete_material_expression(m, e)
        return m
    return TOOLS.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())


def collection(name, scalars, vectors=()):
    """The collection with these parameters. Existing ones are kept as they are (their ids stay: the materials that
    read them keep reading them); only missing ones are added."""
    path = f"{MATS}/{name}"
    mpc = unreal.load_asset(path) if EAL.does_asset_exist(path) else         TOOLS.create_asset(name, MATS, unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
    ps = list(mpc.get_editor_property("scalar_parameters"))
    have = {str(p.get_editor_property("parameter_name")) for p in ps}
    changed = False
    for n, v in scalars:
        if n in have:
            continue
        p = unreal.CollectionScalarParameter()
        p.set_editor_property("parameter_name", n)
        p.set_editor_property("default_value", float(v))
        ps.append(p)
        changed = True
    vs = list(mpc.get_editor_property("vector_parameters"))
    vhave = {str(p.get_editor_property("parameter_name")) for p in vs}
    for n, v in vectors:
        if n in vhave:
            continue
        p = unreal.CollectionVectorParameter()
        p.set_editor_property("parameter_name", n)
        p.set_editor_property("default_value", v)
        vs.append(p)
        changed = True
    if changed:
        mpc.set_editor_property("scalar_parameters", ps)
        mpc.set_editor_property("vector_parameters", vs)
        EAL.save_loaded_asset(mpc)
    return mpc


_TEX = {}


def texture_set(key):
    """{map: Texture2D} of one of the Cube's sets, imported when its PNG is new or changed."""
    if key in _TEX:
        return _TEX[key]
    out = {}
    folder = f"{TEX}/{key}"
    for kind in pbr.MAPS:
        png = os.path.join(LIBRARY, key, f"{kind}.png")
        if not os.path.exists(png):
            continue
        name = f"T_{key}_{kind}"
        path = f"{folder}/{name}"
        stamp = f"{os.path.getmtime(png):.0f}:{os.path.getsize(png)}"
        known = pbr._stamps().get(path)
        if not EAL.does_asset_exist(path) or known != stamp:
            task = unreal.AssetImportTask()
            task.filename = png
            task.destination_path = folder
            task.destination_name = name
            task.automated = True
            task.replace_existing = True
            task.save = False
            TOOLS.import_asset_tasks([task])
            pbr._stamps()[path] = stamp
            pbr._save_stamps()
        tex = unreal.load_asset(path) if EAL.does_asset_exist(path) else None
        if tex is None:
            warn(f"{path} not imported")
            continue
        out[kind] = tex
    if "Normal" in out and "ORM" in out:
        try:
            mode = pbr._enum(unreal.CompositeTextureMode, "CTM_NORMAL_ROUGHNESS_TO_GREEN")
            if mode is not None and out["ORM"].get_editor_property("composite_texture") != out["Normal"]:
                out["ORM"].set_editor_property("composite_texture", out["Normal"])
                out["ORM"].set_editor_property("composite_texture_mode", mode)
                out["ORM"].set_editor_property("composite_power", 1.0)
        except Exception as e:  # noqa: BLE001
            warn(f"{key}: specular antialiasing not set ({e})")
    for kind, tex in out.items():
        pbr._configure(tex, kind)
        EAL.save_loaded_asset(tex, only_if_is_dirty=True)
    _TEX[key] = out
    return out


# ---------------------------------------------------------------------------------------------- graphs

class Graph:
    """A material's expressions, laid out in columns."""

    def __init__(self, m):
        self.m = m
        self.row = 0

    def expr(self, cls):
        self.row += 1
        return MEL.create_material_expression(self.m, cls, -1600 - 320 * (self.row // 24), (self.row % 24) * 120)

    def scalar(self, name, value):
        e = self.expr(unreal.MaterialExpressionScalarParameter)
        e.set_editor_property("parameter_name", name)
        e.set_editor_property("default_value", float(value))
        return e

    def vector(self, name, rgb):
        e = self.expr(unreal.MaterialExpressionVectorParameter)
        e.set_editor_property("parameter_name", name)
        e.set_editor_property("default_value", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
        return e

    def const(self, value):
        e = self.expr(unreal.MaterialExpressionConstant)
        e.set_editor_property("r", float(value))
        return e

    def texcoord(self, index):
        e = self.expr(unreal.MaterialExpressionTextureCoordinate)
        e.set_editor_property("coordinate_index", index)
        return e

    def simple(self, cls):
        return self.expr(cls)

    def mpc(self, mpc, name, vector=False):
        e = self.expr(unreal.MaterialExpressionCollectionParameter)
        e.set_editor_property("collection", mpc)
        e.set_editor_property("parameter_name", name)
        return e

    def world_position(self):
        e = self.expr(unreal.MaterialExpressionWorldPosition)
        return e

    def texture(self, tex, uv, name=None):
        """A sample of tex at uv (shared wrap sampler: many samples, few samplers)."""
        e = self.expr(unreal.MaterialExpressionTextureSampleParameter2D if name else unreal.MaterialExpressionTextureSample)
        if name:
            e.set_editor_property("parameter_name", name)
        e.set_editor_property("texture", tex)
        st = unreal.MaterialSamplerType
        kind = tex.get_editor_property("compression_settings")
        if kind == unreal.TextureCompressionSettings.TC_NORMALMAP:
            e.set_editor_property("sampler_type", st.SAMPLERTYPE_NORMAL)
        elif not tex.get_editor_property("srgb"):
            e.set_editor_property("sampler_type", st.SAMPLERTYPE_LINEAR_COLOR)
        try:
            e.set_editor_property("sampler_source", unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS)
        except Exception:  # noqa: BLE001
            pass
        MEL.connect_material_expressions(uv, "", e, "UVs")
        return e

    def custom(self, code, outputs, inputs, description):
        e = self.expr(unreal.MaterialExpressionCustom)
        names = [n for n, _ in inputs]
        if not unreal.MuseeNatureEditorLibrary.configure_custom_expression(e, code, outputs, names, description):
            raise RuntimeError(f"{self.m.get_name()}: the Custom expression '{description}' could not be set up")
        for name, src in inputs:
            source, output = src if isinstance(src, tuple) else (src, "")
            if not MEL.connect_material_expressions(source, output, e, name):
                warn(f"{self.m.get_name()}: '{description}' input {name} not connected")
        return e

    def out(self, prop, expr, output=""):
        if not MEL.connect_material_property(expr, output, prop):
            warn(f"{self.m.get_name()}: could not connect {prop}")


def setp(obj, name, value):
    try:
        obj.set_editor_property(name, value)
    except Exception as e:  # noqa: BLE001
        warn(f"{obj.get_name()}: {name} not set ({e})")


def usage(m, flag, prop):
    """A usage flag the game needs (it cannot add one at run time): the library's call, else the property."""
    try:
        MEL.set_material_usage(m, getattr(unreal.MaterialUsage, flag))
        return
    except Exception:  # noqa: BLE001
        pass
    setp(m, prop, True)


def finish(m):
    # Instanced (fishes, boulders, climbers, flakes, lamps, corals): every surface material of the journeys may be.
    try:
        if m.get_editor_property("material_domain") == unreal.MaterialDomain.MD_SURFACE:
            usage(m, "MATUSAGE_INSTANCED_STATIC_MESHES", "used_with_instanced_static_meshes")
    except Exception as e:  # noqa: BLE001
        warn(f"{m.get_name()}: usage ({e})")
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m)
    log(f"{m.get_name()} made")
    return m


# ---------------------------------------------------------------------------------------------- the Cube

# The light-field panels (ACubeStructure: UV0 metres across a face from its corner, UV1.x the face). Every face is 14 m
# from the centre, so a panel's distance from the car's eyes is sqrt(p² + 14²) with p its centre on the face.
PANEL_COMMON = r"""
float2 q = UV / 0.7;
float2 cell = floor(q);
float2 f = q - cell;
float h = frac(sin(dot(cell + float2(UV1.x * 57.0, UV1.x * 13.0), float2(12.9898, 78.233))) * 43758.5453);
float h2 = frac(h * 91.37 + 0.123);
float2 e2 = min(f, 1.0 - f) * 0.7;
float d = min(e2.x, e2.y);
float aa = max(max(abs(ddx(UV.x)), abs(ddy(UV.x))), max(abs(ddx(UV.y)), abs(ddy(UV.y)))) + 1e-6;
float w = 0.002;   // the seam (2 mm), its LED edge light
float seam = saturate(w / (w + 2.0 * aa)) * (1.0 - smoothstep(w, w + 3.0 * aa, d));   // two pixels wide at least, its light spread: no shimmer (the flicker audit)
float2 pc = (cell + 0.5) * 0.7 - 14.0;
float r = sqrt(dot(pc, pc) + 196.0);
float tp = 0.06 + 0.60 * saturate((r - 14.0) / 10.25) + 0.30 * h;
float rv = Reveal * 1.08;
"""

PANEL_MASK = PANEL_COMMON + r"""
return rv < tp ? 1.0 : 0.0;
"""

PANEL_EMISSIVE = PANEL_COMMON + r"""
// At rest: the panels' black level, a touch uneven panel to panel, the seams a little brighter (their LED edge
// lights): a faint grid. Switching: a panel brightens to white just before it becomes the scene.
float rest = lerp(PanelNits * (0.85 + 0.3 * h2), SeamNits, seam) * GridGlow;
float flash = rv < tp ? pow(saturate(1.0 - (tp - rv) / 0.07), 3.0) * step(0.0001, Reveal) : 0.0;
return float3(1.0, 1.0, 1.0) * rest + float3(0.92, 0.97, 1.0) * flash * FlashNits;
"""

PANEL_ROUGH = PANEL_COMMON + r"""
return saturate(0.14 + 0.05 * h2 + 0.45 * seam);
"""

PANEL_NORMAL = PANEL_COMMON + r"""
// Each panel set a hair out of plane (±0.06°): the room's reflections break at the seams, as a real wall's do.
float2 tilt = (float2(h, h2) - 0.5) * 0.002;
return normalize(float3(tilt, 1.0));
"""


def make_cube_panel(mpc):
    m = material("M_CubePanel")
    setp(m, "blend_mode", unreal.BlendMode.BLEND_MASKED)
    setp(m, "opacity_mask_clip_value", 0.5)
    g = Graph(m)
    uv, uv1 = g.texcoord(0), g.texcoord(1)
    reveal, glow = g.mpc(mpc, "Reveal"), g.mpc(mpc, "GridGlow")
    common = [("UV", uv), ("UV1", uv1), ("Reveal", reveal)]
    mask = g.custom(PANEL_MASK, 1, common, "Panel reveal")
    em = g.custom(PANEL_EMISSIVE, 3, common + [("GridGlow", glow), ("PanelNits", g.scalar("PanelNits", 0.003)),
                                                ("SeamNits", g.scalar("SeamNits", 2.4)), ("FlashNits", g.scalar("FlashNits", 6.0))], "Panel light")
    rough = g.custom(PANEL_ROUGH, 1, common, "Panel roughness")
    nrm = g.custom(PANEL_NORMAL, 3, common, "Panel tilt")
    g.out(unreal.MaterialProperty.MP_OPACITY_MASK, mask)
    g.out(unreal.MaterialProperty.MP_EMISSIVE_COLOR, em)
    g.out(unreal.MaterialProperty.MP_ROUGHNESS, rough)
    g.out(unreal.MaterialProperty.MP_NORMAL, nrm)
    black = g.vector("BaseColor", (0.006, 0.006, 0.007))
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, black)
    g.out(unreal.MaterialProperty.MP_SPECULAR, g.scalar("Specular", 0.5))
    return finish(m)


# The ground behind the shaft's glass (ACubeStructure::GroundFace: vertex colour R soil, G clay, B chalk, A flint, the
# rest crushed stone; UV0 metres (round, down); UV1.x fossil stain, UV1.y topsoil). Each layer its own photographed set.
STRATA_UV = r"""
return UV / Scale;
"""

STRATA_COLOUR = r"""
float wSoil = VC.r, wClay = VC.g, wChalk = VC.b, wFlint = VA;
float wStone = saturate(1.0 - wSoil - wClay - wChalk - wFlint);
float3 soil = lerp(SoilSub.rgb * CSoil.rgb * 1.6, CSoil.rgb * SoilTint.rgb, UV1.y);
float3 clay = CClay.rgb * ClayTint.rgb;
float3 chalk = CChalk.rgb * ChalkTint.rgb;
float3 flint = CFlint.rgb * FlintTint.rgb;
float3 stone = CStone.rgb * StoneTint.rgb;
// Flint nodules: in the chalk glassy black inside a white cortex a few millimetres thick (the rim of the mask); in the
// clay broken, brown-stained, no cortex.
float core = smoothstep(0.5, 0.8, VA);
float chalkiness = saturate(wChalk / max(wSoil + wClay + wChalk, 1e-3));
float rim = saturate(wFlint * 2.5) * (1.0 - core) * smoothstep(0.5, 0.9, chalkiness);
float3 c = soil * wSoil + clay * wClay + chalk * wChalk + stone * wStone;
c = c / max(1.0 - wFlint, 0.25);
c = lerp(c, float3(0.80, 0.78, 0.72), rim * 0.55);
float3 flintC = lerp(float3(0.22, 0.18, 0.14) * (0.8 + 0.4 * CFlint.g), flint, chalkiness);
c = lerp(c, flintC, core);
c *= lerp(float3(1, 1, 1), Fossil.rgb, UV1.x);
return c;
"""

STRATA_NORMAL = r"""
float wSoil = VC.r, wClay = VC.g, wChalk = VC.b, wFlint = VA;
float wStone = saturate(1.0 - wSoil - wClay - wChalk - wFlint);
float3 n = NSoil * wSoil + NClay * wClay + NChalk * wChalk + NFlint * wFlint + NStone * wStone;
n.xy *= Strength;
return normalize(n);
"""

STRATA_ORM = r"""
float wSoil = VC.r, wClay = VC.g, wChalk = VC.b, wFlint = VA;
float wStone = saturate(1.0 - wSoil - wClay - wChalk - wFlint);
float3 o = OSoil * wSoil + OClay * wClay + OChalk * wChalk + OFlint * wFlint + OStone * wStone;
// Flint is glassy; the soil a little damp (darker, glossier); the chalk dry.
o.g = lerp(o.g, 0.32, wFlint);
o.g = saturate(o.g - 0.12 * wSoil * UV1.y);
return float3(o.r, o.g, 0.0);
"""


def make_cube_strata():
    sets = {k: texture_set(k) for k in ("JS1", "JS2", "JS3", "JS4", "JS5")}
    m = material("M_CubeStrata")
    g = Graph(m)
    uv, uv1, vc = g.texcoord(0), g.texcoord(1), g.simple(unreal.MaterialExpressionVertexColor)
    scales = {"JS1": 0.9, "JS2": 1.2, "JS3": 1.6, "JS4": 0.4, "JS5": 0.8}
    names = {"JS1": "Soil", "JS2": "Clay", "JS3": "Chalk", "JS4": "Flint", "JS5": "Stone"}
    col, nrm, orm = {}, {}, {}
    for k, s in sets.items():
        tuv = g.custom(STRATA_UV, 2, [("UV", uv), ("Scale", g.const(scales[k]))], f"{names[k]} UV")
        col[k] = g.texture(s["Color"], tuv)
        nrm[k] = g.texture(s["Normal"], tuv)
        orm[k] = g.texture(s["ORM"], tuv)
    # The subsoil: the topsoil's photograph, paler and browner, sampled at another scale.
    sub_uv = g.custom(STRATA_UV, 2, [("UV", uv), ("Scale", g.const(1.3))], "Subsoil UV")
    sub = g.texture(sets["JS1"]["Color"], sub_uv)
    inputs = [("VC", vc), ("VA", (vc, "A")), ("UV1", uv1), ("SoilSub", (sub, "RGB"))] + [(f"C{names[k]}", (col[k], "RGB")) for k in sets] + [
        ("SoilTint", g.vector("SoilTint", (0.9, 0.85, 0.8))), ("ClayTint", g.vector("ClayTint", (1.05, 0.92, 0.8))),
        ("ChalkTint", g.vector("ChalkTint", (1.0, 0.99, 0.95))), ("FlintTint", g.vector("FlintTint", (0.55, 0.55, 0.6))),
        ("StoneTint", g.vector("StoneTint", (0.85, 0.85, 0.85))), ("Fossil", g.vector("FossilStain", (0.86, 0.74, 0.58)))]
    colour = g.custom(STRATA_COLOUR, 3, inputs, "Strata colour")
    normal = g.custom(STRATA_NORMAL, 3, [("VC", vc), ("VA", (vc, "A"))] + [(f"N{names[k]}", (nrm[k], "RGB")) for k in sets] + [("Strength", g.scalar("NormalStrength", 1.0))], "Strata normal")
    orm_in = [("VC", vc), ("VA", (vc, "A")), ("UV1", uv1)] + [(f"O{names[k]}", (orm[k], "RGB")) for k in sets]
    rough = g.custom(STRATA_ORM.replace("return float3(o.r, o.g, 0.0);", "return o.g;"), 1, orm_in, "Strata roughness")
    ao = g.custom(STRATA_ORM.replace("return float3(o.r, o.g, 0.0);", "return o.r;"), 1, orm_in, "Strata AO")
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, colour)
    g.out(unreal.MaterialProperty.MP_NORMAL, normal)
    g.out(unreal.MaterialProperty.MP_ROUGHNESS, rough)
    g.out(unreal.MaterialProperty.MP_AMBIENT_OCCLUSION, ao)
    g.out(unreal.MaterialProperty.MP_SPECULAR, g.scalar("Specular", 0.5))
    return finish(m)


# The mast: gilt, the spiral seam of its two bands (world space: it stands still as the column is fed out), 45 mm pitch.
MAST_SEAM = r"""
float2 d = WP.xy - Axis.xy;
float a = atan2(d.y, d.x) / 6.2831853;
float t = frac(WP.z / (Pitch * 100.0) + a);
float s = min(t, 1.0 - t) * Pitch * 100.0;       // cm from the seam
float aa = fwidth(WP.z) * 0.7 + 1e-4;
return 1.0 - smoothstep(0.08, 0.08 + aa + 0.05, s);
"""


def make_cube_mast():
    m = material("M_CubeMast")
    g = Graph(m)
    wp = g.world_position()
    seam = g.custom(MAST_SEAM, 1, [("WP", wp), ("Axis", g.vector("Axis", (5400.0, 0.0, 0.0))), ("Pitch", g.scalar("Pitch", 0.045))], "Mast seam")
    colour = g.custom("return lerp(Gold.rgb, Gold.rgb * 0.35, S);", 3, [("Gold", g.vector("Gold", (0.98, 0.76, 0.40))), ("S", seam)], "Mast colour")
    rough = g.custom("return lerp(R, 0.55, S);", 1, [("R", g.scalar("Roughness", 0.17)), ("S", seam)], "Mast roughness")
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, colour)
    g.out(unreal.MaterialProperty.MP_METALLIC, g.const(1.0))
    g.out(unreal.MaterialProperty.MP_ROUGHNESS, rough)
    return finish(m)


# The Cube at rest (Cube/CubeRest.h): the muons' condensation tracks. One thin cylinder per line, instanced; custom
# data 0 the glow, 1 the line's radius (cm). Kept at least a pixel wide wherever it is (its light spread to keep its
# energy), so a line 12 m off is as faint as it should be, not broken into dashes.
MUON_WPO = r"""
float d = length(WP - Cam);
float w = max(Rad, d * PixelAngle * 0.9);
return normalize(N) * (w - Rad);
"""

MUON_LIGHT = r"""
float d = length(WP - Cam);
float w = max(Rad, d * PixelAngle * 0.9);
float edge = saturate(abs(dot(normalize(N), normalize(Cam - WP))));
return float3(0.93, 0.96, 1.0) * Nits * Glow * (Rad / w) * edge * edge;
"""


def make_cube_muon():
    m = material("M_CubeMuon")
    setp(m, "blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    setp(m, "shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    setp(m, "enable_responsive_aa", True)
    setp(m, "use_translucency_vertex_fog", False)
    g = Graph(m)
    glow = g.expr(unreal.MaterialExpressionPerInstanceCustomData)
    glow.set_editor_property("data_index", 0)
    rad = g.expr(unreal.MaterialExpressionPerInstanceCustomData)
    rad.set_editor_property("data_index", 1)
    rad.set_editor_property("const_default_value", 0.25)
    cam = g.simple(unreal.MaterialExpressionCameraPositionWS)
    common = [("Cam", cam), ("Rad", rad), ("PixelAngle", g.scalar("PixelAngle", 0.0012))]
    wpo = g.custom(MUON_WPO, 3, common + [("WP", g.world_position()), ("N", g.simple(unreal.MaterialExpressionVertexNormalWS))], "Muon width")
    light = g.custom(MUON_LIGHT, 3, common + [("WP", g.world_position()), ("N", g.simple(unreal.MaterialExpressionVertexNormalWS)),
                                             ("Glow", glow), ("Nits", g.scalar("Nits", 6.0))], "Muon light")
    g.out(unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET, wpo)
    g.out(unreal.MaterialProperty.MP_EMISSIVE_COLOR, light)
    return finish(m)


# The car's floor and rail cleared in the Cube (UElanCar): the frost that shows them in the dark room. Thin glass has
# no diffuse, so a frosted pane lit only by the car's glimmer vanished; this is the frost's own scatter: lit
# translucency (forward shading, every light), pale, satin, mostly clear so what glows below shows through softly.
def make_cube_frost():
    m = material("M_CubeFrost")
    setp(m, "blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    setp(m, "translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE)
    setp(m, "output_translucent_velocity", True)
    g = Graph(m)
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, g.vector("Tint", (0.80, 0.84, 0.83)))
    g.out(unreal.MaterialProperty.MP_ROUGHNESS, g.scalar("Roughness", 0.45))
    g.out(unreal.MaterialProperty.MP_SPECULAR, g.scalar("Specular", 0.5))
    g.out(unreal.MaterialProperty.MP_OPACITY, g.scalar("Opacity", 0.22))
    return finish(m)


CUBE_PARAMETERS = [("Reveal", 0.0), ("GridGlow", 1.0), ("IceGlow", 1.0), ("Daylight", 1.0), ("WaterLevel", -2200.0),
                   ("WaterOn", 0.0), ("SunNits", 1.0), ("Plankton", 0.0)]


def cube_collection():
    return collection("MPC_Cube", CUBE_PARAMETERS)


def make_cube():
    mpc = cube_collection()
    make_cube_panel(mpc)
    make_cube_strata()
    make_cube_mast()
    make_cube_muon()   # (the Cube at rest: what reaches the earth)
    make_cube_frost()  # (the car cleared at the centre)


# ---------------------------------------------------------------------------------------------- the Matterhorn

TERRAIN_DIR = os.path.join(HERE, "..", "SourceArt", "Journeys", "Matterhorn", "terrain")
MH = ROOT + "/Matterhorn"
MH_MATS = MH + "/Materials"


def import_image(png, folder, name):
    path = f"{folder}/{name}"
    stamp = f"{os.path.getmtime(png):.0f}:{os.path.getsize(png)}"
    if not EAL.does_asset_exist(path) or pbr._stamps().get(path) != stamp:
        task = unreal.AssetImportTask()
        task.filename = png
        task.destination_path = folder
        task.destination_name = name
        task.automated = True
        task.replace_existing = True
        task.save = False
        TOOLS.import_asset_tasks([task])
        pbr._stamps()[path] = stamp
        pbr._save_stamps()
    tex = unreal.load_asset(path)
    T = unreal.TextureCompressionSettings
    for k, v in {"compression_settings": pbr._enum(T, "TC_BC7") or T.TC_DEFAULT, "srgb": True}.items():
        try:
            if tex.get_editor_property(k) != v:
                tex.set_editor_property(k, v)
        except Exception as e:  # noqa: BLE001
            warn(f"{name}.{k}: {e}")
    EAL.save_loaded_asset(tex, only_if_is_dirty=True)
    return tex


# The land (MuseeJourneyLibrary::BuildTerrain: UV0 metres from the tile's north-west corner, UV1 the tile's place in its
# 8 km orthophoto block). The de-lit orthophoto gives the colour and what lies where (snow and ice, rock and scree,
# the meadows); close to the eye the photographed sets give it grain and relief; where there is no photograph (Italy,
# the far land) the land is drawn from its height and slope.
TERRAIN_CLASS = r"""
float3 o = Ortho.rgb;
float cover = HasOrtho * Ortho.a;
float2 huv = UV0 * HiresScale + HiresOffset.xy;
float inH = HasHires * step(0.0, huv.x) * step(huv.x, 1.0) * step(0.0, huv.y) * step(huv.y, 1.0);
o = lerp(o, Hires.rgb, inH);
cover = max(cover, inH);
float lum = dot(o, float3(0.2126, 0.7152, 0.0722));
float mx = max(o.r, max(o.g, o.b)), mn = min(o.r, min(o.g, o.b));
float sat = (mx - mn) / (mx + 1e-3);
float snowO = smoothstep(0.60, 0.76, lum) * (1.0 - smoothstep(0.10, 0.28, sat));
float grassO = saturate((o.g - 0.5 * (o.r + o.b)) * 18.0) * (1.0 - snowO);
float n = frac(sin(dot(floor(UV0 / 180.0) + UV1 * 97.0, float2(12.9898, 78.233))) * 43758.5453);
float snowP = smoothstep(3000.0, 3400.0, Elev + (n - 0.5) * 250.0) * smoothstep(0.35, 0.6, NZ);
float grassP = (1.0 - smoothstep(2200.0, 2700.0, Elev)) * smoothstep(0.75, 0.9, NZ) * (1.0 - snowP);
float snow = lerp(snowP, snowO, cover);
float grass = lerp(grassP, grassO, cover);
// On steep faces the orthophoto is stretched and was taken in their shade: trust it less for the colour there.
float coverC = cover * lerp(0.45, 1.0, smoothstep(0.3, 0.62, NZ));
return float4(snow, grass, coverC, lum);
"""

TERRAIN_COLOUR = r"""
float snow = C.x, grass = C.y, cover = C.z;
float rock = saturate(1.0 - snow - grass);
float3 o = Ortho.rgb;
float2 huv = UV0 * HiresScale + HiresOffset.xy;
float inH = HasHires * step(0.0, huv.x) * step(huv.x, 1.0) * step(0.0, huv.y) * step(huv.y, 1.0);
o = lerp(o, Hires.rgb, inH);
float3 proc = RockTint.rgb * rock + SnowTint.rgb * snow + GrassTint.rgb * grass;
float3 base = lerp(proc, o, cover);
// Steep rock the flight saw in shade stays too dark after de-lighting: lift it towards gneiss's own albedo.
float steep = 1.0 - smoothstep(0.35, 0.65, NZ);
float lumB = dot(base, float3(0.2126, 0.7152, 0.0722));
base *= lerp(1.0, clamp(0.13 / max(lumB, 0.01), 1.0, 3.0), steep * rock);
// The grain of the photographed sets: their variation about their own mean, strongest close to the eye.
float near = 1.0 - smoothstep(40.0, 1400.0, Dist);
// (each set's own linear mean: its variation about 1)
float dr = pow(dot(DRock.rgb, float3(0.3, 0.4, 0.3)) / 0.071, 0.6);
float dr2 = pow(dot(DRock2.rgb, float3(0.3, 0.4, 0.3)) / 0.072, 0.6);
float ds = pow(dot(DSnow.rgb, float3(0.3, 0.4, 0.3)) / 0.86, 0.8);
float dg = pow(dot(DScree.rgb, float3(0.3, 0.4, 0.3)) / 0.121, 0.6);
float grain = rock * lerp(dr, dr * dr2, 0.5) + snow * ds + grass * dg;
float3 c = base * lerp(1.0, clamp(grain, 0.4, 1.7), near * Grain);
return saturate(c);
"""

TERRAIN_NORMAL = r"""
float snow = C.x, grass = C.y;
float rock = saturate(1.0 - snow - grass);
float near = 1.0 - smoothstep(60.0, 2500.0, Dist);
float3 n = NRock * rock + NSnow * snow + NScree * grass;
n.xy *= near * Strength;
return normalize(n);
"""

TERRAIN_ROUGH = r"""
float snow = C.x, grass = C.y;
float rock = saturate(1.0 - snow - grass);
return saturate(rock * (0.62 + 0.3 * RRock) + snow * (0.42 + 0.25 * RSnow) + grass * 0.9);
"""


def make_terrain_master():
    sets = {k: texture_set(k) for k in ("JR1", "JR2", "JR3", "JN1")}
    m = material("M_MH_Terrain", MH_MATS)
    g = Graph(m)
    uv0, uv1 = g.texcoord(0), g.texcoord(1)
    blank = unreal.load_asset("/Engine/EngineResources/Black")
    ortho_uv = g.custom("return UV1 + UV0 / 8000.0;", 2, [("UV0", uv0), ("UV1", uv1)], "Ortho UV")
    ortho = g.texture(blank, ortho_uv, "Ortho")
    hires_scale, hires_off = g.scalar("HiresScale", 0.001), g.vector("HiresOffset", (0, 0, 0))
    hires_uv = g.custom("return UV0 * S + O.xy;", 2, [("UV0", uv0), ("S", hires_scale), ("O", hires_off)], "Hires UV")
    hires = g.texture(blank, hires_uv, "Hires")
    lp = g.simple(unreal.MaterialExpressionLocalPosition)
    elev = g.custom("return P.z / 100.0;", 1, [("P", lp)], "Elevation")
    nz = g.custom("return N.z;", 1, [("N", g.simple(unreal.MaterialExpressionVertexNormalWS))], "Slope")
    cam = g.simple(unreal.MaterialExpressionCameraPositionWS)
    wp = g.world_position()
    dist = g.custom("return length(W - C) / 100.0;", 1, [("W", wp), ("C", cam)], "Distance")
    has_o, has_h = g.scalar("HasOrtho", 1.0), g.scalar("HasHires", 0.0)
    cls = g.custom(TERRAIN_CLASS, 4, [("Ortho", (ortho, "RGBA")), ("Hires", (hires, "RGBA")), ("HasOrtho", has_o), ("HasHires", has_h), ("HiresScale", hires_scale),
                                      ("HiresOffset", hires_off), ("UV0", uv0), ("UV1", uv1), ("Elev", elev), ("NZ", nz)], "Terrain classes")

    def duv(scale):
        return g.custom("return UV0 / S;", 2, [("UV0", uv0), ("S", g.const(scale))], f"Detail UV {scale}")
    u_r1, u_r2, u_s, u_n = duv(2.5), duv(10.0), duv(2.0), duv(2.5)
    d = {"DRock": g.texture(sets["JR1"]["Color"], u_r1), "DRock2": g.texture(sets["JR2"]["Color"], u_r2),
         "DScree": g.texture(sets["JR3"]["Color"], u_s), "DSnow": g.texture(sets["JN1"]["Color"], u_n)}
    colour = g.custom(TERRAIN_COLOUR, 3, [("C", cls), ("Ortho", (ortho, "RGBA")), ("Hires", (hires, "RGBA")), ("HasHires", has_h), ("HiresScale", hires_scale),
                                          ("HiresOffset", hires_off), ("UV0", uv0), ("Dist", dist), ("Grain", g.scalar("Grain", 1.0)), ("NZ", nz),
                                          ("RockTint", g.vector("RockTint", (0.16, 0.15, 0.14))), ("SnowTint", g.vector("SnowTint", (0.82, 0.84, 0.88))),
                                          ("GrassTint", g.vector("GrassTint", (0.10, 0.12, 0.06)))] + [(k, (v, "RGB")) for k, v in d.items()], "Terrain colour")
    nr = g.texture(sets["JR1"]["Normal"], u_r1)
    ns = g.texture(sets["JN1"]["Normal"], u_n)
    nc = g.texture(sets["JR3"]["Normal"], u_s)
    normal = g.custom(TERRAIN_NORMAL, 3, [("C", cls), ("Dist", dist), ("Strength", g.scalar("NormalStrength", 1.0)),
                                          ("NRock", (nr, "RGB")), ("NSnow", (ns, "RGB")), ("NScree", (nc, "RGB"))], "Terrain normal")
    rr = g.texture(sets["JR1"]["ORM"], u_r1)
    rs = g.texture(sets["JN1"]["ORM"], u_n)
    rough = g.custom(TERRAIN_ROUGH, 1, [("C", cls), ("RRock", (rr, "G")), ("RSnow", (rs, "G"))], "Terrain roughness")
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, colour)
    g.out(unreal.MaterialProperty.MP_NORMAL, normal)
    g.out(unreal.MaterialProperty.MP_ROUGHNESS, rough)
    g.out(unreal.MaterialProperty.MP_SPECULAR, g.scalar("Specular", 0.5))
    return finish(m)


def instance(name, parent, folder, scalars=None, vectors=None, textures=None):
    path = f"{folder}/{name}"
    if EAL.does_asset_exist(path):
        mi = unreal.load_asset(path)
    else:
        mi = TOOLS.create_asset(name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, parent)
    for k, v in (scalars or {}).items():
        MEL.set_material_instance_scalar_parameter_value(mi, k, float(v))
    for k, v in (vectors or {}).items():
        MEL.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
    for k, v in (textures or {}).items():
        if v is not None:
            MEL.set_material_instance_texture_parameter_value(mi, k, v)
    MEL.update_material_instance(mi)
    EAL.save_loaded_asset(mi)
    return mi


def make_matterhorn(only="", build=True):
    import json
    with open(os.path.join(TERRAIN_DIR, "terrain.json"), encoding="utf-8") as f:
        terrain = json.load(f)
    images = os.path.join(TERRAIN_DIR, "images")
    tex = {}
    for b in terrain["blocks"] + terrain["hires"]:
        png = os.path.join(images, b["name"] + ".png")
        if os.path.exists(png):
            tex[b["name"]] = import_image(png, MH + "/Textures", b["name"])
    master = make_terrain_master()
    for b in terrain["blocks"]:
        if b["name"] in tex:
            instance(b["name"].replace("T_MH_", "MI_MH_"), master, MH_MATS, {"HasOrtho": 1, "HasHires": 0}, textures={"Ortho": tex[b["name"]]})
    for h in terrain["hires"]:
        e, n = h["name"].split("_")[-2:]
        bx = (int(e) * 1000 - 2608000) // 8000
        by = (1101000 - (int(n) + 1) * 1000) // 8000
        instance(f"MI_MH_Hires_{e}_{n}", master, MH_MATS, {"HasOrtho": 1, "HasHires": 1, "HiresScale": 0.001}, {"HiresOffset": (0, 0, 0)},
                 {"Ortho": tex.get(f"T_MH_Ortho_{bx}{by}"), "Hires": tex.get(h["name"])})
    for t in terrain["tiles"]:
        if t["kind"] != "patch":
            continue
        # A patch's high-resolution image: the 1 km tile it lies in (the patches are 300 m, inside one).
        e, n = int(t["west"] // 1000), int((t["north"] - 1) // 1000)
        hn = f"T_MH_Hires_{e}_{n}"
        off = ((t["west"] - e * 1000) / 1000.0, ((n + 1) * 1000 - t["north"]) / 1000.0, 0)
        instance("MI_MH_" + t["name"], master, MH_MATS, {"HasOrtho": 1, "HasHires": 1 if hn in tex else 0, "HiresScale": 0.001},
                 {"HiresOffset": off}, {"Ortho": tex.get(t["block"]), "Hires": tex.get(hn)})
    instance("MI_MH_Far", master, MH_MATS, {"HasOrtho": 0, "HasHires": 0})
    # Riffelsee: clear, cold, a little green; still at dawn (a mirror for the mountain).
    water = unreal.load_asset(f"{P.MATERIALS}/M_Water")
    if water:
        instance("MI_MH_Lake", water, MH_MATS, {"Ripple": 0.015, "Roughness": 0.015},
                 {"Absorption": (0.0045, 0.0016, 0.0022), "Scattering": (0.0003, 0.0007, 0.0005)})
    if build:
        n = unreal.MuseeJourneyLibrary.build_terrain(os.path.join(TERRAIN_DIR, "terrain.json"), MH + "/Terrain", MH_MATS, only)
        log(f"Matterhorn: {n} terrain packages saved")


# Glacier ice (the crevasse: vertex colour R the depth below the lip, 0 … 1 over 44 m; G a dirt band; B the firn near the
# top). Blue ice glows: daylight enters from the surface and scatters metres through it, bluer and dimmer the deeper
# (IceGlow, from the sun, set by the journey); the wall itself is glossy with melt, scalloped, banded near the top.
ICE_COLOUR = r"""
// Glacier ice as it looks in a crevasse (the references: smooth, glassy, flowing bulges; pale cyan where the light comes
// through near the lip and on the faces turned up to the sky, deep blue in the recesses and below).
float depth = VC.r * 44.0;
float3 pale = float3(0.62, 0.86, 0.92);
float3 ice = float3(0.22, 0.55, 0.74);
float3 deep = float3(0.04, 0.16, 0.32);
float up = saturate(N.z * 0.5 + 0.5);
float3 c = lerp(ice, deep, saturate(depth / 26.0));
c = lerp(c, pale, saturate((1.0 - depth / 12.0)) * 0.6 + up * 0.25);
c = lerp(c, float3(0.80, 0.86, 0.90), VC.b * 0.8);
// Dirt: a band of fine dark specks.
float2 q = UV * 14.0;
float speck = step(0.975, frac(sin(dot(floor(q), float2(12.9898, 78.233))) * 43758.5453));
c = lerp(c, float3(0.30, 0.30, 0.28), saturate(VC.g * 1.5) * (0.15 + 0.25 * speck));
return c;
"""

ICE_NORMAL = r"""
// Melt scallops: shallow concave cups a hand across (a cellular field: each cup's floor at its cell's point, sharp low
// rims between), over a gentler field three times larger; both fade out where they would shimmer (far, grazing).
float3 n = float3(0, 0, 1);
[unroll] for (int k = 0; k < 2; k++)
{
    float size = k == 0 ? 0.16 : 0.5;
    float2 q = UV / size + k * 17.3;
    float fade = saturate(1.6 - length(fwidth(q)) * 4.0);
    float2 cell = floor(q);
    float2 best = 0; float bd = 9;
    [unroll] for (int y = -1; y <= 1; y++) [unroll] for (int x = -1; x <= 1; x++)
    {
        float2 c = cell + float2(x, y);
        float2 r = frac(sin(float2(dot(c, float2(127.1, 311.7)), dot(c, float2(269.5, 183.3)))) * 43758.5453);
        float2 d = c + r - q;
        float l = dot(d, d);
        if (l < bd) { bd = l; best = d; }
    }
    n.xy += best * (k == 0 ? 0.5 : 0.3) * Strength * fade;
}
return normalize(n);
"""

ICE_GLOW = r"""
float depth = VC.r * 44.0;
float3 tint = lerp(float3(0.35, 0.70, 0.95), float3(0.02, 0.16, 0.50), saturate(depth / 24.0));
float k = exp(-depth / 8.0) * (1.0 - 0.8 * VC.g) * (1.0 - 0.6 * VC.b);
return tint * k * Glow * Nits;
"""


def make_ice(mpc):
    m = material("M_MH_Ice", MH_MATS)
    g = Graph(m)
    uv, vc = g.texcoord(0), g.simple(unreal.MaterialExpressionVertexColor)
    nws = g.simple(unreal.MaterialExpressionVertexNormalWS)
    colour = g.custom(ICE_COLOUR, 3, [("VC", vc), ("UV", uv), ("N", nws)], "Ice colour")
    glow = g.custom(ICE_GLOW, 3, [("VC", vc), ("Glow", g.mpc(mpc, "IceGlow")), ("Nits", g.scalar("GlowNits", 45.0))], "Ice glow")
    rough = g.custom("return lerp(0.16, 0.5, VC.b) + 0.1 * VC.g;", 1, [("VC", vc)], "Ice roughness")
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, colour)
    g.out(unreal.MaterialProperty.MP_EMISSIVE_COLOR, glow)
    g.out(unreal.MaterialProperty.MP_NORMAL, g.custom(ICE_NORMAL, 3, [("UV", uv), ("Strength", g.scalar("ScallopStrength", 0.2))], "Ice scallops"))
    g.out(unreal.MaterialProperty.MP_ROUGHNESS, rough)
    g.out(unreal.MaterialProperty.MP_SPECULAR, g.scalar("Specular", 0.3))
    return finish(m)


def make_snow():
    snow = texture_set("JN1")
    m = material("M_MH_Snow", MH_MATS)
    g = Graph(m)
    uv = g.texcoord(0)
    duv = g.custom("return UV / 2.5;", 2, [("UV", uv)], "Snow UV")
    c = g.texture(snow["Color"], duv)
    colour = g.custom("float l = dot(C.rgb, float3(0.3, 0.4, 0.3)); return Tint.rgb * lerp(0.9, 1.05, l);", 3,
                      [("C", (c, "RGB")), ("Tint", g.vector("Tint", (0.86, 0.88, 0.91)))], "Snow colour")
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, colour)
    g.out(unreal.MaterialProperty.MP_NORMAL, g.texture(snow["Normal"], duv))
    g.out(unreal.MaterialProperty.MP_ROUGHNESS, g.custom("return 0.45 + 0.3 * R;", 1, [("R", (g.texture(snow["ORM"], duv), "G"))], "Snow roughness"))
    g.out(unreal.MaterialProperty.MP_SPECULAR, g.scalar("Specular", 0.5))
    return finish(m)


# The huts' surfaces (JourneyGen's buildings: UV0 metres on every face): one master, a photographed set per instance,
# tinted to its colour; vertical ribs for the annex's cladding, the cells of the solar panels, the Valais flag.
SURFACE_COLOUR = r"""
float3 t = C.rgb;
float g = dot(t, float3(0.3, 0.4, 0.3)) / max(Mean, 1e-3);
float3 c = Tint.rgb * lerp(1.0, g, TexAmount);
if (Flag > 0.5) { c = UV.x < 0.5 ? float3(0.75, 0.03, 0.04) : float3(0.82, 0.82, 0.80); }
if (Cells > 0.5) { float2 f = frac(UV / float2(0.156, 0.156)); float l = step(0.04, f.x) * step(0.04, f.y); c = lerp(float3(0.55, 0.56, 0.58), Tint.rgb, l); }
return c;
"""

SURFACE_NORMAL = r"""
float3 n = N;
n.xy *= NormalAmount;
if (Ribs > 0.0) { float p = frac(UV.x / RibPitch); n.x += Ribs * (p < 0.15 ? -1.0 : (p < 0.3 ? 1.0 : 0.0)); }
return normalize(n);
"""


def make_surface_master():
    m = material("M_MH_Surface", MH_MATS)
    g = Graph(m)
    uv = g.texcoord(0)
    scale = g.scalar("Scale", 1.0)
    tuv = g.custom("return UV / S;", 2, [("UV", uv), ("S", scale)], "Texture UV")
    grey = unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture")
    flat = unreal.load_asset("/Engine/EngineMaterials/FlatNormal")
    c = g.texture(grey, tuv, "Colour")
    n = g.texture(flat, tuv, "Normal")
    o = g.texture(grey, tuv, "ORM")
    colour = g.custom(SURFACE_COLOUR, 3, [("C", (c, "RGB")), ("UV", uv), ("Tint", g.vector("Tint", (0.5, 0.5, 0.5))), ("TexAmount", g.scalar("TexAmount", 1.0)),
                                          ("Mean", g.scalar("Mean", 0.5)), ("Flag", g.scalar("Flag", 0.0)), ("Cells", g.scalar("Cells", 0.0))], "Surface colour")
    normal = g.custom(SURFACE_NORMAL, 3, [("N", (n, "RGB")), ("UV", uv), ("NormalAmount", g.scalar("NormalAmount", 1.0)), ("Ribs", g.scalar("Ribs", 0.0)),
                                          ("RibPitch", g.scalar("RibPitch", 0.2))], "Surface normal")
    rough = g.custom("return saturate(R + (O - 0.5) * RoughTex);", 1, [("R", g.scalar("Roughness", 0.7)), ("O", (o, "G")), ("RoughTex", g.scalar("RoughTex", 0.5))], "Surface roughness")
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, colour)
    g.out(unreal.MaterialProperty.MP_NORMAL, normal)
    g.out(unreal.MaterialProperty.MP_ROUGHNESS, rough)
    g.out(unreal.MaterialProperty.MP_METALLIC, g.scalar("Metallic", 0.0))
    g.out(unreal.MaterialProperty.MP_SPECULAR, g.scalar("Specular", 0.5))
    return finish(m)


def surface(name, master, texset=None, museum=False, **p):
    tex = {}
    if texset:
        t = pbr.textures(texset) if museum else texture_set(texset)
        tex = {"Colour": t.get("Color"), "Normal": t.get("Normal"), "ORM": t.get("ORM")}
    scal = {k: v for k, v in p.items() if not isinstance(v, tuple)}
    vec = {k: v for k, v in p.items() if isinstance(v, tuple)}
    if not texset:
        scal.setdefault("TexAmount", 0.0)
        scal.setdefault("NormalAmount", 0.0)
    return instance(name, master, MH_MATS, scal, vec, tex)


def make_hut_surfaces():
    s = make_surface_master()
    surface("MI_MH_Render", s, "P1", True, Tint=(0.70, 0.64, 0.52), Scale=2.0, Roughness=0.86, TexAmount=0.8, Mean=0.662)
    surface("MI_MH_Cladding", s, "M2", True, Tint=(0.045, 0.047, 0.05), Scale=1.0, Roughness=0.42, RoughTex=0.3, Metallic=0.8, Ribs=0.6, RibPitch=0.25, Mean=0.306)
    surface("MI_MH_Frame", s, "W2", True, Tint=(0.09, 0.06, 0.04), Scale=1.8, Roughness=0.55, TexAmount=0.6, Mean=0.224)
    surface("MI_MH_WindowGlass", s, None, Tint=(0.02, 0.022, 0.025), Roughness=0.04, Specular=0.5)
    surface("MI_MH_RoofMetal", s, "M2", True, Tint=(0.30, 0.31, 0.32), Scale=1.0, Roughness=0.38, Metallic=0.85, Ribs=0.5, RibPitch=0.5, Mean=0.306)
    surface("MI_MH_StoneWall", s, "JR3", Tint=(0.33, 0.32, 0.30), Scale=1.5, Roughness=0.8, Mean=0.121)
    surface("MI_MH_Boulder", s, "JR1", Tint=(0.26, 0.25, 0.235), Scale=1.2, Roughness=0.82, TexAmount=1.0, Mean=0.071)
    surface("MI_MH_Timber", s, "W2", True, Tint=(0.33, 0.24, 0.17), Scale=1.8, Roughness=0.75, TexAmount=0.9, Mean=0.224)
    surface("MI_MH_Steel", s, None, Tint=(0.35, 0.35, 0.36), Roughness=0.45, Metallic=1.0)
    surface("MI_MH_Solar", s, None, Tint=(0.02, 0.03, 0.07), Roughness=0.12, Specular=0.6, Cells=1.0)
    surface("MI_MH_Flag", s, None, Tint=(0.8, 0.8, 0.8), Roughness=0.85, Flag=1.0)
    surface("MI_MH_Paint", s, "W2", True, Tint=(0.30, 0.10, 0.055), Scale=1.2, Roughness=0.7, TexAmount=0.5, Mean=0.224)
    for name, tint in (("MI_MH_JacketRed", (0.55, 0.03, 0.03)), ("MI_MH_JacketBlue", (0.02, 0.10, 0.45)), ("MI_MH_JacketOrange", (0.8, 0.25, 0.02)),
                       ("MI_MH_Clothes", (0.03, 0.03, 0.035)), ("MI_MH_Helmet", (0.75, 0.75, 0.72))):
        surface(name, s, None, Tint=tint, Roughness=0.45 if "Helmet" in name else 0.6)
    surface("MI_MH_Rope", s, None, Tint=(0.18, 0.19, 0.2), Roughness=0.9)
    # Snowflakes and spindrift: small round white motes (the engine's plane, masked to a disc), lit, two-sided.
    flake = material("M_MH_Flake", MH_MATS)
    setp(flake, "blend_mode", unreal.BlendMode.BLEND_MASKED)
    setp(flake, "two_sided", True)
    g = Graph(flake)
    uv = g.texcoord(0)
    g.out(unreal.MaterialProperty.MP_OPACITY_MASK, g.custom("return 1.0 - step(0.5, length(UV - 0.5));", 1, [("UV", uv)], "Disc"))
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, g.vector("Colour", (0.9, 0.92, 0.95)))
    g.out(unreal.MaterialProperty.MP_ROUGHNESS, g.scalar("Roughness", 0.5))
    finish(flake)
    lamp = material("M_MH_Lamp", MH_MATS)
    setp(lamp, "shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    g = Graph(lamp)
    g.out(unreal.MaterialProperty.MP_EMISSIVE_COLOR, g.custom("return Colour.rgb * Nits;", 3, [("Colour", g.vector("Colour", (1.0, 0.62, 0.3))), ("Nits", g.scalar("Nits", 400.0))], "Lamp"))
    finish(lamp)


# The sea of clouds (places 5 and 6: an inversion's stratus filling the valleys, its top near 3,000 m, rising and
# falling; the peaks stand out of it). A volumetric cloud layer's density: the sample's height in the layer
# (NormAltitudeInLayer), the engine's Perlin–Worley volume for the top's billows and the holes.
CLOUD_DENSITY = r"""
float2 xy = P.xy / 100.0;
float big = Tex.SampleLevel(TexSampler, float3(xy / 5200.0, 0.21), 0).r;
float mid = Tex.SampleLevel(TexSampler, float3(xy / 1300.0, 0.57), 0).r;
float top = 0.62 + 0.20 * big + 0.12 * (mid - 0.5);
float body = saturate((top - H) / 0.06) * saturate(H / 0.08);
float cover = saturate(0.55 + (big - 0.35) * 2.2);
float d = body * cover;
float fine = Tex.SampleLevel(TexSampler, float3(xy / 700.0, H * 1.5) + float3(T * 0.0005, T * 0.0002, 0), 0).r;
float nearTop = 1.0 - saturate((top - H) / 0.22);
d = saturate(d - fine * 0.12 * nearTop - (1.0 - cover) * 0.3);
return d * Density;
"""


def make_cloud_sea():
    m = material("M_MH_CloudSea", MH_MATS)
    setp(m, "material_domain", unreal.MaterialDomain.MD_VOLUME)
    setp(m, "blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    usage(m, "MATUSAGE_VOLUMETRIC_CLOUD", "used_with_volumetric_cloud")
    g = Graph(m)
    wp = g.world_position()
    attr = g.simple(unreal.MaterialExpressionCloudSampleAttribute)
    tobj = g.expr(unreal.MaterialExpressionTextureObject)
    tobj.set_editor_property("texture", unreal.load_asset("/Engine/EngineSky/VolumetricClouds/T_Volume_PerlinWorley_Balanced"))
    dens = g.custom(CLOUD_DENSITY, 1, [("P", wp), ("H", (attr, "NormAltitudeInLayer")), ("Tex", tobj), ("T", g.simple(unreal.MaterialExpressionTime)),
                                        ("Density", g.scalar("Density", 0.06))], "Cloud density")
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, g.vector("Albedo", (1.0, 1.0, 1.0)))
    g.out(unreal.MaterialProperty.MP_SUBSURFACE_COLOR, dens)
    try:
        adv = g.expr(unreal.MaterialExpressionVolumetricAdvancedMaterialOutput)
        for k, v in (("gray_scale_material", True), ("multi_scattering_approximation_octave_count", 2), ("const_multi_scattering_contribution", 0.5),
                     ("const_multi_scattering_occlusion", 0.5), ("const_multi_scattering_eccentricity", 0.6), ("const_phase_g", 0.6),
                     ("const_phase_g2", -0.4), ("const_phase_blend", 0.5)):
            try:
                adv.set_editor_property(k, v)
            except Exception as e:  # noqa: BLE001
                warn(f"cloud {k}: {e}")
    except Exception as e:  # noqa: BLE001
        warn(f"no volumetric advanced output: {e}")
    return finish(m)


def make_matterhorn_things():
    mpc = cube_collection()
    instance("MI_MH_Ice", make_ice(mpc), MH_MATS)
    instance("MI_MH_Snow", make_snow(), MH_MATS)
    make_hut_surfaces()
    make_cloud_sea()
    n = unreal.MuseeJourneyLibrary.build_things("matterhorn", MH + "/Things", MH_MATS)
    log(f"Matterhorn: {n} things saved")


# ---------------------------------------------------------------------------------------------- Raja Ampat

RA = ROOT + "/Sea"
RA_MATS = RA + "/Materials"

# Under water (a post-process before depth of field): the light's path through the sea from what is seen to the eye is
# attenuated per channel (clear Coral Triangle water, Jerlov type I: red gone within a few metres, blue carried 30 m),
# and the sea's own glow is scattered into it (brighter looking up, darker with depth). With the eyes above the water
# (the surface places), only the part of the path under it. MPC_Cube: WaterLevel (world cm), WaterOn, SunNits.
UNDERWATER = r"""
// The ray from the camera (CV points to the camera); the distance clamped (the sky's depth is infinite: no NaN).
float3 d = -normalize(CV);
float D = length(WP - Cam);
if (!(D < 1e8)) D = 1e8;
float L;
if (Cam.z < Level) { float ts = d.z > 1e-5 ? (Level - Cam.z) / d.z : 1e9; L = min(D, ts); }
else { float ti = d.z < -1e-5 ? (Level - Cam.z) / d.z : 1e9; L = max(0.0, D - ti); }
L = min(L, 4e5) / 100.0;
float3 sigma = float3(0.42, 0.080, 0.048);
float3 T = exp(-sigma * L);
float depthEye = max(0.0, (Level - Cam.z) / 100.0);
float up = saturate(d.z * 0.5 + 0.5);
float3 glow = float3(0.008, 0.085, 0.15) * exp(-float3(0.25, 0.045, 0.030) * depthEye) * lerp(0.18, 2.2, up * up * up);
float3 inscat = glow * Nits * View.PreExposure;
float3 c = Scene * T + inscat * (1.0 - T);
return lerp(Scene, c, On);
"""


def make_underwater(mpc):
    m = material("M_RA_Underwater", RA_MATS)
    setp(m, "material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    loc = pbr._enum(unreal.BlendableLocation, "BL_SCENE_COLOR_BEFORE_DOF", "BL_BEFORE_TRANSLUCENCY")
    if loc is not None:
        setp(m, "blendable_location", loc)
    g = Graph(m)
    scene = g.expr(unreal.MaterialExpressionSceneTexture)
    scene.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    wp = g.world_position()
    cam = g.simple(unreal.MaterialExpressionCameraPositionWS)
    c = g.custom(UNDERWATER, 3, [("Scene", (scene, "Color")), ("WP", wp), ("Cam", cam), ("CV", g.simple(unreal.MaterialExpressionCameraVectorWS)), ("Level", g.mpc(mpc, "WaterLevel")),
                                  ("On", g.mpc(mpc, "WaterOn")), ("Nits", g.mpc(mpc, "SunNits"))], "Under water")
    g.out(unreal.MaterialProperty.MP_EMISSIVE_COLOR, c)
    return finish(m)


# The sea's surface, from above and from below: waves (a sum of directional trains, for the displacement and the normal);
# a mirror by Fresnel (water's 2 % face on), and from below total internal reflection beyond 48.6° from the vertical
# (Snell's window: inside it the sky, outside it the sea's own floor mirrored).
WAVES = r"""
float2 p = WP.xy / 100.0;
float t = Time;
float h = 0; float2 g = 0;
float2 dirs[6] = { float2(1, 0.2), float2(0.7, 0.7), float2(-0.3, 1), float2(0.9, -0.45), float2(-0.8, 0.5), float2(0.2, -1) };
float lens[6] = { 7.3, 4.1, 2.7, 1.6, 0.93, 0.55 };
float amps[6] = { 0.045, 0.028, 0.017, 0.009, 0.005, 0.0025 };
[unroll] for (int i = 0; i < 6; i++)
{
    float2 dd = normalize(dirs[i]);
    float k = 6.2831853 / lens[i];
    float w = sqrt(9.81 * k);
    float ph = dot(dd, p) * k - w * t;
    h += amps[i] * sin(ph) * Calm;
    g += amps[i] * k * cos(ph) * dd * Calm;
}
return float3(g, h);
"""

SURFACE_OPACITY = r"""
float3 n = normalize(float3(-W.x, -W.y, 1.0));
float3 V = normalize(Cam - WP);
float c = dot(n, V);
float F;
if (Side > 0) { c = saturate(c); F = 0.02 + 0.98 * pow(1.0 - c, 5.0); }
else
{
    c = saturate(-c);
    float st2 = (1.33 * 1.33) * (1.0 - c * c);
    if (st2 >= 1.0) { F = 1.0; }
    else { float ct = sqrt(1.0 - st2); float rs = (1.33 * c - ct) / (1.33 * c + ct); float rp = (c - 1.33 * ct) / (c + 1.33 * ct); F = 0.5 * (rs * rs + rp * rp); }
}
return saturate(F);
"""


def make_sea_surface(mpc):
    m = material("M_RA_Surface", RA_MATS)
    setp(m, "blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    setp(m, "two_sided", True)
    setp(m, "translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    setp(m, "tangent_space_normal", False)
    g = Graph(m)
    wp = g.world_position()
    waves = g.custom(WAVES, 3, [("WP", wp), ("Time", g.simple(unreal.MaterialExpressionTime)), ("Calm", g.scalar("Calm", 1.0))], "Waves")
    cam = g.simple(unreal.MaterialExpressionCameraPositionWS)
    side = g.simple(unreal.MaterialExpressionTwoSidedSign)
    opac = g.custom(SURFACE_OPACITY, 1, [("W", waves), ("WP", wp), ("Cam", cam), ("Side", side)], "Fresnel and Snell")
    normal = g.custom("return normalize(float3(-W.x, -W.y, 1.0)) * Side;", 3, [("W", waves), ("Side", side)], "Surface normal")
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, g.vector("Colour", (1.0, 1.0, 1.0)))
    g.out(unreal.MaterialProperty.MP_METALLIC, g.const(1.0))
    g.out(unreal.MaterialProperty.MP_ROUGHNESS, g.scalar("Roughness", 0.03))
    g.out(unreal.MaterialProperty.MP_OPACITY, opac)
    g.out(unreal.MaterialProperty.MP_NORMAL, normal)
    g.out(unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET, g.custom("return float3(0, 0, W.z * 100.0);", 3, [("W", waves)], "Swell"))
    return finish(m)


# Caustics: the light function on the sun under water (the surface's waves focus the sunlight into a moving net of
# bright lines on everything below, fading with depth); above the water, plain light.
CAUSTICS = r"""
float2 p = WP.xy / 100.0;
float t = Time;
float net = 1.0;
[unroll] for (int layer = 0; layer < 2; layer++)
{
    float sc = layer == 0 ? 0.85 : 1.37;
    float2 q = p * sc + float2(t * 0.11, t * 0.07) * (layer == 0 ? 1.0 : -0.8);
    float2 cell = floor(q);
    float f1 = 9, f2 = 9;
    [unroll] for (int y = -1; y <= 1; y++) [unroll] for (int x = -1; x <= 1; x++)
    {
        float2 c = cell + float2(x, y);
        float2 r = frac(sin(float2(dot(c, float2(127.1, 311.7)), dot(c, float2(269.5, 183.3)))) * 43758.5453);
        r = 0.5 + 0.42 * sin(t * 0.9 + 6.2831 * r);
        float dd = length(c + r - q);
        if (dd < f1) { f2 = f1; f1 = dd; } else if (dd < f2) { f2 = dd; }
    }
    float edge = 1.0 - smoothstep(0.0, 0.16, f2 - f1);
    net = net * (0.35 + edge);
}
float depth = max(0.0, (Level - WP.z) / 100.0);
float under = step(WP.z, Level);
float strength = under * saturate(depth / 0.6) * exp(-depth / 18.0);
float c = lerp(0.62, saturate(0.18 + net * 0.75), strength);
return lerp(1.0, c, On);
"""


def make_caustics(mpc):
    m = material("M_RA_Caustics", RA_MATS)
    setp(m, "material_domain", unreal.MaterialDomain.MD_LIGHT_FUNCTION)
    g = Graph(m)
    c = g.custom(CAUSTICS, 1, [("WP", g.world_position()), ("Time", g.simple(unreal.MaterialExpressionTime)), ("Level", g.mpc(mpc, "WaterLevel")),
                               ("On", g.mpc(mpc, "WaterOn"))], "Caustics")
    g.out(unreal.MaterialProperty.MP_EMISSIVE_COLOR, c)
    return finish(m)


# The fishes (SeaGen: vertex colour R the position from the snout (0) to the tail's tip (1), G the height across the
# body (−1 belly … +1 back), B fins, A eyes). They swim by a wave travelling down the body (world position offset,
# lateral, growing towards the tail), each at its own phase and pace (per-instance random).
FISH_SWIM = r"""
float t01 = VC.r;
float ph = Rand * 6.2831853;
float w = Freq * (0.8 + 0.4 * Rand);
float a = Amp * Length * (0.08 + 0.92 * t01 * t01);
float off = a * sin(6.2831853 * 1.1 * t01 - w * Time + ph);
return float3(0, off, 0);
"""

FISH_COLOUR = r"""
float t = VC.r, h = VC.g, fin = VC.b, eye = VC.a;
float3 c = lerp(Belly.rgb, Back.rgb, smoothstep(-0.6, 0.6, h));
if (Pattern == 1) { c = lerp(c, Stripe.rgb, smoothstep(0.35, 0.5, h) * step(0.25, t)); c = lerp(c, Stripe.rgb, step(0.82, t)); }
if (Pattern == 2) { float bars = step(0.5, frac(t * Bars)) * step(0.15, t) * step(t, 0.85); c = lerp(c, Stripe.rgb, bars * smoothstep(-0.2, 0.3, h)); }
if (Pattern == 3) { float2 q = float2(t * 38.0, h * 7.0); float sp = step(0.72, frac(sin(dot(floor(q), float2(12.9, 78.2))) * 43758.5)); float2 f = frac(q) - 0.5; c = lerp(c, Stripe.rgb, sp * step(length(f), 0.3)); }
if (Pattern == 4) { c = lerp(c, Stripe.rgb, step(abs(t - 0.14), 0.035)); c = lerp(c, Stripe.rgb, step(0.9, t) * 0.8); }
float3 fc = lerp(FinColour.rgb, c, 0.25);
if (Pattern == 5) { fc = lerp(fc, Stripe.rgb, smoothstep(0.75, 0.95, abs(h))); }
c = lerp(c, fc, fin);
c = lerp(c, float3(0.01, 0.01, 0.012), eye);
return c;
"""


def make_fish_master():
    m = material("M_RA_Fish", RA_MATS)
    g = Graph(m)
    vc = g.simple(unreal.MaterialExpressionVertexColor)
    vca = (vc, "")
    rnd = g.simple(unreal.MaterialExpressionPerInstanceRandom)
    swim = g.custom(FISH_SWIM, 3, [("VC", vca), ("Rand", rnd), ("Time", g.simple(unreal.MaterialExpressionTime)), ("Amp", g.scalar("SwimAmp", 0.09)),
                                   ("Freq", g.scalar("SwimFreq", 9.0)), ("Length", g.scalar("LengthCm", 30.0))], "Swim")
    tr = g.expr(unreal.MaterialExpressionTransform)
    tr.set_editor_property("transform_source_type", unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL)
    tr.set_editor_property("transform_type", unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
    MEL.connect_material_expressions(swim, "", tr, "Input")
    inputs = [("VC", vca), ("Pattern", g.scalar("Pattern", 0)), ("Bars", g.scalar("Bars", 6.0)), ("Back", g.vector("Back", (0.2, 0.3, 0.4))),
              ("Belly", g.vector("Belly", (0.8, 0.8, 0.8))), ("Stripe", g.vector("Stripe", (0.9, 0.7, 0.1))), ("FinColour", g.vector("FinColour", (0.5, 0.5, 0.5)))]
    # VC.a (the eyes) needs the alpha: pass the four channels.
    vc4 = g.custom("return float4(C.rgb, A);", 4, [("C", (vc, "")), ("A", (vc, "A"))], "Vertex colour")
    inputs[0] = ("VC", vc4)
    colour = g.custom(FISH_COLOUR, 3, inputs, "Fish colour")
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, colour)
    g.out(unreal.MaterialProperty.MP_METALLIC, g.scalar("Silver", 0.2))
    g.out(unreal.MaterialProperty.MP_ROUGHNESS, g.scalar("Roughness", 0.32))
    g.out(unreal.MaterialProperty.MP_SPECULAR, g.scalar("Specular", 0.6))
    g.out(unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET, tr)
    setp(m, "used_with_nanite", False)   # plain instanced meshes (SeaGen: their swimming offset failed in Nanite's raster)
    return finish(m)


FISH = {
    # name: pattern, back, belly, stripe, fin, silver, length cm, swim amp, swim freq
    "Fusilier": (1, (0.05, 0.22, 0.55), (0.85, 0.88, 0.95), (0.95, 0.72, 0.05), (0.9, 0.7, 0.1), 0.35, 28, 0.08, 11.0),
    "Anthias": (0, (0.95, 0.35, 0.12), (0.95, 0.55, 0.35), (0.9, 0.2, 0.5), (0.95, 0.4, 0.2), 0.1, 10, 0.1, 14.0),
    "Trevally": (0, (0.32, 0.36, 0.38), (0.85, 0.87, 0.88), (0.1, 0.1, 0.1), (0.25, 0.27, 0.3), 0.7, 65, 0.06, 6.0),
    "Barracuda": (2, (0.35, 0.39, 0.42), (0.86, 0.88, 0.9), (0.12, 0.13, 0.15), (0.1, 0.1, 0.12), 0.7, 110, 0.05, 4.0),
    "Butterfly": (4, (0.95, 0.85, 0.2), (0.95, 0.93, 0.85), (0.02, 0.02, 0.03), (0.95, 0.8, 0.1), 0.05, 15, 0.07, 8.0),
    "Sweetlips": (3, (0.9, 0.88, 0.8), (0.92, 0.9, 0.85), (0.08, 0.07, 0.06), (0.95, 0.8, 0.3), 0.1, 45, 0.07, 6.0),
    "Batfish": (2, (0.6, 0.62, 0.6), (0.85, 0.85, 0.82), (0.12, 0.12, 0.12), (0.2, 0.2, 0.2), 0.4, 45, 0.05, 4.0),
    "Parrot": (0, (0.1, 0.55, 0.45), (0.3, 0.7, 0.75), (0.8, 0.4, 0.7), (0.2, 0.6, 0.8), 0.1, 45, 0.07, 7.0),
    "Chromis": (0, (0.35, 0.85, 0.75), (0.55, 0.9, 0.85), (0.3, 0.8, 0.9), (0.4, 0.85, 0.85), 0.3, 8, 0.1, 16.0),
    "BlacktipShark": (5, (0.42, 0.40, 0.36), (0.9, 0.88, 0.85), (0.02, 0.02, 0.02), (0.4, 0.38, 0.35), 0.05, 140, 0.07, 3.5),
    "WalkingShark": (3, (0.55, 0.42, 0.28), (0.85, 0.78, 0.65), (0.15, 0.1, 0.06), (0.55, 0.42, 0.28), 0.0, 70, 0.1, 3.0),
}


# The manta (SeaGen: R across the span 0 … 1, G along the chord −1 tail … +1 front, B the belly, A gill slits and mouth):
# black above with the reef manta's two pale shoulder patches, white below with its own dark spots; its wings flap in a
# wave from root to tip.
MANTA_FLAP = r"""
float s = VC.r;
float a = 60.0 * pow(s, 1.6);
float w = 6.2831853 / 5.2;
return float3(0, 0, a * sin(w * Time - s * 1.6 + Rand * 6.28));
"""

MANTA_COLOUR = r"""
float s = VC.r, x = VC.g, belly = VC.b, gill = VC.a;
float3 top = float3(0.015, 0.016, 0.018);
float patch = smoothstep(0.18, 0.28, s) * (1.0 - smoothstep(0.5, 0.62, s)) * smoothstep(-0.1, 0.2, x) * (1.0 - smoothstep(0.55, 0.75, x));
top = lerp(top, float3(0.55, 0.56, 0.55), patch * 0.85);
float3 bot = float3(0.82, 0.83, 0.82);
float2 q = UV * 3.0;
float sp = step(0.8, frac(sin(dot(floor(q), float2(12.9, 78.2))) * 43758.5)) * step(length(frac(q) - 0.5), 0.3);
bot = lerp(bot, float3(0.05, 0.05, 0.06), sp * smoothstep(0.2, 0.5, s));
bot = lerp(bot, float3(0.15, 0.15, 0.16), smoothstep(0.85, 1.0, s));
float3 c = lerp(top, bot, belly);
return lerp(c, float3(0.03, 0.03, 0.035), gill);
"""


def make_manta():
    m = material("M_RA_Manta", RA_MATS)
    g = Graph(m)
    vc = g.simple(unreal.MaterialExpressionVertexColor)
    vc4 = g.custom("return float4(C.rgb, A);", 4, [("C", (vc, "")), ("A", (vc, "A"))], "Vertex colour")
    flap = g.custom(MANTA_FLAP, 3, [("VC", vc4), ("Time", g.simple(unreal.MaterialExpressionTime)), ("Rand", g.simple(unreal.MaterialExpressionPerInstanceRandom))], "Flap")
    tr = g.expr(unreal.MaterialExpressionTransform)
    tr.set_editor_property("transform_source_type", unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL)
    tr.set_editor_property("transform_type", unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
    MEL.connect_material_expressions(flap, "", tr, "Input")
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, g.custom(MANTA_COLOUR, 3, [("VC", vc4), ("UV", g.texcoord(0))], "Manta colour"))
    g.out(unreal.MaterialProperty.MP_ROUGHNESS, g.scalar("Roughness", 0.45))
    g.out(unreal.MaterialProperty.MP_SPECULAR, g.scalar("Specular", 0.5))
    g.out(unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET, tr)
    setp(m, "used_with_nanite", False)   # plain instanced meshes (SeaGen: their swimming offset failed in Nanite's raster)
    return finish(m)


# Corals, sponges, soft corals (SeaGen: R height 0 … 1, G tip-ness, B the variant's random, A living tissue): each
# instance its own shade between two colours, paler (or bluer) tips, a dead grey-brown foot; fine polyp relief.
CORAL_COLOUR = r"""
float k = frac(Rand * 7.13 + VC.b * 3.7);
float3 c = lerp(ColourA.rgb, ColourB.rgb, k);
c = lerp(c, Tip.rgb, VC.g * TipAmount);
c = lerp(float3(0.20, 0.18, 0.15), c, saturate(VC.a * 1.5));
float2 q = UV * PolypScale;
float n = frac(sin(dot(floor(q), float2(12.9898, 78.233))) * 43758.5453);
return c * (0.85 + 0.3 * n);
"""

CORAL_NORMAL = r"""
float2 q = UV * PolypScale;
float2 f = frac(q) - 0.5;
float d = length(f);
float2 g = d < 0.35 ? -f / max(d, 1e-3) * sin(d / 0.35 * 3.14159) * Relief : 0;
return normalize(float3(g, 1.0));
"""


def make_coral_master(name, subsurface):
    m = material(name, RA_MATS)
    if subsurface:
        setp(m, "shading_model", unreal.MaterialShadingModel.MSM_SUBSURFACE)
    g = Graph(m)
    vc = g.simple(unreal.MaterialExpressionVertexColor)
    vc4 = g.custom("return float4(C.rgb, A);", 4, [("C", (vc, "")), ("A", (vc, "A"))], "Vertex colour")
    uv = g.texcoord(0)
    ps = g.scalar("PolypScale", 60.0)
    colour = g.custom(CORAL_COLOUR, 3, [("VC", vc4), ("UV", uv), ("Rand", g.simple(unreal.MaterialExpressionPerInstanceRandom)), ("ColourA", g.vector("ColourA", (0.5, 0.4, 0.3))),
                                        ("ColourB", g.vector("ColourB", (0.4, 0.45, 0.3))), ("Tip", g.vector("Tip", (0.8, 0.8, 0.9))), ("TipAmount", g.scalar("TipAmount", 0.6)),
                                        ("PolypScale", ps)], "Coral colour")
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, colour)
    g.out(unreal.MaterialProperty.MP_NORMAL, g.custom(CORAL_NORMAL, 3, [("UV", uv), ("PolypScale", ps), ("Relief", g.scalar("Relief", 0.5))], "Polyps"))
    g.out(unreal.MaterialProperty.MP_ROUGHNESS, g.scalar("Roughness", 0.65))
    g.out(unreal.MaterialProperty.MP_SPECULAR, g.scalar("Specular", 0.4))
    if subsurface:
        g.out(unreal.MaterialProperty.MP_SUBSURFACE_COLOR, g.custom("return C * 1.2;", 3, [("C", colour)], "Through"))
        g.out(unreal.MaterialProperty.MP_OPACITY, g.scalar("Subsurface", 0.6))
    return finish(m)


# The bottom (SeaGen's beds: R sand, G rubble, B reef rock, A algae), and the karst islands (G jungle, B the notch).
SEABED_COLOUR = r"""
float3 w = VC.rgb; float alg = VC.a;
float rest = saturate(1.0 - w.r - w.g - w.b);
float3 sand = Sand.rgb * pow(dot(CS.rgb, float3(0.3, 0.4, 0.3)) / 0.30, 0.6);
float3 rub = Rubble.rgb * pow(dot(CR.rgb, float3(0.3, 0.4, 0.3)) / 0.122, 0.6);
float3 rock = Rock.rgb * pow(dot(CK.rgb, float3(0.3, 0.4, 0.3)) / 0.5, 0.6);
float2 q = UV / 1.3;
float pink = smoothstep(0.55, 0.75, frac(sin(dot(floor(q), float2(12.9, 78.2))) * 43758.5));
rock = lerp(rock, float3(0.45, 0.22, 0.30), pink * 0.5);
float3 c = sand * (w.r + rest) + rub * w.g + rock * w.b;
return lerp(c, float3(0.18, 0.20, 0.08), alg * 0.7);
"""

KARST_COLOUR = r"""
float3 lime = Lime.rgb * pow(dot(CK.rgb, float3(0.3, 0.4, 0.3)) / 0.5, 0.7);
// Rain streaks darken the grey cliffs; the notch is dark and wet.
float streak = frac(sin(floor(UV.x / 0.7) * 91.7) * 43758.5);
lime *= lerp(1.0, 0.55, streak * smoothstep(0.2, 0.8, VC.r));
lime *= lerp(1.0, 0.4, VC.b);
float2 q = UV / 3.0;
float n = frac(sin(dot(floor(q), float2(12.9898, 78.233))) * 43758.5453);
float3 jungle = lerp(float3(0.04, 0.09, 0.03), float3(0.10, 0.16, 0.05), n);
return lerp(lime, jungle, smoothstep(0.12, 0.5, VC.g));
"""


def make_seabed(mpc):
    sets = {k: texture_set(k) for k in ("JW1", "JW3", "JK1")}
    m = material("M_RA_Seabed", RA_MATS)
    g = Graph(m)
    uv, vc = g.texcoord(0), g.simple(unreal.MaterialExpressionVertexColor)
    vc4 = g.custom("return float4(C.rgb, A);", 4, [("C", (vc, "")), ("A", (vc, "A"))], "Vertex colour")
    wpos = g.world_position()
    muv = g.custom("return WP.xy / 100.0;", 2, [("WP", wpos)], "Metres")
    su = g.custom("return UV / 2.0;", 2, [("UV", muv)], "Sand UV")
    ru = g.custom("return UV / 1.5;", 2, [("UV", muv)], "Rubble UV")
    ku = g.custom("return UV / 3.0;", 2, [("UV", muv)], "Rock UV")
    cs, cr, ck = g.texture(sets["JW1"]["Color"], su), g.texture(sets["JW3"]["Color"], ru), g.texture(sets["JK1"]["Color"], ku)
    ns, nr, nk = g.texture(sets["JW1"]["Normal"], su), g.texture(sets["JW3"]["Normal"], ru), g.texture(sets["JK1"]["Normal"], ku)
    colour = g.custom(SEABED_COLOUR, 3, [("VC", vc4), ("UV", muv), ("CS", (cs, "RGB")), ("CR", (cr, "RGB")), ("CK", (ck, "RGB")),
                                         ("Sand", g.vector("Sand", (0.75, 0.70, 0.58))), ("Rubble", g.vector("Rubble", (0.55, 0.50, 0.42))),
                                         ("Rock", g.vector("Rock", (0.38, 0.33, 0.28)))], "Seabed colour")
    normal = g.custom("float3 w = VC.rgb; float rest = saturate(1.0 - w.r - w.g - w.b); return normalize(NS * (w.r + rest) + NR * w.g + NK * w.b);", 3,
                      [("VC", vc4), ("NS", (ns, "RGB")), ("NR", (nr, "RGB")), ("NK", (nk, "RGB"))], "Seabed normal")
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, colour)
    g.out(unreal.MaterialProperty.MP_NORMAL, normal)
    g.out(unreal.MaterialProperty.MP_ROUGHNESS, g.scalar("Roughness", 0.8))
    finish(m)
    k = material("M_RA_Karst", RA_MATS)
    g = Graph(k)
    uv, vc = g.texcoord(0), g.simple(unreal.MaterialExpressionVertexColor)
    wpos = g.world_position()
    muv = g.custom("return float2(WP.x + WP.y, WP.z) / 100.0;", 2, [("WP", wpos)], "Metres")
    ku = g.custom("return UV / 4.0;", 2, [("UV", muv)], "Rock UV")
    ck, nk = g.texture(sets["JK1"]["Color"], ku), g.texture(sets["JK1"]["Normal"], ku)
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, g.custom(KARST_COLOUR, 3, [("VC", (vc, "")), ("UV", muv), ("CK", (ck, "RGB")), ("Lime", g.vector("Lime", (0.40, 0.39, 0.36)))], "Karst colour"))
    g.out(unreal.MaterialProperty.MP_NORMAL, nk)
    g.out(unreal.MaterialProperty.MP_ROUGHNESS, g.scalar("Roughness", 0.85))
    finish(k)
    return m, k


# The night's sea: plankton that flashes blue where the water moves (tiny emissive motes; Plankton in MPC_Cube).
PLANKTON = r"""
float t = Time * 1.7 + Rand * 40.0;
float f = pow(saturate(sin(t) * sin(t * 0.37 + Rand * 9.0)), 12.0);
return float3(0.15, 0.55, 1.0) * f * Nits * On;
"""


def make_plankton(mpc):
    m = material("M_RA_Plankton", RA_MATS)
    setp(m, "shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    setp(m, "blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    g = Graph(m)
    g.out(unreal.MaterialProperty.MP_EMISSIVE_COLOR, g.custom(PLANKTON, 3, [("Time", g.simple(unreal.MaterialExpressionTime)),
                                                                           ("Rand", g.simple(unreal.MaterialExpressionPerInstanceRandom)),
                                                                           ("Nits", g.scalar("Nits", 30.0)), ("On", g.mpc(mpc, "Plankton"))], "Plankton"))
    return finish(m)


def make_sea(build=True):
    mpc = cube_collection()
    make_underwater(mpc)
    make_sea_surface(mpc)
    make_caustics(mpc)
    fish = make_fish_master()
    for name, (pat, back, belly, stripe, fin, silver, length, amp, freq) in FISH.items():
        instance(f"MI_RA_Fish_{name}", fish, RA_MATS, {"Pattern": pat, "Silver": silver, "LengthCm": length, "SwimAmp": amp, "SwimFreq": freq,
                                                        "Bars": 9.0 if name == "Barracuda" else 3.5},
                 {"Back": back, "Belly": belly, "Stripe": stripe, "FinColour": fin})
    instance("MI_RA_Manta", make_manta(), RA_MATS)
    coral = make_coral_master("M_RA_Coral", False)
    soft = make_coral_master("M_RA_SoftTissue", True)
    instance("MI_RA_Coral_Table", coral, RA_MATS, {"PolypScale": 90, "TipAmount": 0.5}, {"ColourA": (0.33, 0.28, 0.16), "ColourB": (0.22, 0.30, 0.20), "Tip": (0.55, 0.70, 0.75)})
    instance("MI_RA_Coral_Staghorn", coral, RA_MATS, {"PolypScale": 120, "TipAmount": 0.8}, {"ColourA": (0.40, 0.33, 0.20), "ColourB": (0.25, 0.30, 0.45), "Tip": (0.75, 0.80, 0.90)})
    instance("MI_RA_Coral_Massive", coral, RA_MATS, {"PolypScale": 30, "TipAmount": 0.2, "Relief": 0.8}, {"ColourA": (0.42, 0.36, 0.20), "ColourB": (0.25, 0.32, 0.18), "Tip": (0.55, 0.5, 0.35)})
    instance("MI_RA_SoftCoral", soft, RA_MATS, {"PolypScale": 50, "TipAmount": 0.7, "Subsurface": 0.7}, {"ColourA": (0.75, 0.15, 0.45), "ColourB": (0.85, 0.40, 0.12), "Tip": (0.95, 0.85, 0.9)})
    instance("MI_RA_SeaFan", coral, RA_MATS, {"PolypScale": 200, "TipAmount": 0.3, "Relief": 0.3}, {"ColourA": (0.65, 0.12, 0.08), "ColourB": (0.80, 0.45, 0.10), "Tip": (0.9, 0.6, 0.3)})
    instance("MI_RA_Sponge", coral, RA_MATS, {"PolypScale": 12, "TipAmount": 0.3, "Relief": 0.9}, {"ColourA": (0.35, 0.15, 0.10), "ColourB": (0.45, 0.25, 0.16), "Tip": (0.6, 0.4, 0.3)})
    instance("MI_RA_ReefRock", coral, RA_MATS, {"PolypScale": 8, "TipAmount": 0.0, "Relief": 0.6}, {"ColourA": (0.30, 0.26, 0.22), "ColourB": (0.40, 0.25, 0.30), "Tip": (0.4, 0.4, 0.4)})
    instance("MI_RA_PygmySeahorse", soft, RA_MATS, {"PolypScale": 400, "TipAmount": 0.9, "Subsurface": 0.5}, {"ColourA": (0.75, 0.45, 0.55), "ColourB": (0.72, 0.42, 0.52), "Tip": (0.85, 0.15, 0.12)})
    instance("MI_RA_Mangrove", coral, RA_MATS, {"PolypScale": 6, "TipAmount": 0.0, "Relief": 0.4}, {"ColourA": (0.22, 0.16, 0.11), "ColourB": (0.18, 0.15, 0.12), "Tip": (0.3, 0.3, 0.3)})
    instance("MI_RA_Canopy", coral, RA_MATS, {"PolypScale": 9, "TipAmount": 0.0, "Relief": 0.25, "Roughness": 0.7}, {"ColourA": (0.05, 0.12, 0.04), "ColourB": (0.09, 0.15, 0.05), "Tip": (0.1, 0.2, 0.05)})
    bed, karst = make_seabed(mpc)
    instance("MI_RA_Seabed", bed, RA_MATS)
    instance("MI_RA_Karst", karst, RA_MATS)
    make_plankton(mpc)
    if build:
        n = unreal.MuseeJourneyLibrary.build_things("sea", RA + "/Things", RA_MATS)
        log(f"Raja Ampat: {n} things saved")


def make_ready(which):
    """The marker the game looks for before it offers a journey in the car (JourneyMatterhorn/JourneySea::IsAvailable)."""
    folder = MH if which == "matterhorn" else RA
    path = f"{folder}/Ready"
    if not EAL.does_asset_exist(path):
        TOOLS.create_asset("Ready", folder, unreal.CurveFloat, unreal.CurveFloatFactory())
        log(f"{which}: ready")


# ---------------------------------------------------------------------------------------------- II · Sea Light

# The Cube's first piece (Source/MuseeVision/Cube/SeaLight.h): a million cells in the dark water round the car
# (SM_SeaLight_Cards: tiny quads, each a cell at rest, UV0 its corner ±0.5, UV1 its two randoms), each lit by how
# stirred the water is where it sits (the field: 96³ over 12 m, packed 10 × 10 in a 960² texture the game uploads).
SL = ROOT + "/SeaLight"

SEALIGHT_FIELD = r"""
float3 q = (P - FieldOrigin.xyz) / FieldSize + 0.5;
float inside = step(0.0, q.x) * step(q.x, 1.0) * step(0.0, q.y) * step(q.y, 1.0) * step(0.0, q.z) * step(q.z, 1.0);
// 96 slices of 96², ten to a row (SeaLight.h: N, Tiles). R the stirring (0 … 2), G B A the water's motion (±2 m/s).
float gz = clamp(saturate(q.z) * 96.0 - 0.5, 0.0, 95.0);
float z0 = floor(gz); float fz = gz - z0; float z1 = min(z0 + 1.0, 95.0);
float2 xy = clamp(q.xy, 0.5 / 96.0, 95.5 / 96.0);
float2 t0 = (float2(fmod(z0, 10.0), floor(z0 / 10.0)) + xy) / 10.0;
float2 t1 = (float2(fmod(z1, 10.0), floor(z1 / 10.0)) + xy) / 10.0;
float4 f4 = lerp(Tex.SampleLevel(TexSampler, t0, 0), Tex.SampleLevel(TexSampler, t1, 0), fz);
float F = f4.r * 2.0 * inside;
float3 vel = (f4.gba - 0.5) * 4.0 * inside;
// Each cell its own threshold: a little stirring wakes a few, a lot wakes most (sparks, not a glow).
float thr = 0.15 + 0.8 * R.x + 0.9 * Thin;
float b = saturate((F - thr) / 0.25) * Fade;
float3 drift = float3(sin(Time * 0.31 + R.x * 40.0), sin(Time * 0.27 + R.y * 40.0), 0.5 * sin(Time * 0.23 + (R.x + R.y) * 30.0)) * 6.0;
float d = length(P - Cam);
// A pixel and a half at least far off; near the eye out of focus: large and soft (its light spread to keep its energy).
float s = max(max(0.15, d * PixelAngle * 1.4), 1.3 * saturate((110.0 - d) / 70.0));
// Carried by the water: drawn as a streak along the motion across the view, about a seventh of a second of it.
float3 dir = (P - Cam) / max(d, 1e-3);
float3 v = vel * 100.0;
float3 along = v - dir * dot(v, dir);
float sp = length(along);
// (Only some cells streak: the rest flash as points, so the water reads as sparks carried, not hair.)
float L = min(sp * 0.07, 18.0) * step(0.45, R.y);
"""

SEALIGHT_WPO = r"""
float3 P = WP;
""" + SEALIGHT_FIELD + r"""
P += drift;
float3 up0 = abs(dir.z) > 0.95 ? float3(1, 0, 0) : float3(0, 0, 1);
float3 A = sp > 1.0 ? along / sp : normalize(cross(dir, up0));
float3 B = normalize(cross(dir, A));
float on = step(0.002, b);
// The streak runs ahead of the cell, the way the water carries it.
// Far off, strongly stirred water glows as a sheet: its cells carry a wide faint halo that runs together.
float halo = saturate((F - 0.5) / 0.7) * saturate((d - 200.0) / 200.0);
float w = s * (1.0 + 5.0 * halo);
float3 off = ((UV.x * (w + L) + 0.5 * L) * A + UV.y * w * B) * on;
return P + off - WP;
"""

SEALIGHT_STATE = r"""
float3 P = WP;
""" + SEALIGHT_FIELD + r"""
float halo = saturate((F - 0.5) / 0.7) * saturate((d - 200.0) / 200.0);
return float4(b, s, L, halo);
"""

SEALIGHT_LIGHT = r"""
// The flash: cyan-white while fresh and strong, deep blue as it fades; a soft core, a streak's head brighter than its
// tail.
float b = St.x, s = St.y, L = St.z, halo = St.w;
// The quad is (1 + 5 halo) times the core: the core keeps its size, a faint wide glow fills the rest.
float k = 1.0 + 5.0 * halo;
float g = exp(-11.0 * dot(UV, UV) * k * k) + 0.06 * halo * exp(-9.0 * dot(UV, UV));
float head = L > 0.5 ? lerp(0.3, 1.0, saturate(UV.x + 0.5)) : 1.0;
float t = saturate((b - 0.3) / 0.7);
float3 col = lerp(float3(0.01, 0.16, 1.0), float3(0.45, 0.86, 1.0), t * t);
float flick = 0.85 + 0.15 * sin(Time * (9.0 + 14.0 * R.y) + R.x * 60.0);
float energy = (0.25 / s) * (0.25 / s) * sqrt(s / (s + L));
return col * (PeakNits * b * g * head * flick * energy);
"""

FISH_WPO = r"""
// The tail beats (a wave down the body, most at the tail), in the fish's own frame.
float x = LP.x;
float amp = 2.0 * pow(saturate((5.0 - x) / 22.0), 1.6);
float y = amp * sin(Time * 9.0 + Rnd * 40.0 - x * 0.22);
return TransformLocalVectorToWorld(Parameters, float3(0.0, y, 0.0));
"""

FISH_LIGHT = r"""
// A scad in the dark sea is seen only by the light it stirs on its skin: the outline glows faintly, and fine sparks
// flash along it as it swims.
float3 V = normalize(Cam - WP);
float ndv = abs(dot(normalize(N), V));
float rim = pow(1.0 - ndv, 3.0);
float3 q = LP / 0.9;
float3 c = floor(q) + Rnd * 97.0;
float h = frac(sin(dot(c, float3(12.9898, 78.233, 37.719))) * 43758.5453);
float dotp = saturate(1.0 - length(frac(q) - 0.5) * 2.5);
float spark = step(0.93, h) * dotp * saturate(0.5 + 0.5 * sin(Time * (5.0 + 9.0 * h) + h * 40.0));
return float3(0.05, 0.42, 1.0) * Nits * Glow * Fade * (1.6 * rim + 1.2 * spark * rim);
"""

MANTA_LIGHT = r"""
// Its body is dark; along its edges the water it pushes flashes: a broken line of fine sparks (round points 2 cm apart
// at most, each lit for a moment), strongest on the downstroke.
float3 q = LP / 2.5;
float3 c = floor(q);
float h0 = frac(sin(dot(c, float3(1.7, 9.2, 3.1))) * 91.3);
float h = frac(sin(dot(c + floor(Time * 4.0 + h0), float3(12.9898, 78.233, 37.719))) * 43758.5453);
float dotp = saturate(1.0 - length(frac(q) - 0.5) * 3.0);
float spark = step(0.6, h) * dotp;
float edge = smoothstep(0.86, 1.0, saturate(Edge)) * saturate(Lead);
return float3(0.03, 0.4, 1.0) * Nits * edge * spark * (0.35 + 0.65 * saturate(-cos(WingPhase)));
"""

MANTA_WPO = r"""
// The wing stroke: the tips rise and fall together, bending from the body out (WingPhase from the game).
float y = abs(LP.y) / 225.0;
float z = 45.0 * sin(WingPhase) * y * y;
return TransformLocalVectorToWorld(Parameters, float3(0.0, 0.0, z));
"""

RING_LIGHT = r"""
// A drop's ring spreading on the underside of the surface: a thin, slightly wobbling circle of sparks, broken in places
// (a real ring is never perfect: the surface is moving, the drop was not round).
float2 p = (UV - 0.5) * 2.0;
float r = length(p);
float a = atan2(p.y, p.x);
float wob = 0.035 * sin(a * 3.0 + Rnd * 23.0) + 0.02 * sin(a * 5.0 + Rnd * 41.0) + 0.012 * sin(a * 11.0 + Rnd * 7.0);
float width = 0.045 + 0.025 * (0.5 + 0.5 * sin(a * 4.0 + Rnd * 13.0));
float ring = exp(-pow((r - (0.78 + wob)) / width, 2.0));
float seg = floor((a + 3.14159) * 9.0 / 3.14159);
float gap = frac(sin((seg + Rnd * 57.0) * 12.9898) * 43758.5453);
float broken = smoothstep(0.18, 0.3, gap);
float h = frac(sin((floor((a + 3.14159) * 14.0) + Rnd * 31.0) * 12.9898) * 43758.5453);
float sp = 0.35 + 0.65 * smoothstep(0.4, 0.9, h);
return float3(0.05, 0.5, 1.0) * Nits * Glow * ring * sp * broken;
"""


def make_sealight_cells():
    m = material("M_SeaLight_Cells", SL)
    setp(m, "blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    setp(m, "shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    setp(m, "two_sided", True)
    setp(m, "enable_responsive_aa", True)
    setp(m, "output_translucent_velocity", True)
    setp(m, "use_translucency_vertex_fog", False)
    g = Graph(m)
    uv, uv1 = g.texcoord(0), g.texcoord(1)
    tex = g.expr(unreal.MaterialExpressionTextureObjectParameter)
    tex.set_editor_property("parameter_name", "Field")
    tex.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/Black"))
    cam = g.simple(unreal.MaterialExpressionCameraPositionWS)
    time = g.simple(unreal.MaterialExpressionTime)
    common = [("UV", uv), ("R", uv1), ("Tex", tex), ("FieldOrigin", g.vector("FieldOrigin", (0.0, 0.0, 0.0))), ("FieldSize", g.scalar("FieldSize", 1200.0)),
              ("Thin", g.scalar("Thin", 0.0)), ("Fade", g.scalar("Fade", 1.0)), ("Cam", cam), ("Time", time), ("PixelAngle", g.scalar("PixelAngle", 0.0012))]
    wp_vs = g.world_position()
    try:
        wp_vs.set_editor_property("world_position_shader_offset", unreal.WorldPositionIncludedOffsets.WPT_EXCLUDE_ALL_SHADER_OFFSETS)
    except Exception as e:  # noqa: BLE001
        warn(f"sea light: world position offsets ({e})")
    wpo = g.custom(SEALIGHT_WPO, 3, common + [("WP", wp_vs)], "Cell place")
    state = g.custom(SEALIGHT_STATE, 4, common + [("WP", wp_vs)], "Cell state")
    interp = g.expr(unreal.MaterialExpressionVertexInterpolator)
    if not MEL.connect_material_expressions(state, "", interp, ""):
        warn("sea light: the vertex interpolator is not connected")
    light = g.custom(SEALIGHT_LIGHT, 3, [("UV", uv), ("R", uv1), ("Time", time), ("St", interp), ("PeakNits", g.scalar("PeakNits", 12.0))], "Cell light")
    g.out(unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET, wpo)
    g.out(unreal.MaterialProperty.MP_EMISSIVE_COLOR, light)
    return finish(m)


def make_sealight_fish():
    m = material("M_SeaLight_Fish", SL)
    setp(m, "blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    setp(m, "shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    setp(m, "two_sided", True)
    setp(m, "enable_responsive_aa", True)
    setp(m, "output_translucent_velocity", True)
    setp(m, "use_translucency_vertex_fog", False)
    g = Graph(m)
    glow = g.expr(unreal.MaterialExpressionPerInstanceCustomData)
    glow.set_editor_property("data_index", 0)
    glow.set_editor_property("const_default_value", 1.0)
    wpo = g.custom(FISH_WPO, 3, [("LP", g.simple(unreal.MaterialExpressionLocalPosition)), ("Rnd", g.simple(unreal.MaterialExpressionPerInstanceRandom)),
                                 ("Time", g.simple(unreal.MaterialExpressionTime))], "Tail beat")
    light = g.custom(FISH_LIGHT, 3, [("LP", g.simple(unreal.MaterialExpressionLocalPosition)), ("Rnd", g.simple(unreal.MaterialExpressionPerInstanceRandom)),
                                     ("Time", g.simple(unreal.MaterialExpressionTime)), ("Cam", g.simple(unreal.MaterialExpressionCameraPositionWS)),
                                     ("WP", g.world_position()), ("N", g.simple(unreal.MaterialExpressionVertexNormalWS)), ("Glow", glow),
                                     ("Nits", g.scalar("Nits", 1.2)), ("Fade", g.scalar("Fade", 1.0))], "Skin light")
    g.out(unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET, wpo)
    g.out(unreal.MaterialProperty.MP_EMISSIVE_COLOR, light)
    return finish(m)


def make_sealight_manta():
    m = material("M_SeaLight_Manta", SL)
    setp(m, "shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    setp(m, "two_sided", True)
    g = Graph(m)
    wpo = g.custom(MANTA_WPO, 3, [("LP", g.simple(unreal.MaterialExpressionLocalPosition)), ("WingPhase", g.scalar("WingPhase", 0.0))], "Wing stroke")
    g.out(unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET, wpo)
    vc = g.simple(unreal.MaterialExpressionVertexColor)
    light = g.custom(MANTA_LIGHT, 3, [("LP", g.simple(unreal.MaterialExpressionLocalPosition)), ("Time", g.simple(unreal.MaterialExpressionTime)),
                                      ("Edge", (vc, "R")), ("Lead", (vc, "G")), ("WingPhase", g.scalar("WingPhase", 0.0)), ("Nits", g.scalar("Nits", 9.0))], "Edge sparks")
    g.out(unreal.MaterialProperty.MP_EMISSIVE_COLOR, light)
    return finish(m)


def make_sealight_ring():
    m = material("M_SeaLight_Ring", SL)
    setp(m, "blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    setp(m, "shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    setp(m, "two_sided", True)
    setp(m, "enable_responsive_aa", True)
    setp(m, "use_translucency_vertex_fog", False)
    g = Graph(m)
    glow = g.expr(unreal.MaterialExpressionPerInstanceCustomData)
    glow.set_editor_property("data_index", 0)
    light = g.custom(RING_LIGHT, 3, [("UV", g.texcoord(0)), ("Rnd", g.simple(unreal.MaterialExpressionPerInstanceRandom)), ("Glow", glow),
                                     ("Nits", g.scalar("Nits", 3.0))], "Ring light")
    g.out(unreal.MaterialProperty.MP_EMISSIVE_COLOR, light)
    return finish(m)


def make_sealight():
    make_sealight_cells()
    make_sealight_fish()
    make_sealight_manta()
    make_sealight_ring()
    n = unreal.MuseeJourneyLibrary.build_things("sealight", SL, SL)
    log(f"Sea Light: {n} packages saved")


# ---------------------------------------------------------------------------------------------- main

PARTS = {"cube": make_cube, "matterhorn": make_matterhorn, "mhmat": lambda: make_matterhorn(build=False), "mhthings": make_matterhorn_things,
         "sea": make_sea, "seamat": lambda: make_sea(build=False), "sealight": make_sealight,
         "mhready": lambda: make_ready("matterhorn"), "seaready": lambda: make_ready("sea")}


# The Cube is a room at rest: its own materials only. The journeys (the Matterhorn, Raja Ampat) are parked: their parts
# still build when asked for by name, their assets are kept out of the game's content (see the Élan Cube's notes).
DEFAULT = ["cube"]   # (II · Sea Light: "sealight", once the piece is installed)


def main(args=None):
    args = [a for a in (args or []) if not a.startswith("-")]
    names = list(PARTS) if args == ["all"] else (args or DEFAULT)
    for name in names:
        PARTS[name]()


if __name__ == "__main__":
    main(sys.argv[1:])
