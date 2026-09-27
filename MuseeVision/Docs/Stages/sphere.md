# The Sphere · design sheet

- **Stage key** `sphere` (module `MuseeSphere`, `USphereRoom : USiteRoom`). Display name "The Sphere". Era: **Future** (Élan
  is three stages, D-012: the Atrium at level 0, the Cube below, the Sphere above). The Future's workings: pearl satin,
  clean oil gilt, mirror-polished bronze seats, Crema Marfil mirror-polished, low-iron glass; oak absent.
- **Sources:** D-001…D-019 (D-002, D-003, D-005, D-006, D-012, D-014, D-017, D-018 binding); Palette.json and Story.md ("The
  Sphere (Future)", "Élan outside"); the finished **`atrium.md`** and its schedules (the car, the channels, the iris class,
  the interlocks: they win at the seam); the drafter's spec `r-specs/…/Stages/sphere.md` (verified; corrections in §11); the
  boards `plan/canvas/ElanSphere.dc.html`, `AsBuiltElan.dc.html`, `plan/README.md:127-131`; the as-built code in `windows/`
  (§11). Schedules: `Data/Stages/sphere/{Finishes,Lighting,Mechanisms}.json` (no Hang, no Planting).
- **Units** metres. **Frame:** the Atrium's frame: origin on the Élan axis at the museum's floor datum (the Atrium's
  marble face, z 0.000); x east, y south, z up, true north. **museum = local + (54.000, 0.000, 0.000)**, yaw 0. So local z =
  museum z = every as-built Élan height, and the car's stops read the same in both sheets. The Sphere's centre **C** is
  local (0, 0, 22.000).
- **Bearings** are compass bearings, clockwise from north: a point at bearing b and radius r is (r sin b, −r cos b). Plan
  angle (from +x towards +y, as the old code and the iris pins) = bearing − 90°.

## 1. Identity and placement

A sphere 28 m across sits in the Atrium like a ball in a cup: equator on the drum at 22 m, its underside the Atrium's glowing
ceiling, its upper half the Élan's pearl dome with a gilt ball on top. The Atrium's glass car climbs through a pearl neck
(sealed below by the Atrium's collar iris, above by the Sphere's neck iris), then up a gilt mast to a stone platform at the
exact centre, the one point where an image on a sphere is true in every direction. Inside, the building is gone: the real
sky over the museum's city is all round, below the feet too. There is no sun inside, so by day it shows the stars the sun
hides (the board).

| | |
|---|---|
| Origin in the museum plan | **(54.000, 0.000, 0.000)**, yaw 0 (the Atrium's origin, `MuseePlan.h:167`). Site derives it from `sphere.down ↔ atrium.up` (§2): local (0, 0, 7.715) = museum (54, 0, 7.715). |
| Level | 2 (the platform, z 20.420; `plan/README.md:127`). |
| Footprint kind | Building, stacked on the Atrium on the same axis (NoOverlap and RoomStaysInside compare volumes). |
| Vertical extent | z 7.715 (`sphere.down`, the neck's foot) … 37.440 (the gilt ball's top). |

**Key levels:** mating plane (`sphere.down`) 7.715; rails from 7.745; car at the neck stop (home) 8.400; the Sphere's plate
meets the throat 8.372–8.382; image surface meets the throat 8.628; neck ring of light 9.600; the mast's dogs 10.850–11.150;
mouth (flange underside, the dogs' top, the mast's foot plane) 11.150; rails' top 11.340; neck iris stack 11.370–11.396;
flange top 11.400; the car's stop `mast` (car floor) 11.310; the car's stop `gate` (car floor) 17.400; deck soffit 20.170; platform iris stack 20.370–20.396; **platform
top 20.420** (= the car's floor top at the `sphere` stop, car frame 20.400); handrail centre 21.470; C 22.000; entablature's
wash (the Atrium's) 22.566–22.580; skin's foot 22.625; skin's crown 36.300; ball top 37.440.

**Plan outline:** the circle r **14.318** (the drip ring's outer edge), drawn as a 96-gon of circumradius 14.326, first
vertex at bearing 0°. **Volume** (solid of revolution; (r, z), anticlockwise from the axis): (0, 7.715) → (3.000, 7.715) →
(3.000, 8.325) → arc R 14.000 about (0, 22.000) out to (14.000, 22.000) → (14.080, 22.000) → (14.080, 22.580) → down the
Atrium's wash to (14.320, 22.566) → (14.320, 22.656) → arc R 14.335 about (0, 22.000) up to (0.600, 36.322) → (0.600,
37.450) → (0, 37.450) → down the axis.

**Seams** (the Atrium's, `atrium.md` §1): *below the equator*, the sphere R **14.000** about C (the Atrium's canopy's upper
face lies on it; the Sphere builds only inside it) and the neck column r < 3.000 from the plane 7.715 up to it (z 8.325);
*above the equator*, everything inside r 14.080 and above the entablature's wash. The throat (r 2.980) leaves a 20 mm shadow
gap to the Atrium's compression ring's bore (r 3.000), open, dark behind.

**Bearing on the Atrium (the load path):** the dome and everything hung in it bear on the Atrium's entablature wash through
the gilt drip ring, bedded face to face on the wash (§3.2); the neck's throat stands on the Atrium's collar top (7.715, r
2.465–4.100), which hangs from its ring. The plate shell stays **50 mm clear** of the canopy glass (R 13.950 ↔ 14.000): the
canopy carries nothing. Nothing bears on the Atrium's ribs.

| Enclosure | Kind | Bounded by |
|---|---|---|
| `sphere.interior` | Enclosed | the image surface (R 13.700 about C) down to the throat (r 2.980, z 8.628); the throat's outer face; the flange; the shut neck iris (11.396). ≈ 10,770 m³ less the tower. Sealed while the neck iris is shut; with it open (the car up) it joins `sphere.neck`, and both are sealed below by the Atrium's collar iris (shut, A-5). |
| `sphere.neck` | Enclosed | the throat's bore r 2.550 from the plane 7.715 (the Atrium's collar iris, top face 7.695) to the neck iris (11.396); ≈ 75 m³. The airlock. |

The car is the Atrium's moving body, not an enclosure; it stands in whichever enclosure it is in. The dome's outer face is
outdoors (the Grounds' air; weather museum). **Openings (Form's one list):** Port `sphere.down`; Housings
`sphere.neck.iris.pocket` (r 2.460–4.100, z 11.369–11.397), `sphere.platform.iris.pocket` (r 2.460–4.100, z
20.369–20.397) and `sphere.mast.dogs.housing.1` … `.4` (one per dog, inside r 2.550–5.650, z 10.830–11.150, §3.6). No Glazed or Open opening: the Sphere never sees the sky; it shows it (§4).

## 2. Ports

| Key | Kind | Class | Local frame (position; facing) | Museum frame | Face; depth | Mates with | Wells |
|---|---|---|---|---|---|---|---|
| `sphere.down` | vertical (received well) | **`iris.4600`** | (0.000, 0.000, 7.715); **−z** | (54.000, 0.000, 7.715); −z | the plane z 7.715 (the throat's foot on the collar's top); **3.685** (the neck, 7.715 → 11.400, the flange's top) | `atrium.up` (Atrium-local (0, 0, 7.715) facing +z = museum (54, 0, 7.715)). **Passage 0.00.** | **receiving**: the Atrium's served prism above, 7.715 → **23.390** |

- **`iris.4600`** as `atrium.md` §2 defines it: the well's section is the circle r **2.300** less two notches 16 mm wide
  (tangential ±0.008) from r 2.265 to 2.300 at bearings 0° and 180°. Closure: seven blades, each a 110° arc of the middle
  circle r 3.095, 1.250 wide, round ends, **2 mm thick at a 4 mm pitch** (stack 26 mm), pins at r 3.745 on plan angles
  360°k/7 (k 0…6, pin 0 due east), blade k 4k mm below the top; each turns 52° on its pin, eased over 4.0 s; open, the
  inner edge at r 2.470 in a seat r ≥ 2.460; in travel the blades reach r 4.010; each blade's pin-end is lengthened by a **pin lug**, a round boss r 0.060 about the pin joined to the round end by a 0.120-wide web on the pin's radius, so the blade encloses its pin (as built the pin stood 25 mm outside the blade). States shut 0, clamp 0.3206 (the channel maps linearly to the blade angle (Motion `follows`), so the as-built eased clamp 0.378 is **0.3206** here, the same blade pose), open 1.
- **The port's closure is the Atrium's** (the stage below the plane): its collar iris, top blade face 7.695, publishing
  `atrium.up.closure`. The Sphere builds no closure at the plane. **Joint:** none; the faces touch (the throat on the
  collar's top).
- **The received well:** the Sphere reserves the prism r ≤ 2.300 (less the notches) from 7.715 to 23.390 and builds inside
  it only what the class and this seam allow: the rails in the notches (7.745 → 11.340); its two class closures at declared
  levels, the **neck iris** (top face 11.396, `sphere.neck.closure`) and the **platform iris** (top face 20.396,
  `sphere.platform.closure`); its **mast** (r 0.200, foot ring r 0.400) on the axis under the car; the mast's four
  **dogs**, which enter the well (to r 0.300, z 10.850–11.150) only with the car's floor above 11.300; and the **platform's door sill**
  (r 2.245–2.300 across ±26° of west, z 20.397–20.420; ruled, §3.5, as the Atrium's own landing sill). What moves in the
  well is the Atrium's: the car, its leaves, guides, light, crown and Cube mast.
- **Closed states.** Mated (tonight, whenever the Atrium is built) the Atrium's collar iris closes it. **Unmated** (the
  stand): Site sets the class closure shut on the plane, `sphere.down.blind-iris` (z 7.715, blades facing down); no car; the
  neck and platform irises stay shut and the mast at 0.
- **Topology (D-012):** the Sphere is a leaf: one port, mating `atrium.up`. The car never lands: no doorway, light lock or
  landing door; at the `sphere` stop it stands in the platform's way. No stage pushes a sky.

### Anchors (the stage contract)
Bodies are drawn closed (blades shut, dogs retracted, the mast at zero length with its foot at the car's stop `mast`).

| Key | Kind | Frame (origin; axes) | Belongs to |
|---|---|---|---|
| `sphere.mast.head` | Body | (0, 0, 11.150): the mast column's top on the axis at the foot plane; axes of the stage; moves +z 0 → 9.090, following `atrium.car.height` | the mast (§3.6): the column's top, which bears on the car's under-floor seat |
| `sphere.mast.foot` | Body | (0, 0, 11.150): the foot plane on the axis (the foot ring drawn 11.152–11.210); axes of the stage; moves along z 0 → −34.910 (riding the car below `mast`) | the mast's foot ring and the column's base |
| `sphere.mast.dogs.1`, `sphere.mast.dogs.2`, `sphere.mast.dogs.3`, `sphere.mast.dogs.4` | Body | each at its tip's top face, retracted at r 2.560 on bearings 45°, 135°, 225°, 315°: (1.810, −1.810, 11.150), (1.810, 1.810, 11.150), (−1.810, 1.810, 11.150), (−1.810, −1.810, 11.150); axis radial inward: (−0.707, 0.707, 0), (−0.707, −0.707, 0), (0.707, −0.707, 0), (0.707, 0.707, 0) | the mast's bearing dogs, dog k in Housing `sphere.mast.dogs.housing.k` |
| `sphere.neck.iris.blade.1`, `sphere.neck.iris.blade.2`, `sphere.neck.iris.blade.3`, `sphere.neck.iris.blade.4`, `sphere.neck.iris.blade.5`, `sphere.neck.iris.blade.6`, `sphere.neck.iris.blade.7` | Body | each at its pin on its top face: (3.745, 0.000, 11.396), (2.335, 2.928, 11.392), (−0.833, 3.651, 11.388), (−3.374, 1.625, 11.384), (−3.374, −1.625, 11.380), (−0.833, −3.651, 11.376), (2.335, −2.928, 11.372); axes of the stage; turns about z through the pin | the neck iris |
| `sphere.platform.iris.blade.1`, `sphere.platform.iris.blade.2`, `sphere.platform.iris.blade.3`, `sphere.platform.iris.blade.4`, `sphere.platform.iris.blade.5`, `sphere.platform.iris.blade.6`, `sphere.platform.iris.blade.7` | Body | the same plan positions, top faces 20.396, 20.392, 20.388, 20.384, 20.380, 20.376, 20.372 | the platform iris |
| `sphere.call` | Use | (0, 0, 20.420): the platform way's centre. Usable: feet on the platform (\|z − 20.420\| < 0.50, r 2.460 … 5.290), or a look at the way | the Sphere's uses of the Atrium's car (rides on `atrium.car.lift`): "Call the car" (at `neck`) and "Send the car down" (at `sphere`) |
| `sphere.image.mount` | Mount | C (0, 0, 22.000); axes of the stage; the image surface is the sphere R 13.700 about it, facing in | the image luminaire (§4) |

No Wall, Bed, Water, Seat or Glazing anchors: no works, plants or seats. **Channels** (the Sphere's, 0…1, set by the car's
sequences in `atrium.car.lift` and held by §6's interlocks; the blades follow them): **`sphere.neck.closure`** (the neck
iris: 0 shut, 1 open), **`sphere.platform.closure`** (the platform iris) and **`sphere.mast.dogs`** (the dogs: 0 retracted,
1 extended). **Followed:** `atrium.car.height` (the mast's head and foot).
**Read by the interlocks:** `atrium.up.closure`.

## 3. Pieces by trade

Every piece is one solid of one substance meeting its neighbour face to face.

### 3.1 Steelwork (hidden, `sphere.steel`)
- **Plate shell:** the sphere R 13.940 … 13.950 about C, 10 mm plate, welded and faired, open only round the neck; its hole's
  edge welds to the throat's outer face r 2.980 at z 8.372 … 8.382. Stiffeners, the 180 mm mineral-wool absorber behind the
  panels (R 13.760 … 13.940) and the image panels' fixings are polish.
- **Standoffs** (polish) from the plate (R 13.950) to the skin (R 14.240), above z 22.625 only, on the skin's seam grid.

### 3.2 The exterior dome (as built `bPearlShell`; lathes about the axis, (r, z), C = (0, 22.000))
- **Skin** `sphere.skin`, 60 mm: (14.2263, 22.625) → (14.2863, 22.625) → arc R **14.300** about C up to (0.490, 36.2916) →
  (0.490, 36.2316) → arc R **14.240** about C down to (14.2263, 22.625). Seam hairlines (welded, ground, read only in
  grazing light): meridians at bearings 7.5°k up to latitude 60°, every 15° above; parallels every 6° of latitude.
- **Drip ring** `sphere.gilt.outside` (the skin's foot; a 50 mm half-round, 31 mm proud): (14.2263, 22.5716) → along the
  Atrium's wash to (14.2926, 22.5679) → (14.2926, 22.575) → half-round about (14.2926, 22.600) R 0.025 through (14.3176,
  22.600) to (14.2926, 22.625) → (14.2263, 22.625) → close. Its underside is bedded face to face on the wash (which falls
  0.0571 per metre outwards from (14.080, 22.580)); the half-round's underside stands 7–9 mm clear of it: the drip.
- **Boss** `sphere.gilt.outside`: (0, 36.230) → (0.485, 36.230) → (0.485, 36.2918) → along R 14.300 about C to (0.550,
  36.2894) → quarter-round about (0.510, 36.2894) R 0.040 to (0.510, 36.3294) → (0.300, 36.420) → (0.200, 36.5869) → the
  ball's seat, R 0.450 about (0, 36.990), down to (0, 36.540) → close. The spigot (r 0.485) passes the skin's crown hole (r
  0.490).
- **Ball** `sphere.gilt.outside`: the sphere R 0.450 about (0, 0, 36.990), top 37.440, in the boss's seat.

### 3.3 The image surface (`sphere.image`)
The sphere **R 13.700 about C** (Ø 27.40), facing in, less the neck's hole (edge r 2.980, z 8.628). Light's image panels
(light-field, 0.7 m, 60 mm deep, R 13.700 … 13.760, perforated for sound) are drawn by Light from its record; tonight one
continuous surface.

### 3.4 The neck (the airlock; `ElanSphereTop.cpp:191-256`)
| Piece | Geometry | Surface |
|---|---|---|
| Throat | annulus r 2.550 … 2.980, z 7.715 … 11.150, welded double skin | `sphere.throat` |
| Courses | six bands r 2.532 … 2.550, 24 mm tall, centred z 8.1446, 8.5739, 9.0033, 9.8620, 10.2913, 10.7207 (eight courses of 0.42935 from 7.7152, less the one within 0.2 of the light ring); stopped 10 mm short of the rail brackets | `sphere.throat.gilt` |
| Ribs | 16 at bearings 11.25° + 22.5°k: r 2.526 … 2.550, 22 mm tangential, z 7.715 … 11.150, stopped 10 mm short of the light channel (9.540 / 9.660) | `sphere.throat.gilt` |
| Light channel | r 2.520 … 2.550, z 9.550 … 9.650, bearings 5°–175° and 185°–355° (Light's body, §4) | — |
| Flange (the mouth, the neck iris's seat) | z 11.150 … 11.400; bore a 96-gon of circumradius **2.460** (sides centred on bearings 3.75°k; inscribed r 2.4587); outer r **4.500**; its top plate 3 mm (11.397 … 11.400) over the pocket; seven pins Ø 0.060 at r 3.745 on the blades' plan angles through the pocket | `sphere.neck.seat` |
| Pocket (Housing `sphere.neck.iris.pocket`) | r 2.460 … 4.100, z 11.369 … 11.397, open to the bore | — |
| Neck iris | bodies `sphere.neck.iris.blade.1` … `.7` (Anchors); stack 11.370 … 11.396; tops `sphere.neck.iris.top`, undersides and edges `sphere.neck.iris.under`. Shut, it closes the mouth 4 mm under the flange's top | as named |
| Rails | north and south (bearings 0°, 180°), the Atrium's section: blade r 2.265 … 2.385, 16 mm; foot r 2.385 … 2.425, 140 wide; a rack on each back; z **7.745 … 11.340** (30 mm under the neck iris's stack) | `sphere.rail` |
| Rail brackets | 180 wide × 100 tall, centred z 7.900, 8.900, 9.900, 10.900 (r 2.425 … 2.550, to the throat) and 11.250 (r 2.425 … 2.4587, to the flange's bore) | `sphere.rail` |

The Atrium's rails end at 7.425; the 0.320 break at the collar is bridged by the car's second ring of guides (the Atrium's).

### 3.5 The platform (`ElanSphereTop.cpp:272-369`, `ElanKit.h:88-93`)
Top **z 20.420**. Radii from the axis.

| Piece | Geometry | Surface |
|---|---|---|
| Deck | steel box r 2.460 (the 96-gon) … 5.498, z 20.170 … 20.390 under the stone, … 20.369 under the pocket | `sphere.steel` |
| Soffit, fascia | the deck's underside (20.170) and outer face r 5.498 from 20.170 to 20.360 | `sphere.platform.soffit` |
| Stone ring | r 4.100 … 5.498, 30 mm (20.390 … 20.420), 24 slabs of 15°, 1 mm joints at bearings 7.5° + 15°k | `sphere.platform.floor` |
| Seat ring (the pocket's lid) | r 2.460 (96-gon) … 4.100, 23 mm (20.397 … 20.420), 16 cast segments, joints at bearings 11.25° + 22.5°k, bolted to the pins and the pocket's outer wall | `sphere.platform.seat` |
| Door sill | r 2.245 … 2.460, z 20.397 … 20.420, across bearings 244°–296° (±26° of the car's door), cantilevered from the seat ring over the iris's plane; 10 mm from the car's base ring (2.235) at the stop | `sphere.platform.seat` |
| Pocket (Housing `sphere.platform.iris.pocket`) | r 2.460 … 4.100, z 20.369 … 20.397; seven pins Ø 0.060 at r 3.745 tie lid to deck | — |
| Platform iris | bodies `sphere.platform.iris.blade.1` … `.7`; stack 20.370 … 20.396; tops `sphere.platform.iris.top`, undersides `sphere.platform.iris.under`. Shut, a pale disc 24 mm under the platform: a closure, **not a floor** | as named |
| Nosing | r 5.498 … 5.512, z 20.360 … 20.423 | `sphere.platform.gilt` |
| Balustrade shoe | r 5.290 … 5.350, z 20.420 … 20.480 | `sphere.platform.gilt` |
| Posts | 32, 25 × 25, centred r 5.320 at bearings 5.625° + 11.25°k, z 20.480 … 21.442 | `sphere.platform.gilt` |
| Glass | 32 curved panes on r 5.320 (faces 5.3136 … 5.3264), z 20.480 … 21.440, each centred on bearing 11.25°k over 10.873° (5 mm clear of each post) | `sphere.platform.glass` |
| Handrail | tube Ø 0.056 on the circle r 5.320, centre z 21.470 (top 1.078 above the floor) | `sphere.platform.rail` |
| Light channel | r 5.270 … 5.292, z 21.425 … 21.440, all round inside the post line (Light's strip in it, §4) | `sphere.platform.gilt` |
| Channel brackets | 32, mid-pane at bearings 11.25°k: CuSn10 strap 25 wide × 6 thick bent to an L, vertical leg r 5.292 … 5.298, z 21.419 … 21.453 saddle-brazed to the handrail tube's underside, horizontal leg r 5.270 … 5.298, z 21.419 … 21.425 under the channel (legs 0.034 + 0.028); the channel fixed to each by one M4 CuSn screw | `sphere.platform.bracket` |
| Struts | 8 tubes Ø 0.168 × 10 at bearings 22.5° + 45°k, axis from (r 4.300, z 11.400) on the flange's rim to (r 5.300, z 20.170) under the deck (8.827 long, 6.5° off vertical); end plates Ø 0.300 × 0.020 (feet 11.400 … 11.420 on the flange, heads 20.150 … 20.170 under the deck) | `sphere.platform.gilt` |

**Walkable:** the stone ring and the seat ring and sill, r 2.245 (at the door) / 2.460 … 5.290. The shut platform iris is
not walked (2 mm blades; §10).

### 3.6 The mast (the Sphere's; as built `ElanElevator.cpp:682-692`)
A gilt column `sphere.mast`, r **0.200** (as built Ø 0.40), on the axis from the foot plane **z 11.150** (the mouth) up to
the car's under-floor seat (the Atrium's `atrium.car.mast` drum, car frame −0.160 … −0.030). Its top is the body
`sphere.mast.head`; the column between foot plane and head is **Motion's variable-length piece** (length 0 → 9.090, drawn by
rule, never at one length; at length 0 it has no geometry, so it never meets the car at home).

**Its footing (Q-1, ruled): the foot ring and four bearing dogs.**
- **Foot ring** `sphere.mast` (body `sphere.mast.foot`): a gilt CuSn10 annulus r 0.205 … 0.400, 58 mm deep, holding the
  column's base in its bore. Below the car's stop `mast` it rides the car, latched round the car's under-floor seat (car
  frame −0.158 … −0.100; the car's lowest point stays the seat's −0.160; the latch is the Atrium's). At `mast` (car floor
  11.310) its underside is at 11.152, 2 mm over the dogs; the dogs slide under it, the car's first 2 mm of rise sets it on
  them and the latch lets go; from there up it stays at 11.150 and the column grows between it and the seat.
- **Dogs** `sphere.mast.dogs` (bodies `sphere.mast.dogs.1` … `.4`), at bearings **45°, 135°, 225°, 315°** (clear of the rail
  notches at 0° and 180°, of the throat's ribs at 33.75°/56.25° etc. by ≥ 0.4 m, and of the struts at 22.5° + 45°k):
  solid **CuSn10** bars (EN 1982 CC480K, the classic bearing bronze), section **0.160 wide × 0.300 deep**, **3.060 long**,
  top face **z 11.150** (the foot plane), bottom 10.850. **Stroke 2.260:** retracted, the tip stands at r 2.560 (10 mm
  behind the throat's bore, outside r 2.300); extended, at **r 0.300**, 100 mm under the foot ring, with 0.800 of the bar
  still in its housing. Driven by `sphere.mast.dogs` over 2.0 s, eased.
- **Sizing:** the car (≈ 6 t, Form volumes × Matter densities), its rated load (EN 81-20 for its 14.25 m² floor, ≈ 8.3 t)
  and the column (≈ 1 t): ≈ 152 kN, × 1.5 dynamic ≈ 230 kN, taken by any two of the four dogs: 115 kN each at a 2.20 m lever
  from the housing's mouth, ≈ 250 kNm; the section's modulus 2.4 × 10⁻³ m³ gives ≈ 104 MPa, under CuSn10's 0.2 % proof
  (≈ 130 MPa); tip deflection ≈ 11 mm (E ≈ 100 GPa). The back reaction (≈ 310 kN over the 0.800 engagement) goes into
  the housing's hard-chromed wear strips and the flange above (Q-2 confirms).
- **Housings** (Housings `sphere.mast.dogs.housing.1` … `.4`, one per dog): welded boxes of S355 plate 20 mm
  (`sphere.mast.dogs.plate`), 0.220 wide outside, 0.180 inside (10 mm running clearance each side), inside z 10.830 …
  11.150 (the top the flange's underside), from the throat's bore r 2.550 out along the bearing to r 5.650 inside:
  two side plates 0.340 × 3.120 (r 2.550 … 5.670, z 10.810 … 11.150); a bottom plate 0.220 × 3.120 (z 10.810 … 10.830);
  beyond the flange a top plate 0.220 × 1.170 (r 4.500 … 5.670, z 11.150 … 11.170); an end plate 0.220 × 0.360 at r
  5.650 … 5.670 carrying the drive (a hydraulic ram inside). Fixings: full-penetration welds to both skins of the throat
  where they pass it; the side plates bolted to the flange's underside by M20 grade 8.8 bolts at 0.250 centres through
  welded flats. **Wear strips** (`sphere.mast.dogs.liner`, hard-chromed steel 10 mm, M8 countersunk at 0.200): on the
  bottom plate at the mouth (r 2.560 … 3.400, top at 10.840: 10 mm under the dog) and inset flush in the flange's
  underside over the dog's rear (r 3.200 … 4.450). Pearl, seen from the platform 9 m down.

### 3.7 What the Sphere does not build
The Atrium: the collar and its iris (the port's closure), the ring and its hangers, the canopy and grid, the entablature and
its wash (the dome's seat), the car with everything on it (leaves, guides, crown, Cube mast, the under-floor seat, the car's
light). Site: the blind iris on the stand. Light: the image panels and the strips from their records.

### 3.8 Finishes (summary; Finishes.json holds the notes)

| Surfaces | Material · working · history |
|---|---|
| `sphere.skin` | `white.pearl-steel` · pearl.fluoropolymer · weathered |
| `sphere.gilt.outside` | `bronze.cusn10` · gilt.gold-leaf.oil · weathered |
| `sphere.steel` | `white.pearl-steel` · blast-cleaned.primed · new |
| `sphere.image` | `glass.low-iron` · lenslet-array.black · new |
| `sphere.throat`, `sphere.neck.iris.under`, `sphere.platform.iris.top`, `sphere.platform.soffit`, `sphere.down.blind-iris` | `white.pearl-steel` · pearl.satin · new |
| `sphere.neck.iris.top`, `sphere.platform.iris.under` | `white.pearl-steel` · gilt.gold-leaf.oil · new |
| `sphere.throat.gilt`, `sphere.platform.gilt`, `sphere.platform.bracket`, `sphere.mast` | `bronze.cusn10` · gilt.gold-leaf.oil · new |
| `sphere.mast.dogs.plate` | `white.pearl-steel` · pearl.satin · new |
| `sphere.mast.dogs.liner` | `steel.hard-chrome` · ground.hard-chromed · new |
| `sphere.platform.rail` | `bronze.cusn10` · gilt.gold-leaf.oil · handled |
| `sphere.neck.seat` | `bronze.cusn10` · mirror-polished.lacquered · new |
| `sphere.mast.dogs` | `bronze.cusn10` · mirror-polished · new (unlacquered bearing faces) |
| `sphere.platform.seat` | `bronze.cusn10` · mirror-polished · foot-worn |
| `sphere.platform.floor` | `stone.crema-marfil` · mirror-polished · foot-worn |
| `sphere.rail` | `steel.hard-chrome` · ground.hard-chromed · new |
| `sphere.platform.glass` | `glass.low-iron` · toughened.laminated · new |

## 4. Light

**Daylight:** none (no Glazed or Open opening; the dome is opaque).

**The sky (D-002, D-003, D-012):** the image luminaire shows the **real sky over the museum's city now**: Light's image
source draws the celestial sphere for Time's sky of the museum's observer (Beijing by default, New York by the setting):
the stars to V 6.5 (Yale Bright Star Catalogue, `assets/sky/stars.bin`) at their true places and colours, the Moon at its
true size, place and phase, the Milky Way (NASA SVS Deep Star Maps, its bright stars masked); **no Sun, no atmosphere**
(the board: by day it shows the stars the sun hides). It turns with sidereal time and the hour key. It is a display in the
Sphere's own shell, not a sky pushed into the scene: no scoped-sky machinery.

**Luminaires** (Lighting.json, 13; points local; no output values here)

| Ids | Type | Count | Point and aim | Operation |
|---|---|---|---|---|
| `image` | `gap.light-field-panel.d65` | 1 (the whole R 13.700 surface) | Mount `sphere.image.mount` (C), facing in; aim C | always |
| `rail.01` … `rail.08` | `linear-cove.3000k` | 8 runs of 45° (4.148 each) | strip on r 5.281, z 21.432 in the channel under the handrail, centred on bearings 22.5° + 45°k; aim the floor at r 3.900 on the same bearing | presence (a body in the Sphere) |
| `neck.01` … `neck.04` | `linear-cove.3000k` | 4 runs of 85° (3.761 each) | strip bent to r 2.535, z 9.600, centred on bearings 47.5°, 132.5°, 227.5°, 312.5°; aim the axis at 9.600 | presence (a body in the neck) |

Retired (not ported): the platform's 3,000 lm "fill no one sees" 9 m up, the eight point lamps and glow bands, the neck's six
6500 K lamps, the "building hidden" switch (`ElanElevator.cpp:374-386`). The car's light ring is the Atrium's (§6 request).

**Targets:** `sphere.platform.floor` (with the seat ring) `orientation` 5 lx (UseTargets: low light that shows the way in a
darkened space); the way out is the car (`lift`, the Atrium's). No care targets (no works).

**Design order** (measured, never tuned): the image surface's background (black level plus all spill, Moon down, nothing
performing) ≤ 1/1000 of the lit stone's mean luminance, so the Milky Way's band (≈ 10⁻³ cd/m²) stands above it; the strips
are never seen directly from a standing eye on the platform (the rail shields them). A failure is a finding.

## 5. Time and weather
- **Inside:** the sky of §4, from Time (sidereal time, the Meeus almanac for the Moon); D-003's idealised sun does not
  apply inside: the Sun is never drawn. No weather inside; the shell is sealed.
- **Outside:** the skin, ring, boss and ball take the **museum** observer (D-005): rain wets the pearl and runs down it
  (heavier low on the dome), the drip ring throws it onto the Atrium's wash; snow lies on the crown where the slope allows.
  The museum's one sun (idealised) moves its warm light and the sky's reflections over the lacquer. Night façade light is
  the Grounds' (reserved on the Atrium's cornice).

## 6. Motion
The Sphere owns the mast (head and foot), its four dogs and two irises (Mechanisms.json, 20). With the Atrium's car and
collar iris and the Cube's ceiling iris they form one coupling group; Site binds the channels across the seam. The car's
stops in the Sphere (the Atrium's M-1, `atrium.car.height` = (z + 23.600) ÷ 44.000):

| Stop | Car floor z | Fraction | What |
|---|---|---|---|
| `neck` | 8.400 | 0.727273 | home: in the neck, the collar iris and the neck iris shut |
| `mast` | 11.310 | 0.793409 | the mast's foot ring 2 mm over the dogs' top (11.150): the dogs go in or out |
| `gate` | 17.400 | 0.931818 | the car's head 30 mm under the platform iris's stack: the iris opens or shuts |
| `sphere` | 20.400 | 1.000000 | eyes at the centre; the car's floor top level with the platform (20.420) |

**M-S1 `sphere.mast`** (body `sphere.mast.head`): prismatic +z, travel **9.090**, **follows** `atrium.car.height` in
[0.793409, 1] → out [0, 9.090] (the column's top stays on the car's seat: length = car floor − 0.160 − 11.150). Handover:
the car's base-ring guides leave the rails' top (11.340) at car floor 11.410, so rails and mast overlap 0.100.

**M-S2 `sphere.mast.foot`** (body `sphere.mast.foot`): prismatic along z, travel 34.910, **follows** `atrium.car.height`
in [0, 0.793409] → out [−34.910, 0]: below `mast` the foot ring rides the car 1:1 round its seat; from `mast` up it rests
on the dogs at 11.150.

**M-S3…M-S6 the dogs** `sphere.mast.dogs.1` … `.4`: prismatic, radially inward (axes in the Anchors), travel **2.260**,
2.0 s, eased; **follow** `sphere.mast.dogs` in [0, 1] → out [0, 2.260].

**M-S7…M-S13 the neck iris** `sphere.neck.iris.blade.1` … `.7` and **M-S14…M-S20 the platform iris**
`sphere.platform.iris.blade.1` … `.7`: revolute about each blade's pin (its body's origin), drawn shut, opening 52° towards
decreasing plan angle (axis −z); follow `sphere.neck.closure` / `sphere.platform.closure` in [0, 1] → out [0, 52], eased
over 4.0 s. Housed in their pockets. (20 entries: M-S1…M-S20.)

**Sequences** (the car's, in `atrium.car.lift`; the Sphere's legs, steps `{channel, to, time}` and moves):
- **Leg up** (in "Up to the Sphere" from the Atrium or the Cube, after the collar iris has shut at `neck`, and in
  `sphere.call`): `sphere.neck.closure` → 1 (4.0) → move `mast` → `sphere.mast.dogs` → 1 (2.0) → move `gate` →
  `sphere.platform.closure` → 1 (4.0) → move `sphere` → `atrium.car.light` → 0.03 (2.5) → `atrium.car.doors` → 1 (1.5).
- **Leg down** ("Down to the Atrium" and "Send the car down"): `atrium.car.doors` → 0 (1.5) → move `gate` →
  `sphere.platform.closure` → 0 (4.0) → move `mast` → `sphere.mast.dogs` → 0 (2.0) → move `neck` → `sphere.neck.closure` → 0
  (4.0) → `atrium.car.light` → 1 (2.5). "Down to the Atrium" then goes on to the Atrium (collar iris open, move `atrium`,
  leaves open); "Send the car down" ends there, at home.
- **Uses:** `sphere.call` gives two rides on the car: **"Call the car"** at `neck` (requires `atrium.up` mated; the leg up;
  caption "The car rises out of the neck to meet you."), and **"Send the car down"** at `sphere` (requires
  `atrium.car.doors` ≥ 1; the leg down; caption "The car sinks back into the neck, and the Sphere is yours."). In the car,
  the Atrium's "Down to the Atrium".

**Interlocks** (Motion, by key; each honours the Atrium's rule of the same subject):

| # | Rule | Atrium |
|---|---|---|
| S-1 | Airlock: `sphere.neck.closure` rises above 0 only with `atrium.up.closure` ≤ 0.001, and falls back to 0 before the collar iris may open. | A-5 |
| S-2 | The car's head (2.940 above its floor) passes the neck iris's stack (11.370 … 11.396): the car floor rises above **8.410** or descends below **11.606** (seat bottom −0.160 over the stack top + 0.050) only with `sphere.neck.closure` ≥ 0.999; the neck iris leaves open only with the car floor ≤ 8.410 (the mast then at 0, the dogs in). | A-4 (mirrored) |
| S-3 | Platform gate: the car floor passes **17.400** (`gate`) upward only with `sphere.platform.closure` ≥ 0.999; the platform iris leaves open only with the car floor ≤ 17.400. | A-7 |
| S-4 | Occupancy: the platform iris does not open while a body stands within r < 2.810 of the axis ("Step off the centre: the car is coming up."); closing, it holds or reverses while a body is in its travel slice (`body-in-travel`). The leg up opens it only with the car waiting at `gate`. | A-3 |
| S-5 | The car's leaves open at `sphere` only (never between); the platform's sill is 10 mm from the car's base ring there. | A-2 |
| S-6 | The mast leaves 0 (the car floor rises above `mast`, 11.310) only with `sphere.mast.dogs` ≥ 0.999. | — |
| S-7 | The dogs retract (`sphere.mast.dogs` below 1) only with the mast at 0 and the car at `mast` (11.310 ± 0.005). | — |
| S-8 | The dogs extend (`sphere.mast.dogs` above 0) only with the car's floor above **11.300**. | — |

**Clearances:** car head at `neck` (11.340) ↔ neck iris stack (11.370) 30 mm; rails' top (11.340) ↔ that stack 30 mm; car
head at `gate` (20.340) ↔ platform iris stack (20.370) 30 mm; foot ring (11.152) ↔ dogs' top (11.150) 2 mm at `mast`, then
bearing; dogs' tips (0.300) ↔ the car's seat (0.200) 100 mm; retracted dogs (2.560) ↔ the car (2.235) 325 mm; car rings
(2.235) ↔ throat (2.550) 315, ↔ the seats' 96-gon (2.4587) 224, ↔ sill (2.245) 10 mm; guides (2.290) ↔ rail blades:
rolling contact; open blades (2.470) ↔ car (2.235) 235; mast (0.200) ↔ open blades 2,270; car ↔ struts (r ≥ 4.216) ≥ 1.98 m;
car's head at `sphere` 23.340 ≤ well top 23.390; blades ↔ blades 2 mm (pitch).

**The Atrium's side (done by the coordinator, matched here):** the stops `mast` and `gate`; the legs above in "Up to the
Sphere", "Down to the Atrium" and `sphere.call`; the new ride "Send the car down"; the car's light to a glimmer at
`sphere`; the platform iris shut behind a descending car; the foot ring's latch on the car's seat.

## 7. Nature
None: no plants, water or ground. Rain and snow fall on the dome (the Grounds' outdoors) and stop at the skin.

## 8. Art
No works. *The Starry Night* is the Atrium's, on its stele. The sky is not a work (it is Light's image, §4). Reserved:
works made for the full sphere ("new art on the whole sphere", to be planned) and the proposed opening piece *The sky
becomes The Starry Night* (ElanSphere board: proposed, not built); either would replace the sky's image source on the same
luminaire while it performs. No Hang.json.

## 9. Sound
- **Acoustics** from Matter, never tuned: `sphere.interior` ≈ 10,770 m³ inside the perforated image panels over 180 mm of
  mineral wool on the steel plate (a planetarium's build): a hush, the sphere's focus absorbed (RT60 expected ≈ 0.4–0.8 s).
  `sphere.neck`, ≈ 75 m³ of hard pearl: bright and short.
- **Emitters:** footfalls on mirror-polished stone and bronze; the irises' blades (4 s slide, a soft seat); the car's leaves
  and drive and the mast's feed (the Atrium's actuations and the mast's). No music, no ambience bed.
- **Occlusion:** the sealed shell and the airlock; rain on the skin is not heard inside.

## 10. Visitor
- **Arrive** by the Atrium's car ("Up to the Sphere"): the airlock in the neck (the collar iris shuts below, the neck iris
  opens above), then up the gilt mast into the sky; the car stops with the eyes at the centre and its leaves open west.
- **Step out** over the bronze sill onto the seat ring and the stone; walk the ring (r 2.46 … 5.29), the gilt rail at
  1.078 m; look up, round and down: the sky is below the feet too. The hour key turns it. Nothing to sit on.
- **Uses:** in the car, the Atrium's "Down to the Atrium"; on the platform (`sphere.call`), "Send the car down" (the car at
  `sphere`, its leaves open: it sinks into the neck and the Sphere is yours) and "Call the car" when it is home. Never trapped: the call is the way out (Site.Reachable over the state graph).
- The shut platform iris is **not walkable** (thin blades): the Visitor's walk stops at the seat ring's inner edge (a 24 mm
  drop) and step-to never lands on it or in the open way. The Eye adapts from the lit car to the dark Sphere (nothing local).

## 11. The old-code map
Paths relative to `windows\Source\MuseeVision\` unless noted. Re-express, never copy.

| Part | Where |
|---|---|
| Sphere centre and radius, top floor, speed, `RingHeight()` | Plan/MuseePlan.h:160-215 |
| Exterior constants (entablature 20.90–22.58) | Plan/MuseePlan.h:217-256 |
| Car (heights, seat r 0.200 −0.160 … −0.030), neck, irises, platform, crown, gate | Elan/ElanKit.h:40-56, 68-104; ElanKit.cpp:22 (`CollarTop`); Elan/ElanStructure.h:46 (`RingDepth`) |
| Iris blade (outline, pins, stack, swing, easing) | Elan/ElanSphereTop.cpp:46-181 |
| Neck (throat, courses, ribs, light ring, flange, both irises) | Elan/ElanSphereTop.cpp:191-256 |
| Platform (slab, nosing, balustrade, rail light, struts, fill, iris, guard) | Elan/ElanSphereTop.cpp:272-393 |
| Timings; on-platform and doorway tests; call; prompts; captions; send away | Elan/ElanElevator.cpp:36-46, 210-228, 237-248, 290-340, 362-371 |
| Irises' goals, platform gate hold, home, the mast | Elan/ElanElevator.cpp:532-586, 633-639, 682-692 |
| "Building hidden" / sky sphere mode (retired) | Elan/ElanElevator.cpp:374-386; Sky/MuseeSky.cpp:118-125 |
| Stars and Milky Way (Light's evidence) | Sky/MuseeSky.cpp:246-344; repo assets/sky/stars.bin |
| Pearl dome, drip ring, boss, ball | ElanExterior/ElanExteriorStructure.cpp:1131-1162; ElanExteriorStructure.h:52-59, 216-219 |
| Boards | repo plan/canvas/ElanSphere.dc.html, AsBuiltElan.dc.html; plan/README.md:127-131 |

**Corrections to the draft** (`r-specs/…/Stages/sphere.md`):
1. **The car is the Atrium's** (the ruled seam): the draft's car, crown, base housing, mast foot, both columns, drives,
   stops and `sphere.car.*` channels are removed; the Sphere **receives** the well (draft: serves).
2. **Port** `sphere.shaft` → **`sphere.down`**, class **`iris.4600`** (draft `shaft.lift.4900.iris`); its closure is the
   Atrium's collar iris (draft: a Site "port iris" publishing `sphere.shaft.closure`, a bronze ring and a closure ring on
   the plane: all gone). Frame and museum position unchanged: (54, 0, 7.715).
3. **The mast** follows `atrium.car.height` [0.793409, 1] → [0, 9.090] from the foot plane 11.150 (as built); the draft's
   clamp stop `neck.clamp` 11.702 and the car-carried base housing are gone; the footing is the coordinator's ruling (§3.6):
   a foot ring carried on four bronze dogs from the car's stop `mast` (11.310).
4. **Channels** `sphere.neck.closure`, `sphere.platform.closure`, `sphere.mast.dogs` (draft `…iris.state`).
5. **Blades** 2 mm at a 4 mm pitch, stack 26 mm (draft 5 mm at 3 mm, 23 mm); neck stack 11.370–11.396, pocket 11.369–11.397.
6. **Rails** 7.745 … 11.340, hard-chromed steel as the Atrium's (draft 7.735 … 11.020, brushed bronze; the draft's rails
   ended 0.290 under the mast's start: the car had no support between).
7. **Seam** R 14.000 (the Atrium's) with the plate 50 mm clear of the canopy (draft Σ 13.960 and bearing pads on the ribs);
   the drip ring bedded face to face on the wash (draft floated on a sealant bed).
8. **Materials** are palette keys in the Future's workings (draft role strings `cream-marble…`, `bronze.patinated.dark`
   flange top, `bronze.brushed` rails, stone-veneered platform iris): see §3.8.
9. **Light:** catalogue `linear-cove.3000k` for both rings (draft gaps `linear-handrail.3000k`, `linear-slot.4000k`), the
   image a gap `gap.light-field-panel.d65`; target `orientation` 5 lx (draft a new class `assembly.dark`).
10. **Anchors** in the contract's kinds (draft `sphere.space`, kind Space, dropped: no such kind).
11. **Verified unchanged:** origin and heights; the throat, courses, ribs and light ring; the flange's 96-gon; the platform's
    radii, stone ring, nosing, shoe, posts, glass and handrail (32 panes between posts, as the draft); the skin, ring, boss
    and ball sections (the draft's re-expressions kept).

## 12. The first-pass cut
**Tonight:** the declaration (port, volume, enclosures, housings, anchors, channels); the plate shell; the image surface as
one continuous image luminaire with the sky; the dome (skin with seam lines, drip ring, boss, ball); the neck (throat,
courses, ribs, flange, rails and brackets); the platform (deck, stone and seat rings, sill, nosing, balustrade, handrail,
struts); both irises; the mast (foot ring, column as Motion's variable-length piece) and its four dogs with their housings;
M-S1…M-S20 and S-1…S-8 with the Atrium's car; the 12
strips; acoustics and footfalls.

**Waits for polish:** the image panels' tiling, stiffeners, absorber and standoffs as drawn pieces; the rack detail; planets
and earthshine; mechanism voices; any full-sphere work.

## 13. Palette keys to confirm and catalogue gaps
**Palette keys** (all real entries or ruled; workings outside `workingsByEra` "future" or roles guessed):
- `white.pearl-steel` **blast-cleaned.primed** for hidden steel (the palette names the steel only as pearl-finished).
- `white.pearl-steel` **gilt.gold-leaf.oil** on the irises' gilt faces (oil gilt over the pearl lacquer, as the Atrium's).
- `glass.low-iron` **lenslet-array.black** for the image surface (the palette names the lenslet array for the Cube).
- `bronze.cusn10` **mirror-polished.lacquered** for the flange and **mirror-polished** (walked) for the platform's seat ring
  and sill: the palette's "iris seat ring … mirror-polished seat ring".
- `bronze.cusn10` + gilt **weathered** outdoors (ring, boss, ball): the palette says "clean, never patinated".
- `steel.hard-chrome` (ruled) for the neck's rails: the palette's "Sphere: rail (gilt)" is read as the platform's handrail.

**Catalogue gaps** (Light): `gap.light-field-panel.d65`: a light-field image panel (after Light Field Lab SolidLight),
0.7 m modules curved to R 13.700, black mask (face reflectance ≤ 1 %), D65 white, fed by Light's celestial image source;
the Cube's panels are the flat kind.

**Questions for the Architect:**
- **Q-1 · The mast's footing: closed, as ruled by the coordinator.** Four radial CuSn10 bearing dogs (0.160 × 0.300, stroke
  2.260, at bearings 45°, 135°, 225°, 315°) slide in at the car's stop `mast` under the mast's foot ring (§3.6, §6, S-6…S-8).
  Below `mast` the foot ring rides the car on a latch round its seat (the Atrium's latch).
- **Q-2 · Structure.** The platform (≈ 76 m² at 5 kN/m² plus the deck) on eight struts, now Ø 168 × 10 (Euler ≈ 220 kN each
  at 8.8 m, CuSn10), onto a flange rim that cantilevers 1.52 m from the throat, which stands on the Atrium's collar and its
  hangers; the 23 mm cast seat ring spanning over the pocket between the pins. A structural review confirms the sections.

## Build notes
- **Frame and translation:** the Atrium's frame; **museum = local + (54.000, 0, 0)**, yaw 0; old plan x − 54; every height
  is the as-built height. Bearings clockwise from north; plan angle = bearing − 90°.
- **The port** is the plane 7.715 facing −z; the throat's foot touches the collar's top; the Sphere builds nothing below it
  and no closure on it (the collar iris is the Atrium's).
- **Received well:** keep r ≤ 2.300 (less the notches) clear from 7.715 to 23.390 except the rails, the two irises, the mast
  and the door sill (r ≥ 2.245 across ±26° of west); the dogs enter it (to r 0.300, z 10.850–11.150) only above the car.
- **Moving bodies:** 14 blades (fixed-frame bodies on their pins, drawn shut); 4 dogs (drawn retracted, tips at r 2.560);
  the mast head (drawn at zero length) and the mast foot (the foot ring, drawn at 11.152–11.210, the car's stop `mast`).
  The mast column and nothing else is variable-length; draw it by rule from the foot plane 11.150 to the head. The one
  Sphere piece that rides the car is the foot ring below `mast` (following the car's height, latched round its seat); the
  car's pieces and light are the Atrium's.
- **Dogs' housings** `sphere.mast.dogs.housing.1` … `.4` are pieces (`sphere.mast.dogs.plate`, wear strips `sphere.mast.dogs.liner`) through the throat wall and under the flange to r 5.650 at bearings 45°,
  135°, 225°, 315°; they clear the throat's ribs (nearest 0.49 m) and top course (0.097 below) and the struts' feet.
- **Seats of the dome:** the drip ring lies on the Atrium's wash face to face; the skin starts on the ring (22.625). The
  plate shell never touches the canopy glass (50 mm).
- **Draw by rule:** the lathes (skin, ring, boss, ball, flange, rings); the 96-gon seats; the blades from the class; the
  balustrade by bearing. The image surface is Light's piece, not a mesh texture.
- **Unmated** (the stand): Site's blind iris at 7.715, both irises shut, mast 0, no car.

**Departures from as-built** (for the owner's review):
- **iris.4600 corrected:** each blade gains a pin lug (r 0.060 boss on a 0.120 web) so it encloses its pin; as built the pin stood 25 mm outside the blade. The clamp value is 0.3206 on the linear channel (as built 0.378 through a smoothstep: the same blade pose, r 0.197). Shut, clamp, open and reach checked by polygon union.
- **Ruled with the Atrium:** the car, its leaves, guides, crown, seat and light are the Atrium's; the port's closure is the
  Atrium's collar iris; the Sphere's mast is the Sphere's and follows the car's height.
- **Ruled: the mast's footing** (as built it stood on nothing): a gilt foot ring carried on four CuSn10 bearing dogs that
  slide in from housings in the neck wall at the car's new stop `mast` (11.310); the ring rides the car below it. The ride
  up gains a stop at `mast` (dogs 2.0 s) and one at `gate` (platform iris 4.0 s), about 8 s over the as-built 27 s.
- **"Send the car down"** kept (as built) as the Sphere's use; the platform iris opens only with the car at `gate`.
- **Rails in the neck** (7.745 … 11.340, hard-chromed, as the Atrium's): as built the car floated in the neck.
- **Blades 2 mm at a 4 mm pitch** (class `iris.4600`; as built 5 mm at 3 mm, interpenetrating).
- **The platform iris** is lowered 20 mm (top 20.396, as built 20.416) under a **23 mm mirror-polished bronze seat ring**
  (as built a 3 mm stone lid), and its blades are pearl (the palette's lining; as built stone on top): 2 mm blades and a 3 mm
  lid can't be walked. So the shut iris is a closure, not a floor (the board's "a full stone floor"); the Visitor keeps off
  it.
- **A bronze door sill** r 2.245 … 2.460 across the car's door at the platform (as built a 225 mm gap over the drop; EN 81-20
  allows 35 mm), inside the well's radius as the Atrium's own landing sill is.
- **Struts** Ø 168 × 10 tubes from the flange's rim (r 4.300) to under the deck (r 5.300), the flange out to r 4.500 (as
  built Ø 80 bars from r 3.350, over the iris pocket, to r 4.400; too slender, and bearing on the pocket's 3 mm plate).
- **Dome on the Atrium:** the plate shell 50 mm clear of the canopy glass (as built no shell); the drip ring bedded on the
  wash (as built 5 mm into it); the boss's spigot and ball seat (as built overlapping), as the draft.
- **Light:** 3000 K strips in catalogue types (as built 6500 K neck lamps, glow bands, a hidden 3,000 lm fill); the platform
  at `orientation` 5 lx. The "building hidden" sky mode is retired: the sky is a display on the Sphere's own shell.
- **Materials follow the Story:** Crema Marfil mirror-polished platform (as built the Future's "mirror" travertine shader);
  mirror-polished bronze seats (as built dark bronze); pearl-lined blades; fluoropolymer pearl outdoors.
- Lighting.json (K3-LIGHT-NATURE): the image luminaire names the real type `light-field-panel.d65`. An image panel is laid over a flat extent or a plan zone; a face curved to the sphere, and the celestial image source that feeds it, wait for Light's image work, so the entry stands as a finding until then.
