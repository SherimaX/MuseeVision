# 02 · Bay 2: Modern Paris

Generation method: built-in image_gen; one photorealistic concept visualization.

## Actual generation prompt

Use case: sketch-to-render. Create one finished, extremely photorealistic architectural image for proposed Musée Vision museum, view 02 Modern Paris. This is a visualization of a proposed virtual museum.
Input image 1 is the precise composition target. It is a guide page: render ONLY its large central-left architectural scene, full bleed. Exclude the entire page layout, margins, title, key, palette swatches, plan diagram, all guide numbers and all other text. Preserve this scene's camera, horizon, proportions, depth, and placement, replacing flat colours and placeholders with tangible real materials, accurately reproduced art, and photographic light.
Input images 2–5 are background context from all ten supplied PDFs: complete Design Plan, complete Rotunda Design Sheet, all eight image-guide compositions. They establish coherent shared stone/gilt/bronze/glass materials and progression from classical architecture to modern light to futuristic space. All ten PDF texts were read first. Context images are not output layouts. Specific current view 02 prompt below supersedes older design-plan differences. Five bays of 84m Salon Impression lead to the Nymphéas Oval; this second bay is Paris-grey and the following third bay sage.

Exact source prompt:
Inside Bay 2 of Salon Impression, 'Modern Paris', seen from its south-west corner looking north-east. The north wall, covered in cool Paris-grey fabric, carries Caillebotte's Paris Street; Rainy Day (212 × 276 cm) at its centre, flanked by Pissarro's The Boulevard Montmartre on a Winter Morning on the left and The Boulevard Montmartre at Night on the right, each in a gilt frame at true scale and washed by a hidden picture light. In the right foreground, Degas's Little Dancer Aged Fourteen (bronze cast, 98 cm tall, with a real fabric tutu and ribbon) stands on a pale stone plinth 0.9 m high. Overhead, a coffered stone barrel vault; on the right a stone transverse arch frames the next bay, whose walls are sage green. Travertine floor, a thin shadow gap instead of a skirting board. Soft overcast daylight, like a rainy Paris afternoon. Photorealistic architectural photograph, eye level 1.6 m, 20 mm wide-angle lens, 3:2 landscape, straight verticals unless the view says the camera tilts up. Classical architecture with restrained modern detailing and only a light touch of the future. Physically accurate natural light, real material textures. Paintings are faithful reproductions of the named public-domain works at true scale in gilt frames. No captions, signage, logos or UI overlays (inscriptions cut into the architecture are fine); at most one or two small visitors in neutral clothing for scale.

Must include: exactly three paintings on the grey north wall, with the very large horizontal Caillebotte canvas centrally hung, two much smaller horizontal Pissarro canvases flanking; Little Dancer on pale stone plinth in the right foreground, clear of all three paintings; monumental stone transverse arch on right opens to sage-green bay; coffered barrel vault; overcast rainy-afternoon skylight and subtle warm hidden picture lights.
Avoid: any extra north-wall paintings, moving the dancer in front of Caillebotte, glass case, direct sunny shafts, visitors blocking art, signage, numbers, stacked hanging, fisheye distortion.
Artwork reference details: Caillebotte's public-domain Paris Street; Rainy Day is a wide painting dominated by Haussmann street facades converging left of centre, a green lamppost near centre, foreground well-dressed couple under grey umbrella on the right, wet cobbles and distant umbrella-bearing walkers. Left Pissarro is a small elevated winter boulevard view with pale muted tan-grey facades, bare trees, tiny pedestrians and carriages. Right Pissarro is a small blue-violet night boulevard view with yellow streetlight reflections and dark buildings. Degas bronze dancer stands with chin up, hands clasped behind back, one foot extended in front, cream fabric tutu, hair tied with a real pale ribbon; bronze body rather than a living dancer. Reference museum sources: Art Institute of Chicago, Metropolitan Museum of Art, National Gallery London.
Finish: believable architectural photography, fine grey fabric weave, pale honed travertine, lightly textured creamy stone, physical bronze patina and delicate real tulle, photographically detailed gilt frames, no CG flatness. Landscape 3:2, 1536 × 1024.

## Context reviewed before generation

All ten PDF text extracts and five contact sheets covering the complete Design Plan, complete Rotunda Design Sheet, and all eight guide compositions. Individual composition guide and prompts.md control the view.

## Reference research

- [Art Institute of Chicago — Paris Street; Rainy Day](https://archive.artic.edu/500ways/artwork/20684)
- [Metropolitan Museum of Art — The Boulevard Montmartre on a Winter Morning](https://www.metmuseum.org/art/collection/search/437310)
- [National Gallery — The Boulevard Montmartre at Night](https://www.nationalgallery.org.uk/paintings/camille-pissarro-the-boulevard-montmartre-at-night)
- [Metropolitan Museum of Art — The Little Fourteen-Year-Old Dancer](https://www.metmuseum.org/art/collection/search/196439)

External references inform artwork identification; supplied composition and museum design documents control architecture.

## Artwork fidelity correction

The first render had approximate artwork. The following targeted built-in imagegen correction uses downloaded public-domain painting images as insertion references while retaining the generated room.

Use case: compositing / precise-object-edit.
Image 1 is the EDIT TARGET: the completed photorealistic Modern Paris museum gallery. Keep its entire architecture, camera, composition, grey fabric walls, vaults, floor, sage bay, lighting, sculpture, plinth, and gilt frames exactly unchanged. Make one localized correction: replace only the painted image content inside the THREE gilt frames on the foreground Paris-grey north wall with faithfully reproduced source artwork from reference images 2, 3, and 4, perspective-mapped to the existing frames and naturally lit.
Image 2 is the exact public-domain Caillebotte Paris Street; Rainy Day artwork source. Insert the entire painting into the LARGE CENTRAL frame, preserving its authentic original composition, faces, clothing, building shapes, lamppost, and brushwork. Do not redraw it loosely.
Image 3 is the exact public-domain Pissarro The Boulevard Montmartre on a Winter Morning source. Insert its entire painted canvas, excluding black border and raw canvas edge, into the SMALL LEFT frame. Preserve its elevated hotel-window viewpoint, pale grey atmosphere, facade shape, bare trees, road perspective and painterly dabs. Do not turn it into an eye-level street scene.
Image 4 is the exact public-domain Pissarro The Boulevard Montmartre at Night source. Insert its entire painted canvas into the SMALL RIGHT frame. Preserve its elevated viewpoint, deep violet sky, slanted blue rooftops, many small electric-light dabs, and broad loose brushwork.
Keep all artwork fully within frames, without changing their placement or adding any frames. Preserve every other pixel-level feature as closely as possible. No text, labels, watermarks. Output same 1536 × 1024 landscape image. Photographic integration, with realistic existing soft warm art lighting, but no alterations to the artwork compositions. This follows the museum brief from all ten PDFs already reviewed.

Downloaded image sources:

- https://www.artic.edu/iiif/2/f8fd76e9-c396-5678-36ed-6a348c904d27/full/843,/0/default.jpg
- https://collectionapi.metmuseum.org/api/collection/v1/iiif/437310/2035868/main-image
- https://upload.wikimedia.org/wikipedia/commons/8/82/Camille_Pissarro%2C_The_Boulevard_Montmartre_at_Night%2C_1897.jpg

## QA

- Final selected image: 02-modern-paris.png, 1536 × 1024; the artwork-corrected version.
- Preserves three paintings on Paris-grey north wall, large Caillebotte between smaller Pissarros, bronze Little Dancer with fabric tutu on the right foreground plinth, opening to sage bay, coffered vault, travertine floor and soft overcast light.
- Targeted artwork correction substantially improved both Pissarro compositions using actual painting references, while retaining room geometry and sculpture.
- Output inspected visually. No guide layout, guide numbers, signage, captions, watermark or glass case.
- Limitation: imagegen still interprets rather than exactly composites painting details, especially Caillebotte figures. Artwork sizes and room dimensions are visual approximations, not measured construction geometry. This is a photorealistic visualization of a proposed virtual museum.
