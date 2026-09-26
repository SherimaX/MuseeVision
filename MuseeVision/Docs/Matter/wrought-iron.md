# Wrought iron

**Key:** `wrought-iron` · **Family:** accent, of glass · **Records:**
- the substance: `Data/Matter/Substances/wrought-iron.json`;
- its workings: `riveted.iron`, `forged.iron` and `painted.iron`;
- its histories: `iron.paint-ageing` and `iron.corrosion`.

Wrought iron is the iron that carries Albion's glass:
- the riveted box girders on the arcade lines, which carry the lead valley gutters at 10.0 m;
- the built-up main ribs on the column lines, 0.26 m deep, each with a lighter inner arch and a chain of rings;
- the T ribs between them, 0.14 m deep, a rib every 1.5 m;
- the purlins, 0.10 m, and the glazing bars, 0.05 m deep, a bar every 0.75 m;
- the end screens' mullions and transoms;
- the saddle bars of the lancets;
- the forged rings in the spandrels of the arcade's pointed arches.

The sources for these members are [6] and [7]. Like the cast iron, it is always seen under paint. Numbers in brackets
refer to the sources at the end.

## What it is

Wrought iron is nearly pure iron with threads of glassy slag drawn through it. By weight it is 99 to 99.8 % iron and
0.05 to 0.25 % carbon, with a little manganese, sulfur, phosphorus and silicon [2]. It carries up to 2 % slag by
weight, about 250,000 inclusions to the square inch [2].

The slag is what makes it wrought iron:
- **It flows like a flux.** In the fire the slag lets two pieces forge-weld readily [2].
- **It gives the iron a grain like wood.** Worked iron draws the slag into long threads, so a fresh fracture is
  "bluish, silky, fibrous" [2]. Rust or etching brings the grain up.
- **It spreads corrosion.** Rust forms as an even film rather than pits [2].

Wrought iron is tough and ductile, and about as strong in tension as in compression [1]. That is why the Victorians
used it for girders, ribs and ties, and for the smith's ornament. At Oxford the spandrels of branches are wrought iron:
sycamore, walnut and palm among them [4], [5].

## How it was made

Henry Cort's puddling process (1784) [2] runs as follows:
1. **Puddling.** Pig iron is melted on the hearth of a reverberatory furnace, where the fuel never touches the metal.
   The puddler stirs the bath, and the carbon burns out.
2. **Balling.** As the carbon goes, the iron thickens to a paste. He gathers it into balls: a charge of about 200 kg
   makes balls of about 50 kg [1].
3. **Shingling.** The balls are hammered to squeeze out most of the slag.
4. **Rolling.** They are rolled to "muck bar".
4. **Piling.** For the better grades the bars are cut, piled and rolled again.

The last wrought-iron works in the world, Thomas Walmsley and Sons' Atlas Forge at Bolton, closed in 1973 [2]. Engine
Shed puts the end in the 1960s [3]. Conservation now uses salvaged and re-rolled iron. The museum depicts the iron;
Matter records it as the real thing.

## The measured constants

| Property | Value | Source |
|---|---|---|
| Tensile strength | 234-372 MPa | [2] (after *Machinery's Handbook*) |
| Yield point | 159-221 MPa | [2] |
| Yield, suggested for appraisal | 220 MPa, similar in tension and compression | [1] |
| Young's modulus | 193 GPa | [2] |
| Density | 7500-7900 kg/m³ (specific gravity 7.5-7.9) | [2] |
| Melting point | about 1540 °C | [2] |

Measured tests of 19th-century British wrought iron exist, but those papers were paywalled and are listed as
unverified.

## How it is worked

**Riveted** (`riveted.iron`). Rolled plates, angles and tees are punched or drilled and built into box girders and
I-section ribs. Red-hot rivets are driven through the holes and headed; as they cool they shrink and pull the plates
together. Every seam carries a row of domed heads. In the frozen code these heads have a radius of 11 mm, and the bolt
heads 16 mm [6]. The hot-riveting description itself is trade knowledge not read from a source, and is marked so.

**Forged** (`forged.iron`). The smith heats the bar and works it on the anvil:
- drawing tapers and scrolling;
- bending rings over a mandrel;
- joining pieces by forge-welding or with collars and rivets.

Leaves are cut from sheet, veined and dished. At Oxford the capitals of the iron clusters carry wrought-iron flowers and
foliage: water-lily, passion-flower and holly [4]. In Albion the smith's work is the spandrel rings and the saddle bars.

**Painted** (`painted.iron`). It takes the same lead-and-linseed-oil schemes as the cast iron; see the cast-iron dossier.

## How it ages

Wrought iron is "more resistant to corrosion than mild steel", but less so than cast iron [3]. It is also "more
vulnerable to water damage" [1]. It rusts along its grain: "As iron corrodes and delaminates (its layers separate) it
expands", and thin members "bend and distort as deterioration progresses" [3]. Under failed paint the rust lifts in
long blisters over swollen, layered iron. The paint's own ageing is in `iron.paint-ageing`.

## What to look for in a photograph

- Built-up members: separate plates and angles under the paint, and rows of domed rivet heads along every seam.
- Forged work: hammer-softened arrises and tapering scrolls, but none of the parting lines of a casting.
- Where it has rusted, iron swollen into layers along its grain.

## Sources

1. M. Bussell, "19th-century Structural Ironwork in Buildings", *The Building Conservation Directory* (2011).
2. Wikipedia, "Wrought iron":
   - its Table 2 is cited to Oberg, Jones and Ryffel, *Machinery's Handbook*, 26th ed. (2000), p. 476;
   - its Table 1 is cited to Camp and Francis, *The Making, Shaping and Treating of Steel* (1920);
   - the Atlas Forge closure is cited to Smith and Gale, *Historical Metallurgy* 21 (1987).
3. Historic Environment Scotland, Engine Shed, "Iron".
4. Oxford University Museum of Natural History, "Design and decoration: Central Court Roof".
5. Oxford University Museum of Natural History, "The Museum's Architecture".
6. `windows/Source/MuseeVision/Albion/AlbionIron.cpp` (frozen reference).
7. `windows/Source/MuseeVision/Albion/AlbionPlan.h` and the Albion Details board (approved proposal).
