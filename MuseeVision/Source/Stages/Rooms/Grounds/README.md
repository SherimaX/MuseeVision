# The Grounds

The museum's site under its own sky, the one Ground stage. Module `MuseeGrounds` (Editor): a stage that describes itself,
`UGroundsRoom : USiteRoom`, key `grounds`. Its design sheet is `Docs/Stages/grounds.md`; its schedules are
`Data/Stages/grounds/` (Finishes, Lighting, Planting), read by the systems and never restated here. Its frame is the
museum's plan frame; it has no port, and the site plan places it at the anchor's datum (`"placed"` in `SitePlan.json`).

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `GroundsRoom.h` | `UGroundsRoom`: `StageKey()` and `Describe(FSiteDescription&)`: footprint, anchors, enclosure, pieces |

## Plan and parts (Private)

| File | What it holds |
| --- | --- |
| `GroundsPlan.h` | every number the stage is built to, from the sheet, by section |
| `GroundsRun` | a mason's run in plan (straight legs and arcs), set out in stones, a joint and a mitre on every corner |
| `GroundsSettingOut` | the lines several trades work to: where the walk and the path meet the ring's and terrace's circles; the kerbs' runs |
| `GroundsSite.cpp` | the Ground footprint, the anchors (Water, Mount, Bed, Seat) and the Outdoors enclosure |
| `GroundsTerrain.cpp` | the soil, -0.33 to -0.03, over the 800 m square less the buildings standing on it, the basins, the parvis and every laid or swept piece set into it (drawn last, from what each covers in plan) |
| `GroundsPaving.cpp` | the parvis's 17 courses of travertine slabs |
| `GroundsGravel.cpp` | the walks: side walks, allées, cross walks, canal walks, avenue, walk east, ring, path north, terrace |
| `GroundsStrip.cpp` | the strip round the stone complex and its stone edge |
| `GroundsKerbs.cpp` | the kerbs, stone by stone along their runs |
| `GroundsBasins.cpp` | the canal's and the round basin's copings and bluestone floors |
| `GroundsLamps.cpp` | the ten bronze standards with their cased-opal globes and finials |

The lawn, the 37 trees, the box loops and balls, the yew exedra and the water are Nature's, from Planting.json; the
lamps' light and the 81 step lights are Light's, from Lighting.json.

## Waiting on the systems

- **Form** cuts no solid by another: the terrain gives way to the buildings Site hands it (their prisms through the
  soil), the two basins and the pieces set into it by holes the stage draws from their plans (`SetInto`); the lamps at
  the parvis's corners stand bedded in its slabs.
- **Form** has no union of plan regions: the strip's bands are laid face by face and merged where they meet square (the
  Salon and its cabinet); its junctions with the oval's band (the Salon's west end) and the drum's (its east end) are
  left unlaid.
- **Form** lays no surface (`Lay`): the parvis's slabs and the kerbs' and copings' stones are drawn one by one.
- **Site** reads no `"placed"` entry in `SitePlan.json` yet (KIT-BACKLOG); **Nature** reads no bed field beyond the
  species, count and spacing, and has no record for the five species; **Light** has no `post-top-globe.2700k` or
  `step-light.2700k`.

## Tests

`Musee.Grounds.Ports`, `.Anchors`, `.Described`, `.Plan`; the stage has no Enclosed shell, so Studio's
`Musee.Studio.RoomsSealed` only makes its drawings.

## Make

`MuseeVision/Scripts/make.ps1 grounds`, then `make.ps1 site`.
