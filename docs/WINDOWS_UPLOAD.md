# Windows build: package and upload (deadline 16 Nov 08:59 Vietnam time)

For the session on the Windows laptop. The Mac build is already uploaded to the same itch page by the Mac session.

- itch page: https://tsang1.itch.io/deep-cut (project "Deep Cut", account tsang1). Upload the Windows build to THIS page, do not create a new project.
- Jam rule: "packaged for Windows ... as a development build", zip named `TeamName_GameName`, under 1 GB for the browser upload (bigger needs butler).
- Full background notes: `docs/packaging-windows.md`.

## 1. Get the code (latest `main`)

```
git clone https://github.com/tsang326/megajam-2026.git   (or, if already cloned: git pull --ff-only)
cd megajam-2026
git lfs install
git lfs pull
```

Check `git log --oneline -1` shows `3561ac0` or newer. Without `git lfs pull` the `.uasset` files are pointer files and the cook fails.

Needs: Unreal Engine 5.8 (Epic Launcher) and Visual Studio 2022 with "Desktop development with C++" + "Game development with C++".

## 2. Package (Development, Win64)

From `cmd` in the repo folder (fix the engine path if UE is installed elsewhere):

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun ^
  -project="%CD%\MegaJam.uproject" -noP4 -platform=Win64 ^
  -clientconfig=Development -build -cook -stage -pak -package -archive ^
  -archivedirectory="%CD%\Saved\Package" -prereqs -unattended -utf8output
```

The first run compiles all shaders; it is slow. Output: `Saved\Package\Windows\` with `MegaJam.exe`.

If it fails, read the first `Error:` line in the output (or `Saved\Logs`). Do not change gameplay code to make it pass; only build/packaging settings.

## 3. Quick check

Run `Saved\Package\Windows\MegaJam.exe`. It must reach the station (first-person diver in the base). Press ESC: the pause menu opens. Quit.

## 4. Zip

Zip the whole `Saved\Package\Windows` folder as `tsang1_DeepCut_Windows.zip` (right click > Send to > Compressed folder, or `tar -a -c -f tsang1_DeepCut_Windows.zip -C Saved\Package Windows`).

## 5. Upload to the same itch page

- Browser: itch.io > Dashboard > Deep Cut > Edit game > Uploads > "Upload files", choose the zip, tick **Windows** as the platform, then **Save**. Browser limit 1 GB.
- Over 1 GB, or if the browser upload stalls: butler (itch's official uploader, from https://itch.io/docs/butler/), `butler login`, then `butler push tsang1_DeepCut_Windows.zip tsang1/deep-cut:windows`.

Do not delete or replace the Mac upload already on the page.

## 6. Tell the user

Report: commit packaged, zip size, upload done (yes/no), and anything that failed.

## Rules for this session

- Do not commit or push anything back to the repo (the Mac session owns `main`). Do not commit `Saved/`.
- No force push, no `git reset --hard`.
