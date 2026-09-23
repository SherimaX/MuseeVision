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

The building is defined once. The Swift builder (on the Mac) exports it as USD, one layer per wing with
stable names and a material slot on every surface, and Unreal imports that. Only the behaviour is
rewritten for Unreal. When the design changes: canvas → `plan/` → the Swift wing → re-export USD →
reimport in Unreal (stable names keep the materials and lighting).

1. **Start now, PC (≈ 1 week):** set up Unreal 5.8, Visual Studio 2022 and Git LFS; material library
   from the Concept palette; the 12 sculptures from their full-resolution originals as Nanite meshes;
   walking and collision.
2. **In parallel, Mac (1–2 weeks):** the USD exporter; coffers, cornices, fluting, mouldings and brick
   courses modelled as geometry.
3. **Look-match, PC (1–2 weeks):** import the USD, light it, then render the Rotunda and the Salon
   arrival from the cameras of `output/imagegen` renderings `05-the-rotunda-v2` and `01-salon-arrival`
   (on the Mac; ask the user for them) and compare side by side. Get the stone rooms right before
   building out.
4. **Build-out, PC (3–5 weeks):** sky, elevator, pond and Reserve, handscroll, cup, stereo stones,
   placards, the Sphere.
5. **Alive and ship, PC (1–2 weeks):** gardens, wind and rain, sound, performance, path-traced photo
   mode, packaged app.

Until the USD exporter lands, don't hand-build the architecture in Unreal; it would be thrown away.
Summary with the renderings: https://claude.ai/artifact/RyC1hYn7S2w59TtozVvTCY

Put the project in `windows/` in this repository. Keep build output out of git: `Binaries/`,
`Intermediate/`, `Saved/`, `DerivedDataCache/`, `.vs/`. Unreal's `.uasset` and `.umap` files are binary
and large, so set up **Git LFS** for them before the first commit (GitHub rejects files over 100 MB).

## Where the design comes from

Read these in this order:

1. **`plan/Musee-Vision-Guide.pdf`**: the whole museum, room by room, in visit order.
2. **`plan/README.md`**: the plan in words, and an index of the boards.
3. **`plan/canvas/*.dc.html`**: the boards themselves. Every plan, section and elevation is inline SVG,
   so exact positions and dimensions can be read from the markup.
4. **`README.md`, "Where the plan was read, resolved or bent"**: every place the boards disagree or
   can't be built as drawn, and the reading chosen. Use the same readings so the two builds match.
5. **`Shared/` (Swift)**: the iPhone build already turned the boards into numbers. Treat it as an exact
   spec to port, not as code to reuse:
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

## Assets in the repo

| Path | What | Notes |
|---|---|---|
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
the iPhone build gives them bronze, marble or stone materials by work.

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
