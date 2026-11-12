# Prototype report – match-cut movement (brief 01)

2026-11-12. Everything below comes from scripted offscreen runs of the `-game` build on the M1 Pro, all at background priority (`taskpolicy -b`). Nobody has played it with a keyboard yet. Each claim names the log or capture it rests on; the last section lists what is not verified.

## Verdict

**The rule holds as a mechanism. Whether it holds as a game is not decided by this prototype.**

- The cut is exact and repeatable. It does not depend on window size, and that part of the risk is gone.
- All five shots can be finished by walking and cutting only (no teleports), including the timing pair.
- The cost of the rule is in level design, and it is high: a landing surface is a region of the screen, so every camera move changes every cut into and out of that shot. I could not design the shots by eye and had to write a solver (`Tools/cutmap.py`) first.
- Three things about the feel can only be judged by a person holding the controls: whether lining up against the ghost is satisfying or fiddly, whether an NG reads as fair, and whether the ghost is readable once shots have real art. None of these was tested.

My recommendation: play it for the planned 10–15 minutes before committing. If lining up feels good in shots 1–3, the rule is safe to build on for a jam-sized game of roughly 8–12 shots, with the constraints listed under "What would break first". If it feels like pixel hunting, fall back; nothing in the code will fix that.

## What was built

- `AMatchCutGameMode` spawns five greybox shots from `Content/MatchCut/shots.json`, each with one fixed camera. Sequence: SOUNDSTAGE → LEDGE → ONE MARK → CRANE → FINALE → "THAT'S A WRAP".
- CUT (C / gamepad B or X): the hero's feet are projected through the current camera into a fixed 16:9 frame, the same point is deprojected through the next camera, and the first thing that ray hits must be the top face of an actor tagged `Cuttable` with room for the capsule. Otherwise NG with a reason, 0.9 s, and the shot restarts.
- Ghost: the next shot rendered to a 960x540 target and added over the frame at 35 %. No landing marker.
- Movement is the template character, input relative to the shot camera. Velocity and facing are rotated by the yaw difference between the two cameras.
- Colours: orange = landing surface, dark grey = not a landing surface, green = goal.

| Pair | Teaches | Fail shown | Success shown |
|---|---|---|---|
| 1 SOUNDSTAGE → LEDGE | cut across a gap onto a ledge | void, and untagged floor | lands on the ledge |
| 2 LEDGE → ONE MARK | only one spot works (9.2 % of the ledge) | void from the start mark | lands on the 200x200 mark |
| 3 ONE MARK → CRANE | timing: platform moves in the next shot | void when the platform is away | lands on it and rides it |
| 4 CRANE → FINALE | wrong spot is safe but useless | side wall of the raised block | pit (safe, cannot reach the goal; R retakes) and the raised block → goal |

## Evidence

Logs and full-size captures are in `Saved/MatchCut/` (not in git). Small copies of the key captures are in `docs/captures/`.

- Pairs: `Saved/MatchCut/pairs/run.log` and `p1_*` … `p4_*.png`. The log has, for every pair, at least one `CUT … NG reason=…` and one `CUT … OK` line, and ends with `WRAP reached in shot 4`.
- Whole sequence by walking: `Saved/MatchCut/playthrough/run.log` – four `CUT … OK` lines, no `FELL`, then `WRAP`. `run_0_moved_right.png` shows the hero moved screen-right after one second of "right" input.
- Cut accuracy: feet after the cut are 1.40–2.36 px (at 1080p) from where they were before it, in every OK line. That offset is the 2-unit lift used when placing the capsule, not drift.
- Our own screen position against the engine's: `PROJ` lines show `diff=0.00px` in every run.

One capture does not show what its name says: `p3_b_fail_timing.png` was written after the NG had already ended, so it shows the retake, not the NG text. The log line for that NG is there (`CUT 2->3 NG reason="nothing there"`).

## Letterbox

Same script (`Tools/tests/letterbox.txt`) at four sizes. The cut gives identical world results in all of them, and the picture is the same 16:9 frame with bars.

| Window | View rect in the log | Feet before cut (ndc) | Landed at | Bars |
|---|---|---|---|---|
| 1920x1080 | (0,0)-(1920,1080) | (0.2654, 0.1272) | (-104, 266) | none |
| 1280x720 | (0,0)-(1280,720) | (0.2654, 0.1272) | (-104, 266) | none |
| 2560x1080 | (320,0)-(2240,1080) | (0.2654, 0.1272) | (-104, 266) | left/right |
| 1024x768 | (0,96)-(1024,672) | (0.2654, 0.1272) | (-104, 266) | top/bottom |

HUD text and the ghost stay inside the 16:9 rect (`docs/captures/letterbox_*.png`). This was run on macOS/Metal through `-ForceRes` offscreen. Windows, real DPI scaling and real fullscreen were not run; the cut maths never reads the window, so I expect the same result, but that is reasoning, not a test.

## Frame time

From `Saved/MatchCut/frametime/run.log`, 1920x1080, about 6 s per line, no captures:

| Shot | Ghost | Average | Worst |
|---|---|---|---|
| 0 | on | 73.8 ms | 385 ms |
| 1 | on | 92.9 ms | 400 ms (clamped) |
| 0 | off | 27.6 ms | 74 ms |
| 1 | off | 33.5 ms | 333 ms |
| 2 | on | 50.0 ms | 329 ms |
| 3 | on | 43.5 ms | 174 ms |
| 4 | none (last shot) | 34.3 ms | 255 ms |

These numbers are not what a player will see: every run was forced to background priority, which on Apple Silicon keeps the process on the two efficiency cores, and it is the editor binary, not a packaged build. What they do show is the ratio. Turning the ghost off cut the frame time to roughly a third in the same shots. The ghost is a second full scene render every frame and is the main cost of this design. Frame time at normal priority is unmeasured.

## Landing edge cases found

1. **Long frames and moving platforms.** The first version left the hero falling the last 2 units after a cut. At ~5 fps the platform moved out from under them before they landed, and a correct cut turned into a fall. Fixed: a successful cut now puts the hero in walking mode, based on the surface, at once. Seen in an earlier `pairs` run, gone in the current one (`p3_e_riding.png`).
2. **Falling was a shortcut.** A fall respawned the hero on that shot's start mark, which in CRANE is past the platform ride. Fixed: a fall now sends you back to redo the previous cut. A failed cut still restarts the current shot, as the brief says. The fix compiles and the walking run passes, but no test falls on purpose after it, so the new path itself is unexercised.
3. **Side faces are NG and they are big on screen.** With the camera pitched about 35° down, the side of a 300-unit block covers more of the frame than its top. Thin slabs keep this band narrow; pair 4 still has a "hit a wall" band (6.7 % of the floor in shot 4).
4. **Small targets.** Share of shot A's walkable floor from which a cut succeeds (10-unit grid, from `cutmap.py`): pair 1 17.7 %, pair 2 9.2 %, pair 3 0 % with the platform away and 80 % when it passes, pair 4 89 %. Pair 2's good area is about 200x200 units, a little over two capsule widths. That is the "one spot" lesson working, and also the size at which it may start to feel like pixel hunting.
5. **The ghost can cover the shot.** In CRANE the next shot's floor fills most of the frame, so the ghost tints almost everything orange and the current shot gets harder to read (`p4_b_fail_wall.png`). With textured art this will be worse.
6. **Depth is ambiguous by design.** Two surfaces on the same line of sight look the same in the ghost; the nearer one wins. Pair 4 uses this on purpose (block over pit). Players get no cue for which one they will land on except occlusion.
7. **Jumping.** The cut uses the feet, so a jump moves the cut point up the screen. It is allowed; vertical speed is dropped on landing. In these five shots a jump at the start marks does not open any cut that walking does not (checked in `cutmap.py` only), but nothing stops a later layout from being skippable by jump-cutting.
8. **Timing window.** In pair 3 the platform is under the cut point for about 1.3 s of its 6 s cycle, in two passes. Not tuned with a human.

## What would break first

1. **Level design time.** Each shot's camera is coupled to both neighbours. With real art, moving a camera for composition silently breaks cuts. A full game needs the solver in the loop (it reads the same JSON) or an in-editor preview, and layouts must be locked before art.
2. **Ghost readability and cost** once shots have art, lighting and effects. An additive 35 % overlay of a lit scene over another lit scene will turn to mud, and it doubles or triples render cost. A flat mask of the landing surfaces only would be cheaper and clearer, but it moves toward the explicit marker the brief rules out.
3. **Packaging.** The game reads `Content/MatchCut/shots.json` from disk and `Config/` has no staging entry for it, so a packaged build would most likely start with no shots. The test map is not in the cook list either. No package was built. Details in `docs/notes-prototype.md`.
4. **Fairness of NG.** Reasons are text only. A wall hit and a void miss a few pixels from the surface look the same to the player before the cut.
5. **Camera angles.** All five shots look down from 32–38°. Flat side-on or very steep shots have not been tried; landing surfaces shrink to slivers at low angles.
6. **Two deprecated engine calls** (`SetBase` with a component; warning in UE 5.8) will stop compiling in a later engine version. Irrelevant for the jam.

## How to play it

```
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" "/Users/os/Documents/side-gigs/itch/megajam-2026/MegaJam/MegaJam.uproject" /Game/MatchCut/Lvl_MatchCut -game -windowed -ResX=1600 -ResY=900 -nosplash -nosound
```

or `Tools/play.sh [ResX ResY]`. This opens a window. WASD/arrows move, Space jump, C cut, R retake (back to the previous shot), Esc quit.

## Not verified

- **Keyboard and gamepad input.** Every run was driven by the script runner, which calls the cut and the movement directly. The key polling in `AMatchCutPlayerController` compiles but has never received a real key press. The windowed launch command above has not been run either, because it would open a window on a machine in use.
- Frame time at normal priority, and anything on Windows.
- The fall-sends-you-back path (edge case 2) and the "no room to stand" and "out of frame" NG reasons: code exists, no test hits them.
- Whether it is fun. That was never something a script could answer.
- **The fresh-context review asked for in the brief did not happen.** A separate reviewer was started and stopped on a usage limit before returning anything. In its place I re-checked my own report: the letterbox table, the frame-time table and the 1.40–2.36 px range were re-read from the logs and match, and the before/after captures of all four pairs were looked at again. That is the author checking the author, so treat it as weaker than an independent pass.
