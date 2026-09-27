# Pearl steel: the Élan's pearl-white lacquer

**Records:**
- the working `painted.pearl`, in `Data/Matter/Workings/`, over the steel group's `steel`;
- its history `steel.pearl-weathering`, for the shell outdoors. Indoors the pearl ages as `steel.coating-ageing` says.

This is a working dossier, like `gilt.md`. The steel record and its dossier belong to the steel group; this is the
pearl.

The owner's white family runs plaster, then limewash, then pearl steel. Pearl steel is the Future's white [8]:
"Pearl-white lacquered steel, satin". The museum uses it in four places:
- **the Atrium's twelve ribs,** 0.42 × 0.30 m and chamfered, with the collar and the irises;
- **the Sphere's underside grid,** its pearl throat, and its floor's underside;
- **the Hall of Light's posts, frame and vault;**
- **the Élan's exterior:** the drum's fins and mullions, and the dome's seamless pearl shell, from 22 to 36.3 m.

Numbers in brackets refer to the sources at the end.

## What makes a white "pearl"

The colour is RAL 1013. Its English name is Oyster white; its German name, the official one, is *Perlweiss*, "pearl
white". Its light reflectance value is 70.76 [1].

The pearl itself is a lustre pigment [2], [3]:
- **The platelets.** Mica (n ≈ 1.5), 10-60 µm across and about 0.5 µm thick, is coated with a thin film of titanium
  dioxide (n ≈ 2.7). The silver-white grade is muscovite with anatase.
- **Interference.** The film reflects by interference.
- **Alignment.** The platelets lie parallel to the surface, like a mirror broken into flakes. So the lustre brightens
  toward the reflection of the sky and dims at the flanks.
- **Size.** Platelets of 10-60 µm give the best lustre. Larger ones sparkle; smaller ones give a satin sheen and more
  hiding.
- **The coat.** A pearl coat carries 10-20 % pigment by solids and covers at 15-25 µm dry.

**Why the silhouette cools.** A thin-film colour shifts toward shorter wavelengths as the view turns to grazing, because
2nd·cos θ = mλ. So a silver-white pearl can cool faintly toward blue or lilac at its silhouette while its face stays
warm. The as-built shell drew exactly this: "cooler and faintly lilac where it turns from the eye" [9].

## How it is built

**Inside** (the ribs, lattice, collar, irises and throat):
1. The steel is prepared as the Rotunda's lattice (`painted.steel`): blast-cleaned to Sa 2½, then a 125 µm epoxy build
   [7].
2. A pearl basecoat goes on over a RAL 1013 ground.
3. A satin clear finishes it. The as-built ribs are roughness 0.32 [10].

**Outside** (the dome's seamless shell). D-014 gives it the joint kind "welded and faired", so no joint shows. The
real craft is the superyacht hull's [4], [5]:
1. The steel plates are welded and the welds ground.
2. Epoxy fairing compound is laid on and sanded.
3. A high-build epoxy surfacer goes on: two coats give 150-300 µm dry, three give 225-450 µm, at most 500 µm in one
   application. It is sanded straight.
4. A white basecoat, the pearl, and a gloss clearcoat.

A yacht coating has five layers. The painters fair hull lines of 50-60 m "with a tolerance of less than 500 microns",
and the clearcoat gives "particularly high durability, UV resistance, and visual depth" [5].

**The architectural alternative** is a 70 % PVDF system with mica (PPG Duranar) [6]:
- a 6.4 µm primer;
- a colour coat of at least 25.4 µm;
- and, for metallic and exotic colours, a barrier coat and a 15 µm clearcoat.

## Three whites on record, and one decision

| Source | White |
|---|---|
| The material-language board [8] | #E3DCCB, "(RAL 1013)", satin 0.32 |
| The as-built ribs [10] | #ECE9E3: brighter and cooler |
| The as-built shell [9] | linear (0.80, 0.79, 0.76), under a clear of roughness 0.07 |

One substance in one finish gives one pearl. RAL 1013's LRV of about 0.71 sits closest to the board's colour.
**Which white it is, is the Architect's decision.**

**The family.** A family belongs to a substance, and the steel record declares steel an accent of the glass family.
Pearl steel is therefore steel in this working, although the principles give its look to the white family. The
Architect should decide whether to amend steel's family or to admit the reading.

**The joints.** The as-built shell's hairline joints are superseded by D-014's welded-and-faired shell. They were 6 mm
wide, on meridians every 7.5° and parallels every 6°.

## How it ages

Outdoors, the clearcoat takes the sun and the rain (`steel.pearl-weathering`). A fluoropolymer mica system after ten
years in South Florida at 45° shows at most 5 ΔE of colour change, chalking no worse than rating 8, and at most 5 %
film loss [6].

Long before that, what the eye sees is dirt. Rain carries grime down the shell in faint runs, heavier low on the dome
where more water has passed. The as-built shell drew runs 30 cm apart at the foot. Indoors the pearl ages as the painted
lattice does.

## What to look for in a photograph

- A warm white with a soft, deep lustre, brightening toward the reflection of the sky, with fine glitter close up.
- The shell faired and clear-coated: long, unbroken reflections of the sky and the trees, and no panel lines.
- The ribs satin: a broad, soft highlight along each rib.

## Sources

1. RALcolorchart.com, "RAL 1013 Oyster white".
2. A. Rohrer and M. T. Venturini, "Pearlescent Pigments in Coatings – a Primer", *PCI Magazine* (2018).
3. Susonity (formerly Merck), "IRIODIN 100 Silver Pearl".
4. Mankiewicz, "ALEXSEAL Super Build 302" data sheet (2024).
5. ALEXSEAL, "Fairing & Finishes", vol. XVI (2024).
6. PPG, "Duranar PVDF liquid extrusion coatings" (2024).
7. `MuseeVision/Data/Matter/Workings/painted.steel.json`.
8. The owner's principles, and `plan/canvas/MaterialsOther.dc.html` and `MaterialsStone.dc.html`.
9. `windows/Scripts/exterior_materials.py` (frozen reference).
10. `windows/Scripts/native.py`, `make_rib_pearl` (frozen reference).
