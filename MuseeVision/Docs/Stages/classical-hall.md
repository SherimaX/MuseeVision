# The Classical Hall · stage specification

The stage `classical-hall` is the Classical Hall at level −1 below the Rotunda: a barrel-vaulted sculpture gallery after
the Vatican's Braccio Nuovo, and beyond a screen of two columns a round domed tribune after the Sala delle Muse and the
Sala Rotonda. It holds 41 plaster casts from the Royal Cast Collection (SMK, Copenhagen). There is no daylight: the
light comes from laylights in the vault's crowns and the dome's eye, and from coves concealed on the cornices. The
visitor reaches the hall only by the Rotunda's spiral stair. The stair is the Rotunda's; this stage receives its well.

This page is the design sheet for the Room Builder, written for the first pass on Sunday 27 September 2026 (D-016). It
reproduces the hall as built (D-001), taking its numbers from the frozen code (section 11), and puts them through the new
systems. No number here may be changed by hand to make the room look right. When a number is wrong, the fix goes to the
system that owns it.

**Module and names.** The module is `MuseeClassicalHall` in `Source/Stages/Rooms/ClassicalHall`. It has the class
`UClassicalHallRoom : USiteRoom`, the public header `Public/ClassicalHallRoom.h`, and the plan numbers in
`Private/ClassicalHallPlan.h`. The stage key is `classical-hall`.

**Conventions used on this page.**
- **Units and axes.** Metres, in the stage's own frame: x east, y south, z up, true north.
- **Angles.** A plan angle φ is measured from east (+x) towards south (+y), as in the old code. So φ 90° is south
  (towards the gallery) and φ 270° is north. A compass bearing is φ + 90°.
- **Profiles.** A profile is a list of points (A, B).
  - A runs out from the named face and B runs up. It is traced with the solid on the left.
  - "round to P, convex" is a quarter circle from the previous point to P that bulges outwards; "concave" bulges
    inwards.
  - "cyma to P, convex first" is two quarter rounds that meet at the chord's midpoint: a cyma reversa. "Concave first"
    gives a cyma recta.
- **Material keys.** A key is `substance.working.history`. The working is the leaf of the Matter working record
  (`stone.honed` becomes `honed`, `cast.plaster` becomes `cast`). The history is a regime, defined in section 3.8.

---

## 1. Identity and placement

**Origin.** The origin O is the centre of the tribune, on its finished floor. It is the axis of the centrepiece and lies
under the dome's eye.

**In the museum plan.** O stands at plan (x 0.00, y −39.30) from the Rotunda's centre, and its floor is at z −6.00
below the Rotunda's finished floor. This is level −1.
- Site derives this position; the room never states it. Mate solves the translation from the mating
  `rotunda.stair ↔ classical-hall.stair`, which gives (0.00, −39.30, −6.00).
- **Converting old numbers.** x = x_old, y = y_old + 39.30, z = h_old = z_old + 6.00. Here h_old is the height above the
  hall's floor.

**Finished levels.**
- The floor is at z 0.00 everywhere, and meets the joint's threshold flush. The as-built 1 cm porphyry step at the port
  is retired.
- The underside of the structural slab is at z −0.40.
- The top of the roof is at z +5.45. This is museum −0.55, where the stages above begin.
- The highest inner surfaces are the laylight diffusers, at z 5.38.

**Footprint** (kind Building). It is a prism from z −0.40 to z 5.45. Its plan is closed, running clockwise seen from
above (north up), starting at the south-east corner:

1. (4.20, 31.40), then west along the shell's south face to (2.10, 31.40).
2. South along the east side of the sleeve to (2.10, 31.9959), which lies on the port cylinder.
3. Along the port cylinder (radius 7.60 about W = (0.00, 39.30)) through (0.00, 31.70) to (−2.10, 31.9959).
4. North to (−2.10, 31.40), then west to (−4.20, 31.40).
5. North along the gallery box's west face to (−4.20, 6.6910).
6. Along the tribune drum (radius 7.90 about O), the long way round by the north, from φ 122.12° through 180° and 270°
   to 360° + 57.88°, arriving at (4.20, 6.6910).
7. South along the east face back to (4.20, 31.40).

**Site.NoOverlap must compare volumes.** This footprint lies in plan under the Rotunda's drum and north passage (y
31.40 to about 28.1) and under Chenghuai (the tribune lies under the west lobe of its pond). Site.NoOverlap must
therefore compare volumes, each a plan polygon with a z range, and not plan polygons (see Risks).

**Enclosures.** There is one enclosure, `classical-hall.interior`, of kind Enclosed.
- It is a single sealed shell: the passage, the gallery, the throat under the lintel, and the tribune with its
  ambulatory, ring, dome and eye.
- It opens only at its one Port opening (section 2).
- The five laylight wells are Housing openings. Each is a pocket in the vault that holds a luminaire; the shell closes
  above them.
- There are no Court or Outdoors enclosures, and no Glazed or Open openings.

**Plan in words** (local frame).
- **The passage.** It runs from the port face (the cylinder r 7.60 about W, crossing the axis at y 31.70) to the
  gallery's south wall at y 31.10.
- **The gallery.** Its inner faces are x = ±3.00. It is 24.00 m long, from the south wall at y 31.10 to the screen line
  at y 7.10, in four bays of 6.00 m.
  - The bay centres are at y 28.10, 22.10, 16.10 and 10.10.
  - The column pairs stand on the bay lines y 25.10, 19.10 and 13.10.
- **The throat.** North of the screen, the gallery's side walls continue as jambs under a lintel (soffit z 3.075) to
  where they meet the tribune wall at y 5.8788.
- **The tribune.** Its outer wall is the cylinder r 6.60 about O.
  - A ring of twelve columns stands on r 3.40.
  - The ambulatory lies between r 3.64 and r 6.60.
  - The eye has radius 1.20.

```
   N (−y)
        ___________
      /   tribune   \          O = (0,0): tribune centre, floor z 0
     |   ( ring 3.4 ) |        outer wall r 6.60, shell r 7.90
      \_____  _____ /
            ||  throat, lintel soffit 3.075, y 5.879 … 7.10
         screen y 7.10 (two columns x ±1.10)
          | bay 4 |  y 7.10 … 13.10
          | bay 3 |  y 13.10 … 19.10      gallery x ±3.00, shell x ±4.20
          | bay 2 |  y 19.10 … 25.10
          | bay 1 |  y 25.10 … 31.10
          |_passage_|  south wall face y 31.10 → port face y 31.70 (r 7.60 about W)
            ( well )   W = (0, 39.30): the Rotunda's stair shaft, served by the Rotunda
   S (+y)
```

---

## 2. Ports

There is one port. By D-012 the stage is a leaf: its only mating is with the Rotunda's served stairwell. The Rotunda's
ports are west, north, south, east and the stair; the Classical Hall hangs from the stair alone.

| Field | Value |
|---|---|
| Key | `classical-hall.stair` |
| Kind | Stairwell. This stage is the **receiving** side of the well. |
| Class | `stairwell.spiral.3600` (Data/Site/PortClasses.json; see below) |
| Frame origin | (0.00, 31.70, 0.00): on the port face, on the opening's centreline, at threshold level |
| Facing | +y, south, out of the stage towards the well |
| Face shape | Cylindrical, radius 7.60 about the well axis W = (0.00, 39.30), convex towards the hall |
| Wall depth | 0.60 on the centreline (the port face to the gallery's south wall face at y 31.10); 0.8162 at the clear jambs x ±1.80 (the face there is at y 31.9162) |
| Well | A vertical cylinder of radius 7.60 about W, from z −0.40 to z +6.00 (museum −6.40 to 0.00). **Receiving:** this stage reserves the prism as a void and builds nothing inside it. The Rotunda serves it: the shaft masonry r 6.48–7.60, the drum's pocket, the spiral stair of 40 risers × 0.150, the landing at z 0.00 and the tympanum over the portal. |
| Mates | `rotunda.stair` (the Rotunda serves the well). The passage length is 0 m: the two faces touch on the cylinder r 7.60. |

**What the class must hold** (Site's record, taken from the as-built evidence):
- **Clear opening.** 3.60 wide by 4.40 high. The head is a half circle of radius 1.80 springing at z 2.60, with its crown
  at z 4.40. The jambs are the planes x = ±1.80, and the intrados is the half-cylinder R 1.80 about the line
  (x 0, z 2.60) running along y. These are `MuseePlan::SunClock::PortWidth/PortHeight/PortSpring`.
- **Closure.** An open portal, with no leaves.
- **Collar envelope.** The as-built sleeve: 4.20 wide (x ±2.10), with a round head of R 2.10 springing at z 2.60 (crown
  z 4.70), from the slab underside at z −0.40.
- **Rough-opening allowance.** Site's lining thickness, written *a* below. The room never invents it.

**What this stage builds at the port** (the rough-opening rule).
- It builds only the structural opening, which is the clear opening grown by *a*: jambs at x ±(1.80 + *a*), and a head of
  R 1.80 + *a* about (x 0, z 2.60). The opening runs from the gallery's south wall face (y 31.10) through the sleeve to
  the port face.
- It builds nothing past the port cylinder.
- Site's joint builds the rest:
  - the travertine threshold through both wall depths;
  - the brushed-bronze strip on the mating plane, which here is the arc r 7.60 about W across the threshold;
  - the lining in front of both rough faces;
  - the surrounds;
  - the collar.
- On this side the surround sits on the gallery's flat south face (y 31.10). It replaces the as-built carved marble
  architrave, keystone, plinth blocks and porphyry sill, by the one-doorway rule of principle 2 (see Risks).

---

## 3. Pieces by trade

Every piece is one solid of one substance (Form). The shells are closed and thick. The structural masonry behind the
revetments (the wall cores, the vault shells, the slab, the roof and the sleeve) is `travertine.sawn.aged`. It is never
seen, and it fills from each inner finish to the shell faces in section 1.

### 3.1 Walls, openings and reveals

- **Gallery long walls.** The inner faces are the planes x = ±3.00.
  - Between the south wall (y 31.10) and the screen (y 7.10) they rise from z 0 to the cornice ledge at z 3.70, where
    the vault springs.
  - North of the screen, from y 7.10 to y 5.8788, they are the throat's jambs and rise only to the lintel soffit at
    z 3.075.
  - They are split at z 0.45: below is the dado, in `rosso-antico.polished.aged`, as slabs 1.20 long in one course.
    Above is the field, in `cream-marble.honed.aged` ashlar in courses of 1.20 and blocks of 1.90 ± 30 % in half bond,
    with 3 mm joints. This is the Rotunda's coursing; use the Rotunda's Data/Form coursing record and don't duplicate
    it.
  - The walls are 1.20 thick to the shell (x ±4.20).
- **The south wall.** Its inner face is the plane y = 31.10, from x −3.00 to +3.00 and from z 0 up to the vault's end
  arc: x = 3.8284 sin(s/3.8284), z = 1.3216 + 3.8284 cos(s/3.8284), for |s| ≤ 3.4473.
  - It is split into dado and field at z 0.45.
  - It is pierced by the port's rough opening (section 2).
  - The wall is 0.30 thick to the shell face at y 31.40. The sleeve (x ±2.10, round head R 2.10 springing at z 2.60)
    then continues the masonry to the port face.
- **The passage.** It runs from y 31.10 to the port face. Its jambs, soffit and floor are the rough faces. Site lines
  them.
- **The tribune wall.** It is the cylinder r 6.60 about O, from z 0 to z 3.70, split into dado and field at z 0.45.
  - It is 1.30 thick to the shell (r 7.90).
  - It opens to the throat between φ 62.964° and φ 117.036°, where the cylinder meets x = ±3.00 at y 5.8788. Over that
    opening the wall exists only from the lintel soffit (z 3.075) to z 3.70.
- **The lintel.**
  - The soffit is the plane z 3.075 over the throat, from x −3.00 to +3.00, running from the screen line (y 7.10) to the
    tribune wall.
  - Its face towards the gallery is the plane y = 7.10, from z 3.075 up to the vault's end arc. It carries the lunette
    panel (3.4).
  - Both are in `cream-marble.honed.aged`.
- **Gallery niches (8).** They lie on x = ±3.00 at the bay centres y 28.10, 22.10, 16.10 and 10.10. They are named E1…E4
  and W1…W4 from south to north.
  - Each is half-round in plan with R 0.75, its centre on the wall plane.
  - The sill is at z 0.45. The half-cylinder rises from 0.45 to the springing at 2.19. The head is a quarter sphere of
    R 0.75 centred at (the wall plane, z 2.19), with its crown at 2.94.
  - The interior is `pompeian-red-stucco.trowelled.aged`. The sill slab (the niche's floor at z 0.45, 0.05 thick) is
    `statuary-marble.honed.aged`.
- **Tribune niches (5).** They sit at φ 30° (SE), 150° (SW), 210° (NW), 270° (N) and 330° (NE).
  - The mouth is the plane tangent to r 6.60 at the niche's axis.
  - Each is half-round with R 0.90, centred on the mouth plane at r 6.60. The sill is at 0.45, the springing at 2.05 and
    the crown at 2.95.
  - A thin reveal joins the cylinder to the mouth plane. It is deepest, 0.0617, at the mouth's edges.
  - The materials are the same as in the gallery niches.
  - The Apollo Belvedere (2.24 m) was the reason for this height.

### 3.2 Floors (opus sectile, polished; each figure laid in pieces within Matter's stock, 1.5 mm joints)

All floor stones finish at z 0.00, 0.03 thick on the slab. The large porphyry discs (Ø 3.10 and Ø 3.00) are larger than
any porphyry block, so Lay() cuts them into sectors within stock.

**The gallery** (after the Pantheon's floor):
- **The white border** (`statuary-marble.polished.worn`):
  - the side strips x ±2.10…±3.00, the full length;
  - the south strip, y 30.70–31.10;
  - the north strip, y 7.10–7.50;
  - the throat, from y 5.8788 to 7.10, running on to the tribune pattern's rim.
- **The carpet** (x ±2.10, y 7.50–30.70):
  - **The end panels.** They are verde (`verde-antico.polished.worn`), at y 30.20–30.70 and 7.50–8.00.
  - **One square per bay.** Each is 4.20 × 4.20, centred on the bay centre (y 28.10, 22.10, 16.10 and 10.10). It has a
    verde frame 0.15 wide and inside it a giallo field (`giallo-antico.polished.worn`). In the field is a white ring
    (r 1.55–1.65 about the bay centre) around a porphyry disc of r 1.55 (`porphyry.polished.worn`).
  - **Between the bays**, at the column pairs, are zones 1.80 long: y 24.20–26.00, 18.20–20.00 and 12.20–14.00.
    - Each zone has a verde frame 0.15 wide around three white cells.
    - The cells run x −1.95…−0.65, −0.65…0.65 and 0.65…1.95, and are 1.50 deep.
    - Each cell holds a lozenge in `rosso-antico.polished.worn`, centred in it, with half-diagonals of 0.55 along both x
      and y.

**The tribune** (polar about O; the ring boundaries are at φ 15° + 30°k):

| r from | r to | Figure | Stone |
|---|---|---|---|
| 0 | 1.50 | disc under the pedestal | porphyry |
| 1.50 | 1.65 | ring | giallo |
| 1.65 | 2.90 | 12 sectors, each with a disc r 0.22 at r 2.275 on its mid-angle | white field, porphyry discs |
| 2.90 | 3.05 | band | giallo |
| 3.05 | 3.80 | band (the ring columns stand on it) | white |
| 3.80 | 3.95 | band | verde |
| 3.95 | 6.15 | 12 sectors, each a giallo field inside a white border 0.10, with a disc r 0.80 at r 5.05 on its mid-angle | the discs alternate: porphyry at φ 30°, 90°, 150°, 210°, 270° and 330°; verde at φ 0°, 60°, 120°, 180°, 240° and 300° |
| 6.15 | 6.30 | band | verde |
| 6.30 | 6.60 | border to the wall | white |

### 3.3 Vaults and dome

All are in `lime-plaster.run.aged`: run in place, on the masonry vault shells.

- **The gallery barrel.** It is segmental, with radius 3.8284 about the axis line (x 0, z 1.3216) running along y from
  7.10 to 31.10.
  - It springs from the cornice ledge (x ±3.00, z 3.70) at a half-angle of 51.592°. The crown is at z 5.15.
  - The coordinate s is the arc length from the crown, positive to the east. The springing is at s ±3.4473.
  - There is a plain springing band for |s| from 3.00 to 3.4473. The coffer field covers s ∈ [−3.00, +3.00]: 10 rows of
    0.60 along the arc (at s = ±3.00, x = ±2.7023 and z = 4.0335).
  - **Along y**, from south to north:
    - a plain band, 31.10–30.50;
    - bay 1 coffers, 30.50–25.70 (8 × 0.60);
    - a plain pair band, 25.70–24.50;
    - bay 2, 24.50–19.70;
    - a pair band, 19.70–18.50;
    - bay 3, 18.50–13.70;
    - a pair band, 13.70–12.50;
    - bay 4, 12.50–7.70;
    - a plain band, 7.70–7.10.
  - **Coffers.** Each bay has 10 × 8 cells, less the lantern's 4 × 4, which leaves 64 coffers. The gallery has 256.
  - **The lantern** in each bay:
    - **The opening.** It spans s ∈ [−1.20, +1.20] (x ±1.1805, z 4.9635) by the bay centre ± 1.20 along y.
    - **The frame.** A flat frame 0.06 wide runs on the vault surface inside the opening's edge, so the inner edge is at
      s ±1.14 (x ±1.1232, z 4.9815) and the bay centre ± 1.14.
    - **The well.** Vertical plaster walls run from that inner edge up to the diffuser plane at z 5.38. They are 0.40
      high at the long sides and 0.23 at mid-length.
    - **Housing pocket.** The well is a Housing opening that holds the laylight (section 4).
- **The coffer** (a 0.60 cell in the gallery):
  1. a flat rib 0.06 wide on the vault surface (0.12 between neighbours);
  2. a riser 0.06 into the vault, then a setback of 0.035;
  3. a riser of 0.06, then a setback of 0.035;
  4. a riser of 0.06 to the back. The total depth is 0.18, and the back panel is 0.34 × 0.34.
  - **The rosette.** On the back's centre is a gilt rosette (`plaster-of-paris.gilt.aged`) of radius 0.3 × the back's
    smaller side (0.102 here). It has eight petals.
  - **The rosette's section.** It is turned from (radius fraction, height × R/0.08):
    (0, −0.004) (1.00, −0.004) (1.00, 0.004) (0.86, 0.014) (0.62, 0.020) (0.36, 0.023) (0.20, 0.032) (0, 0.036).
  - **The petals.** On the rows from 1.00 to 0.36, the radius is multiplied by 0.84 + 0.16 |cos 4α|.
- **The annular vault** (over the ambulatory) is turned about O.
  - **Its section** is a circle of radius 1.5009 centred at (r 5.10, z 3.6491).
  - **It springs** from the tribune entablature's ledge (r 6.60, z 3.70) and from the ring beam's outer ledge (r 3.60,
    z 3.70). Its crown is at r 5.10, z 5.15.
  - **Plain bands** 0.30 of arc run at both springings. The coffer field spans section angles 13.395° to 166.605°, from
    r 6.560 to r 3.640.
  - **The coffers** are 60 cells round (6°, starting at φ 15°, so that cells centre on the niche axes) by 7 rows across,
    each 21.887°, or 0.573 of arc.
  - **The coffer detail** is as in the gallery, with a rib of 0.05 and the steps scaled by 0.85 (setbacks 0.02975), and
    a rosette on each.
- **The dome** (over the ring) is a spherical cap of radius 3.9464, centred on the axis at z 1.3905.
  - **It springs** from the ring beam's inner ledge (r 3.20, z 3.70) and rises to the eye (r 1.20, z 5.15).
  - **Plain bands** run at both edges: 0.25 of arc at the springing and 0.20 at the eye.
  - **Coffers.** There are 24 round (15° each, starting at φ 7.5°) in 3 rings. The ring edges (r, z) are (3.047, 3.898),
    (2.463, 4.474), (1.893, 4.853) and (1.389, 5.084).
  - **The coffer detail.** The rib is 0.05. The step scale is 1.0 on the lowest ring, 0.9 on the middle ring and 0.8 on
    the top ring. Each has a rosette.
- **The eye.**
  - A cylindrical well of r 1.20 runs from z 5.15 to the diffuser at z 5.38. It is a Housing opening.
  - A gilt bead (`plaster-of-paris.gilt.aged`) is turned round its lower edge. Its section, as (r, z): (1.19, 5.17),
    round to (1.19, 5.09) convex, then (1.22, 5.09) and (1.22, 5.17).

### 3.4 The order and the mouldings

**The Ionic order** (after Vignola; D 0.34; a column is 9 D = 3.06). The Ionic order is shared with the exterior dress,
so it goes into `Data/Form/Orders.json` as `order.ionic`, parametrised by D. The hall's own cornice with its lip stays
in `ClassicalHallPlan.h` while it is used only here.

**The column (26)** is turned. The base and capital are `statuary-marble.honed.aged`. The shaft is a separate piece of
coloured marble. The section is (r, z):

```
square plinth 0.47 × 0.47, z −0.01 … 0.055 (box)
base   (0, 0.050) (0.200, 0.050) (0.200, 0.055) round to (0.200, 0.115) convex  [lower torus, bulging to 0.23]
       (0.190, 0.115) (0.190, 0.120) round to (0.190, 0.155) concave                [scotia]
       (0.185, 0.155) (0.185, 0.160) round to (0.185, 0.195) convex                 [upper torus]
       (0.176, 0.195) (0.176, 0.200) round to (0.170, 0.215) concave                [apophyge]
shaft  r 0.170 from z 0.215 to 1.0983 (the lower third, straight); then entasis
       r(t) = 0.170 − 0.02833 (1 − cos(π t / 2)), t 0 … 1, up to z 2.865 (r 0.14167 = 5/6 of D/2)
cap.   round to (0.14967, 2.875) concave [apophyge]; (0.14967, 2.878) round to (0.14967, 2.900) convex [astragal];
       (0.14567, 2.900) round to (0.195, 2.965) convex [echinus]; (0, 2.965)
```

- **The capital** (Carving) has these parts:
  - **Canalis block:** 0.31 across the volutes by 0.34 front to back, from z 2.945 to 3.04.
  - **Plate:** 0.47 by 0.32, from z 3.005 to 3.05.
  - **Bolsters.** Two bolsters are turned about the front-to-back axis, at ±0.155 from the column's axis and z 2.945.
    Each has radius 0.075, runs ±0.19 long, and is waisted to r 0.056, with a belt of r 0.064 for |u| ≤ 0.03.
  - **Volutes.** On both faces of each bolster is a volute: a logarithmic spiral fillet with a 12 × 16 mm section, 2.5
    turns from R 0.069 to R 0.0193, round an eye disc of R 0.018. It winds outwards and down from the top, clockwise on
    the right as seen from its face.
  - **Abacus:** 0.37 × 0.37, from z 3.035 to 3.07. Its edge profile is (0, 0), (0, 0.012), round to (0.015, 0.027)
    convex, then (0.015, 0.035).
- **Placement:**
  - **Gallery pairs (12, shafts `giallo-antico.polished.aged`):** x ±2.60, at y 25.60/24.60, 19.60/18.60 and
    13.60/12.60. The volutes face the gallery's axis.
  - **The screen (2, giallo):** x ±1.10, y 7.0283. They face south.
  - **The tribune ring (12, shafts `pavonazzetto.polished.aged`):** r 3.40 at φ 15° + 30°k, with the fronts radial.

**Pilasters (10).**
- **Form.**
  - The shaft block's face is 0.07 proud of the wall. It is 0.34 wide and runs from z 0.20 to 2.90, in the shaft's
    marble.
  - The Attic base is swept round the three faces. Its profile is (A out from the shaft's face, B up):

    ```
    (−0.010, −0.010) (0.065, −0.010) (0.065, 0.055) (0.045, 0.055) round to (0.075, 0.085) convex, round to (0.045, 0.115)
    convex; (0.035, 0.115) (0.035, 0.120) round to (0.020, 0.1375) concave, round to (0.035, 0.155) concave; (0.030, 0.155)
    (0.030, 0.160) round to (0.0475, 0.1775) convex, round to (0.030, 0.195) convex; (0.005, 0.195) (0.005, 0.200) round to
    (0, 0.215) concave; (−0.010, 0.215)
    ```

  - The neck is swept round the three faces: (−0.01, 2.865) (0.004, 2.865), round to (0.004, 2.89) convex, round to
    (0.03, 2.945) convex, then (−0.01, 2.945).
  - The capital has a canalis block and two flat volute discs of R 0.075 at ±0.155, z 2.945, with the spiral fillets. The
    abacus is 0.40 wide, with its face 0.01 proud of the canalis.
- **Gallery pilasters (giallo):**
  - at the south corners: x ±3.00, y 30.85 (8 cm clear of the south wall);
  - the antae at the screen: x ±3.00, y 7.27.
- **Tribune pilasters (pavonazzetto):** on r 6.60 at φ 0°, 61.38°, 118.62°, 180°, 240° and 300°.
  - The two at 61.38° and 118.62° flank the opening: each stands 1.58° off the jamb.

**The entablature** has a lip that hides the cove light. It stands 0.72 above the architrave's soffit (z 3.06), with the
ledge at z 3.70 and the lip's top at z 3.78. The section is (A out from the lowest fascia, B up from z 3.06), in
`statuary-marble.honed.aged`:

```
(BackA, 0) (0, 0) (0, 0.055) (0.012, 0.055) (0.012, 0.115) (0.024, 0.115) (0.024, 0.185)     three fasciae
cyma to (0.050, 0.215) convex first; (0.050, 0.228) (0.012, 0.228)                              architrave crown
(0.012, 0.418)                                                                                   frieze
cyma to (0.040, 0.445) convex first; (0.045, 0.445) (0.045, 0.500) (0.085, 0.500) (0.085, 0.515) bed moulding, dentil band
round to (0.125, 0.555) convex; (0.260, 0.555) (0.260, 0.620) (0.272, 0.620) (0.272, 0.630)      ovolo, corona
cyma to (0.330, 0.705) concave first; (0.330, 0.720) (0.250, 0.720) (0.250, 0.640) (BackA, 0.640) sima, lip, ledge
BackA = −0.08 against a wall (the architrave is 0.07 proud, plus 0.01 into the wall)
```

- **The dentils** are blocks 0.034 wide × 0.040 deep × 0.048 high (z 3.516–3.564) at a pitch of 0.052. They stand on the
  dentil band, from 0.035 to 0.075 out from the architrave's face, with a 0.06 margin at each end of a run. On a ring the
  count is floor(2πR / 0.052).
- **The gallery run.**
  - The entablature runs along both long walls with the architrave face 0.07 proud.
  - Over each column pair it breaks forward in a ressaut. The architrave face there is 0.2583 from the wall, for the
    pair's y ± 0.72.
  - It crosses the screen with its face at y 7.17 and returns into the south wall.
  - Solid fillers behind each ressaut run from the wall to the ressaut, for z 3.07–3.685 and the pair's y ± 0.65.
- **The tribune run.**
  - The entablature is turned round the outer wall with the architrave face at r 6.53, continuous over the opening,
    where it rides on the lintel.
  - It is also turned round **the ring beam** on both faces: the inner architrave face at r 3.2583 and the outer at
    r 3.5417, over a common soffit at z 3.06.
  - The ring beam's inner ledge runs r 3.008–3.20 at z 3.70; the dome springs at r 3.20.
  - Its outer ledge runs r 3.60–3.792 at z 3.70; the annular vault springs at r 3.60.
  - Dentils run on both faces.

**Wall mouldings** (`statuary-marble.honed.aged`), swept round the whole perimeter from one side of the port's surround to
the other: the gallery's south wall, the long walls, the throat, and round the tribune.
- **The skirting** is 0.14 high and 0.025 proud: (−0.010, −0.025) (0.025, −0.025) (0.025, 0.100), cyma to
  (0.006, 0.135) convex first, then (0.006, 0.140) and (−0.010, 0.140).
- **The dado rail's** top is at 0.46, 1 cm over the niche sills; the rail runs across the niche mouths:
  (−0.010, 0.350) (0.008, 0.350), cyma to (0.040, 0.405) concave first, then (0.045, 0.405) (0.045, 0.435), round to
  (0.030, 0.460) convex, then (−0.010, 0.460).
- **Niche surrounds.** A band 0.10 wide runs round each niche's jambs and head from z 0.46, in the mouth plane. On the
  curved wall it runs on the plane through the jambs' wall points and is sunk to meet the wall. Its profile, as (A out
  from the opening's edge in the wall plane, B out of the wall): (−0.015, −0.01) (0.100, −0.01) (0.100, 0.014), cyma to
  (0.075, 0.035) convex first, (0.012, 0.035), round to (−0.015, 0.019) convex.
- **Keystones.** One sits at each niche's crown. It runs from 0.04 below the crown up into the architrave's soffit at
  z 3.07, widening from 0.10 to 0.15, and stands 0.055 proud.

**Framed panels and consoles.**
- **Panels.** A panel is a `pavonazzetto.polished.aged` slab 0.80 × 1.90, from z 0.85 to 2.75, standing 0.02 proud. It
  sits in a frame of `statuary-marble.honed.aged` with the profile (−0.012, −0.01) (0.045, −0.01) (0.045, 0.012), cyma to
  (0.012, 0.035) concave first, then (−0.012, 0.035).
- **Where they go:**
  - 16 in the gallery, at bay centre ± 1.60 on both walls;
  - 10 in the tribune, at each niche's φ ± 15°. On the curved wall each panel is flat, tangent at its centre, and set
    0.02 into the wall at its edges.
- **The console** is a quadrant bracket on the panel's centre line. It is 0.36 wide, 0.30 proud and 0.41 high, with its
  top at z 1.55. Its section is (−0.03, 1.14) (0.06, 1.14), an elliptic quarter centred (0.06, 1.50) with radii
  0.21 × 0.36 from −90° to 0°, then (0.30, 1.50) (0.30, 1.55) (−0.03, 1.55).
- **The lunette panel.** A pavonazzetto panel 2.60 × 0.80 (z 3.98–4.78), framed, sits on the lintel's face at y 7.10,
  centred on x 0 and facing south.

### 3.5 Carriers for the works (built by the room)

- **Niche plinths.** In `statuary-marble.honed.aged`. The section is (0.015, −0.005) (0.015, 0.025), cyma to (0, 0.045)
  convex first, then (0, H − 0.03), cyma to (0.02, H − 0.005) concave first, then (0.02, H).
  - As built, the gallery plinths were 0.56 × 0.40 × H 0.12 (top z 0.57), and the tribune plinths 0.70 × 0.52 × H 0.08
    (top z 0.53). Each was centred on the niche's axis 0.42 R back from the mouth.
  - Their size now comes from the hang rule (section 8).
- **The centrepiece pedestal** is turned about O, Ø 2.20 and 0.30 high: (0, −0.01) (1.10, −0.01) (1.10, 0.03), round to
  (1.08, 0.07) convex, then (1.05, 0.07) (1.05, 0.24), cyma to (1.10, 0.285) convex first, then (1.10, 0.30) and
  (0, 0.30).
- **Consoles** (3.4) carry the busts.

### 3.6 Stairs

None. The spiral stair is the Rotunda's, inside the served well.

### 3.7 Glazing

None. The laylight diffusers are luminaires (section 4).

### 3.8 Material keys and histories

| Key | Where |
|---|---|
| `cream-marble.honed.aged` | walls above the dado, the lintel's face and soffit, the throat's jambs |
| `rosso-antico.polished.aged` | the dado, z 0–0.45 |
| `statuary-marble.honed.aged` | bases, capitals, entablatures, the ring beam, pilaster bases and capitals, skirting, dado rail, surrounds, keystones, frames, consoles, niche sills, plinths, the pedestal, ressaut fillers |
| `giallo-antico.polished.aged` | the gallery and screen column shafts, the gallery pilaster shafts |
| `pavonazzetto.polished.aged` | the tribune's ring shafts and pilaster shafts, the wall panels, the lunette panel |
| `pompeian-red-stucco.trowelled.aged` | niche interiors |
| `lime-plaster.run.aged` | vaults, the dome, coffers, the lantern and eye wells |
| `plaster-of-paris.gilt.aged` | rosettes, the eye's bead (gold leaf on red bole over cast plaster) |
| `statuary-marble.polished.worn` | white floor fields, borders, the throat |
| `giallo-antico.polished.worn` | floor fields and rings |
| `verde-antico.polished.worn` | floor frames, bands and discs |
| `porphyry.polished.worn` | floor discs |
| `rosso-antico.polished.worn` | floor lozenges |
| `travertine.sawn.aged` | structural masonry, the slab, the roof, the sleeve (never seen) |
| `plaster-of-paris.cast.aged` | the 41 casts (Art's objects) |
| `opal-glass.laminated.clean` | laylight diffusers (the substance under Light's diffuser) |

**History regimes.** They map to Matter's history records as follows:
- **worn:** `stone.foot-traffic-wear` plus `stone.pore-soiling`.
- **aged**, on stone: `stone.pore-soiling`, indoors and untrodden.
- **aged**, on white: `white.soiling` plus `white.micro-cracking`.
- **aged**, on gilt: `gilt.wear`.
- **clean:** glass kept clean.

Everything here is interior, so the sky exposure is 0 and there is never wetness.

**New Matter records this stage needs.**
- **Accents of the stone family** (`family: accent`, `accentOf: stone`). The colours are the boards':
  - `statuary-marble`: Carrara statuario, #ECE8E0;
  - `giallo-antico`: Numidian yellow from Chemtou, #D9B874;
  - `pavonazzetto`: Docimian white with violet veins, #EBE3E8;
  - `rosso-antico`: from Cape Tainaron, #6E2A22 with veins #2E0F0C;
  - `verde-antico`: Thessalian serpentinite breccia, #2F3A30;
  - `porphyry`: Imperial porphyry from Mons Porphyrites.
- **An accent of the white family:** `pompeian-red-stucco`, lime stucco pigmented with red ochre, #8E4B3D, matt.
- **Glass:** `opal-glass`.
- **Existing records used:** `cream-marble`, `lime-plaster`, `plaster-of-paris`, `travertine`, and the workings
  `stone.honed`, `stone.polished`, `run`, `trowelled`, `cast.plaster` and `gilt`.

---

## 4. Light

**Daylight.** None. The stage is underground and has no Glazed or Open opening. The only daylight near it falls down the
Rotunda's raised drum onto the landing, on the serving side of the well.

**Colour.** 4000 K throughout. This is one CCT for the room, the owner's choice for works. It was 3800 K as built.

**Luminaires.** The TYPE is the catalogue key. Light chooses the beam and the count unless the TYPE is named. The room
never states lumens.

| Mount key | Mount | TYPE | Count and position |
|---|---|---|---|
| `classical-hall.laylight.bay1`…`bay4` | horizontal ceiling plane in each lantern well at z 5.38; the body within z 5.38–5.43 | `luminous-panel.4000k`: an edge-lit LED light-guide panel behind an opal diffuser, ≤ 50 mm deep. **Light must add this record.** | 4 panels, each 2.2464 (x) × 2.280 (y), centred (0, 28.10 / 22.10 / 16.10 / 10.10). Light modules them. |
| `classical-hall.laylight.eye` | the same, in the eye well | `luminous-panel.4000k` | 1 disc of Ø 2.40, centred on O at z 5.38 |
| `classical-hall.cove.gallery.e1`…`e4`, `.w1`…`w4` | on the cornice ledge (z 3.70) behind the lip, at x ±2.87 | `linear-cove.4000k`: the 4000 K sibling of the Underscore strip. **Light must add this record**; the catalogue holds only `linear-cove.3000k`. | 8 runs, 4 per wall, between the ressauts: y 31.05–25.87, 24.33–19.87, 18.33–13.87 and 12.33–7.15 (5.18, 4.46, 4.46 and 5.18 m) |
| `classical-hall.cove.tribune.outer` | on the tribune entablature's ledge at r 6.47, z 3.70 | `linear-cove.4000k` | one continuous ring (about 40.7 m) |
| `classical-hall.cove.ring.outer`, `.ring.inner` | on the ring beam's ledges, at r 3.70 and r 3.10, z 3.70 | `linear-cove.4000k` | two rings (about 23.2 m and 19.5 m) |
| `classical-hall.washer.ring` | a short track on the ring beam's top behind its outer lip, at r 3.78, z 3.86, at φ 30°k between the columns, aimed out and 33° down at the outer wall | Light chooses (`track-wallwasher.4000k` is evidence-incomplete, so it can't be lit) | 12 positions |
| `classical-hall.accent.gallery-vault.e`, `.w` | a 0.30 track section recessed in the back of each coffer in the rows flanking the lanterns: s ∈ ±[1.20, 1.80], that is x ±1.18…±1.73 and z 4.96…4.73, all along the gallery | `track-spot.4000k.12deg`, `.20deg` or `.38deg`, chosen by the Scheme | one per work, lit from across the gallery at 30–55° from the vertical. As built: 1 per place, 2 per reclining figure. |
| `classical-hall.accent.annular` | coffer-recessed track in the annular vault's inner rows, r 3.95–4.55 (z about 4.66–5.02) | `track-spot.4000k.*` | one per tribune work, swung 7–11° aside so the light rakes. As built: 2 per ambulatory work. |
| `classical-hall.accent.dome` | coffer-recessed track in the dome's lowest ring, r 2.46–3.05 | `track-spot.4000k.*` | the centrepiece. As built: 3, 120° apart at r 2.30. |

**Targets.** Light holds each one at every reference moment.
- **Walking floors** (gallery, throat, ambulatory and the tribune's centre): `circulation`.
  - 100 lx maintained, with Uo ≥ 0.4.
  - Walls ≥ 75 lx and ceilings ≥ 50 lx.
  - UGR ≤ 28.
- **The passage at the port:** `circulation`, in Site's joint.
- **The works:**
  - **Class.** They are plaster casts, unpainted gypsum with a thin shellac sheen, so they take the care class
    `stone-and-bronze`.
  - **Target.** The recommended 300 lx on the work's front, measured vertically at the centre of its face. There is no
    maximum.
  - **Nothing here is light-sensitive.** There is no paper, silk or oil, so there is no 50 lx or 150–200 lx limit in
    this stage.
  - **UV:** within the class's limit.
  - **As-built evidence:** accents of 450 lx on statues, 350 on busts and 300 on the centrepiece, over the general
    light.
- **Night:** the same targets. The museum never closes (D-006).

**Design targets**, measured as a luminance order. Light's commissioning measures them; they are never tuned.
- In the gallery: diffuser > the lit face of each cast > the coffers round each lantern > the cream-marble wall above the
  dado ≥ the floor fields > the Pompeian niche back behind each cast.
- In the tribune: the eye's diffuser ≥ the gallery's diffusers; the centrepiece > the casts in the niches > the coffers
  of the dome > the annular vault > the outer wall.
- A ratio: the luminance of a cast's lit face over that of its niche's back at the same height must be at least 4:1.
  This is the white-on-red reading of the Braccio Nuovo.
- Evidence: as built, the diffusers stood at 1300 cd/m².

**Operation** (D-006).
- **Laylights, coves and washers:** control class `circulation`. They hold steady day and night, because underground
  there is no daylight to link to. The as-built option to follow the sky is left off, as built.
- **Accents:** control class `gallery`, linked to presence. They are on while the visitor is in this stage or in the
  lower half of the well.

---

## 5. Time and weather

- **Weather places.** None. The stage has no glazed opening, so no weather place applies: museum, Giverny and London all
  do not.
- **The sun.** No sun, moon or sky reaches the stage. There is no dial.
- **The Moment.** Time's Moment reaches this stage only through Light's Operation (the control classes) and Matter's
  history. Because every surface has a sky exposure of 0, it never gets wet or snowy.
- **Commissioning.** Nothing here depends on the sky, so commissioning needs one reference moment. Studio may run the
  others to prove the stage's light is independent of the hour.

---

## 6. Motion

- **This stage has no mechanism.** The port is an open portal with no leaves. The as-built bronze angle on the sill is
  retired; the joint's strip replaces it.
- **The neighbouring mechanism is the Rotunda's.** It is the sun-clock drum:
  - it lifts 3.5 m in 8 s;
  - its wall slides in the pocket from r 6.54 to 7.00, down to museum −3.58;
  - its balustrade panels retract;
  - the tympanum at r 7.05 over a lintel at museum −3.68 closes the upper part of the portal.

  All of this lies inside the served well, at radius 7.60 or less. Nothing of this stage enters the well, so this stage
  sets no clearance requirement there.
- **An interlock across the seam** (Site binds it, Motion enforces it). The drum's `lower` is refused while the visitor's
  body is in `classical-hall`, in the joint's passage, or in the well below the dial.
  - This keeps the rule "it never lowers while you are inside it or below".
  - It makes Site.Reachable hold: from this dead-end stage the one way out is up the stair, and it must be open in every
    configuration the visitor can reach.

---

## 7. Nature

There are no plants, bodies of water or ground cover in this stage. The floors are laid stone.

**A constraint from the stage above.** Chenghuai's pond (its west lobe) lies over the tribune, with its bed at museum
−0.46 above the roof top at −0.55. Its basin is Chenghuai's, and Nature.WaterContained is Chenghuai's check. This stage's
roof is not a basin.

---

## 8. Art

### Sources

- **Scans.** The casts are from SMK's Royal Cast Collection, public-domain 3D scans (`assets/classical/*.stl`, converted
  to `.glb`).
- **Sizes.** Each is scaled to the documented height, or length for a reclining figure, in `assets/classical/SOURCES.json`.
- **Labels.** They come from `data/classical.json`: title, artist or "after", date, the original's collection and city,
  notes, and the SMK cast number (KAS…). Studio imports them into `Data/Art/Collection.json` under the keys
  `classical-<id>`.

### How the casts are made and shown

- **Objects.** Each cast is a Form Scan drawing with provenance, at true size (Art.TrueSize), in
  `plaster-of-paris.cast.aged`. There are no frames or cases: the casts stand free.
- **Anchors.** Every place is a Plinth anchor. Its transform's location is the top centre of the carrier; its x axis is
  the way the work faces, and its z axis is up.

### The hang rule

- **The carrier fits the work.**
  - A carrier's top is the base footprint of the work's scan (the convex hull of the scan below 0.04 m) grown by 0.03 on
    every side.
  - The as-built sizes are the minimum.
- **A niche plinth stays inside its niche.**
  - Its front must stay 0.01 behind the mouth plane, behind the dado rail.
  - Its back corners must clear the niche's half-round by 0.02.
  - Where that is impossible, it becomes a pedestal standing on the floor in front of the niche. The top stays at the
    same height, and the skirting and dado rail are returned round it.
- **A work clears the architecture.**
  - Its envelope must clear the niche's head and walls, the columns and the vaults by at least 0.02.
  - A niche plinth is lowered, to no less than 0.03 high, until the head clears.
  - Nothing is moved by hand. The check (Form.NoOverlap between a Scan and the pieces) reports what doesn't fit, for the
    owner's walk.
- **Facing.** A work faces out of its niche, or as listed below.

### The places, as built (`windows/Scripts/classical_art.py` PLACEMENT, STATUES and BUSTS)

The scan sizes are w × d, and the base is the bottom slice at z < 0.04. Positions are local, and "top z" is the
carrier's top as built.

**Gallery niches.** The carrier is centred at x ±3.315 at the bay's y, with its top at z 0.57. It faces −x on the east
wall and +x on the west.

| Anchor | y | Work | Height | Scan w × d | Base |
|---|---|---|---|---|---|
| `classical-hall.gallery.niche.e1` | 28.10 | classical-augustus-prima-porta | 2.08 | 1.18 × 0.99 | 0.78 × 0.64 |
| `…niche.w1` | 28.10 | classical-doryphoros | 2.12 | 0.75 × 0.77 | 0.69 × 0.74 |
| `…niche.e2` | 22.10 | classical-capitoline-venus | 1.93 | 0.66 × 0.77 | 0.55 × 0.48 |
| `…niche.w2` | 22.10 | classical-lansdowne-heracles | 2.08 | 0.88 × 0.73 | 0.88 × 0.60 |
| `…niche.e3` | 16.10 | classical-minerva-giustiniani | 2.19 | 0.91 × 0.62 | 0.90 × 0.54 |
| `…niche.w3` | 16.10 | classical-capitoline-antinous | 1.89 | 0.66 × 0.61 | 0.55 × 0.46 |
| `…niche.e4` | 10.10 | classical-sophocles | 2.16 | 0.84 × 0.65 | 0.82 × 0.57 |
| `…niche.w4` | 10.10 | classical-farnese-diadoumenos | 1.85 | 0.83 × 0.44 | 0.62 × 0.44 |

**Tribune niches.** The carrier's centre is at r 6.978, with its top at z 0.53. Each faces the centre.

| Anchor | φ (bearing) | Position | Work | Height | Scan w × d | Base |
|---|---|---|---|---|---|---|
| `classical-hall.tribune.niche.n` | 270° (0°) | (0.000, −6.978) | classical-apollo-belvedere | 2.24 | 1.42 × 0.98 | 0.81 × 0.75 |
| `…niche.ne` | 330° (60°) | (6.043, −3.489) | classical-laocoon | 2.08 | 1.55 × 0.92 | 1.36 × 0.77 |
| `…niche.nw` | 210° (300°) | (−6.043, −3.489) | classical-venus-de-milo | 2.04 | 0.67 × 0.61 | 0.58 × 0.52 |
| `…niche.se` | 30° (120°) | (6.043, 3.489) | classical-barberini-faun | 2.15 | 1.57 × 1.34 | 1.18 × 1.00 |
| `…niche.sw` | 150° (240°) | (−6.043, 3.489) | classical-belvedere-antinous | 2.335 | 0.93 × 0.77 | 0.82 × 0.74 |

**Free-standing places.** The plinths and herms are Art's fittings in `statuary-marble.honed.aged`: moulded blocks with
the section (0.03, −0.01) (0.03, 0.06), cyma to (0, 0.10) convex first, then (0, H − 0.075), cyma to (0.03, H − 0.02)
concave first, then (0.03, H).

| Anchor | Carrier (as built) | Position (top z) | Facing | Work | Size |
|---|---|---|---|---|---|
| `classical-hall.gallery.plinth.2` | plinth 2.30 × 1.04 × 0.55, long side north–south | (0, 22.10) (0.55) | +x | classical-sleeping-ariadne | length 1.95; scan 1.95 × 0.78 × 0.90 high; base 1.82 × 0.70 |
| `classical-hall.gallery.plinth.3` | the same | (0, 16.10) (0.55) | −x | classical-dying-gaul | 0.93 high; 1.92 × 0.95; base 1.92 × 0.95 |
| `classical-hall.tribune.plinth.e` | plinth 0.90 × 0.90 × 0.50 | (5.10, 0) (0.50) | −x | classical-discobolus | 1.55; 0.96 × 0.47; base 0.70 × 0.41 |
| `classical-hall.tribune.plinth.w` | plinth 0.90 × 0.90 × 0.50, grows to 1.15 along the ambulatory | (−5.10, 0) (0.50) | +x, its arms along the ambulatory | classical-artemision-bronze | 2.09; 2.23 × 0.52; base 1.09 × 0.28 |
| `classical-hall.tribune.centre` | the pedestal Ø 2.20 × 0.30 (3.5) | (0, 0) (0.30) | +y, towards the gallery and the stair | classical-belvedere-torso | 1.59; 1.05 × 1.17; base 0.87 × 0.51 |
| `classical-hall.gallery.herm.w` | herm (below) | (−2.45, 30.83) (1.25) | −y | classical-homer | bust 0.57 |
| `classical-hall.gallery.herm.e` | herm | (2.45, 30.83) (1.25) | −y | classical-socrates | bust 0.51 |

**The herm** has three parts:
- a moulded plinth 0.40 × 0.40 × 0.14;
- a shaft widening upwards from 0.26 square at z 0.13 to 0.32 square at z 1.17;
- a moulded cap 0.40 × 0.40 × 0.09 from z 1.16. The top is at z 1.25.

**Gallery busts** stand on consoles at x ±2.85, with the top at z 1.55. They face −x on the east wall and +x on the west.
The "S" places are at the bay centre + 1.60 and the "N" places at the bay centre − 1.60.

| Anchor | y | Work (height) | | Anchor | y | Work (height) |
|---|---|---|---|---|---|---|
| `…bust.w1s` | 29.70 | classical-euripides (0.56) | | `…bust.e1s` | 29.70 | classical-julia-titi (0.485) |
| `…bust.w1n` | 26.50 | classical-plato (0.49) | | `…bust.e1n` | 26.50 | classical-julius-caesar (0.52) |
| `…bust.w2s` | 23.70 | classical-kimon (0.66) | | `…bust.e2s` | 23.70 | classical-antoninus-pius (0.895) |
| `…bust.w2n` | 20.50 | classical-homer-epimenides (0.49) | | `…bust.e2n` | 20.50 | classical-young-marcus-aurelius (0.75) |
| `…bust.w3s` | 17.70 | classical-chrysippus (0.44) | | `…bust.e3s` | 17.70 | classical-severan-lady (0.81) |
| `…bust.w3n` | 14.50 | classical-pseudo-seneca (0.47) | | `…bust.e3n` | 14.50 | classical-septimius-severus (0.85) |
| `…bust.w4s` | 11.70 | classical-sciarra-amazon (0.61) | | `…bust.e4s` | 11.70 | classical-oxford-bust (0.80) |
| `…bust.w4n` | 8.50 | classical-zeno-of-citium (0.47) | | `…bust.e4n` | 8.50 | classical-philip-the-arab (0.46; base 0.52 wide) |

**Tribune busts** stand on consoles at r 6.43, with the top at z 1.55, and face the centre. Place "a" is the niche's
φ − 15° and place "b" is φ + 15°.

| Anchor | φ | Position | Work |
|---|---|---|---|
| `classical-hall.tribune.bust.ne-a` | 315° | (4.547, −4.547) | classical-athena-parthenos (0.805; base 0.33 × 0.34) |
| `…bust.ne-b` | 345° | (6.211, −1.664) | classical-apollo-giustiniani (0.58) |
| `…bust.nw-a` | 195° | (−6.211, −1.664) | classical-alexander-helios (0.71) |
| `…bust.nw-b` | 225° | (−4.547, −4.547) | classical-ptolemy-ii (0.70) |
| `…bust.n-a` | 255° | (−1.664, −6.211) | classical-head-of-a-gaul (0.39) |
| `…bust.n-b`, `…bust.se-a`, `…bust.se-b`, `…bust.sw-a`, `…bust.sw-b` | 285°, 15°, 45°, 135°, 165° | (1.664, −6.211), (6.211, 1.664), (4.547, 4.547), (−4.547, 4.547), (−6.211, 1.664) | empty, as built |

**Totals.**
- 41 casts: 18 statues and 23 busts.
- 46 places: 5 consoles stand empty.
- The Rotunda's four niche casts (Dying Slave, Rebellious Slave, Bacchus, Venere Italica) are the Rotunda's.

**What the hang check will report tonight** (from the scans' bounding boxes; Art's hull test decides):
- Most niche statues' bases are larger than the as-built plinths. The rule grows those plinths.
- **Pedestals on the floor.** These figures pass the mouth and become pedestals on the floor:
  - the Laocoön, by about 0.31;
  - the Barberini Faun, by about 0.44;
  - the Doryphoros, by about 0.17;
  - Augustus and the Lansdowne Heracles, by about 0.10.
- **Head clearance.** The Belvedere Antinous (2.335) leaves about 0.002 under the tribune niche's head at the as-built
  0.08 plinth. The rule lowers its plinth to about 0.06.
- **Too big for the carrier:**
  - The Artemision Zeus's base (1.09) is larger than its 0.90 plinth.
  - Philip the Arab (0.52 wide) and the Athena Parthenos (0.34 deep) are larger than their consoles (0.36 × 0.30).

### Captions

Captions are FMuseeText. The visitor gazes at a cast and its caption shows (IMuseeCaptioned). The credit line is
"Cast: SMK, KASnnn; scan public domain". There are no physical wall labels tonight.

---

## 9. Sound

- **Emitters.** None, as built.
- **Acoustic character.** The hall is one Enclosed volume of about 1350 m³: the gallery about 680, the throat about 23,
  and the tribune about 645.
  - It is bounded almost entirely by polished and honed marble and run lime plaster. The coffers add area and scatter.
  - With typical absorption, a Sabine estimate gives a long, bright reverberation of about 7–9 s at 500 Hz, like a
    marble church.
  - Sound.ReverberationFromMatter computes the real figure from Form's made facts and Matter's absorption coefficients,
    which the new stone records must carry. Nobody tunes it.
- **Footfalls.** Hard and crisp, on the polished statuary and coloured marbles, by substance.
- **Occlusion.** The port is an open portal, so the joint passes sound. The stair's footfalls, and the drum's mechanism
  when it moves (the Rotunda's), carry down the well into the gallery. The hall is otherwise sealed.
- **For polish, not tonight:** a real underground gallery has an air-handling system, whose faint supply noise would be
  a real emitter.

---

## 10. Visitor

1. **Arrival.** The visitor raises the sun clock (the Rotunda's) and walks down the spiral stair to the landing. They
   pass the joint at the port into the gallery, facing north, with the whole enfilade ahead: four laylit bays, the
   screen, and the Apollo Belvedere in the far niche of the tribune on the axis.
2. **The gallery.** The visitor walks the 24 m gallery. Statues stand in the red niches and busts on consoles between
   them. They pass round the reclining Ariadne and the Dying Gaul on the axis.
3. **The tribune.** Through the screen is the tribune. The visitor can walk the ambulatory round the ring of columns, or
   step in under the eye to the Belvedere Torso.
4. **Back.** The visitor leaves the way they came. This is a dead end.

- **What there is to do.** Walk and look. Gazing at a cast shows its caption. There are no seats as built, nothing to
  use, and no prompts other than captions.
- **Wayfinding.** Site places a sign at the port: "Rotunda", up the stair.
- **The body.** The floor is flat at z 0 everywhere. By FMuseeHumanMeasure the body can't step onto a niche sill (0.45)
  or any carrier (at least 0.30), so no hidden blockers are needed: collision is Form's own solids. The as-built
  invisible guards are retired.
- **Clear widths.** The ambulatory is clear from r 3.64 to 6.60. The plinths at r 5.10 leave about 1.0 on either side.
  Between each axis plinth and the wall's dado rail the gallery's aisle is about 2.3–2.4 wide, or about 2.25 where a
  niche pedestal projects.

---

## 11. The old code map

These files are read-only references. Re-express the geometry through Form, Matter, Light and Art; don't copy the old
kit. Paths are relative to `C:\Users\anan9\Desktop\Projects\Musee Vision\`.

| What | Where |
|---|---|
| The hall's plan numbers | `windows/Source/MuseeVision/Plan/MuseePlan.h:461-509` |
| The serving shaft and port (Rotunda side, for the mating) | `windows/Source/MuseeVision/Plan/MuseePlan.h:119-158`, `windows/Source/MuseeVision/SunClock/SunClockShaft.cpp:1-100`, `SunClock/SunClock.h:14-52` |
| The design summary | `windows/Source/MuseeVision/ClassicalHall/ClassicalHallStructure.h:48-68` |
| Derived dimensions (vault radius, jambs, lip, shell) | `ClassicalHall/ClassicalHallGeometry.cpp:59-143` |
| Station lists and bay and pair centres | `ClassicalHallGeometry.cpp:145-213` |
| Niches (flat and curved, the head, the reveal) | `ClassicalHallGeometry.cpp:215-363`, `859-875` (gallery), `1065-1083` (tribune) |
| Profiles: entablature, skirting, dado rail, pilaster base, surround, door architrave (retired), panel frame | `ClassicalHallGeometry.cpp:365-455` |
| Column section, volute, column, pilaster, dentils | `ClassicalHallGeometry.cpp:501-710` |
| The coffer, rosette and surfaces | `ClassicalHallGeometry.cpp:712-847` |
| Gallery walls, south wall, lip, passage, sleeve | `ClassicalHallGeometry.cpp:877-1061` |
| The tribune wall | `ClassicalHallGeometry.cpp:1086-1119` |
| The gallery vault and lanterns | `ClassicalHallGeometry.cpp:1123-1254` |
| The annular vault | `ClassicalHallGeometry.cpp:1257-1317` |
| The dome, the eye and the bead | `ClassicalHallGeometry.cpp:1320-1400` |
| The ring beam | `ClassicalHallGeometry.cpp:1403-1418` |
| Floors: the gallery carpet and the tribune pattern | `ClassicalHallGeometry.cpp:1482-1671` |
| The lintel and soffit | `ClassicalHallGeometry.cpp:1675-1700` |
| The shell (the footprint's evidence) | `ClassicalHallGeometry.cpp:1704-1772` |
| The entablatures, wall mouldings and tribune entablature | `ClassicalHallGeometry.cpp:1777-1853` |
| Plinths, herms, keystones, framed panels, niche fittings, consoles | `ClassicalHallGeometry.cpp:1858-2019` |
| The door frame (retired: Site's joint) | `ClassicalHallGeometry.cpp:2022-2041` |
| The pedestal | `ClassicalHallGeometry.cpp:2043-2052` |
| Pilasters and column placement | `ClassicalHallGeometry.cpp:2057-2101` |
| The works' places (specs, herm bases, spots, centrepiece) | `ClassicalHallGeometry.cpp:2105-2132`, `2181-2321` |
| Hidden blockers (retired: collision comes from the solids) | `ClassicalHallGeometry.cpp:2138-2178` |
| Lights (evidence only: laylight nits, coves, washers, accents) | `ClassicalHallGeometry.cpp:2323-2486` |
| The assembly order and the lunette panel | `ClassicalHallGeometry.cpp:2489-2558` |
| Material names, colours, the diffuser | `ClassicalHall/ClassicalHallStructure.cpp:147-162`, `279-319` |
| Light placement (evidence) | `ClassicalHallStructure.cpp:322-366` |
| The old geometry kit (don't port; Form replaces it) | `ClassicalHall/ClassicalHallKit.h:1-799` |
| The hang (which cast goes where) | `windows/Scripts/classical_art.py:144-192` |
| Scan scaling to true size | `windows/Scripts/classical_art.py:109-141` |
| Cast sizes | `assets/classical/SOURCES.json` |
| Labels | `data/classical.json` |
| Materials: polished statuary floors, cast plaster, gilt, the coloured marbles, statuary, stucco | `windows/Scripts/materials.py:1055-1062`, `1103-1124`, `1144-1175`, `1217-1256`, `1334-1339` |
| The cream marble's coursing | `windows/Scripts/materials.py:1063-1071`, `1336-1338` |
| The coloured marbles' images (evidence for Matter) | `windows/Scripts/stone_textures.py:488-635` |
| The bronze angle on the sill (retired: the joint's strip) | `windows/Source/MuseeVision/Furniture/MuseeFurniture.cpp:336-344` |
| The pond above the tribune (a neighbour's constraint) | `windows/Source/MuseeVision/Chenghuai/ChenghuaiPlan.h:135-137`, `Chenghuai/ChenghuaiGarden.cpp:28-45` |
| Boards | `plan/canvas/Rotunda.dc.html` (long section and plan), `plan/canvas/AsBuiltClassical.dc.html`, `plan/canvas/MaterialsStone.dc.html`, `plan/canvas/MaterialsOther.dc.html` |

---

## 12. First-pass cut

**Built tonight.**
1. **The frame.** The port with its receiving well; the footprint volume; one Enclosed enclosure. The shell is sealed
   except at the port. The rough opening and the collar are built to the class.
2. **The walls.** Walls and the dado split. The 8 gallery niches and 5 tribune niches with their heads. The throat and
   the lintel.
3. **The vaults.** The gallery barrel with its 256 stepped coffers and 4 lantern wells. The annular vault's 420
   coffers. The dome's 72 coffers and the eye well. The ring beam.
4. **The order.** 26 columns (turned, entasis, Ionic capitals with spiral volutes) and 10 pilasters. The gallery
   entablature with its ressauts and fillers, the tribune entablature, and the dentils.
5. **The mouldings.** Skirting, dado rail, niche surrounds and keystones. 26 framed panels with consoles and the lunette
   panel.
6. **The floors.** The opus sectile floors: the gallery carpet and the tribune's polar pattern.
7. **The carriers.** Niche plinths sized by the hang rule, the centrepiece pedestal, and Art's plinths and herms.
8. **Art.** The 41 casts at true size on their anchors, with captions. The hang check's findings go to the owner's
   walk.
9. **Light.** The laylights (5), the coves (8 runs and 3 rings), the washer mount and the accent mounts. Commissioned to
   the targets at 4000 K.
10. **Sound.** RT60 from Matter; footfalls.

**Waits for polish.**
- The gilt rosettes on the coffer backs and the eye's bead. They are tonight only if Form's Carving (a lathe with angular
  modulation) has landed; otherwise the coffer backs stay plain.
- Physical wall labels.
- An air-handling emitter.
- Benches in the tribune (not as built; an option for the owner).
- Pore-soiling and wear calibration against photographs.
- Resolving the hang findings (the pedestals, Philip, the Athena, the Artemision plinth), which the owner decides on
  Sunday.
- Deeper laylight plenums, if the Architect changes the section (see Risks).

---

## Risks

1. **The laylights have almost no depth.** There is only 0.07 between the diffusers (z 5.38) and the stage's top
   (z 5.45, museum −0.55).
   - Only an edge-lit luminous panel of 50 mm or less is physically real there.
   - The catalogue has no such kind and no 4000 K cove. Without `luminous-panel.4000k` and `linear-cove.4000k` the hall
     has no general light tonight.
   - The roof's own thickness over the lanterns (0.02–0.07) also relies on the structure of the stages above, down to
     −0.55.
2. **The stage lies under other stages.** It sits under the Rotunda's drum and north passage, and under Chenghuai,
   whose pond bed is at museum −0.46 over the tribune, 0.09 above this roof.
   - Site.NoOverlap and Site.RoomStaysInside must test volumes (plan by z), or they will report false overlaps.
   - The neighbours must keep their footings and basins above −0.55.
3. **The mating face is a cylinder.** The port face is the cylinder r 7.60, while the room's own face 0.60–0.82 behind
   it is flat.
   - Site's joint has to lay the threshold, the bronze strip on the arc and the lining through a wall depth that varies
     across the opening.
   - The Rotunda must declare the identical well (r 7.60 about the same axis, museum −6.40 to 0.00) and the same class,
     or Site.PortsMate fails.
4. **The casts don't fit the as-built carriers.**
   - Most niche statues' bases are larger than the plinths.
   - The Laocoön and the Faun pass their niches' mouths by 0.3–0.44.
   - The Belvedere Antinous leaves about 2 mm under its niche's head.
   - The Artemision's base is larger than its plinth, and Philip and the Athena are larger than their consoles.

   The hang rule makes this visible and physical, but the Sunday walk will show pedestals in front of some niches.
5. **Matter records are missing.** Six stone accents, the Pompeian stucco and opal glass don't exist yet. With no
   fallback material, Matter.Resolve fails and the stage can't be made until they land.
6. **The casts have no care class.** `Data/Art/Care.json` lists no medium for a plaster cast. Art must add "plaster
   cast" to `stone-and-bronze`, or Art.WithinCare can't resolve the 41 targets.
7. **The doorway changes visibly.** Site's joint replaces the as-built carved architrave, keystone, plinth blocks and
   porphyry sill with the museum's one travertine doorcase and bronze strip, following principle 2.
8. **Residency.** The stage is visible only while the Rotunda's drum is raised. Site's residency must follow the serving
   mechanism's state, or the hall streams in late or stays loaded.
9. **Making cost.** The stage has about 750 coffers of 7 rings each, 26 carved capitals, and 41 scans of 0.4–2 M
   triangles (capitoline-antinous 2.0 M, homer 0.9 M): about 18 M triangles of Nanite in the scans alone.
   Path-traced commissioning of the coffers may be slow; one reference moment is enough (section 5).
10. **Structure.** The vault crown behind the coffers is only 0.12 to the roof top, and the gallery passes under the
    Rotunda's drum wall. Form.MinThickness may flag the roof over the lanterns. That is a design question for the
    Architect, not something for the Room Builder to patch.
