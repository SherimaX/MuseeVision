# Motion

Everything that moves, built as a real machine. Module `MuseeMotion` (Runtime). Its stage dresser runs first
(order 10), since the bodies it moves are what the other systems ride on.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `MotionSchedule.h` | `FMotionSchedule`: a stage's `Mechanisms.json` |
| `MotionMechanismSpec.h` | `FMotionMechanismSpec`: key, joint (`prismatic`, `revolute`), body, axis, travel, time, profile, control, `open` / `close` sequences, guard, clearance, `publishes`, `follows`, the control's words |
| `MotionStep.h`, `MotionFollowing.h`, `MotionUseText.h` | a sequence's step (ramp a channel, or move to `open` / `closed`); following another's channel; the control's verb and caption |
| `MotionProfile.h` | `Musee::Motion::ParseProfile`, `Covered`, `RelativeSpeed`: `linear` and `ease-in-out` (minimum jerk) |
| `MotionMechanism.h` | `UMotionMechanism`: `State`, `Position`, `MoveTo`, `Usage`, `Use`, `Advance`, `OnMoved`; publishes `FMuseeActuation` and its position channel; registers its body in `UMuseeRegistry` |
| `MotionControl.h` | `UMotionControl`: the `IMuseeUsable` sphere at a `Use` anchor, riding on the body when it sits on it |

The occupancy guard (D-014) sweeps the slice each solid part of the body is about to enter for the visitor's `Body`
channel: `body-in-travel` holds, `body-in-travel-reverse` turns a door back.

## Later (tier 2, owner Motion)

- Drives with explicit speed, acceleration and jerk limits (the S-curve of the spec) in place of the time-scaled
  minimum-jerk move; `Motion.Comfortable`.
- Loads: `FMuseeActuation.LoadKilograms` from Form's volumes and Matter's densities; counterweights, `Motion.Balanced`.
- Interlocks by key (`UMotionInterlock`), such as the pond's last 0.10 m waiting for the rails.
- The state space (`Musee::Motion::Configurations`), travel volumes, housings, `Motion.Clears`, `Motion.Housed`.
- Machine elements as Form drawings (gears, racks, rails); the screw joint.
