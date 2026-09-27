# Room sub-agent brief (shared by every room)

You write the FINAL design sheet and data schedules for ONE room of Musée Vision, a virtual museum rebuilt tonight
in Unreal Engine 5.8. Room builders will build the room in one pass *from your sheet alone*, without opening the
old code. Faithful numbers and real materials matter more than prose. Be complete, exact and unambiguous.

## §0. Safety (from the brief; binding)
- Use absolute paths in every tool call. **Never create, change or run git in
  `C:\Users\anan9\Desktop\Projects\Musee Vision`**: it's the owner's own checkout, with 280 uncommitted files of
  theirs. Read it only.
- Never write to `C:\Users\anan9\.claude\projects\C--Users-anan9-Desktop-Projects-Musee-Vision\memory`.
- Never use the Workflow tool, worktree tools, or the desktop app's branch or pull-request tools. Never run Unreal:
  you write documents and data only.
- Write ONLY your room's files in the slot `C:\mv\wt\ds-sheets`:
  `C:\mv\wt\ds-sheets\MuseeVision\Docs\Stages\<room>.md` and `C:\mv\wt\ds-sheets\MuseeVision\Data\Stages\<room>\*.json`.
- **Never run git at all** (not even status). The coordinator commits. You may read other slots' files directly
  from disk (read only).
- No downloads and no installs. Reading web pages to check a real building or product is fine.
- Never message other sessions.

## The rules (Decisions D-001…D-019, full text at C:\mv\wt\integration\MuseeVision\Journal\Decisions.md — read it)
- D-001: the design is the as-built museum plus the approved proposals (Chenghuai north, Albion south, Salon
  interior update, pond edge, Élan exterior dome, Sea Light in the Cube). Retired, never built: Sculpture Hall, old
  Chinese Wing, the Square, the Cube's Matterhorn and Raja Ampat journeys. Later decision beats an older board.
- D-017: port, don't redesign. Geometry and numbers come from the as-built museum (old code is the source). Designer
  choices live in the four schedules.
- D-018: materials are CHOSEN as specific real materials (stone by trade name/quarry/cut, wood by species/origin/
  sawing, alloy by composition, glass by make), never copied from `materials.py`. Use the draft palette's keys (below).
- D-012 topology: Rotunda ports west→Salon, north→Chenghuai, south→Albion, east→Hall of Light, plus the stairwell it
  serves down to the Classical Hall. Hall of Light east→Atrium. Atrium vertical ports → Cube (below) and Sphere
  (above), closed by irises; the glass car never lands. Arrival point: the gilt sun at the Rotunda's centre.
- D-019: the Nymphéas oval (with the lifting pond and the stair to the Reserve) is its own stage `nympheas`, beyond
  Salon bay 5. The Salon sheet covers only the five bays and the Manet cabinet, with a west port mating
  `nympheas.east`. (D-012's "Salon's pond serves the stair to the Reserve" is now the oval's.)
- D-004: the Rotunda's eye is glazed. D-005: Salon lanterns take Giverny's weather, Albion's glass London's; sun and
  hour are always the museum's. D-006: always open — controls are daylight-linked dimming, dusk switching of
  architectural light, presence-linked gallery light. D-014: no automatic blinds; care met by hang and UV-filtering
  glazing; stages may declare design targets as luminance ratios between named surfaces.
- **Light in real products:** luminaires are chosen by catalogue TYPE from
  `C:\mv\wt\r-light\MuseeVision\Data\Light\Luminaires\*.json` (open them with encoding utf-8). The 16 keys:
  downlight.3000k.31deg, downlight.4000k.31deg, linear-cove.3000k, linear-slot.3000k, picture-light.2700k,
  track-framing.3000k.medium, track-framing.3000k.small, track-spot.3000k.{12,20,29,38}deg,
  track-spot.4000k.{12,20,38}deg, track-wallwasher.3000k, track-wallwasher.4000k. NEVER write lumens, candela,
  intensity, exposure or "brighter here". If the design needs a type the catalogue lacks (step light, underwater,
  exterior uplight, cold-cathode…), write a provisional key `gap.<kind>.<cct>` and list it in §13 under
  "Catalogue gaps" with the real product type you mean.
- **Targets:** use classes from `C:\mv\wt\r-light\MuseeVision\Data\Light\UseTargets.json`: circulation (100 lx),
  circulation.night (50), stairs (100), lift (100), entrance-hall (100), orientation (5), exterior-walkway (5),
  escape-route (10.76 avg). Care classes from `C:\mv\wt\r-light\MuseeVision\Data\Art\Care.json`: works-on-paper (50),
  works-on-paper.fugitive (50), textiles (50), textiles.silk (50), oil-and-tempera (max 200, rec 150),
  stone-and-bronze (rec 300), ceramic-and-glass (rec 300). Write lux ranges only as copied from these classes, with
  `source` "UseTargets" or "Care". (Plaster casts have no class: use stone-and-bronze and flag it.)
- **Never per-room tuning.** No fill lights, no exposure, no fudge. The sheet records the design.

## Palette keys (the draft palette has landed: USE ITS KEYS)
The palette is committed on work/R-STORY-01. Read-only copies are
`C:\Users\anan9\AppData\Local\Temp\claude\C--Users-anan9-Desktop-Projects-Musee-Vision\fe83b0c0-89f0-4a7a-aec6-d7928054970e\scratchpad\Palette.json`
(48 entries: key, realMaterial, workingsByEra, roles) and `Story.md` in the same folder (the material story). Read the
entries whose `roles` name your room, and use exactly those keys. The key set includes, among others:
stone.travertine-romano, stone.crema-marfil, accent.statuario, accent.negro-marquina, accent.pierre-bleue,
white.lime-plaster, white.fibrous-plaster, white.plaster-of-paris, white.limewash, white.pearl-steel,
bronze.cusn10, gilt.gold-leaf, accent.brass-cuzn30, oak.sessile-french, accent.linen, accent.silk,
accent.mohair-velvet, glass.low-iron, glass.clear-float, glass.mouth-blown and water.still.
- `material` is a palette key. `working` is the finish that the entry's `workingsByEra` gives for your room's era
  (for example honed, patinated, oiled and waxed, brushed, polished, or mirror-polished). `history` is one of
  new, foot-worn, handled, weathered or aged.patina.
- Gilt is a working (D-014, D-015). Its `material` is the substrate's key, and its `working` is
  `gilt.gold-leaf.oil` or `gilt.gold-leaf.water`. For example:
  `{"material":"bronze.cusn10","working":"gilt.gold-leaf.oil"}`.
- Every material a surface needs must come from this palette. If a surface needs one the palette lacks, write
  `todo.<family>.<material>` and list it in §13 with the real material you mean. §13 lists only those gaps, plus any
  palette entry whose role for your room you had to guess.
- Plants belong in Planting.json and never in Finishes.json.
- Keep the sheet as short as completeness allows. Exact numbers and schedules matter; prose does not.

## Sources (read them; all read-only)
1. The drafter's spec for your room: `C:\mv\wt\r-specs\MuseeVision\Docs\Stages\<room>.md` (if it exists). It has
   exact numbers and an old-code map, and is usually good. It is a DRAFT: verify its numbers against the old code
   (spot-check every key dimension, all ports, all art positions), keep what's right, correct what's wrong, and
   record each correction in a "Corrections to the draft" note in §11 or §12.
2. Past design sheets: `C:\Users\anan9\Desktop\Projects\Musee Vision\plan\canvas\*.dc.html` (dimensions in SVG
   markup), `plan\boards\*.png` (you can view PNGs), the approved proposals
   `plan\proposals\{salon-interior,chenghuai,albion}\` (canvas HTML and any README), `plan\README.md`, the as-built
   boards `plan\canvas\AsBuilt*.dc.html`. `plan\renderings` is mood only.
3. As-built code for exact numbers: `C:\Users\anan9\Desktop\Projects\Musee Vision\windows\Source\MuseeVision\`
   (Plan\MuseePlan.h and each room's *Plan.h and structure files) and `windows\Scripts\*.py` (hangs, lights).
4. The collection: `C:\Users\anan9\Desktop\Projects\Musee Vision\assets\collection.json` (use its ids verbatim in
   Hang.json; real sizes in metres), images in assets\paintings, assets\salon, assets\albion, assets\classical. Hang
   data in windows\Scripts and data\*.json.

## Deliverable A: `MuseeVision/Docs/Stages/<room>.md` — 13 sections, in this order, with these headings
`# <Display name> · design sheet`, a short header (stage key, sources, units: metres, frame x east / y south / z up /
true north, angle convention if used), then:
1. Identity and placement — origin, level, footprint polygon in the room's own frame (point list), where it sits in
   the museum plan (plan translation + yaw), enclosures (Enclosed, Court or Outdoors) with what bounds each.
2. Ports — table: key, kind (doorway/stair/well/vertical…), class (e.g. `doorway.3000x4500.arched`), local frame
   (position + facing), mates with (`<stage>.<port>`), wells (serving/receiving). Every port that is unmated
   tonight gets its closed bronze doors (say so).
3. Pieces by trade (floors, walls, order/mouldings, ceilings/vaults, glazing, joinery, metalwork, fittings,
   mechanisms…) with exact numbers and profiles (point lists where the old code has them). EVERY piece carries a
   **surface name** in backticks (lowercase dot-scoped, e.g. `drum.wall.inner`) that Finishes.json uses.
4. Light — daylight openings (glazing key, weather observer, control); luminaires by catalogue type, count, mount
   and aim (a table matching Lighting.json); targets; design ratios.
5. Time and weather.
6. Motion — mechanisms with travel, speed, clearances, interlocks (or "none").
7. Nature — plants and water (or "none").
8. Art — every work: catalogue id and source catalogue, title, artist, size (w×h m), wall, centre, support, frame, care class.
9. Sound.
10. Visitor — what one does there.
11. The old-code map — file:line for every part (paths relative to `windows\`), plus "Corrections to the draft".
12. The first-pass cut — what's built tonight and what waits for polish. Small and honest.
13. Palette keys to confirm (every `todo.` key and every guessed role → the real material meant), and Catalogue gaps.

## Deliverable B: schedules in `MuseeVision/Data/Stages/<room>/`, to the STAGE CONTRACT
The builders follow `C:\mv\briefs\STAGE-CONTRACT.md`. Read it completely: it defines the anchors, bodies, channels
and dresser orders. Include only the schedules the room needs, and every file must parse as JSON.
```
Finishes.json   { "stage": "<room>", "finishes": [ { "surface": "drum.wall.inner", "material": "<palette key>", "working": "<working>", "history": "<history>", "note": "why" } ] }
Lighting.json   { "stage": "<room>",
                  "daylight":   [ { "opening": "<opening key>", "glazing": "<palette key>", "weather": "museum|giverny|london", "control": "none|<device type>" } ],
                  "luminaires": [ { "id": "<id>", "type": "<catalogue key>", "mount": "<Mount anchor key>" or [x,y,z], "aim": [x,y,z],
                                    "operation": "always|dusk|presence", "follows": "<channel key, optional>", "body": "<body key, optional>" } ],
                  "targets":    [ { "surface": "<surface>", "class": "<use or care class>", "lux": [min,max], "source": "UseTargets|Care" } ],
                  "design":     [ { "brighter": "<surface>", "darker": "<surface>", "ratio": [min,max], "note": "" } ] }
Hang.json       { "stage": "<room>", "works": [ { "work": "<id>", "source": "<repo-relative path of the owning catalogue>", "anchor": "<Wall anchor key>",
                  "along": <left edge along the wall's x, m>, "height": <lower edge above the floor, m>, "size": [w,h],
                  "support": "canvas|panel|paper|silk|cast|…", "frame": "<frame type or none>", "care": "<care class>" } ] }
                  (A work in the round, such as a cast or a sculpture, may instead use a Mount or Seat-like anchor. If a
                  Wall frame doesn't fit, add a "wall" surface and a "centre": [x,y,z], and note it.)
Planting.json   { "stage": "<room>",
                  "water": [ { "key": "<stage>.<water>", "anchor": "<Water anchor key>", "body": "<body key or empty>",
                               "outline": { "ellipse": [a,b] } | { "polygon": [[x,y],...] }, "depth": d, "freeboard": f, "material": "<palette key>" } ],
                  "beds":  [ { "bed": "<Bed anchor key>", "species": "<Latin name>", "count": n, "spacing": s, "body": "<body key or empty>", "note": "" } ] }
Mechanisms.json { "stage": "<room>", "mechanisms": [ { "key": "<stage>.<mech>", "joint": "prismatic|revolute", "body": "<Body anchor key>", "axis": [x,y,z],
                  "travel": <m or deg>, "time": <s>, "profile": "ease-in-out|linear", "control": "<Use anchor key or channel>",
                  "open": [ { "channel": "<channel>", "to": 1.0, "time": t } | { "move": "open" } ], "close": [ ... ],
                  "guard": "body-in-travel", "clearance": <m> } ] }
```
- **Anchors.** Section 1 or 3 of the sheet carries an **Anchors** table: key, kind, frame (position, plus the axes
  or facing), and what it belongs to. The kinds are Body, Use, Mount, Wall, Bed, Water and Seat. Every anchor that
  any schedule names must appear in it. A Wall anchor's frame has its origin at the face, x along the wall, y out of
  the wall and z up. A Water anchor's frame is the water level at the centre.
- **Mechanisms that publish and follow**: `"publishes": "<channel>"` (position as a fraction of travel, 0–1);
  `"follows": {"channel", "in": [c0, c1], "out": [t0, t1]}` (linear, clamped) replaces control/open/close for a
  mechanism driven by another's channel. A channel step (`{"channel", "to", "time"}`) stays in open/close sequences.
- **Curved walls**: Hang.json gets `"walls": [{"anchor", "path": {"ellipse": [a,b], "from_deg": θ0, "direction": "increasing|decreasing"}, "note"}]`
  (a circle is an ellipse [r, r]); θ is the PARAMETRIC angle (x = a cos θ, y = b sin θ) in the stage frame, from +x
  towards +y (y south). `along` is the arc length on the face from θ0. Works on curved walls are built curved to it.
- A mechanism driven by `follows` has no control/open/close.
- **Channels** are named `<stage>.<thing>.<what>`. Motion publishes them and Light follows them (`follows`).
- Care classes and use classes are the keys in Care.json and UseTargets.json (for example `oil-and-tempera`, not
  the contract's example `oil-paintings`). Water material is a palette key (`water.still`).
Hang ids come verbatim from the catalogue that owns the work (collection.json, data/*.json, assets/*/*.json). Name
that catalogue in `source` (for example `data/albion.json`), and never unify the catalogues. A size missing from the
catalogue comes from the old hang scripts or the boards, cited in §8. The Reserve's stair port mates
`nympheas.stair`, and the Salon has no stair port.
Positions are metres in the room's frame. `aim` is a POINT in the room frame that the luminaire aims at, not a
vector. Keys are lowercase and dot-scoped. Every surface named in the sheet appears in Finishes.json, and every
`surface` in the Lighting targets is a surface from the sheet. Write JSON in UTF-8 with a 2-space indent.
Frame types (provisional): `frame.gilt.carved`, `frame.gilt.plain`, `frame.oak.plain`, `frame.black.lacquer`,
`mount.scroll.silk`, `mount.album` and `none`. Add others if you need them, and flag them.

## Before you finish: self-check
Run a small python check (encoding utf-8) that every JSON parses, that every Finishes surface appears in the .md,
that every surface used in Hang/Lighting exists in Finishes, that every Hang `work` id exists in collection.json,
that every luminaire `type` is a catalogue key or a `gap.` key, and that no JSON or the sheet contains "lumen",
"candela" or "exposure" as a setting. Fix anything it finds.

## Your final reply (to the coordinator, not the owner), compact:
- files written;
- palette keys to confirm (key → real material);
- catalogue gaps;
- every conflict between sources and how you resolved it (or the question to ask the Architect);
- each port's key, class and the neighbour port it mates with (so the coordinator can cross-check neighbours);
- corrections you made to the draft.
