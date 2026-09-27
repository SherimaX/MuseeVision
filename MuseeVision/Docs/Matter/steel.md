# Steel: structural steel S355J2, the lattice of the Rotunda's eye

**Key:** `steel` · **Family:** accent of `glass` · **Records:**
- the substance: `Data/Matter/Substances/steel.json`;
- its working: `painted.steel`, in `Data/Matter/Workings/`;
- its history: `steel.coating-ageing`, in `Data/Matter/Histories/`.

The Rotunda's eye is glazed (D-004). It is a triangulated steel-and-glass lattice, after the roof of the British Museum's Great Court. As built, it has:
- a ring (radius 2.42–2.50 m) carrying the glass at 20.38 m;
- bars in three directions, 60 degrees apart and 0.8 m apart, each 50 mm wide and 100 mm deep.

This record is the steel of that lattice. The steel is never seen bare: it is always under its paint. It is declared an **accent of the glass family** because it exists only to carry the glass. D-014 admits no core family for steel, so this reading is the record's; the Architect may overturn it.

Numbers in brackets refer to the sources at the end. Every number is also in the JSON records, next to its unit, grade and source.

## The precedent: the Great Court

The Great Court roof was built by Foster and Partners with Buro Happold, fabricated and erected by Waagner-Biro, and opened in 2000. Its figures [1][2][3]:
- about 800 tonnes of steel and glass over 5,900 m² of glazing;
- 4,878 members meeting at 1,566 nodes, the counts Waagner-Biro gives. Architectural Record counts 1,826 nodes of six welded members each;
- 3,312 double-glazed triangles, each unique: toughened glass above, laminated below, lightly tinted, with fritted dots over half their surface.

The members are welded steel boxes that taper, from about 3 in deep at the Reading Room to about 7 in at the corners. Buro Happold specified "Grade D" steel, rather than a lower grade, to reduce the risk of brittle failure at the welds [3].

The British grade D of the 355 MPa class is BS 4360 grade 50D. Today it is EN 10025-2 S355J2 [4]. That is the museum's steel. Reading "Grade D" as 50D is recorded as unverified, because the source does not give the strength class.

## How it is made

Iron is reduced from ore (or scrap is melted), refined to steel with its carbon and manganese set, cast continuously, and hot rolled to plate, flats and sections. The lattice's members are:
- hot-finished rectangular hollow sections of S355J2H (EN 10210);
- or boxes welded from S355J2 plate where members taper, as in the Great Court.

They are cut, bevelled and welded at the nodes in the fabricator's shop, and the welds are ground and faired.

The steel's cast analysis limits are [5]:

| Element | Maximum (wt%) |
|---|---|
| C | 0.20 (thickness up to 40 mm) |
| Si | 0.55 |
| Mn | 1.60 |
| P, S | 0.030 each |
| Cu | 0.55 |

Its carbon equivalent (CEV) is at most 0.45 up to 30 mm thick, which keeps it weldable [5]. Its properties [4][6]:
- yield strength at least 355 MPa;
- tensile strength 470–630 MPa;
- Charpy toughness 27 J at −20 °C, the "J2";
- density 7,850 kg per cubic metre, Young's modulus 210 GPa, Poisson's ratio 0.3;
- expansion 12 millionths per degree Celsius, less than the bronze (18) or brass (20).

## How it is worked

**Painted (`painted.steel`).** After welding, the lattice is abrasive blast-cleaned to Sa 2½ (ISO 8501-1): mill scale, rust and old coatings removed, only slight staining left [7]. It is then shop-sprayed with a certified two-coat system for a C2 atmosphere of very high durability (more than 25 years to first major maintenance) [8]:
1. an epoxy build coat, 125 µm;
2. a polyurethane topcoat, 55 µm.

The total is 180 µm of dry film [8].

The Rotunda is heated, which is C1: steel loses at most 1.3 µm a year there [7]. But the lattice lies under the glazing, where condensation can form on cold nights. So the record specifies the next category, C2 (1.3–25 µm a year) [7]. This choice is reasoned, not measured.

**Colour.** The topcoat is the as-built lattice colour, a dark neutral grey (tint #3C3F42), with a linear albedo of about 0.05. It comes from the finished museum's design, not from a paint measurement. The old build also has a warmer lattice grey for the Hall of Light (#3B3A36). One steel in one paint would give the museum one lattice colour, and the choice between them is the Architect's.

**Optics.** The paint is a dielectric coat. Its F0 is about 0.04 for a binder index near 1.5, which is unverified. The spray leaves a faint orange peel. The bare steel's own F0, which only a cut edge would show, is [0.529, 0.513, 0.494] from iron's measured optical constants [9].

## How it ages

The paint system is rated to last more than 25 years in C2 before its first major maintenance [8]. Until then the steel does not corrode. Indoors and under glass, the surface changes slowly:
- **Dust and soot** settle on the upward faces of the members. The museum's deposition rates are those recorded with the white family's soiling.
- **Ultraviolet** reaches the paint only through the glazing. The glass group records its UV transmittance: low-iron float passes about 0.86, and the UV-filtering laminate under 0.01.
- **Condensation**, where it forms, can leave faint marks along lower edges.

Nothing here is walked or touched.

## What to look for in a photograph

- Slender dark grey bars against the sky, with a soft satin sheen.
- A faint orange peel seen only close to, and faired welds with no bolts and no rust bleed.
- From below at a low angle, slightly lighter, dusty upper edges.

## Its evidence

The steel needs no visual calibration, because it is always painted. The paint's colour is a design value. No CC0 photograph of a real painted steel lattice was found on this PC. The old build's `windows/SourceArt/PBR/M5` (ambientCG Metal029, "satin lacquer orange peel") is procedurally generated, not a photograph, and is rejected. A photograph of the Great Court roof at a known exposure would be tier (c) evidence for the lattice's appearance; its paint colour and system are not in any source that could be read.

## Sources

1. Waagner-Biro Steel and Glass, "Queen Elizabeth II Great Court, British Museum". https://www.wb-sg.com/projects/britishmuseum/
2. Buro Happold, "The British Museum, Queen Elizabeth II Great Court". https://www.burohappold.com/projects/queen-elizabeth-ii-great-court-the-british-museum/
3. S. Hart, *Architectural Record* (January 2003), as quoted in "Buro Happold: British Museum", Cantilever, https://cantilever.wordpress.com/about-innovation/buro-happold-british-museum/. See also S. Brown and M. Cook, "The Design and Manufacture of the British Museum Great Court Roof" (2002).
4. AJN Steelstock, "Grades, Properties and Nearest Equivalents" (EN 10025-2:2004, EN 10025:1993, BS 4360:1990). https://www.ajnsteelstock.co.uk/wp-content/uploads/2022/11/Grades-Properties-and-Nearest-Equivalents.pdf
5. SteelNumber, "S355J2 / 1.0577" (EN 10025-2:2004). http://www.steelnumber.com/en/steel_composition_eu.php?name_id=10
6. EN 1993-1-1:2005, Eurocode 3, Design of steel structures, Part 1-1, clause 3.2.6 (elastic constants and expansion); EN 1993-1-2:2005, Part 1-2, clause 3.2.2 (unit mass, 7,850 kg/m³).
7. World of Corrosion, "ISO 12944 Coating Systems" (ISO 12944-2 categories; ISO 8501-1 Sa 2½). https://worldofcorrosion.com/iso-12944.html
8. Sherwin-Williams Protective and Marine, *ISO 12944 Systems Guide*, 2021 edition (compliant with ISO 12944:2018), pp. 2–3.
9. P. B. Johnson, R. W. Christy, "Optical constants of transition metals: Ti, V, Cr, Mn, Fe, Co, Ni, and Pd", *Phys. Rev. B* 9, 5056–5070 (1974).
