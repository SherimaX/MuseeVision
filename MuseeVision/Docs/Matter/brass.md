# Brass: cartridge brass CuZn30, the bronze family's accent

**Key:** `brass` · **Family:** accent of `bronze` · **Records:**
- the substance: `Data/Matter/Substances/brass.json`;
- its workings: `brushed` and `patinated`, in `Data/Matter/Workings/`;
- its histories: `bronze.tarnish`, `bronze.wear` and `bronze.weathering`, in `Data/Matter/Histories/`. They are shared with the bronze because they are the same copper chemistry.

The master boards call some metal "brass". Decision D-014 reads it as a **declared accent of the bronze family**, never a second bronze. The museum uses brass only where a wrought yellow copper alloy is the real material:
- the woven mesh of the Reserve's racks, which is drawn wire;
- small brushed fittings such as the feet of the steles in the Salon and the Atrium.

Everything cast, walked or large is the bronze. Numbers in brackets refer to the sources at the end. Every number is also in the JSON record, next to its unit, grade and source.

## Which brass, and why

The alloy is **cartridge brass**, 70 % copper and 30 % zinc [1]:
- UNS C26000, "Cartridge Brass, 70%";
- EN CW505L, CuZn30.

It is a single-phase alpha brass, readily worked cold. It is rolled to sheet and strip, drawn to rod, tube and wire (and so woven to mesh), and deep-drawn. It is made to the ASTM standards B36, B134, B135, B19 and B569 [1]. Screws and bolts of the same alloy match its colour [1]. It was chosen because its optical constants have been measured on exactly this 70/30 composition [2], so its colour is known from tier (a) evidence without a proxy.

## How it is made

Copper and zinc are melted together and cast as slabs or billets. These are then hot and cold rolled, extruded and drawn. Cold work hardens it and annealing softens it. It begins to melt (its solidus) at 916 °C [1].

Its properties are [1]:
- density 8,530 kg per cubic metre;
- Young's modulus 110.3 GPa;
- expansion 20.0 millionths per degree Celsius;
- thermal conductivity 121 W/(m·K);
- 28 % of copper's electrical conductivity.

As half-hard sheet (H02) its tensile strength is 427 MPa and its yield strength 359 MPa, with 25 % elongation.

## How it is worked

**Brushed (`brushed`).** This is the M32 medium satin of the bronze: 180–240 grit, in one direction [1]. Handled brass fittings take an INCRALAC-type clear coat, an acrylic lacquer with benzotriazole. Baked films of it resist abrasion best indoors, but they need periodic maintenance [1]. The hardware trade's name for this finish is BHMA 606, "satin brass, clear coated" [1].

**Patinated (`patinated`).** Brass takes the same sulfide statuary finishes as the bronze [1]. The museum does not use it on brass, but the record admits it.

## Its optics

Brass's normal-incidence reflectance (F0) is computed from Querry's measured optical constants of Cu70Zn30 brass [2]. They are integrated over the visible spectrum under D65 [3]. The result is **[0.909, 0.779, 0.422]** in linear Rec.709, with luminance 0.781. By wavelength, the F0 is 0.434 at 450 nm, 0.789 at 550 nm and 0.887 at 650 nm.

The edge tint at 82 degrees is [0.907, 0.865, 0.709]. The brass whitens toward grazing.

The bronze's F0 is [0.961, 0.693, 0.491]. Beside it, the brass is plainly yellower: its green is higher and its red lower. That is why brass stays an accent and never stands in for the bronze. The CDA gives the natural colour of C26000 as "yellow", and places it in the "Yellow-Gold" colour group [1].

## How it ages

**Indoors.** Unlacquered brass dulls to a yellow-brown and keeps fingerprints: skin oils tarnish copper alloys [1]. Lacquered brass stays bright until the coat breaks at an edge.

**Handled.** Rubbing polishes it bright. Wear follows the hands (`bronze.wear`).

**Outdoors.** Cartridge brass weathers "yellowish, ultimately to gray-green patina" [1].

## What to look for in a photograph

- Clean brass is a clear yellow, yellower than the golden bronze beside it.
- Unlacquered brass dulls to yellow-brown with fingerprints. Lacquered brass stays bright until the lacquer breaks.
- Woven mesh reads as a fine yellow grid whose sheen changes with the angle of view.

## Its evidence

The brass is calibrated from **tier (a)**: Querry's measured n and k of the exact alloy [2]. No CC0 photograph of real brass was found on this PC. The old build's `usd/textures/brass_mesh.png` has no recorded source, so it is not evidence.

## Sources

1. Copper Development Association, *Copper Brass Bronze Design Handbook: Architectural Applications* (A4039): Table 1 (C26000), Tables 7, 8 and 15, pp. 26, 39 and 42. https://www.copper.org/publications/pub_list/pdf/A4039-ArchitecturalApplications.pdf
2. M. R. Querry, *Optical constants*, Contractor Report CRDC-CR-85034 (1985), https://apps.dtic.mil/sti/citations/ADA158623. Data file: refractiveindex.info database, other/alloys/Cu-Zn, Querry-Cu70Zn30.
3. CIE 018:2019, CIE 1931 colour-matching functions (doi:10.25039/CIE.DS.xvudnb9b), and CIE standard illuminant D65.
