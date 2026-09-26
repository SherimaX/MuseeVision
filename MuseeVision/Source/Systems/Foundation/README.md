# Foundation

The standards every other system is built to: units, frames, identity, poses, the human measure, records, the few
contracts and signals that cross layers, settings, commands, and the diagnostics framework. Nothing more. Module
`MuseeFoundation` (Runtime), layer L0; its Build.cs also defines `MuseeModuleRules`, the base of every Build.cs.

## Owns

- **Typed quantities** (`Musee::Meters`, `Kilograms`, `Seconds`, `Degrees`, `Radians`, `MetersPerSecond`, `Lux`,
  `Lumens`, `Candela`, `Nits`, `Kelvin`, `EV100`, `Decibels`), with only the arithmetic their physics allows, and the one
  conversion to engine units, `Musee::ToEngine` / `Musee::FromEngine`. Centimetres appear nowhere else.
- **Frames**: `FMuseeFrame`, a rigid local frame in metres; a stage's frame has x east, y south, z up, and plan north is
  true north. These are the engine's axes, so a frame reaches the engine by a change of unit only.
- **Identity**: `FMuseeKey` (lowercase, dot-scoped, kebab within a segment), `Musee::Seed` (the one seeded stream of a
  key; there is no unseeded randomness), and `UMuseeRegistry`, which resolves a key to the scene component carrying it.
- **Poses**: `FMuseePose` and `FMuseeConfiguration`, the one currency for "in every mechanism state".
- **The human measure**: `FMuseeHumanMeasure`, the body's constants with their sources.
- **Records**: `Musee::ReadRecords<T>` / `ReadRecord<T>`, the one reader of `Data/<System>/*.json` into typed structs;
  the dependency set every read joins (`FMuseeDependencySet`); `FMuseeText`. Foundation is the only user of Json.
- **Crossing contracts**: `IMuseeUsable` with `FMuseeUse`; `IMuseeCaptioned` with `FMuseeCaption`.
- **Signals and channels**: `UMuseeSignals` with `FMuseeContact`, `FMuseeActuation` and `FMuseeChannel`.
- **Settings and commands**: `FMuseeSetting` (one owner, one persisted copy, its command made for it) and
  `FMuseeCommand` (one registry for the console, the keyboard, the headset and Studio).
- **Diagnostics**: `FMuseeCheck` and `MUSEE_CHECK` with `EMuseeCheckScope`; `FMuseeFinding` and `FMuseeMeasurement`;
  `IMuseeProbe` and `MUSEE_PROBE`; the log and stat-group macros (`MuseeLog.h`); the defect record, `FMuseeDefect`.
- **Collision names**: `Musee::Collision::Solid`, `Walkable`, `Body`, `Clearance`, and their channels and profiles in
  `Config/DefaultEngine.ini` (section `/Script/Engine.CollisionProfile`).
- `MuseeModuleRules` and the admission rule: Foundation grows only by adding a standard, reviewed as a new system is.

## Does not own

- Any geometry algorithm or drawing (Form).
- The clock, the sky or the weather (Time).
- Any rendering, material, light or sound.
- Any knowledge of a particular stage or work.
- Running checks (Studio) or showing measurements (Instruments).
- Any number that belongs to a substance, fixture, room or work.
- Any formula over the human measure (Blondel's rule in Form, walking in Visitor, the reach agent in Site), and the
  metering constant that turns luminance into exposure (the Eye).
- Which world a command acts on, and what a channel's value means (its publisher declares it).

## Connectors

Tier 1, one public type per header:

| Header | Connector |
| --- | --- |
| `MuseeUnits.h` | `Musee::TQuantity<EUnit>` and its aliases; `ToRadians`, `ToDegrees`; `ToEngine`, `FromEngine` for lengths, speeds and points |
| `MuseeFrame.h` | `FMuseeFrame` (points, directions and frames to and from the parent, inverse, compass directions); `ToEngine` / `FromEngine` for frames |
| `MuseeKey.h` | `FMuseeKey` (`Parse`, `IsValid`, `Segments`, `Parent`, `Child`; a plain string in records); `Musee::Seed(Key)` |
| `MuseeRegistry.h` | `UMuseeRegistry`: `Register(Key, Component)`, `Unregister(Key, Component)`, `Find(Key)`, `OnRegistered()` |
| `MuseePose.h`, `MuseeConfiguration.h` | `FMuseePose` (key and frame in the stage's frame); `FMuseeConfiguration` (`Find`, `Set`) |
| `MuseeHumanMeasure.h` | `FMuseeHumanMeasure`: eye height, stature, capsule radius and half-height, step, slope, headroom, reach, guard height, pace |
| `MuseeRecords.h` | `Musee::ReadRecords<T>(Path)`, `Musee::ReadRecord<T>(Path)`, `Musee::Records::DataRoot()`, `meta = (MuseeRequired)` |
| `MuseeDependencySet.h` | `FMuseeDependencySet` (`FScope`, `Files`, `Hash`) |
| `MuseeText.h`, `MuseeCaption.h` | `FMuseeText` (`ToText`, namespace `Musee`); `FMuseeCaption` (title, body, credit) |
| `MuseeUse.h`, `MuseeUsable.h`, `MuseeCaptioned.h` | `FMuseeUse` (`Available`, `Unavailable`, `Seating`); `IMuseeUsable` (`Usage`, `Use`); `IMuseeCaptioned` (`Caption`) |
| `MuseeSignals.h` | `UMuseeSignals`: `Publish` and `OnContact` / `OnActuation`; channels by `Publish`, `Read`, `Subscribe`, `Unsubscribe` |
| `MuseeContact.h`, `MuseeActuation.h`, `MuseeChannel.h` | the three signals |
| `MuseeSetting.h` | `FMuseeSetting`: `RegisterNumber`, `RegisterChoice`, `Unregister`, `Find`, `SetNumber`, `SetChoice`, `SetFromText`, `OnChanged`; command `musee.<system>.set-<setting>` |
| `MuseeCommand.h` | `FMuseeCommand`: `Register`, `Unregister`, `Run(Name, Args)`, `Names`, `IsValidName` |
| `MuseeCheck.h`, `MuseeCheckScope.h` | `MUSEE_CHECK("<System>.<Truth>", Scope, Function)`; `FMuseeCheck` (`Find`, `All`, `InputTypes`, `Evaluate(FInputs)`); `EMuseeCheckScope` |
| `MuseeFinding.h`, `MuseeMeasurement.h` | `FMuseeFinding` (check, owner, drawer, summary, reading, context); `FMuseeMeasurement` (value, unit, expected range, subject, frame, location) |
| `MuseeProbe.h` | `MUSEE_PROBE("<System>::<Verb><Thing>", Function)`; `IMuseeProbe` (`Read(FQuery)`, `Register`, `Find`, `Names`) |
| `MuseeDefect.h` | `FMuseeDefect` (the Journal entry: `ToMarkdown`, `ParseJournal`, `NamesExistingCheck`) |
| `MuseeLog.h` | `MUSEE_DECLARE_LOG_CATEGORY`, `MUSEE_DEFINE_LOG_CATEGORY` (`LogMusee<System>`), `MUSEE_DECLARE_STAT_GROUP` (`STATGROUP_Musee<System>`) |
| `MuseeCollision.h` | `Musee::Collision::Solid`, `Walkable`, `Body`, `Clearance`: each a channel and a profile name |

**A record file** is one JSON object: the struct's fields by name, or `"$records": [...]` holding several. `"$units"`
declares the unit each named field is written in (any name or symbol of the engine's unit table), and `"$sources"` says
where the numbers come from. Reading fails on a missing `MuseeRequired` field, a declared unit that isn't the field's
`Units`, an unknown field, a value of the wrong kind or a key that breaks the rule. The full rule is in `MuseeRecords.h`.

**Later** (tier 2, owner Foundation; no header exists yet):

- Probe geometry: the paths and volumes a probe returns for Instruments to draw (leak paths, travel volumes). A reading
  carries one point today.
- `FMuseeConfiguration` composition: joining the configurations of independent coupling groups into one state.
- The layer declared through `MuseeModuleRules`, for Studio.Wiring to read.
- Required and unit checks in cooked builds, which carry no reflection metadata; today they run wherever records are
  made (the editor and its commandlets).
- Setting kinds beyond a number and a choice.
- A trend log of channel values, as a building-automation system keeps, for Instruments.

## Depends on

- Core, CoreUObject and Engine (every Musée module, through `MuseeModuleRules`).
- Json and JsonUtilities, privately. Foundation is their only user in the museum.

## Checks

Foundation's standards are enforced where they are made, not by a check run later:

- **At compile time.** Only physically meaningful arithmetic between quantities compiles; the human measure is
  consistent (the capsule is a capsule, the body stands under the least headroom); a check in Configuration scope takes
  the configuration.
- **At registration.** A key, a command, a setting, a check and a probe each follow their naming rule and have one
  owner; anything else is refused with an error.
- **At reading.** Records are refused on a missing required field, a disagreeing unit, an unknown field, a wrong kind of
  value or a broken key.

Foundation registers no `MUSEE_CHECK` of its own: it holds no truth that varies with a stage, a configuration, a moment
or a capture. Its automation tests are `Musee.Foundation.*`.

## Frame budget

Foundation owns no render pass and does no per-frame work, so it declares no stat group. Publishing a signal costs what
its listeners cost; the registries are touched on registration, lookup and publish only.

## Glossary

- **Quantity**: a number with its unit (`Musee::Lux(150)`).
- **Frame**: where a thing stands and how it is turned, in metres, in its parent.
- **Key**: the name of a thing in the data (`rotunda.west`). **Seed**: the random stream a key owns.
- **Pose**: one body's frame in one state. **Configuration**: one state of every mechanism in scope.
- **Human measure**: the body the museum is built for.
- **Record**: a specification as text under `Data/`. **Declaration**: a record file's `$records`, `$units`, `$sources`.
- **Dependency set**: the record files a make read, each with its content hash.
- **Text**: a localisable string in a record. **Caption**: a thing's title, body and credit.
- **Use**: what a usable thing offers the visitor now.
- **Signal**: news between systems. **Contact**: a body struck a surface. **Actuation**: a mechanism moved.
- **Channel**: a control point with one value, as in a building-automation system.
- **Setting**: a value the visitor or owner may change, with one owner and one stored copy. **Command**: a named action.
- **Check**: pure maths proving one truth. **Scope**: where a check's truth must hold. **Finding**: what a check found wrong.
- **Measurement**: one reading of an instrument. **Probe**: an owner's way of measuring its truth where the visitor stands.
- **Defect**: a Journal entry naming the check that catches it.
- **Collision class**: Solid, Walkable, Body or Clearance.

## Sources

- The human measure: S. Pheasant and C. Haslegrave, *Bodyspace*, 3rd edition (2006), for eye height, stature and reach;
  J. J. Fruin, *Pedestrian Planning and Design* (1971), for the body ellipse; the Building Regulations for England,
  *Approved Document K* (2013), for rise, headroom and guarding; EN ISO 14122-1:2016 for the steepest ramp; DIN 18065:2020
  and F. Blondel, *Cours d'architecture* (1675), for the pace.
- Seeds: the FNV-1a hash (G. Fowler, L. C. Noll and K.-P. Vo; IETF draft-eastlake-fnv) and the SplitMix64 finaliser
  (G. Steele, D. Lea and C. Flood, "Fast splittable pseudorandom number generators", OOPSLA 2014).
- Channels report a change of value as BACnet points do (ANSI/ASHRAE Standard 135, change-of-value reporting).
- Units, their names and symbols: the engine's unit table (`Math/UnitConversion.h`); the degree and the radian by
  definition; the nit is one candela per square metre (SI).
