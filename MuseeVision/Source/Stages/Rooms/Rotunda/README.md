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
| `RotundaDome.cpp` | the plain bands, shell, curb and lead; the coffers' ring ribs and meridional ribs |
| `RotundaEye.cpp` | the lattice's ring and bars, the laminated glass, the node |
| `RotundaFloors.cpp` | the ring's core and slabs; the dial's bands and fillets and the gilt sun (body `rotunda.sun-clock.drum`) |
| `RotundaDialInlay.cpp` | the dial's hour ring, hour lines, gilt meridian and minute ticks, laid from Time's dial (`Musee::Time::FMuseumDial`, sheet §5) |
| `RotundaSunClock.cpp` | the lifting drum: wall, foot, base, pilasters, entablature, door archivolt, deck, soffit, rim, pendant |
| `RotundaShaft.cpp` | the sleeve, the pocket's masonry and bores, the tympanum and the port's reveal, the rams, the foundation, the landing and rosette |
| `RotundaStair.cpp` | the helical stair's treads, nosings and string |

## Waiting on the systems

- The numerals wait for Form's lettering and the ported glyphs, as the motto does; the hour ring, lines, meridian and
  ticks are laid from Time's dial (RotundaDialInlay). Nothing is drawn by hand here.
- Form draws straight passages through curved walls radially, sweeps only along plan paths, and has no relief on a
  surface of revolution or lofted solid: the doorways, the stair's string and soffit, the coffers' meridional ribs and
  the sun's rays are their nearest honest forms, as each file's comment says. The stair's balustrades, handrails and
  their housings wait for sweeps along helices.

## Tests

`Musee.Rotunda.Ports`, `.Anchors`, `.Described`, `.Plan`; its shells are proved sealed by Studio's
`Musee.Studio.RoomsSealed`, since a room never sees FormMaking.

## Make

`MuseeVision/Scripts/make.ps1 rotunda`, then `make.ps1 site`.
