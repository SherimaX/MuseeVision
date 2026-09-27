# The Sphere · stage spec

**Stage key:** `sphere` · **module:** `MuseeSphere` (Editor, `Source/Stages/Rooms/Sphere`), class `USphereRoom :
USiteRoom` · **display name:** *The Sphere* (FText) · **level:** 2 · **schedules:** `Data/Stages/Sphere/Finishes.json`,
`Lighting.json`, `Hang.json` (empty tonight).

This is the design sheet the Room Builder builds from tonight (D-016). It covers:
- the Élan Sphere, Ø 28 m, level 2;
- the glass car that never lands, with its two spiral-band masts' ends (the crown and the base housing);
- the pearl neck (the airlock), its irises and the stone platform at the centre;
- the live sky over the museum's city;
- the Élan's exterior pearl dome: the seamless pearl skin, the hairline gilt ring and the gilt ball.

Every number comes from the as-built museum (the frozen `windows/` code, the `ElanSphere`, `ElanCube`,
`FutureAtrium` and `AsBuiltElan` boards, and `plan/README.md`), which D-001 makes the design. D-017 ports the geometry.
Where this sheet re-expresses the old geometry because a rule of the new architecture forces it (solidity, a real
mechanism, the well), the line starts **Re-expressed:**. Questions for the Architect are collected at the end
(Q-S*, R-S*).

**Decisions applied:**
- D-001: the approved pearl dome replaces the old misty lattice dome and lantern.
- D-002: the museum's city (Beijing by default, New York) sets the sky.
- D-003: the museum's sun is idealised outside; the Moon and the stars are always the real ephemeris.
- D-012: the car never lands; its vertical ports close with an iris; the Sphere shows the real sky over the museum's
  city; no stage pushes a sky; no scoped-sky machinery.
- D-014: gilt is a working; joints *welded and faired* for the pearl skin; design targets as luminance order.
- D-016: first pass tonight.
- D-017: port, don't redesign; the designer's choices live in the schedules.
- D-018: material keys name **roles**; the palette names the real material behind each.

**Units and frame.** Metres. x east, y south, z up, true north. Plan positions are given as compass **bearings**
(clockwise from north) unless a line says *plan angle* (from east towards south, as the old code measures it;
bearing = plan angle + 90°). A point at radius r and bearing b is (x, y) = (r sin b, −r cos b).

---

## 1. Identity and placement

**What it is.** A sphere 28 m across sits in the Atrium like a ball in a cup: its equator on the drum at 22 m, its
underside the Atrium's ceiling. The glass car climbs through a pearl neck, sealed by an iris at each end, then up a
gilt mast to a stone platform at the exact centre. That is the only point where an image on a sphere looks true in
every direction. Inside, the building is gone: the live sky for the museum's city is all round you, below your feet
too. There is no sun inside, so by day it shows the stars the sun hides. Outside, the Sphere's upper half is the
Élan's pearl dome with a gilt ball on top.

**Origin.** On the Élan axis at the Atrium's finished-floor level. This is the datum every as-built Élan number is
measured from (MuseePlan.h:160-215), so every height below is the old code's height unchanged.

**Placement in the museum plan** (x east, y south from the Rotunda's centre; z from the museum datum, the Rotunda's
floor = the Atrium floor = 0): the origin sits at **(54.000, 0.000, 0.000)**. Local z = museum z. Site only
translates.

**Named datums** (local = museum z):

| Name | Point | Source |
|---|---|---|
| **C**, the Sphere's centre and the eye at the stop | (0, 0, 22.000) | MuseePlan.h:200 |
| Mating plane (the Atrium collar's top, the neck's foot) | z 7.715 (7.7152) | ElanKit.cpp:22 `CollarTop`; RingHeight 8.3252 − RingDepth 0.58 − 0.03 |
| Atrium ceiling's ring round the opening (r 3.000) | z 8.3252 | MuseePlan.h:208-210 |
| Car at home in the neck (car frame), the mast's foot clamped | z 8.000 (as built 8.400; §6.1) | ElanKit.h:86 `NeckStop`, ElanKit.h:89 `NeckLightZ` 9.6 = the eyes at home |
| The neck's mouth: flange top, the Sphere's floor opening | z 11.400 | ElanKit.h:85 `NeckTop` |
| Platform gate (car frame) | z 17.400 | ElanKit.h:102 `PlatformGate` |
| Platform top = car floor top at the stop | z 20.420 | ElanKit.h:91 `PlatformTop` |
| Car frame at the stop (eyes 22.02) | z 20.400 | MuseePlan.h:203 `TopFloor` |
| Pearl skin's crown | z 36.300 | ElanExteriorStructure.cpp:1135, 1152 |
| Gilt ball's top | z 37.440 | ElanExteriorStructure.cpp:1158-1160 |

**The car frame** (used for every car number): z 0 is the car's *level* on the axis, as built. The floor's top is
+0.020 above it (ElanKit.h:46). This is the convention `cube.md` uses (car frame −23.600 at the Cube's stop).

### 1.1 Footprint (kind **Building**)

**Plan outline:** a regular 96-gon circumscribing r 14.350 (vertex radius 14.358), first vertex at bearing 0°.

**Volume:** the solid of revolution of this (r, z) outline about the axis, counter-clockwise from the axis:

```
(0.000,  7.715) → (2.990,  7.715) → (2.990,  8.364)                      the neck's foot, inside the Atrium's ring
→ arc R 13.960 about (0, 22.000) up to (13.960, 22.000)                   Σ, the lower boundary (1.2)
→ (14.060, 22.000) → (14.060, 22.580)                                     over the drum's glass, up the entablature's back
→ (14.350, 22.5637)                                                       along the entablature's wash
→ (14.350, 22.700) → arc R 14.367 about (0, 22.000) up to (0.600, 36.354) the dome's envelope
→ (0.600, 37.450) → (0.000, 37.450)                                       the boss and the ball
```

In the museum it stacks on the Atrium (same axis). **Site.NoOverlap must compare volumes, not plan outlines**
(cube.md R-11 says the same of the Atrium–Cube stack; R-S11).

### 1.2 The seam with the Atrium (Σ)

The Sphere's lower half is the Atrium's ceiling seen from below. Two stages can't own one surface, so the boundary is
declared:
- **Below the entablature's wash**, the boundary is the sphere **Σ: R 13.960 about C**. The Sphere owns everything
  inside Σ. The Atrium owns everything outside it: its ceiling glass (R 14.000, as built), its grid (R 14.05 … 14.17),
  its ribs (backs at R 13.970), its compression ring (r 3.000 … 3.600, 7.745 … 8.325), its drum and its entablature.
- **Above the wash**, the Sphere owns the pearl skin, the gilt ring, the boss and the ball.
- **Bearing:** the Sphere's steel shell bears on the Atrium's drum head at the equator and on the twelve ribs' backs.
  The bearing pads are the Atrium's pieces and reach Σ. No port is involved.
- **Re-expressed:** as built, the Sphere was one two-sided misty glass surface at R 14.000, its lower half the
  Atrium's roof and its upper half hidden under the dome. The new Sphere is a real enclosure with an image surface
  inside, so its whole build-up (§3.1, §3.2) lies inside R 13.960. Nothing of the Atrium's as-built ceiling moves.
- **Consequence for the Atrium** (R-S5): the Sphere is opaque (the approved pearl dome), so no daylight can reach
  the Atrium's ceiling glass through it. The Atrium's glowing ceiling must be lit by the Atrium's own luminaires.

### 1.3 Enclosures

| Key | Kind | Bounds (local) | Openings |
|---|---|---|---|
| `sphere.interior` | Enclosed | inside the image surface (sphere R 13.700 about C), less the neck's tower (r ≤ 2.980 up to 11.150), the flange and the platform | `sphere.neck.mouth` (Port, internal) |
| `sphere.neck` | Enclosed | the neck's bore r < 2.550, from the port iris (top blade face 7.695) to the neck's upper iris (11.396) | `sphere.shaft.mouth` (Port), `sphere.neck.mouth` (Port, internal) |

- The shell is the welded steel plate (§3.1), lined by the image layer. It is sealed except at the neck.
- `sphere.neck` is the airlock. Its two irises are never open together (§6.6). With the port iris shut or clamped
  and the upper iris shut, it is sealed.
- `sphere.interior` is sealed whenever the upper iris is shut. While the car is up, the upper iris is open and the
  neck joins the interior; the pair is then sealed from the Atrium by the port iris clamped on the mast's foot (§6).
- The car is a moving body, not an enclosure. `RoomAt` inside it returns the enclosure it stands in.

### 1.4 Openings (Form's one list)

| Key | Kind | Geometry (local) |
|---|---|---|
| `sphere.shaft.mouth` | Port | circle r 2.550 (the neck's bore) on the mating plane z 7.715, where the well leaves the stage |
| `sphere.neck.mouth` | Port (internal) | the flange's seat: a regular 96-gon of circumradius 2.460, sides centred on bearings 3.75°k, through z 11.150 … 11.400; closed by the upper iris |
| `sphere.neck.iris.housing` | Housing | the upper iris's pocket in the flange: r 2.460 … 4.100, z 11.371 … 11.397 |
| `sphere.platform.way` | Port (internal) | the platform's seat: the same 96-gon, through z 20.170 … 20.420; closed by the platform iris |
| `sphere.platform.iris.housing` | Housing | r 2.460 … 4.100, z 20.391 … 20.417 |

There is no Glazed and no Open opening. The Sphere never sees the real sky; it shows it (§4.3).

### 1.5 Anchors

- **`sphere.space`** (kind **Space**): the whole image surface, for works made for the full sphere ("Later: new art on
  the whole sphere").
  - Origin C; axes the stage's.
  - Bounds: the ball R 13.700 about C.
  - Named zones: **the platform** (the walkable ring r 2.460 … 5.290 at z 20.420) and **the car at the stop**
    (cylinder r 2.235, z 20.330 … 23.340).
- `Hang.json` binds nothing tonight. While no work performs, the image surface shows the live sky (§4.3).
- There are no Wall, CaseBed, Plinth, Bench, Barrier or Sign anchors.

---

## 2. Ports

The Sphere has **exactly one port**.

| Key | Kind | Class | Frame (local) | Wall depth | Face | Mates with |
|---|---|---|---|---|---|---|
| `sphere.shaft` | Shaft | `shaft.lift.4900.iris` (cube.md's record; used twice) | origin (0.000, 0.000, 7.715) on the mating plane, facing **down** (−z), out of the stage | 0.000 (the neck stands on the Atrium's collar) | flat (horizontal) | `atrium.up`, the Atrium's collar top |

**The well** (the same well as cube.md §2, continued):
- **Section:** circle **r 2.300** about the axis, less two notches 16 mm wide from r 2.265 to r 2.300 at bearings 0°
  and 180°, where the guide rails' blades stand. Identical to `cube.shaft`'s.
- **Extent:** from **z 23.360** (the car's head top at the stop, 23.340, plus 20 mm) down to **z −24.000** (the car's
  lowest point at the Cube's stop, −23.987, plus 13 mm).
- **Serving side: the Sphere.** It owns what moves in the well:
  - the glass car, its roller guides and its lights;
  - the crown with the Cube mast's head (the Cube mast is fed from the crown), and the base housing with the Sphere
    mast's foot;
  - both spiral-band columns;
  - inside its own volume, the neck's rails, the upper iris and the platform iris.
- **Receiving sides:** the **Atrium** (a through-well: from the mating plane 7.715 down through its slab) and the
  **Cube** (below the Atrium's slab soffit, −0.500). Each reserves the prism as a void. Their fit-out stands outside
  the section: tubes, landing enclosure, collar, lining, rails and brackets, glass.
- **Contacts across the seams** are declared (cube.md R-5):
  - the roller guides on the rails;
  - the port iris clamped on the Sphere mast's foot;
  - the Cube's iris clamped on the Cube mast's head.
- **Re-expressed (Q-S1):** `cube.md` assumes the Atrium serves (key `atrium.car`). This sheet was assigned the car.
  The Sphere serving is also the only split in which the upper iris and the platform iris, which close across the
  well, belong to the serving side. The car's sections (§3.7, §3.8, §6) move as a block if the Architect decides
  otherwise.

**The port's closure (Site's joint):** the class's iris, seven blades. It is set in the Atrium's pearl collar with
its top blade face at **z 7.695**, 20 mm below the mating plane, and the stack runs to 7.672.
- Faces: pearl on top (into the throat), gilt underneath (as the Atrium sees it). This is as built: ElanSphereTop.cpp:248-251.
- States: shut, **clamp** (o = 0.378, round the Sphere mast's foot) and open. It publishes `sphere.shaft.closure`.
- It clamps the Sphere mast's foot while the car is at home or above (§6.2), as the Cube's iris clamps the Cube
  mast's head. Both ports of the class `shaft.lift.4900.iris` clamp a mast.

**The joint also draws:**
- the bronze ring on the mating plane: r 2.460 … 2.550, 10 mm, z 7.705 … 7.715;
- a 20 mm closure ring that fills the annulus between the neck's outer face (r 2.980) and the Atrium ring's inner
  face (r 3.000), from 7.715 to 8.364 (Σ). It is a gilt shadow gap, and it keeps the Atrium's light and sound out of
  the Sphere's shell zone.

**What the neighbours must declare to mate** (for the Atrium and Cube drafters and the Site builder):
- **Atrium:** port `atrium.up`, kind Shaft, class `shaft.lift.4900.iris`, frame at museum (54.000, 0.000, 7.715) on
  its collar's top, facing up.
  - It receives the well from 7.715 down to its slab's soffit (−0.500), and its port `atrium.down` passes the same
    well on to the Cube.
  - Its collar houses the port iris: a Housing opening r 2.460 … 4.100, 7.669 … 7.700.
  - Nothing of the Atrium may lie inside the well's section. As built, the Atrium floor's lip at r 2.290 and the landing
    sill's inner edge at r 2.245 do (ElanKit.h:62, MuseePlan.h:333): both move out to r ≥ 2.300 (Q-S3).
  - **Guide rails** through its slab and glass tube, north and south, the same section as the Cube's, from −0.520 up
    to 7.715. As built the car floated here. Without rails it has no drive between the Cube's shaft and the neck
    (Q-S2).
- **Cube:** the well's bottom moves from −23.800 to **−24.000** because the car carries the Sphere mast's foot under
  its floor (§3.7). The mating line becomes "served by the Sphere, through the Atrium". The interlock key
  `atrium.car` in cube.md §6.3 becomes `sphere.car`.

**D-012 topology, for this stage:**
- The Sphere is a leaf. Its one port mates the Atrium's upward vertical port. The Atrium's downward port mates the
  Cube. The Hall of Light's east port mates the Atrium's west door.
- There is no doorway, no light lock and no landing door. The car never lands.
- The live sky is the museum's own, for the museum's city. Nothing pushes a sky.
- `SitePlan.json` gets the mating line `atrium.up ↔ sphere.shaft`, passage 0.000 m, well served by the Sphere.

**On the stand (L_Stand):**
- `sphere.shaft` gets a blind at the mating plane with a **blind well** continuing the prism to −24.000, so the car
  runs its full travel alone.
- The blind carries a stand iris in the port iris's place, so the airlock logic runs.

---

## 3. Pieces by trade

Each piece is one solid of one substance, with its role key `substance.working.history` (D-018; the palette names
the real material). Solids meet by bearing, never by interpenetrating. The small solidity fixes to as-built overlaps
are marked **Re-expressed**.

### 3.1 Steelwork: the Sphere's shell (hidden)

Built as a real thing: a welded steel sphere, like a spherical storage vessel, carried at its equator and on the
Atrium's ribs.

| Piece | Geometry (about C unless stated) | Key |
|---|---|---|
| Plate shell | a sphere R 13.940 … 13.950 (10 mm plate, welded and faired), open only round the neck: its hole's edge welds to the neck's outer face r 2.980 at z 8.372 … 8.382 | `steel.painted.clean` |
| Stiffeners (polish) | meridian and ring T-stiffeners inside the plate, R 13.800 … 13.940, on the image modules' 0.7 m grid | `steel.painted.clean` |
| Absorber (polish) | mineral wool 180 mm behind the modules, R 13.760 … 13.940 (acoustic, §9) | in the image layer's absorption record |
| Skin standoffs (polish) | brackets from the plate (R 13.950) to the pearl skin (R 14.240) on its seam grid, above the wash | `steel.painted.clean` |

**Re-expressed:** new. As built, the Sphere was a surface with the building hidden. The shell is what makes it
sealed (Form.Sealed), acoustically dead (§9) and able to carry the dome.

### 3.2 The image surface (Light's image-luminaire mounts)

- **Faces:** the sphere **R 13.700 about C** (Ø 27.40 m), less the neck's hole: its edge is r 2.980 at z 8.628.
- **Modules** (the image luminaire's housings, drawn by Light from its record): a design depth of 0.060 m
  (R 13.700 … 13.760). They are perforated for sound, laid in rings of latitude.
  - 57 rows of 2.9375° from latitude −77.44° (the neck) to the north pole.
  - Each row has round(2π · 13.700 · cos φ_mid / 0.700) modules.
  - A round cap module closes the pole.
  - Tonight the layer is one continuous image surface (§12); the tiling is polish.
- **Key:** `low-iron-glass.lenslet-array.clean` (the module faces, shaded by Light's diffuser master; the same role
  as cube.md's panels).
- **Re-expressed:** as built there was no inner surface. Its radius is set by the Atrium's ceiling (R 14.000, as
  built) outside it, less Σ, the plate and the modules. The approved exterior's note, "the Sphere inside is a full
  projection surface" (ElanExteriorStructure.h:53), is this surface.

### 3.3 Exterior: the pearl dome (as built, `bPearlShell`)

All lathes about the axis; (r, z) outlines. C = (0, 22.000).

**Pearl skin**, `pearl-steel.lacquered.weathered`: a 60 mm stiffened steel skin, white ceramic coat under a clear
lacquer (as built MI_Ext_Pearl, exterior_materials.py:226-295). Its joints are *welded and faired* (D-014), with seam
lines on the as-built panel grid:
```
(14.2263, 22.625) → (14.2863, 22.625)
→ arc R 14.300 about C up to (0.490, 36.2916)       outer face
→ (0.490, 36.2316)                                   the crown's hole (under the boss)
→ arc R 14.240 about C down to (14.2263, 22.625)     inner face
```
- **Seams** (as built, exterior_materials.py:229-247):
  - meridians every 7.5° of longitude, at bearings 7.5°k, up to latitude 60°, and every 15° above it;
  - parallels every 6° of latitude from the equator (6°, 12°, … 84°).
  - Welded and faired, they read only as faint hairlines under grazing light, never as open 6 mm joints.
- **Re-expressed:** as built the skin started at −0.3° inside the entablature and ran through it. It now starts on
  the gilt ring (z 22.625), which bears on the entablature's wash.

**Hairline gilt drip ring**, `bronze.gilt.weathered`: a 50 mm half-round, 31 mm proud of the skin. Its drip throws the
dome's water clear of the wash (ElanExteriorStructure.cpp:1142-1150):
```
(14.2263, 22.575) → (14.2926, 22.575)
→ half-round centre (14.2926, 22.600), r 0.025, through (14.3176, 22.600) to (14.2926, 22.625)
→ (14.2263, 22.625) → close
```
Its underside rides 4 … 9 mm over the Atrium's wash (z 22.566 … 22.571 under it) on a sealant bed. That bed is a
joint piece. **Re-expressed:** raised 5 mm from as built so it bears on the wash instead of cutting it.

**Gilt boss**, `bronze.gilt.weathered` (ElanExteriorStructure.cpp:1151-1157):
```
(0.000, 36.230) → (0.485, 36.230) → (0.485, 36.2918)        a spigot through the skin's crown hole
→ along R 14.300 about C to (0.550, 36.2894)                 seated on the skin
→ quarter-round centre (0.510, 36.2894), r 0.040, to (0.510, 36.3294)
→ (0.300, 36.420) → (0.200, 36.5869)
→ spherical seat R 0.450 about (0, 36.990) down to (0.000, 36.540) → close
```
**Gilt ball**, `bronze.gilt.weathered`: a sphere R 0.450 (Ø 0.90) about (0, 0, 36.990), top 37.440, seated in the boss.
- **Re-expressed:** as built the boss's flat underside cut 6 cm into the skin and the ball sank into the boss. The
  spigot and the seat keep the silhouette and remove both overlaps.
- The boards' "gilt ball at 40.8 m" belongs to the retired lantern design. The approved pearl dome's ball tops at
  37.44.

### 3.4 The neck (the airlock), as built (ElanSphereTop.cpp:191-256)

| Piece | Geometry | Key |
|---|---|---|
| Throat | annulus r 2.550 … 2.980, z 7.715 … 11.150. It is a welded double-skin steel cylinder; the stiffening between the skins is polish. | `pearl-steel.painted.clean` |
| Gilt courses | six bands r 2.532 … 2.550, 24 mm tall, centred at z 8.1446, 8.5739, 9.0033, 9.8620, 10.2913 and 10.7207. These are eight courses of 0.42935 m from 7.7152, less the one within 0.2 m of the light ring (9.4326). | `bronze.gilt.clean` |
| Ribs | 16, at bearings 11.25° + 22.5°k: radial r 2.526 … 2.550, tangential 22 mm, z 7.715 … 11.150. One solid with the courses (one gilt trellis). | `bronze.gilt.clean` |
| Light ring | Light's luminaire body, r 2.520 … 2.550, z 9.550 … 9.650, in two arcs (bearings 5° … 175° and 185° … 355°, clear of the rail brackets); §4.2 | Light's |
| Flange (the Sphere's floor ring) | z 11.150 … 11.400. Inner edge the 96-gon of circumradius 2.460 (§1.4); outer r 4.200. Its top plate is 3 mm (11.397 … 11.400) over the upper iris's pocket (r 2.460 … 4.100, 11.371 … 11.397). Seven vertical pins at r 3.745. | top: `bronze.patinated.dark`; sides and underside: `bronze.gilt.clean` |
| Rails | north and south (bearings 0°, 180°), cube.md's section: blade r 2.265 … 2.385, 16 mm; foot plate r 2.385 … 2.425, 140 mm wide. z 7.735 … 11.020. | `bronze.brushed.clean` |
| Rail brackets | at z 7.900, 8.900, 9.900 and 10.900: blocks r 2.425 … 2.550, 180 mm wide, 100 mm tall. The courses and ribs stop 10 mm short of them. | `bronze.patinated.dark` |

- **Re-expressed:** as built, the courses and ribs ran 5–6 mm into the throat's face, and the light ring 5 mm into it.
  They now sit on the face.
- **Re-expressed:** the rails are new. As built the car floated in the neck. The rails end 0.13 m under the flange,
  and the car's top guides leave them as it rises onto the mast.

### 3.5 The irises (one design, three places)

The Élan iris (ElanKit.h:68-84, ElanSphereTop.cpp:46-181), identical to cube.md §6.1:

| Quantity | Value |
|---|---|
| Blades | 7, each a 110° arc of the middle circle, 1.250 m wide, with semicircular ends r 0.625 |
| Seat | r 2.460 (the car's glass is r 2.200) |
| Middle circle, open | r 3.095 (inner edge r 2.470, outer r 3.720) |
| Pins | r 3.745 (0.650 outside the middle circle); blade k's pin at plan angle 360°k/7, i.e. bearing 90° + 51.43°k |
| Blade | 5 mm thick; each blade 3 mm below the last (stack 23 mm) |
| Swing | 52° about its pin from open to shut; eased s = 3o² − 2o³ |
| Store | blades reach r 4.01 in travel; pockets to r 4.100 |

**Blade outline**, in its pin's frame (the pin at (3.745, 0) at plan angle 0), counter-clockwise:
- the inner arc r 2.470 from 0° to 110°;
- the far cap: a semicircle r 0.625 about (3.095 cos 110°, 3.095 sin 110°);
- the outer arc r 3.720 back to 0°;
- the pin cap: a semicircle r 0.625 about (3.095, 0).

Inlay lines are 7 mm wide along both long edges: 4 mm inside the inner edge and 11 mm inside the outer.

| Iris | Top blade face | Top face | Underside | Inlay | Note |
|---|---|---|---|---|---|
| **Port iris** (Site's joint, §2) | 7.695 | `pearl-steel.painted.clean` | `bronze.gilt.clean` | none | the airlock's lower door |
| **Upper iris** `sphere.neck.iris` | 11.396 (4 mm under the flange top) | `bronze.gilt.clean` | `pearl-steel.painted.clean` | `bronze.patinated.dark` | the airlock's upper door and the Sphere mast's clamp |
| **Platform iris** `sphere.platform.iris` | 20.416 (4 mm under the stone) | `cream-marble.polished.clean` (veneer) | `bronze.gilt.clean` | `pearl-steel.painted.clean` ("gilt mirrored the black sky and read as cracks") | shut, it is a full stone disc to walk on; a spiral of fine pearl lines |

### 3.6 The platform, as built (ElanSphereTop.cpp:272-369, ElanKit.h:90-93)

Platform frame: top **z 20.420**. Radii from the axis.

| Piece | Geometry | Key |
|---|---|---|
| Deck | ring r 2.460 (the 96-gon seat) … 5.498, z 20.170 … 20.390. Beyond r 4.100 it runs to 20.390; inside, it stops at 20.391 under the platform iris's pocket. Steel. | `steel.painted.clean` (hidden) |
| Soffit and fascia | the deck's underside (20.170) and its outer face r 5.498 below the nosing | `pearl-steel.painted.clean` |
| Stone floor | r 4.100 … 5.498, 30 mm (20.390 … 20.420), in 24 slabs of 15°, radial joints 1 mm at bearings 7.5° + 15°k (the west axis is a slab's centre) | `cream-marble.polished.clean` |
| Pocket lid | r 2.460 … 4.100, 3 mm (20.417 … 20.420): a stone veneer on a bronze plate over the platform iris's pocket (20.391 … 20.417), in 16 segments with joints at bearings 11.25° + 22.5°k | `cream-marble.polished.clean` |
| Nosing | r 5.498 … 5.512, z 20.360 … 20.423 (3 mm proud) | `bronze.gilt.clean` |
| Balustrade shoe | r 5.290 … 5.350, z 20.420 … 20.480, on the stone | `bronze.gilt.clean` |
| Posts | 32, 25 × 25 mm, centred on r 5.320 at bearings 95.625° + 11.25°k, z 20.480 … 21.442 | `bronze.gilt.clean` |
| Glass | 32 curved panes on r 5.320 between the posts, 12.76 mm laminated (faces r 5.3136 … 5.3264), z 20.480 … 21.440. Each runs from 5 mm clear of one post to 5 mm clear of the next. | `low-iron-glass.laminated.clean` |
| Handrail | a tube Ø 56 mm (r 0.028) on the circle r 5.320, centre z 21.470 (the rail's top at 1.078 above the floor) | `bronze.gilt.clean` |
| Rail light | Light's luminaire body r 5.270 … 5.292, z 21.425 … 21.440, inside the rail line, on 32 brackets under the rail; §4.2 | Light's |
| Struts | 8 round bars Ø 0.080 at bearings 22.5° + 45°k. Each runs from (r 3.350, z 11.400) on the flange top to (r 4.400, z 20.170) under the deck, with gilt end plates Ø 0.160 × 10 mm. | `bronze.gilt.clean` |

- **Re-expressed:** as built the 32 posts stood in the middle of eight 45° panes, and the struts' tops ran 2 cm into
  the slab. The glass is now 32 panes held between the posts, and the struts end on the deck's soffit. The silhouette
  is unchanged.
- **Walkable:** the ring from the seat (r 2.460) to the shoe (r 5.290), and the shut platform iris, which is 4 … 22 mm
  below the stone in 3 mm steps.
- The struts are the only support. Q-S6 asks for their structural check; as built they are slender.

### 3.7 The glass car, as built (ElanKit.h:40-56, 94-104; ElanCar.cpp:20-187)

Car frame (z 0 = the level; floor top +0.020). Radii from the car's axis. The door faces **west** (bearing 270°).

| Piece | Geometry | Key |
|---|---|---|
| Floor | disc r ≤ 2.130, z −0.030 … +0.020, frosted laminated glass (it is the visitor's moving base) | `low-iron-glass.acid-etched.clean` |
| Base ring | r 2.130 … 2.235, z −0.070 … +0.025; it carries the floor's edge in a rebate | `bronze.gilt.clean` |
| Top ring | r 2.130 … 2.235, z 2.530 … 2.600 | `bronze.gilt.clean` |
| Fixed panes | 4 curved panes on r 2.200, 17.52 mm laminated (faces 2.1912 … 2.2088), z 0.025 … 2.530, spanning bearings 294° → 2°, 2° → 90°, 90° → 178° and 178° → 246°. Green glass edges at the joints and jambs. | `low-iron-glass.laminated.clean` |
| Leaves | 2 curved leaves on r 2.150, the same glass, z 0.025 … 2.530. Closed: north leaf 270.01° → 295°, south leaf 245° → 269.99°. Each has a gilt leading-edge fin 12 × 25 mm. | glass `low-iron-glass.laminated.clean`; fin `bronze.gilt.clean` |
| Roof | a clear pane r ≤ 2.130, z 2.5655 … 2.583, in the top ring | `low-iron-glass.laminated.clean` |
| Light ring | Light's luminaire body r 2.060 … 2.128, z 2.490 … 2.528, tucked under the top ring; §4.2 | Light's |
| Handrail | a glass rod Ø 36 mm on the circle r 2.000 at z 0.970, from bearing 300° clockwise through 0°, 90° and 180° to 240°, with spherical ends. Five gilt brackets 12 × 12 mm, r 2.018 … 2.1912, at bearings 330°, 30°, 90°, 150° and 210°. | rod `low-iron-glass.laminated.clean`; brackets `bronze.gilt.clean` |
| Roller guides | at bearings 0° and 180°, on both rings: r 2.235 … 2.290, ±0.045 tangential, the rings' heights, each slotted r 2.265 … 2.290 × 20 mm for the rail blade | `bronze.gilt.clean` |
| Crown (the Cube mast's housing) | drum r 0.340, z 2.583 … 2.768; bead torus r 0.012 on r 0.328 at 2.768; cap disc r 0.316, z 2.756 … 2.780; skirt r 0.336 … 0.352, z 2.583 … 2.613; four arms on bearings 45°, 135°, 225° and 315°, r 0.310 … 2.225, ±0.022, z 2.600 … 2.632, bearing on the top ring | `bronze.gilt.clean` |
| Cube mast's head (rides on the crown) | lathe from the crown top 2.780 (CubeWay.cpp:38-50): (0, 0), (0.248, 0), (0.260, 0.012), (0.260, 0.064), (0.254, 0.070), (0.195, 0.070), (0.195, 0.110), (0.254, 0.110), (0.260, 0.116), (0.260, 0.130), then a quarter arc to (0, 0.160). The head top is 2.940. | `bronze.gilt.clean` |
| Base housing (the Sphere mast's housing) | drum r 0.340, z −0.227 … −0.030; skirt r 0.336 … 0.352, z −0.060 … −0.030; four arms on the diagonals, r 0.310 … 2.125, ±0.022, z −0.062 … −0.030, under the floor | `bronze.gilt.clean` |
| Sphere mast's foot (rides under the housing) | the head's profile mirrored downward from −0.227: collar r 0.260 −0.227 … −0.297; groove r 0.195 −0.297 … −0.337; cap r 0.260 to −0.357, then a quarter arc to (0, −0.387) | `bronze.gilt.clean` |

- **Re-expressed:**
  - The floor is r 2.130, not 2.135, so it sits in the base ring instead of running into it.
  - The panes run between the rings. As built they ran 5 mm into each.
  - The light ring is tucked under the top ring. As built it ran 1 cm into it.
- **Re-expressed:** the base housing replaces the as-built collar (r 0.200, −0.160 … −0.030). The Sphere mast needs a
  real housing (Motion.Housed). It mirrors the crown, as the board says: "the Sphere's mast upside down".
- **Heights:** the car's highest point is the head top, +2.940. Its lowest point is the foot, −0.387.
- **Mass:** from Form's volumes × Matter's densities (Motion.Balanced); there is no counterweight.

### 3.8 The spiral-band columns (Motion's bodies, Form-drawn)

- **The Sphere mast:** a spiral-band column r 0.170, seam pitch 0.045 m, as the Cube's (cube.md §6.2).
  - It is fed out of the base housing between the housing's underside and the foot's collar.
  - Its exposed length is (car frame − 11.702): 0 at the clamp level, 8.698 m at the stop.
  - Its foot is clamped by the upper iris (§6.2).
- **The Cube mast:** fed from the crown, clamped at its head by the Cube's ceiling iris (cube.md §6.2). Its exposed
  length is 0 at the Cube's clamp level and 12.785 m at the Cube's stop.
- Both columns are drawn as one course piece (one band pitch, r 0.170), **instanced** by Motion up to the exposed
  length, so the seams stand still in the world as the column is fed. Key `bronze.gilt.clean` (Q-S9).
- **Re-expressed:** as built, the Sphere mast was an engine cylinder Ø 0.40 scaled to reach from the flange's
  underside (11.15) to the car, bearing on nothing. It becomes the Cube's column type, standing on its clamped foot.

### 3.9 What this stage does not build

- **The Atrium builds:**
  - its drum, ribs, ceiling glass and grid, compression ring and collar (with the port iris's pocket);
  - its landing enclosure and glass tube, with guide rails from its slab to the collar;
  - its floor with the waiting ring;
  - the exterior plinth, drum order and entablature, with its wash (the pearl skin's seat).
- **Site's joint builds:** the port iris, the bronze ring on the mating plane and the 20 mm closure ring.
- **The Cube builds:** its shaft fit-out and its ceiling iris (by the joint), which clamps the Cube mast's head.
- **Light builds:** the image modules' housings and the luminaires' bodies from its records.

### Materials used by the Sphere's own pieces

| Key | Where | Matter status tonight |
|---|---|---|
| `pearl-steel.lacquered.weathered` | the exterior pearl skin | substance `pearl-steel` (white family) and working `lacquered` **missing**; as-built evidence: linear albedo (0.80, 0.79, 0.76), base roughness 0.30, clear coat roughness 0.07 (exterior_materials.py:282-285) |
| `pearl-steel.painted.clean` | neck throat, platform soffit and fascia, port iris top, upper iris underside, platform iris inlay | **missing**; as built #ECE9E3, roughness 0.32 (native.py:161-176) |
| `bronze.gilt.clean` | car rings, fins, brackets, guides, crown, head, housing, foot, masts; neck courses and ribs; flange sides; platform nosing, shoe, posts, handrail, struts; iris undersides | exist (gilt over bronze, D-014) |
| `bronze.gilt.weathered` | the drip ring, boss and ball (exterior) | history `weathered` for gilt: **to confirm** (bronze/gilt wear exist) |
| `bronze.patinated.dark` | flange top, upper iris inlay, rail brackets | as cube.md; dark regime to confirm |
| `bronze.brushed.clean` | the neck's rails | as cube.md |
| `cream-marble.polished.clean` | platform floor, pocket lid veneer, platform iris tops | substance exists; the Future's working "polished" (as built: the Rotunda's cream marble, glass-smooth, materials.py:1262-1269) |
| `low-iron-glass.laminated.clean` | car panes, leaves, roof and rod; balustrade panes | exist. The palette may add the anti-reflective working (Q-S8). |
| `low-iron-glass.acid-etched.clean` | the car's floor (frosted) | working `acid-etched` **missing** |
| `low-iron-glass.lenslet-array.clean` | the image modules' faces | as cube.md: working **missing** |
| `steel.painted.clean` | the plate shell, deck (hidden) | exist (`steel`, `painted.steel`) |

---

## 4. Light

### 4.1 Daylight

**None.** The Sphere has no Glazed or Open opening. The exterior pearl dome is lit by the museum's one sun and sky
(§5). No daylight enters.

### 4.2 Luminaires (type, count, position; never lumens)

| Mount | Type (catalogue key) | Count | Position (local) | Aim | Control class |
|---|---|---|---|---|---|
| The image surface | `image.light-field-panel.700.curved`: image kind, light-field (**catalogue gap**; the curved variant of cube.md's `image.light-field-panel.700`) | the module grid of §3.2 (≈ 4,800) | R 13.700 about C | into the Sphere | image |
| Under the handrail | `linear-handrail.3000k` (**catalogue gap**: a handrail-integrated linear LED, asymmetric, full cut-off above the horizontal; as built 3000 K) | 32 runs, one per glass pane | r 5.281, z 21.432, between the posts | down and in, onto the floor ring | circulation |
| The car's roof ring | `linear-slot.4000k` (**catalogue gap**; opal, curved; as built 5000 K) | 4 quarter arcs | r 2.094, z 2.509 (car frame), under the top ring | down onto the car floor | circulation (lift) |
| The neck's light ring | `linear-slot.4000k` (**catalogue gap**; as built 6500 K) | 2 arcs of 170° | r 2.535, z 9.600, the eyes of a visitor waiting in the neck | a luminous band, facing in | circulation |

Retired, as physically false:
- the platform's fill (a 3,000 lm rect light 9 m up, "an emitter no one sees", ElanSphereTop.cpp:328-341);
- the as-built point lamps and glow materials, which the luminaire records replace;
- the "building hidden" switch (ElanElevator.cpp:374-386).

### 4.3 The image luminaire and its source: the live sky

- **The luminaire record** needs design values: modules 0.700 m, curved to R 13.700.
  - Light-field emission: each point's radiance depends on direction, so content at infinity holds still from any
    eye in the Sphere. That keeps the board's "a sky that holds still … as you rise".
  - Native black ≤ 1 × 10⁻⁴ cd/m²; face reflectance ≤ 1 % (a black lenslet mask); peak ≥ 3,000 cd/m².
  - Evidence: design-stage, after Light Field Lab SolidLight, as cube.md's.
- **The image source, the celestial sphere,** is an `ILightImageSource` that Light owns: Light renders the sky, and
  this is the same sky seen above the atmosphere. It draws the whole celestial sphere, below the horizon too, for
  Time's `FTimeSky` of the museum's observer:
  - **Stars:** the Yale Bright Star Catalogue, 5th revised edition (VizieR V/50), every star to V 6.5, from
    `assets/sky/stars.bin` (9,096 records: RA, Dec, V, B−V). Each is a point at its true illuminance at the eye from
    V, coloured from B−V. As built: MuseeSky.cpp:246-310.
  - **The Moon:** its disc at its true angular size, position and phase (Meeus), at the full Moon's surface luminance
    (≈ 2.5 × 10³ cd/m²) scaled by phase.
  - **The Milky Way:** NASA SVS *Deep Star Maps 2020*, plate carrée in equatorial J2000
    (`windows/SourceArt/Textures/T_milkyway.png`), calibrated in cd/m². Stars brighter than V 6.5 are removed from
    it so they aren't counted twice (R-S7). As built: MuseeSky.cpp:312-343.
  - **No Sun, no atmosphere, no airglow.** "There is no sun inside."
- When a future work performs in `sphere.space`, its own image source replaces the sky on the same luminaire. Nothing
  pushes a sky (D-012).

### 4.4 Targets

- **Care targets:** none. No object is hung in the Sphere.
- **Use targets:**

| Surface | Class | Value | Note |
|---|---|---|---|
| Platform ring (walkable, r 2.460 … 5.290, z 20.420) | proposed `assembly.dark` (**gap** in `UseTargets.json`: IBC 2021 §1008.2.1 exception, 0.2 fc = 2.15 lx at the walking surface in darkened assembly spaces) | **2.15 lx** maintained, uniformity 0.4 | the `orientation` (5 lx) and `escape-route` (10.76 lx) classes would drown the sky; Q-S4 |
| Car floor while moving or with doors shut | `lift` | 100 lx | EN 12464-1 row, as in `UseTargets.json` |
| Car floor at the Sphere's stop, doors open | `assembly.dark` | 2.15 lx | by channel, §4.6 |
| Neck throat at the light ring (the car in the neck) | `circulation` wall minimum | 75 lx on the throat's face | presence-linked |

### 4.5 Design targets (luminance order between named surfaces; measured by `Light::MeasureLuminance`, not knobs)

From the eye at C at the stop, the car's doors open, the platform empty:
1. *the Moon's disc* > *any luminaire's diffuser seen from a standing eye on the platform*. The handrail runs are
   shielded under the rail, so none is seen directly.
2. *the platform stone* (≈ 0.4 cd/m² at 2.15 lx) > *the gilt rail, posts and struts* > *the car's frosted floor*.
3. *the Milky Way's bright band* (Sagittarius to Cygnus, ≈ 10⁻³ cd/m²) > **the image surface's background**, which is
   its black level plus every luminaire's spill. The spill onto the shell must stay below one third of the Milky Way's
   mean surface brightness.
   - This is a physical test, and it is expected to fail at 2.15 lx: the spill is about 1 × 10⁻⁴ … 4 × 10⁻⁴ cd/m².
   - The fix is never a number in this sheet. It is the platform's class (Q-S4), the handrail's cut-off, or the
     modules' reflectance.
4. From the Atrium: nothing of the Sphere's interior is ever seen. The port iris is shut or clamped whenever the
   upper iris is open.

What the Eye should then read (an expectation for `Visitor::MeasureExposure`, not a setting): about EV100 1 to 2 on
the platform, adapting down over a minute. The stars read as points at any exposure. The Milky Way appears only if
the platform goes dark (Q-S4).

### 4.6 Operation (Light's `Operation.json`, by channel)

- **Image surface:** always on, sky content, day and night (D-006).
- **Handrail runs:** on while a body is in `sphere.interior`. They hold `assembly.dark` and never go brighter.
- **The car's ring:** follows `sphere.car.state`:
  - `lift` (100 lx) while moving, with the doors shut, or at the Atrium;
  - `assembly.dark` at the Sphere's stop once the doors are open.
- **The neck's ring:** on while the car is in the neck or passing it.
- Nothing is dimmed by hand. Commissioning sets every level against these targets.

---

## 5. Time and weather

- **Glazed openings:** none, so no weather place falls on the interior.
- **The live sky** is Time's `FTimeSky` for the museum's city: Beijing (39.9° N) by default, New York by the city
  setting (D-002).
  - The star field turns with local sidereal time. The Moon's position and phase come from the Meeus almanac.
  - The sky follows `time.hour` and `time.rate` like every other clock, so the hour key turns the sky.
  - D-003's idealised sun doesn't apply inside: the Sun is never drawn.
- **The exterior dome** takes the **museum** observer's weather (D-005 default):
  - Matter's surface law wets the lacquer and carries rain runs down it, heavier low on the dome where more water has
    passed (as built).
  - Snow lies on the crown where Form's slope and ponding facts allow.
  - The drip ring sheds onto the Atrium's wash.
- **Sun on the dome:** the museum's one sun, idealised: 15° an hour from 06:00 in the west through north to 18:00 in
  the east (D-003). The pearl dome takes the sun's warm light and the sky's reflections sliding over its lacquer.

---

## 6. Motion

### 6.1 The car (`sphere.car`): one prismatic body on the axis

| State | Car frame z | What |
|---|---|---|
| `cube` | −23.600 | eyes at the Cube's centre (cube.md) |
| `cube.clamp` | −10.825 | the Cube mast's head in the Cube iris (cube.md) |
| `atrium` | 0.000 | the Atrium landing; doors open with the landing's leaves |
| `neck` | 8.400 | **home**: the car waits here with both irises shut |
| `neck.clamp` | **11.702** | the Sphere mast's foot groove in the upper iris |
| `sphere.gate` | 17.400 | the head 53 mm under the platform iris's stack |
| `sphere` | 20.400 | floor top level with the platform (20.420), eyes 22.020 |

- **Drives:**
  - **Linear.** Between `cube.clamp` and `neck`, a ropeless linear-motor drive on the two rails: 1.5 m/s, 0.75 m/s²
    (MuseePlan.h:204-205), jerk ≤ 1.5 m/s³ (Motion.Comfortable).
  - **Sphere mast.** Between `neck.clamp` and `sphere`, the base housing feeds the column: 1.5 m/s, 0.75 m/s².
  - **Cube mast.** Below `cube.clamp`, the crown feeds the Cube mast: 1.0 m/s, 0.5 m/s² (cube.md §6.2).
  - **Re-expressed:** as built the car simply moved. The drives above are how the as-built motion is carried out.
- **Travel volume:** the well's section, from −24.000 to 23.360.
- **`neck.clamp` derivation:** the upper iris's stack mid-plane is 11.3845 (11.373 … 11.396). The foot's groove mid is
  0.317 below the car frame (0.227 + 0.090). So 11.3845 + 0.317 = 11.7015, rounded to **11.702**. The groove
  (11.405 … 11.365) clears the stack by 9 mm above and 8 mm below.

### 6.2 The Sphere mast (`sphere.mast`)

- A fed column (§3.8). Its foot is a body that rides under the base housing until `neck.clamp`, where the upper iris
  closes to **clamp** (o = 0.378, leaving r 0.197 round the groove r 0.195). The foot then stays in the iris.
- Above `neck.clamp`, the column's exposed length = car frame − 11.702, up to 8.698 m at `sphere`.
- Going down, the column feeds back in. At `neck.clamp` the iris opens and the foot rides down with the car.
- **Re-expressed:** this adds a stop that the as-built car didn't make (Q-S5). It is the Cube's mechanism mirrored,
  and the board says the Cube's mast is "the Sphere's mast upside down".

### 6.3 The car's doors (`sphere.car.leaf.north`, `.south`)

- Two curved leaves, each a revolute joint about the **car's axis**. Each turns 24° away from the west centre line
  into its pocket, behind the fixed panes (north leaf to 294° → 319°, south to 221° → 246°).
- 1.5 s from shut to open, eased (ElanElevator.cpp:39).
- They open only at `atrium` (with the Atrium's landing leaves, bound by key) and at `sphere`.

### 6.4 The irises

- Motion's **iris machine element** (§3.5), three bodies of seven blades.
- 4.0 s from shut to open (ElanElevator.cpp:40). The clamp takes 0.622 × 4.0 = 2.49 s from open.
- States: **shut** o = 0, **clamp** o = 0.378, **open** o = 1.
  - `sphere.neck.iris` uses all three.
  - `sphere.platform.iris` uses shut and open.
  - The port iris (the joint's) uses shut and open.
- **Housed:**
  - The upper iris's blades move only within r 2.460 … 4.100 and 11.371 … 11.397.
  - The platform iris's within r 2.460 … 4.100 and 20.391 … 20.417.

### 6.5 Sequences and times (from the rules above; Motion adds ≈ 0.5 s per start and stop for the jerk limit)

| Ride | Steps | Time |
|---|---|---|
| **Atrium → Sphere**, platform empty | doors shut 1.5 · run 0 → 8.400 in 7.60 · port iris shuts 4.0 · upper iris opens 4.0 · run 8.400 → 11.702 in 4.20 · upper iris clamps 2.49 · run 11.702 → 20.400 in 7.80 (the platform iris opens meanwhile) · doors open 1.5 | stops at **31.6 s**, doors open by 33.1 s (board: 27 s; Q-S5) |
| **Called from the Atrium** (car at home) | port iris opens 4.0 · run 8.400 → 0 in 7.60 · doors 1.5 | **13.1 s** (board: "about 13 s") |
| **Called from the platform** | upper iris opens 4.0 · run to 11.702 in 4.20 · clamp 2.49 · run to 17.400 in 5.80 · wait: platform iris opens 4.0 · run to 20.400 in 4.00 · doors 1.5 | 26.0 s |
| **Sent away** (the visitor stays on the platform) | doors shut 1.5 · run to 17.400 in 4.00 · wait: platform iris shuts 4.0 (the floor is whole after 9.5 s) · run to 11.702 in 5.80 · release 2.49 · run to 8.400 in 4.20 · upper iris shuts 4.0 | 26.0 s |
| **Sphere → Atrium** (the visitor rides) | doors 1.5 · run to 11.702 in 7.80 · release 2.49 · run to 8.400 in 4.20 · upper iris shuts 4.0 · port iris opens 4.0 · run to 0 in 7.60 · doors 1.5 | 33.1 s |
| **Left at the Atrium** | doors open, nobody aboard for 12 s (ElanElevator.cpp:634-639) → home to the neck | |

### 6.6 Interlocks (by key; Site binds the port iris, the Atrium's landing leaves and the Cube's iris)

| # | Rule | Keys |
|---|---|---|
| S-I1 | The car moves only with its doors shut. The doors open only at `atrium` or `sphere`. They hold or reverse while a body is in the doorway (west of the axis, within 1.25 m of the centre line, r 1.85 … 2.80). | `sphere.car`, its leaves |
| S-I2 | The car crosses the mating plane (its body in 7.695 − 2.94 … 7.695 + 0.39) only with the port iris **open** (o ≥ 0.999). | `sphere.car`, `sphere.shaft.iris` |
| S-I3 | **Airlock:** the port iris and the upper iris are never both off shut, except while the upper iris is clamped with the port iris shut. | both irises |
| S-I4 | The car's head rises above 11.368 (car frame > 8.428) only with the upper iris open. | `sphere.car`, `sphere.neck.iris` |
| S-I5 | The upper iris leaves **open** for **clamp** only with the car at 11.702 ± 0.005. The car leaves 11.702 upward only with the iris at clamp (±0.002), and downward only with it open. Above 11.707 the iris stays at clamp. | `sphere.car`, `sphere.mast`, `sphere.neck.iris` |
| S-I6 | The car frame exceeds 17.400 only with the platform iris open (as built, ElanKit.h:101-102). | `sphere.car`, `sphere.platform.iris` |
| S-I7 | The platform iris is open or opening while the car is below 17.400 **only if no body is on the platform ring**. Otherwise the car waits at `sphere.gate` and the iris opens only then. Going down, if a body is on the platform, the car waits at `sphere.gate` until the iris is shut. | `sphere.car`, `sphere.platform.iris` |
| S-I8 | **Occupancy guard:** the platform iris won't open while a body stands within r < 2.460 + the capsule radius ("Step off the centre: the car is coming up."). Closing, it holds or reverses if a body enters its travel slice. | `sphere.platform.iris` |
| S-I9 | Across the seams: the Atrium's landing leaves open only with the car at `atrium`. The Cube's I-1 … I-4 apply with key `sphere.car`. | Site bindings |

**Re-expressed:** S-I7 and S-I8 replace the as-built invisible "guard" colliders round the car's way
(ElanSphereTop.cpp:365-367, 376-384). While the platform iris is open, the car fills the opening or lies 0.38 m
under it; otherwise nobody is on the platform.

### 6.7 Clearances (Motion.Clears ≥ 5 mm unless a declared contact)

| Pair | Clearance |
|---|---|
| Car glass r 2.200 ↔ iris seats and flange/platform seats r 2.460 | 0.260 |
| Car rings r 2.235 ↔ neck throat r 2.550 | 0.315 |
| Roller guides r 2.290 ↔ well boundary r 2.300 | 0.010 (as built) |
| Roller guides ↔ rail blades | contact (declared, cube.md R-5) |
| Head top at `neck` (11.340) ↔ upper iris stack (11.373) | 0.033 |
| Head top at `sphere.gate` (20.340) ↔ platform iris stack (20.393) | 0.053 |
| Foot bottom at `neck` (8.013) ↔ port iris (7.695) | 0.318 |
| Foot groove walls ↔ upper iris stack at clamp | 0.009 above, 0.008 below |
| Clamped blades r 0.197 ↔ groove r 0.195 | 2 mm: a declared clamp contact (cube.md R-5) |
| Car ↔ struts (r ≥ 3.27 at every height) | ≥ 0.98 |
| Car glass r 2.235 ↔ the platform's edge r 2.460 at the stop | 0.225: the step across (Q-S3) |

### 6.8 Channels declared by this stage

| Channel | Published by | Meaning |
|---|---|---|
| `sphere.car.state` | Motion (the car) | the named state or `moving`, the doors' opening, the car frame z |
| `sphere.neck.iris.state` | the upper iris | o (0 shut, 0.378 clamp, 1 open) |
| `sphere.platform.iris.state` | the platform iris | o |
| `sphere.shaft.closure` | the port iris (joint) | o |

---

## 7. Nature

- **Plants:** none. **Water:** none. **Ground:** none. The Sphere stands on the Atrium.
- **Precipitation** falls on the exterior dome (the Grounds' Outdoors) and stops at the sealed skin. Nothing falls
  inside.
- **Growth space:** not applicable.

---

## 8. Art

- **No works** from the collection are shown in the Sphere. *The Starry Night* (73.7 × 92.1 cm, MoMA) is the Atrium's,
  on its stele by the car's door.
- **The live sky is not a Work.** It is Light's celestial image source on the image luminaire (§4.3).
- **`sphere.space`** (Space, §1.5) is reserved for works made for the full sphere. Not built:
  - "new art on the whole sphere", to be planned;
  - the proposed opening piece, *The sky becomes The Starry Night* (the ElanSphere board: "proposed, not built").
- **Frames, cases, casts, scans:** none. **Hang rule:** none tonight.

---

## 9. Sound

**Acoustic character:** a hush. The image layer is perforated and backed by 180 mm of mineral wool on the steel
plate, as a planetarium dome is. The sphere's focusing is absorbed, so no echo returns to the centre.
- `sphere.interior`: V ≈ 10,750 m³ (the ball R 13.700, less the neck's tower, the flange and the platform); image
  layer ≈ 2,331 m².
- The layer's absorption per octave (125 Hz … 4 kHz) belongs in its Matter record: panel plus wool plus plate,
  expected about 0.35, 0.75, 0.95, 0.95, 0.90 and 0.85.
- Sound computes RT60 from these (Sound.ReverberationFromMatter). The expected result is ≈ 0.4 s (Eyring) to 0.8 s
  (Sabine) at mid frequencies.
- `sphere.neck`: a small hard pearl cylinder (V ≈ 75 m³), bright, short.
- **Emitters (as built there were none):**
  - **Footfalls:** on polished marble (hard, crisp, no echo), the pocket lids and the iris blades (stone over bronze),
    and the car's frosted glass floor.
  - **Mechanism voices** from `FMuseeActuation`, attached by key:
    - the three irises (a slow blade slide, 4 s, and a soft seat at the end; the clamp's lighter click);
    - the car's leaves (1.5 s glide);
    - the linear drive (a faint low hum while moving);
    - the column feed (a soft ratchet-whirr, 1.5 m/s).
    - They need recorded evidence at a measured SPL; this is polish if none is on disk.
- **Occlusion:** the sealed shell and the airlock (Sound.Occluded). Rain on the pearl skin is not heard inside.
- **The mix:** no music, no ambience bed. The quiet is the design.

---

## 10. Visitor

- **Getting there:**
  - The car takes you from the Atrium. The first ride goes down to the Cube before it goes up (as built):
    *Up to the Sphere* is offered at the Atrium only once the Cube has been visited.
  - From the Cube's stop, *Up to the Sphere* goes straight through.
- **Riding:** the body rides the car's floor as a moving base. Its movement ticks after the car's (Visitor: riding
  moving floors). The frosted floor and the glass rail stay with you the whole way.
- **Uses** (`IMuseeUsable`: verb, caption, availability and reason):

| Where | Use | Available when |
|---|---|---|
| In the car at `atrium`, doors open | *Up to the Sphere* | the Cube has been visited in this visit |
| In the car at `cube`, glass back | *Up to the Sphere* | no work is performing |
| In the car at `sphere`, doors open | *Down to the Atrium* | always |
| On the platform | *Send the car down* | the car is at `sphere` with its doors open, and you are not in it |
| On the platform | *Call the car* | the car is not at `sphere` and not already coming up |

- **Captions** (FMuseeText, as built, ElanElevator.cpp:243, 365-370, 584, 614):
  - "…then up to the stars. A steady 1.5 metres a second."
  - "Up through the earth, then the Atrium, to the stars."
  - "The centre of the Sphere: the sky over you, now. Step out onto the platform."
  - "The car sinks back into the neck, and the Sphere is yours."
  - "The car rises out of the neck to meet you."
  - "Step off the centre: the car is coming up."
- **What one does:** step out onto the stone ring, walk it (r 2.46 … 5.29), look up, round and down past the rail, and
  stand at the exact centre in the car. The hour key turns the sky. In the headset the sky is true from wherever you
  stand (light field).
- **Seats:** none (as built).
- **The Eye:** it adapts from the lit car and neck to the dark Sphere at human speeds, down to about EV100 0. The
  tone curve's highlight shoulder keeps the Moon's disc from clipping.
- **Never trapped:** on the platform with the car away, *Call the car* is the way out (D-014: a usable control is
  not a trap). Site.Reachable judges it over the state graph.

---

## 11. The old code map (re-express, don't copy)

`windows/` = `C:\Users\anan9\Desktop\Projects\Musee Vision\windows`.

- **Plan numbers:** `Source/MuseeVision/Plan/MuseePlan.h`
  - 160-215: the Sphere, ring, top floor, speed and acceleration, `Underside()`, `RingHeight()`;
  - 217-256: the exterior's radii, the entablature, the old dome and crown.
- **The kit:** `Source/MuseeVision/Elan/ElanKit.h`
  - 22-56: the car;
  - 57-66: the landing (the Atrium's);
  - 68-93: the neck, irises and platform;
  - 94-104: the crown, head, guides and gate.
- `Source/MuseeVision/Elan/ElanKit.cpp`:
  - 22: `CollarTop`;
  - 24-42: the panes' spans;
  - 46-192: the pane, band, tube, ball, block, rod and polygon-ring helpers (Form's language now);
  - 201-260: the glass and metal parameters (evidence only; D-018);
  - 327-386: fixed glass, leaves, `TurnLeaves`.
- `Source/MuseeVision/Elan/ElanSphereTop.cpp`:
  - 18-32: lamps (retired);
  - 46-164: the iris blade's outline, inlay and stack;
  - 166-181: its easing;
  - 191-256: the neck;
  - 272-369: the platform;
  - 371-393: iris, guard and lit (the guard is retired, S-I7/S-I8).
- `Source/MuseeVision/Elan/ElanCar.cpp`:
  - 20-187: the car's build;
  - 194-274: place, doors, the Cube's clearing, glow;
  - 284-407: the Atrium's landing and collar (the neighbour's; the collar's pocket for the port iris).
- `Source/MuseeVision/Elan/ElanElevator.cpp`:
  - 36-46: door, iris and dim times, the Cube's speeds;
  - 99-128: home in the neck;
  - 203-228: in-car, on-platform and doorway tests;
  - 237-372: calls, prompts and captions;
  - 374-386: the "building hidden" switch (retired);
  - 441-453: legs through the neck;
  - 480-693: the tick: the airlock, the Cube's iris, the platform gate, the as-built mast.
- `Source/MuseeVision/Elan/ElanStructure.cpp`:
  - 374-400: the old Sphere shells (retired; R 14 is the Atrium's ceiling now);
  - 409-435: the rib's back line, R 13.97 (the Atrium's, the seam Σ);
  - 498-503: the compression ring; `ElanStructure.h:46` is `RingDepth`.
- `Source/MuseeVision/ElanExterior/ElanExteriorStructure.h`: 39-74 (the description) and 216-223 (`bPearlShell`).
- `Source/MuseeVision/ElanExterior/ElanExteriorStructure.cpp`:
  - 24-56: constants;
  - 327-341: points on the sphere, bearings;
  - 365-377: the entablature section (the Atrium's; the wash the gilt ring sits on);
  - 979-1041: the night washes (the Atrium's cornice; polish);
  - 1130-1162: **the pearl skin, the gilt ring, the boss and the ball**.
- **Scripts** (evidence only, D-018):
  - `windows/Scripts/exterior_materials.py:226-295`: the pearl (the panel grid, joints, lacquer, rain runs);
  - `windows/Scripts/materials.py:1262-1269`: the Future's floor, cream marble glass-smooth;
  - `windows/Scripts/native.py:161-176`: the pearl rib paint;
  - `windows/Scripts/setup_project.py:194-252`: the star sprite and the Milky Way.
- **The sky:** `Source/MuseeVision/Sky/MuseeSky.h`, 36-58 and 89-120, and `MuseeSky.cpp`:
  - 118-152: sphere mode (no sun, no atmosphere), retired as a mode;
  - 187-205: the Moon and the stars' rotation;
  - 246-344: the star field and the Milky Way sphere.
  - `Sky/Ephemeris.h`, `Sky/Ephemeris.cpp`: Meeus, now Time's almanac.
- **The Cube side:** `Source/MuseeVision/Cube/CubePlan.h:55-73` (the iris's clamp and the mast), and
  `Cube/CubeWay.cpp`, 17-21 (the clamp level) and 37-54 (the head's profile).
- **Data:** `assets/sky/stars.bin` and `assets/sky/CREDITS.md` (Yale BSC V/50); `windows/SourceArt/Textures/T_milkyway.png`
  (NASA SVS, 57 MB).
- **Boards:** `plan/canvas/ElanSphere.dc.html`, `ElanCube.dc.html` (the comparison table), `FutureAtrium.dc.html`
  (the iris) and `AsBuiltElan.dc.html`; `plan/README.md`, "Élan, level 2".

---

## 12. First-pass cut

**Built tonight, in this order:**
1. **The declaration:**
   - port `sphere.shaft` with its served well;
   - the footprint's envelope and Σ;
   - enclosures `sphere.interior` and `sphere.neck`, and the openings of §1.4;
   - anchor `sphere.space`;
   - the channels of §6.8.
2. **The shell:** the plate (§3.1) and the image surface as **one continuous image-luminaire surface** R 13.700.
3. **The exterior:** the pearl skin as one lathe with its seam lines, the gilt drip ring, the boss and the ball.
4. **The neck:** the throat, the gilt trellis (courses and ribs), the flange with its pocket, the rails and brackets.
5. **The platform:** the deck, the stone ring and lids, the nosing, the shoe, the 32 posts and 32 panes, the
   handrail and the 8 struts.
6. **The car:** everything in §3.7, and both columns as instanced courses.
7. **Motion:**
   - the car's states and three drives;
   - the doors, the upper and platform irises (the port iris via Site);
   - interlocks S-I1 … S-I9, the occupancy guards, the channels.
8. **Light:**
   - the celestial image source (stars, Moon, Milky Way) on the image luminaire;
   - the handrail runs, the car's ring and the neck's ring under Operation;
   - the targets of §4.4 commissioned.
9. **Sound:** RT60 from Matter for both enclosures; footfalls.
10. **Checks:**
    - Form.Sealed on `sphere.interior` (upper iris shut and clamped) and on `sphere.neck` (both shut);
    - Form.NoOverlap in every car state;
    - Motion.Housed, Motion.Clears (with the declared contacts), Motion.Interlocked, Motion.Comfortable;
    - Site.RoomStaysInside (footprint plus the served well);
    - Site.Reachable (the platform with the car away);
    - Light.MeetsTarget on §4.4 and §4.5.

**Waits for polish:**
- the image modules' tiling (57 latitude rows) and their seams; the plate's stiffeners, the absorber and the skin's
  standoffs as drawn pieces;
- planets in the sky image, earthshine on the Moon, twinkling (none above the atmosphere: correct as is);
- the dome's night washes. Their mounts are on the Atrium's cornice (ElanExteriorStructure.cpp:1022-1034): the
  Atrium's sheet, with a facade class in `UseTargets.json`;
- mechanism voices (recorded evidence), the Spiralift band detail on the columns, the blades' brush seals if the leak
  probe finds the 2–4 mm slots (cube.md R-9);
- any work for `sphere.space`, including *The Starry Night* opening piece (proposed, not approved).

### Readings for the owner (each keeps the as-built look; each is forced by physics or the architecture)

1. The image surface is Ø 27.40 inside the Ø 28 structure (§3.2).
2. The Sphere mast stands on its foot, which the upper iris clamps. The ride up takes 31.6 s instead of 27 s (Q-S5).
3. The platform's floor light is the darkened-assembly level, 2.15 lx (Q-S4).
4. The balustrade is 32 panes between its 32 posts (§3.6).
5. The pearl skin's joints are welded and faired, D-014's hairlines rather than open 6 mm joints (§3.3).

---

## Open questions and risks (for the Architect)

**Q-S1 · Who serves the car's well.**
- The task assigns the car, the neck, the irises and the platform to this sheet, so the Sphere serves. The well then
  runs through the Atrium into the Cube, and the upper and platform irises, which close across it, are the serving
  side's own.
- `cube.md` assumes the Atrium serves (`atrium.car`). If that stands:
  - §3.7, §3.8 and §6.1 … §6.3 move to the Atrium as a block;
  - the Sphere becomes a receiving stage whose two irises cross a received well. Site would need "landing closures
    inside a receiving well".
- Decide before the Atrium's and the Sphere's builders start.

**Q-S2 · Rails through the Atrium.**
- The car needs guides and a drive from the Cube's shaft (rails end at −0.52) to the neck (rails start at 7.735).
  As built it floated through the Atrium's glass tube.
- Two gilt rails, north and south, inside the tube change the Atrium's look slightly. The owner decides, but without
  them the car is not a machine.

**Q-S3 · Gaps at the landings.**
- The receiving well is r 2.300, but the Atrium's floor lip (2.290) and landing sill (2.245) lie inside it.
- At the platform the step from the car (r 2.235) to the stone (r 2.460) is 0.225 m across a 12 m drop. A lift
  standard allows 35 mm between car and landing sills (EN 81-20).
- As built, and the capsule can't fall through, but a real foot could. The fix would be a gilt sill on the car at
  the door (to r 2.270) and a matching lip on the platform. It is a design change, so it is asked, not made.

**Q-S4 · Dark enough to see the Milky Way.**
- The design shows the Milky Way, but physics allows it only if the platform is nearly unlit (≲ 0.05 lx) and the
  modules are blacker than 1 %. At `orientation` (5 lx) or `escape-route` (10.76 lx) it cannot show; at 2.15 lx it
  still fails target §4.5.3 by about 1–4×.
- Options:
  - (a) a new `assembly.dark` class at 2.15 lx, accepting a faint Milky Way;
  - (b) a presence rule: 2.15 lx while the visitor moves, step-marker level when still;
  - (c) no Milky Way.
- The owner decides. The Cube's Q-6 is the same question.

**Q-S5 · The ride takes 31.6 s, not 27 s.**
- The mast must stand on something. Clamping its foot in the upper iris is the Cube's mechanism mirrored, and it
  keeps home at 8.40 and the mast rising out of the neck's mouth.
- The alternative keeps 27 s: the port iris clamps the foot at home, and home moves down to 7.98. The mast would then
  run through the whole neck, and the upper iris would stay open while the car is up (as built).

**Q-S6 · Structure.**
- The platform (≈ 76 m² of stone ring, a crowd load of about 5 kN/m²) hangs on eight 80 mm bronze bars 8.9 m long.
  Euler buckling is about 25 kN each (E 110 GPa); the load is about 60 kN each.
- The Sphere mast is a free 8.7 m column carrying the car, stiffened only by a 23 mm iris clamp at its foot.
- The 3 mm lids over the iris pockets are walked on.
- The as-built geometry is kept (D-017). A structural review decides the sections, perhaps 168 mm tubes or sixteen
  struts.

**Q-S7 · Evidence and calibration.**
- The Milky Way map includes the bright stars, so they must be masked out before it is calibrated in cd/m².
- `T_milkyway.png` (57 MB) and `stars.bin` are on disk. Studio's evidence import copies them, with their credits, to
  `Evidence/Light` and `C:/mv/third_party/SourceArt` (D-008).

**Q-S8 · The car's clearing in the Cube** (cube.md §6.2: "its glass clears in 2.5 s").
- Glass can't fade. The physical version is anti-reflective low-iron glass with the car's light down to a glimmer:
  in the dark Cube, and under the sky here, it nearly vanishes.
- The palette should name the AR make; nothing in the geometry changes.

**Q-S9 · Gilt on a steel band column.** The spiral-band columns are steel bands, but the design calls them gilt.
D-014 lists plaster, bronze and wood as gilt substrates. Matter should name the real coating (a gold PVD over steel?)
or accept gilt over steel.

**R-S5 · The Atrium's ceiling can't be daylit.** With the opaque pearl dome, the as-built "glowing ceiling … follows
the daylight" needs the Atrium's own luminaires. The Atrium's sheet owns that.

**R-S7 · Two new Light capabilities carry the whole interior:** the light-field image luminaire and the celestial
source. If either misses tonight, the Sphere is black (there is no fallback sky, by design). The Cube's *Sea Light*
needs the same luminaire kind.

**R-S10 · Residency and cost.**
- The pearl dome is on the skyline from the Grounds, the Hall of Light's glass vault and the courts. The interior is
  never seen from outside.
- Site should keep an exterior group (the skin, ring, boss and ball) resident with the Grounds, and the interior only
  while the car is above `atrium` or the visitor is inside.
- The image source renders a full sphere every frame, and in the headset twice.

**R-S11 · Stacked footprints.** The Sphere's volume sits on the Atrium's on the same axis. NoOverlap and
RoomStaysInside must use volumes (§1.1).
