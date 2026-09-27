# Chenghuai 澄懷: the stage sheet

Chenghuai is the three-court Beijing house (三进四合院) on the Rotunda's north door, with a Suzhou garden (臥遊) beside
it to the east. Both names come from 宗炳: 澄懷觀道，臥以遊之, "clear the heart to see the Way, and travel through it
lying down." The house holds the pictures; the garden is where you walk them. There is one door, the Rotunda's, and
one loop of sixteen stops.

The Room Builder builds `MuseeChenghuai` from this sheet tonight (D-016, first pass). Every number is in metres, in the
stage's own frame, unless it is marked **plan** (the old museum plan: Rotunda centre at the origin, x east, y south).
It is re-expressed from the as-built code and the approved proposal (`plan/proposals/chenghuai`), and follows
Decisions D-001 to D-016.

**Sources, in order of authority (D-001):**
1. Decisions D-001 to D-016 and the owner's standing rules (principles; "no characters on or over a door or gate").
2. The approved proposal boards: `plan/proposals/chenghuai/canvas/*.dc.html` and `boards/*.png` (Main, Plan, Rooms,
   Hang, Materials, Garden, Section, Rules, Names).
3. The as-built code: `windows/Source/MuseeVision/Chenghuai/*` and `windows/Scripts/chenghuai*.py`.
4. The data: `assets/collection.json`, `windows/SourceArt/Textures/ch_vessels.json`, `assets/paintings/`.

Where the code and a board disagree, the board wins and the change is listed in Appendix B. Where the as-built breaks
a physical truth the new checks guard (headroom, step height, a trap, an overlap), the fix is listed there too.

**One conversion does it all:** local x = plan x; **local y = plan y + 17.0**; local z = plan z. Every number from
the old code carries over by adding 17.0 to its y.

---

## 1. Identity and placement

| | |
|---|---|
| Stage key | `chenghuai` (display text *澄懷 Chenghuai*, FText) |
| Module | `MuseeChenghuai` (Editor), `Source/Stages/Rooms/Chenghuai`; class `UChenghuaiRoom : USiteRoom` |
| Plan numbers | Only in `Private/ChenghuaiPlan.h` (the house) and `Private/ChenghuaiGardenPlan.h` (the garden's outlines, Appendix A). The Qing hall composition (§3.0) is singular to this stage, so it stays private (the promotion rule) |
| Era and working | Classical wing. Chinese: stone bush-hammered, brick rubbed or pointed, timber lacquered and painted |

### The frame

- **Origin:** on the Rotunda's north–south axis (the house's gate axis), on the site's south outer face (the line of
  the gate platform's front edge and the front row's back wall outer face), at the finished-floor datum z 0.
- **In the museum plan:** plan **(0.0, −17.0, 0.0)**. The Rotunda's centre lies at local **(0, +17.0)**; its drum's
  outer face (R 11.2) crosses the axis at local **(0, +5.8)**.
- Axes: x east, y south, z up, true north. Site translates only.
- z 0 is the Rotunda's floor, the porch's paving and the datum of the courts. Courts' paving tops are at +0.02; the
  garden's ground is at +0.02.

### Level

Level 0 (ground). The house stands on stepped platforms (台基) from 0.30 to 0.90. Nothing is underground except the
pond (bed −0.85) and footings to −0.10.

**The Classical Hall lies beneath** (a separate stage, floor −6.0, roof −0.55). Its box runs under the porch, the gate,
the entry yard and the pond's west lobe: the gallery (plan x ±3, y −8.2 … −32.2, i.e. local +8.8 … −15.2) and the
round tribune (centre local **(0, −22.3)**, masonry to r 7.9). **Chenghuai builds nothing below z −0.50 inside the
Classical Hall's footprint.** The pond's bed rises to −0.46 there (§7), and the bridge's westmost piers stop on it
(Appendix B, item 9).

### Footprint (Building)

Plan polygon, local, counter-clockwise from the south-west corner:

```
(−20.70,  0.000) (−2.60, 0.000) (−2.60, 6.106)
  arc of r 11.2 about (0, 17.0), passing (0, 5.800)
( 2.60,  6.106) ( 2.60, 0.000) (16.90, 0.000) (16.90, −44.900) (−20.70, −44.900)
```

- The south edge between x ±2.6 is the drum's own circle: Chenghuai's porch meets the Rotunda there, and the two
  footprints touch only along that arc.
- Height range: z −1.00 … +12.00 (the tallest piece is the main hall's ridge at 8.46; trees grow to about 10).
  Inside the Classical Hall's footprint the range starts at −0.50.
- Area: the site is 37.6 × 44.9 = 1,688 m² (house 23.0 × 44.9, garden 14.6 × 44.9, Main board), plus the porch.
- **The neighbour to the south-west.** The Salon's Manet cabinet (plan x −24.4 … −15.6, north face plan y −16.0) is
  1.0 m south of Chenghuai's south face. Chenghuai builds nothing south of y 0 except the porch, so the slot belongs to
  the Salon's and the Grounds' sheets (Grounds sheet, open item 4).

### Enclosures

The house's three courts and its entry yard are **Court** enclosures: each shell is closed by walls and buildings at
least 3.2 m high, and its top is one declared Open opening at z 3.20. The porch (1.1 m walls) and the garden (whose
rockery pavilion puts the eye at 4.35 over 3.6 m walls) are open to the air on their sides, so they are **Outdoors**.
The rooms are **Enclosed**.

| Key | Kind | Plan extent (local) | Floor | Notes |
|---|---|---|---|---|
| `chenghuai.porch` | Outdoors | x ±2.0, y 0 … the drum arc (+5.8 on the axis) | 0.00 | Low walls 1.10 |
| `chenghuai.gate-hall` | Enclosed | x ±1.8, y −0.3 … −5.2 (the column lines) | 0.90 | Open S and N between the gables |
| `chenghuai.entry-yard` | Court | x −1.9 … 1.8, y −5.5 … −11.1 | 0.02 | Top Open at 3.20 |
| `chenghuai.front-court` | Court | x −20.2 … −2.3, y −5.3 (臨池's lattice) … −11.1, minus 臨池 | 0.02 (the platform's edge 0.30) | Top Open at 3.20 |
| `chenghuai.linchi` | Enclosed | x −20.2 … −2.3, y −5.3 … −0.5 | 0.30 | 倒座房 · 臨池 |
| `chenghuai.inner-court` | Court | x −20.2 … 1.8, y −11.5 … −30.7, minus the side halls' rooms, the main hall's and ear rooms' interiors; includes the 垂花门, the galleries, the verandas and both corner passages | 0.02 (galleries 0.45 / 0.75) | Top Open at 3.20 |
| `chenghuai.tianqing` | Enclosed | x −20.2 … −16.3, y −23.9 … −14.5 | 0.45 | 西厢房 · 天青 |
| `chenghuai.changnan` | Enclosed | x −2.1 … 1.8, y −23.9 … −14.5 | 0.45 | 东厢房 · 昌南 |
| `chenghuai.chenghuaitang` | Enclosed | x −14.2 … −4.2, y −28.3 … −34.4 | 0.75 | 正房 · 澄懷堂, open to the rafters |
| `chenghuai.qingbi` | Enclosed | x −17.2 … −14.7, y −28.9 … −33.4 | 0.50 | 西耳房 · 清閟 |
| `chenghuai.tingyun` | Enclosed | x −3.7 … −1.2, y −28.9 … −33.4 | 0.50 | 东耳房 · 停雲 |
| `chenghuai.rear-court` | Court | x −20.2 … 1.8, y −31.1 … −39.7 (舒卷's lattice), minus the main hall and ear rooms | 0.02 | Top Open at 3.20. No art: a pause |
| `chenghuai.shujuan` | Enclosed | x −20.2 … 1.8, y −39.7 … −44.4 | 0.30 | 后罩房 · 舒卷 |
| `chenghuai.garden` | Outdoors | x 2.3 … 16.4, y −0.5 … −44.4 | 0.02 | 臥遊: the walk, the pond, the rockery and its pavilion, the water pavilion |
| `chenghuai.linquan` | Enclosed | x 7.0 … 14.3, y −38.5 … −42.1 | 0.45 | 花厅 · 林泉 |

**Openings between enclosures of this stage** (the doorways inside the house). Form's kinds (Glazed, Open, Port,
Housing) have no kind for an unglazed opening shared by two enclosures of one stage; Albion's sheet raises the same
gap. Declare these as **Open**, shared by the two enclosures, and Form must accept an Open opening that is not to the
sky. If it cannot tonight, merge each room into its court (one RT60 each) rather than invent a kind.

| Opening | Between | Size (clear) |
|---|---|---|
| Gate hall front / back | porch / gate hall / entry yard | 3.6 × to the eave (column lines y −0.3, −5.2) |
| Yard door (屏门) | entry yard / front court | y −7.5 … −9.3 in the wall at x −2.3 … −1.9; 1.80 × 2.35 |
| Garden door | entry yard / garden | y −7.5 … −9.3 in the east wall at x 1.8 … 2.3; 1.80 × 2.35 (stone surround) |
| 垂花门 front | front court / inner court | x −10.36 … −8.04 (2.32 clear) at y −11.3; sill 0.12, head at 2.674 |
| Cross-wall doors | inner court / rear court | x −19.5 … −18.4 and 0.0 … 1.1 at y −31.1 … −30.7; 1.10 × 2.30 |
| Moon gate (月洞门 · 臥遊) | rear court / garden | Ø 2.40, centre y −37.3 in the east wall; cut flat at z 0.10 |
| Room door bays | each room / its court | the open pairs of leaves (§3.5) |
| Hall gable doors | 澄懷堂 / 清閟, 停雲 | y −31.4 … −32.6 through the hall's gables; 2.30 high above the hall floor |
| Leak windows (漏窗) | court / court, court / garden | §3.4; lattice in them, not passable |

**Exterior openings** (to the Grounds, other than the port): the garden walls' seven leak windows (§3.4) and every
Court's top. Site keeps the Grounds resident while the visitor is in Chenghuai.

---

## 2. Ports

### `chenghuai.south`: the stage's only port

| Field | Value |
|---|---|
| Kind | Doorway |
| Class | `doorway.4000x4500.arched`: 4.00 m clear, semicircular head springing at 4.50 (crown 6.50). The class of the Rotunda's north and south doors (`MuseePlan.h:44-47`); Albion's sheet uses the same key |
| Frame | Local **(0.00, +5.80, 0.00)** = plan (0, −11.2, 0): on the drum's outer face at the threshold, on the axis, facing out of Chenghuai, i.e. **south (+y)** |
| Face shape | **Cylindrical, R 11.2**, axis vertical through local (0, +17.0) (the Rotunda's centre). The porch ends on the drum itself |
| Wall depth | **0.00** on Chenghuai's side: the porch is open and has no wall across its mouth |
| Mates | `rotunda.north` (cylindrical R 11.2, drum 1.20 deep). Passage length 0 m. The mating plane is the drum's outer face |
| Closure | Open portal (no leaves) |
| Wells | None |

**What Chenghuai builds at the port:** nothing inside the class's collar envelope. The porch's paving and its two low
walls stop at the collar's boundary off the drum face.

**What Site's joint builds:** the travertine threshold through the drum's 1.20 m; the brushed-bronze strip on the
mating plane, an arc of R 11.2 across the 4.00 m clear width; the linings; the surround on the Rotunda's inner face
and the surround on the drum's outer face (following the drum), both round the 4.0 × 6.5 arch; and the short run of
paving and wall stub that closes the gap between the porch's ends and the drum inside the collar.

**The porch is not Albion's porch.** Albion's port is flat with a 0.30 m end wall and a 7.30 × 7.50 collar of wedge
masonry. Chenghuai's port is cylindrical, so there are no wedges; the collar only holds the surround and the
threshold. The class's collar must therefore be defined for a cylindrical face too (Site's open item, §12).

**On the stand**, Site blinds `chenghuai.south` with the class's blind on the drum arc.

### Topology (D-012)

- **The mating:** `rotunda.north ↔ chenghuai.south` (passage 0 m), one line in `Data/Site/SitePlan.json`. It is
  Chenghuai's only mating.
- **A dead end by design.** "The Rotunda door is the only way in or out ... the garden brings you back to the gate"
  (Plan board). Chenghuai declares no other port and serves or receives no well.
- **The visit** is one loop inside the stage (§10) that returns to the port. Site.Reachable judges it over the stage's
  internal openings and steps.

---

## 3. Pieces by trade

### 3.0 The Qing hall: one composition, drawn fourteen times

Every building of the house and garden is the same Qing official-style carpentry (清式小式大木) on a stone-kerbed
platform, described by one spec in its own building frame and made by one composition in the stage's Private plan.
The frame: **u** along the building's front, **v** from its front eave column line to the back, **z** up. A building is
square to the plan, so a box in its frame is a box in the stage.

**The spec's fields** (re-expressed from `FHallSpec`, `ChenghuaiHall.h:62-114`): frame origin and axes; column axes
along u (ends included); gable walls' outer faces U0, U1 and thickness; floor (platform top); platform extent; eave
column height ColH and diameter D; purlin stations v and the rise (举) per step; rolled roof or not; tile kind; ridge
kind; facade line (the lattice, a veranda in front when > 0); back wall v0 … v1 and whether the back eave is sealed
(封护檐); interior column lines; eave reach front and back; the bays' fill; painted or not; wall and base finishes;
gable openings; steps; mitred roof ends; open ends; roof extent; lattice pattern.

**The section rule** (`ChenghuaiHall.cpp:129-177`), the one law every roof follows:
- Eave column top Zc = floor + ColH. Purlin radius Pr = max(0.09, 0.45 D).
- Seat[0] (the eave purlin's top, the rafters' underside) = Zc + 0.12 + 2 Pr. Seat[i+1] = Seat[i] + Rise[i] ×
  (v[i+1] − v[i]). The rises steepen toward the ridge: 举架 5, 7, 9 tenths.
- The tiles' bed (the lime's top) at a purlin = seat + 0.095 (rafter) + 0.020 (boarding) + 0.140 (lime and mud bed);
  +0.06 more at a rolled ridge. Between knots the bed is a monotone cubic (Fritsch–Carlson) through: the eave's edge,
  the round rafters' end, every purlin (and the rolled ridge's midpoint), and the back edge.
- **The eave:** round rafters run out to v = −0.66 × EaveFront; square flying rafters (飞椽) lie on them from 0.90 m
  behind that point to the edge at v = −EaveFront, at 0.55 of the first step's slope (the eave's lift). A sealed back
  continues the bed at 0.8 × the last step's slope to the back wall's outer face + EaveBack; an open back mirrors the
  front eave.

**Derived heights** (the rule above, checked against the Section board's ridge heights: 正房 8.2, 大门 6.5, 厢房 5.5,
倒座 / 后罩 / 垂花门 5.2):

| Building | Zc | Pr | Seats (z) | Ridge bed | Ridge top |
|---|---|---|---|---|---|
| Front row | 2.800 | 0.117 | 3.154 3.785 4.694 3.785 3.154 | 4.949 | 5.409 |
| Gate hall | 3.850 | 0.135 | 4.240 4.853 5.734 4.853 4.240 | 5.989 | 6.449 |
| Side hall | 3.350 | 0.122 | 3.713 4.263 5.138 5.138 4.263 3.858 | 5.453 | 5.528 |
| Main hall | 4.650 | 0.144 | 5.058 5.708 6.601 7.748 6.623 5.741 5.096 | 8.003 | 8.463 |
| Ear room | 3.900 | 0.117 | 4.254 4.848 5.703 4.848 4.254 | 5.958 | 6.418 |
| Rear row | 2.800 | 0.117 | 3.154 3.773 4.664 3.773 3.154 | 4.919 | 5.379 |
| Gallery | 3.050 | 0.099 | 3.368 3.616 3.616 3.368 | 3.931 | 4.006 |
| 垂花门 front roof | 3.070 | 0.108 | 3.406 3.846 3.406 | 4.101 | 4.561 |
| 垂花门 back roof | 3.070 | 0.108 | 3.406 3.679 3.679 3.337 | 3.994 | 4.069 |
| Covered walk | 2.450 | 0.090 | 2.750 3.313 | (single slope) | — |
| Flower hall | 3.450 | 0.108 | 3.786 4.361 4.717 4.717 4.361 3.786 | 5.032 | 5.107 |
| Water pavilion | 2.950 | 0.090 | 3.250 3.828 3.828 3.250 | 4.143 | 4.218 |

(The covered walk's row uses the raised column of Appendix B, item 5.)

#### The building table

All in local coordinates. "Frame" gives the origin and the directions of u and v.

| Building | Frame origin; u; v | u range (gables) | Columns (u) | Floor | ColH | D | Purlin v | Rise | Roof | Facade v | Back wall v | Eave F/B | Bays |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| **Front row** 倒座房 · 臨池 | (−20.7, −5.3); east; south | 0 … 18.9 (west gable 0.5; east end open: the gate's gable) | 0.5 + k·17.9/6, k 0…6 | 0.30 | 2.50 | 0.26 | 0, 1.2625, 2.525, 3.7875, 5.05 | .5 .72 −.72 −.5 | 合瓦, 清水脊, sealed back | 0 | 4.8 … 5.3 | 0.75 / 0.28 | DoorsOpen, W, W, W, W, DoorsOpen |
| **Gate hall** 广亮大门 | (−2.3, −0.3); east; north | 0 … 4.6 (gables 0.5 both) | joinery §3.6 | 0.90 | 2.95 | 0.30 | 0, 1.225, 2.45, 3.675, 4.9 | .5 .72 −.72 −.5 | 筒瓦, 清水脊, open back | — | none | 0.9 / 0.9 | Open; gables run to v 5.36 |
| **West side hall** 西厢房 · 天青 | (−15.2, −24.4); south; west | 0 … 10.4 (gables 0.5) | 0.5, 3.55, 6.85, 9.9 | 0.45 | 2.90 | 0.27 | 0, 1.1, 2.35, 3.1, 4.35, 5.25 | .5 .7 0 −.7 −.45 | 合瓦, rolled 卷棚, sealed back | 1.1 | 5.0 … 5.5 | 0.85 / 0.30 | W, DoorsOpen, W |
| **East side hall** 东厢房 · 昌南 | the west hall mirrored about x −9.2 (x′ = −18.4 − x) | | | | | | | | | | | | |
| **Main hall** 正房 · 澄懷堂 | (−14.7, −27.0); east; north | 0 … 11.0 (gables 0.5) | 0.5, 3.55, 7.45, 10.5; interior 金柱 at v 6.36 | 0.75 | 3.90 | 0.32 | 0, 1.3, 2.575, 3.85, 5.1, 6.36, 7.65 | .5 .7 .9 −.9 −.7 −.5 | 筒瓦, 清水脊, sealed back | 1.3 | 7.4 … 7.9 | 1.10 / 0.32 | W, DoorsOpen, W |
| **West ear room** 西耳房 · 清閟 | (−17.7, −28.9); east; north | 0 … 3.0 (west gable 0.5; east end open: the hall's gable) | 0.5, 3.0 | 0.50 | 3.40 | 0.26 | 0, 1.1875, 2.375, 3.5625, 4.75 | .5 .72 −.72 −.5 | 合瓦, 清水脊, sealed back; roof to u 3.05 | 0 | 4.5 … 5.0 | 0.80 / 0.25 | Doors (closed) |
| **East ear room** 东耳房 · 停雲 | the west ear room mirrored about x −9.2 | | | | | | | | | | | | |
| **Rear row** 后罩房 · 舒卷 | (−20.7, −39.7); east; north | 0 … 23.0 (gables 0.5) | 0.5 + k·22/7, k 0…7 | 0.30 | 2.50 | 0.26 | 0, 1.2375, 2.475, 3.7125, 4.95 | .5 .72 −.72 −.5 | 合瓦, 清水脊, sealed back | 0 | 4.7 … 5.2 | 0.75 / 0.28 | DoorsOpen, W, W, Doors, W, W, DoorsOpen |
| **Flower hall** 花厅 · 林泉 | (6.6, −38.5); east; north | 0 … 8.1 (gables 0.4) | 0.4, 2.4, 5.7, 7.7 | 0.45 | 3.00 | 0.24 | 0, 1.15, 1.625, 2.125, 2.6, 3.75 | .5 .75 0 −.75 −.5 | 合瓦, rolled, open back over a back wall | 0 | 3.6 … 3.9 | 0.80 / 0.70 | Doors (冰裂纹), DoorsOpen (灯景), Doors |
| **Water pavilion** 水榭 · 知魚 | (13.3, −29.1); north; east | 0 … 4.5, roof −0.55 … 5.05; no gables | 0.3, 4.2, front and back lines | 0.35 | 2.60 | 0.20 | 0, 1.05, 1.75, 2.8 | .55 0 −.55 | 合瓦, rolled, back columns | — | none | 0.75 / 0.10 | Open |
| **Covered walk** 复廊 · 遊目 | (3.55, −5.5); north; west | 0 … 33.8, roof −0.3 … 34.1; no gables | 0.3 + 2.7k, k 0…12 (13 columns, front line x 3.55 only) | 0.15 | **2.30** (B-5) | 0.20 | 0, 1.25 | .45 | 合瓦, single slope to the house wall | — | the house's east wall | 0.55 / 0.06 | Open |

Galleries (抄手游廊, all painted row 3, rolled 合瓦 roofs, floor 0.45, ColH 2.60, D 0.22, purlins v 0, 0.45, 0.85,
1.30, rises .55 0 −.55, eaves 0.60 both, no gables, back columns; each has an east twin mirrored about x −9.2):

| Gallery | Frame origin; u; v | u range | Columns / frames | Roof ends | Platform |
|---|---|---|---|---|---|
| G1 south (along the court wall) | (−16.3, −12.9); east; south | 0 … 5.4 | columns u 0, 1.3, 3.4, 5.2 | mitred at u0 (slope −1 about v 0), roof u 1.3 … 5.45; back eave 0.12 into the court wall | u −0.1 … 5.4, v −0.1 … 1.4 |
| G2 west, south part | (−15.0, −11.6); north; west | 1.3 … 2.45 | frames at u 2.3; its columns stand at y −13.9, x −15.0 and −16.3 | mitred at u0 | u 1.4 … 2.4 |
| G3 west, north part | (−15.0, −24.4); north; west | −0.05 … 3.8 | frames at u 0.1, 2.4 | mitred at u1 (slope +1), roof −0.05 … 2.5 | built by §3.1 |
| G4 turn into the hall's veranda | (−15.0, −26.9); east; north | 0 … 0.35 | none | mitred at u0 | built by §3.1 |

The side halls' verandas (1.1 m, between the eave columns and the lattice) run on through both gables as the galleries'
middle length (gable openings v 0.06 … 1.04, 2.30 high, framed).

### 3.1 Platforms, steps and stone

**Platforms (台基)** (`ChenghuaiHall.cpp:424-453`): a brick body from −0.10 to (floor − 0.13); a stone footing course
(土衬) 0.05 proud all round from −0.10 to +0.03; a stone kerb (阶条石) 0.34 wide × 0.13 thick round the top; corner
stones (埋头) 0.30 square at the front corners; the floor tiles inside the kerb. A platform whose floor is ≤ 0.21
(the walk, the terrace) has kerb stones solid from +0.03 to the floor instead. Platform extents per building (u × v):

| Building | Platform u | Platform v | Body |
|---|---|---|---|
| Front row | 0 … 18.4 | −0.6 … 5.3 | lime-pointed brick |
| Gate hall | 0 … 4.6 | −0.3 … 5.2 | rubbed brick |
| Side halls | 0 … 10.4 | −0.6 … 5.5 | lime-pointed brick |
| Main hall | 0 … 11.0 | −0.7 … 7.9 | rubbed brick (干摆 base) |
| Ear rooms | 0 … 3.0 | −0.6 … 5.0 | lime-pointed brick |
| Rear row | 0 … 23.0 | −0.6 … 5.2 | lime-pointed brick |
| Flower hall | −0.05 … 8.15 | −0.3 … 3.95 | bluestone |
| Water pavilion | −0.05 … 4.55 | −0.3 … 3.1 | bluestone; its west 1.3 m stands in the pond as a bluestone base from the bed |
| Covered walk | −0.1 … 33.9 | −0.15 … 1.28 | bluestone kerbs, solid |

**The galleries' floors** (`ChenghuaiHouse.cpp:580-622`), west side (mirror east): platform x −16.4 … −14.9,
y −26.0 … −24.35, top 0.45, kerb along x −14.95 … −14.85; step up at y −26.0 … −26.33; upper platform x −16.4 … −14.7,
y −26.33 … −28.35, top 0.75, kerbed; columns at (−15.0, −24.5), (−15.0, −26.8), (−16.3, −24.5), (−16.3, −26.8),
(−16.3, −28.2) (floor 0.45 north of y −26.2, 0.75 beyond) and at (−15.0, −13.9), (−16.3, −13.9).

**Steps (踏跺)** are Form stairs by Blondel's rule (2R + T = 0.63, FFormStair), fanned from the court's level to the
platform, stone treads, with sloping side stones (垂带) 0.30 wide either side. Every flight:

| Flight | Where (local) | Width | Rise | Risers | R | T | Run |
|---|---|---|---|---|---|---|---|
| Gate, front | x ±1.2, y +1.10 → 0 (up north onto the gate platform) | 2.40 | 0.90 | 5 | 0.180 | 0.275 | 1.10 |
| Gate, back | x ±1.2, y −5.5 → −6.612 (down into the yard) | 2.40 | 0.88 | 5 | 0.176 | 0.278 | 1.112 |
| Front row ×2 | centred x −18.708, −3.792; y −5.9 → −6.25 | 1.50 | 0.28 | 2 | 0.140 | 0.350 | 0.35 |
| Side halls ×2 | centred y −19.2 at x −14.6 (west hall, down east), mirror | 2.00 | 0.43 | 3 | 0.143 | 0.343 | 0.687 |
| Main hall | centred x −9.2, y −26.3 → −25.505 | 3.00 | 0.73 | 4 | 0.1825 | 0.265 | 0.795 |
| Rear row ×3 | centred x −18.629, −9.200, 0.229; y −39.1 → −38.75 | 1.50 | 0.28 | 2 | 0.140 | 0.350 | 0.35 |
| 垂花门 | x −10.2 … −8.2, y −9.9 → −9.213 | 2.00 | 0.43 | 3 | 0.143 | 0.343 | 0.687 |
| Gallery, up to the hall veranda ×2 | x −16.4 … −14.95, y −26.0 → −26.33 | 1.45 | 0.30 | 2 | 0.150 | 0.330 | 0.33 |
| Gallery, off to the corner passage ×2 | x −16.4 → −17.087, y −26.0 … −24.8 | 1.20 | 0.43 | 3 | 0.143 | 0.343 | 0.687 |
| Water pavilion ×2 (new, B-7) | at the path ends, x 14.6 … 15.8, y −29.05 and −33.65 | 1.20 | 0.33 | 2 | 0.165 | 0.300 | 0.30 |
| Flower hall → terrace | x 9.9 … 11.4 | 1.50 | 0.15 | 1 | 0.150 | — | — |
| Terrace → garden | x 9.9 … 11.4, y −36.7 → −36.35 | 1.50 | 0.28 | 2 | 0.140 | 0.350 | 0.35 |

Thresholds and sills: every wall doorway has a stone threshold 0.10 wider than the opening each side, 0.06 over the
ground, 0.06 past both faces; the gate's wooden sill (门槛) is **0.20** high (B-8); partition doors' sills 0.10; the
垂花门's 0.12; the moon gate's stone sill 1.80 wide, top 0.102.

**Columns** (`ChenghuaiHall.cpp:220-233`): a square base stone (柱顶石) 1.9 D, from floor −0.08 to +0.012; a turned drum
(鼓镜), profile (r, z) about the axis:

```
(0, 0) (0.68D, 0) (0.68D, 0.012) (0.62D, 0.045)* (0.55D, 0.055) (0, 0.055)        * smooth
```

The shaft stands on it from +0.06, tapering (收分) from R0 = D/2 to R1 = R0 (1 − 0.001 H/D) at the top, its foot's
arris eased 8 mm. Facade-line and interior (金柱) columns are D + 0.03. The front columns of an open veranda stand free;
the end columns stand inside the gable walls.

### 3.2 Timber frame (大木)

Per building, from the section rule (`ChenghuaiHall.cpp:511-635`):
- **Purlins (檩):** round, radius Pr, their tops on the seats; into the gable walls by 0.12 (or out to an open end).
- **Eave line:** in each bay a lintel (檐枋) 0.9 D + 0.04 high × 0.75 D thick under Zc, and a board (垫板) 0.06
  thick from Zc to Seat[0] − 2 Pr + 0.02. In a veranda, the same pair (金枋) on the facade line under its purlin.
- **Frames across (梁架)** at every free column (or at the listed frame u): the main beam (五架梁 / 七架梁) from the
  front support to the back, W = 0.8 D + 0.04, H = 1.2 D + 0.06, top at min(Seat[a], Seat[b]) − 1.6 Pr; each beam
  above steps up by one purlin each side, ×0.88 wide and ×0.85 high, on struts (瓜柱) 0.7 W square; a block under the
  higher purlin where seats differ; the ridge strut (脊瓜柱) 0.6 W. A veranda adds the 抱头梁 from the eave column to the
  facade column and a tie (穿插枋) 0.6 D × 0.8 D at Zc − 0.75. A gallery's back line gets lintels too.
- **Rafters (椽):** round, Ø 0.095, one for every 0.22 of u (一椽一当), purlin to purlin, 0.04 past each; eave
  round rafters from 0.2 behind the eave purlin to v −0.66 EaveFront; flying rafters (飞椽) 0.085 square, their tails
  0.90 back on the boarding. Inset from the gables 0.12 + gable thickness; mitred ends cut the rafters that would cross.
- **Boarding (望板)** 0.02, the eave boards (小连檐 riser at the round rafters' end; 大连檐 and 瓦口 fascia at the edge),
  the gable ends' brick band (博缝) as a strip under the edge tiles.

Painted buildings (苏式彩画): the gate is plain; the front and rear rows are painted restrained (row 2 / row 0 of the
old atlas); the side halls, main hall, galleries and 垂花门 fully. Painted members: eave lintels and boards, the
facade line's lintels, the beams across, the purlins. Everything else in the house is iron-oxide red; the rafters and
boarding are the red darkened by age and smoke. Garden buildings are chestnut lacquer, unpainted.

### 3.3 Roofs, tiles and ridges

**The roof slab:** the lime bed (top) as a surface over the bed curve across u at 6 stations and along v at ≤ 0.12
plus every knot; the boarding (underside) in two or three pieces either side of the eave's step; risers and fascias;
the ends. It is one sealed piece with the rafters under it (Form.Sealed: the gable walls rise 0.015 into the boarding,
`ChenghuaiHall.cpp:921-923`, so no sun slit shows inside).

**Tiles** (`ChenghuaiRoof.h:61-93`, `ChenghuaiRoof.cpp:22-76, 78-292`), laid in rows down the slope, in courses whose
lower ends stand 9 mm proud of the next (a lip), so the rows read as tiles in grazing light; rows cut exactly by
half-planes at valleys, hips and ends, capped with their own section.

| Style | Where | Row pitch | Lower pan half-width | Upper tile half-width | Pan course | Upper course | Lip |
|---|---|---|---|---|---|---|---|
| 筒瓦 (Tong): concave pans under half-round covers | gate, main hall, 垂花门 | 0.26 | 0.115 | 0.065 (cover) | 0.19 | 0.30 | 0.009 |
| 合瓦 (He, butterfly): pans face up, pans face down | everything else, copings | 0.25 | 0.115 | 0.105 (upper pan) | 0.17 | 0.17 | 0.009 |

Sections (across, height over the bed): lower pan: base −0.01, top 0.02 + 0.03 (o/half)²; cover: base 0.03, top
0.045 + 0.05 √(1 − (o/half)²); upper pan: base 0.035, top 0.05 + 0.035 (1 − (o/half)²). Rows start at the ridge's foot
(0.13 from the ridge line for a 清水脊, 0 for a rolled ridge) and run to the eave's edge; they begin 0.26 in from a gable
(the edge tiles, 排山) at an offset of that + PanHalf. Drip (滴水) and end (勾头) tiles where a row reaches the eave
(first pass: omitted, §12).

**Ridges** (swept sections, `ChenghuaiHall.cpp:844-908`):

清水脊 (the gate, main hall, ear rooms, rows, 垂花门 front), from u(ridge start) + 0.02 to u(end) − 0.02, on the bed at
the ridge line (x across, z up):
```
(−0.17,−0.08) (0.17,−0.08) (0.17,0.06) (0.13,0.08) (0.13,0.12) (0.105,0.14) (0.105,0.34) (0.12,0.36) (0.12,0.40)
(0.075,0.44)* (0,0.46)* (−0.075,0.44)* (−0.12,0.40) (−0.12,0.36) (−0.105,0.34) (−0.105,0.14) (−0.13,0.12)
(−0.13,0.08) (−0.17,0.06)
```
Scorpion tails (蝎子尾), one at each end, centred 0.35 in: a plate 0.56 (u) × 0.32 × 0.07 at ridge +0.40 … +0.47; the
tail swept along u = end ± (0.10 + 0.55 t), z = ridge + 0.44 + 0.34 t + 0.08 t², t 0 … 1 (9 stations), section
`(−0.06,−0.06) (0.06,−0.06) (0.06,0.04) (0,0.07)* (−0.06,0.04)`.

Rolled ridge (过垄脊: side halls, galleries, 垂花门 back, flower hall, water pavilion):
`(−0.16,−0.04) (0.16,−0.04) (0.16,0.02) (0.10,0.06)* (0,0.075)* (−0.10,0.06)* (−0.16,0.02)`.

Gable edges (排山) on every hard-gabled roof, down each slope at bed + 0.02 from the ridge to 0.02 past the eave, 17
stations: `(−0.145,−0.06) (0.145,−0.06) (0.145,0.05) (0.08,0.11)* (0,0.13)* (−0.08,0.11)* (−0.145,0.05)`.

### 3.4 Walls

**Gable walls (硬山)** (`ChenghuaiHall.cpp:912-989`): from v −0.36 to the back wall's outer face (or v 5.36 for the
gate), top following the boarding's underside + 0.015; a base (下碱) of rubbed brick 1.00 high, 15 mm proud outside;
the body above in the building's wall finish; 15 mm lime plaster inside where it faces a room (behind the lattice
line). Openings through a gable get brick over their head and, if framed, a red frame (head 0.10, jambs 0.08).
The corbelled front end (墀头): a stone at the base's top (−0.12 … 0, 0.55 deep); five rubbed courses 0.085 high from
Zc − 0.35, each stepping out 0.055 k forward and 0.02 k sideways; a sloping board to the eave's underside.

**Back walls** (`ChenghuaiHall.cpp:991-1021`): between the gables, v0 … v1 above, base from floor − 0.05 to floor + 1.0,
top following the boarding; plaster skin inside; a sealed eave's corbelled cornice (冰盘檐), four courses 0.07 high
stepping 0.06 k out under the roof's back edge. Back walls of the front row, side halls and rear row are the house's
outer walls.

**Wall finishes:** gate and 垂花门 bases: rubbed brick; main hall, side halls, ear rooms: hairline-jointed above a rubbed
base; front and rear rows: lime-pointed above a rubbed base.

**Free-standing walls** (`ChenghuaiParts.cpp:226-340`): base course (rubbed) with its own height, body above, optional
skins, and a tiled coping on top: a corbel course 0.07 high, (T + 0.10) wide; a lime bed of section
`(−Hf,0) (Hf,0) (Hf,0.05) (0,0.18) (−Hf,0.05)` with Hf = T/2 + 0.14; 合瓦 rows down both sides from 0.06 to Hf; a
coping ridge `(−0.08,−0.04) (0.08,−0.04) (0.08,0.06) (0.05,0.11)* (0,0.125)* (−0.05,0.11)* (−0.08,0.06)`.

| Wall | Line (local) | Thick | Height | Base | Body (faces) | Openings |
|---|---|---|---|---|---|---|
| House west wall | x −20.7 … −20.2; y −5.6 … −14.0 and −24.4 … −39.35 | 0.5 | 4.20 | 0.93 | lime-pointed brick both faces | — |
| House east wall, south run | x 1.8 … 2.3; y −5.5 … −14.0 | 0.5 | 4.20 | 0.93 | lime-pointed; garden face limewashed | garden door y −7.5 … −9.3, 2.35 high, stone surround; leak window y −12.2 … −13.2, z 1.25 … 2.25, circle (ice-crack) |
| House east wall, north run | x 1.8 … 2.3; y −24.4 … −39.35 | 0.5 | 4.20 | 0.93 | as above | leak windows y −26.4 … −27.4 (hexagons), −29.4 … −30.4 (coins), −32.4 … −33.4 (octagon), z 1.25 … 2.25; **moon gate** y −36.1 … −38.5 |
| Court wall (between front and inner courts) | y −11.5 … −11.1; x −20.2 … −10.9 and −7.5 … −2.3 | 0.4 | 3.20 | 0.93 rubbed | limewashed lime both faces | leak windows x −13.6 … −12.6 (octagon) and −5.8 … −4.8 (hexagon), z 1.5 … 2.5 |
| Court wall behind the screen wall | y −11.5 … −11.1; x −2.3 … 1.8 | 0.4 | 3.20 | 0.93 | rubbed brick | — |
| Yard wall | x −2.3 … −1.9; y −5.5 … −11.1 | 0.4 | 3.20 | 0.93 | rubbed brick | yard door (屏门) y −7.5 … −9.3, 2.35 high, frame and **green** leaves folded open (B-4) |
| Cross walls ×2 | y −31.1 … −30.7; x −20.2 … −17.7 and −0.7 … 1.8 | 0.4 | 3.20 | 0.93 | lime-pointed | doors x −19.5 … −18.4 and 0.0 … 1.1, 2.30 high, red frame, red leaves folded open |
| Garden north wall | y −44.9 … −44.4; x 2.3 … 16.9 | 0.5 | 3.60 | 0.55 | limewashed lime both faces (粉墙) | leak windows x 3.9 … 4.9 (hexagons), 15.0 … 16.0 (octagon), z 1.15 … 2.15 |
| Garden east wall | x 16.4 … 16.9; y −0.5 … −44.4 | 0.5 | 3.60 | 0.55 | 粉墙 | leak windows y −21.0 … −22.0 (circle, ice-crack), −25.6 … −26.6 (coins), −37.8 … −38.8 (hexagons) |
| Garden south wall | y −0.5 … 0.0; x 2.3 … 16.4 | 0.5 | 3.60 | 0.55 | 粉墙 | leak windows x 4.4 … 5.4 (octagon), 14.2 … 15.2 (circle) |
| Porch walls ×2 | x −2.6 … −2.0 and 2.0 … 2.6; y +0.04 to the collar | 0.6 | 0.98 + coping to 1.10 | — | rubbed brick, stone coping 0.04 proud each side | — (B-1) |

**Leak windows (漏窗)** (`ChenghuaiParts.cpp:16-124`): a panel filling the rectangle with a shaped hole (circle as a
48-gon; square turned 45° with R√2; hexagon; octagon turned 22.5°), R = 0.46 × min(width, height); a reveal round the
hole; the lattice of rubbed-brick-and-tile bars 0.028 wide × 0.06 deep on the wall's centre plane, clipped to the
shape: ice-crack (9 straight cracks seeded per window), coins (octagon rings Ø 0.24 on a 0.20 grid), hexagons (side
0.11); a rim 0.05 × 0.07 round the shape.

**The moon gate (月洞门 · 臥遊)** (`ChenghuaiParts.cpp:166-178, 297-334`): a circle R 1.20 about (y −37.3, z 1.25),
cut flat at z 0.10 (the threshold); the wall around it a panel from 0.22 outside the circle; a ring of rubbed brick
0.13 wide standing 0.025 proud of both faces; the stone sill 1.80 wide, 0.08 past both faces, from −0.10 to 0.102.

**The screen wall (影壁 · 澄懷觀道)** (`ChenghuaiHouse.cpp:442-463`), against the court wall's south face, x ±1.4, face
at y −10.55, back at −11.1: a stone plinth (须弥座) in three stages, −0.05 … 0.18 (0.08 proud), 0.18 … 0.42 (0.02),
0.42 … 0.55 (0.07); the rubbed-brick body 0.55 … 3.10; the heart (影壁心) 1.80 square centred z 1.80, in a moulded frame
0.10 wide, 35 mm proud, its face 2 cm back (the carving: §8, first pass plain, §12); a cornice of four rubbed courses
0.07 high stepping 0.04 out; a tiled coping at 3.38 over x ±1.6.

### 3.5 Fronts (装修): doors, windows, lattice

On each bay of the facade line (`ChenghuaiHall.cpp:1024-1119`), between A and B (the gable's inner face, or the
column's face: ColR = D/2, +0.015 in a veranda):
- **Frame (槛框):** posts 0.10 against the columns, a head rail 0.10 under the lintel's underside (Head = column top −
  (0.9 D + 0.04)); frame thickness 0.10.
- **Doors (隔扇):** sill 0.10 (0.12 thick); door top = max(min(floor + 2.55, Head − 0.45), floor + 0.10 + 1.95); if at
  least 0.18 is left, a door head 0.10 and a transom (横披) of lattice panels ≈ 0.95 wide with mullions 0.06; otherwise
  the leaves run to the head rail. Four leaves in a bay wider than 2.2, else two. **DoorsOpen:** the middle pair swung
  90° into the room about their outer stiles (fixed poses; §6). Each leaf (`ChenghuaiHall.cpp:394-418`): stiles 0.065,
  thickness 0.065, five rails 0.06 at the foot, at 0.30 H, at 0.39 H, at 0.92 H and the head; an apron panel (裙板) with
  a raised field 0.07 in, a waist panel (绦环板), a top panel, 1 cm back; the lattice head over paper between the waist
  and the upper rail.
- **Windows (支摘窗):** a rubbed-brick sill wall (槛墙) 0.34 thick to floor + 0.79; a sill board (榻板) 0.40 deep to
  floor + 0.85; a window sill 0.08; a middle post 0.08; each half split at 0.42 of its height: the lower sash a plain
  grid, the upper sash the building's pattern.
- **Lattice panels** (`ChenghuaiHall.cpp:278-392`): rim 0.030, bars 0.018 wide × 0.024 deep, mulberry-paper glazing
  6 mm behind the bars on the room side (a Glazed opening per panel). Patterns: grid (cells ≈ 0.13 × 0.15); lantern
  (灯笼框, one ring of cells, margin min(0.11, 0.18 × short side), an open centre with a light cross); step brocade
  (步步锦, two rings, ties alternating); ice-crack (冰裂纹, the panel split again and again by straight cuts, each ending
  on an earlier one, into convex cells 8–14 cm across; about one split per 0.011 m²; seeded by the panel's position).
  Painted buildings take step brocade; plain ones lantern; the flower hall ice-crack with lantern in its open bay.
- **Hanging fretwork (挂落)** under the gallery lintels, 0.30 high, step brocade, on the court side between columns
  (`ChenghuaiHouse.cpp:625-649`).

Bay spans (local, clear between frame posts), for Glazed and Open openings:

| Room | Bay | Span | Fill |
|---|---|---|---|
Spans are A … B (the frame posts stand inside them, 0.10 each).

| Room | Bay | Span A … B | Fill |
|---|---|---|---|
| 臨池 | 1 | x −20.2 … −17.347 | 4 leaves, middle pair open (Open 1.33 wide), outer leaves paper |
| 臨池 | 2–5 | between the columns, ColR 0.13 | windows, paper |
| 臨池 | 6 | x −5.153 … −2.43 | 4 leaves, middle pair open (1.26) |
| 天青 / 昌南 | 1, 3 | y −23.9 … −21.0, −17.4 … −14.5 | windows |
| 天青 / 昌南 | 2 | y −20.7 … −17.7 | 4 leaves, middle pair open (1.40), transom |
| 澄懷堂 | 1, 3 | x −14.2 … −11.325, −7.075 … −4.2 | windows |
| 澄懷堂 | 2 | x −10.975 … −7.425 | 4 leaves, middle pair open (1.68), transom |
| 清閟 / 停雲 | 1 | x −17.2 … −14.83 (mirror) | 2 leaves, closed, paper, transom |
| 舒卷 | 1, 7 | x −20.2 … −17.187, −1.213 … 1.8 | 4 leaves, middle pair open (1.41) |
| 舒卷 | 4 | the middle bay | 4 leaves closed |
| 舒卷 | 2, 3, 5, 6 | between the columns | windows |
| 林泉 | 1, 3 | x 7.0 … 8.88, 12.42 … 14.3 | 4 leaves closed, ice-crack paper, transoms |
| 林泉 | 2 | x 9.12 … 12.18 | 4 leaves, middle pair open (1.43), lantern pattern, transom |

### 3.6 The gates

**The gate (广亮大门)** (`ChenghuaiHouse.cpp:268-344`), in the gate hall frame (origin (−2.3, −0.3), u east, v north),
floor 0.90:
- Four columns at u 0.8, 3.8 × v 0, 4.9 (x ±1.5; y −0.3, −5.2), r 0.15 → 0.145, on bases 0.56 square; two middle
  columns (中柱) at v 2.45 (y −2.75), r 0.16 → 0.155, to Seat[2] − 0.30.
- Beams across at u 0.8 and 3.8: front half v −0.2 … 2.45 and back half 2.45 … 5.1, 0.26 × 0.34, at Seat[0] − 0.28;
  struts 0.18 square under the second purlins.
- The door frame on the ridge line (y −2.75): door clear 1.90 (x ±0.95); posts 0.12 × 0.18 to the head at floor +
  2.75; sill (门槛) 0.20 high (B-8); middle rail x ±1.35 at 2.75 … 2.95 above the floor; top rail under Seat[2] − 0.35;
  the panel over the door (走马板) and side panels (余塞板) 0.06 thick.
- **Four plain door pins (门簪)** on the middle rail at x −0.57, −0.19, 0.19, 0.57, z floor + 2.85, turned from
  `(0,0) (0.075,0) (0.075,0.10) (0.06,0.12)* (0,0.125)` pointing south. **No characters** (Names board: "The gate carries
  no words"; B-3).
- The leaves: black lacquer, 0.95 wide × 0.08 thick, from floor + 0.21 to the head − 0.02, standing open north against
  the side panels (fixed pose). Brass knocker plates (门钹) turned from `(0,0) (0.11,0) (0.11,0.006) (0.05,0.03)* (0,0.035)`
  at floor + 1.35, rings a torus R 0.07, tube 0.009, hanging 0.07 lower.
- Box-type stone drums (门墩) either side of the sill at x ±1.25: base 0.34 × 1.10 (y −2.0 … −3.1) from floor − 0.05 to
  + 0.28; box 0.30 × 0.60 to + 0.70; the lion block 0.20 × 0.40 to + 0.86 (carving: polish).

**The festooned gate (垂花门 · 齋莊中正)** (`ChenghuaiHouse.cpp:348-438`): platform x −10.9 … −7.5, y −12.6 … −9.9, floor
0.45 (rubbed-brick body, kerb 0.30); two roofs one behind the other (一殿一卷), frame origin x −11.35, u east over
0 … 4.3, v north: the front roof from y −10.2 (purlins v 0, 0.55, 1.1; rises .8 −.8; 筒瓦; 清水脊; eaves 0.75 / 0.04),
the back roof from y −11.3 (purlins v 0, 0.42, 0.72, 1.1; rises .65 0 −.9; rolled; 筒瓦; eaves 0.04 / 0.70), sharing the
valley purlin. Columns at x −10.6, −7.8 on the wall line (y −11.3) and the back line (y −12.4), r 0.125 → 0.12, bases
0.44 square. Cantilever beams (麻叶抱头梁), green, 0.20 × 0.30, from y −12.5 to −9.95 at z 3.106 over each column line;
the hanging posts (垂柱) r 0.10 at y −10.2 from 3.256 down to 2.70, each ending in a lotus bud (垂头) turned from
`(0,0) (0.05,0.02)* (0.11,0.10)* (0.13,0.19)* (0.12,0.26)* (0.10,0.30) (0,0.30)` from z 2.40, gilt on brass. A band
of step-brocade lattice 0.30 high between the hanging posts under the lintel (bottom at 2.514). The door frame on the
wall line: posts 0.12 wide at x −10.48 … −10.36 and −8.04 … −7.92, 0.14 thick; head at 2.674 … 2.816; sill 0.12
(0.16 thick). The leaves are taken down by day (none built). **The green screen door (屏门), shut**
on the back line (y −12.3): four panels 0.61 wide from x −10.46 at a 0.63 pitch, 0.06 thick, from 0.58 to 2.654, a red
disc Ø 0.32 at z 2.20 on each with one character of 齋莊中正 (carved and gilt: polish; first pass the plain red disc);
screen head, board and sill. Everyday traffic goes round by the galleries (Rules board).

### 3.7 Courts, paving and beds

Paving is laid in the open court only, cut round every platform's footing (no overlap, Form.NoOverlap); 0.12 deep.

| Area | Extent (local) | Top | Finish |
|---|---|---|---|
| Porch | x ±2.0, y 0 → the collar | 0.00 | city bricks on edge (城砖) |
| Entry yard | x −1.9 … 1.8, y −5.5 … −11.1 | 0.02 | square paving bricks (方砖) |
| Front court | x −20.2 … −2.3, y −5.9 … −11.1 | 0.02 | square paving bricks |
| Front court paths | x −19.6 … −2.3, y −7.5 … −9.3; x −10.1 … −8.3, y −7.5 … −11.1 | 0.026 | city bricks on edge |
| Inner court | x −20.2 … 1.8, y −11.5 … −30.7 | 0.02 | square paving bricks |
| Inner court cross | x −10.1 … −8.3, y −12.6 … −25.3; x −14.6 … −3.8, y −18.3 … −20.1 | 0.026 | city bricks on edge |
| Inner court beds ×4 | x −13.8 … −10.6 and −7.8 … −4.6 × y −20.5 … −24.7 and −13.7 … −17.9 | kerb 0.14 wide to 0.14; earth to 0.08 | stone kerb; earth |
| Rear court | x −20.2 … 1.8, y −31.1 … −39.1 | 0.02 | square paving bricks |
| Rear court path | x −20.2 … 1.8, y −36.7 … −38.0 | 0.026 | city bricks on edge |
| Rear court tree pits ×2 | centred x −16.8, −1.6; y −34.6 … −36.0 (1.40 square) | kerb 0.12 wide to 0.12; earth to 0.05 | stone kerb; earth |

The courts are flat as built. Real courts fall about 1 % to drain; see §12 (risk: Matter's ponding law puts puddles
everywhere on flat paving after rain).

### 3.8 The garden (臥遊)

**Ground and pond** (`ChenghuaiGarden.cpp:79-149`): the garden's earth, a sheet at 0.02 over x 2.3 … 16.4,
y −0.5 … −44.4 with the pond cut out (outline: Appendix A). The bank runs from the edge (at 0.02) 0.45 m inward and
down to the bed; the bed is at **−0.85**, rising to **−0.46** over the Classical Hall's tribune: depth(r) =
lerp(−0.46, −0.85, clamp((r − 8.3) / 1.5, 0, 1)), r measured from local (0, −22.3). **The water is at −0.28**
(0.30 under the ground). Pond area 106.6 m², perimeter 41.7 m, depth 0.18 … 0.57. Under the water pavilion the edge is
pulled out to x 14.3 between y −29.0 and −33.7 (its west half stands in the water).

**Edge rocks (湖石驳岸)** (`ChenghuaiRockery.cpp:178-215`): along the edge, a stone every 0.60 … 0.95 m: a broad lower
stone, R 0.42 (±20 %), from water − 0.35 to 0.00 … +0.06; on 55 % of them an upper stone R 0.30 from −0.03 to
+0.12 … +0.42; each a 7-sided outline stretched 1.3× along the bank, seeded. First pass: Taihu limestone prisms as
drawn; Nature's dissolution later.

**Paths (花街铺地)** (`ChenghuaiGarden.cpp:154-183`, centre lines in Appendix A): 0.95 wide: a pebble-mosaic bed 0.83
wide from −0.04 to 0.04 and two bluestone kerbs 0.06 wide from −0.06 to 0.055, swept along each centre line on the
ground. **GardenPath0 parts round 石兄** (B-10). Path lengths: 4.40, 4.89, 3.13, 1.50, 11.32, 5.80, 1.70.

**The zigzag bridge** (`ChenghuaiGarden.cpp:187-213`): points (5.2, −27.9) (7.2, −27.9) (8.2, −26.5) (10.4, −26.5)
(11.4, −27.5) (13.0, −27.5). Bluestone slabs 1.00 wide × 0.14 thick, top at **0.10**, each run carried 0.5 past its
turn points; piers 0.50 square at ≤ 1.6 m centres from 0.10 below the bed to the slab's underside (B-9 over the
tribune). No rail (Suzhou).

**The rockery (假山)** (`ChenghuaiRockery.cpp:22-172, 289-398`): a height field made into a sealed solid:
- Massing: 1.2 smoothstep(−0.2, 1.8, −d_foot) + 1.05 smoothstep(−0.4, 1.3, −d_upper) + 0.5 smoothstep(0, 1.4, 3.2 −
  r_pavilion), with d the signed distance to the foot and upper outlines (Appendix A) and r_pavilion the distance to
  the hexagonal pavilion's centre (10.6, −11.3).
- Stones: a jittered Voronoi of cells 0.95 m; each stone lifted (hash − 0.45) × 0.55, leaning up to ±0.15 per metre,
  rounded off to a crevice 0.16 deep within 0.24 of its edge; settling into the ground at the foot.
- The summit plateau at **2.60** within r 2.05 of the pavilion's centre, blending to r 2.60.
- The path (Appendix A): 0.55 half-width, cut as a ledge whose height is 2.6 × min(up, down) quantised to 0.15 risers
  (+0.02); the stone beside it at least 0.20 higher, or falling away. The path's west branch climbs 2.6 in 3.87 m (the
  short ascent, §12), the east branch descends over 14.95 m.
- Taihu hollows (up to 110, R 0.26 … 0.52) and standing peaks (up to 6, 1.3 … 2.1 m): polish.
- Sampled on a 0.10 grid over x 4.6 … 16.2, y −19.8 … −2.7, z −0.25 … 4.9. Area of the foot 157.7 m², of the upper
  level 60.6 m².

**The hexagonal pavilion (亭 · 見山)** (`ChenghuaiRockery.cpp:401-558`), centre (10.6, −11.3), corners at 60° k from
east: platform hexagon R 1.70 from 2.30 to **2.75** (floor); six chestnut columns at R 1.45, r 0.09 → 0.0855, 2.35 high,
on bases 0.28 square; lintels 0.12 × 0.20 centred at floor + 2.25; step-brocade fretwork 0.28 high from floor + 1.87
(**not on the two open faces**, B-6); on the four closed faces a seat (plank 0.36 × 0.05 at floor + 0.44 on a base
0.30 × 0.40, 0.18 inside the face line) with a goose-neck back (美人靠): balusters 0.035 square about every 0.13, each
rising 0.42 from floor + 0.46 and leaning out 0.02 + 0.20 sin(1.9 t); a back rail 0.07 × 0.05 at floor + 0.89, 0.19
outside. The open faces are k 3 (west-north-west, where the path arrives) and k 0 (east-south-east, where it leaves).
Roof: eave corners at R 2.35; z = base + (apex − base)(1 − f)^1.35 + 0.45 c³ f², base = floor + 2.70 = 5.45, apex
6.85, f the fraction out along the face's normal, c the corner factor |sector| / 30°; a chestnut soffit 0.14 under
the bed; 合瓦 rows between the hips; hips `(−0.08,−0.06) (0.08,−0.06) (0.08,0.08) (0.05,0.12)* (0,0.13)* (−0.05,0.12)*
(−0.08,0.08)` rising 0.12 t⁶ at the corners; corner beams 0.10 × 0.16; the finial (宝顶) turned from `(0,0) (0.2,0)
(0.2,0.08) (0.13,0.14)* (0.17,0.30)* (0.12,0.45)* (0.05,0.58)* (0,0.62)` from 6.73.

**The covered walk (复廊 · 遊目)** along the house's east wall (building table): its roof's back meets the limewashed
wall face at x 2.3; the twelve engraved stones (书条石, `ChenghuaiGarden.cpp:296-321`) set in that face, bluestone
0.90 × 0.32, 0.03 proud (x 2.29 … 2.33), z 1.17 … 1.49, two to a bay: for bay marks B = −36.25 (B-11), −25.6, −22.9,
−20.2, −17.5, −12.1, the stones run y B + 0.35 … B + 1.25 and B + 1.55 … B + 2.45 (read north to south). Their
incised text: polish.

**The flower hall (花厅 · 林泉)** and its **terrace (月台)**: the terrace x 8.1 … 13.2, y −36.7 … −38.2, top 0.30,
bluestone; steps §3.1. The flower hall's walls limewashed lime.

**The water pavilion (水榭 · 知魚)**: building table; bluestone platform, its west part a base standing in the pond.

### 3.9 Fittings drawn with the building

- **Lanterns (纱灯):** 17 plain hexagonal paper lanterns (§4). Each: a brass rod from its hook; a black-lacquer top plate
  (R + 0.018, 0.025) and crown (R − 0.05, 0.02); six posts 0.02 square; mulberry-paper panels 4 mm between the posts,
  with a rail 0.012 × 0.008 across their middle; a base plate and foot like the top. No characters, no tassels. None
  at, on or over the gate or the 垂花门 (the owner's rule). `ChenghuaiDisplay.cpp:360-407`.
- **Plaques and couplet** (Names board; `ChenghuaiHouse.cpp:658-772`): black-lacquer boards 25 mm on the wall, gilt
  characters read right to left. Boards: 臨池 1.0 × 0.5 on the front row's east end wall at (−2.327, −2.95, 2.45)
  facing west; 天青 1.1 × 0.55 at (−18.1, −14.541, 2.90) facing north; 昌南 at (−0.3, −14.541, 2.90) facing north;
  清閟 0.8 × 0.4 at (−14.725, −30.05, 3.05) facing west; 停雲 at (−3.675, −30.05, 3.05) facing east; 舒卷 1.1 × 0.55 at
  (−9.2, −44.359, 2.42) facing south; 林泉 1.2 × 0.6 at (10.65, −42.059, 2.95) facing south; 澄懷堂 2.0 × 0.64 on a
  painted lintel (金枋) between the rear 金柱 (x −10.975 … −7.425, y −33.36 ± 0.1, z 4.67 … 4.97), the board's face at
  y −33.234, centre z 4.84, facing south. The couplet (抱柱聯) wraps the hall's two middle veranda columns (x −11.15 and
  −7.25, y −27.0), 1.9 rad centred facing south, 22 mm lacquer shells, z 2.05 … 3.45: first line 行到水窮處 on the right
  (east). The Names board's 臥遊, 知魚, 見山 and 遊目 are not placed: the owner's rule keeps names off doors and gates
  (as built). First pass: the boards only, no characters (§12).
- **Guards:** the as-built's invisible collision guards (pond, bridge, rockery path, cases, pavilion) are not built.
  Collision matches the visible solids (Form.CollisionMatchesVisual); the edges are held by the Visitor's ledge rule
  (§10, and §12 risk).

### 3.10 Tolerances and stock

- Chord tolerance: Form's one number. Lattice bars and tiles are real members; identical tiles and bars are instanced.
- Brick stock: grey brick 0.288 × 0.144 × 0.064 (the Beijing 城砖 is larger: 0.48 × 0.24 × 0.12 on edge); floor tiles
  尺七 0.54 square, 尺四 0.45 square; paving bricks 0.40 square (old CH_PAVING repeat 2.1 m over about 5 × 5).
  (Matter's stock rules for the new brick substance; the sizes are Qing standard, to be written with sources.)
- Timber: columns and beams whole members; boards (望板) ≤ 0.30 wide; lattice bars 18 × 24 mm.

### 3.11 Materials: every key

Keys are **substance.working.history**. Records that exist today in Data/Matter (slot r-matter) are marked ●; all
others are new records for tonight, each with its family and, for an accent, the core family it ties to (D-014).

| Key | Where |
|---|---|
| `fangshan-marble.bush-hammered.foot-traffic-wear` | 青白石 (Fangshan, Dashiwo): platform kerbs, step treads and side stones, thresholds, the moon-gate sill (accent of stone; working ● `stone.bush-hammered`, history ● `stone.foot-traffic-wear`) |
| `fangshan-marble.bush-hammered.dissolution-outdoors` | column bases and drums, corner and footing stones, 门墩, the screen wall's plinth, the porch copings, bed and tree-pit kerbs, gable corner stones (history ● `stone.dissolution-outdoors`) |
| `grey-brick.rubbed-dry-laid.weathered` | 干摆: gate hall walls, screen wall body and cornice, every wall base (下碱), sill walls, 墀头 corbels, sealed-eave cornices, coping corbels, the moon ring, leak-window bars and rims, porch walls, gable bands (博缝), ridge bodies |
| `grey-brick.hairline-jointed.weathered` | 丝缝: main hall, side halls and ear rooms walls above the base |
| `grey-brick.lime-pointed.weathered` | 淌白: front and rear rows' walls, house outer walls, cross walls, platform bodies |
| `grey-brick.oil-soaked.foot-traffic-wear` | 尺七方砖 · 钻生 (0.54 m, tung-oil soaked): main hall and ear rooms floors |
| `grey-brick.ground-jointed.foot-traffic-wear` | 尺四方砖 · 细墁 (0.45 m): side halls, front and rear rows, galleries, gate hall, 垂花门 floors |
| `grey-brick.ground-jointed.weathered` | the courts' square paving (moss in the joints) |
| `grey-brick.laid-on-edge.foot-traffic-wear` | 城砖 on edge: the courts' cross paths, the porch |
| `grey-clay-tile.reduction-fired.lichen-weathered` | every roof tile (筒瓦 pans and covers, 合瓦), coping tiles, ridge caps |
| `lime-plaster.trowelled.soiling` | the roofs' lime bed where it shows (mitres, ends) (● `lime-plaster`, ● `trowelled`, ● `white.soiling`) |
| `lime-plaster.limewashed.soiling` | 粉墙: garden walls, the court wall's body, the house east wall's garden face, the flower hall (● `limewashed`) |
| `lime-plaster.whiting-washed.micro-cracking` | 麻刀灰 · 大白: the rooms' 15 mm skins inside (● `white.micro-cracking`) |
| `bluestone.bush-hammered.foot-traffic-wear` | 青石: garden platforms, floors and steps, the bridge's slabs, path kerbs, the terrace, the pavilion's platform and bases |
| `bluestone.sawn.submerged` | the bridge's piers, the water pavilion's base in the pond |
| `bluestone.honed.weathered` | the twelve 书条石 |
| `bluestone.natural.lichen-weathered` | the rockery's body (Materials board: "rockery body" in 青石) |
| `taihu-limestone.natural.lichen-weathered` | edge rocks; later the rockery's peaks, 石兄 and the Lingbi rock's kin |
| `river-pebble.set-on-edge.foot-traffic-wear` | 花街铺地 path beds |
| `puddled-clay.natural.submerged` | the pond's bank and bed (a Suzhou pond is clay-lined inside its stone edge) |
| `korean-pine.iron-red-lacquered.crazed` | 红松 under 铁红 on 一麻五灰: columns, unpainted lintels and beams, frames, door leaves, lattice, windows, fretwork, gable-opening frames, cross-wall doors (accent of oak) |
| `korean-pine.suzhou-painted.crazed` | 苏式彩画 (azurite, malachite, gold leaf on the ground): painted lintels, eave boards, beams and purlins |
| `korean-pine.green-lacquered.crazed` | the 屏门 panels, the yard door's leaves, the 垂花门's cantilever beams |
| `korean-pine.black-lacquered.crazed` | the gate's leaves, plaque and couplet boards, lantern frames, case toes |
| `chinese-fir.iron-red-lacquered.smoke-darkened` | 杉木: purlins in unpainted buildings, rafters, flying rafters, boarding, eave boards (accent of oak) |
| `chinese-fir.chestnut-lacquered.weathered` | 栗壳色 广漆: all garden timber (walk, flower hall, water pavilion, pavilion) |
| `nanmu.waxed.handled` | 楠木 (plantation Phoebe): case plinths, ends, canopies, mullions, stands, the 花罩 screens (accent of oak) |
| `huanghuali.waxed.handled` | the flower hall's painting table and chairs (collection: "huanghuali-type hardwood") |
| `brass.cast.tarnish` | knocker plates and rings, lotus buds (gilt later), lantern rods (● `brass`, accent of bronze; ● `bronze.tarnish`) |
| `brass.drawn.tarnish` | the cases' hanging rails and lamp rails |
| `low-iron-glass.anti-reflective.cleaning-wear` | the cases' laminated glass (● `low-iron-glass`, ● `anti-reflective`, ● `glass.cleaning-wear`) |
| `mulberry-paper.glass-laminated.soiling` | 桑皮纸 夹层: every lattice's glazing, paper in low-iron glass with a UV-filtering interlayer (infill of the Glazed openings; accent of white; transmission recorded in Matter) |
| `mulberry-paper.oiled.soiling` | the lanterns' panels (shaded by Light's diffuser master) |
| `silk.plain-woven.light-faded` | the cases' backs and beds (a warm grey 绫) (accent of white) |
| `water.still.pond` | the pond and the fish bowl (Nature's water body) |

Evidence to calibrate against (tier c, old photographed sets): `windows/Scripts/chenghuai_pbr.py:1-60` (Bricks061,
PavingStones112, white_plaster_02, Rock042L, Rock046L, rosewood_veneer_02, Ground037, a crazing relief) and the old
tuned colours in `windows/Scripts/chenghuai.py:200-311` (Beijing grey brick ≈ #7A7C79, lime #E3DFD5, iron red
#64251F, nanmu #74532E, tiles #4D4F4D).

---

## 4. Light

### Daylight openings

- **Glazed, paper infill** (`mulberry-paper.glass-laminated.soiling`, UV-filtering): every lattice panel of the
  fronts, transoms and closed leaves: 臨池 (faces north), 天青 (faces east), 昌南 (faces west), 澄懷堂 (faces south,
  under a 1.3 m veranda and a 1.1 m eave), 清閟 and 停雲 (south, behind the hall's gables), 舒卷 (south), 林泉 (south).
  Light enters as diffuse transmission through paper; direct sun arrives as glow on the paper, not as a beam (Materials
  board).
- **Open:** the open pairs of leaves in each DoorsOpen bay (unglazed); the courts' tops; the porch and the garden
  (Outdoors); the leak windows.
- No blinds (D-014). Care is met by the hang and the paper's UV-filtering interlayer.

### Luminaires (catalogue TYPE, count, position)

| Type | Count | Control class | Where (local) |
|---|---|---|---|
| `case-linear.3000k` (**new record**: a linear LED vitrine luminaire, UV/IR filtered, baffled) | 19 runs | gallery | Under each case canopy (upright cases: at v 0.05 … 0.10 from the glass), and a second low run behind the deck lip in every case taller than 2 m inside; under the back lamp rail of each slanted case. §8 lists the 13 cases |
| `track-wallwasher.3000k` (exists) | 75 heads on 11 black tracks | gallery | Tracks below: 臨池 south (x −19.6 … −2.9, y −4.95, z 3.02, aimed south and down): 14; 臨池 north (same x, y −0.85, aimed north): 7; 天青 (x −16.65, y −23.3 … −15.1, z 3.17, aimed west): 7; 昌南 (x −1.75, aimed east): 7; 澄懷堂 (x −13.7 … −4.7, y −28.7, z 4.45, aimed north): 8; 清閟 south (x −16.9 … −15.0, y −29.25, z 3.45): 2; 清閟 east (x −15.0, y −29.7 … −32.8, z 3.70, aimed west): 3; 停雲 south (x −3.4 … −1.5): 2; 停雲 west (x −3.4, aimed east): 3; 舒卷 (x −19.6 … 1.2, y −40.1, z 3.02, aimed north): 17; 林泉 (x 7.5 … 13.8, y −38.9, z 3.25, aimed north): 5. Counts are the design pitch (≈ 1.2 m); Light.Scheme confirms them |
| `track-spot.3000k.12deg` (exists) | 1 | gallery | On the 天青 track at (−16.65, −19.2, 3.17), aimed at the Ru basin on its pedestal |
| `lantern.2200k` (**new record**: an LED filament lamp in the hexagonal paper lantern housing of §3.9; the paper is Light's diffuser over `mulberry-paper`) | 17 | lantern (dusk switching) | Hook top (x, y, z); rod; lantern R × H: hall veranda (−10.2, −27.15, 4.90) and (−8.2, −27.15, 4.90), rod 0.86, 0.17 × 0.36; galleries (−13.0, −12.45, 3.20) and (−5.4, −12.45, 3.20), rod 0.30, 0.12 × 0.26; side verandas (−15.75, −19.2, 3.60) and (−2.65, −19.2, 3.60), rod 0.47, 0.15 × 0.32; 臨池 doors (−18.708, −5.58, 3.10) and (−3.792, −5.58, 3.10), rod 0.52, 0.13 × 0.26; 舒卷 doors (−18.629, −39.42, 3.10) and (0.229, −39.42, 3.10), rod 0.52, 0.13 × 0.26; walk (2.925, y, 2.75) for y −9.4, −17.5, −25.6, −33.7, rod 0.12, 0.11 × 0.24 (B-5); water pavilion (14.7, −31.35, 3.20), rod 0.25, 0.17 × 0.36; flower hall (9.9, −38.4, 3.31) and (11.4, −38.4, 3.31), rod 0.35, 0.17 × 0.36 |

Tracks are the luminaire's own housing product (painted aluminium), fixed under the lintels, not a Matter piece.
The old code's lumens, `LampScale`, `WindowScale`, window "glow" lights and both exposure volumes
(`ChenghuaiStructure.cpp:203-241, 448-486`; `chenghuai.py:312-349` paper emission) are **not** re-expressed: levels
come from Light's scheme and commissioning, and the room's hush from real transmission.

### Targets (by use and conservation)

| Surface | Class | Target |
|---|---|---|
| Silk: Guo Xi, Fan Kuan, Li Tang (hanging); Mi Fu, Qingming, Thousand Li (hand) | Care `textiles.silk` | ≤ 50 lx on the silk, 2800–3000 K, UV ≤ 75 µW/lm |
| Paper: Orchid Pavilion, Yan Zhenqing, Huai Su, Su Shi (hand); Ni Zan, Wang Meng, Shen Zhou, Wen Zhengming (hanging) | Care `works-on-paper` | ≤ 50 lx |
| Ceramics in 天青 and 昌南 | Care `ceramic-and-glass` | 200 lx on the case decks (Rooms board) |
| 林泉's painting table | Rooms board | 150 lx |
| The rooms' floors (walking zones) | **needs a low-light gallery use class** (§12) | 50 lx maintained |
| Courts, galleries, porch, garden paths, outdoor steps after dusk | `exterior-walkway` | 5 lx |
| Courts and garden by day | daylight | not held (daylight) |

### Design targets (luminance order, measured, not knobs)

- **澄懷堂 by day:** the inside face of the paper lattice > Fan Kuan's silk ≥ Guo Xi's and Li Tang's silk > the plastered
  walls > the floor > the rafters and boarding. Each scroll's silk at least 2× the walls behind the screens.
- **The paper rooms by day (臨池, 舒卷, 清閟, 停雲):** the court seen through the open door bays > the paper > the works'
  beds > the walls. The works are the brightest reflecting surfaces in the room.
- **Every room at night:** the works (case light) > the walls (washers) ≥ one fifth of the works.
- **Ceramic halls:** the decks' white porcelain the brightest reflecting surface; the Ru basin's pedestal a step
  brighter than the wall case.
- **Courts at night:** lantern paper > the paving in the lanterns' pools > the moonlit roofs.
- **The garden by day:** sunlit lime walls (粉墙) are the brightest diffuse surfaces; the pond darker than every wall.

### Transport

Diffuse transmission through the paper glazing must be carried by Light's transport (translucency). The old build
faked it with an emissive glow (now forbidden, Matter.NoEmission). If the renderer can't carry it, that is a renderer
limit, fixed museum-wide in Light's transport or documented; never a glow (§12).

---

## 5. Time and weather

- **Weather place:** every Glazed opening (the paper lattices) and every Court top takes **`museum`** weather. No remote
  weather (D-005 names only the Salon and Albion).
- **Rain and snow** fall into the courts, the porch and the garden (Nature's precipitation through Court tops and
  Outdoors), stop under the roofs, verandas and galleries; the pond takes ripples; Matter's surface law wets the
  paving, the tiles and the lime (sky exposure from Form).
- **Sun:** the museum's one sun as Time declares it (D-003: idealised by default). The design's reading (Materials
  board, at 40° N): summer noon 73.5° stays out of the rooms under the eaves; winter noon 26.6° reaches the paper and
  arrives as glow. Checks run at Time's reference moments under the declared model.
- **Seasons** come from the solar terms: the courts' trees and the pond's lotus follow Nature's phenology (§7).

---

## 6. Motion

**No mechanisms tonight.** Every leaf is a fixed piece in its as-built pose: the gate's leaves open against the side
panels; the 屏门 shut; the rooms' open pairs swung 90° in; the yard door's and cross-wall doors' leaves folded open
against the reveals. None is in a walked path's travel, so Motion has nothing to guard.

Later (polish): the 屏门 opens on the festival solar terms (Rules board: "It opens only for weddings, funerals and
festivals"): a pair of revolute mechanisms, 90°, 0.3 rad/s, interlocked with nothing, its travel inside the 垂花门.

---

## 7. Nature

**Water bodies**
- `chenghuai.pond`: the basin is the pond's bank and bed (§3.8), level **−0.28**, area 106.6 m², depth 0.18 … 0.57,
  still (no flow). Optics from the old pond: absorption (0.06, 0.02, 0.06) and scattering (0.006, 0.014, 0.008) per
  metre, a green-brown Suzhou pond (`chenghuai.py:407-427`: evidence, not a knob).
- `chenghuai.fish-bowl`: the glazed fish bowl on the inner court's crossing (§8, court things), water at 0.58.

**Plants** (positions local; crown radii as built). The boards name crabapple before the main hall, pomegranates in
tubs, jujubes in the rear court, bamboo and lotus; the old build stood in other species for some. Grow the real ones:

| Plant | Species | Position (x, y) | Height / crown |
|---|---|---|---|
| Crabapple ×2 (before the hall) | *Malus × micromalus* (西府海棠) | (−12.2, −22.6), (−6.2, −22.6) | 4.6 / 1.7 × 1.2 |
| South beds ×2 | *Osmanthus fragrans* as built (not hardy in Beijing; §12) | (−12.2, −15.8), (−6.2, −15.8) | 3.6 / 1.3 × 1.2 |
| Jujube ×2 (rear court pits) | *Ziziphus jujuba* | (−16.8, −35.3), (−1.6, −35.3) | 5.2 / 1.9 × 1.3 |
| Pomegranate ×2 in tubs | *Punica granatum* | (−11.6, −25.6), (−6.8, −25.6), in the tubs of §8 | 1.9 / 0.75 × 0.55 |
| Garden trees ×6 | the board's pine and willow (the as-built stood in osmanthus and plum): willow *Salix babylonica* at (6.0, −23.1) and (5.1, −20.9) by the water; pine *Pinus bungeana* at (13.4, −15.9) on the rockery; plum *Prunus mume* at (15.1, −37.0) and (14.7, −20.7); crabapple *Malus × micromalus* at (6.8, −5.3) | as listed | 3.4 … 4.8 / 1.2 … 2.1 |
| Bamboo ×3 clumps | *Phyllostachys aureosulcata* | culm points in `chenghuai_works.py:454-463` (y + 17.0): east by the pond (x ≈ 15.8, y −23.3 … −28.1); north-west and north-east behind the flower hall (y −42.8 … −44.1) | Nature's growth |
| Lotus ×6 | *Nelumbo nucifera* | (7.6, −33.9) Ø 0.9, (8.6, −32.8) 0.7, (6.8, −32.3) 0.8, (11.0, −24.5) 0.9, (9.8, −23.9) 0.7, (11.8, −25.5) 0.6; flowers on the first and fourth | Nature |
| Bank plants | *Acorus calamus* and *Iris sanguinea* in clumps between the edge rocks, not at the bridge's ends, the water pavilion or the terrace | as `chenghuai_works.py:511-541` | polish |

**Ground:** the garden's earth under moss (ground cover `moss` over `garden-soil`), the inner court's beds and the tree
pits in bare earth; growth spaces from Form's free space in each bed, shrunk by sway.

**Rock:** Taihu limestone by dissolution (Nature's Rock): 石兄, the rockery's peaks and hollows, the Lingbi rock: polish.

**Precipitation:** through Court tops and Outdoors, as §5.

---

## 8. Art

### The hang rule (Hang board: "cases cut to the works"; `chenghuai_works.py:292-353, 647-728`)

- **Hanging scrolls** hang by their cords from a brass rail 0.05 under the case canopy, 0.05 in front of the silk back;
  the top stick's middle 0.085 under the rail. Mount (装裱): heaven (天头) and earth (地头) per work; two bands (隔水)
  0.06; borders (镶边) 0.04; a roller at the foot with knobs past the mount. The roller must clear the deck by ≥ 0.01.
- **Handscrolls** lie on a slanted case's silk bed (11°), each opened from its beginning at the reader's right,
  equally spaced along the bed; each shows its lead (and wrapper, if any) to the right and its rolled remainder to the
  left. If the spans overrun the bed, the lead shortens, never the work.
- **Ceramics** stand on carved nanmu stands (radius 1.25 × foot, 0.028 high for pieces under 0.20, else 0.04) on the
  case deck.

### Works on paper and silk: 14, sizes are the works' own (image, h × w)

| Work (key) | Size | Ground | Room, case | Mount / placement |
|---|---|---|---|---|
| `chinese-orchid-pavilion` (Feng Chengsu after Wang Xizhi) | 24.5 × 69.9 cm | paper | 臨池, slant case, 1st from west | lead 0.06, no wrapper, roll r 0.025, band 0.025, border 0.012 |
| `chinese-yan-zhenqing-nephew` | 28.2 × 75.5 | paper | 臨池, 2nd | as above |
| `chinese-huaisu-autobiography` | 28.3 × 755 | paper | 臨池, 3rd | as above |
| `chinese-su-shi-cold-food` | 34.2 × 199.5 | paper | 臨池, 4th | as above |
| `chinese-mi-fu-shu-su` | 27.8 × 270.8 | silk | 臨池, 5th | as above |
| `chinese-thousand-li` (Wang Ximeng) | 51.5 × 1191.5 | silk | 舒卷, west case; begins at its east end | lead 0.45, wrapper 0.22, roll 0.035, band 0.03 |
| `chinese-qingming` (Zhang Zeduan) | 24.8 × 528.7 | silk | 舒卷, east case; begins at its east end | lead **0.23** (B-12), wrapper 0.22, roll 0.03, band 0.03 |
| `chinese-guo-xi-early-spring` | 158.3 × 108.1 | silk | 澄懷堂, west case | heaven 0.55, earth 0.30 (mounted length 2.553) |
| `chinese-fan-kuan-travelers` | 206.3 × 103.3 | silk | 澄懷堂, centre case (中堂) | heaven 0.62, earth 0.36 (3.163) |
| `chinese-li-tang-wind-in-pines` | 188.7 × 139.8 | silk | 澄懷堂, east case | heaven 0.60, earth 0.32 (2.927) |
| `chinese-ni-zan-rongxi` | 74.7 × 35.5 | paper | 清閟, back case | heaven 0.36, earth 0.20 |
| `chinese-wang-meng-qingbian` | 140.6 × 42.2 | paper | 清閟, side case (west gable) | heaven 0.42, earth 0.22 |
| `chinese-shen-zhou-lushan` | 193.8 × 98.1 | paper | 停雲, back case | heaven 0.42, earth 0.22 (2.698) |
| `chinese-wen-zhengming-living-aloft` | 95.3 × 45.7 | paper | 停雲, side case (east gable) | heaven 0.40, earth 0.22 |

Handscroll spacing check (catalogue widths): 臨池's five span 13.707 + 5 × 0.173 = 14.572 on a 15.05 bed, leaving
0.08 gaps; the Thousand Li spans 12.90 on 13.25; the Qingming 6.047 on 6.05 with the shortened lead.

Images: `assets/paintings/<key>.jpg` (the Thousand Li and Qingming in four tiles each, Huai Su in two), colour-managed
as the paint layer's albedo (Art.ColorManaged). Credits: `assets/art_sources_extra.json`, `assets/collection.json`.

### Ceramics: 13 works (+ the handling copy)

Deck of the side-hall cases: z 1.282. Items stand on the case's centre line, x −19.9 (天青) and x 1.5 (昌南).

| Work | Size (catalogue) | Object | Position y | Stand |
|---|---|---|---|---|
| `chinese-ding-basin` | 11.5 × 24.8 cm | lathe from `ch_vessels.json` "ding" (profile h 0.1256: rescaled to 0.115, B-13) + glaze image `T_ch_v_ding` | 天青 −22.6 | yes |
| `chinese-jun-flowerpot` | Ø 20.3 | "jun" | −21.6 | yes |
| `chinese-sancai-horse` | c. 70 cm high | scan (`usd/sculptures/chinese-sancai-horse.usdc`), turned 90° | −19.2 | none |
| `chinese-guan-vase` | h 34 | "guan" (0.354 → 0.340) | −17.0 | yes |
| `chinese-jian-bowl` | 7.3 × 12.1 | "jian" | −16.2 | yes |
| `chinese-longquan-vase` | h 14.6 | "longquan" (0.154 → 0.146) | −15.5 | yes |
| `chinese-ru-basin` | 23 × 16.4 cm (mouth; height per the NPM record) | scan (ChineseWing export), alone on the pedestal case at (−18.2, −19.2), deck 1.412, turned 90° | — | none |
| `chinese-meiping` | h 44.1 | scan | 昌南 −22.8 | r 0.06 |
| `chinese-xuande-jar` | h 48.3 | "xuande" (0.506 → 0.483) | −21.7 | yes |
| `chinese-wucai-jar` | h 23.2 | "wucai" | −19.9 | yes |
| `chinese-chicken-cup` | Ø 8.3 | scan | −18.5 | r 0.03 |
| `chinese-peachbloom` | h 15.9 | scan | −16.9 | r 0.045 |
| `chinese-falangcai-bowl` | 5.4 × 11.4 | "falangcai" | −15.8 | yes |
| Chicken cup, handling copy | as the cup | scan | on 昌南's table at (−0.28, −19.08, 1.25) | none |

Scans: the sancai horse's USD is in `usd/sculptures/`; the Ru basin, meiping, chicken cup and peachbloom exist only as
prims of `usd/wings/ChineseWing.usdc` (and the old Content). Studio imports them to `Evidence/Art` with provenance.

### Building works and furniture works (captions; objects §12)

| Work | Where | Size |
|---|---|---|
| `chenghuai-yingbi` 澄懷觀道 | the screen wall's heart | 1.8 × 1.8 m carved brick |
| `chenghuai-shutiao` 书条石 | the walk's wall | 12 stones, 0.9 × 0.32 |
| `chenghuai-shixiong` 石兄 | (5.3, −37.3), 3 m inside the moon gate on its axis | c. 3 m |
| `chenghuai-altar-table` 翘头案 | 澄懷堂 before Fan Kuan: x −10.2 … −8.2, y −32.54 … −32.96 | 2.0 × 0.42 × 0.82, everted ends |
| `chenghuai-scholars-desk` | 臨池's east bay: table x −4.25 … −2.75, y −1.9 … −2.65, h 0.82; a chair at (−3.5, −1.55) facing north | nanmu; inkstone, brush pot, paper |
| `chenghuai-painting-table` + chairs | 林泉: table centre (10.65, −40.6), 1.9 × 0.8 × 0.8; chairs at (10.15, −41.35) and (11.15, −41.35) facing south | huanghuali |
| `chenghuai-lingbi-rock` | on the painting table's east end, (11.35, −40.45, 1.28), on a stand r 0.11 | 0.32 high |

Also the court's things (Rules board: 天棚 鱼缸 石榴树): the fish bowl (lathe "fishbowl": Ø 1.17, h 0.668, green glaze
over brown) at (−9.2, −19.2) on the court's crossing, water at 0.58; two tubs (lathe "tub": Ø 0.63, h 0.46, dark
glaze) at (−11.6, −25.6) and (−6.8, −25.6) with soil at 0.405 and the pomegranates in them. Solid (the visitor walks
round them). The 天棚 (summer reed awning over the inner court): polish.

### Cases: 13 (Art's case objects; `ChenghuaiDisplay.cpp:63-204, 510-597`)

| Case | Kind | Extent (local) | Depth | Deck / canopy / top | Notes |
|---|---|---|---|---|---|
| 臨池 | slanted | x −19.9 … −4.7, front y −1.3, back to the wall (−0.5) | 0.80 | floor 0.30 | reader on the north |
| 舒卷 Thousand Li | slanted | x −19.7 … −6.3, front y −43.6, back −44.4 | 0.80 | floor 0.30 | reader on the south |
| 舒卷 Qingming | slanted | x −5.3 … 0.9 | 0.80 | floor 0.30 | |
| 天青 wall case | upright | y −23.4 … −15.0, glass at x −19.6, back to −20.2 | 0.60 | 1.27 / 2.77 / 3.03 | mullions at y −20.85, −17.55; no rail |
| 昌南 wall case | upright | mirror: glass at x 1.2 | 0.60 | same | |
| 澄懷堂 ×3 | upright, full height | x −13.55 … −11.85, −10.0 … −8.4, −6.7 … −4.7; glass at y −33.8, back −34.4 | 0.60 | 0.95 / 4.31 / 4.61 | brass rails; low lamp run |
| 清閟 back | upright | x −16.5 … −15.4, glass y −32.95, back −33.4 | 0.45 | 0.70 / 3.58 / 3.86 | rail; low run |
| 清閟 side | upright | y −32.6 … −31.4, glass x −16.75, back −17.2 | 0.45 | same | |
| 停雲 back | upright | x −3.2 … −1.7 | 0.45 | same | |
| 停雲 side | upright | y −32.6 … −31.4, glass x −1.65, back −1.2 | 0.45 | same | |
| 天青 pedestal | free-standing | centre (−18.2, −19.2), 0.80 square | — | pedestal 0.95, glass hood 0.50 | the Ru basin alone, opposite the door |

Slanted case: black toe 0.09; nanmu body to floor + 0.84 with panels every ≈ 1.5 m; base moulding and cornice; the silk
bed tilted 11° from floor + 0.905 at v 0.05 to v D − 0.06; the glass front upright to floor + 1.10, its top sloping at
11° to the back; nanmu ends 0.035; a brass lamp rail at the back under the glass hiding the lamp run. Upright case:
black toe 0.08; nanmu plinth to the deck with mouldings and a conditioning drawer; silk deck 12 mm and back 25 mm on a
10 mm board; ends 0.045; mullions 0.05; a canopy with a cornice 0.02 proud and a baffle 0.05 hiding the lamp run;
glass 10 mm at v 0.012; in cases taller than 2 m a nanmu lip 0.048 at the deck's front hiding the low run. The case's
own solids collide (no invisible guard).

### Fittings

Seats in the pavilion (§3.8) are Art bench anchors of the building's own drawing (four seat frames, §10). No
wayfinding signs: "A literati house names itself; it is not labelled" (Names board).

---

## 9. Sound

- **Rooms (Enclosed):** RT60 by Sabine or Eyring from each room's volume and Matter's absorption: lime plaster, paper
  lattice (partly open), tile floors, timber, silk-lined cases. Small, dry rooms (臨池 ≈ 17.9 × 4.8 × 3.5; 澄懷堂 ≈
  10.0 × 6.1, open to the rafters at 7.7). Occluded from the courts by walls; coupled through the open door bays and
  the paper.
- **Courts:** open tops, no tail: early reflections from brick walls and paving only.
- **The porch and the garden (Outdoors):** free field, with the garden walls' reflections.
- **Weather ambience** on Form's exposed surfaces by substance: rain on clay tile roofs (the house's dominant sound in
  rain), on brick paving, on the pond's water, on the bamboo; wind in the bamboo and the willow.
- **Footfalls** by substance: grey-brick tile floors, 青白石 kerbs and steps, bluestone slabs, pebble mosaic, earth.
- **Emitters:** none tonight. Later: a recorded Suzhou garden at its measured level.

---

## 10. Visitor

**The loop (16 stops, Plan board), one door in, one door out:**
1. 大门: through the porch and up the gate's five steps.
2. 影壁 · 澄懷觀道: turn west through the green 屏门.
3. 臨池: enter at its west door bay, walk east along the 15 m case, the desk in the east bay; out by the east bay.
4. 垂花门 · 齋莊中正: the screen stays shut; go round by the galleries.
5. 天青: through the veranda and the middle bay; the Ru basin alone opposite the door.
6. 澄懷堂: up the hall's steps; Guo Xi, Fan Kuan, Li Tang, one per bay.
7. 清閟 (west ear room) through the hall's west gable door.
8. 停雲 (east ear room) through the east gable door.
9. 昌南: porcelain; the handling copy on the table.
10. 后院: through the north-east cross-wall door; two jujubes, a pause.
11. 舒卷: enter at the east bay, walk west, reading right to left; out by the west bay.
12. 月洞门 · 臥遊: into the garden; 石兄 framed as you step through.
13. 林泉: the studio on the pond (terrace and flower hall).
14. 知魚: the water pavilion (paths 1 and 2) and the zigzag bridge.
15. 見山: up the rockery; the Rotunda's dome is the southern mountain (借景).
16. 遊目: back along the walk past the engraved stones, through the garden door into the entry yard, out by the gate.

**Uses:** sit on the pavilion's four seats (IMuseeUsable seat frames, facing inward, seat at floor + 0.44); look at
every work (captions from Art, IMuseeCaptioned) including the building works; the hour key (the courts show the sun
move). The chicken cup's "the one you may hold" needs a hold intent the Visitor does not have: polish.

**Prompts:** none on the building (no signs). Captions only.

**The ledge rule:** the body does not walk off a drop taller than MaxStepHeight (0.22): the pond's edge, the bridge's
sides, the rockery path's drops, the platforms' kerbs. It uses the steps. This replaces the as-built invisible guards
and keeps collision equal to what is seen (§12).

---

## 11. The old code map

All paths under `C:\Users\anan9\Desktop\Projects\Musee Vision\` (read-only). Re-express, don't copy: add 17.0 to every y.

| Part | Where |
|---|---|
| Plan numbers: site, porch, gate, yard, front row, court wall, 垂花门, inner court, side halls, main hall, ear rooms, galleries, rear court and row, garden buildings, bridge, pond level, tribune | `windows/Source/MuseeVision/Chenghuai/ChenghuaiPlan.h:27-137` |
| Garden outlines (pond, rockery foot and upper, rockery path, seven paths) | `ChenghuaiGardenPlan.h:12-105` |
| The part (material) list | `ChenghuaiGeometry.h:15-57` |
| Hall spec vocabulary (FHallSpec, bays, ridges, gable openings, steps) | `ChenghuaiHall.h:32-114` |
| Hall proportions (rafters, boarding, bed, kerbs, base course, plaster, sill, door head) | `ChenghuaiHall.cpp:18-33` |
| The section: seats, bed curve, eave | `ChenghuaiHall.cpp:38-192` |
| Column with base and drum | `ChenghuaiHall.cpp:220-233` |
| Beams, struts, wall prisms | `ChenghuaiHall.cpp:236-273` |
| Lattice panel (grid, lantern, step brocade, ice-crack) | `ChenghuaiHall.cpp:278-392` |
| Door leaf (隔扇) | `ChenghuaiHall.cpp:394-418` |
| Platform, steps | `ChenghuaiHall.cpp:424-483` |
| Columns, frame (purlins, lintels, beams across) | `ChenghuaiHall.cpp:485-635` |
| Rafters and flying rafters | `ChenghuaiHall.cpp:638-690` |
| Roof slab, tiles, ridge, scorpion tails, rolled ridge, gable edges | `ChenghuaiHall.cpp:693-909` |
| Gable walls and corbelled ends | `ChenghuaiHall.cpp:912-989` |
| Back walls and sealed-eave cornice | `ChenghuaiHall.cpp:991-1021` |
| Fronts: frames, doors, transoms, windows | `ChenghuaiHall.cpp:1024-1119` |
| Tile styles and shapes, rows | `ChenghuaiRoof.h:61-99`, `ChenghuaiRoof.cpp:22-76, 78-292` |
| Building specs: front row, gate, side hall, main hall, ear room, rear row, galleries G1-G4 | `ChenghuaiHouse.cpp:60-264` |
| The gate's joinery | `ChenghuaiHouse.cpp:268-344` |
| 垂花门 | `ChenghuaiHouse.cpp:348-438` |
| 影壁 | `ChenghuaiHouse.cpp:442-463` |
| House walls, court wall, yard wall, cross walls, porch walls | `ChenghuaiHouse.cpp:467-531` |
| Courts' paving, paths, beds, tree pits | `ChenghuaiHouse.cpp:535-576` |
| Gallery floors, steps, columns; fretwork | `ChenghuaiHouse.cpp:580-649` |
| Plaques and couplet | `ChenghuaiHouse.cpp:658-772` |
| Assembly order | `ChenghuaiHouse.cpp:776-802` |
| Leak-window shapes and lattices, panel with hole, moon outline | `ChenghuaiParts.cpp:16-178` |
| Paving, kerbs, tiled coping, free-standing wall with holes | `ChenghuaiParts.cpp:182-340`, `ChenghuaiParts.h:16-63` |
| Garden levels and bed depth | `ChenghuaiGarden.cpp:27-47` |
| Garden walls | `ChenghuaiGarden.cpp:58-75` |
| Ground, pond bank and bed, edge rocks, pond guards (not re-expressed) | `ChenghuaiGarden.cpp:79-149` |
| Paths | `ChenghuaiGarden.cpp:154-183` |
| Zigzag bridge | `ChenghuaiGarden.cpp:187-213` |
| Walk, flower hall, water pavilion specs; terrace | `ChenghuaiGarden.cpp:218-288` |
| Engraved stones | `ChenghuaiGarden.cpp:296-321` |
| Rockery: constants, path height, envelope, stones, height | `ChenghuaiRockery.cpp:20-176` |
| Edge rocks | `ChenghuaiRockery.cpp:178-215` |
| Taihu hollows and peaks; surface nets | `ChenghuaiRockery.cpp:217-399` |
| Hexagonal pavilion | `ChenghuaiRockery.cpp:401-558` |
| Rockery guards (not re-expressed) | `ChenghuaiRockery.cpp:561-581` |
| Pond outline and water surface | `ChenghuaiWater.cpp:9-47` |
| Cases: slanted, upright, pedestal | `ChenghuaiDisplay.cpp:63-204` |
| Table, chair, desk things, open screen (花罩) | `ChenghuaiDisplay.cpp:209-348` |
| Lanterns and their positions | `ChenghuaiDisplay.cpp:360-443` |
| Wall-washer tracks and their positions | `ChenghuaiDisplay.cpp:456-508` |
| Rooms' furnishing and cases' positions | `ChenghuaiDisplay.cpp:510-597` |
| Scroll mount builder (hanging and hand) | `ChenghuaiScroll.h:10-108`, `ChenghuaiScroll.cpp:37-274` |
| Vessel turning | `ChenghuaiVessel.h:10-63`, `ChenghuaiVessel.cpp:9-109` |
| Painted-timber and plaque atlases (evidence for polish) | `ChenghuaiPaint.h:7-75`, `windows/Scripts/chenghuai_textures.py:254-527` |
| **Not to port:** exposure volumes, per-lamp lumens, scales, daylight "glow" | `ChenghuaiStructure.cpp:203-241, 448-486`; `ChenghuaiStructure.h:95-146` |
| Material evidence (old tuned values, photographed sets) | `windows/Scripts/chenghuai.py:155-311, 407-456`; `chenghuai_pbr.py:1-60` |
| Works: sizes, mounts, hang, ceramics placement, court things, plants | `windows/Scripts/chenghuai_works.py:55-69, 195-215, 292-353, 358-374, 440-582, 605-633, 647-728` |
| Vessel profiles and the court's wares | `windows/Scripts/chenghuai_vessels.py:26-36, 270-281`; `windows/SourceArt/Textures/ch_vessels.json` |
| The Rotunda's north door and the Classical Hall beneath | `windows/Source/MuseeVision/Plan/MuseePlan.h:44-47, 462-509` |

---

## 12. First-pass cut

### Built tonight

1. **Site:** the stage frame, the Building footprint (with its height range and the Classical Hall reservation), the one
   port `chenghuai.south` mated to `rotunda.north`, the 15 enclosures and their openings.
2. **The Qing hall composition** once, instanced as: front row, gate hall, two side halls, main hall, two ear rooms, rear
   row, eight galleries, the 垂花门's two roofs, the covered walk, the flower hall, the water pavilion. Platforms and
   steps (Blondel), columns, frames, rafters, roof slabs, tile rows (with lips, cut at ends, valleys and hips), ridges
   and scorpion tails, gable walls with 墀头, back walls with cornices, fronts with doors, windows, transoms and paper
   lattice (all four patterns).
3. **The gate's joinery** (plain pins, leaves, knockers, drums as blocks), the 垂花门 (posts, buds, lattice band, frame,
   shut 屏门 with plain discs), the 影壁 (body, plinth, cornice, coping, plain heart), every wall with its openings,
   copings, leak windows with lattice, the moon gate, the porch.
4. **Courts:** paving cut round the platforms, paths, beds, tree pits.
5. **Garden:** ground, pond basin and water, edge rocks (prisms), paths with kerbs, the bridge, the rockery mass
   (height field with the path's ledge, no hollows or peaks), the hexagonal pavilion with seats, the terrace.
6. **Matter:** every key of §3.11; the new substance, working and history records at the evidence the systems accept
   tonight.
7. **Light:** paper Glazed openings and Open tops; 19 case runs, 75 washer heads on 11 tracks, 1 spot, 17 lanterns;
   targets; commissioning.
8. **Art:** the 14 scrolls in their mounts, the 13 cases, the 8 turned vessels and 5 scans with their stands, the fish
   bowl and tubs; captions for every work including the building works.
9. **Nature:** the pond and fish-bowl water; the trees (12), the three bamboo clumps, the lotus, the pomegranates; moss
   and earth; precipitation.
10. **Visitor:** walking the loop with the ledge rule; sitting on the pavilion's seats.

### Waits for polish

- Carving and inscription: plaques' gilt characters, the couplet, the 影壁 heart, the 书条石, the 屏门 discs, the 门墩
  lions, the apron panels' carving (Form's Ornament from the calligraphy images; no flat character texture stands in).
- The Suzhou painting's 包袱 scenes and rafter-end painting (first pass: the pigments' plain bands by the rule).
- Drip and end tiles (滴水, 勾头).
- Taihu dissolution: 石兄, the rockery's hollows and peaks, the Lingbi rock.
- Furniture works: the altar table, the desk and its things, the painting table and chairs, 昌南's table and chairs,
  the two 花罩 screens (their captions are in tonight).
- The 天棚, the 太平缸 fire vats, the rear court's stone benches, the bank plants, the seasonal flowers.
- The 屏门's festival opening (Motion); a hold intent for the chicken cup; recorded sound.

### Open for the Architect (these block checks tonight)

1. **The cylindrical port with no wall on Chenghuai's side.** Site's joint and the class `doorway.4000x4500.arched`
   must define the collar on a cylindrical face (Albion's side is flat with wedges). The porch's walls were moved out
   0.1 m each side so they no longer stand inside the door's clear width (B-1).
2. **The Rotunda's outer dress** at the north door must stop at the collar so the porch meets the drum.
3. **An Open opening shared by two enclosures** of one stage (Albion's item 3).
4. **A low-light gallery use class** in `UseTargets.json`: `circulation` (100 lx, walls 75 lx) would outshine 50-lx silk
   and break the luminance order.
5. **The ledge rule** is a Visitor capability; without it the pond's bed is a trap (bank slope 62°, Site.Reachable).
6. **Footprints by volume:** the Classical Hall lies under Chenghuai; Site.NoOverlap must judge volumes, not plans.

### Risks (watch them in tonight's checks)

- **Light through paper.** The rooms' daylight is diffuse transmission through the paper glazing. The old build faked
  it with an emissive glow because the renderer didn't carry it. If Lumen still doesn't, the rooms go dark: that is a
  renderer limit for Light's transport (museum-wide) or a documented limit, never a glow or an exposure volume.
- **Sun on silk.** Low winter sun through the open middle bay of 澄懷堂 can reach 5 m in, near the altar table and the
  case bases; through 舒卷's open end bays it can reach the Thousand Li's west end. Art.WithinCare will say. The fix is
  in the hang or in which pair stands open, not in a blind (D-014).
- **Annual dose.** Silk's care class allows 15,000 lx·h a year; at 50 lx in an always-open museum that is 300 hours.
  The board's answer is rotation. Annual dose is tier 2 (D-014), so it is not checked tonight, but presence-linked
  case light (D-006) must dim the cases when the rooms are empty.
- **Night on the garden paths.** The lanterns light the verandas, galleries, doors, the walk, the water pavilion and
  the flower hall. The rockery path, the bridge, the courts' middles and the porch have no luminaire (no lantern at the
  gate, by the owner's rule), so `exterior-walkway` (5 lx) will fail there after dusk except by moonlight.
- **Flat courts.** As built the courts are level; after rain Matter's law over Form's ponding field will stand water on
  all of them. Real courts fall about 1 % to drains. First pass keeps them level; the owner decides.
- **The rockery's short ascent** climbs 2.6 in 3.87 m (0.15 risers on 0.22 treads). It is Nature's ground with steps,
  not a Form stair, and the risers are under MaxStepHeight; the long branch is gentle. If Form.StepsFitTheBody is
  applied to it, it fails.
- **Osmanthus** in the south beds is not hardy in Beijing (grown in tubs and brought in for the winter); Nature's botany
  may flag it. The garden's willow, pine and plum replace the old stand-ins (the old code says the board had pine and
  willow).
- **New Matter records.** Every substance except lime plaster, low-iron glass and brass is new (§3.11). Matter.OneLanguage
  fails until each is written with its family and accent tie; there is no oak in this room at all.
- **Scans.** Four of the five ceramic scans exist only as prims inside `usd/wings/ChineseWing.usdc` (the retired wing's
  export). Studio must import them to `Evidence/Art` with provenance.
- **The borrowed view.** From 見山 the Rotunda's dome is "the southern mountain". The Rotunda stays resident (the open
  joint), but the Salon's and the Hall of Light's roofs, seen over the walls, may not be in Site's visibility graph.
- **Tiles.** About 1,000 m² of roof and coping: some 50,000 tiles if each is its own piece (合瓦 about 47 a square
  metre, 筒瓦 about 33). The old build swept each row as one piece with its course lips; either way, identical courses
  must be instanced for Nanite.

---

## Appendix A. The garden's outlines (local metres)

From `ChenghuaiGardenPlan.h:12-105` (sampled from the Plan board's Bezier paths at 18.6 px/m), y + 17.0. Paths are
centre lines, 0.95 wide. They go in `Private/ChenghuaiGardenPlan.h`.

**pond.edge (the water pavilion notch applied: the edge pulled to x 14.3 between y -29.0 and -33.7)**, 75 points, x y:

```
 14.199 -35.500   13.896 -35.745   13.369 -35.943   12.712 -36.098   12.023 -36.216   11.398 -36.301
 10.833 -36.352   10.261 -36.364    9.692 -36.341    9.136 -36.285    8.602 -36.199    8.074 -36.088
  7.549 -35.950    7.047 -35.778    6.590 -35.564    6.199 -35.301    5.871 -34.974    5.590 -34.589
  5.363 -34.168    5.198 -33.731    5.102 -33.301    5.116 -32.877    5.239 -32.445    5.404 -32.005
  5.547 -31.557    5.602 -31.102    5.534 -30.640    5.390 -30.173    5.220 -29.697    5.073 -29.207
  5.000 -28.699    4.997 -28.159    5.030 -27.590    5.105 -27.011    5.225 -26.441    5.398 -25.898
  5.622 -25.370    5.895 -24.842    6.216 -24.339    6.585 -23.883    7.000 -23.500    7.485 -23.185
  8.042 -22.923    8.635 -22.717    9.233 -22.575    9.801 -22.500   10.352 -22.499   10.911 -22.568
 11.454 -22.697   11.958 -22.878   12.398 -23.102   12.793 -23.378   13.160 -23.714   13.469 -24.092
 13.692 -24.493   13.801 -24.898   13.735 -25.315   13.516 -25.759   13.230 -26.214   12.963 -26.666
 12.801 -27.102   12.767 -27.474   12.804 -27.795   12.877 -28.130   12.954 -28.544   13.221 -29.102
 14.300 -29.882   14.300 -30.845   14.300 -31.867   14.300 -32.827   13.212 -33.602   13.227 -34.165
 13.585 -34.600   13.950 -34.943   14.197 -35.231
```

**rockery.foot**, 64 points, x y:

```
 15.699 -18.398   15.242 -18.817   14.544 -19.157   13.724 -19.392   12.898 -19.500   12.057 -19.405
 11.150 -19.144   10.242 -18.860    9.398 -18.699    8.622 -18.776    7.881 -18.987    7.200 -19.155
  6.602 -19.102    6.060 -18.743    5.575 -18.187    5.203 -17.538    5.000 -16.898    5.073 -16.278
  5.362 -15.626    5.671 -14.960    5.801 -14.301    5.627 -13.661    5.277 -13.026    4.939 -12.378
  4.801 -11.699    4.991 -10.969    5.382 -10.201    5.783  -9.432    6.000  -8.699    5.899  -8.018
  5.608  -7.364    5.338  -6.729    5.301  -6.102    5.562  -5.448    6.006  -4.782    6.572  -4.175
  7.199  -3.699    7.903  -3.377    8.706  -3.168    9.555  -3.050   10.398  -3.000   11.268  -2.996
 12.174  -3.056   13.043  -3.213   13.801  -3.500   14.455  -3.948   15.037  -4.532   15.501  -5.199
 15.801  -5.898   15.847  -6.629   15.692  -7.413   15.491  -8.241   15.398  -9.102   15.488 -10.029
 15.664 -11.014   15.832 -11.993   15.898 -12.898   15.802 -13.717   15.606 -14.481   15.406 -15.203
 15.301 -15.898   15.406 -16.592   15.639 -17.269   15.802 -17.885
```

**rockery.upper**, 64 points, x y:

```
 13.763 -15.704   13.479 -15.963   13.046 -16.172   12.538 -16.317   12.027 -16.382   11.505 -16.324
 10.941 -16.163   10.377 -15.987    9.855 -15.887    9.373 -15.934    8.914 -16.065    8.491 -16.169
  8.118 -16.134    7.785 -15.914    7.485 -15.571    7.254 -15.170    7.129 -14.774    7.173 -14.387
  7.352 -13.982    7.543 -13.570    7.624 -13.161    7.515 -12.764    7.298 -12.371    7.090 -11.970
  7.005 -11.548    7.124 -11.094    7.366 -10.618    7.614 -10.142    7.747  -9.688    7.685  -9.264
  7.503  -8.858    7.335  -8.463    7.312  -8.075    7.474  -7.670    7.750  -7.258    8.103  -6.882
  8.495  -6.586    8.931  -6.387    9.428  -6.258    9.955  -6.185   10.478  -6.156   11.015  -6.153
 11.577  -6.190   12.116  -6.287   12.586  -6.462   12.990  -6.740   13.351  -7.102   13.639  -7.516
 13.823  -7.952   13.853  -8.405   13.757  -8.891   13.633  -9.403   13.575  -9.935   13.630 -10.511
 13.739 -11.123   13.844 -11.730   13.887 -12.290   13.827 -12.799   13.704 -13.273   13.579 -13.721
 13.516 -14.151   13.580 -14.581   13.724 -15.000   13.826 -15.383
```

**rockery.path (centre line, west end first)**, 55 points, x y:

```
  5.602 -14.102    5.826 -14.017    6.142 -13.902    6.513 -13.764    6.904 -13.613    7.279 -13.456
  7.602 -13.301    7.867 -13.146    8.104 -12.982    8.325 -12.813    8.541 -12.642    8.762 -12.470
  9.000 -12.301    9.250 -12.141    9.504 -11.989    9.763 -11.837   10.030 -11.677   10.309 -11.501
 10.602 -11.301   10.920 -11.077   11.266 -10.840   11.624 -10.587   11.977 -10.314   12.309 -10.019
 12.602  -9.699   12.882  -9.345   13.171  -8.959   13.438  -8.550   13.652  -8.131   13.783  -7.711
 13.801  -7.301   13.690  -6.875   13.474  -6.419   13.175  -5.962   12.815  -5.536   12.416  -5.171
 12.000  -4.898   11.541  -4.715   11.015  -4.597   10.451  -4.538    9.875  -4.538    9.316  -4.592
  8.801  -4.699    8.312  -4.884    7.823  -5.152    7.351  -5.474    6.912  -5.825    6.522  -6.176
  6.199  -6.500    5.953  -6.822    5.770  -7.168    5.637  -7.513    5.540  -7.834    5.465  -8.105
  5.398  -8.301
```

**path.moon-gate-to-terrace**, 11 points, x y:

```
  3.699 -37.452    3.998 -37.452    4.420 -37.452    4.913 -37.452    5.423 -37.452    5.898 -37.452
  6.374 -37.452    6.885 -37.452    7.378 -37.452    7.801 -37.452    8.102 -37.452
```

**path.terrace-to-water-pavilion**, 16 points, x y:

```
 13.199 -37.500   13.402 -37.404   13.694 -37.276   14.025 -37.116   14.345 -36.924   14.602 -36.699
 14.799 -36.430   14.970 -36.114   15.112 -35.773   15.223 -35.428   15.301 -35.102   15.329 -34.772
 15.312 -34.422   15.270 -34.088   15.225 -33.804   15.199 -33.602
```

**path.water-pavilion-to-bridge**, 16 points, x y:

```
 14.801 -29.102   14.830 -28.959   14.881 -28.754   14.926 -28.520   14.941 -28.291   14.898 -28.102
 14.785 -27.946   14.617 -27.803   14.416 -27.679   14.203 -27.576   14.000 -27.500   13.790 -27.461
 13.559 -27.456   13.333 -27.471   13.138 -27.490   13.000 -27.500
```

**path.bridge-to-walk**, 6 points, x y:

```
  5.199 -27.898    4.972 -27.898    4.637 -27.898    4.265 -27.898    3.928 -27.898    3.699 -27.898
```

**path.walk-to-pond-south**, 21 points, x y:

```
  3.699 -18.301    4.055 -18.579    4.551 -18.982    5.139 -19.436    5.770 -19.867    6.398 -20.199
  7.046 -20.418    7.747 -20.571    8.464 -20.684    9.161 -20.784    9.801 -20.898   10.391 -21.016
 10.955 -21.118   11.484 -21.221   11.968 -21.343   12.398 -21.500   12.777 -21.721   13.112 -21.995
 13.397 -22.278   13.628 -22.527   13.801 -22.699
```

**path.walk-to-rockery-west**, 16 points, x y:

```
  3.699 -14.301    3.956 -14.284    4.319 -14.265    4.743 -14.235    5.185 -14.184    5.602 -14.102
  6.005 -13.984    6.423 -13.835    6.839 -13.665    7.238 -13.484    7.602 -13.301    7.942 -13.097
  8.271 -12.867    8.571 -12.638    8.820 -12.440    9.000 -12.301
```

**path.rockery-east-to-walk**, 6 points, x y:

```
  3.699  -8.398    3.958  -8.383    4.340  -8.361    4.761  -8.337    5.141  -8.316    5.398  -8.301
```

---

## Appendix B. Changes from the as-built

Each is either the approved board over the code (D-001) or the fix for a physical truth a new check guards.

| # | Change | Why | Old code |
|---|---|---|---|
| B-1 | The porch's walls stand at x ±2.0 … ±2.6 (as built ±1.9 … ±2.5); walls and paving stop at the joint's collar instead of running 8 cm into the drum | The as-built porch (3.8 wide) stood 0.1 inside the 4.0 door's clear width; the joint now owns the seam | `ChenghuaiHouse.cpp:524-530`, `ChenghuaiPlan.h:37-39` |
| B-2 | Every flight by Blondel (§3.1). The gate: 5 risers of 0.180, treads 0.275 (the Plan comment and board); the main hall: 4 of 0.1825 | The old step routine gave 6 × 0.15 on 0.22 treads (2R + T = 0.52); the board's "three to the 正房" would need 0.25 risers, over MaxStepHeight 0.22 | `ChenghuaiHall.cpp:455-483`, `ChenghuaiPlan.h:40-42` |
| B-3 | The four door pins are plain | Names board: "The gate carries no words ... four plain door pins" | `ChenghuaiHouse.cpp:306-314` |
| B-4 | The yard door's leaves are green | Plan board: "Turn west through the green 屏门" | `ChenghuaiHouse.cpp:509-514`, `ChenghuaiParts.cpp:256-275` |
| B-5 | The covered walk's columns are 2.30 (as built 2.10); its lantern hooks at 2.75 | Its lintel stood 1.88 over the walk's floor and its lanterns hung to 1.95: under MinHeadroom 2.0. Now 2.08 and 2.15 | `ChenghuaiGarden.cpp:227`, `ChenghuaiDisplay.cpp:435-438` |
| B-6 | No fretwork on the hexagonal pavilion's two open faces | It hung to 1.87 over the floor where the path passes | `ChenghuaiRockery.cpp:445` |
| B-7 | Two risers of 0.165 at each of the water pavilion's path ends | The paths met a 0.33 platform edge with no step (over MaxStepHeight) | `ChenghuaiGarden.cpp:260-278` |
| B-8 | The gate's sill (门槛) is 0.20 (as built 0.25) | Over MaxStepHeight 0.22 | `ChenghuaiHouse.cpp:300` |
| B-9 | Bridge piers within r 7.9 of the tribune's centre stop on the shallow bed, never below −0.50; the pond's and bridge's invisible guards are not built | As built every pier went to −0.95, into the Classical Hall's roof (−0.55) | `ChenghuaiGarden.cpp:199-211, 129-148` |
| B-10 | GardenPath0 parts round 石兄: it forks at x 4.3 into two branches passing 0.4 clear of the rock's base and rejoins at x 6.3 | The rock (5.3, −37.3) stands on the path's centre line (y −37.452) | `chenghuai_works.py:466`, `ChenghuaiGardenPlan.h:69-72` |
| B-11 | The first bay of engraved stones starts 0.15 further south (B −36.25) | The northmost stone overlapped the moon gate's brick ring (outer edge y −35.97) by 8 cm | `ChenghuaiGarden.cpp:298` |
| B-12 | The Qingming's lead is 0.23 (as built 0.25); handscroll widths are the catalogue's, not the images' aspect | With the catalogue's 528.7 cm the mount overran its 6.2 m case by 1.7 cm (Art.TrueSize and Art.HangResolves) | `chenghuai_works.py:218-223, 689-690` |
| B-13 | Turned vessels are scaled to the catalogue's height: ding 0.1256 → 0.115, guan 0.354 → 0.340, xuande 0.506 → 0.483, longquan 0.154 → 0.146 | The profiles were measured from photographs (Art.TrueSize) | `ch_vessels.json`, `chenghuai_vessels.py:26-36` |
| B-14 | No invisible guards anywhere (pond, bridge, rockery path, cases, pavilion seats); the Visitor's ledge rule holds the edges | Form.CollisionMatchesVisual | `ChenghuaiStructure.cpp:196-201`; guards throughout |
| B-15 | The courts' paving is cut round the platforms (as built it ran under them) | Form.NoOverlap | `ChenghuaiHouse.cpp:553-575` |
| B-16 | No exposure volumes, per-lamp lumens, scales or emissive paper | Light and the Eye own these; Matter.NoEmission | `ChenghuaiStructure.cpp:203-241, 448-486`; `chenghuai.py:312-349, 351-405` |

