# The Reserve

The museum's picture store (D-020), a brick undercroft 60 m long under the Salon, reached only by the Nymphéas oval's
stair once the golden iris has raised the pond. Module `MuseeReserve` (Editor): a stage that describes itself,
`UReserveRoom : USiteRoom`, key `reserve`. Its design sheet is `Docs/Stages/reserve.md`; its schedules are
`Data/Stages/reserve/` (Finishes, Lighting, Hang, Mechanisms), read by the systems and never restated here.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `ReserveRoom.h` | `UReserveRoom`: `StageKey()` and `Describe(FSiteDescription&)`: footprint, port, opening, anchors, enclosure, pieces |

## Plan and parts (Private)

| File | What it holds |
| --- | --- |
| `ReservePlan.h` | every number the undercroft is built to, from the sheet, by section |
| `ReserveParts` | the bays, rows and racks as the sheet sets them out, the vault's soffit over any plan point, and a box drawn standing on its own level |
| `ReserveSite.cpp` | footprint (two prisms), the port `reserve.stair` receiving the oval's well, its opening, anchors, the enclosure |
| `ReserveShell.cpp` | the four walls with the west door, its travertine threshold and P6 architrave, the flags' brick bed, the top under the Salon |
| `ReserveFloors.cpp` | the bluestone flags (nave and aisles) cut round the bases and tracks, the 36 flush bronze tracks |
| `ReservePiers.cpp` | fourteen piers (base, chamfered shaft, impost) and eighteen responds |
| `ReserveVaults.cpp` | the 24 groin vaults as four webs each, cut on their diagonals; cross and arcade bands; springing blocks |
| `ReserveTracks.cpp` | the 36 overhead channels, stops, wall plates and bolts, and their hanger rods |
| `ReserveRacks.cpp` | the 36 racks, each on its body `reserve.screen-<name>`: frame, wire mesh, carriage, pull, picture-light trough |
| `ReserveLuminaires.cpp` | the 24 globe pendants (chains in the nave, rods in the aisles), 56 uplight cans, the easel spot |
| `ReserveFittings.cpp` | the easel and the four plan-chest islands with their drawers and the pastels' felts |

## Tests

`Musee.Reserve.Ports`, `.Anchors` (every work on a rack names the rack's body), `.Described`, `.Plan`, `.Heights` (every box
at the level the sheet gives it); its shells are proved sealed by Studio's `Musee.Studio.RoomsSealed`, and its door's joint
clear of it and of the oval by `Musee.Studio.JointsClear.nympheas.stair`, since a room never sees FormMaking.

## Make

`MuseeVision/Scripts/make.ps1 nympheas`, then `make.ps1 reserve`, then `make.ps1 site`.

## Departures from the sheet's drawing, and the kit's gaps

- **The mesh is straight wire.** Form makes each of a drawing's repeats as a copy of its mesh; a crimped wire followed
  over its 123 to 193 crossings, 11,376 times, would hold gigabytes. The weave is drawn as its two layers of Ø 3.0
  wires touching on the centre plane, in 8 mm slots. A Form gap: repeats that share one made mesh.
- **The nave's flags part from the aisles' on the joint line |y| 4.50**, the grid's line nearest 4.65, so no flag is
  finished two ways.
- **The flags' lime joints take the flags' finish**: the palette has no lime mortar.
- **Parts that the sheet runs into their neighbours stop at the face they are fixed to**: the easel's brackets and ledge
  at the uprights' faces, its cross foot between the runners, the guide shoes under the kick rail, the fins under the
  shoes, the trolleys' nuts on their saddles, the tracks' channels at the wall plates' faces (the plates' 30 mm let
  into the brick is not drawn: a wall takes no pocket).
- **The webs, bands and haunch fill are one brick mass** from the soffit to the top: the 0.105 shell and the bands'
  0.165 are inside it, and what shows is the soffit the sheet draws.
- **The nave's canopies' eyes come down to the chain's top loop**, so the chain is of whole links.
- **The door's threshold is the Reserve's own**, not the joint's: the port's class carries no allowance, so Site lays no
  threshold and each stage finishes its side. The Reserve lays the sheet's honed travertine from the strip's edge
  (x -30.570) to the flags, on the brick sill lowered by the stone's 30 mm; the joint's bronze strip lies on the mating
  plane between it and the oval's landing, which is rebated for the strip's other half.
- **The pastels' window mats are not made** (Art's gap): a card on its reader is made glazed, but its mat is read only
  for prints on walls.
- **The uplights' cans are the sheet's Ø 44**; the catalogue's GU10 lamp is 50 across (Light's record and the sheet
  disagree).
