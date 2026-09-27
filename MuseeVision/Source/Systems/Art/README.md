# Art

The collection as real objects. Modules `MuseeArt` (Runtime: the catalogue, the hang, care, the work component) and
`MuseeArtMaking` (Editor: making the works). Its piece source and its dresser both run at order 30.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `ArtHang.h`, `ArtHangEntry.h`, `ArtWall.h`, `ArtWallPath.h` | `FArtHang`: a stage's `Hang.json`, its curved walls and its works; every field the stages' sheets write |
| `ArtAfter.h`, `ArtScrollMount.h`, `ArtScanPlacement.h`, `ArtCarrier.h` | an entry's original (`after`), a scroll's mount, a cast's scan and carrier |
| `ArtCatalogue.h`, `ArtCatalogueEntry.h`, `ArtGallery.h`, `ArtScan.h` | the museum's catalogues, read as they are written |
| `ArtWork.h` | `FArtWork` (the catalogue's facts, the hang's size, support, care and reproduction, its image, its scan's credit; `Caption`); `Musee::Art::ReadWork`, `FindImage`, `PaintMaterial` (`paint.<id>`, `paint.<id>.<index>`), `RepositoryRoot`, `SourcePath` |
| `ArtCare.h`, `ArtLightDose.h` | `Musee::Art::CareClasses`, `Reproductions`, `ReproductionCredit`; `CareFaults` (a dose against a class; a reproduction not held to care is within under any light) |
| `ArtWorkComponent.h` | `UArtWorkComponent`: a work's label (`IMuseeCaptioned`) and care class, the component its pieces hang from; the tags `musee.work:<id>`, `musee.care:<class>` |
| `ArtWorkImage.h` (making) | `Musee::ArtMaking::ComposeWorkImage`: composes a texture Studio makes from a work's image |
| `ArtPaintLayer.h` (making) | `FArtPaintLayer` and `Musee::ArtMaking::PaintLayers`: each hung work's image, its size and its paint's spec (`paint.<id>` over `accent.oil-on-linen`), which Studio makes before casting |
| `ArtCastScan.h` (making) | `FArtCastScan` and `Musee::ArtMaking::CastScans`: the glTF scans a stage's works are cast from, which Studio imports (Interchange) at `ScanPackage` (`<Root>/Art/Casts/SM_Scan_<Scan>`) before its pieces are made |

Each hung canvas is made as a stretched canvas: its face in its work's own paint, standing 40 mm off the wall on its
stretcher, its tacking edges and back in linen; curved to the wall where the hanging plan gives the wall a path. A work
on several canvases (`"canvas": <index>`) shows its one image across them, each face's UV0 in metres from the image's
top-left corner. A canvas the plan frames (`"frame": "<surface>"`) gets the museum's moulding (`Data/Art/Moulding.json`)
swept round it, mitred, as a closed solid in the finish of that surface.

Canvases that each name their own `image` (the Rouen series) each show it, in their own layer `paint.<id>.<index>`.
A `glass` entry at a `Glazing` anchor (Albion's band, D-014) is a pane of the glass its `wall` surface names (Lamberts'
antique glass, 3 mm), in Form's opening, its image in its own layer over that glass's finish.

**Paper, plates, transparencies and ware.** Each support's layer is a palette finish (`Data/Art/Supports.json`): a print
is `paint.<id>` over `accent.print-on-paper`, a daguerreotype over `accent.daguerreotype` (Matter's plate master: the
image is the cover of scattering particles over a silver mirror), De Morgan's ware over `accent.lustreware` (its
undecorated faces in the same entry's tin glaze), an autochrome over its plate's glass (the stained-glass master).
- A print on a Wall anchor lies on its mount (`mat`, `matSize`, `matDepth`: the mount's face on the anchor's plane), under
  its `glazing` (a pane of the surface's glass, its thickness from Matter's working), in its frame (`frameStyle`
  `photograph`: `Data/Art/Mouldings/photograph.json`; any other style takes the museum's moulding until its own is drawn).
- A card or a daguerreotype's plate lies centred on its reader (a Mount anchor at the plate's face), under its glazing.
- A transparency (glass on a Wall anchor in its window mat: an autochrome) stands behind its mat's window, the diffusing
  pane it names as `backing` behind it, glazed and framed; it is not stained glass, which is leaded in a Glazing light.
- Ware stands at its `centre` on its Mount anchor, its front towards `facing_deg`: a dish turned to the section of
  `Data/Art/Ware.json`, a vessel to its outline in `Data/Art/Turned.json` (traced from its photograph), the photograph
  laid over it from above (a dish) or through it from the front (a vessel).

**Casts: works in the round from their scans.** A `cast` entry (and `ceramic` with a scan) is its scan, stood on its
carrier's top (its Mount anchor) at its `centre`, its front towards `facing_deg`:
- the scan is the collection's: a glTF binary (`assets/classical/*.glb`, the Royal Cast Collection's casts), which
  Studio's make imports with the engine's own glTF import before the stage is made, and Art reads from the imported
  mesh's description; or an `.mvm` (`assets/sculptures`, the old museum's format), which Art reads itself. Both are
  already stood up (y up, the front +z), so a plan's `scan.turn` is the record of that, never applied again;
- it is scaled to its true height (the plan's `size[1]`) and set by its base slice, the scan below `baseSliceMeters`
  (`Data/Art/Casts.json`): that slice's centre stands on `centre`. A plan width (`size[0]`) is checked against the scan's
  at that height; 0 leaves it to the scan;
- its material is its medium's, by the catalogue's `medium` (`Data/Art/Casts.json`: the SMK casts are sized plaster of
  Paris, Degas's dancer a patinated bronze), never the scan's colour; a part in another material (the dancer's cotton
  tulle skirt) is the band of heights the record gives, a piece `cast.<id>.<part>` of its own;
- a scan's UV0 is a box projection in metres (a scan has no drawing's UVs; plaster and bronze have no grain);
- its label credits the scan as its licence asks (`Data/Art/Scans.json`, from the collection's CREDITS.md).
The carrier (the Classical Hall's niche pedestals, consoles, herms and plinths, the Salon's plinth) is the stage's.

Every object is named `<word>.<id>[.<index>]` (`canvas`, `glass`, `paper`, `plate`, `ware`, `cast`), its parts `<name>.<part>`.

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
- Panels, scrolls, tapestry, silk, stone, turned ceramic without a scan, books, and paper framed without a mount: the
  supports and kinds the plans name that Art doesn't make yet. A cast's part by more than a band of heights (the
  dancer's satin ribbon; the hands the skirt's band takes). A vessel's handles (Turned.json
  keeps none yet); a lustre's metallic sheen (the lustre layer is the glaze's gloss over the image); a daguerreotype's
  case.
- Wall labels as objects, fittings (benches, stanchions), vitrines; `UArtImmersiveWork`.
