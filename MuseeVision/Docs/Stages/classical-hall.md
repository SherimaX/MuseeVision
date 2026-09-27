# The Classical Hall · design sheet

- **Stage key** `classical-hall` (module `MuseeClassicalHall`, `UClassicalHallRoom : USiteRoom`). Display name "The
  Classical Hall". Era: Classical. Level −1 under the Rotunda: a barrel-vaulted sculpture gallery after the Vatican's
  Braccio Nuovo, and through a screen of two columns a round domed tribune after the Sala delle Muse. It holds 41
  plaster casts from SMK's Royal Cast Collection. No daylight. The only way in is the Rotunda's spiral stair, whose
  well this stage **receives**.
- **Sources:** D-001…D-019 (D-012, D-017, D-018 binding here); Palette.json and Story.md §5 and "The Classical Hall";
  the drafter's spec `r-specs/…/Stages/classical-hall.md` (verified, corrections in §11); the finished `rotunda.md`
  (§2, the stair port and served well); the as-built code in `windows/` (§11). Schedules:
  `Data/Stages/classical-hall/{Finishes,Lighting,Hang}.json` (no Planting, no Mechanisms).
- **Units** metres. **Frame:** origin O at the tribune's centre on its finished floor (the axis of the centrepiece,
  under the eye); x east, y south, z up, true north. **φ** is the plan angle from +x towards +y (east 0°, south 90°
  towards the gallery, north 270°); a compass bearing is φ + 90°. **Old numbers convert** as x = x_old,
  y = y_old + 39.30, z = z_old + 6.00.

## 1. Identity and placement

| | |
|---|---|
| Origin in the museum plan | (0.000, −39.300, −6.000); yaw 0°. museum = local + (0, −39.30, −6.00). Site derives it from the mating `rotunda.stair ↔ classical-hall.stair` (§2). |
| Level | −1. Finished floor z 0.00 everywhere (museum −6.00), flush with the joint's threshold (the as-built 1 cm step is retired). |
| Footprint kind | Building, below grade. |
| Vertical extent | z −0.40 (slab underside) to 5.45 (roof top = museum −0.55, where the stages above begin). Highest inner surface: the vault crowns, 5.15 (the roof slab's underside). Laylight glass 4.94 (§3.3). |

**Footprint polygon** (clockwise on the north-up plan, from the south-east corner; a prism z −0.40 … 5.45):
(4.200, 31.400) → (2.100, 31.400) → (2.100, 31.996) → arc r 7.600 about W = (0, 39.300), through (0.000, 31.700),
bulging north into the hall → (−2.100, 31.996) → (−2.100, 31.400) → (−4.200, 31.400) → (−4.200, 6.691) → arc r 7.900
about O the long way by the north, φ 122.12° → 417.88°, through (−6.842, 3.950) (−7.900, 0.000) (−6.842, −3.950)
(−3.950, −6.842) (0.000, −7.900) (3.950, −6.842) (6.842, −3.950) (7.900, 0.000) (6.842, 3.950) → (4.200, 6.691) →
back to (4.200, 31.400). Build the arcs at 1.5° stations (240 round).

**Site must compare volumes, not plans:** the footprint lies under the Rotunda's drum and north passage (y 31.40 to
≈ 28.1) and under Chenghuai (its pond's west lobe over the tribune, bed at museum −0.46, 0.09 above this roof).

| Enclosure | Kind | Bounded by |
|---|---|---|
| `classical-hall.interior` | Enclosed | One sealed shell: the passage, the gallery, the throat under the lintel, and the tribune (ambulatory, ring, dome, eye). ≈ 1,350 m³ (gallery ≈ 680, throat ≈ 23, tribune ≈ 645). Its only opening is Port `classical-hall.stair`. Housings: the four lantern wells and the eye well (§3.3), each holding a laylight. No Glazed, Open, Court or Outdoors. |

**Plan.** Passage: from the port face (the cylinder r 7.60 about W, y 31.70 on the axis) to the gallery's south wall,
y 31.10. Gallery: inner faces x = ±3.00, 24.00 long from y 31.10 to the screen line y 7.10, four bays of 6.00 (bay
centres y 28.10, 22.10, 16.10, 10.10; column pairs on the bay lines y 25.10, 19.10, 13.10). Throat: the gallery's side
walls continue north of the screen under a lintel (soffit 3.075) to the tribune wall at y 5.8788. Tribune: outer wall
r 6.60, a ring of 12 columns on r 3.40, ambulatory r 3.64–6.60, eye r 1.20. Shell faces: south y 31.40, sides x ±4.20,
tribune r 7.90.

## 2. Ports

The frame is on the port face at the threshold (z 0), on the opening's centreline, facing out, up +z.

| Key | Kind | Class | Local frame (position; facing) | Face; wall depth | Mates with | Wells |
|---|---|---|---|---|---|---|
| `classical-hall.stair` | stair (receiving well) | `stairwell.spiral.3600x4400.arched` | (0.000, 31.700, 0.000); +y (south, towards the well) | cylindrical R 7.60 about W (0, 39.30), convex towards the hall; 0.600 on the axis (y 31.10 → 31.70), 0.816 at the clear jambs x ±1.80 (face y 31.916) | `rotunda.stair` (the Rotunda's frame (0, −7.60, −6.00), facing −y). Passage 0.00: the faces touch on the cylinder. | **receiving**: the cylinder r 7.60 about W, z −0.40 … 6.00 (museum −6.40 … 0.00) |

- **The one port.** By D-012 the hall is a leaf: its only mating is the Rotunda's served stairwell. The as-built hall
  has no other opening (checked: `ClassicalHallGeometry.cpp` builds one door, the shell is closed elsewhere).
- `stairwell.spiral.3600x4400.arched` (the Rotunda's name; the draft wrote `stairwell.spiral.3600`): clear 3.600, jambs
  the planes x = ±1.800; semicircular head r 1.800 about the line (x 0, z 2.600) running along y, springing 2.600 above
  the threshold, crown 4.400 (`MuseePlan.h:143-157`; the hall's side `MuseePlan.h:480-481`). An open portal, no leaves.
- **Museum check.** Port frame: (0, 31.70 − 39.30, 0 − 6.00) = **(0.000, −7.600, −6.000)** = the Rotunda's `rotunda.stair`
  frame (stage = museum there); facings +y and −y are opposite. The well's axis W maps to (0, 0): the Rotunda's axis.
  The receiving cylinder r 7.60, museum z −6.40 … 0.00, is exactly the Rotunda's well prism. As built: tribune centre
  `TribuneCentreY` −39.3 and `FloorZ` −6.0 (`MuseePlan.h:503, 474`), shaft outer r 7.6 (`MuseePlan.h:141, 480`).
- **Rough opening** (all this stage builds at the port): the clear opening grown by Site's lining allowance *a*
  (0.050 proposed, as the Rotunda): jambs x ±1.850, head r 1.850 about (x 0, z 2.600), sill z −0.050, straight through
  from the south wall's face y 31.10 to the port cylinder; faces `classical-hall.port.rough-opening`. The **sleeve**
  (collar envelope, as built) carries it: x ±2.10, round head r 2.10 springing 2.60 (crown 4.70), from z −0.40, from the
  shell's south face y 31.40 to the cylinder; `classical-hall.shell`. Nothing past the port cylinder.
- **Site's joint** builds the travertine threshold through the depth, the brushed-bronze strip on the mating arc
  (r 7.60 about W), the linings and the surround on the gallery's flat south face (y 31.10). It replaces the as-built
  carved architrave, keystone, plinth blocks and porphyry sill (the one doorway, Story §4).
- **The well (received).** The Rotunda serves it: shaft masonry r 6.48–7.60, pocket, rams, stair (40 × 0.150),
  landing (museum −6.00) and rosette, and the lintel and tympanum over its portal. This stage reserves the cylinder as a
  void and builds nothing inside it; the sleeve and the footprint stop on r 7.60.
- **Blind doors.** Mated tonight: open portal. On the stand alone (or with the Rotunda absent): a closed pair of cast
  bronze leaves in the class opening on the port face, meeting rail on the axis, `classical-hall.stair.blind-doors`.

### Anchors (the stage contract)
One `Mount` per place for a work in the round (the brief's Hang rule; the Rotunda's casts use the same kind): origin
at the **carrier's top centre**, x = the way the work faces (horizontal), z up. Hang.json names each with its carrier
surface (`wall`) and `centre`. The five empty consoles are listed for completeness; no schedule names them.

| Key | Kind | Frame: origin = carrier top centre (x, y, z); x = facing | Belongs to |
|---|---|---|---|
| `classical-hall.gallery.niche.e1` | Mount | (3.247, 28.100, 0.57); x (−1, 0, 0) (180°) | niche-pedestal: classical-augustus-prima-porta |
| `classical-hall.gallery.niche.e2` | Mount | (3.315, 22.100, 0.57); x (−1, 0, 0) (180°) | niche-plinth: classical-capitoline-venus |
| `classical-hall.gallery.niche.e3` | Mount | (3.250, 16.100, 0.57); x (−1, 0, 0) (180°) | niche-pedestal: classical-minerva-giustiniani |
| `classical-hall.gallery.niche.e4` | Mount | (3.267, 10.100, 0.57); x (−1, 0, 0) (180°) | niche-pedestal: classical-sophocles |
| `classical-hall.gallery.niche.w1` | Mount | (−3.226, 28.100, 0.57); x (1, 0, 0) (0°) | niche-pedestal: classical-doryphoros |
| `classical-hall.gallery.niche.w2` | Mount | (−3.229, 22.100, 0.57); x (1, 0, 0) (0°) | niche-pedestal: classical-lansdowne-heracles |
| `classical-hall.gallery.niche.w3` | Mount | (−3.315, 16.100, 0.57); x (1, 0, 0) (0°) | niche-plinth: classical-capitoline-antinous |
| `classical-hall.gallery.niche.w4` | Mount | (−3.315, 10.100, 0.57); x (1, 0, 0) (0°) | niche-plinth: classical-farnese-diadoumenos |
| `classical-hall.gallery.bust.e1s` | Mount | (2.850, 29.700, 1.55); x (−1, 0, 0) (180°) | console: classical-julia-titi |
| `classical-hall.gallery.bust.e1n` | Mount | (2.850, 26.500, 1.55); x (−1, 0, 0) (180°) | console: classical-julius-caesar |
| `classical-hall.gallery.bust.e2s` | Mount | (2.850, 23.700, 1.55); x (−1, 0, 0) (180°) | console: classical-antoninus-pius |
| `classical-hall.gallery.bust.e2n` | Mount | (2.850, 20.500, 1.55); x (−1, 0, 0) (180°) | console: classical-young-marcus-aurelius |
| `classical-hall.gallery.bust.e3s` | Mount | (2.850, 17.700, 1.55); x (−1, 0, 0) (180°) | console: classical-severan-lady |
| `classical-hall.gallery.bust.e3n` | Mount | (2.845, 14.500, 1.55); x (−1, 0, 0) (180°) | console: classical-septimius-severus |
| `classical-hall.gallery.bust.e4s` | Mount | (2.850, 11.700, 1.55); x (−1, 0, 0) (180°) | console: classical-oxford-bust |
| `classical-hall.gallery.bust.e4n` | Mount | (2.850, 8.500, 1.55); x (−1, 0, 0) (180°) | console: classical-philip-the-arab |
| `classical-hall.gallery.bust.w1s` | Mount | (−2.850, 29.700, 1.55); x (1, 0, 0) (0°) | console: classical-euripides |
| `classical-hall.gallery.bust.w1n` | Mount | (−2.850, 26.500, 1.55); x (1, 0, 0) (0°) | console: classical-plato |
| `classical-hall.gallery.bust.w2s` | Mount | (−2.840, 23.700, 1.55); x (1, 0, 0) (0°) | console: classical-kimon |
| `classical-hall.gallery.bust.w2n` | Mount | (−2.850, 20.500, 1.55); x (1, 0, 0) (0°) | console: classical-homer-epimenides |
| `classical-hall.gallery.bust.w3s` | Mount | (−2.850, 17.700, 1.55); x (1, 0, 0) (0°) | console: classical-chrysippus |
| `classical-hall.gallery.bust.w3n` | Mount | (−2.850, 14.500, 1.55); x (1, 0, 0) (0°) | console: classical-pseudo-seneca |
| `classical-hall.gallery.bust.w4s` | Mount | (−2.850, 11.700, 1.55); x (1, 0, 0) (0°) | console: classical-sciarra-amazon |
| `classical-hall.gallery.bust.w4n` | Mount | (−2.850, 8.500, 1.55); x (1, 0, 0) (0°) | console: classical-zeno-of-citium |
| `classical-hall.gallery.herm.w` | Mount | (−2.450, 30.830, 1.25); x (0, −1, 0) (270°) | herm: classical-homer |
| `classical-hall.gallery.herm.e` | Mount | (2.450, 30.830, 1.25); x (0, −1, 0) (270°) | herm: classical-socrates |
| `classical-hall.gallery.plinth.2` | Mount | (0.000, 22.100, 0.55); x (1, 0, 0) (0°) | plinth: classical-sleeping-ariadne |
| `classical-hall.gallery.plinth.3` | Mount | (0.000, 16.100, 0.55); x (−1, 0, 0) (180°) | plinth: classical-dying-gaul |
| `classical-hall.tribune.niche.n` | Mount | (0.000, −6.960, 0.53); x (0, 1, 0) (90°) | niche-pedestal: classical-apollo-belvedere |
| `classical-hall.tribune.niche.ne` | Mount | (5.807, −3.352, 0.53); x (−0.866, 0.5, 0) (150°) | niche-pedestal: classical-laocoon |
| `classical-hall.tribune.niche.nw` | Mount | (−6.043, −3.489, 0.53); x (0.866, 0.5, 0) (30°) | niche-plinth: classical-venus-de-milo |
| `classical-hall.tribune.niche.se` | Mount | (5.798, 3.347, 0.53); x (−0.866, −0.5, 0) (210°) | niche-pedestal: classical-barberini-faun |
| `classical-hall.tribune.niche.sw` | Mount | (−6.029, 3.481, 0.51); x (0.866, −0.5, 0) (330°) | niche-pedestal: classical-belvedere-antinous |
| `classical-hall.tribune.bust.n-a` | Mount | (−1.664, −6.211, 1.55); x (0.2588, 0.9659, 0) (75°) | console: classical-head-of-a-gaul |
| `classical-hall.tribune.bust.n-b` | Mount | (1.664, −6.211, 1.55); x towards O (105°) | empty console 0.36 × 0.30 (as built) |
| `classical-hall.tribune.bust.ne-a` | Mount | (4.511, −4.511, 1.55); x (−0.7071, 0.7071, 0) (135°) | console: classical-athena-parthenos |
| `classical-hall.tribune.bust.ne-b` | Mount | (6.206, −1.663, 1.55); x (−0.9659, 0.2588, 0) (165°) | console: classical-apollo-giustiniani |
| `classical-hall.tribune.bust.nw-a` | Mount | (−6.211, −1.664, 1.55); x (0.9659, 0.2588, 0) (15°) | console: classical-alexander-helios |
| `classical-hall.tribune.bust.nw-b` | Mount | (−4.547, −4.547, 1.55); x (0.7071, 0.7071, 0) (45°) | console: classical-ptolemy-ii |
| `classical-hall.tribune.bust.se-a` | Mount | (6.211, 1.664, 1.55); x towards O (195°) | empty console 0.36 × 0.30 (as built) |
| `classical-hall.tribune.bust.se-b` | Mount | (4.547, 4.547, 1.55); x towards O (225°) | empty console 0.36 × 0.30 (as built) |
| `classical-hall.tribune.bust.sw-a` | Mount | (−4.547, 4.547, 1.55); x towards O (315°) | empty console 0.36 × 0.30 (as built) |
| `classical-hall.tribune.bust.sw-b` | Mount | (−6.211, 1.664, 1.55); x towards O (345°) | empty console 0.36 × 0.30 (as built) |
| `classical-hall.tribune.plinth.e` | Mount | (5.100, 0.000, 0.50); x (−1, 0, 0) (180°) | plinth: classical-discobolus |
| `classical-hall.tribune.plinth.w` | Mount | (−5.100, 0.000, 0.50); x (1, 0, 0) (0°) | plinth: classical-artemision-bronze |
| `classical-hall.tribune.centre` | Mount | (0.000, 0.000, 0.30); x (0, 1, 0) (90°) | pedestal: classical-belvedere-torso |

No Body, Use, Wall, Bed, Water or Seat anchors (nothing moves, no paintings, no planting, no seats). Luminaires use
`point`s (Lighting.json). No channels.

## 3. Pieces by trade

**Build-ups.** Every piece is one closed solid of one substance. The structural masonry behind every finish (wall
cores, vault shells, slab, roof, sleeve) is `classical-hall.shell`, hidden; it fills from each inner finish to the
shell faces of §1. Crema Marfil is a **laid finish** (30 mm) over that core, as in the Rotunda. Pieces meet face to
face: the profiles below keep the as-built 0.010 sink points (A −0.010), which Form trims to the face they meet.
Profiles are (A out from the named face, B up), traced with the solid on the left; "round to P" is a quarter circle
(convex bulges out, concave in); "cyma to P, convex first" is a cyma reversa of two quarter rounds meeting at the
chord's midpoint ("concave first", a cyma recta). Station counts as built: round arches 48; the tribune 240 (1.5°).

### 3.1 Walls, niches, lintel
- **Gallery long walls** x = ±3.00, y 31.10 → 7.10, z 0 → the ledge 3.70; north of the screen (y 7.10 → 5.8788) they
  are the throat's jambs, z 0 → the lintel soffit 3.075. 1.20 thick to the shell (x ±4.20).
- **South wall** y = 31.10, x ±3.00, z 0 → the vault's end arc x = 3.8284 sin(s/3.8284), z = 1.3216 + 3.8284 cos(s/3.8284),
  |s| ≤ 3.4473; pierced by the rough opening (§2); 0.30 to the shell face y 31.40.
- **Tribune wall** the cylinder r 6.60 about O, z 0 → 3.70, 1.30 to the shell (r 7.90). Open to the throat between
  φ 62.964° and 117.036° (where r 6.60 meets x = ±3.00 at y 5.8788); over the opening it exists only from 3.075 to 3.70.
- **Every wall** splits at z 0.45: **dado** `classical-hall.wall.dado` below (slabs 1.20 long, one course), **field**
  `classical-hall.wall.field` above (courses 1.20, blocks 1.90 ± 30 % half-bond, 3 mm joints: the Rotunda's coursing
  record, not duplicated).
- **Lintel** `classical-hall.lintel`: soffit plane z 3.075 over the throat, x ±3.00, from y 7.10 to the tribune wall;
  its south face the plane y 7.10 from 3.075 up to the vault's end arc (carries the lunette panel, §3.4).
- **Gallery niches (8)** on x = ±3.00 at the bay centres, E1…E4 and W1…W4 south to north: half-round R 0.75 centred on
  the wall plane; sill 0.45; springing 2.19; head a quarter sphere R 0.75 about (wall plane, 2.19), crown 2.94.
- **Tribune niches (5)** at φ 30° (SE), 150° (SW), 210° (NW), 270° (N), 330° (NE): mouth the plane tangent to r 6.60 on
  the niche axis; half-round R 0.90 centred on the mouth plane at r 6.60; sill 0.45, springing 2.05, crown 2.95 (for
  the Apollo Belvedere); a sliver reveal joins the cylinder to the mouth plane, 0.0617 deep at the mouth's edges
  (surface `classical-hall.wall.field`).
- Niche backs and heads `classical-hall.niche.interior`; each niche's floor slab at 0.45, 0.05 thick,
  `classical-hall.niche.sill`.

### 3.2 Floors (opus sectile, 30 mm on the slab, finish z 0.00, 1.5 mm joints; each figure laid in pieces within stock)
The porphyry discs Ø 3.10 and Ø 3.00 exceed any block: Lay() cuts them in sectors.
- **Gallery** (after the Pantheon's floor). White border `classical-hall.floor.white`: side strips x ±2.10…±3.00 the full
  length; south strip y 30.70–31.10; north strip y 7.10–7.50; the throat y 5.8788–7.10 on to the tribune's rim.
  **Carpet** x ±2.10, y 7.50–30.70: end panels `classical-hall.floor.verde` y 30.20–30.70 and 7.50–8.00; per bay a square
  4.20 × 4.20 on the bay centre: verde frame 0.15, field `classical-hall.floor.giallo`, a white ring r 1.55–1.65 round a
  porphyry disc r 1.55 `classical-hall.floor.porphyry`. **Between the bays** (y 24.20–26.00, 18.20–20.00, 12.20–14.00):
  a verde frame 0.15 round three white cells (x −1.95…−0.65, −0.65…0.65, 0.65…1.95; 1.50 deep), each with a centred
  lozenge `classical-hall.floor.rosso`, half-diagonals 0.55 in x and y.
- **Tribune** (polar about O; sector boundaries at φ 15° + 30°k; 240 stations):

| r from | r to | Figure | Surface |
|---|---|---|---|
| 0 | 1.50 | disc under the pedestal | porphyry |
| 1.50 | 1.65 | ring | giallo |
| 1.65 | 2.90 | 12 sectors, each a white field with a disc r 0.22 at r 2.275 on its mid-angle | white; porphyry discs |
| 2.90 | 3.05 | band | giallo |
| 3.05 | 3.80 | band (the ring columns stand on it) | white |
| 3.80 | 3.95 | band | verde |
| 3.95 | 6.15 | 12 sectors, each a giallo field in a white border 0.10, a disc r 0.80 at r 5.05 on its mid-angle | discs porphyry at φ 30°, 90° … 330°; verde at 0°, 60° … 300° |
| 6.15 | 6.30 | band | verde |
| 6.30 | 6.60 | border to the wall | white |

### 3.3 Vaults, dome, laylights (lime plaster run in place on the shells)
- **Gallery barrel** `classical-hall.vault.gallery`: segmental, R 3.8284 about the line (x 0, z 1.3216) along y 7.10 →
  31.10; springs from the ledge (x ±3.00, z 3.70) at a half-angle of 51.592°; crown 5.15. s = arc length from the
  crown (+ east); springing s ±3.4473. Plain springing bands |s| 3.00–3.4473; coffer field s ∈ [−3.00, 3.00] (x ±2.7023,
  z 4.0335 at its edges): 10 rows of 0.60. Along y (south to north): plain 31.10–30.50; bay 1 coffers 30.50–25.70
  (8 × 0.60); pair band 25.70–24.50; bay 2 24.50–19.70; pair band 19.70–18.50; bay 3 18.50–13.70; pair band 13.70–12.50;
  bay 4 12.50–7.70; plain 7.70–7.10. Each bay 10 × 8 cells less the lantern's 4 × 4: 64 coffers, **256** in all.
- **Coffer** (0.60 cell): flat rib 0.06 on the vault face (0.12 between neighbours); riser 0.06 into the vault, setback
  0.035; riser 0.06, setback 0.035; riser 0.06 to the back: depth 0.18, back 0.34 × 0.34. On the back's centre a
  **rosette** `classical-hall.vault.rosette` (gallery, white), radius 0.3 × the back's smaller side (0.102), turned from
  (radius fraction, height × R/0.08): (0, −0.004) (1.00, −0.004) (1.00, 0.004) (0.86, 0.014) (0.62, 0.020) (0.36, 0.023)
  (0.20, 0.032) (0, 0.036), eight petals: on the rows 1.00 → 0.36 the radius × (0.84 + 0.16 |cos 4α|).
- **Lanterns** (one per bay): opening s ∈ [−1.20, 1.20] (x ±1.1804, z 4.9635) × bay centre ± 1.20 along y; a flat frame
  0.06 on the vault inside its edge (inner edge s ±1.14: x ±1.1232, z 4.9815; bay centre ± 1.14). Within that inner edge
  the **laylight** `classical-hall.laylight.glass` 2.2464 × 2.280 lies flat at **z 4.940** (underside; laminated 2 × 4 mm,
  top 4.949), just under the lantern's edge (4.9635 at s ±1.20). Cheeks `classical-hall.laylight.well` close it all round,
  from the glass up to the frame's inner edge: 0.04 along the long sides, and at the short ends 0.04 at the corners
  rising to 0.21 at mid-length.
  Above the glass the **plenum** 4.949 → 5.150 (0.20) holds the luminous ceiling (§4); above that is the roof slab 5.15 → 5.45
  (0.30). The lantern is a Housing opening. (As built the glass stood at 5.38 in a 0.23–0.40 well, with 0.02 of roof over it:
  corrected for structure and plenum, ruled.)
- **Annular vault** `classical-hall.vault.annular`, turned about O: section a circle R 1.5009 centred (r 5.10, z 3.6491),
  springing from the tribune entablature's ledge (r 6.60, 3.70) and the ring beam's outer ledge (r 3.60, 3.70), crown
  (5.10, 5.15). Plain bands 0.30 of arc at both springings; coffers between section angles 13.395° and 166.605° (r 6.560 →
  3.640): **60** round (6°, from φ 15° so cells centre on the niche axes) × **7** across (21.887°, 0.573 of arc): 420.
  Coffer detail as the gallery's with rib 0.05 and steps × 0.85 (setbacks 0.02975); rosette `classical-hall.tribune.rosette`.
- **Dome** `classical-hall.dome`: spherical cap **R 4.3377 centred (0, 0, 0.7716)**, from the ring beam's inner ledge
  (r 3.20, z 3.70) to the eye (r 1.20, **z 4.94**). Re-struck through the same springing and eye radius so that the eye's
  laylight has its 0.20 plenum under the 0.30 roof slab (ruled); as built R 3.9464 about 1.3905, eye at 5.15. Plain bands 0.25 of arc at the springing, 0.20 at the eye. Coffers 24
  round (15°, from φ 7.5°) × 3 rings, ring edges (r, z) (3.047, 3.859) (2.463, 4.342) (1.893, 4.674) (1.389, 4.881) (1.200, 4.940): 72;
  rib 0.05, step scale 1.0 / 0.9 / 0.8 from the lowest ring; each with a tribune rosette.
- **Eye:** the dome's rim r 1.20 at z 4.94, closed flush by the laylight disc Ø 2.40 (`classical-hall.laylight.glass`,
  underside 4.940, top 4.949); plenum above to the slab at 5.15. Bead `classical-hall.eye.bead` turned round the rim, (r, z):
  (1.19, 4.96) round to (1.19, 4.88) convex, (1.22, 4.88) (1.22, 4.96).
- **Ring beam** (the entablature turned on both faces of r 3.40, §3.4): inner ledge r 3.008–3.20 at 3.70 (the dome
  springs at 3.20); outer ledge r 3.60–3.792 at 3.70 (the annular vault springs at 3.60).

### 3.4 The order and the mouldings (Ionic after Vignola, D 0.34, column 9 D = 3.06)
- **Columns (26)**, turned; base and capital `classical-hall.order.carved`, shaft a separate piece. Section (r, z):
  plinth box 0.47 × 0.47, z −0.01 … 0.055; base (0, 0.050) (0.200, 0.050) (0.200, 0.055) round to (0.200, 0.115) convex
  (lower torus, to 0.23) (0.190, 0.115) (0.190, 0.120) round to (0.190, 0.155) concave (scotia) (0.185, 0.155)
  (0.185, 0.160) round to (0.185, 0.195) convex (upper torus) (0.176, 0.195) (0.176, 0.200) round to (0.170, 0.215)
  concave (apophyge); **shaft** r 0.170 from 0.215 to 1.0983, then entasis r(t) = 0.170 − 0.02833 (1 − cos(πt/2)),
  t 0…1, to z 2.865 (r 0.14167); **capital** round to (0.14967, 2.875) concave, (0.14967, 2.878) round to
  (0.14967, 2.900) convex (astragal), (0.14567, 2.900) round to (0.195, 2.965) convex (echinus), (0, 2.965). Canalis
  block 0.31 across the volutes × 0.34, z 2.945–3.04; plate 0.47 × 0.32, z 3.005–3.05; two bolsters about the
  front-to-back axis at ±0.155, z 2.945, r 0.075, ±0.19 long, waisted to r 0.056 with a belt r 0.064 for |u| ≤ 0.03; on
  both faces of each a volute (logarithmic spiral fillet 12 × 16 mm, 2.5 turns R 0.069 → 0.0193, round an eye disc
  R 0.018, winding outwards and down from the top, clockwise on the right seen from its face); abacus 0.37 × 0.37,
  z 3.035–3.07, edge (0, 0) (0, 0.012) round to (0.015, 0.027) convex (0.015, 0.035).
  - Gallery pairs (12), shafts `classical-hall.shaft.giallo`: x ±2.60 at y 25.60/24.60, 19.60/18.60, 13.60/12.60,
    volutes facing the gallery's axis. Screen (2), giallo: x ±1.10, y 7.0283, facing south. Tribune ring (12), shafts
    `classical-hall.shaft.pavonazzetto`: r 3.40 at φ 15° + 30°k, fronts radial.
- **Pilasters (10)**: shaft block 0.07 proud × 0.34 wide, z 0.20–2.90, in the shaft's marble; Attic base swept round
  three faces, (A, B): (−0.010, −0.010) (0.065, −0.010) (0.065, 0.055) (0.045, 0.055) round to (0.075, 0.085) convex,
  round to (0.045, 0.115) convex; (0.035, 0.115) (0.035, 0.120) round to (0.020, 0.1375) concave, round to (0.035, 0.155)
  concave; (0.030, 0.155) (0.030, 0.160) round to (0.0475, 0.1775) convex, round to (0.030, 0.195) convex; (0.005, 0.195)
  (0.005, 0.200) round to (0, 0.215) concave; (−0.010, 0.215). Neck: (−0.01, 2.865) (0.004, 2.865) round to (0.004, 2.89)
  convex, round to (0.03, 2.945) convex, (−0.01, 2.945). Capital: canalis block, two flat volute discs R 0.075 at
  ±0.155, z 2.945 with the spiral fillets, abacus 0.40 wide, 0.01 proud of the canalis. Bases and capitals
  `classical-hall.order.carved`. Gallery (giallo): x ±3.00 at y 30.85 (8 cm clear of the south wall) and the antae at
  y 7.27. Tribune (pavonazzetto): r 6.60 at φ 0°, 61.38°, 118.62°, 180°, 240°, 300° (the two by the opening 1.58° off
  the jambs).
- **Entablature** `classical-hall.entablature`, 0.72 over the architrave soffit 3.06 (ledge 3.70, lip top 3.78);
  (A out from the lowest fascia, B up from 3.06): (BackA, 0) (0, 0) (0, 0.055) (0.012, 0.055) (0.012, 0.115)
  (0.024, 0.115) (0.024, 0.185) cyma to (0.050, 0.215) convex first; (0.050, 0.228) (0.012, 0.228) (0.012, 0.418)
  cyma to (0.040, 0.445) convex first; (0.045, 0.445) (0.045, 0.500) (0.085, 0.500) (0.085, 0.515) round to
  (0.125, 0.555) convex; (0.260, 0.555) (0.260, 0.620) (0.272, 0.620) (0.272, 0.630) cyma to (0.330, 0.705) concave
  first; (0.330, 0.720) (0.250, 0.720) (0.250, 0.640) (BackA, 0.640); BackA −0.08 against a wall (architrave face 0.07
  proud). **Dentils** 0.034 wide × 0.040 deep × 0.048 high (z 3.516–3.564), pitch 0.052, on the band 0.035–0.075 out,
  0.06 margin at each run's end; on a ring floor(2πR/0.052). **Gallery run:** both long walls, face 0.07 proud; over each
  column pair a ressaut, face 0.2583 off the wall for the pair's y ± 0.72, solid fillers from the wall for z 3.07–3.685
  and y ± 0.65; across the screen with its face at y 7.17, returning into the south wall. **Tribune run:** round the
  outer wall, face r 6.53, continuous over the opening on the lintel; round the **ring beam** on both faces (architrave
  faces r 3.2583 and 3.5417, common soffit 3.06), dentils on both.
- **Wall mouldings** `classical-hall.moulding`, swept round the whole perimeter from one side of the port's surround to
  the other: **skirting** 0.14 × 0.025: (−0.010, −0.025) (0.025, −0.025) (0.025, 0.100) cyma to (0.006, 0.135) convex
  first, (0.006, 0.140) (−0.010, 0.140). **Dado rail** (top 0.46, across the niche mouths): (−0.010, 0.350)
  (0.008, 0.350) cyma to (0.040, 0.405) concave first, (0.045, 0.405) (0.045, 0.435) round to (0.030, 0.460) convex,
  (−0.010, 0.460). Skirting and rail return round every floor pedestal (§8). **Niche surrounds** (0.10 band round jambs
  and head from 0.46, in the mouth plane; on the curved wall on the plane through the jambs' wall points, sunk to meet
  it), (A out from the opening's edge, B out of the wall): (−0.015, −0.01) (0.100, −0.01) (0.100, 0.014) cyma to
  (0.075, 0.035) convex first, (0.012, 0.035) round to (−0.015, 0.019) convex. **Keystones** at each niche's crown, from
  0.04 below it up into the architrave soffit (3.07), widening 0.10 → 0.15, 0.055 proud.
- **Framed panels** `classical-hall.panel`, 0.80 × 1.90 (z 0.85–2.75), 0.02 proud, in frames (moulding):
  (−0.012, −0.01) (0.045, −0.01) (0.045, 0.012) cyma to (0.012, 0.035) concave first, (−0.012, 0.035). Gallery 16 at bay
  centre ± 1.60 on both walls; tribune 10 at each niche's φ ± 15°, flat, tangent at the centre, 0.02 into the wall at the
  edges. **Lunette panel** 2.60 × 0.80 (z 3.98–4.78), framed, on the lintel's face y 7.10, centred on x 0, facing south.
- **Consoles** `classical-hall.console` on each panel's centre line: quadrant bracket 0.36 wide × 0.30 proud × 0.41 high,
  top 1.55; section (−0.03, 1.14) (0.06, 1.14), elliptic quarter centred (0.06, 1.50), radii 0.21 × 0.36, −90° → 0°,
  (0.30, 1.50) (0.30, 1.55) (−0.03, 1.55). Three grow to their busts (§8): Philip 0.58 wide; Kimon 0.32 and Septimius
  0.31 proud; in the tribune Athena 0.39 × 0.40 and Apollo Giustiniani 0.31 proud.

### 3.5 Carriers for the works (built by the room; sizes in §8)
- **Niche plinths and pedestals** `classical-hall.plinth`, moulded: (0.015, −0.005) (0.015, 0.025) cyma to (0, 0.045)
  convex first, (0, H − 0.03) cyma to (0.02, H − 0.005) concave first, (0.02, H). A **niche plinth** stands on the sill
  (H 0.12 gallery, 0.08 tribune; top 0.57 / 0.53). A **niche pedestal** stands on the floor, its top at the same height,
  running back over the sill into the niche to its back line; the dado and skirting return round its sides.
- **Axis and ambulatory plinths** (plinth), moulded: (0.03, −0.01) (0.03, 0.06) cyma to (0, 0.10) convex first,
  (0, H − 0.075) cyma to (0.03, H − 0.02) concave first, (0.03, H). Gallery bays 2 and 3: 2.30 (y) × 1.04 × 0.55 on the
  axis. Ambulatory: at (±5.10, 0), 0.90 × 0.90 × 0.50; the west one 1.15 along y for the Artemision's arms.
- **Herms** `classical-hall.herm` at (±2.45, 30.83): moulded plinth 0.40 × 0.40 × 0.14 (the plinth profile); shaft
  widening from 0.26 square at 0.13 to 0.32 at 1.17; moulded cap 0.40 × 0.40 × 0.09 from 1.16; top 1.25.
- **Centrepiece pedestal** `classical-hall.pedestal`, turned about O, Ø 2.20 × 0.30: (0, −0.01) (1.10, −0.01) (1.10, 0.03)
  round to (1.08, 0.07) convex, (1.05, 0.07) (1.05, 0.24) cyma to (1.10, 0.285) convex first, (1.10, 0.30) (0, 0.30).

### 3.6 Stairs, glazing, fittings
No stair (the spiral stair and its landing are the Rotunda's, in the well). No glazing to the sky (the laylights are
Light's luminous ceilings behind etched glass). No seats, labels or signs (captions by gaze); the blind doors: §2. The
Rotunda builds its **landing** in the well with the palette's "Classical Hall: landing" keys (`stone.crema-marfil`,
`accent.rojo-alicante`, `accent.negro-marquina`, honed, foot-worn): nothing of it is this stage's.

## 4. Light

**Daylight:** none (Lighting.json `daylight` empty). Underground, no Glazed or Open opening; the only daylight near is
the Rotunda's, down its raised drum onto the landing.

**Luminaires** (Lighting.json, 113; all 3000 K, the Rotunda's and the Salon's colour; no output values here)

| Id | Type | Count / extent | Point and aim | Operation |
|---|---|---|---|---|
| `laylight.bay1…4`, `laylight.eye` | `gap.luminous-ceiling.3000k` | 5: four 2.246 × 2.28 on the bay centres, the eye's disc Ø 2.40 on O | backlit luminous-ceiling modules in the 0.20 plenum over the etched glass (glass top 4.949, slab 5.15; point z 5.10); aim straight down | always |
| `cove.gallery.e1…e4`, `.w1…w4` | `linear-cove.3000k` | 8 runs between the ressauts: y 31.05–25.87, 24.33–19.87, 18.33–13.87, 12.33–7.15 (5.18, 4.46, 4.46, 5.18) | on the cornice ledge behind the lip, x ±2.87, z 3.72; aim up the near half of the barrel (x ±1.73, z 4.737) | always |
| `cove.tribune.01…18` | `linear-cove.3000k` | 18 × 2.208 on the arc, midpoints φ 10° + 20°k (continuous over the lintel) | tribune ledge r 6.47, z 3.72; aim onto the annular vault (r 5.77, z 4.95) | always |
| `cove.ring.outer.01…12`, `cove.ring.inner.01…12` | `linear-cove.3000k` | 12 × 1.887 at r 3.70 and 12 × 1.573 at r 3.10, midpoints φ 30°k (between the columns) | the ring beam's outer and inner ledges, z 3.72; aim the annular vault (r 4.30, z 4.92) and the dome's middle ring (r 2.46, z 4.48) | always |
| `washer.000…330` | `track-wallwasher.3000k` | 11, φ 30°k except 90° (the throat) | short track on the ring beam behind its outer lip (r 3.78, z 3.86); aim out and 33° down at the outer wall (r 6.60, z 2.03) | always |
| `accent.gallery.niche.*` (8), `accent.tribune.niche.*` (5), `accent.tribune.plinth.e.a/b`, `accent.tribune.centre.1…3` | `track-spot.3000k.20deg` | 18 | gallery: track in the vault across the gallery at (∓1.45, y + 0.9, 4.805) (the coffer row beside the lantern); tribune niches: annular vault r 4.20 at φ + 11°, z 4.790; Discobolus: r 4.25 at φ ± 28°, z 4.826; centrepiece: dome's lowest ring r 2.30 at φ 90°, 210°, 330°, z 4.537. Aim: the work's centre (carrier top + 1.10…1.15, 0.10–0.15 in front; centrepiece + 0.90) | presence |
| `accent.gallery.plinth.2/3.w/e`, `accent.tribune.plinth.w.a/b` | `track-spot.3000k.29deg` | 6: two per reclining figure and the Artemision | reclining: (±1.45, y ∓ 0.9, 4.805), aim + 0.45 over the plinth top; Artemision: r 4.25 at 180° ± 28° | presence |
| `accent.gallery.bust.*` (16), `accent.gallery.herm.w/e`, `accent.tribune.bust.*` (5) | `track-spot.3000k.12deg` | 23 | gallery busts (∓1.45, y + 0.9, 4.805); herms (∓1.45, 29.53, 4.805); tribune busts annular vault r 4.40 at φ + 7°, z 4.917. Aim + 0.35 over the carrier top | presence |

Every point, aim and length is in Lighting.json. Accents are one per work as built (two per reclining or
free-standing figure, three on the centrepiece); the five empty consoles get none.

**Targets**

| Surface | Class | Lux | Source |
|---|---|---|---|
| `classical-hall.floor.white` (all walking floors: gallery, throat, ambulatory, centre) | circulation | 100 | UseTargets |
| `classical-hall.wall.field` | circulation (wall min) | 75 | UseTargets |
| `classical-hall.vault.gallery`, `classical-hall.vault.annular`, `classical-hall.dome` | circulation (ceiling min) | 50 | UseTargets |
| `classical-hall.cast` | stone-and-bronze (plaster: flagged §13) | 300 recommended, no maximum ([300, null]) | Care |

Day and night alike: underground there is no dusk to switch at (D-006: architectural light always, gallery light by
presence, on while the visitor is in this stage or in the lower half of the well). **Design order** (measured, never
tuned): laylight glass ≥ lit casts > coffers round each lantern > the cream wall ≥ floor fields > the red niche backs;
in the tribune the eye ≥ dome > annular vault > outer wall. Each niche cast's front ≥ **4 ×** its niche's back at the
same height (the Braccio Nuovo's white on red). Not re-expressed: the as-built rect lights at 1300 cd/m² behind the
diffusers, the 3800 K colour, the as-built lux figures.

## 5. Time and weather
No weather place applies (no Glazed opening): museum, Giverny and London do not reach it. No sun, moon, sky or dial.
Every surface is interior (no sky): never wet or snowy. Time reaches the stage only through Light's operation and
Matter's history. One reference moment suffices for commissioning.

## 6. Motion
None: no mechanism, no channel. The port is an open portal. The neighbouring mechanism is the Rotunda's sun-clock drum
(3.50 in 8 s, inside r 7.60 only); nothing of this stage enters the well, so this stage sets no clearance there. The
Rotunda's "Lower" is already refused while anyone is below z −0.30 there (rotunda.md §6): a visitor anywhere in this
hall (museum z −6.00) keeps the stair open. Site.Reachable must hold that in every configuration.

## 7. Nature
None: no plants, water or ground. Constraint from above: Chenghuai's pond (west lobe) over the tribune, bed at museum
−0.46; its basin is Chenghuai's (Nature.WaterContained is its check), this roof (museum −0.55) is not a basin.

## 8. Art
41 plaster casts (18 statues, 23 busts) after SMK's Royal Cast Collection (public-domain scans). Catalogue
`data/classical.json` (ids verbatim; captions: title, artist, year, collection, city, notes with the KAS number); true
heights (length for Ariadne) from `assets/classical/SOURCES.json`; w × d × h and the base slice (z < 0.04) measured from
`assets/classical/<id>.glb`. Not in `assets/collection.json`. Which cast stands where: `classical_art.py` PLACEMENT,
STATUES, BUSTS (re-run, identical to the draft). All: support cast, frame none, care `stone-and-bronze` (flagged),
finish `classical-hall.cast`. The Rotunda's four niche casts are the Rotunda's.

**Hang rule** (the draft's, computed here; Hang.json carries each carrier):
- A carrier's top is the scan's base slice grown 0.03 on every side, never smaller than as built (gallery niche plinth
  0.56 × 0.40 × 0.12, tribune 0.70 × 0.52 × 0.08, console 0.36 × 0.30). The work's base slice is centred on the
  anchor's origin (the carrier's top centre), front along the anchor's x.
- A niche plinth keeps the as-built axis depth (0.42 R: 0.315 / 0.378 behind the mouth) if its back corners clear the
  half-round by 0.02 and its front stays 0.01 behind the mouth; else it moves forward to that front line; where even
  that fails it becomes a **niche pedestal**: its back corners on the 0.02 line (R − 0.02), its front out past the
  mouth by −front (c = axis depth, − = in front of the mouth), standing on the floor.
- A work clears the niche head, walls, columns and vaults by ≥ 0.02 (Form.NoOverlap, scan against pieces); a niche
  carrier may be lowered to no less than 0.03 high. By the axis estimate only the Belvedere Antinous needs it: top 0.51
  (0.06 high). Nothing else is moved by hand; findings go to the owner's walk.
- Facing: out of the niche; busts and niche statues in the tribune face O; as listed otherwise.

| Anchor | Work (id) | Title · artist | KAS | w × d × h (m) | Base w × d | Carrier (kind, top W × D, top z; niche: axis depth c, front) |
|---|---|---|---|---|---|---|
| `…gallery.niche.e1` | classical-augustus-prima-porta | *Augustus of Prima Porta* · Unknown Roman sculptor | KAS65 | 1.179 × 0.994 × 2.08 | 0.78 × 0.64 | niche-pedestal 0.84 × 0.70, 0.57; c 0.247, front −0.103 |
| `…gallery.niche.e2` | classical-capitoline-venus | *Capitoline Venus* · After Praxiteles | KAS493 | 0.656 × 0.772 × 1.93 | 0.55 × 0.48 | niche-plinth 0.61 × 0.54, 0.57; c 0.315, front +0.045 |
| `…gallery.niche.e3` | classical-minerva-giustiniani | *Minerva Giustiniani* · Unknown Roman sculptor | KAS255 | 0.908 × 0.616 × 2.19 | 0.90 × 0.54 | niche-pedestal 0.96 × 0.60, 0.57; c 0.250, front −0.050 |
| `…gallery.niche.e4` | classical-sophocles | *Lateran Sophocles* · Unknown Roman sculptor | KAS633 | 0.844 × 0.654 × 2.16 | 0.82 × 0.57 | niche-pedestal 0.88 × 0.63, 0.57; c 0.267, front −0.048 |
| `…gallery.niche.w1` | classical-doryphoros | *Doryphoros (The Spear Bearer)* · After Polykleitos | KAS11 | 0.751 × 0.775 × 2.12 | 0.69 × 0.74 | niche-pedestal 0.75 × 0.80, 0.57; c 0.226, front −0.174 |
| `…gallery.niche.w2` | classical-lansdowne-heracles | *Lansdowne Heracles* · After Skopas? | KAS224 | 0.881 × 0.730 × 2.08 | 0.88 × 0.60 | niche-pedestal 0.94 × 0.66, 0.57; c 0.229, front −0.101 |
| `…gallery.niche.w3` | classical-capitoline-antinous | *Capitoline Antinous* · Unknown Roman sculptor | KAS824 | 0.660 × 0.607 × 1.89 | 0.55 × 0.46 | niche-plinth 0.61 × 0.52, 0.57; c 0.315, front +0.055 |
| `…gallery.niche.w4` | classical-farnese-diadoumenos | *Farnese Diadoumenos* · After Polykleitos | KAS79 | 0.832 × 0.442 × 1.85 | 0.62 × 0.44 | niche-plinth 0.68 × 0.50, 0.57; c 0.315, front +0.065 |
| `…gallery.bust.e1s` | classical-julia-titi | *Julia Titi* · Unknown Roman sculptor | KAS866 | 0.232 × 0.310 × 0.485 | 0.16 × 0.21 | console 0.36 × 0.30, 1.55 |
| `…gallery.bust.e1n` | classical-julius-caesar | *Julius Caesar* · Unknown Roman sculptor | KAS297 | 0.230 × 0.256 × 0.52 | 0.22 × 0.22 | console 0.36 × 0.30, 1.55 |
| `…gallery.bust.e2s` | classical-antoninus-pius | *Antoninus Pius in a toga* · Unknown Roman sculptor | KAS596 | 0.673 × 0.335 × 0.895 | 0.24 × 0.24 | console 0.36 × 0.30, 1.55 |
| `…gallery.bust.e2n` | classical-young-marcus-aurelius | *Marcus Aurelius as a youth* · Unknown Roman sculptor | KAS1090 | 0.521 × 0.323 × 0.75 | 0.23 × 0.23 | console 0.36 × 0.30, 1.55 |
| `…gallery.bust.e3s` | classical-severan-lady | *A lady of the Severan court* · Unknown Roman sculptor | KAS687 | 0.489 × 0.336 × 0.81 | 0.23 × 0.23 | console 0.36 × 0.30, 1.55 |
| `…gallery.bust.e3n` | classical-septimius-severus | *Septimius Severus in a toga* · Unknown Roman sculptor | KAS683 | 0.546 × 0.396 × 0.85 | 0.25 × 0.25 | console 0.36 × 0.31, 1.55 |
| `…gallery.bust.e4s` | classical-oxford-bust | *The Oxford bust* · Unknown Roman sculptor | KAS106 | 0.573 × 0.314 × 0.80 | 0.22 × 0.22 | console 0.36 × 0.30, 1.55 |
| `…gallery.bust.e4n` | classical-philip-the-arab | *Philip the Arab* · Unknown Roman sculptor | KAS2147 | 0.557 × 0.310 × 0.46 | 0.52 × 0.23 | console 0.58 × 0.30, 1.55 |
| `…gallery.bust.w1s` | classical-euripides | *Euripides (Farnese type)* · Unknown Roman sculptor | KAS620 | 0.303 × 0.301 × 0.56 | 0.22 × 0.22 | console 0.36 × 0.30, 1.55 |
| `…gallery.bust.w1n` | classical-plato | *Plato* · After Silanion | KAS2111 | 0.247 × 0.288 × 0.49 | 0.22 × 0.22 | console 0.36 × 0.30, 1.55 |
| `…gallery.bust.w2s` | classical-kimon | *A general, identified as Kimon* · Unknown Roman sculptor | KAS534 | 0.303 × 0.334 × 0.66 | 0.30 × 0.26 | console 0.36 × 0.32, 1.55 |
| `…gallery.bust.w2n` | classical-homer-epimenides | *Homer (Epimenides type)* · Unknown Roman sculptor | KAS2237 | 0.234 × 0.240 × 0.49 | 0.18 × 0.18 | console 0.36 × 0.30, 1.55 |
| `…gallery.bust.w3s` | classical-chrysippus | *Chrysippus* · Unknown Roman sculptor | KAS602 | 0.267 × 0.261 × 0.44 | 0.18 × 0.18 | console 0.36 × 0.30, 1.55 |
| `…gallery.bust.w3n` | classical-pseudo-seneca | *The so-called Seneca* · Unknown Hellenistic sculptor | KAS94 | 0.252 × 0.273 × 0.47 | 0.19 × 0.19 | console 0.36 × 0.30, 1.55 |
| `…gallery.bust.w4s` | classical-sciarra-amazon | *Head of an Amazon (Sciarra type)* · After a Greek sculptor | KAS615 | 0.322 × 0.278 × 0.61 | 0.20 × 0.20 | console 0.36 × 0.30, 1.55 |
| `…gallery.bust.w4n` | classical-zeno-of-citium | *Zeno of Citium* · Unknown Roman sculptor | KAS745 | 0.254 × 0.294 × 0.47 | 0.25 × 0.21 | console 0.36 × 0.30, 1.55 |
| `…gallery.herm.w` | classical-homer | *Homer (the blind type)* · Unknown Hellenistic sculptor | KAS210 | 0.307 × 0.292 × 0.57 | 0.18 × 0.18 | herm 0.40 × 0.40, 1.25 |
| `…gallery.herm.e` | classical-socrates | *Socrates* · Unknown Roman sculptor | KAS635 | 0.310 × 0.284 × 0.51 | 0.30 × 0.25 | herm 0.40 × 0.40, 1.25 |
| `…gallery.plinth.2` | classical-sleeping-ariadne | *Sleeping Ariadne* · Unknown Roman sculptor | KAS134 | 1.950 × 0.784 × 0.904 | 1.82 × 0.70 | plinth 2.30 × 1.04, 0.55 |
| `…gallery.plinth.3` | classical-dying-gaul | *Dying Gaul* · After Epigonos of Pergamon | KAS1312 | 1.918 × 0.954 × 0.93 | 1.92 × 0.95 | plinth 2.30 × 1.04, 0.55 |
| `…tribune.niche.n` | classical-apollo-belvedere | *Apollo Belvedere* · After Leochares | KAS353 | 1.419 × 0.978 × 2.24 | 0.81 × 0.75 | niche-pedestal 0.87 × 0.81, 0.53; c 0.360, front −0.045 |
| `…tribune.niche.ne` | classical-laocoon | *Laocoön and His Sons* · Agesander, Athenodoros and Polydorus | KAS385 | 1.548 × 0.918 × 2.08 | 1.36 × 0.77 | niche-pedestal 1.42 × 0.83, 0.53; c 0.105, front −0.310 |
| `…tribune.niche.nw` | classical-venus-de-milo | *Venus de Milo* · Attributed to Alexandros of Antioch | KAS434 | 0.674 × 0.615 × 2.04 | 0.58 × 0.52 | niche-plinth 0.70 × 0.58, 0.53; c 0.378, front +0.088 |
| `…tribune.niche.se` | classical-barberini-faun | *Barberini Faun* · Unknown Hellenistic sculptor | KAS208 | 1.568 × 1.341 × 2.15 | 1.18 × 1.00 | niche-pedestal 1.24 × 1.06, 0.53; c 0.094, front −0.436 |
| `…tribune.niche.sw` | classical-belvedere-antinous | *Belvedere Antinous (Hermes)* · Unknown Roman sculptor | KAS1161 | 0.934 × 0.773 × 2.335 | 0.82 × 0.74 | niche-pedestal 0.88 × 0.80, **0.51**; c 0.362, front −0.038 |
| `…tribune.bust.n-a` | classical-head-of-a-gaul | *Head of a Gaul* · Unknown sculptor | KAS796 | 0.210 × 0.244 × 0.39 | 0.19 × 0.20 | console 0.36 × 0.30, 1.55 |
| `…tribune.bust.ne-a` | classical-athena-parthenos | *Athena Parthenos* · After Phidias | KAS2272 | 0.647 × 0.555 × 0.805 | 0.33 × 0.34 | console 0.39 × 0.40, 1.55 |
| `…tribune.bust.ne-b` | classical-apollo-giustiniani | *Head of Apollo Giustiniani* · Unknown Roman sculptor | KAS616 | 0.348 × 0.519 × 0.58 | 0.25 × 0.25 | console 0.36 × 0.31, 1.55 |
| `…tribune.bust.nw-a` | classical-alexander-helios | *Alexander the Great as Helios* · Unknown Hellenistic sculptor | KAS283 | 0.376 × 0.372 × 0.71 | 0.22 × 0.22 | console 0.36 × 0.30, 1.55 |
| `…tribune.bust.nw-b` | classical-ptolemy-ii | *Ptolemy II Philadelphus* · Unknown Hellenistic sculptor | KAS752 | 0.435 × 0.287 × 0.70 | 0.23 × 0.23 | console 0.36 × 0.30, 1.55 |
| `…tribune.plinth.e` | classical-discobolus | *Discobolus* · After Myron | KAS1074 | 0.959 × 0.467 × 1.55 | 0.70 × 0.41 | plinth 0.90 × 0.90, 0.50 |
| `…tribune.plinth.w` | classical-artemision-bronze | *Zeus or Poseidon of Artemision* · Unknown Greek sculptor | KAS2100 | 2.233 × 0.523 × 2.09 | 1.09 × 0.28 | plinth 1.15 (along y) × 0.90, 0.50 |
| `…tribune.centre` | classical-belvedere-torso | *Belvedere Torso* · Apollonios, son of Nestor | KAS402 | 1.047 × 1.166 × 1.59 | 0.87 × 0.51 | pedestal Ø 2.20, 0.30 |

Five tribune consoles stand empty as built (n-b, se-a, se-b, sw-a, sw-b). Scans carrying a `turn` in SOURCES.json
(pseudo-seneca, alexander-helios, athena-parthenos) carry it in Hang.json. Captions by gaze; credit "Cast: SMK – National
Gallery of Denmark, The Royal Cast Collection, KASnnn; scan public domain (SMK Open)". No physical labels.

## 9. Sound
- **Acoustics** (from Matter, never tuned): one Enclosed ≈ 1,350 m³ of polished and honed marble and run lime plaster,
  coffers scattering: a long, bright reverberation, roughly 7–9 s at 500 Hz by a Sabine estimate (a marble church).
  The new stone records must carry absorption coefficients.
- **Emitters:** footfalls by substance (polished statuario and coloured marbles, hard and crisp). The stair's footfalls
  and the Rotunda's drum (its lift voice) carry down the well through the open port. No sound as built; an air-handling
  hiss is polish.

## 10. Visitor
- **Arrive** down the Rotunda's stair to its landing and through the port into the gallery facing north: four laylit
  bays, the screen, and on the axis beyond, the Apollo Belvedere in the tribune's north niche.
- **Walk** the 24 m gallery past statues in red niches and busts on consoles, round the Ariadne and the Dying Gaul on
  the axis; through the screen into the tribune; the ambulatory round the ring (clear r 3.64–6.60; ≈ 1.0 either side
  of the ambulatory plinths), or in under the eye to the Belvedere Torso. Gallery aisle ≈ 2.3–2.4 between an axis
  plinth and the dado rail (≈ 2.0 where a niche pedestal projects). **Leave** the way you came: a dead end.
- **Look:** gaze at a cast for its caption. **Step-to (VR):** the floors only; never a sill (0.45) or a carrier.
  **Sit:** nowhere (no seats as built; benches are an owner's option). **Use:** nothing. Wayfinding: Site's sign at the
  port, "Rotunda", up the stair. Collision is Form's own solids (the as-built invisible blockers are retired).

## 11. The old-code map
Paths relative to `windows\` unless marked repo. Re-express, never copy.

| Part | Where |
|---|---|
| Hall plan numbers (floor −6, roof −0.55, laylights −0.62, shell −6.4, shaft r 7.6, opening, gallery −8.2/−32.2, order, crowns, tribune −39.3) | Source/MuseeVision/Plan/MuseePlan.h:461-509 |
| The serving shaft and port (Rotunda side) | …/Plan/MuseePlan.h:119-158; SunClock/SunClockShaft.cpp:44-93 |
| Design summary | Source/MuseeVision/ClassicalHall/ClassicalHallStructure.h:48-68 |
| Derived dimensions (vault R, jambs, lip, shell, sleeve) | …/ClassicalHall/ClassicalHallGeometry.cpp:59-143 |
| Stations, bay and pair centres | …/ClassicalHallGeometry.cpp:145-213 |
| Niches (flat and curved, head, reveal); gallery and tribune niche lists | …/ClassicalHallGeometry.cpp:215-363, 859-875, 1065-1083 |
| Profiles (entablature, skirting, dado rail, pilaster base, surround, door architrave (retired), panel frame) | …/ClassicalHallGeometry.cpp:365-455 |
| Column section, volute, capital, pilaster, dentils | …/ClassicalHallGeometry.cpp:501-710 |
| Coffer, rosette | …/ClassicalHallGeometry.cpp:712-847 |
| Walls, south wall, lip, passage, sleeve; tribune wall | …/ClassicalHallGeometry.cpp:877-1061, 1086-1119 |
| Gallery vault and lanterns; annular vault; dome, eye, bead; ring beam | …/ClassicalHallGeometry.cpp:1123-1254, 1257-1317, 1320-1400, 1403-1418 |
| Floors; lintel; shell (footprint evidence) | …/ClassicalHallGeometry.cpp:1482-1671, 1675-1700, 1704-1772 |
| Entablatures and wall mouldings | …/ClassicalHallGeometry.cpp:1777-1853 |
| Plinths, herms, keystones, panels, niche fittings, consoles; door frame (retired); pedestal | …/ClassicalHallGeometry.cpp:1858-2041, 2043-2052 |
| Pilasters and columns; places (axis, ambulatory, herms) | …/ClassicalHallGeometry.cpp:2057-2132 |
| Hidden blockers (retired) | …/ClassicalHallGeometry.cpp:2138-2178 |
| Spots (places, facings); lights (evidence: laylights, coves, washers, accents, 3800 K) | …/ClassicalHallGeometry.cpp:2181-2321, 2323-2486 |
| Assembly; lunette panel; panel and console placement | …/ClassicalHallGeometry.cpp:2489-2552 |
| As-built materials, the diffuser | …/ClassicalHall/ClassicalHallStructure.cpp:147-162, 279-319 |
| The hang (PLACEMENT, STATUES, BUSTS); scaling to true size | Scripts/classical_art.py:144-192, 109-141 |
| Sizes and scans; labels | repo assets/classical/SOURCES.json, assets/classical/*.glb; repo data/classical.json |
| Bronze angle on the sill (retired: the joint's strip) | Source/MuseeVision/Furniture/MuseeFurniture.cpp:336-344 |
| The pond above the tribune | Source/MuseeVision/Chenghuai/ChenghuaiPlan.h:135-136 |

**Corrections to the draft** (`r-specs/…/Stages/classical-hall.md`):
1. **Class name** `stairwell.spiral.3600x4400.arched` (width × crown, the Rotunda's), not `stairwell.spiral.3600`.
2. **Materials are palette keys** (D-018, Story "The Classical Hall"): cream marble → `stone.crema-marfil`; rosso antico
   dado and lozenges → `accent.rojo-alicante`; giallo antico → `accent.giallo-siena`; statuary → `accent.statuario`;
   pavonazzetto, verde, porphyry → the palette's (porphyry **reclaimed**); Pompeian stucco → `white.lime-plaster`
   pigmented with red ochre, burnished; opal glass → `glass.low-iron` acid-etched, laminated. **Rosettes:** gilt only in
   the tribune (the palette's role); the gallery's 256 are white fibrous plaster (draft: all gilt plaster of Paris).
3. **Light, 3000 K** (draft 4000 K with two new records `luminous-panel.4000k` and `linear-cove.4000k`): the coves are
   the catalogue's `linear-cove.3000k`, the washers `track-wallwasher.3000k` (complete), the spots
   `track-spot.3000k.*`; one gap, `gap.luminous-ceiling.3000k`, the Salon cabinet's. Every accent is listed with its
   point and aim (draft: "Scheme chooses"). The washer at φ 90° is dropped (it aimed through the throat opening at no
   wall): 11, not 12. Operation `always` / `presence` (the contract's words).
4. **Hang:** the draft's rule is computed into carriers (§8) instead of deferred findings; anchors are `Mount` (the
   contract has no "Plinth" kind); console growth for Philip, Kimon, Septimius, Athena and Apollo Giustiniani.
5. **Verified unchanged:** frame and conversion (+39.30, +6.00); footprint (31.40, 31.996, 6.691, 57.88°/122.12°);
   port frame and depths (0.600 / 0.816); every wall, niche, lintel and throat number (5.8788, 62.964°); vault R 3.8284 /
   axis 1.3216 / s 3.4473; lantern edges (1.1805, 1.1232); annular R 1.5009 at (5.10, 3.6491); dome R 3.9464 at 1.3905;
   coffer counts 256 / 420 / 72; all profile point lists; column and capital; pilaster positions (30.85, 7.27, 61.38°,
   118.62°); columns (2.60, 7.0283, r 3.40); ressaut 0.2583; plinths, herms, pedestal, consoles; floors; cove runs; the
   hang assignment (re-run from classical_art.py); every cast id, height and scan bound.

## 12. The first-pass cut
**Tonight:** Site (frame, footprint volume, the receiving well, the port with its class, rough opening, sleeve and
stand-alone blind doors, one enclosure); walls with the dado split, 13 niches with heads, reveals and sills, the throat
and lintel; the gallery barrel with 256 stepped coffers and 4 lantern wells, the annular vault's 420, the dome's 72 and
the eye well, the ring beam; 26 columns (turned, entasis, carved Ionic capitals) and 10 pilasters; the entablatures
with ressauts, fillers and dentils; skirting, dado rail, surrounds, keystones, 26 framed panels with consoles, the
lunette panel; the opus sectile floors; the carriers of §8; the 41 casts on their anchors with captions; the laylight
glass, luminaires and targets of §4; acoustics and footfalls. If Form's Carving isn't ready, the coffer backs stay
plain (no rosettes) and the volutes are the flat discs of the pilasters.

**Waits for polish:** rosettes and the eye's bead if Carving hasn't landed; physical labels; an air-handling emitter;
benches in the tribune (owner's option); pore-soiling and wear calibrated to photographs; the owner's walk over the
hang findings (the pedestals, the Antinous's lowered plinth, grown consoles); nothing structural (the section is ruled, §3.3).

## 13. Palette keys to confirm and catalogue gaps
**Palette keys** (no `todo.` keys; roles guessed or workings outside `workingsByEra`):
- `accent.statuario` for the **niche sills, plinths and pedestals, skirting, dado rail, niche surrounds, keystones and
  panel frames** (roles name bases, capitals, entablatures, herms, consoles, white floor fields).
- `accent.pavonazzetto` for the **26 wall panels, the lunette panel and the 6 tribune pilaster shafts** (role: the
  tribune's twelve columns); `accent.giallo-siena` for the **4 gallery pilaster shafts** (role: the paired shafts).
- `accent.rojo-alicante` polished for the **floor lozenges** (roles: the dado, the landing's red).
- `white.fibrous-plaster` + `gilt.gold-leaf.oil` for the **eye's bead** (role: tribune rosettes); the gallery rosettes
  white fibrous plaster, **cast**, ungilded.
- Workings written as compound strings: `burnished.red-ochre` (niches), `acid-etched.laminated` (laylights),
  `polished.reclaimed` (porphyry), `cast.seams-kept.sized` (casts); `stone.travertine-romano` **sawn** (hidden core, not
  in workingsByEra); `trowelled` for the laylight wells.
- **Resolved by Matter (FX-JOIN):** the stones of §3 now have palette entries and typed records (`accent.statuario`,
  `accent.giallo-siena`, `accent.pavonazzetto`, `accent.verde-antico`, `accent.porphyry`; Rojo Alicante gains its
  polished finishes), `polished` is a working, and `glass.low-iron` offers `acid-etched.laminated`. Three keys broke the
  Story's physics, and **the schedule was changed to the Story's material**:
  - the niches are `white.lime-plaster.red-ochre`, **burnished**: red ochre is mixed into the setting coat, so it is in
    the plaster's body (a variant of the substance, one key per pigment as the silks are one key per dye), not a
    working (was `white.lime-plaster` / `burnished.red-ochre`);
  - the porphyry roundels are **polished**: reclaimed is the stone's provenance, not a working (was
    `polished.reclaimed`);
  - the gallery rosettes are **`cast.plaster`**, the fibrous plaster's cast working (was `cast`, which names no
    working).
- **Care class of the casts:** plaster has no class; `stone-and-bronze` used (ruled: sized plaster). Care.json says a
  coated surface takes its coating's class, and the Story seals the casts with size: confirm (as for the Rotunda).
- The **landing** keys the palette gives "Classical Hall: landing floor … the landing's red / black" are used by the
  Rotunda, which builds that landing in its served well (`rotunda.stair.landing.floor`, `.red`, `.black`).

**Catalogue gaps** (Light): `gap.luminous-ceiling.3000k` (the Salon cabinet's gap), a real **backlit luminous-ceiling module**
in a 0.20 plenum (glass top 4.949, slab underside 5.15), dimmable, CRI > 90,
four 2.246 × 2.28 and one Ø 2.40 disc. No other gap.

**Conflicts and questions for the Architect:**
(a) **CCT.** The draft and the owner's earlier choice (Luminaires.md: "high-CRI 4000 K for the works") against the
finished Rotunda and Salon (3000 K). This sheet follows the finished sheets (one colour through the stair into the
hall, no new records). If the Architect rules 4000 K: spots → `track-spot.4000k.{12,20,38}deg` (no 29°: use 38°), washers
→ `track-wallwasher.4000k` (evidence incomplete), coves and laylights → `gap.linear-cove.4000k`,
`gap.luminous-ceiling.4000k`.
(b) **The hang** changes the as-built look: niche pedestals project in front of 9 of the 13 niches (the Faun by 0.44,
the Laocoön by 0.31). The as-built plinths were smaller than the casts' bases; the alternative is to accept overhanging
bases (not physical). Owner's walk.
(c) **Structure over the laylights (ruled):** the roof is a 0.30 slab, z 5.15 → 5.45 (museum −0.85 → −0.55), under the
Rotunda's floor, Chenghuai's pond and the Grounds. The vault and annular crowns (5.15) meet its underside. The laylight
glass drops from 5.38 to 4.94 so that a 0.20 plenum lies between it and the slab, and the dome is re-struck through its
springing to meet the eye at 4.94 (R 4.3377 about 0.7716). No other stage moves.
(d) The Rotunda sheet says its floor bears on this roof "north of y −8.20" (the gallery's inner face); the hall's shell
reaches museum y −7.90 (x ±4.20) and its sleeve the r 7.60 cylinder. No volume clash (both meet at −0.55); Site should
use the shell line.

## Build notes
- **Section (ruled).** Roof slab 0.30 (z 5.15 → 5.45); plenum 0.20 over every laylight; laylight glass underside
  4.940 in the four lanterns and the eye; the dome re-struck R 4.3377 about (0, 0, 0.7716). Nothing stands above 5.45.
- **Frame.** Origin O = the tribune's centre on its floor; museum = local + (0, −39.30, −6.00), yaw 0. Old y + 39.30,
  old z + 6.00.
- **Nothing moves.** No body, no mechanism, no channel. The Rotunda's drum above is the only mechanism near.
- **You receive the well.** The cylinder r 7.60 about W = (0, 39.30), z −0.40 … 6.00, is the Rotunda's: build nothing
  inside it. The port `classical-hall.stair` at (0, 31.70, 0) facing +y mates `rotunda.stair` (0, −7.60, −6.00), class
  `stairwell.spiral.3600x4400.arched`.
- **Joint vs room.** The room builds only the rough opening (clear + a) through the south wall and sleeve to the
  cylinder. Site's joint builds the threshold, bronze strip on the arc, linings and surround. Blind doors only on the
  stand.
- **Unusual drawings.** The port face is a cylinder while the wall behind is flat (depth 0.600 → 0.816). The tribune
  panels are flat on a curved wall. The ring beam carries the entablature on both faces. Porphyry discs are cut into
  sectors within stock.
- **Carriers are sized by the works.** Build each plinth, pedestal and console at its §8 size. A niche pedestal
  stands on the floor, and the skirting and dado rail return round it.
- **Shared anchors.** The 46 `Mount` anchors (carrier top centres) are read by Art (the casts) and located by the
  room's carriers. Light uses points, not anchors.

**Departures from as-built** (for the owner's review):
- The **cast carriers are sized to the casts** (ruled). 9 of the 13 niches get floor pedestals that project past the mouth; the Belvedere Antinous's plinth is lowered to a 0.51 top; five consoles grow to fit their busts.
- **The section is made real** (ruled): a 0.30 roof slab and a 0.20 plenum over every laylight. The laylight glass drops from 5.38 to 4.94, and **the tribune dome is re-struck** through the same springing and eye (R 4.3377 about 0.7716), so its crown is 0.21 lower.
- The as-built 1 cm step at the threshold is retired, so the floor is flush.
- All light is 3000 K (as built 3800 K). The φ 90° wall-washer, which aimed at no wall, is dropped.
- Materials follow the Story; only the tribune's rosettes are gilt. The niches' red is `white.lime-plaster.red-ochre`,
  the porphyry is `polished` and the gallery rosettes `cast.plaster` (§13: the schedule changed to the Story's material).
