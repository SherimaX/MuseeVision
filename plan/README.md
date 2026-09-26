# The plan

A snapshot of the Musée Vision design canvas, taken on 2026-09-23 (canvas version `1790186716-2a8f`).
The live canvas is still where the design is edited:
https://claude.ai/artifact/5KS3Zkfoub5zBdopET1Qc6. When a board changes there, refresh this folder
from it so every build (iPhone, Windows) reads the same plan.

| File | What it is |
|---|---|
| `Musee-Vision-Guide.pdf` | **Start here.** The whole museum in visit order, 38 landscape pages built from the boards and the current design: Rotunda → Salon + Reserve → Sculpture → Chinese Wing → Hall of Light → Élan. |
| `guide/` | The guide's source: `guide.html` (one section per page), `guide.css`, the renderings in `images/`, and `elan_drawings.py` (the Élan drawings at true scale). `guide/build.ps1` prints the PDF with headless Chrome; `-Png` renders pages for checking. |
| `canvas/*.dc.html` | The boards' own source. Plans, sections and elevations are inline SVG with every dimension in the markup, so they can be read directly. They render properly only inside the canvas; use the PDF for looking. |
| `canvas/canvas.json` | Board titles and their layout on the canvas. |
| `renderings/` | The GPT concept renderings with their prompts: mood targets, not the plan. Three are out of date; see its README. |

## Boards

| Board | Title |
|---|---|
| `Main` | Concept: classical bones, modern light, future glass; palette and type |
| `MasterPlan` | Master plan (v4): every wing around the Rotunda |
| `Journey` | The visit on Vision Pro |
| `Rotunda` | The Rotunda: sun clock, section, the four doors |
| `Salon`, `SalonPlan`, `SalonHang`, `SalonDetails` | 01 · Salon Impression: bays, plan, the hang of 37 works, details |
| `SalonReserve` | The lily pond that lifts, and the Reserve under the Salon |
| `Sculpture` | Sculpture Hall: one top-lit court, ten open-scan works |
| `ChineseWing` | The Chinese Wing: Garden of the Four Seasons |
| `HallOfLight` | Hall of Light: 30 m glass hall between two gardens |
| `FutureAtrium` | Section II · Élan · The Atrium, level 0 |
| `ElanSquare` | Section II · Élan · The Square, level −1 |
| `ElanSphere` | Section II · Élan · The Sphere, level 2 |

## The plan in words

**Convention.** Metres. The Rotunda's centre is the origin, x runs east, plan y runs south, height is up.

**Theme.** Classical bones, modern light, future glass. The visit walks that sequence: *Classical* is the
Rotunda, the Salon and the two side wings; *Modern · Alive* is the Hall of Light; *Future* is Élan (the
Atrium, the Square and the Sphere).

**The Rotunda (start).** Every visit begins on the gilt sun rosette at the centre of a Ø 20 m drum under
a coffered dome with a steel-and-glass lattice eye (British Museum Great Court as the reference). The
floor is a Ø 14 m bronze sun clock (VI–XII–VI in Roman numerals, gilt meridian, motto *HORAS NON NUMERO
NISI SERENAS*). A bronze node in the lattice casts the point of shadow, and the sun is idealised at 15° an
hour. There is no floor map. Four doors: west to the Salon, north to Sculpture, south to the Chinese
Wing, east to the Hall of Light.

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
10 m. It holds ten works from CC0/CC-BY scans. *The Gates of Hell* stands on the north wall, closing the
sight line from the sun rosette, with the *Thinker* and the *Kiss* either side. Also Carpeaux's *La Danse*
(west wall), the *Age of Bronze*, the *Burghers of Calais* at floor level, *Balzac*, the *Walking Man*,
Rude's head and Bourdelle's *Heracles* near the door. Nothing stands on the axis.

**The Chinese Wing (south): Garden of the Four Seasons, 四時園.** Vestibule with the solar-term stele →
moon gate Ø 3.6 m → an open moon terrace → a cloister on three sides around an 11 m garden (pond, Taihu
rock on the axis, plum, osmanthus, bamboo). The route runs counter-clockwise: the *Orchid Pavilion
Preface*; five ceramics (the Chenghua chicken cup is the one you may hold); six hanging scrolls on the
south wall; then Wang Ximeng's *Thousand Li* handscroll along the east walk, which opens an arm's length
at a time as you walk north. The garden follows the 24 solar terms, and the real moon lies in the pond.
(The master plan still draws a four-sided cloister with a 9 m garden; the `ChineseWing` board is newer.)

**The Hall of Light (east), Modern · Alive.** A glass hall 30 × 9 m inside, glass walls to 5 m with
mullions every 3 m, under the Rotunda's lattice drawn out into a shallow vault (crown 7.5 m). Gardens run
out to y = ±14 m on both sides: a birch meadow to the north and a flowering orchard to the south. One
photograph hangs per 3 m bay, walking east is forward in time (Talbot, Atkins, Nadar, Muybridge, Lumière
autochromes), and four stereo stones stand off the axis. The doors are 3 m (Rotunda) and 4 m (Atrium).

**Élan, level 0: the Atrium.** A Ø 28 m circle, its centre 54 m east of the Rotunda, with twelve bays
between rib piers. Its only door is the west bay. Eleven bays (A–K) are for new art; bay F, on the east
axis, is seen through the glass car. Above a 6 m travertine base, a drum of plain misty glass rises to
22 m, where the Sphere sits on it; the Sphere's underside is the ceiling, and the ribs run up the drum
and under the Sphere to a ring round the opening over the car (changed since the boards, which still
draw ribs leaning in to a ring at 22 m). A round glass car (Ø 4.4 m) stands at the centre with its door facing west. *The Starry
Night* (the only painting here) stands on a glass stele 1.9 m north of the axis. You wait on the bronze
ring, and the first ride goes down before it goes up.

**Élan, level −1: the Square.** A 28 m square under the Atrium, with rammed-earth walls in strata and 7.5 m
of clear height. The floor is the Lo Shu's nine squares (north up: 6 1 8 / 7 5 3 / 2 9 4) with bronze
numerals. The car lands on 5, where a glass floor looks down through soil, clay, chalk and bedrock. There
are open galleries on the cross (1, 3, 9, 7) and dark rooms in the corners (6, 8, 4, 2), for new art.

**Élan, level 2: the Sphere.** A Ø 28 m shell after Boullée's, used like the Las Vegas Sphere: the whole
inner surface is one image. Its radius is the Atrium's and it sits in the Atrium like a ball in a cup,
its centre 22 m up, its equator on the top of the drum. (Changed since the boards, which draw Boullée's
Ø 150 m shell centred 101 m up.) One glass car rises through the opening at the south pole (8.3 m) and up
a bronze mast to the exact centre at a steady 1.5 m/s (about 16 s, with no skip). There the glass dims
to a rail and a floor, and the image surrounds you, below your feet too. From outside it is a misty
pearl on the drum; from inside, the building is gone. It
always shows the real sky over your location now, and new art on a timetable. The proposed opening piece
has the live sky turn into *The Starry Night*.

Where the iPhone build had to interpret, bend or resolve a board, it says so in the top-level
`README.md` ("Where the plan was read, resolved or bent"). Other builds should use the same readings.
