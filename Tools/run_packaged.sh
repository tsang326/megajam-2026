#!/bin/sh
# Runs one test script in the PACKAGED Mac build, offscreen, at background priority, without sound.
#   Tools/run_packaged.sh stage1 Saved/DeepCut/stage1_pkg [ResX ResY]
# The script is a name from Content/Data/tests (these are staged with the build).
# The packaged Mac app is sandboxed: it can only read its own bundle and write inside its container
# (~/Library/Containers/<bundle id>/Data). So the captures are written there and copied to the
# output folder afterwards, and the log is taken from stdout.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SCRIPT="$1"; OUT="$2"; RX="${3:-1920}"; RY="${4:-1080}"
case "$OUT" in /*) ;; *) OUT="$ROOT/$OUT";; esac
mkdir -p "$OUT"
APP="$(ls -d "$ROOT"/Saved/Package/Mac*/MegaJam.app 2>/dev/null | head -1)"
[ -n "$APP" ] || { echo "No packaged build. Run Tools/package_mac.sh first."; exit 1; }
BID="$(/usr/libexec/PlistBuddy -c "Print :CFBundleIdentifier" "$APP/Contents/Info.plist")"
BOX="$HOME/Library/Containers/$BID/Data/Library/Application Support/Epic/MegaJam/Saved/DeepCut"
# old captures are moved aside, never deleted (a delete asks the user a question)
TRASH="$ROOT/Saved/_trash/packaged_$(date +%H%M%S)"; mkdir -p "$TRASH"; mv "$BOX"/*.png "$TRASH"/ 2>/dev/null || true
taskpolicy -b "$APP/Contents/MacOS/MegaJam" -RenderOffscreen -ForceRes -ResX="$RX" -ResY="$RY" -windowed \
  -unattended -nosplash -nosound -NoVSync -MCScript="$SCRIPT" $MC_EXTRA > "$OUT/run.log" 2>&1
cp "$BOX"/*.png "$OUT"/ 2>/dev/null || echo "No captures found in $BOX"
echo "log: $OUT/run.log, captures: $(ls "$OUT"/*.png 2>/dev/null | wc -l)"
