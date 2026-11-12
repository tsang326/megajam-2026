# Packaging the Windows build

The jam wants a packaged Windows build. It has to be made on the Windows PC: this Mac cannot build Windows binaries. None of the steps below has been run yet. The Mac package (see `docs/slice-report.md`, Packaging) starts and loads the layout from inside the package, which covers the cook list and the staged `Data` folder. It says nothing about Windows libraries or shaders.

## What is already set up in the repo

- `Config/DefaultGame.ini`, `[/Script/UnrealEd.ProjectPackagingSettings]`: the map `/Game/MatchCut/Lvl_MatchCut` is in `MapsToCook`, `/Game/DeepCut` (the materials) is always cooked, and `Content/Data` is staged as UFS, so `Content/Data/deepcut.json` ends up inside the pak.
- `Config/DefaultEngine.ini`: `GameDefaultMap` is the slice map and `GlobalDefaultGameMode` is `/Script/MegaJam.MatchCutGameMode`, so the packaged game starts straight in the opening film.
- The game reads the layout with `FPaths::ProjectContentDir() / "Data/deepcut.json"`, which resolves inside the pak in a packaged build.

## One-time setup on the Windows PC

1. Epic Games Launcher: install **Unreal Engine 5.8** (same minor version as `MegaJam.uproject`).
2. **Visual Studio 2022** with the workloads "Desktop development with C++" and "Game development with C++" (MSVC v143, Windows 10/11 SDK). Without it the C++ module cannot be compiled.
3. Install Git LFS (`git lfs install`), then `git clone git@github.com:tsang326/megajam-2026.git` and `git lfs pull`. The map, the materials and all images are stored in LFS (`.gitattributes`); without the pull they are small pointer files and the cook fails or the game starts with missing materials.

## Package (command line)

From a "x64 Native Tools" or plain `cmd` prompt, with the engine in the default place:

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun ^
  -project="C:\path\to\megajam-2026\MegaJam.uproject" -noP4 -platform=Win64 ^
  -clientconfig=Shipping -build -cook -stage -pak -package -archive ^
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

## What went wrong on Mac, and the Windows version of it

- Without `-package` the Mac archive was an empty shell that died at launch on a missing library. `-package` is in the command above for the same reason. After packaging, check that the output folder holds `Engine\` and `MegaJam\Content\Paks\` next to `MegaJam.exe`, not the exe alone. Then test the zip on a PC that has no Unreal Engine installed: a build can lean on libraries of the local engine install without anyone noticing.
- The Mac package quit at start because the machine fell back to a shader format that had not been cooked. On Windows the config cooks SM6 for DX12 and SM5 for DX11 (`Config/DefaultEngine.ini`, `D3D12TargetedShaderFormats` and `D3D11TargetedShaderFormats`), so `-dx11` has shaders to fall back to. A GPU that can do neither will quit the same silent way. The reason is in the log of a Development build.

## Known unknowns

- DX12 with SM6 is the project default (`DefaultGraphicsRHI_DX12`). On old integrated GPUs DX12 can fail to start; `MegaJam.exe -dx11` is the fallback to try.
- Virtual shadow maps and volumetric fog on Iris Xe are untested.
- The engine prerequisites installer is included by `-prereqs`; judges without the VC++ runtime need it.
