# The Rotunda · stage sheet

This is the design sheet the Room Builder builds the Rotunda from. It covers the drum, the order, the coffered dome,
the glazed lattice eye with its bronze node, the four niches and their casts, the sun clock and the spiral stair.

- **Sources.** The sheet is ported from the as-built museum, not redesigned (D-001, D-017):
  - the old code (`windows/`, read-only), which gives the geometry;
  - the `Rotunda` board and `plan/README.md`, which add the gilt meridian;
  - the approved proposals, which give the three neighbours.
- **Units and angles.** All numbers are metres in the stage's own frame. Angles φ are plan angles measured from east
  (+x) towards south (+y), the old code's convention. A compass bearing is φ + 90°.
- **As built and delta.** Where the sheet departs from the as-built code, it says so and gives the reason.
  Everything else is as built.
- **Schedules.** Per D-017, the designer's choices live beside the room as schedules in `Data/Stages/Rotunda/`:
  `Finishes.json` (§3.12), `Lighting.json` (§4) and `Hang.json` (§8). The geometry lives in the module (`Private/RotundaPlan.h`
  and the part files named in Architecture.md).

---

## 1. Identity and placement

| | |
|---|---|
| Stage key | `rotunda` |
| Module | `MuseeRotunda` (Editor), `Source/Stages/Rooms/Rotunda`; class `URotundaRoom : USiteRoom` |
| Display name | FText "The Rotunda" |
| Era | Classical. Stone is honed or polished, bronze is cast, patinated or gilt, and plaster is matt. The eye is the one touch of the future. |

**Frame and origin.** The local frame runs x east, y south, z up, on true north. The origin is the drum's centre on
the finished floor. z 0 is the top of the fixed ring and of the lowered dial. The gilt boss of the sun stands 0.085
above it.

**In the museum plan.** The Rotunda is Site's anchor stage. Its origin *is* the museum plan origin: plan (0, 0, 0)
at level 0 (the ground floor). It is not translated.

**Arrival point.** The gilt sun at (0, 0, 0), facing west (−x, yaw 180°). This is D-012, and the old code's
`MuseePlan::SpawnFeet` and `SpawnYawDegrees`.

**Footprint.** Kind Building.
- **Plan outline.** The disc r 11.20 (the drum's outer face). Where Site needs a polygon, it takes Form's shared
  stations on r 11.20, so each neighbour's face meets the drum vertex for vertex.
- **Vertical extent.** z −0.55 (the floor structure's underside, on the Classical Hall's roof at −0.55) to z +21.00
  (above the eye's curb at 20.45).
- **Plus the served well** (§2): the cylinder r 7.60, z −6.40 … −0.55.
- **What stays outside the footprint tonight.** The exterior dress (§3.10) stands proud of r 11.20: a podium 0.54
  proud, then a gravel strip and stone edge to r 12.76 on the free arcs (the Grounds sheet expects this). When the
  dress is built, the footprint grows to r 12.76 on the free arcs. Tonight it is r 11.20.

**Enclosures.**

| Key | Kind | Bounded by | Volume |
|---|---|---|---|
| `rotunda.hall` | Enclosed | The drum's inner face (r 10.00, the niches to r 11.00), the dome's coffered inner surface, the eye's glass (the only opening to the sky, Glazed: D-004), and the floor at z 0 (ring and dial). It is open at the four doorway ports and, while the sun clock is raised, through the drum's door into `rotunda.stair`. | ≈ 5,240 m³: drum π·10²·10 plus a hemisphere of R 10, before the niches and coffers |
| `rotunda.stair` | Enclosed | The stair wall (r 6.48) from the landing (z −6.00) up to the dial's soffit. Raised, it also takes in the drum's interior (r 6.545) to z 3.37. It is open at the stair port and, while raised, through the drum's west door. Lowered, it is sealed by the dial. | ≈ 774 m³ lowered, ≈ 1,250 m³ raised |

**Openings** (Form's one list):

| Key | Kind | Where |
|---|---|---|
| `rotunda.eye` | Glazed | The disc r 2.50 at z 20.38–20.39 in the curb (§3.5). Weather: `museum`. |
| `rotunda.west`, `.north`, `.south`, `.east` | Port | The four rough openings in the drum (§2, §3.2). |
| `rotunda.stair` | Port | The north portal at the foot of the stair (§2, §3.7). |
| `rotunda.sun-clock.door` | Open | The drum's west door between `rotunda.hall` and `rotunda.stair`. It exists in the raised configurations only. |
| `rotunda.sun-clock.pocket` | Housing | The annulus r 6.54–7.00, z −3.58 … 0, which the drum's wall slides into (§3.7). |
| `rotunda.sun-clock.rams` | Housing | Eight bores for the lift's rams (§3.7, §6). |
| `rotunda.stair.balustrade-housings` | Housing | The slots of the retracting balustrade (§3.9). |
| `rotunda.sun-clock.luminaires` | Housing | The recesses of the 13 downlights in the drum's soffit (§4). |

---

## 2. Ports

Every port's frame sits on the outer face at the opening centreline and at threshold level, facing out. The
Rotunda builds only the **rough opening** at each port (the clear opening plus the class allowance) and nothing
past its port face. The doorcase is Site's joint:
- the travertine threshold through both wall depths;
- the brushed-bronze strip on the mating plane;
- a lining in front of both rough faces;
- surrounds on both mating faces;
- passage masonry, including the wedges where a flat neighbour meets the drum.

| Key | Kind | Class | Frame: origin, outward normal | Face | Wall depth | Clear opening | Mates | Passage |
|---|---|---|---|---|---|---|---|---|
| `rotunda.west` | Doorway | `doorway.3000x4500.arched` | (−11.200, 0, 0), (−1, 0, 0) | cylindrical, R 11.20 about the z axis | 1.20 (r 10.00 → 11.20) | 3.00 wide, springing 4.50, semicircular head, crown 6.00 | `salon.east` | 2.20 m, to the Salon's Bay 1 end wall outer face at x −13.40 |
| `rotunda.north` | Doorway | `doorway.4000x4500.arched` | (0, −11.200, 0), (0, −1, 0) | cylindrical, R 11.20 | 1.20 | 4.00 wide, springing 4.50, crown 6.50 | `chenghuai.south` | 0 (the open porch abuts the drum) |
| `rotunda.south` | Doorway | `doorway.4000x4500.arched` | (0, 11.200, 0), (0, 1, 0) | cylindrical, R 11.20 | 1.20 | 4.00 × springing 4.50, crown 6.50 | `albion.north` | 0 (the porch abuts the drum) |
| `rotunda.east` | Doorway | `doorway.3000x4500.arched` | (11.200, 0, 0), (1, 0, 0) | cylindrical, R 11.20 | 1.20 | 3.00 × springing 4.50, crown 6.00 | `hall-of-light.west` | 0 (the glass hall abuts the drum) |
| `rotunda.stair` | Stairwell | `stairwell.spiral.3600` | (0, −7.600, −6.000), (0, −1, 0) | cylindrical, R 7.60 about the z axis | 0.55 (the arched reveal, r 7.05 → 7.60). The jambs run on through the masonry to the stair wall at r 6.48 (§3.7). | 3.60 wide (jambs are the planes x = ±1.80), springing 2.60 above the threshold (z −3.40), crown 4.40 (z −1.60) | `classical-hall.stair` | 0 |

**The well of `rotunda.stair`.**
- **Prism.** The vertical cylinder r 7.60 about the z axis, from z −6.40 to −0.55: the part of the stair's shaft
  that lies inside the Classical Hall's volume. The shaft's outer face (r 7.60) coincides with the well.
- **Serving side: `rotunda`.** It owns everything inside the well: the shaft masonry, the stair wall, the pocket,
  the ram housings, the landing and its rosette, the helical stair and its balustrade, and the drum's full travel.
- **Receiving side: `classical-hall`.** It reserves the prism as a void and builds nothing inside it. Its walls meet
  r 7.60 (as-built `ClassicalHall::ShaftOuterRadius`).
- **On the stand.** The well is unmated there. Site gives it a blind well to z −6.40 and blinds the portal, so the
  stair and the lift run alone.

**Port classes.** These are proposals to Site; `Data/Site/PortClasses.json` governs.
- **`doorway.3000x4500.arched` and `doorway.4000x4500.arched`.** The second number is the springing height above the
  threshold, and the head is a semicircle.
  - Closure: open portal (the as-built doors have no leaves).
  - Rough-opening allowance: 0.050 on each jamb and at the head, for the lining. That gives rough widths of 3.10
    and 4.10, with head radii 1.55 and 2.05 about the same springing.
  - Rough sill at z −0.05: the joint's 50 mm threshold stone sits there, its top flush at 0.00.
  - Collar envelope: the south mate needs at least 7.30 × 7.50 m (Albion sheet, wedge analysis).
  - The as-built door has **impost blocks** at the springing. Their section runs through the wall depth: 0.24 high
    (4.26–4.50), 0.05 into the clear opening, with a 30 mm chamfer at the foot, as the point list (lateral into the
    opening, height from the springing): (−0.01, −0.24) (0.02, −0.24) (0.05, −0.21) (0.05, 0) (−0.01, 0). That
    reveal is now the joint's lining, so the impost belongs to the doorway class detail. The Rotunda asks Site to
    carry it in the arched classes. The archivolt on the drum's inner face (§3.3) springs from it.
- **`stairwell.spiral.3600`.** A 3.60 m portal with a semicircular head springing 2.60 above the threshold, an open
  portal, and a well.

**D-012 topology** (Site's `SitePlan.json`: the anchor is `rotunda` at the origin, and the arrival point is the gilt
sun, facing west):

```
rotunda.west  <-> salon.east            passage 2.20
rotunda.north <-> chenghuai.south       passage 0
rotunda.south <-> albion.north          passage 0
rotunda.east  <-> hall-of-light.west    passage 0
rotunda.stair <-> classical-hall.stair  passage 0, well served by rotunda
```

The Architecture's older line `rotunda.entrance <-> grounds.gate` is superseded by D-012. The Rotunda has no entrance
port.

**Neighbour mismatches, for Site and the neighbours' sheets** (as-built numbers):
- **West.** The Salon's door in Bay 1's end wall is 3.00 wide but springs at **3.30** (`SalonStructure.cpp:41`), not
  4.50. As built, the Rotunda's passage (springing 4.50) ran 2.20 m and stopped against that lower arch. Either the
  2.20 m passage steps down to the Salon's arch (Site's joint detail), or `salon.east` takes this class.
- **North.** Chenghuai's open porch is only 3.80 wide inside (`ChenghuaiPlan.h:38`, PorchHalf 1.9), 0.10 narrower
  on each side than the 4.00 door. The joint's surround must sit within it.
- **At the thresholds.** The as-built bronze strips lay on the sun clock's edge (r 10.30, `MuseeFurniture.cpp:305-311`).
  They now go on each mating plane, per the joint rule.

---

## 3. Pieces by trade

**Material keys.** Each piece's key is `substance.working.history`; the full finish schedule is §3.12. Workings map
to Matter's records:

| Short working | Matter record |
|---|---|
| honed | `stone.honed` |
| polished | `stone.polished` |
| sawn | `stone.sawn.vein-cut` |
| run | `run` |
| trowelled | `trowelled` |
| cast | `cast.bronze` or `cast.plaster` |
| gilt | `gilt` |
| brushed | `brushed` |
| painted | `painted.steel` |
| laminated | `laminated` |

History words are presets over Matter's history records:

| Preset | For | Matter records |
|---|---|---|
| `as-laid` | hidden pieces | none |
| `aged` | indoors | stone: `stone.pore-soiling`; white: `white.soiling` + `white.micro-cracking`; bronze: `bronze.tarnish`; steel: `steel.coating-ageing`; glass: `glass.soiling` |
| `worn` | underfoot | `aged` + `stone.foot-traffic-wear`, or `bronze.wear` + `gilt.wear` |
| `handled` | handrails | `bronze.tarnish` + `bronze.wear` |
| `weathered` | outdoors | `stone.dissolution-outdoors` + `stone.pore-soiling` |

**Build-ups.** A piece is one solid of one substance, within stock. The cream marble is a thin-slab stone (10–30 mm
stock), so every cream-marble or statuary surface is a **laid finish** over a hidden core. The core is travertine
ashlar, as Roman revetment over travertine was built. The visible geometry is exactly as built; only the section
is made real.

**Joints between pieces.** Pieces meet face to face: no piece sinks into another. As built, the pilasters' backs, the
mouldings and the inlays sank 10 mm into their neighbours. Now:
- the moulding is trimmed to the face it meets;
- the stone is cut where bronze is set.

**Stations.** The as-built station counts are evidence of the needed fineness, not rules. Form's chord tolerance sets
the division. Pieces that meet share stations.

| Curve | As-built stations |
|---|---|
| each semicircular arch | 48 |
| the drum's faces, between openings | 320 per turn |
| the dome | 280 angles, 10 per coffer |
| every sun-clock circle | 768 |

### 3.1 Floors

**The fixed ring, r 7.00 → 10.04.**
- **Surface.** Top at z 0.00, finish 30 mm, core to −0.55.
- **Extent.** The outer edge is the back of the drum's shadow gap (r 10.04). In the four doorways, the ring stops on
  the drum's inner face line (the arc r 10.00 across the rough opening), where the joint's threshold begins. *Delta:*
  as built, the ring ran on to r 10.30 under the wall.
- **Three courses of radial slabs, alternating in two tones** by the parity of (slab + course):
  1. r 7.00–7.75: 48 slabs.
  2. r 7.75–8.95: 32 slabs. This course carries the motto.
  3. r 8.95–10.04: 64 slabs, set half a slab round from the course grid (+2.8125°).
- **The two tones** are the same statuary marble, sawn two ways: tone A vein-cut, tone B cross-cut. Form's per-piece
  bed frame carries the choice.
- **The motto** (bronze, §3.1.3) is centred on the south, φ 90°.

**The dial, the top of the sun-clock drum** (a moving body, §3.6). It is r 0 → 6.99, top at z_d 0 (z_d is the
drum's own frame: z = z_d lowered, z = z_d + 3.50 raised). It has seven concentric bands:

| r from | r to | Stone |
|---|---|---|
| 0.00 | 1.40 | A: statuary, vein-cut |
| 1.40 | 1.52 | rosso antico |
| 1.52 | 3.20 | B: statuary, cross-cut |
| 3.20 | 4.90 | A |
| 4.90 | 6.30 | B |
| 6.30 | 6.38 | nero marquina |
| 6.38 | 6.99 | A, to the corona's edge |

The ring and the dial meet with 10 mm clear (r 6.99 against r 7.00): the lift's clearance (§6).

#### 3.1.1 The dial's inlay

Everything here is cast bronze unless stated otherwise. Each inlay is set in a groove cut 2 mm into the stone, and
stands proud with a bevelled top edge.

| Mark | Where | Section |
|---|---|---|
| **Hour ring** | r 6.76–6.80, centred on the circle of the node's shadow (r 6.78) | 3 mm proud, 1.5 mm bevel |
| **Band fillets** | 10 mm wide, centred on r 1.40, 1.52, 6.30 and 6.38 | 2 mm proud, 0.8 mm bevel |
| **Hour lines** | 13 bars at φ_h = 180° + 15°·(h − 6), h = 6 … 18 (VI in the west, XII in the north, VI in the east), radial from r 1.60 to 6.77 | 24 mm wide, 2.5 mm proud, 1 mm bevel |
| **The meridian** | The XII line at φ 270°, from r 1.60 to r 6.80 | 60 mm wide, 2.5 mm proud, 1 mm bevel |
| **Minute ticks** | Every minute from VI to VI except the hours, ending at r 6.77. The shadow moves 2.96 cm a minute on the hour ring. | See below |
| **Numerals** | Roman, 13 of them: VI VII VIII IX X XI XII I II III IV V VI | See below |

**The meridian is gilt bronze** (`bronze.gilt.worn`). *Delta:* the `Rotunda` board and `plan/README.md` ("gilt meridian")
draw it at about 2.3 times the hour lines' weight. The as-built code (`SunClockDial.cpp:13`, "No meridian") made the
XII line an ordinary 24 mm bar.

**Minute ticks:**

| Tick | From r | Width | Proud | Bevel |
|---|---|---|---|---|
| 1 min | 6.665 | 3 mm | 2 mm | 0.6 mm |
| 5 min | 6.62 | 5 mm | 2.2 mm | 0.8 mm |
| 15 min | 6.58 | 7 mm | 2.5 mm | 1 mm |
| 30 min | 6.52 | 10 mm | 2.5 mm | 1 mm |

**Numerals:**
- Baseline r 6.815, cap height 0.16, tops outward. They read from the dial.
- Each is centred on its hour's φ and laid along the arc.
- Tracking −0.03 cap.
- 5 mm proud, 2 mm bevel.

#### 3.1.2 The gilt sun at the centre

**Rays.** 32 of them, at φ = 11.25°·i, each rising from r 0.24:
- 16 long straight rays (i even, so one points to XII) run to r 1.25. Each is 72 mm wide at the root and tapers as
  (1 − t)^0.62. The crest is 42 mm high, falling as (1 − t)^1.2 to 4 mm at the tip.
- 16 short flames (i odd) run to r 0.86. Each is 60 mm wide, tapering as (1 − t)^0.75, with a 30 mm crest. The crest
  weaves sideways by 0.03·sin(3πt)·√t·(1 − t).
- Across each flank: the foot, a hollow at 0.40 of the crest height, then the ridge.

**Boss.** Gilt bronze, a lathe about the z axis. Profile (r, z):

```
(0.285, −0.002) (0.285, 0.035)
arc to (0.265, 0.055): a 0.02 shoulder, then 20 steps up the dome
dome: sphere R 1.1854 with its top at z 0.085, from r 0.265 in to r 0
```

- A bead ring r 0.005 at r 0.19, standing on the dome.
- A beaded foot: a torus of section radius 0.014 at r 0.30, z 0.012.
- This is where every visit begins (the arrival point).

#### 3.1.3 The motto

**Text:** HORAS · NON · NUMERO · NISI · SERENAS, in cast bronze capitals.
- Cap height 0.26. The baseline is r 8.53, so the tops lie towards the dial (cap line r 8.27).
- Centred on φ 90° and read by a visitor on the ring facing the dial, running west to east.
- Tracking 0.16 cap, word gap 0.6 cap. Each "·" is a gap of 1.15 cap holding a gilt triangular stop:
  - point down, spanning ±0.12 cap at 0.60 cap height, with the point at 0.36 cap;
  - its apex 8 mm high.
- Letters are 8 mm proud with a 2.5 mm bevel.

**Glyphs.** As built, the outlines are Centaur capitals (Monotype), baked into `SunClockGlyphs.h`. Their licence is
unresolved (§12).

#### 3.1.4 The landing at the stair's foot

The landing floor is at z −6.00, from r 0 to the stair wall (r 6.48), and through the portal out to r 7.05, where
the joint's threshold begins. It is polished statuary with a rosette under the well:

| Part | Stone |
|---|---|
| Pale eye, r 0–0.12 | A |
| Disc, r 0.12–0.30, edged by a 32-gon through the rays' roots and valleys | nero |
| 16-point star (below) | nero and rosso halves |
| Pale field between the points, to r 1.90 | A |
| Band, r 1.90–2.00 | nero |
| Band, r 2.00–2.06 | rosso |
| Landing, r 2.06 → 6.48 | A |

**The star.** Long points to r 1.80 at φ = 45°·k; short points to r 1.20 at φ = 22.5° + 45°·k. Each point is split
on its axis into a nero half and a rosso half. The long points have nero on the lower-φ side; the short points the
other way round.

### 3.2 Walls: the drum

**Section.** The drum is 1.20 thick, from the inner face at r 10.00 to the outer face at r 11.20. It is a travertine
ashlar core with a 30 mm cream-marble revetment inside.

**Inner face**, r 10.00:
- From z 0.15 to 10.00, hidden above 9.47 behind the entablature.
- The revetment is laid in ashlar courses within cream-marble stock. As-built coursing: courses about 1.2 m, blocks
  up to 1.9 m long, half-bond. Form sets the courses to divide 0.15 → 9.47 evenly (8 courses of 1.165) with the
  datums (niche sill 2.00, springings 4.50 and 4.67) on joints where they fall within ±0.10.

**Shadow gap.** At the foot of the inner face, z 0 → 0.15, recessed 0.04 to r 10.04, with its back face and soffit.
This is as built: the floor never meets the wall on one line.

**Outer face**, r 11.20, from z 0 to 11.00: travertine, honed.
- As built, a bare face. The façade dress over it (§3.10) is for polish.
- Wherever a neighbour abuts (the Hall of Light's vault, Albion's porch, Chenghuai's porch walls, the west passage),
  the outer face is the port face. Site's surrounds and passage masonry close against it.

**Drum top.** A flat annulus at z 11.00 from r 10.553 (the dome shell's foot) to r 11.20.

**Door rough openings**, at φ 0° (east, 3.00), 90° (south, 4.00), 180° (west, 3.00) and 270° (north, 4.00):
- The jambs are planes parallel to the door axis at the rough half-width. The head is a half-cylinder about the
  springing line at z 4.50, running straight through the wall.
- Where a jamb plane cuts the drum:
  - W and E: x = ±11.092 on the rough jambs (±11.099 on the clear jambs);
  - N and S: ±11.011 on the rough jambs (±11.020 on the clear jambs).

**Niches**, at φ 45° (SE), 135° (SW), 225° (NW) and 315° (NE):
- **Size.** 2.00 wide (half-width 1.00). Sill at z 2.00, springing 4.67, crown 5.67.
- **In plan.** The niche's front plane is the chord plane on its axis at r 10.00. Between the curved drum face and
  that plane there is a sliver reveal: 0.050 deep at the jambs, 0 on the axis.
- **Back.** A half-round of radius 1.00 about the axis point at r 10.00, reaching r 11.00. That leaves 0.20 of wall
  behind it.
- **Head.** A half-dome (quarter sphere) of radius 1.00 about (r 10.00, z 4.67).
- **Floor.** The niche floor is the top of the sill ledge (§3.3), running back to the half-round.
- **Finish.** Cream marble, honed, laid.

### 3.3 The order: pilasters, entablature, archivolts

**Order:** `doric-roman.pilaster`, proposed for `Data/Form/Orders.json` from day one, since the anchor defines the
language. Its parts:
- a Tuscan torus base;
- a shaft with 7 stopped flutes;
- a Roman Doric capital (astragal, necking, annulet, echinus, abacus);
- a plain-frieze entablature.

Height to the abacus top is 9.47. It has no carving: the capitals are mouldings only.

**The 16 pilasters.**
- **Plan.** Each is 0.90 wide. The face stands at r 9.57 (0.43 proud of the wall face); the back stands on the wall
  face, r 10.00. The plinth fills the shadow gap to r 10.04 beneath it.
- **Positions.** The plan rule is 11.25° + 22.5°·k. A pilaster that would stand in a doorway steps out just enough for
  its widest part (the abacus, 0.115 each side of the shaft) to clear the jamb. As built, the flank is 11.776° from a
  3.00 door's axis and 14.663° from a 4.00 door's. So φ:

  ```
  11.776  33.750  56.250  75.337  104.663  123.750  146.250  168.224
  191.776 213.750 236.250 255.337  284.663  303.750  326.250  348.224
  ```

  The flank rule, for reference: flank = max(11.25°, asin(h/ρ) + atan2(0.565, b)), where h is the door's
  half-width, b = 10.06 the as-built back radius, and ρ = √(b² + 0.565²).

**Base.** Swept round the front and both returns with mitred corners. Points are (a out from the shaft face, z):

```
(0.10, 0) (0.10, 0.18) (0.03, 0.18)
arc c(0.03, 0.24) ρ0.06, −90°→90° (0.03, 0.30)    torus
(0.02, 0.30)
arc c(0.02, 0.32) ρ0.02, 270°→180° (0, 0.32)      apophyge
```

**Shaft.** z 0.32–8.90.
- **Flutes.** z 0.60–8.62, stopped square top and bottom.
- **Across the face:** a 0.070 margin, 7 flutes each 0.085 wide separated by 6 fillets of 0.0275, then a 0.070
  margin. The flute section is depth 0.030·sin(πt) across its width.
- **Returns.** The shaft's sides are plain.

**Capital.** Swept like the base. Points are (a, z):

```
(0, 8.90)
arc c(0, 8.93) ρ0.03, −90°→90° (0, 8.96)           astragal
(0, 9.14)                                          necking
(0.025, 9.14) (0.025, 9.17)                        annulet
arc c(0.025, 9.26) ρ0.09, −90°→0° (0.115, 9.26)    echinus
(0.115, 9.47)                                      abacus
```

The abacus top is flat, back to the wall.

**Entablature.** A ring swept about the z axis on the dome's stations. Points are (r, z), from the dome's surface
down to the wall; the solid is below and outboard. It is a cavetto crown, the corona, an ovolo bed moulding, a plain
frieze, then the architrave (ovolo crown and two fasciae):

```
(9.9875, 10.500)                                   on the dome sphere at the cornice top (elevation 2.866°)
(9.220, 10.500) (9.220, 10.460)
arc c(9.220, 10.400) ρ0.060, 90°→0° (9.280, 10.400)     cavetto crown
(9.280, 10.240)                                    corona face
(9.420, 10.240) (9.420, 10.210)                    corona soffit
arc c(9.510, 10.210) ρ0.090, 180°→270° (9.510, 10.120)  ovolo bed moulding
(9.510, 9.840)                                     plain frieze
(9.430, 9.840) (9.430, 9.820)
arc c(9.480, 9.820) ρ0.050, 180°→270° (9.480, 9.770)    architrave crown
(9.480, 9.630) (9.500, 9.630)                      upper fascia
(9.500, 9.470)                                     lower fascia
(10.000, 9.470)                                    soffit, back to the wall face (as built 10.01)
```

The cornice's top ledge, from r 9.22 to 9.99 at z 10.50, is the luminaire mount of §4.

**Archivolts**, round each door's arch on the drum's inner face.
- **Path.** From one springing (z 4.50) round the semicircle to the other. The band dies against the flanking
  pilasters' sides, trimmed to them.
- **Section.** (s out from the clear intrados, p proud of the wall along the door axis):

  ```
  (0.30, 0) (0.30, 0.10)
  arc c(0.27, 0.10) ρ0.03, 0°→90° (0.27, 0.13)
  (0.20, 0.13) (0.20, 0.10) (0.09, 0.10) (0.09, 0.08) (0, 0.08) (0, 0)
  ```

  That is an outer band with a rounded arris (0.13 proud), a middle fascia (0.10) and an inner fascia (0.08). Its
  inner return (s = 0) meets the joint's lining face on the clear intrados.

**Niche surrounds.** An architrave up each jamb from the sill (z 2.00) to the springing (4.67), round the head
(radius 1.00 + s), and down the other jamb. Section (s out from the opening edge, p proud):

```
(0.20, 0) (0.20, 0.05)
arc c(0.17, 0.05) ρ0.03, 0°→90° (0.17, 0.08)
(0.07, 0.08) (0.07, 0.06) (0, 0.06) (0, 0)
```

**Niche sill ledges.** A ledge 0.16 proud and 0.16 high (z 1.84–2.00), running laterally ±1.26 about the niche axis
along the drum face, with capped ends. Section (d proud, z):

```
(0, 1.84) (0.13, 1.84)
arc c(0.13, 1.87) ρ0.03, −90°→0° (0.16, 1.87)
(0.16, 2.00) (0, 2.00)
```

Its top continues back as the niche floor to the half-round. It carries the cast.

**Imposts.** See §2: they go to the doorway class detail.

**Catalogue.** The profiles above are promoted to `Data/Form/Profiles.json` under names for what they are:

| Key | Profile |
|---|---|
| `base.torus-apophyge` | pilaster base |
| `capital.doric-roman` | pilaster capital |
| `entablature.plain-frieze.cavetto-crown` | entablature |
| `archivolt.two-fascia-roll` | door archivolt |
| `architrave.niche.two-step-roll` | niche surround |
| `sill.rounded-ledge` | niche sill ledge |
| `coffer.two-step-bevel` | dome coffer (§3.4) |

### 3.4 The dome

**Inner surface.** A sphere R 10.00 about (0, 0, 10.00), lime plaster, trowelled. From the bottom up:

| Zone | Elevation | Detail |
|---|---|---|
| Lower plain band | 2.866° → 4° | 2.866° is the cornice top at z 10.50, r 9.9875 |
| Coffers | 4° → 64° | five rings of 28 |
| Upper plain band | 64° → 72.54° | z 18.988, r 4.384 up to z 19.539, r 3.000 |
| The eye's stepped curb | | inner profile below |

**The eye's stepped curb**, inner profile (r, z):

```
(3.000, 19.539) (3.000, 19.420) (2.750, 19.420) (2.750, 19.580) (2.500, 19.580) (2.500, 20.450)
```

That is a 0.12 hanging fascia, a step up, and the eye's cylindrical face (r 2.50) up to the curb top at 20.45.

**Coffers:** 5 rings × 28, each ring 12° of elevation, all sharing the same angles.

| Ring | Elevation | z | r | Cell | Rib | Step | Bevel |
|---|---|---|---|---|---|---|---|
| 1 | 4°–16° | 10.698–12.756 | 9.976–9.613 | 2.210 | 0.243 | 0.099 | 0.110 |
| 2 | 16°–28° | 12.756–14.695 | 9.613–8.829 | 2.081 | 0.229 | 0.094 | 0.104 |
| 3 | 28°–40° | 14.695–16.428 | 8.829–7.660 | 1.860 | 0.205 | 0.084 | 0.093 |
| 4 | 40°–52° | 16.428–17.880 | 7.660–6.157 | 1.559 | 0.171 | 0.070 | 0.078 |
| 5 | 52°–64° | 17.880–18.988 | 6.157–4.384 | 1.189 | 0.131 | 0.054 | 0.059 |

- **Cell** is the ring's cell width at mid-elevation: (2π/28)·10·cos(e_mid).
- **The section** is the same on all four sides of every coffer, scaled to the ring's cell. Points are (s inward
  from the cell edge, depth into the dome):

  ```
  (0, 0) (Rib, 0) (Rib, 0.12) (Rib+Step, 0.12) (Rib+Step, 0.26) (Rib+2·Step, 0.26) (Rib+2·Step+Bevel, 0.45)
  ```

  with Rib = 0.11·Cell, Step = 0.045·Cell and Bevel = 0.05·Cell. So each coffer has a rib frame on the sphere, two
  steps, and a bevel down to a panel 0.45 deep (on the sphere R 10.45).
- **Measuring in.** Offsets across θ use 1/(10·cos e_mid) radians per metre; offsets up the elevation use 1/10.
- **Ribs.** Neighbouring coffers' ribs meet on the cell edge.
- **Stations.** The sides run the same way as the neighbours' and the bands' rings, so they share stations.

**Shell.** Thickness 0.60. Travertine core; inner surface lime plaster (laid); outer face travertine, honed.

**Outer surface.** A sphere R 10.60 about (0, 0, 10.00), from its foot on the drum top (r 10.553, z 11.00) up to
r 2.90 (z 20.196). From there, (2.90, 20.196) → (2.90, 20.450) → (2.50, 20.450): the curb, up and over.

### 3.5 Glazing: the eye

This is the only opening to the sky. It is Glazed (D-004), after the British Museum's Great Court.

**Lattice.** Painted steel, one welded body (joint kind: welded and faired).
- **Ring.** A rectangular section r 2.42–2.50 × z 20.22–20.38, standing against the curb's inner face.
- **Bars.** Three directions, at φ 0°, 60° and 120°. Each direction has 7 bars at lateral offsets 0.80·k,
  k = −3 … 3. All three k = 0 bars cross at the centre, so the grid is triangulated with the node at its heart.
- **Bar section.** 0.05 wide × 0.10 deep, centred at z 20.30 (so z 20.25–20.35).
- **Bar lengths.** Each bar runs 0.04 into the ring. Lengths are 4.920 (k 0), 4.653 (±1), 3.737 (±2) and
  1.080 (±3).
- **Paint.** As built, a dark bronze-grey, sRGB #1E1C19: the lattice reads as a dark drawing against the sky.
  *Correction:* as built, the lattice was shaded as a bare metal (metallic 1); it is a painted dielectric coat.

**Glass.** One pane of laminated low-iron glass, 2 × 5 mm with a 0.76 PVB interlayer (10.76 mm). It is a disc r 2.50,
underside at z 20.38 on the ring and top at 20.39, its edge against the curb's inner face.
- *Correction:* as built it was 10 mm monolithic. Overhead glass is laminated, and the PVB also cuts UV below
  380 nm.

**Node.** Gilt bronze. A sphere of radius 0.12 centred at (0, 0, 20.25), enclosing the bars' crossing. It is the
nodus of the sun clock (§5).

**Obstruction.** The bars and ring cover about 4.8 m² of the eye's 19.6 m² (about 24 %).

### 3.6 The sun clock's drum (a moving body)

Everything in this section moves with the lift (§6). Coordinates are z_d (z = z_d lowered, z = z_d + 3.50 raised).
Raised, the drum is a tempietto 14 m across (r 6.99) with its door to the west, facing the Salon.

**Build-up.**
- **Wall and dial slab.** A travertine core, faced with cream marble (walls) or statuary (dial).
- **Soffit.** Cast fibrous-plaster coffers hung under the slab.
- **Polish.** A welded steel carcass in place of the stone core. This is the real machine's section: tens of
  tonnes, not the ≈ 200 t of a solid core.

**Wall.**
- Face r 6.900 from the base (z_d −3.08) to the architrave (−0.45).
- Inner face r 6.545, from −3.56 to the rim groove (−0.06).
- Between them, 0.355 thick.
- Everything stays inside r 6.99 (the cornice, the plinth and the dial's edge), so the drum slides into its pocket.

**Plinth and foot.**
- The plinth runs all round at r 6.99 from z_d −3.56 to the sill level −3.50, including under the door.
- The foot is a flat annulus r 6.545–6.99 at z_d −3.56, and it bears on the rams.

**Base**, above the sill. It is a plinth, a torus, a fillet, an astragal and a chamfer, and it is not run across the
door. Points are (r, z_d):

```
(6.990, −3.500) (6.990, −3.300) (6.955, −3.300)
arc c(6.955, −3.265) ρ0.035, −90°→90° (6.955, −3.230)
(6.925, −3.230) (6.925, −3.215) (6.915, −3.215)
arc c(6.915, −3.200) ρ0.015, −90°→90° (6.915, −3.185)
(6.905, −3.175) (6.905, −3.100) (6.900, −3.080)
```

**Entablature**, all round. It is two fasciae and a crown fillet, a plain frieze, an ovolo, and a step out to the
corona, which is the dial's edge. Points are (r, z_d):

```
(6.900, −0.450) (6.930, −0.450) (6.930, −0.395) (6.940, −0.390) (6.940, −0.335) (6.952, −0.330) (6.952, −0.315)
(6.915, −0.315) (6.915, −0.195)
arc c(6.915, −0.150) ρ0.045, −90°→0° (6.960, −0.150)
(6.990, −0.150) (6.990, 0.000)
```

**Door**, to the west (φ 180°).
- 2.60 wide at the wall face and 2.95 high above the sill (z_d −3.50 → −0.55).
- The jambs are radial (half-angle 10.860°), so every moulding ends on them.
- The head is conical: a semicircle of radius r·sin(10.860°) at each radius (1.300 at the face, 1.233 at the inner
  face), springing z_d −1.85 (1.65 above the floor when raised).
- The sill is flat from r 6.545 to 6.99 at z_d −3.50.

**Door dressing.**
- **Archivolt.** It runs down both jambs to the base top (z_d −3.10). Its centre line is 1.355 from the door axis,
  1 cm clear of the opening. Section (across, proud): a 90 mm band 30 mm proud, with a half-round bead of r 0.012 on
  its outer edge:

  ```
  (0.045, −0.010) (0.045, 0.036)
  arc c(0.033, 0.036) ρ0.012, 0°→180° (0.021, 0.036)
  (0.010, 0.036) (0.010, 0.030) (−0.045, 0.030) (−0.045, −0.010)
  ```

- **Imposts.** Blocks from 1.300 to 1.430 off the axis, z_d from the springing −0.07 to +0.01, 0.06 proud.
- **Keystone.** A trapezoid, 0.15 wide at z_d −0.545 widening to 0.20 at −0.440, 0.07 proud.

**Pilasters.** 12 of them, at φ = 15° + 30°·k. The two at 165° and 195° flank the door, clear of its archivolt.
Each has:

| Part | Width | Proud | z_d |
|---|---|---|---|
| Pedestal block | 0.50 | 0.075 | −3.10 → −2.98 |
| Fluted shaft | 0.44 | 0.055 | −2.99 → −0.61 |
| Necking | 0.47 | 0.064 | −0.62 → −0.56 |
| Echinus | 0.49 | 0.071 | −0.56 → −0.53 |
| Abacus | 0.51 | 0.080 | −0.53 → −0.44 |

The shaft carries 5 flutes of 0.0632 between fillets of 0.016, with 0.03 margins. Each flute is a circular arc
sagging 0.018.

**Soffit**, the dial slab's underside, seen from the stair.
- **Ring ribs** at z_d −0.13, at r 0–0.55 (the centre, under the pendant), 1.25–1.35, 2.25–2.35, 3.45–3.55 and
  4.75–4.85.
- **Five coffer rings**, with radial ribs between the coffers:

  | r | Coffers | Radial rib |
  |---|---|---|
  | 0.55–1.25 | 8 | 6.5625° |
  | 1.35–2.25 | 12 | 2.8125° |
  | 2.35–3.45 | 16 | 1.875° |
  | 3.55–4.75 | 24 | 0.9375° |
  | 4.85–6.20 | 32 | 0.9375° |

- **Each coffer** is sunk in two steps: from the rib (−0.13) to a 40 mm inset frame at −0.115, then to the panel at
  −0.100.

**Bronze rim.** A ring. Points are (r, z_d):

```
(6.190, −0.090) (6.190, −0.125)
arc c(6.205, −0.125) ρ0.015, 180°→270° (6.205, −0.140)
(6.320, −0.140) (6.330, −0.132) (6.340, −0.140) (6.460, −0.140)
(6.460, −0.060) (6.545, −0.060)
```

That is a rounded arris, a fine groove, and the deep groove r 6.46–6.545 up to −0.06. Lowered, the stair wall's top
(−0.07) sits in that groove, so raised there is no open slot into the pocket.

**Pendant boss.** Gilt bronze, over the well. A lathe profile (r, z_d):

```
(0, −0.265)
low dome r = 0.11·sin t, z = −0.265 + 0.04·(1 − cos t), for t = 0 → 90° (0.110, −0.225)
bead arc c(0.125, −0.225) ρ0.015, 180°→0° (0.140, −0.225)
(0.140, −0.215) (0.240, −0.190) (0.340, −0.170) (0.400, −0.158)
arc c(0.400, −0.148) ρ0.010, −90°→90° (0.400, −0.138)
(0.400, −0.120)
```

**On top of the drum:** the dial and its inlay (§3.1).

### 3.7 The shaft (fixed, inside the served well)

**Stair wall, the "sleeve".**
- Face r 6.48, back r 6.54, from the landing (z −6.00) to its top at z −0.07.
- Cut on the north by the portal: jambs x = ±1.80, head at the lintel, z −3.68.

**Pocket.** The annulus r 6.54–7.00, floor at z −3.58, open up to z 0. Its outer face is r 7.00 from −3.58 to 0. The
drum's wall slides here.

**Masonry.**
- From r 7.00 to 7.60 above the pocket floor, and from r 6.54 to 7.60 below it, down to the landing.
- The outer face at r 7.60 runs from z −6.00 to 0. It is cut by the port's arch.
- The core is travertine. The faces seen from the stair are cream marble, honed.

**The port.**
- **At the port face (r 7.60).** 3.60 wide (jambs x = ±1.80), sill on the landing (−6.00), a semicircular head
  springing −3.40, crown −1.60.
- **The arched reveal** runs from r 7.60 in to the tympanum face at r 7.05.
- **Inside r 7.05, the drum's pocket passes over.** There the opening is closed above z −3.68 by:
  - a **lintel** (underside −3.68) from r 6.48 to 7.05;
  - a **tympanum** on the cylinder r 7.05, filling the arch from −3.68 up to the arch head.
- **So the portal onto the stair is 3.60 × 2.32**, flat-headed. The lunette is solid because the lowered drum's wall
  passes behind it.

**Ram housings.** New: they house the drive the design left open (§6).
- Eight vertical bores of Ø 0.36, centred on r 6.77 (under the drum's foot ring).
- At φ = 22.5° + 45°·k. These positions clear both the portal (±15.42° about 270°) and the drum's door (±10.86°
  about 180°).
- From the pocket floor (z −3.58) down to z −5.90, all inside the well.

**Port dressings on the stair side** (polish; as built in statuary):
- an archivolt of two orders: a plain arch ring 0.18 wide, 0.14 proud, and a moulded architrave out to 0.40 with
  its crown 0.20 proud;
- plinth blocks 0.40 high;
- a keystone;
- moulded transoms on the lintel on both faces.

### 3.8 The stair

After Momo and Bramante: one wide helical flight round an open well Ø 4.40. It descends anticlockwise in plan, with φ
decreasing: from the drum's door (west), round by the south and east, to a landing whose port opens north.

**Layout** (the plan numbers, `SunClockKit.cpp:82-121`):

| | |
|---|---|
| Risers | 40 × 0.150 = 6.00. Riser 1 is the step from the raised drum's sill (z 0) down to the top landing. |
| Top landing | Tread 1, z −0.15. φ 194° → 164° (30°), spanning the drum's door. |
| Treads 2–19 | z −0.30 … −2.85. Each turns α = (214° − 20°)/37 = 5.2432° (0.091512 rad). |
| Mid landing | Tread 20, z −3.00. β = 20°, from φ 69.622° to 49.622°. |
| Treads 21–39 | z −3.15 … −5.85, each α |
| Last riser | At φ −50° (310°), so the foot faces the port across the landing. The landing floor is at −6.00. |
| Nosing lines | Front(1) = 164°. Front(K) = Front(K−1) − α, except Front(20) = Front(19) − β. Tread K spans from Front(K) to Front(K−1). |

**Span.** The treads run from the inner string (r 2.35) to the stair wall (r 6.48): 4.13 wide.

**Goings:**

| r | Going | Note |
|---|---|---|
| 2.35 | 0.215 | at the string |
| 3.61 | 0.330 | Blondel 2R + G = 0.630 exactly |
| 4.40 | 0.403 | mid-span, 2R + G = 0.703 |
| 6.48 | 0.593 | at the wall |

The walking line is declared at **r 3.61**, 1.26 from the string (Form.StepsFitTheBody).

**Treads.** Flat, flush with the nosing line.
- **Nosing.** A rounded nosing over each riser. Section (forward from the riser line, up from the tread top):

  ```
  (0, drop − 0.01) (0, −0.050) (0.022, −0.040)
  arc c(0.022, −0.020) ρ0.020, −90°→90° (0.022, 0.000)
  ```

- **Nosing plan.** The top edge stands 22 mm forward of the riser line.

**Inner string.** r 2.20–2.35. Its top is 0.12 above the pitch line, but never above z −0.14, so it stays under the
lowered dial. It is flush with the top landing.

**Pitch line.** Through the treads' mid-going, flat on the landings.

**Soffit.** A smooth helicoid 0.40 under the pitch line, never below −6.02, running into the landing floor at the foot.
It spans from the well (r 2.20) to the wall.

**Collision.** From the treads and landings themselves. The body climbs 0.150 risers within the human measure's
step height. *Delta:* the as-built hidden ramp is not ported.

### 3.9 Fittings: balustrades, housings, handrails

The Rotunda has **no benches, labels, signs or floor map**: the board says "There is no floor map". The only fittings
are the stair's.

**Well balustrade.** In the plane r 2.15, side-fixed by brackets to the inner string. Heights are above the string
top:
- **Bars.** A bottom bar at 0.0575, a mid bar at 0.5325 and a top bar at 0.8675. Each is 25 mm high × 20 mm.
- **Handrail.** At 0.90: a soft rounded bar 60 × 45 mm (superellipse, exponent 0.7).
- **Standards.** 30 × 30 mm, at most 1.6 m apart and at every kink, running down to 0.25 below the string top onto
  brackets.
- **Balusters.** 14 mm square, about 0.110 apart, between the bottom and mid bars.
- **Rings.** Between the mid and top bars, true circles on the rake touching both bars (radius about 0.155·cos
  rake). Each has a 12 mm band, 12 mm thick, a boss r 0.022, and four spokes 7 mm wide at 45°.
- **Newel** at the foot: a post 56 mm square to handrail height + 0.03, a cap 72 × 20 mm, and a ball finial r 0.045.

**The split** is at tread 8's nosing.
- **Below it, the balustrade is fixed.** Its handrail tops out at −0.23 there, under the lowered dial's ribs (−0.13).
- **Above it, the balustrade retracts.** This covers the upper flight's well side, and a straight run across the
  top landing's back edge (φ 194°, in the plane 0.07 behind it). Each run is a free panel (standards, balusters,
  rings, handrail, and a carrier bar at the foot at −0.22 … −0.18) standing in a hollow housing.

**The housings.**
- **Section.** A U channel 1.20 deep with a 30 mm floor and 40 mm end blocks. The slot is 76 mm (the 60 mm handrail
  passes with 4 mm to spare).
- **Along the string:** r 2.07–2.20, top flush with the string.
- **Behind the landing:** 0–0.14 behind its back edge, top flush with the landing.
- **Sides:** statuary marble, with sunk fields from −0.10 to −1.09 where seen.
- **Bronze parts:** a cap round the slot 0.06, a shoe 0.05, and end plates.

While the dial is down, a panel sits 0.94 lower, wholly inside its housing, with its handrail just under the slot's
lips. It moves by the cam in §6.

**Wall handrail.**
- A round bar Ø 0.050 at r 6.40, 0.92 above the pitch line, on brackets every 1.25 m.
- It runs from tread 7's nosing to 4° past the last riser. At the top it returns to the wall on a rose, with its top
  at −0.18, under the lowered dial.
- The top flight's wall side has no rail; the well side guards it.

### 3.10 The exterior dress (polish, recorded so nothing is lost)

The as-built dress was the façade actor's. Each room owns both faces of its shell, so it is now the Rotunda's (the
Grounds sheet §3.11 hands it over). Plan numbers are from `MuseePlan.h:518-561` and `MuseeFacadeStructure.cpp:600-744`.

**Podium.** `podium.rusticated`, 2.00 high, 0.54 proud, then a gravel strip and stone edge to r 12.76 on the free
arcs.

**Skin.** 0.20 of travertine ashlar over r 11.20, framing the door arches down to the ground.

**Order.** `order.ionic-vignola` with D 0.827:
- entablature z 9.44–11.30, with dentils;
- an attic to 12.15 under a small cornice;
- a wash in to the dome.

**Pilaster pairs** at φ 33.75°, 56.25°, 123.75°, 146.25°, 213.75°, 236.25°, 303.75° and 326.25°.

**Blind windows**, on the four diagonals:
- arched, 1.50 wide, sill 3.2, springing 6.2;
- frame 0.20, transom 0.75;
- a round tablet r 0.38 at z 8.2.

**Stone cover over the dome.** R 11.10 about z 10.00.
- Two step rings of 0.40 on the attic (outer r 11.25 and 10.98).
- A smooth dome up to r 3.0.
- A rolled ring round the eye, r 2.92–3.28, on the curb at 20.45.

The dome is seen as "the southern mountain" from Chenghuai's 見山 pavilion.

**Night floodlights.** Eight, grazing the drum pilasters from the podium cap (control class `facade`).

### 3.11 What the Rotunda does not build

- The doorcases, thresholds, bronze strips, linings, surrounds and passage masonry, including the west passage's
  2.20 m shell: these are Site's joints.
- The neighbours' floors in the doorways.
- The Classical Hall's side of the well.

### 3.12 Finish schedule (seeds `Data/Stages/Rotunda/Finishes.json`)

The as-built material each key replaces seeds its Matter record (D-017).

| Surface or part | Key | As built |
|---|---|---|
| Drum inner face, shadow gap, niche recesses and half-domes, sliver reveals | `cream-marble.honed.aged` | M_Marble_Wall |
| Pilasters (base, shaft, capital) | `cream-marble.honed.aged` | M_Marble_Wall |
| Entablature, archivolts, niche surrounds, niche sill ledges | `lime-plaster.run.aged` | M_Plaster_Moulding |
| Dome inner surface: bands, coffers, eye curb | `lime-plaster.trowelled.aged` | M_Plaster_Coffer |
| Drum core, dome shell core, floor bed, shaft core, drum-body core | `travertine.sawn.as-laid` | (solid) |
| Drum outer face, drum top, dome outer shell | `travertine.honed.weathered` | wall material; *delta:* the exterior stone is travertine, and the cream marble stays inside |
| Eye lattice (ring and bars) | `steel.painted.aged`, paint #1E1C19 | M_RibSteel |
| Eye glass | `low-iron-glass.laminated.aged` | M_Glass |
| Node | `bronze.gilt.aged` | M_Gilt |
| Fixed ring, tones A and B | `statuary-marble.polished.worn`; A vein-cut, B cross-cut | M_SunClock_Marble, M_SunClock_MarbleAlt |
| Dial bands A and B | `statuary-marble.polished.worn` | as above |
| Dial band r 1.40–1.52; rosette's rosso | `rosso-antico.polished.worn` | M_Marble_Rosso |
| Dial band r 6.30–6.38; rosette's nero | `nero-marquina.polished.worn` | M_Marble_Nero |
| Hour ring, fillets, hour lines, minute ticks, numerals, rays, motto letters | `bronze.cast.worn` | MI_bronze_gold_r40 |
| Meridian, gilt boss and its beads, motto stops | `bronze.gilt.worn` | M_Gilt_Aged; the meridian is new |
| Sun-clock drum: faces, base, entablature, pilasters, door, dressing | `cream-marble.honed.aged` | M_SunClock_Drum |
| Drum soffit coffers | `fibrous-plaster.cast.aged` | M_Plaster_Coffer |
| Drum rim | `bronze.cast.aged` | MI_bronze_gold_r40 |
| Pendant boss | `bronze.gilt.aged` | M_Gilt_Aged |
| Stair wall, portal jambs, lintel, tympanum, stair soffit | `cream-marble.honed.aged` | M_Marble_Wall |
| Treads, nosings, landings, landing floor, port dressings | `statuary-marble.polished.worn` | M_SunClock_Marble |
| Inner string, housing sides | `statuary-marble.polished.aged` | M_SunClock_Marble |
| Balustrade: standards, bars, balusters, rings, brackets, newel, housing caps, shoes and plates | `bronze.cast.aged` | MI_bronze_gold_r40 |
| Handrails (well and wall) | `bronze.brushed.handled` | MI_bronze_gold_r40; *delta:* the board's "brushed bronze: handrails" |
| Rams (hidden) | `steel.painted.as-laid` | none |

---

## 4. Light

**Daylight.**
- `rotunda.eye` is the one opening to the sky: Glazed, clear Ø 5.00 (19.6 m²), with the lattice's triangulated
  shadow. The glass transmission, lattice included, comes from Matter and Form.
- A first-order daylight factor on the floor is about 1.3 %: roughly 130 lx under a 10 klx overcast sky.
- The sun patch is the eye's 5 m disc projected along the idealised sun onto the floor, centred on the node's shadow
  point. It reaches no further than r 9.28, so it never touches the wall or the niches.
- Borrowed light enters through the four open portals: the Hall of Light's glass, Albion's glass court, Chenghuai's
  open porch, and the Salon passage.
- **No blinds.** The works here are unpainted plaster casts, and D-014 forbids automatic blinds anyway.

**Luminaires** (seeds `Data/Stages/Rotunda/Lighting.json`). As built, the Rotunda proper had no luminaire: it lived by
the eye. Two things now require light at dusk and at night:
- D-006 (the museum is always open, with dusk switching and daylight-linked dimming);
- the use targets below.

Every fitting is concealed; none may be seen from the floor.

| Mount key | Where (stage frame) | Catalogue type | Count | Aim |
|---|---|---|---|---|
| `rotunda.cornice-ledge.cove` | The cornice's top ledge, a continuous ring r 9.22–9.99 at z 10.50, hidden behind the cavetto crown's lip at r 9.22 | `linear-cove`, 4000 K wanted; the catalogue has only `linear-cove.3000k` (gap) | ≈ 59 m, the full ring at r 9.40 | up, washing coffer rings 1–3 |
| `rotunda.cornice-ledge.spots` | The same ledge, on a concealed track at r 9.60, z 10.50 | `track-spot.4000k.*`; beam and count by Light's Scheme | Scheme; about 2 per cast and a ring for the floor | the four casts (about 30° from the vertical where the geometry allows) and the floor |
| `rotunda.sun-clock.soffit` | The drum's soffit: alternate coffers of its 4th ring, r 4.15, φ = 7.5° + 30°·k, face at z_d −0.108 | `downlight.4000k.31deg` | 12 | straight down onto the treads |
| `rotunda.sun-clock.pendant` | Under the pendant boss, (0, 0, z_d −0.29) | `downlight.4000k.31deg`, or a narrower beam if the catalogue gains one | 1 | down the well onto the rosette, 9.21 m below when raised |
| `rotunda.stair.handrail` (candidate) | In the well balustrade's handrail, uplighting the soffit | `linear-slot` or `linear-cove`; Scheme | 0 unless the stair's ceiling target fails | up |

**Operation.**
- **Cove and cornice spots.** Control class `circulation`, daylight-linked: they hold the floor target at every
  reference moment. They switch at dusk.
- **The 13 drum luminaires.** They follow the channel `rotunda.sun-clock.lift` and are on while the lift is above
  0.30 m (as built, `SunClock.cpp:303-309`). They ride on the moving body.
- **CCT.** 4000 K throughout. The as-built stair was 3800 K, and the catalogue's nearest is 4000 K.
- **Removed.** The as-built fill light (a point source with no fitting, `SunClock.h:145-151`) is not ported: it is
  forbidden by Light.LuminairesFromCatalogue.

**Targets by use.** These are the use classes in `UseTargets.json` and the care classes in `Care.json`.

| Surface | Target |
|---|---|
| Floor: dial and ring, r 0–10 | `entrance-hall`, 100 lx maintained, Uo ≥ 0.4. At night, `circulation.night` (50 lx), never below `escape-route`. |
| Stair treads, landings and landing floor | `stairs`, 100 lx, day and night whenever raised |
| Stair wall | 75 lx |
| Drum soffit (the stair's ceiling) | 50 lx |
| The four casts | Care class for unpainted plaster: `stone-and-bronze` (low responsivity, no lux maximum). The modelling aim is 150–300 lx on each cast's front. |

There are no paintings and no paper or silk here, so the 150–200 lx and 50 lx limits do not arise.

**Design targets**, luminance order between named surfaces, measured and never tuned:

*By day* (clear sky, 12:00, and overcast):
1. The eye's glass (the sky through it) is the brightest surface in view. The dial inside the sun patch is next.
2. The dial outside the patch ≥ the drum's inner face ≥ coffer ring 1 ≥ coffer ring 5. The dome reads darker than
   the drum and darkest round the eye (the Pantheon order).
3. Each cast ≥ 1.3 × the back of its niche.

*At night:*
1. Coffer rings 1–3 (cove-washed) ≥ the drum's inner face. The dome glows over a quieter drum.
2. Each cast ≥ 1.3 × its niche back.
3. The eye is dark: the real night sky. No luminaire is visible from any standable point (Light.NoGlare).

*On the stair:* treads ≥ stair wall ≥ drum soffit. The rosette is pooled by the pendant light.

---

## 5. Time and weather

**Weather over the glazing.** `rotunda.eye` takes the observer `museum` (D-005). Its rain wets the eye's outer face
and the exterior stone through Matter's surface law. The sun and the hour are always the museum's.

**The sun keeps the approved idealised path (D-003).** The model is declared `idealised` in `Data/Time/Museum.json`.
- **Nodus.** The node's centre, (0, 0, 20.25).
- **Dial plane.** z 0, the top of the lowered dial.
- **Shadow circle.** r 6.78, the hour ring.
- **Elevation.** Constant, atan(20.25 / 6.78) = **71.489°**. The old sky used the lattice height 20.3 (71.531°),
  which puts the node's shadow 1.7 cm inside the ring, still on the bronze. This sheet uses the node's true centre.
  D-003's "20.3 m" is the rounded figure.
- **Direction.** At civil local hour h (06:00 ≤ h ≤ 18:00, the museum city's clock, including daylight saving):
  - the shadow point is S(h) = 6.78·(cos φ_h, sin φ_h, 0), with φ_h = 180° + 15°·(h − 6);
  - the sun's direction is from S(h) through the node;
  - the sun's azimuth (bearing) is 90° + 15°·(h − 6): east at 06:00, south at 12:00, west at 18:00.
- **Outside 06:00–18:00** there is no sun. The moon and stars through the eye are the real Meeus ephemeris for the
  observer (D-003).
- **Evidence.** As built: `MuseeSky.cpp:109-116` and `:133-138`.

**Time::Dial.** `Musee::Time::Dial(model, nodus, dial plane)` returns the hour-line angles φ_h and the shadow radius.
The Rotunda lays its hour lines, meridian, minute ticks and numerals from that output. Nothing is typed by hand.

**Time.DialTrue** checks, for every hour 6 … 18 and every minute tick, that the nodus's shadow lands on the laid mark
within the mark's half-width. Under the idealised model the dial does not depend on latitude, so the Beijing/New York
switch stays free (D-002).

**The raised dial.** It is 3.50 m above its plane. It still shows marks but is not a dial there. The check runs on
the lowered configuration.

---

## 6. Motion

**Mechanism `rotunda.sun-clock.lift`.**
- **Body.** `rotunda.sun-clock.drum`: the dial with its inlay, the drum wall and dressings, the soffit, the rim, the
  pendant, and the 13 downlights.
- **Joint.** Prismatic, vertical, travel **0 → 3.50 m**. Named states: `lowered` (0) and `raised` (3.50).

**Drive.** An S-curve profile, travel time about 8.0 s (as built: 8 s with smoothstep easing, `SunClock.cpp:117-121`),
within these limits:

| Limit | Value |
|---|---|
| v max | 0.60 m/s |
| a max | 0.35 m/s² |
| jerk max | 1.0 m/s³ |

That gives about 7.9 s end to end.

**The machine.** Proposed, since the design names none.
- Eight synchronised three-stage telescopic hydraulic rams, closed height ≤ 2.30.
- They stand in the ram housings at r 6.77, φ = 22.5° + 45°·k, from z −5.90 up to the pocket floor (−3.58).
- Their heads bear on the drum's foot ring (z_d −3.56).
- The mass is computed by Motion from Form's volumes × Matter's densities: about 200 t with a solid stone core, and
  much less with the polish carcass. Each ram is rated for 1/8 of it with margin (Motion.Balanced).
- Hydraulics need no counterweight.

**Travel volume.** The cylinder r ≤ 6.99 from z −3.56 (the lowered foot) to +3.50 (the raised dial), plus the pendant
and downlights below the soffit.

**Clearances** (Motion.Clears ≥ 5 mm at every sampled phase; as built, `SunClock.h:35-51`, `SunClockBuild.h:40-54`):

| Pair | Clearance |
|---|---|
| Drum wall inner face (r 6.545) ↔ stair wall back (6.54) | 5 mm |
| Drum's outermost (6.99) ↔ pocket face and ring edge (7.00) | 10 mm |
| Lowered foot (−3.56) ↔ pocket floor (−3.58) | 20 mm |
| Soffit ribs (−0.13) ↔ string top (≤ −0.14) | 10 mm |
| Rim underside (−0.14) ↔ top landing (−0.15) | 10 mm |
| Rim groove roof (−0.06) ↔ sleeve top (−0.07) | 10 mm |
| Ribs ↔ fixed balustrade's handrail (≤ −0.23) | ≥ 100 mm |
| Ribs ↔ wall rail top (−0.18) | 50 mm |
| Downlight at φ 187.5° (−0.108, plus its trim) ↔ top landing | ≥ 30 mm |

**Mechanism `rotunda.stair.balustrade`.** A follower in the same coupling group.
- **Bodies.** The two retracting panels.
- **Joint.** Prismatic, vertical, travel 0.94.
- **Coupled to the lift** by a cam with a dwell at each end: panel offset = clamp(h − 1.20, 0, 0.94) − 0.94, where h
  is the lift (as built, `SunClockBuild.h:140-143`).
- **So** the panels stay home until the dial is 1.20 up, rise with it over the next 0.94, then stand.
- **Lowering**, they are home before the drum comes within 1.07 of them. Nothing rides under the moving ceiling.

**Control.** IMuseeUsable on the lift. Verbs and captions are FText in the spec (as built, `SunClock.cpp:339-404`):

| Verb | Available | Caption |
|---|---|---|
| **Raise the sun clock** | State `lowered`, the body on the ring (7.35 < r < 10.00, \|z\| < 0.40) | "The sun clock rises…"; on arrival, "Under the sun clock, a stair winds down into the earth." |
| **Raise** (refused) | Body on the dial (r ≤ 7.35, −0.30 < z < 0.60) | "Step off the sun clock to raise it." |
| **Lower the sun clock** | State `raised`, the body on the ring, and nobody inside r 7.20 or below z −0.30 | none |

**Interlocks and the occupancy guard** (D-014: Motion owns the guard):
- raising holds while any body is on the dial;
- lowering reverses to raising if a body enters r < 7.20 or goes below z −0.30.

The lift never lifts a visitor and never lowers onto one.

**Channel.** The stage declares both ends of `rotunda.sun-clock.lift` (value: the lift in metres). Motion publishes
it, and the drum's luminaires follow it.

**Configurations.** `lowered`, `raised`, and sampled phases at 0.30, 1.20, 1.67, 2.14 and 3.00 m. These are the cam's
knees and the lights' switch point. Form.Sealed, Motion.Clears and Site.Reachable run on each.

---

## 7. Nature

None. No plants, no water, no ground.
- Precipitation stops at the shell: the eye is glazed, and the Rotunda is Enclosed.
- Wetness and ponding on the eye's glass, and on the exterior drum top and dome, are Matter's surface law over
  Form's ponding field.
- The flat glass inside its 60 mm curb upstand will pond. That is expected, and noted in §12.

---

## 8. Art (seeds `Data/Stages/Rotunda/Hang.json`)

**Four plaster casts**, one in each diagonal niche, from SMK's Royal Cast Collection (public domain 3D scans).

| Anchor (kind Plinth) | φ | Work key | Title, artist, date | Height | Scan box w × d | Cast | Scan |
|---|---|---|---|---|---|---|---|
| `rotunda.niche.ne` | 315° | `classical-dying-slave` | *Dying Slave*, Michelangelo, 1513–1516 (original: Louvre) | 2.28 | 0.804 × 0.739 | SMK KAS87 | `assets/classical/dying-slave.stl` / `.glb` |
| `rotunda.niche.se` | 45° | `classical-rebellious-slave` | *Rebellious Slave*, Michelangelo, 1513–1516 (Louvre) | 2.13 | 0.787 × 0.538 | SMK KAS86 | `rebellious-slave.*`, turn [[0,0,1],[0,1,0],[−1,0,0]] |
| `rotunda.niche.sw` | 135° | `classical-venere-italica` | *Venere Italica*, Antonio Canova, 1804–1812 (Palazzo Pitti) | 1.73 | 0.464 × 0.528 | SMK KAS793 | `venere-italica.*` |
| `rotunda.niche.nw` | 225° | `classical-bacchus` | *Bacchus*, Michelangelo, 1496–1497 (Bargello) | 2.08 | 0.866 × 0.541 | SMK KAS83 | `bacchus.*`, turn [[−1,0,0],[0,1,0],[0,0,−1]] |

**Sources.** Heights, casts and scan URLs are from `assets/classical/SOURCES.json` (kind `rotunda`). Captions are from
`data/classical.json`: title, artist, year, original's collection and city, and notes. The scan boxes are the
converted glTFs' bounds. The casts are not in `assets/collection.json`; Art's catalogue takes them from these two
files.

**The hang rule** (as built, `classical_art.py:262-279`):
- Each cast stands on its niche's sill ledge at z 2.00.
- Its vertical axis is at r 10.25 on the niche axis, 0.25 behind the drum's inner face.
- Its front is turned to the gilt sun: yaw = φ + 180°.
- The scan is scaled to its recorded height (Art.TrueSize), with its feet on the sill.

**Fit.** Every cast clears its niche:
- the deepest (the Dying Slave, 0.739) spans r 9.88–10.62, on the ledge (from r 9.84) and inside the half-round
  (r 10.92 at its lateral ±0.40);
- the tallest top (4.28) stays under the springing (4.67).

Form.NoOverlap checks each scan against its niche.

**No frames, cases or plinths.** The niche sill is the plinth.

**Care.** Unpainted plaster, class `stone-and-bronze`; lighting as in §4.

**Credit** (Art.Credited): "Cast: SMK – National Gallery of Denmark, The Royal Cast Collection, KAS87" (and so on).
The scan is in the public domain (SMK Open, Public Domain Mark). Provenance is the SMK download URL in `SOURCES.json`.

**Labels.** None physical. The caption appears by gaze (IMuseeCaptioned through Art; Visitor shows it).

**Also Art's:** the sun clock's text is not a work. It is architecture (Form, from Time::Dial).

---

## 9. Sound

**Acoustic character.** A very live stone rotunda.
- RT60 per octave is computed by Sound from `rotunda.hall` (about 5,240 m³, plus niches and coffers) and Matter's
  absorption. The surfaces are polished statuary, honed cream marble, lime plaster, the glass eye, and the four open
  portals (about 83 m² opening to the neighbours).
- A Sabine estimate gives several seconds at 500 Hz. It is computed, never tuned.
- `rotunda.stair` is its own enclosure: a 13 m stone cylinder, 6 m deep.
- Occlusion and passage run through the four portals, and through the drum's door while raised.

**Emitters tonight.**
- Footfalls by substance, from FMuseeContact: statuary marble (polished) on the dial, ring, treads and landing;
  bronze on the inlays and the gilt boss.

**Emitters for polish.**
- **The lift's voice.** A hydraulic pump and the drum's stone sliding in its pocket, from FMuseeActuation, attached by
  key to `rotunda.sun-clock.drum`.
- **Rain on the eye's glass.** Weather ambience on Form's exposed glass surface (19.6 m²), from the `museum`
  observer.

There was no sound as built.

---

## 10. Visitor

**Arrive.** On the gilt sun, facing west down the Salon's line.
- The four doors show four chapters: the Salon (west), Chenghuai (north), Albion (south, 53 m down its nave) and the
  Hall of Light (east, "the way on").

**Look.**
- Up at the coffered dome and the lattice eye.
- At the point of shadow walking the numerals, VI in the west to VI in the east.
- The hour key T (`musee.time.advance-hour`) moves the hour, and the shadow with it.
- The city key changes nothing on the dial (D-002, D-003).

**Captions.** Gaze at a cast shows its caption: title, artist, date, original, and the SMK cast number.

**Use.** From the ring: **Raise the sun clock**, then walk through the tempietto's west door and down 40 risers to
the Classical Hall. **Lower the sun clock**, also from the ring, once everyone is out (§6).

**Sit.** Nowhere. The design has no seat in the Rotunda.

**Walk.** On the ring, the dial, the treads and the landings. The body climbs the helical stair on its real treads.
The top landing is 0.15 below the raised drum's sill, and the 6 cm stair-wall top between them is a 0.07 step.

**Step-to (VR).** It may land on the ring, the dial, the treads and the landings. It never lands on the raised dial's
top (3.50 m up), the niche sills, or the cornice.

---

## 11. The old code map

Re-express, don't copy: the builder reads these for numbers and intent, and rebuilds through Form, Matter, Light,
Motion and Site. Paths are relative to `C:\Users\anan9\Desktop\Projects\Musee Vision`, and the tree is read-only.

**Plan numbers**
- `windows/Source/MuseeVision/Plan/MuseePlan.h:26-28`: arrival on the gilt sun, yaw 180°.
- `MuseePlan.h:30-64`: the Rotunda: drum, doors, niches, coffers, eye, node, shadow radius, passages.
- `MuseePlan.h:119-158`: the sun clock: drum radii, lift 3.5 m in 8 s, door, shaft, pocket, landing, port.
- Mating evidence, the neighbours' numbers:
  - `MuseePlan.h:258-299` (the Hall of Light meets the drum);
  - `MuseePlan.h:461-509` (the Classical Hall meets the shaft at r 7.6);
  - `MuseePlan.h:518-561` (the façade dress numbers: DrumD, the dome cover).

**The drum, order and dome:** `windows/Source/MuseeVision/Rotunda/RotundaStructure.cpp`

| Lines | What |
|---|---|
| 24-60 | segment counts, shadow gap, pilaster and flute heights, cornice top, shell radius, eye curb, lattice ring |
| 84-136 | the profile builder |
| 281-313 | the eight openings round the drum and the north-door switch |
| 315-455 | the drum's stations and wall faces with the shadow gap |
| 457-533 | door reveals, niche recesses, half-domes |
| 535-618 | archivolt, imposts, niche surround |
| 620-653 | niche sill ledge |
| 655-670 | passage shell (now Site's) |
| 672-791 | flutes, base, capital, pilaster |
| 793-815 | the door-flank rule for pilasters |
| 817-894 | dome points, coffer elevations, entablature profile, plain bands and eye curb |
| 896-961 | coffers |
| 963-1015 | drum top and outer shell |
| 1036-1081 | lattice, glass, node |
| 1089-1135 | assembly |
| 1166-1173 | as-built materials |

**The sun clock:** `windows/Source/MuseeVision/SunClock/`

| File and lines | What |
|---|---|
| `SunClock.h:14-62` | what the mechanism is, its clearances, the retracting balustrade |
| `SunClock.h:193-206` | as-built light levels (evidence only) |
| `SunClock.cpp:17-25` | ring and inside zones |
| `SunClock.cpp:117-121` | lift easing |
| `SunClock.cpp:256-310` | lights and switching |
| `SunClock.cpp:312-404` | prompts, interlocks, reversal |
| `SunClockBuild.h:40-54` | the drum-frame datums |
| `SunClockBuild.h:87-151` | stair radii and the balustrade cam |
| `SunClockBuild.h:153-160` | luminaire positions |
| `SunClockKit.cpp:7-20` | drum door |
| `SunClockKit.cpp:58-79` | north port |
| `SunClockKit.cpp:82-138` | the stair layout and its pitch, string and soffit lines |
| `SunClockDial.cpp:19-53` | bands |
| `SunClockDial.cpp:55-84` | ring, fillets, hour lines, ticks |
| `SunClockDial.cpp:86-107` | numerals |
| `SunClockDial.cpp:109-165` | sunburst and boss |
| `SunClockDial.cpp:175-233` | ring courses and motto |
| `SunClockDrum.cpp:25-45` | drum profiles |
| `SunClockDrum.cpp:47-128` | walls and door |
| `SunClockDrum.cpp:160-201` | door dressing |
| `SunClockDrum.cpp:203-255` | pilasters |
| `SunClockDrum.cpp:257-346` | soffit coffers |
| `SunClockDrum.cpp:348-373` | rim and pendant |
| `SunClockShaft.cpp:44-93` | shaft walls, pocket, port, tympanum, lintel |
| `SunClockShaft.cpp:217-291` | port dressings |
| `SunClockShaft.cpp:293-337` | landing and rosette |
| `SunClockStair.cpp:59-93` | treads and nosings |
| `SunClockStair.cpp:95-145` | string, soffit, (ramp: not ported) |
| `SunClockRails.cpp:33-36` | balustrade registers |
| `SunClockRails.cpp:105-222` | uprights, rings, handrail, balustrade |
| `SunClockRails.cpp:224-370` | housings and retracting panels |
| `SunClockRails.cpp:430-517` | assembly: split, newel, wall rail |
| `SunClockChecks.cpp:101-526` | the as-built clearance, step and overlap checks at several lifts (evidence to reuse, D-017) |
| `SunClockGlyphs.h`, `bake_glyphs.py:1-40` | the letter outlines and their source font |

**The sky:** `windows/Source/MuseeVision/Sky/MuseeSky.cpp:109-116` (the idealised sun's direction) and `:133-138`
(the 06:00–18:00 window). `Sky/MuseeSky.h:35-43` describes it.

**Thresholds** (now Site's): `windows/Source/MuseeVision/Furniture/MuseeFurniture.h:29-32` and
`MuseeFurniture.cpp:301-311`.

**The exterior dress** (polish): `windows/Source/MuseeVision/Facade/MuseeFacadeStructure.cpp:600-673` (drum),
`:702-744` (dome cover) and `:781-786` (drum floodlights).

**Neighbours at the ports:**
- `windows/Source/MuseeVision/Salon/SalonStructure.cpp:27,41,1279-1282` (Bay 1's door, 3.0 × springing 3.3);
- `Chenghuai/ChenghuaiPlan.h:20-38` (the north porch);
- `Albion/AlbionPlan.h:95-101` (the south porch);
- `HallOfLight/HallOfLightStructure.cpp:48-83` (the hall into the drum).

**Materials** (seeds, D-017):
- `windows/Scripts/materials.py:1063-1072` (M_Marble_Wall), `:1107-1112` (plaster coffer and moulding), `:1122`
  (M_Gilt_Aged), `:1129-1145` (sun-clock marbles and drum), `:1216-1240` (the LANGUAGE rows for the Rotunda's
  stones, white and gilt);
- `windows/Scripts/setup_project.py:548-567` (M_RibSteel #1E1C19, M_Gilt);
- `windows/Scripts/native.py:29-48,89-90`.

**Casts:** `windows/Scripts/classical_art.py:262-279` (the niche hang), `:109-140` (scaling scans to height);
`assets/classical/SOURCES.json` (kind `rotunda`), `assets/classical/CREDITS.md`, `data/classical.json`.

**Design evidence:**
- `plan/canvas/Rotunda.dc.html` (plan SVG at 15 px/m: meridian, doors, niches, lattice; section; long section);
- `plan/boards/Rotunda.png`, `MaterialsStone.png`, `MaterialsOther.png`;
- `plan/README.md` ("The plan in words").

---

## 12. First-pass cut (tonight, D-016)

**Tonight: the room reads whole from the gilt sun, and the stair works.**

1. **Site.** Frame, footprint, the five ports with their classes and the served well, the arrival point, and the two
   enclosures and all openings (§1, §2). This comes first: it is what the neighbours mate to.
2. **Shell and order.**
   - The drum with its 4 rough openings, 4 niches (recess, half-dome, sliver reveal, surround, sill ledge) and the
     shadow gap.
   - The drum top and exterior face (plain travertine).
   - The 16 fluted pilasters with base and capital, the entablature ring, and the 4 archivolts.
   - The dome: bands, 5 × 28 coffers, eye curb, outer shell.
3. **The eye.** Ring, 21 bars, the laminated pane, the gilt node.
4. **Floor and dial.**
   - Ring courses in two tones, the 7 dial bands, the bronze hour ring and fillets.
   - 13 hour lines, the gilt meridian, the minute ticks.
   - The rays and the gilt boss (plain beads).
   - Numerals and motto, only if the glyph licence (below) is cleared tonight. Otherwise they wait, and the hour
     lines alone read the hour.
5. **The sun clock.**
   - The drum body: wall, base and entablature profiles, 12 pilasters, the door with its archivolt (keystone and
     imposts may wait), the soffit coffers, rim and pendant.
   - The lift, its S-curve, the guard, the prompts and the channel.
   - The shaft: sleeve, pocket, masonry, port with lintel and tympanum, and the eight ram housings with plain rams.
   - The landing with its rosette.
   - The helical stair: treads, nosings, string, soffit.
   - The balustrade: the fixed lower part with standards, bars, balusters and handrail; the retracting panels in
     their housings on the cam; the wall handrail; the newel.
6. **Light.** Daylight through the eye; the cornice ledge mounts; the 12 + 1 drum downlights on the channel; the
   targets and the design order declared, with commissioning at the reference moments.
7. **Art.** The four casts hung.
8. **Sound.** Acoustics for both enclosures, and footfalls.

**If Form's Laid is not ready tonight,** the cream-marble and statuary surfaces are made as solids of their visible
substance. The resulting Form.WithinStock findings are listed, not waived, and polish lays them over their cores.

**Waits for polish:**
- the exterior dress (§3.10): podium and gravel strip, skin, Ionic order, blind windows, attic, dome cover,
  floodlights;
- the balustrade's rings and bosses (plain balusters full height tonight);
- the drum door's keystone and imposts;
- the stair-side port dressings;
- the flames' weave and the boss's beaded foot;
- the drum's steel carcass;
- the lift's voice and rain on the eye;
- falls for the eye's glass;
- path-traced daylight coefficients (tier 2).

---

## Risks and open points

**The meridian is the one geometric delta from the as-built code.** It is a 60 mm gilt XII line, from the board.
If the owner prefers the as-built, it is a single `Finishes.json` line plus a width back to 24 mm.

**The glyph licence.** The as-built letters are Centaur (Monotype) outlines baked from the Windows font. Studio's
evidence import will fail its licence check. An open-licence Trajan-like face (for example Cinzel, under the OFL)
would need downloading and re-baking, which the owner must approve.

**Missing substances.** Matter has no record yet for `statuary-marble`, `rosso-antico` or `nero-marquina`.
- They are shared with the Classical Hall (rosso antico, statuary) and needed tonight.
- The cream-marble record lists only thin-slab stock (1–3 cm). Solid pieces need quarry-block stock.

**Class mismatches at the ports.**
- `salon.east` springs at 3.30, not 4.50.
- Chenghuai's porch is 3.80 wide against a 4.00 door.
- `PortClasses.json` and the joint must resolve both before Site.PortsMate can pass.

**Imposts.** They move into the doorway class detail (Site). If Site doesn't carry them, the archivolts spring from a
plain lining.

**Blondel.** It holds exactly only at r 3.61 (2R + G = 0.63). At mid-span it is 0.70. Form.StepsFitTheBody must
evaluate on the declared walking line.

**Ponding on the eye.** The eye's flat glass lies 60 mm below its curb, so rain will pond on it (Form.Ponding, Matter).
That is physically right for the as-built design, and a candidate for a fall in polish.

**The cove's colour temperature.** The catalogue has no `linear-cove.4000k`. If it stays so, the dome's night wash is
3000 K against the 4000 K stair, or it waits.

**Drum mass and the rams.** With a solid stone core the drum weighs about 200 t, and the eight rams are sized for it.
The real carcass is polish.

**Reachability and the lift.** Site.Reachable must read the lift's interlock and guard as part of the state graph
(D-014). Otherwise the lowered configuration looks like a trap for the Classical Hall. The guard makes that state
unreachable. As built, there is no raise control at the landing.

**Luminaires inside the travel envelope.** The 13 drum luminaires ride on a moving body. Light and Motion must both
accept a luminaire registered on a moving body key.

**The 0.07 ledge at the drum's door.** When raised, the stair wall's 6 cm-wide top at −0.07 lies between the sill
(0) and the top landing (−0.15). Form.StepsFitTheBody may flag it.

**Stations and trims.** The coffers, archivolts and pilasters meet by trimming rather than by the as-built 10 mm
sinks. Form needs a trim operation for a sweep ending on another piece's face.
