# The Nymphéas Oval · design sheet

- **Stage key** `nympheas` (module `MuseeNympheas`). Display name "The Nymphéas Oval". First room built (D-019).
- **Sources:** D-001…D-019 (D-019 binding); the approved `plan/proposals/salon-interior` boards (Oval primary; Plan, Light,
  Details, Main); Palette.json + Story.md (draft palette); the Salon draft `r-specs/…/salon.md` (its oval parts, verified);
  the as-built code in `windows/` (§11). Schedules: `Data/Stages/nympheas/{Finishes,Lighting,Hang,Planting,Mechanisms}.json`, to the stage contract (`briefs/STAGE-CONTRACT.md`).
- **Units** metres. **Frame:** origin at the oval's centre on the finished floor; x east, y south, z up, true north.
  Angles in degrees. **pond-z** = height above the pond's base; pond-z = z when the pond is Down.

## 1. Identity and placement

| | |
|---|---|
| Origin in the museum plan | (−86.500, 0.000, 0.000); yaw 0°. museum = local + (−86.50, 0, 0). |
| Level | 0 (ground floor). The well descends to z −5.80 (the Reserve's floor level). |
| Footprint kind | Building; plus the served well below grade (§2). |
| Vertical extent | Room: z −0.40 (floor slab over the well) to 7.80 (glass dome crown). Well prism: z −6.15 to 0.00. |

**Footprint polygon** (plan, clockwise from the passage block's south-east corner): the oval's outer face is the 0.60
normal offset of the inner ellipse a 11.000 × b 7.500 (not an ellipse); sampled every 15° of the ellipse parameter:
(11.900, 2.100) (11.207, 2.100) (9.984, 4.138) (8.116, 5.799) (5.720, 7.053) (2.955, 7.835) (0.000, 8.100) (−2.955, 7.835)
(−5.720, 7.053) (−8.116, 5.799) (−9.984, 4.138) (−11.184, 2.161) (−11.600, 0.000) (−11.184, −2.161) (−9.984, −4.138)
(−8.116, −5.799) (−5.720, −7.053) (−2.955, −7.835) (0.000, −8.100) (2.955, −7.835) (5.720, −7.053) (8.116, −5.799)
(9.984, −4.138) (11.184, −2.161) (11.207, −2.100) (11.900, −2.100). Build the curve itself at 720 stations.

**Well prism** (below the footprint): x −4.300…+11.900, y ±1.600, z −6.150…0.000.

| Enclosure | Kind | Bounded by |
|---|---|---|
| `nympheas.oval` | Enclosed | The oval wall's inner face, the floor, the velarium; includes the passage from the port face x +11.90, the cove, the attic between the velarium and the glass dome, and the pond in every state. Its only opening to the sky is Glazed: `nympheas.roof` (the dome). |
| `nympheas.well` | Enclosed | The stair shaft: brick side walls, west end wall, head wall, soffit, the stair. Opens up into `nympheas.oval` through the pond opening (closed by the pond when Down) and east through port `nympheas.stair`. |

## 2. Ports

| Key | Kind | Class | Local frame (threshold centre on the outer face; facing out; up +z) | Mates with | Wells |
|---|---|---|---|---|---|
| `nympheas.east` | doorway | `doorway.3000x4800.arched` | (+11.900, 0.000, 0.000), facing +x | `salon.west` (Bay 5 far wall, outer face museum x −74.60). Passage 0.00 (faces touch). | none |
| `nympheas.stair` | stair (served well) | `stairwell.straight.2400x3200.arched` | (+11.900, 0.000, −5.800), facing +x | `reserve.stair` (the Reserve's arched west door, outer face museum x −74.60). Passage 0.00. | **serving**: the well prism above |

- `doorway.3000x4800.arched`: clear 3.000; semicircular head r 1.500 springing 3.300; crown 4.800 (as built: the
  Salon's door and the oval passage, SalonStructure.cpp:41-42, 1291-1297). Same class as the Salon draft's east door.
- `stairwell.straight.2400x3200.arched`: clear 2.400; semicircular head r 1.200 springing 2.000 above the landing;
  crown 3.200 (the Reserve's west door as built: DoorHalf 1.19, springing 2.0, ReserveStructure.cpp:37-38).
- **Rough openings.** At `nympheas.east` the oval builds the tunnel through the passage block and its own wall, from the
  port face x +11.900 to the oval's inner face (x +11.000 on the axis, +10.778 at the jambs y ±1.500). At
  `nympheas.stair` it builds the class opening in its 0.40 brick head wall (x +11.500…+11.900). Nothing past a port face.
  The joint lays each threshold slab (honed travertine) and bronze strip on the mating plane.
- **Blinds.** `nympheas.stair`: the Reserve is not built tonight → **the Reserve's closed bronze doors**, a closed pair
  in the class opening on the port face (`nympheas.stair.blind-doors`). `nympheas.east`: on the stand alone, closed
  bronze doors in the class opening (`nympheas.east.blind-doors`) until `salon.west` is mated.
- **The well (served).** Clear void x −3.900…+11.900, y ±1.200, z −5.800…0.000; soffit at z −0.400 east of the slab edge
  x +3.600. Top opening in the oval floor x −3.900…+3.600, y ±1.200, of which x −3.900…−3.500 is the stair's flush top
  landing; the pond is its lid. `nympheas` owns everything in the prism; the receiving side builds nothing west of x +11.900.

### Anchors (the stage contract)
All frames are in the stage frame with the pond Down. Anchors on `nympheas.basin` ride it: add 2.600 to z when it is Up.

| Key | Kind | Frame | Belongs to |
|---|---|---|---|
| `nympheas.basin` | Body | origin (0, 0, 0), axes of the stage; moves +z 0 → 2.600 | coping, frieze, lip, vessel, tray, the golden iris, the water, the planting and the 32 rim lengths (§6) |
| `nympheas.guard-rails` | Body | origin (0, 0, 0), axes of the stage; moves +z 0 → 1.000 | the three rails (§3.6) |
| `nympheas.post.stage-1` … `stage-3` | Body | origin (0, 0, 0), axes of the stage; move +z 0 → 0.867 / 1.733 / 2.600 | the four posts' telescopic stages (§3.5, M-3) |
| `nympheas.golden-iris` | Use | (−4.150, 0.000, 0.420), on the coping's seat, facing +x; usable from within 6.50 of the centre | on `nympheas.basin` |
| `nympheas.golden-iris` | Bed | the same frame: Nature's cast specimen (Planting.json `cast`: *Iris laevigata* grown, cast in oil-gilt `bronze.cusn10`, handled) | on `nympheas.basin` |
| `nympheas.pond.water` | Water | (0, 0, 0.380): the water level at the centre | on `nympheas.basin` |
| `nympheas.pond.lilies` | Bed | (0, 0, 0.380): the water's surface, inside the edge, clear of the shelves (\|x\| ≤ 3.12) | on `nympheas.basin` |
| `nympheas.pond.shelf.east` | Bed | (+3.500, 0, 0.260): the east shelf, x +3.12…+3.90 | on `nympheas.basin` |
| `nympheas.pond.shelf.west` | Bed | (−3.500, 0, 0.260): the west shelf | on `nympheas.basin` |
| `nympheas.wall.willows` | Wall | origin on the inner face at the east door's north jamb (10.778, −1.500, 0.000); x = arc length along the inner face, north side first, to s 55.597 at the south jamb; path (Hang.json `walls`): ellipse a 11.000 × b 7.500, parametric θ from +x towards +y, from −11.537°, direction "decreasing" (counter-clockwise on the north-up plan); y out of the wall into the room; z up from the floor | the four willows (§8) |
| `nympheas.bench.b1` … `b4` | Seat | seat top z 0.450, at each centreline's midpoint: B1 (−3.98, 2.99), B2 (3.98, 2.99), B3 (3.98, −2.99), B4 (−3.98, −2.99); facing out to the willows along the wall's normal: B1 (−0.53, 0.85), B2 (0.53, 0.85), B3 (0.53, −0.85), B4 (−0.53, −0.85); seat frames every 0.70 along each curve | the benches (§3.8) |
| `nympheas.coping.seat` | Seat | the coping's seat ring, z 0.420 at d 0.25 in from its outer edge, frames every 0.80, facing out or in; none within 0.40 of the golden iris | on `nympheas.basin` |

Luminaires mount at the points given in Lighting.json. The rim lengths carry `body: nympheas.basin`. **Channels:**
`nympheas.pond.rim` (the rim's reveal, 0…1) and `nympheas.pond.height` (the pond's height ÷ 2.600, 0…1; the lift `publishes` it and the rails follow it), both
published by Motion.

## 3. Pieces by trade

### 3.1 Walls and masonry
- **Oval wall:** inner face the ellipse a 11.000 (x) × b 7.500 (y) about (0, 0); outer face its 0.600 normal offset
  (x ±11.600 on the axis, y ±8.100). Plaster `nympheas.wall.plaster` from the floor to 5.000, then the cove. Outer face
  `nympheas.exterior.wall` rises to the roof ring at 7.200. Inner wall length from the door's north jamb, round by the
  west, to its south jamb: **55.597**; full inner perimeter 58.641.
- **Door:** at the east apex, 3.000 wide (jambs y ±1.500 on the inner face at x +10.778), semicircular head springing
  3.300, crown 4.800, 64 stations per semicircle.
- **Passage:** tunnel 3.000 wide with the same arched soffit, from x +11.900 to the inner face; reveal
  `nympheas.passage.reveal`. Floor `nympheas.floor` continues through it.
- **Passage block:** masonry filling |y| ≤ 2.100 between the port face x +11.900 and the oval's outer face (x +11.207 at
  |y| 2.100, +11.600 on the axis), floor to a flat top at 5.200; sides and top `nympheas.exterior.wall`.
- The Facade's dress round the oval (Ionic order D 0.484, hyphen wall |y| 5.5) is polish, not tonight.

### 3.2 Mouldings (profiles: A in from the wall face / out from the opening, B up; metres)
- **P6 slim architrave** `nympheas.door.architrave` (`architrave.two-fascia.160`), swept up the jambs and over the arch
  on the oval face, following the curved wall; top 4.960 (under the cove): (0.160, 0.000) (0.160, 0.075) (0.145, 0.075)
  arc(c 0.145, 0.058; r 0.017; 90→180°; 6) (0.100, 0.058) (0.100, 0.052) (0.020, 0.046) (0.000, 0.030) (0.000, 0.000).
- **P9 oval cove** `nympheas.cove` (`cove.quarter-ellipse.1450`), swept on the wall's own 720 stations, continuous over
  the door: (0.000, 5.000) (0.030, 5.000) (0.030, 5.050) (0.015, 5.070) arc(c 1.300, 5.070; radii 1.285 × 1.130;
  180→90°; 32) (1.450, 6.200) (1.450, 6.350). Its last point is the velarium's rim.
- **P10 skirting** `nympheas.wall.skirting` (`skirting.flat.100`): (0.018, 0.000) (0.018, 0.090) (0.008, 0.100)
  (0.000, 0.100); from y ±1.630 either side of the door round the room.

### 3.3 Ceiling: velarium, attic, glass dome, roof
- **Velarium** `nympheas.velarium`: linen stretched on the cove's rim (the 1.450 inward parallel of the wall, ≈ ellipse
  9.550 × 6.050 at z 6.350), rising to **6.850** at the centre: z = 6.350 + 0.500 (1 − ρ²), ρ the elliptic radius
  fraction. Widths 1.40 along x, double-stitched seams at y = ±0.70, ±2.10, ±3.50, ±4.90.
- **Velarium frame** `nympheas.velarium.frame`: rim ring (bronze angle 60 × 60 × 6) on the cove's top at the rim line;
  eight T-ribs 40 × 40 × 5 along x above the seam lines, following the cloth's rise, ends on the ring; hung from the dome
  bars by Ø 6 rods at 1.20. The cloth is laced to the ring and ribs from above.
- **Attic upstand** `nympheas.attic.upstand`: 0.10 thick at the rim line, z 6.350…7.200, attic face white.
- **Glass dome** `nympheas.roof.glass` (Glazed opening `nympheas.roof`): within the rim line, z = 7.200 + 0.600 (1 − ρ²),
  crown **7.800**. Bars `nympheas.roof.bars` 40 wide × 70 deep, along x at y = ±0.70, ±2.10, ±3.50, ±4.90 (over the
  seams) and along y at x = 0, ±1.20 … ±8.40; a bronze curb on the upstand. Under the glass, a **fixed louvre** (daylight
  control, catalogue gap §13); not a blind (D-014). The attic is 0.85 deep at the rim, 0.95 at the centre.
- **Roof ring** `nympheas.exterior.roof`: slab 7.000…7.200 from the outer face in to the rim line; the void behind the
  cove is closed and unseen.

### 3.4 Floors
- `nympheas.floor`: 30 mm honed travertine on its bed, finished level z 0.000, over the whole oval and the passage to
  x +11.900. Courses 0.60 wide along x, symmetric about the axis (joints at y = ±0.30 + 0.60k), random lengths
  0.90–1.50, 2 mm joints, cut to the skirting, the curb and the viewing stones. Over the well: a 0.40 slab (soffit
  z −0.400) east of x +3.600; elsewhere on grade.
- **Viewing stones** `nympheas.floor.viewing-stone` (disc r 0.400) in rings `nympheas.floor.viewing-stone.ring`
  (r 0.400–0.450), flush, at **(+7.200, 0.000)** (on the slab over the well) and **(−7.200, 0.000)**, beside the
  ellipse's foci (±8.047, 0). Each needs 0.60 clear round its centre.

### 3.5 The pond (lifting body `nympheas.pond`)
**Geometry.** Centre (0, 0); the base sits on the floor at rest. Water's edge: ellipse **a 3.900 × b 2.000**
(perimeter 19.019); water surface pond-z **0.380**. Everything is built on arc-length parallels of the water's edge,
U(t, d) = s(t) + d·θ(t) (d out from the edge, θ the tangent's turning), so stations stay radial.

**Coping** `nympheas.pond.coping`, sitting height 0.420, width 0.440:
- 16 blocks, block k centred at U = k · 21.438/16 on the die parallel (1.340 there; 1.189 at the water's edge, 1.361 at
  the outer face); block 0 centred on the east apex, block 8 on the west. Joints radial: 3 mm mortar 3 mm deep, arrises
  chamfered 4.5 mm; each block its own slice, tone and polish, lippage ±0.25 mm.
- Upper profile (d, pond-z): (0.000, 0.300) (0.000, 0.398) arc(c 0.012, 0.398; r 0.012; 180→90°; 4) (0.050, 0.4194)
  (0.120, 0.4200) (0.385, 0.4200) arc(c 0.385, 0.365; r 0.055; 90→−60°; 20) [torus, face d 0.440] (0.4125, 0.300)
  arc(c 0.4125, 0.2725; r 0.0275; 90→180°; 8) ending (0.385, 0.2725). Seat flat d 0.120…0.385 at 0.420.
- Lower profile: (0.385, 0.075) (0.425, 0.075) arc(c 0.425, 0.060; r 0.015; 90→0°; 6) (0.440, 0.018) (0.425, 0.018)
  (0.425, −0.002): ledge, ovolo, plinth, toe recess.
- Outline: outer face d 0.440 (≈ 4.34 × 2.44 semi-axes; **8.68 × 4.88 overall**); toe d 0.425 at the floor.

**Carved lily frieze** `nympheas.pond.frieze` (Form Ornament on the die, Art Nouveau after Gallé and Majorelle): the
band d 0.385, pond-z 0.075…0.2725 (0.1975 tall). Height field on a 2.5 mm grid, relief 40–55 mm proud, edges cut
square and undercut 3 mm, fading to 0 within 4 mm of the band's top and foot. Along each half from the east keystone
(u 0) to the west (u 10.719): a keystone motif 0.36 long at each apex (a lily from the front 0.19 across, rings of
12/10/8 petals and a stamen crown; a pad 0.25 across and a bud either side); between them 10 repeats of 1.000: a
whiplash rhizome (two tangent-continuous cubics, low at the repeat's ends), two overlapping pads 0.21–0.25 across and a
third lifted on its stalk, an open lily 0.13–0.15 across on its own stem (every other one half-open), a bud high on the
rhizome. North and south halves mirror. Design functions: SalonPond.cpp:168-441.

**Lip** `nympheas.pond.lip`, one jointless ring (a 10 mm band): (−0.010, 0.355) (−0.010, 0.3840) (−0.005, 0.3840)
(−0.005, 0.3970) (−0.010, 0.3970) (−0.010, 0.403) arc(c 0.0015, 0.403; r 0.0115; 180→0°; 10) (0.013, 0.394). Bead top
0.4145, 34 mm over the brimming water. The slot 13 mm tall × 5 mm deep (pond-z 0.3840–0.3970, 4 mm over the water)
houses the line of light (§4), its face flush at d −0.010.

**Vessel.** `nympheas.pond.bed`: lining on the parallel d −0.003 from pond-z 0.058 to 0.358; bed at 0.060; two
planting shelves at the ends, |x| ≥ 3.200, tops at 0.260 (0.12 under the water), vertical risers at |x| = 3.200.
`nympheas.pond.tray`: bronze under everything out to the toe (d 0.425), underside pond-z −0.002 (12 mm tray + 50 mm
bluestone bed). Water `nympheas.pond.water` (§7).

**Posts** `nympheas.pond.post`: four at **(±3.000, ±1.350)**. Telescopic, three stages Ø 0.210 (lowest) / 0.180 /
0.150, each 0.95 long, overlaps 0.08; collapsed inside sleeves Ø 0.230 × 1.00 deep in the shaft walls and curb
(y 1.235…1.465, ≥ 35 mm of brick to the shaft face), heads flush with the floor under the tray at rest. Each stage
(all four posts' stage 1, 2, 3) rides its own body `nympheas.post.stage-1…3`, moved by M-3 (§6).

**The golden iris** `nympheas.golden-iris` (the pond's control; replaces the golden lily; Nature grows it as a cast specimen, Planting.json, and Motion makes it the control, the Use anchor): a cast gilt-bronze
*Iris laevigata* at life size, set on the coping's seat at the **west apex, (−4.150, 0.000), pond-z 0.420** (the head of
the axis and of the stair; over the west keystone). A boss Ø 0.08 × 0.012 let flush into the seat; a stem Ø 8 mm rising
0.34; one open flower (three falls 0.072 × 0.042 with the signal line raised, three standards 0.050 × 0.018, three
style arms 0.034, a two-bract spathe); three sword leaves 0.40–0.50 tall, 12–18 mm wide, fanned across the axis (their
flat faces to a visitor on the axis). Top ≈ pond-z 0.86. It emits nothing; the lip's line is the signal.

**Mass (for Motion):** water 7.41 m³ ≈ 7.4 t; coping and frieze ≈ 3.3 m³ travertine ≈ 8.1 t; bluestone bed, lining and
shelves ≈ 1.6 m³ ≈ 4.5 t; bronze tray, lip, iris ≈ 2 t. **≈ 22 t**, ≈ 5.5 t per post.

### 3.6 Guard rails round the opening (the retracting body `nympheas.guard-rails`)
`nympheas.guard.rail`: three rails housed in chases in the curb when down: north and south on y = ∓1.275 (chases
y 1.250–1.300, x −3.500…+3.600, stopped ±0.13 either side of the posts at x ±3.000), east on x +3.675 (chase
x 3.650–3.700, y −1.300…+1.300, in the slab, with a closed bronze housing 0.10 × 0.65 below the soffit, its own
surface `nympheas.guard.housing`). At full
height: top rail Ø 40 at 1.000, mid-rail Ø 25 at 0.500, stanchions Ø 25 at ≤ 1.10. Chase depth 1.05.

### 3.7 The stair and the well
- **Flight:** 36 risers of 0.16111 (5.80/36), goings 0.3078, running east; pitch 27.63°, 2R + T = 0.630 (Blondel's
  comfortable pace; see Departures). Riser k (k = 0…35) at x_k = −3.5000 + 0.3078k from z −0.16111k to −0.16111(k+1);
  the last at x +7.2730. Treads
  `nympheas.stair.tread`, one stone each, running 2 mm into the strings; nosing 25 mm proud, 40 mm deep, 12 mm round
  on top, 12 mm undercut; treads, risers and nosings share their profile's stations.
- **Top landing:** x −3.900…−3.500 at z 0.000, flush with the floor, wall to wall. **Bottom landing:** x +7.2730…+11.900
  at z −5.800, between the strings, then wall to wall through the head wall's opening to the port face.
- **Strings** `nympheas.stair.string`: face y ±1.175 (25 mm proud of the brick); top = nosing line + 0.24 (capped at
  z −0.06 under the top landing's nosing, to x −2.869), bottom = nosing line − 0.35; on the bottom landing a 0.15
  skirting to the port face.
- **Curb** `nympheas.well.curb`: travertine 0.30 deep (z −0.30…0) on the wall tops along the opening's north and south
  edges (y 1.20…1.60) and as the slab edge at x +3.600; carries the guard chases and post sleeves.
- **Walls** `nympheas.well.wall`: brick side walls, inner faces y ±1.200, outer ±1.600, z −5.850 up to the curb (and to
  the soffit east of x +3.600); west end wall x −4.300…−3.900; **head wall** x +11.500…+11.900, z −5.850…−0.400, with the
  class opening. Foundation underside z −6.150.
- **Soffit** `nympheas.well.soffit`: z −0.400, x +3.600…+11.500, y ±1.200.
- **Handrails** `nympheas.stair.handrail`, both walls: Ø 50 with a flat underside (the chord 240°–300°, 25 mm wide) that
  carries the LED strip (§4); axis y ±1.115, 0.90 over the nosing line. From x −1.502 (axis z −0.146), out of the wall
  with a r 0.06 return, down the flight, a r 0.15 knee at x +7.5808 (one going past the last riser) onto the landing
  (axis z −4.90), level to x +11.200,
  returned into the wall r 0.06. Brackets evenly ≈ 1.30: wall rose Ø 60 × 8, arm Ø 16, 75 mm drop.

### 3.8 Fittings: four curved benches
Centrelines (Catmull-Rom, 8 subdivisions per span, length 4.418 each), 1.26 clear of the coping's face:
- **B1 (SW)** (−5.54, 1.39) (−5.29, 1.81) (−5.00, 2.17) (−4.68, 2.48) (−4.34, 2.76) (−3.98, 2.99) (−3.60, 3.20)
  (−3.21, 3.38) (−2.81, 3.53) (−2.40, 3.66) (−1.98, 3.76)
- **B2 (SE)** (1.98, 3.76) (2.40, 3.66) (2.81, 3.53) (3.21, 3.38) (3.60, 3.20) (3.98, 2.99) (4.34, 2.76) (4.68, 2.48)
  (5.00, 2.17) (5.29, 1.81) (5.54, 1.39)
- **B3 (NE)** and **B4 (NW)** mirror B2 and B1 in y.
- Seat `nympheas.bench.seat`: oak slab 0.55 × 0.10 (z 0.35…0.45), every arris eased r 10, grain along the arc.
  Plinths `nympheas.bench.plinth`: two per bench, 0.45 long, 0.30 in from the ends, set back 0.06 from the slab's faces
  (0.43 wide), top 0.33; a 20 mm shadow gap (a neck set in 30 mm) up into the slab; toe recess 12 high × 8 in;
  arrises eased 6 mm.

### 3.9 Blinds
`nympheas.stair.blind-doors` and `nympheas.east.blind-doors`: closed pairs of cast bronze door leaves filling each class
opening on its port face, meeting rail on the axis (§2).

## 4. Light

**Daylight**

| Opening | Glazing | Weather | Control | Below it |
|---|---|---|---|---|
| `nympheas.roof` (dome within the rim line) | `glass.low-iron`, laminated UV-filtering, acid-etched inner face | **giverny** | fixed louvre (catalogue gap) | the attic, then the linen velarium |

**Luminaires** (Lighting.json; mounts at rest, pond-z for the rim; no output values are set here)

| Id | Type | Count / extent | Mount and aim | Operation |
|---|---|---|---|---|
| `velarium.row.*` | `linear-slot.3000k` (opal-cover battens, surface-hung on the dome bars) | 7 rows along x at y 0, ±1.5, ±3.0, ±4.5; lengths 18.50 / 17.86 / 15.78 / 11.52 (≈ 109 m), stepped in 1.0–2.0 m battens 0.30 above the cloth | row midpoint (0, y, cloth + 0.30); aim straight down onto the cloth | presence (daylight-linked top-up by day, gallery light at night) |
| `pond.rim.01…32` | `linear-cove.3000k` strip | 32 lengths of 0.594 on centres (8 mm gaps) round the water's edge | in the lip's slot, face d −0.010, pond-z 0.3905, each `along` the edge's tangent; aim 0.40 inward onto the water. Order: from the **west apex, clockwise (north first)**, back to the west apex | always, follows `nympheas.pond.rim`; body `nympheas.basin` |
| `stair.rail.north/south` | `linear-cove.3000k` strip | 2 runs of 10.215, each rail's straight run down the flight (a straight strip can't turn the knee) | the rails' flat underside, `along` the flight (0.886, 0, −0.464); aim 30° in from the flat's normal onto the treads | always |
| `stair.downlight.1/2` | `downlight.3000k.31deg`, recessed | 2 | soffit (6.30, 0, −0.40) and (10.10, 0, −0.40); aim straight down | always |

**Targets**

| Surface | Class | Lux | Source |
|---|---|---|---|
| `nympheas.wall.plaster` (measured on the willows' faces, z 0.70–2.70) | oil-and-tempera | 150–200 | Care |
| `nympheas.floor` | circulation / circulation.night | 100 / 50 | UseTargets |
| `nympheas.stair.tread` | stairs | 100 | UseTargets |

**Design ratio:** L(`nympheas.velarium`) / L(`nympheas.wall.plaster`) 1.5–4.0 at every reference moment: the linen is
the brightest surface in view. Measured, never tuned. The willows are lit by daylight through the linen alone when it
suffices; the battens top up to the care range. **Not re-expressed:** the emissive velarium, the glowing golden lily, the
rim's "halo", the rect lights standing in for the cloth.

## 5. Time and weather
- `nympheas.roof` takes **giverny**'s weather (D-005): the Light board's "Giverny, live · the oval under four skies"
  gives the velarium the sky over Monet's pond (cloud shadows cross the cloth; rain on the glass above it shows through
  as soft moving drop shadows). The sun and the hour are always the museum's (idealised path, D-003).
- The outer faces and the roof ring are exterior: **museum** weather.
- Indoor phenology for the planting (heated room), §7.

## 6. Motion

**M-1 `nympheas.pond-lift`** (the pond that lifts, body `nympheas.basin`, control `nympheas.golden-iris`; D-019)
- **Body:** coping, frieze, lip, vessel, tray, the golden iris, the planting (by key) and the water (Nature basin
  `nympheas.pond`), and the 32 rim strips.
- **Joint:** prismatic +z, travel **0.000 → 2.600**. **Drive:** four synchronised hydraulic telescopic rams, one in each
  post; no counterweight (≈ 22 t). **Profile:** 5.00 s each way, symmetric jerk-limited S-curve: a ≤ 0.70 m/s²,
  jerk ≤ 1.2 m/s³, cruise 0.791 m/s (no smoothstep).
- **States:**

| State | Height | Entered by | Rim light |
|---|---|---|---|
| Down | 0.0 | rest; end of Lowering | dark |
| Waking | 0.0 | one use of the golden iris from Down | runs round, 1.00 s, 32 lengths in order |
| Rising | 0 → 2.6 | end of Waking | lit |
| Up | 2.6 | end of Rising | lit |
| Lowering | 2.6 → 0 | a use from Up; or automatically when the visitor is on level 0 (feet z > −0.05) more than 16.0 from the pond's centre | lit; dark on landing |

- **Uses** (IMuseeUsable, click or headset trigger on the golden iris, or the prompt within 6.50 of the centre): from Down
  by a visitor on the oval floor; from Up only by a visitor on the oval floor not over the opening; none while
  Waking, Rising or Lowering. Nobody below can lower it, and it never auto-lowers while the visitor is below, so the stair
  is never shut (no stair-foot plate is needed tonight).
- **Guard (Motion, D-014):** the drive runs only while no part of the visitor's body is inside the swept volume V = the
  coping outline grown 0.05, from pond-z −0.002 at rest to z 3.70; otherwise it **holds** (never reverses, never pushes)
  and resumes when V is clear. No teleport.
- **Clearances (≥ 5 mm at every sampled phase):** outline to bench faces 1.26; at Up the tallest leaf (pond-z 1.04) is at
  z 3.64 and the golden iris at 3.46, ≥ 2.7 under the velarium's rim (6.35); tray underside at Up 2.598 over the floor and
  the top landing, ≥ 2.6 over every tread; rails at Up 1.598 under the tray. At Down the tray's toe ring bears on the floor
  and curb round the opening.
- **Housings:** four post sleeves Ø 0.230 × 1.00. **Channels:** `nympheas.pond.rim` (rim reveal 0–1: 0 → 1 over 1.00 s while Waking, as built, PondLift.cpp:209; 0 on landing Down) and `nympheas.pond.height` (0–1). Mechanisms.json: open = rim → 1 over 1.0 s, then move; close = move, then rim → 0
  and FMuseeActuation (the pump's voice).

**M-2 `nympheas.guard-rails`** (body `nympheas.guard-rails`; follows `nympheas.pond.height`, in [0.0385, 0.4231] → out [0, 1.0], linear, clamped): the three rails (§3.6), prismatic +z, travel 0 → 1.000; rail height =
clamp(h − 0.10, 0, 1.00) for pond height h. Interlock: the pond may not enter its last 0.10 of descent unless the rails
are housed.

**M-3 `nympheas.post.stage-1…3`** (the posts' telescopic stages, each its own body; each follows `nympheas.pond.height`,
prismatic +z, linear, clamped, no guard of its own: the pond's guard sweeps the posts' columns). A single-acting
telescopic ram extends its largest stage first, since the pump's pressure lifts the load soonest on the largest area;
each stage strokes a third of the travel (0.867 of its 0.95, keeping 0.083 ≥ the 0.08 overlap):

| Mechanism | Stage | Follows in → out |
|---|---|---|
| `nympheas.post.stage-1` | Ø 0.210, lowest | [0, 0.3333] → [0, 0.8667] |
| `nympheas.post.stage-2` | Ø 0.180 | [0, 0.6667] → [0, 1.7333] |
| `nympheas.post.stage-3` | Ø 0.150, the ram under the tray | [0, 1] → [0, 2.600] |

## 7. Nature
- **Water:** `nympheas.pond` (Planting.json `water`; anchor `nympheas.pond.water`; body `nympheas.basin`; ellipse a 3.90 × b 2.00, depth 0.32, freeboard 0.023, `water.still`), volume **7.41 m³** (24.50 m² × 0.32, less 2 × 1.088 m² of shelf × 0.20);
  surface pond-z 0.380, brimming 23 mm under the lip's top face; conserved in every state; still, ripples ≤ 3.5 mm; dyed
  black (the light gone within centimetres: a mirror).
- **Water lilies** (Planting.json `pond.lilies.*`), colonies clustered as at Giverny; (x, y, reach):

| # | Position | Reach | Flower | Species |
|---|---|---|---|---|
| L1 | (+2.90, −0.60) | 0.45 | pink | 'Carnea' |
| L2 | (+1.90, +0.90) | 0.38 | — | 'Albida' |
| L3 | (+0.60, −1.20) | 0.42 | — | 'Carnea' |
| L4 | (−0.40, +0.50) | 0.50 | white | 'Albida' |
| L5 | (−1.60, −0.70) | 0.36 | — | 'Albida' |
| L6 | (−2.60, +0.80) | 0.44 | pink | 'Carnea' |
| L7 | (−2.85, −0.45) | 0.30 | — | 'Albida' |
| L8 | (+2.75, +0.72) | 0.30 | — | 'Carnea' |
| L9 | (+0.90, +1.40) | 0.30 | white | 'Albida' |
| L10 | (−1.10, +1.50) | 0.28 | — | 'Carnea' |

  Each colony: round(2 + 8 × reach) extra pads (radius 0.55–1.0 × the main pad's 0.45 × reach, clamped 0.10–0.22) out
  to its reach; flowers 0.085–0.115 on the water; 60 % carry a bud 0.22–0.40 away. Pads kept inside the water's edge and
  clear of the shelves (|x| ≤ 3.12).
- **Marginals, only at the two ends of the axis** (feet on the shelves, pond-z 0.26): *Iris laevigata* clumps at
  (±3.50, −0.60) and (±3.47, +0.62) (flowers: east 2 and 3, west 2 and 2); *Carex elata* tufts at (+3.74, +0.04),
  (−3.74, −0.04), (±3.33, −0.92), (±3.33, +0.93); *Myosotis scorpioides* patches at (±3.30, 0.00) r 0.14,
  (±3.62, −0.25) r 0.08, (±3.60, +0.30) r 0.08. The axis between them stays open. Growth space: over the shelves,
  inside the lip, under pond-z 1.10. Counts, sizes: Planting.json.
- **Indoor phenology:** Nymphaea flower Lixia → Hanlu, pads thinned through winter; irises Mangzhong → Xiazhi;
  forget-me-nots Lixia → Dashu; sedge evergreen.

## 8. Art
The Orangerie's second room at true size (D-019). Unframed, unvarnished oil on canvas; every canvas **4.25 × 2.00**,
bottom 0.70, top 2.70, curved to the wall on its stretcher, **35 mm proud** of the plaster; joints between a work's
canvases 1.5 mm with a 12 mm linen strip set 12 mm back. Works run from the door's north jamb (10.778, −1.500) along
the inner face (s), north side first, with five equal gaps of **0.919**. Each image reads left to right from the room.
Wall `nympheas.wall.plaster`; support canvas; frame none; care oil-and-tempera; source `assets/collection.json`
(sizes: Scripts/salon_interior.py WILLOWS and assets/salon/manifest.json).

| # | Work (id) | Title · artist | w × h | s from → to (Hang `along` = from) | Start → end (x, y) | Canvas joints (x, y) | Centre (face, z) |
|---|---|---|---|---|---|---|---|
| 1 | monet-willows-clear-morning | Clear Morning with Willows · Claude Monet | 12.75 × 2.00 (3) | 0.919 → 13.669 | (10.443, −2.355) → (−0.531, −7.491) | (7.586, −5.431) (3.685, −7.067) | (5.691, −6.380, 1.70) |
| 2 | monet-willows-two-willows | The Two Willows · Claude Monet (ahead across the axis) | 17.00 × 2.00 (4) | 14.589 → 31.589 | (−1.448, −7.435) → (−9.730, 3.498) | (−5.572, −6.467) (−9.125, −4.189) (−10.979, −0.459) | (−9.100, −4.164, 1.70) |
| 3 | monet-willows-morning | Morning with Willows · Claude Monet | 12.75 × 2.00 (3) | 32.508 → 45.258 | (−9.125, 4.189) → (2.785, 7.256) | (−5.572, 6.467) (−1.448, 7.435) | (−3.537, 7.066, 1.70) |
| 4 | monet-willows-reflections-of-trees | Reflections of Trees · Claude Monet | 8.50 × 2.00 (2) | 46.178 → 54.678 | (3.685, 7.067) → (10.443, 2.355) | (7.586, 5.431) | (7.567, 5.402, 1.70) |

Hang.json: every work on the Wall anchor `nympheas.wall.willows`, `along` = s from, `height` 0.70. Start/end/joint points are on the inner face (the canvas face is 0.035 in along the normal). The first room's four
panels are not hung. Captions on look (Art).

## 9. Sound
- **Acoustics** (from Matter, never tuned): `nympheas.oval` ≈ 1,600 m³ of hard burnished lime in an ellipse, foci at
  (±8.047, 0) beside the two viewing stones: expect focusing (a whispering gallery). `nympheas.well` ≈ 170 m³, brick
  and stone, bright; coupled to the oval while the pond is up.
- **Emitters:** rain on the dome's glass, driven by the **giverny** observer's rain, heard above the cloth; the pond's
  hydraulic power unit (soft pump hum, valve clicks) on the `nympheas.pond` body, driven by FMuseeActuation; footfalls
  by substance (honed travertine). The still water is silent.

## 10. Visitor
- **Walk:** in from `nympheas.east` on the axis; round the pond; with the pond up, from the west end onto the top landing
  and down the 36 steps to the closed bronze doors, and back. **Step-to:** the two viewing stones.
- **Sit:** the four benches (seat 0.45, frames every 0.70 along the curve, facing out to the willows); the coping
  (seat 0.42 all round, frames every 0.80 at d 0.25, facing out or in, none within 0.40 of the golden iris). The coping
  is not a step; nothing invisible guards the water.
- **Use** (the golden iris, look + click or headset trigger; prompt within 6.50):

| State | Prompt | Message |
|---|---|---|
| Down | "Lift the pond" | "A line of light runs round the rim, and the pond rises on four bronze posts…" |
| Up (on arrival) | — | "Beneath the pond, a long stair leads down to the Reserve." |
| Up | "Lower the pond" | — |
| Guard holding | — | "The pond waits until you step clear." |

- **Look:** captions on the four works. **Eye:** from Bay 5 into the white oval is an adaptation walk the Eye owns.

## 11. The old-code map
Paths relative to `windows\` unless marked. Re-express, never copy.

| Part | Where |
|---|---|
| Oval constants (centre −86.5, a 11, b 7.5, wall 0.6, band 5.0, roof 7.2, block 2.1/5.2, opening) | Source/MuseeVision/Salon/SalonStructure.cpp:67-72 |
| Door constants (3.0, springing 3.3, 64 segs) | …/SalonStructure.cpp:41-42 |
| P6 slim architrave; P9 cove; P10 skirting; bench slab | …/SalonStructure.cpp:441-450, 462-469, 471-476, 479-486 |
| Ellipse, offset outer face, BlockT, wall columns | …/SalonStructure.cpp:488-574 |
| Plan bench lines (superseded) | …/SalonStructure.cpp:576-590 |
| Oval passage tunnel and slim architrave | …/SalonStructure.cpp:1291-1297 |
| BuildOval: walls, cove, velarium (rise 0.5), skirting, outer face, roof, passage block | …/SalonStructure.cpp:1398-1508; hull :1510-1517 |
| Floor strips; floor grid; the pond opening's corners | …/SalonStructure.cpp:1533-1553, 1555-1650 |
| Viewing stones (−79.3, 0) and (−93.7, 0) | …/SalonStructure.cpp:1921-1927 |
| Velarium rim (a − 1.45, b − 1.45, 6.35); materials | …/SalonStructure.cpp:2129-2136, 1908-1916 |
| Mesh kit (FProfile, Sweep, PlanRuns, SweepPlan) | Source/MuseeVision/Salon/SalonKit.h:40, 238, 287, 360 |
| Pond constants (a 3.9, b 2.0, bed, shelves, die, band, outer, seat, toe, 16 blocks, joints, 32 rim) | Source/MuseeVision/Salon/SalonPond.cpp:19-33 |
| Arc-length parallels; block stations | …/SalonPond.cpp:58-114, 140-166 |
| Frieze design (shapes, keystone, repeat, composition) | …/SalonPond.cpp:168-441 (FFrieze :268, Keystone :358, Repeat :385, Build :408) |
| Coping upper and lower profiles | …/SalonPond.cpp:497-521 |
| IsOver / PushOut (the old push-out: **do not port**) | …/SalonPond.cpp:705-721 |
| Rim light: 32 lengths, slot, reveal order | …/SalonPond.cpp:723-755 |
| Lip; lining, bed, shelves; tray; hit block | …/SalonPond.cpp:916-924, 926-957, 958-959, 961-966 |
| Planting: IrisFlower, IrisClump, Sedge, ForgetMeNots, placement | …/SalonPond.cpp:1128, 1168, 1232, 1276, 1330-1349 |
| Pond edge and planting descriptions | Source/MuseeVision/Salon/SalonInterior.h:150-239 |
| Oval wall line; willow canvas build | Source/MuseeVision/Salon/SalonInterior.cpp:152-196, 420-476 |
| Pond lift: constants, CanUse, Use, Tick (auto-lower), Apply (smoothstep, scaled posts: **do not port**), CheckClearance (swept volume) | Source/MuseeVision/Salon/PondLift.cpp:18-26, 132-167, 176-198, 200-250, 252-275, 277-330; PondLift.h:11-24 |
| Long stair numbers | Source/MuseeVision/Plan/MuseePlan.h:346-368 |
| Facade numbers round the oval (polish) | …/MuseePlan.h:518-526, 544 |
| Stair: constants, flight, strings, shaft, rail path, downlights | Source/MuseeVision/ReserveStair/ReserveStairStructure.cpp:28-65, 129-177, 179-201, 203-234, 270-281, 61; .h:13-51 |
| Invisible guards; hidden walkway ramp (**do not port**) | …/ReserveStairStructure.cpp:65, 238-248 |
| Reserve west door (class) | Source/MuseeVision/Reserve/ReserveStructure.cpp:37-38 |
| Oval benches (moved out for the coping); slab on plinths | Source/MuseeVision/Furniture/MuseeFurniture.cpp:207-231, 79-101 |
| Lily colonies and colours | Source/MuseeVision/Nature/MuseeWaterLilies.cpp:372-447, 205-216 |
| Lily plan positions (two moved in from the shelves) | Scripts/nature.py:909-919, 1203-1224 |
| Willows list and placement; velarium material (seams); pond materials (rim 2700 K) | Scripts/salon_interior.py:49-62, 1049-1068, 766-791, 877-902 |
| Giverny weather; rain sound | Source/MuseeVision/Salon/SalonSky.h:13-28; SalonRain.h:8-39 |
| PondPlan (Swift reference; repo root) | Shared/Wings/Reserve.swift:11-32; Shared/Plan/MuseumPlan.swift:106-136 |

**Corrections to the draft** (`r-specs/…/salon.md`, its oval parts):
1. Frame: the oval's own (origin at its centre, museum −86.50), not Salon-local (subtract 72.5 from draft x).
2. Lilies L7 and L8 are at (−2.85, −0.45) and (+2.75, +0.72) as built (nature.py moved them in off the shelves); the
   draft had (−3.10, −0.40) and (+3.15, +0.50).
3. The golden lily is gone (D-019): the golden iris on the coping's west apex; L4 is an ordinary white-flowering lily;
   one use wakes the rim light and lifts (no Armed state, no 5 s timeout); the rim runs from the west apex clockwise,
   not from (west, y +0.5).
4. Handrails start at x −1.502 as built (museum −88.00), not the draft's slab edge; they end at x +11.200, 0.30 before
   the draft's head wall (as built they ended at museum −74.90, inside the head wall's thickness).
5. The draft's guard-rail line y ±1.215 lay 15 mm in the shaft void and clashed with the post sleeves and handrail
   brackets: moved to ±1.275 in chases 50 mm behind the brick face, interrupted at the posts; east rail at x +3.675.
6. The stair-foot "Raise the pond" plate is dropped: lowering is only from the oval floor and auto-lowering only happens
   with the visitor on level 0 away from the oval, so no one below is shut in.
7. The lip is a 10 mm band with a 13 × 5 mm slot to house the catalogue strip (the draft's 6 mm slot fit no product).
8. Materials are palette keys: floor honed, filled (draft: polished); walls burnished lime (draft: limewashed); velarium
   linen (draft: cotton muslin); bed and lining bluestone (draft: bronze vessel), hence mass ≈ 22 t (draft 19 t).
9. Lighting: stair lights 3000 K catalogue types (as built 3800 K); the rim strip 3000 K (as built 2700 K).
10. Verified unchanged: wall length 55.597, gaps 0.919, willow points, pond profiles, posts (±3.0, ±1.35), opening,
    stair numbers, benches, viewing stones, velarium 6.35/6.85, dome 7.2/7.8, water 7.41 m³.

## 12. The first-pass cut
**Tonight:** Site (ports, blinds, served well, footprint, enclosures); the shell (wall, door, passage, block, P6, P9,
P10, roof ring, upstand, dome with bars, velarium and frame); the floor and two viewing stones; the stair and well
(flight, landings, strings, curb, walls, head wall, soffit, handrails); the pond (coping blocks and joints, **the carved
frieze**, lip, vessel, tray, posts, golden iris), M-1 and M-2 with the guard and the channel; the water, 10 lily
colonies and the marginals; the four willows; four benches; the luminaires of §4 and daylight through the dome with
Giverny's cloud; sound; the use.

**Waits for polish:** the fixed louvre as a catalogue product (meanwhile Light sizes a generic fixed control to the care
target); each coping block's own tone and lippage beyond Matter's defaults; the waterline history; the exterior dress;
the whispering-gallery check; a per-opening cloud field so the sky seen through the dome matches Giverny.

## 13. Palette keys to confirm and catalogue gaps
**Palette keys** (no `todo.` keys; roles guessed or workings outside `workingsByEra`):
- `stone.travertine-romano` working **carved** (the frieze): not in workingsByEra → a "carved (filled block)" working.
- `gilt.gold-leaf` on `bronze.cusn10` for **the golden iris**: role guessed (roles name the lip and rings).
- `bronze.cusn10` patinated for the **tray, guard rails, glazing bars, velarium frame and blind doors**: roles guessed
  by analogy (lantern frames, posts, the Grounds' blind door).
- `accent.milled-lead` for the **oval's roof ring**: guessed (roles: Albion flats, the Rotunda's dome).
- `stone.travertine-romano` honed, weathered for the **oval's outer face**: guessed from "Grounds: façade".
- `accent.pierre-bleue` for the **lining and shelves** (role names the bed only); history "weathered" stands for
  "submerged" (to add).
- `white.lime-plaster` for the **well soffit and attic upstand**: guessed. `accent.brique-bourgogne` for the **head wall**:
  extends "the long stair's shaft".
- `water.still` working written `still.dyed-black`; `stone.travertine-romano` coping history "handled" stands for
  hand-and-seat contact plus a waterline (to add).

**Catalogue gaps** (Light): (1) **fixed daylight control** under the dome: a fixed louvre (white aluminium blades,
cut-off fixed), transmission sized so the brightest reference moment gives ≤ 200 lx on the willows (Lighting.json
control "fixed-louvre"); (2) a **control class that follows a mechanism channel** for `pond.rim.*` (written "presence");
(3) a surface **LED batten** type for the attic (the SLOTLIGHT channel is recessed; used as the nearest type).

**Questions for the Architect:** (a) "the head of the axis" read as the west apex (the far end, and the stair's head);
confirm or move the golden iris to the east apex. (b) D-019's "8.4 × 4.6 m" is the board's first-design outline; the
as-built pond-edge redesign (water 7.80 × 4.00, coping 8.68 × 4.88) is kept per D-017. (c) The Reserve draft puts
`reserve.stair` at the oval floor's opening (a 2.40 × 7.50 stairwell facing −z); this sheet puts the mating plane at the
Reserve's west door (x +11.90, z −5.80), where the stair ends at the closed bronze doors, as the brief says.

## Build notes
- **Frame.** The origin is the oval's centre on the floor, at museum (−86.50, 0, 0), yaw 0. Every number here is local.
- **The inner face is a true ellipse** (a 11.000, b 7.500). The outer face is its 0.60 normal offset, which is **not**
  an ellipse, so build it from 720 stations.
- **One moving body.** The coping, frieze, lip, vessel, tray, water, planting, golden iris and the 32 rim lengths all
  hang on `nympheas.basin`, which lifts 2.600 in 5.0 s (Mechanisms.json).
  - The guard rails are a second body, `nympheas.guard-rails`. They follow `nympheas.pond.height` and are housed in
    curb chases at y ±1.275, with the east rail at x +3.675.
- **The oval serves the well.** The 36-riser stair runs east under the floor (x −4.30 … +11.90) and ends at the
  Reserve's closed bronze doors at `nympheas.stair` (+11.90, 0, −5.80). When the pond is Down it is the well's lid.
  Build nothing of the Reserve.
- **The east passage is the oval's.** Build the passage block and tunnel from x +11.90 to the inner face. The Salon
  builds only its rough opening.
- **The frieze is in tonight's cut.** The carved Art Nouveau lily frieze on the coping's outer face is Form's
  carving test.
- **The golden iris is two anchors with one key.** It is a Use anchor, so Motion makes it the control, and a Bed
  anchor, where Nature grows *Iris laevigata* and casts it in oil-gilt CuSn10. Do not model it by hand.
- **The willows hang on one curved Wall anchor.** `along` is the arc length from the north jamb, with θ decreasing
  (Hang.json `walls`). The canvases are built curved to the wall and stand 35 mm proud.
- **Water.** The water brims 23 mm under the lip's top and is dyed black. The velarium's glass takes Giverny's
  weather, with a fixed louvre as its daylight control (a catalogue gap).

**Departures from as-built** (for the owner's review):
- The golden lily is replaced by **the golden iris**: a gilt-bronze *Iris laevigata* on the coping at the west apex (D-019). One use toggles the pond; there is no armed state and no 5 s timeout.
- **Retracting guard rails** round the open well replace the as-built invisible guards (ruled).
- The **stair-foot plate** is dropped, because the pond never lowers while the visitor is below.
- The lip carries a 10 mm band with a 13 × 5 mm slot sized to the catalogue strip. The rim light is 3000 K (as built 2700 K), and the stair lights 3000 K (as built 3800 K).
- A **fixed louvre** under the glass dome keeps the willows within their care range (no blinds, D-014).
- **The stair walks at a comfortable pace.** As built, its 36 risers of 0.16111 had goings of 0.3389: 2R + T = 0.661,
  a stride too long for the rise. The going is now 0.3078, so 2R + T = 0.630, inside Blondel's 0.60–0.64 m; the 36
  risers and the 5.80 rise are kept. The flight runs 10.773 in plan (1.089 shorter) and ends at x +7.273, inside the well;
  the bottom landing grows to match, the handrails' knee moves with the last riser (one going past it, x +7.581), and
  the stair's rail strips follow the rails' new straight run (10.215 each).
- Materials follow the Story (D-018): the floor is honed, not polished; the walls are burnished lime; the velarium is linen; the pond's bed is Belgian bluestone.
