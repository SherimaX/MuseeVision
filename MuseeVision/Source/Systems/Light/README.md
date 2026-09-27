# Light

All light in the museum, in real units. Module `MuseeLight` (Runtime), depending on Foundation, Time, Matter and Form.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `LightSky.h` | `ALightSky`: the one sun, Moon, atmosphere, sky light, volumetric clouds and height fog, following Time's moment (sun in lux at the luminous solar constant over the distance squared; Moon by its phase law; cloud cover; rain and snow haze). One is placed in every game world. |
| `LightLuminaire.h` | `FLightLuminaire` and `Musee::Light::FindLuminaire`, `LuminaireKeys`: the catalogue of `Data/Light/Luminaires` as emitters (spot, rect, linear), with why a type stands unlit |
| `LightLuminaireComponent.h` | `ULightLuminaireComponent`: a catalogue type mounted in a stage, at its catalogue output, switched by `ELightOperation` and dimmed by the channel it follows |
| `LightOperation.h` | `ELightOperation`: `Always`, `Dusk` (sun down), `Presence` (`Data/Light/Operation.json`) |

The stage dresser (order 40) reads `Data/Stages/<stage>/Lighting.json` and places luminaires at their `Mount`
anchors, on the moving body named by `body`, aimed by `aim` (else the mount's x axis). A linear type takes the length
laid from the entry's `length` (metres). Findings: an unknown type, mount, body or operation, a line without length,
an opening whose weather isn't one of Time's places, and a type that stands unlit.

The renderer's transport stays in `Config/DefaultEngine.ini`, as it is.

## Later (tier 2, owner Light)

- A `mount` given as a point `[x, y, z]` (the record reader can't read a field that is either a key or a point).
- Emitters for wallwashers, framers and picture lights, from their photometric files.
- `M_Light_Diffuser` driven with each luminaire, the Scheme, Commission, daylight coefficients and
  `PredictIlluminance`; targets and design ratios measured; daylight-linked dimming.
- A cloud material calibrated to cover fraction (the engine's simple volumetric cloud stands in, its coverage set to
  the cloud cover); per-opening cloud for remote weather (D-005).
- `light.profile`, the probes and the checks (`Light.OneSunOneSky`, `Light.LuminairesFromCatalogue`, ...).

## Tests

`Musee.Light.Catalogue`, `Photometry`, `Luminaire`, `Dresser`, `Sky`.
