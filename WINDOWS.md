# Musée Vision · Windows (RTX 4090)

A brief for building the desktop version of Musée Vision on a Windows PC with an RTX 4090. It
starts from this repository alone: the plan, the collection, the images and scans, and the iPhone build
as a working reference.

## The goal

The same museum as the iPhone build, every wing, every moving part, the same plan. Here it is rendered
at the quality a 4090 allows, not the quality a phone allows: real global illumination through the
Rotunda's eye and the Hall of Light's glass, ray-traced reflections in the pond and the polished stone,
the sculpture scans at full resolution, and 4K at a steady frame rate. The iPhone app is a sketch of the
building; this should feel like standing in it.

## Engine

**Unreal Engine 5.8** (decided). It has Lumen with hardware ray tracing for the daylight, MegaLights
for many soft-shadowed lights, Nanite for multi-million-triangle scans and coffers, the path tracer for
stills, DLSS for 4K, and OpenXR if the museum later goes into a PC headset.

## Pipeline and order of work

**Now: the 4090 desktop build only.** The iPhone isn't a target for now. Whether Unreal later becomes
the main build for every platform (iPhone and Vision Pro included) is undecided; until then, keep the
architecture coming from the Mac.

The building is defined once. The Swift builder (on the Mac) exports it as USD, one layer per wing with
stable names and a material slot on every surface. The export is in [`usd/`](usd/README.md). When the
design changes: canvas → `plan/` → the Swift wing → re-export on the Mac (`tools/usd-export/export.sh`)
→ reimport in Unreal.

**Import `usd/` as it is; don't rebuild the architecture by hand.** Its geometry was checked against
the plan wing by wing. Import it with Interchange as ordinary Unreal assets (static meshes, materials,
levels), not as a live USD Stage actor, with Nanite on. Keep the prim and material names: they trace
back to the plan and let a reimport keep what was assigned in Unreal.

**Then rebuild natively only where the export is weak or missing:**

| Part | Why | Do instead |
|---|---|---|
| Coffers, cornices, mouldings | Flat textures in the export | Model one coffer or moulding piece and instance it; Nanite carries the detail |
| Materials | Basic colour and roughness only | Real Unreal materials matched by name (travertine, Carrara, bronze, gilt, brick…) |
| Lights | Approximate conversion from the iPhone's values | Relight with Lumen and MegaLights, using the exported positions as a guide |
| Sculptures | Decimated to ≤150k triangles | Full-resolution originals from `assets/sculptures/CREDITS.md`, as Nanite meshes |
| Gardens and plants | Simple stand-ins | Unreal's foliage and procedural tools, with wind |
| Collision | Not in the export | Generate it in Unreal |
| Behaviour | USD carries none | Build it in Unreal; the moving parts are separate prims tagged `museevision:movingPart` |

**Order:**

1. **Set up (≈ 1 week):** Unreal 5.8, Visual Studio 2022, Git LFS; the project in `windows/` with
   hardware ray tracing, Lumen, MegaLights, Nanite and DLSS on; walking and collision.
2. **The Rotunda first:** import only `usd/wings/Rotunda`, then do its materials, lighting and coffers.
   Render it from the camera of `plan/renderings/05-the-rotunda-v2.png` and compare side by side, in
   real time and path-traced. Get this room right before importing the rest.
3. **The other wings:** the same way, starting with the Salon (look-match against
   `plan/renderings/01-salon-arrival.png`).
4. **Build-out (3–5 weeks):** sky, elevator, pond and Reserve, handscroll, cup, stereo stones,
   placards, the Sphere.
5. **Alive and ship (1–2 weeks):** gardens, wind and rain, sound, performance at 4K, path-traced photo
   mode, packaged app.

Summary with the renderings: https://claude.ai/artifact/RyC1hYn7S2w59TtozVvTCY

Put the project in `windows/` in this repository. Keep build output out of git: `Binaries/`,
`Intermediate/`, `Saved/`, `DerivedDataCache/`, `.vs/`. Unreal's `.uasset` and `.umap` files are binary
and large, so set up **Git LFS** for them before the first commit (GitHub rejects files over 100 MB).

## Where the design comes from

Read these in this order:

1. **`plan/boards/*.png`**: every board of the canvas as an image (current). The older
   `plan/Musee-Vision-Guide.pdf` walks the museum in visit order but dates from 23 September.
2. **`plan/README.md`**: the plan in words, an index of the boards, and the proposals in
   `plan/proposals/` (Salon interior update, Chenghuai, Albion) with what is decided and what is not.
3. **`plan/canvas/*.dc.html`**: the boards themselves. Every plan, section and elevation is inline SVG,
   so exact positions and dimensions can be read from the markup.
4. **`README.md`, "Where the plan was read, resolved or bent"**: every place the boards disagree or
   can't be built as drawn, and the reading chosen. Use the same readings so the two builds match.
5. **`Shared/` (Swift)**: the iPhone build already turned the boards into numbers. The building itself
   reaches Unreal through `usd/`; for everything else, treat it as an exact spec to port, not as code
   to reuse:
   - `Shared/Plan/`: the Salon plan and hang, and the Hall of Light planting.
   - `Shared/Wings/<Wing>.swift`: one file per wing, with every wall, opening, vault, plinth and position.
   - `Shared/Core/`: the sky (sun and moon ephemerides, the 24 solar terms, the star catalogue),
     multi-level floors and collision, and the gardens.
   - `Shared/MuseumScene.swift`: the moving parts (the pond lift, the racks, the easel, the elevator,
     the handscroll, the chicken cup, the stereographs).

The design is still edited on the live canvas (https://claude.ai/artifact/5KS3Zkfoub5zBdopET1Qc6).
`plan/` is a snapshot of it. If a board has changed since, the canvas wins.

## Coordinates

The plan uses metres, with the Rotunda's centre at the origin, x east, plan y south and height up. That
is left-handed, like Unreal, so the mapping has no flips:

```
Unreal X = plan x × 100    (east)
Unreal Y = plan y × 100    (south)
Unreal Z = height × 100    (up)
```

Swift uses RealityKit's y-up frame, so plan y is RealityKit **z** there, and height is RealityKit y.
The USD in `usd/` keeps that frame (metres, `upAxis = "Y"`); Unreal's USD importer turns it into the
mapping above.

## Assets in the repo

| Path | What | Notes |
|---|---|---|
| `usd/` | The whole museum as USD: `museum.usda`, one layer per wing, the scans, materials, drawn textures | Import `museum.usda`. Exported from the Swift builder; every file is under 50 MB, so it needs no LFS. See `usd/README.md`. |
| `data/artworks.json`, `data/sculptures.json` | Catalogue and placard text for the Salon paintings and the sculptures | Same files the iPhone app bundles. |
| `assets/collection.json` | Placards for the Chinese Wing, the Hall of Light and *The Starry Night* | |
| `assets/paintings/` | Every painting, photograph and scroll as JPEG | Sources and licences in `CREDITS.md`. Where a museum offers a larger open-access file, fetch it: a 4090 can hold them. |
| `assets/sculptures/*.mvm` | The 12 scans, decimated to ≤150k triangles | Better: download the **full-resolution originals** from the sources in `CREDITS.md` and import them with Nanite. The `.mvm` files are the fallback. |
| `assets/sky/` | Yale Bright Star Catalogue (9,096 stars) | |
| `assets/logo/` | Logo, lockups and colours | |
| `assets/reference/` | Glaze-colour references for the Chinese ceramics | Reference only. |

**`.mvm` format** (little-endian): `u32` magic `0x314D564D` ("MVM1"), `u32` vertex count *V*, `u32`
triangle count *T*, then *V* × `float3` positions, *V* × `float3` normals, then *T* × 3 `u32` indices.
Metres, y-up (RealityKit). The meshes are open in places, so render them two-sided. There are no UVs;
the iPhone build gives them bronze, marble or stone materials by work. In `usd/sculptures/` each scan
prim carries its work id and scan source, so the full-resolution original can be swapped in for it.

## What to build (parity with the iPhone app)

- **All wings:** Rotunda with the sun clock; Salon Impression with the Manet cabinet and the Nymphéas
  oval; the lifting pond, the stair and the Reserve with its racks and easel; Sculpture Hall; the Chinese
  Wing; the Hall of Light with its gardens; Élan (the Atrium, the Square and the Sphere) with the glass
  elevator.
- **Live sky:** the real sun, moon and stars for the viewer's location and time. The sun clock keeps
  local time. The Chinese garden follows the solar term. The moon lies in the pond at night. On a PC,
  let the user pick a city or type a latitude and longitude, and default to the time zone.
- **Interactions:** the Vision Pro "gaze and pinch" becomes look and click (or **E**). The golden lily
  takes two steps (wake, then lift). One rack is out at a time. A painting can be sent to the easel. The
  chicken cup can be picked up and turned. The elevator is called from the bronze ring and runs in real
  time at 1.5 m/s with no skip, and the first ride goes down to the Square before going up. The
  handscroll opens an arm's length at a time as you walk north beside it.
- **Placards:** a glass placard with title, artist, date and collection when you look at a work.
- **Movement:** WASD and the mouse, or a gamepad. Walking pace, solid walls, stairs and multi-level
  floors. The elevator and the lifting pond carry you.
- **Stereographs:** on a monitor, keep the iPhone's gentle rocking between the two views. In a headset,
  show them in true stereo.
- **The Sphere** is never seen from outside. Once the car passes the south pole at 26 m, hide the
  building.

## Where a 4090 can go further

Only in ways the plan already implies:

- Daylight through the eye, the lattices and the glass hall, moving with the real sun. Rain and wind in
  the Hall of Light's gardens ("Modern · Alive").
- Real reflections and refraction in the pond, the glass steles, the glass car and the misty dome.
- Stone, bronze and gilt that read as stone, bronze and gilt: travertine, Carrara, patinated bronze.
  The palette is on the `Main` board.
- A path-traced screenshot or photo mode.

## Done means

- A clean clone plus the README steps builds and runs on the Windows PC.
- Every room in the guide can be reached on foot or by elevator, in the order of the visit.
- Positions and sizes match the boards and the iPhone build's readings.
- A steady frame rate at 4K with DLSS on the 4090.
