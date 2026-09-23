# Musée Vision · USD

The whole museum as USD, exported from the Swift builder. The architecture is still defined only in
`Shared/`; this folder is its output, for Unreal on the Windows PC (see [`../WINDOWS.md`](../WINDOWS.md))
and later for baking light in Blender. Don't edit these files by hand: change the Swift, then re-export.

## Re-export

On the Mac, from the repository root:

```bash
tools/usd-export/export.sh
```

It compiles `Shared/` and `tools/usd-export/` into a macOS command-line tool (`museum-usd`), builds the
museum exactly as the app does, writes this folder (this README stays), and runs `usdchecker` on
`museum.usda`. It needs Xcode-beta (the macOS SDK with RealityKit; set `DEVELOPER_DIR` for another
Xcode) and `usdcat` / `usdchecker`, which ship with macOS in `/usr/bin`. It takes about a minute.

The export is deterministic: an unchanged Swift build gives byte-identical files. What follows the
clock in the app (the gardens' season, the Chinese Wing's solar term, the Rotunda's sun) is built for a
fixed moment, 21 June 2026 at noon UTC, latitude 40° N. Another moment: `export.sh --date 2026-12-21T12:00:00Z`.

## Frame

RealityKit's frame, unchanged: **metres** (`metersPerUnit = 1`), **Y up** (`upAxis = "Y"`), the centre of
the Rotunda at the origin, **+X east**, **+Z south** (the plan's y), right-handed. Unreal's USD importer
converts Y-up to Z-up by swapping Y and Z and scales to centimetres, which should give the mapping in
`WINDOWS.md`: Unreal X = x × 100 (east), Y = plan y × 100 (south), Z = height × 100.

## Files

| File | What |
|---|---|
| `museum.usda` | The root layer. Open or import this. It defines `/Museum` and sublayers everything below except `extras/`. |
| `wings/<Wing>.usdc` | One layer per wing: `Rotunda`, `Salon` (with the Manet cabinet and the Nymphéas oval), `Reserve` (the lifting pond, stair, cellar, racks and easel), `SculptureHall`, `ChineseWing`, `HallOfLight` (with its gardens), `Elan` (the Atrium, the Square and the elevator). |
| `sculptures/Sculptures.usda` | The 12 scans in place, each referencing its geometry in `sculptures/<id>.usdc` (the decimated `.mvm` files; about 840k triangles in all). |
| `Materials.usda` | Every material, under `/Museum/Materials`. |
| `textures/` | The textures the app draws in code (stone, brick, lettering, the sun clock…), as PNG. |
| `extras/Helpers.usdc` | Not sublayered. The phone's runtime-only helpers: contact shading along walls, the sun clock's sun patch and point of shadow. |
| `extras/Sky.usdc` | Not sublayered. The phone's sky dome (900 m) and star field, which follow the camera there. |
| `summary.json` | Per wing: triangles, meshes, prims, materials, lights and bounds; the material and texture lists. |

To see an extra, add it to `museum.usda`'s `subLayers` (or add it as a sublayer in Unreal's USD Stage
editor).

## Prims

Every wing is `/Museum/<Wing>`, and below it the entity tree of the Swift build, one prim per entity,
named from the entity's name (`Drum`, `Lily pond`, `monet-impression-sunrise`) made into a USD
identifier: accents folded (`Élan elevator` → `Elan_elevator`), spaces and dashes to `_`, other
characters spelled as `u` + hex. Unnamed entities are named by their mesh (`rack_frame`, `car_glass`),
their role (`Canvas` for a painting's image, `SpotLight`) or `Group`. Siblings with the same name are
numbered in build order: `Trunk`, `Trunk_2`, … An entity with one mesh and no children is a `Mesh`;
otherwise an `Xform`, with its geometry in `Geom` children.

So paths depend only on the Swift build: re-exporting an unchanged build gives the same paths, so
Unreal can keep what was assigned to them. Renaming an entity, or adding a same-named sibling before
others, changes paths.

Each prim carries `museevision:entity` (the Swift name). Disabled entities (the pond's rim lights, the
car's glow, the mast) are exported where they are at the start, with `visibility = invisible` and
`museevision:enabledAtStart = 0`.

**Moving parts** are separate prims, tagged `museevision:movingPart`, so Unreal can animate them:

| Tag | Prims |
|---|---|
| `pond` | `/Museum/Reserve/Lily_pond` (basin, water, lilies, the golden lily; rises 2.6 m) |
| `pond_post`, `pond_rim_light`, `pond_halo` | `Pond_post_1…4` (scaled to 0.001 in y at rest), `Rim_light_01…32`, `Halo` |
| `reserve_rack`, `reserve_rack_work` | `The_Reserve/Rack_A…F`, `Rack_N…`, `Rack_S…` (56), and the works hung on racks A–E |
| `elevator`, `elevator_car`, `elevator_car_door`, `elevator_car_glow`, `elevator_mast` | `/Museum/Elan/Elan_elevator/Car/…`, `Mast` |
| `elevator_landing_door`, `elevator_iris` | the landing doors at the Atrium and the Square; the iris at the south pole |
| `chicken_cup` | `/Museum/ChineseWing/chinese_chicken_cup` |
| `handscroll_veil`, `handscroll_roller` | the Thousand Li's veils and rollers |
| `stereograph_view` | the two alternating views on each stereo stone |
| `tree_crown` | the crowns that sway |
| `sun`, `easel_light` | the Rotunda's sun (at the export moment) and the easel's spot |

## Geometry

Triangle meshes with `points`, vertex `normals`, `faceVertexIndices` / `faceVertexCounts` (all 3),
`extent`, `subdivisionScheme = "none"`, and UV0 as `primvars:st`. Vertices are welded where position,
normal and UV agree. Glass and the scans are `doubleSided`.

**UV0 is in metres** wherever a material is a surface: the builder's own UVs, rescaled to metres where
they were in tiles or fractions, and a box projection in metres (by each triangle's facing) where the
builder left them at zero (boxes, reveals, the scans). Only image-mapped surfaces keep their 0–1 UVs:
paintings, photographs and scrolls, lettering, the sun clock, the ceramics' glazes.

## Materials

One `UsdPreviewSurface` material per distinct material, under `/Museum/Materials`, with base colour,
roughness, metallic, opacity (and clear coat on the polished floors). Names come from content, not
order, so they stay the same across re-exports:

- paintings and photographs: `painting_<image id>`, reading `../assets/paintings/<id>.jpg`;
- drawn textures: the texture's name (`travertine_honed`, `travertine_polished`, `marble_polished`,
  `brick`, `rammed_earth`, `stone_slab_<colour>`, `letter_plate_A`…), with the PNG in `textures/` and a
  `UsdTransform2d` that scales metres back to the app's tiling;
- plain colours: a name from `tools/usd-export/Palette.swift` (`gilt`, `bronze`, `moulding`, `stone`,
  `glass_case`, `wall_bay_1`…), or `<family>_<hex>` for a colour not listed there;
- variants that would share a name get a suffix from what differs: the tint (`travertine_honed_FFFBF3`),
  roughness (`gilt_r35`), opacity (`glass_landing_o20`).

The app's self-lit surfaces (skylights, lettering, glows) are marked `museevision:unlit` and also drive
`emissiveColor`. Normal maps are not exported. Unreal will assign its own materials; the names are what
matter.

## Lights

The app's spot lights are `SphereLight`s (radius 5 cm) with `ShapingAPI`. RealityKit's cone angles are
the full cone (measured), so `shaping:cone:angle` is half the app's outer angle, and the softness is
(outer − inner) / outer. Intensity is converted from the app's lumens to the sphere's luminance,
lumens / (4π² r²); the lumens, angles and attenuation radius are kept as `museevision:` attributes.
The sun is a `DistantLight` of 9,000 lux, placed for the export moment.

## Sculptures

Each scan prim (`/Museum/SculptureHall/rodin_thinker`, the Little Dancer in the Salon, the Tang horse in
the Chinese Wing) carries its work id, the `.mvm` file, the work, the scan's source and source page,
licence and triangle counts, both as `customData.museevision` and as `museevision:` attributes, taken
from `assets/sculptures/CREDITS.md`. To use the full-resolution originals, swap the referenced
geometry and keep the prim and its transform.

## Not included

Collision and walkable floors, pick targets and placards, sound, and all behaviour (the pond, the
racks, the elevator, the handscroll, the stereographs, the sky): rebuild those in Unreal from
`Shared/MuseumScene.swift` and the wing files. The garden follows the export moment, not today.

## Importing into Unreal 5.8

Either **File → Import Into Level** with `museum.usda` (Interchange), which makes static meshes,
material instances and textures as assets, or a **USD Stage** actor opened on `museum.usda`, which
keeps the layers live (each wing can be muted, and a re-export shows up on reload). In both, the
prim paths and material names stay the same across re-exports, so a reimport should update the
geometry and keep the materials assigned in Unreal. The `museevision:` values are ordinary USD
attributes and `customData`, readable from Unreal's USD Python API or its metadata import options.
(Not yet tried in Unreal: this was checked with `usdchecker` and `usdrecord` on the Mac.)
