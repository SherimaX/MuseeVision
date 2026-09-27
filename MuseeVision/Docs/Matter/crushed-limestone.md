# Crushed limestone, 0/6 mm (the Paris gardens' sable)

**Key:** `crushed-limestone` · **Family:** accent, tied to **stone** · **Records:**
- the substance: `Data/Matter/Substances/crushed-limestone.json`;
- its workings: `rolled.self-binding` and `rolled.stabilised`, in `Data/Matter/Workings/`. The paving workings `laid.sand-jointed` and `laid.mortar-jointed` are there too; the first uses this sand in its joints;
- its histories: `ground.trodden` and `paving.joint-infill`, in `Data/Matter/Histories/`.

This is the pale, dusty ground of the French formal garden: the *stabilisé* of the Tuileries, which the old build called "the Paris gardens' pale sable (a beige grit with small stones)". In the museum it is:
- **the grounds' walks,** rolled: the podium's strip, the allées beside the parvis, the cour round the canal, the avenue, the walk east, the rond-point, and the Élan's terrace;
- **the sand in the joints and bed of the grounds' stone paving** (`laid.sand-jointed`).

It ties to the stone family because it is the same calcite as the museum's travertine and marble, broken small. Numbers in brackets refer to the sources at the end. Every number is also in the JSON records, next to its unit and its source.

## How it is made

Quarried limestone is crushed and screened. The **0/6** fraction keeps everything that passes a 6.3 mm sieve, from grit down to dust; the dust is called the fines [1]. Crushing leaves the grains **angular**, unlike the "grains arrondis polis" of river sand [2]. That is why the material locks together when it is wetted and rolled. It is sold as "Blanc–Jaune", "Brun" and "Ocre" [3]. Loose in bulk it weighs about 1.5 t/m³ and bulks by 5–8 % [2].

The grains are calcite: 2.71 g/cm³ and Mohs 3 [4]. A single grain face reflects about 5 % (F0 0.053, from calcite's two indices; see the travertine record) [5]. A bed of grains is bright because light scatters many times through them, not because of that face.

## How it is worked

**Rolled, self-binding (`rolled.self-binding`).** A French trade guide describes the walk [6]:
1. The ground is dug out about 15 cm.
2. A gravel base a few centimetres thick goes in.
3. The 0/6 is spread about **5 cm** thick. A supplier gives 3–5 cm for the finish layer [3].
4. It is raked, soaked, and compacted with a vibrating plate or a heavy roller. "Le compactage ne se fait jamais à sec": the water carries the fines into the voids, and the angular grains lock together as they dry.

Maintenance is summer watering and refilling hollows.

**Rolled, stabilised (`rolled.stabilised`).** The same sand is premixed with **3.5, 5 or 7 %** hydraulic binder [1], so it sets into a weak, still-porous crust. It keeps "la couleur naturelle du sable et son aspect rustique" [7]. This is the stabilisé of the Paris gardens. franceinfo calls it "le revêtement inventé par Lenôtre pour une fréquentation de promeneurs" [8]. Under trucks and fourteen million visitors a year, the Tuileries' 23 ha of it break into a fine grey powder that "envahit tout".

**Where it meets the paving (`laid.sand-jointed`, `laid.mortar-jointed`).** The Dimension Stone Design Manual [9] gives two ways to lay exterior stone.

- **Laid flexibly on sand.** The stone sits on a bedding course, about 38 mm deep, of sharp limestone screenings or concrete sand. Sand is broomed and tamped into the joints so each stone is bedded round its edges.
- **Laid rigidly on mortar.** The stone sits on a full bed of 1 : 4–5 cement : sand mortar, at least 32 mm and preferably 51 mm thick. The joints are pointed with mortar or grout, or sealed.

The joint widths are the same for both:
- at least 6.4 mm, preferably 9.5 mm;
- 12.7 mm or more for large stones;
- 19–25 mm for cleft or textured faces.

The manual adds: "in no case should natural stone be installed with tight joints."

## How it ages

**Trodden (`ground.trodden`).** Feet grind the surface grains, pack the fines along the lines people walk and push loose grit to the sides. The lanes read smoother and paler, and the edges looser. In dry weather the ground-down fines lift as dust; after rain, puddles stand in the trodden hollows.

On soil (see `topsoil`), the same history compacts the ground. The history record also carries the ground's acoustic shift, from Nord2000 class C or D toward E and F [10].

**Joint infill (`paving.joint-infill`).** Sand joints in outdoor paving collect wind-blown dust, the paving's own fines and leaf fragments, and they stay damp. Mosses colonise them; in cities that is typically silver moss, *Bryum argenteum*, "common to urban areas where it can be found between cracks on sidewalks" [11]. The moss is Nature's plant; the damp, dark, soil-filled joint is Matter's history.

The local photograph CH_PAVING (ambientCG PavingStones112) shows this: joints about 6–11 mm dark, at a 0.43 m pitch, with moss in some and not others.

**Wet.** Wet gravel darkens. Water in the pores and on the grains lowers the index step and traps diffusely reflected light [12, 13].

## Sound

A rolled walk is Nord2000 class F, "compacted dense ground (gravel road, parking lot)", 2000 kPa s/m². Loose-surfaced gravel is class E, 500 kPa s/m² [10]. The record derives normal-incidence absorption from these with the Delany-Bazley model [14]:

| Class | 125 Hz | 250 | 500 | 1 k | 2 k | 4 kHz |
|---|---|---|---|---|---|---|
| F (rolled walk) | 0.02 | 0.04 | 0.06 | 0.09 | 0.15 | 0.24 |
| E (loose gravel) | 0.06 | 0.09 | 0.15 | 0.24 | 0.37 | 0.53 |

This is an extrapolation below the model's fitted range. Sound should compute the ground effect from the flow resistivity. The crunch underfoot is the footfall class `gravel`.

## What to look for in a photograph

- Pale beige-grey grit with small angular stones.
- The trodden lanes are paler and smoother; the edges are looser, with low ridges of swept grit.
- Dust on shoes, on the feet of hedges and on bench legs in a dry summer.
- After rain the walk is uniformly darker and dries from its crown. Puddles leave a pale skin of fines.
- Grain faces glint in low sun.

## Evidence

There is no photograph of a crushed-limestone walk on this PC. G3 (ambientCG Gravel022) is a rounded river gravel of mixed grey, red and green stones, mean CIELAB 53.4 / −0.9 / 6.9 uncalibrated. That is the wrong material, and it is kept in `evidenceRejected`.

A tier-(b) specimen closes the gap: a calibrated photograph of a Tuileries or Luxembourg walk, dry and wet, lane and edge, with a ColorChecker.

## Not verified

These are listed in the records' `unverified` arrays:
- the walk's reflectance. A search summary cites solar reflectance of about 0.42–0.45 for unbound limestone aggregate, not read at the source;
- the old build's colours, #BDAF94 for the walk and #D6CBB2 for the lanes. They were set by eye;
- the Tuileries mix, lime with gravel and sand, and its watering and harrowing. These come from an unreadable article;
- friction, crossfall, surface firmness, and the basis of the binder percentage;
- a unitless "masse volumique 1.7";
- the drying time of a filled joint.

## Sources

1. Bronzo Perasso, *Sable naturel pour sols stabilisés 0/6,3* and *Sable traité pour sols stabilisés (3,5 %, 5 %, 7 %)*: https://bronzoperasso.fr/produit/sable-naturel-pour-sols-stabilises-0-63/
2. Koncrete, *Sable 0/6 livré en vrac*: https://www.koncrete.fr/produits/sable-0-6
3. Via Matériaux, *Sable 0/6 concassé calcaire – Vrac*: https://viamateriaux.fr/produit/sable-0-6-concasse-calcaire/
4. Handbook of Mineralogy, *Calcite*: http://www.handbookofmineralogy.org/pdfs/calcite.pdf
5. G. Ghosh, *Optics Communications* 163 (1999) 95–102, via https://refractiveindex.info/?shelf=main&book=CaCO3&page=Ghosh-o (the derivation is in the travertine record)
6. Carrière Vila, *Allée en sable stabilisé : guide pas à pas* (2026): https://www.carriere-vila.com/guide-realiser-allee-en-sable-stabilise/
7. Colas & Vous, *Sable stabilisé*: https://www.colasetvous.fr/produits/sable-stabilise/
8. franceinfo, "Les jardins des Tuileries recherchent 15 millions pour éradiquer la poussière" (3 May 2013): https://www.franceinfo.fr/culture/patrimoine/les-jardins-des-tuileries-recherchent-15-millions-pour-eradiquer-la-poussiere_3310669.html
9. Marble Institute of America, *Dimension Stone Design Manual* VIII (2016), "Horizontal Surfaces": https://nsrstone.com/wp-content/uploads/2025/07/Horizontal-Surfaces.pdf
10. M. Ahearn et al., *The Analysis of Modeling Aircraft Noise with the Nord2000 Noise Model*, DOT/FAA/AEE/2012-5 (2012), Table 4: https://rosap.ntl.bts.gov/view/dot/9687/dot_9687_DS1.pdf
11. R. Sudan, *Silvergreen Bryum Moss*, USDA Forest Service: https://www.fs.usda.gov/wildflowers/plant-of-the-week/bryum_argenteum.shtml
12. J. Lekner, M. C. Dorf, *Applied Optics* 27(7) (1988) 1278–1280, https://doi.org/10.1364/AO.27.001278
13. S. A. Twomey, C. F. Bohren, J. L. Mergenthaler, *Applied Optics* 25(3) (1986) 431–437, https://doi.org/10.1364/AO.25.000431
14. M. E. Delany, E. N. Bazley, "Acoustical properties of fibrous absorbent materials", *Applied Acoustics* 3 (1970) 105–116
