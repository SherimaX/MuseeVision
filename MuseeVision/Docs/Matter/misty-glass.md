# Misty glass: etched and fritted

**Records:**
- the workings `etched.glass` and `fritted`, in `Data/Matter/Workings/`;
- both lie over the glass group's `low-iron-glass`, on its `annealed` sheet.

This is a working dossier, like `gilt.md`. The two workings are the museum's "misty glass", and the natural home for
them, the `low-iron-glass` dossier, belongs to another group. That dossier should gain a pointer to this one.

The Élan is the museum's future, and misty glass is its skin:
- **the Atrium's drum:** misty glass from the 6 m travertine base to the ring at 22 m, 14 m in radius;
- **the Sphere's underside,** which is the Atrium's ceiling: "glass on a triangulated grid of pearl-white steel";
- **the Hall of Light:** the misty glass above its clear walls, under the pearl vault;
- **the drum's exterior layer,** behind a clear outer pane;
- **the glass car's floor,** "lightly frosted", which the visitor stands on.

Numbers in brackets refer to the sources at the end.

## Two ways to make glass misty

**Acid-etched** (`etched.glass`). One face of the float is exposed to a hydrofluoric-acid bath, which dissolves it
unevenly and leaves a field of microscopic facets and pits [1], [2]. What that does:
- **The light still passes, but scattered.** A Velour etch on 6 mm clear passes 0.91 of the light, with a haze of
  0.89; an Opaque etch has a haze of 0.90 [1], [3]. Haze is the share of the transmitted light that deviates by more
  than 2.5°.
- **The strength is unchanged.** Etched glass keeps its substrate's strength [2].
- **It can still be worked.** It is etched flat, then toughened, laminated or put into insulating units.

The panes must match within ΔE* 2. The typical defect is "clouds" of uneven haze [2]. AGC's patent examples, for one
faceted etch, give the scale of an etched face [4]:

| Property | Value | Conditions |
|---|---|---|
| Ra | 1.5-4 µm | 12 mm evaluation length |
| Ra, fine texture | 0.08-0.4 µm | 200 µm evaluation length |
| RSm, mean spacing | 40-250 µm | |
| Light transmittance | 0.883-0.887 | 4 mm glass |
| Haze | 0.76-0.90 | 4 mm glass |
| Gloss, 60° | 7-9 GU | |

**Fired ceramic frit** (`fritted`). A paste of glass-and-ceramic frit is laid on the air side of the float by screen
printing, digital printing, roller or curtain coating. It is dried at 170-190 °C, and fused to the glass in the heat of
toughening [5]. Fraunhofer ISE measured a full white frit (RAL 9010) on 6 mm low-iron glass [5]:

| Property | Value |
|---|---|
| Luminous transmittance | 0.320 |
| Luminous reflectance, glass side | 0.572 |
| Luminous reflectance, frit side | 0.623 |
| Absorptance | about 0.11 (derived) |

A full grey frit passes 0.035, and a black one nothing. The measurement needs a large integrating sphere: a 220 mm
sphere under-read the white pane at 0.218. The Great Court's roof is fritted with dots over half its surface (the steel
record).

**Sandblasting,** rejected. Sandblasting soda-lime glass raises Ra from 0.035 to 2.27 µm [6]. Its specular
transmittance falls from 0.916 to 0.13, and its scattered transmittance rises to 0.81. The surface is visibly damaged
above about Ra 1.5 µm. It is not the museum's finish. (That it also leaves strength-reducing microcracks was
seen only in a search summary, and is recorded as unverified.)

## What the as-built build did, and what the physics says (D-017)

The as-built `M_MistyGlass` is self-lit [7]:
- about 450 nits at noon, scaled by a daylight factor;
- tint #F1F4F2;
- a faint cloudiness 6 m across;
- a seam every 1.6 m.

Before that it was a dot frit that "read as a grey perforated sheet".

Physically, misty glass passes the sky's light and scatters it. Its luminance follows the real sky behind it, and Light
should compute it from the sky and these records, never from a nits constant (principle 5). The two workings look quite
different:
- **An etched pane** passes about 0.9 of the light into a wide cone. It glows as a bright satin, and the sun shows as a
  soft blur.
- **A white frit** passes about 0.3 almost evenly. It reads as a milky white: "a soft pearl, not a lamp".

The as-built intent (a soft pearl, about 450 nits against a sky of several thousand) is much closer to the frit, or to
etched glass laminated with a white interlayer, than to a satin etch. **This is the Architect's decision, and it is
listed as unverified.**

**The car's floor** is walked on. There the etch is right: a lightly etched walking face, laminated, reads as "a floor
you can see down through". The as-built used opacity 0.3, roughness 0.3 and diffuse 0.08 [8].

## Weather and wear

**Wet glass clears.** A water film fills the etched texture with a liquid whose index is close to glass's. Each facet's
reflectance then falls from 0.043 to 0.0044, derived from n = 1.523 for the glass and 1.333 for water. So a wet etched
face goes partly clear. The etched face of the drum and the roof should face the dry side, inside.

**Etched faces mark.** They show fingerprints and grease where they are touched: the car's rail, the floor underfoot.
This is trade knowledge not read from a source, and it is marked unverified.

**Soiling** follows the existing `glass.soiling`.

## Sources

1. Walker Glass (Gardner Walker), "Acid-etched glass" (Velour).
2. AGC Glass Europe, "Technical Data Sheet Matelux" (08/2014).
3. Walker Glass (Gardner Walker), "Acid-etched glass … so much more than what you thought".
4. AGC Glass Europe, "Frosted glass sheet", WO 2014/012809 A1.
5. H. R. Wilson and M. Elstner, "Spot Landing: Determining the Light and Solar Properties of Fritted and Coated Glass", *Challenging Glass 6* (2018), doi:10.7480/cgc.6.2133.
6. S. Benterki, C. Bousbaa, N. Laouar and A. Faci, "Characterization of the sandblasted glass degradation by light scattering", *Optik* (2018) (abstract).
7. `windows/Scripts/setup_project.py`, `make_misty()` (frozen reference).
8. `windows/Source/MuseeVision/Elan/ElanCar.cpp` (frozen reference).
