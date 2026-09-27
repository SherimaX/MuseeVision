# Nature

Everything grown or formed by natural process. Modules `MuseeNature` (Runtime: the records) and `MuseeNatureMaking`
(Editor: growing). Its piece source runs at order 20, after the stage's own pieces.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `NaturePlanting.h` | `FNaturePlanting`: a stage's `Planting.json`, its water and its beds |
| `NatureWater.h`, `NatureOutline.h` | `FNatureWater` (anchor, body, outline, depth, freeboard, material, working, history); `FNatureOutline` (an ellipse or a polygon: `IsValid`, `Contains`, `Area`) |
| `NatureBed.h`, `NatureCast.h` | `FNatureBed`: a species in a Bed anchor, a count at a spacing, its material spec, or `FNatureCast`: a cast specimen's material |
| `NatureSpecies.h`, `NatureOrgan.h` | `FNatureSpecies`: a record of `Data/Nature/Species/` (form, organs and their tissue, measures, colours, sources) |

The making twin registers `Nature`'s piece source: each body of water's surface (non-Nanite, its own palette key)
and each bed's plants, grown by the forms `water-lily`, `iris`, `sedge` and `forget-me-not` (ported from the
as-built pond), one piece per organ, seeded by `Musee::Seed` of the bed, species and place. A living part takes its
species' tissue (or the bed's material) with the bed's working and history; a cast specimen's parts (the golden iris)
all take the cast's material, and are solid. Every grown surface is drawn on Form's `FFormMesh`; a polygon of water is
Form's flat face.

## Later (tier 2, owner Nature)

- Runtime behaviour at dresser order 20: sway in the wind, ripples, and `UNatureWaterComponent` (the level held by
  gravity in a moving basin, `Nature.WaterContained`).
- Phenology by solar term (`MPC_Nature`) and `Nature.InSeason`; the species records name their flowering terms in
  notes today.
- Growth spaces from Form's free space (`FNatureGrowthSpace`), so a marginal keeps to its shelf by geometry rather
  than by its bed's share of the water; `Nature.WithinGrowthSpace`, `Nature.Rooted`, `Nature.WithinBotany`.
- Trees and ground covers (space colonisation, the pipe model), rock and ground; precipitation.
