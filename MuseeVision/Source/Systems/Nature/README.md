# Nature

Everything grown or formed by natural process. Modules `MuseeNature` (Runtime: the records) and `MuseeNatureMaking`
(Editor: growing). Its piece source runs at order 20, after the stage's own pieces.

## Connectors (tier 1)

| Header | Connector |
| --- | --- |
| `NaturePlanting.h` | `FNaturePlanting`: a stage's `Planting.json`, its water and its beds |
| `NatureWater.h`, `NatureOutline.h` | `FNatureWater` (anchor, body, outline, depth, freeboard, material, working, history); `FNatureOutline` (an ellipse or a polygon: `IsValid`, `Contains`, `Area`) |
| `NatureBed.h`, `NatureCast.h` | `FNatureBed`: a species in a Bed anchor, a count at a spacing or at its `points`, its `point`, along its `line` or over its `outline`; a tree's `height`, `crownCentre`, `crownRadii` and `leafDensity` (its share of its species' full leaf); its material spec, or `FNatureCast`: a cast specimen's material |
| `NatureGround.h`, `NatureClip.h`, `NatureTraining.h` | `FNatureGround`: the soil, lawn or gravel a bed lays over its outline; `FNatureClip`: a clipped hedge's section; `FNatureTraining`: a path a climber is trained along |
| `NatureSpecies.h`, `NatureOrgan.h` | `FNatureSpecies`: a record of `Data/Nature/Species/` (form, organs and their tissue, measures, colours, sources) |
| `NatureTreeHabit.h`, `NatureBranching.h` | `FNatureTreeHabit`, `FNatureBranching`: a tree species' crown, trunk, orders of branching, and how its leaves and flowers are set |

The making twin registers `Nature`'s piece source: each body of water's surface (non-Nanite, its own palette key),
each bed's ground (its `ground` spec over its outline, less the water lying in it: Form's flat face, walkable), and
each bed's plants, one piece per organ, seeded by `Musee::Seed` of the bed, species and place. The forms:
`water-lily`, `iris`, `sedge` and `forget-me-not` (ported from the as-built pond); `tree` (the as-built tree grower,
its habit now the species record's), `bamboo` (the as-built grove), `lotus`, `herb` (meadow flowers and bulbs),
`vine` (trained along its bed's paths), each plant at its crown; `hedge`, grown once along its bed's line to its
clipped section; `grass`, tufts over its bed's region; `moss`, a carpet laid over the region as Form's face. A living
part takes its species' tissue with the bed's working and history; a trunk (a tree's, a vine's stem, a hedge's core)
is solid, the rest collides with nothing; a cast specimen's parts (the golden iris) all take the cast's material, and
are solid. Every grown surface is drawn on Form's `FFormMesh`.

A planting plan's plan points (points, lines, polygons' corners, a water polygon's too) are the stage's; an ellipse is
centred on its anchor. The museum shows its plants in early summer (Mangzhong to Xiazhi): each species record says
what flowers then, as its floweringShare.

## Later (tier 2, owner Nature)

- Runtime behaviour at dresser order 20: sway in the wind, ripples, and `UNatureWaterComponent` (the level held by
  gravity in a moving basin, `Nature.WaterContained`).
- Phenology by solar term (`MPC_Nature`) and `Nature.InSeason`; the species records name their flowering terms in
  notes today.
- Growth spaces from Form's free space (`FNatureGrowthSpace`), so a marginal keeps to its shelf by geometry rather
  than by its bed's share of the water; `Nature.WithinGrowthSpace`, `Nature.Rooted`, `Nature.WithinBotany`.
- Trees grown in their growth space (space colonisation, the pipe model), where the first pass grows them by their
  habit; fruit; bark as its own tissue (Matter); rock and ground; precipitation.

## Tests

`Musee.Nature.Species`, `Water`, `Growth`, `Planting`, `Garden` (ground less its water, a moss carpet, a tree at its
point, bamboo culms, a clipped hedge, a sward, a vine trained up its post, an empty body).
