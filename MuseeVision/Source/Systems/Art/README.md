# Art

The collection as real objects. Modules `MuseeArt` (Runtime: the catalogue, the hang, care, the work component) and
`MuseeArtMaking` (Editor: making the works). Its piece source and its dresser both run at order 30.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `ArtHang.h`, `ArtHangEntry.h`, `ArtWall.h`, `ArtWallPath.h` | `FArtHang`: a stage's `Hang.json`, its curved walls and its works |
| `ArtCatalogue.h`, `ArtCatalogueEntry.h`, `ArtGallery.h`, `ArtScan.h` | the museum's catalogues, read as they are written |
| `ArtWork.h` | `FArtWork` (the catalogue's facts, the hang's size, support and care, its image; `Caption`); `Musee::Art::ReadWork`, `FindImage`, `PaintMaterial` (`paint.<id>`), `RepositoryRoot` |
| `ArtCare.h` | `Musee::Art::CareClasses`: the keys of `Data/Art/Care.json` |
| `ArtWorkComponent.h` | `UArtWorkComponent`: a hung work's label (`IMuseeCaptioned`) and care class, on its face; the tags `musee.work:<id>`, `musee.care:<class>` |
| `ArtWorkImage.h` (making) | `Musee::ArtMaking::ComposeWorkImage`: composes a texture Studio makes from a work's image |
| `ArtPaintLayer.h` (making) | `FArtPaintLayer` and `Musee::ArtMaking::PaintLayers`: each hung work's image, its size and its paint's spec (`paint.<id>` over `accent.oil-on-linen`), which Studio makes before casting |

Each hung canvas is made as a stretched canvas: its face in its work's own paint, standing 40 mm off the wall on its
stretcher, its tacking edges and back in linen; curved to the wall where the hanging plan gives the wall a path. A work
on several canvases (`"canvas": <index>`) shows its one image across them, each face's UV0 in metres from the image's
top-left corner. A canvas the plan frames (`"frame": "<surface>"`) gets the museum's moulding (`Data/Art/Moulding.json`)
swept round it, mitred, as a closed solid in the finish of that surface.

## Later (tier 2, owner Art)

- More mouldings than the one (Albion's reeded and Watts frames), carved ornament on the moulding, panels, paper in a
  mat, silk scrolls; the hairline joints between a work's canvases; the stretcher's wood and keys; frames on curved walls.
- `Musee::Art::Targets` (FLightTarget from Care.json) and the checks `Art.TrueSize`, `Art.WithinCare`,
  `Art.ColorManaged`, `Art.Credited`, `Art.HangResolves`.
- Wall labels as objects, fittings (benches, stanchions), vitrines; `UArtImmersiveWork`.
