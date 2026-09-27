# Art

The collection as real objects. Modules `MuseeArt` (Runtime: the catalogue, the hang, care, the work component) and
`MuseeArtMaking` (Editor: making the works). Its piece source and its dresser both run at order 30.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `ArtHang.h`, `ArtHangEntry.h`, `ArtWall.h`, `ArtWallPath.h` | `FArtHang`: a stage's `Hang.json`, its curved walls and its works; every field the stages' sheets write |
| `ArtAfter.h`, `ArtScrollMount.h`, `ArtScanPlacement.h`, `ArtCarrier.h` | an entry's original (`after`), a scroll's mount, a cast's scan and carrier |
| `ArtCatalogue.h`, `ArtCatalogueEntry.h`, `ArtGallery.h`, `ArtScan.h` | the museum's catalogues, read as they are written |
| `ArtWork.h` | `FArtWork` (the catalogue's facts, the hang's size, support, care and reproduction, its image; `Caption`); `Musee::Art::ReadWork`, `FindImage`, `PaintMaterial` (`paint.<id>`, `paint.<id>.<index>`), `RepositoryRoot` |
| `ArtCare.h`, `ArtLightDose.h` | `Musee::Art::CareClasses`, `Reproductions`, `ReproductionCredit`; `CareFaults` (a dose against a class; a reproduction not held to care is within under any light) |
| `ArtWorkComponent.h` | `UArtWorkComponent`: a work's label (`IMuseeCaptioned`) and care class, the component its pieces hang from; the tags `musee.work:<id>`, `musee.care:<class>` |
| `ArtWorkImage.h` (making) | `Musee::ArtMaking::ComposeWorkImage`: composes a texture Studio makes from a work's image |
| `ArtPaintLayer.h` (making) | `FArtPaintLayer` and `Musee::ArtMaking::PaintLayers`: each hung work's image, its size and its paint's spec (`paint.<id>` over `accent.oil-on-linen`), which Studio makes before casting |

Each hung canvas is made as a stretched canvas: its face in its work's own paint, standing 40 mm off the wall on its
stretcher, its tacking edges and back in linen; curved to the wall where the hanging plan gives the wall a path. A work
on several canvases (`"canvas": <index>`) shows its one image across them, each face's UV0 in metres from the image's
top-left corner. A canvas the plan frames (`"frame": "<surface>"`) gets the museum's moulding (`Data/Art/Moulding.json`)
swept round it, mitred, as a closed solid in the finish of that surface.

Canvases that each name their own `image` (the Rouen series) each show it, in their own layer `paint.<id>.<index>`.
A `glass` entry at a `Glazing` anchor (Albion's band, D-014) is a pane of the glass its `wall` surface names (Lamberts'
antique glass, 3 mm), in Form's opening, its image in its own layer over that glass's finish.

A work the stage builds (`"built": "form"`) gets only its caption and care: the stage names its pieces
`work.<id>[.<index>][.<part>]`, and the dresser hangs them from the work's label. A reproduction (`"reproduction":
"exhibition-facsimile"`, Care.json's `reproductions`) keeps its original's care class and credits the original; care
accepts it under any light. `after` records the original's size for a work shown at another (`TimesOriginal`).

The collection's catalogues and images are read from MUSEE_COLLECTION, this working copy, or the repository's main
working tree when this copy is a linked worktree that doesn't check them out.

`Musee.Art.EveryHangResolves` holds every stage's Hang.json to Art's reader: every entry resolves to a work its catalogue
holds, made by Art or built by its stage.

## Later (tier 2, owner Art)

- More mouldings than the one (Albion's reeded and Watts frames), carved ornament on the moulding, panels, paper in a
  mat, silk scrolls; the hairline joints between a work's canvases; the stretcher's wood and keys; frames on curved walls.
- `Musee::Art::Targets` (FLightTarget from Care.json) and the checks `Art.TrueSize` (with `after`), `Art.WithinCare`
  (over `CareFaults`), `Art.ColorManaged`, `Art.Credited`, `Art.HangResolves`.
- Papers, panels, scrolls, casts, case objects and books: the supports the plans name that Art doesn't make yet.
- Wall labels as objects, fittings (benches, stanchions), vitrines; `UArtImmersiveWork`.
