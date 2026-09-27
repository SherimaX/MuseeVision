# Hall of Light · stage sheet

Stage key `hall-of-light`. Module `MuseeHallOfLight` in `Source/Stages/Rooms/HallOfLight` (one class,
`UHallOfLightRoom : USiteRoom`). This is the Drafter's sheet for the first pass (D-016). It ports the as-built hall
(D-001, D-017): the old plan numbers are the source. The only changes are the few the stage model forces, and each
of them is listed in 1.6.

**Conventions.**
- **Units.** Metres and degrees.
- **Frame.** The stage's local frame runs x east, y south, z up, on true north.
- **Plan.** "Plan" means the museum plan: the Rotunda's centre is the origin, with x east and y south. Here
  local x = plan x − 11.20, local y = plan y, and z is the height above the Rotunda's finished floor.
- **Depth d.** Long members along the glass walls are given in (d, z). d is the depth measured in from the glass
  line towards the axis. A member on the north wall lies at y = −(4.50 − d), and on the south wall at
  y = +(4.50 − d).
- **Vault angle θ.** θ is measured from the crown, positive to the south. A point at vault angle θ and radius ρ is
  (y, z) = (ρ sin θ, 2.20 + ρ cos θ).

**Sources.**
- `windows/Source/MuseeVision/Plan/MuseePlan.h`.
- `windows/Source/MuseeVision/HallOfLight/*`.
- `Shared/Wings/HallOfLight.swift`.
- `windows/Scripts/{nature,frames,gallery_lights,materials,native}.py`.
- `plan/canvas/HallOfLight.dc.html` (the board) and `plan/README.md` (lines 107–111).
- The holding museums' records, for the sizes of the works (section 8).

Section 11 gives every file and line range.

---

## 1. Identity and placement

### 1.1 What it is
*Modern · Alive.* A glass hall 9 m wide runs east from the Rotunda's drum to the Atrium's stone base: 28.0 m on the
axis, and 29.6 m along the glass. It stands between two gardens: a birch meadow to the north and an orchard of
cherries and apples to the south.

Glass walls stand 5 m high on a honed travertine plinth. Over them is a shallow vault of pearl-white steel and glass
(R 5.30 about z 2.20, crown 7.50), the Rotunda's lattice drawn out along the hall.

The hang:
- Twenty photographs from 1840 to 1914, one per 3 m bay on each wall, run forward in time as you walk east.
- Bays 9–10 hold the four autochromes, set against the glass.
- Four stereo stones stand off the axis, and five flush viewing stones lie on it.

### 1.2 Origin and level
- **Origin O.** On the hall's axis, on the finished floor, on the outer face of the Rotunda's drum. This is the
  threshold point of the west port, at plan **(11.20, 0.00)**.
- **Level.** Ground floor, z 0.00. This is the Rotunda's floor, which is also the sun clock and the Atrium's floor.
- **Neighbours' centres** (used only to shape the port faces; the stage never refers to another stage):
  - Rotunda: local (−11.20, 0);
  - Atrium: local (42.80, 0), which is plan (54.00, 0).

### 1.3 Datums

| Datum | Value |
|---|---|
| Glass line | \|y\| = 4.50 |
| Plinth top | 0.30 |
| Transom | 3.20 (centre) |
| Eaves box | 4.86 … 5.10 |
| Wall glass head | 4.85 |
| Vault | R 5.30 about (y 0, z 2.20). Springing on the glass line at θ ±58.11° (z 5.00). Crown 7.50. The glass and the arches bear on the eaves' top at θ ±56.83° (z 5.10). |
| Purlin angles θ | 0°, ±14.73°, ±29.46°, ±44.19° |
| Eaves lines θ | ±56.83° (arches, glass) and ±58.11° (as-built node line) |

Seven purlins and the two eaves make 8 panels across (board: "eight panels across with a cross in each"). The as-built
rule for the purlins: θ ±44.19° is where the vault crosses the Atrium's coping at z 6.00, and the others divide that
angle equally (HallOfLightStructure.cpp:93-104).

**Rhythms** (local x):

| Element | Positions (local x) | Plan x |
|---|---|---|
| Posts | −0.20 + 3k, k 0…9: −0.20 … 26.80 | 11.0 … 38.0 |
| Arches | 1.30 + 1.5k, k 0…17: 1.30 … 26.80 | 12.5 … 38.0 |
| Print bays 1–9 | 1.30 + 3(b−1) | 12.5 … 36.5 |
| Print bay 10 | 27.75 | 38.95 |
| Viewing stones | 2.80, 8.80, 14.80, 20.80, 26.80 (on post lines) | 14, 20, 26, 32, 38 |
| Stereo stones | (5.80, −2.40), (11.80, +2.40), (17.80, −2.40), (23.80, +2.40) (on post lines, each under an arch) | |

The odd arches (2.80, 5.80, …) stand over posts. The even arches (1.30, 4.30, …) stand over print bays 1–9.

### 1.4 Footprint (kind **Building**)

The plan polygon is given in local metres, clockwise from the north-west. Arcs are vertical cylinders; Site takes
Form's shared stations on them.

| # | Vertex or arc | Local (x, y) |
|---|---|---|
| F1 | NW garden corner | (−0.300, −14.250) |
| F2 | NE garden corner | (29.900, −14.250) |
| F3 | | (29.900, −8.227) |
| – | arc R 15.300 about (42.800, 0) (the Atrium's socle) | to F4 |
| F4 | | (28.747, −6.050) |
| F5 | front face of the Élan frontispiece (Atrium's), plan x 39.0 | (27.800, −6.050) |
| F6 | | (27.800, −4.800) |
| F7 | | (28.800, −4.800) |
| – | arc R 14.800 about (42.800, 0) (the east port face), through (28.000, 0) | to F8 |
| F8 | | (28.800, 4.800) |
| F9 … F13 | mirror of F6 … F2 | (27.800, 4.800), (27.800, 6.050), (28.747, 6.050), arc R 15.300, (29.900, 8.227), (29.900, 14.250) |
| F14 | SW garden corner | (−0.300, 14.250) |
| F15 | | (−0.300, 4.800) |
| F16 | | (−1.081, 4.800) |
| – | arc R 11.200 about (−11.200, 0) (the west port face), through (0, 0) | to F17 |
| F17 | | (−1.081, −4.800) |
| F18 | | (−0.300, −4.800) |

**Footprint volume.** Site judges by volume.
- **Body band** (\|y\| ≤ 4.80, between the two port arcs): z −0.12 … 7.62.
- **Gardens:** z −1.00 … 12.00.

The body passes through the opening of the Atrium's frontispiece with the as-built clearance:
- the frontispiece's clear opening is \|y\| < 4.95 to z 5.22, then an arc of R 5.80 about z 2.20;
- the body's outline is \|y\| ≤ 4.80 (kerb) to z 0.06, \|y\| ≤ 4.68 (plinth) to z 0.30, \|y\| ≤ 4.60 up to the
  eaves' top at z 5.10, and R 5.42 above that.

So the two volumes never touch.

This outline refines the Grounds sheet's "old lawn exclusion" (plan x 10.9 … 41.1, \|y\| ≤ 14.25). The bounding box
is the same, less the Rotunda's drum and the Atrium's socle and frontispiece.

### 1.5 Enclosures

| Key | Kind | Bounded by | Openings |
|---|---|---|---|
| `hall-of-light.hall` | **Enclosed** | floor, plinths, glass walls, vault glass, west lining, east lining with its lunette | Port `hall-of-light.west`, Port `hall-of-light.east`, Glazed `hall-of-light.wall.north` and `.wall.south` (11 panes each), Glazed `hall-of-light.vault`, Glazed `hall-of-light.lunette`, and 20 Housings in the plinths (`hall-of-light.plinth.north.h0…h9`, `.south.h0…h9`) |
| `hall-of-light.meadow` | **Outdoors** | north garden: F1–F6, F18, the kerb line y −4.80 | none |
| `hall-of-light.orchard` | **Outdoors** | south garden, the mirror | none |

The hall's volume is about 1,700 m³ (section about 60.9 m²; 27.9 m on the axis, 29.5 m along the glass). Form's made
fact decides the exact value.

### 1.6 Deltas from the as-built, forced by the stage model

These need the Architect's nod under D-017.

1. **End linings.** As built, the glass hall had no end walls: its glass, steel and stone ran 1.4–5.5 cm *into* the
   Rotunda's drum and the Atrium's base. A stage must be sealed alone and must build nothing past its port faces,
   so the hall gains two **50 mm honed travertine linings**. Each lies against its neighbour's face (concave
   cylinders R 11.20 → 11.25, and R 14.80 → 14.85) and closes the hall's section except at the door. Seen from
   inside, a stone face with a doorway remains a stone face with a doorway. Every as-built x position holds. Only
   the collars and end posts move 5 cm.
2. **The 19th arch is dropped** (plan x 39.5). Above z ≈ 6.3 it lay beyond r 14.80, over the Atrium's coping,
   which is the Atrium's volume. Now there are 18 arches (plan 12.5 … 38.0) and 17 crossed bays.
3. **The east lunette.** As built, the vault ran over the Atrium's coping to its misty drum glass (r 14.0). Now the
   east lining is stone to z 6.20, and the vault's section above it is a **glazed lunette** (6 flat panes). The
   misty drum glass is still seen through it, 0.85 m beyond.
4. **Joints, not embeds.** Wherever the old code ran one solid 1–5 cm into another, pieces now meet with real
   joints (touching faces, butt or welded). The places:
   - glass into the plinth and the eaves;
   - posts into the plinth;
   - transoms and purlins through webs;
   - kerb into the plinth;
   - floor under the plinth;
   - the crossing bars.

   Form.NoOverlap holds. Each is listed at its piece in section 3.
5. **Works at true size** (Art.TrueSize). The as-built prints were scaled up to fill 0.78 × 0.68 m. They now hang
   at their recorded sizes on the unchanged 1.0 × 0.9 m mounts (section 8). Talbot's leaf is 8 × 9.7 cm.

---

## 2. Ports

| Key | Kind | Class | Frame origin (local) | Facing (out) | Wall depth | Face shape | Mates | Passage | Closure |
|---|---|---|---|---|---|---|---|---|---|
| `hall-of-light.west` | Doorway | `doorway.3000x4500.arched` | (0.000, 0.000, 0.000) | (−1, 0, 0) west | 0.050 (the lining) | cylindrical, **concave**, R 11.20, vertical axis through (−11.20, 0) | `rotunda.east` | 0 m | open portal |
| `hall-of-light.east` | Doorway | `doorway.4000x4500.square` | (28.000, 0.000, 0.000) | (+1, 0, 0) east | 0.050 | cylindrical, **concave**, R 14.80, vertical axis through (42.80, 0) | `atrium.west` | 0 m | open portal |

**Classes.** These are proposals to Site; `Data/Site/PortClasses.json` governs.
- **`doorway.3000x4500.arched`.** This is the Rotunda sheet's class:
  - clear 3.00 wide;
  - springing 4.50, with a semicircular head (crown 6.00);
  - allowance 0.050 at the jambs and the head (rough 3.10 wide, head R 1.55);
  - rough sill at −0.05 (the joint's 50 mm threshold stone);
  - open portal.
  It is the as-built Rotunda west and east door (MuseePlan.h:44-47).
- **`doorway.4000x4500.square`.** This is the as-built Atrium west door (MuseePlan.h:171,
  AtriumBaseStructure.cpp:66-67):
  - clear 4.00 wide and 4.50 high, with a square head;
  - allowance and sill proposed as for the arched class (rough 4.10 × 4.55, sill −0.05);
  - open portal.
  The Atrium's sheet must declare the same class.
- **The rough-opening rule.** Each lining builds only the rough opening of its class, read from the class record and
  never restated here.

**D-012 topology.**
- The Hall is a link in the chain `rotunda.east ↔ hall-of-light.west` and `hall-of-light.east ↔ atrium.west`, both
  with passage 0 m.
- It has no vertical ports and no wells, so there is no serving or receiving side.
- It has no gates to the gardens: they are seen through the glass and never entered, as built.
- Mate places the stage at translation (11.20, 0, 0) from the anchor. The Atrium's centre then lands at plan
  (54.00, 0), which Site.MatchesCanvas checks.

**The joints** (Site's, drawn once per class):
- the travertine threshold through both wall depths:
  - west: the lining 0.05 plus the Rotunda's drum 1.20;
  - east: the lining 0.05 plus the Atrium's base 0.80;
- the brushed-bronze strip on the mating surface, which here is the cylinder R 11.20 or R 14.80 across the
  threshold;
- linings in both rough openings;
- surrounds on both faces, in travertine per the one doorway detail. As built, the hall's faces had a plaster
  two-fascia architrave. Its profile is given in 3.6 as evidence for Site's surround.
- No passage masonry: the faces touch.

**Coordination, for the neighbours' sheets.**
- **Rotunda.** `rotunda.east` is declared on r 11.20 (the Rotunda sheet agrees). The drum's façade dress (skin
  11.20 → 11.40) may run over the hall only above z 7.62 within \|y\| ≤ 4.80, as built ("over the Hall of Light,
  the drum's skin starts clear of its vault", MuseeFacadeStructure.cpp:77-80).
- **Atrium, the port.** `atrium.west` must sit on the base's outer face, r 14.80 about the Atrium's centre.
- **Atrium, the plinth mouldings.** They (socle to r 15.30, corona to r 15.40) must not enter \|y\| ≤ 4.80 west of
  r 15.30. As built they were cut back ±22° about west, where the frontispiece takes them.
- **Atrium, the frontispiece.** The travertine frontispiece is the Atrium's (as the Grounds sheet expects). It is
  12.10 wide, 9.50 high, with its front at plan x 39.0, and its opening is the hall's section with room to spare
  (ElanExteriorStructure.cpp:413-552). Its volume must keep the clear opening given in 1.4.
- **Grounds.** It receives this footprint and lays nothing inside it.

---

## 3. Pieces by trade

**Material keys** follow the Rotunda sheet: `substance.working.history`, with its short workings and history
presets. The presets are:
- `as-laid`: hidden;
- `aged`: indoors;
- `worn`: underfoot;
- `handled`: touched;
- `weathered`: outdoors.

This sheet adds the following workings and substances:

| Short working | Matter record | State |
|---|---|---|
| `laminated-uv` | `laminated.uv-filtering` | the glazing that holds the UV limit (D-014) |
| `patinated` | `patinated` | |
| `fumed` | *(missing)* | oak |
| `rag-board` | *(missing)* | paper |
| `annealed` | `annealed` | opal glass |

**Missing records:** the substances `oak`, `paper` and `opal-glass`, and the pearl paint colour (see 3.2).

### 3.1 Masonry (travertine)

| Piece | Key | Geometry | Joints and stock |
|---|---|---|---|
| **Floor** `hall-of-light.floor` | `travertine.polished.worn` | Top z 0.000, bottom −0.100. Between the linings' inner faces (R 11.25 about (−11.20, 0); R 14.85 about (42.80, 0)) and the plinths' inner faces, \|y\| ≤ 4.32. Five circular cut-outs of R 0.450 at the viewing stones. | Laid by Form's `Lay()` within travertine stock. Slabs are 1.20 across (y) × 0.80 along (x), in a grid with no stagger, with a joint on the axis and joints of 2.5 mm (as-built M_Travertine_Polished: FloorSlab 0.8, FloorAspect 1.5, FloorStagger 0, JointWidth 0.0025). They are cut to the linings' arcs. As built the floor ran 2 cm under the plinths; now it butts them. |
| **Viewing stones** ×5 `hall-of-light.viewing-stone.1…5` | `travertine.honed.worn` | On the axis at x 2.80, 8.80, 14.80, 20.80, 26.80. A rebated disc: R 0.450 from −0.100 to −0.030, and R 0.400 from −0.030 to 0.000. Flush. | One piece each. The bronze ring (3.4) sits in the rebate. |
| **Plinths** ×2 `hall-of-light.plinth.north` and `.south` | `travertine.honed.aged` | Section (d, z), closed: (−0.18, −0.10) (0.18, −0.10) (0.18, 0.29) (0.17, 0.30) (−0.17, 0.30) (−0.18, 0.29). This is 0.36 wide on the glass line with 10 mm chamfers at both top arrises. It runs from the west lining's inner face to the east lining's, trimmed to their cylinders. | Ashlar blocks 0.40 × 0.36 × up to 1.80 long (as-built BlockLength 1.8). Ten Housing pockets per plinth for the path lights, at each post x: x ±0.050, z 0.110–0.170, d from 0.180 back to 0.060. The shell is unbroken, since the glass line is at d 0. |
| **Kerbs** (planter edges) ×2 `hall-of-light.kerb.north` and `.south` | `travertine.honed.weathered` | Section (d, z): (−0.30, −0.12) (−0.18, −0.12) (−0.18, 0.06) (−0.29, 0.06) (−0.30, 0.05). This is \|y\| 4.68 … 4.80, with a 10 mm chamfer on the outer top arris. It runs lining to lining. | Butts the plinth's outer face at d −0.18. As built it was −0.17, 1 cm into the plinth. Blocks up to 1.80. |
| **West lining** `hall-of-light.lining.west` | `travertine.honed.aged` | Between the cylinders R 11.200 (outer, the port face) and R 11.250 (inner) about (−11.200, 0). The outline in (y, z), both sides mirrored: (±4.800, −0.120) → (±4.800, 0.060) → (±4.680, 0.060) → (±4.680, 0.300) → (±4.600, 0.300) → (±4.600, 5.100) → (±4.579, 5.100) → an arc R 5.420 about (0, 2.200) to the crown (0, 7.620). The outline covers the ends of the kerb, plinth, post caps, eaves and vault. Less the rough opening of `doorway.3000x4500.arched` (rough sill −0.050). | Cladding courses 0.60 high, slabs up to 1.80 long (as-built M_Travertine_Honed: CourseHeight 0.6, BlockLength 1.8). Every hall piece that meets the west end bears on its inner face. |
| **East lining** `hall-of-light.lining.east` | `travertine.honed.aged` | Between R 14.800 (outer, the port face) and R 14.850 (inner) about (42.800, 0), with the same outline. Less the rough opening of `doorway.4000x4500.square`. Less the lunette opening {z ≥ 6.200, r ≤ 5.190 about (0, 2.200)}. Above z 6.20 the stone remains only as a band between r 5.19 and the outline (r 5.42). | As the west lining. The lunette's sill is the stone's cut at z 6.200, clear of the Atrium's coping top (6.05–6.20 as built). |
| **Stereo stones** ×4 `hall-of-light.stereo-stone.1…4` | `travertine.honed.handled` | A lathe profile (r, z): (0, 0) (0.350, 0) (0.350, 0.985) (0.335, 1.000) (0, 1.000). That is Ø 0.70 × 1.00 with a 15 mm top chamfer. Centres as in 1.3. They stand on the floor. As built they started 1 cm below it. | One block each. |

### 3.2 Steelwork (pearl-white painted steel)

Every steel piece is `steel.painted.aged`, with the topcoat in **pearl white #ECE9E3** (the as-built M_RibPearl:
base colour 0xECE9E3, roughness 0.32; native.py:161-176). Matter's `painted.steel` record carries only the eye's dark
grey today, so it needs a pearl topcoat. By the principles, pearl steel belongs to the white family's future finish.

The steel is:

| Piece | Section (points closed) | Path and extent |
|---|---|---|
| **Posts** ×20 `hall-of-light.post.n0…n9`, `.s0…s9` | An I-section in (x relative to the post, d): (−0.030, −0.030) (0.030, −0.030) (0.030, −0.010) (0.010, −0.010) (0.010, 0.140) (0.030, 0.140) (0.030, 0.155) (−0.030, 0.155) (−0.030, 0.140) (−0.010, 0.140) (−0.010, −0.010) (−0.030, −0.010). The pressure cap is outside the glass, the web runs through the glass line, and the flange is inside. | Vertical at the post x, z 0.300 → 4.860. It bears on the plinth and meets the eaves' underside (as built 0.28 → 4.90, into both). |
| **Glazing shoes** ×2 | A U-channel in (d, z): (−0.025, 0.300) (0.025, 0.300) (0.025, 0.340) (0.010, 0.340) (0.010, 0.310) (−0.010, 0.310) (−0.010, 0.340) (−0.025, 0.340) | Lining to lining on the plinth top. The glass stands in the slot on setting blocks at 0.310. It is broken by the post webs. |
| **Transoms** ×2 | A rectangle d 0.012 … 0.110, z 3.170 … 3.230 | Inside the glass, in segments between post webs (and lining to the first and last post). As built, one bar passed "through" the webs. |
| **Eaves** ×2 | (d, z): (−0.10, 4.86) (0.20, 4.86) (0.20, 5.05) (0.15, 5.10) (−0.10, 5.10). A box eave 0.30 × 0.24 with a 50 mm chamfer on the inner top arris. | Lining to lining. The wall glass is held in a channel in its underside. The vault glass and the arches bear on its top. |
| **End posts** ×4 | A rectangle: along x 0.000 … 0.070 from the lining's inner face, d −0.040 … 0.120 | z 0.300 → 4.860, fixed to the lining. They hold the glass's end edge. |
| **Arches** ×18 `hall-of-light.arch.0…17` | An I-section in (x relative to the arch, b = radial out from R 5.30): (−0.035, −0.150) (0.035, −0.150) (0.035, −0.130) (0.012, −0.130) (0.012, 0.025) (0.035, 0.025) (0.035, 0.045) (−0.035, 0.045) (−0.035, 0.025) (−0.012, 0.025) (−0.012, −0.130) (−0.035, −0.130). The inner flange is under the glass, the web runs through it, and a cap sits over it. | Swept about (y 0, z 2.20) from θ −56.83° to +56.83°, with the feet cut level at z 5.100 on the eaves' top. There is none at plan 39.5 (1.6). |
| **Purlins** ×7 lines | A rectangle (across ±0.030, b −0.120 … +0.015) | At θ 0°, ±14.73°, ±29.46°, ±44.19°, in segments: from each end collar to the first or last arch, and between arch webs. As built they ran drum to drum through the webs. |
| **Crosses** 17 bays × 8 panels = 136 | Bar A: a rectangle (±0.015, b −0.080 … −0.020). Bar B: (±0.015, b −0.090 … −0.030). | In each panel between adjacent arches (1.30 … 26.80) and adjacent purlin or eaves lines (the eaves line at θ ±56.83°), a diagonal from node to node on the vault surface, trimmed 0.025 short of each node. A runs through; B is cut at the crossing and butts A on both sides, as a welded joint (D-014). As built the bars interpenetrated by 5 cm. The end bays have no crosses, as built. |
| **Collars** ×2 | A rectangle: along x 0.000 … 0.080 in from the lining face, b −0.110 … −0.012 | Swept along the vault from θ −56.83° to +56.83°, fixed to each lining. The vault glass's end edge rests on it. There is one section at the east now (as built it changed section at the coping). |
| **Lunette mullions** ×5 | A T-section: along x 0.000 … 0.100 in from the east lining face, across ±0.030 | Vertical at y 0.000, ±1.348, ±2.607 (the purlins' y), from the sill at z 6.200 to the collar's underside. |

### 3.3 Glazing (`low-iron-glass.laminated-uv.aged`)

All the glazing is laminated low-iron glass, 8 + 1.52 PVB (UV) + 8 = 17.52 mm, heat-strengthened. Matter's stock
rules govern the pane sizes. The outer faces take the surface law outdoors (wetness and soiling by sky exposure).

| Opening | Panes |
|---|---|
| `hall-of-light.wall.north` and `.south` (Glazed) | Centred on the glass line (d −0.0088 … +0.0088), z 0.310 → 4.850. One pane per post bay, 11 per wall: the west end bay (end post to post 0, about 0.60 wide), nine bays of 2.970 (3.000 less the web and 5 mm gaskets), and the east end bay (post 9 to end post, about 1.76). A 2.97 × 4.54 pane is within jumbo float (3.21 × 6.00). The transom bears on the inner face through a gasket. |
| `hall-of-light.vault` (Glazed) | Hot-bent to R 5.30 (centred), one pane per lattice panel: 8 across × 19 along (17 crossed bays and the 2 end bays) = 152 panes. The panes are 1.466 along x, and 1.169 (eaves to ±44.19°) or 1.363 (the others) around the arc. The end-bay panes are cut to the lining's cylinder. |
| `hall-of-light.lunette` (Glazed) | 6 flat panes in the east lining's inner face, faceted to R 14.85 (chord deviation ≤ 15 mm), between the mullions, the sill at z 6.200 and the collar. |
| Autochrome cassettes | Art's Glazing anchors against the wall glass in bays 9–10 (section 8). The wall glass is continuous behind them. |

### 3.4 Bronze

| Piece | Key | Geometry |
|---|---|---|
| Viewing-stone rings ×5 | `bronze.cast.worn` | Annulus R 0.400–0.450, z −0.030 … 0.000, flush, in the disc's rebate |
| Print stems ×32 (2 per print, bays 1–8, both walls) | `bronze.patinated.aged` | A flat bar (x ±0.015, d 0.098 … 0.120), z 0.320 → 2.000, at bay x ± 0.300. The mount's back (d 0.120) bears on them. As built there was a 1 cm air gap: do not copy it. |
| Stem shoes ×32 | `bronze.patinated.aged` | (x ±0.035, d 0.080 … 0.128), z 0.300 … 0.320, on the plinth top (as built 2 cm into it) |
| Autochrome glazing bars ×8 (2 per autochrome) | `bronze.patinated.aged` | (x ±0.0125, d 0.035 … 0.055) at bay x ± 0.560, z 0.310 → 3.170 (shoe to transom) |
| Stereo reader stems ×4 | `bronze.patinated.handled` | A rod Ø 0.030 from the stone top (1.000) to the reader plate's back, 0.016 behind the stone's centre on the reader's back side |

The path lights' bronze louvre faces belong to the luminaire (Light's catalogue record), not the stage.

### 3.5 Art's objects at this stage's anchors
Art draws these in Form's language, and they are listed here for fit (section 8 gives the rules):
- the print mounts (1.000 × 0.900 × 0.030);
- the frames (dark oak, 22 mm);
- the autochrome cassettes;
- the stereo readers: a bronze plate 0.680 × 0.400 × 0.030, `bronze.patinated.handled`.

### 3.6 Mouldings, stairs, orders
- **Stairs, domes, niches:** none. **Columns and pilasters:** none. The posts are steel; there is no order.
- **The stage's own mouldings:** only the arris chamfers above (plinth 10 mm, kerb 10 mm, eaves 50 mm, stereo drum
  15 mm). These are singular to this stage, so they stay private (the promotion rule).
- **The doorcase architrave** belongs to Site's joint and is evidence only. It is the as-built two-fascia architrave
  round both doors, given as (s out from the opening's edge, p proud of the wall), closed through the wall
  (HallOfLightStructure.cpp:644-652): (0.290, −0.015) (0.012, −0.015) (0.012, 0.024) (0.100, 0.030) (0.100, 0.040)
  (0.190, 0.046) (0.190, 0.054) (0.205, 0.054). Then an ovolo of R 0.040 about (0.245, 0.054) through (0.2064, 0.0644)
  (0.2104, 0.0740) (0.2167, 0.0823) (0.2250, 0.0886) (0.2346, 0.0926) (0.2450, 0.0940). Then (0.290, 0.094). It is
  0.278 wide and 0.094 proud. As built it ran up the jambs and round the arch at the west, and was mitred
  square-headed at the east. The one doorway detail makes it travertine.

### 3.7 Finish schedule (seeds `Data/Stages/HallOfLight/Finishes.json`)

| Part | Key | As-built material |
|---|---|---|
| Floor slabs | `travertine.polished.worn` | M_Travertine_Polished |
| Viewing-stone discs | `travertine.honed.worn` | M_Travertine_Honed |
| Plinths, linings | `travertine.honed.aged` | M_Travertine_Honed |
| Kerbs | `travertine.honed.weathered` | M_Travertine_Honed |
| Stereo stones | `travertine.honed.handled` | M_Travertine_Honed |
| All structural steel | `steel.painted.aged`, pearl #ECE9E3 | M_RibPearl |
| Wall, vault and lunette glass | `low-iron-glass.laminated-uv.aged` | M_Glass |
| Viewing-stone rings | `bronze.cast.worn` | M_Bronze_Statuary |
| Stems, shoes, glazing bars | `bronze.patinated.aged` | M_Bronze_Statuary: base (0.10, 0.061, 0.032) linear, roughness 0.40, patina 0x3A4232 |
| Reader plates and stems | `bronze.patinated.handled` | M_Bronze_Statuary |
| Frames (Art) | `oak.fumed.aged` *(oak missing)* | M_Frame_Wood_Dark |
| Mounts and mats (Art) | `paper.rag-board.aged` *(paper missing)* | the Swift's matte 0xF2EDE3 |
| Frame glazing (Art) | `low-iron-glass.anti-reflective.aged` over `laminated-uv` | none as built |
| Autochrome backing (Art) | `opal-glass.annealed.as-laid` *(opal glass missing)* | none as built |

---

## 4. Light

### 4.1 Daylight openings
All of these are Glazed, UV-filtering laminate, with no daylight-control devices (D-014: no automatic blinds).

| Opening | Area (about) | Weather observer |
|---|---|---|
| `hall-of-light.wall.north`, `.wall.south` | 2 × 29.5 × 4.54 ≈ 268 m² | museum |
| `hall-of-light.vault` | 10.5 m of arc × about 28.5 m ≈ 300 m² | museum |
| `hall-of-light.lunette` | ≈ 6 m² | museum |

By day only the sun and the sky light the hall (board). The lattice draws its shadow on the floor.

### 4.2 Luminaires (seeds `Data/Stages/HallOfLight/Lighting.json`)
Positions are local. Light.Scheme may refine the beams; the mounts are fixed here.

| Mount key | Where | Catalogue type | Count | Aim | Control class |
|---|---|---|---|---|---|
| `hall-of-light.arch.<k>.accent.<side>` | Under the arch's inner flange (R 5.15) on a short three-circuit track. Arches k = 0, 2, …, 14 (x = print bays 1–8, 1.30 … 22.30), at y = −1.10 for the north print and +1.10 for the south, z 7.23 | `track-spot.3000k.12deg` | 16 | The print's centre (x, ∓4.35, 1.60): 30.0° from vertical, throw 6.50 m; the beam of about 1.4 m covers the frame | gallery (presence and daylight-linked) |
| `hall-of-light.arch.<k>.stereo` | Under the flange of the arch over each stereo stone (k = 3, 7, 11, 15; x 5.80, 11.80, 17.80, 23.80), on the far side of the axis from the stone: y = −sign(stone y) × 1.19, z 7.21 | `track-spot.3000k.12deg` | 4 | The card on the reader, 30° above the plate's normal; throw 6.9 m | gallery |
| `hall-of-light.eaves.<side>.uplight.<j>` | Bracketed off the eaves' inner face (body within d 0.20 … 0.40, z 4.95 … 5.15). This is under the vault glass, which is 0.25–0.55 m above there. x = 0.55 + 3j, j 0…9 (midway in every other arch bay, clear of the arches' feet), both sides | `track-wallwasher.4000k` (nearest catalogue type; the as-built lamps were 15°/40° cone uplights at 3800 K) | 20 | Up across the vault to (x, ∓1.00, 7.40), washing the far side's arches and purlins | architectural (on at dusk, presence-dimmed) |
| `hall-of-light.plinth.<side>.h<k>` | Recessed in the plinth Housing under each post (x −0.20 + 3k), face at d 0.180, centre z 0.140. Never under a print. | **missing**: a louvred step light, proposed `step-light.4000k.louvred` (bronze faceplate 0.10 × 0.06) | 20 | Across the floor, nearly level, to (x, ∓2.00, 0) | architectural |

**None** for the autochromes: they are transparencies, lit through their opal backing by the daylight behind. A front
spot would only glaze and darken them (see 12, open decisions). **CCT:** 3000 K on the works (Care's 2800–3000 K) and
4000 K on the architecture (the Rotunda sheet's choice; the catalogue's nearest to the as-built 3800 K).

### 4.3 Targets by use and by care

| Surface | Target | When |
|---|---|---|
| The 16 prints, the 4 stereograph cards | Care class `works-on-paper`: ≤ 50 lx, 2800–3000 K, UV ≤ 75 µW/lm, ≤ 150,000 lx·h a year | always (Art.WithinCare) |
| The 4 autochromes | Care class `works-on-paper.fugitive` (dyed starch grains): ≤ 50 lx, ≤ 15,000 lx·h a year | always |
| Floor, axis and walking zone | `circulation`, 100 lx, Uo 0.4 | by day (daylight) |
| Floor at night | `circulation.night`, 50 lx, Uo 0.4, when anyone is present. Never below `escape-route`, 10.76 lx average along the axis. | after dusk |

The as-built prints got about 70 lx from the accent spots. That was the old workshop's figure, not Care's.

### 4.4 Design targets (luminance order, measured, not knobs)
- **Night.**
  - L(arches' inner flanges) ≥ L(print mounts) > L(floor) > L(glass reflections).
  - The pearl lattice is the hall's brightest surface: it reads as the glowing canopy of the design ("wash the pearl
    vault").
  - L(lattice) ≥ 1.5 × L(floor).
- **Day.** L(sky through the vault) > L(gardens through the walls) > L(floor) > L(print mounts). The prints stand
  before the bright garden by design ("so the garden shows over them").

### 4.5 Expected finding (not to be tuned)
- **By day, the prints and the stereo cards cannot hold 50 lx.** The idealised sun stands at 71.53°, and from 06:00
  to 18:00 it shines through the south glass and the vault onto the north wall's print faces.
  - At noon that is roughly cos(71.53°) × E_normal × τ, thousands to tens of thousands of lux.
  - The south faces and the stereo readers take the sky at hundreds to thousands of lux.
- UV-filtering glazing (D-014) holds the UV limit but not the lux or the dose.
- The hang must answer this (section 12, open decision 1). Light must report it and never dim or shade around it.

---

## 5. Time and weather
- **Weather.** Every Glazed opening here (walls, vault, lunette) takes the **`museum`** observer. D-005's remote
  weather belongs only to the Salon's lanterns (giverny) and Albion's glass (london). The gardens are Outdoors under
  the museum's weather too.
- **Sun.** The museum's model, `idealised` by default (D-003): a constant altitude of 71.53°, bearing
  180° + 15°·(h − 12), from 06:00 to 18:00. The hall runs exactly east–west, so:
  - **06:00 and 18:00.** The sun stands on the axis (east, then west). The arches' shadows fall across the floor,
    shifted along the axis by 0.334 × height (2.4 m from the crown). No direct sun reaches the walls' faces.
  - **12:00.** The sun is due south. The arches' planes contain the sun, so each arch's shadow lies straight under
    it. The purlins' shadows shift north by 0.334 × height, so the lattice draws a grid on the floor and on the
    north wall. The north prints are in sun; the south prints are backlit (sun on their mounts' backs).
  - **Ephemeris model.** In Beijing the noon altitude runs from 26.6° (winter solstice) to 73.6° (summer solstice).
    In winter the south wall throws long shadows across the floor.
- **Precipitation.**
  - **Rain.** Rain runs on the vault's and walls' outer faces (Matter's surface law by slope and sky exposure) and
    falls in both gardens (Nature, Outdoors). It stops at the hall's shell.
  - **Snow.** Snow holds on the vault's flatter crown and slides from its steep flanks (up to 58°, Matter's law). It
    lies on the kerbs, the hedges' tops and the gardens.
- **Season.** Phenology follows Time's solar term (MPC_Nature, section 7). **Wind:** the museum's wind moves the
  trees, flowers and grass. The hall is sealed, so there is no wind inside.

## 6. Motion
- **None.** The hall has no mechanisms. Both ports are open portals (their closure is the class's, and Site builds
  it). There are no blinds (D-014). Nothing here takes part in a coupling group, so the stage has one configuration.

## 7. Nature (seeds `Data/Stages/HallOfLight/Planting.json`)

**Ground.** `Ground(footprints, surface)` over the two gardens' plan outlines:
- flat at z 0.00, soil to −0.30;
- the kerbs' tops stand 60 mm proud;
- no water bodies.

Ground covers (Grounds sheet names):

| Garden | Cover | Plan bounds | Holes |
|---|---|---|---|
| North (meadow) | `meadow` (`grass.grown.seasonal`: meadow sward) | x 0.10 … 29.70, y −13.75 … −4.80 | 0.30 m round each birch |
| South (orchard) | `lawn` (`grass.mown.seasonal`: rough orchard sward) | x 0.10 … 29.70, y 4.80 … 13.75 | 0.35 m round each tree |

The as-built densities were 2.5 in the meadow and 1.5 in the orchard (nature.py:1117-1127). Both covers are cut by the
footprint at the east (F3–F6).

**Trees.** Each is grown by `Grow(species, seed from key, age, growth space)`. The growth space is the garden
volume less the sway at the design wind, and it must keep canopies off the eaves (\|y\| 4.60) and the vault.

| Keys | Species | Local (x, y) | Height | Crown centre | Crown radii (across, vertical) |
|---|---|---|---|---|---|
| `hall-of-light.birch.1…8` | `betula-pendula` (silver birch) | (2.80, −8.00) (5.30, −11.50) (9.80, −7.20) (13.30, −11.20) (16.80, −8.30) (20.80, −11.60) (24.30, −7.60) (27.30, −11.00) | 9.3 | 6.8 | 1.9, 2.5 |
| `hall-of-light.orchard.1…10` | alternating `prunus-avium` (sweet cherry, odd) and `malus-domestica` (apple, even) | cherry (3.30, 7.80); apple (9.30, 7.80); cherry (15.30, 7.80); apple (21.30, 7.80); cherry (26.80, 7.80); apple (4.80, 11.60); cherry (10.80, 11.60); apple (16.80, 11.60); cherry (22.80, 11.60); apple (28.30, 11.60) | 6.7 | 4.9 | 2.0, 1.8 |

- **Seeds.** As built, birch i used 100 + i and orchard i used 200 + i. Now the seed comes from each key.
- **Clearance to the glass.** The nearest birch (9.80, −7.20) clears the eaves by 0.70 at full crown.
- **Growth-space limit.** The apple at (28.30, 11.60) meets the footprint at x 29.90 (plan 41.1), so its crown grows
  about 0.4 m narrower on the east than the as-built sphere. That is the growth-space rule, not a patch.

**Hedges** (clipped form, handed over by the Grounds sheet 3.12):
- `hall-of-light.hedge.north` and `.south`, `taxus-baccata` (yew);
- clipped 0.45 wide × 1.50 high, batter 0.02, shoulder 0.06, core inset 0.065, open-ended;
- centre line y ∓14.00 from x −0.20 to 29.80 (plan 11.0 … 41.0).

**Flower beds.** These use Nature's instanced beds, and the points are the as-built ones
(`Shared/Plan/HallOfLightPlanting.swift`, `nature.py:921-930`) shifted by −11.20 in x.

| Bed | Colour | Points | Species | Height |
|---|---|---|---|---|
| North | lilac 0xA08AB0 | 41 | `knautia-arvensis` (field scabious) | 0.4–0.9 |
| North | pink 0xD9A5A0 | 41 | `malva-moschata` (musk mallow) | 0.4–0.9 |
| North | copper 0xC9826A | 53 | `helenium-autumnale` (sneezeweed) | 0.4–0.9 |
| North | yellow 0xE6C77A | 35 | `ranunculus-acris` (meadow buttercup) | 0.4–0.9 |
| South, spring bulbs *(species proposed; the board says only "spring bulbs")* | white 0xF3EEE5 | 60 | `narcissus-poeticus` | 0.2–0.35 |
| South | pink 0xD9A5A0 | 44 | `tulipa-saxatilis` | 0.2–0.35 |
| South | yellow 0xE6C77A | 46 | `narcissus-pseudonarcissus` | 0.2–0.35 |

The north flowers are the board's "scabious, mallow, helenium and buttercups".

**Phenology by solar term** (as built, HallOfLight.swift:232-263):
- birches leaf out in spring, turn gold in autumn and stand bare in winter;
- the cherry blossom is white and early, and the apple blossom pink-white and later;
- the fruit ripens in summer and autumn;
- the meadow flowers bloom in summer, thin by half in autumn, and stand as seed heads in winter;
- the bulbs flower in spring only;
- the yew stays evergreen.

## 8. Art (seeds `Data/Art/Hang.json`, anchors `hall-of-light.bay<b>.<wall>`)

**Hang rule.**
- **Order and anchors.** One work per wall per bay, west to east in time. Bays 1–8 are prints on Wall anchors; bays
  9–10 are autochromes on **Glazing** anchors (D-014).
- **Centre line.** z 1.600, the human measure's eye height.
- **Print mount.** A pale mount 1.000 × 0.900 (z 1.150 … 2.050), rag-board faced. Its face is at d 0.150
  (\|y\| 4.350) and its back at d 0.120, bearing on two bronze stems at bay x ± 0.300.
- **The print.** At true size, centred on the mount, in a dark oak frame 22 mm wide (outer 1.044 × 0.944, as-built
  frames.py `PHOTO_WIDTH`, `HOL_MOUNT`). It is glazed with anti-reflective UV glass and has a backing board, which
  is what you see from the garden.
- **Autochromes.**
  - The plate is at true size, centred in a white mat 1.000 × 0.900, in the same frame.
  - It stands against the wall glass's inner face (d 0.02 … 0.06), between the bronze glazing bars at bay x ± 0.560.
  - It is backed by opal glass, so the garden's daylight lights it evenly from behind.
- **Captions.** Captions are Art's, from the catalogue (FMuseeText), shown by the Visitor's overlay on gaze.

In the table, "Wall N" faces south and "Wall S" faces north. Sizes are the holding record's image size, height ×
width, in cm.

| Bay | x | Wall | Work key | Work | Medium (record) | Holding | Size h × w (cm) | Size source |
|---|---|---|---|---|---|---|---|---|
| 1 | 1.30 | N | `photo-talbot-leaf` | W. H. Fox Talbot, *[Leaf]*, c. 1840 | salted paper print | Met 285951 | 8.0 × 9.7 (sheet) | Met API |
| 1 | 1.30 | S | `photo-talbot-open-door` | Talbot, *The Open Door*, before May 1844 | salted paper print from paper negative | Met 283068 | 14.3 × 19.4 | Met API |
| 2 | 4.30 | N | `photo-atkins-algae` | Anna Atkins, *Delesseria hypoglossum*, c. 1853 | cyanotype | Met 291575 | 25.3 × 20.0 | Met API |
| 2 | 4.30 | S | `photo-atkins-algae-2` | Atkins, *Halydrys siliquosa*, c. 1853 | cyanotype | Met 291512 | 25.3 × 20.0 | Met API |
| 3 | 7.30 | N | `photo-hill-adamson-newhaven-fishwives` | Hill and Adamson, *Newhaven Fishwives*, c. 1845 | salted paper print | Met 282022 | 29.5 × 21.7 | Met API |
| 3 | 7.30 | S | `photo-fenton-valley-shadow-of-death` | Roger Fenton, *The Valley of the Shadow of Death*, 1855 | salted paper print | LoC (LC-DIG-ppmsca-35546) | 28 × 36 | LoC catalogue (via search) |
| 4 | 10.30 | N | `photo-nadar-bernhardt` | Nadar, *Sarah Bernhardt*, negative c. 1864, print c. 1924 | gelatin silver print | Getty | 21.1 × 16.2 (mount 33 × 24.1) | Getty record (via search) |
| 4 | 10.30 | S | `photo-le-gray-great-wave` | Gustave Le Gray, *The Great Wave, Sète*, 1857 | albumen print from glass negative | Met 261941 | 33.7 × 41.4 | Met API |
| 5 | 13.30 | N | `photo-marville-rue-traversine` | Charles Marville, *Rue Traversine*, c. 1868 | albumen print | Met 283713 | 34.8 × 27.5 | Met API |
| 5 | 13.30 | S | `photo-cameron-julia-jackson` | Julia Margaret Cameron, *Julia Jackson*, 1867 | albumen print | Met 267426 | 27.4 × 20.6 | Met API |
| 6 | 16.30 | N | `photo-muybridge-horse` | Eadweard Muybridge, *The Horse in Motion*, 1878 | albumen print on card | LoC | 14 × 22 card (LoC gives 22 × 14; landscape assumed) | LoC catalogue (via search) |
| 6 | 16.30 | S | `photo-osullivan-canyon-de-chelly` | T. H. O'Sullivan, *Ancient Ruins in the Cañon de Chelle*, 1873 | albumen print | LoC (ppmsca-10055) | ≈ 28 × 20.4 (AIC copy 27.4 × 20.1) | via search; **verify** |
| 7 | 19.30 | N | `photo-marey-bird-in-flight` | É.-J. Marey, *[Bird in Flight]*, 1886 | albumen print (record) | Met 286544 | 3.3 × 17.3 | Met API |
| 7 | 19.30 | S | `photo-atget-organ-grinder` | Eugène Atget, *Joueur d'orgue*, 1898/99 | gelatin silver printing-out paper (record) | AIC 55394 | 23.0 × 17.8 | AIC API |
| 8 | 22.30 | N | `photo-stieglitz-steerage` | Alfred Stieglitz, *The Steerage*, 1907 | photogravure | Getty | 33.4 × 26.4 (sheet 46.6 × 32) | Getty record (via search) |
| 8 | 22.30 | S | `photo-hine-sadie-pfeifer` | Lewis W. Hine, *Sadie Pfeifer, Lancaster Cotton Mills*, 1908 | photographic print | LoC (nclc-01455) | **unverified** (NCLC prints are commonly 12.7 × 17.8) | not read |
| 9 | 25.30 | N | `photo-lumiere-autochrome` | Lumière, *[Peacock]*, c. 1907 | autochrome | Met 285665 | 13.0 × 17.7 | Met API |
| 9 | 25.30 | S | `photo-lumiere-autochrome-4` | J. Gervais-Courtellemont, *Brest, the naval port*, c. 1912 | autochrome | Cinémathèque Robert-Lynen | **unverified** (9 × 12 or 13 × 18 plate) | not found |
| 10 | 27.75 | N | `photo-lumiere-autochrome-3` | Auguste Léon, *Serbian women in festival dress*, 1913 | autochrome | Musée Albert-Kahn | 9 × 12 (the Archives de la Planète's standard plate; the record itself not read) | via search |
| 10 | 27.75 | S | `photo-lumiere-autochrome-2` | Arnold Genthe, *Ann Murdock with Buzzer IV*, 1914 | autochrome | LoC | 12.7 × 17.8 (5 × 7 in) | LoC (via search) |

**Stereographs** (Plinth anchors `hall-of-light.stereo<n>`):
- The card lies at true size, centred on the bronze reader plate.
- The plate is 0.680 × 0.400 × 0.030 with its centre at z 1.320. It faces the axis, tilted back 28.65° (0.5 rad).
- It is glazed with anti-reflective UV glass.

| Stone | Local (x, y) | Work key | Work | Holding | Mount h × w (cm) |
|---|---|---|---|---|---|
| 1 | (5.80, −2.40), faces south | `stereo-1` | *Universal Exposition of 1855, interior*, stereo daguerreotype | Met 291843 | 8.6 × 17.1 (images 5.5 × 5.5 each) |
| 2 | (11.80, +2.40), faces north | `stereo-2` | London Stereoscopic Co., *Crystal Palace: Una and the Lion*, 1854–62, daguerreotype | Met 291844 | 8.3 × 17.1 (images 6.8 × 5.9 each) |
| 3 | (17.80, −2.40), faces south | `stereo-3` | George Stacy, *American Falls and Niagara River*, c. 1860–65, albumen stereograph | LoC | ≈ 8 × 17 (**verify**) |
| 4 | (23.80, +2.40), faces north | `stereo-4` | B.K. éditeur, *Boulevard des Italiens*, c. 1860–70, albumen stereograph | LoC (ds-04850) | 8.4 × 17.1 |

**Other notes on the works.**
- **Catalogue discrepancies.** `assets/collection.json` gives Atget as "albumen" and Marey as "chronophotograph".
  The holding records say gelatin silver printing-out paper and albumen. Art's catalogue should take the records.
- **Care classes.**
  - `works-on-paper`: the prints and the stereo cards (a daguerreotype on a card mount takes its most sensitive
    component).
  - `works-on-paper.fugitive`: the autochromes.
- **Casts and scans:** none. **Fittings:** no benches, stanchions or signs, as built. The readers and stems are the
  only fittings (Art's `Fittings.json`).

## 9. Sound
- **Character.** A live glass-and-stone hall, about 1,700 m³, with no absorbers in the design:
  - polished and honed travertine;
  - about 570 m² of laminated glass;
  - painted steel.
- **Reverberation.** Sound.Acoustics derives it from the enclosure and Matter's absorption. Expect several seconds
  at mid frequencies and less in the bass, where the large panes absorb. That is the physical truth of such a hall,
  so it is **never tuned**.
- **Emitters and events.**
  - **Footfalls** on polished travertine, from FMuseeContact, by the substance's footfall class.
  - **Rain** on the vault and walls: Sound's weather ambience placed on Form's exposed glass surfaces (≈ 570 m²).
    It is the hall's signature in weather.
  - **Wind** in the birch and orchard leaves outside, keyed to the Moment's wind.
  - **Birdsong** in the crowns (polish; see 12). Birds are matched to the museum's city by recorded evidence, for
    example Beijing: Eurasian tree sparrow and azure-winged magpie; New York: American robin and northern cardinal.
- **Occlusion.** The glazed shell occludes the gardens (the glass's transmission loss). The two open portals pass
  sound to and from the Rotunda and the Atrium.

## 10. Visitor
- **Walk the axis** west to east, forward in time from Talbot to Genthe, between the two gardens.
- **Viewing stones.** Stop on one: they are the five places on the axis between the stereo stones. In the headset
  they are natural step-to landings (they are flush, so they are walkable).
- **Look** at a print, an autochrome or a stereo card, and its caption comes up (IMuseeCaptioned, the Visitor's
  overlay). The board: "Look at a work and its placard comes up."
- **Nothing to sit on and nothing to operate**, as built. The gardens are seen, never entered. The glass is solid
  to the body, and step-to never lands beyond it.
- **The hour key and the city key** (global) visibly move the sun's shadows across the floor and the north wall.

## 11. The old code map
Re-express these through the new systems. Never copy the helpers.

| File : lines | What it holds | Goes to |
|---|---|---|
| `windows/Source/MuseeVision/Plan/MuseePlan.h:258-299` | The hall's numbers: half-width, eaves, plinth, vault R/centre, transom, posts, arches, stereo and viewing stones, print bays, mounts | `Private/HallOfLightPlan.h` (local frame, −11.20 in x) |
| `MuseePlan.h:30-64, 165-215, 245-256, 327-344, 533` | The Rotunda (r 11.2, doors), the Élan (centre 54, r 14 + 0.8, door 4 × 4.5), the frontispiece, the Atrium's floor, HallKerb 4.8 | port faces and coordination only |
| `windows/Source/MuseeVision/HallOfLight/HallOfLightStructure.h:12-41` | The hall in words (the design statement) | the module README |
| `HallOfLightStructure.cpp:29-68` | Embeds (drop them), floor edges, eaves/transom/shoe numbers, the light positions | plan constants; embeds become joints |
| `HallOfLightStructure.cpp:73-127` | Neighbour faces, the vault point, the springing and coping angles, purlin and strip angles, post and arch x | Form drawings (arcs, stations) |
| `HallOfLightStructure.cpp:129-279` | The sweep, I-section, lathe, along-side and upright helpers | Form's kernel (Sweep, Lathe); never re-implement |
| `HallOfLightStructure.cpp:291-456` | Floor, viewing stones and rings | `Lay()` slabs with cut-outs; rebated discs |
| `HallOfLightStructure.cpp:458-472` | Plinth and kerb sections | 3.1 |
| `HallOfLightStructure.cpp:474-527` | Glass of the walls and vault (the coping split: drop it) | 3.3 Glazed openings |
| `HallOfLightStructure.cpp:529-559` | Posts, transom, shoe, eaves, end posts | 3.2 |
| `HallOfLightStructure.cpp:561-639` | Arches, purlins, crosses, collars | 3.2 (18 arches, one collar section) |
| `HallOfLightStructure.cpp:641-693` | The doors' architraves | Site's doorcase (evidence, 3.6) |
| `HallOfLightStructure.cpp:698-716` | Stereo drums, reader stems | 3.1, 3.4, Art's reader |
| `HallOfLightStructure.cpp:718-746` | Print stems and shoes, autochrome glazing bars, path-light louvres | 3.4; louvres to Light's record |
| `HallOfLightStructure.cpp:762-796, 994-1021` | Uplight and path-light aims, cones, 3800 K, candela | 4.2 mounts and aims (candela never; the catalogue decides) |
| `HallOfLightStructure.cpp:845-852, 959-992` | Material assignments | 3.7 finish schedule |
| `HallOfLightStructure.cpp:904-929` | Night level from MPC Daylight | Light's Operation (dusk switching), not stage code |
| `windows/Source/MuseeVision/HallOfLight/check_hall_of_light.py:1-1100` | Python port and watertightness checks (the 1 cm rule, embeds) | Form.Sealed, Form.NoOverlap, FindLeakPaths |
| `Shared/Wings/HallOfLight.swift:10-35` | The plan (x0 10.26, x1 39.90, posts, stones, trees) | cross-check |
| `HallOfLight.swift:114-138` | Mounts 1.0 × 0.9, backs 0.12 in, the autochromes at the glass | 8 |
| `HallOfLight.swift:140-191` | Stereo reader: plate 0.68 × 0.40, tilt 0.5 rad, alternating views at 4 Hz | Art reader; the stereoscope is polish |
| `HallOfLight.swift:198-217` | The hang order | 8 |
| `HallOfLight.swift:219-266` | Gardens, hedges, trees, seasons, flower logic | 7 |
| `Shared/Plan/HallOfLightPlanting.swift:1-14` and `windows/Scripts/nature.py:921-930` | Flower-bed points | Planting.json |
| `nature.py:774-802, 849-850, 1089-1127` | Bark, leaf and blossom looks; tree positions; tree sizes; meadow bounds | species records and 7 |
| `windows/Scripts/frames.py:17-19, 52-59, 386-390` | Photograph frames: 22 mm dark, mount 1.0 × 0.9, autochrome mats | Art frame style |
| `windows/Scripts/gallery_lights.py:26-40, 76, 286-308` | Accent rule: 2.4 m above, 30° off the wall, 70 lx on paper, 4000 K | superseded by Care and 4.2 |
| `windows/Scripts/materials.py:519-521, 1018-1057, 1323-1332, 1394-1396` | Travertine colours, M_Travertine_Honed and Polished (slab 0.8 × 1.2, joints), bronze statuary | Matter seeds (D-017.3) |
| `windows/Scripts/native.py:161-176` | M_RibPearl (0xECE9E3, roughness 0.32) | pearl topcoat record |
| `windows/Source/MuseeVision/Facade/MuseeLandscape.cpp:283-311` and `Nature/MuseeHedge.cpp:28-41` | The Hall's yew hedges (0.45 × 1.5, y ±14, x 11–41) | 7 |
| `windows/Source/MuseeVision/Nature/MuseeLawn.cpp:314` | The lawn excluded from the gardens | Grounds receives the footprint |
| `windows/Source/MuseeVision/Facade/MuseeFacadeStructure.cpp:55-80` | The drum's skin kept clear of the hall | Rotunda coordination |
| `windows/Source/MuseeVision/ElanExterior/ElanExteriorStructure.cpp:36-41, 413-552` | The frontispiece round the hall's east end | Atrium's; its clear opening in 1.4 |
| `windows/Source/MuseeVision/Atrium/AtriumBaseStructure.cpp:66-67` | Atrium west door 4.0 × 4.5, square | port class |
| `windows/Source/MuseeVision/Furniture/MuseeFurniture.cpp:300-323` | Bronze thresholds at both doors | Site's joint |
| `plan/canvas/HallOfLight.dc.html` (lines 467–560) and `plan/README.md:107-111` | The board: the hang table, the stereo stones, the gardens, light by day and night, and the section (9 m, 5 m, 7.5 m) | the design; Site.MatchesCanvas |

## 12. First-pass cut

**Tonight** (enough to walk the hall between its neighbours, lit by the real sky):
1. **Ports and footprint.**
   - Both ports, with the concave faces and classes of section 2.
   - The footprint volume.
   - The three enclosures.
2. **Masonry.**
   - The floor, laid 1.20 × 0.80, with the five viewing stones and their rings.
   - The plinths with their 20 housings.
   - The kerbs.
   - Both linings, with their rough openings and the east lunette opening.
   - The four stereo stones.
3. **Steelwork, all of 3.2.**
   - 20 posts, the shoes, the transoms, the eaves, and 4 end posts.
   - 18 arches, 7 purlin lines, 136 crosses, 2 collars, and 5 lunette mullions.
   - All in pearl-white paint.
4. **Glazing, all of 3.3.**
   - The walls: 22 panes.
   - The vault: 152 hot-bent panes.
   - The lunette: 6 panes.
   - Weather: `museum`.
5. **Light.**
   - Daylight through the Glazed openings, and commissioning at the reference moments.
   - The 20 accent spots and the 20 uplights from the catalogue.
   - The path lights, only if their catalogue record lands tonight. Otherwise the floor's night target is reported,
     not faked.
6. **Art.**
   - The 16 prints at true size, on mounts and stems, framed and glazed.
   - The 4 autochromes in cassettes with opal backing.
   - The 4 stereo cards flat on the readers.
   - Captions on gaze.
7. **Nature.**
   - The flat ground with the meadow and orchard covers.
   - The 18 trees grown in their growth spaces.
   - The two clipped yew hedges.
   - Phenology by solar term.
8. **Sound.** Reverberation from Matter, footfalls, and rain on the glass.

**Waits for polish:**
- the flower and bulb beds (the ground covers carry the gardens tonight);
- birdsong;
- a true stereoscope (lenses, each eye its own view in the headset, instead of a flat card);
- the Atrium's frontispiece dress (the Atrium's);
- the Rotunda's dress over the vault;
- the doorcase surrounds in their final profile (Site's);
- rivulets and snow-slide refinement on the vault;
- per-pane glass variation;
- any night backlight for the autochromes.

**Open decisions (for the Architect; the builder is not blocked).**
1. **Care against daylight.** No original photograph can meet `works-on-paper` in this hall by day (4.5), and Care
   has no class for the alternative. The honest options:
   - (a) hang **exhibition copies** at true size, credited to the originals, as real museums do in daylit or outdoor
     settings (the autochromes as duplicate transparencies). Art needs a care class for the copy's material.
   - (b) accept a recorded exception, as D-005 does for weather.

   Until this is decided, Art.WithinCare reports the Hall as a known finding. Nothing is dimmed or shaded by hand.
2. **The autochromes by night.** Lit only by the daylight behind them, they are dark after dusk. That is honest, but
   the board said every photograph has its own spot. Options:
   - leave them dark;
   - add a backlight in the cassette (an image luminaire behind the opal).

   A front spot is not an option: it only glazes the plate.
3. **The deltas in 1.6:** the 50 mm linings, the 19th arch dropped, and the east lunette. These are the smallest
   changes that let the hall be a sealed stage with its ports on its neighbours' faces.
4. **Verify the unverified.** Before Art.TrueSize is final, check:
   - two work sizes (Hine, Brest);
   - O'Sullivan's LoC print;
   - the Muybridge card's orientation;
   - stereo-3's mount.

   The LoC's pages refuse scripted reads (HTTP 403), so these need a person's browser.
5. **Catalogue and record gaps** (these block the named checks until filled):
   - Light: a louvred step light, and ideally a 4000 K linear or cove uplight;
   - Matter: the pearl topcoat for `painted.steel`, and the substances `oak`, `paper` and `opal-glass`;
   - Nature: the species records `betula-pendula`, `prunus-avium`, `malus-domestica`, `taxus-baccata` and the
     seven flower species.
