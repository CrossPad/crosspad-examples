#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Put the examples into a platform-idf or crosspad-pc checkout, or take them out.

    python3 tools/install.py --target idf ~/CrossPad/platform-idf            # every example
    python3 tools/install.py --target pc  ~/GIT/crosspad-pc hello-screen      # one, by slug
    python3 tools/install.py --target idf ~/CrossPad/platform-idf --remove    # take them out

An example lands where the build looks for apps: components/crosspad-<id> on
ESP-IDF, src/apps/crosspad-<id> on the simulator. Each copy carries a
.crosspad-example marker; a folder without one is never overwritten or removed.
Reconfigure afterwards (ESP-IDF: idf.py fullclean; simulator: cmake -B build).
"""
import argparse
import json
import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
MARKER = ".crosspad-example"
DEST = {"idf": "components", "pc": "src/apps"}


def examples(names):
    for folder in sorted((REPO / "examples").iterdir()):
        manifest = folder / "crosspad-app.json"
        if not manifest.is_file():
            continue
        app_id = json.loads(manifest.read_text())["id"]
        slug = app_id.removeprefix("example-")
        if not names or slug in names or folder.name in names:
            yield folder, app_id


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--target", choices=DEST, required=True)
    ap.add_argument("root", type=Path)
    ap.add_argument("names", nargs="*", help="example slugs (default: all)")
    ap.add_argument("--remove", action="store_true")
    a = ap.parse_args(argv)

    base = a.root / DEST[a.target]
    if not base.is_dir():
        print(f"error: {a.root} has no {DEST[a.target]}/ -- is it a {a.target} checkout?", file=sys.stderr)
        return 2

    chosen = list(examples(set(a.names)))
    known = {name for folder, app_id in chosen for name in (folder.name, app_id.removeprefix("example-"))}
    unknown = [n for n in a.names if n not in known]
    if unknown:
        slugs = ", ".join(app_id.removeprefix("example-") for _, app_id in examples(set()))
        print(f"error: no example named {', '.join(unknown)} (examples: {slugs})", file=sys.stderr)
        return 2

    for folder, app_id in chosen:
        dest = base / f"crosspad-{app_id}"
        if dest.exists() and not (dest / MARKER).exists():
            print(f"error: {dest} exists and was not installed by tools/install.py; move it first", file=sys.stderr)
            return 2
        if a.remove and not dest.exists():
            continue
        if dest.exists():
            shutil.rmtree(dest)
        if a.remove:
            print(f"removed {dest}")
            continue
        shutil.copytree(folder, dest, ignore=shutil.ignore_patterns("build", "__pycache__"))
        (dest / MARKER).write_text(f"{folder.name}\n")
        print(f"installed {app_id} -> {dest}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
