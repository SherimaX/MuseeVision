# Luminaires

Every light in the museum is a real product. A luminaire record in `Data/Light/Luminaires/` describes one
configuration of one product, as its maker publishes it: the luminous flux, the beam, the colour of the light, the
power, how it dims, how it mounts, and the size of its housing. Light reads these records and nothing else when it
places, lights and dims a luminaire. A room never states how much light a luminaire gives.

Two companion files set how much light is wanted:

- `Data/Light/UseTargets.json` holds the targets for the architecture, by use class (circulation, stairs, entrance
  hall, orientation, night), from the lighting standards.
- `Data/Art/Care.json` holds the conservation limits for works, by medium (works on paper, textiles and silk, oil and
  tempera, stone and bronze, ceramic and glass). Art owns it and sends each work's limit to Light as a target.

## The keys

A key names what a luminaire does, then the colour of its light, then what tells it apart from its siblings:
`track-spot.3000k.12deg` is a spotlight for track, warm white at 3000 K, with a 12° beam. The kinds are:

| Kind | What it is |
|---|---|
| `track-spot` | A spotlight on a track, aimed at one work or one surface. |
| `track-wallwasher` | A luminaire on a track that spreads an even wash down a whole wall. |
| `track-framing` | A projector whose shutters cut the beam to the edges of a picture. |
| `picture-light` | A hood on an arm fixed to the frame or the wall above a picture. |
| `linear-cove` | A continuous line of LEDs laid in a cove to light a vault or ceiling indirectly. |
| `linear-slot` | A continuous light line recessed in a ceiling slot. |
| `downlight` | A luminaire recessed in the ceiling that lights the floor below it. |

## The catalogue

Sixteen luminaires from ERCO, iGuzzini, Zumtobel, Edison Price Lighting and Hogarth Lighting. Every track spotlight
and wallwasher has a colour rendering index of 95 or more, at both 3000 K and 4000 K. The flux is the luminaire's own
output (the light that leaves the housing), not the LED's.

| Key | Product | Code | Flux | Beam | CCT | CRI | Power | Control class | Evidence |
|---|---|---|---|---|---|---|---|---|---|
| `track-spot.3000k.12deg` | iGuzzini Palco ø 102 mm | QG54 | 1876.8 lm | 12° | 3000 K | 97 | 28.5 W | gallery | complete |
| `track-spot.3000k.20deg` | iGuzzini Palco ø 102 mm | QG55 | 1871.19 lm | 20° | 3000 K | 97 | 28.5 W | gallery | complete |
| `track-spot.3000k.29deg` | ERCO Optec New, size L | A9001745 | 2541 lm | 28.6° | 3000 K | 97 | 38.0 W | gallery | complete |
| `track-spot.3000k.38deg` | iGuzzini Palco ø 102 mm | QG56 | 1814.84 lm | 38° | 3000 K | 97 | 28.5 W | gallery | complete |
| `track-spot.4000k.12deg` | iGuzzini Palco ø 102 mm | QG50 | 1987.2 lm | 12° | 4000 K | 97 | 28.5 W | gallery | complete |
| `track-spot.4000k.20deg` | iGuzzini Palco ø 102 mm | QG51 | 1981.26 lm | 20° | 4000 K | 97 | 28.5 W | gallery | complete |
| `track-spot.4000k.38deg` | iGuzzini Palco ø 102 mm | QG52 | 1921.59 lm | 38° | 4000 K | 97 | 28.5 W | gallery | complete |
| `track-wallwasher.3000k` | iGuzzini Sipario LV ø 73 mm | 362G | 999 lm | wall wash | 3000 K | 97 | 16.1 W | gallery | complete |
| `track-wallwasher.4000k` | Edison Price Stacklite | STK-CZE-2000L-40K-XIA-95+-DIMTR-120V-WH | 2000 lm, nominal | wall wash | 4000 K | ≥ 95 | not published | gallery | incomplete |
| `track-framing.3000k.small` | iGuzzini Palco framer ø 99 mm | Q745 | 552 lm | shutters | 3000 K | 97 | 24.2 W | gallery | complete |
| `track-framing.3000k.medium` | iGuzzini Palco framer ø 115 mm | P252 | 896 lm | shutters | 3000 K | 97 | 35 W | gallery | complete |
| `picture-light.2700k` | Hogarth Classic Picture Light, antique bronze | #100 | not published | shuttered hood | 2700 K | 97 | not published | gallery | incomplete |
| `linear-cove.3000k` | iGuzzini Underscore No-Dot strip | RE18 | 1420 lm/m | general | 3000 K | ≥ 90 | 16 W/m | circulation | complete |
| `linear-slot.3000k` | Zumtobel SLOTLIGHT infinity II, recessed | article not public | LED package 1600 lm/m | opal cover | 3000 K | > 90 | not published | circulation | incomplete |
| `downlight.3000k.31deg` | iGuzzini Laser fixed round, Super Comfort | RA18 | 1944.8 lm | 31° | 3000 K | 92 | 17 W | circulation | complete |
| `downlight.4000k.31deg` | iGuzzini Laser fixed round, Super Comfort | RA24 | 2068 lm | 31° | 4000 K | 92 | 17 W | circulation | complete |

Each record's `sources` list names the page or data sheet every group of numbers came from, and the day it was read.

## What a record holds

The unit of every number is in its field name, the way the museum writes units everywhere.

- `luminousFluxLumens` is the luminaire's flux. `fluxBasis` says what kind of number it is: `luminaire` (measured
  output of the whole luminaire), `per-metre` (a linear luminaire), or `nominal-package` (the lumen class in the
  maker's ordering code). `ledFluxLumens` is the bare LED's flux, kept only to show the optic's losses.
- `beamAngleDegrees` is the full angle at which the intensity falls to half its peak. Framers and wallwashers have no
  single beam angle: their light is shaped by shutters or an asymmetric lens, described in `beamName`.
- `peakCandela` is the luminous intensity on the beam's axis. It is the number that decides the illuminance a spot
  lays on a work: for a surface at distance *d* metres whose normal makes angle *θ* with the beam, the illuminance is
  `peakCandela × cos θ / d²` lux.
- `cctKelvin`, `cri`, `tm30Rf`, `tm30Rg` and `macAdamSteps` describe the colour of the light and how faithfully it
  renders colour. `criBasis` marks a published minimum (`≥`) or a published bound (`> 90`).
- `systemPowerWatts`, `control` and `mounting` describe the electrical side: DALI or an on-board dimmer, and which
  track or ceiling the luminaire fits.
- `housing` gives the size, weight and materials, so Form can draw the luminaire as a real object in the room.
- `photometricFile` links the maker's IES or LDT file where one is public. The files are not in the repository; the
  record names where each one is published.
- `controlClass` is the class Light's Operation uses to switch and dim the luminaire through the day: `gallery` for
  light on works, `circulation` for light on the architecture.
- `evidenceState` is `complete` when every number the Scheme needs (flux, beam or distribution, colour, power) is
  published. An `incomplete` record is a real product whose maker leaves one of these unpublished. It may stand in a
  room as an object, with its real housing and finish, but it is not lit. Its `unverified` list says what is missing.

## How a room gets its light

A room never writes a lumen, a candela or a dimming level. A lumen in a room would be a knob: turned until the room
looked right, it would hide whatever physical truth had gone wrong. Instead a room says what the light is for and
where a luminaire can hang, and Light works out the rest by the rules a lighting designer uses.

**What the room declares, in its own frame and in metres:**

1. **Targets.** Each walked or seen surface names a use class from `UseTargets.json`: the Rotunda's floor around the
   gilt sun is an `entrance-hall`, its spiral stair is `stairs`, a passage is `circulation`. Each hung work brings its
   own target: Art reads the work's medium, finds its class in `Care.json`, and sends Light the limit.
2. **Mounts.** The places a luminaire can go: a track run under a ceiling, a cove along a vault's springing, a slot
   in a ceiling, a grid of ceiling positions, a picture rail. A mount fixes where light can come from, never how much.
3. **The type, where the design names one.** When the approved design calls for a particular luminaire (the Salon's
   bronze picture lights, say), the room names its catalogue key. Otherwise Light chooses.

**What Light's Scheme chooses, by rule:**

1. **The kind** from the mount: a track takes spotlights, wallwashers and framers; a cove takes `linear-cove`; a
   ceiling grid takes downlights.
2. **The beam** from the size of the target seen from the mount. A work is lit from the track at the museum angle,
   about 30° from the vertical, which keeps the frame's shadow short and throws the reflection of the light on
   varnish and glass down and away from the visitor's eye (Light.NoGlare tests that mirror path for every work).
   The beam must cover the work and not much more.
3. **The count** from the target's size and uniformity: one spot for a small painting, a row of wallwashers for a
   wall, downlights spaced to hold the floor's uniformity.
4. **The colour** from the room's design and the work's care class (below).

**What commissioning sets.** The Scheme places luminaires at full output. Studio then renders the stage with the path
tracer at the reference moments, and Light's commissioning reads the illuminance on every target with the calibrated
grey card, the way a lighting designer walks a gallery with a lux meter. It sets each luminaire's dimming level so
the work receives its care limit and the architecture its use target. Operation then switches and dims by control
class through the day and night. If the level a target needs lies below what a luminaire can dim to, the luminaire
is the wrong type for the mount, and the Scheme picks a smaller or wider one; the fix is never a lower number in the
room.

### A worked example

A painting's centre hangs at eye height, 1.60 m, on a wall whose track runs 4.50 m above the floor. The light drops
2.90 m to the work. Aimed 30° from the vertical, the track must lie 1.67 m out from the wall, and the light travels
3.35 m to the painting (*d*² = 11.21 m²). The beam meets the wall at 60° from its normal, so the illuminance at the
painting's centre is `peakCandela × cos 60° / 11.21` lux. At full output:

| Luminaire | Peak | On the painting | Dimming for oil at 200 lx | Dimming for paper at 50 lx |
|---|---|---|---|---|
| `track-spot.3000k.12deg` (QG54) | 20 026 cd | 893 lx | 22 % | 5.6 % |
| `track-spot.3000k.20deg` (QG55) | 10 963 cd | 489 lx | 41 % | 10 % |
| `track-spot.3000k.29deg` (ERCO A9001745) | 9423 cd | 420 lx | 48 % | 12 % |
| `track-spot.3000k.38deg` (QG56) | 5482 cd | 244 lx | 82 % | 20 % |

The beam's size settles which one it is. The 12° spot lights a patch 0.70 m wide, from 0.74 m to 2.20 m up the wall:
right for a painting of about that size. The 20° spot lights a patch 1.18 m wide from the floor to 2.51 m. The same
luminaire then serves an oil painting or a watercolour; only the commissioned dimming level differs, and that level
comes from measurement against `Care.json`, never from the room.

## Colour temperature: 3000 K and 4000 K

Both colours are in the catalogue at the same colour rendering, so the choice is a design decision, made once per
room, and not a compromise on quality.

- The owner found 3000 K too warm in the earlier build and chose high-CRI 4000 K for the works, as sitting between the
  laylights' daylight and a warm lamp (the frozen build, `windows/Scripts/gallery_lights.py`).
- For light-sensitive works lit at 50 lx, the Canadian Conservation Institute notes that viewers at low light levels
  prefer warmer light, like the 2800 K of an incandescent lamp or the 3000 K of a halogen one. `Care.json` records
  this as a preference, not a damage limit.
- At the same illuminance, a spectrum richer in short wavelengths carries more potential for damage (Zumtobel, citing
  a TU Darmstadt test report). The annual dose limits in `Care.json` hold whichever colour a room chooses.

## Care limits and use targets

`Care.json` gives, for each class of medium, the most lux a work may receive, a recommended level, the annual dose in
lux-hours, the ultraviolet limit in microwatts per lumen, and the colour temperature guidance, each with its source.
A work of mixed media takes the class of its most sensitive part: Degas's dancer of wax, silk, satin and hair is lit
as silk. A bronze with a lacquer or wax coating is lit as its coating. Art.WithinCare checks the lux at every
reference moment and the dose over the year.

`UseTargets.json` gives, for each use class, the maintained illuminance on the reference plane, the uniformity, the
glare limit and the colour rendering floor, with the rows of the standards they come from. Two classes, night
circulation and orientation, have no museum row in the standards and are taken from the nearest row that does exist
(corridors at night and night observation lighting in health-care buildings); each is marked `analogue`. The
`escape-route` class is the floor under every other: no walked route falls below it in any configuration.

## Evidence

- Every record names its sources. A value that could not be confirmed is listed in the record's `unverified` array,
  and in `Care.json` a value taken from a standard that was not read directly carries `confirmed: false`.
- The CIE 157:2004 limits in `Care.json` (200 lx and 600 000 lx·h a year for oil and tempera, 50 lx and 150 000 lx·h
  for paper and most textiles, 50 lx and 15 000 lx·h for silk and fugitive colours) are the report's widely
  reproduced table; the report itself was not read. The medium and low annual values agree with the Canadian
  Conservation Institute's reference year of 3000 lit hours at 50 lx and 200 lx.
- The use targets are the EN 12464-1:2011 and EN 12464-2:2007 rows as reproduced in Zumtobel's Lighting Handbook. The
  2021 edition of EN 12464-1 adds a modified illuminance and wall, ceiling and cylindrical values to every row; the
  wall and ceiling minimums recorded here come from TRILUX's summary of the standard.
- IES RP-30-17 and CIBSE LG8 were not publicly readable, so neither supplies a number here.

## Sources

- iGuzzini product pages, read 2026-09-26: [QG50](https://www.iguzzini.com/en/qg50/),
  [QG51](https://www.iguzzini.com/en/qg51/), [QG52](https://www.iguzzini.com/en/qg52/),
  [QG54](https://www.iguzzini.com/en/qg54/), [QG55](https://www.iguzzini.com/en/qg55/),
  [QG56](https://www.iguzzini.com/en/qg56/), [362G](https://www.iguzzini.com/en/362g/),
  [Q745](https://www.iguzzini.com/en/q745/), [P252](https://www.iguzzini.com/en/p252/),
  [RE18](https://www.iguzzini.com/en/re18/), [RA18](https://www.iguzzini.com/en/ra18/),
  [RA24](https://www.iguzzini.com/en/ra24/).
- ERCO, [data sheet A9001745](https://download-service.erco.com/download/A9001745/en/?types=15&formattedId=&productType=article),
  edition 28.04.2026; [IES downloads](https://download.erco.com/en/planning_luminaire/ies/);
  [Museum lighting with 50lx](https://www.erco.com/en/designing-with-light/mediaassetpool/lighting-technology/museum-lighting-with-50lx-6481/).
- Edison Price Lighting, [Stacklite STK-CZE spec sheet](https://epl.com/media/spec/stk-cze.pdf) and
  [product page](https://epl.com/product/stk-cze/).
- Hogarth Lighting, [Classic Picture Light](https://hogarthlighting.com/our_picture_lights/classic_picture_light).
- Zumtobel, [SLOTLIGHT infinity II brochure](https://www.zumtobel.com/PDB/Ressource/teaser/en/com/SLOTLIGHT_infinity_II.pdf),
  [ARCOS brochure](https://www.zumtobel.com/PDB/Ressource/teaser/en/com/ARCOS.pdf),
  [The Lighting Handbook, 6th edition, 2018](https://www.zumtobel.com/PDB/teaser/EN/lichthandbuch.pdf).
- Stefan Michalski, [Agent of deterioration: Light, ultraviolet and infrared](https://www.canada.ca/en/conservation-institute/services/agents-deterioration/light.html),
  Canadian Conservation Institute, 2018.
- Garry Thomson, *The Museum Environment*, 2nd ed., 1986, p. 23, as cited in
  [Agents of deterioration](https://en.wikipedia.org/wiki/Agents_of_deterioration).
- CIE 157:2004, [Control of damage to museum objects by optical radiation](https://cie.co.at/publications/control-damage-museum-objects-optical-radiation).
- TRILUX, [Lighting of public areas](https://www.trilux.com/en/lighting-practice/indoor-lighting/specific-lighting-requirements/lighting-of-public-areas/corridors-entrance-halls-cloakrooms/).
