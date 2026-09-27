# Albion: the stage sheet

Albion is the Victorian iron-and-glass court on the Rotunda's south door: England 1848–1898, after the Oxford
University Museum. Ten polished British stone columns carry clustered iron, pointed glass vaults and two iron-and-glass
end screens. The south screen holds the Tristram and Isolde band. London's weather falls on its glass (D-005).

The Room Builder builds `MuseeAlbion` from this sheet tonight (D-016, first pass). Every number is in metres, in the
stage's own frame, unless it is marked as plan. It is re-expressed from the as-built code and the approved proposal
(`plan/proposals/albion`), and follows Decisions D-001 to D-016.

**Sources, in order of authority:**
1. Decisions D-001 to D-016.
2. The approved proposal boards (`plan/proposals/albion/canvas/*.dc.html` and `boards/*.png`).
3. The as-built code (`windows/Source/MuseeVision/Albion/*`, `windows/Scripts/albion_*.py`).
4. The data (`assets/albion/hang.json`, `assets/albion/paintings/SOURCES.json`, `data/albion.json`,
   `assets/albion/vessels.json`).

Where the code and a comment disagree, the code's number wins, and the conflict is noted here.

---

## 1. Identity and placement

| | |
|---|---|
| Stage key | `albion` (display text *Albion*, FText) |
| Module | `MuseeAlbion` (Editor), `Source/Stages/Rooms/Albion`; class `UAlbionRoom : USiteRoom` |
| Plan numbers | Only in `Private/AlbionPlan.h`. Its mouldings are singular to Albion, so they stay private (the promotion rule) |
| Era and working | Classical wing. Stone is honed, rubbed or polished. The iron is painted. The glass is clear, or pot-metal and leaded |

### The frame

- **Origin:** the centre of the court, on the nave's axis, at the finished floor (the top of the encaustic tiles).
- **Axes:** x east, y south, z up, with plan north at true north. Site only translates the stage and never rotates it.
- **In the museum plan:** with the Rotunda's centre as the plan origin, Albion's origin sits at plan **(0.0, 35.2)**,
  at level **0.00** (the Rotunda's floor, museum level 0).
  - To convert: **local = plan − (0, 35.2, 0)**.
  - The Rotunda's gilt sun is at local (0, −35.2, 0).
  - Mate solves this translation from the ports (§2). The number here is the design value that Mate must reproduce.
- **Setting-out:**

| Item | Local value |
|---|---|
| Column lines 1 to 7 (every 6.0) | y = −18, −12, −6, 0, 6, 12, 18 |
| Bays 1 to 6, centres | y = −15, −9, −3, 3, 9, 15 |
| Arcade lines | x = ±6.0 |
| Aisle axes | x = ±10.0 |
| Side walls' inner faces | x = ±14.0 |
| End walls' inner faces | y = ±18.0 |
| Wall thickness (outer faces x ±14.6, y ±18.6) | 0.60 |

### Level

One storey at z 0.00. Nothing is below the floor except footings: the plinth course and the buttress base courses run
down to z −1.20, because the Grounds' lawn falls away to the south (`AlbionMasonry.cpp:23`). The Grounds' terrain
decides how much of each footing shows.

### Footprint (Building)

The footprint is an envelope polygon in local metres, walked clockwise as seen on the plan with north up:

`(−3.70, −24.00) → (3.70, −24.00) → (3.70, −19.60) → (15.60, −19.60) → (15.60, 19.60) → (−15.60, 19.60) →
(−15.60, −19.60) → (−3.70, −19.60)`

- **Height:** z −1.20 to 17.70. The nave's ridge ventilator cap is the top, at 17.60.
- **What the envelope covers:**
  - buttresses and their base courses: x ±15.56, and y ±19.56 on the end walls;
  - rainwater heads: up to 0.35 m off the walls;
  - copings;
  - the porch to the port plane at y −24.00.
- **The Grounds** should pave or plant to the ground-contact outline, not to this envelope. That outline is the plinth
  line (x ±14.68, y ±18.68, and the porch at x ±3.68) plus the 18 buttress lobes. Form derives it from the made pieces.

### Enclosures

| Key | Kind | Bounds | Volume |
|---|---|---|---|
| `albion.court` | Enclosed | x ±14, y ±18, floor 0 up to the glass of three pointed vaults (springing 10.0; nave crown 17.0, aisle crowns 15.2) | ≈ 14,580 m³ (section 405.1 m² × 36.0) |
| `albion.porch` | Enclosed | x ±3.0, y −23.70 to −18.60, z 0 to 6.90 (oak ceiling) | ≈ 211 m³ |

**The court's openings:**
- Glazed: `albion.vault.nave`, `albion.vault.west`, `albion.vault.east`, `albion.screen.north` and `albion.screen.south`
  (the south screen contains the Tristram band), and `albion.lancet.w1` to `w6` and `albion.lancet.e1` to `e6`.
- The court door, shared with the porch.

**The porch's openings:** the port `albion.north` and the court door.

**The court door is shared by two enclosures of one stage.** Form's opening kinds (Glazed, Open, Port, Housing) have no
kind for an interior doorway. Form must accept one opening shared by two enclosures. If it cannot tonight, declare one
enclosure `albion.court` that includes the porch. Sound then uses one RT60 for both (see §12, *Open for the Architect*).

---

## 2. Ports

### `albion.north`: the stage's only port

| Field | Value |
|---|---|
| Kind | Doorway |
| Class | `doorway.4000x4500.arched`: 4.00 m clear, semicircular head springing at 4.50 (crown 6.50). This is the class of the Rotunda's north and south doors (`MuseePlan.h:44-47`) |
| Frame | Local (0.00, −24.00, 0.00), which is plan (0, 11.2, 0). This is the outer face of the porch's north end wall, at the threshold, facing north (−y) |
| Face shape | Flat. It is tangent to the drum's outer face (R 11.2) on the axis |
| Wall depth | 0.30 (the porch's north end wall) |
| Mates | `rotunda.south`: cylindrical face R 11.2, drum 1.20 deep. Passage length 0 m |
| Closure | Open portal (no leaves) |
| Wells | None |

**What Albion builds at the port.** Albion builds only the rough opening: the clear opening plus the class allowance,
cut through a 0.30 m end wall that spans the porch (x ±3.60, from z 0 to the coping line at 7.47).

**What Site's joint builds:**
- the travertine threshold through both wall depths;
- the bronze strip on the mating plane (local y −24.00);
- linings in front of both rough faces;
- surrounds on the Rotunda's face and on the porch's face;
- passage masonry that closes the wedges between Albion's flat face and the drum.

**The wedges.** The wedge is 0 on the axis and grows to 0.612 m at x ±3.65. It runs the full height of the porch
walls, the lead flat and the copings. So the class's **collar envelope must be at least 7.30 m wide (x ±3.65) and
7.50 m high**. Those are the old museum's `Facade::VestibuleHalf` and `VestibuleTop` (`MuseePlan.h:528`). A smaller
collar leaves a gap between the porch and the drum. That gap is a leak, and Form.Sealed catches it on the assembled
museum.

**The surround on the porch face** must fit inside the porch:
- no more than 0.35 m outside the rough opening at the head (the ceiling is at 6.90, the beams' soffit at 6.61);
- no more than 0.90 m at the jambs (the walls are at x ±3.0).

**Changes from the as-built porch:**
- The old porch had no north wall. A 0.10 m banded "lining" at plan y 11.30 carried a roll moulding round the Rotunda
  door (`AlbionMasonry.cpp:603-651`). That detail is now Site's surround, because every doorway shares one detail.
- The new 0.30 m end wall shortens the porch's inside from 5.30 m to 5.10 m (y −23.70 to −18.60).

**On the stand**, Site blinds `albion.north` with the class's blind.

### Topology (D-012)

- **The mating.** `rotunda.south ↔ albion.north` (passage 0 m) is Albion's only mating. It is written as one line in
  `Data/Site/SitePlan.json`.
- **A dead end.** Albion declares no other port, and it serves and receives no well. The Plan board says: "No other
  doors: the court is a dead end, like the Nymphéas oval".
- **Residency.** The court has exterior Glazed openings, so Site keeps the Grounds resident while the visitor is in
  Albion. The open portal puts Albion in the Rotunda's visibility graph, and the Rotunda in Albion's.
- **The axis.**
  - The visitor arrives on the gilt sun at the centre of the Rotunda, facing west. The view south runs 53.2 m down
    the nave to the Attainment tapestry.
  - Albion registers the check **Albion.AxisClear**: a 0.60 m-diameter sightline from the gilt sun's eye point,
    local (0, −35.2, 1.60), to the tapestry's centre, local (0, 17.95, 2.22), meets no piece and no fitting.
  - "Nothing stands on the axis."

---

## 3. Pieces by trade

Profiles are point lists (A, z): A is out from the face (or r for a lathe) and z is height. A point marked `s` is
smooth, meaning its normal is averaged. Each material key is `substance.working.history`; the full list is in §3.10.

### 3.1 Masonry: the court's walls, inside

**Coursing.** The same rule applies to every banded face: side walls, end walls, the porch inside, and the outer faces
from the plinth up.

| Band | Height |
|---|---|
| Slate skirting | 0.00–0.20 |
| Buff base course (one course) | 0.20–0.90 |
| Then a 1.20 m rhythm: one red course of 0.30 and three buff courses of 0.30 | from 0.90 |
| Red courses | 0.90–1.20, 2.10–2.40, 3.30–3.60, 4.50–4.80, 5.70–6.00, 6.90–7.20, 8.10–8.40, 9.30–9.60, and outside also 10.50–10.80 |

- **Blocks:** buff 0.75 long, red 0.95 long, half-bond with ±0.2 jitter, 4 mm joints (`albion_materials.py:90-99`).
- **Conflict:** the header comment at `AlbionPlan.h:23` says the band is 0.28 m. The code (`AlbionPlan.h:50`) says
  0.30 m, and 0.30 m is used here.

**Side walls** (x ±14.0, inner faces), from y −18 to 18 and z 0 to 9.73:
- **Lancets:** six a side, one on each bay's centre line (y −15, −9, −3, 3, 9, 15).
  - The light is 1.40 wide (half 0.70), with its sill at 5.00 and its head at 9.00.
  - The head is two-centred: radius 0.808, the centres 0.108 across the axis, springing at 8.199.
  - The opening is chamfered 0.06 on the inner face and revealed to the glass line 0.30 into the wall
    (x ±14.30, the wall's middle).
  - The inner sill is level at 5.00, running from the chamfer's foot to the glass.
  - Material: `bath-stone.rubbed.soot-interior`.
- **Pilasters:** on lines 2 to 6 (y −12, −6, 0, 6, 12), 0.30 wide by 0.10 proud, from 0.19 to 9.30, in dressed stone.
  - A three-step corbel above each pilaster carries a main rib's shoe:

    | Heights | Width | Proud |
    |---|---|---|
    | 9.30–9.45 | 0.34 | 0.15 |
    | 9.45–9.60 | 0.38 | 0.20 |
    | 9.60–9.74 | 0.42 | 0.26 |

  - The skirting returns round each pilaster.
- **Aisle cornice:** a chamfered string under a fillet, 9.72 to 10.00. It is swept round each aisle:
  - west aisle: from the south wall at x −6, west, north along the west wall, then east along the north wall to x −6;
  - east aisle: the mirror image.
  - Profile (A, z): (−0.01, 9.72), (0.03, 9.72), (0.075, 9.77), (0.075, 9.84), (0.12, 9.88), (0.12, 10.00),
    (−0.01, 10.00).
- **Corner corbels:** one in each aisle corner (x ±14, y ±18) for the gable ribs, 0.22 × 0.22, from 9.35 to 9.74.
- **Skirting:** Welsh slate, 0.20 high and 0.015 proud, with a 4 mm bevel on the top arris, in 1.2 m lengths.
  Material: `welsh-slate.honed.worn`.

**End walls** (y ±18.0, inner faces), 0.60 thick:
- The **aisles' ends** (|x| 6 to 14) rise to 10.00. Their tops carry the gables' glass, which stands on the wall's middle.
- The **nave's ends** (|x| ≤ 6) are stone to **8.50**, with the iron-and-glass screen above.
  - **Returns** at x ±6 run from 8.50 to 10.00.
  - **The nave's cill** under each screen:
    - its front runs from 8.38 to 8.52, 0.02 proud of the inner face;
    - its top weathers outwards from 8.52 to 8.47, 0.05 beyond the outer face.
- **The court door**, in the north wall between the porch and the court (y −18.6 to −18.0):
  - 4.00 wide and pointed: springing 3.30, crown 5.50, radius 2.21, the centres 0.21 across.
  - Chamfer 0.10 on the court face and 0.06 on the porch face, with the reveal between.
  - A **hood mould** runs on the court face. Its path is the door outline grown by 0.16, with level returns of 0.28 at
    the springing. Its drip section (across the path, out from the wall) is (−0.045, −0.01), (0.045, −0.01),
    (0.045, 0.035), (0.02, 0.07), (−0.03, 0.07), (−0.045, 0.05).
  - **Label stops** at the return ends: 0.14 wide, 0.10 proud, from 3.14 to 3.36.
- **Ruskin's tablet** is over the court door on the court face: x ±4.60, z 6.50 to 7.50, 0.03 proud, with a 0.02
  chamfered edge. Its text is REJECTING NOTHING · SELECTING NOTHING · SCORNING NOTHING (Ruskin, *Modern Painters* I,
  1843). The letters are V-cut in Centaur capitals.
- **The ALBION tablet** is on the north wall's court face in the west aisle: x −12.60 to −7.40, z 1.85 to 3.45,
  0.03 proud. Its text is ALBION / 1848 · 1898.

### 3.2 Masonry: outside, and the porch

- **Outer faces:**
  - side walls: x ±14.60, banded from the plinth to the parapet's top at 10.90;
  - aisle ends: y ±18.60, banded from the plinth to 10.90;
  - nave ends: banded to 8.50.
  - On the north face the porch covers x ±3.60 up to its roof, and the banded stone continues above it to 8.50.
- **Plinth course:** runs round the court and the porch, 0.08 proud, with its top at 0.60 and footings to −1.20.
  - Profile (A, z): (−0.02, −1.20), (0.08, −1.20), (0.08, 0.52), (0.00, 0.60), (−0.02, 0.60).
  - Material: `bath-stone.rubbed.weathered-urban`.
- **Buttresses:** 18 in all, each 1.00 wide.

| Where | Count | Deep | Top |
|---|---|---|---|
| Side walls, on lines 1–7 | 14 | 0.90 | 9.90 |
| End walls, at x ±6 on the north and south faces | 4 | 0.90 | 10.40 |

  - Each has a full-depth lower stage from 0.60 to the set-off at 4.80.
  - A weathering rises 0.35 (to 5.15) while the face steps back to 0.558.
  - The upper stage runs to top − 0.60, then a steep weathering takes the face back into the wall at the top.
  - A base course, 0.06 proud all round, runs from −1.20 to 0.52 and is weathered to 0.60.
- **Lancets, outside:**
  - chamfer 0.06;
  - reveal from the face to the glass line, 0.30 deep;
  - an outer sill weathered from 5.04 at the glass to 5.00 at the face;
  - a drip block 1.68 wide, 0.07 proud, from 4.88 to 5.00.
- **Parapets and gutters:**
  - Side walls: the parapet is 0.30 thick (inner face x ±14.30), from 10.00 to 10.90.
  - The lead gutter behind it runs from x ±14.00 to ±14.30 at 10.012 and turns up 0.15 on the parapet.
  - End walls over the aisles: the parapets' inner faces are at y ∓18.30, between |x| 6.0 and 14.3. Each parapet's end
    faces the nave over the cill.
- **Copings:** a weathered cap over every parapet, with a 0.05 overhang and a drip on both faces.
  - Section (A out, z): (−0.35, 10.88), (0.05, 10.88), (0.05, 10.96), (−0.15, 11.05), (−0.35, 10.96).
  - It is swept as two U-shaped runs, one from x −6 round the west side and one from x +6 round the east side.
- **The porch:**
  - **Side walls:** inner faces at x ±3.00 and outer faces at ±3.60, from the port plane (y −24.00) to the court's
    north outer face (y −18.60).
    - Inside, they are banded to the ceiling at 6.90, with a slate skirting.
    - Outside, they are banded to 7.35.
  - **North end wall:** 0.30 thick (y −24.00 to −23.70), x ±3.60, carrying the port's rough opening (§2). Inside it is
    banded, with a skirting that stops at the opening.
  - **South end:** the court wall's porch face carries the court door (0.06 chamfer), with skirting to
    x ±(2.00 + 0.061).
  - **Porch parapets:** inner faces at x ±3.35, rising from the lead flat at 7.20 to 7.35.
  - **Copings:** from x ±3.30 to ±3.65, 7.35 to 7.47.

### 3.3 Floors

| Floor | Extent | Laying | Material |
|---|---|---|---|
| Nave field | x ±5.70, y ±17.70 | 152.4 mm (6-inch) encaustic tiles on the 45° diagonal, 2 mm dark-grout joints. Pattern after Minton and Pugin: 2 × 2 tiles make a roundel, each group framed by plain black tiles with a buff square at the crossings. One repeat is 2.586 m (`albion_textures.py:11-17, 127-128`) | `encaustic-tile.waxed.trodden` |
| Nave border | 0.30 wide round the field, mitred at the corners (x ±6.0, y ±18.0) | 1½-inch black fillet, a row of 6-inch border tiles (a running trefoil in buff on red), a buff fillet, a chequer of 3-inch black and red squares | `encaustic-tile.waxed.trodden` (border pattern) |
| Aisles | x −14.0 to −6.0 and 6.0 to 14.0, y ±18.0 (run 0.01 under the walls) | York stone flags in 0.60 courses along the aisle, random lengths 0.6–1.2 (mean 0.90), broken bond, 5 mm lime-mortar joints | `york-stone.sawn.trodden` |
| Porch field | x ±2.70, y −23.40 to −18.90 | encaustic tiles as in the nave | `encaustic-tile.waxed.trodden` |
| Porch border | 0.30 wide round the porch field | as the nave border | `encaustic-tile.waxed.trodden` |
| Court door reveal | x ±2.00, y −18.60 to −18.00 | York stone, with a bronze strip 0.06 × 4.03 on the wall's middle line (y −18.30), 0.5 mm proud | `york-stone.sawn.trodden`, `bronze.brushed.wear` |

The port's threshold (y −24.00 to −23.70 within the opening) belongs to Site's joint: travertine.

### 3.4 Columns: "the court column" (a Gothic-Revival shaft)

The profiles for this column live in `AlbionPlan.h`: `base.roll-hollow-roll`, `abacus.cavetto-roll-square` and
`bell.chalice`. They are named for what they are, and stay private while they are singular.

- **Positions** (stone letters from the Details board):

| | Line 2 | Line 3 | Line 4 | Line 5 | Line 6 |
|---|---|---|---|---|---|
| West row, x −6 | A (−6, −12) | B (−6, −6) | C (−6, 0) | D (−6, 6) | E (−6, 12) |
| East row, x +6 | F (6, −12) | G (6, −6) | H (6, 0) | I (6, 6) | J (6, 12) |

- **Half-columns:** four, on lines 1 and 7 at x ±6, half-round against the end walls' inner faces (y ±18).
- **Base, 0 to 0.355:**
  - A square plinth 0.72 × 0.72 with 0.07 chamfered corners, from 0 to 0.12. The half-columns keep the court side only.
  - A lathe of roll, hollow and roll, as (r, z): (0, 0.12), (0.345, 0.12), (0.345, 0.135), (0.345, 0.16)s,
    (0.33, 0.185)s, (0.30, 0.195), (0.285, 0.20), (0.275, 0.235)s, (0.285, 0.26), (0.29, 0.27)s, (0.30, 0.29)s,
    (0.29, 0.31)s, (0.27, 0.318), (0.262, 0.33), (0.262, 0.355), (0, 0.355).
  - Material: `bath-stone.rubbed.soot-interior`.
- **Shaft:** Ø 0.50, polished, from 0.34 to 5.505 (5.165 tall). Each is one monolith of its own stone:

| | Stone | From | Colour evidence |
|---|---|---|---|
| A | Lizard serpentine | Cornwall | #3E2E27 |
| B | Peterhead granite | Aberdeenshire | #9A6A5D |
| C | Rubislaw granite | Aberdeen | #AEABA5 |
| D | Purbeck marble | Dorset | #4A443B |
| E | Frosterley marble | Co. Durham | #2B2927 |
| F | Hopton Wood stone | Derbyshire | #D6C8AC |
| G | Ashburton marble | Devon | #7E6A62 |
| H | Iona marble | Inner Hebrides | #D8DACF |
| I | Tiree marble | Inner Hebrides | #C49A90 |
| J | Mona marble | Anglesey | #33503A |

  - The colour evidence is sRGB, from `albion_stones_cc0.py:25-36`. Polished roughness is 0.04–0.16.
  - The half-columns' shafts are in the dressing stone.
- **Necking and astragal** (r, z): (0, 5.49), (0.258, 5.49), (0.262, 5.50)s, (0.278, 5.525)s, (0.262, 5.55)s,
  (0.255, 5.56).
- **Bell:** a chalice from 5.56 to 6.08, with r(z) = 0.255 + 0.145·t^1.8, where t = (z − 5.56)/0.52. Lathe it in 12
  stations.
- **Abacus:** square, 0.90 (half 0.45), from 6.075 to 6.20. Its moulded underside is a profile swept round the square:
  - a quarter-circle cavetto from plan half-size 0.36 at 6.075 out to 0.415 at 6.115;
  - a half-round roll of r 0.018 centred at (0.415, 6.133);
  - a fillet to (0.438, 6.151) and (0.45, 6.153);
  - the face up to 6.188;
  - a 0.012 chamfer to the top at 6.20.
- **Capital block:** one block of Bath stone for each capital, 0.90 × 0.90 × 0.70 (5.50 to 6.20). Material:
  `bath-stone.carved.soot-interior`.
- **Carving (Form Carving; polish, §12):** each capital carries the plants the Details board gives it
  (`AlbionColumns.cpp:260-547`).
  - A: fallen oak with acorns (*Mariana*). B: poppies, pansies and violets (*Ophelia*). C: cabbages in nets
    (*The Last of England*). D: ivy (*The Long Engagement*). E: bramble and hawthorn.
  - F: daisy (Morris, 1864). G: trellis roses (Morris, 1864). H: pomegranates (Morris's *Fruit*, 1866).
    I: acanthus (Morris, 1875). J: willow (Morris's *Willow Bough*, 1887).
  - Every design shares, as at Oxford:
    - four corner leaves, 0.44 long, rolling out under the abacus's corners as volutes of about 0.09;
    - four smaller leaves (scale 0.72) under the middle of each face;
    - an upper tier of eight of the capital's own leaf, from 5.76.
  - The half-capitals: north-west oak, north-east daisy, south-west bramble, south-east willow.
  - Leaves are cut as solids at least 0.028 thick, 0.04 for the volutes. The leaf rules are at
    `AlbionColumns.cpp:42-81` and `128-230`, and the ring counts and heights for each design at `295-510`.

### 3.5 Iron and lead

All of this is painted iron (`cast-iron.painted.aged` or `wrought-iron.painted.aged`), a dark slate grey (#434C50). It
is light enough to read against the sky. The iron has no collision, being out of reach, but it casts shadows.

- **Clusters:** 14 in all, one on each column and one on each half-column. The half-columns' clusters are set 0.12
  into the court, at y ±17.88.
  - **Shoe:** a plate 0.44 × 0.44 from 6.198 to 6.23, with four bolts at ±0.16.
  - **Base lathe** (r, z): (0, 6.23), (0.20, 6.23), (0.20, 6.25)s, (0.17, 6.29)s, (0.15, 6.30), (0.15, 6.34), (0, 6.34).
  - **Shaft:** a core of Ø 0.15 and four colonnettes of Ø 0.09 on the diagonals at radius 0.095, from 6.33 to 9.42.
  - **Two annulets**, at 7.45 and 8.65: (0, z−0.03), (0.15, z−0.03), (0.165, z−0.01)s, (0.165, z+0.01)s,
    (0.15, z+0.03), (0, z+0.03).
  - **Cast capital** (r, z): (0, 9.41), (0.15, 9.41), (0.165, 9.44)s, (0.15, 9.47), (0.155, 9.50)s, (0.19, 9.59)s,
    (0.25, 9.66), (0, 9.66). It carries eight cast leaves (polish).
  - **Top plate:** 0.60 × 0.60, from 9.655 to 9.702.
- **Box girders:** one on each arcade line (x ±6), from y −18.02 to 18.02 and z 9.70 to 10.00.
  - Bottom flange 0.70 × 0.018; top flange 0.74 × 0.018.
  - Two webs, 0.012 thick, at ±0.29, with flange angles of 0.06 outstand.
  - Detail (polish): rivets (Ø 0.022 at 0.15 pitch along every angle, both faces), a cast bead (0.018 × 0.024) at
    mid-height, and splice plates 0.50 × 0.014 at each column line with 2 × 5 bolts at 0.09 pitch.
  - The lead valley gutter lies on top: 0.66 wide at 10.004 (`lead.milled.patinated`).
- **Arcade arches:** 12 bays, each a pointed arch in the plane x = ±6.
  - It springs from the capital top (6.20) at ±2.75 from the bay's centre, and its crown (extrados) is at 9.70 under
    the girder: R 3.602, the centres 0.852 beyond the axis.
  - Rib section, 0.16 deep: a web 0.024 thick, an outer flange 0.11 × 0.018, an inner flange 0.09 × 0.018, and a roll
    of Ø 0.044 on the intrados. The soffit is at 9.54 at the crown.
  - A boss of r 0.05 at the crown.
  - **Spandrel rings,** in wrought iron, in each half:
    - a ring centred 2.34 from the bay's centre at z 9.20, R 0.46, tube Ø 0.036;
    - an inner ring R 0.41, tube Ø 0.016;
    - a quatrefoil of four rings, R 0.17, tube Ø 0.018, on the diagonals at 0.19;
    - lugs of 0.04 × 0.044 tying the rings to the arch below and the girder above.
- **The glass arcs** (transverse section, local (x, z)); the glass lies on these arcs:

| Vault | Half | Springs at | Arc centre | R | Crown |
|---|---|---|---|---|---|
| Nave | west | (−6, 10) | (1.0833, 10) | 7.0833 | (0, 17.0) |
| Nave | east | (6, 10) | (−1.0833, 10) | 7.0833 | (0, 17.0) |
| West aisle | west | (−14, 10) | (−8.62, 10) | 5.38 | (−10, 15.2) |
| West aisle | east | (−6, 10) | (−11.38, 10) | 5.38 | (−10, 15.2) |
| East aisle | west | (6, 10) | (11.38, 10) | 5.38 | (10, 15.2) |
| East aisle | east | (14, 10) | (8.62, 10) | 5.38 | (10, 15.2) |

  - Half-arc lengths: nave 10.04, aisle 7.06.
  - The glass runs from y −18.30 to 18.30 (the screens' planes).
- **Main ribs:** on lines 1 to 7, each in six halves (42 half-ribs). Each is a built-up I section whose top is 0.15
  under the glass:
  - outer flange 0.016 thick × 0.12 wide (0.16 on the end lines 1 and 7);
  - web 0.014 × 0.26 deep;
  - inner flange 0.016 × 0.102 (0.136 on the end lines).
  - **Inner arch:** a flat bar 0.03 × 0.018, 0.62 under the glass at the springing. Its depth blends as (s/L)^6 into
    the rib by 94% of the arc.
  - **Ring chain:** rings of R 0.097 and tube Ø 0.018 between the rib and the inner arch, spaced at 0.24 up to 82% of
    the arc.
  - **Shoe:** 0.40 × 0.20 × 0.026, with two bolts at ±0.14.
- **Light ribs:** every 1.5 m between the main lines (18 lines × 6 halves). Each is a tee:
  - table 0.09 × 0.012, 0.15 under the glass;
  - stem 0.012 × 0.14;
  - bead 0.022 × 0.018 on the stem's edge.
- **Side bearers:** along each side wall at the springing, 0.38 into the court and 0.06 high (9.94 to 10.00), full
  length.
  - A cast bracket under the bearer at **every light rib**: a plate 0.016 × 0.29, and a strut from the wall at 9.65 to
    0.33 into the court at 9.93.
  - **Correction:** the old code put the brackets at y −17.25 + 1.5k, midway between the ribs (`AlbionIron.cpp:556`).
    Put them at the light ribs' feet instead.
- **Purlins:** these run the full length (y ±18.28).
  - Nave halves: 5 purlins, one every ~1.8 m of arc. Aisle halves: 3.
  - Section: flange 0.052 × 0.012 at 0.05–0.062 under the glass; web 0.01 from 0.05 to 0.15 under.
  - A ridge bar at each crown: 0.10 wide, from 0.004 to 0.15 under the glass.
- **Glazing bars:** 48 per vault half, at a pitch of 0.747 (36.6/49), following the arc from 0.10 above the springing
  to the crown.
  - Web 0.008 × 0.047 (0.003 to 0.05 under the glass); foot flange 0.032 × 0.01.
- **End screens,** in the planes y ∓18.30:
  - **Nave mullions:** at x −6, −4.8, −3.2, −1.6, 0, 1.6, 3.2, 4.8, 6. The edge mullions are 0.12 wide × 0.20 deep and
    the others 0.07 × 0.12. They rise from 8.50 to the arc (less 0.02).
  - **Glazing bars:** 0.024 × 0.05, at x = −5.4 + 0.8k (15 bars). On the south screen they stop at the band:
    8.50 to 9.00, and 12.60 to the arc.
  - **Transoms:** at 8.50, 9.00, 10.70, 10.90, 12.60 and 14.80. They are 0.07 × 0.10, except the band's transoms on
    the south screen (9.00 to 12.60), which are 0.10 × 0.14. Each stops where the arc comes down to it.
  - **Upper transom:** at 16.00, 0.03 × 0.06.
  - **Aisle gables:** mullions at |x| 6.8, 8.4, 10.0, 11.6 and 13.2 (0.06 × 0.10), from 10.0 to the arc; transoms at
    11.60 and 13.40 (0.06 × 0.08); a cast cill 0.06 high at 10.00.
  - **Nave cill:** on the stone, 0.16 deep, from 8.52 to 8.59.
- **Saddle bars in the lancets:** square bars of 0.016, 0.015 inside the glass line, spanning the light plus 0.03 each
  side, at z 5.45, 5.90, 6.35, 6.80, 7.25, 7.70 and 8.15.
- **Lead ridges:**
  - Rolls of Ø 0.09 on the three crowns (crown + 0.05), from y −18.42 to 18.42.
  - **Aisle crestings:** a plate 0.03 thick from 15.28 to 15.54 (outside ridge 15.6). Rings of R 0.08 every 0.60 are
    polish.
  - **Nave ridge ventilator,** 0.44 wide:
    - cast standards 0.30 × 0.04 × 0.40 (17.08 to 17.48) every 1.5 m;
    - louvre blades, four a side at 0.09 pitch from 17.12, each from x ±0.12 to ±0.17, rising 0.06;
    - a lead cap from x ±0.22 at 17.48 to the ridge at 17.60, closed underneath.
- **Rainwater goods:** 10 downpipes.

| Where | Pipe axis | Head top |
|---|---|---|
| Side walls, beside the buttresses of lines 2, 4 and 6 | y −12.8, −0.8, 11.2; 0.14 off the x ±14.60 faces | 10.55 |
| End walls | x ±6.80 on the y ∓18.60 faces | 10.35 |

  - Pipe Ø 0.104.
  - Head 0.38 × 0.29 × 0.36, with a lip 0.44 × 0.32 × 0.07.
  - A taper from the head to the pipe over 0.14.
  - A shoe of Ø 0.128 from 0.02 to 0.16.
  - Holderbats every 1.80 from 1.20.
  - Material: `cast-iron.painted.aged`.

### 3.6 Glazing (Glazed openings)

| Opening | Where | Glass | Pieces |
|---|---|---|---|
| `albion.vault.nave` | two halves on the nave arcs, from z 10.10 to the crown, y ±18.30 | clear: `soda-lime-glass.laminated-uv-filtering.soiling` | flat panes 0.747 (along y, between bars) × 0.60 (round the arc), each its own plane, each lapping 10 mm over the pane below (17 facets per half) |
| `albion.vault.west` and `albion.vault.east` | four halves on the aisle arcs | the same | 12 facets per half |
| `albion.screen.north` | y −18.30. Nave: from 8.52 to the arc. Aisle gables: from 10.00 to the arcs | clear, the same | panes between the mullions and transoms |
| `albion.screen.south` | y +18.30, as the north screen, with the band inside it | clear round the band | clear panes: 8.52–9.00 (x ±6); 9.00–12.60 at |x| 4.8–6.0; 10.70–10.90 (x ±4.8); 12.60 to the arc; the gables |
| Tristram band (in `albion.screen.south`) | x +4.8 to −4.8 in six columns of 1.60. Upper row 10.90–12.60 (T1–T6); lower row 9.00–10.70 (T7–T12) | `pot-metal-glass.leaded.soiling` with the Art image | 12 panels 1.60 × 1.70, each with a 0.07 quarry border. Read left to right from inside looking south, so T1 is at x 4.8 to 3.2 |
| `albion.lancet.w1` to `w6` | x −14.30 (the glass line); y −15, −9, −3, 3, 9, 15 | `pot-metal-glass.leaded.soiling` | the outline grown 0.01 (1.42 × 4.01) |
| `albion.lancet.e1` to `e6` | x +14.30; the same y | the same | the same |

**Control:** no blinds (D-014). Care is met by the hang and by the UV-filtering interlayer, and the daylight problem is
flagged in §12. **Weather:** see §5.

### 3.7 The porch's ceiling and roof

- **Ceiling:** oak boards at 6.90 over x ±3.0 and y −23.70 to −18.60.
- **Beams:** three moulded oak beams across the porch, at y −22.60, −21.25 and −19.90. Each is 0.20 wide and 0.30 deep
  (6.61 to 6.91), with 12 mm eased arrises.
- **Wall plates:** along the side walls, 0.14 × 0.18 (6.73 to 6.91).
- **Lead flat:** at 7.20 between x ±3.35, from y −24.00 to −18.59.
- Materials: `oak.fumed-french-polished.aged` and `lead.milled.patinated`.

### 3.8 Fittings drawn with the building

- **Picture rail:** a bronze rod Ø 0.022 at z 4.20, 0.035 off the side walls.
  - It runs bay by bay between the pilasters: 0.03 clear of each pilaster's face (0.18 from its centre line), and 0.05
    from the end walls in bays 1 and 6.
  - Ball ends of r 0.016.
  - Brackets every ~1.4 m (at least two a bay): a wall plate 0.06 wide and 0.009 thick from 4.14 to 4.20, and an arm
    0.024 wide reaching 0.035 out, from 4.165 to 4.188.
  - Material: `bronze.brushed.tarnish`.
  - This is Art's hanging system, drawn in Form's language.
- **Benches:** two, at (±3.2, 4.8), in oak, long along y.
  - Top 3.00 × 0.50 at seat height 0.45, 0.065 thick: two boards with a 12 mm rounded arris.
  - Ends: boards on edge 0.38 wide × 0.07, set 0.30 in from each end, from 0.10 to 0.385.
  - Feet: 0.14 × 0.08 at ±0.15 in x, from 0 to 0.10.
  - Cleats under the top.
  - Stretcher: 0.06 × 0.11 at 0.20 high, from y ±1.28, wedged through the ends.
- **Table cases:** four, at (±10.4, −6.0) and (±10.4, 6.0), in oak, 1.00 (x) × 2.20 (y) × 0.95 top.
  - Turned legs from 0 to 0.36, with a square block from 0.36 to 0.54.
    - **Correction:** the old turning profile's points at 0.40 and 0.44 lie above the leg's top
      (`AlbionFurniture.cpp:69-70`). Fit the turning into 0 to 0.36.
  - Stretchers 0.036 × 0.05 at 0.12.
  - An apron 0.14 deep, and a deck from 0.50 to 0.535 with a moulded edge 0.015 proud.
  - A felt bed from 0.535 to 0.55 (the objects lie at 0.552).
  - A glazed box from 0.535 to 0.95, with oak corner posts 0.028 square and top rails 0.028 × 0.024.
  - Case glass: `low-iron-glass.anti-reflective.cleaning-wear`.
  - **No invisible guard.** The old collision-only box to 1.5 m (`AlbionFurniture.cpp:105-107`) breaks
    Form.CollisionMatchesVisual. The case's collision is its own solids.
- **Lectern:** in Morris's bay at (12.2, −15.0), the reader facing east (+x), in oak.
  - Cross feet 0.60 × 0.09 × 0.08 on four pads.
  - A post with stopped chamfers, 0.11 tapering to 0.085 square, from 0.07 to 0.90, with a cap 0.15 square from 0.88
    to 0.92.
  - A desk 0.74 × 0.60, sloping 18°, its far edge at 1.20, with a 0.035 book ledge (`AlbionChaucer.cpp:25-32, 209-307`).

### 3.9 Tolerances and stock

- Each column shaft is one 5.165 m monolith, Ø 0.50. Form.WithinStock needs each stone's largest block from Matter.
  Purbeck and Frosterley monoliths of this length are historically rare. If a stone's stock is shorter, lay the shaft
  in drums at the stone's bed height rather than breaking the rule.
- Each capital is one block of 0.90 × 0.90 × 0.70.
- Glass panes are 0.747 × 0.60, and the stained panels 1.60 × 1.70 in leaded pieces.

### 3.10 Materials: every key

| Where | Key | Family |
|---|---|---|
| Inner ashlar, buff courses, including the porch inside | `bath-stone.honed.soot-interior` | stone |
| Inner red bands | `mansfield-red-sandstone.honed.soot-interior` | stone |
| Inner dressings: pilasters, corbels, cornice, chamfers, reveals, sills, cills, hood mould, label stops, tablets, column bases, half-column shafts | `bath-stone.rubbed.soot-interior` | stone |
| Capitals | `bath-stone.carved.soot-interior` | stone |
| Shafts A–J | `<stone>.polished.touched` | stone |
| Skirting | `welsh-slate.honed.worn` | stone |
| Outer ashlar | `bath-stone.honed.weathered-urban` | stone |
| Outer red bands | `mansfield-red-sandstone.honed.weathered-urban` | stone |
| Outer dressings: plinth, buttress weatherings and base courses, outer sills, copings | `bath-stone.rubbed.weathered-urban` | stone |
| Nave and porch floors and borders | `encaustic-tile.waxed.trodden` | accent (of stone) |
| Aisle flags and the court door's reveal | `york-stone.sawn.trodden` | stone |
| Cast iron | `cast-iron.painted.aged` | accent (of bronze) |
| Wrought iron: rings, inner arches, saddle bars | `wrought-iron.painted.aged` | accent (of bronze) |
| Gutters, ridge rolls, porch flat, ventilator cap | `lead.milled.patinated` | accent (of bronze) |
| Picture rail, rods, the court door's strip, electrolier bodies | `bronze.brushed.tarnish` | bronze |
| Vault and screen glass | `soda-lime-glass.laminated-uv-filtering.soiling` | glass |
| Band and lancets | `pot-metal-glass.leaded.soiling` | glass |
| Case glass | `low-iron-glass.anti-reflective.cleaning-wear` | glass |
| Benches, cases, lectern, porch ceiling, battens, panels | `oak.fumed-french-polished.handled` | oak |
| Case beds | `wool-baize.dyed.faded` | accent (of oak) |
| Frames | `pine.gilt.worn` (gesso over pine; gilt is a working, D-014) | oak, with the gilt working |
| At the port (Site's joint) | `travertine.honed.worn`, `bronze.brushed.wear` | stone, bronze |

The `<stone>` substances for the shafts are `lizard-serpentine`, `peterhead-granite`, `rubislaw-granite`,
`purbeck-marble`, `frosterley-marble`, `hopton-wood-stone`, `ashburton-marble`, `iona-marble`, `tiree-marble` and
`mona-marble`.

**Which records exist in the Matter slot tonight:**
- Substances `low-iron-glass`, `bronze` and `travertine`.
- Workings `stone.honed`, `stone.polished`, `stone.sawn.*`, `laminated.uv-filtering`, `anti-reflective`, `gilt`,
  `brushed` and `painted.steel`. Read `honed` as `stone.honed`, and `laminated-uv-filtering` as `laminated.uv-filtering`.
- Histories `glass.soiling`, `glass.cleaning-wear`, `bronze.tarnish`, `bronze.wear` and `gilt.wear`.

**What is new (a gap for Matter):**
- substances `bath-stone`, `mansfield-red-sandstone`, `welsh-slate`, `york-stone` and `encaustic-tile`, plus the ten
  shaft stones, `cast-iron`, `wrought-iron`, `lead`, `soda-lime-glass`, `pot-metal-glass`, `oak`, `wool-baize` and
  `pine`;
- workings `rubbed`, `carved`, `waxed`, `milled`, `leaded`, `fumed-french-polished` and `painted` for iron;
- histories `soot-interior`, `weathered-urban`, `trodden`, `touched`, `aged`, `patinated` for lead, and `handled`.

**Families (D-014).** Iron and lead are declared accents of the bronze family, the metal family, as brass is. The
encaustic tile is an accent of stone. Matter.OneLanguage must accept these declarations.

**Colour evidence** from the as-built materials (sRGB; `albion_materials.py:86-212`):

| Material | Colour |
|---|---|
| Buff inside | #CFB892 |
| Red inside | #A98272 |
| Dressing inside | #D6C6A6 |
| Carving | #D2C2A2 |
| Buff outside | #C2B196 |
| Red outside | #9A7F70 |
| Dressing outside | #CDC2AB |
| Slate | #3B3F44 |
| York stone | #9A8F7C |
| Oak | #4C3725 |
| Iron | #434C50 |
| Lead | #7E8285 |

---

## 4. Light

### Daylight openings

| Opening | Approximate area |
|---|---|
| The vaults | ≈ 1,768 m² (nave 735, aisles 1,033) |
| The two end screens | ≈ 143 m² each (the south includes the band's 32.6 m² of stained glass) |
| 12 lancets of stained or quarry glass | ≈ 61 m² |

All are Glazed openings, with no daylight control device (D-014).

### Luminaires (catalogue TYPE, count, position)

| Type | Count | Position | Control |
|---|---|---|---|
| `electrolier.6-arm.opal.2700k` (new record: an 1880s electrolier form, turned bronze body, scrolled arms, cased opal globes Ø 0.17) | 12 | One in each arcade bay at (±6.0, −15, −9, −3, 3, 9, 15), hung by a clamp and chain from the arch's soffit at 9.54. Body rim at 5.85; six globes on a 0.505 radius, centres at z ≈ 5.85 | circulation (dusk switching, D-006) |
| `electrolier.4-arm.opal.2700k` (new record) | 1 | Porch, at (0, −21.25), hung by its stem from the middle beam's soffit at 6.61. Body rim at 5.30 | circulation |
| `track-framing.3000k.small` or `.medium` (framing projector on a girder-clamp adaptor) | 33 (one per wall work) | Side-wall works: mount line M1. End-wall works: mounts M2 and M3 | gallery (presence-linked, daylight-linked dimming) |
| `track-spot.3000k.12deg` | 9 (8 case objects and the Chaucer) | Mount line M1, on the object's aisle side | gallery |

Scheme chooses the final types and aims from the targets and mounts below. The counts are the as-built rule of one
luminaire per work. Nothing hangs over the nave's axis.

### Mounts

| Mount | Where | Aim |
|---|---|---|
| **M1** | Continuous clamp under each arcade girder's bottom flange, at x ±6.45, lamp centre z 9.30. Anywhere in y ±17.7, clear of the cluster capitals (±0.35 about each column line) and the electrolier chains (±0.15 about each bay centre) | About 45° down onto the aisle wall; the reflection falls to the floor short of a viewer |
| **M2** | Under the inner flange of the main rib on line 2 (y −12), at z = glass(x) − 0.80 | North-wall works, about 25° off the vertical |
| **M3** | The same on line 6 (y +12) | South-wall works, including the tapestry from (0, 12, 16.2) |

glass(x) is the arc height at x: 17.0 at x = 0, and 15.2 at x = ±10.

### Targets

Targets are illuminance at every reference moment, daylight and electric together.

| Target | Class | Lux |
|---|---|---|
| 26 oil paintings (on canvas, wood or panel) | Care `oil-and-tempera` | 150 recommended, 200 maximum; 600,000 lx·h a year |
| Lady Lilith, Love's Messenger, The Star of Bethlehem, the three wallpapers, the Kelmscott Chaucer | Care `works-on-paper` | 50; 150,000 lx·h a year |
| The Attainment (wool and silk) | Care `textiles.silk` (the most sensitive component) | 50; 15,000 lx·h a year |
| 8 De Morgan ceramics | Care `ceramic-and-glass` | 300 recommended |
| Floors of the nave, aisles and porch | Use `circulation` | 100 maintained (walls 75, ceilings 50); `circulation.night` 50 after dusk |

### Design targets

These are luminance orders between named surfaces, measured at the reference moments and never set by hand.

**By day:**
1. From the gilt sun's eye point:
   - the sky seen through `albion.screen.south`'s clear panes is brighter than the Tristram band;
   - the band is brighter than the stone of the south end wall round the tapestry.
   "The coloured band glowing above it."
2. Every work's face is at least as bright as the banded wall within 0.5 m of its frame. A work never reads darker
   than its wall.
3. The iron soffits are darker than the glass behind them, but not black. The iron reads as grey against the sky.
4. The band's and the lancets' coloured sun patches are the only coloured light in the court.

**By night:**
1. The opal globes are the brightest thing in view.
2. Then the works, which are brighter than their walls.
3. Then the floors.
4. The vault glass is darkest.

### Transport

- The sun through the band and the lancets lays their colours on the floor. This is Light's transport: coloured
  transmission through a stained Glazed opening, as a ray-traced translucent shadow with the glass's transmittance.
- There are no auxiliary lights and no light functions. This replaces the old three RGB directional lights
  (`AlbionSky.h:24-28`, `AlbionSky.cpp:295-329`).
- The glass casts no opaque shadow of its own. Its iron does.

**Removed:** the old night exposure bias and white-balance volume (`AlbionLamps.h:29-30`, `exposure.py:33`). The Eye
alone meters the scene (Visitor.OneEye).

---

## 5. Time and weather

| Opening | Weather observer (D-005) |
|---|---|
| `albion.vault.nave`, `albion.vault.west`, `albion.vault.east` | `london` |
| `albion.screen.north`, `albion.screen.south` (band included) | `london` |
| `albion.lancet.w1` to `w6`, `albion.lancet.e1` to `e6` | `london` |
| `albion.north` (port) | none (a doorway to the Rotunda) |

**The `london` observer:** Westminster, 51.5072 N, 0.1276 W (`AlbionSky.cpp:34`). It is fetched in the one request with
the museum and Giverny. Offline, Time's seeded generator runs on London's climate normals.

**What London's weather does on the glass:**
- Matter's surface law, reading the observer each opening declares:
  - rain beads, and rivulets running down each pane's fall (the arc's slope);
  - the wet sheen, drying slowly after rain;
  - snow lying on the panes that face up (the upper nave and aisle facets);
  - soot along the bars and putty (the `glass.soiling` history).
- Sound's rain on glass and lead (§9).

**Not granted by D-005:**
- *London's cloud dimming the daylight* ("the court goes a stop darker", "the light below turns blue"). Daylight comes
  from the museum's one sky. The old `SunOn` from London's cloud (`AlbionSky.cpp:241-293`) is not carried over.
- *Fog in the vaults* above 10 m (`AlbionSky.cpp:70-89`). A heated court does not fill with outdoor fog, and no
  system owns fog inside an enclosure. It is not built.

**The sun is the museum's, on the idealised model (D-003):**
- a constant altitude of 71.53°, at azimuth 180° + 15°·(h − 12), up from 06:00 to 18:00;
- at noon it stands due south and throws the band's colours onto the nave floor about 3.0–4.2 m in front of the south
  screen (y ≈ 14.1–15.3, x ≈ ±4.8), sliding east and west with the hour;
- in the early morning the east lancets throw patches into the east aisle (x ≈ 11.0–12.3); in the late afternoon the
  west lancets throw them into the west aisle;
- the Sections board's "London noon, 38.5°, 11–16 m from the south wall" is superseded (D-005: the sun and the hour are
  the museum's);
- the vaults admit direct sun onto the aisle walls: from the east in the morning onto the west wall, and from the west
  in the afternoon onto the east wall (see §12).

The Moment's season and hour are the museum's.

---

## 6. Motion

**None tonight.** Albion has no doors and no moving architecture, and its port is an open portal.

**Polish:** the Kelmscott Chaucer's leaf.
- One body, the leaf, with a revolute joint on the spine's axis. It travels 0° to 180° in 1.6 s under the comfort
  limits.
- It is an IMuseeUsable ("Turn the page" and "Turn back", the old `chaucer.next` and `chaucer.prev`), offered within
  2.4 m.
- The next opening's images swap at the end of travel. Clearance to the lectern's ledge is at least 5 mm.
- The curl of real paper is later still (the old curl is at `AlbionChaucer.cpp:112-160`).

---

## 7. Nature

- **Inside:** none. There are no plants, water or ground. The plants of Albion are carved in stone (§3.4) and are Form's.
- **Outside:** the lawn round the court, and the gravel walk that bends round the south end, belong to the Grounds.
  - The Grounds must route its east walk round Albion. The old walk ran through plan y 40–44 (`MuseePlan.h:580-584`),
    which is inside Albion's footprint.
  - The Outside board says: "the walk bends round Albion's south end, 14 m to the front".
- **Polish:** precipitation falling onto the roof glass from the `london` observer (Nature's precipitation reading the
  opening's observer, D-005).

---

## 8. Art

### The hang rule (the Hang board; `albion_hang.py:26-104, 112-187`)

- **Two deep round a 1.6 m line:**
  - row 0 is centred at the board's height for each work (1.645–1.765);
  - large works are raised so the foot of the frame clears about 0.55–0.75;
  - row 1 is centred over the work before it, with 0.18 clear between the frames;
  - the top of the hang is about 3.2–3.6.
- **Spacing:** in each bay, the row-0 frames are spread evenly between the pilasters (bay centre ± 2.85), with equal
  gaps of at least 0.30 and at most 0.90.
- **Side walls:**
  - canvases and panels hang by bronze rods Ø 0.006 from the picture rail at 4.20;
  - the frame's back is on the wall, and the painted face is 0.05 proud;
  - end-wall works are centred on x = −10, 0 or +10.
- **Walk order:**
  - west aisle, "Truth to nature 1848–68", walked south;
  - the south end, "The legend";
  - east aisle, "Beauty 1860–96", walked back north;
  - the north end, "The way out".
- **Frames:**

| Style | Used for | Moulding width | Outer size |
|---|---|---|---|
| ReededGilt | Rossetti, Madox Brown, Moore, Stillman | 0.10 (0.13 if the work is ≥ 1 m) | sight + 2 × 1.02 × width |
| WattsGilt | the others | 0.11 (< 1 m), 0.14 (< 2 m), 0.17 | sight + 2 × 1.0 × width |
| CabinetGilt | works < 0.5 m | 0.07 | sight + 2 × 0.9 × width |
| Paper panel frame | the wallpapers | 0.035 | as listed |

- **Frame details:**
  - Arched and oval sights get a gilt slip in front of the canvas.
  - Paper works are matted and glazed in their frames.
  - **Correction:** Love's Messenger (on paper) was unglazed in the old hang (`albion_hang.py:102`). Glaze it.
- **Supports** follow the medium:
  - panel for oil on wood or panel: Mariana, Pretty Baa-Lambs, The Last of England, Morgan-le-Fay, Medea;
  - stretched canvas for the rest;
  - paper in a mat for works on paper. The old code drew every work as a canvas.

### Wall works: 33 anchors

Anchor positions are the anchor's centre in local metres. A wall anchor's origin is on the wall, facing the room.
Sizes are the real sight sizes, width × height in metres, from `SOURCES.json`.

| Anchor | Work | Medium | Size | Frame | Sight | Glazed | At (x, y, z) | Care |
|---|---|---|---|---|---|---|---|---|
| albion.w1.1 | Millais, *Mariana*, 1851 | oil on wood | 0.495 × 0.597 | WattsGilt 0.11 | rect | no | (−14.00, −15.000, 1.650) | oil |
| albion.w1.2 | Madox Brown, *The Pretty Baa-Lambs*, 1851–59 | oil on panel | 0.762 × 0.610 | ReededGilt 0.10 | rect | no | (−14.00, −15.000, 2.646) | oil |
| albion.w2.1 | Millais, *Ophelia*, 1851–52 | oil on canvas | 1.118 × 0.762 | WattsGilt 0.14 | arched | no | (−14.00, −9.000, 1.650) | oil |
| albion.w2.2 | Madox Brown, *An English Autumn Afternoon*, 1852–53 | oil on canvas | 1.346 × 0.717 | ReededGilt 0.13 | oval | no | (−14.00, −9.000, 2.842) | oil |
| albion.w3.1 | Holman Hunt, *The Awakening Conscience*, 1853 | oil on canvas | 0.559 × 0.762 | WattsGilt 0.11 | arched | no | (−14.00, −3.927, 1.650) | oil |
| albion.w3.2 | Madox Brown, *The Last of England*, 1852–55 | oil on panel | 0.750 × 0.825 | ReededGilt 0.10 | oval (tondo) | no | (−14.00, −2.161, 1.650) | oil |
| albion.w4.1 | Millais, *The Blind Girl*, 1856 | oil on canvas | 0.622 × 0.826 | WattsGilt 0.11 | rect | no | (−14.00, 2.046, 1.655) | oil |
| albion.w4.2 | Wallis, *The Stonebreaker*, 1857 | oil on canvas | 0.787 × 0.654 | WattsGilt 0.11 | rect | no | (−14.00, 3.871, 1.645) | oil |
| albion.w5.1 | Hughes, *The Long Engagement*, 1859 | oil on canvas | 0.533 × 1.070 | WattsGilt 0.14 | arched | no | (−14.00, 8.277, 1.645) | oil |
| albion.w5.2 | Madox Brown, *Walton-on-the-Naze*, 1860 | oil on canvas | 0.419 × 0.317 | CabinetGilt 0.07 | rect | no | (−14.00, 9.856, 1.650) | oil |
| albion.w6.1 | Sandys, *Morgan-le-Fay*, 1864 | oil on panel | 0.437 × 0.618 | WattsGilt 0.11 | rect | no | (−14.00, 14.212, 1.650) | oil |
| albion.w6.2 | Sandys, *Medea*, 1866–68 | oil on panel | 0.456 × 0.612 | WattsGilt 0.11 | rect | no | (−14.00, 15.778, 1.650) | oil |
| albion.e1.1 | Morris, *Trellis*, 1864 (birds by Webb) | block-printed distemper on paper | panel 1.20 × 3.00 (repeat 0.52 × 0.52) | paper panel 0.035 | rect | yes | (14.00, −16.743, 1.700) | paper |
| albion.e1.2 | Morris, *Acanthus*, 1875 | the same | panel 1.20 × 3.00 (repeat 0.66 h × 0.53 w) | paper panel 0.035 | rect | yes | (14.00, −15.000, 1.700) | paper |
| albion.e1.3 | Morris, *Willow Bough*, 1887 | the same | panel 1.20 × 3.00 (repeat 0.436 × 0.514) | paper panel 0.035 | rect | yes | (14.00, −13.258, 1.700) | paper |
| albion.e2.1 | Spartali Stillman, *Love's Messenger*, 1885 | watercolour, tempera, gold on paper | 0.660 × 0.813 | ReededGilt 0.10 | rect | yes | (14.00, −11.088, 1.645) | paper |
| albion.e2.2 | Millais, *Portia*, 1886 | oil on canvas | 0.838 × 1.251 | WattsGilt 0.14 | rect | no | (14.00, −9.767, 1.645) | oil |
| albion.e2.3 | E. De Morgan, *Flora*, 1894 | oil on canvas | 0.880 × 1.990 | WattsGilt 0.14 | rect | no | (14.00, −8.299, 1.745) | oil |
| albion.e2.4 | Leighton, *Lachrymae*, 1894–95 | oil on canvas | 0.629 × 1.575 | WattsGilt 0.14 | rect | no | (14.00, −6.934, 1.690) | oil |
| albion.e3.1 | Burne-Jones, *The Golden Stairs*, 1880 | oil on canvas | 1.168 × 2.692 | WattsGilt 0.17 | rect | no | (14.00, −4.188, 2.095) | oil |
| albion.e3.2 | Moore, *Dreamers*, 1882 | oil on canvas | 1.210 × 0.700 | ReededGilt 0.13 | rect | no | (14.00, −1.796, 1.645) | oil |
| albion.e4.1 | Burne-Jones, *Pygmalion: The Heart Desires*, 1878 | oil on canvas | 0.760 × 0.990 | WattsGilt 0.11 | rect | no | (14.00, 0.996, 1.645) | oil |
| albion.e4.2 | Burne-Jones, *Pygmalion: The Hand Refrains*, 1878 | oil on canvas | 0.760 × 0.990 | WattsGilt 0.11 | rect | no | (14.00, 2.332, 1.645) | oil |
| albion.e4.3 | Burne-Jones, *Pygmalion: The Godhead Fires*, 1878 | oil on canvas | 0.760 × 0.990 | WattsGilt 0.11 | rect | no | (14.00, 3.668, 1.645) | oil |
| albion.e4.4 | Burne-Jones, *Pygmalion: The Soul Attains*, 1878 | oil on canvas | 0.760 × 0.990 | WattsGilt 0.11 | rect | no | (14.00, 5.004, 1.645) | oil |
| albion.e5.1 | Burne-Jones, *The Love Song*, 1868–77 | oil on canvas | 1.559 × 1.143 | WattsGilt 0.14 | rect | no | (14.00, 9.000, 1.650) | oil |
| albion.e6.1 | Rossetti, *Beata Beatrix*, c. 1864–70 | oil on canvas | 0.660 × 0.864 | ReededGilt 0.10 | rect | no | (14.00, 13.411, 1.650) | oil |
| albion.e6.2 | Rossetti, *Lady Lilith*, 1867 | watercolour and gouache on paper | 0.440 × 0.513 | ReededGilt 0.10 | rect | yes | (14.00, 14.994, 1.650) | paper |
| albion.e6.3 | Rossetti, *Proserpine*, 1874 | oil on canvas | 0.610 × 1.251 | ReededGilt 0.13 | rect | no | (14.00, 16.583, 1.645) | oil |
| albion.s.1 | Waterhouse, *The Lady of Shalott*, 1888 | oil on canvas | 2.000 × 1.530 | WattsGilt 0.17 | rect | no | (−10.00, 18.000, 1.765) | oil |
| albion.s.2 | Morris & Co. after Burne-Jones, *The Attainment*, 1895–96 | wool and silk tapestry on a cotton warp | 6.950 × 2.440 | none: on an oak batten with two iron brackets, lined | rect | no | (0.00, 18.000, 2.220) | silk |
| albion.s.3 | Burne-Jones, *King Cophetua and the Beggar Maid*, 1884 | oil on canvas | 1.359 × 2.934 | WattsGilt 0.17 | rect | no | (10.00, 18.000, 2.165) | oil |
| albion.n.1 | Burne-Jones, *The Star of Bethlehem*, 1887–91 | watercolour and gouache on paper | 3.870 × 2.560 | WattsGilt 0.17 | rect | yes | (10.00, −18.000, 2.180) | paper |

**Facing:** the west wall's works face +x (east), the east wall's −x, the south wall's −y and the north wall's +y.

**Captions** (IMuseeCaptioned) come from `data/albion.json`: artist, title, year, medium, collection and notes. The
holding collection and licence come from `SOURCES.json`, which gives accession numbers. Every licence there is public
domain, CC0 or CC BY-SA 3.0 (the Walters rice dish, which must carry its credit).

### Case objects: 8, on 4 CaseBed anchors

The case anchors are `albion.case.w3`, `albion.case.e3`, `albion.case.w5` and `albion.case.e5`. Objects lie on the felt
at z 0.552.

| Case | Object | Real size | Local (x, y) |
|---|---|---|---|
| w3 (−10.4, −6.0) | W. De Morgan, *Charger*, 1882–88, earthenware with red lustre (Met 29.32) | Ø 0.514, h 0.07 | (−10.40, −6.45) |
| w3 | *Dish with an eagle*, c. 1900, lustre on a blue ground (Cleveland 1990.73) | Ø 0.252, h 0.04 | (−10.40, −5.50) |
| e3 (10.4, −6.0) | *Dish with a peacock*, 1888–98 (Birmingham 1981M15) | Ø 0.40 (**unverified**: no size in SOURCES), h 0.055 | (10.40, −6.30) |
| e3 | *Vase with cover*, 1888–98, red lustre (Met 23.163.2ab) | 0.241 w × 0.349 h, turned from the `vessels.json` profile | (10.40, −5.45) |
| w5 (−10.4, 6.0) | *Rice dish*, 1898–1907 (Walters 48.2756) | Ø 0.311, h 0.044 | (−10.40, 5.55) |
| w5 | *Dish with a great fish*, c. 1880 (Chicago 1968.7) | Ø 0.314, h 0.05 | (−10.40, 6.45) |
| e5 (10.4, 6.0) | *Plate with a ship*, c. 1900 (Chicago 1938.358) | Ø 0.20, h 0.03 | (10.40, 5.60) |
| e5 | *Jar with two fish*, c. 1888–97 (Cooper Hewitt 1992-124-2) | 0.24 w × 0.38 h, turned from the `vessels.json` profile | (10.40, 6.40) |

**Care:** `ceramic-and-glass`. The photographed image lies within `vessels.json`'s window for each object.

### The book

- **Anchor:** `albion.lectern`, of kind Plinth, the lectern's desk (§3.8).
- **The work:** Morris, *The Works of Geoffrey Chaucer*, Kelmscott Press, 1896. Printed in black and red in Chaucer type
  on handmade paper, with woodcuts after Burne-Jones (Cleveland Museum of Art, Ingalls Library).
- **Size:** a folio leaf of 0.425 × 0.290. The block is 0.066 thick (277 leaves, 554 pages). The boards are 0.003
  thick with a 0.005 square, in half holland over blue-grey paper boards.
- **Shown:** open at the first opening. There are 40 page scans (`assets/albion/chaucer/page_000` to `page_039`), so 20
  openings are available.
- **Care:** `works-on-paper`.

### Glazing anchors (kind Glazing, D-014)

Art owns each image, Form the opening and Matter the glass.

**The Tristram band** (`albion.band.t1` to `t12`), in `albion.screen.south`:
- Upper row, from x +4.8 westward: T1 *The Birth of Sir Tristram* (Hughes), T2 *The Fight with Sir Marhaus*
  (Rossetti), T3 *The Departure from Ireland* (Prinsep), T4 *The Love Potion* (Rossetti), T5 *The Marriage with
  Isoude les Blanches Mains* (Burne-Jones), T6 *The Madness of Tristram* (Burne-Jones).
- Lower row: T7 *The Attempted Suicide of La Belle Isoude* (Burne-Jones), T8 *The Recognition* (Morris or
  Burne-Jones), T9 *At the Court of King Arthur* (Morris), T10 *King Mark slays Tristram* (Madox Brown), T11 *The Tomb*
  (Burne-Jones), T12 *Queen Guenevere and Isoude les Blanches Mains* (Morris).
- **Source:** each panel's photograph, `assets/albion/glass/tristram-panel-T1…T12`. The originals are Morris,
  Marshall, Faulkner & Co., 1862, from Harden Grange (Bradford Museums).
- **Size:** each original is about 0.68 × 0.605 and is glazed here at 1.60 × 1.70, about 2.5×. T13 stays out.

**The east lancets** (`albion.lancet-glass.e1` to `e6`, north to south): Alceste, Cleopatra, Thisbe, Dido, Ariadne and
Lucrece. They are after Burne-Jones's *Chaucer's Good Women* for Morris & Co., from `goodwomen-*.jpg`.

**The west lancets** (`albion.lancet-glass.w1` to `w6`): Philip Webb's birds and Morris's flowers in diamond quarries,
from `quarries-*.jpg` and `mmf-1860s-*.jpg`.

### Fittings

| Fitting | Anchor | Where |
|---|---|---|
| Two benches | `albion.bench.west` and `albion.bench.east`, kind Bench | (∓3.2, 4.8), §3.8 |
| Four table cases | the CaseBed anchors above | §3.8 |
| The lectern | `albion.lectern` | §3.8 |
| The picture rail and rods | built with the walls | §3.8 |
| The tapestry's batten | with the tapestry | §3.8 |

There are no stanchions or ropes in the design.

### Scans

None. Albion has no sculpture. The capitals are Form carving, not casts.

---

## 9. Sound

**Acoustic character.** This is a hard, tall court of glass and stone. Its RT60 is long, several seconds at 500 Hz, and
is derived by Sound from `albion.court` and Matter's absorption, never tuned. As a check on Sound's sums:

| Surface | Area |
|---|---|
| Volume | ≈ 14,580 m³ |
| Floor | 1,008 m² (tile 432, York stone 576) |
| Inner stone walls | ≈ 1,160 m² |
| Vault glass | ≈ 1,768 m² |
| Screen glass | ≈ 286 m² |
| Lancet glass | ≈ 61 m² |

The porch (≈ 211 m³, stone, tile and an oak ceiling) is shorter and drier.

**Emitters and events:**
- **Rain** on the glass (the vault panes as Form's exposed surfaces of glass) and on the lead (gutters and ridge rolls).
  It runs at the rain rate of the **`london`** observer, which the openings declare (D-005): "Rain runs down the glass
  and drums on the ribs". Wind on the glass is from the same observer.
- **Footfalls** by substance: encaustic tile (hard, waxed ceramic), York stone (sandstone), and travertine (the joint's
  threshold).
- **Nothing else sounds.** There are no mechanisms, and the electroliers are silent.
- **Occlusion** is through the open port to the Rotunda.

---

## 10. Visitor

**Arrival.** From the gilt sun, the visitor looks south through the porch, down 53 m of nave to the Attainment. The
Tristram band glows above it. Nothing stands on the axis.

**The walk:**
1. The porch: encaustic tiles, and Ruskin's words over the court door.
2. The west aisle, walked south.
3. The legend at the south end: *The Lady of Shalott*, the Attainment and *King Cophetua*, with the band above.
4. The east aisle, walked back north.
5. Morris's bay (E1): the wallpapers and the Chaucer.
6. *The Star of Bethlehem* on the way out.

The nave between the arcades is open circulation.

**What the visitor can do:**
- **Sit:** IMuseeUsable ("Sit") on the two benches. Each bench offers two seat frames on each long side, at y ±0.6 from
  its centre, seat height 0.45. They face ±x, so the visitor faces the west or the east aisle.
- **Look and read:** a caption (IMuseeCaptioned) for:
  - every work, case object, band panel and lancet;
  - each column, for example "B · Peterhead granite, Aberdeenshire · the capital: poppies, pansies and violets, from
    *Ophelia*";
  - the two tablets.
- **Use (polish):** the Chaucer: "Turn the page" and "Turn back" within 2.4 m (§6).
- **Pass through:** there are no doors to open. The court door and the port are open.

**Standable places.** Every standable place reaches `albion.north`. The nave, the aisles, the space between cases and
walls (at least 3.0 m) and the space behind the lectern are all walkable, and there are no wedges.

---

## 11. The old code map

The Room Builder re-expresses these numbers through Form, Matter, Light and Art. Nothing is copied. Paths are under
`C:\Users\anan9\Desktop\Projects\Musee Vision\` (read-only).

| Part | File and lines |
|---|---|
| All plan numbers: court, heights, columns, arcade, vaults, lancets, band, porch, doors, buttresses, furniture | `windows/Source/MuseeVision/Albion/AlbionPlan.h:34-118` |
| The Rotunda's door, drum and wall | `windows/Source/MuseeVision/Plan/MuseePlan.h:30-47`; porch envelope `:528`; the Grounds' east walk `:568-586` |
| Material slots, stone table, band rule, openings, vault halves | `Albion/AlbionBuild.h:16-110`; `Albion/AlbionMasonry.cpp:727-784` |
| Pointed-arch and opening maths, wall faces cut by bands, reveals | `Albion/AlbionKit.h:262-309, 318-445` |
| Wall constants (chamfer, glass depth, pilasters, cornice, parapet, coping, plinth, rail) | `Albion/AlbionMasonry.cpp:14-24` |
| Cornice profile | `AlbionMasonry.cpp:134-141` |
| Side walls: lancets, skirting, pilasters, corbels, picture rail | `AlbionMasonry.cpp:145-200` (skirting `94-106`) |
| Hood mould, tablets | `AlbionMasonry.cpp:202-265` |
| End walls: nave stone, cills, court door, Ruskin, ALBION | `AlbionMasonry.cpp:267-332` |
| Aisle cornices, corner corbels | `AlbionMasonry.cpp:334-353` |
| Outer walls, lancets outside | `AlbionMasonry.cpp:357-400` |
| Plinth, buttresses, parapets, copings | `AlbionMasonry.cpp:402-530` |
| Floors of the court | `AlbionMasonry.cpp:534-572` |
| Porch walls, lining (now Site's), ceiling, lead flat, floor | `AlbionMasonry.cpp:576-720` |
| Columns: bell, leaves, flowers, ten carving designs, corner leaves | `Albion/AlbionColumns.cpp:16-547` |
| Column base, capital body and abacus, shaft, placing | `AlbionColumns.cpp:549-695` |
| Iron: clusters | `Albion/AlbionIron.cpp:85-134` |
| Iron: girders and gutters | `AlbionIron.cpp:138-185` |
| Iron: arcade arches and spandrel rings | `AlbionIron.cpp:189-233` |
| Iron: main ribs, light ribs, purlins, glazing bars | `AlbionIron.cpp:237-321` |
| Iron: end screens | `AlbionIron.cpp:325-415` |
| Iron: saddle bars, lead ridges, ventilator, rainwater | `AlbionIron.cpp:419-518` |
| Iron: placing, bearers and brackets | `AlbionIron.cpp:525-570` |
| Glass: vault panes, screens, band atlas order, lancets | `Albion/AlbionGlass.cpp:18-164` |
| Furniture: benches, table cases | `Albion/AlbionFurniture.cpp:26-122` |
| Components, collision, court box | `Albion/AlbionStructure.h:10-93`; `AlbionStructure.cpp:96-137` |
| Electroliers: fittings list, globe height, body | `Albion/AlbionLamps.h:15-33`; `AlbionLamps.cpp:26-62, 65-235` |
| Accent projectors | `AlbionLamps.cpp:255-297`; mounts `windows/Scripts/albion_place.py:52-82`; aiming `windows/Scripts/gallery_lights.py:255-331` |
| London weather: fetch, mapping, the old fog and sun hacks (not carried over) | `Albion/AlbionSky.h:13-31`; `AlbionSky.cpp:34-36, 70-89, 188-329` |
| Work objects: canvas, tapestry, wallpaper, dish, vase | `Albion/AlbionWork.h:9-55`; `AlbionWork.cpp:16-183, 219-355` |
| Kelmscott Chaucer: book, lectern, pages, prompts | `Albion/AlbionChaucer.h:13-27`; `AlbionChaucer.cpp:25-32, 209-307, 327-437` |
| The hang: constants, frames, places, kinds, layout, pottery | `windows/Scripts/albion_hang.py:26-104, 112-187` |
| The laid-out hang | `assets/albion/hang.json` |
| Materials: stone, oak, iron, lead, glass, stained glass | `windows/Scripts/albion_materials.py:86-212, 367-525` |
| Encaustic tiles, border, tablets' letters | `windows/Scripts/albion_textures.py:11-24, 127-300` |
| Column stones' evidence | `windows/Scripts/albion_stones_cc0.py:25-36` |
| Stained glass: Tristram panels and lancets | `windows/Scripts/albion_glass_band.py:18-46`; `windows/Scripts/albion_glass.py:33-40, 255-440`; `assets/albion/glass/` |
| Placards, sizes, licences | `data/albion.json`; `assets/albion/paintings/SOURCES.json`; `assets/albion/vessels.json` |
| The design | `plan/proposals/albion/canvas/{Main,Plan,Sections,Hang,Details,Outside}.dc.html`, `boards/*.png` |

---

## 12. First-pass cut

**The rule:** tonight, build everything that sets the shell, the light, the reach and the hang. Ornament that changes
none of those waits.

### Built tonight

1. **The stage:** the frame, `albion.north` with its class, the footprint, the two enclosures, and every opening (§1,
   §2). Then register Albion.AxisClear.
2. **Masonry by coursing,** inside and out: bands, slate skirting, pilasters and corbels, the aisle cornice (sweep),
   end walls, cills and returns, and the lancet and court-door openings with chamfers, reveals and sills. Then the
   porch's walls and end wall, the plinth (sweep), buttresses, parapets, copings (sweep) and the hood mould (sweep).
   - **The tablets** are plain dressed-stone panels. Their lettering waits.
3. **Floors:**
   - encaustic tiles of 152.4 mm, laid on the diagonal with 2 mm joints and the border;
   - if the Minton roundel inlay is not in Matter's image tonight, lay the tiles by rule in red, buff and black;
   - York stone flags;
   - the court door's reveal and its bronze strip.
4. **Columns:** bases, polished shafts in the ten stones, and the neck, bell and abacus. **The capitals are left
   boasted,** as plain blocks with the bell and abacus. That is true to Oxford, where many capitals stayed uncarved for
   years. The carving waits.
5. **Iron:**
   - clusters without their cast leaves;
   - girders as plain boxes, without rivets or splice plates;
   - arcade arches with their spandrel rings;
   - main ribs with the inner arch and ring chain;
   - light ribs, purlins, glazing bars, bearers and brackets;
   - both screens' mullions and transoms, and the saddle bars;
   - lead gutters, ridge rolls, the porch flat and the plain ventilator cap;
   - downpipes and heads.
6. **Glass:**
   - the vault panes (0.747 × 0.60 with laps) and the screens, in the one clear glass;
   - the Tristram band from the twelve panel photographs, leads included;
   - the east lancets from the Good Women designs, and the west lancets in plain diamond quarries.
7. **The porch:** the oak ceiling with its three beams and wall plates, and the lead flat.
8. **Art:**
   - 33 wall works at true size, with frames (reeded, Watts and cabinet), slips, mats and glazing for paper, and rods
     to the picture rail;
   - the tapestry hung flat and lined on its batten;
   - the three wallpaper panels;
   - four cases, with the six dishes as turned discs from their photographs and the two vases turned from `vessels.json`;
   - the lectern with the Chaucer open, static;
   - two benches with their seat frames;
   - captions.
9. **Light:**
   - daylight through every Glazed opening;
   - the 13 electroliers (the new catalogue records);
   - projectors placed by Scheme on mounts M1 to M3;
   - care and use targets;
   - commissioning at the reference moments.
10. **Time and Matter:** `london` on every Glazed opening, with rain wetness, rivulets and lying snow on the glass by the
    surface law.
11. **Sound:** acoustics derived for the court and porch, footfalls by substance, and rain on glass and lead from `london`.

### Waits for polish

- the ten carved capitals and four half-capitals, and the cluster capitals' cast leaves;
- rivets, bolts, splice plates and the founder's bead;
- the ventilator's louvres and standards, and the aisle cresting rings;
- the label stops;
- V-cut lettering on the Ruskin and ALBION tablets;
- the encaustic roundel inlay (if not in tonight);
- the tapestry's folds;
- the vases' handles;
- the Chaucer's turning leaf (§6);
- the coloured sun patches on the floor, if Light's transport cannot carry coloured transmission tonight;
- precipitation falling past the glass (Nature);
- fine soot and weathering terms;
- the electroliers' scrolled-arm detail;
- better images for the low-resolution works.

The low-resolution sources are: *The Stonebreaker* 703 px, *The Golden Stairs* 850 px, *Proserpine* 1,041 px,
*King Cophetua* 1,054 px, *Portia* 1,268 px and *Acanthus* 1,328 px (`SOURCES.json` pixel_w).

### Open for the Architect (these block checks tonight)

1. **Daylight against care.**
   - The clear vaults over the aisle walls admit diffuse light far above 200 lx.
   - They also admit **direct sun onto the walls** of works: the idealised sun stands at 71.5°. From the east, the sun
     through the west aisle's glass strikes the west wall from the floor to about 3.2 m.
   - The paper works, the silk tapestry and the Chaucer are held to 50 lx under a glass roof.
   - Art.WithinCare will fail, and a failing check stops packaging.
   - D-014 allows no automatic blinds. Physically real options, for a decision:
     - (a) The vaults glazed in rolled plate glass that diffuses the light. This was the historical Victorian roof
       glass, Hartley's patent of 1847. It does not lower the level.
     - (b) A fixed linen velarium under the aisle vaults, a daylight-control device that does not move.
     - (c) The paper and textile works moved into cases, or out of the daylit court.
     - (d) Glass of a lower measured transmittance.
   - Until then, the builder declares the true targets and records the failures as a gap. Nothing is tuned.
2. **The port's collar.** The joint must close the porch's wedges against the drum, which needs x ±3.65 by 7.50 high
   (§2). `doorway.4000x4500.arched` must carry that envelope. Its key and face must agree with the Rotunda's sheet.
3. **The interior doorway.** The court door is one opening shared by two enclosures of one stage. It needs an opening
   kind, or the fallback of one enclosure.
4. **True size of the stained glass.** The Tristram band is glazed about 2.5× the size of the Bradford panels, as the
   approved design states. Art.TrueSize needs Albion's window recorded as its own work, "after" the 1862 panels, or an
   exception.
5. **The Grounds' east walk** crosses Albion's footprint (old plan y 40–44). The Grounds must bend it round the south
   end.
