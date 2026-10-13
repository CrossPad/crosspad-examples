# CrossPad examples

Small, real apps for the CrossPad, one per recipe in the
[app cookbook](https://crosspad.app/docs/apps/cookbook/). Each one is a complete
app you can build and run, short enough to read in a sitting (about 150 to 250
lines of C++), and each folder links the cookbook page that explains it:
`https://crosspad.app/docs/apps/cookbook/<slug>/`.

One source tree serves both targets: the ESP-IDF firmware and the desktop
simulator.

## The examples

| Folder | App | Kind | What it teaches | Page |
|--------|-----|------|-----------------|------|
| `examples/01-hello-screen` | HelloScreen | UI | `REGISTER_APP`, building and tearing down a screen, the encoder, the power button, closing the app from inside | [Hello screen](https://crosspad.app/docs/apps/cookbook/hello-screen/) |
| `examples/02-pads-leds` | PadsLeds | instrument | `REGISTER_APP_PL`, `IPadLogicHandler`, pad colours, `PadAnimator`, passing pad data to the screen safely | [Pads and LEDs](https://crosspad.app/docs/apps/cookbook/pads-leds/) |
| `examples/03-play-sample` | PlaySample | instrument | the current kit (`IKitManager`), `postSamplePlayback`, playing through the Sampler's pad logic | [Play a sample](https://crosspad.app/docs/apps/cookbook/play-sample/) |
| `examples/05-midi-out` | MidiOut | MIDI controller | `getMidiOutput()`: notes, control change, pitch bend, and no hanging notes | [MIDI out](https://crosspad.app/docs/apps/cookbook/midi-out/) |
| `examples/07-tempo-transport` | TempoTransport | sequencer or tool | `transportTempo`, `TempoChanged`, the smallest `ITransportTarget`, claiming PLAY and giving it back | [Tempo and transport](https://crosspad.app/docs/apps/cookbook/tempo-transport/) |
| `examples/09-motion` | Motion | MIDI controller | `MotionGesture` events, reading `IImu`, `IVibration` presets, running without the sensors | [Motion and vibration](https://crosspad.app/docs/apps/cookbook/motion/) |
| `examples/10-ui-kit` | UiKit | UI | design tokens (`cp_design`), toasts, a confirm dialog, `ui_back_push`, `postToGui` from another task | [UI kit](https://crosspad.app/docs/apps/cookbook/ui-kit/) |

Every example follows the same layout: `examples/NN-<slug>/` with
`crosspad-app.json`, `CMakeLists.txt`, `include/example-<slug>/`, `src/`,
`assets/example-<slug>.png` (48x48) and a host test in `tests/`. The app id is
`example-<slug>` and the registered name, the label you see in the launcher, is
the CamelCase identifier given to `REGISTER_APP`.

## Try them on the simulator

You need a checkout of
[crosspad-pc](https://github.com/CrossPad/crosspad-pc) that builds (see its
README). Then:

```bash
python3 tools/install.py --target pc ~/GIT/crosspad-pc            # every example
python3 tools/install.py --target pc ~/GIT/crosspad-pc hello-screen   # or one, by slug
cd ~/GIT/crosspad-pc
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build
./bin/CrossPad
```

The examples show up in the launcher. To check which apps a running simulator
has registered:

```bash
python3 tools/sim_apps.py
```

Without a display (CI, a server), start the simulator with
`HOME=$(mktemp -d) SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SDL_RENDER_DRIVER=software ./bin/CrossPad`.

To see the MIDI the simulator sends (05 midi-out) on Linux, without a board, run
`python3 tools/midi_monitor.py`. It opens a receive-only ALSA port named
"CrossPad Monitor", which the simulator picks as its MIDI output on its own.

`tools/install.py --target pc <checkout> --remove` takes them out again. It only
touches folders it made itself (they carry a `.crosspad-example` marker).

## Try them on the board

You need a checkout of
[platform-idf](https://github.com/CrossPad/platform-idf) with the ESP-IDF
toolchain it documents.

```bash
python3 tools/install.py --target idf ~/CrossPad            # or one example, by slug
cd ~/CrossPad
idf.py -B build_v2 fullclean                                # apps are found only on a fresh configure
idf.py -B build_v2 -DSDKCONFIG=sdkconfig.v2 build
python3 tools/ota_flash.py                                  # answer Allow, then Update, on the board
python3 tools/fs_transfer.py assets                         # the app icons; an OTA update never writes them
```

The lines are for a v2 board (PCB r20); for a v1 board use `build_v1` and
`sdkconfig.v1`. Run them in a terminal with ESP-IDF loaded. Each cookbook
page's "Run it on the CrossPad" section has the full steps and the output to
expect.

To open an example on the board: pad 8 (**More**) on the MODE map, which lists
every app without a pad of its own, arrives with the next firmware. Until then,
open it from another app with `mode_overlay_open_app("<Name>")`, or, on a
development board in MIDI + Serial mode, over USB serial with the bench's
command-line tool: `crosspad-hil app start <Name>`. Close the MODE map first:
on the board it is the home screen, so hit pad 1 to open the Sampler, then run
the command. Started while the map is up, the app opens but the map keeps the
pads (a known bug).

The registry holds 24 apps and a firmware image registers a handful of its own,
so install the examples you want rather than assuming all fit next to a full
set of apps.

## Run the tests

Host tests cover each example's model (plain C++ with Catch2, no board and no
LVGL). They need a checkout of
[crosspad-core](https://github.com/CrossPad/crosspad-core):

```bash
cmake -S tests -B tests/build -G Ninja -DCROSSPAD_CORE_DIR=<path to crosspad-core>
cmake --build tests/build
ctest --test-dir tests/build --output-on-failure
```

The layout rules (names, manifest, icon, licence header, size) are checked by
Python tests:

```bash
pip install pytest pillow
python3 -m pytest tests/tools -q
```

To draw an example's icon: `python3 tools/make_icon.py example-<slug> 4FC3F7 Hi`.

## Licence

GPL-3.0-or-later. See [LICENSE](LICENSE); every source file carries an SPDX
header.
