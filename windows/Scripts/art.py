"""
High-resolution art: prepare the downloaded images, then re-import them over the museum's textures.

1. Prepare (plain Python, outside Unreal):

       python windows/Scripts/art.py prepare

   For every image in assets/paintings_hires/ named <id>.<ext> (an id of assets/art_sources.json):
   - finds the current, correctly framed image (assets/paintings/<id>.*, or its 2048 px original kept
     in _2048) inside the larger scan by feature matching, and crops exactly that: full-resolution
     masters with mounts, margins or colophons come out framed as before. Without a confident match
     it falls back to the proportions: within 4 % it is centre-cropped to them, beyond that it is
     left out and reported, since the canvas would stretch it;
   - caps the long edge at MAX_EDGE (16384 for the panoramas: the handscroll, the Orangerie panels);
   - keeps the current image in assets/paintings/_2048/ and writes the new one in its place
     (JPEG quality 95, or PNG for a PNG source).

2. Clean copies (plain Python): python windows/Scripts/art.py clean   (baseline JPEGs Unreal can decode)

3. Re-import (in the editor, through the one-session runner or on its own):

       UnrealEditor-Cmd.exe MuseeVision.uproject -run=pythonscript -script="<repo>/windows/Scripts/art.py reimport"

   Every texture asset named <id> under /Game/Museum/*/*/Textures is re-imported in place from its
   new file (its materials and canvases keep it), with no size cap below the image's own.
"""
import glob
import json
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
PAINTINGS = os.path.join(REPO, "assets", "paintings")
HIRES = os.path.join(REPO, "assets", "paintings_hires")
BACKUP = os.path.join(PAINTINGS, "_2048")
MAX_EDGE = 8192
PANORAMA_EDGE = 16384
PANORAMAS = ("chinese-thousand-li", "monet-water-lilies-")
ASPECT_TOLERANCE = 0.04


def current_file(work_id):
    hits = [f for f in glob.glob(os.path.join(PAINTINGS, work_id + ".*")) if not f.endswith(".md")]
    return hits[0] if hits else None


def register(current, source):
    """
    Where the current (correctly framed) image lies inside a larger scan of the same work: ORB features
    matched between the two, a similarity transform fitted with RANSAC. Returns the crop box in the
    source's pixels, or None if the match isn't confident (too few inliers, or a scale or rotation
    that doesn't make sense).
    """
    import cv2
    import numpy as np
    work = 1600.0
    cs = work / max(current.size)
    ss = min(1.0, 3200.0 / max(source.size))
    a = cv2.cvtColor(np.asarray(current.convert("RGB").resize((max(1, round(current.width * cs)), max(1, round(current.height * cs))))), cv2.COLOR_RGB2GRAY)
    b = cv2.cvtColor(np.asarray(source.convert("RGB").resize((max(1, round(source.width * ss)), max(1, round(source.height * ss))))), cv2.COLOR_RGB2GRAY)
    orb = cv2.ORB_create(8000)
    ka, da = orb.detectAndCompute(a, None)
    kb, db = orb.detectAndCompute(b, None)
    if da is None or db is None or len(ka) < 30 or len(kb) < 30:
        return None
    matches = cv2.BFMatcher(cv2.NORM_HAMMING).knnMatch(da, db, k=2)
    good = [m for m, n in (p for p in matches if len(p) == 2) if m.distance < 0.8 * n.distance]
    if len(good) < 25:
        return None
    pa = np.float32([ka[m.queryIdx].pt for m in good])
    pb = np.float32([kb[m.trainIdx].pt for m in good])
    M, inl = cv2.estimateAffinePartial2D(pa, pb, method=cv2.RANSAC, ransacReprojThreshold=3.0, maxIters=5000)
    if M is None or inl is None or int(inl.sum()) < 25:
        return None
    scale = float(np.hypot(M[0, 0], M[1, 0]))
    angle = float(np.degrees(np.arctan2(M[1, 0], M[0, 0])))
    if abs(angle) > 1.0 or scale <= 0:
        return None
    corners = np.float32([[0, 0], [a.shape[1], 0], [a.shape[1], a.shape[0]], [0, a.shape[0]]]).reshape(-1, 1, 2)
    box = cv2.transform(corners, M).reshape(-1, 2) / ss
    x0, y0 = box[:, 0].min(), box[:, 1].min()
    x1, y1 = box[:, 0].max(), box[:, 1].max()
    x0, y0 = max(0, int(round(x0))), max(0, int(round(y0)))
    x1, y1 = min(source.width, int(round(x1))), min(source.height, int(round(y1)))
    if (x1 - x0) < 0.5 * current.width or (y1 - y0) < 0.5 * current.height:
        return None     # no real gain, or a wrong match
    return (x0, y0, x1, y1), int(inl.sum())


def open_large(path):
    """
    An image as a Pillow image; a gigapixel JPEG (The Starry Night's master is 44567 x 35291) is decoded
    at a reduced scale inside libjpeg by OpenCV, so it never has to fit in memory at full size.
    """
    from PIL import Image
    Image.MAX_IMAGE_PIXELS = None
    with Image.open(path) as probe:
        w, h = probe.size
    if w * h > 250e6 and path.lower().endswith((".jpg", ".jpeg")):
        import cv2
        flag = cv2.IMREAD_REDUCED_COLOR_4 if max(w, h) / 4 >= MAX_EDGE else cv2.IMREAD_REDUCED_COLOR_2
        arr = cv2.imread(path, flag)
        return Image.fromarray(cv2.cvtColor(arr, cv2.COLOR_BGR2RGB))
    im = Image.open(path)
    return im.convert("RGB") if im.mode not in ("RGB", "L") else im


def prepare(ids=None):
    from PIL import Image
    Image.MAX_IMAGE_PIXELS = None
    os.makedirs(BACKUP, exist_ok=True)
    done, skipped = [], []
    for src in sorted(glob.glob(os.path.join(HIRES, "*.*"))):
        work_id, ext = os.path.splitext(os.path.basename(src))
        if ids and work_id not in ids:
            continue
        cur = current_file(work_id)
        backup = os.path.join(BACKUP, os.path.basename(cur)) if cur else None
        ref = backup if backup and os.path.exists(backup) else cur   # always match the original framing
        if not ref:
            skipped.append((work_id, "no current image to match (a new work: place its canvas first)"))
            continue
        old = Image.open(ref)
        target = old.width / old.height
        im = open_large(src)
        how = "proportions"
        hit = register(old, im)
        if hit:
            box, inliers = hit
            im = im.crop(box)
            how = f"registered ({inliers} matches)"
        aspect = im.width / im.height
        if abs(aspect - target) / target > ASPECT_TOLERANCE:
            skipped.append((work_id, f"proportions {aspect:.3f} vs the canvas's {target:.3f} ({how})"))
            continue
        # Trim to the canvas's exact proportions (centre crop), then cap the long edge.
        if aspect > target:
            w = round(im.height * target)
            im = im.crop(((im.width - w) // 2, 0, (im.width - w) // 2 + w, im.height))
        elif aspect < target:
            h = round(im.width / target)
            im = im.crop((0, (im.height - h) // 2, im.width, (im.height - h) // 2 + h))
        cap = PANORAMA_EDGE if work_id.startswith(PANORAMAS) else MAX_EDGE
        if max(im.size) > cap:
            s = cap / max(im.size)
            im = im.resize((round(im.width * s), round(im.height * s)), Image.LANCZOS)
        if max(im.size) <= max(old.size):
            skipped.append((work_id, f"no gain: {im.size} vs the current {old.size} ({how})"))
            continue
        if not os.path.exists(backup):
            shutil.copy2(cur, backup)
        out = os.path.splitext(cur)[0] + (".png" if ext.lower() == ".png" and im.mode == "RGBA" else ".jpg")
        if out != cur and os.path.exists(cur):
            os.remove(cur)
        if im.mode != "RGB":
            im = im.convert("RGB")
        im.save(out, quality=95, subsampling=0)
        done.append((work_id, im.size, how))
    for work_id, size, how in done:
        print(f"prepared {work_id}: {size[0]} x {size[1]} ({how})")
    for work_id, why in skipped:
        print(f"SKIPPED {work_id}: {why}")
    print(f"{len(done)} prepared, {len(skipped)} skipped; the old images are in {BACKUP}")


CLEAN = os.path.join(REPO, "windows", "Saved", "ArtImport")


def clean(ids=None):
    """
    Clean baseline JPEGs of the images for Unreal's decoder, which rejects several source JPEGs
    (musee_paths.BAD_JPEGS: progressive, or with stray bytes). Plain Python (Pillow): run before the
    re-import, which prefers windows/Saved/ArtImport/<id>.jpg to the original.
    """
    from PIL import Image
    Image.MAX_IMAGE_PIXELS = None
    os.makedirs(CLEAN, exist_ok=True)
    if not ids:
        ids = {os.path.splitext(os.path.basename(f))[0] for f in glob.glob(os.path.join(BACKUP, "*"))}
    for work_id in sorted(ids):
        src = current_file(work_id)
        if not src:
            continue
        with Image.open(src) as im:
            im.convert("RGB").save(os.path.join(CLEAN, work_id + ".jpg"), quality=95, subsampling=0,
                                   progressive=False, optimize=False)
        print(f"clean copy: {work_id}")


def reimport(ids=None):
    """
    Re-import textures in place from assets/paintings. Without ids: the images art.py prepare replaced
    (those with an original kept in _2048); with ids: exactly those (e.g. to repair one).
    """
    import unreal
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    files = {os.path.splitext(os.path.basename(f))[0]: f for f in glob.glob(os.path.join(PAINTINGS, "*"))
             if not f.endswith(".md") and os.path.isfile(f)}
    if not ids:
        ids = {os.path.splitext(os.path.basename(f))[0] for f in glob.glob(os.path.join(BACKUP, "*"))}
    tasks, names = [], []
    for data in reg.get_assets_by_path("/Game/Museum", recursive=True):
        if str(data.asset_class_path.asset_name) != "Texture2D":
            continue
        name = str(data.asset_name)
        folder = str(data.package_path)
        if not folder.endswith("/Textures") or name not in files or name not in ids:
            continue
        task = unreal.AssetImportTask()
        cleaned = os.path.join(CLEAN, name + ".jpg")
        task.filename = cleaned if os.path.exists(cleaned) else files[name]
        task.destination_path = folder
        task.destination_name = name
        task.automated = True
        task.replace_existing = True
        task.replace_existing_settings = False
        task.save = True
        tasks.append(task)
        names.append(f"{folder}/{name}")
    tools.import_asset_tasks(tasks)
    for path in names:
        tex = unreal.load_asset(path)
        if tex:
            tex.set_editor_property("max_texture_size", 0)
            tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
            unreal.EditorAssetLibrary.save_loaded_asset(tex)
            unreal.log(f"[art] {path}: {tex.blueprint_get_size_x()} x {tex.blueprint_get_size_y()}")
    unreal.log(f"[art] {len(names)} textures re-imported")


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    if args and args[0] == "prepare":
        prepare(set(args[1:]) or None)
    elif args and args[0] == "clean":
        clean(set(args[1:]) or None)
    elif args and args[0] == "reimport":
        reimport(set(args[1:]) or None)
    else:
        print(__doc__)
