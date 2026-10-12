#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Print the apps a running simulator has registered, one per line.

    SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software ./bin/CrossPad &
    python3 tools/sim_apps.py --timeout 60

Talks to the simulator's control port (127.0.0.1:19840, one JSON object per line).
"""
import argparse
import json
import socket
import sys
import time


def names(reply: dict) -> list[str]:
    return [a["name"] if isinstance(a, dict) else str(a) for a in reply.get("apps", [])]


def ask(port: int) -> dict:
    with socket.create_connection(("127.0.0.1", port), timeout=5) as s:
        s.sendall(b'{"cmd":"app_list"}\n')
        buf = b""
        while not buf.endswith(b"\n"):
            chunk = s.recv(65536)
            if not chunk:
                break
            buf += chunk
    return json.loads(buf)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--port", type=int, default=19840)
    ap.add_argument("--timeout", type=float, default=30)
    a = ap.parse_args(argv)
    deadline = time.monotonic() + a.timeout
    while True:
        try:
            reply = ask(a.port)
            break
        except (OSError, ValueError):
            if time.monotonic() > deadline:
                print(f"error: no simulator answered on 127.0.0.1:{a.port}", file=sys.stderr)
                return 1
            time.sleep(0.5)
    for n in names(reply):
        print(n)
    return 0 if reply.get("ok", True) else 1


if __name__ == "__main__":
    sys.exit(main())
