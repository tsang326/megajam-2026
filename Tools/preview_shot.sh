#!/bin/sh
# Quick look at one shot without playing to it: one offscreen picture, no window, no sound (about 40 s).
#   Tools/preview_shot.sh 2            -> Saved/DeepCut/preview/shot2.png
#   Tools/preview_shot.sh 2 solved     -> the same with its puzzle solved (marks showing)
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
N="${1:-0}"; OUT="$ROOT/Saved/DeepCut/preview"; mkdir -p "$OUT"
S="$OUT/_preview.txt"
{ echo "wait 2.5"; [ "$2" = "solved" ] && echo "givepart" && echo "opengate" && echo "wait 1.6"; echo "snap shot$N${2:+_$2}"; echo "quit"; } > "$S"
MC_EXTRA="-MCShot=$N" "$ROOT/Tools/run_offscreen.sh" "$S" "$OUT" > /dev/null 2>&1 || true
ls -l "$OUT/shot$N${2:+_$2}.png"
