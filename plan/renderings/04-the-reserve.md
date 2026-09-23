# 04 — The Reserve

This first draft is superseded by [04-the-reserve-v2.png](04-the-reserve-v2.png), which corrects the leftmost painting using the official public-domain museum image. See [updated prompt and QA notes](04-the-reserve-v2.md).

Final image: [04-the-reserve.png](04-the-reserve.png)

## Context reviewed before generation

All 10 folder PDFs were read in full as extracted text before generation: Musee-Vision-Design-Plan.pdf; Musee-Vision-Rotunda-Design-Sheet.pdf; all eight numbered PDFs in image-prompts/. All five PDF context contact sheets were visually inspected, along with this view's original composition PNG. The newer individual view guides were treated as authoritative where they superseded the original design plan.

Generated with the built-in image_gen tool. These are photorealistic visualizations of the proposed virtual museum, not photographs of a constructed building. Named artwork depictions are generated approximations, not archival or pixel-exact reproductions. Final output 1536×1024 PNG.

## References

- Primary composition: image-prompts/04-the-reserve-composition.png.
- [Art Institute of Chicago — Acrobats at the Cirque Fernando](https://publications.artic.edu/node/135637): subject and physical size.
- [Art Institute of Chicago — Two Sisters (On the Terrace)](https://publications.artic.edu/renoir/api/epub/135446/135639/print_view): subject and physical size.
- [Musée d'Orsay — La Balançoire](https://www.musee-orsay.fr/en/artworks/la-balancoire-1096): subject, colors and physical size.
- Generation attached the composition PNG, complete combined Design-Plan sheet, both Rotunda sheets and the all-guides sheet. Thus all ten PDFs were represented in the five attached visual context images.

## QA

Reviewed output at full displayed size. Brick groin vaults and square brick columns, one pulled-out brass mesh rack facing the camera with three canvases, edge-on racks on right, subtle teal floor tracks, low wooden plan chests on left, warm pendant pools of light, deep shadows and one visitor are present. No clutter, crates, warehouse shelving, guide numbers or signage. The three paintings convey the named works, but details of the figures, especially Acrobats, are generated interpretations rather than exact reproductions.

## Actual prompt

Use case: sketch-to-render. Create one finished photorealistic architectural photograph-style visualization, full-bleed 1536 × 1024 landscape 3:2.
Input image 1 is the strict composition reference for The Reserve. Render ONLY the interior scene within its large left-hand rectangular panel as a convincing real architectural interior. Discard all page design, borders, title, diagrams, numbers, key, captions and palette chips. Match the camera, horizon, proportions, foreground columns, pulled-out rack and visitor placement. Other input images provide context from ALL museum design PDFs: complete original museum masterplan, complete updated Rotunda design sheet, all eight revised room guides. They set the restrained classical-to-modern-to-future material language. Do not combine rooms or create a collage. New individual image guide takes precedence over older masterplan.

PRIMARY USER PROMPT:
The Reserve: a secret storage cellar five metres below the Nymphéas Oval, reached through a hidden door and a spiral stair. We look east down a central viewing aisle under warm brick groin vaults carried on square brick columns. On the right, a row of tall brass-mesh painting racks, 3 m high and 5 m long, stands edge-on to us on floor tracks lit by thin teal rails. One rack has been pulled right out across the aisle with its mesh face towards us, holding three Impressionist canvases in gilt frames: Renoir's Acrobats at the Cirque Fernando, Two Sisters (On the Terrace) and The Swing. A single visitor studies them. On the left, low wooden plan chests for works on paper; pendant lamps hang from the vaults. Hushed, intimate, archival: warm tungsten pools of light, deep shadow, a little dust in the air. Photorealistic architectural photograph, eye level 1.6 m, 20 mm wide-angle lens, 3:2 landscape, straight verticals unless the view says the camera tilts up. Classical architecture with restrained modern detailing and only a light touch of the future. Physically accurate natural light, real material textures. Paintings are faithful reproductions of the named public-domain works at true scale in gilt frames. No captions, signage, logos or UI overlays (inscriptions cut into the architecture are fine); at most one or two small visitors in neutral clothing for scale.

GUIDE-SPECIFIC REQUIREMENTS:
Brick GROIN vaults have intersecting curved masonry surfaces, real individually laid warm weathered bricks and pale mortar joints, carried on SQUARE BRICK columns (not round pillars, not plaster).
Only one pulled-out brass mesh rack faces the camera, bearing exactly the three Renoirs in a single row. Approximate unframed artwork physical sizes: Acrobats at the Cirque Fernando 131.2×99.2cm (two young circus performers, one carrying oranges), Two Sisters 100.4×80.9cm (seated woman in red hat and blue dress, small child, leafy terrace), The Swing 92×73cm (woman in pale dress with blue bows near swing, man in straw hat seen from behind, dappled garden). Preserve recognizable public-domain artwork compositions, colors and differing true sizes; never substitute arbitrary invented paintings. They appear as oil canvases hung on brass mesh in gilt frames.
Remaining tall narrow racks remain edge-on on RIGHT in orderly parallel tracks, with slim dim teal illuminated floor rails. Low wooden drawing plan chests on LEFT. One visitor in muted neutral clothing near the right of the pulled-out rack, looking at the paintings, no facial emphasis. Warm pendant lamps form isolated pools of tungsten illumination, with deep soft shadows and a quiet archival atmosphere. Physically accurate metal mesh, gently aged brass, stone floor with subtle polish, real wood grain. No industrial warehouse shelving, bright even lighting, white-cube museum storage, crates, clutter, signage, numbers, UI overlays or watermark. No glossy CGI/toy look.
