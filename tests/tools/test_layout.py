# SPDX-License-Identifier: GPL-3.0-or-later
import json
import re
from pathlib import Path

from PIL import Image

REPO = Path(__file__).resolve().parents[2]
FOLDERS = sorted(p for p in (REPO / "examples").glob("[0-9][0-9]-*") if p.is_dir())
REQUIRED = {"name", "id", "version", "description", "category", "icon", "component_path", "platforms", "requires", "changelog"}


def test_every_example_has_a_complete_manifest():
    for f in FOLDERS:
        m = json.loads((f / "crosspad-app.json").read_text())
        slug = f.name[3:]
        assert REQUIRED <= m.keys(), f
        assert m["id"] == f"example-{slug}", f
        assert m["category"] == "example", f
        assert m["icon"] == f"example-{slug}.png", f
        assert m["component_path"] == f"components/crosspad-example-{slug}", f
        assert m["platforms"] == ["esp-idf", "pc"], f
        assert re.fullmatch(r"\d+\.\d+\.\d+", m["version"]), f


def test_every_example_has_its_icon_its_component_and_a_test():
    for f in FOLDERS:
        slug = f.name[3:]
        icon = Image.open(f / "assets" / f"example-{slug}.png")
        assert icon.size == (48, 48) and icon.mode == "RGBA", f
        assert "WHOLE_ARCHIVE ON" in (f / "CMakeLists.txt").read_text(), f
        assert list((f / "tests").glob("*.cpp")), f
        assert (f / "include" / f"example-{slug}").is_dir(), f


def test_every_source_file_carries_the_licence_and_stays_small():
    for f in FOLDERS:
        lines = 0
        for src in [*f.rglob("*.cpp"), *f.rglob("*.hpp")]:
            text = src.read_text()
            assert text.startswith("// SPDX-License-Identifier: GPL-3.0-or-later"), src
            if "tests" not in src.parts:
                lines += text.count("\n")
        assert 40 <= lines <= 330, f"{f.name}: {lines} lines of source"


def test_no_example_uses_what_the_docs_forbid():
    banned = ["indev_encoderTop", "lv_async_call(", "->destroyApp()", "setActivePadLogic", "pushPadLogic("]
    for f in FOLDERS:
        for src in f.rglob("*.[ch]pp"):
            text = src.read_text()
            for b in banned:
                assert b not in text, f"{src}: {b}"


def test_app_names_are_unique():
    names = []
    for f in FOLDERS:
        for src in (f / "src").glob("*.cpp"):
            names += re.findall(r"REGISTER_APP(?:_PL)?\(\s*(\w+)", src.read_text())
    assert len(names) == len(set(names)) == len(FOLDERS)


def test_the_examples_fit_the_app_registry():
    # The examples image registers 6 apps of its own (facts d); the registry holds 24.
    assert 6 + len(FOLDERS) <= 24


def test_every_folder_under_examples_is_named_nn_slug_and_prefixes_are_unique():
    # FOLDERS only globs NN-*, so a misnamed folder would escape every check above
    # while tools/install.py still installs it.
    dirs = sorted(p.name for p in (REPO / "examples").iterdir() if p.is_dir() and not p.name.startswith("."))
    bad = [d for d in dirs if not re.fullmatch(r"\d\d-[a-z0-9]+(-[a-z0-9]+)*", d)]
    assert not bad, f"misnamed example folders: {bad}"
    prefixes = [d[:2] for d in dirs]
    assert len(prefixes) == len(set(prefixes)), f"duplicate NN prefixes: {prefixes}"
