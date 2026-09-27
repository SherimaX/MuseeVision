# Decisions

Each decision records its context, the decision, and what follows from it. The owner asked the Architect to decide
these by judgment (2026-09-26): "use your judgment on all … the designs of the previous complete museum was great
already. we are just restructuring it to make it a real one piece of complete art." The owner can overturn any of
them; a changed decision gets a new record, never an edit.

## D-001 · The design is the finished museum
**Context:** The as-built museum (September 2026) and the approved proposals are the design.
**Decision:** The new build reproduces that design through the new systems; designs are not re-approved. The design
consists of the as-built rooms and these approved proposals:
- Chenghuai on the north door;
- Albion on the south door;
- the Salon interior update and the pond edge;
- the Élan exterior dome;
- the Cube's first work, *Sea Light*.

The Sculpture Hall, the Chinese Wing and the Square are retired, as are the Cube's Matterhorn and Raja Ampat
journeys. None of them is built.
**Follows:** Room specs draw on the owner's standing decisions, then the approved proposals, then the master boards,
then the as-built photographs. Where a board contradicts a later decision (for example, a board still showing the
Sculpture Hall north), the later decision wins.

## D-002 · Where the museum stands
**Context:** The old museum had a city switch: Beijing (the default) or New York. A real sun needs a real place, and
a sundial is cut for one latitude.
**Decision:** Keep both cities. Beijing (39.9° N) and New York (40.7° N) lie within 0.8° of latitude, so one dial cut
for 40.3° N reads within a fraction of a degree in either. The city stays a runtime setting, restricted to cities
within 1° of the dial's latitude. `Time.DialTrue` enforces that restriction.
**Follows:** Time holds the observer. Beijing is the default.

## D-003 · The sun keeps the approved idealised path
**Context:** The approved design idealises the museum's sun at 15° an hour, from 06:00 to 18:00, on a path through the
bronze node onto the hour ring, so the sun clock reads every hour. A real sun, with the node 20.3 m above a 14 m dial,
would reach the dial only around noon in June and July.
**Decision:** Keep the design. Time declares a **sun model** in `Data/Time/Museum.json`. The model is `idealised`
(the default, as designed) or `ephemeris`. The moon, the stars and the solar terms always come from the real Meeus
ephemeris. `Time::Dial` and `Time.DialTrue` check the dial against the declared model. With the idealised sun the
dial doesn't depend on latitude, so the Beijing/New York switch (D-002) stays at runtime without restriction.

## D-012 · The finished museum's topology, restored
The panel's architecture had drifted from the as-built design. The Site plan follows the museum as built:
- **The Rotunda's ports:** west (to the Salon), north (to Chenghuai), south (to Albion) and east (to the Hall of Light),
  plus the stairwell it serves (down to the Classical Hall).
- **Further matings:**
  - the Hall of Light's east port to the Atrium;
  - the Salon's stair port to the Reserve's, a well that the Salon's lifting pond serves;
  - the Atrium's vertical ports to the Cube (below) and the Sphere (above).
- **The arrival point** is the gilt sun at the Rotunda's centre.
- **Élan is three stages:** Atrium, Cube and Sphere.
  - The glass car never lands.
  - Its vertical ports close with an **iris** closure class.
  - The Cube is sealed by its ceiling iris around the mast. It has no side vestibule, light lock or landing door.
- **The Sphere** shows the real sky over the museum's city. No stage "pushes" a different sky, and the
  scoped-sky machinery is removed. Sea Light is a near piece that performs inside the Cube's sealed shell.

## D-013 · Connectors come in two tiers
**Decision:** A system's public connectors are split into two tiers, and a tier-2 item is built when the first stage
that needs it is specified.
- **Tier 1 (now)** is what the Rotunda and the mock-up bay need. It is written, implemented and frozen at Gate A′.
- **Tier 2 (declared)** is named in the system's README under "Later", with its owner, but no header exists for it
  yet.

## D-014 · Amendments from the second architecture check
Each of these is folded into the architecture:
- **Matter**
  - *Gilt* is a working, not a substance: gold leaf with its measured complex F0, laid over a substrate (plaster,
    bronze or wood). The bole shows through with wear.
  - Every substance record carries its family (stone, white, bronze, oak, accent, glass, water, living), and an
    accent names the core family it ties back to. `Matter.OneLanguage` fails any accent that isn't declared.
  - The boards' *brass* is read as a declared accent of the bronze family.
- **Form**
  - The drawing vocabulary admits **Carving** (relief or in-the-round ornament made by rule inside a block within
    stock) and **Ornament** (a displacement relief on a host surface, drawn from evidence such as a cast or a
    measured drawing).
  - Joints gain the kind **welded and faired**, for seamless shells such as the Élan's pearl shell.
  - The leak probe (`Form::FindLeakPaths`, pure ray casting) lives in the runtime module, so Instruments can show it
    in any development build.
- **Glazing**
  - A Glazed opening is *any transmissive infill*: glass, stained glass with lead cames, paper or silk, each with
    its transmission in Matter.
  - A stained-glass work is an Art anchor of kind **Glazing** set into a Form opening. Art owns the image, Form owns
    the opening, and Matter owns the glass.
- **Light**
  - Light reads the museum Moment for operation and prediction.
  - A stage may declare **design targets** as luminance order or ratio between named surfaces (for example, the
    Salon's half-light vault). These are measured, not knobs.
  - No automatic blinds. Where the design has sun patches, care limits are met by the hang and by UV-filtering
    glazing, as real museums do.
  - For now: direct-model prediction plus measurement. Path-traced commissioning, daylight coefficients and annual
    dose are tier 2.
- **Visitor**
  - The Eye owns exposure, the tone curve (with a highlight shoulder, so skies and glass never clip) and *sampling*:
    screen percentage, the upscaler and anti-aliasing. Light owns transport quality only.
  - The owner's intents are kept: walk, stride, hop, look, sit, step-to, the hour key and the city key.
  - Quest Touch bindings are primary in the headset adapter.
- **Site**
  - Reachability is judged over a state graph: walking, plus any use the visitor can trigger. A closed door with a
    usable handle is not a trap.
  - The **passage** between two mated ports is an enclosure owned by the joint. It has its own pieces, traffic and
    light targets, and `RoomAt` returns "between A and B" there.
  - Wayfinding text belongs to Site; captions and labels belong to Art.
- **Motion** owns the runtime occupancy guard: a drive holds, or reverses for a door, when the visitor's body is in
  its next travel slice.
- **Time** owns a seeded weather generator conditioned on each place's climate normals, used when the live feed
  is unreachable, so the weather never falls back to a plain default.
- **Foundation's record reader** fails on a missing field marked Required, and on a field whose units don't match.
- **Content** is generated entirely by Studio, including the material parameter collections (MPC_Moment and
  others), from definitions in their owning systems. There are no hand-made assets.
- **Specimens:** the mock-up bay lives in `Source/Stages/Specimens/MockupBay`. It is made of two small stages mated
  by the standard doorway, so the joint, a blind and Mate are all exercised before the Rotunda.
- **Modules**
  - Matter gains an editor twin, `MuseeMatterMaking`, which builds its material graphs in C++ (D-007).
  - The screen adapter lives in `MuseeVisitor`.
  - `MuseeVisitorHeadset` is built only in the VR target (D-010).

## D-004 · The Rotunda's eye is glazed
**Decision:** The steel-and-glass lattice eye stays as built, after the British Museum's Great Court. The Rotunda is an
Enclosed shell whose only opening to the sky is Glazed.

## D-005 · Remote weather over two rooms' glass
**Context:** The approved designs give the Salon's lanterns Giverny's live cloud and rain, and Albion's glass London's.
**Decision:** Keep them, as the owner designed. The sun and the hour are always the museum's. Only the *weather* over a
room's glazing may come from another place. Time keeps its weather by keyed observers (museum, giverny, london),
fetched in one request. A room's Glazed opening may declare which observer's weather falls on it.
**Follows:** Time's weather service is keyed. Matter's surface law and Nature's precipitation read the weather of the
observer the opening declares. This is a deliberate artistic exception to "one sky", limited to weather and recorded
here.

## D-006 · The museum is always open
**Decision:** There are no opening hours. Light operates the way a modern gallery's controls do: daylight-linked
dimming, dusk switching of architectural light, and presence-linked gallery light.

## D-007 · Generation in C++
**Decision:** Studio generates meshes, levels and material graphs in C++ (UMaterialEditingLibrary and Substrate
expressions), where automation tests reach them. Python is at most a thin driver. A Phase 0a spike confirms that a
Substrate material can be generated from code.

## D-008 · Plugins and source art outside the repo
**Decision:** The DLSS and Streamline plugins live in `C:/mv/third_party/Plugins`, which the `.uproject` names through
`AdditionalPluginDirectories`. The folder is writable, because game-target builds compile into it, and game builds
run one at a time. Source art lives in `C:/mv/third_party/SourceArt`. The engine's shared Zen DDC is used as it
is: `UE-LocalDataCachePath` is never set.

## D-009 · Knowledge in the Journal, process in the Register
**Decision:** `Journal/` holds what the museum knows: decisions, defects and reviews. The Register (`C:/mv/register`,
branch `register`) holds the state of the work: orders, gaps, inspections, verify stamps and jobs.

## D-010 · The headset
**Decision:** An installed engine can't enable plugins per target. So OpenXR is enabled for DebugGame game builds
only, and the headset adapter (`MuseeVisitorHeadset`) is built only in the `MuseeVisionVR` target. The desktop game
never links OpenXR.

## D-011 · Folders and modules
**Decision:**
- A system's folder is named for the system (`Source/Systems/Form`).
- Its modules are named `Musee<System>` (`MuseeForm`, and `MuseeFormMaking` for the editor half).
- Stages live under `Source/Stages/Rooms/<Room>` and `Source/Stages/Works/<Work>`.
- Studio and Instruments sit beside the systems, in `Source/Studio` and `Source/Instruments`.

## D-015 · Matter defines, Studio writes
**Context:** The architecture says "Studio alone writes assets", while D-007 and D-014 put the material graph
builders in C++.
**Decision:** `MuseeMatterMaking` exposes pure builders: `BuildMaster(EMatterMaster, UMaterial&)` and
`FillInstance(const FMatterSpec&, UMaterialInstanceConstant&)`. They fill an object they are given and never create,
name or save a package. Studio's make step calls them and saves the result under `/Game/Generated/Matter/`, with
names formed by one function in Matter. `Musee::Matter::Resolve(spec)` depends on the spec alone, never on a room,
and there is never a fallback material. A family (D-014's data concept) and a master (a Substrate shading
configuration) are kept apart. Masters are split by physical shading topology and never by room. Gilt is a working,
applied as a vertical layer that any opaque master can carry.
**Follows:** The same pattern holds for every system with a making twin: the system defines its builders, and
Studio writes the assets.

## D-016 · The whole museum, first pass, by Sunday night
**Context:** The owner (Saturday 26 September, 18:10): "this is a weekend project. tomorrow is the deadline." Offered
three scopes, the owner chose **"Whole museum, first pass"**.
**Decision:** By Sunday night, every stage is rebuilt on the new systems, joined by the site plan, and packaged
for desktop and VR: the Rotunda, Classical Hall, Salon, Reserve, Hall of Light, Chenghuai, Albion, Atrium, Sphere,
Cube with Sea Light, and the Grounds. "First pass" means:
- every room is built through the shared systems, from real records and specs (never tuned by hand);
- the systems are built to the depth the rooms need tonight;
- polish follows in later sessions.

The gates compress:
- no separate specimen-wall walk;
- the owner walks the joined museum on Sunday and gives notes, and one fix round follows;
- inspections stay, one inspector per order.

The principles and the wiring don't compress.
**Follows:**
- The systems are built tonight in parallel, and the rooms overnight as the systems land.
- Assembly and packaging happen Sunday morning, then the owner's walk, one fix round, and the final package on
  Sunday evening.

## D-017 · Port the finished rooms; refine details later through schedules
**Context:** The owner (Saturday 18:45): "we are not changing the geometry of the room, it's just some details we
want to change … it should already be 95% there … we need to design the system so that it's easy to refine details
later, like changing the material of something … use the Rotunda to verify and make it robust, and then the other
rooms should really be one-shot … reuse ideas from previous work."
**Decision:**
1. **Port, don't redesign.** Each room's geometry is carried over from the as-built museum (the old room code and
   its plan numbers are the source), expressed through Form's language, with only the approved deltas (D-001).
   Research refines what the old museum already had; it never starts from zero.
2. **Geometry is code; everything a designer changes is data.** Each room keeps its shape in its module, and keeps
   its choices in plain files beside it, in `Data/Stages/<Room>/`, named after the documents real architects use:
   - `Finishes.json`, the **finish schedule:** each named surface or part mapped to its material key
     (substance.working.history);
   - `Lighting.json`, the **lighting schedule:** each fixture's catalogue type, mount and aim, and each surface's
     target;
   - `Hang.json`, the **hanging plan:** works to anchors;
   - `Planting.json`, the **planting plan:** species to beds (where the room has any).

   Changing a material, a fixture or a painting is a one-line edit to a schedule. Studio's incremental make remakes
   only what the edit touches, and the checks re-run.
3. **Materials start from the as-built language.** The old museum's material values (the LANGUAGE, NAMED,
   PHYSICS and POLISH tables of `windows/Scripts/materials.py`, and the CC0 photographic sets the owner already
   approved in the Rotunda and Salon) seed the new records. The research corrects them where the physics was wrong
   and sources them. A record says which of the two each number came from.
4. **The Rotunda proves the pipeline.** It goes through the whole pipeline first. Every defect it reveals is fixed
   in the system that owns it, and the pipeline's checks are hardened, so that each later room is a single pass. The
   checks every room must pass are:
   - sealed shells, with no light leaks;
   - every material key resolved;
   - UVs in metres;
   - lux targets met;
   - reachability, with no traps;
   - ports mated;
   - performance within budget.

   Overnight, the other rooms' modules are ported and compiled in parallel. Their full make, lighting and checks run
   after the Rotunda has hardened the pipeline.
5. **Reuse the old workshop's good ideas:**
   - one Unreal session doing many steps (`apply_all`);
   - named views shot in one launch (`tour.ps1`, the MuseeDo steps);
   - the sun clock's clearance checks at several states;
   - the Nanite bake settings.

   These are kept as evidence and verification. They are never used as targets for tuning a room (D-013).

## D-018 · The material story (replaces D-017 point 3)
**Context:** The owner (Saturday 19:00): "you need to make sure you have the actual wood or stone in your mind, not
copying their values, but look at the design to decide your kind of wood or stones, and these materials should
build a whole story, we don't decide the materials just by the previous work, we need to understand the choice and
maybe change to better ones."
**Decision:** Geometry is ported (D-017), but materials are **chosen**. Before any material instance is made:
- **One document tells the museum's material story,** `Docs/Matter/Story.md`, with its data in
  `Data/Matter/Palette.json`. It reads the design (the boards, the proposals, the rooms' eras: Classical bones,
  modern light, future glass) and the four families (stone, white, bronze, oak; D-014). For every role in the museum
  it names the **specific real material**: the stone by its trade name, quarry and cut; the wood by species, origin
  and sawing; the alloy by its composition; the glass by its make. It says why that material, in that room, in that
  finish, belongs to the story.
- **Each old choice is examined:** what it was and why it was made. It is kept, or replaced by a better real
  material, with the reason.
- **Values come from the chosen real material.** Its records carry its measured values with their sources. Old
  values from `materials.py` are never copied as values. The CC0 scan sets count as evidence only where they are a
  photograph of that material.
- The rooms' finish schedules name palette entries, so the story is carried through every room.

**Follows:** The Matter session builds its records and instances from the palette, not from the old tables. The
story is reviewed by independent critics before the instances are made.

## D-019 · The Nymphéas oval is the first room, and the golden iris opens the pond
**Context:** The owner (Saturday 19:20): "let's rebuild the lily pond room first, as that's a room [that has]
different things inside and [is] quite simple. we don't need to build the basement yet, but you can make the lily
pond openable by clicking on the golden iris on it."
**Decision:**
- **The Nymphéas oval becomes its own stage**, `nympheas`, split from the Salon. Its east port mates the Salon's
  west port, beyond bay 5. It proves the pipeline in place of the Rotunda (D-017 point 4). Its geometry is simple,
  but it exercises nearly every system:
  - water carried by a moving basin, and planting (Nature);
  - the lift (Motion);
  - the carved lily frieze (Form);
  - travertine, gilt bronze, plaster and canvas (Matter);
  - the four willow panels of the Orangerie's second room, 51 m at true size (Art);
  - daylight under the velarium (Light), and the click (Visitor).
- The Rotunda joins the one-pass rooms.
- **The pond** follows the approved design (plan/proposals/salon-interior, the Oval board):
  - an 8.4 × 4.6 m oval echoing the room;
  - a sitting-height moulded travertine coping with a gilt-bronze inner lip;
  - brimming black water that mirrors the velarium and the panels;
  - lilies in clumps as at Giverny;
  - an Art Nouveau carved lily frieze on the coping's outer face;
  - iris clumps only at the two ends of the axis.

  It lifts 2.6 m in 5 s on four bronze posts, water and planting with it, over the 36-step stair east to the
  Reserve.
- **The golden iris** is a gilt-bronze iris set on the coping at the head of the axis, echoing the living irises at
  its ends. (The old build's control was a golden lily; the owner's word is iris.) Using it (click, or the headset
  trigger) runs the line of warm light round the lip, then lifts the pond. Using it again lowers it. Motion's guard
  stops the pond if the visitor is in its way.
- **The basement isn't built yet.** The stair is the oval's own (it serves the well), and it ends at the Reserve's
  closed bronze doors: the port's blind.

## D-020 · The Reserve is built (lifts D-019's deferral of the basement)
**Context:** The owner (Sunday 11:40), after the first full build: "you can work on the reserve. why is it deferred".
D-019 had deferred the basement on the owner's word ("we don't need to build the basement yet"). That deferral was for the demo room only: "that's a decision before when we create the demo room, but now we are making the full build, let's add it in." The full build holds every room.
**Decision:**
- **The Reserve becomes a stage**, `reserve`, built from its sheet (`Docs/Stages/reserve.md`). Its one port,
  `reserve.stair` (`stairwell.straight.2400x3200.arched`), mates `nympheas.stair`: the oval's stair runs down to it,
  and the golden iris's pond lift is the way in.
- The oval's stair port stops being blind once the Reserve's level exists: Site mates it, as it mates every port.
- The racks are top-hung as built (D-017); at most one is drawn out at a time (their `group`). Rack C starts drawn out.
- It joins the build after the arrival fix, so the owner's first walk isn't held for it.
