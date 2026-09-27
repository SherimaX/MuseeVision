# The Cube

Élan's room under the Atrium (D-012): a sealed room 28 m each way, 22 m down on the Élan axis, whose six inner faces are
light-field panels, reached by the Atrium's glass car down a glass shaft through the site's ground. The car sinks on a
gilt mast through the ceiling iris until the visitor's eyes are at the room's centre, where II · Sea Light performs.
Module `MuseeCube` (Editor): a stage that describes itself, `UCubeRoom : USiteRoom`, key `cube`. Its design sheet is
`Docs/Stages/cube.md`; its schedules are `Data/Stages/cube/` (Finishes, Lighting, Mechanisms), read by the systems and
never restated here. Nothing is hung and nothing is planted.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `CubeRoom.h` | `UCubeRoom`: `StageKey()` and `Describe(FSiteDescription&)`: footprint, the port, openings, anchors, enclosures, pieces |

## Plan and parts (Private)

| File | What it holds |
| --- | --- |
| `CubePlan.h` | every number the room is built to, from the sheet, by section, with §7's stratigraphic rule |
| `CubeParts` | keys, surfaces, bearings and frames; bands and prisms about the axis; the blades' bodies and pins |
| `CubeSite.cpp` | footprint, the port `cube.up` with the well it receives, openings, anchors, the three enclosures |
| `CubeBox.cpp` | the concrete box: four walls, base and the roof pierced for the sleeve |
| `CubePanels.cpp` | the 9,512 panel modules on six faces, the ceiling's 44 shaped to its rough opening |
| `CubeShaft.cpp` | the shaft's sixteen panes, gilt frames and joint strips, head and foot plates, tier webs |
| `CubeRails.cpp` | the two rails and their tails, their bronze brackets and the tails' gilt brackets |
| `CubeCeiling.cpp` | the sleeve and its line, the iris's housing and hangers, the soffit ring and its beads |
| `CubeIris.cpp` | the ceiling iris: seven blades of class iris.4600, drawn shut on their bodies |
| `CubeStrata` | §7's rule: the contacts, the pipe, each layer's face, and the ammonite's shell |
| `CubeGround.cpp` | the ground's cut face, one relief per layer, its two collars, and the ammonite |

The frame is the Atrium's (museum = local + (54, 0, 0)), so every height is the as-built height. The moving bodies are
the seven blades only; the car, the mast's head and the column are the Atrium's. A Mount's x faces into the room, as
Light reads a mount.

## Tests

`Musee.Cube.Ports`, `.Anchors`, `.Described`, `.Plan`, `.Iris` (shut, open and clamped against the class) and `.Ground`
(the rule within its bounds); its shells are proved sealed by Studio's `Musee.Studio.RoomsSealed`, since a room never
sees FormMaking.

## Make

`MuseeVision/Scripts/make.ps1 cube`. The site plan mates `cube.up` with `atrium.down`; until the Atrium is built the port
stands blind.
