"""
The photographed layer every master can carry: one PBR texture set of the museum's library (windows/SourceArt/PBR,
fetched by pbr_fetch.py; the materials spec's material language), sampled in metres and turned into
- a colour factor: the photograph's variation around its own mean, so the instance's BaseColor stays the albedo the
  spec wants (PBRColorAmount 0 keeps only relief and polish, as on the Rotunda's walls);
- a world-space normal (the relief the ray-traced light catches; NormalStrength);
- a roughness offset (the photograph's roughness around its mean: polish that varies as real stone's does);
- ambient occlusion, metalness and height (the relief master displaces with it, Nanite tessellation).

Projection: world metres, box-projected along the walls (u along the course, v up) and on floors (x, y), as the
stone's coursing is, the axis chosen per pixel by the normal (dithered where two meet, so a curved face has no
seam); or UV0 in metres (PBRUseUV0: brick that follows a vault, oak along its members). Where the master has blocks (ashlar,
slabs, bricks), each block takes its own slice of the photograph and is mirrored at random (PBRBlockShift), as
stone cut from one quarry is: no block repeats its neighbour.
"""
import json
import os

import unreal

import musee_paths as P  # noqa: E402

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

HERE = os.path.dirname(os.path.abspath(__file__))
LIBRARY = os.path.join(HERE, "..", "SourceArt", "PBR")
FOLDER = P.MUSEUM_CONTENT + "/Textures/PBR"
MAPS = ("Color", "Normal", "ORM", "Height")
_STATS = None
_CACHE = {}


def stats():
    global _STATS
    if _STATS is None:
        with open(os.path.join(LIBRARY, "stats.json"), encoding="utf-8") as f:
            _STATS = json.load(f)
    return _STATS


def _enum(cls, *names):
    for n in names:
        v = getattr(cls, n, None)
        if v is not None:
            return v
    return None


def _configure(tex, kind):
    """Each map as the renderer reads it: colour sRGB (BC1/BC7), normal BC5, ORM linear BC7 with the normal's
    variance folded into the roughness mips (no sparkle on polished floors), height BC4."""
    T = unreal.TextureCompressionSettings
    props = {
        "Color": {"compression_settings": T.TC_DEFAULT, "srgb": True},
        "Normal": {"compression_settings": T.TC_NORMALMAP, "srgb": False},
        "ORM": {"compression_settings": _enum(T, "TC_BC7") or T.TC_MASKS, "srgb": False},
        # Grayscale (the sampler reads it as linear grayscale; an alpha-compressed height failed the relief master).
        "Height": {"compression_settings": _enum(T, "TC_GRAYSCALE"), "srgb": False},
    }[kind]
    groups = {"Color": "TEXTUREGROUP_WORLD", "Normal": "TEXTUREGROUP_WORLD_NORMAL_MAP", "ORM": "TEXTUREGROUP_WORLD_SPECULAR",
              "Height": "TEXTUREGROUP_WORLD"}
    g = _enum(unreal.TextureGroup, groups[kind])
    if g is not None:
        props["lod_group"] = g
    lossy = _enum(getattr(unreal, "TextureLossyCompressionAmount", object), "TLCA_NONE")
    if lossy is not None:
        props["lossy_compression_amount"] = lossy
    changed = False
    for k, v in props.items():
        try:
            if tex.get_editor_property(k) != v:
                tex.set_editor_property(k, v)
                changed = True
        except Exception as e:  # noqa: BLE001
            unreal.log_warning(f"[pbr] {tex.get_name()}.{k}: {e}")
    return changed


STAMPS = os.path.join(HERE, "..", "Saved", "pbr_import_stamps.json")
_STAMP_DATA = None


def _stamps():
    global _STAMP_DATA
    if _STAMP_DATA is None:
        try:
            with open(STAMPS, encoding="utf-8") as f:
                _STAMP_DATA = json.load(f)
        except Exception:  # noqa: BLE001
            _STAMP_DATA = {}
    return _STAMP_DATA


def _save_stamps():
    try:
        with open(STAMPS, "w", encoding="utf-8") as f:
            json.dump(_stamps(), f, indent=1)
    except Exception as e:  # noqa: BLE001
        unreal.log_warning(f"[pbr] stamps not saved: {e}")


def textures(key):
    """{map: Texture2D} for one set (imported once, and again whenever its PNG changes: pbr_fetch's derived relief)."""
    if key in _CACHE:
        return _CACHE[key]
    out = {}
    folder = f"{FOLDER}/{key}"
    for kind in MAPS:
        png = os.path.join(LIBRARY, key, f"{kind}.png")
        if not os.path.exists(png):
            continue
        name = f"T_{key}_{kind}"
        path = f"{folder}/{name}"
        stamp = f"{os.path.getmtime(png):.0f}:{os.path.getsize(png)}"
        known = _stamps().get(path)
        if known is None:     # no stamp yet: the asset is stale if the PNG is newer than its package
            pkg = os.path.join(HERE, "..", "Content", path[len("/Game/"):] + ".uasset")
            stale = os.path.exists(pkg) and os.path.getmtime(png) > os.path.getmtime(pkg)
        else:
            stale = known != stamp
        if not EAL.does_asset_exist(path) or stale:
            task = unreal.AssetImportTask()
            task.filename = png
            task.destination_path = folder
            task.destination_name = name
            task.automated = True
            task.replace_existing = True
            task.save = False
            TOOLS.import_asset_tasks([task])
        if known != stamp:
            _stamps()[path] = stamp
            _save_stamps()
        tex = unreal.load_asset(path) if EAL.does_asset_exist(path) else None
        if tex is None:
            unreal.log_warning(f"[pbr] {path} not imported")
            continue
        out[kind] = tex
    if "Normal" in out and "ORM" in out:
        orm = out["ORM"]
        try:
            mode = _enum(unreal.CompositeTextureMode, "CTM_NORMAL_ROUGHNESS_TO_GREEN")
            if mode is not None and orm.get_editor_property("composite_texture") != out["Normal"]:
                orm.set_editor_property("composite_texture", out["Normal"])
                orm.set_editor_property("composite_texture_mode", mode)
                orm.set_editor_property("composite_power", 1.0)
        except Exception as e:  # noqa: BLE001
            unreal.log_warning(f"[pbr] {key}: specular antialiasing not set ({e})")
    for kind, tex in out.items():
        _configure(tex, kind)
        EAL.save_loaded_asset(tex, only_if_is_dirty=True)
    _CACHE[key] = out
    return out


def instance_values(key, scale=None, colour_amount=1.0, contrast=1.0, rough_influence=1.0, normal=1.0,
                    block_shift=0.0, use_uv0=False, swap_uv=False, relief_cm=0.0, ao=1.0, metal_amount=0.0, floor_normal=1.0):
    """The scalars, vectors, textures and switches an instance needs for set `key` (textures by name 'PBR:<key>:<map>')."""
    st = stats()[key]
    mean = st["color_mean"]
    scalars = {"PBRScale": float(scale or st["scale"]), "PBRColorAmount": colour_amount, "PBRContrast": contrast,
               "PBRRoughMean": st["rough_mean"], "PBRRoughInfluence": rough_influence, "NormalStrength": normal,
               "PBRSwapUV": 1.0 if swap_uv else 0.0, "PBRAO": ao}
    if block_shift:
        scalars["PBRBlockShift"] = block_shift        # masters with blocks only (M_Stone's slabs, M_Wood's boards)
    if floor_normal != 1.0:
        scalars["FloorNormalScale"] = floor_normal
    if metal_amount:
        scalars["PBRMetalAmount"] = metal_amount
    if relief_cm:
        scalars["PBRReliefCm"] = relief_cm
    vectors = {"PBRMean": (max(mean[0], 1e-3), max(mean[1], 1e-3), max(mean[2], 1e-3))}
    tex = {"PBRColor": f"PBR:{key}:Color", "PBRNormal": f"PBR:{key}:Normal", "PBRORM": f"PBR:{key}:ORM"}
    if relief_cm and st.get("has_height"):
        tex["PBRHeight"] = f"PBR:{key}:Height"
    switches = {"UsePBR": True}
    if use_uv0:
        switches["PBRUseUV0"] = True
    return scalars, vectors, tex, switches


def resolve(name):
    """'PBR:S1:Color' → the texture (None if it isn't one of ours)."""
    if not isinstance(name, str) or not name.startswith("PBR:"):
        return None
    _, key, kind = name.split(":")
    return textures(key).get(kind)


def defaults():
    """Placeholder textures for the masters' PBR parameters (any set's maps: the switch is off by default)."""
    t = textures("S1")
    return t.get("Color"), t.get("Normal"), t.get("ORM"), t.get("Height")


def vec3(g, x, y, z):
    return g.node(unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(float(x), float(y), float(z), 1.0))


def sample(g, name, default, uv, sampler):
    e = g.node(unreal.MaterialExpressionTextureSampleParameter2D, parameter_name=name)
    if default:
        e.set_editor_property("texture", default)
    st = _enum(unreal.MaterialSamplerType, sampler)
    if st is not None:
        e.set_editor_property("sampler_type", st)
    e.set_editor_property("group", "Photo")
    g.link(uv, e, "UVs")
    return e


def layer(g, proj, cells=None, relief=False):
    """
    The photographed layer. proj: the master's projection (materials.box_projection): u, v in metres, floor (1 on
    floors and ceilings), facing_x, and the world directions t_u, t_v of +u and +v. cells: (cell_a, cell_b, blocks_on)
    from a master with blocks (stone slabs, oak boards): each takes its own mirrored, shifted slice.
    Returns {colour, normal (world, normalised, the face's sign applied), rough, ao, ao_raw, metal, height, tangent
    (world direction of the sample's +u: the grain or the brushing, for anisotropy), side}: neutral unless UsePBR is on.
    """
    u, v = proj["u"], proj["v"]
    dc, dn, do, dh = defaults()
    scale = g.scalar("PBRScale", 1.2, "Photo", 0.05, 10)
    uv0 = g.node(unreal.MaterialExpressionTextureCoordinate)
    use_uv0 = lambda a_, b_: g.switch("PBRUseUV0", False, a_, b_, "Photo")  # noqa: E731
    uu = use_uv0(g.mask(uv0, "R"), u)
    vv = use_uv0(g.mask(uv0, "G"), v)
    swap = g.scalar("PBRSwapUV", 0.0, "Photo", 0, 1)
    uu, vv = g.lerp(uu, vv, swap), g.lerp(vv, uu, swap)
    if cells:
        cell_a, cell_b, blocks_on = cells
        shift = g.mul(g.scalar("PBRBlockShift", 0.0, "Photo", 0, 1), blocks_on)
        # Mirror a block's slice by its seed (sign -1 or 1), and offset it.
        su = g.sub(1.0, g.mul(g.mul(g.step(0.5, g.frac(g.mul(cell_a, 7.31))), 2.0), shift))
        sv = g.sub(1.0, g.mul(g.mul(g.step(0.5, g.frac(g.mul(cell_b, 5.17))), 2.0), shift))
        off = g.mul(g.append(cell_a, cell_b), shift)
    else:
        su = sv = g.const(1.0)
        off = None
    uv = g.div(g.append(g.mul(uu, su), g.mul(g.mul(vv, sv), -1.0)), scale)   # image up = up the wall
    if off is not None:
        uv = g.add(uv, off)

    col = sample(g, "PBRColor", dc, uv, "SAMPLERTYPE_COLOR")
    nrm = sample(g, "PBRNormal", dn, uv, "SAMPLERTYPE_NORMAL")
    orm = sample(g, "PBRORM", do, uv, "SAMPLERTYPE_LINEAR_COLOR")

    # Detail fade (PBRDetailFade, off by default): a fine regular pattern (a weave: PBRDetailPeriod metres) near a
    # pixel's footprint beats against the pixel grid, worst at grazing angles where the footprint stretches beyond what
    # anisotropic filtering covers (the Reserve linen's moiré). The footprint is the sample's own screen derivative
    # in metres (its larger axis); as it nears the period, the weave's relief and contrast fade to their average and
    # the lost relief turns into a little roughness, as a real cloth seen from afar.
    m_uv = g.append(uu, vv)
    fx, fy = g.unary(unreal.MaterialExpressionDDX, m_uv), g.unary(unreal.MaterialExpressionDDY, m_uv)
    foot = g.max(g.unary(unreal.MaterialExpressionLength, fx), g.unary(unreal.MaterialExpressionLength, fy))
    period = g.scalar("PBRDetailPeriod", 0.002, "Photo", 0.0002, 0.05)
    # Behind a static switch (PBRDetail): the instances without a fine weave compile none of it.
    detail = g.switch("PBRDetail", False,
                      g.one_minus(g.mul(g.smoothstep(g.mul(period, 0.25), g.mul(period, 0.7), foot),
                                        g.scalar("PBRDetailFade", 1.0, "Photo", 0, 1))), 1.0, "Photo")
    # Colour: the photograph's variation about its mean, at PBRContrast, blended in by PBRColorAmount.
    mean = g.vector("PBRMean", (0.5, 0.5, 0.5), "Photo")
    varied = g.div(g.lerp(mean, (col, "RGB"), g.scalar("PBRContrast", 1.0, "Photo", 0, 1.5)), mean)
    colour = g.lerp(1.0, varied, g.mul(g.scalar("PBRColorAmount", 1.0, "Photo", 0, 1), g.lerp(0.35, 1.0, detail)))

    # Normal (DirectX: red along the image's +x, green towards its bottom). The image's +x is the sample's +uu (times
    # the block's mirror su); its bottom is -vv (the sample's y is -vv * sv). So along +uu: a = r*su; along +vv: b = -g*sv.
    # FloorNormalScale: one stone, two finishes (a travertine's voids open on its walls, filled flush on its floors).
    strength = g.mul(g.mul(g.scalar("NormalStrength", 1.0, "Photo", 0, 3),
                           g.lerp(1.0, g.scalar("FloorNormalScale", 1.0, "Photo", 0, 1), proj["floor"])), detail)
    relief_there = g.sat(strength)          # the occlusion goes with the relief: none where the voids are filled flush
    a = g.mul(g.mul(g.mask(nrm, "R"), strength), su)
    b = g.mul(g.mul(g.mul(g.mask(nrm, "G"), strength), sv), -1.0)
    nz = g.mask(nrm, "B")
    vn = g.node(unreal.MaterialExpressionVertexNormalWS)
    tu = g.lerp(proj["t_u"], proj["t_v"], swap)        # the world direction of +uu
    tv = g.lerp(proj["t_v"], proj["t_u"], swap)        # ... and of +vv
    world_proj = g.add(g.add(g.mul(tu, a), g.mul(tv, b)), g.mul(vn, nz))
    # On UV0, +uu is the mesh's +U (its tangent) or, swapped, its +V (its bitangent).
    ts = g.append(g.append(g.lerp(a, b, swap), g.lerp(b, a, swap)), nz)
    xf = _to_world(g, ts)
    # World-space normals aren't turned over on a two-sided surface's back face (tangent-space ones are): the
    # face's sign does it, or a whitewashed wall seen from inside is lit as if it faced the lawn outside.
    side = g.node(unreal.MaterialExpressionTwoSidedSign)
    normal_ws = g.mul(g.unary(unreal.MaterialExpressionNormalize, use_uv0(xf, world_proj)), side)
    flat_ws = g.mul(vn, side)
    # The grain / brushing direction (the sample's +uu), in world space.
    t_img = use_uv0(_to_world(g, g.append(g.append(g.one_minus(swap), swap), 0.0)), tu)

    rough = g.add(g.mul(g.mul(g.sub((orm, "G"), g.scalar("PBRRoughMean", 0.5, "Photo", 0, 1)),
                              g.scalar("PBRRoughInfluence", 1.0, "Photo", 0, 2)), detail),
                  g.mul(g.one_minus(detail), 0.05))
    ao = g.lerp(1.0, (orm, "R"), g.mul(g.scalar("PBRAO", 1.0, "Photo", 0, 1), relief_there))
    metal = g.mul((orm, "B"), g.scalar("PBRMetalAmount", 0.0, "Photo", 0, 1))
    height = None
    if relief:
        h = sample(g, "PBRHeight", dh, uv, "SAMPLERTYPE_LINEAR_GRAYSCALE")
        height = g.mul((h, "R"), g.scalar("PBRReliefCm", 0.5, "Photo", 0, 3))

    on = lambda a_, b_: g.switch("UsePBR", False, a_, b_, "Photo")  # noqa: E731
    return {"colour": on(colour, 1.0), "normal": on(normal_ws, flat_ws), "rough": on(rough, 0.0), "ao": on(ao, 1.0),
            "ao_raw": on((orm, "R"), 1.0), "metal": on(metal, 0.0), "height": on(height, 0.0) if relief else None,
            "tangent": on(t_img, proj["t_u"]), "side": side}


def _to_world(g, vec):
    xf = g.node(unreal.MaterialExpressionTransform)
    try:
        xf.set_editor_property("transform_source_type", unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_TANGENT)
        xf.set_editor_property("transform_type", unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
    except Exception as e:  # noqa: BLE001
        unreal.log_warning(f"[pbr] transform node: {e}")
    g.link(vec, xf, "")
    return xf


def world_normal_material(m):
    """Normals come out in world space (the photo layer builds them from the projection's axes)."""
    try:
        if m.get_editor_property("tangent_space_normal"):
            m.set_editor_property("tangent_space_normal", False)
    except Exception as e:  # noqa: BLE001
        unreal.log_warning(f"[pbr] {m.get_name()}: tangent_space_normal: {e}")


def relief_material(m):
    """Nanite tessellation on, displacement in cm outward from the surface (centre 0: RT sees the base surface)."""
    try:
        m.set_editor_property("enable_tessellation", True)
        m.set_editor_property("displacement_scaling", unreal.DisplacementScaling(magnitude=1.0, center=0.0))
    except Exception as e:  # noqa: BLE001
        unreal.log_warning(f"[pbr] {m.get_name()}: tessellation: {e}")
