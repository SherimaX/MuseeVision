# Rotunda

The Rotunda (D-012), the anchor stage, where every visit begins on the gilt sun. Module `MuseeRotunda` (Editor): a
stage that describes itself, `URotundaRoom : USiteRoom`, key `rotunda`. Its design sheet is `Docs/Stages/rotunda.md`;
its schedules are `Data/Stages/rotunda/` (Finishes, Lighting, Hang, Mechanisms), read by the systems and never
restated here.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `RotundaRoom.h` | `URotundaRoom`: `StageKey()` and `Describe(FSiteDescription&)`: footprint, ports, openings, anchors, enclosures, pieces |

## Plan and parts (Private)

| File | What it holds |
| --- | --- |
| `RotundaPlan.h` | every number the room is built to, from the sheet, by section, with the sheet's profiles as point lists |
| `RotundaParts` | keys, surfaces, frames on circles, profiles from the sheet's steps, and mouldings laid as voussoirs on a curved face |
| `RotundaSite.cpp` | footprint, the five ports (the stair's serving the well), openings, anchors, enclosures |
| `RotundaDrum.cpp` | the drum's wall with its doorways and niches, footing, plinth over the shadow gap, top course; the niches' backs, heads and floors |
| `RotundaOrder.cpp` | sixteen fluted pilasters, the entablature, the door archivolts, the niche surrounds and sills |
| `RotundaDome.cpp` | the plain bands, shell, curb and lead; the 140 coffers, each its section run round its cell in the dome's development |
| `RotundaEye.cpp` | the lattice's ring and bars, the laminated glass, the node |
| `RotundaFloors.cpp` | the ring's core and slabs and the motto (Form's lettering, Centaur's capitals and gilt stops); the dial's bands and fillets and the gilt sun (body `rotunda.sun-clock.drum`) |
| `RotundaDialInlay.cpp` | the dial's hour ring, hour lines, gilt meridian and minute ticks, laid from Time's dial (`Musee::Time::FMuseumDial`, sheet §5) |
| `RotundaSunClock.cpp` | the lifting drum: wall, foot, base, pilasters, entablature, door archivolt, deck, soffit, rim, pendant |
| `RotundaShaft.cpp` | the sleeve, the pocket's masonry and bores, the tympanum and the port's reveal, the rams, the foundation, the landing and rosette |
| `RotundaStairLine` | the stair's setting-out: nosing lines, the pitch line, soffit and string top, and the cylinders its fittings are drawn on |
| `RotundaStair.cpp` | the helical stair's treads (spandrel steps dressed to the helicoid soffit), nosings and string |
| `RotundaStairFittings.cpp` | the well balustrade (fixed, and the two retracting panels on body `rotunda.stair.balustrade.panels`), their housings, the newel, the wall rail with its return, rose and brackets |

## Waiting on the systems

- The hour ring, lines, meridian and ticks are laid from Time's dial (RotundaDialInlay); its numerals wait to be lettered
  with Form's lettering. The doorways through the drum are still cut radially on the room's face, and the housings'
  sunk fields are polish.

## Tests

`Musee.Rotunda.Ports`, `.Anchors`, `.Described`, `.Plan`; its shells are proved sealed by Studio's
`Musee.Studio.RoomsSealed`, since a room never sees FormMaking.

## Make

`MuseeVision/Scripts/make.ps1 rotunda`, then `make.ps1 site`.
