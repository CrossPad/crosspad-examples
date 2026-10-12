# SPDX-License-Identifier: GPL-3.0-or-later
import json
import socket
import subprocess
import sys
import threading
from pathlib import Path

TOOL = Path(__file__).resolve().parents[2] / "tools" / "sim_apps.py"


def serve_once(reply: dict) -> int:
    srv = socket.socket()
    srv.bind(("127.0.0.1", 0))
    srv.listen(1)

    def run():
        conn, _ = srv.accept()
        with conn:
            assert json.loads(conn.recv(4096).decode()) == {"cmd": "app_list"}
            conn.sendall((json.dumps(reply) + "\n").encode())
        srv.close()

    threading.Thread(target=run, daemon=True).start()
    return srv.getsockname()[1]


def test_prints_one_name_per_line_for_string_or_object_entries():
    port = serve_once({"ok": True, "apps": ["Sampler", {"name": "HelloScreen"}]})
    r = subprocess.run([sys.executable, str(TOOL), "--port", str(port), "--timeout", "5"], capture_output=True, text=True)
    assert r.returncode == 0, r.stderr
    assert r.stdout.split() == ["Sampler", "HelloScreen"]


def test_times_out_without_a_simulator():
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        port = s.getsockname()[1]          # bound, never listening: refused
    r = subprocess.run([sys.executable, str(TOOL), "--port", str(port), "--timeout", "1"], capture_output=True, text=True)
    assert r.returncode == 1
    assert "no simulator" in r.stderr
