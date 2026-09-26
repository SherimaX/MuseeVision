# Musée Vision · Renaissance: the build plan

**Status:** draft 2, 2026-09-26, for the owner's review. **Branch:** `renaissance`.

Draft 2 answers an adversarial review of draft 1 from four independent reviewers: a delivery lead, an Unreal
build engineer, a multi-agent sceptic, and an advocate for the owner's artistic direction. They found 11 blocking
issues and about 40 major ones; every one is addressed here. The engine facts below were checked against the
installed UE 5.8 source and this PC.

**Depends on:** the final system architecture from the design panel, which is still running. This plan uses the
names the panel's judges converged on: eleven systems (Foundation, Time, Matter, Form, Light, Sound, Motion,
Nature, Art, Visitor, Site), the stages (Rooms and Works), and Studio and Instruments beside them. Two ownership
questions wait for the panel's final report: which system owns the sky, and which owns the physical camera.

---

## 0. At a glance

```
 Owner (Art Director) ── approves gates, specs, dossiers; walks every build
  └─ Architect ── contracts, orders, triage, merges (a role handed between sessions)
      ├─ Drafters ········ headers, room specs, material dossiers  → reviewed by the Architect
      ├─ Master Builders · the eleven systems, Studio, Instruments
      ├─ Room Builders ··· one room each, from its spec, systems only
      ├─ Fitters ········· site entries, packaging, museum-wide runs (scripts only)
      └─ Inspectors ······ independent; rubric + evidence renders; never build
```

| Phase | What | Gate | First estimate |
|---|---|---|---|
| 0a | Workshop + engineering spikes (not tied to the architecture) | spike results | 1–2 days, **can start now** |
| 0.5 | **Specimens:** material dossiers researched from the real things | owner reviews photos | runs alongside 0a–2 |
| 0b | Project skeleton from the final architecture | | ½ day |
| 1 | Contracts + ART.md + the Rotunda's spec | **A** (visual) | 2–3 days |
| 1.5 | Walking skeleton: one thin path through every system | headers frozen | 1–1½ days |
| 2 | Foundation, in parallel | **B** (mock-up bay, desktop + VR) | 3–5 days |
| 3 | The Rotunda, with 2–3 owner look rounds | | 3–6 days |
| 4 | Engineering + art review | **C** | 1–3 days |
| 5 | Rooms in three waves | each wave | 12–20 days |
| 6 | Assembly and release | release | 2–3 days |
| | **Calendar total, including the owner's review time** | | **about 5–7 weeks** |

The five rules that carry the plan:

1. **Contracts before code.** Real headers, specs with numbers, and tests written before the builder starts.
2. **Problems flow up.** A builder that finds something missing reports it and never works around it.
3. **Nobody grades their own work.** Tools re-run every check, and independent inspectors judge from evidence.
4. **The truth lives in files, in one place.** Any session can end without losing anything.
5. **The look is checked like the code.** Renders to a fixed protocol, real-photograph comparisons, and the
   owner walking builds throughout, not only at the end.

---

## 1. The idea

The museum is built by a small organisation of agents arranged like the code itself.
- The **Architect** plans, and writes the plan as *contracts*: real headers, room specifications and checks.
- **Master Builders** implement the shared systems behind those contracts.
- **Room Builders** build rooms using only those systems.
- **Fitters** join the rooms and produce builds.
- **Inspectors**, who never build, check every piece against its contract, its principles and its rendered
  evidence.

The owner is the **Art Director**. Work flows down; problems flow up. A builder who finds the layer below missing
something reports it and never patches around it. That one rule is the difference between this build and the old
one.

## 2. The organisation

| Role | Held by | Mission | Writes | Must never |
|---|---|---|---|---|
| **Art Director** | the owner | Decides what the museum is; approves gates, specs and material dossiers; walks builds | Decisions (the Architect records them the same turn, quoting the owner) | — |
| **Architect** | the main session, handed on by file between sessions (§5.5) | Contracts, orders, triage, merges, gate packets | `docs/` contracts, public headers, `wiring.json`, `OWNERS.json`, `.uproject`, `Target.cs`, `Config/` | Merge anything that the merge tool hasn't verified and an inspector hasn't passed |
| **Drafters** | strong agents, one per document | Draft headers (Phase 1), room specs, material dossiers | Drafts only, for the Architect's review | Merge their own drafts |
| **Master Builders** | strong agents, one per system group, each in a persistent worktree slot | Implement a system behind its public header | `Source/<System>/Private/**`, their own `Build.cs`, `Tests/Unit/**` | Change a public header or a locked acceptance test |
| **Room Builders** | strong agents, one per room | Build a room from its spec, using only the systems' connectors | `Source/Rooms/<Room>/**` | Add shared code, or change a room to fix how it looks (§4.5) |
| **Fitters** | fast agents | Run the site, packaging and museum-wide scripts, and report structured results | Site entries (`Site/rooms/<room>.json`), run reports | Edit code, config or content. A failed run goes up as a defect |
| **Inspectors** | a fresh strong agent per inspection; two for critical orders | Try to refute that the work meets its contract, the principles and ART.md | Structured inspection records (§6.3) | Build or fix anything |

Why the tiers are shaped this way:
- **Mistakes cost the most at the top,** so contracts and the foundation get the strongest model.
- **The rooms are the art,** so room builders are strong agents. The Salon and the first wave always use the
  strongest model.
- **Joining is mechanical by design.** A room is joined by one file in the site plan, and the doorway detail is
  built once by the systems. Fitters only run scripts; anything that fails goes back up.
- **Nobody grades their own work.** Tools re-run every check (§5.3), and inspectors see the contract, the diff,
  the tool-produced verify log and the evidence renders, never the builder's reasoning.

## 3. The contract: what exists before a builder starts

| Document | What it holds | Written by |
|---|---|---|
| `ARCHITECTURE.md` + `wiring.json` | The system map; the wiring as machine-readable data; the stages; the map strategy (§7.6); module registration; the complete list of rooms and stages with the wave each belongs to; the plugin versions | Architect, from the panel's report |
| `CRAFT.md` | The mindset and the code rules: naming, file and function limits, comment style, the banned list (per-room tuning, magic numbers, duplicate helpers, "temporary" code, World Partition, Blueprint graphs), and what "finished" means | Architect |
| `ART.md` | The look counterpart of CRAFT: the six principles as visual rules, the museum-wide look rules kept from the old LOOK_TARGETS "Everywhere" section (no clipped whites, no bloom halos, crisp contact shadows, gilt never flat yellow…), and real photographic references. No per-room targets | Architect; the owner approves it at Gate A |
| System READMEs | One page each: its purpose, what it owns, what it doesn't own, and its connectors | Drafters, reviewed by the Architect |
| Public headers + stubs | The connectors, fully documented, and compiling | Drafters to one house-style sample, then one consistency pass by the Architect |
| `docs/matter/<material>.md` | **Material dossiers** (§9, Phase 0.5): substance × working × history, with real evidence | Drafters (research); approved by the owner as images |
| `docs/light/luminaires.md` | A catalogue of real luminaire types: lumen package, beam, IES profile, colour temperature, CRI. Room specs choose fixtures from it and **never contain a raw lumen value** | Drafter + Light's Master Builder |
| Room spec format + the Rotunda's spec | Local frame, dimensions, ports, parts, material keys, light plan (fixture types, counts, positions; lux targets drawn from cited practice), mechanisms, works, budgets per spec camera, checks. It also has a **Real references** section (measured drawings and photographs of the real buildings it draws on) and a **Readings** section (every interpretation made, for the owner to approve) | Spec Drafter, reviewed by the Architect, approved by the owner |
| `docs/tolerances.md` | Every numeric pass mark (§8) | Architect |
| Acceptance tests | For each order, written **before** it is issued, in `Tests/Acceptance/**`. Builders can't edit them; verify checks their hashes | Architect or a test Drafter |
| `OWNERS.json` | The paths each order and role may touch | Architect |
| `orders.yaml` | The manifest the order files are generated from | Architect |
| Lessons ledger | What worked visually in the old Rotunda and Salon, the physical reason it worked, and which new system inherits that method. It records methods, not code | Drafter, reviewed by the Architect |

**Where room specs get their facts, in priority order:**
1. the owner's decision records;
2. approved proposals;
3. the master boards (after refreshing `plan/` from the live canvas);
4. the as-built photographs.

Renderings are mood only. The old code is never a source of numbers: the material boards copy its values, so
**boards give design intent** (family, finish per era, colour range), and **dossiers give the physics**.

## 4. Work orders

### 4.1 Size
An order is one coherent unit of about **400–800 new lines, capped at 1,000**. That is small enough to build in
one session with compiling, and to inspect carefully. Each order's "read first" list names exact files and line
ranges. References to the old code are specific paths, never "the old code".

### 4.2 Identity and states
- An order's id looks like `F-FORM-02`. Its branch is **`work/F-FORM-02`**. (Git can't create `renaissance/…`
  branches while the `renaissance` branch exists.)
- Its gaps are `F-FORM-02-g1`, `-g2`…, its inspections `F-FORM-02-i1`…. No global numbering, so parallel work
  never collides.
- An order's states are:
  `drafted → issued → building → built → verified → in inspection → passed → merged`
  plus `blocked (gaps)`, `partial`, `stale` (its contract changed) and `returned (reasons)`.
- An order records the **contract version** it was built against: the `Source/*/Public` tree hash or the last
  decision record. Verify fails if the branch's base is behind the current contract.

### 4.3 Life of an order
1. The Architect issues it (the order file is generated from `orders.yaml`, and its acceptance tests are locked).
2. The dispatcher starts a builder in its persistent worktree slot (§5).
3. The builder works:
   - it commits work in progress after every step that compiles;
   - it keeps `HANDOFF.md` up to date in its order folder (done, next, open questions, last verify result);
   - it files gaps to the Register as they arise (§6.1).
4. The builder marks the order `built`. The **tool** then re-runs verify on that exact commit. A builder's own
   claim of passing counts for nothing.
5. A fresh inspector reviews it with the tool's log and the evidence renders.
6. **The merge tool:**
   1. rebases onto `renaissance` in the integration worktree;
   2. re-runs verify on the merged candidate;
   3. fast-forwards only if it is green;
   4. runs verify again after the merge.

   If `renaissance` ever turns red, the merge is reverted and the order reopened, never patched. **`renaissance`
   is always green.**
7. **A rebase that changes more than trivially** invalidates the inspection. That needs a new verify plus a
   short inspection of just the change.

### 4.4 Returns
- Only **blocker** and **major** findings cause a return. **Minor** findings go to a follow-up list.
- A re-inspection receives the previous report and checks that each finding is closed, plus the diff since. A new
  finding on unchanged code counts as minor unless it is a blocker, so the goalposts don't move.
- **After two returns,** the Architect re-examines the order itself. The contract may be wrong, not the builder.
- **A returned order** goes to a new builder, with the order, `HANDOFF.md`, the inspection report and the diff so
  far. Agents don't persist; files do.

### 4.5 The two kinds of change
When something about a room is to change, the Architect classifies it and shows the owner the classification:

- **A design note** means the owner changes what the room *is*: "bay 2's silk one step deeper", "a bench here",
  "deeper coffers". It becomes a decision record and a spec revision, which the owner approves.
- **A truth defect** means it looks unreal or behaves wrongly: a light streak, a room too dark, stone that reads
  as plastic. It goes to the defect log and to the system that owns the broken truth (§6.4).

**What is forbidden:** a builder changing a room to fix how it looks. That means brightening a corner, adding a
fill light, swapping a material, or exposure tricks. The spec may change, but only through an owner decision.

## 5. The dispatch loop: how agents actually run

Agents don't persist and can't message each other. The loop below is how work moves anyway.

### 5.1 Launching
- **One Workflow script per wave of orders.** For each order it runs these stages in a pipeline: **build →
  verify by the tool → inspect → record.**
- **Builders run in persistent worktree slots,** made once by `tools/worktrees.ps1`:
  - the slots are `C:\mv\wt\mb1…mb5` (Master Builders), `rb1…rb3` (Room Builders), `insp` (inspectors) and
    `integration`;
  - each slot uses a sparse checkout (the project, `docs/`, `data/` and `tools/` only), with
    `core.longpaths=true`.

  The dispatcher gives each agent its slot's absolute path. The Workflow tool's automatic worktree isolation
  isn't used for builders, because it creates a fresh copy each run, and each fresh copy would compile from cold.
- **At most 3 builders and 1 inspector on the strongest model run at once,** to stay within usage limits.
  Workflow's resume restarts interrupted items.
- **A project permissions allowlist** (`.claude/settings.json`, which the owner approves once) lets background
  agents run the tools without stopping at prompts:
  - `tools/verify.ps1`;
  - `tools/unreal.ps1`;
  - `tools/register.ps1`;
  - read-only git;
  - `git add` and `git commit` inside `C:\mv\wt\*`.

  Everything else still asks.

### 5.2 What each agent receives
- **A builder's packet:**
  - the order file, generated from its template;
  - CRAFT.md and ART.md;
  - the README and headers of its system;
  - the headers of the systems it depends on;
  - the exact reference files and line ranges;
  - `HANDOFF.md` and the previous inspection, if the order is resumed or was returned.
- **An inspector's packet:**
  - the order file and the contract;
  - `git diff renaissance...work/<id>`;
  - the tool-produced verify log for that SHA;
  - the evidence renders and measurements (§8.3);
  - the previous inspection report, if there was one.

  Inspectors may run verify, and write throwaway probe tests in the `insp` slot, but never commit.

### 5.3 Checks are run by tools, not claimed by agents
- `tools/verify.ps1` writes `verify-<sha>.json` into the Register.
- The merge tool accepts only a green stamp whose SHA matches the tree it merges.
- Verify prints a short summary (the first 10 errors, one line per failed check), and full logs go to
  `Saved/verify/<sha>/`. Compile output never floods an agent's context.

### 5.4 Metrics
The dispatcher records, for every order:
- the wall time;
- the Unreal queue wait and the Unreal run time;
- the number of returns;
- the blocker and major findings;
- the tokens used, where the tools report it.

`tools/status.ps1` builds `STATUS.md` from these records. Nobody writes it by hand.

### 5.5 The Architect is a role, not a session
A single session can't hold weeks of triage.
- **Each Architect session covers about one wave.** It starts by reading `STATUS.md`, `ARCHITECTURE.md`, the
  decision records since its last log, and the open gaps. It ends by writing a short session log.
- **The Architect reads records, never transcripts.** A builder's reasoning stays in its transcript; only its
  structured result reaches the Architect.
- **Drafting is delegated** to Drafters (room specs, dossiers, header sets). The Architect reviews and decides.
- **Merging is a tool.** The Architect's part is the decision, and the mechanics are scripted.
- **Gaps are triaged within hours,** measured in STATUS. Meanwhile, builders carry on with the parts of their
  order that the gap doesn't touch.

## 6. How problems travel: the Register

The **Register** is where all shared state lives. (A museum's register is the book in which every object is
recorded.) It is:
- a separate worktree at `C:\mv\register`, on its own branch `register`;
- visible to every agent immediately, with no merging needed;
- written **only** through `tools/register.ps1`, which serialises writes with a lock and commits each entry as its
  own file, so there are never merge conflicts.

```
register/
  orders/<id>.md            status, contract version, slot, metrics
  gaps/<id>-g<k>.md         filed by builders the moment a gap appears
  inspections/<id>-i<k>.md
  verify/<sha>.json         tool stamps
  jobs/<job>.json           Unreal job queue (§7.4)
  defects/<date>-<slug>.md  one file per defect
  critique/<date>/          each owner walk: images, the owner's words verbatim, where each item went
  gates/A.md B.md C.md      gate packets and the owner's decisions, quoted
  sessions/<date>-<n>.md    Architect session logs
  STATUS.md                 generated
```

Decision records (`docs/decisions/<n>-<slug>.md`) live on `renaissance` itself, because they change contracts.

### 6.1 Gap reports (problems flow up)
A builder files a gap with `tools/register.ps1 gap` as soon as it is found. The gap says:
- what was needed, and why;
- which connectors were considered;
- what is blocked, and what continues meanwhile.

The order goes to `blocked` or `partial`. The Architect triages it in one of three ways:
- **a connector really is missing:** a decision record plus a foundation order;
- **the contract is wrong:** a decision record, and the affected orders are marked `stale`, then rebased, or
  restarted with the decision record added to their read-first list;
- **it already exists:** the gap is answered with how to use it.

### 6.2 Contract changes
After Gate A′ (§9, Phase 1.5), a public header changes only through a decision record. Each decision record
carries: context, decision, consequences, the affected orders, and the new contract version.

### 6.3 Inspections: rigorous and calibrated
- **The output follows a fixed schema.** For every acceptance check and every CRAFT and ART rule, it gives pass or
  fail with file:line evidence or a cited frame. Findings are graded blocker, major or minor. It also answers
  **"looks real?"**, citing the evidence frame.
- **Critical orders get two inspectors:** Foundation core, Form, Matter, Light and the camera. The order is
  returned if either one finds a blocker.
- **Inspectors are calibrated before Phase 2.** We plant 8–10 known defects in a sample order, and inspectors
  must catch at least 80% of them. This is repeated whenever the model or the prompt changes.

### 6.4 Defects
Each defect is one file with four parts: **symptom → the physical truth that was broken → the fix in the
owning system (and its order) → the check that now prevents it.** For example: *a light streak on the dome → the
shell had a seam, so it wasn't watertight → Form closes the seam where pieces meet (F-FORM-07) → the leak check
runs on every generation.*

## 7. The workshop

### 7.1 Layout on disk
- **The new project:** `<repo>/<project>/`. The name comes from the panel; the draft is `MuseeVision/`.
- **Everything the build needs that git doesn't carry:** `C:\mv\` (short paths, no spaces).
- **The main checkout:** no agent ever runs git in it. It holds the owner's 281 uncommitted edits (84 paintings,
  `Shared/*.swift`, docs). All merges happen in `C:\mv\wt\integration`, and the tools refuse `git add -A` and
  `git stash`.
- **A backup exists:** it was taken on 2026-09-26 and verified, at `D:\MuseeVision-Backup\2026-09-26` (the old
  Content, SourceArt and Plugins included).

### 7.2 Things git doesn't carry
- **The DLSS and Streamline plugins:**
  - they live in one **writable** shared folder, `C:\mv\third_party\Plugins`, which the `.uproject` references
    through `AdditionalPluginDirectories`;
  - the folder can't be read-only, because game-target builds compile the plugin into its own Intermediate
    folder. For the same reason, every game-target build runs in the HEAVY slot (§7.4);
  - the plugin version is recorded in ARCHITECTURE.md: 8.8.0-NGX310.9.1.
- **The source art** (CC0 photographic sets and the material evidence) lives in `C:\mv\third_party\SourceArt`, and
  the pipeline finds it through one setting.
- **The derived-data cache:** the installed engine already shares one Zen cache (about 85 GB, and warm) across
  every project and worktree. **We never set `UE-LocalDataCachePath`;** doing so would move the cache to an
  empty location. Phase 0a confirms in a log that the Zen cache is in use.

### 7.3 Compiling
- **Only one build at a time.** UnrealBuildTool runs one instance per engine installation. Every compile uses
  `-WaitMutex`, so compiles queue rather than fail. That's acceptable at 10–20 s per incremental build.
- **Editor builds every loop; game and VR builds at the end.** A builder builds the editor target in every loop.
  The desktop game target and the VR DebugGame target are built before an order is marked `built`; this catches
  editor-only code leaking into runtime and a broken VR module.
- **Every module is registered from day one.** The `.uproject` and all three `Target.cs` files list every planned
  module (the systems, Studio, Instruments, every room and the VR layer) as a stub. That way no order edits
  those files, and a room that no other module uses still ends up in the game.
- **Shared Build settings live in one base class,** `MuseeModuleRules`.

### 7.4 Unreal runs: slots, not one lock
What really conflicts across the machine is RAM, the GPU and the shared plugin build folder, not the project
DLLs, since each worktree has its own. So `tools/unreal.ps1` enforces **slots**:

| Slot | Capacity | Used for |
|---|---|---|
| **GPU** | 1 | Measurements, evidence renders, test-bench walks, game runs |
| **HEAVY** | 1 | Nanite and texture generation, cooking and packaging, any game-target build |
| **LIGHT** | 2 | Headless (`-nullrhi`) automation tests and geometry checks, admitted only while free RAM is above 40 GB |

- **Each slot is a Windows named mutex,** held by the wrapper process that launches Unreal inside a Job Object.
  If the wrapper dies, the operating system releases the mutex and ends Unreal with it. When a run finishes, the
  wrapper waits for its shader-compile workers.
- **Before any GPU-slot run,** `nvidia-smi` must show the GPU idle. The owner playing a build or using VR always
  takes priority, and the measurement waits.
- **Long jobs never block an agent.** `unreal.ps1 submit` returns a job id at once, and `unreal.ps1 wait <id>`
  polls within the 10-minute limit on tool calls. Packaging always runs in the background.
- **Phase 0a measures** whether two LIGHT jobs really can run side by side. The slot table is set from the
  measurement, not assumed.

### 7.5 Content is a build product, generated deterministically
Everything in `Content/` is generated from code, specs and source art by Studio, and nothing there is edited by
hand. For this to be affordable, generation has to be **deterministic**:
- meshes are committed with `bUseHashAsGuid = true`, and textures use `UseHashAsGuid()`;
- each asset stores the hash of its inputs, and a package is **only rewritten when that hash changes**;
- package names are stable, and no timestamps are saved.

Without this, every regeneration misses the cache and forces a full recook. Phase 0a tests it: generate twice in
two worktrees, and the second run must cause zero cache misses and rewrite zero packages.

**All generation lives in C++, in Studio,** including the material graphs, which are built with
`UMaterialEditingLibrary` and Substrate expressions. That way generation can be covered by automation tests.
Python is at most a thin driver of about 200 lines.

**The shape of what Form and Studio output:**
- always `UStaticMesh` assets, never runtime procedural meshes;
- Nanite on for opaque parts, and off for translucent parts and water;
- meshes built in batches through `BatchBuild`, with the peak memory checked;
- fixed collision, distance-field and Lumen-card settings for each kind of part;
- a minimum wall thickness of 10 cm, so Lumen and distance fields don't leak.

### 7.6 Maps
- **Each room generates its own level,** `/Game/Rooms/<Room>/L_<Room>`, in its local frame. These levels don't
  use World Partition.
- **The Site generates `/Game/Site/L_Museum`,** holding one Level Instance per site-plan entry.
- **The mock-up bay** is its own generated level.
- **Every generated map is listed for cooking.** Runtime data is staged as non-asset files, and
  `DefaultGameTarget=MuseeVision` is set.
- **`-SkipCook` is banned in scripts.**

### 7.7 Packaging for the owner
- **After every merge wave, not every order,** the Fitter packages `renaissance` from the integration worktree
  into `C:\mv\Builds\<target>-<yyyyMMdd-HHmm>` (desktop, and VR as DebugGame), in the HEAVY slot.
- **The owner gets the exe path,** a one-line changelog and the latest evidence contact sheet. These builds are
  for looking and need no reply. The gates remain the formal approvals.
- **Cold and warm package times** are measured in Phase 3 and replace the estimates here.

### 7.8 Disk and RAM
**Disk:**
- about 5 GB of generated Content per worktree slot, across about 10 slots;
- the Zen cache, about 85 GB and shared;
- builds, about 5 GB each.

**RAM:** every Unreal run needs at least 40 GB free before it starts. The old mass Nanite build peaked at about
100 GB of the 128 GB.

## 8. The verify tool

### 8.1 Profiles
Each order names the profile it must pass, so no order is held to a check that can't run yet:

| Profile | Steps | Required from |
|---|---|---|
| `code` | 1–5 | Phase 1.5 on, for every order |
| `physical` | + 6 | Phase 1.5 on, for any order that makes geometry |
| `measured` | + 7, 8 | Phase 1.5 on, for Light, Matter, the camera, and every room |

### 8.2 Steps
1. **Compile:** the editor target on every run. The game and VR targets before `built`.
2. **Wiring:** UBT's own resolved module graph (`-Mode=JsonExport`, for the editor, game and VR targets) is
   compared with `wiring.json`. Also banned:
   - `#include` paths containing `..` or another module's `Private/`;
   - `PublicIncludePaths`, `PublicIncludePathModuleNames` and `DynamicallyLoadedModuleNames` outside an
     allowlist;
   - `CircularlyReferencedDependentModules`.
3. **Ownership:** the diff touches only the order's paths, and the locked acceptance tests' hashes are unchanged.
4. **Craft lint,** using **lizard** (pure Python, MIT-licensed, kept in `tools/third_party`):
   - function length and complexity limits, taken from CRAFT.md;
   - duplicate blocks longer than 30 lines;
   - no room names in any system;
   - no `/Game/` paths or raw lumen values outside specs;
   - no magic numbers, meaning a float literal other than 0, 1, 0.5 or 2 outside a named `constexpr` that carries
     its unit.
5. **Unit and acceptance tests:** Unreal automation, in the LIGHT slot:
   `UnrealEditor-Cmd <uproject> -nullrhi -unattended -nosplash -nosound -ExecCmds="Automation RunTests Musee.;Quit" -TestExit="Automation Test Queue Empty" -ReportExportPath=<dir>`
   The editor build runs first. (An installed engine can't build Low-Level Test programs, so automation is the
   test runner.)
6. **Physical checks:** headless, LIGHT slot, each an algorithm with a tolerance.

   | Check | Method | Pass |
   |---|---|---|
   | Leak | Voxelise the assembled shell at 2 cm, cap the declared openings, flood-fill from outside | No interior voxel is reached. Walls are at least 10 cm thick |
   | Traps | Flood-fill a 34 cm × 180 cm capsule over the walkable voxels from each port | Every walkable voxel reaches a port |
   | Clearances | Sample each mechanism's motion at 64 or more steps; measure mesh-to-mesh distance | At least the clearance stated in its spec (5 mm by default) |
   | Ports | Compare each pair of mated port frames in world space | Within 1 mm |
   | Surfaces | UVs in metres, tangents derived from UVs, every material key has an approved dossier | All pass |

7. **Measurements:** GPU slot, in a Development or Test build, with DLSS at 67% SR+RR, Frame Generation off for
   GPU timing, and the GPU confirmed idle.

   | Measurement | Method | Tolerance |
   |---|---|---|
   | Direct illuminance | Analytic: E = I·cosθ/d² from the light components, with shadow rays | Within ±10% of the rendered value |
   | Total illuminance | Fixed-exposure (EV100 = 0) HDR capture, cosine-weighted | Within ±10% of the path tracer; the lux target within the spec's band |
   | Lux-meter validation | One known-lumen light in a zero-albedo room | Within ±5% before Gate B relies on the meter |
   | GPU time | A settled view from each of the spec's cameras, including what's visible through the ports | ≤ the spec's budget; the museum's total is 20 ms at 4K |
   | Exposure across ports | Measured EV along the walking path through each port | Follows the camera model's adaptation curve (set in Phase 1) |

8. **Evidence renders:** GPU slot, see §8.3.

### 8.3 Evidence renders
The same camera protocol is used for **every** room and system. It gathers evidence; it is not a target to tune
towards:
- eye height from each port;
- per material: one raking-light view, one grazing floor view, and one far view at 10–20 m to show repetition;
- behind, below and mid-motion for every mechanism;
- each view at two hours of the day;
- a VR frame of the specimen wall.

Two more kinds of evidence build on these:
- **Blind pairs** for Matter and Light: a specimen render beside a CC0 photograph of the real material under
  matching light. If inspectors reliably pick out the render, that is grounds for a return.
- **At Gate C, the new Rotunda beside the old packaged build,** from the same cameras. This is a floor that must
  not regress, not a target. The target is still real photographs.

## 9. The phases

### Phase 0a: the workshop and the spikes (starts as soon as the owner approves this plan)
None of this depends on the system names.
- **The tools:**
  - `worktrees.ps1`, `unreal.ps1` (slots, jobs, Job Objects), `register.ps1` and `status.ps1`;
  - the verify skeleton (steps 1, 3 and 4, driven by data);
  - the merge tool, and the order, gap and inspection templates;
  - the dispatcher Workflow, tested on a dummy order;
  - the permissions allowlist.
- **The shared folders:** `C:\mv\third_party` (copied from the backup, never moved).
- **The spikes,** each with a number that decides it:

  | Spike | Passes when |
  |---|---|
  | Parallel compile | The UBT behaviour is recorded and the slot table set |
  | Plugins in the shared folder | A desktop build and a VR DebugGame build both succeed |
  | Deterministic regeneration | The second run has zero cache misses and rewrites zero packages |
  | Cold generation + cook | Timed and recorded |
  | Automation from the command line | A test runs and reports |
  | Wiring through JsonExport | The resolved module graph is exported and compared |
  | Two LIGHT jobs at once | Run side by side without errors |
  | A Substrate material generated from code | Haze plus a second specular lobe renders |
  | Level Instances | A level made of Level Instances cooks |
  | Lux meter | Within ±5% on one known light |
  | Cold build and first editor start in a new worktree | Timed and recorded |
  | Zen cache | Confirmed in the log |

- **Exit:**
  - a builder in a worktree files a gap, and the Architect sees it in the Register;
  - the dummy order goes all the way round the dispatch loop;
  - every spike has its number.

### Phase 0.5: the specimens (runs alongside 0a, 0b, 1 and 2)
The owner's principle 4 starts here, before any code: **understand the real material first.**
- **The specimen list comes from the Rotunda's material keys, not from memory:**
  - honed travertine;
  - cream crackle marble;
  - coffer lime plaster, moulding plaster and cast plaster;
  - bronze, as one substance with two states: patinated (the sun clock's dial) and brushed (the threshold strip);
  - aged gilt (the rosette, the meridian and the inscription);
  - the lattice steel, with its finish named;
  - low-iron glass.
- **Each dossier covers four things:**
  - **substance:** e.g. Tivoli travertine is calcite laid down by hot springs, with bands and open or filled
    pores;
  - **working:** vein-cut or cross-cut, honed grit, the direction of brushing;
  - **history:** wear at thresholds, dirt settled in pores, bronze patina chemistry;
  - **calibration evidence:** CC0 photographs with their lighting, measured albedo ranges, IOR, and how roughness
    is spread.
- **The owner reviews each dossier as images:** the real references, then the first specimen renders when Matter
  can produce them.
- **Alongside it:** the luminaire catalogue, and the lessons ledger (what worked visually in the old Rotunda and
  Salon, and why).

### Phase 0b: the skeleton (after the panel's report)
- **Work:**
  - the new project, with every module registered as a stub;
  - `ARCHITECTURE.md` and `wiring.json` from the panel's report;
  - the `.gitignore` for Binaries, Intermediate, Saved, DerivedDataCache and Content;
  - verify step 2.
- **Exit:** the empty project compiles for the editor, desktop game and VR targets, and verify passes.

### Phase 1: the contracts · **Gate A** (visual)
- **Work:**
  - CRAFT.md and ART.md;
  - one sample header and README in the house style;
  - Drafters write each system's headers to that sample, then the Architect does one consistency pass (names,
    units, ownership);
  - the room spec format and the Rotunda's spec (with its real references and readings);
  - `tolerances.md`, the acceptance tests for Phase 1.5 and 2, `OWNERS.json` and `orders.yaml`;
  - inspector calibration.
- **Inspection:** two independent inspectors try to break the contracts. One sends the defect scenarios through
  the headers; the other checks each header against the principles.
- **Gate A: the owner approves a visual packet:**
  - the system map, ART.md and the craft rules;
  - the Rotunda's spec, shown with its boards and real reference photographs (the Pantheon, the British Museum's
    Great Court);
  - the Rotunda's material dossiers.

### Phase 1.5: the walking skeleton · **Gate A′** (headers frozen)
One Master Builder builds the thinnest path through every system end to end:
- Foundation core;
- Form making one wall with an opening;
- Matter with one material;
- Studio generating it, deterministically;
- Light with one catalogue fixture;
- the camera;
- Instruments reading lux at one point;
- verify running every profile.

Until this path runs, the headers may still change freely. **Once it runs, they freeze.** That is the real end of
contract-making. After Gate A′, a header changes only through a decision record. This step also gives the first
measured speed of the loop (compile + editor start-up + generation), which replaces the estimates in §12.

### Phase 2: the foundation · **Gate B**
- **Staffing:** 3–5 Master Builders. Parallel work is limited to orders that use other systems' *headers only*.
  An order that relies on another system's *behaviour* is issued after that system's order has merged.
- **Merge order:**

  | Wave | Orders |
  |---|---|
  | 2a | Foundation core, Time, the full Matter substance model, Form, Studio (generation) |
  | 2b | Light (fixtures, the sun, **the sky and the camera, both complete**), Instruments, Motion, Sound, Visitor (desktop + VR) |
  | 2c | Art (hanging, frames, picture lights, the scan acceptance rules), Site (ports, joints, the doorway detail), Nature (connectors only; its full build is scheduled below) |

- **The mock-up bay** replaces an abstract test wall. It is the physical mock-up architects build before they
  approve stone, and it tests the systems; it is not Rotunda work. It contains:
  - one drum bay, with a pilaster, niche and entablature;
  - a section of coffers;
  - a stretch of floor;
  - in every core material family;
  - under the real sun through an opening, and under gallery light;
  - a hung painting in a gilt frame with a picture light, and one glass stele;
  - two spaces at very different light levels, joined by the standard doorway.
- **Gate B:** every check passes in verify, the mock-up bay's measurements are within tolerance, and **the owner
  walks the bay on desktop and in VR.**

### Phase 3: the Rotunda
- **Staffing:** one Room Builder, a Fitter, inspectors, **and the Master Builders for Form, Matter and Light,**
  because this is where the foundation gets proven.
- **Work:**
  - the drum, niches, pilasters, entablature, the coffered dome and the eye;
  - the passages up to four sealed ports, and the stair port under the sun clock;
  - the sun clock (the first user of Motion);
  - the four casts (checked first against the Art scan-acceptance rules).
- **Two or three owner look rounds,** each on a packaged build with its evidence sheet. Each note is classified
  (§4.5): design notes revise the spec, and truth defects go to the systems. Fixes to the foundation from the
  Rotunda get a **fast lane**: a smaller inspection scope now, and a full inspection at Gate C.
- **Exit:** every check passes, the room is measured, and it is packaged for desktop and VR.

### Phase 4: the review · **Gate C**
Two scorecards, each with numeric thresholds written into `tolerances.md` in Phase 1:

**The engineering scorecard:**
- 0 wiring violations;
- 0 duplicate blocks over 30 lines;
- no capability in the Rotunda that duplicates or bypasses the kit;
- the time from an edit to seeing it is within the threshold measured at Phase 1.5;
- Rotunda generation within its threshold;
- GPU time within 20 ms at 4K in the Rotunda's worst spec view;
- craft lint clean.

**The art scorecard:**
- blind real-photo pairs: a panel of 3 inspectors identifies the render in no more than 70% of the pairs;
- the evidence contact sheet;
- the side-by-side with the old build shows no regression;
- ART.md compliance.

Fixes go to the foundation through orders. **At Gate C the owner walks the packaged Rotunda and reads both
scorecards,** and only then does the room tier begin.

### Phase 5: the rooms
- **Every room spec is drafted ahead of time.** The specs for waves 5.1 and 5.2 are drafted during Phases 3–4
  and approved as a batch with Gate C. The wave 5.3 specs are approved during wave 5.1.
- **For each room:**
  1. the spec (already approved);
  2. the room is built;
  3. it is inspected, with evidence renders;
  4. a Fitter joins it;
  5. the owner walks it, one walk per wave.

| Wave | Rooms and stages | Model |
|---|---|---|
| 5.1 | Classical Hall (from the Rotunda's stair port) · Salon (the second quality benchmark), with the pond lift | strongest |
| 5.2 | Reserve (+ its stair) · Hall of Light · Chenghuai · Albion | calibration on the Reserve (below), then as measured |
| 5.3 | Élan: Atrium, Sphere, lift and car · the Cube with *Sea Light* (a Work) · Grounds (façade, Élan exterior, landscape, canal) | as measured |

- **Calibration:** 2–3 Reserve orders are built by both models in parallel. The inspector doesn't know which built
  which. Rooms get the lighter model only if its average returns per order are ≤ 1.0 and its blocker and major
  counts match the strong model's.
- **The Sculpture Hall** is retired by a decision record.
- **Performance is measured on the joined build:** from wave 5.2 on, a Fitter runs the per-view performance
  measurement at the end of every wave, including the views through ports.

**How complete each system is, and by when:** so no room waits on a half-built system.

| System | Complete for the Rotunda by | Complete for the whole museum by |
|---|---|---|
| Light (incl. the sky and the camera, pending the panel's ownership) | Gate B | Gate B |
| Nature: plants, wind, seasons | — | before wave 5.2 (Hall of Light, Chenghuai) |
| Nature: water (pond, canal) | — | before wave 5.1 (the Salon's pond) |
| Motion: lifts and cars | the sun clock at Gate B | before wave 5.3 (Élan) |
| Sound: rooms, rain, footsteps | the Rotunda's sound at Gate B | before wave 5.1 |
| Visitor: VR | at Gate B | at Gate B |
| Art: hangs, cases, steles | the casts at Gate B | before wave 5.1 |

**The design-decisions track** runs alongside Phases 1–4. These decisions must be taken before the wave that
needs them:

| Open decision | Needed by |
|---|---|
| Salon interior proposal ("Proposed") | wave 5.1 spec, drafted in Phase 3 |
| Chenghuai design (position confirmed, design proposed) | wave 5.2 spec |
| Albion ("Proposed") | wave 5.2 spec |
| The Cube's first works, and the Sphere at Ø 28 m | wave 5.3 spec |

The waves are ordered so that a room whose design is still undecided never blocks one that is decided. Before
any spec is drafted, `plan/` is refreshed from the live canvas.

### Phase 6: assembly and release
- **Work:**
  - the Site joins every room;
  - the museum-wide checks run: every port mated, exposure along the walk through every port, performance for
    every view, no traps anywhere;
  - desktop and VR builds are packaged.
- **Exit:** the owner walks the whole museum.

## 10. Models, budgets and stop rules

**Models:**

| Role | Model |
|---|---|
| Architect | this session's model |
| Drafters, Master Builders, Inspectors, Room Builders in waves 5.1 and the Rotunda | the strongest available |
| Room Builders from wave 5.2 | as measured by the calibration |
| Fitters | a fast model; it only runs scripts, and every failure goes up to a strong agent |

**Budgets:**
- **Unreal time:** one generation session took 4–9 minutes in the old build. An order needs about 3–6 Unreal runs,
  so the foundation's roughly 30 orders need about 10–25 hours in the HEAVY and LIGHT slots. The runner works at
  night too. STATUS reports each day's queue waits.
- **Tokens:** each phase has a budget, recorded in STATUS. An order that uses more than twice its estimate goes to
  the Architect. A phase that runs 1.5 times over is reported to the owner.

**Stop rules:**
- If Gate C slips to more than twice its estimate, the owner decides how to go on.
- If the Rotunda fails the side-by-side with the old build after three look rounds, the owner decides how to go on.

Either way, `main` and the old build stay playable throughout.

## 11. What the owner is asked to do

Each request is one page, plus one build where it makes sense. Every owner decision is written into the Register
the same turn, with the owner's words quoted.

| When | The owner |
|---|---|
| Now | Approves this plan (Phase 0a and 0.5 start) |
| During 0.5 | Reviews the material dossiers as images |
| Gate A | Approves the system map, ART.md, the Rotunda's spec and its dossiers (a visual packet) |
| Gate B | Walks the mock-up bay on desktop and in VR |
| Phase 3 | 2–3 look rounds on packaged Rotundas |
| Gate C | Walks the Rotunda, reads both scorecards, and approves the wave 5.1–5.2 specs as a batch |
| Each wave | Walks the wave's rooms; approves the next wave's specs |
| Open design decisions | By the date in the decisions track |
| Release | Walks the whole museum |

Builds after each merge wave arrive with an exe path and need no reply. The owner can raise anything at any time,
and every note is classified as a design note or a truth defect (§4.5) and routed.

## 12. Estimates and the critical path

```
0a ─┬─ 0b ── 1 ──(A)── 1.5 ──(A′)── 2 ──(B)── 3 ──── 4 ──(C)── 5.1 ── 5.2 ── 5.3 ── 6
0.5 ┴────────────── dossiers ───────────┘   specs 5.1–5.2 ─┘   decisions ─┘
```

| Phase | Days | Owner review time included |
|---|---|---|
| 0a + 0b | 1½–2½ | — |
| 1 + Gate A | 2–3 | ½–1 |
| 1.5 | 1–1½ | — |
| 2 + Gate B | 3–5 | ½–1 |
| 3 | 3–6 | 2–3 look rounds |
| 4 + Gate C | 1–3 | ½–1 |
| 5 (three waves, in order) | 12–20 | a walk per wave |
| 6 | 2–3 | ½ |
| **Total** | **about 5–7 weeks** | |

**Agent speed is not the constraint.** The old build produced about 90k lines in three days. What drives the time
is the owner's reviews and design decisions, the Unreal slots, and look iteration.

**How big it is expected to be:**
- the foundation: about 15–20k lines of C++;
- each room: 1–4k lines;
- the whole museum: 35–50k lines, against 92k in the old build.

These estimates are replaced with measured speed at Phase 1.5, not at the end of Phase 2.

## 13. Risks

| Risk | Sign | Prevention |
|---|---|---|
| The rooms drift apart | Builders interpret specs differently | Contracts, specs with numbers, locked acceptance tests, ownership checks, calibrated inspectors, evidence renders |
| Patches creep back | A workaround inside a room | Gap reports; the ownership check; the two kinds of change (§4.5) |
| Contracts turn out wrong | Many gaps against one header | The walking skeleton before the freeze; the two-returns review; decision records with `stale` orders |
| The Unreal queue jams | Long waits in STATUS | Slots instead of one lock; runs at night; mutexes that release themselves when a process dies; LIGHT jobs admitted only with RAM free |
| Regeneration is slow | Cache misses, full recooks | Deterministic generation, tested in Phase 0a |
| Worktrees break | Missing plugins or source art | The shared third-party folder; Zen shared by default; sparse checkouts at short paths |
| A session dies mid-order | Half-finished work | Work-in-progress commits, `HANDOFF.md`, the Register, and Workflow resume |
| The Architect's context overflows | Triage slows, contracts drift | One session per wave, records not transcripts, drafting delegated, merging scripted |
| The owner becomes a bottleneck | Gates wait | Specs approved in batches ahead of time; the decisions track; one page per request |
| The look is lost between specs and builders | It "passes every check" but looks CG | ART.md, dossiers, evidence renders, blind pairs, look rounds, builds after every wave |
| Performance arrives too late | Over 20 ms when joined | Budgets per spec camera; per-wave measurement on the joined build |
| The owner's own work is damaged | A sweeping git command in the main checkout | No agent runs git there; `add -A` and `stash` refused; merges only in the integration worktree |

## 14. The next steps

1. **The owner approves this plan.** Phase 0a (the workshop and spikes) and Phase 0.5 (the specimens) can then
   start at once, alongside the design panel.
2. The panel finishes, the Architect reviews its report critically, and the owner approves the system map.
3. Phase 0b, then Phase 1.

---

## Appendix: templates

**Work order,** `register/orders/<id>.md` (generated from `orders.yaml`):
```
id: F-FORM-02     branch: work/F-FORM-02     slot: mb2     status: issued
role: Master Builder     model: <tier>     verify profile: physical
contract version: <Public tree hash> (decision record 0)
allowed paths: <project>/Source/Form/Private/**, <project>/Source/Form/Tests/Unit/**, <project>/Source/Form/Form.Build.cs
acceptance tests (locked): <project>/Source/Form/Tests/Acceptance/Wall*.cpp  sha256: …
read first: CRAFT.md; ART.md; Source/Form/README.md; Source/Form/Public/*.h;
            windows/Source/MuseeVision/Salon/SalonKit.h:86-235 (reference: the indexed mesh builder)
build: <what, in plain words, ~400–800 lines>
done when: verify profile passes; acceptance tests pass; <measurements>
returns: 0     metrics: <filled in by the dispatcher>
```

**Gap report,** `register/gaps/<id>-g<k>.md`:
```
order: F-ROOM-ROT-03     against: Light / Public/LightPlan.h
needed: <what, and why this can't be built without it>
considered: <existing connectors, and why they don't fit>
blocked: <parts waiting>     continuing: <parts in progress>
```

**Inspection,** `register/inspections/<id>-i<k>.md`:
```
verdict: pass | return     verify stamp: <sha>     previous: <id>-i<k-1> (findings closed: …)
per check / rule:  <rule> — pass | fail — <file:line or frame>
findings: [blocker|major|minor] <what> — <evidence>
looks real?: <yes/no> — <frame cited>
```

**Defect,** `register/defects/<date>-<slug>.md`:
```
symptom → broken truth → fix (system, order) → check that now prevents it
```

**Decision record,** `docs/decisions/<n>-<slug>.md`:
```
context · decision (owner's words quoted where theirs) · consequences · affected orders · new contract version
```

**Handoff,** `<order folder>/HANDOFF.md`:
```
done · next · open questions · last verify result (sha, profile, summary)
```
