"""
The Classical Hall's sculptures: casts from the Royal Cast Collection (SMK, Copenhagen; public domain),
listed in assets/classical/SOURCES.json and downloaded as STL into assets/classical/.

1. Convert (plain Python, outside Unreal):

       python windows/Scripts/classical_art.py convert

   Each binary STL is welded into an indexed mesh with smooth normals and written as a glTF binary
   (assets/classical/<id>.glb), scaled to the real work's height or length (SOURCES.json: SMK's print
   files are normalised to about 130 units) and turned from STL's z-up to glTF's y-up. Prints each
   figure's size, to check against the real works.

2. Import and place (in the editor; run through apply_all.py's "classical" step):

   imports each .glb as a Nanite static mesh in /Game/Museum/Classical/, gives it M_Plaster_Cast (a warm
   white cast plaster), and stands it at a statue spot of the Classical Hall (AClassicalHallStructure::
   GetStatueSpots), tagged work:classical-<id> so its placard (data/classical.json) comes up.
"""
import json
import math
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
FOLDER = os.path.join(REPO, "assets", "classical")
SOURCES = os.path.join(FOLDER, "SOURCES.json")
CONTENT = "/Game/Museum/Classical"
TAG = "musee.classical"


def items():
    with open(os.path.join(FOLDER, "SOURCES.json"), encoding="utf-8") as f:
        return json.load(f)


def read_stl(path):
    import numpy as np
    with open(path, "rb") as f:
        data = f.read()
    n = struct.unpack_from("<I", data, 80)[0]
    if 84 + n * 50 != len(data):
        raise ValueError(f"{path}: not a binary STL ({n} triangles, {len(data)} bytes)")
    rec = np.frombuffer(data, dtype=np.dtype([("n", "<f4", 3), ("v", "<f4", (3, 3)), ("a", "<u2")]), count=n, offset=84)
    return rec["v"].reshape(-1, 3).astype(np.float64)


def weld(tri_verts):
    import numpy as np
    span = np.ptp(tri_verts, axis=0).max()
    key = np.round(tri_verts / (span * 1e-6)).astype(np.int64)
    uniq, inverse = np.unique(key, axis=0, return_inverse=True)
    pos = np.zeros((len(uniq), 3))
    np.add.at(pos, inverse, tri_verts)
    pos /= np.bincount(inverse, minlength=len(uniq))[:, None]
    faces = inverse.reshape(-1, 3)
    faces = faces[(faces[:, 0] != faces[:, 1]) & (faces[:, 1] != faces[:, 2]) & (faces[:, 0] != faces[:, 2])]
    return pos, faces


def normals(pos, faces):
    import numpy as np
    fn = np.cross(pos[faces[:, 1]] - pos[faces[:, 0]], pos[faces[:, 2]] - pos[faces[:, 0]])   # area-weighted
    vn = np.zeros_like(pos)
    for k in range(3):
        np.add.at(vn, faces[:, k], fn)
    length = np.linalg.norm(vn, axis=1, keepdims=True)
    return vn / np.where(length > 0, length, 1.0)


def write_glb(path, pos, nrm, faces):
    import numpy as np
    p = pos.astype(np.float32)
    n = nrm.astype(np.float32)
    i = faces.astype(np.uint32).ravel()
    blobs = [p.tobytes(), n.tobytes(), i.tobytes()]
    offsets, total = [], 0
    for b in blobs:
        offsets.append(total)
        total += (len(b) + 3) // 4 * 4
    buf = b"".join(b + b"\0" * ((4 - len(b) % 4) % 4) for b in blobs)
    gltf = {
        "asset": {"version": "2.0", "generator": "Musee Vision classical_art.py"},
        "scene": 0, "scenes": [{"nodes": [0]}], "nodes": [{"mesh": 0, "name": os.path.splitext(os.path.basename(path))[0]}],
        "meshes": [{"primitives": [{"attributes": {"POSITION": 0, "NORMAL": 1}, "indices": 2}]}],
        "buffers": [{"byteLength": len(buf)}],
        "bufferViews": [
            {"buffer": 0, "byteOffset": offsets[0], "byteLength": len(blobs[0]), "target": 34962},
            {"buffer": 0, "byteOffset": offsets[1], "byteLength": len(blobs[1]), "target": 34962},
            {"buffer": 0, "byteOffset": offsets[2], "byteLength": len(blobs[2]), "target": 34963},
        ],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": len(p), "type": "VEC3",
             "min": p.min(0).tolist(), "max": p.max(0).tolist()},
            {"bufferView": 1, "componentType": 5126, "count": len(n), "type": "VEC3"},
            {"bufferView": 2, "componentType": 5125, "count": len(i), "type": "SCALAR"},
        ],
    }
    js = json.dumps(gltf, separators=(",", ":")).encode()
    js += b" " * ((4 - len(js) % 4) % 4)
    with open(path, "wb") as f:
        f.write(struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(js) + 8 + len(buf)))
        f.write(struct.pack("<II", len(js), 0x4E4F534A) + js)
        f.write(struct.pack("<II", len(buf), 0x004E4942) + buf)


def convert():
    import numpy as np
    for it in items():
        stl = os.path.join(FOLDER, it["id"] + ".stl")
        glb = os.path.join(FOLDER, it["id"] + ".glb")
        if not os.path.exists(stl):
            print(f"{it['id']}: not downloaded")
            continue
        if os.path.exists(glb) and os.path.getmtime(glb) > os.path.getmtime(stl) and os.path.getmtime(glb) > os.path.getmtime(SOURCES):
            continue   # up to date
        tv = read_stl(stl)
        pos, faces = weld(tv)
        # SMK normalises its print files (the largest side about 130 units): scale each cast to the real
        # work's documented height (or length, for a reclining figure) from SOURCES.json.
        # STL z-up → glTF y-up (x, z, −y). The casts face +z; a scan saved lying down or turned carries a "turn"
        # (a rotation matrix in y-up space, SOURCES.json) that stands it up and faces it +z.
        yup = np.stack([pos[:, 0], pos[:, 2], -pos[:, 1]], axis=1)
        if "turn" in it:
            yup = yup @ np.array(it["turn"], dtype=float).T
        extent = np.ptp(yup, axis=0)
        if "height" in it:
            yup = yup * (it["height"] / extent[1])
        elif "length" in it:
            yup = yup * (it["length"] / max(extent[0], extent[2]))
        elif extent.max() > 20:
            yup = yup / 1000.0
        # Feet on y = 0, centred on the vertical axis.
        yup[:, 1] -= yup[:, 1].min()
        yup[:, 0] -= (yup[:, 0].max() + yup[:, 0].min()) / 2
        yup[:, 2] -= (yup[:, 2].max() + yup[:, 2].min()) / 2
        write_glb(glb, yup, normals(yup, faces), faces)
        e = np.ptp(yup, axis=0)
        print(f"{it['id']}: {len(faces):,} triangles, {e[0]:.2f} wide x {e[1]:.2f} tall x {e[2]:.2f} deep (m)")


# Which cast stands where (AClassicalHallStructure::GetStatueSpots names), and a yaw correction (degrees)
# for a cast that should turn from its niche's axis.
PLACEMENT = {
    "Tribune.Centrepiece": "belvedere-torso",
    "Tribune.Niche.N": "apollo-belvedere",
    "Tribune.Niche.NE": "laocoon",
    "Tribune.Niche.NW": "venus-de-milo",
    "Tribune.Niche.SE": "barberini-faun",
    "Tribune.Plinth.W": "artemision-bronze",
    "Tribune.Plinth.E": "discobolus",
    "Gallery.Plinth.2": "sleeping-ariadne",
    "Gallery.Plinth.3": "dying-gaul",
    "Gallery.Niche.E1": "augustus-prima-porta",
    "Gallery.Niche.W1": "doryphoros",
    "Gallery.Niche.E2": "capitoline-venus",
    "Gallery.Herm.W": "homer",
}
YAW = {}

# The rest fill the free places by kind, in this order. The tallest new figures take the tribune's niches.
# Along the gallery the west wall is Greece (poets, thinkers, a general) and the east wall is Rome (Caesar to
# the Severans); the tribune ring keeps gods, heroes and kings. The door's second herm pairs Homer with Socrates.
STATUES = ["belvedere-antinous", "minerva-giustiniani", "sophocles", "lansdowne-heracles",
           "capitoline-antinous", "farnese-diadoumenos"]
BUSTS = {
    "Gallery.Herm.E": ["socrates"],
    "Gallery.Bust.W": ["plato", "euripides", "homer-epimenides", "kimon", "pseudo-seneca", "chrysippus",
                       "zeno-of-citium", "sciarra-amazon"],
    "Gallery.Bust.E": ["julius-caesar", "julia-titi", "young-marcus-aurelius", "antoninus-pius",
                       "septimius-severus", "severan-lady", "philip-the-arab", "oxford-bust"],
    "Tribune.Bust.": ["athena-parthenos", "apollo-giustiniani", "alexander-helios", "ptolemy-ii",
                      "head-of-a-gaul"],
}


def assignment(spots):
    """PLACEMENT, plus every other cast in a free place of its kind (statues in niches, busts on consoles)."""
    out = dict(PLACEMENT)
    free = lambda pred: sorted(n for n in spots if n not in out and pred(n))
    niches = free(lambda n: n.startswith("Tribune.Niche.")) + free(lambda n: n.startswith("Gallery.Niche."))
    for spot, work in zip(niches, STATUES):
        out[spot] = work
    spill = []
    for prefix, works in BUSTS.items():
        places = free(lambda n: n.startswith(prefix))
        out.update(zip(places, works))
        spill += works[len(places):]
    out.update(zip(free(lambda n: ".Bust." in n or ".Herm." in n), spill))
    return out


def import_meshes():
    import unreal
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    tasks = []
    for it in items():
        glb = os.path.join(FOLDER, it["id"] + ".glb")
        if not os.path.exists(glb):
            continue
        t = unreal.AssetImportTask()
        t.filename = glb
        t.destination_path = f"{CONTENT}/{it['id']}"
        t.automated = True
        t.replace_existing = True
        t.save = True
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    meshes = {}
    material = unreal.load_asset("/Game/Museum/Materials/M_Plaster_Cast")
    for it in items():
        found = [a for a in unreal.EditorAssetLibrary.list_assets(f"{CONTENT}/{it['id']}", recursive=True)
                 if unreal.EditorAssetLibrary.find_asset_data(a).asset_class_path.asset_name == "StaticMesh"]
        if not found:
            unreal.log_warning(f"[classical] {it['id']}: no static mesh imported")
            continue
        mesh = unreal.load_asset(found[0])
        nanite = mesh.get_editor_property("nanite_settings")
        nanite.set_editor_property("enabled", True)
        mesh.set_editor_property("nanite_settings", nanite)
        if material:
            for i in range(len(mesh.get_editor_property("static_materials"))):
                mesh.set_material(i, material)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
        meshes[it["id"]] = mesh
    return meshes


def place():
    import unreal
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.load_level("/Game/Maps/Museum")
    meshes = import_meshes()
    cls = unreal.load_class(None, "/Script/MuseeVision.ClassicalHallStructure")
    hall = next((a for a in eas.get_all_level_actors() if cls and a.get_class() == cls), None)
    if not hall:
        unreal.log_warning("[classical] no Classical Hall in the map: place it first (native.py)")
        return
    for a in eas.get_all_level_actors():
        if TAG in [str(t) for t in a.tags]:
            eas.destroy_actor(a)
    spots = {str(s.get_editor_property("name")): s for s in hall.get_statue_spots()}
    spots.setdefault("Tribune.Centrepiece", hall.get_centrepiece_spot())
    placed = 0
    for spot_name, work in assignment(spots).items():
        spot, mesh = spots.get(spot_name), meshes.get(work)
        if spot is None or mesh is None:
            unreal.log_warning(f"[classical] {spot_name} / {work}: {'no spot' if spot is None else 'no mesh'}")
            continue
        xf = spot.get_editor_property("transform")
        rot = xf.rotation.rotator()
        rot = unreal.Rotator(0.0, 0.0, rot.yaw - 90.0 + YAW.get(work, 0.0))   # the cast's front (glTF +z) to the spot's X (checked in game)
        a = eas.spawn_actor_from_class(unreal.StaticMeshActor, xf.translation, rot)
        a.static_mesh_component.set_static_mesh(mesh)
        a.set_actor_label(f"Cast {work}")
        a.tags = [unreal.Name(TAG), unreal.Name("musee.building"), unreal.Name("musee.wing:ClassicalHall"),
                  unreal.Name(f"work:classical-{work}")]
        placed += 1
    # The Rotunda's four niches (MuseePlan::Rotunda: on the diagonals, 2 m wide, the sill 2.0 m up, half-round in
    # plan behind the drum's inner face at r 10 m): a statue on each sill, 25 cm back from the face, facing the centre.
    for it in items():
        if it.get("kind") != "rotunda" or it["id"] not in meshes:
            continue
        a = math.radians(it["niche"])
        x, y = 10.25 * math.cos(a), 10.25 * math.sin(a)
        facing = math.degrees(math.atan2(-y, -x))
        rot = unreal.Rotator(0.0, 0.0, facing - 90.0 + YAW.get(it["id"], 0.0))
        c = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x * 100.0, y * 100.0, 200.0), rot)
        c.static_mesh_component.set_static_mesh(meshes[it["id"]])
        c.set_actor_label(f"Cast {it['id']}")
        c.tags = [unreal.Name(TAG), unreal.Name("musee.building"), unreal.Name("musee.wing:Rotunda"),
                  unreal.Name(f"work:classical-{it['id']}")]
        placed += 1
    les.save_current_level()
    unreal.log(f"[classical] {placed} casts placed")


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    if args and args[0] == "place":
        place()
    elif args and args[0] == "convert":
        convert()
    else:
        print(__doc__)
