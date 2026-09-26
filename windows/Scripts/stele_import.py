"""
Imports the Chinese Wing's lettering (stele_inscriptions.py): the 24 stele inscriptions into
/Game/Museum/ChineseWing/Stele (cooked always: the game loads today's by name), today's also over the
imported stele_inscription texture (what the editor shows), and the plaque over the imported plaque texture.
"""
import datetime
import math
import os
import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
ART = os.path.join(HERE, "..", "SourceArt", "Textures")
WING = "/Game/Museum/ChineseWing/ChineseWing/Textures"
STELE = "/Game/Museum/ChineseWing/Stele"


def solar_term(utc):
    """0 立春 … 23 大寒 (the sun's ecliptic longitude from 315 degrees in 15-degree steps; Meeus, low precision)."""
    n = (utc - datetime.datetime(2000, 1, 1, 12)).total_seconds() / 86400.0
    g = math.radians((357.528 + 0.9856003 * n) % 360)
    lam = (280.460 + 0.9856474 * n + 1.915 * math.sin(g) + 0.020 * math.sin(2 * g)) % 360
    return int(((lam - 315) % 360) // 15)


def imp(png, folder, name):
    t = unreal.AssetImportTask()
    t.filename = os.path.abspath(png)
    t.destination_path = folder
    t.destination_name = name
    t.automated = True
    t.replace_existing = True
    t.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])


def main():
    for i in range(24):
        imp(os.path.join(ART, f"T_stele_{i:02d}.png"), STELE, f"T_stele_{i:02d}")
    today = solar_term(datetime.datetime.utcnow())
    imp(os.path.join(ART, f"T_stele_{today:02d}.png"), WING, "stele_inscription")
    imp(os.path.join(ART, "T_plaque.png"), WING, "plaque")
    m = unreal.load_asset("/Game/Museum/ChineseWing/ChineseWing/Materials/stele_inscription")
    names = [str(t.parameter_info.name) for t in m.get_editor_property("texture_parameter_values")]
    unreal.log(f"[stele] 24 inscriptions, today's term {today}; stele_inscription's texture parameters: {names}")


if __name__ == "__main__":
    main()
