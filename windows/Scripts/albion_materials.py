"""
Albion's materials (Source/MuseeVision/Albion; plan/proposals/albion), in /Game/Museum/Materials/Albion:

    UnrealEditor-Cmd.exe MuseeVision.uproject -run=pythonscript -script="<repo>/windows/Scripts/albion_materials.py"
    (or apply_all.py albion, which runs this and places the hang)

Instances of the museum's masters (materials.py: M_Stone, M_StoneRelief, M_Metal), in the material language
(materials/SPEC.md): the stone family finished for a Victorian court (buff ashlar banded with red, a Bath-stone dressing
for the carving, a slate skirting, York stone flags underfoot), the ten polished British stones of the columns, the
encaustic tiles; the iron painted, the lead dull. Their photographed layers are the museum's CC0 sets (pbr.py) and
Albion's own generated ones (albion_textures.py: AL_*, whose statistics this adds to pbr.py's table).

And four masters of Albion's own:
- M_Albion_Glass: the vaults' and screens' glass (Thin Translucent): low-iron but Victorian (a faint green, a slow
  waviness in the pane), with London's weather on it from MPC_Albion (AAlbionSky): rain as beads and running
  rivulets down each pane's slope, the wet sheen, snow lying on the panes that face up;
- M_Albion_Stained: pot-metal glass from an atlas (RGB the glass's transmittance, A the lead), antique glass's ripple;
- M_Albion_SunLF: the sun's light function (and the three coloured lights' that carry the stained glass's colours onto
  the floor): each lit point's ray towards the sun (MPC_Albion SunDir) is followed to the south screen's band and the
  lancets, and the glass it passes through read from the same atlases (Channel −1: the white sun, less what the stained
  glass carries; 0, 1, 2: that glass's red, green, blue); London's cloud (MPC_Albion Sun) takes the sun off Albion.
- MPC_Albion: Rain, Wet, Snow, Fog, Sun (0 … 1) and SunDir.

Idempotent: masters are rebuilt in place, instances updated in place.
"""
import json
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import musee_paths as P  # noqa: E402
import pbr  # noqa: E402
import materials as MAT  # noqa: E402
import setup_project as S  # noqa: E402

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
FOLDER = P.MATERIALS + "/Albion"
TEXTURES = P.MUSEUM_CONTENT + "/Textures/Albion"
HERE = os.path.dirname(os.path.abspath(__file__))
ART = os.path.normpath(os.path.join(HERE, "..", "SourceArt", "Albion"))
STATS = os.path.normpath(os.path.join(HERE, "..", "SourceArt", "PBR", "albion_stats.json"))
lin = MAT.lin


def log(msg):
    unreal.log(f"[albion_materials] {msg}")


def warn(msg):
    unreal.log_warning(f"[albion_materials] {msg}")


# ---------------------------------------------------------------------------------------------- the generated sets

def add_stats():
    """Albion's generated sets into pbr.py's table (in memory; stats.json is pbr_fetch.py's)."""
    table = pbr.stats()
    with open(STATS, encoding="utf-8") as f:
        mine = json.load(f)
    table.update(mine)
    return mine


# ---------------------------------------------------------------------------------------------- the instances

STONE_KW = dict(block_shift=1.0, contrast=0.8, normal=0.7, rough_influence=0.5)

# The weather on the stone (exterior_materials.py's M_Ext_Stone). Inside, a Victorian court's grime: the soot of gas and
# of London's air settles under the glass (the walls darken towards the springing and under the lancets' sills), dust on
# every ledge, the foot scuffed. Outside, English soot: grey-black runs under the parapet, the buttresses' set-offs and
# the sills, washed paler between, a damp foot, lichen where the sun doesn't reach.
INSIDE = dict(SootAmount=0.6, StreakAmount=0.45, StreakFall=4.5, StreakWidth=0.3, WashAmount=0.0, DampAmount=0.0,
              LichenAmount=0.0, FootDirt=0.14, DustUp=0.12, MacroVariation=0.12)
INSIDE_TOPS = {"StreakTopsA": (10.0, 8.5, 5.0), "StreakTopsB": (0.0, 0.0, 0.0), "StreakTopsC": (0.0, 0.0, 0.0),
               "StreakTint": (0.42, 0.4, 0.37)}
OUTSIDE = dict(StreakAmount=0.55, SootAmount=0.28, StreakFall=2.8, StreakWidth=0.2, WashAmount=0.45, DampAmount=0.2,
               DampHeight=0.5, LichenAmount=0.55)
OUTSIDE_TOPS = {"StreakTopsA": (10.9, 9.9, 7.45), "StreakTopsB": (5.0, 4.8, 0.0), "StreakTopsC": (0.0, 0.0, 0.0),
                "StreakTint": (0.42, 0.41, 0.39)}


def stone_specs():
    NJ, UB = MAT.NO_JOINTS, MAT.UNBROKEN
    wall = MAT.merged(MAT.QUIET_DRAWN, MAT.STONE_WALL)
    specs = {}
    # The buff ashlar: Bath stone's colour (a warm honey oolite, as the renderings; the red bands close to it in value), honed, 0.3 m courses of 0.75 m blocks (joints on every band's
    # edges: the bands are 0.3 m every 1.2 m); the red: Mansfield red sandstone, one course deep, longer blocks.
    buff = MAT.merged(wall, Roughness=0.64, ClearCoat=0.0, CourseHeight=0.30, CourseOffset=0.0, BlockLength=0.75, BlockJitter=0.3,
                      Stagger=0.5, StaggerJitter=0.2, FloorSlab=0.9, WallJoints=1.0, FloorJoints=1.0, JointWidth=0.004,
                      JointDarkness=0.5, BlockTone=0.12, BlockHue=0.02, MacroScale=4.0, MacroVariation=0.08, FootDirt=0.08,
                      DustUp=0.05, Specular=0.55)
    specs["MI_Albion_Buff"] = MAT.spec("M_Ext_Stone", MAT.merged(buff, INSIDE), MAT.merged({"BaseColor": lin(0xCFB892)}, INSIDE_TOPS), photo=("S3", STONE_KW))
    # (Mansfield red: a dusty brown-red close in value to the buff (Oxford's and Butterfield's polychromy is muted: the bands
    # are felt, not shouted), each block its own tone; the photo's grain at full strength so it reads as stone.)
    specs["MI_Albion_Red"] = MAT.spec("M_Ext_Stone", MAT.merged(buff, INSIDE, BlockLength=0.95, BlockTone=0.2, BlockHue=0.03, MacroVariation=0.1, Roughness=0.72),
                                      MAT.merged({"BaseColor": lin(0xA98272)}, INSIDE_TOPS), photo=("S3", dict(STONE_KW, colour_amount=0.85, contrast=1.1, normal=0.9)))
    # The same outside, weathered: a greyer, more varied face, darker at its foot.
    specs["MI_Albion_Buff_Ext"] = MAT.spec("M_Ext_Stone", MAT.merged(buff, OUTSIDE, BlockTone=0.2, MacroVariation=0.1, Roughness=0.72, FootDirt=0.1),
                                           MAT.merged({"BaseColor": lin(0xC2B196)}, OUTSIDE_TOPS), photo=("S3", dict(STONE_KW, normal=0.9)))
    specs["MI_Albion_Red_Ext"] = MAT.spec("M_Ext_Stone", MAT.merged(buff, OUTSIDE, BlockLength=0.95, BlockTone=0.26, BlockHue=0.04, Roughness=0.76, FootDirt=0.1),
                                          MAT.merged({"BaseColor": lin(0x9A7F70)}, OUTSIDE_TOPS), photo=("S3", dict(STONE_KW, colour_amount=0.85, contrast=1.1, normal=1.0)))
    # The dressings (plinth course, pilasters, strings, surrounds, copings): the paler, finer Bath stone, tooled smooth;
    # one tall course from the skirting to the first band.
    dress = MAT.merged(wall, Roughness=0.6, ClearCoat=0.0, CourseHeight=0.70, CourseOffset=-0.2857, BlockLength=1.1, BlockJitter=0.25,
                       JointWidth=0.003, JointDarkness=0.75, BlockTone=0.1, FootDirt=0.04, DustUp=0.06, Specular=0.55)
    specs["MI_Albion_Dressing"] = MAT.spec("M_Ext_Stone", MAT.merged(dress, INSIDE), MAT.merged({"BaseColor": lin(0xD6C6A6)}, INSIDE_TOPS), photo=("S3", dict(STONE_KW, contrast=0.6)))
    specs["MI_Albion_Dressing_Ext"] = MAT.spec("M_Ext_Stone", MAT.merged(dress, OUTSIDE, Roughness=0.7, BlockTone=0.16, FootDirt=0.1),
                                               MAT.merged({"BaseColor": lin(0xCDC2AB)}, OUTSIDE_TOPS), photo=("S3", dict(STONE_KW, contrast=0.7, normal=0.9)))
    # The carving: the same stone, no joints (each capital is one block).
    specs["MI_Albion_Carved"] = MAT.spec("M_Ext_Stone", MAT.merged(NJ, UB, INSIDE, Roughness=0.58, ClearCoat=0.0, MacroVariation=0.05, BlockTone=0.0,
                                                             StrataAmount=0.0, PoreAmount=0.05, DustUp=0.12, Specular=0.55),
                                         MAT.merged({"BaseColor": lin(0xD2C2A2)}, INSIDE_TOPS), photo=("S3", dict(contrast=0.6, normal=0.4, rough_influence=0.4)))
    # The slate skirting: Welsh slate, honed, in 1.2 m lengths.
    specs["MI_Albion_Slate"] = MAT.spec("M_Stone", MAT.merged(NJ, CourseHeight=60.0, BlockLength=1.2, WallJoints=1.0, JointWidth=0.002,
                                                            JointDarkness=0.6, Roughness=0.42, ClearCoat=0.0, BlockTone=0.06, Specular=0.5),
                                        {"BaseColor": lin(0x3B3F44)}, photo=("G5", dict(colour_amount=0.4, normal=0.3, rough_influence=0.3)))
    # York stone flags in the aisles: grey-buff sandstone, sawn and worn, courses 0.6 m of 0.9 m flags (1.2 m on the
    # plan's lines), open joints of lime mortar.
    specs["MI_Albion_YorkStone"] = MAT.spec("M_Stone", MAT.merged(MAT.QUIET_DRAWN, MAT.STONE_FLOOR, Roughness=0.62, ClearCoat=0.0, FloorSlab=0.6,
                                                                FloorAspect=1.5, FloorStagger=0.37, JointWidth=0.005, JointDarkness=0.62,
                                                                BlockTone=0.13, BlockHue=0.02, SlabTilt=0.25, TrafficWear=0.5, Specular=0.52),
                                            {"BaseColor": lin(0x9A8F7C)}, photo=("S3", dict(block_shift=1.0, contrast=0.9, normal=0.9, rough_influence=0.5)),
                                            switches=MAT.MACRO)
    # The encaustic tiles (AL_TILE: the tiles, joints, wear drawn; plan metres): waxed and buffed (a polished floor's coat,
    # LESSONS: 0.4-0.5), the sheen worn duller in the lanes.
    tile = dict(NJ, **UB, Roughness=0.46, ClearCoat=0.4, ClearCoatRoughness=0.2, MacroScale=6.0, MacroVariation=0.09, BlockTone=0.0,
                StrataAmount=0.0, PoreAmount=0.0, RoughWear=0.14, TrafficWear=0.8, Specular=0.5, FootDirt=0.0)
    specs["MI_Albion_Encaustic"] = MAT.spec("M_Stone", tile, {"BaseColor": tuple(STATS_MINE["AL_TILE"]["color_mean"])},
                                            photo=("AL_TILE", dict(normal=0.6, rough_influence=1.0, contrast=1.0)), switches=MAT.MACRO)   # (normal 1.0 sparkled at grazing: the flicker audit)
    specs["MI_Albion_EncausticBorder"] = MAT.spec("M_Stone", tile, {"BaseColor": tuple(STATS_MINE["AL_BORDER"]["color_mean"])},
                                                  photo=("AL_BORDER", dict(normal=1.0, rough_influence=1.0, contrast=1.0, use_uv0=True)))
    # The tablets: the dressing stone, their letters V-cut (Nanite displacement once baked, the normal before).
    for key, name in (("AL_RUSKIN", "MI_Albion_Ruskin"), ("AL_NAME", "MI_Albion_Name")):
        specs[name] = MAT.spec("M_StoneRelief", MAT.merged(NJ, UB, Roughness=0.62, ClearCoat=0.0, BlockTone=0.0, StrataAmount=0.0, PoreAmount=0.0,
                                                          MacroVariation=0.02, Specular=0.55),
                               {"BaseColor": tuple(STATS_MINE[key]["color_mean"])},
                               photo=(key, dict(normal=1.0, rough_influence=1.0, use_uv0=True, relief_cm=0.55)))
    # The ten polished stones (UV0 round and up each shaft; the set's own colour).
    for key in [k for k in STATS_MINE if k.startswith("AL_ST_")]:
        st = STATS_MINE[key]
        specs["MI_Albion_Stone_" + key[len("AL_ST_"):]] = MAT.spec(
            "M_Stone", dict(NJ, **UB, Roughness=float(st["rough_mean"]), RoughnessVariation=0.02, ClearCoat=0.0, MacroScale=3.0,
                            MacroVariation=0.03, BlockTone=0.0, StrataAmount=0.0, PoreAmount=0.0, Specular=0.58, DustUp=0.03),
            {"BaseColor": tuple(st["color_mean"])},
            photo=(key, dict(normal=0.5, rough_influence=1.0, contrast=1.0, use_uv0=True)))
    return specs


def book_specs():
    """The Kelmscott Chaucer's binding (half holland over blue-grey paper boards) and its paper's edges."""
    specs = {}
    specs["MI_Albion_BookBoards"] = MAT.spec("M_Fabric", {"Roughness": 0.78, "Specular": 0.35, "SheenAmount": 0.08, "SlubAmount": 0.02,
                                                          "WeaveAmount": 0.02, "MottleAmount": 0.06, "ThreadSize": 0.001},
                                             {"BaseColor": lin(0x61707D)})
    specs["MI_Albion_BookCloth"] = MAT.spec("M_Fabric", {"Roughness": 0.72, "Specular": 0.35, "SheenAmount": 0.2, "SlubAmount": 0.12,
                                                         "WeaveAmount": 0.12, "MottleAmount": 0.05, "ThreadSize": 0.0009},
                                            {"BaseColor": lin(0xD3C9B2)})
    specs["MI_Albion_PaperEdge"] = MAT.spec("M_Plaster", {"Roughness": 0.85, "Variation": 0.04, "GrainAmount": 0.03},
                                            {"BaseColor": lin(0xE4D8BE)})
    return specs


def build_page():
    """M_Albion_Page: a leaf of Batchelor's handmade paper, printed: its scan (Page, UV0), the ink a touch less matt than
    the paper, and light through the leaf (two-sided foliage: a page held up glows)."""
    m = S.new_material(FOLDER, "M_Albion_Page")
    g = G(m)
    uv = g.e(unreal.MaterialExpressionTextureCoordinate)
    tex = g.e(unreal.MaterialExpressionTextureSampleParameter2D, parameter_name="Page")
    default = unreal.load_asset("/Engine/EngineResources/DefaultTexture")
    if default:
        tex.set_editor_property("texture", default)
    g.link(uv, tex, "UVs")
    g.prop(tex, unreal.MaterialProperty.MP_BASE_COLOR)
    rough = g.custom("float l = dot(C, float3(0.3, 0.59, 0.11)); return lerp(0.58, 0.82, saturate(l * 1.6));", [("C", tex)], F1)
    g.prop(rough, unreal.MaterialProperty.MP_ROUGHNESS)
    g.prop(g.scalar("Specular", 0.38), unreal.MaterialProperty.MP_SPECULAR)
    sss = g.custom("return C * float3(0.55, 0.5, 0.42);", [("C", tex)], F3)
    g.prop(sss, unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    errors = list(MEL.recompile_material(m) or [])
    log(f"M_Albion_Page: {len(errors)} errors {errors}")
    EAL.save_loaded_asset(m)
    mi = simple_instance("MI_Albion_Page", m)
    MEL.update_material_instance(mi)
    EAL.save_loaded_asset(mi)
    return m


def wood_specs():
    """The furniture's oak (benches, cases, the lectern, the porch's ceiling, battens and panels): the museum's oak
    (M_Oak_Fumed's boards and photograph) in the Victorian finish, fumed darker and French-polished to a soft sheen."""
    return {"MI_Albion_Oak": MAT.spec("M_Wood", MAT.merged(MAT.OAK_BOARDS, Roughness=0.4), {"BaseColor": lin(0x4C3725)},
                                      photo=("W2", MAT.OAK_PHOTO))}


def metal_specs():
    specs = {}
    # The iron: cast and wrought iron in a dark slate-grey paint (the Details board's; Oxford's is grey), light enough to read against the sky (satin, a little orange peel, worn at the arrises).
    specs["MI_Albion_Iron"] = MAT.spec("M_Metal", {"Roughness": 0.42, "Metallic": 0.0, "Variation": 0.06, "NoiseScale": 0.4,
                                                   "PatinaAmount": 0.0},
                                       {"BaseColor": lin(0x434C50)}, photo=("M5", dict(colour_amount=0.0, normal=0.35, rough_influence=0.3)))
    # The lead: weathered sheet lead, a dull pale grey (its carbonate patina), no metal left showing.
    specs["MI_Albion_Lead"] = MAT.spec("M_Metal", {"Roughness": 0.62, "Metallic": 0.0, "Variation": 0.08, "NoiseScale": 0.6,
                                                   "PatinaAmount": 0.0},
                                       {"BaseColor": lin(0x7E8285)}, photo=("M1", dict(colour_amount=0.3, normal=0.4, rough_influence=0.3)))
    return specs


# ---------------------------------------------------------------------------------------------- Albion's own masters

def texture(name, srgb=True, normal=False):
    """T_<name> in /Game/Museum/Textures/Albion, from SourceArt/Albion/T_<name>.png (re-imported each run)."""
    png = os.path.join(ART, f"T_{name}.png")
    path = f"{TEXTURES}/T_{name}"
    if os.path.exists(png):
        task = unreal.AssetImportTask()
        task.filename = png
        task.destination_path = TEXTURES
        task.destination_name = f"T_{name}"
        task.automated = True
        task.replace_existing = True
        task.save = True
        TOOLS.import_asset_tasks([task])
    tex = unreal.load_asset(path) if EAL.does_asset_exist(path) else None
    if tex is None:
        warn(f"{path} missing")
        return None
    tex.set_editor_property("srgb", srgb and not normal)
    tex.set_editor_property("compression_settings",
                            unreal.TextureCompressionSettings.TC_NORMALMAP if normal else unreal.TextureCompressionSettings.TC_DEFAULT)
    EAL.save_loaded_asset(tex)
    return tex


def mpc():
    path = f"{FOLDER}/MPC_Albion"
    if EAL.does_asset_exist(path):
        c = unreal.load_asset(path)
    else:
        c = TOOLS.create_asset("MPC_Albion", FOLDER, unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
    scalars = []
    for n, v in (("Rain", 0.0), ("Wet", 0.0), ("Snow", 0.0), ("Fog", 0.0), ("Sun", 1.0), ("Frost", 0.0)):
        p = unreal.CollectionScalarParameter()
        p.set_editor_property("parameter_name", n)
        p.set_editor_property("default_value", v)
        scalars.append(p)
    vectors = []
    for n, v in (("SunDir", unreal.LinearColor(0.0, 0.32, 0.95, 0.0)),):
        p = unreal.CollectionVectorParameter()
        p.set_editor_property("parameter_name", n)
        p.set_editor_property("default_value", v)
        vectors.append(p)
    c.set_editor_property("scalar_parameters", scalars)
    c.set_editor_property("vector_parameters", vectors)
    EAL.save_loaded_asset(c)
    return c


class G:
    """A tiny graph helper for Albion's masters."""

    def __init__(self, m):
        self.m = m
        self.n = 0

    def e(self, cls, **props):
        self.n += 1
        x = -1800 + (self.n // 20) * 320
        y = -1200 + (self.n % 20) * 120
        node = MEL.create_material_expression(self.m, cls, x, y)
        for k, v in props.items():
            node.set_editor_property(k, v)
        return node

    def link(self, a, b, pin="", out=""):
        if isinstance(a, tuple):
            a, out = a
        if not MEL.connect_material_expressions(a, out, b, pin):
            warn(f"{self.m.get_name()}: could not connect {a.get_name()}[{out}] → {b.get_name()}[{pin}]")

    def scalar(self, name, v):
        return self.e(unreal.MaterialExpressionScalarParameter, parameter_name=name, default_value=v)

    def vector(self, name, c):
        return self.e(unreal.MaterialExpressionVectorParameter, parameter_name=name, default_value=unreal.LinearColor(*c, 1.0))

    def coll(self, c, name, vector=False):
        n = self.e(unreal.MaterialExpressionCollectionParameter, collection=c, parameter_name=name)
        return n

    def custom(self, code, inputs, out_type, extra=()):
        n = self.e(unreal.MaterialExpressionCustom)
        n.set_editor_property("code", code)
        n.set_editor_property("output_type", out_type)
        ins = []
        for name, _ in inputs:
            ci = unreal.CustomInput()
            ci.set_editor_property("input_name", name)
            ins.append(ci)
        n.set_editor_property("inputs", ins)
        outs = []
        for name, t in extra:
            co = unreal.CustomOutput()
            co.set_editor_property("output_name", name)
            co.set_editor_property("output_type", t)
            outs.append(co)
        if outs:
            n.set_editor_property("additional_outputs", outs)
        for name, src in inputs:
            self.link(src, n, name)
        return n

    def prop(self, src, prop, out=""):
        if isinstance(src, tuple):
            src, out = src
        MEL.connect_material_property(src, out, prop)


F1, F3 = unreal.CustomMaterialOutputType.CMOT_FLOAT1, unreal.CustomMaterialOutputType.CMOT_FLOAT3

RAIN_HLSL = r"""
// London's weather on a pane: beads, rivulets down its slope, snow on the panes that face up. WP in cm.
float3 P = WP * 0.01;
float3 N = normalize(Nrm);
float3 Up = float3(0, 0, 1);
float3 Down = normalize(-Up + N * dot(Up, N) + float3(1e-4, 0, 0));      // down the pane
float3 Across = normalize(cross(N, Down));
float2 q = float2(dot(P, Across), dot(P, Down));                        // metres on the pane, y down its slope
float slope = saturate(1.0 - abs(N.z));                                  // 0 on a flat pane, 1 upright
// Beads: a hashed cell of 9 mm, one drop in some of them, a little lens (its normal) of 1.5 ... 3.5 mm.
float2 c = q / 0.009;
float2 ci = floor(c);
float2 cf = frac(c) - 0.5;
float h = frac(sin(dot(ci, float2(127.1, 311.7))) * 43758.5453);
float h2 = frac(sin(dot(ci, float2(269.5, 183.3))) * 43758.5453);
float2 o = float2(h - 0.5, h2 - 0.5) * 0.5;
float r = lerp(0.17, 0.39, h2);
float2 d = cf - o;
float dl = length(d);
float present = step(1.0 - Rain * 0.85, h);
float bead = present * saturate(1.0 - dl / r);
float3 beadN = (Across * d.x + Down * d.y) * (bead > 0 ? 1.6 : 0.0);
// Rivulets: a few lines running down the slope, wandering, moving (faster on steep panes).
float lane = q.x / 0.035;
float li = floor(lane);
float lh = frac(sin(li * 91.7) * 43758.5453);
float wander = sin(q.y * 7.0 + lh * 40.0) * 0.18 + sin(q.y * 23.0 + lh * 11.0) * 0.06;
float lx = abs(frac(lane) - 0.5 - wander);
float run = frac(q.y * 0.35 - Time * (0.25 + slope * 1.2) * (0.6 + lh) + lh);
float streak = step(1.0 - Rain * 0.35, lh) * saturate(1.0 - lx / 0.09) * smoothstep(0.0, 0.2, run) * (1.0 - smoothstep(0.6, 1.0, run));
float3 streakN = Across * (frac(lane) - 0.5 - wander) * streak * 2.0;
// A film of water on a wet pane, even between the beads: a slow sheet flow.
Drops = saturate(bead + streak) * saturate(Rain * 1.5);
Streak = streak * saturate(Rain * 1.5);
float snowFace = smoothstep(0.15, 0.5, N.z);
float drift = 0.75 + 0.25 * sin(q.x * 3.1 + q.y * 1.7) * sin(q.y * 5.3);
SnowCov = saturate(Snow * snowFace * drift * 1.2);
return (beadN + streakN) * saturate(Rain * 1.5) * (1.0 - SnowCov);
"""


def build_glass(c):
    m = S.new_material(FOLDER, "M_Albion_Glass")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("two_sided", True)
    m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    m.set_editor_property("tangent_space_normal", False)
    try:
        m.set_editor_property("refraction_method", unreal.RefractionMode.RM_NONE)
    except Exception:  # noqa: BLE001
        pass
    g = G(m)
    wp = g.e(unreal.MaterialExpressionWorldPosition)
    vn = g.e(unreal.MaterialExpressionVertexNormalWS)
    t = g.e(unreal.MaterialExpressionTime)
    rain = g.coll(c, "Rain")
    snow = g.coll(c, "Snow")
    cu = g.custom(RAIN_HLSL, [("WP", wp), ("Nrm", vn), ("Time", t), ("Rain", rain), ("Snow", snow)], F3,
                  extra=[("Drops", F1), ("Streak", F1), ("SnowCov", F1)])
    # Waviness of drawn Victorian glass: a slow ripple in each pane's normal.
    wv = g.custom("float3 P = WP * 0.01; float a = sin(P.x * 3.7 + P.y * 1.3 + P.z * 2.1) * sin(P.y * 2.9 - P.z * 1.7 + P.x * 0.7);"
                  "float b = sin(P.x * 1.9 - P.y * 3.3 + P.z * 0.9) * sin(P.z * 3.1 + P.x * 1.1);"
                  "return float3(a, b, a * b) * Amt;", [("WP", wp), ("Amt", g.scalar("Waviness", 0.012))], F3)
    nsum = g.e(unreal.MaterialExpressionAdd)
    g.link(vn, nsum, "A")
    g.link(cu, nsum, "B")
    nsum2 = g.e(unreal.MaterialExpressionAdd)
    g.link(nsum, nsum2, "A")
    g.link(wv, nsum2, "B")
    norm = g.e(unreal.MaterialExpressionNormalize)
    g.link(nsum2, norm)
    tsgn = g.e(unreal.MaterialExpressionTwoSidedSign)
    nfinal = g.e(unreal.MaterialExpressionMultiply)
    g.link(norm, nfinal, "A")
    g.link(tsgn, nfinal, "B")
    g.prop(nfinal, unreal.MaterialProperty.MP_NORMAL)
    # Transmittance: the glass's tint, dimmed where water scatters (beads, rivulets) and where snow lies.
    tint = g.vector("Tint", (0.90, 0.95, 0.92))
    tr = g.custom("return Tint * (1.0 - 0.28 * Drops - 0.15 * Streak) * (1.0 - 0.85 * SnowCov);",
                  [("Tint", tint), ("Drops", (cu, "Drops")), ("Streak", (cu, "Streak")), ("SnowCov", (cu, "SnowCov"))], F3)
    out = g.e(unreal.MaterialExpressionThinTranslucentMaterialOutput)
    g.link(tr, out, "TransmittanceColor")
    # A London roof's grime: each pane its own (some washed, some not; the panes about 0.75 × 0.6 m), heavier along its
    # edges where the putty and the bars hold the soot, in streaks down the slope; rain washes it a little thinner.
    grime = g.custom("float3 P = WP * 0.01; float2 cell = floor(float2(P.y / 0.75, (P.x + P.z) / 0.62));"
                     "float r = frac(sin(dot(cell, float2(12.9898, 78.233))) * 43758.5453);"
                     "float2 f = frac(float2(P.y / 0.75, (P.x + P.z) / 0.62));"
                     "float edge = pow(saturate(1.0 - min(min(f.x, 1.0 - f.x), min(f.y, 1.0 - f.y)) * 9.0), 2.0);"
                     "float streak = 0.5 + 0.5 * sin(P.y * 41.0 + sin(P.x * 3.1) * 2.0);"
                     "return saturate((0.25 + 0.75 * r * r) * (0.55 + 0.45 * streak) + 0.8 * edge) * Amount * (1.0 - 0.4 * Wet);",
                     [("WP", wp), ("Amount", g.scalar("Grime", 1.0)), ("Wet", g.coll(c, "Wet"))], F1)
    # Coverage: the grime's film (up to 5 %: the audit found the vault's glass read as nothing at all, the sky through it
    # a clean blue; a real Victorian roof's panes are filmed and edged with soot), the water's film a little, snow a lot.
    cov = g.custom("return 0.002 + 0.2 * Grime + 0.02 * Drops + 0.8 * SnowCov;",
                   [("Grime", grime), ("Drops", (cu, "Drops")), ("SnowCov", (cu, "SnowCov"))], F1)
    g.prop(cov, unreal.MaterialProperty.MP_OPACITY)
    base = g.custom("return lerp(lerp(float3(0.62, 0.62, 0.6), float3(0.34, 0.32, 0.29), saturate(Grime * 1.2)), float3(0.86, 0.88, 0.9), SnowCov);",
                    [("Grime", grime), ("SnowCov", (cu, "SnowCov"))], F3)
    g.prop(base, unreal.MaterialProperty.MP_BASE_COLOR)
    rough = g.custom("return lerp(0.012 + 0.03 * Drops, 0.85, SnowCov);", [("Drops", (cu, "Drops")), ("SnowCov", (cu, "SnowCov"))], F1)
    g.prop(rough, unreal.MaterialProperty.MP_ROUGHNESS)
    g.prop(g.scalar("Specular", 0.5), unreal.MaterialProperty.MP_SPECULAR)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_THIN_TRANSLUCENT)
    errors = list(MEL.recompile_material(m) or [])
    log(f"M_Albion_Glass: {len(errors)} errors {errors}")
    EAL.save_loaded_asset(m)
    return m


def build_stained(atlas_default):
    m = S.new_material(FOLDER, "M_Albion_Stained")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("two_sided", True)
    m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    m.set_editor_property("tangent_space_normal", False)
    g = G(m)
    uv = g.e(unreal.MaterialExpressionTextureCoordinate)
    tex = g.e(unreal.MaterialExpressionTextureSampleParameter2D, parameter_name="Atlas")
    tex.set_editor_property("texture", atlas_default)
    g.link(uv, tex, "UVs")
    wp = g.e(unreal.MaterialExpressionWorldPosition)
    vn = g.e(unreal.MaterialExpressionVertexNormalWS)
    # Antique glass: seeds and a ripple (its surface catches the light in small pools).
    rip = g.custom("float3 P = WP * 0.01; float a = sin(P.x * 41.0 + P.y * 37.0 + P.z * 29.0) + 0.5 * sin(P.x * 97.0 - P.z * 83.0 + P.y * 61.0);"
                   "float b = sin(P.z * 43.0 - P.x * 31.0 + P.y * 17.0) + 0.5 * sin(P.y * 101.0 + P.z * 79.0);"
                   "return normalize(Nrm + float3(a, b, a - b) * 0.02);", [("WP", wp), ("Nrm", vn)], F3)
    tsgn = g.e(unreal.MaterialExpressionTwoSidedSign)
    nfinal = g.e(unreal.MaterialExpressionMultiply)
    g.link(rip, nfinal, "A")
    g.link(tsgn, nfinal, "B")
    g.prop(nfinal, unreal.MaterialProperty.MP_NORMAL)
    tr = g.custom("return Col.rgb * (1.0 - Col.a) * Gain;", [("Col", (tex, "RGBA")), ("Gain", g.scalar("Gain", 1.0))], F3)
    out = g.e(unreal.MaterialExpressionThinTranslucentMaterialOutput)
    g.link(tr, out, "TransmittanceColor")
    cov = g.custom("return saturate(Col.a);", [("Col", (tex, "RGBA"))], F1)
    g.prop(cov, unreal.MaterialProperty.MP_OPACITY)
    g.prop(g.vector("LeadColor", (0.10, 0.10, 0.10)), unreal.MaterialProperty.MP_BASE_COLOR)
    g.prop(g.scalar("Roughness", 0.08), unreal.MaterialProperty.MP_ROUGHNESS)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_THIN_TRANSLUCENT)
    errors = list(MEL.recompile_material(m) or [])
    log(f"M_Albion_Stained: {len(errors)} errors {errors}")
    EAL.save_loaded_asset(m)
    return m


SUN_LF_HLSL = r"""
// The sun's ray from this point (m) to the stained glass it crosses, if any. Returns (white, coloured) as a float3:
// Channel < 0 → the white share (1 − the stained coverage); Channel 0/1/2 → that colour of the glass's transmittance.
float3 P = WP * 0.01;
float3 L = normalize(SunDir.xyz + float3(0, 0, 1e-5));
float cover = 0.0;
float3 T = 1.0;
// The south screen's band: plane y = 53.5, x ±4.8, 9.0 ... 12.6 (two rows: 9.0-10.7 lower, 10.9-12.6 upper).
if (L.y > 1e-4)
{
    float t = (53.5 - P.y) / L.y;
    float3 Q = P + L * t;
    if (t > 0.0 && abs(Q.x) < 4.8 && Q.z > 9.0 && Q.z < 12.6 && !(Q.z > 10.7 && Q.z < 10.9))
    {
        float u = (4.8 - Q.x) / 9.6;
        float v = Q.z >= 10.9 ? (12.6 - Q.z) / 1.7 * 0.5 : 0.5 + (10.7 - Q.z) / 1.7 * 0.5;
        float4 s = Texture2DSampleLevel(Band, BandSampler, float2(u, v), 0);
        T = s.rgb * (1.0 - s.a);
        cover = 1.0;
    }
}
// The lancets: planes x = ±14.3, one a bay (centres y 20.2 + 6 k), 1.42 wide, 5.0 ... 9.01.
for (int side = 0; side < 2; ++side)
{
    float sx = side == 0 ? -1.0 : 1.0;
    if (L.x * sx > 1e-4 && cover < 0.5)
    {
        float t = (sx * 14.3 - P.x) / L.x;
        float3 Q = P + L * t;
        float b = floor((Q.y - 17.2) / 6.0);
        float yc = 20.2 + 6.0 * b;
        if (t > 0.0 && b >= 0.0 && b <= 5.0 && abs(Q.y - yc) < 0.71 && Q.z > 5.0 && Q.z < 9.01)
        {
            float u = (Q.y - (yc - 0.71)) / 1.42;
            if (side == 0) u = 1.0 - u;
            float2 uv = float2((b + saturate(u)) / 6.0, (9.01 - Q.z) / 4.01);
            float4 s = side == 0 ? Texture2DSampleLevel(West, WestSampler, uv, 0) : Texture2DSampleLevel(East, EastSampler, uv, 0);
            T = s.rgb * (1.0 - s.a);
            cover = 1.0;
        }
    }
}
// Albion under London's sky: its cloud takes the sun off the building (a soft edge 2 m wide round it).
float2 ext = max(float2(abs(P.x) - 15.6, max(10.2 - P.y, P.y - 55.0)), 0.0);
float inside = 1.0 - saturate(length(ext) / 2.0);
float sun = lerp(1.0, SunOn, inside);
// Under the court's clear glass (roof and screens) the sun loses what a pane reflects and absorbs.
float court = (abs(P.x) < 14.0 && P.y > 17.2 && P.y < 53.2 && P.z > -1.0 && P.z < 17.5) ? 1.0 : 0.0;
float clearT = lerp(1.0, 0.86, court);
if (Channel < -0.5) return (1.0 - cover) * sun * clearT;
float k = Channel < 0.5 ? T.r : (Channel < 1.5 ? T.g : T.b);
return cover * k * sun;
"""


def build_sun_lf(c, band, west, east):
    m = S.new_material(FOLDER, "M_Albion_SunLF")
    m.set_editor_property("material_domain", unreal.MaterialDomain.MD_LIGHT_FUNCTION)
    g = G(m)
    wp = g.e(unreal.MaterialExpressionWorldPosition)
    sd = g.e(unreal.MaterialExpressionCollectionParameter, collection=c, parameter_name="SunDir")
    son = g.coll(c, "Sun")
    ch = g.scalar("Channel", -1.0)
    tb = g.e(unreal.MaterialExpressionTextureObjectParameter, parameter_name="BandAtlas")
    tb.set_editor_property("texture", band)
    tw = g.e(unreal.MaterialExpressionTextureObjectParameter, parameter_name="WestAtlas")
    tw.set_editor_property("texture", west)
    te = g.e(unreal.MaterialExpressionTextureObjectParameter, parameter_name="EastAtlas")
    te.set_editor_property("texture", east)
    cu = g.custom(SUN_LF_HLSL, [("WP", wp), ("SunDir", sd), ("SunOn", son), ("Channel", ch), ("Band", tb), ("West", tw), ("East", te)], F1)
    g.prop(cu, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    errors = list(MEL.recompile_material(m) or [])
    log(f"M_Albion_SunLF: {len(errors)} errors {errors}")
    EAL.save_loaded_asset(m)
    for name, channel in (("MI_Albion_SunLF_White", -1.0), ("MI_Albion_SunLF_R", 0.0), ("MI_Albion_SunLF_G", 1.0), ("MI_Albion_SunLF_B", 2.0)):
        mi = simple_instance(name, m)
        MEL.set_material_instance_scalar_parameter_value(mi, "Channel", channel)
        MEL.update_material_instance(mi)
        EAL.save_loaded_asset(mi)
    return m


def simple_instance(name, parent):
    path = f"{FOLDER}/{name}"
    if EAL.does_asset_exist(path):
        mi = unreal.load_asset(path)
    else:
        mi = TOOLS.create_asset(name, FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, parent)
    return mi


def placeholder_atlases():
    """Until albion_glass.py has made the stained glass: plain coloured cells with lead lines, so the pipeline runs."""
    os.makedirs(ART, exist_ok=True)
    try:
        from PIL import Image, ImageDraw
    except ImportError:
        return
    for name, cols, rows in (("albion_tristram", 6, 2), ("albion_lancets_west", 6, 1), ("albion_lancets_east", 6, 1)):
        png = os.path.join(ART, f"T_{name}.png")
        if os.path.exists(png):
            continue
        W, H = 1536, 512 if rows == 2 else 1024
        im = Image.new("RGBA", (W, H), (200, 210, 205, 0))
        d = ImageDraw.Draw(im)
        pal = [(40, 70, 150), (150, 30, 30), (200, 160, 40), (40, 110, 60), (110, 60, 130), (190, 100, 50)]
        for r in range(rows):
            for k in range(cols):
                x0, y0 = k * W // cols, r * H // rows
                d.rectangle([x0 + 8, y0 + 8, x0 + W // cols - 8, y0 + H // rows - 8], fill=pal[(k + r) % 6] + (0,))
                d.rectangle([x0, y0, x0 + W // cols - 1, y0 + H // rows - 1], outline=(20, 20, 20, 255), width=8)
        im.save(png)


# ---------------------------------------------------------------------------------------------- main

STATS_MINE = {}


def main():
    global STATS_MINE
    STATS_MINE = add_stats()
    if not EAL.does_directory_exist(FOLDER):
        EAL.make_directory(FOLDER)
    # The masters' parameter names (materials.py reads them back to check every instance's values).
    for name in ("M_Stone", "M_StoneRelief", "M_Metal", "M_Fabric", "M_Wood", "M_Plaster", "M_Ext_Stone"):
        m = unreal.load_asset(f"{P.MATERIALS}/{name}")
        if m:
            MAT.PARAMS[name] = MAT.parameter_names(m)
    specs = {}
    specs.update(stone_specs())
    specs.update(metal_specs())
    specs.update(wood_specs())
    specs.update(book_specs())
    made = []
    for name, s in specs.items():
        parent = unreal.load_asset(f"{P.MATERIALS}/{s['parent']}")
        if parent is None:
            warn(f"{name}: parent {s['parent']} missing")
            continue
        if MAT.material_instance(name, FOLDER, parent, s, {}):
            made.append(name)
    log(f"instances: {len(made)} ({', '.join(made)})")
    c = mpc()
    placeholder_atlases()
    band = texture("albion_tristram")
    west = texture("albion_lancets_west")
    east = texture("albion_lancets_east")
    glass = build_glass(c)
    mi = simple_instance("MI_Albion_Glass", glass)
    MEL.update_material_instance(mi)
    EAL.save_loaded_asset(mi)
    stained = build_stained(band)
    for name, tex in (("MI_Albion_Tristram", band), ("MI_Albion_LancetWest", west), ("MI_Albion_LancetEast", east)):
        mi = simple_instance(name, stained)
        if tex:
            MEL.set_material_instance_texture_parameter_value(mi, "Atlas", tex)
        MEL.update_material_instance(mi)
        EAL.save_loaded_asset(mi)
    # The table cases' glass: the museum's vitrine glass (M_Ch_CaseGlass: plain translucency with an anti-reflective
    # coating's faint reflection and no velocity; M_Glass's Thin Translucent smeared a milky haze over small lit cases).
    case_parent = unreal.load_asset(f"{P.MATERIALS}/Chenghuai/M_Ch_CaseGlass")
    if case_parent is None:
        warn("M_Ch_CaseGlass missing (chenghuai.py makes it): the cases keep M_Glass")
        case_parent = unreal.load_asset(f"{P.MATERIALS}/M_Glass")
    case = simple_instance("MI_Albion_CaseGlass", case_parent)
    MEL.update_material_instance(case)
    EAL.save_loaded_asset(case)
    build_sun_lf(c, band, west, east)
    build_page()
    EAL.save_directory(FOLDER, only_if_is_dirty=True, recursive=True)
    if MAT.WARNINGS:
        log(f"{len(MAT.WARNINGS)} warnings from materials.py's checks (see above)")
    log("done")


if __name__ == "__main__":
    main()
