# Motion

Everything that moves, built as a real machine. Module `MuseeMotion` (Runtime). Its stage dresser runs first
(order 10), since the bodies it moves are what the other systems ride on.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `MotionSchedule.h` | `FMotionSchedule`: a stage's `Mechanisms.json` |
| `MotionMechanismSpec.h` | `FMotionMechanismSpec`: key, joint (`prismatic`, `revolute`), body, axis, travel, `time` and `profile` or a `drive`, `stops`, `drawn`, `home`, control, `open` / `close` sequences, `rides`, guard, clearance, `publishes`, `follows`, the control's words; `PositionOf` a named state |
| `MotionStep.h`, `MotionFollowing.h`, `MotionUseText.h` | a sequence's step (ramp a channel, or move to a stop, `open` or `closed`); following another's channel; the control's verb and caption |
| `MotionStop.h` | `FMotionStop`: a named place on the travel, as a share of it (`at` decides; `z` is a note) |
| `MotionDrive.h`, `MotionDriveZone.h` | `FMotionDrive`: speed, acceleration and jerk limits with zones; `Run` gives a move's S-curve under the strictest limits it passes through |
| `MotionSCurve.h` | `Musee::Motion::FSCurve`: the jerk-limited rest-to-rest run, in closed form |
| `MotionRide.h`, `MotionRequirement.h`, `MotionChannelCondition.h` | a ride offered at a stop from a Use anchor, with structured `requires`: mated ports and channel bounds (`atLeast`, `atMost`) |
| `MotionProfile.h` | `Musee::Motion::ParseProfile`, `Covered`, `RelativeSpeed`: `linear` and `ease-in-out` (minimum jerk) |
| `MotionMechanism.h` | `UMotionMechanism`: `State`, `IsAt`, `Position`, `StandAtHome`, `MoveTo`, `Usage` / `Use`, `UsageAt` / `UseAt` a Use anchor, `Holds`, `SetMatedPorts`, `Advance`, `OnMoved`; publishes `FMuseeActuation` and its position channel; registers its body in `UMuseeRegistry` |
| `MotionControl.h` | `UMotionControl`: the `IMuseeUsable` sphere at a `Use` anchor (the mechanism's control or its rides'), riding on the body when it sits on it |

A lift (Docs/Stages contract, "many stops"): its body is drawn at `drawn` and starts at `home`; `{ "move": "<stop>" }`
drives it there, under its `drive` when it gives one. A ride is offered at its `use` anchor while the body rests at its
stop, its ports are mated and its channels hold; the first such ride at an anchor is the control's use. A ride's Use
anchor on another stage (`sphere.call`) is Site's to bind across the seam, so the dresser places no control for it.

The occupancy guard (D-014) sweeps the slice each solid part of the body is about to enter for the visitor's `Body`
channel: `body-in-travel` holds, `body-in-travel-reverse` turns a door back.

## Later (tier 2, owner Motion)

- `Motion.Comfortable` over the drives' S-curves (ISO 18738-1).
- Loads: `FMuseeActuation.LoadKilograms` from Form's volumes and Matter's densities; counterweights, `Motion.Balanced`.
- Interlocks by key (`UMotionInterlock`), such as the pond's last 0.10 m waiting for the rails.
- The state space (`Musee::Motion::Configurations`), travel volumes, housings, `Motion.Clears`, `Motion.Housed`.
- Machine elements as Form drawings (gears, racks, rails); the screw joint.
