# Art

The collection as real objects. Modules `MuseeArt` (Runtime: the catalogue, the hang, care, the work component) and
`MuseeArtMaking` (Editor: making the works). Its piece source and its dresser both run at order 30.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `ArtHang.h`, `ArtHangEntry.h`, `ArtWall.h`, `ArtWallPath.h`, `ArtMesh.h` | `FArtHang`: a stage's `Hang.json`, its curved walls, the meshes works hang on and its works; every field the stages' sheets write |
| `ArtAfter.h`, `ArtScrollMount.h`, `ArtScanPlacement.h`, `ArtCarrier.h` | an entry's original (`after`), a scroll's mount, a cast's scan and carrier |
| `ArtCatalogue.h`, `ArtCatalogueEntry.h`, `ArtGallery.h`, `ArtScan.h` | the museum's catalogues, read as they are written |
| `ArtWork.h` | `FArtWork` (the catalogue's facts, the hang's size, support, care and reproduction, its image, its scan's credit; `Caption`); `Musee::Art::ReadWork`, `FindImage`, `FindSectionImages` (a work's numbered photographs, from one folder, distinct), `PaintMaterial` (`paint.<id>`, `paint.<id>.<index>`), `RepositoryRoot`, `SourcePath` |
| `ArtCare.h`, `ArtLightDose.h` | `Musee::Art::CareClasses`, `Reproductions`, `ReproductionCredit`; `CareFaults` (a dose against a class; a reproduction not held to care is within under any light) |
| `ArtWorkComponent.h` | `UArtWorkComponent`: a work's label (`IMuseeCaptioned`) and care class, the component its pieces hang from; the tags `musee.work:<id>`, `musee.care:<class>` |
| `ArtWorkImage.h` (making) | `Musee::ArtMaking::ComposeWorkImage`, `ComposeLayerImage` and `LayerImageIdentity`: compose a texture Studio makes from a work's image, or a painted vessel's decoration from its views, and say what it is made from |
| `ArtPaintLayer.h` (making) | `FArtPaintLayer` and `Musee::ArtMaking::PaintLayers`: each hung work's image, its size and its paint's spec (`paint.<id>` over `accent.oil-on-linen`), which Studio makes before casting |
| `ArtCastScan.h` (making) | `FArtCastScan` and `Musee::ArtMaking::CastScans`: the glTF scans a stage's works are cast from, which Studio imports (Interchange) at `ScanPackage` (`<Root>/Art/Casts/SM_Scan_<Scan>`) before its pieces are made |

Each hung canvas is made as a stretched canvas: its face in its work's own paint, standing 40 mm off the wall on its
stretcher, its tacking edges and back in linen; curved to the wall where the hanging plan gives the wall a path. A work
on several canvases (`"canvas": <index>`) shows its one image across them, each face's UV0 in metres from the image's
top-left corner. A canvas the plan frames (`"frame": "<surface>"`) gets the museum's moulding (`Data/Art/Moulding.json`)
swept round it, mitred, as a closed solid in the finish of that surface.

**Works on a mesh.** A Wall anchor the plan names in `meshes` (`ArtMesh.h`: a store's rack, its level and upright wires
behind the anchor's plane) has its works hung from the mesh, never floating off it: each framed canvas there hangs by a
pair of S-hooks (`<name>.hook.1`, `.2`) through screw eyes in its frame's back (`<name>.eye.1`, `.2`), in the mesh's
`hooks` surface. The eyes' rings lie level against the frame's back, a third of the frame down, each moved to the gap
between two uprights; each hook rises from its eye to the lowest level wire that leaves it its least shank, passes over
it and turns down behind it (`Data/Art/Hooks.json`). A mesh too close behind the frame for the hooks' bends, no wire
high enough, or a work with no frame to hang by is a finding.

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

**Casts: works in the round from their scans.** A `cast` entry (and `ceramic` with a scan) is its scan, stood on its
carrier's top (its Mount anchor) at its `centre`, its front towards `facing_deg`:
- the scan is the collection's: a glTF binary (`assets/classical/*.glb`, the Royal Cast Collection's casts), which
  Studio's make imports with the engine's own glTF import before the stage is made, and Art reads from the imported
  mesh's description; or an `.mvm` (`assets/sculptures`, the old museum's format), which Art reads itself. Both are
  already stood up (y up, the front +z), so a plan's `scan.turn` is the record of that, never applied again;
- it is scaled to its true height (the plan's `size[1]`) and set by its base slice, the scan below `baseSliceMeters`
  (`Data/Art/Casts.json`): that slice's centre stands on `centre`. A plan width (`size[0]`) is checked against the scan's
  at that height; 0 leaves it to the scan;
- its material is its medium's, by the catalogue's `medium` (`Data/Art/Media.json`: the SMK casts are sized plaster of
  Paris, Degas's dancer a patinated bronze), never the scan's colour; a part in another material (the dancer's cotton
  tulle skirt) is the band of heights `Data/Art/Casts.json` gives, a piece `cast.<id>.<part>` of its own;
- a scan's UV0 is a box projection in metres (a scan has no drawing's UVs; plaster and bronze have no grain);
- its label credits the scan as its licence asks (`Data/Art/Scans.json`, from the collection's CREDITS.md).
The carrier (the Classical Hall's niche pedestals, consoles, herms and plinths, the Salon's plinth) is the stage's.

**Turned vessels.** `ceramic` without a scan (Chenghuai's cases) is turned on Form's lathe from its section in
`Data/Art/Vessels.json` (the stage's sheet's traced outlines), scaled to its plan's size, smooth through its traced points
but where it turns more than the record's crease angle (lip, foot ring), in its medium's glaze (`Data/Art/Media.json`:
Ding ivory over porcelain; Jun, Guan, hare's fur and celadon over stoneware), standing at its `centre` on its Mount anchor
and, where its plan says `stand`, on its carved phoebe stand (`ware.<id>.stand`), both sized by the record.

**Vessels from their photographs** (grade `derived` in `Data/Art/Vessels.json`, made by `MuseeVision/Scripts/vessels.py`,
each derivation in its record): the section as the vessel's own photograph shows it through its fitted camera, at its
museum's size (`museum`); an oval one (the Ru basin) turned and pressed front to back to its foot's oval and its
mouth's, evenly between (`Vessel/VesselSection.h` `PressOf`, `Vessel/VesselMesh.h`), on its feet (`ware.<id>.foot.<n>`).
A painted one (`views`) wears the work's own layer `paint.<id>` over its medium's finish (`accent.porcelain-painted`): its
decoration composed by `Vessel/VesselDecoration.h` in the layout its UV0 carries (U the widest parallel's metres round
from the front, V the section's metres along), each view projected through its camera onto all the surface it sees,
obliquely too (where no other part of the vessel hides it, inside its silhouette by the camera's miss and an edge's blur),
views blended by the square of how squarely they see it and fading into the glaze over the last 12 degrees to their
silhouettes, each photograph divided by its own white, and where none sees the outside a band's own repeat (`bands`) or
the glaze. Its registered views are checked against the overlaps that placed them (`PlacementFaults`), measured again
from their cameras and photographs. Its label credits its photographs
(the catalogue's `credit`, `MuseeVision/Data/Art/Ceramics.json`).

**Scholar's rocks.** `stone` (Chenghuai's 石兄 and its Lingbi rock) is formed as its kind of rock is
(`Data/Art/Stones.json`, `Stone/StoneMesh.h`): a signed distance field of its kind's profile of ellipses swaying as it
rises, cut flat at its foot and under its peaks, pierced by its holes, pitted and wrinkled, found by surface nets on a grid
of the kind's cells up its height; its plan's height, its width as wide as the record's foot radius lets it be, in its
medium's stone (`Data/Art/Media.json`), standing at its `centre` on its Mount anchor. A rock whose plan says `stand` stands on the carved
stand (座) Art carves to it (`Stone/StoneStand.cpp`, the record's `stand`): its top recessed to the rock's own foot, the
rock sitting down into it, an even lip, a bead round its side and short feet, in phoebe; its part is `<name>.stand`.

Every object is named `<word>.<id>[.<index>]` (`canvas`, `glass`, `paper`, `plate`, `ware`, `cast`, `stone`), its parts `<name>.<part>`.
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
- A handscroll (`kind` `handscroll`, on `silk` or `paper`) lies opened on its case's bed (a Wall anchor: x along the bed,
  y out of its face, z up its slope), read from right to left:
  - its painting or writing at its `along` and `height`, at its size, from its photographs: its one photograph, or its
    numbered ones (`<id>-1`, `-2`..., `Musee::Art::FindSectionImages`) joined in the reading order
    `Data/Art/Handscrolls.json` gives, right to left, with the evidence it was read from (where each opens and closes,
    the seams, a whole view's correlation); a photograph's `window` (`from`, `to` along its width, `top`, `bottom` down
    its height) keeps off the sheet, and out of the proportion, what it caught beside the work: the mount's silk at the
    head and foot, a separator, a frontispiece, a neighbouring sheet;
  - each photograph a panel of the sheet (`<name>.section.<n>`) in its own layer `paint.<id>.<n>` over the ink on its
    silk or paper, at its own proportion, never stretched: joined, they are laid to cover the sheet, what overruns it cut
    evenly from two opposite edges; a proportion that strays from the catalogue's by more than
    `Data/Art/Handscroll.json`'s tolerance (3 %) is a finding, and the work is made all the same;
  - its mount in the `frame` surface's silk (`<name>.mount`): `mount.border` above and below, `mount.band` at the
    painting's two ends, and beyond its beginning the lead (`mount.lead`) and the wrapper as far as it lies unrolled
    (`mount.wrapper`);
  - at the right, the stave (`<name>.stave`, a half-round) with the rest of the wrapper wound round it
    (`<name>.wrapper`), as long as `wrapperTurns` takes round the scroll rolled whole, and over it what of the mount's
    separator and frontispiece (`separatorMeters`, `frontispieceMeters` in `Data/Art/Handscrolls.json`, the museum's
    record) the band and lead don't lay open; a wrapper laid unrolled over a front still wound is a finding;
  - at the left, the unshown length wound on the roller to `mount.roll_radius` (`<name>.roll`), the roller
    (`<name>.roller`) and its knobs (`<name>.knob.foot`, `.head`, in `mount.knob`'s surface) past the scroll's edges.
  `Data/Art/Handscroll.json` gives the fittings every handscroll shares and their wood; the silk's thickness is
  `Data/Art/Scrolls.json`'s. A work's image is clamped at its edges, so panels meet without a seam.
- A book (`kind` `book`) lies open at its `centre` on a Mount anchor (a case's bed, a lectern's desk; the anchor's z out
  of it), its foot to its reader (`facing_deg`): on each side its board (its cover, standing its square past the leaves),
  the leaves that side holds at its `opening`, and the page on top. One object is the opening whole, showing its one
  image across both pages; a work's two book objects are its verso and recto, each its own image. `Data/Art/Books.json`
  gives each book's leaves, block, boards and finishes. A leaf (`kind` `leaf`) lies flat at its centre the same way.

Every object is named `<word>.<id>[.<index>]` (`canvas`, `panel`, `glass`, `paper`, `silk`, `plate`, `ware`,
`tapestry`), its parts `<name>.<part>`.

**Works on moving bodies.** An entry whose anchor rides a moving body (a store's rack) names it, `"body": "<Body
anchor>"`, as Lighting's and Planting's entries do. Its pieces are made where the anchor stands at rest and carried on
that body, each piece's frame in the body's frame, so Studio hangs them from the body's component and they move, and
are guarded, with it; the dresser hangs the work's label where its pieces hang. A body that isn't a Body anchor of the
stage is a finding.

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
- A handscroll's tie and toggle, its wrapper's own brocade and the backing paper its rolls show; an open book's leaves curving up to its spine (they lie
  flat); a scroll's jingyan ribbons.
- `Musee::Art::Targets` (FLightTarget from Care.json) and the checks `Art.TrueSize` (with `after`), `Art.WithinCare`
  (over `CareFaults`), `Art.ColorManaged`, `Art.Credited`, `Art.HangResolves`.
- Panels, scrolls, tapestry, silk, stone, books, and paper framed without a mount: the supports and kinds the plans name
  that Art doesn't make yet. A cast's part by more than a band of heights (the dancer's satin ribbon; the hands the
  skirt's band takes). Painted porcelain's inside and underside (no view is composed onto them: the glaze); the far side
  of a vessel no photograph shows (its bands carried round, its field the glaze); a sancai lead glaze
  (Matter has none); a vessel's rim band or repair (the Ding basin's copper, the Longquan vase's lacquer and gold). A vessel's handles (Turned.json
- Casts, stone, and ceramic in the round other than De Morgan's ware: the supports and kinds the plans
  name that Art doesn't make yet. A vessel's handles (Turned.json
  keeps none yet); a lustre's metallic sheen (the lustre layer is the glaze's gloss over the image); a daguerreotype's
  case.
- Wall labels as objects, fittings (benches, stanchions), vitrines; `UArtImmersiveWork`.
