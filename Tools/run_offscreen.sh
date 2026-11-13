#!/bin/sh
# Runs one test script offscreen, at background priority, without opening a window and without sound.
#   Tools/run_offscreen.sh stage1 Saved/DeepCut/stage1 [ResX ResY]
# The script is a name from Content/Data/tests (no .txt) or a path to a file.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SCRIPT="$1"; OUT="$2"; RX="${3:-1920}"; RY="${4:-1080}"
case "$SCRIPT" in */*) case "$SCRIPT" in /*) ;; *) SCRIPT="$ROOT/$SCRIPT";; esac;; esac
case "$OUT" in /*) ;; *) OUT="$ROOT/$OUT";; esac
mkdir -p "$OUT"
UE="/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor-Cmd"
exec taskpolicy -b "$UE" "$ROOT/MegaJam.uproject" /Game/MatchCut/Lvl_MatchCut -game \
  -RenderOffscreen -ForceRes -ResX="$RX" -ResY="$RY" -windowed -unattended -nosplash -nosound -NoVSync \
  -MCScript="$SCRIPT" -MCOut="$OUT" -abslog="$OUT/run.log" $MC_EXTRA
