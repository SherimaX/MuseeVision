# The Grounds · stage sheet

**Stage:** `grounds`. Module `MuseeGrounds` (Editor), class `UGroundsRoom : USiteRoom`, in
`Source/Stages/Rooms/Grounds`. The one stage whose footprint is of kind **Ground**.

**What it is:** the museum's site. A French formal front laid south of the portico: a stone parvis, a gravel
cour round a 30 m still canal, two lawn panels edged in clipped box, allées of round-headed osmanthus, a yew
exedra closing the cross walk, and an avenue of birches running south. From the cour a gravel walk runs east,
bends round Albion's south end, and reaches a rond-point with a round basin on the Élan's meridian. A path
climbs north from the rond-point to a gravel terrace round the Élan's plinth. Everything else is mown lawn up
to the walls, with groves of birch and evergreen osmanthus round the buildings, under the museum's one sun,
moon and sky.

**Sources, in the order D-001 ranks them:** D-001, D-003, D-005, D-006 and D-012 to D-016
(`Journal/Decisions.md`); `Docs/Architecture.md` (stages, How rooms join, Naming); the approved Albion
proposal (`plan/proposals/albion`, board *Outside*: the walk bends round Albion's south end); the master boards
(`MasterPlan`: *The front* and *The grounds*; `AsBuilt`: *Lawn up to the walls*; `MaterialsOther`); the as-built
code (`windows/Source/MuseeVision/Plan/MuseePlan.h` and `Facade/MuseeLandscape.*`, `Nature/*`, `Ground/*`,
`Scripts/nature.py`, `Scripts/exterior_materials.py`). Every number below comes from one of these. The old
code line for each number is in section 11.

---

## 1. Identity and placement

**Frame.** Metres. x is east, y is south, z is up, and plan north is true north (Foundation's `FMuseeFrame`).

**Origin.** The site datum: the plan point under the gilt sun at the Rotunda's centre (the arrival point), at
z 0.00, the museum's ground-floor finished level. So the Grounds' frame *is* the museum plan frame, and every
number in `MuseePlan.h` carries over unchanged.

**Placement in the museum plan.** Site places the Grounds at translation (0, 0, 0) from the anchor's origin.
The Grounds has no port (section 2), so Mate cannot place it from a mating. `SitePlan.json` needs a ground
entry: the Ground stage stands at the anchor's datum. This is a Site capability and needs a decision record
(see the open points).

**Level.** One level, the ground. The lawn's soil surface is at **z −0.03**, 3 cm under the buildings'
floors, as built. Made surfaces stand a little proud of it:

| Surface | Top z (m) |
|---|---|
| Lawn (soil surface) | −0.030 |
| Gravel walks | +0.018 |
| Parvis paving | +0.024 |
| Kerb tops | +0.045 |
| Basin copings | +0.450 (sitting height) |
| Still water in the basins | +0.300 |
| Basin floors | −0.250 |

**Footprint.** Kind **Ground**. The plan polygon is the 800 m square of the as-built ground plane:
(−400, −400), (400, −400), (400, 400), (−400, 400). Site.OneGround judges containment in plan. The Grounds'
own pieces live between **z −0.32** (the deepest coping foot is −0.31) and z +13.0 (the tallest birch is 12.0 m,
plus its sway). They stay outside every Building volume it receives.

**What the Grounds receives at Detail** (by footprint kind, never by name):
- every placed Building's **at-grade outline**, the line where the building meets the ground;
- every joint passage's at-grade outline;
- the top z of every Building that lies *under* the ground.

The Grounds lays nothing inside an at-grade outline. Its terrain is cut by the outlines. The lawn therefore
comes "up to the walls" and is "cut back wherever the building opens into the ground" (the `AsBuilt` board) by
rule, with no holes drawn by hand.

**Outlines the Grounds expects**, for the builder's sanity check (each is the other stage's to declare; these are
the as-built numbers):

| Building | At-grade outline as built |
|---|---|
| Rotunda (stone complex) | Drum outer face r 11.2. Podium 0.54 m proud, then a gravel strip and stone edge to **1.56 m** from the face: r 12.76 on the free arcs. |
| Salon, cabinet, oval, hyphen | The wall faces (y ±7.6; x −74.6 … −13.4; cabinet x −24.4 … −15.6 to y −16.0; the oval's outer face) plus 1.56 m: the parvis meets it at **y 9.16**. |
| Salon's portico | Steps and cheek walls x −56.8 … −31.2, from y 7.6 to **y 17.3** (the first nosing). |
| Chenghuai | x −20.7 … 16.9, y −61.9 … −17.0, and its open porch \|x\| ≤ 2.5 from the drum to y −17.0. |
| Albion | Walls x ±14.6 with buttresses to x ±15.5, y 16.6 … 53.8; its porch \|x\| ≤ 3.6 from the drum. |
| Hall of Light with its gardens | x 10.9 … 41.1, \|y\| ≤ 14.25 (the old lawn exclusion; see section 3.12). |
| Atrium (Élan) | The socle, r **15.30** about (54, 0), and the portal frontispiece west of it (x 39.0, \|y\| ≤ 6.05). Its cornice (r 15.70, at z 20.9 … 22.58) oversails the terrace; that is allowed because the check is by volume. |
| Underground: Reserve | x −74.6 … −13.4, \|y\| ≤ 12.6, top **z −0.33** where it lies under lawn. |
| Underground: Classical Hall | Roof z −0.55. |
| Underground: Cube | Under the Atrium, about ±14 m round (54, 0) plus its walls. Its top must be at z −0.33 or lower. |

**Enclosures.** One enclosure, **`grounds.open`**, of kind **Outdoors**: the footprint minus every received
outline. It has no shell, so rain and snow fall everywhere in it. `RoomAt` returns the Grounds there.

---

## 2. Ports

**None.** The Grounds has no doorway, stairwell, shaft or gate. It serves no well and receives none.

- **D-012 is exhaustive.** The Rotunda's ports are west, north, south and east, plus the stairwell it serves.
  The Hall of Light meets the Atrium; the Salon's stair meets the Reserve; the Atrium's vertical ports meet the
  Cube and the Sphere.
- **No as-built door leads outside.** The portico's bronze door is a **blind** dress on the Salon's skin, 0.14 m
  deep and never cut into its wall (`Door.bBacked = false`, `bGlazed = false`).
- **The panel's mating is superseded.** `rotunda.entrance ↔ grounds.gate` (Architecture.md, How rooms join) was
  the panel's drift, and D-012 corrects it.

**Wells: none.** The museum's three ways down all lie inside Building outlines: the Rotunda's stair (shaft
r 7.6), the Élan car's shaft, and the Salon pond's stair. The old ground plane needed holes for them only
because it ran *under* the floors. The new terrain stops at the outlines instead.

**D-012 topology from the Grounds' side.** The Grounds is the site that contains the mated graph. It meets
every Building only along at-grade outlines. There is no joint, doorcase or bronze strip between the Grounds and
anything else. The visitor never walks onto it in the visit; it is seen, not entered (section 10).

---

## 3. Pieces by trade

Every piece below is drawn in the Grounds' frame. Keys are dot-scoped under `grounds.`.

### 3.1 Setting-out lines

| Line | Value | Source |
|---|---|---|
| The front's axis | x **−44.0** (the portico's axis, the Salon's mid-length) | `Grounds::AxisX = Facade::PorticoAxisX` |
| Parvis | y 9.16 … 23.0, x −60.0 … −28.0 (axis ± 16) | `ParvisY0/Y1/Half` |
| Allée walks beside the parvis and cour | axis ± 16 … 20: x −64 … −60 and −28 … −24 | `WalkInner/Outer` |
| Cour (gravel round the canal) | y 23.0 … 62.0 | `CourY1` |
| Canal (to the coping's outer face) | axis ± 4.5, y 27.0 … 57.0 | `CanalHalf/Y0/Y1` |
| Lawn panels (tapis verts) | \|x − axis\| 7.5 … 14.0, y 27.0 … 55.0 | `PanelInner/Outer`, `CanalY1 − 2` |
| Cross walk | y 57.0 … 62.0 | `CourY1` |
| Avenue | axis ± 4.0, y 62.0 … 130.0 | `AvenueHalf/Y1` |
| Walk east | y 40.0 … 44.0, bending round Albion (3.4) | `EastWalkY0/Y1`; the Albion *Outside* board |
| Rond-point | centre (54.0, 42.0), ring r 7.0, basin r 3.0 | `RondPoint*` |
| Élan terrace | about (54.0, 0.0), r 15.32 … 19.40, −125° … +125° through east | `ElanTerrace*` |

Angles are measured from east towards south (+x towards +y), as in the old code.

### 3.2 Ground (terrain and covers) · Nature `Ground`, Form heightfield

- **Terrain `grounds.terrain`.** A flat heightfield: the site is level. Its top is at z −0.03 over the whole
  footprint (±400 m), and it is made into one Nanite mesh (no Landscape).
  - It is cut by every received at-grade outline, and by the Grounds' own hard surfaces (paving, gravel,
    kerbs, basins). Each of those replaces the terrain under its own outline.
  - It is modelled as a solid 0.20 m thick (bottom z −0.23), so collision and the leak probe see a solid.
  - Over the underground Buildings it keeps above their tops: 0.10 m clear of the Reserve's −0.33.
- **Covers on the terrain:**
  - **`lawn`**: mown fine sward everywhere on the terrain.
    - As built: blades 3–6 cm, 12,000 blades/m², tillers of 2–4 blades, most with the mower's blunt tip.
    - Mowing stripes are 2 m wide along the north–south axis.
    - Full sward over the region x −135 … 125, y −75 … 145. Beyond it the sward thins and shortens over a 30 m
      fringe, to 0.45 of full height at the outer edge.
    - It keeps 0.025 m clear of anything standing on it, and 0.06 m clear of hedge cores.
  - **`gravel`**: the walks in 3.4, laid over the terrain's cut to top z +0.018.

### 3.3 Paving · the parvis `grounds.parvis` (Form laid surface)

- **Region.** x −60.0 … −28.0, y 9.16 … 23.0, minus the received outlines (the portico's steps and cheeks take
  x −56.8 … −31.2 up to y 17.3). As built, 234.5 m² of stone shows.
- **Setting-out** (a mason's division of the as-built rectangle):
  - 17 courses running east–west, each **0.8141 m** deep, from y 9.16 to 23.0. The course joint at y 17.30
    falls on the portico's first nosing.
  - Slabs are **1.1852 m** long: 27 to a full course, with the middle slab centred on the axis x −44.0.
  - Alternate courses are offset half a slab, cut at the ends.
  - Slabs are **0.10 m** thick (top +0.024, bottom −0.076). Joints are 6 mm.
- **Flat as built.** No falls are designed. Puddles form from each slab's laying tolerance, read by Form's
  ponding field.
- **Material:** `travertine.honed.weathered`, with traffic lanes from Site's traffic field (stand only; see
  section 10).

### 3.4 Gravel walks (Nature ground cover `gravel`, top +0.018)

Each region below is one union of plan shapes. Where shapes meet they merge, with no overlaps and no
z-offsets. The old code staggered tops by 1–3 mm (0.018 … 0.021) only to hide z-fighting between overlapping
slabs, and that is not re-expressed.

| Key | Plan region |
|---|---|
| `grounds.allee.west` / `.east` beside the parvis | x −64 … −60 / −28 … −24, y 9.16 … 23.0 |
| `grounds.cour` | x −64 … −24, y 23.0 … 62.0, minus the canal (x −48.5 … −39.5, y 27 … 57) and the two lawn panels (x −58 … −51.5 and −36.5 … −30, y 27 … 55) |
| `grounds.avenue` | x −48 … −40, y 62 … 130. Its south end meets lawn with no kerb, as built. |
| `grounds.walk-east` | (1) x −24 … −17.5, y 40 … 44; (2) x −21.5 … −17.5, y 44 … 62; (3) x −17.5 … 17.5, y 58 … 62; (4) x 17.5 … 21.5, y 44 … 62; (5) x 17.5 to the ring's arc (x 47.29 at y 40 and 44), y 40 … 44 |
| `grounds.rond-point` | annulus about (54, 42), r 3.0 (the coping's outer face) … 7.0 |
| `grounds.path-north` | x 52 … 56, from the terrace's outer arc (y 19.30) to the ring's arc (y 35.29) |
| `grounds.terrace` | annulus about (54, 0), r 15.32 … 19.40, from −125° to +125° through 0°. Area 309 m². Its inner edge is 2 cm off the Élan's socle. |

**Material:** crushed travertine, `travertine.crushed.trodden`. This is the Paris gardens' pale sable: a beige
grit with small stones, colour about #BDAF94, with paler trodden lanes about #D6CBB2. It keeps the one stone.

### 3.5 Kerbs `grounds.kerb.*` (Form sweep, closed profile)

**Profile** (A across, centred on the path; B up; metres), closed:
`(-0.060,-0.160) (0.060,-0.160) (0.060,0.030) (0.045,0.045) (-0.045,0.045) (-0.060,0.030)`.
That is 0.12 m wide, top +0.045 with 15 mm chamfered arrises, and 0.16 m deep.

Stones are 1.20 m long, cut within Matter's travertine stock. Material: `travertine.sawn.weathered`.

| Key | Centre line (open unless marked closed) |
|---|---|
| `panel.west` (closed) | (−51.5, 27) → (−58, 27) → (−58, 55) → (−51.5, 55) |
| `panel.east` (closed) | (−36.5, 27) → (−30, 27) → (−30, 55) → (−36.5, 55) |
| `parvis.west`, `parvis.east` | x −60 and x −28, y 9.16 → 23.0 |
| `parvis.south` | y 23.0, x −64 → −24 |
| `cour.west` | (−64, 9.16) → (−64, 62) → (−48, 62) |
| `cour.east-south` | (−40, 62) → (−24, 62) → (−24, 44) |
| `cour.east-north` | (−24, 40) → (−24, 9.16) |
| `avenue.west`, `avenue.east` | x −48 and x −40, y 62 → 130 |
| `walk-east.inner` | (−24, 40) → (−17.5, 40) → (−17.5, 58) → (17.5, 58) → (17.5, 40) → (47.29, 40) |
| `walk-east.outer` | (−24, 44) → (−21.5, 44) → (−21.5, 62) → (21.5, 62) → (21.5, 44) → (47.29, 44) |
| `rond-point.north-west` | arc r 7.0 about (54, 42), 196.60° → 253.40° |
| `rond-point.rest` | arc r 7.0 about (54, 42), 286.60° → 523.40° (through east and south) |
| `path-north.west`, `.east` | x 52 and x 56, y 19.30 → 35.29 |
| `terrace.outer` | arcs r 19.40 about (54, 0): −125° → 84.08°, and 95.92° → 125° |
| `terrace.ends` | radial at −125° and +125°, r 15.37 → 19.40 |

The gaps are the openings the walks and paths pass through. The ring's gaps are ±16.60° about 180° and 270°;
the terrace's gap is ±5.92° about 90°. Each kerb end meets the next kerb's end exactly (for example
(47.29, 40) and (47.29, 44)), so there are no overlapping stones.

### 3.6 Basins (Form, the containers Nature fills)

**Coping profile** (A out from the nose line, positive away from the water; B up), closed. It has a rounded
nose that overhangs the water, and a rounded outer arris:

```
(0.040,-0.310) (0.500,-0.310) (0.500,0.400)
arc centre (0.450,0.400) r 0.050 from 0° to 90°  -> (0.450,0.450)
(0.030,0.450)
arc centre (0.030,0.420) r 0.030 from 90° to 180° -> (0.000,0.420)
(0.000,0.360) (0.040,0.360)
```

- The coping is 0.50 m wide and its top is at sitting height, 0.45 m.
- The nose overhangs the wetted face by 40 mm, and its underside at 0.36 m throws a shadow line 60 mm above the
  water.
- The **wetted face is at A 0.04**. The water meets that face, never the nose line. The old code left a
  4 cm gap there; this sheet re-expresses it.
- The **floor** is a slab inside the coping ring, from z −0.31 to −0.25. The water is **0.55 m** deep.

**`grounds.canal`**
- Coping outer faces x −48.5 … −39.5, y 27.0 … 57.0. Nose line x −48.0 … −40.0, y 27.5 … 56.5.
- Coping stones are 1.25 m long, with mitred corners.
- Water surface x −48.04 … −39.96, y 27.54 … 56.46: 8.08 × 28.92 m, 233.7 m², **128.5 m³** at z 0.30.

**`grounds.rond-point.basin`**
- Centre (54, 42). Nose r 2.50, wetted face r 2.54, outer face r 3.00.
- The coping is 12 radially jointed stones.
- Water is r 2.54: 20.3 m², **11.1 m³** at z 0.30.

**Materials:**
- copings: `travertine.honed.waterline` (the weathered hone, plus the band the water wets and stains at the
  water level, read from the water body's level by key);
- floors: `travertine.sawn.submerged` (permanently wet, with a dark algal biofilm and silt, so the water holds
  reflections; the old code used a dark "basin bed" stone for this; the one stone stays).

### 3.7 Fittings · the lamp standards `grounds.lamp.01 … 10` (Form lathe + Light luminaire)

A cast-bronze lamp standard, lathed with 20 sides.

**Post profile** (r, z from the foot, metres). An asterisk marks a smooth vertex; the others are creases:

```
(0,0) (0.27,0) (0.27,0.07) (0.23,0.11) (0.21,0.40)* (0.17,0.46)* (0.12,0.52)* (0.10,0.60)
(0.085,0.70) (0.062,3.05) (0.095,3.10) (0.095,3.17) (0.06,3.21) (0.07,3.30)* (0.15,3.38) (0.16,3.41) (0,3.41)
```

The parts, from the foot up:
1. a moulded base, Ø 0.54;
2. a slender tapering shaft, Ø 0.17 at 0.70 m narrowing to Ø 0.124 at 3.05 m;
3. a collar, then a cup whose flat top is at 3.41.

**The globe** is the luminaire's opal diffuser: a sphere r 0.24 about z 3.62, truncated at z 3.41. It stands
on the cup through its lower opening (r 0.116), with glass 3 mm thick.

**The finial** is bronze, lathed with 16 sides. Its underside seats on the globe's crown instead of passing into
it (re-expressed):
`(0,3.860) (0.09,3.842) (0.06,3.910) (0.025,3.940) (0.035,3.980) (0,4.010)`. Overall height is 4.01 m.

**Positions** (foot on the gravel):

| Key | x | y |
|---|---|---|
| 01, 02 | −48.95, −39.05 | 29.0 |
| 03, 04 | −48.95, −39.05 | 37.0 |
| 05, 06 | −48.95, −39.05 | 45.0 |
| 07, 08 | −48.95, −39.05 | 53.0 |
| 09, 10 | −58.50, −29.50 | 21.5 (the parvis's corners) |

The canal lamps stand on the walks' canal side, 0.45 m clear of the coping (axis ± 4.95). Material for the
post and finial: `bronze.cast.weathered`, the one bronze alloy with its outdoor patina.

### 3.8 Clipped hedges (Nature, clipped form)

Each hedge is a leaf shell grown inside a clipped section. The section is:
- battered on both sides;
- softened at its shoulders;
- built over a dark core of bare twigs set Inset inside the clipped surface, and the core carries the collision.

| Kind | Species | Width at foot × height | Batter each side | Shoulder r | Core inset | Leaves / m² |
|---|---|---|---|---|---|---|
| Box run, corner, end | `buxus-sempervirens` | 0.42 × 0.52 | 0.015 | 0.045 | 0.055 | 32,000 |
| Box ball | `buxus-sempervirens` | Ø 0.92, sunk 0.04 | — | — | 0.055 | 32,000 |
| Yew run, end | `taxus-baccata` | 0.90 × 1.70 | 0.04 | 0.08 | 0.075 | 58,000 |

The clipped surface wanders a few millimetres in height along a run (±1.2 % over about 5.5 m). The wander is a
function of position, so joints between runs never show.

| Key | Centre line |
|---|---|
| `grounds.hedge.panel.west` | Closed box loop (−51.95, 27.45) → (−57.55, 27.45) → (−57.55, 54.55) → (−51.95, 54.55). 65.4 m, with convex corners. |
| `grounds.hedge.panel.east` | Closed box loop (−36.05, 27.45) → (−30.45, 27.45) → (−30.45, 54.55) → (−36.05, 54.55). 65.4 m. |
| `grounds.hedge.ball.01 … 08` | x −52.85, −56.65, −35.15, −31.35 at y 28.35 and 53.65 |
| `grounds.exedra.east` | Yew arc r 14.0 about (−44, 62), from 0.41° to 69.08°. 16.78 m. |
| `grounds.exedra.west` | Yew arc r 14.0 about (−44, 62), from 110.92° to 179.59°. 16.78 m. |

- **Box loops.** Each loop runs 0.45 m inside its panel's kerb. Lawn fills inside the loop and in the 0.18 m
  strip between the loop and the kerb.
- **Box balls.** Four to a panel, each 0.9 m in from the loop's corner lines.
- **Exedra.** The semicircle closes the cross walk from the south. It is open ±20.92° about due south, a
  10.0 m chord, for the avenue.
  - Its ends stand on the plane y 62.10, 4 cm clear of the cross walk's kerb. The old code ran them 6 cm into
    that kerb.

### 3.9 Walls, vaults, domes, columns, stairs, glazing

The Grounds has **none of its own**. The only stone "architecture" on the Grounds is the kerbs, copings and
paving above.

### 3.10 Materials (Matter specs, substance.working.history)

| Spec | Where |
|---|---|
| `travertine.honed.weathered` | Parvis slabs. History: outdoor dissolution and pore soiling, with lichen on up-facing faces (`stone.dissolution-outdoors`, `stone.pore-soiling`), plus `stone.foot-traffic-wear` along the lanes. |
| `travertine.sawn.weathered` | Kerbs. Long single stones with worn tops. |
| `travertine.honed.waterline` | Canal and basin copings: the weathered hone plus the wetted, stained band at the water level. |
| `travertine.sawn.submerged` | Basin floors under the water: biofilm and silt, dark. |
| `travertine.crushed.trodden` | Every gravel walk, the cour, the rond-point ring, the path and the terrace (Nature's gravel cover). |
| `bronze.cast.weathered` | Lamp posts and finials (`bronze.weathering`). |
| `opal-glass.blown.cleaned` | Lamp globes: the luminaire's diffuser, shaded by Light's diffuser master. |
| `water.still.clear` | Canal and round basin (Nature's water bodies). |
| `grass.mown.seasonal` | Lawn sward (Nature cover; living family). |
| `leaf.clipped.seasonal` | Box and yew leaf shells (from each species record). |
| `bark.grown.weathered`, `leaf.grown.seasonal` | Birch and osmanthus (from each species record). |

**Matter work these specs need:**
- Workings `crushed` (graded 4–8 mm aggregate, rolled) and `blown` (glass), and the working that `water`
  takes.
- Histories `submerged` and `waterline`.
- Substances not yet recorded: opal glass, water, grass, leaf, bark.

### 3.11 Handover: the façade dress belongs to the Salon and the Rotunda

The old `AMuseeFacadeStructure` dressed every stone wing from outside in one actor, beside the landscape. The
new rule is that **each room owns both faces of its own shell** (Architecture.md, Stages), and a stage never
builds against another stage except at a port. So the dress is **not** a Grounds piece: it goes to the stage
whose wall it covers. The numbers are recorded here so that nothing is lost; the details are in section 11.

**Shared language, used by four volumes.** The promotion rule sends these to `Data/Form` as reviewed catalogue
entries:
- **`order.ionic-vignola`**:
  - column 9 D, entablature 2.25 D;
  - architrave 0.625 D in three fasciae, frieze 0.75 D, cornice 0.875 D with dentils;
  - Attic base 0.5 D; pilaster projection D/6; 24 flutes; entasis.
- **`podium.rusticated`**: 2.0 m high. Profile (d out from the wall face, z), closed at the wall:

  ```
  (-0.02,-0.12) (0.54,-0.12) (0.54,0.36) arc c(0.50,0.36) r0.04 0→90° (0.44,0.40)
  arc c(0.44,0.50) rx0.06 ry0.10 −90→−180° (0.38,0.56) (0.33,0.60) (0.37,0.64) (0.37,1.16) (0.33,1.20)
  (0.37,1.24) (0.37,1.76) (0.33,1.80) (0.40,1.80) (0.40,1.84) ovolo 0.07×0.08 (0.50,1.92) (0.50,2.00) (-0.02,2.00)
  ```

  At its foot is a gravel strip from d 0.50 to 1.45 (top +0.012), with a stone edge from d 1.44 to 1.56
  (top +0.045). This strip-and-edge line is the stone complex's **at-grade outline** that the Grounds meets.
- Balustrade, pedestal and urn; blind window dress (architrave, keystone, sill on consoles, apron, bronze
  glazing bars over a dark backing); the round tablet.

**The Salon** (Salon stage), all on its own outer faces:
- **Giant order** D **1.10**: entablature z 11.9 … 14.375, over its roof slab at 14.05.
- **Pilasters** every 4 m on the long faces:
  - south face x −74.0, −70.0, … −14.0;
  - north face the same, stepping up to the cabinet's roof (6.3) over the cabinet;
  - end faces y ±7.0 and ±3.0.
- **Blind round-arched windows**, 1.70 wide, sill 3.6, springing 7.7, a round tablet r 0.46 at z 10.35 over
  each:
  - south face x −72, −68, −64, −60, −52, −48, −40, −36, −28, −24, −20, −16;
  - north face x −72 … −28 every 4 m.
- **Balustrade**: H 1.15, 0.44 deep, pedestals 1.2 over the pilasters, balusters at 0.36, urns 1.45 m at the
  corners. It is broken at x −58 … −30 by the pediment.
- **Manet cabinet**: D 0.40 (5.6 … 6.5). One arched window 1.10 wide, sill 2.75, springing 4.35. Balustrade H
  0.72.
- **Nymphéas oval**: D 0.484 (6.36 … 7.45). An odd number of bays of about 4.2 m round the outer face, so an
  oculus stands on the axis. Oculi r 0.52 centred at z 4.35. Balustrade H 0.85. The hyphen's roof is at 7.42
  (\|y\| 5.5).

**The portico** is the Salon's entrance front, standing on its podium:
- **Columns**: six Ionic columns of the Salon's order at x −54, −50, −46, −42, −38, −34 on y 12.0.
- **Stylobate** front at y 12.9. **Steps**: 11 risers of 0.1818 m and goings of 0.40, to y 17.3, spanning x
  −55.6 … −32.4.
- **Cheek walls** 1.2 m wide either side, dressed with the podium profile. A pedestal and a 1.7 m urn stand at
  each cheek's front (y 16.35).
- **Ceiling**: coffered, with beams on x −50, −46, −42, −38 and one along the middle.
- **Pediment** pitched at 13.5°, with a round tablet r 0.78 in the tympanum. The gabled roof runs back to
  y 6.9.
- **Inscription** on the frieze: **MVSÉE · VISION** in gilt-bronze Roman capitals, cap height 0.52,
  raised 38 mm.
- **Door**: a blind bronze door, 3.0 wide, from z 2.0 to 7.8, on the axis.

**The Rotunda** (Rotunda stage), on its drum:
- **Order** D **0.827**: entablature 9.44 … 11.30, an attic to 12.15.
- **Pilaster pairs** at 33.75°, 56.25°, 123.75°, 146.25°, 213.75°, 236.25°, 303.75° and 326.25°.
- **Blind windows** on the four diagonals: arched, 1.50 wide, sill 3.2, springing 6.2, a tablet r 0.38 at z 8.2.
- **Stone cover over the dome**: R 11.1 about z 10.0.
  - It starts from two 0.40 m step rings on the attic (outer r 11.25 and 10.98).
  - It rises to r 3.0, with a rolled ring round the eye (r 2.92 … 3.28) on the curb at 20.45.

**Also handed over:**
- The passages' outer skins and small cornices go to the joints that own those passages (Site).
- The Sculpture Hall's dress is retired (D-001).

**Night floodlights of the façade**: control class `facade`, warm white. They belong to the Salon's and the
Rotunda's façade schemes:
- the six portico columns, grazed from their feet;
- the pediment;
- the door;
- ten south-front pilasters;
- eight drum pilasters;
- every other oval pilaster.

Two of them stand **on the Grounds**: in-ground uplighters on the parvis at (−51.5, 22.0) and (−36.5, 22.0),
aimed at the pediment. When that scheme lands, the Grounds leaves a flush 0.30 m square recess in the parvis
for each. That is polish, not tonight.

### 3.12 Handover: the Hall of Light's garden hedges

The two narrow yew hedges along the Hall of Light's gardens were built by the old landscape actor. They are
0.45 × 1.50 m, batter 0.02, shoulder 0.06, inset 0.065, open-ended, on y ±14.0 from x 11.0 to 41.0. They
bound the Hall of Light's meadow and orchard, so they go with the gardens to the Hall of Light stage. Its
at-grade outline is the old lawn exclusion: x 10.9 … 41.1, \|y\| ≤ 14.25. The Grounds builds nothing inside it.

---

## 4. Light

**Daylight.** The Grounds is Outdoors. It has no openings and admits nothing: the one sun, moon and sky light
it directly (Light.OneSunOneSky). Shadows fall from the received buildings (their volumes), the trees and the
hedges.

**Luminaires** (by catalogue type; Light chooses the product):

| Type | Count | Mount | Control class | Operation |
|---|---|---|---|---|
| **Post-top lantern with an opal glass globe**: Ø 0.48 m globe, symmetric distribution, warm white. The as-built lamps were 2800 K; the nearest catalogue CCT is 2700 K. | **10** | On `grounds.lamp.01 … 10`: the diffuser's centre at z 3.62 over the positions in 3.7 | `lantern` | On at dusk and off at dawn by solar altitude (Light's `Operation.json`), all night: the museum never closes (D-006). |

The luminaire drives its globe and its light together (`ULightLuminaireComponent`). The globe is the only
emitting surface on the Grounds.

**Light's catalogue has no exterior post-top luminaire yet.** A real product record with photometry is needed.

**Targets.** No works stand here, so there are no care limits.

| Target | Surface | Class | When |
|---|---|---|---|
| `grounds.canal-walks` | The gravel between the canal's coping and the panels' kerbs, both sides: \|x − axis\| 4.5 … 7.5, y 27 … 57 | `exterior-walkway`, **5 lx** maintained on the ground | After dusk |

The other walks (the avenue, the walk east, the rond-point, the terrace, the parvis's middle) carry **no lamps
as built**, so no target is declared there; they are dark at night, as designed. The parvis's middle will take
spill from the façade floodlights when those land.

**Design targets** (measured luminance order, never knobs):
- **Day, clear sky, sun above 30°:**
  sky at the horizon > parvis in sun > gravel in sun > coping in sun > lawn in sun > box hedge > yew exedra's
  sunlit face > canal water where it does not mirror the sky.
- **The mirror.** Seen from the cour's north edge at eye height (y 25, z 1.6), the reflected portico in the
  canal reads darker than the portico itself (water's Fresnel reflectance at grazing view) and brighter than the
  basin floor seen through the water.
- **Night:**
  lamp globes > canal walks under the lamps > copings > lawn panels > yew exedra (darkest). The canal holds the
  globes' reflections.

---

## 5. Time and weather

- **Glazed openings: none.** The whole Grounds takes the **museum** observer's weather: its rain and snow
  history, wind and temperature. Giverny's and London's weather fall only on the Salon's and Albion's glass
  (D-005).
- **Sun.** The museum's one sun, under the sun model in `Data/Time/Museum.json` (idealised by default, D-003).
  The moon, stars and solar terms come from the Meeus ephemeris.
- **Seasons** run at runtime through Nature's phenology by solar term (MPC_Nature):
  - Birches leaf out in spring, turn yellow in autumn and stand bare through winter.
  - Osmanthus stays evergreen; its new growth is paler from Chunfen to Guyu, and it flowers cream-orange in the
    leaf axils around Qiufen (Bailu to Hanlu).
  - Box and yew stay evergreen; box shows paler new growth in spring.
  - The lawn's colour follows the season.
- **Weather on surfaces** comes from Matter's surface law (MPC_Moment × sky exposure × porosity × Form's ponding
  depth):
  - the wet parvis darkens and puddles form in the slabs' low spots;
  - the gravel darkens;
  - snow settles on the lawn, hedges, copings and kerbs.
- **Weather in the air.** Nature's rain and snow fall over the whole enclosure and stop at the received
  buildings' shells.

---

## 6. Motion

**None.** No mechanisms, gates, doors, blinds or lifts. The water is still (no fountain, no flow). Plants sway
by Nature's wind response, which is not a Motion mechanism.

---

## 7. Nature

### 7.1 Trees (grown by `Grow(species, seed from key, age, growth space)`)

Two species records:
- **`betula-pendula`**, silver birch: white bark with dark lenticels, pendulous fine twigs, small triangular
  leaves about 6 cm.
- **`osmanthus-fragrans`**, sweet osmanthus: a dense evergreen trained round-headed, with glossy lanceolate
  leaves about 8.5 cm.

Each tree grows to fill its **crown envelope**: the crown's centre height, then its horizontal and vertical
radii. Each grows in its growth space: Form's free space at its planting spot, shrunk by the species' sway at
the design wind. Seeds come from the keys.

| Keys | Species | Positions (x, y) | Height | Crown centre | Crown radii (h, v) | Leaf density |
|---|---|---|---|---|---|---|
| `grounds.allee.w.01 … 06` | osmanthus | x −62.0; y 26, 32, 38, 44, 50, 56 | 6.4 | 4.3 | 2.1, 1.9 | 1.4 |
| `grounds.allee.e.01 … 06` | osmanthus | x −26.0; y 26, 32, 38, 44, 50, 56 | 6.4 | 4.3 | 2.1, 1.9 | 1.4 |
| `grounds.avenue.w.01 … 04` | birch | x −50.5; y 80, 92, 104, 116 | 11.5 | 7.6 | 2.3, 3.2 | 0.8 |
| `grounds.avenue.e.01 … 04` | birch | x −37.5; y 80, 92, 104, 116 | 11.5 | 7.6 | 2.3, 3.2 | 0.8 |
| `grounds.walk.s.01, s.02, s.03` | birch | (−12, 63.8), (3, 63.8), (18, 63.8): south of the bend's south leg | 10.5 | 7.0 | 2.2, 3.0 | 0.8 |
| `grounds.walk.n.04`, `grounds.walk.s.04` | birch | (33, 38.2), (33, 45.8): either side of the walk east | 10.5 | 7.0 | 2.2, 3.0 | 0.8 |
| `grounds.grove.01, 02` | birch | (−40, −44), (−33, −49): north of the Salon | 12.0 | 8.0 | 2.6, 3.4 | 0.8 |
| `grounds.grove.03` | osmanthus | (−47, −51) | 7.0 | 4.4 | 2.4, 2.1 | 1.4 |
| `grounds.grove.04, 05` | birch | (−112, −8), (−115, 7): beyond the oval | 12.0 | 8.0 | 2.6, 3.4 | 0.8 |
| `grounds.grove.06` | birch | (−92, 44): west of the parterre | 12.0 | 8.0 | 2.6, 3.4 | 0.8 |
| `grounds.grove.07` | osmanthus | (−98, 56) | 7.0 | 4.4 | 2.4, 2.1 | 1.4 |
| `grounds.grove.08` | birch | (88, −18): east of the Élan | 12.0 | 8.0 | 2.6, 3.4 | 0.8 |
| `grounds.grove.09` | osmanthus | (92, 10) | 7.0 | 4.4 | 2.4, 2.1 | 1.4 |
| `grounds.grove.10` | birch | (30, 66): south of the walk | 12.0 | 8.0 | 2.6, 3.4 | 0.8 |
| `grounds.grove.11, 12` | osmanthus | (−20.5, 18), (−20.5, 30.5): between the parterre and Albion | 7.0 | 4.4 | 2.4, 2.1 | 1.4 |

**Totals:** 20 birches and 17 osmanthus, **37 trees**.

- **Retired:** the bamboo against the Chinese Wing's walls went with the wing.
- **Not the Grounds':** the Hall of Light's meadow birches, orchard and flowers, Chenghuai's garden plants, and
  the Salon's lilies.
- Each tree stands on lawn or in gravel with its root flare at the ground.
- Where a crown comes near a received outline, the growth space keeps it off the wall. The nearest cases are
  groves 11 and 12, 2.6 m from Albion's buttresses.

### 7.2 Clipped hedges

Box (`buxus-sempervirens`) and yew (`taxus-baccata`), as drawn in 3.8: 131 m of box in two loops, 8 box balls
and 33.6 m of yew exedra.

### 7.3 Water bodies (`UNatureWaterComponent`, still)

| Body key | Basin body key | Volume | Level | Character |
|---|---|---|---|---|
| `grounds.canal.water` | `grounds.canal` | 128.5 m³ | z 0.30 | Still. The wind's slow ripples. Rain rings when wet. |
| `grounds.rond-point.water` | `grounds.rond-point.basin` | 11.1 m³ | z 0.30 | The same. |

Nature.WaterContained is judged over the basins' closed copings and floors.

### 7.4 Ground

The flat terrain and its `lawn` and `gravel` covers are described in 3.2 and 3.4. No rock is formed by process
here.

---

## 8. Art

**None.** No works, casts, scans, frames, cases or plinths. No fittings: the as-built grounds have no benches,
stanchions or signs. The inscription MVSÉE · VISION belongs to the Salon's portico (3.11).

---

## 9. Sound

- **Acoustic character.** Outdoors, a free field over a hard ground plane. There is no enclosure
  reverberation: Sound's acoustics for an Outdoors enclosure are the direct sound plus one ground reflection.
  The received buildings' faces reflect as Form's exposed surfaces.
- **Weather ambience** comes from the museum Moment. Rain is placed on the exposed surfaces by substance:
  - travertine paving and copings;
  - crushed-travertine gravel;
  - open water (the canal and the basin);
  - leaves (birch, osmanthus, box, yew);
  - lawn.
- **Wind in the foliage** is keyed to the Moment's wind: the birches' leaves rustle and the evergreens hiss.
- **Footfalls** are by substance (gravel crunch, stone, grass) through `FMuseeContact`. They are heard only on
  the stand, because the visit never walks here.
- **No emitters of the stage's own.** No fountain and no recorded ambience. Birdsong would need recorded
  evidence per season and hour, and waits for polish.

---

## 10. Visitor

- **In the visit: seen, not entered.** No port leads out (section 2). The Grounds is resident whenever the
  current stage has an exterior opening (Site's residency).
- **Where it is seen from:**
  - **the Hall of Light's glass**: south over Albion's lawn, the walk east, the rond-point and the terrace;
    north over Chenghuai's walls to the groves;
  - **Chenghuai's open courts and porch**: tree crowns over the walls, and the groves north of the Salon;
  - **Albion's lancets and glass roofs**: the groves' tops and the sky;
  - **the Rotunda's eye and the Salon's lanterns**: the sky only.
- The front (parvis, canal, parterre) is not in view from any walked room. It is seen on the stand and in the
  owner's photographs.
- **On the stand.** The body walks the paving, gravel and lawn (Walkable faces from the body's slope). Footfalls
  sound. There are no seats and no uses.
- **Exposure** is the one Eye's (D-014). The old exterior exposure volume (a local compensation curve for
  sunlit lawn) is retired. If a sunlit lawn clips, that is the Eye's tone shoulder, not a Grounds matter.

---

## 11. The old code map

All paths are under `C:\Users\anan9\Desktop\Projects\Musee Vision\windows\` (read-only). Re-express the
numbers through the systems; never copy the code.

**The Grounds' own numbers**
- `Source/MuseeVision/Plan/MuseePlan.h:563-587`: the grounds' layout (axis, parvis, canal, copings, panels,
  walks, cour, avenue, walk east, rond-point, terrace).
- `Source/MuseeVision/Plan/MuseePlan.h:518-561`: the faces the grounds meet (Salon faces, podium 2.0, portico
  steps end 17.3 at 555-557, oval 524).
- `Source/MuseeVision/Plan/MuseePlan.h:165-168, 214`: the Élan's centre (54, 0). Lines 225-229: the socle
  r 15.30. Lines 251-255: the portal.
- `Source/MuseeVision/Plan/MuseePlan.h:376-392`: the Reserve (its top at −0.33 under the lawn, 382-384).
- `Source/MuseeVision/Facade/MuseeLandscape.cpp`:
  - 28-36: levels (gravel +0.018, paving +0.024, slab bottom −0.12, panel y 27 … 55);
  - 43-54: lamp positions;
  - 56-80: the lamp standard's post, globe and cap profiles;
  - 82-101: slab and annulus helpers (not re-expressed);
  - 103-116: the kerb profile;
  - 118-138: the coping profile and basin;
  - 140-311: hedges (the module placement 164-261; BuildHedges 282-311);
  - 314-364: forecourt (parvis, cour, canal, kerbs, avenue);
  - 366-419: walk east with the Albion bend (378-386), rond-point, path, terrace;
  - 421-431: BuildAll;
  - 579-608: lamp light (candela, kelvin, dusk dimming), re-expressed as a catalogue luminaire and Light's
    Operation;
  - 642-669: material slots, re-expressed as Matter specs (3.10);
  - 757-868: tree placements (seeds 700 onward are replaced by keys).
- `Source/MuseeVision/Facade/MuseeLandscape.h:56-82`: the grounds described.
- `Source/MuseeVision/Nature/MuseeHedge.h:6-33, 30-93`: the clipped hedge model and module kinds.
  `MuseeHedge.cpp:28-41`: the hedge spec table. `MuseeHedge.cpp:90-321`: the leaf-shell builder.
- `Source/MuseeVision/Nature/MuseeLawn.h:11-35, 50-74, 110-150`: the lawn model (blades, density, region,
  fringe, margins, stripes). `MuseeLawn.cpp:20-288`: the blade patch builder. `MuseeLawn.cpp:305-320`: the
  exclusions (these become received outlines). `MuseeLawn.cpp:342-352`: the fringe weight.
- `Source/MuseeVision/Nature/MuseeTree.h:9-24` and `MuseeTree.cpp`:
  - 127-148: birch;
  - 202-221: osmanthus;
  - 231-350: seasonal look (osmanthus flowers 335-347);
  - 380-415: the keep box (becomes the growth space);
  - 583-800: growth.
- `Source/MuseeVision/Nature/NatureTypes.h:14-26` (species), `NatureMesh.h` (wind per vertex): reference for
  Nature.
- `Source/MuseeVision/Ground/MuseeGround.h:10-58`, `MuseeGround.cpp:40-90`: the 800 m plane at −0.03 and its
  holes (the holes become outline cuts).
- `Scripts/nature.py:1175-1200`: `place_grounds`.
- `Scripts/exterior_materials.py`:
  - 1-30: the outdoor weathering model (streaks, damp foot, lichen, water line, lanes);
  - 347: paving;
  - 350-352: kerbs;
  - 353-355: basin bed;
  - 356-361: gravel;
  - 368-374: lamp metal.

**Handed to other stages (section 3.11, 3.12)**
- `Source/MuseeVision/Facade/MuseeFacadeStructure.cpp`:
  - 177-231: podium and gravel strip (Salon and Rotunda);
  - 233-328: Salon dress;
  - 330-430: portico;
  - 432-478: cabinet;
  - 480-541: oval and hyphen roof;
  - 600-671: drum dress (Rotunda);
  - 673-700: passage skins (Site's joints);
  - 702-745: dome cover (Rotunda);
  - 747-809: façade floodlights.
- `Source/MuseeVision/Facade/FacadeOrder.h:6-116` and `FacadeOrder.cpp`:
  - 10-26: podium profile;
  - 28-78: cornice and entablature;
  - 80-97: dentils;
  - 98-260: Attic base, pilaster, column;
  - 261-446: windows and door;
  - 448-521: balustrade and urn;
  - 522-...: medallion;
  - 655-680: inscription.
- `Source/MuseeVision/Facade/MuseeLandscape.cpp:307-310`: the Hall of Light's yew hedges.

**Retired (not re-expressed)**
- `MuseeFacadeStructure.cpp:543-598`: Sculpture Hall dress.
- `MuseeLandscape.cpp:823-859`: bamboo.
- `MuseeLandscape.cpp:542-577` and `MuseeLandscape.h:148-162`: exterior exposure volume. The Eye owns exposure.
- Lighting channels, bake tags and material fallback chains: `MuseeLandscape.cpp:436-504, 610-640, 642-669`.
- `Facade/FacadePreview.cpp`: the preview command, replaced by Studio's stand.
- `Facade/FacadeKit.h`: geometry helpers, replaced by Form's kernel.

---

## 12. First-pass cut

**Built tonight**
1. **Site.** The Ground stage is placed at the datum. The `grounds.open` Outdoors enclosure. The received
   outlines and underground tops.
2. **Ground.** The flat terrain at −0.03 over ±400 m, cut by the outlines. The `lawn` cover as a cover material
   only (no blades). The `gravel` cover over the walk regions in 3.4.
3. **Hard landscape** (Form):
   - the parvis's 17 courses of slabs;
   - every kerb in 3.5;
   - the canal's and the round basin's copings and floors.
4. **Water** (Nature). The two still bodies at z 0.30.
5. **Lamps.** Ten bronze standards with the post-top opal-globe luminaire, dusk switching, and the 5 lx
   exterior-walkway target on the canal walks.
6. **Trees.** All 37, grown at the current solar term with runtime phenology.

**Waits for polish**
- The clipped hedges (box loops, balls, yew exedra). The front is not seen from any walked room, so they are
  data in this sheet until Nature's clipped hedge exists. If Nature builds that capability tonight for the
  Hall of Light's hedges, the Grounds' hedges are only data and can go in at once.
- Lawn blades (12,000/m²), mowing stripes and the thinning fringe.
- The finer outdoor weathering: streaks under ledges, lichen, the copings' water line and the trodden lanes in
  the gravel.
- Rain rings, ice on the basins, wind in the foliage and birdsong.
- The façade dress and floodlights (the Salon's and the Rotunda's cut), and the two parvis uplighter recesses.

---

## Open points for the integrator

1. **Placement without a port.** Mate uses ports alone (Architecture.md). A Ground stage with no mating needs a
   Site rule and a decision record: it stands at the anchor's datum.
2. **Reachability.** The Grounds' walkable faces are unreachable by design. Site.Reachable must judge the visit
   graph and not fail the port-less Ground stage.
   - If the owner wants to walk out onto the parvis, that is a new port, for example opening the portico's
     bronze door into the Salon. It is a reviewed design change, not part of this sheet.
3. **Façade dress.** The Salon's and the Rotunda's sheets must carry their outer dress (3.11). Otherwise the
   exterior is their plain outer faces tonight.
4. **The cabinet and Chenghuai conflict.** The cabinet's north face (y −16.0) stands only 1.0 m from
   Chenghuai's south face (y −17.0), for x −20.7 … −15.6. The cabinet's podium (0.54) fits there; its gravel
   strip (to 1.56) does not. The Salon's and Chenghuai's sheets must resolve it. The Grounds lays only terrain
   in whatever slot remains.
5. **Underground tops.** The Cube's and every other underground top must be at z −0.33 or lower wherever it
   lies under the Grounds. The Reserve's is −0.33 as built, 1 cm under the Grounds' deepest piece.
6. **The Hall of Light's boundary.** Its at-grade outline (x 10.9 … 41.1, \|y\| ≤ 14.25) and its two yew hedges
   must match its own sheet.
