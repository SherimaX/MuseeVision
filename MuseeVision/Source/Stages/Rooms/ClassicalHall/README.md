# The Classical Hall

Level −1 under the Rotunda: a barrel-vaulted sculpture gallery after the Braccio Nuovo and, through a screen of two
columns, a round domed tribune after the Sala delle Muse, holding 41 plaster casts. Module `MuseeClassicalHall`
(Editor): a stage that describes itself, `UClassicalHallRoom : USiteRoom`, key `classical-hall`. Its design sheet is
`Docs/Stages/classical-hall.md`; its schedules are `Data/Stages/classical-hall/` (Finishes, Lighting, Hang), read by the
systems and never restated here.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `ClassicalHallRoom.h` | `UClassicalHallRoom`: `StageKey()` and `Describe(FSiteDescription&)`: footprint, port with its received well, openings, 46 Mount anchors, enclosure, pieces |

## Plan and parts (Private)

| File | What it holds |
| --- | --- |
| `ClassicalHallPlan.h` | every number the hall is built to, from the sheet, by section; the carriers and their anchors (§2, §8) |
| `ClassicalHallParts` | the pen the sheet's profiles are written with, frames, the barrel's section, the niches' setting-out, the rosettes |
| `ClassicalHallSite.cpp` | footprint, the port `classical-hall.stair` receiving the Rotunda's well, the rough opening and five laylight Housings, anchors, enclosure |
| `ClassicalHallWalls.cpp` | slab, south wall and sleeve to the port cylinder, the gallery's and the tribune's walls (laid finish over the core), lintel |
| `ClassicalHallNiches.cpp` | the thirteen niches, each in its flat bay: finish, core, sill, lining, hood, surround, keystone |
| `ClassicalHallFloors.cpp` | the opus sectile of the gallery and the tribune |
| `ClassicalHallGalleryVault.cpp` | the coffered barrel (a relief per field, rosettes carved in), lanterns and laylights, the masonry and roof over it |
| `ClassicalHallTribuneVaults.cpp` | the annular vault and the dome (turned plaster, turned ribs, cast rosettes), the eye, the masonry and roof over them |
| `ClassicalHallOrder.cpp` | the 26 Ionic columns and 10 pilasters |
| `ClassicalHallEntablatures.cpp` | the gallery's entablature with its ressauts, the tribune's and the ring beam's, their dentils |
| `ClassicalHallMouldings.cpp` | skirting and dado rail round the perimeter, the framed panels and the lunette panel |
| `ClassicalHallCarriers.cpp` | niche plinths and pedestals, consoles, herms, axis and ambulatory plinths, the centrepiece's pedestal |

The structural masonry's pieces are named `shell.*`; they bound the enclosure `classical-hall.interior`.

## Tests

`Musee.ClassicalHall.Ports`, `.Anchors`, `.Described`, `.Plan`; its shells are proved sealed by Studio's
`Musee.Studio.RoomsSealed`, since a room never sees FormMaking.

## Make

`MuseeVision/Scripts/make.ps1 classical-hall`, then `make.ps1 site` once the Rotunda is built.
