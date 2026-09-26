"""
Albion's hang (plan/proposals/albion, the Hang board): twenty-nine places, hung two deep round a 1.6 m line.

    python windows/Scripts/albion_hang.py layout        (plain Python: writes assets/albion/hang.json)
    UnrealEditor-Cmd ... -script="<repo>/windows/Scripts/albion_hang.py import"   (textures, materials)
    (apply_all.py albion runs materials, import and place)

- layout: each work's place from the board's elevations (its bay, its order along the wall, its height), spaced so the
  frames keep 0.3 m between them in their bay; the frame's style after its painter (Rossetti's and Madox Brown's reeded
  frames, Watts frames for Burne-Jones, Leighton, Waterhouse, Hunt and Millais's larger works, cabinet frames for the
  small); written to assets/albion/hang.json, which AAlbionStructure's preview (-MuseeAlbion) and place() both read.
- import (in Unreal): each image as /Game/Museum/Textures/Albion/Paintings/T_<id>, the Kelmscott Chaucer's pages as
  .../Albion/Chaucer/T_chaucer_NNN; the masters M_Albion_Canvas (oil under varnish, the canvas's weave in the light),
  M_Albion_Paper (a watercolour, matt), M_Albion_Tapestry (wool and silk, the warp's ribs, a fuzz), M_Albion_Wallpaper
  (block-printed distemper), M_Albion_Lustre (tin glaze and lustre); an instance per work.
- place (in Unreal): an AAlbionWork per work, tagged work:<id> (its placard) and musee.wing:Albion, and an AMuseeFrame
  round each framed one (the reeded and Watts styles are Albion's additions to Frames/).
"""
import json
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
ART = os.path.join(REPO, "assets", "albion")
LAYOUT = os.path.join(ART, "hang.json")

# The court (AlbionPlan.h).
X0, X1, Y0, Y1 = -14.0, 14.0, 17.2, 53.2
RAIL = 4.2                   # the bronze picture rail on the walls
GAP = 0.30                   # between frames in a row
PILASTER = 0.15

# Frame styles by painter; widths (m) of the moulding.
REEDED, WATTS, CABINET = "ReededGilt", "WattsGilt", "CabinetGilt"


def frame_for(w):
    a, big = w["artist"], max(w["w"], w["h"])
    if w.get("kind") in ("Tapestry", "Dish", "Vase"):
        return None, 0.0
    if w.get("kind") == "Wallpaper":
        return "Photograph", 0.035
    if big < 0.5:
        return CABINET, 0.07
    if a in ("rossetti", "brown", "moore", "stillman"):
        return REEDED, 0.10 if big < 1.0 else 0.13
    return WATTS, 0.11 if big < 1.0 else (0.14 if big < 2.0 else 0.17)


def outer(w, style, fw):
    """The frame's outside width and height (m)."""
    k = {REEDED: 1.02, WATTS: 1.0, CABINET: 0.9, "Photograph": 1.0}.get(style, 0.0)
    return w["w"] + 2 * fw * k, w["h"] + 2 * fw * k


# The places (the Hang board). wall: W / E / S / N. order: along the wall in the direction of the walk's reading
# (west: north to south; east: north to south); row: 0 on the line, 1 above the work before it.
PLACES = [
    # West aisle, truth to nature 1848–68 (bays W1 … W6, north to south).
    ("W", 1, 0, "albion-millais-mariana", 1.65, "millais"),
    ("W", 1, 1, "albion-brown-pretty-baa-lambs", 2.605, "brown"),
    ("W", 2, 0, "albion-millais-ophelia", 1.65, "millais"),
    ("W", 2, 1, "albion-brown-english-autumn-afternoon", 2.705, "brown"),
    ("W", 3, 0, "albion-hunt-awakening-conscience", 1.65, "hunt"),
    ("W", 3, 0, "albion-brown-last-of-england", 1.65, "brown"),
    ("W", 4, 0, "albion-millais-blind-girl", 1.655, "millais"),
    ("W", 4, 0, "albion-wallis-stonebreaker", 1.645, "wallis"),
    ("W", 5, 0, "albion-hughes-long-engagement", 1.645, "hughes"),
    ("W", 5, 0, "albion-brown-walton-on-the-naze", 1.65, "brown"),
    ("W", 6, 0, "albion-sandys-morgan-le-fay", 1.65, "sandys"),
    ("W", 6, 0, "albion-sandys-medea", 1.65, "sandys"),
    # East aisle, beauty 1860–96 (bays E1 … E6, north to south; the visitor reads them walking north).
    ("E", 1, 0, "albion-morris-trellis", 1.70, "morris"),
    ("E", 1, 0, "albion-morris-acanthus", 1.70, "morris"),
    ("E", 1, 0, "albion-morris-willow-bough", 1.70, "morris"),
    ("E", 2, 0, "albion-stillman-loves-messenger", 1.645, "stillman"),
    ("E", 2, 0, "albion-millais-portia", 1.645, "millais"),
    ("E", 2, 0, "albion-de-morgan-flora", 1.745, "de-morgan"),
    ("E", 2, 0, "albion-leighton-lachrymae", 1.69, "leighton"),
    ("E", 3, 0, "albion-burne-jones-golden-stairs", 2.095, "burne-jones"),
    ("E", 3, 0, "albion-moore-dreamers", 1.645, "moore"),
    ("E", 4, 0, "albion-burne-jones-pygmalion-1", 1.645, "burne-jones"),
    ("E", 4, 0, "albion-burne-jones-pygmalion-2", 1.645, "burne-jones"),
    ("E", 4, 0, "albion-burne-jones-pygmalion-3", 1.645, "burne-jones"),
    ("E", 4, 0, "albion-burne-jones-pygmalion-4", 1.645, "burne-jones"),
    ("E", 5, 0, "albion-burne-jones-love-song", 1.65, "burne-jones"),
    ("E", 6, 0, "albion-rossetti-beata-beatrix", 1.65, "rossetti"),
    ("E", 6, 0, "albion-rossetti-lady-lilith", 1.65, "rossetti"),
    ("E", 6, 0, "albion-rossetti-proserpine", 1.645, "rossetti"),
    # The south end, the legend: the aisles' ends and the nave's stone under the screen.
    ("S", -10.0, 0, "albion-waterhouse-lady-of-shalott", 1.765, "waterhouse"),
    ("S", 0.0, 0, "albion-morris-attainment", 2.22, "morris"),
    ("S", 10.0, 0, "albion-burne-jones-king-cophetua", 2.165, "burne-jones"),
    # The north end, the way out.
    ("N", 10.0, 0, "albion-burne-jones-star-of-bethlehem", 2.18, "burne-jones"),
]

KINDS = {"albion-morris-attainment": "Tapestry", "albion-morris-trellis": "Wallpaper", "albion-morris-acanthus": "Wallpaper",
         "albion-morris-willow-bough": "Wallpaper"}
WATERCOLOURS = {"albion-rossetti-lady-lilith", "albion-burne-jones-star-of-bethlehem"}
# Wallpaper lengths on their panels (the board: 1.2 × 3.0 m), and each paper's repeat (height, width) measured on its sheet.
WALLPAPER = {"albion-morris-trellis": (0.52, 0.52), "albion-morris-acanthus": (0.66, 0.53), "albion-morris-willow-bough": (0.436, 0.514)}


def sources():
    with open(os.path.join(ART, "paintings", "SOURCES.json"), encoding="utf-8") as f:
        return {w["id"]: w for w in json.load(f)}


def layout():
    src = sources()
    works = []
    for wall, where, row, wid, zc, artist in PLACES:
        s = src[wid]
        kind = KINDS.get(wid, "Canvas")
        if kind == "Wallpaper":
            w, h = 1.2, 3.0
        else:
            w, h = float(s["width_cm"]) / 100.0, float(s["height_cm"]) / 100.0
        works.append(dict(id=wid, wall=wall, where=where, row=row, z=zc, artist=artist, kind=kind, w=w, h=h,
                          sight={"arched": "Arched", "oval": "Oval"}.get(s.get("shape"), "Rect"),
                          glazed=wid in WATERCOLOURS, repeat=WALLPAPER.get(wid)))
    for w in works:
        w["frame"], w["frame_width"] = frame_for(w)
        w["outer_w"], w["outer_h"] = outer(w, w["frame"], w["frame_width"]) if w["frame"] else (w["w"], w["h"])
    # Along the side walls: each bay's row-0 works spread evenly (equal gaps, at least GAP between frames); a row-1 work
    # centred over the work before it.
    for wall in ("W", "E"):
        for bay in range(1, 7):
            row0 = [w for w in works if w["wall"] == wall and w["where"] == bay and w["row"] == 0]
            if not row0:
                continue
            b0, b1 = Y0 + 6.0 * (bay - 1) + PILASTER, Y0 + 6.0 * bay - PILASTER
            total = sum(w["outer_w"] for w in row0)
            if len(row0) == 1:
                gap = 0.0
            else:
                gap = max(GAP, min(0.9, (b1 - b0 - total) / (len(row0) + 1)))
            span = total + gap * (len(row0) - 1)
            y = 0.5 * (b0 + b1) - 0.5 * span
            if span > b1 - b0:
                print(f"warning: bay {wall}{bay} is over-full ({span:.2f} m in {b1 - b0:.2f})")
            for w in row0:
                w["y"] = y + 0.5 * w["outer_w"]
                y += w["outer_w"] + gap
        for i, w in enumerate(works):
            if w["wall"] == wall and w["row"] == 1:
                below = works[i - 1]
                w["y"] = below["y"]
                # Keep 0.18 m between the frames, the board's heights otherwise.
                w["z"] = max(w["z"], below["z"] + 0.5 * below["outer_h"] + 0.18 + 0.5 * w["outer_h"])
    out = []
    for w in works:
        if w["wall"] == "W":
            x, y, yaw = X0, w["y"], 0.0
        elif w["wall"] == "E":
            x, y, yaw = X1, w["y"], 180.0
        elif w["wall"] == "S":
            x, y, yaw = w["where"], Y1, -90.0
        else:
            x, y, yaw = w["where"], Y0, 90.0
        e = dict(id=w["id"], kind=w["kind"], sight=w["sight"], width=round(w["w"], 4), height=round(w["h"], 4),
                 x=round(x, 4), y=round(y, 4), z=round(w["z"], 4), yaw=yaw, frame=w["frame"] or "None",
                 frame_width=w["frame_width"], canvas_offset=0.05 if w["frame"] else 0.0, glazed=w["glazed"],
                 outer_w=round(w["outer_w"], 4), outer_h=round(w["outer_h"], 4),
                 rail_above=round(RAIL - w["z"], 4) if w["wall"] in ("W", "E") and w["kind"] == "Canvas" else 0.0,
                 repeat=w["repeat"], material=f"/Game/Museum/Materials/Albion/Works/MI_{w['id'].replace('-', '_')}")
        out.append(e)
    # The De Morgan lustreware in the four table cases (lines 3 and 5, both aisles): the charger and dishes lying flat on
    # the felt at 0.55 m, the vases standing (0.4 m under the glass).
    pottery = [("albion-de-morgan-charger", "Dish", 0.514, 0.07, -10.4, 29.2 - 0.45), ("albion-de-morgan-dish-eagle", "Dish", 0.252, 0.04, -10.4, 29.2 + 0.5),
               ("albion-de-morgan-peacock-dish", "Dish", 0.40, 0.055, 10.4, 29.2 - 0.3), ("albion-de-morgan-vase", "Vase", 0.241, 0.349, 10.4, 29.2 + 0.55),
               # line 5: the rice dish (Walters, Ø 31.1) and the dish with a great fish (Chicago, Ø 31.4) west; the ship plate
               # (Chicago, Ø 20) and the jar with two fish (Cooper Hewitt, 38 cm) east.
               ("albion-de-morgan-rice-dish", "Dish", 0.311, 0.044, -10.4, 41.2 - 0.45), ("albion-de-morgan-fish-dish", "Dish", 0.314, 0.05, -10.4, 41.2 + 0.45),
               ("albion-de-morgan-ship-plate", "Dish", 0.20, 0.03, 10.4, 41.2 - 0.4), ("albion-de-morgan-jar", "Vase", 0.24, 0.38, 10.4, 41.2 + 0.4)]
    for wid, kind, w, h, x, y in pottery:
        out.append(dict(id=wid, kind=kind, sight="Rect", width=w, height=h, x=x, y=y, z=0.552, yaw=0.0, frame="None", frame_width=0.0,
                        canvas_offset=0.0, glazed=False, outer_w=w, outer_h=h, rail_above=0.0, repeat=None,
                        material=f"/Game/Museum/Materials/Albion/Works/MI_{wid.replace('-', '_')}"))
    with open(LAYOUT, "w", encoding="utf-8") as f:
        json.dump({"rail": RAIL, "works": out}, f, indent=1)
    print(f"{len(out)} works laid out → {LAYOUT}")
    for e in out:
        print(f"  {e['id']:44s} {e['kind']:9s} {e['frame']:11s} ({e['x']:7.2f}, {e['y']:6.2f}) z {e['z']:.2f}  {e['width']:.2f} × {e['height']:.2f}")
    return out


# ============================================================================================ in Unreal

def unreal_import():
    import unreal
    sys.path.append(HERE)
    import musee_paths as P
    import setup_project as S
    MEL = unreal.MaterialEditingLibrary
    EAL = unreal.EditorAssetLibrary
    TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
    mats = P.MATERIALS + "/Albion/Works"
    texs = P.MUSEUM_CONTENT + "/Textures/Albion/Paintings"
    pages = P.MUSEUM_CONTENT + "/Textures/Albion/Chaucer"

    def log(m):
        unreal.log(f"[albion_hang] {m}")

    def imp(png, folder, name, srgb=True):
        """Import (again only when the image is newer than its asset)."""
        path = f"{folder}/{name}"
        pkg = os.path.join(HERE, "..", "Content", path[len("/Game/"):] + ".uasset")
        if EAL.does_asset_exist(path) and os.path.exists(pkg) and os.path.getmtime(pkg) >= os.path.getmtime(png):
            return unreal.load_asset(path)
        task = unreal.AssetImportTask()
        task.filename = png
        task.destination_path = folder
        task.destination_name = name
        task.automated = True
        task.replace_existing = True
        task.save = True
        TOOLS.import_asset_tasks([task])
        t = unreal.load_asset(path)
        if t:
            t.set_editor_property("srgb", srgb)
            t.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
            t.set_editor_property("max_texture_size", 8192)
            EAL.save_loaded_asset(t)
        return t

    # Masters.
    def custom(m, code, inputs, out_type):
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

    F1, F3 = unreal.CustomMaterialOutputType.CMOT_FLOAT1, unreal.CustomMaterialOutputType.CMOT_FLOAT3
    default_tex = unreal.load_asset("/Engine/EngineResources/DefaultTexture")

    def master(name, model, colour_code=None, rough_code="return 0.5;", coat=None, normal_code=None, cloth=False, tiled=False):
        """A master through MakeMaterialAttributes (its ClearCoat pin is the clear coat, or the cloth's amount; its
        SubsurfaceColor the cloth's fuzz)."""
        m = S.new_material(mats, name)
        m.set_editor_property("use_material_attributes", True)
        ma = MEL.create_material_expression(m, unreal.MaterialExpressionMakeMaterialAttributes, -200, 0)
        uv = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1400, 0)
        tex = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -1200, 0)
        tex.set_editor_property("parameter_name", "Tex")
        tex.set_editor_property("texture", default_tex)
        MEL.connect_material_expressions(uv, "", tex, "UVs")
        MEL.connect_material_expressions(tex, "RGB", ma, "BaseColor")
        r = custom(m, rough_code, [("C", (tex, "RGB")), ("UV", (uv, ""))], F1)
        MEL.connect_material_expressions(r, "", ma, "Roughness")
        if normal_code:
            nn = custom(m, normal_code, [("UV", (uv, "")), ("C", (tex, "RGB"))], F3)
            MEL.connect_material_expressions(nn, "", ma, "Normal")

        def const(v, y):
            c = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, y)
            c.set_editor_property("r", v)
            return c
        if coat:
            MEL.connect_material_expressions(const(coat[0], 300), "", ma, "ClearCoat")
            MEL.connect_material_expressions(const(coat[1], 380), "", ma, "ClearCoatRoughness")
        if cloth:
            MEL.connect_material_expressions(const(0.7, 300), "", ma, "ClearCoat")
            fz = custom(m, "return lerp(C, float3(0.85, 0.82, 0.76), 0.35);", [("C", (tex, "RGB"))], F3)
            MEL.connect_material_expressions(fz, "", ma, "SubsurfaceColor")
        MEL.connect_material_property(ma, "", unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
        m.set_editor_property("shading_model", model)
        errs = list(MEL.recompile_material(m) or [])
        log(f"{name}: {len(errs)} errors {errs}")
        EAL.save_loaded_asset(m)
        return m

    if not EAL.does_directory_exist(mats):
        EAL.make_directory(mats)
    # Oil under an old varnish: a clear coat over the paint (the paint itself matt where it's lean, the varnish's sheen
    # over all); the canvas's plain weave (about 16 threads a cm) in the normal, too faint to see face on.
    weave = ("float2 p = UV * float2(Size.x, Size.y); float a = sin(p.x * 6.2832) * 0.5 + 0.5; float b = sin(p.y * 6.2832) * 0.5 + 0.5;"
             "return normalize(float3((a - 0.5) * 0.05, (b - 0.5) * 0.05, 1.0));")
    canvas = master("M_Albion_Canvas", unreal.MaterialShadingModel.MSM_CLEAR_COAT,
                    rough_code="float l = dot(C, float3(0.3, 0.59, 0.11)); return lerp(0.55, 0.42, saturate(l));", coat=(0.55, 0.28))
    paper = master("M_Albion_Paper", unreal.MaterialShadingModel.MSM_DEFAULT_LIT,
                   rough_code="float l = dot(C, float3(0.3, 0.59, 0.11)); return lerp(0.62, 0.8, saturate(l));")
    tapestry = master("M_Albion_Tapestry", unreal.MaterialShadingModel.MSM_CLOTH,
                      rough_code="return 0.86;",
                      normal_code="float r = sin(UV.x * 5000.0) * 0.5; float w = sin(UV.y * 2400.0 + sin(UV.x * 700.0)) * 0.25;"
                                  "return normalize(float3(r * 0.35, w * 0.2, 1.0));", cloth=True)
    wallpaper = master("M_Albion_Wallpaper", unreal.MaterialShadingModel.MSM_DEFAULT_LIT, rough_code="return 0.84;")
    lustre = master("M_Albion_Lustre", unreal.MaterialShadingModel.MSM_CLEAR_COAT,
                    rough_code="float s = max(C.r, max(C.g, C.b)) - min(C.r, min(C.g, C.b)); return lerp(0.35, 0.18, saturate(s * 2.0));",
                    coat=(1.0, 0.04))
    parents = {"Canvas": canvas, "Paper": paper, "Tapestry": tapestry, "Wallpaper": wallpaper, "Dish": lustre, "Vase": lustre}
    with open(LAYOUT, encoding="utf-8") as f:
        lay = json.load(f)
    for e in lay["works"]:
        png = os.path.join(ART, "paintings", e["id"] + ".jpg")
        if not os.path.exists(png):
            log(f"{e['id']}: no image")
            continue
        tex = imp(png, texs, "T_" + e["id"].replace("-", "_"))
        kind = "Paper" if e.get("glazed") else e["kind"]
        name = "MI_" + e["id"].replace("-", "_")
        path = f"{mats}/{name}"
        mi = unreal.load_asset(path) if EAL.does_asset_exist(path) else TOOLS.create_asset(name, mats, unreal.MaterialInstanceConstant,
                                                                                             unreal.MaterialInstanceConstantFactoryNew())
        MEL.set_material_instance_parent(mi, parents[kind])
        MEL.set_material_instance_texture_parameter_value(mi, "Tex", tex)
        MEL.update_material_instance(mi)
        EAL.save_loaded_asset(mi)
    log(f"{len(lay['works'])} works' materials")
    # The Chaucer's pages.
    folder = os.path.join(ART, "chaucer")
    n = 0
    for fn in sorted(os.listdir(folder)):
        if fn.startswith("page_") and fn.endswith(".jpg"):
            imp(os.path.join(folder, fn), pages, "T_chaucer_" + fn[5:8])
            n += 1
    log(f"{n} Chaucer pages")


def unreal_place():
    import unreal
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    TAG = "musee.albion.hang"
    for a in eas.get_all_level_actors():
        if TAG in [str(t) for t in a.tags]:
            eas.destroy_actor(a)
    with open(LAYOUT, encoding="utf-8") as f:
        lay = json.load(f)
    vessels_path = os.path.join(os.path.dirname(LAYOUT), "vessels.json")
    vessels = json.load(open(vessels_path, encoding="utf-8")) if os.path.exists(vessels_path) else {}
    kinds = {"Canvas": unreal.AlbionWorkKind.CANVAS, "Tapestry": unreal.AlbionWorkKind.TAPESTRY, "Wallpaper": unreal.AlbionWorkKind.WALLPAPER,
             "Dish": unreal.AlbionWorkKind.DISH, "Vase": unreal.AlbionWorkKind.VASE}
    sights = {"Rect": unreal.AlbionSight.RECT, "Arched": unreal.AlbionSight.ARCHED, "Oval": unreal.AlbionSight.OVAL}
    n = 0
    for e in lay["works"]:
        at = unreal.Vector(e["x"] * 100.0, e["y"] * 100.0, e["z"] * 100.0)
        rot = unreal.Rotator(0.0, 0.0, e["yaw"])
        w = eas.spawn_actor_from_class(unreal.AlbionWork, at, rot)
        w.set_editor_property("kind", kinds[e["kind"]])
        w.set_editor_property("sight", sights[e["sight"]])
        w.set_editor_property("width", e["width"])
        w.set_editor_property("height", e["height"])
        w.set_editor_property("canvas_offset", e["canvas_offset"] or 0.045)
        w.set_editor_property("frame_outer_half_width", 0.5 * e["outer_w"] if e["frame"] != "None" else 0.0)
        w.set_editor_property("frame_outer_top", 0.5 * e["outer_h"])
        w.set_editor_property("rail_above", e["rail_above"])
        if e.get("repeat"):
            w.set_editor_property("pattern_repeat", unreal.Vector2D(e["repeat"][0], e["repeat"][1]))
        mat = unreal.load_asset(e["material"])
        if mat:
            w.set_editor_property("face_material", mat)
        v = vessels.get(e["id"])
        if v:   # the De Morgan pieces: their box in the photograph, a vase's outline and handles (assets/albion/vessels.json)
            try:
                win = v["window"]
                w.set_editor_property("image_window", unreal.Vector4(win[0], win[1], win[2], win[3]))
                if v.get("profile"):
                    w.set_editor_property("profile", [unreal.Vector2D(r, z) for r, z in v["profile"]])
                if v.get("handle"):
                    w.set_editor_property("handle_path", [unreal.Vector2D(y, z) for y, z in v["handle"]])
                    w.set_editor_property("handle_thickness", v.get("handle_thickness", 0.0))
            except Exception as ex:  # noqa: BLE001 - an editor built before AAlbionWork had them: the generic shapes
                unreal.log_warning(f"[albion_hang] {e['id']}: its outline not set ({ex})")
        w.set_actor_label(f"Albion {e['id']}")
        extra = {"Canvas": ["musee.albion.wall"], "Tapestry": ["musee.albion.wall", "musee.textile"],
                 "Wallpaper": ["musee.albion.wall", "musee.paper"]}.get(e["kind"], [])   # (gallery_lights.py reads these)
        w.tags = [unreal.Name(t) for t in ["musee.building", "musee.wing:Albion", TAG, f"work:{e['id']}"] + extra]
        w.rebuild()
        if e["frame"] != "None":
            fr = eas.spawn_actor_from_class(unreal.MuseeFrame, at, rot)
            fr.set_editor_property("sight_width", e["width"])
            fr.set_editor_property("sight_height", e["height"])
            fr.set_editor_property("canvas_offset", e["canvas_offset"])
            fr.configure(e["frame"], e["frame_width"], 0.0, unreal.Vector2D(0.0, 0.0), False)
            fr.attach_to_actor(w, "", unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, False)
            fr.set_actor_label(f"Albion frame {e['id']}")
            fr.tags = [unreal.Name("musee.building"), unreal.Name("musee.wing:Albion"), unreal.Name(TAG), unreal.Name("musee.frame")]
        n += 1
    unreal.log(f"[albion_hang] placed {n} works")


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    what = args[0] if args else "layout"
    if what == "layout":
        layout()
    elif what == "import":
        unreal_import()
    elif what == "place":
        unreal_place()
