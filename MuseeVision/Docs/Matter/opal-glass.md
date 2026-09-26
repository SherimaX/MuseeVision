# Opal glass

**Key:** `opal-glass` · **Family:** glass · **Records:**
- the substance: `Data/Matter/Substances/opal-glass.json`;
- its workings: `flashed` (a thin opal skin on a clear sheet) and `blown.cased` (mouth-blown globes);
- its history: the glass family's `glass.soiling`.

Opal glass is the museum's lamp glass. It is used in five places:
- **cased globes on Albion's electroliers:** 17 cm across, six to a fitting [5];
- **the Reserve's pendant globes:** 44 cm in the nave and 34 cm in the aisles [6];
- **the grounds' lamp standards;**
- **the Salon's bay-2 lantern glass:** "Milk glass spreads the light evenly" [7];
- **the Reserve's picture-light lenses.**

Numbers in brackets refer to the sources at the end.

## What it is

Opal glass holds a second phase dispersed through it. It is melted with an opacifier that separates, as the glass
cools, into tiny particles of a different refractive index. Each particle scatters, and together they send light out in
nearly every direction. The glass glows evenly and hides the lamp. The 1911 account lists the opacifiers: "oxide of tin,
oxide of arsenic, phosphate of lime, cryolite or a mixture of felspar and fluorspar" [1].

A solid opal glass would swallow too much light. So the trade carries a thin opal layer on clear glass:
- **Flashed.** Schott's OPALIKA is "a colourless base glass … and a thin white flashed layer", 0.45 mm thick [2].
- **Cased (triplex).** "A thin opal layer is sandwiched in between two outer layers of clear glass" [3].

## What light does in it (measured)

Schott's data sheet for OPALIKA is the measured reference [2]:
- **Luminous transmittance:** 0.32 ± 0.08 (illuminant A). The tolerance is wide because the opal layer varies across
  the sheet.
- **Spectrum:** close to 0.01 at 300 nm, rising to about 0.28 at 375 nm, then almost flat: 0.29 at 400 nm, 0.32 at
  550 nm and 0.345 at 800 nm. The table in the record was digitised by eye from the data sheet's plot. The slight rise
  toward red means that light through opal is faintly warmer than the light that went in.
- **Diffusion:** "almost ideal". The angular indicatrix is nearly a circle, that is, Lambertian. On the axis there is a
  small direct "nose", which depends on wavelength and rises sharply above 800 nm.
- **The clear base:** n_e = 1.525, so each glossy face reflects about 4.3 % at normal incidence (derived here).

**Cased globes** pass more light, because their opal layer can be thinner. A maker gives 50-55 % for conventional opal
glass shades, and "up to 86 %" for its triplex [3]. These are luminaire-efficiency claims and are marked as such.

**The as-built value agrees.** The Salon's bay-2 diffuser used τ = 0.30 [7], inside OPALIKA's measured 0.32 ± 0.08.

## Physical

The base glass's properties are from [2]:

| Property | Value |
|---|---|
| Density | 2600 kg/m³ |
| Admissible bending stress, annealed | 30 MPa |
| Softening point | 719 °C |
| Transformation temperature | 533 °C |
| Thermal expansion | 9.4 × 10⁻⁶ /K |

Flashed sheet is made from 2.40 to 8.00 mm thick, up to 2400 × 1600 mm. It is flat to within 4.8 mm over a 320 mm
square. The sheet is drawn by the Fourcault process [4].

## How it is made

**Flashed** (`flashed`). The opal is laid as a skin on a clear carrier while both are hot. It is drawn as a sheet
(OPALIKA) or blown and flattened like antique flashed glass.

**Cased** (`blown.cased`):
1. The gaffer blows a thin gather of clear glass.
2. He overlays it with opal and, for a triplex globe, with clear again.
3. He blows the globe to shape, with a neck and a rolled lip for the gallery.

Outside, the globe is glossy clear glass and mirrors the room. Inside, the opal diffuses the lamp so that no filament
shows.

## What the as-built build did, and what the physics says (D-017)

The as-built globes are emissive: a 600 + 5400·cos⁷ nit profile, milky clouds of ±9 %, and a glossy skin at roughness
0.04 with F0 0.04 [6]. The lantern globe is an unlit 2500-nit constant.

The physics says a lit globe's luminance is the lamp's flux through the opal:
- about 0.3 of it for flashed opal, and about 0.5 for conventional cased opal;
- spread nearly evenly over the globe, plus the thin direct nose.

Light should derive the glow from each lamp's lumens and this record, and never from a nits constant (principle 5). The
as-built look, brighter toward the lamp and faintly warmer at the rim, is consistent with this.

## What to look for in a photograph

- A lit globe brightest where the view passes nearest the lamp, falling off toward the rim, faintly warmer at the rim.
- The glossy skin mirrors the room and the other lamps.
- Unlit, the globe is a soft milky white. Hand-blown opal is never quite even: faint clouds and a denser neck.

## Unverified

- **The diffuse reflectance of flashed opal.** It is probably near 0.6, by analogy with a white frit of the same
  transmittance, but it was not read.
- **The cased globes' own transmittance,** which depends on the opal layer the maker sets.
- **The modern opacifier chemistry** (fluorides such as NaF or CaF₂).

## Sources

1. H. J. Powell, W. Rosenhain and A. Nesbitt, "Glass", *Encyclopaedia Britannica*, 11th ed. (1911).
2. SCHOTT North America, "OPALIKA" technical data sheet (970-3d/09/04-P).
3. Arlico Lighting, "Lucis: hand made opal glass lighting".
4. Adamer Specialglass, "OPALIKA white flashed opal glass".
5. `windows/Source/MuseeVision/Albion/AlbionLamps.h` (frozen reference).
6. `windows/Scripts/reserve_materials.py` and the as-built Reserve board.
7. `windows/Scripts/salon_interior.py` and the Salon Light board (approved proposal).
