# Nymphéas

The Nymphéas Oval (D-019), the first room and the site plan's anchor. Module `MuseeNympheas` (Editor): a stage that
describes itself, `UNympheasRoom : USiteRoom`, key `nympheas`. Its design sheet is `Docs/Stages/nympheas.md`; its
schedules are `Data/Stages/nympheas/` (Finishes, Lighting, Hang, Planting, Mechanisms), read by the systems and never
restated here.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `NympheasRoom.h` | `UNympheasRoom`: `StageKey()` and `Describe(FSiteDescription&)`: footprint, ports, openings, anchors, enclosures, pieces |

## Plan and parts (Private)

| File | What it holds |
| --- | --- |
| `NympheasPlan.h` | every number the room is built to, from the sheet, by section |
| `NympheasCurves` | the oval's setting-out: the ellipse, its parallels and their arc lengths; Catmull-Rom centrelines |
| `NympheasSite.cpp` | footprint, ports (`nympheas.east`, `nympheas.stair` with its served well), openings, anchors, enclosures |
| `NympheasShell.cpp` | the wall and its straight door, the passage block, the door's architrave, cove, skirting, velarium and rim ring, upstand, roof ring, glass dome |
| `NympheasRoofFrame.cpp` | the velarium's tees and rods, the dome's glazing bars and curb, following the cloth's and the glass's rise |
| `NympheasFloors.cpp` | the travertine floor, viewing stones, curb with chases and sleeves, shaft walls, slab, the east rail's housing |
| `NympheasWell.cpp` | foundation, end and head walls, the flight, landings, strings, handrails and brackets |
| `NympheasPond.cpp`, `NympheasFrieze` | tray, lining, shelves, sixteen coping blocks with the carved frieze, lip, posts |
| `NympheasFittings.cpp` | the guard rails (body `nympheas.guard-rails`) and the four benches |

## Tests

`Musee.Nympheas.Ports`, `.Anchors`, `.PlantingAnchors`, `.Described`, `.Plan`; its shells are proved sealed by
Studio's `Musee.Studio.RoomsSealed`, since a room never sees FormMaking.

## Make

`MuseeVision/Scripts/make.ps1 nympheas`, then `make.ps1 site`.
