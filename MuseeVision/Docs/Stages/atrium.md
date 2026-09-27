# The Atrium · design sheet

- **Stage key** `atrium` (module `MuseeAtrium`, `UAtriumRoom : USiteRoom`). Display name "The Atrium". Era: **Future**
  (Élan is three stages, D-012: the Atrium at level 0, the Cube below, the Sphere above). The Future's workings: mirror
  polish and clean gilt; oak absent; low-iron glass; pearl steel; Crema Marfil mirror-polished underfoot; travertine
  filled and polished at the 6 m base only.
- **Sources:** D-001…D-019 (D-003, D-005, D-006, D-012, D-014, D-017, D-018 binding); Palette.json and Story.md ("The
  Atrium (Future)", "Élan outside"); the drafter's spec `r-specs/…/Stages/atrium.md` (verified; corrections in §11); the
  finished `hall-of-light.md` (§2, `hall-of-light.east`); the drafts `cube.md` and `sphere.md` (for the seams); the boards
  `plan/canvas/FutureAtrium.dc.html`, `AsBuiltElan.dc.html`, `ElanSphere.dc.html`, `plan/README.md:113-131`; the as-built
  code in `windows/` (§11). Schedules: `Data/Stages/atrium/{Finishes,Lighting,Hang,Mechanisms}.json` (no Planting).
- **Units** metres. **Frame:** origin on the Élan axis on the finished floor (the marble's face, z 0.000), the centre of
  the car's way. x east, y south, z up, true north. **museum = local + (54.000, 0.000, 0.000)**, yaw 0; z 0 is the
  museum's floor datum (the Rotunda's floor, the Hall of Light's, the ground outside; `MuseePlan.h:19, 167`).
- **Bearings** are compass bearings, clockwise from north: a point at bearing b and radius r is (r sin b, −r cos b). The
  old code's plan angle (from +x towards +y) = bearing − 90°. Ribs and fins stand at bearings **15° + 30°k**; bay centres
  and mullions at **30°k**; the door bay is centred on **270°** (west).

## 1. Identity and placement

A drum 28 m across: a 6 m travertine base, misty glass above it to the equator at 22 m, and overhead the glazed, glowing
underside of the Sphere on a triangulated pearl grid, carried by twelve pearl ribs that rise from bronze shoes, turn at
9 m and run under the Sphere to a ring round the car's way. A floor of Crema Marfil polished to a mirror. One door, west,
from the Hall of Light. Eleven bays A–K for new art (empty tonight). At the centre a round glass car in a glass landing
joins the Cube 22 m below to the Sphere above; beside the landing's door *The Starry Night* stands on a glass stele,
the sign for the elevator.

| | |
|---|---|
| Origin in the museum plan | **(54.000, 0.000, 0.000)**, yaw 0 (`MuseePlan.h:167`, `E::Centre()`). Site derives it from `atrium.west ↔ hall-of-light.east` (§2) and must reproduce it. |
| Level | 0. The Sphere's centre is local (0, 0, 22.000); the Cube's (0, 0, −22.000). |
| Footprint kind | Building, in stacked prisms (the portal arches over the Hall of Light's end; the Cube lies below, the Sphere above). |
| Vertical extent | z −0.500 (the slab's soffit, the seam with the Cube) … 22.580 (the entablature's wash, the seam with the Sphere's dome). The served wells run on below and above (§2). |

**Key levels** (local z = museum z): slab soffit (`atrium.down`) −0.500; finished floor 0.000; cove top 0.140; rib shoes'
top 0.360; *The Starry Night*'s centre 1.380; stele top 2.200; landing head ring top 3.000; west door head 4.500; coping
band 5.720–6.000; **wall top 6.000** (the drum stands on it); exterior wash's inner edge and the order's seats 6.200;
collar soffit 7.455; collar iris top blade 7.695; **collar top 7.715** (`atrium.up`); car at the neck stop (home) 8.400;
ribs turn at 9.003; portal top 9.500; transoms 10.000, 14.000, 18.000; car at the Sphere stop 20.400; entablature
20.900–22.580; canopy meets the drum 21.180; equator 22.000.

**Footprint** (plan outline at the ground, closed, clockwise on a north-up plan): **G1 (−15.000, −6.050) → G2 (−14.162,
−6.050) → arc R 15.400 about (0, 0) clockwise through (0, −15.400), (15.400, 0), (0, 15.400) → G3 (−14.162, 6.050) → G4
(−15.000, 6.050) → G1** (the portal's front at x −15.000, museum 39.000). Stacked prisms:

| Prism | z | Plan |
|---|---|---|
| P1 | −0.500 … 5.480 | the disc r 15.300 (the socle) outside the west sector (bearings 248°–292°); inside it the disc r 14.800 plus the portal x −15.000 … the disc, \|y\| ≤ 6.050 |
| P1c | 5.480 … 6.200 | as P1 with r 15.400 (the plinth's corona) outside the west sector |
| P2 | 6.200 … 9.500 | the disc r 15.600 (fins and pedestals) outside ±25° of west; inside it the disc r 14.080 and the portal block |
| P3 | 9.500 … 22.580 | the disc r 15.700 (the entablature's corona oversails all) |
| Notch (all prisms) | | minus the Hall of Light's end: \|y\| < 4.950, x < −√(14.800² − y²), below the portal arch's soffit z < 2.200 + √(5.800² − y²) (8.000 at the crown, 5.221 at the jambs) |

**Seams.** *Atrium | Sphere:* below the equator and outside the neck column, the sphere R **14.000** about (0, 0, 22.000)
(the Atrium builds nothing inside it; the canopy's upper face lies on it); the neck column r 3.000 from the plane 7.715 up
to that sphere (z 8.323); above the equator everything inside r 14.080 and above the entablature's wash. *Atrium | Cube:*
the plane z −0.500. *Atrium | Hall of Light:* the base's face r 14.800 between the jambs (the port) and the notch.
*Atrium | Grounds:* the ground plane z 0 outside the socle; the terrace starts at r 15.32.

| Enclosure | Kind | Bounded by |
|---|---|---|
| `atrium.hall` | Enclosed | floor (into the cove to r 14.090); base inner face r 14.000 to z 6.000; drum inner leaf r 14.000 to 21.180; the canopy's room face (R 14.024 about z 22) in to the ring (r 3.600); the ring and the collar; the landing's and the tube's glass; the landing's leaves. ≈ 7,700 m³. Sealed while the landing's leaves are shut; with them open (the car at the landing) the well joins it, closed above by the collar iris or the Sphere's neck iris and below by the Cube's ceiling iris or the down blind. |

The car, the landing and the stele stand inside `atrium.hall`; the drum's cavity (r 14.024–14.050) is part of the
glazing make-up. **Openings (Form's one list):** Ports `atrium.west`, `atrium.down`, `atrium.up`; Glazed `atrium.drum`;
Housings `atrium.cove` (r 14.000–14.090, z 0.000–0.140) and `atrium.collar.pocket` (r 2.465–4.050, z 7.660–7.705).
The canopy is not an opening: the Sphere above it is opaque; its panes are a luminaire's diffuser (§4).

## 2. Ports

| Key | Kind | Class | Local frame (position; facing) | Museum frame | Face; depth | Mates with | Wells |
|---|---|---|---|---|---|---|---|
| `atrium.west` | doorway | `doorway.4000x4500.square` | (−14.800, 0.000, 0.000); **−x** | (39.200, 0, 0); −x | cylindrical R 14.800 about the stage's z axis (convex, the base's face); **0.800** (r 14.000 → 14.800) | `hall-of-light.east` (its frame (28.000, 0, 0) + (11.200, 0, 0) = museum (39.200, 0, 0), facing +x; concave R 14.800 about museum (54, 0)). **Passage 0.00.** | none |
| `atrium.down` | vertical (served well) | **`iris.4600`** | (0.000, 0.000, −0.500); **−z** | (54.000, 0, −0.500); −z | the plane z −0.500 (the slab's soffit); **0.500** (the slab and marble) | `cube.up` (museum (54.000, 0, −0.500), facing +z). Passage 0.00. | **serving**: the prism below, to −23.800 |
| `atrium.up` | vertical (served well) | **`iris.4600`** | (0.000, 0.000, 7.715); **+z** | (54.000, 0, 7.715); +z | the plane z 7.715 (the collar's top); **0.260** (the collar, 7.455 → 7.715) | `sphere.down` (museum (54.000, 0, 7.715), facing −z). Passage 0.00. | **serving**: the prism above, to +23.390 |

- **`doorway.4000x4500.square`** (as the Hall of Light's sheet; `MuseePlan.h:171`, `AtriumBaseStructure.cpp:66-67`): clear
  **4.000** (jambs y ±2.000), square head at **4.500**, jambs parallel; open portal (no leaves). Jambs meet the port face
  at x −14.664 and the inner face r 14.000 at x −13.856. **Rough opening** (the Atrium builds only this; a = 0.050): jambs
  y ±2.050, head 4.550, sill −0.050, straight through r 14.000 → 14.800; faces `atrium.port.rough-opening`. **Site's
  joint** builds the travertine threshold through both wall depths (base 0.800 + the hall's lining 0.050), the brushed
  CuSn10 strip on the mating cylinder, the linings and the surrounds. **Surround clearance:** the base's inner face is
  kept clear for \|y\| < 2.350, z < 4.850 (the cove stops at \|y\| 2.350, §3.2; the ribs stand at \|y\| ≥ 3.47).
- **`iris.4600`** (new; used by both vertical ports and by the Cube's `cube.up` and the Sphere's `sphere.down`): the class
  names the served well's clear diameter.
  - **Well section:** the circle r **2.300** about the axis, less two notches 16 mm wide (tangential ±0.008) from r 2.265 to
    2.300 at bearings 0° and 180° (the guide rails' blades stand in them).
  - **Closure:** a seven-blade iris in the horizontal plane. Each blade a 110° arc of the middle circle r 3.095, 1.250
    wide with round ends, **2 mm thick at a 4 mm pitch** (the stack 26 mm deep from the top blade's face); pins at r
    3.745, plan angles 360°k/7 (k 0…6, pin 0 due east), blade k lying 4k mm below the top; it turns 52° on its pin, eased
    over **4.0 s**. Open, the blades' inner edge stands at r 2.470 in a seat r ≥ 2.460 and the blades reach r 4.010 in
    travel; each blade's pin-end is lengthened by a **pin lug**, a round boss r 0.060 about the pin joined to the round end by a 0.120-wide web on the pin's radius, so the blade encloses its pin (as built the pin stood 25 mm outside the blade). States: **shut 0**, **clamp 0.3206** (round a mast head's groove r 0.195; the blades leave r 0.197; the channel maps linearly to the blade angle (Motion `follows`), so the as-built eased clamp 0.378 is **0.3206** here, the same blade pose), **open 1**. Checked by exact polygon union (shapely): shut, the seven cover the well r 2.300 with no gap; clamp leaves r 0.197; open, the inner edge is at r 2.470; in travel the reach is 4.009 (lug included); no blade ever passes over another blade's pin, so each pin runs through the stack to its housing.
  - **Ownership rule:** the closure belongs to the **stage below the mating plane**, at the seal depth that stage declares:
    `atrium.up`'s is the Atrium's collar iris, top blade face **0.020 below** the plane (z 7.695); `atrium.down`'s is the
    Cube's ceiling iris, top blade face **7.442 below** the plane (museum z −7.942, `CubePlan.h:62`). Each publishes
    `<its stage's up port>.closure`: `atrium.up.closure`, `cube.up.closure` (0…1).
  - **Joint:** no threshold at a vertical port; the faces touch. Anything under the lining (the Cube's shaft ring) is the
    receiving side's.
- **The wells (served by the Atrium; the car is the Atrium's).** The receiving stages reserve the well prism and build
  nothing inside it but what the class allows: *below*, from −0.500 to **−23.800** (the car's lowest point at the Cube
  stop is −23.760), the Cube's rails in the notches; *above*, from 7.715 to **+23.390** (the car's highest point at the
  Sphere stop is 23.340), the Sphere's rails in the notches of its neck and the Sphere's mast (r ≤ 0.260) under the car.
  The Atrium owns what moves in them: the car, its leaves, guides and light, the crown and the Cube mast (§3.6, §6).
- **Closed states and blinds.** `atrium.west`: an open portal when mated (tonight); on the stand a closed pair of cast
  CuSn10 leaves `atrium.west.blind-doors` fills the class opening on the port face, meeting rail on the axis.
  `atrium.up`: its closure is the Atrium's own collar iris; **unmated, it stays shut** (no other blind) and the car never
  rises above the Atrium stop. `atrium.down`: **unmated, Site sets the class closure shut on the mating plane**,
  `atrium.down.blind-iris` (z −0.500, blades facing up into the lining), with a blind well to −23.800 only on the stand;
  the car never goes below the Atrium stop. Tonight each vertical port is mated only if its stage is built.
- **Topology (D-012):** `hall-of-light.east ↔ atrium.west` (the Atrium's only door); `atrium.down ↔ cube.up`;
  `atrium.up ↔ sphere.down`. The glass car never lands in the Cube (no landing, door, vestibule or light lock there); its
  one landing is the Atrium's; at the Sphere it stops in the platform's opening (the Sphere's). No stage pushes a sky.
- **What the neighbours must match.** *Cube:* `cube.up` at museum (54, 0, −0.500) facing +z, class `iris.4600`, its
  ceiling iris at museum −7.942 publishing `cube.up.closure`, clamping the Cube mast's head with the car at −10.825; rails
  in the notches from −0.520 down. *Sphere:* `sphere.down` at museum (54, 0, 7.715) facing −z, class `iris.4600`; its
  throat (r 2.550–2.980) from 7.715; rails in the notches from 7.745 to its mouth; its upper neck iris (top face 11.396,
  `sphere.neck.closure`), its platform iris (`sphere.platform.closure`), its platform call (Use `sphere.call`) and its
  mast rising from the mouth (§6).
- **Coordination.** The Hall of Light's end stands inside the portal's opening and meets the port face r 14.800 (not the
  ashlar r 14.880). Above the coping (6.200) between the hall's lunette (R 14.800) and the drum's outer leaf the air is
  outdoors, under the portal. The plinth's corona (r 15.400, z 5.48–5.86) oversails the Hall of Light's gardens by 0.100
  between bearings 293° and 302.5° (and 237.5°–247°); the gardens' volumes stop under it. The Grounds' terrace starts at
  r 15.32.

### Anchors (the stage contract)
Bodies are drawn in their closed pose at the stage positions below. Anchors on `atrium.car` ride it.

| Key | Kind | Frame (origin; axes) | Belongs to |
|---|---|---|---|
| `atrium.car` | Body | (0, 0, 0): the car's floor datum on the axis at the **Atrium stop**; axes of the stage; moves along +z from −23.600 to +20.400 | the car (§3.6): glass, rings, floor, roof, handrail, guides, crown, the under-floor seat, the light ring |
| `atrium.car.mast.head` | Body | (0, 0, 2.780): the crown's top, on the car; moves +z relative to the car 0 → 12.775 | the Cube mast's head and the column below it |
| `atrium.car.leaf.north`, `atrium.car.leaf.south` | Body | (0, 0, 0) on the car, axes of the stage; each turns about the z axis | the car's two leaves |
| `atrium.landing.leaf.north`, `atrium.landing.leaf.south` | Body | (0, 0, 0), axes of the stage; each turns about the z axis | the landing's two leaves |
| `atrium.up.iris.blade.1`, `atrium.up.iris.blade.2`, `atrium.up.iris.blade.3`, `atrium.up.iris.blade.4`, `atrium.up.iris.blade.5`, `atrium.up.iris.blade.6`, `atrium.up.iris.blade.7` | Body | each at its pin on its top face: (3.745, 0.000, 7.695), (2.335, 2.928, 7.691), (−0.833, 3.651, 7.687), (−3.374, 1.625, 7.683), (−3.374, −1.625, 7.679), (−0.833, −3.651, 7.675), (2.335, −2.928, 7.671); axes of the stage; drawn shut | the collar iris (the closure of `atrium.up`) |
| `atrium.call` | Use | (−3.310, 0.000, 0.000): the waiting mark's centre. Usable: feet on the floor (\|z\| < 0.50) outside the landing (r > 2.490) within r 8.000, or a look at the landing's glass from outside it | calling the car |
| `atrium.car.panel` | Use | (0, 0, 0) on `atrium.car`. Usable: feet inside the car (r < 2.130) within 0.30 of its floor | the ride verbs (§6, §10) |
| `atrium.stele` | Wall | (−3.840, −2.650, 0.000): the stele glass's west face at its north end, on the floor; x (0, 1, 0) (south, the viewer's right); y (−1, 0, 0) (west, towards the viewer); z up | *The Starry Night* |
| `atrium.bay.a`, `atrium.bay.b`, `atrium.bay.c`, `atrium.bay.d`, `atrium.bay.e`, `atrium.bay.f`, `atrium.bay.g`, `atrium.bay.h`, `atrium.bay.i`, `atrium.bay.j`, `atrium.bay.k` | Wall | on the base's inner face r 14.000 at the left rib's side face, z 0; x along the face towards increasing bearing; y towards the axis; curved (Hang.json `walls`, arc 7.030 each) | the eleven empty bays, centred on bearings 300° (A), 330°, 0°, 30°, 60°, 90° (F, seen through the car), 120°, 150°, 180°, 210°, 240° (K) |

No Mount, Bed, Water, Seat or Glazing anchors: no seats as built; luminaires stand at the points in Lighting.json (the
car's carry `body: atrium.car`). **Channels** (Motion): `atrium.car.height` (published: (z + 23.600) ÷ 44.000),
`atrium.car.doors`, `atrium.landing.doors`, `atrium.up.closure`, `atrium.car.light` (set by the car's sequences, §6).
Referenced across seams: `cube.up.closure`, `sphere.neck.closure`, `sphere.platform.closure`; Use `sphere.call`.

## 3. Pieces by trade

Every piece is one solid of one substance meeting its neighbour face to face (the as-built embeds are not ported).

### 3.1 Structure (hidden)
- **Slab** `atrium.slab`: r 2.560 … 14.000, z −0.500 … −0.030, under the marble; its edge bears on the base wall's footing.
- **Footing:** the base wall continues down to −0.500 (`atrium.base`); the marble's last ring and the cove floor lie on it.

### 3.2 Masonry
**The base** `atrium.base` (inner stone): annulus r **14.000 … 14.800**, z −0.500 … 6.000, laid as ashlar by Form, vertical
joints on the rib lines (behind the ribs) and on the bay centres. Outside the west sector the exterior dress runs its
face out to r 14.880; inside it (bearings 248°–292°) the face stays r 14.800, where the Hall of Light and the portal meet
it. The door's rough opening as §2. As built there were pilasters; `bPilasters` was false (retired).
- **Cove** (Housing `atrium.cove`): recess r 14.000 … 14.090, z 0.000 … 0.140, all round but the door bay; square ends
  where the inner face is **0.350** short of each jamb (y ±2.350). The marble runs into it to r 14.090.
- **Coping band** (inside, lathed between the ribs), section (r, z): (14.000, 5.720) → (13.900, 5.720) → (13.900,
  5.970) → (13.930, 6.000) → (14.000, 6.000): 100 mm proud, 30 mm chamfer on the top arris, stopping 5 mm short of each
  rib's side faces, square returns.
- **Wall top** z 6.000 from r 13.930 to 14.880: the drum's two leaves stand on it; the coping wash sits on its outer part.

**The exterior dress** `atrium.dress`, lathed outside the west sector (bearing 292° clockwise to 248°). Section (r, z),
arcs from start to end angle (0° = +r, 90° = +z): (14.880, −0.100) → (15.300, −0.100) → (15.300, 0.400) → (15.260, 0.440)
→ (15.140, 0.440) → torus about (15.140, 0.540) R 0.100, −90° → 90° → (15.080, 0.640) → (15.080, 0.680) → scotia about
(15.080, 0.860) R 0.180, −90° → −180° → (14.880, 0.880) → (14.880, 5.200) (ashlar face) → (14.940, 5.200) → (14.940,
5.280) → ovolo about (14.940, 5.480) R 0.200, −90° → 0° → (15.400, 5.480) → (15.400, 5.860) (corona) → (15.360, 5.860) →
(15.360, 5.890) → bead about (15.240, 5.890) R 0.120, 0° → 90° → (15.200, 6.010) → (15.200, 6.050) → wash to (**14.080**,
6.200) → (14.080, 6.000) → along the wall top back to (14.880, 6.000). Every vertical of the order stands on a **level
seat** cut into the wash at z 6.200 (±0.280 tangential at the fins, ±0.070 at the mullions).

**The entablature at the equator** `atrium.dress`, lathed all round, notched round each fin (±0.170 tangential at bearings
15° + 30°k, from 20.900 to 22.050): (14.080, 20.900) → (14.620, 20.900) → (14.620, 21.100) → (14.700, 21.100) → (14.700,
21.340) → (14.760, 21.340) → (14.760, 21.400) → (14.660, 21.400) → (14.660, 21.840) → (14.700, 21.840) → ovolo about
(14.700, 22.050) R 0.210, −90° → 0° → (15.700, 22.050) → (15.700, 22.300) → (15.660, 22.300) → (15.660, 22.330) → bead
about (15.520, 22.330) R 0.140, 0° → 90° → (15.480, 22.470) → (15.480, 22.500) → wash to (14.080, 22.580) → down the
inner face to (14.080, 20.900). The Sphere's dome shell rises from the wash (the Sphere's).

**The portal** `atrium.dress` (after the Pantheon's intermediate block): front plane x −15.000, \|y\| ≤ 6.050, z −0.100 …
9.500; its back meets r 14.050 above z 6.000 and the base's face r 14.800 below. Opening: jambs y ±4.950 from −0.100 to
5.221, then an arch R 5.800 about (y 0, z 2.200), crown 8.000, straight through to the drum and base (clears the hall's
kerbs by 0.150 and its vault's crown by 0.500). **Socle** round each pier's front, section (a out, z): (−0.020, −0.100) →
(0.420, −0.100) → (0.420, 0.400) → (0.380, 0.440) → (0.260, 0.440) → torus about (0.260, 0.540) R 0.100, −90° → 90° →
(0.200, 0.640) → (0.200, 0.680) → scotia about (0.200, 0.860) R 0.180, −90° → −180° → (0.000, 0.880) → (−0.020, 0.860);
north path (−13.300, −6.050) → (−15.000, −6.050) → (−15.000, −4.980), south mirrored. **Cornice** (a, z): (0, 8.750) →
(0.050, 8.750) → (0.050, 8.820) → ovolo about (0.050, 9.000) R 0.180, −90° → 0° → (0.450, 9.000) → (0.450, 9.320) →
(0.410, 9.320) → (0.410, 9.350) → bead about (0.300, 9.350) R 0.110, 0° → 90° → (0.260, 9.460) → (0.260, 9.500) → (0,
9.500) → (−0.020, 9.480) → (−0.020, 8.770); path (−12.680, −6.050) → (−15.000, −6.050) → (−15.000, 6.050) → (−12.680,
6.050), mitred. **Architrave** (polish), (a, b away from the opening): (−0.020, 0.030) → (0.060, 0.030) → (0.060, 0.120)
→ (0.080, 0.120) → (0.080, 0.360) → arc about (0, 0.360) R 0.080, 0° → 90° → (0, 0.440) → (−0.020, 0.440), up the north
jamb from z 0.700, round the arch, down to 0.700. **Keystone** (polish): prism (y, z) (−0.220, 8.020) (0.220, 8.020)
(0.310, 8.900) (−0.310, 8.900), a −0.020 … 0.130.

### 3.3 The floor
**Marble** `atrium.floor`, 30 mm (z −0.030 … 0.000), laid by Form in rings of radial slabs, **6 mm** joints:

| Ring | r | Slabs |
|---|---|---|
| 1 | 2.290 (the lip) … 2.940 | 12 |
| the waiting ring | 2.940 … 3.060 | — (`atrium.waiting-ring`) |
| 2 · 3 · 4 | 3.060 … 4.583 · … 6.107 · … 7.630 | 12 · 24 · 24 |
| 5 · 6 · 7 | … 9.153 · … 10.677 · … 12.200 | 36 · 36 · 48 |
| 8 | 12.200 … 14.090 (into the cove) | 36 |

- Radial joints at bearings 15° + k·360°/n (one on every rib line). The door zone: the marble stops on the base's inner
  face between the jambs; the joint's threshold takes over.
- **Lip:** ring 1's inner face r 2.290 from 0.000 to −0.030, its underside running out over the lining's top; lip and
  lining **notched at bearings 0° and 180°** for the rails (blade slot 16 mm to r 2.385; foot slot 140 mm from 2.385 to
  2.430). Elsewhere it clears the car's rings (r 2.235) by 55 mm.
- **Waiting ring** `atrium.waiting-ring`: annulus r 2.940 … 3.060, z −0.030 … 0.000, flush. **Waiting mark** (same
  surface), 2 mm proud in 10 mm recesses (z −0.010 … 0.002): a half ring r 0.380 … 0.460 about (−3.310, 0) on its west
  half, open towards the car, and a dot r 0.140 at (−3.650, 0).
- **Bay letters A–K** `atrium.letters` (polish): 0.300 high, Didot-like, at r 13.100 on the bay-centre bearings, each
  letter's top facing its bay, 2 mm proud in 10 mm recesses; strokes as `AtriumBaseStructure.cpp:534-725`.
- **Rib feet:** each rib's foot plate bears on the slab through a cut in the marble, under its shoe.

### 3.4 Glazing (Glazed `atrium.drum`, weather **museum**)
- **Inner leaf, the misty glass** `atrium.drum.inner`: cylinder r **14.000 … 14.024**, z 6.000 … **21.180** (where the
  canopy meets it); laminated 2 × 12 mm on a UV-filtering interlayer, room face acid-etched, cavity face a white ceramic
  frit. Panes 15° wide, joints at bearings 15°k (behind the ribs, fins and mullions), four tiers split at 10.000, 14.000,
  18.000: 96 panes, 8 mm silicone butt joints. The chord (3.654) exceeds common float stock; Form.WithinStock may split
  each on its 7.5° line.
- **Head closer** `atrium.canopy.grid`: a ring r 14.000 … 14.080, z 21.180 … 21.200, capping the leaf and the cavity.
- **Outer leaf** `atrium.drum.outer`: cylinder r **14.050 … 14.074**, z 6.000 (in a channel on the wall top) … 20.950,
  clear heat-strengthened laminated; panes between the order's verticals, 15° wide, four tiers (6.000 | 10.000 | 14.000 |
  18.000 | 20.950). The cavity r 14.024 … 14.050 is sealed top and bottom.
- **Canopy** `atrium.canopy.glass` (the Sphere's underside, glazed): a spherical zone, upper face R **14.000** (on the
  seam), room face R **14.024** about (0, 0, 22.000), from the ring (r 3.600, z 8.446) to the drum (r 14.000, z 21.180):
  the triangles of the grid (≈ 2,350 panes, 0.6–1.4 m a side), 2 × 12 mm laminated, the room face acid-etched in a
  light-extraction pattern; each pane bears on the grid members' tops. The outermost band (ring 1 to the drum) butts the
  inner leaf at 21.180 with a silicone joint.

### 3.5 Steelwork (pearl steel)
**Ribs** `atrium.rib`, twelve, in vertical planes at bearings 15° + 30°k: one constant section swept along a spine, the
rib's back line in (r, z): (1) up the wall, vertical at r **13.995** (5 mm off the stone and glass) from 0.000 to **9.003**;
(2) the turn, an arc R **3.000** about (10.995, 9.003) from 0° to 130.230° (from +r towards +z), ending at (9.057,
11.293) tangent to the canopy; (3) under the Sphere, an arc R **14.024** about (0, 22.000) from −49.770° to −75.125°,
ending on the ring's outer face r 3.600, capped. Section (d from the back into the room, t tangential): (0, −0.150) →
(0.390, −0.150) → (0.420, −0.120) → (0.420, 0.120) → (0.390, 0.150) → (0, 0.150): 0.420 × 0.300, 30 mm chamfers on the
room-side arrises. **Shoe** `atrium.rib.shoe`: sleeve (0, −0.165) → (0.435, −0.165) → (0.435, 0.165) → (0, 0.165), z
0.000 … 0.360, flat top. **Line** `atrium.rib.line` (polish): 24 mm wide, 2 mm proud (d 0.420 … 0.422), from the shoe's
top to the ring.

**Compression ring** `atrium.canopy.grid`, lathed, (r, z): (3.000, 7.745) → (3.600, 7.890) → (3.600, 8.446) → (3.000,
8.301): its top on the canopy's room face, its bore r 3.000 the neck column. **Hangers** ×12 (same surface), 10 mm plates,
radial r 3.150 … 3.450 at the rib bearings, from the collar's top (7.715) up to the ring's sloping soffit: they carry the
collar.

**Canopy grid** `atrium.canopy.grid` (after the Great Court): members **60 wide × 120 deep**, top face on the canopy's
room face R 14.024, depth along the Sphere's radius to R 14.144. Nodes on 20 rings at the as-built polar angles (equal
steps of the conformal coordinate ln tan(θ/2), θ from the Sphere's south pole), projected onto the room face. (r, z) from
the rim inwards: rings 1–14 (72 nodes, 5° apart): (13.795, 19.478) (13.573, 18.474) (13.285, 17.507) (12.936, 16.585)
(12.537, 15.715) (12.095, 14.901) (11.619, 14.148) (11.120, 13.455) (10.604, 12.822) (10.079, 12.249) (9.552, 11.732)
(9.030, 11.270) (8.515, 10.857) (8.014, 10.491); rings 15–19 (36 nodes, 10° apart): (7.087, 9.898) (6.232, 9.437)
(5.458, 9.082) (4.765, 8.810) (4.149, 8.604); ring 20 lies on the compression ring's face (3.600, 8.446). Ring 1's nodes
at bearings 90° + 5°j, alternate rings offset half a step; the first 36-node ring stands over every other node of ring 14,
then they alternate. Members: ring members, and each node to the two nearest nodes of the ring outside (at the 72 → 36
change, each inner node fans to three). Members stop 5 mm short of the ribs' side faces and of the ring. The top channel
of every member holds the canopy's edge-lit LED line (§4).

**The drum's order** `atrium.order`, outdoors; root r **14.074** (the outer leaf's face); B = the seat, 6.200 on the wash,
or 9.500 on the portal's roof within ±25° of west (the fins at 255°, 285° and the mullion at 270°):

| Member | Count, bearings | Section (r, z), extruded tangentially | Width |
|---|---|---|---|
| Fin | 12, 15° + 30°k | (root, B + 0.800) → (15.000, B + 0.800) → (14.800, 19.400) → elliptical arc about (15.600, 19.400), radii 0.800 (r) and 2.680 (z), 180° → 90° → (15.600, 22.080) → (root, 22.080); head cut level at the corona's soffit 22.050 | ±0.170 |
| Pedestal | 12, under each fin | (root, B) → (15.100, B) → (15.100, B + 0.780) → (15.030, B + 0.850) → (root, B + 0.850) | ±0.280 |
| Mullion | 12, 30°k | (root, B) → (14.450, B) → (14.450, 20.950) → (root, 20.950) | ±0.070 |
| Transom | 3 rings, z 10.000, 14.000, 18.000 | (root, z ∓ 0.050) out to 14.300, between the verticals | — |

### 3.6 The lift installation (the Atrium serves both wells)
- **Lining** `atrium.lining`: cylinder shell r 2.300 … 2.560, z −0.500 … −0.030, slotted at bearings 0° and 180°.
- **Guide rails** `atrium.rail`: T-section, blade r 2.265 … 2.385, 16 mm thick; foot r 2.385 … 2.425, 140 wide; at
  bearings 0° and 180° from −0.490 to **7.425** (230 mm under the iris stack), a rack on each rail's back; brackets 180 ×
  100, r 2.421 … 2.450, fixing the foot to the landing's head ring (3.000) and the tube's ring frame (5.358), never through
  glass. The foot bears in the lining's slot on the slab. The breaks (30 mm at −0.520 over the Cube's rails; 0.320 at the
  collar under the Sphere's neck rails) are bridged by the car's second ring of guides.
- **Landing** (as built "a glass enclosure Ø 4.9 m, door west"): panes `atrium.landing.glass` r 2.450 … 2.465, z 0.060 …
  2.940, spans clockwise from the door's centre line (bearing 270°): doorway −24° … +24° (1.993 clear chord); pocket panes
  24°–48° and 312°–336°; large panes 48°–136°, 136°–224°, 224°–312°. **Leaves** ×2 (same surface) r 2.375 … 2.390, z
  0.008 … 2.940, closed each from the centre line to 25°, gilt leading fins 12 × 25. **Shoe** `atrium.landing.bronze` r
  2.360 … 2.490, z −0.010 … 0.070 (in a 10 mm rebate in ring 1), open across the doorway; **head ring** r 2.360 … 2.490,
  z 2.930 … 3.000; both notched round the rails. **Sill** `atrium.landing.sill` r 2.245 … 2.515, z −0.030 … 0.008, across
  the door ±26°, set into the marble, bridging to the car's base ring.
- **Tube** `atrium.landing.glass` r 2.450 … 2.465, two tiers 3.000 … 5.3375 and 5.3775 … 7.685 (the upper runs 0.230 up
  into the collar's hub bore), six panes a tier (the landing's divisions plus one over the door); **ring frame**
  `atrium.landing.bronze` r 2.410 … 2.480, z 5.3375 … 5.3775.
- **Collar** `atrium.collar` ("pearl, rimmed in gilt, 8.2 m across"), lathed (r, z): (2.465, 7.455) → (2.800, 7.455) →
  (2.800, 7.545) → (3.620, 7.545) → (3.620, 7.455) → (4.000, 7.455) → (4.020, 7.465) → ovolo about (4.020, 7.545) R 0.080,
  −90° → 0° → (4.100, 7.545) → (4.100, 7.715) → (2.465, 7.715) → close. **Spokes** (polish): 24 at bearings 15° + 15°k,
  centred r 3.210, half-length 0.414, 44 wide, 50 deep from the coffers' back (7.545). **Gilt** `atrium.collar.gilt`
  (polish): hub liner r 2.360 … 2.469, 7.415 … 7.715; tori (major r, tube r, z) (2.475, 0.008, 7.453), (2.796, 0.007,
  7.457), (3.624, 0.007, 7.457), (3.960, 0.016, 7.459); lip band r 4.090 … 4.120, z 7.685 … 7.715. **Pocket**
  `atrium.collar.pocket` r 2.465 … 4.050, z 7.660 … 7.705.
- **Collar iris** (the closure of `atrium.up`, class `iris.4600`): bodies `atrium.up.iris.blade.1` … `.7` at the pins of
  the Anchors table; undersides `atrium.up.iris.under`, tops `atrium.up.iris.top`; top blade face 7.695, stack bottom
  7.669; shut, it seals the hub bore r 2.465.
- **The car** (body `atrium.car`; car frame = its floor datum on the axis):

| Part | Geometry (car frame) | Surface |
|---|---|---|
| Panes | r 2.185 … 2.200, z 0.020 … 2.535, set 5 mm into both rings; spans from the door's centre line (west): 24°–92°, 92°–180°, 180°–268°, 268°–336° | `atrium.car.glass` |
| Leaves ×2 (`atrium.car.leaf.north`, `.south`) | r 2.135 … 2.150, z 0.025 … 2.535; closed each 0°–25° from the centre line; open, turned 24° behind the panes; fins 12 × 25 | `atrium.car.glass`; fins `atrium.car.bronze` |
| Base ring · top ring | r 2.130 … 2.235, z −0.070 … 0.025 · 2.530 … 2.600 | `atrium.car.bronze` |
| Floor | disc r ≤ 2.135, z −0.030 … 0.020 (walking face 20 mm over the stop level) | `atrium.car.floor` |
| Roof | clear disc r ≤ 2.135 at z 2.575, inside the top ring | `atrium.car.glass` |
| Handrail | glass rod Ø 36 on r 2.000 at z 0.970, 30° → 330° from the door, ball ends; brackets at 60°, 120°, 180°, 240°, 300°, r 2.000 … 2.196, 12 × 12 | rod `atrium.car.handrail`; brackets `atrium.car.bronze` |
| Roller guides | on both rings at bearings 0° and 180°: blocks r 2.229 … 2.290, ±0.045 tangential, the rings' height; rollers either side of the blade, a pinion on the rack (a declared rolling contact) | `atrium.car.bronze` |
| Crown | gilt drum r 0.340, 2.583 … 2.780, on four arms at bearings 45°, 135°, 225°, 315° (r 0.310 … 2.225, z 2.600 … 2.632); bead round its top | `atrium.car.bronze` |
| Cube mast | head r 0.260 (body `atrium.car.mast.head`), groove r 0.195 from 0.070 to 0.110 over the crown top, cap to 0.160 (2.940 above the car floor: the car's highest point); the spiral-band column r 0.170 between crown and head, up to 12.775 | `atrium.car.mast` |
| Under-floor seat | drum r 0.200, z −0.160 … −0.030 ("where the mast meets the car"): the Sphere's mast bears here | `atrium.car.mast` |
| Light ring | opal slot r 2.060 … 2.140, z 2.495 … 2.540 (Light's, §4) | — |

### 3.7 Fittings
**The Starry Night's stele** (Art's fitting, the Salon stele's twin): glass `atrium.stele.glass` x −3.840 … −3.780, y
−2.650 … −1.150, z 0.060 … 2.200 (40 cm of clear glass over the frame); foot `atrium.stele.foot` x −4.010 … −3.610, y
−2.750 … −1.050, z 0.000 … 0.080, top edges eased 6 mm, the glass set 20 mm into it. Collision: the box of the foot's plan
to z 2.200 blocks the body only; the look passes to the painting.

### 3.8 What the Atrium does not build
Site's joint: the west threshold, strip, linings and surrounds; the blinds (`atrium.west.blind-doors`,
`atrium.down.blind-iris`) while unmated. The Cube: everything below −0.500 (shaft glass, sleeve, rails, its ceiling iris
and housing). The Sphere: everything inside R 14.000 and above 7.715 in the neck column (throat, neck rails, irises,
platform, its mast, the dome shell and crown). The Hall of Light: its end inside the portal. The Grounds: the terrace from
r 15.32. Light: luminaire bodies from their records. Art: the canvas and its frame.

### 3.9 Finishes (summary; Finishes.json holds the notes)

| Surfaces | Material · working · history |
|---|---|
| `atrium.floor` | `stone.crema-marfil` · mirror-polished · foot-worn |
| `atrium.base` | `stone.travertine-romano` · filled.polished · new |
| `atrium.dress` | `stone.travertine-romano` · honed · weathered |
| `atrium.port.rough-opening` | `stone.travertine-romano` · sawn · new |
| `atrium.slab` | `accent.concrete` · cast-in-situ.sealed · new |
| `atrium.drum.inner` | `glass.low-iron` · laminated.uv-filtering.acid-etched.fritted · new |
| `atrium.drum.outer` | `glass.low-iron` · heat-strengthened.laminated.uv-filtering · weathered |
| `atrium.canopy.glass` | `glass.low-iron` · laminated.acid-etched · new |
| `atrium.rib`, `atrium.canopy.grid`, `atrium.collar`, `atrium.up.iris.top`, `atrium.down.blind-iris` | `white.pearl-steel` · pearl.satin · new |
| `atrium.order` | `white.pearl-steel` · pearl.fluoropolymer · weathered |
| `atrium.up.iris.under` | `white.pearl-steel` · gilt.gold-leaf.oil · new |
| `atrium.rib.line`, `atrium.letters`, `atrium.collar.gilt`, `atrium.landing.bronze`, `atrium.car.bronze`, `atrium.car.mast` | `bronze.cusn10` · gilt.gold-leaf.oil · new |
| `atrium.rib.shoe`, `atrium.stele.foot`, `atrium.west.blind-doors` | `bronze.cusn10` · mirror-polished.lacquered · new |
| `atrium.waiting-ring`, `atrium.landing.sill` | `bronze.cusn10` · mirror-polished · foot-worn |
| `atrium.lining` | `bronze.cusn10` · patinated.dark · new |
| `atrium.rail` | `steel.hard-chrome` · ground.hard-chromed · new |
| `atrium.landing.glass`, `atrium.car.glass`, `atrium.stele.glass` | `glass.low-iron` · toughened.laminated · new |
| `atrium.car.floor` | `glass.low-iron` · laminated.acid-etched · foot-worn |
| `atrium.car.handrail` | `glass.low-iron` · toughened · handled |
| `atrium.frame.gilt` | `oak.sessile-french` · gilt.gold-leaf.water · new |

## 4. Light

**Daylight.** One opening, `atrium.drum` (≈ 1,335 m² seen from inside): the clear outer leaf, then the fritted and
etched inner leaf, which scatters it: a luminous band 15 m tall all round, no sharp sun patches; the sunny side glows
brighter, the order's fins and transoms read as soft bands. UV filtered by the interlayers. **No controls** (D-014). The
canopy gets no daylight (the Sphere is opaque, `ElanExteriorStructure.h:52-59`); by night the drum glows from outside
with the interior's own light. No emissive skins.

**Luminaires** (Lighting.json, 70; points and aims local, the car's at its Atrium stop; no output values here)

| Ids | Type | Count | Point and aim | Operation |
|---|---|---|---|---|
| `cove.a1` … `cove.k3` | `linear-cove.3000k` | 33 (3 per bay, bays A–K; none in the door bay) | flexible strip bent to the cove's arc r 14.070, z 0.0425, each run 2.279 long (the bay's arc less the shoes and 0.100 clear); facing the room tilted 20° down: aim the floor at r 13.953 | dusk (daylight-linked dimming holding the floor target) |
| `canopy.i01` … `canopy.i12`, `canopy.o01` … `canopy.o12` | `gap.edge-lit-glass.3000k` | 24 zones: inner ring r 3.600–8.500, outer 8.500–14.000, each 30° centred on the bearings 30°k | LED lines in the grid members' top channels bounding the zone's panes, lighting the panes through their edges; point at the zone's centre on the room face (r 6.0 or 11.0), aim straight down | always: follows the museum observer's daylight up to its commissioned level, never below 0.20 of it (as built) |
| `car.ring.01` … `car.ring.12` | `linear-slot.3000k` | 12 arcs of 30° (1.100 each) | in the car's top ring on r 2.100, z 2.5175, facing down; `body: atrium.car`, `follows: atrium.car.light` | presence (full while occupied or moving; 0.03 at the Cube stop) |
| `starry-night` | `track-framing.3000k.medium` | 1 | on a 0.150 bracket off the room face of the rib at bearing 285° (the door bay's north rib), (−12.968, −3.475, 6.000); aim the canvas centre (−3.885, −1.900, 1.380): throw 10.3, 28° off the canvas normal (its reflection falls to the floor 2.7 m west, under every eye); shutters framed to 1.036 × 0.852 | presence (holds the care target when daylight falls short) |

Reserved (polish, only if a design target fails or the Grounds schedule night façade light): 12 in-ground uplights on the
rib lines at r 12.700; fin uplights on the coping; dome washes on the cornice (the Sphere's target). The as-built 3800 K
cove, 5000 K car light, 24 invisible rect "laylights" and 900/250 cd/m² emissive glass are not ported.

**Targets.** `atrium.floor` `entrance-hall` 100 lx (UseTargets names the Atrium an arrival space), after dusk
`circulation.night` 50 lx; `atrium.car.floor` `lift` 100 lx (and the landing floor inside r 2.450); *The Starry Night*
(measured on the canvas, surface `atrium.frame.gilt`) `oil-and-tempera` 150–200 lx, Care. No walked route in any
configuration below `escape-route`. **Care against daylight:** the painting faces west in a 1,335 m² luminous drum; by day
the diffuse drum may exceed 200 lx on it. The fix lives in the glass's make-up (the frit's transmittance is Matter's
measured record) and is measured and reported as a finding by Art.WithinCare, never dimmed or tuned.

**Design order** (Lighting.json `design`; measured, never tuned): the canopy ≥ the drum's inner leaf at night and at
overcast moments (a glowing ceiling); the ribs' room faces ≥ 0.25 × the canopy behind them (pearl, never silhouettes);
at night the canopy is the brightest extended surface. *The Starry Night* reads as "a small square of night in a daylit
room": its own low luminance is the design.

## 5. Time and weather
- **Weather:** the outer leaf, the plinth's coping and wash, the entablature's wash and the portal's roof take the
  **museum** observer (D-005): rain wets the outer glass and stone, snow lies on the washes and the portal's roof. The
  inner leaf and the canopy are indoors and dry.
- **Sun** (D-003, idealised): it lights the drum from outside and enters diffused; the glow band moves round the drum with
  the hour. **Night** (D-006, always open): the cove switches on at dusk, the canopy falls to its night floor, the drum
  reads from the Grounds as a lantern lit from within. No plants: no solar terms. The Sphere's sky is the Sphere's.

## 6. Motion
All mechanisms are the Atrium's (Mechanisms.json, 13). With the Cube's ceiling iris and the Sphere's neck and platform
irises they form one coupling group; Site binds the channels across the seams.

**M-1 `atrium.car.lift`** (body `atrium.car`): prismatic +z, travel **44.000** (z −23.600 … +20.400; the body frame and travel give the range), publishes
`atrium.car.height` = (z + 23.600) ÷ 44.000.

| Stop | Car floor z | Fraction | What |
|---|---|---|---|
| `cube` | −23.600 | 0.000000 | eyes at the Cube's centre (−22.000); never lands |
| `clamp` | −10.825 | 0.290341 | the Cube mast's head in the Cube's ceiling iris (`CubeWay.cpp:17-21`) |
| `atrium` | 0.000 | 0.536364 | the landing |
| `neck` | 8.400 | 0.727273 | **home**: in the Sphere's neck, both neck irises shut |
| `mast` | 11.310 | 0.793409 | the Sphere's mast foot ring just above its bearing dogs (`sphere.mast.dogs`, the Sphere's) |
| `gate` | 17.400 | 0.931818 | the car waits under the Sphere's platform iris (head 30 mm under its stack) |
| `sphere` | 20.400 | 1.000000 | eyes at the Sphere's centre; floor top 20.420 level with the platform |

- **Speeds:** on the rails 1.5 m/s, 0.75 m/s² (`MuseePlan.h:204-205`); on the Cube mast (below `clamp`) 1.0 m/s, 0.5 m/s²
  (`ElanElevator.cpp:44`); jerk ≤ 1.0 m/s³ (as built unlimited). Every leg an S-curve from rest to rest.
- **Drive:** rack and pinion on the two rails above `clamp` (pinions in the guide blocks of both rings); below it the car
  hangs on the Cube mast its crown feeds out; above the Sphere's mouth it rides the Sphere's mast (the Sphere's, rising
  from its mouth to the car's under-floor seat). No counterweight: Motion rates the drive for the car's mass (Form volumes
  × Matter densities) plus its rated load (EN 81-20, 14.25 m² floor).
- **Uses and sequences** (Mechanisms.json `open`, `close`, `rides`; channel steps `{channel, to, time}`, moves name stops):

| Use | Verb (when) | Sequence (as-built timings, `ElanElevator.cpp:480-695`) |
|---|---|---|
| `atrium.call` | "Call the car" (the car isn't here or coming) | collar iris open 4.0 s → move `atrium` (≈ 7.6 s) → car and landing leaves open 1.5 s: ≈ 13 s |
| (automatic) | home: open at the Atrium, nobody within r 3.500 (or off the floor) for 12 s | leaves shut 1.5 s → move `neck` → collar iris shut 4.0 s |
| `atrium.car.panel` at `atrium` | "Down to the Cube" (`atrium.down` mated) | leaves shut → `cube.up.closure` → 1 (4.0 s, while descending; the car waits at −7.750 if not yet open) → move `clamp` → clamp (→ 0.3206, 2.49 s) → move `cube` on the mast (≈ 14.8 s) → `atrium.car.light` → 0.03 (2.5 s): ≈ 25 s |
| `atrium.car.panel` at `atrium` | "Up to the Sphere" (requires `atrium.up` mated) | leaves shut → move `neck` → collar iris shut 4.0 s → **the Sphere leg**: `sphere.neck.closure` → 1 (4.0 s) → move `mast` → `sphere.mast.dogs` → 1 (2.0 s) → move `gate` → `sphere.platform.closure` → 1 (4.0 s) → move `sphere` → `atrium.car.light` → 0.03 (2.5 s, so the car doesn't light the sky) → car leaves open |
| `atrium.car.panel` at `cube` | "Up to the Atrium" / "Up to the Sphere" (the light is back; no work performs) | light → 1 (2.5 s) → move `clamp` → release (→ 1, 2.5 s) → move `atrium` (leaves open) or on to `neck` and the airlock as above; `cube.up.closure` → 0 behind the car once above −7.450 |
| `atrium.car.panel` at `sphere` | "Down to the Atrium" (leaves open) | **the Sphere's descent**: car leaves shut → move `gate` → `sphere.platform.closure` → 0 (4.0 s) → move `mast` → `sphere.mast.dogs` → 0 (2.0 s) → move `neck` → `sphere.neck.closure` → 0 → light → 1 (2.5 s); then collar iris open → move `atrium` → leaves open |
| `sphere.call` (the Sphere's) at `neck` | "Call the car" on the platform (requires `atrium.up` mated) | the Sphere leg as above, from `sphere.neck.closure` → 1 |
| `sphere.call` at `sphere` | "Send the car down" (leaves open; as built `ElanElevator.cpp:368-371`) | the Sphere's descent as above, ending at home `neck` |

- **The Sphere's mast foot rides the car:** the ring `sphere.mast.foot` (the Sphere's body; gilt CuSn10 annulus r 0.205–0.400,
  58 deep) is latched round the car's under-floor seat at car frame −0.158 … −0.100, and follows `atrium.car.height`
  [0, 0.793409] → [−34.910, 0]. At `mast` its underside is 2 mm over the Sphere's dogs, and the car's first 2 mm of rise sets
  it on them and releases the latch. The seat carries the latch (Form: a latch ring in `atrium.car` under the floor, inside
  the seat's −0.160). The car's lowest point is unchanged, so A-4 holds.
- **Unmated ports:** `atrium.down` unmated: no stop below `atrium`, no "Down to the Cube", "Up to the Sphere" offered at
  once. `atrium.up` unmated: home is `atrium` with the leaves shut, the collar iris stays shut, no stop above `atrium`.
  Both unmated: the car stands at the landing; "Call the car" opens its leaves; no ride verbs.

**M-2 `atrium.cube-mast`** (body `atrium.car.mast.head`, on the car): prismatic +z, 0 → 12.775 relative to the car,
**follows** `atrium.car.height` in [0, 0.290341] → out [12.775, 0] (the head stays in the Cube's iris while the car sinks;
the column between crown and head is Motion's variable-length piece). Not reached with `atrium.down` unmated.

**M-3…M-6 the leaves:** `atrium.car.leaf.north`, `atrium.car.leaf.south` (ride the car) follow `atrium.car.doors`;
`atrium.landing.leaf.north`, `atrium.landing.leaf.south` follow `atrium.landing.doors`; each revolute about the stage's
z axis, 24° away from the door's centre line into its pocket (north leaves towards increasing bearing, south towards
decreasing), in [0, 1] → out [0, 24], eased s = 3o² − 2o³ over 1.5 s. The landing's open only with the car at the Atrium.

**M-7…M-13 the collar iris:** `atrium.up.iris.blade.1` … `.7`, revolute about each blade's pin, drawn shut, opening 52°
towards decreasing plan angle; follow `atrium.up.closure` in [0, 1] → out [0, 52], eased over 4.0 s. As built it is open
while the car is below the neck stop and shut while the car is at or above it (`ElanElevator.cpp:537-547`).

**Interlocks** (Motion, by key; Site binds those across seams):

| # | Rule |
|---|---|
| A-1 | The car moves only with its leaves shut, and at the Atrium the landing's too. |
| A-2 | The landing's leaves open only with the car at 0.000 ± 5 mm; the car's only at `atrium` or `sphere`, never at `cube`, `clamp` or `neck`. |
| A-3 | Occupancy guard (`body-in-travel`): a leaf holds or reverses while a body is in its next travel slice; the car waits while a body stands in the doorway; no call opens the landing onto a moving car. |
| A-4 | The car's head (2.940 above its floor) passes the collar iris's stack (bottom 7.669) only with `atrium.up.closure` ≥ 0.999: the car floor rises above **4.679**, or descends below **7.915** (its seat's bottom −0.160 over the stack's top 7.705 + 0.050), only with it open. |
| A-5 | Airlock: the collar iris and the Sphere's neck iris are never both open; at `neck` the one behind shuts before the one ahead opens. |
| A-6 | The Cube: the car floor passes below −7.750 only with `cube.up.closure` ≥ 0.999; leaves `clamp` downward only clamped (0.3206 ± 0.002), upward only released (≥ 0.999). |
| A-7 | The Sphere: the car passes its platform gate (floor 17.400) only with `sphere.platform.closure` ≥ 0.999 (the Sphere's guard). |
| A-8 | The car leaves the Cube stop only with `atrium.car.light` back at 1; a work performing holds it. |

**Clearances:** guides (r 2.290) ↔ lining bore (2.300) 10 mm; car rings (2.235) ↔ floor lip (2.290) 55 mm; ↔ landing
leaves (2.375) 140 mm; ↔ tube frame (2.410) 175 mm; ↔ collar hub (2.465) and the open blades (2.470) 230 mm; rail foot
(2.425) ↔ landing panes (2.450) 25 mm; car leaves (2.150) ↔ car panes (2.185) 35 mm; landing leaves (2.390) ↔ landing
panes (2.450) 60 mm; car head at `neck` (11.340) ↔ the Sphere's neck iris stack (11.370) 30 mm; car's lowest point at
`neck` (8.240) ↔ the collar iris (7.695) 545 mm; blades ↔ blades 2 mm (pitch); rollers ↔ blades: rolling contact.

## 7. Nature
None: no plants, water or ground of the Atrium's own. Rain and snow stop at its shell; the terrace and planting round it
are the Grounds'.

## 8. Art
**The work** `van-gogh-starry-night` (`assets/collection.json`): Vincent van Gogh, *The Starry Night* (*De
sterrennacht*), 1889, oil on canvas, **0.921 × 0.737 m** (73.7 × 92.1 cm h × w; the catalogue has no size: the
FutureAtrium board and `assets/paintings/CREDITS.md:75`), Museum of Modern Art, New York. Catalogue note: "The sign for the
elevator: this car goes up to the stars." Image `assets/paintings/van-gogh-starry-night.jpg` (Google Art Project, PD-Art).

- **Hang** (Hang.json): Wall anchor `atrium.stele`, `along` 0.290, `height` 1.012 (centre (−3.885, −1.900, **1.380**) as
  built, `Elan.swift:32`), facing west to the door; the canvas face 45 mm proud of the glass.
- **Support:** canvas at true size on a keyed pine stretcher; the back open, so through the stele one sees the unbleached
  linen and the stretcher's keys (the board).
- **Frame** `atrium.frame.gilt`: slim, the Salon's gilt profile at moulding width **W 0.060** (`MuseeFrame.cpp:310-371`),
  outer 0.921 + 2 × 0.957W by 0.737 + 2 × 0.957W = **1.036 × 0.852**; burnished water gilding on an oak core; its back
  bears on the glass through clear pads; the ornament is polish.
- **Care** `oil-and-tempera` (150 rec., ≤ 200 lx, UV ≤ 75 µW/lm); the daylight finding of §4.
- **Caption:** title, artist, date, medium, holder; the board's words: "A small square of night in a daylit room … It is
  the sign for the elevator: this car goes up to the stars."
- **The bays** A–K (`atrium.bay.a` … `atrium.bay.k`, Hang.json `walls`): empty tonight, for new art. No casts, scans or
  cases. The bay letters and the stele are pieces (§3), not works.

## 9. Sound
- **Acoustics** from Matter, never tuned: ≈ 7,700 m³ of polished marble (≈ 600 m²), polished travertine (≈ 510 m²), glass
  drum (≈ 1,335 m²) and glass canopy (≈ 1,120 m²): very live (≈ 7 s at mid frequencies, shorter in the bass where the big
  panes absorb). The concave drum focuses a sound made at the centre back to it 82 ms later; the landing and the car's
  glass break it partly; the convex canopy diffuses.
- **Emitters:** the car's drive (pinions on racks; the spiral-band column), the leaves (1.5 s), the collar iris's blades
  (4 s) from the Motion actuations; footfalls on mirror marble, bronze (ring, sill) and etched glass (the car); rain on the
  drum's outer glass and the washes. The open west door joins the Hall of Light; the shut irises occlude the Cube and the
  Sphere.

## 10. Visitor
- **Arrive** from the Hall of Light through the 4 m door; ahead the glass car with its door facing you, *The Starry
  Night* on its stele to its left (north), facing you. The mirror floor holds the canopy; the bays run clockwise from A;
  bay F lies on the east axis, seen through the car.
- **Wait** on the bronze ring (the waiting mark west of it); **use** "Call the car" (`atrium.call`); **ride** from inside
  the car (`atrium.car.panel`): verbs in §6. The car's floor is a moving base. The first ride goes down to the Cube before
  it goes up. Adapting from the daylit hall to the car and the dark Cube is the Eye's (nothing local).
- **Look** at the painting's caption. Nothing to sit on (as built). Step-to never lands on the stele's foot, in the
  landing when the car is away, or in the car when it isn't at the landing.

## 11. The old-code map
Paths relative to `windows\Source\MuseeVision\` unless noted. Re-express, never copy.

| Part | Where |
|---|---|
| Élan numbers (radius, wall, base, door, car, wait ring, Sphere, speeds, `Underside()`) | Plan/MuseePlan.h:160-215 |
| Élan exterior (plinth, drum skins, order, entablature, portal) | Plan/MuseePlan.h:217-256 |
| Atrium level 0 (floor, lip, ring, mark, stele, letters) | Plan/MuseePlan.h:322-344 |
| Base constants (joints, cove, band, retired pilasters, door, inlays, stele) | Atrium/AtriumBaseStructure.cpp:23-78 |
| Floor, lip, door zone; coping band and wall top; wall, cove, door | Atrium/AtriumBaseStructure.cpp:149-270, 279-390, 392-472 |
| Bronze inlays (glyphs, placing, waiting mark); the stele | Atrium/AtriumBaseStructure.cpp:534-767, 769-800 |
| Cove lights (as built 3 per bay, 3800 K: not ported) | Atrium/AtriumBaseStructure.cpp:806-832, 1118-1154; AtriumBaseStructure.h:150-176 |
| Grid and laylight constants; grid; laylights and rib uplights (retired / reserved); emissive nits (retired) | Elan/ElanStructure.cpp:20-44, 87-165, 278-338, 340-372 |
| Glass lathes; ribs (spine, section, shoe, line); ring | Elan/ElanStructure.cpp:374-400, 402-497, 498-503; ElanStructure.h:46 |
| Kit dimensions (car, landing, tube, iris, neck, crown, head, gates) | Elan/ElanKit.h:37-104; ElanKit.cpp:21-41 (CollarTop, spans) |
| Car; landing, tube, collar | Elan/ElanCar.cpp:20-190, 284-400 |
| Iris blade, pins, swing; the collar iris's placement | Elan/ElanSphereTop.cpp:46-184 (pins 128, swing 177), 249 |
| Timings and speeds; calls; legs; doors, irises, airlock, drive, home | Elan/ElanElevator.cpp:36-46, 230-370, 441-453, 480-695 (home 637) |
| Cube interface (lining, rails, iris top, clamp, mast) | Cube/CubePlan.h:40-73; Cube/CubeWay.cpp:17-21; Cube/CubeWay.h:12-40 |
| Exterior constants; plinth and entablature sections; socle, cornice, architrave; portal; order; skins | ElanExterior/ElanExteriorStructure.cpp:25-56, 346-377, 380-411, 418-556, 564-586 and 1096-1127, 1196-1216 |
| Canvas placement; frame; free-standing hang | repo Shared/Wings/Elan.swift:32, 232-240; Frames/MuseeFrame.cpp:310-371; windows/Scripts/frames.py:41-44; gallery_lights.py:49 |
| Materials as built (evidence only) | windows/Scripts/materials.py:1262-1269, 1397-1402; exterior_materials.py |
| Boards | repo plan/canvas/FutureAtrium.dc.html, AsBuiltElan.dc.html, ElanSphere.dc.html; plan/README.md:113-131 |

**Corrections to the draft** (`r-specs/…/Stages/atrium.md`):
1. **Vertical ports renamed and reclassed** to D-012's pairing: `atrium.down ↔ cube.up`, `atrium.up ↔ sphere.down`
   (draft `cube.shaft`, `sphere.shaft`), class **`iris.4600`** named by the well's clear diameter (draft
   `shaft.lift.4900.iris`); the closure is the **lower stage's** iris (the Atrium's collar iris for `atrium.up`, the
   Cube's ceiling iris for `atrium.down`; the draft put `atrium.up`'s at the Sphere's mouth). Frames and museum
   coordinates unchanged: (54, 0, −0.500) and (54, 0, 7.715).
2. **Iris blades** 2 mm at a 4 mm pitch (draft and as built 5 mm at 3 mm: they interpenetrated); the 26 mm stack kept, so
   the clamp level −10.825 stands.
3. **The Sphere's mast** is the Sphere's, rising from its mouth to the car's under-floor seat (as built: "the mast rises
   out of the neck's mouth", `ElanElevator.cpp:681-684`); the draft had the car feed it.
4. **Materials are palette keys** in the Future's workings (draft role strings): floor Crema Marfil **mirror-polished**;
   base travertine **filled and polished** (draft honed); waiting ring, rib shoes, stele foot, sill **mirror-polished
   bronze** (draft gilt, brass); lining dark-patinated; rails **hard-chromed steel** (draft brushed bronze); slab
   `accent.concrete`; iris undersides gilt on pearl; frame water-gilt oak (draft "gesso").
5. **Light:** catalogue types or gaps only, 3000 K throughout (draft a 4000 K car slot); the canopy a gap
   `gap.edge-lit-glass.3000k` with operation `always` (draft a new "laylight" class); the painting a framing projector
   (draft a spot with two mounts); cove in 33 runs.
6. **Anchors** in the contract's kinds: the bays are Wall anchors with curved paths (draft "Space"), the stele's Wall
   origin at its north end on the floor (draft at the canvas centre); the letters are pieces (draft "Sign" anchors); Use
   anchors `atrium.call`, `atrium.car.panel`.
7. **Canopy grid rings** re-derived at the as-built polar angles on the room face R 14.024 (draft's ring radii differ by up
   to 26 mm); ring 20 is the compression ring's face.
8. **Cove** stops 0.350 short of the jambs (draft and as built 0.250) to keep the joint's surround clear.
9. **Verified unchanged:** origin and datum; every section (base, band, dress, entablature, portal, collar, ring, ribs,
   fins); the floor rings and joints; lip, ring, mark and stele; landing, tube and car; stops and speeds; the timings.

## 12. The first-pass cut
**Tonight:** Site (frame, prisms, the west port and its rough opening; the vertical ports as their stages exist; blinds
otherwise); masonry (base with cove, band, wall top; the exterior dress; the entablature notched at the fins; the portal's
block, opening and cornice); the floor's eight rings with lip and notches, the waiting ring and mark; both drum leaves,
the head closer, the canopy's panes; ribs with shoes, ring and hangers, the canopy grid, the exterior order; the lift
(lining, rails, landing, tube, collar and its iris, the car with crown, head and seat); the stele and *The Starry Night*
(frame without ornament) with its caption; the 70 luminaires (the canopy's only if its gap record lands; otherwise the
canopy is unlit glass and the design order fails honestly); Motion M-1…M-13 with A-1…A-8 as the neighbours exist;
acoustics and footfalls; the call and ride verbs.

**Waits for polish:** the bay letters; the ribs' gilt lines, the collar's spokes and gilt; the portal's socle,
architrave and keystone; the frame's ornament; rack, pinion and spiral-band drawings (the crown's drum grows to house its
column: Motion.Housed); the reserved uplights and façade light; mechanism voices; drum panes split by stock if needed.

## 13. Palette keys to confirm and catalogue gaps
**Palette keys** (all real entries or ruled; roles guessed or workings outside `workingsByEra` "future"):
- `steel.hard-chrome` (ruled) for the guide rails and racks: machined steel rail, ground and hard-chromed (the lift's
  working steel; no bronze rail carries a rack).
- `accent.concrete` for the Atrium's slab (the palette names it for the Cube's box; the same hidden role here).
- `bronze.cusn10` **patinated.dark** for the lining (seen only from the Cube's shaft; the Cube's working); mirror-polished
  for the sill and the west blind doors (by analogy with the ring and the shoes).
- `white.pearl-steel` with **gilt.gold-leaf.oil** on the iris blades' undersides (oil gilt on a lacquer, Story §2.3);
  **pearl.fluoropolymer** for the order outdoors (Story: "outdoors the same build in fluoropolymer").
- `glass.low-iron` **laminated.uv-filtering.acid-etched.fritted** for the inner leaf: the white ceramic frit is not named
  in the palette's workings (it sets the misty glass's transmittance, the care cap's lever).
- Compound workings: `filled.polished`, `mirror-polished`, `mirror-polished.lacquered`, `pearl.satin`,
  `pearl.fluoropolymer`, `cast-in-situ.sealed`, `ground.hard-chromed`, `patinated.dark`, `toughened.laminated`,
  `heat-strengthened.laminated.uv-filtering`, `laminated.acid-etched`.

**Catalogue gaps** (Light): `gap.edge-lit-glass.3000k`: an edge-lit light-guide glass luminaire, LED lines (3000 K,
CRI ≥ 90, DALI) coupled into the edges of laminated low-iron panes whose room faces carry an etched extraction pattern,
the lines housed in the grid members' top channels; 24 zones. (The Classical Hall's `gap.luminous-ceiling.3000k` is a
backlit plenum module: a different product.)

**Site:** `PortClasses.json` needs `doorway.4000x4500.square` (as the Hall of Light's) and **`iris.4600`** (§2), with the
closure-at-declared-depth rule.

## Build notes
- **Frame and translation:** origin on the Élan axis on the marble's face; **museum = local + (54.000, 0, 0)**, yaw 0;
  old plan x − 54. Bearings clockwise from north; plan angle = bearing − 90°.
- **Ports:** the west face is the cylinder R 14.800 (convex); build only the rough opening; keep the inner face clear for
  \|y\| < 2.350, z < 4.850. The vertical ports are planes at −0.500 (slab soffit) and 7.715 (collar top); nothing of the
  Atrium's fixed fabric enters the well prism r ≤ 2.300 beyond the planes; the rails stand in the notches.
- **Moving bodies:** `atrium.car` carries the glass, rings, floor, roof, handrail, guides, crown, seat, the light ring
  (12 luminaires with `body`) and, as children, its two leaves and the mast head. The landing's leaves and the seven
  blades are fixed-frame bodies. Everything moving is drawn in its closed pose (car at the Atrium stop, leaves shut,
  blades shut).
- **Wells served:** both. The car's travel reaches into the Cube (to −23.760) and the Sphere (to 23.340) only when those
  stages are mated; Site binds `cube.up.closure`, `sphere.neck.closure`, `sphere.platform.closure` and `sphere.call`.
- **Draw by rule, not by hand:** the ribs' spine (two arcs); the canopy grid (rings in the conformal coordinate, members
  node to node); the canopy panes (one per grid triangle, on the members); the floor rings; the lathed sections; the
  spiral-band column as Motion's variable-length piece.
- **Joints, not embeds:** ribs 5 mm off stone and glass and bearing on the canopy; coping band and grid members stopped
  short of the ribs; the entablature notched round the fins; the order on level seats and rooted on the outer leaf's
  face; inlays in recesses; the landing shoe in a rebate; the collar hung from the ring.
- **Unmated states** are part of the design (§2, §6): no well, no ride that way, the iris shut.

**Departures from as-built** (for the owner's review):
- **iris.4600 corrected:** each blade gains a pin lug (r 0.060 boss on a 0.120 web) so it encloses its pin; as built the pin stood 25 mm outside the blade. The clamp value is 0.3206 on the linear channel (as built 0.378 through a smoothstep: the same blade pose, r 0.197). Shut, clamp, open and reach checked by polygon union.
- **The Sphere leg** (ruled with the Sphere's Q-1): the car stops at `mast` while the Sphere's bearing dogs slide under its mast's foot ring, and at `gate` before the platform iris opens (never an open drop beside a visitor). The platform iris shuts behind a descending car, "Send the car down" is restored from the as-built code, and the car's light drops to a glimmer at `sphere`, so it doesn't light the sky.
- **No memory of the visit** (first pass): "Up to the Sphere" from the Atrium no longer waits for a visit to the Cube; rides are gated only by mated ports and channels (`requires`). The Cube rides require `cube.performance.active` ≤ 0, which the Cube publishes.
- **Stage model:** the ribs stand 5 mm clear of stone and glass (as built tucked 30 mm in); the canopy's glass lies on its
  grid at R 14.024 (as built at R 14.000 with the grid 5 cm under it, unsupported); the wash and the order's root moved out
  to 14.080 and 14.074 (as built through the outer pane); the entablature notched round the fins and the coping band
  stopped at the ribs (as built they crossed); the order on level seats (as built sunk in the wash); hanger plates carry
  the collar (as built it floated); inlays in recesses; the cove stops 0.350 short of the jambs (as built 0.250).
- **One real double-skin drum** (misty inner leaf, clear outer leaf; as built three skins for the renderer).
- **The lift made real:** guide rails with racks from the slab to under the collar (as built the car ran on nothing above
  the Cube's shaft); jerk-limited travel; the Cube mast fed from the crown (as built); iris blades 2 mm at a 4 mm pitch
  (as built interpenetrating); the car's glass no longer "fades" at the Cube: its light falls to a glimmer (the as-built
  glimmer 0.03) and clear glass in a dark room shows little.
- **Ruled: the port closures.** `atrium.up` is closed by the Atrium's own collar iris (as built the neck's lower iris) and
  `atrium.down` by the Cube's ceiling iris: each vertical port's iris belongs to the stage below its plane.
- **Ruled: the Sphere's mast** is the Sphere's (as built it rose from the neck's mouth; the draft had the car feed it).
- **Ruled: unmated vertical ports** keep the car at the Atrium (no blind wells tonight unless on the stand).
- **Light:** the canopy's glow is a real edge-lit luminous ceiling (as built emissive glass and 24 invisible rect
  lights); the drum's glow is daylight through real glass (as built emissive); 3000 K cove (as built 3800 K) and car
  light (as built 5000 K); the painting framed by a projector on the door bay's rib; the canopy's "brighter while the
  car runs" is retired.
- **Materials follow the Story:** mirror-polished Crema Marfil; filled and polished travertine base (as built "honed"
  shader); mirror-polished bronze for the ring, shoes, sill and stele foot (as built gilt and brass); hard-chromed rails.
