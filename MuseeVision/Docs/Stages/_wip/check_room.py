"""Check one room's sheet and schedules against the brief's section 3."""
import json, os, re, sys, glob

SLOT = r"C:\mv\wt\ds-sheets\MuseeVision"
CAT = r"C:\mv\wt\r-light\MuseeVision\Data\Light\Luminaires"
USE = r"C:\mv\wt\r-light\MuseeVision\Data\Light\UseTargets.json"
CARE = r"C:\mv\wt\r-light\MuseeVision\Data\Art\Care.json"
PALETTE = "C:/Users/anan9/AppData/Local/Temp/claude/C--Users-anan9-Desktop-Projects-Musee-Vision/fe83b0c0-89f0-4a7a-aec6-d7928054970e/scratchpad/Palette.json"
COLL = r"C:\Users\anan9\Desktop\Projects\Musee Vision\assets\collection.json"

HEADINGS = ["Identity and placement", "Ports", "Pieces by trade", "Light", "Time and weather", "Motion",
            "Nature", "Art", "Sound", "Visitor", "old-code map", "first-pass cut", "Palette keys to confirm"]


def load(p):
    with open(p, encoding="utf-8") as f:
        return json.load(f)


OLD = r"C:\Users\anan9\Desktop\Projects\Musee Vision"
CATALOGUES = [COLL] + glob.glob(OLD + r"\data\*.json") + glob.glob(OLD + r"\assets\*\*.json") \
    + glob.glob(OLD + r"\assets\*.json")


def collection_ids():
    ids = set()
    for p in CATALOGUES:
        try:
            ids |= ids_in(load(p))
        except Exception:
            pass
    return ids


def ids_in(c):
    ids = set()

    def walk(o):
        if isinstance(o, dict):
            if "id" in o and isinstance(o["id"], str):
                ids.add(o["id"])
            for v in o.values():
                walk(v)
        elif isinstance(o, list):
            for v in o:
                walk(v)
    walk(c)
    return ids


def main(room):
    errs, warns = [], []
    md_path = os.path.join(SLOT, "Docs", "Stages", room + ".md")
    md = open(md_path, encoding="utf-8").read()
    h2 = [l for l in md.splitlines() if l.startswith("## ")]
    pos = -1
    for i, h in enumerate(HEADINGS, 1):
        idx = next((j for j, l in enumerate(h2) if l.lower().startswith(f"## {i}.") and h.lower() in l.lower()), None)
        if idx is None:
            errs.append(f"section {i} '{h}' missing or misnamed")
        elif idx < pos:
            errs.append(f"section {i} out of order")
        else:
            pos = idx
    for bad in ("lumen", "candela", "exposure"):
        for m in re.finditer(bad, md, re.I):
            line = md[max(0, md.rfind("\n", 0, m.start())):md.find("\n", m.end())].strip()
            warns.append(f"sheet mentions '{bad}': {line[:140]}")

    ddir = os.path.join(SLOT, "Data", "Stages", room)
    files = {os.path.basename(p): p for p in glob.glob(os.path.join(ddir, "*.json"))}
    data = {}
    for name, p in files.items():
        try:
            data[name] = load(p)
        except Exception as e:
            errs.append(f"{name} does not parse: {e}")
    for name, d in data.items():
        if d.get("stage") != room:
            errs.append(f"{name}: stage is {d.get('stage')!r}")
        raw = open(files[name], encoding="utf-8").read().lower()
        for bad in ("lumen", "candela", "exposure", "intensity"):
            if bad in raw:
                errs.append(f"{name} contains '{bad}'")
    surfaces = set()
    palette = {e["key"] for e in load(PALETTE)["entries"]}
    if "Finishes.json" not in data:
        errs.append("Finishes.json missing")
    else:
        for f in data["Finishes.json"]["finishes"]:
            for k in ("surface", "material", "working", "history", "note"):
                if k not in f:
                    errs.append(f"finish {f.get('surface')} lacks {k}")
            s = f.get("surface", "")
            m = f.get("material", "")
            if m not in palette and not m.startswith("todo."):
                errs.append(f"finish {s}: material {m} not in palette")
            if "gilt" in f.get("working", "") and f.get("working") not in ("gilt.gold-leaf.oil", "gilt.gold-leaf.water"):
                errs.append(f"finish {s}: gilt working {f.get('working')}")
            if s in surfaces:
                errs.append(f"finish surface duplicated: {s}")
            surfaces.add(s)
            if s and s not in md:
                errs.append(f"finish surface not named in sheet: {s}")
            if s != s.lower():
                errs.append(f"surface not lowercase: {s}")
    # surfaces named in the sheet in backticks that look like surfaces but lack a finish
    ticked = set(re.findall(r"`([a-z0-9-]+(?:\.[a-z0-9-]+)+)`", md))
    cat = {os.path.basename(p)[:-5] for p in glob.glob(os.path.join(CAT, "*.json"))}
    use = {c["key"] for c in load(USE)["classes"]}
    care = {c["key"] for c in load(CARE)["classes"]}
    if "Lighting.json" in data:
        L = data["Lighting.json"]
        ids = set()
        for l in L.get("luminaires", []):
            if l["id"] in ids:
                errs.append(f"luminaire id duplicated: {l['id']}")
            ids.add(l["id"])
            if l["type"] not in cat and not l["type"].startswith("gap."):
                errs.append(f"luminaire {l['id']}: unknown type {l['type']}")
            if l.get("operation") not in ("always", "dusk", "presence"):
                errs.append(f"luminaire {l['id']}: operation {l.get('operation')}")
            for k in ("mount", "aim"):
                if k == "mount" and isinstance(l.get(k), str):
                    continue
                if not (isinstance(l.get(k), list) and len(l[k]) == 3):
                    errs.append(f"luminaire {l['id']}: {k} not [x,y,z]")
        for d in L.get("daylight", []):
            if d.get("weather") not in ("museum", "giverny", "london"):
                errs.append(f"daylight {d.get('opening')}: weather {d.get('weather')}")
        for t in L.get("targets", []):
            if t["surface"] not in surfaces:
                errs.append(f"target surface without finish: {t['surface']}")
            src = t.get("source")
            if (src == "UseTargets" and t["class"] not in use) or (src == "Care" and t["class"] not in care) \
                    or src not in ("UseTargets", "Care"):
                errs.append(f"target {t['surface']}: class {t['class']} / source {src}")
        for d in L.get("design", []):
            for k in ("brighter", "darker"):
                if d[k] not in surfaces:
                    errs.append(f"design ratio surface without finish: {d[k]}")
    anchors_used = set()
    if "Hang.json" in data:
        cids = collection_ids()
        for w in data["Hang.json"]["works"]:
            if w["work"] not in cids:
                errs.append(f"hang work not in any catalogue: {w['work']}")
            if not w.get("source"):
                errs.append(f"hang {w['work']}: no source catalogue")
            if w.get("care") not in care:
                errs.append(f"hang {w['work']}: care {w.get('care')}")
            anchors_used.add(w.get("anchor"))
            if "wall" in w and w["wall"] not in surfaces:
                errs.append(f"hang wall without finish: {w['wall']}")
            if not (("along" in w and "height" in w) or "centre" in w):
                errs.append(f"hang {w['work']}: needs along+height (or centre)")
            if len(w.get("size", [])) != 2:
                errs.append(f"hang {w['work']}: size shape")
    if "Planting.json" in data:
        P = data["Planting.json"]
        for b in P.get("beds", []):
            anchors_used.add(b["bed"])
            if not re.match(r"^[A-Z][a-z]+ [a-z×'-]+", b["species"]):
                warns.append(f"species not a Latin binomial? {b['species']}")
        for w in P.get("water", []):
            anchors_used.add(w.get("anchor"))
            for k in ("key", "anchor", "outline", "depth", "freeboard", "material"):
                if k not in w:
                    errs.append(f"water {w.get('key')}: lacks {k}")
            if w.get("material") not in palette and not str(w.get("material")).startswith("todo."):
                errs.append(f"water {w.get('key')}: material not in palette")
    if "Mechanisms.json" in data:
        for m in data["Mechanisms.json"]["mechanisms"]:
            for k in ("key", "joint", "body", "axis", "travel", "time", "profile", "control", "open", "close", "guard"):
                if k not in m:
                    errs.append(f"mechanism {m.get('key')}: lacks {k}")
            anchors_used.add(m.get("body"))
            anchors_used.add(m.get("control"))
    if "Lighting.json" in data:
        for l in data["Lighting.json"].get("luminaires", []):
            if isinstance(l.get("mount"), str):
                anchors_used.add(l["mount"])
            if l.get("body"):
                anchors_used.add(l["body"])
    for a in sorted(x for x in anchors_used if x):
        if a not in md:
            errs.append(f"anchor/body named in a schedule but not in the sheet: {a}")
    if "anchor" not in md.lower():
        errs.append("sheet has no anchors table")
    print(f"{room}: {len(errs)} errors, {len(warns)} warnings; files {sorted(files)}; "
          f"{len(surfaces)} surfaces; {len(ticked - surfaces)} dotted names in sheet without a finish")
    for e in errs:
        print("  ERR ", e)
    for w in warns:
        print("  warn", w)
    return 1 if errs else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
