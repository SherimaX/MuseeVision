# Grey cast iron

**Key:** `cast-iron` · **Family:** accent, of glass · **Records:**
- the substance: `Data/Matter/Substances/cast-iron.json`;
- its workings: `cast.iron` (the foundry) and `painted.iron` (the paint it is always seen under), in `Data/Matter/Workings/`;
- its histories: `iron.paint-ageing` and `iron.corrosion`, in `Data/Matter/Histories/`.

Cast iron is the iron of Albion's clustered shafts. The court's stone stops at the carved capitals, 6.2 m up; above them
everything is iron and glass. From each capital a cluster of cast-iron shafts rises on a bolted shoe to 9.7 m, where a
riveted girder carries the valley gutter. Cast iron also makes the ridges, ventilators, rainwater heads and downpipes.
The visitor never sees the metal itself, only its paint. Numbers in brackets refer to the sources at the end. Every
number is also in the JSON record, next to its unit, grade and source.

## What it is

Grey cast iron is iron with about 4 % carbon [2]. Molten pig iron carries 4 to 4.5 % [1]. When the iron freezes in the
mould, most of that carbon comes out as flakes of graphite, randomly oriented. The flakes make up 6 to 10 % of the
volume [3], and they give a fresh break its grey, crystalline look [2].

The flakes decide how the iron behaves:
- **In compression it is very strong**, "actually stronger than mild steel" [1].
- **In tension it is weak.** The flakes act as voids with sharp corners, so the iron cracks and fails "abruptly in a
  brittle manner" [1].

This is why the Victorians used it for columns and not for beams or ties. It is also why Oxford's roof stands on it. At
the Oxford University Museum, Skidmore's first roof was built entirely of wrought iron. It failed in February 1858,
when thicker glass had been specified. He rebuilt it by early 1859 on structural cast-iron columns and kept the
decorative wrought iron [6], [7].

Grey iron is the kind met in Victorian buildings: re-melted pig iron shaped by sand casting [1].

## How it is made

1. **The pattern.** The patternmaker carves the part in wood about one per cent oversize, because the iron shrinks as
   it cools [1]. Every face is given draft so the pattern will lift out of the sand.
2. **The mould.** The moulder rams damp green sand round the pattern in a two-part flask, lifts the pattern, and sets
   cores for any hollows [2].
3. **The pour.** Pig iron is remelted and poured at about 1300 °C. That is far below the melting point of pure iron,
   about 1535 °C [1].
4. **Fettling.** Once cold, the casting is knocked out and its runners and fins are broken off.

The casting keeps the marks of the mould: the sand's grain, parting lines, draft and generous fillets. It also keeps a
hard skin that resists corrosion well [1].

Cast iron is not welded. Parts are joined by bolted flanges and spigots, and the as-built shafts stand on bolted shoes
over the stone [9].

## The measured constants

Victorian iron was not made to any class. The modern ASTM A48 Classes 20 to 30 bracket its strength and stiffness.
That bracket is this record's reading, and it is listed as unverified.

| Property | Value | Source |
|---|---|---|
| Carbon, molten pig iron | 4-4.5 % by weight | [1] |
| Graphite | 6-10 % of the volume | [3] |
| Tensile strength, Class 20 | 152 MPa (22 ksi) | [3] |
| Tensile strength, Class 30 | 214 MPa (31 ksi) | [3] |
| Compressive strength, Class 20 | 572 MPa (83 ksi) | [3] |
| Young's modulus, Classes 20-30 | 69-97 GPa (10-14 Mpsi); it falls as the graphite share rises | [3] |
| Thermal conductivity, Class 40 | 53 W/(m·K) | [4] |
| Casting temperature | about 1300 °C | [1] |
| Shrinkage allowance | about 1 % | [1] |

Two values were not confirmed from a primary source, and are recorded only as unverified:
- **density:** 7100-7300 kg/m³;
- **thermal expansion:** about 10-11 × 10⁻⁶ /K.

## What light does

Bare cast iron is a metal. Its constants are iron's, already recorded from Johnson and Christy in the steel record.
Graphite flakes darken a bare, cut face. But no bare face is ever shown: the optical surface of Albion's iron is its
paint, described in `painted.iron` below.

## How it is worked: the paint (`painted.iron`)

The period system is oil paint [8]:
- **The primer.** Red lead in linseed oil was "the most common pigment in linseed oil primers in the late 18th and 19th
  centuries".
- **The build and finish coats.** White lead (lead carbonates and sulfates) ground in linseed oil, with turpentine,
  driers and coloured pigments. A scheme is two to four coats, sometimes more.

Iron is repainted for generations. One analysed sample held 37 schemes, 35 of them lead paints [8]. Today lead paint may
be used in the UK only to maintain historic finishes [8]. The modern equivalent is a zinc-based primer, a micaceous iron
oxide coat and two coats of gloss [2].

**The colours of the period.** The Victorians painted their iron in "invisible" greens through the first half of the
century. Mid-century, green stayed common, but "dark blue, red and chocolate brown were also popular" [10]. Oxford's
roof carried stencilled patterns on its arched girders. That scheme was revised during building and repainted in April
1864 by G. Thicke and J. Randall, whose inscription was found in 2013 [6].

**The as-built court.** Albion painted all its iron one dark slate grey (#434C50), satin, correctly as a non-metal
[11]. The design stands (D-001), but the research corrects one thing. A real Victorian finish is a stack of schemes: many
coats round every arris of a casting, and the film's sheen is set by its age. So Matter should carry the scheme count
and let `iron.paint-ageing` set the sheen and the soiling, rather than use one hand-set roughness.

## How it ages

**The paint** (`iron.paint-ageing`). Linseed oil paint oxidises, hardens, loses its gloss and chalks. Every repainting
campaign thickens the film. Indoors under the glass, soot from gas and London's air settles on every upward face; the
as-built court already darkens toward the springing. The film fails first where it is thinnest or where water lies: at
arrises, bolt heads, joints and shoes. A large, complex cast-iron structure needs a thorough service about every 100
years [5].

**The metal** (`iron.corrosion`). Cast iron resists corrosion better than wrought iron [2]. It has one treacherous
mode. In graphitic corrosion the iron dissolves and leaves the graphite network behind, filled with corrosion products,
so the piece "retains its appearance and shape but is weaker structurally" [12]. Where iron is built into stone (the
shoes on the capitals), rust takes up more volume than the iron it replaces and can crack the stone [2].

## What to look for in a photograph

- The paint, not the metal: a thick film rounding every arris, with runs and sags in the hollows of the cluster.
- The marks of casting: parting lines, draft and fillets. Bolt heads and nuts at every flange, and no welds anywhere.
- Rust bleeding from the joints only where the paint has failed.

No CC0 photograph of Victorian iron is on this PC. The PBR set M5 is procedural and is rejected as evidence.

## Open questions for the Architect

- **The family.** Cast iron is declared an accent of glass, as steel is, because in Albion it exists to carry the glass.
- **The paint.** The court is one slate grey (as built). Oxford's scheme was polychrome and stencilled. The design
  keeps the grey unless the owner asks otherwise.

## Sources

1. M. Bussell, "19th-century Structural Ironwork in Buildings: Understanding, Care and Re-use", *The Building Conservation Directory* (2011).
2. Historic Environment Scotland, Engine Shed, "Iron".
3. Wikipedia, "Gray iron": the ASTM A48 table, cited to P. A. Schweitzer, *Metallic Materials* (CRC Press 2003), p. 73.
4. nuclear-power.com, "Properties of Gray Cast Iron – ASTM A48 Class 40".
5. A. Davey and E. Troup, "Maintaining and Repairing Coatings on Cast Iron Structures", *The Building Conservation Directory* (2016).
6. Oxford University Museum of Natural History, "Design and decoration: Central Court Roof".
7. Oxford University Museum of Natural History, "The Museum's Architecture".
8. K. Blackney, "Painting Historic Ironwork", *Historic Churches* (2002).
9. `windows/Source/MuseeVision/Albion/AlbionIron.cpp` and `AlbionPlan.h` (frozen reference).
10. The Victorian Society, "Paint colours and finishes".
11. `windows/Scripts/albion_materials.py`, `MI_Albion_Iron` (frozen reference).
12. Corrosionpedia, "Graphitic Corrosion".

The full references, with their URLs, are in the JSON records' `sources` fields.
