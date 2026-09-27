# Salon

The Salon Impression (D-019: the five bays and the Manet cabinet). Module `MuseeSalon` (Editor): a stage that describes
itself, `USalonRoom : USiteRoom`, key `salon`. Its design sheet is `Docs/Stages/salon.md`; its schedules are
`Data/Stages/salon/` (Finishes, Lighting, Hang, Planting; nothing moves here), read by the systems and never restated
here.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `SalonRoom.h` | `USalonRoom`: `StageKey()` and `Describe(FSiteDescription&)`: footprint, ports, openings, anchors, enclosures, pieces |

## Plan and parts (Private)

| File | What it holds |
| --- | --- |
| `SalonPlan.h` | every number the room is built to, from the sheet, by section, with the mouldings' sections P1 to P8 |
| `SalonParts` | the setting-out shared by the trades: keys and surfaces, face frames, arched outlines and trims, the vault's column lines and rows, the lanterns' rectangles |
| `SalonSite.cpp` | footprint, ports (`salon.east`, `salon.west`), openings, anchors, enclosures |
| `SalonWalls.cpp` | the masonry (the east wall's step, the far wall's rough opening), the doors' stone linings, the silk, the lunettes |
| `SalonPiers.cpp` | piers with base, impost and silk; the arches' voussoirs, keystones, spandrels and archivolts |
| `SalonMouldings.cpp` | the cornices (P1, P8), skirtings (P2) and architraves (P5) |
| `SalonVault.cpp` | each bay's coffered vault: shell, bands, transverse and longitudinal ribs, open over the lantern |
| `SalonRosettes.cpp` | the 750 gilt rosettes, carved as reliefs on the coffers' panels |
| `SalonLanterns.cpp` | the shafts, bronze grids, inner panes, bay 4's linen scrim |
| `SalonRoof.cpp` | the roof slab, the outer lanterns on their kerbs, bay 3's pergola and planters |
| `SalonCabinet.cpp` | the Manet cabinet's walls, ceiling with its laylight well and laylight, roof |
| `SalonFloors.cpp` | the oak, the stone under the arches and in the cabinet door, the strips, the viewing stones |
| `SalonFittings.cpp` | the benches, the stele, the 1874 case, the Little Dancer's plinth and vitrine |

## Tests

`Musee.Salon.Ports`, `.Anchors`, `.Described`, `.Plan`; its shells are proved sealed by Studio's
`Musee.Studio.RoomsSealed`, since a room never sees FormMaking.

## Make

`MuseeVision/Scripts/make.ps1 salon`, then `make.ps1 site`.
