# The Hall of Light · design sheet

- **Stage key** `hall-of-light` (module `MuseeHallOfLight`, `UHallOfLightRoom : USiteRoom`). Display name "The Hall of
  Light". Era: **Modern** (*Modern · Alive*): travertine filled and polished, CuSn10 brushed, oak fumed, wire-brushed and
  oiled, pearl steel, clear low-iron laminate.
- **Sources:** D-001…D-019 (D-003, D-005, D-012, D-014, D-017, D-018 binding here); Palette.json + Story.md ("The Hall
  of Light (Modern)"); the drafter's spec `r-specs/…/Stages/hall-of-light.md` (verified, corrections in §11); the finished
  `rotunda.md` (§2, `rotunda.east`); the Atrium draft `r-specs/…/Stages/atrium.md` (§2, `atrium.west`); the board
  `plan/canvas/HallOfLight.dc.html` and `plan/README.md:107-111`; the as-built code in `windows/` (§11). Schedules:
  `Data/Stages/hall-of-light/{Finishes,Lighting,Hang,Planting}.json` (no Mechanisms: nothing moves).
- **Units** metres. **Frame:** origin on the hall's axis, on the finished floor (the Rotunda's floor level), on the
  Rotunda drum's outer face: the west port's threshold point. x east, y south, z up, true north. **Old numbers convert**
  as x = x_plan − 11.200, y = y_plan, z = z_plan. **museum = local + (11.200, 0, 0)**, yaw 0.
- **Depth d** (members along the glass walls): d in from the glass line towards the axis; north members at
  y = −(4.50 − d), south at y = +(4.50 − d). **Vault angle θ** from the crown, positive south: a point at θ and radius
  ρ is (y, z) = (ρ sin θ, 2.20 + ρ cos θ).

## 1. Identity and placement

A glass hall 9 m wide from the Rotunda's drum to the Atrium's stone base, 28.0 on the axis (29.6 along the glass),
between a birch meadow (north) and an orchard (south). Glass walls 5 m high on a travertine plinth; a shallow vault of
pearl steel and glass (R 5.30 about z 2.20, crown 7.50): the Rotunda's eye lattice drawn out along the hall. Twenty
photographs 1840–1914, one per 3 m bay per wall, forward in time walking east; the last four are autochromes against the
glass; four stereo stones stand off the axis, five flush viewing stones lie on it.

| | |
|---|---|
| Origin in the museum plan | **(11.200, 0.000, 0.000)**; yaw 0°. Site derives it from `hall-of-light.west ↔ rotunda.east` (§2); this is the value it must reproduce. The Rotunda's centre is local (−11.200, 0); the Atrium's centre local (42.800, 0) = museum (54.000, 0). |
| Level | 0 (ground floor): the Rotunda's floor, the Atrium's floor. |
| Footprint kind | Building (the hall) with two Outdoors gardens. |
| Vertical extent | Hall band (\|y\| ≤ 4.80): z −0.12 … 7.62. Gardens: z −1.00 … 12.00 (soil −0.30, birches 9.3 with sway). |

**Datums.** Glass line \|y\| 4.50; plinth top 0.30; transom 3.20 (centre); eaves box 4.86–5.10; wall glass head 4.85;
vault R 5.30 about (y 0, z 2.20), springing on the glass line at θ ±58.11° (z 5.00), crown 7.50; glass and arches bear
on the eaves' top at θ ±56.83° (z 5.10, \|y\| 4.436). **Purlin lines** θ 0°, ±14.73°, ±29.46°, ±44.19° (±44.19° is
where the vault crosses the Atrium's coping at z 6.00; the others divide it equally,
`HallOfLightStructure.cpp:93-104`); with the two eaves lines, 8 panels across.

**Rhythms** (local x; plan x = local + 11.20):

| Element | Local x |
|---|---|
| Posts k 0…9 | −0.20 + 3k: −0.20 … 26.80 |
| Arches k 0…17 | 1.30 + 1.5k: 1.30 … 26.80 (odd arches over posts, even arches over print bays 1–9) |
| Print bays 1–9; bay 10 | 1.30 + 3(b − 1): 1.30 … 25.30; **27.75** |
| Viewing stones (on the axis) | 2.80, 8.80, 14.80, 20.80, 26.80 |
| Stereo stones (x, y) | (5.80, −2.40), (11.80, +2.40), (17.80, −2.40), (23.80, +2.40), each under an arch (3, 7, 11, 15) |

**Footprint polygon** (local, clockwise on the north-up plan; arcs are vertical cylinders on Form's shared stations):
F1 (−0.300, −14.250) → F2 (29.900, −14.250) → F3 (29.900, −8.227) → arc R 15.300 about (42.800, 0) (the Atrium's
socle) → F4 (28.747, −6.050) → F5 (27.800, −6.050) (the front of the Atrium's portal, plan x 39.0) → F6 (27.800, −4.800)
→ F7 (28.800, −4.800) → arc R 14.800 about (42.800, 0) through (28.000, 0) (the east port face) → F8 (28.800, 4.800) →
F9 (27.800, 4.800) → F10 (27.800, 6.050) → F11 (28.747, 6.050) → arc R 15.300 → F12 (29.900, 8.227) → F13 (29.900,
14.250) → F14 (−0.300, 14.250) → F15 (−0.300, 4.800) → F16 (−1.081, 4.800) → arc R 11.200 about (−11.200, 0) through
(0, 0) (the west port face) → F17 (−1.081, −4.800) → F18 (−0.300, −4.800) → F1. The hall's body passes through the
Atrium's portal opening (\|y\| < 4.95 to z 5.22, then R 5.80 about z 2.20) with its as-built clearance: body \|y\| ≤ 4.80
to z 0.06 (kerbs), ≤ 4.68 to 0.30 (plinths), ≤ 4.60 to 5.10 (eaves), R 5.42 above.

| Enclosure | Kind | Bounded by |
|---|---|---|
| `hall-of-light.hall` | Enclosed | floor, plinths, wall glass, eaves, vault glass, the two end linings (the east one with its lunette). Section 60.9 m² (45.0 under the eaves + the 15.9 segment); ≈ 1,700 m³. |
| `hall-of-light.meadow` | Outdoors | the north garden: F1–F6, F18 and the kerbs' outer face y −4.80. |
| `hall-of-light.orchard` | Outdoors | the south garden, the mirror. |

**Openings (Form's one list):** Ports `hall-of-light.west`, `hall-of-light.east`; Glazed `hall-of-light.wall.north`,
`hall-of-light.wall.south` (11 panes each), `hall-of-light.vault` (152 panes), `hall-of-light.lunette` (6 panes);
Housings: the 20 step-light pockets `hall-of-light.plinth.north.h0` … `.h9`, `hall-of-light.plinth.south.h0` … `.h9`.

## 2. Ports

Frames on the port face at the threshold (z 0), on the opening's centreline, facing out of the stage, up +z. Class names
are **width × crown** (the Rotunda's convention). Mated frames coincide and face opposite ways.

| Key | Kind | Class | Local frame (position; facing) | Museum frame | Face; wall depth | Mates with | Wells |
|---|---|---|---|---|---|---|---|
| `hall-of-light.west` | doorway | `doorway.3000x6000.arched` | (0.000, 0.000, 0.000); **−x** | (11.200, 0, 0); −x | cylindrical R 11.200 about local (−11.200, 0) = the Rotunda's axis: the hall's west face is **concave** onto the drum; **0.050** (the west lining, R 11.20 → 11.25) | `rotunda.east` (museum (11.200, 0, 0), facing +x; drum 1.20). **Passage 0.00.** | none |
| `hall-of-light.east` | doorway | **`doorway.4000x4500.square`** | (28.000, 0.000, 0.000); **+x** | (39.200, 0, 0); +x | cylindrical R 14.800 about local (42.800, 0) = the Atrium's axis (museum (54, 0)): concave onto the base; **0.050** (the east lining, R 14.80 → 14.85) | `atrium.west` (Atrium frame (−14.800, 0, 0) about its centre museum (54, 0, 0) = museum (39.200, 0, 0), facing −x; base 0.80). **Passage 0.00.** | none |

- **`doorway.3000x6000.arched`** (the Rotunda's): clear 3.000 (jambs y ±1.500); semicircular head r 1.500 springing
  4.500; crown 6.000 (`MuseePlan.h:44-47`). Jambs meet the port face at x −0.101 and the lining's inner face at −0.050.
- **`doorway.4000x4500.square`** (defined here from the as-built Atrium west door, `MuseePlan.h:171`,
  `AtriumBaseStructure.cpp:66-67`): clear **4.000** (jambs y ±2.000), **square head at 4.500** (crown 4.500), jambs
  parallel, open portal. Jambs meet the port face at x 28.136 and the lining's inner face at 28.085. The Atrium draft
  declares the same key; Site's `PortClasses.json` must carry it (§13).
- **Museum check.** West: (0, 0, 0) + (11.200, 0, 0) = (11.200, 0, 0) = `rotunda.east`; facings −x / +x. East:
  (28.000, 0, 0) + (11.200, 0, 0) = (39.200, 0, 0) = 54.000 − 14.800 on the Atrium's axis; facings +x / −x.
- **Rough openings** (the hall builds only these, through its linings; allowance a = 0.050, Site's): west 3.10 wide,
  head r 1.55 about the springing 4.50 (crown 6.05); east 4.10 wide, head 4.55; both sills −0.05; faces
  `hall-of-light.port.rough-opening`. **Site's joint** builds the travertine thresholds through both wall depths (west:
  drum 1.20 + lining 0.05; east: base 0.80 + lining 0.05), the brushed CuSn10 strip on each mating cylinder across the
  threshold, the linings of the rough openings and the surrounds. No passage masonry: the faces touch. The hall builds
  nothing past R 11.20 (west) or R 14.80 (east).
- **Surround clearance (as Chenghuai's and Albion's).** The joint's surround on the hall's face of each lining may reach
  **0.35** beyond the clear opening. The hall keeps the west lining's inner face clear for **\|y\| < 1.85, z < 6.35**,
  and the east lining's inner face clear for **\|y\| < 2.35, z < 4.85**. What is nearest: the west collar's underside is
  at z ≥ 7.05 within \|y\| 1.85, arch 0 stands at x 1.30, viewing stone 1 (flush) at 2.80; at the east arch 17 stands
  at 26.80, the lunette's sill at 6.20, the autochromes at \|y\| ≥ 4.44, viewing stone 5 (flush) at 26.80. The Rotunda's
  own archivolt (0.30) is on the drum's inner face (r 10.00), the far side of the joint. The as-built plaster doorcase
  (0.278 wide, 0.094 proud, `HallOfLightStructure.cpp:644-652`) is evidence for Site's surround only.
- **Blinds (closed bronze doors).** A closed pair of cast CuSn10 leaves fills each class opening on its port face,
  meeting rail on the axis, while that port is unmated: `hall-of-light.west.blind-doors` (on the stand, or with the
  Rotunda absent) and `hall-of-light.east.blind-doors` (**tonight**, until the Atrium is built; the Atrium is specified
  after this sheet). Mated, each is an open portal (as built).
- **Topology (D-012):** a link, `rotunda.east ↔ hall-of-light.west`, `hall-of-light.east ↔ atrium.west`. No vertical
  ports, no wells. No gates to the gardens: they are seen through the glass, never entered.
- **Coordination.** Rotunda: its façade dress may run over the hall only above z 7.62 within \|y\| ≤ 4.80
  (`MuseeFacadeStructure.cpp:77-83`), and its podium and gravel stay west of x −0.30 (plan 10.90) outside \|y\| 4.80.
  Atrium: the port on the base's face r 14.800 (not the ashlar dress r 14.880); its portal's clear opening as in §1; above
  the coping (z 6.20) between the lunette (R 14.80) and the drum glass the air is outdoors, under the portal. Grounds:
  receive this footprint and lay nothing inside it.

### Anchors (the stage contract)
Wall anchors: origin on the hanging plane (z 0 = the floor) at the left end as seen by the viewer, x to the viewer's
right, y out of the plane towards the viewer, z up.

| Key | Kind | Frame (origin; axes) | Belongs to |
|---|---|---|---|
| `hall-of-light.prints.north` | Wall | (0.000, −4.350, 0); x (1, 0, 0); y (0, 1, 0) | the 8 north prints (their mounts' face plane, d 0.150) |
| `hall-of-light.prints.south` | Wall | (28.000, 4.350, 0); x (−1, 0, 0); y (0, −1, 0) | the 8 south prints |
| `hall-of-light.autochromes.north` | Wall | (0.000, −4.440, 0); axes as the north wall | the 2 north autochromes (cassette face, d 0.060) |
| `hall-of-light.autochromes.south` | Wall | (28.000, 4.440, 0); axes as the south wall | the 2 south autochromes |
| `hall-of-light.stereo.1`, `hall-of-light.stereo.2`, `hall-of-light.stereo.3`, `hall-of-light.stereo.4` | Mount | the reader plate's centre (5.800, −2.400, 1.320), (11.800, 2.400, 1.320), (17.800, −2.400, 1.320), (23.800, 2.400, 1.320); x to the viewer's right ((1,0,0) for 1 and 3, (−1,0,0) for 2 and 4); y the plate's normal, towards the axis and tilted up 28.65° ((0, ±0.878, 0.479)); z up the plate | the four stereo cards |
| `hall-of-light.meadow.ground`, `hall-of-light.orchard.ground` | Bed | the gardens' ground, z 0.00 (x 0.10 … 29.70, \|y\| 4.80 … 13.75) | the swards |
| `hall-of-light.meadow.flowers.lilac`, `hall-of-light.meadow.flowers.pink`, `hall-of-light.meadow.flowers.copper`, `hall-of-light.meadow.flowers.yellow`, `hall-of-light.orchard.bulbs.white`, `hall-of-light.orchard.bulbs.pink`, `hall-of-light.orchard.bulbs.yellow` | Bed | the plotted points (Planting.json) | meadow flowers and spring bulbs |
| `hall-of-light.birch.1`, `hall-of-light.birch.2`, `hall-of-light.birch.3`, `hall-of-light.birch.4`, `hall-of-light.birch.5`, `hall-of-light.birch.6`, `hall-of-light.birch.7`, `hall-of-light.birch.8` | Bed | ground points (§7) | the birches |
| `hall-of-light.orchard.1`, `hall-of-light.orchard.2`, `hall-of-light.orchard.3`, `hall-of-light.orchard.4`, `hall-of-light.orchard.5`, `hall-of-light.orchard.6`, `hall-of-light.orchard.7`, `hall-of-light.orchard.8`, `hall-of-light.orchard.9`, `hall-of-light.orchard.10` | Bed | ground points (§7) | the cherries and apples |
| `hall-of-light.hedge.north`, `hall-of-light.hedge.south` | Bed | the lines y ∓14.00, x −0.20 … 29.80 | the yew hedges |

No Body, Use, Water or Seat anchors; no channels. Luminaires stand at the points in Lighting.json.

## 3. Pieces by trade

Every piece meets its neighbour face to face (as built, pieces ran 1–5.5 cm into each other and into the drums: never
port those embeds). Surfaces are listed with their pieces; finishes in Finishes.json.

### 3.1 Masonry (travertine)
- **Floor** `hall-of-light.floor`: top z 0.000, bottom −0.100, between the two linings' inner faces (R 11.25 about
  (−11.20, 0); R 14.85 about (42.80, 0)) and the plinths' inner faces \|y\| 4.32. Slabs 1.20 along x × 0.80 across,
  unstaggered grid, a joint on the axis, 2.5 mm joints; vein-cut bands along the hall; cut to the linings' arcs and round
  the viewing stones. Laid by Form within travertine stock. Butts the plinths (as built it ran 2 cm under them).
- **Viewing stones** ×5 `hall-of-light.viewing-stone` at x 2.80, 8.80, 14.80, 20.80, 26.80 on the axis: a rebated disc,
  R 0.450 from −0.100 to −0.030 and R 0.400 from −0.030 to 0.000; flush. Ring `hall-of-light.viewing-stone.ring`: annulus
  R 0.400–0.450, z −0.030 … 0.000 in the rebate, flush.
- **Plinths** ×2 `hall-of-light.plinth`, on the glass line, section (d, z) closed: (−0.18, −0.10) (0.18, −0.10)
  (0.18, 0.29) (0.17, 0.30) (−0.17, 0.30) (−0.18, 0.29) (10 mm chamfers), lining to lining, trimmed to their cylinders;
  ashlar 0.40 × 0.36 × ≤ 1.80. Housings `hall-of-light.plinth.housing` at every post x: x ±0.050, z 0.110–0.170, d 0.180
  back to 0.060 (the step lights, §4).
- **Kerbs** ×2 `hall-of-light.kerb` (planter edges, outdoors), section (d, z): (−0.30, −0.12) (−0.18, −0.12)
  (−0.18, 0.06) (−0.29, 0.06) (−0.30, 0.05): \|y\| 4.68 … 4.80, 10 mm outer chamfer; butts the plinth at d −0.18 (as built
  −0.17, 1 cm into it); lining to lining; blocks ≤ 1.80.
- **West lining** `hall-of-light.lining`: between R 11.200 (the port face) and R 11.250 about (−11.200, 0); outline in
  (y, z), mirrored: (±4.800, −0.120) → (±4.800, 0.060) → (±4.680, 0.060) → (±4.680, 0.300) → (±4.600, 0.300) → (±4.600,
  5.100) → (±4.579, 5.100) → arc R 5.420 about (0, 2.200) to the crown (0, 7.620); it covers the ends of kerbs, plinths,
  eaves and vault. Less the rough opening (§2). Courses 0.60, slabs ≤ 1.80. Every hall piece at the west end bears on its
  inner face.
- **East lining** `hall-of-light.lining`: between R 14.800 (the port face) and R 14.850 about (42.800, 0), the same
  outline; less the rough opening (§2); less the **lunette** {z ≥ 6.200, r ≤ 5.190 about (y 0, z 2.200)}: above 6.20 the
  stone is a band r 5.19 … 5.42. The lunette's sill (z 6.20) clears the Atrium's coping (6.05–6.20).
- **Stereo stones** ×4 `hall-of-light.stereo-stone`, lathe (r, z): (0, 0) (0.350, 0) (0.350, 0.985) (0.335, 1.000)
  (0, 1.000): Ø 0.70 × 1.00, 15 mm top chamfer, standing on the floor (as built 1 cm into it). One block each.

### 3.2 Steelwork (pearl steel)
Walls `hall-of-light.wall.steel`; vault `hall-of-light.vault.steel`. Sections closed; points in metres.

| Piece | Section | Path and extent |
|---|---|---|
| Posts ×20 | I in (x rel. post, d): (−0.030, −0.030) (0.030, −0.030) (0.030, −0.010) (0.010, −0.010) (0.010, 0.140) (0.030, 0.140) (0.030, 0.155) (−0.030, 0.155) (−0.030, 0.140) (−0.010, 0.140) (−0.010, −0.010) (−0.030, −0.010): pressure cap outside, web through the glass line, flange inside | vertical at post x, z 0.300 → 4.860, bearing on the plinth, meeting the eaves' underside (as built 0.28 → 4.90) |
| Glazing shoes ×2 | U in (d, z): (−0.025, 0.300) (0.025, 0.300) (0.025, 0.340) (0.010, 0.340) (0.010, 0.310) (−0.010, 0.310) (−0.010, 0.340) (−0.025, 0.340) | lining to lining on the plinth top, broken by the post webs; glass on setting blocks at 0.310 |
| Transoms ×2 | d 0.012 … 0.110, z 3.170 … 3.230 | inside the glass, in segments between post webs (and lining to the end posts) |
| Eaves ×2 | (d, z): (−0.10, 4.86) (0.20, 4.86) (0.20, 5.05) (0.15, 5.10) (−0.10, 5.10): a box 0.30 × 0.24, 50 mm chamfer on the inner top arris | lining to lining; wall glass in a channel in its underside; vault glass and arches bear on its top |
| End posts ×4 | along x 0.000 … 0.070 from the lining's inner face, d −0.040 … 0.120 | z 0.300 → 4.860, fixed to the linings, holding the glass's end edges |
| Arches ×18 | I in (x rel. arch, b out from R 5.30): (−0.035, −0.150) (0.035, −0.150) (0.035, −0.130) (0.012, −0.130) (0.012, 0.025) (0.035, 0.025) (0.035, 0.045) (−0.035, 0.045) (−0.035, 0.025) (−0.012, 0.025) (−0.012, −0.130) (−0.035, −0.130) | about (y 0, z 2.20) from θ −56.83° to +56.83°, feet cut level at z 5.100 on the eaves; at x 1.30 … 26.80 (none at plan 39.5, §11) |
| Purlins ×7 lines | across ±0.030, b −0.120 … +0.015 | at θ 0°, ±14.73°, ±29.46°, ±44.19°, in segments: collar to arch 0, between arch webs, arch 17 to the east collar |
| Crosses 17 bays × 8 panels = 136 | bar A ±0.015 × b −0.080 … −0.020; bar B ±0.015 × b −0.090 … −0.030 | in each panel between adjacent arches and adjacent purlin/eaves lines (eaves at ±56.83°), a diagonal node to node on the vault surface, 0.025 short of each node; A runs through, B is cut and welded to A at the crossing. No crosses in the two end bays (as built). |
| Collars ×2 | along x 0.000 … 0.080 in from the lining face, b −0.110 … −0.012 | swept θ −56.83° → +56.83° on each lining; the vault glass's end edge rests on it; one section at the east |
| Lunette mullions ×5 | T: along x 0.000 … 0.100 from the east lining face, across ±0.030 | vertical at y 0.000, ±1.348, ±2.607 (the purlins' y), from the sill 6.200 to the collar's underside |

### 3.3 Glazing (Glazed openings; D-014)
All panes low-iron laminate 8 + 1.52 UV-filtering PVB + 8 (17.52 mm); outer faces take the museum's weather.
- `hall-of-light.wall.north`, `hall-of-light.wall.south` → `hall-of-light.wall.glass` (toughened): centred on the glass
  line (d ±0.0088), z 0.310 → 4.850, one pane per post bay, 11 per wall: the west end bay (end post to post 0, ≈ 0.60),
  nine of 2.970 (3.000 less the web and gaskets) and the east end bay (post 9 to the end post, ≈ 1.76). 22 panes,
  ≈ 268 m². The transom bears on the inner face through a gasket.
- `hall-of-light.vault` → `hall-of-light.vault.glass` (heat-strengthened, hot-bent to R 5.30): one pane per panel,
  8 across × 19 along (17 crossed bays + 2 end bays, cut to the linings' cylinders) = 152 panes, ≈ 300 m². Panes 1.466
  along x; 1.169 (eaves to ±44.19°) or 1.363 around the arc.
- `hall-of-light.lunette` → `hall-of-light.lunette.glass`: 6 flat panes in the east lining, faceted to R 14.85 (chord
  ≤ 15 mm), between the mullions, the sill 6.20 and the collar; ≈ 5.4 m². Through it the Atrium's misty drum glass
  (r 14.0) shows 0.8 m beyond, as built.

### 3.4 Bronze (CuSn10, brushed)
- **Print stems** ×32 `hall-of-light.print.stem` (2 per print, bays 1–8, both walls): flat bar x ±0.015, d 0.098 …
  0.120, z 0.320 → 2.000 at bay x ± 0.300; the mount's back (d 0.120) bears on them (as built a 1 cm gap). **Shoes** ×32
  (same surface): x ±0.035, d 0.080 … 0.128, z 0.300 … 0.320 on the plinth top.
- **Autochrome glazing bars** ×8 `hall-of-light.autochrome.bar`: x ±0.0125, d 0.035 … 0.055 at bay x ± 0.560, z 0.310
  → 3.170 (shoe to transom), bays 9 and 10.
- **Stereo readers** ×4 `hall-of-light.stereo.reader`: plate 0.680 × 0.400 × 0.030, centre z 1.320 over the stone's
  centre, facing the axis (stones 1, 3 face south; 2, 4 north), tilted back 0.5 rad (28.65°); stem Ø 0.030 from the stone
  top (1.000) into the plate's back, 0.016 behind the stone's centre on the plate's back side.

### 3.5 Art's objects at the anchors (fit only; §8 gives the rules)
Art builds them: print mounts 1.000 × 0.900 × 0.030 `hall-of-light.mount` (face d 0.150, back 0.120, z 1.15–2.05); the
22 mm frames `hall-of-light.frame.oak` (round each print's mount, outer 1.044 × 0.944; round each autochrome's mat);
glazing `hall-of-light.frame.glazing` (also over the stereo cards); the autochromes' cassettes (d 0.020 … 0.060, between
the glazing bars) with white window mats `hall-of-light.autochrome.mat` and a diffusing pane behind the plate
`hall-of-light.autochrome.backing`. Blind doors (§2): `hall-of-light.west.blind-doors`, `hall-of-light.east.blind-doors`.

### 3.6 Orders, mouldings, stairs
None: no stair, dome, niche, column or order (the posts are steel). The only mouldings are arris chamfers (plinth and kerb
10 mm, eaves 50 mm, stereo drum 15 mm), private to this stage. The doorcases are Site's.

### 3.7 Finishes (summary; Finishes.json holds the notes)

| Surfaces | Material · working · history |
|---|---|
| `hall-of-light.floor`, `hall-of-light.viewing-stone` | `stone.travertine-romano` · filled.polished · foot-worn |
| `hall-of-light.plinth`, `hall-of-light.lining` | `stone.travertine-romano` · filled.polished · new |
| `hall-of-light.stereo-stone` | `stone.travertine-romano` · filled.polished · handled |
| `hall-of-light.kerb` | `stone.travertine-romano` · honed · weathered (outdoors) |
| `hall-of-light.port.rough-opening`, `hall-of-light.plinth.housing` | `stone.travertine-romano` · sawn · new (hidden) |
| `hall-of-light.wall.steel`, `hall-of-light.vault.steel` | `white.pearl-steel` · pearl.satin · new |
| `hall-of-light.wall.glass`, `hall-of-light.vault.glass`, `hall-of-light.lunette.glass` | `glass.low-iron` · toughened / heat-strengthened, laminated, UV-filtering · weathered |
| `hall-of-light.viewing-stone.ring` | `bronze.cusn10` · brushed · foot-worn |
| `hall-of-light.print.stem`, `hall-of-light.autochrome.bar`, blind doors | `bronze.cusn10` · brushed · new |
| `hall-of-light.stereo.reader` | `bronze.cusn10` · brushed · handled |
| `hall-of-light.frame.oak` | `oak.sessile-french` · fumed.wire-brushed.oiled · new |
| `hall-of-light.mount`, `hall-of-light.autochrome.mat` | `accent.rag-board` · faced / cut.bevelled · new (§13) |
| `hall-of-light.autochrome.backing` | `glass.low-iron` · acid-etched · new |
| `hall-of-light.frame.glazing` | `glass.low-iron` · laminated.anti-reflective · new |

The joint's thresholds, strips, linings and surrounds are Site's (travertine honed, brushed CuSn10).

## 4. Light

**Daylight** (Lighting.json `daylight`; every opening weather **museum**, control none; no blinds, D-014)

| Opening | Glazing | Notes |
|---|---|---|
| `hall-of-light.wall.north`, `hall-of-light.wall.south` | `glass.low-iron`, toughened laminated UV-filtering | the gardens behind the works |
| `hall-of-light.vault` | `glass.low-iron`, heat-strengthened laminated UV-filtering, clear | the sky; the lattice draws its shadow on the floor (board) |
| `hall-of-light.lunette` | `glass.low-iron`, laminated UV-filtering | the Atrium's misty drum beyond |

**Care against daylight (ruled; D-014 "met by the hang and by UV-filtering glazing, as real museums do").** The UV limit
is met: every glass daylight crosses to a work filters UV (the laminate, then the frames' and readers' glazing). The lux
and dose limits cannot be met by day in a glass hall by any glazing that keeps it a glass hall. Under the idealised sun
(elevation 71.489°, bearing 90° + 15°·(h − 6)) the sun reaches the **north prints' faces** through the vault's north
half (θ −27° … −58°) all day but at 06:00 and 18:00 (≈ 0.32 of the normal irradiance at noon); the **stereo cards** on
stones 1 and 3 through the crown 08:07–15:53 and on stones 2 and 4 through the south rows (the plates tilt up to the
sun); the south prints never (they face north; their mounts' backs take the sun); diffuse sky alone puts thousands of lux
on every work. So the hall **hangs exhibition facsimiles** at true size of all 24 works (pigment prints on cotton rag for
the prints and stereographs, duplicate transparencies for the autochromes), credited to the originals, which stay with
their holders: the practice of museums that show photographs in daylight. The catalogue ids and the originals' care
classes stay (Hang `reproduction`: `exhibition-facsimile`); Art.WithinCare measures and reports the hall's daylight as a known
finding against them, and Art sets the facsimiles' replacement from the measured dose. Nothing is dimmed or shaded.
Under the `ephemeris` sun (a city's low winter sun) the south glass lets the sun onto the north wall too: reported alike.

**Luminaires** (Lighting.json; 60; points and aims local; no output values are set here)

| Ids | Type | Count | Point and aim | Operation |
|---|---|---|---|---|
| `print.n1`…`n8`, `print.s1`…`s8` | `track-framing.3000k.small` | 16 | on a short track under the inner flange of arch 2(b − 1) (flange z 7.23): (x_b, ∓1.10, 7.10); aim the print's centre (x_b, ∓4.35, 1.60): 30° from vertical, throw 6.4; shutters framed to the frame (1.044 × 0.944) | presence |
| `stereo.1`…`stereo.4` | `track-framing.3000k.small` | 4 | under arches 3, 7, 11, 15 across the axis: (x_s, −sign(y_s)·1.19, 7.08); aim the plate's centre (x_s, y_s, 1.32), ≈ 29° off its normal; framed to the plate | presence |
| `uplight.n0`…`n9`, `uplight.s0`…`s9` | `track-spot.3000k.38deg` | 20 | on the eaves' top at its inner edge (hidden from below by the eaves box, as built), (0.55 + 3j, ∓4.25, 5.18), midway in every other arch bay; aim across the vault (x, ±1.00, 7.40): the far half's arches and purlins | dusk |
| `step.n0`…`n9`, `step.s0`…`s9` | `gap.step-light.2700k` (ruled) | 20 | in the housings under each post (never under a print): (−0.20 + 3k, ∓4.38, 0.14), louvre flush with the plinth's face; aim nearly level across the floor (x, ±2.00, 0) | dusk |

No light on the autochromes: they are transparencies, lit through their diffusing backing by the garden's daylight; a
front light would only light their glass. After dusk they are dark, as a plate against a night garden is. The as-built
3800 K night lights and 4000 K accent spots at 70 lx are not ported.

**Targets**

| Surface | Class | Lux | Source |
|---|---|---|---|
| `hall-of-light.mount` (the 16 prints) | works-on-paper | 50 | Care |
| `hall-of-light.stereo.reader` (the 4 cards) | works-on-paper | 50 | Care |
| `hall-of-light.autochrome.mat` (the 4 autochromes) | works-on-paper.fugitive (15,000 lx h a year) | 50 | Care |
| `hall-of-light.floor` | gallery.low-light (ruled: the hall holds 50 lx works); after dusk | 50 | UseTargets |

No walked line falls below `escape-route`. **Design order** (Lighting.json `design`; measured, never tuned): by night the
washed pearl lattice is the brightest surface, `hall-of-light.vault.steel` ≥ 1.5 × `hall-of-light.floor`; each framed
print ≥ the floor. By day the sky through the vault ≥ the gardens through the walls; the autochromes glow by transmitted
light above the prints' mounts.

## 5. Time and weather
- Every Glazed opening takes the **museum** observer (D-005: remote weather is only the Salon's and Albion's); the gardens
  are Outdoors under the same weather. Rain runs on the vault's and walls' outer faces and falls in the gardens; snow holds
  on the vault's flatter crown and slides from its flanks (to 57°), lies on the kerbs and hedge tops (Matter's surface law,
  Nature). Sealed inside: no wind, no rain.
- **Sun** (D-003, idealised): elevation 71.489°, 06:00–18:00. The hall runs east–west: at 06:00 and 18:00 the sun is on
  the axis and the arches' shadows fall across the floor shifted 0.335 × height along it; at 12:00 the arches' planes
  hold the sun (each shadow straight under its arch) and the purlins' shadows shift north 0.335 × height: the lattice
  draws a grid on the floor and on the north wall. Under the `ephemeris` model (the city key) the noon sun in Beijing runs
  26.6° (winter) to 73.6° (summer).
- **Season:** phenology by solar term (§7). The hour key and the city key move the shadows (global).

## 6. Motion
None: no mechanisms, no coupling, one configuration. Both ports are open portals when mated; the blinds of §2 are static.

## 7. Nature (Planting.json)
**Ground** (`hall-of-light.meadow.ground`, `hall-of-light.orchard.ground`): flat at z 0.00, soil to −0.30
(`accent.site-geology`, topsoil), the kerbs 60 mm proud; swards of *Festuca rubra* with meadow grasses, x 0.10 … 29.70,
\|y\| 4.80 … 13.75 (as built densities 2.5 north, 1.5 south), cut by the footprint at the east; holes 0.30 round each
birch, 0.35 round each fruit tree. No water.

| Beds | Species | Points (local) | Height · crown centre · radii (across, vertical) |
|---|---|---|---|
| `hall-of-light.birch.1` … `.8` | *Betula pendula* | (2.80, −8.00) (5.30, −11.50) (9.80, −7.20) (13.30, −11.20) (16.80, −8.30) (20.80, −11.60) (24.30, −7.60) (27.30, −11.00) | 9.3 · 6.8 · 1.9, 2.5 |
| `hall-of-light.orchard.1` … `.10` | *Prunus avium* (odd), *Malus domestica* (even) | (3.30, 7.80) (9.30, 7.80) (15.30, 7.80) (21.30, 7.80) (26.80, 7.80) (4.80, 11.60) (10.80, 11.60) (16.80, 11.60) (22.80, 11.60) (28.30, 11.60) | 6.7 · 4.9 · 2.0, 1.8 |
| `hall-of-light.hedge.north`, `.south` | *Taxus baccata*, clipped 0.45 × 1.50, batter 0.02, shoulder 0.06, core inset 0.065, open ends | y ∓14.00, x −0.20 … 29.80 | 1.50 |
| `hall-of-light.meadow.flowers.lilac` / `.pink` / `.copper` / `.yellow` | *Knautia arvensis* (41) / *Malva moschata* (41) / *Helenium autumnale* (53) / *Ranunculus acris* (35) | as-built plotted points, x − 11.20 | 0.4–0.9 |
| `hall-of-light.orchard.bulbs.white` / `.pink` / `.yellow` | *Narcissus poeticus* (60) / *Tulipa saxatilis* (44) / *Narcissus pseudonarcissus* (46) | as-built plotted points, x − 11.20 | 0.20–0.35 |

Growth spaces are the garden volumes less the sway at the design wind, off the eaves (\|y\| 4.60) and the vault: the
nearest birch clears the eaves by 0.70; `hall-of-light.orchard.10`'s crown meets the footprint's east edge (x 29.90) and
grows narrower there (the rule, not a patch). Seeds from each key. **Phenology** (as built, `HallOfLight.swift:232-263`):
birches leaf in spring, gold in autumn, bare in winter; cherry blossom white and early, apple pink-white and later; fruit
in summer and autumn; the meadow flowers bloom in summer, half in autumn, seed heads in winter; the bulbs in spring only;
the yew evergreen. The museum's wind moves trees, flowers and grass.

## 8. Art
**Sources:** ids verbatim from `assets/collection.json` (Hang `source`); sizes from the holding museums' records as the
draft read them (Met, Getty, AIC and LoC records; cited per work in Hang.json `note`), cross-checked against the
proportions of `assets/paintings/<id>.jpg`; the hang order and bays from `HallOfLight.swift:198-217` and the board. Works
are hung as **exhibition facsimiles at true size** (§4); the as-built scaling to 0.78 × 0.68 is not ported.

**Hang rule** (board; `HallOfLight.swift:116-154`; `frames.py:53-55, 381-390`): one work per wall per bay, west to east
in time; centre line z **1.600**. Bays 1–8: the print at true size centred on its pale mount 1.000 × 0.900 (z 1.15–2.05,
face d 0.150, back d 0.120 on two bronze stems at bay x ± 0.30), the 22 mm fumed-oak frame round the mount (outer 1.044 ×
0.944), glazed; from the garden one sees its backing. Bays 9–10: the autochrome at true size in a white window mat 1.000
× 0.900 in the same frame, in a cassette against the wall glass (d 0.020–0.060) between the bronze glazing bars, a
diffusing low-iron pane behind it. `along`/`height` place the work's lower-left corner on its Wall anchor: N / S =
`hall-of-light.prints.north` / `hall-of-light.prints.south`; N (glass) / S (glass) = `hall-of-light.autochromes.north` /
`hall-of-light.autochromes.south`. Frame `hall-of-light.frame.oak` for all twenty.

| Bay · x | Wall | Work (id) · title · maker, date · medium | w × h | along · height | Care |
|---|---|---|---|---|---|
| 1 · 1.30 | N | photo-talbot-leaf · *A leaf* · Talbot, c. 1840 · salted paper | 0.097 × 0.080 | 1.252 · 1.560 | works-on-paper |
| 1 · 1.30 | S | photo-talbot-open-door · *The Open Door* · Talbot, 1844 · salted paper | 0.194 × 0.143 | 26.603 · 1.529 | works-on-paper |
| 2 · 4.30 | N | photo-atkins-algae · *Delesseria hypoglossum* · Atkins, 1843–53 · cyanotype | 0.200 × 0.253 | 4.200 · 1.474 | works-on-paper |
| 2 · 4.30 | S | photo-atkins-algae-2 · *Halydrys siliquosa* · Atkins · cyanotype | 0.200 × 0.253 | 23.600 · 1.474 | works-on-paper |
| 3 · 7.30 | N | photo-hill-adamson-newhaven-fishwives · *Newhaven Fishwives* · Hill and Adamson, c. 1845 · salted paper | 0.217 × 0.295 | 7.191 · 1.453 | works-on-paper |
| 3 · 7.30 | S | photo-fenton-valley-shadow-of-death · *The Valley of the Shadow of Death* · Fenton, 1855 · salted paper | 0.360 × 0.280 | 20.520 · 1.460 | works-on-paper |
| 4 · 10.30 | N | photo-nadar-bernhardt · *Sarah Bernhardt* · Nadar, 1864 · gelatin silver | 0.162 × 0.211 | 10.219 · 1.495 | works-on-paper |
| 4 · 10.30 | S | photo-le-gray-great-wave · *The Great Wave, Sète* · Le Gray, 1857 · albumen | 0.414 × 0.337 | 17.493 · 1.431 | works-on-paper |
| 5 · 13.30 | N | photo-marville-rue-traversine · *Rue Traversine* · Marville, c. 1868 · albumen | 0.275 × 0.348 | 13.163 · 1.426 | works-on-paper |
| 5 · 13.30 | S | photo-cameron-julia-jackson · *Julia Jackson* · Cameron, 1867 · albumen | 0.206 × 0.274 | 14.597 · 1.463 | works-on-paper |
| 6 · 16.30 | N | photo-muybridge-horse · *The Horse in Motion* · Muybridge, 1878 · albumen on card | 0.220 × 0.140 | 16.190 · 1.530 | works-on-paper |
| 6 · 16.30 | S | photo-osullivan-canyon-de-chelly · *Ancient Ruins in the Cañon de Chelle* · O'Sullivan, 1873 · albumen | 0.204 × 0.280 (unverified) | 11.598 · 1.460 | works-on-paper |
| 7 · 19.30 | N | photo-marey-bird-in-flight · *Bird in Flight* · Marey, 1886 | 0.173 × 0.033 | 19.213 · 1.584 | works-on-paper |
| 7 · 19.30 | S | photo-atget-organ-grinder · *Organ Grinder* · Atget, 1898–99 | 0.178 × 0.230 | 8.611 · 1.485 | works-on-paper |
| 8 · 22.30 | N | photo-stieglitz-steerage · *The Steerage* · Stieglitz, 1907 · photogravure | 0.264 × 0.334 | 22.168 · 1.433 | works-on-paper |
| 8 · 22.30 | S | photo-hine-sadie-pfeifer · *Sadie Pfeifer* · Hine, 1908 | 0.178 × 0.127 (unverified) | 5.611 · 1.537 | works-on-paper |
| 9 · 25.30 | N (glass) | photo-lumiere-autochrome · *Peacock* · Lumière, c. 1907 · autochrome | 0.177 × 0.130 | 25.212 · 1.535 | works-on-paper.fugitive |
| 9 · 25.30 | S (glass) | photo-lumiere-autochrome-4 · *Brest, the naval port* · Gervais-Courtellemont, c. 1912 · autochrome | 0.120 × 0.090 (unverified) | 2.640 · 1.555 | works-on-paper.fugitive |
| 10 · 27.75 | N (glass) | photo-lumiere-autochrome-3 · *Serbian women in festival dress* · Léon, 1913 · autochrome | 0.120 × 0.090 | 27.690 · 1.555 | works-on-paper.fugitive |
| 10 · 27.75 | S (glass) | photo-lumiere-autochrome-2 · *Ann Murdock with Buzzer IV* · Genthe, 1914 · autochrome | 0.178 × 0.127 | 0.161 · 1.537 | works-on-paper.fugitive |

**Stereographs** on the Mount anchors (Hang `wall` `hall-of-light.stereo.reader`, `centre` = the plate's centre, tilt
28.65°): the card whole and centred on the plate, glazed. stereo-1 · *Universal Exposition of 1855, interior* · stereo
daguerreotype · 0.171 × 0.086 on `hall-of-light.stereo.1` (faces south); stereo-2 · *Crystal Palace: Una and the Lion* ·
London Stereoscopic Co. · stereo daguerreotype · 0.171 × 0.083 on `hall-of-light.stereo.2` (north); stereo-3 · *American
Falls and Niagara River* · Stacy · albumen stereograph · 0.171 × 0.081 (unverified) on `hall-of-light.stereo.3` (south);
stereo-4 · *Boulevard des Italiens* · B.K. éditeur · albumen stereograph · 0.171 × 0.084 on `hall-of-light.stereo.4`
(north). Care works-on-paper (the daguerreotypes on their card mounts take the most sensitive component; flagged).

**Care classes (Care.json by medium):** photographs `works-on-paper` (CCI's 50 lx row names photographs); the autochromes
`works-on-paper.fugitive`: an autochrome's image is a screen of potato-starch grains dyed with highly fugitive dyes, and
that class's media list ("colorants known to be highly fugitive") fits; not `ceramic-and-glass`, whose media are fired
and irresponsive (flagged, §13). Catalogue notes for Art: collection.json calls Atget "albumen" and Marey
"chronophotograph"; the holders' records say gelatin silver printing-out paper and albumen. No labels on the glass;
captions on gaze. No casts, scans or cases.

## 9. Sound
- **Acoustics** from Matter, never tuned: `hall-of-light.hall` ≈ 1,700 m³ of polished travertine, ≈ 575 m² of
  laminated glass and pearl steel, no absorbers: live at mid frequencies, shorter in the bass where the big panes absorb.
- **Emitters:** footfalls on polished travertine and the bronze rings; rain on the vault and walls (≈ 575 m² of exposed
  glass) from the museum's weather, the hall's signature; wind in the birches and fruit trees outside. Birdsong by the
  museum's city (polish). The glazed shell occludes the gardens; the two portals pass sound to the Rotunda and the Atrium.

## 10. Visitor
- **Walk** the axis west to east from the Rotunda, forward in time from Talbot to Genthe, between the gardens; the Atrium
  door ahead ("the way on").
- **Stop** on the five flush viewing stones (step-to landings in the headset); **look** at a print, an autochrome or a
  stereo card: its caption (title, maker, date, medium, holder; "exhibition facsimile after the original in …").
- **Nothing to sit on, nothing to operate**, as built. The gardens are seen, never entered; the glass is solid to the body
  and step-to never lands beyond it, nor on a stereo stone, plinth or kerb.

## 11. The old-code map
Paths relative to `windows\` unless noted; **x − 11.200** throughout. Re-express, never copy.

| Part | Where |
|---|---|
| Hall numbers (half-width, eaves, plinth, vault, transom, posts, arches, stones, bays, mounts) | Source/MuseeVision/Plan/MuseePlan.h:258-299 |
| The Rotunda's doors; the Élan's door 4 × 4.5; the portal; the Atrium floor; HallKerb 4.8 | …/Plan/MuseePlan.h:30-47, 171, 247-255, 327-331, 533 |
| The hall in words | Source/MuseeVision/HallOfLight/HallOfLightStructure.h:12-41 |
| Embeds (dropped), floor edges, eaves/transom/shoe, light positions | …/HallOfLightStructure.cpp:29-68 |
| Neighbour faces, vault point, springing and coping angles, purlin and strip angles, post and arch x | …/HallOfLightStructure.cpp:76-127 |
| Sweep, I-section, lathe, along-side, upright helpers (Form's kernel replaces them) | …/HallOfLightStructure.cpp:184-279 |
| Floor, viewing stones, rings | …/HallOfLightStructure.cpp:292-456 |
| Plinths and kerbs | …/HallOfLightStructure.cpp:459-472 |
| Wall and vault glass (the coping split dropped) | …/HallOfLightStructure.cpp:475-527 |
| Posts, transoms, shoes, eaves, end posts | …/HallOfLightStructure.cpp:530-559 |
| Arches, purlins, crosses, collars | …/HallOfLightStructure.cpp:562-639 |
| Door architraves (evidence for Site) | …/HallOfLightStructure.cpp:642-693 |
| Stereo drums and stems; print stems and shoes; glazing bars; louvres | …/HallOfLightStructure.cpp:698-746 |
| Uplight and path-light aims; night level; materials; lamps (3800 K: not ported) | …/HallOfLightStructure.cpp:769-796, 845-851, 904-929, 994-1021 |
| Python port and watertightness checks (the embed rule) | …/HallOfLight/check_hall_of_light.py |
| Drum skin kept clear of the hall | Source/MuseeVision/Facade/MuseeFacadeStructure.cpp:77-83 |
| Atrium west door | Source/MuseeVision/Atrium/AtriumBaseStructure.cpp:66-67 |
| Thresholds (now Site's) | Source/MuseeVision/Furniture/MuseeFurniture.cpp:301-311 |
| Hedges | Source/MuseeVision/Facade/MuseeLandscape.cpp:307-310; Nature/MuseeHedge.cpp:38-39 |
| Plan, prints and mounts, stereo reader (tilt 0.5 rad), hang order, gardens and phenology | repo Shared/Wings/HallOfLight.swift:10-35, 116-154, 156-195, 198-217, 220-266 |
| Flower points; birches; bark and leaf looks; tree sizes and meadow bounds | Scripts/nature.py:922-930, 849-850, 774-802, 1089-1127 |
| Photograph frames (22 mm, mount 1.0 × 0.9, autochrome mats) | Scripts/frames.py:53-55, 381-390 |
| Accent rule (4000 K, 70 lx, 2.4 m above: superseded) | Scripts/gallery_lights.py:27-39, 286-308 |
| Polished travertine (slab 0.8 × 1.5 aspect, joints 2.5 mm); pearl steel (#ECE9E3) | Scripts/materials.py:1043-1057, 1330-1332; Scripts/native.py:161-176 |
| Catalogue, image sources | repo assets/collection.json; assets/art_sources_extra.json; assets/paintings/ |
| Board and words | repo plan/canvas/HallOfLight.dc.html; plan/README.md:107-111 |

**Corrections to the draft** (`r-specs/…/Stages/hall-of-light.md`):
1. **West class** `doorway.3000x6000.arched` (width × crown; draft `doorway.3000x4500.arched`), as the Rotunda's sheet;
   east class `doorway.4000x4500.square` kept (width × crown and width × height agree). Frames, faces and the translation
   (11.200, 0, 0) verified; museum coordinates of the east port (39.200, 0, 0) added for the Atrium.
2. **Surround clearance** stated in the Chenghuai/Albion convention (±0.35 beyond the clear opening on each lining's
   inner face); the draft had none.
3. **Materials are palette keys** (D-018; draft substance.working.history): plinth, linings and stereo stones travertine
   **filled and polished** (draft honed; the modern working); kerbs honed, weathered; all bronze **brushed** (draft
   cast/patinated); frames fumed, wire-brushed, oiled oak; mounts and mats `accent.rag-board`; the autochromes' backing
   **acid-etched low-iron** (draft opal glass, not in the modern palette).
4. **Light:** every luminaire a catalogue type or ruled gap; 3000 K gallery light (draft 4000 K uplights and path lights);
   uplights `track-spot.3000k.38deg` on the eaves' top as built (draft `track-wallwasher.4000k` on brackets 0.20–0.40
   out); path lights the ruled `gap.step-light.2700k` (draft a new `step-light.4000k.louvred`); prints and stereo cards
   **framing projectors** (draft 12° spots, whose 1.4 m pools light the glass behind the mounts); the floor's class
   `gallery.low-light` (draft circulation / circulation.night).
5. **Floor** slabs 1.20 **along** the hall × 0.80 across (draft 1.20 across), so the vein-cut bands run along the hall
   (Story); joints 2.5 mm confirmed (`materials.py:1330`).
6. **Open decisions ruled:** care against daylight → exhibition facsimiles (§4); autochromes by night → dark, no front
   light, no backlight; the draft's deltas (linings, lunette, 19th arch) kept.
7. **Hang** as `along`/`height` on four Wall anchors (autochromes on their own glass-plane anchors), frames as a Finishes
   surface, stereo cards on Mount anchors; sizes cross-checked against the image proportions (Muybridge landscape
   confirmed; stereo-3 8.1 × 17.1 from its 2.107:1 image; Brest a 9 × 12 plate by its 1.339:1 image).
8. **Verified unchanged:** every section point list (plinth, kerb, posts, shoe, transom, eaves, end posts, arches,
   purlins, crosses, collars), datums and angles (58.11°, 56.83°, 44.19°), rhythms, stereo and viewing stones, the
   print stems' fix (d 0.120, as built 0.110), the stems' shoes, glazing bars, stereo reader, the footprint vertices
   (F3, F4, F7, F16 on their arcs), trees, hedges, flower points, the hang order and the uplight and path-light positions.

## 12. The first-pass cut
**Tonight:** Site (frame, footprint, both ports with classes and rough openings, the east blind doors, the three
enclosures and their openings); masonry (floor laid 1.20 × 0.80, five viewing stones with rings, plinths with 20
housings, kerbs, both linings with the lunette opening, four stereo stones); all steel of §3.2 in pearl; all glazing of
§3.3 under the museum's weather; the 60 luminaires and daylight; Art (16 prints on mounts and stems, framed and glazed; 4
autochromes in cassettes; 4 cards on the readers; captions); Nature (ground and swards, 18 trees, two clipped hedges,
phenology); acoustics, footfalls, rain on the glass.

**Waits for polish:** the flower and bulb beds if Nature's instanced beds aren't ready (the swards carry the gardens);
birdsong; a true stereoscope (each eye its view in the headset, instead of the flat card); rivulets and snow-slide on the
vault; per-pane glass variation; the Rotunda's dress over the vault and the Atrium's portal dress (theirs).

## 13. Palette keys to confirm and catalogue gaps
**Palette keys**
- `accent.rag-board` → **acid-free 100 % cotton museum board** (4-ply, buffered, e.g. Rising Museum Board), faced on a
  rigid aluminium-composite core for the prints' 1.0 × 0.9 mounts (`hall-of-light.mount`) and bevel-cut as the
  autochromes' window mats (`hall-of-light.autochrome.mat`). The palette has no paper.
- Guessed roles or workings outside `workingsByEra` "modern": `glass.low-iron` **acid-etched** as the autochromes'
  diffusing backing (a classical/future working; the Story's opal glass is Lamberts, absent in the modern);
  `glass.low-iron` laminated.anti-reflective for the frame and reader glazing (roles name cases); travertine
  **filled.polished** on the stereo stones (palette "modern", as ruled; the Story's room table says honed) and on the
  linings (roles name floor, plinth, stereo stones); travertine honed (outdoors) for the kerbs, sawn for hidden faces;
  `bronze.cusn10` brushed for the viewing-stone rings and the blind doors (roles by analogy).
- Workings written as compound strings: `filled.polished`, `pearl.satin`, `toughened.laminated.uv-filtering`,
  `heat-strengthened.laminated.uv-filtering`, `laminated.anti-reflective`, `fumed.wire-brushed.oiled`, `faced`,
  `cut.bevelled`, `acid-etched`.
- **Care flags:** autochromes as `works-on-paper.fugitive` (colorant, not support); stereo daguerreotypes as
  `works-on-paper`; Care.json has no class or flag for an **exhibition facsimile** (Art's record gap).

**Catalogue gaps** (Light): none new. The step lights use the ruled `gap.step-light.2700k` (a louvred, hidden-source step
light recessed in stone, bronze faceplate ≈ 0.10 × 0.06).

**Site:** `PortClasses.json` needs `doorway.3000x6000.arched` and `doorway.4000x4500.square` (clear 4.000, square head
4.500, open portal; the Atrium declares the same key).

## Build notes
- **Frame and translation.** Origin at the west port's threshold point on the axis; **museum = local + (11.200, 0, 0)**,
  yaw 0; every old number x − 11.2. The east port lands at museum (39.200, 0, 0).
- **Ports.** Both faces are cylinders about the neighbours' axes (R 11.20 about local (−11.20, 0); R 14.80 about (42.80, 0)),
  concave onto the drum and the base. Build only the rough openings through the 50 mm linings, nothing past the port
  faces; keep the linings' inner faces clear for \|y\| < 1.85, z < 6.35 (west) and \|y\| < 2.35, z < 4.85 (east). The joint
  builds thresholds, strips, linings and surrounds. East blind doors tonight.
- **Draw by rule:** the vault and every lattice member about (y 0, z 2.20) at R 5.30 + b; the purlin angles from the
  coping angle (44.19°) in thirds; the crosses node to node on the vault; the linings' outline R 5.42; panes one per
  panel.
- **Joints, not embeds:** glass into plinth and eaves, posts into plinth, transoms and purlins through webs, kerb into
  plinth, floor under plinth, crossing bars, stereo drums into the floor: all now butt or welded (Form.NoOverlap).
- **Art's anchors:** prints on the mounts' plane (\|y\| 4.350), autochromes on the cassette plane (\|y\| 4.440), stereo
  cards on the four reader Mounts. Luminaire fittings are Light's.
- **Nature owns** the gardens inside the footprint (ground, trees, hedges, beds); the Grounds lay nothing inside it.

**Departures from as-built** (for the owner's review):
- **Stage model:** two 50 mm travertine end linings (as built the hall ran 1.4–5.5 cm into the drum and the base); the
  19th arch (plan 39.5) dropped, as it crossed the east port face above z ≈ 6.3; the vault over the Atrium's coping
  replaced by a glazed lunette in the east lining (the misty drum still seen through it); every embed a real joint.
- **Care (ruled):** the hall shows **exhibition facsimiles** of its 24 works, credited to the originals: no photograph
  original survives a glass hall's daylight (direct sun on the north wall and the stereo cards; thousands of lux of
  sky everywhere); care by the hang, UV by the glazing (D-014); nothing dimmed or shaded.
- **Works at true size** (as built scaled up to fill 0.78 × 0.68) on the unchanged 1.0 × 0.9 mounts.
- **Materials follow the Story:** plinth and stereo stones filled and polished (as built honed); bronze brushed (as built
  statuary patina); frames fumed, wire-brushed oak (as built "dark wood"); mounts museum board; the autochromes backed by
  a diffusing pane; the doorcases the joint's travertine (as built plaster architraves).
- **Light:** 3000 K framing projectors at 50 lx on the prints and cards (as built 4000 K spots at 70 lx); 3000 K
  uplights (as built 3800 K cones) and the ruled 2700 K step lights (as built 3800 K); no light on the autochromes (as
  built each had an accent spot): dark after dusk.
- **Floor** laid in slabs 1.20 along the hall (as built a shader's grid of unstated direction).
