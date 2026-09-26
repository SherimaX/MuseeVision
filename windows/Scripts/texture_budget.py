"""
The museum's colour images (the paintings, scrolls, prints, placards, documents) as the GPU should hold them:
BC7-compressed, with a full mip chain, streamed. The USD import had left 104 paintings uncompressed at up to
8K with one mip and streaming off: 4.4 GB resident at all times, and the game ran out of video memory
("Video memory has been exhausted") as the new wings arrived. Streamed BC7 keeps them as sharp up close (the
top mip is the image stretched to the next power of two, BC7) and lets the far ones drop to the mips they need.

    apply_all.py textures        (runs before the bake)      or on its own:  texture_budget.py [--dry]

Left alone: the PBR sets (their own compression per map), normal maps, masks and height maps, HDR images, the
sky (never streamed on purpose), UI textures and anything already compressed and streamed.
"""
import sys

import unreal

ROOT = "/Game/Museum"
SKIP_FOLDERS = ("/Game/Museum/Textures/PBR", "/Game/Museum/Sky")
T = unreal.TextureCompressionSettings
# Data textures keep their own compression. The paintings came in from the USD import as TC_EDITOR_ICON
# ("UserInterface2D": never compressed, no mips): under /Game/Museum that is only ever a picture, so it is fixed.
KEEP = {getattr(T, n) for n in ("TC_NORMALMAP", "TC_MASKS", "TC_GRAYSCALE", "TC_ALPHA", "TC_HDR", "TC_HDR_COMPRESSED",
                                 "TC_HDR_F32", "TC_DISTANCE_FIELD_FONT", "TC_SINGLE_FLOAT",
                                 "TC_HALF_FLOAT", "TC_LQ", "TC_ENCODED_REFLECTION_CAPTURE") if hasattr(T, n)}


def _set(tex, prop, value):
    if tex.get_editor_property(prop) != value:
        tex.set_editor_property(prop, value)
        return True
    return False


def fix(root=ROOT, dry=False):
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    bc7 = getattr(T, "TC_BC7", T.TC_DEFAULT)
    changed, saved_mb = [], 0.0
    for data in reg.get_assets_by_path(root, recursive=True):
        if str(data.asset_class_path.asset_name) != "Texture2D":
            continue
        folder = str(data.package_path)
        if folder.startswith(SKIP_FOLDERS):
            continue
        tex = unreal.load_asset(str(data.package_name))
        if tex is None:
            continue
        comp = tex.get_editor_property("compression_settings")
        if comp in KEEP or tex.get_editor_property("lod_group") in (unreal.TextureGroup.TEXTUREGROUP_SKYBOX,
                                                                      unreal.TextureGroup.TEXTUREGROUP_UI):
            continue
        w, h = tex.blueprint_get_size_x(), tex.blueprint_get_size_y()
        if max(w, h) < 1024:
            continue
        if dry:
            unreal.log(f"[textures] would fix {data.package_name} ({w} x {h}, {comp})")
            continue
        dirty = False
        dirty |= _set(tex, "compression_settings", bc7)
        # Block compression needs sides divisible by 4 and mips need powers of two: a painting at 8192 x 6474
        # silently fell back to uncompressed, one mip, never streamed. Stretched to the power of two (the
        # mesh's UVs still span the whole image, so it looks the same) it compresses, mips and streams.
        if (w & (w - 1)) or (h & (h - 1)):
            P2 = unreal.TexturePowerOfTwoSetting
            dirty |= _set(tex, "power_of_two_mode", getattr(P2, "STRETCH_TO_POWER_OF_TWO", None) or P2.PAD_TO_POWER_OF_TWO)
        dirty |= _set(tex, "never_stream", False)
        dirty |= _set(tex, "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
        if dirty:
            unreal.EditorAssetLibrary.save_loaded_asset(tex)
            changed.append(str(data.package_name))
            saved_mb += w * h * 4 / 1048576 - w * h / 1048576
    unreal.log(f"[textures] {len(changed)} colour images now BC7, mipped and streamed "
               f"(about {saved_mb:.0f} MB less resident at full size)")
    return changed


if __name__ == "__main__":
    fix(dry="--dry" in sys.argv)
