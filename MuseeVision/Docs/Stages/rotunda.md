# The Rotunda · design sheet

- **Stage key** `rotunda` (module `MuseeRotunda`, `URotundaRoom : USiteRoom`). Display name "The Rotunda". Era: Classical
  (stone honed, bronze cast and patinated or oil-gilt, plaster matt); the eye's pearl lattice is the one seed of the
  future. **The anchor stage** (D-012): its origin is the museum plan origin; every visit begins on its gilt sun.
- **Sources:** D-001…D-019 (D-004, D-012, D-017, D-018 binding here); Palette.json + Story.md §6 "The Rotunda"; the
  drafter's spec `r-specs/…/Stages/rotunda.md` (verified, corrections in §11); the `Rotunda` board
  (`plan/canvas/Rotunda.dc.html`) and `plan/README.md`; the as-built code in `windows/` (§11). Schedules:
  `Data/Stages/rotunda/{Finishes,Lighting,Hang,Mechanisms}.json` (no Planting: nothing grows here).
- **Units** metres. **Frame:** origin at the drum's centre on the finished floor (top of the ring and of the lowered
  dial); x east, y south, z up, true north. **φ** = plan angle from +x towards +y (east 0°, south 90°, west 180°,
  north 270°); a compass bearing is φ + 90°. **Drum frame z_d:** the lifting drum's own frame, z = z_d lowered,
  z = z_d + 3.500 raised.

## 1. Identity and placement

| | |
|---|---|
| Origin in the museum plan | (0.000, 0.000, 0.000); yaw 0°. museum = local. Not translated: Site's anchor. |
| Arrival point | The gilt sun's top, (0, 0, 0.085) (feet on the boss), facing **west** (−x, yaw 180°) down the Salon's line (D-012; as built `MuseePlan::SpawnFeet`, `SpawnYawDegrees` 180). Declared in `SitePlan.json`. |
| Level | 0 (ground floor). The served well descends to the landing at z −6.000 (level −1, the Classical Hall's floor). |
| Footprint kind | Building, plus the served well below grade (§2). |
| Vertical extent | Room: z −0.55 (floor structure's underside; north of y −8.20 it bears on the Classical Hall's roof, −0.55) to 21.00 (over the eye's curb, 20.45). Well prism: z −6.40 … 0.00. |

**Footprint polygon:** the circle r **11.200** (the drum's outer face); build it on Form's shared stations so each
neighbour meets it vertex for vertex. Reference points every 22.5°: (11.200, 0.000) (10.347, 4.286) (7.920, 7.920)
(4.286, 10.347) (0.000, 11.200) (−4.286, 10.347) (−7.920, 7.920) (−10.347, 4.286) (−11.200, 0.000) (−10.347, −4.286)
(−7.920, −7.920) (−4.286, −10.347) (0.000, −11.200) (4.286, −10.347) (7.920, −7.920) (10.347, −4.286). The exterior
dress (§3.10, polish) will stand proud of it; tonight the footprint is r 11.20.

**Well prism** (inside the footprint): the vertical cylinder r 7.600 about the z axis, z −6.400 … 0.000.

| Enclosure | Kind | Bounded by |
|---|---|---|
| `rotunda.hall` | Enclosed | The drum's inner face (r 10.00; niches to r 11.00), the entablature, the coffered dome, the eye's glass, the floor (ring and lowered dial). ≈ 5,240 m³ (π·10²·10 + hemisphere R 10). Openings: the four doorway ports; Glazed `rotunda.eye` (D-004, its only opening to the sky); while raised, the drum's west door into `rotunda.stair`. |
| `rotunda.stair` | Enclosed | The stair wall (r 6.48) from the landing (z −6.00) to the dial's soffit; raised, also the drum's interior (r 6.545) to z 3.37. ≈ 774 m³ lowered, ≈ 1,250 m³ raised. Openings: port `rotunda.stair`; while raised, the drum's west door. Lowered it is sealed by the dial. |

**Openings (Form's one list):** Glazed `rotunda.eye` (disc r 2.50, z 20.38–20.39; weather `museum`); Ports
`rotunda.west`, `.north`, `.south`, `.east`, `.stair`; Open `rotunda.sun-clock.door` (raised configurations only);
Housings: `rotunda.sun-clock.pocket` (annulus r 6.54–7.00, z −3.58…0), `rotunda.sun-clock.rams` (8 bores),
`rotunda.stair.balustrade-housings` (§3.9), `rotunda.sun-clock.luminaires` (13 recesses in the drum's soffit).

## 2. Ports

All frames on the port face at the threshold (z of the finished threshold), on the opening's centreline, facing out,
up +z. Class names follow the finished Nymphéas sheet: **width × crown height** (see the note under the table).

| Key | Kind | Class | Local frame (position; facing) | Face; wall depth | Mates with | Wells |
|---|---|---|---|---|---|---|
| `rotunda.west` | doorway | `doorway.3000x6000.arched` | (−11.200, 0.000, 0.000); −x | cylindrical R 11.20 about the z axis; 1.20 (r 10.00 → 11.20) | `salon.east` (Bay 1's end wall, outer face museum x −13.40). **Passage 2.20** (the joint's). | none |
| `rotunda.north` | doorway | `doorway.4000x6500.arched` | (0.000, −11.200, 0.000); −y | cylindrical R 11.20; 1.20 | `chenghuai.south` (the open porch on the drum face). Passage 0.00. | none |
| `rotunda.south` | doorway | `doorway.4000x6500.arched` | (0.000, +11.200, 0.000); +y | cylindrical R 11.20; 1.20 | `albion.north` (the porch's flat end wall tangent to the drum). Passage 0.00; the joint fills the wedges. | none |
| `rotunda.east` | doorway | `doorway.3000x6000.arched` | (+11.200, 0.000, 0.000); +x | cylindrical R 11.20; 1.20 | `hall-of-light.west` (concave R 11.20). Passage 0.00. | none |
| `rotunda.stair` | stair (served well) | `stairwell.spiral.3600x4400.arched` | (0.000, −7.600, −6.000); −y | cylindrical R 7.60 about the z axis; 0.55 (the arched reveal r 7.05 → 7.60) | `classical-hall.stair` (its frame (0, 31.70, 0), facing +y). Passage 0.00. | **serving**: the well prism |

- `doorway.3000x6000.arched`: clear 3.000; semicircular head r 1.500 springing **4.500**; crown 6.000
  (`MuseePlan.h:44-47`). `doorway.4000x6500.arched`: clear 4.000; head r 2.000 springing 4.500; crown 6.500.
  `stairwell.spiral.3600x4400.arched`: clear 3.600 (jambs the planes x = ±1.800); head r 1.800 springing 2.600 above
  the threshold (z −3.400); crown 4.400 (z −1.600) (`MuseePlan.h:147-157`). **The r-specs drafts named these by the
  springing** (`doorway.3000x4500.arched`, `doorway.4000x4500.arched`, `stairwell.spiral.3600`); the geometry is the
  same. Site's `PortClasses.json` must carry one name per class (§13).
- **Jamb/face intersections:** clear jambs meet the outer face at x = ±11.099 (W, E) and y = ±11.020 (N, S); the
  inner face at ±9.887 and ±9.798.
- **Rough openings.** The Rotunda builds only the rough opening = clear opening + the class's lining allowance *a*
  (Site's; the drafts propose a = 0.050: rough widths 3.10 / 4.10, head radii 1.55 / 2.05 about the springing 4.50,
  rough sill z −0.05), straight through the drum from r 10.00 to 11.20, faces `rotunda.port.rough-opening`. Nothing
  past a port face. **Site's joint** builds the travertine threshold through the wall depth, the brushed-bronze strip
  on the mating plane, the linings, the surrounds at the mating faces, the passage (the west one 2.20 m, to the
  Salon's face at x −13.40) and any wedge masonry. The **imposts** (section, lateral into the opening × height from
  the springing: (−0.01, −0.24) (0.02, −0.24) (0.05, −0.21) (0.05, 0) (−0.01, 0), z 4.26–4.50) belong to the lining,
  i.e. the class detail; if Site doesn't carry them tonight, the archivolts spring from the plain lining.
- **The stair port.** At the port face r 7.60 the class opening; the arched reveal `rotunda.stair.port.reveal` runs in
  to r 7.05; inside r 7.05 the drum's pocket passes over, so the opening is closed above z −3.68 by a lintel
  (r 6.48 → 7.05) and a tympanum on r 7.05: the portal onto the stair is **3.60 × 2.32**, flat-headed. The Rotunda
  finishes its whole side of this port itself: the reveal and the portal are one straight passage at the class's clear
  opening (jambs the planes x = ±1.80 from the stair's face to r 7.60, the reveal's head r 1.80 about the springing), its
  landing is the threshold (§3.7), and its surround is the stair-side dressing (§3.7). The joint lines only the Classical
  Hall's wall, lays the strip on the mating arc, and draws nothing on this side (K10).
- **The well (served).** `rotunda` owns everything inside r 7.60, z −6.40 … 0.00: shaft masonry, stair wall, pocket,
  ram bores, landing and rosette, stair, balustrade, the drum's full travel. `classical-hall` reserves the prism as a
  void and builds nothing inside it; its walls meet r 7.60.
- **Blinds (closed bronze doors).** Every port has a closed pair of cast bronze leaves filling its class opening on
  its port face, meeting rail on the axis, placed while the port is unmated (on the stand, or when its neighbour is
  absent): `rotunda.west.blind-doors`, `rotunda.north.blind-doors`, `rotunda.south.blind-doors`,
  `rotunda.east.blind-doors`, `rotunda.stair.blind-doors` (on the stand, also a blind well to z −6.40). With all five
  neighbours built tonight, all four doorways are open portals (no leaves, as built).

### Anchors (the stage contract)
Frames in the stage frame with the drum lowered. Anchors on `rotunda.sun-clock.drum` ride it: add 3.500 to z when
raised.

| Key | Kind | Frame | Belongs to |
|---|---|---|---|
| `rotunda.sun-clock.drum` | Body | origin (0, 0, 0) (the dial's top centre), axes of the stage; moves +z 0 → 3.500 | the dial and its inlay, the meridian, the gilt sun, the drum wall and dressings, the soffit, rim, pendant and the 13 downlights (§3.1, §3.6, §4) |
| `rotunda.stair.balustrade.panels` | Body | origin (0, 0, 0), axes of the stage, with the panels **at home** (housed, 0.94 below standing); moves +z 0 → 0.940 | the two retracting balustrade panels (§3.9) |
| `rotunda.sun-clock.control` | Use | origin (0, 0, 0.085), the gilt sun's top; the usable volume is the ring: feet at 7.35 < r < 10.00, \|z\| < 0.40 (prompt, click or headset trigger there) | fixed (not on the body) |
| `rotunda.niche.ne.cast` | Mount | (7.248, −7.248, 2.000) on the NE niche's sill (φ 315°); x towards the centre (−0.7071, +0.7071, 0); z up | the Dying Slave (§8) |
| `rotunda.niche.se.cast` | Mount | (7.248, 7.248, 2.000), φ 45°; x (−0.7071, −0.7071, 0) | the Rebellious Slave |
| `rotunda.niche.sw.cast` | Mount | (−7.248, 7.248, 2.000), φ 135°; x (+0.7071, −0.7071, 0) | the Venere Italica |
| `rotunda.niche.nw.cast` | Mount | (−7.248, −7.248, 2.000), φ 225°; x (+0.7071, +0.7071, 0) | the Bacchus |

No Wall, Bed, Water or Seat anchors (no paintings, no planting, no seats). Luminaires mount at the points in
Lighting.json; the 13 drum lights carry `body: rotunda.sun-clock.drum`. **Channels** (Motion publishes):
`rotunda.sun-clock.height` (lift ÷ 3.500, 0…1) and `rotunda.sun-clock.lights` (0 or 1, §6).

## 3. Pieces by trade

**Build-ups.** Crema Marfil is thin-slab stock: every Crema Marfil face is a **laid finish** (30 mm) over a hidden
travertine core (Roman revetment over travertine); visible geometry exactly as built. Pieces meet face to face (as
built they sank 10 mm into each other: mouldings are now trimmed to the face they meet, stone cut where bronze is
set). As-built station counts (evidence of fineness; Form's chord tolerance divides): each semicircular arch 48; drum
faces 320 per turn; dome 280 angles (10 per coffer); every sun-clock circle 768.

### 3.1 Floors (fixed ring; the dial rides the drum, §3.6)
- **Fixed ring** r 7.000 → 10.040 (the back of the shadow gap), top z 0.000, finish 30 mm, core to −0.55. In each
  doorway it stops on the arc r 10.00 across the rough opening (the joint's threshold runs on). Three courses of
  radial slabs: r 7.00–7.75 **48** slabs; r 7.75–8.95 **32** (carries the motto); r 8.95–10.04 **64**, offset half a
  slab (+2.8125°). Tone by parity of (slab + course): even `rotunda.floor.ring.a`, odd `rotunda.floor.ring.b`.
- **Dial** (top of the lifting drum), r 0 → 6.990, top z_d 0.000, seven bands:

| r from | r to | Surface |
|---|---|---|
| 0.00 | 1.40 | `rotunda.dial.field.a` |
| 1.40 | 1.52 | `rotunda.dial.band.red` |
| 1.52 | 3.20 | `rotunda.dial.field.b` |
| 3.20 | 4.90 | `rotunda.dial.field.a` |
| 4.90 | 6.30 | `rotunda.dial.field.b` |
| 6.30 | 6.38 | `rotunda.dial.band.black` |
| 6.38 | 6.99 | `rotunda.dial.field.a` (to the corona's edge) |

Ring and dial meet with 10 mm clear (r 6.99 against 7.00).

**Dial inlay** `rotunda.dial.inlay` (cast bronze, each set in a groove 2 mm deep, proud with a bevelled top edge):

| Mark | Where | Section (width / proud / bevel) |
|---|---|---|
| Hour ring | r 6.76–6.80 (on the shadow circle r 6.78) | 40 / 3 / 1.5 mm |
| Band fillets | centred on r 1.40, 1.52, 6.30, 6.38 | 10 / 2 / 0.8 mm |
| Hour lines | 12 bars at φ_h = 180° + 15°·(h − 6), h = 6…18 except 12, radial r 1.60 → 6.77 (VI west, XII north, VI east) | 24 / 2.5 / 1 mm |
| Minute ticks | every minute 06:00–18:00 except the hours, to r 6.77; from r 6.665 (1 min), 6.62 (5), 6.58 (15), 6.52 (30) | 3/2/0.6; 5/2.2/0.8; 7/2.5/1; 10/2.5/1 mm |
| Numerals | Roman VI VII VIII IX X XI XII I II III IV V VI, each centred on its φ_h, laid along the arc; baseline r 6.815, cap 0.16, tops outward (read from the dial); tracking −0.03 cap | 5 mm proud, 2 mm bevel |

- **The gilt meridian** `rotunda.dial.meridian`: the XII line at φ 270°, r 1.60 → 6.80, **60 mm** wide, 2.5 mm proud,
  1 mm bevel (the board draws it 2.3 × the hour lines; as built it was an ordinary 24 mm bar).
- **The gilt sun** `rotunda.dial.sun` (the arrival point). **Rays:** 32 at φ = 11.25°·i from r 0.24; 16 long (i even,
  one to XII) to r 1.25, 72 mm wide at the root tapering (1 − t)^0.62, crest 42 mm falling (1 − t)^1.2 to 4 mm; 16 short
  flames (i odd) to r 0.86, 60 mm tapering (1 − t)^0.75, crest 30 mm, weaving 0.03·sin(3πt)·√t·(1 − t); across each
  flank: foot (−2 mm), a hollow at 0.40 of the crest height, the ridge. **Boss** (lathe, (r, z)): (0.285, −0.002)
  (0.285, 0.035) arc(c 0.265, 0.035; r 0.020; 0→90°) then 20 steps up the dome: sphere R 1.1854 with its top at z
  0.085, r 0.265 → 0. Bead ring r 0.005 at r 0.19 on the dome; beaded foot, torus section r 0.014 at r 0.30, z 0.012.
- **The motto** `rotunda.floor.motto`: HORAS · NON · NUMERO · NISI · SERENAS in cast bronze capitals on the middle
  course; cap 0.26, baseline r 8.53 (tops towards the dial, cap line r 8.27); centred on φ 90° (south), read from the
  ring facing the dial, running west to east; tracking 0.16 cap, word gap 0.6 cap; 8 mm proud, 2.5 mm bevel. Each "·"
  is a gap of 1.15 cap holding a triangular stop `rotunda.floor.motto.stop` (point down, ±0.12 cap at 0.60 cap height,
  point at 0.36 cap, apex 8 mm high). Glyphs: port the as-built outlines baked from Centaur (installed at C:/Windows/Fonts/CENTAUR.TTF) in `SunClockGlyphs.h`; no download. A public release switches to an open-licence face.

### 3.2 Walls: the drum
- **Section** 1.20: inner face r 10.00 to outer face r 11.20; travertine core, 30 mm Crema Marfil inside.
- **Inner face** `rotunda.drum.wall.inner`, z 0.15 → 10.00 (hidden above 9.47 behind the entablature). Courses: 8 of
  1.165 from 0.15 to 9.47, blocks ≤ 1.9 long, half-bond; datums (niche sill 2.00, springings 4.50, 4.67) on joints
  within ±0.10.
- **Shadow gap** `rotunda.drum.shadow-gap`: z 0 → 0.15, recessed 0.04 (back face r 10.04, soffit z 0.15). Door jambs step
  back into it at the floor. The plinth course that carries the wall over it, from the rough sill z −0.05, stops on each
  doorway's rough jamb planes, as the wall's straight passages do, so the joint's threshold and lining stand against it
  through the whole drum (K10).
- **Outer face** `rotunda.drum.wall.outer` r 11.20, z 0 → 11.00. **Drum top** `rotunda.drum.top`: annulus z 11.00,
  r 10.553 → 11.20.
- **Door rough openings** at φ 0° (E, 3.00), 90° (S, 4.00), 180° (W, 3.00), 270° (N, 4.00): jambs planes parallel
  to the axis at the rough half-width, head a half-cylinder about the springing line z 4.50, straight through (§2).
- **Niches** at φ 45° (SE), 135° (SW), 225° (NW), 315° (NE): width 2.00, sill z 2.00, springing 4.67, crown 5.67.
  Front plane: the chord plane at r 10.00 on the niche axis; between it and the curved face the sliver reveal
  `rotunda.niche.reveal` (0.050 deep at the jambs, 0 on the axis). Back `rotunda.niche.back`: a half-round r 1.00 about
  the axis point at r 10.00, reaching r 11.00 (0.20 of wall behind), z 2.00 → 4.67. Head `rotunda.niche.head`: a
  quarter sphere r 1.00 about (r 10.00, z 4.67). Floor: the sill ledge's top run back to the half-round.

### 3.3 The order: pilasters, entablature, archivolts, niche dressings (profiles (a out, z) or (s, p); metres)
- **16 pilasters** `rotunda.pilaster` (`doric-roman.pilaster`: Tuscan torus base, 7 stopped flutes, Roman Doric
  capital, no carving), 0.90 wide, face at r 9.57, back on the wall face (plinth fills the shadow gap). φ, stepped
  clear of the doors so the abacus (0.115 each side) clears the jamb: **11.776, 33.750, 56.250, 75.337, 104.663,
  123.750, 146.250, 168.224, 191.776, 213.750, 236.250, 255.337, 284.663, 303.750, 326.250, 348.224** (rule:
  max(11.25°, asin(h/ρ) + atan2(0.565, 10.06)), h the door's half-width, ρ = √(10.06² + 0.565²)).
  - Base (swept round front and returns, mitred): (0.10, 0) (0.10, 0.18) (0.03, 0.18) arc(c 0.03, 0.24; r 0.06;
    −90→90°) (0.03, 0.30) (0.02, 0.30) arc(c 0.02, 0.32; r 0.02; 270→180°) (0, 0.32).
  - Shaft z 0.32–8.90; flutes z 0.60–8.62 stopped square: margin 0.070, 7 flutes × 0.085 between 6 fillets × 0.0275,
    margin 0.070; flute depth 0.030·sin(πt). Returns plain.
  - Capital: (0, 8.90) arc(c 0, 8.93; r 0.03; −90→90°) (0, 8.96) (0, 9.14) (0.025, 9.14) (0.025, 9.17)
    arc(c 0.025, 9.26; r 0.09; −90→0°) (0.115, 9.26) (0.115, 9.47); abacus top flat back to the wall.
- **Entablature** `rotunda.entablature` (`entablature.plain-frieze.cavetto-crown`), a ring on the dome's stations,
  (r, z) from the dome down to the wall: (9.9875, 10.500) (9.220, 10.500) (9.220, 10.460) arc(c 9.220, 10.400; r 0.060;
  90→0°) (9.280, 10.400) (9.280, 10.240) (9.420, 10.240) (9.420, 10.210) arc(c 9.510, 10.210; r 0.090; 180→270°)
  (9.510, 10.120) (9.510, 9.840) (9.430, 9.840) (9.430, 9.820) arc(c 9.480, 9.820; r 0.050; 180→270°) (9.480, 9.770)
  (9.480, 9.630) (9.500, 9.630) (9.500, 9.470) (10.000, 9.470). The cornice's top ledge r 9.22–9.99 at z 10.50 carries
  the luminaires of §4.
- **Door archivolts** `rotunda.door.archivolt` (`archivolt.two-fascia-roll`), on the inner face from springing to
  springing round each door's clear arch, dying against the flanking pilasters' sides; (s out from the clear intrados,
  p proud): (0.30, 0) (0.30, 0.10) arc(c 0.27, 0.10; r 0.03; 0→90°) (0.27, 0.13) (0.20, 0.13) (0.20, 0.10) (0.09, 0.10)
  (0.09, 0.08) (0, 0.08) (0, 0). Its inner return meets the joint's lining.
- **Niche surrounds** `rotunda.niche.surround` (`architrave.niche.two-step-roll`): up each jamb from z 2.00 to 4.67,
  round the head (r 1.00 + s), down: (0.20, 0) (0.20, 0.05) arc(c 0.17, 0.05; r 0.03; 0→90°) (0.17, 0.08) (0.07, 0.08)
  (0.07, 0.06) (0, 0.06) (0, 0).
- **Niche sill ledges** `rotunda.niche.sill` (`sill.rounded-ledge`): 0.16 proud, z 1.84–2.00, laterally ±1.26 about
  the axis, capped ends; (d proud, z): (0, 1.84) (0.13, 1.84) arc(c 0.13, 1.87; r 0.03; −90→0°) (0.16, 1.87) (0.16, 2.00)
  (0, 2.00); the top runs back to the half-round as the niche floor. It carries the cast.

### 3.4 The dome
- **Inner surface** sphere R 10.00 about (0, 0, 10.00): plain band `rotunda.dome.band` 2.866° (cornice top, z 10.50,
  r 9.9875) → 4°; **coffers** `rotunda.dome.coffer` 4° → 64°; upper plain band 64° (z 18.988, r 4.384) → 72.54° (z 19.539,
  r 3.000); **eye curb** `rotunda.dome.eye-curb` (r, z): (3.000, 19.539) (3.000, 19.420) (2.750, 19.420) (2.750, 19.580)
  (2.500, 19.580) (2.500, 20.450).
- **Coffers:** 5 rings × 28, each 12° of elevation, sharing the 28 cell angles (θ = 360°/28·c).

| Ring | Elevation | z | r | Cell | Rib | Step | Bevel |
|---|---|---|---|---|---|---|---|
| 1 | 4°–16° | 10.698–12.756 | 9.976–9.613 | 2.210 | 0.243 | 0.099 | 0.110 |
| 2 | 16°–28° | 12.756–14.695 | 9.613–8.829 | 2.081 | 0.229 | 0.094 | 0.104 |
| 3 | 28°–40° | 14.695–16.428 | 8.829–7.660 | 1.860 | 0.205 | 0.084 | 0.093 |
| 4 | 40°–52° | 16.428–17.880 | 7.660–6.157 | 1.559 | 0.171 | 0.070 | 0.078 |
| 5 | 52°–64° | 17.880–18.988 | 6.157–4.384 | 1.189 | 0.131 | 0.054 | 0.059 |

  Cell = (2π/28)·10·cos(e_mid); Rib 0.11, Step 0.045, Bevel 0.05 × Cell. Section on all four sides (s in from the cell
  edge, depth): (0, 0) (Rib, 0) (Rib, 0.12) (Rib+Step, 0.12) (Rib+Step, 0.26) (Rib+2Step, 0.26) (Rib+2Step+Bevel, 0.45);
  panel on the sphere R 10.45. Offsets across θ: 1/(10·cos e_mid) rad per metre; up the elevation 1/10. Neighbouring
  ribs meet on the cell edge.
- **Shell** 0.60: outer sphere R 10.60 about (0, 0, 10.00) from its foot on the drum top (r 10.553, z 11.00) to r 2.90
  (z 20.196), then (2.90, 20.196) → (2.90, 20.450) → (2.50, 20.450), the curb up and over. Covered in lead:
  `rotunda.dome.outer` (milled sheet on boarding with rolls; the boarding and rolls sit on the shell line, polish may
  thicken it outward).

### 3.5 Glazing: the eye (Glazed `rotunda.eye`, D-004; after the Great Court)
- **Lattice** `rotunda.eye.lattice`, one welded and faired body. Ring: r 2.42–2.50 × z 20.22–20.38 against the curb's
  inner face. Bars in three directions φ 0°, 60°, 120°, each 7 at lateral offsets 0.80·k (k −3…3), all three k 0 bars
  crossing at the centre; section 0.05 wide × 0.10 deep, z 20.25–20.35; each runs 0.04 into the ring: lengths 4.920
  (k 0), 4.653 (±1), 3.737 (±2), 1.080 (±3). 21 bars; bars and ring cover ≈ 4.8 of the eye's 19.6 m² (≈ 24 %).
- **Glass** `rotunda.eye.glass`: one disc r 2.50, laminated low-iron 2 × 5 mm + 0.76 PVB (10.76 mm), underside z 20.38
  on the ring, top 20.39, edge against the curb. Flat inside a 60 mm upstand: it ponds (expected; falls are polish).
- **Node** `rotunda.eye.node`: a sphere r 0.12 centred (0, 0, 20.25), enclosing the bars' crossing: the sun clock's
  nodus.

### 3.6 The sun clock's drum (moving body `rotunda.sun-clock.drum`; coordinates z_d)
Raised, a tempietto 13.98 across (r 6.99) with its door west, facing the Salon. Everything stays inside r 6.99.
- **Wall** `rotunda.sun-clock.wall`: face r 6.900 from the base top (z_d −3.08) to the architrave (−0.45); inner face
  `rotunda.sun-clock.wall.inner` r 6.545 from −3.56 to the rim groove (−0.06). **Plinth** r 6.99, z_d −3.56 → −3.50 all
  round (under the door too); **foot** a flat annulus r 6.545–6.99 at −3.56, bearing on the rams.
- **Base** (not across the door), (r, z_d): (6.990, −3.500) (6.990, −3.300) (6.955, −3.300) arc(c 6.955, −3.265;
  r 0.035; −90→90°) (6.955, −3.230) (6.925, −3.230) (6.925, −3.215) (6.915, −3.215) arc(c 6.915, −3.200; r 0.015; −90→90°)
  (6.915, −3.185) (6.905, −3.175) (6.905, −3.100) (6.900, −3.080).
- **Entablature** (all round): (6.900, −0.450) (6.930, −0.450) (6.930, −0.395) (6.940, −0.390) (6.940, −0.335)
  (6.952, −0.330) (6.952, −0.315) (6.915, −0.315) (6.915, −0.195) arc(c 6.915, −0.150; r 0.045; −90→0°) (6.960, −0.150)
  (6.990, −0.150) (6.990, 0.000): the corona is the dial's edge.
- **Door** `rotunda.sun-clock.door`, west (φ 180°): 2.60 wide at the wall face, 2.95 high above the sill (z_d −3.50 →
  −0.55); radial jambs (half-angle 10.860°); conical head, a semicircle of radius r·sin 10.860° at each radius (1.300 at
  the face, 1.233 at the inner face), springing z_d −1.85; sill `rotunda.sun-clock.door.sill` flat r 6.545–6.99 at −3.50.
  Archivolt down both jambs to z_d −3.10, centre line 1.355 from the axis; (across, proud): (0.045, −0.010)
  (0.045, 0.036) arc(c 0.033, 0.036; r 0.012; 0→180°) (0.021, 0.036) (0.010, 0.036) (0.010, 0.030) (−0.045, 0.030)
  (−0.045, −0.010). Imposts 1.300–1.430 off the axis, springing −0.07 → +0.01, 0.06 proud. Keystone: trapezoid 0.15 wide
  at z_d −0.545 to 0.20 at −0.440, 0.07 proud.
- **12 pilasters** `rotunda.sun-clock.pilaster` at φ = 15° + 30°·k (165° and 195° flank the door): pedestal 0.50 wide,
  0.075 proud, z_d −3.10 → −2.98; fluted shaft 0.44 / 0.055 / −2.99 → −0.61 (5 flutes 0.0632 between fillets 0.016,
  margins 0.03, each a circular arc sagging 0.018); necking 0.47 / 0.064 / −0.62 → −0.56; echinus 0.49 / 0.071 / −0.56 →
  −0.53; abacus 0.51 / 0.080 / −0.53 → −0.44.
- **Soffit** `rotunda.sun-clock.soffit` (the stair's ceiling): ring ribs at z_d −0.13 at r 0–0.55, 1.25–1.35,
  2.25–2.35, 3.45–3.55, 4.75–4.85; coffer rings r 0.55–1.25 (8 coffers, radial ribs 6.5625°), 1.35–2.25 (12, 2.8125°),
  2.35–3.45 (16, 1.875°), 3.55–4.75 (24, 0.9375°), 4.85–6.20 (32, 0.9375°); each coffer sunk from the rib (−0.13) to a
  40 mm inset frame at −0.115, then the panel at −0.100.
- **Rim** `rotunda.sun-clock.rim` (bronze ring, (r, z_d)): (6.190, −0.090) (6.190, −0.125) arc(c 6.205, −0.125;
  r 0.015; 180→270°) (6.205, −0.140) (6.320, −0.140) (6.330, −0.132) (6.340, −0.140) (6.460, −0.140) (6.460, −0.060)
  (6.545, −0.060): the deep groove r 6.46–6.545 takes the sleeve's top (−0.07) when lowered.
- **Pendant boss** `rotunda.sun-clock.pendant` over the well (lathe, (r, z_d)): (0, −0.265), low dome r = 0.11 sin t,
  z = −0.265 + 0.04(1 − cos t), t 0→90° to (0.110, −0.225), bead arc(c 0.125, −0.225; r 0.015; 180→0°) (0.140, −0.225)
  (0.140, −0.215) (0.240, −0.190) (0.340, −0.170) (0.400, −0.158) arc(c 0.400, −0.148; r 0.010; −90→90°) (0.400, −0.138)
  (0.400, −0.120).
- On top: the dial (§3.1). Real section (polish): a welded steel carcass faced in stone, tens of tonnes; tonight a
  solid core (≈ 200 t).

### 3.7 The shaft (fixed, in the served well)
- **Stair wall (sleeve)** `rotunda.stair.wall`: face r 6.48, back r 6.54, landing (z −6.00) to top z −0.07; cut on
  the north by the portal (jambs x = ±1.80, head at the lintel, z −3.68). Lintel (underside −3.68, r 6.48 → 7.05) and
  tympanum (on r 7.05, from −3.68 to the arch) are the same surface.
- **Pocket** `rotunda.shaft.pocket`: annulus r 6.54–7.00, floor z −3.58, open up to z 0; outer face r 7.00, −3.58 → 0;
  includes the sleeve's top and back.
- **Masonry:** r 7.00–7.60 above the pocket floor, r 6.54–7.60 below it; outer face r 7.60, z −6.00 → 0, cut by the
  port's arch (reveal `rotunda.stair.port.reveal`, r 7.05 → 7.60) at the class's clear opening, straight: its jambs the
  portal's planes x = ±1.80, not radial, so they meet the Classical Hall's lining flush at r 7.60.
- **Ram bores** (housing `rotunda.sun-clock.rams`): 8 vertical bores Ø 0.36 centred on r 6.77 at φ = 22.5° + 45°·k,
  pocket floor (−3.58) down to z −5.90; they clear the portal (±15.42° about 270°) and the drum's door (±10.86° about
  180°). Rams `rotunda.sun-clock.ram` (§6).
- **Stair-side port dressings** `rotunda.stair.port.dressing` (polish): an arch ring 0.18 wide, 0.14 proud; a moulded
  architrave out to 0.40, crown 0.20 proud; plinth blocks 0.40 high; a keystone; moulded transoms on the lintel.
  They are the port's surround on the stair side, so the joint draws none there; **until polish they are not drawn**, and
  the portal's face on the stair is the sleeve's plain cut.
- **Landing** `rotunda.stair.landing.floor` at z −6.000 from r 0 to the stair wall and through the portal to r 7.57: it is
  the stair port's threshold, stopped half the class's 0.060 strip short of the port face, so the joint's strip lies on
  the mating arc and the joint lays no second stone on this side (K10).
  Rosette: pale eye r 0–0.12; black disc r 0.12–0.30 (edge a 32-gon through the points' roots and valleys); a 16-point
  star, long points to r 1.80 at φ = 45°·k, short to r 1.20 at φ = 22.5° + 45°·k, each split on its axis, black on the
  lower-φ half of the long points and red on the other (the short points the other way round); pale field between the
  points to r 1.90; band r 1.90–2.00 black `rotunda.stair.landing.black`; band r 2.00–2.06 red `rotunda.stair.landing.red`;
  pale landing r 2.06 → 6.48.

### 3.8 The stair (after Momo and Bramante: one wide helical flight round an open well Ø 4.40)
Descends anticlockwise in plan (φ decreasing) from the drum's west door round by the south and east to the landing,
whose portal opens north.

| | |
|---|---|
| Risers | 40 × 0.150 = 6.00. Riser 1: the raised drum's sill (z 0) to the top landing. |
| Top landing | tread 1, z −0.15, φ 194° → 164° (spans the drum's door) |
| Treads 2–19 | z −0.30 … −2.85, each turning α = (214° − 20°)/37 = 5.2432° (0.091512 rad) |
| Mid landing | tread 20, z −3.00, β 20°, φ 69.622° → 49.622° |
| Treads 21–39 | z −3.15 … −5.85, each α |
| Last riser | at φ −50° (310°) onto the landing floor z −6.00: the foot faces the portal across the landing |
| Nosing lines | Front(1) = 164°; Front(K) = Front(K−1) − α, except Front(20) = Front(19) − β; tread K spans Front(K) → Front(K−1) |

- Treads `rotunda.stair.tread` (one stone each, flat) span r 2.35 → 6.48 (4.13), running 10 mm into the string and
  the wall. Goings: 0.215 at r 2.35; **0.330 at r 3.61** (2R + G = 0.630: the declared walking line); 0.403 at 4.40;
  0.593 at 6.48. Nosing over each riser, top edge 22 mm forward of the riser line; section (forward, up from the
  tread top): (0, drop − 0.01) (0, −0.050) (0.022, −0.040) arc(c 0.022, −0.020; r 0.020; −90→90°) (0.022, 0.000).
- **Inner string** `rotunda.stair.string` r 2.20–2.35: top 0.12 above the pitch line (through the treads' mid-going,
  flat on the landings), never above z −0.14 (under the lowered dial); flush with the top landing; runs 0.5° past the
  last riser.
- **Soffit** `rotunda.stair.soffit`: a smooth helicoid 0.40 under the pitch line, never below −6.02, into the landing
  at the foot; r 2.20 to the wall.
- Collision from the treads and landings themselves (the as-built hidden ramp is not ported).

### 3.9 Fittings: balustrades, housings, handrails
No benches, labels, signs or floor map ("There is no floor map"). Blind doors: §2.
- **Well balustrade** `rotunda.stair.balustrade`, plane r 2.15, side-fixed by brackets to the string; heights above
  the string top: bars at 0.0575, 0.5325, 0.8675 (25 × 20 mm); standards 30 × 30 at ≤ 1.6 and every kink, down to
  −0.25 onto brackets; balusters 14 mm square ≈ 0.110 apart between the bottom and mid bars; between the mid and top
  bars, rings touching both (radius ≈ 0.155·cos rake; 12 mm band, 12 thick, boss r 0.022, four 7 mm spokes at 45°);
  handrail `rotunda.stair.handrail` at 0.90, a soft bar 60 × 45 (superellipse, exponent 0.7). Newel at the foot:
  post 56 mm square to handrail + 0.03, cap 72 × 20, ball finial r 0.045. The run ends 1° past the last riser.
- **The split** at tread 8's nosing. Below it the balustrade is **fixed** (its handrail top at −0.23 there, under the
  lowered dial's ribs −0.13). Above it, and across the top landing's back edge (φ 194°, plane 0.07 behind it), it
  **retracts** (body `rotunda.stair.balustrade.panels`): each run a free panel (standards, balusters, rings, handrail,
  a carrier bar at the foot −0.22 … −0.18) in a hollow housing: a U channel 1.20 deep, 30 mm floor, 40 mm end blocks,
  slot 76 mm; along the string r 2.07–2.20 (top flush with the string), behind the landing 0–0.14 behind its back edge
  (top flush with it). Sides `rotunda.stair.housing.side` (sunk fields −0.10 … −1.09 where seen); cap round the slot
  0.06, shoe 0.05 and end plates `rotunda.stair.housing.cap`. At home a panel sits 0.94 lower, wholly inside, its
  handrail just under the slot's lips; 5 mm clear to its end blocks.
- **Wall handrail** `rotunda.stair.handrail`: round Ø 0.050 at r 6.40, 0.92 above the pitch line, brackets every 1.25
  (from 0.30); from tread 7's nosing (returned to the wall on a rose, top at −0.18, under the lowered dial) to 4° past
  the last riser. The top flight's wall side has no rail (the well side guards it).

### 3.10 The exterior dress (polish; recorded so nothing is lost)
The Rotunda owns both faces of its shell (the Grounds hand the dress over). From `MuseePlan.h:518-561` and
`MuseeFacadeStructure.cpp:600-744`: a rusticated podium 2.00 high, 0.54 proud, then gravel and a stone edge to r 12.76
on the free arcs; 0.20 travertine ashlar skin over r 11.20 framing the door arches; `order.ionic-vignola` D 0.827,
entablature z 9.44–11.30 with dentils, attic to 12.15 under a small cornice, a wash in to the dome; pilaster pairs at φ
33.75, 56.25, 123.75, 146.25, 213.75, 236.25, 303.75, 326.25; blind arched windows on the diagonals (1.50 wide, sill
3.2, springing 6.2, frame 0.20, transom 0.75, round tablet r 0.38 at z 8.2); dome cover R 11.10 about z 10.00 with two
0.40 step rings (outer r 11.25, 10.98), smooth to r 3.0, a rolled ring r 2.92–3.28 on the curb at 20.45; eight night
floodlights grazing the pilasters (a `gap.exterior-uplight` when built). Seen from Chenghuai's 見山 pavilion as "the
southern mountain".

## 4. Light

**Daylight**

| Opening | Glazing | Weather | Control | Notes |
|---|---|---|---|---|
| `rotunda.eye` (clear Ø 5.00, 19.6 m², lattice ≈ 24 %) | `glass.low-iron`, laminated UV-filtering | **museum** | none | Sun patch = the eye's disc projected along the idealised sun, centred on the node's shadow; reaches no further than r 9.28 (never the wall or niches). First-order daylight factor on the floor ≈ 1.3 %. Borrowed light through the four portals. No blinds (D-014; the works are casts). |

**Luminaires** (Lighting.json; drum lights with the drum lowered, add 3.500 raised; no output values are set here)

| Id | Type | Count / extent | Mount and aim | Operation |
|---|---|---|---|---|
| `dome.cove.01…28` | `linear-cove.3000k` strip | 28 lengths of 2.10 on the cornice's top ledge, one per coffer bay, midpoints at φ = 12.857°·(c + ½) | ledge (r 9.40, z 10.504), hidden from every standable point behind the corona (r 9.22); aim straight up onto coffer rings 1–3 (r 9.40, z 13.40) | dusk (daylight-linked) |
| `cast.<niche>.a/b` | `track-spot.3000k.12deg` | 8: two per cast, across the room at φ_n + 160° and φ_n + 200° | head on a short stem on the ledge at r 9.30, z 10.60 (beam clears the corona's edge); aim the cast's centre (r 10.25 on its axis, z 2.00 + h/2), throw 19.25, ≈ 21° down, entering its niche within 0.05 of the axis | presence |
| `floor.spot.01…16` | `track-spot.3000k.38deg` | 16 at φ = 11.25° + 22.5°·k | ledge r 9.30, z 10.60; aim the floor at r 4.00, φ + 180° | dusk (daylight-linked; holds the floor target) |
| `stair.soffit.01…12` | `downlight.3000k.31deg`, recessed | 12, alternate coffers of the soffit's 4th ring | (r 4.15, φ = 7.5° + 30°·k, z_d −0.108); aim straight down onto the treads | always, follows `rotunda.sun-clock.lights`; body `rotunda.sun-clock.drum` |
| `stair.well` | `downlight.3000k.31deg` | 1 | under the pendant (0, 0, z_d −0.29); aim straight down the well onto the rosette (9.21 below when raised) | always, follows `rotunda.sun-clock.lights`; body `rotunda.sun-clock.drum` |

The cornice spots' heads may show a few centimetres over the corona from the far side of the floor, dark against the
lit dome, as track on a cornice does. The as-built fill light (a point with no fitting, `SunClock.h:145-151`) is not
ported. Aim points of the drum lights are given in the lowered frame at z_d −9.50 (the landing when raised).

**Targets**

| Surface | Class | Lux | Source |
|---|---|---|---|
| `rotunda.floor.ring.a`, `rotunda.dial.field.a` (the floor r 0–10) | entrance-hall / circulation.night | 100 / 50 | UseTargets |
| `rotunda.drum.wall.inner` | entrance-hall (wall min) | 75 | UseTargets |
| `rotunda.dome.coffer` | entrance-hall (ceiling min) | 50 | UseTargets |
| `rotunda.stair.tread`, `rotunda.stair.landing.floor` | stairs (whenever raised, day and night) | 100 | UseTargets |
| `rotunda.stair.wall` / `rotunda.sun-clock.soffit` | stairs (wall min / ceiling min) | 75 / 50 | UseTargets |
| `rotunda.niche.cast` | stone-and-bronze (plaster: no class; flagged §13) | 300 recommended, no maximum (JSON [300, null]) | Care |

No walked route in any configuration falls below `escape-route`. **Design order** (Lighting.json `design`; measured,
never tuned): by day the eye's glass is the brightest surface in view, then the sun patch; the dial outside the patch ≥
the drum's inner face ≥ the coffers (darkest round the eye). At night the cove-washed coffers ≥ the drum; the eye is
dark. Each cast ≥ 1.3 × its niche back, always. On the stair: treads ≥ stair wall ≥ drum soffit.

## 5. Time and weather
- `rotunda.eye` takes the **museum** observer (D-005): its rain wets the eye's outer face, the lead and the outer drum
  (Matter's surface law). The sun and the hour are always the museum's.
- **Idealised sun** (D-003, `Data/Time/Museum.json` model `idealised`): nodus = the node's centre (0, 0, 20.25); dial
  plane z 0 (the lowered dial); shadow circle r 6.78 (the hour ring); constant elevation atan(20.25/6.78) = **71.489°**
  (the old sky used 20.30, 71.531°: the shadow fell 1.7 cm inside the ring). At civil local hour h, 06:00 ≤ h ≤ 18:00:
  shadow point S(h) = 6.78·(cos φ_h, sin φ_h, 0), φ_h = 180° + 15°·(h − 6); the sun's direction is from S(h) through the
  node; its bearing 90° + 15°·(h − 6) (east at 06:00, south at noon, west at 18:00). Outside 06:00–18:00 no sun; moon
  and stars from the Meeus ephemeris.
- **Time::Dial** lays the hour lines, meridian, ticks and numerals from its output (nothing typed by hand);
  **Time.DialTrue** checks every hour and minute tick on the lowered configuration. The raised dial is 3.50 up and is
  not a dial there.

## 6. Motion

**M-1 `rotunda.sun-clock.lift`** (Mechanisms.json)
- **Body** `rotunda.sun-clock.drum`: the dial, inlay, meridian, gilt sun, drum wall and dressings, soffit, rim, pendant,
  13 downlights. **Joint** prismatic +z, travel **0 → 3.500**; states `lowered` (0) and `raised` (3.50).
- **Drive:** eight synchronised three-stage telescopic hydraulic rams in the bores (r 6.77, φ 22.5° + 45°·k), from
  z −5.90 up to the pocket floor, heads bearing on the drum's foot ring (z_d −3.56); closed height ≤ 2.30; no
  counterweight; each rated for ⅛ of the drum with margin. **Profile** ease-in-out, **8.0 s** each way, as built
  (`SunClock.cpp:117-121`, D-017); peak speed ≈ 0.66 m/s.
- **Use** (Use anchor `rotunda.sun-clock.control`; the prompt shows only with the feet on the ring, 7.35 < r < 10.00,
  |z| < 0.40):

| State | Verb | Condition | Message |
|---|---|---|---|
| lowered | "Raise the sun clock" | on the ring; refused while the user is on the dial (r ≤ 7.35, −0.30 < z < 0.60) | "The sun clock rises…"; refused: "Step off the sun clock to raise it."; on arrival: "Under the sun clock, a stair winds down into the earth." (cleared once the user is 1 m below) |
| raised | "Lower the sun clock" | on the ring, and nobody inside r 7.20 or below z −0.30 | none |

- **Guard (Motion, D-014)** `body-in-travel`: raising **holds** while any body is on the dial; lowering **reverses** to
  raising if a body enters r < 7.20 or goes below z −0.30 (as built). The lift never lifts a visitor and never lowers
  onto one; there is no control at the landing, and the lowered state is unreachable with anyone below.
- **Travel volume:** the cylinder r ≤ 6.99 from z −3.56 (lowered foot) to +3.50 (raised dial), plus the pendant and
  downlights under the soffit.
- **Clearances** (≥ 5 mm at every sampled phase): drum inner face 6.545 ↔ sleeve back 6.54: **5 mm**; drum's outermost
  6.99 ↔ pocket face and ring edge 7.00: 10 mm; lowered foot −3.56 ↔ pocket floor −3.58: 20 mm; soffit ribs −0.13 ↔
  string top ≤ −0.14: 10 mm; rim underside −0.14 ↔ top landing −0.15: 10 mm; groove roof −0.06 ↔ sleeve top −0.07:
  10 mm; ribs ↔ fixed balustrade handrail (≤ −0.23): ≥ 100 mm; ribs ↔ wall rail top (−0.18): 50 mm; the downlight at
  φ 187.5° (−0.108 plus trim) ↔ top landing: ≥ 30 mm.
- **Channels:** publishes `rotunda.sun-clock.height` (0…1). Sequence: open = [`rotunda.sun-clock.lights` → 1, move
  open]; close = [move closed, `rotunda.sun-clock.lights` → 0]. The 13 drum lights follow `rotunda.sun-clock.lights`
  (as built on above 0.30 m of lift; they sit in the closed pocket below that).
- **Sampled configurations:** lowered, raised, and 0.30, 1.20, 1.67, 2.14, 3.00 m (the cam's knees, the lights'
  switch): Form.Sealed, Motion.Clears and Site.Reachable run on each.

**M-2 `rotunda.stair.balustrade`** (coupled): body `rotunda.stair.balustrade.panels`, prismatic +z, travel 0 → 0.940,
**follows** `rotunda.sun-clock.height`, in [0.342857, 0.611429] (1.20 → 2.14 m of 3.50) → out [0, 0.94], clamped
(the as-built cam: offset = clamp(h − 1.20, 0, 0.94) − 0.94). Home until the dial is 1.20 up; standing from 2.14 up;
lowering, home before the drum comes within 1.07. Nothing rides under the moving ceiling.

## 7. Nature
None: no plants, no water, no ground. The Rotunda is Enclosed; precipitation stops at the eye's glass and the lead
(ponding on the flat eye inside its 60 mm upstand is Matter's surface law over Form's ponding field).

## 8. Art
Four plaster casts, one on each diagonal niche's sill, from SMK's Royal Cast Collection (public-domain 3D scans).
Catalogue: `data/classical.json` (ids verbatim, captions: title, artist, year, original, notes); scans and true heights
from `assets/classical/SOURCES.json` (kind `rotunda`); widths × depths are the converted glTFs' bounds
(`assets/classical/<id>.glb`, measured). The casts are not in `assets/collection.json`.

**Hang rule** (as built, `classical_art.py:262-279`): each cast's vertical axis at **r 10.25 on its niche axis**, 0.25
behind the drum's inner face, feet on the sill at **z 2.00**, front turned to the gilt sun (facing φ + 180°), scaled to
its recorded height (Art.TrueSize). No frames, cases or plinths: the sill is the plinth. Care: unpainted plaster,
class `stone-and-bronze` (flagged). Hang.json: anchor = the niche's Mount anchor; a Wall frame doesn't fit a cast in
the round, so each entry carries `wall` `rotunda.niche.sill`, `centre` (the foot of the axis), `height` 2.00,
`facing_deg`, and its `scan`.

| Anchor | φ | Work (id) | Title · artist · date (original) | Cast | w × d × h (m) | Centre (x, y, z) | Scan |
|---|---|---|---|---|---|---|---|
| `rotunda.niche.ne.cast` | 315° | classical-dying-slave | *Dying Slave* · Michelangelo · 1513–1516 (Louvre) | SMK KAS87 | 0.804 × 0.739 × 2.28 | (7.248, −7.248, 2.00) | `dying-slave` |
| `rotunda.niche.se.cast` | 45° | classical-rebellious-slave | *Rebellious Slave* · Michelangelo · 1513–1516 (Louvre) | SMK KAS86 | 0.787 × 0.538 × 2.13 | (7.248, 7.248, 2.00) | `rebellious-slave`, turn [[0,0,1],[0,1,0],[−1,0,0]] |
| `rotunda.niche.sw.cast` | 135° | classical-venere-italica | *Venere Italica* · Antonio Canova · 1804–1812 (Palazzo Pitti) | SMK KAS793 | 0.464 × 0.528 × 1.73 | (−7.248, 7.248, 2.00) | `venere-italica` |
| `rotunda.niche.nw.cast` | 225° | classical-bacchus | *Bacchus* · Michelangelo · 1496–1497 (Bargello) | SMK KAS83 | 0.866 × 0.541 × 2.08 | (−7.248, −7.248, 2.00) | `bacchus`, turn [[−1,0,0],[0,1,0],[0,0,−1]] |

**Fit:** the deepest (Dying Slave, 0.739) spans r 9.88–10.62: on the ledge (from r 9.84) and inside the half-round
(r 10.92 at its lateral ±0.40); the tallest top (4.28) stays under the springing (4.67). Form.NoOverlap checks each
scan against its niche. Surface `rotunda.niche.cast` (plaster of Paris) is the casts' finish. **Credit** (Art): "Cast:
SMK – National Gallery of Denmark, The Royal Cast Collection, KAS87" (and so on); scans public domain (SMK Open). No
physical labels; captions by gaze. The sun clock's letters are architecture, not a work.

## 9. Sound
- **Acoustics** (computed from Matter, never tuned): `rotunda.hall` ≈ 5,240 m³ of honed marble, lime plaster, glass
  and the four open portals (≈ 83 m² to the neighbours): very live, several seconds at 500 Hz. `rotunda.stair` its own
  enclosure, a 13 m stone cylinder 6 m deep; coupled through the drum's door while raised. Occlusion through the four
  portals.
- **Emitters:** footfalls by substance (honed Crema Marfil on ring, dial, treads, landing; bronze on inlay and the gilt
  sun); the lift's voice (hydraulic pump, the drum's stone sliding in its pocket) on `rotunda.sun-clock.drum`, driven by
  FMuseeActuation; rain on the eye's glass (19.6 m²) from the **museum** observer. (No sound as built.)

## 10. Visitor
- **Arrive** on the gilt sun facing west: four doors, four chapters: the Salon (west), Chenghuai (north), Albion
  (south), the Hall of Light (east, "the way on").
- **Look:** up at the coffered dome and the pearl lattice eye; at the point of shadow walking the numerals VI → XII →
  VI. The hour key T moves the hour and the shadow with it; the city key changes nothing on the dial (D-002, D-003).
  Gaze at a cast: its caption (title, artist, date, original, SMK cast number).
- **Use** (§6): from the ring, "Raise the sun clock"; through the tempietto's west door and down 40 risers to the
  Classical Hall; "Lower the sun clock", from the ring, once everyone is out.
- **Walk:** ring, dial, treads, landings; the body climbs the real treads (0.150 risers). The raised drum's sill (z 0),
  the sleeve's 60 mm top (−0.07) and the top landing (−0.15) make a 0.07 + 0.08 step at the door.
- **Step-to (VR):** ring, dial, treads, landings; never the raised dial's top, the niche sills or the cornice.
- **Sit:** nowhere (the design has no seat here).

## 11. The old-code map
Paths relative to `windows\`. Re-express, never copy.

| Part | Where |
|---|---|
| Arrival (gilt sun, yaw 180) | Source/MuseeVision/Plan/MuseePlan.h:24-28 |
| Rotunda numbers (drum, doors, niches, coffers, eye, node, shadow r, passages) | …/Plan/MuseePlan.h:30-64 |
| Sun clock numbers (radii, lift 3.5 in 8 s, door, shaft, pocket, landing, port) | …/Plan/MuseePlan.h:119-158 |
| Neighbours: Hall of Light meets the drum; Classical Hall meets the shaft at r 7.6; façade dress | …/Plan/MuseePlan.h:258-299, 461-509, 518-561 |
| Constants (segments, shadow gap, pilaster/flute heights, cornice top, shell R, curb, lattice ring) | Source/MuseeVision/Rotunda/RotundaStructure.cpp:24-60 |
| Openings round the drum; north door switch | …/RotundaStructure.cpp:281-313; Chenghuai/ChenghuaiPlan.h:25, 38 |
| Drum stations, wall faces, shadow gap | …/RotundaStructure.cpp:315-455 |
| Reveals, niche recesses, half-domes | …/RotundaStructure.cpp:457-533 |
| Archivolt, imposts, niche surround, niche ledge | …/RotundaStructure.cpp:535-653 |
| Passage shell (now Site's) | …/RotundaStructure.cpp:655-670 |
| Flutes, base, capital, pilaster; door-flank rule | …/RotundaStructure.cpp:672-791, 793-815 |
| Dome, coffer elevations, entablature, plain bands, eye curb; coffers | …/RotundaStructure.cpp:817-894, 896-961 |
| Drum top, outer shell | …/RotundaStructure.cpp:963-1015 |
| Lattice, glass, node; assembly; as-built materials | …/RotundaStructure.cpp:1036-1081, 1089-1135, 1166-1173 |
| Sun clock: description, clearances, retracting balustrade, as-built light | Source/MuseeVision/SunClock/SunClock.h:14-62, 137-206 |
| Ring and inside zones; lift easing; lights and switching; prompts, interlocks, reversal | …/SunClock/SunClock.cpp:17-25, 117-121, 256-310, 312-404 |
| Drum datums; stair radii and balustrade cam; luminaire positions | …/SunClock/SunClockBuild.h:40-54, 118-151, 153-160 |
| Drum door; north port; stair layout, pitch, string, soffit lines | …/SunClock/SunClockKit.cpp:7-20, 58-79, 82-160 |
| Dial bands; ring, fillets, hour lines, ticks; numerals; sunburst and boss; ring courses and motto | …/SunClock/SunClockDial.cpp:19-53, 55-84, 86-107, 109-165, 175-233 |
| Drum profiles; walls and door; door dressing; pilasters; soffit coffers; rim and pendant | …/SunClock/SunClockDrum.cpp:25-45, 47-128, 160-201, 203-255, 257-346, 348-373 |
| Shaft walls, pocket, port, tympanum, lintel; port dressings; landing and rosette | …/SunClock/SunClockShaft.cpp:44-93, 217-291, 293-337 |
| Treads and nosings; string, soffit (ramp: **do not port**) | …/SunClock/SunClockStair.cpp:59-93, 95-145 |
| Balustrade registers; uprights, rings, handrail; housings and panels; split, newel, wall rail | …/SunClock/SunClockRails.cpp:33-36, 105-222, 224-370, 430-517 |
| As-built clearance, step and overlap checks at several lifts (evidence, D-017) | …/SunClock/SunClockChecks.cpp:101-526 |
| Letter outlines and their font | …/SunClock/SunClockGlyphs.h; bake_glyphs.py:1-40 |
| Idealised sun direction; 06:00–18:00 window | Source/MuseeVision/Sky/MuseeSky.cpp:109-116, 133-138 |
| Thresholds (now Site's) | Source/MuseeVision/Furniture/MuseeFurniture.h:29-32; MuseeFurniture.cpp:301-311 |
| Exterior dress (polish) | Source/MuseeVision/Facade/MuseeFacadeStructure.cpp:600-673, 702-744, 781-786 |
| Neighbours at the ports | Salon/SalonStructure.cpp:28, 41; Chenghuai/ChenghuaiPlan.h:20-38; Albion/AlbionPlan.h:95-101; HallOfLight/HallOfLightStructure.cpp:48-83 |
| Casts: niche hang; scaling to height | Scripts/classical_art.py:262-279, 109-140; repo assets/classical/SOURCES.json, data/classical.json |
| Board and words | repo plan/canvas/Rotunda.dc.html (meridian drawn 3.5 px against 1.5 px hour marks), plan/README.md:75-81 |

**Corrections to the draft** (`r-specs/…/Stages/rotunda.md`):
1. **Materials are palette keys** (D-018, Story §6): floor ring, dial fields, treads, landing, string and housing sides
   are **Crema Marfil in two selections** (draft: statuary vein-cut/cross-cut; statuary's roles are the Classical
   Hall's); the red band **Rojo Alicante** (draft: rosso antico); entablature **fibrous plaster** (draft: lime plaster
   run); niche surrounds and sills **Crema Marfil** (draft: plaster); drum soffit **lime plaster** (draft: fibrous);
   eye lattice **pearl steel** (draft: dark paint #1E1C19); node **patinated bronze** (draft: gilt); outer dome
   **milled lead** (draft: travertine); handrails patinated, hand-polished (draft: brushed). The whole sun (rays too) is
   gilt.
2. **Light:** every luminaire is a 3000 K catalogue type (the draft wanted 4000 K and flagged a cove gap; the catalogue's
   cove is 3000 K, and the sibling Nymphéas stair is 3000 K) — no gap. The draft's cast spots on the cornice ledge "about
   30° from vertical" cannot reach a niche: from the ledge every beam towards r > 9.58 falls onto the entablature
   itself, so the cast spots stand across the room (φ_n ± 160°/200°). Floor spots are given explicitly (draft: "a ring,
   by Scheme"). The draft's 150–300 lx "modelling aim" on the casts is replaced by the Care class value (300,
   recommended). The as-built fill light is dropped (as the draft said).
3. **Channels** are 0…1 (the contract): `rotunda.sun-clock.height` and `rotunda.sun-clock.lights` replace the draft's
   metre-valued `rotunda.sun-clock.lift`; the balustrade follows it as in/out ranges.
4. **Well prism** top z 0.00 (the shaft's outer face r 7.60 runs to z 0 as built, `SunClockShaft.cpp:68`, and the
   Classical Hall draft reserves −6.40…0.00), not −0.55.
5. **Class names** use width × crown (the Nymphéas convention): the draft's `doorway.3000x4500.arched`,
   `doorway.4000x4500.arched`, `stairwell.spiral.3600` → `doorway.3000x6000.arched`, `doorway.4000x6500.arched`,
   `stairwell.spiral.3600x4400.arched` (geometry unchanged).
6. The landing floor runs through the portal to the port face **r 7.60** (as built), not to 7.05.
7. Treads run 10 mm into the string and the wall; the string runs 0.5° past the last riser; the well balustrade ends 1°
   past it (as built; the draft was silent).
8. Motion's time kept at the as-built 8.0 s (the draft proposed a 7.9 s S-curve); the lights switch through a channel step at the
   sequence ends instead of at 0.30 m (inside the closed pocket there, so unseen).
9. **Verified unchanged:** drum 10.00/11.20, doors and flank angles (11.776°, 14.663°), niches (2.00, 2.00/4.67, sliver
   0.050, ledge ±1.26), every profile point list, coffers table, curb, shell 10.60 (foot 10.553/11.00, 2.90/20.196),
   lattice (bar lengths 4.920/4.653/3.737/1.080), glass 20.38–20.39, node 20.25, dial bands, inlay sections, numerals,
   motto, sun, drum profiles, door (10.860°, springing −1.85), soffit ribs and coffers, rim, pendant, shaft radii, port,
   rosette, stair layout (Front(39) = −50.000°), goings, balustrade registers, cam, clearances, cast ids, heights and
   scan bounds.

## 12. The first-pass cut
**Tonight:** Site (frame, footprint, the five ports with classes, blinds and the served well, the arrival point,
enclosures and openings); the drum with 4 rough openings, 4 niches (reveal, back, half-dome, surround, sill) and the
shadow gap; outer face and top; 16 pilasters; the entablature; 4 archivolts; the dome (bands, 5 × 28 coffers, curb,
shell with its lead); the eye (ring, 21 bars, laminated pane, node); the ring courses, dial bands, hour ring, fillets,
hour lines, the gilt meridian, minute ticks, the gilt sun (rays, boss, plain beads); the lifting drum (wall, base and
entablature, 12 pilasters, door with archivolt, soffit coffers, rim, pendant); M-1 and M-2 with the guard, prompts and
channels; the shaft (sleeve, pocket, masonry, port with lintel and tympanum, 8 bores with plain rams); the landing and
rosette; the stair (treads, nosings, string, soffit); the balustrade (fixed part with bars, balusters, handrail; the
retracting panels in their housings; the wall rail; the newel); the luminaires of §4 and daylight through the eye; the
four casts; acoustics and footfalls. Numerals and motto from the ported `SunClockGlyphs.h` outlines. If Form's Laid isn't ready, the Crema Marfil faces are solids of their visible stone
and Form.WithinStock findings are listed, not waived.

**Waits for polish:** the exterior dress (§3.10); the balustrade's rings and bosses (plain balusters full height
tonight); the drum door's keystone and imposts; the stair-side port dressings; the flames' weave and the boss's beaded
foot; the drum's steel carcass; the lift's voice and rain on the eye; falls for the eye's glass; path-traced daylight.

## 13. Palette keys to confirm and catalogue gaps
**Palette keys** (roles guessed or workings outside `workingsByEra`):
- `steel.hard-chrome` (ruled by the Architect; Matter adds it) → hard-chromed carbon-steel hydraulic ram rods (hidden
  in their bores).
- `stone.crema-marfil` **two selections** (Story: "two selections for the two tones"): A = the paler ground with sparse
  crackle, B = the warmer ground with fuller crackle; which is which is guessed. Used also for the **door archivolts**
  (as built plaster; roles name only niche surrounds and sills), the **stair soffit, string, landing, pocket and port
  dressings** (extending "spiral stair" and "wall ashlar").
- `bronze.cusn10` + `gilt.gold-leaf.oil` on the **32 rays** ("gilt sun" read as the whole sunburst; as built the rays
  were bronze), the **motto stops** (as built gilt) and the **pendant boss** (as built gilt): guessed.
- `bronze.cusn10` patinated for the **drum rim, housing caps and the five blind-door pairs**: by analogy (drum
  balustrade; the Grounds' blind door).
- `stone.travertine-romano` **sawn** for the hidden rough-opening faces (a core working, not in workingsByEra).
- Workings written as compound strings: `pearl.satin` (white.pearl-steel), `laminated.uv-filtering` (glass.low-iron),
  `cast.seams-kept.sized` (white.plaster-of-paris), `run` (lime plaster coffers and drum soffit).
- Landing rosette: the palette names Crema Marfil / Rojo Alicante / Negro Marquina for "the Classical Hall's landing";
  the Rotunda builds that landing inside its served well with the same keys.
- **Care class of the casts:** plaster has no class; `stone-and-bronze` used (brief), but Care.json says a surface with
  an organic coating takes its coating's class, and the Story seals the casts with size. Confirm.

**Catalogue gaps** (Light): (1) a **control class that follows a mechanism channel** for the 13 drum luminaires
(written `always` + `follows`); (2) when the exterior dress is built, an exterior uplight (`gap.exterior-uplight`) for
the eight floodlights. No other gap.

**Conflicts and questions for the Architect:**
(a) **West port** (ruled): mated ports share one class. `salon.east` is `doorway.3000x6000.arched`, the same as
`rotunda.west`, and the Salon steps down to its own 4.80 arch inside its east wall (salon.md §2). The joint's 2.20 m
passage is a plain drum-section arch.
(b) **Class naming**: width × crown (Nymphéas) vs width × springing (the Rotunda, Albion, Chenghuai and Hall-of-Light
drafts). This sheet uses width × crown; `hall-of-light.west`, `chenghuai.south`, `albion.north` and
`classical-hall.stair` must use the same names.
(c) **North**: Chenghuai's porch is 3.80 wide inside (PorchHalf 1.9) against the 4.00 door; the joint's surround must
sit within it.
(d) Story choices over the as-built: pearl lattice (as built dark), lead dome (as built stone), patinated node (as built
gilt), gilt meridian 60 mm (as built none: "No meridian", `SunClockDial.cpp:13`). Kept per D-018 and the board.
(e) **Glyphs** (ruled): port the as-built Centaur outlines from `SunClockGlyphs.h` (Centaur is installed on this PC); a
public release would switch to an open-licence face.

## Build notes
- **Frame.** The stage origin **is** the museum origin. The arrival point is the gilt sun at (0, 0, 0), facing west.
- **The sun clock is one moving body.** The drum, dial and 13 soffit downlights rise 3.500 in 8.0 s, ease-in-out, on
  eight rams.
  - The drum's wall slides into the pocket annulus (r 6.54–7.00, z −3.58 … 0).
  - The retracting balustrade follows `rotunda.sun-clock.height`.
  - Guard: raising **holds** while someone is on the dial; lowering **reverses** if someone enters r < 7.20.
- **The Rotunda serves the well.** It owns everything inside r 7.60, z −6.40 … 0.00: shaft, pocket, stair and
  landing. The spiral stair lands at `rotunda.stair` (0, −7.60, −6.00), facing −y, which mates
  `classical-hall.stair`.
- **The dial is drawn by Time, never by hand.** Time::Dial lays the hour lines, ticks and numerals. The numerals and
  motto are the ported outlines in `SunClockGlyphs.h`.
- **Ports.** Build only the rough openings (clear opening plus the lining allowance) through the drum. The joint
  builds the thresholds, linings, surrounds and the 2.20 m west passage.
  - The stair port is finished by the Rotunda at the class's clear opening (§2): the joint lines only the Classical
    Hall's side and lays the strip.
  - All four doorways are width × crown classes springing at 4.50.
  - Every port unmated tonight gets closed bronze doors.
- **The eye is the only opening to the sky.** It is Glazed (D-004) and takes the museum's weather.
- **Light.** The cast spots stand across the room, not on the cornice ledge, which the entablature shades.

**Departures from as-built** (for the owner's review):
- **The rams (K3-MOTION):** each of the eight is a three-stage telescope in Mechanisms.json `extents` (stages Ø 0.250, 0.190 and 0.130, 2.200 long, 1.1667 m strokes: design numbers inside the room's Ø 0.300 barrel), 2.340 closed (bore bottom −5.900 to the lowered foot −3.560; the sheet's "closed ≤ 2.30" is 40 mm short of the drawn barrel) and 5.840 with the drum raised. The node's centre (20.25) is now the idealised sun's node in Data/Time/Museum.json (it was 20.30 there).
- **The dial's inlay (K3-TIME):** the hour ring, the hour lines, the gilt meridian and the minute ticks are laid from Time's dial (`FMuseumDial`), the ring in a 2 mm groove of the outer band; the lines, the meridian and the ticks stop at the ring's inner edge (r 6.760) rather than running 10 mm under it (6.77, 6.80), and lie on the face (Form cuts no radial grooves yet). The numerals wait for lettering.
- The Story overrides the as-built on four points (D-018): the lattice is **pearl-white** (as built dark grey), the dome is covered in **lead** (as built stone), the node is **patinated** (as built gilt), and a **60 mm gilt meridian** is added (as built none).
- Stone and gilt: **Crema Marfil** replaces statuary on the ring, dial, treads, landing and string; **Rojo Alicante** replaces rosso antico; the sun's 32 rays are gilt (as built bronze).
- All light is 3000 K. The cast spots stand across the room instead of on the cornice ledge, which the entablature shades.
- The imposts belong to the doorway class's lining, not to the drum.
