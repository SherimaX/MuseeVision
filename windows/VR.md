# Musée Vision in VR (Meta Quest 3, from a Windows PC)

The VR build is the same museum, rendered by the PC's RTX 4090 and streamed to a headset through
SteamVR. The main headset is a **Meta Quest 3 with its Touch controllers**, connected with Steam
Link. The Vision Pro also works through ALVR, using hand gestures (see the end of this page).

```
MuseeVisionVR.exe ──OpenXR──▶ SteamVR ──Steam Link, Wi-Fi──▶ Steam Link app on the Quest 3
```

## What to install

- On the PC: **SteamVR** (free, from Steam). It must be the OpenXR runtime; SteamVR offers this
  when it starts, and it's the case on this PC.
- On the Quest 3: **Steam Link** from the Meta Horizon Store (free).

The network matters most: the PC on Ethernet to the router, the Quest on 5 GHz or 6 GHz Wi-Fi, both
on the same network. Meta Horizon Link (a USB-C cable or Air Link) works too; set it as the OpenXR
runtime in the Meta Horizon Link app.

## Run it

1. On the Quest, open **Steam Link** and connect to this PC. SteamVR starts, and you see its grey
   room.
2. Start **Musée Vision VR** from SteamVR's library, or run
   `windows\Builds\MuseeVisionVR-Latest\MuseeVisionVR.exe` on the PC.

`MuseeVisionVR-Latest` always points to the newest build: `package_vr.ps1` repoints it after each
package. Each build keeps its own dated folder, and `windows\Builds\LATEST_VR.txt` names the newest.

### In SteamVR's library (once)

1. In Steam, choose **Games → Add a Non-Steam Game to My Library… → Browse** and pick
   `windows\Builds\MuseeVisionVR-Latest\MuseeVisionVR.exe`. Then click **Add Selected Programs**.
2. Right-click it in the library and choose **Properties**. Name it **Musée Vision VR** and tick
   **Include in VR Library**.

It then appears in SteamVR's library in the headset, and it always launches the newest build.

You arrive on the gilt sun in the Rotunda, facing the west door. Your eyes are at 1.6 m whether you
sit or stand, and the view recentres itself once the headset is tracking. Without a headset, the
same exe is the ordinary desktop game.

To make a new build:

```bash
powershell -ExecutionPolicy Bypass -File windows/Scripts/package_vr.ps1
```

## Controls: Quest Touch controllers

| Do | Controller |
|---|---|
| **Point** | The right controller casts a beam of small dots. Where it lands, a dot opens into a gilt ring over anything you can use, and its sign appears under your view |
| **Use** what the beam points at (the lily, the car, a button) | **Right trigger**, or **A**. With nothing under the beam, it uses what you're looking at |
| **Step** somewhere | Hold the **right trigger** or push the **right stick forward**. The beam becomes an arc with a gilt ring on the floor; let go to step there. Point higher to go further (up to about 6 m) |
| **Walk** | **Left stick** (click it for a brisker walk), or hold the **left trigger** to walk where you look |
| **Turn** 30° | **Right stick** left or right |
| **Another hour** of the sky | **B** |
| **Beijing / New York** (the museum's clock and sky) | **X** |
| **Recentre** (eyes back to 1.6 m, facing ahead) | **Y** |

- **The buttons** for where you stand (for example, the elevator's) float low and to your right.
  Point at one and pull the trigger.
- **Wall labels** appear low and to your left after you look at a work for a moment.
- **Walking around your room** moves you through the museum. Walls stop your body but not your head.

## Controls: bare hands on the Quest 3

Put the controllers down and the Quest tracks your hands. Steam Link passes the hand joints to the
museum (OpenXR hand tracking), which reads the pinches from your fingertips itself:

| Do | Gesture |
|---|---|
| **Point** | The beam runs from your right shoulder through your right pinch |
| **Use** what the beam points at | Right thumb to index finger, a quick pinch |
| **Step** somewhere | Pinch and hold with the right hand, aim the arc, and let go |
| **Walk** where you look | Pinch and hold with the left hand |
| **Turn** 30° | Thumb to middle finger: the right hand turns right, the left hand turns left |
| **Another hour** of the sky | Right thumb to ring finger |
| **Recentre** | Left thumb to ring finger |

Hand tracking must be on in the Quest's settings (**Settings → Movement tracking**) and in Steam
Link's settings. Steam Link opens the SteamVR menu with its own left-hand gesture: a pinch while
looking at your left palm.

To try the controls on a desktop without a headset, start the exe with `-MuseeVR`. Then the right
mouse button uses or steps, and **F** walks.

## How it's built (and why it doesn't disturb the desktop game)

- The code lives in `Source/MuseeVisionVR/`: `AMuseeVRCharacter` (the visitor in a headset),
  `AMuseeVRButton`, `MuseeVRStyle` and `MuseeVRExposure`. The target `MuseeVisionVR` builds it as
  **DebugGame**, with the game code optimised.
- The `.uproject` enables **OpenXR** only for DebugGame game builds, and the `MuseeVisionVR` module
  only for the VR target. An installed engine can't enable plugins per target, so the build
  configuration keeps them apart. The editor, Development and Shipping never load OpenXR.
- The game mode uses the VR visitor when that class is linked in, which is only true in the VR exe.
  The VR visitor is a subclass of the desktop one, so look-and-use, placards, prompts, the elevator
  and the pond all run the same code.
- **In the headset**, the visitor switches to a VR quality preset:
  - Lumen and MegaLights at High, with reflections from Lumen's surface cache.
  - 67% resolution upscaled by DLSS, with Ray Reconstruction.
  - The desktop's DLSS Frame Generation off, because it can't make frames for a headset.

  With both eyes emulated at 3840 × 1920 on the 4090, the GPU takes about 11 ms per frame, which
  is 90 fps. To measure it without a headset, run with `-emulatestereo -MuseeVRQuality`.
- The signs are Slate drawn into widget components, lifted by the inverse of the museum's
  auto-exposure (`MuseeVRExposure`) so they read as on the desktop.
- **Bare hands** come from the `OpenXRHandTracking` plugin, which, like OpenXR, is enabled only
  for DebugGame game builds. `UpdateHands` measures thumb-to-fingertip gaps, closing a pinch under
  2 cm and opening it over 3.5 cm. While a hand's joints are tracked, its trigger button is ignored,
  so SteamVR's own gesture remapping can't fire the same pinch twice.
- For the first five minutes in a headset, the log records every controller button as it goes down
  and up. Run with `-MuseeVRInputLog` to keep it on.
- `package_vr.ps1` cooks into `Saved/CookedVR` and stages in `Saved/StagedBuildsVR`, apart from
  `package.ps1`. It cooks with the editor as it is (`-nocompileeditor`). `-SkipCook` reuses the
  last cook, but only when nothing the map saves has changed since.

## The Vision Pro (ALVR, hand gestures)

Install the **ALVR streamer** for Windows (`alvr-org/ALVR` releases, matching the headset app's
version) and **ALVR** from the visionOS App Store. In the ALVR dashboard, go to **Settings → Headset
→ Controllers** and set:

- **Hand tracking interaction:** on. This turns pinches into Quest buttons.
- **Hand skeleton:** off. With it on, SteamVR flips between hands and controllers and drops the
  right pinch.

ALVR's gestures stand in for the Quest buttons above:

- Thumb to index finger is the trigger.
- Thumb to middle finger is B/Y.
- Thumb to ring finger is A/X.

For example, a right thumb-and-index pinch uses or steps, and a left thumb-and-middle pinch
recentres. ALVR often loses the hand's pose; the beam and the step then follow your gaze.

## Still to do

- **Comfort:** a vignette while walking with the stick, and fading the view when your head goes
  into a wall.
