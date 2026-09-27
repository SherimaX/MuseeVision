# 澄懷 Chenghuai

The house and garden on the Rotunda's north door (D-001): a three-court Beijing house (三进四合院) with a Suzhou garden
(臥遊) beside it. Module `MuseeChenghuai` (Editor): a stage that describes itself, `UChenghuaiRoom : USiteRoom`, key
`chenghuai`. Its design sheet is `Docs/Stages/chenghuai.md`; its schedules are `Data/Stages/chenghuai/` (Finishes,
Lighting, Hang, Planting), read by the systems and never restated here.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `ChenghuaiRoom.h` | `UChenghuaiRoom`: `StageKey()` and `Describe(FSiteDescription&)`: footprint, port, openings, anchors, enclosures, pieces |

## Plan and parts (Private)

| File | What it holds |
| --- | --- |
| `ChenghuaiPlan.h` | every number of the site, the port, the anchors, the openings and the house's trades (§1-§3.8) |
| `ChenghuaiPlanHalls.h` | the building table (§3.1): each building as the one composition's spec |
| `ChenghuaiPlanGarden.h`, `ChenghuaiPlanFittings.h` | the garden (§3.9) and the fittings (§3.10) |
| `ChenghuaiFrame`, `ChenghuaiParts` | a building's frame in either hand; the drawing words (blocks, sections run along or across, raised outlines, turned pieces, rods, bars, reliefs over a curve), open regions on the plan |
| `ChenghuaiHall*` | the Qing hall: the section rule, the platform and columns, the timber frame, the roof (slab, rafters, tiles, ridges), the walls, the front |
| `ChenghuaiLattice` | the fronts' lattice panels and door leaves, the leak windows' grilles |
| `ChenghuaiBuildings.cpp`, `ChenghuaiGates.cpp` | the table drawn; the gate's joinery, the 垂花门, the screen wall |
| `ChenghuaiWalls.cpp`, `ChenghuaiCourts.cpp` | the free-standing walls with copings, leak windows and the moon gate; flights, paving, beds, bowl, tubs, gallery floors |
| `ChenghuaiGarden.cpp`, `ChenghuaiRockery.cpp` | the pond's basin (one landform: its bank and bed), edge rocks, paths, bridge, terrace, engraved stones; the rockery and its pavilion, whose roof is bent both ways |
| `ChenghuaiCases.cpp`, `ChenghuaiFurniture.cpp` | the cases; the screens, furniture, desk things, lanterns, name boards and couplet |
| `ChenghuaiSite.cpp` | footprint, port, openings (each doorway between two spaces declared once, naming the space beyond), anchors, enclosures |

## Tests

`Musee.Chenghuai.Ports`, `.Anchors`, `.Described`, `.Plan`; its shells are proved sealed by Studio's
`Musee.Studio.RoomsSealed`, since a room never sees FormMaking.

## Make

`MuseeVision/Scripts/make.ps1 chenghuai`.
