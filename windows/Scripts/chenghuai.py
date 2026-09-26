"""
澄懷 Chenghuai (plan/proposals/chenghuai): the three-court Beijing house on the Rotunda's north door and its Suzhou
garden (Source/MuseeVision/Chenghuai). Run inside the editor:

    UnrealEditor-Cmd.exe MuseeVision.uproject -run=pythonscript -script="<repo>/windows/Scripts/chenghuai.py materials place"

Steps (any of them, in this order):
    textures   the painted atlases (chenghuai_textures.py; plain Python, run it by hand first when PIL is there)
    masters    (re)builds Chenghuai's masters M_Ch_Stone, M_Ch_StoneRelief, M_Ch_Plaster, M_Ch_Wood: materials.py's own graphs
               with weathering() (rain streaks, splash zone, moss) and nanmu's chatoyance; then the materials step
    materials  the wing's material instances in /Game/Museum/Materials/Chenghuai (MI_Ch_<part>, one per mesh section of
               AChenghuaiStructure, in the museum's material language: its masters and photographed sets, the museum's
               library and Chenghuai's own CC0 sets, SourceArt/PBR/CH_*, fetched by chenghuai_pbr.py), MI_Ch_Pond,
               M_Ch_Paper (mulberry paper in laminated glass: two-sided foliage, so the sun through it glows)
    place      AChenghuaiStructure at the origin; the Sculpture Hall retired (its native room removed, its imported pieces
               and lights hidden); the lawn kept off the site
    works      (chenghuai_works.py) the 33 works, their mounts, cases' contents, placards; the plants

native.py places the structure too (ROOMS), so a later `apply_all native` keeps it.
"""
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import musee_paths as P  # noqa: E402
import materials as MAT  # noqa: E402
import pbr  # noqa: E402
import setup_project as S  # noqa: E402

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
FOLDER = P.MATERIALS + "/Chenghuai"
lin = MAT.lin
TAG = "musee.wing:Chenghuai"
CLASS = "/Script/MuseeVision.ChenghuaiStructure"
SITE = (-20.7, -61.9, 16.9, -17.0)      # plan metres: the site (x0, y0, x1, y1)


def log(msg):
    unreal.log(f"[chenghuai] {msg}")


def warn(msg):
    unreal.log_warning(f"[chenghuai] {msg}")


# ---------------------------------------------------------------------------------------------- materials

NJ, UB = MAT.NO_JOINTS, MAT.UNBROKEN
CH_STATS = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "SourceArt", "PBR", "chenghuai_stats.json"))


def add_stats():
    """Chenghuai's photographed sets (chenghuai_pbr.py: SourceArt/PBR/CH_*) into pbr.py's table, in memory."""
    import json
    with open(CH_STATS, encoding="utf-8") as f:
        mine = json.load(f)
    pbr.stats().update(mine)
    return mine


def weathering(g, proj, c, rough, grime=None):
    """
    The weather on a surface, in the order it lays it down, for any master built on materials.py's Graph and projection
    (box_projection's proj: u, v, p, n). Reusable: every amount defaults to 0, so a room indoors is untouched.
    - the rooms' dirt first (materials.grime: FootDirt at the foot of a wall, DustUp on ledges);
    - RainStreaks: water running off copings, sills and eaves carries dust down vertical faces: dark runs 3-8 cm wide,
      0.5-3 m long, none in the splash zone at the foot;
    - SplashZone: the foot of an outdoor wall (0-35 cm), darker and greener where rain splashes back off the paving;
    - Moss (MossCover, MossColor): on faces turned to the sky (MossUp) and in the splash zone (MossFoot), in patches
      that grow from the damp: matt (roughness 0.95).
    Returns (colour, roughness).
    """
    if grime is not None:
        c, rough = grime(g, proj, c, rough)
    p, n = proj["p"], proj["n"]
    nz = g.mask(n, "B")
    z = g.mask(p, "B")
    h = MAT.height_above_floor(g, z)
    vertical = g.one_minus(g.smoothstep(0.35, 0.7, g.abs(nz)))
    brk = g.noise(g.div(p, 0.17), levels=2, out_min=0.0, out_max=1.0)
    # Rain streaks: a noise stretched 25 times down the wall, in patches (where a coping drips, not everywhere), darkest
    # high on the wall where the water leaves the coping and fading as it runs down.
    runs = g.noise(g.append(g.append(g.div(proj["u"], 0.07), g.div(z, 1.6)), 7.3), levels=3, out_min=0.0, out_max=1.0)
    patch = g.noise(g.append(g.div(proj["u"], 1.3), g.append(g.div(z, 6.0), 3.9)), levels=2, out_min=0.0, out_max=1.0)
    streak = g.mul(g.mul(g.smoothstep(0.6, 0.85, runs), g.smoothstep(0.45, 0.75, patch)), vertical)
    streak = g.mul(streak, g.mul(g.scalar("RainStreaks", 0.0, "Weather", 0, 0.6), g.smoothstep(0.9, 2.6, h)))
    c = g.mul(c, g.one_minus(streak))
    # The splash zone.
    foot = g.mul(g.one_minus(g.smoothstep(0.0, 0.35, g.add(h, g.mul(brk, 0.1)))), vertical)
    splash = g.mul(foot, g.scalar("SplashZone", 0.0, "Weather", 0, 0.5))
    c = g.lerp(c, g.mul(c, pbr.vec3(g, 0.6, 0.65, 0.52)), splash)
    # Moss.
    patches = g.noise(g.div(p, 0.45), levels=4, out_min=0.0, out_max=1.0)
    cover = g.scalar("MossCover", 0.0, "Weather", 0, 1)
    where = g.max(g.mul(g.smoothstep(0.55, 0.95, nz), g.scalar("MossUp", 1.0, "Weather", 0, 1)),
                  g.mul(foot, g.scalar("MossFoot", 0.6, "Weather", 0, 1)))
    lo = g.one_minus(cover)
    moss = g.mul(g.smoothstep(lo, g.add(lo, 0.18), patches), where)
    fine = g.noise(g.div(p, 0.025), levels=1, out_min=0.0, out_max=1.0)
    mcol = g.mul(g.vector("MossColor", lin(0x3A4424), "Weather"), g.add(0.7, g.mul(fine, 0.6)))
    c = g.lerp(c, mcol, moss)
    rough = g.sat(g.add(g.add(rough, g.mul(streak, 0.08)), g.mul(splash, 0.1)))
    rough = g.lerp(rough, 0.95, moss)
    return c, rough


def chatoyant(layer):
    """
    Nanmu's silk (金丝): the fibres of its interlocked grain lean one way, then the other, in bands a centimetre or two
    wide, so the wax's anisotropic sheen jumps from band to band as you move. The photo layer's grain direction (its
    tangent) is turned in the surface's plane by a banded noise (Chatoyance: 0 off … 1 about ±40°).
    """
    def wrapped(g, proj, cells=None, relief=False):
        out = layer(g, proj, cells=cells, relief=relief)
        t, nrm = out["tangent"], out["normal"]
        b = g.pins(g.node(unreal.MaterialExpressionCrossProduct), [("A", nrm, None), ("B", t, None)])
        uv0 = g.node(unreal.MaterialExpressionTextureCoordinate)
        band = g.noise(g.append(g.append(g.div(g.mask(uv0, "R"), 0.35), g.div(g.mask(uv0, "G"), 0.014)), 2.7), levels=2)
        turn = g.mul(band, g.scalar("Chatoyance", 0.0, "Figure", 0, 1))
        out["tangent"] = g.unary(unreal.MaterialExpressionNormalize, g.add(t, g.mul(b, turn)))
        return out
    return wrapped


CH_MASTERS = ("M_Ch_Stone", "M_Ch_StoneRelief", "M_Ch_Plaster", "M_Ch_Wood")


def build_masters():
    """
    Chenghuai's masters: materials.py's own M_Stone (and its relief twin), M_Plaster and M_Wood graphs, built under
    Chenghuai's names with weathering() in place of the rooms' grime, and nanmu's chatoyance on the wood's grain. The
    museum's language stays one (the same graphs, the same photographed sets); only the weather is added.
    """
    orig_grime, orig_master, orig_layer = MAT.grime, MAT.master, pbr.layer
    tex = MAT.texture_asset("ch_suhua") or unreal.load_asset("/Engine/EngineResources/DefaultTexture")
    names = {"M_Plaster": "M_Ch_Plaster", "M_Wood": "M_Ch_Wood"}
    out = {}
    try:
        MAT.grime = lambda g, proj, c, rough, foot_default=0.0, dust_default=0.0: weathering(g, proj, c, rough, orig_grime)
        out["M_Ch_Stone"] = MAT.build_stone(tex, "M_Ch_Stone")
        out["M_Ch_StoneRelief"] = MAT.build_stone(tex, "M_Ch_StoneRelief", relief=True)
        MAT.master = lambda name, sm: orig_master(names.get(name, name), sm)
        out["M_Ch_Plaster"] = MAT.build_plaster()
        pbr.layer = chatoyant(orig_layer)
        out["M_Ch_Wood"] = MAT.build_wood()
    finally:
        MAT.grime, MAT.master, pbr.layer = orig_grime, orig_master, orig_layer
    log(f"masters: {', '.join(out)}")
    return out


WEATHER_WALL = {"RainStreaks": 0.16, "SplashZone": 0.28, "MossCover": 0.3, "MossUp": 0.5, "MossFoot": 0.8}


def brick(key, rough, colour, weather=True):
    """Grey brick (青砖), reduction-fired, from the photographed set: each brick its own tone and face, the joints the set's
    (CH_BRICK: lime-pointed, 淌白; CH_BRICKRUB: rubbed and dry-laid, a hairline, 干摆 and 丝缝), the wall's slow waviness,
    the weather on the outside faces."""
    s = MAT.merged(NJ, UB, StrataAmount=0.0, PoreAmount=0.0, VeinAmount=0.0, BlockTone=0.0, MacroScale=3.0, MacroVariation=0.06,
                   Roughness=rough, RoughnessVariation=0.05, ClearCoat=0.0, Specular=0.5, Waviness=0.15, WaveScale=0.6,
                   FootDirt=0.04, **(WEATHER_WALL if weather else {}))
    return MAT.spec("M_Ch_Stone", s, {"BaseColor": lin(colour)},
                    photo=(key, dict(contrast=1.0, normal=1.0, rough_influence=0.7)), switches=MAT.MACRO)


def floor_tile(size, colour, rough, coat, joint):
    """Square floor bricks (方砖): ground smooth, laid on the square with fine joints; the hall's soaked in tung oil."""
    s = MAT.merged(MAT.QUIET_DRAWN, MAT.STONE_FLOOR, StrataAmount=0.0, PoreAmount=0.1, PoreSize=0.004, VeinAmount=0.0,
                   CourseHeight=size, BlockLength=size, FloorSlab=size, FloorAspect=1.0, FloorStagger=0.0, WallJoints=1.0,
                   FloorJoints=1.0, JointWidth=joint, JointDarkness=0.72, BlockTone=0.2, BlockHue=0.012, Roughness=rough,
                   RoughnessVariation=0.07, ClearCoat=coat, ClearCoatRoughness=0.3, SlabTilt=0.2, TrafficWear=0.9, RoughWear=0.12,
                   Specular=0.5)
    return MAT.spec("M_Ch_Stone", s, {"BaseColor": lin(colour)},
                    photo=("K1", dict(block_shift=1.0, contrast=0.9, normal=0.7, rough_influence=0.5)), switches=MAT.MACRO)


def paint(colour, rough, coat, coat_rough=0.2, outdoor=True):
    """Paint on the lime-and-hemp ground (一麻五灰 地仗) under a coat of boiled tung oil (光油): no grain shows; the oil's
    gloss, a little orange peel, and the fine crazing of an old coat (断纹, CH_CRAZE: the cracks' dirt, each cell its
    own fade); worn thinner where hands and shoulders rub; dust and the foot's dirt outdoors."""
    s = MAT.merged(MAT.OAK_BOARDS, GrainAmount=0.0, FigureAmount=0.0, BoardWidth=1.0, BoardLength=40.0, BoardTone=0.03, BoardHue=0.01,
                   BoardJoint=0.0, Variation=0.05, Roughness=rough, ClearCoat=coat, ClearCoatRoughness=coat_rough, LacquerWear=0.3,
                   Anisotropy=0.0, Specular=0.5, PBRBlockShift=0.0,
                   **({"FootDirt": 0.06, "DustUp": 0.06, "SplashZone": 0.15} if outdoor else {}))
    return MAT.spec("M_Ch_Wood", s, {"BaseColor": lin(colour)},
                    photo=("CH_CRAZE", dict(colour_amount=0.3, contrast=0.8, normal=0.7, rough_influence=0.6, use_uv0=False)))


def image(texture, rough, coat=0.0, spec=0.5, craze=0.0):
    """An atlas painted or lettered (UV0): the painted beams, the plaques; `craze` lays the old paint's crazing over it."""
    s = MAT.merged(NJ, UB, StrataAmount=0.0, PoreAmount=0.0, VeinAmount=0.0, BlockTone=0.0, MacroVariation=0.03, Roughness=rough,
                   RoughnessVariation=0.05, ClearCoat=coat, ClearCoatRoughness=0.2, TexScale=1.0, TexAspect=1.0, Specular=spec)
    photo = ("CH_CRAZE", dict(colour_amount=craze, contrast=1.2, normal=craze, rough_influence=0.6)) if craze else None
    return MAT.spec("M_Ch_Stone", s, {"BaseColor": (1.0, 1.0, 1.0)}, textures={"Texture": texture}, switches={"UseTexture": True}, photo=photo)


def specs():
    SW = MAT.STONE_WALL
    out = {}
    # ---- Masonry (Materials board: 干摆, 丝缝, 淌白): the photographed grey brick, Beijing's #7A7C79 (Y ≈ 0.19).
    out["BrickFine"] = brick("CH_BRICKRUB", 0.72, 0x7C7C78)
    out["BrickHairline"] = brick("CH_BRICKRUB", 0.76, 0x787874)
    out["BrickPointed"] = brick("CH_BRICK", 0.82, 0x767571)
    # ---- Plaster: the garden's and courts' white lime render (粉墙: its mottle photographed, streaked under the copings,
    #      green at the foot), the rooms' whitened hemp lime (大白: a warm neutral, a stone quieter than the lime outside).
    out["PlasterWhite"] = MAT.spec("M_Ch_Plaster", dict({"Roughness": 0.93, "Variation": 0.03, "GrainAmount": 0.02, "Waviness": 0.8, "WaveScale": 0.4,
                                                        "FootDirt": 0.06, "DustUp": 0.04}, **dict(WEATHER_WALL, RainStreaks=0.22, MossUp=0.0)),
                                   {"BaseColor": lin(0xE3DFD5)}, photo=("CH_LIME", dict(colour_amount=0.6, contrast=0.8, normal=0.6, rough_influence=0.3)),
                                   switches=MAT.MACRO)
    out["PlasterRoom"] = MAT.spec("M_Ch_Plaster", {"Roughness": 0.9, "Variation": 0.025, "GrainAmount": 0.015, "Waviness": 0.6, "WaveScale": 0.4,
                                                   "FootDirt": 0.05},
                                  {"BaseColor": lin(0xD4D0C6)}, photo=("CH_LIME", dict(colour_amount=0.3, contrast=0.5, normal=0.4, rough_influence=0.3)),
                                  switches=MAT.MACRO)
    # ---- Stone: 青白石 (Fangshan's grey-white marble) honed and weathered; bluestone (青石) darker, sawn.
    kerb = MAT.merged(NJ, SW, StrataAmount=0.0, VeinAmount=0.04, PoreAmount=0.05, CourseHeight=60.0, BlockLength=1.4, WallJoints=1.0, FloorJoints=1.0,
                      FloorSlab=1.2, JointWidth=0.002, JointDarkness=0.6, BlockTone=0.1, Roughness=0.6, ClearCoat=0.0, Specular=0.58,
                      RainStreaks=0.12, SplashZone=0.2, MossCover=0.16, MossUp=0.35, MossFoot=0.7, TrafficWear=0.5)
    out["StoneKerb"] = MAT.spec("M_Ch_Stone", kerb, {"BaseColor": lin(0x9FA09A), "VeinColor": lin(0x8C8E89)},
                                photo=("S3", dict(block_shift=1.0, contrast=1.35, normal=0.9, rough_influence=0.5)))
    blue = MAT.merged(MAT.QUIET_DRAWN, MAT.STONE_FLOOR, StrataAmount=0.03, PoreAmount=0.05, VeinAmount=0.0, FloorSlab=0.6, FloorAspect=1.5,
                      FloorStagger=0.5, CourseHeight=0.35, BlockLength=0.9, JointWidth=0.003, JointDarkness=0.6, BlockTone=0.14, Roughness=0.62,
                      ClearCoat=0.0, Specular=0.52, TrafficWear=0.5, SplashZone=0.2, MossCover=0.2, MossUp=0.3, MossFoot=0.8)
    out["BlueStone"] = MAT.spec("M_Ch_Stone", blue, {"BaseColor": lin(0x5D615E)},
                                photo=("S3", dict(block_shift=1.0, contrast=0.8, normal=0.8, rough_influence=0.5)), switches=MAT.MACRO)
    # ---- Floors: 尺七方砖 soaked in tung oil (the hall, the ear rooms); 尺四 ground-jointed (the rest); the courts' paving
    #      (photographed: moss and dirt in its joints); city bricks on edge for the paths.
    out["FloorLarge"] = floor_tile(0.54, 0x4F5153, 0.45, 0.25, 0.0015)
    out["FloorSmall"] = floor_tile(0.45, 0x585B5C, 0.64, 0.0, 0.002)
    court = MAT.merged(NJ, UB, StrataAmount=0.0, PoreAmount=0.0, BlockTone=0.0, MacroScale=4.0, MacroVariation=0.08, Roughness=0.82,
                       RoughnessVariation=0.06, ClearCoat=0.0, TrafficWear=0.6, RoughWear=0.08, Specular=0.5, Waviness=0.12, WaveScale=0.7,
                       MossCover=0.14, MossUp=0.25, MossFoot=0.0)
    out["CourtPaving"] = MAT.spec("M_Ch_Stone", court, {"BaseColor": lin(0x6E6F6B)},
                                  photo=("CH_PAVING", dict(contrast=1.0, normal=1.0, rough_influence=0.7)), switches=MAT.MACRO)
    out["CourtPath"] = MAT.spec("M_Ch_Stone", MAT.merged(court, MossCover=0.1, Roughness=0.8), {"BaseColor": lin(0x6A6C69)},
                                photo=("CH_BRICKRUB", dict(contrast=1.0, normal=1.0, rough_influence=0.7)), switches=MAT.MACRO)
    # ---- The garden's ground: pebble mosaic (花街铺地), earth under moss, the rockery's Taihu limestone (photographed,
    #      displaced 3 cm by its own relief: the stone's holes and ridges in the light), the pond's dark bed.
    out["GardenPebble"] = MAT.spec("M_StoneRelief", MAT.merged(NJ, UB, StrataAmount=0.0, PoreAmount=0.0, BlockTone=0.0, MacroVariation=0.06,
                                                              Roughness=0.72, ClearCoat=0.0, Specular=0.5),
                                   {"BaseColor": lin(0x8A877F)}, photo=("G2", dict(contrast=1.0, normal=1.0, rough_influence=0.6, relief_cm=0.6)))
    out["GardenGround"] = MAT.spec("M_Stone", MAT.merged(NJ, UB, StrataAmount=0.0, PoreAmount=0.0, BlockTone=0.0, MacroScale=2.0, MacroVariation=0.14,
                                                        Roughness=0.93, ClearCoat=0.0, Specular=0.4),
                                   {"BaseColor": lin(0x49492F)}, photo=("CH_MOSSGROUND", dict(contrast=1.0, normal=1.0, rough_influence=0.5)))
    rock = MAT.merged(NJ, UB, StrataAmount=0.0, PoreAmount=0.0, BlockTone=0.0, MacroScale=2.5, MacroVariation=0.14, Roughness=0.84, ClearCoat=0.0,
                      Specular=0.55, RainStreaks=0.35, SplashZone=0.25, MossCover=0.42, MossUp=1.0, MossFoot=0.8, DustUp=0.0)
    out["Rockery"] = MAT.spec("M_Ch_StoneRelief", rock, {"BaseColor": lin(0x6D6C68)},
                              photo=("CH_TAIHU", dict(colour_amount=0.6, contrast=1.1, normal=1.0, rough_influence=0.5, relief_cm=3.0)))
    out["PondBed"] = MAT.spec("M_Stone", MAT.merged(NJ, UB, StrataAmount=0.0, PoreAmount=0.0, BlockTone=0.0, MacroVariation=0.1, Roughness=0.45,
                                                   ClearCoat=0.0, Specular=0.5),
                              {"BaseColor": lin(0x2A2A22)}, photo=("G5", dict(contrast=0.8, normal=0.7, rough_influence=0.4)))
    # ---- Timber: iron-oxide red (铁红) on the lime-and-hemp ground, green and black lacquer (all crazed, oiled), waxed nanmu,
    #      chestnut lacquer.
    out["RedLacquer"] = paint(0x64251F, 0.52, 0.3)
    out["GreenLacquer"] = paint(0x284A3D, 0.5, 0.4)
    out["BlackLacquer"] = paint(0x161310, 0.42, 0.55, 0.12)
    # 楠木, waxed, never painted: golden brown, a fine straight grain (CH_NANMU along each member), the silk of its
    # interlocked grain in the wax's anisotropic sheen (Chatoyance), F0 0.036.
    out["Nanmu"] = MAT.spec("M_Ch_Wood", MAT.merged(MAT.OAK_BOARDS, Roughness=0.36, BoardWidth=0.18, BoardTone=0.05, BoardHue=0.015, BoardJoint=0.1,
                                                    Specular=0.45, ClearCoat=0.12, ClearCoatRoughness=0.22, Anisotropy=-0.55, Chatoyance=0.7),
                            {"BaseColor": lin(0x74532E)},
                            photo=("CH_NANMU", dict(contrast=0.9, normal=0.7, rough_influence=0.8, use_uv0=True, swap_uv=False, block_shift=1.0,
                                                    scale=1.2)))
    # The rafters and the boarding over them (椽, 望板): the same iron red, darkened by a century of lamp smoke and dust
    # (the renderings' dark ceilings; the bright red bounced pink onto every wall under it).
    out["RafterRed"] = paint(0x4A1E19, 0.62, 0.15, outdoor=False)
    out["Chestnut"] = MAT.spec("M_Ch_Wood", MAT.merged(MAT.CHESTNUT_LACQUER, FootDirt=0.06, DustUp=0.06, SplashZone=0.15), {"BaseColor": lin(0x47301F)},
                               photo=("W2", MAT.OAK_PHOTO))
    out["PaintedBeam"] = image("ch_suhua", 0.62, 0.08, craze=0.25)
    out["Engraved"] = image("ch_shutiao", 0.6, 0.0, spec=0.5)
    out["Plaques"] = image("ch_plaques", 0.35, 0.45, craze=0.25)
    out["Brass"] = MAT.spec("M_Metal", {"Roughness": 0.36, "Metallic": 1.0, "Variation": 0.14, "NoiseScale": 0.1, "PatinaAmount": 0.25},
                            {"BaseColor": lin(0xC9A266), "PatinaColor": lin(0x4A4636)},
                            photo=("M1", dict(colour_amount=0.4, normal=0.4, rough_influence=0.5, metal_amount=1.0)))
    # ---- The cases: silk over their backs and beds (a warm grey 绫, quiet behind ink and celadon).
    out["CaseSilk"] = MAT.spec("M_Fabric", {"Roughness": 0.66, "SheenAmount": 0.22, "SlubAmount": 0.05, "WeaveAmount": 0.1, "MottleAmount": 0.03,
                                            "Specular": 0.35},
                               {"BaseColor": lin(0x9C9685), "SheenTint": lin(0xD8D3C6)},
                               photo=("F1", dict(colour_amount=0.3, contrast=0.5, normal=0.5, rough_influence=0.4, scale=0.3)))
    # ---- The scrolls' mounts (装裱): pale silk damask for the fields, a darker blue-grey silk for the bands and
    #      borders, ivory-toned knobs (bone, as the museum's reproductions use), the sticks in dark lacquer.
    out["MountSilk"] = MAT.spec("M_Fabric", {"Roughness": 0.6, "SheenAmount": 0.28, "SlubAmount": 0.04, "WeaveAmount": 0.12, "MottleAmount": 0.05,
                                             "Specular": 0.35},
                                {"BaseColor": lin(0xC3BA9F), "SheenTint": lin(0xE6E0D0)},
                                photo=("F1", dict(colour_amount=0.25, contrast=0.5, normal=0.5, rough_influence=0.4, scale=0.25)))
    out["MountBand"] = MAT.spec("M_Fabric", {"Roughness": 0.58, "SheenAmount": 0.3, "SlubAmount": 0.03, "WeaveAmount": 0.14, "MottleAmount": 0.04,
                                             "Specular": 0.35},
                                {"BaseColor": lin(0x55606A), "SheenTint": lin(0x9AA3AA)},
                                photo=("F1", dict(colour_amount=0.25, contrast=0.5, normal=0.5, rough_influence=0.4, scale=0.25)))
    out["Knob"] = MAT.spec("M_Stone", MAT.merged(NJ, UB, StrataAmount=0.0, PoreAmount=0.0, VeinAmount=0.0, BlockTone=0.0, MacroVariation=0.04,
                                                Roughness=0.36, ClearCoat=0.3, ClearCoatRoughness=0.2, Specular=0.5),
                           {"BaseColor": lin(0xE2D6BC)})
    # ---- Roofs: reduction-fired grey clay tiles (小青瓦, 筒瓦) weathered as old roofs are (CH_LICHEN: lichen and moss
    #      in patches, heavier where the slope faces the sky), their lime bed, the ridges' darker brick.
    roof = MAT.merged(NJ, UB, StrataAmount=0.0, PoreAmount=0.06, PoreSize=0.003, BlockTone=0.0, MacroScale=3.0, MacroVariation=0.14, Roughness=0.82,
                      RoughnessVariation=0.08, ClearCoat=0.0, Specular=0.5, DustUp=0.0, MossCover=0.18, MossUp=0.8, MossFoot=0.0)
    out["Tiles"] = MAT.spec("M_Ch_Stone", roof, {"BaseColor": lin(0x4D4F4D)},
                            photo=("CH_LICHEN", dict(colour_amount=0.55, contrast=0.9, normal=0.7, rough_influence=0.6, scale=1.4)))
    out["RoofMortar"] = MAT.spec("M_Ch_Plaster", {"Roughness": 0.93, "Variation": 0.06, "GrainAmount": 0.04, "RainStreaks": 0.2},
                                 {"BaseColor": lin(0x8A877F)}, photo=("CH_LIME", dict(colour_amount=0.5, normal=0.6, rough_influence=0.3)))
    out["Ridges"] = MAT.spec("M_Ch_Stone", MAT.merged(roof, MossCover=0.12, RainStreaks=0.2), {"BaseColor": lin(0x484A48)},
                             photo=("CH_BRICKRUB", dict(contrast=0.9, normal=0.8, rough_influence=0.5)))
    # ---- 石兄: the standing Taihu rock (AMuseeTaihuRock's mesh, its RockMaterial): the same photographed limestone, paler
    #      where the rain washes it, dark in its hollows and streaks, moss on its ledges.
    out["Taihu"] = MAT.spec("M_Ch_Stone", MAT.merged(rock, RainStreaks=0.4, MossCover=0.3), {"BaseColor": lin(0x7E7C76)},
                            photo=("CH_TAIHU", dict(colour_amount=0.85, contrast=1.1, normal=1.0, rough_influence=0.5, scale=1.6)))
    return out


def make_paper():
    """
    M_Ch_Paper: mulberry paper laminated in glass (桑皮纸 夹层). Two-sided foliage: the paper takes the sun on its outer face
    and glows on the inner (the lattice's shadow in it, as in a real paper window); a thin glossy sheen of the glass over it;
    a little emissive by day (MPC_Musee Daylight) for the sky's diffuse light through it, which Lumen doesn't carry.
    """
    m = S.new_material(FOLDER, "M_Ch_Paper")
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    m.set_editor_property("two_sided", True)
    tone = MEL.create_material_expression(m, unreal.MaterialExpressionNoise, -900, 0)
    try:
        tone.set_editor_property("scale", 12.0)
        tone.set_editor_property("levels", 3)
    except Exception:  # noqa: BLE001
        pass
    wp = MEL.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -1100, 0)
    MEL.connect_material_expressions(wp, "", tone, "Position")
    base = S.constant3(m, lin(0xE6DCC6), -900, -200)     # old mulberry paper: a warm cream, not white
    var = S.multiply(m, tone, S.scalar_param(m, "Variation", 0.04, -900, 150), -700, 100)
    one = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -700, 250)
    one.set_editor_property("r", 1.0)
    add = MEL.create_material_expression(m, unreal.MaterialExpressionAdd, -550, 100)
    MEL.connect_material_expressions(var, "", add, "A")
    MEL.connect_material_expressions(one, "", add, "B")
    col = S.multiply(m, base, add, -400, -100)
    MEL.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    sss = S.multiply(m, S.constant3(m, lin(0xEFE7D6), -700, 400), S.scalar_param(m, "Transmission", 0.85, -700, 500), -450, 400)
    MEL.connect_material_property(sss, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    rough = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 300)
    rough.set_editor_property("r", 0.42)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mpc = unreal.load_asset(f"{P.MATERIALS}/MPC_Musee")
    glow = S.multiply(m, S.multiply(m, base, S.scalar_param(m, "Glow", 750.0, -700, 700), -450, 650), S.daylight_factor(m, mpc, 0.0, -700, 800), -250, 650)
    MEL.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m)
    return m


def make_lamp():
    """M_Ch_Lamp: the cases' lamp strips seen through their baffles, warm (about 3000 K), a soft diffuser's luminance."""
    m = S.new_material(FOLDER, "M_Ch_Lamp")
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    glow = S.multiply(m, S.constant3(m, unreal.LinearColor(1.0, 0.72, 0.46, 1), -600, 0), S.scalar_param(m, "Luminance", 900.0, -600, 200), -300, 0)
    MEL.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m)
    return m


def make_lantern():
    """M_Ch_Lantern: a paper lantern's mulberry paper (纱灯), lit from within after dusk (glow × (1 − Daylight)^6): the
    paper's own warm cream by day, its lamp's 2500 K through it at night, brighter at the panel's middle than at its
    frame (the lamp is a small source a hand's breadth away), never a flat white disc."""
    m = S.new_material(FOLDER, "M_Ch_Lantern")
    m.set_editor_property("two_sided", True)
    paper = S.constant3(m, lin(0xE6DCC4), -700, -200)
    MEL.connect_material_property(paper, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 100)
    rough.set_editor_property("r", 0.8)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mpc = unreal.load_asset(f"{P.MATERIALS}/MPC_Musee")
    d = MEL.create_material_expression(m, unreal.MaterialExpressionCollectionParameter, -900, 400)
    d.set_editor_property("collection", mpc)
    d.set_editor_property("parameter_name", "Daylight")
    inv = MEL.create_material_expression(m, unreal.MaterialExpressionOneMinus, -700, 400)
    MEL.connect_material_expressions(d, "", inv, "")
    sat = MEL.create_material_expression(m, unreal.MaterialExpressionSaturate, -550, 400)
    MEL.connect_material_expressions(inv, "", sat, "")
    pw = MEL.create_material_expression(m, unreal.MaterialExpressionPower, -420, 400)
    MEL.connect_material_expressions(sat, "", pw, "Base")
    e = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -560, 520)
    e.set_editor_property("r", 6.0)
    MEL.connect_material_expressions(e, "", pw, "Exp")
    # The panel's falloff from its middle: 1 - |2·frac(z / H) - 1|^2 along the world height would need the lantern's
    # size; a soft noise in the paper's fibres (5 mm) and its warm 2500 K colour do most of it.
    fib = MEL.create_material_expression(m, unreal.MaterialExpressionNoise, -900, 650)
    try:
        fib.set_editor_property("scale", 2.0)
        fib.set_editor_property("levels", 2)
        fib.set_editor_property("output_min", 0.8)
        fib.set_editor_property("output_max", 1.0)
    except Exception:  # noqa: BLE001
        pass
    wp = MEL.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -1100, 650)
    MEL.connect_material_expressions(wp, "", fib, "Position")
    glow = S.multiply(m, S.multiply(m, S.constant3(m, unreal.LinearColor(1.0, 0.56, 0.27, 1), -700, 200),
                                    S.scalar_param(m, "Luminance", 260.0, -700, 280), -450, 200), pw, -250, 300)
    glow = S.multiply(m, glow, fib, -150, 400)
    MEL.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m)
    return m


def make_pond():
    """MI_Ch_Pond: a Suzhou garden pond (the museum's M_Water, Single Layer Water): still (the faintest ripple under the
    leaves), jade to olive-brown with the algae and silt of an old pond, its bed gone by 30 cm, so it reads as a body
    of green water that holds the reflections of the walls and trees rather than a blue mirror of the sky."""
    parent = unreal.load_asset(f"{P.MATERIALS}/M_Water")
    if parent is None:
        warn("M_Water missing: the pond keeps the museum's garden water")
        return None
    path = f"{FOLDER}/MI_Ch_Pond"
    mi = unreal.load_asset(path) if EAL.does_asset_exist(path) else unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "MI_Ch_Pond", FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, parent)
    MEL.set_material_instance_vector_parameter_value(mi, "Absorption", unreal.LinearColor(0.06, 0.02, 0.06, 1))
    MEL.set_material_instance_vector_parameter_value(mi, "Scattering", unreal.LinearColor(0.006, 0.014, 0.008, 1))
    MEL.set_material_instance_scalar_parameter_value(mi, "Ripple", 0.025)
    MEL.set_material_instance_scalar_parameter_value(mi, "Roughness", 0.045)
    MEL.set_material_instance_scalar_parameter_value(mi, "ColorScaleBehindWater", 0.7)
    MEL.update_material_instance(mi)
    EAL.save_loaded_asset(mi)
    return mi


def make_case_glass():
    """
    M_Ch_CaseGlass: the cases' anti-reflective low-iron laminated glass (Hang board). The museum's M_Glass (Thin
    Translucent, translucent velocity) showed a milky haze and a bright smear in these small lit cases (a diagnostic run
    with translucency off cleared both), so the cases get plain surface-lit translucency: a clear pane with an AR
    coating's faint reflection (F0 about 0.005), a trace of the laminate's green in what passes, and no velocity.
    """
    m = S.new_material(FOLDER, "M_Ch_CaseGlass")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("two_sided", True)
    for prop, value in (("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING),
                        ("output_translucent_velocity", False)):
        try:
            m.set_editor_property(prop, value)
        except Exception as e:  # noqa: BLE001
            warn(f"M_Ch_CaseGlass: {prop} not set ({e})")
    MEL.connect_material_property(S.constant3(m, unreal.LinearColor(0.01, 0.012, 0.011, 1), -500, -100), "", unreal.MaterialProperty.MP_BASE_COLOR)
    for prop, value, y in ((unreal.MaterialProperty.MP_OPACITY, 0.035, 0), (unreal.MaterialProperty.MP_ROUGHNESS, 0.03, 100),
                           (unreal.MaterialProperty.MP_SPECULAR, 0.04, 200), (unreal.MaterialProperty.MP_METALLIC, 0.0, 300)):
        c = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, y)
        c.set_editor_property("r", value)
        MEL.connect_material_property(c, "", prop)
    errs = list(MEL.recompile_material(m) or [])
    if errs:
        warn(f"M_Ch_CaseGlass: {errs}")
    EAL.save_loaded_asset(m)
    return m


def make_materials(rebuild_masters=False):
    if not EAL.does_directory_exist(FOLDER):
        EAL.make_directory(FOLDER)
    add_stats()
    if rebuild_masters or not all(EAL.does_asset_exist(f"{P.MATERIALS}/{m}") for m in CH_MASTERS):
        build_masters()
    for name in ("M_Stone", "M_StoneRelief", "M_Metal", "M_Fabric", "M_Wood", "M_Plaster") + CH_MASTERS:
        m = unreal.load_asset(f"{P.MATERIALS}/{name}")
        if m:
            MAT.PARAMS[name] = MAT.parameter_names(m)
    textures = {t: MAT.texture_asset(t) for t in ("ch_suhua", "ch_plaques", "ch_shutiao")}
    for t in textures.values():
        if t:
            t.set_editor_property("srgb", True)
            EAL.save_loaded_asset(t)
    made = []
    for part, s in specs().items():
        parent = unreal.load_asset(f"{P.MATERIALS}/{s['parent']}")
        if parent is None:
            warn(f"{part}: parent {s['parent']} missing")
            continue
        if MAT.material_instance(f"MI_Ch_{part}", FOLDER, parent, s, textures):
            made.append(part)
    if make_pond():
        made.append("MI_Ch_Pond")
    # The paper, the glass (the museum's clear low-iron glass), the cases' lamps, the guard (never drawn).
    paper = make_paper()
    for name, parent in (("MI_Ch_Paper", paper), ("MI_Ch_Glass", make_case_glass()),
                         ("MI_Ch_CaseLamp", make_lamp()), ("MI_Ch_LanternSilk", make_lantern()),
                         ("MI_Ch_Guard", unreal.load_asset(f"{P.MATERIALS}/M_Plaster"))):
        path = f"{FOLDER}/{name}"
        mi = unreal.load_asset(path) if EAL.does_asset_exist(path) else unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        MEL.set_material_instance_parent(mi, parent)
        MEL.update_material_instance(mi)
        EAL.save_loaded_asset(mi)
        made.append(name)
    EAL.save_directory(FOLDER, only_if_is_dirty=True, recursive=True)
    log(f"materials: {len(made)} ({', '.join(made)})" + (f"; {len(MAT.WARNINGS)} warnings" if MAT.WARNINGS else ""))


# ---------------------------------------------------------------------------------------------- the wing in the map

def tag_value(actor, prefix):
    for t in actor.tags:
        s = str(t)
        if s.startswith(prefix):
            return s[len(prefix):]
    return None


def retire(actor):
    actor.set_actor_hidden_in_game(True)
    actor.set_actor_enable_collision(False)
    actor.set_is_temporarily_hidden_in_editor(True)
    tags = [t for t in actor.tags if str(t) != "musee.building"]
    if "musee.retired" not in [str(t) for t in tags]:
        tags.append(unreal.Name("musee.retired"))
    actor.tags = tags


def retire_sculpture_hall(eas):
    """The Sculpture Hall (empty since its scans were withdrawn) gives the north door to Chenghuai: its native room goes
    from the map (its code stays), its imported pieces and its laylight's area light are hidden for good."""
    removed, hidden = 0, 0
    cls = unreal.load_class(None, "/Script/MuseeVision.SculptureHallStructure")
    for a in eas.get_all_level_actors():
        if cls and a.get_class() == cls:
            eas.destroy_actor(a)
            removed += 1
            continue
        wing = tag_value(a, "musee.wing:")
        prim = tag_value(a, "prim:") or ""
        if wing == "SculptureHall" or prim.startswith("/Museum/SculptureHall"):
            if a.get_components_by_class(unreal.LightComponent):
                eas.destroy_actor(a)   # its laylight's area light and accents
                removed += 1
            elif "musee.retired" not in [str(t) for t in a.tags] or not a.is_hidden_ed():
                retire(a)
                hidden += 1
    log(f"Sculpture Hall: {removed} actors removed, {hidden} imported pieces retired")


def place_structure(eas):
    cls = unreal.load_class(None, CLASS)
    if not cls:
        warn("no AChenghuaiStructure: build the editor module first")
        return None
    for a in eas.get_all_level_actors():
        if a.get_class() == cls:
            eas.destroy_actor(a)
    actor = eas.spawn_actor_from_class(cls, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    actor.set_actor_label("Chenghuai (native)")
    actor.tags = [unreal.Name("musee.building"), unreal.Name(TAG), unreal.Name("musee.native"), unreal.Name("musee.bakeable")]
    log("placed AChenghuaiStructure")
    return actor


def keep_lawn_off(eas):
    """The grounds' grass blades stay off the site (the courts are paved, the garden has its own ground)."""
    for a in eas.get_all_level_actors():
        if a.get_class().get_name() == "MuseeLawn":
            ex = list(a.get_editor_property("exclusions"))
            box = unreal.Vector4(SITE[0] - 0.1, SITE[1] - 0.1, SITE[2] + 0.1, SITE[3] + 0.1)
            if not any(abs(e.x - box.x) < 0.01 and abs(e.y - box.y) < 0.01 for e in ex):
                ex.append(box)
                a.set_editor_property("exclusions", ex)
                log("lawn: the site excluded")


def place(eas):
    retire_sculpture_hall(eas)
    place_structure(eas)
    keep_lawn_off(eas)


def native_place(eas):
    """native.py's hook (after its ROOMS have placed AChenghuaiStructure): the Sculpture Hall's native room and lights out
    of the map (its code stays), then the works, their mounts and the plants (a run replaces the last run's)."""
    retire_sculpture_hall(eas)
    import chenghuai_works
    chenghuai_works.main(eas)


TEST_MAP = "/Game/Maps/ChenghuaiTest"


def main(argv):
    """Steps as above; `testmap` first copies the museum's map to /Game/Maps/ChenghuaiTest and the later steps act on
    that copy (for trying the wing without touching the real map; never cooked: DefaultGame.ini cooks Museum only)."""
    steps = [a for a in argv if not a.startswith("-")] or ["materials", "place"]
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if "materials" in steps or "masters" in steps:
        make_materials(rebuild_masters="masters" in steps)
    target = P.MAP_PATH
    if "testmap" in steps:
        if EAL.does_asset_exist(TEST_MAP):
            EAL.delete_asset(TEST_MAP)
        if not EAL.duplicate_asset(P.MAP_PATH, TEST_MAP):
            warn("could not copy the map")
            return
        target = TEST_MAP
        log(f"working on {TEST_MAP}")
    elif "ontest" in steps:
        target = TEST_MAP
    if any(s in steps for s in ("place", "works")):
        les.load_level(target)
        if "place" in steps:
            place(eas)
        if "works" in steps:
            import chenghuai_works
            chenghuai_works.main(eas)
        les.save_current_level()
    log("done")


if __name__ == "__main__":
    main(sys.argv[1:])
