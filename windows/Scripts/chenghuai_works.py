"""
Chenghuai's works (Rooms and Hang boards), placed by chenghuai.py's `works` step inside the editor:

  * the scrolls: AChenghuaiScroll actors at their true sizes in their mounts, the images from assets/paintings
    (imported to /Game/Museum/Textures/Chenghuai, one material each on M_Ch_Silk or M_Ch_PaperWork);
      - 臨池 (the front row): five calligraphy handscrolls on the 15.2 m slanted case, west to east, each beginning at the
        reader's right;
      - 澄懷堂 (the main hall): Guo Xi, Fan Kuan, Li Tang hanging in their full-height cases;
      - 清閟 / 停雲 (the ear rooms): Ni Zan and Wang Meng; Shen Zhou and Wen Zhengming;
      - 舒卷 (the rear row): the Thousand Li and the Qingming scrolls on their slanted cases, beginning at the east;
  * the ceramics: the museum's scanned pieces (the sancai horse, the Ru basin, the meiping, the chicken cup, the
    peachbloom vase) and AChenghuaiVessel pieces turned from the Met's photographs (chenghuai_vessels.py), each on a
    carved nanmu stand; the Ru basin alone on its pedestal; the chicken cup's handling copy on the east hall's table.

Every actor carries work:<id> (its placard, assets/collection.json), musee.wing:Chenghuai and musee.chenghuai.works;
a run replaces the previous run's. Plan metres as ChenghuaiPlan.h (x east, y south, z up; Unreal = × 100).
"""
import json
import math
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.append(HERE)
import musee_paths as P  # noqa: E402
import materials as MAT  # noqa: E402
import setup_project as S  # noqa: E402

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
MATS = P.MATERIALS + "/Chenghuai/Works"
TEXS = P.MUSEUM_CONTENT + "/Textures/Chenghuai"
CH_MATS = P.MATERIALS + "/Chenghuai"
ART = os.path.join(P.REPO_DIR, "assets", "paintings")
TAG = "musee.chenghuai.works"
WING = "musee.wing:Chenghuai"
SCROLL = "/Script/MuseeVision.ChenghuaiScroll"
VESSEL = "/Script/MuseeVision.ChenghuaiVessel"
SLANT = math.radians(11.0)


def log(m):
    unreal.log(f"[chenghuai] works: {m}")


def warn(m):
    unreal.log_warning(f"[chenghuai] works: {m}")


# ---------------------------------------------------------------------------------------------- the house's numbers
# (ChenghuaiPlan.h, ChenghuaiDisplay.cpp)
FRONT_F, SIDE_F, HALL_F, EAR_F, REAR_F = 0.30, 0.45, 0.75, 0.50, 0.30
AXIS = -9.2
LINCHI = dict(x0=-19.9, x1=-4.7, front=-18.3, depth=0.8, toward=+1)     # v runs south to the wall
REAR = dict(front=-60.6, depth=0.8, toward=-1)                          # v runs north to the wall
THOUSAND_X, QINGMING_X = (-19.7, -6.3), (-5.3, 0.9)
HALL_CASES = [(-13.55, -11.85), (-10.0, -8.4), (-6.7, -4.7)]
HALL_BACK, HALL_CASE_D = -51.4, 0.6
EAR_N, EAR_X0, EAR_X1, EAR_D = -50.4, -17.2, -14.7, 0.45
EAR_SIDE_Y = (-49.6, -48.4)
SIDE_CASE_Y = (-40.4, -32.0)
SIDE_CASE_D, SIDE_BACK_W = 0.6, -20.2
LONE = (-18.2, -36.2)

# Mount proportions (装裱, 立轴): heaven and earth in metres; the bands and borders.
BAND, BORDER = 0.06, 0.04


# ---------------------------------------------------------------------------------------------- materials

def _custom(m, code, inputs, out_type):
    n = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -600, 0)
    n.set_editor_property("code", code)
    n.set_editor_property("output_type", out_type)
    ins = []
    for name, _ in inputs:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", name)
        ins.append(ci)
    n.set_editor_property("inputs", ins)
    for name, (src, out) in inputs:
        MEL.connect_material_expressions(src, out, n, name)
    return n


def _master(name, rough_code, normal_code):
    """A work's image: the colour from the texture (UV0), the roughness from its tone (ink is duller than the ground
    where it lies thick; silk's sheen is in the lighter ground), a faint weave or fibre in the normal."""
    if EAL.does_asset_exist(f"{MATS}/{name}") and not os.environ.get("CH_REMAKE_MASTERS"):
        return unreal.load_asset(f"{MATS}/{name}")   # kept: the works' instances hang on it (CH_REMAKE_MASTERS=1 remakes it)
    if EAL.does_asset_exist(f"{MATS}/{name}"):
        EAL.delete_asset(f"{MATS}/{name}")
    m = S.new_material(MATS, name)
    uv = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1400, 0)
    tex = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -1200, 0)
    tex.set_editor_property("parameter_name", "Tex")
    tex.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/DefaultTexture"))
    MEL.connect_material_expressions(uv, "", tex, "UVs")
    MEL.connect_material_property(tex, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    F1, F3 = unreal.CustomMaterialOutputType.CMOT_FLOAT1, unreal.CustomMaterialOutputType.CMOT_FLOAT3
    r = _custom(m, rough_code, [("C", (tex, "RGB"))], F1)
    MEL.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    wp = MEL.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -1400, 400)
    nn = _custom(m, normal_code, [("P", (wp, ""))], F3)
    MEL.connect_material_property(nn, "", unreal.MaterialProperty.MP_NORMAL)
    spec = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 300)
    spec.set_editor_property("r", 0.4)
    MEL.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    errs = list(MEL.recompile_material(m) or [])
    if errs:
        warn(f"{name}: {errs}")
    EAL.save_loaded_asset(m)
    return m


def masters():
    silk = _master("M_Ch_Silk",
                   "float l = dot(C, float3(0.3, 0.59, 0.11)); return lerp(0.8, 0.62, saturate(l * 1.6));",
                   # plain-woven silk: warp and weft about 8 a millimetre, far too fine to see (the mesh carries the cockle).
                   "return float3(0.0, 0.0, 1.0);")
    paper = _master("M_Ch_PaperWork",
                    "float l = dot(C, float3(0.3, 0.59, 0.11)); return lerp(0.88, 0.76, saturate(l * 1.4));",
                    "float n = frac(sin(dot(floor(P.xy * 3.0), float2(12.9898, 78.233))) * 43758.5453);"
                    "return normalize(float3((n - 0.5) * 0.02, (frac(n * 7.13) - 0.5) * 0.02, 1.0));")
    return {"silk": silk, "paper": paper}


def import_texture(path, name):
    dest = f"{TEXS}/{name}"
    pkg = os.path.join(P.PROJECT_DIR, "Content", dest[len("/Game/"):] + ".uasset")
    if EAL.does_asset_exist(dest) and os.path.exists(pkg) and os.path.getmtime(pkg) >= os.path.getmtime(path):
        return unreal.load_asset(dest)
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = TEXS
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = True
    TOOLS.import_asset_tasks([task])
    t = unreal.load_asset(dest)
    if t:
        t.set_editor_property("srgb", True)
        t.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
        t.set_editor_property("max_texture_size", 8192)
        EAL.save_loaded_asset(t)
    return t


def instance(name, parent, params=None, textures=None):
    path = f"{MATS}/{name}"
    mi = unreal.load_asset(path) if EAL.does_asset_exist(path) else TOOLS.create_asset(name, MATS, unreal.MaterialInstanceConstant,
                                                                                         unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, parent)
    for k, v in (textures or {}).items():
        MEL.set_material_instance_texture_parameter_value(mi, k, v)
    MEL.update_material_instance(mi)
    EAL.save_loaded_asset(mi)
    return mi


def image_materials(work, M):
    """One material per image tile."""
    out = []
    for f in work["images"]:
        t = import_texture(os.path.join(ART, f), "T_" + os.path.splitext(f)[0].replace("-", "_"))
        if not t:
            warn(f"{f}: not imported")
            continue
        out.append(instance("MI_" + os.path.splitext(f)[0].replace("-", "_"), M[work["ground"]], textures={"Tex": t}))
    return out


def vessel_material(name, rough, coat):
    """A turned vessel's glaze: M_Stone with the unwrapped photograph (UV0), the glaze as the clear coat."""
    MAT.PARAMS["M_Stone"] = MAT.parameter_names(unreal.load_asset(f"{P.MATERIALS}/M_Stone"))
    tex = MAT.texture_asset(f"ch_v_{name}")
    if tex:
        tex.set_editor_property("srgb", True)
        EAL.save_loaded_asset(tex)
    s = MAT.merged(MAT.GLAZE, Roughness=rough, ClearCoat=coat, ClearCoatRoughness=0.05, TexScale=1.0, TexAspect=1.0, Specular=0.5,
                   MacroVariation=0.01)
    spec = MAT.spec("M_Stone", s, {"BaseColor": (1.0, 1.0, 1.0)}, textures={"Texture": f"ch_v_{name}"}, switches={"UseTexture": True})
    parent = unreal.load_asset(f"{P.MATERIALS}/M_Stone")
    MAT.material_instance(f"MI_Ch_Vessel_{name}", MATS, parent, spec, {f"ch_v_{name}": tex})
    return unreal.load_asset(f"{MATS}/MI_Ch_Vessel_{name}")


# ---------------------------------------------------------------------------------------------- the works

# Hanging scrolls: id, images, ground, image W × H (m), heaven, earth.
HANGING = {
    "chinese-guo-xi-early-spring": dict(images=["chinese-guo-xi-early-spring.jpg"], ground="silk", w=1.081, h=1.583, heaven=0.55, earth=0.3),
    "chinese-fan-kuan-travelers": dict(images=["chinese-fan-kuan-travelers.jpg"], ground="silk", w=1.033, h=2.063, heaven=0.62, earth=0.36),
    "chinese-li-tang-wind-in-pines": dict(images=["chinese-li-tang-wind-in-pines.jpg"], ground="silk", w=1.398, h=1.887, heaven=0.6, earth=0.32),
    "chinese-ni-zan-rongxi": dict(images=["chinese-ni-zan-rongxi.jpg"], ground="paper", w=0.355, h=0.747, heaven=0.36, earth=0.2),
    "chinese-wang-meng-qingbian": dict(images=["chinese-wang-meng-qingbian.jpg"], ground="paper", w=0.422, h=1.406, heaven=0.42, earth=0.22),
    "chinese-shen-zhou-lushan": dict(images=["chinese-shen-zhou-lushan.jpg"], ground="paper", w=0.981, h=1.938, heaven=0.42, earth=0.22),
    "chinese-wen-zhengming-living-aloft": dict(images=["chinese-wen-zhengming-living-aloft.jpg"], ground="paper", w=0.457, h=0.953,
                                               heaven=0.4, earth=0.22),
}
# Handscrolls: id, image tiles (left to right), ground, height (m) and width (from the images' aspect unless given).
HAND = {
    "chinese-orchid-pavilion": dict(images=["chinese-orchid-pavilion.jpg"], ground="paper", h=0.245),
    "chinese-yan-zhenqing-nephew": dict(images=["chinese-yan-zhenqing-nephew.jpg"], ground="paper", h=0.282),
    "chinese-huaisu-autobiography": dict(images=["chinese-huaisu-autobiography-1.jpg", "chinese-huaisu-autobiography-2.jpg"], ground="paper",
                                         h=0.283),
    "chinese-su-shi-cold-food": dict(images=["chinese-su-shi-cold-food.jpg"], ground="paper", h=0.342),
    "chinese-mi-fu-shu-su": dict(images=["chinese-mi-fu-shu-su.jpg"], ground="silk", h=0.278),
    "chinese-thousand-li": dict(images=[f"chinese-thousand-li-{k}.jpg" for k in (1, 2, 3, 4)], ground="silk", h=0.515, w=11.915),
    "chinese-qingming": dict(images=[f"chinese-qingming-{k}.jpg" for k in (1, 2, 3, 4)], ground="silk", h=0.248),
}


def aspect_width(work):
    w = 0
    for f in work["images"]:
        iw, ih = jpeg_size(os.path.join(ART, f))
        w += iw / ih
    return w * work["h"]


def jpeg_size(path):
    """The pixel size from a JPEG's SOF marker (no PIL in the editor's Python)."""
    with open(path, "rb") as f:
        data = f.read()
    i = 2
    while i < len(data):
        if data[i] != 0xFF:
            i += 1
            continue
        marker = data[i + 1]
        length = int.from_bytes(data[i + 2:i + 4], "big")
        if marker in (0xC0, 0xC1, 0xC2):
            h = int.from_bytes(data[i + 5:i + 7], "big")
            w = int.from_bytes(data[i + 7:i + 9], "big")
            return w, h
        i += 2 + length
    raise ValueError(path)


def cm(x, y, z):
    return unreal.Vector(x * 100.0, y * 100.0, z * 100.0)


def rot_xz(xv, zv):
    return unreal.MathLibrary.make_rot_from_xz(unreal.Vector(*xv), unreal.Vector(*zv))


def spawn(eas, cls, loc, rot, label, work_id):
    a = eas.spawn_actor_from_class(cls, loc, rot)
    a.set_actor_label(label)
    tags = [unreal.Name(TAG), unreal.Name(WING)]
    if work_id:
        tags.append(unreal.Name("work:" + work_id))
    a.tags = list(a.tags) + tags
    return a


def mesh_actor(eas, mesh, loc, rot=None):
    """A StaticMeshActor showing mesh (spawn_actor_from_object returns None in the commandlet)."""
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, loc, rot or unreal.Rotator(0, 0, 0))
    if a:
        a.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
        a.static_mesh_component.set_static_mesh(mesh)
        a.static_mesh_component.set_mobility(unreal.ComponentMobility.STATIC)
    return a


def mount_materials():
    load = lambda n: unreal.load_asset(f"{CH_MATS}/{n}")  # noqa: E731
    return dict(mount=load("MI_Ch_MountSilk"), band=load("MI_Ch_MountBand"), wood=load("MI_Ch_Chestnut"), knob=load("MI_Ch_Knob"))


def set_scroll(a, kind, w, h, mats, images, **extra):
    a.set_editor_property("kind", kind)
    a.set_editor_property("image_width", w)
    a.set_editor_property("image_height", h)
    for k, v in extra.items():
        a.set_editor_property(k, v)
    a.set_editor_property("image_materials", images)
    a.set_editor_property("mount_material", mats["mount"])
    a.set_editor_property("band_material", mats["band"])
    a.set_editor_property("wood_material", mats["wood"])
    a.set_editor_property("knob_material", mats["knob"])
    a.rebuild()


def hang(eas, cls, M, mats, wid, at, facing):
    """A hanging scroll from its hook: at = (x, y, z) of the top stick's middle (plan m), facing = its normal (plan)."""
    w = HANGING[wid]
    images = image_materials(w, M)
    a = spawn(eas, cls, cm(*at), rot_xz((facing[0], facing[1], 0.0), (0.0, 0.0, 1.0)), wid.replace("chinese-", "Scroll "), wid)
    set_scroll(a, unreal.ChenghuaiScrollKind.HANGING, w["w"], w["h"], mats, images, heaven=w["heaven"], earth=w["earth"], band=BAND,
               border=BORDER)
    length = w["heaven"] + 2 * BAND + w["h"] + w["earth"]
    return a, length


def lay(eas, cls, M, mats, wid, origin, normal, up, lead, wrapper, roll):
    """A handscroll on a slanted bed: origin = its image's right-hand edge's middle (plan m); normal and up in plan."""
    w = HAND[wid]
    images = image_materials(w, M)
    a = spawn(eas, cls, cm(*origin), rot_xz(normal, up), wid.replace("chinese-", "Handscroll "), wid)
    width = w.get("w") or aspect_width(w)
    set_scroll(a, unreal.ChenghuaiScrollKind.HAND, width, w["h"], mats, images, lead=lead, wrapper=wrapper, roll_radius=roll,
               band=0.025 if wrapper <= 0 else 0.03, border=0.012)
    return a


def right_extent(lead, wrapper, band):
    """How far a handscroll reaches to the reader's right of its image (its lead, bands, wrapper or its roll)."""
    return 2 * band + lead + (wrapper + 0.19 if wrapper > 0 else 0.013)


def left_extent(band, roll):
    return band + roll


def handscroll_row(eas, cls, M, mats, ids, x_right, x_left, bed, lead, wrapper, roll, reader_right_is_west):
    """Lays handscrolls along a slanted bed from the reader's right to left, spacing them evenly."""
    widths = [HAND[i].get("w") or aspect_width(HAND[i]) for i in ids]
    band = 0.025 if wrapper <= 0 else 0.03
    spans = [right_extent(lead, wrapper, band) + w + left_extent(band, roll) for w in widths]
    free = abs(x_left - x_right) - sum(spans)
    gap = free / (len(ids) + 1)
    if gap < 0:
        warn(f"handscrolls {ids}: {-free:.2f} m too long for the case")
        gap = 0.0
    s = 1.0 if reader_right_is_west else -1.0           # the reader's leftward direction in x
    x = x_right
    for wid, w, span in zip(ids, widths, spans):
        x += s * gap
        origin_x = x + s * right_extent(lead, wrapper, band)
        lay(eas, cls, M, mats, wid, (origin_x, bed["y"], bed["z"]), bed["normal"], bed["up"], lead, wrapper, roll)
        x += s * span


def bed_line(case_front, depth, toward, floor, h):
    """Where a handscroll of height h lies on a slanted case's bed: its centre line (y, z), its normal and up (plan).
    The bed (ChenghuaiDisplay.cpp SlantCase): from v 0.05 (z F + 0.905) to v D − 0.06, tilted 11°, a 2 cm board whose
    top is 1 cm above that line."""
    va, vb = 0.05, depth - 0.06
    vm = 0.5 * (va + vb)
    zm = floor + 0.905 + math.tan(SLANT) * (vm - va)
    n_v, n_z = -math.sin(SLANT), math.cos(SLANT)        # up and toward the reader
    lift = 0.0115
    y = case_front + toward * (vm + n_v * lift)
    z = zm + n_z * lift
    return dict(y=y, z=z, normal=(0.0, toward * n_v, n_z), up=(0.0, toward * math.cos(SLANT), math.sin(SLANT)))


# ---------------------------------------------------------------------------------------------- ceramics

SCANNED = {
    # id: mesh, the target size (m) along its largest dimension or its height ("h").
    "chinese-sancai-horse": ("/Game/Museum/ChineseWing/ChineseWing/StaticMeshes/chinese_sancai_horse", "h", 0.70),
    "chinese-ru-basin": ("/Game/Museum/ChineseWing/ChineseWing/StaticMeshes/chinese_ru_basin", "l", 0.23),
    "chinese-meiping": ("/Game/Museum/ChineseWing/ChineseWing/StaticMeshes/chinese_meiping", "h", 0.441),
    "chinese-chicken-cup": ("/Game/Museum/ChineseWing/ChineseWing/StaticMeshes/chinese_chicken_cup", "l", 0.083),
    "chinese-peachbloom": ("/Game/Museum/ChineseWing/ChineseWing/StaticMeshes/chinese_peachbloom", "h", 0.159),
}
SCANNED_MATS = {
    "chinese-sancai-horse": "/Game/Museum/Materials/USD/MI_sancai_amber",
    "chinese-ru-basin": "/Game/Museum/Materials/USD/MI_celadon_ru",
    "chinese-meiping": "/Game/Museum/Materials/USD/MI_meiping",
    "chinese-chicken-cup": "/Game/Museum/Materials/USD/MI_chicken_cup",
    "chinese-peachbloom": "/Game/Museum/Materials/USD/MI_peachbloom",
}
VESSEL_GLAZE = {"ding": (0.3, 0.6), "jun": (0.28, 0.7), "guan": (0.42, 0.5), "jian": (0.14, 0.9), "longquan": (0.2, 0.8),
                "xuande": (0.12, 0.85), "wucai": (0.14, 0.85), "falangcai": (0.1, 0.9), "fishbowl": (0.32, 0.45), "tub": (0.3, 0.6)}


def place_scanned(eas, wid, x, y, deck, yaw, stand, label=None):
    mesh_path, how, size = SCANNED[wid]
    mesh = unreal.load_asset(mesh_path)
    if not mesh:
        warn(f"{wid}: mesh {mesh_path} missing")
        return None
    stand_h = 0.0
    if stand:
        stand_h = 0.03
        st = spawn(eas, unreal.load_class(None, VESSEL), cm(x, y, deck), unreal.Rotator(0, 0, 0), f"Stand {wid}", wid)
        st.set_editor_property("stand_radius", stand)
        st.set_editor_property("stand_height", stand_h)
        st.set_editor_property("stand_material", unreal.load_asset(f"{CH_MATS}/MI_Ch_Nanmu"))
        st.rebuild()
    # A plain StaticMeshActor with the mesh set on it (spawn_actor_from_object returned None for the horse once).
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, cm(x, y, deck + stand_h), unreal.Rotator(0, 0, yaw))
    if not a:
        warn(f"{wid}: could not place {mesh_path}")
        return None
    a.set_actor_label(label or wid)
    a.tags = [unreal.Name(TAG), unreal.Name(WING), unreal.Name("work:" + wid)]
    comp = a.static_mesh_component
    comp.set_mobility(unreal.ComponentMobility.MOVABLE)
    comp.set_static_mesh(mesh)
    comp.set_mobility(unreal.ComponentMobility.STATIC)
    mat = unreal.load_asset(SCANNED_MATS[wid])
    if mat:
        comp.set_material(0, mat)
    b = mesh.get_bounding_box()
    ext = [b.max.x - b.min.x, b.max.y - b.min.y, b.max.z - b.min.z]
    k = size * 100.0 / (ext[2] if how == "h" else max(ext))
    a.set_actor_scale3d(unreal.Vector(k, k, k))
    # Stand it on its base, centred over the spot.
    o, e = a.get_actor_bounds(False)
    a.set_actor_location(unreal.Vector(a.get_actor_location().x + (x * 100 - o.x), a.get_actor_location().y + (y * 100 - o.y),
                                       a.get_actor_location().z + ((deck + stand_h) * 100 - (o.z - e.z))), False, False)
    return a


def place_vessel(eas, profiles, name, wid, x, y, deck, yaw=0.0, stand=True, solid=False):
    p = profiles[name]
    mat = vessel_material(name, *VESSEL_GLAZE[name])
    a = spawn(eas, unreal.load_class(None, VESSEL), cm(x, y, deck), unreal.Rotator(0, 0, yaw), f"Vessel {name}", wid)
    a.set_editor_property("profile", [unreal.Vector2D(r, z) for r, z in p["profile"]])
    a.set_editor_property("material", mat)
    foot = next((r for r, z in p["profile"] if z == 0.0), p["radius"] * 0.5)
    a.set_editor_property("stand_radius", max(foot * 1.25, 0.03) if stand else 0.0)
    a.set_editor_property("stand_height", 0.028 if p["height"] < 0.2 else 0.04)
    a.set_editor_property("stand_material", unreal.load_asset(f"{CH_MATS}/MI_Ch_Nanmu"))
    a.rebuild()
    if solid:
        # Out in the court: the visitor walks round it.
        c = a.get_component_by_class(unreal.ProceduralMeshComponent)
        c.set_collision_profile_name("BlockAll")
        c.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
    return a


# ---------------------------------------------------------------------------------------------- the plants

# The Plan board's trees (plan metres; crown radius): the inner court's four beds, the rear court's pair, the garden's.
# The museum's Nature species stand in for the board's (see the report): 西府海棠 crabapple = OrchardFruit's apple;
# the rear court's jujubes = the gnarled Chinese plum; the garden's pine and willow = osmanthus and plum.
TREES = [
    ("Crabapple_W", (-12.2, -39.6), "ORCHARD_FRUIT", dict(fruit_variant=1, tree_height=4.6, crown_height=3.3, crown_radii=(1.7, 1.2)), 701),
    ("Crabapple_E", (-6.2, -39.6), "ORCHARD_FRUIT", dict(fruit_variant=1, tree_height=4.5, crown_height=3.2, crown_radii=(1.7, 1.2)), 702),
    ("Osmanthus_W", (-12.2, -32.8), "OSMANTHUS", dict(tree_height=3.6, crown_height=2.3, crown_radii=(1.3, 1.2)), 703),
    ("Osmanthus_E", (-6.2, -32.8), "OSMANTHUS", dict(tree_height=3.5, crown_height=2.3, crown_radii=(1.3, 1.2)), 704),
    ("Jujube_W", (-16.8, -52.3), "CHINESE_PLUM", dict(tree_height=5.2, crown_height=3.8, crown_radii=(1.9, 1.3)), 705),
    ("Jujube_E", (-1.6, -52.3), "CHINESE_PLUM", dict(tree_height=5.0, crown_height=3.7, crown_radii=(1.9, 1.3)), 706),
    ("Garden_Osmanthus_1", (6.0, -40.1), "OSMANTHUS", dict(tree_height=4.4, crown_height=2.9, crown_radii=(2.0, 1.6)), 707),
    ("Garden_Plum_1", (15.1, -54.0), "CHINESE_PLUM", dict(tree_height=3.6, crown_height=2.7, crown_radii=(1.2, 0.9)), 708),
    ("Garden_Plum_2", (14.7, -37.7), "CHINESE_PLUM", dict(tree_height=3.8, crown_height=2.8, crown_radii=(1.4, 1.0)), 709),
    ("Garden_Osmanthus_2", (5.1, -37.9), "OSMANTHUS", dict(tree_height=3.4, crown_height=2.2, crown_radii=(1.2, 1.1)), 710),
    ("Garden_Osmanthus_3", (13.4, -32.9), "OSMANTHUS", dict(tree_height=4.8, crown_height=3.2, crown_radii=(2.1, 1.7)), 711),
    ("Garden_Crabapple", (6.8, -22.3), "ORCHARD_FRUIT", dict(fruit_variant=1, tree_height=4.2, crown_height=3.0, crown_radii=(1.7, 1.2)), 712),
]
BAMBOO_CLUMPS = {
    "East": [(15.87, -44.6), (16.05, -42.28), (15.8, -45.08), (15.8, -40.54), (15.95, -41.74), (15.69, -43.12), (16.07, -40.35), (16.0, -44.8),
             (15.53, -44.41), (15.65, -43.26), (16.05, -45.14), (15.62, -42.94), (15.54, -41.31), (15.7, -42.35)],
    "North_W": [(8.05, -60.73), (8.72, -60.11), (6.46, -60.69), (9.52, -60.34), (6.84, -59.87), (8.76, -61.02), (8.95, -59.81), (6.5, -60.99),
                (10.01, -59.95), (6.58, -60.74), (8.47, -60.43), (7.5, -59.96), (9.09, -61.02), (6.47, -60.3), (5.63, -60.41), (5.33, -59.98),
                (7.59, -60.41), (7.08, -60.14)],
    "North_E": [(14.72, -60.55), (14.34, -60.46), (12.9, -60.6), (15.75, -60.5), (14.73, -61.02), (14.59, -60.78), (12.09, -59.95),
                (14.35, -60.87), (13.46, -60.47), (13.13, -61.1), (11.97, -60.37), (11.72, -60.58), (13.73, -60.89), (15.42, -61.1),
                (14.89, -60.63), (13.25, -60.65), (14.66, -59.99), (10.92, -60.47), (10.36, -60.64), (12.41, -60.81), (15.69, -60.74)],
}
LOTUS = [((7.6, -50.9), 0.9, 0.28), ((8.6, -49.8), 0.7, 0.0), ((6.8, -49.3), 0.8, 0.0), ((11.0, -41.5), 0.9, 0.3), ((9.8, -40.9), 0.7, 0.0),
         ((11.8, -42.5), 0.6, 0.0)]
ROCK = (5.3, -54.3)       # 石兄: 3 m inside the moon gate, on its axis (Garden board)
WATER = -0.28


def plants(eas):
    import nature as N
    sp = N.Spawner(True)
    n = 0
    for label, (x, y), species, props, seed in TREES:
        p = {"species": N.enum("MuseeTreeSpecies", species), "seed": seed}
        for k, v in props.items():
            p[k] = N.v2(*v) if isinstance(v, tuple) else v
        a = sp.spawn("MuseeTree", label, "Chenghuai", N.cm(x, y), p)
        if a:
            a.tags = list(a.tags) + [unreal.Name(TAG)]
            n += 1
    for i, (name, pts) in enumerate(BAMBOO_CLUMPS.items()):
        cx, cy = sum(p[0] for p in pts) / len(pts), sum(p[1] for p in pts) / len(pts)
        a = sp.spawn("MuseeTree", f"Bamboo_{name}", "Chenghuai", N.cm(cx, cy), {
            "species": N.enum("MuseeTreeSpecies", "BAMBOO"), "seed": 720 + i, "culms": [N.v2(x - cx, y - cy) for x, y in pts]})
        if a:
            a.tags = list(a.tags) + [unreal.Name(TAG)]
            n += 1
    # The lotus: the pond's outline (its bank 0.12 m in) from the C++ plan, relative to the plants' centre.
    try:
        outline = [(p.x, p.y) for p in unreal.ChenghuaiStructure.get_pond_outline()]
    except Exception:  # noqa: BLE001
        outline = []
    cx = sum(p[0][0] for p in LOTUS) / len(LOTUS)
    cy = sum(p[0][1] for p in LOTUS) / len(LOTUS)
    specs = []
    for (x, y), d, f in LOTUS:
        s_ = unreal.MuseeLilySpec()
        N.set_prop(s_, "position", N.v2(x - cx, y - cy))
        N.set_prop(s_, "radius", d / 2)
        N.set_prop(s_, "flower", f > 0)
        N.set_prop(s_, "flower_size", f)
        specs.append(s_)
    props = {"kind": N.enum("MuseeWaterPlant", "LOTUS"), "seed": 730, "plants": specs}
    if outline:
        props["pond_outline"] = [N.v2(x - cx, y - cy) for x, y in outline]
    a = sp.spawn("MuseeWaterLilies", "Lotus", "Chenghuai", N.cm(cx, cy, WATER), props)
    if a:
        a.tags = list(a.tags) + [unreal.Name(TAG)]
        n += 1
    # The pond's banks: clumps of strap-leaved plants between the edge stones (as Suzhou's ponds have iris and sedge at
    # the water): the museum's orchid clumps, their pots sunk out of sight in the bank's earth. Not at the bridge's ends,
    # the water pavilion or the flower hall's terrace.
    if outline:
        ox = sum(p[0] for p in outline) / len(outline)
        oy = sum(p[1] for p in outline) / len(outline)
        keep_off = [((5.2, -44.9), 1.4), ((13.0, -44.5), 1.5), ((10.3, -43.9), 0.9)]
        k = 0
        for i in range(0, len(outline), 4):
            x, y = outline[i]
            dx, dy = x - ox, y - oy
            d = math.hypot(dx, dy) or 1.0
            px, py = x + dx / d * 0.45, y + dy / d * 0.45
            if any(math.hypot(px - a, py - b) < r for (a, b), r in keep_off):
                continue
            if 12.4 < px < 16.9 and -51.2 < py < -45.5:       # the water pavilion
                continue
            if py < -52.9 and 7.3 < px < 13.8:                # the flower hall's terrace
                continue
            tx, ty = -dy / d, dx / d                          # along the bank
            for j, (off, sc) in enumerate(((-0.28, 1.7), (0.3, 2.2))):
                if (i + j) % 5 == 4:
                    continue
                a = sp.spawn("MuseePottedPlant", f"Bank_{k}", "Chenghuai", N.cm(px + tx * off, py + ty * off, -0.262 * sc - 0.01),
                             {"kind": N.enum("MuseePottedPlantKind", "ORCHID"), "seed": 900 + k})
                if a:
                    a.set_actor_scale3d(unreal.Vector(sc, sc, sc))
                    a.set_actor_rotation(unreal.Rotator(0, 0, (k * 67) % 360), False)
                    a.tags = list(a.tags) + [unreal.Name(TAG)]
                    n += 1
                k += 1
    # 石兄: the standing Taihu rock, about 3 m.
    # Its stone the photographed Taihu limestone (MI_Ch_Taihu: weathered, water-stained, moss in its hollows).
    a = sp.spawn("MuseeTaihuRock", "Shixiong", "Chenghuai", N.cm(*ROCK),
                 {"seed": 731, "rock_material": f"{CH_MATS}/MI_Ch_Taihu.MI_Ch_Taihu"})
    if a:
        o, e = a.get_actor_bounds(False)
        k = 300.0 / max(2 * e.z, 1.0)
        a.set_actor_scale3d(unreal.Vector(k, k, k))
        o, e = a.get_actor_bounds(False)
        loc = a.get_actor_location()
        a.set_actor_location(unreal.Vector(loc.x, loc.y, loc.z - (o.z - e.z) - 5.0), False, False)
        a.tags = list(a.tags) + [unreal.Name(TAG), unreal.Name("work:chenghuai-shixiong")]
        n += 1
    # The court lived in (Rules board): the fish bowl on the axis with water in it; the pomegranates in their tubs before
    # the hall's platform.
    try:
        profiles = json.load(open(os.path.join(P.PROJECT_DIR, "SourceArt", "Textures", "ch_vessels.json"), encoding="utf-8"))
        place_vessel(eas, profiles, "fishbowl", None, AXIS, -36.2, 0.0, stand=False, solid=True)
        bowl = mesh_actor(eas, unreal.load_asset("/Engine/BasicShapes/Cylinder"), cm(AXIS, -36.2, 0.58))
        bowl.set_actor_scale3d(unreal.Vector(1.06, 1.06, 0.01))
        bowl.set_actor_label("Fish bowl water")
        bowl.tags = [unreal.Name(TAG), unreal.Name(WING), unreal.Name("musee.nobake")]
        wc = bowl.get_component_by_class(unreal.StaticMeshComponent)
        wc.set_material(0, unreal.load_asset(f"{P.MATERIALS}/MI_Water_Garden"))
        wc.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        for label, x in (("Pomegranate_W", -11.6), ("Pomegranate_E", -6.8)):
            place_vessel(eas, profiles, "tub", None, x, -42.6, 0.0, stand=False, solid=True)
            soil = mesh_actor(eas, unreal.load_asset("/Engine/BasicShapes/Cylinder"), cm(x, -42.6, 0.405))
            soil.set_actor_scale3d(unreal.Vector(0.58, 0.58, 0.01))
            soil.set_actor_label(f"{label} soil")
            soil.tags = [unreal.Name(TAG), unreal.Name(WING)]
            soil.get_component_by_class(unreal.StaticMeshComponent).set_material(0, unreal.load_asset(f"{CH_MATS}/MI_Ch_GardenGround"))
            a = sp.spawn("MuseeTree", label, "Chenghuai", N.cm(x, -42.6, 0.41), {
                "species": N.enum("MuseeTreeSpecies", "ORCHARD_FRUIT"), "fruit_variant": 1, "seed": 740 + int(x), "tree_height": 1.9,
                "crown_height": 1.35, "crown_radii": N.v2(0.75, 0.55)})
            if a:
                a.tags = list(a.tags) + [unreal.Name(TAG)]
                n += 1
    except Exception as ex:  # noqa: BLE001
        warn(f"the court's things: {ex}")
    log(f"{n} plants and the standing rock")


# ---------------------------------------------------------------------------------------------- placards on the building

def placard_volume(eas, wid, centre, half):
    """An invisible box that only the look trace sees (Visibility), carrying work:<id>, for a work that is part of the
    building (the screen wall's carving, the engraved stones, the tables): plan centre (x, y, z) and half-sizes (m)."""
    cube = unreal.load_asset("/Engine/BasicShapes/Cube")
    a = mesh_actor(eas, cube, cm(*centre))
    a.set_actor_label(f"Placard {wid}")
    a.set_actor_scale3d(unreal.Vector(half[0] * 2, half[1] * 2, half[2] * 2))   # the cube is 1 m
    a.tags = [unreal.Name(TAG), unreal.Name(WING), unreal.Name("work:" + wid), unreal.Name("musee.nobake")]
    c = a.get_component_by_class(unreal.StaticMeshComponent)
    c.set_collision_enabled(unreal.CollisionEnabled.QUERY_ONLY)
    c.set_collision_response_to_all_channels(unreal.CollisionResponseType.ECR_IGNORE)
    c.set_collision_response_to_channel(unreal.CollisionChannel.ECC_VISIBILITY, unreal.CollisionResponseType.ECR_BLOCK)
    c.set_cast_shadow(False)
    c.set_visibility(False)
    a.set_actor_hidden_in_game(True)
    return a


def building_placards(eas):
    # The screen wall's carved heart (影壁心) on the entry yard's north wall, facing south.
    placard_volume(eas, "chenghuai-yingbi", (0.0, -27.5, 1.9), (0.9, 0.06, 0.9))
    # The engraved stones: one box along the walk's wall (x 2.3 face) over the six bays that hold them.
    for y0 in (-53.4, -42.6, -39.9, -37.2, -34.5, -29.1):
        placard_volume(eas, "chenghuai-shutiao", (2.34, y0 + 1.35, 1.33), (0.04, 1.0, 0.2))
    placard_volume(eas, "chenghuai-altar-table", (AXIS, -49.75, HALL_F + 0.5), (1.0, 0.21, 0.4))
    placard_volume(eas, "chenghuai-scholars-desk", (-3.5, -19.27, FRONT_F + 0.5), (0.75, 0.38, 0.4))
    placard_volume(eas, "chenghuai-painting-table", (10.65, -57.6, 0.45 + 0.5), (0.95, 0.4, 0.4))
    # The scholar's rock on its stand, on the painting table's east end.
    try:
        import nature as N
        rock = N.Spawner(True).spawn("MuseeTaihuRock", "Lingbi_rock", "Chenghuai", N.cm(11.35, -57.45, 0.45 + 0.8 + 0.03), {"seed": 732})
        if rock:
            o, e = rock.get_actor_bounds(False)
            k = 32.0 / max(2 * e.z, 1.0)
            rock.set_actor_scale3d(unreal.Vector(k, k, k))
            o, e = rock.get_actor_bounds(False)
            loc = rock.get_actor_location()
            rock.set_actor_location(unreal.Vector(loc.x, loc.y, loc.z - (o.z - e.z) + (0.45 + 0.8 + 0.03) * 100), False, False)
            rock.tags = list(rock.tags) + [unreal.Name(TAG), unreal.Name("work:chenghuai-lingbi-rock")]
            st = spawn(eas, unreal.load_class(None, VESSEL), cm(11.35, -57.45, 0.45 + 0.8), unreal.Rotator(0, 0, 0), "Stand Lingbi", "chenghuai-lingbi-rock")
            st.set_editor_property("stand_radius", 0.11)
            st.set_editor_property("stand_height", 0.03)
            st.set_editor_property("stand_material", unreal.load_asset(f"{CH_MATS}/MI_Ch_Nanmu"))
            st.rebuild()
    except Exception as ex:  # noqa: BLE001
        warn(f"Lingbi rock: {ex}")
    log("placards on the building")


# ---------------------------------------------------------------------------------------------- the rooms

def clear(eas):
    n = 0
    for a in eas.get_all_level_actors():
        if TAG in [str(t) for t in a.tags]:
            eas.destroy_actor(a)
            n += 1
    return n


def main(eas):
    removed = clear(eas)
    cls = unreal.load_class(None, SCROLL)
    if not cls or not unreal.load_class(None, VESSEL):
        warn("AChenghuaiScroll / AChenghuaiVessel missing: build the editor module first")
        return
    for d in (MATS, TEXS):
        if not EAL.does_directory_exist(d):
            EAL.make_directory(d)
    M = masters()
    mats = mount_materials()
    n = 0
    # 澄懷堂: the three hang from the rails (canopy − 5 cm) by their cords, 2 cm before the silk.
    rail_z = HALL_F + 3.56 - 0.05
    y = HALL_BACK + 0.05 + 0.003
    for wid, (x0, x1) in zip(("chinese-guo-xi-early-spring", "chinese-fan-kuan-travelers", "chinese-li-tang-wind-in-pines"), HALL_CASES):
        a, length = hang(eas, cls, M, mats, wid, (0.5 * (x0 + x1), y, rail_z - 0.085), (0.0, 1.0))
        n += 1
        if rail_z - 0.085 - length < HALL_F + 0.21:
            warn(f"{wid}: its roller reaches the deck ({rail_z - 0.085 - length:.3f})")
    # The ear rooms.
    ear_rail = EAR_F + 3.08 - 0.05
    for side in (0, 1):
        s = -1.0 if side else 1.0
        x0 = 2 * AXIS - EAR_X1 if side else EAR_X0
        x1 = 2 * AXIS - EAR_X0 if side else EAR_X1
        back, gable = (("chinese-shen-zhou-lushan", "chinese-wen-zhengming-living-aloft") if side else
                       ("chinese-ni-zan-rongxi", "chinese-wang-meng-qingbian"))
        hang(eas, cls, M, mats, back, (0.5 * (x0 + x1), EAR_N + 0.05 + 0.003, ear_rail - 0.085), (0.0, 1.0))
        wall_x = x1 if side else x0
        hang(eas, cls, M, mats, gable, (wall_x + s * (0.05 + 0.003), 0.5 * sum(EAR_SIDE_Y), ear_rail - 0.085), (s, 0.0))
        n += 2
    # 臨池: looking south, the reader's right is west; the five begin at the west end.
    bed = bed_line(LINCHI["front"], LINCHI["depth"], LINCHI["toward"], FRONT_F, 0.3)
    handscroll_row(eas, cls, M, mats, ["chinese-orchid-pavilion", "chinese-yan-zhenqing-nephew", "chinese-huaisu-autobiography",
                                       "chinese-su-shi-cold-food", "chinese-mi-fu-shu-su"],
                   LINCHI["x0"] + 0.075, LINCHI["x1"] - 0.075, bed, 0.06, 0.0, 0.025, reader_right_is_west=True)
    n += 5
    # 舒卷: looking north, the reader's right is east; each begins at its case's east end.
    bed = bed_line(REAR["front"], REAR["depth"], REAR["toward"], REAR_F, 0.5)
    handscroll_row(eas, cls, M, mats, ["chinese-thousand-li"], THOUSAND_X[1] - 0.075, THOUSAND_X[0] + 0.075, bed, 0.45, 0.22, 0.035,
                   reader_right_is_west=False)
    handscroll_row(eas, cls, M, mats, ["chinese-qingming"], QINGMING_X[1] - 0.075, QINGMING_X[0] + 0.075, bed, 0.25, 0.22, 0.03,
                   reader_right_is_west=False)
    n += 2
    # The ceramics. 天青 (west): north bay the Northern Song (Ding, Jun), the horse in the middle, the Southern Song south
    # (Guan, Jian, Longquan); the Ru basin alone on its pedestal. 昌南 (east), Yuan to Qing from north to south.
    profiles = {}
    pj = os.path.join(P.PROJECT_DIR, "SourceArt", "Textures", "ch_vessels.json")
    if os.path.exists(pj):
        with open(pj, encoding="utf-8") as f:
            profiles = json.load(f)
    deck = SIDE_F + 0.82 + 0.012
    wx = SIDE_BACK_W + SIDE_CASE_D - 0.3
    ex = 2 * AXIS - wx
    west = [("vessel", "ding", "chinese-ding-basin", -39.6), ("vessel", "jun", "chinese-jun-flowerpot", -38.6),
            ("scan", "chinese-sancai-horse", None, -36.2),
            ("vessel", "guan", "chinese-guan-vase", -34.0), ("vessel", "jian", "chinese-jian-bowl", -33.2),
            ("vessel", "longquan", "chinese-longquan-vase", -32.5)]
    east = [("scan", "chinese-meiping", 0.06, -39.8), ("vessel", "xuande", "chinese-xuande-jar", -38.7),
            ("vessel", "wucai", "chinese-wucai-jar", -36.9), ("scan", "chinese-chicken-cup", 0.03, -35.5),
            ("scan", "chinese-peachbloom", 0.045, -33.9), ("vessel", "falangcai", "chinese-falangcai-bowl", -32.8)]
    for x, items in ((wx, west), (ex, east)):
        for kind, a1, a2, yy in items:
            if kind == "vessel":
                if a1 in profiles:
                    place_vessel(eas, profiles, a1, a2, x, yy, deck)
                    n += 1
                else:
                    warn(f"{a1}: no profile (run chenghuai_vessels.py)")
            else:
                place_scanned(eas, a1, x, yy, deck, 90.0 if a1 == "chinese-sancai-horse" else 0.0, a2)
                n += 1
    # The Ru basin on its pedestal (ChenghuaiDisplay.cpp PedestalCase: the deck F + 0.95 + 0.012).
    place_scanned(eas, "chinese-ru-basin", LONE[0], LONE[1], SIDE_F + 0.95 + 0.012, 90.0, 0.0)
    # The handling copy of the chicken cup on the east hall's table.
    place_scanned(eas, "chinese-chicken-cup", 2 * AXIS - LONE[0] - 0.08, LONE[1] + 0.12, SIDE_F + 0.8, 30.0, 0.0,
                  label="Chicken cup (handling copy)")
    n += 2
    log(f"{n} works placed ({removed} removed)")
    building_placards(eas)
    plants(eas)
