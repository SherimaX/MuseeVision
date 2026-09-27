# The Hall of Light

The photography hall under a glazed vault, from the Rotunda's drum to the Atrium's stone base. Module
`MuseeHallOfLight` (Editor): a stage that describes itself, `UHallOfLightRoom : USiteRoom`, key `hall-of-light`. Its
design sheet is `Docs/Stages/hall-of-light.md`; its schedules are `Data/Stages/hall-of-light/` (Finishes, Lighting,
Hang, Planting), read by the systems and never restated here.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `HallOfLightRoom.h` | `UHallOfLightRoom`: `StageKey()` and `Describe(FSiteDescription&)`: footprint, ports, openings, anchors, enclosures, pieces |

## Plan and parts (Private)

| File | What it holds |
| --- | --- |
| `HallOfLightPlan.h` | every number the hall is built to, from the sheet, by section |
| `HallOfLightParts` | keys and surfaces, the rhythms along the hall, the vault's lines, the stereo readers' frames |
| `HallOfLightEnds` | the two ends (cylinders about the neighbours' axes) and runs between them, cut to the linings in plan |
| `HallOfLightSite.cpp` | footprint, ports (`hall-of-light.west`, `hall-of-light.east`), openings, anchors, enclosures |
| `HallOfLightMasonry.cpp` | the floor, viewing stones and rings, plinths with the step lights' housings, kerbs, stereo stones |
| `HallOfLightLinings.cpp` | the end linings, their tops on the vault's arc, with their rough openings straight through and the east lunette cut above; the lunette's panes and mullions |
| `HallOfLightWalls.cpp` | posts, end posts, glazing shoes, transoms, eaves and the wall panes |
| `HallOfLightVault.cpp` | arches, purlins, crosses and collars (curved on the barrel, in its development) and the vault's panes, the end bays cut to the linings |
| `HallOfLightBronze.cpp` | the prints' stems and shoes, the autochromes' glazing bars, the stereo readers |

## Tests

`Musee.HallOfLight.Ports`, `.Anchors`, `.Described`, `.Plan`; its shell is proved sealed by Studio's
`Musee.Studio.RoomsSealed`, since a room never sees FormMaking.

## Make

`MuseeVision/Scripts/make.ps1 hall-of-light`.
