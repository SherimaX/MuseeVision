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
| `ArtWorkImage.h` (making) | `Musee::ArtMaking::FillWorkImage`: fills a texture Studio makes with a work's image |

Each hung work is made as a stretched canvas: its face in its own paint, standing 40 mm off the wall on its
stretcher, its tacking edges and back in linen; curved to the wall where the hanging plan gives the wall a path.

## Later (tier 2, owner Art)

- Frames (`frame.gilt.carved` and the rest), panels, paper in a mat, silk scrolls; multi-canvas works with their
  hairline joints; the stretcher's wood and keys.
- `Musee::Art::Targets` (FLightTarget from Care.json) and the checks `Art.TrueSize`, `Art.WithinCare`,
  `Art.ColorManaged`, `Art.Credited`, `Art.HangResolves`.
- Wall labels as objects, fittings (benches, stanchions), vitrines; `UArtImmersiveWork`.
