# The Reserve · design sheet

- **Stage key** `reserve` (module `MuseeReserve`, `UReserveRoom : USiteRoom`). Display name "The Reserve". Era: Classical.
  The museum's picture store: a brick undercroft 60 m long under the Salon, reached only by the Nymphéas oval's stair.
  **Not built tonight (D-019: "the basement isn't built yet")**; this sheet is for the later pass (§12).
- **Sources:** D-001…D-019 (D-006, D-014, D-017, D-018, D-019 binding here); Palette.json + Story.md ("The Reserve");
  the neighbour sheet `nympheas.md` (§2: `nympheas.stair` and the served well); `salon.md` (above); the drafter's spec
  `r-specs/…/Stages/reserve.md` (verified; corrections in §11); the as-built code in `windows/` (§11); the boards
  `plan/canvas/SalonReserve.dc.html`, `AsBuiltReserve.dc.html`. Schedules: `Data/Stages/reserve/{Finishes,Lighting,Hang,Mechanisms}.json`.
- **Units** metres. **Frame:** origin on the axis (y 0) midway between the end walls' inner faces, on the finished
  floor; x east, y south, z up, true north. **museum = local + (−44.000, 0.000, −5.800)**, yaw 0°.

## 1. Identity and placement

| | |
|---|---|
| Origin in the museum plan | (−44.000, 0.000, −5.800); yaw 0°. The west wall's outer face x −30.600 lands at museum x −74.600 (the Salon's far wall plane and `nympheas.stair`'s face); the east wall's outer face x +30.600 at museum −13.400. |
| Level | −1. Floor 5.80 below the Rotunda's and the Salon's floors (MuseePlan.h:378). |
| Footprint kind | Building, wholly underground (Site compares footprints as volumes). |
| Vertical extent | z −0.350 (footings) to 5.750 (box top under the Salon, museum −0.050, where the Salon's floor build-up bears) and 5.470 elsewhere (museum −0.330, under the lawn's 0.30 of soil). |

**Footprint polygon** (plan, clockwise on the north-up plan from the north-west corner): (−30.600, −12.600) (30.600, −12.600)
(30.600, 12.600) (−30.600, 12.600). Top: 5.750 over x −30.550…30.550, |y| ≤ 7.550 (under the Salon's outer faces, 50 mm
in); 5.470 elsewhere, with vertical 0.280 risers at |y| 7.550 and x ±30.550.

Above it: the Salon's five bays (museum x −74.6…−13.4, |y| ≤ 7.6), the lawn either side, the Manet cabinet (local x
19.6…28.4, y −16.0…−7.6, over the north wall) and the portico's stylobate over the south aisle. West of the plane
x −30.600 lies `nympheas`'s well (§2): nothing of the Reserve is west of it.

| Enclosure | Kind | Bounded by |
|---|---|---|
| `reserve.undercroft` | Enclosed | The four walls' inner faces (x ±30.000, y ±12.000), the floor, the vaults. One opening: the port `reserve.stair` (the arched west door) into `nympheas.well`. No daylight. ≈ 7,000 m³. |

## 2. Ports

| Key | Kind | Class | Local frame (threshold centre on the port face; facing out; up +z) | Mates with | Wells |
|---|---|---|---|---|---|
| `reserve.stair` | stair (received well) | `stairwell.straight.2400x3200.arched` | (−30.600, 0.000, 0.000), facing **−x** = museum (−74.600, 0.000, −5.800) | `nympheas.stair` (its frame nympheas (+11.900, 0, −5.800) = museum (−74.600, 0, −5.800), facing +x). Passage 0.00 (faces touch). | **receiving**: the oval's well |

- **Class** `stairwell.straight.2400x3200.arched`: clear 2.400 (y ±1.200); jambs to the springing 2.000; semicircular
  head r 1.200 about (y 0, z 2.000); crown 3.200. 64 stations per semicircle (shared with the oval's head wall).
- **The well is served by the oval.** `nympheas` owns the prism museum x −90.80…−74.60, y ±1.60, z −6.15…0.00 (local x
  −46.80…−30.60, z −0.35…+5.80): the 36-riser flight, landings, strings, handrails, shaft walls, head wall, soffit,
  stair lights and the pond's posts. Its bottom landing (z 0.000 here) runs through its head wall to the plane. **The
  Reserve builds nothing west of x −30.600.** There is no stair of the Reserve's own.
- **At the port** the Reserve builds the class opening through its 0.60 west wall (x −30.600 → −30.000), brick reveal
  `reserve.door.reveal`. The joint lays the honed travertine threshold slab through the wall depth (x −30.600 →
  −30.000, 2.400 wide, top 0.000) and the brushed bronze strip on the mating plane (doorway detail, Story §4).
- **Blinds.** Tonight the Reserve is absent, so the blind is the oval's (`nympheas.stair.blind-doors`: the closed bronze
  doors on the port face). On the Reserve's own stand (oval absent) it fills the class opening with its own closed pair
  of cast bronze leaves, meeting rail on the axis: `reserve.stair.blind-doors`.

### Anchors (the stage contract)
All frames at rest (every rack home except C, which starts out). Anchors on a rack's body ride it.

| Key | Kind | Frame | Belongs to |
|---|---|---|---|
| North racks: reserve.screen-a, reserve.screen-b, reserve.screen-c, reserve.screen-d, reserve.screen-e, reserve.screen-f, reserve.screen-n07, reserve.screen-n08, reserve.screen-n09, reserve.screen-n10, reserve.screen-n11, reserve.screen-n12, reserve.screen-n13, reserve.screen-n14, reserve.screen-n15, reserve.screen-n16, reserve.screen-n17, reserve.screen-n18 | Body | origin at the rack's floor centre at home (x from §3.7, y −9.150, z 0), axes of the stage; moves +y 0 → 5.650 | the rack: frame, mesh, carriage, pull, picture light, and its Wall anchors |
| South racks: reserve.screen-s01, reserve.screen-s02, reserve.screen-s03, reserve.screen-s04, reserve.screen-s05, reserve.screen-s06, reserve.screen-s07, reserve.screen-s08, reserve.screen-s09, reserve.screen-s10, reserve.screen-s11, reserve.screen-s12, reserve.screen-s13, reserve.screen-s14, reserve.screen-s15, reserve.screen-s16, reserve.screen-s17, reserve.screen-s18 | Body | the same at y +9.150; moves −y 0 → 5.650 | as above |
| One per rack: reserve.screen-a.pull, reserve.screen-b.pull, reserve.screen-c.pull, reserve.screen-d.pull, reserve.screen-e.pull, reserve.screen-f.pull, reserve.screen-n07.pull, reserve.screen-n08.pull, reserve.screen-n09.pull, reserve.screen-n10.pull, reserve.screen-n11.pull, reserve.screen-n12.pull, reserve.screen-n13.pull, reserve.screen-n14.pull, reserve.screen-n15.pull, reserve.screen-n16.pull, reserve.screen-n17.pull, reserve.screen-n18.pull, reserve.screen-s01.pull, reserve.screen-s02.pull, reserve.screen-s03.pull, reserve.screen-s04.pull, reserve.screen-s05.pull, reserve.screen-s06.pull, reserve.screen-s07.pull, reserve.screen-s08.pull, reserve.screen-s09.pull, reserve.screen-s10.pull, reserve.screen-s11.pull, reserve.screen-s12.pull, reserve.screen-s13.pull, reserve.screen-s14.pull, reserve.screen-s15.pull, reserve.screen-s16.pull, reserve.screen-s17.pull, reserve.screen-s18.pull | Use | the pull's midpoint (rack x, −6.592 north / +6.592 south, 1.275) at home, facing the nave; prompt within 1.60 | on its rack's body |
| `reserve.screen-a.west` | Wall | origin (−28.162, −11.650, 0); x (0,+1,0); y (−1,0,0) | rack A's west face |
| `reserve.screen-b.west` | Wall | (−26.262, −11.650, 0); x (0,+1,0); y (−1,0,0) | rack B's west face |
| `reserve.screen-b.east` | Wall | (−26.238, −6.650, 0); x (0,−1,0); y (+1,0,0) | rack B's east face |
| `reserve.screen-c.west` | Wall | (−24.362, −11.650, 0) at home (y −6.000 at the out stop); x (0,+1,0); y (−1,0,0) | rack C's west face |
| `reserve.screen-d.west` | Wall | (−20.662, −11.650, 0); x (0,+1,0); y (−1,0,0) | rack D's west face |
| `reserve.screen-d.east` | Wall | (−20.638, −6.650, 0); x (0,−1,0); y (+1,0,0) | rack D's east face |
| `reserve.screen-e.west` | Wall | (−18.762, −11.650, 0); x (0,+1,0); y (−1,0,0) | rack E's west face |
| `reserve.screen-e.east` | Wall | (−18.738, −6.650, 0); x (0,−1,0); y (+1,0,0) | rack E's east face |
| `reserve.easel` | Wall | origin (28.945, −1.000, 0) on the uprights' front plane (the painting's back); x (0,+1,0); y (−1,0,0) | the easel (§3.9) |
| `reserve.chest.bay7-south` | Mount | (18.750, 8.675, 0.905): the felt's top centre; the work's top edge to +y | the south island in bay 7 |
| `reserve.chest.bay8-south` | Mount | (26.250, 8.675, 0.905) | the south island in bay 8 |

Wall faces: origin at the face's left end seen from the room, on the works' back plane (±0.012 off the rack's centre
plane), z from the floor. Luminaires mount at the points in Lighting.json; the picture lights carry their rack's body.
No Seat, Bed or Water anchors. **Channels:** `reserve.screen-<name>.position` (0 home … 1 out), published by each
rack; `reserve.screen-a.lamp` … `reserve.screen-e.lamp` (derived, §6), followed by the picture lights.

## 3. Pieces by trade

**Plan.** Walls' inner faces x ±30.000, y ±12.000; outer ±30.600, ±12.600. Piers on the arcade lines **y ±5.000** at
**x −22.50, −15.00, −7.50, 0.00, 7.50, 15.00, 22.50** (museum −66.5 + 7.5k). Bays 1–8 west to east (clear spans):

| Bay | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| x from → to | −30.00 → −22.85 | −22.15 → −15.35 | −14.65 → −7.85 | −7.15 → −0.35 | 0.35 → 7.15 | 7.85 → 14.65 | 15.35 → 22.15 | 22.85 → 30.00 |
| clear centre | −26.425 | −18.750 | −11.250 | −3.750 | 3.750 | 11.250 | 18.750 | 26.425 |

Rows: north aisle y −12.00…−5.35 (rise 1.25), nave −4.65…+4.65 (rise 1.55), south aisle +5.35…+12.00 (rise 1.25).
Brick: Burgundy pressed brick 220 × 105 × 55 in 10 mm lime mortar (courses 65 mm, gauge 230 mm), English bond. Every
piece bears on its neighbour at a shared face; no sinks or overlaps.

### 3.1 Walls
- Four walls 0.60 thick, from the footing z −0.350 to the top (5.750 inside the Salon's outline, 5.470 beyond). Inner
  faces `reserve.wall.brick` rise behind the vaults' wall arches; outer faces, top and haunch fill
  `reserve.exterior.buried`.
- **West door** (the port, §2): class opening through x −30.600 → −30.000; arch turned in two brick rowlock rings
  (0.215 each); reveal `reserve.door.reveal`. On the hall face the doorway surround `reserve.door.architrave`: the
  museum's slim architrave P6 (`architrave.two-fascia.160`, nympheas §3.2: 0.160 wide out from the opening's edge,
  0.075 proud), up the jambs and round the arch (top 3.360), feet on the flags.

### 3.2 Floor
- `reserve.floor` (nave, |y| ≤ 4.65) and `reserve.floor.aisle` (the rest): Belgian bluestone flags **0.750 × 0.750 ×
  0.060**, honed, 3 mm lime joints on the grid x = 0.75k, y = 0.75k (joints on every pier line and on the axis), top
  z 0.000; bed of flat brick in lime to z −0.350 (`reserve.exterior.buried`). Flags run to the walls' faces (x −30.000
  meets the joint's threshold) and are cut round the pier and respond bases.
- **Flush floor tracks** `reserve.floor.slot`, 36, one in each rack's plane (x per §3.7), from |y| 1.200 to 11.450 on
  the rack's side of the axis: a cut 30 mm wide; a bronze channel 20 mm clear × 45 mm deep, closed at ends and bottom;
  5 mm lips flush with the flags.

### 3.3 Piers (14) and responds (18)
- **Base** `reserve.stone.base`: a solid square travertine block swept from (half-width, z): (0.45, 0.00) (0.45, 0.36)
  (0.41, 0.44) (0.41, 0.50) (0.37, 0.54), closed to the axis at 0.00 and 0.54 (`base.square-splayed`).
- **Shaft** `reserve.pier.brick`: 0.70 square with 40 mm chamfers, 0.54 → 3.64; plan octagon (±0.35, ±0.31) (±0.31,
  ±0.35); English bond, the chamfer in cut brick.
- **Impost** `reserve.stone.impost`: (0.37, 3.64) (0.41, 3.69) (0.45, 3.78) (0.45, 4.00), closed to the axis
  (`impost.square-splayed`). Its top at **4.000** is every arch's springing; the corner ledges outside the bands (0.35
  … 0.45 from the centre) carry the uplights.
- **Responds** where a band springs from a wall: on the north and south walls at each pier x (14), on the end walls at
  y ±5.00 (4). Measured out from the wall face (d), centred on the line: brick pilaster `reserve.wall.brick` 0.70 wide,
  d 0 … 0.12, z 0.54 → 3.64; travertine base `reserve.stone.base` 0.90 wide × d 0.22, z 0 → 0.36, then 0.82 × d 0.18,
  0.36 → 0.54; impost `reserve.stone.impost` 0.90 wide, a solid block (d, z): (0.00, 3.64) (0.14, 3.64) (0.22, 3.76)
  (0.22, 4.00) (0.00, 4.00).

### 3.4 Vaults
24 groin vaults (8 bays × 3 rows), bands 0.06 below the webs, all springing at 4.000.
- **Webs** `reserve.vault.web`, intrados over bay b, row r: z = 4.06 + max(R_r·√(1 − v²), R_r·√(1 − u²)), u = (x −
  x_b)/h_b, v = (y − y_r)/h_r (x_b, h_b the clear span's centre and half; y_r, h_r the row's). Crowns 5.61 (nave), 5.31
  (aisles). Both arches sampled at the same 40 angular stations: the groins are shared edges on the bay's diagonals.
- **Bands** `reserve.vault.band`: cross bands over every pier line (7 × 3 = 21), 0.70 wide (pier x ± 0.35), intrados
  4.00 + R_r·√(1 − v²) (crowns 5.55, 5.25), the aisle ones springing from the wall responds; arcade bands along y ±5.00
  ± 0.35 over every bay (16), intrados 4.00 + 1.25·√(1 − u²) (crown 5.25), the end bays' from the end-wall responds.
  Side faces rise from the intrados to the web beside it: 0.06 on the aisle side; on the nave side of each arcade band
  a **lunette** at |y| 4.65 from 4.00 + 1.25 f(x) to 4.06 + 1.55 f(x). Over each pier the springing plane closes the
  vault at 4.00 within ±0.35.
- **Thickness:** webs half-brick shells **0.105** normal to the intrados, stretcher bond, courses parallel to each
  barrel's axis, cut brick at the groins; bands 0.165. Haunches filled solid in brick to the top
  (`reserve.exterior.buried`). Extrados crowns: nave 5.715 (35 mm under the Salon's zone), aisles 5.415 (55 mm under
  the lawn top).
- No cornice, no string course; the bands' arrises are square.

### 3.5 Glazing
None in the fabric. The pastels' glazing `reserve.pastel.glass` is Art's (§8).

### 3.6 Overhead tracks (fixed), 36
One per rack, along y at the rack's x, from the side wall's face (|y| 11.998) to **|y| 0.900**; `reserve.track`:
- **Channel** 60 wide (x ±0.030), underside z **3.580**, top **3.700**: 12 mm top plate, two 8 mm sides, lips leaving a
  **26 mm** slot (x ±0.013) for the trolley stems.
- **Nave-end stop** 68 × 12 mm, z 3.574 … 3.704. **Wall plate** 0.150 wide, z 3.510 … 3.760, 42 mm (30 into the brick),
  four bolts, hex heads Ø 22 × 12, at x ±0.05, z 3.535 and 3.735.
- **Hanger rods**, 3 per track at |y| **7.60, 5.00, 1.20**: clamp 0.072 × 0.060 × 0.042 on the track; rod Ø 14 from the
  track's top to the intrados above it (at 5.00 the arcade band: lowest over rack C, z 5.018); collar Ø 28 × 28;
  canopy Ø 76 × 50 against the brick.

### 3.7 The racks (moving bodies), 36
Three per bay in both aisles, bays 1–6, edge-on to the nave. Rack x (bay pitch centre −1.90, 0, +1.90):

| Bay | x | North (home y −9.150) | South (home y +9.150) |
|---|---|---|---|
| 1 | −28.15, −26.25, −24.35 | A, B, C | S01, S02, S03 |
| 2 | −20.65, −18.75, −16.85 | D, E, F | S04, S05, S06 |
| 3 | −13.15, −11.25, −9.35 | N07, N08, N09 | S07, S08, S09 |
| 4 | −5.65, −3.75, −1.85 | N10, N11, N12 | S10, S11, S12 |
| 5 | 1.85, 3.75, 5.65 | N13, N14, N15 | S13, S14, S15 |
| 6 | 9.35, 11.25, 13.15 | N16, N17, N18 | S16, S17, S18 |

Home: centre |y| 9.150, spanning |y| 6.650 … 11.650 (0.35 off the side wall). Out: centre |y| 3.500, spanning 1.000 …
6.000. One rack in its body frame (x across, faces at ±x; y along, nave end towards the axis; z up):

| Part | Extent | Surface |
|---|---|---|
| End posts (2) | drawn hollow section 0.090 along × 0.160 across, wall 3 mm, centred y ±2.455; z 0.070 … 3.400; arrises r 8 mm | `reserve.screen.frame` |
| Top rail | 0.100 across × 0.110, y ±2.410, z 3.290 … 3.400 | `reserve.screen.frame` |
| Kick rail | 0.100 across × 0.150, y ±2.410, z 0.070 … 0.220 | `reserve.screen.frame` |
| Mesh | one sheet in the centre plane x 0, y ±2.410, z 0.220 … 3.290, edges clamped 12 mm into slots in posts and rails; crimped woven wire Ø 3.0 at 25 mm pitch (real wires, not an alpha) | `reserve.screen.mesh` |
| Trolleys (2) | at y ±2.100: saddle 0.072 × 0.140 × 0.014 on the top rail; stem Ø 16, z 3.410 … 3.610 into the track; nut Ø 32 × 16 | `reserve.screen.carriage` |
| Guide shoes (2) | under the kick rail at y ±2.200: 0.044 × 0.140, z 0.050 … 0.074 | `reserve.screen.carriage` |
| Guide fins (2) | 9 mm × 0.100, z −0.030 … 0.052, in the floor track | `reserve.screen.carriage` |
| Pull | on the nave end post: bar Ø 22, z 0.950 … 1.600, 58 mm off the post on two stand-offs Ø 14 at 1.000 and 1.550 | `reserve.screen.pull` |
| Picture-light trough (lit faces only, §4) | 64 × 44 mm at x ±0.420, centre z 3.320, over the lamp span; two arms Ø 14 from the top rail to the trough 0.12 in from the span's ends; plates 0.050 × 0.060 on the rail | `reserve.screen.lamp` |

Works hang on the mesh by pairs of bronze S-hooks at the frames' hangers (Art's hardware); their backs 0.012 off the
centre plane. Letter plates A–F wait for polish.

### 3.8 Luminaire bodies
- **Pendants (24)**, one at each bay's clear centre: nave (8) y 0, globe `reserve.globe` **Ø 0.44**, centre z **3.95**,
  on a **chain** from a canopy at the nave crown 5.61; aisles (16) y ±8.675, globe **Ø 0.34**, centre z **4.42**, on a
  **rod Ø 15** from a canopy at the aisle crown 5.31. With R the globe's radius (lathe profiles, (r, z − zc), zc the
  sphere's cut height at neck radius 0.34R): globe cut at 0.34R, neck 0.12R tall, rolled lip (0.34R, top − 0.006)
  (0.34R + 0.004, top − 0.004) (0.34R + 0.005, top − 0.002) (0.34R − 0.004, top); gallery (spun 1.5 mm) (0.52R, −0.013)
  (0.535R, −0.009) (0.52R, −0.005) (0.44R, 0.004) (0.41R, 0.008) (0.41R, 0.030) (0.38R, 0.035) (0.20R, 0.041) (0.13R,
  0.043); socket cup (0.13R, 0.042) (0.13R, 0.091) (0.15R, 0.093) (0.15R, 0.097) (0.12R, 0.100) (0.04R, 0.104) (0,
  0.104); canopy at crown Cz, k 1.00 nave / 0.85 aisle, (r, z − Cz): (0.070k, 0.020) (0.070k, −0.006) (0.064k, −0.020)
  (0.045k, −0.034) (0.020k, −0.041) (0.012, −0.043) (0.012, −0.050) (0, −0.050), its top 0.02 in the brick. Nave chain:
  links of 6.4 mm wire, 28 × 12 inside, 28 mm pitch, each turned 90°, loops Ø 9.5 at both ends. Aisle rod from the cup
  + 0.002 to Cz − 0.066, swivel ball Ø 26 at Cz − 0.062, hex locknut Ø 22 × 12 on the cup. Three thumb screws through the
  gallery's band at 90° + 120°k. Fittings `reserve.pendant.fitting`.
- **Uplights (56)**, four on each pier's impost ledges: lens centre (pier x ± 0.40, pier y ± 0.40, **4.095**), aimed
  **58°** above horizontal along the diagonal away from the pier; can Ø 44 × 75, lens 6 mm behind a 4 mm bezel, knuckle
  stem Ø 14 to the ledge; `reserve.uplight.body`.
- **Easel spot (1)**: canopy on the nave web of bay 8 at (24.40, 1.90, 5.475), stem Ø 12 down to the head at **(24.40,
  1.90, 4.25)**; `reserve.uplight.body`.

### 3.9 Fittings: the easel and the plan chests
**Easel** `reserve.easel` (oak), fittings `reserve.easel.fitting` (bronze); uprights' front faces at **x 28.950**, on the
axis, facing west:

| Part | x | y | z |
|---|---|---|---|
| Runners (2) | 28.55 … 29.45 | ±0.505 … ±0.595 | 0.00 … 0.09 |
| Cross foot | 29.25 … 29.33 | −0.59 … 0.59 | 0.00 … 0.09 |
| Uprights (2) | 28.95 … 29.03 | ±0.515 … ±0.585 | 0.09 … 2.35 |
| Caps (2, bronze) | 28.945 … 29.035 | ±0.51 … ±0.59 | 2.35 … 2.38 |
| Lower / upper rail | 28.955 … 29.025 | −0.515 … 0.515 | 0.35 … 0.43 / 2.18 … 2.26 |
| Ledge brackets (2, bronze) | 28.84 … 28.96 | ±0.50 … ±0.60 | 0.70 … 0.75 |
| Ledge; lip | 28.84 … 28.97; 28.84 … 28.87 | −0.62 … 0.62 | 0.75 … 0.80; 0.80 … 0.835 |

**Plan chests**, four islands of two chests back to back, centres x **18.75** (bay 7) and **26.25** (bay 8), y **±8.675**:
plinth `reserve.chest.plinth` x ±1.05, y ±1.00, z 0 … 0.09; carcass `reserve.chest` x ±1.10, y ±1.05, z 0.09 … 0.865;
top x ±1.12, y ±1.07, z 0.865 … **0.900**; ten drawers a side (y faces), two columns of five: fronts 1.064 × 0.1364,
12 mm gaps, from x −1.07 and z 0.115, 25 thick, 15 proud; pulls `reserve.chest.pull` 140 × 18, 22 proud, centred at
0.62 of the front's height. Felt `reserve.chest.mat` on the two south islands only, 5 mm (z 0.900 … 0.905): 0.99 (x)
× 0.76 (y) in bay 7, 0.58 × 0.74 in bay 8.

## 4. Light
**Daylight:** none (buried; the port's light from the well is transport across a joint, not an opening).
**Colour:** 3000 K throughout (as built 2900 K). **Operation (D-006):** a store is dark by default; every luminaire
is **presence**-linked: on while a visitor is in `reserve.undercroft` or on the oval's lower flight, off when it is
empty. The nympheas stair lights (always on) show the closed doors or the door.

| Id | Type | Count | Mount and aim | Operation |
|---|---|---|---|---|
| `pendant.nave.1…8` | `gap.pendant-globe.3000k` | 8 | globe centre (bay centre, 0, 3.95); aim down | presence |
| `pendant.aisle-n.1…8`, `pendant.aisle-s.1…8` | `gap.pendant-globe.3000k` | 16 | (bay centre, ±8.675, 4.42); aim down | presence |
| `uplight.p<line><n/s>.<corner>` | `gap.uplight.3000k` | 56 | §3.8; aim 1.5 m along the 58° diagonal (onto the groin) | presence |
| `easel.spot` | `track-spot.3000k.12deg` | 1 | (24.40, 1.90, 4.25); aim the painting's centre (28.90, 0, 1.515) | presence |
| `screen-<x>.<face>.picture` | `linear-cove.3000k` in the trough | 8 | lens (rack x ± 0.420, −9.150, 3.298) at home, along y; aim the face at z 1.55; lengths A west 1.19, B west 1.56, B east 0.84, C west 3.80, D west 1.04, D east 1.19, E west 1.03, E east 0.89 | presence, follows `reserve.screen-<x>.lamp`; body its rack |

**Targets** (Lighting.json): the works' faces `reserve.frame.gilt` oil-and-tempera 150–200 (the rack out and the easel;
racks at home are in store); the pastels `reserve.pastel.glass` works-on-paper 50 (annual 150 000 lx·h, kept by presence);
the nave `reserve.floor` circulation 100; the aisles `reserve.floor.aisle` gallery.low-light 50. Empty: dark.
**Design ratios** (measured, never tuned): lit works ≥ 3 × the vault webs; webs (uplit near the groins) ≥ the piers'
faces; piers ≥ the bluestone floor, the darkest surface, which carries the globes' reflections.

## 5. Time and weather
No glazed opening: no weather place, no sky, no wetness. The Moment is read only for presence. With the pond up the
oval's Giverny light reaches the stair's foot through the well; the Reserve's results are the same at every moment.

## 6. Motion
**M-1…M-36 `reserve.screen-<name>`** (Mechanisms.json), one per rack:
- Body `reserve.screen-<name>`: the rack's frame, mesh, carriage, pull, picture light and hung works. Joint prismatic
  along **+y** (north racks) or **−y** (south), travel **0 → 5.650** (home |y| 9.150 → out 3.500).
- Drive: jerk-limited S-curve, **1.0 m/s, 0.6 m/s², 1.2 m/s³** (≈ 7.8 s). Top-hung on two trolleys in its track, kept
  upright by two fins in the floor track.
- Control: the Use anchor `reserve.screen-<name>.pull`: "Draw out screen C" at home, "Slide screen C home" when out.
- **Group `reserve.screens`:** at most one rack out; drawing one sends any other home first (a sequence rule, not an
  interlock: the travel volumes are disjoint, 1.90 apart). Rack C **starts out** (the three Renoirs in the nave).
- **Guard:** holds while the visitor's body is in the next travel slice (radius + 0.05 against the rack's footprint,
  frames ±0.14 off its plane); caption "Screen C waits: you are in its way"; never pushes, never reverses.
- **Channels:** each publishes `reserve.screen-<name>.position`; for A–E a derived `reserve.screen-<name>.lamp` follows
  it, in [0.99, 1.00] → out [0, 1]: the picture light comes up in the last 57 mm, only out.
- **Clearances (≥ 5 mm at every phase):** fin 9 mm in the 20 mm floor track, 5.5 each side, 15 mm over its floor; stem
  Ø 16 in the 26 mm track slot, 5 each side; stem nut top 3.428 to the track's underside 3.580: 0.152; rack foot
  0.070 (shoes 0.050) over the floor; home 0.35 off the wall; out, the posts' faces to the pier bases 1.32 (e.g. C's
  east face −24.27 to the base −22.95), D's west trough to the base at −22.05: 0.95; the pull (|y| 0.942) to the nave
  globe (|y| ≤ 0.22): 0.72; the track's end (|y| 0.900, top 3.700) to the globe: 0.68 in y, 0.03 below; trough top (≈ 3.342) to an aisle globe's bottom (4.25): 0.91; track top (3.70) to the lowest arch
  over it (5.018): 1.32. At home each rack stands in its aisle bay (its housing). Nothing of the Reserve enters the well.

## 7. Nature
None inside. Above: the lawn's 0.30 of soil on the top at 5.470 (the Grounds' terrain).

## 8. Art
13 works from `data/artworks.json` (not in `assets/collection.json`); sizes from MuseePlan.h:433-446 and the as-built
mats (ReserveStructure.cpp:1131). Oils: canvas in `reserve.frame.gilt` (0.090 face), care oil-and-tempera. On a rack
face: one row centred on the rack, centre height 1.55, 0.35 between frames. Positions at home (x = back plane; y
centre; z 1.55); rack C's column also given out.

| Work (id) | Title · artist | w × h | Anchor · along / height | Centre (x, y) |
|---|---|---|---|---|
| caillebotte-young-man-window | Young Man at His Window · Caillebotte | 0.81 × 1.16 | `reserve.screen-a.west` · 2.095 / 0.970 | (−28.162, −9.150) |
| degas-place-de-la-concorde | Place de la Concorde · Degas | 1.18 × 0.78 | `reserve.screen-b.west` · 1.910 / 1.160 | (−26.262, −9.150) |
| degas-orchestra-opera | The Orchestra at the Opera · Degas | 0.46 × 0.57 | `reserve.screen-b.east` · 2.270 / 1.265 | (−26.238, −9.150) |
| renoir-cirque-fernando | Acrobats at the Cirque Fernando · Renoir | 0.99 × 1.31 | `reserve.screen-c.west` · 0.705 / 0.895 | (−24.362, −10.450; out −4.800) |
| renoir-two-sisters | Two Sisters (On the Terrace) · Renoir | 0.81 × 1.01 | `reserve.screen-c.west` · 2.225 / 1.045 | (−24.362, −9.020; out −3.370) |
| renoir-the-swing | The Swing · Renoir | 0.73 × 0.92 | `reserve.screen-c.west` · 3.565 / 1.090 | (−24.362, −7.720; out −2.070) |
| pissarro-red-roofs | Red Roofs · Pissarro | 0.66 × 0.55 | `reserve.screen-d.west` · 2.170 / 1.275 | (−20.662, −9.150) |
| pissarro-avenue-opera-snow | Avenue de l'Opéra, Snow Effect · Pissarro | 0.81 × 0.65 | `reserve.screen-d.east` · 2.095 / 1.225 | (−20.638, −9.150) |
| sisley-bridge-villeneuve | The Bridge at Villeneuve-la-Garenne · Sisley | 0.65 × 0.50 | `reserve.screen-e.west` · 2.175 / 1.300 | (−18.762, −9.150) |
| sisley-snow-louveciennes | Snow at Louveciennes · Sisley | 0.51 × 0.61 | `reserve.screen-e.east` · 2.245 / 1.245 | (−18.738, −9.150) |
| caillebotte-pont-europe | Le Pont de l'Europe · Caillebotte | 1.80 × 1.25 | `reserve.easel` · 0.100 / 0.890 | (28.945, 0.000), z 1.515, facing west |
| degas-the-tub | The Tub · Degas (pastel on card) | 0.83 × 0.60 | `reserve.chest.bay7-south`, centre (18.750, 8.675, 0.905) | flat, top edge south |
| degas-the-star | The Star · Degas (pastel over monotype) | 0.42 × 0.58 | `reserve.chest.bay8-south`, centre (26.250, 8.675, 0.905) | flat, top edge south |

- The pastels lie flat on their felt, read from the arcade side, each in a window mat `reserve.pastel.mat` (80 mm
  border) under `reserve.pastel.glass` on a 6 mm spacer; care works-on-paper.
- Rack F and the 30 unlettered racks are empty (kept for acquisitions). Rack A's east face is empty while the Pont is on
  the easel (the send-to-easel carry waits, §12). Captions on look (Art). Every painting has a back, seen through the mesh.

## 9. Sound
From Matter, never tuned: ≈ 7,000 m³ of lime-jointed brick vault over honed bluestone with no soft surface (the mesh
absorbs nothing): a long, live masonry decay; footsteps carry down the nave. Emitters: footfalls by substance; the
racks' trolleys in their bronze tracks and the soft stop (FMuseeActuation on each rack's body). Sealed from outside;
with the pond up, the oval's rain and the pond's pump are heard faintly down the well.

## 10. Visitor
- **Arrive:** in the oval, lift the pond with the golden iris; walk down the 36 steps and through the arched west door
  onto the nave's axis; the nave runs 60 m east to the easel. **Leave** the same way (the pond never lowers while the
  visitor is below: nympheas §6).
- **Walk** the nave and the aisles. **Use a rack** (pull, beam or gaze; prompt within 1.60): "Draw out screen C" /
  "Slide screen C home"; one out at a time; a rack waits while you stand in its way.
- **Look:** captions at the rack faces, the easel and the chests. **Sit:** none (as built).
- **Start of a visit:** rack C out with its three Renoirs in the nave; *Le Pont de l'Europe* on the easel.

## 11. The old-code map
Paths relative to `windows\`. Re-express, never copy. Museum → local: x + 44.00, z + 5.80.

| Part | Where |
|---|---|
| Reserve plan numbers (walls, piers, rises, racks, works, easel, chests) | Source/MuseeVision/Plan/MuseePlan.h:370-459 (namespace Reserve :376) |
| Long stair (the oval's; the Reserve's west wall outer face −74.6) | …/MuseePlan.h:346-368 |
| Constants: door (DoorHalf 1.19, spring 2.0), stone, rises, slot, pendants, uplights | Source/MuseeVision/Reserve/ReserveStructure.cpp:28-81 (door :38, slot :66, globes :75-76, uplights :81) |
| Bays, rows, pendant list | …/ReserveStructure.cpp:88-121 |
| Arches (shared stations), groin webs, cross bands, arcade bands and lunettes | …/ReserveStructure.cpp:175-343 |
| Pier sweeps, chamfered shaft; corbel and respond | …/ReserveStructure.cpp:345-399, 402-451 |
| Slot rectangles; floor with holes | …/ReserveStructure.cpp:480-536 |
| Plan chests; easel | …/ReserveStructure.cpp:539-566, 573-588 |
| Chain link; uplights; pendant | …/ReserveStructure.cpp:704-722, 783-828, 835-950 |
| Walls, west door, top; piers and responds placed; floor and slots; vault assembled; chests placed | …/ReserveStructure.cpp:955-1016, 1018-1040, 1042-1068, 1072-1113, 1130-1137 |
| **Do not port:** WeldWeights, the void cap, fallback materials, hidden landing collision | …/ReserveStructure.cpp:724-781, 1119-1128, 1142-1190, 1068-1069 |
| Rack constants; work slots; lamp spans; one rack; tracks and rods; home/out | Source/MuseeVision/Reserve/ReserveRacks.cpp:33-54, 103-127, 164-169, 172-256, 293-332, 334-340 |
| Initial rack; occupancy guard; glide (4.5 s, smoothstep); lamps on at out; one out | …/ReserveRacks.cpp:668-685, 746-766, 768-806, 808-832, 836-843; ReserveRacks.h:111 |
| Uses and prompts; send-to-easel carry (polish) | …/ReserveRacks.cpp:852-994, 996-1060 |
| **Do not port:** ArrangeImported, the lamps' hand-set output, oak/linen materials | …/ReserveRacks.cpp:583-666; ReserveRacks.h:113-127 |
| Lamp colour 2900 K, pendant and uplight roles, easel spot (−19.6, 1.9, −1.55) | Scripts/native.py:102-129, 280-350 |
| **Do not port:** the Reserve white-balance volume (4300 K) | Scripts/native.py:117-121, 351-364 |
| Brick, globe, frames | Scripts/reserve_materials.py:1-48; Scripts/frames.py:13, 47-48 |

**Corrections to the draft** (`r-specs/…/Stages/reserve.md`):
1. **The port (D-019, the Architect's ruling).** The draft's `reserve.stair` (`stairwell.straight.2400x7500`, a 7.50 ×
   2.40 opening in the oval's floor facing +z, mating `salon.stair`) is superseded: `reserve.stair` is the west door,
   `stairwell.straight.2400x3200.arched`, local (−30.600, 0, 0) facing −x = museum (−74.600, 0, −5.800), mating
   `nympheas.stair`. The Salon has no stair port.
2. **The well is the oval's.** The draft's shaft limb (x −46.80…−30.60), receiving well W1/W2, post bores B1–B4, the
   collar, shaft walls, base and roof slabs and soffit are gone from the Reserve; the footprint is the hall box alone.
3. Door clear **2.40** (the class; as built 2.38), crown 3.20 (draft 3.19). The joint's threshold runs through the
   wall depth only; the as-built 0.30 notch into the hall (to x −29.70) is dropped and the flags run to the wall.
4. Materials per the Story: bluestone flags (draft polished concrete); brass-framed woven brass mesh racks (draft oak and
   linen screens); fumed oak chests (draft walnut); bronze pulls and easel hardware (draft brass); Burgundy brick 220 ×
   105 × 55 and a 0.105 shell (draft UK 215 × 102.5 × 65, 0.1025).
5. The picture rails and hanging rods go with the linen: works hang on the mesh; backs 0.012 off the centre plane
   (draft 0.065). Eight picture lights, not nine (A's east face is empty while the Pont is on the easel).
6. Luminaire types are catalogue keys or `gap.` keys (the draft's `pendant-globe…`, `uplight-mini…`,
   `picture-light.3000k.linear` were unlisted); the easel spot is `track-spot.3000k.12deg`.
7. The draft's "pull to nave globe 2.0 m apart in x" is wrong: in bays 2–7 the nave globe shares the middle rack's x;
   the clearance is 0.72 in y (pull) and 0.68 (track end).
8. No stair-foot call control (nympheas never lowers the pond with the visitor below). Anchor kinds `Plinth` and
   `CaseBed` are not in the contract: the easel is a Wall anchor, the chests Mount anchors.
9. Light: aisles gallery.low-light 50 (draft circulation 100 everywhere); the empty store is dark (draft orientation 5).
10. Verified unchanged: origin (museum −44.00, −5.80), walls, bays, piers, profiles, rises, crowns, bands, responds, rack
    x and home/out, travel 5.65, works and sizes, row slots, easel, chests, mats, pendants, uplights, easel spot, track
    and rod positions.

## 12. The first-pass cut
**Tonight (D-019):** the Reserve is **not built**. Only its blind is: the closed bronze doors on the port face at museum
x −74.60, where the oval's stair ends, built by the oval (`nympheas.stair.blind-doors`, nympheas §3.9). Nothing of
this sheet is made tonight.

**The Reserve's first pass (later):** the shell (walls, west door and architrave, piers, responds, the 24 vaults, bands,
lunettes, fill, top), the bluestone floor with its 36 flush tracks, the 36 overhead tracks and rods, the 36 racks with
M-1…M-36, the guard, group and channels; the 24 pendants, 56 uplights, easel spot and 8 picture lights; the easel and
four chest islands; the 13 works with frames, mats and glazing; presence operation; sound.
**Waits for polish:** the send-to-easel carry (and A east's light); letter plates A–F; trolley wheels and flex; the
racks' rolling voice; brick bond detail at the chamfers and rings; a study bench before the easel (owner).

## 13. Palette keys to confirm and catalogue gaps
**Palette keys** (no `todo.` keys; roles guessed or extended):
- `accent.brass-cuzn30` (frame "drawn, lacquered", mesh "woven, lacquered") on the **as-built top-hung rack geometry**:
  the role is the Story's, the geometry the code's (ruling, below).
- `bronze.cusn10` patinated for the **overhead tracks and hanger rods**: the Story names only "rack tracks, flush:
  brushed" (the floor tracks); guessed classical patina for the unwalked ones.
- `bronze.cusn10` patinated for **pendant fittings, uplight bodies, picture-light troughs, easel caps and brackets,
  chest pulls and plinths, the blind doors**: guessed by analogy (globe rods, carriages).
- `accent.brique-bourgogne` for the **buried fill, outer faces and floor bed**: extends "piers, vaults".
- `accent.rag-board` (ruled) for the **pastel mats** and `glass.low-iron` for their **glazing**: roles guessed (Art's).
- `oak.sessile-french` "fumed and waxed" for the chests, "oiled and hard-waxed" for the easel: as the Story words them.

**Catalogue gaps** (Light):
1. `gap.pendant-globe.3000k`: a cased opal glass globe pendant (Ø 0.44 and 0.34), lamp inside the globe, the globe the
   diffuser, bronze gallery; 3000 K.
2. `gap.uplight.3000k`: a miniature surface uplight can (≈ 45 mm lens, wide beam, knuckle mount, glare-shielded),
   3000 K, standing on a ledge.
3. The racks' picture lights use `linear-cove.3000k` in a bronze trough; the catalogue's `picture-light.2700k` is
   2700 K and evidence-incomplete. A trough picture-light type at 3000 K would be truer.

**Contract questions (for the Architect):** Mechanisms.json here uses two fields the contract lacks: `"group":
"reserve.screens"` (at most one member out; a use sends the others home first) and `"start": "open"` (rack C's
initial position; `home` stays `closed`).

## Build notes
- **Frame.** Origin at the nave's centre on the floor, museum (−44.00, 0, −5.80), yaw 0. Everything here is local.
- **The port is a door, not a hole in the oval's floor.** `reserve.stair` at (−30.600, 0, 0) faces −x and mates
  `nympheas.stair` (museum −74.600, 0, −5.800, facing +x). The oval serves the well; build nothing west of x −30.600.
  The Reserve builds the class opening through its own 0.60 wall; the joint lays the threshold and the bronze strip.
- **36 moving bodies.** Each rack (frame, mesh, carriage, pull, picture light, its Wall anchors and works) hangs on
  `reserve.screen-<name>` and glides 5.650 along ±y. The overhead tracks, rods and floor tracks are fixed.
  Rack C starts out. At most one out (group).
- **Picture lights ride their rack** (Lighting `body`) and follow the derived `.lamp` channel: lit only at the out stop.
- **The vault is a rule, not a model**: z = 4.06 + max of two semi-elliptical barrels per bay, 40 shared stations,
  so the groins are shared edges; bands and lunettes by the same arches. Half-brick shells, solid haunch fill.
- **The mesh is real wire** (crimped CuZn30, Ø 3.0 at 25 mm): the works' backs and the paintings' backs are seen through it.
- **Chains** are instanced links; globes are lathe profiles with Light's lamp inside the opal.
- **Presence**: every light is presence-linked; the empty store is dark (pastels' annual dose).

**Departures from as-built** (for the owner's review):
- **Schedule corrected to the contract (K3-MATTER):** Finishes.json names its workings by their palette keys (`laid.mortar-jointed`, `drawn.lacquered`, `woven.lacquered`, `oiled.hard-waxed`, `fumed.waxed`, `laminated.uv-filtering`) where it wrote them as prose, which the reader refused; the frames' water gilt lies on `white.gesso` (the gesso over the carved oak), as the Salon's does.
- **Materials follow the Story (D-018):** honed Belgian bluestone flags replace the dark polished concrete; the racks'
  frames are drawn brass and their faces woven brass mesh (the Story's racks) in place of oak and linen, on the
  as-built top-hung geometry and tracks (ruled: port the geometry, D-017; the material is the Story's); fumed oak chests
  replace walnut; bronze replaces brass on pulls and easel hardware; Burgundy brick replaces the buff stock.
- With the linen gone, the picture rails and rods go; works hang on the mesh by S-hooks, backs 0.012 off the centre
  plane (as built 0.065).
- **Door** 2.40 clear, crown 3.20 (as built 2.38 / 3.19), to the port class; threshold within the wall depth; the 0.30
  notch into the hall dropped; a honed travertine P6 architrave on the hall face (the Story's doorway detail).
- **Clearances to ≥ 5 mm (ruled):** floor track 16 → 20 mm clear, track slot 20 → 26 mm.
- **Glide** ≈ 7.8 s, a jerk-limited S-curve at 1.0 m/s (as built 4.5 s smoothstep peaking ≈ 1.9 m/s): a 5 m rack
  drawn by hand at a walking pull beside the visitor (ruled: physically real).
- **Light:** 3000 K catalogue types (as built 2900 K, output set by hand); the 4300 K white-balance volume retired; all
  light presence-linked and the empty store dark; the aisles at gallery.low-light; A's east picture light waits with the
  carry.
- **Pastels** glazed in window mats on their felt (as built bare on the felt).
- **Respond imposts** are solid blocks from the wall face (as built a corbel in front of the pilaster, leaving a gap).
- Lighting.json (K3-LIGHT-NATURE): the catalogue gaps are real types, `pendant-globe.3000k` (a 596 lm opal E27 lamp in the room's cased opal globe) and `uplight.3000k` (a 500 lm, 40 degree GU10 reflector lamp in the room's can, its face 50 mm where the sheet asks about 45).
- Hang.json (ST-RESERVE), to §8: the pastels' window mats are named as their mats (`mat` reserve.pastel.mat, `matSize` 0.99 × 0.76 and 0.58 × 0.74), not as frames, and their glazing as `glazed` / `glazing` reserve.pastel.glass; a card lying on its reader is unframed.
