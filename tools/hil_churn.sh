#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Flash the examples image on the bench, churn every example, put the release back.
#   tools/hil_churn.sh <folder with the built .bin files>
# Runs on the bench machine (the self-hosted runner) and nowhere else.
set -euo pipefail
cd "$(dirname "$0")/.."
FW_DIR=${1:?usage: tools/hil_churn.sh <folder with the built .bin files>}
HIL=${CROSSPAD_HIL:-crosspad-hil}
H=(--holder crosspad-examples-ci)

APPS=$(grep -ho 'REGISTER_APP\(_PL\)\?( *[A-Za-z0-9_]*' examples/*/src/*.cpp | sed 's/.*( *//' | paste -sd, -)
FW=$(ls "$FW_DIR"/*.bin | grep -v -e ota_data -e partition -e bootloader | head -1)
# The newest platform-idf release, pre-releases included: there is no stable one yet.
TAG=$(gh release list --repo CrossPad/platform-idf --exclude-drafts --limit 1 --json tagName -q '.[0].tagName')
[ -n "$APPS" ] || { echo "error: no REGISTER_APP found under examples/*/src" >&2; exit 1; }
[ -n "$FW" ]   || { echo "error: no firmware .bin in $FW_DIR" >&2; exit 1; }
[ -n "$TAG" ]  || { echo "error: could not find a platform-idf release to restore" >&2; exit 1; }

# Slot budget: the image registers 6 apps of its own (Settings, Firmware, Mixer,
# KitSelector, Sampler, Power OFF) and the registry holds 24 (MAX_APPS).
n=$(echo "$APPS" | tr ',' '\n' | grep -c .)
if [ $((6 + n)) -gt 24 ]; then
  echo "error: $n examples + 6 built-in apps = $((6 + n)) registrations, the registry holds 24 (MAX_APPS); apps past the limit are dropped" >&2
  exit 1
fi

restore() {
  local dir
  dir=$(mktemp -d)
  gh release download "$TAG" --repo CrossPad/platform-idf --pattern "CrossPad-$TAG-v2-full.bin" --dir "$dir" --clobber
  "$HIL" "${H[@]}" flash "$dir/CrossPad-$TAG-v2-full.bin"
  "$HIL" "${H[@]}" bench release "platform-idf $TAG full"
}

"$HIL" "${H[@]}" bench claim "crosspad-examples CI ${GITHUB_SHA:-local}, ~20 min" --ttl 25
# Until we flash, the board is untouched: a failed preflight only gives the bench back.
trap '"$HIL" "${H[@]}" bench release "unchanged: crosspad-examples preflight failed"' EXIT

# Unattended means no on-screen tap, ever. The USB serial guard is readable over
# CDC (a bare USB_GUARD only asks): refuse to start with it on.
# The other half, OTA auto-accept (NVS ota_prefs/auto_accept), has NO read-only
# check in crosspad-hil or in the firmware's CDC verbs. If it is off, the OTA flash
# below stops at OTA_WAIT and fails after the host's 90 s timeout; it never taps.
# Turn it on once per bench board (the OTA prompt's "always allow" box, also in the Update app) and keep it on.
guard=$("$HIL" "${H[@]}" cdc USB_GUARD 2>&1) || true
case "$guard" in
  *"USB_GUARD: 0"*) ;;
  *) echo "error: the bench board's USB serial guard is not off (reply: '$guard'); an unattended run would wait for an on-screen Allow. Turn it off once: $HIL cdc 'USB_GUARD 0'" >&2; exit 1 ;;
esac

trap restore EXIT          # from here on the board is ours and gets put back
"$HIL" "${H[@]}" flash "$FW"
"$HIL" "${H[@]}" run app_churn --apps "$APPS" --rounds 4
"$HIL" "${H[@]}" run back_churn --apps "$APPS" --rounds 4
