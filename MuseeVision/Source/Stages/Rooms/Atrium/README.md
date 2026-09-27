# Atrium

The Atrium, Élan's stage at level 0 (D-012): the glass drum on the Élan axis, with its canopy grid of edge-lit glass, the
glass lift car and its landing, and two irises, the collar iris above (the closure of `atrium.up`) and, through the down
port, the Cube's ceiling iris. Module `MuseeAtrium` (Editor): a stage that describes itself, `UAtriumRoom : USiteRoom`,
key `atrium`. Its design sheet is `Docs/Stages/atrium.md`; its schedules are `Data/Stages/atrium/` (Finishes, Lighting,
Hang, Mechanisms), read by the systems and never restated here.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `AtriumRoom.h` | `UAtriumRoom`: `StageKey()` and `Describe(FSiteDescription&)`: footprint, ports, openings, anchors, the hall, pieces |

## Plan and parts (Private)

| File | What it holds |
| --- | --- |
| `AtriumPlan.h` | every number the drum is built to, from the sheet, by section, with its sections as steps of lines and arcs |
| `AtriumParts` | keys, surfaces, bearings and radii, frames in radial planes, sectors, turned bands |
| `AtriumSite.cpp` | footprint, the three ports (the two vertical ones serving their wells), openings, anchors, the hall |
| `AtriumMasonry.cpp` | the base with its cove and the door's rough opening, the coping band, the dress, the entablature, the portal (its block and upper course, the segmental arch straight through both) |
| `AtriumFloor.cpp` | the slab, the eight marble rings with the lip's notches and the door bay, the waiting ring and mark |
| `AtriumGlazing.cpp` | the drum's inner and outer leaves, the head closer |
| `AtriumSteel.cpp` | the ribs (their chamfered section swept up their spines) and shoes, the compression ring and hangers, the drum's order |
| `AtriumCanopyGrid.cpp` | the canopy grid: nodes on twenty rings, ring runs and diagonal bars, stopped at the ribs and the ring; the canopy's 2,412 panes cut on it and bent onto the Sphere's seam |
| `AtriumLift.cpp` | the lining, rails and brackets, the landing, tube and collar, the collar iris's seven blades |
| `AtriumCar.cpp` | the car (body `atrium.car`), its leaves, the crown and the Cube mast's head; the stele |

## Waiting on the systems

- Form lays no ashlar or marble (`Lay`), cuts no rebate or recess in a slab, finishes no extrusion's faces apart, and
  turns solids that end on radial planes (K3-FORM-A's): the base's stones, the landing shoe's rebate, the waiting mark's
  recesses, the blades' gilt undersides and the dress's ends are their nearest honest forms, as each
  file's comment says.
- Studio hangs every body from the stage's frame: the car's leaves and the mast's head are declared where they stand
  with the car at the Atrium stop, and ride the car only when bodies can ride bodies.
- Site's `iris.4600` and `doorway.4000x4500.square`, and vertical ports with served wells, are KIT-2's; Motion's
  many-stop lift is KIT-2's.

## Tests

`Musee.Atrium.Ports`, `.Anchors`, `.Described`, `.Plan`; its shells are proved sealed by Studio's
`Musee.Studio.RoomsSealed`, since a room never sees FormMaking.

## Make

`MuseeVision/Scripts/make.ps1 atrium`, then `make.ps1 site`.
