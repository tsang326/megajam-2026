#!/bin/sh
# Play Deep Cut in a window, with sound (this one DOES open a window; it is for a human).
#   Tools/play.sh                      the 3D game (brief 06/07)
#   Tools/play.sh 1920 1080            the same, at another window size
#   Tools/play.sh 1600 900 -MC2D       the old side-view game (also: git tag v2d-fallback)
#   Tools/play.sh 1600 900 -MCSens=0.08   mouse sensitivity, degrees per count (default 0.11; [ and ] change it in game)
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
exec "/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
  "$ROOT/MegaJam.uproject" /Game/MatchCut/Lvl_MatchCut -game -windowed -ResX="${1:-1600}" -ResY="${2:-900}" -nosplash ${3:+"$3"} ${4:+"$4"}
