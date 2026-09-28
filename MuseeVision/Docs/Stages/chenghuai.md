# 澄懷 Chenghuai · design sheet

- **Stage key** `chenghuai` (module `MuseeChenghuai`, `UChenghuaiRoom : USiteRoom`). Display name "澄懷 Chenghuai". The
  approved proposal on the Rotunda's north door (D-001): a three-court Beijing house (三进四合院) with a Suzhou garden
  (臥遊) beside it to the east. Era: a Classical wing in Chinese materials: brick rubbed or jointed by rank, stone
  axe-dressed, timber lacquered and painted; the garden white-walled and chestnut-lacquered. One door, one loop of 16
  stops. The old Chinese Wing is retired and nothing is taken from it: its USD export is used only as evidence for four
  ceramic *works* the proposal names (§8).
- **Sources:** D-001…D-019 (D-001, D-003, D-006, D-012, D-014, D-017, D-018 binding here); the approved boards
  `plan/proposals/chenghuai/canvas/{Main,Plan,Rooms,Hang,Materials,Garden,Section,Rules,Names}.dc.html` (no README);
  Palette.json + Story.md §5, §6 "Chenghuai"; the drafter's spec `r-specs/…/Stages/chenghuai.md` (verified, corrections
  in §11); the finished `rotunda.md` (§2, `rotunda.north`) and `classical-hall.md` (the hall beneath); the as-built code
  `windows/Source/MuseeVision/Chenghuai/*` and `windows/Scripts/chenghuai*.py` (§11). Schedules:
  `Data/Stages/chenghuai/{Finishes,Lighting,Hang,Planting}.json` (no Mechanisms: nothing moves tonight).
- **Units** metres. **Frame:** origin on the gate axis (the Rotunda's north–south axis, x 0) on the site's south outer
  face (y 0), at the datum z 0 (the Rotunda's floor); x east, y south, z up, true north. **Old numbers convert** as
  x = x_old, **y = y_old + 17.000**, z = z_old (the old plan's origin was the Rotunda's centre). **Building frames**
  (u, v, z): §3.1. **φ** (the pavilion only) = plan angle from +x towards +y.

## 1. Identity and placement

| | |
|---|---|
| Origin in the museum plan | (0.000, −17.000, 0.000); yaw 0°. **museum = local + (0, −17.000, 0).** Site derives it from the mating `chenghuai.south ↔ rotunda.north` (§2). |
| Level | 0. Courts and garden ground at z 0.02, the porch at 0.00; the buildings on stepped platforms (台基) 0.15–0.90. Below the datum only footings (−0.10), paving beds (−0.12), the pond (bed −0.85) and the bridge piers. |
| Footprint kind | Building (house, garden, porch). |
| Vertical extent | z −0.95 (bridge piers outside the tribune zone) to 8.463 (the main hall's ridge top); trees to ≈ 10 (Nature's growth space, not the footprint). |
| Site | 37.6 × 44.9 = 1,688 m² (house x −20.7 … 2.3, garden 2.3 … 16.9; y 0 … −44.9), plus the porch. |

**Footprint polygon** (counter-clockwise on the north-up plan from the south-west corner): (−20.700, 0.000) →
(−2.950, 0.000) → (−2.950, 6.195) → arc R 11.200 about (0.000, 17.000) through (0.000, 5.800) → (2.950, 6.195) →
(2.950, 0.000) → (16.900, 0.000) → (16.900, −44.900) → (−20.700, −44.900). The porch's end is the Rotunda's drum face
(museum r 11.20): the two footprints touch only along that arc, |x| ≤ 2.95.

**Below: the Classical Hall** (a separate stage; roof top museum −0.55 = local −0.55). Its gallery shell x ±4.20 runs
from y 9.10 north to its tribune, a circle r 7.90 about **(0.000, −22.300)** (`classical-hall.md` §1). Chenghuai builds
nothing below z −0.50 inside x ±4.20 north of the drum or inside r 8.30 of (0, −22.30); only the pond and the bridge
come near it (§3.9). Site compares volumes, not plans.

**Neighbours:** the Salon's Manet cabinet (museum y −16.0, x −24.4 … −15.6) stands 1.00 south of the house's south
face; the slot between is the Grounds'. The Rotunda's exterior dress (polish) stops at the porch walls.

| Enclosure | Kind | Plan extent (local) and floor | Bounded by / notes |
|---|---|---|---|
| `chenghuai.porch` | Outdoors | x ±2.35, y 0.00 → the drum; 0.00 | Low walls to 1.10; port `chenghuai.south` at its south end |
| `chenghuai.gate-hall` | Enclosed | x ±1.80, y −0.30 … −5.20; 0.90 | Gables, roof; open south and north between the gables |
| `chenghuai.entry-yard` | Court | x −1.90 … 1.80, y −5.50 … −11.10; 0.02 | Gate hall, yard wall, screen wall/court wall, house east wall (≥ 3.20); top Open at 3.20 |
| `chenghuai.front-court` | Court | x −20.20 … −2.30, y −5.90 … −11.10; 0.02 | Front row, west wall, court wall, yard wall; top Open at 3.20 |
| `chenghuai.linchi` | Enclosed | x −20.20 … −2.30, y −0.50 … −5.30; 0.30 | 倒座房 · 臨池 |
| `chenghuai.inner-court` | Court | x −20.20 … 1.80, y −11.50 … −30.70; 0.02 (galleries 0.45 / 0.75) | Includes the 垂花门, galleries, verandas and corner passages; minus the rooms; top Open at 3.20 |
| `chenghuai.tianqing` | Enclosed | x −20.20 … −16.30, y −14.50 … −23.90; 0.45 | 西厢房 · 天青 |
| `chenghuai.changnan` | Enclosed | x −2.10 … 1.80, y −14.50 … −23.90; 0.45 | 东厢房 · 昌南 |
| `chenghuai.chenghuaitang` | Enclosed | x −14.20 … −4.20, y −28.30 … −34.40; 0.75 | 正房 · 澄懷堂, open to the rafters (7.75) |
| `chenghuai.qingbi` | Enclosed | x −17.20 … −14.70, y −28.90 … −33.40; 0.50 | 西耳房 · 清閟 |
| `chenghuai.tingyun` | Enclosed | x −3.70 … −1.20, y −28.90 … −33.40; 0.50 | 东耳房 · 停雲 |
| `chenghuai.rear-court` | Court | x −20.20 … 1.80, y −31.10 … −39.70; 0.02 | No art: a pause; top Open at 3.20 |
| `chenghuai.shujuan` | Enclosed | x −20.20 … 1.80, y −39.70 … −44.40; 0.30 | 后罩房 · 舒卷 |
| `chenghuai.garden` | Outdoors | x 2.30 … 16.40, y −0.50 … −44.40; 0.02 | 臥遊: walk, pond, rockery, pavilions, flower hall terrace |
| `chenghuai.linquan` | Enclosed | x 7.00 … 14.30, y −38.50 … −42.10; 0.45 | 花厅 · 林泉 |

**Openings.** Glazed: the paper lattices of every room (§4). Open: each Court's top (z 3.20); the open door pairs; the
leak windows (lattice, not passable). Port: `chenghuai.south`. **Openings between enclosures of this stage** are
declared **Open, shared by two enclosures** (Form must accept an Open that is not to the sky; if it cannot tonight,
each room merges into its court): gate hall front/back (3.60 wide to the eaves, at y −0.30 and −5.20); yard door (屏门,
x −2.30 … −1.90, y −7.50 … −9.30, 1.80 × 2.35); garden door (x 1.80 … 2.30, y −7.50 … −9.30, 1.80 × 2.35); 垂花门
(x −10.36 … −8.04, 2.32 clear, head 2.674, at y −11.30); cross-wall doors (x −19.50 … −18.40 and 0.00 … 1.10, y −30.70
… −31.10, 1.10 × 2.30); the moon gate (Ø 2.40, centre y −37.30, z 1.25, in the house east wall); the hall's gable doors
into the ear rooms (y −31.40 … −32.60, 2.30 above the hall floor); each room's open door bays (§3.6).

## 2. Ports

Frame on the port face at the threshold (z 0), on the opening's centreline, facing out of the stage, up +z. Class
names are **width × crown** (the Architect's ruling). **Convention checked:** `salon.east` faces +x and mates
`rotunda.west` facing −x; `classical-hall.stair` faces +y and mates `rotunda.stair` facing −y. Each port faces out of
its own stage, so mated frames coincide and face opposite ways.

| Key | Kind | Class | Local frame (position; facing) | Face; wall depth | Mates with | Wells |
|---|---|---|---|---|---|---|
| `chenghuai.south` | doorway | `doorway.4000x6500.arched` | (0.000, 5.800, 0.000); **+y** (south, to the Rotunda) | cylindrical R 11.200 about the vertical line (0, 17.000), convex towards Chenghuai (the drum bulges into the porch); **0.000** (the porch is open: no wall across its mouth) | `rotunda.north` (frame museum (0, −11.200, 0), facing −y; drum 1.20 deep). **Passage 0.00.** | none |

- **Class** `doorway.4000x6500.arched`: clear 4.000 (jambs x ±2.000); semicircular head r 2.000 springing 4.500; crown
  6.500 (`MuseePlan.h:44-47`). Open portal.
- **Museum check.** (0, 5.800, 0) + (0, −17.000, 0) = **(0.000, −11.200, 0.000)** = `rotunda.north`'s frame; facings +y
  and −y are opposite; the face is the drum's outer cylinder r 11.20 about the museum origin on both sides.
- **The porch against the 4.00 door (resolved without changing the Rotunda).** As built the porch was 3.80 inside
  (walls |x| 1.90 … 2.50): 0.10 of wall stood inside each jamb of the 4.00 door. The porch now leaves the drum face
  clear for **|x| < 2.35** from the paving up:
  - **Porch walls** `chenghuai.brick.rubbed` at **|x| 2.35 … 2.95** (0.60 thick), from y 0.00 (the site's south face,
    where they abut the front row's back wall and the garden's south wall) to the drum face, cut on the cylinder
    R 11.20 (inner corner y 6.049, outer y 6.195); z −0.10 … 0.98; coping `chenghuai.porch.coping` 0.98 … 1.10, 0.04
    proud each side, cut on the same cylinder.
  - **So the joint's surround fits:** the surround on the drum's outer face may be up to **0.35** wide beyond the clear
    jambs (the Rotunda's inner archivolt is 0.30), and runs down to the paving on both jambs; the walls stop beside it
    on the drum's own face. Nothing of Chenghuai touches the drum face within |x| < 2.35 except the paving's edge, and
    nothing touches it elsewhere except the walls' ends.
  - **Porch paving** `chenghuai.porch.paving`: |x| ≤ 2.35, from y 0.00 (the site face) to the mating arc R 11.20, cut
    round the gate's front steps and side stones (y 0 … 1.17, |x| ≤ 1.50), the platform's footing (to y 0.05) and the
    gables' ends (to y 0.06); top z 0.000 (the Rotunda's floor), 0.12 deep. The joint's threshold starts on that arc.
- **Site's joint** builds the travertine threshold through the drum's 1.20, the brushed-bronze strip on the mating arc
  (R 11.20, across the clear width), the linings, and the surround on the drum's outer face (the porch side). The
  Rotunda's own archivolt (rotunda.md §3.3) is the dressing on its inner face, and its port says so (`rotunda.north`
  marks its surround as its own), so the joint draws none there. There are no wedges (the faces are the same cylinder).
  Chenghuai builds no rough opening: its side has no wall.
- **Blinds.** Mated tonight: open portal, no leaves. On the stand alone (or with the Rotunda absent) a closed pair of
  cast bronze leaves fills the class opening on the port cylinder, meeting rail on the axis:
  `chenghuai.south.blind-doors`.
- **Topology (D-012):** the only port. A dead end by design ("the garden brings you back to the gate"); no well served
  or received. The visit is one loop inside the stage (§10).

### Anchors (the stage contract)
Wall anchors: origin on the hanging face at the left end as seen by the viewer, x to the viewer's right, y out of the
face towards the viewer, z up (for a slanted handscroll bed: up the slope). z of the origin is the room's floor, so
Hang `height` is above that floor (for a bed: up the slope from its front edge).

| Key | Kind | Frame (origin; x; y; z) | Belongs to |
|---|---|---|---|
| `chenghuai.chenghuaitang.case.west` | Wall | (−13.505, −34.365, 0.750); x (1,0,0); y (0,1,0) | Guo Xi's case back (silk face), x −13.505 … −11.895 |
| `chenghuai.chenghuaitang.case.centre` | Wall | (−9.955, −34.365, 0.750); x (1,0,0); y (0,1,0) | Fan Kuan's case, x −9.955 … −8.445 |
| `chenghuai.chenghuaitang.case.east` | Wall | (−6.655, −34.365, 0.750); x (1,0,0); y (0,1,0) | Li Tang's case, x −6.655 … −4.745 |
| `chenghuai.qingbi.case.back` | Wall | (−16.455, −33.365, 0.500); x (1,0,0); y (0,1,0) | Ni Zan |
| `chenghuai.qingbi.case.side` | Wall | (−17.165, −31.445, 0.500); x (0,−1,0); y (1,0,0) | Wang Meng (west gable) |
| `chenghuai.tingyun.case.back` | Wall | (−3.155, −33.365, 0.500); x (1,0,0); y (0,1,0) | Shen Zhou |
| `chenghuai.tingyun.case.side` | Wall | (−1.235, −32.555, 0.500); x (0,1,0); y (−1,0,0) | Wen Zhengming (east gable) |
| `chenghuai.linchi.case.bed` | Wall | (−4.735, −1.250, 1.215); x (−1,0,0); y (0,−0.1908,0.9816); z (0,0.9816,0.1908) | the 15.13 m slanted bed (the reader faces south) |
| `chenghuai.shujuan.case.thousand-li.bed` | Wall | (−19.665, −43.650, 1.215); x (1,0,0); y (0,0.1908,0.9816); z (0,−0.9816,0.1908) | the Thousand Li's bed (the reader faces north) |
| `chenghuai.shujuan.case.qingming.bed` | Wall | (−5.265, −43.650, 1.215); axes as above | the Qingming's bed |
| `chenghuai.entry-yard.yingbi` | Wall | (−0.900, −10.535, 0.020); x (1,0,0); y (0,1,0) | the screen wall's heart (影壁心) |
| `chenghuai.walk.stones` | Wall | (2.330, −9.650, 0.150); x (0,−1,0); y (1,0,0) | the twelve 书条石 in the walk's wall |
| `chenghuai.tianqing.case.deck` | Mount | (−19.900, −19.200, 1.282), the deck's centre line; x (0,1,0); works face +x | the Tang–Song ceramics |
| `chenghuai.changnan.case.deck` | Mount | (1.500, −19.200, 1.282); x (0,1,0); works face −x | the Yuan–Qing porcelain |
| `chenghuai.tianqing.pedestal` | Mount | (−18.200, −19.200, 1.412), deck top centre; faces +x (the door) | the Ru basin |
| `chenghuai.changnan.table` | Mount | (−0.200, −19.200, 1.250), table top centre | the chicken cup's handling copy |
| `chenghuai.garden.shixiong` | Mount | (5.300, −37.300, 0.020); faces −x (the moon gate) | 石兄 |
| `chenghuai.chenghuaitang.altar-table` | Mount | (−9.200, −32.750, 1.570), table top centre | the altar table (caption) |
| `chenghuai.linchi.desk` | Mount | (−3.500, −2.275, 1.120) | the scholar's desk (caption) |
| `chenghuai.linquan.table` | Mount | (10.650, −40.600, 1.250) | the painting table and chairs (caption) |
| `chenghuai.jianshan.seat.1`, `chenghuai.jianshan.seat.2`, `chenghuai.jianshan.seat.4`, `chenghuai.jianshan.seat.5` | Seat | seat top z 3.190 at (10.600, −10.224), (9.668, −10.762), (10.600, −12.376), (11.532, −11.838); each facing the pavilion's centre (10.600, −11.300) | the pavilion's four seats (faces k 1, 2, 4, 5) |
| `chenghuai.pond.water` | Water | (9.603, −29.668, −0.280): the water level at the outline's centroid | the pond |
| `chenghuai.fish-bowl.water` | Water | (−9.200, −19.200, 0.606): the bowl's water level at its axis | the court's fish bowl |
| `chenghuai.inner-court.tub.w`, `chenghuai.inner-court.tub.e` | Bed | soil z 0.425 in the tubs at (−11.600, −25.600), (−6.800, −25.600) | the pomegranates |
| `chenghuai.pond.lotus` | Bed | (9.603, −29.668, −0.280) | the lotus (§7) |
| `chenghuai.inner-court.bed.nw`, `chenghuai.inner-court.bed.ne`, `chenghuai.inner-court.bed.sw`, `chenghuai.inner-court.bed.se` | Bed | soil z 0.080 at (−12.200, −22.600), (−6.200, −22.600), (−12.200, −15.800), (−6.200, −15.800) | crabapples (north pair), osmanthus (south pair) |
| `chenghuai.rear-court.pit.w`, `chenghuai.rear-court.pit.e` | Bed | soil z 0.050 at (−16.800, −35.300), (−1.600, −35.300) | the jujubes |
| `chenghuai.garden.ground` | Bed | the garden's ground, z 0.020 (outline §3.9) | the ground and its moss |
| `chenghuai.garden.tree.willow-1`, `chenghuai.garden.tree.willow-2`, `chenghuai.garden.tree.pine`, `chenghuai.garden.tree.plum-1`, `chenghuai.garden.tree.plum-2`, `chenghuai.garden.tree.crabapple` | Bed | ground points (6.000, −23.100), (5.100, −20.900), (13.400, −15.900) on the rockery, (15.100, −37.000), (14.700, −20.700), (6.800, −5.300) | the garden's trees |
| `chenghuai.garden.bamboo.east`, `chenghuai.garden.bamboo.north-west`, `chenghuai.garden.bamboo.north-east` | Bed | centroids (15.810, −25.990), (7.640, −43.380), (13.580, −43.630) | the three bamboo clumps |

No Body or Use anchors; no channels. Luminaires mount at the points in Lighting.json.

## 3. Pieces by trade

### 3.1 The Qing hall: one composition, drawn fourteen times
Every building is the same Qing official-style carpentry (清式小式大木) on a stone-kerbed platform, made by one
composition from a spec in its own frame: **u** along the building's front, **v** from its front eave column line
back, z up (every frame is square to the plan).

**Spec fields** (`ChenghuaiHall.h:62-114`): frame origin and u, v; column axes along u; gable outer faces U0, U1 and
thickness; floor; platform extent; eave column height ColH and diameter D; purlin stations v and the rise per step;
rolled or not; tile kind; ridge kind; facade (lattice) line v; back wall v0 … v1 and sealed back eave (封护檐) or not;
interior column lines; eave reach front/back; bays' fill; painted or not; wall and base finishes; gable openings;
steps; mitred roof ends; open ends; roof extent; lattice pattern.

**The section rule** (`ChenghuaiHall.cpp:129-177`):
- Zc = floor + ColH. Purlin radius Pr = max(0.09, 0.45 D). Seat[0] (eave purlin top = rafter underside) = Zc + 0.12 +
  2 Pr; Seat[i+1] = Seat[i] + Rise[i] × (v[i+1] − v[i]); rises steepen to the ridge (举架 5, 7, 9 tenths).
- Tile bed (lime top) at a purlin = seat + 0.095 (rafter) + 0.020 (boarding) + 0.140 (lime and mud); +0.06 at a rolled
  ridge's middle. Between knots a monotone cubic (Fritsch–Carlson) through the eave edge, the round rafters' end, every
  purlin (and the rolled ridge's middle) and the back edge.
- Eave: round rafters to v = −0.66 × EaveFront; square flying rafters (飞椽) from 0.90 behind that to the edge
  v = −EaveFront, at 0.55 of the first step's slope. A sealed back continues the bed at 0.8 × the last step's slope to
  the back wall's outer face + EaveBack; an open back mirrors the front eave.

**Derived heights** (checked against the Section board: 正房 8.2, 大门 6.5, 厢房 5.5, 倒座 / 后罩 / 垂花门 5.2):

| Building | Zc | Pr | Seats (z) | Ridge bed | Ridge top |
|---|---|---|---|---|---|
| Front row | 2.800 | 0.117 | 3.154 3.785 4.694 3.785 3.154 | 4.949 | 5.409 |
| Gate hall | 3.850 | 0.135 | 4.240 4.853 5.734 4.853 4.240 | 5.989 | 6.449 |
| Side hall | 3.350 | 0.122 | 3.713 4.263 5.138 5.138 4.263 3.858 | 5.453 | 5.528 |
| Main hall | 4.650 | 0.144 | 5.058 5.708 6.601 7.748 6.623 5.741 5.096 | 8.003 | 8.463 |
| Ear room | 3.900 | 0.117 | 4.254 4.848 5.703 4.848 4.254 | 5.958 | 6.418 |
| Rear row | 2.800 | 0.117 | 3.154 3.773 4.664 3.773 3.154 | 4.919 | 5.379 |
| Gallery | 3.050 | 0.099 | 3.368 3.616 3.616 3.368 | 3.931 | 4.006 |
| 垂花门 front / back roof | 3.070 | 0.108 | 3.406 3.846 3.406 / 3.406 3.679 3.679 3.337 | 4.101 / 3.994 | 4.561 / 4.069 |
| Covered walk (ColH 2.30) | 2.450 | 0.090 | 2.750 3.313 | single slope | — |
| Flower hall | 3.450 | 0.108 | 3.786 4.361 4.717 4.717 4.361 3.786 | 5.032 | 5.107 |
| Water pavilion | 2.950 | 0.090 | 3.250 3.828 3.828 3.250 | 4.143 | 4.218 |

**The buildings** (local; "u; v" are the frame's directions):

| Building | Origin; u; v | u range (gables) | Columns (u) | Floor | ColH | D | Purlin v | Rise | Roof | Facade v | Back wall v | Eave F/B | Bays |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Front row 倒座房 · 臨池 | (−20.70, −5.30); E; S | 0 … 18.9 (W gable 0.5; E end open: the gate's gable) | 0.5 + k·17.9/6, k 0…6 | 0.30 | 2.50 | 0.26 | 0, 1.2625, 2.525, 3.7875, 5.05 | .5 .72 −.72 −.5 | 合瓦, 清水脊, sealed back; roof to u 18.45 | 0 | 4.8 … 5.3 | 0.75 / 0.28 | DoorsOpen, W, W, W, W, DoorsOpen |
| Gate hall 广亮大门 | (−2.30, −0.30); E; N | 0 … 4.6 (gables 0.5) | joinery §3.7 | 0.90 | 2.95 | 0.30 | 0, 1.225, 2.45, 3.675, 4.9 | .5 .72 −.72 −.5 | 筒瓦, 清水脊, open back | — | none | 0.90 / 0.90 | Open; gables run to v 5.36 |
| W side hall 西厢房 · 天青 | (−15.20, −24.40); S; W | 0 … 10.4 (gables 0.5) | 0.5, 3.55, 6.85, 9.9 | 0.45 | 2.90 | 0.27 | 0, 1.1, 2.35, 3.1, 4.35, 5.25 | .5 .7 0 −.7 −.45 | 合瓦, rolled 卷棚, sealed back | 1.1 | 5.0 … 5.5 | 0.85 / 0.30 | W, DoorsOpen, W |
| E side hall 东厢房 · 昌南 | the west hall mirrored about x −9.20 (x′ = −18.40 − x) | | | | | | | | | | | | |
| Main hall 正房 · 澄懷堂 | (−14.70, −27.00); E; N | 0 … 11.0 (gables 0.5) | 0.5, 3.55, 7.45, 10.5; 金柱 line v 6.36 | 0.75 | 3.90 | 0.32 | 0, 1.3, 2.575, 3.85, 5.1, 6.36, 7.65 | .5 .7 .9 −.9 −.7 −.5 | 筒瓦, 清水脊, sealed back | 1.3 | 7.4 … 7.9 | 1.10 / 0.32 | W, DoorsOpen, W |
| W ear room 西耳房 · 清閟 | (−17.70, −28.90); E; N | 0 … 3.0 (W gable 0.5; E end open) | 0.5, 3.0 | 0.50 | 3.40 | 0.26 | 0, 1.1875, 2.375, 3.5625, 4.75 | .5 .72 −.72 −.5 | 合瓦, 清水脊, sealed back; roof to u 3.05 | 0 | 4.5 … 5.0 | 0.80 / 0.25 | Doors (closed) |
| E ear room 东耳房 · 停雲 | the west ear room mirrored about x −9.20 | | | | | | | | | | | | |
| Rear row 后罩房 · 舒卷 | (−20.70, −39.70); E; N | 0 … 23.0 (gables 0.5) | 0.5 + k·22/7, k 0…7 | 0.30 | 2.50 | 0.26 | 0, 1.2375, 2.475, 3.7125, 4.95 | .5 .72 −.72 −.5 | 合瓦, 清水脊, sealed back | 0 | 4.7 … 5.2 | 0.75 / 0.28 | DoorsOpen, W, W, Doors, W, W, DoorsOpen |
| Flower hall 花厅 · 林泉 | (6.60, −38.50); E; N | 0 … 8.1 (gables 0.4) | 0.4, 2.4, 5.7, 7.7 | 0.45 | 3.00 | 0.24 | 0, 1.15, 1.625, 2.125, 2.6, 3.75 | .5 .75 0 −.75 −.5 | 合瓦, rolled, open back over a back wall | 0 | 3.6 … 3.9 | 0.80 / 0.70 | Doors (ice-crack), DoorsOpen (lantern), Doors |
| Water pavilion 水榭 · 知魚 | (13.30, −29.10); N; E | 0 … 4.5, roof −0.55 … 5.05; no gables | 0.3, 4.2, front and back lines | 0.35 | 2.60 | 0.20 | 0, 1.05, 1.75, 2.8 | .55 0 −.55 | 合瓦, rolled | — | none | 0.75 / 0.10 | Open |
| Covered walk 复廊 · 遊目 | (3.55, −5.50); N; W | 0 … 33.8, roof −0.3 … 34.1; no gables | 0.3 + 2.7k, k 0…12 (x 3.55 line only) | 0.15 | **2.30** | 0.20 | 0, 1.25 | .45 | 合瓦, single slope to the house wall (x 2.30) | — | the house's east wall | 0.55 / 0.06 | Open |

**Galleries** (抄手游廊): painted, rolled 合瓦, floor 0.45, ColH 2.60, D 0.22, purlins v 0, 0.45, 0.85, 1.30, rises .55 0
−.55, eaves 0.60 both, no gables, back columns; each has an east twin mirrored about x −9.20.

| Gallery | Origin; u; v | u range | Columns / frames | Roof | Platform |
|---|---|---|---|---|---|
| G1 south (by the court wall) | (−16.30, −12.90); E; S | 0 … 5.4 | columns u 0, 1.3, 3.4, 5.2 | mitred at u0 (slope −1 about v 0), roof u 1.3 … 5.45; back eave 0.12 into the court wall | u −0.1 … 5.4, v −0.1 … 1.4 |
| G2 west, south part | (−15.00, −11.60); N; W | 1.3 … 2.45 | frame u 2.3; columns at (−15.00, −13.90), (−16.30, −13.90) | mitred at u0 | u 1.4 … 2.4 |
| G3 west, north part | (−15.00, −24.40); N; W | −0.05 … 3.8 | frames u 0.1, 2.4 | mitred at u1 (slope +1), roof −0.05 … 2.5 | the gallery floors below |
| G4 turn into the hall's veranda | (−15.00, −26.90); E; N | 0 … 0.35 | none | mitred at u0 | the gallery floors below |

The side halls' verandas (1.1, between eave columns and lattice) run on through both gables as the galleries' middle
length (gable openings v 0.06 … 1.04, 2.30 high, framed). The main hall's gables open at the veranda ends (v 0.08 …
1.22, 2.45 high) and into the ear rooms (v 4.40 … 5.60, 2.30 high).

### 3.2 Platforms, steps, columns (stone `chenghuai.stone.kerb`, `chenghuai.stone.step`, `chenghuai.stone.base`)
- **Platforms (台基)** (`ChenghuaiHall.cpp:424-453`): brick body `chenghuai.brick.pointed` (`chenghuai.brick.rubbed` for
  the gate, main hall and 垂花门; `chenghuai.garden.bluestone` in the garden) from −0.10 to floor − 0.13; a footing
  course 0.05 proud all round, −0.10 … +0.03; kerb (阶条石) 0.34 wide × 0.13 round the top; corner stones (埋头) 0.30
  square at the front corners; floor tiles inside the kerb. A platform ≤ 0.21 high (the walk) has solid kerbs +0.03 to
  the floor. Extents (u × v): front row 0 … 18.4 × −0.6 … 5.3; gate 0 … 4.6 × −0.3 … 5.2; side halls 0 … 10.4 × −0.6 …
  5.5; main hall 0 … 11.0 × −0.7 … 7.9; ear rooms 0 … 3.0 × −0.6 … 5.0; rear row 0 … 23.0 × −0.6 … 5.2; flower hall
  −0.05 … 8.15 × −0.3 … 3.95; water pavilion −0.05 … 4.55 × −0.3 … 3.1 (its west 1.3 stands in the pond on a bluestone
  base from the bed); walk −0.1 … 33.9 × −0.15 … 1.28.
- **Gallery floors** (west; mirror east): platform x −16.40 … −14.90, y −24.35 … −26.00, top 0.45, kerb x −14.95 …
  −14.85; two risers of 0.15 (going 0.33) to y −26.33; upper platform x −16.40 … −14.70, y −26.33 … −28.35, top 0.75.
  Columns (red, bases 0.40 square) at (−15.00, −24.50) and (−16.30, −24.50) on 0.45; (−15.00, −26.80), (−16.30, −26.80),
  (−16.30, −28.20) on 0.75; and (−15.00, −13.90), (−16.30, −13.90) on 0.45 (G2's).
- **Steps (踏跺)**: Form stairs by Blondel (2R + T = 0.63), stone treads, sloping side stones (垂带) 0.30 wide each side;
  thresholds 0.10 wider than their openings each side, 0.06 over the ground, 0.06 past both faces.

| Flight | Where (local) | Width | Rise | Risers | R | T | Run |
|---|---|---|---|---|---|---|---|
| Gate, front | x ±1.20, y 1.100 → 0.000 (up north onto the gate platform) | 2.40 | 0.90 | 5 | 0.180 | 0.275 | 1.100 |
| Gate, back | x ±1.20, y −5.500 → −6.612 (down into the yard) | 2.40 | 0.88 | 5 | 0.176 | 0.278 | 1.112 |
| Front row ×2 | centred x −18.708, −3.792; y −5.90 → −6.25 | 1.50 | 0.28 | 2 | 0.140 | 0.350 | 0.350 |
| Side halls ×2 | centred y −19.20, from x −14.60 down east (mirror) | 2.00 | 0.43 | 3 | 0.143 | 0.343 | 0.687 |
| Main hall | centred x −9.20, y −26.300 → −25.505 | 3.00 | 0.73 | 4 | 0.1825 | 0.265 | 0.795 |
| Rear row ×3 | centred x −18.629, −9.200, 0.229; y −39.10 → −38.75 | 1.50 | 0.28 | 2 | 0.140 | 0.350 | 0.350 |
| 垂花门 | x −10.20 … −8.20, y −9.900 → −9.213 | 2.00 | 0.43 | 3 | 0.143 | 0.343 | 0.687 |
| Gallery to the hall veranda ×2 | x −16.40 … −14.95, y −26.00 → −26.33 | 1.45 | 0.30 | 2 | 0.150 | 0.330 | 0.330 |
| Gallery off to the corner passage ×2 | x −16.400 → −17.087, y −24.80 … −26.00 | 1.20 | 0.43 | 3 | 0.143 | 0.343 | 0.687 |
| Water pavilion path ends ×2 | x 14.60 … 15.80 at y −29.05 and −33.65 | 1.20 | 0.33 | 2 | 0.165 | 0.300 | 0.300 |
| Flower hall → terrace | x 9.90 … 11.40, at y −38.20 | 1.50 | 0.15 | 1 | 0.150 | — | — |
| Terrace → garden | x 9.90 … 11.40, y −36.70 → −36.35 | 1.50 | 0.28 | 2 | 0.140 | 0.350 | 0.350 |

- Sills: the gate's wooden sill (门槛) **0.20**; partition doors' 0.10; the 垂花门's 0.12; the moon gate's stone sill
  1.80 wide, 0.08 past both faces, −0.10 … 0.102.
- **Columns** (`ChenghuaiHall.cpp:220-233`): base stone (柱顶石) 1.9 D square, floor −0.08 … +0.012; a turned drum (鼓镜),
  (r, z): (0, 0) (0.68D, 0) (0.68D, 0.012) (0.62D, 0.045)* (0.55D, 0.055) (0, 0.055) (* smooth); the shaft from +0.06,
  tapering from R0 = D/2 to R0 (1 − 0.001 H/D), foot arris eased 8 mm, in `chenghuai.timber.red`
  (`chenghuai.timber.garden` in the garden). Facade-line and 金柱 columns are D + 0.03. Veranda fronts stand free; end
  columns stand in the gables.

### 3.3 Timber frame (大木) (`ChenghuaiHall.cpp:485-690`)
- **Purlins (檩)** round, radius Pr, tops on the seats, 0.12 into the gables.
- **Eave line:** per bay a lintel (檐枋) (0.9 D + 0.04) high × 0.75 D under Zc and a board (垫板) 0.06 thick from Zc to
  Seat[0] − 2 Pr + 0.02; in a veranda the same pair (金枋) on the facade line.
- **Frames across (梁架)** at every free column (or listed frame u): main beam W = 0.8 D + 0.04, H = 1.2 D + 0.06, top at
  min(Seat[a], Seat[b]) − 1.6 Pr; each beam above one purlin in each side, ×0.88 wide, ×0.85 high, on struts (瓜柱) 0.7 W
  square; a block under the higher purlin where seats differ; ridge strut 0.6 W. A veranda adds the 抱头梁 and a tie
  (穿插枋) 0.6 D × 0.8 D at Zc − 0.75. A gallery's back line gets lintels too.
- **Rafters (椽)** round Ø 0.095 at 0.22 (一椽一当), 0.04 past each purlin; eave rafters from 0.20 behind the eave purlin
  to v −0.66 EaveFront; flying rafters 0.085 square, tails 0.90 back; inset 0.12 + the gable from the gables; mitred
  ends cut them. **Boarding (望板)** 0.02; eave boards (小连檐, 大连檐, 瓦口); the gable band (博缝) under the edge tiles.
  Rafters, flying rafters, boarding, eave boards and unpainted purlins: `chenghuai.timber.rafter`.
- **Finish by rank:** painted (苏式彩画) `chenghuai.timber.painted` on the eave lintels and boards, facade lintels, beams
  across and purlins of the side halls, main hall, galleries and 垂花门 (fully) and the front and rear rows (restrained);
  the gate unpainted. Everything else in the house `chenghuai.timber.red`; all garden timber `chenghuai.timber.garden`.

### 3.4 Roofs, tiles and ridges
- **Slab:** the lime bed's top over the bed curve (u at 6 stations, v at ≤ 0.12 plus every knot), boarding underside,
  risers, fascias, ends; one sealed piece with the rafters under it; gable walls rise 0.015 into the boarding (no sun
  slit). Exposed bed at mitres and ends `chenghuai.roof.bed`.
- **Tiles** (`ChenghuaiRoof.h:61-93`, `.cpp:22-292`), rows down the slope, courses whose lower ends stand 9 mm proud;
  rows cut by half-planes at valleys, hips and ends. Identical courses instanced.

| Style | Surface | Where | Row pitch | Lower pan half | Upper half | Pan course | Upper course |
|---|---|---|---|---|---|---|---|
| 筒瓦: concave pans under half-round covers | `chenghuai.roof.tong` | gate, main hall, 垂花门 | 0.26 | 0.115 | 0.065 (cover) | 0.19 | 0.30 |
| 合瓦: pans up, pans down | `chenghuai.roof.he` | everything else, all copings | 0.25 | 0.115 | 0.105 | 0.17 | 0.17 |

  Sections over the bed (o across, half the half-width): lower pan base −0.01, top 0.02 + 0.03 (o/half)²; cover base
  0.03, top 0.045 + 0.05 √(1 − (o/half)²); upper pan base 0.035, top 0.05 + 0.035 (1 − (o/half)²). Rows start 0.13 from a
  清水脊's line (0 for a rolled ridge) and run to the eave edge; 0.26 in from a gable (the edge tiles). Drip and end
  tiles (滴水, 勾头): polish.
- **Ridges** `chenghuai.roof.ridge` (swept, `ChenghuaiHall.cpp:844-908`), (across, up):
  - 清水脊 (gate, main hall, ear rooms, rows, 垂花门 front), u(start) + 0.02 … u(end) − 0.02: (−0.17,−0.08) (0.17,−0.08)
    (0.17,0.06) (0.13,0.08) (0.13,0.12) (0.105,0.14) (0.105,0.34) (0.12,0.36) (0.12,0.40) (0.075,0.44)* (0,0.46)*
    (−0.075,0.44)* (−0.12,0.40) (−0.12,0.36) (−0.105,0.34) (−0.105,0.14) (−0.13,0.12) (−0.13,0.08) (−0.17,0.06).
    Scorpion tails (蝎子尾) centred 0.35 in from each end: a plate 0.56 × 0.32 × 0.07 at ridge +0.40 … +0.47; the tail swept
    u = end ± (0.10 + 0.55 t), z = ridge + 0.44 + 0.34 t + 0.08 t², t 0…1 (9 stations), section (−0.06,−0.06) (0.06,−0.06)
    (0.06,0.04) (0,0.07)* (−0.06,0.04).
  - Rolled ridge (过垄脊: side halls, galleries, 垂花门 back, flower hall, water pavilion): (−0.16,−0.04) (0.16,−0.04)
    (0.16,0.02) (0.10,0.06)* (0,0.075)* (−0.10,0.06)* (−0.16,0.02).
  - Gable edges (排山) down each hard-gabled slope at bed + 0.02, 17 stations: (−0.145,−0.06) (0.145,−0.06) (0.145,0.05)
    (0.08,0.11)* (0,0.13)* (−0.08,0.11)* (−0.145,0.05).

### 3.5 Walls
- **Gables (硬山)** (`ChenghuaiHall.cpp:912-989`) from v −0.36 to the back wall's outer face (v 5.36 for the gate); top
  on the boarding's underside + 0.015; base (下碱) `chenghuai.brick.rubbed` 1.00 high, 15 mm proud outside; body in the
  building's wall finish (gate `chenghuai.brick.rubbed`; main hall, side halls, ear rooms `chenghuai.brick.hairline`;
  rows `chenghuai.brick.pointed`; flower hall `chenghuai.wall.limewash`); 15 mm skin `chenghuai.room.plaster` inside a
  room. Framed gable openings: brick over, a red frame (head 0.10, jambs 0.08). Corbelled front end (墀头): a stone at the
  base top (−0.12 … 0, 0.55 deep); five rubbed courses 0.085 high from Zc − 0.35, stepping 0.055 k forward and 0.02 k
  sideways; a sloping board to the eave.
- **Back walls** (`:991-1021`) between the gables, v0 … v1; base floor − 0.05 … floor + 1.00; plaster skin inside; a
  sealed eave's cornice (冰盘檐), four courses 0.07 stepping 0.06 k out. The front row's, side halls' and rear row's back
  walls are the house's outer walls.
- **Free-standing walls** (`ChenghuaiParts.cpp:226-340`): base course (rubbed), body, and a tiled coping: corbel course
  0.07 high, (T + 0.10) wide; lime bed (−Hf,0) (Hf,0) (Hf,0.05) (0,0.18) (−Hf,0.05), Hf = T/2 + 0.14, `chenghuai.roof.bed`;
  合瓦 rows down both sides from 0.06 to Hf; coping ridge (−0.08,−0.04) (0.08,−0.04) (0.08,0.06) (0.05,0.11)* (0,0.125)*
  (−0.05,0.11)* (−0.08,0.06).

| Wall | Line (local) | T | H | Base | Body | Openings |
|---|---|---|---|---|---|---|
| House west | x −20.70 … −20.20; y −5.60 … −14.00, −24.40 … −39.35 | 0.5 | 4.20 | 0.93 | `chenghuai.brick.pointed` both faces | — |
| House east, south run | x 1.80 … 2.30; y −5.50 … −14.00 | 0.5 | 4.20 | 0.93 | pointed; garden face `chenghuai.wall.limewash` | garden door y −7.50 … −9.30 (2.35, stone surround); leak window y −12.20 … −13.20, z 1.25 … 2.25, circle, ice-crack |
| House east, north run | x 1.80 … 2.30; y −24.40 … −39.35 | 0.5 | 4.20 | 0.93 | as above | leak windows y −26.40 … −27.40 (hexagons), −29.40 … −30.40 (coins), −32.40 … −33.40 (octagon), z 1.25 … 2.25; moon gate y −36.10 … −38.50 |
| Court wall | y −11.10 … −11.50; x −20.20 … −10.90 and −7.50 … −2.30 | 0.4 | 3.20 | 0.93 rubbed | `chenghuai.wall.limewash` both faces | leak windows x −13.60 … −12.60 (octagon), −5.80 … −4.80 (hexagon), z 1.50 … 2.50 |
| Court wall behind the screen | same line, x −2.30 … 1.80 | 0.4 | 3.20 | 0.93 | `chenghuai.brick.rubbed` | — |
| Yard wall | x −2.30 … −1.90; y −5.50 … −11.10 | 0.4 | 3.20 | 0.93 | `chenghuai.brick.rubbed` | yard door (屏门) y −7.50 … −9.30, 2.35, frame; leaves `chenghuai.timber.green` folded open |
| Cross walls ×2 | y −30.70 … −31.10; x −20.20 … −17.70 and −0.70 … 1.80 | 0.4 | 3.20 | 0.93 | pointed | doors x −19.50 … −18.40, 0.00 … 1.10, 2.30, red frame, red leaves folded open |
| Garden north | y −44.40 … −44.90; x 2.30 … 16.90 | 0.5 | 3.60 | 0.55 | `chenghuai.wall.limewash` (粉墙) | leak windows x 3.90 … 4.90 (hexagons), 15.00 … 16.00 (octagon), z 1.15 … 2.15 |
| Garden east | x 16.40 … 16.90; y −0.50 … −44.40 | 0.5 | 3.60 | 0.55 | 粉墙 | y −21.00 … −22.00 (circle, ice-crack), −25.60 … −26.60 (coins), −37.80 … −38.80 (hexagons) |
| Garden south | y 0.00 … −0.50; x 2.30 … 16.40 | 0.5 | 3.60 | 0.55 | 粉墙 | x 4.40 … 5.40 (octagon), 14.20 … 15.20 (circle) |
| Porch ×2 | x ±2.35 … ±2.95; y 0.00 → the drum (§2) | 0.6 | 0.98 + coping to 1.10 | — | `chenghuai.brick.rubbed`; coping `chenghuai.porch.coping` | — |

- **Leak windows (漏窗)** (`ChenghuaiParts.cpp:16-124`): a panel filling the rectangle with a shaped hole (circle as a
  48-gon; hexagon; octagon turned 22.5°), R = 0.46 × min(w, h); a reveal round it; lattice `chenghuai.leak-window.lattice`
  of bars 0.028 × 0.06 on the wall's centre plane, clipped to the shape: ice-crack (9 cracks, seeded per window), coins
  (octagon rings Ø 0.24 on a 0.20 grid), hexagons (side 0.11); a rim 0.05 × 0.07 round the shape.
- **Moon gate (月洞门 · 臥遊)** (`ChenghuaiParts.cpp:166-178, 297-334`): circle R 1.20 about (y −37.30, z 1.25), cut flat
  at z 0.10; a ring of `chenghuai.brick.rubbed` 0.13 wide, 0.025 proud of both faces; the sill `chenghuai.stone.step`.
- **Screen wall (影壁 · 澄懷觀道)** (`ChenghuaiHouse.cpp:442-463`) against the court wall's south face, x ±1.40, face
  y −10.55, back −11.10: plinth (须弥座) `chenghuai.stone.base` in three stages −0.05 … 0.18 (0.08 proud), 0.18 … 0.42
  (0.02), 0.42 … 0.55 (0.07); body `chenghuai.brick.rubbed` 0.55 … 3.10; the heart `chenghuai.screen-wall.heart` 1.80
  square centred z 1.80 in a moulded frame 0.10 wide, 0.035 proud, the heart's face 0.02 back from the frame's (plain
  tonight; the carving 澄懷觀道 is polish, §8); a cornice of four courses 0.07 stepping 0.04; tiled coping at 3.38 over
  x ±1.60.

### 3.6 Fronts (装修) (`ChenghuaiHall.cpp:278-418, 1024-1119`), all `chenghuai.timber.red` (garden: `.garden`)
- **Frame (槛框):** posts 0.10 against the columns (between A and B: the gable's inner face or the column's face,
  ColR = D/2, +0.015 in a veranda); head rail 0.10 under the lintel's underside (column top − (0.9 D + 0.04)); 0.10 thick.
- **Doors (隔扇):** sill 0.10 (0.12 thick); door top = max(min(floor + 2.55, head − 0.45), floor + 2.05); if ≥ 0.18 is
  left, a door head 0.10 and a transom (横披) of lattice panels ≈ 0.95 wide, mullions 0.06; else leaves to the head rail.
  Four leaves in a bay wider than 2.2, else two. **DoorsOpen:** the middle pair swung 90° into the room about their outer
  stiles (fixed). Leaf: stiles 0.065, 0.065 thick, rails 0.06 at the foot, 0.30 H, 0.39 H, 0.92 H and the head; apron
  panel (裙板) with a raised field 0.07 in, waist panel, top panel 0.01 back; lattice over paper between waist and top rail.
- **Windows (支摘窗):** sill wall (槛墙) `chenghuai.brick.rubbed` 0.34 thick to floor + 0.79; sill board 0.40 deep to
  floor + 0.85; window sill 0.08; middle post 0.08; each half split at 0.42 of its height: lower sash plain grid, upper
  the building's pattern.
- **Lattice panels:** rim 0.030, bars 0.018 × 0.024; glazing `chenghuai.lattice.paper` 6 mm behind the bars on the room
  side (each panel a Glazed opening). Patterns: grid (cells ≈ 0.13 × 0.15); lantern (灯笼框: one ring of cells, margin
  min(0.11, 0.18 × short side), open centre with a light cross); step brocade (步步锦: two rings, ties alternating);
  ice-crack (冰裂纹: convex cells 8–14 cm by successive straight cuts, ≈ one per 0.011 m², seeded by position). Painted
  buildings step brocade; plain ones lantern; the flower hall ice-crack with lantern in its open bay.
- **Hanging fretwork (挂落)** under gallery lintels on the court side, 0.30 high, step brocade.
- Bay spans A … B (frame posts inside them) for the Glazed and Open openings:

| Room | Bay | Span | Fill |
|---|---|---|---|
| 臨池 | 1 / 6 | x −20.200 … −17.347 / −5.153 … −2.430 | 4 leaves, middle pair open (1.33 / 1.26) |
| 臨池 | 2–5 | between the columns | windows |
| 天青 / 昌南 | 1, 3 / 2 | y −23.90 … −21.00, −17.40 … −14.50 / −20.70 … −17.70 | windows / 4 leaves, middle pair open (1.40), transom |
| 澄懷堂 | 1, 3 / 2 | x −14.200 … −11.325, −7.075 … −4.200 / −10.975 … −7.425 | windows / 4 leaves, middle pair open (1.68), transom |
| 清閟 / 停雲 | 1 | x −17.200 … −14.830 (mirror) | 2 leaves closed, transom |
| 舒卷 | 1, 7 / 4 / 2, 3, 5, 6 | x −20.200 … −17.187, −1.213 … 1.800 / middle / between columns | 4 leaves, middle pair open (1.41) / 4 closed / windows |
| 林泉 | 1, 3 / 2 | x 7.00 … 8.88, 12.42 … 14.30 / 9.12 … 12.18 | 4 closed, ice-crack, transoms / 4, middle pair open (1.43), lantern, transom |

### 3.7 The gates
**The gate (广亮大门)** (`ChenghuaiHouse.cpp:268-344`), gate frame (origin (−2.30, −0.30), u east, v north), floor 0.90,
floor `chenghuai.brick.on-edge`:
- Columns `chenghuai.timber.red` at u 0.8, 3.8 × v 0, 4.9 (x ±1.50; y −0.30, −5.20), r 0.15 → 0.145, bases 0.56 square;
  middle columns (中柱) at v 2.45 (y −2.75), r 0.16 → 0.155, to Seat[2] − 0.30.
- Beams across at u 0.8, 3.8: v −0.2 … 2.45 and 2.45 … 5.1, 0.26 × 0.34, at Seat[0] − 0.28; struts 0.18 square.
- Door frame on the ridge line (y −2.75): clear 1.90 (x ±0.95); posts 0.12 × 0.18 to the head at floor + 2.75; sill
  (门槛) **0.20**; middle rail x ±1.35 at 2.75 … 2.95 over the floor; top rail under Seat[2] − 0.35; the panel over the
  door (走马板) and side panels (余塞板) 0.06 thick.
- Four plain door pins (门簪) `chenghuai.timber.green` on the middle rail at x −0.57, −0.19, 0.19, 0.57, z floor + 2.85,
  turned (0,0) (0.075,0) (0.075,0.10) (0.06,0.12)* (0,0.125), pointing south. **No characters** (Names board).
- Leaves `chenghuai.timber.black`, 0.95 × 0.08, floor + 0.21 … head − 0.02, open north against the side panels.
  Knocker plates (门钹) `chenghuai.bronze.hardware` turned (0,0) (0.11,0) (0.11,0.006) (0.05,0.03)* (0,0.035) at floor +
  1.35; rings a torus R 0.07, tube 0.009, 0.07 lower.
- Stone drums (门墩) `chenghuai.stone.base` at x ±1.25: base 0.34 × 1.10 (y −2.00 … −3.10) floor − 0.05 … + 0.28; box
  0.30 × 0.60 to + 0.70; lion block 0.20 × 0.40 to + 0.86 (carving: polish).

**The festooned gate (垂花门 · 齋莊中正)** (`ChenghuaiHouse.cpp:348-438`): platform x −10.90 … −7.50, y −9.90 … −12.60,
floor 0.45 (`chenghuai.floor.chisi`, body rubbed, kerb 0.30). Two roofs (一殿一卷), frame origin x −11.35, u east over
0 … 4.3, v north: front roof from y −10.20 (purlins v 0, 0.55, 1.1; rises .8 −.8; 筒瓦; 清水脊; eaves 0.75 / 0.04), back
roof from y −11.30 (purlins v 0, 0.42, 0.72, 1.1; rises .65 0 −.9; rolled 筒瓦; eaves 0.04 / 0.70), sharing the valley
purlin. Columns at x −10.60, −7.80 on the wall line (y −11.30) and the back line (y −12.40), r 0.125 → 0.12, bases 0.44
square. Cantilever beams (麻叶抱头梁) `chenghuai.timber.green` 0.20 × 0.30 from y −12.50 to −9.95 at z 3.106; hanging
posts (垂柱) r 0.10 at y −10.20 from 3.256 down to 2.70, each ending in a lotus bud (垂头) `chenghuai.gilt.bud` turned
(0,0) (0.05,0.02)* (0.11,0.10)* (0.13,0.19)* (0.12,0.26)* (0.10,0.30) (0,0.30) from z 2.40. A step-brocade band 0.30 high
between the hanging posts (bottom 2.514). Door frame on the wall line: posts 0.12 wide at x −10.48 … −10.36 and −8.04 …
−7.92, 0.14 thick; head 2.674 … 2.816; sill 0.12. No leaves (taken down by day). **The green screen door (屏门), shut**,
on y −12.30: four panels `chenghuai.timber.green` 0.61 wide from x −10.46 at 0.63 pitch, 0.06 thick, 0.58 … 2.654; a red
disc Ø 0.32 at z 2.20 on each (its character of 齋莊中正: polish); screen head, board and sill. Traffic goes round by
the galleries.

### 3.8 Courts and paving (`ChenghuaiHouse.cpp:535-576`)
Paving only in the open courts, cut round every platform's footing, 0.12 deep. Courts level as built (§12).

| Area | Extent (local) | Top | Surface |
|---|---|---|---|
| Porch | §2 | 0.000 | `chenghuai.porch.paving` |
| Entry yard | x −1.90 … 1.80, y −5.50 … −11.10 | 0.020 | `chenghuai.court.paving` |
| Front court | x −20.20 … −2.30, y −5.90 … −11.10 | 0.020 | `chenghuai.court.paving` |
| Front court paths | x −19.60 … −2.30, y −7.50 … −9.30; x −10.10 … −8.30, y −7.50 … −11.10 | 0.026 | `chenghuai.brick.on-edge` |
| Inner court | x −20.20 … 1.80, y −11.50 … −30.70 | 0.020 | `chenghuai.court.paving` |
| Inner court cross | x −10.10 … −8.30, y −12.60 … −25.30; x −14.60 … −3.80, y −18.30 … −20.10 | 0.026 | `chenghuai.brick.on-edge` |
| Inner court beds ×4 | x −13.80 … −10.60, −7.80 … −4.60 × y −20.50 … −24.70, −13.70 … −17.90 | kerb 0.14 wide to 0.14; soil 0.08 | `chenghuai.stone.kerb`; soil Nature's (§7) |
| Rear court | x −20.20 … 1.80, y −31.10 … −39.10 | 0.020 | `chenghuai.court.paving` |
| Rear court path | x −20.20 … 1.80, y −36.70 … −38.00 | 0.026 | `chenghuai.brick.on-edge` |
| Tree pits ×2 | 1.40 square centred (−16.80, −35.30), (−1.60, −35.30) | kerb 0.12 wide to 0.12; soil 0.05 | `chenghuai.stone.kerb`; soil Nature's |

**The court lived in** (Rules board: 天棚 鱼缸 石榴树; `chenghuai_vessels.py:270-281`), turned solids that collide (the visitor walks round):
- **Fish bowl (鱼缸)** `chenghuai.fish-bowl` on the inner court's crossing, foot on the path top at (−9.20, −19.20, 0.026); profile (r, z)
  (0, 0.01) (0.34, 0.01) (0.36, 0) (0.40, 0) (0.42, 0.04) (0.50, 0.20) (0.55, 0.42) (0.56, 0.58) (0.575, 0.63) (0.585, 0.655)
  (0.57, 0.668) (0.545, 0.66) (0.535, 0.60) (0.52, 0.42) (0.46, 0.18) (0.36, 0.06) (0, 0.05): Ø 1.17, h 0.668, rolled rim;
  water §7.
- **Two tubs** `chenghuai.tub` on the paving at (−11.60, −25.60) and (−6.80, −25.60), flanking the main hall's steps; profile
  (0, 0.01) (0.20, 0.01) (0.21, 0) (0.24, 0) (0.25, 0.03) (0.28, 0.20) (0.305, 0.40) (0.315, 0.445) (0.31, 0.46) (0.29, 0.455)
  (0.285, 0.40) (0.26, 0.22) (0.23, 0.06) (0, 0.05): Ø 0.63, h 0.46; soil at 0.405 above the foot (z 0.425), the pomegranates §7.

### 3.9 The garden (臥遊)
- **Ground:** the garden's ground at z 0.02 over x 2.30 … 16.40, y −0.50 … −44.40 with the pond cut out is Nature's
  ground bed `chenghuai.garden.ground` (§7); Form builds the paths, kerbs, platforms, the rockery and the pond basin on it.
- **Pond basin** (`ChenghuaiGarden.cpp:79-149`) `chenghuai.pond.bed`: the bank from the edge (z 0.02) 0.45 inward and
  down to the bed; the bed at **−0.85**, rising to **−0.46** over the tribune: bed(r) = lerp(−0.46, −0.85,
  clamp((r − 8.30)/1.50, 0, 1)), r from (0, −22.30). Water at **−0.28** (§7). Area 106.6 m², perimeter 41.7, depth 0.18 …
  0.57. Under the water pavilion the edge is pulled to x 14.30 between y −29.0 and −33.7.
- **Edge rocks (湖石驳岸)** `chenghuai.edge-rock` (`ChenghuaiRockery.cpp:178-215`): along the edge every 0.60 … 0.95 a
  broad stone R 0.42 (±20 %) from water − 0.35 to 0.00 … +0.06; on 55 % an upper stone R 0.30 from −0.03 to +0.12 … +0.42;
  each a 7-sided outline stretched 1.3× along the bank, seeded. Prisms tonight; dissolution polish.
- **Paths (花街铺地)** (`:154-183`): 0.95 wide: pebble bed `chenghuai.garden.path` 0.83 wide, −0.04 … 0.04, between two
  kerbs `chenghuai.garden.bluestone` 0.06 wide, −0.06 … 0.055, swept on the ground along the centre lines below. Path 0
  **divides round 石兄**: it leaves its straight line at x 3.833 and rejoins it at x 6.767, the two branches on the
  circle r 1.475 about (5.300, −37.300) (the rock's base within r 0.60, kerbs 0.40 clear).
- **Zigzag bridge** (`:187-213`): points (5.20, −27.90) (7.20, −27.90) (8.20, −26.50) (10.40, −26.50) (11.40, −27.50)
  (13.00, −27.50); slabs `chenghuai.garden.bluestone` 1.00 wide × 0.14, top **0.10**, each run 0.50 past its turns; piers
  `chenghuai.garden.bluestone.submerged` 0.50 square at ≤ 1.60 centres from 0.10 below the local bed to the slab's
  underside (within r 8.30 of the tribune they stop on the shallow bed, never below −0.50). No rail (Suzhou); a low
  kerb `chenghuai.bridge.kerb` 0.10 wide × 0.08 high on both edges of every run (the as-built code's own "low kerb
  along each side", `ChenghuaiGarden.cpp:189`; clear 0.80 between), hosting the step lights (§4).
- **Rockery (假山)** `chenghuai.rockery` (`ChenghuaiRockery.cpp:22-172, 289-398`): a height field made a sealed solid.
  Massing 1.2 smoothstep(−0.2, 1.8, −d_foot) + 1.05 smoothstep(−0.4, 1.3, −d_upper) + 0.5 smoothstep(0, 1.4, 3.2 − r_p)
  (d signed distance to the outlines below, r_p distance to the pavilion's centre (10.60, −11.30)); stones a jittered
  Voronoi of 0.95 cells, each lifted (hash − 0.45) × 0.55, leaning ≤ ±0.15 per metre, crevices 0.16 deep within 0.24 of
  its edge; summit plateau **2.60** within r 2.05 of the pavilion, blending to r 2.60. The path: half-width 0.55, a
  ledge at 2.6 × min(up, down) quantised to 0.15 risers (+0.02), stone beside it ≥ 0.20 higher or falling away (west
  branch climbs 2.6 in 3.87; east descends over 14.95). Sampled on 0.10 over x 4.6 … 16.2, y −2.7 … −19.8. Taihu
  hollows (≤ 110) and standing peaks (≤ 6): polish.
- **Hexagonal pavilion (亭 · 見山)** (`:401-558`), centre (10.60, −11.30), corners at φ 60° k: platform hexagon R 1.70,
  `chenghuai.garden.bluestone`, 2.30 → **2.75** (floor); six columns `chenghuai.timber.garden` at R 1.45, r 0.09 → 0.0855,
  2.35 high, bases 0.28 square; lintels 0.12 × 0.20 centred floor + 2.25; step-brocade fretwork 0.28 high from floor +
  1.87 on faces 1, 2, 4, 5 only (not the open faces k 0 and 3); on the same four faces a seat (plank 0.36 × 0.05 at
  floor + 0.44 on a base 0.30 × 0.40, 0.18 inside the face) with a goose-neck back (美人靠): balusters 0.035 square about
  every 0.13, rising 0.42 from floor + 0.46 and leaning out 0.02 + 0.20 sin(1.9 t); back rail 0.07 × 0.05 at floor + 0.89,
  0.19 outside. Roof: eave corners R 2.35; z = base + (apex − base)(1 − f)^1.35 + 0.45 c³ f², base 5.45, apex 6.85, f the
  fraction out along the face normal, c = |sector|/30°; soffit 0.14 under the bed; 合瓦 between hips; hips (−0.08,−0.06)
  (0.08,−0.06) (0.08,0.08) (0.05,0.12)* (0,0.13)* (−0.05,0.12)* (−0.08,0.08) rising 0.12 t⁶ at the corners; corner beams
  0.10 × 0.16; finial (宝顶) turned (0,0) (0.2,0) (0.2,0.08) (0.13,0.14)* (0.17,0.30)* (0.12,0.45)* (0.05,0.58)* (0,0.62)
  from 6.73, `chenghuai.roof.ridge`.
- **Covered walk (复廊 · 遊目)**: roof back meets the limewashed face at x 2.30. **Engraved stones (书条石)**
  `chenghuai.shutiao`, twelve, 0.90 × 0.32, 0.03 proud (x 2.29 … 2.33), z 1.17 … 1.49, two to a bay: for bay marks B =
  −36.25, −25.60, −22.90, −20.20, −17.50, −12.10 the stones run y B + 0.35 … B + 1.25 and B + 1.55 … B + 2.45. Incised
  text polish.
- **Flower hall (花厅 · 林泉)** walls `chenghuai.wall.limewash` on a rubbed base; floor `chenghuai.garden.bluestone`
  (ice-crack); **terrace (月台)** x 8.10 … 13.20, y −36.70 … −38.20, top 0.30. **Water pavilion (水榭 · 知魚)**: building
  table; its west part on a base `chenghuai.garden.bluestone.submerged` in the pond.

**Garden outlines** (`ChenghuaiGardenPlan.h:12-105`, y + 17.000; paths are centre lines, 0.95 wide):

pond.edge (the pavilion notch applied), 75 points, x y:
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
rockery.foot, 64 points:
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
rockery.upper, 64 points:
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
rockery.path (centre line, west end first), 55 points:
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
path 0, moon gate to terrace (straight at y −37.452), x 3.699 → 8.102 (11 points). path 1, terrace to water pavilion:
```
 13.199 -37.500   13.402 -37.404   13.694 -37.276   14.025 -37.116   14.345 -36.924   14.602 -36.699
 14.799 -36.430   14.970 -36.114   15.112 -35.773   15.223 -35.428   15.301 -35.102   15.329 -34.772
 15.312 -34.422   15.270 -34.088   15.225 -33.804   15.199 -33.602
```
path 2, water pavilion to bridge:
```
 14.801 -29.102   14.830 -28.959   14.881 -28.754   14.926 -28.520   14.941 -28.291   14.898 -28.102
 14.785 -27.946   14.617 -27.803   14.416 -27.679   14.203 -27.576   14.000 -27.500   13.790 -27.461
 13.559 -27.456   13.333 -27.471   13.138 -27.490   13.000 -27.500
```
path 3, bridge to walk (straight at y −27.898), x 5.199 → 3.699. path 4, walk to the pond's south:
```
  3.699 -18.301    4.055 -18.579    4.551 -18.982    5.139 -19.436    5.770 -19.867    6.398 -20.199
  7.046 -20.418    7.747 -20.571    8.464 -20.684    9.161 -20.784    9.801 -20.898   10.391 -21.016
 10.955 -21.118   11.484 -21.221   11.968 -21.343   12.398 -21.500   12.777 -21.721   13.112 -21.995
 13.397 -22.278   13.628 -22.527   13.801 -22.699
```
path 5, walk to the rockery's west foot:
```
  3.699 -14.301    3.956 -14.284    4.319 -14.265    4.743 -14.235    5.185 -14.184    5.602 -14.102
  6.005 -13.984    6.423 -13.835    6.839 -13.665    7.238 -13.484    7.602 -13.301    7.942 -13.097
  8.271 -12.867    8.571 -12.638    8.820 -12.440    9.000 -12.301
```
path 6, rockery's east foot to walk: (3.699, −8.398) (3.958, −8.383) (4.340, −8.361) (4.761, −8.337) (5.141, −8.316)
(5.398, −8.301).

### 3.10 Fittings
- **Cases** (Art's display furniture, built by the stage from these numbers; `ChenghuaiDisplay.cpp:63-204, 510-597`).
  Bodies, plinths, ends, canopies, mullions, stands `chenghuai.case.phoebe`; toes `chenghuai.timber.black`; beds, decks
  and backs `chenghuai.case.silk` (scroll cases) or `chenghuai.case.silk.ceramics` (ceramic cases); glass
  `chenghuai.case.glass`; hanging and lamp rails `chenghuai.case.bronze`. Each case's own solids collide.

| Case | Kind | Extent (local) | Depth | Deck / canopy / top (z) |
|---|---|---|---|---|
| 臨池 | slanted | x −19.90 … −4.70, front y −1.30, back to the wall (−0.50) | 0.80 | floor 0.30 |
| 舒卷 Thousand Li / Qingming | slanted | x −19.70 … −6.30 / −5.30 … 0.90, front y −43.60, back −44.40 | 0.80 | floor 0.30 |
| 天青 wall case (mullions y −20.85, −17.55) | upright | y −23.40 … −15.00, glass x −19.60, back −20.20 | 0.60 | 1.270 / 2.770 / 3.030 |
| 昌南 wall case | upright | mirror: glass x 1.20, back 1.80 | 0.60 | same |
| 澄懷堂 ×3 (rails, low run) | upright | x −13.55 … −11.85, −10.00 … −8.40, −6.70 … −4.70; glass y −33.80, back −34.40 | 0.60 | 0.950 / 4.310 / 4.610 |
| 清閟 back / side | upright | x −16.50 … −15.40, glass y −32.95 / y −31.40 … −32.60, glass x −16.75 | 0.45 | 0.700 / 3.580 / 3.860 |
| 停雲 back / side | upright | x −3.20 … −1.70 / y −31.40 … −32.60, glass x −1.65 | 0.45 | same |
| 天青 pedestal | free-standing | centre (−18.20, −19.20), 0.80 square | — | pedestal 0.95 above the floor, glass hood 0.50 |

  Slanted: black toe 0.09; phoebe body to floor + 0.84, panels every ≈ 1.5; base moulding and cornice to + 0.88; the silk
  bed tilted 11° from floor + 0.905 at v 0.05 to v D − 0.06 (a 0.02 board, top 0.01 above that line); glass front upright
  to floor + 1.10, top sloping 11° to the back; ends 0.035; a bronze lamp rail at the back under the glass (v D − 0.10 …
  D − 0.03) hiding the lamp run. Upright: toe 0.08; plinth to the deck with mouldings and a conditioning drawer; silk
  deck 12 mm and back 25 mm on a 10 mm board (silk face at v D − 0.035); ends 0.045; mullions 0.05; canopy with cornice
  0.02 proud and a baffle 0.05 hiding the lamp run; glass 10 mm at v 0.012; cases over 2 m tall inside have a phoebe lip
  0.048 at the deck's front hiding the low run; scroll cases a bronze hanging rail Ø 0.016 at v D − 0.05, canopy − 0.05.
- **Hall screens (落地罩)** `chenghuai.screen.phoebe` across the main hall on x −11.15 and −7.25, from y −28.46 to −33.35,
  floor 0.75 to 4.63: end posts 0.09; fretwork legs 0.55 (step brocade) on solid bases 0.42; a frieze 0.50 (lantern
  lattice) under a head 0.07; grid spandrels 0.42 × 0.30 in the upper corners.
- **Furniture** `chenghuai.furniture` (phoebe, Ming forms): altar table (翘头案) x −10.20 …
  −8.20, y −32.54 … −32.96, 0.82 high with everted ends (+0.06); scholar's desk x −4.25 … −2.75, y −1.90 … −2.65, 0.82,
  its chair at (−3.50, −1.55) facing north; 昌南's table x −0.50 … 0.10, y −18.65 … −19.75, 0.80, chairs at (−0.20,
  −20.15) facing south and (−0.20, −18.25) facing north. Tables: top 0.035, legs 0.045 set in 0.06, aprons, low
  stretchers. Chairs (南官帽椅): 0.58 × 0.46, seat 0.50, crest 1.03, S-splat 0.16, stepped stretchers.
- **林泉's painting table and rose chairs** `chenghuai.furniture.huanghuali` (huanghuali, as the collection records; the
  real type): a recessed-leg painting table (画案, 夹头榫) centred (10.65, −40.60), 1.90 × 0.80, top 0.80 (0.035 thick,
  ice-plate edge), round legs Ø 0.05 set 0.10 in from the ends and 0.05 from the sides, a plain apron with spandrels,
  two stretchers between the legs at each end, no drawers; a pair of rose chairs (玫瑰椅) at (10.15, −41.35) and (11.15,
  −41.35) facing south: seat 0.57 × 0.44 at 0.50, a straight low back to 0.86 and arms to 0.72, square-framed in round
  members Ø 0.025, a low openwork rail 0.12 over the seat round back and sides, stepped stretchers, a board seat.
- **Desk things** on the scholar's desk (top 1.12) and on the painting table (top 1.25): a fired-clay inkstone (澄泥砚)
  `chenghuai.desk.inkstone` 0.18 × 0.26 × 0.03 in a phoebe box 0.20 × 0.28 × 0.012 (`chenghuai.furniture`), a brush pot
  r 0.065 × 0.16 and a water dropper r 0.04 × 0.05 `chenghuai.desk.stoneware`. Desk: inkstone at (−3.45, −2.27), pot
  (−3.10, −2.22), dropper (−3.90, −2.27). Painting table: inkstone (10.85, −40.55), pot (10.45, −40.45), dropper (10.30,
  −40.70); the **Lingbi rock** (§8) on a phoebe stand r 0.11 × 0.03 at (11.35, −40.45), `chenghuai.lingbi-rock`.
- **Lanterns (纱灯)**, 17 plain hexagons (`ChenghuaiDisplay.cpp:360-443`): a bronze rod `chenghuai.bronze.hardware` from
  the hook; black-lacquered top plate (R + 0.018, 0.025) and crown (R − 0.05, 0.02), base plate and foot the same
  (`chenghuai.timber.black`); six posts 0.02 square; silk-gauze panels `chenghuai.lantern.silk` 4 mm between the posts
  with a rail 0.012 × 0.008 across their middle. No characters, no tassels; none at, on or over the gate or the 垂花门.
  Positions §4.
- **Plaques and couplet** (Names board; `ChenghuaiHouse.cpp:658-772`), boards `chenghuai.timber.black` 25 mm on the wall
  (characters gilt: polish): 臨池 1.0 × 0.5 at (−2.327, −2.95, 2.45) facing west; 天青 1.1 × 0.55 at (−18.10, −14.541,
  2.90) facing north; 昌南 at (−0.30, −14.541, 2.90) facing north; 清閟 0.8 × 0.4 at (−14.725, −30.05, 3.05) facing west;
  停雲 at (−3.675, −30.05, 3.05) facing east; 舒卷 1.1 × 0.55 at (−9.20, −44.359, 2.42) facing south; 林泉 1.2 × 0.6 at
  (10.65, −42.059, 2.95) facing south; 澄懷堂 2.0 × 0.64 on a painted lintel between the rear 金柱 (x −10.975 … −7.425,
  y −33.36 ± 0.10, z 4.67 … 4.97), face y −33.234, centre z 4.84, facing south. The couplet shells (22 mm) wrap the
  hall's middle veranda columns (x −11.15, −7.25; y −27.00), 1.9 rad centred facing south, z 2.05 … 3.45. Names are never
  on or over a door or gate (the owner's rule).
- **No invisible guards** (pond, bridge, rockery path, cases, pavilion): collision equals the visible solids; the
  Visitor's ledge rule holds the edges (§10).

### 3.11 Stock
Grey brick 0.288 × 0.144 × 0.064; 城砖 0.48 × 0.24 × 0.12 on edge; floor tiles 尺七 0.54 and 尺四 0.45 square; court
paving 0.40 square; lattice bars 18 × 24 mm; boards ≤ 0.30 wide. Identical tiles, bars and courses are instanced.

### 3.12 Surfaces (Finishes.json)

| Surface | Where | Palette key |
|---|---|---|
| `chenghuai.stone.kerb` | platform kerbs, corner stones, footing courses, gallery and bed kerbs | accent.qingbaishi |
| `chenghuai.stone.step` | the house's steps and side stones, thresholds, moon-gate sill | accent.qingbaishi |
| `chenghuai.stone.base` | column bases and drums, 门墩, the screen wall's plinth | accent.qingbaishi |
| `chenghuai.porch.paving`, `chenghuai.porch.coping` | the porch | accent.qingbaishi |
| `chenghuai.brick.rubbed` | 干摆 (§3.5, §3.6) | accent.grey-brick |
| `chenghuai.brick.hairline` | 丝缝 walls | accent.grey-brick |
| `chenghuai.brick.pointed` | 淌白 walls, platform bodies | accent.grey-brick |
| `chenghuai.brick.on-edge` | 城砖: court cross paths, the gate hall floor | accent.grey-brick |
| `chenghuai.court.paving` | the courts' square paving | accent.grey-brick |
| `chenghuai.screen-wall.heart` | 影壁心 | accent.grey-brick |
| `chenghuai.leak-window.lattice` | leak-window bars and rims | accent.bu-wa |
| `chenghuai.floor.chiqi` | main hall, ear rooms | accent.suzhou-fangzhuan |
| `chenghuai.floor.chisi` | side halls, rows, galleries, 垂花门 | accent.suzhou-fangzhuan |
| `chenghuai.roof.tong`, `chenghuai.roof.he`, `chenghuai.roof.ridge` | tiles, copings, ridges, finial | accent.bu-wa |
| `chenghuai.roof.bed` | exposed lime bed | white.lime-plaster |
| `chenghuai.wall.limewash` | 粉墙 | white.limewash |
| `chenghuai.room.plaster` | rooms' inner skins | white.limewash |
| `chenghuai.timber.red`, `chenghuai.timber.painted`, `chenghuai.timber.green`, `chenghuai.timber.black` | house timber by finish | accent.qing-pine-lacquered |
| `chenghuai.gilt.bud` | the 垂花门's lotus buds | accent.qing-pine-lacquered + gilt.gold-leaf.oil |
| `chenghuai.timber.rafter` | rafters, boarding, eave boards, unpainted purlins | accent.chinese-fir-guangqi |
| `chenghuai.timber.garden` | all garden timber | accent.chinese-fir-guangqi |
| `chenghuai.case.phoebe`, `chenghuai.screen.phoebe`, `chenghuai.furniture` | cases, 花罩, the other tables and chairs, inkstone boxes, the rock's stand | accent.phoebe |
| `chenghuai.bronze.hardware`, `chenghuai.case.bronze`, `chenghuai.fire-vat`, `chenghuai.south.blind-doors` | knockers, rods; case rails; 太平缸 (polish); the port's blind (on the stand) | bronze.cusn10 |
| `chenghuai.lattice.paper`, `chenghuai.case.glass` | paper glazing; case glass | glass.low-iron |
| `chenghuai.case.silk`, `chenghuai.case.silk.ceramics`, `chenghuai.mount.silk`, `chenghuai.lantern.silk` | case silks, scroll mounts, lanterns | accent.silk |
| `chenghuai.garden.bluestone`, `chenghuai.garden.bluestone.submerged`, `chenghuai.pond.bed`, `chenghuai.shutiao`, `chenghuai.rockery` | garden stone | accent.qingshi |
| `chenghuai.edge-rock`, `chenghuai.shixiong` | lake rocks | accent.taihu-rock |
| `chenghuai.garden.path` | 花街铺地 | accent.pebble-mosaic |
| `chenghuai.bridge.kerb` | the bridge's low kerb | accent.qingshi |
| `chenghuai.fish-bowl`, `chenghuai.tub`, `chenghuai.desk.stoneware`, `chenghuai.desk.inkstone` | the court's bowl and tubs; desk things | accent.stoneware-glazed |
| `chenghuai.furniture.huanghuali` | 林泉's painting table and chairs | accent.huanghuali |
| `chenghuai.lingbi-rock` | the Lingbi rock | accent.lingbi-rock |

## 4. Light

**Daylight** (Lighting.json `daylight`; no blinds, D-014)

| Opening | Glazing | Weather | Control | Faces |
|---|---|---|---|---|
| `chenghuai.linchi.lattice` | `glass.low-iron`, laminated with mulberry paper, UV-filtering | museum | none | north (the court) |
| `chenghuai.tianqing.lattice` / `chenghuai.changnan.lattice` | same | museum | none | east / west |
| `chenghuai.chenghuaitang.lattice` | same | museum | none | south, under a 1.30 veranda and 1.10 eave |
| `chenghuai.qingbi.lattice` / `chenghuai.tingyun.lattice` | same | museum | none | south, behind the hall's gables |
| `chenghuai.shujuan.lattice` | same | museum | none | south, deep eaves |
| `chenghuai.linquan.lattice` | same | museum | none | south over the pond |

Open: the courts' tops, the porch and the garden (Outdoors; Light sets sun and sky per enclosure), the open door pairs,
the leak windows. Light through the paper is diffuse transmission carried by Light's transport (never a glow).

**Care by the hang (D-014).** Silk and paper hang only where no direct sun reaches: the north-lit front row, the deep
hall, the ear rooms (lit only from the hall), the rear row behind deep eaves; ceramics take the east- and west-facing
side halls. Every lattice is paper in UV-filtering laminated glass. Under the museum's idealised sun (D-003: constant
elevation 71.49°, tan = 2.985) no direct sun lands more than ≈ 1.4 in plan inside an eave's edge: on 澄懷堂's veranda it
stops ≥ 0.9 short of the lattice, in 舒卷 within 0.15 of it (the cases are 4.7 behind), and 臨池 faces north (the sun
never stands north of east–west). Under the `ephemeris` model (winter noon ≈ 26.6° at 40° N) sun can reach ≈ 5 m into 澄懷堂's open bay and
the west end of 舒卷; Art.WithinCare decides, and the answer is the hang or which door pair stands open, never a blind.

**Luminaires** (Lighting.json; 145; no output values are set here)

| Ids | Type | Count / extent | Point and aim | Operation |
|---|---|---|---|---|
| `case.linchi`, `case.thousand-li`, `case.qingming` | `case-linear.3000k` | 3 runs of 15.08, 13.28, 6.08 | under each slanted case's rear lamp rail, z 1.474, 0.065 in from the wall; aim the bed's centre line (z 1.283) | presence |
| `case.tianqing`, `case.changnan` | `case-linear.3000k` | 2 runs of 8.26 | behind the canopy baffle (z 2.764, 0.075 inside the glass); aim the back silk at z 1.60 | presence |
| `case.hall.west/.centre/.east` + `.low` | `case-linear.3000k` | 6 runs (1.56, 1.46, 1.86) | canopy (z 4.304) and deck lip (z 0.970), y −33.875; aim the scroll's middle / lower part | presence |
| `case.qingbi.back/.side`, `case.tingyun.back/.side` + `.low` | `case-linear.3000k` | 8 runs (0.96, 1.06, 1.36, 1.06) | canopy z 3.574 and lip z 0.720; aim the scroll | presence |
| `spot.ru-basin` | `track-spot.3000k.12deg` | 1 | on the 天青 track (−16.650, −19.200, 3.170); aim the basin (−18.200, −19.200, 1.450) | presence |
| `wash.linchi.s.01…14` | `track-wallwasher.3000k` | 14 on a black track x −19.60 … −2.90, y −4.95, z 3.02 | aim the south wall over the case (x, −0.515, 1.90) | always |
| `wash.linchi.n.01…07` | `track-wallwasher.3000k` | 7, same x, y −0.85, z 3.02 | aim the lattice front (x, −5.20, 1.20) | always |
| `wash.tianqing.01…07` / `wash.changnan.01…07` | `track-wallwasher.3000k` | 7 + 7 on x −16.65 / −1.75, y −23.30 … −15.10, z 3.17 | aim the wall case's glass (−19.60 / 1.20, y, 2.00) | always |
| `wash.hall.01…08` | `track-wallwasher.3000k` | 8, x −13.70 … −4.70, y −28.70, z 4.45 | aim the case wall (x, −33.80, 2.60) | always |
| `wash.qingbi.s.01…02`, `wash.qingbi.e.01…03` | `track-wallwasher.3000k` | 2 (x −16.90 … −15.00, y −29.25, z 3.45) + 3 (x −15.00, y −29.70 … −32.80, z 3.70) | aim the back case (x, −32.95, 2.00) / the side case (−16.75, y, 2.00) | always |
| `wash.tingyun.s.01…02`, `wash.tingyun.w.01…03` | `track-wallwasher.3000k` | 2 (x −3.40 … −1.50) + 3 (x −3.40) | aim (x, −32.95, 2.00) / (−1.65, y, 2.00) | always |
| `wash.shujuan.01…17` | `track-wallwasher.3000k` | 17, x −19.60 … 1.20, y −40.10, z 3.02 | aim the north wall (x, −44.385, 2.00) | always |
| `wash.linquan.01…05` | `track-wallwasher.3000k` | 5, x 7.50 … 13.80, y −38.90, z 3.25 | aim the back wall (x, −42.10, 1.80) | always |
| `lantern.*` | `lantern.2200k` | 17 | the lamp at the lantern's centre (below); aim straight down | dusk |
| `step.bridge.1…5` | `gap.step-light.2700k` | 5, one per run at its midpoint, alternate sides | in the kerb's inner face 0.40 off the centre line, z 0.14 (0.04 over the tread, under the kerb's lip); aim the slab 0.20 past the centre line, z 0.10 | dusk |
| `step.rockery.01…14` | `gap.step-light.2700k` | 14: every fourth riser of the ascent, every second of the descent, alternate sides | in the flanking stone face 0.62 off the path's centre line, 0.20 over the upper tread; aim the path 0.90 further on | dusk |
| `step.path4.1…3`, `step.fork.1`, `step.court.1…6`, `step.gate.1…3`, `step.yard.1` | `gap.step-light.2700k` | 14 (ruled for the short walks) | flush in kerbs, bed kerbs, the drum's base, the back kerb, a side stone and a threshold (Night, below); aim across the walk's centre line | dusk |

The washers are the rooms' architectural light: `always`, daylight-dimmed by Light (D-006); the works' case runs stay
presence-linked (dose). Heads on a track stand at the centres of equal divisions of it. Tracks are the luminaires' own product (black), fixed
under the lintels. **Lanterns** (hook (x, y, z); rod; R × H; lamp z): hall veranda (−10.20 / −8.20, −27.15, 4.90); 0.86;
0.17 × 0.36; 3.81 · galleries (−13.00 / −5.40, −12.45, 3.20); 0.30; 0.12 × 0.26; 2.72 · side verandas (−15.75 / −2.65,
−19.20, 3.60); 0.47; 0.15 × 0.32; 2.92 · 臨池 doors (−18.708 / −3.792, −5.58, 3.10); **0.40**; 0.13 × 0.26; 2.52 · 舒卷
doors (−18.629 / 0.229, −39.42, 3.10); 0.40; 0.13 × 0.26; 2.52 · walk (2.925, −9.40 / −17.50 / −25.60 / −33.70, 2.75);
0.12; 0.11 × 0.24; 2.46 · water pavilion (14.70, −31.35, 3.20); 0.25; 0.17 × 0.36; 2.72 · flower hall (9.90 / 11.40,
−38.40, 3.31); 0.35; 0.17 × 0.36; 2.73. Every lantern's foot clears its floor by ≥ 2.04.

Not re-expressed (as-built fakes): window "glow" lights, emissive paper, per-lamp output values and scales, the two post-process
volumes, the invisible guards (`ChenghuaiStructure.cpp:196-241, 448-486`; `chenghuai.py:312-405`).

**Targets**

| Surface | Class | Lux | Source |
|---|---|---|---|
| `chenghuai.case.silk` (measured on the silk works) | textiles.silk | 50 / 50 | Care |
| `chenghuai.case.silk` (measured on the paper works) | works-on-paper | 50 / 50 | Care |
| `chenghuai.case.silk.ceramics` | ceramic-and-glass | 300 recommended, no max | Care |
| `chenghuai.floor.chiqi`, `chenghuai.floor.chisi` (rooms' walking zones) | gallery.low-light (silk, paper, ink) | 50 | UseTargets |
| `chenghuai.porch.paving`, `chenghuai.court.paving`, `chenghuai.brick.on-edge`, `chenghuai.stone.step`, `chenghuai.garden.path`, `chenghuai.garden.bluestone`, `chenghuai.rockery` (the walks' centre lines, after dusk) | garden.night (after EN 1838's escape-route floor) | ≥ 1 | UseTargets |

**Night: `garden.night`** (≥ 1 lx on a walk's centre line after dusk). The walks are the loop's outdoor centre lines (§10)
and the courts' and garden's paths (§3.8, §3.9). The design's own places meet it first. The estimate is by distance: a
design estimate that Light measures, never a setting.
- Each **lantern** (dusk) holds 1 lx within ≈ 4.0 in plan of its hook where it sees the walk (a small flame-warm source
  2.5–3.8 up, falling off with the square of the distance).
- Each **room's lit front** (the washers, `always`: the paper lattice and the open door pair lit from inside) holds 1 lx
  within ≈ 3.5 of that front.
- The **Rotunda's lit portal** (its cove and floor light at dusk, seen through 4.00 × 6.50) holds 1 lx within ≈ 6 of the
  drum.

| Walk | Met by | Estimate |
|---|---|---|
| Porch and the gate's front steps (y 5.8 → 0) | the Rotunda's portal | meets |
| Gate hall and back steps (y 0 → −6.6) | the portal covers to y −0.2; beyond it 6–12 away | below 1 lx from y −0.3 → **step lights** (3) |
| Entry yard (gate ↔ yard door, garden door ↔ gate) | walk lantern 1 (0.6 outside the garden door) to 4.0 through the door; the back-steps light | the garden-door route meets; the walk to the yard door (x −0.7 … −1.9, 4.2–5.1 from the lantern) is below → **step light** (1) |
| Front court path (y −8.40) and on to the 垂花门 | 臨池's lit front 3.1 away; its door lanterns; the gallery lanterns 3.8 from the 垂花门's axis | meets (the 垂花门's steps marginally) |
| Galleries, verandas, corner passages, cross-wall doors | gallery, veranda and hall lanterns; the lit fronts | meets |
| Inner court cross paths | the side halls' and hall's lit fronts; gallery and hall lanterns | the N–S path from y −14 to −23 and the E–W path from x −12.8 to −5.6 are below → **step lights** (6) |
| Rear court path (y −37.35) | 舒卷's lit front 2.35 away; its door lanterns | meets |
| Covered walk | walk lanterns 8.1 apart | meets (≤ 4.05 from one) |
| Path 0 (moon gate → terrace, with the fork) | walk lantern 4; the flower hall pair | the south branch meets (3.2); the north branch is 4.4–5.6 away → **step light** (1) |
| Paths 1, 2, 3, 6 | 知魚, flower hall and walk lanterns | meets |
| Path 4 (walk → the pond's south) | walk lantern 2 covers its first 2.91 | the remaining 8.41 is below → **step lights** (3) |
| Path 5 and the rockery path to the pavilion | walk lantern 2 at its start; no lantern on the rockery | short → **step lights** |
| Zigzag bridge | walk lantern 3 and 知魚 at its ends; the middle run 5.3 from both | short → **step lights** |

**Step lights** `gap.step-light.2700k` (33, dusk) have hidden sources and no visible fitting: each is set flush behind a
louvre in a kerb or edge stone and grazes the walk. Each is taken to hold 1 lx on the centre line within ±2.0 along it,
so they stand at the fewest that cover each short stretch.
- Bridge: 5 in the inner face of its low kerb (`chenghuai.bridge.kerb`), one per run at its midpoint on alternate sides,
  lighting `chenghuai.garden.bluestone`.
- Rockery: 14 in the flanking stones (`chenghuai.rockery`), at every fourth riser of the steep ascent and every second of
  the long descent, on alternate sides, lighting the path's treads (`chenghuai.rockery`).
- Path 4: 3 set flush in its bluestone kerbs (`chenghuai.garden.bluestone`), alternate sides, at equal spacing (2.80) over
  the 8.41 beyond the lantern's reach, lighting `chenghuai.garden.path`.
- The fork round 石兄: 1 in the outer kerb of its north branch at (5.30, −39.25), lighting `chenghuai.garden.path`.
- Inner court: 6 in the faces of the four beds' kerbs (`chenghuai.stone.kerb`), z 0.08: one on each arm of the cross
  paths and two at the bed corners that face the crossing (the fish bowl stands there), lighting `chenghuai.brick.on-edge`.
- Gate hall: 1 in the inner face of the east stone drum's base (`chenghuai.stone.base`) for the floor and the sill; 1 flush
  in the platform's back kerb (`chenghuai.stone.kerb`) for the floor north of the sill; 1 in the east side stone of the back
  steps (`chenghuai.stone.step`) for the steps. None stands at, on or over the gate's door, and there is no lantern.
- Entry yard: 1 in the yard-side face of the yard door's stone threshold (`chenghuai.stone.step`), lighting
  `chenghuai.court.paving`.
- Points and aims are in Lighting.json, each with its `host` and lit `surface`. With these, every walk meets 1 lx by
  estimate; Light measures (ruled, §13 (b)).

**Design order** (Lighting.json `design`; measured, never tuned): in every scroll room the works on their silk are the
brightest reflecting surfaces, 2–5 × the plastered walls, day and night; the walls over the dark oiled floor; by day the
paper lattice's inner face is the brightest thing in a paper room; the ceramics' decks over the side halls' walls; at
night the lantern silk over the court paving in its pools. By day the sunlit 粉墙 are the garden's brightest diffuse
surfaces, the pond darker than every wall (checked, not a ratio).

## 5. Time and weather
- Every Glazed opening (the paper lattices), every Court top, the porch and the garden take the **museum** observer
  (D-005 names no remote weather here). Rain and snow fall into the courts, porch and garden and stop under roofs,
  verandas and galleries; the pond takes ripples; Matter's surface law wets paving, tiles and lime.
- Sun and hour are the museum's (D-003, idealised by default): the courts show the sun move.
- Seasons from the solar terms: trees, bamboo and lotus follow Nature's phenology (§7).

## 6. Motion
None tonight: every leaf is a fixed piece in its pose (gate leaves open against the side panels; the 屏门 shut; the
rooms' open pairs swung 90° in; the yard door's and cross-wall doors' leaves folded open against their reveals), none in
a walked path. Polish: the 屏门 opens on festival solar terms (Rules board), a revolute pair, 90°, its travel inside the
垂花门.

## 7. Nature
- **Water** `chenghuai.pond` (Planting.json): anchor `chenghuai.pond.water`; outline the 75-point polygon (§3.9); level
  −0.28; depth 0.18 … 0.57 (0.57 where deep); freeboard 0.30 (the ground at 0.02); `water.still`, still and clear over
  the bluestone bed (palette); no flow. Old optics (absorption 0.06, 0.02, 0.06 and scattering 0.006, 0.014, 0.008 per
  metre, `chenghuai.py:407-427`) are evidence, not a knob.
- **Fish bowl water** `chenghuai.fish-bowl`: anchor `chenghuai.fish-bowl.water`; a circle r 0.532 at the level z 0.606
  (0.58 above the bowl's foot); depth 0.53; freeboard 0.088 to the rim; `water.still`, still and clear.
- **Ground beds** (Nature makes the form; the palette owns the material): each bed entry carries `"ground"` with
  `accent.site-geology` (topsoil) and its working and history. They are `chenghuai.garden.ground` (the garden's ground at
  0.02 with the pond cut out, topsoil under moss *Haplocladium microphyllum*), the four inner-court beds (soil 0.08) and
  the two rear-court pits (0.05), bare under their trees, and the two pomegranate tubs (soil 0.425, potted).
- **Plants** (Planting.json; crown sizes as built):

| Bed | Species | Count | Height / crown radii |
|---|---|---|---|
| `chenghuai.inner-court.bed.nw`, `chenghuai.inner-court.bed.ne` | *Malus × micromalus* (西府海棠, before the main hall) | 1 each | 4.6, 4.5 / 1.7 × 1.2 |
| `chenghuai.inner-court.bed.sw`, `chenghuai.inner-court.bed.se` | *Osmanthus fragrans* (as built) | 1 each | 3.6, 3.5 / 1.3 × 1.2 |
| `chenghuai.rear-court.pit.w`, `chenghuai.rear-court.pit.e` | *Ziziphus jujuba* | 1 each | 5.2, 5.0 / 1.9 × 1.3 |
| `chenghuai.garden.tree.willow-1`, `chenghuai.garden.tree.willow-2` | *Salix babylonica* (the board's willow, by the water) | 1 each | 4.4, 3.4 / 2.0 × 1.6, 1.2 × 1.1 |
| `chenghuai.garden.tree.pine` | *Pinus bungeana* (the board's pine, on the rockery) | 1 | 4.8 / 2.1 × 1.7 |
| `chenghuai.garden.tree.plum-1`, `chenghuai.garden.tree.plum-2` | *Prunus mume* | 1 each | 3.6, 3.8 / 1.2 × 0.9, 1.4 × 1.0 |
| `chenghuai.garden.tree.crabapple` | *Malus × micromalus* | 1 | 4.2 / 1.7 × 1.2 |
| `chenghuai.garden.bamboo.east` / `chenghuai.garden.bamboo.north-west` / `chenghuai.garden.bamboo.north-east` | *Phyllostachys aureosulcata* | 14 / 18 / 21 culms | east by the pond x 15.53 … 16.07, y −23.35 … −28.14; behind the flower hall x 5.33 … 10.01 and 10.36 … 15.75, y −42.81 … −44.10 (culm points `chenghuai_works.py:454-463`, y + 17) |
| `chenghuai.pond.lotus` | *Nelumbo nucifera* | 6 | (7.60, −33.90) Ø 0.9, (8.60, −32.80) 0.7, (6.80, −32.30) 0.8, (11.00, −24.50) 0.9, (9.80, −23.90) 0.7, (11.80, −25.50) 0.6; flowers on the first and fourth |
| `chenghuai.garden.ground` | *Haplocladium microphyllum* (moss) | cover | the whole ground bed; growth spaces from Form's free space |
| `chenghuai.inner-court.tub.w`, `chenghuai.inner-court.tub.e` | *Punica granatum* (石榴, in the glazed tubs of §3.8) | 1 each | 1.9 / 0.75 × 0.55 |

- All outdoors: history `weathered`, museum weather, wind in the bamboo and willow. Bank plants (*Acorus calamus*,
  *Iris sanguinea*) and the dissolution of 石兄 and the rockery: polish.
- **Precipitation** through Court tops and Outdoors (§5).

## 8. Art
**Sources:** `assets/collection.json` (every id below, verbatim). Sizes: scrolls from `chenghuai_works.py:195-215`
(image w × h) and the catalogue's handscroll widths; the Tianqing case's monochromes from the catalogue (turned profiles
`windows/SourceArt/Textures/ch_vessels.json` scaled to them, B-13); the Changnan case's vessels and the Ru basin from their
own photographs (`assets/ceramics/<id>/`, K9), catalogued with their museums' sizes, accessions and credits in
`MuseeVision/Data/Art/Ceramics.json`; images `assets/paintings/<id>*.jpg` (the Thousand Li
and Qingming in four tiles, Huai Su in two), colour-managed as the paint layer's albedo. Credits `assets/collection.json`,
`assets/art_sources_extra.json`.

**Hanging scrolls** hang by cords from the case's bronze rail (canopy − 0.05), the top stick's middle 0.085 under it,
the mount 0.02 off the silk. Mount (装裱) `chenghuai.mount.silk`: heaven (天头) and earth (地头) per work, bands (隔水)
0.06, borders 0.04, a roller with knobs at the foot, clearing the deck by ≥ 0.01. `along` and `height` place the
**image** (the painting); size is the image.

| Anchor | Work (id) · title · artist | Support · care | Image w × h | Heaven / earth (mounted length) | along · height | Image centre (x, y, z) |
|---|---|---|---|---|---|---|
| `chenghuai.chenghuaitang.case.west` | chinese-guo-xi-early-spring · Early Spring · Guo Xi | silk · textiles.silk | 1.081 × 1.583 | 0.55 / 0.30 (2.553) | 0.2645 · 1.232 | (−12.700, −34.347, 2.774) |
| `chenghuai.chenghuaitang.case.centre` | chinese-fan-kuan-travelers · Travellers among Mountains and Streams · Fan Kuan | silk · textiles.silk | 1.033 × 2.063 | 0.62 / 0.36 (3.163) | 0.2385 · 0.682 | (−9.200, −34.347, 2.464) |
| `chenghuai.chenghuaitang.case.east` | chinese-li-tang-wind-in-pines · Wind in Pines among a Myriad Valleys · Li Tang | silk · textiles.silk | 1.398 × 1.887 | 0.60 / 0.32 (2.927) | 0.2560 · 0.878 | (−5.700, −34.347, 2.572) |
| `chenghuai.qingbi.case.back` | chinese-ni-zan-rongxi · The Rongxi Studio · Ni Zan | paper · works-on-paper | 0.355 × 0.747 | 0.36 / 0.20 (1.427) | 0.3275 · 1.778 | (−15.950, −33.347, 2.652) |
| `chenghuai.qingbi.case.side` | chinese-wang-meng-qingbian · Dwelling in the Qingbian Mountains · Wang Meng | paper · works-on-paper | 0.422 × 1.406 | 0.42 / 0.22 (2.166) | 0.3440 · 1.059 | (−17.147, −32.000, 2.262) |
| `chenghuai.tingyun.case.back` | chinese-shen-zhou-lushan · Lofty Mount Lu · Shen Zhou | paper · works-on-paper | 0.981 × 1.938 | 0.42 / 0.22 (2.698) | 0.2145 · 0.527 | (−2.450, −33.347, 1.996) |
| `chenghuai.tingyun.case.side` | chinese-wen-zhengming-living-aloft · Living Aloft · Wen Zhengming | paper · works-on-paper | 0.457 × 0.953 | 0.40 / 0.22 (1.693) | 0.3265 · 1.532 | (−1.253, −32.000, 2.509) |

**Handscrolls** lie on the slanted silk beds (11°), each opened from its beginning at the reader's right, its lead (and
wrapper) to the right and its rolled remainder at the left; spaced equally along the bed; if the spans overrun, the lead
shortens, never the work. `along` is the image's left edge (as the reader sees it) on the bed anchor's x; `height` its
lower edge up the slope from the bed's front edge (the image centred on the bed's mid-line, 0.3515 up the slope).

| Anchor | Work (id) · artist | Support · care | Image w × h | Lead / wrapper / roll / band / border | along · height | Image x (world) |
|---|---|---|---|---|---|---|
| `chenghuai.linchi.case.bed` | chinese-orchid-pavilion · Feng Chengsu after Wang Xizhi | paper · works-on-paper | 0.699 × 0.245 | 0.06 / 0 / 0.025 / 0.025 / 0.012 | 14.188 · 0.229 | −19.622 … −18.923 |
| same | chinese-yan-zhenqing-nephew · Yan Zhenqing | paper · works-on-paper | 0.755 × 0.282 | same | 13.181 · 0.211 | −18.671 … −17.916 |
| same | chinese-huaisu-autobiography · Huai Su | paper · works-on-paper | 7.550 × 0.283 | same | 5.378 · 0.210 | −17.663 … −10.113 |
| same | chinese-su-shi-cold-food · Su Shi | paper · works-on-paper | 1.995 × 0.342 | same | 3.130 · 0.181 | −9.860 … −7.865 |
| same | chinese-mi-fu-shu-su · Mi Fu | silk · textiles.silk | 2.708 × 0.278 | same | 0.170 · 0.213 | −7.613 … −4.905 |
| `chenghuai.shujuan.case.thousand-li.bed` | chinese-thousand-li · Wang Ximeng | silk · textiles.silk | 11.915 × 0.515 | 0.45 / 0.22 / 0.035 / 0.03 / 0.012 | 0.280 · 0.094 | −19.385 … −7.470 |
| `chenghuai.shujuan.case.qingming.bed` | chinese-qingming · Zhang Zeduan | silk · textiles.silk | 5.287 × 0.248 | **0.23** / 0.22 / 0.03 / 0.03 / 0.012 | 0.102 · 0.228 | −5.164 … 0.124 |

臨池's five span 13.707 + 5 × 0.173 = 14.572 on a 15.05 bed (gaps 0.080); the Thousand Li 12.90 on 13.25; the Qingming
6.047 on 6.05. Bed centre lines: 臨池 y −0.907, z 1.283; 舒卷 y −43.993, z 1.283.

**Ceramics** (anchor Mount; `centre` = the foot on the deck, stands on it; stands `chenghuai.case.phoebe` carved, radius
1.25 × the foot, 0.028 high under 0.20 tall, else 0.04; the scanned pieces stand on the deck unless given a stand radius):

| Work (id) | Object | Size w × h (m) | Anchor · y | Stand |
|---|---|---|---|---|
| chinese-ding-basin | turned "ding" | 0.248 × 0.115 | `chenghuai.tianqing.case.deck` · −22.60 | yes |
| chinese-jun-flowerpot | turned "jun" | 0.203 × 0.190 | same · −21.60 | yes |
| chinese-sancai-horse | scan `usd/sculptures/chinese-sancai-horse.usdc`, turned 90° | — × 0.70 | same · −19.20 | none |
| chinese-guan-vase | turned "guan" | 0.206 × 0.340 | same · −17.00 | yes |
| chinese-jian-bowl | turned "jian" | 0.121 × 0.073 | same · −16.20 | yes |
| chinese-longquan-vase | turned "longquan" | 0.079 × 0.146 | same · −15.50 | yes |
| chinese-ru-basin | derived (NPM 故瓷017851), oval, on four feet, turned 90° | 0.230 × 0.069 (mouth 23.0 × 16.4) | `chenghuai.tianqing.pedestal` | none |
| chinese-meiping | derived (V&A C.8-1952), painted | 0.206 × 0.356 | `chenghuai.changnan.case.deck` · −22.80 | yes |
| chinese-xuande-jar | derived (Met 37.191.1), painted | 0.487 × 0.483 | same · −21.70 | yes |
| chinese-wucai-jar | derived (Met 17.127.2), painted | 0.237 × 0.232 | same · −19.90 | yes |
| chinese-chicken-cup (object 1) | derived (NPM 故瓷005190), painted | 0.083 × 0.037 | same · −18.50 | yes |
| chinese-peachbloom | derived (Met 14.40.377), peach-bloom glaze | 0.080 × 0.197 | same · −16.90 | yes |
| chinese-falangcai-bowl | derived (Met 65.86.12), painted | 0.113 × 0.054 | same · −15.80 | yes |
| chinese-chicken-cup (the handling copy, object 2) | as the cup, yaw 30° | 0.083 × 0.037 | `chenghuai.changnan.table` at (−0.280, −19.080, 1.250) | none |

Care: ceramic-and-glass (the sancai horse is lead-glazed earthenware: the same class). The four scans that exist only as
prims of the retired wing's export are *works' evidence*; Studio imports them to `Evidence/Art` with provenance.

**Vessels from their photographs** (K9; `Data/Art/Vessels.json`, grade `derived`, each derivation in its record, made by
`MuseeVision/Scripts/vessels.py`): each section is the photograph's silhouette turned through the photograph's own
camera (a pinhole fitted to the rim's and foot's ellipses, which corrects its perspective), a ring for each row of the
photograph, at the museum's height, the widest diameter of the section as stored checked against the museum's: the
Xuande jar 48.7 cm against 48.3, the falangcai bowl 11.3 against 11.4 (foot 4.4 against 4.4), the chicken cup 8.3 against
8.2 (foot 4.1 against 4.1), the meiping 20.6 against the V&A's 22.0 (a departure, below); the wucai jar (23.7) and the
peach-bloom vase (8.0) have no published diameter. Each figure is the section's as stored, every point of it (the bowl's
flared lip is its widest), as make and Musee.Art.PaintedVessel measure it, and the test holds each derivation's figure to
its section's. Each is turned at its museum's height by one scale, so it keeps its
photographs' proportion, and its plan gives the section's width: a widest diameter more than 2% from the museum's must be
a departure its record explains, or make and Musee.Art.PaintedVessel find it. Below 12% of its height the wucai jar's
side is lost against the ground in its own soft shadow: there its section runs through the rings its bottom view DP342699
measures (a pinhole on the axis, fitted to the collar's upper line and the silhouette): the collar's lower line 61.3 mm
from the axis at 7.2 mm up, and the foot's outer edge 57.6 mm (11.5 cm across; the side view's shadow had made it 13.3). The outside keeps the fine turns its photograph shows where its edge is
crisp (the peach-bloom's two fillets at the neck's foot and its everted lip, the Xuande jar's neck step and rolled lip),
and is smoothed over a hundredth of its height elsewhere, where a pale glaze's faint edge against the ground is pulled
in by the painting just inside it. The inside is a wall's thickness in from the outside along its normal: the thin-potted
chicken cup's 0.65 mm, the falangcai bowl's 0.79 and the peach-bloom's 1.28, each its lip's thickness read on its
photograph where the rim turns at its side; the jars' and the meiping's Data/Art/Ware.json's 6 mm. No section crosses
itself or thins its wall under half of it (checked by the script, by make and by Musee.Art.PaintedVessel). The Ru basin
has no side view: its section joins the museum's four sizes by the proportions its oblique photograph shows, turned in
the round and pressed front to back as a potter presses a thrown round to an oval: at the lip to the mouth's
23.0 × 16.4 and at the flange's face to the foot's 19.3 × 12.9, the pressing running evenly up the wall between them
(make and Musee.Art.PaintedVessel check both axes of the mouth and the foot, the test on the turned mesh itself). Its
plan is the rounded oblong its photographs show, not an ellipse: a superellipse of exponent 2.35 at the flange's face,
fitted to the flange's outer edge on the base photograph (611 of 720 rays; the rest are hidden by the feet), and 2.36
at the lip, fitted to the white lip on the oblique photograph through a pinhole fitted with it (an ellipse misses the lip
by 7.6 px rms, exponent 3 by 9.9, 2.36 by 2.0). Musee.Art.PaintedVessel checks the turned mesh's plan against both
traced outlines all round, across the diagonals too, within 3%. Its underside runs out to the flange's face over a 1 mm
arris, and it stands on its four cloud-scroll feet, each traced from the base photograph as it hugs a corner of the base,
9 mm high, within the underside (make checks it).
The painted vessels (Xuande, wucai, falangcai, chicken cup, meiping) wear their decoration, composed by Art round the
turned form from their views: each view projected through its camera onto all the surface it sees, obliquely too (the
Xuande jar's reign mark and the meiping's shoulder, tilted up and away, the wucai jar's lappets round its shoulder),
wherever no other part of the vessel hides it and it shows inside the silhouette by its camera's miss and 2 px more;
views blended by the square of how squarely they see it, each fading into the glaze over the last 12° before its
silhouette (where a pixel of it covers five times the surface and the glaze's shading falls away steeply) and over a
millimetre where another part hides the surface. Each photograph is divided by its white: the glaze's hue, at the
lightness the glaze itself shows round each place (the 90th percentile of its glaze's samples there, the glaze being the
samples of little colour once that hue is divided out), so the light falling off down the meiping in its gallery, or the
chicken cup's wall darkening as it turns under, is taken off and the painting keeps its darkness against its own ground
(Musee.Art.PaintedVessel compares it with each photograph's, window by window up the outside, and counts the texels
clipped to white). Where two views' paintings lie more than 5 mm apart where they overlap (the wucai jar's sides, 6.5 to
7.0 mm, measured block by block as the composition weighs them), they are not blended, which showed its carp twice: each
keeps its own side of the overlap, and they meet at a seam run where their paintings agree best, cross-faded over 2 mm;
where a carp crosses the seam its line steps by those millimetres. Where no view sees the outside, each band of ornament
whose repeat is known is carried round by it, and elsewhere the glaze is left: the Xuande jar's 23 lotus panels and 8
neck clouds, counted on its photograph (the neck's band read between the neck's lower and upper double lines, 43.66 to
48.06 cm up, so it no longer reaches down onto the shoulder, where the reign mark stands once: the record says so, and
the test composes the jar and finds the mark nowhere else round it); the chicken cup's two plain lines. The rest have no whole repeat to carry (the wucai jar's collar, below), a
count the photographs don't settle (the meiping's lotus panels, below) or no repeat the photographs trust (the wucai
jar's lappets and neck scroll, the meiping's diaper band and peony scroll: their painting lies on itself at a
correlation under 0.5 at every turn), and are left in the glaze where no view sees them. A further view keeps the camera
its own silhouette fixes, and is composed only where its painting lies on the views it overlaps within 5 mm in every
fifth of the height they share. The wucai jar's three sides are placed round it by its bottom view, which sees the collar
and the pond above it all round: each side's collar and pond laid on the bottom view's by their enamels' colours
(DP342698 at 185°, DP342700 at 96° from DP342697; correlations 0.53 to 0.58 against 0.19 to 0.28 at the next best turn),
and the sides lie on each other's painting where they meet (DP342698 and DP342700 at 0.49, DP342700 and DP342697 at
0.38). Musee.Art.PaintedVessel measures every placed view's overlaps again from its recorded camera and its photographs,
and finds a view turned a quarter turn from its place. The Xuande jar's photograph sees 42% of it, the wucai jar's three
sides 93%, the meiping's side A 42%, the bowl's and cup's one photograph each 43 to 44%. Left out (Departures): the
Xuande jar's older photograph, the meiping's side B, and the wucai jar's bottom view, which places its sides and measures
its foot but is not composed (its painting lies 9 mm off DP342700's in the pond).

**Building and garden works** (built by Form or Nature; Art adds only the caption, Hang `"built": "form"`):
chenghuai-yingbi (the screen wall's heart `chenghuai.screen-wall.heart`, 1.8 × 1.8, along 0.00, height 0.88 on
`chenghuai.entry-yard.yingbi`; care ceramic-and-glass, fired brick; plain tonight); chenghuai-shutiao (12 stones on
`chenghuai.walk.stones`, 0.90 × 0.32, height 1.02, one entry per stone as `canvas` 1…12 read from the moon gate
south); chenghuai-altar-table, chenghuai-scholars-desk, chenghuai-painting-table (the furniture of §3.10 at their Mount
anchors). **石兄** chenghuai-shixiong: a Taihu rock 3.00 high, grown as built by Nature's Rock (seed 731) and set by Art
on `chenghuai.garden.shixiong` in `chenghuai.shixiong`, framed by the moon gate on its axis (care stone-and-bronze).
**The Lingbi rock** chenghuai-lingbi-rock (`chenghuai.lingbi-rock`, `accent.lingbi-rock`): 0.32 high, grown as built by
Nature's Rock (seed 732) and set by Art on its phoebe stand at (11.35, −40.45, 1.28) on the painting table
(`chenghuai.linquan.table`); care stone-and-bronze. **No wall labels** ("a literati house names itself"): captions on look.

## 9. Sound
- **Rooms** (Enclosed): small and dry (臨池 ≈ 17.9 × 4.8 × 3.5; 澄懷堂 10.0 × 6.1, open to 7.75), from Matter: lime
  skins, paper lattice partly open, tile floors, timber, silk-lined cases; coupled to the courts through the open door
  pairs and the paper.
- **Courts:** open tops, early reflections only. **Porch and garden** (Outdoors): free field with the walls' reflections.
- **Weather on Form's exposed surfaces by substance:** rain on clay tile roofs (the house's sound in rain), on brick
  paving, on the pond, in the bamboo; wind in bamboo and willow (museum observer).
- **Footfalls:** tile floors, 青白石, bluestone, pebble mosaic, brick on edge. No emitters tonight (later: a recorded
  Suzhou garden at its measured level).

## 10. Visitor
**The loop** (Plan board; one door in and out): 1 大门 (porch, five steps up) → 2 影壁 · 澄懷觀道, west through the
green 屏门 → 3 臨池, in at the west door bay, east along the case, the desk in the east bay, out east → 4 垂花门 (shut;
round by the galleries) → 5 天青, the Ru basin opposite the door → 6 澄懷堂, up four steps; Guo Xi, Fan Kuan, Li Tang →
7 清閟 and 8 停雲 through the hall's gable doors → 9 昌南, the handling copy on the table → 10 后院 through the north-east
cross-wall door → 11 舒卷, in at the east bay, walking west → 12 月洞门 · 臥遊, 石兄 framed → 13 林泉 (terrace and hall)
→ 14 知魚 and the zigzag bridge → 15 見山, up the rockery: the Rotunda's dome is the southern mountain (借景) → 16 遊目,
back along the walk past the engraved stones, through the garden door into the entry yard, out by the gate.
- **Sit:** the pavilion's four seats (Seat anchors, facing inward, seat top 3.19).
- **Look:** captions on every work, including the building works. The chicken cup's "the one you may hold" needs a hold
  intent the Visitor lacks: polish.
- **Step-to (VR):** courts, paths, floors, platforms, the bridge, the rockery path; never the pond or a roof.
- **The ledge rule:** the body never walks off a drop taller than MaxStepHeight (0.22) (pond edge, bridge sides, rockery
  drops, platform kerbs); it uses the steps. This replaces the as-built invisible guards.

## 11. The old-code map
Paths relative to `windows\`. Re-express, never copy; **y + 17.000** throughout.

| Part | Where |
|---|---|
| Plan numbers (site, porch, gate, yard, rows, court wall, 垂花门, halls, galleries, garden buildings, bridge, water, tribune) | Source/MuseeVision/Chenghuai/ChenghuaiPlan.h:27-137 |
| Garden outlines (pond, rockery foot/upper/path, seven paths); pond notch | …/ChenghuaiGardenPlan.h:12-105; ChenghuaiWater.cpp:9-26 |
| Part list (as-built materials) | …/ChenghuaiGeometry.h:15-57 |
| Hall spec vocabulary; proportions; the section; columns; beams | …/ChenghuaiHall.h:32-114; ChenghuaiHall.cpp:18-33, 38-192, 220-273 |
| Lattice panels; door leaf; platform and steps; frame; rafters | …/ChenghuaiHall.cpp:278-418, 424-483, 485-690 |
| Roof slab, tiles, ridges, scorpion tails, gable edges | …/ChenghuaiHall.cpp:693-909; ChenghuaiRoof.h:61-99; ChenghuaiRoof.cpp:22-292 |
| Gables, 墀头; back walls; fronts | …/ChenghuaiHall.cpp:912-989, 991-1021, 1024-1119 |
| Building specs (rows, gate, halls, ear rooms, galleries G1–G4) | …/ChenghuaiHouse.cpp:60-264 |
| Gate joinery; 垂花门; 影壁; walls and porch; courts; gallery floors; fretwork; plaques | …/ChenghuaiHouse.cpp:268-344, 348-438, 442-463, 467-531, 535-576, 580-649, 658-772 |
| Leak windows, moon gate, paving, coping, walls with holes | …/ChenghuaiParts.cpp:16-340; ChenghuaiParts.h:16-63 |
| Garden levels, walls, ground and pond, paths, bridge, walk/flower hall/water pavilion, terrace, engraved stones | …/ChenghuaiGarden.cpp:27-47, 58-75, 79-149, 154-213, 218-288, 296-321 |
| Rockery, edge rocks, hollows and peaks, pavilion | …/ChenghuaiRockery.cpp:20-176, 178-215, 217-399, 401-558 |
| Cases; tables, chairs, desk things, 花罩; lanterns; washer tracks; rooms | …/ChenghuaiDisplay.cpp:63-204, 209-348, 360-443, 456-508, 510-597 |
| Scroll mounts; vessel turning | …/ChenghuaiScroll.h:10-108, ChenghuaiScroll.cpp:37-274; ChenghuaiVessel.h/.cpp |
| Not ported: glows, output values, scales, post-process volumes, guards | …/ChenghuaiStructure.cpp:196-241, 448-486; Scripts/chenghuai.py:312-405 |
| Works: sizes, mounts, hang, ceramics, plants, court things, placards | Scripts/chenghuai_works.py:55-69, 195-215, 292-353, 440-582, 605-728 |
| Vessel profiles | Scripts/chenghuai_vessels.py:26-36, 270-281; SourceArt/Textures/ch_vessels.json |
| The Rotunda's north door; porch in the Rotunda's dress | Source/MuseeVision/Plan/MuseePlan.h:44-47; Rotunda/RotundaStructure.cpp:309-312; Facade/MuseeFacadeStructure.cpp:33-37, 198-200, 635-642 |
| Boards | repo plan/proposals/chenghuai/canvas/*.dc.html |

**Corrections to the draft** (`r-specs/…/Stages/chenghuai.md`):
1. **Class** `doorway.4000x6500.arched` (width × crown; draft `doorway.4000x4500.arched`). Port frame, facing and
   museum translation verified unchanged: (0, 5.80, 0) facing +y, museum = local + (0, −17, 0).
2. **Porch:** walls at |x| 2.35 … 2.95 (draft 2.0 … 2.6), so the joint's outer surround (≤ 0.35 past the jambs) runs to
   the paving; the draft's "collar" and its open item are dropped; porch paving `accent.qingbaishi` (Story, "then
   青白石"; draft 城砖).
3. **Materials are palette keys** (D-018; draft's substance.working.history keys): e.g. Fangshan marble →
   `accent.qingbaishi`; Korean pine → `accent.qing-pine-lacquered`; nanmu → `accent.phoebe`; brass hardware and case
   rails → `bronze.cusn10` patinated (Story: gate hardware, case frames); lantern panels silk gauze (纱灯; as built
   silk; draft oiled paper); pond bank and bed `accent.qingshi` (palette: clear over bluestone; draft puddled clay);
   gate hall floor 城砖 (Materials board; draft 尺四).
4. **Light:** the draft's new records are the ruled catalogue types `case-linear.3000k` and `lantern.2200k`; every
   head is listed with a point and an aim point; operations always / presence / dusk (D-006). Care targets come from
   Care.json: ceramics 300 recommended (the draft took 200 from the Rooms board). Rooms' floors use the ruled
   `gallery.low-light` (50); the walks use the ruled `garden.night` (≥ 1), with 33 step lights at the bridge, the
   rockery and the short walks (the draft left the night walks unlit).
5. **Row-door lanterns' rods 0.40** (draft and as built 0.52): the lantern's foot hung 1.93 over the 0.30 platforms.
6. **Built tonight** (the draft deferred them): the furniture (phoebe; the painting table and chairs in huanghuali), the
   fish bowl, tubs and pomegranates and the desk things (glazed stoneware), and the Lingbi rock (Lingbi stone). The
   painting table's brush pot moves to (10.45, −40.45): as built it overlapped the Lingbi rock's stand.
7. **Ground:** Nature's ground beds in `accent.site-geology` (topsoil), not a Form sheet in "garden-soil".
8. **Path 0's fork round 石兄** given exactly (circle r 1.475 about the rock; draft "forks at x 4.3 … rejoins at 6.3").
9. **Hang:** image positions computed from the as-built rule (hanging scrolls' `along`/`height`; handscrolls on tilted
   bed anchors).
10. **Verified unchanged:** the y + 17.000 conversion; the building table and derived heights (spot-checked seats and
    ridges); galleries; flights (Blondel values); walls and openings; gates; courts; pond (area 106.6, perimeter 41.7,
    bed law, notch); outlines; bridge; rockery; pavilion; walk; engraved stones (B-11); cases; lantern hooks; washer
    tracks; plaques; works, sizes, mounts and ceramic positions; plants' positions (y + 17).

## 12. The first-pass cut
**Tonight:** Site (frame, footprint, port with its class, blinds on the stand, enclosures and shared Open openings); the
hall composition instanced as the front row, gate hall, two side halls, main hall, two ear rooms, rear row, eight
galleries, the 垂花门's two roofs, the walk, the flower hall and the water pavilion (platforms, steps, columns, frames,
rafters, slabs, tile rows with lips, ridges and scorpion tails, gables with 墀头, back walls with cornices, fronts with
all four lattices); the gate's joinery (drums as blocks), the 垂花门 (plain discs), the 影壁 (plain heart), every wall
with its openings, copings and leak windows, the moon gate, the porch; courts, paths, bed kerbs and pits; the garden's
pond basin and water, edge-rock prisms, paths with the fork, the bridge, the rockery mass (no hollows or peaks), the
pavilion with seats, the terrace, the bridge's kerb; cases, 花罩, furniture (the painting table and chairs in
huanghuali), desk things, the fish bowl and tubs, lanterns, plaque boards; the luminaires of §4 with the step lights;
the 14 scrolls in their mounts, 8 turned vessels, 5 scans and the handling copy; 石兄 and the Lingbi rock if Nature's
Rock is ready; trees, bamboo, lotus, pomegranates, moss; the pond and the fish bowl's water; precipitation; footfalls.

**Waits for polish:** characters (plaques, couplet, 影壁心, 书条石, 屏门 discs), the 门墩 lions and carved aprons (Form's
Ornament from evidence, never a flat texture); 包袱 scenes and rafter-end painting; drip and end tiles; Taihu dissolution
(石兄, rockery peaks and hollows); the goldfish; paper, brushes and paperweights on the desks; a cane seat for the rose
chairs; the 天棚, the bronze 太平缸 `chenghuai.fire-vat`, the rear court's stone benches, bank plants, seasonal flowers;
the 屏门's festival opening; the hold intent; recorded sound; falls on the courts.

**Risks (watch tonight's checks):** light through paper needs Light's translucent transport (never a glow); annual dose
for silk (15,000 lx·h) is tier 2: presence-linked case light must dim empty rooms; **after dusk** `garden.night` rests on
distance estimates (lanterns, lit fronts, the portal, 33 step lights; §4): Light measures it, and there is no lantern at
the gate (the owner's rule); flat courts pond after rain (real courts fall ≈ 1 % to drains; the owner decides); the rockery's
short ascent (0.15 risers on 0.22) is Nature's ground, not a Form stair; *Osmanthus fragrans* is not hardy in Beijing
(kept in tubs there); ≈ 50,000 tiles must be instanced for Nanite; the Rotunda must stay resident for the borrowed view.

## 13. Palette keys to confirm and catalogue gaps
**Palette keys** (no `todo.` keys; roles guessed or workings outside `workingsByEra`):
- `accent.qingbaishi` for the **porch paving and coping** (Story: "the doorway detail, then 青白石") and thresholds.
- `accent.grey-brick` for the **courts' square paving** (roles name the cross paths only; a Beijing court's 方砖) and the
  **gate hall floor** (城砖, Materials board).
- `accent.bu-wa` for the **leak-window lattices** (tile-and-brick bars; roles name roofs).
- `accent.chinese-fir-guangqi` for the house's **rafters, boarding and unpainted purlins** in iron-red oil, aged (the
  palette role says "unfinished"; as built red); working `iron-red.tung-oil`, outside workingsByEra.
- `accent.qingshi` for the **pond bank and bed** (palette water.still: "clear over bluestone"), the **bridge piers** and
  the **rockery body** worked "as found" (outside workingsByEra).
- `accent.taihu-rock` for the **pond's edge rocks** (roles: 石兄 and peaks).
- `accent.silk` for the **case silks, scroll mounts and lantern gauze** (roles name the Salon only).
- `accent.phoebe` for the **other furniture**, the inkstone boxes and the rock's stand (roles name the screens and case
  plinths). Ruled keys used: `accent.huanghuali` (the painting table and rose chairs), `accent.stoneware-glazed` (the
  fish bowl, tubs and desk things, the inkstone worked `unglazed.polished`), `accent.lingbi-rock`.
- `accent.qing-pine-lacquered` + `gilt.gold-leaf.oil` for the **lotus buds** (as built gilt brass); `bronze.cusn10`
  patinated for **knockers, rings, lantern rods and case rails** (Story: gate hardware, case frames; the boards'
  "brass").
- Ground beds: `accent.site-geology` (topsoil), ruled, with workings `topsoil.under-moss`, `topsoil.bare`,
  `topsoil.potted` (its classical working is "absent": new workings for Matter).
- `accent.qingshi` for the **bridge's low kerb** (hosting the step lights).
- **Care classes guessed:** the furniture works' captions use `oil-and-tempera` (waxed wood has no class); 石兄, the Lingbi
  rock and the 书条石 `stone-and-bronze`; the 影壁心 (fired brick) and the sancai horse (lead-glazed earthenware)
  `ceramic-and-glass`.
- Workings written as compound strings: `rubbed.dry-laid`, `hairline-jointed`, `lime-pointed`, `laid-on-edge`,
  `square.ground-joints`, `tung-oil-soaked`, `ground-joints`, `laminated.mulberry-paper.uv-filtering`,
  `laminated.anti-reflective`, `ma-hui.whiting` and others in Finishes.json.

**Catalogue gaps** (Light):
- `gap.step-light.2700k`: a hidden-source step light recessed behind a louvre in a stone face (a small warm-white LED
  step/wall light with a glare-free louvre, of the kind set into stair strings and garden walls), 19: the bridge's kerb
  (5) and the rockery path's flanking stones (14). The ruled types `case-linear.3000k` (19 runs) and `lantern.2200k`
  (17) are used as catalogue keys.

**Conflicts and questions for the Architect:**
(a) **Porch vs door** (resolved here, no change to the Rotunda): walls moved out to |x| 2.35 … 2.95; confirm Site's
outer surround is ≤ 0.35 wide beyond the jambs, and that the Rotunda's exterior dress (polish) stops at the walls.
(b) **Night walks** (ruled, closed): hidden-source step lights set flush in kerbs or edge stones, at the fewest that meet
1 lx, wherever the estimate fell short: path 4, the fork round 石兄, the inner court's cross paths, the gate hall and
the entry yard (14 more, §4). Still no lantern at the gate.
(c) Materials board "bamboo blinds keep direct sun off every work": no blinds (D-014); care by the hang and UV glazing.
(d) Materials board "plank shutters close the lattice at night": not built (no mechanism tonight).
(e) Materials board draws the garden roofs 歇山 / 攒尖; as built they are rolled (the flower hall, water pavilion) and a
curved hexagonal roof (the pavilion): kept as built (D-017).
(f) The draft's osmanthus (as built) in the south beds is kept; the garden's osmanthus and plum stand-ins are replaced
by the board's willow and pine (plums kept).

## Build notes
- **Frame.** Origin on the gate axis at the site's south face, z 0 = the Rotunda's floor; **museum = local + (0, −17.000,
  0)**, yaw 0. Every old number: y + 17.000.
- **The port.** `chenghuai.south` at (0, 5.800, 0) facing +y on the drum's cylinder R 11.20 about (0, 17.000); class
  `doorway.4000x6500.arched`; passage 0; Chenghuai has no wall there and builds no rough opening. The joint builds the
  threshold, strip, linings and the surround on the drum's outer face (the porch side); the Rotunda's own archivolt
  (rotunda.md §3.3) dresses the inner face. Chenghuai's porch walls stand at |x| 2.35 … 2.95 and end on the drum
  face, the paving ends on the mating arc.
- **One composition.** Build the Qing hall once (§3.1 rule) and instance it 14 times plus the galleries; the gate, the
  垂花门, walls and garden are their own drawings.
- **The Classical Hall is beneath.** Nothing below z −0.50 within x ±4.20 north of the drum or within r 8.30 of
  (0, −22.30); the pond's bed and the bridge piers obey it there.
- **Nature makes the ground and the water.** The garden's ground, the court beds', pits' and tubs' soil (Nature's beds,
  material `accent.site-geology` in each bed's `ground`), the pond's and the fish bowl's water (Planting.json); Form
  makes the pond basin, kerbs, paths, platforms, rockery, the bowl and tubs.
- **Night.** The washers are `always` (the rooms' lit fronts light the courts), the lanterns and step lights `dusk`;
  the 33 step lights sit hidden in the bridge's new low kerb, the rockery's flanking stones and the kerbs and edge stones
  of the short walks (points in Lighting.json).
- **Hang.** Hanging scrolls hang from the case rails; `along`/`height` place the image, the mount (`chenghuai.mount.silk`)
  is Art's. Handscroll bed anchors are tilted 11°: their z runs up the slope. Building works (影壁心, 书条石, furniture)
  are Form's pieces; Art adds captions only.
- **Nothing moves; no invisible guards.** The ledge rule holds the edges; collision equals the visible solids.
- **Lanterns are architecture with a `lantern.2200k` lamp.** None at, on or over the gate or the 垂花门; names never
  over doors.

**Departures from as-built** (for the owner's review):
- **Porch walls** move out from |x| 1.90 … 2.50 to 2.35 … 2.95, so the Rotunda's 4.00 door and its surround stand clear
  (as built the walls stood 0.10 inside each jamb and ran 8 cm into the drum).
- **Flights by Blondel** (the gate 5 × 0.18 on 0.275, was 6 × 0.15 on 0.22; the main hall 4 × 0.1825, was 5 × 0.15); the
  gate's sill 0.20 (was 0.25); the covered walk's columns 2.30 (was 2.10, headroom 1.88) and its lanterns hooked at 2.75;
  two steps added at each water-pavilion path end (a 0.33 edge with no step); the row-door lanterns' rods 0.40 (was
  0.52, headroom 1.93).
- **The pavilion's two open faces lose their fretwork** (it hung at 1.87 over the path); **path 0 divides round 石兄**
  (the rock stood on its centre line); the first pair of engraved stones moves 0.15 south (it overlapped the moon ring).
- **Bridge piers** stop on the shallow bed over the Classical Hall (as built they went to −0.95, into its roof).
- **Courts' paving is cut round the platforms** (it ran under them); **no invisible guards**; no glows, post-process volumes
  or per-lamp output values.
- **Materials follow the Story:** 青白石 porch paving (was 城砖), 城砖 gate hall floor (was 尺四 tiles, per the Materials
  board), bronze hardware and case rails (was brass, ruled `bronze.cusn10`), silk-gauze lanterns, bluestone pond bed (was
  dark stone), phoebe furniture.
- **Light:** listed catalogue heads (wall washers, case runs, one spot, 17 lanterns) replace the as-built soft area
  lights and window glows.
- **Ruling 1, night light:** the walks hold the ruled `garden.night` (≥ 1 lx on the centre line after dusk). The washers
  run `always` so the rooms' fronts light the courts at night (as built they lit the rooms day and night). 19 hidden
  step lights (`gap.step-light.2700k`) sit in a new low kerb on the bridge (0.10 × 0.08, as the as-built code's comment
  describes) and in the rockery's flanking stones. There is still no lantern at the gate.
- **Ruling on §13 (b), the short night walks:** 14 more hidden step lights, set flush in kerbs and edge stones at the
  fewest that meet 1 lx: 3 in path 4's kerbs, 1 at the fork round 石兄, 6 in the inner court's bed kerbs, 3 at the gate
  hall (the east drum's base, the back kerb, a back-step side stone; no lantern and nothing on or over the door) and 1 in
  the yard door's threshold. As built, none of these walks had any light.
- **Ruling 2, ground:** the ground beds are Nature's, in `accent.site-geology` (topsoil); there is no Form earth sheet.
- **Ruling 4, rooms:** the rooms' floors hold `gallery.low-light` (50 lx, for silk, paper and ink), not `circulation` (100).
- **Ruling 5, objects restored:** the fish bowl (with water), the pomegranates' tubs, the pomegranates and the desk things
  in glazed stoneware (a fired-clay inkstone); the painting table and its chairs in huanghuali, built as a recessed-leg
  画案 and a pair of rose chairs (as built phoebe, and the chairs were official's hat chairs); the Lingbi rock in Lingbi
  stone. The paper, brushes and paperweights are not built (the palette has no key for them).
- **Hang:** the Qingming's lead 0.23 (was 0.25; the mount overran its case by 1.7 cm); handscroll widths from the
  catalogue, not the images; turned vessels scaled to the catalogue's sizes, and those derived from their photographs to
  the catalogue's heights only, their widths their sections' (below).
- **The vessels from their photographs (K9):** each at its museum's size, catalogued in `MuseeVision/Data/Art/Ceramics.json`
  with its accession and its photographs' credit (the collection's `assets/collection.json` is read only): the Xuande jar
  0.483 across (was 0.460); the peach-bloom vase its true 19.7 cm (was 15.9); the chicken cup 3.7 × 8.2 at the National
  Palace Museum (was "museum collections", 8.3 wide) and its handling copy its second object; the Ru basin 6.9 high on
  its feet. **The meiping is the V&A's C.8-1952**, Yuan, 1320–50, with scenes from *The West Chamber*, at its own
  35.6 cm high (was 44.1) and 20.6 cm across (below): it stands in for the Nanjing Museum's meiping with Xiao He pursuing Han Xin by moonlight,
  of which no open image exists, and its label credits its photographers, Gary Todd and UK_FGR for Wikipedia Loves Art
  (side B is left out: below).
  **It stands 20.6 cm across, not the V&A's 22** (the V&A's record, C.8-1952, gives height 35.6 cm and diameter 22 cm,
  with no note of where either is measured). Both photographs give the narrower vase: side A's section, through the camera
  its EXIF focal length and its silhouette fix (4.1 px rms), is 20.6 cm across at 35.6 cm high (height over diameter
  1.73), and side B, an independent photograph from another camera, spans 1.77 times as high as it is wide. Held to 22 cm,
  side A's camera reaches 21.3 cm at best and its section's image misses the silhouette by 50 px rms. The section keeps
  the photographs' proportion and is turned at the V&A's height by one scale (before, it was stretched 7% across alone).
- **The Ru basin's pressing between its foot and its mouth:** its foot is pressed to the museum's 19.3 × 12.9 and its
  mouth to its 23.0 × 16.4, and the pressing runs evenly up the wall between them (pressed to the mouth's ratio all the
  way down, the foot came to 19.3 × 13.8; a wall flaring as far at the ends as at the sides would give 12.7). No
  photograph shows how the wall's flare changes round the oval between the two.
- **The Ru basin's feet** are traced from its base photograph and mirrored to the four corners, as the photograph shows
  them alike, laid in shares of the flange's fitted outline. The tracing reaches 1.066 of the flange at the corners, where
  the feet's own faces show beyond its edge; a foot stands under the base, so its outer edge is brought within the
  underside there.
- **The Xuande jar's lotus panels** are carried round only to 7.73 cm: their tips reach 9.27 cm. Taken to the tips, the
  band's measured repeat is trusted at 15.1 to 15.4 degrees (23.3 to 23.8 panels), which settles neither the 23 panels
  read on the photograph nor 24, so the band keeps its reading and its height.
- **Photographs left out:**
  - the Xuande jar's older photograph MET_DT5083: through the camera its own silhouette fixes, its painting lies 1.0 to
    3.8 cm higher than DP-32251-001's, band by band up the height (correlation 0.64 to 0.79), and a camera moved to make
    the two agree misses its silhouette by centimetres; composed, it would double the lotus panels, the dragon and the
    neck's lines. The jar is composed from DP-32251-001 alone, its far side from its bands' repeats and the glaze;
  - the meiping's side B (a close, soft gallery photograph): its camera can't be fixed (its silhouette missed by 24 px
    rms with its EXIF focal length), and its painting lies on side A's at a correlation of only 0.34; composed, it would
    double the figures where the sides meet;
  - the wucai jar's bottom view DP342699: it places the side views and measures the foot and the collar's lower line,
    and sees the lower body obliquely all round, but through the camera its rings fix its painting lies 9 mm higher than
    DP342700's in the pond (0.2 to 0.4 of the height, correlation 0.81); composed, it would double the fish and lotus
    there;
  - the falangcai bowl's black-and-white view MET_201714: it carries no colour, but it shows what no colour view does,
    the bowl's inside painted with a peony and flowering branches and a chrysanthemum on the far side of its outside.
    **The real bowl is painted inside and on its far side; here both are the glaze** (the far side where no band
    repeats), for want of a colour photograph of them;
  - the chicken cup's PNG, the same photograph as its TIFF.
- **The wucai jar's sides are placed by its bottom view:** DP342699 is taken from under the jar along its axis (its
  painted rings are round and centred on each other within a few pixels), and shows the collar's 13 clouds and the pond
  above them all round. Each side's collar and pond, laid on it by their enamels' colours, places DP342698 at 185° and
  DP342700 at 96° from DP342697 (before, they stood where a turn guessed them, at 224° and 136°). Composed wherever they
  see the jar, obliquely too, the three sides leave 7% of its outside unseen.
- **The wucai jar's collar is left in the glaze where no side view sees it:** its 13 clouds run yellow, blue, red, green
  four at a time with one red cloud more (DP342699), so no whole number of repeats carries them round (90° anticorrelates;
  120° would put clouds up to a third of a cloud off and blend them). Its lappets and neck scroll show no repeat the
  photographs trust, and are left in the glaze there too.
- **The meiping's lotus panels are left in the glaze where side A doesn't see them:** their count isn't settled. On side
  A the three panels' emblems stand 49 and 51.5° apart and its front panel's dividers 55° (7 panels are 51.4° each, 6
  would be 60°), but the panels' measured repeat, 55.4° (self-correlation 0.74), is 6.5 to the turn, halfway between the
  two; side B, whose camera can't be fixed, puts its two whole panels' emblems 67° or 43° apart through the cameras its
  silhouette allows (15 and 39 px rms), no count at all. Its diaper band and shoulder scroll show no repeat the
  photographs trust, and are left in the glaze too, on the 58% of the vase side A doesn't see.
- **The wucai jar's foot** is 11.5 cm across as its bottom view shows (the side view, whose foot is lost in its shadow, had
  made it 13.3), and the jar 23.7 cm across (was 23.3) now that the foot's front edge is read where the unglazed rim meets
  the ground (the shadow had taken the silhouette's bottom 44 px lower: the jar spanned 2% more of the photograph than it
  does, and so was turned 2% narrow).
- **Planting:** the garden's willow and pine replace the osmanthus stand-ins (plums kept); the crabapples and jujubes are
  their real species; the pomegranates stand in their tubs (ruling 5).
- **Door pins plain** (as built they carried characters) and the yard door's leaves green (Names and Plan boards).
- Planting.json, to the fields Nature reads (K3-LIGHT-NATURE): the trees give their `height`, `crownCentre` and `crownRadii` (as built, `chenghuai_works.py` TREES; the pomegranates 1.9 and 0.75 x 0.55); the bamboo and lotus give their culms' and plants' `points` (as built, y + 17); the ground beds give their soil's `outline` inside the kerbs (beds 0.14, pits 0.12), in the tubs (r 0.285), and the garden's ground (x 2.30 to 16.40, y -0.50 to -44.40), from which Nature cuts the pond. The pond's polygon stays in stage coordinates, as Nature now reads every plan point. The trees' fruit is not grown in the first pass; their wood takes `living.stem` until Matter records a bark.
