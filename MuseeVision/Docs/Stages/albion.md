# Albion · design sheet

- **Stage key** `albion` (module `MuseeAlbion`, `UAlbionRoom : USiteRoom`). Display name "Albion". The approved proposal on
  the Rotunda's south door (D-001): England 1848–1898, a museum court of iron and glass on ten columns of polished
  British stone, after the Oxford University Museum. Era: a Classical wing in English materials: stone honed, rubbed,
  carved or polished; iron painted in its own oxide; lead patinated; oak fumed and French-polished; gilt on oak.
- **Sources:** D-001…D-019 (D-005, D-006, D-012, D-014, D-017, D-018 binding here); the approved boards
  `plan/proposals/albion/canvas/{Main,Plan,Sections,Hang,Details,Outside}.dc.html` (no README); Palette.json + Story.md
  ("Albion"); the drafter's spec `r-specs/…/Stages/albion.md` (verified, corrections in §11); the finished `rotunda.md`
  (§2, `rotunda.south`) and `chenghuai.md` (the sibling port); the as-built code `windows/Source/MuseeVision/Albion/*`,
  `windows/Scripts/albion_*.py`, `assets/albion/hang.json`, `assets/albion/vessels.json`, `data/albion.json`,
  `assets/albion/paintings/SOURCES.json` (§11). Schedules: `Data/Stages/albion/{Finishes,Lighting,Hang}.json` (no
  Planting: nothing grows inside; no Mechanisms: nothing moves tonight).
- **Units** metres. **Frame:** origin at the centre of the court on the nave's axis (x 0) and on line 4, at the finished
  floor (top of the encaustic tiles, the Rotunda's floor level); x east, y south, z up, true north. **Old numbers
  convert** as x = x_plan, **y = y_plan − 35.200**, z = z_plan (the old plan's origin was the Rotunda's centre).
  **Sun bearing** b: compass bearing from north; the idealised sun's bearing is 90° + 15°·(h − 6).

## 1. Identity and placement

| | |
|---|---|
| Origin in the museum plan | (0.000, +35.200, 0.000); yaw 0°. **museum = local + (0, 35.200, 0).** Site derives it from the mating `albion.north ↔ rotunda.south` (§2); this is the value it must reproduce. The gilt sun is at local (0, −35.200, 0). |
| Level | 0. One storey on the Rotunda's floor. Below z 0 only footings: the plinth course and the buttresses' base courses run to −1.20 (the Grounds' lawn falls to the south). |
| Footprint kind | Building (court and porch). |
| Vertical extent | z −1.20 to 17.60 (the nave ventilator's lead cap). |

**Setting-out:** column lines 1–7 at y −18, −12, −6, 0, 6, 12, 18; bays 1–6 centred y −15, −9, −3, 3, 9, 15; arcade lines
x ±6.0; aisle crowns x ±10.0; side walls' inner faces x ±14.0, outer ±14.6; end walls' inner faces y ±18.0, outer ±18.6
(walls 0.60). Porch: inner faces x ±3.00, outer ±3.60, from the port plane y −24.000 to the court's north outer face
y −18.600; its end lining's porch face y −23.900.

**Footprint polygon** (envelope, clockwise on the north-up plan): (−3.70, −24.00) → (3.70, −24.00) → (3.70, −19.60) →
(15.60, −19.60) → (15.60, 19.60) → (−15.60, 19.60) → (−15.60, −19.60) → (−3.70, −19.60). It covers the buttresses and
their base courses (x ±15.56; y ±19.56 at x ±6 on the end walls), rainwater heads (≤ 0.31 off the faces), copings, the
plinth (0.08 proud) and the porch to the port plane. The Grounds pave or plant to the ground-contact outline (plinth
line x ±14.68, y ±18.68, porch ±3.68, plus the 18 buttress lobes), which Form derives from the made pieces. Between the
port plane and the drum (|x| ≤ 3.65) the wedge is the joint's (§2).

| Enclosure | Kind | Bounded by |
|---|---|---|
| `albion.court` | Enclosed | x ±14.0, y ±18.0, floor 0 up to the glass of three pointed vaults springing at 10.00 (nave crown 17.00, aisle crowns 15.20). Section 405.1 m² × 36.0 = **14,583 m³**. |
| `albion.porch` | Enclosed | x ±3.00, y −23.90 … −18.60, floor 0 to the oak ceiling at 6.90: **219 m³**. |

**Openings (Form's one list).** Glazed: `albion.vault.nave`, `albion.vault.west`, `albion.vault.east`,
`albion.screen.north`, `albion.screen.south` (the nave's screens above 8.52), `albion.gable.nw`, `albion.gable.ne`,
`albion.gable.sw`, `albion.gable.se` (the aisles' gables above 10.00), `albion.band` (the twelve Tristram panels in the
south screen), `albion.lancet.w1`…`w6`, `albion.lancet.e1`…`e6`. Port: `albion.north`. **The court door** (§3.1) is one
opening shared by `albion.court` and `albion.porch`, declared **Open, shared by two enclosures** (as Chenghuai's; if
Form cannot tonight, the porch merges into the court).

## 2. Ports

Frame on the port face at the threshold (z 0), on the opening's centreline, facing out of the stage, up +z. Class
names are **width × crown**. Mated frames coincide and face opposite ways (`salon.east`/`rotunda.west`,
`chenghuai.south`/`rotunda.north`).

| Key | Kind | Class | Local frame (position; facing) | Face; wall depth | Mates with | Wells |
|---|---|---|---|---|---|---|
| `albion.north` | doorway | `doorway.4000x6500.arched` | (0.000, −24.000, 0.000); **−y** (north, to the Rotunda) | **flat**, the plane y −24.000, tangent to the drum's outer cylinder R 11.20 at x 0; **0.10** (the porch's end lining, y −24.00 → −23.90, as built) | `rotunda.south` (frame museum (0, +11.200, 0), facing +y; cylindrical R 11.20; drum 1.20). **Passage 0.00**; the joint fills the wedges. | none |

- **Class** `doorway.4000x6500.arched`: clear 4.000 (jambs x ±2.000); semicircular head r 2.000 springing 4.500; crown
  6.500 (`MuseePlan.h:44-47`). Open portal (no leaves).
- **Museum check.** (0, −24.000, 0) + (0, +35.200, 0) = **(0.000, +11.200, 0.000)** = `rotunda.south`'s frame; facings −y
  and +y are opposite; the plane touches the drum's cylinder (museum r 11.20) on the axis.
- **The wedges.** Between the port plane and the drum, 11.20 − √(11.20² − x²): 0 on the axis, 0.180 at the clear jambs
  (x ±2.00), 0.409 at x ±3.00, 0.594 at the porch's outer faces (±3.60), 0.611 at the copings' edge (±3.65). **Site's
  joint** builds: the travertine threshold from the drum's inner face (r 10.00) through the drum, the wedge and Albion's
  0.10 lining to y −23.90; the brushed CuSn10 strip on the port plane across the clear width; the linings of both rough
  openings; the surrounds; and the wedge masonry over **|x| ≤ 3.65, z −1.20 … 7.50** (the porch's outer faces, lead flat,
  parapets and copings run into it: `Facade::VestibuleHalf` 3.65, `VestibuleTop` 7.5, `MuseePlan.h:528`). Albion builds
  nothing past its port plane.
- **Albion builds** the rough opening only, through its 0.10 end lining: clear + the class allowance 0.050 (width 4.10,
  head r 2.05 about the springing 4.50, crown 6.55, rough sill −0.05), faces `albion.north.rough-opening`.
- **Surround clearance (as Chenghuai's).** The joint's surround on the porch face of the lining (y −23.90) may be up to
  **0.35** beyond the clear opening: Albion keeps that face clear for **|x| < 2.35 and z < 6.85** (the skirting stops at
  x ±2.35; the side walls stand 0.65 beyond it at ±3.00; the oak ceiling is at 6.90 and the first beam's soffit, 6.61, is
  1.30 into the porch). Albion touches nothing of the drum: the wedge and the drum's outer surround (if any, ≤ 0.35 past
  the jambs) are the joint's. The Rotunda's own archivolt (0.30) is on its inner face.
- **Blinds.** Mated tonight: open portal, no leaves. On the stand alone (or with the Rotunda absent) a closed pair of cast
  bronze leaves fills the class opening on the port plane, meeting rail on the axis: `albion.north.blind-doors`.
- **Topology (D-012):** the only port; a dead end by design ("no other doors: the court is a dead end, like the Nymphéas
  oval"). No well served or received. The court's exterior Glazed openings keep the Grounds resident while the visitor is
  in Albion.
- **The axis.** From the gilt sun's eye (local (0, −35.20, 1.60)) the view runs 53.15 m through both doors to the
  Attainment's centre (0, 17.95, 2.22). Albion registers **Albion.AxisClear**: a 0.60 m sightline between those points
  meets no piece (benches at |x| ≥ 2.95; electroliers on the arcade lines; the porch electrolier's lowest point 5.00
  against the line's 1.76 there).

### Anchors (the stage contract)
Wall anchors: origin on the hanging face (z 0 = the floor) at the left end as seen by the viewer, x to the viewer's
right, y out of the face towards the viewer, z up. Glazing anchors (kind **Glazing**, D-014; Art's image in Form's
opening; the contract's kind table lacks it, §13): origin at the panel's lower-left seen from inside, x to the right, y
into the court.

| Key | Kind | Frame (origin; axes) | Belongs to |
|---|---|---|---|
| `albion.wall.west` | Wall | (−14.000, 18.000, 0); x (0,−1,0); y (1,0,0) | the west wall's 12 works (truth to nature, walked south) |
| `albion.wall.east` | Wall | (14.000, −18.000, 0); x (0,1,0); y (−1,0,0) | the east wall's 17 works (beauty, walked north) |
| `albion.wall.south` | Wall | (14.000, 18.000, 0); x (−1,0,0); y (0,−1,0) | the legend: Shalott, the Attainment, Cophetua |
| `albion.wall.north` | Wall | (−14.000, −18.000, 0); x (1,0,0); y (0,1,0) | the Star of Bethlehem |
| `albion.case.w3`, `albion.case.e3`, `albion.case.w5`, `albion.case.e5` | Mount | the baize bed's top centre (∓10.400, −6.000, 0.550), (∓10.400, 6.000, 0.550) (w: x −10.4; e: +10.4); stage axes | the De Morgan lustreware (§8) |
| `albion.lectern` | Mount | the desk's centre (12.200, −13.300, 1.107); x up the slope (1,0,0) tilted 18° up; z the desk's normal; the reader faces +x | the Kelmscott Chaucer |
| `albion.band.t1`, `albion.band.t2`, `albion.band.t3`, `albion.band.t4`, `albion.band.t5`, `albion.band.t6` | Glazing | origin x 4.80, 3.20, 1.60, 0.00, −1.60, −3.20; y 18.300; z 10.900; x (−1,0,0); y (0,−1,0) | the upper row (T1 spans x 4.8 → 3.2) |
| `albion.band.t7`, `albion.band.t8`, `albion.band.t9`, `albion.band.t10`, `albion.band.t11`, `albion.band.t12` | Glazing | the same x; y 18.300; z 9.000; axes as above | the lower row |
| `albion.lancet.w1` … `albion.lancet.w6` | Glazing | (−14.300, y_b + 0.70, 5.000), y_b = −15, −9, −3, 3, 9, 15; x (0,−1,0); y (1,0,0) | reserved: Webb's birds and Morris's quarries (no catalogue ids, §13) |
| `albion.lancet.e1` … `albion.lancet.e6` | Glazing | (14.300, y_b − 0.70, 5.000); x (0,1,0); y (−1,0,0) | reserved: Chaucer's Good Women, north to south Alceste, Cleopatra, Thisbe, Dido, Ariadne, Lucrece |
| `albion.bench.west.seat.1`, `.2` / `.3`, `.4` | Seat | seat top z 0.450 at (−3.325, 4.200), (−3.325, 5.400) facing −x / (−3.075, 4.200), (−3.075, 5.400) facing +x | the west bench |
| `albion.bench.east.seat.1`, `.2` / `.3`, `.4` | Seat | (3.325, 4.200), (3.325, 5.400) facing +x / (3.075, 4.200), (3.075, 5.400) facing −x | the east bench |

No Body, Use, Bed or Water anchors; no channels. Luminaires stand at the points in Lighting.json.

## 3. Pieces by trade

Profiles are point lists (A out from the face or r for a lathe, z up); `*` marks a smooth point. Every piece meets its
neighbour face to face. Surfaces are listed in §3.10.

### 3.1 Masonry inside
- **Coursing** (every banded face, inside and out; `BandCuts`): slate skirting 0.00–0.20; one buff course 0.20–0.90;
  then every 1.20 one red course of 0.30 and three buff of 0.30. Red courses 0.90–1.20, 2.10–2.40, 3.30–3.60,
  4.50–4.80, 5.70–6.00, 6.90–7.20, 8.10–8.40, 9.30–9.60 (outside also 10.50–10.80). Buff blocks 0.75 long, red 0.95,
  half bond ± 0.2 jitter, 4 mm joints. Buff `albion.wall.buff`, red `albion.wall.red`.
- **Side walls** (x ±14.00), y −18 … 18, z 0 → 9.73.
  - **Lancets**, one on each bay's centre line: light 1.40 wide (half 0.70), sill 5.00, head 9.00, two-centred head
    radius 0.808, centres 0.108 across the axis, springing 8.199. Chamfer 0.06 on the inner face, reveal to the glass
    line 0.30 into the wall (x ±14.30); inner sill level at 5.00 from the chamfer's foot to the glass.
  - **Pilasters** on lines 2–6: 0.30 wide × 0.10 proud, z 0.19 → 9.30; a three-step corbel above each (width / proud):
    9.30–9.45 0.34 / 0.15; 9.45–9.60 0.38 / 0.20; 9.60–9.74 0.42 / 0.26 (a main rib's shoe). The skirting returns round.
  - **Aisle cornice** (swept round each aisle from the south wall at x ∓6 along the side wall to the north wall at x ∓6):
    (−0.01, 9.72) (0.03, 9.72) (0.075, 9.77) (0.075, 9.84) (0.12, 9.88) (0.12, 10.00) (−0.01, 10.00).
  - **Corner corbels** in each aisle corner: 0.22 × 0.22, z 9.35 → 9.74.
  - **Skirting** `albion.skirting`: 0.20 × 0.015 proud, 4 mm bevel on the top arris, 1.2 m lengths, round every pilaster.
- **End walls** (y ±18.00), 0.60 thick. Aisle ends (|x| 6 … 14) rise to 10.00, their tops (a dressing) carry the gables'
  glass on the wall's middle. Nave ends (|x| ≤ 6) are stone to **8.50**; returns at x ±6 from 8.50 to 10.00; the
  **nave cill**: front 8.38 → 8.52, 0.02 proud; top weathering 8.52 (inner) → 8.47 at 0.05 beyond the outer face.
- **The court door** in the north wall (y −18.60 … −18.00): 4.00 wide, pointed, springing 3.30, crown 5.50, R 2.21, the
  centres 0.21 across the axis; chamfer 0.10 on the court face, 0.06 on the porch face, reveal between. **Hood mould** on
  the court face: the door outline grown 0.16 with level returns 0.28 at the springing; section (across, out)
  (−0.045, −0.01) (0.045, −0.01) (0.045, 0.035) (0.02, 0.07) (−0.03, 0.07) (−0.045, 0.05); **label stops** 0.14 wide,
  0.10 proud, z 3.14–3.36. North-wall skirting stops at x ±2.101.
- **Tablets** `albion.tablet` (0.03 proud, 0.02 chamfered edge in `albion.dressing`): **Ruskin's** over the court door,
  x ±4.60, z 6.50–7.50: REJECTING NOTHING · SELECTING NOTHING · SCORNING NOTHING (*Modern Painters* I, 1843);
  **ALBION / 1848 · 1898** on the north wall in the west aisle, x −12.60 … −7.40, z 1.85–3.45. Letters V-cut (polish;
  Centaur capitals, as the Rotunda's).

### 3.2 Masonry outside and the porch
- **Outer faces** banded (`albion.ext.buff`, `albion.ext.red`) from the plinth: side walls x ±14.60 to the parapets'
  top 10.90; aisle ends y ±18.60 to 10.90; nave ends to 8.50 (on the north the porch covers x ±3.60 up to its roof,
  the banded face continues above it from 7.35 to 8.50).
- **Plinth course** round court and porch (from the port plane): (−0.02, −1.20) (0.08, −1.20) (0.08, 0.52) (0.00, 0.60)
  (−0.02, 0.60).
- **Buttresses** 18, each 1.00 wide × 0.90 deep: 14 on the side walls at lines 1–7 (top 9.90); 4 on the end walls at
  x ±6 (top 10.40). Full depth 0.60 → set-off 4.80; weathering to 5.15 while the face steps back to 0.558; upper stage to
  top − 0.60, then a steep weathering into the wall. Base course 0.06 proud all round, −1.20 → 0.52, weathered to 0.60.
- **Lancets outside:** chamfer 0.06; reveal from the face to the glass line (0.30); outer sill weathered 5.04 (glass) →
  5.00 (face); drip block 1.68 wide, 0.07 proud, z 4.88–5.00.
- **Parapets and gutters:** side parapets 0.30 thick (inner face x ±14.30), 10.00 → 10.90, lead gutter x ±14.00 …
  ±14.30 at 10.012 turned up 0.15; end parapets over the aisles, inner faces y ∓18.30 between |x| 6.0 and 14.3, their ends
  facing the nave over the cill. **Copings** (two U runs, from x ∓6 round each side): (−0.35, 10.88) (0.05, 10.88)
  (0.05, 10.96) (−0.15, 11.05) (−0.35, 10.96).
- **The porch.** Side walls inner x ±3.00, outer ±3.60, from the port plane to y −18.60; inside banded to the ceiling
  (6.90) with slate skirting; outside banded to 7.35. **End lining** (north): 0.10, y −24.00 → −23.90, x ±3.60, z −1.20 →
  7.20, banded on its porch face (skirting from x ±2.35 to ±3.00), carrying the port's rough opening (§2). South end: the
  court wall's outer face with the court door (0.06 chamfer), skirting x ±2.061 … ±3.00. **Parapets** inner faces x ±3.35,
  from the lead flat (7.20) to 7.35; **copings** x ±3.30 … ±3.65, 7.35 → 7.47 (`albion.ext.dressing`).

### 3.3 Floors (tops at z 0)

| Floor | Extent | Laying | Surface |
|---|---|---|---|
| Nave field | x ±5.70, y ±17.70 | 152.4 mm encaustic tiles on the 45° diagonal, 2 mm dark joints; 2 × 2 roundels framed by black tiles with buff squares at the crossings (after Minton and Pugin); repeat 2.586 | `albion.floor.nave` |
| Nave border | 0.30 round the field, mitred at x ±6.0, y ±18.0 | 1½-inch black fillet, 6-inch running trefoil (buff on red), buff fillet, 3-inch black-and-red chequer | `albion.floor.nave.border` |
| Aisles | x −14.0 … −6.0 and 6.0 … 14.0, y ±18.0 (1 cm under the walls) | York stone flags in 0.60 courses along the aisle, lengths 0.6–1.2 (mean 0.90), broken bond, 5 mm lime joints | `albion.floor.aisle` |
| Porch field | x ±2.70, y −23.60 … −18.90 | as the nave | `albion.porch.floor` |
| Porch border | 0.30 round the field | black | `albion.porch.floor.border` |
| Court door reveal | x ±2.00, y −18.60 … −18.00 | York stone; strip 0.06 × 4.03 on y −18.30, 0.5 mm proud | `albion.door.reveal`, `albion.door.strip` |

The port's threshold (drum face to y −23.90 within the opening) is the joint's travertine.

### 3.4 Columns (the court column; lines 2–6 at x ±6; profiles private to Albion)

| | Line 2 (y −12) | Line 3 (−6) | Line 4 (0) | Line 5 (6) | Line 6 (12) |
|---|---|---|---|---|---|
| West, x −6 | A Lizard serpentine | B Peterhead granite | C Rubislaw granite | D Purbeck marble | E Frosterley marble |
| East, x +6 | F Hopton Wood stone | G Ashburton marble | H Iona marble | I Tiree marble | J Mona marble |

- **Half-columns**: four, on lines 1 and 7 at x ±6, half-round against the end walls (y ±18.00), shafts in the dressing.
- **Base** 0 → 0.355 (`albion.dressing`): square plinth 0.72 with 0.07 chamfered corners, 0 → 0.12 (court half only on a
  half-column); lathe (r, z) (0, 0.12) (0.345, 0.12) (0.345, 0.135) (0.345, 0.16)* (0.33, 0.185)* (0.30, 0.195)
  (0.285, 0.20) (0.275, 0.235)* (0.285, 0.26) (0.29, 0.27)* (0.30, 0.29)* (0.29, 0.31)* (0.27, 0.318) (0.262, 0.33)
  (0.262, 0.355) (0, 0.355).
- **Shaft** Ø 0.50, polished, 0.34 → 5.505 (5.165), one monolith of its stone: A `albion.shaft.a`, B `albion.shaft.b`,
  C `albion.shaft.c`, D `albion.shaft.d`, E `albion.shaft.e`, F `albion.shaft.f`, G `albion.shaft.g`, H `albion.shaft.h`,
  I `albion.shaft.i`, J `albion.shaft.j`.
- **Necking** (r, z): (0, 5.49) (0.258, 5.49) (0.262, 5.50)* (0.278, 5.525)* (0.262, 5.55)* (0.255, 5.56). **Bell** 5.56 →
  6.08, r = 0.255 + 0.145·t^1.8, t = (z − 5.56)/0.52, 12 stations. **Abacus** 0.90 square (half 0.45), 6.075 → 6.20: a
  quarter-circle cavetto from half-size 0.36 at 6.075 to 0.415 at 6.115; a half-round r 0.018 about (0.415, 6.133); a
  fillet to (0.438, 6.151) (0.45, 6.153); the face to 6.188; a 0.012 chamfer to the top 6.20. Capital block
  `albion.capital`, one Bath stone block 0.90 × 0.90 × 0.70 (5.50 → 6.20).
- **Carving** (Form Carving, polish; `AlbionColumns.cpp:16-547`): A fallen oak with acorns (*Mariana*), B poppies,
  pansies and violets (*Ophelia*), C cabbages in nets (*The Last of England*), D ivy (*The Long Engagement*), E bramble and
  hawthorn; F daisy (Morris 1864), G trellis roses (1864), H pomegranates (*Fruit*, 1866), I acanthus (1875), J willow
  (*Willow Bough*, 1887). Half-capitals: NW oak, NE daisy, SW bramble, SE willow. Each: four corner leaves 0.44 long
  rolling into volutes ≈ 0.09 under the abacus corners, four leaves (scale 0.72) under each face's middle, an upper tier
  of eight from 5.76; leaves ≥ 0.028 thick (volutes 0.04).

### 3.5 Iron and lead
Cast iron `albion.iron.cast`, wrought `albion.iron.wrought`, outside `albion.iron.ext`, lead `albion.lead`. The iron
inside is out of reach: no collision; it casts shadows.
- **Clusters** (14: every column; the half-columns' at y ±17.88): shoe 0.44 × 0.44, 6.198 → 6.23, four bolts at ±0.16;
  base lathe (0, 6.23) (0.20, 6.23) (0.20, 6.25)* (0.17, 6.29)* (0.15, 6.30) (0.15, 6.34) (0, 6.34); core Ø 0.15 and four
  colonnettes Ø 0.09 on the diagonals at r 0.095, 6.33 → 9.42; annulets at 7.45 and 8.65: (0, z−0.03) (0.15, z−0.03)
  (0.165, z−0.01)* (0.165, z+0.01)* (0.15, z+0.03) (0, z+0.03); capital (0, 9.41) (0.15, 9.41) (0.165, 9.44)* (0.15, 9.47)
  (0.155, 9.50)* (0.19, 9.59)* (0.25, 9.66) (0, 9.66), eight cast leaves (polish); top plate 0.60 × 0.60, 9.655 → 9.702.
- **Box girders** on x ±6, y −18.02 … 18.02, z 9.70 → 10.00: bottom flange 0.70 × 0.018, top 0.74 × 0.018, two webs 0.012
  at ±0.29 with angles of 0.06 outstand; (polish: rivets Ø 0.022 at 0.15, a bead 0.018 × 0.024 at mid-height, splice
  plates 0.50 × 0.014 with 2 × 5 bolts at each line). Lead valley gutter 0.66 wide at 10.004.
- **Arcade arches** (12 bays, planes x ±6): two-centred from the capital tops (6.20) at ±2.75 from the bay's centre to
  the extrados crown 9.70 under the girder, R 3.602 (centres 0.852 beyond the axis). Rib 0.16 deep: web 0.024, outer
  flange 0.11 × 0.018, inner flange 0.09 × 0.018, a roll Ø 0.044 on the intrados; soffit 9.54 at the crown; a boss r 0.05.
  **Spandrels** (wrought): a ring R 0.46 (tube Ø 0.036) centred 2.34 from the bay's centre at z 9.20; an inner ring R 0.41
  (Ø 0.016); a quatrefoil of four rings R 0.17 (Ø 0.018) on the diagonals at 0.19; lugs 0.04 × 0.044 to the arch and the
  girder.
- **The glass arcs** (transverse section; the glass lies on them, y −18.30 … 18.30):

| Vault | Half | Springs | Arc centre | R | Crown | Half-arc |
|---|---|---|---|---|---|---|
| Nave | west / east | (∓6, 10) | (±1.0833, 10) | 7.0833 | (0, 17.00) | 10.04 |
| West aisle | west / east | (−14, 10) / (−6, 10) | (−8.62, 10) / (−11.38, 10) | 5.38 | (−10, 15.20) | 7.06 |
| East aisle | west / east | (6, 10) / (14, 10) | (11.38, 10) / (8.62, 10) | 5.38 | (10, 15.20) | 7.06 |

- **Main ribs** on lines 1–7 (42 halves), a built-up I whose top is 0.15 under the glass: outer flange 0.016 × 0.12
  (0.16 on lines 1, 7), web 0.014 × 0.26, inner flange 0.016 × 0.102 (0.136). **Inner arch** (wrought) a bar 0.03 × 0.018
  0.62 under the glass at the springing, blending as (s/L)^6 into the rib by 94 % of the arc; **ring chain** of rings
  R 0.097 (Ø 0.018) at 0.24 pitch between rib and inner arch to 82 % of the arc; **shoe** 0.40 × 0.20 × 0.026, two bolts
  at ±0.14.
- **Light ribs** every 1.50 between the main lines (18 lines × 6 halves), a tee: table 0.09 × 0.012 0.15 under the glass,
  stem 0.012 × 0.14, bead 0.022 × 0.018.
- **Side bearers** along each side wall at the springing, 0.38 into the court × 0.06 (9.94 → 10.00); a cast bracket under
  each **light rib's foot** (y = −16.5 + 1.5k except the main lines): plate 0.016 × 0.29, strut from the wall at 9.65 to
  0.33 into the court at 9.93.
- **Purlins** full length (y ±18.28): 5 per nave half, 3 per aisle half (≈ 1.8 m of arc), flange 0.052 × 0.012 at
  0.05–0.062 under the glass, web 0.01 from 0.05 to 0.15; a ridge bar 0.10 wide, 0.004 → 0.15 under, at each crown.
- **Glazing bars**: 48 per vault half at 0.747 (36.6/49), from 0.10 above the springing to the crown: web 0.008 × 0.047
  (0.003–0.05 under the glass), foot 0.032 × 0.01.
- **End screens** (planes y ∓18.30). Nave mullions at x −6, −4.8, −3.2, −1.6, 0, 1.6, 3.2, 4.8, 6 (edge ones 0.12 × 0.20,
  others 0.07 × 0.12), 8.50 → the arc − 0.02; glazing bars 0.024 × 0.05 at x = −5.4 + 0.8k (15; on the south they stop at
  the band: 8.50–9.00 and 12.60 → arc); transoms at 8.50, 9.00, 10.70, 10.90, 12.60, 14.80 (0.07 × 0.10; the band's four
  on the south 0.10 × 0.14), each stopping where the arc comes down; an upper transom at 16.00 (0.03 × 0.06); nave cill on
  the stone 0.16 deep, 8.52 → 8.59. **Aisle gables**: mullions at |x| 6.8, 8.4, 10.0, 11.6, 13.2 (0.06 × 0.10), 10.00 →
  arc; transoms at 11.60 and 13.40 (0.06 × 0.08); a cast cill 0.06 high at 10.00.
- **Saddle bars** in the lancets (wrought): 0.016 square, 0.015 inside the glass line, the light's width + 0.03 each side,
  at z 5.45, 5.90, 6.35, 6.80, 7.25, 7.70, 8.15.
- **Lead ridges**: rolls Ø 0.09 on the three crowns (crown + 0.05), y −18.42 … 18.42; aisle crestings (`albion.iron.ext`)
  a plate 0.03 thick 15.28 → 15.54 (rings R 0.08 every 0.60: polish); **nave ventilator** 0.44 wide: standards 0.30 ×
  0.04 × 0.40 (17.08 → 17.48) every 1.5, four louvre blades a side at 0.09 pitch from 17.12 (x ±0.12 → ±0.17, rising
  0.06), lead cap from x ±0.22 at 17.48 to the ridge at 17.60, closed underneath.
- **Rainwater** (`albion.iron.ext`), 10 downpipes Ø 0.104, 0.14 off the face: side walls at y −12.8, −0.8, 11.2 (heads to
  10.55); end walls at x ±6.80 on y ∓18.60 (heads to 10.35). Head 0.38 × 0.29 × 0.36 with a lip 0.44 × 0.32 × 0.07, taper
  over 0.14, shoe Ø 0.128 from 0.02 to 0.16, holderbats every 1.80 from 1.20.

### 3.6 Glazing (Glazed openings; the glass lies on the arcs and screen planes)

| Opening | Where | Glass surface | Pieces |
|---|---|---|---|
| `albion.vault.nave` | two halves on the nave arcs, 10.10 → crown | `albion.vault.glass.nave` (clear) | flat panes 0.747 × 0.60, each its own plane lapping 10 mm over the one below (17 per half) |
| `albion.vault.west`, `albion.vault.east` | four halves on the aisle arcs | `albion.vault.glass.aisle` (**diffusing**, §4) | 12 per half |
| `albion.screen.north` | y −18.30, x ±6, 8.52 → arc | `albion.screen.glass` | between mullions, transoms and bars |
| `albion.screen.south` | y +18.30, x ±6, round the band: 8.52–9.00 (x ±6); 9.00–12.60 at \|x\| 4.8–6.0; 10.70–10.90 (x ±4.8); 12.60 → arc | `albion.screen.glass` | as the north |
| `albion.gable.nw`, `.ne`, `.sw`, `.se` | y ∓18.30, \|x\| 6 … 14, 10.00 → arc | `albion.screen.glass.gable` (**diffusing**) | between mullions and transoms |
| `albion.band` | y +18.30, x +4.8 … −4.8 in six columns of 1.60; upper row 10.90–12.60 (T1–T6), lower 9.00–10.70 (T7–T12); T1 at x 4.8 → 3.2 (read left to right from inside) | `albion.band.glass`, leads `albion.came` | 12 panels 1.60 × 1.70, each a 0.07 border of cream quarries round the image (§8); protective glazing 0.05 outside (`albion.guard-glazing`) |
| `albion.lancet.w1`…`w6`, `albion.lancet.e1`…`e6` | x ∓14.30 (the glass line), y −15, −9, −3, 3, 9, 15, the outline grown 0.01 (1.42 × 4.01) | `albion.lancet.glass`, `albion.came` | protective glazing 0.05 outside (`albion.guard-glazing`) |

No blinds (D-014). Weather §5.

### 3.7 The porch's ceiling and roof
- **Ceiling** `albion.porch.ceiling`: oak boards at 6.90 over x ±3.00, y −23.90 … −18.60; three moulded beams across at
  y **−22.60, −21.25, −19.90**, 0.20 × 0.30 (6.61 → 6.91), 12 mm eased arrises (bays 1.30 / 1.35 / 1.35 / 1.30); wall
  plates along the side walls at x ±2.93, 0.14 × 0.18 (6.73 → 6.91).
- **Lead flat** `albion.lead` at 7.20 between x ±3.35, from the port plane (y −24.00) to −18.59; the lining and parapet
  ends close the space between ceiling and flat.

### 3.8 Fittings
- **Picture rail** `albion.picture-rail` (Art's hanging system, drawn by Form): bronze rod Ø 0.022 at z 4.20, 0.035 off
  the side walls, bay by bay between the pilasters (0.03 clear of each pilaster's face; 0.05 from the end walls in bays 1
  and 6), ball ends r 0.016; brackets every ≈ 1.4 (≥ 2 a bay): wall plate 0.06 × 0.009, z 4.14 → 4.20, arm 0.024 wide
  reaching 0.035, z 4.165 → 4.188. Works hang by two rods Ø 0.006.
- **Benches** `albion.bench`, two, centred (±3.2, 4.8), long along y: top 3.00 × 0.50, seat 0.45, 0.065 thick, two boards
  with a 12 mm rounded arris; ends boards on edge 0.38 × 0.07, 0.30 in from each end, 0.10 → 0.385; feet 0.14 × 0.08 at
  x ±0.15, 0 → 0.10; cleats under the top; stretcher 0.06 × 0.11 at 0.20 from y ±1.28, wedged through the ends.
- **Table cases** `albion.case` (glass `albion.case.glass`, bed `albion.case.baize`), four, centred (±10.4, −6.0) and
  (±10.4, 6.0), 1.00 (x) × 2.20 (y), top 0.95: legs turned 0 → 0.36 (the turning fitted inside it), square block
  0.36 → 0.54; stretchers 0.036 × 0.05 at 0.12; apron 0.14 deep (0.38 → 0.52); deck 0.50 → 0.535 with a moulded edge 0.015
  proud; baize bed 0.535 → 0.550; glazed box 0.535 → 0.95, oak corner posts 0.028 square, top rails 0.028 × 0.024.
  Collision = its own solids (no guard box).
- **Lectern** `albion.lectern`, centred **(12.2, −13.30)** in Morris's bay, the reader facing east: cross feet 0.60 ×
  0.09 × 0.08 on four pads 0.10 square; post with stopped chamfers 0.11 → 0.085 square, 0.07 → 0.90; collar 0.15 square
  0.88 → 0.92; two brackets each side; desk 0.74 × 0.60 × 0.03 sloping 18°, far edge 1.20, near edge 1.015; ledge 0.035
  up the desk.
- **The tapestry's batten** `albion.tapestry.batten`: oak 7.00 × 0.10 × 0.03 in the lining's sleeve at the top edge
  (z 3.44), on two painted-iron brackets (`albion.iron.cast`) at x ±2.50 on the south wall; lining `albion.tapestry.lining`.
- **Electroliers** (Light's fitting, catalogue gap §13): body `albion.electrolier.body`, globes
  `albion.electrolier.globe`. Twelve at (±6.0, bay centres), hung from a clamp on the arcade arch's soffit at the crown
  (9.54) by a chain of 7 mm links to a turned stem ≈ 1.3 long; body rim B = 5.85; six scrolled arms reaching 0.505 and
  rising 0.12 to galleries holding cased opal globes R 0.085 (neck r 0.038), centres z B − 0.001; corona rings r 0.27
  (tube 0.0075) and 0.30 (0.004) at B − 0.012; bowl (r, z − B): (0, −0.30) (0.010, −0.292)* (0.020, −0.27)* (0.024,
  −0.245)* (0.020, −0.215)* (0.013, −0.20) (0.011, −0.18) (0.018, −0.165)* (0.011, −0.15) (0.012, −0.13) (0.030, −0.11)*
  (0.052, −0.075)* (0.068, −0.04)* (0.076, −0.012)* (0.082, −0.004)* (0.082, 0.006)* (0.074, 0.014) (0.066, 0.02) (0.052,
  0.045)* (0.030, 0.066)* (0.014, 0.074) (0, 0.074). One four-armed fitting in the porch at **(0, −21.25)**, its stem
  from a rose on the middle beam's soffit (6.61), B = 5.30. Nothing over the nave's axis but the porch fitting (§2).
- No stanchions, ropes, labels or guards. Blind doors: §2.

### 3.9 Stock
Each shaft one monolith 5.165 × Ø 0.50 (Form.WithinStock: if a stone's largest block is shorter, drums at its bed
height, never a broken rule); each capital one block 0.90 × 0.90 × 0.70; vault panes 0.747 × 0.60; band panels 1.60 ×
1.70 in leaded pieces; tiles 152.4 mm; flags ≤ 1.2; identical panes, bars, ribs and tiles instanced.

### 3.10 Surfaces (Finishes.json)

| Surface | Where | Palette key · working · history |
|---|---|---|
| `albion.wall.buff`, `albion.wall.red` | inner ashlar and bands (court, porch, the end lining) | `accent.bath-stone` / `accent.mansfield-red` · honed · aged.patina (soot) |
| `albion.dressing`, `albion.tablet`, `albion.capital` | inner dressings, tablets, capitals | `accent.bath-stone` · rubbed / rubbed / carved · aged.patina |
| `albion.skirting` | slate skirting | `accent.welsh-slate` · honed.bevelled · foot-worn |
| `albion.shaft.a` … `albion.shaft.j` | column shafts A–J | `accent.british-column-stones` · polished.hand-touched · handled |
| `albion.ext.buff`, `albion.ext.red`, `albion.ext.dressing` | outside | bath stone / Mansfield red · honed, honed, rubbed · weathered |
| `albion.north.rough-opening` | the port's rough opening (hidden) | `accent.bath-stone` · sawn · new |
| `albion.floor.nave`, `albion.floor.nave.border`, `albion.porch.floor`, `albion.porch.floor.border` | encaustic floors | `accent.encaustic-tile` · waxed · foot-worn |
| `albion.floor.aisle`, `albion.door.reveal` | York flags | `accent.york-stone` · sawn · foot-worn |
| `albion.door.strip` | the court door's strip | `bronze.cusn10` · brushed · foot-worn |
| `albion.iron.cast`, `albion.iron.wrought`, `albion.iron.ext` | iron | `accent.victorian-iron` · painted (iron-oxide brown-red) · aged.patina / weathered |
| `albion.lead`, `albion.came` | lead | `accent.milled-lead` · patinated · weathered / aged.patina |
| `albion.vault.glass.nave`, `albion.screen.glass`, `albion.guard-glazing` | clear glass | `glass.clear-float` · laminated.uv-filtering · weathered |
| `albion.vault.glass.aisle`, `albion.screen.glass.gable` | the aisles' glass | `glass.clear-float` · laminated.uv-filtering.diffusing · weathered |
| `albion.band.glass`, `albion.lancet.glass`, `albion.electrolier.globe` | Lamberts glass | `glass.mouth-blown` · leaded / leaded / cased-opal |
| `albion.case.glass`, `albion.frame.glazing` | case and frame glazing | `glass.low-iron` · laminated.anti-reflective |
| `albion.picture-rail`, `albion.electrolier.body`, `albion.north.blind-doors` | bronze | `bronze.cusn10` · patinated |
| `albion.porch.ceiling`, `albion.bench`, `albion.case`, `albion.lectern`, `albion.tapestry.batten`, `albion.frame.oak` | oak | `oak.sessile-french` · fumed.french-polished |
| `albion.frame.gilt`, `albion.frame.gilt.glazed` | frames | `oak.sessile-french` · `gilt.gold-leaf.oil` · aged.patina (the grain through the gold) |
| `albion.case.baize` | case beds | `accent.wool-felt` · laid · aged.patina |
| `albion.tapestry.lining` | the tapestry's lining | `accent.linen` · stretched · new |

The joint's threshold, strip, linings, surrounds and wedge masonry are Site's (travertine and brushed CuSn10).

## 4. Light

**Daylight** (Lighting.json `daylight`; every opening weather **london**, control none; no blinds, D-014)

| Opening | Glazing | Notes |
|---|---|---|
| `albion.vault.nave` | `glass.clear-float`, laminated UV-filtering | clear: the sky, the rain, the sun on the nave |
| `albion.vault.west`, `albion.vault.east` | `glass.clear-float`, laminated with a **diffusing** UV-filtering interlayer | 1,033 m² over the hanging walls |
| `albion.screen.north`, `albion.screen.south` | `glass.clear-float`, clear | the train-shed ends |
| `albion.gable.nw`, `.ne`, `.sw`, `.se` | `glass.clear-float`, diffusing | as the aisles |
| `albion.band` | `glass.mouth-blown`, leaded, protective glazing outside | 32.6 m² of stained glass |
| `albion.lancet.w1`…`w6`, `albion.lancet.e1`…`e6` | `glass.mouth-blown`, leaded, protective glazing outside | ≈ 61 m² |

**Care by the hang and the glazing (D-014; ruled here).** Under the museum's idealised sun (constant elevation
71.489°, tan 2.987; bearing east at 06:00, south at noon, west at 18:00) clear aisle vaults would put direct sun on the
**whole** west wall 06:00–12:00, the whole east wall 12:00–18:00 and the north wall all day: every work but the south
wall's. So:
1. The aisles' vaults and gables are glazed in the palette's clear float laminated with a **diffusing** UV-filtering
   interlayer: no sun patch through them; the nave vault and the nave's screens stay clear (the sky, the axis, the band).
2. Every glass that daylight crosses to reach a work filters UV: the laminated float, and a protective laminated pane
   0.05 outside every lancet and band panel. Works on paper are glazed in their frames (UV-filtering, anti-reflective).
3. The hang keeps every work out of every patch that remains:

| Direct sun lands on (idealised, 06:00–18:00) | Through |
|---|---|
| the nave floor, benches, columns and arcade; the nave's north end wall face (\|x\| < 6: court door, Ruskin) | the clear nave vault |
| aisle floors within 3.35 of the arcade lines (at a case top, 0.95: within 3.03; the cases start at 3.90) | the nave vault, under the arcade |
| aisle floors x ±11.29 … ±12.63 at 06:00/18:00, sliding north and towards the wall with the hour | the lancets (coloured): east ones in the morning, west in the afternoon |
| the nave floor y 14.08–15.29 at noon (2.9–5.7 north of the south screen), sliding east and west | the band (coloured) and the south screen's clear panes |
| the north wall's aisle ends within 2.4 of the arcade lines (to z ≈ 4.3 at 1.0 from the line, 0.65 at 2.4): the east one in the late afternoon, the west one in the early morning | the nave vault, through the arcade's first bay |
| **no hanging wall, frame, case, the tapestry or the lectern** | — |

   Two works move for it: the **Star of Bethlehem** 0.5 m east (centre x 10.5; as built at 10.0 its lower-left corner
   caught the 16:00–17:00 sun through the arcade) and the **lectern** from y −15.00 to −13.30 (lancet e1's early sun fell
   on the desk). The south wall (facing north) never takes sun.
4. Diffuse daylight in the glass court will exceed 50 and 200 lx in bright hours. That is measured (Art.WithinCare,
   annual dose tier 2), never tuned; if it fails, the answer is the glazing's measured transmittance (a denser diffusing
   interlayer: a Matter record) or the hang, never a blind or a dimmed light. Under the `ephemeris` sun model (low summer
   sun north of east) the clear north screen can reach the south wall; Art.WithinCare decides.

**Luminaires** (Lighting.json; 55; points and aims in the stage frame; no output values are set here)

| Ids | Type | Count | Point and aim | Operation |
|---|---|---|---|---|
| `frame.<work>` (side walls) | `track-framing.3000k.small` (outer ≤ 1.5) / `track-framing.3000k.medium` (larger) | 29 | a projector on a clamp under the arcade girder's bottom flange on the work's aisle side, (±6.45, y_work, 9.30) (two works stacked at one y: y ∓ 0.18); aim the work's centre on its face (±13.95, y, z) (≈ 45° down: the varnish's reflection falls short of a viewer) | presence |
| `frame.<work>` (end walls) | `track-framing.3000k.medium` | 4 | under the inner flange of the main rib of line 6 (y 12.0) for the south wall, line 2 (−12.0) for the north: (x, ±12.0, glass(x) − 0.80); aim the work (x, ±17.95, z). Shalott (−10.0, 12.0, 14.40); Attainment (0, 12.0, 16.20); Cophetua (10.0, 12.0, 14.40); Star (10.5, −12.0, 14.24) | presence |
| `spot.case.<object>` | `track-spot.3000k.12deg` | 8 | (±6.45, y_object, 9.30); aim the object's centre (±10.4, y, 0.55 + h/2) | presence |
| `spot.chaucer` | `track-spot.3000k.12deg` | 1 | line 2's main rib over the lectern (12.2, −12.0, 13.22); aim the open book (12.2, −13.30, 1.12), clear of the reader's head | presence |
| `electrolier.w1`…`w6`, `electrolier.e1`…`e6` | `gap.electrolier.3000k` | 12 | the globes' ring centre (±6.0, y_bay, 5.849); aim the floor below | dusk |
| `electrolier.porch` | `gap.electrolier.3000k` | 1 | (0, −21.25, 5.299); aim the floor | dusk |

glass(x) = 10 + √(7.0833² − (\|x\| + 1.0833)²) for \|x\| ≤ 6, 10 + √(5.38² − (\|\|x\| − 10\| + 1.38)²) for the aisles.
Projectors are the catalogue products on the iron (the clamp and stem their own); the as-built night white balance,
metering bias and bare point lights are not ported (the Eye meters, Visitor.OneEye).

**Targets**

| Surface | Class | Lux | Source |
|---|---|---|---|
| `albion.frame.gilt` (the 26 oils' faces) | oil-and-tempera | 150 rec / 200 max | Care |
| `albion.frame.gilt.glazed` (Lady Lilith, Love's Messenger, the Star) | works-on-paper | 50 | Care |
| `albion.frame.oak` (the three wallpapers) | works-on-paper | 50 | Care |
| `albion.lectern` (the Chaucer's pages) | works-on-paper | 50 | Care |
| `albion.tapestry.lining` (the Attainment, wool and silk) | textiles.silk | 50 | Care |
| `albion.case.baize` (the lustreware) | ceramic-and-glass | 300 recommended, no max ([300, null]) | Care |
| `albion.floor.nave`, `albion.floor.aisle` | gallery.low-light (ruled; the court holds 50 lx works) | 50 | UseTargets |
| `albion.porch.floor` | circulation / circulation.night | 100 / 50 | UseTargets |

No walked route falls below `escape-route`. **Design order** (Lighting.json `design`; measured, never tuned): by day,
from the gilt sun's eye, the sky through the south screen's clear panes ≥ the band ≥ the south wall's stone round the
tapestry ("the coloured band glowing above it"); every work ≥ its banded wall within 0.5 of its frame, day and night;
the iron soffits darker than the glass behind, never black; the band's and lancets' patches the only coloured light. By
night: the opal globes brightest, then the works, then the floors; the vault glass darkest. Light carries the band's and
lancets' colours by coloured transmission (ray-traced translucent shadow): no auxiliary lights (the as-built three RGB
directionals and light functions are not ported).

## 5. Time and weather
- Every Glazed opening takes the **london** observer (D-005): Westminster, 51.5072 N, 0.1276 W (`AlbionSky.cpp:34`),
  fetched with the museum and Giverny in one request; offline, Time's seeded generator on London's normals. Matter's
  surface law reads it on the glass, lead and outer stone: rain beading and rivulets down each pane's fall, the wet sheen
  drying slowly, snow lying on the upward facets and along the three ridges, soot along the bars (the glass's history).
- The sun and the hour are the museum's (D-003, idealised by default): elevation 71.489°, bearing 90° + 15°·(h − 6),
  06:00–18:00 (§4 for the patches). The Sections board's "London noon, 38.5°" is superseded.
- Not ported: London's cloud dimming the court's daylight and fog in the vaults (`AlbionSky.cpp:70-89, 241-293`): daylight
  comes from the museum's one sky; nothing owns fog inside an enclosure.

## 6. Motion
None tonight: no doors, no moving architecture; the port is an open portal. Polish: the Chaucer's leaf, a revolute body
on the spine's axis, 0° → 180° in 1.6 s, a Use "Turn the page" / "Turn back" within 2.4 m, the next opening's images
swapped at the end of travel, ≥ 5 mm clear of the ledge.

## 7. Nature
None inside: no plants, water or ground; Albion's plants are carved in its capitals (Form's). Outside, the lawn and the
gravel walk bending round the south end (14 m to the front, the Outside board) are the Grounds'; the Grounds' old east
walk (plan y 40–44) crossed Albion's footprint and must bend round it. Precipitation falls on the roof glass from the
`london` observer (Nature's, polish).

## 8. Art
**Sources:** every id from `data/albion.json` (Hang `source`); sizes from `assets/albion/paintings/SOURCES.json`
(sight, w × h), the wallpaper panels and the peacock dish from `assets/albion/hang.json`; hang places from
`assets/albion/hang.json` (y − 35.2); images `assets/albion/paintings/<id>.jpg`, the band `assets/albion/glass/`.
Captions from `data/albion.json`, credits and licences from `SOURCES.json` (all public domain, CC0 or CC BY-SA 3.0; the
Walters rice dish carries its credit).

**The hang rule** (Hang board; `albion_hang.py:26-187`): two deep round a 1.6 line; row 0 centred at 1.645–1.765, large
works raised so the frame's foot clears ≈ 0.55–0.75; row 1 over the work below with 0.18 between frames; top of the hang
≈ 3.2–3.6. Row-0 frames spread evenly between the pilasters (bay centre ± 2.85), gaps 0.30–0.90. Side-wall works hang
by two bronze rods from the rail; end-wall works on concealed bronze plates. Frames: reeded (Rossetti, Madox Brown,
Moore, Stillman; moulding 0.10, 0.13 if ≥ 1 m, outer = sight + 2 × 1.02 w), Watts (the rest; 0.11 < 1 m, 0.14 < 2 m,
0.17; outer = sight + 2 w), cabinet (< 0.5 m; 0.07; outer = sight + 2 × 0.9 w), gilt oak slips in arched and oval sights;
works on paper glazed. `along`/`height` place the **sight** (the painted or printed area); `size` is the sight; the face
stands 0.05 off the wall. Hang.json also carries `centre`, `frameStyle`, `frameWidth`, `sight` and `glazed`.

| Anchor | Work (id) · title · artist | Support · care | Sight w × h | Frame | along · height | Centre (x, y, z) |
|---|---|---|---|---|---|---|
| `albion.wall.west` | albion-millais-mariana · *Mariana* · Millais | panel · oil-and-tempera | 0.495 × 0.597 | `albion.frame.gilt` watts 0.11 | 32.752 · 1.351 | (−13.950, −15.000, 1.650) |
| `albion.wall.west` | albion-brown-pretty-baa-lambs · *The Pretty Baa-Lambs* · Madox Brown | panel · oil-and-tempera | 0.762 × 0.610 | `albion.frame.gilt` reeded 0.10 | 32.619 · 2.341 | (−13.950, −15.000, 2.646) |
| `albion.wall.west` | albion-millais-ophelia · *Ophelia* · Millais | canvas · oil-and-tempera | 1.118 × 0.762 | `albion.frame.gilt` watts 0.14, arched | 26.441 · 1.269 | (−13.950, −9.000, 1.650) |
| `albion.wall.west` | albion-brown-english-autumn-afternoon · *An English Autumn Afternoon* · Madox Brown | canvas · oil-and-tempera | 1.346 × 0.717 | `albion.frame.gilt` reeded 0.13, oval | 26.327 · 2.484 | (−13.950, −9.000, 2.842) |
| `albion.wall.west` | albion-hunt-awakening-conscience · *The Awakening Conscience* · Holman Hunt | canvas · oil-and-tempera | 0.559 × 0.762 | `albion.frame.gilt` watts 0.11, arched | 21.648 · 1.269 | (−13.950, −3.927, 1.650) |
| `albion.wall.west` | albion-brown-last-of-england · *The Last of England* · Madox Brown | panel · oil-and-tempera | 0.750 × 0.825 | `albion.frame.gilt` reeded 0.10, oval | 19.786 · 1.237 | (−13.950, −2.161, 1.650) |
| `albion.wall.west` | albion-millais-blind-girl · *The Blind Girl* · Millais | canvas · oil-and-tempera | 0.622 × 0.826 | `albion.frame.gilt` watts 0.11 | 15.643 · 1.242 | (−13.950, 2.046, 1.655) |
| `albion.wall.west` | albion-wallis-stonebreaker · *The Stonebreaker* · Wallis | canvas · oil-and-tempera | 0.787 × 0.654 | `albion.frame.gilt` watts 0.11 | 13.736 · 1.318 | (−13.950, 3.871, 1.645) |
| `albion.wall.west` | albion-hughes-long-engagement · *The Long Engagement* · Hughes | canvas · oil-and-tempera | 0.533 × 1.070 | `albion.frame.gilt` watts 0.14, arched | 9.456 · 1.110 | (−13.950, 8.277, 1.645) |
| `albion.wall.west` | albion-brown-walton-on-the-naze · *Walton-on-the-Naze* · Madox Brown | canvas · oil-and-tempera | 0.419 × 0.317 | `albion.frame.gilt` cabinet 0.07 | 7.934 · 1.491 | (−13.950, 9.856, 1.650) |
| `albion.wall.west` | albion-sandys-morgan-le-fay · *Morgan-le-Fay* · Sandys | panel · oil-and-tempera | 0.437 × 0.618 | `albion.frame.gilt` watts 0.11 | 3.570 · 1.341 | (−13.950, 14.212, 1.650) |
| `albion.wall.west` | albion-sandys-medea · *Medea* · Sandys | panel · oil-and-tempera | 0.456 × 0.612 | `albion.frame.gilt` watts 0.11 | 1.994 · 1.344 | (−13.950, 15.778, 1.650) |
| `albion.wall.east` | albion-morris-trellis · *Trellis* · Morris (birds by Webb) | paper · works-on-paper | 1.200 × 3.000 | `albion.frame.oak` panel 0.035, glazed | 0.657 · 0.200 | (13.950, −16.743, 1.700) |
| `albion.wall.east` | albion-morris-acanthus · *Acanthus* · Morris | paper · works-on-paper | 1.200 × 3.000 | `albion.frame.oak` panel 0.035, glazed | 2.400 · 0.200 | (13.950, −15.000, 1.700) |
| `albion.wall.east` | albion-morris-willow-bough · *Willow Bough* · Morris | paper · works-on-paper | 1.200 × 3.000 | `albion.frame.oak` panel 0.035, glazed | 4.142 · 0.200 | (13.950, −13.258, 1.700) |
| `albion.wall.east` | albion-stillman-loves-messenger · *Love's Messenger* · Spartali Stillman | paper · works-on-paper | 0.660 × 0.813 | `albion.frame.gilt.glazed` reeded 0.10 | 6.582 · 1.239 | (13.950, −11.088, 1.645) |
| `albion.wall.east` | albion-millais-portia · *Portia* · Millais | canvas · oil-and-tempera | 0.838 × 1.251 | `albion.frame.gilt` watts 0.14 | 7.814 · 1.020 | (13.950, −9.767, 1.645) |
| `albion.wall.east` | albion-de-morgan-flora · *Flora* · E. De Morgan | canvas · oil-and-tempera | 0.880 × 1.990 | `albion.frame.gilt` watts 0.14 | 9.261 · 0.750 | (13.950, −8.299, 1.745) |
| `albion.wall.east` | albion-leighton-lachrymae · *Lachrymae* · Leighton | canvas · oil-and-tempera | 0.629 × 1.575 | `albion.frame.gilt` watts 0.14 | 10.751 · 0.902 | (13.950, −6.934, 1.690) |
| `albion.wall.east` | albion-burne-jones-golden-stairs · *The Golden Stairs* · Burne-Jones | canvas · oil-and-tempera | 1.168 × 2.692 | `albion.frame.gilt` watts 0.17 | 13.228 · 0.749 | (13.950, −4.188, 2.095) |
| `albion.wall.east` | albion-moore-dreamers · *Dreamers* · Moore | canvas · oil-and-tempera | 1.210 × 0.700 | `albion.frame.gilt` reeded 0.13 | 15.599 · 1.295 | (13.950, −1.796, 1.645) |
| `albion.wall.east` | albion-burne-jones-pygmalion-1 · *The Heart Desires* · Burne-Jones | canvas · oil-and-tempera | 0.760 × 0.990 | `albion.frame.gilt` watts 0.11 | 18.616 · 1.150 | (13.950, 0.996, 1.645) |
| `albion.wall.east` | albion-burne-jones-pygmalion-2 · *The Hand Refrains* | canvas · oil-and-tempera | 0.760 × 0.990 | `albion.frame.gilt` watts 0.11 | 19.952 · 1.150 | (13.950, 2.332, 1.645) |
| `albion.wall.east` | albion-burne-jones-pygmalion-3 · *The Godhead Fires* | canvas · oil-and-tempera | 0.760 × 0.990 | `albion.frame.gilt` watts 0.11 | 21.288 · 1.150 | (13.950, 3.668, 1.645) |
| `albion.wall.east` | albion-burne-jones-pygmalion-4 · *The Soul Attains* | canvas · oil-and-tempera | 0.760 × 0.990 | `albion.frame.gilt` watts 0.11 | 22.624 · 1.150 | (13.950, 5.004, 1.645) |
| `albion.wall.east` | albion-burne-jones-love-song · *The Love Song* · Burne-Jones | canvas · oil-and-tempera | 1.559 × 1.143 | `albion.frame.gilt` watts 0.14 | 26.221 · 1.079 | (13.950, 9.000, 1.650) |
| `albion.wall.east` | albion-rossetti-beata-beatrix · *Beata Beatrix* · Rossetti | canvas · oil-and-tempera | 0.660 × 0.864 | `albion.frame.gilt` reeded 0.10 | 31.081 · 1.218 | (13.950, 13.411, 1.650) |
| `albion.wall.east` | albion-rossetti-lady-lilith · *Lady Lilith* · Rossetti | paper · works-on-paper | 0.440 × 0.513 | `albion.frame.gilt.glazed` reeded 0.10 | 32.774 · 1.393 | (13.950, 14.994, 1.650) |
| `albion.wall.east` | albion-rossetti-proserpine · *Proserpine* · Rossetti | canvas · oil-and-tempera | 0.610 × 1.251 | `albion.frame.gilt` reeded 0.13 | 34.278 · 1.020 | (13.950, 16.583, 1.645) |
| `albion.wall.south` | albion-waterhouse-lady-of-shalott · *The Lady of Shalott* · Waterhouse | canvas · oil-and-tempera | 2.000 × 1.530 | `albion.frame.gilt` watts 0.17 | 23.000 · 1.000 | (−10.000, 17.950, 1.765) |
| `albion.wall.south` | albion-morris-attainment · *The Attainment* · Morris & Co. after Burne-Jones | tapestry (wool and silk on cotton warp) · textiles.silk | 6.950 × 2.440 | none (lined, on its batten) | 10.525 · 1.000 | (0.000, 17.950, 2.220) |
| `albion.wall.south` | albion-burne-jones-king-cophetua · *King Cophetua and the Beggar Maid* · Burne-Jones | canvas · oil-and-tempera | 1.359 × 2.934 | `albion.frame.gilt` watts 0.17 | 3.321 · 0.698 | (10.000, 17.950, 2.165) |
| `albion.wall.north` | albion-burne-jones-star-of-bethlehem · *The Star of Bethlehem* · Burne-Jones | paper · works-on-paper | 3.870 × 2.560 | `albion.frame.gilt.glazed` watts 0.17 | 22.565 · 0.900 | (**10.500**, −17.950, 2.180) |

**Case objects** (Mount anchors; `centre` = the foot on the baize at z 0.550; care ceramic-and-glass; frame none; dishes
turned discs from their photographs, the two vessels turned from `vessels.json`'s profiles in its image window; handles:
polish):

| Anchor | Work (id) | Size w × h | Centre (x, y) |
|---|---|---|---|
| `albion.case.w3` | albion-de-morgan-charger (Met 29.32) | Ø 0.514 × 0.070 | (−10.40, −6.45) |
| `albion.case.w3` | albion-de-morgan-dish-eagle (Cleveland 1990.73) | Ø 0.252 × 0.040 | (−10.40, −5.50) |
| `albion.case.e3` | albion-de-morgan-peacock-dish (Birmingham 1981M15) | Ø 0.40 (unverified: hang.json) × 0.055 | (10.40, −6.30) |
| `albion.case.e3` | albion-de-morgan-vase (Met 23.163.2ab) | 0.241 × 0.349 | (10.40, −5.45) |
| `albion.case.w5` | albion-de-morgan-rice-dish (Walters 48.2756) | Ø 0.311 × 0.044 | (−10.40, 5.55) |
| `albion.case.w5` | albion-de-morgan-fish-dish (Chicago 1968.7) | Ø 0.314 × 0.050 | (−10.40, 6.45) |
| `albion.case.e5` | albion-de-morgan-ship-plate (Chicago 1938.358) | Ø 0.200 × 0.030 | (10.40, 5.60) |
| `albion.case.e5` | albion-de-morgan-jar (Cooper Hewitt 1992-124-2) | 0.240 × 0.380 | (10.40, 6.40) |

**The book** on `albion.lectern`: albion-kelmscott-chaucer, *The Works of Geoffrey Chaucer*, Kelmscott Press 1896
(Cleveland, Ingalls Library): folio leaf 0.425 × 0.290, block 0.066 (277 leaves), boards 0.003 with a 0.005 square, half
holland over blue-grey paper boards; open at the first opening (`assets/albion/chaucer/page_000`, `page_001`; 40 scans,
20 openings); size [0.59, 0.435]; care works-on-paper; static tonight.

**The Tristram and Isolde band** (D-014: Glazing anchors in `albion.band`; Art owns the images, Form the opening, Matter
the glass): albion-tristram-isolde (Morris, Marshall, Faulkner & Co., 1862, Harden Grange; Bradford Museums), twelve
entries `canvas` 1…12 on `albion.band.t1`…`t12`, each image 1.46 × 1.56 at along 0.07, height 0.07 inside its panel's
quarry border; support glass, care ceramic-and-glass (fired stained glass), frame none. Upper row T1 *The Birth of Sir
Tristram* (Hughes), T2 *The Fight with Sir Marhaus* (Rossetti), T3 *The Departure from Ireland* (Prinsep), T4 *The Love
Potion* (Rossetti), T5 *The Marriage with Isoude les Blanches Mains* (Burne-Jones), T6 *The Madness of Tristram*
(Burne-Jones); lower row T7 *The Attempted Suicide of La Belle Isoude* (Burne-Jones), T8 *The Recognition* (Morris or
Burne-Jones), T9 *At the Court of King Arthur* (Morris), T10 *King Mark slays Tristram* (Madox Brown), T11 *The Tomb*
(Burne-Jones), T12 *Queen Guenevere and Isoude les Blanches Mains* (Morris); images
`assets/albion/glass/tristram-panel-T<k>-*.jpg`. T13 stays out. The originals (≈ 0.68 × 0.605) are glazed here ≈ 2.3–2.4×,
as the approved design states (Art.TrueSize: §13).

**The lancets** (`albion.lancet.e1`…`e6`: Chaucer's Good Women after Burne-Jones; `albion.lancet.w1`…`w6`: Webb's birds
and Morris's flowers in diamond quarries; images `assets/albion/glass/goodwomen-*`, `quarries-*`, `mmf-1860s-*`) have no
ids in `data/albion.json`: not in Hang.json tonight; Form glazes them in plain leaded quarries (§13).

No labels on the walls; captions on look (every work, case object, panel, each column: "B · Peterhead granite,
Aberdeenshire · the capital: poppies, pansies and violets, from *Ophelia*", the two tablets). No scans, no sculpture.

## 9. Sound
- **Acoustics** from Matter, never tuned: `albion.court` ≈ 14,583 m³ of honed stone, tile, York stone and ≈ 2,115 m² of
  glass (vaults 1,768, screens and gables ≈ 286, lancets ≈ 61): a long, live court. `albion.porch` (219 m³; stone, tile,
  an oak ceiling) shorter and drier, coupled through the court door and the open portal to the Rotunda.
- **Emitters:** rain on the vault glass and the lead at the **london** observer's rate ("rain runs down the glass and
  drums on the ribs"), wind on the glass from the same; footfalls by substance (encaustic tile, York stone, the joint's
  travertine). Nothing else sounds (no sound as built).

## 10. Visitor
- **Arrive** from the gilt sun looking south: 53 m down the nave to the Attainment, the band glowing above it.
- **The walk:** the porch (encaustic tiles, Ruskin over the court door) → the west aisle walked south (truth to nature,
  1848–68) → the legend at the south end (Shalott, the Attainment, Cophetua; the band above) → the east aisle walked
  north (beauty, 1860–96) → Morris's bay (E1: the wallpapers and the Chaucer) → the Star of Bethlehem on the way out.
  The nave between the arcades is open circulation; nothing stands on the axis.
- **Sit** on the two benches (the eight Seat anchors, facing the aisles). **Look:** captions (§8). **Use:** the Chaucer's
  page (polish). **Pass:** the court door and the port are open.
- **Step-to (VR):** floors only (never a case, bench top, sill or the lectern). Every standable place reaches
  `albion.north`; cases stand ≥ 3.0 from the walls; no invisible guards: collision = the visible solids.

## 11. The old-code map
Paths relative to `windows\`; **y − 35.200** throughout. Re-express, never copy.

| Part | Where |
|---|---|
| Plan numbers (court, heights, columns, arcade, vaults, lancets, band, porch, doors, buttresses, furniture) | Source/MuseeVision/Albion/AlbionPlan.h:34-118 |
| The Rotunda's door and drum; the porch envelope; the Grounds' east walk | Source/MuseeVision/Plan/MuseePlan.h:30-47, 528, 568-586 |
| Slots, stones A–J, band rule, openings, vault halves | Albion/AlbionBuild.h:16-110; AlbionMasonry.cpp:727-784 |
| Pointed arches, openings, banded faces, reveals | Albion/AlbionKit.h:262-309, 318-445 |
| Masonry constants; cornice; side walls, rail; hood mould, tablets; end walls; cornices, corbels | AlbionMasonry.cpp:14-24, 134-141, 145-200, 202-265, 271-332, 334-353 |
| Outside walls, lancets outside, plinth, buttresses, parapets, copings | AlbionMasonry.cpp:357-530 |
| Floors; porch (walls, lining now Site's, ceiling, lead flat, floor) | AlbionMasonry.cpp:534-572, 576-720 |
| Column carving; base, capital, abacus, shaft, placing | Albion/AlbionColumns.cpp:16-547, 549-695 |
| Clusters; girders; arcade arches and rings; ribs, purlins, bars; screens; saddle bars, lead, ventilator, rainwater; bearers | Albion/AlbionIron.cpp:85-134, 138-185, 189-233, 237-321, 325-415, 419-518, 525-570 |
| Glass: vault panes, screens, band atlas, lancets | Albion/AlbionGlass.cpp:18-164 |
| Benches, table cases (the guard box not ported) | Albion/AlbionFurniture.cpp:26-122 |
| Electroliers (fittings, globe height, body); projectors | Albion/AlbionLamps.cpp:26-62, 146-234, 256-298; Scripts/albion_place.py:52-82 |
| London weather (observer; fog and sun hacks not ported) | Albion/AlbionSky.h:13-31; AlbionSky.cpp:34, 70-89, 188-329 |
| Lectern and book | Albion/AlbionChaucer.cpp:25-47, 209-306; Scripts/albion_place.py:17-49 |
| The hang: rule, frames, layout; the laid-out hang | Scripts/albion_hang.py:26-187; repo assets/albion/hang.json |
| Materials (evidence only), tiles, stones, band, lancets | Scripts/albion_materials.py:86-212; albion_textures.py:11-24, 127-300; albion_stones_cc0.py:25-36; albion_glass_band.py:18-46; albion_glass.py:33-40, 255-440 |
| Captions, sizes, licences | repo data/albion.json; assets/albion/paintings/SOURCES.json; assets/albion/vessels.json |
| Boards | repo plan/proposals/albion/canvas/*.dc.html |

**Corrections to the draft** (`r-specs/…/Stages/albion.md`):
1. **Class** `doorway.4000x6500.arched` (width × crown; draft `doorway.4000x4500.arched`). Frame (0, −24.00, 0) facing −y
   and museum translation (0, +35.20, 0) verified.
2. **The port's wall** is the as-built 0.10 lining (porch face y −23.90 = plan 11.30), not a new 0.30 wall: the porch
   stays 5.30 inside, the three beams keep their symmetric bays (1.30/1.35/1.35/1.30), the porch field runs to −23.60.
3. **Surround clearance** in the Chenghuai convention: the porch face clear for |x| < 2.35, z < 6.85 (draft: ≤ 0.35 at the
   head, ≤ 0.90 at the jambs); the joint's wedge masonry to |x| 3.65, z 7.50 kept.
4. **Materials are palette keys** (D-018; draft's substance.working.history keys): the iron `accent.victorian-iron` in
   iron-oxide brown-red (draft slate grey #434C50); frames gilded oak (draft gilt pine); rail, rods, electroliers
   patinated CuSn10 (draft brushed, tarnish); vaults and screens `glass.clear-float`; lancets and band
   `glass.mouth-blown`; baize `accent.wool-felt`.
5. **Daylight and care** (draft's open item 1) ruled by the principles (§4): the draft's "direct sun on the west wall to
   about 3.2 m" understated it (the whole wall 06:00–12:00); aisle glazing diffusing, protective glazing on the leaded
   glass, the Star 0.5 east, the lectern to y −13.30.
6. **Sun** elevation 71.489° (draft 71.53°, the old sky's 20.30); lancet patches at 06:00 x 11.29–12.63 (draft 11.0–12.3).
7. **Light:** catalogue types with points and aims (draft: "Scheme chooses"); electroliers a gap at 3000 K (draft's new
   2700 K records; 3000 K is ruled museum-wide); the Chaucer's spot from line 2's rib (over the reader's head from the
   girder); the porch electrolier under the middle beam at y −21.25 (as built at −21.30 its rose sat inside the beam).
8. **Targets:** court floors `gallery.low-light` (ruled); ceramics 300 recommended; oils [150, 200].
9. **Hang:** `along`/`height` computed from the as-built centres on four Wall anchors; frames are Finishes surfaces; the
   band is twelve `canvas` entries of albion-tristram-isolde; the lancets, lacking ids, wait; the wallpapers and Love's
   Messenger glazed (as the draft asked).
10. Kept from the draft: brackets at the light ribs' feet (as built midway, `AlbionIron.cpp:556`); case legs turned inside
    0–0.36; no guard boxes; the court door as a shared opening.
11. **Verified unchanged:** plan constants, coursing (0.30 bands; the 0.28 header comment is wrong), lancet geometry
    (springing 8.199), pilasters and corbels, cornice, court door and hood mould, tablets, cill, plinth, buttresses,
    parapets and copings, floors, column profiles and bell law, every iron section, arcade (R 3.602), vault arcs, rib and
    purlin counts, glazing bars (48 at 0.747), screens, saddle bars, lead, rainwater, porch ceiling, benches, cases, all
    sizes against SOURCES.json, hang places, electrolier geometry.

## 12. The first-pass cut
**Tonight:** Site (frame, footprint, the port with its class and rough opening, blinds on the stand, the enclosures, the
shared court door, Albion.AxisClear); masonry by coursing inside and out (bands, skirting, pilasters and corbels, cornice
sweeps, end walls, cills and returns, lancets and court door with chamfers, reveals and sills, the porch's walls and
lining, plinth, buttresses, parapets, copings, hood mould; plain tablets); floors (tiles by rule if the roundel inlay
isn't in Matter's image; York flags; the court door's reveal and strip); columns (bases, ten polished shafts, necks,
bells, abaci; capitals boasted, as many at Oxford stayed for years); iron (clusters without leaves, plain box girders,
arcade arches with rings, main ribs with inner arch and chain, light ribs, purlins, bars, bearers, brackets, screens,
saddle bars; lead gutters, rolls, flat, plain ventilator cap; rainwater); glass (vault panes with laps, clear and
diffusing; screens and gables; the band from its twelve photographs; lancets in plain leaded quarries; protective
glazing); the porch ceiling; the hang (33 wall works, frames, slips, glazing, rods; the tapestry on its batten; 8 case
objects; the Chaucer static); benches, cases, lectern, rail; the 55 luminaires; London's weather on the glass;
acoustics and footfalls.

**Waits for polish:** the capitals' carving and the clusters' cast leaves; rivets, bolts, splice plates, the founder's
bead; ventilator louvres and standards, cresting rings; label stops; V-cut lettering; the roundel inlay; the tapestry's
folds; vase handles; the Chaucer's leaf; the lancets' designs (after catalogue ids); precipitation past the glass; soot
terms; the electroliers' scroll detail; better images (Stonebreaker 703 px, Golden Stairs 850, Proserpine 1,041,
Cophetua 1,054, Portia 1,268, Acanthus 1,328).

**Risks (watch tonight's checks):** diffuse daylight in a glass court exceeds the care maxima in bright hours (§4.4:
Art.WithinCare measures; the remedy is a glass record or the hang, never a blind); coloured transmission through the band
and lancets needs Light's translucent transport; ≈ 4,000 vault panes and their bars must be instanced; Purbeck and Frosterley
monoliths of 5.165 are rare (drums if Matter's stock says so).

## 13. Palette keys to confirm and catalogue gaps
**Palette keys** (no `todo.` keys; roles guessed or workings outside `workingsByEra`):
- `glass.clear-float` working **`laminated.uv-filtering.diffusing`** for the aisle vaults and the four gables: the same
  float laminated with a translucent, diffusing UV-filtering interlayer (a matte-white PVB of the Saflex Vanceva
  "Arctic Snow" / Trosifol diffuse class). The care ruling of §4 rests on it.
- `glass.clear-float` for the **protective glazing** outside the lancets and band (role guessed).
- `glass.low-iron` for the **frame glazing** of the works on paper (roles name case glass only).
- `accent.milled-lead` for the **cames**; `accent.victorian-iron` for the **rainwater goods, crestings, ventilator and the
  tapestry's brackets** (roles name the iron inside).
- `accent.linen` for the **tapestry's lining** (roles name the Salon and the oval).
- `accent.bath-stone` **sawn** for the hidden rough opening; `bronze.cusn10` patinated for the **blind doors** (by
  analogy with the Grounds' blind door).
- `accent.british-column-stones`: one key for ten stones; Matter must carry the ten selections A–J (surfaces
  `albion.shaft.a` … `.j` name them).
- Workings written as compound strings: `honed.bevelled`, `polished.hand-touched`, `fumed.french-polished`,
  `laminated.uv-filtering`, `laminated.anti-reflective`, `cased-opal`, `leaded`, `laid`, `stretched`.

**Catalogue gaps** (Light):
- `gap.electrolier.3000k`: a reproduction 1880s electrolier (turned patinated-bronze body, six or four scrolled arms,
  cased opal globes Ø 0.17) with dimmable LED lamps, 3000 K, CRI > 90, on a chain or stem; 13 fittings.

**Other gaps:** Art: `data/albion.json` has no ids for the twelve lancets' designs (the Good Women after Burne-Jones;
Webb's birds and Morris's quarries); the band is glazed ≈ 2.3–2.4× its 1862 panels (Art.TrueSize needs an "after" record
or an exception). Contract: the anchor kind **Glazing** (D-014) is not in the stage contract's kind table.

## Build notes
- **Frame and translation.** Origin at the court's centre on the Rotunda's floor; **museum = local + (0, 35.200, 0)**,
  yaw 0; every old number y − 35.2.
- **The port.** `albion.north` at (0, −24.000, 0) facing −y, flat, tangent to the drum; class `doorway.4000x6500.arched`;
  passage 0. Albion builds only the rough opening through its 0.10 end lining and nothing past the port plane. The joint
  builds the threshold, strip, linings, surrounds and the wedge masonry (|x| ≤ 3.65, z ≤ 7.50). Keep the lining's porch
  face clear for |x| < 2.35, z < 6.85.
- **Draw by rule, not by hand:** the coursing (`BandCuts`), the two-centred arches (lancets, court door, arcade, vaults)
  from their radii and centres, the vault panes as flat facets lapping 10 mm, the bell law, the glazing bars at 36.6/49.
- **Two glasses in the vaults:** clear over the nave, diffusing over the aisles (and their gables). The band and lancets
  are Lamberts glass in cames behind a protective pane.
- **Glazing anchors**: the band's twelve panels are Art's images in Form's `albion.band` opening; the lancets' anchors
  are reserved.
- **The iron** has no collision and casts shadows; the electroliers and projectors are Light's fittings on it.
- **Nothing moves; no guards**; the axis from the gilt sun stays clear (Albion.AxisClear).

**Departures from as-built** (for the owner's review):
- **The aisles' vaults and gables are glazed in diffusing laminated float** (as built clear), and a protective laminated
  pane stands outside every lancet and band panel: under the idealised sun clear aisle glass put direct sun on every
  side-wall work for half the day (care by glazing and hang, D-014; physically real, no blind).
- **The Star of Bethlehem moves 0.5 m east** (centre x 10.5) and **the lectern 1.7 m south** (y −13.30), out of the last
  sun patches.
- **Works on paper glazed** (Love's Messenger and the wallpapers were not).
- **The port:** the porch's 0.10 lining ends on a flat port plane tangent to the drum; the reveal and roll moulding
  against the drum become the joint's; the porch's walls, flat and copings stop at the port plane (as built they ran to
  the drum).
- **Materials follow the Story:** iron painted iron-oxide brown-red (was blue-grey), gilded oak frames (was the frames'
  bronze material; the draft's pine), patinated CuSn10 rail and electroliers (was brushed), clear float with its green
  edge and Lamberts glass.
- **Light:** listed 3000 K catalogue projectors on the iron and 3000 K electroliers replace the as-built spots, 2900 K
  point lights, night white balance and metering bias; the Chaucer's spot on line 2's rib; the porch fitting under the
  middle beam.
- **Not ported:** London's cloud dimming the daylight, fog in the vaults, the three RGB directionals, the cases' guard
  boxes.
