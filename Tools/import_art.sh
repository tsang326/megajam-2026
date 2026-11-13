#!/bin/sh
# Imports the models of a manifest.json into Content/DeepCut/Art (headless editor, background priority, no window).
#   Tools/import_art.sh                                  ArtDrop -> Content/Data/art.json
#   Tools/import_art.sh SourceAssets/art_test art_test.json /Game/DeepCut/ArtTest
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export DC_ART_DIR="${1:-ArtDrop}" DC_ART_OUT="${2:-art.json}" DC_ART_DEST="${3:-/Game/DeepCut/Art}"
exec taskpolicy -b "/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor-Cmd" "$ROOT/MegaJam.uproject" \
  -run=pythonscript -script="$ROOT/Tools/import_assets.py" -nullrhi -unattended -nosplash -nosound
