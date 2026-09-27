# Time

The museum's one clock, its sky's bodies and its weather. Module `MuseeTime` (Runtime); the only user of HTTP.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `TimeMoment.h` | `FTimeMoment`: the museum's instant and civil hour in its city, the sun it shows (idealised by default, D-003, with twilight after its hours), the real Moon and star field, the solar term and season, and `Weather` at each place |
| `TimeWeather.h` | `FTimeWeather`: one place's weather (`museum`, `giverny`, `london`; D-005), with the surface wetness and snow cover it has left |
| `TimeSubsystem.h` | `UTimeSubsystem` (world): `Now()`, `OnMoment()` (announced on change, never polled), `Refresh()` |
| `TimeMomentParameters.h` | `MPC_Moment`'s path and its scalar parameters (the stage contract); Time writes them at every moment |

Settings and commands: `time.hour` (-1 live, or a civil hour), `time.city` (the cities within the dial's tolerance,
D-002), `musee.time.advance-hour` (the hour key), `musee.time.advance-city` (the city key).

The live weather: one Open-Meteo request for every place every `Weather.json` `live.refreshMinutes`, kept in
`Saved/Musee/Time/LiveWeather.<city>.json` and read back through Foundation's record reader; each place's seeded
generator (from its climate normals) gives the weather whenever the reading isn't current.

## Later (tier 2, owner Time)

- `FTimeSky` and a pushed sky observer (retired with the scoped sky by D-012; only if a work needs it again).
- `time.rate` as a setting (the rate is `Museum.json`'s `clock.rate` today).
- `Musee::Time::ReferenceMoments()` and the checks `Time.EphemerisMatchesReference`, `Time.DialTrue`,
  `Time.NormalsForEveryCity` registered with Foundation's check framework.
- Visibility and per-opening cloud from the feed, for Light's daylight through remote glazing.

## Tests

`Musee.Time.*`: the core's almanac, calendar, clock, dial and weather tests, and `Records`, `Moment.Sun`,
`Moment.Sky`, `Moment.City`, `HourKey`, `Parameters`, `Feed`, `Subsystem`.
