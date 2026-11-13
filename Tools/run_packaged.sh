#!/bin/sh
# Runs one test script in the PACKAGED Mac build, offscreen, at background priority, without sound.
#   Tools/run_packaged.sh stage1 Saved/DeepCut/stage1_pkg [ResX ResY]
# The script is a name from Content/Data/tests (these are staged with the build).
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SCRIPT="$1"; OUT="$2"; RX="${3:-1920}"; RY="${4:-1080}"
case "$OUT" in /*) ;; *) OUT="$ROOT/$OUT";; esac
mkdir -p "$OUT"
APP="$(ls -d "$ROOT"/Saved/Package/Mac*/MegaJam.app 2>/dev/null | head -1)"
[ -n "$APP" ] || { echo "No packaged build. Run Tools/package_mac.sh first."; exit 1; }
exec taskpolicy -b "$APP/Contents/MacOS/MegaJam" -RenderOffscreen -ForceRes -ResX="$RX" -ResY="$RY" -windowed \
  -unattended -nosplash -nosound -NoVSync -MCScript="$SCRIPT" -MCOut="$OUT" -abslog="$OUT/run.log" $MC_EXTRA
