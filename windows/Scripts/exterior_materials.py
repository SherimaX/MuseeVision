"""
The exterior's weathered materials (the modern wing's audit, 2026-09-26), in /Game/Museum/Materials/Exterior:

    UnrealEditor-Cmd.exe MuseeVision.uproject -run=pythonscript -script="<repo>/windows/Scripts/exterior_materials.py [assign]"

Outside, stone is never clean: the grounds and the façades looked fresh from the saw (the audit: "would it pass as a
photograph?"). Paris limestone and travertine in the open weather in known places, and each has a parameter here:

- rain and soot streaks hanging from every ledge (the cornices, string courses, sills, the podium's cap): a grey
  wash just under the ledge fading down over StreakFall metres, broken into vertical runs (StreakAmount, SootAmount;
  the ledges' heights per building in StreakTopsA/B/C, metres above the ground), with paler washed runs between
  (WashAmount);
- the damp foot of a wall: rising damp and rain splash, a darker band up to DampHeight with a ragged tide line;
- lichen and algae where the sun doesn't dry the stone: grey-green crusts on the north faces and on what faces up
  (copings, steps, cornice tops), a green-black film at a north wall's foot (LichenAmount, LichenColor);
- a water line on a basin's coping: the band the water wets and stains (WaterLineZ, WaterLineAmount);
- on open floors: rain stains and dirt drifts (FloorStain), and the paths people walk across the gravel and the paving,
  paler and dustier where the grit is trodden flat (LaneAmount; the grounds' walks from MuseePlan::Grounds).

These are two masters of the exterior's own, M_Ext_Stone and M_Ext_StoneRelief: materials.py's stone (build_stone:
coursing, joints, per-block tone and tilt, the photographed layer, grime), built again under another name with this
weathering added where build_stone applies its grime. materials.py itself is not touched.

And M_Ext_Pearl / MI_Ext_Pearl, the Élan dome's seamless shell (AElanExteriorStructure, bPearlShell): white ceramic-coated
panels under a clear lacquer, hairline joints on meridians and parallels that fade out under a pixel, faint rain runs.

Self-contained: it imports materials.py and pbr.py and changes neither (build_stone is rebuilt under another name with
materials.grime swapped for weathered_grime during the call). Parameters in the "Weather" group; the weathering is one
Custom node (WEATHER_HLSL), cheap (value noise, no textures). Anything thinner than a pixel (lichen speckle, the runs)
fades to its mean with distance (PixelDepth), so it doesn't shimmer under TSR (the flicker audit).

`assign` also points the exterior's actors at these instances (the façade dress, the grounds, the Élan's exterior
stone) and rebakes just those actors.
"""
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import musee_paths as P  # noqa: E402
import pbr  # noqa: E402
import materials as MAT  # noqa: E402

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
FOLDER = P.MATERIALS + "/Exterior"
lin = MAT.lin


def log(msg):
    unreal.log(f"[exterior_materials] {msg}")


def warn(msg):
    unreal.log_warning(f"[exterior_materials] {msg}")


# ---------------------------------------------------------------------------------------------- the weathering

WEATHER_HLSL = r"""
struct WZ {
    float h(float2 p) { p = frac(p * float2(123.34, 456.21)); p += dot(p, p + 45.32); return frac(p.x * p.y); }
    float n(float2 p) {
        float2 i = floor(p), f = frac(p), u = f * f * (3.0 - 2.0 * f);
        return lerp(lerp(h(i), h(i + float2(1, 0)), u.x), lerp(h(i + float2(0, 1)), h(i + float2(1, 1)), u.x), u.y);
    }
    float fbm(float2 p) { return (0.5 * n(p) + 0.25 * n(p * 2.03 + 7.1) + 0.125 * n(p * 4.01 + 3.3) + 0.0625 * n(p * 8.1 + 1.7)) / 0.9375; }
    float seg(float2 p, float2 a, float2 b) { float2 pa = p - a, ba = b - a; float t = saturate(dot(pa, ba) / dot(ba, ba)); return length(pa - ba * t); }
};
WZ w;
float3 n = normalize(N);
float vert = 1.0 - smoothstep(0.35, 0.7, abs(n.z));
float up = smoothstep(0.6, 0.9, n.z) * (1.0 - Floor * 0.0);
float z = P.z;
float3 c = C;
float r = R;

// Streaks: the nearest ledge above (StreakTops, m), how far under it.
float below = 1e3;
float tops[9] = { TA.r, TA.g, TA.b, TB.r, TB.g, TB.b, TC.r, TC.g, TC.b };
[unroll] for (int k = 0; k < 9; k++) { float t = tops[k]; if (t > 0.01 && z <= t + 0.02) below = min(below, t - z); }
float hang = saturate(1.0 - below / max(Fall, 0.05));
// Each run: a lane Width wide (wandering a little), a run in about half of them, its own length (a quarter to all of
// StreakFall), width and strength; widest under the ledge, tapering and fading as it runs down.
float x = U / max(Width, 0.005) + 0.6 * (w.n(float2(U * 0.8, z * 0.9)) - 0.5);
float li = floor(x), fx = frac(x) - 0.5;
float r1 = w.h(float2(li, 3.7)), r2 = w.h(float2(li, 9.1)), r3 = w.h(float2(li, 17.3));
float len = Fall * lerp(0.2, 1.0, r2 * r2);
float along = saturate(1.0 - below / max(len, 0.05));
float wid = lerp(0.18, 0.46, r3) * (0.35 + 0.65 * along);
float prof = 1.0 - smoothstep(wid * 0.35, wid, abs(fx));
prof = lerp(0.28, prof, saturate(1.0 - (Depth * 0.01 * 0.0008 * 1.5 - Width * 0.2) / (Width * 0.3)));   // runs thinner than a pixel: their mean
float streak = step(0.5, r1) * prof * pow(along, 0.8) * lerp(0.45, 1.0, r1) * (0.7 + 0.3 * w.n(float2(x * 2.0, z * 3.0)));
// (Soot in patches, not a smooth gradient: the renderer's local exposure flattens a slow gradient away, and real soot
// lies in clouds where the water has and hasn't washed.)
float soot = pow(hang, 1.4) * saturate(0.15 + 1.3 * w.fbm(float2(U / 0.9, z / 1.3) + 5.0) - 0.25);
float dark = vert * saturate(StreakAmount * streak + SootAmount * soot);
c = lerp(c, c * StreakTint.rgb, dark);
r = saturate(r + 0.08 * dark);
float washed = step(r1, 0.22) * prof * pow(along, 1.5);
float wash = vert * washed * WashAmount;
c = lerp(c, saturate(c * 1.07 + 0.015), wash);

// The damp foot: a darker band to a ragged tide line.
float tide = DampHeight * (0.7 + 0.6 * w.fbm(float2(U / 1.3, 5.7)));
float damp = vert * (1.0 - smoothstep(tide * 0.6, tide, H)) * DampAmount;
c = c * (1.0 - damp) * lerp(float3(1, 1, 1), float3(0.96, 1.0, 0.93), saturate(damp * 3.0));

// Lichen and algae: north faces (world -Y), what faces up, and a north wall's foot.
float north = saturate(-n.y * 1.25);
float2 q = lerp(float2(U, z), P.xy, Floor);
float blot = w.fbm(q / 0.42 + 11.0);
float pixm = Depth * 0.01 * 0.0008;                                   // a pixel (m)
float fine = lerp(0.5, w.n(q / 0.045), saturate(1.0 - (pixm - 0.006) / 0.012));   // no sub-pixel speckle (it sparkled)
float where = saturate(north * vert * (0.55 + 0.45 * (1.0 - smoothstep(0.0, 3.0, H))) + up * (1.0 - Floor) * 0.8 + Floor * 0.15);
float lich = saturate((blot + 0.2 * fine - 0.5) * 5.0) * where * LichenAmount;
float3 crust = lerp(LichenColor.rgb, LichenColor.rgb * 2.2, step(0.62, fine));
c = lerp(c, crust * (0.85 + 0.3 * fine), lich * 0.75);
r = lerp(r, 0.92, lich);
float bio = north * vert * (1.0 - smoothstep(0.0, 0.9 + 0.5 * blot, H)) * LichenAmount;
c = lerp(c, c * float3(0.42, 0.5, 0.36), saturate(bio * 0.9));

// The water line of a basin's coping, and its wetted stone below.
float wl = WaterLineAmount * vert * (1.0 - smoothstep(0.0, 0.18 + 0.1 * blot, abs(z - WaterLineZ - 0.03)));
float sub = WaterLineAmount * vert * step(z, WaterLineZ);
c = lerp(c, c * float3(0.42, 0.47, 0.36), saturate(wl + 0.5 * sub));
r = lerp(r, 0.35, saturate(sub * 0.7));

// Floors: rain stains and dirt drifts; the trodden paths across the grounds (plan metres: x east, y south).
float stain = Floor * saturate((w.fbm(P.xy / 2.1) - 0.52) * 3.0) * FloorStain;
c *= 1.0 - 0.3 * stain;
float2 p = P.xy + (float2(w.n(P.xy / 2.7), w.n(P.xy / 2.7 + 9.1)) - 0.5) * 0.9;
float d = 1e3;
d = min(d, w.seg(p, float2(-50.0, 22.5), float2(-50.0, 57.8)));
d = min(d, w.seg(p, float2(-38.0, 22.5), float2(-38.0, 57.8)));
d = min(d, w.seg(p, float2(-62.0, 22.5), float2(-62.0, 62.0)));
d = min(d, w.seg(p, float2(-26.0, 22.5), float2(-26.0, 62.0)));
d = min(d, w.seg(p, float2(-64.0, 60.0), float2(-24.0, 60.0)));
d = min(d, w.seg(p, float2(-44.8, 60.0), float2(-44.8, 130.0)));
d = min(d, w.seg(p, float2(-43.2, 60.0), float2(-43.2, 130.0)));
d = min(d, w.seg(p, float2(-26.0, 42.0), float2(48.0, 42.0)));
d = min(d, w.seg(p, float2(-44.0, 17.4), float2(-44.0, 23.0)));
d = min(d, abs(length(p - float2(54.0, 42.0)) - 5.0));
d = min(d, abs(length(p - float2(54.0, 0.0)) - 17.2));
d = min(d, w.seg(p, float2(54.0, 19.4), float2(54.0, 37.0)));
float lane = Floor * exp(-(d * d) / max(LaneWidth * LaneWidth, 1e-3)) * LaneAmount;
c = lerp(c, lerp(c, LaneColor.rgb, 0.35) * 1.06, lane);
r = saturate(r + 0.05 * lane);
Rough = r;
return c;
"""


def custom(g, code, inputs, extra):
    """A Custom node on materials.Graph: inputs [(name, expression)], extra [(name, CMOT)] (output 0 is float3)."""
    n = g.node(unreal.MaterialExpressionCustom)
    n.set_editor_property("code", code)
    n.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
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
    n.set_editor_property("additional_outputs", outs)
    for name, src in inputs:
        g.link(src, n, name)
    return n


def tops(g, name, values):
    e = g.node(unreal.MaterialExpressionVectorParameter, parameter_name=name,
               default_value=unreal.LinearColor(values[0], values[1], values[2], 0.0))
    MAT.set_quiet(e, "group", "Weather", f"{g.name}.{name}")
    return e


_GRIME = MAT.grime


def weathered_grime(g, proj, c, rough, foot_default=0.0, dust_default=0.0):
    """materials.grime (the wall foot, the dust on ledges), then the weather (see WEATHER_HLSL)."""
    c, rough = _GRIME(g, proj, c, rough, foot_default, dust_default)
    p = proj["p"]
    h = MAT.height_above_floor(g, g.mask(p, "B"))
    s = lambda name, v, lo=0.0, hi=1.0: g.scalar(name, v, "Weather", lo, hi)  # noqa: E731
    inputs = [("C", c), ("R", rough), ("P", p), ("N", proj["n"]), ("Depth", g.node(unreal.MaterialExpressionPixelDepth)), ("U", proj["u"]), ("Floor", proj["floor"]), ("H", h),
              ("TA", tops(g, "StreakTopsA", (0.0, 0.0, 0.0))), ("TB", tops(g, "StreakTopsB", (0.0, 0.0, 0.0))),
              ("TC", tops(g, "StreakTopsC", (0.0, 0.0, 0.0))),
              ("Fall", s("StreakFall", 2.5, 0.1, 12)), ("Width", s("StreakWidth", 0.09, 0.01, 1)),
              ("StreakAmount", s("StreakAmount", 0.0)), ("SootAmount", s("SootAmount", 0.0)),
              ("StreakTint", g.vector("StreakTint", (0.52, 0.5, 0.47), "Weather")), ("WashAmount", s("WashAmount", 0.0)),
              ("DampAmount", s("DampAmount", 0.0)), ("DampHeight", s("DampHeight", 0.5, 0, 3)),
              ("LichenAmount", s("LichenAmount", 0.0)), ("LichenColor", g.vector("LichenColor", (0.2, 0.21, 0.16), "Weather")),
              ("WaterLineZ", s("WaterLineZ", -100.0, -100, 100)), ("WaterLineAmount", s("WaterLineAmount", 0.0)),
              ("FloorStain", s("FloorStain", 0.0)), ("LaneAmount", s("LaneAmount", 0.0)),
              ("LaneWidth", s("LaneWidth", 0.55, 0.1, 3)), ("LaneColor", g.vector("LaneColor", (0.62, 0.56, 0.47), "Weather"))]
    node = custom(g, WEATHER_HLSL, inputs, [("Rough", unreal.CustomMaterialOutputType.CMOT_FLOAT1)])
    return node, (node, "Rough")


def build_masters():
    default_tex = unreal.load_asset(f"{MAT.TEXTURES}/T_gravel") or unreal.load_asset("/Engine/EngineResources/DefaultTexture")
    MAT.grime = weathered_grime
    try:
        stone = MAT.build_stone(default_tex, "M_Ext_Stone")
        relief = MAT.build_stone(default_tex, "M_Ext_StoneRelief", relief=True)
    finally:
        MAT.grime = _GRIME
    for name, m in (("M_Ext_Stone", stone), ("M_Ext_StoneRelief", relief)):
        MAT.PARAMS[name] = MAT.parameter_names(m)
    for name in ("M_Stone", "M_StoneRelief", "M_Metal"):
        m = unreal.load_asset(f"{P.MATERIALS}/{name}")
        if m:
            MAT.PARAMS[name] = MAT.parameter_names(m)
    return stone, relief


# ---------------------------------------------------------------------------------------------- the Élan's pearl shell

PEARL_HLSL = r"""
// The Élan's dome (AElanExteriorStructure, bPearlShell): 6 cm panels of white ceramic-coated metal under a clear
// lacquer, on a sphere about Centre (m). Panels 7.5° of longitude (15° above 60° of latitude) by 6° of latitude
// (≈ 1.5 m); each its own faint tone and a hair of tilt, so the sky's reflection kinks at the joints; the joints
// hairlines (JointWidth m, no lacquer, a little dirt in them) that fade out once thinner than a pixel. Rain carries
// the grime down the shell in faint runs, heavier low on the dome where more water has passed (StreakAmount).
float3 d = P - Centre.xyz;
float r = max(length(d), 1e-3);
float3 n = d / r;
float az = atan2(n.y, n.x);
float el = asin(saturate(n.z));
float cols = el < 1.0472 ? 48.0 : 24.0;
float ua = az / 6.2831853 * cols;
float ue = el / 0.10472;
float dm = (0.5 - abs(frac(ua) - 0.5)) / cols * 6.2831853 * r * max(cos(el), 0.02);
float dp = (0.5 - abs(frac(ue) - 0.5)) * 0.10472 * r;
float dj = min(dm, dp);
float pix = max(Depth * 0.01 * 0.0008, 1e-5);
float half_ = JointWidth * 0.5;
float seen = saturate(JointWidth / pix);
float joint = (1.0 - smoothstep(half_, half_ + pix, dj)) * seen;
float2 cell = float2(floor(ua), floor(ue));
float h = frac(sin(dot(cell, float2(12.9898, 78.233))) * 43758.5453);
float h2 = frac(h * 91.37 + 0.123);
// Runs down the shell: lanes 30 cm apart at the foot, each its own length from the top.
float lane = az * r / 0.3;
float li = floor(lane);
float lh = frac(sin(li * 91.7) * 43758.5453);
float lf = abs(frac(lane) - 0.5);
float low = pow(saturate(1.0 - el / 1.5708), 1.5);
float run = step(0.55, lh) * (1.0 - smoothstep(0.08, 0.3, lf)) * low * lerp(0.4, 1.0, lh);
float grime = saturate(StreakAmount * run + DirtAmount * (0.4 + 0.6 * low) * (0.7 + 0.6 * h2));
// The pearl: a warm white, cooler and faintly lilac where it turns from the eye (its lacquer's thin film).
float3 V = normalize(Cam - P);
float fr = pow(1.0 - saturate(dot(n, V)), 3.0);
float3 c = Pearl.rgb * (1.0 + (h - 0.5) * PanelTone) * lerp(float3(1, 1, 1), float3(0.95, 0.965, 1.04), fr);
c = lerp(c, c * float3(0.72, 0.71, 0.68), grime);
c = lerp(c, c * 0.45, joint);
Rough = saturate(BaseRough + 0.05 * h2 + 0.25 * grime + 0.4 * joint);
Coat = saturate(1.0 - joint) * (1.0 - 0.5 * grime);
CoatRough = saturate(CoatRoughness + 0.03 * h + 0.1 * grime);
AO = lerp(1.0, 0.35, joint);
float3 tu = normalize(float3(-n.y, n.x, 0.0) + float3(1e-4, 0, 0));
float3 tv = cross(n, tu);
Nrm = normalize(n + (tu * (h - 0.5) + tv * (h2 - 0.5)) * (PanelTilt * 0.01745 * 2.0));
return c;
"""


def build_pearl():
    m = MAT.master("M_Ext_Pearl", unreal.MaterialShadingModel.MSM_CLEAR_COAT)
    g = MAT.Graph(m)
    s = lambda name, v, lo=0.0, hi=1.0: g.scalar(name, v, "Pearl", lo, hi)  # noqa: E731
    F1 = unreal.CustomMaterialOutputType.CMOT_FLOAT1
    F3 = unreal.CustomMaterialOutputType.CMOT_FLOAT3
    inputs = [("P", MAT.world_metres(g)), ("Cam", g.mul(g.node(unreal.MaterialExpressionCameraPositionWS), 0.01)),
              ("Depth", g.node(unreal.MaterialExpressionPixelDepth)),
              ("Centre", g.vector("Centre", (54.0, 0.0, 22.0), "Pearl")), ("Pearl", g.vector("Pearl", (0.80, 0.79, 0.76), "Pearl")),
              ("PanelTone", s("PanelTone", 0.03)), ("PanelTilt", s("PanelTilt", 0.05, 0, 1)), ("JointWidth", s("JointWidth", 0.006, 0, 0.05)),
              ("BaseRough", s("BaseRough", 0.3)), ("CoatRoughness", s("CoatRoughness", 0.07)),
              ("StreakAmount", s("StreakAmount", 0.18)), ("DirtAmount", s("DirtAmount", 0.12))]
    node = custom(g, PEARL_HLSL, inputs, [("Rough", F1), ("Coat", F1), ("CoatRough", F1), ("AO", F1), ("Nrm", F3)])
    pbr.world_normal_material(m)
    g.output(BaseColor=node, Metallic=0.0, Specular=s("Specular", 0.5), Roughness=(node, "Rough"), ClearCoat=(node, "Coat"),
             ClearCoatRoughness=(node, "CoatRough"), AmbientOcclusion=(node, "AO"), Normal=(node, "Nrm"))
    MAT.finish(m)
    return m


# ---------------------------------------------------------------------------------------------- the instances

# The ledges each building's water runs off (m above the ground; MuseePlan::Facade, ElanExterior).
FACADE_WALL_TOPS = ((11.9, 9.44, 9.08), (6.36, 5.6, 3.6), (3.2, 2.75, 2.0))        # entablatures' soffits, sills, the podium cap
FACADE_DRESS_TOPS = ((14.4, 12.15, 11.3), (10.85, 7.45, 6.5), (3.6, 3.2, 2.0))     # cornices, attic, sills, the podium cap
ELAN_TOPS = ((22.58, 20.9, 6.2), (5.48, 0.4, 0.0), (0.0, 0.0, 0.0))                 # the cornice, the plinth's coping, its socle

# Paris weather, as the references show it: soot grey under every ledge, paler washed runs, a damp foot, lichen where
# the stone stays wet; all within what a well-kept building shows (it is cleaned, not abandoned).
WEATHER = {"StreakAmount": 0.5, "SootAmount": 0.2, "StreakFall": 2.6, "StreakWidth": 0.22, "WashAmount": 0.5,
           "DampAmount": 0.26, "DampHeight": 0.5, "LichenAmount": 0.8}


def tops_vectors(t):
    return {"StreakTopsA": t[0], "StreakTopsB": t[1], "StreakTopsC": t[2]}


STONE_KW = dict(block_shift=1.0, contrast=0.9, normal=0.9, rough_influence=0.6)


def specs():
    S = MAT.spec
    W = MAT.merged
    wall = W(MAT.QUIET_DRAWN, MAT.STONE_WALL, WEATHER, ClearCoat=0.0, Specular=0.58, FootDirt=0.08, DustUp=0.06)
    out = {}
    # The façade's ashlar: the vein-cut travertine (S1) of the dress, honed and weathered; each block its own tone.
    out["MI_Ext_Ashlar"] = S("M_Ext_Stone", W(wall, Roughness=0.66, RoughnessVariation=0.08, CourseHeight=0.6, BlockLength=1.2,
                                               BlockJitter=0.25, Stagger=0.5, StaggerJitter=0.15, JointWidth=0.004, JointDarkness=0.55,
                                               BlockTone=0.2, BlockHue=0.025, MacroScale=4.0, MacroVariation=0.08, VoidOpen=1.0,
                                               VoidDarkness=0.45),
                             W({"BaseColor": lin(0xD9CDB9)}, tops_vectors(FACADE_WALL_TOPS)),
                             photo=("S1", dict(STONE_KW, contrast=0.8)), switches=MAT.MACRO)
    # The dressings, shafts, podium and steps: the weathered honed limestone (S3), no coat.
    dress = W(wall, Roughness=0.68, RoughnessVariation=0.08, CourseHeight=0.6, BlockLength=1.8, JointWidth=0.003, JointDarkness=0.6,
              BlockTone=0.14, BlockHue=0.02, MacroScale=4.0, MacroVariation=0.07)
    out["MI_Ext_Dressing"] = S("M_Ext_Stone", dress, W({"BaseColor": lin(0xD6CCBA)}, tops_vectors(FACADE_DRESS_TOPS)),
                               photo=("S3", STONE_KW), switches=MAT.MACRO)
    out["MI_Ext_Shaft"] = S("M_Ext_Stone", W(dress, **MAT.NO_JOINTS, BlockTone=0.0, SlabTilt=0.0),
                            W({"BaseColor": lin(0xD8CEBD)}, tops_vectors(FACADE_DRESS_TOPS)), photo=("S3", STONE_KW))
    out["MI_Ext_Carved"] = S("M_Ext_Stone", W(dress, **MAT.NO_JOINTS, **MAT.UNBROKEN, BlockTone=0.0, SlabTilt=0.0, Waviness=0.0,
                                                Roughness=0.64, DustUp=0.1, StreakAmount=0.36, SootAmount=0.18),
                             W({"BaseColor": lin(0xDCD3C3)}, tops_vectors(FACADE_DRESS_TOPS)), photo=("S3", dict(STONE_KW, contrast=0.7)))
    # The podium and steps' greyer base stone, the parvis and the kerbs: sawn and worn outdoor paving.
    floor = W(MAT.QUIET_DRAWN, MAT.STONE_FLOOR, WEATHER, ClearCoat=0.0, Specular=0.56, Roughness=0.7, RoughnessVariation=0.1,
              FloorSlab=0.8, FloorAspect=1.5, FloorStagger=0.5, JointWidth=0.006, JointDarkness=0.5, JointColorAmount=0.55,
              BlockTone=0.2, BlockHue=0.03, SlabTilt=0.45, RoughWear=0.1, TrafficWear=0.9, JointBevel=0.003,
              FloorStain=0.5, LaneAmount=0.25, LaneWidth=0.9, FootDirt=0.1)
    joint = {"JointColor": lin(0x3E3A30)}
    out["MI_Ext_Base"] = S("M_Ext_Stone", W(floor, CourseHeight=0.6, BlockLength=1.4, WallJoints=1.0),
                           W({"BaseColor": lin(0xB9B1A4)}, joint, tops_vectors(FACADE_DRESS_TOPS)), photo=("S3", STONE_KW), switches=MAT.MACRO)
    out["MI_Ext_Paving"] = S("M_Ext_Stone", W(floor, BlockTone=0.14), W({"BaseColor": lin(0xC4BBAA)}, joint), photo=("S3", STONE_KW), switches=MAT.MACRO)
    # Kerbs and copings: long single stones, their tops worn; the canal's with a water line.
    kerb = W(floor, CourseHeight=0.3, BlockLength=1.2, FloorSlab=0.5, FloorAspect=2.4, FloorStagger=0.5, LaneAmount=0.0,
             FloorStain=0.3, JointColorAmount=0.3, SlabTilt=0.3)
    out["MI_Ext_Kerb"] = S("M_Ext_Stone", W(kerb, WaterLineZ=0.30, WaterLineAmount=0.8),
                           W({"BaseColor": lin(0xCFC5B2)}, joint), photo=("S3", STONE_KW), switches=MAT.MACRO)
    # The basins' beds: dark stone under water.
    out["MI_Ext_BasinBed"] = S("M_Ext_Stone", W(MAT.QUIET_DRAWN, ClearCoat=0.0, Roughness=0.55, FloorSlab=1.0, JointWidth=0.004,
                                                 JointDarkness=0.7, BlockTone=0.15, LichenAmount=0.0, FloorStain=0.6, MacroVariation=0.12),
                               {"BaseColor": lin(0x3A3a30)}, photo=("G5", dict(block_shift=1.0, normal=0.8, rough_influence=0.5)))
    # The gravel: the Paris gardens' pale sable (a beige grit with small stones), not the Chinese court's grey pebbles:
    # Gravel022 at 0.7 m (finer), its colours quietened towards the beige, trodden paler along the walks.
    out["MI_Ext_Gravel"] = S("M_Ext_StoneRelief", W(MAT.IMAGE, ClearCoat=0.0, Roughness=0.9, MacroScale=3.0, MacroVariation=0.12,
                                                     FloorStain=0.35, LaneAmount=0.75, LaneWidth=0.55, FootDirt=0.0, LichenAmount=0.0),
                             {"BaseColor": lin(0xBDAF94), "LaneColor": lin(0xD6CBB2)},
                             photo=("G3", dict(scale=0.7, colour_amount=0.7, contrast=1.0, normal=1.0, relief_cm=0.6, rough_influence=0.4)))
    # The Élan's socle and cornice: the museum's travertine, honed, weathered as the dress.
    out["MI_Ext_Elan"] = S("M_Ext_Stone", W(wall, Roughness=0.64, CourseHeight=0.6, BlockLength=1.8, JointWidth=0.003,
                                            JointDarkness=0.62, BlockTone=0.16, MacroVariation=0.06, VoidOpen=1.0, VoidDarkness=0.5),
                           W({"BaseColor": lin(0xDCD2C0)}, tops_vectors(ELAN_TOPS)), photo=("S1", dict(STONE_KW, contrast=0.7)),
                           switches=MAT.MACRO)
    # The lamp standards: Paris's cast iron, painted the gardens' dark bronze-green (the Louvre's, the Luxembourg's, the
    # Panthéon's; the references), satin, the paint chalked and worn paler on the arrises, streaked where rain runs.
    out["MI_Ext_LampBronze"] = S("M_Metal", {"Roughness": 0.46, "Metallic": 0.0, "Variation": 0.2, "NoiseScale": 0.05,
                                             "PatinaAmount": 0.35},
                                 {"BaseColor": lin(0x2F332C), "PatinaColor": lin(0x55594C)},
                                 photo=("M1", dict(colour_amount=0.3, normal=0.6, rough_influence=0.6)))
    return out


def make_instances():
    if not EAL.does_directory_exist(FOLDER):
        EAL.make_directory(FOLDER)
    made = {}
    for name, s in specs().items():
        parent = unreal.load_asset(f"{P.MATERIALS}/{s['parent']}")
        if parent is None:
            warn(f"{name}: parent {s['parent']} missing")
            continue
        mi = MAT.material_instance(name, FOLDER, parent, s, {})
        if mi:
            made[name] = mi
    pearl = build_pearl()
    mi = MAT.material_instance("MI_Ext_Pearl", FOLDER, pearl, MAT.spec("M_Ext_Pearl"), {})
    if mi:
        made["MI_Ext_Pearl"] = mi
    log(f"instances: {', '.join(made)}")
    return made


# ---------------------------------------------------------------------------------------------- the actors

def soft(name):
    return unreal.SoftObjectPath(f"{FOLDER}/{name}.{name}")


ASSIGN = {
    "MuseeFacadeStructure": {"ashlar_material": "MI_Ext_Ashlar", "stone_material": "MI_Ext_Dressing", "shaft_material": "MI_Ext_Shaft",
                             "carved_material": "MI_Ext_Carved", "base_material": "MI_Ext_Base", "gravel_material": "MI_Ext_Gravel",
                             "bronze_material": None},
    "MuseeLandscape": {"paving_material": "MI_Ext_Paving", "gravel_material": "MI_Ext_Gravel", "kerb_material": "MI_Ext_Kerb",
                       "basin_material": "MI_Ext_BasinBed", "lamp_material": "MI_Ext_LampBronze"},
    "ElanExteriorStructure": {"stone_material": "MI_Ext_Elan", "moulding_material": "MI_Ext_Elan", "shell_material": "MI_Ext_Pearl"},
}


def assign():
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    lib = unreal.MuseeBakeLibrary
    done = 0
    for a in eas.get_all_level_actors():
        cls = a.get_class().get_name()
        if cls not in ASSIGN:
            continue
        baked = lib.is_baked(a)
        if baked:
            lib.unbake_actor(a)
        for prop, mi in ASSIGN[cls].items():
            if not mi:
                continue
            asset = unreal.load_asset(f"{FOLDER}/{mi}")
            if asset is None:
                warn(f"{mi} missing")
                continue
            try:
                a.set_editor_property(prop, asset)
            except Exception:  # noqa: BLE001 - a soft pointer may want the path
                try:
                    a.set_editor_property(prop, soft(mi))
                except Exception as e:  # noqa: BLE001
                    warn(f"{a.get_actor_label()}.{prop}: {e}")
        # (A property change reruns the construction with the new materials.)
        if baked:
            n = lib.bake_actor(a, "")
            log(f"{a.get_actor_label()} ({cls}): materials set, rebaked ({n} components)")
        else:
            log(f"{a.get_actor_label()} ({cls}): materials set (not baked)")
        done += 1
    return done


def main(args):
    build_masters()
    make_instances()
    if "assign" in args:
        les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        les.load_level(P.MAP_PATH)
        n = assign()
        les.save_current_level()
        log(f"{n} actors assigned; map saved")
    EAL.save_directory(FOLDER, only_if_is_dirty=True, recursive=True)
    if MAT.WARNINGS:
        log(f"{len(MAT.WARNINGS)} warnings from materials.py's checks (see above)")
    log("done")


if __name__ == "__main__":
    main(sys.argv[1:])
