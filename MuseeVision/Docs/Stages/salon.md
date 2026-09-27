# The Salon Impression · design sheet

- **Stage key** `salon` (module `MuseeSalon`, `USalonRoom : USiteRoom`). Display name "Salon Impression". Era: Classical
  (stone honed, bronze patinated or oil-gilt, plaster matt, oak oiled and waxed). **Scope (D-019): the five bays and the
  Manet cabinet only.** The Nymphéas oval beyond bay 5 is the stage `nympheas`.
- **Sources:** D-001…D-019 (D-005, D-014, D-017, D-018, D-019 binding here); the approved `plan/proposals/salon-interior`
  boards (Main, Plan, Light, Details; the Oval board is `nympheas`'s); Palette.json + Story.md ("The Salon Impression");
  the drafter's spec `r-specs/…/Stages/salon.md` (verified; corrections in §11); the as-built code in `windows/` and the
  hang in `Shared/Plan/Hang.swift` (§11). Schedules: `Data/Stages/salon/{Finishes,Lighting,Hang,Planting}.json` (no
  Mechanisms: nothing moves here).
- **Units** metres. **Frame:** origin on the axis (y 0) in the plane of **bay 1's east end wall's silk face**, on the
  finished floor (the boards' "salon data 0"); x east, y south, z up, true north. museum = local + (−14.000, 0, 0).

## 1. Identity and placement

| | |
|---|---|
| Origin in the museum plan | (−14.000, 0.000, 0.000); yaw 0°. `salon.east`'s face x +0.600 lands at museum x −13.400; `salon.west`'s face x −60.600 at museum x −74.600. |
| Level | 0 (ground floor), level with the Rotunda's floor. The floor build-up bears on the Reserve's top at z −0.050 (MuseePlan.h:382-384): Site compares footprints as volumes. |
| Footprint kind | Building. |
| Vertical extent | z −0.050 to 14.050 (roof slab top); outer lanterns to 14.534; bay 3's pergola to 16.500 (leaves ≈ 16.75). Cabinet to 6.300. |

**Footprint polygon** (plan, clockwise on the north-up plan from the north-east corner): (+0.600, −7.600) (+0.600,
+7.600) (−60.600, +7.600) (−60.600, −7.600) (−10.400, −7.600) (−10.400, −16.000) (−1.600, −16.000) (−1.600, −7.600).
The oval's passage block (|y| ≤ 2.100, z ≤ 5.200) bears on the far wall's outer face from the west; it is `nympheas`'s.

| Enclosure | Kind | Bounded by |
|---|---|---|
| `salon.bays` | Enclosed | The five bays under one vault: long walls' silk faces y ±7.000, end wall x 0.000, far wall x −60.000, floor, vault, lunettes, the five lantern shafts up to their outer glazing. Openings: Glazed `salon.bay1.lantern` … `salon.bay5.lantern`; ports `salon.east`, `salon.west`; Open `salon.cabinet.door`. ≈ 10,000 m³. |
| `salon.cabinet` | Enclosed | The Manet cabinet: silk faces x −10.000, −2.000, y −15.600, −7.600; floor; ceiling 5.500 and its laylight well (to 5.900). Its only opening is `salon.cabinet.door` into `salon.bays`. No daylight. ≈ 350 m³. |
| `salon.outdoors.roof` | Outdoors | The roof slab's top (14.050) inside the outer faces: the five outer lanterns, bay 3's pergola and planters. Museum weather; not walked. |

## 2. Ports

Frames on the port face at the threshold (z 0), on the opening's centreline, facing out, up +z. Class names are
**width × crown** (the Architect's ruling).

| Key | Kind | Class | Local frame (position; facing) | Face; wall depth | Mates with | Wells |
|---|---|---|---|---|---|---|
| `salon.east` | doorway | `doorway.3000x6000.arched` | (+0.600, 0.000, 0.000); +x | flat; 0.600 (bay 1's end wall, x 0.000 → +0.600) | `rotunda.west` (frame museum (−11.20, 0, 0), facing −x). **Passage 2.20**, the joint's. | none |
| `salon.west` | doorway | `doorway.3000x4800.arched` | (−60.600, 0.000, 0.000); −x | flat; 0.600 (bay 5's far wall, x −60.000 → −60.600) | `nympheas.east` (its frame nympheas (+11.900, 0, 0) = museum (−74.60, 0, 0), facing +x). Passage 0.00. | none |

- `doorway.3000x6000.arched`: clear 3.000; semicircular head r 1.500 springing 4.500; crown 6.000 (MuseePlan.h:44-47).
  `doorway.3000x4800.arched`: clear 3.000; head r 1.500 springing 3.300; crown 4.800 (the Salon's doors as built,
  SalonStructure.cpp:41-42). Rough opening = clear + the class's lining allowance *a* (Site's; the sibling sheets use
  a = 0.050: rough width 3.100, head radius 1.550 about the springing, rough sill z −0.050).
- **`salon.east`, the step down (final).** The port's class is the Rotunda's arch; the Salon keeps its own 4.80 door on
  its bay face, and steps between them **inside its own east wall**:
  - **x +0.600 → +0.300 (outer 0.300):** the class rough opening (3.100 wide, head r 1.550 about z 4.500, rough crown
    6.050, sill −0.050), faces `salon.port.rough-opening`. The joint's lining runs on into it from the passage and stops
    against the step face; its finished face is the class section (3.000, crown 6.000).
  - **x +0.300, the step** `salon.east.step`: a vertical face facing +x, the band between the class clear arch above
    (z = 4.500 + √(1.500² − y²)) and the Salon's clear arch below (z = 3.300 + √(1.500² − y²)), |y| ≤ 1.500, and the two
    jamb strips 3.300 ≤ z ≤ 4.500 at y ±1.500 up to the lining's back (|y| 1.550): a constant 1.200 high on every
    vertical; its arris to the Salon's soffit eased 3 mm. 64 stations per semicircle, shared by both arches.
  - **x +0.300 → 0.000 (inner 0.300):** the Salon's own door, finished: jambs at y ±1.500, head r 1.500 springing 3.300,
    crown 4.800, reveal `salon.east.reveal`. The jambs run straight on from the lining's face (both 3.000 clear), so
    only the head steps. On the bay face (x 0.000), the P5 architrave `salon.door.architrave` (§3.3); the P1 cornice
    at 5.60 stays whole.
  - The joint lays the threshold slab through the whole wall depth (x 0.000 → +0.600, 3.100 wide in the outer part,
    3.000 between the finished jambs) and the bronze strip on the mating plane x +0.600; the Salon's own strip
    `salon.floor.strip` at x 0.000 (oak to threshold).
- **`salon.west`:** the Salon builds only the rough opening through its far wall (x −60.000 → −60.600; 3.100 wide,
  head r 1.550 about z 3.300, rough crown 4.850, sill −0.050), faces `salon.port.rough-opening`; the P5 architrave on
  the bay face (x −60.000) and its strip at x −60.000. The oval owns the passage block and the tunnel from the port
  face x −60.600 west; the joint lines the Salon's wall depth and lays the threshold and the strip on the mating plane.
  The far wall's outer face `salon.exterior.wall` stops at the passage block's outline (|y| 2.100, z 5.200).
- **Blinds.** Both neighbours are built tonight: both ports are open portals. On the stand alone (or with a neighbour
  absent), a closed pair of cast bronze leaves fills each class opening on its port face, meeting rail on the axis:
  `salon.east.blind-doors`, `salon.west.blind-doors`.
- **No stair port** (D-019): the Reserve's stair mates `nympheas.stair`.

### Anchors (the stage contract)
Wall anchors: origin on the hanging face at the wall's **left end as seen from the room**, x along the face to the
right, y out of the face into the room, z up from the floor. Hang `along` = the work's left edge along x.

| Key | Kind | Frame (origin; x axis; y axis) | Belongs to |
|---|---|---|---|
| `salon.bay1.wall.north` | Wall | (−11.400, −7.000, 0); x (+1,0,0); y (0,+1,0) | bay 1 north silk, x −11.40 … 0.00 (cabinet door at x −6.00) |
| `salon.bay1.wall.south` | Wall | (0.000, +7.000, 0); x (−1,0,0); y (0,−1,0) | bay 1 south silk, to x −11.40 |
| `salon.bay1.wall.east` | Wall | (0.000, −7.000, 0); x (0,+1,0); y (−1,0,0) | bay 1's end wall silk, y −7.00 … +7.00 (door y ±1.50, architrave ±1.72) |
| `salon.bay1.stele.east` | Wall | (−6.125, +2.000, 0); x (0,−1,0); y (+1,0,0) | the stele's east glass face, y +2.00 … +0.80 |
| `salon.bay2.wall.north` | Wall | (−23.400, −7.000, 0); x (+1,0,0); y (0,+1,0) | bay 2 north silk |
| `salon.bay2.wall.south` | Wall | (−12.600, +7.000, 0); x (−1,0,0); y (0,−1,0) | bay 2 south silk |
| `salon.bay3.wall.north` | Wall | (−35.400, −7.000, 0); x (+1,0,0); y (0,+1,0) | bay 3 north silk |
| `salon.bay3.wall.south` | Wall | (−24.600, +7.000, 0); x (−1,0,0); y (0,−1,0) | bay 3 south silk |
| `salon.bay4.wall.north` | Wall | (−47.400, −7.000, 0); x (+1,0,0); y (0,+1,0) | bay 4 north silk |
| `salon.bay4.wall.south` | Wall | (−36.600, +7.000, 0); x (−1,0,0); y (0,−1,0) | bay 4 south silk |
| `salon.bay5.wall.north` | Wall | (−60.000, −7.000, 0); x (+1,0,0); y (0,+1,0) | bay 5 north silk |
| `salon.bay5.wall.south` | Wall | (−48.600, +7.000, 0); x (−1,0,0); y (0,−1,0) | bay 5 south silk |
| `salon.cabinet.wall.north` | Wall | (−10.000, −15.600, 0); x (+1,0,0); y (0,+1,0) | cabinet north silk |
| `salon.cabinet.wall.east` | Wall | (−2.000, −15.600, 0); x (0,+1,0); y (−1,0,0) | cabinet east silk |
| `salon.cabinet.wall.west` | Wall | (−10.000, −7.600, 0); x (0,−1,0); y (+1,0,0) | cabinet west silk |
| `salon.cabinet.case` | Mount | (−8.600, −8.450, 0.864), the felt's top centre; x (+1,0,0) the case's length; reader's side −y | the 1874 documents (§8) |
| `salon.bay2.dancer` | Mount | (−20.500, +3.400, 1.000), the plinth's top centre; facing +x | the Little Dancer |
| `salon.bench.bay2.n`, `salon.bench.bay2.s`, `salon.bench.bay3.n`, `salon.bench.bay3.s`, `salon.bench.bay4.n`, `salon.bench.bay4.s`, `salon.bench.bay5.n`, `salon.bench.bay5.s` | Seat | seat top z 0.440 at the bench centres (−18.000, ∓2.600), (−30.000, ∓2.600), (−42.000, ∓2.600), (−54.300, ∓2.600) (`n` at y −2.600); long axis x; two frames per face at ±0.60 along, facing +y and −y (backless) | the velvet benches |
| `salon.pergola.planter.nw`, `salon.pergola.planter.ne`, `salon.pergola.planter.sw`, `salon.pergola.planter.se` | Bed | soil surface z 14.540 at (−32.600, −2.200), (−27.400, −2.200), (−32.600, +2.200), (−27.400, +2.200) | the vines (§7) |

No Body, Use or Water anchors. Luminaires mount at the points in Lighting.json. No channels.

## 3. Pieces by trade

**Plan (local):** bays by their silk faces: bay 1 x −11.400 → 0.000; bay 2 −23.400 → −12.600; bay 3 −35.400 → −24.600;
bay 4 −47.400 → −36.600; bay 5 −60.000 → −48.600. Piers at x −12, −24, −36, −48 (faces ±0.600). Long walls' silk faces
y ±7.000. Doors: 64 stations per semicircle.

### 3.1 Walls
- **Masonry** travertine ashlar: long walls y ±7.025 (masonry face) → ±7.600 (outer), end wall x +0.025 → +0.600,
  far wall x −60.025 → −60.600, to 13.850. Outer faces `salon.exterior.wall` (from −0.05 to 13.85; cabinet's to 6.10).
- **Silk** `salon.bay1.silk`, `salon.bay2.silk`, `salon.bay3.silk`, `salon.bay4.silk`, `salon.bay5.silk`, `salon.cabinet.silk`: face at the silk planes above, z 0.150 (into
  the skirting's slot) → 6.250 (behind the cornice); 25 mm build-up (silk over 5 mm felt interlining on 18 mm battens).
  Colours (the approved one step deeper): bay 1 oxblood #6A2A22, 2 zinc #47515A, 3 sage #5A6D54, 4 old rose #8A5A54,
  5 haystack #A0793A; cabinet green #2F3A33 (to 5.100). End walls silked round their doors.
- **Cabinet door**: centre x −6.000 through the north wall (y −7.000 → −7.600), 3.000 clear, springing 3.300, crown
  4.800; reveal `salon.door.reveal`; P5 on both faces. Skirtings stop 1.68 from each door's centre (inside the
  architrave's 1.72).
- **Cabinet**: silk faces x −10.000, −2.000, y −15.600 (south face y −7.600 = the Salon's north outer face); walls 0.40
  (outer x −10.400, −1.600, y −16.000); ceiling 5.500; roof slab 6.100 → 6.300.

### 3.2 Piers and transverse arches
- **Piers** `salon.pier`: from the masonry (|y| 7.025) to the jamb face |y| 4.500 (2.50 deep), 1.20 thick, floor to
  6.500. Base `salon.pier.base` (P3) and impost `salon.pier.impost` (P4) swept round the three faces, mitred.
- **Pier silk** (the bay's silk surface): on each side face (x = pier ± 0.600) in the colour of the bay it looks into;
  **12 mm proud**; from |y| 7.025 to **4.640** (0.14 of stone left at the jamb); z 0.300 → 6.060; jamb edge, top and
  bottom returned to the stone.
- **Arches**, over each pier pair, 9.000 clear: centre (y 0, z 6.500), intrados R 4.500, 1.200 thick.
  - `salon.arch.voussoir`: 15 stones of 12°, R 4.500 → 5.300; radial joints V-chamfered **12 mm** into the stone on both
    faces and across the soffit; springers on the imposts. Each its own slice of the quarry.
  - `salon.arch.keystone`: stone 7 (84°–96°), faces **20 mm proud**, radial sides, flat top z 12.300; no V on its faces.
  - `salon.arch.archivolt` P7 round the extrados (frames at R 5.296) on both faces, two runs stopped against the keystone.
  - `salon.arch.spandrel`: coursed stone on both faces from R 5.300 to R 7.030 (30 mm behind the vault), on the vault's
    stations.

### 3.3 Mouldings (A out of the host face, B up; `arc(cA, cB, rA, rB, θ0→θ1, n)`; metres)
- **P1 cornice** `salon.cornice` (`cornice.trough-lipped.470`), the long walls between piers and round both lunettes,
  on the vault's stations: (0.000, 5.625) (0.040, 5.625) (0.040, 5.600) (0.075, 5.600) (0.075, 5.800) (0.095, 5.800)
  (0.095, 5.825) arc(0.095, 5.905, 0.080, 0.080, −90→0, 8) (0.175, 5.925) (0.195, 5.925) (0.195, 5.945) (0.470, 5.945)
  (0.470, 6.150) (0.490, 6.150) (0.490, 6.180) arc(0.490, 6.280, 0.040, 0.100, −90→0, 8)~ arc(0.570, 6.280, 0.040,
  0.100, 180→90, 8) (0.585, 6.380) (0.585, 6.470) (0.510, 6.470) (0.510, 6.250) (0.000, 6.250) (0.000, 6.500). The lip
  hides the **trough** (0.510 × 0.220): cove strip at 0.255, z 6.270; framing track at 0.400 (§4). The recess A 0–0.040,
  B 5.600–5.625 is the **silk-top slot** (washer strip at 0.020, z 5.620).
- **P2 skirting** `salon.skirting` (`skirting.slotted.180`): (0.060, 0.000) (0.060, 0.160) (0.040, 0.180) (0.030, 0.180)
  (0.030, 0.150) (0.000, 0.150); bays and cabinet; dies into the pier bases. Its 30 mm recess is an unlit shadow slot.
- **P3 pier base**: (0.100, 0.000) (0.100, 0.160) (0.060, 0.160) arc(0.060, 0.200, 0.040, 0.040, −90→90, 12) (0.045,
  0.240) (0.030, 0.270) (0.030, 0.300) (0.000, 0.300).
- **P4 pier impost**: (0.000, 6.060) (0.020, 6.060) (0.020, 6.085) arc(0.020, 6.140, 0.055, 0.055, −90→0, 8) (0.075,
  6.160) (0.085, 6.160) (0.085, 6.400) (0.100, 6.400) (0.100, 6.500) (0.000, 6.500).
- **P5 door architrave** `salon.door.architrave` (`architrave.two-fascia.220`; A out from the opening's edge, B out of
  the wall), up the jamb, over the arch, down: (0.220, 0.000) (0.220, 0.090) (0.205, 0.090) arc(0.205, 0.070, 0.020,
  0.020, 90→180, 6) (0.160, 0.070) (0.160, 0.064) (0.100, 0.058) (0.100, 0.052) (0.020, 0.046) (0.000, 0.030) (0.000,
  0.000). On: the east door's and the oval door's bay faces; the cabinet door's two faces.
- **P7 archivolt** (`archivolt.bead-fascia-cavetto.175`; A radially out from R 5.296, B out of the arch face): (0.175,
  0.000) arc(0.175, 0.012, 0.022, 0.012, −90→−180, 6) (0.030, 0.012) (0.026, 0.004) arc(0.013, 0.004, 0.013, 0.010,
  0→180, 8) (0.000, 0.000).
- **P8 cabinet cornice** `salon.cabinet.cornice` (`cornice.slot-cove.330`), a closed loop on the ceiling's grid lines:
  (0.000, 5.125) (0.040, 5.125) (0.040, 5.100) (0.070, 5.100) (0.070, 5.180) (0.090, 5.180) (0.090, 5.200)
  arc(0.270, 5.200, 0.180, 0.180, 180→90, 12) (0.290, 5.380) (0.290, 5.420) (0.330, 5.420) (0.330, 5.500). Silk-top
  slot at 5.100–5.125.

### 3.4 Vault, coffers, lunettes, lanterns, roof
- **Barrel vault**, intrados centre (y 0, z 6.500), R 7.000, crown 13.500, 114 stations per semicircle; between arch
  faces or end walls. **Coffers** `salon.vault.coffer`: 19 rows of 9.4737° (1.1574 along the intrados), row 0 at the
  north springing; 8 columns per bay between plain bands `salon.vault.band` **0.7704** wide (sealed 4 mm behind the arch
  and end-wall faces); column pitch 1.1574 (bays 2–4, square), 1.2324 (bays 1, 5; 11.40 long). Section (inset from the
  rib line, depth into the vault): (0, 0) (0.100, 0) (0.115, 0.080) (0.175, 0.080) (0.190, 0.160) (0.240, 0.160)
  (0.280, 0.240): three steps to a panel 0.24 deep.
- **Rosettes** `salon.vault.rosette`: one per coffer on the deepest panel's centre, **750** (5 × 150), instanced;
  Ø 0.220: 12 outer petals r 0.022–0.112, 0.044 wide, domed 16 → 4 mm; 12 inner petals (offset 15°) r 0.020–0.078,
  0.034 wide, domed 26 → 12 mm; sunk midribs; boss r 0.021, 30 mm high, in a bead ring r 0.025.
- **Lunettes** `salon.lunette`: the end and far walls above 6.500 up to the vault (30 mm behind it).
- **Lanterns** (row 9, columns 3–4 of each bay; y −0.5781 → +0.5781, 1.156 wide):

| Bay | x (west → east) | Centre x | Inner pane at 13.580 | Glazed opening |
|---|---|---|---|---|
| 1 | −6.9324 → −4.4676 (2.465) | −5.700 | clear `salon.lantern.pane.clear` | `salon.bay1.lantern` |
| 2 | −19.1574 → −16.8426 (2.315) | −18.000 | flashed opal `salon.lantern.pane.opal` | `salon.bay2.lantern` |
| 3 | −31.1574 → −28.8426 | −30.000 | clear (under the pergola) | `salon.bay3.lantern` |
| 4 | −43.1574 → −40.8426 | −42.000 | clear + linen scrim `salon.lantern.scrim` | `salon.bay4.lantern` |
| 5 | −55.5324 → −53.0676 (2.465) | −54.300 | clear | `salon.bay5.lantern` |

  - Shaft `salon.lantern.shaft`: vertical walls from 4 mm inside the vault's face up to 14.050.
  - Grid `salon.lantern.grid` 13.510 → 13.580: perimeter 40 wide, one bar along x on the axis 30 wide, five cross bars
    at the sixths 30 wide. Bay 4: the linen stretched at **13.490**, sag 0.030·(1 − (2s−1)²)(1 − (2t−1)²), on a
    25 × 25 angle frame 13.465 → 13.494 (a fixed diffuser, not a blind: D-014).
  - **Outer lantern** (the weather glazing, the Glazed opening): kerb `salon.lantern.kerb` 0.15 wide, 14.050 → 14.200
    round the opening; two glass slopes at 30° to a ridge along x at **14.534**, glazed gables,
    `salon.lantern.outer.glass`; bars at the sixths, ridge and cap flashing `salon.lantern.outer.bars`.
- **Roof**: slab 13.850 → 14.050 over x −60.600 → +0.600, y ±7.600, flush with the outer faces (nothing past a port
  face); open only over the lanterns; top `salon.roof.paving`. The void over the vault closed from inside (Form.Sealed).
- **Bay 3's pergola** `salon.pergola` (on the roof, centred (−30.000, 0)): posts 70 square at x −33.000 / −27.000,
  y ±2.600, 14.050 → 16.390; beams along x at y ±2.600, 50 × 80, 16.390 → 16.470, x −33.150 → −26.850; 13 bars across
  every 0.50 (x −33.000 → −27.000), 28 × 30, top 16.500, y −2.720 → +2.720. **Planters** `salon.pergola.planter`
  0.70 square × 0.55, walls 60, centres (−32.600, ±2.200), (−27.400, ±2.200); soil 60 under the rim.

### 3.5 Cabinet ceiling
`salon.cabinet.ceiling`: flat at 5.500 inside P8 (0.330 off the walls), round a **laylight well** x −8.500 → −3.500,
y −14.100 → −9.100 (centred (−6.000, −11.600)), its sides 5.500 → 5.900. Laylight `salon.cabinet.laylight` at 5.900
(5.00 × 5.00), the diffuser of a luminous ceiling in the 0.20 void under the roof slab (§4).

### 3.6 Floors (z 0 finished)
- `salon.floor.parquet`: oak **Versailles** panels, 22 mm, in the bays' field x −59.400 → −0.600, y −6.600 → +6.600
  (cell edges x = −0.600 − 1.2k, y = −0.600 + 1.2k: centred on the axis, every pier face on a grid line; 9 × 11 panels
  a bay) and the cabinet's field x −9.600 → −2.400, y −15.200 → −8.000 (cell edges x = −6.000 + 1.2k,
  y = −11.600 + 1.2k; 6 × 6). Each panel: a mitred frame 85 wide; a lattice of 3 strips each way on the diagonals,
  70 wide, over and under in turn (pitch 0.2428 across the diagonal); diamond infills. **Border** of 100 mm boards,
  random lengths 1.2–2.4, parallel to the walls: 0.40 along the long walls, 0.60 at both end walls; 0.40 in the cabinet.
- `salon.floor.stone`: 30 mm travertine under every arch (x = pier ± 0.600, |y| ≤ 4.500). Door reveals: the joint's
  threshold (ports) and the Salon's own (the cabinet door reveal, `salon.floor.stone`).
- `salon.floor.strip`: 12 wide × 12 deep, 0.5 proud: x = pier ± 0.600 for |y| ≤ 4.500 (8); x 0.000 and x −60.000 for
  |y| ≤ 1.500; the cabinet door at y −7.000 and −7.600 for x −7.500 → −4.500.
- **Viewing stones** `salon.floor.viewing-stone` (disc r 0.400) in `salon.floor.viewing-stone.ring` (r 0.400–0.450),
  flush in the oak, 0.60 clear round each: bay 1 (−6.00, −4.00) (−5.70, +3.60); cabinet (−6.00, −11.60); bay 2
  (−18.00, 0.00) (−18.00, −3.60) (−16.00, +3.80) (−21.20, +4.60); bay 3 (−30.00, 0.00) (−30.00, −3.60) (−30.00, +3.60);
  bay 4 (−42.00, 0.00) (−42.00, −3.80) (−42.00, +3.80); bay 5 (−54.30, 0.00) (−54.30, −3.80) (−54.30, +3.80). **16.**
- Build-up 0.050, underside z −0.050, on the Reserve's top.

### 3.7 Fittings
- **Velvet benches ×8** (bays 2–5, centres §2 Anchors, long axis x; the axis clear): backless 2.40 × 0.60 × 0.44.
  Plinth `salon.bench.plinth` 75 high, set back 40, arrises r 4. Body `salon.bench.bay2.velvet`, `salon.bench.bay3.velvet`, `salon.bench.bay4.velvet`, `salon.bench.bay5.velvet`
  (the bay's colour): top edge rolled r 40, bottom r 14, ends r 40 in plan; flat top 0.428 quilted in 4 panels domed
  16; 4 mm piping round top and bottom and across each seam.
- **Glass stele** (`stele.glass.1200x2400`): `salon.stele.glass` 0.050 thick, mid-plane x −6.150, y +0.800 → +2.000,
  z 0.080 → 2.480, frameless. Foot `salon.stele.foot` x −6.350 → −5.950, y +0.700 → +2.100, 0.080 high, top edges
  eased 6. Impression, Sunrise on the east face; its back seen through the glass.
- **The 1874 case**, in the Manet cabinet (ruled: care, no daylight). Centre (−8.600, −8.450), long axis x, 1.60 × 0.70, 1.00
  high: x −9.400 → −7.800, y −8.800 → −8.100. Its back is 0.50 off the south silk, 0.60 off the west silk and 0.30 clear of the
  door's west jamb line (x −7.500). The reader stands on its north side (−y), facing south; it is clear of the Bar's wall
  zone and of the viewing stone at (−6.00, −11.60). Parts: `salon.case.carcase` plinth
  recessed 40 to 0.090; body to 0.800, its long faces framed and panelled (bottom rail 0.110–0.170, top rail
  0.730–0.790, stiles 0.06 wide at case-x −0.80, −0.03, +0.74, all 5 proud); top rail 0.800–0.860, 12 proud. Hood
  `salon.case.hood` 0.864 → 0.990, its twelve edges in 12 mm angles `salon.case.angles` (and an escutcheon on the
  reader's rail). Floor `salon.case.felt` at 0.864; Le Charivari's felt board 0.36 × 0.513 × 0.010 at case-x +0.28.
- **The Little Dancer's plinth** `salon.dancer.plinth` 0.80 × 0.80 × 1.00 at (−20.500, +3.400); vitrine
  `salon.dancer.vitrine` 0.76 × 0.76, 1.00 → 2.10.
- **Frames** `salon.frame.gilt` (§8). No wall labels (captions on look).

## 4. Light

**Daylight** (Lighting.json `daylight`; no blinds, D-014)

| Opening | Glazing (outer) | Weather | Control | Below it |
|---|---|---|---|---|
| `salon.bay1.lantern` | `glass.low-iron`, laminated UV-filtering | **giverny** | none | clear pane: a sharp sun patch, "plein air" |
| `salon.bay2.lantern` | same | **giverny** | none | flashed opal: the sun stopped, "Paris under cloud" |
| `salon.bay3.lantern` | same | **giverny** | none | clear, under the vine canopy: dappled sun |
| `salon.bay4.lantern` | same | **giverny** | none | clear + linen scrim: shadowless, "a curtained room" |
| `salon.bay5.lantern` | same | **giverny** | none | clear: the patch walks with the hour |

Under the idealised sun (elevation 71.49°) a crown patch lands ≤ 4.54 from under its lantern: it never reaches the silk
of the long walls, and reaches the end and far walls only below z 0.23 (06:00, 18:00); no work, the stele or the case is
ever in it. Under a real city's lower sun the clear lanterns (1, 3, 5) can reach the silk: Art.WithinCare answers by
the hang only. The cabinet has no daylight.

**Luminaires** (Lighting.json; 74; no output values are set here)

| Ids | Type | Count / extent | Mount and aim | Operation |
|---|---|---|---|---|
| `cove.bay1.north` … `cove.bay5.south`, `cove.bay1.east`, `cove.bay5.west` | `linear-cove.3000k` | 12 runs (≈ 134 m): each long wall of each bay from pier face + 0.12 (or the end-wall corner) to the next; both lunettes y ±6.745 | in the P1 trough, 0.255 off the silk, z 6.270; aim up onto the vault (y ±5.362, z 11.00; lunettes (−2.50/−57.50, 0, 12.00)) | dusk |
| `wash.bay1.north` … `wash.bay5.west`, `wash.cabinet.*` | `linear-cove.3000k` (a strip in the slot) | 12 runs in the bays + 4 in the cabinet | in the silk-top slot, 0.020 off the silk, z 5.620 (cabinet 5.120); aim down the silk to z 2.50 | presence |
| `work.<id>` (`.a`/`.b` for works over 2.00 wide) | `track-framing.3000k.medium` | 39: one per canvas in the bays and on the stele (Rouen 5), two each for Women in the Garden, Bazille and Paris Street | on a bronze track in the **opposite** cornice's trough, 0.400 off the silk (y ±6.600), head on a stem at z 6.650 (beam clears the lip's 6.47); aim at the canvas's centre at y ∓6.950, framed to the canvas. Bazille: two heads on the east face of pier 1's north impost (−11.350, −5.60/−6.00, 6.620). Impression, Sunrise: bay 1's end-wall cornice (−0.400, +1.400, 6.650) | presence |
| `work.manet-*` | `track-framing.3000k.small` | 4 (Luncheon ×2) | ceiling track 1.20 off the hung wall at z 5.400 (y −14.400; x −3.200; x −8.800); aim at the canvas centre | presence |
| `work.salon-1874-case` | `track-framing.3000k.small` | 1 | cabinet ceiling (−8.600, −9.900, 5.400) → the documents (−8.600, −8.450, 0.870) | presence |
| `work.degas-little-dancer` | `track-framing.3000k.small` | 1 | south cornice (−19.000, +6.600, 6.650) → the dancer (−20.500, +3.400, 1.500) | presence |
| `cabinet.laylight` | `gap.luminous-ceiling.3000k` | 1, 5.00 × 5.00 | LED modules in the void over the laylight (−6.000, −11.600, 6.000); aim down | presence |

Removed as-built fakes, not re-expressed: floating accent spots 4.5 m over each work, floating wall-washers, rect
lights standing in for the opal, muslin and laylight, sky "veil" panels in the shafts, the emissive laylight, the
Salon's post-process zone, the 650 lx painting level (gallery_lights.py:34-53).

**Targets**

| Surface | Class | Lux | Source |
|---|---|---|---|
| `salon.bay1.silk` … `salon.bay5.silk`, `salon.cabinet.silk` (measured on the works' faces) | oil-and-tempera | 150–200 | Care |
| `salon.case.felt` (the catalogue / Le Charivari) | works-on-paper / works-on-paper.fugitive | 50 / 50 | Care |
| `salon.dancer.plinth` (measured on the dancer) | textiles.silk | 50 | Care |
| `salon.floor.parquet`, `salon.floor.stone` | circulation / circulation.night | 100 / 50 | UseTargets |

**Design** (Lighting.json `design`; measured at every reference moment, never tuned; the Light board's order, section
through bay 2): lantern pane > works > silk > stone > vault. The works 2.5–4.0 × the silk between them (the silk
"about a third"), per bay and in the cabinet; silk ≥ `salon.pier` (the stone lit only by the room); `salon.pier` ≥
`salon.vault.coffer` (the vault in half-light, cove at half, coffers in shade; the rosettes read as points of light).
Cabinet: laylight > works > silk. The oak shows no image of a lantern brighter than the works (checked, not a ratio).

## 5. Time and weather
- The five lanterns take **giverny**'s weather (D-005; Monet's garden, 49.0757 N, 1.5337 E; SalonSky.cpp:29-31): cloud
  attenuates the museum's sun and sky through each lantern; rain and wetness on the outer glass; the rain's sound.
- The roof, kerbs, pergola, planters and outer faces: **museum** weather. Sun and hour are always the museum's (D-003).
- Bay 3's vines leaf out about Qingming and fall about Shuangjiang (museum solar terms), so the dapple goes in winter.

## 6. Motion
None: no mechanism in the Salon.

## 7. Nature
- **Water:** none (the pond is `nympheas`'s).
- **Vines** (Planting.json): *Vitis vinifera* ×4, one in each planter `salon.pergola.planter.nw`, `salon.pergola.planter.ne`, `salon.pergola.planter.sw`, `salon.pergola.planter.se`, trained
  up the nearest post, along the beams and out along the bars; leaves in clusters with holes between (the dapple over
  bay 3's lantern). Growth space z 16.39–16.75 over the pergola. Outdoor phenology, museum wind.

## 8. Art
**Hang rule** (Hang.swift:15-17): centre at **1.55**, or **0.60 + h/2** when that would put the bottom under 0.60;
frame's back on the silk; neighbours ≥ 0.04 apart; true size. Every oil framed in the carved gilt frame `salon.frame.gilt` (Hang `frame`; finish
`salon.frame.gilt`; moulding 0.09 + 0.015 × the longer side, clamped 0.10–0.14, MuseeFrame.cpp:1031). Support canvas;
care oil-and-tempera. **Source** `data/artworks.json` (ids verbatim) unless noted; **sizes** from `Shared/Plan/Hang.swift`
(the SalonHang board; the catalogue has none). Hang `along` = left edge on the anchor's x; `height` = bottom.

| Anchor | Work (id) · title · artist | w × h | Centre (x, y, z) | along · height |
|---|---|---|---|---|
| `salon.bay1.stele.east` | monet-impression-sunrise · Impression, Sunrise · Monet | 0.63 × 0.48 | (−6.062, +1.400, 1.550) | 0.285 · 1.310 |
| `salon.bay1.wall.east` | bazille-family-reunion · Family Reunion · Bazille | 2.30 × 1.52 | (0.000, −4.000, 1.550) | 1.850 · 0.790 |
| `salon.bay1.wall.south` | monet-women-in-the-garden · Women in the Garden · Monet | 2.05 × 2.55 | (−5.700, +7.000, 1.875) | 4.675 · 0.600 |
| `salon.bay1.wall.south` | sisley-flood-port-marly · Flood at Port-Marly · Sisley | 0.81 × 0.60 | (−2.200, +7.000, 1.550) | 1.795 · 1.250 |
| `salon.bay1.wall.south` | pissarro-hoarfrost · Hoarfrost · Pissarro | 0.93 × 0.65 | (−9.200, +7.000, 1.550) | 8.735 · 1.225 |
| `salon.bay1.wall.north` | monet-the-magpie · The Magpie · Monet | 1.30 × 0.89 | (−2.400, −7.000, 1.550) | 8.350 · 1.105 |
| `salon.bay1.wall.north` | monet-terrace-sainte-adresse · Garden at Sainte-Adresse · Monet | 1.30 × 0.98 | (−9.400, −7.000, 1.550) | 1.350 · 1.060 |
| `salon.cabinet.wall.north` | manet-luncheon-on-the-grass · Luncheon on the Grass · Manet | 2.65 × 2.08 | (−6.000, −15.600, 1.640) | 2.675 · 0.600 |
| `salon.cabinet.wall.east` | manet-olympia · Olympia · Manet | 1.90 × 1.30 | (−2.000, −11.600, 1.550) | 3.050 · 0.900 |
| `salon.cabinet.wall.west` | manet-bar-folies-bergere · A Bar at the Folies-Bergère · Manet | 1.30 × 0.96 | (−10.000, −11.600, 1.550) | 3.350 · 1.070 |
| `salon.bay2.wall.north` | pissarro-boulevard-montmartre-winter · Boulevard Montmartre, Winter Morning · Pissarro | 0.81 × 0.65 | (−14.600, −7.000, 1.550) | 8.395 · 1.225 |
| `salon.bay2.wall.north` | caillebotte-paris-street-rainy-day · Paris Street; Rainy Day · Caillebotte | 2.76 × 2.12 | (−18.000, −7.000, 1.660) | 4.020 · 0.600 |
| `salon.bay2.wall.north` | pissarro-boulevard-montmartre-night · Boulevard Montmartre at Night · Pissarro | 0.65 × 0.53 | (−21.400, −7.000, 1.550) | 1.675 · 1.285 |
| `salon.bay2.wall.south` | monet-gare-saint-lazare · The Saint-Lazare Station · Monet | 1.04 × 0.75 | (−14.200, +7.000, 1.550) | 1.080 · 1.175 |
| `salon.bay2.wall.south` | caillebotte-floor-scrapers · The Floor Scrapers · Caillebotte | 1.47 × 1.02 | (−16.800, +7.000, 1.550) | 3.465 · 1.040 |
| `salon.bay2.wall.south` | degas-absinthe · L'Absinthe · Degas | 0.69 × 0.92 | (−19.400, +7.000, 1.550) | 6.455 · 1.090 |
| `salon.bay2.wall.south` | degas-dance-class · The Dance Class · Degas | 0.77 × 0.84 | (−21.600, +7.000, 1.550) | 8.615 · 1.130 |
| `salon.bay3.wall.north` | monet-la-grenouillere · La Grenouillère · Monet | 1.00 × 0.75 | (−26.800, −7.000, 1.550) | 8.100 · 1.175 |
| `salon.bay3.wall.north` | renoir-boating-party · Luncheon of the Boating Party · Renoir | 1.73 × 1.30 | (−30.000, −7.000, 1.550) | 4.535 · 0.900 |
| `salon.bay3.wall.north` | monet-woman-with-a-parasol · Woman with a Parasol · Monet | 0.81 × 1.00 | (−33.200, −7.000, 1.550) | 1.795 · 1.050 |
| `salon.bay3.wall.south` | renoir-dance-at-bougival · Dance at Bougival · Renoir | 0.98 × 1.82 | (−26.800, +7.000, 1.550) | 1.710 · 0.640 |
| `salon.bay3.wall.south` | renoir-bal-du-moulin · Bal du moulin de la Galette · Renoir | 1.75 × 1.31 | (−30.000, +7.000, 1.550) | 4.525 · 0.895 |
| `salon.bay3.wall.south` | monet-poppy-field · Poppy Field · Monet | 0.65 × 0.50 | (−33.200, +7.000, 1.550) | 8.275 · 1.300 |
| `salon.bay4.wall.north` | morisot-harbor-lorient · The Harbor at Lorient · Morisot | 0.73 × 0.44 | (−37.900, −7.000, 1.550) | 9.135 · 1.330 |
| `salon.bay4.wall.north` | morisot-summers-day · Summer's Day · Morisot | 0.75 × 0.46 | (−39.600, −7.000, 1.550) | 7.425 · 1.320 |
| `salon.bay4.wall.north` | morisot-the-cradle · The Cradle · Morisot | 0.46 × 0.56 | (−42.000, −7.000, 1.550) | 5.170 · 1.270 |
| `salon.bay4.wall.north` | morisot-dining-room · In the Dining Room · Morisot | 0.50 × 0.61 | (−44.200, −7.000, 1.550) | 2.950 · 1.245 |
| `salon.bay4.wall.north` | cassatt-childs-bath · The Child's Bath · Cassatt | 0.66 × 1.00 | (−46.000, −7.000, 1.550) | 1.070 · 1.050 |
| `salon.bay4.wall.south` | renoir-la-loge · La Loge · Renoir | 0.64 × 0.80 | (−38.400, +7.000, 1.550) | 1.480 · 1.150 |
| `salon.bay4.wall.south` | cassatt-in-the-loge · In the Loge · Cassatt | 0.66 × 0.81 | (−39.600, +7.000, 1.550) | 2.670 · 1.145 |
| `salon.bay4.wall.south` | cassatt-blue-armchair · Little Girl in a Blue Armchair · Cassatt | 1.30 × 0.90 | (−42.400, +7.000, 1.550) | 5.150 · 1.100 |
| `salon.bay4.wall.south` | cassatt-boating-party · The Boating Party · Cassatt | 1.17 × 0.90 | (−45.400, +7.000, 1.550) | 8.215 · 1.100 |
| `salon.bay5.wall.south` | monet-stacks-of-wheat · Stacks of Wheat (End of Summer) · Monet | 1.00 × 0.60 | (−52.000, +7.000, 1.550) | 2.900 · 1.250 |
| `salon.bay5.wall.south` | monet-bridge-water-lilies · Bridge over a Pond of Water Lilies · Monet | 0.74 × 0.93 | (−56.600, +7.000, 1.550) | 7.630 · 1.085 |
| `salon.bay5.wall.north` | monet-rouen-cathedral · Rouen Cathedral series · Monet: five canvases `-1` … `-5` (images assets/paintings/monet-rouen-cathedral-N.jpg) | 0.65 × 1.00 each | x −52.100, −53.200, −54.300, −55.400, −56.500; y −7.000; z 1.550 | 7.575, 6.475, 5.375, 4.275, 3.175 · 1.050 |
| `salon.cabinet.case` | salon-1874-case (`assets/collection.json`) · The first exhibition, April 1874: the catalogue (pp. 14–15, open, flat at case-x −0.40) and Le Charivari 25 April 1874 p. 2 (on the felt board at case-x +0.28) | 0.250 × 0.184; 0.320 × 0.473 (assets/salon/manifest.json) | (−9.000, −8.450, 0.866); (−8.320, −8.450, 0.875) | support paper; frame none; care works-on-paper / works-on-paper.fugitive |
| `salon.bay2.dancer` | degas-little-dancer · Little Dancer Aged Fourteen · Degas (the Met's posthumous bronze with cotton tulle skirt and silk-satin ribbon; scan `assets/sculptures/degas-little-dancer.mvm`, CC BY 4.0, credit required) | h 0.994 (data/sculptures.json) | feet (−20.500, +3.400, 1.000), facing +x | frame none; care textiles.silk; the caption names the NGA wax as the original |

36 catalogue works (39 canvases incl. Rouen's five), the case's two documents and the dancer. Not hung here: the Water
Lilies' first room (`monet-water-lilies-orangerie`, retired) and the willows (`nympheas`); the thirteen reserve works
(MuseePlan.h:431). Captions on look (IMuseeCaptioned) from `data/artworks.json` and `assets/collection.json`.

## 9. Sound
- **Acoustics** (from Matter, never tuned): `salon.bays` ≈ 10,000 m³ of stone and plaster, the silk on felt and the
  velvet absorbing mids and highs: a long, warm tail; `salon.cabinet` ≈ 350 m³, silk-lined, the driest room.
  Occlusion through the three door openings.
- **Emitters:** rain on each outer lantern's glass, driven by the **giverny** observer's rain (SalonRain.h:8-39: a
  procedural hiss with ticking drops); footfalls by substance (oiled oak; honed travertine; bronze strips). No others.

## 10. Visitor
- **Walk:** the axis stays clear from `salon.east` to `salon.west` (the stele stands clear of it in bay 1; the benches
  beside it, not on it); through the cabinet door into the cabinet. **Step-to:** the 16 viewing stones.
- **Sit:** the eight benches (seat 0.44; two frames per face at ±0.60, facing either wall).
- **Look:** captions on every work, the case's documents in the cabinet (read from its north side, −y) and the dancer. No use.
- **Eye:** walking west, oxblood, zinc, sage, old rose, haystack on the piers, and the white oval at the end: an
  adaptation walk the Eye owns; the stage sets nothing for it.

## 11. The old-code map
Paths relative to `windows\` unless marked. Re-express, never copy. Museum x = local − 14.

| Part | Where |
|---|---|
| Salon constants: end/far walls, halves, springing, vault R, intrados, piers, bays | Source/MuseeVision/Salon/SalonStructure.cpp:27-39 |
| Doors (3.0, springing 3.3, 64 segs), skirting ends, cabinet door, silk heights, light slots | …/SalonStructure.cpp:41-52 |
| Vault rows, lantern row/columns, roof 13.85/14.05, glass 13.58, grid, coffer section | …/SalonStructure.cpp:55-60 |
| Cabinet constants; laylight | …/SalonStructure.cpp:63-65 |
| Interior update: pier silk, voussoirs, keystone, V-joint 0.012, strips, seals, muslin, pergola | …/SalonStructure.cpp:77-91 |
| Vault helpers, coffer pitch, plain band, column x, door stations | …/SalonStructure.cpp:106-134 |
| WallFace, Tunnel (reveals), DoorFrames (architraves) | …/SalonStructure.cpp:164-282 |
| P1, P2, P3, P4, P5, P8 profiles (the unused `Archivolt()` :421-428 is not ported) | …/SalonStructure.cpp:380-419, 431-438, 452-459 |
| Pier silk; P7 archivolt; voussoirs and keystone | …/SalonStructure.cpp:631-640, 657-667, 675-799 |
| Bays: silk walls, piers, bases, imposts, spandrels | …/SalonStructure.cpp:802-857 |
| Coffer; rosette | …/SalonStructure.cpp:860-882, 889-937 |
| Lanterns (rect, shaft, panes, opal, muslin, grid); lunettes | …/SalonStructure.cpp:946-1057 |
| Pergola, planters, vines | …/SalonStructure.cpp:1066-1201 |
| Cornice runs; skirtings | …/SalonStructure.cpp:1204-1266 |
| Envelope: outer faces, east door, cabinet door, oval door and P5, roof, sealed void | …/SalonStructure.cpp:1269-1342 |
| Cabinet: silk, P8, skirting, ceiling, laylight well, outer faces, roof | …/SalonStructure.cpp:1345-1396 |
| Parquet zones; floor strips; floor grid; viewing stones' rings | …/SalonStructure.cpp:1520-1551, 1558-1623, 1652-1735 |
| Stele foot; light slots (cove, silk top, silk base) | …/SalonStructure.cpp:1739-1752, 1766-1840 |
| Viewing-stone centres; as-built materials | …/SalonStructure.cpp:1921-1927, 1883-1918 |
| Benches; 1874 case (sizes, carcase, hood, documents) | Source/MuseeVision/Salon/SalonInterior.cpp:64-114, 300-378; SalonInterior.h:56-98 |
| Silk colours; parquet pattern and its widths; case documents' sizes | Scripts/salon_interior.py:47, 337-462, 478-481, 1023-1044; repo assets/salon/manifest.json |
| Giverny weather; rain sound | Source/MuseeVision/Salon/SalonSky.h:13-28, SalonSky.cpp:29-31; SalonRain.h:8-39 |
| Old Salon lights (not ported: floating spots, 650 lx, washers) | Scripts/gallery_lights.py:24-53, 144-245 |
| Frame widths (SalonGilt) | Source/MuseeVision/Frames/MuseeFrame.cpp:1031 |
| Rotunda's west door (class); passage end −13.4; the Reserve's top under the Salon | Source/MuseeVision/Plan/MuseePlan.h:44-47, 61-63, 382-384 |
| Exterior dress (polish): Salon order D 1.10, hyphen wall \|y\| 5.5, portico x −40 … −20 local | …/Plan/MuseePlan.h:518-561 |
| Hang (positions, sizes, centre rule); Plan.Salon (stele, dancer plinth, cabinet); stele glass, dancer plinth and vitrine | repo Shared/Plan/Hang.swift:15-17, 48-98; Shared/Plan/MuseumPlan.swift:39-107; Shared/Wings/Salon.swift:261-283 |
| Boards | repo plan/proposals/salon-interior/canvas/{Main,Plan,Light,Details}.dc.html |

**Corrections to the draft** (`r-specs/…/Stages/salon.md`):
1. **Scope (D-019): moved out to `nympheas`** — the oval shell (wall, P6 slim architrave, P9 cove, P10 skirting, the
   passage tunnel and passage block, outer face, roof ring), the velarium and its glass dome, the pond (coping, frieze,
   lip, vessel, tray, posts, guard rails), M-1/M-2, the golden lily (now the golden iris), the lilies and marginals, the
   stair and well (flight, landings, strings, curb, shaft walls, head wall, soffit, handrails, downlights), the willows,
   the four oval benches, the two oval viewing stones, lights L8–L11, the enclosures `salon.oval` and `salon.stair`, the
   port `salon.stair` and its served well, the stair-foot plate. Added: port `salon.west` (mates `nympheas.east`).
2. `salon.east` is `doorway.3000x6000.arched` (Architect's ruling; draft `doorway.3000x4800.arched`), with the step
   inside the Salon's own wall (§2).
3. Materials are palette keys (D-018, Story): vault and coffers **lime plaster run** (draft fibrous-plaster panels);
   lantern grid **patinated bronze** (draft painted steel); strips and stele foot **bronze brushed** (draft brass); bay 4
   **linen** scrim (draft cotton muslin); floor under the arches **honed, filled** (draft polished); frames **oak,
   water-gilt** (draft "wood.gilt"); cabinet laylight **etched low-iron** (draft flashed opal).
4. Lighting: the draft's ≈ 110 coffer-recessed spots (L3–L5, L7, "Scheme decides", 4000 K) become **45 framing
   projectors** listed one by one (§4), across the room from the opposite cornice's trough (throw ≈ 14.5 m, ≈ 20° below
   the horizontal, no fixture near the art, nothing hung from the vault); the silk washer is a strip in the slot (draft
   `linear-slot.3000k`, a recessed opal channel that cannot graze); the laylight is a catalogue gap. All 3000 K.
5. Viewing stones: **16** here (draft 18, with the oval's two).
6. Verified unchanged: frame and origin (museum −14), ports' faces (−13.40, −74.60), bays, piers, arches (15 × 12°,
   R 4.5/5.3, key top 12.30, V 12 mm: the code's comment says 8 mm, its constant 0.012), vault (R 7, 19 rows, bands
   0.7704, pitches 1.1574/1.2324), lanterns (x, y ±0.5781, glass 13.58, grid, muslin 13.49/0.03), roof 13.85/14.05,
   cabinet (x −10/−2, y −15.6/−7.6, 5.5, laylight 5 × 5 at 5.9), every profile point list, parquet (fields, anchors,
   85/70/100 mm, lattice 3), strips, viewing stones, benches, stele, case, dancer, pergola, silk colours, and all 36 hang
   positions and sizes against Hang.swift.

## 12. The first-pass cut
**Tonight:** Site (ports with the east step, rough openings, blinds on the stand, footprint, enclosures); walls, silk
and pier silk; piers, bases, imposts; the four arches (voussoirs, keystones, archivolts, spandrels); the vault with
coffers and the 750 rosettes (instanced); lunettes; P1 cornices, P2 skirtings, P5 architraves; the five lanterns (shafts,
inner panes, grids, bay 2's opal, bay 4's linen, the outer glazing and kerbs); roof slab; the cabinet (silk, P8,
ceiling, laylight well, laylight glass, roof); floors (Versailles parquet as pieces, stone under the arches, strips,
16 viewing stones); benches, stele, case, dancer plinth and vitrine; the whole hang and the dancer; the luminaires of §4
and daylight with Giverny's cloud; rain on the lanterns; footfalls.

**Waits for polish:** the pergola's vines if Nature has no *Vitis* tonight (the frame and planters are built); the
exterior dress (podium, giant Ionic order D 1.10, blind windows, balustrades, the south portico, the hyphen wall); the
luminous-ceiling product; each voussoir's own tone beyond Matter's defaults; the parquet's worn lane; falls on the roof.

## 13. Palette keys to confirm and catalogue gaps
**Palette keys** (no `todo.` keys; roles guessed or workings outside `workingsByEra`):
- `white.lime-plaster` trowelled for the **lunettes, lantern shafts and cabinet ceiling** (roles name the vault only).
- `white.fibrous-plaster` cast for the **cabinet cornice P8** (roles: "Salon: cornice with the cove").
- `glass.low-iron` **acid-etched** for the **cabinet laylight** (the classical working names laylights; the roles name
  the Classical Hall's).
- `accent.wool-felt` for the **1874 case's bed** (roles: the Reserve's mats, Albion's baize).
- `stone.travertine-romano` honed, weathered for the **roof paving, lantern kerbs and planters** (from "Grounds: façade").
- `bronze.cusn10` **cast, weathered** for the **outer lantern bars and bay 3's pergola** (the outdoors working); patinated
  for the **case's angles and both blind-door pairs** (by analogy).
- `stone.travertine-romano` for the **dancer's plinth**; `glass.low-iron` for her **vitrine** (not in roles).
- Workings written as compound strings: `oiled.hard-waxed`, `fumed.waxed`, `flashed-opal`, `toughened.laminated`,
  `anti-reflective.laminated`, `toughened.anti-reflective`, `upholstered`. Silk history `new` (no light-fading history yet).
- `salon.frame.gilt` is declared here for Art's frames (oak, water gilt); confirm Art takes its finish from the stage.

**Catalogue gaps** (Light): `gap.luminous-ceiling.3000k`: the cabinet's luminous ceiling, an LED backlight module field
(e.g. a museum laylight system) in the 0.20 void over the etched glass, dimmable, 3000 K, CRI > 90.

**Conflicts and questions for the Architect:**
(a) **Care (ruled).** The 1874 case is moved into the Manet cabinet (no daylight). The Little Dancer stays in bay 2;
if Light's measurement shows bright days over 50 lx on her, she moves to the cabinet.
(b) The Details board draws the cornice at 6.28–6.62; as built P1 runs 5.60–6.47 (kept, D-017).
(c) The Light board calls bay 4's diffuser "a linen blind"; it is a fixed linen scrim (D-014), palette `accent.linen`.
(d) The palette's "viewing stones (18)" counts the oval's two, now `nympheas`'s.

## Build notes
- **Frame.** The origin is on the axis at bay 1's east silk face, on the floor, at museum (−14.00, 0, 0), yaw 0.
- **Scope.** The five bays and the Manet cabinet only. Everything beyond bay 5's far wall is `nympheas`.
- **The east port steps down inside the Salon's own wall.**
  - From x +0.60 to +0.30: the 3000x6000 rough opening.
  - At x +0.30: the step face `salon.east.step`.
  - From x +0.30 to 0.00: the Salon's own 3.00 × 4.80 door.
  - The jambs run continuous, and the cornice at 5.60 stays whole.
- **The west port.** Build only the rough opening through the far wall (x −60.00 → −60.60). The oval owns the
  passage block and tunnel.
- **The cabinet** opens off bay 1's north wall through a 3.00 × 4.80 door at x −6.00. It has no daylight, and the 1874
  case stands in it (anchor `salon.cabinet.case`).
- **Nothing moves.** There is no Mechanisms.json.
- **Daylight.** The lanterns take Giverny's weather. Bay 4's diffuser is a fixed linen scrim, not a blind.
- **Hang.** The framing projectors stand in the opposite cornice's trough, on stems at z 6.65.
  - The Rouen series has one Hang entry per canvas.
  - The Little Dancer's width comes from her scan.
- **Planting.** The pergola vines on the roof are outdoors, so they take the museum's weather.

**Departures from as-built** (for the owner's review):
- The **east door steps down inside the Salon's own wall**, from the Rotunda's 6.00 crown to the Salon's 4.80 arch (ruled). As built, the drum's arch ran through the passage.
- The **1874 case moves from bay 1 into the Manet cabinet**, where there is no daylight (ruled, care).
- Bay 4's diffuser is a **fixed linen scrim**; the board shows a linen blind, and D-014 allows no blinds.
- Materials follow the Story: the vault is run lime plaster (not fibrous panels), the lantern grid is patinated bronze (not painted steel), the strips and stele foot are brushed bronze (not brass), and the frames are oak with water gilding.
- Light: 45 listed framing projectors at 3000 K replace the ≈ 110 coffer spots at 4000 K.
