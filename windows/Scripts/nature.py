"""
The living plants: native trees, water lilies, the Chinese garden's plants and rock, the meadow
(run after the wings are imported and relit; it is idempotent):

    UnrealEditor-Cmd.exe MuseeVision.uproject -run=pythonscript -script="Scripts/nature.py"

Options (after the script's name, inside the quotes):
    --term N          the Chinese garden's solar term, 0 (Lichun) … 23 (Dahan); default 9 (Xiazhi)
    --season NAME     the Hall of Light's season: spring, summer, autumn, winter; default summer
    --today           the plants follow today's solar term when play begins (bFollowToday)
    --materials-only  only (re)make the materials
    --no-materials    only place the plants

The defaults are the export moment of usd/ (21 June 2026, noon UTC, 40° N: summer, Xiazhi), so the
garden shows what the Swift build chose for it.

1. Materials, in /Game/Museum/Nature (no textures: leaf and petal outlines, veins, bark and stone
   are drawn in the materials):
   - M_NatureBark: bark by UV metres (u round, v along): mottling, fissures, lenticels, peeling
     bands, the birch's dark foot (by height, UV3.x), twig colour (UV3.y); a matching normal.
   - M_NatureLeaf: masked, two-sided foliage shading (light glows through): a leaf outline from
     UV (skew, fullness, serration, petiole, acuminate tip) or a flower of N lobes (notch, centre);
     midrib and veins; vertex colour (sRGB) × detail; vertex alpha = occlusion.
   - M_NaturePetal: opaque two-sided foliage for solid petals, pads, strap leaves and grass: veins,
     a base tint, an underside colour (lily pads are red beneath), an optional glow.
   - M_NatureSolid: vertex colour, for fruit, pots, soil, the lotus seed head.
   - M_NatureRock: the Taihu limestone by world position (grain, pores, rain streaks, bumps).
   - M_VelariumDappled: the Nymphéas oval's velarium (as MI_velarium) with soft, moving leaf
     shadows, as if trees stood in the sun above it.
   Wind is a world-position offset in every plant material: each vertex carries how far it bends
   (UV1.x, from the ground to the twig tips), its leaf flutter (UV1.y) and phases (UV2); the
   strength, speed and direction are in MPC_Wind, shared by all, so leaves stay on their twigs.
2. The map: hides the stand-ins from the export (trees, meadow flowers, bamboo, lotus, the rock,
   pots, the lily pond's pads and flowers; by prim path) and places the native plants at the plan's
   positions: AMuseeTree, AMuseeWaterLilies (attached to /Museum/Reserve/Lily_pond so they rise
   with it), AMuseePottedPlant, AMuseeTaihuRock, AMuseeMeadow. They are tagged "musee.building",
   "musee.wing:<Wing>" and "musee.nature" (re-importing a wing removes them: run this again).
3. The grounds (place_grounds): the trees of AMuseeLandscape.get_tree_placements() round the museum's exterior,
   tagged "musee.wing:Exterior".
"""
import math
import os
import re
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import musee_paths as P  # noqa: E402

NATURE = P.MUSEUM_CONTENT + "/Nature"
MEL = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

EXPORT_TERM = 9          # Xiazhi, the summer solstice (Ephemeris.solarTerm at 2026-06-21 12:00 UTC)


def todays_term():
    """Today's solar term, 0 立春 … 23 大寒 (the sun's ecliptic longitude; Meeus, low precision), as the game's."""
    import datetime
    n = (datetime.datetime.utcnow() - datetime.datetime(2000, 1, 1, 12)).total_seconds() / 86400.0
    g = math.radians((357.528 + 0.9856003 * n) % 360)
    lam = (280.460 + 0.9856474 * n + 1.915 * math.sin(g) + 0.020 * math.sin(2 * g)) % 360
    return int(((lam - 315) % 360) // 15)
EXPORT_SEASON = "summer"  # Season.now: term 9 / 6 = summer


def log(msg):
    unreal.log(f"[nature] {msg}")


def warn(msg):
    unreal.log_warning(f"[nature] {msg}")


def lin(hex_rgb):
    """0xRRGGBB (sRGB) → a linear LinearColor."""
    def c(v):
        v /= 255.0
        return v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4
    return unreal.LinearColor(c((hex_rgb >> 16) & 255), c((hex_rgb >> 8) & 255), c(hex_rgb & 255), 1.0)


def rgb(r, g, b):
    return unreal.LinearColor(r, g, b, 1.0)


# ---------------------------------------------------------------------------------------------
# HLSL for the Custom expressions (set up through UMuseeNatureEditorLibrary, since Python can't
# make a Custom expression's inputs).

NOISE = r"""
struct FMuseeNoise
{
    float Hash(float2 P)
    {
        float3 Q = frac(float3(P.xyx) * 0.1031);
        Q += dot(Q, Q.yzx + 33.33);
        return frac((Q.x + Q.y) * Q.z);
    }
    float Hash3(float3 P)
    {
        float3 Q = frac(P * 0.1031);
        Q += dot(Q, Q.zyx + 31.32);
        return frac((Q.x + Q.y) * Q.z);
    }
    float2 Hash22(float2 P)
    {
        float3 Q = frac(float3(P.xyx) * float3(0.1031, 0.1030, 0.0973));
        Q += dot(Q, Q.yzx + 33.33);
        return frac((Q.xx + Q.yz) * Q.zy);
    }
    float Noise(float2 P)
    {
        float2 I = floor(P);
        float2 F = frac(P);
        F = F * F * (3.0 - 2.0 * F);
        float A = Hash(I), B = Hash(I + float2(1.0, 0.0)), C = Hash(I + float2(0.0, 1.0)), D = Hash(I + float2(1.0, 1.0));
        return lerp(lerp(A, B, F.x), lerp(C, D, F.x), F.y);
    }
    float Noise3(float3 P)
    {
        float3 I = floor(P);
        float3 F = frac(P);
        F = F * F * (3.0 - 2.0 * F);
        float A = Hash3(I), B = Hash3(I + float3(1, 0, 0)), C = Hash3(I + float3(0, 1, 0)), D = Hash3(I + float3(1, 1, 0));
        float E = Hash3(I + float3(0, 0, 1)), G = Hash3(I + float3(1, 0, 1)), H = Hash3(I + float3(0, 1, 1)), K = Hash3(I + float3(1, 1, 1));
        return lerp(lerp(lerp(A, B, F.x), lerp(C, D, F.x), F.y), lerp(lerp(E, G, F.x), lerp(H, K, F.x), F.y), F.z);
    }
    float Fbm(float2 P)
    {
        float S = 0.0, Amp = 0.5;
        for (int O = 0; O < 4; O++) { S += Amp * Noise(P); P = P * 2.03 + float2(17.1, 9.2); Amp *= 0.5; }
        return S / 0.9375;
    }
    float Fbm3(float3 P)
    {
        float S = 0.0, Amp = 0.5;
        for (int O = 0; O < 4; O++) { S += Amp * Noise3(P); P = P * 2.03 + float3(17.1, 9.2, 4.7); Amp *= 0.5; }
        return S / 0.9375;
    }
};
FMuseeNoise Nz;
"""

# Wind: a slow sway along the wind (with a gust envelope and a little sideways swing), scaled by
# how far each vertex bends (UV1.x), and a quick flutter of the leaves along their normals (UV1.y).
# Centimetres. Phases in UV2 (the branch's, continuous along it; the leaf's own).
WIND = r"""
float t = GameTime * Speed;
float ph = Phase.x * 6.2831853;
float sway = sin(t * 0.83 + ph) * 0.55 + sin(t * 1.91 + ph * 1.7) * 0.22 + sin(t * 0.31 + ph * 0.4) * 0.45;
float gust = 0.65 + 0.35 * sin(t * 0.117 + ph * 0.1);
float2 d2 = float2(DirX, DirY);
d2 = d2 / max(length(d2), 0.0001);
float3 d = float3(d2, 0.0);
float3 side = float3(-d2.y, d2.x, 0.0);
float b = Bend.x * Strength * gust;
float3 o = d * ((sway * 0.7 + 0.3) * b) + side * (sin(t * 1.37 + ph * 2.3) * b * 0.3);
o.z -= abs(sway) * b * 0.05;
float fp = Phase.y * 6.2831853;
float f = sin(t * 8.3 + fp) * 0.6 + sin(t * 13.1 + fp * 2.7) * 0.4;
o += Nrm * (f * Bend.y * Flutter * (0.5 + 0.5 * gust));
return o;
"""

# A leaf (Lobes 0) or a flower of Lobes petals, drawn from the card's UV (u across, v from the stalk).
LEAF_SHAPE = r"""
float m = 0.0;
if (Lobes < 0.5)
{
    float x = abs(UV.x - 0.5) * 2.0;
    if (UV.y < Petiole)
    {
        m = x < 0.09 ? 1.0 : 0.0;
    }
    else
    {
        float s = saturate((UV.y - Petiole) / max(1.0 - Petiole, 0.001));
        float w = pow(saturate(sin(3.14159265 * pow(max(s, 0.0001), Skew))), Fullness) * LeafWidth;
        w *= lerp(1.0, saturate((1.0 - s) * 3.0), Tip);
        float teeth = 0.5 + 0.5 * sin(s * Teeth * 6.2831853);
        w *= 1.0 - Serration * teeth * smoothstep(0.08, 0.35, s) * (1.0 - smoothstep(0.85, 1.0, s));
        m = saturate((w - x) * 25.0 + 0.5);
    }
}
else
{
    float2 p = (UV - 0.5) * 2.0;
    float r = length(p);
    float a = atan2(p.y, p.x);
    float c = abs(cos(a * Lobes * 0.5));
    float R = lerp(Inner, 1.0, pow(c, LobeSharp)) - Notch * pow(c, 40.0);
    m = saturate((R * 0.96 - r) * 25.0 + 0.5);
}
return m;
"""

LEAF_COLOUR = r"""
float3 c = pow(max(VC, 0.0), 2.2);
if (Lobes < 0.5)
{
    float x = abs(UV.x - 0.5) * 2.0;
    float onBlade = step(Petiole, UV.y);
    float s = saturate((UV.y - Petiole) / max(1.0 - Petiole, 0.001));
    float midrib = (1.0 - smoothstep(0.0, 0.07, x)) * onBlade;
    float lateral = pow(abs(sin((s * 7.0 - x * 2.2) * 3.14159265)), 18.0) * (1.0 - x) * onBlade;
    c *= 1.0 + Vein * (0.45 * midrib + 0.2 * lateral);
    c *= lerp(1.0, 0.82, smoothstep(0.55, 1.0, x));
    c *= lerp(0.9, 1.06, s);
}
else
{
    float2 p = (UV - 0.5) * 2.0;
    float r = length(p);
    float a = atan2(p.y, p.x);
    float centre = 1.0 - smoothstep(CentreSize * 0.75, CentreSize, r);
    float streaks = pow(abs(cos(a * 14.0)), 10.0) * smoothstep(CentreSize, 1.0, r);
    c *= 1.0 - Vein * 0.12 * streaks;
    c *= lerp(0.82, 1.0, smoothstep(CentreSize, CentreSize + 0.3, r));
    c = lerp(c, CentreColour, centre);
}
return c;
"""

PETAL_COLOUR = r"""
float3 c = pow(max(VC, 0.0), 2.2);
float lines = pow(abs(sin(UV.x * VeinCount * 3.14159265)), 4.0);
c *= 1.0 - Vein * 0.18 * lines * smoothstep(0.0, 0.2, UV.y);
c = lerp(c, c * BaseTint, BaseAmount * (1.0 - smoothstep(0.0, 0.35, UV.y)));
c = lerp(c, Underside, saturate(-Sign) * UndersideMix);
return c;
"""

DECODE = r"""
return pow(max(VC, 0.0), 2.2);
"""

BARK_FUNCTIONS = r"""
struct FBark
{
    FMuseeNoise Nz;
    float Fiss(float2 uv, float Scale)
    {
        float f = Nz.Fbm(uv * float2(Scale, Scale * 0.22) + 7.3);
        return smoothstep(0.78, 0.95, 1.0 - abs(f * 2.0 - 1.0));
    }
    float Lent(float2 uv, float Scale)
    {
        return smoothstep(0.74, 0.8, Nz.Noise(uv * float2(Scale * 2.0, Scale * 12.0) + 3.1));
    }
    float Foot(float2 uv, float Height, float BaseDark)
    {
        if (BaseDark <= 0.001) return 0.0;
        return 1.0 - smoothstep(BaseDark * 0.35, BaseDark, Height + (Nz.Noise(uv * float2(2.5, 1.0)) - 0.5) * BaseDark * 0.7);
    }
    float FootFiss(float2 uv)
    {
        return smoothstep(0.55, 0.9, 1.0 - abs(Nz.Fbm(uv * float2(9.0, 2.0)) * 2.0 - 1.0));
    }
};
"""

BARK_COLOUR = NOISE + BARK_FUNCTIONS + r"""
FBark Bk;
float mott = Nz.Fbm(UV * float2(3.0, 1.2));
float3 c = lerp(BarkColour, BarkDark, saturate((mott - 0.35) * Mottle * 2.0));
float band = smoothstep(0.6, 0.9, Nz.Noise(float2(UV.y * 5.0, UV.x * 0.5))) * Peel;
c = lerp(c, c * 1.25, band);
c = lerp(c, LenticelColour, Bk.Lent(UV, LenticelScale) * Lenticels);
c = lerp(c, BarkDark * 0.55, Bk.Fiss(UV, FissureScale) * Fissures);
float foot = Bk.Foot(UV, Growth.x, BaseDark);
c = lerp(c, lerp(BaseDarkColour, BaseDarkColour * 0.45, Bk.FootFiss(UV)), foot);
c = lerp(c, TwigColour, smoothstep(0.55, 0.95, Growth.y));
return c * pow(max(VC, 0.0), 2.2);
"""

BARK_NORMAL = NOISE + BARK_FUNCTIONS + r"""
FBark Bk;
float e = 0.003;
float2 du = float2(e, 0.0), dv = float2(0.0, e);
float h0 = -Bk.Fiss(UV, FissureScale) * Fissures - 0.35 * Bk.Lent(UV, LenticelScale) * Lenticels + 0.3 * Nz.Fbm(UV * float2(3.0, 1.2)) - Bk.Foot(UV, Growth.x, BaseDark) * Bk.FootFiss(UV);
float hu = -Bk.Fiss(UV + du, FissureScale) * Fissures - 0.35 * Bk.Lent(UV + du, LenticelScale) * Lenticels + 0.3 * Nz.Fbm((UV + du) * float2(3.0, 1.2)) - Bk.Foot(UV + du, Growth.x, BaseDark) * Bk.FootFiss(UV + du);
float hv = -Bk.Fiss(UV + dv, FissureScale) * Fissures - 0.35 * Bk.Lent(UV + dv, LenticelScale) * Lenticels + 0.3 * Nz.Fbm((UV + dv) * float2(3.0, 1.2)) - Bk.Foot(UV + dv, Growth.x, BaseDark) * Bk.FootFiss(UV + dv);
float k = NormalStrength * (1.0 - 0.8 * saturate(Growth.y));
return normalize(float3(-(hu - h0) / e * k, -(hv - h0) / e * k, 1.0));
"""

ROCK_COLOUR = NOISE + r"""
float3 p = WP / 100.0;
float grain = Nz.Noise3(p * 40.0);
float blotch = Nz.Fbm3(p * 1.7);
float3 c = pow(max(VC, 0.0), 2.2);
c *= lerp(0.86, 1.08, blotch);
c *= lerp(1.0, 0.7, smoothstep(0.78, 0.9, grain));
float streak = smoothstep(0.55, 0.8, Nz.Noise3(float3(p.x * 6.0, p.y * 6.0, p.z * 0.6)));
c = lerp(c, c * float3(0.78, 0.76, 0.72), streak * 0.5);
return c;
"""

ROCK_NORMAL = NOISE + r"""
float3 p = WP / 100.0;
float e = 0.004;
float h0 = Nz.Fbm3(p * 6.0) * 0.6 + Nz.Noise3(p * 30.0) * 0.25;
float hx = Nz.Fbm3((p + float3(e, 0, 0)) * 6.0) * 0.6 + Nz.Noise3((p + float3(e, 0, 0)) * 30.0) * 0.25;
float hy = Nz.Fbm3((p + float3(0, e, 0)) * 6.0) * 0.6 + Nz.Noise3((p + float3(0, e, 0)) * 30.0) * 0.25;
float hz = Nz.Fbm3((p + float3(0, 0, e)) * 6.0) * 0.6 + Nz.Noise3((p + float3(0, 0, e)) * 30.0) * 0.25;
float3 g = float3(hx - h0, hy - h0, hz - h0) / e;
g -= Nrm * dot(g, Nrm);
return normalize(Nrm - g * Bump);
"""

# Leaf shadows on the velarium: clusters of soft leaf shapes (a wide penumbra, as from trees high
# above), gathered in drifts, stirring with the wind; only when the sun is up.
DAPPLE = NOISE + r"""
float2 p = WP.xy / 100.0 * DappleScale;
float t = GameTime * 0.6;
float2 drift = float2(sin(t * 0.37), cos(t * 0.29)) * 0.05;
float canopy = smoothstep(0.35, 0.72, Nz.Noise(p * 0.16 + 3.1) * 0.65 + Nz.Noise(p * 0.47 + 9.2) * 0.35);
float shade = 0.0;
for (int L = 0; L < 2; L++)
{
    float2 q = L == 0 ? p * 2.1 + drift : p * 3.7 - drift * 1.7 + 5.3;
    float tt = L == 0 ? t : t * 1.4;
    float2 ci = floor(q);
    float s = 0.0;
    for (int j = -1; j <= 1; j++)
    {
        for (int i = -1; i <= 1; i++)
        {
            float2 c = ci + float2(i, j);
            for (int k = 0; k < 3; k++)
            {
                float2 r = Nz.Hash22(c + float2(k * 17.31, k * 5.73));
                float2 o = c + r + 0.06 * float2(sin(tt + r.x * 6.28), cos(tt * 1.3 + r.y * 6.28));
                float a = r.x * 6.2831853 + 0.25 * sin(tt * 0.8 + r.y * 9.0);
                float2 dd = q - o;
                float2 e = float2(cos(a) * dd.x + sin(a) * dd.y, -sin(a) * dd.x + cos(a) * dd.y);
                e /= float2(0.34 + 0.2 * r.y, (0.34 + 0.2 * r.y) * 0.42);
                s = max(s, 1.0 - smoothstep(0.45, 1.0, length(e)));
            }
        }
    }
    shade = max(shade, s * (L == 0 ? 1.0 : 0.8));
}
shade = saturate(shade * canopy + canopy * 0.18);
return 1.0 - DappleStrength * shade * saturate(Daylight);
"""

# The grounds' mown lawn (AMuseeLawn's blades): the blade's colour (vertex colour, sRGB; darker and yellower down in
# the sward), drifting over metres as a real lawn does (a little lusher or leaner, here and there yellower), a pale
# midrib, a paler underside.
LAWN_COLOUR = NOISE + r"""
float3 c = pow(max(VC, 0.0), 2.2);
float2 p = WP.xy / 100.0;
float big = Nz.Fbm(p / MacroScale);
float blot = Nz.Fbm(p / (MacroScale * 0.3) + 7.7);
c *= lerp(1.0 - MacroValue, 1.0 + MacroValue, big);
float yellow = smoothstep(0.55, 0.85, blot * 0.65 + big * 0.35) * YellowAmount;
c = lerp(c, c * float3(1.2, 1.03, 0.6), yellow);
float x = abs(UV.x - 0.5) * 2.0;
c *= 1.0 + 0.07 * (1.0 - smoothstep(0.0, 0.3, x)) * UV.y;
c = lerp(c, c * UndersideTint.rgb, saturate(-Sign) * UndersideMix);
return c * Tint.rgb;
"""

# A blade's roughness: the waxy upper blade glossier than the sheath down in the turf (the sheen a mown lawn shows
# towards the sun).
LAWN_ROUGHNESS = r"""
return lerp(RoughnessRoot, Roughness, smoothstep(0.1, 0.8, UV.y));
"""

# The blades are kept out of the ray-traced scene, so nothing occludes the sky's reflection down in the sward: its
# specular fades there (the tips keep their sheen).
LAWN_SPECULAR = r"""
return Specular * lerp(SpecularRoot, 1.0, smoothstep(0.35, 0.95, UV.y));
"""

# The clipped hedges' leaves (MuseeHedge: box leaves and yew needles as geometry): the leaf's colour (vertex colour,
# sRGB), a pale midrib, the paler underside; the dark core (MI_Hedge_Core) is the Tint alone (its vertex colour white).
HEDGE_COLOUR = r"""
float3 c = pow(max(VC, 0.0), 2.2);
float x = abs(UV.x - 0.5) * 2.0;
float onLeaf = step(0.02, UV.y) * (1.0 - step(0.98, UV.y));
c *= 1.0 + Midrib * (1.0 - smoothstep(0.0, 0.16, x)) * onLeaf;
c = lerp(c, c * UndersideTint.rgb, saturate(-Sign) * UndersideMix);
return c * Tint.rgb;
"""

# Glossy above (the box's waxy cuticle), matt beneath. Far off, where a pixel holds many leaves at many angles, their
# glints average into a soft sheen: the roughness rises with distance (else the sub-pixel leaves glitter).
HEDGE_ROUGHNESS = r"""
float r = lerp(Roughness, RoughnessUnder, saturate(-Sign));
float far = smoothstep(FarStart, FarEnd, Depth);
return lerp(r, max(r, FarRoughness), far);
"""


# ---------------------------------------------------------------------------------------------
# Material graphs

class Graph:
    """Builds a material's expressions, laying them out in a column."""

    def __init__(self, material):
        self.m = material
        self.row = 0

    def expr(self, cls):
        self.row += 1
        return MEL.create_material_expression(self.m, cls, -1400 - 300 * (self.row // 25), (self.row % 25) * 110)

    def scalar(self, name, value):
        e = self.expr(unreal.MaterialExpressionScalarParameter)
        e.set_editor_property("parameter_name", name)
        e.set_editor_property("default_value", float(value))
        return e

    def vector(self, name, colour):
        e = self.expr(unreal.MaterialExpressionVectorParameter)
        e.set_editor_property("parameter_name", name)
        e.set_editor_property("default_value", colour)
        return e

    def texcoord(self, index):
        e = self.expr(unreal.MaterialExpressionTextureCoordinate)
        e.set_editor_property("coordinate_index", index)
        return e

    def simple(self, cls):
        return self.expr(cls)

    def collection(self, mpc, name):
        e = self.expr(unreal.MaterialExpressionCollectionParameter)
        e.set_editor_property("collection", mpc)
        e.set_editor_property("parameter_name", name)
        return e

    def multiply(self, a, b, a_out="", b_out=""):
        e = self.expr(unreal.MaterialExpressionMultiply)
        MEL.connect_material_expressions(a, a_out, e, "A")
        MEL.connect_material_expressions(b, b_out, e, "B")
        return e

    def custom(self, code, outputs, inputs, description):
        e = self.expr(unreal.MaterialExpressionCustom)
        names = [n for n, _ in inputs]
        if not unreal.MuseeNatureEditorLibrary.configure_custom_expression(e, code, outputs, names, description):
            raise RuntimeError(f"{self.m.get_name()}: could not set up the Custom expression '{description}' (is the C++ built?)")
        for name, src in inputs:
            source, output = src if isinstance(src, tuple) else (src, "")
            if not MEL.connect_material_expressions(source, output, e, name):
                warn(f"{self.m.get_name()}: '{description}' input {name} not connected")
        return e

    def out(self, prop, expr, output=""):
        if not MEL.connect_material_property(expr, output, prop):
            warn(f"{self.m.get_name()}: could not connect {prop}")

    def wind(self, mpc, flutter):
        return self.custom(WIND, 3, [
            ("Bend", self.texcoord(1)), ("Phase", self.texcoord(2)), ("GameTime", self.simple(unreal.MaterialExpressionTime)),
            ("Nrm", self.simple(unreal.MaterialExpressionVertexNormalWS)), ("Strength", self.collection(mpc, "WindStrength")),
            ("Speed", self.collection(mpc, "WindSpeed")), ("Flutter", self.scalar("Flutter", flutter)),
            ("DirX", self.collection(mpc, "WindDirX")), ("DirY", self.collection(mpc, "WindDirY")),
        ], "Wind")


def set_prop(obj, name, value):
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception as e:  # noqa: BLE001
        warn(f"{obj.get_name()}: {name} not set ({e})")
        return False


def master(name):
    """The material, emptied (kept rather than deleted, so its instances stay valid)."""
    path = f"{NATURE}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        m = unreal.load_asset(path)
        MEL.delete_all_material_expressions(m)
    else:
        m = tools.create_asset(name, NATURE, unreal.Material, unreal.MaterialFactoryNew())
    return m


def make_wind_collection():
    """MPC_Wind: one wind for every plant (strength in cm at the twig tips, speed, direction)."""
    path = f"{NATURE}/MPC_Wind"
    mpc = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else \
        tools.create_asset("MPC_Wind", NATURE, unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
    params = []
    for n, v in [("WindStrength", 10.0), ("WindSpeed", 1.0), ("WindDirX", 0.8), ("WindDirY", 0.6)]:
        p = unreal.CollectionScalarParameter()
        p.set_editor_property("parameter_name", n)
        p.set_editor_property("default_value", v)
        params.append(p)
    mpc.set_editor_property("scalar_parameters", params)
    unreal.EditorAssetLibrary.save_loaded_asset(mpc)
    return mpc


def make_bark(mpc):
    m = master("M_NatureBark")
    set_prop(m, "two_sided", False)
    g = Graph(m)
    uv, growth, vc = g.texcoord(0), g.texcoord(3), g.simple(unreal.MaterialExpressionVertexColor)
    p = {n: g.scalar(n, v) for n, v in [("Lenticels", 0.5), ("LenticelScale", 8.0), ("Fissures", 0.5), ("FissureScale", 8.0),
                                         ("Mottle", 0.6), ("BaseDark", 0.0), ("Peel", 0.0), ("NormalStrength", 0.003)]}
    v = {n: g.vector(n, c) for n, c in [("BarkColour", lin(0x6E6254)), ("BarkDark", lin(0x3E362E)), ("TwigColour", lin(0x5A4232)),
                                         ("LenticelColour", lin(0x2B2522)), ("BaseDarkColour", lin(0x3A342E))]}
    colour = g.custom(BARK_COLOUR, 3, [("UV", uv), ("Growth", growth), ("VC", vc)] + [(n, v[n]) for n in v]
                      + [(n, p[n]) for n in ("Lenticels", "LenticelScale", "Fissures", "FissureScale", "Mottle", "BaseDark", "Peel")], "Bark")
    normal = g.custom(BARK_NORMAL, 3, [("UV", uv), ("Growth", growth)]
                      + [(n, p[n]) for n in ("Lenticels", "LenticelScale", "Fissures", "FissureScale", "BaseDark", "NormalStrength")], "Bark normal")
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, colour)
    g.out(unreal.MaterialProperty.MP_NORMAL, normal)
    g.out(unreal.MaterialProperty.MP_ROUGHNESS, g.scalar("Roughness", 0.8))
    g.out(unreal.MaterialProperty.MP_SPECULAR, g.scalar("Specular", 0.35))
    g.out(unreal.MaterialProperty.MP_AMBIENT_OCCLUSION, vc, "A")
    g.out(unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET, g.wind(mpc, 0.0))
    MEL.recompile_material(m)
    return m


def make_leaf(mpc):
    m = master("M_NatureLeaf")
    set_prop(m, "shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    set_prop(m, "blend_mode", unreal.BlendMode.BLEND_MASKED)
    set_prop(m, "two_sided", True)
    set_prop(m, "opacity_mask_clip_value", 0.5)
    g = Graph(m)
    uv, vc = g.texcoord(0), g.simple(unreal.MaterialExpressionVertexColor)
    shape = [("Lobes", 0.0), ("Skew", 0.8), ("Fullness", 0.7), ("LeafWidth", 0.95), ("Serration", 0.05), ("Teeth", 12.0),
             ("Notch", 0.0), ("Petiole", 0.1), ("LobeSharp", 1.0), ("Inner", 0.3), ("Tip", 0.5)]
    p = {n: g.scalar(n, val) for n, val in shape}
    mask = g.custom(LEAF_SHAPE, 1, [("UV", uv)] + [(n, p[n]) for n, _ in shape], "Leaf shape")
    colour = g.custom(LEAF_COLOUR, 3, [("UV", uv), ("VC", vc), ("Lobes", p["Lobes"]), ("Petiole", p["Petiole"]),
                                       ("CentreColour", g.vector("CentreColour", lin(0xE8C840))), ("CentreSize", g.scalar("CentreSize", 0.2)),
                                       ("Vein", g.scalar("Vein", 0.5))], "Leaf colour")
    g.out(unreal.MaterialProperty.MP_OPACITY_MASK, mask)
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, colour)
    g.out(unreal.MaterialProperty.MP_SUBSURFACE_COLOR, g.multiply(colour, g.vector("SubsurfaceTint", rgb(1.0, 1.1, 0.6))))
    g.out(unreal.MaterialProperty.MP_ROUGHNESS, g.scalar("Roughness", 0.5))
    g.out(unreal.MaterialProperty.MP_SPECULAR, g.scalar("Specular", 0.4))
    g.out(unreal.MaterialProperty.MP_AMBIENT_OCCLUSION, vc, "A")
    g.out(unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET, g.wind(mpc, 1.0))
    MEL.recompile_material(m)
    return m


def make_petal(mpc):
    m = master("M_NaturePetal")
    set_prop(m, "shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    set_prop(m, "two_sided", True)
    g = Graph(m)
    uv, vc = g.texcoord(0), g.simple(unreal.MaterialExpressionVertexColor)
    colour = g.custom(PETAL_COLOUR, 3, [
        ("UV", uv), ("VC", vc), ("Sign", g.simple(unreal.MaterialExpressionTwoSidedSign)), ("Vein", g.scalar("Vein", 0.5)),
        ("VeinCount", g.scalar("VeinCount", 8.0)), ("BaseTint", g.vector("BaseTint", rgb(1, 1, 1))), ("BaseAmount", g.scalar("BaseAmount", 0.0)),
        ("Underside", g.vector("Underside", lin(0x5E7A3A))), ("UndersideMix", g.scalar("UndersideMix", 0.0)),
    ], "Petal colour")
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, colour)
    g.out(unreal.MaterialProperty.MP_SUBSURFACE_COLOR, g.multiply(colour, g.vector("SubsurfaceTint", rgb(1.0, 1.0, 0.9))))
    g.out(unreal.MaterialProperty.MP_EMISSIVE_COLOR, g.multiply(colour, g.scalar("Glow", 0.0)))
    g.out(unreal.MaterialProperty.MP_ROUGHNESS, g.scalar("Roughness", 0.5))
    g.out(unreal.MaterialProperty.MP_SPECULAR, g.scalar("Specular", 0.4))
    g.out(unreal.MaterialProperty.MP_AMBIENT_OCCLUSION, vc, "A")
    g.out(unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET, g.wind(mpc, 0.5))
    MEL.recompile_material(m)
    return m


def make_lawn():
    """M_NatureLawn: the mown lawn's blades (AMuseeLawn: Nanite, instanced). Two-sided foliage, so a blade against
    the low sun glows through; opaque and still (no wind: 3–6 cm of mown grass barely stirs, and without a
    world-position offset Nanite draws the sward on its fastest path)."""
    m = master("M_NatureLawn")
    set_prop(m, "shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    set_prop(m, "two_sided", True)
    g = Graph(m)
    uv, vc = g.texcoord(0), g.simple(unreal.MaterialExpressionVertexColor)
    colour = g.custom(LAWN_COLOUR, 3, [
        ("UV", uv), ("VC", vc), ("WP", g.simple(unreal.MaterialExpressionWorldPosition)),
        ("Sign", g.simple(unreal.MaterialExpressionTwoSidedSign)), ("MacroScale", g.scalar("MacroScale", 14.0)),
        ("MacroValue", g.scalar("MacroValue", 0.1)), ("YellowAmount", g.scalar("YellowAmount", 0.25)),
        ("UndersideTint", g.vector("UndersideTint", rgb(1.03, 1.03, 1.0))), ("UndersideMix", g.scalar("UndersideMix", 0.5)),
        ("Tint", g.vector("Tint", rgb(1, 1, 1))),
    ], "Lawn colour")
    rough = g.custom(LAWN_ROUGHNESS, 1, [("UV", uv), ("Roughness", g.scalar("Roughness", 0.5)),
                                         ("RoughnessRoot", g.scalar("RoughnessRoot", 0.65))], "Lawn roughness")
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, colour)
    # A thin blade passes a little yellow-green light (well under what it reflects).
    g.out(unreal.MaterialProperty.MP_SUBSURFACE_COLOR, g.multiply(colour, g.vector("SubsurfaceTint", rgb(0.55, 0.65, 0.28))))
    spec = g.custom(LAWN_SPECULAR, 1, [("UV", uv), ("Specular", g.scalar("Specular", 0.4)),
                                       ("SpecularRoot", g.scalar("SpecularRoot", 0.2))], "Lawn specular")
    g.out(unreal.MaterialProperty.MP_ROUGHNESS, rough)
    g.out(unreal.MaterialProperty.MP_SPECULAR, spec)
    g.out(unreal.MaterialProperty.MP_AMBIENT_OCCLUSION, vc, "A")
    MEL.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m)
    return m


def make_lawn_material():
    """M_NatureLawn and its instance MI_Lawn_Blade (Scripts/lawn.py; make_materials makes them too). The blades' wax
    (F0 0.04): glossy at the tips, matt down the sheath; less of the thin blade's transmitted light than before, and
    only a touch of the yellowing drift (it washed the sward out)."""
    unreal.EditorAssetLibrary.make_directory(NATURE)
    m = make_lawn()
    return instance("MI_Lawn_Blade", m, {"Roughness": 0.42, "RoughnessRoot": 0.7, "Specular": 0.5, "SpecularRoot": 0.15,
                                         "YellowAmount": 0.1, "MacroValue": 0.12},
                    {"SubsurfaceTint": rgb(0.22, 0.42, 0.1)})


def make_hedge():
    """M_NatureHedge: the clipped hedges' leaves (MuseeHedge, Nanite, instanced) and their dark cores. Two-sided
    foliage, opaque, still (no world-position offset: clipped box and yew hardly stir)."""
    m = master("M_NatureHedge")
    set_prop(m, "shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    set_prop(m, "two_sided", True)
    g = Graph(m)
    uv, vc = g.texcoord(0), g.simple(unreal.MaterialExpressionVertexColor)
    sign = g.simple(unreal.MaterialExpressionTwoSidedSign)
    colour = g.custom(HEDGE_COLOUR, 3, [
        ("UV", uv), ("VC", vc), ("Sign", sign), ("Midrib", g.scalar("Midrib", 0.15)),
        ("UndersideTint", g.vector("UndersideTint", rgb(2.4, 1.7, 2.6))), ("UndersideMix", g.scalar("UndersideMix", 1.0)),
        ("Tint", g.vector("Tint", rgb(1, 1, 1))),
    ], "Hedge colour")
    rough = g.custom(HEDGE_ROUGHNESS, 1, [("Sign", sign), ("Roughness", g.scalar("Roughness", 0.3)),
                                          ("RoughnessUnder", g.scalar("RoughnessUnder", 0.55)),
                                          ("Depth", g.simple(unreal.MaterialExpressionPixelDepth)),
                                          ("FarStart", g.scalar("FarStart", 500.0)), ("FarEnd", g.scalar("FarEnd", 2500.0)),
                                          ("FarRoughness", g.scalar("FarRoughness", 0.62))], "Hedge roughness")
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, colour)
    g.out(unreal.MaterialProperty.MP_SUBSURFACE_COLOR, g.multiply(colour, g.vector("SubsurfaceTint", rgb(0.3, 0.4, 0.12))))
    g.out(unreal.MaterialProperty.MP_ROUGHNESS, rough)
    g.out(unreal.MaterialProperty.MP_SPECULAR, g.scalar("Specular", 0.5))
    g.out(unreal.MaterialProperty.MP_AMBIENT_OCCLUSION, vc, "A")
    MEL.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m)
    return m


def make_hedge_materials():
    """MI_Hedge_Box (box: glossy above, pale yellow-green beneath), MI_Hedge_Yew (yew: the two pale stomatal bands
    beneath) and MI_Hedge_Core (the bare twigs in the shade inside a clipped hedge) for Scripts/lawn.py hedges."""
    unreal.EditorAssetLibrary.make_directory(NATURE)
    m = make_hedge()
    instance("MI_Hedge_Box", m, {"Roughness": 0.3, "RoughnessUnder": 0.55, "Specular": 0.5, "Midrib": 0.18},
             {"UndersideTint": rgb(2.4, 1.7, 2.6), "SubsurfaceTint": rgb(0.3, 0.4, 0.12)})
    instance("MI_Hedge_Yew", m, {"Roughness": 0.46, "RoughnessUnder": 0.62, "Specular": 0.42, "Midrib": 0.08},
             {"UndersideTint": rgb(1.9, 1.55, 1.7), "SubsurfaceTint": rgb(0.25, 0.32, 0.1)})
    return instance("MI_Hedge_Core", m, {"Roughness": 0.85, "RoughnessUnder": 0.85, "Specular": 0.35, "Midrib": 0.0, "UndersideMix": 0.0},
                    {"Tint": rgb(0.016, 0.02, 0.009), "SubsurfaceTint": rgb(0.0, 0.0, 0.0)})


def make_solid(mpc):
    m = master("M_NatureSolid")
    g = Graph(m)
    vc = g.simple(unreal.MaterialExpressionVertexColor)
    colour = g.custom(DECODE, 3, [("VC", vc)], "sRGB vertex colour")
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, colour)
    g.out(unreal.MaterialProperty.MP_ROUGHNESS, g.scalar("Roughness", 0.4))
    g.out(unreal.MaterialProperty.MP_SPECULAR, g.scalar("Specular", 0.5))
    g.out(unreal.MaterialProperty.MP_AMBIENT_OCCLUSION, vc, "A")
    g.out(unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET, g.wind(mpc, 0.0))
    MEL.recompile_material(m)
    return m


def make_rock():
    m = master("M_NatureRock")
    set_prop(m, "tangent_space_normal", False)
    g = Graph(m)
    vc, wp = g.simple(unreal.MaterialExpressionVertexColor), g.simple(unreal.MaterialExpressionWorldPosition)
    nrm = g.simple(unreal.MaterialExpressionVertexNormalWS)
    colour = g.custom(ROCK_COLOUR, 3, [("WP", wp), ("VC", vc)], "Limestone")
    normal = g.custom(ROCK_NORMAL, 3, [("WP", wp), ("Nrm", nrm), ("Bump", g.scalar("Bump", 0.004))], "Limestone normal (world)")
    g.out(unreal.MaterialProperty.MP_BASE_COLOR, colour)
    g.out(unreal.MaterialProperty.MP_NORMAL, normal)
    g.out(unreal.MaterialProperty.MP_ROUGHNESS, g.scalar("Roughness", 0.88))
    g.out(unreal.MaterialProperty.MP_SPECULAR, g.scalar("Specular", 0.4))
    g.out(unreal.MaterialProperty.MP_AMBIENT_OCCLUSION, vc, "A")
    MEL.recompile_material(m)
    return m


def make_velarium(mpc_wind):
    """M_VelariumDappled / MI_Velarium_Dappled: M_Daylit's velarium (setup_project.py) with leaf shadows."""
    texture = unreal.load_asset(f"{P.MUSEUM_CONTENT}/Textures/T_velarium") \
        if unreal.EditorAssetLibrary.does_asset_exist(f"{P.MUSEUM_CONTENT}/Textures/T_velarium") else None
    daylight_mpc = unreal.load_asset(f"{P.MATERIALS}/MPC_Musee") if unreal.EditorAssetLibrary.does_asset_exist(f"{P.MATERIALS}/MPC_Musee") else None
    if not texture or not daylight_mpc:
        warn("T_velarium or MPC_Musee is missing (run setup_project.py): no dappled velarium")
        return None
    m = master("M_VelariumDappled")
    set_prop(m, "shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    set_prop(m, "two_sided", True)
    g = Graph(m)
    uv = g.multiply(g.texcoord(0), g.scalar("Tiling", 1.0))
    tex = g.expr(unreal.MaterialExpressionTextureSampleParameter2D)
    tex.set_editor_property("parameter_name", "Image")
    tex.set_editor_property("texture", texture)
    MEL.connect_material_expressions(uv, "", tex, "UVs")
    lit = g.multiply(tex, g.vector("Tint", rgb(1, 1, 1)), "RGB", "")
    nits = g.multiply(lit, g.scalar("Luminance", 1800.0))
    daylight = g.collection(daylight_mpc, "Daylight")
    floor = g.expr(unreal.MaterialExpressionMax)
    MEL.connect_material_expressions(daylight, "", floor, "A")
    MEL.connect_material_expressions(g.scalar("NightFloor", 0.25), "", floor, "B")
    dapple = g.custom(DAPPLE, 1, [
        ("WP", g.simple(unreal.MaterialExpressionWorldPosition)), ("GameTime", g.simple(unreal.MaterialExpressionTime)),
        ("Daylight", daylight), ("DappleStrength", g.scalar("DappleStrength", 0.42)), ("DappleScale", g.scalar("DappleScale", 1.0)),
    ], "Leaf shadows")
    glow = g.multiply(g.multiply(nits, floor), dapple)
    g.out(unreal.MaterialProperty.MP_EMISSIVE_COLOR, glow)
    MEL.recompile_material(m)
    mi = instance("MI_Velarium_Dappled", m, {"Luminance": 1800.0}, {"Tint": rgb(1, 1, 1)})
    return mi


def instance(name, parent, scalars=None, vectors=None):
    path = f"{NATURE}/{name}"
    mi = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else \
        tools.create_asset(name, NATURE, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, parent)
    MEL.clear_all_material_instance_parameters(mi)
    # (In 5.8 these setters report False even when the value is written, so their result says nothing.)
    for k, v in (scalars or {}).items():
        MEL.set_material_instance_scalar_parameter_value(mi, k, float(v))
    for k, v in (vectors or {}).items():
        MEL.set_material_instance_vector_parameter_value(mi, k, v)
    MEL.update_material_instance(mi)
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    return mi


def bark(colour, dark, twig, lenticel, lenticels, lscale, fissures, fscale, mottle, rough, normal, base_dark=0.0, base_colour=0x3A342E,
         peel=0.0, specular=0.35):
    return ({"Lenticels": lenticels, "LenticelScale": lscale, "Fissures": fissures, "FissureScale": fscale, "Mottle": mottle,
             "BaseDark": base_dark, "Peel": peel, "Roughness": rough, "NormalStrength": normal, "Specular": specular},
            {"BarkColour": lin(colour), "BarkDark": lin(dark), "TwigColour": lin(twig), "LenticelColour": lin(lenticel),
             "BaseDarkColour": lin(base_colour)})


def leaf(skew, fullness, serration, teeth, petiole, tip, vein=0.5, rough=0.5, spec=0.4, sss=(1.0, 1.1, 0.6), flutter=1.0, width=0.95):
    return ({"Lobes": 0, "Skew": skew, "Fullness": fullness, "LeafWidth": width, "Serration": serration, "Teeth": teeth, "Notch": 0,
             "Petiole": petiole, "Tip": tip, "Vein": vein, "Roughness": rough, "Specular": spec, "Flutter": flutter},
            {"SubsurfaceTint": rgb(*sss)})


def flower(lobes, sharp, inner, notch, centre, centre_size, vein=0.5, rough=0.5, sss=(1.0, 1.0, 0.95), flutter=0.8):
    return ({"Lobes": lobes, "LobeSharp": sharp, "Inner": inner, "Notch": notch, "CentreSize": centre_size, "Vein": vein,
             "Roughness": rough, "Flutter": flutter},
            {"CentreColour": lin(centre), "SubsurfaceTint": rgb(*sss)})


def petal(vein, veins, rough, underside=0x5E7A3A, under_mix=0.0, base_tint=(1, 1, 1), base_amount=0.0, glow=0.0, sss=(1.0, 1.0, 0.9),
          flutter=0.5, spec=0.4):
    return ({"Vein": vein, "VeinCount": veins, "Roughness": rough, "UndersideMix": under_mix, "BaseAmount": base_amount, "Glow": glow,
             "Flutter": flutter, "Specular": spec},
            {"Underside": lin(underside), "BaseTint": rgb(*base_tint), "SubsurfaceTint": rgb(*sss)})


def make_materials():
    unreal.EditorAssetLibrary.make_directory(NATURE)
    mpc = make_wind_collection()
    m_bark, m_leaf, m_petal, m_solid, m_rock = make_bark(mpc), make_leaf(mpc), make_petal(mpc), make_solid(mpc), make_rock()

    barks = {
        # Silver birch: white with dark horizontal lenticels, a black fissured foot, dark purple-brown twigs.
        "MI_Bark_Birch": bark(0xE9E5DC, 0xBDB6AA, 0x4A3028, 0x2B2522, 1.0, 9.0, 0.15, 5.0, 0.6, 0.6, 0.0015, base_dark=1.3, peel=0.25),
        # Cherry: glossy red-brown with pale horizontal lenticels and peeling bands.
        "MI_Bark_Cherry": bark(0x5A3A30, 0x3A2622, 0x6A3A2A, 0xA89080, 0.9, 7.0, 0.25, 6.0, 0.5, 0.5, 0.003, peel=0.6),
        # Apple: grey-brown, scaly.
        "MI_Bark_Apple": bark(0x6E6254, 0x3E362E, 0x6A5040, 0x9A8A78, 0.3, 8.0, 0.8, 20.0, 0.8, 0.85, 0.006, peel=0.3),
        # Mume: dark and deeply fissured, lichen-grey patches, green one-year shoots.
        "MI_Bark_Plum": bark(0x3E332C, 0x221C18, 0x3A4A2A, 0x6A6A58, 0.3, 6.0, 0.9, 16.0, 1.0, 0.85, 0.007),
        # Osmanthus: smooth pale grey.
        "MI_Bark_Osmanthus": bark(0x847E70, 0x5A564C, 0x6A7250, 0xA8A494, 0.5, 10.0, 0.15, 6.0, 0.6, 0.7, 0.002),
        # Bamboo: waxy green culms with fine striations (the nodes and their white rings are in the vertex colour).
        "MI_Bark_Bamboo": bark(0x6E8B3D, 0x5A7430, 0x6E8B3D, 0x6E8B3D, 0.0, 8.0, 0.05, 30.0, 0.3, 0.35, 0.0006, specular=0.5),
        # Green stems and stalks: colour from the vertex colour.
        "MI_Stem_Green": bark(0xFFFFFF, 0xDADADA, 0xFFFFFF, 0xFFFFFF, 0.0, 8.0, 0.0, 8.0, 0.3, 0.5, 0.0005),
    }
    for name, (s, v) in barks.items():
        instance(name, m_bark, s, v)

    leaves = {
        "MI_Leaf_Birch": leaf(0.55, 0.8, 0.1, 9.0, 0.12, 0.6, rough=0.55),
        "MI_Leaf_Cherry": leaf(1.1, 0.7, 0.06, 16.0, 0.14, 0.8, vein=0.6, rough=0.4),
        "MI_Leaf_Apple": leaf(0.9, 0.6, 0.05, 14.0, 0.1, 0.4, rough=0.55),
        "MI_Leaf_Plum": leaf(0.75, 0.7, 0.06, 16.0, 0.1, 0.9, rough=0.5),
        "MI_Leaf_Osmanthus": leaf(0.95, 0.55, 0.03, 10.0, 0.07, 0.5, vein=0.35, rough=0.3, spec=0.5, sss=(0.7, 0.8, 0.4)),
        "MI_Leaf_Bamboo": leaf(0.5, 0.5, 0.0, 1.0, 0.04, 1.0, vein=0.3, rough=0.45, flutter=1.4),
        "MI_Leaf_Chrysanthemum": leaf(0.9, 0.5, 0.35, 3.5, 0.15, 0.2, vein=0.6, rough=0.7),
        "MI_Leaf_Meadow": leaf(0.8, 0.6, 0.04, 10.0, 0.02, 0.8, rough=0.6),
        "MI_Blossom_Cherry": flower(5, 0.9, 0.25, 0.12, 0xC8C050, 0.18, sss=(1.1, 1.1, 1.1)),
        "MI_Blossom_Apple": flower(5, 0.6, 0.4, 0.0, 0xE8D060, 0.16, sss=(1.1, 1.05, 1.05)),
        "MI_Blossom_Plum": flower(5, 0.5, 0.45, 0.0, 0xF0D060, 0.22, sss=(1.1, 1.0, 1.0)),
        "MI_Flower_Osmanthus": flower(4, 0.8, 0.3, 0.0, 0xE08830, 0.15, sss=(1.1, 0.9, 0.6)),
        "MI_Flower_Chrysanthemum": flower(26, 2.5, 0.25, 0.02, 0xB8A040, 0.16, sss=(1.0, 0.9, 0.5), rough=0.6),
        "MI_Flower_Scabious": flower(14, 1.2, 0.55, 0.1, 0xB0A0D8, 0.35),
        "MI_Flower_Mallow": flower(5, 0.55, 0.3, 0.25, 0xF0E8E0, 0.12, vein=1.0),
        "MI_Flower_Helenium": flower(13, 1.4, 0.3, 0.12, 0x4A2A18, 0.3, sss=(1.0, 0.8, 0.5)),
        "MI_Flower_Buttercup": flower(5, 0.45, 0.5, 0.0, 0xD0B020, 0.2, rough=0.25, sss=(1.0, 0.95, 0.5)),
        "MI_Flower_Narcissus": flower(6, 0.9, 0.3, 0.02, 0xE89030, 0.16),
        "MI_Flower_Daffodil": flower(6, 0.9, 0.3, 0.0, 0xE8A020, 0.4, sss=(1.0, 0.95, 0.5)),
        "MI_Flower_SeedHead": flower(30, 3.0, 0.5, 0.0, 0x3A2A1A, 0.5, vein=0.2, rough=0.9, sss=(0.6, 0.5, 0.4)),
    }
    for name, (s, v) in leaves.items():
        instance(name, m_leaf, s, v)

    petals = {
        # Nymphaea pads: waxy green above, red-purple beneath, radial veins.
        "MI_Pad_Lily": petal(1.0, 24.0, 0.32, underside=0x7A2E3A, under_mix=0.85, sss=(0.4, 0.6, 0.2), flutter=0.3, spec=0.5),
        "MI_Petal_Lily": petal(0.6, 7.0, 0.55, base_tint=(1.0, 1.0, 0.85), base_amount=0.2, sss=(1.0, 0.95, 0.95), flutter=0.3),
        # The golden lily glows a little (it is the pond's switch).
        "MI_Petal_GoldenLily": petal(0.6, 7.0, 0.45, base_tint=(1.0, 0.9, 0.6), base_amount=0.3, glow=0.6, sss=(1.0, 0.9, 0.5), flutter=0.3),
        "MI_Sepal_Lily": petal(0.4, 5.0, 0.5, underside=0x4A6A2A, under_mix=1.0, sss=(0.7, 0.8, 0.5), flutter=0.2),
        "MI_Stamen": petal(0.2, 1.0, 0.6, sss=(1.0, 0.9, 0.4), flutter=0.2),
        # Lotus: glaucous, matt, water-repellent leaves; pale beneath.
        "MI_Leaf_Lotus": petal(0.8, 22.0, 0.6, underside=0x9AAE80, under_mix=0.9, sss=(0.5, 0.7, 0.3), flutter=0.8, spec=0.35),
        "MI_Petal_Lotus": petal(0.8, 11.0, 0.5, base_tint=(1.0, 1.0, 0.8), base_amount=0.3, sss=(1.0, 0.9, 0.92), flutter=0.5),
        "MI_Leaf_Orchid": petal(0.4, 3.0, 0.35, underside=0x5E8040, under_mix=0.4, sss=(0.6, 0.8, 0.4), flutter=0.6),
        "MI_Petal_Orchid": petal(0.5, 6.0, 0.5, flutter=0.5),
        "MI_Grass": petal(0.3, 3.0, 0.55, underside=0x7A9A50, under_mix=0.3, sss=(0.8, 1.0, 0.5), flutter=0.6),
        "MI_Petal_Meadow": petal(0.5, 8.0, 0.45, flutter=0.5),
    }
    for name, (s, v) in petals.items():
        instance(name, m_petal, s, v)

    for name, rough, spec in [("MI_Fruit", 0.3, 0.6), ("MI_Pot", 0.18, 0.6), ("MI_Soil", 0.95, 0.3), ("MI_Receptacle", 0.6, 0.4)]:
        instance(name, m_solid, {"Roughness": rough, "Specular": spec})
    instance("MI_Rock_Taihu", m_rock, {"Bump": 0.004, "Roughness": 0.88})
    make_lawn_material()
    make_hedge_materials()
    make_velarium(mpc)
    unreal.EditorAssetLibrary.save_directory(NATURE, only_if_is_dirty=False, recursive=True)
    log("materials made in " + NATURE)


# ---------------------------------------------------------------------------------------------
# The plan (Shared/Wings/HallOfLight.swift, ChineseWing.swift, Reserve.swift, Shared/Plan/*.swift)

BIRCHES = [(14, -8), (16.5, -11.5), (21, -7.2), (24.5, -11.2), (28, -8.3), (32, -11.6), (35.5, -7.6), (38.5, -11)]
ORCHARD = [(14.5, 7.8), (20.5, 7.8), (26.5, 7.8), (32.5, 7.8), (38, 7.8), (16, 11.6), (22, 11.6), (28, 11.6), (34, 11.6), (39.5, 11.6)]

PLUM = (3.60, 20.10)
OSMANTHUS = (-3.49, 26.70)
ROCK = (0.08, 25.26)
# Culms stay within |x| <= 4.75: beyond that they would pierce the Chinese Wing's upturned eaves. The feet alone don't
# keep them there (the culms lean out and branch up to a metre), so each plant also gets a keep box (AMuseeTree.keep,
# plan metres; KEEP below): the culms, branches and leaves stay inside it.
BAMBOO = {
    "NW": [(-3.09, 14.18), (-2.83, 14.05), (-2.72, 13.82), (-4.45, 13.62), (-4.17, 13.93), (-3.68, 13.95), (-3.00, 14.12), (-2.73, 13.82),
           (-2.55, 13.83), (-3.56, 14.15), (-3.49, 13.76), (-3.17, 14.20), (-2.66, 14.01), (-3.48, 14.03), (-3.38, 14.05), (-3.79, 14.03)],
    "NE": [(3.99, 13.91), (4.00, 13.80), (4.62, 14.11), (4.20, 13.78), (3.85, 13.86), (3.15, 14.17), (4.52, 13.62), (4.74, 14.00),
           (3.49, 13.84), (4.25, 13.93), (4.28, 14.03), (2.85, 14.08), (4.45, 14.22), (4.10, 13.72), (2.51, 13.77), (4.60, 13.90)],
    # The SE clump, moved north-west so its feet stand 0.25 m and more inside the eaves' corner (x 4.9, y 28.4).
    "SE": [(x - 0.6, y - 0.55) for x, y in [
        (5.09, 28.55), (4.78, 27.77), (4.30, 28.33), (4.77, 28.46), (4.67, 28.48), (4.57, 28.52), (5.03, 27.98), (4.84, 28.35), (4.49, 28.18),
        (5.10, 27.77), (5.10, 28.36), (5.15, 27.87), (4.39, 28.52), (5.10, 28.03), (4.40, 27.68), (4.93, 27.81), (5.25, 28.22), (4.50, 28.32),
        (4.66, 28.70), (5.21, 28.12), (4.95, 28.38), (5.10, 28.77)]],
}
# The keep boxes (plan x0, y0, x1, y1; None: open that way) and the height they apply from: the north wall's inner
# face (y 13.5) and the eaves' edges (x ±4.9, y 28.4) less 0.15 m. The trees' crowns only above 1.5 m (their trunks
# stand clear), which keeps them off the colonnade and under the eaves.
KEEP = {
    "Bamboo_NW": ((-4.75, 13.65, 4.75, None), 0.0),
    "Bamboo_NE": ((-4.75, 13.65, 4.75, None), 0.0),
    "Bamboo_SE": ((None, None, 4.75, 28.25), 0.0),
    "Plum": ((None, None, 4.75, None), 1.5),
    "Osmanthus": ((-4.75, None, None, None), 1.5),
}


def keep_box(bounds, origin):
    """An unreal.Box2D relative to the plant at origin, from plan bounds (x0, y0, x1, y1; None: 50 m away)."""
    far = 50.0
    x0, y0, x1, y1 = bounds
    lo = v2((x0 - origin[0]) if x0 is not None else -far, (y0 - origin[1]) if y0 is not None else -far)
    hi = v2((x1 - origin[0]) if x1 is not None else far, (y1 - origin[1]) if y1 is not None else far)
    box = unreal.Box2D()
    box.set_editor_property("min", lo)
    box.set_editor_property("max", hi)
    box.set_editor_property("is_valid", True)
    return box


def keep_props(label, origin):
    if label not in KEEP:
        return {}
    bounds, from_height = KEEP[label]
    return {"keep": keep_box(bounds, origin), "keep_from_height": from_height}


POND = [(3.66, 20.90), (3.23, 21.21), (2.66, 21.53), (2.47, 21.90), (2.22, 22.22), (1.56, 22.34), (0.76, 22.39), (0.00, 22.48), (-0.75, 22.50),
        (-1.47, 22.31), (-2.10, 22.05), (-2.56, 21.83), (-2.92, 21.57), (-3.26, 21.25), (-3.47, 20.90), (-3.50, 20.54), (-3.30, 20.20),
        (-2.74, 19.93), (-2.03, 19.72), (-1.35, 19.59), (-0.65, 19.52), (0.05, 19.44), (0.73, 19.41), (1.39, 19.42), (2.00, 19.53),
        (2.53, 19.83), (2.99, 20.20), (3.45, 20.55)]
LOTUS = [((-1.90, 20.60), 0.76, 0.30), ((-1.10, 21.50), 0.64, 0.0), ((1.30, 20.40), 0.80, 0.0), ((2.00, 21.30), 0.60, 0.24),
         ((0.90, 21.60), 0.52, 0.0)]
POTS = [(-2.0, 14.6), (-1.6, 14.6), (1.6, 14.6), (2.0, 14.6), (-5.0, 17.7), (5.0, 17.7)]

OVAL_POND = (-86.5, 0.0)
POND_WATER = 0.38
BASIN = (4.2 - 0.3, 2.3 - 0.3)
GOLDEN_LILY = (-86.9, 0.5)
LILIES = [(-83.60, -0.60, 0.45, True), (-84.60, 0.90, 0.38, False), (-85.90, -1.20, 0.42, False), (-86.90, 0.50, 0.50, True),
          (-88.10, -0.70, 0.36, False), (-89.10, 0.80, 0.44, True), (-89.35, -0.45, 0.30, False), (-83.75, 0.72, 0.30, False),
          (-85.60, 1.40, 0.30, True), (-87.60, 1.50, 0.28, False)]
# The pond-edge redesign (Salon): the planting shelves at the ends of the long axis (|x| > 3.2 m from the centre, irises,
# sedge, forget-me-nots) are kept clear of pads: the lilies grow inside the water's edge cut off at |x| = 3.12 m (two of
# the plan's lilies moved in from the shelves, above).
LILY_CLEAR_X = 3.12

# Shared/Plan/HallOfLightPlanting.swift: (garden, colour, points).
PLANTING = [
    ("north", 0xA08AB0, [(11.9, -11.7), (12.3, -9.7), (12.4, -9.8), (12.8, -11.3), (12.9, -8.3), (14.2, -5.6), (14.5, -5.6), (14.6, -6.1), (14.8, -11.4), (16.5, -12.0), (18.2, -9.0), (18.6, -9.7), (18.6, -8.5), (19.0, -11.9), (19.8, -7.9), (20.0, -13.3), (20.3, -11.7), (20.4, -10.9), (21.5, -5.8), (21.7, -12.0), (22.4, -10.3), (23.4, -9.2), (24.7, -8.2), (25.8, -11.5), (25.9, -5.1), (25.9, -10.3), (26.9, -7.0), (29.1, -5.6), (30.0, -12.8), (30.6, -6.3), (31.8, -11.8), (31.9, -10.8), (34.0, -5.2), (35.3, -5.5), (36.8, -11.0), (38.1, -6.4), (38.2, -9.9), (38.2, -10.4), (38.5, -5.3), (38.7, -13.4), (39.4, -10.7)]),
    ("north", 0xD9A5A0, [(11.6, -12.7), (14.5, -7.2), (15.0, -13.0), (15.5, -7.0), (16.5, -13.4), (16.8, -7.6), (17.0, -9.1), (17.8, -12.5), (19.1, -12.8), (20.4, -10.0), (22.6, -9.0), (23.7, -12.5), (24.9, -7.0), (25.2, -7.0), (25.8, -11.6), (26.3, -8.9), (26.4, -10.1), (27.0, -6.0), (27.5, -5.9), (28.9, -12.3), (29.1, -9.2), (29.8, -8.5), (29.9, -7.0), (30.5, -9.3), (30.5, -7.4), (30.6, -13.1), (30.6, -9.0), (31.4, -10.8), (31.5, -11.8), (31.9, -10.4), (32.5, -13.4), (33.4, -6.2), (33.8, -6.1), (35.9, -7.0), (35.9, -5.4), (36.1, -10.7), (36.2, -9.2), (37.0, -6.0), (37.1, -11.4), (37.3, -7.8), (37.6, -9.6)]),
    ("north", 0xC9826A, [(11.6, -10.8), (12.1, -12.6), (12.9, -12.4), (13.1, -8.3), (13.9, -6.8), (14.5, -12.1), (14.9, -7.7), (15.9, -13.6), (16.1, -9.4), (16.5, -6.0), (16.7, -12.0), (18.2, -12.0), (18.8, -11.6), (19.6, -13.1), (21.2, -11.5), (21.7, -11.3), (22.2, -6.2), (22.3, -12.9), (23.4, -10.1), (23.4, -7.7), (24.9, -7.5), (25.2, -7.5), (25.6, -11.4), (25.7, -5.3), (26.5, -8.8), (26.9, -8.9), (27.3, -8.1), (27.3, -5.4), (27.6, -13.2), (27.7, -11.6), (28.3, -12.4), (28.7, -7.0), (28.7, -7.1), (28.7, -5.9), (29.6, -13.2), (29.9, -9.7), (30.8, -10.5), (31.2, -7.2), (31.3, -13.0), (31.8, -5.8), (33.5, -12.4), (33.7, -6.3), (34.6, -7.5), (35.0, -13.5), (35.5, -5.4), (35.5, -11.4), (35.8, -12.9), (36.4, -5.6), (36.7, -13.0), (37.1, -13.4), (37.6, -9.2), (37.7, -8.1), (38.8, -6.0)]),
    ("north", 0xE6C77A, [(11.8, -7.3), (12.0, -8.4), (13.7, -7.3), (14.4, -6.7), (14.9, -5.7), (15.9, -10.2), (16.3, -6.9), (20.4, -6.5), (23.7, -7.5), (23.9, -6.3), (24.8, -11.7), (25.1, -10.3), (25.6, -5.1), (26.7, -10.1), (27.1, -6.2), (27.4, -5.2), (28.2, -6.3), (29.7, -12.3), (29.9, -7.4), (30.6, -10.3), (31.9, -7.8), (32.2, -8.7), (33.9, -13.3), (34.2, -9.0), (34.2, -13.3), (34.5, -10.7), (34.6, -13.2), (34.6, -13.1), (35.1, -11.5), (35.9, -5.2), (36.5, -12.8), (36.7, -9.8), (36.9, -7.0), (38.5, -7.9), (38.8, -6.3)]),
    ("south", 0xF3EEE5, [(11.9, 10.6), (12.2, 8.3), (13.6, 9.7), (13.9, 7.6), (14.3, 10.8), (14.4, 7.6), (14.7, 8.4), (15.4, 5.4), (16.1, 10.4), (16.6, 8.4), (17.0, 6.9), (17.2, 11.6), (17.3, 9.7), (18.2, 7.5), (18.2, 5.4), (18.5, 6.2), (18.6, 7.4), (18.8, 8.2), (19.4, 7.3), (19.4, 11.1), (19.7, 13.4), (20.0, 10.6), (20.4, 12.8), (21.1, 7.7), (21.1, 9.2), (21.6, 13.3), (21.7, 12.3), (22.0, 7.6), (22.4, 13.1), (23.1, 8.1), (24.1, 6.8), (24.2, 7.8), (24.2, 8.2), (26.0, 7.9), (26.7, 11.7), (27.7, 6.8), (30.3, 11.7), (31.4, 7.0), (32.0, 8.2), (32.9, 10.5), (33.2, 11.1), (34.0, 13.3), (34.3, 8.7), (35.3, 10.2), (35.5, 8.5), (35.6, 11.1), (35.8, 6.9), (36.5, 12.5), (36.9, 13.2), (37.0, 13.5), (37.2, 13.2), (37.8, 8.2), (37.8, 10.9), (38.1, 12.4), (38.3, 5.4), (38.4, 8.8), (38.5, 6.0), (38.8, 6.4), (39.4, 12.5), (39.9, 12.9)]),
    ("south", 0xD9A5A0, [(12.1, 10.4), (12.2, 6.4), (12.4, 9.2), (12.5, 5.6), (14.5, 8.5), (15.2, 9.6), (16.3, 6.7), (16.4, 11.4), (17.5, 12.0), (18.1, 6.6), (18.6, 7.8), (20.1, 9.0), (20.6, 12.5), (23.5, 10.6), (23.8, 8.0), (23.9, 8.9), (24.3, 7.3), (24.6, 6.7), (24.7, 12.5), (24.8, 10.8), (25.2, 10.2), (25.6, 11.0), (25.9, 11.5), (25.9, 12.2), (26.1, 8.3), (28.9, 13.0), (28.9, 5.4), (29.0, 10.6), (29.3, 10.8), (29.5, 13.4), (29.5, 6.0), (29.6, 6.3), (30.9, 5.3), (31.1, 13.4), (31.2, 5.8), (31.6, 12.7), (33.7, 10.2), (33.8, 9.1), (34.9, 5.9), (35.7, 9.5), (35.7, 12.1), (36.3, 8.0), (37.5, 10.2), (38.0, 11.3)]),
    ("south", 0xE6C77A, [(12.0, 13.2), (12.2, 13.3), (13.0, 8.8), (13.7, 12.1), (13.7, 10.6), (14.5, 5.5), (15.2, 5.2), (15.4, 12.2), (15.5, 13.5), (15.8, 11.1), (16.2, 9.3), (16.3, 8.5), (17.0, 11.8), (17.0, 11.8), (17.1, 12.7), (17.8, 5.1), (18.0, 13.3), (18.3, 8.4), (20.9, 8.2), (21.0, 8.8), (21.1, 5.3), (21.3, 9.2), (22.9, 6.6), (23.1, 11.1), (24.8, 5.9), (25.0, 13.6), (25.1, 7.4), (26.2, 7.2), (26.3, 10.5), (26.7, 6.5), (28.3, 5.1), (28.5, 6.1), (28.5, 12.4), (28.6, 11.9), (29.4, 8.0), (30.8, 5.9), (31.3, 13.2), (32.6, 8.0), (33.6, 5.4), (34.5, 7.6), (35.3, 10.2), (35.8, 8.2), (38.0, 9.5), (38.8, 5.3), (38.8, 12.7), (39.4, 5.4)]),
]

# The stand-ins the export made (prim paths; duplicates are numbered _2, _3 …), by what replaces them.
STAND_INS = [
    ("trees (Hall of Light)", r"^/Museum/HallOfLight/(Trunk|Crown)(_\d+)?(/.*)?$"),
    ("meadow flowers and bulbs", r"^/Museum/HallOfLight/(Meadow_flowers|Meadow_flowers_stems|Seed_heads|Seed_heads_stems|Bulbs|Bulbs_stems)(_\d+)?$"),
    ("plum and osmanthus", r"^/Museum/ChineseWing/(Trunk|Crown)(_\d+)?(/.*)?$"),
    ("bamboo", r"^/Museum/ChineseWing/(Bamboo_culms|Bamboo_leaves)(_\d+)?$"),
    ("lotus", r"^/Museum/ChineseWing/(Lotus_pads|Lotus_flowers)(_\d+)?$"),
    ("Taihu rock", r"^/Museum/ChineseWing/(Taihu_rock|Rock_hollows|Snow_on_the_rock)(_\d+)?$"),
    ("potted plants", r"^/Museum/ChineseWing/(Pots|Orchids|Orchids_stems|Chrysanthemums|Chrysanthemums_stems)(_\d+)?$"),
    ("water lilies", r"^/Museum/Reserve/Lily_pond/(Lily_pads|Lily_flowers|The_golden_lily)(_\d+)?$"),
    # Plain boxes: AMuseeLandscape grows clipped yew in their place (MuseeHedge; lawn.py hedges hides them too).
    ("Hall of Light hedges", r"^/Museum/HallOfLight/Hedges(_\d+)?$"),
]

# What the export at 2026-06-21 must contain (the Swift build at Xiazhi, summer).
EXPECTED = (
    [f"/Museum/HallOfLight/Trunk{'' if i == 1 else f'_{i}'}" for i in range(1, 19)]
    + [f"/Museum/HallOfLight/Crown{'' if i == 1 else f'_{i}'}" for i in range(1, 19)]
    + ["/Museum/HallOfLight/Meadow_flowers", "/Museum/HallOfLight/Meadow_flowers_stems",
       "/Museum/ChineseWing/Trunk", "/Museum/ChineseWing/Trunk_2", "/Museum/ChineseWing/Crown", "/Museum/ChineseWing/Crown_2",
       "/Museum/ChineseWing/Bamboo_culms", "/Museum/ChineseWing/Bamboo_leaves", "/Museum/ChineseWing/Lotus_pads",
       "/Museum/ChineseWing/Lotus_flowers", "/Museum/ChineseWing/Taihu_rock", "/Museum/ChineseWing/Rock_hollows",
       "/Museum/Reserve/Lily_pond/Lily_pads", "/Museum/Reserve/Lily_pond/Lily_flowers", "/Museum/Reserve/Lily_pond/The_golden_lily"]
)


# ---------------------------------------------------------------------------------------------
# The level

def tag_value(actor, prefix):
    for t in actor.tags:
        s = str(t)
        if s.startswith(prefix):
            return s[len(prefix):]
    return None


def has_tag(actor, tag):
    return tag in [str(t) for t in actor.tags]


def cm(x, y, z=0.0):
    return unreal.Vector(x * 100.0, y * 100.0, z * 100.0)


def v2(x, y):
    return unreal.Vector2D(x, y)


def enum(enum_name, member):
    e = getattr(unreal, enum_name, None)
    value = getattr(e, member, None) if e else None
    if value is None:
        warn(f"unreal.{enum_name}.{member} is missing (is the C++ built?)")
    return value


def hide_stand_in(actor):
    """Hidden in game and in the editor, no collision; the prim tags stay (import_wing.py and relight.py use them)."""
    for step, fn in [("hidden in game", lambda: actor.set_actor_hidden_in_game(True)),
                     ("collision", lambda: actor.set_actor_enable_collision(False)),
                     ("editor", lambda: actor.set_is_temporarily_hidden_in_editor(True))]:
        try:
            fn()
        except Exception as e:  # noqa: BLE001
            warn(f"{actor.get_actor_label()}: {step} not set ({e})")
    # Invisible components stay invisible even when the building is shown again (MuseeWorld::SetBuildingHidden).
    for comp in actor.get_components_by_class(unreal.PrimitiveComponent):
        try:
            comp.set_visibility(False, True)
            comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        except Exception as e:  # noqa: BLE001
            warn(f"{actor.get_actor_label()}: component {comp.get_name()} not hidden ({e})")
    if not has_tag(actor, "musee.nature.replaced"):
        actor.tags = list(actor.tags) + [unreal.Name("musee.nature.replaced")]


def hide_stand_ins(actors):
    by_prim = {}
    for a in actors:
        path = tag_value(a, "prim:")
        if path:
            by_prim.setdefault(path, []).append(a)
    total = 0
    for what, pattern in STAND_INS:
        rx = re.compile(pattern)
        paths = sorted(p for p in by_prim if rx.match(p))
        for p in paths:
            for a in by_prim[p]:
                hide_stand_in(a)
                total += 1
        if paths:
            shown = ", ".join(paths[:6]) + (f" … ({len(paths)} prims)" if len(paths) > 6 else "")
            log(f"replaced {what}: {shown}")
        else:
            log(f"no stand-ins for {what} in the level (none at this moment, or the wing isn't imported)")
    for p in EXPECTED:
        if p not in by_prim:
            warn(f"stand-in {p} not found (the wing may not be imported, or the export changed)")
    log(f"{total} stand-in actors hidden")
    return by_prim


def remove_previous(actors):
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    old = [a for a in actors if has_tag(a, "musee.nature")]
    for a in old:
        eas.destroy_actor(a)
    if old:
        log(f"removed {len(old)} native plants from a previous run")


class Spawner:
    def __init__(self, today):
        self.eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        self.today = today
        self.total = 0
        self.count = 0

    def spawn(self, class_name, label, wing, location, props, attach_to=None):
        cls = unreal.load_class(None, f"/Script/MuseeVision.{class_name}")
        if not cls:
            warn(f"{class_name} is missing: build the C++ first")
            return None
        a = self.eas.spawn_actor_from_class(cls, location, unreal.Rotator(0, 0, 0))
        if not a:
            warn(f"could not spawn {label}")
            return None
        a.set_actor_label(label)
        props = dict(props)
        props["follow_today"] = self.today
        for k, v in props.items():
            if v is not None:
                set_prop(a, k, v)
        a.tags = [unreal.Name("musee.building"), unreal.Name(f"musee.wing:{wing}"), unreal.Name("musee.nature"), unreal.Name(f"nature:{label}")]
        if attach_to:
            try:
                a.attach_to_actor(attach_to, "", unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD,
                                  unreal.AttachmentRule.KEEP_WORLD, False)
            except Exception as e:  # noqa: BLE001
                warn(f"{label}: not attached to {attach_to.get_actor_label()} ({e})")
        tris = 0
        try:
            a.regrow()   # grows now, sizes the collision (saved with the map)
            tris = int(a.get_editor_property("triangle_count"))
        except Exception as e:  # noqa: BLE001
            warn(f"{label}: regrow failed ({e})")
        self.total += tris
        self.count += 1
        log(f"  {label}: {tris:,} triangles")
        return a


def season_of(name):
    return enum("MuseeNatureSeason", name.upper())


def place_hall_of_light(s, season):
    log(f"Hall of Light ({season}):")
    for i, (x, y) in enumerate(BIRCHES):
        # Garden.tree: trunk 6 m, crown centre 6.8 m, radii (1.9, 2.5, 1.9).
        s.spawn("MuseeTree", f"Birch_{i + 1:02d}", "HallOfLight", cm(x, y), {
            "species": enum("MuseeTreeSpecies", "BIRCH"), "season": season_of(season), "seed": 100 + i,
            "tree_height": 9.3, "crown_height": 6.8, "crown_radii": v2(1.9, 2.5)})
    for i, (x, y) in enumerate(ORCHARD):
        # Garden.tree: trunk 3.6 m, crown centre 4.9 m, radii (2.0, 1.8, 2.0); cherries and apples alternate.
        s.spawn("MuseeTree", f"Orchard_{i + 1:02d}_{'apple' if i % 2 else 'cherry'}", "HallOfLight", cm(x, y), {
            "species": enum("MuseeTreeSpecies", "ORCHARD_FRUIT"), "season": season_of(season), "seed": 200 + i, "fruit_variant": i % 2,
            "tree_height": 6.7, "crown_height": 4.9, "crown_radii": v2(2.0, 1.8)})

    def beds(garden, origin):
        out = []
        for g, colour, points in PLANTING:
            if g != garden:
                continue
            bed = unreal.MuseeFlowerBed()
            set_prop(bed, "colour", colour)
            set_prop(bed, "meadow", garden == "north")
            set_prop(bed, "points", [v2(px - origin[0], py - origin[1]) for px, py in points])
            out.append(bed)
        return out

    def holes(trees, origin, r):
        return [unreal.Vector4(x - origin[0] - r, y - origin[1] - r, x - origin[0] + r, y - origin[1] + r) for x, y in trees]

    # The meadow north of the glass (y −4.75 … −13.8, x 11.3 … 40.9), the orchard's grass south of it.
    north = (26.0, -9.3)
    s.spawn("MuseeMeadow", "Meadow_north", "HallOfLight", cm(*north), {
        "season": season_of(season), "seed": 41, "beds": beds("north", north),
        "grass_min": v2(11.3 - north[0], -13.75 - north[1]), "grass_max": v2(40.9 - north[0], -4.8 - north[1]),
        "grass_density": 2.5, "grass_holes": holes(BIRCHES, north, 0.3)})
    south = (26.0, 9.3)
    s.spawn("MuseeMeadow", "Meadow_south", "HallOfLight", cm(*south), {
        "season": season_of(season), "seed": 42, "beds": beds("south", south),
        "grass_min": v2(11.3 - south[0], 4.8 - south[1]), "grass_max": v2(40.9 - south[0], 13.75 - south[1]),
        "grass_density": 1.5, "grass_holes": holes(ORCHARD, south, 0.35)})


def place_chinese_garden(s, term):
    log(f"Chinese Wing (solar term {term}):")
    common = {"solar_term": term}
    # Garden.tree(plum): trunk 2.9 m, crown centre 2.8 m, radii (1.05, 0.6); gnarled and leaning.
    s.spawn("MuseeTree", "Plum", "ChineseWing", cm(*PLUM), dict(common, **keep_props("Plum", PLUM), **{
        "species": enum("MuseeTreeSpecies", "CHINESE_PLUM"), "seed": 301, "tree_height": 3.6, "crown_height": 2.8, "crown_radii": v2(1.35, 0.95)}))
    # Garden.tree(osmanthus): trunk 1.6 m, crown centre 2.7 m, radius 1.4.
    s.spawn("MuseeTree", "Osmanthus", "ChineseWing", cm(*OSMANTHUS), dict(common, **keep_props("Osmanthus", OSMANTHUS), **{
        "species": enum("MuseeTreeSpecies", "OSMANTHUS"), "seed": 302, "tree_height": 4.1, "crown_height": 2.7, "crown_radii": v2(1.45, 1.4)}))
    for i, (name, points) in enumerate(BAMBOO.items()):
        cx = sum(p[0] for p in points) / len(points)
        cy = sum(p[1] for p in points) / len(points)
        s.spawn("MuseeTree", f"Bamboo_{name}", "ChineseWing", cm(cx, cy), dict(common, **keep_props(f"Bamboo_{name}", (cx, cy)), **{
            "species": enum("MuseeTreeSpecies", "BAMBOO"), "seed": 88 + i, "culms": [v2(x - cx, y - cy) for x, y in points]}))
    s.spawn("MuseeTaihuRock", "Taihu_rock", "ChineseWing", cm(*ROCK), dict(common, **{"seed": 77}))

    # The lotus in the pond: water 0.3 m down, the edge 0.12 m inside the bank (buildChineseGarden).
    n = len(POND)
    cx, cy = sum(p[0] for p in POND) / n, sum(p[1] for p in POND) / n
    outline = []
    for x, y in POND:
        dx, dy = cx - x, cy - y
        d = math.hypot(dx, dy) or 1.0
        outline.append(v2(x + dx / d * 0.12 - cx, y + dy / d * 0.12 - cy))
    plants = []
    for (x, y), d, f in LOTUS:
        spec = unreal.MuseeLilySpec()
        set_prop(spec, "position", v2(x - cx, y - cy))
        set_prop(spec, "radius", d / 2)
        set_prop(spec, "flower", f > 0)
        set_prop(spec, "flower_size", f)
        plants.append(spec)
    s.spawn("MuseeWaterLilies", "Lotus", "ChineseWing", cm(cx, cy, -0.3), dict(common, **{
        "kind": enum("MuseeWaterPlant", "LOTUS"), "seed": 5, "plants": plants, "pond_outline": outline}))

    # Orchids at Chunfen, chrysanthemums at Shuangjiang, in pots by the bamboo beds and the rails.
    if 2 <= term <= 5 or 16 <= term <= 19:
        kind = "ORCHID" if term <= 5 else "CHRYSANTHEMUM"
        for i, (x, y) in enumerate(POTS):
            s.spawn("MuseePottedPlant", f"Pot_{i + 1}_{kind.lower()}", "ChineseWing", cm(x, y), dict(common, **{
                "kind": enum("MuseePottedPlantKind", kind), "seed": 500 + i}))
    else:
        log("  no potted orchids or chrysanthemums at this term (Chunfen 2–5, Shuangjiang 16–19)")


def place_grounds(s, season):
    """
    The grounds' trees round the museum (Source/MuseeVision/Facade, AMuseeLandscape): the allées of round-headed
    osmanthus in the parterre before the portico, birches along the avenue and the walk east, bamboo against the
    Chinese Wing's white outer walls, groves round the building. AMuseeLandscape.get_tree_placements() is their one
    source (the preview, musee.Facade.Preview, grows the same list); the Hall of Light's meadow and orchard stay
    place_hall_of_light's.
    """
    cls = getattr(unreal, "MuseeLandscape", None)
    if cls is None:
        warn("MuseeLandscape is missing (build the C++ first): the grounds get no trees")
        return
    log(f"The grounds ({season}):")
    for t in cls.get_tree_placements():
        props = {"species": t.species, "season": season_of(season), "seed": t.seed, "leaf_density": t.leaf_density}
        if t.tree_height > 0:
            props["tree_height"] = t.tree_height
        if t.crown_height > 0:
            props["crown_height"] = t.crown_height
        if t.crown_radii.x > 0:
            props["crown_radii"] = t.crown_radii
        if len(t.culms) > 0:
            props["culms"] = [v2(c.x, c.y) for c in t.culms]
        if t.keep.is_valid:
            props["keep"] = t.keep   # stays clear of a wall (the bamboo against the Chinese Wing)
        s.spawn("MuseeTree", t.name, "Exterior", cm(t.position.x, t.position.y), props)


def place_lilies(s, by_prim, season):
    log("The lily pond (Nymphéas oval):")
    pond = (by_prim.get("/Museum/Reserve/Lily_pond") or [None])[0]
    if not pond:
        warn("/Museum/Reserve/Lily_pond is missing: the lilies are placed but not attached (they won't rise with the pond)")
    plants = []
    for x, y, r, flower in LILIES:
        spec = unreal.MuseeLilySpec()
        golden = abs(x - GOLDEN_LILY[0]) < 0.01 and abs(y - GOLDEN_LILY[1]) < 0.01
        set_prop(spec, "position", v2(x - OVAL_POND[0], y - OVAL_POND[1]))
        set_prop(spec, "radius", r)
        set_prop(spec, "flower", flower)
        set_prop(spec, "golden", golden)
        plants.append(spec)
    # (Salon pond-edge redesign) the outline: the water's edge, cut off short of the planting shelves.
    outline = []
    for k in range(96):
        t = 2 * math.pi * k / 96
        outline.append(v2(max(-LILY_CLEAR_X, min(LILY_CLEAR_X, BASIN[0] * math.cos(t))), BASIN[1] * math.sin(t)))
    s.spawn("MuseeWaterLilies", "Water_lilies", "Reserve", cm(OVAL_POND[0], OVAL_POND[1], POND_WATER), {
        "kind": enum("MuseeWaterPlant", "WATER_LILY"), "season": season_of(season), "seed": 3, "plants": plants,
        "basin_radii": v2(*BASIN), "pond_outline": outline}, attach_to=pond)


def dapple_velarium(by_prim):
    path = f"{NATURE}/MI_Velarium_Dappled"
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        warn("MI_Velarium_Dappled is missing: the velarium keeps its material")
        return
    mi = unreal.load_asset(path)
    n = 0
    for a in by_prim.get("/Museum/Salon/Velarium", []):
        for comp in a.get_components_by_class(unreal.StaticMeshComponent):
            for i, m in enumerate(comp.get_materials()):
                if m and "velarium" in m.get_name().lower():
                    comp.set_material(i, mi)
                    n += 1
    if n:
        log(f"the velarium has leaf shadows ({n} slot{'s' if n > 1 else ''})")
    else:
        warn("no velarium found (/Museum/Salon/Velarium): import the Salon and run relight.py first")


def parse_args(argv):
    # The plants are baked into the build: grow them for the term (and season) of the day the map is made.
    term = todays_term()
    opts = {"term": term, "season": ("spring", "summer", "autumn", "winter")[term // 6], "today": False,
            "materials": True, "place": True}
    i = 0
    while i < len(argv):
        a = argv[i]
        key, _, val = a.partition("=")
        if key in ("--term", "--season") and not val and i + 1 < len(argv):
            val = argv[i + 1]
            i += 1
        if key == "--term":
            opts["term"] = int(val) % 24
        elif key == "--season":
            if val.lower() not in ("spring", "summer", "autumn", "winter"):
                raise ValueError(f"--season {val}: one of spring, summer, autumn, winter")
            opts["season"] = val.lower()
        elif key == "--today":
            opts["today"] = True
        elif key == "--materials-only":
            opts["place"] = False
        elif key == "--no-materials":
            opts["materials"] = False
        i += 1
    return opts


def main(argv):
    opts = parse_args(argv)
    if opts["materials"]:
        make_materials()
    if not opts["place"]:
        return
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not unreal.EditorAssetLibrary.does_asset_exist(P.MAP_PATH):
        raise RuntimeError(f"{P.MAP_PATH} is missing: run Scripts/setup_project.py and import the wings first")
    les.load_level(P.MAP_PATH)
    remove_previous(eas.get_all_level_actors())
    by_prim = hide_stand_ins(eas.get_all_level_actors())
    s = Spawner(opts["today"])
    place_hall_of_light(s, opts["season"])
    if not hasattr(unreal, "AlbionStructure"):   # (Albion stands on the south door now: the garden went with the Chinese Wing)
        place_chinese_garden(s, opts["term"])
    place_grounds(s, opts["season"])
    place_lilies(s, by_prim, opts["season"])
    dapple_velarium(by_prim)
    log(f"{s.count} native plants placed, {s.total:,} triangles in all")
    les.save_current_level()
    unreal.EditorAssetLibrary.save_directory(NATURE, only_if_is_dirty=True, recursive=True)
    log("saved")


if __name__ == "__main__":
    main(sys.argv[1:])
