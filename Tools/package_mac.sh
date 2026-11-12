#!/bin/sh
# Packages a Development build for Mac (background priority, no window). Output: Saved/Package/Mac/
# The editor target must already be built (Tools/build_editor.sh); do not rebuild it while this runs.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
exec taskpolicy -b "/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
  -project="$ROOT/MegaJam.uproject" -noP4 -platform=Mac -clientconfig=Development \
  -build -nocompileeditor -cook -iterate -stage -pak -package -archive -archivedirectory="$ROOT/Saved/Package" \
  -unattended -utf8output -nosplash
