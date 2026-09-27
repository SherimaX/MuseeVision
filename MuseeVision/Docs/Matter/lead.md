# Lead

**Key:** `lead` · **Family:** accent, of glass · **Records:**
- the substance: `Data/Matter/Substances/lead.json`;
- its workings: `milled.lead` (sheet) and `leaded` (came glazing, shared with `antique-glass`);
- its histories: `lead.patina` and `lead.creep`.

In Albion lead does two jobs:
- **It holds the glass.** The H-section cames of the Tristram and Isolde band and of the twelve lancets are lead.
- **It keeps the water off the glass roofs.** The valley gutters on the arcade lines (10.0 m) and the porch's lead flat
  (7.45 m) are lead [6], [7].

It is declared an accent of the glass family, because it exists for the glass. Numbers in brackets refer to the sources
at the end.

## What it is

Lead is soft, dense and heavy, and it melts low [1]:
- it melts at 327 °C;
- it is 11,348 kg/m³ at 20 °C;
- it is so soft (Mohs 1.5) that a fingernail scratches it.

Freshly cut it is "shiny silvery with a bluish tint". In air it "tarnishes to dull gray" under a thin protective film.
Lead carbonate is a common part of that film, with sulfate or chloride in city or sea air. The film leaves the metal
"effectively inert" [1].

Its stiffness is low and its thermal movement is large [1]:

| Property | Value |
|---|---|
| Young's modulus | 16 GPa |
| Shear modulus | 5.6 GPa |
| Poisson's ratio | 0.44 |
| Thermal expansion | 28.7 × 10⁻⁶ /K |
| Thermal conductivity | 35.3 W/(m·K) |

One metre of sheet moves 1.44 mm over a 50 K swing between a winter night and a summer noon. That is derived here from
the expansion coefficient, and it is why sheet lead is laid in bays fixed at the head only [3].

## Sheet (`milled.lead`)

Rolled lead sheet is made to BS EN 12588, in codes named for the old weight in pounds per square foot [2]:

| Code | Thickness | Mass |
|---|---|---|
| 3 | 1.32 mm | 15.0 kg/m² |
| 4 | 1.80 mm | 20.41 kg/m² |
| 5 | 2.24 mm | 25.40 kg/m² |
| 6 | 2.65 mm | 30.10 kg/m² |
| 7 | 3.15 mm | 35.72 kg/m² |
| 8 | 3.55 mm | 40.26 kg/m² |

It comes in 3 m and 6 m rolls, 150 to 1580 mm wide [2].
- **Code 6** is used for flat roofs, gutters, valleys and box gutters.
- **Code 7** is for high exposure and heritage work [3].

Code 7 is suggested for Albion's porch flat and valley gutters, but that is still a design choice and is recorded as
unverified.

On the roof the plumber lays the sheet in bays, joined across the fall by rolls and down it by drips. Each bay is fixed
at its head only. Design follows BS 6915. New sheet is wiped with patination oil at the end of each day, or rain leaves
uneven white carbonate blooms while the patina forms [3].

## Came (`leaded`)

Came is an H-section: two leaves joined by a heart, with a channel on each side to take the glass. The maker's range is
[4]:
- **leaf widths:** 3-25 mm flat, or 4-20 mm oval;
- **hearts:** narrow, 3-4 mm, or wide, 6-7 mm;
- **alloy:** pure lead or a low-antimony alloy (more antimony makes it stiffer);
- **supply:** 1.8 m lengths.

Historic milled cames run from 3 mm to 20 mm or more [5]. The lead mill was known from the later 15th century, and the
earliest surviving one dates from 1717. It squeezes cast lead through cheeks and toothed wheels, which leave reeding on
the heart: "the further apart the reeding is, the older the cames are" [5]. The as-built atlases lead the glass at 6 mm
[8]. How the glazier works the came is in the `antique-glass` dossier.

## How it ages

**Patina** (`lead.patina`). New lead is bright for hours, then dull grey. While its film forms, rain leaves white
carbonate blooms. Weathered lead is a soft, pale bluish grey, darker where water runs and paler where it dries [1], [3].
Inside the court the cames patinate only to a dull grey.

**Creep and fatigue** (`lead.creep`). Lead creeps under its own weight and the glass's:
- A leaded light slowly sags and bows between its saddle bars, and its cement loosens.
- Sheet held by too many fixings cracks by fatigue.

The "100 years or so" often said of came is a myth [5]. Some leadwork "disintegrated within years", while some is
"fully functioning after more than 800 years".

## What to look for in a photograph

- Sheet: dull, pale bluish grey in bays with rolls and drips, gently undulating, with white blooms at the drips.
- Came from inside: a black line of steady width against the light, with small bright-grey solder lumps at each
  junction.

## The as-built values (D-017)

- **The sheet:** `MI_Albion_Lead`, lin(#7E8285), roughness 0.62, non-metal. The physics agrees, because a patinated
  face is dielectric; the albedo is a design value.
- **The came:** a colour of 0.10, seen against the light [6].

## Sources

1. Wikipedia, "Lead" (infobox; "Chemistry").
2. Ecobat, "Weight Chart: Rolled Lead Sheet to BS EN 12588".
3. The Lead Lads, "Leadwork: the ultimate guide to lead roofing" (BS EN 12588; BS 6915).
4. Heaps, Arnold & Heaps Ltd, "Lead came: decorative lead profiles".
5. S. Strobl, "Lead in Stained Glass Windows", *Historic Churches* (2002).
6. `windows/Scripts/albion_materials.py` (frozen reference).
7. The Albion Plan board ("lead gutters on the arcade lines, a lead roof over the porch") and `AlbionPlan.h`.
8. `windows/Scripts/albion_glass.py` (frozen reference).
