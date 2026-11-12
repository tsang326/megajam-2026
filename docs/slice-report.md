# Deep Cut slice report (brief 02)

Written 2026-11-12, Vietnam time. Everything below was checked with scripted offscreen runs (`-game -RenderOffscreen`, 1920x1080, background priority) and by looking at the captures. **No human has played this build, and keyboard input has not been pressed by anyone.** The script drives the same functions the keys call (`PressCut`, `PressLamp`, `PressRetake`, `PressAnyKey`) and the same movement code, but it does not go through the player controller.

Evidence: `Saved/DeepCut/slice/run.log` and the PNGs next to it (not in git); downscaled copies in `docs/captures/slice/`.

## What works

One scripted run (`Tools/tests/slice.txt`, 40 captures) goes from the opening film to the end card:

| Step | Log line | Capture |
|---|---|---|
| Opening film, first view, 25.5 s, a key press does not skip it | `FILM start length=25.5s skippable=0` | `f01_launch`, `f03_gap1`, `f05_unit7`, `f07_end` |
| Film drops the player into shot 1, the reel strip slides to the top bar | `FILM end -> drop into shot 0`, `DROP done: playing` | `g00_drop`, `g01_shot0_start_green` |
| Cut a (plain). A press in the red gives a line of text, no failed take | `CUT 0->1 NG reason="ROCK IN THE WAY" ... first=1`, then `CUT 0->1 OK` | `g03_shot0_soft_fail` |
| Cut b (one open spot). Failed take, then success | `CUT 1->2 NG reason="ROCK IN THE WAY"`, `CUT 1->2 OK` | `g06_shot1_take2_rock`, `g07_shot1_one_spot_green` |
| Cut c (timing, a plate drifts across the next shot) | `CUT 2->3 NG reason="ROCK IN THE WAY"`, `CUT 2->3 OK` | `g09_shot2_plate_red`, `g10_shot2_take2_timing`, `g11_shot2_plate_clear_green` |
| Next shot is dark and there is no lamp yet | `CUT 3->4 NG reason="TOO DARK"` | `g13_shot3_too_dark` |
| The part | `PART picked up in shot 3: lamp granted, restore-by-cut armed` | `g15_part_found_restore_ghost` |
| Restore-by-cut, failed then lined up | `RESTORE NG reason="NOT LINED UP" ... off=448px`, `RESTORE OK ... off=35px` | `g17_restore_take2`, `g18_restore_lined_up` |
| Flashback, 12 s, fills gap 1 in the reel | `FLASHBACK start (removed scene 1, 12s)`, `FLASHBACK end: gap 1 filled` | `g20_flashback_b`, thumbnail in the reel in `g23` |
| Cut d (dark shot, lamp on). Failed take on rock, then success | `CUT 3->4 NG reason="ROCK IN THE WAY" ... lamp=1`, `CUT 3->4 OK ... lamp=1` | `g23_lamp_on_pocket_green`, `g25_dark_take2_rock`, `g27_shot4_dark_lamp` |
| Rescue, the crew robot follows | `RESCUE crew robot freed in shot 4 (lamp=1): CREW 1/7` | `g29_rescue`, `g30_crew_follows` |
| Last cut and end card | `CUT 4->5 OK`, `END CARD scenes=1/3 crew=1/7` | `g32_end_card` |

Every successful cut logs `err=0.00px@1080p`: the robot is at the same screen position after the cut.

Also in place: reel strip in the top letterbox bar with three red gaps, camera tag and timecode in the bottom bar (nothing is drawn over the picture except the corner marks and the cut preview), the film is skippable with any key from the second view on (flag file `Saved/DeepCut/intro_seen.txt`; the skip path itself was only exercised through `-MCSkipFilm` and the `skipfilm` script command, not through a second scripted viewing).

## What does not work, or is not verified

- Not played by a person. Feel (float, brake distance, how hard the one-spot and timing cuts are) is unjudged.
- Keyboard and gamepad polling in `MatchCutPlayerController.cpp` compiled but was never exercised.
- Frame times below are not reliable (see Frame times).
- The timing cut cannot be failed by standing still in the wrong place only: with the plate at the far end of its travel the solver says no spot works, at the near end 23.7 % of the water works. Whether six seconds is a comfortable period is unjudged.
- In the dark shot the next shot's ghost (open water, not dark) is drawn at full strength over a black picture. The brown floor of the ghost is the brightest thing on screen (`g28_shot4_dark_no_lamp`). It is readable but looks wrong.
- The chimney shot is two thirds one flat rock face (`g11`). It reads as a black wall.
- The clapper text ("TAKE 2") and the end title are the engine's bitmap font scaled up, and they are blurry.
- The look-dev captures show the first tutorial line in shot 4 because the script jumps there with `goto`. In a normal run that line goes away after the first cut.
- No audio at all.
- One scripted run failed half way: the machine was under load (frame time 130 ms and more), the script's swim target sat on the edge of the dark pocket, and the cut was refused. The target was moved to the middle of the pocket and the next run passed. A player at that frame rate would simply correct; the scripts are fragile at low frame rates.

## Cut-rule choices and why

- **Rule.** On CUT the robot keeps its screen position and its distance along the camera's forward axis, clamped to the next shot's depth range. The take fails if that point is outside the picture, outside the next shot's swim rectangle, or touches rock there, or if the next shot is dark and the lamp is off.
- **Fail, not pull-forward.** The fail version is in. The pull-forward alternative was not tried: with the landing ring showing green or red before the press, a refused cut is announced in advance, so it did not look unfair in the captures. That is a guess until someone plays it.
- **Everything is 2D in the camera plane.** Every rock is a box facing the camera, authored as a rectangle in screen units at the depth the robot has in that shot. Collision is a circle against rectangles, done by hand. Reason: the engine cube's physics body turned out to be hollow to overlap tests (the first validity mask showed "free water" inside rocks), and a 2D rule is exactly what the offline solver `Tools/cutmap.py` can mirror.
- **Movement.** Custom 2D float, not UE's Swimming mode: acceleration 1500, top speed 430, drag 2.4, and 2.5 times that drag when no key is held so the robot settles in about one body length. Velocity is kept in screen axes, so it carries across cameras whatever their yaw and pitch. Soft push back near the edges of the swim rectangle.
- **Lamp rule.** A dark shot shows no ghost and no valid area. With the lamp on, the ghost and the valid area show only inside a circle around the robot (radius 0.40 of the half width), and a cut into a dark shot is only accepted with the lamp on. So: turn the lamp on, sweep it around, find the green.
- **Failed take.** Clapper card "TAKE n" with the reason for 0.85 s, then back to the shot's start mark. R goes back one shot.
- **Restore-by-cut.** After the pickup the ghost switches to the removed scene's frame, with a ring marked MATCH on the place of the robot in that frame. C inside 0.13 of the mark plays the flashback. The normal cut out of that shot is blocked until the scene is restored.
- **Rescue.** Touching the crew robot frees it. It sits in the only pocket the dark cut can land in, so reaching it needs the cut and the lamp. It follows and keeps its place through cuts.

Solver numbers (`python3 Tools/cutmap.py`): the cut works from 67.0 % of the reachable water for cut a, 11.4 % for cut b, 23.7 % (plate near) to 0 % (plate far) for cut c, 11.9 % for the dark cut, 100 % for the last one.

## Ghost in fog (the early test)

`Tools/tests/ghostfog.txt`, captures `docs/captures/slice/gf_shot0_old_ghost.jpg` and `gf_shot0_new_ghost.jpg`.

The brief-01 ghost (the next shot's lit frame, fog included, added at 35 %) does not work underwater: both shots are the same fog green, so they blend into one picture and nothing says which rock belongs to the next shot. It was replaced before building further. The ghost is now a base-colour capture of the next shot (no lights, no fog), tinted warm, with the valid landing area tinted and outlined in green and a ring on the robot. That one reads against the fog in every capture of the run.

## Frame times

Same shot (CAM 04, lamp on, ghost off), about 10 s each, `Tools/tests/lookdev.txt`, log `Saved/DeepCut/lookdev/run.log`:

| Look | Capture | Average | Frames |
|---|---|---|---|
| A, murky green, volumetric fog, shafts | `Saved/DeepCut/lookdev/look_A.png` | 25.84 ms | 366 |
| B, clear deep blue, strong lamp | `Saved/DeepCut/lookdev/look_B.png` | 29.36 ms | 331 |
| C, flat toon bands | `Saved/DeepCut/lookdev/look_C.png` | 20.65 ms | 459 |
| A with the ghost on | `Saved/DeepCut/lookdev/look_A_with_ghost.png` | 74.51 ms | 110 (8 s) |

Do not rank the looks on these numbers. The runs are at background priority (`taskpolicy -b`) on a Mac that was in use (load average around 14), every window has hitches up to the 400 ms clamp, and the same setting measured twice differs more than the looks do: an earlier run gave A 27.95, B 24.98, C 47.84, and in this run look A without the ghost measured 22.07 ms and then 43.83 ms a few seconds later. What can be said: none of the three looks is clearly more expensive than the others here, and the ghost capture is probably still the largest single cost (also seen in brief 01). A clean number needs an idle machine at normal priority, and the Windows PC is the one that matters.

Look C needed a fix during the run: with the lights off its emissive bands were about three times too dark for the fixed exposure, so the first capture was silhouettes. The bands are now multiplied by 3.

## Packaging

`Tools/package_mac.sh` (RunUAT BuildCookRun, Mac, Development) ends with `BUILD SUCCESSFUL`. Output: `Saved/Package/Mac/MegaJam.app`, 890 MB.

The packaged app was started offscreen at background priority. Its output is kept in `Saved/DeepCut/packaged/stdout_noscript.txt`:

```
LogMatchCut: Display: Layout loaded from ../../../MegaJam/Content/Data/deepcut.json: 6 shots, 3 looks, fov 60.0
LogMatchCut: Display: FILM start length=25.5s skippable=0
LogMatchCut: Display: FILM end -> drop into shot 0
LogMatchCut: Display: DROP done: playing
```

So the packaged build starts by itself in the opening film, reads the layout from inside the package, plays the film and hands control to the player in shot 1. A second start logged `intro seen before: 1`, so the first-view flag survives between runs.

Not proven in the packaged build: nothing past shot 1, and there is no picture from it. The test-script runner could not open a script file there (`Cannot read script`, two folders tried), so no cut was made and no capture was taken. Nobody has looked at the packaged game's screen.

It took three packages to get there. The first two did not start:

1. **The first package (finished about 22:04) was only a shell.** The script had no `-package` step, and on Mac the archive step then copies a bare `MegaJam.app` holding the executable and nothing else: no `Contents/UE`, so no game data and no engine libraries. It died at launch in dyld (`Library not loaded: @rpath/libtbb.12.dylib`, and `libmetalirconverter.dylib` after that). This is the build that crashed when it was opened by hand. It was a finished build, not one caught half way. Fix: `-package` added to `Tools/package_mac.sh`. The archive now carries `Contents/UE`, with both libraries put there by the engine's own staging (no manual copy).
2. **The second package started and quit at once.** On this Mac (macOS 27.2.1) UE 5.8 logs `To use SM6 on this system, please ensure you are running Mac OS 15. Falling back to SM5`, and the template's config cooked Metal SM6 shaders only. The editor hides this because it compiles SM5 shaders on demand. Fix: `Config/DefaultEngine.ini` now targets both `SF_METAL_SM5` and `SF_METAL_SM6`.
3. **The third package runs**, as above.

Every look-dev and slice capture in this report was rendered with Metal SM5 in the editor binary, not in the package.

Windows steps: `docs/packaging-windows.md` (none of them run).

## Placeholder lines

Every line the player can read. All are placeholders until approved. They live in `Source/MegaJam/MatchCut/DeepCutText.h`, except the per-shot hints and shot names (`Content/Data/deepcut.json`) and two HUD literals marked below. ASCII only: the engine font has no en dash.

Opening film
- `FATHOM DYNAMICS  /  MISSION FILM  /  OFFICIAL RELEASE`
- `FATHOM DYNAMICS SURVEY CREW. LAUNCH DAY.`
- `DESCENT TO THE SEA FLOOR.`
- `EXPLORATION MISSION SUCCESSFUL.`
- `UNIT 7 - DEFECTIVE, DECOMMISSIONED.`
- `[SCENE REMOVED]`
- `REMOVED BY: ________`
- `PRESS ANY KEY TO SKIP`
- `END OF FILM.  3 SCENES MISSING.`
- `REMOVED` (label on a gap in the reel)

Camera frame
- `REC`, `LIVE`, `FATHOM CAM 01` .. `06`, `SCENE 01` .. `06`, `TAKE 1`, `TAKE 2` ..
- Shot names: `DROP ZONE`, `THE SHELF`, `THE CHIMNEY`, `THE WRECK`, `THE DARK`, `OPEN WATER`

Cutting
- `Line up with the ghost. Press C to cut.`
- `Not here. Move to the green area.`
- `C: CUT`
- `ROCK IN THE WAY`, `TOO DARK`, `TOO DARK - F: LAMP`, `NO WATER THERE`, `OUT OF FRAME`, `NOT LINED UP`, `NO NEXT SHOT`
- `CUT`
- Hints: `Rock blocks the cut. Find the open water.` / `A plate drifts across the next shot. Cut when the ring is green.` / `Something is shining. Swim to it.` / `A crew robot is stuck. Swim to it.`

Part, restore, rescue
- `HEADLAMP FOUND`
- `F: lamp on / off`
- `A removed scene. Line up with the frame. Press C.`
- `MATCH`
- `RECOVERED FOOTAGE`
- `SCENE 1/3` (HUD literal, under the flashback), `TC 03:23:29:10` (HUD literal, flashback timecode)
- `SCENE RESTORED  1/3`
- `The next shot is dark. Turn on the lamp (F) and look for the signal.`
- `CREW SIGNAL`
- `CREW FOUND`
- `CREW 0/7`, `CREW 1/7`

General
- `WASD move    C cut    R retake    ESC quit`
- `WASD move    C cut    F lamp    R retake    ESC quit`
- `END OF SLICE`
- `Scenes restored 1/3.   Crew found 1/7.`
- `R: play again     ESC: quit`

## How a first-time player gets through shot 1

What is on screen when control starts (`g01_shot0_start_green`), with nothing read outside the game:

1. The film has just ended on "END OF FILM. 3 SCENES MISSING." and the reel with three red REMOVED gaps has moved to the top of the screen.
2. The next shot is laid over the picture as a warm ghost that pulses until the first cut. The water the robot can land in is tinted green with a green outline.
3. The robot already has a green ring around it with `C: CUT` under it. The start mark was placed inside the valid area on purpose, so pressing C without moving works. The solver puts the valid area at 67 % of the water the robot can reach.
4. One line under the picture: `Line up with the ghost. Press C to cut.` It stays until the first successful cut.
5. If the player swims into a red spot and presses C, the ring is red with the reason under it (`ROCK IN THE WAY`), and the press gives no failed take: the line changes to `Not here. Move to the green area.` and nothing is lost (`g03_shot0_soft_fail`).

From the second cut on a wrong press costs a take.

The brief-01 play-test failed on "four presses, all refused, no idea why". Here the first press succeeds from the start mark, and a refused press says why and where to go. Whether that is enough for a real newcomer is not known until one plays it.

## What breaks first when this grows to four stages

In the order I expect to hit them:

1. **Level authoring.** A shot is a list of camera-facing rectangles typed into JSON, and the picture is those same boxes. That is fine for six greybox shots. With real art the collision rectangles and the visible rock will drift apart, and the valid-area outline will stop matching what the player sees. Twenty-odd shots need the rectangles kept as the truth and the art dressed around them, plus a way to see a shot without a six-minute rebuild or a two-minute offscreen run (`cutmap.py` only shows the logic).
2. **Film and flashbacks are posed in C++** (`PoseFilm`, `PoseFlashback`). One film and one flashback took a large part of this slice. Three flashbacks and an ending this way will not fit in the time left. They need to be data (key poses in JSON) or cut down to one camera move each.
3. **One 1,700-line game mode** holds the state machine, the spawning, the cut rule, the film and the script hooks. Adding three more abilities to it will get slow and error-prone. A header change costs a 6-minute rebuild.
4. **Performance is unknown.** The ghost capture was the main cost in brief 01 and nothing here contradicts that. Nothing has been measured on the Windows PC or the Iris Xe machine.
5. **Text.** The engine bitmap font is ASCII only and blurry when large. Any real title card or caption needs a proper font asset.
6. **Audio is zero**, and it is one of the seven judged criteria.

## Files

- Game: `Source/MegaJam/MatchCut/` (`MatchCutGameMode.*`, `MatchCutHUD.*`, `MatchCutPlayerController.*`, `MatchCutScript.cpp`, `DeepCutText.h`)
- Layout and looks: `Content/Data/deepcut.json`
- Materials: `Content/DeepCut/` (made by `Tools/make_deepcut_materials.py`)
- Tools: `Tools/build_editor.sh`, `Tools/run_offscreen.sh`, `Tools/package_mac.sh`, `Tools/cutmap.py`, `Tools/play.sh`, `Tools/tests/{slice,lookdev,ghostfog}.txt`

## Play it

```
cd /Users/os/Documents/side-gigs/itch/megajam-2026/MegaJam && Tools/play.sh
```

This opens a 1600x900 window. `Tools/play.sh 1600 900 -MCSkipFilm` skips the film, `-MCLook=B` or `-MCLook=C` picks another look. Keys: WASD or arrows to move, C to cut, F for the lamp after the pickup, R to go back one shot, Esc to quit.
