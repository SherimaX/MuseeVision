# The Sphere

The Sphere (D-012), the Future's stage above the Atrium on the Élan axis. Module `MuseeSphere` (Editor): a stage that
describes itself, `USphereRoom : USiteRoom`, key `sphere`. Its design sheet is `Docs/Stages/sphere.md`; its schedules
are `Data/Stages/sphere/` (Finishes, Lighting, Mechanisms), read by the systems and never restated here.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `SphereRoom.h` | `USphereRoom`: `StageKey()` and `Describe(FSiteDescription&)`: footprint, port, openings, anchors, enclosures, pieces |

## Plan and parts (Private)

| File | What it holds |
| --- | --- |
| `SpherePlan.h` | every number the stage is built to, from the sheet, by section |
| `SphereParts` | setting out about the axis in bearings; rings, sectors, the seats' 96-gon, the class's iris blade, the bodies' frames |
| `SphereSite.cpp` | footprint, the vertical port `sphere.down` with the well it receives, openings, anchors, enclosures |
| `SphereShells.cpp` | the steel plate, the image surface, the pearl skin, the drip ring on the Atrium's wash, the boss and the ball |
| `SphereNeck.cpp` | the throat and its band between the dogs' housings, the gilt trellis, the flange and pins, the neck iris, the rails |
| `SpherePlatform.cpp` | deck, stone ring, seat ring and sill, the platform iris and pins, nosing, balustrade, handrail, channel, struts |
| `SphereMast.cpp` | the mast's foot ring (body `sphere.mast.foot`), the four bearing dogs and their housings |

The bodies are drawn closed (§2): blades shut on their pins, dogs retracted, the mast at zero length. The mast's column
is Motion's variable-length piece and is not drawn here.

## Tests

`Musee.Sphere.Ports`, `.Anchors`, `.Described`, `.Plan`; its shells are proved sealed by Studio's
`Musee.Studio.RoomsSealed`, since a room never sees FormMaking.

## Make

`MuseeVision/Scripts/make.ps1 sphere`, then `make.ps1 site`.
