# Craft

How code is written in Musée Vision. Every agent reads this first. The code that makes the museum is part of the
work, so it is held to the same standard as the rooms: clear, physically true, and finished inside where nobody
looks.

## The mindset
- **Build from the real thing.** Before code models stone, light or a mechanism, know how the real one forms, is
  worked and ages. Every number has a source (a record in `Data/`, with its evidence) and a unit.
- **Fix causes, never symptoms.** If something looks or behaves wrong, find the physical truth the model broke and
  fix it in the system that owns that truth, with a check so it can't return. Never tune one place to look right.
- **One home for every idea.** If something already exists, use it. If it is missing below you, report the gap
  (Register) and don't work around it.
- **Interfaces first.** `Public/` is the connector: small, documented, stable. `Private/` is the inside.
- **Nothing is temporary.** No stubs left behind, no "for now" and no commented-out code. Unfinished work is an
  open order, not code.
- **Measure, don't guess.** Performance, light and fit are measured by instruments and checks.

## The wiring
- A module depends only on modules below it (see `Docs/Architecture.md`). Peers never see each other; they meet
  through Foundation's keys, signals, channels, settings and commands.
- Every `Build.cs` derives from `MuseeModuleRules`.
- Each engine capability has one owner (HTTP in Time, Json in Foundation, GeometryCore in FormMaking, and so on).
- No `#include` reaches into another module's `Private/`, and no include path contains `..`.

## Names
- **Systems** are single English nouns: Foundation, Time, Matter, Form, Light, Sound, Motion, Nature, Art, Visitor,
  Site. **Modules** are `Musee<System>`, or `Musee<System>Making` for an editor twin.
- **Reflected types:** the Unreal prefix, then the home word, then a noun: `FFormProfile`, `UMatterSubstance`,
  `FTimeMoment`, `AVisitorBody`. Foundation's home word is `Musee` (`FMuseeKey`).
- **Plain C++** goes in `namespace Musee::<System>`, with no global free functions. One public type per header, with
  the file named after the type without its prefix (`FormProfile.h` holds `FFormProfile`).
- **Units:** C++ interfaces take `Musee::` typed quantities (`Meters`, `Lux`, `Kelvin`…). A UPROPERTY is a plain
  float with the unit in its name and `Units` metadata (`HeightMeters`, `IlluminanceLux`). Engine centimetres
  appear only inside `Musee::ToEngine`.
- **Data keys** are lowercase and dot-scoped, kebab-case within a segment: `travertine`, `rotunda.west`,
  `track-spot.3000k.25deg`. Display text is `FText` and never a key.
- **Checks** are `<System>.<Truth>`, phrased positively (`Form.Sealed`). **Probes** are `<System>::Measure…`,
  `Find…` or `List…`. **Commands** are `musee.<system>.<verb>[-<object>]`. **Logs** are `LogMusee<System>`.
- **Banned in identifiers and asset names**, as whole tokens: Temp, Tmp, Hack, Fix, New, Old, Final, V2, Copy,
  Extra, Misc, Util, Utils, Helper, Manager, Override, Fudge, Tweak, Boost, Compensation, Multiplier, Fill,
  Brightness, Intensity, Strength. `Scale` is banned inside Light, Matter and the Eye. A word from a real trade
  passes: Fillet, Threshold, Temperature.

## Shape
- A function does one thing and fits on a screen. The soft limit is 60 lines and the hard limit 120.
- A file holds one idea. The soft limit is 600 lines and the hard limit 1,000.
- A duplicated block longer than 30 lines is a defect: promote it to its owner.
- No magic numbers. A physical constant is a named `constexpr` with its unit and its source in a comment; a design
  number lives in a record in `Data/`.

## Comments
- Say which physical truth the code models and where its numbers come from. Don't narrate the code.
- A header's comment says what the type *is* in the museum, in plain words, before any detail.
- Write in plain English sentences. Quote the source of a value: a standard, a table, or a measured specimen.

## Tests and checks
- Every public connector has an automation test under `Private/Tests` (`Musee.<System>.<Case>`).
- Every physical truth a system owns has a check, registered with Foundation's check framework as pure maths over
  inputs that Studio supplies.

## Git
- A work order runs on its own branch, `work/<order-id>`, in its own worktree slot. Never run git in the owner's
  main checkout (`C:\Users\anan9\Desktop\Projects\Musee Vision`).
- Stage only the paths the order owns: never `git add -A`, `git add .`, `git stash`, `git reset --hard` or
  `git clean`.
- Commit messages start with the owner of the change: `Form: shared stations for closed loops`. End them with the
  attribution line given to the session.
