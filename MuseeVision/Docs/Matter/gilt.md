# Gilt: gold leaf laid over a substrate (a working, not a substance)

**Key:** `gilt` (a working) · **Laid over:** plaster, bronze or oak · **Records:**
- the working: `Data/Matter/Workings/gilt.json`;
- its history: `gilt.wear`, in `Data/Matter/Histories/`.

D-014 makes gilt a **working**, not a substance. It is gold leaf, with gold's measured complex F0, laid over a substrate: plaster, bronze or wood. The bole shows through with wear. This dossier stands beside the substance dossiers because gilt is how the museum shows gold:
- the gilt sun at the Rotunda's centre, the arrival point (D-012);
- the gilt boss and the stops of the sun clock's motto;
- carved gilt frames and ornament.

Numbers in brackets refer to the sources at the end. Every number is also in the JSON records, next to its unit, grade and source.

## The leaf

The trade leaf is **23.75 carat**: 23.75/24, or 98.96 %, gold. The rest is silver and copper in the maker's proportions [1]. Architectural gilders use at least 22 carat [2].

The gold is rolled and then beaten between skins. The finished leaf is about **0.1 µm** thick, "1/250,000th of an inch" [2][3]. The leaf's weight gives the same figure. Booklets sold at 15, 16 or 18 g, read as the weight of 1000 leaves of 80–84 mm, give 0.11–0.15 µm of gold, at 19.283 g/cm³ [1][4].

The 23–23.75 carat leaves carry no tarnishing risk, and the risk rises as the carat falls [1].

The leaf is **optically thick**. Light that enters gold falls to 1/e within 14–21 nm, so a 100 nm leaf lets through only 0.1–0.9 % of it. Most of that is near 500 nm, which is why thin gold looks green when held against the light [5]. What the eye sees is bulk gold. Nothing of the substrate shows through an intact leaf.

## The optics

Gold's F0 is computed from three measured sets of optical constants, through the Fresnel equations and the CIE observer under D65 [5][6][7][8]:

| Dataset | F0 (linear Rec.709) | Luminance |
|---|---|---|
| Johnson and Christy 1972 | [1.037, 0.729, 0.364] | 0.768 |
| Rioux et al. 2014 | [1.055, 0.779, 0.335] | 0.806 |
| Babar and Weaver 2015 (chosen) | [1.058, 0.774, 0.307] | 0.800 |

By wavelength, gold reflects about 0.36–0.41 at 450 nm, 0.79–0.85 at 550 nm and 0.96–0.97 at 650 nm. Red above 1 is real: gold's colour lies outside the Rec.709 gamut. Whether to render in a wider space or to clip is a decision for the Matter builder and the Eye, not for this record.

The edge tint at 82 degrees is [1.001, 0.896, 0.630]: toward grazing, gold whitens.

The leaf's roughly 1 % of silver and copper moves F0 less than the spread between the datasets. Ten per cent silver moves it by about 0.05 in red and 0.04 in green and blue [7].

## How it is laid

**Water gilding** is used for carved wood, frames and fine interior work, and it is the only method that burnishes to a mirror. The layers are:
1. Six to twelve coats of **gesso**: chalk or gypsum in rabbit-skin glue [2][9].
2. Four to eight coats of **bole**: a refined clay in glue [2]. Red bole takes its colour from iron oxide [10]. Bole comes in several colours [2][11]:
   - red, "dull rich red", which warms the gold;
   - yellow, for paler gold;
   - blue-grey, under burnished work;
   - black or white, under silver.
3. **Gilder's liquor**: weak glue in water that re-wets the bole. Its strength is not in the sources read; the one-to-ten rabbit-skin glue [11] is the glue of the gesso and the bole.
4. The **leaf**, floated on.

After 5–8 hours in summer or 20–24 hours in winter, the gold is burnished with an agate [11]. The springy bole beneath lets the stone press the leaf into a true mirror [9][12]. Burnished and matte passages set side by side are the gilder's signature. The museum's water gilding uses red bole.

**Oil gilding** is used for plaster, stone, metal and architectural work. The ground is sealed. A drying-oil size (boiled linseed oil with litharge) is brushed on, and the leaf is laid when the size is touch-dry but still tacky [2][9]. Oil gilding cannot be burnished. It "lacks this capacity for brilliance": its lustre is soft and even. But it is more robust than water gilding and less troubled by humidity [12].

**On bronze.** Historic gilt bronze (ormolu) was fire-gilt. An amalgam of gold and mercury was applied and the mercury driven off by heat, leaving gold that holds 13–16 % mercury [9]. The fumes cause serious illness. France legislated against the method around 1830, and it gave way to electroplating [13]. The museum's gilt bronze is oil gilt over the clean metal.

## How it ages (`gilt.wear`)

Gold does not tarnish, so gilding ages in only two ways: by losing its leaf and by gathering dirt.

**Wear.** The leaf is a tenth of a micrometre thick, and any rubbing wears through it. The high points of carving go first, then the edges that hands and dusters pass.
- On water gilding, the worn places show the bole, a dull red glinting through the gold, and then the white gesso.
- On oil gilding they show the size, and then the substrate.

**Dirt and humidity.** Dust and soot settle in the hollows. Water gilding is hygroscopic: its glue and chalk swell and shrink with the humidity, so over decades it cracks and flakes [12].

**Longevity.** Protected gilding lasts millennia; Tutankhamun's couch is about 3,000 years old. Exposed gilding "will not last so long" [2].

**Walked gilt.** Where gilt is walked on, as at the arrival sun, feet wear the leaf through wherever they land. The surface law will show the brown bronze coming through there. No measured rate exists, so the wear rate is recorded as unverified.

## What to look for in a photograph

- Burnished water gilding is a gold mirror with sharp highlights. Matte gilding has a soft, even, warm lustre.
- Faint square seams where leaves overlap, about 80 mm apart on flat work.
- Red bole on the rubbed ridges of old carved gilding, and white gesso where it is worn further.
- Darker dirt in the hollows against bright raised gold.
- A crackle of fine lines in old water gilding.

## Its evidence

Gold is calibrated from **tier (a)**: three measured n,k datasets [6][7][8]. No CC0 photograph of real gilding was found on this PC. The old build's `windows/SourceArt/PBR/M3` and `M4` (ambientCG Metal048C "aged gilt" and Metal048A "clean gilt") are procedurally generated, not photographs, and are not evidence. The bole's colour and the gloss of burnished and matte gilding need a tier (b) specimen: a ColorChecker photograph of a water-gilt test board with burnished, matte and rubbed passages over red bole.

## Sources

1. Giusto Manetti Battiloro, "French cut leaf – 24 to 6 karats" and the gold leaf catalogue. https://www.manetti.com/en/product/french-cut-leaf/ ; https://www.manetti.com/en/catalogue/gold-leaf/
2. K. Howell, "Gilding", *The Building Conservation Directory* (1999). https://www.buildingconservation.com/articles/gilding/gilding.htm
3. Wikipedia, "Gold leaf" (citing J. M. Vilfranc, "Thickness of gold leaf", *The Physics Factbook*, 1999). https://en.wikipedia.org/wiki/Gold_leaf
4. Wikipedia, "Gold" (density at 20 °C). https://en.wikipedia.org/wiki/Gold
5. P. B. Johnson, R. W. Christy, "Optical constants of the noble metals", *Phys. Rev. B* 6, 4370–4379 (1972).
6. S. Babar, J. H. Weaver, "Optical constants of Cu, Ag, and Au revisited", *Appl. Opt.* 54, 477–481 (2015).
7. D. Rioux et al., "An analytic model for the dielectric function of Au, Ag, and their alloys", *Adv. Opt. Mater.* 2, 176–182 (2014).
8. CIE 018:2019, CIE 1931 colour-matching functions (doi:10.25039/CIE.DS.xvudnb9b), and CIE standard illuminant D65.
9. Wikipedia, "Gilding" (water, oil and fire gilding; its ref. 3). https://en.wikipedia.org/wiki/Gilding
10. Wikipedia, "Armenian bole". https://en.wikipedia.org/wiki/Armenian_bole
11. Giusto Manetti Battiloro, "Water gilding: tools, instructions and tips". https://www.manetti.com/en/water-gilding/
12. R. Piper, "Oil Gilding vs. Water Gilding: A Comparative Study of Technique, Aesthetics, and Conservation". https://www.rhiannonpiper.com/articles/oil-gilding-vs-water-gilding-a-comparative-study-of-technique-aesthetics-and-conservation
13. Wikipedia, "Ormolu". https://en.wikipedia.org/wiki/Ormolu
