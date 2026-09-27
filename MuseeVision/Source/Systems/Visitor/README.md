# Visitor

The person in the museum. Modules `MuseeVisitor` (Runtime) and `MuseeVisitorHeadset` (Runtime, the VR target only,
D-010). Visitor depends on Foundation only; it reaches Time through its commands and things through Foundation's
contracts.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `VisitorBody.h` | `AVisitorBody` (`/Script/MuseeVisitor.VisitorBody`): the body from `FMuseeHumanMeasure`; walk, stride, hop, look; `FindAlong`, `Look`, `UseLooked`; the hour and city keys; `OnControlsReady` for adapters |
| `VisitorEye.h` | `UVisitorEye`: the one unbound post-process volume, camera fields only (EV100 metering, adaptation, ACES tone curve with its shoulder); `ExposureOf(Nits)` |
| `VisitorOverlay.h` | `UVisitorOverlay`: the caption, the use offered, and a message after a key |
| `VisitorAdapter.h` | `IVisitorAdapter`, `RegisterAdapter`, `ActiveAdapter`: the screen or the headset, exactly one active |

Desktop: W A S D walk, the mouse looks, the left button or E uses, Shift strides, Space hops, H another hour, C the
other city, Esc leaves. Headset (Quest Touch): left stick walks, right stick turns 30 degrees, right trigger uses what
the hand points at or steps to the floor it points at, A uses what the eyes rest on, B another hour, X the other
city, Y recentres, left stick click strides.

A usable thing answers the Visibility trace (blocking or overlapping it) within `UseDistance()`: one pace towards it
and an arm's reach, down to the floor.

## Later (tier 2, owner Visitor)

- Sitting at a seat frame (`FMuseeUse::bSeats`) and footfall contacts (`FMuseeContact`) for Sound and Light.
- The step ring shown on the floor in the headset, and bare-hand pinches (OpenXR hand tracking).
- `visitor.comfort`, `visitor.sampling`, the probe `Visitor::MeasureExposure` and the checks (`Visitor.OneEye`, ...).

## Tests

`Musee.Visitor.Body`, `Eye`, `Use`, `Overlay`, `Adapter`.
