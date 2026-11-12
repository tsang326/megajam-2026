#!/bin/sh
# Builds the editor target from the command line at background priority (no window).
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
exec taskpolicy -b "/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh" MegaJamEditor Mac Development -project="$ROOT/MegaJam.uproject" -waitmutex
