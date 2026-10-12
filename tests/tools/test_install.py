# SPDX-License-Identifier: GPL-3.0-or-later
import json
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
INSTALL = REPO / "tools" / "install.py"


def make_example(root: Path, folder: str, app_id: str) -> None:
    ex = root / "examples" / folder
    (ex / "src").mkdir(parents=True)
    (ex / "src" / "App.cpp").write_text("// app\n")
    (ex / "crosspad-app.json").write_text(json.dumps({"id": app_id, "name": app_id, "version": "0.1.0"}))


def run(*args, cwd):
    return subprocess.run([sys.executable, str(INSTALL), *args], cwd=cwd, capture_output=True, text=True)


def fake_repo(tmp_path):
    repo = tmp_path / "examples-repo"
    (repo / "tools").mkdir(parents=True)
    (repo / "tools" / "install.py").write_text(INSTALL.read_text())
    make_example(repo, "01-hello-screen", "example-hello-screen")
    make_example(repo, "02-pads-leds", "example-pads-leds")
    return repo


def test_idf_install_copies_every_example_with_a_marker(tmp_path):
    repo, proj = fake_repo(tmp_path), tmp_path / "platform-idf"
    (proj / "components").mkdir(parents=True)
    r = subprocess.run([sys.executable, "tools/install.py", "--target", "idf", str(proj)], cwd=repo, capture_output=True, text=True)
    assert r.returncode == 0, r.stderr
    dest = proj / "components" / "crosspad-example-hello-screen"
    assert (dest / "src" / "App.cpp").read_text() == "// app\n"
    assert (dest / ".crosspad-example").exists()
    assert (proj / "components" / "crosspad-example-pads-leds").is_dir()


def test_pc_install_goes_to_src_apps_and_names_filter(tmp_path):
    repo, proj = fake_repo(tmp_path), tmp_path / "crosspad-pc"
    (proj / "src" / "apps").mkdir(parents=True)
    r = subprocess.run([sys.executable, "tools/install.py", "--target", "pc", str(proj), "pads-leds"], cwd=repo, capture_output=True, text=True)
    assert r.returncode == 0, r.stderr
    assert (proj / "src" / "apps" / "crosspad-example-pads-leds").is_dir()
    assert not (proj / "src" / "apps" / "crosspad-example-hello-screen").exists()


def test_install_refuses_to_overwrite_a_folder_it_did_not_make(tmp_path):
    repo, proj = fake_repo(tmp_path), tmp_path / "platform-idf"
    mine = proj / "components" / "crosspad-example-hello-screen"
    mine.mkdir(parents=True)
    (mine / "keep.txt").write_text("mine")
    r = subprocess.run([sys.executable, "tools/install.py", "--target", "idf", str(proj)], cwd=repo, capture_output=True, text=True)
    assert r.returncode == 2
    assert "not installed by tools/install.py" in r.stderr
    assert (mine / "keep.txt").read_text() == "mine"


def test_reinstall_replaces_and_remove_deletes_only_marked(tmp_path):
    repo, proj = fake_repo(tmp_path), tmp_path / "platform-idf"
    (proj / "components" / "crosspad-sampler").mkdir(parents=True)
    for _ in range(2):
        assert subprocess.run([sys.executable, "tools/install.py", "--target", "idf", str(proj)], cwd=repo).returncode == 0
    r = subprocess.run([sys.executable, "tools/install.py", "--target", "idf", str(proj), "--remove"], cwd=repo, capture_output=True, text=True)
    assert r.returncode == 0, r.stderr
    assert sorted(p.name for p in (proj / "components").iterdir()) == ["crosspad-sampler"]


def test_unknown_target_root_is_an_error(tmp_path):
    repo = fake_repo(tmp_path)
    r = subprocess.run([sys.executable, "tools/install.py", "--target", "idf", str(tmp_path / "nope")], cwd=repo, capture_output=True, text=True)
    assert r.returncode == 2
    assert "no components/" in r.stderr
