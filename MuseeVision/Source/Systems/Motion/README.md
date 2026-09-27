# Motion

Everything that moves, built as a real machine. Module `MuseeMotion` (Runtime), and its making twin
`MuseeMotionMaking` (Editor). Its stage dresser runs first (order 10), since the bodies it moves are what the other
systems ride on; its piece source runs first too (order 10).

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `MotionSchedule.h` | `FMotionSchedule`: a stage's `Mechanisms.json` (mechanisms, `channels`, `performances`, `interlocks`, `extents`); `Read` keys texts written as plain strings |
| `MotionMechanismSpec.h` | `FMotionMechanismSpec`: key, joint (`prismatic`, `revolute`), body, axis, travel, `time` and `profile` or a `drive`, `stops`, `drawn`, `home`, `start`, `group`, `returns`, control, `open` / `close` sequences, `rides`, guard, clearance, `publishes`, `follows`, the control's words; `PositionOf` a named state |
| `MotionStep.h`, `MotionFollowing.h`, `MotionUseText.h` | a sequence's step (ramp a channel, or move to a stop, `open` or `closed`); following another's channel (the clamped linear map); the control's verb and caption |
| `MotionStop.h` | `FMotionStop`: a named place on the travel, as a share of it (`at` decides; `z` is a note) |
| `MotionDrive.h`, `MotionDriveZone.h` | `FMotionDrive`: speed, acceleration and jerk limits with zones; `Run` gives a move's S-curve under the strictest limits it passes through |
| `MotionSCurve.h` | `Musee::Motion::FSCurve`: the jerk-limited rest-to-rest run, in closed form |
| `MotionRide.h`, `MotionRequirement.h`, `MotionChannelCondition.h` | a ride offered at a stop from a Use anchor, with structured `requires`: mated ports and channel bounds (`atLeast`, `atMost`) |
| `MotionReturn.h` | `FMotionReturn`: a machine left open at a state goes home by itself after a wait with nobody near (the Élan car's 12 s) |
| `MotionInterlock.h` | `FMotionInterlock`: a band of a mechanism's travel or a channel's value entered only while one of its `allows` holds |
| `MotionChannelSpec.h`, `MotionChannel.h` | a point the stage keeps: derived from another (`follows`), or ramped by sequences from its `rest` |
| `MotionPerformanceSpec.h`, `MotionPerformance.h` | a timed piece started (and ended) at a control, publishing 1 while it runs |
| `MotionExtentSpec.h`, `MotionTelescopeStage.h`, `MotionTurns.h`, `MotionExtent.h` | a piece of variable length: a telescope whose stages slide out largest first, or a column built turn by turn; its length follows a channel |
| `MotionProfile.h` | `Musee::Motion::ParseProfile`, `Covered`, `RelativeSpeed`: `linear` and `ease-in-out` (minimum jerk) |
| `MotionMechanism.h` | `UMotionMechanism`: `State`, `IsAt`, `Position`, `StandAtStart`, `MoveTo`, `Usage` / `Use`, `UsageAt` / `UseAt` a Use anchor, `Holds`, `WaitsFor` / `HeldBy`, `SetMatedPorts`, `SetInterlocks`, `Advance`, `OnMoved`; publishes `FMuseeActuation` and its position channel; registers its body in `UMuseeRegistry` |
| `MotionControl.h` | `UMotionControl`: the `IMuseeUsable` sphere at a `Use` anchor (a mechanism's control or its rides', or a performance's), riding on the body when it sits on it |

A lift (Docs/Stages contract, "many stops"): its body is drawn at `drawn` and starts at `start` (its `home` by
default); `{ "move": "<stop>" }` drives it there, under its `drive` when it gives one. A ride is offered at its `use`
anchor while the body rests at its stop, its ports are mated and its channels hold; the first such ride at an anchor is
the control's use. A ride's Use anchor on another stage (`sphere.call`) is Site's to bind across the seam, so the
dresser places no control for it.

The occupancy guard (D-014) sweeps the slice each solid part of the body is about to enter for the visitor's `Body`
channel: `body-in-travel` holds, `body-in-travel-reverse` turns a door back. While it holds, the control says why in the
mechanism's `guardCaption` (the Reserve's "Screen C waits: you are in its way"), or in Motion's own words without one. The stage's interlocks are asked before
every step of travel or of a ramp: the machine waits while one forbids it, and never reverses for one.

A mechanism's texts may be plain strings: `openUse.verb` of `atrium.car.lift` is keyed `atrium.car.lift.open-use.verb`,
a ride's `…ride-<n>.verb`, its guard's `…guard-caption`, a performance's `<performance>.verb`.

`MuseeMotionMaking` draws each extent's parts with Form's lathe: a telescope's stages home (every stage's far end at the
closed length), and one turn of a column at its anchored end. The dresser gives each stage a carriage and turns the
column's turn into an instanced column.

## Later (tier 2, owner Motion)

- `Motion.Comfortable` over the drives' S-curves (ISO 18738-1).
- Loads: `FMuseeActuation.LoadKilograms` from Form's volumes and Matter's densities; counterweights, `Motion.Balanced`.
- Homes that depend on the mated ports (the Élan car homes at the Atrium when `atrium.up` is unmated).
- The state space (`Musee::Motion::Configurations`), travel volumes, housings, `Motion.Clears`, `Motion.Housed`.
- Machine elements as Form drawings (gears, racks, rails); the screw joint; a spiral band's helical seam (Form's
  sweeps along helices).
