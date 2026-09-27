# Albion

The British court on the Rotunda's south door (D-001): England 1848–1898, a museum court of iron and glass on ten
columns of polished British stone, after the Oxford University Museum. Module `MuseeAlbion` (Editor): a stage that
describes itself, `UAlbionRoom : USiteRoom`, key `albion`. Its design sheet is `Docs/Stages/albion.md`; its schedules
are `Data/Stages/albion/` (Finishes, Lighting, Hang), read by the systems and never restated here.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `AlbionRoom.h` | `UAlbionRoom`: `StageKey()` and `Describe(FSiteDescription&)`: footprint, the port, openings, anchors, the enclosure, pieces |

## Plan and parts (Private)

| File | What it holds |
| --- | --- |
| `AlbionPlan.h` | every number the court is built to, from the sheet, by section |
| `AlbionParts` | keys, surfaces and frames; simple solids; `FArch`, an opening's outline (two-centred, semicircular or flat) drawn piece by piece; `FVaultHalf`, a vault half's arc and everything set out under it |
| `AlbionCoursing` | the banded leaves: each wall laid as two leaves meeting on the glass line, cut at every colour change, pierced or notched by the openings exactly |
| `AlbionSite.cpp` | footprint, the port `albion.north`, openings, anchors, the enclosure `albion.court` |
| `AlbionMasonry.cpp` | the court's leaves, cills, pilasters and corbels, cornices, skirting, tablets, the hood mould |
| `AlbionOutside.cpp` | the weathered leaves, buttresses and base courses, the plinth course, copings, side gutters |
| `AlbionPorch.cpp` | the porch's walls, the end lining with the rough opening, the oak ceiling, the lead flat, copings |
| `AlbionFloors.cpp` | the encaustic nave and porch, the York stone aisles, the court door's reveal and strip |
| `AlbionColumns.cpp` | the ten columns and four half-columns: plinths, bases, monolith shafts, bells, abaci |
| `AlbionArcade.cpp` | the clusters, box girders and valley gutters, the arcade's arches, bosses and spandrels |
| `AlbionVaults.cpp` | main ribs with shoes, inner arches and ring chains; light ribs; purlins and ridge bars; glazing bars; bearers and brackets |
| `AlbionRoof.cpp` | the vaults' lapped panes, the ridge rolls, crestings and the ventilator |
| `AlbionScreens.cpp` | the nave's screens, the aisles' gables, the Tristram and Isolde band's leaded border, cames and guards |
| `AlbionLancets.cpp` | the lancets' glass, quarry lattice, guards, saddle bars, weathered sills and drips |
| `AlbionRainwater.cpp` | the ten downpipes, heads and holderbats |
| `AlbionFittings.cpp` | the picture rail, benches, table cases, lectern, the tapestry's batten and lining |

## Tests

`Musee.Albion.Ports`, `.Anchors`, `.Described`, `.Plan`; its shells are proved sealed by Studio's
`Musee.Studio.RoomsSealed`, since a room never sees FormMaking.

## Make

`MuseeVision/Scripts/make.ps1 albion`. The site plan mates `albion.north` with `rotunda.south`; until the Rotunda is
built the port stands blind.
