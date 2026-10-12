# CrossPad examples

Small, real apps for the CrossPad, one per recipe in the
[app cookbook](https://crosspad.app/docs/apps/cookbook/). Each one is a complete
app you can build and run, short enough to read in a sitting (80 to 250 lines of
source), and each folder links the cookbook page that explains it:
`https://crosspad.app/docs/apps/cookbook/<slug>/`.

One source tree serves both targets: the ESP-IDF firmware and the desktop
simulator.

## The examples

| Folder | App | What it teaches |
|--------|-----|-----------------|
| _The examples are added one by one; this table fills as they land._ | | |

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

`tools/install.py --target pc <checkout> --remove` takes them out again. It only
touches folders it made itself (they carry a `.crosspad-example` marker).

## Try them on the board

You need a checkout of
[platform-idf](https://github.com/CrossPad/platform-idf) with the ESP-IDF
toolchain it documents.

```bash
python3 tools/install.py --target idf ~/CrossPad/platform-idf
cd ~/CrossPad/platform-idf
idf.py -B build_v2 -DSDKCONFIG=sdkconfig.v2 fullclean build flash
```

On the device press MODE and open **More**: the examples are listed with the
other apps. The registry holds 24 apps and a firmware image registers a handful
of its own, so install the examples you want rather than assuming all fit next
to a full set of apps.

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
