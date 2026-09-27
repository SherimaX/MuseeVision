# Lime plaster (lime putty and marble dust)

**Key:** `lime-plaster` · **Family:** white · **Records:**
- the substance: `Data/Matter/Substances/lime-plaster.json`;
- its workings: `trowelled`, `run` and `limewashed`, in `Data/Matter/Workings/`;
- its histories: `white.soiling`, `white.micro-cracking` and `white.patching`, in `Data/Matter/Histories/`.

Lime plaster is the Rotunda's coffered dome: five rings of 28 coffers between the cornice and the glazed eye. It is laid
in coats over the masonry and closed with a thin setting coat of lime and marble dust, polished with a steel trowel.
It is a white made of stone: once it has hardened, both the binder and the fine aggregate of the finish are calcite.
Numbers in brackets refer to the sources at the end. Every number is also in the JSON record, next to its unit and
its source.

## How it is made

**From limestone to putty.** Limestone is burnt in a kiln to quicklime. The quicklime is slaked with water, sieved to
lime putty and left to mature for at least three months before use [1]. Vitruvius already asks for lime "tempered a
long time before it is wanted", so that any lumps that were not burnt enough slake through [2].

**The coats.** The plasterer builds the surface in layers [1][3]:

| Coat | Stuff | Thickness |
|---|---|---|
| First (scratch) coat | haired coarse stuff: 1 part lime putty to 2 of coarse sand (passing 3-5 mm) by volume | 9-12 mm, never over 15 mm |
| Straightening (floating) coat | haired coarse stuff, ruled straight and scoured with a devil float | 9-12 mm, never over 15 mm |
| Setting (finishing) coat | setting stuff: lime putty and fine sand (passing 1 mm) or marble dust, 1:1 for standard work | 2-3 mm, in two or three thin coats |

The scratch coat dries for two to four weeks, the floating coat for about a week [3]. Vitruvius, writing of
stucco, gives the classical version: at least three coats of sand mortar, then three coats of marble dust, graded
from coarse to fine, which make the wall "solid, and not liable to cracks" [2].

**How it hardens.** Lime putty is calcium hydroxide (portlandite). Exposed to air it takes up carbon dioxide and
becomes calcium carbonate (calcite): Ca(OH)2 + CO2 → CaCO3 + H2O. As it carbonates the pore structure changes [4].
The pore size at which the volume grows (0.1 µm has been attributed to [4]) could not be checked against the paper,
so the record keeps it as unverified. The front moves inward from the face, so the skin is hard long before the
core. A 1:3 air-lime mortar (hydrated lime and sand) measured at 28 days is a light, porous, weak material: 1742 kg/m³
bulk density, 32.4 % open porosity, 1.08 MPa compressive strength and a vapour diffusion resistance factor of only
10.7 (wet cup) [5]. That is a
sanded mortar; the dense marble-dust finish is harder at its face than these numbers suggest, but it is the same kind
of body.

**Its optics.** Calcite is birefringent. Its refractive indices are 1.658 (ω) and 1.486 (ε) [6], so the reflectance of
a single facet at normal incidence lies between 0.038 and 0.061 (Fresnel, [7]). A plaster is a random mass of such
grains, so this is its specular F0. The white is not a pigment. Light is scattered at countless grain and pore
boundaries until most of it comes back out. Fine ground white marble, the finish's aggregate at its whitest, measures
a luminous reflectance of 0.95 (Ry, illuminant C) and L* 98.2 [8]. A set plaster must lie below that, because it has
fewer scattering boundaries per millimetre than a loose powder and carries impurities from the binder and the sand.
No measured value for set lime plaster was found, so the record gives 0.80-0.92 as unverified until the
ColorChecker specimen (tier b) settles it.

**Wet.** Water fills the open pores and matches calcite's index far better than air does. Scattering drops and the
surface darkens until it dries.

## How it is worked

- **Trowelled** (`trowelled`). The setting coat is floated with a timber float as it begins to set, then polished with
  a steel trowel as it hardens. It is re-wetted, scoured and polished over the following hours and days to close
  shrinkage cracks [1]. The trowel presses the fine particles to the face and gives a closed skin with a soft sheen.
  Floats alone give an open, sandy texture [3]. Vitruvius compares well-made thick stucco to a thick silver mirror
  that takes a high polish [2].
- **Run** (`run`). The coffer steps, ribs and bands are profiles. In situ they are run with a zinc template on a
  wooden horse drawn along rules, in lime gauged with plaster of Paris. The gypsum's quick set and slight expansion
  suit running [9][10].
- **Limewashed** (`limewashed`). Optionally the finish is limewashed; see `limewash`.

**Stock.** Plaster has no blocks. Its stock is the coat and the working session. Each field of setting coat is laid
and trowelled in one session, because the thin coats follow each other in quick succession and are polished as they
set [1]. In the dome, the coffers, ribs and bands bound the fields. The area of a day's work is unverified.

## How it ages

- **Soiling** (`white.soiling`). Dark particles settle on the surface. A white surface darkens by the fraction of
  its area that black particles cover. Observers first see it at a few percent coverage (2.4-3.6 % side by side,
  12 % across a gap) [11]. Coarse dust gathers on upward faces. In the dome the coffers face down and inward, so the
  fields stay clean and the upward-facing steps and the backs of coffers collect dirt.
- **Micro-cracking** (`white.micro-cracking`). Lime shrinks slightly as it dries [9]. A finish that dried too fast
  shows crazing, a fine web of hairline cracks [1]. Single hairlines follow the structure and joints below and open
  and close with the seasons [1]. Hairlines are under 0.1 mm and fine cracks up to 1 mm (BRE Digest 251, [12]).
- **Patching** (`white.patching`). Repairs are cut back to a sound face with a slight undercut and built in thin
  coats of a matched mix [1]. Gypsum in a repair changes its suction, so limewash over it looks different and the
  patch shows [1].

## What to look for in a photograph

- A dense, closed skin with a soft sheen where the trowel polished it. Raking light shows long, shallow trowel arcs
  and broad, very low clouding where suction varied.
- The arrises of coffers and ribs are crisp but never machine-sharp. Run members carry fine striations along their
  length.
- A craze web on patches that dried fast, and single hairlines over structure.
- Grey on the upward steps and in the backs of coffers. Patches read by a slightly different texture and a cleaner
  white.

## Evidence

Local, CC0, read-only. These are reference photographs of unknown exposure: they show texture, relief and ageing, not
albedo.
- `windows/SourceArt/PBR/P1`: ambientCG **Plaster001**, "trowelled lime plaster" (CREDITS.md). It shows trowel strokes
  in the normal map and broad clouding in the colour map. The mean of its colour map is (0.68, 0.65, 0.63), linear,
  and the median of its authored roughness map is 0.79. Neither is a measurement.
- `windows/SourceArt/PBR/CH_LIME`: Poly Haven **white_plaster_02**, "white lime render" (CREDITS_chenghuai.md). It
  shows an aged, soiled lime surface, with grey-brown blotches darker in the hollows. It is evidence for the soiling
  history.

No photograph of a coffered lime dome or of marble-dust stucco at known exposure is on this PC. Albedo needs a tier
(b) ColorChecker specimen.

## Sources

1. Napier, W. & Jenkins, M. (2018) *Inform Guide: Plain Lime Plastering*. Historic Environment Scotland. https://www.historicenvironment.scot/archives-and-research/publications/publication/?publicationId=67d4d72c-7b8c-4622-a244-a94b01090177
2. Vitruvius, *De Architectura* VII.2.1, VII.3.6, VII.3.9, trans. J. Gwilt (1826). LacusCurtius, https://penelope.uchicago.edu/Thayer/E/Roman/Texts/Vitruvius/7*.html
3. Ratcliffe, T. (2006) Internal Lime-Plastering. *The Building Conservation Directory*. https://www.buildingconservation.com/articles/internallimeplast/lime_plaster.htm
4. Lawrence, R.M., Mays, T.J., Rigby, S.P., Walker, P. & D'Ayala, D. (2007) Effects of carbonation on the pore structure of non-hydraulic lime mortars. *Cement and Concrete Research* 37(7), 1059-1069. https://doi.org/10.1016/j.cemconres.2007.04.011
5. Pavlíková, M. et al. (2019) Influence of wood-based biomass ash admixing on the structural, mechanical, hygric, and thermal properties of air lime mortars. *Materials* 12(14), 2227. https://doi.org/10.3390/ma12142227 (reference mortar MR)
6. Anthony, J.W., Bideaux, R.A., Bladh, K.W. & Nichols, M.C. (eds.) *Handbook of Mineralogy*: Calcite; Portlandite. Mineral Data Publishing. http://www.handbookofmineralogy.org/pdfs/calcite.pdf
7. Born, M. & Wolf, E. (1999) *Principles of Optics*, 7th ed., Cambridge University Press, §1.5.
8. Omya Pars, Product Information OMYACARB 2-MH (ground white marble), edition 27/02/07. https://mychem.ir/uploads/tds/28825.pdf
9. Ratcliffe, T. (1997) The Use of Gypsum Plaster. *The Building Conservation Directory*. https://www.buildingconservation.com/articles/gypsum/gypsum_plaster.htm
10. Vadstrup, S., Running mould decorations. Plaster Architecture project essay. https://www.palazzospinelli.org/plaster/essay/essay/Soren06.html
11. Bellan, L.M., Salmon, L.G. & Cass, G.R. (2000) A Study on the Human Ability To Detect Soot Deposition onto Works of Art. *Environmental Science & Technology* 34(10), 1946-1952. https://doi.org/10.1021/es990769f
12. BRE Digest 251 (revised 1995) *Assessment of damage in low-rise buildings*; categories as given at https://www.designingbuildings.co.uk/wiki/Cracking%20and%20building%20movement
