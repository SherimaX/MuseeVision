# Musée Vision · Windows (Unreal 5.8)

The native build of Musée Vision and its source of truth, for an RTX 4090 (see
[`../WINDOWS.md`](../WINDOWS.md)). It started from a one-time import of [`../usd`](../usd/README.md);
walking, the sky and the moving parts are C++ ported from the Swift prototype in `../Shared`, and the
architecture is being rebuilt natively where the import was weak. There is no more Mac export.

## Set up a PC

1. **Unreal Engine 5.8** from the Epic Games Launcher.
2. **A C++ toolchain.** Either Visual Studio 2022 17.14 / 2026 with the *Game development with C++*
   workload, or **Build Tools 2026** with *MSVC*, a *Windows 11 SDK* and the **.NET Framework 4.8 SDK**
   (Unreal's build tool stops without it).
3. **Git LFS** (`git lfs install`) before the first commit of anything in `Content/`: `.uasset` and `.umap`
   files go to LFS (see `../.gitattributes`).

## Build, set up, import

From the repository root, with `UE` pointing at the engine (`C:\Program Files\Epic Games\UE_5.8`):

```bash
"$UE/Engine/Build/BatchFiles/Build.bat" MuseeVisionEditor Win64 Development -Project="$PWD/windows/MuseeVision.uproject" -WaitMutex
```

```bash
"$UE/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "$PWD/windows/MuseeVision.uproject" -run=pythonscript -script="$PWD/windows/Scripts/setup_project.py"
```

```bash
"$UE/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "$PWD/windows/MuseeVision.uproject" -run=pythonscript -script="$PWD/windows/Scripts/import_wing.py Rotunda"
```

`setup_project.py` makes the Museum map (`/Game/Maps/Museum`) with the sky, the exposure and a player
start on the gilt sun, and the few native materials. `import_wing.py <Wing>…` imports wings (or `all`);
re-running a wing replaces it. The order of the brief is the Rotunda first, then the Salon, then the rest.

After an import, and whenever a script's settings change, apply the rest in one editor session. The
engine starts, loads the map and saves it once, rather than once per script:

```bash
"$UE/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "$PWD/windows/MuseeVision.uproject" -run=pythonscript -script="$PWD/windows/Scripts/apply_all.py all"
```

Name only the steps that changed, for example `apply_all.py native lights` or `apply_all.py relight:Salon,Elan lights`.
The steps are `art`, `materials`, `glass`, `exposure`, `relight`, `native`, `frames`, `lights` and `nature`, always run in that order.
Leave out `materials` unless the materials changed, because it recompiles shaders.

Then open `windows/MuseeVision.uproject` in the editor and press Play, or run the game directly:

```bash
"$UE/Engine/Binaries/Win64/UnrealEditor.exe" "$PWD/windows/MuseeVision.uproject" -game
```

It opens full screen at 4K. The first run compiles every shader and takes a while.

### A standalone game

```bash
powershell -ExecutionPolicy Bypass -File windows/Scripts/package.ps1
```

It stages the runtime data (the catalogues and the stars) into `Content/MuseeData`, then builds, cooks
and packages a Shipping build to its own folder, `windows/Builds/MuseeVision-<date-time>/` (git-ignored, about 1.2 GB; `windows/Builds/LATEST.txt` names the newest). Run
its `MuseeVision.exe`; **Esc** quits. `-Config Development` keeps the console
(`~`) for `musee.Hour` and the like.

### In VR (Meta Quest 3, from this PC)

`Scripts/package_vr.ps1` packages `MuseeVisionVR`, the same museum streamed through SteamVR to a
Meta Quest 3 (via Steam Link) and played with its Touch controllers. The Vision Pro also works,
through ALVR. It doesn't change the desktop game or the editor. See [`VR.md`](VR.md).

## Controls

- **Walk:** WASD or the arrows, or the left stick; **Shift** (or clicking the stick) for a brisker walk.
- **Look:** the mouse, or the right stick.
- **Use** what you look at (the golden lily, a rack, the car): click, **E**, or **A**.
- **Placards** appear when you look at a work for a moment.
- **Buttons** for what can be done where you stand appear lower right: **1–4** (or B, X, Y, D-pad down).
- **Photo mode:** **P** (or the View button) switches to the path tracer; hold still and it converges
  and denoises in about 80 seconds at 4K. **F9** saves a screenshot at twice the resolution to
  `Saved/Screenshots`; **P** again returns.
- **Time of day:** **T** (or D-pad up) steps the sky: now → 10:00 → 16:00 → 21:00 → now. The sky, the
  sun clock and the stars otherwise follow the real clock where you are.
- **Quit:** **Esc**.

## Display and rendering

- **4K, full screen, native:** 3840 × 2160 exclusive full screen with VSync (`Config/DefaultGameUserSettings.ini`),
  rendered at 100% (no automatic resolution drop), DPI aware so a 4K display at 150% scaling stays 4K.
  About 67 fps in the Rotunda and 45 fps in the Atrium on the RTX 4090 without DLSS.
- **Ray tracing:** Lumen with hardware ray tracing and hit lighting for reflections, ray-traced
  shadows, ray-traced translucency with refraction (glass), MegaLights; quality set on the post-process
  volume by `setup_project.py`. The path tracer (256 samples, NNE denoiser) for photo mode.
- **HDR output is off** (SDR).

## Development options

- `-MuseePose=x,y,feet,yaw,pitch` starts the visitor there (plan metres and degrees; yaw 0 faces east,
  90 south, 180 west). The Atrium's centre is `54,0,0`.
- `-MuseeShot=C:/path/shot.png` waits for the shaders, saves a screenshot and quits; it ignores input and
  hides the HUD. Add `-MuseePhoto` for a path-traced one; `-fullscreen -ResX=3840 -ResY=2160` for 4K.
- `-MuseeQuit=<seconds>` runs normally and quits.
- `musee.Hour 9.5` (console) shows the sky at that local hour; `-1` returns to the live clock.

## How it fits together

| Where | What |
|---|---|
| `Source/MuseeVision/Plan/MuseePlan.h` | The plan's numbers and the coordinate mapping (Unreal X = x × 100 east, Y = plan y × 100 south, Z = height × 100). |
| `Source/MuseeVision/Sky/` | The ephemerides (sun, moon, sidereal time, solar terms) and `AMuseeSky`: the Rotunda's idealised sun for the whole museum (the clock keeps local time), the real moon and the 9,096 stars. |
| `Source/MuseeVision/Visitor/` | The visitor (`AMuseeCharacter`), the game mode, the look-and-use interfaces, and `MuseeWorld` (finding imported actors by their tags). |
| `Source/MuseeVision/UI/MuseeHUD` | The dot, the glass placard, the buttons and the message line. |
| `Source/MuseeVision/Catalog/` | The placards from `../data` and `../assets/collection.json`, read from the repository (or `Content/MuseeData` when packaged). |
| `Source/MuseeVision/Elan/ElanStructure` | The Atrium's misty-glass drum, the Sphere (R 14 m, centre 22 m up, its underside the Atrium's ceiling) and the ribs, built natively: the Élan layer in `usd/` predates this design. |
| `Scripts/` | Editor Python: `setup_project.py`, `import_wing.py`, `musee_paths.py`. |
| `Import/` | Generated wrapper layers for each wing (git-ignored). |

Imported actors keep their USD prim names as labels and are tagged from the USD: `musee.building`,
`musee.wing:<Wing>`, `prim:/Museum/…`, `part:<movingPart>` (for the moving parts) and `work:<id>` (for
the placards). Nanite is on for opaque meshes; collision is the mesh itself.

## Where it stands

Done:

- The project: Lumen with hardware ray tracing, MegaLights, Nanite, virtual shadow maps, DX12 SM6.
- Walking and collision, look-and-use, placards, and the sky.
- All seven wings imported with their scans.
- The Élan drum and Sphere as native geometry.
- The elevator: calls, rides, doors, the Square first, and the Sphere mode. It finds its imported
  parts, but no ride has been tested yet.

Next, in the brief's order:

- The Rotunda's materials, lighting and coffers, checked against
  `plan/renderings/05-the-rotunda-v2.png`.
- The same pass for the other wings.
- The other moving parts: the pond, racks, easel, handscroll, cup and stereographs.
- The gardens, sound and packaging.

DLSS is on (`r.NGX.DLSS.Enable=1`): DLAA at the desktop's native 100%, and DLSS upscaling from 67%
in the headset. NVIDIA's plugin is git-ignored (1.1 GB), so on a new PC, download **"UE 5.8 DLSS
Plugin"** from [developer.nvidia.com/rtx/dlss/get-started](https://developer.nvidia.com/rtx/dlss/get-started)
(sign in, accept the license), and copy its `Plugins/DLSS` and `Plugins/StreamlineNGXCommon` into
`windows/Plugins/`.

Known issues:

- Unreal's decoder rejects five painting JPEGs (listed in `Scripts/musee_paths.py`).
  `import_wing.py` re-encodes them to PNG in `Import/images/`.
- Light leaks in the exported geometry, plain in the path tracer: slits of sky beside the Rotunda's
  niches and doors and along the dome's base (the dome's ring doesn't meet the drum), and single-sided
  walls round openings. Being rebuilt natively, wing by wing (see `GEOMETRY_AUDIT.md`).
- The misty glass's frit reads too coarse from a distance.
