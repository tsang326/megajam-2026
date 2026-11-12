# Notes – match-cut prototype (brief 01)

Lessons and gotchas, newest at the bottom. Factual; anything not checked says so.

## How the prototype is put together

- No level geometry is saved in a map. `AMatchCutGameMode` (Source/MegaJam/MatchCut) reads `Content/MatchCut/shots.json` at start and spawns cameras, boxes, lights and the hero. The map `/Game/MatchCut/Lvl_MatchCut` is empty and only selects the game mode.
- Shots sit 50 000 units apart on world Y, so a shot never sees another one and the ghost capture only shows the next shot.
- The cut never touches the window. The hero's feet are projected with the shot camera's own FOV and a fixed 16:9 aspect into [-1,1]², and the same point is deprojected from the next camera. Window size, DPI and letterbox bars cannot change where a cut lands.
- Rule as built: first blocking hit of the ray in the next shot must be an actor tagged `Cuttable`, hit on an upward face (normal Z ≥ 0.7), and the capsule must fit there. Anything else is NG: "nothing there", "not a landing surface", "hit a wall", "no room to stand", "out of frame".
- Velocity and facing are rotated by the yaw difference between the two cameras, so motion keeps its on-screen direction. Vertical speed is dropped (a cut always puts the feet on the surface).
- Ghost: one `SceneCapture2D` parked on the next shot's camera, 960x540, drawn by the HUD with additive blending at 35 %. The next shot's void is black and adds nothing.
- Colours: orange = tagged landing surface, dark grey = untagged, green = goal mark.
- Input is polled in `AMatchCutPlayerController` (WASD/arrows, Space, C, R, Esc; gamepad left stick, A jump, B/X cut, Y retake). No Enhanced Input assets were added. Gamepad is untested.

## Tools

- `Tools/cutmap.py` mirrors the rule in plain Python and prints, for each shot pair, where on shot A a cut succeeds. Use it to design shots; it runs in milliseconds. `--png DIR` writes small previews with the ghost.
- Script runner: `-MCScript=<file> -MCOut=<dir>` drives the game without input (teleport, walk, cut, screenshots, quit). Commands are listed above `LoadScript()` in MatchCutGameMode.cpp. Scripts live in `Tools/tests/`.
- `Tools/run_offscreen.sh <script> <out dir> [ResX ResY]` runs one script offscreen at background priority.

## Gotchas

- The prototype cube mesh (`LevelPrototyping/Meshes/SM_Cube`) does not have its pivot in the centre. Boxes are placed from the mesh bounds, not from an assumed pivot.
- Side faces eat screen space. With a camera pitched ~35° down, the side of a 300-unit-tall block covers more of the frame than its top, and every pixel of it is an NG ("hit a wall"). Thin slabs (40 units) keep the NG band narrow. A raised platform still casts a thin NG band along its visible edges.
- A landing surface is a screen-space region, not a place. Moving a camera moves every cut that enters or leaves that shot; the layout has to be solved pair by pair, in order. `cutmap.py` exists because doing this by eye was not workable.
