# 05 · The Rotunda

Generated architectural visualization of the proposed museum; not a photograph of an existing museum.

## Actual generation prompt

Use case: sketch-to-render.
Asset type: final photorealistic architectural visualization of the proposed Musée Vision virtual museum, a speculative concept, not documentary photography of an existing building.
Input image roles: Image 1 is the PRIMARY COMPOSITION REFERENCE/EDIT TARGET, a design guide page. Transform ONLY its large rectangular scene into a full-bleed finished photograph. Do not reproduce page layout, title, text, sidebar, palette, border, or numbered circles. Images 2–5 are supporting whole-project context sheets from all 10 supplied PDFs, not output layouts. All documents have been read before this request; details from the specific current image guide and prompt take precedence over earlier design plan.
Whole-project context: Musée Vision progresses through classical proportion and stone, restrained modern light, then future glass. Current sequence is 84 m Salon Impression in five fabric-colored bays (oxblood, Paris grey, sage, dusty rose, ochre), Nymphéas Oval with curved unframed Monet panels and a still lily pond, brick-vault Reserve with brass mesh painting racks, marble Rotunda with bronze sundial, long concrete Hall of Light, circular Future Atrium of twelve leaning twisting ribs with central round glass elevator, enormous 150 m dark-blue Sphere above. Materials are real pale marble/travertine, gilt, bronze, smooth concrete and low-iron glass, at human scale. Avoid generic sci-fi styling.
The Rotunda's more detailed design PDF replaces the old floor map with a bronze sun clock: Ø20 m room, Ø5 m regular triangular slim steel and low-iron glass oculus with modest 0.8 m rise and bronze centre node, five rings of 28 coffers, pale marble drum and 150 mm shadow gap, four arched portals and four empty niches, Ø14 m sundial with Roman numeral hours and a gilt centre sun, natural light. Modern steel/glass detailing like the British Museum Great Court should look plausible, precise and restrained.
Render real material texture, credible construction joints, natural reflections, realistic optical exposure and subtle imperfections, as a high-end architectural photograph. No captions, branding, UI, watermark, guide labels, or page furniture. Landscape 3:2, 1536 × 1024.

EXACT SOURCE PROMPT:

The Rotunda, the hub of Musée Vision, from just inside its west portal looking east across the room, the camera tilted up so the dome fills the upper frame. A pale marble drum 20 m across with slender pilasters, tall arched portals and shallow empty niches on the diagonals, crowned by a coffered hemispherical dome with five rings of 28 coffers. The eye of the dome, 5 m wide, is glazed with a triangulated steel-and-glass lattice in the spirit of the British Museum's Great Court roof, with a single bronze node at its centre. The floor is a 14 m bronze sun clock inlaid in pale marble: a bronze ring with Roman numerals from VI through XII to VI, a gilt meridian line running north, a small gilt sun rosette at the centre and the motto HORAS NON NUMERO NISI SERENAS set in bronze. Late afternoon: a bright disc of sunlight from the eye lies on the floor north-east of centre, and the node's point of shadow falls on the numeral IV. Through the far portal, a glimpse of a cooler glass corridor. Classical, calm, luminous. Photorealistic architectural photograph, eye level 1.6 m, 20 mm wide-angle lens, 3:2 landscape, straight verticals unless the view says the camera tilts up. Classical architecture with restrained modern detailing and only a light touch of the future. Physically accurate natural light, real material textures. Paintings are faithful reproductions of the named public-domain works at true scale in gilt frames. No captions, signage, logos or UI overlays (inscriptions cut into the architecture are fine); at most one or two small visitors in neutral clothing for scale.


View-specific instructions and conflict resolution: Camera uses the guide's explicit 18 mm lens, eye height 1.6 m at the west portal looking east and tilted up 24°, with the dome dominating the upper frame. Preserve the illustrated horizon, scale, room shape, portal and niche positions, and sundial placement. Use a credible hemispherical, deeply recessed coffered dome, not a flat ceiling. The eye is fully glazed with a regular triangular steel-and-glass lattice and visible bronze central node, NEVER an open Pantheon hole. Include the bright sunlight disc north-east of centre and an obvious small sharp point shadow specifically falling on the bronze Roman numeral IV. Gilt meridian points north; with the view facing east this means to the camera's left. The floor is a sundial, not a map, compass rose, or decorative star. Include the inscription exactly "HORAS NON NUMERO NISI SERENAS" as bronze inlay following the ring, not floating text. At most one small realistically dressed visitor. Empty niches, no sculpture. Through the far portal a restrained glimpse of a cooler glass corridor. Avoid sci-fi glazing, glowing lines, light beams with heavy haze, and artificial lights. Need richly photoreal pale marble with fine restrained veining, tactile bronze inlay, and luminous reflected sunlight.

## Context and references

All 10 supplied PDFs were read before generation. The composition guide and five contact sheets were inspected; the composition, complete combined design-plan sheet, both Rotunda design-sheet contacts, and all-image-guides contact were supplied as image context (the tool accepts five images maximum). Individual image guides take precedence over the earlier overall design plan.

- [British Museum: Great Court](https://www.britishmuseum.org/about-us/british-museum-story/architecture/great-court), primary reference for plausible restrained steel-and-glass detailing.
- Built-in image_gen tool: one initial generation and one targeted correction.

## QA

### Targeted revision prompt

Use case: precise-object-edit / text-localization. Edit the supplied photorealistic Rotunda image to correct the bronze sundial and lighting only. Preserve the camera, 3:2 framing, all architecture, five coffer rings, steel/glass oculus, portals, empty niches, glass corridor and single visitor exactly. Keep the beautiful natural pale marble textures and realistic photographic style.

Make these precise corrections:
1. The bronze motto across the foreground curve MUST read exactly: "HORAS NON NUMERO NISI SERENAS". In particular the final word is S E R E N A S, not SERRNAS or any other spelling. Correct that word only if other words are correct. Use the same elegant Roman uppercase letters inlaid flush with the marble.
2. The sundial is a half-day dial, NOT a 12-hour clock. Its hour numerals around the upper half must be VI, VII, VIII, IX, X, XI, XII, I, II, III, IV, V, VI in that order, with no duplicates except VI at the two ends. Hour lines are thin bronze. The single gilt meridian goes left from the centre sun to the north; remove the thick gilt line from the centre sun straight towards the bottom edge of the image. No compass arms.
3. Move the bright circular pool of oculus sunlight to north-east of centre, which is just left of the far-east portal axis as viewed from the west. Keep its triangular lattice shadow physically credible. The bronze centre node casts one small clear dark point onto the hour mark IV within this pool. It is a shadow, not a solid object or label. Natural sun is the only lighting in the Rotunda.
4. Remove the artificial uplighting cones at the bases of all pilasters and the artificial ceiling spotlights visible in the portals. Replace their glow with broad soft natural bounced daylight. No changes to architectural forms or image composition.

No new objects, no statues, no text overlays, no watermark. Preserve everything else.

Selected final: `05-the-rotunda-v2.png`. The original `05-the-rotunda.png` is retained as a non-destructive first version.

Inspected both outputs. The selected 1536 × 1024 full-bleed image preserves the marble drum, five visible coffer rings, glazed triangulated oculus with a bronze node, empty niches, arched portals, cool glass corridor, bronze sundial and one small visitor. The revision corrected the Latin motto to HORAS NON NUMERO NISI SERENAS, removed the thick foreground gilt line, and reduced visible artificial uplighting. Remaining limitations: Roman numerals and exact sundial geometry are generative approximations; the IV shadow point and the requested north-east sunlight position are not reliable. The oculus has an irregular triangular pattern rather than a guaranteed equal-triangle grid. No numeric guide labels, captions, UI or watermark are present.
