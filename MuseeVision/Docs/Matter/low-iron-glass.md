# Low-iron float glass

**Key:** `low-iron-glass` · **Family:** glass · **Records:**
- the substance: `Data/Matter/Substances/low-iron-glass.json`;
- its workings: `annealed`, `heat-strengthened`, `toughened`, `laminated`, `laminated.uv-filtering` and `anti-reflective`, in `Data/Matter/Workings/`;
- its histories: `glass.soiling`, `glass.cleaning-wear`, `glass.water-stain` and `glass.edge-delamination`, in `Data/Matter/Histories/`.

Low-iron float glass is the museum's glass. It is used in three places:
- **the Rotunda's glazed eye** (D-004), a steel-and-glass lattice after the British Museum's Great Court;
- **the vitrines**;
- **the glazing of the daylit galleries.** There the laminate's UV-filtering interlayer protects the paintings, and no blinds are needed (D-014).

It is ordinary window glass made from purer sand. It looks colourless through its face and pale aqua-blue along its edge. Numbers in brackets refer to the sources at the end. Every number is also in the JSON records, next to its unit and its source.

## How it is made

**The melt.** The batch is:
- sand (SiO2);
- soda ash (Na2CO3);
- limestone (CaCO3);
- dolomite (MgCa(CO3)2);
- a little salt cake (Na2SO4);
- about 20 % cullet, which is broken glass returned to the furnace [3].

A typical clear float glass is:

| Oxide | Share |
|---|---|
| SiO2 | 72.6 % |
| Na2O | 13.9 % |
| CaO | 8.4 % |
| MgO | 3.9 % |
| Al2O3 | 1.1 % |
| K2O | 0.6 % |
| SO3 | 0.2 % |
| Fe2O3 | 0.11 % |

The shares are by weight [4]. The iron comes from the sand. Glassmakers keep a little of it because it helps the melting.

**Where the colour comes from.** Iron sits in the glass in two states [6]:
- **Ferric iron (Fe3+)** absorbs in narrow bands at 385, 420 and 435 nm. The tail of these bands takes out blue-violet light, which leaves a yellow-green cast.
- **Ferrous iron (Fe2+)** has one broad band at 1050 nm. That band is about ten times stronger, and its tail reaches into the visible, taking out red, orange and yellow. This leaves a blue-green cast.

Both kinds of iron also have strong charge-transfer bands in the ultraviolet. Their tails set the apparent UV edge of window glass near 310 nm [6].

Low-iron glass comes from two changes:
- **less iron:** a total iron content below 0.010 % by weight (100 ppm) [5];
- **a less reducing melt:** the ferrous share of the iron is held below 0.20 [5].

What is left absorbs so little that the glass looks colourless through its face.

**The float.** The melt runs onto a bath of molten tin. It spreads to an even thickness and its faces are fire-finished. The ribbon is then annealed in a lehr and cut into sheets. The largest standard sheet (the jumbo) is 6000 × 3210 mm [7].

The two faces differ:
- **The upper face (the air side)** is fire-finished.
- **The face that lay on the tin (the tin side)** carries tin that has diffused into it. Under a shortwave (254 nm) lamp it glows and the air side does not, and glaziers use that glow to find it [9].

Both faces are extraordinarily smooth. Their rms roughness is below 0.25 nm over 1 µm², which makes them perfectly specular at any scale a visitor can see [8]. Pilkington's low-iron glass is made from 2 to 19 mm thick [10].

**The measured constants.**

| Property | Value | Source |
|---|---|---|
| Refractive index | 1.523 at 589.3 nm, 1.511 at 1 µm | [3] |
| Refractive index, a float slide's air side | 1.5223 at 587.6 nm | [13] |
| Dispersion (Rubin's fit) | n = 1.5130 − 0.003169 λ² + 0.003962 λ⁻², for λ from 0.31 to 4.6 µm | [1] |
| Density | 2500 to 2530 kg/m³ | [3], [4] |
| Young's modulus | 70 to 72 GPa | [3], [16] |
| Poisson's ratio | 0.22 to 0.23 | [3], [4] |
| Thermal conductivity | 0.937 W/(m·K) | [3] |
| Specific heat | 880 J/(kg·K) | [3] |
| Thermal expansion | 8.3 × 10⁻⁶ /K | [3] |
| Mohs hardness | 5 to 6 | [3] |
| Hemispherical emissivity | 0.837 to 0.84 | [3], [14] |
| Optiwhite softening point | about 732 °C | [3] |
| Optiwhite annealing point | about 559 °C | [3] |
| Optiwhite strain point | about 526 °C | [3] |

## What light does in it

**Through the face.** At normal incidence each face reflects about 4.3 %, from Fresnel's law at n = 1.525 and 550 nm. A pane reflects about 8 %, and Pilkington measures 0.08 [10].

Absorption inside the glass comes from Rubin's measured extinction coefficient k for low-iron window glass [1]. The absorption coefficient is α = 4πk/λ, as shown below.

| Wavelength | α, low-iron glass | α, clear float glass |
|---|---|---|
| 450 nm | 0.0012 /mm | 0.0054 /mm |
| 490 nm (the least) | 0.00076 /mm | 0.0040 /mm |
| 550 nm | 0.0011 /mm | 0.0050 /mm |
| 650 nm | 0.0035 /mm | 0.016 /mm |

The clear float values are from Rubin's clear glass [2].

These numbers reproduce the maker's measured light transmittance [10] to within 0.005 at the four thicknesses below, and to within 0.007 at every thickness the maker lists (the largest gaps are at 3 and 4 mm, where Pilkington's rounded 0.92 sits 0.006 and 0.007 above the prediction):

| Thickness | From the spectrum, at 550 nm | Pilkington Optiwhite, measured (EN 410) |
|---|---|---|
| 2 mm | 0.915 | 0.92 |
| 6 mm | 0.911 | 0.91 |
| 12 mm | 0.906 | 0.91 |
| 19 mm | 0.899 | 0.90 |

Because of this agreement, the spectrum is the record's evidence. Matter should integrate that spectrum, not a hand-picked tint.

Two more measured values for 4 mm low-iron glass (EN 410) [11]:
- **UV transmittance:** 0.86. Conventional float transmits 0.70.
- **Colour rendering index Ra:** 99.

**Along the edge.** The edge colour is not a tint. It is iron absorption multiplied by the length of the path, which is what Architecture's defect table says.

Through 1 m of low-iron glass the internal transmittance is:

| Colour | Wavelength | Transmittance |
|---|---|---|
| Blue | 450 nm | 0.31 |
| Green | 550 nm | 0.35 |
| Red | 650 nm | 0.03 |

The red is gone, so the edge reads **aqua-blue**. Vitro calls it Starphire's "signature blue edge" [12].

The same metre of clear float glass passes 0.005 at 450 nm, 0.007 at 550 nm and essentially nothing red. That leaves a **dark green** edge. At 100 mm, the depth a polished vitrine edge is seen through, low-iron glass still passes 0.89 in blue, 0.90 in green and 0.71 in red, which gives a faint, clean aqua.

Vogt et al. measured the same spectrum at 5, 10 and 703 ppm of Fe2O3 [15]. Their series is in the record, so a greener, older glass can be specified later from the same physics.

## How it is worked

**Cut and edged (`annealed`).** A glazier scores the glass and snaps it along the score. The edge is then finished in one of three ways:
- **arrised:** a narrow bevel on each arris, which reads as a bright line;
- **ground;**
- **polished:** this shows the glass's own aqua colour.

Annealed glass is optically quiet and can be cut, but it breaks into long shards. Its strengths are:
- characteristic bending strength (EN 572-1): 45 MPa [16];
- mean breaking stress in service: about 41 MPa [3].

**Heat-strengthened** and **toughened.** For both, the finished pane is reheated on rollers to near softening and cooled by jets of air. This leaves the faces in compression.

| | Heat-strengthened (EN 1863) | Toughened (EN 12150) |
|---|---|---|
| Characteristic strength | 70 MPa | 120 MPa |
| Mean breaking stress | 83 MPa | 165 MPa |
| How it breaks | large pieces, which stay in a laminate | small blunt dice |

The sources are [16] and [3]. Because heat-strengthened glass breaks into large pieces, heat-strengthened plies are the usual choice for overhead laminates.

Reheating leaves two marks you can see:
- **Roller wave.** A faint ripple, which architectural quality holds to about 0.08 mm peak to valley in the middle of a pane and 0.2 mm at the border [17].
- **Quench marks (anisotropy).** A pattern of patches where the air nozzles cooled the glass slightly harder. Birefringence makes the pattern visible only in polarised light: skylight at certain angles to the sun, oblique views, or polarising sunglasses. It is inherent to all heat-treated glass and is not a defect [18].

Toughened glass has one more risk. A rare nickel-sulphide inclusion can break it years later. Heat soaking at 290 °C (BS EN 14179-1) screens most of these out, and the residual risk is about one break in 400 tonnes [19].

**Laminated (`laminated`).** Two or more plies are bonded with a polyvinyl butyral (PVB) interlayer under heat and pressure in an autoclave. The interlayer comes 0.38, 0.76, 1.14 or 1.52 mm thick, and the widest rolls are 3220 mm [20][21]. When the glass breaks, the shards stay stuck to the interlayer and the pane stays in place.

PVB's refractive index is 1.479 [21], close to the glass's 1.525. The interface between them reflects only 0.02 %, so a laminate reads as one pane. A 3/0.76/3 mm laminate has:
- visible transmittance: 0.89;
- more than 99 % of UV screened from 300 to 380 nm;
- yellowness index below 1 [21].

**Overhead glazing.** The International Building Code requires at least a 0.76 mm PVB interlayer in sloped glazing, which it defines as more than 15° from vertical [22]. The thinner 0.38 mm interlayer is allowed only in dwellings, for small, low panes. Germany's TRLV allows an annealed laminate only 15 MPa overhead [16].

The Great Court's roof, the model for the Rotunda's eye, is a sound precedent [23]:
- 38.76 mm insulating units;
- an outer 10 mm toughened pane;
- a 16 mm cavity;
- an inner laminate of two 6 mm annealed sheets on 0.76 mm PVB.

Its panes are triangles from 800 × 1500 to 2200 × 3300 mm. Its outer pane was green-tinted and fritted for solar control, with about 30 % light transmission. The museum's eye uses low-iron glass instead.

**UV-filtering laminate (`laminated.uv-filtering`).** Standard PVB screens 99 % of UV only up to 380 nm. The band from 380 to 400 nm, still ultraviolet by convention [26], partly passes. A UV-filtering PVB such as Saflex UV screens 99 % up to 400 nm (ISO 13837 A) [24][25]; Trosifol Extra Protect, the like product tested in [27], blocks from 400 nm. For a 3/0.76/3 mm laminate the two interlayers compare as follows [21][24]:

| | Standard PVB | UV-filtering PVB |
|---|---|---|
| Transmitted UV | below 1 % (300–380 nm) | below 1 % (300–400 nm) |
| Visible transmittance | 0.89 | 0.88 |
| Damage-weighted transmittance, Krochmann | 0.30 | 0.21 |
| Damage-weighted transmittance, ISO | 0.62 | 0.53 |

In 148 days of outdoor exposure over the summer of 2021, behind laminates of two 4 mm panes, the 400 nm interlayer measurably delayed the fading of medium-stable samples (Blue Wool 4 to 7, most clearly 5 and 6), compared with a 380 nm standard PVB [27]. There is a small visual cost: cutting 380–400 nm slightly yellows the light [28].

Conservators measure UV in microwatts per lumen:
- **daylight:** about 600 to 1000 µW/lm;
- **daylight through window glass:** about 400 to 500 µW/lm;
- **the traditional limit:** 75 µW/lm [26];
- **what modern filters easily achieve:** 0 to 10 µW/lm [29].

Behind this laminate, daylight's UV can be at most about 11 µW/lm. That bound is at most 1000 µW/lm × under 0.01 transmitted ÷ 0.88 visible. The true value is lower, and Art.WithinCare measures it with Light's probe.

**Anti-reflective (`anti-reflective`).** Thin metal-oxide layers are built up on both outer faces. SCHOTT applies them by dip coating and burns the layers in. The reflections from the layer boundaries cancel across the visible, which gives [30]:
- a pane reflectance of about 1 %, against plain glass's 8 %;
- transmittance of 0.98 for 4 mm monolithic glass, or 0.97 laminated 4/0.76/4;
- Ra of 100;
- UV transmittance of 0.01 when laminated;
- hard faces (9H).

The glass underneath is extra-clear low-iron float. Pilkington's OptiView Protect is the like product: coated faces 1 and 4 of a laminate, reflectance under 2 % and more than 99 % of UV blocked [31]. This is vitrine glass. The visitor sees the object and not the room.

## How it ages

- **Soiling** (`glass.soiling`). City air lays a film on the weather face:
  - **soot** absorbs light, up to about 16 %;
  - **soluble salts** scatter it, up to about 20 % of the light diffused.

  Both saturate with deposit, following Favez's measured curves from six European cities [32]. In Paris, sheltered float glass soiled along a sigmoid curve over two years [33]. Measured losses on real windows [32]:
  - 4 to 8 % on commercial buildings in clean environments (UK);
  - rarely more than 10 % of diffuse transmittance on vertical windows (UK);
  - 9 to 36 % on tropical vertical and horizontal windows (Singapore).

  The loss depends on the particulates in the air, on rain and on the building's form [32]; rain washes a steep face and a flat one keeps its dirt. An overhead eye therefore needs roof cleaning. Glass has no open pores, so all the dirt is a film on the face. Indoors the same film comes from touch: fingerprints at hand height on vitrines.
- **Cleaning wear** (`glass.cleaning-wear`). Grit trapped under a squeegee, or a razor dragged across the face, leaves hairline scratches. They are invisible when you look through the glass and appear when low sun rakes the face [34].
- **Water and alkali** (`glass.water-stain`). Concrete or mortar slurry that runs down or splashes on glass is especially damaging, and GANA says to wash it off at once. Glass kept wet in storage can stain so that it needs an aggressive cleaning [34]. The chemistry is not asserted in the record, because no readable source was found; it is listed under Not verified.
- **The laminate's edge** (`glass.edge-delamination`). Moisture that reaches the interlayer at an exposed edge clouds it into a band of delamination:
  - modern PVB: none to 2 mm in service; up to 4.7 mm after 54 months of open-edge Florida exposure, most defects under 3.1 mm;
  - an obsolete PVB: 8 to 12 mm, at the Sydney Opera House and Changi Terminal 2 [35].
- **Bloom.** Reheating iron-poor glass can raise a faint haze on the tin side, from a tin-rich skin 50 to 150 nm thick [8]. This is a making defect rather than ageing, but it is peculiar to low-iron glass.

## What to look for in a photograph

1. **The edge.** A polished edge, or light running along a pane, is aqua-blue in low-iron glass and bottle-green in ordinary glass. If an edge is green, it is not low-iron. If the face is tinted, the glass is not clear at all (the Great Court's roof is green-tinted and fritted).
2. **Two reflections per pane.** Each face reflects about 4 % head-on, rising toward grazing. There are two images offset by the thickness, and four in an insulating unit. A laminate adds none.
3. **Straight lines in reflection.** A slow ripple means reheated glass, which shows roller wave. A perfectly calm reflection means annealed glass.
4. **Patches under polarised skylight** mean quench marks, from heat treatment.
5. **Dirt.** A grey veil on the weather face of sloped glass, heaviest where water pools against glazing bars, with drainage streaks below them.
6. **Vitrines.** Coated glass gives a dim, slightly coloured reflection. Look for fingerprints at hand height and a thin line of interlayer at a polished laminated edge.
7. **Colour through the face.** It should be neutral (Ra 99). A faint warmth under a UV-filtering laminate is the violet cut.

## Evidence

- **Tier (a), measured tables.** Architecture calibrates glass mainly from these:
  - Rubin's and Vogt's spectra [1][2][15];
  - Pilkington's transmittance table [10];
  - SCHOTT's EN 410 table [30];
  - Eastman's interlayer data [21][24].
- **Photographs on this PC.** There are none. `windows/SourceArt` has no CC0 photograph or PBR set of architectural glass. The Albion lancet images show stained-glass designs, which are Art's. JS4 is flint.
- **Photographs worth adding.** Photographs at known exposure (tier c) would help:
  - a polished low-iron edge beside a clear float edge;
  - the Great Court roof from below under an overcast sky;
  - an anti-reflective vitrine face.

## Not verified

These values are kept in the records' `unverified` lists and not used:
- **Glass-on-glass friction** (0.9–1.0 static, 0.4 kinetic). This is quoted on the web without a primary source.
- **Guardian UltraClear's 0.01 % iron.** The page refused access.
- **The iron content of Rubin's sample.** The report is a scan without text.
- **The original source of the glass acoustic row.** It is commonly credited to Everest's *Master Handbook of Acoustics*, but that was not confirmed.
- **Two EN numbers seen only in search summaries:**
  - the heat-soak duration of 2 h;
  - an EN 12150-1 roller-wave limit of 0.5 mm over 300 mm.
- **The soiling deposit rate against time** (Lombardo's Hill parameters).
- **The mechanism of water stain.** Water leaching sodium and calcium, then the alkaline film attacking the silica, was seen only in search summaries of pages that refused access.
- **The density of cleaning scratches** on in-service glass. No measurement was found.
- **The CIBSE LG10 and BS 8206-2 tables of dirt loss by slope.**

## Sources

1. M. Rubin, "Optical properties of soda lime silica glasses", *Solar Energy Materials* 12 (1985) 275–288. The low-iron glass's k and dispersion, via https://refractiveindex.info/?shelf=glass&book=soda-lime&page=Rubin-lowiron
2. The same paper, clear glass: https://refractiveindex.info/?shelf=glass&book=soda-lime&page=Rubin-clear
3. Pilkington North America, Technical Bulletin ATS-129, *Properties of soda-lime silica float glass*, 04/2025: https://www.pilkington.com/-/media/pilkington/site-content/usa/window-manufacturers/technical-bulletins/ats-129---properties-of-soda-lime-silica-float-glass.pdf
4. Pilkington, *Properties of soda-lime-silica float glass* (earlier edition, citing Button and Pye, *Glass in Building*, 1993): http://www.swiftglass.com/assets/floatglass.pdf
5. Vitro (PPG), patent EP2396284B1, *Method of reducing redox ratio of molten glass for making ultra-clear glass*: https://patents.google.com/patent/EP2396284B1/en
6. J. E. Shelby, *Color in commercial glasses*, IMI-NFG Glass Processing Course, Lehigh University, 2015: https://www.lehigh.edu/imi/teched/GlassProcess/Lectures/Lecture04_Shelby_ColoredGlass.pdf
7. Pilkington, *Optifloat Clear* product page: https://www.pilkington.com/en/gbl/architectural-and-technical-glass/product-categories/thermal-insulation/pilkington-optifloat-clear
8. D. Moseler, G. Heide, G. H. Frischat, "Atomic force microscope study of the topography of float glasses and a model to explain the bloom effect", *Glass Sci. Technol.* 75(4) (2002) 174–183: https://www.osti.gov/etdeweb/biblio/20330699
9. Camlab, *How to distinguish the tin side of float glass*: https://www.camlab.co.uk/blog/how-to-distinguish-the-tin-side-of-float-glass
10. Pilkington Building Products UK, *Performance data summary: Low-iron clear float glass, Pilkington Optiwhite*, publication 4101, July 2011: https://www.flairwindows.com/assets/pilkington-optiwhite-datasheet.pdf. The 2–19 mm range is from https://www.pilkington.com/en/gbl/architectural-and-technical-glass/product-categories/enhanced-visibility/pilkington-optiwhite
11. SCHOTT, *AMIRAN technical data sheet*, February 2022, the "conventional float glass in comparison" rows: see [30].
12. Vitro, *Heavy Starphire Glass*: https://www.vitroglazings.com/products/low-iron-glass/starphire-ultra-clear-glass/heavy-starphire-glass/
13. A. Kamptner, M. C. Scharber, M. Schiek, *ChemPhysChem* (2024) e202400233, via https://refractiveindex.info/?shelf=glass&book=soda-lime&page=Kamptner
14. M. Rubin, LBNL publication record (abstract, emissivity 0.837): https://windows.lbl.gov/publications/optical-constants-and-bulk-optical
15. M. R. Vogt et al., *IEEE J. Photovoltaics* 6 (2016) 111–118, via https://refractiveindex.info/?shelf=glass&book=soda-lime&page=Vogt-10ppm (with the 5 ppm and 703 ppm pages)
16. Saint-Gobain Building Glass UK, *Glass Fundamentals GF-1B: The Strength of Glass*, 2018: https://www.saint-gobain-glass.co.uk/wp-content/uploads/2024/02/Glass-Fundamentals-1B-The-Strength-of-Glass-19-09-2018-1.pdf
17. D. Lohrer, J. P. Martínez, *Making glass flatness a standard*, glassonweb, 2024: https://www.glassonweb.com/article/making-glass-flatness-standard
18. Viracon, *Quench pattern characteristics*, VTT-009, 2019: https://viracon.com/wp-content/uploads/2020/03/VTT-009_QuenchPatternCharacteristics.pdf
19. Glazing Vision, *Reducing the risk of spontaneous glass breakage*, 2025: https://glazingvision.com/news-and-insights/reducing-the-risk-of-spontaneous-glass-breakage/
20. Eastman, *Saflex Clear PVB interlayer* bulletin AI-ARCH-4321, 2018: https://saflex-vanceva.eastman.com/content/dam/saflex/product-pages/saflex-clear/ai-arch-4321_saflex_clear_product_bulletin.pdf
21. Eastman, *Saflex Clear* technical data 031720: https://saflex-vanceva.eastman.com/content/dam/saflex/pdf-documents/arch/saflex-clear/product_technical_sheet_-_saflex_clear_031720_0.pdf
22. International Building Code 2024, §2405 (TWIA 2024 adoption), via https://up.codes/s/sloped-glazing-and-skylights
23. Pilkington, *The Great Court at The British Museum*: https://projects.pilkington.com/show/7555/The-Great-Court-at-The-British-Museum-London-UK.aspx
24. Eastman, *Saflex UV (RB4U)* technical data 031720: https://saflex-vanceva.eastman.com/content/dam/saflex/pdf-documents/arch/saflex-uv/Product%20Technical%20Sheet%20-%20Saflex%20UV.pdf
25. Eastman, *Saflex UV* page: https://saflex-vanceva.eastman.com/en/architectural/interlayer-performance/uv
26. S. Michalski, *Agent of Deterioration: Light, Ultraviolet and Infrared*, CCI, 2018: https://www.canada.ca/en/conservation-institute/services/agents-deterioration/light.html
27. M. Müller, S. Reich, C. Pfütze, I. Stelzer, "Investigation of color protection of laminated glass by UV-blocking interlayers for conservation application", *Glass Struct. Eng.* (2024), doi:10.1007/s40940-023-00246-9. Summary: https://www.glassonweb.com/article/investigation-color-protection-laminated-glass-uv-blocking-interlayers-conservation
28. J. Tétreault, C. Anuzet, *Ultraviolet Filters*, CCI Notes 2/1, 2015: https://www.canada.ca/en/conservation-institute/services/conservation-preservation-publications/canadian-conservation-institute-notes/ultraviolet-filters.html
29. CCAHA, *Light Exposure for Artifacts on Exhibition*: https://ccaha.org/resources/light-exposure-artifacts-exhibition
30. SCHOTT, *AMIRAN – Anti-Reflective Glass for Showcases*, technical data sheet, February 2022: https://media.schott.com/api/public/content/15fee70340c44e738fd4f24c40702d8c?v=35137cd2&download=true
31. Pilkington, *OptiView Protect*: https://www.pilkington.com/en/gbl/architectural-and-technical-glass/product-categories/enhanced-visibility/pilkington-optiview-protect
32. J. Du, X. Zhang, S. Sharples, "Air pollution and daylight availability in the urban area", *Proc. IBPSA 2017*, 437–444 (quoting Favez et al. 2006; Tregenza et al. 1999; Sharples et al. 2001; Ullah et al. 2003): https://publications.ibpsa.org/proceedings/bs/2017/papers/BS2017_114.pdf
33. T. Lombardo et al., "Soiling of silica-soda-lime float glass in urban environment: measurements and modelling", *Atmos. Environ.* 39(6) (2005) 989–997: https://hero.epa.gov/reference/126177/
34. GANA, *Glass Informational Bulletin 01-0300: Proper procedures for cleaning architectural glass products*: https://euroline-windows.com/wp-content/uploads/2023/04/GANA-Proper-Procedures-For-Cleaning-Glass.pdf
35. P. Davies, R. Cadwallader (DuPont), *Delamination issues with laminated glass: causes and prevention*, Glass Processing Days (c. 2003): https://www.aisglass.com/wp-content/uploads/2020/11/AIS-82.pdf

Acoustic absorption by octave band (125 Hz to 4 kHz): 0.18, 0.06, 0.04, 0.03, 0.02, 0.02 for large heavy panes. The sources are https://commercial-acoustics.com/sound-advice/sound-absorption-coefficient-chart/ and http://usenclosure.com/Diffraction%20Graphs/Sound%20Absorption%20Coefficients.pdf.
