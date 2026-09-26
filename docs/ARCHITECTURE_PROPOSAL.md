# Musée Vision · Renaissance: the system architecture (proposal)

**Status:** proposal from the design panel, 2026-09-26: five independent architects, three judges, a synthesis, an adversarial stress test (46 scenarios) and a revision. **Not yet approved by the owner.** Once approved, it becomes `ARCHITECTURE.md` in the new project (Phase 0b of [BUILD_PLAN.md](BUILD_PLAN.md)).

## The Museum's Trades, closed: eleven systems, two phases, and stages joined at one doorway (branch renaissance, after the stress test)

Each shared system is one real trade. It owns one body of physical truth, the probe that measures that truth and the check that guards it. Everything is defined by what it really is before how it looks: a substance with a history, cut into pieces with real joints, lit in lux, seen by one eye, moved by a machine, grown by a rule. The museum is made in the editor and only lived in the game. What is cut at making time depends only on making-time decisions. What changes while the museum runs (wetness, season, dimming, blinds, the hour) changes through runtime parameters, never by remaking. Checks are pure maths in the system that owns the truth. Studio supplies the context (the assembled pieces, every mechanism configuration, reference moments, rendered captures), so no lower system ever reaches upward. Things find each other by key and by signal, never by compile-time reference. Rooms describe themselves in their own frames. Form alone turns drawings into solid pieces. Studio alone writes assets. Stages meet only at standard ports placed by one site plan. A connection that has to cross a seam (a stair, a lift) runs through a declared well, and the brushed-bronze strip of every doorway lies exactly on the seam between two stages.

## In plain words

Musée Vision's code is organised like the trades that build a real museum. Eleven shared systems each own one kind of truth:
- Foundation: the measures;
- Time: the clock, sun and weather;
- Matter: what things are made of, and how weather and wear mark them;
- Form: how they are cut and laid;
- Light;
- Sound;
- Motion: the machines;
- Nature: plants and water;
- Art: the collection and gallery furniture;
- Visitor: the body and the eye;
- Site: how rooms join.

Each system holds the instrument that measures its truth and the check that guards it. When something looks wrong, you measure, the measurement names which truth broke, and the fix goes into that one system. Every room then improves at once.

A stress test tried 42 situations against the design. The revision closes every gap it found:
- lamps now switch on at dusk and blinds close to protect paintings;
- stairs and lifts pass between rooms through declared wells;
- doors belong to the doorway;
- benches have a home;
- seasons change while you walk;
- puddles and snow follow one law;
- nothing in a lower system reaches up to a higher one.

Rooms and immersive works are "stages", as in SpaceX. Each is built and tested alone, with its doorways capped. They join only at standard doorways listed in one site plan, and one shared doorway detail is built at every join: stone running through, the bronze strip exactly on the seam, travertine around it. Adding a room means adding its folder and one line to the plan.

The museum is made in the editor and only lived in the game. Things that change while you visit (hour, weather, season, lamps) change by measured rules, never by hand. There are no brightness or exposure knobs a room could reach, so patches are impossible rather than merely forbidden.

Next: write every system's connectors, prove them on a travertine wall and an oak floor, then build only the Rotunda and review it.

## The eleven systems

### Foundation

*Module:* MuseeFoundation (Runtime). Its Build.cs also defines the shared MuseeModuleRules base.  
*Depends on:* Unreal Core, CoreUObject, Engine, Json (Foundation is its only user)

The standards every other system is built to: units, frames, identity, poses, the human measure, records, settings, the few contracts and signals that cross layers, and the diagnostics framework. Nothing more.

**Owns**

- Typed quantities in namespace Musee (Meters, Kilograms, Seconds, Degrees, Lux, Lumens, Candela, Nits, Kelvin, EV100, Decibels), and the one conversion from metres to engine centimetres, Musee::ToEngine
- Frames: FMuseeFrame, a local frame in metres. x is east, y is south and z is up, and plan north is true north
- Identity: FMuseeKey (lowercase, dot-scoped keys), deterministic seeds from keys (there is no unseeded randomness), and UMuseeRegistry, the runtime key registry. A key resolves to the scene component that carries it (a moving body, a water body, a luminaire), so systems attach to one another's things by key without seeing one another
- Poses: FMuseePose (key and transform in the stage frame) and FMuseeConfiguration (a named set of poses: one state of every mechanism in scope). This is the one currency for anything checked 'in every mechanism state'
- The human measure: FMuseeHumanMeasure holds only the body's constants: eye height 1.60 m, capsule radius and height, maximum step height, maximum walkable slope, minimum headroom, reach, guard height, and pace length 0.63 m. The formulas that use them live with their owners: Blondel's rule in Form, walking in Visitor, the reach agent in Site
- Records: the one reader that turns a Data/ JSON file into a typed USTRUCT by reflection. It is the only user of the engine's Json module. FMuseeText carries localisable strings as a key and a source string. Every read is added to a dependency set that Studio hashes
- Crossing contracts: IMuseeUsable (Usage() returns FMuseeUse: a verb, a caption, whether it is available and why not, and an optional seat frame; Use()) and IMuseeCaptioned (Caption() returns FMuseeCaption: title, body and credit, all FText)
- Signals on UMuseeSignals: FMuseeContact (a body struck a surface of a substance), FMuseeActuation (a mechanism moved with a speed and load) and FMuseeChannel (a named control point with a value, like a point in a building-automation system: one system publishes a demand, and whatever follows that key acts on it)
- Settings: FMuseeSetting. The owning system registers each setting once (time.hour, light.profile, visitor.comfort). The one settings store persists it, and registering it also creates its command musee.<system>.set-<setting>. Every setting has exactly one owner and one persisted copy
- The command registry (FMuseeCommand): a command is registered once, and the keyboard, the VR buttons, the console and Studio all call it
- Diagnostics: FMuseeCheck with a declared scope (Stage, Configuration, Museum, Moment, Capture) and typed inputs that the runner supplies; FMuseeFinding (measured value with its unit, expected range, location, owning system, and the system that drew the piece); IMuseeProbe and the registries; the log-category and stat-group macros; the defect-record format
- The collision channel and profile names: Solid, Walkable, Body (the visitor) and Clearance (queries). Form assigns them, and Motion, Visitor and Site query them
- MuseeModuleRules, the one C# base every Build.cs derives from. It sets the layer, PCH, warnings and OptimizeCode = Always for Musee game modules, so the DebugGame VR build runs optimised museum code
- The admission rule: Foundation grows only by adding a standard, and each addition gets the same review as a new system

**Does not own**

- Any geometry algorithm or drawing (Form)
- The clock, sky or weather (Time)
- Any rendering, material, light or sound
- Any knowledge of a particular stage or work
- Running checks (Studio) or showing measurements (Instruments)
- Any number that belongs to a substance, fixture, room or work
- Any formula over the human measure (each consumer owns its own)

**Connectors**

- `Musee::Meters / Lux / Lumens / Kelvin / EV100 / Decibels ..., Musee::ToEngine()`
- `FMuseeFrame, FMuseeKey, Musee::Seed(FMuseeKey)`
- `UMuseeRegistry: Register(Key, USceneComponent*), Find(Key)`
- `FMuseePose, FMuseeConfiguration`
- `FMuseeHumanMeasure`
- `Musee::ReadRecords<T>(Data/<System>/...), FMuseeText`
- `IMuseeUsable (Usage() -> FMuseeUse, Use()), IMuseeCaptioned (Caption() -> FMuseeCaption)`
- `UMuseeSignals: Publish/On Contact, Actuation, Channel`
- `FMuseeSetting: Register(key, default), Get, Set (the command is created automatically)`
- `FMuseeCommand: Register(name, fn), Run(name, args)`
- `MUSEE_CHECK("<System>.<Truth>", Scope, fn(inputs) -> TArray<FMuseeFinding>); MUSEE_PROBE("<System>::<Verb><Thing>", fn)`
- `Musee::Collision::Solid / Walkable / Body / Clearance`
- `MuseeModuleRules (C#) with Layer`

### Time

*Module:* MuseeTime (Runtime)  
*Depends on:* Foundation, Unreal HTTP (Time is its only user)

The world outside, told once: the clock, the calendar, the observer, the almanac, the dial and the real weather, broadcast as one museum Moment, plus the sky a sealed work may borrow.

**Owns**

- The one clock: UTC and the museum city's civil time with daylight saving. It registers the settings time.hour (live, or a fixed hour) and time.rate (ClockRate). Time is the only system that reads the real clock
- The museum observer. The museum's city is a making-time decision in Data/Time/Museum.json, recorded in Journal/Decisions.md. There is no runtime city command, because the dial and every made fact are cut for one latitude. Changing the city is a remake
- The sky observer (FTimeSky): what Light renders as sun, moon and sky. Normally it is the museum observer. Only Art's immersive-work base may push a scoped sky, and the push changes FTimeSky only. The museum Moment (history, phenology, weather ambience) never changes
- The calendar: civil date, the 24 solar terms, season, and the museum's opening hours (Data/Time/Hours.json), all carried in the Moment
- The almanac: a full-precision Meeus/NOAA ephemeris (about 0.01 degrees) for the sun and moon, moon phase, star-field rotation, refraction, the equation of time, sunrise and sunset
- Dialling (gnomonics): hour lines and the nodus shadow point for any latitude and dial plane
- The weather: one fetcher that asks Open-Meteo about every observer location in one request, then caches and smooths it. Climate normals exist for every city in Data/Time/Cities.json, so any pushed sky works offline. Time is the only user of HTTP
- Reference moments: the standard set every Moment-dependent check samples (solstices and equinoxes at 09:00, 12:00 and 15:00 local, dusk and night, under clear and overcast skies)
- FTimeMoment (museum) and FTimeSky, both immutable and broadcast through OnMoment and OnSky. Nobody polls
- MPC_Moment, the shader-visible museum Moment (rain history, snow, temperature, solar term, wind). Time is its only writer, and its parameter names are published in TimeMomentParameters.h

**Does not own**

- How the sun and sky light the museum (Light)
- Wetness, standing water and snow on surfaces (Matter's surface law)
- Rain and snow in the air, and bodies of water (Nature)
- The sound of rain (Sound)
- The sun clock's stone, bronze and cylinder (the Rotunda, through Form, Matter and Motion)
- When a work may borrow a sky (Art's immersive base and Site's light lock)
- Showing the hour (Visitor)

**Connectors**

- `UTimeSubsystem: Now() -> FTimeMoment, OnMoment; Sky() -> FTimeSky, OnSky`
- `Musee::Time::Sun(Utc, Observer), Moon(Utc, Observer): pure functions, also used at making time`
- `Musee::Time::Dial(Latitude, Nodus, DialPlane) -> hour lines and shadow point`
- `Musee::Time::ReferenceMoments()`
- `PushSky(FTimeObserver) / PopSky() (called only by UArtImmersiveWork)`
- `TimeMomentParameters.h (MPC_Moment parameter names)`
- `Settings time.hour and time.rate; command musee.time.advance-hour (the T key and the VR button)`
- Checks Time.EphemerisMatchesReference, Time.DialTrue, Time.NormalsForEveryCity, Time.SkyScopeClosed (every push is popped, and no push outlives its stage)

### Matter

*Module:* MuseeMatter (Runtime, default loading phase) + MuseeMatterShaders (Runtime, PostConfigInit, depends only on Core, RenderCore and Projects: it maps /Project/Matter). Graphs/ Python is run by Studio.  
*Depends on:* Foundation, Time (TimeMomentParameters.h only)

What every thing is made of: the real substance (how it formed), how it was worked (cut and finish), and what time and weather have done to its surface (history), before how it looks.

**Owns**

- Substance records in Data/Matter: travertine, cream marble, plaster, limewash, pearl steel, the one bronze alloy, oak, low-iron glass, opal diffuser glass, gilt, brick, iron, water, canvas, gesso, varnish, paper, silk, limestone, bark, leaf and grass. Each records how it formed, measured optics, physical properties, acoustic absorption by octave band, and stock rules (bed height, largest block, grain direction, board widths, pane sizes, and the bay size of a continuous finish such as plaster)
- Workings (sawn, honed, polished, brushed, patinated, gilt, mirror). An era changes the working, never the substance
- The surface law: one law for everything time and weather do to a surface. It covers age, traffic wear, touch, soiling (pore-scale from porosity, macro-scale from Form's cavity and wash facts), patina chemistry, and live wetness, standing water and snow depth. The live terms are MPC_Moment rain and snow history, times sky exposure, times porosity, times Form's ponding depth. It is written once in History.ush. Light's diffuser and Nature's foliage and ground-cover shaders include it, so wet grass and wet stone obey one law
- FMatterSpec = substance x working x history, and the one resolver from a spec to an engine material
- The piece-data layout (FMatterPieceLayout), sized to Nanite's attribute budget:
  - UV0: face UVs in piece-local metres, so there is no precision loss on 28 m walls;
  - UV1.x: PieceId;
  - vertex colour: cavity, wash exposure, arris proximity and one spare;
  - per stage, a piece table texture indexed by PieceId: seed, bed or grain frame, sky exposure, walkable, age;
  - per stage, plan-field textures in stage-local metres: traffic density and ponding depth;
  - Custom Primitive Data: the stage origin and table index
- The shading library (.ush files: Stone, Wood, Metal, Glass, Water, Cloth, History) mapped at /Project/Matter by the tiny MuseeMatterShaders module. Thin master graphs (opaque, metal, glass, water, subsurface, cloth) are generated by Python that holds no numbers. Masters have no emissive input
- UPhysicalMaterial per substance (friction, footfall class)
- Evidence tiers and the specimen-sheet metric. A spec is usable once its evidence passes:
  - (a) measured tables: albedo ranges, spectral IOR, complex F0 for metals, glass absorption per mm;
  - (b) ColorChecker photographs of physical specimens;
  - (c) museum photographs at known exposure.
  Metals and glass are calibrated mainly from (a). Matter owns the comparison maths, and Studio supplies the rendered capture

**Does not own**

- The shapes of pieces, joints and coursing, and the geometric facts it reads (cavity, wash, ponding, sky exposure), which are Form's
- Any emission (Light's diffuser master)
- Bodies of water and water in the air (Nature)
- The traffic field (Site)
- The image a canvas carries (Art) or a work's image content (the work)
- Rendering the specimen sheet (Studio assembles the capture on the stand under Light's standard luminaire)
- Writing assets (Studio runs the graph builders)

**Connectors**

- `Musee::Matter::Resolve(FMatterSpec) -> UMaterialInterface*`
- `UMatterSubstance (read-only, including Stock())`
- `FMatterWorking, FMatterHistory, FMatterSpec`
- `FMatterPieceLayout (where each piece attribute lives)`
- `/Project/Matter/*.ush, including History.ush (the surface law)`
- `Musee::Matter::Physical(substance) -> UPhysicalMaterial*`
- `Musee::Matter::CompareSheet(FMatterSheetCapture, Evidence) -> findings`
- Checks Matter.Plausible, Matter.Calibrated (Capture scope), Matter.OneLanguage (every non-emitting material on a piece traces to a substance record, and there is one bronze), Matter.NoEmission, Matter.PieceDataFits

### Form

*Module:* MuseeForm (Runtime: the drawing, the catalogue, the enclosures and the made facts) + MuseeFormMaking (Editor: the kernel; only Studio calls it)  
*Depends on:* Foundation, Matter, MuseeFormMaking only: GeometryCore, GeometryAlgorithms, DynamicMesh (Form is their only user)

How made things take shape. Form has two halves: the drawing, the architectural language every system describes shape in, and the making, the one kernel that cuts drawings into real pieces with real joints as sealed solids and writes the geometric facts others read.

**Owns**

- The drawing (runtime-safe data): FFormDrawing, a closed vocabulary of sweep, lathe, extrude, loft (sections along a spine), surface (B-spline patches for Chinese roof planes, the Élan sphere and the car body), laid surface, tubes along a curve tree, heightfield, and scan (an Evidence mesh with provenance, allowed only for Art's objects). The vocabulary grows only through Form's admission rule, reviewed like a new system
- The architectural language and its catalogue: moulding profiles, the orders, and named compositions in Data/Form (Profiles.json, Orders.json), named for what they are and never for a room (cornice.ionic-attic). The Rotunda's mouldings are written into this catalogue from day one, because the anchor defines the language. The promotion rule: a detail stays in one stage's Private plan only while it is singular, and a second use promotes it into Data/Form as a reviewed design change
- Walls with openings and reveals, niches, arches, vaults, domes, coffers, and stairs by Blondel's rule (2R + T = the pace length from the human measure)
- Openings: FFormOpening with its kind: Glazed (daylight through glazing), Open (unglazed to the sky: a courtyard, an open oculus), Port (a rough opening at a port) and Housing (a pocket). Light, Nature, Site and the leak probe all read this one list
- Enclosures: FFormEnclosure with its kind: Enclosed (a closed shell), Court (a shell whose top is one declared Open opening) or Outdoors (no shell, as for the Grounds). Studio casts them as UFormEnclosureComponent, and Musee::Form::EnclosureAt(point) is the one runtime query 'which enclosure am I in', used by Sound and Site
- Coursing rules within Matter's stock, and the chord tolerance, the one number that sets how finely every curve is divided
- The making (editor only): the one kernel (mesh builder, triangulator, sweep, lathe, extrude, loft, surface tessellation, tubes), MikkTSpace tangents from piece-local metre UVs, normals by one crease rule, and stations shared along every curve
- Setting-out: Lay() cuts surfaces into blocks, slabs, boards, panes, tiles and plaster bays within stock, with real joints and arrises
- Shells and solidity: every Enclosed or Court shell is closed and thick except at its declared openings. Collision comes from the same solids. Walkable faces are tagged from the human measure's slope and headroom. Fixed pieces never interpenetrate
- Made facts (FFormPieceFacts, FFormEnclosure):
  - volume per piece and body;
  - per enclosure: volume, area by finish, and the exposed surfaces (position, normal, area, substance);
  - per vertex: cavity and wash exposure;
  - per piece: sky exposure (hemispherical, independent of city);
  - per stage: the ponding depth field, and the free space for growth (FreeSpace)
- Granularity: pieces merge into one Nanite mesh per stage, rigid body and material family, never across moving bodies. Identical pieces are instanced. Glass and water are marked non-Nanite
- The leak probe and every solidity check are pure functions over FFormAssembly and FMuseeConfiguration inputs, so Form never needs to know Motion or Site

**Does not own**

- Substances, finishes and the surface law (Matter)
- Mechanism states and what moves (Motion supplies configurations)
- The assembly of stages (Site places them, and Studio builds FFormAssembly)
- Growth rules and erosion (Nature)
- Frame and fitting design (Art) and the doorcase (Site), which are drawn in Form's language
- Writing assets (Studio)
- The design of any stage

**Connectors**

- FFormDrawing (Sweep, Lathe, Extrude, Loft, Surface, Laid, Tubes, Heightfield, Scan), FFormProfile, FFormOrder, FFormWall, FFormOpening, FFormCoursing, FFormStair
- `UFormCatalogue (Data/Form profiles and orders, by key)`
- `FFormEnclosure, UFormEnclosureComponent, Musee::Form::EnclosureAt(Point)`
- `FFormPieceFacts, FFormExposedSurface, FFormAssembly (read-only)`
- Editor only: Musee::Form::Make(drawings, FMatterSpec) -> FFormPieces; Stations(curve, tolerance); Lay(surface, spec, coursing); FreeSpace(pieces, region); Ponding(pieces)
- `Probe Form::FindLeakPaths(FFormAssembly, FMuseeConfiguration, sky directions)`
- Checks Form.Sealed, Form.NoOverlap (Configuration and Museum scopes), Form.SharedStations, Form.MinThickness, Form.WithinStock, Form.StepsFitTheBody, Form.CollisionMatchesVisual, Form.NormalsContinuous

### Light

*Module:* MuseeLight (Runtime)  
*Depends on:* Foundation, Time, Matter, Form

All light in the museum, in real units and over time: the one sun, moon and sky, daylight through Form's openings and its control, every luminaire and how it is operated through the day, the lighting scheme that meets measured targets, and the renderer's transport.

**Owns**

- The sun, moon and sky as rendered, from Time's FTimeSky: illuminance in lux, colour, SkyAtmosphere, volumetric clouds and fog. There is exactly one of each
- Daylight: which Glazed and Open openings admit it, and daylight coefficients per target (the Tregenza sky-patch method, precomputed at commissioning by the path tracer). At runtime, the illuminance at each target is predicted cheaply from the current sky
- Daylight control devices from the catalogue (roller blinds with a real openness factor, louvres, UV-filtering film). Light's operation publishes their demand on FMuseeChannel '<opening key>.shade'. The blind's movement is a Motion mechanism that the stage declares to follow that channel
- The luminaire catalogue in Data/Light: real products with lumens, beam, CCT, CRI, IES file, housing drawing and control class (gallery, circulation, facade, lantern, image). ULightLuminaireComponent drives the light and its matched diffuser together, so they cannot disagree
- The one emissive master, M_Light_Diffuser. It shades the diffuser's substance through Matter's .ush library and adds emission in nits computed from the luminaire record. It is the only emitting material in the museum
- Image luminaires: a luminaire kind for projectors and LED walls, with measured peak luminance. Its per-frame image comes from an ILightImageSource, which a work implements
- Use targets for architecture (circulation, orientation, night lighting) by use class, from lighting standards. Care limits for works are Art's and arrive as FLightTarget
- Scheme: Scheme(targets, mounts) chooses catalogue luminaires by rule (beam to cover the target, aimed at about 30 degrees, glare geometry) using the analytic direct model. It is a small rule set, not a global optimiser
- Commissioning: at making time Studio renders each stage on the stand with the path tracer at the reference moments. Commission() sets dimming levels and daylight coefficients from those renders, as a real lighting designer commissions with a lux meter
- Operation: the museum-wide control rules in Data/Light/Operation.json. They cover switching by opening hours and solar altitude (lamps at dusk, lanterns, facade at night), daylight-linked dimming that holds each target, and shading demand that holds care limits
- Transport: Lumen with hardware ray tracing, MegaLights, ray-traced shadows and translucency, the renderer section of DefaultEngine.ini, and one unbound post-process volume that sets only transport fields (GI and reflection method, Lumen quality, translucency). The render profiles change sampling, never calibration
- Photometric probes: illuminance by a calibrated grey card (radiance read with the renderer's pre-exposure removed, never through the Eye), luminance, the path-traced reference, the source list and the capped-openings render
- Private knobs: the sun, transport and raw light components are Private

**Does not own**

- Exposure, white balance, tone and lens (Visitor's Eye owns the camera fields)
- Opening geometry and shell sealing (Form)
- Sun position, weather and opening hours (Time)
- Glass, diffuser and film substances (Matter)
- Care limits of works (Art)
- A work's image content (the work)
- Moving a blind (Motion follows the channel)

**Connectors**

- `ULightSubsystem (sun, moon and sky from OnSky)`
- `ULightLuminaire (record), ULightLuminaireComponent`
- FLightTarget{surface, lux range, CCT, uniformity, annual dose}; Musee::Light::Scheme(targets, mounts) -> placements; Musee::Light::Commission(captures) -> dimming and daylight coefficients
- `Musee::Light::PredictIlluminance(target, FTimeSky)`
- `ILightImageSource`
- `Channel '<opening key>.shade' (published)`
- `Setting light.profile <desktop-4k|headset>`
- Probes Light::MeasureIlluminance, Light::MeasureLuminance, Light::MeasureReferenceIlluminance, Light::ListSources, Light::FindTransportLeaks
- Checks Light.OneSunOneSky, Light.LuminairesFromCatalogue, Light.EmissiveHasEmitter, Light.MeetsTarget (Moment scope: every reference moment), Light.NoGlare, Light.MeterCalibrated, Light.OneTransport (only transport fields, only in Light's volume), Light.Operated (every luminaire has a control class)

### Sound

*Module:* MuseeSound (Runtime)  
*Depends on:* Foundation, Time, Matter, Form, Unreal AudioMixer, MetasoundEngine (Sound is their only user)

Sound as acoustics: sources at real levels, and each enclosure's reverberation derived from its volume and the absorption of its substances, never tuned by ear.

**Owns**

- Emitters: recorded evidence at measured levels (dB SPL at 1 m) with directivity
- Footfalls by substance, from FMuseeContact
- Mechanism voices from FMuseeActuation, attached to the moving body by key through UMuseeRegistry
- Weather ambience from the museum Moment: rain placed on Form's exposed surfaces by substance (glass, stone, water, lead), and wind
- Room acoustics: RT60 per octave by Sabine or Eyring from FFormEnclosure and Matter's absorption, applied for the enclosure that Form::EnclosureAt returns for the listener
- Occlusion by sealed shells and passage through joints and Open openings
- The mix and its one loudness reference

**Does not own**

- The listener (Visitor's Ear)
- The events that make sound
- Absorption coefficients (Matter)
- Enclosure volumes and surfaces (Form's made facts)
- A work's soundtrack (the work)

**Connectors**

- `USoundEmitterComponent (recording, level, directivity)`
- `Musee::Sound::Acoustics(FFormEnclosure) -> reverb settings`
- `Probe Sound::MeasureReverberationTime`
- `Checks Sound.ReverberationFromMatter, Sound.Occluded`

### Motion

*Module:* MuseeMotion (Runtime)  
*Depends on:* Foundation, Matter, Form

Everything that moves, built as a real machine: bodies, joints, drives, counterweights, housings, travel volumes, clearances and interlocks. Motion also owns the state space every other check samples.

**Owns**

- UMotionMechanism: rigid bodies made of a stage's (or joint's) pieces; revolute, prismatic and screw joints; drives with real speed, acceleration and jerk limits (about 1 m/s2 and 1-1.5 m/s3)
- Counterweights and balance, with masses from Form's volumes times Matter's densities
- Machine elements (involute gears, rack and pinion, rails), returned as Form drawings
- Named states, sequences and controls implementing IMuseeUsable, with verbs and captions as FText serialised in the spec
- The state space: each mechanism's named states and sampled phases, grouped into coupling groups (mechanisms whose travel volumes intersect or that are interlocked). Musee::Motion::Configurations() returns FMuseeConfigurations, and only coupled groups are multiplied, so the count grows with group size, not 2^N
- Travel volumes (FMotionTravel), clearances and housings
- Interlocks by key (UMotionInterlock). They also bind across a seam when Site supplies the binding (the Élan car at the Cube landing and the landing door)
- Following channels: a mechanism may declare that it follows an FMuseeChannel key (a blind follows '<opening>.shade')
- Kinematic motion only, deterministic, with no Chaos simulation
- Registering every moving body in UMuseeRegistry by its key, and publishing FMuseeActuation

**Does not own**

- Shapes of bodies (Form)
- Water in a moving basin (Nature attaches by key)
- Machine sound (Sound)
- Composing configurations across stages, and reachability (Site)
- Which mechanisms a stage or joint has (the stage's description or Site's joint)
- Why a blind should close (Light's operation)

**Connectors**

- UMotionMechanism, FMotionBody, FMotionJoint, FMotionDrive, FMotionCounterweight, FMotionTravel, FMotionMechanismSpec (with FollowsChannel and CouplingGroup)
- `MoveTo(State), State(), OnMoved`
- `Musee::Motion::Configurations(mechanisms) -> TArray<FMuseeConfiguration>`
- `UMotionInterlock (by keys)`
- `Musee::Motion::Gear / Rack / Counterweight -> FFormDrawing`
- `Probe Motion::MeasureClearance`
- `Checks Motion.Clears (at least 5 mm at every sampled phase), Motion.Housed, Motion.Balanced, Motion.Comfortable, Motion.Interlocked`

### Nature

*Module:* MuseeNature (Runtime)  
*Depends on:* Foundation, Time, Matter, Form, Unreal Niagara (Nature is its only user)

Everything grown or formed by natural process rather than cut by a builder: plants, bodies of water, precipitation in the air, rock and ground.

**Owns**

- Plants grown by botanical rules. Species records in Data/Nature hold phyllotaxis, branching, the pipe model, leaf, bark, phenology by solar term and wind response. Grow() is space-aware (space colonisation, with attraction points only inside the growth space) and is seeded by key
- The growth space (FNatureGrowthSpace): Form's free space for the planting bed, shrunk by the species' maximum sway at the design wind, so neither the tree nor its canopy in wind can reach a wall
- Phenology at runtime: Grow returns a structure drawing (trunk and branches, made once) plus leaf, flower and fruit instances with seeded phenology curves. MPC_Nature (written only by Nature from the Moment's solar term and wind) drives leaf-out, blossom, colour and fall, so a long session moves through the seasons without remaking
- Bodies of water (UNatureWaterComponent): a basin found by body key through UMuseeRegistry, a level set by gravity with volume conserved, waves, ripples and the canal's flow
- Precipitation in the air (rain and snow, Niagara). It stops at shells, falls through Open openings and courts, and is driven by the museum Moment
- Rock formed by process (Taihu stones by dissolution along limestone joints)
- Ground: terrain as a heightfield drawing made into a Nanite mesh (no Landscape), and ground covers (lawn, gravel, meadow) whose wetness and snow come from Matter's History.ush

**Does not own**

- Substances (Matter)
- Any surface state: wetness, puddles and standing water, snow on pieces and on ground covers (Matter's surface law)
- Basins, paving and walls (Form, drawn by the stage)
- Lifting the pond (Motion)
- The weather (Time)
- Free space (Form's made fact)
- Garden layout (the stage)

**Connectors**

- `UNatureSpecies; Musee::Nature::Grow(species, seed, age, FNatureGrowthSpace) -> FNaturePlant{structure drawing, instances with phenology}`
- `UNatureWaterComponent(basin body key, volume)`
- `UNaturePrecipitationSubsystem`
- `MPC_Nature parameter names`
- `Musee::Nature::Rock(process, substance, seed), Ground(footprints, surface)`
- Checks Nature.WaterContained (Configuration scope), Nature.InSeason (every solar term), Nature.Rooted, Nature.WithinBotany, Nature.WithinGrowthSpace

### Art

*Module:* MuseeArt (Runtime)  
*Depends on:* Foundation, Time, Matter, Form, Light

The collection as real objects and the exhibition around it: each work at its real size on its real support, framed, cased or mounted, hung by curatorial rules, lit within its care limits, and the gallery fittings visitors use.

**Owns**

- The catalogue, Data/Art/Collection.json: real dimensions, medium, support, date, holding collection, credit and licence, colour profile and care class. Captions are FMuseeText
- Care, Data/Art/Care.json, the museum's only lux-by-medium numbers: limits for lux, kelvin and UV, and an annual dose in lux-hours (oil 150-200 lux; paper and silk 50 lux). They are sent to Light as FLightTarget
- The work as an object in Form's language: stretched canvas with tacking edges, ground and varnish; panels; paper in a mat; silk scrolls with mounts and rollers. Sculpture and ceramics are Form scan drawings made from Evidence, with provenance
- Frames, vitrines, cases and plinths
- Fittings, Data/Art/Fittings.json: benches, stanchions and barrier ropes, the visitor desk, wall labels and wayfinding signs, as real products in the one material language. Seats implement IMuseeUsable with a seat frame. Sign text comes from Site's plan at detailing
- The Hang, Data/Art/Hang.json (work to anchor, the centre line from the human measure, spacing), and the anchor kinds Wall, CaseBed, Plinth, Space, Bench, Barrier and Sign
- Colour-managed images as the albedo of the paint layer
- UArtImmersiveWork, the base of every immersive work. It performs only while the visitor is inside the work's sealed Space and pushes Time's sky observer only while the joint's light lock is closed (read from FMuseeChannel '<port>.lock'). It pops before the outer door may open, so works cannot get the scope wrong

**Does not own**

- Substances (Matter)
- Profiles and the kernel (Form)
- Luminaires, aim and operation (Light)
- Stage walls and anchors (the stages)
- Showing a caption (Visitor)
- The content of an immersive work (the work)
- Wayfinding text (Site's plan)

**Connectors**

- `UArtWork, UArtFitting`
- `Musee::Art::Object(work or fitting) -> drawings + FMatterSpecs; Musee::Art::Targets(work) -> FLightTarget`
- `FArtHang, keyed by anchor; anchor kinds`
- `UArtImmersiveWork`
- Checks Art.TrueSize, Art.WithinCare (Moment scope, and the annual dose), Art.ColorManaged, Art.Credited, Art.HangResolves, Art.ImmersiveScoped

### Visitor

*Module:* MuseeVisitor (Runtime) + MuseeVisitorScreen (Runtime) + MuseeVisitorHeadset (Runtime), all in every game target, one active  
*Depends on:* Foundation only, Unreal EnhancedInput, UMG/Slate, Streamline (Visitor is their only user), VisitorHeadset adds OpenXR and HeadMountedDisplay (its only user)

The person in the museum: a body of human size that walks and sits, an eye that exposes like a camera, an ear, hands for screen or headset, and their overlay.

**Owns**

- AVisitorBody: the capsule, step height and walkable slope, all read from FMuseeHumanMeasure and never set locally; walking, stairs, gravity, riding moving floors, and sitting at a seat frame
- Intents: walk, step-to (the VR arc), turn, look and sit
- The Eye: the one camera model. It uses reflected metering of scene luminance with one museum-wide calibration constant (ISO 2720, K = 12.5), human adaptation speeds, white balance, tone curve and lens. It owns one unbound post-process volume that sets only camera fields
- The Ear: the listener
- The Hands: gaze or beam targeting of IMuseeUsable and IMuseeCaptioned
- The overlay, composited after tone mapping (stereo layers in VR): hint, caption, the settings card (it lists Foundation's registered settings) and Instruments' panels
- The adapters VisitorScreen and VisitorHeadset. Both are compiled into every game build. Exactly one is active, chosen at start: Headset when an OpenXR session is running, otherwise Screen
- Registering visitor.comfort and visitor.sampling, and publishing FMuseeContact per footfall

**Does not own**

- What is used (each IMuseeUsable owner)
- Caption text (Art)
- Light levels, transport fields and their volume (Light)
- Reachability (Site)
- The hour and render profile (Time's and Light's settings; Visitor only shows them)
- Ride comfort (Motion)

**Connectors**

- `AVisitorBody`
- `UVisitorEye::EV100() (registered as the probe Visitor::MeasureExposure)`
- `UVisitorOverlay::Show(FMuseeCaption)`
- `IVisitorAdapter`
- `Input -> FMuseeCommand bindings`
- Checks Visitor.OneEye (camera fields exist only in the Eye's volume), Visitor.ActionsCovered, Visitor.OverlayOutsideScene, Visitor.BodyFromMeasure, Visitor.OneAdapterActive

### Site

*Module:* MuseeSite (Runtime, the primary game module)  
*Depends on:* Foundation, Time, Matter, Form, Light, Sound, Motion, Nature, Art, Visitor, Unreal NavigationSystem (Site is its only user)

The museum's integration standard: the stage contract and the port (with its well), the one site plan that mates stages, the one joint built at every seam (with its doors), the museum's configurations, traffic and reachability, and the running visit (which stage you are in, and what is loaded).

**Owns**

- USiteRoom, the contract every room fulfils: a local frame, ports, footprint, anchors, and Describe(FSiteDescription&). The description has no method for exposure, intensity, fill light, finish override, world position or another room
- FSitePort: its kind (Doorway, Stairwell, Shaft, Gate) and class key; a frame on the outer face at the threshold, facing out; the wall depth; the face shape (flat, or cylindrical with radius R); and for vertical ports a well. The serving side owns what is in the well (the stair, the lift car). The receiving side reserves the same prism as a void and builds nothing in it
- Port classes in Data/Site/PortClasses.json: size, rough-opening allowance, collar envelope, and closure (open portal, pair of doors, gate, light lock, landing door)
- Footprints with a kind: Building (must not overlap another Building) or Ground (exactly one; it must contain every Building). The rule is by kind, never by room name
- Data/Site/SitePlan.json: the anchor stage, the matings with passage lengths, and the arrival point
- Mate(), pass 1 (ports only): translation-only solving from the anchor, then kind, class, level, facing, well coincidence, loop closure within 1 mm, and no overlaps between Buildings
- Detail(description), pass 2: turns a stage's description into its drawings and living specs. It asks Art for objects and fittings at anchors, Light for the scheme, and Nature for plantings, and gives the Ground stage the placed Building footprints
- The joint: Joint(A, B) returns FSiteJoint = drawings, mechanism specs and interlock bindings:
  - the travertine threshold through both wall depths;
  - the bronze strip on the mating plane;
  - a lining set in front of both rough faces;
  - surrounds on both faces;
  - passage masonry that closes the whole gap between two faces within the collar envelope (the wedges where a flat face meets the drum);
  - door leaves, a light lock or a landing door as Motion mechanisms;
  - well collars with the bronze ring on the mating plane.
  There is a blind for every unmated port, and on the stand a blind well that continues an unmated well to its declared depth, so the serving mechanism runs its full travel
- Traffic: Traffic(stage) routes between the stage's ports, anchors and seats over walkable faces and rasterises a density field in the stage's plan. Each stage is computed alone, so it stays self-contained
- Configurations of the assembly: each stage's configurations from Motion, combined only where a coupling group touches a port or a joint mechanism
- Reachability, factored: per stage and per configuration, a port-to-port connectivity table for the human measure's body (NavigationSystem agent built from FMuseeHumanMeasure), composed over the site graph
- USiteStageInfo, written by Studio into each stage level: key, display name (FText) and enclosures
- At play time: RoomAt (Form::EnclosureAt, then the stage), stage entered and left, the GameMode, arrival, and residency (streaming). The current stage, the stages visible through open joints (a visibility graph made at making time) and the Ground stage (when the current stage has an exterior opening) are loaded, and the rest unloaded

**Does not own**

- The design of any stage
- Profiles and substances (Form, Matter)
- What is hung, and fittings (Art)
- The body and eye (Visitor)
- Mechanism physics and interlock logic (Motion)
- Writing the map, joints or stage info (Studio)

**Connectors**

- `USiteRoom::Describe(FSiteDescription&)`
- `FSitePort, FSiteWell, FSiteFootprint, port classes`
- `USitePlan; Musee::Site::Mate(); Musee::Site::Detail(FSiteDescription) -> FSiteDetails`
- `Musee::Site::Joint(A, B) / Blind(port) / BlindWell(port) -> FSiteJoint`
- `Musee::Site::Traffic(stage) -> plan field`
- `Musee::Site::Configurations(assembly)`
- `USiteStageInfo`
- `USiteSubsystem: RoomAt(), Arrival(), OnStageEntered/Left, residency`
- `Probe Site::FindUnreachable`
- Checks Site.PortsMate, Site.RoomStaysInside (the footprint plus serving wells, minus receiving wells), Site.LoopsClose, Site.NoOverlap, Site.OneGround, Site.Reachable, Site.ReachWalked (the perf tour's real body walks every route the analysis calls reachable), Site.PlanResolves, Site.MatchesCanvas, Site.ResidencyFits

## Beside the museum

### Studio

*Module:* MuseeStudio (Editor)  
*Depends on:* Every module, Unreal UnrealEd, MeshDescription, StaticMeshDescription, MeshConversion, AssetTools, LevelInstanceEditor, Localization, PythonScriptPlugin (Studio is their only user)

The workshop beside the museum (editor only). It runs the one make pipeline, assembles the context every check needs, runs the checks, measures performance and packages builds. It owns no truth of its own.

**Owns**

- The make pipeline, a fixed order:
  1. describe (rooms and works' making modules);
  2. mate (Site pass 1);
  3. detail (Site pass 2);
  4. make the fabric (FormMaking: pieces, made facts, enclosures, ponding, per-vertex fields, free space);
  5. derive (Site traffic, Motion configurations, growth spaces);
  6. grow (Nature) and make the living drawings;
  7. cast;
  8. commission (Light, path-traced on the stand);
  9. write levels and assemble;
  10. check.
  Every rule belongs to its owner
- The caster: FFormPieces -> MeshDescription -> UStaticMesh. It turns Nanite on, keeps Form's normals, uses MikkTSpace tangents from UVs, writes no lightmap UVs, takes collision from Form, and casts glass and water non-Nanite. It also writes each stage's piece table, plan-field textures, UFormEnclosureComponents, USiteStageInfo and the commissioning data
- Check contexts: FFormAssembly from Site's placements, configurations from Motion and Site, Time's reference moments, and captures on the stand (specimen sheets under Light's standard luminaire, grey cards, the path tracer). Lower systems' checks receive these as inputs and never fetch them
- Incremental making: the hash of each output is its generator code version plus every record read through Foundation's reader (the dependency set). A change to PortClasses, a substance or a luminaire therefore recasts exactly what used it. Batches run under a memory cap, and Nanite builds are cached in the DDC. Studio.Incremental: a second make writes zero packages
- A make report broken down by step and by owner (time, piece counts, memory)
- Running Matter's Python graph builders
- L_Museum: a persistent level that streams each stage level at its mated translation, plus L_Joints
- The stand, L_Stand: one stage at its true orientation, with blinds and blind wells, the real sky at reference moments, the same Eye and the same instruments
- The check runner. A fast set runs on every make; a deep set (leak rays over every sky direction and configuration, path-traced targets, reachability) runs before packaging. Each stage runs alone, then the museum, and one report is written. A failing check stops packaging
- Conventions (Studio.Wiring):
  - every Build.cs edge points down;
  - each engine capability has one owner;
  - no stage names another stage;
  - no room name appears in shared code or shared data;
  - no asset path appears outside its owner;
  - a token-based banned-word lint;
  - Python holds no numbers.
  It generates Docs/Wiring.md and Docs/FrameBudget.md
- Studio.JournalNamesChecks
- Evidence import: colour-managed paintings, IES files, recordings, scan meshes for Art, each with credit and a licence check
- Localisation: gathering FMuseeText from Data/ JSON into string tables, plus the engine's gather of FText serialised in cooked specs
- The performance tour: a walk derived from the site plan's ports, with the real AVisitorBody (which also feeds Site.ReachWalked). It measures GPU time per render pass through the RDG GPU profiler, maps passes to owners using each README's declared passes, and uses show-flag ablation to attribute the shared passes
- Packaging dated desktop and VR builds. It refuses to start while an Unreal process is running

**Does not own**

- Any truth it makes, checks or measures
- Any composition of knowledge (Site's Detail and Form's made facts)
- Runtime behaviour

**Connectors**

- `musee.studio.make <stage|matter|site|all> (inside the open editor, with Live Coding)`
- `musee.studio.check <System.Truth|fast|deep>`
- `musee.studio.perf, musee.studio.package <desktop|vr>`
- `-run=MuseeStudio commandlet (unattended, editor closed)`
- `Scripts/make.ps1, check.ps1, perf.ps1, package.ps1 (thin entry points)`
- `Checks Studio.Wiring, Studio.Incremental, Studio.CastRules, Studio.JournalNamesChecks`
- `Nothing in the museum depends on Studio`

### Instruments

*Module:* MuseeInstruments (Runtime; TargetConfigurationDenyList Shipping in the .uproject)  
*Depends on:* Foundation, Visitor

The case of measuring instruments inside the running museum (editor play, Development, and the DebugGame headset build; never Shipping). It shows each owner's measurements where the visitor stands.

**Owns**

- The lux meter (Light::MeasureIlluminance at the reticle, next to the target returned with the reading)
- Luminance false colour (absolute nits, log scale) and the live EV100 from the Eye
- The leak view (Form's leak paths and Light's capped-openings render)
- The source list (every source with its catalogue entry, control state, and measured against predicted lux)
- The clearance view (Motion), the reachability map (Site), the reverberation readout (Sound), and the traffic and ponding fields
- Frame time by render pass against each owner's budget line
- The check-findings overlay

**Does not own**

- The physics of any measurement (every probe is its owner's)
- Any fix

**Connectors**

- `musee.instruments.show <lux|false-color|exposure|leaks|sources|clearance|reach|reverberation|fields|frame|findings>`
- `Reads Foundation's probe and check registries and draws through Visitor's overlay`

## The stages: rooms and works

**Stages are not systems.** They are the payload the kit carries, each in its own Unreal module and its own true-north local frame, testable alone on the stand and never aware of another stage. They live under Source/Stages/.

**Rooms** (Source/Stages/Rooms/<Room>/, module Musee<Room>, Editor type): Rotunda, Salon, Reserve, ClassicalHall, HallOfLight, Atrium, Elan, Cube, Chenghuai, Albion and Grounds.
- A room is a description, not a program. It has one class, U<Room>Room : USiteRoom, and Describe(FSiteDescription&) is a plain, deterministic function behind a thin UCLASS shell (Live Coding can patch it).
- It declares, in its own frame and in metres:
  - **ports**, including every doorway it will ever have, from day one. The Rotunda declares west, north, south and entrance, and a stairwell whose well it serves;
  - a **footprint** with its kind (Building, or Ground for the Grounds), and its **anchors** (art, fittings, the Cube's Space). These are in Public/<Room>Room.h, its only public header;
  - **enclosures** with their kind: Enclosed; Court (Chenghuai's three courts, Albion's court if it is open, the Rotunda if the owner leaves the oculus open); or Outdoors (the Grounds);
  - **pieces** as Form drawings with Matter specs, using catalogue profiles from Data/Form, and the sun-clock marks laid from Time::Dial;
  - **openings**: Glazed with glazing and an optional daylight control, Open to the sky, Port rough openings, and Housings;
  - **light targets** by use and **luminaire mounts**;
  - **mechanisms** (Motion specs whose bodies are its own pieces, including blinds that follow a shade channel);
  - **water, plantings and sound emitters**.
- Its plan numbers live only in Private/<Room>Plan.h. A detail becomes shared by promotion into Data/Form, never by including another room.
- **Wells:** a vertical connection belongs to exactly one room, the serving side, and runs through the well its port declares. The Rotunda serves the spiral stair into the Classical Hall's receiving stairwell. Élan serves the lift into the Cube's receiving shaft. The receiving room reserves the well as a void and builds nothing inside it.
- A room has no play-time code. Everything that lives is a kit component configured by a serialised spec. A behaviour no kit component offers is a missing capability in its owner, never room code.
- Each room owns both faces of its own shell.

**Works** (Source/Stages/Works/<Work>/): SeaLight and future immersive works. A work has the same two phases as Form:
- **Musee<Work>** (Runtime, performs): derives from Art's UArtImmersiveWork and is found by key through class registration. Hang.json binds sea-light to cube.space.
  - Its luminous content is image luminaires (Light's kind) that implement ILightImageSource, inside the Cube's sealed shell.
  - Any content material a work renders (a light-field or sea shader) lives under /Game/Generated/Works/<Work>/ and only ever feeds an ILightImageSource render target. It is never assigned to a piece, so the one material language holds.
  - The base class pushes the sky observer only while the Cube's light lock is closed.
- **Musee<Work>Making** (Editor, describes): the work's geometry and generated content, described as Form drawings and data that Studio makes and casts. It never calls FormMaking itself.
- A work depends on Art, the kit and Site, never on a room.

**Every stage module has the same shape:** a Build.cs, a one-page README with the standard headings, Public/ (the declaration header only), Private/ (plan, parts, tests), and its own checks registered with Foundation.

## The wiring

```
 BESIDE    Studio (Editor) ................ sees everything; nothing sees it
           Instruments (dev runtime) ...... sees Foundation (probes) + Visitor (overlay); nothing sees it
 ======================================================================================================
 STAGES    Rooms/  Rotunda Salon Reserve ClassicalHall HallOfLight Atrium   (Editor: describe) -> Site + kit
                   Elan Cube Chenghuai Albion Grounds
           Works/  SeaLight (Runtime: perform) -> Art + kit + Site
                   SeaLightMaking (Editor: describe) -> SeaLight + Site + kit
           a stage never sees another stage; wells and joints connect them through Site
 ------------------------------------------------------------------------------------------------------
 L6  SITE      contract . ports+wells . plan . mate . detail . joints . traffic . reach . visit . residency
 L5  ART       catalogue . care . objects . fittings . hang . immersive base      -> Light, Form, Matter, Time
 L4  LIGHT     sky . daylight . luminaires . scheme . commission . operation . transport  \
     SOUND     sources . acoustics . occlusion . mix                                     |  peers: never see
     MOTION    mechanisms . state space . travel . interlocks                             |  each other; meet by
     NATURE    plants . phenology . water bodies . precipitation . rock . ground         /   signal, channel, key
 L3  FORM      drawing . catalogue . openings . enclosures . made facts . EnclosureAt  -> Matter
       '-- FormMaking (Editor): kernel . stations . setting-out . sealing . fields   [called only by Studio]
 L2  MATTER    substance x working x history . surface law . piece layout            -> Time
       '-- MatterShaders (PostConfigInit): maps /Project/Matter              -> Core, RenderCore, Projects only
 L1  TIME      clock . calendar . observer . sky scope . almanac . dial . weather     -> Foundation
 L0  FOUNDATION units . frames . keys+registry . poses . human measure . records . settings
                . contracts . signals+channels . commands . diagnostics
     VISITOR   body . eye . ear . hands . overlay  (side column: -> Foundation only)
       |-- VisitorScreen    '-- VisitorHeadset   (both in every game build; one active)
```

**Exact edges** (what each Build.cs lists; Foundation is reachable from everything):
- Time -> Foundation (+ HTTP)
- MatterShaders -> (Core, RenderCore, Projects)
- Matter -> Time
- Form -> Matter
- FormMaking -> Form (+ GeometryCore, GeometryAlgorithms, DynamicMesh)
- Light -> Form, Matter, Time
- Sound -> Form, Matter, Time (+ AudioMixer, MetasoundEngine)
- Motion -> Form, Matter
- Nature -> Form, Matter, Time (+ Niagara)
- Art -> Light, Form, Matter, Time
- Visitor -> Foundation (+ EnhancedInput, UMG, Streamline); VisitorScreen -> Visitor; VisitorHeadset -> Visitor (+ OpenXR, HeadMountedDisplay)
- Site -> Art, Light, Sound, Motion, Nature, Form, Matter, Time, Visitor (+ NavigationSystem)
- each Room -> Site + the kit it uses (never FormMaking, Visitor, Studio or another stage)
- each Work -> Art + kit + Site; each WorkMaking -> its Work + Site + kit (never FormMaking or a room)
- Instruments -> Foundation, Visitor
- Studio -> everything (+ UnrealEd, MeshDescription, StaticMeshDescription, MeshConversion, AssetTools, LevelInstanceEditor, Localization, PythonScriptPlugin)

**The rules**
1. **Arrows point down only.** Each Build.cs declares its layer through MuseeModuleRules. Peers (Light, Sound, Motion, Nature) never see each other. Visitor sees only Foundation.
2. **Two phases, enforced by module type.**
   - Making modules (FormMaking, rooms, WorkMaking, Studio) are Editor modules, so no game build can link geometry generation.
   - Anything cut at making time may depend only on making-time decisions: the museum city, the plan, the records.
   - Whatever changes while the museum runs changes through runtime parameters: MPC_Moment, MPC_Nature, dimming, channels and mechanism states. Examples are the season, wetness, lamps at dusk and blinds.
3. **Checks never reach up.** A check is pure maths in its owner over typed inputs: FFormAssembly, FMuseeConfiguration, reference moments and captures. Studio assembles those inputs. Motion owns the state space, and Site composes it across seams.
4. **News travels up, or across between peers, only through Foundation:**
   - signals (Contact, Actuation);
   - channels (Light's shade demand, which a Motion blind follows; a light-lock state, which Art's immersive base reads);
   - the key registry (water attaches to a basin body; sound attaches to a moving body);
   - settings and commands.
   A channel's key is declared by the stage that owns both ends, so no system names another.
5. **Each engine capability has one owner:**
   - HTTP: Time
   - Json: Foundation
   - GeometryCore and DynamicMesh: FormMaking
   - AudioMixer and MetaSound: Sound
   - Niagara: Nature
   - EnhancedInput, UMG and Streamline: Visitor
   - OpenXR: VisitorHeadset
   - NavigationSystem: Site
   - editor APIs, Localization and Python: Studio
   - ProceduralMeshComponent, Landscape and the Water plugin: nowhere
6. **Public/ is the connector, and Private/ is the inside.** The sun, transport, raw light components and the Eye's metering are Private. Post-process fields are split disjointly: camera fields in the Eye's volume, transport fields in Light's. No other post-process volume may exist (Visitor.OneEye, Light.OneTransport).
7. **The diagram cannot lie.** Docs/Wiring.md is generated from the Build.cs files. Stages find each other only through data (SitePlan.json and Hang.json keys) and by class registration.

## How rooms join

**A room is a stage.** It lives in its own Editor module and derives from USiteRoom. It knows nothing of other rooms or of where it stands.

**Its frame.** A room is authored in its own local frame, in metres: x east, y south, z up, aligned to true north. Its origin is a meaningful datum (for the Rotunda, the drum's centre on the finished floor). Site only translates and never rotates, so:
- the stand shows the real sun from the real direction;
- mating cannot drift.

**Its declared interface** (Public/<Room>Room.h, reviewed before any design code):
- **Ports.** Each has a kind (Doorway, Stairwell, Shaft, Gate) and a class key from PortClasses.json, for example doorway.3000x4500.arched, stairwell.spiral.3600 or shaft.lift.2400x2400. Its frame sits on the outer face at the opening centreline and threshold level, facing out, with the wall depth and the face shape (flat, or cylindrical with radius R).
  - A vertical port also carries a well: a prism with a serving side (which owns its contents) and a receiving side (which reserves it as a void).
  - The anchor stage declares every port it will ever need on day one.
- **A footprint** with its kind: Building, or Ground (exactly one stage, the Grounds).
- **Anchors:** Wall, CaseBed, Plinth, Space, Bench, Barrier and Sign.

**Its description.** Describe(FSiteDescription&) declares:
- enclosures with their kind (Enclosed, Court, Outdoors);
- pieces;
- openings (Glazed, Open, Port, Housing);
- targets and mounts;
- mechanisms (including blinds following '<opening>.shade');
- water, plantings and emitters.

**The rough-opening rule.** At a port the room builds only the structural opening: the clear opening plus the class allowance. It builds nothing past its port face. Site.RoomStaysInside checks every piece against the footprint volume, plus the room's serving wells, minus its receiving wells.

**Testing alone, on the stand (L_Stand).** Every port gets a blind. Every unmated well gets a blind well that continues it to its declared depth, so the Rotunda's stair and Élan's lift run their full travel alone. The real sky runs at reference moments. There the stage must pass every check:
- Form.Sealed over each enclosure (Court enclosures open only at their declared Open opening), in every configuration Motion supplies;
- its lux targets met across reference moments after commissioning;
- its mechanisms clear, housed, balanced, comfortable and interlocked;
- every standable place reaching a port in every configuration;
- its traffic field computed from its own ports, anchors and seats.

**Joining: two passes.**
1. **Mate (ports only).** SitePlan.json is a mating graph with passage lengths (0 m where the walls touch), the anchor and the arrival point:
   - rotunda.west <-> salon.east
   - rotunda.north <-> chenghuai.south
   - rotunda.south <-> albion.north
   - rotunda.stair <-> classical-hall.stair (the well is served by the Rotunda)
   - elan.shaft <-> cube.shaft (the well is served by Élan)
   - elan.east <-> cube.west, whose light-lock class gives Sea Light its dark vestibule
   - rotunda.entrance <-> grounds.gate

   Mate walks the graph from the Rotunda (the anchor at the origin; the arrival point is the gilt sun, facing west) and solves translations. It then checks:
   - Site.PortsMate: kind, class and level agree, the ports face opposite, and wells coincide;
   - Site.LoopsClose: within 1 mm;
   - Site.NoOverlap: between Building footprints;
   - Site.OneGround: the Ground footprint contains every Building;
   - Site.MatchesCanvas.
2. **Detail.** Each stage's description is detailed. The Ground stage receives the placed Building footprints as its input, so there is no cycle and no room-name branch.

**The joint is a doorcase,** drawn once by Site and made by Form and Matter:
- the travertine threshold runs through both wall depths and the passage;
- the brushed-bronze strip lies on the mating plane;
- a travertine lining sits in front of both rough faces, so no surfaces are coplanar;
- surrounds cover both faces, flat or following the drum;
- passage masonry closes the whole gap between the two faces inside the class's collar envelope, including the wedges where a flat face meets the curved drum;
- the class's closure is added as Motion mechanisms: door leaves, a gate, a light lock (two doors that are never open together, publishing '<port>.lock'), or a landing door interlocked by key with the serving car at that landing;
- wells get a collar at the slab with the bronze ring on the mating plane.

Form.Sealed runs over each stage and over the assembled museum in every composed configuration, and every finding names its drawer.

**Assembly in Unreal.**
- Studio writes each stage to L_<Stage> in its local frame, with USiteStageInfo, the enclosure components, the piece table and the field textures.
- L_Museum streams each stage level at its solved translation, and L_Joints holds joints and blinds.
- At play time Site's subsystem:
  - knows the stage (Form::EnclosureAt, then the stage);
  - keeps resident the current stage, the stages visible through open joints (a visibility graph computed at making), and the Grounds whenever the current stage has an exterior opening;
  - spawns the body at the arrival point.
- The Grounds terrain is a Nanite mesh.

**Adding a room.**
1. A new module under Source/Stages/Rooms/<Room> declaring its ports.
2. One mating line in SitePlan.json.
3. The module entry in the .uproject.
4. musee.studio.make <room>, then musee.studio.make site.

No other room is edited: the neighbour's blind is simply replaced by a joint, which is Site's output. Studio's dependency hashing remakes only the joints and the stages whose records changed. A port the anchor never declared is a real design change to that room, reviewed as one. Studio.Wiring fails any stage-to-stage reference.

**Works** plug in the same way. The Cube declares a Space anchor, Art's Hang binds the work to it, and the work performs inside the Cube's sealed shell behind its light lock.

## The project tree

```
Musee Vision/                               repo; branch renaissance
├── windows/  Shared/  iOS/  usd/            frozen, read-only references (full backup exists)
├── plan/                                    the design canvas (evidence for Site.MatchesCanvas)
├── assets/paintings/  data/                 evidence of the collection (imported by Studio into Art)
└── MuseeVision/                             the new Unreal 5.8 project
    ├── MuseeVision.uproject                 modules in reading order with types; MuseeInstruments has
    │                                        TargetConfigurationDenyList [Shipping]; OpenXR enabled project-wide
    ├── README.md                            the atlas: systems, stages, make, check, play, add a room
    ├── Config/                              DefaultEngine.ini (renderer section: Light; collision names: Foundation)
    │                                        DefaultInput.ini (Visitor)  DefaultGame.ini
    ├── Plugins/                             DLSS / Streamline only (git-ignored)
    ├── Docs/        Principles.md  Glossary.md  Wiring.md (GENERATED)  FrameBudget.md (GENERATED)
    ├── Data/                                specifications as text; each folder owned by one system
    │   ├── Time/      Museum.json (city: a making-time decision)  Cities.json  Hours.json  ClimateNormals/
    │   ├── Matter/    Substances/*.json  Workings.json  Histories.json
    │   ├── Form/      Profiles.json  Orders.json            (the language; named by what they are)
    │   ├── Light/     Luminaires/ (*.json + *.ies)  DaylightControls/  UseTargets.json  Operation.json
    │   ├── Sound/     Emitters/*.json
    │   ├── Nature/    Species/*.json
    │   ├── Art/       Collection.json  Care.json  Fittings.json  Hang.json
    │   └── Site/      SitePlan.json  PortClasses.json
    ├── Evidence/    Matter/ Light/ Sound/ Nature/ Art/ (incl. scans) Site/   each credited, each with its tier
    ├── Journal/     Defects.md  Decisions.md  Reviews/Rotunda.md
    ├── Source/
    │   ├── MuseeVision.Target.cs            desktop (Development/Shipping): living modules; Instruments not in Shipping
    │   ├── MuseeVisionVR.Target.cs          headset (DebugGame; Musee modules optimised by MuseeModuleRules)
    │   ├── MuseeVisionEditor.Target.cs      editor: everything
    │   ├── Systems/                         in reading order
    │   │   ├── Foundation/  MuseeFoundation.Build.cs (+ MuseeModuleRules)  README.md
    │   │   │   ├── Public/   MuseeUnits.h MuseeFrame.h MuseeKey.h MuseeRegistry.h MuseePose.h MuseeConfiguration.h
    │   │   │   │             MuseeHumanMeasure.h MuseeRecords.h MuseeText.h MuseeSetting.h MuseeUsable.h
    │   │   │   │             MuseeCaptioned.h MuseeSignals.h MuseeCommand.h MuseeCheck.h MuseeProbe.h MuseeCollision.h
    │   │   │   └── Private/  Tests/
    │   │   ├── Time/        Public/ (TimeMoment.h TimeSky.h TimeSubsystem.h TimeObserver.h TimeDial.h
    │   │   │                TimeMomentParameters.h)  Private/ (Almanac/ Calendar/ Weather/ Tests/)
    │   │   ├── Matter/      README.md
    │   │   │   ├── MuseeMatter/         Public/ (MatterSubstance.h MatterSpec.h MatterPieceLayout.h MatterResolve.h
    │   │   │   │                        MatterSheet.h)  Private/  Graphs/*.py
    │   │   │   └── MuseeMatterShaders/  Shaders/ (Stone Wood Metal Glass Water Cloth History .ush)
    │   │   ├── Form/        README.md
    │   │   │   ├── MuseeForm/        Public/ (FormDrawing.h FormProfile.h FormCatalogue.h FormOrder.h FormWall.h
    │   │   │   │                     FormOpening.h FormEnclosure.h FormCoursing.h FormStair.h FormFacts.h
    │   │   │   │                     FormAssembly.h)  Private/ (Language/ Enclosures/ Tests/)
    │   │   │   └── MuseeFormMaking/  Public/ (FormMake.h FormLeaks.h FormFields.h)
    │   │   │                         Private/ (Kernel/ Stations/ SettingOut/ Sealing/ Solidity/ Fields/ Granularity/ Tests/)
    │   │   ├── Light/       Private/ (Sky/ Daylight/ Luminaire/ Diffuser/ Scheme/ Commission/ Operation/ Transport/ Probes/)
    │   │   ├── Sound/  Motion/ (Private/ StateSpace/ Interlock/ ...)
    │   │   ├── Nature/      Private/ (Plants/ Phenology/ Water/ Precipitation/ Rock/ Ground/)
    │   │   ├── Art/         Private/ (Catalogue/ Care/ Objects/ Fittings/ Hang/ Immersive/)
    │   │   ├── Visitor/     MuseeVisitor/  MuseeVisitorScreen/  MuseeVisitorHeadset/
    │   │   └── Site/        Public/ (SiteRoom.h SiteDescription.h SitePort.h SiteWell.h SiteFootprint.h SitePlan.h
    │   │                    SiteJoint.h SiteStageInfo.h SiteSubsystem.h)
    │   │                    Private/ (Contract/ Plan/ Detail/ Joint/ Traffic/ Configurations/ Reach/ Visit/ Residency/ Tests/)
    │   ├── Stages/
    │   │   ├── Rooms/
    │   │   │   ├── Rotunda/      MuseeRotunda.Build.cs (Editor)  README.md
    │   │   │   │   ├── Public/   RotundaRoom.h                    ports (4 doorways + served stairwell), footprint, anchors
    │   │   │   │   └── Private/  RotundaPlan.h RotundaDrum.cpp RotundaOrder.cpp RotundaDome.cpp RotundaOculus.cpp
    │   │   │   │                 RotundaSunClock.cpp (dial + cylinder) RotundaStair.cpp Tests/
    │   │   │   └── Salon/ Reserve/ ClassicalHall/ HallOfLight/ Atrium/ Elan/ Cube/ Chenghuai/ Albion/ Grounds/  (later)
    │   │   └── Works/SeaLight/
    │   │       ├── MuseeSeaLight/        (Runtime: performs)  Public/  Private/
    │   │       └── MuseeSeaLightMaking/  (Editor: describes)
    │   ├── Studio/          Private/ (Make/ Cast/ Contexts/ Levels/ Commission/ Check/ Conventions/ Import/
    │   │                    Localization/ Perf/ Package/)
    │   └── Instruments/     Public/  Private/
    ├── Content/
    │   ├── Systems/<System>/                hand-made assets a system owns (Time/MPC_Moment, Nature/MPC_Nature, Light/IES)
    │   ├── Generated/                       written only by Studio; hashed by recorded dependencies; reproducible
    │   │   ├── Matter/                      M_Matter_<Family>, MI_<Substance>_<Working>_<History>, PM_<Substance>
    │   │   ├── Light/                       M_Light_Diffuser, MI_Diffuser_<Luminaire>
    │   │   ├── Stages/<Stage>/              L_<Stage>, SM_<Stage>_<Piece>, T_<Stage>_Pieces, T_<Stage>_Traffic,
    │   │   │                                T_<Stage>_Ponding, DA_<Stage>_Commissioning
    │   │   ├── Works/<Work>/                content materials that only feed image luminaires
    │   │   └── Site/                        L_Museum, L_Joints, SM_Joint_<PortA>_<PortB>
    │   ├── Studio/                          L_Stand
    │   └── Localization/                    string tables gathered by Studio
    └── Scripts/                             make.ps1 check.ps1 perf.ps1 package.ps1 (no numbers, no room names)
```

## Where physics lives

"Physics" is not one system. It is the museum's physical truth, split by meaning. Each truth has exactly one owner, one probe and one check. Checks are pure maths over inputs Studio supplies, so none reaches upward.

- **Units, constants and the body's measures:** Foundation (constants only; each formula lives with its user).
- **Astronomy, calendar and weather:** Time. Checks: Time.EphemerisMatchesReference, Time.DialTrue, Time.NormalsForEveryCity, Time.SkyScopeClosed.
- **Material physics and every surface state** (optics, density, friction, porosity, absorption; wetting, drying, standing water, snow, soiling, wear, patina): Matter. It uses one law (History.ush), shared by every shader. Checks: Matter.Plausible, Matter.Calibrated, Matter.NoEmission, Matter.PieceDataFits.
- **Solidity and geometric facts** (closed thick shells except at declared openings; no interpenetration; collision from the same solids; walkable faces from the body's slope; stairs by Blondel; sky exposure, cavity, wash, ponding, free space; enclosure membership): Form. Checks: Form.Sealed and Form.NoOverlap over every configuration, Form.MinThickness, Form.CollisionMatchesVisual, Form.StepsFitTheBody.
- **The energy of light over time** (emission, daylight coefficients and control, scheme, commissioning, operation, transport): Light. Checks: Light.OneSunOneSky, Light.EmissiveHasEmitter, Light.MeetsTarget (at every reference moment), Light.OneTransport, Light.Operated.
- **Conservation limits for works** (lux, UV, annual dose): Art. Check: Art.WithinCare, measured with Light's probe.
- **The sensor** (reflected metering with one constant, adaptation, white balance, tone): Visitor's Eye. Check: Visitor.OneEye.
- **Acoustics:** Sound. Checks: Sound.ReverberationFromMatter, Sound.Occluded.
- **Mechanics and the state space** (joints, drives, masses, travel, clearance, housings, interlocks, comfort, and the configurations every other check samples): Motion. Checks: Motion.Clears, Motion.Housed, Motion.Balanced, Motion.Interlocked, Motion.Comfortable.
- **Fluid bodies, growth and erosion** (water level by gravity with volume conserved, waves, flow, rain in the air, plants in their growth space with runtime phenology, rock by dissolution): Nature. Checks: Nature.WaterContained (every configuration), Nature.InSeason (every solar term), Nature.WithinGrowthSpace, Nature.Rooted.
- **The visitor's body** (gravity, walking, stairs, sitting, riding): Visitor. It uses character movement set only from FMuseeHumanMeasure (Visitor.BodyFromMeasure).
- **Egress:** Site, over every composed configuration (Site.Reachable), confirmed by the real body on the perf tour (Site.ReachWalked).
- **Never trapped** is split by cause:
  - a part moving into an occupied travel volume is Motion.Interlocked;
  - a plan with no way out in some configuration is Site.Reachable;
  - a wedge between fittings and walls is Site.Reachable with the true body.

**Sorting a physics bug by truth names its owner:**
- light through a wall: solidity (Form);
- too dark: energy (Light), or the sensor (the Eye) when the lux are right;
- a puddle on a slope: surface state (Matter) over Form's ponding fact;
- water through a kerb: a fluid body (Nature);
- crushed by a lift: mechanics (Motion).

Chaos is used only for collision queries. Nothing is a free rigid-body simulation. Every defect is first classified as "physics broken" or "renderer limit" using Light's path-traced reference. A renderer limit gets a museum-wide change in Light's transport or a documented limit.

## Naming

**Systems:** each is one singular English noun naming a truth or a party of a museum. In reading order: Foundation, Time, Matter, Form, Light, Sound, Motion, Nature, Art, Visitor, Site. Stages are Rooms and Works. Beside the museum stand a place (Studio) and a set of tools (Instruments). The plural is deliberate: it names a case of instruments, and it is the only such name.

**Language:** code is English and ASCII, spelled the way Unreal spells it (Meters, Color: Art.ColorManaged, false-color). Prose keeps the owner's spelling. French and Chinese appear only as proper names in identifiers (Salon, Elan, Chenghuai) and with full accents in display text through FText (Élan, Nymphéas, 澄懷).

**Modules:** Musee<System>; a second phase or device module is Musee<System><Phase> (MuseeFormMaking, MuseeMatterShaders, MuseeVisitorScreen, MuseeVisitorHeadset); Musee<Room>; Musee<Work> and Musee<Work>Making; MuseeStudio; MuseeInstruments. The folder name equals the module name, and every Build.cs derives from MuseeModuleRules.

**Reflected types:** the Unreal letter, then the home word, then a noun: FFormProfile, UMatterSubstance, ULightLuminaire, FTimeMoment, UMotionMechanism, UNatureWaterComponent, UArtFitting, AVisitorBody, FSitePort. Foundation's home word is Musee (FMuseeKey, FMuseeCaption). Everything that lives in a stage is a component (UNatureWaterComponent, ULightLuminaireComponent), not an actor. The only actors are AVisitorBody and the GameMode.

**Non-reflected code:** namespaces Musee::<System>, with no free functions in the global namespace. One public type per header, and the file is named after the type without its letter. Public/ holds only connectors. Tests live in Private/Tests/.

**Units:** C++ interfaces use Musee:: typed quantities. Reflected UPROPERTYs are plain floats with the unit in the name and Units metadata (IlluminanceLux, HeightMeters). Engine centimetres appear only in Musee::ToEngine.

**The glossary** (Docs/Glossary.md, one word per thing):
- Opening: a hole in a wall (kinds Glazed, Open, Port, Housing), never 'aperture'
- Enclosure: a space bounded by a shell (kinds Enclosed, Court, Outdoors)
- Port: a stage's declared interface
- Well: the prism a vertical port shares (serving and receiving sides)
- Joint: the doorcase between two mated ports
- Blind: the closure of an unmated port; blind well: its vertical twin on the stand
- Piece: one solid of one substance
- Body: pieces that move as one
- Drawing: shape as data
- Sweep: Form's profile-along-path primitive only
- Travel: the volume a body passes through
- Clearance: the gap between a travel volume and anything else
- Growth space: the free volume a plant may occupy
- Spec: substance x working x history
- Luminaire: a light fixture; there is no other word for one
- Target: lux a surface must receive
- Care: a work's light limits
- Fitting: gallery furniture
- Work, Anchor, Hang, Moment, Sky, Observer, Mechanism, Configuration, Signal, Channel, Setting, Probe, Check
- Stage: a room or work module only. The test stand is the Stand, and the Cube's anchor kind is Space
- Drum: the cylindrical wall under a dome only. The sun clock's rising part is the Cylinder
- Describe: what a room does. Detail: what Site does to a description. Resolve: what Matter does to a spec. Scheme: what Light does to targets. Commission: what Light does after measuring

**Data keys:** lowercase and dot-scoped, kebab within a segment, one style everywhere: travertine, rotunda.west, cube.space, salon.bay1.north, monet-impression-sunrise, prunus-mume, doorway.3000x4500.arched, stairwell.spiral.3600, track-spot.3000k.25deg. Display names are FText, never keys.

**Checks** are named <System>.<Truth> as a positive statement (Form.Sealed, Light.LuminairesFromCatalogue, Art.ColorManaged), with a declared scope. **Probes** are verbs: <System>::Measure<Quantity> (Light::MeasureIlluminance, Motion::MeasureClearance, Sound::MeasureReverberationTime, Visitor::MeasureExposure), <System>::Find<Thing> (Form::FindLeakPaths, Site::FindUnreachable, Light::FindTransportLeaks) or <System>::List<Thing> (Light::ListSources). **Defects** are numbered D-0001 onward, and each must name an existing check.

**Commands:** always musee.<system>.<verb>[-<object>]: musee.time.advance-hour, musee.time.set-hour 15, musee.light.set-profile headset, musee.studio.make rotunda, musee.studio.check deep, musee.instruments.show lux. Settings create their set-commands automatically. **Log categories:** LogMusee<System>. **Stat groups:** STATGROUP_Musee<System> (CPU). GPU cost is attributed by render pass through each README's declared passes.

**Assets:** Epic's kind prefixes: SM_<Stage>_<Piece>, M_Matter_<Family>, MI_<Substance>_<Working>_<History>, PM_<Substance>, M_Light_Diffuser, T_<Stage>_<Field>, DA_<Stage>_Commissioning, L_<Stage>, L_Museum, L_Joints, L_Stand. Generated assets live only under /Game/Generated/<Owner>/, and hand-made assets only under /Game/Systems/<System>/. No asset path string appears outside its owner.

**Banned words**, matched as whole CamelCase or kebab tokens of Musee identifiers and asset names (engine names and proper names in data are exempt): Temp, Tmp, Hack, Fix, New, Old, Final, V2, Copy, Extra, Misc, Util, Utils, Helper, Manager, Override, Test (outside Tests/), Boost, Fudge, Tweak, Compensation, Multiplier, Fill, Brightness, Intensity, Strength. Scale is banned inside Light, Matter and Visitor's Eye. Legitimate trade words pass because matching is by token (Fillet, Fixed, Threshold, Temperature). The clock's rate is ClockRate, and the fixed hour is the setting time.hour.

**READMEs:** every module has the same eight headings in this order: Owns, Does not own, Connectors, Depends on, Checks, Frame budget (the render passes it owns, the ms and the stat group), Glossary, Sources.

**Comments** say which physical truth the code models and where its numbers come from. **Commits** on renaissance start with the owner ('Form: shared stations for closed loops', 'Rotunda: coffer rings').

## How the principles are carried

- **1. Everything works physically:** Each physical truth has one owner, one probe and one check, sampled wherever the truth can vary:
- light never leaks: Form.Sealed over every configuration Motion supplies, per stage and on the assembled museum; Open openings and courts are declared, never faked;
- no part passes through another: Motion.Clears and Motion.Housed, including through wells on the stand;
- buildable mechanisms: Motion.Balanced;
- never trapped: Site.Reachable (factored configurations), Site.ReachWalked and Motion.Interlocked, with the landing and light-lock doors in joints;
- water behaves like water: Nature.WaterContained in every configuration, and standing water on paving by Matter's law over Form's ponding field;
- plants never grow into walls: Nature.WithinGrowthSpace;
- real PBR: Matter.Plausible and Matter.Calibrated.
- **2. One unified material language:** Matter is the only source of materials on pieces. Every surface resolves through substance x working x history (Matter.OneLanguage). The only emitting material is Light's diffuser master, which shades a Matter substance (opal glass). A work's content materials only ever feed an image luminaire. The era rule is built into the data model, and there is one bronze. One surface law (History.ush) makes wet grass, wet travertine and a wet frame obey the same physics. Profiles and orders live in Data/Form, authored from the Rotunda first, so later rooms draw from the anchor's language by promotion, never by copying. Fittings (benches, barriers, signs) are catalogued once by Art in the same materials. The doorcase is drawn once by Site.
- **3. Indistinguishable from real photographs:** Quality is calibrated against reality:
- specs pass evidence tiers (measured tables, ColorChecker specimens, museum photographs at known exposure);
- lighting is schemed by rule, then commissioned against path-traced measurement at reference moments, as real designers commission with a lux meter;
- daylight uses the standard daylight-coefficient method;
- the Eye meters like a camera with one constant;
- geometry is real pieces at one chord tolerance, with loft and surface patches for doubly curved forms and scans only for real art objects.
Fidelity comes only from the shared systems.
- **4. Build from the real thing:** Each system is a real trade:
- the quarry and foundry (Matter);
- the architect, mason and joiner (Form);
- the astronomer and diallist (Time);
- the lighting designer and commissioning engineer (Light);
- the acoustician (Sound);
- the millwright and lift engineer (Motion);
- the gardener and hydrologist (Nature);
- the curator, conservator and exhibition designer (Art);
- the surveyor and site architect (Site).
Mechanisms are real systems: blinds follow a building-automation channel, light locks are two interlocked doors, lifts run in wells with landing doors. Wear follows the real traffic field. Plants grow toward free space and change with the solar term. The sundial is cut for the one real city.
- **5. Fix causes, never symptoms:** The defect_routing table sends every symptom to the owner of the broken truth, and every finding names its drawer. Journal/Defects.md entries must name an existing check.

Patches are impossible by construction:
- Matter cannot emit;
- only Light makes light, and every luminaire has a control class;
- post-process fields are split disjointly between the Eye and Light's transport, and no other volume may exist;
- the description has no exposure, intensity or finish-override method;
- runtime changes (season, wetness, dusk) go through parameters, so no one remakes or hand-adjusts;
- per-room settings do not exist, while a design change to a stage's description is legitimate and reviewed;
- the token-based lint leaves a patch no name.

The instruments measure: the lux meter, false colour, exposure, leaks, sources, clearance, reach, reverberation, fields and frame time.
- **6. The code is part of the art:** The design follows the logic-board discipline:
- one home per truth;
- small connectors;
- one-way dependencies declared per layer;
- peers meeting only through Foundation's signals, channels and key registry;
- checks as pure functions over supplied inputs;
- one owner per engine capability;
- one way to do each thing: one kernel, one surface law, one resolver, one emissive master, one clock, one weather fetcher, one eye, one doorcase, one caster, one settings store;
- names that say where things live, with overloaded words (Stage, Drum, Sweep, Design, Resolve) removed.
The unseen insides (piece layout, stations, dependency hashing, pass attribution) have checks of their own.
- **SpaceX model:** The eleven systems are the shared engines and tanks. Rooms and works are stages in their own modules and true-north frames, each fired alone on the stand with blinds and blind wells. Site is the standard interface: ports with wells, one mating graph solved in two passes, and one doorcase whose bronze strip lies on the interstage plane, with the closure mechanism built into the joint. A new room plugs in with its module and one mating line. The anchor declares every port up front, so no other room is edited.
- **Method: all structure first, then the Rotunda only, then review:** 1. Write every Public/ connector and README, the MuseeModuleRules layers, the registries (checks, probes, keys, settings, channels) and Studio's pipeline and contexts, and review them before any Private/ code.
2. Implement each system completely for its truth. Code goes only as deep as the Rotunda needs, and missing capabilities are absent, never stubbed.
3. Prove the loop (describe, detail, make, cast, commission, check, measure) on the travertine ashlar and oak board specimen sheets under one luminaire read by the lux meter. This proves the piece layout within Nanite's budget.
4. Run one specimen per remaining system on the stand: a painting on its anchor, a tree in a growth space, a water body on a moving basin, a blind following a shade channel, and a headset session.
5. Build the Rotunda alone: the drum, 16 fluted pilasters, the entablature, the coffered dome, the oculus (open or glazed, by decision), the sun clock cut for the museum city, the rising cylinder, the spiral stair in its served well, and four declared doorways.
6. Write Journal/Reviews/Rotunda.md.
7. Only then start the next stage.

## Defect routing (every stress-test scenario, re-routed)

| Symptom | Owner | Why |
|---|---|---|
| A light streak across the Rotunda dome, between coffers, or down an inside corner (the old Reserve streak) | Form (or the system that drew the leaking piece, named in the finding) | A shell is open where it should be solid. Form::FindLeakPaths runs over the Studio-built FFormAssembly in every configuration Motion supplies. The oculus is a declared Open or Glazed opening and is excluded. The fix goes into the generator that left the open edge. Form.Sealed and Form.MinThickness guard it. It is never fixed with a blocker plane. |
| A leak remains although Form.Sealed passes | Light | The geometry is proven closed, so the fault is in transport (the surface cache, distance fields on thin pieces). Light::FindTransportLeaks locates it. The fix goes into Light's transport or Form's granularity rule, for every stage. |
| A leak appears only when the Salon pond is raised | Form (the leaking piece's drawer), with the configuration supplied by Motion | Motion's state space yields the raised configuration, and Studio passes it to Form.Sealed as an input. Form never reaches up to Motion. The finding names the piece and the configuration. |
| A gap, crack of sky or flicker where a passage meets a room | Site | Either a room built past its port face (Site.RoomStaysInside) or the doorcase leaves a gap. The lining sits in front of both rough faces, and Form.Sealed on the assembled museum names Site's joint as the drawer. |
| Wedge voids beside the surrounds where Chenghuai's flat port meets the Rotunda's curved drum | Site | The joint's passage masonry closes the whole gap between the two port faces within the class's collar envelope, following each face's shape. Site.PortsMate checks that the pair of face shapes is supported, and Form.Sealed on the assembly finds any remainder. |
| A step, level change or mismatched surround at a doorway, or the bronze strip on one side only | Site | Mated ports must share class and level and face opposite (Site.PortsMate), and the doorcase is drawn once. A mismatch is a build error. |
| An open-air court (Chenghuai, Albion, an open oculus) fails Form.Sealed, or has to fake glazing | Form (the enclosure and opening kinds) | Court enclosures are sealed except at one declared Open opening, and the Grounds is Outdoors. The check follows the declared kinds, never a room name. Rain falls through Open openings by the same declaration, and the stage draws the drains. |
| Plugging Chenghuai into the Rotunda's north requires editing the Rotunda | Site (the contract) | The anchor declares all four doorways and its stairwell on day one, and unmated ports carry blinds. If a port was never declared, adding it is a reviewed design change to that room. Studio.Wiring fails any stage-to-stage reference. |
| Changing the shared doorway detail museum-wide leaves some stages stale | Site (PortClasses), with Studio's hashing | The joint and the rough-opening allowance are defined once per class. Every stage that read PortClasses.json through Foundation's reader has it in its dependency set, so exactly those stages and joints are recast (Studio.Incremental). |
| A gallery reads too dark by day | Light, or the stage's opening design, or the Eye, in that order of measurement | Measure the lux first. Below target means the scheme, the commissioning or an undersized daylight opening (flagged by Light.MeetsTarget at the reference moments). If the lux are right, the Eye's metering is wrong, and the fix changes every room. It is never a fill light. |
| A room too dark at dusk or at night (the Rotunda under its oculus after sunset) | Light (operation and use targets) | Every stage declares targets by use, including night and orientation levels. Light.MeetsTarget holds at every reference moment, including dusk and night, and Operation switches and dims luminaires from opening hours and solar altitude. A stage with no mounts where a night target exists fails the check, and adding mounts is a reviewed design change. |
| Lamps or canal lanterns do not switch on at dusk, or a diffuser glows while its lamp is off | Light | Light's Operation (Data/Light/Operation.json) switches by control class from the Moment's opening hours and solar altitude. One ULightLuminaireComponent drives both the light and its diffuser from one record. Light.Operated requires a control class on every luminaire. |
| Luminous ceiling panels glow but light nothing (the Classical Hall) | Light | A luminous surface is a luminaire: a rect light plus a matched diffuser from one record. Matter.NoEmission and Light.EmissiveHasEmitter prevent a glow without an emitter. |
| The diffuser material is grey, or is flagged by Matter.OneLanguage | Light (M_Light_Diffuser), with the diffuser substance from Matter | The only emitting material is Light's diffuser master. It shades a Matter substance (opal glass or acrylic) through the .ush library, and Light adds emission in nits from the luminaire record. Matter.OneLanguage covers non-emitting piece materials, so there is no double claim. |
| A painting measures 400 lux at noon instead of 150, or silk or paper above 50 | Art (the care limit), fulfilled by Light (scheme, operation, daylight control) | The limit exists only in Art's Care.json. Measured at noon, the excess is live daylight: Light's daylight coefficients predict it, Operation dims the electric light and publishes '<opening>.shade', and the blind (a Motion mechanism following that channel) closes. Art.WithinCare holds at every reference moment and within the annual dose. A wrong care class is Art's fix. |
| Glare or a hot spot on a varnished painting | Light | The luminaire breaks the aiming rule. Light.NoGlare tests the mirror reflection from the eye zone for every target. |
| A green or orange cast on white plaster | Light (source CCT), Matter (albedo) or the Eye (white balance) | Measure first. The plaster is never re-tinted. |
| A sunlit room is washed out at noon, dims whenever the sun is in view, or adapts too fast or too slowly from dark to sun | Visitor (Eye), unless the measured lux are wrong (Light) | The Eye uses reflected metering with one constant and human adaptation speeds, chosen once, museum-wide. There are no exposure zones (Visitor.OneEye), and every room is re-surveyed after a change. |
| GI or reflection quality differs between places, or exposure changes when a transport setting is changed | Light (transport fields) or Visitor (camera fields) | Post-process fields are disjoint: camera fields exist only in the Eye's unbound volume, transport fields only in Light's (Visitor.OneEye, Light.OneTransport). Any third volume fails both checks. |
| The VR image is darker than the desktop, or menus go black in bright rooms | Visitor | There is one Eye for every adapter, and the overlay is composited after tone mapping (Visitor.OverlayOutsideScene). |
| Two rooms show different sun or sky at the same hour | Light (rendering) or Time (state) | There is one sun, one sky and one FTimeSky (Light.OneSunOneSky), and the clock and HTTP each have one owner. |
| Sea Light's sky remains after the visitor leaves the Cube, or the wrong sky shows through the joint into Élan | Art (the immersive base), with Site's light lock and Time's scope | UArtImmersiveWork pushes only FTimeSky, and only while '<port>.lock' reports the light lock closed. It pops before the outer door may open, and Lumen re-converges behind closed doors. The museum Moment (wetness, season, weather ambience) never changes. Art.ImmersiveScoped and Time.SkyScopeClosed guard it. |
| Sea Light shows no weather offline | Time | Climate normals exist for every city a sky can be pushed to (Time.NormalsForEveryCity), and the one fetcher asks about all observer locations in one request. |
| Sea Light needs its own light-field shader and generated geometry | The work (MuseeSeaLight and MuseeSeaLightMaking) through Light's image-luminaire kind | The work's making module describes geometry and content as data for Studio. Its runtime module implements ILightImageSource for image luminaires with measured peak luminance, and its content materials only feed that source. Light owns the light and Matter the pieces, so nothing is out of law. |
| Glass turns black | Sorted by the path-traced reference: Studio (cast), Matter (glass) or Light (transport) | If the path tracer shows clear glass, it is a renderer limit: Light's translucency and refraction settings, or glass cast as Nanite (Studio.CastRules). If the path tracer also shows black, Matter's absorption per mm is wrong (Matter.Plausible). |
| Glass edges too green, or glass looks like a mirror | Matter (the glass substance), with the pane thickness from Form | The teal edge is iron absorption times the real path length at IOR 1.52, not a tint. |
| Stone looks like wallpaper, a repeating texture or plastic | Matter (Form if pieces carry no identity) | Every piece carries a PieceId into its stage's piece table (seed, bed frame, sky exposure) and is shaded from its substance. Missing piece data is Form's setting-out, and a repeat is Matter's model (Matter.Calibrated). |
| Nanite runs out of vertex attributes, or UVs swim on a 28 m wall | Matter (the piece layout), with Form's piece-local UVs | FMatterPieceLayout fits Nanite's budget: UV0 in piece-local metres, PieceId in UV1, four vertex-colour fields, and everything constant per piece in the piece table and plan fields. Continuous finishes are laid as real bays. Matter.PieceDataFits guards it. |
| Joints across an arris, courses missing an impost, a 6 m travertine block, or oak grain across a board | Form (setting-out) | Lay solves coursing with fixed lines within stock. Form.WithinStock guards it. |
| A faceted arch, polygonal steps on the drum, or crescent slits at the dome bands | Form (stations) | There is one chord tolerance, with shared stations (Form.SharedStations). |
| Visible seams between wall pieces, or a dark streak along a moulding under ray tracing | Form | One crease rule and MikkTSpace tangents from piece-local metre UVs (Form.NormalsContinuous). Intended joints are real arrises, not seams. |
| Bronze looks like yellow plastic, or differs between rooms | Matter | There is one alloy with a measured complex F0, brushing is its working, and patina is its history (Matter.OneLanguage). |
| Stone stays dry in the rain, dries instantly, or an interior floor turns wet | Matter (the surface law), or Form if a piece's sky exposure is wrong | Wetness = rain history x sky exposure (hemispherical, from Form) x porosity. Travertine darkens and dries over hours. |
| A puddle on the travertine terrace, or puddles on a slope | Matter (the surface law) over Form's ponding field | Standing water is a surface state: rain history times Form's ponding depth (low points found by FormMaking from the walkable relief). Nature owns only bodies of water, so the film has one owner. |
| Snow on the lawn and snow on the paving disagree | Matter | Snow depth is one term of the surface law in History.ush, which Nature's ground-cover shaders include. There is one implementation for every surface. |
| Rain falls indoors | Nature | Precipitation stops at shells and passes only through declared Open openings. |
| The oak floor wears uniformly, or someone hand-paints wear lines | Site (the traffic field), applied by Matter | Site::Traffic routes between each stage's ports, anchors and seats and writes a plan-field texture. Matter's law multiplies it by age. It is computed per stage, so the stage stays self-contained. |
| No rain-wash streaks under sills, or no grime in sheltered corners | Form (cavity and wash facts), applied by Matter | FormMaking writes cavity and wash exposure per vertex, and Matter's soiling term reads them. Nobody paints masks. |
| The sun clock's rising cylinder or its panels pass through the spiral stair | Motion detects; the Rotunda's description is fixed | Motion.Clears and Motion.Housed find it at every sampled phase. Both bodies are the Rotunda's, so the geometry change is a design change in the Rotunda: the owner of the fix is the stage, by rule. |
| Élan's lift is blocked on the stand, or builds into the Cube's volume | Site (the well) | The shaft port carries a well that Élan serves and the Cube reserves. On the stand a blind well continues the shaft, so Motion.Clears judges the full travel alone. Site.RoomStaysInside allows the serving well, and the landing door is a joint mechanism interlocked by key with the car. |
| The Rotunda's spiral stair appears to violate RoomStaysInside in the Classical Hall | Site (the well) | The stairwell is a well that the Rotunda serves and the Classical Hall reserves. The collar with its bronze ring lies on the mating plane at the slab. |
| The visitor is trapped: a lift moves with the visitor half inside, or the raised cylinder leaves a dead end | Motion (interlock) or Site (egress) | A part moving into an occupied travel volume fails Motion.Interlocked. A configuration with no way out fails Site.Reachable, factored by coupling groups. |
| The visitor is wedged between a bench and a wall | Site (reachability), with the bench from Art | Fittings are Art's catalogued objects placed at Bench anchors. Site.Reachable uses an agent built from FMuseeHumanMeasure, and Site.ReachWalked confirms each route with the real body. |
| Every room builds its own bench, or the visitor cannot sit | Art (fittings), with Visitor's sit intent | Benches, barriers, the desk and signs are in Data/Art/Fittings.json. A seat implements IMuseeUsable with a seat frame, and Visitor's body sits. |
| The visitor steps up a kerb Form calls non-walkable, or cannot climb a ramp Site calls reachable | Foundation (the measure), guarded in each consumer | Form's walkable tag, the body's movement and Site's agent all derive from FMuseeHumanMeasure (Visitor.BodyFromMeasure, Form.StepsFitTheBody). Site.ReachWalked catches any remaining disagreement. |
| The visitor walks through a railing, snags or stumbles on a stair | Form | Collision comes from the same solids (Form.CollisionMatchesVisual), and stairs follow Blondel over the body's pace (Form.StepsFitTheBody). |
| Nausea when the Élan lift or car moves | Motion | Motion.Comfortable enforces the passenger-lift limits on acceleration and jerk. |
| The sun clock's shadow falls off the hour, or the sun enters the oculus at the wrong time | Time (then the Rotunda's dial layout) | Measure the sun against the reference ephemeris (Time.EphemerisMatchesReference), then check that the dial was laid from Time::Dial for the museum city (Time.DialTrue). |
| The owner wants the museum in another city, and the dial would be false | Time (a making-time decision), remade by Studio | There is no runtime city command. The city is in Data/Time/Museum.json, recorded in Decisions.md. Every dial and made fact that read it is in its dependency set, so changing it remakes them. |
| The weather differs between rooms, freezes offline, or does not match the city | Time | One fetcher, one Moment, and climate normals offline. |
| Trees in leaf in winter, or the season frozen during a long session | Nature | Structure is made once. Leaves, blossom and colour are instances driven at runtime through MPC_Nature from the solar term (Nature.InSeason over every term). |
| A plant grows through a wall, or its canopy sways into one | Nature | Grow takes the growth space (Form's free space shrunk by the species' maximum sway), and attraction points exist only there (Nature.WithinGrowthSpace). |
| Identical plants, or a Taihu rock like a noise blob | Nature | Growth is seeded by key and ruled by species, and rock forms by dissolution along joints. |
| Water spills from the Salon pond as it lifts, or its surface tilts | Nature (Motion if the basin itself tilts) | UNatureWaterComponent attaches by key (UMuseeRegistry) to the basin body, which Form merges separately from fixed pieces. Nature.WaterContained runs over the configurations Studio supplies. |
| Rain audible under the solid vault, the Rotunda sounding carpeted, or parquet footsteps sounding like stone | Sound (Matter for absorption or footfall class) | Sound.ReverberationFromMatter uses Form's volume and Matter's absorption. Sound.Occluded uses the shells. Footfalls use the substance underfoot. |
| The reverb changes at the wrong place when walking between spaces | Form (EnclosureAt), applied by Sound | Form::EnclosureAt over the cooked enclosure components is the one membership query, shared by Sound and Site's RoomAt. |
| Rain on glass sounds from the wrong place | Sound, from Form's exposed surfaces | FFormEnclosure lists each exposed surface with its position, area and substance, and Sound places rain emitters there. |
| A painting at the wrong size, stretched, too vivid or flat | Art | Art.TrueSize, Art.ColorManaged, and a real canvas object. |
| A scanned marble sculpture or ceramic has no legal way in | Art (the object), through Form's scan drawing and Studio's import | Studio imports the scan as Evidence with its credit. Art draws the object as a Form scan with a Matter spec. Scans are reserved for real art objects. |
| Frame corners open, or mitres mismatch | Form (closed sweep); Art only if the profile or widths are wrong | The kernel owns mitre closure for every closed moulding. |
| The Rotunda cornice is wanted in the Salon but lives in the Rotunda's Private plan | Form (the catalogue) | The Rotunda's mouldings are authored in Data/Form from day one, named for what they are. Any later singular detail is promoted when a second stage needs it, as a reviewed change. |
| Chenghuai's upswept roof planes, the Élan sphere or the car body cannot be drawn | Form (the drawing vocabulary) | The vocabulary now contains Loft and Surface (B-spline patches) with shared stations. It grows only through Form's admission rule, never by importing raw meshes. |
| No door or gate can exist at a joint, or the Cube lacks a light lock | Site (the joint), made operable by Motion | The port class declares its closure. Joint() returns door leaves, gates, light locks and landing doors as Motion mechanism specs, with interlock bindings by key. |
| The Quest beam passes through a usable thing | Visitor (targeting), or the owner of the IMuseeUsable | Visitor.ActionsCovered checks every adapter against every action, and the use-shape belongs to the thing's owner. |
| A Vision Pro pinch is lost through ALVR | Visitor (VisitorHeadset) | The headset adapter maps OpenXR hand input to actions (Visitor.ActionsCovered). A mapping loss inside ALVR is recorded as a documented limit in the README. |
| A caption, wall label or sign is missing or not translated | Art (the text as FMuseeText), gathered by Studio | Record strings carry keys and are gathered into string tables. The overlay shows an FMuseeCaption. |
| A mechanism prompt ('Raise the sun clock') is missing or not translated | Motion (the spec's FText), gathered by Studio | FMuseeUse carries its verb and caption as FText, serialised in the cooked spec and gathered by the engine's localisation pass. |
| After loading, the saved hour disagrees with the clock | Time (the setting time.hour), persisted by Foundation | There is one setting, with one owner and one persisted copy. Visitor's settings card only lists and edits it. |
| Adding a room required editing another room, or a room includes another's header | Site (the contract), enforced by Studio.Wiring | Ports are declared up front, and unmated ports are blinded. |
| A shared system or data file branches on a room name, or a number differs between C++ and Python | Studio flags it; the knowledge moves to its owner | Studio.Wiring: no room names in shared code or data, and Python holds no numbers. The Grounds' special handling is by footprint kind, not by name. |
| The Grounds fails Site.NoOverlap, or cannot be described before mating | Site (footprint kinds and the two passes) | NoOverlap compares Building footprints only, and the single Ground must contain them (Site.OneGround). Mate uses ports alone, and the Ground's description receives the placed footprints in the detail pass. |
| A grey or missing material, or a finish lost after a rename | Matter (the resolver), enforced by Studio | Only owners map to their own assets, and a path literal elsewhere fails Studio.Wiring. |
| A baked mesh no longer matches the data, or a one-line change recasts everything | Studio | Output hashes include every record read through Foundation's reader (Studio.Incremental). |
| Making is too slow, or a Nanite build runs out of memory | Studio (casting, batching), or Form (granularity) if piece count dominates | The make report splits time and memory by step and owner, which decides the route. |
| A build fails with LNK1104 (DLL locked) | Studio | There is one Unreal process: making runs in the open editor, and packaging refuses while Unreal runs. |
| GPU time over 20 ms at 4K (or the headset budget) in one room | The owner of the over-budget render pass, per the README pass map; or the stage's description | The perf tour times each render pass and attributes shared passes by ablation: - Lumen and shadows: Light - Nanite raster: Form's granularity - translucency: Matter's glass - foliage, Niagara and water: Nature - post and upscaling: Visitor If a stage asks for more than the budget (too many targets or openings), changing its description is legitimate design. A per-room setting never is. |
| Hitches or memory exhaustion with the whole museum loaded | Site (residency) | Site streams the current stage, the stages visible through open joints and the Ground when exterior openings are visible. Site.ResidencyFits measures memory and GPU time on the perf tour. |
| Specimen calibration cannot run inside Matter, or metals and glass cannot be photographed neutrally | Matter (the metric and the evidence tiers), with Studio's capture | Studio renders the sheet on the stand under Light's standard luminaire, scene-referred with pre-exposure removed. Matter compares it with evidence, and metals and glass use measured optical tables. |
| The lux meter reading changes when exposure adapts | Light | MeasureIlluminance removes the renderer's pre-exposure and never reads the Eye (Light.MeterCalibrated). |
| The VR build lacks the headset adapter, Instruments leak into Shipping, or DebugGame VR is slow | Visitor (adapter selection), Studio (packaging), Foundation (MuseeModuleRules) | Both adapters ship, and the active one is chosen by the OpenXR session (Visitor.OneAdapterActive). Instruments are denied only for Shipping by TargetConfigurationDenyList. MuseeModuleRules forces optimised Musee code in DebugGame. |
| MuseeMatter fails to load early enough to map its shaders | Matter | The mapping lives in MuseeMatterShaders (PostConfigInit; Core, RenderCore and Projects only). MuseeMatter loads normally with its Time dependency. |

## Changes from the first draft

- Life is renamed Nature and owns natural process (plants, bodies of water, precipitation in the air, rock, ground).
- Place is split: Site is the integration standard, and rooms and works become stage modules under Source/Stages/.
- Sound is added as a shared system (acoustics had no owner).
- Matter moves below Form and becomes substance x working x history with stock rules.
- Exposure moves to the Visitor's Eye, and emission moves to Light.
- The making and living split is enforced by module type (MuseeFormMaking; rooms as Editor modules).
- Studio is split into Studio (Editor) and Instruments (development runtime), and probes belong to the measured systems.
- The doorway model becomes rough opening plus doorcase, with translation-only mating in true north.
- Stress-test revision: checks are pure functions over inputs Studio supplies (FFormAssembly, FMuseeConfiguration, reference moments, captures), and Motion owns the state space in coupling groups. Why: Form.Sealed, Nature.WaterContained and Matter.Calibrated reached upward or sideways.
- Stress-test revision: Foundation gains UMuseeRegistry (attach by key), FMuseePose and FMuseeConfiguration, FMuseeSetting (one owner and one persisted copy per setting), FMuseeChannel (a building-automation point), FMuseeText, FMuseeCaption and FMuseeUse (the signatures are now specified), and record dependency tracking. Why: peers needed lawful meeting points, and the hour, captions and hashing had no single definition.
- Stress-test revision: Foundation's human measure keeps constants only (the pace length included). Blondel's rule moves to Form, and the body, the walkable tag and the reach agent all derive from the measure (Visitor.BodyFromMeasure, Site.ReachWalked). Why: walkability was defined three times.
- Stress-test revision: ports get wells (serving and receiving sides), the stand gets blind wells, and joints get closure mechanisms (doors, light lock, landing door) and passage masonry that closes flat-to-curved wedges. Why: the stair and lift crossed the seam, doors had no home, and wedge voids had no owner.
- Stress-test revision: footprints get kinds (Building and Ground), and Site mates in two passes (ports, then detail). Why: the Grounds failed NoOverlap and created a data cycle without a room-name branch.
- Stress-test revision: Form's openings are Glazed, Open, Port and Housing, and enclosures are Enclosed, Court and Outdoors, with Form::EnclosureAt as the one membership query. Why: open courts were undefined under Form.Sealed, and Sound duplicated RoomAt.
- Stress-test revision: Form's vocabulary adds Loft, Surface and Scan (art objects only), under an admission rule, and Data/Form holds named profiles and orders with a promotion rule. The Rotunda's mouldings are authored there first. Why: freeform geometry, scans and shared profiles had no legal path.
- Stress-test revision: Form's making writes ponding, cavity, wash and free space as made facts, and merges pieces per rigid body. Why: puddles, macro soiling, plant growth and water attachment needed geometric inputs.
- Stress-test revision: Matter owns one surface law (History.ush) for wetness, standing water, snow, wear and soiling, shared by Light's diffuser and Nature's shaders. Nature keeps only bodies of water. Why: puddles and snow were double-owned.
- Stress-test revision: Matter's piece layout fits Nanite (piece-local metre UVs, PieceId, vertex-colour fields, a per-stage piece table, plan fields, Custom Primitive Data), guarded by Matter.PieceDataFits. Why: the attribute budget overflowed and UVs lost precision.
- Stress-test revision: Matter's calibration uses evidence tiers (measured tables, ColorChecker specimens, museum photographs), with the metric in Matter and the capture by Studio. Why: the sheet gate was an upward dependency and a bottleneck, and metals and glass are hard to photograph.
- Stress-test revision: MuseeMatterShaders (PostConfigInit, no UObject dependencies) takes over the shader mapping. Why: MuseeMatter dragged Time and HTTP into early loading.
- Stress-test revision: Light gains daylight coefficients, daylight controls published on shade channels, rule-based Scheme and path-traced Commission, Operation (dusk switching, opening hours, daylight-linked dimming), image luminaires with ILightImageSource, the one emissive diffuser master, and a transport-only post-process volume. Why: lamps at dusk, care limits under live sun, the diffuser conflict, the post-process conflict and Sea Light's light had no owner.
- Stress-test revision: Art is the only owner of lux-by-medium (Care.json, with an annual dose), gains fittings (benches, barriers, desk, signs, sit) and the anchor kind Space, and its immersive base scopes the sky to a closed light lock. Why: double-owned lux, unowned furniture, the overloaded word 'stage', and a sky that leaked through the joint.
- Stress-test revision: Time's city becomes a making-time decision (Museum.json), and the runtime city command is removed. A pushed observer changes only FTimeSky, never the museum Moment. Time adds opening hours, reference moments and normals for every city. Why: the dial went false on a city change, and a scoped observer changed the whole world.
- Stress-test revision: Nature grows plants in a growth space (free space shrunk by sway), and phenology runs at runtime through MPC_Nature on instances. Water attaches by key. Why: plants grew through walls and the season froze at the making date.
- Stress-test revision: Visitor uses reflected metering with one constant. Both adapters are compiled into every game build, and one is active by OpenXR session. Visitor gains a sit intent. Why: incident metering needed Light, and the installed engine cannot link adapters per target.
- Stress-test revision: Site gains the traffic field, factored configurations and reachability, USiteStageInfo, and runtime residency (streaming stage levels at their offsets). Why: wear, the 2^N cost, runtime stage identity and whole-museum residency had no owner.
- Stress-test revision: works get a making twin (Musee<Work>Making), and their content materials only feed image luminaires. Why: works had no legal home for generators or shaders.
- Stress-test revision: Studio's pipeline becomes ten ordered steps with commissioning. Studio assembles check contexts, hashes by recorded record dependencies, reports making by step and owner, attributes GPU time by render pass using the README pass maps, and gathers localisation. The test stand is renamed the Stand (L_Stand). Why: stale recasts, perf attribution, localisation and the 'Test' naming collision.
- Stress-test revision: naming fixes:
- a token-based banned-word lint;
- Light.LuminairesFromCatalogue;
- Color spelling (Art.ColorManaged);
- commands always musee.<system>.<verb>;
- probes as Measure, Find or List;
- Describe, Detail, Scheme and Resolve as distinct words;
- the Cylinder and Travel replace the overloaded Drum and Sweep;
- Piece and Body replace Part;
- FMuseeCaption;
- component-only kit types;
- dotted port-class keys;
- the Instruments plural stated.
Why: every naming issue the test found.
- The draft's ten systems became eleven; the only net addition is still Sound. Every responsibility the stress test found unowned went to an existing trade.

## Risks

- Owner decisions are needed before the Rotunda is built. They are recorded in Journal/Decisions.md:
- the museum city (Beijing or New York), which is now a making-time decision;
- the sun clock's correction (an analemma, or a mean-time correction);
- remote skies: Giverny and London are allowed only as a scoped sky inside a sealed work behind a light lock;
- whether the oculus is open (a Court enclosure with rain and drains, as at the Pantheon) or glazed;
- the opening hours, which drive night lighting.
- The piece layout (FMatterPieceLayout) is the keystone of Form and Matter. Nanite's attribute limits and the piece-table sampling must be proven on the two specimen sheets before the Rotunda.
- The make pipeline now has two passes and ten steps, including commissioning on the stand with the path tracer. Path-traced commissioning and daylight coefficients are slow. They run only in the deep set and are cached by dependency hash.
- Real pieces multiply geometry. Mitigation: granularity by stage, body and material family, instancing, and the budget watched from the first room.
- Large moving Nanite bodies under Lumen and MegaLights may lag or leak mid-motion. The Rotunda's cylinder tests this first.
- Lumen is not a photometric renderer. The meter removes pre-exposure and is cross-checked once against the analytic prediction and the path tracer, and commissioning uses the path tracer, so 'measured in Lumen' never becomes a tuning loop.
- Reflected metering with one constant was chosen over incident metering to keep the Eye free of Light. Some views (the sun under the oculus) will clip like a real camera, and that is intended.
- Streaming stage levels with offsets under Lumen can show a moment of re-convergence at joints. Joints with doors hide it, and open portals keep the neighbour resident through the visibility graph.
- The configuration space is bounded by coupling groups. A stage with many coupled mechanisms still multiplies, so review coupling at the description.
- Unreal mechanics to confirm on the first build:
- UBT accepts MuseeModuleRules defined in MuseeFoundation.Build.cs;
- MuseeMatterShaders maps at PostConfigInit;
- TargetConfigurationDenyList keeps Instruments out of Shipping;
- OptimizeCode = Always holds in DebugGame;
- OpenXR enabled project-wide does not disturb the desktop build;
- Custom Primitive Data and four UV channels work on merged Nanite meshes.
- About 31 modules (15 kit modules, 11 rooms, 2 for Sea Light, Studio and Instruments) are heavy for a non-programmer. Mitigation: a few lines per Build.cs, unity builds, Live Coding on plain functions, and plain-language Studio.Wiring messages.
- The one-process rule queues making, checking, perf and play, so dependency-hashed incremental making is essential. Made content is build output, not source.
- Python material graphs depend on changing Substrate APIs. Keep them to a handful of thin masters, plus Light's diffuser.
- Real weather needs a network, so the climate normals must be first-class for every city.
- Substance, luminaire, acoustic and care data need real sources. The evidence tiers make this faster (published tables first), but inventing numbers would reintroduce tuning. Every source is recorded.
- Interfaces may be guessed before they are used. One specimen per system on the stand (painting, fitting, tree in a growth space, water on a moving basin, a blind on a channel, a light lock, a headset session) comes before the Rotunda review.
- Foundation grew (registry, poses, settings, channels, captions). Each addition is a standard admitted by review, and the signal and channel set stays closed until a stage names a need.
- The pull to port old code. Reuse only what meets the bar, re-expressed through the new connectors:
- the Meeus ephemeris, validated against NOAA;
- the Rotunda's swept profiles, which now go into Data/Form;
- the shared-stations lessons;
- the clearance-at-several-states pattern;
- the static-mesh settings;
- the VR control scheme.

## Rationale

The stress test found that the spine holds: one truth per system, Matter under Form, exposure in the Eye, and stages joined at ports. Its five structural faults each had one root, and each is closed at that root rather than by a special case.

1. **Lower checks reached upward.** The root was that checks fetched their own context. Now a check is pure maths in its owner over typed inputs, and Studio, which sees everything, assembles those inputs:
   - FFormAssembly;
   - FMuseeConfiguration, Foundation's new pose currency;
   - reference moments;
   - captures.

   Motion owns the state space, reduced to coupling groups, and Site composes it across seams. This removed every upward edge: Form.Sealed, Nature.WaterContained, Matter.Calibrated, the lux meter's use of the Eye, and incident metering (replaced by reflected metering with one constant).
2. **Cross-stage connections contradicted the ownership line.** The root was that a port was only a face. A vertical port now carries a well with a serving and a receiving side. The stand continues it with a blind well, and the joint carries the closure mechanisms (doors, light lock, landing door) as Motion specs. The same rule covers the Rotunda stair and the Élan lift.
3. **The making and living split left gaps.** The root was that Moment-dependent things were baked. The rule is now: what is cut depends only on making-time decisions (the city is one), and what varies runs through parameters:
   - phenology through MPC_Nature on made structure;
   - wetness, standing water and snow through Matter's one surface law;
   - dusk and dimming through Light's operation;
   - blinds through channels.

   Works got a making twin like Form's. Light got an image-luminaire kind, so Sea Light has a lawful home. The drawing vocabulary gained Loft and Surface, and Scan for art objects only.
4. **Double ownership.** Each case went to a single owner:
   - lux limits: Art alone;
   - post-process: split disjointly between the Eye's camera fields and Light's transport fields;
   - the emissive: Light's one diffuser master, which shades a Matter substance;
   - surface water and snow: Matter's law, shared through History.ush, with Nature keeping only bodies of water;
   - walkability: FMuseeHumanMeasure constants, with formulas at each user and a walked confirmation;
   - Blondel: Form;
   - the hour: a Foundation setting owned by Time;
   - traffic: Site computes the field, Matter applies it.
5. **Unowned responsibilities went to their natural trade, with no new system:**
   - luminaire operation and daylight control: Light;
   - fittings: Art, the exhibition designer;
   - doors: Site's joint;
   - traffic: Site;
   - streaming: Site's residency;
   - GPU attribution: Studio's pass map from the READMEs;
   - localisation: FMuseeText, gathered by Studio;
   - runtime stage identity: USiteStageInfo;
   - the enclosure query: Form::EnclosureAt, which also removed Sound's duplicate of RoomAt.

**Smaller fixes:**
- open courts are declared enclosure and opening kinds;
- the Grounds is handled by footprint kind and two passes, not by name;
- the piece layout fits Nanite, with piece-local UVs;
- MuseeMatterShaders isolates the PostConfigInit mapping;
- adapters are selected at runtime, because the installed engine does not link per target;
- dependency hashing is built into the one record reader.

**Naming:** the lint matches whole tokens. Check, command and probe names now follow their patterns. The overloaded words each got one meaning (Stage, Drum and Cylinder, Sweep and Travel, Describe, Detail, Scheme, Resolve).

Peers still meet only through Foundation, now by signals, channels and keys. Every channel key is declared by the stage that owns both ends, so no system names another. The system count stays at eleven: every new responsibility had a natural owner, and adding a system would have split a trade.
