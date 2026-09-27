# The Grounds · design sheet

- **Stage key** `grounds` (module `MuseeGrounds`, `UGroundsRoom : USiteRoom`). Display name "The Grounds". The museum's
  site under its own sky: a French formal front south of the Salon's portico (a stone parvis, a gravel cour round a
  30 m still canal, two box-edged lawn panels, osmanthus allées, a yew exedra, a birch avenue running south), a gravel
  walk east that bends round Albion's south end to a rond-point with a round basin on the Élan's meridian, a path north
  to a gravel terrace round the Élan's socle, lawn up to every wall, groves round the buildings, ten bronze lamp
  standards. Era: outdoors (stone honed, sawn or bush-hammered, then weathered; bronze cast, weathered).
- **Sources:** D-001…D-019 (D-001, D-003, D-005, D-006, D-012, D-017, D-018 binding here; **the Square is retired and
  never built**); Palette.json + Story.md ("The Grounds"); the drafter's spec `r-specs/…/Stages/grounds.md` (verified,
  corrections in §11); the finished sheets `rotunda.md`, `salon.md`, `nympheas.md`, `chenghuai.md`, `albion.md`,
  `hall-of-light.md`, `atrium.md`, `sphere.md`, `cube.md`, `classical-hall.md`, `reserve.md` (their §1 and Build notes:
  footprints and translations, never re-specified here); boards `plan/canvas/{MasterPlan,AsBuilt,MaterialsOther}`,
  `plan/proposals/albion/canvas/Outside` ("the walk bends round Albion's south end"), the approved pond edge
  (`plan/proposals/salon-interior`, the oval's; the Grounds' copings share its sitting-height language); the as-built
  code in `windows/` (§11). Schedules: `Data/Stages/grounds/{Finishes,Lighting,Planting}.json` (no Hang: no works; no
  Mechanisms: nothing moves).
- **Units** metres. **Frame:** the museum plan frame itself: origin under the gilt sun at the Rotunda's centre, z 0 the
  ground-floor finished level; x east, y south, z up, true north. **Plan angle** α: from +x towards +y (east 0°, south
  90°), as in the old code; a compass bearing is α + 90°.

## 1. Identity and placement

| | |
|---|---|
| Origin in the museum plan | (0.000, 0.000, 0.000); yaw 0°. **museum = local.** No port places it (§2): `SitePlan.json` stands it at the anchor's datum (a Site rule, §13 c). |
| Level | 0. Lawn soil top **z −0.030**; gravel walks top +0.018; strip gravel top +0.012; parvis top +0.024; kerb and strip-edge tops +0.045; copings top +0.450; still water +0.300; basin floors top −0.250. |
| Footprint kind | **Ground** (the one such stage). |
| Vertical extent | z −0.330 (the soil's bottom, face to face with the Reserve's top) to ≈ 13.0 (the tallest birch, 12.0, with sway). |

**Footprint polygon:** the 800 m square of the as-built ground (`MuseeGround.h:39`): (−400, −400) (400, −400)
(400, 400) (−400, 400). The Grounds lays nothing inside any received outline (below): its terrain, lawn and gravel are
**cut by rule** at every at-grade outline, so the lawn comes "up to the walls" with no hole drawn by hand.

**Received outlines** (each the other stage's to declare; numbers from the finished sheets, museum frame):

| Stage | At-grade outline the Grounds meets | Under / over |
|---|---|---|
| Rotunda | drum r 11.20 (tonight; its dress, polish, adds a podium to r 11.74) | its well lies inside r 7.60 |
| Joints (Site) | the Salon passage x −13.40 → the drum (2.20; as built \|y\| ≤ 1.80); Albion's wedge \|x\| ≤ 3.65 between the drum and y 11.20 | — |
| Salon | x −74.60 … −13.40, \|y\| ≤ 7.60; Manet cabinet x −24.40 … −15.60, y −16.00 … −7.60 | its floor bears on the Reserve (−0.05) |
| Nymphéas | the oval's outer face (the 0.60 normal offset of the ellipse 11.00 × 7.50 about (−86.50, 0): x −98.10 / −74.90 on the axis, \|y\| ≤ 8.10); passage block \|y\| ≤ 2.10 to x −74.60 | its well prism x −90.80 … −74.60, \|y\| ≤ 1.60 |
| Chenghuai | x −20.70 … 16.90, y −61.90 … −17.00; porch \|x\| ≤ 2.95 from y −17.00 to the drum (R 11.20) | the Classical Hall under its courts |
| Albion | envelope x ±15.60, y 15.60 … 54.80 (ground contact: plinth x ±14.68, y 16.52 … 53.88, porch ±3.68, 18 buttress lobes; Form derives it); porch \|x\| ≤ 3.70 from y 11.20 | footings to −1.20 (buried) |
| Hall of Light | x 10.90 … 41.10, \|y\| ≤ 14.25, closed by arcs on the drum (R 11.20) and the Élan socle (R 15.30 about (54, 0)); its gardens, hedges and trees are its own | soil to −0.30 (its own) |
| Atrium (Élan) | socle r **15.30** about (54, 0) outside the west sector (bearings 248°–292°, inside the Hall of Light's outline); corona r 15.40 at z 5.48–5.86 and entablature r 15.70 at z ≥ 20.90 oversail (volumes, not plans) | the Cube below −0.50 |
| Sphere | none at grade (its dome stands on the Atrium's entablature, z ≥ 22.0) | — |
| Reserve (below) | x −74.60 … −13.40, \|y\| ≤ 12.60: top **−0.330** outside \|y\| 7.55 (under the strips, lawn, parvis and side walks) | the Grounds' soil sits on it |
| Classical Hall (below) | roof **−0.550**; under the Grounds only where 2.95 < \|x\| ≤ 4.20, between the drum and y −17.00 | — |
| Cube (below) | box x 37.80 … 70.20, \|y\| ≤ 16.20, top **−5.800** (under the terrace and lawn round the socle) | — |

| Enclosure | Kind | Bounded by |
|---|---|---|
| `grounds.open` | Outdoors | the footprint minus every received volume; the museum's sky above. No shell: rain and snow fall everywhere in it and stop at the received shells. Weather **museum**. |

### Anchors (the stage contract)

| Key | Kind | Frame | Belongs to |
|---|---|---|---|
| `grounds.canal.water` | Water | (−44.000, 42.000, 0.300), stage axes | the canal (Planting `water`) |
| `grounds.rond-point.water` | Water | (54.000, 42.000, 0.300), stage axes | the round basin |
| `grounds.lamp.01`, `grounds.lamp.02` … `grounds.lamp.10` | Mount | the globe's centre (x, y, 3.620) over each standard (§3.7); facing down | each lamp's luminaire |
| `grounds.lawn` | Bed | the terrain's top, z −0.030, stage axes | the lawn |
| `grounds.hedge.panel.west`, `grounds.hedge.panel.east` | Bed | the box loops' centre lines (§3.8), z −0.030 | box |
| `grounds.hedge.ball.01` … `grounds.hedge.ball.08` (keys listed in §7) | Bed | each ball's plan centre, z −0.030 | box |
| `grounds.exedra.east`, `grounds.exedra.west` | Bed | arcs r 14.00 about (−44.00, 62.00), z −0.030 | yew |
| trees (37 keys, §7.1) | Bed | each root flare's plan point, z −0.030 (on lawn) or +0.018 (in gravel) | birch, osmanthus |
| `grounds.canal.coping.seat` | Seat | frames every 0.80 along the canal coping's seat line (d 0.25 in from its outer face, z 0.450), facing out or in; none within 0.60 of a corner or a lamp's base | the coping |
| `grounds.rond-point.coping.seat` | Seat | frames every 0.80 on r 2.75 about (54, 42), z 0.450, facing out or in | the coping |

No Body, Use, Wall or Glazing anchors: nothing moves, no works hang, no glass.

## 2. Ports

**None.** The Grounds has no doorway, stair, well or vertical port; it serves no well and receives none.
- **D-012 is exhaustive.** The museum's ways down (the Rotunda's stair, the Élan car's shaft, the oval's stair) all lie
  inside Building outlines. The panel's old `rotunda.entrance ↔ grounds.gate` is retired.
- **The portico's door is blind** (as built `Door.bBacked = false`, `bGlazed = false`): a closed cast-bronze door
  (`bronze.cusn10`, patinated, weathered, per the palette and the Story) dressed on the Salon's south face, 3.00 wide,
  z 2.00 → 7.80 on the axis x −44.00. It opens into nothing, so it is **not a port**. It belongs to the Salon's exterior
  dress (polish, `salon.md` §12); the Grounds builds none of it.
- **How the Grounds meets each building** (no joint, no doorcase, no bronze strip; faces meet at outlines, volumes
  never overlap):

| Building | What the Grounds lays against it | Clearance |
|---|---|---|
| **Stone complex** (Rotunda drum on its free arcs, Salon, Manet cabinet, Nymphéas oval and passage block) | **The strip** `grounds.gravel` + `grounds.strip.edge`: gravel from the received outline to d 1.44, a stone edge d 1.44 → 1.56 off the wall face (§3.5). Where two faces' strips meet they merge into one band with one edge. | 0 (gravel meets the outline); the edge is fixed at d 1.56 (Rotunda r 12.76, Salon y ±9.16) |
| Cabinet ↔ Chenghuai | the 1.00 slot x −20.70 … −15.60, y −17.00 … −16.00: strip gravel wall to wall, no edge | 0 both sides |
| Chenghuai | lawn up to its walls and porch walls | 0 |
| Albion | lawn up to its ground contact; the walk east's kerbs stand 1.84 west and east of the envelope (x ∓17.44) and 3.14 south of it (y 57.94) | ≥ 1.84 |
| Hall of Light | lawn up to its outline; the Rotunda's strip stops at its west edge x 10.90 | 0 |
| Atrium (Élan) | the terrace from r 15.32 (bearings 325° → 90° → 215°); lawn to the socle between the terrace's ends and the Hall of Light | 0.02 off the socle |
| Reserve, Classical Hall, Cube (below) | the soil's bottom −0.33 lies on the Reserve's top; 0.22 over the Classical Hall's roof; 5.47 over the Cube | 0 / 0.22 / 5.47 |

- **Where the visitor can walk.** In the visit, nowhere: no port leads out (seen, not entered, §10). On Studio's stand,
  every paved, gravel and lawn face is walkable; the copings are seats; water and hedges are not walked.

## 3. Pieces by trade

### 3.1 Setting-out (museum frame)

| Line | Value |
|---|---|
| The front's axis | x **−44.00** (the portico's axis, the Salon's mid-length) |
| Parvis | x −60.00 … −28.00 (axis ± 16), y 9.16 (the Salon strip's edge) … 23.00; the portico's first nosing (Salon dress) falls at y 17.30 |
| Side walks | x −64.00 … −60.00 and −28.00 … −24.00, y 9.16 … 23.00 |
| Cour | x −64.00 … −24.00, y 23.00 … 62.00 |
| Canal (coping outer faces) | x −48.50 … −39.50, y 27.00 … 57.00 |
| Lawn panels (kerb centre lines) | \|x + 44\| 7.50 … 14.00, y 27.00 … 55.00 |
| Cross walk | y 57.00 … 62.00 (55.00 … 62.00 beside the panels) |
| Avenue | x −48.00 … −40.00, y 62.00 … 130.00 |
| Walk east | 4.00 wide: y 40 … 44 from x −24.00; bends round Albion on x ∓19.50 and y 60.00 (centre lines) to the rond-point |
| Rond-point | centre (54.00, 42.00): basin outer face r 3.00, ring r 3.00 … 7.00 |
| Path north | x 52.00 … 56.00, from the terrace's outer arc (y 19.297) to the ring's arc (y 35.292) |
| Élan terrace | annulus about (54.00, 0) r 15.32 … 19.40, α −125° → +125° through east |

### 3.2 Terrain `grounds.terrain` (Form solid; Nature's lawn on its top)
A flat solid of soil over the whole footprint, **z −0.330 … −0.030** (0.30 of topsoil, as the Reserve's sheet expects),
made as one Nanite mesh (no Landscape). It is the footprint minus every received Building volume and minus every
Grounds piece set into it (paving, gravel, kerbs, edges, copings, basin floors), so each hard piece sits in its own
pocket and nothing overlaps. Over the Reserve it lies face to face on the top at −0.33. Its top takes the lawn
(`grounds.lawn`, §7.3).

### 3.3 The parvis `grounds.parvis.paving` (Form laid surface)
- **Region** x −60.00 … −28.00, y 9.16 … 23.00, less the kerbs (§3.5) and any received outline (the portico's steps and
  cheeks, x −56.80 … −31.20 to y 17.30, cut it when the Salon's dress lands). Tonight 442.9 m² less the kerbs.
- **Setting-out** (a mason's division of the as-built rectangle): 17 courses east–west, each **0.8141** deep from y 9.16
  (the joint at y 17.30 falls on the portico's first nosing); slabs **1.1852** long, 27 to a full course, the middle slab
  centred on x −44.00; alternate courses offset half a slab, cut at the ends; joints 6 mm; slab section **z −0.120 →
  +0.024** (as built). Flat as built: puddles come from each slab's laying tolerance (Form's ponding field).

### 3.4 Gravel `grounds.gravel`, `grounds.gravel.canal-walks` (Form laid surface, top +0.018, bottom −0.120)
One union of plan regions per key; regions meet without overlaps or staggered tops. Kerbs, edges, copings and received
outlines are cut out of them.

| Region | Plan |
|---|---|
| Side walks | x −64 … −60 and −28 … −24, y 9.16 … 23.00 |
| Cour | x −64 … −24, y 23 … 62, minus the canal (x −48.5 … −39.5, y 27 … 57) and the panels (x −58 … −51.5 and −36.5 … −30, y 27 … 55) |
| **Canal walks** (`grounds.gravel.canal-walks`, part of the cour) | \|x + 44\| 4.50 … 7.50, y 27 … 57 |
| Avenue | x −48 … −40, y 62 … 130; its south end meets lawn with no kerb (as built) |
| Walk east | (1) x −24 … −17.5, y 40 … 44; (2) x −21.5 … −17.5, y 44 … 62; (3) x −17.5 … 17.5, y 58 … 62; (4) x 17.5 … 21.5, y 44 … 62; (5) x 17.5 → the ring's arc (x 47.292 at y 40 and 44), y 40 … 44 |
| Rond-point ring | annulus about (54, 42), r 3.00 … 7.00 |
| Path north | x 52 … 56, y 19.297 … 35.292 |
| Terrace | annulus about (54, 0), r 15.32 … 19.40, α −125° → +125° through 0° (309 m²) |
| **The strip** round the stone complex | from the received outline to d 1.44 off the wall face, top **+0.012**, bottom −0.100 (§3.5) |

### 3.5 Kerbs `grounds.kerb` and the strip's edge `grounds.strip.edge` (Form sweeps, closed profiles)
**Kerb profile** (A across, centred on the line; B up): `(-0.060,-0.160) (0.060,-0.160) (0.060,0.030) (0.045,0.045)
(-0.045,0.045) (-0.060,0.030)`: 0.12 wide, top +0.045, 15 mm chamfers, 0.205 deep. Stones 1.20 long.

| Kerb | Centre line (open unless closed) |
|---|---|
| panel.west (closed) | (−51.5, 27) → (−58, 27) → (−58, 55) → (−51.5, 55) |
| panel.east (closed) | (−36.5, 27) → (−30, 27) → (−30, 55) → (−36.5, 55) |
| parvis.west, parvis.east | x −60 and x −28, y 9.16 → 23.00 |
| parvis.south | y 23.00, x −64 → −24 |
| cour.west | (−64, 9.16) → (−64, 62) → (−48, 62) |
| cour.east-south | (−40, 62) → (−24, 62) → (−24, 44) |
| cour.east-north | (−24, 40) → (−24, 9.16) |
| avenue.west, avenue.east | x −48 and x −40, y 62 → 130 |
| walk-east.inner | (−24, 40) → (−17.5, 40) → (−17.5, 58) → (17.5, 58) → (17.5, 40) → (47.292, 40) |
| walk-east.outer | (−24, 44) → (−21.5, 44) → (−21.5, 62) → (21.5, 62) → (21.5, 44) → (47.292, 44) |
| rond-point | arcs r 7.00 about (54, 42): α 196.60° → 253.40°, and 286.60° → 523.40° (gaps ±16.60° about 180° and 270°) |
| path-north.west, .east | x 52 and x 56, y 19.297 → 35.292 |
| terrace.outer | arcs r 19.40 about (54, 0): α −125° → 84.08°, and 95.92° → 125° (gap ±5.92° about 90°) |
| terrace.ends | radial at α ±125°, r 15.37 → 19.40 |

Each kerb end meets the next exactly (no overlapping stones); kerb ends at y 9.16 butt the strip's edge.

**The strip** (as built the podium's foot, `MuseeFacadeStructure.cpp:217-230`, now the Grounds' ground piece): along
every face of the stone complex, offset d from the **wall face**; gravel (§3.4) from the received outline to d 1.44; the
edge `grounds.strip.edge` profile (d, z) `(1.44,-0.14) (1.56,-0.14) (1.56,0.035) (1.55,0.045) (1.45,0.045) (1.44,0.035)`.
The bands of all faces are one region (their union, minus received outlines); the edge runs once along its outer
boundary: r 12.76 on the drum's free arcs, y ±9.16 along the Salon, round the cabinet and the oval. Where the band
would run into another building (the cabinet–Chenghuai slot) the gravel fills to that outline and there is no edge.
When the Salon's and Rotunda's podia land (polish), their outlines grow to d 0.54 and cut the gravel back; the edge
stays.

### 3.6 Basins `grounds.coping`, `grounds.basin.bed` (Form; Nature fills them)
**Coping profile** (A out from the nose line, away from the water; B up), closed, 0.50 wide, top at sitting height
(the approved pond edge's language):
`(0.040,-0.310) (0.500,-0.310) (0.500,0.400)`, arc centre (0.450, 0.400) r 0.050, 0° → 90° to `(0.450,0.450)`,
`(0.030,0.450)`, arc centre (0.030, 0.420) r 0.030, 90° → 180° to `(0.000,0.420)`, `(0.000,0.360) (0.040,0.360)`.
The nose overhangs the **wetted face (A 0.04)** by 40 mm; its underside at 0.36 throws a shadow line 60 mm over the
water. The water meets the wetted face. Floor: a bluestone slab `grounds.basin.bed` inside the coping ring,
z −0.310 → −0.250; water depth **0.55**.
- **Canal:** coping outer faces x −48.50 … −39.50, y 27.00 … 57.00; nose line x −48.00 … −40.00, y 27.50 … 56.50;
  stones 1.25 long, mitred corners. Water to x −48.04 … −39.96, y 27.46 … 56.54.
- **Rond-point basin:** centre (54, 42); nose r 2.50, wetted face r 2.54, outer face r 3.00; 12 radially jointed stones.

### 3.7 Lamp standards `grounds.lamp.post`, `grounds.lamp.globe` (Form lathe; Light's luminaire inside)
**Post** (r, z from the foot; * = smooth vertex), lathed with 20 sides:
`(0,0) (0.27,0) (0.27,0.07) (0.23,0.11) (0.21,0.40)* (0.17,0.46)* (0.12,0.52)* (0.10,0.60) (0.085,0.70) (0.062,3.05)
(0.095,3.10) (0.095,3.17) (0.06,3.21) (0.07,3.30)* (0.15,3.38) (0.16,3.41) (0,3.41)`: a moulded base Ø 0.54, a shaft
tapering Ø 0.17 → 0.124, a collar and a cup (flat top 3.41). **Globe:** sphere r 0.24 about z 3.62, truncated at 3.41
(lower opening r 0.116, seated in the cup), glass 3 mm. **Finial**, 16 sides, seated on the globe's crown:
`(0,3.860) (0.09,3.842) (0.06,3.910) (0.025,3.940) (0.035,3.980) (0,4.010)`. Overall 4.01. Foot at z 0.000, bedded
18 mm into the gravel or paving.

| Key | x | y | Where |
|---|---|---|---|
| `grounds.lamp.01`, `grounds.lamp.02` | −48.95, −39.05 | 29.0 | canal walks (axis ± 4.95: the base 0.18 off the coping's face) |
| `grounds.lamp.03`, `grounds.lamp.04` | −48.95, −39.05 | 37.0 | canal walks |
| `grounds.lamp.05`, `grounds.lamp.06` | −48.95, −39.05 | 45.0 | canal walks |
| `grounds.lamp.07`, `grounds.lamp.08` | −48.95, −39.05 | 53.0 | canal walks |
| `grounds.lamp.09`, `grounds.lamp.10` | −58.50, −29.50 | 21.5 | the parvis's south corners |

### 3.8 Hedges (Nature's clipped form; data in Planting.json)
A leaf shell grown inside a clipped section (battered both sides, shoulders softened) over a dark core of bare twigs
set Inset inside it; the core carries the collision. The clipped top wanders ±1.2 % over ≈ 5.5 m as a function of
position, so joints never show.

| Kind | Species | Foot width × height | Batter | Shoulder r | Core inset | Leaves / m² |
|---|---|---|---|---|---|---|
| Box loop | *Buxus sempervirens* | 0.42 × 0.52 | 0.015 | 0.045 | 0.055 | 32,000 |
| Box ball | *Buxus sempervirens* | Ø 0.92, sunk 0.04 | — | — | 0.055 | 32,000 |
| Yew exedra | *Taxus baccata* | 0.90 × 1.70 | 0.04 | 0.08 | 0.075 | 58,000 |

- **Box loops** 0.45 inside each panel's kerb: west (−51.95, 27.45) → (−57.55, 27.45) → (−57.55, 54.55) →
  (−51.95, 54.55); east (−36.05, 27.45) → (−30.45, 27.45) → (−30.45, 54.55) → (−36.05, 54.55); 65.4 m each, closed.
- **Box balls**, four to a panel, 0.9 in from the loop's corner lines: x −52.85, −56.65, −35.15, −31.35 at y 28.35 and
  53.65.
- **Exedra:** yew arcs r 14.00 about (−44, 62), α 0.41° → 69.08° and 110.92° → 179.59° (16.78 m each), open ±20.92°
  about south (a 10.0 chord) for the avenue; the ends stand on y 62.10, 0.04 clear of the cross walk's kerb.

### 3.9 Walls, vaults, stairs, glazing, joinery
None of the Grounds' own.

### 3.10 What the Grounds does not build
- **The façade and portico dress** (podium, the orders, blind windows, balustrades, the portico with its steps,
  pediment, inscription MVSÉE · VISION and blind bronze door; the drum's dress and the dome's lead): the Salon's and the
  Rotunda's (`salon.md` §12, `rotunda.md` §3.10), polish. The palette's Grounds roles "façade, portico" (travertine,
  honed, weathered) and "the portico's blind door" (CuSn10, patinated, weathered) pass to that dress with it. The oval's
  dress is `nympheas`'s; the passages' skins are Site's joints'.
- **The Hall of Light's gardens** (meadow, orchard, their birches and fruit trees, the two yew hedges on y ±14.00): its
  own. **The Élan's socle, dress and dome**: the Atrium's and the Sphere's. **The pond edge**: the oval's.

## 4. Light

**Daylight.** Outdoors, no openings: the museum's one sun, moon and sky light the Grounds directly
(`Lighting.json` daylight is empty). Shadows fall from the received volumes, the trees and the hedges.

**Luminaires** (Lighting.json; all `dusk`: on at dusk and off at dawn by the museum's sun, all night, D-006):

| Id | Type | Count | Mount / point | Aim |
|---|---|---|---|---|
| `lamp.01` … `lamp.10` | `post-top-globe.2700k` | 10 | Mount `grounds.lamp.01` … `.10` (the globe's centre, z 3.62) | straight down to the foot |
| `step.side.w`, `.e` | `step-light.2700k` | 2 | kerbs x −64 / −24 (inner face), y 10.20, z 0.032 | the side walk's centre line |
| `step.cour.w`, `.e` | `step-light.2700k` | 2 | kerbs x −64 / −24, y 60.00 | the allée–cross-walk corner (x −61 / −27, y 59.50) |
| `step.avenue.01` … `.16` | `step-light.2700k` | 16 | y 68.5 + 4k, alternately in the west (x −47.94) and east (x −40.06) kerb | the axis x −44 |
| `step.walk-east.01` … `.29` | `step-light.2700k` | 29 | per leg, evenly at ≤ 4.00, alternate kerbs (inner face 1.94 off the centre line) | the centre line |
| `step.ring.1` … `.8` | `step-light.2700k` | 8 | in the basin coping's outer face r 3.00, z 0.15, α 22.5° + 45k | the ring's centre line r 5.00 |
| `step.path.1` … `.4` | `step-light.2700k` | 4 | y 33.29, 29.29, 25.29, 21.29, alternately kerbs x 52.06 / 55.94 | x 54.00 |
| `step.terrace.01` … `.20` | `step-light.2700k` | 20 | the outer kerb's inner face r 19.34, α −118.69° … 119.53° (two flank the path's gap at 83.5° and 96.5°) | the centre line r 17.36 |

Step lights have hidden sources: a louvred slot 0.20 long × 0.02 high in the kerb's inner face just over the gravel
(or in the coping's face), grazing the walk (the ruled pattern from Chenghuai). Exact points are in Lighting.json.

**Night design** (estimates by distance, which Light measures; never settings). Each globe, taken at the as-built
lamp's output, holds 5 lx within ≈ 4.8 in plan of its post and 1 lx within ≈ 9.6; each step light holds 1 lx on the
centre line within ±2.0 along it.

| Walk (centre line) | Met by | Estimate |
|---|---|---|
| Canal walks x −50 / −38, y 27 … 57 | lamps 01–08 (8 m apart) | ≥ 8 lx: **exterior-walkway** met |
| Parvis axis (y 17.30 → 23) and cross line y 21.50 | lamps 01, 02, 09, 10 | ≥ 1.9 lx |
| Cour cross line y 25; allées x −61 / −27, y 23 … 57.8 | the lamps | ≥ 1 lx |
| Side walks, y 9.16 … 11.1 (their dead ends at the strip) | — | below → **2 step lights** |
| Allées y 57.8 … 62 and the cross walk's ends | — | below → **2 step lights** |
| Avenue y 66.5 … 130 | lamps to y 64.5 | below → **16** |
| Walk east (107 m to the ring) | lamps for its first 1.3 | below → **29** |
| Rond-point ring (31.4 m) | — | below → **8** |
| Path north (16 m) | — | below → **4** |
| Terrace (75.7 m) | — (the Élan's etched drum is not counted) | below → **20** |

**Targets** (Lighting.json):

| Surface | Class | lux | Source |
|---|---|---|---|
| `grounds.gravel.canal-walks` (after dusk) | exterior-walkway | ≥ 5 | UseTargets |
| `grounds.gravel` (every walk's centre line, after dusk) | garden.night | ≥ 1 | UseTargets |
| `grounds.parvis.paving` (axis and cross line, after dusk) | garden.night | ≥ 1 | UseTargets |

No care classes (no works). `escape-route` does not apply: no route of egress crosses the Grounds (no port, §2). No
design ratios are declared.

## 5. Time and weather
- **Weather:** the **museum** observer's everywhere (rain and snow history, wind, temperature). Giverny's and London's
  weather fall only on the Salon's and Albion's glass (D-005). Sun, moon, stars and solar terms are the museum's
  (`Data/Time/Museum.json`, idealised by default, D-003).
- **Seasons** (Nature's phenology by solar term): birches leaf out in spring, turn yellow in autumn, stand bare in
  winter; osmanthus evergreen, paler new growth Chunfen → Guyu, cream-orange flowers in the leaf axils Bailu → Hanlu;
  box and yew evergreen (box paler in spring); the lawn's colour follows the season.
- **On surfaces** (Matter's surface law: MPC_Moment × the sky each face sees × porosity × Form's ponding): the wet
  parvis darkens and puddles in its low spots; the gravel darkens; snow settles on lawn, hedges, copings, kerbs; the
  copings' water line stays wet.

## 6. Motion
None. The water is still (no fountain, no flow); plants sway by Nature's wind response, not a mechanism.

## 7. Nature

### 7.1 Trees (Planting.json beds; grown in each tree's growth space: Form's free space at its spot, shrunk by its sway at the design wind; seeds from the keys)

| Keys | Species | Positions (x, y) | Height | Crown centre | Crown radii (h, v) | Leaf density |
|---|---|---|---|---|---|---|
| `grounds.allee.w.01`, `grounds.allee.w.02`, `grounds.allee.w.03`, `grounds.allee.w.04`, `grounds.allee.w.05`, `grounds.allee.w.06` | *Osmanthus fragrans* | x −62.0; y 26, 32, 38, 44, 50, 56 (in the gravel) | 6.4 | 4.3 | 2.1, 1.9 | 1.4 |
| `grounds.allee.e.01`, `grounds.allee.e.02`, `grounds.allee.e.03`, `grounds.allee.e.04`, `grounds.allee.e.05`, `grounds.allee.e.06` | *Osmanthus fragrans* | x −26.0; y 26 … 56 | 6.4 | 4.3 | 2.1, 1.9 | 1.4 |
| `grounds.avenue.w.01`, `grounds.avenue.w.02`, `grounds.avenue.w.03`, `grounds.avenue.w.04` | *Betula pendula* | x −50.5; y 80, 92, 104, 116 | 11.5 | 7.6 | 2.3, 3.2 | 0.8 |
| `grounds.avenue.e.01`, `grounds.avenue.e.02`, `grounds.avenue.e.03`, `grounds.avenue.e.04` | *Betula pendula* | x −37.5; y 80 … 116 | 11.5 | 7.6 | 2.3, 3.2 | 0.8 |
| `grounds.walk.s.01`, `grounds.walk.s.02`, `grounds.walk.s.03` | *Betula pendula* | (−12, 63.8), (3, 63.8), (18, 63.8) | 10.5 | 7.0 | 2.2, 3.0 | 0.8 |
| `grounds.walk.n.04`, `grounds.walk.s.04` | *Betula pendula* | (33, 38.2), (33, 45.8) | 10.5 | 7.0 | 2.2, 3.0 | 0.8 |
| `grounds.grove.01`, `grounds.grove.02` | *Betula pendula* | (−40, −44), (−33, −49): north of the Salon | 12.0 | 8.0 | 2.6, 3.4 | 0.8 |
| `grounds.grove.03` | *Osmanthus fragrans* | (−47, −51) | 7.0 | 4.4 | 2.4, 2.1 | 1.4 |
| `grounds.grove.04`, `grounds.grove.05` | *Betula pendula* | (−112, −8), (−115, 7): beyond the oval | 12.0 | 8.0 | 2.6, 3.4 | 0.8 |
| `grounds.grove.06` | *Betula pendula* | (−92, 44) | 12.0 | 8.0 | 2.6, 3.4 | 0.8 |
| `grounds.grove.07` | *Osmanthus fragrans* | (−98, 56) | 7.0 | 4.4 | 2.4, 2.1 | 1.4 |
| `grounds.grove.08` | *Betula pendula* | (88, −18): east of the Élan | 12.0 | 8.0 | 2.6, 3.4 | 0.8 |
| `grounds.grove.09` | *Osmanthus fragrans* | (92, 10) | 7.0 | 4.4 | 2.4, 2.1 | 1.4 |
| `grounds.grove.10` | *Betula pendula* | (30, 66) | 12.0 | 8.0 | 2.6, 3.4 | 0.8 |
| `grounds.grove.11`, `grounds.grove.12` | *Osmanthus fragrans* | (−20.5, 18), (−20.5, 30.5): 4.9 from Albion's envelope | 7.0 | 4.4 | 2.4, 2.1 | 1.4 |

**20 birches, 17 osmanthus: 37 trees.** None stands inside a received outline; the growth space keeps crowns off walls.

### 7.2 Hedges
§3.8: box loops `grounds.hedge.panel.west`, `grounds.hedge.panel.east` (218 plants each at 0.30); box balls
`grounds.hedge.ball.01`, `grounds.hedge.ball.02`, `grounds.hedge.ball.03`, `grounds.hedge.ball.04`,
`grounds.hedge.ball.05`, `grounds.hedge.ball.06`, `grounds.hedge.ball.07`, `grounds.hedge.ball.08`; yew
`grounds.exedra.east`, `grounds.exedra.west` (34 plants each at 0.50).
Planting.json writes them as Nature reads them (K11): a loop is a `line` through its four corners with `"closed": true`,
its runs mitred at the corners; the exedra an `arc` (`centre`, `r`, `from_deg`, `to_deg`, degrees from +x towards +y),
set out as one chord to each plant; a ball a `point` with `clipped.ball` (the diameter) and `clipped.sink`; and each
kind's `clipped.leavesPerM2` (the table's leaves to the square metre, as the shears keep them) wins over its species'.

### 7.3 Lawn
`grounds.lawn`: mown fine sward (*Festuca rubra*, Chewings and strong creeping, with *Agrostis capillaris*) on the
terrain's top (`accent.site-geology`, topsoil). As built: blades 3–6 cm, ≈ 12,000/m², blunt mown tips, 2 m stripes
north–south; full sward over x −135 … 125, y −75 … 145, thinning over a 30 m fringe to 0.45 of full height; 0.025 clear
of anything standing on it, 0.06 of hedge cores. Planting.json gives it `"mown": 0.045` (the middle of the as-built
blades) and `"on": "terrain"`: Nature lays a mown sward as its carpet at that height over the top of the terrain piece,
as it lays a moss, since 12,000 blades to the square metre over the terrain can't be grown one by one. The terrain's top
is cut round every received building and every Grounds piece set into it (§3.2), so the sward (top +0.015) stops at the
faces of the strip (+0.012), the walks (+0.018), the parvis, the kerbs, the edges and the copings and never lies over
them; the as-built 0.025 clearance was its single blades', which a carpet has none of.

### 7.4 Water (Planting.json `water`; `water.still`, still, clear over bluestone)

| Key | Anchor | Outline | Level | Depth | Freeboard | Area, volume |
|---|---|---|---|---|---|---|
| `grounds.canal` | `grounds.canal.water` | x −48.04 … −39.96, y 27.46 … 56.54 | 0.30 | 0.55 | 0.15 | 234.97 m², 129.2 m³ |
| `grounds.rond-point.basin` | `grounds.rond-point.water` | circle r 2.54 | 0.30 | 0.55 | 0.15 | 20.27 m², 11.1 m³ |

Wind ripples and rain rings; the canal holds the portico's reflection (when built) and the globes'.

## 8. Art
None: no works, casts, frames or labels. The inscription MVSÉE · VISION is the Salon's portico's.

## 9. Sound
- **Acoustics:** Outdoors, a free field over a hard ground: the direct sound plus the ground's and the received faces'
  reflections (Form's exposed surfaces); no enclosure reverberation.
- **Weather** from the museum Moment, by substance: rain on travertine paving and copings, on gravel, on open water, on
  leaves (birch, osmanthus, box, yew) and on lawn; wind in the foliage (birches rustle, evergreens hiss).
- **Footfalls** by substance (gravel crunch, stone, grass), heard only on the stand. No emitters of its own (no
  fountain, no recorded ambience); birdsong waits for recorded evidence (polish).

## 10. Visitor
- **Seen, not entered.** No port leads out. The Grounds is resident whenever the current stage has an exterior opening.
- **Seen from:** the Hall of Light's glass (the lawn, Albion's side, the walk east, groves; north over Chenghuai's
  walls); Chenghuai's courts and garden (tree crowns, the groves north of the Salon); Albion's glass roofs (grove tops,
  sky); the Rotunda's eye and the Salon's lanterns (sky only). The front (parvis, canal, parterre) is seen from no walked
  room: on the stand and in the owner's photographs.
- **On the stand:** walk the paving, gravel and lawn; sit on the copings (`grounds.canal.coping.seat`,
  `grounds.rond-point.coping.seat`). No uses.
- The old outdoor tone-compensation volume is retired: the one Eye judges the sunlit lawn (D-014).

## 11. The old-code map
Paths relative to `windows\` (read only). Re-express through the systems; never copy.

| Part | Old code |
|---|---|
| Grounds layout (axis, parvis, canal, copings, panels, walks, cour, avenue, walk east, rond-point, terrace) | Source/MuseeVision/Plan/MuseePlan.h:563-587 |
| Faces met (Salon, cabinet, oval, hyphen, drum; portico: steps end 17.30, half 11.6, cheeks 1.2) | Source/MuseeVision/Plan/MuseePlan.h:518-561 |
| Levels (gravel +0.018, paving +0.024, slab bottom −0.12), panels y 27 … 55 | Source/MuseeVision/Facade/MuseeLandscape.cpp:28-36 |
| Lamp positions; post, globe, cap profiles | …/MuseeLandscape.cpp:43-80 |
| Kerb profile; coping profile and basin | …/MuseeLandscape.cpp:103-138 |
| Hedges (placement; BuildHedges) | …/MuseeLandscape.cpp:140-311; Nature/MuseeHedge.cpp:28-41 (specs), MuseeHedge.h:70-77 |
| Forecourt (parvis from y 9.16, cour, canal, kerbs, avenue) | …/MuseeLandscape.cpp:314-364 |
| Walk east with the Albion bend (19.5 / 60 / 2.0), rond-point, path, terrace | …/MuseeLandscape.cpp:366-419 |
| Lamp light (2800 K; re-expressed as a gap type, dusk) | …/MuseeLandscape.cpp:579-608; MuseeLandscape.h:165-168 |
| Materials (evidence only; D-018) | …/MuseeLandscape.cpp:492-501, 642-669; Scripts/exterior_materials.py |
| Trees (seeds 700+ replaced by keys) | …/MuseeLandscape.cpp:757-868; Scripts/nature.py:1175-1200 |
| Lawn (region, fringe, margins, stripes; exclusions become received outlines) | Source/MuseeVision/Nature/MuseeLawn.h:112-143; MuseeLawn.cpp:315-320 |
| Ground plane ±400 m at −0.03 (holes become outline cuts) | Source/MuseeVision/Ground/MuseeGround.h:37-47; MuseeGround.cpp:54-90 |
| The strip at the podium's foot (gravel d 0.50 … 1.45, edge 1.44 … 1.56) | Source/MuseeVision/Facade/MuseeFacadeStructure.cpp:179-231 |
| Retired: outdoor tone-compensation volume; bamboo; the Sculpture Hall's dress; lighting channels, bake tags, material fallback chains | …/MuseeLandscape.cpp:474-489, 542-577, 823-859, 610-669; MuseeFacadeStructure.cpp:543-598 |

**Corrections to the draft:**
1. **Canal water** runs to the wetted faces on all four sides: y 27.46 … 56.54 (the draft pulled y in to 27.54 …
   56.46): 8.08 × 29.08 = 234.97 m², **129.2 m³** (draft 128.5).
2. **Chenghuai's porch** is \|x\| ≤ **2.95** (draft 2.5); **Albion's** envelope is x ±15.60, y 15.60 … 54.80 with its
   porch \|x\| ≤ 3.70 (the draft gave the walls); the Nymphéas footprint and the Reserve's top follow their sheets
   (D-019: the oval's stair, not the Salon's).
3. **Terrain** is 0.30 of soil, bottom **−0.33**, lying on the Reserve's top as `reserve.md` expects (draft 0.20 thick,
   0.10 clear).
4. **Parvis slabs** keep the as-built section −0.12 → +0.024 (draft 0.10 thick); the draft's course and slab division
   is kept.
5. **The strip** (gravel and edge to d 1.56) is the Grounds' piece, laid from the received outline, not the stone
   complex's at-grade outline; the cabinet–Chenghuai slot (draft open point 4) is gravel wall to wall.
6. **Lamps:** the bases stand **0.18** off the coping's face (the draft's 0.45 is centre to face).
7. **Materials** are palette keys (D-018): parvis **bush-hammered** (draft honed), basin beds **Belgian bluestone**
   (draft travertine), globes Lamberts cased opal, posts CuSn10 cast; the draft's `substance.working.history` specs retire.
8. **Night:** every walk holds `garden.night` with 81 hidden step lights where the lamps fall short (draft: dark
   beyond the canal); luminaire type `post-top-globe.2700k`.
9. **Hedges are tonight** (Nature's clipped form is built for the Hall of Light's hedges); the draft deferred them.
10. **The façade dress** is already carried by the finished Salon and Rotunda sheets; the draft's §3.11 numbers are not
    repeated. Seat anchors are added on the copings.

## 12. The first-pass cut
**Tonight:** Site (the Ground stage at the datum, `grounds.open`, received outlines and underground tops); the terrain
(−0.33 … −0.03) cut by every outline and piece; the lawn sward; the parvis's 17 courses; all gravel regions and the strip
round the stone complex; every kerb and the strip's edge; both basins (copings, bluestone floors) and their still water;
the ten lamp standards with their globes and luminaires; the 81 step lights; the 37 trees with phenology; the box loops,
balls and yew exedra; rain, snow and footfalls.

**Waits for polish:** mowing stripes and the lawn's fringe; the finer outdoor weathering (streaks, lichen, trodden
lanes, the copings' water line beyond Matter's defaults); rain rings and ice on the basins; birdsong; the façade dress
and its two in-ground uplighter recesses in the parvis at (−51.5, 22.0) and (−36.5, 22.0) (the Salon's and Rotunda's
dress, when it lands).

## 13. Palette keys to confirm and catalogue gaps
**Palette keys** (no `todo.` keys; roles or workings to confirm):
- `accent.site-geology` working **topsoil.bare** for the Grounds' terrain and the lawn's ground (the role names only the
  Cube's shaft; ruled for ground beds).
- `stone.travertine-romano`: **bush-hammered** (parvis), **sawn** (kerbs, strip edge), **honed** (copings),
  **crushed.stabilise** (all gravel, including the strips): the palette's outdoors workings and the Story.
- `accent.pierre-bleue` **honed**, **submerged** (a history); `bronze.cusn10` **cast.bronze** (outdoors); `glass.mouth-blown` **cased-opal**;
  `water.still` **still.clear**.
- Living: `living.festuca-rubra`, `living.betula-pendula`, `living.osmanthus-fragrans`, `living.buxus-sempervirens`,
  `living.taxus-baccata`.

**Catalogue** (Light; the gaps are filled):
- `post-top-globe.2700k`: the lamp inside each Ø 0.48 cased-opal globe: a LEDVANCE HQL LED Filament Value, 5400 lm,
  2700 K, Ra 80, 32 W, frosted ellipsoid, IP65, made for post-top lanterns in parks; all round, dusk-switched
  (`Data/Light/Luminaires/post-top-globe.2700k.json`).
- `step-light.2700k`: FX Luminaire RH, the ruled hidden-source step light (86 lm, 49 cd), set in the kerbs' inner faces
  and the basin coping's face; its faceplate is the maker's (about 0.10 × 0.06), not the 0.20 × 0.02 slot first drawn.

**Conflicts and questions:**
(a) **Scope.** The brief lists "façade, portico" for the Grounds and the palette gives the Grounds those roles; the
finished `salon.md` and `rotunda.md` own their dress (polish). Resolved: not re-specified here; the roles pass with it.
(b) `albion.md` says "the Grounds' lawn falls to the south": the site is level as built (one plane at −0.03); Albion's
footings are simply buried.
(c) **Site:** a Ground stage with no port stands at the anchor's datum; `Site.Reachable` must judge the visit graph and
not fail it. Walking out onto the parvis would need a new port (the portico's door made real): a design change.
(d) `atrium.md` draws its footprint envelope at R 15.40 (the corona at z ≥ 5.48); the terrace starts at r 15.32 below
it: Site must compare volumes.

## Build notes
- **Frame.** The Grounds' frame is the museum frame (translation 0, yaw 0). Every number here is a museum number.
- **Cut by rule, never by hand.** Terrain, lawn and gravel stop at every received outline; kerbs, edges and copings
  are cut out of the gravel and paving they cross; the terrain is cut by every piece set into it. When a building's
  outline grows (the podia, the portico), the cut follows it.
- **The strip is one band.** Union of the d ≤ 1.56 bands off every stone-complex face, minus outlines; one edge on its
  outer boundary (r 12.76 at the drum, y ±9.16 at the Salon).
- **Under and over.** The soil's bottom (−0.33) lies face to face on the Reserve's top; the Classical Hall's roof
  (−0.55) and the Cube (−5.80) are below; the Atrium's corona oversails the terrace at z ≥ 5.48.
- **Water meets the wetted face** (A 0.04), never the nose line; beds are bluestone, water clear.
- **Lamps:** Light makes the lamp inside each globe from the Mount anchor; Form makes the post and globe. Step lights
  are Light's, set in the kerbs' faces at the points in Lighting.json; no visible fitting.
- **Nature owns** the lawn, the trees (point, height, crown), the clipped hedges and the water from Planting.json. Do not
  model plants or hedge cores by hand.
- **Nothing moves; no ports.** Site places the stage at the datum.

**Departures from as-built** (for the owner's review):
- **Materials follow the Story (D-018):** parvis bush-hammered travertine (as built honed), basin beds Belgian
  bluestone (as built a dark stone), gravel the museum's crushed travertine bound as a stabilisé, globes Lamberts cased
  opal, posts cast CuSn10 weathered.
- **Night (ruled):** the ten lamps are a `post-top-globe.2700k` (as built 2800 K point lights); every walk holds
  `garden.night` (≥ 1 lx on the centre line), met by the lamps first and 81 hidden `step-light.2700k` in kerbs and
  the basin's coping where they fall short (as built the walks beyond the canal were dark); `exterior-walkway` (5 lx)
  holds only on the lamp-lit canal walks; `escape-route` does not apply (no egress crosses the Grounds).
- **The strip is the Grounds'** and runs from the outline (as built from d 0.50, under the podium); the cabinet–
  Chenghuai slot is gravel wall to wall (as built the strip ran into Chenghuai's site).
- **Soil 0.30** on the Reserve's top (as built a plane at −0.03 with holes).
- **No overlaps:** water to the wetted face (as built a 4 cm gap under the nose); the exedra's ends 0.04 clear of the kerb
  (as built 0.06 into it); kerbs cut the paving and gravel, one gravel top +0.018 (as built staggered 0.018–0.021 over
  overlapping slabs).
- **Seat anchors** on the copings (sitting height as built; the approved pond edge's language).
- **Retired:** the outdoor tone-compensation volume (the Eye owns it), the bamboo (with the Chinese Wing), the
  Sculpture Hall's dress, the Square (never built).
- **Species, not palette keys (the contract, at the build):** Planting.json's beds named their material as
  `living.<species>` palette keys (`living.festuca-rubra`, `living.betula-pendula`, `living.osmanthus-fragrans`,
  `living.buxus-sempervirens`, `living.taxus-baccata`). Those keys are retired: each bed names its `species`, and Nature's
  species record gives each organ its tissue, worked and aged as the bed's `working` and `history` say. The fifty beds'
  `material` fields are dropped (§13's Living line is superseded); the lawn's `ground` (`accent.site-geology`,
  topsoil) stayed, as it is a soil, not a plant.
- **The lawn lies on the terrain (K11):** the lawn's `ground` and its ±400 outline are dropped. The terrain
  (`grounds.terrain`, §3.2) is the lawn's soil, in the same topsoil, cut round every received building; a second face of
  soil over the whole footprint lay coplanar with its top and ran into every building. The lawn names the terrain
  (`"on": "terrain"`) and Nature lays it, mown to 0.045, over the terrain's top, which the stage cuts round the
  walks, the strip, the parvis, the kerbs, the edges and the copings as §3.2 asks (the terrain is drawn after them, from
  what each covers in plan; `Musee.Grounds.TerrainCut`).
