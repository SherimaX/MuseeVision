# The Cube · design sheet

- **Stage key** `cube` (module `MuseeCube`, `UCubeRoom : USiteRoom`). Display name "The Cube". Level −1. Era: **Future**
  (Élan is three stages, D-012: the Atrium at level 0, the Cube below, the Sphere above). Its first work: *II · Sea Light*
  (`sea-light`, D-001). The Future's workings, with the Cube's own exception (Story, "The Cube"): the one room where the
  future is **black**: dark-patinated CuSn10 for sleeve, housing and plates, clean oil gilt for its joint lines and rings,
  low-iron glass, a lenslet array on the panel faces.
- **Sources:** D-001…D-019 (D-001, D-006, D-012, D-014, D-017, D-018 binding); Palette.json and Story.md ("The Cube
  (Future, level −1)"); the finished **`atrium.md`** and its `Mechanisms.json` (the car, the Cube mast, the stops, the
  sequences and interlocks: they win at the seam) and `sphere.md` (the same class, matched); the drafter's spec
  `r-specs/…/Stages/cube.md` (verified; corrections in §11); the boards `plan/canvas/ElanCube.dc.html`,
  `plan/boards/ElanCube.png`, `plan/README.md:119-125`; the as-built code in `windows/` (§11). Schedules:
  `Data/Stages/cube/{Finishes,Lighting,Mechanisms}.json` (no Hang, no Planting).
- **Units** metres. **Frame:** the Atrium's frame, as the old code's (`CubePlan.h:10-15`): origin on the Élan axis at the
  museum's floor datum (the Atrium's marble face, z 0.000); x east, y south, z up, true north. **museum = local + (54.000,
  0.000, 0.000)**, yaw 0. So local z = museum z = every as-built Cube height; the room's centre (the eyes at the stop) is
  local **(0, 0, −22.000)** = museum (54, 0, −22.000).
- **Bearings** are compass bearings, clockwise from north: a point at bearing b and radius r is (r sin b, −r cos b). Plan
  angle (from +x towards +y, as the old code and the iris pins) = bearing − 90°.

## 1. Identity and placement

A room 28 m each way, 22 m under the Atrium floor: the Sphere's twin, mirrored in the Atrium floor. Its six inner faces are
light-field panels (0.7 m modules) that put light into the room in three dimensions, true from anywhere. A glass shaft comes
down to it through soil, clay and chalk. The Atrium's glass car comes down the shaft, the iris in the Cube's ceiling opens,
closes on the head of a gilt mast, and the car sinks on the mast until the visitor's eyes are at the exact centre. The car
never lands; the room is sealed by its ceiling iris round the mast (D-012). It is for **near** pieces, which happen round the
visitor inside the room's own shell (no scoped sky). At rest it is black but for the faint 0.7 m grid of the panels.

| | |
|---|---|
| Origin in the museum plan | **(54.000, 0.000, 0.000)**, yaw 0 (the Atrium's origin, `MuseePlan.h:167`). Site derives it from `cube.up ↔ atrium.down` (§2): local (0, 0, −0.500) = museum (54, 0, −0.500). |
| Level | −1 (the eyes at the stop, z −22.000; `CubePlan.h:26`). |
| Footprint kind | Building, under the Atrium on the same axis (Site.NoOverlap and RoomStaysInside compare volumes: the box lies under the Atrium and 1.4 m under the Hall of Light's east end, x −16.200 … −14.800, all below the plane −0.500). |
| Vertical extent | z −38.200 (the box's base) … −0.500 (`cube.up`, the Atrium slab's soffit). |

**Key levels** (local z = museum z): **mating plane** (`cube.up`, the Atrium slab's soffit) −0.500; head plate −0.510 …
−0.500; rails' top −0.520; ground contacts (mean) −0.720 (hardcore | topsoil), −1.020 (topsoil | subsoil), −1.500 (subsoil |
clay-with-flints), −4.000 (clay | chalk; the solution pipe to −4.750); flint bands −4.580, −5.360; ammonite −4.950; urchin
−5.220; glass tiers' boundaries −0.500, −1.825, −3.150, −4.475, −5.800; **box roof** top −5.800, soffit −6.800 (the sleeve's
joint); housing roof −7.600 … −7.590; the car's floor at the **ceiling gate** −7.750 (and the iris shuts behind a rising
car above −7.450); rails' foot −7.937; **top blade face −7.942**, stack bottom −7.968; soffit ring top −7.970; **ceiling**
(panel faces, the ring's underside) −8.000; mast pinspots −8.012; rail tails −7.973 … −8.345; car floor at **`clamp`
−10.825** (the mast head's groove in the iris); **eyes −22.000** (the room's centre); the car's floor at **`cube`
−23.600**; the car's lowest point −23.760; the well's bottom −23.800; **floor** (panel faces) −36.000; box base −37.200 …
−38.200.

**Footprint** (plan outline, the box): **(−16.200, −16.200) → (16.200, −16.200) → (16.200, 16.200) → (−16.200, 16.200) →
close** (museum x 37.800 … 70.200, y −16.200 … 16.200). Stacked prisms:

| Prism | z | Plan |
|---|---|---|
| P1 the box | −38.200 … −5.800 | the square above |
| P2 the shaft column | −5.800 … −0.500 | the circle r 2.900 about the axis (a 96-gon of circumradius 2.905) |

**Seams.** *Cube | Atrium:* the plane z −0.500 (the Atrium's slab soffit and lining bottom r 2.300 … 14.000 above it; the
Cube's head plate, top tier frame, cavity top plate, ground and rail brackets touch it face to face). Outside P1 and P2 the
ground is the site's (not modelled).

| Enclosure | Kind | Bounded by |
|---|---|---|
| `cube.room` | Enclosed | the panel faces x, y ∈ [−14.000, 14.000], z ∈ [−36.000, −8.000]; at the ceiling's opening the soffit ring and the ceiling iris (shut, or clamped round the mast head). 21,952 m³. Sealed whenever the iris is shut or clamped; joined to `cube.shaft` only while the iris is open for the car. |
| `cube.shaft` | Enclosed | the bore from the iris (top blade −7.942) to the plane −0.500: housing inner wall and sleeve r 2.450, the frames r 2.410 and the glass r 2.450 between. ≈ 140 m³. Open at `cube.up` to the Atrium's well (the lining's bore), sealed below by the iris. |
| `cube.shaft.cavity` | Enclosed | the annulus r 2.510 (the glass's outer face) … the ground's cut face (r 2.660 less its relief), between the head plate (−0.510) and the foot plate (−5.790); the three tier webs cross it to r 2.600. ≈ 13 m³, lit, dark to the room. |

**Openings (Form's one list):** Port `cube.up`; Housing `cube.up.iris.housing` (r 2.460 … 4.300, z −7.970 … −7.600). The
shaft's glass is a partition between `cube.shaft` and its cavity, not an opening: the Cube has no Glazed or Open opening,
no daylight and no sky (D-012). The panels are a luminaire, not an opening.

## 2. Ports

| Key | Kind | Class | Local frame (position; facing) | Museum frame | Face; depth | Mates with | Wells |
|---|---|---|---|---|---|---|---|
| `cube.up` | vertical (received well) | **`iris.4600`** | (0.000, 0.000, −0.500); **+z** | (54.000, 0.000, −0.500); +z | the plane z −0.500 (the Atrium slab's soffit); **7.500** (to the ceiling, −8.000) | `atrium.down` (Atrium-local (0, 0, −0.500) facing −z = museum (54, 0, −0.500)). **Passage 0.00.** | **receiving**: the Atrium's served prism below, −0.500 → **−23.800** |

- **`iris.4600`** as `atrium.md` §2 defines it: the well's section is the circle r **2.300** less two notches 16 mm wide
  (tangential ±0.008) from r 2.265 to 2.300 at bearings 0° and 180°. Closure: seven blades, each a 110° arc of the middle
  circle r 3.095, 1.250 wide with round ends, **2 mm thick at a 4 mm pitch** (stack 26 mm), pins at r 3.745 on plan angles
  360°k/7 (k 0…6, pin 0 due east), blade k 4k mm below the top; each turns 52° on its pin, eased over 4.0 s; open, the
  inner edge at r 2.470 in a seat r ≥ 2.460; in travel the blades reach r 4.010; each blade's pin-end is lengthened by a **pin lug**, a round boss r 0.060 about the pin joined to the round end by a 0.120-wide web on the pin's radius, so the blade encloses its pin (as built the pin stood 25 mm outside the blade). States **shut 0**, **clamp 0.3206** (round
  the mast head's groove r 0.195; the blades leave r 0.197; the channel maps linearly to the blade angle (Motion `follows`), so the as-built eased clamp 0.378 is **0.3206** here, the same blade pose), **open 1**.
- **The closure is the Cube's** (the stage below the plane): its **ceiling iris**, top blade face **−7.942** (7.442 below
  the plane, `CubePlan.h:62`), declaring **`cube.up.closure`** (0 shut, 0.3206 clamp, 1 open). The Atrium's car sequences
  step it; the blades follow it (§6). **Joint:** none; the faces touch (the Atrium's lining bottom on the Cube's head plate,
  top frame and brackets). No collar or threshold at the plane.
- **The received well:** the Cube reserves the prism r ≤ 2.300 (less the notches) from −0.500 to −23.800 and builds inside
  it only what the class allows: the **rails** in the notches (−0.520 → −7.937, and their **tails** −7.973 → −8.345 under the
  iris) and its **ceiling iris** (the blades sweep across the well at −7.968 … −7.942). Below the ceiling the well is the
  room's air down to −23.800. What moves in the well is the Atrium's: the car, its leaves, guides, crown and light, the Cube
  mast and its head (`atrium.car.mast.head`), the column the crown feeds out.
- **Contacts across the seam** (declared, checked for no interpenetration, not for clearance): the car's roller guides and
  pinions on the Cube's rail blades and racks; the clamped blades round the mast head's groove (2 mm radial).
- **Closed states.** Mated (whenever the Atrium is built) the ceiling iris closes the port. **Unmated** (the stand): Site
  sets the class closure shut on the plane, `cube.up.blind-iris` (z −0.500, blades facing up); the ceiling iris stays shut;
  no car, no mast, no performance.
- **Topology (D-012):** the Cube is a leaf: one port, mating `atrium.down`. The glass car never lands in the Cube: no side
  vestibule, light lock, landing, landing door or doorway. No stage pushes a sky; Sea Light performs inside the sealed shell.

### Anchors (the stage contract)
Bodies are drawn in their closed pose (blades shut).

| Key | Kind | Frame (origin; axes) | Belongs to |
|---|---|---|---|
| `cube.up.iris.blade.1`, `cube.up.iris.blade.2`, `cube.up.iris.blade.3`, `cube.up.iris.blade.4`, `cube.up.iris.blade.5`, `cube.up.iris.blade.6`, `cube.up.iris.blade.7` | Body | each at its pin on its top face: (3.745, 0.000, −7.942), (2.335, 2.928, −7.946), (−0.833, 3.651, −7.950), (−3.374, 1.625, −7.954), (−3.374, −1.625, −7.958), (−0.833, −3.651, −7.962), (2.335, −2.928, −7.966); axes of the stage; turns about z through the pin | the ceiling iris (the closure of `cube.up`) |
| `cube.performance` | Use | (0, 0, −23.600): the car's floor datum on the axis at the `cube` stop. Usable: feet inside the car (r < 2.130) within 0.30 of its floor, the car at `cube` | the work's verbs: "II · Sea Light, 8 min" and "End the piece" (§6, §8) |
| `cube.panels.ceiling` | Mount | (0, 0, −8.000); faces −z (into the room); x, y of the stage | the ceiling's panel modules |
| `cube.panels.floor` | Mount | (0, 0, −36.000); faces +z | the floor's panel modules |
| `cube.panels.north` | Mount | (0, −14.000, −22.000); faces +y | the north wall's modules |
| `cube.panels.south` | Mount | (0, 14.000, −22.000); faces −y | the south wall's modules |
| `cube.panels.east` | Mount | (14.000, 0, −22.000); faces −x | the east wall's modules |
| `cube.panels.west` | Mount | (−14.000, 0, −22.000); faces +x | the west wall's modules |

No Wall, Bed, Water, Seat or Glazing anchors: nothing is hung, planted or sat on; the visitor stays in the car.
**Channels** (the Cube's, 0…1; Mechanisms.json `channels`): **`cube.up.closure`** (the ceiling iris; stepped by the
Atrium's car sequences, followed by the blades), **`cube.performance.active`** (published: 1 while a work performs, set by
the performance, §6), **`cube.mast.light`** (derived, followed by the mast pinspots). **Read:** `atrium.car.height`,
`atrium.car.doors`, `atrium.car.light`.

## 3. Pieces by trade

Every piece is one solid of one substance meeting its neighbour face to face. Radii from the axis; (r, z) sections lathed
about it.

### 3.1 Structure: the concrete box (hidden, `cube.box`)
| Piece | Solid | 
|---|---|
| East, west walls | x ±(15.200 … 16.200), y −16.200 … 16.200, z −38.200 … −5.800 |
| North, south walls | y ±(15.200 … 16.200), x −15.200 … 15.200, same z |
| Base slab | x, y −15.200 … 15.200, z −38.200 … −37.200 |
| Roof slab | x, y −15.200 … 15.200, z −6.800 … −5.800, pierced by the circle r 2.750 (the sleeve) |

Cast in situ, 1.000 thick, sealed. The **service void** between the panels' backs (±14.100, ceiling −7.900, floor −36.100)
and the box (±15.200, −6.800, −37.200) holds the panels' subframe, the ultrasound arrays (neither modelled tonight) and the
iris housing.

### 3.2 The panel wall (Light's image luminaire; the faces `cube.panels`)
- **Face planes** x = ±14.000, y = ±14.000, z = −8.000 (ceiling) and −36.000 (floor), each module's face towards the centre.
- **Grid** 0.700 pitch, 40 × 40 a face, a module corner at each face's centre; **module** face 0.698 × 0.698 (2 mm seams),
  0.100 deep behind the face (a design value until Light's record gives one). At the room's edges the faces meet on the
  planes (no overlap; a 0.100 × 0.100 void behind each edge).
- **Ceiling cut:** the rough opening is the circle **r 4.130** (the soffit ring's outer edge 4.120 plus a 10 mm shadow gap):
  88 positions wholly inside it are omitted, **44** crossed by it are shaped to it (made to order). **9,512 modules** (the
  board's 9,600 less 88).
- Collision: Solid; nothing Walkable. The ±0.06° set-out tilt between modules is the record's mounting tolerance.
- Mounts: the six `cube.panels.*` anchors; Light places the modules on the grid and draws their housings from its record.

### 3.3 The shaft's glass and gilt (−5.800 … −0.500)
- **Glass** `cube.shaft.glass`: r **2.450 … 2.510**, 60 mm laminated low-iron, **four tiers** between −0.500, −1.825,
  −3.150, −4.475, −5.800; four curved panes a tier (16), 90° each, joints on bearings 45°, 135°, 225°, 315°. Pane edges:
  tier 1 −0.555 … −1.800; tier 2 −1.850 … −3.125; tier 3 −3.175 … −4.450; tier 4 −4.500 … −5.745 (each laps 5 mm behind the
  frames). The glass stands in compression on the tier webs and the foot plate through setting blocks; gaskets close the
  joints (polish).
- **Tier frames** `cube.shaft.gilt`: gilt bands r **2.410 … 2.450** inside the glass: top −0.560 … −0.500; middle three
  60 mm centred on −1.825, −3.150, −4.475; bottom −5.800 … −5.740. Each bolts to the web or plate at its level (§3.4).
- **Joint strips** `cube.shaft.gilt`: r 2.438 … 2.450, 25 mm wide, on bearings 45°, 135°, 225°, 315° over the pane joints,
  each running one tier between its frames: −0.560 → −1.795, −1.855 → −3.120, −3.180 → −4.445, −4.505 → −5.740.
- Frames and strips are notched **0.190** wide round each rail and its bracket (5 mm clear).

### 3.4 Plates, webs and the cavity (`cube.shaft.plate`)
| Piece | Solid |
|---|---|
| Head plate | annulus r 2.450 … 2.770, z −0.510 … −0.500 (over the glass's head and the cavity; under the Atrium's lining and slab) |
| Tier webs ×3 | annuli r 2.450 … 2.600, 10 mm, centred on −1.825, −3.150, −4.475, through the glass's 50 mm tier joints (the panes stand on them); the graze lines hang under them |
| Foot plate | annulus r 2.450 … 2.770, z −5.800 … −5.790, on the sleeve's top and the box roof; the glass stands on it |

The cavity r 2.510 … 2.660 is closed by the head and foot plates; the webs stop 60 mm short of the cut face.

### 3.5 Guide rails (`cube.rail`, brackets `cube.rail.bracket`)
- **Rails** north and south (bearings 0°, 180°), the Atrium's section: blade r **2.265 … 2.385**, 16 mm; foot r 2.385 …
  2.425, 140 wide; a rack on each back. **From −0.520 to −7.937** (5 mm over the top blade; the 30 mm break at −0.520 over
  the Atrium's rails' foot −0.490 is bridged by the car's second ring).
- **Tails** below the iris (same section): **−7.973 … −8.345** (5 mm under the stack), so the car's top-ring guides and
  pinions hold it on rack to the `clamp` stop (top ring −8.295 … −8.225 there); the 36 mm break across the stack is bridged
  by each guide block (70 and 95 mm tall). Each tail hangs from a gilt bracket under the soffit ring (`cube.soffit`): r
  2.425 … 2.560, 0.180 wide, z −8.060 … −8.000, bolted up into the ring (the inner bead notched round it).
- **Brackets** 0.180 wide × 0.100 tall, r 2.425 … 2.450, fixing each rail's foot: at the head plate (−0.600 … −0.500), the
  webs (centred −1.825, −3.150, −4.475), the foot plate (−5.800 … −5.700) and on the sleeve's face (centred −6.200 and
  −7.100). The rails' vertical load goes to the sleeve brackets and the box roof; the others guide.

### 3.6 Sleeve, housing and ceiling ring
| Piece | Geometry | Surface |
|---|---|---|
| Sleeve, void course | r 2.450 … 2.750, z −7.590 … −6.800 | `cube.sleeve` |
| Sleeve, roof course | r 2.450 … 2.750, z −6.800 … −5.800 (through the roof slab) | `cube.sleeve` |
| Sleeve joint line | r 2.442 … 2.450 (8 mm proud), z −6.810 … −6.790, over the courses' joint | `cube.shaft.gilt` |
| Housing inner wall (the iris's seat) | r 2.450 … 2.460, z −7.938 … −7.600 | `cube.housing` |
| Housing roof | annulus r 2.450 … 4.310, z −7.600 … −7.590 (the sleeve stands on it) | `cube.housing` |
| Housing outer wall | r 4.300 … 4.310, z −7.900 … −7.600, on the ceiling modules' backs (between −8.000 and −7.900 their cut edges r 4.130 close it) | `cube.housing` |
| Ring hangers | 14 flats 10 mm (tangential) × 60 mm, r 4.040 … 4.100 (clear of the blades' 4.010 sweep), z −7.970 … −7.600, at plan angles 12.857° + 25.714°k | `cube.housing` |
| **Soffit ring** | (r, z): (2.460, −7.970) → (2.460, −8.000) → (4.116, −8.000) → (4.116, −8.004) → (4.120, −8.004) → (4.120, −7.970) → close: a 30 mm gilt annulus, 4 mm drip at its outer edge | `cube.soffit` |
| Ring beads | tori: major r 2.472, tube r 0.009, centre z −8.002; major r 4.100, tube r 0.007, centre z −8.001 | `cube.soffit` |

The housing (Housing `cube.up.iris.housing`) is the space r 2.460 … 4.300, −7.970 … −7.600 in which the blades move and lie
open; the soffit ring hides them from the room.

### 3.7 The ceiling iris (the closure of `cube.up`, class `iris.4600`)
Bodies `cube.up.iris.blade.1` … `.7` at the pins of the Anchors table. Each blade: 2 mm pearl steel, a 110° arc of the
middle circle r 3.095, 1.250 wide, round ends; its lug and pin bearing per the class (polish). Upper faces
`cube.up.iris.top`, undersides and edges `cube.up.iris.under`; a **hairline** `cube.up.iris.line` along each long edge of the
upper face (7 mm, 4 mm inside the inner edge and 11 mm inside the outer, 0.4 mm proud) so the shut iris reads as a spiral
from the shaft. Stack −7.968 … −7.942: shut, it seals the bore r 2.460 30 mm over the ceiling plane; clamped, it closes to r
0.197 round the mast head's groove.

### 3.8 The ground's cut face (`cube.ground.*`)
A heightfield on an inward-facing cylinder, drawn by Form from the stratigraphic rule of §7: **nominal face r 2.660**,
standing proud towards the axis by the rule's relief (≤ 0.050); back face r **2.860**; from the head plate (−0.510) to the
foot plate (−5.790) and, beyond r 2.770, up to the slab (−0.500) and down to the roof (−5.800). Mesh step 12.5 mm (the
flints round, not stepped): 1,337 stations round × 423 down. **One piece per layer**, split along the rule's contacts
(shared stations):

| Layer | Between (mean) | Surface |
|---|---|---|
| Crushed stone (hardcore) | −0.510 … −0.720 | `cube.ground.crushed-stone` |
| Topsoil | −0.720 … −1.020 | `cube.ground.topsoil` |
| Subsoil | −1.020 … −1.500 | `cube.ground.subsoil` |
| Clay-with-flints | −1.500 … −4.000 (the pipe to −4.750) | `cube.ground.clay-with-flints` |
| Chalk | −4.000 … −5.790 | `cube.ground.chalk` |
| Flint nodules and flints (polish) | §7 | `cube.ground.flint` |
| Ammonite (bearing 58°, −4.950); heart urchin (286°, −5.220, polish) | §7 | `cube.ground.fossil` |

The face is consolidated in place (as a soil monolith is) and held by soil nails behind it beyond r 2.860 (hidden, polish).

### 3.9 What the Cube does not build
The Atrium: the lining through its slab (r 2.300 … 2.560, −0.500 … −0.030), its own rails above −0.490, and everything that
moves in the well (the car with its guides, pinions, crown, light and leaves; the Cube mast's head and the column the crown
feeds out). Site: the blind `cube.up.blind-iris` on the stand. Light: the panel modules' housings, the strips and pinspots
from their records. Art: Sea Light's image source and content.

### 3.10 Finishes (summary; Finishes.json holds the notes)

| Surfaces | Material · working · history |
|---|---|
| `cube.box` | `accent.concrete` · cast-in-situ.sealed · new |
| `cube.panels` | `glass.low-iron` · lenslet-array.black · new |
| `cube.shaft.glass` | `glass.low-iron` · laminated · new |
| `cube.shaft.gilt`, `cube.soffit` | `bronze.cusn10` · gilt.gold-leaf.oil · new |
| `cube.sleeve`, `cube.housing`, `cube.shaft.plate`, `cube.rail.bracket`, `cube.up.iris.line` | `bronze.cusn10` · patinated.dark · new |
| `cube.rail` | `steel.hard-chrome` · ground.hard-chromed · new |
| `cube.up.iris.top`, `cube.up.iris.under` | `white.pearl-steel` · gilt.gold-leaf.oil · new |
| `cube.up.blind-iris` | `white.pearl-steel` · pearl.satin · new |
| `cube.ground.crushed-stone`, `cube.ground.topsoil`, `cube.ground.subsoil`, `cube.ground.clay-with-flints`, `cube.ground.chalk`, `cube.ground.flint`, `cube.ground.fossil` | `accent.site-geology` · excavated.&lt;layer&gt; (`excavated.crushed-stone`, `excavated.topsoil.damp`, `excavated.subsoil.damp`, `excavated.clay-with-flints.spade-cut`, `excavated.chalk.dry`, `excavated.flint.nodular`, `excavated.chalk.fossil-stained`) · new |

## 4. Light

**Daylight:** none. No opening to the sky; the sun never reaches level −1. Light from the Atrium's well falls down the shaft
onto the shut iris and stays out of the sealed room.

**Luminaires** (Lighting.json, 42; points local; no output values here)

| Ids | Type | Count | Point and aim | Operation |
|---|---|---|---|---|
| `panel.ceiling`, `panel.floor`, `panel.north`, `panel.south`, `panel.east`, `panel.west` | `gap.light-field-panel.d65` (flat 0.7 m modules) | 6 faces, 9,512 modules | Mounts `cube.panels.*`, facing the centre; aim (0, 0, −22.000) | always: at rest the standby image (the black level and the faint 2 mm seam grid); while `cube.performance.active` = 1 the performing work's image and the seams off |
| `graze.1.1` … `graze.4.8` | `linear-cove.3000k` | 32 (4 rings × 8), each 1.700 long | strip bent to r 2.585 at bearings 22.5° + 45°k, z −0.550, −1.875, −3.200, −4.525 (50 mm under each tier's top), under the head plate or a web in a dark channel, facing down; aim the cut face (r 2.660) at mid-tier | presence: a body in the shaft, the car between the Atrium stop and `clamp`, or someone at the Atrium's landing |
| `mast.1` … `mast.4` | `pinspot.3000k` (6.1°, DALI-2) | 4 | under the soffit ring's inner edge at r 2.500 on bearings 45°, 135°, 225°, 315°, z −8.012; aim (0, 0, −20.220), 0.6 over the crown at the stop (throw 12.46, 11.6° off vertical); the beams end on the car | presence, following `cube.mast.light` (the car in the room, no work performing) |

The graze lines light the ground only (their light reaches the car through the glass, never the room). The pinspots light
the gilt mast and the crown, and through the car's clear roof its floor; nothing lights the panel faces. Sea Light's light
is the panels' image and lights no piece in the first pass.

**Targets.** No care class (no object) and no use class on a Cube surface (no Cube surface is walked). **Orientation**
(UseTargets: "the Cube's sealed shell during a performance", 5 lx): on the Atrium's car floor (`atrium.car.floor`) at the
`cube` stop, during a performance and at rest, met by the car's own light ring at its glimmer (`atrium.car.light` 0.03, the
Atrium's) and, at rest, the pinspots' spill through the roof. Entered in the Atrium's Lighting.json targets (the surface is the
Atrium's; `when` the car is at `cube`); measured, never tuned; a shortfall is a finding. No walked route falls below
`escape-route` outside a performance.

**Design order** (Lighting.json `design`; measured, never tuned): in the shaft, under the same graze lines, chalk ≥
clay-with-flints ≥ topsoil (albedo alone); from the car the gilt tier frames read brighter than the glass's reflections of
them. In the room (the panels' record, measured as findings): at rest the spill of every luminaire on the panel faces adds
at most a tenth of their black level; during Sea Light the room's mean luminance stays in the work's record (the proposal's
0.005 … 0.15 cd/m²), black at both ends. The Eye adapts on its own (no local clamp).

## 5. Time and weather
No weather place and no Moment term on any Cube surface: sealed, 6 to 38 m down. The damp soils are a fixed working, not
live wetness. Sea Light's rain (6:00 … 7:00) is the work's score, not the museum's weather. D-006: the Cube is always open;
its light is presence-linked.

## 6. Motion
The Cube owns its ceiling iris (Mechanisms.json, 7 entries) and declares three channels and one performance. With the
Atrium's car, Cube mast and collar iris and the Sphere's irises they form one coupling group; Site binds the channels
across the seam. The car's stops in the Cube (the Atrium's M-1, `atrium.car.height` = (z + 23.600) ÷ 44.000):

| Stop | Car floor z | Fraction | What |
|---|---|---|---|
| `cube` | −23.600 | 0.000000 | eyes at the room's centre; never lands |
| `clamp` | −10.825 | 0.290341 | the mast head's groove centred in the stack (`CubeWay.cpp:79-83`): the iris clamps or releases |
| `atrium` | 0.000 | 0.536364 | the Atrium's landing |

**M-C1…M-C7 the ceiling iris** `cube.up.iris.blade.1` … `.7`: revolute about each blade's pin (its body's origin), axis −z,
drawn shut, opening 52° towards decreasing plan angle; **follow `cube.up.closure`** in [0, 1] → out [0, 52], eased s = 3o² −
2o³ over 4.0 s. Housed in `cube.up.iris.housing`.

**Channels** (Mechanisms.json `channels`):
- `cube.up.closure`: stepped by the Atrium's `atrium.car.lift` sequences (below); the blades follow it.
- `cube.performance.active`: 1 from a work's Begin until its fade-out has ended, else 0 (the performance below).
- `cube.mast.light` (derived; Mechanisms.json `channels`): follows `cube.performance.active` in [0, 1] → out [1, 0].
  The pinspots' operation is `presence`, so they light only with the visitor's car in the Cube, and while no work performs.

**The Atrium's sequences at the Cube (matched with `atrium.md` §6 and its Mechanisms.json):**
- "Down to the Cube" (`atrium.car.panel` at `atrium`; requires `atrium.down` mated): leaves shut → `cube.up.closure` → 1
  (4.0 s) → move `clamp` (the car waits at −7.750 until the iris is open) → `cube.up.closure` → 0.3206 (2.49 s) → move `cube`
  on the mast (1.0 m/s, 0.5 m/s², ≈ 14.8 s) → `atrium.car.light` → 0.03 (2.5 s). ≈ 25 s.
- "Up to the Atrium" / "Up to the Sphere" (`atrium.car.panel` at `cube`; both **require `cube.performance.active` ≤ 0**):
  `atrium.car.light` → 1 (2.5 s) → move `clamp` → `cube.up.closure` → 1 (2.49 s) → move on up → `cube.up.closure` → 0 (4.0 s)
  once the car is above −7.450.

**The performance** (Mechanisms.json `performances`, `cube.sea-light`): Use `cube.performance` at `cube`, verb **"II · Sea
Light, 8 min"**, caption as §8; **requires** `cube.up` mated, `atrium.car.height` ≤ 0 (at `cube`), `cube.up.closure` 0.3186 …
0.3226 (clamped: sealed), `atrium.car.doors` ≤ 0, `atrium.car.light` ≤ 0.03 (the glimmer reached) and
`cube.performance.active` ≤ 0; sequence `cube.performance.active` → 1; the work runs 480 s (fade in 2 s). **Ends** (fade out
3 s, then `cube.performance.active` → 0) when its score ends, on **"End the piece"** (the same Use while it performs), or
when the eyes leave the room's cube (\|x\|, \|y\| or \|z + 22\| > 14.000).

**Interlocks** (Motion, by key; each honours the Atrium's rule of the same subject):

| # | Rule | Atrium |
|---|---|---|
| C-1 | The car floor passes below the ceiling gate **−7.750** only with `cube.up.closure` ≥ 0.999 (its lowest point, −0.160, then clears the top blade by 32 mm). | A-6 |
| C-2 | The iris leaves **open** towards **clamp** only with the car floor at −10.825 ± 0.005 (the head's groove in the stack), and shuts (→ 0) only with the car floor above **−7.450** and not heading down through it. | A-6 |
| C-3 | The car leaves `clamp` downward only with the iris at 0.3206 ± 0.002, upward only with it ≥ 0.999. | A-6 |
| C-4 | The car leaves `cube` only with `cube.performance.active` ≤ 0 and `atrium.car.light` back at 1: a performing work holds it. | A-8 |
| C-5 | A work begins only with the room sealed: the car at `cube`, its leaves shut, the iris clamped (0.3206 ± 0.002). | — |
| C-6 | Occupancy guard (`body-in-travel`): a blade holds or reverses while any body but the mast head is in its next travel slice. | A-3 |

**Clearances:** car rings (2.235) ↔ tier frames (2.410) 175 mm, ↔ housing inner wall (2.450) 215, ↔ soffit bore (2.460) 225,
↔ open blades (2.470) 235; guides (2.290) ↔ the well (2.300) 10 mm, ↔ frames 120; guides ↔ rail blades and racks: rolling
contact; rail foot (−7.937) ↔ top blade (−7.942) 5 mm; stack bottom (−7.968) ↔ tails (−7.973) 5 mm, ↔ soffit ring top
(−7.970) 2 mm; top blade ↔ housing inner wall's foot (−7.938) 4 mm; blades in travel (r ≤ 4.010) ↔ hangers (4.040) 30 mm,
↔ the ceiling modules' cut edges (4.130) 120, ↔ outer wall (4.300) 290; stack (26 mm) ↔ the head's groove (40 mm, 0.070 …
0.110 over the crown) 7 mm above and below at `clamp`; clamped blades (0.197) ↔ groove (0.195) 2 mm: a designed clamp
contact; car's top ring at `clamp` (−8.225) ↔ ceiling (−8.000) 225 mm; pinspots (heads Ø 0.105 about r 2.500: r 2.448 … 2.553) ↔ guides (2.290) 158 mm;
car's lowest point at `cube` (−23.760) ↔ well bottom (−23.800) 40 mm; the car ↔ the panels ≥ 11.8 m.

## 7. Nature
**No plants and no water** (Sea Light's water is only light). **The ground** (Form's drawing by rule, `CubeStructure.cpp:238-390`;
seeded periodic noise round the cylinder, keyed `cube.ground`; θ round the axis, z local; noise N(θ, z, f) at f cycles/m):

| Contact / layer | Rule | Face |
|---|---|---|
| Hardcore \| topsoil | −0.720 + 0.025 N(4) | crushed stone: angular stones 12–28 mm in a 45 mm jittered cell, 95 % filled, ≤ 14 mm proud |
| Topsoil \| subsoil | −1.020 + 0.06 N(2.5) + 0.02 N(9); a 6 cm colour blend | crumbly: relief 7 mm (18 /m) + 4 mm (55 /m); stones 8–20 mm (70 mm cell, 30 %), ≤ 9 mm proud |
| Subsoil \| clay | −1.500 + 0.07 N(2) + 0.03 N(7) | as topsoil |
| Clay \| chalk | −4.000 + 0.12 N(1.6) + 0.05 N(6), less a **solution pipe** at bearing 205°: 0.75 exp(−(s/0.32)²), s the arc from it | clay-with-flints: smooth, spade-cut, vertical marks 1.8 mm at a 0.11 pitch; angular flints 15–45 mm (0.11 cell, 28 %), ≤ 16 mm proud (polish) |
| Chalk | to the foot plate | blocky: relief 10 mm (5 /m) + 5 mm (16 /m); bedding partings every 0.37, 5 mm deep, 12 mm wide |

- **Flint nodule bands** (polish): at −4.580 and −5.360 (± 0.05 N(3)); nodules r 0.06–0.12 in a 0.24 cell, 62 % filled, ≤ 40
  mm proud as rounded knobs; a white cortex, glassy black inside.
- **Ammonite** (Mantelliceras-like, `CubeStructure.cpp:397-437`): Ø 0.360 at bearing 58°, centre −4.950, its mid-plane 4 mm
  proud of the face, coil facing the shaft, half exposed; 3.4 whorls on a logarithmic spiral expanding 2.1× a turn; whorl
  section 0.46 × 0.30 of the whorl radius; 26 ribs a turn, 7 % high, growing in from the first whorl.
- **Heart urchin** (Micraster, polish, `CubeStructure.cpp:440-472`): ≈ 64 mm, bearing 286°, centre −5.220; a front notch and
  five faint petals.

## 8. Art
**The work** `sea-light`: **II · Sea Light**, after Anna Atkins, *Photographs of British Algae: Cyanotype Impressions*
(1843); the museum's own commission for the Élan Cube (2026); a light field with placed sound and touch (in the game: the
panels' image, placed procedural sound, controller haptics); **480 s**. Not in `assets/collection.json`: Art adds it to its
collection (related works in the Hall of Light: `photo-atkins-algae`, `photo-atkins-algae-2`). **Care:** none (no object).
**Caption** (`SeaLight.h`): "A sea at night, full of dinoflagellates that flash blue when the water moves. Nothing lights up
unless something moves, and the first thing that moves is you." **Hang:** none (no Hang.json): it performs in `cube.room`
through the panel wall (the `cube.panels.*` mounts) at the Use `cube.performance`, only while the room is sealed (§6).

**The work as built** (`SeaLight.h`, `SeaLight.cpp`; metres from the eyes at (0, 0, −22.000), the stage's axes):

| Part | As built |
|---|---|
| The field | 96³ cells over a 12.0 m cube centred on the eyes (0.125 m): how stirred (0 … 2) and which way it moves (±2 m/s); stirring decays with τ 0.7 s |
| The cells | 1,048,576 points (four turned copies of 262,144), within r 0.3 … 5.9 of the eyes and \|z\| ≤ 4.4, densest in the reach (r = 0.3 + 5.6u^1.35); each flashes above its threshold (0.15 + 0.8 × its random + 0.9 × the thinning), cyan-white fresh (0.45, 0.86, 1.0) to deep blue fading (0.01, 0.16, 1.0), ≈ 475 nm; ≥ 1.5 px wide; those with a random above 0.45 streak along the flow (≈ 1/7 s) |
| The hand | a wake along its path (splats r 0.07, 10 × its travel, 0.8 × its velocity) and a swirl of six splats on a ring r 0.11 at 5 rad/s |
| The shoal (2:30 … 4:00) | 700 scad, 0.30 m, a band 12 × 5 × 3 from x −13 heading (1, −0.12, 0); cruise 1.1 … 1.4 m/s within 0.7 … 1.9; alignment 2.2, cohesion 0.9, separation 0.06 within 0.3, neighbours in a 0.8 m grid; they part round the car, never inside r 3.000, z −2.8 … +2.6 |
| The manta (4:30 … 5:30) | 4.5 m span at 1 m/s from (−14, 7, 2.8) along (1, −0.5, −0.03) over the car's roof; banks 22° and turns 35° away between 20 and 32 m; wingbeat 5 s, tips lift 0.45 |
| The rain (6:00 … 7:00) | ≤ 18 drops/s (8 s ramps) on a surface 4.5 m over the eyes within r 6; rings 0.03 → 0.45 (τ 0.6 s), 1.4 s life, sparks 0.15 below |
| The score | 0:00 black, in over 2 s; 0:45 far sparks 2/s; 1:00 "Reach out and stir the water" (9/s); 2:30 the shoal (16/s); 4:30 the manta; 6:00 the rain; 7:00 … 7:58 the bloom thins until only the hand stirs; 8:00 it ends, out over 3 s |
| Content | `SM_SeaLight_Cards`, `SM_SeaLight_Fish` (0.28 m spindle, forked tail), `SM_SeaLight_Manta` (19-point outline, 0.35 deep): under `/Game/Generated/Works/SeaLight/` only, feeding Light's image source; never on a piece |

Not built: the proposals *Ink*, *Snow*, *Murmuration*, *Brushwork*; the Matterhorn and Raja Ampat journeys (retired, D-001).

## 9. Sound
- **Acoustics** from Matter, never tuned: `cube.room` 21,952 m³ inside 4,704 m² of panels over a 1.1 m absorptive void (the
  panels pass sound to the arrays behind them): near-anechoic (expected RT60 well under 0.5 s at 500 Hz). `cube.shaft`, a
  4.9 m glass tube: hard, ringing along its axis. The clamped iris cuts the room off from the shaft and the Atrium.
- **Emitters:** at rest, silence. The blades (seven on pins, 4 s) and the car's drive and mast feed (the Atrium's) from the
  Motion actuations. **Sea Light's four voices** (48 kHz mono, procedural, HRTF-placed but the hush): hush (brown noise,
  breathing ±25 % over 9 s, at the listener, gain 0.35); fizz (ticks 2–6 kHz, 1.5 ms, up to 900/s with the hand's speed, at the
  hand, inner r 0.6, falloff 15 m, 0.6); shoal hiss (band noise following the fish within 10 m, at the shoal's centre, falloff
  25 m, 0.6); rain (ticks 300–800 Hz, 12 ms, 40/s, at (0, 0, −17.500), inner r 6, falloff 15 m, 0.6). Levels at 1 m are the
  work's record (Sound's gap).

## 10. Visitor
- **Arrive** by the Atrium's car ("Down to the Cube"): the strata go by in the glass shaft (≈ 9 s), the iris opens ahead,
  closes on the mast ("The iris has closed round the mast. The car sinks to the Cube's centre, 22 m under the Atrium floor."),
  and the car sinks on it (≈ 15 s). At the stop its light falls to a glimmer: "The Cube: 28 m each way, its six faces
  light-field panels, at rest. Its first piece: II · Sea Light."
- **Stay in the car:** no step out, no seat; the car's door never opens here. Look anywhere: every face is 14 m away.
- **Uses:** at rest, "II · Sea Light, 8 min" (`cube.performance`), then the car's "Up to the Atrium" / "Up to the Sphere";
  while it performs, only "End the piece". At 1:00, **reach**: on the desktop hold the right mouse button and move (the hand a
  point 0.6 m along the look ray); in the headset the right Touch controller (its tip 6 cm ahead of the grip), which buzzes
  with the sparks and the shoal's passing (frequency 0.8, amplitude ≤ 0.6).
- **Leave:** the piece fades (3 s), the car's light comes back (2.5 s), the car climbs the mast, the iris lets the head go.

## 11. The old-code map
Paths relative to `windows\Source\MuseeVision\` unless noted. Re-express, never copy.

| Part | Where |
|---|---|
| All the Cube's plan numbers (room, void, box, stop, shaft, lining, glass, earth, frames, sleeve, soffit, iris top, housing, clamp, rails, mast, ammonite) | Cube/CubePlan.h:17-83 |
| Élan axis, eye height, car, speeds | Plan/MuseePlan.h:24, 160-215 |
| Panel faces and the ceiling hole (r 4.13, square 5.6) | Cube/CubeStructure.cpp:43-101 |
| Concrete box and roof hole | Cube/CubeStructure.cpp:105-131 |
| Lining soffit, cavity plates, tier frames, strips, sleeve and its line, housing, soffit ring and beads, rails and brackets | Cube/CubeStructure.cpp:140-211 |
| Glass panes | Cube/CubeStructure.cpp:215-234 |
| Strata rule, cut face mesh; ammonite; urchin | Cube/CubeStructure.cpp:238-390; 392-437; 439-472 |
| Materials (evidence only); cavity lights (4000 K, 1.7 m, barn doors 60°) | Cube/CubeStructure.cpp:527-541, 616-634; 516-525, 636-663; CubeStructure.h:137-140 |
| Retired: the room's post-process clamps (RoomPost, SetRest), journey mode | Cube/CubeStructure.cpp:501-514, 572-587, 665-706; CubeStructure.h:95-102 |
| Iris placement, clamp level, mast head profile, column, mast lights (3500 K, 5°), apply | Cube/CubeWay.cpp:79-83, 95-98, 100-117, 119-137, 138-158, 163-192 |
| The iris (blade outline, pins, stack, inlay, swing, easing); constants | Elan/ElanSphereTop.cpp:46-181; Elan/ElanKit.h:77-84 |
| Car rings, crown, head groove, head top, guides | Elan/ElanKit.h:43-55, 97-100 (the Atrium's) |
| Timings; gate; prompts at the stop; piece holds the car; ceiling iris goals and clamp hold; mast speed; messages; glimmer | Elan/ElanElevator.cpp:40-46, 309-321, 505-517, 552-561, 588-592, 620-631, 673-674 |
| Sea Light: plan constants, voices, begin/end, field, hand, shoal, manta, rain, score, tick | Cube/SeaLight.h:17-199; Cube/SeaLight.cpp:26-35, 54-128, 335-464, 466-527, 529-677, 679-733, 735-772, 774-845, 847-934 |
| Sea Light meshes; panel, strata and mast materials | Cube/JourneyGen.cpp:655-807; windows/Scripts/journeys.py:266-331, 331-407, 410-433 |
| Resting state (muons, polish) | Cube/CubeRest.h:10-65; CubeRest.cpp |
| Boards | repo plan/canvas/ElanCube.dc.html; plan/boards/ElanCube.png; plan/README.md:119-125 |

**Corrections to the draft** (`r-specs/…/Stages/cube.md`):
1. **Frame:** the Atrium's frame (the old code's, `CubePlan.h:10`), origin on the axis at z 0, museum = local + (54, 0, 0);
   the draft's room-centred origin (local = museum + 22 in z) is dropped. Every z in this sheet is the museum's.
2. **Port** `cube.shaft` → **`cube.up`**, class **`iris.4600`** (draft `shaft.lift.4900.iris`); the closure is the Cube's own
   ceiling iris publishing **`cube.up.closure`** (draft: Site's joint, `cube.shaft.closure`; Q-2 closed); no collar ring on
   the plane (no threshold at a vertical port). Frame and museum position unchanged: (54, 0, −0.500).
3. **Blades** 2 mm at a 4 mm pitch (draft and as built 5 mm at 3 mm), stack −7.968 … −7.942; pearl steel with oil gilt faces
   (draft `bronze.gilt.clean`/`dusted`).
4. **Channels and anchors:** `cube.performance.active` (draft `cube.space.performing`); the anchor `cube.space` (kind
   Space, not a contract kind) replaced by the Use `cube.performance` and six Mount anchors; no Hang.json.
5. **Rails** hard-chromed steel (draft brushed bronze), run to −7.937 (draft −7.912) with **tails** under the iris (new):
   the draft's rails ended 0.313 above the car's top-ring guides at the clamp stop, so the car had no rack there.
6. **Plates and webs:** head and foot plates r 2.450 … 2.770 and three tier webs carry the glass (draft cavity plates r 2.510
   … 2.770 only; the panes had no bearing). The bottom rail bracket at −5.750 (as built centred −5.800 across the sleeve's top).
7. **Light:** 3000 K throughout; the graze lines in the catalogue's `linear-cove.3000k` (draft gap `linear-graze.4000k`);
   the panels `gap.light-field-panel.d65` shared with the Sphere (draft `image.light-field-panel.700`); pinspots
   `gap.pinspot.3000k`; the Q-6 exemption replaced by the `orientation` target (ruled).
8. **Materials** as palette keys (draft role strings): `accent.concrete`, `accent.site-geology` with the layer as its working,
   `steel.hard-chrome`, the dark patina and oil gilt on `bronze.cusn10`.
9. **Verified unchanged:** the box, void and face planes; the 9,512 modules (88 omitted, 44 shaped); the glass tiers, pane
   edges, frames and strips; the sleeve, its line and the housing; the soffit ring and beads; the rail section and brackets'
   heights; the graze and pinspot positions and aims; clamp −10.825, gate −7.750, stop −23.600, well −23.800; the strata rule,
   ammonite and urchin; Sea Light's numbers, score, voices and caption.

## 12. The first-pass cut
**Tonight:** the declaration (port `cube.up` with its received well, the two prisms, three enclosures, the housing, the
anchors, three channels and the performance); the box; the sleeve, housing and hangers, the soffit ring and beads; the head
and foot plates and tier webs; the glass with its frames and strips; the rails with tails and brackets; the five layer pieces
of the ground with the rule's contacts, the pipe, the bedding partings, the spade marks and **the ammonite**; the ceiling iris
(M-C1…M-C7) with C-1…C-6 against the Atrium's car; the 42 luminaires (the panels only if their gap record lands; otherwise
the faces are black glass and the design order fails honestly); *Sea Light* (field, cells, hand, shoal, manta, rain, score,
four voices, haptics), performing only while sealed; acoustics.

**Waits for polish:** the flint bands, the flints in the clay, the urchin, the stones beyond the rule's relief; the blades'
hairlines, lugs and pin bearings; setting blocks and gaskets; the panels' subframe, the arrays, the soil nails, the box's
construction joints; the resting state's muon tracks (a standby image source for the panels, Light's); the iris's light
seals if the leak probe finds the 2 and 4 mm slots; the field lighting nearby pieces; ultrasound touch; Sea Light's headset
scale.

## 13. Palette keys to confirm and catalogue gaps
**Palette keys** (all real entries or ruled; workings outside `workingsByEra` "future" or roles guessed):
- `bronze.cusn10` **patinated.dark** for sleeve, housing, plates, webs, brackets and the blades' hairlines: the palette's
  Cube role and the Story's "dark", outside the future's "mirror-polished and lacquered, or gilt" (the palette asks for a
  dark patina regime).
- `bronze.cusn10` **gilt.gold-leaf.oil** for frames, strips, the sleeve line, the soffit ring and the tails' brackets: the
  Story's "dusted on upward faces" needs a **dusted** history the brief's set lacks (recorded as `new`).
- `white.pearl-steel` with **gilt.gold-leaf.oil** on both faces of the ceiling iris's blades (guessed role: the palette names
  pearl for iris linings; as built gilt both sides; oil gilt over the lacquer as the Atrium's).
- `glass.low-iron` **lenslet-array.black** (the palette's "a lenslet array in the Cube"; the working needs a record) and
  **laminated** for the shaft glass.
- `accent.site-geology` with the layer as the working (`excavated.crushed-stone`, `.topsoil.damp`, `.subsoil.damp`,
  `.clay-with-flints.spade-cut`, `.chalk.dry`, `.flint.nodular`, `.chalk.fossil-stained`): one palette key for the whole
  ground; records per layer are the palette's open research (and the family of a soil).
- `accent.concrete` **cast-in-situ.sealed**; `steel.hard-chrome` (ruled) **ground.hard-chromed** for the rails and tails.

**Catalogue gaps** (Light):
- `gap.light-field-panel.d65`: the Sphere's gap record, here its **flat** module: light-field image panels (after Light Field
  Lab SolidLight), 0.700 pitch, 0.698 faces, 2 mm seams with an edge-lit seam line for the standby grid, black mask (face
  reflectance ≤ 1 %), volumetric (points where the rays cross), fed by a work's image source; 9,512 in six faces.
- `gap.pinspot.3000k`: a miniature pinspot, 5° beam (2° core), 3000 K, CRI ≥ 90, DALI (e.g. an ERCO Parscan or Mike Stoane
  pinspot class), its body under the soffit ring; the catalogue's narrowest (`track-spot.3000k.12deg`) would spill past the car
  onto the room. **Closed (K11):** `pinspot.3000k` is ERCO's Optec spotlight, narrow spot, 3000 K CRI 92, DALI-2, article
  75995.000 (data sheet 28.04.2026: 642 lm, 29 001 cd, a 6.1° cone, 8.7 W, 0.73 kg, head Ø 105), on ERCO's DALI track or a
  DALI singlet. The Parscan 48V narrow spot at 3000 K (A1014621, 4.4°) was set aside: its only control is On-board Dim, a
  knob on the fitting, which can't follow presence or `cube.mast.light`, and no Casambi narrow spot article was found. The
  6.1° beam is a little wider than the sheet's 5°: at the 12.46 m throw it spreads 1.33 m, so it still ends on the car.

**Site:** `PortClasses.json` needs **`iris.4600`** (as `atrium.md` §2) with the closure-at-declared-depth rule: the Cube's at
7.442 below the plane.

## Build notes
- **Frame and translation:** the Atrium's frame; **museum = local + (54.000, 0, 0)**, yaw 0; old plan x − 54; every height is
  the as-built height; the room's centre is (0, 0, −22.000). Bearings clockwise from north; plan angle = bearing − 90°.
- **The port** is the plane −0.500 facing +z; the Cube's head plate, top frame, cavity plate, bracket and ground touch the
  Atrium's lining and slab there. No threshold.
- **Received well:** keep r ≤ 2.300 (less the notches) clear from −0.500 to −23.800 except the rails and tails in the
  notches and the iris's blades. The room's air is the well below −8.000.
- **Moving bodies:** only the seven blades (fixed-frame bodies on their pins, drawn shut). The car, the mast head and the
  column are the Atrium's (never modelled here); the column is Motion's variable-length piece.
- **Channels:** `cube.up.closure` is stepped by the Atrium's car and followed by the blades; `cube.performance.active` is the
  work's, read by the Atrium's rides; `cube.mast.light` is derived and followed by the pinspots.
- **Draw by rule:** the panel grid and the 44 shaped ceiling modules; the soffit ring and beads (lathes); the blades from the
  class; the ground's heightfield (§7, 12.5 mm) split by layer; the ammonite's spiral.
- **Housing and ring:** the soffit ring hangs from the housing roof on 14 hangers outside the blades' sweep; the rail tails
  hang under the ring; nothing crosses the stack's level inside r 4.010.
- **Unmated** (the stand): Site's blind iris on the plane, the ceiling iris shut, no car, no performance.

**Departures from as-built** (for the owner's review):
- **Ground and panels (K3-MATTER):** each ground layer is its own substance, laid by its excavated working (chalk, iron-stained chalk, flint, subsoil, clay-with-flints, crushed-limestone hardcore; colours are first values from Munsell field descriptions, unmeasured). The panels' lenslet array is a clear face working; its black matrix is the light-field panel's.
- **iris.4600 corrected:** each blade gains a pin lug (r 0.060 boss on a 0.120 web) so it encloses its pin; as built the pin stood 25 mm outside the blade. The clamp value is 0.3206 on the linear channel (as built 0.378 through a smoothstep: the same blade pose, r 0.197). Shut, clamp, open and reach checked by polygon union.
- **Mast pinspots** (contract shape): as built on below the clamp + 0.5 with no work performing; now `presence` with a derived channel that follows the performance, which is equivalent because the visitor is in the Cube only in the car.
- **Ruled with the Atrium:** the port is `cube.up` (class `iris.4600`) closed by the Cube's own ceiling iris; the car, the
  mast head and the column are the Atrium's; the clamp (−10.825) and gate (−7.750) are kept exactly.
- **Retired (D-001):** the Matterhorn and Raja Ampat journeys, "Down: later, new art" and the journey mode that hid the
  building; Sea Light is the only work.
- **The rails reach the clamp:** extended to 5 mm over the stack and continued as tails under the iris (as built they ended
  30 mm over the blades, so the car hung on nothing for its last 0.31 m to the clamp).
- **The glass stands on webs:** head and foot plates and three tier webs through the tier joints carry the panes and the
  graze lines (as built the frames were inside the glass and the panes had no bearing); the ground's face is consolidated and
  soil-nailed (as built it stood unsupported).
- **Blades 2 mm at a 4 mm pitch** (class `iris.4600`; as built 5 mm at 3 mm, interpenetrating), pearl steel gilt on both faces
  (as built gilt bronze).
- **The soffit ring hangs** from the housing roof on 14 flats outside the blades' sweep; the housing's outer wall stops on the
  modules' backs; the cavity plates meet the glass instead of passing into it (as built floating surfaces).
- **Panels as modules:** 9,512 whole or shaped modules 0.100 deep (as built continuous quads with no depth).
- **Light:** 3000 K graze lines (as built 4000 K) and pinspots (as built 3500 K); the local post-process clamps (EV 2 at rest,
  EV 1 in the piece) retired: the Eye adapts on its own.
- **The car's glimmer stays 0.03 during a performance** (as built it fell to 0.002): UseTargets' `orientation` (5 lx) for the
  Cube's sealed shell rules it.
- **No memory of the visit:** "Up to the Sphere" no longer waits for a visit to the Cube (the Atrium's ruling).
- **Materials follow the Story:** dark-patinated bronze for the shaft's metal and hard-chromed steel rails (as built dark and
  brushed bronze); the geology named as the ground under Élan.
- **Schedule corrections (ST-CUBE, to the stage contract):** Mechanisms.json's `channels` entries are keyed by `key`,
  and the performance gives `seconds` (480) and `publishes` (`cube.performance.active`) in place of `length` and the two
  channel steps that set and cleared it. The clamp is the contract's **0.3206** under Motion's linear follow (the sheet's
  0.378 eased, s = 3o² − 2o³), and the performance requires `cube.up.closure` 0.3178 … 0.3235 (the sheet's 0.378 ± 0.002
  eased). §2, §6 and C-2, C-3, C-5 keep the eased figures.
- **Panel mounts:** each `cube.panels.*` Mount's x is the way its face looks into the room, as Light reads a mount (its
  light leaves along x); the ceiling's and the floor's keep the stage's y.
- Lighting.json, to the contract (K3-LIGHT-NATURE): the catalogue gaps are real types now, `light-field-panel.d65` (Light Field Lab SolidLight, an image panel lit by its picture, dark at its standby black level) and `pinspot.3000k` (ERCO Parscan 48V narrow spot, standing unlit until its 3000 K data sheet is read; K11: ERCO's Optec narrow spot on DALI-2, 75995.000, lit); each panel wall gives its face as an `extent`, 28.0 x 28.0 (40 x 40 modules at 0.700), the ceiling 28.0 x 26.46 (its 1,512 modules' area).
