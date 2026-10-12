#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Print the MIDI the CrossPad simulator sends (Linux, ALSA; standard library only).

Creates a receive-only MIDI port named "CrossPad Monitor". The simulator connects
its MIDI output to the first port whose name contains "CrossPad", so it finds this
one on its own; because the port only receives, the simulator's MIDI input stays on
a port nothing writes to, and the app's notes cannot come back to it as pad presses.
Channels are printed 1 to 16, as on the CrossPad's screen. Stop with Ctrl+C.
"""
import ctypes
import sys

NAME = b"CrossPad Monitor"
OPEN_INPUT = 2                      # SND_SEQ_OPEN_INPUT
CAP_WRITE, CAP_SUBS_WRITE = 1 << 1, 1 << 6
TYPE_MIDI_GENERIC, TYPE_APPLICATION = 1 << 1, 1 << 20
NOTEON, NOTEOFF, CONTROLLER, PITCHBEND = 6, 7, 10, 13


def main() -> int:
    try:
        alsa = ctypes.CDLL("libasound.so.2")
    except OSError:
        print("libasound.so.2 not found: this monitor needs Linux with ALSA.", file=sys.stderr)
        return 1
    seq = ctypes.c_void_p()
    if alsa.snd_seq_open(ctypes.byref(seq), b"default", OPEN_INPUT, 0) < 0:
        print("Cannot open the ALSA sequencer.", file=sys.stderr)
        return 1
    alsa.snd_seq_set_client_name(seq, NAME)
    port = alsa.snd_seq_create_simple_port(seq, NAME, CAP_WRITE | CAP_SUBS_WRITE,
                                           TYPE_MIDI_GENERIC | TYPE_APPLICATION)
    print(f"Listening on {NAME.decode()} ({alsa.snd_seq_client_id(seq)}:{port}). Ctrl+C to stop.",
          flush=True)
    ev = ctypes.POINTER(ctypes.c_ubyte)()
    while True:
        if alsa.snd_seq_event_input(seq, ctypes.byref(ev)) < 0:
            continue
        kind, data = ev[0], ctypes.addressof(ev.contents) + 16   # snd_seq_event_t.data
        ch = ctypes.c_ubyte.from_address(data).value + 1
        if kind in (NOTEON, NOTEOFF):
            note, vel = ev[17], ev[18]
            label = "Note on " if kind == NOTEON else "Note off"
            print(f"{label}        ch {ch:2}  note {note:3}  velocity {vel}", flush=True)
        elif kind == CONTROLLER:
            cc = ctypes.c_uint.from_address(data + 4).value
            val = ctypes.c_int.from_address(data + 8).value
            print(f"Control change  ch {ch:2}  controller {cc}  value {val}", flush=True)
        elif kind == PITCHBEND:
            val = ctypes.c_int.from_address(data + 8).value
            print(f"Pitch bend      ch {ch:2}  value {val}", flush=True)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(0)
