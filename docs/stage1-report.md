# Deep Cut: stage 1 report (brief 03, tape marks and puzzles)

Date: 2026-11-13. Includes the design change of 2026-11-13 morning (`docs/OUTLINE.md`): Unit 0 is a shelved prototype from the first second, and puzzles are edit puzzles only (no valves). Engine: Unreal 5.8.4, C++ module `MegaJam`. Everything below was checked with scripted runs that render offscreen; captures are in `docs/captures/stage1/` (960x540 copies; full-size ones are written to `Saved/DeepCut/`). Nobody has played this build with a keyboard and nobody has heard it. See "Not verified" at the end.

## How to run

- Play (opens a window, with sound): `Tools/play.sh` (add `-MCSkipFilm` to start in shot 1, `-MCShot=N` to start in shot N).
- Scripted check, editor binary, no window: `Tools/run_offscreen.sh stage1 Saved/DeepCut/stage1`
- Scripted check, packaged Mac build: `Tools/package_mac.sh`, then `Tools/run_packaged.sh stage1 Saved/DeepCut/stage1_pkg`
- One shot only, as a quick preview: `Tools/preview_shot.sh N [solved]`
- Map of all shots with marks, props and checks, without the engine: `python3 Tools/shotmap.py`

## What works

One scripted run (`Content/Data/tests/stage1.txt`) goes from the design-file card to the end card. It solves every puzzle with the normal movement code (the script swims, it does not teleport), and makes one failed take of each kind on the way. Captures: `docs/captures/stage1/<name>.jpg`.

| Step | Log line | Capture |
|---|---|---|
| Design-file card: UNIT 0 - PROTOTYPE, SHELVED 02:00 AM, three features struck out | | `f00_design_card` |
| Fathom film with three removed gaps (10.4 s) | `FILM start length=10.4s` | `f01_launch` .. `f06_end` |
| Shot 1: mark is there from the start; C off the mark gives a hint, not a failed take | `CUT 0->1 NG "NOT ON A MARK" first=1` | `s0a_mark_from_start`, `s0b_cut_off_mark_soft` |
| Cut 1 to 2 (6 s after control starts, in the script) | `CUT 0->1 OK ... err=0.00px` | `s0c_on_mark`, `s1a_after_cut_no_marks` |
| Shot 2: no mark before the plate is picked up | `CUT 1->2 NG "NO MARK YET"` | `s1b_cut_before_solved` |
| Plate picked up, marks appear | `MARKS revealed in shot 1 (gate "grab")` | `s1c_at_plate_prompt`, `s1d_marks_reveal` |
| Failed take: not on a mark | `CUT 1->2 NG "NOT ON A MARK"` | `s1e_FAIL_not_on_mark` |
| Cut with the plate held but not over the vent: no marks in the next shot, the current pushes back | | `s1f_plate_held_comes_not_fits`, `s2a_vent_plate_missed`, `s2b_current_pushes_back` |
| R: back one shot | `RETAKE from shot 2 back to shot 1` | `s1g_back_one_shot` |
| Plate dropped where the VENT frame is, then cut: current stops, marks appear | `SOCKET vent in shot 2 filled by plate`, `MARKS revealed (gate "socket:vent")` | `s1h_plate_dropped_on_vent_frame`, `s1i_on_mark_plate_fits`, `s2c_vent_current_stopped`, `s2d_marks_shown` |
| Failed take: on the mark but too close | `CUT 2->3 NG "TOO CLOSE - SHIFT"` | `s2g_on_mark_too_close`, `s2h_FAIL_too_close` |
| Camera rides its rail; the mark stays where it is on screen | | `s2e_rail_midway` |
| Cut 3 to 4 at the right depth | `CUT 2->3 OK` | `s2i_on_mark_depth_ok`, `s3a_wreck` |
| Shot 4: headlamp part, feature line rebuilt | `PART picked up` | `s3b_part_found_restore_frame` |
| Failed take: restore not lined up | `RESTORE NG "NOT LINED UP" off=262px` | `s3c_FAIL_restore_not_lined_up` |
| Restore-by-cut, removed scene plays, gap 1 filled, two marks appear | `RESTORE OK off=5px`, `FLASHBACK start/end`, `MARKS revealed (gate "restore"): 2 exits` | `s3d_restore_lined_up`, `s3e_flashback_1`, `s3f_flashback_2`, `s3g_scene_restored_marks_reveal`, `s3h_two_cameras_lamp_off` |
| Failed take: blue mark with the lamp off | `CUT 3->4 NG "TOO DARK - F: LAMP"` | `s3i_on_dark_mark_lamp_off`, `s3j_FAIL_too_dark` |
| Power cell picked up, lamp on, cut on blue | `CUT 3->4 OK tape=blue carried=[ cell ]` | `s3k_at_cell_prompt`, `s3l_blue_mark_lamp_on_cell_comes` |
| Shot 5 (dark): crew robot freed | `RESCUE ... CREW 1/7` | `s4a_dark_lamp_on`, `s4b_dark_lamp_off`, `s4c_crew_found` |
| Failed take: on the mark while it is faint | `CUT 4->5 NG "NOT YET - WAIT"` | `s4d_on_mark_closed`, `s4e_FAIL_not_yet` |
| Cut 5 to 6 while the mark is bright, with the cell | `CUT 4->5 OK carried=[ cell ]` | `s4f_on_mark_open`, `s5a_dock` |
| Cell in the dock, end card | `SOCKET dock filled by cell`, `END CARD scenes=1/3 crew=1/7` | `s5b_at_dock_with_cell`, `s5c_power_on`, `s5d_end_card` |

Every successful cut lands on the same screen position (`err=0.00px`). The run has no `NOT REACHED` except the intended one against the current.

A second script (`route_yellow`, output in `Saved/DeepCut/route_yellow/`) starts in the wreck with the part given and takes the yellow camera straight to the dock: `CUT 3->5 OK tape=yellow`, R back (`RETAKE from shot 5 back to shot 3`), cell picked up, cut again, `SOCKET dock filled`, `END CARD scenes=1/3 crew=0/7`. One thing it showed: the cell lies about 215 cm from the yellow mark, inside the carry radius (280 cm), so it comes through the yellow cut without being picked up (`carried=[ cell ]` on the first cut). The case "in the dock with no cell" is therefore not tested, and the yellow route asks less of the player than intended. Moving the cell farther from the yellow mark would fix it; not done.

## The stage

Six shots. Blocks used: carry across the cut (twice), lamp-reveal (twice), cut on the right moment (once), camera choice (once). No valves, switches or doors.

| # | Name | What makes the marks appear | Puzzle | How a newcomer learns it |
|---|---|---|---|---|
| 1 | DROP ZONE | nothing: the mark is there | Tutorial. One yellow X about 130 px from where the robot arrives, one line: "Swim onto the tape mark. Press C to cut." | A C press off the mark never costs a take on the first cut: it shows a hint line. The mark ignores depth, so only left/right/up/down matter. |
| 2 | THE SHELF | pick up the plate (E) | B: carry across the cut. A dashed circle around the robot shows what comes along. Each loose thing has a ring: STAYS (too far), COMES, FITS (it would land in a frame of the next shot). The VENT frame of the next shot is drawn here as brackets. | Cutting with the plate held works, but the plate lands beside the vent: no marks, the current pushes the robot back, the line says "No plate on the vent? R: back one shot." R returns to this shot as it was on entering. |
| 3 | THE VENT | the plate sits on the vent | Pays off puzzle 2: the current stops and the mark appears. Then the first mark with depth: the ring must be the same size as the mark. The camera can ride a rail to the right; nothing is needed there now. | The landing ring turns red with "TOO CLOSE - SHIFT" or "TOO FAR - SPACE" before C is pressed. |
| 4 | THE WRECK | restore the removed scene | The headlamp part: "FEATURE CUT -> BUILT: HEADLAMP", the HEADLAMP line in the top bar loses its strike. Restore-by-cut with its own frame ("MATCH"), no tape. After the removed scene two marks appear: yellow (dock) and blue (crew signal in the dark). A: choose the camera. The power cell lies here: B again, over two cuts on the blue route. | The blue mark is hidden without the lamp: "TOO DARK - F: LAMP". The hint names both cameras. A "CREW SIGNAL" beacon blinks in the ghost of the blue camera. |
| 5 | THE DARK | nothing, but only the lamp shows the mark | Lamp-reveal and cut on the right moment: the mark is bright for 2.6 s out of every 5 s. While it is faint an arc around it runs down to the next opening. The crew robot is on the way and is freed by touching it; it is not needed for the cut. | Ring text "NOT YET - WAIT", hint line. |
| 6 | THE DOCK | (last shot) | The cell from shot 4 must be here. Dropping it in the DOCK frame ends the stage. | Without the cell: "No cell? R: back one shot." |

The timed mark has no story reason yet: nothing in the dock shot drifts or turns to explain why the mark comes and goes. It is the mechanic only. A drifting object in the next shot that covers the landing spot would make it an edit puzzle in the sense of the outline.

## Mark radius and depth rule

- A cut works when the robot's centre is within **0.085 of the picture height** of a visible mark, measured on screen (92 px at 1080p). Reason: at the working depth of 10 to 13 m the robot's landing ring is about 60 to 80 px in radius, so "the robot covers the X" is what the number means; and the float brakes hard when no key is held, so a keyboard player should be able to stop inside it (not tried by hand). The two marks that share a shot (the wreck) are more than 900 px apart, so the nearest-mark rule never has to guess.
- Depth: the first two marks ignore depth. From shot 3 on the robot must be within **160 cm** of the mark's depth. The mark is drawn at the size the robot's ring has at the right depth, so "same size" is the rule the player sees; the ring text says which key to hold.
- A mark belongs to the next shot's camera: it is the place where that shot has free water. A landing inside rock is refused ("ROCK IN THE WAY"); `Tools/shotmap.py` checks that no authored mark is blocked.
- A mark into or out of a dark shot is only shown, and only usable, inside the lamp circle.
- Sockets use the same idea with radius 0.075: a carried or nearby loose thing whose screen position falls on a frame of the next shot is put into it by the cut.

## Movement and collision

- Swim volume: x/y on screen plus depth (Space closer, Shift farther). Acceleration 1500 cm/s² (depth 1200), top speed 430 cm/s, drag 2.4 (2.5 times stronger with no key held).
- Collision approach: the robot is a sphere (radius 36 cm) tested against oriented boxes. Each box is read from the transform of the mesh actor that is drawn, so what blocks is what is seen; there is no second hand-made collision list and no engine physics body. Penetration is pushed out along the shortest axis and the remaining velocity slides along the face. The same test answers "is the landing blocked" for a cut, so the ring and the movement can never disagree.
- Known limit: boxes only. A slanted rock is an oriented box; anything rounder would need more boxes.

## Camera rail rule

A shot may have a rail: the camera trucks sideways between two ends and follows the robot slowly once it is more than 30 % of the half-width from the centre of the picture. It does not drift back by itself. The cut uses the camera pose at the moment of the cut: the robot's screen position is measured with the truck as it is then. The next shot always starts at its own entry pose, and its marks are drawn from that entry pose, so marks stay fixed on screen while the camera rides. Only shot 3 has a rail in stage 1, and after the valves were removed nothing on it is needed. A mark placed outside the middle zone would only be reachable at the end of the rail, so marks in a rail shot must sit inside it; `Tools/shotmap.py` does not check this yet.

## Reveal

When a shot's marks appear: the `cam_rec` sound plays (reused; no new file), white brackets close on each mark for 1.2 s, "MARKS SET" shows, and the ghost of the next shot goes from 60 % to full. The focus pull from the brief is not built: the picture has no depth of field in the chosen look, and adding it for 1.2 s was not worth a new post-process pass. It is the first thing to add if the reveal feels weak in play.

## Audio

13 files, all from `../audio/picked/`, imported to `/Game/DeepCut/Audio`, listed in `ASSETS.md`. `Content/Data/audio.json` holds the mixer: four buses (music, film, amb, sfx) and one gain per file. Every play is logged (`AUDIO play <name> bus=<bus> vol=<gain>`), also when the run has no audio device.

| Event | File |
|---|---|
| Opening film | `mus_opening` + `projector` (loops) |
| Play | `amb_underwater` + `mus_explore` (loops) |
| Removed scene | `mus_flashback` + `projector`; gameplay music ducked to 0.18 (-14.9 dB) and back |
| Moving | `thruster` every 1.1 s while a key is held, pitch random within ±5 % |
| Good cut | `cut_splice` |
| TAKE n card | `take_clapper` |
| Marks appear | `cam_rec` (reused) |
| Scene restored | `reel_restore` |
| Headlamp found, power on | `part_pickup` |
| Crew found | `reel_restore` at pitch 1.2 |
| Lamp key | `lamp_on` |
| Grab, drop, plate on the vent | `plate_lift` at different pitches |

Loudness. Source loudness is ffmpeg `ebur128` integrated LUFS of each file; the level in the game is that plus the gain.

| File | Source LUFS | Gain | In game | Against the bed |
|---|---|---|---|---|
| amb_underwater (bed) | -34.7 | 2.00 | -28.7 | 0 |
| mus_explore | -23.7 | 0.68 | -27.1 | +1.6 |
| mus_opening | -22.7 | 0.68 | -26.1 | +2.6 |
| mus_flashback | -22.1 | 0.64 | -26.0 | +2.7 |
| projector | -23.1 | 0.45 | -30.0 | -1.3 |
| cam_rec | -23.6 | 0.95 | -24.0 | +4.7 |
| cut_splice | -15.6 | 0.38 | -24.0 | +4.7 |
| take_clapper | -19.1 | 0.57 | -24.0 | +4.7 |
| part_pickup | -32.8 | 2.75 | -24.0 | +4.7 |
| reel_restore | -27.2 | 1.45 | -24.0 | +4.7 |
| plate_lift | -13.9 | 0.31 | -24.1 | +4.6 |
| thruster | -12.8 | 0.20 | -26.8 | +1.9 |
| lamp_on | -38.1 | 3.20 | -28.0 | +0.7 |

No effect is more than 4.7 dB over the bed (limit in the brief: about 6). These are computed numbers, not a measurement of the game's output. Integrated loudness of a half-second click also says little about its peak, and the four files with a gain above 1 (`amb_underwater`, `part_pickup`, `lamp_on`, `reel_restore`) could clip if their peaks are high. This needs ears.

## Text

HUD text is drawn with two OFL fonts loaded at run time from `Content/Data/Fonts` (IBM Plex Mono for the HUD, Big Shoulders Bold for titles), at the size it is shown, so it is sharp at any resolution. If the files are missing the engine font is used.

## Opening and feature list

- Card (3.2 s): `FATHOM DYNAMICS / DESIGN FILE`, `UNIT 0 - PROTOTYPE`, `SHELVED 02:00 AM`, `reason: over budget`, then HEADLAMP, GRIPPER, MULTI-CAM struck out with `CUT` beside each.
- Film: 10.4 s plus a 1.3 s end line, 14.9 s in all. The clips are the old ones played about 2.2 times faster; captions are on screen for about 2 s each. On a first viewing it cannot be skipped; later any key skips it.
- Top bar, under SCENE / TAKE: `UNIT 0` and the three features, struck out in red. HEADLAMP turns amber without the strike when the part is picked up.
- The card and the banner use ASCII (`-`, `->`) because the two fonts were not checked for the dot and arrow glyphs.

## Fixes from the slice

- Ghost too bright over the dark shot: the ghost is drawn at 18 % there (it was 35 % in the first build of this stage, still too strong in the capture), and at 60 % before a gate opens.
- CHIMNEY black wall: that shot no longer exists. All six shots are new and wider (each about 12 m across with 3 to 5 m of usable depth); none has a black wall in the captures.
- Blurry text: see Text.

## Level authoring

- Shots are data: `Content/Data/deepcut.json` (camera, rail, boxes, gate, marks with tape colour and target, props, sockets, currents, hint lines). Adding a shot is a JSON edit, no C++.
- Film and removed scenes are data: `Content/Data/film.json`, key poses every 0.25 s, written by `Tools/make_film_json.py`.
- Quick look at one shot: `Tools/preview_shot.sh N [solved]` renders it offscreen in about three minutes on this machine (almost all of it engine start). `Tools/shotmap.py` draws every shot as SVG in a second and checks marks against rock and sockets against props.
- The game mode is split by job: `DCLayout` (data), `DCWorld` (actors), `DCCutRule` (movement, collision, rail, cut test), `DCPuzzles` (gates, props, sockets, currents, history), `DCFilm`, `DCAudio`, `MatchCutScript`, `MatchCutHUD`. It is still one class; the split is by file.

## Frame times

Average frame time per shot at 1920x1080, look A, offscreen, from the `FRAMETIME` log lines. Every run was started with `taskpolicy -b` (background priority, which macOS throttles hard) while the machine was doing other work, so these are upper bounds and not what a player gets. A foreground run on 12 Nov, before this work, logged 13-14 ms in every shot; that has not been measured again with the new shots.

| Shot | Packaged, run 1 | Packaged, run 2 | Editor `-game` |
|---|---|---|---|
| 0 Drop zone | 38 ms | 48 ms | 43 ms |
| 1 The shelf | 31-41 ms | 75-81 ms | 63-69 ms |
| 2 The chimney | 39-47 ms | 41-66 ms | 46 ms |
| 3 The wreck (lamp) | 29 ms | 34 ms | 88 ms |
| 4 The dark (lamp) | 47 ms | 50 ms | 39 ms |
| 5 Dock, end card | 65 ms | 48 ms | 31 ms |

The two packaged runs differ by a factor of two in the same shot, so the load on the machine decides these numbers more than the scene does. `max` is 400 ms in every line; I did not look into where those long frames fall. Not measured: a foreground run at normal priority, and anything on Windows.

## Packaging

- `Tools/package_mac.sh` builds a Development package into `Saved/Package/Mac/MegaJam.app` (cook, stage, pak; about 25 minutes at background priority).
- `Tools/run_packaged.sh stage1 Saved/DeepCut/stage1_pkg` plays stage 1 in the packaged build, offscreen. Result on 13 Nov: `Layout loaded` (6 shots, 2 props, 2 sockets), `Film loaded` (10.4 s), `AUDIO init: 13 of 13 sounds loaded`, `HUD fonts: mono=1 title=1`, every cut and every failed take the same as in the editor run, `END CARD scenes=1/3 crew=1/7`, no `NOT REACHED`, 79 audio events, 50 captures. Seven of the captures are in `docs/captures/stage1_pkg/`.
- The packaged Mac app is sandboxed (`com.apple.security.app-sandbox`, added by the engine's packaging). It reads only its own bundle and writes only inside `~/Library/Containers/com.YourCompany.MegaJam/Data/`. A script given by path outside the bundle is not read, and `-MCOut` or `-abslog` pointing outside the container write nothing, without an error. The runner therefore uses a script name (staged in `Content/Data/tests`), lets the captures go to the container and copies them out, and takes the log from stdout. The save file `intro_seen.txt` also lives in the container.
- The bundle id is still the template's `com.YourCompany.MegaJam`. It only matters for a Mac release, which the jam does not ask for.
- Windows: not built. `docs/packaging-windows.md` has the steps and the checks; none of them has been run.

## New player-facing lines (all need approval)

In code (`Source/MegaJam/MatchCut/DeepCutText.h`):

- Card: `FATHOM DYNAMICS  /  DESIGN FILE`, `UNIT 0 - PROTOTYPE`, `SHELVED 02:00 AM`, `reason: over budget`, `FEATURES`, `CUT`
- Feature list (card and top bar): `UNIT 0`, `HEADLAMP`, `GRIPPER`, `MULTI-CAM`
- `FEATURE CUT -> BUILT: HEADLAMP` (replaces `HEADLAMP FOUND`)
- `Not here. Swim onto a tape mark.`
- `No tape mark yet. Solve this shot first.`
- `NO MARK YET`
- `NOT ON A MARK`
- `TOO CLOSE - SHIFT`
- `TOO FAR - SPACE`
- `NOT YET - WAIT`
- `CAM 02` (label on a mark when a shot has two cameras; the number of the camera it leads to)
- `E: GRAB` / `E: DROP`
- `STAYS` / `COMES` / `FITS`
- `CURRENT STOPPED`
- `POWER ON`
- `MARKS SET`
- `WASD swim   SPACE / SHIFT closer / farther   E use   C cut   R back one shot   ESC quit` (and the same with `F lamp` once the lamp is built)
- `END OF STAGE 1`

In `Content/Data/deepcut.json`:

- Shot names: `DROP ZONE`, `THE SHELF`, `THE VENT`, `THE WRECK`, `THE DARK`, `THE DOCK`
- Frame labels: `VENT`, `DOCK`
- `Swim onto the tape mark. Press C to cut.`
- `Pick up the loose plate. Swim to it and press E.`
- `Near things come with you. Drop the plate on the VENT frame. Then cut.`
- `The current is too strong. No plate on the vent? R: back one shot.`
- `Make your ring the same size as the mark. SPACE closer, SHIFT farther.`
- `Something is shining in the wreck. Swim to it.`
- `Two cameras. Yellow: the dock. Blue: a crew signal in the dark (F: lamp).`
- `Dark. F: lamp shows the tape mark. It comes and goes: cut when it is bright.`
- `Put the power cell in the dock. No cell? R: back one shot.`

Film captions in `Content/Data/film.json` are unchanged from the slice. `E: TURN` is still in the header but nothing shows it now (no valves).

Unchanged lines from the slice are in the same header without the NEW mark.

## What to cut first

1. The camera rail (shot 3 no longer needs it).
2. The timed mark in THE DARK (set it to always open): it has no story reason yet.
3. The yellow shortcut from the wreck (keep only the blue camera; the camera choice would then be gone from stage 1).
4. The depth rule on marks (set every mark to ignore depth; removes one failure type and the Space/Shift lesson).
5. The second carry (cell to the dock): end the stage at the crew robot.

Keep to the last: tape marks, the plate over the vent, restore-by-cut, the card and feature list, audio.

## Not verified

- No human has played it. The keyboard path (key bindings, Space/Shift, E) has only been driven through the same functions by the script.
- Sound has never been heard. Runs use `-nosound`; only the play calls and gains are logged. Looping, the duck during the removed scene and the thruster pitch are code that has not been listened to.
- Whether a newcomer understands STAYS / COMES / FITS and the dashed circle without help, and whether the first cut takes a person under 20 s (the script needs 6 s; the mark is about 130 px from the arrival point).
- The dark shot is crowded when the cell is held on the mark: prompt, ring text, lamp circle and countdown arc overlap (`s4d_on_mark_closed`).
- Whether 92 px is comfortable by hand. The script stops within 22 cm of its target, a player will not.
- Frame times at normal priority. Every number in this report comes from a throttled background run.
- The packaged build has only been run by script, offscreen. Nobody has opened `MegaJam.app` in a window.
- The Windows build (see `docs/packaging-windows.md`).
- The focus pull on reveal is not built.
