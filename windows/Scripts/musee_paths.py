"""Paths shared by the editor scripts (run inside Unreal's Python)."""
import os

import unreal

PROJECT_DIR = os.path.normpath(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
REPO_DIR = os.path.normpath(os.path.join(PROJECT_DIR, ".."))
USD_DIR = os.path.join(REPO_DIR, "usd")
IMPORT_DIR = os.path.join(PROJECT_DIR, "Import")          # generated wrapper layers (git-ignored)

MAP_PATH = "/Game/Maps/Museum"
MUSEUM_CONTENT = "/Game/Museum"
MATERIALS = MUSEUM_CONTENT + "/Materials"

# Wings in the order of the brief (WINDOWS.md): the Rotunda first.
WINGS = ["Rotunda", "Salon", "Reserve", "SculptureHall", "ChineseWing", "HallOfLight", "Elan"]

# Paintings whose JPEG streams Unreal's decoder rejects ("corrupt JPEG data": stray bytes before the
# end marker, or a truncated segment). Browsers and Windows read them; import_wing.py re-encodes them
# to PNG in Import/images/ and points their materials there. The files in assets/ are left as they are.
BAD_JPEGS = [
    "chinese-thousand-li-1.jpg",
    "chinese-thousand-li-3.jpg",
    "monet-rouen-cathedral-3.jpg",
    "monet-water-lilies-clouds.jpg",
    "pissarro-boulevard-montmartre-winter.jpg",
]
