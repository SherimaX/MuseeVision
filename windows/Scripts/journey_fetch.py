"""
The Élan Cube's own texture sets (CC0, ambientCG), fetched as pbr_fetch.py does into windows/SourceArt/Journeys/PBR/<Key>/:
Color.png (sRGB), Normal.png (DirectX), ORM.png (AO, roughness, metalness; linear), Height.png. Credits:
windows/SourceArt/Journeys/CREDITS.md (written with the terrain's). The museum's library (SourceArt/PBR) is left alone.

    python windows/Scripts/journey_fetch.py [KEY ...]      (all when none; a set already fetched is skipped)
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.append(HERE)
import pbr_fetch  # noqa: E402

OUT = os.path.join(HERE, "..", "SourceArt", "Journeys", "PBR")

# key: (source, asset id, resolution, what it stands for, metres per repeat)
SETS = {
    # The ground behind the shaft's glass: under the Atrium's slab, a little crushed stone; topsoil; clay-with-flints; chalk.
    "JS1": ("ambientcg", "Ground048", "2K", "topsoil, dark and crumbly", 0.9),
    "JS2": ("ambientcg", "Ground104", "2K", "clay-with-flints: ochre clay, stones", 1.2),
    "JS3": ("ambientcg", "Rock019", "2K", "chalk, bedded", 1.6),
    "JS4": ("ambientcg", "Rock041", "2K", "flint (dark, smooth, glassy)", 0.4),
    "JS5": ("ambientcg", "Gravel023", "2K", "crushed stone under the slab", 0.8),
    # The Matterhorn: gneiss and its scree, snow, glacier ice.
    "JR1": ("ambientcg", "Rock030", "2K", "grey layered rock (gneiss, the Matterhorn's faces)", 3.0),
    "JR2": ("ambientcg", "Rock028", "2K", "grey cliff rock (the ridge's blocks)", 2.5),
    "JR3": ("ambientcg", "Rocks011", "2K", "broken slabs (scree and the ridge's crest)", 2.0),
    "JN1": ("ambientcg", "Snow010A", "2K", "wind-packed snow", 2.0),
    "JN2": ("ambientcg", "Snow005", "2K", "fine new snow", 1.5),
    "JI1": ("ambientcg", "Ice002", "2K", "glacier ice (for the crevasse's relief)", 2.0),
    # Raja Ampat: coral sand, rubble, the karst's limestone.
    "JW1": ("ambientcg", "Ground055S", "2K", "rippled coral sand", 2.0),
    "JW2": ("ambientcg", "Ground101", "2K", "fine pale sand", 1.5),
    "JW3": ("ambientcg", "Rocks007", "2K", "coral rubble", 1.5),
    "JK1": ("ambientcg", "Rock018", "2K", "weathered pale limestone (karst)", 3.0),
}


def fetch(key):
    pbr_fetch.OUT = OUT
    pbr_fetch.SETS[key] = SETS[key]
    try:
        pbr_fetch.fetch(key)
    finally:
        del pbr_fetch.SETS[key]


def credits_rows():
    rows = []
    for key in sorted(SETS):
        src, asset, res, what, scale = SETS[key]
        rows.append(f"| {key} | {asset} (ambientCG, {res}) | {what} | {scale} m | https://ambientcg.com/view?id={asset} | CC0 1.0 |")
    return rows


if __name__ == "__main__":
    keys = sys.argv[1:] or list(SETS)
    os.makedirs(OUT, exist_ok=True)
    for k in keys:
        fetch(k)
