# The Élan Atrium · stage spec

**Stage key:** `atrium` · **module:** `MuseeAtrium` (Editor, `Source/Stages/Rooms/Atrium`) · **display name:** *The
Atrium* (FText) · **level:** 0 · **schedules:** `Data/Stages/Atrium/Finishes.json`, `Lighting.json`, `Hang.json`
(D-017).

This is the design sheet the Room Builder builds from tonight (D-016). Every number comes from the as-built museum:
- the frozen `windows/` code (`Atrium/`, `Elan/`, `ElanExterior/`, `Plan/MuseePlan.h`);
- the boards `FutureAtrium`, `AsBuiltElan` and `ElanSphere`;
- `plan/README.md`.

D-001 makes that the design. Where this sheet changes the old geometry to satisfy a rule of the new architecture
(solidity, stock, a real mechanism, a real light source), the line starts **Re-expressed:**. Open questions for the
Architect are collected at the end.

**Decisions applied:**
- D-001: the design is the as-built museum. The Atrium keeps its as-built shape. The journeys are retired.
- D-003: the museum's sun is the idealised one.
- D-005: the drum's weather is the museum's own.
- D-006: always open; the light is controlled the way a gallery's is.
- D-012: the Atrium's ports are its west door and its two vertical ports, which it serves; iris closures; the car never
  lands in the Cube; no scoped sky.
- D-014: glazing, design targets as luminance order, no automatic blinds, passages, the occupancy guard.
- D-017: port, don't redesign; choices live in schedules.
- D-018: materials are chosen. Material keys below name **roles**; the palette (`Docs/Matter/Story.md`, not yet
  written) names the real material behind each one.

**Sibling sheets this one mates with:**
- `cube.md`: this sheet adopts its seam and class exactly (§2).
- `sphere.md`: not yet written. §1 fixes the seam it must respect.
- `grounds.md`: its terrace edge r 15.32 is 2 cm off this socle.

**Units and frame.** Metres. The stage's local frame: x east, y south, z up, true north. Plan directions are compass
**bearings**, clockwise from north. A point at bearing b and radius r is (r sin b, −r cos b). The old code measures
*plan angles* from east towards south: bearing = plan angle + 90°.

---

## 1. Identity and placement

**What it is.** A drum 28 m across, the Future's hall:
- travertine to 6 m and misty glass above it to the equator at 22 m;
- overhead, the glazed and glowing underside of the Sphere, on a triangulated pearl-steel grid, carried by twelve pearl
  ribs that rise from bronze shoes on the floor, turn at 9 m and run under the Sphere to a ring round the car's way;
- a floor of cream marble polished to a mirror;
- one door, west, from the Hall of Light;
- eleven bays, A to K, for new art;
- at the centre, a round glass car in a glass landing, which joins the Cube 22 m below to the Sphere above;
- beside the car's door, *The Starry Night* on a glass stele.

**Origin.** The Élan axis at the Atrium's finished floor: the centre of the car's way, z 0.000 on the marble's face.

**Placement in the museum plan** (x east, y south from the Rotunda's centre, z from the finished-floor datum): the
origin sits at **(54.000, 0.000, 0.000)**, level 0. The Atrium floor, the Rotunda floor, the Hall of Light floor and the
ground outside are all at z 0. In this frame the Sphere's centre is (0, 0, +22.000) and the Cube's centre is
(0, 0, −22.000).

**Key levels** (local z = museum z):

| What | z | Source |
|---|---|---|
| Car floor at the Cube's stop / the down well's bottom | −23.600 / −23.800 | CubePlan.h:41; cube.md §2 |
| Car floor at the clamp level (the mast head in the Cube's iris) | −10.825 | CubeWay.cpp:17-21 |
| Joint's collar ring under the mating plane | −0.510 … −0.500 | cube.md §2 |
| **Slab soffit: the mating plane of `atrium.down`** | −0.500 | CubePlan.h:45 |
| Lining's top, under the marble | −0.030 | AtriumBaseStructure.cpp:226-237 |
| **Finished floor** (marble face) | 0.000 | |
| Cove soffit | 0.140 | AtriumBaseStructure.cpp:47 |
| Rib shoes' top | 0.360 | ElanStructure.cpp:445 |
| *The Starry Night*'s centre | 1.380 | Elan.swift:32 |
| Stele glass top | 2.200 | MuseePlan.h:339 |
| Landing head ring top / tube starts | 3.000 | ElanElevator.cpp:41 |
| West door head | 4.500 | MuseePlan.h:171 |
| Tube's ring frame (centre) | 5.358 | ElanCar.cpp:318-330 (2 tiers) |
| Coping band soffit | 5.720 | AtriumBaseStructure.cpp:55 |
| **Wall top: the drum's glass stands on it** | 6.000 | MuseePlan.h:170 |
| Exterior coping wash's inner edge; the order's seats | 6.200 | ElanExteriorStructure.cpp:360 |
| Collar soffit | 7.455 | ElanCar.cpp:341-372 |
| Collar iris: top blade's top face (the seal) | 7.695 | ElanSphereTop.cpp:249 |
| **Collar top: the mating plane of `atrium.up`** | 7.715 | ElanKit.cpp:21 (CollarTop) |
| Compression ring soffit at r 3.000 | 7.745 | ElanStructure.cpp:498-503 |
| Car floor at the neck stop (home, in the Sphere's well) | 8.400 | ElanKit.h:86 |
| Ribs leave the wall and turn | 9.003 | derived, §3.5 |
| Portal top | 9.500 | MuseePlan.h:255 |
| Transoms | 10.000, 14.000, 18.000 | MuseePlan.h:237 |
| Car floor at the Sphere's stop | 20.400 | MuseePlan.h:203 |
| Entablature soffit / outer leaf's head | 20.900 / 20.950 | MuseePlan.h:239 |
| Canopy meets the drum's inner leaf | 21.180 | derived, §3.4 |
| Equator (the Sphere's centre) | 22.000 | MuseePlan.h:200 |
| Entablature top (the wash's inner edge) | 22.580 | MuseePlan.h:239 |
| Up well's top | 23.390 | §2 |

**Footprint** (kind **Building**): stacked prisms, because the portal arches over the Hall of Light's end, the Cube
lies below and the Sphere above (see cube.md R-11). The **west sector** is bearings 248° … 292°: ±22° about west, where
the portal takes the plinth.

| Prism | z | Plan |
|---|---|---|
| P1 | −0.500 … 6.200 | Outside the west sector, the disc r 15.400 (the corona; the socle is r 15.300). Inside it, the disc r 14.800, plus the portal: x from −15.000 to the disc, \|y\| ≤ 6.050. Minus the Hall of Light's notch: \|y\| < 4.950, west of the base face (x < −√(14.800² − y²)), below the portal's arch soffit (z < 2.200 + √(5.800² − y²); 8.000 at the crown, 5.221 at the jambs). |
| P2 | 6.200 … 9.500 | Outside ±25° about west, the disc r 15.600 (fins and pedestals). Inside it, the disc r 14.080, the portal block and the same notch. |
| P3 | 9.500 … 22.580 | The disc r 15.700 (the entablature's corona oversails everything, including the Hall of Light's end and the terrace). |
| Wells | | Served by this stage: §2. |

In the museum plan: the disc about (54, 0), and the portal's front at x 39.000, \|y\| ≤ 6.050.

**The seams with the neighbours:**
- **Atrium | Sphere.**
  1. Below the equator and outside the neck column, the seam is the sphere of radius **14.000** about (0, 0, 22.000).
     The Atrium builds nothing inside it; the Sphere builds nothing outside it. The canopy's upper face lies on it.
  2. The **neck column** is the cylinder r 3.000 from the mating plane z 7.715 up to that sphere (z 8.323). Inside it is
     the Sphere's throat; outside it is the Atrium's ring. The Atrium's well contents pass through it.
  3. Above the equator, everything inside r 14.080, and everything above the entablature's wash, is the Sphere's. Its
     dome shell rises from the wash.
- **Atrium | Cube.** The horizontal plane z −0.500 (the slab's soffit). The Cube owns the shaft through the ground
  below it (cube.md Q-1).
- **Atrium | Hall of Light.** The base's outer face r 14.800 between the jambs: the doorway port. Round it, the Hall's
  end stands inside the portal's opening (the notch above).
- **Atrium | Grounds.** The ground plane z 0 outside the socle. The terrace starts at r 15.32.

**Enclosures:**

| Key | Kind | Bounds (local) | Openings |
|---|---|---|---|
| `atrium.hall` | **Enclosed** | Floor z 0.000 (into the cove to r 14.090). Base inner face r 14.000 to z 6.000. Drum inner leaf r 14.000 from 6.000 to 21.180. The canopy's room face (sphere R 14.024 about (0, 0, 22)) from 21.180 in to the ring at r 3.600. The ring's underside and bore to r 3.000 at z 7.745. The collar's top, hub (r 2.465) and soffit. The lining's bore (r 2.300) from the floor down to z −0.500. | `atrium.west`, `atrium.down`, `atrium.up` (Ports); `atrium.drum` (Glazed); `atrium.cove`, `atrium.collar.pocket` (Housings) |

- The shell is sealed whenever the collar iris is shut and the down port's blind (or the Cube's iris) is closed.
- The landing enclosure, the car and the stele stand inside `atrium.hall`. None of them is an enclosure.
- The drum's cavity (between the leaves) and the pocket behind the canopy's edge are parts of the glazing make-up, not
  enclosures.

**Openings** (Form's one list):

| Key | Kind | Geometry (local) |
|---|---|---|
| `atrium.west` | Port | The rough opening for the 4.000 × 4.500 clear doorway (square head), jambs on the planes y = ±2.000, through the wall from r 14.000 to r 14.800, plus the class allowance. |
| `atrium.down` | Port | The circle r 2.300 on the mating plane z −0.500. Above it the lining's bore (r 2.300) rises through the slab to the floor's lip (r 2.290, z −0.030 … 0.000). |
| `atrium.up` | Port | The circle r 3.000 (the ring's bore) on the plane z 7.715. The collar's hub bore (r 2.465) is the rough opening below it. |
| `atrium.drum` | Glazed | The drum: the inner leaf (r 14.000 … 14.024, z 6.000 … 21.180) and the outer leaf (r 14.050 … 14.074, z 6.000 … 20.950). Weather observer **museum**. No daylight control (D-014). |
| `atrium.cove` | Housing | The cove recess r 14.000 … 14.090, z 0.000 … 0.140, all round but the door bay (§3.2). |
| `atrium.collar.pocket` | Housing | The collar iris's pocket, r 2.465 … 4.050, z 7.660 … 7.705 (§3.6). |

The canopy is **not** an opening. It is a closed glass shell whose panes are the diffusers of a luminaire (§4). No
daylight reaches it from above: the Sphere is opaque (ElanExteriorStructure.h:52-59).

**Anchors:**
- `atrium.stele` (**Wall**, on Art's glass-stele fitting): origin (−3.840, −1.900, 1.380) on the stele glass's west face,
  facing west. §8.
- `atrium.bay.a` … `atrium.bay.k` (**Space**, empty tonight): one per art bay, bounded by the two rib shoes, the base's
  inner face, r 9.000 and z 6.000. The bays are centred on bearings 300° (A), 330° (B), 0° (C), 30° (D), 60° (E),
  90° (F, on the east axis, seen through the car), 120° (G), 150° (H), 180° (I), 210° (J) and 240° (K). The route runs
  clockwise, A to K.
- `atrium.call` (the place to call the car): the waiting ring (r 2.940 … 3.060) and the mark west of it (§3.3).
- **Sign** anchors `atrium.letter.a` … `.k`: the bronze bay letters in the floor at r 13.100 (§3.3; polish).
- No Bench, Barrier or CaseBed. As built, the Atrium has no seats.

---

## 2. Ports

| Key | Kind | Class | Frame (local) | Wall depth | Face | Mates with |
|---|---|---|---|---|---|---|
| `atrium.west` | Doorway | `doorway.4000x4500.square` | origin (−14.800, 0.000, 0.000), facing **west** (−1, 0, 0), z up | 0.800 on the axis (0.808 at the jambs) | **cylindrical, R 14.800** about the stage's z axis (convex outward) | `hall-of-light.east`, passage 0.000 |
| `atrium.down` | Shaft | `shaft.lift.4900.iris` | origin (0.000, 0.000, −0.500) on the slab's soffit, facing **down** (0, 0, −1) | 0.500 (the slab) | flat (horizontal) | `cube.shaft`, passage 0.000; **served by the Atrium** |
| `atrium.up` | Shaft | `shaft.lift.4900.iris` | origin (0.000, 0.000, 7.715) on the collar's top, facing **up** (0, 0, +1) | 0.260 (the collar) | flat (horizontal) | the Sphere's downward shaft port (proposed `sphere.shaft`), passage 0.000; **served by the Atrium** |

Museum positions: `atrium.west` (39.200, 0, 0); `atrium.down` (54, 0, −0.500); `atrium.up` (54, 0, 7.715).

**The wells** (both served by the Atrium; section identical to the Cube's, cube.md §2):
- **Section:** the circle r **2.300** about the axis, less two notches 16 mm wide (tangential ±0.008) from r 2.265 to
  r 2.300 at bearings 0° and 180°. The receiving side's rail blades stand in the notches.
- **`atrium.down`'s well:** from z −0.500 down to **−23.800** (the car's lowest point at the Cube's stop is −23.760).
  The Cube reserves it.
- **`atrium.up`'s well:** from z 7.715 up to **+23.390** (the mast head's top at the Sphere's stop is 20.400 + 2.940 =
  23.340). The Sphere reserves it.
- **What the Atrium owns in the wells** (the serving side owns what moves):
  - the glass car, its leaves, roller guides and light;
  - the **crown** and the **Cube mast**: the spiral-band column the crown feeds out, and its head;
  - the **under-floor drum** and the **Sphere mast**: the column the car stands on above the neck (§3.6, §6).
  This settles cube.md Q-3.
- **What the receiving sides own**, outside the section:
  - the Cube: its shaft glass, sleeve, rails, frames and strata;
  - the Sphere: its throat (r 2.550 … 2.980, as built), the rails in its neck if it keeps them, its floor ring and
    platform.

**Port classes:**
- **`doorway.4000x4500.square`** (proposed for `Data/Site/PortClasses.json`; used once):
  - clear opening 4.000 wide × 4.500 high, square-headed, jambs parallel;
  - closure: **open portal**, no leaves (as built: the Atrium's only door is always open);
  - the joint (Site) builds the museum's one doorway detail: a travertine threshold through both wall depths, the
    brushed-bronze strip on the mating face, the travertine lining in front of both rough faces, and surrounds;
  - on the Atrium's side the mating face is the cylinder R 14.800. The Hall of Light's end must meet the same cylinder,
    or be flat at x −14.800 with the joint's passage masonry closing the wedge (at most 0.136 m at the jambs:
    14.800 − √(14.800² − 2.000²)).
- **`shaft.lift.4900.iris`**: the Cube's record (cube.md §2), unchanged. This sheet uses it for **both** vertical ports
  and adds:
  - **Seal plane per port.** The iris closure sits at the *receiving* stage's declared seal face:
    - Cube: top blade face 7.442 m below the port frame (museum −7.942);
    - Sphere: its neck-mouth iris, top blade face 3.681 m above the port frame (museum 11.396; ElanSphereTop.cpp:252).
    Site must place a closure at a declared depth from the mating plane (cube.md Q-2).
  - **Collar ring.** The joint's patinated-bronze collar ring r 2.300 … 2.454, 10 mm deep, lies on the receiving side
    of each mating plane: −0.510 … −0.500 below the Atrium, and 7.715 … 7.725 above it.
  - **Collar envelope at `atrium.up`:** r 3.000. The joint closes the 20 mm slot between the Sphere's throat
    (r 2.980) and the ring's bore (r 3.000) on the mating plane with a gilt band, the throat's first course line.

**D-012 topology, for this stage:**
- `hall-of-light.east ↔ atrium.west`: the Atrium's only door.
- `atrium.down ↔ cube.shaft` and `atrium.up ↔ sphere.shaft`: the Atrium serves both wells.
- The glass car never lands in the Cube: it has no landing, landing door, vestibule or light lock there.
- The Atrium's own landing enclosure is the car's one landing (as built). At the Sphere the car stops in the platform's
  opening, which is the Sphere's.
- No stage pushes a sky. The old building-hiding in the Sphere (`MuseeWorld::SetBuildingHidden`) is retired.
- The architecture draft's `elan.shaft ↔ cube.shaft` and `elan.east ↔ cube.west` (light lock) are superseded by D-012.

**On the stand (L_Stand):**
- `atrium.west` gets a blind.
- `atrium.down` gets a blind at −0.500 and a blind well down to −23.800.
- `atrium.up` gets a blind at 7.715 and a blind well up to +23.390.
- The car runs its full 44.000 m alone. The blind wells must carry guide rails like the receiving shafts', or the car's
  guides run free there (see risk R-6).
- Form.Sealed is checked for `atrium.hall` with the collar iris shut and the car at each stop.

---

## 3. Pieces by trade

Each piece is one solid of one substance, with its role key `substance.working.history` (the table at the end of this
section is `Finishes.json`).

### 3.1 Structure (hidden)

| Piece | Solid (local) | Key |
|---|---|---|
| Floor slab | disc r 2.560 … 14.090, z −0.500 … −0.030 (under the marble), and the door zone out to the wall's inner face | `concrete.cast-in-situ.sealed` (**missing** substance, as the Cube's) |
| Wall footing | the base wall continues down to z −0.500 on the slab's edge | `travertine.honed.clean` (the wall's own stone) |

### 3.2 Masonry: the stone base, the exterior dress, the entablature, the portal

**The stone base wall** (`travertine.honed.clean` inside):
- Annulus: inner face r **14.000**, core outer face r **14.800**, z −0.500 … 6.000.
- Outside the west sector, travertine ashlar dresses the outer face out to r **14.880** (below). Inside the west sector
  the face stays r 14.800: the Hall of Light's end and the portal meet it.
- **Re-expressed:** the as-built base showed no joints. It is now laid as real ashlar by Form's coursing within Matter's
  travertine stock. The vertical joints fall on the rib lines (hidden behind the ribs) and on the bay centres.
- **West door rough opening:** jambs on y = ±2.000 plus the class allowance, head level at z 4.500 plus the allowance,
  through the full wall. The joint's lining returns it to 4.000 × 4.500 clear.
- **Cove** (the Housing `atrium.cove`): a recess 90 mm deep (r 14.000 … 14.090) and 140 mm high (z 0.000 … 0.140) at
  the wall's foot. It runs all round but the door bay and stops with square ends where the inner face is 0.250 m short
  of each jamb (y = ±2.250, bearings 270° ± 9.25°). The marble runs into it to r 14.090. Its luminaire is §4.
- **Coping band** (inside, at the wall's top), lathed between the ribs. Section (r, z):
  (14.000, 5.720) → (13.900, 5.720) → (13.900, 5.970) → (13.930, 6.000) → (14.000, 6.000).
  It is 100 mm proud of the wall with a 30 mm chamfer on its top arris, and stops 5 mm short of each rib's side faces
  (square returns). **Re-expressed:** as built it ran unbroken, crossing through the ribs.
- **Wall top** at z 6.000 from r 13.930 out to r 14.880. The drum's two glass leaves stand on it (§3.4), and the
  exterior coping cornice sits on its outer part from r 14.080.
- Old code: AtriumBaseStructure.cpp:45-78, 272-472. The pilasters (474-530) are retired: `bPilasters` was false as
  built.

**The exterior dress** (`travertine.honed.weathered`). The plinth's base moulding, ashlar face and coping cornice are
lathed about the axis outside the west sector (from bearing 292° clockwise round to 248°). Section (r, z), in order;
arcs run from the named start angle to the named end angle (0° = +r, 90° = +z):
1. (14.880, −0.100) → (15.300, −0.100) → (15.300, 0.400) → (15.260, 0.440) → (15.140, 0.440)
2. **torus**: arc about (15.140, 0.540), R 0.100, from −90° to +90° → (15.140, 0.640)
3. (15.080, 0.640) → (15.080, 0.680)
4. **scotia**: arc about (15.080, 0.860), R 0.180, from −90° to −180° → (14.900, 0.860)
5. (14.880, 0.880) → (14.880, 5.200): the ashlar face
6. (14.940, 5.200) → (14.940, 5.280)
7. **ovolo**: arc about (14.940, 5.480), R 0.200, from −90° to 0° → (15.140, 5.480)
8. (15.400, 5.480) → (15.400, 5.860): the corona
9. (15.360, 5.860) → (15.360, 5.890)
10. **bead**: arc about (15.240, 5.890), R 0.120, from 0° to 90° → (15.240, 6.010)
11. (15.200, 6.010) → (15.200, 6.050)
12. **wash** to (14.080, 6.200) → (14.080, 6.000), then along the wall's top back to the face.

- **Re-expressed:** the wash's inner edge moves from r 14.060 to **14.080**, 6 mm clear of the outer leaf's face
  (r 14.074). As built it cut through the outer pane.
- Every vertical of the drum's order stands on a **level seat cut into the wash at z 6.200** (±0.280 tangential at the
  fins, ±0.070 at the mullions). **Re-expressed:** as built the pedestals sank into the sloping wash.
- Old code: ElanExteriorStructure.cpp:346-364.

**The entablature at the equator** (`travertine.honed.weathered`), lathed all round. Section (r, z):
1. (14.080, 20.900) → (14.620, 20.900) → (14.620, 21.100) → (14.700, 21.100) → (14.700, 21.340): architrave, first fascia
2. (14.760, 21.340) → (14.760, 21.400) → (14.660, 21.400): taenia
3. (14.660, 21.840): frieze → (14.700, 21.840)
4. **ovolo**: arc about (14.700, 22.050), R 0.210, from −90° to 0° → (14.910, 22.050)
5. (15.700, 22.050) → (15.700, 22.300): corona
6. (15.660, 22.300) → (15.660, 22.330)
7. **bead**: arc about (15.520, 22.330), R 0.140, from 0° to 90° → (15.520, 22.470)
8. (15.480, 22.470) → (15.480, 22.500)
9. **wash** to (14.080, 22.580) → back down the inner face to (14.080, 20.900)

- **Re-expressed:** the inner face moves from r 14.060 to 14.080. The architrave, frieze and ovolo are **notched round
  each fin** (±0.170 tangential at bearings 15° + 30°k, from 20.900 to the corona's soffit 22.050): the fins are the
  brackets that carry the corona. As built they crossed.
- Old code: ElanExteriorStructure.cpp:366-377; MuseePlan.h:238-239.

**The portal** (`travertine.honed.weathered`): the frontispiece that takes the Hall of Light's glass section into the
plinth, after the Pantheon's intermediate block.
- **Block:** front face the plane x = −15.000, \|y\| ≤ 6.050, z −0.100 … 9.500. Its back meets the cylinder r 14.050
  above the wall (z > 6.000) and the base's face r 14.800 below.
- **Opening:** jambs on y = ±4.950 from −0.100 up to 5.221, then an arch R 5.800 about (y 0, z 2.200), crown 8.000. It
  runs straight through the block to the drum and base faces.
- **Clearances to the Hall of Light:** 0.150 m to its planter kerbs (\|y\| 4.8) and 0.500 m over its vault's crown
  (7.5).
- **Socle** round each pier's front, as the plinth's base moulding. Section (a out from the face, z): (−0.020, −0.100) →
  (0.420, −0.100) → (0.420, 0.400) → (0.380, 0.440) → (0.260, 0.440) → torus about (0.260, 0.540), R 0.100, −90° → 90° →
  (0.200, 0.640) → (0.200, 0.680) → scotia about (0.200, 0.860), R 0.180, −90° → −180° → (0.000, 0.880) →
  (−0.020, 0.860).
  - North path: (−13.300, −6.050) → (−15.000, −6.050) → (−15.000, −4.980).
  - The south path mirrors it.
- **Cornice** (T = 9.500). Section (a, z): (0, 8.750) → (0.050, 8.750) → (0.050, 8.820) → ovolo about (0.050, 9.000),
  R 0.180, −90° → 0° → (0.450, 9.000) → (0.450, 9.320) → (0.410, 9.320) → (0.410, 9.350) → bead about (0.300, 9.350),
  R 0.110, 0° → 90° → (0.260, 9.460) → (0.260, 9.500) → (0, 9.500) → (−0.020, 9.480) → (−0.020, 8.770).
  - Path: (−12.680, −6.050) → (−15.000, −6.050) → (−15.000, +6.050) → (−12.680, +6.050), mitred.
- **Architrave** round the opening (polish). Section (a out of the face, b away from the opening): (−0.020, 0.030) →
  (0.060, 0.030) → (0.060, 0.120) → (0.080, 0.120) → (0.080, 0.360) → arc about (0, 0.360), R 0.080, 0° → 90° →
  (0, 0.440) → (−0.020, 0.440). Path: up the north jamb from z 0.700, round the arch, down to 0.700.
- **Keystone** (polish): a prism in the front plane with corners (y, z) (−0.220, 8.020), (0.220, 8.020), (0.310, 8.900),
  (−0.310, 8.900). It projects from a = −0.020 to 0.130.
- Old code: ElanExteriorStructure.cpp:380-556; MuseePlan.h:246-255.

### 3.3 The floor: cream marble polished to a mirror

- **Marble** 30 mm (z −0.030 … 0.000), `cream-marble.polished.walked`: the Rotunda's cream marble at the Future's
  mirror polish (materials.py:1262-1269: era III).
- Laid by Form in rings of radial slabs. Joints are **6 mm**, grouted (**missing** substance `grout`).

| Ring | r from | r to | Slabs |
|---|---|---|---|
| 1 | 2.290 (the lip) | 2.940 | 12 |
| (the gilt waiting ring) | 2.940 | 3.060 | — |
| 2 | 3.060 | 4.583 | 12 |
| 3 | 4.583 | 6.107 | 24 |
| 4 | 6.107 | 7.630 | 24 |
| 5 | 7.630 | 9.153 | 36 |
| 6 | 9.153 | 10.677 | 36 |
| 7 | 10.677 | 12.200 | 48 |
| 8 | 12.200 | 14.090 (into the cove) | 36 |

- **Ring joints** are centred on r 4.583, 6.107, 7.630, 9.153, 10.677 and 12.200.
- **Radial joints:** in a ring of n slabs, at bearings 15° + k·360°/n. So there is always one on every rib line.
- **The lip round the shaft:** ring 1's inner edge is a 30 mm lip, face r 2.290 from 0.000 down to −0.030, its underside
  running out to r 2.302 on the lining's top.
  - The lip and the lining are **notched at bearings 0° and 180°** for the Atrium's rails: the blade slot is 16 mm wide
    to r 2.385; the foot slot is 140 mm wide from 2.385 to 2.430.
  - Elsewhere the lip clears the car's rings (r 2.235) by 55 mm.
- **Door zone:** the marble stops on the wall's inner face between the jambs. The joint's threshold takes over.
- **Gilt waiting ring:** annulus r 2.940 … 3.060, z −0.030 … 0.000, flush, `bronze.gilt.walked`.
- **Waiting mark** (`bronze.gilt.walked`), cast 2 mm proud:
  - a half ring r 0.380 … 0.460 about (−3.310, 0.000) on its **west** half, open towards the car;
  - a dot r 0.140 at (−3.650, 0.000).
  - Each sits in a 10 mm recess: z −0.010 … +0.002. **Re-expressed:** as built they started 1 mm under the face with no
    recess.
- **Bay letters A–K** (polish), cast in gilt bronze 2 mm proud, set in the same kind of recess:
  - 0.300 m high, Didot-like, at r 13.100 on the bay-centre bearings (A 300° … K 240°);
  - each letter's top faces its bay;
  - the strokes, their widths and stress are given as point lists in AtriumBaseStructure.cpp:534-725.
- Old code: AtriumBaseStructure.cpp:41-43, 149-270, 728-767; MuseePlan.h:327-343.

### 3.4 Glazing

**The drum's inner leaf, "the misty glass"** (`low-iron-glass.misty.clean`):
- Cylinder r **14.000** (room face) … 14.024, z 6.000 … **21.180**, where the canopy meets it.
- **Panes:** 15° wide, with joints at bearings 15°k (on the rib lines and the bay centres, behind the exterior fins and
  mullions). Four tiers, split at z 10.000, 14.000 and 18.000: 96 panes. Butt joints 8 mm, silicone.
- **Make-up:** laminated low-iron glass, 2 × 12 mm, with a **UV-filtering** interlayer (`laminated.uv-filtering`). The
  room face is **acid-etched** to a satin. The cavity face carries a white **ceramic frit**.
  - Its visible transmittance is a Matter record value, chosen so that §4.4's care cap holds (estimate 5–10 %). It is
    never a knob.
  - The etched and fritted workings are **missing** in Matter.
- **Stock:** the chord is 3.654 m, beyond common float stock (3.21 m). Form.WithinStock may split each pane on its 7.5°
  line (R-5).
- **Head closer:** a pearl-steel ring r 14.000 … 14.080, z 21.180 … 21.200 (`steel.painted-pearl.clean`). It caps the
  inner leaf and the cavity under the entablature's inner face.

**The drum's outer leaf** (`low-iron-glass.laminated.weathered`):
- Cylinder r **14.050 … 14.074**, z 6.000 (in a glazing channel on the wall's top) … 20.950 (up beside the
  entablature's inner face).
- Clear, laminated, heat-strengthened low-iron glass.
- **Panes:** between the order's verticals, 15° wide, in four tiers (6.000 | 10.000 | 14.000 | 18.000 | 20.950).
- The **cavity** r 14.024 … 14.050 is sealed top and bottom.
- **Re-expressed:** as built the drum had three concentric skins (the interior glass at 14.00, an exterior "misty" skin
  at 14.03 and a clear pane at 14.07), because the old renderer couldn't light one skin for both sides. It is now one
  real double-skin façade: the misty inner leaf and the clear outer leaf.
- Old code: ElanStructure.cpp:395; ElanExteriorStructure.cpp:1196-1216; MuseePlan.h:231-237.

**The canopy: the Sphere's underside, glazed** (`low-iron-glass.etched.clean`):
- A spherical zone: upper face R **14.000** (on the seam), room face R **14.024**, about (0, 0, 22.000).
- It runs from the compression ring (r 3.600, z 8.446) out to the drum's inner leaf (r 14.000, z 21.180).
- **Panes:** the triangles of the grid (§3.5), about 2,350 of them, 0.6–1.4 m on a side. Laminated low-iron glass,
  2 × 12 mm, the room face acid-etched in a light-extraction pattern.
  - Each pane's edges bear on the grid members' tops.
  - The panes are the **diffusers of the luminous ceiling** (§4).
  - The outermost band of panes, between the last grid ring (r 13.821, z 19.622) and the drum, butts the inner leaf at
    z 21.180 with a silicone joint.
- **Re-expressed:** as built the glass lay at R 14.000 with the grid 5 cm under it and no support between them. The
  panes now sit *on* the members, which carry them. The room face moves down 24 mm to R 14.024, so the glass stays
  outside the seam.
- Old code: ElanStructure.cpp:381-400.

### 3.5 Steelwork: pearl-white painted steel

**The twelve ribs** (`steel.painted-pearl.clean`), in vertical planes at bearings **15° + 30°k**, k = 0 … 11. Each is one
constant section swept along a spine. The spine is the rib's back line, given in (r, z) in its plane:
1. **Up the wall:** vertical at r **13.995** (5 mm off the stone and the inner leaf), from z 0.000 to **9.003**.
2. **The turn:** an arc of radius **3.000** about (10.995, 9.003), from 0° to 130.230° (angles from +r towards +z). It
   ends at (9.057, 11.293), tangent to the canopy.
3. **Under the Sphere:** an arc of radius **14.024** about (0, 22.000), from −49.770° to −75.125° (angles from +r
   towards +z). It ends at r 3.600 on the ring's outer face, where the rib's end is capped.

- **Section** (d = depth from the back towards the room, t = tangential): (0, −0.150) → (0.390, −0.150) →
  (0.420, −0.120) → (0.420, 0.120) → (0.390, 0.150) → (0, 0.150). That is 0.420 deep and 0.300 wide, with 30 mm chamfers
  on the room-side arrises.
- **Shoe** (`bronze.gilt.clean`): a sleeve 15 mm proud on the three room faces, from 0.000 to 0.360, flat top. Section:
  (0, −0.165) → (0.435, −0.165) → (0.435, 0.165) → (0, 0.165).
- **Line** (`bronze.gilt.clean`; polish): a strip 24 mm wide, 2 mm proud (d 0.420 … 0.422), up the rib's face from the
  shoe's top to the ring.
- **Re-expressed:** as built the back was tucked 30 mm *into* the stone, the glass and the canopy (r 14.030 and
  R 13.970). It now stands 5 mm clear of the stone and glass, and bears on the canopy's room face.
- Old code: ElanStructure.cpp:402-497.

**The compression ring round the car's way** (`steel.painted-pearl.clean`), lathed. Section (r, z): (3.000, 7.745) →
(3.600, 7.890) → (3.600, 8.446) → (3.000, 8.301).
- Its top follows the canopy's room face. Its soffit falls 30 mm clear of the collar's top.
- The ribs end on its outer face; its bore (r 3.000) is the neck column.
- Twelve **hanger plates** (`steel.painted-pearl.clean`), 10 mm thick, radial r 3.150 … 3.450, at the rib bearings,
  bridge the 30 mm gap from the ring's soffit to the collar's top and carry the collar.
  **Re-expressed:** as built the collar floated.
- Old code: ElanStructure.cpp:498-503; ElanStructure.h:46.

**The canopy grid** (`steel.painted-pearl.clean`), after the Great Court's roof:
- Rectangular members **60 mm wide × 120 mm deep**. The top face is on the canopy's room face (R 14.024), with the depth
  along the Sphere's radius to R 14.144.
- **Nodes** lie on rings spaced in the conformal coordinate, so the triangles stay near equilateral. Ring positions on
  the canopy's room face, (r, z), from the rim inwards:

| Rings | Nodes per ring | (r, z) |
|---|---|---|
| 1–14 | 72 | (13.821, 19.622), (13.606, 18.600), (13.321, 17.615), (12.974, 16.676), (12.574, 15.790), (12.130, 14.961), (11.650, 14.193), (11.145, 13.488), (10.623, 12.844), (10.092, 12.262), (9.558, 11.738), (9.029, 11.269), (8.508, 10.851), (8.000, 10.482) |
| 15–20 | 36 | (7.074, 9.891), (6.221, 9.432), (5.449, 9.078), (4.756, 8.807), (4.142, 8.602), (3.600, 8.446) |

- **Node bearings:** in the 72-node rings, 5° apart starting at bearing 90° (plan angle 0°); every other ring is offset
  by half a step. The 36-node rings are 10° apart. The first of them stands over every other node of ring 14; after
  that they alternate.
- **Members:** each node joins its ring neighbours and the two nearest nodes on the ring outside. At the 72 → 36
  transition, each inner node fans to three.
- Members stop 5 mm short of the ribs' side faces and of the ring (square ends). The top channel of every member holds
  the edge-lit LED line of the luminous ceiling (§4).
- **Re-expressed:** as built the members crossed the ribs.
- Old code: ElanStructure.cpp:27-34, 87-165.

**The drum's exterior order** (`steel.painted-pearl.weathered`). The root, where every member starts, is r **14.074**,
the outer leaf's face. **Re-expressed:** it was r 14.050, inside the pane.

In the sections below, B is the seat: **6.200** on the wash, or **9.500** on the portal's roof within ±25° of west (the
fins at 255° and 285° and the mullion at 270°).

| Member | Count, bearings | Section (r, z), extruded tangentially | Width |
|---|---|---|---|
| Fin | 12, at 15° + 30°k (on the ribs' bearings) | (root, B + 0.800) → (15.000, B + 0.800) → (14.800, 19.400) → elliptical arc about (15.600, 19.400), radii 0.800 in r and 2.680 in z, from 180° to 90° → (15.600, 22.080) → (root, 22.080); its head is cut level at the corona's soffit (22.050) | ±0.170 |
| Pedestal | 12, under each fin | (root, B) → (15.100, B) → (15.100, B + 0.780) → (15.030, B + 0.850) → (root, B + 0.850) | ±0.280 |
| Mullion | 12, at 30°k (the bay centres) | (root, B) → (14.450, B) → (14.450, 20.950) → (root, 20.950) | ±0.070 |
| Transom | 3 rings, at z 10.000, 14.000 and 18.000, all round | (root, z − 0.050) → (14.300, z − 0.050) → (14.300, z + 0.050) → (root, z + 0.050), running between the verticals | — |

Old code: ElanExteriorStructure.cpp:564-586, 1096-1127; MuseePlan.h:231-237.

### 3.6 The lift installation (the Atrium serves both wells)

**The lining through the slab** (`bronze.patinated.dark`): a cylinder shell r 2.300 … 2.560, z −0.500 … −0.030. It is
slotted at bearings 0° and 180° for the rails. It is the Atrium's; the collar ring under it is the joint's (cube.md
§3.10).

**The Atrium's guide rails** (the fixed fit-out through its own zone; `bronze.brushed.clean`, the Cube's role):
- T-section as the Cube's: blade r 2.265 … 2.385, 16 mm thick; foot plate r 2.385 … 2.425, 140 mm wide.
- At bearings 0° and 180°, from z −0.490 up to 7.425 (30 mm under the collar's soffit).
- Across the seam they leave a 30 mm break over the Cube's rail tops (−0.520). The roller guides (95 mm) bridge it.
- **Brackets** (`bronze.patinated.dark`), each 180 mm wide and 100 mm tall, r 2.421 … 2.450. The rail's foot bears in
  the lining's slot on the slab; brackets fix it to the landing's head ring (3.000) and the tube's ring frame (5.358).
  They never pass through glass. The rail's top 2.07 m, above the last bracket, stands free, within the 2.5 m bracket
  pitch.
- **Re-expressed:** as built the car had no rails above the Cube's shaft. A real car needs guides over its whole travel
  (principle 1). The rails stand inside the glass at north and south, clear of the doors.

**The landing enclosure** ("a glass enclosure Ø 4.9 m, door west", `low-iron-glass.laminated.clean`, 15 mm):
- **Panes:** r 2.450 … 2.465, z 0.060 … 2.940. Spans are measured clockwise from the door's centre line (bearing 270°):
  - doorway: −24° … +24° (1.993 m clear chord);
  - pocket panes: 24° … 48° and 312° … 336°;
  - large panes: 48° … 136°, 136° … 224° and 224° … 312°.
- **Leaves ×2:** curved glass at r 2.375 … 2.390, z 0.008 … 2.940.
  - Closed, each spans from the door's centre line to 25°, tucking 1° behind its pocket pane.
  - Each has a gilt-bronze leading fin 12 × 25 mm.
- **Gilt bronze** (`bronze.gilt.clean`):
  - shoe r 2.360 … 2.490, z −0.010 … 0.070, open across the doorway;
  - head ring r 2.360 … 2.490, z 2.930 … 3.000.
  - Both are notched round the rails.
- **Sill** (`bronze.gilt.walked`): r 2.245 … 2.515, z −0.030 … 0.008, across the door ±26°. It is set into the marble
  and bridges the car's base ring to the floor.
- The glass's deep-green edges show at every joint and jamb (Matter's glass edge; no separate piece).

**The tube**, over the landing up to the collar (`low-iron-glass.laminated.clean`, 15 mm, r 2.450 … 2.465):
- **Two tiers:** 3.000 … 5.333 and 5.383 … 7.685. The upper tier runs 0.230 up into the collar's hub bore (r 2.465).
- **Six panes a tier**, on the landing's divisions plus one over the door (−24° … +24°).
- **Ring frame** (`bronze.gilt.clean`): r 2.410 … 2.480, z 5.338 … 5.378. The tiers stop 5 mm short of it.
- Old code: ElanCar.cpp:284-340; ElanKit.cpp:24-41; ElanKit.h:57-66.

**The collar** (the iris's housing, "pearl, rimmed in gilt, 8.2 m across"; `steel.painted-pearl.clean`), lathed. Section
(r, z):
1. (2.465, 7.455) → (2.800, 7.455) → (2.800, 7.545) → (3.620, 7.545) → (3.620, 7.455): the hub, and the coffered band
   between the inner and outer rings
2. (4.000, 7.455) → (4.020, 7.465)
3. **ovolo**: arc about (4.020, 7.545), R 0.080, from −90° to 0° → (4.100, 7.545)
4. (4.100, 7.685) → (4.100, 7.715): the fascia
5. (2.465, 7.715): the top, the mating plane
6. back to the start

- **Spokes (polish):** 24 radial pearl ribs in the coffered band at bearings 15° + 15°k, centred on r 3.210, half-length
  0.414, width 44 mm, hanging 50 mm from the coffer's back (7.545).
- **Gilt lines (polish),** `bronze.gilt.clean`:
  - a liner r 2.360 … 2.469 in the hub, 7.415 … 7.715;
  - tori (major r, tube r, centre z): (2.475, 0.008, 7.453), (2.796, 0.007, 7.457), (3.624, 0.007, 7.457) and
    (3.960, 0.016, 7.459);
  - the lip band r 4.090 … 4.120, z 7.685 … 7.715.
- **The pocket** (`atrium.collar.pocket`): r 2.465 … 4.050, z 7.660 … 7.705. The open blades lie in it, 20 mm under the
  top.
- Old code: ElanCar.cpp:341-400.

**The collar iris** (body key `atrium.collar.iris`): the Atrium's own ceiling seal, the lower end of the neck's airlock.
- It has the class's geometry (cube.md §6.1): 7 blades, each a 110° arc of the middle circle r 3.095; width 1.250 with
  round ends; pins at r 3.745; 5 mm thick; stacked 3 mm apart.
- The top blade's top face is at **7.695**, the stack's bottom at 7.669.
- Blades' undersides (seen from the Atrium) `bronze.gilt.clean`; their tops (seen from the neck)
  `steel.painted-pearl.clean`.
- Its motion is §6.
- **Re-expressed:** as built it belonged to the neck. The class closure of `atrium.up` is now the Sphere's mouth iris
  (§2), so this iris is the Atrium's.
- Old code: ElanSphereTop.cpp:46-184, 249-251; ElanKit.h:68-84.

**The car** (body key `atrium.car`). Its frame is its floor level on the axis.

| Part | Geometry (car frame) | Key |
|---|---|---|
| Panes | r 2.185 … 2.200 (15 mm), z 0.020 … 2.535, set 5 mm into grooves in both rings; spans clockwise from the door's centre line (west): 24° … 92°, 92° … 180°, 180° … 268°, 268° … 336° | `low-iron-glass.laminated.clean` |
| Leaves ×2 | r 2.135 … 2.150, z 0.025 … 2.535. Closed, each spans from the centre line to 25°. Open, each turns 24° behind the panes. Leading fins 12 × 25 mm. | `low-iron-glass.laminated.clean`; fins `bronze.gilt.clean` |
| Base ring | r 2.130 … 2.235, z −0.070 … 0.025; carries the floor's edge | `bronze.gilt.clean` |
| Top ring | r 2.130 … 2.235, z 2.530 … 2.600 | `bronze.gilt.clean` |
| Floor | disc r ≤ 2.135, z −0.030 … 0.020, laminated, anti-slip etched. The walking face is 20 mm over the stop level. | `low-iron-glass.etched.walked` |
| Roof | a clear disc r ≤ 2.135 at z 2.575, inside the top ring | `low-iron-glass.laminated.clean` |
| Light ring | the luminaire's opal diffuser, r 2.060 … 2.140, z 2.495 … 2.540 (§4) | Light's |
| Handrail | glass rod Ø 36 mm on r 2.000 at z 0.970, from 30° to 330° from the door, ball ends; five brackets at 60°, 120°, 180°, 240° and 300°, r 2.000 … 2.196, 12 × 12 mm | rod `low-iron-glass.toughened.clean`; brackets `bronze.gilt.clean` |
| Roller guides | on both rings at bearings 0° and 180°: blocks r 2.229 … 2.290, ±0.045 tangential, the rings' height. Rollers either side of the rail blade: a declared rolling contact (cube.md R-5). | `bronze.gilt.clean` |
| Crown | a gilt drum r 0.340 from 2.583 to 2.780 on four arms at bearings 45°, 135°, 225° and 315° (r 0.310 … 2.225, z 2.600 … 2.632); a bead round its top | `bronze.gilt.clean` |
| Cube mast | spiral-band column r 0.170 (45 mm seam pitch), fed out of the crown up to the Cube's iris below the clamp level: at most 12.785 m | `bronze.gilt.clean` |
| Cube mast head | r 0.260; groove r 0.195 from 0.070 to 0.110 above the crown top; cap to 0.160 above it (2.940 above the car floor) | `bronze.gilt.clean` |
| Under-floor drum | r 0.200, z −0.160 … −0.030 (as built: "where the mast meets the car") | `bronze.gilt.clean` |
| Sphere mast | the column the car stands on above the Sphere's mouth: fed down from the under-floor drum to a head the Sphere's mouth iris clamps (the Cube's arrangement, mirrored). Length ≈ 9 m at the top stop. | `bronze.gilt.clean` |

- **Re-expressed:** the as-built glass "faded" at the Cube's stop. The car's glass is now real glass. It clears the
  physical way: its light dims to a glimmer, and clear low-iron glass lit only by that, in a black room, shows almost
  nothing (§4, §6).
- **Risk R-7:** the crown (Ø 0.68) and the under-floor drum (Ø 0.40) are too small to house 12.8 m and 9 m of
  spiral-band column. Motion.Housed decides; the drums grow at polish.
- Old code: ElanCar.cpp:20-190; ElanCar.h:13-27; ElanKit.h:40-55, 94-103; CubePlan.h:71-73.

### 3.7 Fittings

**The Starry Night's glass stele** (Art's fitting `stele.glass.1500x2200`, the Salon's *Impression, Sunrise* stele's
twin):
- **Glass:** 60 mm low-iron glass, laminated from toughened plies (`low-iron-glass.laminated.clean`): x −3.840 …
  −3.780, y −2.650 … −1.150, z 0.060 … 2.200. That leaves 40 cm of clear glass over the frame.
- **Foot** (`brass.brushed.clean`): x −4.010 … −3.610, y −2.750 … −1.050, z 0.000 … 0.080. Its top edges are eased
  6 mm. The glass is set 20 mm into it.
- **Collision:** the box x −4.010 … −3.610, y −2.750 … −1.050, z 0 … 2.200 blocks the body only. The look trace passes
  to the painting.
- Old code: AtriumBaseStructure.cpp:75-77, 769-800, 1118-1126; MuseePlan.h:338-339.

### 3.8 What the Atrium does not build

- **Site's joints build:**
  - the west door's threshold, bronze strip, lining and surrounds;
  - the collar rings on both shaft mating planes and the slot band at `atrium.up`;
  - the class irises at the receiving stages' seal faces.
- **The Cube builds:** its shaft glass, the strata, its rails below −0.520, the sleeve, the soffit ring and the iris
  housing.
- **The Sphere builds:** its throat (r 2.550 … 2.980 from 7.715), its floor ring, its platform and platform iris, and
  its dome shell and exterior crown.
- **The Hall of Light builds:** its end, standing inside the portal's opening.
- **The Grounds build:** the terrace from r 15.32.
- **Light builds:** the luminaires' housings and diffusers from their records (the canopy panes are drawn here and
  shaded by Light's diffuser master).

### Finishes (the schedule `Data/Stages/Atrium/Finishes.json`)

| Role key | Where | Matter status tonight |
|---|---|---|
| `travertine.honed.clean` | base wall (inner face, cove, coping band, wall top), footing | exists (travertine, honed) |
| `travertine.honed.weathered` | exterior dress, entablature, portal and its mouldings | exists; the history maps to stone.dissolution-outdoors + stone.pore-soiling |
| `cream-marble.polished.walked` | the floor | exists; the Future's "mirror" grade of `polished.stone` to confirm |
| `bronze.gilt.walked` | waiting ring and mark, landing sill (bay letters: polish) | exists (gilt over cast bronze, D-014; gilt.wear + bronze.wear) |
| `bronze.gilt.clean` | rib shoes and lines, landing shoe and head ring, tube frame, car rings, fins and guides, crown, masts and heads, collar gilt lines, blades' undersides | exists |
| `bronze.patinated.dark` | lining through the slab, rail brackets | exists (the Cube's role) |
| `bronze.brushed.clean` | guide rails (the Cube's role; D-018 may name a machined rail steel) | exists |
| `brass.brushed.clean` | stele foot | exists (brass, accent of bronze) |
| `steel.painted-pearl.clean` | ribs, ring, hangers, canopy grid, collar, head closer, blades' tops | `painted.steel` exists; pearl-white coat and the **family question** (R-8) |
| `steel.painted-pearl.weathered` | fins, pedestals, mullions, transoms | as above, outdoors (steel.coating-ageing) |
| `low-iron-glass.misty.clean` | drum inner leaf | **missing** workings: acid-etched and ceramic frit, stacked on `laminated.uv-filtering` |
| `low-iron-glass.laminated.weathered` | drum outer leaf | exists (heat-strengthened, laminated; glass.water-stain + glass.soiling) |
| `low-iron-glass.etched.clean` | canopy panes (the luminous ceiling's diffuser) | **missing** working `etched` |
| `low-iron-glass.laminated.clean` | car panes, leaves and roof; landing; tube; stele | exists |
| `low-iron-glass.etched.walked` | car floor | **missing** working `etched` |
| `low-iron-glass.toughened.clean` | car handrail rod | exists |
| `concrete.cast-in-situ.sealed` | floor slab (hidden) | **missing** substance |
| `grout.cementitious.clean` | floor joints | **missing** substance |

---

## 4. Light

### 4.1 Daylight

- **One daylight opening:** the drum (`atrium.drum`), a diffusing double-skin glazing about 1,335 m² seen from inside
  (r 14.000, z 6.000 … 21.180).
- Light passes the clear outer leaf, then the fritted and etched inner leaf, which scatters it. The room is lit by a
  luminous band 15 m tall all round. There are **no sharp sun patches**.
- The sunny side of the drum glows brighter. The fins, mullions and transoms cast soft shadow bands on the inner leaf,
  read from inside.
- **UV:** the UV-filtering interlayer keeps UV under 75 µW/lm at the works.
- **No daylight controls:** no blinds or louvres (D-014). Care is met by the glazing's make-up and the hang (§4.4).
- **The canopy gets no daylight from above** (the Sphere is opaque). Its glow is electric (§4.2), and daylight reaches
  it only by inter-reflection from the drum.
- **By night** the drum glows from outside with the interior's own light transmitted through it. There is no emissive
  skin.
- **Re-expressed:** as built, the drum and the canopy were emissive materials (`M_MistyGlass`, 250 and 900 cd/m², ×
  daylight, with a night floor), and 24 invisible rect lights hung under the canopy. The glow is now light:
  - the drum's is daylight through real glass;
  - the canopy's is a luminaire whose diffuser is the canopy (D-014: one emissive master, every emitter from the
    catalogue).
- Old code: ElanStructure.cpp:36-44, 278-314, 340-372; ElanExteriorStructure.h:155-177.

### 4.2 Luminaires (catalogue type, count, position; never lumens) — the schedule `Lighting.json`

| # | Type (catalogue) | Count | Mount | Control class |
|---|---|---|---|---|
| L1 | `linear-cove.3000k` (exists) | 11 runs, one per art bay, each ≈ 6.84 m (75.3 m in all) | In the cove, on its back face (r 14.090) at z 0.030 … 0.055, facing the room and tilted 20° down. Each run ends 0.100 m clear of the rib shoes. **None in the door bay** (as built). | circulation |
| L2 | **luminous ceiling, edge-lit light-guide glass** (**missing** kind and record) | 24 zones: an inner ring (r 3.600 … 8.500) and an outer ring (r 8.500 … 14.000, to the drum), 12 zones each, centred on the bay-centre bearings 30°k | The zone's canopy panes are the diffuser. Its LED lines lie in the top channels of the grid members bounding those panes. | **laylight** (a new class, §4.3) |
| L3 | `linear-slot` opal at **4000 K** (**missing**: only `linear-slot.3000k` exists) | 1 ring, 13.19 m | In the car's top ring, r 2.100, z 2.495 … 2.540, facing down. The as-built car light was 5000 K; at 3100 K the neck turned amber. | **lift** |
| L4 | `track-spot.3000k` (exists; Scheme picks the beam) | 1, on *The Starry Night* | Two declared mounts; Scheme picks by its aim rule (≈ 30°, glare geometry): **M1** a bracket on the room face of the rib at bearing 285° (the door bay's north rib) at z 6.000, (−13.112, −3.513, 6.000); **M2** the canopy grid member nearest (−7.400, −1.900), underside z ≈ 10.10. Aim: the canvas centre (−3.885, −1.900, 1.380). | gallery |

- **Reserved mounts** (polish; built only if a target fails or the exterior night lighting is scheduled):
  - L5: 12 in-ground uplights, one per rib at r 12.700 on its bearing, aimed up the rib's face (as built:
    ElanStructure.cpp:316-338). Built only if target T3 fails.
  - L6: 12 fin uplights on the coping at each fin's foot (r 15.050, z 6.200), facade class.
  - L7: 12 dome washes on the cornice at the bay centres (r 15.300, z 22.500), facade class. They light the Sphere's
    dome, whose stage declares their target.
- **Re-expressed:** the as-built cove was 3800 K. The catalogue's cove is 3000 K. A 4000 K cove record, if the palette
  asks for it, is a one-line schedule change.

### 4.3 Operation (Light's `Operation.json`, by channel)

| Luminaire | Rule |
|---|---|
| L1 cove | Dusk switching by solar altitude, and daylight-linked dimming that holds the floor target. Off when daylight holds it. |
| L2 canopy | The **laylight** class: output follows the museum observer's exterior daylight, up to its commissioned level, never below a **night floor of 0.20** of that level (as built). The commissioned level is capped so the care target (§4.4) holds at every reference moment. |
| L3 car | On while the car is occupied or moving. At the Cube's stop, once stopped, it dims to **0.03** of its level over 2.5 s: the glimmer, the glass "clearing". It is restored before the car leaves (cube.md I-5). Channel `atrium.car.light` (1 full, 0.03 glimmer). |
| L4 painting | Presence-linked. It holds the painting's target when the room's light falls short, and stays off when daylight holds it. |

### 4.4 Targets

**Use targets** (`UseTargets.json`):

| Surface | Class | Numbers |
|---|---|---|
| The floor's walking zone (r 3.060 … 14.000) | `entrance-hall` (the Atrium is named in it) | 100 lx maintained, U0 ≥ 0.4, UGR ≤ 22, walls ≥ 75 lx, ceiling ≥ 50 lx |
| The same, after dusk | `circulation.night` | 50 lx, U0 ≥ 0.4 |
| The car's floor, and the landing inside r 2.450 | `lift` | 100 lx |
| Every walked route, every configuration | `escape-route` | ≥ 10.76 lx. The car at the Cube's stop during *Sea Light* is cube.md Q-6. |

**Care** (Art's, `Care.json`): *The Starry Night*, oil on canvas, class `oil-and-tempera`: **150 lx** recommended,
**≤ 200 lx** at every reference moment, UV ≤ 75 µW/lm.

**This cap sets the room's absolute brightness.** The painting faces west in the middle of a 1,335 m² luminous drum,
under a luminous canopy. Its vertical illuminance is about 0.5 × the drum's mean luminance plus 0.6 × the canopy's.
To stay ≤ 200 lx, both must stay near **150–250 cd/m²** by day. So:
- the inner leaf's transmittance must be low (≈ 5–10 % against a 15 klx overcast façade), which is Matter's record;
- the canopy's commissioned level is capped (Light).

The worst case is a clear-sky reference moment at 15:00, with the sun west of south on the drum the painting faces:
risk R-1.

**Design targets** (luminance order between named surfaces, measured by Light::MeasureLuminance, never knobs):
- **T1 · At night** the canopy's mean luminance exceeds that of every other extended surface in the room: the drum's
  inner leaf, the base, the floor and the ribs. Luminaire diffusers are excluded.
- **T2 · At overcast reference moments** the canopy's mean is at least the drum inner leaf's mean: "a glowing
  ceiling". Clear-sky moments are exempt on the sunlit third of the drum.
- **T3 · The ribs** read as pearl structure, never as silhouettes: their room faces are at least 0.25 × the canopy's
  mean behind them. If this fails, the Scheme adds L5.
- **T4 · *The Starry Night*** reads as "a small square of night in a daylit room" (the board). Its own low luminance
  is the design; there is no brightness target beyond care.

---

## 5. Time and weather

- **Weather place:** the drum's outer leaf, the plinth's coping and wash, the entablature's wash and the portal's roof
  take the **museum** observer's weather (D-005). There is no remote weather here.
  - Rain wets the outer leaf and the stone (Matter's surface law, sky exposure from Form's facts).
  - Snow lies on the washes and the portal's roof.
  - The inner leaf and the canopy are indoors and dry.
- **Sun:** the museum's one sun, idealised by default (D-003). It lights the drum from outside, and its light enters
  diffused. There is no sun patch indoors. The glow band moves round the drum with the hour.
- **Night:** the museum is always open (D-006). The cove switches at dusk and the canopy falls to its night floor. From
  the Grounds the drum reads as a lantern lit from within.
- **Seasons:** nothing in the Atrium follows the solar terms (no plants).
- **The Sphere's sky** is the Sphere's own. Nothing in the Atrium reads it.

---

## 6. Motion

All mechanisms are the Atrium's own, or the Atrium's serving bodies inside its wells. They form **one coupling group**
with the Cube's iris (the joint's) and the Sphere's mouth and platform irises. Site binds their keys across the seams.

### 6.1 The car (`atrium.car`)

| Quantity | Value |
|---|---|
| Joint | prismatic along the stage's z axis |
| Travel | car floor −23.600 … +20.400: **44.000 m** |
| Named stops | `cube` −23.600 (eyes at −22.000, the Cube's centre); `clamp` −10.825 (the crown's head in the Cube's iris); `atrium` 0.000 (the landing); `neck` 8.400 (home, both neck irises shut); `sphere` 20.400 (floor top 20.420, level with the Sphere's platform; eyes at 22.000) |
| On the rails (−10.825 … the Sphere's mouth) | 1.5 m/s, 0.75 m/s² (MuseePlan.h:204-205) |
| On the Cube mast (below −10.825) | 1.0 m/s, 0.5 m/s² (ElanElevator.cpp:44) |
| On the Sphere mast (above the Sphere's clamp level) | 1.5 m/s, 0.75 m/s² (the ElanSphere board) |
| Jerk | ≤ 1.0 m/s³ (Motion.Comfortable). **Re-expressed:** the as-built drive had no jerk limit. |
| Drive | **Above the clamp level**, rack and pinion on the rails: racks on the rails' backs, pinions in the roller-guide shoes at both rings. Two of the four pinions are always engaged across the rail breaks (polish: the gear drawings, Musee::Motion::Rack). **Below it**, the crown feeds the Cube mast. **In the Sphere**, the under-floor drum feeds the Sphere mast. There is no counterweight: Motion.Balanced rates each drive for the car's mass (Form volumes × Matter densities) plus its rated load (EN 81-20 for its 14.25 m² floor). |
| Visitor | the floor is the rider's moving base (Visitor) |

### 6.2 Leaves

| Body | Joint | Travel | Time |
|---|---|---|---|
| `atrium.car.leaf.north` and `.south` | revolute about the stage's z axis, riding with the car | 24° each, away from the door's centre line, behind the panes 24° … 92° and 268° … 336° | 1.5 s, eased s = 3o² − 2o³ |
| `atrium.landing.leaf.north` and `.south` | revolute about the stage's z axis, fixed | 24° each, into the pocket panes | 1.5 s, eased |

The car's and the landing's leaves at the Atrium move together.

### 6.3 The collar iris (`atrium.collar.iris`)

- The class geometry and motion (cube.md §6.1): 7 revolute blades on pins at r 3.745, a 52° swing, 4.0 s (eased);
  states **shut** (0) and **open** (1). There is no clamp state here.
- **Housed:** in travel the blades reach r 4.01, inside the pocket (r ≤ 4.050, z 7.660 … 7.705).
- Channel: `atrium.collar.closure` (0 … 1).

### 6.4 Sequences (the as-built timings; ElanElevator.cpp:480-640)

1. **Called from the Atrium** (the car is home at the neck, both neck irises shut): the collar iris opens (4 s), the
   car descends 8.400 m (≈ 7.6 s), and the car's and landing's leaves open (1.5 s). About **13 s**.
2. **Atrium → Cube:** the leaves shut (1.5 s; they wait while a body stands in the doorway), and the car descends.
   - The Cube's iris opens ahead of it. The car floor passes −7.750 only with the iris open.
   - The car stops at the clamp level, and the iris clamps the head (4 s).
   - The car sinks on the mast to −23.600 (≈ 14.8 s), then its light dims to the glimmer (2.5 s).
   - About **25 s**, with the irises.
3. **Cube → Atrium or Sphere:** the light comes back (2.5 s), the car rises to the clamp level, the iris opens and
   lets the head go, and the car climbs the rails to 0 (the leaves open) or on to the neck.
4. **Atrium → Sphere:** the leaves shut, and the car rises to the neck stop, 8.400.
   - The collar iris shuts behind it (4 s), then the Sphere's mouth iris opens ahead (4 s): the airlock.
   - The car rises to 20.400 (≈ 10 s) while the Sphere opens its platform iris ahead. The leaves open.
   - About **27 s**.
5. **Home:** if the car stands open at the Atrium and nobody is within r 3.500 of the axis for 12 s, it shuts and rises
   to the neck stop, and the collar iris shuts.
6. **The first ride goes down.** "Up to the Sphere" is offered at the Atrium only after the car has been to the Cube in
   this visit.

### 6.5 Interlocks (Motion, by key; Site binds those across seams)

| # | Rule | Keys |
|---|---|---|
| A-1 | The car moves only with its leaves shut, and at the Atrium also the landing's. | `atrium.car`, `atrium.car.leaf.*`, `atrium.landing.leaf.*` |
| A-2 | The landing's leaves open only with the car stopped at 0.000 ± 5 mm. The car's leaves open only at 0.000 (with the landing's) or at 20.400 (with the Sphere's platform interlock). They never open in the Cube. | same, plus the Sphere's platform keys |
| A-3 | **Occupancy guard:** a leaf holds, or reverses, while a body is in its next travel slice. The car waits while a body is in the doorway. | Motion's guard |
| A-4 | The car's head (2.940 above its floor) passes the collar iris's plane (7.695) only with the iris open (≥ 0.999). So the car floor may rise above **4.705** (7.695 − 2.940 − 0.050), or descend below **7.905** (7.695 + 0.160 + 0.050: the under-floor drum's bottom), only with it open. | `atrium.car`, `atrium.collar.iris` |
| A-5 | **Airlock:** the collar iris and the Sphere's mouth iris are never both open. The collar iris is open only while the car floor is below 8.395 (or passing through); the mouth iris only while it is above 8.405. At the neck stop both are shut, and the one behind closes before the one ahead opens. | `atrium.collar.iris`, the Sphere's mouth iris |
| A-6 | The Cube's I-1 … I-5 (cube.md §6.3) bind the car to `cube.shaft.iris` and `cube.space.performing`. | across the down seam |
| A-7 | The Sphere's platform iris and guard (its sheet) bind the car's approach to 20.400. | across the up seam |

### 6.6 Clearances

| Pair | Clearance | Note |
|---|---|---|
| Roller guides (r 2.290) ↔ the lining's bore (r 2.300) | 10 mm | as built (cube.md §6.4) |
| Car rings (r 2.235) ↔ the floor lip (r 2.290) | 55 mm | the lip is notched at the rails |
| Car rings ↔ landing leaves (r 2.375) | 140 mm | |
| Car rings ↔ tube ring frame (r 2.410) | 175 mm | |
| Car rings ↔ collar hub (r 2.465), blades open (r 2.470) | 230 mm | |
| Rail foot (r 2.425) ↔ landing panes and tube (r 2.450) | 25 mm | |
| Car leaves (r 2.150) ↔ car panes (r 2.185, the glass's inner face) | 35 mm | |
| Landing leaves (r 2.390) ↔ landing panes (r 2.450) | 60 mm | |
| Car head at the neck stop (11.340) ↔ the Sphere's mouth iris stack (11.370) | 30 mm | as built |
| Car's lowest point at the neck stop (8.240) ↔ the collar iris (7.695) | 545 mm | |
| Roller guides ↔ rail blades | contact | a declared rolling contact (cube.md R-5) |

### 6.7 Channels declared by this stage

`atrium.car.light`, `atrium.collar.closure`. The car also publishes FMuseeActuation for Sound.

### 6.8 Captions the car's uses carry (FMuseeText)

- "Down through the ground to the Cube: soil, clay, then chalk, then the room 22 m down."
- "…then up to the stars. A steady 1.5 metres a second."
- "Up through the ground to the Atrium."
- "Up through the earth, then the Atrium, to the stars."
- "The centre of the Sphere: the sky over you, now. Step out onto the platform."

Source: ElanElevator.cpp:280-370. The journey lines are retired.

---

## 7. Nature

None. The Atrium has no plants, water bodies or ground of its own. Rain and snow in the air stop at its shell (Nature's
generic precipitation). The terrace and planting round it are the Grounds'.

---

## 8. Art

**The work** (`van-gogh-starry-night`):
- Vincent van Gogh, *The Starry Night* (*De sterrennacht*), 1889. Oil on canvas, **73.7 × 92.1 cm** (h × w).
- The Museum of Modern Art, New York. Credit line (to confirm against MoMA's record): "Acquired through the Lillie P.
  Bliss Bequest (by exchange)".
- `assets/collection.json` lacks its dimensions and credit. The Art record must carry them (plan/canvas/FutureAtrium;
  assets/paintings/CREDITS.md:75).
- **Image:** `assets/paintings/van-gogh-starry-night.jpg`, the Google Art Project master (public domain, PD-Art;
  CREDITS.md:75, 232), colour-managed.
- **Object:** a stretched canvas at true size, **0.921 × 0.737 m**, on a keyed pine stretcher with tacking edges.
  - From behind, through the glass, the canvas's own back shows: unbleached linen on the pine stretcher, with its keys
    (the FutureAtrium board).
  - The frame is open at the back. **Re-expressed:** frames.py called it "backed". The board's view wins.
- **Frame:** slim gilt, the Salon's gilt profile at moulding width **W = 0.060 m**. The outer size is 0.921 + 2 × 0.957W
  wide by 0.737 + 2 × 0.957W high: **1.036 × 0.852 m**.
  - The profile is a point list in units of W: MuseeFrame.cpp:310-371.
  - Gilt over gesso on a wood core: role `gesso.gilt.aged` (**missing** substance `gesso`).
  - The ornament (leaf, beads, shells) is polish.
- **Hang:** on the stele's west glass face (`atrium.stele`), facing west towards the door and the Hall of Light.
  - The canvas face is at x −3.885, with its centre at (−3.885, −1.900, **1.380**).
  - The frame's back bears on the glass through clear pads.
  - **The anchor declares its centre height (1.380, as built).** Art.HangResolves must accept an anchor-declared centre
    in place of the human-measure line.
- **Care:** `oil-and-tempera` → FLightTarget (§4.4).
- **Caption** (IMuseeCaptioned): the title, artist and date, then the medium and credit. The body is the board's: "A
  small square of night in a daylit room … It is the sign for the elevator: this car goes up to the stars."

**The bays:** eleven Space anchors, A–K, for new art to be commissioned later. Nothing is hung tonight. Any work can go
in any bay (the pilasters are gone).

**Casts and scans:** none.

---

## 9. Sound

**Character, derived and not tuned.** A very live drum of stone and glass. Sound computes RT60 from Form's enclosure
facts and Matter's absorption:
- volume ≈ **7,700 m³**;
- ≈ 600 m² polished marble;
- ≈ 510 m² honed travertine;
- ≈ 1,335 m² glass drum;
- ≈ 1,120 m² glass canopy.

With typical coefficients, expect about **7 s at 500 Hz – 2 kHz**, falling to about **2.5–3 s at 125 Hz**: large panes
absorb the bass. That is a cathedral-like room.

**Geometry to watch:**
- The concave cylinder focuses a sound made at its centre back to the centre, 82 ms later: the "round-room echo". The
  car's glass and the landing break it partly.
- The convex canopy diffuses.
- Sabine won't show focusing (R-10).

**Emitters and sources:**

| Source | Evidence needed | Attached to |
|---|---|---|
| The car's drive (pinions on racks; the spiral-band column) | recorded at SPL at 1 m | `atrium.car`, from FMuseeActuation |
| Leaves sliding (car and landing), 1.5 s | as above | the leaf bodies |
| The collar iris's blades, 4 s | as above | `atrium.collar.iris` |
| Footfalls | Matter's footfall class: polished marble; gilt bronze on the waiting ring and the sill; etched glass in the car | Visitor's FMuseeContact |
| Rain on the drum's outer glass, the coping and the entablature's wash | museum weather, by exposed surface | Form's exposed surfaces |

**Passage:**
- The open west joint joins the Hall of Light's acoustics.
- The shut collar iris and the Cube's iris occlude.
- The car's glass occludes partly while its leaves are shut.

---

## 10. Visitor

- **Arrival:** from the Hall of Light through the 4 m door, the only one. Ahead stands the glass car with its door
  facing you. *The Starry Night* stands to its left (north), facing you.
- **Walking:** the mirror floor reflects the canopy. The bays run clockwise from A. Bay F lies on the east axis, seen
  through the car.
- **Look:** the caption card for *The Starry Night*.
- **Use** (IMuseeUsable; Hands target it):

| Where | Verb (FText) | Available when |
|---|---|---|
| On the Atrium floor, r 2.350 … 8.000 from the axis, no car at the landing | "Call the car" | the car isn't here or already coming |
| In the car at the Atrium | "Down to the Cube" | always |
| In the car at the Atrium | "Up to the Sphere" | after the Cube this visit |
| In the car at the Cube | "Up to the Atrium", "Up to the Sphere" | the light is back and no work performs (the work's own verb is the Cube's) |
| In the car at the Sphere | "Down to the Atrium" | the leaves are open |

- **Ride:** the car's floor is a moving base at 1.5 m/s. The first ride goes down to the Cube before it goes up.
- **Eye:** adapting from the daylit Hall of Light into the Atrium, into the car, and down into the dark Cube is the Eye's
  job (Visitor). No local exposure (cube.md R-12).
- **Sit:** nothing. There are no seats.
- **Hints:** "Step off the centre: the car is coming up" belongs to the Sphere's platform.

---

## 11. The old code map

The builder re-expresses these; it never copies them.

**Plan numbers:**
- `windows/Source/MuseeVision/Plan/MuseePlan.h:160-215`: Élan (radius, wall, base, door, car, speeds, the Sphere,
  `Underside()`).
- `MuseePlan.h:217-256`: Élan's exterior (plinth, drum skins, order, entablature, dome, portal).
- `MuseePlan.h:278-287`: the Hall of Light's view of the Atrium (r 14.8, the 96-gon floor).
- `MuseePlan.h:322-344`: the Atrium's level 0 (floor, ring, mark, stele, bay letters).

**The base, floor, inlays and stele:** `windows/Source/MuseeVision/Atrium/AtriumBaseStructure.cpp`
- 23-80: constants (rings, cove, band, retired pilasters, door, inlays, stele).
- 149-270: the floor (bands 151-158, joints 160-171, the lip 226-237, the door zone 239-269).
- 279-390: the coping band and the wall's top.
- 392-472: the wall (inner face and cove 397-425, door 427-461, outer face 463-471).
- 534-767: the bronze inlays (glyphs 577-665, placing 668-736, the waiting mark 739-767).
- 769-800: the stele.
- 806-832, 1118-1154: the cove lights (as built).
- 890-903, 1039-1116: materials (as built; not values).
- `AtriumBaseStructure.h:13-51, 124-170`: the as-built description and light numbers.

**The drum glass, canopy, grid, ribs and ring:** `windows/Source/MuseeVision/Elan/ElanStructure.cpp`
- 20-44: grid and laylight constants.
- 50-60: sphere helpers.
- 87-165: the grid.
- 278-314: the laylights (retired).
- 316-338: the rib uplights (reserved L5).
- 340-372: the emissive nits (retired).
- 374-400: the glass lathes.
- 402-497: the ribs (spine 409-435, section 442-444, shoe and line 478-496).
- 498-503: the ring.
- `ElanStructure.h:12-29, 46, 93-104`.

**The lift:** `windows/Source/MuseeVision/Elan/`
- `ElanKit.h:22-104`: dimensions of the car, landing, tube, iris, neck, crown and head.
- `ElanKit.cpp:21-41`: CollarTop; pane spans.
- `ElanKit.cpp:327-390`: fixed glass, leaves, turning.
- `ElanCar.cpp:20-190`: the car.
- `ElanCar.cpp:284-407`: the landing, tube and collar (profile 341-372; spokes and lines 373-400).
- `ElanCar.h:13-27, 118-127`.
- `ElanSphereTop.cpp:46-184`: the iris blade (outline, pins, stack).
- `ElanSphereTop.cpp:191-256`: the neck (the Sphere's now; the collar iris's placement is at 249-251).
- `ElanElevator.cpp:36-46`: timings and speeds.
- `ElanElevator.cpp:230-370`: calls and prompts (journey prompts retired).
- `ElanElevator.cpp:441-453`: legs.
- `ElanElevator.cpp:480-695`: doors, irises, airlock, drive, messages, home.
- `ElanElevator.h:20-42`.

**The Cube interface:** `windows/Source/MuseeVision/Cube/CubePlan.h:40-73` (lining, rails, iris, mast);
`Cube/CubeWay.cpp:17-21` (the clamp level); `Cube/CubeWay.h:12-24` (the spiral-band mast).

**The exterior:** `windows/Source/MuseeVision/ElanExterior/ElanExteriorStructure.cpp`
- 25-56: constants.
- 346-377: plinth and entablature sections.
- 380-411: socle, portal cornice, architrave.
- 418-556: the portal.
- 564-586: fin, pedestal, mullion.
- 1096-1127: the order.
- 1196-1216: the skins.
- 1128-1165: the pearl shell and gilt ring. **The Sphere's.**
- `ElanExteriorStructure.h:39-74`.

**Art:**
- `Shared/Wings/Elan.swift:32, 232-240`: the canvas's placement.
- `windows/Source/MuseeVision/Frames/MuseeFrame.cpp:310-371, 1031, 1059`: the gilt profile and outer size.
- `windows/Scripts/frames.py:41-44`.
- `windows/Scripts/gallery_lights.py:49`.
- `windows/Scripts/art.py:92-110`: the gigapixel master.

**Materials as built** (evidence for the palette, never values; D-018):
- `windows/Scripts/materials.py:1262-1269` (the mirror floor), `1397-1402` (brushed brass, the floor's joint);
- `windows/Scripts/exterior_materials.py:24, 278-409`.

**Boards:** `plan/canvas/FutureAtrium.dc.html`, `AsBuiltElan.dc.html`, `ElanSphere.dc.html`; `plan/README.md:113-131`.

---

## 12. First-pass cut

**Built tonight**, through the systems, from this sheet and the schedules:
- **Masonry:** the stone base wall with its door rough opening, cove recess, coping band and wall top; the exterior
  dress (one lathe); the entablature (one lathe, notched at the fins); the portal's block, opening and cornice.
- **Floor:** the eight marble rings with the lip, the notches and the door zone; the gilt waiting ring; the waiting
  mark.
- **Glazing:** both drum leaves, the head closer, the canopy's panes.
- **Steelwork:** the twelve ribs with their shoes, the ring and hangers, the canopy grid, the exterior fins, pedestals,
  mullions and transoms.
- **Lift installation:**
  - the lining, the rails (plain T-section) and their brackets;
  - the landing (panes, leaves, shoe, head ring, sill) and the tube;
  - the collar (lathe body and pocket) and the collar iris;
  - the car: panes, leaves, rings, floor, roof, handrail, guides, crown, both mast heads and plain columns.
- **Fittings and art:** the stele and foot; *The Starry Night* at true size in its gilt frame (the profile without its
  ornament), with its caption.
- **Light:** L1; L2 if the luminous-ceiling record lands tonight (otherwise the canopy is unlit glass, and T1/T2 fail
  honestly and are reported); L3; L4 on the Scheme's chosen mount; commissioning against §4.4.
- **Motion:** the car's prismatic travel with its five stops, speeds, jerk limit and sequences; both leaf pairs; the
  collar iris; interlocks A-1 … A-7 with Site's bindings (on the stand: blinds and blind wells).
- **Sound:** RT60 from the enclosure facts; footfalls.
- **Visitor:** the call and ride verbs.

**Polish** (later sessions):
- the bay letters A–K;
- the collar's spokes and gilt lines, and the ribs' bronze lines;
- the portal's architrave, keystone and socle mouldings;
- the frame's ornament;
- the rack-and-pinion and spiral-band drawings, and drums sized for their columns (R-7);
- the exterior night lighting (L6, L7);
- the rib uplights (L5, only if T3 fails);
- the mechanism voices (need recordings);
- splitting panes by stock if Form.WithinStock requires it (R-5).

---

## Open questions and risks (for the Architect)

**R-1 · Care against daylight.** *The Starry Night* faces west in a 1,335 m² luminous drum.
- Holding ≤ 200 lx needs a misty leaf of low transmittance, about 5–10 %, and a capped canopy.
- At clear-sky afternoon reference moments the sunlit drum may still push it over.
- The fix lives in Matter (the glazing make-up), never in a knob. If it can't hold, the owner decides: a darker drum,
  or turning or moving the painting (a design change).

**R-2 · The glowing canopy is now a luminaire.** No daylight can reach the Sphere's underside, and the as-built glow was
an emissive material. This sheet makes the canopy an edge-lit light-guide luminous ceiling: its panes are the
diffusers, and its LEDs sit in the grid's members, a real product class.
- Light needs the kind, a catalogue record with the maker's photometry, and the `laylight` control class.
- Until then the canopy is dark glass, and T1/T2 fail (honestly).

**R-3 · The Sphere seam is decided here, before `sphere.md` exists:**
- the sphere R 14.000 below the equator (the Sphere's lower shell and image surface must lie inside it: an image
  radius ≤ 13.8 is expected);
- the neck column r 3.000 from z 7.715;
- the entablature's top.

The Sphere's sheet must adopt it, or Site.NoOverlap and Site.PortsMate fail.

**R-4 · Closure placement.** Both vertical ports put their class iris at the receiving side's seal face, not on the
mating plane: 7.442 m down, and 3.681 m up. Site must place a closure at a declared depth (cube.md Q-2). If Site can't
tonight, the fallback is that each receiving stage describes its iris itself, with Site binding the interlocks.

**R-5 · Stock.** The drum's panes are 15° wide (3.65 m chord), beyond common float stock (3.21 m). Form.WithinStock may
split them on the 7.5° lines, adding visible joints. Oversize curved laminated low-iron glass exists; Matter's stock
rule decides.

**R-6 · Rails.** Rails are added in the landing and tube (not as built), with 30 mm breaks at the seam and none through
the collar (7.425 … the Sphere's neck). The guides ride across the breaks; the gap at the collar is 0.32 m. The
Sphere's neck should carry rails as the Cube's shaft does. Blind wells on the stand need rails too, or Motion's guide
contact is unsatisfied there.

**R-7 · The masts' drums.** The as-built crown (Ø 0.68 m) and under-floor collar (Ø 0.40 m) can't house 12.8 m and
about 9 m of spiral-band column. Motion.Housed will fail until they are sized.

**R-8 · Pearl steel's family.** The owner's language makes pearl steel the white family's Future finish: plaster, then
limewash, then pearl steel. Matter's `steel` record is an accent of **glass**. Matter.OneLanguage and the palette
(D-018) must settle it before the ribs, grid and collar resolve.

**R-9 · Missing Matter records:**
- workings `etched` (acid-etched satin) and ceramic frit, and the stacked-working question (laminated.uv-filtering +
  etched + fritted on one pane);
- substances `concrete`, `grout` and `gesso` (plus canvas, oil ground and varnish for the painting);
- the "mirror" grade of polished marble.

Without them, pieces fail "every material key resolved" (D-017).

**R-10 · Acoustics.** A predicted RT60 of about 7 s is the physics of this room, not a defect. The owner may hear it as
too live; the answer is a material choice (D-018), never a reverb knob. Sabine won't show the drum's centre focusing.

**R-11 · The Hall of Light's end.** The port sits on the base's face r 14.800 (as the Hall's old plan has it:
MuseePlan.h:282), not on the ashlar dress (r 14.880). The Hall's end stands inside the portal's opening. Its drafter
must meet the cylinder, and its footprint must stop at the notch. Site.NoOverlap must compare volumes (as the Grounds'
sheet assumes).

**R-12 · Re-expressions a builder must not undo:**
- the canopy's room face is at R 14.024 (was 14.000), and the grid carries the glass;
- the ribs stand 5 mm off the stone and glass;
- the wash and the order's root are at 14.080 and 14.074;
- the entablature is notched at the fins;
- the coping band stops at the ribs;
- the inlays sit in recesses.

Each exists so the pieces are solid and don't interpenetrate (Form.NoOverlap).
