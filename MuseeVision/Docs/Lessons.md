# Lessons

What the old museum taught, written down before the new one is built. The owner judges the old Rotunda and the old
Salon the best rooms, so this ledger records *why* they worked, *why* the other rooms fell short, which old code is
good enough to carry forward, and the exact numbers the Rotunda and its sun clock were built to. It records
**methods, not code**: where old code is named, it is named so that its idea can be re-expressed through the new
connectors, never copied in.

**How to read the references.** Paths are relative to the repository root. The old build lives in `windows/` and the
design in `plan/`; both are frozen and are carried unchanged on this branch (every file cited below was checked
against the branch's tree, and the two canvas files differ only in line endings). Line numbers refer to those files.
`windows/Saved/Logs` is local to the owner's PC and is not in the repository; it is cited only as corroboration.
Decisions D-001 to D-014 in `Journal/Decisions.md` override `Docs/Architecture.md` where the two differ, and this
ledger follows them.

---

## A. What worked in the Rotunda and the Salon, and why

### A1. A sealed shell with one planned daylight opening
**What was done.** The Rotunda was built as a closed solid whose only view of the sky is the glazed eye. The Salon's
roof is a slab open only over its five lanterns, the void between the vault and the roof is closed from the inside so
it stays dark, and the Manet cabinet's roof space is closed the same way.

**Why it works physically.** Ray tracing and Lumen carry light only along paths that exist. In a sealed shell every
photon enters through the opening the architect chose, so the room has one strong source with a true penumbra and
everything else is bounce. That is the "soft, deep bounced light, crisp contact shadows" the look targets ask for
(`windows/LOOK_TARGETS.md:11-13`). In the Rotunda's case the single opening is also the clock: the patch of sun
through the eye falls around the hour ring (A10).

**Evidence.**
- `windows/Source/MuseeVision/Rotunda/RotundaStructure.h:10-23`: "the only view of the sky is through the eye's
  glass".
- `windows/Source/MuseeVision/Salon/SalonStructure.cpp:1299-1330`: the roof is open only over the lanterns, whose
  shafts close its holes.
- `SalonStructure.cpp:1331-1341` and `:1388-1395`: the roof voids are closed from inside. The comment records the
  failure that taught this: from inside the roof space the sky showed through one-sided walls, and "any pinhole in
  the vault read as a bright dot".
- `plan/boards/AsBuiltClassical.png`, "The Rotunda · 4 PM": one sharp pool of sun on the floor, with every other
  surface in soft bounced light.
- `plan/renderings/05-the-rotunda.md`: "Natural sun is the only lighting in the Rotunda".

**Inherited by.**
- **Form**: `FFormEnclosure` kinds and `FFormOpening` kind Glazed, with `Form.Sealed` and `Form.MinThickness`, and
  `Form::FindLeakPaths`, which D-014 places in the runtime module.
- **The Rotunda stage** declares the eye as its only Glazed opening (D-004).
- **Light** admits daylight only through declared openings.

### A2. Mouldings as swept profiles
**What was done.** Every cornice, base, capital, archivolt, impost, architrave and skirting is a real section swept
along its path, with arcs kept smooth and straight runs creased.

**Why it works physically.** At close range a room reads as "museum" through the shadow lines of its mouldings: the
dark quirk under a corona, and the gradient on a torus. A box band has one flat shadow; a profile has the shading a
real carved or run moulding has. `windows/GEOMETRY_AUDIT.md:234-236` names this: "the profile is what reads as
'museum' at close range".

**Evidence.**
- *Rotunda.*
  - `RotundaStructure.cpp:84-136` is the section: straight runs and exact arcs, each piece carrying its own end
    tangents.
  - `:138-181` places a section by a function of the path parameter, so one sweep serves straight, arched, revolved
    and spherical runs. Its normals come from the swept surface's own derivatives.
  - The profiles themselves: `:550-653` (archivolt, imposts, niche surround and ledge), `:708-729` (pilaster base
    and capital) and `:855-877` (entablature).
- *Salon.* `windows/Source/MuseeVision/Salon/SalonKit.h:29-71` and `:237-281` are the sweep: smooth flags per
  profile point, with mitred plan runs split at hard corners. The profiles are at `SalonStructure.cpp:378-476`.
- *Classical Hall.* The richest vocabulary: `ClassicalHallKit.h:271-332` (`Round`, `Cyma`) and
  `ClassicalHallGeometry.cpp:367-455`, a full Ionic entablature with its cyma reversa, bed moulding, dentil band,
  ovolo, corona and sima.

**Inherited by.** **Form**.
- The drawing gets `FFormProfile` and a sweep in the one kernel (FormMaking `Private/Kernel/`).
- The catalogue gets `UFormCatalogue` over `Data/Form/Profiles.json` and `Orders.json`. The Rotunda's mouldings go
  there from day one, named for what they are.

### A3. Curves divided once, with the same stations shared by everything that meets them
**What was done.** Each circle or arch is divided into one set of stations, and every surface that meets it uses
those stations.
- *Rotunda.* One list of 280 angles runs round the dome: the cornice band, all five coffer rings, the eye's bands, the
  outer shell and the lattice ring share it. The drum's faces are divided at the arch stations of every opening, so
  the face, the reveal and the moulding meet vertex for vertex.
- *Salon.* 114 stations per semicircle of vault. The cornices, lunettes, spandrels and coffers use them. Door columns
  stand at the arch stations of each door.
- *Sun clock.* 768 stations round every circle, with the samples of the door and the port inserted wherever an
  opening crosses a circle.

**Why it works physically.** Two surfaces that meet along a curve but are faceted differently leave crescents open to
the sky, up to 6 cm deep on the old dome (`windows/GEOMETRY_AUDIT.md:37`). They also leave "faceted arches" where a
face and its reveal part by 3 cm (`:39`). Shared stations make the joint exact, so no light passes and no shading
seam appears.

**Evidence.**
- *Rotunda.*
  - `RotundaStructure.cpp:24-29` are the counts.
  - `:315-361` is `MakeStations`: arch stations across each opening, and at most `WallStep` between them.
  - `:830-851` is the shared dome angles.
  - `:922` and `:944-957`: coffer sides "run the same way as the neighbouring cells' and the plain bands' rings, so
    they share vertices".
- *Salon.* `SalonStructure.cpp:55` and `:160-223`, and `:1203-1247`, where the cornices sit "on the vault's
  stations".
- *Sun clock.* `windows/Source/MuseeVision/SunClock/SunClockBuild.h:12-15` and `:28`.

**Inherited by.** **Form**: `Stations(curve, tolerance)` in FormMaking `Private/Stations/`, with one chord tolerance
for the museum, and the check `Form.SharedStations`.

### A4. UVs in metres and tangents from the UVs
**What was done.** Every face carries UVs in metres (V down the walls), and the tangent frame is computed from how
those UVs actually run across each triangle.

**Why it works physically.** A normal map describes relief in the texture's own directions. The tangent must point
where U grows, or the relief turns the wrong way under grazing light. Metre UVs keep a travertine pore the same size
on a 28 m wall and on a 0.3 m impost.

**Evidence.**
- *Tangents.*
  - `RotundaStructure.cpp:242-265` is `ComputeTangents`: per triangle, from the UV gradients, made orthogonal to
    each vertex normal, with the bitangent sign.
  - `SalonKit.h:190-234` accumulates the same per shared vertex.
- *Metre UVs.* `RotundaStructure.cpp:13-14`, and `SalonKit.h:10` and `:21-27` (`FaceUV`).

**Inherited by.**
- **Form**: FormMaking computes MikkTSpace tangents from piece-local metre UVs. The check is `Form.NormalsContinuous`.
- **Matter**: `FMatterPieceLayout`, with UV0 in piece-local metres.
- **Studio**: the caster keeps Form's normals and tangents, under `Studio.CastRules`.

### A5. Shared vertices at seams, and strips zipped without T-junctions
**What was done.** Where a strip has different break heights at its two edges (a sill on one side, an arch head on
the other), it is triangulated by zipping the two chains of vertices, so no vertex lies in the middle of another
triangle's edge. Patches take their normals from the grid's own differences, so they are smooth inside and crisp at
their edges.

**Why it works physically.** A T-junction opens a sub-pixel crack under a baked Nanite mesh, and that crack shows the
sky. Normals averaged across a real arris round off an edge that a mason cut sharp.

**Evidence.**
- *Rotunda.* `RotundaStructure.cpp:374-393` (`Zip`) and `:963-1000` (the drum's top, zipped by angle).
- *Salon.*
  - `SalonKit.h:152-188` (`Patch`).
  - `SalonStructure.cpp:85-87`: "a baked Nanite mesh can open a hairline where two edges only touch, and the sky
    showed through it".
- *Classical Hall.* `ClassicalHallKit.h:89-157`: an ear-clipper that refuses any ear with another vertex on its
  edges, so the outline keeps every vertex.

**Inherited by.** **Form**: the one kernel and triangulator in FormMaking `Private/Kernel/`, with the checks
`Form.Sealed` and `Form.NormalsContinuous`.

### A6. Light slots designed into the architecture
**What was done.** The Salon's cornice has a lip that hides a trough washing the vault. There is a slot under the
cornice and a slot in the skirting. The architecture published these as lines with a facing and a slot width, and
the lighting was placed in them. The Classical Hall's entablature has "the lip hides the cove light". The sun clock's
drum hides twelve spots in its coffers.

**Why it works physically.** In a real gallery the source is concealed in the building and only its effect is seen.
A believable room needs light that comes *from somewhere the architecture allows*.

**Evidence.**
- *Salon.*
  - `SalonStructure.cpp:48-52` (the slot positions) and `:378-398` (the cornice and skirting profiles that form
    them).
  - `SalonStructure.h:10-41` (`FSalonLightLine`) and `SalonStructure.cpp:1754-1840` (`PlanLightLines`).
  - `windows/Scripts/gallery_lights.py:144-187` places strips in the slots.
- *Classical Hall.* `ClassicalHallGeometry.cpp:383`.
- *Sun clock.* `SunClock.h:30-33` and `SunClockBuild.h:155-160`.

**Inherited by.**
- **The stage** declares luminaire mounts in `Describe()`, and the profile that forms a slot lives in
  `Data/Form/Profiles.json`.
- **Light**:
  - `Scheme(targets, mounts)` chooses catalogue luminaires for those mounts.
  - The checks are `Light.LuminairesFromCatalogue` and `Light.Operated`.
  - D-014 lets the Salon's half-light vault be stated as a measured design target, as a luminance ratio.

### A7. Real pieces with real joints
**What was done.** Each of the Salon's transverse arches is fifteen voussoirs and a keystone. Each stone is its own
piece with its own slice of the travertine photograph (a deterministic seed per stone). Its tone and polish go in
vertex colour, its joints are V-chamfered 8 mm as geometry, and the keystone stands 20 mm proud.

**Why it works physically.** Masonry is read through its joints and through the small differences from stone to
stone. A continuous texture with painted joints reads as wallpaper. The as-built board sums up the method: "each
block takes its own part of the photograph, so no two neighbours repeat" (`plan/boards/AsBuiltClassical.png`,
"Photographic stone").

**Evidence.** `SalonStructure.cpp:78-82`, `:642-648` (`Hash01`) and `:669-799` (`Voussoirs`).

**Inherited by.**
- **Form**: `Lay()` cuts surfaces into blocks within stock, with real arrises. The check is `Form.WithinStock`.
- **Matter**: the per-stage piece table indexed by PieceId holds the seed and the bed frame, and the check is
  `Matter.Calibrated`.
- **Foundation**: `Musee::Seed(FMuseeKey)` replaces the integer hash.

### A8. Details solved where they meet
**What was done.**
- *Rotunda.* The pilaster's base and capital break wherever the flutes do, so the shaft meets them vertex for vertex.
  The pilasters beside a door step out just far enough that their widest part clears the jamb, instead of standing
  in the opening. A 150 mm shadow gap separates the drum from the floor.
- *Salon.* Skirtings stop inside the door architraves and die into the pier bases. The silk wraps the pier faces with
  a 0.14 m stone edge left at each jamb, as an upholsterer would finish it.

**Why it works physically.** A real building has no two members occupying one place, and every termination has a
reason. The eye finds an unresolved meeting at once.

**Evidence.**
- *Rotunda.* `RotundaStructure.cpp:731-791` (the pilaster), `:793-815` (`DoorFlank`) and `:30-33` (the shadow gap).
- *Salon.* `SalonStructure.cpp:1249-1266` (skirtings) and `:626-640` (`PierSilk`).

**Inherited by.**
- **Form** composes pieces and runs `Form.NoOverlap`.
- **The stage's plan** holds the Rotunda's own rules for its singular details (`Private/RotundaPlan.h`).

### A9. Moving parts checked at many states
**What was done.** The sun clock's rising cylinder, and the balustrade that retracts into its housings, were tested
triangle against triangle. The test ran at named lifts, at ±5 mm margins, and at 200 samples of the eased motion. It
also measured the least clearance under the moving ceiling. The drum never lowers onto a visitor: a lowering drum
reverses if the visitor comes under it.

**Why it works physically.** A real mechanism must clear everything in every state it passes through. Checking only
"up" and "down" misses the clash half-way.

**Evidence.**
- `windows/Source/MuseeVision/SunClock/SunClockChecks.cpp:229-525`: the cases at `:434-450` and the dense sweep at
  `:478-509`.
- `SunClock.h:35-54`: the dwell cam and the housings.
- `SunClock.cpp:384-389`: the reversal.

**Inherited by.**
- **Motion**: `Musee::Motion::Configurations()`, with the checks `Motion.Clears` (at least 5 mm at every sampled
  phase), `Motion.Housed` and `Motion.Interlocked`, and the runtime occupancy guard D-014 gives Motion.
- **Studio** assembles the inputs.

### A10. The sun, the eye and the floor designed as one instrument
**What was done.** The node in the eye's lattice casts a point of shadow onto a bronze hour ring. The idealised sun
moves 15° an hour, so the shadow lands on the hour, and the patch of sun through the eye is centred on that same
point.

**Why it works physically.** The room's light, its ornament and its purpose are one object. The single opening (A1)
is both the source of light and the gnomon's aperture. D-003 keeps this design.

**Evidence.**
- `windows/Source/MuseeVision/Sky/MuseeSky.cpp:109-116`.
- `SunClockDial.cpp:3-31`.
- `plan/canvas/Rotunda.dc.html`, the plan and the section: "10:00 · the point of shadow falls on X". Checked on the
  board: the point lies 101.7 px = 6.78 m out, at 15 px/m, in the direction the code gives.

**Inherited by.**
- **Time**: the sun model in `Data/Time/Museum.json` (`idealised` by default), `Time::Dial` and `Time.DialTrue`.
- **The Rotunda** lays its dial from `Time::Dial`, never the reverse.

---

## B. What went wrong, and what now prevents it

The best rooms are included where they share a fault. Each entry names the owner in the new build and the check that
stops the fault from returning.

### B1. Per-room tuning of exposure, colour and light
**What was done.**
- *Exposure zones.*
  - The Albion court had exposure −0.2 (`windows/Scripts/exposure.py:28-33`).
  - The Salon bays had +0.35 with its own local-exposure contrast (`windows/Scripts/salon_interior.py:996-1020`).
  - The Reserve had its own white balance of 4300 K (`windows/Scripts/native.py:117-121, 350-365`).
- *Camera settings.* A museum-wide +0.9 bias, an exposure-compensation curve by scene EV (`exposure.py:22-25`), and
  local-exposure highlight and shadow squashing (`windows/Scripts/setup_project.py:588-604`).
- *The Salon's half-light vault* was reached with three knobs at once:
  - the plaster's base colour "a shade down from the Rotunda's" (`salon_interior.py:284-295`);
  - the cove lumens set "at half" (`gallery_lights.py:53`);
  - its exposure zone.
- *Painting light above care limits.* 300 lux "a little above conservation levels … so they still read against
  daylight in a game", and 650 lux in the Salon (`windows/Scripts/gallery_lights.py:27-34`).
- *A fill level.* `LUX_SALON_FILL` (`:47`).
- *A factor on lumens.* `LUMENS_SCALE = 0.06` (`windows/Scripts/relight.py:38-40`).
- *A sky tint.* The sky light was tinted warm so that the stone "reads neutral" (`MuseeSky.cpp:59-62`).

**Cause.** Each room was tuned until it looked right, so nothing was true in two rooms at once, and every later change
had to be re-tuned everywhere.

**Now.**
- **Visitor**:
  - One Eye with reflected metering and one constant (K = 12.5).
  - A tone curve with a highlight shoulder, per D-014.
  - The check `Visitor.OneEye`: camera fields exist only in the Eye's volume, so a zone cannot exist.
- **Light**: `Light.OneTransport`, and `Light.MeetsTarget` with design targets as measured luminance ratios (D-014).
- **Art**: `Art.WithinCare`. Oil is limited to 150–200 lux, and the limit exists only in `Care.json`.
- **Matter**: plaster keeps its substance's albedo. There is no finish override in the description.
- **Studio**: `Studio.Wiring`, under which Python holds no numbers, and the banned-word lint (Fill, Brightness,
  Multiplier, Scale inside Light).

### B2. Flat-colour materials with no substance and no history
**What was done.**
- Gilt was a constant colour (`#C9A266`), metallic, with roughness 0.3 (`setup_project.py:560-570`).
- The Salon's stele foot carried brass colour and roughness as editable properties on the room
  (`SalonStructure.h:266-270`, `SalonStructure.cpp:2055-2073`).
- Every part had a fallback material, so a map could show an older look for one part without anyone noticing
  (`SalonStructure.cpp:2003-2054`, `SalonStructure.h:262-264`).
- On the boards:
  - the exterior is one clean white with no weathering (`plan/boards/AsBuilt.png`);
  - the Chinese Wing's ground and rock are flat colour (`AsBuiltChinese.png`);
  - the Élan drum is an even white (`AsBuiltElan.png`).

**Cause.** A colour was chosen for how it looked, not derived from what the thing is, how it was worked, and what
time has done to it.

**Now.** **Matter**:
- `FMatterSpec` = substance × working × history, with the evidence tiers.
- Gilt is a *working* of gold leaf over bole on a substrate (D-014).
- Brass is a declared accent of the bronze family (D-014).
- One resolver.
- The checks `Matter.OneLanguage`, `Matter.Plausible` and `Matter.Calibrated`.
- `Studio.Wiring` allows no asset path outside its owner, so there are no fallback paths.

### B3. The FMuseeMesh tangent bug
**What was done.** `windows/Source/MuseeVision/Geometry/MuseeMesh.h:30-31` sets every vertex tangent to the
normalised first edge of its triangle, `(B - A)`, and never flips the bitangent. This has four consequences:
- The tangent follows whichever edge was listed first, not the direction in which U grows.
- `Quad()` builds its two triangles as `(A, B, C)` and `(A, C, D)`, so their tangents run A→B and A→C. The normal
  map's frame turns across the diagonal of every quad (by 45° on a square quad), and the relief creases there.
- The tangent is not projected off the vertex normal, so on smooth-shaded curves the frame is skewed.
- Mirrored UVs get the wrong bitangent.

The mesh is also unindexed, with three vertices per triangle and no sharing. Its `Lathe` (`:51-75`) gives one normal
per profile edge, which flat-shades every ring. It also normalises UVs to 0–1, so a texture's scale depends on the
object's size.

The bake kept the bad tangents. It set `bRecomputeTangents = false` and Nanite `bExplicitTangents = true`
(`windows/Source/MuseeVisionEditor/MuseeBakeLibrary.cpp:394-395, 432`), so the fault reached the final image in:
- the Reserve (`ReserveStructure.cpp:1268`);
- the Élan drum, ribs and lattice (`ElanStructure.cpp:398-400, 503-509`, `ElanKit.cpp:283`);
- the Élan exterior (`ElanExteriorStructure.cpp:1220`).

The Rotunda also builds with `FMuseeMesh`, but it overwrites the tangents from its UVs (`RotundaStructure.cpp:1130-1133`),
which is one reason it looked right.

**Cause.** A helper wrote a plausible-looking tangent, and nothing measured it.

**Now.**
- **Form**: one kernel, indexed meshes, MikkTSpace from metre UVs, and normals by one crease rule. The check is
  `Form.NormalsContinuous`.
- **Studio**: the caster's rule is stated once and checked by `Studio.CastRules`.

### B4. Box geometry
**What was done.**
- The Hall of Light's lattice was 32 straight boxes per arch, with wedge gaps and overlaps at every joint
  (`windows/GEOMETRY_AUDIT.md:157-159`).
- The oval benches were a chain of boxes (`:41`).
- The Reserve's first columns were 0.5 m brick boxes (`:114-118`).
- The Sculpture Hall's mouldings were rectangles (`:133`).
- The laylit wings' roofs were engine cubes 0.4 m thick laid over them (`relight.py:176-185, 249-270`).
- The Chenghuai code alone calls `Box(` 186 times across three files.
- Even the Rotunda's lattice bars are boxes crossing through one another (`RotundaStructure.cpp:1017-1056`).

**Cause.** A box is the cheapest solid. It has no section, no joint and no arris, so it reads as a placeholder.

**Now.**
- **Form**:
  - The drawing vocabulary is closed: sweep, lathe, extrude, loft, surface, laid surface and tubes.
  - A straight member is an extrusion of its real section: a steel T, an angle, a moulded rail.
  - The checks are `Form.NoOverlap` and `Form.SharedStations`.
- **Motion**: machine elements come back as Form drawings.

### B5. Rooms left procedural
**What was done.** Each room was a procedural actor that rebuilt itself in `OnConstruction`, and a separate bake
converted it into static meshes afterwards. The bake skipped:
- anything that ticked or that the visitor could use;
- any section whose material was made at run time.

This is in `windows/Source/MuseeVision/Geometry/MuseeBake.h:7-28` and `MuseeBakeLibrary.cpp:135-157, 159-191`. The
comment at `:140-141` records that "the Chinese Wing stayed procedural, outside Lumen's surface cache". The local bake
logs of 25 September repeatedly show the Rotunda, the Reserve, the Hall of Light, the Chinese Wing and the Élan
exterior "left procedural: it ticks". Parts were left out one at a time for runtime materials or `musee.nobake` tags:
- the Salon's stele foot;
- the Classical Hall's laylights;
- the Élan's misty glass and iris.

**Cause.** Making and living were not separated. Geometry was built while the level loaded, so what the visitor saw
depended on whether a bake had run.

**Now.**
- **The two phases, enforced by module type.** Rooms are Editor modules that only describe themselves. Studio alone
  makes and casts.
- **`ProceduralMeshComponent`** is owned by nowhere (Architecture, "The wiring", rule 5).
- **Moving bodies** are merged per rigid body and moved by Motion.
- **Checks**: `Studio.CastRules` and `Studio.Incremental`.

### B6. Emissive panels standing in for light
**What was done.**
- *Laylights, velarium and skylights.* `M_Daylit` is an unlit emissive surface in nits × a "Daylight" curve
  (`setup_project.py:109-127`). Lit only by that glow, the Nymphéas walls came out 3.6 stops below the velarium
  instead of about 1.3, so each panel got a hidden rect light sized by a `fill` factor of 0.85
  (`relight.py:187-199, 208-237`).
- *The Daylight curve* was `sqrt(sin(π(h − 6)/12))`, with a night floor, chosen to look right
  (`MuseeSky.cpp:139-145, 154-185`).
- *Misty glass.* `M_MistyGlass` glowed at 450 nits × a cloud texture (`setup_project.py:258-310`).
- *The iris.* `M_IrisGlow` was unlit at 3000 nits (`:571-576`).
- *The Classical Hall.* Self-lit diffusers with rect lights behind them, and a switch to make the underground
  laylights follow the sky (`ClassicalHallStructure.h:56-63, 105-119`).
- *A painted sky.* The Salon's "sky panels" over the lantern glass painted Giverny's cloud, and were simply hidden when
  their material was missing (`SalonStructure.h:152-158`, `SalonStructure.cpp:1012-1015, 2026-2031`).

**Cause.** A glowing surface was used as a picture of light, and a light was then added to make the picture light the
room.

**Now.**
- **Light**:
  - One emissive master, `M_Light_Diffuser`. It shades a Matter substance such as opal glass, and its nits come from
    a luminaire record driven by the same `ULightLuminaireComponent`.
  - Real daylight enters through Glazed openings with real glass.
  - The checks are `Light.EmissiveHasEmitter` and `Light.Operated`.
- **Matter**: `Matter.NoEmission`.
- **Time**: remote weather on a room's glass comes from Time's keyed observers (D-005), not from a painted panel.

### B7. Fill lights and lights without fittings
**What was done.**
- *The Élan.* A rect light "no one sees; diffuse only, no shadows" with no specular (`ElanSphereTop.cpp:327-340`).
  Shadowless rib uplights (`ElanStructure.cpp:320-336`).
- *The Reserve.* "Four shadowless uplights per pier" (`plan/boards/AsBuiltReserve.png`, "Light"). Globes that cast no
  shadow (`ReserveStructure.cpp:1169`).
- *The best rooms had the same fault.*
  - The sun clock's drum carries "a soft fill with no fitting" (`SunClock.h:145-151, 204-206`).
  - The Salon's wall washers are rect lights floating 2.5 m out from the wall at 6.4 m, with barn doors and no
    housing (`gallery_lights.py:188-221`).

**Cause.** When a region read too dark, light was added where it was needed rather than where a fixture could hang.

**Now.**
- **Light**: every luminaire is a catalogue product with a housing drawing and a control class, and the raw light
  components are Private. The checks are `Light.LuminairesFromCatalogue` and `Light.Operated`.
- **The description** has no fill-light method (Architecture, "How the principles are carried").

### B8. Patching the renderer's leaks by position
**What was done.**
- `AMuseeSky::UpdateUnderground` faded the whole sky light whenever the camera was under the Élan. Lumen's radiance
  cache had carried the sky through 1.5 m of slab (`MuseeSky.h:144-150`, `MuseeSky.cpp:231-244`).
- `GEOMETRY_AUDIT.md:187-189` proposed "a thin blocker (5 cm) behind each jamb".
- Lantern glass was set not to cast shadows so that the sun would pass through (`SalonStructure.h:141-143`).

**Cause.** A symptom was treated where it appeared.

**Now.**
- **Form**: `Form.Sealed` and `Form.MinThickness` first.
- **Light**: then `Light::FindTransportLeaks`. The fix goes into Light's transport or Form's granularity rule for
  every stage, and never into a blocker plane (Architecture, "Defect routing").
- **Glass**: glass that turns black is sorted with the path-traced reference.

### B9. Collision that is not the visible solid
**What was done.**
- The sun clock's stair was walked on a hidden ramp through the treads' middles, with hidden fences along the
  balustrade (`SunClock.h:55-58, 133-135`).
- The Reserve had its own hidden `Walkway`, which was left procedural in the bake.

**Cause.** The old character did not climb the true steps reliably.

**Now.**
- **Form**: `Form.CollisionMatchesVisual`, and `Form.StepsFitTheBody` (Blondel over the body's pace).
- **Visitor**: `AVisitorBody` takes its step height and slope from `FMuseeHumanMeasure` (`Visitor.BodyFromMeasure`).
- **Site**: `Site.ReachWalked`.

### B10. Sealing by overlap
**What was done.** The best rooms also closed their joints by running one surface on into the other:
- 1 cm for the Rotunda's mouldings (`RotundaStructure.cpp:16-18, 34-35`);
- 2 mm, 4 mm and 30 mm run-ons in the Salon (`SalonStructure.cpp:24, 85-87`).

This worked for pixels. But it means two solids occupy the same place, and it hides whether the shell is really
closed.

**Now.**
- **Form**: pieces meet at real joints and do not interpenetrate (`Form.NoOverlap`). Closure is proved by the leak
  probe (`Form.Sealed`), not hidden by an overlap.
- **Where a moulding sits on a wall**, the joint is a real bed between the two pieces, drawn by Form.

### B11. A room that reaches into its neighbours
**What was done.** `RotundaStructure.cpp:3` includes `Chenghuai/ChenghuaiPlan.h` and branches on
`Chenghuai::bNorthDoorOpen` (`:309-312`). It also builds the passages to its neighbours itself
(`:655-670`, and `MuseePlan.h:61-63`).

**Now.**
- **Site**: ports and the one joint built at every seam, with the checks `Site.RoomStaysInside` and `Site.PortsMate`.
- **Studio**: `Studio.Wiring` fails any stage-to-stage reference.

### B12. One number in two homes
**What was done.**
- *The node height.* The node is drawn at 20.25 m (`MuseePlan.h:59-60`), but the sun is aimed from the lattice at
  20.3 m (`MuseeSky.cpp:114`). The node's centre therefore casts its shadow at r 6.763 m, not on the hour ring's centre
  line at 6.78 m. The ring is 6.76–6.80 m, so the shadow grazes its inner edge.
- *The dial's rim.* The ring's outer edge is a 128-gon, to match an imported floor that no longer exists
  (`SunClockDial.cpp:177-182`).

**Now.**
- **Time**: `Time::Dial` takes the nodus from the Rotunda's drawing, and `Time.DialTrue` checks the dial against the
  declared sun model.
- **Form**: every circle is divided by the one chord tolerance.

---

## C. Old code worth carrying forward, re-expressed

Each candidate is judged against `Docs/Craft.md`. "Tier" follows D-013: tier 1 is what the Rotunda and the mock-up bay
need. Where the Architecture names no public header for a needed connector, that is marked as a gap for the Register.
It is not invented here.

### C1. The Meeus ephemeris and the museum clock
**Where.**
- *Declarations.* `windows/Source/MuseeVision/Sky/Ephemeris.h:5-61`.
- *The almanac.* `windows/Source/MuseeVision/Sky/Ephemeris.cpp`:
  - `:9-38`: J2000 days, obliquity, the moon's ecliptic position, ecliptic to equatorial, sidereal time;
  - `:145-192`: Julian day, the sun's apparent longitude, the sky's rotation, sun, moon and solar terms;
  - `:57-71`: the 24 terms' names.
- *The clock.* `Ephemeris.cpp:84-143` (`MuseeClock`): Beijing UTC+8, and New York with US daylight saving.

**Good.**
- Small pure functions.
- The sun follows the Astronomical Almanac's short formulae, good to about 0.01° this century.
- Solar terms come from the apparent longitude.
- The New York daylight-saving rule is correct: the second Sunday of March and the first of November.

**Must change.**
- *Accuracy.* The moon has four longitude terms and one latitude term, and no topocentric parallax (up to about 1°).
  That misses Time's 0.01° claim, so the moon becomes Meeus ch. 47 with parallax. The sun gains refraction, the
  equation of time and rise and set. The check is `Time.EphemerisMatchesReference` against published NOAA or JPL
  Horizons tables.
- *The axis detour.* Coordinates pass through RealityKit axes (`:45-55`). They go directly into `FMuseeFrame`
  (x east, y south, z up).
- *Settings.* The CVars `musee.City` and `musee.Hour` (`:79-82`, `MuseeSky.cpp:19-22`) become `FMuseeSetting`s owned
  by Time: `time.hour`, and the city per D-002 and D-003. The commands are `musee.time.advance-hour` and the city key.
  Here D-002 and D-003 override the Architecture's "no runtime city command".
- *A daylight-saving slip.* `UtcNow` subtracts the offset for the *real* moment, not for the fixed hour (`:120-124`),
  so near a daylight-saving change a fixed hour is an hour out.
- *Typing.* Typed units, and named constants with their source (J2000 = JD 2451545.0, the GMST polynomial).
- *Separation.* The clock and the almanac are separate types in separate files.

**Becomes.** **Time**: `Private/Almanac/` and `Private/Calendar/`, `FTimeObserver` in `Public/TimeObserver.h`, and
the connector `Musee::Time::Sun/Moon(Utc, Observer)`. The tree lists no public header for these functions: gap. By
the one-idea rule it would be `TimeAlmanac.h`. **Tier 1.**

### C2. SalonKit's indexed mesh builder and sweeps
**Where.** `windows/Source/MuseeVision/Salon/SalonKit.h`:
- `:86-235`: `FMeshData`, the indexed builder. Winding comes from the normals, degenerate triangles are dropped,
  `Patch` takes its normals from grid differences, and `Write` builds tangents from UVs.
- `:29-71`: the profile, with smooth flags.
- `:237-281`: `Sweep`.
- `:283-357`: `PlanRuns`, mitred plan runs split at corners sharper than 20°.

**Good.**
- Indexed and shared.
- The "face along the given normals" winding means a caller never reasons about winding order.
- Mitred corners, with a hard or smooth corner chosen by angle.
- Metre UVs.

**Must change.**
- *Output.* It writes a `UProceduralMeshComponent` in centimetres (`:17`, `:103-108`, `:191-234`). The kernel outputs
  `FFormPieces` in metres, and `Musee::ToEngine` applies only in Studio's caster.
- *Tangents.* The per-vertex accumulation becomes MikkTSpace, so that the caster and Nanite agree.
- *Segment counts.* The counts are fixed per call (`Arc(…, 8)`). They become stations from the one chord tolerance.
- *Open shells and fans.* `Box` with selectable faces (`:139-150`) produces open shells. `Poly` fans convex polygons
  only (`:125-132`). The kernel's triangulator (C4) replaces both.
- *The mitre clamp.* `0.2` at `:310` becomes a named constant, with its reason: it limits the mitre to five times the
  section at sharp corners.
- *Vertex colour.* `Paint` becomes `FMatterPieceLayout`: vertex colour is cavity, wash, arris and a spare, and the
  PieceId goes in UV1.

**Becomes.** **FormMaking**, `Private/Kernel/` (builder, sweep) and `Private/Stations/`. The data types
`FFormDrawing` and `FFormProfile` go in MuseeForm `Public/FormDrawing.h` and `FormProfile.h`. **Tier 1.**

### C3. The Rotunda's arc profiles, sweeps and shared stations
**Where.** `windows/Source/MuseeVision/Rotunda/RotundaStructure.cpp`:

| Lines | What |
|---|---|
| `:84-136` | The exact line and arc profile, with end tangents per piece |
| `:138-181` | Placement by a function of the path, and normals from the swept surface's derivatives |
| `:183-240` | Ear-clip triangulator and caps |
| `:315-361` | Stations shared across openings |
| `:374-393` | `Zip` |
| `:399-498` | Wall faces with the shadow gap, and reveals |
| `:672-791` | The fluted pilaster, whose base and capital break with the flutes |
| `:793-815` | Pilasters stepped clear of the doors |
| `:817-961` | The dome's shared angles and the coffers: one step profile scaled per cell |

**Good.**
- One sweep serves every run shape, because the path is a placement function.
- Normals are exact, not taken from the facets.
- Neighbours break where each other break.

**Must change.**
- *Indexing.* It builds an unindexed `FMuseeMesh` (B3). The kernel is indexed.
- *Derivatives.* Finite differences with a fixed 1e-5 step (`:37`, `:148-156`) become analytic derivatives, or keep
  the difference with its error stated.
- *Counts.* The fixed counts (`ArchSegments` 48, `CofferSamples` 10, `WallStep` 320 per turn) become stations from
  the chord tolerance under `Form.SharedStations`.
- *Numbers.* The profiles' numbers go to `Data/Form/Profiles.json`, named for what they are, and the plan numbers to
  `Rooms/Rotunda/Private/RotundaPlan.h` (section D).
- *Overlap sealing.* The 1 cm overlaps go (B10).
- *Silent failure.* The triangulator gives up silently (`:218`); failure becomes a finding.
- *What goes entirely.* The Chenghuai include and the passage shells go (B11). So do materials by path string
  (`:1166-1173`).

**Becomes.** **FormMaking** `Private/Kernel/` (the placement-function sweep) and `Private/Stations/`. **MuseeForm**
`Public/FormProfile.h` and `FormCatalogue.h`. **The Rotunda stage** `Private/RotundaDrum.cpp`, `RotundaOrder.cpp` and
`RotundaDome.cpp`, as descriptions that call none of this directly. **Tier 1.**

### C4. The Classical Hall's kit and profiles
**Where.**
- *The kit.* `windows/Source/MuseeVision/ClassicalHall/ClassicalHallKit.h`:
  - `:45-87`: a mesh in metres, CoreMinimal only;
  - `:89-157`: the ear-clipper with no T-junctions, which returns false when it fails;
  - `:182-242`: polygon with one hole, joined by a bridge;
  - `:244-269`: `Zip`;
  - `:271-332`: the profile language `Add`, `Arc`, `Round` and `Cyma`;
  - `:544-586`: `Lathe`.
- *The profiles.* `ClassicalHallGeometry.cpp`:
  - `:367-455`: Ionic entablature, skirting, dado rail, Attic base, surround, door architrave and panel frame;
  - `:457-467`: a wall moulding turned into a lathe section;
  - `:503-534`: the column: Attic base, entasis over the upper two thirds, astragal and echinus.
- *The order.* `MuseePlan.h:492-496`: column 0.34 m, 9 diameters to the abacus.

**Good.**
- The profiles are written in the trade's words: fillet, torus, scotia, ovolo, cyma recta and cyma reversa.
- The kit depends on nothing but CoreMinimal, so it can be tested outside the engine.
- It has the best triangulator of the three kits.

**Must change.**
- *Round.* `Round` infers a quarter circle from the chord (`:296-319`), which is ambiguous for any chord not at 45°.
  The elements become explicit, each with its projection and height.
- *Modules.* Numbers are absolute metres. The order is written in modules of the lower diameter D, after Vignola, so
  it scales.
- *Segments.* The 3–5 segments per round become the chord tolerance.
- *Entasis.* The `1 − cos` entasis becomes a stated construction with its source.
- *Development commands.* The spawn and trace commands in `ClassicalHallChecks.cpp` go.

**Becomes.**
- **MuseeForm**: `Public/FormProfile.h` (`FFormProfile`, whose elements are the trade's), `FormOrder.h`
  (`FFormOrder`), and `UFormCatalogue` over `Data/Form/Profiles.json` and `Orders.json`.
- **FormMaking** `Private/Kernel/`: the triangulator.

**Tier.** The triangulator and the profile language are tier 1. The Ionic order is tier 2: the Classical Hall comes
after the Rotunda, and its order is promoted into `Orders.json` when it is specified.

### C5. The sun clock's checks
**Where.** `windows/Source/MuseeVision/SunClock/SunClockChecks.cpp`:
- `:44-98`: open edges after welding at 0.05 mm, grouped by place;
- `:100-149`: fixed parts kept out of the lowered drum's volume;
- `:151-215`: tread tops against the walked surface along five walking lines, with 2R + G and headroom reported;
- `:250-317`: segment-through-triangle crossing, and a 20 cm hash grid;
- `:319-525`: moving against fixed at named phases and ±5 mm margins, a 200-sample sweep of the eased lift, and the
  least clearance under the moving ceiling;
- `:539-595`: the report, with OBJ dumps.

**Good.** It is the right idea: pure geometry over explicit configurations, with margins stated as cases, dense
sampling of the motion, and findings located in the room's own polar coordinates.

**Must change.**
- *No verdict.* It is a console command that writes a text file and asserts nothing. Expected open edges are named in
  prose (`:563-570`), not declared.
- *One truth, one owner.* In the new build each truth goes to its owner:
  - open edges become `Form.Sealed` over declared openings;
  - fixed–fixed crossings become `Form.NoOverlap`;
  - moving–fixed becomes `Motion.Clears` (at least 5 mm) with the phases from `Musee::Motion::Configurations()`,
    not hand-listed;
  - steps become `Form.StepsFitTheBody` from `FMuseeHumanMeasure`;
  - headroom becomes the human measure's minimum headroom.
- *Tolerances.* `1e-7`, `0.005` and the rest become named constants.
- *What it measured.* The thing it measured, a hidden ramp, goes (B9).
- *Commands.* The spawn and lift commands (`:604-650`) belong to Instruments and Studio.

**Becomes.** Foundation's `MUSEE_CHECK` (`Public/MuseeCheck.h`). **Form** (`Private/Solidity/`, `Private/Sealing/`).
**Motion** (`Motion::MeasureClearance`, `Motion.Clears`). **Studio** assembles `FFormAssembly` and
`FMuseeConfiguration`. **Tier 1**: the cylinder is the Rotunda's first moving body.

### C6. The headset controls
**Where.** `windows/Source/MuseeVisionVR/MuseeVRCharacter.cpp`:

| Lines | What |
|---|---|
| `:287-341` | Bindings: Quest Touch first, Index second, desktop test keys |
| `:418-481` | Bare-hand pinches: close under 2.0 cm, open over 3.5 cm, the closest finger wins, the trigger ignored while the hand is tracked |
| `:386-405` | The aim ray: shoulder through pinch for a bare hand, else the controller's aim pose, else gaze |
| `:531-544` | A 30° snap turn about the head |
| `:546-553` | Recentre with the eyes at 1.6 m |
| `:659-675` | The body follows the head, stopped by walls |
| `:677-740` | The step: a projectile arc at 7.5 m/s over 2.5 s. It lands on floor whose normal Z ≥ 0.7, falls back 0.5 m from a wall, never lands above the eyes, and is taken as a 0.08 s / 0.18 s blink |

The scheme is described for the owner in `windows/VR.md:50-81`.

**Good.** A comfortable scheme, tested on the owner's Quest 3.
- Pinch hysteresis stops chatter.
- Turning about the head avoids a sideways lurch.
- The arc can only land where a person could stand.

**Must change.**
- *Structure.* It is a subclass of the desktop character with its own actions. It becomes one `AVisitorBody` with
  the headset adapter behind `IVisitorAdapter`. Every action is a `FMuseeCommand` intent: walk, stride, hop, look,
  sit, step-to, the hour key and the city key (D-014).
- *Measures.* `StepFloorZ` and the eye height become `FMuseeHumanMeasure`. `ECC_Visibility` traces become Foundation's
  Walkable and Clearance channels.
- *Headset quality.* `ApplyHeadsetQuality` sets `sg.*` and `r.*` directly (`:60-85`). This is split: sampling (screen
  percentage, upscaler) goes to the Eye's `visitor.sampling`, and transport to Light's `light.profile headset`.
- *Signs.* `MuseeVRExposure` lifts world-space widgets by the inverse of eye adaptation
  (`MuseeVRExposure.h:7-12`). The overlay is composited after tone mapping instead, as stereo layers
  (`Visitor.OverlayOutsideScene`).
- *Hard-coded values.* The literal offsets (`:39-58`) and the hour steps 10, 16 and 21 (`:555-563`) become named
  constants and Time's command.

**Becomes.** **MuseeVisitorHeadset** `Private/` (built only in the VR target, D-010), with actions in **MuseeVisitor**
and the check `Visitor.ActionsCovered`. **Tier 1**: a headset session is one of the specimens before the Rotunda.

### C7. The Nanite static-mesh settings
**Where.** `windows/Source/MuseeVisionEditor/MuseeBakeLibrary.cpp`:
- `:262-351`: `Describe`, a MeshDescription with one polygon group per material and every UV channel in use;
- `:389-408`: the build settings. Normals and tangents are not recomputed, MikkTSpace and the high-precision tangent
  basis are on, UVs are full precision, there are no lightmap UVs, distance fields are built (two-sided when the
  material is), and `MaxLumenMeshCards` is 32 for meshes longer than 10 m;
- `:427-444`: Nanite with explicit tangents and a fallback of 100 % of triangles;
- `:447-463`: collision;
- `:556-580`: marking materials for Nanite use;
- `:671-688`: building 24 meshes at a time. One batch of about 270 peaked at 100 GB.

**Good.** These are the right switches, each learned from a failure: UV precision on long walls, Lumen cards on
whole rooms, and memory.

**Must change.**
- *Source.* It converts procedural components after the fact. The caster takes `FFormPieces` directly.
- *Tangents.* Explicit tangents are kept only because Form now computes MikkTSpace itself. The rule is stated once
  under `Studio.CastRules`.
- *Glass and water.* They are chosen as non-Nanite by blend mode (`:122-127`). They should come from the Matter family
  declared on the piece.
- *Collision.* Complex-as-simple double-sided collision becomes Form's collision from the same solids, on
  Foundation's profiles.
- *Asset paths.* `/Game/Museum/Baked` becomes `/Game/Generated/Stages/<Stage>/SM_<Stage>_<Piece>`.
- *Copying settings.* The reflection-based copying of render settings (`:467-488`) is not needed.

**Becomes.** **Studio** `Private/Cast/`, with the checks `Studio.CastRules` and `Studio.Incremental` (the 24-mesh
batching becomes the memory-capped batches). **Tier 1.**

### C8. The idealised sun path and the dial
**Where.**
- *The sun.* `windows/Source/MuseeVision/Sky/MuseeSky.cpp:109-116` (`SunClockDirection`) and `:133-152` (up from
  06:00 to 18:00).
- *The dial.* `windows/Source/MuseeVision/SunClock/SunClockDial.cpp:21-107` (bands, hour ring, lines, ticks, numerals)
  and `:175-233` (ring and motto).
- *The numbers.* `MuseePlan.h:30-64, 119-158`.

**Good.** It is exact and simple, and it is the approved design (D-003).

**Must change.**
- *The nodus.* It comes from the Rotunda's drawing, not from a plan constant read by the sky actor (B12).
- *The hour.* It comes from Time's clock.
- *The model.* It is declared in `Data/Time/Museum.json`.
- *What goes.*
  - The sun's hard cut at 06:00 and 18:00, its fixed 75 000 lux (`MuseeSky.h:109-110`) and the `Daylight` curve are
    rendering choices, and they go to Light's sky.
  - The star field on a procedural mesh with a `Brightness` parameter (`MuseeSky.cpp:246-344`) goes. Its method stays:
    the Yale Bright Star Catalogue turned by the sidereal rotation, which Light renders with Time's rotation.
- *The glyphs.* The motto's glyph outlines (`SunClockGlyphs.h`, from `bake_glyphs.py:1-20`) were traced from
  Monotype's Centaur, a commercial face installed with Windows. They are **not carried forward** without a licence.
  The capitals should be drawn from licensed or public-domain evidence, such as a measured Trajan-inscription source,
  imported by Studio with its credit.

**Becomes.**
- **Time**: the sun model (`idealised` or `ephemeris`), `Musee::Time::Dial` in `Public/TimeDial.h`, and
  `Time.DialTrue`.
- **The Rotunda**: `Private/RotundaSunClock.cpp` lays the inlay from `Time::Dial`.
- **Light**: renders the declared sun (`Light.OneSunOneSky`).

**Tier 1.**

### C9. Two more methods worth keeping
- **The helical stair layout.** `windows/Source/MuseeVision/SunClock/SunClockKit.cpp:84-127`: treads by angle, a mid
  landing, and the string and soffit following the pitch line. Keep it as the rule for `FFormStair` in
  `Public/FormStair.h`, once the walking line is defined (D9).
- **Pieces with their own seed.** `SalonStructure.cpp:642-648, 669-799`: see A7. This goes to Form's `Lay()` and
  Matter's piece table.

### C10. Rejected

| Old code | Reason |
|---|---|
| `Geometry/MuseeMesh.h` | B3 |
| `Geometry/MuseeBake.h` and the bake-after-load pipeline | B5 |
| `M_Daylit`, `M_MistyGlass`, `M_IrisGlow` and the `Daylight` curve | B6 |
| `MuseeVRExposure` | C6 |
| `AMuseeSky::UpdateUnderground` and `FMuseeSkyOverride` | B8, D-012 |
| The exposure, relight, gallery-lights and native scripts' numbers | B1 |
| The hidden ramps and fences | B9 |
| `ClassicalHallChecks.cpp` | Development spawns, not checks |
| The Centaur glyph data | Licence |

---

## D. The Rotunda and its sun clock as designed: the numbers

These are the as-built numbers (D-001: the finished museum is the design). They are given in the Rotunda's own frame:
metres, origin at the centre of the drum on the finished floor, x east, y south, z up, and plan angle φ measured from
east towards south. In the old frame the centre was the museum origin (`MuseePlan.h:5-28`), so the numbers carry over
unchanged. The source of each group is given with it.

### D1. The drum
Source: `MuseePlan.h:30-64` and `RotundaStructure.cpp:29-60, 395-455`.

| Item | Value |
|---|---|
| Inner radius | 10.0 m (Ø 20 m, "just under half the Pantheon") |
| Wall | 1.2 m, so the outer face is r 11.2 m |
| Springing of the dome (drum height) | 10.0 m |
| Drum top, outside | 11.0 m, a flat ring from r 11.2 to r 10.553 (the foot of the outer shell) |
| Shadow gap at the floor | 150 mm high, 40 mm deep (back face at r 10.04) |
| Ports (D-012) | West to the Salon, north to Chenghuai, south to Albion, east to the Hall of Light, and the served stairwell down to the Classical Hall |

### D2. Doors and niches
Source: `MuseePlan.h:44-51` and `RotundaStructure.cpp:281-313, 457-653`.

**Doors.** All are round-arched and springing at 4.5 m.
- West and east: 3.0 m clear, so the crown is at 6.0 m.
- North and south: 4.0 m clear, so the crown is at 6.5 m. The plan board agrees at 15 px/m; the board's section draws
  the north door at 3 m.

| Door dressing | Section |
|---|---|
| Archivolt | 0.30 m wide from the intrados. An inner fascia 0.09 m wide at 0.08 m proud, a second fascia to 0.20 m at 0.10 m proud, then an outer band to 0.30 m at 0.13 m proud, finished with a 30 mm quarter-round. It dies into the pilasters at the springing |
| Imposts | 0.24 m tall under the springing, 0.05 m proud, with a 30 mm chamfer below, run through the wall |

**Niches.** Four, on the diagonals (45°, 135°, 225°, 315°). Each holds a cast (the as-built board; the casts are
Art's).

| Item | Value |
|---|---|
| Width | 2.0 m |
| Sill | 2.0 m |
| Springing | 4.67 m, so the crown is at 5.67 m |
| Plan | Half-round, radius 1.0 m, behind the drum's inner face, under a half-dome |
| Surround | 0.20 m wide, 0.08 m proud: an outer band with a 30 mm round, and an inner fascia 0.07 m wide at 0.06 m proud |
| Ledge under the sill | 0.16 m proud with a 30 mm round, reaching 1.26 m each side of the axis. Its top runs back as the niche's floor |

### D3. The order
Source: `MuseePlan.h:40-43` and `RotundaStructure.cpp:39-47, 672-815, 855-877`.

**The pilasters.**

| Item | Value |
|---|---|
| Number and spacing | 16, at 11.25° + k·22.5°. Beside a door they step out so that the capital clears the jamb: ±11.78° from the axis of a 3 m door, ±14.66° from a 4 m door |
| Section | 0.90 m wide, face at r 9.57 (0.43 m proud), back sunk to r 10.06 |
| Height to the entablature | 9.47 m |
| Base (0–0.32 m) | A plinth 0.18 m tall, 0.10 m proud; a torus 0.12 m tall (radius 0.06 m, 0.09 m proud); a fillet; and an apophyge of radius 0.02 m up to the shaft |
| Shaft | 0.32–8.90 m, fluted between 0.60 m and 8.62 m. The flutes stop square |
| Flutes | Seven, each 85 mm wide and 30 mm deep, between 27.5 mm fillets, with 70 mm plain margins at the arrises |
| Capital (8.90–9.47 m) | An astragal (a 30 mm radius bead) at 8.90–8.96 m; a necking to 9.14 m; an annulet 25 mm proud at 9.14–9.17 m; an echinus (quarter-round, radius 0.09 m) to 9.26 m; and an abacus 0.115 m proud, 0.21 m tall, to 9.47 m |

**The entablature, 9.47–10.50 m.**

| Part | Heights | Section |
|---|---|---|
| Architrave | 9.47–9.84 m | Two fasciae, at r 9.50 and r 9.48, under a crown of radius 0.05 m at r 9.43 |
| Frieze | 9.84–10.12 m | Flat, at r 9.51 |
| Cornice | 10.12–10.50 m | A bed moulding (radius 0.09 m); a corona soffit 0.14 m deep; a corona face 0.16 m tall at r 9.28; a radius 0.06 m moulding; and a sima 0.04 m to the top at r 9.22 |

The cornice's top meets the dome's sphere at 10.5 m, at an elevation of 2.87°. The total projection from the wall is
0.78 m.

### D4. The dome and the coffers
Source: `RotundaStructure.cpp:817-961, 1002-1015` and `MuseePlan.h:52-57`.

**The inner surface.** A sphere of radius 10.0 m centred on the springing plane. It runs from the cornice (2.87°)
through a plain band to 4°.

**The coffers.**

| Item | Value |
|---|---|
| Arrangement | 5 rings × 28, between 4° and 64°, in five equal rings of 12° |
| Depth | 0.45 m |
| Rib | 0.11 of the cell's width |
| Steps | Two steps each 0.045 of the width, at 0.12 m and 0.26 m deep, then a bevel of 0.05 of the width down to the panel at 0.45 m |

**The upper band.** A plain band runs from 64° up to the eye's rim at r 3.0 (72.54°, z 19.54).

**The outer shell.** A sphere of radius 10.6 m about the same centre. Its foot is at z 11.0 on the drum's top, and it
rises to a curb at r 2.9 whose top is at 20.45 m.

**The coffers as built are not square.** With equal 12° rings and a width that shrinks with cos(elevation), the top
coffers are about 1.19 m wide and 2.09 m tall. `GEOMETRY_AUDIT.md:68-70` and target 05 both ask for coffers that
diminish and stay square. For 28 to the ring starting at 4°, square cells need the ring bounds at 4.00°, 16.62°,
28.47°, 39.12°, 48.38° and 56.22°: equal steps in Mercator latitude, since each step is the cell's width in angle
divided by cos(elevation). This is a design question for the owner, raised here and not decided.

### D5. The eye
Source: `MuseePlan.h:34-35, 58-60`, `RotundaStructure.cpp:53-60, 879-894, 1036-1081`, and D-004.

**The opening and its curb.**
- The clear opening is Ø 5.0 m (r 2.5).
- The dome stops at r 3.0.
- At r 3.0 a ring hangs 0.12 m below the sphere, to a soffit at 19.42 m between r 3.0 and 2.75.
- A second soffit sits at 19.58 m, between r 2.75 and 2.5.
- The curb's inner face (r 2.5) then rises to its top at 20.45 m.

**The glazing, as built.** The eye is glazed and flat.
- The lattice ring is r 2.42–2.50, between z 20.22 and 20.38.
- The glass is one 10 mm pane lying on the ring at 20.38–20.39 m.
- The bars are steel, 50 mm wide and 100 mm deep, centred at 20.30 m. They run in three directions 60° apart, 0.8 m
  between parallel bars, and all cross at the centre (seven lines each way, k = −3…3). This makes equilateral
  triangles 0.92 m to a side.
- The node is a bronze sphere of radius 0.12 m centred at 20.25 m.

The rendering prompt for target 05 asks for "a modest 0.8 m rise". The as-built eye is flat, and D-004 keeps it as
built.

### D6. The sun clock floor
Source: `MuseePlan.h:36-37, 128`, `SunClockDial.cpp:3-233` and `plan/canvas/Rotunda.dc.html`.

**The dial** is r 0–7.0 m (Ø 14 m). It is the top of the rising drum, which is 6.99 m at its widest.

**The dial's bands**, from the centre outwards:

| Band | Radius |
|---|---|
| A | 0–1.40 m |
| Rosso | 1.40–1.52 m |
| B | 1.52–3.20 m |
| A | 3.20–4.90 m |
| B | 4.90–6.30 m |
| Nero | 6.30–6.38 m |
| A | 6.38–6.99 m |

Each band edge carries a bronze fillet 10 mm wide and 2 mm proud.

**The hour ring.** Bronze, r 6.76–6.80 m, 3 mm proud, centred on the circle of the point of shadow, **r 6.78 m**.

**The hour lines.** Thirteen lines at φ = 180° + 15°·k, for k = 0…12: VI in the west, through XII in the north, to VI
in the east. Seen from above with north up, the shadow therefore moves clockwise. The lines run from r 1.60 m to the
hour ring, 24 mm wide and 2.5 mm proud.

**The minute ticks** all end at r 6.77 m:

| Tick | Starts at | Width |
|---|---|---|
| Each minute | r 6.665 m | 3 mm |
| Every 5 minutes | r 6.62 m | 5 mm |
| Every 15 minutes | r 6.58 m | 7 mm |
| Every 30 minutes | r 6.52 m | 10 mm |

The shadow moves 2.96 cm a minute along the hour ring.

**The numerals.** Roman VI…XII…VI in bronze, 5 mm proud, their base at r 6.815 m and cap height 0.16 m, tops outward,
read from the dial.

**The meridian.** The as-built dial has no separate gilt meridian (`SunClockDial.cpp:13`); the XII line is the
meridian. `plan/README.md:77` and the rendering prompt ask for a gilt one. D-001 says the as-built dial is the design;
this ledger records the difference and does not decide it.

**The gilt sun** at the centre, where every visit arrives (D-012), facing west (`MuseePlan.h:26-28`):
- 32 rays from r 0.24 m: 16 straight rays to r 1.25 m and 16 flames to r 0.86 m, each ridged with hollow flanks;
- a gilt boss of r 0.285 m with a domed top at 85 mm and beads at r 0.19 and 0.30.

**The fixed ring** runs from r 7.0 to 10.3 m, under the drum wall, in three courses of pale marble in two tones:

| Course | Radius | Slabs to the round |
|---|---|---|
| Inner | 7.00–7.75 m | 48 |
| Middle | 7.75–8.95 m | 32 |
| Outer | 8.95–10.30 m | 64 |

**The motto.** HORAS · NON · NUMERO · NISI · SERENAS, in bronze capitals 0.26 m high and 8 mm proud, with gilt
triangular stops. Its baseline is at r 8.53 m in the middle course. It is centred on the south and read from the ring,
facing the dial.

### D7. The idealised sun (D-003)
Source: `MuseeSky.cpp:109-116, 133-152` and `MuseeSky.h:35-43`.

**The model.**
- The hour h is the museum city's civil clock time, with daylight saving in New York.
- The sun is up from 06:00 to 18:00.
- With Φ = 15°·(h − 6), the node's point of shadow on the floor is P(h) = −6.78·(cos Φ, sin Φ, 0).
- The direction towards the sun is (6.78 cos Φ, 6.78 sin Φ, H), normalised, where H is the node's height above the
  dial.

**What follows.**
- The sun's altitude is constant: 71.49° with H = 20.25 m (the node), or 71.53° with the 20.30 m the old sky used.
- Its compass azimuth is 90° + Φ: east at 06:00, south at 12:00, west at 18:00.
- The point of shadow sits on the numeral of the hour. At 10:00 it is on X, 6.78 m north-west of the centre (the
  board); at 16:00 it is on IV, north-east (target 05).

**The patch of sun.** The eye's clear aperture is the lattice ring's opening (r 2.42), trimmed slightly by the 0.87 m
deep curb (r 2.5, whose foot is 0.21 m off-axis at this altitude). It projects to a patch about 4.8 m across by about
5.0 m along the sun's azimuth, centred close to the point of shadow.

**Shadows.** The sun is 0.533° across and the path from the lattice to the floor is 21.4 m, so a penumbra is about
0.20 m wide.
- The node (Ø 0.24 m) still casts a true point: a dark core about 40 mm across inside a soft disc about 0.44 m across.
  The core fits the 40 mm hour ring.
- The 50 mm bars cast no umbra, only soft bands about 0.25 m wide dimmed by about a quarter. The lattice pattern in
  the pool should therefore not be expected crisp.

These are consequences for Light's commissioning, not for the drawing.

**Open for the Time and Light specs.** The old build rendered this sun with:
- a constant 75 000 lux;
- visibility cut at 06:00 and 18:00;
- stars shown only when the *real* sun was more than 6° below the horizon (`MuseeSky.cpp:195-197`).

The new specs must say which sun altitude drives the atmosphere and the daylight lux under the idealised model.

### D8. The rising cylinder (the tempietto)
Source: `MuseePlan.h:126-158`, `SunClockBuild.h:40-57, 137-160` and `SunClockDrum.cpp:5-17, 203-215`.

**The lift.** The dial rises 3.5 m in 8 s on a smoothstep ease, and the door's sill then lies on the floor. It is
raised from the ring (r 7.35–10.0 m) and never lowers while anyone is inside r 7.2 m or below the floor
(`SunClock.cpp:17-24`).

**The drum.**

| Item | Value |
|---|---|
| Wall | Marble, r 6.545–6.90 m, 6.99 m at its widest (cornice and plinth), from −3.56 m to the dial's underside at −0.14 m |
| Base | −3.30 to −3.08 m |
| Architrave | Top at −0.45 m |
| Pilasters | 12, each 0.44 m wide and 55 mm proud, with five flutes |
| Door | West, 2.6 × 2.95 m, jambs radial, a conical arch of radius 1.3 m at the face |
| Soffit | Coffers in five rings of 8, 12, 16, 24 and 32; a bronze rim; and a gilt pendant boss over the well down to −0.265 m |

**Clearances.** The pocket is r 6.54–7.0 m, down to −3.58 m. The stair's sleeve is r 6.48–6.54 m. The drum slides
with 5 mm and 10 mm to spare, and a groove in the bronze rim (r 6.46 m, up to −0.06 m) takes the sleeve's top at
−0.07 m.

**The retracting balustrade.** It drops 0.94 m into housings 1.20 m deep. It rises only after the lift passes 1.20 m,
and it is home again before the drum comes near.

### D9. The stair, the well and the port (the served stairwell, D-012)
Source: `SunClockStair.cpp:3-13`, `SunClockKit.cpp:84-127`, `SunClockShaft.cpp:3-16` and `SunClockBuild.h:118-122`.

**The flight.**

| Item | Value |
|---|---|
| Risers | 40 of 150 mm, down 6.0 m |
| Top landing | −0.15 m, spanning the drum's door at 164°–194° |
| Flights | 18 treads, then a mid landing of 20° at −3.0 m, then 19 treads |
| Last riser | At −50°, so the foot faces the port across the landing |
| Tread turn | 194°/37 = 5.243° |
| Treads | Span r 2.35–6.48 m, with a rounded nosing |
| Inner string | r 2.2–2.35 m, 120 mm above the pitch line, never above −0.14 m |
| Soffit | A helicoid 400 mm under the pitch line |
| Open well | Ø 4.4 m |

**The goings.** 0.215 m at r 2.35, 0.403 m at r 4.4 and 0.593 m at the wall. The file's header says 0.24 m at the
string; the layout gives 0.215 m.

**Blondel's rule** (2R + G = the human measure's 0.63 m pace) holds at r 3.61 m. The old "walking line" at r 4.4 m
gives 0.70 m. `Form.StepsFitTheBody` needs the stair's walking line declared: this is a spec decision for the Rotunda.

**The shaft and the port.**
- The stair's wall is at r 6.48 m, and the shaft's masonry runs out to r 7.6 m, where the room below begins.
- The landing is at −6.0 m, with a marble rosette under the well.
- The port faces north at r 7.6 m: 3.6 × 4.4 m, springing at −3.4 m, crown at −1.6 m. Inside r 7.05 m the pocket
  passes over it, so it is closed there by a tympanum over a lintel at −3.68 m. The portal onto the stair is
  3.6 × 2.32 m.

Under D-012 this prism is the Rotunda's served well and the Classical Hall reserves it. Which side draws the north
port at r 7.6 m is for the Site plan to settle.

### D10. Light in the drum, as built, and what changes
Source: `SunClock.h:137-151, 193-206` and `SunClockBuild.h:155-160`.

All sources were 3800 K.

| Source | Place | Output, as built |
|---|---|---|
| Twelve concealed spots | In the soffit's fourth ring (r 4.15 m, every other coffer at 7.5° + 30°·i) | 4500 cd each: about 200 lux on the treads half-way down |
| One spot down the well | Under the boss | 13 000 cd: about 150 lux on the rosette 9.4 m below |
| A fill with no fitting | On the well's axis at −2.4 m | 1500 cd |

The fill goes (B7). The spots become catalogue luminaires in declared mounts, with targets by use. Daylight still
enters through the drum's door.

### D11. Differences to settle in the Rotunda's spec
1. **The node height.** The node is at 20.25 m, but the sun was aimed from 20.30 m (B12). Take the nodus from the
   drawing.
2. **The coffers.** Equal 12° rings as built, or square diminishing cells as the target asked (D4).
3. **The meridian.** None as built; a gilt meridian in the README and the rendering (D6).
4. **The lattice's rise.** Flat as built; 0.8 m in the rendering prompt. D-004 keeps it as built.
5. **The stair's walking line**, for Blondel (D9).
6. **The drum's stone.** The code uses honed travertine for the drum (`RotundaStructure.cpp:1167-1168`). The look
   target, the target 05 prompt and the geometry audit ask for pale marble (`windows/LOOK_TARGETS.md:79`,
   `plan/renderings/05-the-rotunda.md`, `windows/GEOMETRY_AUDIT.md:77`). This is for the Matter spec.
7. **The ring's outer edge.** A 128-gon matched to a retired import; it becomes a true circle at the chord tolerance
   (B12).
8. **The idealised sun's sky and lux** (D7).
