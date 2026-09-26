# Design sheets: status at the pause (2026-09-26, 18:55)

- **Complete:** none.
- **Partial:** none. Four sub-agents (rotunda, salon, chenghuai, albion) had started reading their sources and were
  stopped at the pause before writing any files.
- **Not started:** rotunda, salon, chenghuai, albion, classical-hall, cube, sphere, reserve, grounds, atrium,
  hall-of-light.

## What is here
- `room-agent-brief.md`: the shared brief each room's sub-agent gets. It covers the §0 safety rules, the rules
  (D-001 to D-019), the sources, the 13-section format, the JSON schemas, a shared provisional palette-key
  convention (`<family>.<material>[.<variety>]`, gilt as a working), the luminaire catalogue keys, the use and care
  target classes, and a self-check. Its scratchpad paths are gone; on resume, point each sub-agent at this file.
- `check_room.py <room>`: the coordinator's check before committing a room. It checks section order, that each
  JSON file parses, that every surface appears in both the sheet and Finishes.json, the catalogue types, the target
  classes, and the hang ids against every catalogue in the owner's checkout (collection.json, data/*.json,
  assets/*/*.json). It also flags lumens, candela and exposure.

## Notes for the resume
- Neither the palette (work/R-STORY-01) nor the Nymphéas reference (work/R-OVAL-01) had landed. Check both first.
- By the pause, the drafts in r-specs covered 9 rooms: sphere and atrium were still missing.
- collection.json holds only 66 works and has no sizes. Albion, classical, salon and sculpture works live in
  data/*.json and the assets/*/ catalogues, so hang ids come from whichever catalogue owns the work.
- Conflict to raise: D-012 says the Salon's pond serves the stair to the Reserve, but D-019 moves the pond and
  stair to `nympheas`. The Reserve's up-port therefore mates the oval, not the Salon.
