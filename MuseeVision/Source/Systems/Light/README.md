# Light

All light in the museum, in real units. Module `MuseeLight` (Runtime), depending on Foundation, Time, Matter and Form.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `LightSky.h` | `ALightSky`: the one sun, Moon, atmosphere, sky light, volumetric clouds and height fog, following Time's moment (sun in lux at the luminous solar constant over the distance squared; Moon by its phase law; cloud cover; rain and snow haze). One is placed in every game world. |
| `LightLuminaire.h` | `FLightLuminaire` and `Musee::Light::FindLuminaire`, `LuminaireKeys`: the catalogue of `Data/Light/Luminaires` as emitters (spot, rect, linear, point, image, and a maker's measured profile), areal types by the square metre, with why a type stands unlit; `ProfilePackage` and `FindProfile`, the light profile a wallwasher is lit by |
| `LightLuminaireComponent.h` | `ULightLuminaireComponent`: a catalogue type mounted in a stage, at its catalogue output over the length or face it is laid to, switched by `ELightOperation`, dimmed by the channel it follows; an image panel by the picture it is shown (`ShowImage`); a wallwasher by its maker's photometric file, through the light profile Studio imports from it (`LightProfile`) |
| `LightOperation.h` | `ELightOperation`: `Always`, `Dusk` (sun down), `Presence` (`Data/Light/Operation.json`) |

The stage dresser (order 40) reads `Data/Stages/<stage>/Lighting.json` and places each luminaire either on its `mount`
(a Mount anchor's key) or at its `point` (`[x, y, z]` in the stage's frame), never both, on the moving body named by
`body`. Its light leaves towards `aim`, the point `[x, y, z]` in the stage's frame it is aimed at; a luminaire on a
mount without an aim shines along the mount's x axis, and one at a point needs its aim. A linear type lies across the
aim, level where it can (along the stage's x axis when aimed straight up or down), and takes the length laid from the
entry's `length` (metres). An areal type (a luminous ceiling, edge-lit panes, an image panel) is laid over the face its
entry gives: an `extent` [width, height] in metres, or a `zone` of a round field, `{ "r": [inner, outer], "bearing":
[from, to] }` about the stage's vertical axis, bearings clockwise from north; a zone's face is the rectangle of its own
area, as wide as its middle arc (or its outer chord), its height running out from the axis. A luminaire set in a piece
names it as `host`, and the surface it lights as `surface`; they are kept as the tags `musee.host:<key>` and
`musee.lights:<key>`. Targets may hold only `when` a mechanism stands at a stop, and a target's `lux` and a design's
`ratio` may leave one end open (`[50, null]`). Findings: an unknown type, mount, body or operation, a mount and a point
together or neither, a point without its aim, a line without length, an areal type without its face (or with both
faces, or outside its zone), a face on a type that isn't areal, an opening whose weather isn't one of Time's places,
and a type that stands unlit (a wallwasher whose light profile Studio hasn't made among them).

A wallwasher (`track-wallwasher`) is lit by its maker's photometric file, kept byte for byte in `Data/Light/Photometry`:
Light reads the LM-63 file for the flux its candela table integrates to, and lights the luminaire as a point source
carrying the light profile Studio imports from the same file, its intensity the profile's own (`bUseIESBrightness`, in
candelas) and dimmed through `IESBrightnessScale`. The file's Type C frame is turned into the luminaire's so its throw, the
direction of its greatest intensity, lies on x, towards the aim: its nadir below, in the luminaire's vertical plane, and the
throw's plane on +z, towards the wall (`Docs/Light/Luminaires.md`, "Photometric files").

The renderer's transport stays in `Config/DefaultEngine.ini`, as it is.

## Later (tier 2, owner Light)

- Emitters for framers and picture lights, from their photometric files.
- `M_Light_Diffuser` driven with each luminaire, the Scheme, Commission, daylight coefficients and
  `PredictIlluminance`; targets and design ratios measured; daylight-linked dimming.
- A cloud material calibrated to cover fraction (the engine's simple volumetric cloud stands in, its coverage set to
  the cloud cover); per-opening cloud for remote weather (D-005).
- `light.profile`, the probes and the checks (`Light.OneSunOneSky`, `Light.LuminairesFromCatalogue`, ...).

## Tests

`Musee.Light.Catalogue`, `Photometry`, `PhotometricFile` (the 362G's file integrates to its rated 999 lm), `Luminaire`, `Dresser`, `Sky`, and `EveryScheduleReads` (every stage's
Lighting.json reads through Light's reader, and every type it names is in the catalogue).
