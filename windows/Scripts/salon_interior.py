"""
The Salon's interior update (plan proposals/salon-interior; the Main, Plan, Light, Oval and Details boards): its materials,
its textures (the Orangerie's willows, the 1874 documents) and its pieces in the map.

    UnrealEditor-Cmd.exe MuseeVision.uproject -run=pythonscript -script="<repo>/windows/Scripts/apply_all.py salon native"

- `make()` (apply_all step "salon"): the materials, in /Game/Museum/Materials/Salon:
  - MPC_Salon: the sky over the Salon (ASalonSky writes Cloud, Rain, the cloud's drift, the light on the roof and its sun share);
  - MI_Salon_Silk_1…5: the bays' silks one step deeper (F1 slubbed silk under M_Fabric's fuzz), on the walls and the piers;
  - MI_Salon_Velvet_2…5: the benches' velvet in each bay's colour (a deep pile: fuzz 0.9);
  - M_Salon_Parquet / MI_Salon_Parquet: oak Versailles parquet on the 1.2 m grid (W2, the museum's oak veneer), every strip
    its own piece of oak: its grain, its slice of the veneer, its tone; oiled and waxed, the joints in the specular, the
    wax worn along the line;
  - MI_Salon_Voussoir: the arches' stones (honed travertine, one slice per stone on UV0, jointless: the joints are geometry);
  - M_Salon_SkyVeil: the cloud over each lantern (and its shadow); M_Salon_Glass: the lanterns' panes, rain on them;
  - M_Salon_Diffuser: bay 2's opal glass and bay 4's muslin, glowing with what they pass on; M_Salon_Velarium: the oval's
    velarium under Giverny's sky, cloud shadows crossing it, the rain's drops on the glass above it;
  - M_Salon_Canvas: the willows (unvarnished oil: matt, the impasto catching the light); M_Salon_Paper: the documents;
  - MI_Salon_VineLeaf, MI_Salon_CaseFelt, MI_Salon_PondWater (the pond's water dyed black);
  - the pond's edge (the pond-edge redesign): M_Salon_RimLight / MI_Salon_RimLight (the line of light in the lip, 2700 K),
    and the planting's materials in /Game/Museum/Nature (M_NaturePetal / M_NatureLeaf instances): MI_Salon_IrisLeaf,
    MI_Salon_IrisPetal, MI_Salon_Sedge, MI_Salon_ForgetMeNot.
- `place(eas)` (called by native.py after it has placed the rooms): the 1874 case, the four willow canvases, the pond's
  edge (ASalonPond) and its planting (ASalonPondPlants), attached to the lifting pond; the pieces they replace retired;
  the velarium's old light removed.

Sources and credits: assets/salon/ (CREDITS.md). The Salon's own lights are gallery_lights.py's (its Salon section).
"""
import json
import math
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import musee_paths as P  # noqa: E402

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

FOLDER = P.MATERIALS + "/Salon"
TEXTURES = P.MUSEUM_CONTENT + "/Textures/Salon"
ASSETS = os.path.join(P.REPO_DIR, "assets", "salon")
TAG = "musee.salon_interior"

# The silks, one step deeper (the Details board), bays 1–5; the benches' velvet takes the same colour.
SILKS = {1: 0x6A2A22, 2: 0x47515A, 3: 0x5A6D54, 4: 0x8A5A54, 5: 0xA0793A}

# The Orangerie's second room (the Oval board): true sizes, 2 m tall; canvases 4.25 m wide butted edge to edge. Round the
# oval from the door's north jamb: Clear Morning with Willows, The Two Willows (ahead, across the axis), Morning with
# Willows, Reflections of Trees (by the door, south), with equal joints between (about 0.9 m).
WILLOWS = [
    ("monet-willows-clear-morning", "Clear Morning with Willows", 12.75, 3),
    ("monet-willows-two-willows", "The Two Willows", 17.0, 4),
    ("monet-willows-morning", "Morning with Willows", 12.75, 3),
    ("monet-willows-reflections-of-trees", "Reflections of Trees", 8.5, 2),
]
# The imported pieces the interior update replaces: the first room's four panels, the pale basin.
RETIRE = ["/Museum/Salon/morning", "/Museum/Salon/clouds", "/Museum/Salon/green_reflections", "/Museum/Salon/setting_sun",
          "/Museum/Reserve/Lily_pond/Pond_basin"]


def log(msg):
    unreal.log(f"[salon] {msg}")


def warn(msg):
    unreal.log_warning(f"[salon] {msg}")


def materials_module():
    import materials as M   # the museum's material library (Graph, the masters' helpers, the language's specs)
    return M


def lin(h):
    return materials_module().lin(h)


# --------------------------------------------------------------------------- small helpers

def ensure_folder(path):
    if not EAL.does_directory_exist(path):
        EAL.make_directory(path)


def master(name, shading_model, blend=unreal.BlendMode.BLEND_OPAQUE, two_sided=False, attributes=True):
    """A master in FOLDER, emptied and set up (kept if it exists, so its instances stay valid)."""
    M = materials_module()
    ensure_folder(FOLDER)
    path = f"{FOLDER}/{name}"
    if EAL.does_asset_exist(path):
        m = unreal.load_asset(path)
        M.clear_graph(m)
    else:
        m = TOOLS.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
        for usage in ("MATUSAGE_NANITE", "MATUSAGE_STATIC_MESH"):
            u = M.enum_value(getattr(unreal, "MaterialUsage", None), usage)
            if u is not None:
                try:
                    MEL.set_base_material_usage(m, u, True)
                except Exception as e:  # noqa: BLE001
                    warn(f"{name}: usage {usage} ({e})")
    props = [("blend_mode", blend), ("two_sided", two_sided), ("shading_model", shading_model),
             ("use_material_attributes", attributes)]
    if blend == unreal.BlendMode.BLEND_OPAQUE:     # geometric specular antialiasing (materials.master's note)
        props.append(("normal_curvature_to_roughness", True))
    for prop, value in props:
        try:
            if m.get_editor_property(prop) != value:
                m.set_editor_property(prop, value)
        except Exception as e:  # noqa: BLE001
            warn(f"{name}: {prop} ({e})")
    return m


def custom(g, code, outputs, inputs, extra=(), description="Custom"):
    """A Custom expression (Salon/SalonEditorLibrary.h fills its inputs and extra outputs). inputs: [(name, source)];
    extra: [(name, components)]. Returns the node; its extra outputs are (node, name)."""
    e = g.node(unreal.MaterialExpressionCustom)
    ok = unreal.SalonEditorLibrary.configure_custom(e, code, outputs, [n for n, _ in inputs], [n for n, _ in extra],
                                                    [c for _, c in extra], description)
    if not ok:
        raise RuntimeError(f"{g.name}: the Custom expression '{description}' could not be set up (is the C++ built?)")
    for name, src in inputs:
        g.link(src, e, name)
    return e


def mpc_param(g, name):
    e = g.node(unreal.MaterialExpressionCollectionParameter)
    e.set_editor_property("collection", unreal.load_asset(f"{FOLDER}/MPC_Salon"))
    e.set_editor_property("parameter_name", name)
    return e


def musee_param(g, name):
    e = g.node(unreal.MaterialExpressionCollectionParameter)
    e.set_editor_property("collection", unreal.load_asset(f"{P.MATERIALS}/MPC_Musee"))
    e.set_editor_property("parameter_name", name)
    return e


def finish(m):
    M = materials_module()
    try:
        MEL.layout_material_expressions(m)
    except Exception:  # noqa: BLE001
        pass
    errors = list(MEL.recompile_material(m) or [])
    for e in errors:
        warn(f"{m.get_name()}: compile error: {e}")
    EAL.save_loaded_asset(m, only_if_is_dirty=False)
    log(f"{m.get_name()}: {MEL.get_num_material_expressions(m)} expressions, {len(errors)} errors")
    M.PARAMS[m.get_name()] = M.parameter_names(m)
    return m


def instance(name, parent, scalars=None, vectors=None, textures=None, switches=None, two_sided=False):
    """An instance in FOLDER (updated in place). textures: {parameter: Texture2D object}."""
    M = materials_module()
    s = {"parent": parent.get_name(), "scalars": dict(scalars or {}), "vectors": dict(vectors or {}),
         "textures": {}, "switches": dict(switches or {}), "two_sided": two_sided}
    mi = M.material_instance(name, FOLDER, parent, s, {})
    if mi is not None and textures:
        for k, tex in textures.items():
            MEL.set_material_instance_texture_parameter_value(mi, k, tex)
        MEL.update_material_instance(mi)
        for k, tex in textures.items():
            got = MEL.get_material_instance_texture_parameter_value(mi, k)
            if got is None or got.get_path_name() != tex.get_path_name():
                warn(f"{name}: texture {k} not set")
        EAL.save_loaded_asset(mi, only_if_is_dirty=False)
    return mi


def instance_spec(name, parent, spec):
    M = materials_module()
    return M.material_instance(name, FOLDER, parent, spec, {})


def texture(png, name, srgb=True, normal=False, max_size=0):
    """T_<name> in TEXTURES from a PNG/JPEG (re-imported when the file is newer than the asset)."""
    ensure_folder(TEXTURES)
    path = f"{TEXTURES}/{name}"
    pkg = os.path.join(P.PROJECT_DIR, "Content", path[len("/Game/"):] + ".uasset")
    if not os.path.exists(png):
        warn(f"{png} missing")
        return unreal.load_asset(path) if EAL.does_asset_exist(path) else None
    if not EAL.does_asset_exist(path) or not os.path.exists(pkg) or os.path.getmtime(png) > os.path.getmtime(pkg):
        task = unreal.AssetImportTask()
        task.filename = png
        task.destination_path = TEXTURES
        task.destination_name = name
        task.automated = True
        task.replace_existing = True
        task.save = False
        TOOLS.import_asset_tasks([task])
    tex = unreal.load_asset(path) if EAL.does_asset_exist(path) else None
    if tex is None:
        warn(f"{path} not imported")
        return None
    T = unreal.TextureCompressionSettings
    props = {"compression_settings": T.TC_NORMALMAP if normal else T.TC_DEFAULT, "srgb": srgb and not normal}
    if max_size:
        props["max_texture_size"] = max_size
    for k, v in props.items():
        try:
            if tex.get_editor_property(k) != v:
                tex.set_editor_property(k, v)
        except Exception as e:  # noqa: BLE001
            warn(f"{name}.{k}: {e}")
    EAL.save_loaded_asset(tex, only_if_is_dirty=False)
    return tex


# --------------------------------------------------------------------------- the sky's parameters

def make_mpc():
    ensure_folder(FOLDER)
    path = f"{FOLDER}/MPC_Salon"
    mpc = unreal.load_asset(path) if EAL.does_asset_exist(path) else \
        TOOLS.create_asset("MPC_Salon", FOLDER, unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
    params = []
    for n, v in [("Cloud", 0.25), ("Rain", 0.0), ("CloudDriftX", 4.0), ("CloudDriftY", -3.0), ("RoofLux", 60000.0), ("SunShare", 0.6)]:
        p = unreal.CollectionScalarParameter()
        p.set_editor_property("parameter_name", n)
        p.set_editor_property("default_value", v)
        params.append(p)
    mpc.set_editor_property("scalar_parameters", params)
    EAL.save_loaded_asset(mpc)
    return mpc


# --------------------------------------------------------------------------- silk, velvet, felt, stone

def make_fabrics():
    M = materials_module()
    fabric = unreal.load_asset(f"{P.MATERIALS}/M_Fabric")
    made = []
    for bay, hexc in SILKS.items():
        colour = lin(hexc)
        silk = M.with_photo(M.spec("M_Fabric", {"Roughness": 0.62, "SheenAmount": 0.25, "SlubAmount": 0.07, "WeaveAmount": 0.1,
                                                "MottleAmount": 0.04, "Specular": 0.35},
                                   {"BaseColor": colour, "SheenTint": M.cmix(colour, (0.9, 0.88, 0.84), 0.4)}),
                            "F1", dict(colour_amount=0.35, contrast=0.6, normal=0.6, rough_influence=0.4, scale=0.3),
                            {"PBRDetailPeriod": 0.002}, switches={"PBRDetail": True})     # the weave fades before it beats (moiré)
        instance_spec(f"MI_Salon_Silk_{bay}", fabric, silk)
        made.append(f"MI_Salon_Silk_{bay}")
        if bay >= 2:
            # Velvet: a cut pile, its sheen the dyed colour paler (fuzz 0.9); crushed here and there (the mottle); the pile
            # absorbs more than the silk's flat weave, so the same dye reads a shade deeper.
            # (The texture audit: from across a bay the benches read as plain blocks. A real cut pile is dark face-on and
            # bright at grazing, so each rounded edge, piping and quilted dome carries a pale rim: the dyed pile deeper,
            # its fibres' sheen paler and denser (fuzz 1.0), tighter (Roughness 0.55), crushed in 6 cm patches.)
            pile = M.cscale(colour, 0.42)
            velvet = M.with_photo(M.spec("M_Fabric", {"Roughness": 0.55, "SheenAmount": 0.5, "SlubAmount": 0.0, "WeaveAmount": 0.0,
                                                      "MottleAmount": 0.14, "MottleSize": 0.06, "Specular": 0.3},
                                         {"BaseColor": pile, "SheenTint": M.cmix(colour, (0.96, 0.93, 0.9), 0.3)}),
                                  "F2", dict(colour_amount=0.3, contrast=0.6, normal=0.35, rough_influence=0.3, scale=0.25))
            instance_spec(f"MI_Salon_Velvet_{bay}", fabric, velvet)
            made.append(f"MI_Salon_Velvet_{bay}")
    # The case's lining: unbleached linen over felt.
    linen = lin(0xA3957C)
    felt = M.with_photo(M.spec("M_Fabric", {"Roughness": 0.9, "SheenAmount": 0.3, "SlubAmount": 0.05, "WeaveAmount": 0.08,
                                            "MottleAmount": 0.04, "Specular": 0.3},
                               {"BaseColor": linen, "SheenTint": M.cmix(linen, (0.95, 0.93, 0.9), 0.4)}),
                        "F1", dict(colour_amount=0.5, contrast=0.7, normal=0.5, rough_influence=0.4, scale=0.3),
                        {"PBRDetailPeriod": 0.002}, switches={"PBRDetail": True})
    instance_spec("MI_Salon_CaseFelt", fabric, felt)
    made.append("MI_Salon_CaseFelt")
    # The voussoirs, the keystones and the pond's kerb: honed travertine without drawn joints (each stone is geometry),
    # each stone its own slice of the photograph on its UV0 (metres).
    stone = unreal.load_asset(f"{P.MATERIALS}/M_Stone")
    solid = {k: (dict(v) if isinstance(v, dict) else v) for k, v in M.NAMED["M_Travertine_Solid"].items()}
    solid["switches"].update({"PBRUseUV0": True})
    solid["scalars"].update({"PBRBlockShift": 0.0, "Roughness": 0.55, "SlabTilt": 0.0, "BlockTone": 0.0, "FootDirt": 0.0,
                             "DustUp": 0.05})
    M.PARAMS["M_Stone"] = M.parameter_names(stone)
    instance_spec("MI_Salon_Voussoir", stone, solid)
    made.append("MI_Salon_Voussoir")
    # The vault in half-light: the coffers' plaster a shade down from the Rotunda's (its light is the lanterns' and the
    # coves'), dustier on what faces up.
    plaster = unreal.load_asset(f"{P.MATERIALS}/M_Plaster")
    M.PARAMS["M_Plaster"] = M.parameter_names(plaster)
    vault = {k: (dict(v) if isinstance(v, dict) else v) for k, v in M.NAMED["M_Plaster_Coffer"].items()}
    vault["vectors"]["BaseColor"] = lin(0xDCD2C1)
    # (The texture audit: in its half-light the vault read as clean CG, every coffer step the same even white. Real
    # lime in shade shows its trowel and its grain, and dust greys every ledge that faces up.)
    vault["scalars"].update({"DustUp": 0.1, "Variation": 0.06, "PBRColorAmount": 0.8, "PBRContrast": 0.8,
                             "NormalStrength": 0.6, "Waviness": 0.8})
    instance_spec("MI_Salon_Vault", plaster, vault)
    made.append("MI_Salon_Vault")
    log("fabrics and stone: " + ", ".join(made))


# --------------------------------------------------------------------------- the arches' stones

def make_ashlar():
    """M_Salon_Ashlar / MI_Salon_Ashlar: the transverse arches' voussoirs and keystones: honed travertine (S1), each stone
    its own slice of the photograph (UV0, metres) and its own tone (vertex colour R, about ±11 %) and polish (G); on the
    joints' V-chamfers (B) the dirt and shadow of a real joint; the travertine's voids open (dark) as on the walls."""
    import pbr
    M = materials_module()
    m = master("M_Salon_Ashlar", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    g = M.Graph(m)
    tex = pbr.textures("S1")
    uv = g.div(g.node(unreal.MaterialExpressionTextureCoordinate), g.scalar("PBRScale", 1.2, "Photo", 0.3, 5))
    col = pbr.sample(g, "PBRColor", tex.get("Color"), uv, "SAMPLERTYPE_COLOR")
    nrm = pbr.sample(g, "PBRNormal", tex.get("Normal"), uv, "SAMPLERTYPE_NORMAL")
    orm = pbr.sample(g, "PBRORM", tex.get("ORM"), uv, "SAMPLERTYPE_LINEAR_COLOR")
    vc = g.node(unreal.MaterialExpressionVertexColor)
    st = pbr.stats()["S1"]
    mean = g.vector("PBRMean", tuple(st["color_mean"]), "Photo")
    photo = g.div(g.lerp(mean, (col, "RGB"), g.scalar("PBRContrast", 0.7, "Photo", 0, 2)), mean)
    tone = g.add(1.0, g.mul(g.sub(g.mask(vc, "R"), 0.5), g.scalar("StoneTone", 0.22, "Stones", 0, 0.5)))
    joint = g.mask(vc, "B")
    void = g.sat(g.mul(g.one_minus((orm, "R")), 1.43))
    c = g.mul(g.mul(g.vector("BaseColor", lin(0xE1D6C4), "Colour"), photo), tone)
    c = g.lerp(c, g.mul(c, 0.55), g.mul(void, g.scalar("VoidDark", 0.8, "Stones", 0, 1)))   # (the pond's frieze: filled voids)
    c = g.lerp(c, g.mul(c, g.scalar("JointDarkness", 0.62, "Stones", 0, 1)), joint)
    rough = g.sat(g.add(g.add(g.scalar("Roughness", 0.58, "Surface", 0, 1),
                              g.mul(g.sub((orm, "G"), st["rough_mean"]), 0.5)),
                        g.add(g.mul(g.sub(g.mask(vc, "G"), 0.5), 0.1), g.mul(joint, 0.2))))
    n = g.unary(unreal.MaterialExpressionNormalize,
                g.append(g.mul(g.mask(nrm, "RG"), g.scalar("NormalStrength", 0.9, "Photo", 0, 3)), g.mask(nrm, "B")))
    ao = g.mul(g.lerp(1.0, (orm, "R"), 0.6), g.lerp(1.0, 0.45, joint))
    g.output(BaseColor=c, Metallic=0.0, Specular=0.6, Roughness=rough, Normal=n, AmbientOcclusion=ao)
    finish(m)
    instance("MI_Salon_Ashlar", m)


# --------------------------------------------------------------------------- the parquet

PARQUET_HLSL = r"""
// Versailles parquet (Salon interior update). P: world metres (x east, y south). Panels 1.2 m square on the room's grid, a
// mitred frame, an interlaced diagonal lattice (strips crossing over and under in turn), diamond infills; border boards
// round the field. Returns (along, across, distance to the piece's nearest joint, the piece's random); GrainJoint = (grain
// direction xy, the direction into the piece from its nearest joint xy); Extra = (random 2, random 3, kind, the panel's random).
struct FParquet { float H(float3 v) { return frac(sin(dot(v, float3(12.9898, 78.233, 37.719))) * 43758.5453); } };
FParquet f;
float2 p = P.xy;
const float S2 = 0.70710678;
const float2 ea = float2(S2, S2);
const float2 eb = float2(S2, -S2);
bool cab = p.y < -7.6;
float2 anchor = cab ? float2(-20.0, -11.6) : float2(-26.6, -0.6);
float4 fld = cab ? float4(-23.6, -15.2, -16.4, -8.0) : float4(-73.4, -6.6, -14.6, 6.6);
float dist = 1.0;
float2 jd = float2(0.0, 1.0);
float2 g = float2(1.0, 0.0);
float3 id = float3(0.0, 0.0, 0.0);
float kind = 0.0;
bool inField = p.x >= fld.x && p.x <= fld.z && p.y >= fld.y && p.y <= fld.w;
if (!inField)
{
    bool sides = p.y < fld.y || p.y > fld.w;
    float acr = sides ? (p.y < fld.y ? fld.y - p.y : p.y - fld.w) : (p.x < fld.x ? fld.x - p.x : p.x - fld.z);
    float alg = sides ? p.x : p.y;
    g = sides ? float2(1.0, 0.0) : float2(0.0, 1.0);
    float2 gn = sides ? float2(0.0, p.y < fld.y ? -1.0 : 1.0) : float2(p.x < fld.x ? -1.0 : 1.0, 0.0);
    float row = floor(acr / Board);
    float fr = acr / Board - row;
    float len = 1.2 + 1.2 * f.H(float3(row, sides ? 3.0 : 7.0, cab ? 1.0 : 0.0));
    float off = f.H(float3(row, 5.0, 9.0)) * len;
    float seg = floor((alg + off) / len);
    float fs = (alg + off) / len - seg;
    float d1 = min(fr, 1.0 - fr) * Board;
    float d2 = min(fs, 1.0 - fs) * len;
    dist = min(d1, d2);
    jd = d1 < d2 ? gn * (fr < 0.5 ? 1.0 : -1.0) : g * (fs < 0.5 ? 1.0 : -1.0);
    id = float3(row + 100.0, seg, sides ? 1.0 : 2.0);
    kind = 3.0;
}
else
{
    float2 q = p - anchor;
    float2 cell = floor(q / 1.2);
    float2 c = q - (cell + 0.5) * 1.2;
    float2 ac = abs(c);
    float m = max(ac.x, ac.y);
    float inner = 0.6 - FrameW;
    float2 sg = float2(c.x < 0.0 ? -1.0 : 1.0, c.y < 0.0 ? -1.0 : 1.0);
    if (m > inner)
    {
        bool ns = ac.y >= ac.x;
        g = ns ? float2(1.0, 0.0) : float2(0.0, 1.0);
        float2 axis = ns ? float2(0.0, sg.y) : float2(sg.x, 0.0);
        float dOut = 0.6 - m;
        float dIn = m - inner;
        float dMit = abs(ac.y - ac.x) * S2;
        dist = dOut;
        jd = -axis;
        if (dIn < dist) { dist = dIn; jd = axis; }
        float2 mit = ns ? float2(-sg.x, sg.y) * S2 : float2(sg.x, -sg.y) * S2;
        if (dMit < dist) { dist = dMit; jd = mit; }
        id = float3(cell, ns ? sg.y + 2.0 : sg.x + 6.0);
        kind = 0.0;
    }
    else
    {
        float a = dot(c, ea);
        float b = dot(c, eb);
        float D = inner * 1.41421356 / Lattice;
        float fa = a / D;
        float fb = b / D;
        float ia = round(fa);
        float ib = round(fb);
        float da = abs(fa - ia) * D;
        float db = abs(fb - ib) * D;
        float hw = StripW * 0.5;
        bool inA = da < hw;
        bool inB = db < hw;
        float dFrame = inner - m;
        float2 jFrame = ac.x > ac.y ? float2(-sg.x, 0.0) : float2(0.0, -sg.y);
        float s = abs(ia + ib);
        bool aTop = (s - 2.0 * floor(s * 0.5)) < 0.5;
        float sa = (fa - ia) < 0.0 ? -1.0 : 1.0;
        float sb = (fb - ib) < 0.0 ? -1.0 : 1.0;
        if (inA && (!inB || aTop))
        {
            g = eb;
            dist = hw - da;
            jd = -sa * ea;
            if (!inB && !aTop) { float d = db - hw; if (d < dist) { dist = d; jd = sb * eb; } }
            id = float3(cell.x * 7.0 + ia, cell.y * 7.0 + floor((fb - ia - 1.0) * 0.5), 11.0);
            kind = 1.0;
        }
        else if (inB)
        {
            g = ea;
            dist = hw - db;
            jd = -sb * eb;
            if (!inA && aTop) { float d = da - hw; if (d < dist) { dist = d; jd = sa * ea; } }
            id = float3(cell.x * 7.0 + ib, cell.y * 7.0 + floor((fa - ib) * 0.5), 13.0);
            kind = 1.0;
        }
        else
        {
            float2 ic = float2(floor(fa), floor(fb));
            float t = abs(ic.x + ic.y);
            g = (t - 2.0 * floor(t * 0.5)) < 0.5 ? ea : eb;
            dist = da - hw;
            jd = sa * ea;
            if (db - hw < dist) { dist = db - hw; jd = sb * eb; }
            id = float3(cell.x * 7.0 + ic.x, cell.y * 7.0 + ic.y, 17.0);
            kind = 2.0;
        }
        if (dFrame < dist) { dist = dFrame; jd = jFrame; }
    }
}
float r1 = f.H(id + float3(0.13, 0.71, 0.37));
float r2 = f.H(id * 1.7 + float3(3.1, 1.3, 0.2));
float r3 = f.H(id * 2.3 + float3(0.7, 5.1, 2.9));
float2 gp = float2(-g.y, g.x);
GrainJoint = float4(g, jd);
Extra = float4(r2, r3, kind, f.H(float3(floor((p - anchor) / 1.2), 29.0)));
return float4(dot(p, g) + r1 * 1.83, dot(p, gp) + r2 * 1.83, dist, r1);
"""


def make_parquet():
    """
    M_Salon_Parquet: oak Versailles parquet (the Details board). Each piece (the frame's four members, each lattice strip
    between its crossings, each infill, each border board) is its own piece of oak: its grain direction, its slice of the
    veneer photograph (W2, oak_veneer_01, 1.83 m), its tone (±12 %) and a breath of hue, and a slight tilt of its own
    (±0.15°: the reflections kink at the joints). Oiled and waxed: F0 0.035, a thin wax coat, the open pores duller;
    the joints are hairline, dark with old wax and dirt, their arrises eased 0.8 mm, no wax in them and no sky (AO), and they
    stay legible with distance (0.6 of their strength). Along the line the wax is worn thinner (duller, a shade paler).
    """
    import pbr
    M = materials_module()
    m = master("M_Salon_Parquet", unreal.MaterialShadingModel.MSM_CLEAR_COAT)
    g = M.Graph(m)
    p = M.world_metres(g)
    main = custom(g, PARQUET_HLSL, 4, [("P", p), ("Board", g.scalar("BoardWidth", 0.10, "Parquet", 0.05, 0.3)),
                                        ("FrameW", g.scalar("FrameWidth", 0.085, "Parquet", 0.03, 0.2)),
                                        ("StripW", g.scalar("StripWidth", 0.07, "Parquet", 0.02, 0.2)),
                                        ("Lattice", g.scalar("Lattice", 3.0, "Parquet", 1, 6))],
                  [("GrainJoint", 4), ("Extra", 4)], "Versailles parquet")
    along, across, dist, r1 = g.mask(main, "R"), g.mask(main, "G"), g.mask(main, "B"), g.mask(main, "A")
    gj, ex = (main, "GrainJoint"), (main, "Extra")
    grain2 = g.mask(gj, "RG")
    joint_dir = g.mask(gj, "BA")
    r2, r3, panel = g.mask(ex, "R"), g.mask(ex, "G"), g.mask(ex, "A")
    zero = g.const(0.0)
    e_along = g.append(grain2, zero)
    e_across = g.append(g.append(g.mul(g.mask(gj, "G"), -1.0), g.mask(gj, "R")), zero)

    # The veneer: grain up its image, so the image's v runs along the piece.
    tex = pbr.textures("W2")
    scale = g.scalar("PBRScale", 1.83, "Photo", 0.3, 5)
    uv = g.div(g.append(across, g.mul(along, -1.0)), scale)
    col = pbr.sample(g, "PBRColor", tex.get("Color"), uv, "SAMPLERTYPE_COLOR")
    nrm = pbr.sample(g, "PBRNormal", tex.get("Normal"), uv, "SAMPLERTYPE_NORMAL")
    orm = pbr.sample(g, "PBRORM", tex.get("ORM"), uv, "SAMPLERTYPE_LINEAR_COLOR")
    mean = g.vector("PBRMean", tuple(pbr.stats()["W2"]["color_mean"]), "Photo")
    photo = g.div(g.lerp(mean, (col, "RGB"), g.scalar("PBRContrast", 1.25, "Photo", 0, 2)), mean)

    # Tone: each piece ±BoardTone, its panel ±PanelTone (laid from one batch), a breath of hue.
    tone = g.add(g.add(1.0, g.mul(g.sub(r1, 0.5), g.mul(g.scalar("BoardTone", 0.18, "Pieces", 0, 0.4), 2.0))),
                 g.mul(g.sub(panel, 0.5), g.mul(g.scalar("PanelTone", 0.04, "Pieces", 0, 0.2), 2.0)))
    hue = g.mul(g.mul(g.sub(r3, 0.5), 2.0), g.scalar("BoardHue", 0.03, "Pieces", 0, 0.2))
    hue_tint = g.append(g.append(g.add(1.0, hue), 1.0), g.sub(1.0, g.mul(hue, 1.5)))
    c = g.mul(g.mul(g.mul(g.vector("BaseColor", lin(0x6A4527), "Colour"), photo), tone), hue_tint)
    # Old wax and dirt darken a piece towards its joints (a few millimetres), as a century of polishing leaves them.
    c = g.mul(c, g.lerp(g.scalar("EdgeDarkness", 0.8, "Joints", 0, 1), 1.0, g.smoothstep(0.0, 0.006, dist)))

    # The joints: a hairline widened to a pixel and a half with distance, never fading below JointMinVisible.
    pixel = g.mul(g.node(unreal.MaterialExpressionPixelDepth), 0.01 * 0.0008)
    width = g.scalar("JointWidth", 0.0007, "Joints", 0.0002, 0.004)
    seen_w = g.max(width, g.mul(pixel, 1.5))
    seen = g.max(g.div(width, seen_w), g.scalar("JointMinVisible", 0.6, "Joints", 0, 1))
    joint = g.mul(g.one_minus(g.smoothstep(g.mul(seen_w, 0.2), g.mul(seen_w, 0.5), dist)), seen)
    c = g.lerp(c, g.mul(c, g.scalar("JointDarkness", 0.42, "Joints", 0, 1)), joint)

    # The line: the wax worn along the axis (the bays' middle 3 m; not in the cabinet), a little paler and duller.
    y = g.mask(p, "G")
    lane = g.mul(g.one_minus(g.smoothstep(1.0, 1.9, g.abs(y))), g.step(-7.0, y))
    macro = g.noise(g.div(p, 1.7), levels=3)
    lane = g.mul(lane, g.add(0.75, g.mul(macro, 0.25)))
    worn = g.mul(lane, g.scalar("LaneWear", 1.0, "Wear", 0, 1))
    c = g.lerp(c, g.mul(g.lerp(c, g.mul(g.add(g.add(g.mask(c, "R"), g.mask(c, "G")), g.mask(c, "B")), 0.3333), 0.15), 1.06),
               g.mul(worn, 0.5))

    # Polish: waxed satin, the pores duller (the veneer's roughness about its mean), a slow drift, the lane and the joints.
    rough = g.add(g.scalar("Roughness", 0.36, "Surface", 0, 1),
                  g.mul(g.sub((orm, "G"), pbr.stats()["W2"]["rough_mean"]), g.scalar("PBRRoughInfluence", 0.8, "Photo", 0, 2)))
    rough = g.add(g.add(rough, g.mul(macro, 0.04)), g.mul(worn, 0.10))
    rough = g.sat(g.add(rough, g.mul(joint, 0.35)))
    coat = g.mul(g.mul(g.scalar("ClearCoat", 0.45, "Surface", 0, 1), g.one_minus(joint)), g.one_minus(g.mul(worn, 0.5)))
    coat_r = g.sat(g.add(g.add(g.scalar("ClearCoatRoughness", 0.11, "Surface", 0, 1), g.mul(macro, 0.05)), g.mul(worn, 0.12)))

    # The normal: the veneer's relief in the piece's frame, the piece's own tilt, the joints' eased arrises.
    strength = g.scalar("NormalStrength", 0.55, "Photo", 0, 2)
    a_ = g.mul(g.mask(nrm, "R"), strength)
    b_ = g.mul(g.mul(g.mask(nrm, "G"), strength), -1.0)
    up = pbr.vec3(g, 0, 0, 1)
    n = g.add(g.add(g.mul(e_across, a_), g.mul(e_along, b_)), g.mul(up, g.mask(nrm, "B")))
    tilt_amt = g.mul(g.scalar("PieceTilt", 0.15, "Surface", 0, 1), 0.01745 * 2.0)
    tilt = g.mul(g.add(g.mul(e_across, g.sub(r2, 0.5)), g.mul(e_along, g.sub(r3, 0.5))), tilt_amt)
    bevel = g.scalar("JointBevel", 0.0008, "Joints", 0, 0.004)
    in_arris = g.one_minus(g.smoothstep(g.mul(bevel, 0.5), g.add(bevel, 0.0003), dist))
    arris_fade = g.sat(g.div(bevel, g.mul(pixel, 1.5)))
    arris = g.mul(g.append(g.mul(joint_dir, -0.5), zero), g.mul(in_arris, arris_fade))
    n = g.unary(unreal.MaterialExpressionNormalize, g.add(g.add(n, tilt), arris))
    ao = g.mul(g.lerp(1.0, (orm, "R"), 0.5), g.lerp(1.0, g.scalar("JointAO", 0.45, "Joints", 0, 1), joint))
    pbr.world_normal_material(m)
    g.output(BaseColor=c, Metallic=0.0, Specular=g.scalar("Specular", 0.45, "Surface", 0, 1), Roughness=rough, Normal=n,
             AmbientOcclusion=ao, ClearCoat=coat, ClearCoatRoughness=coat_r,
             Anisotropy=g.scalar("Anisotropy", -0.3, "Surface", -1, 1), Tangent=e_along)
    finish(m)
    instance("MI_Salon_Parquet", m)
    return m


# --------------------------------------------------------------------------- the sky, the glass, the diffusers, the velarium

CLOUD_HLSL = r"""
// The cloud field over the Salon, in plan metres at the lanterns (a virtual layer: the real clouds are 1.5 km up; this is
// their size and speed as seen through a lantern from the floor). Cloud: 0 clear … 1 overcast. Returns the cover (0…1) and
// the cloud's density (for its shading).
float2 q = (P.xy + Drift * T) / Scale;
float n = 0.0, amp = 0.5, fr = 1.0;
[unroll] for (int i = 0; i < 5; i++)
{
    float2 x = q * fr + float2(17.3 * i, 9.1 * i);
    float2 ip = floor(x);
    float2 fp = x - ip;
    float2 u = fp * fp * (3.0 - 2.0 * fp);
    float a = frac(sin(dot(ip, float2(127.1, 311.7))) * 43758.5453);
    float b = frac(sin(dot(ip + float2(1, 0), float2(127.1, 311.7))) * 43758.5453);
    float c = frac(sin(dot(ip + float2(0, 1), float2(127.1, 311.7))) * 43758.5453);
    float d = frac(sin(dot(ip + float2(1, 1), float2(127.1, 311.7))) * 43758.5453);
    n += amp * lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
    amp *= 0.5;
    fr *= 2.03;
}
float edge = 0.08;
float t0 = 1.0 - saturate(Cloud) * 1.08;
float cover = smoothstep(t0 - edge, t0 + edge, n);
cover = Cloud >= 0.995 ? 1.0 : cover;
return float2(cover, saturate((n - t0) * 3.0));
"""


def cloud_field(g, scale=9.0):
    M = materials_module()
    p = M.world_metres(g)
    drift = g.mul(g.append(mpc_param(g, "CloudDriftX"), mpc_param(g, "CloudDriftY")), g.scalar("DriftScale", 0.02, "Sky", 0, 1))
    return custom(g, CLOUD_HLSL, 2, [("P", p), ("Drift", drift), ("T", g.node(unreal.MaterialExpressionTime)),
                                     ("Scale", g.scalar("CloudScale", scale, "Sky", 0.5, 100)), ("Cloud", mpc_param(g, "Cloud"))],
                  [], "Cloud field")


def make_sky_veil():
    """M_Salon_SkyVeil: the cloud over a lantern (unlit, masked): where there is cloud it shows it (bright white where the
    sun lights it, grey when overcast, darker in rain) and stops the sun; where there is none the sky and the sun come
    through. MaxCover < 1 (bay 3): never quite closed, so the leaves above still show against an overcast sky."""
    M = materials_module()
    m = master("M_Salon_SkyVeil", unreal.MaterialShadingModel.MSM_UNLIT, unreal.BlendMode.BLEND_MASKED, two_sided=True, attributes=False)
    m.set_editor_property("opacity_mask_clip_value", 0.5)
    g = M.Graph(m)
    field = cloud_field(g)
    cover, density = g.mask(field, "R"), g.mask(field, "G")
    cover = g.min(cover, g.scalar("MaxCover", 1.0, "Sky", 0, 1))
    mask = g.step(M.dither(g), cover)
    cloud, rain = mpc_param(g, "Cloud"), mpc_param(g, "Rain")
    overcast = g.smoothstep(0.7, 1.0, cloud)
    lum = g.mul(g.lerp(g.scalar("SunlitNits", 9000.0, "Sky", 0, 50000), g.scalar("OvercastNits", 3800.0, "Sky", 0, 50000), overcast),
                g.lerp(1.0, 0.45, rain))
    lum = g.mul(lum, g.lerp(1.0, 0.72, g.mul(density, g.one_minus(overcast))))   # a cloud's thick core is greyer
    day = g.max(musee_param(g, "Daylight"), 0.02)
    colour = g.lerp(g.vector("LitTint", (1.0, 0.99, 0.97), "Sky"),
                    g.vector("OvercastTint", (0.90, 0.93, 0.97), "Sky"), overcast)
    MEL.connect_material_property(g.mul(g.mul(colour, lum), day), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.connect_material_property(mask, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    errors = list(MEL.recompile_material(m) or [])
    for e in errors:
        warn(f"M_Salon_SkyVeil: {e}")
    EAL.save_loaded_asset(m)
    instance("MI_Salon_SkyVeil", m)
    instance("MI_Salon_SkyVeilLeaves", m, {"MaxCover": 0.72})
    log("M_Salon_SkyVeil")


RAIN_HLSL = r"""
// Drops on a pane: cells of Cell metres, each with a drop that lands, spreads and dries out in turn (Rain: 0 … 1). Returns
// (slope x, slope y, wetness). P: world metres; T: seconds.
float2 x = P.xy / Cell;
float2 ip = floor(x);
float2 fp = x - ip - 0.5;
float3 acc = 0.0;
[unroll] for (int j = -1; j <= 1; j++)
[unroll] for (int i = -1; i <= 1; i++)
{
    float2 o = float2(i, j);
    float2 id = ip + o;
    float h1 = frac(sin(dot(id, float2(127.1, 311.7))) * 43758.5453);
    float h2 = frac(sin(dot(id, float2(269.5, 183.3))) * 43758.5453);
    float h3 = frac(sin(dot(id, float2(419.2, 371.9))) * 43758.5453);
    float2 c = o + (float2(h1, h2) - 0.5) * 0.7 - fp;
    float life = frac(T * (0.15 + 0.3 * h3) + h1);
    float present = step(h3, Rain * 1.2);
    float r = (0.18 + 0.25 * h2) * saturate(life * 6.0) * present;
    float d = length(c);
    float inside = saturate((r - d) * 12.0);
    float2 slope = -c / max(r, 1e-3) * inside * (1.0 - saturate(d / max(r, 1e-3)));
    acc += float3(slope, inside * (1.0 - life * 0.6));
}
return acc;
"""


def make_glass():
    """M_Salon_Glass: the lanterns' clear panes (low-iron, Thin Translucent as M_Glass), with the rain on them: drops that
    land, spread and dry (their lensing in the reflection, a faint grey film where they lie)."""
    M = materials_module()
    m = master("M_Salon_Glass", unreal.MaterialShadingModel.MSM_DEFAULT_LIT, unreal.BlendMode.BLEND_TRANSLUCENT, two_sided=True,
               attributes=False)
    for prop, value in [("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING),
                        ("refraction_method", unreal.RefractionMode.RM_NONE)]:
        try:
            m.set_editor_property(prop, value)
        except Exception as e:  # noqa: BLE001
            warn(f"M_Salon_Glass: {prop} ({e})")
    g = M.Graph(m)
    p = M.world_metres(g)
    rain = mpc_param(g, "Rain")
    drops = custom(g, RAIN_HLSL, 3, [("P", p), ("T", g.node(unreal.MaterialExpressionTime)), ("Rain", rain),
                                     ("Cell", g.scalar("DropCell", 0.012, "Rain", 0.002, 0.1))], [], "Rain drops")
    wet = g.sat(g.mask(drops, "B"))
    slope = g.mul(g.mask(drops, "RG"), 0.6)
    n = g.unary(unreal.MaterialExpressionNormalize, g.append(slope, 1.0))
    MEL.connect_material_property(n, "", unreal.MaterialProperty.MP_NORMAL)
    tint = g.vector("Tint", (0.93, 0.965, 0.955), "Glass")
    trans = g.mul(g.lerp(g.vector("Clear", (1.0, 1.0, 1.0), "Glass"), tint, 0.35),
                  g.sub(1.0, g.mul(wet, 0.12)))
    out = g.node(unreal.MaterialExpressionThinTranslucentMaterialOutput)
    g.link(trans, out, "TransmittanceColor")
    MEL.connect_material_property(g.vector("Dust", (0.55, 0.55, 0.53), "Glass"), "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(g.add(0.002, g.mul(wet, 0.02)), "", unreal.MaterialProperty.MP_OPACITY)
    MEL.connect_material_property(g.add(g.scalar("Roughness", 0.01, "Glass", 0, 1), g.mul(wet, 0.02)), "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(g.const(0.5), "", unreal.MaterialProperty.MP_SPECULAR)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_THIN_TRANSLUCENT)
    errors = list(MEL.recompile_material(m) or [])
    for e in errors:
        warn(f"M_Salon_Glass: {e}")
    EAL.save_loaded_asset(m)
    instance("MI_Salon_LanternGlass", m)
    log("M_Salon_Glass")


def lantern_rect(bay):
    """Plan metres: bay (1–5)'s lantern opening, as ASalonStructure lays it (19 rows of coffers round, 8 along)."""
    west = {1: -25.4, 2: -37.4, 3: -49.4, 4: -61.4, 5: -74.0}[bay]
    east = {1: -14.0, 2: -26.6, 3: -38.6, 4: -50.6, 5: -62.6}[bay]
    pitch = math.pi * 7.0 / 19.0
    strip = 0.5 * (10.8 - 8 * pitch)
    col = lambda c: west + strip + (east - west - 2 * strip) * c / 8.0  # noqa: E731
    a0, a1 = math.pi * 54 / 114, math.pi * 60 / 114
    return col(3), -7.0 * math.cos(a0), col(5), -7.0 * math.cos(a1)


def make_diffusers():
    """M_Salon_Diffuser: bay 2's opal glass and bay 4's muslin. Each stops the sun (it casts its shadow) and glows with what it
    passes on: τ · E / π nits, E the light on the roof (MPC_Salon RoofLux, from ASalonSky); the room's own light on it too
    (default lit: the opal's glossy face, the muslin's matt weave). On the muslin, where the sun falls, the steel grid above
    it prints its bars."""
    M = materials_module()
    m = master("M_Salon_Diffuser", unreal.MaterialShadingModel.MSM_DEFAULT_LIT, two_sided=True)
    g = M.Graph(m)
    p = M.world_metres(g)
    roof = mpc_param(g, "RoofLux")
    sun = mpc_param(g, "SunShare")
    glow = g.mul(g.mul(roof, g.div(g.scalar("Tau", 0.3, "Diffuser", 0, 1), math.pi)), g.vector("Tint", (1.0, 0.99, 0.97), "Diffuser"))
    # The cloth (or the glass's milkiness): a slow mottle and, for the muslin, the weave and its slubs.
    mottle = g.add(1.0, g.mul(g.noise(g.div(p, 0.18), levels=3), g.scalar("Mottle", 0.03, "Diffuser", 0, 0.3)))
    weave = g.add(1.0, g.mul(g.noise(g.append(g.append(g.div(g.mask(p, "R"), 0.35), g.div(g.mask(p, "G"), 0.0025)), 0.3), levels=2),
                             g.scalar("Slubs", 0.0, "Diffuser", 0, 0.3)))
    # The grid's shadow (muslin): its bars at the lantern's sixths along x and on the middle along y, 30 mm, where the sun is.
    x, y = g.mask(p, "R"), g.mask(p, "G")
    x0, x1 = g.scalar("GridX0", 0.0, "Grid"), g.scalar("GridX1", 1.0, "Grid")
    t = g.div(g.sub(x, x0), g.sub(x1, x0))
    fx = g.mul(g.abs(g.sub(g.frac(g.mul(t, 6.0)), 0.5)), g.div(g.sub(x1, x0), 6.0))   # metres to the nearest bar's centre line …
    bar_x = g.one_minus(g.smoothstep(0.012, 0.018, g.sub(g.div(g.sub(x1, x0), 12.0), fx)))
    bar_y = g.one_minus(g.smoothstep(0.012, 0.018, g.abs(y)))
    bars = g.sat(g.add(bar_x, bar_y))
    shade = g.sub(1.0, g.mul(g.mul(bars, sun), g.scalar("GridShadow", 0.0, "Grid", 0, 1)))
    emissive = g.mul(g.mul(g.mul(glow, mottle), weave), shade)
    g.output(BaseColor=g.vector("BaseColor", (0.85, 0.84, 0.82), "Diffuser"), Metallic=0.0,
             Roughness=g.scalar("Roughness", 0.08, "Diffuser", 0, 1), Specular=0.5, EmissiveColor=emissive)
    finish(m)
    instance("MI_Salon_Opal", m, {"Tau": 0.30, "Roughness": 0.06, "Mottle": 0.025}, {"BaseColor": (0.86, 0.86, 0.85), "Tint": (1.0, 1.0, 0.99)})
    gx0, _, gx1, _ = lantern_rect(4)
    instance("MI_Salon_Muslin", m, {"Tau": 0.40, "Roughness": 0.92, "Mottle": 0.05, "Slubs": 0.08, "GridShadow": 0.65,
                                    "GridX0": gx0, "GridX1": gx1},
             {"BaseColor": lin(0xCFC6B2), "Tint": (1.0, 0.96, 0.88)})


VELUM_RAIN_HLSL = r"""
// The rain on the glass over the velarium, seen through the cloth: the drops' soft shadows, landing and running. Returns darkening 0…1.
float2 x = P.xy / 0.035;
float2 ip = floor(x);
float2 fp = x - ip - 0.5;
float acc = 0.0;
[unroll] for (int j = -1; j <= 1; j++)
[unroll] for (int i = -1; i <= 1; i++)
{
    float2 o = float2(i, j);
    float2 id = ip + o;
    float h1 = frac(sin(dot(id, float2(127.1, 311.7))) * 43758.5453);
    float h2 = frac(sin(dot(id, float2(269.5, 183.3))) * 43758.5453);
    float h3 = frac(sin(dot(id, float2(419.2, 371.9))) * 43758.5453);
    float life = frac(T * (0.2 + 0.35 * h3) + h1);
    float2 c = o + (float2(h1, h2) - 0.5) * 0.8 - fp + float2(0.0, life * 0.6 * h2);
    float r = 0.10 + 0.12 * h2;
    acc = max(acc, step(h3, Rain) * (1.0 - smoothstep(r * 0.4, r, length(c))) * (1.0 - life));
}
return acc;
"""


def make_velarium():
    """M_Salon_Velarium: the oval's velarium, a cloth under a glass roof, glowing with Giverny's sky over it: τ · E / π nits
    (as the room's light, ASalonSky); where broken cloud passes, its shadow crosses the cloth; in rain, the drops on the glass
    above show through as soft moving shadows. The cloth: stretched widths with their seams, a fine weave."""
    M = materials_module()
    m = master("M_Salon_Velarium", unreal.MaterialShadingModel.MSM_UNLIT, two_sided=True, attributes=False)
    g = M.Graph(m)
    p = M.world_metres(g)
    roof, sun, rain = mpc_param(g, "RoofLux"), mpc_param(g, "SunShare"), mpc_param(g, "Rain")
    field = cloud_field(g, 7.0)
    shadow = g.mul(g.mask(field, "R"), sun)
    light = g.mul(roof, g.one_minus(g.mul(shadow, 0.85)))
    drops = custom(g, VELUM_RAIN_HLSL, 1, [("P", p), ("T", g.node(unreal.MaterialExpressionTime)), ("Rain", rain)], [], "Rain on the glass")
    light = g.mul(light, g.one_minus(g.mul(drops, 0.35)))
    # The cloth: widths of 1.4 m along the oval's length with their seams (a faint double line), and the weave.
    y = g.mask(p, "G")
    seam = g.one_minus(g.smoothstep(0.004, 0.012, g.abs(g.sub(g.frac(g.div(g.add(y, 0.7), 1.4)), 0.5))))
    cloth = g.mul(g.add(1.0, g.mul(g.noise(g.div(p, 0.4), levels=3), 0.03)), g.one_minus(g.mul(seam, 0.12)))
    nits = g.mul(g.mul(light, g.div(g.scalar("Tau", 0.12, "Velarium", 0, 1), math.pi)), cloth)
    MEL.connect_material_property(g.mul(nits, g.vector("Tint", (1.0, 0.995, 0.985), "Velarium")), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    errors = list(MEL.recompile_material(m) or [])
    for e in errors:
        warn(f"M_Salon_Velarium: {e}")
    EAL.save_loaded_asset(m)
    instance("MI_Salon_Velarium", m)
    log("M_Salon_Velarium")


# --------------------------------------------------------------------------- canvases, paper, leaves, water

def make_canvas_master():
    """M_Salon_Canvas: a painting on canvas, unvarnished (Monet left the Nymphéas matt): the image on UV0, the paint's relief
    from the image itself (its luminance's slope at a few texels: the strokes catch a raking light), the canvas's weave
    where it is larger than a pixel."""
    M = materials_module()
    m = master("M_Salon_Canvas", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    g = M.Graph(m)
    uv = g.node(unreal.MaterialExpressionTextureCoordinate)
    default = unreal.load_asset("/Engine/EngineResources/DefaultTexture")
    img = g.texture("Image", default, uv, "Painting")
    # The strokes' slope, taken over two texels of the full image, or over the pixel's own footprint where that is larger
    # (the flicker check: from across the oval a two-texel difference read inside a coarse mip was noise, and single
    # pixels of the impasto caught the lamps and sparkled frame to frame). Scaled back to the slope per two texels, so
    # the relief is the same stroke seen nearer or further, only filtered.
    tu, tv = g.scalar("TexelU", 1.0 / 16384, "Painting", 0, 0.01), g.scalar("TexelV", 1.0 / 2570, "Painting", 0, 0.01)
    fx, fy = g.unary(unreal.MaterialExpressionDDX, uv), g.unary(unreal.MaterialExpressionDDY, uv)
    foot_u = g.max(g.abs(g.mask(fx, "R")), g.abs(g.mask(fy, "R")))
    foot_v = g.max(g.abs(g.mask(fx, "G")), g.abs(g.mask(fy, "G")))
    step_u, step_v = g.max(g.mul(tu, 2.0), g.mul(foot_u, 1.5)), g.max(g.mul(tv, 2.0), g.mul(foot_v, 1.5))
    lum = lambda s: g.add(g.add(g.mul(g.mask(s, "R"), 0.3), g.mul(g.mask(s, "G"), 0.59)), g.mul(g.mask(s, "B"), 0.11))  # noqa: E731
    sx = g.texture("ImageX", default, g.add(uv, g.append(step_u, 0.0)), "Painting")
    sy = g.texture("ImageY", default, g.add(uv, g.append(0.0, step_v)), "Painting")
    l0 = lum(img)
    slope = g.append(g.mul(g.sub(lum(sx), l0), g.div(g.mul(tu, 2.0), step_u)),
                     g.mul(g.sub(lum(sy), l0), g.div(g.mul(tv, 2.0), step_v)))
    relief = g.mul(slope, g.scalar("Impasto", 1.6, "Painting", 0, 10))
    length_m, height_m = g.scalar("LengthM", 12.75, "Painting", 0.1, 30), g.scalar("HeightM", 2.0, "Painting", 0.1, 5)
    pixel = g.mul(foot_u, length_m)                      # a pixel's width on the canvas, metres
    wu, wv = g.mul(g.mask(uv, "R"), g.div(length_m, 0.0012)), g.mul(g.mask(uv, "G"), g.div(height_m, 0.0012))
    weave = g.mul(g.append(g.sin(g.mul(wu, 6.2831853)), g.sin(g.mul(wv, 6.2831853))), 0.08)
    weave = g.mul(weave, g.one_minus(g.smoothstep(0.0004, 0.0012, pixel)))
    n = g.unary(unreal.MaterialExpressionNormalize, g.append(g.mul(g.add(relief, weave), -1.0), 1.0))
    g.output(BaseColor=(img, "RGB"), Metallic=0.0, Roughness=g.scalar("Roughness", 0.72, "Painting", 0, 1),
             Specular=g.scalar("Specular", 0.45, "Painting", 0, 1), Normal=n)
    finish(m)
    return m


def make_paper_master():
    """M_Salon_Paper: a printed sheet (the image on UV0), matt, its fibres a whisper in the light, a little aged at the edge."""
    M = materials_module()
    m = master("M_Salon_Paper", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    g = M.Graph(m)
    uv = g.node(unreal.MaterialExpressionTextureCoordinate)
    img = g.texture("Image", unreal.load_asset("/Engine/EngineResources/DefaultTexture"), uv, "Paper")
    edge = g.min(g.min(g.mask(uv, "R"), g.one_minus(g.mask(uv, "R"))), g.min(g.mask(uv, "G"), g.one_minus(g.mask(uv, "G"))))
    aged = g.lerp(g.vector("EdgeTint", (0.86, 0.78, 0.62), "Paper"), g.vector("Clean", (1.0, 1.0, 1.0), "Paper"),
                  g.smoothstep(0.0, 0.03, edge))
    p = M.world_metres(g)
    fibres = g.noise(g.div(p, 0.0012), levels=2)
    n = g.unary(unreal.MaterialExpressionNormalize, g.append(g.mul(g.append(fibres, g.mul(fibres, -0.7)), 0.03), 1.0))
    g.output(BaseColor=g.mul(g.mul((img, "RGB"), aged), g.scalar("Brightness", 1.0, "Paper", 0, 2)), Metallic=0.0,
             Roughness=g.scalar("Roughness", 0.82, "Paper", 0, 1), Specular=0.35, Normal=n)
    finish(m)
    return m


def make_leaf():
    """MI_Salon_VineLeaf: the pergola's vine (Vitis): a palmate leaf of five lobes, a deep sinus at the stalk, toothed."""
    path = f"{P.MUSEUM_CONTENT}/Nature/M_NatureLeaf"
    if not EAL.does_asset_exist(path):
        warn("M_NatureLeaf missing (nature.py): the vine keeps MI_Leaf_Apple")
        return
    parent = unreal.load_asset(path)
    instance("MI_Salon_VineLeaf", parent, {"Lobes": 5.0, "Inner": 0.55, "LobeSharp": 1.4, "Notch": 0.3, "Serration": 0.08,
                                            "Teeth": 16.0, "Tip": 0.5, "Petiole": 0.0, "Roughness": 0.5, "Specular": 0.4,
                                            "Vein": 0.55})


def make_water():
    path = f"{P.MATERIALS}/M_Water"
    if not EAL.does_asset_exist(path):
        warn("M_Water missing (apply_all glass): the pond keeps its water")
        return None
    # Dyed black, as reflecting pools are: the light is gone within a few centimetres, so the water is a mirror; almost still.
    return instance("MI_Salon_PondWater", unreal.load_asset(path), {"Ripple": 0.035, "Roughness": 0.02},
                    {"Absorption": (0.9, 0.9, 0.85), "Scattering": (0.00005, 0.00005, 0.00005)})


# --------------------------------------------------------------------------- the pond's edge (the pond-edge redesign)

def make_pond():
    """M_Salon_RimLight: the line of light in the pond's gilt lip (a warm 2700 K strip, lit length by length by APondLift);
    the planting's materials (in the Nature folder, where the plants look for them)."""
    M = materials_module()
    m = master("M_Salon_RimLight", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    g = M.Graph(m)
    glow = g.mul(g.vector("Colour", (1.0, 0.63, 0.30), "Light"), g.scalar("Luminance", 2500.0, "Light", 0, 20000))
    g.output(BaseColor=g.vector("BaseColor", (0.30, 0.22, 0.12), "Surface"), Metallic=0.0, Specular=0.5,
             Roughness=g.scalar("Roughness", 0.35, "Surface", 0, 1), EmissiveColor=glow)
    finish(m)
    instance("MI_Salon_RimLight", m, {"Luminance": 2500.0})
    # The frieze is carved in the closest-grained, filled travertine the quarry gives (a carver's choice): the voids filled,
    # the veins quieter, so the carving reads through its light and shadow.
    ashlar = f"{FOLDER}/M_Salon_Ashlar"
    if EAL.does_asset_exist(ashlar):
        instance("MI_Salon_AshlarCarved", unreal.load_asset(ashlar), {"VoidDark": 0.08, "PBRContrast": 0.3, "NormalStrength": 0.7,
                                                                      "PBRScale": 2.0, "StoneTone": 0.12})
    # The tray under the pond (seen from the stair when it is lifted): the posts' bronze, brushed and dulled with age, so the
    # soffit stays quiet overhead.
    metal = f"{P.MATERIALS}/M_Metal"
    if EAL.does_asset_exist(metal) and "M_Bronze_Brushed" in M.NAMED:
        tray = {k: (dict(v) if isinstance(v, dict) else v) for k, v in M.NAMED["M_Bronze_Brushed"].items()}
        tray["scalars"].update({"Roughness": 0.5, "PatinaAmount": 0.35, "Variation": 0.08})
        tray["vectors"]["BaseColor"] = lin(0x7A5E40)
        M.PARAMS["M_Metal"] = M.parameter_names(unreal.load_asset(metal))
        instance_spec("MI_Salon_PondTray", unreal.load_asset(metal), tray)
    nature_folder = f"{P.MUSEUM_CONTENT}/Nature"
    if not EAL.does_asset_exist(f"{nature_folder}/M_NaturePetal") or not EAL.does_asset_exist(f"{nature_folder}/M_NatureLeaf"):
        warn("the Nature masters are missing (nature.py): the pond's planting keeps the default material")
        return
    import nature as N
    petal = unreal.load_asset(f"{nature_folder}/M_NaturePetal")
    leaf = unreal.load_asset(f"{nature_folder}/M_NatureLeaf")
    # Iris laevigata: smooth sword leaves (no midrib), a waxy sheen; the flowers' colour is in their vertices (the signal).
    for name, (sc, vc) in {
        "MI_Salon_IrisLeaf": N.petal(0.35, 9.0, 0.42, underside=0x6A8A48, under_mix=0.15, sss=(0.6, 0.85, 0.35), flutter=0.1, spec=0.45),
        "MI_Salon_IrisPetal": N.petal(0.55, 16.0, 0.5, sss=(0.85, 0.75, 1.0), flutter=0.1, spec=0.35),
        "MI_Salon_Sedge": N.petal(0.25, 3.0, 0.52, underside=0x7A8A48, under_mix=0.3, sss=(0.8, 1.0, 0.5), flutter=0.15),
    }.items():
        N.instance(name, petal, sc, vc)
    sc, vc = N.flower(5, 0.4, 0.3, 0.0, 0xF4D040, 0.2, vein=0.3, rough=0.5, sss=(0.9, 0.95, 1.1), flutter=0.1)
    N.instance("MI_Salon_ForgetMeNot", leaf, sc, vc)
    log("the pond's edge: M_Salon_RimLight and the planting's materials")


def manifest():
    path = os.path.join(ASSETS, "manifest.json")
    try:
        with open(path, encoding="utf-8") as f:
            return json.load(f)
    except Exception as e:  # noqa: BLE001
        warn(f"{path}: {e}")
        return {}


def make_art():
    """The willows' and the documents' textures and their instances."""
    canvas = make_canvas_master()
    paper = make_paper_master()
    info = manifest()
    for slug, title, length, _ in WILLOWS:
        img = os.path.join(ASSETS, f"{slug}.jpg")
        tex = texture(img, "T_" + slug.replace("-", "_"), max_size=8192)   # 0.6 px a millimetre on 12.75 m: enough, and light on video memory
        if tex is None:
            continue
        w, h = tex.blueprint_get_size_x(), tex.blueprint_get_size_y()
        instance(f"MI_Salon_{slug.replace('-', '_')}", canvas, {"LengthM": length, "HeightM": 2.0, "TexelU": 1.0 / max(1, w),
                                                                 "TexelV": 1.0 / max(1, h)},
                 textures={"Image": tex, "ImageX": tex, "ImageY": tex})
    for key, asset in (("catalogue", "MI_Salon_Catalogue1874"), ("charivari", "MI_Salon_Charivari1874")):
        img = os.path.join(ASSETS, info.get("docs", {}).get(key, {}).get("file", f"{key}.jpg"))
        tex = texture(img, f"T_salon_{key}_1874", max_size=4096)
        if tex is not None:
            instance(asset, paper, {"Brightness": 0.86}, textures={"Image": tex})   # aged rag paper, not white


def make():
    """Everything above (idempotent: masters rebuilt in place, instances updated)."""
    import pbr  # noqa: F401 - the photo library the masters use
    ensure_folder(FOLDER)
    make_mpc()
    make_fabrics()
    make_ashlar()
    make_parquet()
    make_sky_veil()
    make_glass()
    make_diffusers()
    make_velarium()
    make_leaf()
    make_water()
    make_pond()
    make_art()
    EAL.save_directory(FOLDER, only_if_is_dirty=True, recursive=True)
    log("materials done")


# --------------------------------------------------------------------------- the pieces in the map

def tag_value(actor, prefix):
    for t in actor.tags:
        s = str(t)
        if s.startswith(prefix):
            return s[len(prefix):]
    return None


def spawn(eas, cls_name, label, tags, props=None, wing="Salon"):
    cls = unreal.load_class(None, f"/Script/MuseeVision.{cls_name}")
    if not cls:
        warn(f"no class {cls_name}: build the editor first")
        return None
    a = eas.spawn_actor_from_class(cls, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    for k, v in (props or {}).items():
        a.set_editor_property(k, v)
    a.set_actor_label(label)
    a.tags = [unreal.Name(t) for t in ["musee.building", f"musee.wing:{wing}", "musee.native", TAG] + tags]
    return a


# The bays' exposure (the Light board's order of brightness: the lantern, then the paintings, the silk, the stone, the vault
# in half-light): the museum's sky-lit rooms are exposed high-key (exposure.py's curve, +0.8 stop at EV100 12.5); the bays
# are not, and keep more of their contrast. The oval stays high-key: the white room at the end.
BAYS_EXPOSURE_BIAS = 0.35
BAYS_BOX_CM = ((-7420.0, -1560.0, -40.0), (-1360.0, 740.0, 1420.0))


def exposure_zone(eas):
    lo, hi = BAYS_BOX_CM
    centre = unreal.Vector((lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, (lo[2] + hi[2]) / 2)
    ppv = eas.spawn_actor_from_class(unreal.PostProcessVolume, centre, unreal.Rotator(0, 0, 0))
    ppv.set_actor_label("Salon bays exposure")
    ppv.tags = [unreal.Name("musee.exposure_zone"), unreal.Name("musee.native"), unreal.Name(TAG)]
    ppv.set_editor_property("unbound", False)
    ppv.set_editor_property("priority", 11.0)
    ppv.set_editor_property("blend_radius", 250.0)
    ppv.set_actor_scale3d(unreal.Vector((hi[0] - lo[0]) / 200.0, (hi[1] - lo[1]) / 200.0, (hi[2] - lo[2]) / 200.0))
    s = ppv.settings
    for k, v in (("auto_exposure_bias", BAYS_EXPOSURE_BIAS), ("local_exposure_shadow_contrast_scale", 1.0),
                 ("local_exposure_highlight_contrast_scale", 0.5)):
        s.set_editor_property("override_" + k, True)
        s.set_editor_property(k, v)
    s.set_editor_property("override_auto_exposure_bias_curve", True)
    s.set_editor_property("auto_exposure_bias_curve", None)
    ppv.set_editor_property("settings", s)


def place(eas):
    """The case, the four canvases, the pond's kerb; the pieces they replace retired (idempotent)."""
    import relight
    for a in eas.get_all_level_actors():
        if TAG in [str(t) for t in a.tags]:
            eas.destroy_actor(a)
    info = manifest()
    # The 1874 case, opposite the stele: its documents at their scans' proportions.
    docs = info.get("docs", {})
    def size(key, width):
        d = docs.get(key, {})
        if d.get("px"):
            w, h = d["px"]
            return unreal.Vector2D(width, width * h / max(1, w))
        return None
    props = {}
    cat = size("catalogue", docs.get("catalogue", {}).get("width_m", 0.27))
    cha = size("charivari", docs.get("charivari", {}).get("width_m", 0.32))
    if cat:
        props["catalogue_size"] = cat
    if cha:
        props["charivari_size"] = cha
    if all(EAL.does_asset_exist(f"{FOLDER}/{n}") for n in ("MI_Salon_Catalogue1874", "MI_Salon_Charivari1874")):
        spawn(eas, "SalonCase", "Salon 1874 case", ["work:salon-1874-case", "musee.paper"], props)
    else:
        warn("the 1874 documents' materials missing (salon step): the case waits for them")
    # The willows round the oval: in order from the door's north jamb, equal joints between.
    wall = unreal.SalonCanvas.wall_length()
    total = sum(w[2] for w in WILLOWS)
    joint = (wall - total) / (len(WILLOWS) + 1)
    at = joint
    placed = 0
    for slug, title, length, canvases in WILLOWS:
        mi_path = f"{FOLDER}/MI_Salon_{slug.replace('-', '_')}"
        props = {"start_along": at, "length": length, "canvases": canvases}
        if EAL.does_asset_exist(mi_path):
            props["painting_material"] = unreal.load_asset(mi_path)
        else:
            warn(f"{mi_path} missing: {title} would be blank; left out")
            at += length + joint
            continue
        if spawn(eas, "SalonCanvas", f"Willows: {title}", [f"work:{slug}", "musee.daylit"], props):
            placed += 1
        at += length + joint
    log(f"the willows: {total:.2f} m of painting round {wall:.2f} m of wall, joints of {joint:.2f} m")
    # The pond's edge (the coping, its lip and frieze) and its planting, attached to the lifting pond (APondLift moves what
    # is attached to it). The planting stands at the water's surface (h 0.38) at the pond's centre.
    pond = next((a for a in eas.get_all_level_actors() if "part:pond" in [str(t) for t in a.tags]), None)
    kerb = spawn(eas, "SalonPond", "Salon pond edge", [], wing="Reserve")
    plants = spawn(eas, "SalonPondPlants", "Salon pond planting", ["musee.nobake"], wing="Reserve")
    if plants:
        plants.set_actor_location(unreal.Vector(-8650.0, 0.0, 38.0), False, False)
    for a in (kerb, plants):
        if pond and a:
            a.attach_to_actor(pond, unreal.Name(""), unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD,
                              unreal.AttachmentRule.KEEP_WORLD, False)
    if not pond:
        warn("no pond (part:pond): the pond's edge stands alone")
    # The pond's water, dyed black (Single Layer Water: drawn as an ordinary mesh, never Nanite).
    water_mi = f"{FOLDER}/MI_Salon_PondWater"
    for a in eas.get_all_level_actors():
        if tag_value(a, "prim:") == "/Museum/Reserve/Lily_pond/Water" and EAL.does_asset_exist(water_mi):
            for c in a.get_components_by_class(unreal.StaticMeshComponent):
                c.set_material(0, unreal.load_asset(water_mi))
                try:
                    c.set_editor_property("disallow_nanite", True)
                except Exception as e:  # noqa: BLE001
                    warn(f"the pond's water: disallow_nanite ({e})")
    exposure_zone(eas)
    # What the update replaces.
    # What the update replaces: the first room's panels only once all four willows hang (never a bare oval), the basin
    # only once the kerb stands on the pond.
    retire = [r for r in RETIRE if (placed == len(WILLOWS) or "Lily_pond" in r) and (kerb is not None or "Lily_pond" not in r)]
    retired = 0
    for a in eas.get_all_level_actors():
        if tag_value(a, "prim:") in retire:
            relight.retire(a)
            retired += 1
    # The velarium's old light (relight.py's): ASalonSky lights it now, with the weather.
    removed = 0
    sky = unreal.load_class(None, "/Script/MuseeVision.SalonSky")
    has_sky = sky is not None and any(a.get_class() == sky for a in eas.get_all_level_actors())
    for a in eas.get_all_level_actors():
        if has_sky and isinstance(a, unreal.RectLight) and a.get_actor_label() == "Laylight Velarium":
            eas.destroy_actor(a)
            removed += 1
    log(f"placed the case, {placed} canvases and the pond's kerb; {retired} imported pieces retired, {removed} old velarium light removed")


if __name__ == "__main__":
    make()
