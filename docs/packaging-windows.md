# Packaging the Windows build

The jam wants a packaged Windows build. It has to be made on the Windows PC: this Mac cannot build Windows binaries. None of the steps below has been run yet; the Mac package (see `docs/slice-report.md`, Packaging) proves the same cook and staging settings work.

## What is already set up in the repo

- `Config/DefaultGame.ini`, `[/Script/UnrealEd.ProjectPackagingSettings]`: the map `/Game/MatchCut/Lvl_MatchCut` is in `MapsToCook`, `/Game/DeepCut` (the materials) is always cooked, and `Content/Data` is staged as UFS, so `Content/Data/deepcut.json` ends up inside the pak.
- `Config/DefaultEngine.ini`: `GameDefaultMap` is the slice map and `GlobalDefaultGameMode` is `/Script/MegaJam.MatchCutGameMode`, so the packaged game starts straight in the opening film.
- The game reads the layout with `FPaths::ProjectContentDir() / "Data/deepcut.json"`, which resolves inside the pak in a packaged build.

## One-time setup on the Windows PC

1. Epic Games Launcher: install **Unreal Engine 5.8** (same minor version as `MegaJam.uproject`).
2. **Visual Studio 2022** with the workloads "Desktop development with C++" and "Game development with C++" (MSVC v143, Windows 10/11 SDK). Without it the C++ module cannot be compiled.
3. `git clone git@github.com:tsang326/megajam-2026.git` and `git lfs pull` if the repo uses LFS by then (it does not today).

## Package (command line)

From a "x64 Native Tools" or plain `cmd` prompt, with the engine in the default place:

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun ^
  -project="C:\path\to\megajam-2026\MegaJam.uproject" -noP4 -platform=Win64 ^
  -clientconfig=Shipping -build -cook -stage -pak -archive ^
  -archivedirectory="C:\path\to\megajam-2026\Saved\Package" -prereqs -unattended -utf8output
```

- Use `-clientconfig=Development` for a first test: it keeps the log (`MegaJam\Saved\Logs\MegaJam.log` next to the exe) and the console.
- The first run compiles every shader for Windows and can take a long time. Later runs are fast.
- Output: `Saved\Package\Windows\` with `MegaJam.exe`. Zip that whole folder for itch.

Or in the editor: Platforms > Windows > Shipping, then Platforms > Windows > Package Project.

## Check before uploading

1. Run `MegaJam.exe` on the PC. The opening film must start by itself.
2. In a Development build open `MegaJam\Saved\Logs\MegaJam.log` and look for `Layout loaded from ... 6 shots`. If it says `Cannot read layout`, the `Data` folder was not staged.
3. Play to the end card once with the keyboard.
4. Try it once on the weakest PC available (the Intel Iris Xe one). If it is slow there, start with `MegaJam.exe -MCLook=B` or `-MCLook=C`, or change `"look"` in `deepcut.json` before packaging. Look A uses volumetric fog and is the most expensive of the three.
5. `-MCSkipFilm` skips the opening film (useful while testing). Remove `Saved\DeepCut\intro_seen.txt` to get the "first view" behaviour back.

## Known unknowns

- DX12 with SM6 is the project default (`DefaultGraphicsRHI_DX12`). On old integrated GPUs DX12 can fail to start; `MegaJam.exe -dx11` is the fallback to try.
- Virtual shadow maps and volumetric fog on Iris Xe are untested.
- The engine prerequisites installer is included by `-prereqs`; judges without the VC++ runtime need it.
