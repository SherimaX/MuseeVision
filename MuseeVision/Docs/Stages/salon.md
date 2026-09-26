# Stage · Salon Impression (`salon`)

The design sheet the Room Builder builds `Source/Stages/Rooms/Salon` from. It covers the five skylit bays with their five lantern lights, the Manet cabinet, the Nymphéas oval with the Orangerie's second room (the willows), and the lily pond that lifts 2.6 m on bronze posts, with its new edge. It also covers the stair well that the pond serves, down to the Reserve.

**Sources, in order of authority:**
1. D-001 to D-016.
2. The owner's standing decisions (the memory notes `design-principles`, `expansion-2026-09`).
3. The approved proposal `plan/proposals/salon-interior` (Main, Plan, Light, Oval and Details boards).
4. The master boards `Salon`, `SalonPlan`, `SalonHang`, `SalonDetails` and `SalonReserve`.
5. The as-built code in `windows/`, read-only, which holds the exact numbers. §11 maps every part.

Where the as-built code faked something, this sheet re-expresses it as a physically real piece and says so, with a ⚑ mark. The fakes were an emissive velarium, an emissive laylight, sky "veil" panels, floating lights, invisible guards, a teleporting push-out and a post-process exposure zone. Every number is in metres, in the stage's local frame, unless a unit says otherwise.

---

## 1 · Identity and placement

| | |
|---|---|
| Stage key | `salon` (module `MuseeSalon`, class `USalonRoom : USiteRoom`) |
| Display name | "Salon Impression" (FText) |
| Frame | x east, y south, z up, true north. Site only translates it. |
| Origin | The axis (y 0) on the **silk face of Bay 1's east end wall**, on the finished floor. This is the boards' "salon data 0", from which the SalonPlan and SalonHang boards measure. |
| Placement | The origin sits at **museum plan (x −14.00, y 0.00), z 0.00**, the ground floor, level with the Rotunda's finished floor. local = museum − (−14, 0, 0). |
| Level | 0 (ground floor), everywhere above grade. The stair well descends to z −5.80, the Reserve's floor. |
| Footprint kind | **Building** |

**Footprint polygon** (plan, local, clockwise from the north-east corner of the bays):

1. The bays: (+0.60, −7.60) → (+0.60, +7.60) → (−60.60, +7.60) → (−60.60, +2.10).
2. The passage block: west along y +2.10 to the oval's outer face at (−61.293, +2.10).
3. The oval: round the oval's outer face, the 0.60 m offset curve of the ellipse a 11.00 × b 7.50 about (−72.50, 0). It passes (−72.50, +8.10), the west apex (−84.10, 0) and (−72.50, −8.10), to (−61.293, −2.10).
4. The passage block's north side: → (−60.60, −2.10) → (−60.60, −7.60).
5. The cabinet: along the Salon's north outer face to (−10.40, −7.60) → (−10.40, −16.00) → (−1.60, −16.00) → (−1.60, −7.60) → (+0.60, −7.60).

**Vertical extent:**
- Bays and cabinet: z −0.05 to 14.05. The pergola on bay 3's roof reaches 16.52, and its leaves ≈ 16.8.
- Oval: z −0.40 to 7.80. The oval slab is 0.40 thick over the shaft.
- Served well (below grade, §2): z −6.15 to 0.00.

Directly below the bays lies the Reserve, whose top is at z −0.05, 5 cm under the Salon's floor finish. Site.NoOverlap must therefore compare footprints as volumes (see the risks).

**Enclosures:**

| Key | Kind | What it bounds |
|---|---|---|
| `salon.bays` | Enclosed | The five bays under one vault. It includes the lantern shafts up to their outer glazing, and the reveal of the east port. |
| `salon.cabinet` | Enclosed | The Manet cabinet. It includes its door reveal and the laylight well. |
| `salon.oval` | Enclosed | The Nymphéas oval. It includes the passage from Bay 5, the attic between the velarium and the glass roof, and the pond (in every state). Its only opening to the sky is Glazed (the roof over the velarium). |
| `salon.stair` | Enclosed | The long stair shaft. It opens into `salon.oval` through the pond opening, which the pond closes when it is down, and into the Reserve through `salon.stair` (the port). |

The pergola, the planters and the roof surfaces are exterior pieces on the shell's outer face; no enclosure contains them.

---

## 2 · Ports (D-012 topology)

D-012: the Rotunda's **west** port mates the Salon's east port. The **Salon's stair port** mates the Reserve's. The well between them is served by the Salon, and its lifting pond is the well's lid. The Salon has no other port. The portico's bronze door on the south front is blind; it belongs to the exterior dress (see §12).

| Key | Kind | Class key | Local frame (outer face, threshold, facing out) | Face | Wall depth | Mates | Closure |
|---|---|---|---|---|---|---|---|
| `salon.east` | Doorway | `doorway.3000x4800.arched` | origin (+0.600, 0.000, 0.000); facing +x (east); up +z | flat | 0.60 (Bay 1's end wall) | `rotunda.west`. Passage **2.20 m** on the axis, to the drum's outer face (cylindrical, R 11.20 about the Rotunda's centre, museum (0, 0)). | open portal |
| `salon.stair` | Stairwell | `stairwell.straight.2400x3200.arched` | origin (−60.600, 0.000, −5.800); facing +x (east); up +z | flat | 0.40 (the shaft's brick head wall) | `reserve.stair`, the Reserve's arched west door in its 0.60 m west wall. Passage **0.00** (the faces touch). | open portal |

**Class definitions** (for `Data/Site/PortClasses.json`; the allowances are Site's):
- `doorway.3000x4800.arched`: clear width 3.000; semicircular head of radius 1.500 springing at 3.300; crown 4.800. This is the Salon's door as built (Plan.Salon.door; SalonStructure.cpp:41).
- `stairwell.straight.2400x3200.arched`: clear width 2.400; semicircular head of radius 1.200 springing at 2.000; crown 3.200 above the landing. This is the Reserve's west door as built (ReserveStructure.cpp:37-38).

**The rough-opening rule:**
- At `salon.east` the Salon builds the rough opening through its 0.60 m end wall.
- At `salon.stair` it builds the rough opening through a 0.40 m brick head wall, x −61.00 to −60.60, that closes the shaft's section down to the class.
- It builds nothing past either port face.

The joint draws the rest: the travertine threshold through both wall depths, the brushed-bronze strip on the mating plane, the linings and the surrounds. The strip lies on x +0.60 (museum −13.40, as the as-built strip at −13.43) and on x −60.60 at z −5.80.

**The well (served by the Salon):**

| | |
|---|---|
| Prism, including the shaft walls | x −76.80 to −60.60, y −1.60 to +1.60, z −6.15 to 0.00 |
| Clear stair void | x −76.40 to −60.60, y −1.20 to +1.20, z −5.80 to 0.00. East of x −68.90 the soffit is at z −0.40, the underside of the oval slab. |
| Top opening in the oval floor | x −76.40 to −68.90, y ±1.20. Its lid is the pond (§6). |
| Serving side | `salon` owns everything in the prism: the stair, the strings, the handrails, the shaft walls, the curb, the soffit, the head wall and the pond's post sleeves. |
| Receiving side | `reserve` reserves the prism as a void. It lies wholly west of the Reserve's west wall, so the Reserve builds nothing west of x −60.60. |

The as-built bottom landing ran 0.90 m on through the Reserve's wall into a floor notch (to museum x −73.7). That notch is **dropped**: the joint's threshold now crosses the head wall and the Reserve's wall.

**On the stand:** both ports get blinds, and `salon.stair` gets a blind well continuing the shaft to z −5.80, so the stair and the pond run their full states alone.

---

## 3 · Pieces by trade

Material keys are written `substance.working.history`; §3.12 maps each to Matter's records. The builder re-expresses every piece through Form drawings and catalogue profiles (§11). Nothing is copied from the old code.

### 3.1 Walls, with openings and reveals

**The bays:**

| Wall | Finished (silk) face | Masonry inner face | Outer face | Height | Openings |
|---|---|---|---|---|---|
| North | y −7.000 | y −7.025 | y −7.600 | to 13.85 (roof slab soffit) | the cabinet door, centre x −6.00 |
| South | y +7.000 | y +7.025 | y +7.600 | to 13.85 | none |
| East (Bay 1 end) | x 0.000 | x +0.025 | x +0.600 | to 13.85 (a lunette above 6.50) | port `salon.east`, centre y 0 |
| West (Bay 5 far) | x −60.000 | x −60.025 | x −60.600 | to 13.85 (a lunette above 6.50) | the oval passage, centre y 0 |

- **Masonry:** travertine ashlar, 0.575 thick, coursed by Form within Matter's travertine stock. The outer faces are `travertine.honed.weathered`.
- **Silk on the walls:** the silk runs from z 0.150 (the skirting's slot) up to 6.250 (behind the cornice). It is stretched over 5 mm felt on 18 mm battens: a 25 mm build-up from the masonry face to the silk face.
- **Silk colours (the approved "one step deeper"):**
  - Bay 1 `#6A2A22` (oxblood)
  - Bay 2 `#47515A` (zinc)
  - Bay 3 `#5A6D54` (sage)
  - Bay 4 `#8A5A54` (old rose)
  - Bay 5 `#A0793A` (haystack)
  - The cabinet keeps `#2F3A33` (cabinet green)
- **Internal doors:** 3.000 clear, semicircular head springing at 3.300, crown 4.800, 64 stations per semicircle.
  - **Cabinet door:** centre x −6.000, through the north wall (y −7.000 to −7.600). The reveal is travertine (`travertine.honed.clean`). An architrave (profile P5) is on both faces: the bay face at y −7.000 and the cabinet face at y −7.600.
  - **Oval passage:** a tunnel 3.000 wide, with a round-arched soffit springing at 3.300. It runs from x −60.000 to the oval's inner face (x −61.722 at the jambs, −61.500 on the axis). It is lined in travertine. Architrave P5 is on the far wall's bay face; the slim architrave P6 follows the curved oval wall on the oval side, its top at 4.96, under the cove.
- **Port reveal at `salon.east`:** the rough opening only. The joint lines it. The room's own architrave P5 is on the end wall's bay face (x 0.000).

**The cabinet** (inner/silk faces x −10.000 and −2.000, y −15.600; its south face is the Salon's north outer face, y −7.600, also silk-lined):
- Walls: 0.40 thick travertine ashlar. The outer faces are at x −10.400, x −1.600 and y −16.000. The masonry inner faces lie 25 mm behind the silk.
- Height: 5.500 to the flat ceiling. The roof slab runs from 6.100 to 6.300.

**The oval:**
- The inner face is the ellipse a 11.000 (along x) × b 7.500 about (−72.500, 0).
- The outer face is the 0.600 offset along the normal, which is not an ellipse. Its west apex is at −84.100 and its north and south faces at ±8.100.
- The walls are `lime-plaster.limewashed.clean` (lime white `#F1EDE4`) from the floor to 5.000, then the cove.
- The outer masonry, `travertine.honed.weathered`, rises to 7.200.
- The door is at the east apex (3.000 wide, springing 3.300). The oval's wall length from the door's north jamb, round by the west, to the south jamb is **55.597 m**, measured along the inner face.

**The passage block:**
- Masonry closes the gap between the Bay 5 far wall's outer face (x −60.600) and the oval's outer face, for |y| ≤ 2.100.
- It meets the oval's outer face at x −61.293.
- Its flat top is at 5.200 (`travertine.honed.weathered`).

### 3.2 Piers and transverse arches (the order of the bays)

**Piers:** four, at x −12.000, −24.000, −36.000 and −48.000; faces ±0.600 in x. Each pier runs from the masonry wall (|y| 7.025) to its jamb face at |y| 4.500 (2.50 deep), from the floor to the springing at 6.500. The piers are coursed travertine (`travertine.honed.clean`), with "coursed jambs" per the Details board.
- **Base:** profile P3 swept round the pier's three faces, mitred, from the wall back to the wall. Its top is at 0.300.
- **Impost:** profile P4 swept the same way, from 6.060 to 6.500.
- **Silk on the pier faces:** on each side face (x = pier ± 0.600), in the colour of the bay that face looks into.
  - The panel is upholstered: silk over felt on battens, **12 mm proud** of the stone.
  - It runs from the wall corner (|y| 7.025) to **0.14 m short of the jamb** (|y| 4.640), and from the base's top (0.300) to the impost's underside (6.060).
  - Its jamb edge, top and bottom are returned to the stone (the upholsterer's tacked edge).
  - The pier's jamb face (|y| 4.500) stays stone.

**Transverse arches:** one over each pier pair, spanning 9.000 clear. Each arch is centred on (y 0, z 6.500) with an intrados radius of 4.500, and is 1.200 thick in x (the pier's faces).
- **Voussoirs:** 15 stones of 12° each, from the intrados (R 4.500) to the extrados (R 5.300). Each is its own piece of `travertine.honed.clean`, with its own slice of the quarry.
  - Joints are radial, with **V-chamfers 12 mm into the stone** on both faces and across the soffit.
  - The springers bear on the imposts.
- **Keystone:** the eighth stone (index 7). Its faces are **20 mm proud** on both sides. Its sides are radial and its top is flat at z 12.300 (springing + 5.800). It carries no V on its own faces.
- **Archivolt:** profile P7 swept round the extrados on both faces, in two runs that stop against the keystone's sides.
- **Spandrel diaphragm:** coursed travertine on both faces, from the extrados (R 5.300) up to the vault's back (R 7.030). It seals the vault against the arch.

### 3.3 Vault, coffers, lunettes and lanterns

**The barrel vault:** intrados centre (y 0, z 6.500), R 7.000, crown 13.500. It spans each bay between the arch faces (or the end wall). It is a lining of cast fibrous-plaster panels (`fibrous-plaster.cast.dusted`), hung from the roof slab. The void between the vault's back and the roof slab is closed by the masonry.

**Coffers:**
- **Rows:** 19 around the semicircle, each 9.474° (1.1574 m along the intrados). Row 0 is at the north springing.
- **Columns:** 8 per bay, between plain bands **0.7703 m** wide against each arch face or end wall.
  - Bays 2–4: the column pitch is 1.1574 (square coffers).
  - Bays 1 and 5 (11.40 long): the pitch is 1.2324.
- **Coffer section** (inset from the rib line, depth into the vault): (0, 0) · (0.100, 0) · (0.115, 0.080) · (0.175, 0.080) · (0.190, 0.160) · (0.240, 0.160) · (0.280, 0.240).
  - Three steps down to a panel 0.24 deep.
  - The ribs between coffers are 0.20 wide.
- **Rosettes:** one gilt rosette in every coffer, centred on the deepest panel. That is 5 × (152 − 2) = **750**, all identical and instanced.
  - Carving `rosette.gilt.220`, Ø 0.220.
  - Twelve broad outer petals, radially 0.022 to 0.112, 0.044 wide, domed 16 → 4 mm.
  - Twelve inner petals between them, 0.020 to 0.078, domed 26 → 12 mm.
  - Each petal has a sunk midrib.
  - A domed boss (r 0.021, 30 mm high) sits in a bead ring (r 0.025).
  - Material: `plaster-of-paris.gilt.dusted` (cast, then water-gilt).

**Lunettes:** the end walls above the springing, up to the vault. They are `lime-plaster.trowelled.clean`. The cornice runs round them at the springing.

**Lanterns:** one per bay, in the crown row (row 9) over columns 3–4, counted from the west arch. Each is a rectangular shaft cut through the vault and the roof.

| Bay | x (west → east) | y | Size |
|---|---|---|---|
| 1 | −6.9324 → −4.4676 | −0.5781 → +0.5781 | 2.465 × 1.156 |
| 2 | −19.1574 → −16.8426 | ±0.5781 | 2.315 × 1.156 |
| 3 | −31.1574 → −28.8426 | ±0.5781 | 2.315 × 1.156 |
| 4 | −43.1574 → −40.8426 | ±0.5781 | 2.315 × 1.156 |
| 5 | −55.5324 → −53.0676 | ±0.5781 | 2.465 × 1.156 |

- **Shaft:** vertical walls from the vault's face up to the roof top, 14.050, lined in `lime-plaster.trowelled.clean`. The walls start 4 mm inside the vault's face, so the joint is sealed.
- **Inner pane and grid:** the inner pane's underside is at **13.580**, carried on a steel grid 70 mm deep (13.510 to 13.580). The grid has a perimeter frame 40 mm wide, one bar along x on the axis 30 mm wide, and 5 cross bars at the sixths of the length, 30 mm wide. It is `steel.painted.clean`.
- **Inner panes by bay:**
  - Bay 1: clear, `low-iron-glass.annealed.clean`.
  - Bay 2: **flashed opal**, `opal-glass.flashed.clean`.
  - Bay 3: clear.
  - Bay 4: clear, with a **muslin** under the grid. The muslin (`cotton-muslin.stretched.dusted`) is stretched at 13.490, sagging 30 mm at its centre, on a 25 × 25 mm steel angle frame (13.465 to 13.494).
  - Bay 5: clear.
- **⚑ The outer lantern (the weather glazing).** As built, the only glass was the flat inner pane at the foot of an open shaft. That pane would hold rain, and the Light board draws a lantern standing proud of the vault. So the weather glazing is a pitched lantern on the roof:
  - a kerb from 14.050 to 14.200 round the opening (`travertine.honed.weathered`, with a bronze cap flashing, `bronze.patinated.weathered`);
  - two glass slopes at 30° to a ridge along x at **14.534**, with glazed gable ends;
  - bronze glazing bars at the sixths;
  - `low-iron-glass.laminated-uv-filtering.soiled`.

  This is the Glazed opening (§4).

**Roof:**
- A slab from **13.850 to 14.050** over x −60.600 to +0.600 and y ±7.600, flush with the outer faces. It must not project past the port face.
- It is open only over the five lanterns, whose shafts close its holes.
- Its top is paved in `travertine.honed.weathered`.
- The void above the vault is closed from inside by the masonry: no pinhole reaches the sky (Form.Sealed).

**Bay 3's pergola (on the roof):**
- Centred on bay 3's lantern (x −30.000, y 0), 6.00 × 5.20.
- **Posts:** four, 70 mm square, at x −33.000 / −27.000 and y ±2.600, from 14.050 to 16.390.
- **Beams:** two along x at y ±2.600, 50 wide × 80 deep, from 16.390 to 16.470, running x −33.150 to −26.850.
- **Bars:** across on the beams every 0.50 m from x −33.000 to −27.000 (13 bars), 28 wide × 30 deep, running y −2.720 to +2.720, top at 16.500.
- **Material:** `bronze.patinated.weathered`.
- **Planters:** four in travertine (`travertine.honed.weathered`), 0.70 square × 0.55 high with 60 mm walls. Their centres are (−32.600, ±2.200) and (−27.400, ±2.200). The soil lies 60 mm under the rim.

### 3.4 Mouldings: the profiles

These are singular to the Salon, so they stay in `Private/SalonPlan.h` until a second use promotes them into `Data/Form`. Proposed catalogue keys are given for that day.

Points are (A, B): A is out of the host face, B is up (or out of the arch face, for P7). They are traced with the solid on the left. `arc(cA, cB, rA, rB, θ0→θ1, n)` continues from the previous point along an elliptic arc; `~` marks a smooth point.

- **P1 · cornice with a vault trough** (`cornice.trough-lipped.470`): the bays' long walls between the piers, and round the lunettes.
  - (0.000, 5.625) (0.040, 5.625) (0.040, 5.600) (0.075, 5.600) (0.075, 5.800) (0.095, 5.800) (0.095, 5.825)
  - arc(0.095, 5.905, 0.080, 0.080, −90→0, 8)
  - (0.175, 5.925) (0.195, 5.925) (0.195, 5.945) (0.470, 5.945) (0.470, 6.150) (0.490, 6.150) (0.490, 6.180)
  - arc(0.490, 6.280, 0.040, 0.100, −90→0, 8)~
  - arc(0.570, 6.280, 0.040, 0.100, 180→90, 8)
  - (0.585, 6.380) (0.585, 6.470) (0.510, 6.470) (0.510, 6.250) (0.000, 6.250) (0.000, 6.500)

  The lip hides the **vault-cove trough**, 0.510 wide × 0.220 deep. The 40 mm recess at its foot (A 0 to 0.040, B 5.600 to 5.625) is the **silk-top slot**. Material: `lime-plaster.run.clean`. It is swept on the vault's own stations, so vault and cornice meet vertex for vertex.
- **P2 · slotted skirting** (`skirting.slotted.180`): (0.060, 0.000) (0.060, 0.160) (0.040, 0.180) (0.030, 0.180) (0.030, 0.150) (0.000, 0.150).
  - The 30 mm recess between its lip and the silk is the silk-base slot.
  - It stops 0.18 m outside each door's clear edge, inside the architrave, and dies into the pier bases.
  - The cabinet uses the same profile.
  - `travertine.honed.clean`.
- **P3 · pier base** (`base.plinth-torus.300`):
  - (0.100, 0.000) (0.100, 0.160) (0.060, 0.160)
  - arc(0.060, 0.200, 0.040, 0.040, −90→90, 12)
  - (0.045, 0.240) (0.030, 0.270) (0.030, 0.300) (0.000, 0.300)
  - `travertine.honed.clean`.
- **P4 · pier impost** (`impost.ovolo-abacus.440`):
  - (0.000, 6.060) (0.020, 6.060) (0.020, 6.085)
  - arc(0.020, 6.140, 0.055, 0.055, −90→0, 8)
  - (0.075, 6.160) (0.085, 6.160) (0.085, 6.400) (0.100, 6.400) (0.100, 6.500) (0.000, 6.500)
  - `travertine.honed.clean`.
- **P5 · door architrave** (`architrave.two-fascia.220`): A runs out from the opening's edge, B out of the wall.
  - (0.220, 0.000) (0.220, 0.090) (0.205, 0.090)
  - arc(0.205, 0.070, 0.020, 0.020, 90→180, 6)
  - (0.160, 0.070) (0.160, 0.064) (0.100, 0.058) (0.100, 0.052) (0.020, 0.046) (0.000, 0.030) (0.000, 0.000)
  - Swept up the left jamb, over the arch and down the right jamb (DoorFrames).
  - `travertine.honed.clean`.
- **P6 · slim architrave** (`architrave.two-fascia.160`), on the oval side:
  - (0.160, 0.000) (0.160, 0.075) (0.145, 0.075)
  - arc(0.145, 0.058, 0.017, 0.017, 90→180, 6)
  - (0.100, 0.058) (0.100, 0.052) (0.020, 0.046) (0.000, 0.030) (0.000, 0.000)
- **P7 · outer archivolt** (`archivolt.bead-fascia-cavetto.175`): A runs radially out from the extrados (R 5.296), B out of the arch face.
  - (0.175, 0.000)
  - arc(0.175, 0.012, 0.022, 0.012, −90→−180, 6)
  - (0.030, 0.012) (0.026, 0.004)
  - arc(0.013, 0.004, 0.013, 0.010, 0→180, 8)
  - (0.000, 0.000)
  - A bead 14 mm proud, a fascia at 12 mm and a cavetto.
- **P8 · cabinet cornice** (`cornice.slot-cove.330`): swept as a closed loop on the ceiling's grid lines.
  - (0.000, 5.125) (0.040, 5.125) (0.040, 5.100) (0.070, 5.100) (0.070, 5.180) (0.090, 5.180) (0.090, 5.200)
  - arc(0.270, 5.200, 0.180, 0.180, 180→90, 12)
  - (0.290, 5.380) (0.290, 5.420) (0.330, 5.420) (0.330, 5.500)
  - `lime-plaster.run.clean`.
- **P9 · oval cove** (`cove.quarter-ellipse.1450`): A runs in from the wall. It is swept round the ellipse on the wall's own stations.
  - (0.000, 5.000) (0.030, 5.000) (0.030, 5.050) (0.015, 5.070)
  - arc(1.300, 5.070, 1.285, 1.130, 180→90, 32)
  - (1.450, 6.200) (1.450, 6.350)
  - `lime-plaster.limewashed.clean`.
- **P10 · oval skirting** (`skirting.flat.100`): (0.018, 0.000) (0.018, 0.090) (0.008, 0.100) (0.000, 0.100). It runs from 1.63 m either side of the door round the room. `travertine.honed.clean`.

### 3.5 Ceilings: the cabinet and the velarium

**Cabinet ceiling:**
- A flat `lime-plaster.trowelled.clean` ceiling at 5.500, running inside P8 (from 0.330 off the walls).
- It surrounds a **laylight well** from x −8.500 to −3.500 and y −14.100 to −9.100 (5.00 × 5.00, centred on (−6.000, −11.600)), rising 5.500 → 5.900.
- The laylight is flashed opal glass at 5.900 (`opal-glass.flashed.clean`).
- ⚑ As built it was an emissive "sky-lit" panel. It is now a luminaire's diffuser: an electric luminous ceiling in the 0.20 void under the roof slab (§4).
- The roof void is closed from inside.

**Oval velarium:**
- A stretched cloth, `cotton-muslin.stretched.dusted`, tensioned on a light painted-steel frame.
- Its rim sits on the cove's top edge: the ellipse 9.550 × 6.050 about (−72.500, 0) at z 6.350. It rises to **6.850** at the centre: z = 6.350 + 0.500 (1 − ρ²), where ρ is the elliptic radius fraction.
- The widths run along the oval's length, 1.40 wide, with double-stitched seams.

**⚑ The glass roof over the velarium.** As built, the oval's roof was opaque and the velarium glowed by an emissive material. The approved design wants daylight: "Monet wanted the Water Lilies seen by daylight from above, never by lamps."
- An opaque roof ring runs from the outer face in to the velarium's rim line at **7.200**.
- Inside that line, a low elliptic glass dome rises to **7.800** at the centre. It is `low-iron-glass.laminated-uv-filtering.soiled` on painted-steel bars at ≤ 1.20 m.
- This leaves a real attic, **0.85 m** deep at the rim and 0.95 m at the centre, between the cloth and the glass.
- A fixed daylight control from Light's catalogue sits under the glass (§4, §12). It is not a blind.

### 3.6 Floors

z 0 is the finished floor everywhere at level 0.

**Oak Versailles parquet** in every bay and in the cabinet: `oak.oiled-waxed.worn`, 22 mm thick on its bed, flush with the stone.
- **Bays field:** x −59.400 to −0.600, y −6.600 to +6.600.
  - Panels 1.200 square on the room's grid. Cell edges are at x = −0.600 − 1.2k and y = −0.600 + 1.2k, so the panels are centred on the axis and every pier face falls on a grid line.
  - Each panel is a mitred frame 85 mm wide, an interlaced diagonal lattice (3 strips each way, 70 mm, crossing over and under in turn) and diamond infills. Every strip and infill is its own piece of oak, with its own grain direction.
  - A **border** of 100 mm oak boards (random lengths 1.2 to 2.4, parallel to the walls) runs from the field to the walls: 0.40 wide along the long walls and 0.60 along the two end walls.
  - Per bay the field is 9 × 11 panels. Stone interrupts it under every arch.
- **Cabinet field:** x −9.600 to −2.400, y −15.200 to −8.000 (6 × 6 panels), with cell edges at x = −6.000 + 1.2k and y = −11.600 + 1.2k. It has a 0.40 border.

**Travertine** (`travertine.polished.worn`, 30 mm):
- under every arch (x = pier ± 0.600, |y| ≤ 4.500);
- in the internal door reveals and the oval passage;
- the whole oval floor;
- the stair's top landing.

The joint lays the port thresholds, not the room.

**Brass strips:** 12 mm wide, 12 mm deep, flush (0.5 mm proud), `brass.brushed.worn` (the boards' brass, a declared accent of bronze). They lie where oak meets stone:
- at x = pier ± 0.600 for |y| ≤ 4.500 (8 strips);
- at x 0.000 and x −60.000 for |y| ≤ 1.500;
- at the cabinet door on both faces, y −7.000 and y −7.600, for x −7.500 to −4.500.

**Viewing stones:** 18, flush. Each is a disc of `travertine.honed.worn` (r 0.400) in a ring of `bronze.gilt.worn` (r 0.400 to 0.450). The approved Details board says "gilt ring"; the as-built ring was brushed bronze. Each needs 0.60 clear round its centre. They are let into the oak where they fall in a bay or the cabinet, otherwise into the stone. Centres:
- **Bay 1:** (−6.00, −4.00) (−5.70, +3.60)
- **Cabinet:** (−6.00, −11.60)
- **Bay 2:** (−18.00, 0.00) (−18.00, −3.60) (−16.00, +3.80) (−21.20, +4.60)
- **Bay 3:** (−30.00, 0.00) (−30.00, −3.60) (−30.00, +3.60)
- **Bay 4:** (−42.00, 0.00) (−42.00, −3.80) (−42.00, +3.80)
- **Bay 5:** (−54.30, 0.00) (−54.30, −3.80) (−54.30, +3.80)
- **Oval:** (−65.30, 0.00), over the shaft (the slab) near the east focus; (−79.70, 0.00), near the west focus.

**Slabs and fills:**
- The bays' and the cabinet's floor build-up is 0.050, down to z −0.05, on the Reserve's top.
- The oval's floor is a 0.40 slab over the shaft zone, its soffit at z −0.40.
- The slab edge round the pond opening carries the travertine curb (§3.7).

### 3.7 The stair and its shaft (the served well)

**Flight:**
- **36 risers of 0.16111** (5.80 / 36) and 35 goings of **0.3389**, running east.
  - The first nosing is at x −76.000, level with the oval floor. The last riser is at x −64.1385.
  - 2R + T = 0.661, against the human measure's pace of 0.63 (see the risks).
- **Treads:** each is one stone of `travertine.honed.worn`, with a 25 mm nosing rounded r 12 and undercut 12 beneath. Treads, risers and nosings share their profile's stations.
- **Top landing:** x −76.400 to −76.000 at z 0.000, flush, filling the opening's west end.
- **Bottom landing:** x −64.1385 to −61.000 at z −5.800, then through the head wall to the port face, x −60.600.

**Shaft:**
- Brick side walls (`brick.laid.clean`): inner faces y ±1.200, outer faces ±1.600, from z −5.850 up to the slab.
- A west end wall at x −76.400 to −76.800.
- The 0.40 brick head wall at the east end (§2).
- An underside of the build-up at z −6.150.

**Closed strings:** `travertine.honed.clean` on both walls, 25 mm proud, 0.24 over the nosings. At the bottom landing they turn into a 0.15 skirting.

**Curb:** travertine, 0.30 deep (z −0.30 to 0.00), lining the opening's north, south and east edges and the slab edge at x −68.900.

**Soffit:** `lime-plaster.trowelled.clean` at z −0.40, east of x −68.900.

**Handrails:** two, on the walls, `bronze.brushed.handled`.
- Ø 50 mm with a flat underside (the chord from 240° to 300°) that carries an LED line (§4).
- 0.90 over the nosings.
- They run from x −68.900, where the rail can pass under the oval slab, down to x −60.900. Between −68.9 and −76 nothing may stand, because the pond settles there.
- They ease round a knee (r 0.15) onto the landing and return into the wall (r 0.06) at both ends.
- Brackets every 1.30: a wall rose Ø 60 × 8 mm, an arm Ø 16 mm and a 75 mm drop.
- The rails' axes lie 0.085 off the wall faces.

### 3.8 The pond (the lifting body `salon.pond`) and its new edge

**Geometry:**
- **Centre:** (−72.500, 0.000). At rest the pond's base sits on the oval floor.
- **Water's edge:** the ellipse **a 3.900 × b 2.000**. The water surface is at pond-z **0.380**, and the water is dyed black.
- **Stations:** at arc-length parallels U(t, d) = s(t) + d·θ(t), so every parallel keeps its stations radial (SalonPond.cpp:57-114).

**Coping** (sitting-height, `travertine.honed.touched`):
- **16 blocks**, 1.2 to 1.36 m each at the die. Block 0 is centred on the east end, block 8 on the west.
- Joints are radial: a 3 mm mortar line 3 mm deep, with arrises chamfered 4.5 mm. Each block has its own slice, tone and polish, and a ±0.25 mm lippage.
- **Upper profile** (d out from the water's edge, z up):
  - (0.000, 0.300) (0.000, 0.398)
  - arc(0.012, 0.398, 0.012, 180→90, 4)
  - (0.050, 0.4194)~ (0.120, 0.4200)~ (0.385, 0.4200)
  - arc(0.385, 0.365, 0.055, 90→−60, 20) — the torus
  - (0.4125, 0.300) — the fillet
  - arc(0.4125, 0.2725, 0.0275, 90→180, 8), ending at (0.385, 0.2725) — the cavetto down to the die
  - The seat is flat from d 0.120 to 0.385 at 0.420.
- **Frieze band (the die):** d 0.385, z 0.075 to 0.2725 (0.1975 tall).
- **Lower profile:**
  - (0.385, 0.075) (0.425, 0.075)
  - arc(0.425, 0.060, 0.015, 90→0, 6)
  - (0.440, 0.018) (0.425, 0.018) (0.425, −0.002) — a ledge, an ovolo, the plinth, and a toe recess at the floor.
- **Outline:** the coping's outer face is d 0.440, about 4.34 × 2.44. The pond outline in the stage is x −76.840 to −68.160, y ±2.440.

**Carved lily frieze** on the die: Ornament, carved from a close-grained filled block, `travertine.filled-carved.clean`. It is Art Nouveau, after Gallé and Majorelle.
- A height field on a 2.5 mm grid, 40 to 55 mm proud, the edges cut square and undercut 3 mm.
- The composition folds at the two keystones, so the north and south halves mirror.
- Each ≈1.0 m repeat carries:
  - a whiplash rhizome running on through the band;
  - two overlapping pads 0.21 to 0.25 across, and a third lifted on its stalk;
  - an open lily 0.13 to 0.15 across on its own stem;
  - a bud.
- At each end of the long axis a keystone motif (0.36 long): a lily seen from the front, 0.19 across, with pads and buds.
- The design functions are in SalonPond.cpp:168-441.

**Lip:** `bronze.gilt.wet`, a single ring without joints.
- Profile: (−0.005, 0.355) (−0.005, 0.403); arc(0.004, 0.403, 0.009, 180→0, 10); (0.013, 0.394).
- Its bead top is at 0.412, 32 mm over the brimming water.
- A 6 mm slot on its inner face (d −0.0058, z 0.3845 to 0.3905) holds the rim light (§4).

**Basin vessel:** `bronze.patinated.wet`, one vessel with the tray.
- **Lining:** the parallel d −0.003, from z 0.058 to 0.358.
- **Bed:** at 0.060.
- **Planting shelves:** at both ends, for |x − cx| ≥ 3.200, with their tops at 0.260 (0.12 under the water) and vertical risers at |dx| = 3.200.
- **Tray underneath:** `bronze.brushed.tarnished`, out to the toe (d 0.425), underside at −0.002. From the stair, with the pond up, it is the quiet bronze soffit overhead.

**Golden lily:** the pond's switch, a Form Carving in `bronze.gilt.worn`: a Nymphaea flower Ø 0.24, cast.
- It sits among the living pads of the lily at (−72.900, +0.500), on a bronze stem fixed to the bed. Its petals are at pond-z 0.40 to 0.46.
- ⚑ As built it was a plant with a glowing material. It emits nothing now; the rim light is the signal.

**Posts:** four telescopic bronze posts (`bronze.brushed.tarnished`) at (−69.500, ±1.350) and (−75.500, ±1.350).
- Three stages, Ø 0.150 / 0.180 / 0.210, each 0.95 long with 0.08 overlaps.
- Each is housed in a sleeve Ø 0.230 × 1.00 deep in the shaft's side walls (y 1.235 to 1.465), leaving ≥ 35 mm of brick to the shaft face.
- Down, their heads are flush with the floor under the tray.
- ⚑ As built they were 1 m cylinders scaled in z.

**⚑ Guard rails round the opening** (an addition; the as-built used invisible walls):
- Three rails on the opening's north (y −1.215), south (y +1.215) and east (x −68.885) edges.
- A top rail Ø 50 at 1.000 and a mid-rail at 0.500, on Ø 25 stanchions at ≤ 1.10 m that skip the post sleeves.
- `bronze.brushed.handled`.
- Housed in slots in the curb and the shaft walls; down, flush and under the tray.
- They form a coupled Motion body (§6).

**Mass (for Motion.Balanced):**

| Part | Quantity | Mass |
|---|---|---|
| Water | 24.50 m² water area, less 2 × 1.088 m² of shelf × 0.20, gives 7.41 m³ | ≈ 7.4 t |
| Coping and frieze | ≈ 3.3 m³ of travertine | ≈ 8.1 t |
| Vessel and tray | | ≈ 3 t |
| **Total** | | **≈ 19 t** |

### 3.9 Glazing (summary)

| Piece | Where | Spec | Opening |
|---|---|---|---|
| Lantern outer glazing ×5 | on the roof over each lantern, pitched, ridge 14.534 | `low-iron-glass.laminated-uv-filtering.soiled` | Glazed (weather: giverny) |
| Inner panes ×5 | 13.580, on the steel grid | bays 1, 3, 5, 4: `low-iron-glass.annealed.clean`; bay 2: `opal-glass.flashed.clean` | interior |
| Muslin (bay 4) | 13.490, 30 mm sag | `cotton-muslin.stretched.dusted` | interior |
| Oval roof dome | 7.200 (rim) to 7.800 (centre) | `low-iron-glass.laminated-uv-filtering.soiled` | Glazed (weather: giverny) |
| Velarium | 6.350 (rim) to 6.850 (centre) | `cotton-muslin.stretched.dusted` | interior |
| Cabinet laylight | 5.900 | `opal-glass.flashed.clean` | a luminaire's diffuser (§4) |
| Glass stele, 1874 case hood, dancer's vitrine | §8 | `low-iron-glass.*` | Art fittings |

Glass and water are cast non-Nanite.

### 3.10 Fittings

Fittings are Art's; the stage declares anchors.

- **Velvet benches ×8:** a pair in each of bays 2 to 5.
  - Centres (−18.000, ±2.600), (−30.000, ±2.600), (−42.000, ±2.600), (−54.300, ±2.600). Long axis along x. The axis stays clear.
  - Backless, 2.40 × 0.60, 0.44 high.
  - A fumed-oak plinth, `oak.fumed.handled`, 75 mm high and set back 40 mm.
  - An upholstered body in `velvet.dyed-<bay colour>.crushed`. The top edge is rolled r 40, the bottom edge r 14, the ends r 40 in plan.
  - The flat top is at 0.428, quilted in 4 panels domed 16 mm, with a 4 mm piping cord round the top and bottom and across each seam.
  - Velvet colours are the bay's silk colour.
- **Oval pond benches ×4:** a natural oak slab (`oak.oiled.handled`) 0.55 × 0.10, eased r 10, with its grain along the arc. It sits on two travertine plinths (`travertine.honed.clean`) 0.45 long, 0.30 in from the ends, set back 60, with a 20 shadow gap and an 8 × 12 toe recess. The seat is at 0.45.
  - The centrelines are Catmull-Rom curves through 11 points each, 4.42 long, 1.26 m clear of the coping's face:
  - **B1** (−78.04, 1.39) (−77.79, 1.81) (−77.50, 2.17) (−77.18, 2.48) (−76.84, 2.76) (−76.48, 2.99) (−76.10, 3.20) (−75.71, 3.38) (−75.31, 3.53) (−74.90, 3.66) (−74.48, 3.76)
  - **B2** (−70.52, 3.76) (−70.10, 3.66) (−69.69, 3.53) (−69.29, 3.38) (−68.90, 3.20) (−68.52, 2.99) (−68.16, 2.76) (−67.82, 2.48) (−67.50, 2.17) (−67.21, 1.81) (−66.96, 1.39)
  - **B3** and **B4** mirror B2 and B1 in y.
- **Glass stele · 1874 case · the Little Dancer's plinth and vitrine:** §8.
- **Wall labels:** none are fixed. A caption shows on look (Art captions, drawn by Visitor's overlay).

### 3.11 Exterior (tonight: plain)

- **Outer faces:** honed travertine ashlar (`travertine.honed.weathered`), coursed by Form within stock.
- **Roof:** as above.
- **The exterior dress** is §12 polish: the rusticated podium, the giant Ionic order (D 1.10), the blind windows, the balustrades, and the hexastyle portico on the south front (x −40 to −20, the steps to y 17.30, its door blind).

### 3.12 Material keys → Matter records

| Key | Substance | Working | History | In Matter now? |
|---|---|---|---|---|
| `travertine.honed.clean` | travertine | stone.honed | (none) | yes |
| `travertine.honed.worn` | travertine | stone.honed | stone.foot-traffic-wear | yes |
| `travertine.honed.touched` | travertine | stone.honed | stone.pore-soiling (hand and seat contact) | yes (the contact term is to confirm) |
| `travertine.honed.weathered` | travertine | stone.honed | stone.dissolution-outdoors | yes |
| `travertine.polished.worn` | travertine | stone.polished | stone.foot-traffic-wear | yes |
| `travertine.filled-carved.clean` | travertine (close-grained, filled) | stone.honed + carved | (none) | **no:** the working "filled, carved" |
| `lime-plaster.run.clean` / `.trowelled.clean` | lime-plaster | run / trowelled | (none) | yes |
| `lime-plaster.limewashed.clean` | lime-plaster | limewashed | white.soiling (slight) | yes |
| `fibrous-plaster.cast.dusted` | fibrous-plaster | cast.plaster | white.soiling (dust on up-faces) | yes |
| `plaster-of-paris.gilt.dusted` | plaster-of-paris | cast.plaster + gilt | gilt.wear (slight) | yes |
| `bronze.brushed.handled` / `.tarnished` | bronze | brushed | bronze.wear / bronze.tarnish | yes |
| `bronze.patinated.weathered` / `.wet` | bronze | patinated | bronze.weathering / (wet: live) | yes |
| `bronze.gilt.worn` / `.wet` | bronze | cast.bronze + gilt | gilt.wear | yes |
| `brass.brushed.worn` | brass (accent of bronze) | brushed | bronze.wear | yes |
| `steel.painted.clean` | steel (accent) | painted.steel | steel.coating-ageing | yes |
| `low-iron-glass.annealed.clean` | low-iron-glass | annealed | (none) | yes |
| `low-iron-glass.laminated-uv-filtering.soiled` | low-iron-glass | laminated.uv-filtering | glass.soiling | yes |
| `low-iron-glass.toughened.clean` | low-iron-glass | toughened | glass.cleaning-wear | yes |
| `opal-glass.flashed.clean` | opal diffuser glass | flashed | (none) | **no** |
| `oak.oiled-waxed.worn` | oak | oiled and waxed | foot traffic (the lane down the axis) | **no** |
| `oak.fumed.handled` / `oak.oiled.handled` | oak | fumed / oiled | handled | **no** |
| `silk.dyed-<colour>.faded` | silk (6 dyes: the five bays and cabinet green) | dyed, stretched on felt | light-faded, dusted | **no** |
| `velvet.dyed-<colour>.crushed` | velvet (cut pile) | dyed | crushed | **no** |
| `cotton-muslin.stretched.dusted` | cotton muslin | stretched | dusted | **no** |
| `wool-felt.clean` / `linen.unbleached.clean` | felt / linen (the case lining) | (none) | (none) | **no** |
| `brick.laid.clean` | brick | laid, lime-mortared | (none) | **no** |
| `water.dyed-black.still` | water | dyed black (strong absorption) | (none) | **no** |
| `soil.planting` | soil | (none) | (none) | **no** |

Textiles are accents. Matter must declare the core family each ties back to, or Matter.OneLanguage fails (see the risks).

---

## 4 · Light

### 4.1 Daylight openings (all Glazed; no blinds, per D-014)

| Key | Opening | Infill | Below it |
|---|---|---|---|
| `salon.bay1.lantern` | 2.465 × 1.156 at (x −5.700, y 0), roof 14.05 | pitched laminated UV-filtering low-iron | clear inner pane: **sun patch**, "plein air" |
| `salon.bay2.lantern` | 2.315 × 1.156 at x −18.000 | same | **opal** inner pane: the sun stopped, "Paris under cloud" |
| `salon.bay3.lantern` | 2.315 × 1.156 at x −30.000 | same, under the vine canopy | clear: **dappled** sun, "the dance under the trees" |
| `salon.bay4.lantern` | 2.315 × 1.156 at x −42.000 | same | clear pane + **muslin**: shadowless, "a curtained room" |
| `salon.bay5.lantern` | 2.465 × 1.156 at x −54.300 | same | clear: the patch **walks** towards the Rouen row, high in June and low in December |
| `salon.oval.roof` | elliptic dome within the 9.55 × 6.05 rim | laminated UV-filtering low-iron | a fixed daylight control (catalogue: fixed louvre or diffusing film, record to add), then the velarium cloth |

The cabinet has no daylight.

### 4.2 Luminaires

Each luminaire is given by catalogue type, count and position, with its control class. Light's Scheme places the spots; Commission sets the dimming. No lumens are set here.

| # | Type (catalogue key) | Count / extent | Position (mount) | Control |
|---|---|---|---|---|
| L1 | `linear-cove.3000k` | 12 runs, ≈ 134 m: both long walls of every bay between the piers (pier face ± 0.12), and round the two end-wall lunettes | in the cornice trough (P1), 0.255 off the silk face, z 6.270, facing up to the vault | circulation (dusk: half) |
| L2 | `linear-slot.3000k` | the same 12 runs, ≈ 136 m | in the silk-top slot (P1), 0.020 off the silk, z 5.620, grazing down the silk | gallery (holds the silk ratio, §4.4) |
| L3 | `track-spot.4000k.12deg`, recessed | ≈ 110 (Scheme decides; about 3 per work, 4 to 5 for Bazille, Women in the Garden and Paris Street) | **M1:** on the deepest panel of a coffer, in rows 7–8 for north-wall works and rows 10–11 for south-wall works, in the column nearest each work, aimed ≈ 26° off the wall. For the end-wall Bazille and the stele: bay 1's coffers in columns 5–7. **M2 (optional):** recessed in the transverse-arch soffits at voussoirs 2–3 and 12–13 (z ≈ 8.6), for works within 3 m of a pier. | gallery, daylight-linked, presence-linked at night |
| L4 | `track-spot.3000k.20deg`, recessed | 1 | an M1 coffer above the Little Dancer's vitrine | gallery (50 lx care) |
| L5 | `track-spot.3000k.29deg`, recessed | 1 | an M1 coffer above the 1874 case | gallery (50 lx care) |
| L6 | **`laylight.opal.3000k`** (record missing) | 1 luminous ceiling, 5.00 × 5.00 | the cabinet's laylight well, LED tiles behind the opal at 5.90 | gallery |
| L7 | `track-spot.4000k.20deg`, recessed | 4 (Luncheon ×2, Olympia, the Bar) | the cabinet ceiling band, 1.20 from the hung wall, z 5.500 | gallery |
| L8 | `linear-slot.3000k` battens | ≈ 8 rows, ≈ 110 m on a 1.50 grid parallel to x | in the oval attic, over the velarium, facing down onto the cloth | gallery (daylight-linked top-up, night) |
| L9 | `linear-cove.3000k` strip | 2 runs, ≈ 8.5 m each | in each stair handrail's flat underside, onto the treads | circulation (stairs) |
| L10 | `downlight.3000k.31deg`, recessed | 2 | the shaft soffit (z −0.40) at (−66.200, 0) and (−62.400, 0) | circulation (stairs) |
| L11 | `linear-cove.3000k` strip (the as-built colour was 2700 K) | **32 lengths** of ≈ 0.594 m, round the 19.02 m water's edge | in the pond lip's slot. They light in order from the west end (y +0.50) northwards. | follows channel `salon.pond.state` (the reveal fraction) |

⚑ Replaced as-built fakes, none re-expressed:
- the floating accent spots (4.5 m over the work, 2.6 m out in mid-air);
- the floating wall-washers;
- the rect lights standing in for the opal, the muslin and the velarium;
- the emissive velarium, laylight and golden lily;
- the post-process exposure zone for the bays (the Eye owns exposure);
- the 650 lx Salon painting level (it breaks care).

### 4.3 Illuminance targets (by use and care)

| Surface | Target | Source |
|---|---|---|
| Every oil painting in the bays, the cabinet and on the stele | **150 lx** (recommended), **≤ 200 lx** at every reference moment; ≤ 600,000 lx·h a year; UV ≤ 75 µW/lm | Care `oil-and-tempera` |
| The willows (4 works, 12 canvases) | daylight through the velarium alone by day, **≤ 200 lx** at the brightest reference moment; topped up by L8 to ≥ 150 lx when daylight falls short and at night | Care `oil-and-tempera`; Oval board |
| The 1874 catalogue | **50 lx**, 150,000 lx·h | `works-on-paper` |
| Le Charivari (newsprint) | **50 lx**, **15,000 lx·h** | `works-on-paper.fugitive` |
| The Little Dancer (wax or bronze with a tulle skirt and a silk satin ribbon) | **50 lx**, 15,000 lx·h | `textiles.silk` (the most sensitive component) |
| Floors of the bays, cabinet and oval | 100 lx (day); 50 lx after dusk | `circulation` / `circulation.night` |
| Stair treads and landings | 100 lx day and night | `stairs` |

### 4.4 Design targets (luminance order, measured, not knobs)

The Light board's order of brightness, section through bay 2. Each target is checked at every reference moment.

1. **Bays:** L(lantern's inner pane) is the highest luminance in each bay.
2. L(paintings) > L(silk). E(silk) is **0.25 to 0.40 ×** E(paintings) ("about a third").
3. L(transverse-arch voussoir faces and jambs) > L(vault coffer panels, rows 3–15). "The stone lit only by the room; the vault in half-light: cove at half, coffers in shade."
4. The floor oak shows no specular image of a lantern brighter than L(paintings) ("warm oak, no glare").
5. **Oval:** L(velarium) > L(lime walls) > L(willows).
6. **Cabinet:** L(laylight) > L(paintings) > L(cabinet-green silk).

---

## 5 · Time and weather

| Glazed opening | Weather observer (D-005) | What follows it |
|---|---|---|
| `salon.bay1.lantern` … `salon.bay5.lantern` | **giverny** (Monet's garden, 49.0757 N, 1.5337 E) | cloud attenuation of the museum's sun and sky through that opening; rain and wetness on its glass (Matter's surface law); the rain's sound (Sound) |
| `salon.oval.roof` | **giverny** | the same. Broken cloud sends shadows across the velarium; rain on the glass above the cloth |
| The roof, the pergola, the outer faces and the planters (exterior, seen from outside) | **museum** | wetness, snow and wind: the one museum weather |

**Sun:**
- It is always the museum's: the city from `Data/Time/Museum.json`, with the **idealised** model by default (D-003). Only the weather comes from Giverny.
- Bays 1 and 5 carry sharp patches. Bay 5's walks across the bay towards the Rouen row with the season. Bay 3's is dappled through the vines, when they are grown.
- Bays 2 and 4 carry no patch: the opal and the muslin stop it.
- Vines on the roof follow the museum's solar term (leaf-out ≈ Qingming, fall ≈ Shuangjiang), so bay 3's dapple disappears in winter.

---

## 6 · Motion

**M-1 · `salon.pond` (the lily pond that lifts)**

- **Body:**
  - the whole pond: coping, frieze, lip, vessel and tray;
  - the golden lily;
  - the living plants, attached by key;
  - the water, attached by Nature to the basin key `salon.pond`.
- **Joint:** prismatic along +z. Travel **0.000 → 2.600**.
- **Drive:** four synchronised hydraulic telescopic rams, one per post (3 stages).
  - No counterweight. Load ≈ 19 t, ≈ 4.8 t per ram.
  - Travel time **5.0 s** each way on a jerk-limited S-curve: v ≤ 0.80 m/s, a ≤ 0.65 m/s², jerk ≤ 1.0 m/s³.
  - The as-built used a smoothstep, whose jerk is unbounded at the ends.
- **States:**

| State | Height | Entered by |
|---|---|---|
| Down | 0.0 | rest; the end of Lowering |
| Armed | Down | "Wake the golden lily". The rim light (L11) runs round in 1.0 s. It returns to Down after **5.0 s** unused. |
| Rising | 0 → 2.6 | "Lift the pond" from Armed, or "Raise the pond" from the stair-foot plate (from Down, with no arming) |
| Up | 2.6 | the end of Rising |
| Lowering | 2.6 → 0 | "Lower the pond", only from the oval floor within 6.50 of the pond's centre and off the opening; or automatically when the visitor, on level 0, is more than 16.0 from the pond's centre (has left the oval) and no body is in the well |

- **Uses (IMuseeUsable):**
  - the golden lily at (−72.900, +0.500), pond-z 0.43;
  - ⚑ **the stair-foot plate** (new; see the risks): a bronze lily push-plate on the shaft's north wall at (−62.000, −1.200, −4.700), 1.10 above the bottom landing. Its one verb is "Raise the pond". It is usable when the pond is Down or Lowering, so a visitor below is never shut in (Site.Reachable over the state graph).
- **Occupancy guard** (Motion, D-014): lowering **holds** while any body is in its next travel slice. That slice is the pond's outline grown by 0.05, from the floor up to the tray's underside. The drive resumes when the slice is clear.
  - ⚑ As built, a visitor was teleported out. That is removed.
  - Rising needs no guard: nothing can stand on the water, and the coping is a seat, not a step.
- **Clearances** (Motion.Clears, ≥ 5 mm at every sampled phase):
  - The pond's outline is 1.26 from the benches' faces.
  - At Up, the tallest iris (pond-z 0.38 + 0.90) + 2.60 = 3.88. That is ≥ 2.4 under the velarium's rim at 6.35.
  - The tray's underside at Up is 2.598 over the oval floor and over the top landing, and ≥ 2.6 over every tread.
  - At Down, the tray's toe ring bears on the floor round the opening: x −76.84 to −76.40 and −68.90 to −68.16, |y| 1.20 to 2.44.
- **Housings** (Motion.Housed): four post sleeves, Ø 0.230 × 1.00, in the shaft walls. The collapsed posts (0.95) sit inside them with their heads flush.
- **Channel:** it publishes `salon.pond.state` (state and rim-reveal fraction 0–1) and FMuseeActuation (for the pump's voice).

**M-2 · `salon.pond.guard` (coupled with M-1, one coupling group)**

- **Bodies:** the three guard rails (§3.8).
- **Joint:** prismatic along +z, travel **0.000 → 1.000**.
- **Coupling:** the rail height is min(h − 0.10, 1.00) for pond height h ≥ 0.10, and 0 below. The rails are therefore fully housed before the tray's last 0.10 of descent, and never touch it.
- **Interlock:** the pond may not enter its last 0.10 of travel unless the rails are housed.

The Rotunda ↔ Salon and Salon ↔ Reserve joints have open portals and carry no mechanisms.

---

## 7 · Nature

**Pond water** (UNatureWaterComponent):
- Basin body key `salon.pond`; volume **7.41 m³**; surface at pond-z 0.380. The volume is conserved in every configuration (Nature.WaterContained).
- Still, with ripples ≤ 3.5 mm (the as-built ripple 0.035).
- Dyed black: the light is gone within a few centimetres, so the water is a mirror.

**Nymphaea** (hardy water lily, pink and white): 10 plants from the plan, each with its colony. Positions are (local x, y, pad radius):
- (−69.60, −0.60, 0.45) flowering
- (−70.60, +0.90, 0.38)
- (−71.90, −1.20, 0.42)
- (−72.90, +0.50, 0.50) flowering; the golden lily's host
- (−74.10, −0.70, 0.36)
- (−75.10, +0.80, 0.44) flowering
- (−75.60, −0.40, 0.30)
- **(−69.35, +0.50, 0.30)**, moved 0.15 west of the plan's −69.20 so it stays on the 0.32 m bed and off the east shelf (Nature.WithinBotany)
- (−71.60, +1.40, 0.30) flowering
- (−73.60, +1.50, 0.28)

The flowers alternate pink and white. The colonies are held inside the lining (a 3.90 × 2.00 ellipse).

**Marginals on the shelves** (0.12 deep; feet at pond-z 0.26). Offsets (dx, dy) are from the pond's centre, mirrored at both ends (dx ±):

| Species | Where | Detail |
|---|---|---|
| *Iris laevigata* | (±3.50, −0.60), 2 flowers; (±3.47, +0.62), 2 flowers at the west end and 3 at the east | clumps of fanned sword leaves; blue-violet flowers with a white signal on each fall |
| *Carex* | (±3.74, ±0.04), (±3.33, −0.92), (±3.33, +0.93) | tufts: arching blades, a few culms with brown spikes |
| *Myosotis scorpioides* | (±3.30, 0.00) r 0.14; (±3.62, −0.25) r 0.08; (±3.60, +0.30) r 0.08 | patches: sky-blue flowers with a yellow eye in coiled sprays, pink buds |

The axis itself is left open. Growth space: above the shelves, inside the lip, under 1.30 above the water.

**Indoor phenology** (the oval is heated):
- The Nymphaea flower from Lixia to Hanlu; their pads persist, thinned, through winter.
- Irises flower from Mangzhong to Xiazhi.
- Forget-me-nots flower from Lixia to Dashu.

**Vitis vinifera ×4** on bay 3's pergola:
- They are trained from the planters up the nearest post, along the beams and out along the bars, with leaves in clusters and holes between (the dapple).
- Growth space: the pergola's frame between 16.39 and 16.75, over the planters.
- Outdoor phenology, museum wind.

There is no ground and no rock. The planter soil is `soil.planting`.

---

## 8 · Art

**The hang rule** (SalonHang board, Hang.swift:15-17):
- A work's centre is at **1.55** above the floor. If that would put its bottom below 0.60, the centre is **0.60 + h/2** instead.
- The frame's back lies on the silk face.
- Neighbouring frames are ≥ 0.04 apart.
- Works hang at their real sizes. All oils are framed **SalonGilt**: a carved Barbizon / Louis XV gilt frame, moulding 0.10 to 0.14 (0.09 + 0.015 × the longer side), `wood.gilt.worn` (gilt over carved wood, a new record). No work is under 0.40 across, so CabinetGilt is unused.
- The canvas is a real object: linen, ground, varnish, tacking edges, and a pine stretcher with keys. Its back shows through the stele's glass.
- **Sun-patch rule:** no work sits in a direct-sun patch at any reference moment (Art.WithinCare). A failure moves the work by the hang; it never adds a blind.

**Anchor keys:** `salon.<room>.<wall>.<n>`, numbered east to west along the wall (or clockwise in the oval). The board numbers are in the first column.

**Bay 1 · The Birth of Impressionism** (oxblood; 7 works)

| # | Work (id) | w × h (m) | Anchor · centre (x, y, z) · facing | Frame |
|---|---|---|---|---|
| 1 | monet-impression-sunrise | 0.63 × 0.48 | `salon.bay1.stele`: on the stele's east face, (−6.062, +1.400, 1.550), facing +x | SalonGilt |
| 2 | bazille-family-reunion | 2.30 × 1.52 | `salon.bay1.east.1`: (0.000, −4.000, 1.550), facing −x | SalonGilt |
| 3 | monet-women-in-the-garden | 2.05 × 2.55 | `salon.bay1.south.2`: (−5.700, +7.000, **1.875**), facing −y | SalonGilt |
| 4 | sisley-flood-port-marly | 0.81 × 0.60 | `salon.bay1.south.1`: (−2.200, +7.000, 1.550), facing −y | SalonGilt |
| 5 | pissarro-hoarfrost | 0.93 × 0.65 | `salon.bay1.south.3`: (−9.200, +7.000, 1.550), facing −y | SalonGilt |
| 6 | monet-the-magpie | 1.30 × 0.89 | `salon.bay1.north.1`: (−2.400, −7.000, 1.550), facing +y | SalonGilt |
| 7 | monet-terrace-sainte-adresse | 1.30 × 0.98 | `salon.bay1.north.2`: (−9.400, −7.000, 1.550), facing +y | SalonGilt |

**Cabinet · Around the Group: Manet** (cabinet green; 3 works)

| # | Work (id) | w × h (m) | Anchor · centre (x, y, z) · facing | Frame |
|---|---|---|---|---|
| 8 | manet-luncheon-on-the-grass | 2.65 × 2.08 | `salon.cabinet.north.1`: (−6.000, −15.600, **1.640**), facing +y | SalonGilt |
| 9 | manet-olympia | 1.90 × 1.30 | `salon.cabinet.east.1`: (−2.000, −11.600, 1.550), facing −x | SalonGilt |
| 10 | manet-bar-folies-bergere | 1.30 × 0.96 | `salon.cabinet.west.1`: (−10.000, −11.600, 1.550), facing +x | SalonGilt |

**Bay 2 · Modern Paris** (zinc; 8 works)

| # | Work (id) | w × h (m) | Anchor · centre (x, y, z) · facing | Frame |
|---|---|---|---|---|
| 11 | pissarro-boulevard-montmartre-winter | 0.81 × 0.65 | `salon.bay2.north.1`: (−14.600, −7.000, 1.550) | SalonGilt |
| 12 | caillebotte-paris-street-rainy-day | 2.76 × 2.12 | `salon.bay2.north.2`: (−18.000, −7.000, **1.660**) | SalonGilt |
| 13 | pissarro-boulevard-montmartre-night | 0.65 × 0.53 | `salon.bay2.north.3`: (−21.400, −7.000, 1.550) | SalonGilt |
| 14 | monet-gare-saint-lazare | 1.04 × 0.75 | `salon.bay2.south.1`: (−14.200, +7.000, 1.550) | SalonGilt |
| 15 | caillebotte-floor-scrapers | 1.47 × 1.02 | `salon.bay2.south.2`: (−16.800, +7.000, 1.550) | SalonGilt |
| 16 | degas-absinthe | 0.69 × 0.92 | `salon.bay2.south.3`: (−19.400, +7.000, 1.550) | SalonGilt |
| 17 | degas-dance-class | 0.77 × 0.84 | `salon.bay2.south.4`: (−21.600, +7.000, 1.550) | SalonGilt |
| 18 | degas-little-dancer (scan) | h 0.994 | `salon.bay2.plinth`: plinth centre (−20.500, +3.400), facing +x (east, towards the arriving visitor) | plinth + vitrine |

North-wall works face +y and south-wall works face −y, in bays 2 to 5 as in bay 1.

**Bay 3 · Leisure and Light** (sage; 6 works)

| # | Work (id) | w × h (m) | Anchor · centre (x, y, z) | Frame |
|---|---|---|---|---|
| 19 | monet-la-grenouillere | 1.00 × 0.75 | `salon.bay3.north.1`: (−26.800, −7.000, 1.550) | SalonGilt |
| 20 | renoir-boating-party | 1.73 × 1.30 | `salon.bay3.north.2`: (−30.000, −7.000, 1.550) | SalonGilt |
| 21 | monet-woman-with-a-parasol | 0.81 × 1.00 | `salon.bay3.north.3`: (−33.200, −7.000, 1.550) | SalonGilt |
| 22 | renoir-dance-at-bougival | 0.98 × 1.82 | `salon.bay3.south.1`: (−26.800, +7.000, 1.550) | SalonGilt |
| 23 | renoir-bal-du-moulin | 1.75 × 1.31 | `salon.bay3.south.2`: (−30.000, +7.000, 1.550) | SalonGilt |
| 24 | monet-poppy-field | 0.65 × 0.50 | `salon.bay3.south.3`: (−33.200, +7.000, 1.550) | SalonGilt |

**Bay 4 · Women Impressionists** (old rose; 9 works)

| # | Work (id) | w × h (m) | Anchor · centre (x, y, z) | Frame |
|---|---|---|---|---|
| 25 | morisot-harbor-lorient | 0.73 × 0.44 | `salon.bay4.north.1`: (−37.900, −7.000, 1.550) | SalonGilt |
| 26 | morisot-summers-day | 0.75 × 0.46 | `salon.bay4.north.2`: (−39.600, −7.000, 1.550) | SalonGilt |
| 27 | morisot-the-cradle | 0.46 × 0.56 | `salon.bay4.north.3`: (−42.000, −7.000, 1.550) | SalonGilt |
| 28 | morisot-dining-room | 0.50 × 0.61 | `salon.bay4.north.4`: (−44.200, −7.000, 1.550) | SalonGilt |
| 29 | cassatt-childs-bath | 0.66 × 1.00 | `salon.bay4.north.5`: (−46.000, −7.000, 1.550) | SalonGilt |
| 30 | renoir-la-loge | 0.64 × 0.80 | `salon.bay4.south.1`: (−38.400, +7.000, 1.550) | SalonGilt |
| 31 | cassatt-in-the-loge | 0.66 × 0.81 | `salon.bay4.south.2`: (−39.600, +7.000, 1.550) | SalonGilt, width capped to keep 0.04 from #30 |
| 32 | cassatt-blue-armchair | 1.30 × 0.90 | `salon.bay4.south.3`: (−42.400, +7.000, 1.550) | SalonGilt |
| 33 | cassatt-boating-party | 1.17 × 0.90 | `salon.bay4.south.4`: (−45.400, +7.000, 1.550) | SalonGilt |

**Bay 5 · Monet's Series** (haystack; 3 works, 7 canvases)

| # | Work (id) | w × h (m) | Anchor · centre (x, y, z) | Frame |
|---|---|---|---|---|
| 34 | monet-stacks-of-wheat | 1.00 × 0.60 | `salon.bay5.south.1`: (−52.000, +7.000, 1.550) | SalonGilt |
| 35 | monet-bridge-water-lilies | 0.74 × 0.93 | `salon.bay5.south.2`: (−56.600, +7.000, 1.550) | SalonGilt |
| 36 | monet-rouen-cathedral, canvases 1–5 (images `-1` … `-5`) | 0.65 × 1.00 each | `salon.bay5.north.1` … `.5`: x −52.100, −53.200, −54.300, −55.400, −56.500; y −7.000; z 1.550 | SalonGilt |

**The oval · The Orangerie's second room** (4 works, 12 canvases). Collection records: `assets/collection.json`.
- Unframed, unvarnished oil on canvas: matt, with the impasto catching the light.
- Every canvas is **4.25 wide × 2.00 tall**, its bottom at **0.70**, curved to the wall on its stretcher, **35 mm proud** of the plaster.
- Joints between canvases are 1.5 mm hairlines, with a 12 mm strip of linen set back behind each joint.
- The four works run round from the door's north jamb, (−61.722, −1.500), with equal gaps of **0.919 m**. s is measured along the inner face:

| # | Work (id) | Canvases | s from → to | Start → end (x, y) |
|---|---|---|---|---|
| 37a | monet-willows-clear-morning (2 × 12.75) | 3 | 0.919 → 13.669 | (−62.057, −2.355) → (−73.031, −7.491) |
| 37b | monet-willows-two-willows (2 × 17.00), ahead across the axis | 4 | 14.589 → 31.589 | (−73.948, −7.435) → (−82.230, +3.498) |
| 37c | monet-willows-morning (2 × 12.75) | 3 | 32.508 → 45.258 | (−81.625, +4.189) → (−69.715, +7.256) |
| 37d | monet-willows-reflections-of-trees (2 × 8.50) | 2 | 46.178 → 54.678 | (−68.815, +7.067) → (−62.057, +2.355) |

- Each image reads left to right from the room.
- The first room's four panels (`monet-water-lilies-orangerie`) leave the hang; they are not built.

**The glass stele (fitting, `stele.glass.1200x2400`):**
- Frameless low-iron glass, `low-iron-glass.laminated.clean`, 0.050 thick, 1.200 wide (y +0.800 to +2.000) and 2.400 tall (z 0.080 to 2.480).
- Its mid-plane is at x −6.150.
- It stands on a foot of `brass.brushed.worn`: 0.40 (x −6.350 to −5.950) × 1.40 (y +0.700 to +2.100) × 0.080, its top edges eased 6 mm.
- The painting is on the east face, its back seen through the glass.

**The 1874 case (fitting, CaseBed anchor `salon.bay1.case`):**
- Centre (−6.100, −1.950); long axis along x; 1.60 × 0.70; 1.00 high to the glass top (0.99).
- **Carcase:** fumed oak (`oak.fumed.handled`) on a plinth recessed 40 mm to 0.09. The body runs to 0.80, framed and panelled: stiles and rails 5 mm proud, two fielded panels each side. A top rail from 0.80 to 0.86 stands 12 mm proud.
- **Hood:** low-iron glass (`low-iron-glass.toughened.clean`) from 0.864 to 0.990. Its twelve edges sit in 12 mm bronze angles (`bronze.patinated.handled`).
- **Interior:** a felt and linen floor (`linen.unbleached.clean` over `wool-felt.clean`).
- **Documents:**
  - The **1874 exhibition catalogue** (35 boulevard des Capucines, April 1874), pp. 14–15, open. It is **0.250 × 0.184** (the scan's 2540 × 1866), lying flat at local case-x −0.40.
  - **Le Charivari, 25 April 1874, p. 2**, the start of Louis Leroy's review. It is **0.320 × 0.473** (the scan's 2350 × 3470), on a felt board raised 10 mm at case-x +0.28.
  - Public-domain scans: `assets/salon/manifest.json` and CREDITS.md.

**The Little Dancer:**
- A scan of the Met's bronze cast: `assets/sculptures/degas-little-dancer.mvm`, from Sketchfab, **CC-BY** (the credit is required). Height 0.994.
- It stands on a plinth 0.80 × 0.80 × 1.00, `travertine.honed.clean` (the as-built plinth was matte cream), at (−20.500, +3.400).
- Its vitrine is low-iron glass, 0.76 × 0.76 from 1.00 to 2.10, `low-iron-glass.toughened.clean`.
- The label follows the record: "Pigmented beeswax with fabric, hair and ribbon" names the NGA's wax original, and the bronzes are posthumous.

**Captions:** every work, both documents and the dancer are IMuseeCaptioned. A caption shows artist, title, original title, date, medium, collection and a line of notes. Its text comes from `data/artworks.json` and `assets/collection.json`, through Art's catalogue.

**Care summary:**

| Class | Items |
|---|---|
| `oil-and-tempera` | 39 canvases, the stele painting, 12 willow canvases |
| `works-on-paper` | the catalogue |
| `works-on-paper.fugitive` | Le Charivari |
| `textiles.silk` | the Little Dancer |

---

## 9 · Sound

**Acoustics** (Sound.ReverberationFromMatter):

| Enclosure | Volume | Character |
|---|---|---|
| `salon.bays` | ≈ 60 × 14 × 12 ≈ 10,000 m³ | A long stone and plaster volume. The silk on felt and the velvet absorb the mids and highs. Expect a long, warm tail. |
| `salon.oval` | ≈ 1,550 m³ | Hard lime plaster in an ellipse. Its foci on the axis are at x −64.453 and −80.547, beside the two oval viewing stones, so expect focusing (the whispering gallery). |
| `salon.cabinet` | ≈ 350 m³ | Silk-lined; the driest room. |
| `salon.stair` | | Brick and stone; bright. |

The RT60 comes from these volumes and Matter's absorption, never tuned.

**Emitters:**
- **Rain on glass**, over each lantern's outer glazing and over the oval's glass roof. It is placed on those Glazed openings' exposed surfaces and driven by the **giverny** observer's rain (D-005). You hear it on the glass above the cloth.
- **The pond's hydraulic power unit:** a soft pump hum and valve clicks. It is attached to the `salon.pond` body by key and driven by FMuseeActuation.
- **Footfalls** by substance: oak parquet, polished travertine, the honed travertine treads.

There is no other emitter. The still pond is silent.

---

## 10 · Visitor

**Walk:**
- The line down the axis stays clear from `salon.east` to the oval door (≥ 2.3 between the case and the stele in bay 1).
- Step-to (the VR arc) favours the 18 viewing stones.

**Sit:**
- **Velvet benches ×8:** backless, with seat frames facing either wall, two per face at ±0.60 along the bench.
- **Oval benches ×4:** facing out to the willows, seat frames every 0.70 along the curve.
- **The pond coping:** a seat at 0.42 all round, with frames every 0.80 at d 0.25, facing out or in. The coping is 0.42 high and is not a step: if the human measure's step height is < 0.42, the body cannot climb it. Nothing invisible guards the water.

**Use:**

| Where | Verb | Prompt | Message |
|---|---|---|---|
| The golden lily | wake | "Wake the golden lily" | "A line of light runs round the rim. Use the golden lily again to lift the pond." |
| The golden lily | lift | "Lift the pond" | "The pond rises on four bronze posts…"; then "Beneath the pond, a long stair leads down to the Reserve." |
| The golden lily | lower | "Lower the pond" | |
| ⚑ The stair-foot plate | raise | "Raise the pond" | |

Prompts show within 6.50 of the pond's centre, or at the plate.

**Look:** a caption appears on each work, the case's documents and the dancer.

**Eye:** the dim bays to the bright white oval is an adaptation walk. The Eye owns it; nothing in the stage sets exposure.

---

## 11 · The old code map (re-express, never copy)

All paths are under `C:\Users\anan9\Desktop\Projects\Musee Vision` unless marked Integration.

| Part | Where the numbers are |
|---|---|
| Bays, piers, doors, slots (constants) | `windows/Source/MuseeVision/Salon/SalonStructure.cpp:26-52` |
| Vault, lantern and coffer constants | `…/SalonStructure.cpp:54-60` |
| Cabinet constants | `…/SalonStructure.cpp:62-65` |
| Oval constants; the pond opening | `…/SalonStructure.cpp:67-72` |
| Interior-update constants (pier silk, voussoirs, strips, seals, muslin, pergola) | `…/SalonStructure.cpp:74-91` |
| Vault stations, coffer pitch, plain band, column x, door stations | `…/SalonStructure.cpp:106-134` |
| Walls with round-arched openings (WallFace), reveals (Tunnel), architrave frames | `…/SalonStructure.cpp:155-282` |
| Profiles P1–P10 (cornice, skirting, pier base, impost, architraves, cabinet cornice, oval cove and skirting) | `…/SalonStructure.cpp:376-476` |
| Oval ellipse, offset outer face, passage block, wall columns | `…/SalonStructure.cpp:488-574` |
| Pier silk panels | `…/SalonStructure.cpp:624-640` |
| Outer archivolt P7; voussoirs and keystone | `…/SalonStructure.cpp:650-799` |
| Bays: silk walls, piers, bases, imposts, spandrels | `…/SalonStructure.cpp:801-857` |
| Coffer steps; rosette carving | `…/SalonStructure.cpp:859-937` |
| Lanterns (rect, shaft, glass, opal, muslin, steel grid); lunettes | `…/SalonStructure.cpp:939-1057`; `LanternRect`/`DiffuserZ` `:2114-2127`; `Scripts/salon_interior.py:696-705` |
| Bay 3 pergola, planters, vines | `…/SalonStructure.cpp:1059-1201` |
| Cornice runs; skirtings | `…/SalonStructure.cpp:1203-1266` |
| Envelope: outer faces, port and internal doors, roof and lantern holes, sealed void | `…/SalonStructure.cpp:1268-1342` |
| Cabinet: silk, cornice, ceiling, laylight well, roof | `…/SalonStructure.cpp:1344-1396` |
| Oval: walls, cove, velarium, skirting, outer face, roof, passage masonry | `…/SalonStructure.cpp:1398-1517`; velarium rim `:2129-2136` |
| Floors: parquet and stone zones, brass strips, viewing-stone inlay, pond corners | `…/SalonStructure.cpp:1519-1736` |
| Stele foot | `…/SalonStructure.cpp:1738-1752` |
| Light slots (cove, silk top, silk base) | `…/SalonStructure.cpp:1754-1840` |
| Viewing-stone centres | `…/SalonStructure.cpp:1920-1927` |
| Pond edge: constants, ellipse stations, blocks and joints | `windows/Source/MuseeVision/Salon/SalonPond.cpp:19-166` |
| Pond frieze design (shapes, composition) | `…/SalonPond.cpp:168-441` |
| Pond coping profiles (upper, lower) | `…/SalonPond.cpp:443-521` |
| Pond lip, rim-light lengths, lining, bed, shelves, tray | `…/SalonPond.cpp:734-767, 916-967` |
| Pond outline tests (the old push-out: do **not** re-express) | `…/SalonPond.cpp:705-721` |
| Pond planting (iris, carex, forget-me-not) | `…/SalonPond.cpp:1127-1349`; `windows/Source/MuseeVision/Salon/SalonInterior.h:213-239` |
| Pond lift: states, timings, uses, auto-lower | `windows/Source/MuseeVision/Salon/PondLift.cpp:19-26, 120-250`; `PondLift.h:11-24` |
| Pond lift: easing and scaled posts (replaced by M-1) | `…/PondLift.cpp:252-274` |
| PondPlan: posts, golden lily, basin, stair, shaft | `Shared/Wings/Reserve.swift:11-32` |
| Lilies (plan) | `Shared/Plan/MuseumPlan.swift:119-125`; `windows/Source/MuseeVision/Nature/MuseeWaterLilies.h:9-23`, `.cpp:395-428` |
| Velvet benches | `windows/Source/MuseeVision/Salon/SalonInterior.cpp:62-114`; `.h:18-24` |
| 1874 case | `…/SalonInterior.cpp:116-150, 300-378`; `.h:56-98` |
| Willows (wall line, canvas build) | `…/SalonInterior.cpp:152-196, 420-495`; `Scripts/salon_interior.py:54-59, 1048-1067` |
| Oval benches (moved out for the coping) | `windows/Source/MuseeVision/Furniture/MuseeFurniture.cpp:75-101, 205-231` |
| Thresholds (strips) | `…/MuseeFurniture.cpp:305-328` |
| Hang (sizes, positions, centre rule); Salon plan numbers | `Shared/Plan/Hang.swift:1-115`; `Shared/Plan/MuseumPlan.swift:39-136` |
| Stele glass, dancer plinth and vitrine | `Shared/Wings/Salon.swift:261-283` |
| Frames (SalonGilt profile and widths) | `windows/Source/MuseeVision/Frames/MuseeFrame.cpp:302-372, 1027-1035`; `Scripts/frames.py:1-60` |
| Long stair, shaft, strings, rails, lights | `windows/Source/MuseeVision/ReserveStair/ReserveStairStructure.h:13-50`; `.cpp:28-65, 129-469`; `windows/Source/MuseeVision/Plan/MuseePlan.h:346-368` |
| Reserve west door; the Reserve's top under the Salon | `windows/Source/MuseeVision/Reserve/ReserveStructure.cpp:37-39, 979-1015`; `MuseePlan.h:376-384` |
| Rotunda west passage (for the joint) | `MuseePlan.h:61-63`; `windows/Source/MuseeVision/Rotunda/RotundaStructure.cpp:297-313, 655-668`; `Shared/Wings/Rotunda.swift:183-212` |
| Silk and velvet colours; parquet pattern; ashlar; diffusers τ; velarium; pond materials | `Scripts/salon_interior.py:49, 239-334, 337-561, 707-793, 865-919` |
| Giverny weather source | `windows/Source/MuseeVision/Salon/SalonSky.cpp:29-31, 151`; `SalonSky.h:13-28`. Its lux constants are **not** re-expressed; Light computes daylight. |
| Rain-on-glass sound (procedural) | `windows/Source/MuseeVision/Salon/SalonRain.h:8-39` |
| Old Salon lighting (reference only: floating lights, 650 lx) | `windows/Scripts/gallery_lights.py:24-53, 144-245` |
| Exposure zone (a patch: **not** re-expressed) | `windows/Scripts/salon_interior.py:999-1020` |
| Exterior dress of the Salon, oval and cabinet (polish) | `windows/Source/MuseeVision/Facade/MuseeFacadeStructure.h:12-45`; `MuseePlan.h:518-561` |
| Boards | `plan/proposals/salon-interior/boards/{Light,Plan,Oval,Details,Main}.png`; `plan/canvas/{Salon,SalonPlan,SalonDetails,SalonHang,SalonReserve}.dc.html`; `plan/README.md:50-65, 84-92` |
| Care and luminaire catalogue used above | Integration `C:\mv\wt\r-light\MuseeVision\Data\Art\Care.json`, `…\Data\Light\Luminaires\*.json`, `…\Data\Light\UseTargets.json` |
| Matter records used above | Integration `C:\mv\wt\r-matter\MuseeVision\Data\Matter\{Substances,Workings,Histories}` |

---

## 12 · First-pass cut (tonight, D-016)

**Built tonight**, all through the systems from this sheet:

1. **Shell:**
   - the four walls with their openings and reveals; the ports' rough openings;
   - piers with bases, imposts and silk panels; transverse arches (voussoirs, keystone, archivolt, spandrels);
   - the vault with coffers and the 750 rosettes (instanced); the lunettes and cornices P1; skirtings P2;
   - the roof slab;
   - the five lantern shafts with inner panes, the grid, bay 2's opal, bay 4's muslin and the **pitched outer glazing**;
   - bay 3's pergola frame and planters;
   - plain honed ashlar outer faces and the passage block.
2. **Cabinet:** walls, silk, P8, ceiling, laylight well, the L6 luminous laylight and the roof.
3. **Oval:** walls, cove P9, the velarium cloth, the **glass roof**, skirting P10, the door passage with P5 and P6.
4. **Floors:** the Versailles parquet laid as pieces in bays and cabinet; travertine zones; brass strips; 18 viewing stones.
5. **The stair well:** shaft, curb, soffit, head wall and class opening, 36 treads with nosings, landings, strings, handrails with L9, and the L10 downlights.
6. **The pond:**
   - the coping blocks (upper and lower profiles, joints) with a **plain die**; the lip; the bronze vessel, shelves and tray;
   - the posts and sleeves; the guard rails; the golden lily;
   - the water body; the 10 Nymphaea; the marginals.
   - Mechanisms M-1 and M-2 with states, uses (including the stair-foot plate), the occupancy guard, the coupling and channel `salon.pond.state`, and L11 following it.
7. **Art:** all 39 canvases framed at true size; the stele and Impression, Sunrise; the 12 willow canvases; the 1874 case with its two documents; the Little Dancer on her plinth in her vitrine; 8 velvet and 4 oval benches; captions; care targets.
8. **Light:** L1–L11 schemed and commissioned; daylight through the six Glazed openings with the Giverny observer's cloud; targets §4.3 and design targets §4.4 measured.
9. **Sound:** acoustics for the four enclosures, rain on the Glazed openings (giverny), footfalls and the pump's voice.
10. **Site:** ports `salon.east` and `salon.stair` with the served well; the footprint; enclosures; blinds and the blind well on the stand.

**Waits for polish:**
- The **carved lily frieze** (the Ornament height field; the die is plain tonight).
- The **Vitis vines** on the pergola. Bay 3's light is the bars' shadow until they grow; the owner will see a missing chapter.
- The exterior dress: podium, giant order, blind windows, balustrades, the portico.
- Lantern-shaft night lighting for the opal and muslin lanterns.
- A per-opening cloud field, so that the sky seen through the glass matches Giverny.
- The parquet's worn lane.
- Each voussoir's and coping block's individual tone, polish and lippage beyond Matter's defaults.
- The oval's whispering-gallery tuning.
- A high-output projector record to cut the spot count.

---

## Risks and open points

1. **Port class at `salon.east`.** The Salon's door is 3.0 × 4.8 (springing 3.3). The Rotunda's west drum arch is 3.0 wide and springs at 4.5 (crown 6.0). As built, the Rotunda's tunnel ran 2.2 m at 6.0 m to the Salon's lower arch. Mate needs one class. This sheet declares `doorway.3000x4800.arched`, because a 6.0 m crown would cut the cornice at 5.6. The Rotunda drafter must declare the same class and step the drum arch down inside the drum's 1.2 m wall. The Architect decides.
2. **Accent spot throw.** The board's "spot 4.5 m over the work, 30° out" has nothing to hang from under a 13.5 m vault. Re-expressed as coffer mounts, the throw is 12–13 m at ≈ 26°. The catalogue's 2,000 lm 12° spot then gives ≈ 60 lx per spot on the work, so ≈ 3 per work and ≈ 110 spots in all. Light should add a high-output narrow projector (tier 2), or accept the count.
3. **Sun patches and care.**
   - Clear lanterns in bays 1, 3 and 5 put direct sun on the north walls, at z 0.45 to 2.5 when the altitude is ≈ 55–64°. That could reach north-wall works, the stele painting and the 1874 case under the idealised sun.
   - The case's paper (50 lx) sits beside bay 1's lantern, so diffuse daylight alone may exceed 50 lx.
   - D-014 allows only the hang and UV glass to fix this. Art.WithinCare will name the works; the owner decides any move.
4. **The daylight-only willows.** With glass and a cloth alone the willows would see thousands of lux at a bright noon. The oval roof needs a fixed daylight control sized at commissioning so the brightest reference moment lands at ≤ 200 lx. That is ≈ 0.5–1 % total transmission. The record is missing from Light's daylight-control catalogue.
5. **Giverny weather and one sky.** Only cloud attenuation, rain and wetness follow Giverny. The sky seen through the glass remains the museum's until Light has a per-opening cloud field. A dry museum sky over a rain-streaked lantern is visible from outside (the accepted D-005 exception).
6. **Re-expressions of as-built fakes,** which the owner should see on Sunday's walk:
   - the pitched outer lanterns;
   - the glass roof over the velarium;
   - the electric cabinet laylight;
   - the gilt-bronze golden lily with no glow;
   - the telescopic posts;
   - the guard rails round the pond opening (an addition; the as-built used invisible walls);
   - the stair-foot "Raise the pond" plate (needed for Site.Reachable).
7. **The stair's pace.** 2R + T = 0.661 against the human measure's 0.63 (the as-built stair; 5.8 m in 36 risers within 16 m of shaft). Form.StepsFitTheBody must allow ±0.03, or the stair is a design change.
8. **Volumes, not polygons.** The Salon sits on the Reserve, the same plan 5 cm apart. Site.NoOverlap and Site.RoomStaysInside must judge footprints with z ranges. The cabinet also overhangs the Reserve's lawn-side top (at −0.33).
9. **A hydraulic drive with no counterweight** (≈ 19 t on 4 rams). Motion.Balanced must accept drive capacity as the balance.
10. **Matter gaps tonight.** Only ten substances exist. This stage needs oak, silk (6 dyes), velvet (4), cotton muslin, felt, linen, brick, water (dyed black), opal glass, soil, and gilt wood for the frames. It also needs the working "filled, carved" travertine. Textiles must declare their core-family tie (Matter.OneLanguage).
11. **Light catalogue gaps:**
    - a luminous laylight (L6);
    - a handrail LED;
    - a 2700 K strip for the lip (the as-built colour; 3000 K is specified meanwhile);
    - a fixed louvre or film for the oval roof;
    - a control class for a luminaire that follows a mechanism channel (L11).
12. **The Reserve drafter** must declare `reserve.stair` with class `stairwell.straight.2400x3200.arched` at level −5.80, facing west at the Reserve's west wall's outer face, and drop the as-built floor notch.
13. **Heavy pieces.** The frieze (a 2.5 mm height field over 21 m × 0.2 m) and the parquet (≈ 530 panels × ≈ 30 pieces) are the stage's largest piece counts; watch the cast time and Nanite budget.
14. **Lily placement.** One plan lily sat over the east shelf; it is moved 0.15 m (Nature.WithinBotany).
15. **Nature.InSeason.** The indoor pond needs an indoor (heated) phenology profile distinct from the roof vines'.
