# The Reserve · stage sheet

**Stage:** `reserve`. Module `MuseeReserve` (Editor), class `UReserveRoom : USiteRoom`, in
`Source/Stages/Rooms/Reserve`. Plan numbers live only in `Private/ReservePlan.h`.

**What it is:** the museum's picture store, a brick undercroft under the whole Salon and the lawn either side of
it. A nave 60 m long runs between two arcades of chamfered brick piers on travertine bases and imposts, under
brick groin vaults. In the two aisles, 36 top-hung picture screens of fumed oak faced in natural linen stand
edge-on to the nave. Draw one out and it glides 5.65 m along its bronze track into the nave with its paintings.
Opal globes hang in every bay, small uplights on the imposts wash the vaults, plan chests hold the two Degas
pastels, and an oak easel on the axis at the east end holds *Le Pont de l'Europe*. There is no door: the lily pond
in the Salon's Nymphéas oval lifts 2.6 m, and beneath it a long straight stair runs down east into this stage's
receiving well and arrives through an arched door in the Reserve's west wall.

**Sources, in the order D-001 ranks them:** D-001, D-006, D-012, D-014, D-016 (`Journal/Decisions.md`);
`Docs/Architecture.md` (stages, How rooms join, Naming); the approved Salon-interior proposal
(`plan/proposals/salon-interior`: the pond "as planned", which still lifts; nothing below the floor changes); the
master boards `SalonReserve` and `AsBuiltReserve` (`plan/canvas/*.dc.html`); `plan/README.md:90-94`; the as-built
code (`windows/Source/MuseeVision/Plan/MuseePlan.h`, `Reserve/*`, `ReserveStair/*`, `Salon/PondLift.cpp`,
`Scripts/native.py`, `reserve_materials.py`, `materials.py`, `frames.py`) and `Shared/Wings/Reserve.swift`.
Where a board and the code disagree, the code is the later as-built state and wins (listed in section 13).

---

## 1. Identity and placement

**Frame.** Metres. x is east, y is south, z is up, and plan north is true north (Foundation's `FMuseeFrame`).

**Origin.** The centre of the nave on the Reserve's finished floor: midway between the end walls' inner faces and
on the axis, at floor level.

**Placement in the museum plan.** The origin sits at museum plan **(x −44.00, y 0.00, z −5.80)**: level −1, 5.80 m
below the Rotunda's floor. Site reaches it by translation only: Rotunda → `rotunda.west ↔ salon.east` → Salon →
`salon.stair ↔ reserve.stair` → Reserve. The conversion is

  local x = plan x + 44.00 · local y = plan y · local z = plan z + 5.80.

The axis x = −44.00 in plan is also the Salon's portico axis (`MuseePlan.h:551`), so the Reserve's origin lies
under the portico's centre line.

**What lies above it** (why its top is stepped):
- the Salon's five bays and its floor (plan z 0.00), whose outer faces are x −74.6 … −13.4, |y| ≤ 7.6 in plan;
- the Nymphéas oval and the hyphen west of the Salon, over the shaft limb;
- the lawn (soil surface plan z −0.03) either side, the Manet cabinet (plan x −24.4 … −15.6, y −16.0 … −7.6) over part
  of the north aisle, and the portico's stylobate (plan x −55.6 … −32.4, y 7.6 … 12.9) over part of the south aisle.

**Levels (local z).**

| Surface | local z | plan z |
|---|---|---|
| Outer bottom of the box (walls' footings, slab underside) | −0.35 | −6.15 |
| Bottom of the receiving well (the Salon's landing bed) | −0.15 | −5.95 |
| Finished floor, hall | 0.00 | −5.80 |
| Springing (tops of the imposts) | 4.00 | −1.80 |
| Webs' springing (6 cm above the bands) | 4.06 | −1.74 |
| Aisle web crowns / aisle band crowns | 5.31 / 5.25 | −0.49 / −0.55 |
| Nave web crowns / nave band crowns | 5.61 / 5.55 | −0.19 / −0.25 |
| Shaft soffit (under the shaft's roof slab) | 5.40 | −0.40 |
| Top of the box under the lawn | 5.47 | −0.33 |
| Top of the box under the Salon's floor (and over the shaft limb) | 5.75 | −0.05 |
| Mating plane of the port (the oval's finished floor) | 5.80 | 0.00 |

**Footprint.** Kind **Building**. It is a volume (a plan polygon with a z range per region), because the Reserve
lies wholly under other stages (see risks). Plan polygon, local, counter-clockwise from the shaft limb's
north-west corner:

(−46.80, −1.60), (−30.60, −1.60), (−30.60, −12.60), (30.60, −12.60), (30.60, 12.60), (−30.60, 12.60), (−30.60, 1.60),
(−46.80, 1.60).

| Region | Plan extent (local) | z range (local) |
|---|---|---|
| Hall, under the Salon (raised top) | x −30.55 … 30.55, \|y\| ≤ 7.55 | −0.35 … 5.75 |
| Hall, under the lawn (rest of the hall) | x −30.60 … 30.60, \|y\| ≤ 12.60, less the row above | −0.35 … 5.47 |
| Shaft limb | x −46.80 … −30.60, \|y\| ≤ 1.60 | −0.35 … 5.75 |
| Port column (inside the limb, up to the mating plane) | x −46.40 … −38.90, \|y\| ≤ 1.20 | 5.75 … 5.80 |

Site.RoomStaysInside judges every Reserve piece against this volume minus the receiving well (section 2).

**Enclosures.** One: **`reserve.undercroft`, kind Enclosed.** Its shell is the hall and the shaft limb together.
The arched door between them is a void in an interior cross wall, not a shell opening. The shell has exactly one
opening, the **Port** `reserve.stair` at the top of the shaft limb (section 2), plus four **Housing** bores for the
Salon's pond posts, which are blind holes in the shaft walls and do not pierce the enclosure. There is no Glazed
or Open opening. `Form::EnclosureAt` returns `reserve.undercroft` anywhere in the hall or on the stair.

---

## 2. Ports

The Reserve has **one port**. It declares no other doorway, stairwell, shaft or gate.

### `reserve.stair`

| Field | Value |
|---|---|
| kind | **Stairwell** |
| class | **`stairwell.straight.2400x7500`**: clear opening 2.40 m (y) × 7.50 m (x) at the mating plane |
| frame origin (local) | (−42.650, 0.000, 5.800): the centre of the opening, on the mating plane |
| frame axes | normal **+z** (up, out of the Reserve); first axis +x (east, along the opening's long side) |
| face shape | flat (horizontal) |
| wall depth | 0.30: the collar depth below the mating plane |
| plan position | (−86.650, 0.000, 0.000), in the floor of the Nymphéas oval, under the lily pond |
| mates | **`salon.stair`**: the same rectangle in the oval's floor, facing −z. Passage 0.000 m. |
| well | **served by the Salon, received by the Reserve** (D-012) |

**D-012 topology.** In `SitePlan.json` the line is `salon.stair ↔ reserve.stair`, well served by the Salon. The Salon's
lifting pond sits on the mating plane and closes the well when it is down. The Salon owns everything inside the
well. The Reserve reserves the well as a void and builds its shell round it.

**The well** (local; in plan subtract 44.00 from x and 5.80 from z). It is the union of two boxes and four bores:

| Part | x | y | z | What it holds (the Salon's) |
|---|---|---|---|---|
| W1, the opening column | −46.40 … −38.90 | −1.20 … 1.20 | −0.15 … 5.80 | top landing, upper flight, the pond's seat on the plane |
| W2, the lower shaft | −38.90 … −30.60 | −1.20 … 1.20 | −0.15 … 5.40 | lower flight, bottom landing, handrails, rail lights, soffit downlights |
| B1 to B4, the post bores | centres (−45.50, ±1.35) and (−39.50, ±1.35), Ø 0.160 | | 3.00 … 5.80 | the four bronze pond posts retracted (Ø 0.150, 5 mm clear) |

As an x–z section extruded across y ±1.20, W1 ∪ W2 is the polygon (−46.40, −0.15), (−30.60, −0.15), (−30.60, 5.40),
(−38.90, 5.40), (−38.90, 5.80), (−46.40, 5.80). The bores are sized for the worst case, single-stage posts sliding
the full 2.60 m lift plus 0.20 m for the post's foot. If the Salon's spec uses telescoping posts, the bores shorten
to its collapsed length and the change is made in this table.

- **Receiving side: the Reserve.** It builds nothing inside W1, W2 or the bores. Every Reserve piece round the well
  stands at |y| ≥ 1.20, x ≤ −46.40, at or above z 5.40 over W2, at or below z −0.15 under it, and at or east of
  x −30.60, where the well's east face is the west face of the Reserve's cross wall.
- **Serving side: the Salon.** Section 3.10 lists what it builds in the well, for coordination.

**Site's joint at this seam (the well collar).** Site draws it once for the class:
- a travertine curb 0.30 m deep lining all four faces of the opening, from the mating plane down to z 5.50;
- a brushed-bronze ring flush in the oval floor round the opening, lying on the mating plane;
- four holes in the collar where the posts pass (centres as B1 to B4).

The Reserve's rough faces at the collar are the tops of its two shaft walls (y ±1.20), its west end wall
(x −46.40) and the east edge of its shaft roof slab (x −38.90, z 5.40 … 5.75). They are set back by the class's
rough-opening allowance from `PortClasses.json` over the collar depth. Below the collar, the brick faces lie on the
clear lines.

**On the stand** the port gets a **blind**: a cap over the 7.50 × 2.40 m opening on the mating plane. No blind well
is needed on the receiving side, because nothing of the Reserve moves through the well. With the flight absent,
the hall's floor reaches the well's foot but not the port. Site's reachability on the stand must count the well's
floor (W2 at z −0.15, joined to the hall through the arched door) as reaching `reserve.stair` (see risks).

---

## 3. Pieces by trade

Brick is UK metric stock, 215 × 102.5 × 65 mm, in 10 mm lime mortar: 75 mm courses and 225 mm stretcher gauge
(`materials.py:969-970`, and `CourseHeight`/`BlockLength` in `MI_Brick_Coursed`). Every piece bears on its
neighbour at a shared face. There are no 5 cm sinks and no 2 cm overlaps: the old code used them to hide cracks,
and the kernel's shared stations replace them.

### 3.1 The walls (brick, 0.60 m)

| Wall | x | y | z | Notes |
|---|---|---|---|---|
| North | −30.60 … 30.60 | −12.60 … −12.00 | −0.35 … 5.47 | under the lawn and the cabinet |
| South | −30.60 … 30.60 | 12.00 … 12.60 | −0.35 … 5.47 | under the lawn and the portico's stylobate |
| West (the cross wall) | −30.60 … −30.00 | −12.60 … 12.60 | −0.35 … top | top 5.75 for x ≥ −30.55 and \|y\| ≤ 7.55, else 5.47; the arched door on the axis |
| East | 30.00 … 30.60 | −12.60 … 12.60 | −0.35 … top | top 5.75 for x ≤ 30.55 and \|y\| ≤ 7.55, else 5.47 |

The walls' inner faces rise to the top. The vault's webs and bands meet them along their wall arches, and the
haunch fill is laid against them.

**The arched door in the west wall** (`ReserveStructure.cpp:37-39, 979-1016`):
- clear width **2.38 m** (y ±1.19, 1 cm inside the shaft's faces at ±1.20);
- jambs vertical from 0.00 to the springing at **2.00 m**, then a semicircular arch of radius 1.19 about
  (y 0, z 2.00), crown **3.19 m**;
- the reveal runs through the full 0.60 m (x −30.60 … −30.00);
- the arch is turned in two brick rowlock rings, 0.215 m each, and shows on both faces.

**The threshold** (the as-built landing's run into the room, `MuseePlan.h:350-353`): one honed travertine stone,
x −30.60 … −29.70, y −1.19 … 1.19, top at 0.00, bed at −0.15. It fills the floor's notch 0.30 m into the hall. It
meets the Salon's bottom landing on the plane x −30.60, the well's east face, in a real 3 mm stone joint.

### 3.2 The shaft limb (the receiving well's shell)

`ReserveStairStructure.cpp:45-49, 203-234`, re-owned by the receiving side.

| Piece | x | y | z | Substance |
|---|---|---|---|---|
| North shaft wall | −46.80 … −30.60 | −1.60 … −1.20 | −0.35 … 5.75 for x ≤ −38.90; −0.35 … 5.412 under the roof slab | brick |
| South shaft wall | −46.80 … −30.60 | 1.20 … 1.60 | −0.35 … 5.75 for x ≤ −38.90; −0.35 … 5.412 under the roof slab | brick |
| West end wall | −46.80 … −46.40 | −1.20 … 1.20 | −0.35 … 5.75 | brick |
| Base slab (the well's floor) | −46.40 … −30.60 | −1.20 … 1.20 | −0.35 … −0.15 | concrete, cast |
| Roof slab over W2, bearing on the side walls | −38.90 … −30.60 | −1.60 … 1.60 | 5.412 … 5.75 | concrete, cast |
| Soffit finish, its face the well's top | −38.90 … −30.60 | −1.20 … 1.20 | 5.400 … 5.412 | lime plaster, 12 mm, trowelled |

- The four post bores are cut in the shaft walls (section 2), leaving 70 mm of brick between each bore and the
  shaft's face.
- The side walls meet the west cross wall's west face at x −30.60.
- The brick of the shaft's faces is the hall's brick. Handrail roses and strings (the Salon's) are fixed to them.

### 3.3 The floor

**Hall slab:** polished concrete, x −30.00 … 30.00, y −12.00 … 12.00, from −0.35 to **0.00**. The top is power-floated
and polished (`materials.py:976-983`, and 1546-1552 for the as-built look: charcoal grey, clear-coated).
- **Sawcut panels, 1.50 × 1.50 m**, 4 mm joints, laid by `Form::Lay` from Matter's concrete bay size. The joint grid
  is on the pier centre lines in x (x = 1.5 k) and symmetric about the axis in y (y = ±0.75 + 1.5 k).
- **The notch:** x −30.00 … −29.70, y ±1.19, down to −0.15, for the threshold stone.

**The 36 guide slots** (`ReserveStructure.cpp:61-67, 480-491, 1042-1068`; widened so the fin clears 5 mm, see
section 6). One slot per screen, in the screen's plane (x = the screen's x, table 3.8), running from |y| 1.20 to
|y| 11.45 on the screen's side of the axis:
- a floor cut 30 mm wide;
- a bronze channel inside it, with 20 mm clear width and 45 mm depth, closed at the ends and the bottom;
- 5 mm lips flush with the floor either side.

### 3.4 The piers (14) and responds (18)

No classical order: a plain undercroft pier (look target 04). **Piers** stand on the two arcade lines **y ±5.00**,
at **x −22.50, −15.00, −7.50, 0.00, 7.50, 15.00, 22.50** (plan pitch 7.50 from plan x −66.5).

- **Base: honed travertine**, a solid square block swept from this profile (half-width from the pier's centre,
  height), closed to the axis at z 0.00 and z 0.54. Name `base.square-splayed`, singular to this stage, so it stays
  in `ReservePlan.h`.

  `[(0.45, 0.00), (0.45, 0.36), (0.41, 0.44), (0.41, 0.50), (0.37, 0.54)]`

- **Shaft: brick**, 0.70 m square with 40 mm chamfers on the four arrises, from 0.54 to 3.64. The plan octagon
  (half-widths) is (0.35, −0.31), (0.35, 0.31), (0.31, 0.35), (−0.31, 0.35), (−0.35, 0.31), (−0.35, −0.31),
  (−0.31, −0.35), (0.31, −0.35). English bond, with the chamfer in cut brick.
- **Impost: honed travertine**, a solid square block, closed to the axis at 3.64 and 4.00. Name
  `impost.square-splayed`. Its top face at **4.00** is the springing of every arch. The corners of the top outside
  the bands (0.35 … 0.45 from the centre) are the ledges the uplights stand on.

  `[(0.37, 3.64), (0.41, 3.69), (0.45, 3.78), (0.45, 4.00)]`

**Responds** (`ReserveStructure.cpp:433-451, 1031-1040`) stand where a band springs from a wall:
- on the north and south walls at each pier x (14);
- on the west and east walls at y ±5.00 (4).

Each respond has, measured out from the wall face (d) and centred on its line:
- **Brick pilaster** 0.70 wide, d 0.00 … 0.12, from 0.54 to 3.64.
- **Travertine base**, two steps: 0.90 wide at d 0.00 … 0.22 from 0.00 to 0.36, then 0.82 wide at d 0.00 … 0.18 from
  0.36 to 0.54.
- **Travertine impost**, 0.90 wide, profile (d, z): `[(0.00, 3.64), (0.14, 3.64), (0.22, 3.76), (0.22, 4.00), (0.00, 4.00)]`.

### 3.5 The vaults (brick)

`ReserveStructure.cpp:14-24, 175-343, 1072-1113`. There are 24 groin vaults: 8 bays × 3 rows.

**Bays, numbered 1 to 8 west to east** (clear spans between wall and pier faces, x):

| Bay | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| from | −30.00 | −22.15 | −14.65 | −7.15 | 0.35 | 7.85 | 15.35 | 22.85 |
| to | −22.85 | −15.35 | −7.85 | −0.35 | 7.15 | 14.65 | 22.15 | 30.00 |
| centre | −26.425 | −18.75 | −11.25 | −3.75 | 3.75 | 11.25 | 18.75 | 26.425 |

**Rows:**
- north aisle y −12.00 … −5.35 (half 3.325, centre −8.675), rise R **1.25**;
- nave y −4.65 … 4.65 (half 4.65), rise R **1.55**;
- south aisle y 5.35 … 12.00, rise R **1.25**.

**Web intrados** over bay b and row r. Two semi-elliptical barrels of equal rise and level crowns meet, so the groins
fall on the bay's diagonals in plan:

  z = 4.06 + max( R_r · √(1 − v²), R_r · √(1 − u²) ), with u = (x − x_b)/h_b and v = (y − y_r)/h_r.

Crowns are 5.61 in the nave and 5.31 in the aisles. Sample both arches at the same angular stations (the old code
used 40), so the groins are shared edges.

**Bands**, 6 cm below the webs, all springing at 4.00:
- **Cross bands** over every pier line (7 lines × 3 rows = 21): 0.70 wide (pier x ± 0.35). The intrados is the row's
  arch, 4.00 + R_r · √(1 − v²), with crowns 5.55 in the nave and 5.25 in the aisles. The aisle bands spring from the
  wall responds.
- **Arcade bands** along y ±5.00 ± 0.35 over every bay (8 × 2 = 16). The intrados is the bay's arch at the aisle
  rise, 4.00 + 1.25 · √(1 − u²), crown 5.25. The end bays' arcade bands spring from the end-wall responds.
- **Band faces:** each band's side faces rise from its intrados to the web beside it. The aisle side rises 6 cm.
  The **nave side of each arcade band rises as a lunette** at |y| 4.65, from 4.00 + 1.25 f(x) to 4.06 + 1.55 f(x).
- **Over each pier**, the springing plane closes the vault surface at z 4.00 within x, y ± 0.35.

**Thickness and fill.** The webs are **half-brick shells, 0.1025 m** normal to the intrados, laid in stretcher bond
with the courses parallel to each barrel's axis. They meet at the groins with cut brick. The bands are the same
shell deepened by 0.06 (0.1625 m). Above the extrados the **haunches are filled solid in brick** up to the top of
the box (5.75 under the Salon, 5.47 under the lawn). The fill is sealed and never seen. The clearances are:
- nave crown extrados 5.7125, 3.75 cm under the Salon's zone;
- aisle crown extrados 5.4125, 5.75 cm under the lawn top.

A one-brick vault would not fit under the as-built floors (see risks).

**The top** is flat at 5.75 over x −30.55 … 30.55, |y| ≤ 7.55 and over the shaft limb, and at 5.47 elsewhere. It has
vertical 0.28 m risers at |y| 7.55 and at x ±30.55. Its upper face is the brick's waterproofed back (substance brick,
history buried).

### 3.6 Mouldings

Only the two travertine profiles of 3.4. The bands' arrises are square. There is no cornice or string course.

### 3.7 Stairs

**None of the Reserve's own.** The long stair is in the well and belongs to the Salon (3.10).

### 3.8 Fittings drawn by the Reserve: tracks and screens

The screens are singular to this stage. They are drawn here as the bodies of 36 Motion mechanisms (section 6) and
carry Art's Wall anchors.

**Screens** (`MuseePlan.h:398-429`, `ReserveRacks.cpp:33-54, 171-256`). The six lettered screens A to F are the first
six north ones. Names follow `AReserveRacks::RackName`.

| Bay | Bay centre x | Screen x (−1.90, 0, +1.90) | North aisle | South aisle |
|---|---|---|---|---|
| 1 | −26.25 | −28.15, −26.25, −24.35 | A, B, C | S01, S02, S03 |
| 2 | −18.75 | −20.65, −18.75, −16.85 | D, E, F | S04, S05, S06 |
| 3 | −11.25 | −13.15, −11.25, −9.35 | N07, N08, N09 | S07, S08, S09 |
| 4 | −3.75 | −5.65, −3.75, −1.85 | N10, N11, N12 | S10, S11, S12 |
| 5 | 3.75 | 1.85, 3.75, 5.65 | N13, N14, N15 | S13, S14, S15 |
| 6 | 11.25 | 9.35, 11.25, 13.15 | N16, N17, N18 | S16, S17, S18 |

The screens are placed on the pitch centres (plan −70.25 + 7.5 b), not the clear-span centres. Bays 7 and 8 hold the
plan chests.
- **Home:** screen centre |y| **9.15**; it spans |y| 6.65 … 11.65, 0.35 m off the side wall.
- **Out:** screen centre |y| **3.50**; it spans |y| 1.00 … 6.00, 1.00 m off the axis. The travel is 5.65 m toward the
  axis.

**One screen**, in its carriage frame: origin at the screen's floor centre; x across (faces at ±x, east +); y along the
screen, with the nave end toward +y for north screens and −y for south; z up.

| Part | Extent | Substance |
|---|---|---|
| End posts (2) | 0.09 along × 0.16 thick, at y ±2.455; z 0.07 … 3.40; 8 mm eased arrises | oak, fumed |
| Top rail | x ±0.05, y ±2.42, z 3.29 … 3.40 | oak, fumed |
| Kick rail | x ±0.05, y ±2.42, z 0.07 … 0.22 | oak, fumed |
| Board (core) | x ±0.041, y ±2.412, z 0.216 … 3.294 | oak, fumed (hidden) |
| Linen faces (2) | the board's faces at x ±0.043, over the same area, laid in piece-local metres so the weave rides with the screen | linen, natural unbleached |
| Picture rails (2) | on each face from x ±0.043 to ±0.058, z 3.255 … 3.286, y ±2.414, ends returned into the posts | bronze, brushed |
| Pull | on the nave end post: a Ø 22 mm bar from z 0.95 to 1.60, 58 mm off the post on two Ø 14 mm stand-offs at 1.00 and 1.55 | bronze, brushed |
| Trolley saddles (2) | on the top rail at y ±2.10: 0.072 × 0.14 × 0.014 | bronze, brushed |
| Trolley stems (2) | Ø 16 mm, z 3.41 … 3.61 (into the track), with nut Ø 32 mm × 16 mm | bronze, brushed |
| Guide shoes (2) | under the kick rail at y ±2.20: 0.044 × 0.14, z 0.05 … 0.074 | bronze, brushed |
| Guide fins (2) | 9 mm thick × 0.10 long, z −0.030 … 0.052, running in the floor slot (20 mm clear: 5.5 mm each side) | bronze, brushed |
| Picture-light troughs | on faces with works only (4.2): a trough 64 × 44 mm at x ±0.42, centre z 3.32, over the lamp span; two Ø 14 mm arms from the top rail to the trough at 0.12 m in from the span's ends, with plates on the rail | bronze, brushed |

Hanging rods and hooks belong to Art's hang (section 8), not to the screen.

**Tracks**, one per screen, fixed (`ReserveRacks.cpp:292-332`; slot widened for 5 mm clearance, section 6). The track
runs along y at the screen's x, from the side wall's face (|y| 11.998) to **|y| 0.90**:
- **Channel:** 60 mm wide (x ±0.030), from z **3.58** (underside) to **3.70** (top). It has a 12 mm top plate, two
  8 mm side walls, and lips underneath leaving a **26 mm slot** (x ±0.013) for the trolley stems.
- **Nave-end stop:** a bronze block 68 mm × 12 mm, z 3.574 … 3.704.
- **Wall plate:** 0.15 × 0.042, z 3.51 … 3.76, fixed to the brick by four bronze bolts.
- **Hanger rods (3 per track):** at |y| **7.60, 5.00 and 1.20**. Each is a Ø 14 mm patinated bronze rod from the track
  top up to the vault's intrados at that point (5.00 meets the arcade band). It has a clamp on the track and a
  Ø 76 mm × 50 mm canopy against the brick.

| Track part | Substance |
|---|---|
| Channel, stop, plate, bolts | bronze, brushed |
| Rods, canopies | bronze, patinated |

### 3.9 Luminaire bodies drawn by the Reserve

Section 4 names the types. The bodies are drawn in Form's language from these as-built profiles. Light's
`ULightLuminaireComponent` drives the lamp and the diffuser (the globe) from one record.

**Pendants (24)** (`ReserveStructure.cpp:69-78, 110-121, 835-950`). One pendant hangs at each bay's clear-span centre
(x as in the bay table):
- **nave (8):** y 0.00, globe **Ø 0.44** (17 in), centre z **3.95**, hung on a **chain** from a canopy at the nave web
  crown, z 5.61;
- **aisles (16):** y ±8.675, globe **Ø 0.34** (13 in), centre z **4.42**, hung on a **Ø 15 mm rod** from a canopy at the
  aisle crown, z 5.31.

Each pendant is built as follows, with R the globe's radius:
- **Globe (lathe):**
  - a sphere of radius R cut at the neck, neck radius 0.34 R;
  - a neck 0.12 R tall;
  - a rolled lip at the top: `[(0.34R, zc), (0.34R, top − 0.006), (0.34R + 0.004, top − 0.004), (0.34R + 0.005, top − 0.002), (0.34R − 0.004, top)]`,
    where zc is the cut height and top = zc + 0.12 R.
- **Gallery:** spun bronze, sheet 1.5 mm. Lathe profile (r, z − zc):

  `[(0.52R, −0.013), (0.535R, −0.009), (0.52R, −0.005), (0.44R, 0.004), (0.41R, 0.008), (0.41R, 0.030), (0.38R, 0.035), (0.20R, 0.041), (0.13R, 0.043)]`

- **Socket cup:** lathe profile (r, z − zc), with cup top 0.10:

  `[(0.13R, 0.042), (0.13R, 0.091), (0.15R, 0.093), (0.15R, 0.097), (0.12R, 0.100), (0.04R, 0.104), (0, 0.104)]`

- **Canopy** at the crown Cz, with k = 1.00 for the nave and 0.85 for the aisles. Profile (r, z − Cz):

  `[(0.070k, 0.020), (0.070k, −0.006), (0.064k, −0.020), (0.045k, −0.034), (0.020k, −0.041), (0.012, −0.043), (0.012, −0.050), (0, −0.050)]`

  The top 0.02 enters the brick.
- **Nave chain:** links of 6.4 mm round wire, 28 mm inside length and 12 mm inside width, at 28 mm pitch, each turned
  90° to the last. A 9.5 mm loop of 6 mm wire ends the chain at the canopy and another at the cup.
- **Aisle rod:** Ø 15 mm from the cup top + 0.002 up to Cz − 0.066. A Ø 26 mm swivel ball at Cz − 0.062 and a
  Ø 22 mm hexagonal locknut, 12 mm, on the cup.
- **Thumb screws:** three through the gallery's band at 90° + 120° k, bearing on the neck under its lip.

**Uplights (56)** (`ReserveStructure.cpp:80-81, 783-828`). Four stand on each pier's impost corner ledges:
- lens centre at (pier x ± 0.40, pier y ± 0.40, **4.095**);
- aimed up the groin: 58° above horizontal, azimuth along the diagonal away from the pier (±45°, ±135°);
- body: a bronze can Ø 44 mm × 75 mm, the lens recessed 6 mm behind a 4 mm bezel, on a Ø 14 mm knuckle stem to the
  ledge.

**Easel spot (1):** a monopoint canopy on the nave web of bay 8 at (24.40, 1.90, 5.475), a Ø 12 mm bronze stem down
to the head at **(24.40, 1.90, 4.25)**, aimed at the easel painting's centre (28.945, 0.00, 1.515). This is the
as-built head position (`native.py:128`), given here in local coordinates.

### 3.10 In the well: the Salon's pieces, listed for coordination

The Reserve builds none of these. The numbers are the as-built ones (`MuseePlan.h:346-368`,
`ReserveStairStructure.cpp:28-65, 129-199, 253-465`), in the Reserve's frame.

**Flight:**
- 36 risers of 5.80/36 = **0.16111 m**, goings **0.3389 m** (slope 0.47539);
- first nosing at x −46.00; riser k at x −46.00 + 0.3389 k; last riser at x **−34.1385**;
- honed travertine treads, each one stone with a 25 mm nosing 40 mm deep, a 12 mm round on top and a 12 mm undercut;
- **Blondel: 2R + G = 0.661 m.** The Salon should check this against Form.StepsFitTheBody's tolerance (pace 0.63).

**Landings:**
- top landing flush with the oval floor (z 5.80), x −46.40 … −45.975;
- bottom landing at z 0.00, x −34.1385 … −30.60, wall to wall.

**Closed strings:** travertine, 25 mm proud of the brick, 0.24 above the nosings and 0.35 below, turning into a
0.15 m skirting on the bottom landing.

**Handrails:**
- bronze, Ø 50 mm with a flat underside carrying the rail's LED line (the chord from 240° to 300°);
- 0.90 m above the nosings, on the line y ±1.115;
- the rail starts at x −44.00 (where it passes under the oval's floor), turns a knee of radius 0.15 at x −33.80,
  runs level along the landing and returns into the wall at x −30.90;
- brackets about every 1.3 m: a wall rose Ø 60 mm, an arm Ø 16 mm and a stem.

**Stair light:** the rail LED lines and two surface downlights (bronze cans Ø 100 mm, 80 mm drop) on the Reserve's
soffit at x −36.20 and −32.40, y 0.

**The pond:**
- the lily pond, its basin, water and lilies;
- four bronze posts Ø 0.150 at (−45.50, ±1.35) and (−39.50, ±1.35);
- the lift: 2.60 m in 5 s (`Salon/PondLift.cpp:20-23`, `Reserve.swift:11-19`).

**A call control at the foot of the stair** (see 10 and risks), so the Reserve is never sealed with the pond down.

### 3.11 Fittings drawn by Art, placed at the Reserve's anchors

**The easel** (Art fitting `easel.oak`, `ReserveStructure.cpp:568-588`) stands at anchor `reserve.easel`. The front
faces of its uprights are at **x 28.95**, on the axis, facing west.

| Part | x | y | z | Substance |
|---|---|---|---|---|
| Runners (2) | 28.55 … 29.45 | ±0.505 … ±0.595 | 0.00 … 0.09 | oak, fumed |
| Cross foot | 29.25 … 29.33 | −0.59 … 0.59 | 0.00 … 0.09 | oak, fumed |
| Uprights (2) | 28.95 … 29.03 | ±0.515 … ±0.585 | 0.09 … 2.35 | oak, fumed |
| Caps (2) | 28.945 … 29.035 | ±0.51 … ±0.59 | 2.35 … 2.38 | brass, brushed |
| Lower rail | 28.955 … 29.025 | −0.515 … 0.515 | 0.35 … 0.43 | oak, fumed |
| Upper rail | 28.955 … 29.025 | −0.515 … 0.515 | 2.18 … 2.26 | oak, fumed |
| Ledge brackets (2) | 28.84 … 28.96 | ±0.50 … ±0.60 | 0.70 … 0.75 | brass, brushed |
| Ledge | 28.84 … 28.97 | −0.62 … 0.62 | 0.75 … 0.80 | oak, fumed |
| Ledge lip | 28.84 … 28.87 | −0.62 … 0.62 | 0.80 … 0.835 | oak, fumed |

**Plan chests:** four islands (Art fitting `plan-chest.walnut`, two chests back to back per island,
`ReserveStructure.cpp:538-566`, `MuseePlan.h:454-458`).
- **Positions:** island centres at x **18.75** (bay 7) and **26.25** (bay 8), y **±8.675**.
- **Plinth:** x ±1.05, y ±1.00, z 0.00 … 0.09, 50 mm set back; patinated bronze.
- **Carcass:** x ±1.10, y ±1.05, z 0.09 … 0.865; walnut.
- **Top:** x ±1.12, y ±1.07, z 0.865 … **0.900**; walnut.
- **Drawers:** ten per side (y = ±1.05 faces), in two columns of five.
  - Fronts are 1.064 × 0.1364 with 12 mm gaps, starting at x −1.07 and z 0.115, 15 mm proud of the carcass and 25 mm
    thick. Walnut.
  - Pulls are brass bars 140 × 18 mm, 22 mm proud, centred on the front at 0.62 of its height.
- **Felt mats** on the two south islands only (5 mm, z 0.900 … 0.905): 0.99 (x) × 0.76 (y) in bay 7 under *The Tub*,
  and 0.58 × 0.74 in bay 8 under *The Star*.

### 3.12 Glazing

None in the stage's fabric. The only glass is the pastels' glazing, which is Art's (section 8), and the globes'
opal, which is Light's diffuser.

### Materials used by the Reserve's pieces

Keys are `substance.working.history`. Matter resolves each spec as a struct; Matter's own record keys contain dots,
so the string is never parsed.

| Key | Where | Family |
|---|---|---|
| `brick.sand-faced.aged` | walls, piers, responds, vault webs, bands, lunettes, shaft walls, door arch | stone (accent: fired clay) |
| `brick.sand-faced.buried` | haunch fill, the top's back, the box's outer faces | stone (accent) |
| `travertine.honed.worn` | pier and respond bases, the threshold | stone |
| `travertine.honed.aged` | imposts | stone |
| `concrete.polished.worn` | hall floor (traffic field applied) | stone (accent) |
| `concrete.cast.buried` | shaft base slab, shaft roof slab | stone (accent) |
| `lime-plaster.trowelled.aged` | shaft soffit | white |
| `bronze.brushed.worn` | the 36 slot linings | bronze |
| `bronze.brushed.handled` | tracks, stops, plates, bolts, picture rails, pulls, trolley parts, fins, picture-light bodies, pendant galleries, cups, canopies, chain, uplight cans, spot stem | bronze |
| `bronze.patinated.aged` | track hanger rods and canopies, aisle pendant rods, chest plinths | bronze |
| `oak.fumed.handled` | screen posts, rails and cores; the easel | oak |
| `linen.stretched.aged` | screen faces (natural unbleached; Matter's canvas substance, linen fibre) | accent of white |
| `walnut.planed.waxed` | plan chests | oak (accent) |
| `brass.brushed.handled` | chest pulls, easel caps and brackets (D-014 accent) | bronze (accent) |
| `felt.pressed.new` | pastel mats | accent |
| `opal-glass.cased.new` | pendant globes, shaded by Light's diffuser master | glass |

---

## 4. Light

### 4.1 Daylight

None. The Reserve is buried, with no Glazed or Open opening. When the pond is up, the Salon's light (the oval's
velarium, daylight under Giverny's weather) falls down the well through the port. That is transport across a
joint, not a daylight opening of this stage, so it carries no daylight coefficient here.

### 4.2 Luminaires (catalogue type, count, position; never lumens)

All are **3000 K**. As built they were 2900 K (`native.py:102-122`, `reserve_materials.py:42`). 3000 K is the nearest
catalogue colour and lies in paper's care range of 2800 to 3000 K.

| Type (catalogue key) | Count | Position (section 3.9) | Control class | Evidence |
|---|---|---|---|---|
| `pendant-globe.3000k.430mm` | 8 | nave, one per bay, y 0.00, globe centre 3.95 | circulation | **to add**: a cased opal globe pendant, lamp inside, the globe as diffuser |
| `pendant-globe.3000k.330mm` | 16 | aisles, one per bay, y ±8.675, globe centre 4.42 | circulation | **to add** |
| `uplight-mini.3000k.wide` | 56 | four per pier at the impost corners, z 4.095, 58° up the groins | circulation | **to add**: a small shadowing can, about a 45 mm lens, wide beam |
| `track-spot.3000k.12deg` | 1 | monopoint head at (24.40, 1.90, 4.25), aimed at the easel painting | gallery | in the catalogue (iGuzzini Palco QG54) |
| `picture-light.3000k.linear` | 9 | on the lettered screens' faces that carry works (table below): a trough at x ±0.42, lens at z 3.2975, riding the screen | gallery, following the screen's channel | **to add**, lit. The catalogue's `picture-light.2700k` is evidence-incomplete and would stand unlit. |

**Picture-light spans** (y in the carriage frame): the works' extent plus 0.10 m each end, half-length clamped to
0.35 … 1.90 (`ReserveRacks.cpp:150-169`).

| Face | A west | A east | B west | B east | C west | D west | D east | E west | E east |
|---|---|---|---|---|---|---|---|---|---|
| span, ± (m) | 0.595 | 1.09 | 0.78 | 0.42 | 1.90 | 0.52 | 0.595 | 0.515 | 0.445 |

### 4.3 Targets

Light holds these at every reference moment while the stage is occupied.

| Target | Surface (local) | Class / care | Level |
|---|---|---|---|
| Works on a screen at its **out** stop | each work's face (section 8 positions, out) | Art care `oil-and-tempera` | 150 recommended, 200 max lx |
| Works on a screen at **home** | none: in store, picture light off | | |
| The easel painting | *Le Pont de l'Europe* on the easel | `oil-and-tempera` | 150 … 200 lx |
| The pastels | chest beds at z 0.905, bays 7 and 8 south | `works-on-paper` | ≤ 50 lx; annual dose 150 000 lx·h |
| Nave walking zone | floor, x −30.00 … 30.00, \|y\| ≤ 4.65; and the threshold | `circulation` | 100 lx maintained, Uo 0.4 |
| Aisles | floor between screens and chests | `circulation` | 100 lx |
| Empty store | floor, same zones | `orientation` | 5 lx |

**Operation.**
- The Reserve is a store: all its light is **presence-linked** (D-006). Circulation runs while a visitor is in
  `reserve.undercroft` and falls to orientation when it is empty.
- The easel spot and the picture lights are gallery class, presence-linked. Each picture light also follows its
  screen's channel `reserve.screen-<name>.state` (section 6): it is on only at the out stop.
- The aisle pendants light the pastels, so presence-linking is what keeps the pastels' annual dose (see risks).

### 4.4 Design targets (luminance order, measured, never knobs)

In the nave with screen C out, from the foot of the stair and from mid-nave:
1. the lit works (the easel painting, and the works on the out screen) are the brightest non-emitting surfaces in view;
2. the vault webs near the groins (uplit) are brighter than
3. the brick piers' faces at eye height, which are brighter than
4. the aisle walls and the linen of screens at home, which are brighter than
5. the floor (dark polished concrete), the darkest surface, which carries the globes' reflections.

Ratio: mean luminance of the lit works ≥ 3 × mean luminance of the vault webs in the same view.

Retired: the Reserve's own 4300 K white-balance volume (`native.py:117-121, 351-364`). The Eye's white balance must
carry a 3000 K room (Visitor.OneEye).

---

## 5. Time and weather

- **No glazed opening, so no weather place.** No opening of this stage names `museum`, `giverny` or `london`.
- **The sun never enters.** No surface has sky exposure. Matter's live wetness, standing water and snow are zero
  everywhere, and the Reserve carries no rain history.
- The Moment is read only by Light's operation (presence) and by Sound (the Salon's rain, heard faintly down the
  well when the pond is up).
- Time's reference moments still run for Light.MeetsTarget. The Reserve's results are the same at every moment,
  which is the check that nothing leaks in.

---

## 6. Motion

### 6.1 The picture screens: 36 prismatic mechanisms (the Reserve's)

**Keys and states.**
- Keys: `reserve.screen-a` … `reserve.screen-f`, `reserve.screen-n07` … `reserve.screen-n18`,
  `reserve.screen-s01` … `reserve.screen-s18`.
- Body: the screen's pieces (3.8) and its anchors.
- Joint: **prismatic along y**, +y out for north screens and −y out for south.
- States: **home** (centre |y| 9.15) and **out** (centre |y| 3.50). **Travel 5.65 m.**

**Drive.** Top-hung on two trolleys in its own track, guided by two fins in the floor slot.
- **Speed 1.0 m/s, acceleration 0.6 m/s², jerk 1.2 m/s³.** Full travel takes about 7.8 s.
- As built it glided in 4.5 s on a smoothstep, peaking at 1.9 m/s and 1.7 m/s² (`ReserveRacks.h:109-111`,
  `.cpp:737-744`). That is too fast and hard for a 5 m oak screen passing beside a visitor. The choice is recorded
  in the risks for the owner.

**Use** (IMuseeUsable on each screen; FText verbs):
- "Draw out screen {name}" at home, "Slide screen {name} home" when out.
- The pull is the use shape. A proximity prompt is offered within 1.6 m of a screen (`ReserveRacks.cpp:852-868, 941-949`).

**Control group `reserve.screens`: at most one screen out.** Drawing one out sends any other home first
(`ReserveRacks.cpp:836-843`). This is a **sequence rule, not an interlock**: the travel volumes are disjoint (the
screens are 1.90 m apart), so the mechanisms are not coupled. Configurations stay additive: 1 + 36 × sampled
phases, never 2³⁶.

**Occupancy guard (Motion's).**
- A screen holds while the visitor's body is in its next travel slice, and resumes when the slice is clear
  (`ReserveRacks.cpp:746-766`: the body's radius + 0.03 m against the screen's footprint, frames 0.135 m off its plane).
- Caption: "Screen {name} waits: you are in its way".

**Channel.** Each screen publishes `reserve.screen-<name>.state` (home, moving, out). The Reserve declares the key
and owns both ends: the mechanism and its picture light.

**Initial configuration:** **screen C out**, all others home.

### 6.2 Clearances (Motion.Clears ≥ 5 mm at every sampled phase)

| Pair | Clearance |
|---|---|
| Guide fin (9 mm) in floor slot (20 mm) | 5.5 mm each side; fin bottom 15 mm above the slot floor |
| Trolley stem (Ø 16) in track slot (26 mm) | 5 mm each side |
| Screen top (3.40) and saddles (3.412) to track underside (3.58) | 0.168 m |
| Screen bottom (0.07) and shoes (0.05) to floor | 50 mm |
| Screen at home to side wall | 0.35 m |
| Nearest screen to a pier base (e.g. C at −24.35 to the pier at −22.50, base half 0.45) | 1.32 m |
| Out screen's pull (\|y\| 0.93) to the nave globe (y 0, r 0.22) | 0.71 m in y, and 2.0 m apart in x |
| Picture-light trough (top about 3.34) to the aisle globe's bottom (4.25) | 0.91 m |
| Track top (3.70) to the lowest arch over it (the arcade band at the screen's x: 5.018 over C, the lowest) | ≥ 1.31 m |

Motion.Housed: at home the screen stands in its aisle bay, which is its housing. No mechanism of the Reserve
enters the well.

### 6.3 The Salon's pond, as it touches the Reserve

- The pond lift is the Salon's mechanism, above the mating plane.
- Its four posts travel down into the Reserve's receiving bores B1 to B4 (Ø 0.160 for Ø 0.150 posts).
- The Salon's spec must declare the posts' travel volumes inside the well for Motion.Clears and Motion.Housed.

### 6.4 Interlocks

None of the Reserve's own. Across the seam, the pond's occupancy guard (the Salon's) keeps the pond from settling
onto a visitor on the upper flight. Site's reachability needs a use from below, the call control at the stair's
foot (3.10), or the Reserve is a trap in the pond-down configuration.

---

## 7. Nature

- **Inside:** nothing. No plants, no water body, no precipitation. The shell is sealed and buried.
- **Above:** the lawn's soil (0.30 m) lies on the Reserve's top where it is under the Grounds (top local 5.47, plan
  −0.33). The Grounds keeps its terrain above it (`grounds.md`: 0.10 m clear of −0.33).

---

## 8. Art

**The collection held here: 13 works**, 11 paintings on screens A to E and 2 pastels on the chests.
- They are **not in `assets/collection.json`**. Their records are in `data/artworks.json`. Their sizes are from
  `MuseePlan.h:431-440` (and `Reserve.swift:43-57`), matching the `SalonReserve` board's "what is down here" list.
- Art.TrueSize reads Collection.json, which must take these 13 with the holders' published dimensions.

| Work (id) | Artist, title, date | Medium | Size h × w cm (board) | W × H m (plan) | Anchor | Care |
|---|---|---|---|---|---|---|
| `caillebotte-pont-europe` | Caillebotte, *Le Pont de l'Europe*, 1876 | oil on canvas | 125 × 180 | 1.80 × 1.25 | `reserve.easel` (start); home `reserve.screen-a.east` | oil |
| `caillebotte-young-man-window` | Caillebotte, *Young Man at His Window*, 1876 | oil on canvas | 116 × 81 | 0.81 × 1.16 | `reserve.screen-a.west` | oil |
| `degas-place-de-la-concorde` | Degas, *Place de la Concorde*, 1875 | oil on canvas | 78 × 118 | 1.18 × 0.78 | `reserve.screen-b.west` | oil |
| `degas-orchestra-opera` | Degas, *The Orchestra at the Opera*, c. 1870 | oil on canvas | 57 × 46 | 0.46 × 0.57 | `reserve.screen-b.east` | oil |
| `renoir-cirque-fernando` | Renoir, *Acrobats at the Cirque Fernando*, 1879 | oil on canvas | 131 × 99 | 0.99 × 1.31 | `reserve.screen-c.west`, slot 1 | oil |
| `renoir-two-sisters` | Renoir, *Two Sisters (On the Terrace)*, 1881 | oil on canvas | 101 × 81 | 0.81 × 1.01 | `reserve.screen-c.west`, slot 2 | oil |
| `renoir-the-swing` | Renoir, *The Swing*, 1876 | oil on canvas | 92 × 73 | 0.73 × 0.92 | `reserve.screen-c.west`, slot 3 | oil |
| `pissarro-red-roofs` | Pissarro, *Red Roofs*, 1877 | oil on canvas | 55 × 66 | 0.66 × 0.55 | `reserve.screen-d.west` | oil |
| `pissarro-avenue-opera-snow` | Pissarro, *Avenue de l'Opéra, Snow Effect*, 1898 | oil on canvas | 65 × 81 | 0.81 × 0.65 | `reserve.screen-d.east` | oil |
| `sisley-bridge-villeneuve` | Sisley, *The Bridge at Villeneuve-la-Garenne*, 1872 | oil on canvas | 50 × 65 | 0.65 × 0.50 | `reserve.screen-e.west` | oil |
| `sisley-snow-louveciennes` | Sisley, *Snow at Louveciennes*, 1878 | oil on canvas | 61 × 51 | 0.51 × 0.61 | `reserve.screen-e.east` | oil |
| `degas-the-tub` | Degas, *The Tub*, 1886 | pastel on card | 60 × 83 | 0.83 × 0.60 | `reserve.bay7.south-chest` | works on paper |
| `degas-the-star` | Degas, *The Star*, c. 1878 | pastel over monotype on paper | 58 × 42 | 0.42 × 0.58 | `reserve.bay8.south-chest` | works on paper |

Screen F and the 30 unlettered screens carry nothing; they are kept for acquisitions (the board).

**Anchors declared by the Reserve.**

| Anchor | Kind | Carried by | Frame (local, at home) |
|---|---|---|---|
| `reserve.screen-a.west` … `reserve.screen-e.east` (the nine faces with works) | Wall | the screen's body | the face's plane: work backs at screen x ∓ 0.065 (west −, east +); row centred on the screen centre; centre height 1.55; facing ∓x |
| `reserve.easel` | Plinth (the easel fitting) | fixed | the uprights' front plane x 28.95, y 0.00; ledge top 0.80; facing −x |
| `reserve.bay7.north-chest`, `reserve.bay7.south-chest`, `reserve.bay8.north-chest`, `reserve.bay8.south-chest` | CaseBed (the chest fittings) | fixed | island centres (18.75 or 26.25, ∓8.675); bed at 0.905 on the felt (south ones); 0.900 on the tops (north ones) |

**The hang** (`MuseePlan.h:417-418`, `ReserveRacks.cpp:100-127`):
- **On a screen face:** works in one row, centred along the screen, **centre height 1.55 m**, **0.35 m between frames**.
  The backs are **65 mm off the screen's centre plane** (22 mm off the linen).
- **Hardware:** each work hangs from the face's bronze picture rail by **two patinated bronze rods Ø 4.4 mm**. They
  stand at ±(W/2 + 0.09 − 0.07) from the work's centre, crooked over the rail's top at z 3.295 and dropping behind
  the rail's lip, with a sleeve Ø 9.7 mm × 45 mm at the frame's hanger.
- **Row on C west (carriage y):** cirque −1.30, sisters +0.13, swing +1.43. Every other face has one work at y 0.

**Work centres** (local; faces west unless noted; z 1.55):

| Work | x | y at home | y out |
|---|---|---|---|
| Young Man | −28.215 | −9.15 | −3.50 |
| Concorde | −26.315 | −9.15 | −3.50 |
| Orchestra (east face) | −26.185 | −9.15 | −3.50 |
| Cirque Fernando | −24.415 | −10.45 | **−4.80** |
| Two Sisters | −24.415 | −9.02 | **−3.37** |
| The Swing | −24.415 | −7.72 | **−2.07** |
| Red Roofs | −20.715 | −9.15 | −3.50 |
| Avenue de l'Opéra (east) | −20.585 | −9.15 | −3.50 |
| Bridge at Villeneuve | −18.815 | −9.15 | −3.50 |
| Snow at Louveciennes (east) | −18.685 | −9.15 | −3.50 |

The out column is the start state for C.

**The easel:** *Le Pont de l'Europe* stands with its frame on the ledge at 0.80 and its back on the uprights, back
plane at **x 28.945**, facing west. Its centre is at z 0.80 + 0.09 + 1.25/2 = **1.515**, and the frame's top at 2.23.

**Frames:** Art's carved gilt frame, **90 mm face** (the Salon's gilt at 0.8 of its width, `frames.py:13, 47-48`);
`oak.gilt.aged` (gilt working over a wood substrate, the bole showing with wear). Every painting is a real canvas
object: stretched linen with tacking edges, ground and varnish, a pine stretcher with keys. Its back is seen from
the screen's other side and from the easel's back (the `AsBuiltReserve` board: "every painting has a back").

**The pastels:**
- Each is **paper in a window mat** (cotton museum board, 80 mm border: outer 0.99 × 0.76 for *The Tub*, 0.58 × 0.74
  for *The Star*), **glazed** with UV-filtering low-iron glass on a 6 mm spacer.
- Each lies flat on its felt, top edge to the south, read from the arcade side (y < 8.675).
- As built they lay bare on the felt. Glazing is how a pastel is really kept, and it is Art's paper object, not a
  room choice.

**Care:** oil 150 to 200 lx; paper 50 lx with 150 000 lx·h a year (`Data/Art/Care.json`). Art sends these as
FLightTargets (section 4.3).

**Casts and scans:** none.

**Not ported:** the send-to-easel carry. As built, using a painting on an out screen carried it in 5 s, with a
0.40 m lift arc, to the easel and back (`ReserveRacks.cpp:996-1060`). It needs Art to re-hang a work between two
anchors as a use. It waits for polish (section 12), and meanwhile the easel holds *Le Pont de l'Europe* and A's east
face is empty.

---

## 9. Sound

**Emitters:** none as built. Footfalls come from `FMuseeContact` by substance underfoot:
- polished concrete in the hall;
- honed travertine on the threshold and, in the well, the Salon's treads.

**Mechanism voices:** the screens' trolleys rolling in their bronze tracks and the soft stop at the end of travel
(`FMuseeActuation`, attached by the screen's key). This is polish, not first pass.

**Acoustic character**, derived by Sound from Form's facts and Matter's absorption, never tuned:
- about 7 000 m³: the hall about 6 900 m³ under the vaults, plus the shaft;
- surfaces are brick walls and vaults, polished concrete, and about 1 200 m² of linen on the 36 screens;
- a Sabine estimate puts the mid-frequency decay near 3 s, with the linen as the main absorber: a live masonry
  undercroft where footsteps carry down the nave. This is the expected outcome for Sound.ReverberationFromMatter,
  not a setting.

**Occlusion:**
- The buried shell blocks all outside sound, so there is no rain under the vault.
- With the pond up, the port couples the Reserve to the Salon's oval. The Salon's rain on its velarium and the
  pond's lift are heard faintly down the well (Sound.Occluded through the one opening).

---

## 10. Visitor

**Arrival and departure** are only by the long stair. In the Salon, use the golden lily twice and the pond rises.
Walk down the 36 steps under the oval, through the arched west door, and the nave runs 60 m east to the easel. To
leave, walk back up. With the pond down and the visitor below, the **call control at the stair's foot** (the Salon's,
3.10) raises it. Its FText is "Raise the pond".

**What one does:**
- **Walk** the nave and the aisles between the screens.
- **Use a screen:** "Draw out screen C" / "Slide screen C home", by beam or gaze on the pull or by the proximity
  prompt within 1.6 m.
  - One screen is out at a time: drawing another sends the first home.
  - A screen waits while you stand in its way: "Screen C waits: you are in its way".
- **Look:** each work's caption comes from Art (IMuseeCaptioned) at the screen faces, the easel and the chests.
- **Sit:** none. There is no seat in the Reserve as built. A study bench before the easel is an owner decision for
  polish.

**Start of a visit in this stage:** screen C out with its three Renoirs in the nave; *Le Pont de l'Europe* on the easel.

**Hands:** the Quest beam targets the pull and the works (Visitor.ActionsCovered).

**Eye:** a 3000 K room under museum-wide white balance; see risks.

---

## 11. The old code map

Re-express these through the new systems; do not copy them. Paths are under
`C:/Users/anan9/Desktop/Projects/Musee Vision/`.

| What | Where | Port it as |
|---|---|---|
| Reserve plan numbers | `windows/Source/MuseeVision/Plan/MuseePlan.h:370-459` | `ReservePlan.h` in the local frame (x + 44, z + 5.8) |
| Screens, works, easel, chests numbers | `MuseePlan.h:398-458` | 3.8, 3.11, 8 |
| Long stair numbers | `MuseePlan.h:346-368` | the well (2) and 3.10 (the Salon's) |
| Structure overview | `Reserve/ReserveStructure.h:10-56` | the sheet's sections 1 to 3 |
| Constants: walls, door, pier, base, impost, rises, top, slots, pendants, uplights | `Reserve/ReserveStructure.cpp:25-81` | `ReservePlan.h` |
| Bays, rows, pendant list | `ReserveStructure.cpp:88-121` | 3.5, 3.9 |
| Semi-elliptical arches (shared stations) | `ReserveStructure.cpp:175-215` | Form's arch/loft with shared stations |
| Groin webs | `ReserveStructure.cpp:217-271` | Form: groin vault by rule (two barrels, max) |
| Cross bands | `ReserveStructure.cpp:273-301` | Form bands |
| Arcade bands and lunettes | `ReserveStructure.cpp:303-343` | Form bands with lunette faces |
| Pier base, impost sweeps, chamfered shaft | `ReserveStructure.cpp:345-399` | Form square sweep of the 3.4 profiles; laid brick shaft |
| Responds and corbels | `ReserveStructure.cpp:401-451` | 3.4 |
| Slots and floor with holes | `ReserveStructure.cpp:480-536, 1042-1068` | Form.Lay floor with slots; bronze channels |
| Plan chests | `ReserveStructure.cpp:538-566, 1130-1136` | Art fittings (3.11) |
| Easel | `ReserveStructure.cpp:568-588` | Art fitting (3.11) |
| Chain link | `ReserveStructure.cpp:704-722` | Form tubes, instanced |
| Uplights and their fittings | `ReserveStructure.cpp:783-828` | Light luminaires at impost mounts |
| Pendant | `ReserveStructure.cpp:835-950` | Light luminaire bodies (lathe profiles) |
| Walls, west door, the box's top | `ReserveStructure.cpp:958-1016` | 3.1 and the top |
| Piers and responds placed | `ReserveStructure.cpp:1018-1040` | 3.4 |
| Vault assembled | `ReserveStructure.cpp:1072-1113` | 3.5 |
| **Do not port:** WeldWeights (a displacement workaround at creases) | `ReserveStructure.cpp:724-781` | Matter's relief with Form's crease rule |
| **Do not port:** the "void cap" over the vault | `ReserveStructure.cpp:1119-1128` | solid haunch fill (3.5) |
| **Do not port:** component setup, fallback materials, asset paths | `ReserveStructure.cpp:1142-1190, 1254-1291` | Studio casts; Matter resolves (no fallbacks) |
| Screens overview | `Reserve/ReserveRacks.h:12-47` | 3.8, 6.1 |
| Screen constants | `Reserve/ReserveRacks.cpp:33-54` | `ReservePlan.h` |
| Members and linen field | `ReserveRacks.cpp:57-98` | Form members (eased) and laid linen |
| Work slots and lamp spans | `ReserveRacks.cpp:100-169` | Art hang rule (8); Light mounts (4.2) |
| One screen | `ReserveRacks.cpp:171-256` | 3.8 |
| Lens | `ReserveRacks.cpp:258-266` | the picture light's diffuser (Light) |
| Vault height under a track | `ReserveRacks.cpp:268-290` | the rods meet the made vault (Form fact) |
| Tracks and hanger rods | `ReserveRacks.cpp:292-332` | 3.8 |
| Home and out | `ReserveRacks.cpp:334-340` | 6.1 states |
| Picture-light aim and size | `ReserveRacks.cpp:541-580` | Light Scheme at the trough mount (no lumens) |
| Initial state | `ReserveRacks.cpp:668-685` | initial configuration (6.1) |
| Glide, occupancy, tick, lights | `ReserveRacks.cpp:737-832` | Motion drive and occupancy guard; Light follows the channel |
| One out at a time | `ReserveRacks.cpp:836-843` | Motion control group |
| Use and prompts | `ReserveRacks.cpp:852-994` | IMuseeUsable on each screen |
| Send and return carry | `ReserveRacks.cpp:996-1060` | Art re-hang as a use (polish) |
| **Do not port:** `ArrangeImported`, USD prim retiring, perf console command | `ReserveRacks.cpp:583-666, 1062-1072`; `ReserveRacks.h:109-115` | nothing (no import; no lumen knobs) |
| Stair and shaft overview | `ReserveStair/ReserveStairStructure.h:13-51` | 2, 3.2, 3.10 |
| Stair constants | `ReserveStairStructure.cpp:28-65` | the Salon (3.10); the shaft's in 3.2 |
| Flight, landings, strings | `ReserveStairStructure.cpp:129-199` | the Salon's |
| Shaft walls, curb, soffit, outside | `ReserveStairStructure.cpp:203-234` | the Reserve's shaft limb (3.2); the curb is Site's collar |
| Handrails, brackets, downlights | `ReserveStairStructure.cpp:253-431` | the Salon's |
| Rail light placement | `ReserveStairStructure.cpp:435-465` | the Salon's (Light mounts) |
| **Do not port:** hidden walkway ramp | `ReserveStairStructure.cpp:238-248` | Visitor walks the real treads by the human measure |
| **Do not port:** invisible guards at the opening | `ReserveStairStructure.cpp:655-670` | a real guard is Site's or the Salon's decision (risks) |
| **Do not port:** per-play material instances, joint toggles, lumens, nits | `ReserveStairStructure.cpp:672-761` | Matter and Light |
| Pond opening, lift, speed | `windows/Source/MuseeVision/Salon/PondLift.cpp:20-23`; `Shared/Wings/Reserve.swift:11-19` | the port (2); the Salon's mechanism |
| Lamp CCT, pendant and uplight roles, easel spot position | `windows/Scripts/native.py:102-129, 280-350` | 4.2 (types and positions only) |
| **Do not port:** the Reserve white-balance volume | `native.py:117-121, 351-364` | the Eye (Visitor.OneEye) |
| Brick, globe, linen, lens materials | `windows/Scripts/reserve_materials.py:1-35, 42-48, 293-294` | Matter substances; the globe via Light's diffuser |
| Brick and concrete specs | `windows/Scripts/materials.py:967-983, 1112-1118, 1412-1422, 1546-1552` | Matter evidence (brick, concrete) |
| Frames in the Reserve | `windows/Scripts/frames.py:13, 47-48` | Art frames (8) |
| Boards | `plan/canvas/SalonReserve.dc.html`, `plan/canvas/AsBuiltReserve.dc.html`, `plan/README.md:90-94` | Site.MatchesCanvas; words |

---

## 12. First-pass cut (tonight)

**Built tonight**, all through the shared systems from the numbers above:
1. **Site.** The port `reserve.stair`, the receiving well (W1, W2, B1 to B4), the footprint volume, the enclosure
   `reserve.undercroft`, the stand's blind cap, and the mating line `salon.stair ↔ reserve.stair`.
2. **Form, the whole shell.** Walls with the arched west door and its travertine threshold; the shaft limb; the
   stepped top; the floor slab in sawcut panels with the 36 bronze-lined slots; the 14 piers and 18 responds; the
   24 groin vaults, 21 cross bands, 16 arcade bands and lunettes; the solid haunch fill.
3. **The 36 tracks and 36 screens**, the screens as instances of one drawing.
4. **Light.** 24 pendants (lathe bodies, the chain as instanced links), 56 uplights, the easel spot and 9 picture
   lights; the targets in 4.3; presence operation.
5. **Motion.** 36 screen mechanisms with the drive, home/out states, use, the one-out control group, the occupancy
   guard and the state channel; C starts out.
6. **Art.** The 13 catalogue records; 11 paintings on the nine face anchors; *Le Pont de l'Europe* on the easel; the
   pastels in glazed mats on the south chests; the gilt frames; the easel and four plan-chest islands as fittings.
7. **Sound.** Acoustics derived for the one enclosure; footfalls by substance.

**Waits for polish:**
- the send-to-easel carry and return;
- letter plates A to F on the nave end posts (Art signs, 120 mm, centre z 1.86);
- the cloth flex threaded through the chains and inside the rods; thumb screws, beads and knurls on the galleries;
- trolley wheels inside the tracks; the track wall plates' bolt detail;
- the screens' rolling sound and room tone;
- brick bond refinement (English bond detailing at the chamfers and arch rings);
- a study bench before the easel (owner decision).

If a system lacks a capability tonight, the piece that needs it is **absent, never stubbed**. The shortfall is a
Register gap against that system.

---

## 13. Open points and risks (for the Architect)

1. **Who builds what in the well.** This sheet gives the shaft's shell to the Reserve (receiving side) and the
   flight, landings, strings, handrails, rail lights, soffit downlights, pond, posts and a call control to the
   Salon (serving side). The Salon's sheet must say the same, or Site.NoOverlap and Site.RoomStaysInside fail at
   the seam.
2. **Footprints must be volumes.** The Reserve lies under the Salon, the oval, the cabinet, the portico and the lawn.
   A plan-only Site.NoOverlap fails. Site.OneGround must judge containment in plan and receive the Reserve's top
   (plan −0.33 under the lawn) as an underground top.
3. **Well shape.** The well is two boxes plus four cylinders, or one x–z section extruded across y plus four bores.
   If FSiteWell holds one box tonight, the post bores and the L-section cannot be declared exactly.
4. **Reachability on the stand.** Alone, the Reserve's floor reaches the well's foot but not the port, because the
   flight is the Salon's. Site.Reachable on the stand must treat the receiving well's floor as the port.
5. **Trap with the pond down.** Without a call control at the stair's foot, the pond-down configuration strands a
   visitor below. As built, it simply never lowered while the visitor was below.
6. **Fall at the opening.** With the pond up, the opening's north, south and east edges drop up to 3.4 m beside the
   flight. As built this was guarded by invisible boxes (not ported). A real guard belongs to Site's collar or the
   Salon, and the approved design shows none.
7. **Light shortfall likely.** A rough utilisation estimate gives the as-built globes and uplights about 20 to 40 lx
   on the floor, against the 100 lx circulation class. Measure first: Light.MeetsTarget will report it. The design
   intent is a dim store where the works glow. Meeting 100 lx is a Scheme choice or a design question for the owner,
   never a knob.
8. **Catalogue gaps.** There is no pendant globe and no uplight in `Data/Light/Luminaires`, and `picture-light.2700k`
   is evidence-incomplete, so it stands unlit. Without a lit picture-light record, the out screen's works fall short
   of 150 lx.
9. **Matter records needed:** brick, concrete, oak, walnut (oak-family accent), linen (canvas), felt, cased opal
   glass, and a sand-faced working for brick. Brass (`r-matter`) is still needed for the pulls and easel
   hardware. The racks are oak and linen screens, not brass wire mesh (conflict 12).
10. **Collection records.** The 13 works are missing from `assets/collection.json`. `data/artworks.json` gives
    Musée des Beaux-Arts de Reims for *Avenue de l'Opéra* against the Pushkin in `art_sources.json`.
11. **Structure.**
    - Only 0.14 m lies between the nave crown's intrados (5.61) and the Salon's floor zone (5.75), so the vault is
      a half-brick shell. A one-brick vault needs the Salon's floor raised or the Reserve lowered.
    - The Salon's side walls (plan |y| 7.0 … 7.6) and the portico's columns (plan y 12.0) stand over the aisle vaults
      and the south wall, not over piers. Load paths are modelled by no system.
12. **Board against code.** The code is the later as-built state and wins:
    - The board shows brass wire-mesh racks 5.0 × 3.6 m on carriages with two floor tracks; the code has top-hung
      oak and linen screens 3.40 m high with one guide slot.
    - The board says globes at 3800 K; the code has 2900 K.
    - `plan/README.md:92` says "columns every 6 m"; the code has 7.5 m bays.
13. **Screen speed.** This sheet sets 1.0 m/s, 0.6 m/s² and about 7.8 s against the as-built 4.5 s. The owner may
    prefer the old feel. It is a design decision, not a tuning.
14. **Annual dose.** The pastels meet 150 000 lx·h a year only if the aisles' light is presence-linked (4.3). Always-on
    circulation at even 20 lx gives about 175 000.
15. **White balance.** The as-built room relied on a 4300 K post-process volume, now retired. Unless the Eye adapts
    its white balance to the scene, a 3000 K room reads orange.
16. **Thin brick skin.** 70 mm of brick lies between each post bore and the shaft's face. A bronze sleeve would be
    truer, and the bore would then need to be larger.
