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

**Paper, plates, transparencies and ware.** Each support's layer is a palette finish (`Data/Art/Supports.json`): a print
is `paint.<id>` over `accent.print-on-paper`, a daguerreotype over `accent.daguerreotype` (Matter's plate master: the
image is the cover of scattering particles over a silver mirror), De Morgan's ware over `accent.lustreware` (its
undecorated faces in the same entry's tin glaze), an autochrome over its plate's glass (the stained-glass master).
- A print on a Wall anchor lies on its mount (`mat`, `matSize`, `matDepth`: the mount's face on the anchor's plane, or,
  where the plan gives an `offWall`, the mount's back that far off the wall), under its `glazing` (a pane of the surface's
  glass, its thickness from Matter's working), in its frame (`frameStyle` `photograph` or `panel`:
  `Data/Art/Mouldings/<style>.json`; any other style takes the museum's moulding until its own is drawn).
- A card or a daguerreotype's plate lies centred on its reader (a Mount anchor at the plate's face), under its glazing.
- A transparency (glass on a Wall anchor in its window mat: an autochrome) stands behind its mat's window, the diffusing
  pane it names as `backing` behind it, glazed and framed; it is not stained glass, which is leaded in a Glazing light.
- Ware stands at its `centre` on its Mount anchor, its front towards `facing_deg`: a dish turned to the section of
  `Data/Art/Ware.json`, a vessel to its outline in `Data/Art/Turned.json` (traced from its photograph), the photograph
  laid over it from above (a dish) or through it from the front (a vessel).

**Panels, tapestry, scrolls and books.**
- A panel (`panel`) hangs on its Wall anchor as a canvas does: its paint `paint.<id>` over `accent.oil-on-panel` on a
  board, the board on its cradle (members along the grain, slides across them), both in the wood its `wall` surface
  names; `Data/Art/Panel.json` gives the board, ground and cradle. Framed as a canvas is.
- A tapestry (`tapestry`) is its weave, `paint.<id>` over `accent.tapestry`, 3 mm thick, its back `offWall` off its Wall
  anchor on the batten and lining the stage builds.
- A hanging scroll (`kind` `hanging-scroll`, on `silk` or `paper`) hangs by its cord `offWall` in front of its Wall
  anchor: the painting (`accent.ink-on-silk` or `accent.ink-on-paper`) at its `along` and `height`, laid into its mount
  (the `frame` surface's silk: `mount.border` either side, `mount.band` and `mount.earth` below, `mount.band` and
  `mount.heaven` above); the half-round top stick behind the mount's top edge and the roller in its foot, both in the
  mount's silk; the roller's knobs (`mount.knob`); the cord (`mount.cord`) from two eyes on the stick to its hook
  `mount.hook` above the stick's middle. `Data/Art/Scrolls.json` gives the fittings every mount shares.
- A book (`kind` `book`) lies open at its `centre` on a Mount anchor (a case's bed, a lectern's desk; the anchor's z out
  of it), its foot to its reader (`facing_deg`): on each side its board (its cover, standing its square past the leaves),
  the leaves that side holds at its `opening`, and the page on top. One object is the opening whole, showing its one
  image across both pages; a work's two book objects are its verso and recto, each its own image. `Data/Art/Books.json`
  gives each book's leaves, block, boards and finishes. A leaf (`kind` `leaf`) lies flat at its centre the same way.

Every object is named `<word>.<id>[.<index>]` (`canvas`, `panel`, `glass`, `paper`, `silk`, `plate`, `ware`,
`tapestry`), its parts `<name>.<part>`.

A work the stage builds (`"built": "form"`) gets only its caption and care: the stage names its pieces
`work.<id>[.<index>][.<part>]`, and the dresser hangs them from the work's label. A reproduction (`"reproduction":
"exhibition-facsimile"`, Care.json's `reproductions`) keeps its original's care class and credits the original; care
accepts it under any light. `after` records the original's size for a work shown at another (`TimesOriginal`).

The collection's catalogues and images are read from MUSEE_COLLECTION, this working copy, or the repository's main
working tree when this copy is a linked worktree that doesn't check them out.

`Musee.Art.EveryHangResolves` holds every stage's Hang.json to Art's reader: every entry resolves to a work its catalogue
holds, made by Art or built by its stage.

## Later (tier 2, owner Art)

- More mouldings (Albion's reeded and Watts frames and their gilt slips, the oval and arched sights), carved ornament on
  the moulding; the hairline joints between a work's canvases; the stretcher's wood and keys; frames on curved walls.
- Handscrolls on their beds (lead, wrapper and rolled end); an open book's leaves curving up to its spine (they lie
  flat); a scroll's jingyan ribbons.
- `Musee::Art::Targets` (FLightTarget from Care.json) and the checks `Art.TrueSize` (with `after`), `Art.WithinCare`
  (over `CareFaults`), `Art.ColorManaged`, `Art.Credited`, `Art.HangResolves`.
- Casts, stone, ceramic in the round other than De Morgan's ware, and handscrolls: the supports and kinds the plans
  name that Art doesn't make yet. A vessel's handles (Turned.json
  keeps none yet); a lustre's metallic sheen (the lustre layer is the glaze's gloss over the image); a daguerreotype's
  case.
- Wall labels as objects, fittings (benches, stanchions), vitrines; `UArtImmersiveWork`.
