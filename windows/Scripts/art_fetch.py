"""
Fetch the full-resolution originals chosen in assets/art_sources.json (the research: museum open access,
Wikimedia Commons originals, Library of Congress masters; public domain or open licences only):

    python windows/Scripts/art_fetch.py [ids...]

Every entry whose action is "replace" or "crop-needed" and whose url is a direct file is downloaded once
into assets/paintings_hires/_sources/ (entries sharing a source, such as the handscroll's four
segments, share the download), then linked to assets/paintings_hires/<id>.<ext> for art.py prepare,
which registers the current framing inside it and crops exactly that. Already-fetched files are kept.
"""
import hashlib
import json
import os
import shutil
import sys
import time

import requests

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
SOURCES = [os.path.join(REPO, "assets", "art_sources.json"), os.path.join(REPO, "assets", "art_sources_extra.json")]
HIRES = os.path.join(REPO, "assets", "paintings_hires")
CACHE = os.path.join(HIRES, "_sources")
AGENT = "MuseeVisionPrototype/1.0 (https://github.com/SherimaX/MuseeVision; personal non-commercial museum prototype)"
PAUSE_S = 30   # between downloads: Wikimedia refuses bursts (HTTP 429)


def entries():
    out = {}
    for path in SOURCES:
        if not os.path.exists(path):
            continue
        with open(path, encoding="utf-8") as f:
            data = json.load(f)
        for e in data if isinstance(data, list) else data.get("works", data.get("entries", [])):
            if e.get("id"):
                out[e["id"]] = e   # later files override earlier ones
    return list(out.values())


def fetch(url):
    ext = os.path.splitext(url.split("?")[0])[1].lower() or ".jpg"
    if ext not in (".jpg", ".jpeg", ".png", ".tif", ".tiff"):
        ext = ".jpg"
    path = os.path.join(CACHE, hashlib.sha1(url.encode()).hexdigest()[:16] + ext)
    if os.path.exists(path) and os.path.getsize(path) > 0:
        return path
    os.makedirs(CACHE, exist_ok=True)
    for attempt in range(3):
        try:
            with requests.get(url, headers={"User-Agent": AGENT}, stream=True, timeout=120) as r:
                r.raise_for_status()
                tmp = path + ".part"
                with open(tmp, "wb") as f:
                    for chunk in r.iter_content(1 << 20):
                        f.write(chunk)
                os.replace(tmp, path)
                return path
        except Exception as e:  # noqa: BLE001
            print(f"  attempt {attempt + 1} failed: {e}")
            time.sleep(PAUSE_S * (attempt + 2))
    return None


def main(ids):
    todo = [e for e in entries() if e.get("action") in ("replace", "crop-needed")
            and str(e.get("url", "")).startswith("http") and (not ids or e["id"] in ids)]
    got, failed = 0, []
    for e in todo:
        print(f"{e['id']}: {e['url'][:110]}")
        fresh = not os.path.exists(os.path.join(CACHE, hashlib.sha1(e["url"].encode()).hexdigest()[:16] + os.path.splitext(e["url"].split("?")[0])[1].lower()))
        path = fetch(e["url"])
        if fresh:
            time.sleep(PAUSE_S)
        if not path:
            failed.append(e["id"])
            continue
        dst = os.path.join(HIRES, e["id"] + os.path.splitext(path)[1])
        for old in [os.path.join(HIRES, e["id"] + x) for x in (".jpg", ".jpeg", ".png", ".tif", ".tiff")]:
            if os.path.exists(old) and old != dst:
                os.remove(old)
        if not os.path.exists(dst):
            shutil.copyfile(path, dst)
        got += 1
        print(f"  {os.path.getsize(dst) / 1e6:.1f} MB")
    print(f"{got} fetched, {len(failed)} failed: {failed}")


if __name__ == "__main__":
    main(set(sys.argv[1:]))
