# Fresh water

**Key:** `water` · **Family:** water · **Records:**
- the substance: `Data/Matter/Substances/water.json`;
- its working: `water.dyed-black`, in `Data/Matter/Workings/`;
- its histories: `water.phytoplankton` and `water.dissolved-organic`, in `Data/Matter/Histories/`.

Water is the museum's one transparent liquid. It fills three kinds of basin:
- **the Salon's lily pond** in the Nymphéas oval. It is indoors and dyed black, so it becomes a mirror for the velarium and the panels;
- **Chenghuai's garden pond.** It is outdoors and alive: a green Suzhou pond under the garden's live wind and rain;
- **the grounds' long canal** before the portico, and the **round basin** on the walk east. Both are outdoors, with clear water over a dark bed, so they hold the portico's reflection.

This record holds what light and sound do in water. Nature owns each body of water as an object: its level, its waves and the canal's flow. Numbers in brackets refer to the sources at the end. Every number is also in the JSON records, next to its unit and its source.

## What the colour is made of

"Pond water" has no colour of its own. A basin's colour is the sum of the absorbers in it, and each one has a measured spectrum [8]:
- **the water molecule itself.** Its absorption is weak and rises steeply into the red;
- **algae,** whose chlorophyll absorbs blue and red and leaves green. This is the history `water.phytoplankton`;
- **dissolved organic matter** (humic substances and tannins), which absorbs blue most and fades toward the red. This is the history `water.dissolved-organic`;
- **a dye,** where the keeper adds one on purpose. This is the working `water.dyed-black`.

The absorption coefficients add. Light's path down to the bed and back then decides how much of the bed is seen.

## What light does in it

**At the surface.** The refractive index of distilled water at 20 °C is 1.3334 at 589.3 nm. It rises to 1.3436 at 400 nm and falls to 1.3305 at 700 nm (Daimon and Masumura's Sellmeier fit, valid from 0.18 to 1.13 µm) [1]. From that index:
- **face-on,** the surface reflects about 2 % (F0 = 0.0206 at 550 nm), half of what glass reflects;
- **Brewster's angle** is 53.2°. Skylight reflected near that angle is strongly polarised.

Unpolarised reflectance rises steeply at grazing angles:

| Incidence | 0° | 40° | 60° | 70° | 80° | 85° | 89° |
|---|---|---|---|---|---|---|---|
| Reflectance | 0.021 | 0.025 | 0.060 | 0.134 | 0.349 | 0.584 | 0.896 |

This curve explains why a pond is a window near your feet and a mirror of the far bank. It is geometry, not paint.

**In the volume.** Pope and Fry measured the absorption of pure water in an integrating cavity [2]:

| Wavelength | 420 nm | 450 nm | 500 nm | 550 nm | 600 nm | 650 nm | 700 nm |
|---|---|---|---|---|---|---|---|
| a_w (1/m) | 0.0045 | 0.0092 | 0.020 | 0.057 | 0.22 | 0.34 | 0.62 |

Absorption is least near 418 nm and about 140 times larger at 700 nm. One metre of pure water passes 0.99 of the blue, 0.95 of the green and 0.71 of the red. The museum's basins are all under a metre deep, so the water molecule alone barely tints them. Their colour comes from what is in them. Smith and Baker's "clearest natural waters" [3] agree with Pope and Fry above 550 nm and are higher in the blue, where natural water carries traces of dissolved matter.

**Wet surfaces.** A water film darkens every porous or rough material it wets. It lowers the index step at the grains and traps diffusely reflected light by total internal reflection [11]. That is the surface law's wetness, and it is Matter's job everywhere.

## The surface: still, wrinkled, rippled

**No wind.** The Salon pond is indoors, so its surface is a mirror.

**Light wind.** In a laboratory channel, Paquier, Moisy and Rabaud measured wind blowing over a liquid [4]:
- Below a threshold wind, the surface carries only disorganised **wrinkles** elongated along the wind.
- Their rms height scales as ζ = α ν^−½ u\*^3/2, where α = 1.9 × 10⁻⁷ SI and the friction velocity u\* is about 0.05 times the air speed.
- At 3.2 m/s the wrinkles are about 12 µm high. They are correlated over about 200 mm along the wind and 75 mm across it.
- Their slopes are about 6 × 10⁻⁵, which leaves a near-perfect mirror, faintly streaked.

**The threshold for waves.** At sea and in laboratories, waves start at 1 to 3 m/s. Kahma and Donelan measured 2.9 to 3.4 m/s in the laboratory [4]. The simple Kelvin-Helmholtz prediction of 6.6 m/s is far too high.

**Waves.** Above the threshold, Cox and Munk's sun-glitter photographs give the mean square slope of a clean sea surface as σ² = 0.003 + 5.12 × 10⁻³ W, with W the wind in m/s at 12.5 m [5]. The upwind and crosswind components are 3.16 × 10⁻³ W and 0.003 + 1.92 × 10⁻³ W.

For an isotropic Gaussian slope distribution, the Beckmann roughness is m = σ, so the GGX α can be set to it:

| Wind (m/s) | 0 | 1 | 2 | 3 | 5 | 8 |
|---|---|---|---|---|---|---|
| m (clean sea) | 0.055 | 0.090 | 0.115 | 0.136 | 0.169 | 0.210 |

This is an open ocean with unlimited fetch, so it is an upper bound for a basin a few tens of metres long.

**Slicks.** An oil slick cuts the mean square slope by a factor of two to three (σ² = 0.008 + 1.56 × 10⁻³ W). Natural slicks, a few molecules thick, do not change the reflectance itself [5]. Pollen and dust lanes on a garden pond are this effect.

**Surface tension.** At 20 °C it is 0.0727 N/m [6]. It sets the scale of capillary ripples and the meniscus against a coping or a lily pad.

## How it is worked: dyed black

The Salon pond is **dyed black** (`water.dyed-black`). The pond-dye trade doses about 33 g of powder per cubic metre, or 300 g of the liquid concentrate. The dye is "fully mixed in when you cannot see below the surface of the water" [9]:
- It blocks light, so submerged algae cannot grow.
- Lilies, whose leaves and flowers float on or stand above the water, are not affected.
- One application lasts up to three months.
- The dye's chemistry is a trade secret in the one safety data sheet read [10].

The surface itself is unchanged, and still reflects about 2 % face-on. Black water looks like a mirror because nothing comes back from below to veil the reflection.

No absorption spectrum of a pond dye was found. The Salon pond is about 0.32 m deep in the old build (the surface at 0.38 m and the bed at 0.06 m). A bed that disappears through that depth needs an absorption above about 7 /m across the visible, if 1 % two-way transmittance is taken as the limit of seeing. That figure is in `unverified`.

## How it ages

**Algae (`water.phytoplankton`).** Meler et al. fitted about a thousand spectra from waters with chlorophyll a from 0.31 to 142 mg/m³, a range that covers garden ponds [7]. Their measured power law is a_ph(λ) = A(λ) Chl^E(λ). A and E are in the record every 10 nm from 380 to 700 nm. The law gives these absorptions:

| Chlorophyll a | a_ph 440 nm | 550 nm | 600 nm | 676 nm |
|---|---|---|---|---|
| 5 mg/m³ | 0.20 /m | 0.046 | 0.029 | 0.092 |
| 25 mg/m³ | 0.75 /m | 0.18 | 0.12 | 0.38 |
| 100 mg/m³ | 2.3 /m | 0.55 | 0.38 | 1.31 |

Carlson's trophic state index ties chlorophyll to clarity [12]: TSI(SD) = 60 − 14.41 ln SD and TSI(CHL) = 9.81 ln CHL + 30.6. Where algae alone set the clarity, the Secchi depth is:

| Chlorophyll a | Secchi depth |
|---|---|
| 2.6 µg/L | 4.0 m |
| 20 µg/L | 1.0 m |
| 56 µg/L | 0.5 m |
| 100 µg/L | 0.34 m |

The old build's Chenghuai pond, with "the bed gone by 30 cm", corresponds to about 100 µg/L, a hypereutrophic pond. The clear canal should sit in the oligotrophic to mesotrophic classes.

**Dissolved organic matter (`water.dissolved-organic`).** Yellow substance absorbs as a_y(λ) = a_y(440) exp(−0.014 (λ − 440)), with λ in nm [8]. It adds a tea tint that turns olive with algae. The basin's a_y(440) is the one free parameter, and no garden-pond value was read.

## Sound

A water surface is acoustically hard:
- It absorbs 0.01 from 125 Hz to 1 kHz and 0.02 at 2 and 4 kHz [13]. A second table gives 0.008 to 0.025 [14].
- It is Nord2000's hardest ground class, H: "very hard and dense surface (dense asphalt, concrete, water)", 200 000 kPa s/m² [15].

## What to look for in a photograph

- Near the edge you look down through the water onto the bed. Across the pond you see the far bank and the sky in reflection.
- Any ripple stretches the reflections of verticals into streaks.
- Wind leaves "cat's-paws": dark, rough patches that reflect less sky.
- Clear water over a dark bed reads black. Green water shows its colour where the sun enters it and over pale steps.
- In dyed water, lily stems vanish a finger's depth down.
- Pollen or dust leaves smooth, slick lanes.

## Evidence

No photograph of real water is on this PC. `T_water_ripples.png` is generated, and G5 (Rock032) is the bed's stone, another substance. Water therefore stands on measured spectra and laws. A tier-(b) check would photograph a still pond and dyed water at known sun and view angles.

## Not verified

These are listed in the records' `unverified` arrays:
- the molecular scattering coefficient of pure water (Morel 1974), which was not read at a source;
- the dye's absorption;
- the chlorophyll and a_y(440) of each basin;
- the three-decimal acoustic row. It prints 0.15 and 0.25 at 1 and 4 kHz, read here as 0.015 and 0.025;
- slope statistics for fetch-limited basins;
- the old build's Single Layer Water coefficients. They were set by eye, and are recorded as "design as built", not as data.

## Sources

1. M. Daimon, A. Masumura, "Measurement of the refractive index of distilled water from the near-infrared region to the ultraviolet region", *Applied Optics* 46(18) (2007) 3811–3820, https://doi.org/10.1364/AO.46.003811; coefficients via https://refractiveindex.info/?shelf=main&book=H2O&page=Daimon-20.0C
2. R. M. Pope, E. S. Fry, "Absorption spectrum (380–700 nm) of pure water. II. Integrating cavity measurements", *Applied Optics* 36(33) (1997) 8710–8723, https://doi.org/10.1364/AO.36.008710; table https://omlc.org/spectra/water/data/pope97.txt
3. R. C. Smith, K. S. Baker, "Optical properties of the clearest natural waters (200–800 nm)", *Applied Optics* 20 (1981) 177–184; table https://omlc.org/spectra/water/data/smith81.txt
4. A. Paquier, F. Moisy, M. Rabaud, "Viscosity effects in wind wave generation", arXiv:1611.01208 (2016), Figs. 4, 6 and Table I (with Kahma and Donelan 1988, *J. Fluid Mech.* 192:339): https://arxiv.org/abs/1611.01208
5. C. Cox, W. Munk, "Measurement of the roughness of the sea surface from photographs of the sun's glitter", *J. Opt. Soc. Am.* 44(11) (1954) 838–850, https://doi.org/10.1364/JOSA.44.000838; also the Ocean Optics Web Book, https://www.oceanopticsbook.info/view/surfaces/cox-munk-sea-surface-slope-statistics
6. NIST Chemistry WebBook, SRD 69, *Thermophysical Properties of Fluid Systems* (water, IAPWS-95): https://webbook.nist.gov/cgi/fluid.cgi?ID=C7732185
7. J. Meler, S. B. Woźniak, J. Stoń-Egiert, B. Woźniak, "Parameterization of phytoplankton spectral absorption coefficients in the Baltic Sea", *Ocean Science* 14 (2018) 1523–1545, Table A1: https://doi.org/10.5194/os-14-1523-2018
8. A. Morel, D. Antoine, *Pigment index retrieval in Case 1 waters*, MERIS ATBD 2.9 (2011), quoting A. Bricaud, A. Morel, L. Prieur, *Limnol. Oceanogr.* 26 (1981) 43–53: https://earth.esa.int/eogateway/documents/20142/37627/MERIS-ATBD-2-9.pdf
9. Dyofix, *Black pond dye: Pond Black*: https://www.dyofix.co.uk/black-pond-dye/
10. Lawn and Garden Products Inc., *Monterey Black Pond Dye* safety data sheet (2015): https://www.montereylawngarden.com/product/monterey-black-pond-dye/
11. J. Lekner, M. C. Dorf, "Why some things are darker when wet", *Applied Optics* 27(7) (1988) 1278–1280, https://doi.org/10.1364/AO.27.001278
12. R. E. Carlson, "A trophic state index for lakes", *Limnol. Oceanogr.* 22 (1977) 361–369, as given by NALMS: https://www.nalms.org/secchidipin/monitoring-methods/trophic-state-equations/
13. akustik.ua, *Absorption coefficients*: https://www.acoustic.ua/st/web_absorption_data_eng.pdf
14. US Enclosure, *Sound Absorption Coefficients*: http://usenclosure.com/Diffraction%20Graphs/Sound%20Absorption%20Coefficients.pdf
15. M. Ahearn et al., *The Analysis of Modeling Aircraft Noise with the Nord2000 Noise Model*, DOT/FAA/AEE/2012-5 (2012), Table 4: https://rosap.ntl.bts.gov/view/dot/9687/dot_9687_DS1.pdf
