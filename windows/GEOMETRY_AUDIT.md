# Geometry audit: Swift source → USD → Unreal

> **Update, 23 Sep 2026:** Unreal is now the native build and the source of truth; there is no more
> Mac export. The "Fixed (Swift)" items below won't reach Unreal. Treat every defect as a
> **rebuild natively in Unreal** item. The Swift changes stay in `Shared/` as a reference only.

Audit of the procedural architecture in `Shared/` for defects that show under ray tracing and a close
camera, with the Swift fixes made on 23 Sep 2026 (never compiled). Line numbers refer to the files as
they are now.

## How to read this

- **Fixed (Swift)**: changed in `Shared/`. It takes effect in Unreal after the next Mac export, and
  on the phone after the next build.
- **Seal / rebuild (Unreal)**: fix natively in Unreal. Either Swift can't express it well, or it
  must look right before the next export.
- Coordinates are plan metres (x east, z south, y up). In Unreal: X = x·100, Y = z·100, Z = y·100.

## Tessellation constant

`Tessellation` (`Core/MeshKit.swift:9`) has one quality factor:

- **1 on iPhone and visionOS.** Arches get 48 segments per semicircle, and the Rotunda 336 round.
- **3 in the macOS exporter**, set automatically with `#if os(macOS)`. The app is iPhone-only, so no
  exporter change is needed. Arches get 144 segments per semicircle, and the Rotunda 1008 round.
- **Override:** `MUSEE_TESSELLATION=1…4`. It is capped at 4: above that, the first arch station
  would fall within the 0.1 mm station rounding of the jamb.

Surfaces that meet always derive their counts from the same expression, so no quality setting
opens a seam. The phone gains roughly 120k triangles (mostly the Rotunda coffers, about 50k). At
quality 3 the export gains roughly 1M, which Nanite handles easily.

## Defects by severity (most visible first)

| # | Defect | Where | Status |
|---|---|---|---|
| 1 | **Rotunda dome: sky through slits.** The 28 coffer cells per ring were flat chords. Where they met the smooth plain bands (4° and 64°) they left 28 crescents up to 6 cm deep, open to the sky. The drum (256, starting 22.5°, overrunning 0.01 rad), the dome bands (112), the coffers (28) and the eye curb (64) all had different vertices. | `Wings/Rotunda.swift:30-127`, `Core/Realism.swift:196` | **Fixed**: one count round everything (`around`). Coffers are subdivided on the sphere with smooth normals. |
| 2 | **The Rotunda dome had no outside.** It was a single inward skin on an open-topped drum. From the Hall of Light glass, the gardens or a free camera, the dome vanished and showed the interior. | `Wings/Rotunda.swift:129-148` | **Fixed**: new `Dome exterior` shell from the drum's outer face (r 11.2 m) up to a flat ring at the lattice (20.3 m). |
| 3 | **Light through every opening.** (a) The wall faces sampled arched heads at equal *lateral* steps (32 across), while reveals used equal *angles*. Near the springing the two parted by up to 3 cm, the "faceted arch". (b) On curved walls, the outer face's hole is up to 12% wider than the parallel reveal: slits of 11–24 cm at the jambs and a crescent over the arch, looking into the hollow wall and through it. (c) Niches were built on the tangent plane, so on the Rotunda drum each jamb had a 5 cm slit open to the garden (the user's screenshot). (d) Reveals stuck 1 cm out past both faces. | `Core/MeshKit.swift:268-300` (stations), `:365-436` (reveal), `:442-481` (niche) | **Fixed in MeshKit, all wings.** Reveals and niches are built from the wall's own stations, sharing every vertex. Heads use equal angles. Curved walls get an outer "return" closing the gap. Niches are sliced at the stations. Jamb stations snap exactly to the jamb (`WallOpening.lateral`, `:137`). |
| 4 | **Salon vault crescents.** Each 9.7° coffer row was one chord, 2.5 cm inside the plain barrel strips at both ends of every bay (open to the lanterns and sky). The lunettes used a third polygon. | `Wings/Salon.swift:72-108` | **Fixed**: coffer rows are subdivided, and strips and lunettes take the same `arc` angles. |
| 5 | **Oval benches in pieces.** Each bench was a chain of boxes, each at its own yaw, leaving wedge gaps on the outer edge. | `Core/MeshKit.swift:643` (`curvedBench`), used at `Wings/Salon.swift:245` | **Fixed**: one mitred solid, smooth side normals, 3 cm chamfer on the top edges, square ends at the same plan length (as on look target 03). **Also rebuild natively until the export**: the target shows a slab on two plinths, not a solid block. |
| 6 | **The Oval passage pokes into the Oval.** It ran to x −75.9, while the Oval's inner face on the axis is at −75.5. That left a 0.4 m barrel "hood" and 0.18 m wall fins inside the room, and it also overlapped the reveal. | `Wings/Salon.swift:167-192` | **Fixed**: runs from the far wall's outer face (−74.6) to 0.3 m inside the Oval wall at the jambs. |
| 7 | **Z-fighting where passages overlap reveals.** The Rotunda→Salon and Rotunda→Sculpture passages start inside the drum at −10.6 and share the reveal's soffit plane near the crown. The Salon→Oval passage started 5 cm inside the far wall. The Sculpture passage overlapped the court reveal by 1 cm. | `Wings/Rotunda.swift:185-212`, `Wings/SculptureHall.swift:68,140`, `Wings/Salon.swift:167` | **Fixed**: passages are 5 mm wider than their doors (the floors widen too), so soffits never share a plane. The Sculpture passage now ends exactly at the court wall. |
| 8 | **Loop seams z-fight and can open.** The circular loops overran by 0.01 rad (10 cm of coincident, differently-UV'd wall at the Rotunda's 22.5° seam, the Oval's west apex and the Atrium's east point). The rectangular loops overran by 1 cm and left the outer corner at the loop start open (0.6 × 0.6 m shaft). `stations()` also dropped the last station whenever it rounded *up*, which could leave the last strip (≤12 cm, full height) unbuilt at the seam. | `Wings/Rotunda.swift:39`, `Salon.swift:197`, `Elan.swift:114,315`, `SculptureHall.swift:48`, `ChineseWing.swift:113`, `Reserve.swift:349`, `Core/MeshKit.swift:294` | **Fixed**: every loop closes exactly. Outer faces are mitred at vertices (`outerOffset`, `:255`). The run's end stations are always kept. **Unreal:** check the current import at those seams (Rotunda at 22.5° ESE, plan (9.24, 3.83), Oval at x −97.5, Atrium at x 68). A slot there means the dropped-strip bug hit. |
| 9 | **Flat-shaded curves.** Lathes had one normal per profile edge: the Sphere and drum glass in bands, the vases and the case rings. Coffers had one normal per face. The Reserve vault was a 0.25 m heightfield with flat per-cell normals, coarse at the springing. The Chinese roofs were flat-shaded 0.32 m cells. On the Rotunda and Oval, the strip before each vertex took the next segment's normal, which rotated 90° across the last strip before every room corner. | `Core/MeshKit.swift:750` (lathe), `Reserve.swift:370-414`, `ChineseWing.swift:206-262`, `MeshKit.swift:240` | **Fixed**: normals are blended where the turn is under 35° (lathes) or 20° (wall runs), with sharp corners kept. The Reserve vault is sampled at equal angles, with normals from the barrel that shows in each cell (groins stay crisp). The roofs get normals from the profile slope. |
| 10 | **Chinese roofs miss their fascias.** On the plain 0.32 m grid the east roof ended 14 cm outside its fascia and the west one 2 cm inside, leaving the open underside visible. | `ChineseWing.swift:216-253` | **Fixed**: the grid includes the eave lines ±4.9 and 28.4. |
| 11 | **Moon-gate threshold z-fight.** The reveal of the round opening was clamped to y 0, a soffit lying on the paving. | `Core/MeshKit.swift:423` | **Fixed**: edges at floor level are skipped. |
| 12 | **Stereographs z-fight.** Both views of each stone share one plane and were both enabled until you came within 12 m. The export had both visible. | `HallOfLight.swift:178` | **Fixed**: the second view starts hidden (it will export invisible, `enabledAtStart = 0`). In Unreal, keep one visible, or offset the other by 0.5 mm. |
| 13 | **Mouldings open at openings.** Skirtings and cornices stopped at door jambs with open ends (4 × 18 cm holes). | `Core/MeshKit.swift:486` (`band`) | **Fixed**: end caps where an opening interrupts a band. |
| 14 | **Coarse round things.** The Chinese cloister columns had 12 sides. Arch-continuing passage vaults had 32 segments. The transverse arches and lunettes had 48. | `ChineseWing.swift:182`, passages | **Fixed**: columns have 32 sides; the rest follow `Tessellation`. |
| 15 | **Single-sided shells.** The Salon's long walls, the cabinet, the passages and the Hall of Light end are single-faced (`faces: (true, false)`). The Salon vault, lanterns, Sculpture ceiling, Chinese roofs and vestibule are single skins. No wall has a top cap. | `Salon.swift:28-29,140`, `Rotunda.swift:196-197`, `SculptureHall.swift:81-91`, `ChineseWing.swift:80-93` | **Not changed**: see rebuild item 3. Lumen and ray-traced shadows leak through single planes. |

## Per wing

### Rotunda (`Wings/Rotunda.swift`)

**Fixed**
- `around` = 28 × 12·q (`:34`) is shared by:
  - the drum loop (`:39`), now closed exactly;
  - both plain dome bands (`:86-88`);
  - the coffers (subdivided 12·q × 6·q per cell, `:95`);
  - the curb, lattice ring and glass (`:106,125`);
  - the new exterior shell (`:146-148`).
- The 22.5° drum start is a vertex of the ring (336/16 = 21).
- The niches (4) and doors (4) are fixed by MeshKit. The W passage is 5 mm wider (`:193`).

**Still wrong: rebuild natively (the other agent's rebuild should cover this)**
- **Coffers are uniform in elevation** (12° rows, `:94`), so the top rows are 1.2 m wide by 2.1 m
  tall. On target 05 they diminish and stay square: the row height should shrink with cos(elevation).
- **The coffer profile is one bevel.** Target 05 has three stepped fillets.
- **The pilasters are plain boxes** (`:64-80`). Target 05 has fluted pilasters with Attic bases and
  capitals.
- **Missing elements:**
  - an entablature and cornice at the springing (only a 0.32 m rectangular band, `:56`);
  - archivolts on the 4 portals and 4 niches;
  - the 150 mm shadow gap at the floor;
  - marble (the build uses travertine).
- The passages start inside the drum at x/z ±10.6 and overlap the reveals (now 5 mm clear).
  Natively, end them at the drum's outer face and close the drum with a proper jamb return.

### Salon, Manet cabinet, Nymphéas oval (`Wings/Salon.swift`)

**Fixed**
- The coffer rows are subdivided (`perRow`, `:83`), and the strips and lunettes share `arc` (`:84-107`).
- The end-wall, far-wall, cabinet and Oval doors are fixed by MeshKit.
- The Oval loop is closed exactly (`:197`).
- The Oval passage extent and clearance (`:167-192`).
- The benches (`:245`).

**Still wrong**
- **Walls and vault are single skins.** The long walls are single-faced (`:28-29`), and the vault
  is one skin with open-sided lantern boxes (`:110-123`).
- **Missing Salon elements (targets 01 and 02):**
  - the round viewing stones set flush in the floor on the axis, which are not in the Swift at all;
  - the cove-light slot at the springing (only a 0.35 m band at 6.2–6.6 m);
  - piers with imposts (the transverse arches spring straight off plain boxes, `:57-69`);
  - a narrow glazed crown skylight with mullions (a 3 m opening into a glowing box, `:110-123`);
  - a coffer grid denser and profiled like the target.
- **The Oval (target 03)** needs:
  - a coved ceiling ring between the wall and the velarium (the velarium is a paraboloid
    starting at the wall top, `:224-238`);
  - benches as a slab on two plinths;
  - a flat-headed door with a stone architrave (the plan has an arch).
- **The cabinet** is single-faced, and its ceiling is one quad (`:128-160`).

### Reserve (`Wings/Reserve.swift`)

**Fixed**
- The vault: equal-angle stations and smooth per-barrel normals (`:370-414`).
- The loop is closed exactly (`:349`).
- The west door is fixed by MeshKit.

**Still wrong (target 04)**
- **It is not a groin vault.** It is the union of 6 m cross barrels and a 1.8 m aisle barrel,
  springing at 1.9 m.
- **The supports don't match.** The columns are 0.5 m brick boxes to 2.0 m (`:417+`). The target
  has square brick piers about 1 m wide, stone bases and imposts, ribs over the piers, and a
  springing of about 3 m.
- **The UVs stretch the brick** near the springing, because they are projected as (x, z + y).

Rebuild natively (item 4 below).

### Sculpture Hall (`Wings/SculptureHall.swift`)

**Fixed**
- The loop is closed exactly (`:48`).
- The S door is fixed by MeshKit.
- The passage ends at the court wall (`:68`), is 5 mm clear (`:140-160`), and its floor is widened (`:75`).

**Still wrong**
- **The ceiling is single skins** at 9.65 m with a reveal round the laylight. There is no roof or
  wall top, so from above the court is open.
- **The mouldings are rectangles** (`:56-60`).

### Chinese Wing (`Wings/ChineseWing.swift`)

**Fixed**
- The moon gate is fixed by MeshKit: 96 equal-angle stations round it on the phone, no floor
  soffit, and the wall and reveal share vertices.
- The loop is closed exactly (`:113`).
- The roofs: eave alignment and smooth normals (`:206-262`).
- The columns have 32 sides (`:182`).

**Still wrong**
- **The roofs** are one heightfield skin with a flat tile texture. They need real tiles, ridges,
  eaves and brackets.
- **The moon-gate surround** is a flat 64-segment ring floating 1 cm off the wall (`:135-140`).
- **The vestibule** is single quads (`:80-93`).
- **The Taihu rock and the trees** are low-poly stand-ins (`:311-352`, `Core/Garden.swift`).

### Hall of Light (`Wings/HallOfLight.swift`)

**Fixed**
- The stereograph z-fight (`:178`).

**Still wrong**
- **The lattice is a polygon of boxes.** Each arch is 32 straight boxes, with wedge gaps and
  overlaps at every joint, and the longitudinals and diagonals are boxes too (`:86-107`). Rebuild
  as swept steel sections.
- **The glass vault** has 32 segments (`:79`). This is acceptable.
- **Target 06 is superseded** (mood only).

### Élan: Atrium, Square, Sphere (`Wings/Elan.swift`)

**Fixed**
- The Atrium loop is closed exactly and now shares the coping's vertices (`:114`).
- The flat W door on the curved base gets its outer return from MeshKit (it had an 11 cm slit per jamb).
- The drum glass and the Sphere share `around` (`:204-213`) and shade smoothly.
- The Square loop is closed exactly (`:315`).

**Still wrong**
- **The ribs** are 24-segment strips with a flat 8-triangle haunch fan (`:142-190`).
- **The glass is doubled.** Misty glass and the Sphere are `inside: true` lathes: two coincident
  opposite-facing skins. That is fine for raster, but in Unreal use one two-sided translucent
  surface.
- **Target 07** (mood only) wants white ribs and a warm cove at the base.

## Openings to seal natively before the next Mac export

Every `WallRun` opening in the build, and which defect it has *in the current `usd/`*:

- **A**: arched head (wall face and reveal disagree by up to 3 cm near the springing).
- **B**: outer-face gap on a curved wall.
- **C**: niche jamb slit.
- **D**: the reveal lip, 1 cm proud of both faces.

After the re-export all of these are closed. Until then, seal them in Unreal: re-cut the opening
with Modeling Mode booleans, or place a thin blocker (5 cm) behind each jamb and arch that follows
the reveal.

| Wing | Opening | Centre (x, z) | Size | Wall | Defects |
|---|---|---|---|---|---|
| Rotunda | S door → Chinese Wing | (0, 10) | 4 m, spring 4.5, crown 6.5 | drum r 10→11.2 | A, B (24 cm per jamb, crescent over the arch), D |
| Rotunda | W door → Salon passage | (−10, 0) | 3 m, spring 4.5 | drum | A, B (18 cm), D |
| Rotunda | N door → Sculpture passage | (0, −10) | 4 m, spring 4.5 | drum | A, B (24 cm), D |
| Rotunda | E door → Hall of Light | (10, 0) | 3 m, spring 4.5 | drum | A, B (18 cm, **visible from the Hall of Light**), D |
| Rotunda | 4 niches | (±7.07, ±7.07) at 45/135/225/315° | 2 m wide, sill 2.0, spring 4.67, top 5.67, 1 m deep | drum | **C (5 cm slit, both jambs and round the head: the user's screenshot)**, A |
| Salon | Bay 1 end-wall door | (−14 → −13.4, 0) | 3 m, spring 3.3 | straight | A, D |
| Salon | Bay 5 far-wall door | (−74 → −74.6, 0) | 3 m, spring 3.3 | straight | A, D, plus the Oval passage overlap |
| Salon | Manet cabinet door | (−20, −7 → −7.6) | 3 m, spring 3.3 | straight, single-faced; cabinet side has no reveal | A (both faces) |
| Oval | E door | (−75.5 → −74.9, 0) | 3 m, spring 3.3 | oval, radius ≈ 5.1 m at the apex | A, B (≈17 cm), plus the passage hood inside the room |
| Sculpture Hall | S door | (0, −14.1 → −13.5) | 4 m, spring 4.5 | straight | A, D, plus the passage's 1 cm coplanar overlap |
| Chinese Wing | Moon gate | (0, 13.5 → 12.9), centre height 1.55 | Ø 3.6 | straight | A (the wall face cut 2.8 cm into the circle at the sides), threshold z-fight |
| Reserve | W end-wall door | (−74 → −74.6, 0), floor −5.8 | 2.4 m, spring 2.0 | straight | A, D |
| Élan Atrium | W door → Hall of Light | (40 → 39.2, 0) | 4 × 4.5 m, flat head | circle r 14→14.8 | B (11 cm per jamb, flat head), D |
| Élan Square | 4 partition doorways | level −9 | 2.6 × 3.0 m, flat | straight | D only |

## Where geometry stops us matching the look targets

- **01 and 02, Salon**:
  - no flush round viewing stones on the axis;
  - no cove slot at the springing;
  - the piers lack imposts;
  - coffers too coarse, with a 3 m skylight box instead of a narrow glazed crown skylight.
- **03, Oval**:
  - benches are solid, where the target has a slab on two plinths (now at least continuous);
  - no coved ceiling ring round the velarium;
  - the door is arched, where the target has a flat door with an architrave;
  - the pond is an 8.4 × 4.6 m ellipse, where the target shows a round basin (a plan difference;
    keep the plan).
- **04, Reserve**: not a groin vault; low 0.5 m columns with no stone bases (see above).
- **05, Rotunda**:
  - box pilasters;
  - no entablature, archivolts or shadow gap;
  - coffers not diminishing, and single-stepped.
- **07, Atrium**: ribs and cove are a materials and lighting matter; the geometry is close enough.

## Rebuild natively in Unreal (ordered by payoff)

1. **The Rotunda** (in progress elsewhere): the coffered dome with diminishing, triple-stepped
   coffers; the entablature; fluted pilasters with bases and capitals; archivolts; the shadow gap;
   and a thick closed drum and dome. It is the arrival hall and target 05, and Swift's box and
   bevel vocabulary can't reach it.
2. **Mouldings as swept profiles** (cornices, skirtings, imposts, archivolts, door architraves) in
   every wing, using spline meshes or Modeling Mode sweeps. `band()` only makes rectangles, and
   the profile is what reads as "museum" at close range.
3. **Closed, thick shells for Lumen:**
   - Salon walls, vault and lanterns;
   - the cabinet;
   - all passages;
   - the Sculpture Hall ceiling and roof;
   - the Chinese roofs and vestibule;
   - wall tops.

   Lumen's distance fields and ray-traced shadows leak through single planes. This is the "walls
   are single-sided" complaint, and it can't be fixed from the importer.
4. **The Reserve's groin vaults** on square brick piers with stone bases and imposts (target 04).
   The Swift heightfield is a different structure.
5. **The Salon vault and floor details** (targets 01 and 02): a denser profiled coffer grid, a
   narrow glazed crown skylight with mullions, a cove slot, flush viewing stones, and piers with
   imposts.
6. **Then:**
   - the Oval benches (slab on plinths) and cove ring;
   - the Chinese tile roofs and moon-gate surround;
   - the Hall of Light lattice as swept sections;
   - the Élan ribs.

## Re-export notes (Mac)

- **Everything above takes effect** with `tools/usd-export/export.sh`, at quality 3 automatically.
  Pass `MUSEE_TESSELLATION=4` for more.
- **Prim paths are unchanged** except for one addition, `/Museum/Rotunda/Dome_exterior` (new, stone
  material `travertine_honed_F3ECE0`).
- **The HallOfLight stereograph views** `…_2` will import `visibility = invisible`.
- **Swift was not compiled here.** Watch the first Mac build for type-check errors in:
  - `Core/MeshKit.swift`: `WallRun`, `reveal`, `niche`, `band`, `curvedBench`, `lathe`;
  - `Core/Realism.swift`: `coffers`;
  - `Wings/ChineseWing.swift`: roofs.
- **After export, check in Unreal:**
  - no light through any row of the table above;
  - no crescents at dome rings 4° and 64°;
  - no slit where the Salon's coffers meet the plain vault strips at the bay ends;
  - the loop seams listed in row 8;
  - the Oval passage no longer pokes into the room.
