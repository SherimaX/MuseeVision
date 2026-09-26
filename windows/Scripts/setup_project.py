"""
Set up the project once (and again whenever you like; it is idempotent):

    UnrealEditor-Cmd.exe MuseeVision.uproject -run=pythonscript -script="Scripts/setup_project.py"

- the native materials the C++ refers to (/Game/Museum/Materials, /Game/Museum/Sky);
- the Museum map (/Game/Maps/Museum) with the sky, a post-process volume for the museum's exposure,
  and a player start on the gilt sun facing the west door.

Then import the wings with Scripts/import_wing.py, the Rotunda first.
"""
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import musee_paths as P  # noqa: E402

MEL = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def log(msg):
    unreal.log(f"[setup_project] {msg}")


def hex_colour(h, a=1.0):
    c = unreal.LinearColor()
    srgb = unreal.Color(r=(h >> 16) & 255, g=(h >> 8) & 255, b=h & 255, a=255)
    lin = unreal.MathLibrary.conv_color_to_linear_color(srgb)
    c.r, c.g, c.b, c.a = lin.r, lin.g, lin.b, a
    return c


def new_material(folder, name):
    """A material to (re)build: an existing one is emptied in place, so what refers to it keeps it."""
    path = f"{folder}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        m = unreal.load_asset(path)
        # delete_all_material_expressions leaves some behind in 5.8 (custom output nodes among them: a second
        # water or thin-translucent output then fails the compile), so each leftover goes one by one.
        for _ in range(10):
            if MEL.get_num_material_expressions(m) == 0:
                break
            MEL.delete_all_material_expressions(m)
            for e in list(MEL.get_material_expressions(m)):
                MEL.delete_material_expression(m, e)
        return m
    return tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())


def constant3(mat, colour, x=-400, y=0):
    e = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, x, y)
    e.set_editor_property("constant", colour)
    return e


def scalar_param(mat, name, value, x=-400, y=200):
    e = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", value)
    return e


def multiply(mat, a, b, x=-200, y=0, a_out="", b_out=""):
    m = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, x, y)
    MEL.connect_material_expressions(a, a_out, m, "A")
    MEL.connect_material_expressions(b, b_out, m, "B")
    return m


def import_texture(png, folder):
    name = os.path.splitext(os.path.basename(png))[0]
    task = unreal.AssetImportTask()
    task.filename = png
    task.destination_path = folder
    task.destination_name = "T_" + name
    task.automated = True
    task.replace_existing = True
    task.save = True
    tools.import_asset_tasks([task])
    return unreal.load_asset(f"{folder}/T_{name}")


def make_collection():
    """MPC_Musee: "Daylight" (0 at night … 1 at noon), set by the Sky actor from the sun clock."""
    path = f"{P.MATERIALS}/MPC_Musee"
    mpc = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else         tools.create_asset("MPC_Musee", P.MATERIALS, unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
    daylight = unreal.CollectionScalarParameter()
    daylight.set_editor_property("parameter_name", "Daylight")
    daylight.set_editor_property("default_value", 1.0)
    mpc.set_editor_property("scalar_parameters", [daylight])
    unreal.EditorAssetLibrary.save_loaded_asset(mpc)
    return mpc


def daylight_factor(mat, mpc, night_floor, x=-600, y=300):
    """max(Daylight, NightFloor): sky-lit surfaces by day, backlit at a floor by night."""
    d = MEL.create_material_expression(mat, unreal.MaterialExpressionCollectionParameter, x, y)
    d.set_editor_property("collection", mpc)
    d.set_editor_property("parameter_name", "Daylight")
    m = MEL.create_material_expression(mat, unreal.MaterialExpressionMax, x + 200, y)
    MEL.connect_material_expressions(d, "", m, "A")
    MEL.connect_material_expressions(scalar_param(mat, "NightFloor", night_floor, x, y + 120), "", m, "B")
    return m


def make_daylit(mpc):
    """
    M_Daylit: a surface lit by the sky behind it (skylights, laylights, the velarium): texture × tint
    × Luminance (nits) × max(Daylight, NightFloor). Emissive, so Lumen lights the rooms from it.
    """
    m = new_material(P.MATERIALS, "M_Daylit")
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    tex = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -700, 0)
    tex.set_editor_property("parameter_name", "Image")
    tint = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -700, -220)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    lit = multiply(m, tex, tint, -450, 0, "RGB", "")
    nits = multiply(m, lit, scalar_param(m, "Luminance", 2500.0, -700, 180), -250, 0)
    glow = multiply(m, nits, daylight_factor(m, mpc, 0.25), 0, 0)
    MEL.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m)
    return m


# The laylights are daylight plus electric light behind the same diffuser (as at the Met): never
# below this share of their full glow, by night or on a dull morning.
LAYLIGHT_FLOOR = 0.6
DAYLIT_INSTANCES = ("MI_light_grid_FFFFFF", "MI_light_grid_F2EEE6", "MI_light_grid_FFF3DA", "MI_velarium")


def set_laylight_floor():
    """Apply LAYLIGHT_FLOOR to the laylight instances in place (relight.py calls this)."""
    for name in DAYLIT_INSTANCES:
        path = f"{P.MATERIALS}/{name}"
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            mi = unreal.load_asset(path)
            MEL.set_material_instance_scalar_parameter_value(mi, "NightFloor", LAYLIGHT_FLOOR)
            unreal.EditorAssetLibrary.save_loaded_asset(mi)


def make_daylit_instances(parent):
    """One instance per sky-lit material of the export (named as in usd/Materials.usda)."""
    textures = P.MUSEUM_CONTENT + "/Textures"
    grid = import_texture(os.path.join(P.USD_DIR, "textures", "light_grid.png"), textures)
    velarium = import_texture(os.path.join(P.USD_DIR, "textures", "velarium.png"), textures)
    for usd_name, image, tint, nits in [
        ("light_grid_FFFFFF", grid, 0xFFFFFF, 2500.0),   # the Salon's skylights
        ("light_grid_F2EEE6", grid, 0xF2EEE6, 2000.0),   # the Manet cabinet's laylight
        ("light_grid_FFF3DA", grid, 0xFFF3DA, 2500.0),   # Sculpture Hall's laylight
        ("velarium", velarium, 0xFFFFFF, 1800.0),        # the Nymphéas oval's velarium
    ]:
        name = "MI_" + usd_name
        path = f"{P.MATERIALS}/{name}"
        # Updated in place: the level's meshes keep referring to it.
        mi = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else             tools.create_asset(name, P.MATERIALS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        MEL.set_material_instance_parent(mi, parent)
        MEL.set_material_instance_texture_parameter_value(mi, "Image", image)
        MEL.set_material_instance_vector_parameter_value(mi, "Tint", hex_colour(tint))
        MEL.set_material_instance_scalar_parameter_value(mi, "Luminance", nits)
        MEL.set_material_instance_scalar_parameter_value(mi, "NightFloor", LAYLIGHT_FLOOR)
        unreal.EditorAssetLibrary.save_loaded_asset(mi)


# Ray tracing everywhere it shows: hit lighting for reflections, reflections and refraction on glass (the car, the
# steles, the pond) on the front layer, MegaLights on translucent layers (the global volume's settings; exposure.py
# applies them to the map's volume too). Measured at 4K (DLSS Quality) on the RTX 4090, 2026-09-24: with reflection
# bounces 2, roughness 0.6 and quality 2 the hit-lit reflections alone took 32 ms a frame in the Classical Hall's
# tribune (406 lights in view); one bounce, rays only up to roughness 0.45 (rougher surfaces take Lumen's radiance
# cache, all but the same on honed stone) and quality 1.5 keep the mirrors and polished floors and drop that cost.
RAY_TRACING = {
    "lumen_final_gather_quality": 2.0,       # short-range AO: 8 rays, not 4
    "lumen_scene_detail": 2.0,
    "lumen_scene_lighting_quality": 1.5,
    "lumen_reflection_quality": 1.5,
    "lumen_ray_lighting_mode": unreal.LumenRayLightingModeOverride.HIT_LIGHTING_FOR_REFLECTIONS,
    "lumen_max_roughness_to_trace_reflections": 0.45,
    "lumen_max_reflection_bounces": 1,
    "lumen_max_refraction_bounces": 1,      # reflections see the glass (DefaultEngine.ini)
    "lumen_front_layer_translucency_reflections": True,
    "mega_lights_front_layer_translucency": True,
    "translucency_type": unreal.TranslucencyType.RASTER,
    # Photo mode (P): the path tracer.
    "path_tracing_max_bounces": 16,
    "path_tracing_samples_per_pixel": 256,
    "path_tracing_enable_denoiser": True,
}


def make_stars(sky=None):
    """
    M_Stars: unlit, additive, two-sided; a round sprite tinted by the vertex colour, brightest at the centre and gone
    at the inscribed circle (1 − 2·distance, cubed), so a star is a soft point, never its square quad.
    """
    sky = sky or P.MUSEUM_CONTENT + "/Sky"
    m = new_material(sky, "M_Stars")
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    m.set_editor_property("two_sided", True)
    uv = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -900, 0)
    half = MEL.create_material_expression(m, unreal.MaterialExpressionConstant2Vector, -900, 120)
    half.set_editor_property("r", 0.5)
    half.set_editor_property("g", 0.5)
    dist = MEL.create_material_expression(m, unreal.MaterialExpressionDistance, -700, 60)
    MEL.connect_material_expressions(uv, "", dist, "A")
    MEL.connect_material_expressions(half, "", dist, "B")
    two = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -560, 60)
    MEL.connect_material_expressions(dist, "", two, "A")
    two.set_editor_property("const_b", 2.0)
    fall = MEL.create_material_expression(m, unreal.MaterialExpressionOneMinus, -430, 60)
    MEL.connect_material_expressions(two, "", fall, "")
    sat = MEL.create_material_expression(m, unreal.MaterialExpressionSaturate, -300, 60)
    MEL.connect_material_expressions(fall, "", sat, "")
    soft = MEL.create_material_expression(m, unreal.MaterialExpressionPower, -180, 60)
    MEL.connect_material_expressions(sat, "", soft, "Base")
    soft.set_editor_property("const_exponent", 3.0)
    vc = MEL.create_material_expression(m, unreal.MaterialExpressionVertexColor, -300, -120)
    col = multiply(m, vc, soft, -60, 0)
    bright = multiply(m, col, scalar_param(m, "Brightness", 100.0, -200, 220), 80, 0)   # (8 with sprites three times the size)
    MEL.connect_material_property(bright, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m)

    # M_MilkyWay: NASA SVS's Milky Way (Deep Star Maps 2020, windows/SourceArt/Textures/T_milkyway.png, encoded × 9) on
    # the star field's own sphere (AMuseeSky::BuildStars, section 1): unlit, additive, the 9 divided back out.
    png = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "SourceArt", "Textures", "T_milkyway.png")
    task = unreal.AssetImportTask()
    task.filename = png
    task.destination_path = sky
    task.destination_name = "T_milkyway"
    task.automated = True
    task.replace_existing = True
    task.save = True
    tools.import_asset_tasks([task])
    tex = unreal.load_asset(f"{sky}/T_milkyway")
    if tex:
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_SKYBOX)
        tex.set_editor_property("never_stream", True)
        unreal.EditorAssetLibrary.save_loaded_asset(tex)
    mw = new_material(sky, "M_MilkyWay")
    mw.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mw.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    mw.set_editor_property("two_sided", True)
    sample = MEL.create_material_expression(mw, unreal.MaterialExpressionTextureSample, -500, 0)
    if tex:
        sample.set_editor_property("texture", tex)
    glow = multiply(mw, sample, scalar_param(mw, "Brightness", 4.0, -500, 200), -200, 0, "RGB", "")
    MEL.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(mw)
    unreal.EditorAssetLibrary.save_loaded_asset(mw)
    return m


def make_misty(mats=None, mpc=None):
    """
    M_MistyGlass: milky translucent glazing lit by the sky behind it (about 450 nits at noon: a soft pearl, not a
    lamp), seen from both sides. Smooth, as the Atrium rendering's membrane (07): a faint cloudiness 6 m across, a
    little brighter higher up where the sky reaches it, and the panels' fine horizontal seams every 1.6 m. (It was a
    64 px dot frit, which at the drum's scale read as a grey perforated sheet.)
    """
    mats = mats or P.MATERIALS
    mpc = mpc or unreal.load_asset(f"{P.MATERIALS}/MPC_Musee")
    m = new_material(mats, "M_MistyGlass")
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    E = lambda cls, x, y: MEL.create_material_expression(m, cls, x, y)
    def op(cls, a, b, x, y, **consts):
        n = E(cls, x, y)
        for pin, v in (("A", a), ("B", b)):
            if isinstance(v, (int, float)):
                n.set_editor_property("const_" + pin.lower(), float(v))
            else:
                MEL.connect_material_expressions(v, "", n, pin)
        return n
    wp = E(unreal.MaterialExpressionWorldPosition, -1400, 0)
    z = E(unreal.MaterialExpressionComponentMask, -1250, 0)
    z.set_editor_property("r", False); z.set_editor_property("g", False); z.set_editor_property("b", True)
    MEL.connect_material_expressions(wp, "", z, "")
    zm = op(unreal.MaterialExpressionMultiply, z, 0.01, -1100, 0)                     # metres
    # A faint cloudiness.
    noise = E(unreal.MaterialExpressionNoise, -1100, 200)
    for k, v in (("scale", 1.0 / 600.0), ("levels", 3), ("output_min", -1.0), ("output_max", 1.0)):
        noise.set_editor_property(k, v)
    MEL.connect_material_expressions(wp, "", noise, "")   # Position is the first input
    cloud = op(unreal.MaterialExpressionAdd, op(unreal.MaterialExpressionMultiply, noise, 0.035, -900, 200), 0.965, -750, 200)
    # Brighter higher up.
    grad = E(unreal.MaterialExpressionClamp, -750, 0)
    grad.set_editor_property("min_default", 0.85); grad.set_editor_property("max_default", 1.12)
    MEL.connect_material_expressions(op(unreal.MaterialExpressionAdd, op(unreal.MaterialExpressionMultiply, zm, 0.012, -950, 0), 0.85, -850, 0), "", grad, "")
    # Seams every 1.6 m: |frac(z / 1.6 + 0.5) - 0.5| × 1.6 is the distance (m) to the nearest; 2 cm lines, 15 % darker.
    fr = E(unreal.MaterialExpressionFrac, -950, -200)
    MEL.connect_material_expressions(op(unreal.MaterialExpressionAdd, op(unreal.MaterialExpressionDivide, zm, 1.6, -1100, -200), 0.5, -1020, -200), "", fr, "")
    dist = E(unreal.MaterialExpressionAbs, -850, -200)
    MEL.connect_material_expressions(op(unreal.MaterialExpressionSubtract, fr, 0.5, -900, -200), "", dist, "")
    near = E(unreal.MaterialExpressionSmoothStep, -700, -200)
    near.set_editor_property("const_min", 0.0); near.set_editor_property("const_max", 0.01 / 1.6)
    MEL.connect_material_expressions(dist, "", near, "Value")
    seam = op(unreal.MaterialExpressionAdd, op(unreal.MaterialExpressionMultiply, near, 0.15, -600, -200), 0.85, -500, -200)
    tint = constant3(m, hex_colour(0xF1F4F2), -500, 100)
    lit = multiply(m, tint, scalar_param(m, "Luminance", 450.0, -500, 250), -350, 100)
    lit = multiply(m, lit, op(unreal.MaterialExpressionMultiply, op(unreal.MaterialExpressionMultiply, cloud, grad, -600, 100), seam, -450, 0), -200, 100)
    glow = multiply(m, lit, daylight_factor(m, mpc, 0.2), 0, 100)
    MEL.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m)
    return m


def make_glass(mats=None):
    """
    M_Glass: thin panes, clear with a faint tint, reflecting about 4% like real glass (Lumen's
    front-layer reflections), no refraction (flat panes don't bend the view). Glass scatters almost
    nothing (Diffuse 0): no lit colour, only reflection and what shows through, so the sun or a spot on a
    stele doesn't turn the pane into a lit white film.
    """
    mats = mats or P.MATERIALS
    m = new_material(mats, "M_Glass")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("two_sided", True)
    for prop, value in [("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING),
                        ("refraction_method", getattr(unreal, "RefractionMode", None) and unreal.RefractionMode.RM_NONE)]:
        try:
            if value is not None:
                m.set_editor_property(prop, value)
        except Exception as e:
            log(f"M_Glass: {prop} not set ({e})")
    E = lambda cls, x, y: MEL.create_material_expression(m, cls, x, y)

    def op(cls, a, b, x, y):
        n = E(cls, x, y)
        for pin, v in (("A", a), ("B", b)):
            if isinstance(v, (int, float)):
                n.set_editor_property("const_" + pin.lower(), float(v))
            else:
                MEL.connect_material_expressions(v, "", n, pin)
        return n

    def lerp(a, b, t, x, y):
        n = E(unreal.MaterialExpressionLinearInterpolate, x, y)
        for pin, v in (("A", a), ("B", b), ("Alpha", t)):
            MEL.connect_material_expressions(v, "", n, pin)
        return n

    # Glass as it is: Thin Translucent (UE's physically based pane). The renderer does the Fresnel (about 4 %
    # reflected face on, rising to a mirror at grazing) and the longer path through the pane at a slant, so
    # nothing here fakes them (the old Fresnel-driven opacity greyed the view through the glass).
    # TransmittanceColor: what a pane lets through at normal incidence. Tint (the export's, or an instance's)
    # colours it a little, low-iron glass hardly at all; Opacity darkens it (the car's glass is a touch
    # denser). Smudge: a faint world-space haze of dust and fingerprints that softens the reflection where it lies.
    # GrazingOpacity and Diffuse stay as parameters (instances set them) but the physics replaces them.
    # (Thin Translucent is switched on last, once its output node exists: every property change compiles.)
    try:
        m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    except Exception as e:  # noqa: BLE001
        log(f"M_Glass: shading model not reset ({e})")
    try:
        m.set_editor_property("output_translucent_velocity", True)   # the moving car: DLSS RR/FG see it
    except Exception as e:  # noqa: BLE001
        log(f"M_Glass: translucent velocity not set ({e})")
    opacity = scalar_param(m, "Opacity", 0.1, -1100, 0)
    scalar_param(m, "GrazingOpacity", 0.62, -1100, 60)
    scalar_param(m, "Diffuse", 0.0, -1100, 120)
    wp = E(unreal.MaterialExpressionWorldPosition, -1400, 350)
    noise = E(unreal.MaterialExpressionNoise, -1200, 350)
    for k, v in (("scale", 1.0 / 9.0), ("levels", 4), ("output_min", 0.0), ("output_max", 1.0)):
        noise.set_editor_property(k, v)
    MEL.connect_material_expressions(wp, "", noise, "")
    blot = E(unreal.MaterialExpressionSaturate, -900, 350)
    MEL.connect_material_expressions(op(unreal.MaterialExpressionMultiply, op(unreal.MaterialExpressionSubtract, noise, 0.45, -1050, 350), 2.5, -980, 350), "", blot, "")
    smudge = op(unreal.MaterialExpressionMultiply, blot, scalar_param(m, "Smudge", 0.4, -900, 450), -750, 350)
    tint = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -1100, -100)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(0.93, 0.965, 0.955, 1))   # low-iron
    white = constant3(m, unreal.LinearColor(1, 1, 1, 1), -1100, -200)
    t_amount = E(unreal.MaterialExpressionConstant, -1100, -150)
    t_amount.set_editor_property("r", 0.35)
    tinted = lerp(white, tint, t_amount, -850, -150)
    density = op(unreal.MaterialExpressionSubtract, 1.0, op(unreal.MaterialExpressionMultiply, opacity, 0.3, -900, 0), -800, 0)
    transmittance = multiply(m, tinted, density, -600, -100)
    out = E(unreal.MaterialExpressionThinTranslucentMaterialOutput, -300, -100)
    MEL.connect_material_expressions(transmittance, "", out, "TransmittanceColor")
    # Opacity is Thin Translucent's surface coverage: an opaque layer lying on the pane (at 1 the pane was a
    # black sheet). Here it is only the dust of the smudge (a few per cent where it lies, pale grey).
    dust = constant3(m, unreal.LinearColor(0.55, 0.55, 0.53, 1), -500, -250)
    MEL.connect_material_property(dust, "", unreal.MaterialProperty.MP_BASE_COLOR)
    coverage = op(unreal.MaterialExpressionMultiply, smudge, 0.002, -500, 50)
    MEL.connect_material_property(coverage, "", unreal.MaterialProperty.MP_OPACITY)
    rough = op(unreal.MaterialExpressionAdd, scalar_param(m, "Roughness", 0.01, -700, 200), op(unreal.MaterialExpressionMultiply, smudge, 0.006, -650, 400), -450, 200)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(scalar_param(m, "Specular", 0.5, -500, 300), "", unreal.MaterialProperty.MP_SPECULAR)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_THIN_TRANSLUCENT)
    errors = MEL.recompile_material(m)
    log(f"M_Glass: thin translucent, {len(list(errors or []))} compile errors {list(errors or [])}")
    unreal.EditorAssetLibrary.save_loaded_asset(m)
    return m


# The museum's water: dark, still and reflective, over a dark stone bed (the materials spec, Part 5).
# Absorption and scattering are per centimetre (Single Layer Water's units); (absorption, scattering, ripple, roughness).
WATERS = {
    # The Nymphéas pond: Giverny's still olive water, tannin-dark, the bed gone by about 35 cm.
    "MI_Water_Pond": ((0.020, 0.013, 0.016), (0.0012, 0.0024, 0.0016), 0.10, 0.03),
    # The Chinese court's lotus pond: the same water, a little greener.
    "MI_Water_Garden": ((0.022, 0.012, 0.018), (0.0010, 0.0030, 0.0020), 0.08, 0.03),
    # The canal and round basin outside: a mirror for the portico.
    "MI_Water_Canal": ((0.026, 0.016, 0.018), (0.0008, 0.0016, 0.0012), 0.02, 0.02),
}


def make_water(mats=None):
    """
    M_Water: Single Layer Water, Unreal's physically based water body (reflection by Lumen, refraction of
    what lies below, absorption and scattering by depth). Its surface moves as still water does: two ripple
    normals (T_water_ripples, 0.6 m and 1.7 m tiles, world-aligned) drifting slowly in different directions,
    Ripple strength per instance. Horizontal surfaces only (world-space normal). Not Nanite (the engine
    renders Single Layer Water on ordinary meshes only).
    """
    mats = mats or P.MATERIALS
    m = new_material(mats, "M_Water")
    E = lambda cls, x, y: MEL.create_material_expression(m, cls, x, y)  # noqa: E731
    for prop, value in (("shading_model", unreal.MaterialShadingModel.MSM_SINGLE_LAYER_WATER),
                        ("blend_mode", unreal.BlendMode.BLEND_OPAQUE),
                        ("refraction_method", unreal.RefractionMode.RM_PIXEL_NORMAL_OFFSET),
                        ("tangent_space_normal", False), ("has_pixel_animation", True), ("two_sided", False)):
        try:
            m.set_editor_property(prop, value)
        except Exception as e:  # noqa: BLE001
            log(f"M_Water: {prop} not set ({e})")
    png = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "SourceArt", "Textures", "T_water_ripples.png")
    tex_path = f"{P.MUSEUM_CONTENT}/Textures/T_water_ripples"
    if not unreal.EditorAssetLibrary.does_asset_exist(tex_path):
        task = unreal.AssetImportTask()
        task.filename = png
        task.destination_path = f"{P.MUSEUM_CONTENT}/Textures"
        task.destination_name = "T_water_ripples"
        task.automated = True
        task.replace_existing = True
        tools.import_asset_tasks([task])
    ripples = unreal.load_asset(tex_path)
    ripples.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
    ripples.set_editor_property("srgb", False)
    unreal.EditorAssetLibrary.save_loaded_asset(ripples)

    wp = E(unreal.MaterialExpressionWorldPosition, -1800, 0)
    xy = E(unreal.MaterialExpressionComponentMask, -1650, 0)
    xy.set_editor_property("r", True)
    xy.set_editor_property("g", True)
    xy.set_editor_property("b", False)
    MEL.connect_material_expressions(wp, "", xy, "")
    metres = E(unreal.MaterialExpressionMultiply, -1500, 0)
    MEL.connect_material_expressions(xy, "", metres, "A")
    metres.set_editor_property("const_b", 0.01)
    time = E(unreal.MaterialExpressionTime, -1500, 200)

    def ripple(tile, vx, vy, y):
        uv = E(unreal.MaterialExpressionDivide, -1350, y)
        MEL.connect_material_expressions(metres, "", uv, "A")
        uv.set_editor_property("const_b", tile)
        drift = E(unreal.MaterialExpressionMultiply, -1350, y + 60)
        MEL.connect_material_expressions(time, "", drift, "A")
        v = constant3(m, unreal.LinearColor(vx / tile, vy / tile, 0, 1), -1500, y + 60)
        dm = E(unreal.MaterialExpressionComponentMask, -1250, y + 60)
        dm.set_editor_property("r", True)
        dm.set_editor_property("g", True)
        MEL.connect_material_expressions(v, "", dm, "")
        MEL.connect_material_expressions(dm, "", drift, "B")
        moved = E(unreal.MaterialExpressionAdd, -1150, y)
        MEL.connect_material_expressions(uv, "", moved, "A")
        MEL.connect_material_expressions(drift, "", moved, "B")
        smp = E(unreal.MaterialExpressionTextureSample, -1000, y)
        smp.set_editor_property("texture", ripples)
        smp.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        MEL.connect_material_expressions(moved, "", smp, "UVs")
        return smp

    a = ripple(0.6, 0.012, 0.007, -200)    # metres per second: a slow drift
    b = ripple(1.7, -0.006, 0.010, 200)
    both = E(unreal.MaterialExpressionAdd, -800, 0)
    MEL.connect_material_expressions(a, "", both, "A")
    MEL.connect_material_expressions(b, "", both, "B")
    slope = E(unreal.MaterialExpressionComponentMask, -650, 0)
    slope.set_editor_property("r", True)
    slope.set_editor_property("g", True)
    MEL.connect_material_expressions(both, "", slope, "")
    strength = scalar_param(m, "Ripple", 0.1, -650, 150)
    scaled = multiply(m, slope, strength, -500, 0)
    up = E(unreal.MaterialExpressionAppendVector, -380, 0)
    MEL.connect_material_expressions(scaled, "", up, "A")
    one = E(unreal.MaterialExpressionConstant, -500, 100)
    one.set_editor_property("r", 1.0)
    MEL.connect_material_expressions(one, "", up, "B")
    nrm = E(unreal.MaterialExpressionNormalize, -250, 0)
    MEL.connect_material_expressions(up, "", nrm, "")
    MEL.connect_material_property(nrm, "", unreal.MaterialProperty.MP_NORMAL)
    MEL.connect_material_property(constant3(m, unreal.LinearColor(0, 0, 0, 1), -250, -150), "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(scalar_param(m, "Roughness", 0.03, -250, 150), "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(scalar_param(m, "Specular", 0.5, -250, 250), "", unreal.MaterialProperty.MP_SPECULAR)
    try:
        MEL.connect_material_property(scalar_param(m, "Refraction", 1.1, -250, 350), "", unreal.MaterialProperty.MP_REFRACTION)
    except Exception as e:  # noqa: BLE001
        log(f"M_Water: refraction not connected ({e})")
    out = E(unreal.MaterialExpressionSingleLayerWaterMaterialOutput, 0, 400)

    def vparam(name, value, y):
        e = E(unreal.MaterialExpressionVectorParameter, -250, y)
        e.set_editor_property("parameter_name", name)
        e.set_editor_property("default_value", unreal.LinearColor(value[0], value[1], value[2], 1))
        return e
    MEL.connect_material_expressions(vparam("Scattering", (0.0012, 0.0024, 0.0016), 450), "", out, "ScatteringCoefficients")
    MEL.connect_material_expressions(vparam("Absorption", (0.020, 0.013, 0.016), 550), "", out, "AbsorptionCoefficients")
    MEL.connect_material_expressions(scalar_param(m, "PhaseG", 0.2, -250, 650), "", out, "PhaseG")
    MEL.connect_material_expressions(scalar_param(m, "ColorScaleBehindWater", 1.0, -250, 750), "", out, "ColorScaleBehindWater")
    errors = MEL.recompile_material(m)
    if errors:
        log(f"M_Water: compile errors {list(errors)}")
    unreal.EditorAssetLibrary.save_loaded_asset(m)

    for name, (absorb, scatter, ripple_amount, rough) in WATERS.items():
        path = f"{mats}/{name}"
        mi = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else \
            tools.create_asset(name, mats, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        MEL.set_material_instance_parent(mi, m)
        MEL.set_material_instance_vector_parameter_value(mi, "Absorption", unreal.LinearColor(*absorb, 1))
        MEL.set_material_instance_vector_parameter_value(mi, "Scattering", unreal.LinearColor(*scatter, 1))
        MEL.set_material_instance_scalar_parameter_value(mi, "Ripple", ripple_amount)
        MEL.set_material_instance_scalar_parameter_value(mi, "Roughness", rough)
        MEL.update_material_instance(mi)
        unreal.EditorAssetLibrary.save_loaded_asset(mi)
    log("M_Water and its instances made")
    return m


def make_materials():
    sky = P.MUSEUM_CONTENT + "/Sky"
    mats = P.MATERIALS
    mpc = make_collection()
    make_daylit_instances(make_daylit(mpc))

    make_stars(sky)

    make_misty(mats, mpc)
    make_glass(mats)

    # The ribs and the ring: dark steel.
    m = new_material(mats, "M_RibSteel")
    m.set_editor_property("two_sided", True)
    MEL.connect_material_property(constant3(m, hex_colour(0x1E1C19)), "", unreal.MaterialProperty.MP_BASE_COLOR)
    metal = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 150)
    metal.set_editor_property("r", 1.0)
    MEL.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
    rough = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 250)
    rough.set_editor_property("r", 0.5)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(m)

    # Gilt bronze: the mast.
    m = new_material(mats, "M_Gilt")
    MEL.connect_material_property(constant3(m, hex_colour(0xC9A266)), "", unreal.MaterialProperty.MP_BASE_COLOR)
    metal = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 150)
    metal.set_editor_property("r", 1.0)
    MEL.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
    rough = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 250)
    rough.set_editor_property("r", 0.3)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(m)

    # The iris: a pale glow.
    m = new_material(mats, "M_IrisGlow")
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    glow = multiply(m, constant3(m, hex_colour(0xE3ECEA)), scalar_param(m, "Luminance", 3000.0), 0, 0)
    MEL.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m)
    log("materials made")


def spawn(cls, location, rotation=unreal.Rotator(0, 0, 0), label=None):
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    a = eas.spawn_actor_from_class(cls, location, rotation)
    if label:
        a.set_actor_label(label)
    return a


# Museum photographs (the Met, the Orangerie) are exposed about a stop brighter than a light meter
# would: walls read light, laylights near white.
EXPOSURE_BIAS = 0.9
# Local exposure, as a photographer's highlight recovery: bright openings (a sunlit hall seen through a
# door, a laylight) are compressed towards the room's level instead of clipping to white; the
# shadows lift a little. Below 1 compresses.
LOCAL_HIGHLIGHTS = 0.35
LOCAL_SHADOWS = 0.85
# White balance at daylight's: whites read white (7400 K had every room cream; measured on the walls, blue/red
# 0.64–0.91). The sky light's own slight warmth keeps the lanterns' light from reading blue.
WHITE_TEMP = 6600.0
# The eye adapts only so far into the dark: a room at night stays a room at night (lit pools, darker
# surroundings) instead of being brightened back to midday. The Reserve by its lamps meters about 7.
EXPOSURE_MIN_EV = 5.5
EXPOSURE_MAX_EV = 16.0   # a sunlit room under the eye meters EV100 15.5
EXPOSURE_LOW_PERCENT = 20.0
EXPOSURE_HIGH_PERCENT = 65.0


def make_map():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if unreal.EditorAssetLibrary.does_asset_exist(P.MAP_PATH):
        les.load_level(P.MAP_PATH)
    else:
        les.new_level(P.MAP_PATH)
    # Replace the scaffolding (sky, exposure, start); the imported wings stay.
    sky_cls = unreal.load_class(None, "/Script/MuseeVision.MuseeSky")
    for a in eas.get_all_level_actors():
        if a.get_class() == sky_cls or isinstance(a, (unreal.PostProcessVolume, unreal.PlayerStart)):
            eas.destroy_actor(a)

    spawn(sky_cls, unreal.Vector(0, 0, 0), label="Sky")

    # The museum's exposure.
    ppv = spawn(unreal.PostProcessVolume, unreal.Vector(0, 0, 0), label="Museum exposure")
    ppv.set_editor_property("unbound", True)
    s = ppv.settings
    s.set_editor_property("override_auto_exposure_method", True)
    s.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
    s.set_editor_property("override_auto_exposure_min_brightness", True)
    # EV100 5.5 (a lamplit gallery at night) to 16 (the Rotunda with the sun through its eye).
    s.set_editor_property("auto_exposure_min_brightness", EXPOSURE_MIN_EV)
    s.set_editor_property("override_auto_exposure_max_brightness", True)
    s.set_editor_property("auto_exposure_max_brightness", EXPOSURE_MAX_EV)
    s.set_editor_property("override_auto_exposure_speed_up", True)
    s.set_editor_property("auto_exposure_speed_up", 2.0)
    s.set_editor_property("override_auto_exposure_speed_down", True)
    s.set_editor_property("auto_exposure_speed_down", 1.5)
    # A light touch: a faint bloom on the eye and the skylights, no haze over the rooms.
    # Pale stone and plaster read white, not washed out.
    s.set_editor_property("override_auto_exposure_bias", True)
    s.set_editor_property("auto_exposure_bias", EXPOSURE_BIAS)
    # Meter on the room, not the sky: skylights, the velarium and the misty glass are the brightest
    # tenth to half of a view, and are left to read near-white (high key), as a photographer would.
    s.set_editor_property("override_histogram_log_min", True)
    s.set_editor_property("histogram_log_min", 0.0)
    s.set_editor_property("override_histogram_log_max", True)
    s.set_editor_property("histogram_log_max", 16.0)
    s.set_editor_property("override_auto_exposure_low_percent", True)
    s.set_editor_property("auto_exposure_low_percent", EXPOSURE_LOW_PERCENT)
    s.set_editor_property("override_auto_exposure_high_percent", True)
    s.set_editor_property("auto_exposure_high_percent", EXPOSURE_HIGH_PERCENT)
    s.set_editor_property("override_bloom_intensity", True)
    s.set_editor_property("bloom_intensity", 0.08)
    s.set_editor_property("override_vignette_intensity", True)
    s.set_editor_property("vignette_intensity", 0.15)
    for name, value in RAY_TRACING.items():
        s.set_editor_property("override_" + name, True)
        s.set_editor_property(name, value)
    ppv.set_editor_property("settings", s)
    import exposure   # the compensation curve (exposure.py imports this module, so not at the top)
    exposure.apply(ppv, exposure.make_curve())

    # On the gilt sun, facing the open west door to the Salon (capsule centre 92 cm up).
    spawn(unreal.PlayerStart, unreal.Vector(0, 0, 92), unreal.Rotator(0, 0, 180), label="Gilt sun")
    les.save_current_level()
    log(f"{P.MAP_PATH} ready")


if __name__ == "__main__":
    make_materials()
    make_map()
    unreal.EditorAssetLibrary.save_directory(P.MUSEUM_CONTENT, only_if_is_dirty=True, recursive=True)
