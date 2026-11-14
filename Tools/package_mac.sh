#!/bin/sh
# Packages a Shipping build for Mac (Development builds crash on a normal launch: the memory tracker is
# torn down on the game thread while AppKit allocates on the main thread; Shipping has no memory tracker).
# Packages the build (background priority, no window). Output: Saved/Package/Mac/
# The editor target must already be built (Tools/build_editor.sh); do not rebuild it while this runs.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
exec taskpolicy -b "/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
  -project="$ROOT/MegaJam.uproject" -noP4 -platform=Mac -clientconfig="${MC_CONFIG:-Shipping}" \
  -build -nocompileeditor -cook -iterate -stage -pak -package -archive -archivedirectory="$ROOT/Saved/Package" \
  -unattended -utf8output -nosplash
