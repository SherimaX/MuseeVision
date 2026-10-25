"""Tune the look of the running game live (the game started with -Live; Source/MuseeVision/Live/MuseeLiveLook.cpp).
A change shows within a fraction of a second: no editor step, no relaunch. Without -Live the game is unchanged.

    python windows/Scripts/live.py start [game args ...]   start the game with -Live in a 1920x1080 window
    python windows/Scripts/live.py look post.AutoExposureBias=0.5 cvars.r.Lumen.Reflections.MaxRoughnessToTrace=0.6 \\
                                       "lights.Rotunda*.Intensity=8" materials.MI_Ch_Floor.Roughness=0.3
    python windows/Scripts/live.py look --unset post.AutoExposureBias ...   back to what the level has
    python windows/Scripts/live.py look --show                               what look.json sets now
    python windows/Scripts/live.py lights [text]                             the lights by label
    python windows/Scripts/live.py materials [text]                          the material instances and their parameters
    python windows/Scripts/live.py shots <id> <name>=x,y,feet,yaw,pitch[@hour] ... [--views views.json]
                                       [--settle 6] [--fov 84] [--photo] [--hour 14]
        screenshots from the running game, to Saved/Live/shots/<id>/<name>.png, then done.json. Poses are as
        -MuseePose (plan metres and degrees). --photo uses the path tracer (settle 480 s unless given).
        views.json: [{"name": .., "pose": [x, y, feet, yaw, pitch], "hour": .., "fov": .., "settle": .., "photo": ..}]

Keys are section.name.property: post-process fields are FPostProcessSettings names (BloomIntensity or
bloom_intensity), lights are matched by label or name (wildcards work) and take any property of their light component,
materials by instance name or /Game path. Values are JSON where they parse (0.3, true, [1, 0.9, 0.8]) and text
otherwise. Every edit waits for the game and prints "applied in N ms" and any errors: that line is the confirmation.

Look edits are trials. Once a value is right, write it into its source (exposure.py, gallery_lights.py, a wing's
materials script) and unset it here, so the cooked project carries it.
"""
import json
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT = os.path.abspath(os.path.join(HERE, ".."))
LIVE = os.path.join(PROJECT, "Saved", "Live")
LOOK = os.path.join(LIVE, "look.json")
STATE = os.path.join(LIVE, "state.json")
MATERIALS = os.path.join(LIVE, "materials.json")
GAME = r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
SECTIONS = ("post", "cvars", "lights", "materials")


def load(path, default):
    for _ in range(20):
        try:
            with open(path, encoding="utf-8") as f:
                return json.load(f)
        except FileNotFoundError:
            return default
        except (json.JSONDecodeError, PermissionError):
            time.sleep(0.02)   # the game is writing it
    return default


def save(path, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=1)
    for _ in range(50):   # the game may be reading it this instant
        try:
            os.replace(tmp, path)
            return
        except PermissionError:
            time.sleep(0.02)
    os.replace(tmp, path)


def value(text):
    try:
        return json.loads(text)
    except json.JSONDecodeError:
        return text


def split(key):
    """post.X / cvars.r.A.B / lights.<pattern>.Prop / materials.<MI>.Param -> (section, path list)."""
    section, _, rest = key.partition(".")
    if section not in SECTIONS or not rest:
        sys.exit(f"{key}: keys start with one of {', '.join(SECTIONS)}")
    if section in ("post", "cvars"):
        return section, [rest]
    name, _, prop = rest.rpartition(".")
    if not name:
        sys.exit(f"{key}: {section} keys are {section}.<name>.<property>")
    return section, [name, prop]


def wait(before):
    """Wait for the game to apply the change and print its report."""
    t0 = time.time()
    while time.time() - t0 < 10:
        s = load(STATE, {})
        if s.get("applied", 0) > before:
            print(f"live: look applied in {s.get('last_ms')} ms")
            for e in s.get("errors", []):
                print(f"error: {e}")
            return
        time.sleep(0.05)
    print("live: no game answered in 10 s (start one with: live.py start)")


def look(args):
    data = load(LOOK, {})
    if "--show" in args:
        print(json.dumps(data, indent=1))
        return
    unset = "--unset" in args
    before = load(STATE, {}).get("applied", 0)
    for a in (a for a in args if not a.startswith("--")):
        key, eq, text = a.partition("=")
        section, path = split(key)
        node = data.setdefault(section, {})
        if unset:
            if len(path) == 2:
                node.get(path[0], {}).pop(path[1], None)
                if not node.get(path[0]):
                    node.pop(path[0], None)
            else:
                node.pop(path[0], None)
            if not node:
                data.pop(section)
            continue
        if not eq:
            sys.exit(f"{a}: expected key=value")
        if len(path) == 2:
            node.setdefault(path[0], {})[path[1]] = value(text)
        else:
            node[path[0]] = value(text)
    save(LOOK, data)
    wait(before)


def option(args, name, default=None, flag=False):
    """Take --name [value] out of args."""
    if name not in args:
        return default
    i = args.index(name)
    if flag:
        del args[i]
        return True
    v = args[i + 1]
    del args[i:i + 2]
    return v


def shots(args):
    args = list(args)
    views = option(args, "--views")
    settle, fov, hour = option(args, "--settle"), option(args, "--fov"), option(args, "--hour")
    photo = option(args, "--photo", False, flag=True)
    if not args:
        sys.exit("shots <id> <name>=x,y,feet,yaw,pitch[@hour] ... [--views views.json]")
    rid, specs = args[0], args[1:]
    out = []
    if views:
        with open(views, encoding="utf-8") as f:
            out += json.load(f)
    for s in specs:
        name, _, rest = s.partition("=")
        pose, _, at = rest.partition("@")
        shot = {"name": name, "pose": [float(v) for v in pose.split(",")]}
        if at:
            shot["hour"] = float(at)
        out.append(shot)
    if not out:
        sys.exit("no shots: name some views or give --views")
    if hour is not None:
        for s in out:
            s.setdefault("hour", float(hour))
    req = {"id": f"{rid}_{time.strftime('%H%M%S')}", "photo": photo, "shots": out}
    if settle is not None:
        req["settle"] = float(settle)
    if fov is not None:
        req["fov"] = float(fov)
    done = os.path.join(LIVE, "shots", req["id"], "done.json")
    save(os.path.join(LIVE, "shots.json"), req)
    per = float(settle) if settle is not None else (480 if photo else 6)
    limit = 60 + len(out) * (per + 30)
    print(f"live: {len(out)} shots requested as {req['id']}")
    t0 = time.time()
    while time.time() - t0 < limit:
        d = load(done, None)
        if d:
            print(f"live: {len(d['files'])} of {len(out)} shots in {d['seconds']} s -> {os.path.dirname(done)}")
            return
        time.sleep(0.5)
    print(f"live: no done.json after {limit:.0f} s (is a -Live game running? live.py start)")


def listing(path, text, show):
    data = load(path, None)
    if data is None:
        sys.exit(f"{path} isn't there yet: start the game with live.py start")
    for name, entry in sorted(data.items()):
        if not text or text.lower() in name.lower():
            print(show(name, entry))


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    cmd, args = sys.argv[1], sys.argv[2:]
    if cmd == "start":
        uproject = os.path.join(PROJECT, "MuseeVision.uproject")
        subprocess.Popen([GAME, uproject, "-game", "-Live", "-windowed", "-ResX=1920", "-ResY=1080", "-log"] + args)
        print("live: game starting; edits apply once the map has loaded")
    elif cmd == "look":
        look(args)
    elif cmd == "lights":
        state = load(STATE, None)
        if state is None:
            sys.exit(f"{STATE} isn't there yet: start the game with live.py start")
        for name, l in sorted(state.get("lights", {}).items()):
            if not args or args[0].lower() in name.lower():
                print(f"{name}  {l['class']}  intensity {l['intensity']:g}" + (f"  {l['temperature']:g} K" if "temperature" in l else ""))
    elif cmd == "shots":
        shots(args)
    elif cmd == "materials":
        listing(MATERIALS, args[0] if args else "",
                lambda n, e: f"{n}  ({e['master']})  " + ", ".join(f"{k}={v:g}" for k, v in e["scalars"].items()))
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
