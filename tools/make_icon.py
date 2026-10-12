#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Draw an example's 48x48 icon: a rounded tile in one colour with two letters.

    python3 tools/make_icon.py example-hello-screen 4FC3F7 Hi
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

REPO = Path(__file__).resolve().parent.parent


def main(app_id: str, colour: str, letters: str) -> int:
    slug = app_id.removeprefix("example-")
    folders = [p for p in (REPO / "examples").glob(f"[0-9][0-9]-{slug}") if p.is_dir()]
    if len(folders) != 1:
        print(f"error: no single examples/NN-{slug}/", file=sys.stderr)
        return 2
    img = Image.new("RGBA", (48, 48), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle((2, 2, 45, 45), radius=9, fill="#" + colour)
    font = ImageFont.load_default(size=20)
    d.text((24, 24), letters[:2], font=font, fill="#101418", anchor="mm")
    out = folders[0] / "assets" / f"{app_id}.png"
    out.parent.mkdir(exist_ok=True)
    img.save(out)
    print(out)
    return 0


if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:4]))
