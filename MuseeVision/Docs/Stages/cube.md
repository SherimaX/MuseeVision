# The Cube · stage spec

**Stage key:** `cube` · **module:** `MuseeCube` (Editor, `Source/Stages/Rooms/Cube`) · **display name:** *The Cube*
(FText) · **level:** −1 · **its first work:** *Sea Light* (`sea-light`, modules `MuseeSeaLight` and
`MuseeSeaLightMaking`, `Source/Stages/Works/SeaLight`).

This is the design sheet the Room Builder builds from tonight (D-016). Every number below comes from the as-built
museum (the frozen `windows/` code, the `ElanCube` board, `plan/README.md`), which D-001 makes the design. Where this
sheet re-expresses the old geometry to satisfy a rule of the new architecture, it says so in a line starting
**Re-expressed:**. Open questions for the Architect are collected at the end.

**Decisions applied:**
- D-001: the design is the as-built museum; the Matterhorn and Raja Ampat journeys are retired.
- D-012: the Cube is sealed by its ceiling iris around the mast; no side vestibule, light lock or landing door; the
  glass car never lands; no scoped sky.
- D-014: Carving and Ornament; gilt is a working; the passage; interlocks.
- D-015: Matter defines, Studio writes.
- D-016: the first pass is tonight.
- D-017: port, don't redesign. The as-built geometry is kept; only solidity forces the few re-expressions marked
  below. The designer's choices live in schedules in `Data/Stages/Cube/`:
  - `Finishes.json` is the materials table at the end of §3;
  - `Lighting.json` holds §4.2 and §4.4;
  - `Hang.json` binds `sea-light` to `cube.space`.
- D-018: materials are chosen, not copied. The material keys here name **roles** (the as-built choice). The palette
  (`Docs/Matter/Story.md`, `Data/Matter/Palette.json`) names the real material behind each one.

**Units and frame.** Metres. The stage's local frame: x east, y south, z up, true north. Angles in plan are given as
compass **bearings** (clockwise from north) unless a line says *plan angle* (from east, towards south, as the old code
measures them; bearing = plan angle + 90°).

---

## 1. Identity and placement

**What it is.** A room 28 m each way under the Élan Atrium: the Sphere's twin, mirrored in the Atrium floor. Its six
inner faces are light-field panels (0.7 m modules) that put light into the room in three dimensions, true from
anywhere. A glass shaft comes down to it from the Atrium's floor through soil, clay and chalk. The glass car comes down
the shaft, the iris in the Cube's ceiling opens, the car slides down a gilt mast hung from the iris until the
visitor's eyes are at the exact centre, and the iris closes round the mast. The car never lands. The room is for
**near** pieces: things that happen round the visitor, within the room's own walls. At rest it is black but for the
faint 0.7 m grid of the panels.

**Origin.** The room's exact centre: the point where the visitor's eyes stop, on the Élan axis. Every face of the room
is 14.000 m from it.

**Placement in the museum plan** (x east, y south from the Rotunda's centre; z from the museum's finished-floor
datum, the Atrium floor = 0): the origin sits at **(54.000, 0.000, −22.000)**. Local z = museum z + 22.000. Site only
translates; the stage is authored true north.

**Key levels** (local z / museum z):

| What | Local z | Museum z | Source |
|---|---|---|---|
| Atrium floor slab's soffit: the **mating plane** | +21.500 | −0.500 | CubePlan.h:45 `SlabBottom` |
| Crushed stone / topsoil contact (mean) | +21.280 | −0.720 | CubeStructure.cpp:297 |
| Topsoil / subsoil (mean) | +20.980 | −1.020 | CubeStructure.cpp:298 |
| Subsoil / clay-with-flints (mean) | +20.500 | −1.500 | CubePlan.h:49 |
| Clay-with-flints / chalk (mean) | +18.000 | −4.000 | CubePlan.h:50 |
| Flint nodule band 1 | +17.420 | −4.580 | CubeStructure.cpp:335 |
| Ammonite (centre) | +17.050 | −4.950 | CubePlan.h:76 |
| Heart urchin (centre) | +16.780 | −5.220 | CubeStructure.cpp:445 |
| Flint nodule band 2 | +16.640 | −5.360 | CubeStructure.cpp:335 |
| Box roof top = bottom of the shaft glass | +16.200 | −5.800 | CubePlan.h:36, 51 |
| Box roof soffit | +15.200 | −6.800 | CubePlan.h:35 |
| Iris housing roof | +14.400 | −7.600 | CubePlan.h:63 |
| Top blade's top face (the seal) | +14.058 | −7.942 | CubePlan.h:62 |
| Soffit ring top | +14.030 | −7.970 | CubePlan.h:61 |
| **Ceiling**: panel faces and the soffit ring's underside | +14.000 | −8.000 | CubePlan.h:24 |
| Ceiling gate: car floor may pass below only with the iris open | +14.250 | −7.750 | ElanElevator.cpp:46 |
| Car floor at the clamp level (mast head in the iris) | +11.175 | −10.825 | CubeWay.cpp:17-21 (derived) |
| Eyes at the stop: **origin** | 0.000 | −22.000 | CubePlan.h:26 |
| Car's rail top at the stop | −0.630 | −22.630 | ElanKit.h:53 |
| Car floor (the car's frame) at the stop | −1.600 | −23.600 | CubePlan.h:41 |
| Car's lowest point (mast collar) at the stop | −1.760 | −23.760 | ElanKit.h:55 |
| Bottom of the well | −1.800 | −23.800 | this sheet |
| **Floor**: panel faces | −14.000 | −36.000 | CubePlan.h:25 |
| Box base top | −15.200 | −37.200 | CubePlan.h:37 |
| Box base bottom | −16.200 | −38.200 | CubePlan.h:38 |

**Footprint** (kind **Building**), two stacked prisms:
1. **The box**: square, corners (−16.200, −16.200), (+16.200, −16.200), (+16.200, +16.200), (−16.200, +16.200); z from
   −16.200 to +16.200. In the museum plan: x 37.800 … 70.200, y −16.200 … +16.200, z −38.200 … −5.800.
2. **The shaft column**: circle r 2.900 about the axis (a 96-gon); z from +16.200 to +21.500 (museum −5.800 … −0.500).

The box lies under the Atrium (r 14.8 about the same axis) and, at its west edge, 1.4 m under the east end of the Hall
of Light (plan x 37.8 … 39.2). Both are above the mating plane, so the stages never share a volume; see risk R-11.

**Enclosures:**

| Key | Kind | Bounds (local) | Openings |
|---|---|---|---|
| `cube.room` | Enclosed | the cube x, y, z ∈ [−14.000, +14.000] inside the panel faces | `cube.room.eye` (Port) |
| `cube.shaft` | Enclosed | the shaft's bore from the top blade (+14.058) to the mating plane (+21.500): the glass and the sleeve at r 2.450, the tier frames at r 2.410 | `cube.shaft.mouth` (Port), `cube.room.eye` (Port), `cube.shaft.glazing` (Glazed, internal) |
| `cube.shaft.cavity` | Enclosed | the annulus r 2.510 … 2.660 (the ground's face) from +16.210 to +21.490 | `cube.shaft.glazing` (Glazed, internal) |

- The room's shell is the concrete box (1.000 m, sealed) with the panel wall as its lining; the service void between
  them lies inside the shell's build-up and is not an enclosure.
- `cube.room` is sealed whenever the iris is shut or clamped on the mast head (§6). It is open to `cube.shaft` only
  while the iris is open for the car.
- `cube.shaft` holds the well (§2). Outside the well it holds the Cube's fixed shaft fit-out: the tier frames, the
  joint strips and the guide rails.
- `cube.shaft.cavity` is the lit gap between the glass and the ground's cut face. It is closed top and bottom by
  bronze plates, so the graze lines' light leaves it only through the glass.

**Openings** (Form's one list):

| Key | Kind | Geometry (local) |
|---|---|---|
| `cube.shaft.mouth` | Port | circle r 2.450 on the mating plane (+21.500), where the well leaves the stage. The collar ring r 2.300 … 2.454 is the joint's. |
| `cube.room.eye` | Port | the ceiling's rough opening, r 4.130 through the panel layer (+14.000 … +14.100), and r 2.750 through the roof slab (+15.200 … +16.200), lined by the sleeve down to r 2.450. Closed by the iris (§6.1). |
| `cube.room.eye.housing` | Housing | the iris pocket, r 2.460 … 4.310, +14.030 … +14.410 (§3.8) |
| `cube.shaft.glazing` | Glazed (internal, no sky) | the glass tube, r 2.450 … 2.510, +16.200 … +21.500, between the cavity and the shaft. No weather observer. |

**Anchors:**
- `cube.space` (kind **Space**): the room's performance volume.
  - Origin: local (0, 0, 0), the eyes at the stop; axes the stage's.
  - Bounds: the cube x, y, z ∈ [−14.000, +14.000].
  - Named zones the works use:
    - **the car**: cylinder r 2.200, z −1.760 … +1.180 (collar to crown), and the mast r 0.170 from the crown up to the iris;
    - **the rail**: circle r 2.000 at z −0.630, open 60° at the car's west door (bearings 240° … 300°);
    - **the reach**: the band r 2.000 … 2.700, z −0.800 … +0.500 (0.8 to 2.1 m above the car floor);
    - **the part radius**: creatures keep r ≥ 3.000 from the axis (1 m beyond the rail).
  - `Hang.json` binds `sea-light` to `cube.space`.

There are no Wall, CaseBed, Plinth, Bench, Barrier or Sign anchors. Nothing is hung in the Cube.

---

## 2. Ports

The Cube has **exactly one port**.

| Key | Kind | Class | Frame (local) | Wall depth | Face | Mates with |
|---|---|---|---|---|---|---|
| `cube.shaft` | Shaft | `shaft.lift.4900.iris` | origin (0.000, 0.000, +21.500) on the mating plane; facing **up** (+z), out of the stage | 7.500 m (to the ceiling plane, +14.000) | flat (horizontal) | the Atrium's downward shaft port (proposed key `atrium.down`), served by the Atrium |

**The well** (a vertical port always carries one):
- **Section:** circle r **2.300** about the axis, less two notches 16 mm wide from r 2.265 to r 2.300 at bearings 0°
  and 180°, where the guide rails' blades stand.
  - r 2.300 is the bore of the Atrium's lining through its slab (CubePlan.h:46): the tightest bore on the car's whole
    way.
  - The car's glass is r 2.200 and its roller guides reach r 2.290 (ElanKit.h:41, 100).
- **Extent:** from the mating plane (+21.500) down to **−1.800** (museum −0.500 … −23.800), just below the car's
  lowest point at the stop.
- **Serving side: the Atrium.** It owns what moves in the well: the glass car, its roller guides, the crown, the
  spiral-band mast and its head, and the car's own lights.
- **Receiving side: the Cube.** It reserves the prism as a void and builds nothing inside it. Its shaft fit-out
  stands outside the section, exactly as built:
  - the guide rails (whose blade tips fill the notches);
  - their brackets;
  - the gilt tier frames (r 2.410 …);
  - the joint strips (r 2.438 …);
  - the sleeve's gilt line (r 2.442 …);
  - the glass and the sleeve (r 2.450 …).
- **Contacts across the seam:** the roller guides roll on the rails' blades, and the iris clamps the mast head. Both
  are declared contacts between a serving body and a receiving or joint body (R-5).
- The only thing that enters the well from outside is the port's **closure**, the iris, whose blades close across it
  onto the mast head (§6).

**The port class** (a proposed `Data/Site/PortClasses.json` record; this class is used once in the museum):

| Field | Value |
|---|---|
| key | `shaft.lift.4900.iris` |
| kind | Shaft |
| clear bore | Ø 4.600: the well, and the Atrium's lining through its slab (r 2.300). The shaft glass below it is Ø 4.900. |
| rough-opening allowance | none at the mating plane. The receiving stage's rough openings are declared by the stage: r 2.750 through its roof, r 4.130 through its ceiling panels. |
| collar | a patinated-bronze ring on the mating plane, r 2.300 … 2.454, 10 mm deep (local +21.490 … +21.500). This is the collar at the slab. |
| lining | none. The receiving stage lines its own shaft (glass, sleeve). |
| closure | **iris**: seven blades, set at the receiving stage's seal face, **7.442 m below the port frame** (top blade face, local +14.058). It is not at the mating plane (see Q-2). |
| surround at the closure | the gilt soffit ring on the receiving stage's ceiling (§3.8) |
| closure housing | the iris housing in the receiving stage's service void (§3.8) |
| interlock | the closure binds by key to the serving car and mast head (§6.3) |
| channel | the closure publishes `cube.shaft.closure` (0 shut … 1 open, clamp 0.378) |

**D-012 topology, for this stage:**
- No west doorway. The architecture draft's `elan.east ↔ cube.west` light lock is retired by D-012.
- No landing, landing door, vestibule or light lock. The car never lands; its door never opens in the Cube.
- No scoped sky: Sea Light never pushes a sky. The Sphere alone shows the sky.
- The mating line for `SitePlan.json`: `atrium.down ↔ cube.shaft`, passage 0.000 m, well served by the Atrium.

**What the Atrium must declare to mate** (for the Atrium drafter and the Site builder):
- Port `atrium.down`: kind Shaft, class `shaft.lift.4900.iris`.
- Frame at museum (54.000, 0.000, −0.500) on its slab's soffit, facing down.
- Serving the same well: circle r 2.300 less the two rail notches, from −0.500 down to −23.800.

**On the stand (L_Stand):**
- The port gets a blind that closes the shaft mouth at the mating plane.
- There is no serving mechanism on the Cube's stand, so the iris rests shut.
- Form.Sealed is checked for `cube.room` (iris shut) and for `cube.shaft` (mouth blind, iris shut).

---

## 3. Pieces by trade

Each piece is one solid of one substance, with its spec `substance.working.history`. The Cube's structure is never
seen except for the shaft's inner faces. It is still drawn true (principle 6: finish the insides).

### 3.1 Structure: the concrete box (hidden)

| Piece | Solid (local) | Spec |
|---|---|---|
| East wall | x +15.200 … +16.200, y −16.200 … +16.200, z −16.200 … +16.200 | `concrete.cast-in-situ.sealed` |
| West wall | x −16.200 … −15.200, same y, z | same |
| South wall | y +15.200 … +16.200, x −15.200 … +15.200, z −16.200 … +16.200 | same |
| North wall | y −16.200 … −15.200, same x, z | same |
| Base slab | x, y −15.200 … +15.200, z −16.200 … −15.200 | same |
| Roof slab | x, y −15.200 … +15.200, z +15.200 … +16.200, pierced by the circle r 2.750 (the sleeve's rough opening) | same |

- Walls run full height past the base and the roof (CubeStructure.cpp:111-115).
- Cast in situ, 1.000 m thick; no stock joints. Construction joints wait for polish.
- The void between the panel wall's back (±14.100) and the box's inner faces (±15.200) is 1.100 m deep. It holds the
  panels' subframe (polish, §12), the ultrasound arrays (not modelled) and the iris housing.

### 3.2 The panel wall (image-luminaire mounts)

The six inner faces are not a finish. They are **luminaires**: light-field panel modules, Light's image-luminaire kind.
The Cube declares the **mount grid**; Light places each module and draws its housing from the luminaire record (§4).

- **Face planes:** x = ±14.000, y = ±14.000, z = ±14.000, each module's face toward the origin.
- **Grid:** 0.700 m pitch, 40 × 40 positions a face, centred so that a module corner lies at each face's centre
  (20 modules each side of it).
- **Module:** face 0.698 × 0.698 m, so the joints (seams) are 2 mm. Depth 0.100 m behind the face: a design value
  until the luminaire record carries one.
- **Ceiling cut:** positions wholly inside the circle r 4.130 about the ceiling's centre are omitted (**88**).
  Positions the circle crosses are **44** modules shaped to the circle, made to order.
- **Count:** 5 × 1,600 + (1,600 − 88) = **9,512** modules. The board's 9,600 is the six faces without the opening.
- **Collision:** every module is Solid; no module is Walkable. Nobody stands in the room.
- **Old code:** CubeStructure.cpp:43-101 (faces, the round hole r 4.13 and its square transition 5.6),
  journeys.py:266-331 (the 0.7 m cell, the 2 mm seam, the ±0.06° tilt between modules).

**Re-expressed:** the old ceiling ran continuous panel quads into the hole. The modules are now whole or shaped units.
The ±0.06° set-out tilt between modules (journeys.py:299-303) belongs to the module's mounting tolerance in the
luminaire record, not to a material.

### 3.3 The ceiling's rough opening

The ceiling's rough opening is the circle r **4.130**: the soffit ring's outer edge (4.120) plus a 10 mm shadow gap.
The shaped modules' cut edges form the housing's wall at blade height (+14.000 … +14.100). The rough opening through
the roof slab is r **2.750**.

### 3.4 The shaft's glass

The glass tube runs through the ground from the box roof to the mating plane: +16.200 … +21.500, 5.300 m tall.

- **Glass:** laminated low-iron glass 60 mm thick, r **2.450 … 2.510**, `low-iron-glass.laminated.clean`. The ply
  make-up follows Matter's `laminated` working.
- **Tiers:** four, each 1.325 m, with boundaries at +21.500, +20.175, +18.850, +17.525 and +16.200.
- **Panes:** four curved panes a tier (16 in all). Each spans 90°, butting its neighbours on the diagonals behind the
  joint strips: 3.896 m of arc at the glass's mid-radius r 2.480, and 1.245 to 1.275 m tall.
- **Gilt tier frames** (as built: a slim band inside the glass, the glass standing outside it), r 2.410 … 2.450,
  `bronze.gilt.clean`:
  - top frame: +21.440 … +21.500;
  - three middle frames: 60 mm each, centred on +20.175, +18.850 and +17.525;
  - bottom frame: +16.200 … +16.260.
- **Pane edges:** each pane runs 5 mm behind the frames above and below it:
  - tier 1: +20.200 … +21.445;
  - tier 2: +18.875 … +20.150;
  - tier 3: +17.550 … +18.825;
  - tier 4: +16.255 … +17.500.

  The 50 mm gaps between tiers lie behind the frames, which close them.
- **Gilt joint strips** over the pane joints, on the diagonals (bearings 45°, 135°, 225° and 315°), `bronze.gilt.clean`:
  - 12 mm deep (r 2.438 … 2.450) and 25 mm wide;
  - each running one tier between its frames.

  The panes butt behind them.
- **Old code:** CubePlan.h:43-56, CubeStructure.cpp:152-169 (frames and strips), 215-234 (panes).

**Guide rails** (the Cube's fixed shaft fit-out, as built, CubePlan.h:67-69 and CubeStructure.cpp:187-209):

| Piece | Solid (local) | Spec |
|---|---|---|
| Rails, north and south (bearings 0° and 180°) | T-section: blade r 2.265 … 2.385, 16 mm thick; foot plate r 2.385 … 2.425, 140 mm wide. Both run from +21.480 (20 mm under the mating plane) down to +14.088 (30 mm above the top blade). | `bronze.brushed.clean` (as built: a role for the palette, D-018) |
| Rail brackets at the frames | one per rail at each tier boundary: +21.380, +20.175, +18.850, +17.525 and +16.200 (the end ones kept 0.1 m inside the rail's ends). Each block is r 2.421 … 2.454, 180 mm wide and 100 mm tall. | `bronze.patinated.dark` |
| Rail brackets on the sleeve | one per rail at +15.800 and +14.900. Each block is r 2.421 … 2.454, 180 mm wide and 100 mm tall. | `bronze.patinated.dark` |

The rails' blade tips stand in the well's notches. The car's roller guides roll on them (a declared contact, §6.4).
Where a rail's foot plate or bracket crosses a tier frame, the frame is notched round it, so the solids meet and don't
interpenetrate. As built they were surfaces crossing.

### 3.5 The cavity and its plates

The ground's cut face stands 150 mm behind the glass, at r 2.660: a lit cavity from r 2.510 to 2.660.

| Piece | Solid (local) | Spec |
|---|---|---|
| Cavity top plate | annulus r 2.510 … 2.770, z +21.490 … +21.500 | `bronze.patinated.dark` |
| Cavity bottom plate | annulus r 2.510 … 2.770, z +16.200 … +16.210 | `bronze.patinated.dark` |

Old code: CubeStructure.cpp:149-151.

**Re-expressed (solidity only):** the old plates were surfaces starting 4 mm inside the glass. As 10 mm solids they
start at the glass's outer face (2.510), so they don't interpenetrate it.

### 3.6 The sleeve (through the roof and the void)

| Piece | Solid (local) | Spec |
|---|---|---|
| Void course | cylinder shell r 2.450 … 2.750, z +14.410 … +15.200 | `bronze.patinated.dark` |
| Roof course | cylinder shell r 2.450 … 2.750, z +15.200 … +16.200 | `bronze.patinated.dark` |
| Joint line | band r 2.442 … 2.451 (standing 8 mm proud of the sleeve's face, as built), z +15.190 … +15.210, over the joint between the two courses | `bronze.gilt.clean` |

The glass's bottom ring and the cavity's bottom plate sit on the sleeve's top. Old code: CubeStructure.cpp:170-173.

### 3.7 The ground's cut face (the strata)

The cut face is a heightfield drawing on an inward-facing cylinder, grown by Nature's stratigraphic rule (§7) and made
by Form.

- **Nominal face:** r 2.660, from +16.210 to +21.490.
- **Relief:** the face stands proud (towards the axis) by the rule's relief, up to 0.05 m.
- **Back face:** r 2.860, a ground liner 0.200 m thick behind the face.
- **Mesh step:** 12.5 mm, so the flint nodules come out round rather than stepped (CubeStructure.cpp:360): a grid of
  1,337 points round by 423 down.
- **Pieces** are split by layer along the rule's contacts, sharing stations. One piece per layer:

| Layer | Between (mean, local z) | Spec |
|---|---|---|
| Crushed stone (hardcore) under the slab | +21.490 … +21.280 | `crushed-stone.compacted.dry` |
| Topsoil | +21.280 … +20.980 | `topsoil.excavated.damp` |
| Subsoil | +20.980 … +20.500 | `subsoil.excavated.damp` |
| Clay-with-flints | +20.500 … +18.000 (with the solution pipe, §7) | `clay-with-flints.excavated.damp` |
| Chalk | +18.000 … +16.210 | `chalk.excavated.dry` |
| Flint nodules (two bands in the chalk; flints in the clay) | §7 | `flint.nodular.fresh` (polish) |
| Ammonite | bearing 58°, +17.050 | `chalk.excavated.fossil-stained` |
| Heart urchin | bearing 286°, +16.780 | `chalk.excavated.fossil-stained` (polish) |

Old code: CubeStructure.cpp:238-472.

### 3.8 The closure's surround and housing (drawn by Site's joint, on the Cube's inner face)

These belong to the port class's closure (§2). They are specified here because the class is used once. Section
points are (r, z) in the local frame, closed, lathed about the axis.

**Soffit ring** (the surround), `bronze.gilt.clean`:
- Section: (2.460, 14.030) → (2.460, 14.000) → (4.116, 14.000) → (4.116, 13.996) → (4.120, 13.996) → (4.120, 14.030),
  closing back to the start. This is a 30 mm gilt annulus with a 4 mm drip at its outer edge.
- Bead at the inner edge: torus about the axis, major r 2.472, tube r 0.009, centre z 13.998.
- Bead near the outer edge: torus, major r 4.100, tube r 0.007, centre z 13.999.
- Old code: CubeStructure.cpp:178-185, CubePlan.h:58-63.

**Iris housing**, `bronze.patinated.dark`:
- Inner wall: r 2.450 … 2.460, z +14.062 … +14.400.
- Roof: annulus r 2.450 … 4.310, z +14.400 … +14.410.
- Outer wall: r 4.300 … 4.310, z +14.100 … +14.400. It stands on the panel modules' back plane; between +14.000 and
  +14.100 the shaped modules' cut edges (r 4.130) close the housing.
- Old code: CubeStructure.cpp:174-177.
- **Re-expressed:** the old outer wall ran down to the ring at r 4.30 through the panel layer (its panels had no
  depth). It now stops at the modules' back.

**The iris blades**: §6.1.

### 3.9 Fittings

None. The Cube has no bench, label, barrier or sign. The car's prompts and the work's caption are the only text
(§10).

### 3.10 What the Cube does not build

- **The Atrium builds:** what moves in the well (the car and its roller guides, the crown, the mast and its head, the
  car's lights), and the bronze lining through its own slab (r 2.300 … 2.560, museum −0.500 … −0.030).
- **Site's joint builds:** the collar ring on the mating plane, the iris blades, the soffit ring and the housing.
- **Light builds:** the panel modules' housings, from the luminaire record.

### Materials used by the Cube's own pieces

| Key | Where | Matter status tonight |
|---|---|---|
| `concrete.cast-in-situ.sealed` | the box (hidden) | **missing** substance `concrete` |
| `low-iron-glass.laminated.clean` | shaft glass, 16 curved panes | substance and working exist (r-matter) |
| `bronze.gilt.clean` | tier frames, joint strips, sleeve joint line; soffit ring and blade undersides (joint) | exist (gilt over bronze, D-014) |
| `bronze.brushed.clean` | guide rails (as built; D-018's palette may name a machined rail steel instead) | exist |
| `bronze.gilt.dusted` | the blades' top faces, seen from the shaft (joint) | history `dusted` for gilt: **missing** (bronze/gilt wear exist) |
| `bronze.patinated.dark` | cavity plates, sleeve, iris housing, blade hairline inlay | `patinated` working exists; dark regime to confirm |
| `crushed-stone.compacted.dry` | hardcore band | **missing** |
| `topsoil.excavated.damp`, `subsoil.excavated.damp` | soil bands | **missing** (and the family of a soil: Q-7) |
| `clay-with-flints.excavated.damp` | clay band | **missing** |
| `chalk.excavated.dry`, `chalk.excavated.fossil-stained` | chalk band, ammonite, urchin | **missing** |
| `flint.nodular.fresh` | nodules (polish) | **missing** |
| `low-iron-glass.lenslet-array.clean` | the panel modules' faces (Light's housing drawing; the diffuser master shades it) | working `lenslet-array` **missing** |

The fossil stain, a warm tint (linear 0.86, 0.74, 0.58) on the chalk of the fossils (journeys.py:395), is an iron
stain. It belongs to the history `fossil-stained`, never to a colour knob.

---

## 4. Light

### 4.1 Daylight

**None.** The Cube has no Glazed or Open opening to the sky. The shaft's mouth opens into the Atrium's landing tube,
so some of the Atrium's light falls down the shaft onto the shut iris when the car is away. That is light arriving
through a Port, and it stays out of the room while the iris is shut or clamped (Form.Sealed, §1). No sun ever reaches
level −1.

### 4.2 Luminaires (type, count, position; never lumens)

| Mount | Type (catalogue key) | Count | Position (local) | Aim | Control class |
|---|---|---|---|---|---|
| The panel wall | `image.light-field-panel.700`: image kind, light-field (**catalogue gap**, §Q-5) | 9,512 | the grid of §3.2 | into the room | image |
| Cavity graze lines | `linear-graze.4000k` (**catalogue gap**: nearest in the catalogue is `linear-cove.3000k`) | 32 | 4 rings × 8 | down the face | gallery |
| Mast pinspots | `pinspot.3000k.5deg` (**catalogue gap**; as built 3500 K, beam 5° outer, 2° inner) | 4 | 4 at r 2.500 | the mast | gallery |

**Cavity graze lines:**
- Radius: r 2.585, the cavity's middle.
- Heights: 50 mm under each tier frame, at +21.450, +20.125, +18.800 and +17.475.
- Bearings: 22.5° + 45°k, eight to a ring.
- Each line runs tangentially, 1.70 m long, facing down onto the cut face, with barn doors at 60°.
- They light the cut face only. As built, they are kept out of translucent lighting (CubeStructure.cpp:658-660), so
  they add no cost on the car's or the shaft's glass.

**Mast pinspots:**
- In the soffit ring's inner edge, on the diagonals (bearings 45°, 135°, 225° and 315°), face at +13.988 (12 mm
  below the ring).
- Aimed at local (0, 0, +1.780), 0.6 m above the car's crown at the stop: a throw of 12.46 m, 11.6° off the
  vertical.
- The beams end on the car. They light none of the room's faces.

**The panel luminaire record** needs (as design values, from the board and the as-built material):

| Field | Value |
|---|---|
| module | 0.700 m pitch, 0.698 m face |
| black level | 0.003 cd/m², ±15 % module to module |
| standby image | the seams' edge lines, 2 mm wide, at 2.4 cd/m² (the faint grid), shown at rest and off while a work performs |
| peak luminance | at least 60 cd/m² over a field (the paper white of the proposed *Ink*); Sea Light's points peak at 12 cd/m² at their own size |
| image | volumetric: points of light in the room where the panels' rays cross, true from any eye, fed by a work's `ILightImageSource` |
| effect on pieces | the field lights no piece in the first pass (a documented limit, §12) |
| evidence | design-stage, after Light Field Lab SolidLight modules (the board, "Feasible, not yet built") |

Sources for these values: journeys.py:287-293 and 315-316 (`PanelNits` 0.003, `SeamNits` 2.4), and cube_redesign PROPOSAL
§1 I and II.

### 4.3 Operation (Light's `Operation.json`, by channel)

- **Panels:** standby image whenever `cube.space.performing` = 0. While it is 1, the panels carry the performing work's
  image, and the seam lines are off.
- **Mast pinspots:** on only while the car floor is at or below the clamp level + 0.5 m (local ≤ +11.675), the
  closure is not open, and `cube.space.performing` = 0. They are dark during a piece (ElanElevator.cpp:673,
  CubeWay.cpp:112-118, 141-146).
- **Cavity graze lines:** on whenever the Atrium or the shaft is occupied. The museum never closes (D-006).
- Nothing is dimmed by hand. Commissioning sets the grazers and pinspots against the design targets below.

### 4.4 Targets

- **Care targets:** none. No object is hung in the Cube; Sea Light is light itself.
- **Use targets:** none on the Cube's own surfaces. No surface of the Cube is walked. The car's floor, the only place
  anyone stands, is the Atrium's piece, and its light is the car's (see Q-6 on `orientation` and `escape-route`).
- **Design targets**, as luminance order between named surfaces (measured by `Light::MeasureLuminance`, not knobs):
  1. At rest: *panel faces* (0.003 cd/m²) < *seam lines* (2.4 cd/m² at 2 mm, reading as a faint grid; at least
     1.5 px wide where they are drawn) < *gilt mast and soffit ring's inner edge under the pinspots*.
  2. At rest: the spill of every luminaire on the panel faces adds no more than 10 % of the black level
     (≤ 0.0003 cd/m²). Nothing lights the walls.
  3. During Sea Light, the room's mean luminance is 0.005 … 0.15 cd/m² (PROPOSAL §1 II).
     - A flash peaks at 12 cd/m² at its own size (SeaLight.h:122-124), with its energy spread as it widens to 1.5 px.
     - The scad's skin rim reaches 1.2 cd/m², the manta's edge sparks 9 cd/m², the rain rings 3 cd/m²
       (journeys.py:1551, 1566, 1582).
     - The panels stay at their black level with the seams off. Black at both ends.
  4. In the shaft, on the cut face under the same grazers, chalk is brighter than clay-with-flints, which is brighter
     than the soils. The order comes from albedo alone. The gilt tier frames read brighter than the glass's
     reflections of them.
- **What the Eye should then read** (an expectation for `Visitor::MeasureExposure`, not a setting): about EV100 2
  after a minute at rest, and about EV100 1 to 2 during Sea Light. The old code clamped these by local post-process
  volumes (CubeStructure.h:94-102, SeaLight.h:115-120). Those clamps are retired: the Eye adapts on its own.

---

## 5. Time and weather

- No glazed opening to the sky, so no weather place: no `museum`, `giverny` or `london` observer falls on the Cube.
- The sun never reaches level −1; no Moment term drives any Cube surface. The cut face's damp soils are a fixed
  history (`damp`), not live wetness: the ground 1 to 5 m down doesn't follow the day's rain.
- Sea Light's rain (6:00 … 7:00) is part of the work's score. It is not the museum's weather, and the work never reads
  the Moment.
- The resting state's muons (polish) arrive at a constant cosmic-ray flux, independent of the hour.

---

## 6. Motion

### 6.1 The ceiling iris (the port's closure; Site's joint makes it a Motion mechanism)

A camera's diaphragm in the horizontal plane (ElanKit.h:68-84, ElanSphereTop.cpp:46-180). Body key
`cube.shaft.iris`.

| Quantity | Value |
|---|---|
| Blades | 7, each a 110° arc of the middle circle with round ends |
| Seat (the car's way) | r 2.460 |
| Blade width | 1.250 m (ends: semicircles r 0.625) |
| Middle circle, open | r 3.095 (the blades' inner edge 10 mm outside the seat) |
| Pins | at r 3.745 (0.650 outside the middle circle), every 360/7° |
| Blade thickness | 5 mm |
| Stack | each blade 3 mm below the last: top blade's face +14.058, stack bottom +14.032 (26 mm) |
| Joint | revolute about each pin's vertical axis |
| Swing | 52° from open to shut |
| Easing | s = 3o² − 2o³ of the opening o |
| Travel time | 4.0 s from shut to open or back (ElanElevator.cpp:40) |
| Named states | **shut** o = 0; **clamp** o = 0.378, leaving r 0.197 round the axis (CubePlan.h:64-65); **open** o = 1 |
| Reach in travel | r 4.01 at most. The store rings run to r 4.10; the housing to r 4.30. |

- **Housed:** the blades move only within r 2.460 … 4.100 between +14.032 and +14.058. They lie inside the housing
  (§3.8) and above the soffit ring, which hides them open.
- **Faces:** each blade's underside (seen from the room) is `bronze.gilt.clean` and its top (seen from the shaft)
  `bronze.gilt.dusted`.
- **Hairline inlay:** two 7 mm lines along each blade's long edges, 4 mm inside the inner edge and 11 mm inside the
  outer, 0.4 mm proud, in `bronze.patinated.dark`. They make the shut iris read as a spiral.

### 6.2 The serving travel through the Cube (the Atrium's mechanism, for interlocks and clearances)

- **In the shaft:** the car runs 1.5 m/s at 0.75 m/s² on the rails (MuseePlan.h:204-205). From the Atrium floor to
  the clamp level that is about 9.2 s.
- **At the clamp level** (car floor +11.175), the mast head's groove lies in the iris plane:
  - groove centre = crown top + 0.090 m;
  - the crown top is 2.780 above the car floor (ElanKit.h:97-99);
  - the head is r 0.260; the groove is r 0.195, from 0.070 to 0.110 above the crown top.
- **On the mast:** below the clamp the car sinks on the mast its crown feeds out, at 1.0 m/s and 0.5 m/s², over
  12.775 m to the stop at −1.600: about 14.8 s (ElanElevator.cpp:44, 587-597). The mast then hangs 12.785 m from the
  iris: a spiral-band column r 0.170 with a 45 mm seam pitch.
- **At the stop:** the car's glass clears in 2.5 s (ElanElevator.cpp:42).
- **Down in all:** about 25 s, with the iris's opening and clamping.

### 6.3 Interlocks (Motion interlocks by key; Site supplies the binding across the seam)

| # | Rule | Keys |
|---|---|---|
| I-1 | The car floor may pass below the ceiling gate (+14.250) only with the iris **open** (o ≥ 0.999). | `atrium.car`, `cube.shaft.iris` |
| I-2 | The iris may leave **open** towards **clamp** only with the car floor at the clamp level ± 5 mm. | same |
| I-3 | The car may leave the clamp level downward only with the iris at **clamp** (±0.002), and upward only with the iris **open**. | same |
| I-4 | The iris shuts (o → 0) only when the car floor is at least 0.3 m above the ceiling gate (≥ +14.550) and not heading down through it. It opens ahead of a car heading down. | same |
| I-5 | The car may leave the stop only when `cube.space.performing` = 0 (the work has faded out) and its glass has come back. | `atrium.car`, channel `cube.space.performing` |

Old code: ElanElevator.cpp:539-561 and 587-588.

### 6.4 Clearances

| Pair | Clearance | Note |
|---|---|---|
| Car glass (r 2.200) ↔ iris seat and soffit ring (r 2.460) | 0.260 | |
| Roller guides (r 2.290) ↔ the well's boundary (r 2.300) and the Atrium's lining | 0.010 | as built |
| Car body ↔ tier frames (r 2.410) | 0.120 | |
| Roller guides ↔ rail blades | contact | a declared rolling contact across the seam (R-5) |
| Blade stack (+14.032 … +14.058) ↔ groove walls (0.070 … 0.110 above the crown top) | 7 mm above and below | ≥ 5 mm |
| Clamped blades (r 0.197) ↔ groove (r 0.195) | **2 mm radial** | below Motion.Clears' 5 mm; a clamp is a designed contact (R-5) |
| Top blade (+14.058) ↔ housing inner wall foot (+14.062) | 4 mm | a light path when shut (R-9) |
| Bottom blade (+14.032) ↔ soffit ring top (+14.030) | 2 mm | a light path when shut (R-9) |

### 6.5 Channels declared by this stage

| Channel | Published by | Meaning |
|---|---|---|
| `cube.shaft.closure` | the iris | the opening o (0 shut, 0.378 clamp, 1 open) |
| `cube.space.performing` | the work in `cube.space` (Art's immersive base) | 1 from Begin until its fade-out ends, else 0 |

---

## 7. Nature

- **Plants:** none. **Water bodies:** none. Sea Light's "water" is only light; it is not a water body.
- **Ground: the cut face.** It is formed by process: Nature's stratigraphic rule, a sedimentary column cut by the
  shaft. The rule, re-expressed from CubeStructure.cpp:238-355, uses seeded periodic noise round the cylinder,
  keyed by `cube.shaft.ground`. Positions are θ round the axis and z local.

| Layer | Contact below it (local z) | Surface of the cut face |
|---|---|---|
| Crushed stone | +21.280 ± 0.025 (noise 4 cycles/m) | Angular stones 12 to 28 mm in a 45 mm jittered cell, 95 % filled, standing up to 14 mm proud |
| Topsoil → subsoil | +20.980 ± 0.06 (2.5 /m) ± 0.02 (9 /m); a 6 cm colour blend | Crumbly: relief 7 mm (18 /m) and 4 mm (55 /m); a few stones 8 to 20 mm (70 mm cell, 30 %), up to 9 mm proud |
| Subsoil → clay-with-flints | +20.500 ± 0.07 (2 /m) ± 0.03 (7 /m) | as topsoil |
| Clay-with-flints → chalk | +18.000 ± 0.12 (1.6 /m) ± 0.05 (6 /m); a **solution pipe** at bearing 205° drops it by up to 0.75 m (Gaussian, 0.32 m along the arc) | Smooth, spade-cut: vertical tool marks 1.8 mm deep at a 0.11 m pitch; angular flints 15 to 45 mm (0.11 m cell, 28 %), up to 16 mm proud (polish) |
| Chalk | down to the cavity's bottom plate (+16.210) | Blocky: relief 10 mm (5 /m) and 5 mm (16 /m); bedding partings every 0.37 m, 5 mm deep and 12 mm wide |

- **Flint nodule bands in the chalk (polish):**
  - two bands, at +17.420 and +16.640, each ± 0.05;
  - nodules r 0.06 to 0.12, in a 0.24 m cell, 62 % filled, standing up to 40 mm proud as rounded knobs;
  - a white cortex a few mm thick, glassy black inside.
- **The ammonite** (Mantelliceras-like):
  - Ø 0.360 m, lying in the face at bearing 58°, centre +17.050, its coil facing the shaft, half exposed;
  - its mid-plane 4 mm proud of the face;
  - 3.4 whorls round a logarithmic spiral, expanding 2.1× a turn;
  - whorl section compressed, 0.46 × 0.30 of the whorl's radius;
  - 26 ribs a turn, 7 % high, growing in from the first whorl.
  - Old code: CubeStructure.cpp:397-437.
- **The heart urchin (polish):** Micraster, about 64 mm, bearing 286°, centre +16.780; a front notch and five faint
  petals. Old code: CubeStructure.cpp:440-472.
- **Growth space:** not applicable.

---

## 8. Art

### 8.1 *Sea Light* (the Cube's first work, D-001)

**Catalogue entry** (to add to `Data/Art/Collection.json`; it isn't in `assets/collection.json`):

| Field | Value |
|---|---|
| key | `sea-light` |
| title | *II · Sea Light* |
| after | Anna Atkins, *Photographs of British Algae: Cyanotype Impressions* (1843) |
| medium | light field, placed sound and touch. In the game: an image luminaire, placed procedural sound and controller haptics. |
| duration | 480 s (8 min) |
| made | for the Élan Cube, 2026 (the museum's own commission) |
| care class | none (no object) |
| related works in the collection | `photo-atkins-algae` (*Delesseria hypoglossum*) and `photo-atkins-algae-2` (*Halydrys siliquosa*), cyanotypes, Metropolitan Museum of Art, hung in the Hall of Light |
| caption | "A sea at night, full of dinoflagellates that flash blue when the water moves. Nothing lights up unless something moves, and the first thing that moves is you." (SeaLight.h:53-56) |

**Hang:** `sea-light` → `cube.space` (kind Space). No frame, case, plinth, cast or scan.

**The work, as built** (SeaLight.h / SeaLight.cpp; the as-built numbers win over the proposal's where they differ).
Coordinates are metres in `cube.space` (the eyes at the origin; local axes).

| Part | As built |
|---|---|
| The field | 96³ cells over a 12.0 m cube centred on the eyes (0.125 m cells). Per cell: how stirred the water is (0 … 2) and which way it moves (±2 m/s). Stirring decays with τ = 0.7 s. Each frame it is uploaded as 96 slices of 96², ten to a row. |
| The cells | 1,048,576: four turned and mirrored copies of 262,144 quads, each 4 mm. They lie within r 0.3 … 5.9 of the eyes and \|z\| ≤ 4.4, densest in the reach (r = 0.3 + 5.6 u^1.35). Each flashes from the field where it sits, above its own threshold (0.15 + 0.8 × its random + 0.9 × Thin). Colour runs from cyan-white when fresh (linear 0.45, 0.86, 1.0) to deep blue as it fades (0.01, 0.16, 1.0), about 475 nm. A cell stays at least 1.5 px wide, and grows large and soft near the eye. Only cells with a random above 0.45 streak along the water's motion (about 1/7 s of it). |
| The hand | Stirs a wake along its path (a line of splats r 0.07, 10 × its travel, carrying 0.8 × its velocity), plus a swirl of six splats on a ring r 0.11 turning at 5 rad/s about its line of motion. |
| The shoal (2:30 … 4:00) | 700 scad, 30 cm long, in a band 12 × 5 × 3 m that starts at x −13 (west) and heads along (1, −0.12, 0), east and a little north (y is south). Cruise 1.1 … 1.4 m/s, bounded to 0.7 … 1.9. Boids: alignment 2.2, cohesion 0.9, separation 0.06 within 0.3 m, neighbours by a 0.8 m hashed grid. They part round the car and never come within r 3.000, and stay between z −2.8 and +2.6. Drawn only by the light on their skins and the wake thrown back by the fish at the rear. |
| The manta (4:30 … 5:30) | 4.5 m span, 1 m/s. From (−14, 7, 2.8) along (1, −0.5, −0.03), over the car's roof; between 20 and 32 m of travel it banks 22° and turns 35° away north-east. Wingbeat period 5 s; wingtip lift 0.45 m. Seen only by the sparks its leading edges and tips stir, strongest on the downstroke. |
| The rain (6:00 … 7:00) | Up to 18 drops/s, ramping in and out over 8 s, on the surface 4.5 m above the eyes within r 6 m. Each ring spreads from 0.03 to 0.45 m (τ 0.6 s), lives 1.4 s and splashes sparks 0.15 m below it. |
| The score | 0:00 black, fading in over 2 s. 0:45 far sparks, 2 a second. 1:00 the line *Reach out and stir the water* (9 sparks a second from here). 2:30 the shoal (16 sparks a second from here). 4:30 the manta. 6:00 the rain. 7:00 … 7:58 the bloom thins, until the last sparks follow only the hand. 8:00 it ends, fading out over 3 s. It also ends if the eyes leave the room's cube. |
| Content assets | `SM_SeaLight_Cards`, `SM_SeaLight_Fish` (a 0.28 m spindle body and forked tail), `SM_SeaLight_Manta` (19-point outline, 0.35 m deep at the middle). Their content materials (cells, fish, manta, ring) live only under `/Game/Generated/Works/SeaLight/` and feed the light field's `ILightImageSource`. They are never assigned to a piece. |

**When it performs** (Art's immersive base):
- Only while the visitor's eyes are inside `cube.space`, the car is at the stop with its glass cleared, and
  `cube.shaft.closure` reports **clamp**, so the room is sealed.
- It publishes `cube.space.performing`.
- It ends, and fades out, before the car may move (I-5).

### 8.2 Not built

- The other four proposed pieces (*Ink*, *Snow*, *Murmuration*, *Brushwork*) are proposals, not approved.
- The Matterhorn and Raja Ampat journeys are retired (D-001).
- The Sphere's far works are the Sphere's.

---

## 9. Sound

**Acoustic character:**
- `cube.room` is meant to be near-anechoic: "the room is acoustically dead at rest" (PROPOSAL §1).
  - The panel wall lets sound through to the ultrasound arrays behind it, over a 1.1 m absorptive void.
  - Sound derives RT60 from the enclosure (V = 21,952 m³, S = 4,704 m²) and Matter's absorption for the panel wall.
  - The expected outcome, not a setting: well under 0.5 s at 500 Hz (for absorption 0.95, Eyring gives about 0.25 s).
- `cube.shaft` is a 4.9 m glass tube, hard and reverberant along its axis. Sound derives it too.
- Occlusion: the sealed shell. With the iris clamped the room is cut off from the shaft and the Atrium.

**Emitters:**
- **At rest:** silence. The Cube has no emitter of its own.
- **Mechanism voices** (from `FMuseeActuation` by body key):
  - `cube.shaft.iris`: seven bronze blades turning on pins over 4 s;
  - the Atrium's car on its rails and the mast feeding out of the crown (the Atrium's).
- **Sea Light's four voices** are the work's own procedural sources: 48 kHz mono, placed by Sound, HRTF-spatialised
  except the hush.

| Voice | What it is | Where | Relative gain (as built) |
|---|---|---|---|
| Hush | brown noise, low-passed again (one-pole 0.02), breathing ±25 % over 9.0 s | at the listener, not spatialised | 0.35 |
| Fizz | random ticks at 2 … 6 kHz, 1.5 ms decay; the rate rises to 900/s with the hand's speed (full at 1.5 m/s) | at the hand; inner radius 0.6 m, falloff 15 m | 0.6 |
| Shoal hiss | band noise (two one-pole filters, 0.35 and 0.08); its level follows the number of fish within 10 m of x = 0 (full at 250) | at the shoal's centre; falloff 25 m | 0.6 |
| Rain | soft ticks at 300 … 800 Hz, 12 ms decay, 40/s at full level, over a faint brown wash | at (0, 0, +4.5); inner radius 6 m, falloff 15 m | 0.6 |

Levels in dB SPL at 1 m are not in the old code: the work's record must give them (gap Q-8). Old code:
SeaLight.cpp:54-128, 171-190, 774-779, 827-832.

---

## 10. Visitor

- **Arrive.** The visitor rides the Atrium's car down: "Down through the ground to the Cube: soil, clay, then chalk,
  then the room 22 m down." The glass shaft shows the strata going by (about 9 s), the iris opens ahead, and the car
  sinks on the mast (about 15 s).
- **At the stop.**
  - The car's glass clears to its frosted floor and its bronze rail (r 2.000, 0.95 m high, open at the west door).
  - The message: "The Cube: 28 m each way, its six faces light-field panels, at rest. Its first piece: II · Sea Light."
    (ElanElevator.cpp:624).
  - The visitor stays in the car. There is no seat and no step out: the car never lands and its door stays shut here.
- **Prompts** (each owner's `IMuseeUsable`):
  - at rest: **II · Sea Light, 8 min** (the work's), then **Up to the Atrium** and **Up to the Sphere** (the car's);
  - while it plays: **End the piece** (the work's).
  - Source: ElanElevator.cpp:309-321.
- **Reach.** At 1:00 the line *Reach out and stir the water*.
  - Desktop: hold the right mouse button and move. The hand is a point 0.6 m along the look ray.
  - Headset: the right Touch controller, its tip 6 cm ahead of the grip. The controller buzzes where the sparks are
    and as the shoal's pressure wave passes: frequency 0.8, amplitude up to 0.6, from the fizz and the nearness of the
    shoal (SeaLight.cpp:835-843).
- **Look.** Anywhere: every face is 14 m away. The first minute in the black is part of the work, while the eye
  adapts.
- **Leave.** Take **Up to the Atrium** or **Up to the Sphere**. The piece fades first (3 s), then the glass comes back
  (2.5 s), then the car climbs on the mast. The iris opens only when the mast head is back on the crown.

---

## 11. The old code map

Paths are under the frozen `C:\Users\anan9\Desktop\Projects\Musee Vision\windows\`, read-only. Re-express the
geometry through Form, Matter, Light and Motion. Do not copy it.

| Part | File:lines |
|---|---|
| All the Cube's plan numbers | `Source/MuseeVision/Cube/CubePlan.h:22-83` |
| The Élan axis, heights, speeds | `Source/MuseeVision/Plan/MuseePlan.h:165-215` |
| The Cube's actor, retired knobs listed | `Source/MuseeVision/Cube/CubeStructure.h:13-151` |
| Panel faces and the ceiling hole | `Source/MuseeVision/Cube/CubeStructure.cpp:37-101` |
| Concrete box, roof hole | `Source/MuseeVision/Cube/CubeStructure.cpp:105-131` |
| Lining soffit, cavity plates, tier frames, cover strips, sleeve, housing, soffit ring and beads, rails and brackets | `Source/MuseeVision/Cube/CubeStructure.cpp:140-211` |
| Glass panes | `Source/MuseeVision/Cube/CubeStructure.cpp:215-234` |
| The strata rule (noise, stones, layers, pipe, flints, bedding) | `Source/MuseeVision/Cube/CubeStructure.cpp:238-355` |
| The cut face mesh (12.5 mm) | `Source/MuseeVision/Cube/CubeStructure.cpp:357-390` |
| Ammonite | `Source/MuseeVision/Cube/CubeStructure.cpp:392-437` |
| Heart urchin | `Source/MuseeVision/Cube/CubeStructure.cpp:439-472` |
| Material assignments (to be replaced by specs) | `Source/MuseeVision/Cube/CubeStructure.cpp:527-541, 616-634` |
| Cavity graze lines (positions, beam) | `Source/MuseeVision/Cube/CubeStructure.cpp:516-525, 636-663` |
| Retired: local exposure (RoomPost, SetRest), journey mode | `Source/MuseeVision/Cube/CubeStructure.cpp:501-514, 572-587, 665-706` |
| Iris placement, mast head profile (lathe points), column, mast pinspots, clamp level, apply | `Source/MuseeVision/Cube/CubeWay.cpp:17-21, 23-99, 101-130` |
| The iris (blade outline, pins, stack, inlay, easing) | `Source/MuseeVision/Elan/ElanSphereTop.cpp:46-180`; constants `Source/MuseeVision/Elan/ElanKit.h:68-84` |
| Car, rail, crown, head, guides | `Source/MuseeVision/Elan/ElanKit.h:40-55, 94-102` (the Atrium's) |
| Ride to the Cube: iris timing, gate, clamp stops, speeds, prompts, darkness in a piece | `Source/MuseeVision/Elan/ElanElevator.cpp:40-46, 296-321, 350-362, 505-561, 575-597, 660-676` |
| Sea Light: voices | `Source/MuseeVision/Cube/SeaLight.h:17-51`; `SeaLight.cpp:54-128` |
| Sea Light: the work's plan constants | `Source/MuseeVision/Cube/SeaLight.cpp:26-35`; `SeaLight.h:113-131` |
| Sea Light: cells, creatures | `Source/MuseeVision/Cube/SeaLight.cpp:200-300` |
| Sea Light: retired room-black post-process | `Source/MuseeVision/Cube/SeaLight.cpp:302-333` |
| Sea Light: begin, end, field splats, decay, upload | `Source/MuseeVision/Cube/SeaLight.cpp:335-464` |
| Sea Light: hand | `Source/MuseeVision/Cube/SeaLight.cpp:466-527` |
| Sea Light: shoal | `Source/MuseeVision/Cube/SeaLight.cpp:529-677` |
| Sea Light: manta | `Source/MuseeVision/Cube/SeaLight.cpp:679-733` |
| Sea Light: rain | `Source/MuseeVision/Cube/SeaLight.cpp:735-772` |
| Sea Light: score, placed sound, haptics | `Source/MuseeVision/Cube/SeaLight.cpp:774-845` |
| Sea Light: tick, fade, end on leaving, cell parameters | `Source/MuseeVision/Cube/SeaLight.cpp:847-934` |
| Sea Light: meshes (cards, scad, manta) | `Source/MuseeVision/Cube/JourneyGen.cpp:655-807` |
| Panel material: cell, seam, rest luminance, tilt | `Scripts/journeys.py:266-331` |
| Strata material (layer colours, fossil stain, damp soil) | `Scripts/journeys.py:331-407` |
| Mast material (45 mm spiral seam) | `Scripts/journeys.py:410-433` |
| Sea Light content materials | `Scripts/journeys.py:1370-1594` |
| Resting state, muons (polish) | `Source/MuseeVision/Cube/CubeRest.h:10-65`; `CubeRest.cpp:15-39, 87-132, 160-198`; `Scripts/journeys.py:436-472` |
| Board: section, dimensions, texts | `plan/canvas/ElanCube.dc.html`; `plan/boards/ElanCube.png`; `plan/README.md` ("Élan, level −1") |
| Design intent of the piece and the rest state | cube_redesign `PROPOSAL.md` §1 (II, At rest), §4, §5 (a scratchpad copy; the approved work per D-001) |

**Retired and not to be rebuilt:**
- `ElanJourney.*`, `Journey*.*`, `MatterhornData.inl`, `JourneySea*.*`;
- MPC_Cube's `Reveal`, `IceGlow`, `WaterLevel`, `WaterOn`, `SunNits` and `Plankton`;
- every per-room exposure or bloom override;
- the Square.

---

## 12. First-pass cut

**Built tonight, in this order:**
1. **The stage's declaration:**
   - port `cube.shaft` with its receiving well;
   - the footprint's two prisms;
   - enclosures `cube.room`, `cube.shaft` and `cube.shaft.cavity`;
   - anchor `cube.space`;
   - channels `cube.shaft.closure` and `cube.space.performing`.
2. **The shaft's structure and fit-out:**
   - the box (§3.1), the sleeve (§3.6) and the cavity plates (§3.5);
   - the shaft glass with its gilt tier frames and joint strips, and the guide rails with their brackets (§3.4).
3. **The cut face as five layer pieces** (crushed stone, topsoil, subsoil, clay-with-flints, chalk), with the rule's
   wavy contacts, the solution pipe, the chalk's bedding partings, the clay's spade marks, and **the ammonite**.
4. **The panel wall:** the 9,512 mounts, with the modules drawn from the luminaire record and shown in their standby
   image (black level and the faint seam grid).
5. **The closure:**
   - the iris with its states shut, clamp and open;
   - the soffit ring and beads, and the housing, via the joint;
   - interlocks I-1 … I-5.
6. **Light:** the 32 graze lines and the 4 mast pinspots, under Operation; the design targets measured.
7. ***Sea Light*:**
   - the field, the 1,048,576 cells, the hand (desktop and headset), the shoal, the manta, the rain and the score;
   - the four placed voices and the headset haptics;
   - performing only while sealed, publishing `cube.space.performing`.
8. **Checks:**
   - Form.Sealed on `cube.room` with the iris shut and clamped;
   - Motion.Housed and Motion.Interlocked;
   - Light.MeetsTarget on the design targets;
   - Site.RoomStaysInside;
   - Sea Light within 4 ms on the 4090 at 4K with DLSS Quality (PROPOSAL §5 step 9).

**Waits for polish:**
- **The resting state's muon tracks.** A room has no play-time code, so they need a home (Q-4). The room at rest is
  black with the faint grid tonight.
- **The rest of the geology:** flint nodule bands, the flints in the clay, the heart urchin, the crushed stone's
  individual stones beyond the rule's relief.
- **Hidden structure:** the panels' steel subframe in the void, the box's construction joints, the ultrasound arrays.
- **The iris's light seals** (brush or labyrinth) at the 2 and 4 mm slots, if the leak probe finds them (R-9).
- **Light:** light-field emission lighting nearby pieces (the hand, the rail, the frosted floor), and switching
  Lumen's GI off in the black room during a piece (a museum-wide transport profile, Light's).
- **Touch:** ultrasound beyond controller haptics.
- **Sea Light's headset scale** (counts halved and field at quarter resolution, PROPOSAL §4) and its perf tuning.

---

## Open questions and risks (for the Architect)

**Q-1 · The mating plane.**
- This sheet puts `cube.shaft` on the Atrium slab's soffit (museum z −0.500), with the Cube owning the shaft walls
  through the earth.
- This follows the as-built code (the shaft lives in `ACubeStructure`), the `ElanCube` board ("glass shaft: soil,
  clay, chalk") and the architecture's "the Cube's receiving shaft".
- The Atrium's port must sit on the same plane with the same class. If the Atrium's spec puts its port at z 0.000,
  the mating fails by 0.5 m.

**Q-2 · Where the iris sits.**
- D-012 makes the iris the port's closure class. This sheet has Site's joint place it at the port's declared seal
  depth (7.442 m below the port frame), not on the mating plane.
- If Site can't place a closure away from the mating plane tonight, the fallback is that the Cube describes the iris,
  soffit ring and housing as its own mechanism, with Site supplying only the interlock binding.
- One of the two must be chosen before the Site and Cube builders start.

**Q-3 · The split at the well.**
- The Atrium serves the car, roller guides, crown, mast and head.
- The Cube keeps the rails, brackets, frames, strips, glass and sleeve, as the old code did.
- The mast and head lived in `CubeWay` (the Cube's folder) but are driven by the car's crown, so they are the
  Atrium's now. If the Atrium's spec omits them, the car can't reach the centre.

**Q-4 · The resting state (muons).**
- It is part of the as-built Cube (CubeRest, spawned by the elevator), but a room has no play-time code.
- Proposal: a second small work bound to `cube.space` while no other performs, or a standby image source that Light
  owns.

**Q-5 · The light-field panel isn't a real product** at room scale (the board: "feasible, not yet built").
- Light.LuminairesFromCatalogue needs a design-stage record for `image.light-field-panel.700`.
- Light's image kind must render **volumetric** points (a light field), not only a flat image.
- If only flat image luminaires land tonight, *Sea Light* cannot be lawful and waits.

**Q-6 · Use classes against the designed black.**
- `UseTargets.json` names the Cube's sealed shell under `orientation` (5 lx), and `escape-route` sets 10.76 lx on
  every walked route.
- During *Sea Light* the car floor (the Atrium's) reads about 0 lx, by design.
- Proposal: the car is the lift and the only egress, so it is exempt while the work performs, as in a planetarium.
  Or the car carries a low floor-edge orientation light. The owner decides.

**Q-7 · Matter gaps.**
- Missing substances: concrete, chalk, flint, clay-with-flints, topsoil, subsoil, crushed stone.
- Missing working: `lenslet-array`. Missing histories: `dusted` for gilt and `fossil-stained` for chalk.
- The family of soils (stone? living?) must be declared, or Matter.OneLanguage fails.

**Q-8 · Sea Light's voices have relative gains only.** The work's record needs SPL at 1 m before Sound can place
them at real levels.

**Q-9 · The ground under the museum (D-018).**
- The as-built cut face is southern English geology: clay-with-flints over the Upper Chalk, with flint bands, a
  Cenomanian ammonite and a Micraster urchin.
- The museum stands in Beijing (D-002, default), whose top 6 m are Quaternary alluvium and loess. The New York option
  would be glacial till over Manhattan schist.
- D-017 keeps the as-built strata tonight. The material story should decide whether the Cube's ground stays the
  Chalk, as a chosen, told story, or becomes the ground the museum truly stands on.

**R-5 · Designed contacts across the seam.** Motion.Clears (5 mm) would fail two things that are meant to touch:
- the clamped blades leave 2 mm round the mast head's groove;
- the car's roller guides roll on the Cube's rail blades.

Motion needs **declared contact pairs** (a clamp and a rolling guide), checked for no interpenetration instead of for
clearance, with Site binding the keys across the seam.

**R-9 · Light leaks at the iris.** The 2 mm slot under the stack and the 4 mm slot over it are light paths from the lit
shaft into the room's 0.003 cd/m² black when the iris is shut. The leak probe will likely find them. A real iris of
this kind carries a brush seal.

**R-10 · Emissive content.** Sea Light's content materials are additive and unlit. They must stay under
`/Game/Generated/Works/SeaLight/` and feed only the image source, or Matter.NoEmission and Matter.OneLanguage will
(rightly) fail them.

**R-11 · Stacked footprints.** The Cube lies under the Atrium in plan (and 1.4 m under the Hall of Light's east end).
Site.NoOverlap must compare volumes, not plan outlines. So must the Atrium–Sphere stack.

**R-12 · Exposure.** The old build held the Cube's exposure with local post-process clamps (EV 2 at rest, EV 1 in the
piece). Without them, the Eye's own metering must reach a dark-adapted state in about a minute. Otherwise the black
room reads grey and noisy, and the sparks bloom out.

**R-13 · Cost.**
- About 1 M additive cells, a 567 k-vertex cut face and 9.5 k instanced modules.
- Sparks under DLSS smear unless they keep at least 1.5 px, write velocity and use responsive anti-aliasing (as
  built).
- Budget: 4 ms for the piece.
