# 04 — The Reserve, corrected selected final

Selected final: [04-the-reserve-v2.png](04-the-reserve-v2.png)

Created using the built-in image_gen tool. Photorealistic concept visualization of the proposed virtual museum, not a photograph of an existing building. 1536×1024 PNG.

## Full museum context

All 10 folder PDFs were read in full as extracted text before initial generation: Musee-Vision-Design-Plan.pdf; Musee-Vision-Rotunda-Design-Sheet.pdf; all eight numbered PDFs in image-prompts/. All five visual contact sheets and original composition PNGs were inspected. Initial generation attached the matching composition, complete masterplan contact sheet, both Rotunda sheets and the all-guides sheet. New individual guides superseded older masterplan details.

## Artwork correction and source

The first draft's leftmost canvas did not faithfully depict Renoir's Acrobats at the Cirque Fernando. A targeted second call replaced only that artwork using the actual public-domain Art Institute of Chicago image as an attached reference. Official API artwork ID 81558 confirms is_public_domain=true.

- [Official collection record](https://www.artic.edu/artworks/81558/acrobats-at-the-cirque-fernando-francisca-and-angelina-wartenberg)
- [Official API record](https://api.artic.edu/api/v1/artworks/81558?fields=id,title,image_id,is_public_domain,dimensions)
- [Official reference image](https://www.artic.edu/iiif/2/321c45f5-22a3-84a2-44cc-cf66642d4cf2/full/843,/0/default.jpg)
- Local downloaded reference: tmp/imagegen-context/renoir-acrobats-artic-81558.jpg.
- [Art Institute of Chicago — Two Sisters (On the Terrace)](https://publications.artic.edu/renoir/api/epub/135446/135639/print_view): subject and size context.
- [Musée d'Orsay — La Balançoire](https://www.musee-orsay.fr/en/artworks/la-balancoire-1096): subject and size context.

## QA

The corrected Acrobats canvas now closely preserves the museum source composition: two girls in pale gold-accented costumes, oranges held by the right figure and scattered on the circus floor, cropped audience at the top. The brick groin vaults, square columns, one pulled-out mesh rack, other two paintings, visitor, plan chests, edge-on racks, teal tracks, lamps, lighting and composition are visually retained. No page labels, signage or clutter. Exact pixel-level artwork reproduction is not claimed because the final is a generated architectural visualization.

## Actual initial prompt

Use case: sketch-to-render. Create one finished photorealistic architectural photograph-style visualization, full-bleed 1536 × 1024 landscape 3:2.
Input image 1 is the strict composition reference for The Reserve. Render ONLY the interior scene within its large left-hand rectangular panel as a convincing real architectural interior. Discard all page design, borders, title, diagrams, numbers, key, captions and palette chips. Match the camera, horizon, proportions, foreground columns, pulled-out rack and visitor placement. Other input images provide context from ALL museum design PDFs: complete original museum masterplan, complete updated Rotunda design sheet, all eight revised room guides. They set the restrained classical-to-modern-to-future material language. Do not combine rooms or create a collage. New individual image guide takes precedence over older masterplan.

PRIMARY USER PROMPT:
The Reserve: a secret storage cellar five metres below the Nymphéas Oval, reached through a hidden door and a spiral stair. We look east down a central viewing aisle under warm brick groin vaults carried on square brick columns. On the right, a row of tall brass-mesh painting racks, 3 m high and 5 m long, stands edge-on to us on floor tracks lit by thin teal rails. One rack has been pulled right out across the aisle with its mesh face towards us, holding three Impressionist canvases in gilt frames: Renoir's Acrobats at the Cirque Fernando, Two Sisters (On the Terrace) and The Swing. A single visitor studies them. On the left, low wooden plan chests for works on paper; pendant lamps hang from the vaults. Hushed, intimate, archival: warm tungsten pools of light, deep shadow, a little dust in the air. Photorealistic architectural photograph, eye level 1.6 m, 20 mm wide-angle lens, 3:2 landscape, straight verticals unless the view says the camera tilts up. Classical architecture with restrained modern detailing and only a light touch of the future. Physically accurate natural light, real material textures. Paintings are faithful reproductions of the named public-domain works at true scale in gilt frames. No captions, signage, logos or UI overlays (inscriptions cut into the architecture are fine); at most one or two small visitors in neutral clothing for scale.

GUIDE-SPECIFIC REQUIREMENTS:
Brick GROIN vaults have intersecting curved masonry surfaces, real individually laid warm weathered bricks and pale mortar joints, carried on SQUARE BRICK columns (not round pillars, not plaster).
Only one pulled-out brass mesh rack faces the camera, bearing exactly the three Renoirs in a single row. Approximate unframed artwork physical sizes: Acrobats at the Cirque Fernando 131.2×99.2cm (two young circus performers, one carrying oranges), Two Sisters 100.4×80.9cm (seated woman in red hat and blue dress, small child, leafy terrace), The Swing 92×73cm (woman in pale dress with blue bows near swing, man in straw hat seen from behind, dappled garden). Preserve recognizable public-domain artwork compositions, colors and differing true sizes; never substitute arbitrary invented paintings. They appear as oil canvases hung on brass mesh in gilt frames.
Remaining tall narrow racks remain edge-on on RIGHT in orderly parallel tracks, with slim dim teal illuminated floor rails. Low wooden drawing plan chests on LEFT. One visitor in muted neutral clothing near the right of the pulled-out rack, looking at the paintings, no facial emphasis. Warm pendant lamps form isolated pools of tungsten illumination, with deep soft shadows and a quiet archival atmosphere. Physically accurate metal mesh, gently aged brass, stone floor with subtle polish, real wood grain. No industrial warehouse shelving, bright even lighting, white-cube museum storage, crates, clutter, signage, numbers, UI overlays or watermark. No glossy CGI/toy look.

## Actual targeted correction prompt

Use case: compositing / precise-object-edit.
Input image 1 is the finished photorealistic museum cellar scene, the edit target. Input image 2 is the authentic public-domain Renoir painting Acrobats at the Cirque Fernando from the Art Institute of Chicago and is the exact artwork insert/reference.
Make ONE LOCALIZED correction only: replace the inaccurate image inside the LEFTMOST gilt frame on the pulled-out brass-mesh rack with a faithful reproduction of image 2. Keep that gilt frame's outer boundary, dimensions, position, perspective and light unchanged. The replacement canvas must preserve the actual source artwork's complete composition and proportions: two GIRLS side by side in pale white-and-gold acrobat costumes, girl on left turned toward the right with hands raised near her chest, girl on right holding orange fruit across her torso, orange balls/fruit on the yellow-brown circus ground, cropped dark audience along the top. Do not reimagine the figures. No standing figure with arms raised above the head, no dark blue costume. Fit the supplied artwork faithfully inside this existing frame and retain its full composition with source aspect ratio. Match the physical warm cellar light and slight image perspective so it remains a real oil painting hung in this setting.
Preserve EVERYTHING outside that single canvas interior: all architecture, brick groin vaults, columns, lamps, floor, reflections, brass mesh, rack positions, teal tracks, visitor, camera, framing, existing gilt frame, and the center and right paintings. Do not change any other artwork. Maintain 1536×1024, 3:2. No added text, labels, borders or watermarks.

