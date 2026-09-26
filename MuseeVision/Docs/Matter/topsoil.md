# Topsoil

**Key:** `topsoil` · **Family:** living · **Records:**
- the substance: `Data/Matter/Substances/topsoil.json`;
- its history: `ground.trodden`, in `Data/Matter/Histories/`.

Topsoil is the dark, humus-rich top horizon of a garden soil. It lies under:
- the grounds' mown lawn;
- the Hall of Light's meadow and orchard;
- the feet of the clipped hedges;
- Chenghuai's garden beds, under moss and litter.

It shows where the sward is thin, at the cut edge of a bed, between plants, and wherever feet have worn it bare. Nature grows the plants in it and draws the ground. Matter records the soil's own physics: its darkness, how it darkens further when wet, its density and its acoustic softness. Numbers in brackets refer to the sources at the end. Every number is also in the JSON records, next to its unit and its source.

## How it forms

Rock weathers into grains of sand, silt and clay, and plants add organic matter year on year. Under grass, dense fine roots die back into the soil every year and build a thick, dark surface horizon. Soil scientists call it the **mollic epipedon** [1]. Among other criteria, it has:
- a moist Munsell **value and chroma of 3 or less**;
- **organic carbon of at least 0.6 %** (organic matter at least 1.0 %);
- base saturation of at least 50 %;
- a structure that is "not massive", or a consistence that is not hard when dry;
- a thickness of at least 10 cm over rock, and usually 25 cm or more.

A lawn's topsoil is that kind of horizon, or an imported soil made to resemble it. Its sand and silt are mostly quartz: 2.65 g/cm³ and Mohs 7 [2].

## Density and compaction

Soil scientists judge a soil's health by its bulk density, the dry mass over its volume [3]:

| Texture | Ideal for plant growth | Restricts roots |
|---|---|---|
| Sandy | < 1.60 g/cm³ | > 1.80 g/cm³ |
| Silty | < 1.40 g/cm³ | > 1.65 g/cm³ |
| Clayey | < 1.10 g/cm³ | > 1.47 g/cm³ |

At those ideal limits, the pore space is at least about 0.40 of the volume (sandy) to 0.58 (clayey). This is derived against quartz's grain density. The pores hold the water and air that the surface law's wetness fills and empties.

Trampling pushes the bulk density toward the root-restricting values (`ground.trodden`). The turf thins along desire lines, and the ground there goes bare and hard.

## What light does in it

Soil is a granular, porous medium. Humus absorbs strongly, and the mineral grains scatter. It is diffuse, with no gloss of its own.

A mollic topsoil is dark by definition [1]. When wet it darkens further. Lobell and Asner measured soils at a range of moisture contents [4]:
- Reflectance falls **non-linearly with moisture**, and an **exponential** model describes it well.
- The curve is similar across soils once moisture is expressed as the **degree of saturation**.
- The shortwave infrared is the better band for measuring moisture above about 20 % by volume.

The physics is the one behind every wet surface: water fills the pores and films the grains, lowering the index step and trapping light by total internal reflection [5, 6].

## Sound

A lawn is acoustically soft ground. Nord2000 places ground in classes by effective flow resistivity [7]:
- class C, "uncompacted, loose ground (turf, grass, loose soil)": 80 kPa s/m²;
- class D, "normal uncompacted ground (forest floors, pasture field)": 200 kPa s/m²;
- class E, "compacted field and gravel (compacted lawns, park area)": 500 kPa s/m².

The record derives normal-incidence absorption from these with the Delany-Bazley model [8]:

| Class | 125 Hz | 250 | 500 | 1 k | 2 k | 4 kHz |
|---|---|---|---|---|---|---|
| D (garden bed) | 0.11 | 0.18 | 0.28 | 0.42 | 0.58 | 0.75 |
| C (loose turf) | 0.21 | 0.32 | 0.47 | 0.64 | 0.79 | 0.90 |

This is an extrapolation below the model's fitted range. Sound should compute the ground effect from the flow resistivity.

## What to look for in a photograph

- Very dark brown to near black when damp; dusty grey-brown when the top has dried.
- Crumbs and clods from millimetres to centimetres, with sand grains, fine roots, grit and bits of leaf.
- A pale dry crust after sun, which cracks on a clayey soil.
- Under a lawn it shows only in thin patches and at cut edges. At a bed's edge it shows as a clean cut line.

## Evidence

CH_MOSSGROUND (ambientCG Ground037) is the only local photograph that shows soil. It shows yellow-green moss over earth, with patches of bare brown soil, twigs and leaf fragments.

Its mean colour (uncalibrated CIELAB 62.2 / −6.8 / 30.1) is the moss's, not the soil's. Only the bare patches are evidence, and only of the crumb scale and the litter. A tier-(b) specimen would close the gap: a calibrated photograph of bare garden topsoil, dry and wet.

## Not verified

These are listed in the record's `unverified` array:
- the soil's measured reflectance, dry and wet. Oke's textbook albedo table was not read at a source;
- the conversion of Munsell value 3 to a luminous reflectance of about 6–7 %;
- Lobell and Asner's fitted constants, since only the abstract was read;
- the textbook composition of an ideal loam: 45 % mineral, 5 % organic, 25 % water, 25 % air;
- the old build's ground tones. They were set by eye, and include thatch and moss.

## Sources

1. University of Georgia, *A Key to Common Diagnostic Horizons in Soil Taxonomy* (after the USDA Keys to Soil Taxonomy), item 2, "mollic": https://soils.uga.edu/files/2016/08/A-Key-to-Soil-Orders-and-Common-Diagnostic-Horizons-in-Soil-Taxonomy.pdf
2. Handbook of Mineralogy, *Quartz*: http://www.handbookofmineralogy.org/pdfs/quartz.pdf
3. University of Illinois, *Soil Quality: Bulk Density*, Table 1, after M. A. Arshad, B. Lowery, B. Grossman (1996), in *Methods for Assessing Soil Quality*, SSSA, pp. 123–141: https://soilquality.nres.illinois.edu/bulk-density/
4. D. B. Lobell, G. P. Asner, "Moisture effects on soil reflectance", *Soil Sci. Soc. Am. J.* 66(3) (2002) 722–727, https://doi.org/10.2136/sssaj2002.7220 (abstract via https://asu.elsevierpure.com/en/publications/moisture-effects-on-soil-reflectance/)
5. J. Lekner, M. C. Dorf, *Applied Optics* 27(7) (1988) 1278–1280, https://doi.org/10.1364/AO.27.001278
6. S. A. Twomey, C. F. Bohren, J. L. Mergenthaler, *Applied Optics* 25(3) (1986) 431–437, https://doi.org/10.1364/AO.25.000431
7. M. Ahearn et al., *The Analysis of Modeling Aircraft Noise with the Nord2000 Noise Model*, DOT/FAA/AEE/2012-5 (2012), Table 4: https://rosap.ntl.bts.gov/view/dot/9687/dot_9687_DS1.pdf
8. M. E. Delany, E. N. Bazley, "Acoustical properties of fibrous absorbent materials", *Applied Acoustics* 3 (1970) 105–116
