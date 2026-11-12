#!/bin/sh
# Play-test the match-cut prototype in a window (this one DOES open a window; it is for a human).
#   Tools/play.sh [ResX ResY]
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
exec "/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
  "$ROOT/MegaJam.uproject" /Game/MatchCut/Lvl_MatchCut -game -windowed -ResX="${1:-1600}" -ResY="${2:-900}" -nosplash -nosound
