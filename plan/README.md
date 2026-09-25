# The plan

A snapshot of the Musée Vision design canvas, taken on 2026-09-25 (canvas version `1790355116-aa5a`),
plus three standalone design canvases from the same day (`proposals/`).
The live canvas is still where the design is edited:
https://claude.ai/artifact/5KS3Zkfoub5zBdopET1Qc6. When a board changes there, refresh this folder
from it so every build (iPhone, Windows) reads the same plan.

| File | What it is |
|---|---|
| `boards/*.png` | **Start here.** Every board of the master canvas rendered as an image, 1 px = 1 canvas px. |
| `canvas/*.dc.html` | The boards' own source. Plans, sections and elevations are inline SVG with every dimension in the markup, so they can be read directly. They render properly only inside the canvas; use the PNGs for looking. |
| `canvas/canvas.json` | Board titles and their layout on the canvas. |
| `canvas/assets/` | The photos and images the boards embed. A board's `/_blob/<id>` is `assets/<id>.jpg`. |
| `proposals/<name>/` | Standalone canvases for new work, kept off the master on purpose. Same layout: `boards/*.png`, `canvas/`. |
| `Musee-Vision-Guide.pdf` | The whole museum in visit order as of **23 September**. Out of date for Élan and the as-built boards; the PNGs are current. |
| `renderings/` | GPT concept renderings with their prompts: mood targets, not the plan. `albion/` and `chenghuai/` are the 25 September sets. |

## Boards (master canvas)

| Board | Title |
|---|---|
| `Main` | Concept: classical bones, modern light, future glass; palette and type |
| `MasterPlan` | Master plan (v4): every wing around the Rotunda |
| `Journey` | The visit |
| `Rotunda` | The Rotunda and the Classical Hall: sun clock, section, the four doors |
| `Salon`, `SalonPlan`, `SalonHang`, `SalonDetails` | 01 · Salon Impression: bays, plan, the hang of 37 works, details |
| `SalonReserve` | The lily pond that lifts, and the Reserve under the Salon |
| `Sculpture` | Sculpture Hall · empty for now |
| `ChineseWing` | The Chinese Wing: Garden of the Four Seasons (being replaced, see `proposals/chenghuai`) |
| `HallOfLight` | Hall of Light: 30 m glass hall between two gardens |
| `FutureAtrium` | Section II · Élan · The Atrium, level 0 |
| `ElanCube` | Section II · Élan · The Cube, level −1 (**new**, replaces the Square) |
| `ElanCubeJourneys` | Section II · Élan · The Cube · two journeys (**new**) |
| `ElanSphere` | Section II · Élan · The Sphere, level 2 (**now Ø 28 m**) |
| `AsBuilt`, `AsBuiltClassical`, `AsBuiltElan`, `AsBuiltChinese`, `AsBuiltReserve` | As built in Unreal, September 2026: photos from the game (**new**) |
| `MaterialsStone`, `MaterialsOther` | The material language: stone; white, bronze, oak, glass, water (**new**) |

## Proposals (standalone canvases, 25 September)

None of these is on the master canvas yet. Build from them only where the status says so.

| Folder | Canvas | Status |
|---|---|---|
| `proposals/salon-interior` | https://claude.ai/artifact/U89EiNdzeevRxL3427gkan | **Proposed.** Interior only: the Salon's geometry and the open, straight line down the axis stay exactly as built. |
| `proposals/chenghuai` | https://claude.ai/artifact/MMmko4Lwg1TGgb8SYzmZoT | **Position confirmed:** the Chinese wing becomes 澄懷 Chenghuai, a three-court Beijing house on the Rotunda's **north** door, and the Sculpture Hall swaps to the **south** door. The master plan and `ChineseWing` board still show the old south cloister. |
| `proposals/albion` | https://claude.ai/artifact/ByPBC57MT53m5BN6Km96gw | **Proposed.** A Victorian England wing (1848–1898) in an iron-and-glass court, on the south side in place of the Sculpture Hall. |

**Salon interior** (for the Unreal build): the as-built Salon reads as one pale stone in one even light.
The proposal changes only surfaces, light and furniture:
- Silk on the pier faces: each bay's colour wraps onto the faces of its piers (a 0.14 m stone edge is
  left at each jamb), so the chapters read down the line. Silks one step deeper: bay 1 `#6A2A22`,
  bay 2 `#47515A`, bay 3 `#5A6D54`, bay 4 `#8A5A54`, bay 5 `#A0793A`; the oval stays lime white.
- Oak Versailles parquet in the bays on the existing 1.2 m grid; travertine under every arch.
- The vault in half-light (cove light at half, coffers in shade), a gilt rosette in each coffer,
  voussoirs and a keystone on every transverse arch. Paintings are the brightest thing in view.
- Five lanterns, five lights: same lanterns, new glass. Bay 1 clear, bay 2 opal, bay 3 under a leaf
  canopy (dappled sun), bay 4 muslin, bay 5 clear with a moving sun. Cloud and rain from Giverny, live.
- A pair of backless velvet benches in bays 2–5, 2.6 m either side of the axis (the axis stays clear).
- An 1874 table case opposite the stele in bay 1.
- The oval: the Orangerie's second room (the willows, 51 m) instead of the first (40 m at true size,
  which leaves 15.4 m of the 55.4 m wall bare). True sizes of the first room: *Morning* and *Clouds*
  12.75 m, *Green Reflections* 8.5 m, *Setting Sun* 6 m (the `SalonHang` board's 11.5/13.6 m are
  wrong). The pond built as planned: an 8.4 × 4.6 m oval, 0.3 m kerb, black water; it still lifts.

## The plan in words

**Convention.** Metres. The Rotunda's centre is the origin, x runs east, plan y runs south, height is up.

**Theme.** Classical bones, modern light, future glass. The visit walks that sequence: *Classical* is the
Rotunda, the Salon and the two side wings; *Modern · Alive* is the Hall of Light; *Future* is Élan (the
Atrium, the Cube and the Sphere).

**The Rotunda (start).** Every visit begins on the gilt sun rosette at the centre of a Ø 20 m drum under
a coffered dome with a steel-and-glass lattice eye (British Museum Great Court as the reference). The
floor is a Ø 14 m bronze sun clock (VI–XII–VI in Roman numerals, gilt meridian, motto *HORAS NON NUMERO
NISI SERENAS*). A bronze node in the lattice casts the point of shadow, and the sun is idealised at 15° an
hour. There is no floor map. Four doors: west to the Salon, north to Sculpture, south to the Chinese
Wing, east to the Hall of Light (see Proposals: Chenghuai takes the north door, Sculpture the south).
Raise the sun clock and a stair opens under it to the Classical Hall at level −1 (a coffered gallery and
a round tribune with casts from the Royal Cast Collection, SMK), as built.

**Salon Impression (west).** An 84 m enfilade of five skylit bays (Birth, Modern Paris, Leisure & Light,
Women, Monet's Series), each with its own wall colour. The Manet cabinet opens off Bay 1. You enter Bay 1
from the Rotunda facing *Impression, Sunrise* on a glass stele 1.4 m off the axis. It ends at the far
west in the Nymphéas oval: the Water Lilies panels round a pond of living lilies, ringed by four curved
benches. 37 works are hung (placements on `SalonHang`) and 13 are held in the Reserve.

**The pond and the Reserve.** Gaze at the golden lily and pinch: the whole pond (basin, water and lilies)
lifts 2.6 m on four bronze posts. Beneath it a straight stair of 36 steps descends east to the Reserve at
level −1: a 60 m brick-vaulted cellar under the whole Salon, with a central aisle, columns every 6 m,
sliding racks on both sides (A–E hold the 13 unhung works), plan chests for the Degas pastels, and a
viewing easel at the east end.

**Sculpture Hall (north).** One top-lit court, 16.8 m square inside, with a glass roof over a laylight at
10 m. It is built but empty: its ten scanned works fell short and were taken out until better scans are
found.

**The Chinese Wing (south): Garden of the Four Seasons, 四時園.** Vestibule with the solar-term stele →
moon gate Ø 3.6 m → an open moon terrace → a cloister on three sides around an 11 m garden (pond, Taihu
rock on the axis, plum, osmanthus, bamboo). The route runs counter-clockwise: the *Orchid Pavilion
Preface*; five ceramics (the Chenghua chicken cup is the one you may hold); six hanging scrolls on the
south wall; then Wang Ximeng's *Thousand Li* handscroll along the east walk. This is what is built; it is
being replaced by Chenghuai on the north door (`proposals/chenghuai`).

**The Hall of Light (east), Modern · Alive.** A glass hall 30 × 9 m inside, glass walls to 5 m with
mullions every 3 m, under the Rotunda's lattice drawn out into a shallow vault (crown 7.5 m). Gardens run
out to y = ±14 m on both sides: a birch meadow to the north and a flowering orchard to the south. One
photograph hangs per 3 m bay, walking east is forward in time (Talbot, Atkins, Nadar, Muybridge, Lumière
autochromes), and four stereo stones stand off the axis. The doors are 3 m (Rotunda) and 4 m (Atrium).

**Élan, level 0: the Atrium.** A Ø 28 m circle, its centre 54 m east of the Rotunda, with twelve bays
between rib piers. Its only door is the west bay. Eleven bays (A–K) are for new art; bay F, on the east
axis, is seen through the glass car. The ribs spring from a 6 m travertine base to a ring at 22 m. A
round glass car (Ø 4.4 m) stands at the centre with its door facing west. *The Starry Night* (the only
painting here) stands on a glass stele 1.9 m north of the axis.

**Élan, level −1: the Cube** (replaces the Square). A cube 28 m each way under the Atrium: the Sphere's
twin, mirrored in the Atrium floor. Its six faces are light-field panels (0.7 m tiles, 9,600 in all) that
put light in the room in three dimensions, true from anywhere; ultrasound arrays behind them place sound
and a light touch. The car comes down through the ground in about 25 s and stops at the exact centre,
22 m below the Atrium floor; it never lands and its glass clears to a rail. Two journeys, chosen on the
rail, each a day in about twelve minutes in six places: the Matterhorn (Riffelsee at dawn to the summit
at sunset) and the sea (`ElanCubeJourneys`). Feasible, not yet built.

**Élan, level 2: the Sphere.** A sphere Ø 28 m sits in the Atrium like a ball in a cup: its equator on
the drum at 22 m, its underside the Atrium's ceiling, a gilt ball at 41 m. The glass car climbs a pearl
neck, sealed by an iris at each end, to a stone platform Ø 11 m at the exact centre (27 s at 1.5 m/s).
Always on: the real sky for the museum's city now (stars, Moon, Milky Way). Later: new art on the whole
shell. Proposed opening piece: the live sky turns into *The Starry Night*.

Where the iPhone build had to interpret, bend or resolve a board, it says so in the top-level
`README.md` ("Where the plan was read, resolved or bent"). Other builds should use the same readings.
