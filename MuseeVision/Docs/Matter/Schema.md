# Matter records: the schema

This page describes the fields of the records in `Data/Matter`, so the Matter builder can make USTRUCTs that match
them. The glass group (R-MATTER-01) wrote it first, after reading every record then in the slot: glass, the white
plasters and limewash, bronze, brass, steel and gilt. Groups that finish later extend it.

## Files and keys

The records live in four places:

| Record | File |
|---|---|
| A substance | `Data/Matter/Substances/<key>.json` |
| A working | `Data/Matter/Workings/<key>.json` |
| A history | `Data/Matter/Histories/<key>.json` |
| A dossier, one per substance | `Docs/Matter/<key>.md` |

A working or a history has no dossier of its own; its substances' dossiers describe it. The one exception is `gilt` (see Metals below).

Architecture's tree shows `Workings.json` and `Histories.json` as single files. The Drafter orders asked for one file per record, so that is what is here.

Keys follow Craft: they are lowercase, dot-scoped, and kebab-case within a segment. A dot marks a scope:
- **A variant of a working:** `laminated.uv-filtering` or `cast.plaster`.
- **A family's history:** `glass.soiling` or `white.micro-cracking`.

A file is named after its key.

## Field names

JSON field names are **camelCase** (`accentOf`, `appliesTo`, `axisValues`). There are two reasons:
- **Reflection.** `FJsonObjectConverter` matches JSON keys to UPROPERTY names without regard to case, so `accentOf` fills `AccentOf` with no mapping table. A snake_case key such as `accent_of` would not.
- **Units stay as data.** Craft's rule that a UPROPERTY float carries its unit in its name (`DensityKilogramsPerCubicMeter`) applies to the structs the builder writes. The JSON keeps the unit as a field instead, so Foundation's record reader can refuse a record whose units do not match (D-014).

Every record starts with `schema` (`musee.matter.substance/1`, `musee.matter.working/1` or `musee.matter.history/1`), then `key` and `name`. `name` is English display text; the game shows `FText`, never a key.

## The quantity

Every number sits in an object with its unit and its source:

```json
{"value": 1.523, "unit": "1", "grade": "manufacturer", "conditions": "589.3 nm", "source": "Pilkington ATS-129 ..., https://..."}
```

| Field | Required | Meaning |
|---|---|---|
| `value` | one of `value`, `min`, `max` | A number, or a fixed-length array for a colour triple (then `space` is required). |
| `min`, `max` | | A range. Only one of them makes a bound: `"max": 0.01` means "under 1 %". |
| `unit` | yes | See "Units" below. `"1"` is a dimensionless fraction from 0 to 1, never a percentage. |
| `source` | yes | A URL or a bibliographic reference, written out in full. Two sources are joined with ` ; `. |
| `grade` | yes in new records | How strong the evidence is (see below). |
| `derivation` | when `grade` is `derived` | The formula or procedure. `source` then names the measured inputs, not the derivation itself. |
| `conditions` | no | The test conditions: wavelength, temperature, thickness, standard. |
| `note` | no | A short plain-English remark, such as a quotation from the source. |
| `asReported` | no | The value as printed in the source, before conversion (for example `"8.8 kg/dm3"`). |
| `space` | with colour vectors | For example `"linear Rec.709 (sRGB primaries), D65 white"`. |
| `phase`, `direction` | no | For mineral optics: the phase, and the ray (omega or epsilon). |

These are the **grades**:

| Grade | Meaning |
|---|---|
| `measured` | A measurement published in the scientific or conservation literature. |
| `manufacturer` | A maker's data sheet or project record. |
| `guidance` | A standard, a building code, or a conservation or trade guideline. |
| `secondary` | A reliable secondary account, such as a review, a lecture or a trade article, that quotes the measurement. |
| `historical` | A historical treatise or record, such as Vitruvius. |
| `derived` | Computed here from measured inputs. It must carry a `derivation`. |
| `design` | A value the museum's design decides, not a property of the material: a Decision, `Architecture.md`, or the as-built frozen reference (for example the lattice's paint colour). Added by the metal group. |
| `record` | A pointer to another Matter record that holds the evidence (for example the glazing's UV transmittance, cited by the steel's history). Its strength is that record's grade. Added by the metal group. |

A sourced statement with no number is a **text fact**: `{"text": "...", "grade": "...", "source": "..."}`.

A list of values that share a unit and a source is `{"unit": "mm", "values": [...], "grade": "...", "source": "..."}`. An example is the standard glass thicknesses.

## The table

A spectrum or any other series is one flat object:

```json
{"axis": "wavelength", "axisUnit": "um", "axisValues": [0.40, 0.45, 0.50],
 "unit": "1", "values": [4.57e-8, 4.252e-8, 3.257e-8],
 "grade": "measured", "source": "M. Rubin, ... (1985) ..., https://..."}
```

`axisValues` and `values` have the same length. `derivation` and `conditions` are optional, as for a quantity. The glass records use four kinds of axis:

| Axis | Unit | Example |
|---|---|---|
| `wavelength` | `um` | the extinction coefficient `k` |
| `frequency` | `Hz` | acoustic absorption by octave band |
| `thickness` | `mm` | light transmittance |
| `path` | `mm` | internal transmittance along an edge |
| `tin` | `wt%` | the measured CIELAB colour of tin bronzes by tin content (metal group) |

**Why flat:** Unreal cannot reflect a nested array (`TArray<TArray<float>>` is not a legal UPROPERTY). A table of `[x, y]` rows therefore has no struct to load into. One `FMatterTable` with two `TArray<double>` covers every series.

The metal group converted its nested rows to this form: `bronze` (`optics.measuredColour`, `optics.copperReference`), `brass` and `steel` (`optics.spectral`), `gilt` (`optics.spectral[]`) and `patinated` (`film.phases[].reflectance`, `reflectanceR1`, `reflectanceR2`). A set of complex indices is two tables, `n` and `k`, on the same wavelength axis.

## The substance record (`UMatterSubstance`)

| Field | Required | Meaning |
|---|---|---|
| `schema`, `key`, `name` | yes | As above. |
| `family` | yes | One of `stone`, `white`, `bronze`, `oak`, `accent`, `glass`, `water`, `living` (D-014). |
| `accentOf` | yes | `null` for a core family. For `family: "accent"` it names the core family the accent ties back to (brass: `bronze`). `Matter.OneLanguage` fails an accent without one. |
| `summary` | yes | One or two sentences: what it is and where the museum uses it. |
| `dossier` | yes | The path of its dossier in `Docs/Matter`. |
| `workings`, `histories` | yes | The keys of the workings and histories that apply to it. |
| `aliases` | no | Other names in the trades. |
| `constituents` | no | A list of `{material or mineral, formula, role}`. |
| `formation` | yes | `how` (text), plus `facts` (a map of quantities). It can also hold `reaction`, a composition map, or other family-specific blocks. |
| `optics` | yes | Family-specific; see below. |
| `physical` | yes | A map of quantities: `density`, `youngsModulus`, `thermalConductivity`, `thermalExpansion`, `specificHeat`, `hardness` and so on. |
| `surface` | no | Measured texture: roughness, the faces' differences. |
| `acoustic` | yes | `absorption`: a table on the `frequency` axis in `Hz`, one value per octave band from 125 Hz to 4 kHz. Say which mounting it is for in `conditions`. |
| `stock` | yes | Stock rules: largest block or sheet, standard thicknesses, bed height, grain, board width, bay size. |
| `physicalMaterial` | yes | For `UPhysicalMaterial`: `friction` and a footfall class, or a `note` saying why none is sourced. |
| `photograph` | yes | A list of sentences: what to look for in a photograph of the real material. |
| `evidence` | yes | A list of local CC0 evidence: `{path, set, provider, url, licence, credits, tier, shows, statistics}`. It may be empty. |
| `evidenceSearch` | when `evidence` is empty | `{searched: [paths], found: text, tier: text}`: what was looked through and why nothing was used. |
| `unverified` | yes | A list of `{field, value or min/max, unit, reason}`. These values are **not** evidence and are never read as data. |
| `face` | no | For a composite whose visible face is another substance: that substance's key. `fibrous-plaster` has `face: "plaster-of-paris"`, because a visitor sees only the plaster face coat, never the hessian and laths behind it. The resolver takes optics and the surface histories from the face substance. The composite record then carries only what it adds (reinforcement, stock, fixing, failure) and has no `optics` block of its own. `faceNote` says why. |
| `sources` | no | The bibliography: every reference cited in the record, written out once. |

The white records (`lime-plaster`, `plaster-of-paris`, `fibrous-plaster`, `limewash`) add two things:
- **`evidence[].statistics`.** Figures computed from the local evidence set, each a quantity with `grade: "derived"` and a `derivation`. Two are used:
  - `colorMeanLinear`: a colour triple with `space`;
  - `roughnessMapMedian`: the median of the set's authored roughness map.

  Neither is a measurement of the material: the photograph's exposure is unknown. They let Studio compare a render with the set, and nothing more.
- **`optics.luminousReflectance`.** A map of *bounds*: measured values for related powders (ground marble, ground natural gypsum) that the set body must lie under or near. The value the master graph uses stays in `unverified` until the tier (b) specimen sheet measures it.

The **optics** block depends on the family. What the glass shader reads from `low-iron-glass`:

| Field | Form | Meaning |
|---|---|---|
| `refractiveIndex` | a list of quantities | Measured indices, each with its wavelength in `conditions`. |
| `dispersion` | a formula, `validRange`, `evaluated` (a table) and `abbeNumber` | How the index varies with wavelength. |
| `specularF0` | a derived quantity | Normal-incidence reflectance per face, from the index. |
| `extinctionCoefficient` | a table | The measured spectrum of `k`; this is the truth. |
| `absorptionCoefficient` | a table, in `1/mm` | Derived from `k`. Internal transmittance through a path d is exp(−α d). |
| `luminousTransmittance`, `solarDirectTransmittance` | tables on the `thickness` axis | The maker's measurements, for `Matter.Plausible` to compare with the spectrum. |
| `crossCheck` | a table | The prediction from the spectrum, at the same thicknesses. |
| `edgeColour` | a table per wavelength, on the `path` axis | Internal transmittance at 450, 550 and 650 nm. |
| `uvTransmittance`, `colourRenderingIndex`, `luminousReflectance`, `hemisphericalEmissivity` | quantities | |
| `wet` | text | What a water film does to it. |

The other families use `specularF0` or `f0` (metals: a colour triple with `space`), `refractiveIndex` (a list with `phase` and `direction` for minerals), `measuredColour` and `spectral`. The builder reads the fields its family's master graph needs. It should not flatten every family into one shape, because a metal's complex F0 and a glass's absorption spectrum are different truths.

## The working record (`FMatterWorking`)

| Field | Required | Meaning |
|---|---|---|
| `schema`, `key`, `name` | yes | As above. |
| `appliesTo` | yes | `{families: [...], substances: [...]}`. |
| `over` | yes | The workings this one is laid over or built from. It is empty for a first working. Gilt lies over a substrate (D-014), `patinated` over `cast`, and `anti-reflective` over any of the glass workings. |
| `process` | yes | Plain English: how the trade does it. |
| `lays` | no | For a working that lays another substance over the piece: that substance's key. `limewashed` lays `limewash` over `lime-plaster`, in the way gilt lays leaf over a substrate. The resolver takes the top layer's optics from the laid substance. |
| working-specific blocks | no | For example `surface`, `strength`, `flatness`, `strainPattern`, `interlayer`, `laminate`, `optics`, `stock`, `film`, `leaf`, `steps`, `facts`, `finishes`, `conservation`. `steps` is a list of `{step, grade, source}`. `finishes` (in `cast.plaster`) lists the historical surface treatments a making-time decision chooses between. Every number inside follows the quantity form. |
| `photograph`, `unverified`, `sources` | as for substances | |

**Open question: stacked workings.** A real glazing make-up stacks workings: anti-reflective coated, over a laminate with a UV-filtering interlayer, over heat-strengthened plies. `FMatterSpec` holds one working key, and `over` records which stacks are real. The Architect should decide how a spec names a stack. There are two ways:
- **The top working names the stack.** The spec names the top working (`anti-reflective`), and the piece's make-up names the rest: the plies, their thicknesses and their treatments, which Form owns.
- **The spec holds a list.** `FMatterSpec` holds an ordered list of working keys.

The glass records work with either.

## The history record (`FMatterHistory`)

A history is **one mechanism of the surface law for one family**. Its key is `<family>.<mechanism>`: `glass.soiling`, `white.micro-cracking`.

| Field | Required | Meaning |
|---|---|---|
| `schema`, `key`, `name`, `family` | yes | As above. |
| `describes` | yes | Plain English: the mechanism, and where it acts on a piece. |
| `terms` | yes | A map of that mechanism's facts: quantities, text facts, tables, and a stated model with its fitted constants. |
| `surfaceLawTerms` | no | The terms of the surface law in `History.ush` that the mechanism feeds: `age`, `wear`, `touch`, `soiling`, `chemistry`, `wetness`. |
| `bySubstance` | no | Values that differ between substances of the same family. |
| `drivers` | no | What speeds it up or slows it down. The white histories give a list of `{driver, owner}`, where `owner` names the system and the `FMatterPieceLayout` slot that carries the fact (for example `Form (vertex colour: arris proximity)`). A driver with no slot is reported in `unverified` as a gap. |
| `photograph`, `unverified`, `sources` | as for substances | |

**Open question:** these records do not settle what `FMatterSpec`'s history is. It could be one mechanism, a set of mechanisms, or a regime: an age and a cleaning programme that drives all of them.

## Metals: the metal group's additions

The metal group wrote `bronze`, `brass` and `steel`, the workings `cast.bronze`, `brushed`, `patinated`, `gilt` and `painted.steel`, and the histories `bronze.tarnish`, `bronze.wear`, `bronze.weathering`, `gilt.wear` and `steel.coating-ageing`. They follow everything above. These are the additions.

**The conductor optics block.** A metal's `optics` has `kind: "conductor"` and these fields:

| Field | Form | Meaning |
|---|---|---|
| `f0` | a derived colour triple with `space`, and `min`/`max` triples where datasets differ | Normal-incidence reflectance: the unpolarised Fresnel reflectance of the complex index, integrated against the CIE 1931 2-degree observer under D65, in linear Rec.709. A channel above 1 (gold's red) is out of the Rec.709 gamut, not an error. |
| `f0.luminance` | a quantity | CIE Y of the F0 spectrum. |
| `f0.spectralCheck` | a table (or a map of tables, one per dataset) on `wavelength` | F0 at 450, 550 and 650 nm, for checks. |
| `f82` | a derived colour triple | The Fresnel reflectance at 82 degrees: the edge tint of the F82 model, where metals whiten toward grazing. |
| `spectral` | `{n, k}` tables on one wavelength axis, or a list `[{dataset, n, k}]` where several measured sets exist (gold) | The measured complex index; this is the truth. |
| `measuredColour` | `{instrument, alloys, L, a, b}` with `L`, `a`, `b` tables on the `tin` axis in unit `CIELAB` | A measured colour table, used where no complex index has been measured (tin bronze). |
| `crossCheck`, `copperReference` | a text fact, or `{n, k}` tables | How the colour route and the optical-constant route agree, on the parent metal. |

**Substance blocks used by the metals.** These are not required of other families:
- `identity`: `{class, designations: [{system, designation, number, source, grade}], chosenBecause: [text facts], alternativesConsidered: [{designation, reason, source, grade}]}`. Why this alloy or grade, and what else was weighed.
- `composition`: `{standard, <element>: quantity}`, with limits as `min`/`max` in `wt%`.
- `uses`: `[{where, working, histories, source, grade, note}]`. Where the design puts the substance; `source` is the Decision, `Architecture.md` or the frozen reference (grade `design`).
- `evidenceRejected`: `[{path, set, reason, source, grade}]`. Local sets that look like evidence but are not (the old build's metal PBR sets are procedural, not photographs).
- `familyDeclaration`: a text fact. It is used when D-014's families leave the choice open (steel is declared an accent of `glass`).

**Working blocks used by the metals.** `designation` (the trade code: `{system, code, text}` plus its quantities, such as CDA/NAAMM M32 or C55), `steps`, `preTreatment`, `hotPractice`, `hardwareEquivalents`, `film` (`{chemistry, phases: [{phase, formula, colour, refractiveIndex, density, reflectance or reflectanceR1 and reflectanceR2, f0}], thickness}`), `leaf`, `methods` (`[{method, use, layers: [{layer, material, coats, ...}], burnish, look}]`), `bole`, `substrates`, `coating`, `environment`, `system`, `colour` (a design `albedo` triple), `protection` and `inTraffic`. Every number inside follows the quantity form. A working may carry `evidence`, `evidenceRejected` and `evidenceSearch` like a substance (`gilt` does).

**Keys.** `cast.bronze` and `painted.steel` are scoped variants, like `cast.plaster`: casting and painting differ by material.

**History additions.**
- `appliesTo: {families, substances, workings}` says which pieces the mechanism acts on. The bronze histories act on brass too, because brass is the bronze family's accent and ages by the same chemistry.
- A working's history has `family: null` and names its working in `appliesTo.workings`; `gilt.wear` is the case.

**Unverified text.** An unverified statement that is not a number is `{field, text, reason}` with no unit. It replaces the empty unit `""`.

**One working dossier.** `Docs/Matter/gilt.md` is a dossier for a working, named by the working's `dossier` field. Gilt is laid over three families' substances (plaster, bronze, oak), none of whose dossiers is its natural home, and D-014 singles it out.

## Stone: the stone group's additions

The stone group wrote these records, which follow everything above:
- the substances `travertine` and `cream-marble`;
- the workings `sawn.vein-cut`, `sawn.cross-cut`, `honed`, `polished.stone`, `flamed` and `bush-hammered`;
- the histories `stone.foot-traffic-wear`, `stone.pore-soiling` and `stone.dissolution-outdoors`.

These are the additions.

**Keys.**
- `sawn.vein-cut` and `sawn.cross-cut` are variants of sawing.
- `polished.stone` is a scoped variant, like `cast.bronze`, because a mason's polish and a metal polish are different
  processes.
- `honed`, `flamed` and `bush-hammered` belong to stone alone and are unscoped.

**Cut and finish.** A stone face is made by two workings, a cut and then a finish:
- Each stone working has `kind`, either `cut` or `finish`.
- A finish's `over` lists the cuts it can be built from: either `sawn.vein-cut` or `sawn.cross-cut`. This is the
  stacked-workings question above, in its stone form.
- A cut carries `bedToFace`, the angle between the face and the bedding: 90° for vein-cut, 0° for cross-cut. Form's
  per-piece bed frame carries the actual orientation, and the two must agree.
- A working may carry `processSources`, the sources of its `process` text.

**Excluded workings.** A substance may list `excludedWorkings`, as `[{key, reason}]` where `reason` is a text fact.
Travertine and cream marble exclude `flamed`. The working states the same fact from its side, with `admits` and
`excludes`. `flamed` has `appliesTo.substances: []`, because no substance in the records takes it yet.

**Optics of a stone of mineral grains.**

| Field | Form | Meaning |
|---|---|---|
| `refractiveIndex` | a list with `phase` and `direction` | The mineral's indices, one per ray. |
| `dispersion` | `formula` (with its own `grade` and `source`), `validRange`, and one evaluated table per ray (`evaluatedOmega`, `evaluatedEpsilon`) | How the indices vary with wavelength. |
| `specularF0` | a derived quantity | The reflectance of grains in random orientations. `min` and `max` are the extremes a single grain can show. |
| `measuredColour` | a list of `{specimen, working, cieLab, luminousReflectance}` | `cieLab` is a triple in unit `CIELAB`, with `space`. `luminousReflectance` is derived from L\*. Each row names the specimen measured, which may be a proxy for the substance: Spanish travertines stand in for Tivoli's until a specimen is measured. |
| `luminousReflectance` | a quantity | A maker's light reflectance value (cream marble). |
| `subsurfaceReference` | `{specimen, reducedScattering, absorption, diffuseReflectance, refractiveIndex, diffusionLength, note}` | The scattering of the nearest measured stone (Jensen's white marble). It is a reference, not the substance's value. |
| `polishDarkens` | a text fact | The measured colour change that a polish makes (travertine). |

**A stone's surface.** The stone records use these fields:
- `surface.fabric` and `surface.banding`;
- `surface.voids`, a map of quantities and text facts;
- `surface.voidTreatment`, how the voids are filled;
- `surface.veins`.

**Identity.** Stone uses the metal group's `identity`, with two more fields:
- `identifiedFrom`: text facts giving the design description from which the real stone was identified;
- `correction`: a text fact that corrects a source.

An `alternativesConsidered` entry may carry `facts`, a map of quantities, so that a rejected candidate's data stay
available (Botticino, in `cream-marble`). `etymology` is a text fact.

**Stock.** `stock.bed` is a text fact about the bedding and the saw plane.

**Friction.** `physicalMaterial.friction` is a map of pendulum quantities in `PTV`, named `<finish><Wet|Dry>`
(`bushHammeredWet`), with `method` as a text fact. A printed ">45" is recorded as `min: 45`.

**Histories.** The stone histories carry these fields:
- `appliesTo`, as in the metal histories.
- `drivers`, as `{driver, owner, meaning, source, grade}`. `owner` names the system and the piece-layout slot, for
  example `Form (vertex colour: cavity)`. A driver with no owner, such as `relativeHumidity` or `rainAcidity`, is
  written `none declared (no slot)` and listed in `unverified` as a gap.
- `bySubstance`: each substance's susceptibility. For wear it is the abrasion groove; for soiling, the open porosity.
- `evidence` and `evidenceSearch`, as in a substance.

**Field names and Craft's banned tokens.** Craft bans `Strength`, and inside Matter `Scale`, as whole tokens of
identifiers. JSON field names become UPROPERTY names by reflection, so the stone records avoid them:
- `flexuralBreakingStress` and `compressiveBreakingStress` stand for the test standards' flexural and compressive
  strength;
- `extent` stands for a size.

Several earlier records still use the banned token. The token lint will fail these names, and the Architect should
rule on them (a gap):
- `annealed`, `heat-strengthened` and `toughened`: `strength` and `characteristicBendingStrength`;
- `bronze`: `tensileStrength` and `proofStrength`;
- `brass` and `steel`: `tensileStrength` and `yieldStrength`;
- `lime-plaster`: `compressiveStrength`;
- `plaster-of-paris`: `compressiveStrengthDry`.

**Unverified text.** The stone records use the metal group's `{field, text, reason}` for unverified statements that are
not numbers.

## Units

The unit strings used so far are listed here, so Foundation's reader can map them to `Musee::` quantities.

| Kind | Units |
|---|---|
| Dimensionless and fractions | `1` (a fraction from 0 to 1; also a refractive index or a linear colour channel), `wt%`, `ppm`, `%`, `%IACS`, `count` (a whole number of things: members, nodes, panes), `CIELAB` (an L*, a* or b* coordinate) |
| Length | `nm`, `um`, `mm`, `m`, `in` |
| Area and mass | `m2`, `kg/m2`, `kg/m3`, `t`, `carat`, `kg/kg` |
| Mechanical | `GPa`, `MPa`, `kPa`, `MPa/K`, `J/m2`, `J`, `HB`, `Mohs`, `N`, `Ha` (ASTM C241 abrasion hardness), `PTV` (pendulum test value, EN 14231 and its national variants) |
| Thermal | `degC`, `K`, `1/K`, `J/(kg*K)`, `W/(m*K)` |
| Optical and light | `1/mm`, `1/m`, `uW/lm`, `deg`, `GU` (gloss units, ISO 2813) |
| Deposition and chemistry | `ug/cm2`, `ugC/cm2`, `ug/(m2*day)`, `ugC/(cm2*month)`, `mg/L` (rain composition), `mg/(m2*year)` (corrosion mass change) |
| Time and rate | `s`, `min`, `h`, `1/day`, `week`, `month`, `year`, `year CE`, `m/s`, `mm/day^0.5`, `um/year` (corrosion or wear), `GU/year`, `m3/year` (quarry output), `um/m` (surface recession per metre of rain), `%/week` (dust coverage) |
| Other | `Hz`, `nOhm*m`, `mol/L`, `grit`, `mesh`, `coat`, `kg/(m2*s^0.5)` (capillary water absorption) |

Geological ages are written in `year`, with the source's own figure in `asReported` (`"about 115 and 30 ka"`,
`"approximately 50 Ma"`). A stone's water absorption, porosity and strength loss stay in `%`, the scale their test
standards define.

Some units in the records are free text and need normalising before the reader can check them:
- the empty unit `""` in some `unverified` entries.

The white group's free-text units (`1 (GGX alpha)`, `mm per sqrt(day)`, `kg water per kg hemihydrate`) are now normalised to `1`, `mm/day^0.5` and `kg/kg`, with the model or the substances named in `conditions`.

A percentage should be written as a fraction in `1`. `%` and `%IACS` are kept only where the source's own scale is meant.
