# Deep Cut: stage 2 report (brief 04, the gripper and the other creature)

Date: 2026-11-13. Engine: Unreal 5.8.4, C++ module `MegaJam`. Everything below was checked with scripted runs that render offscreen. Captures are in `docs/captures/stage2/` (960x540 copies; full-size ones are written to `Saved/DeepCut/`). Nobody has played this build with a keyboard and nobody has heard it. See "Not verified".

## How to run

- Play from the start (opens a window, with sound): `Tools/play.sh`
- Play from stage 2 (opens a window): `Tools/play.sh 1600 900 -MCShot=6`. Everything stage 1 gives is given: headlamp, scene 1, one crew robot.
- Stage 2 only, scripted, no window: `Tools/run_offscreen.sh stage2 Saved/DeepCut/stage2`
- Stages 1 and 2 in one run, no window: `Tools/run_offscreen.sh full12 Saved/DeepCut/full12`
- Packaged Mac build: `Tools/package_mac.sh`, then `Tools/run_packaged.sh full12 Saved/DeepCut/full12_pkg`
- Stage 1 checks as before: `Tools/run_offscreen.sh stage1 Saved/DeepCut/stage1`, `Tools/run_offscreen.sh route_yellow Saved/DeepCut/route_yellow`
- After editing the diver clip in `Tools/make_film_json.py`: `python3 Tools/make_film_json.py` (no rebuild).

## Stage 1 changes (done first)

| Change | Capture (`docs/captures/stage2/`) |
|---|---|
| No gripper in stage 1. `E` is gone from the controls line. Loose things are pushed by swimming into them and come through the cut inside the dashed radius (STAYS / COMES / FITS kept). | `s1c_beside_plate`, `s1i_on_mark_plate_fits` |
| THE WRECK: each mark is labelled with the camera it cuts to; the blue one is the crew signal; the hint says the same. | `s3l_blue_mark_lamp_on_cell_comes` |
| THE DARK: the timed mark holds only while a warm glow is on it. The body that carries the glow is not shown in stage 1, only its light. | `s4d_on_mark_glow_away`, `s4f_on_mark_glow_on_it` |
| `stage1` and `route_yellow` scripts rewritten for pushing; both run to the end card. | |

## What works in stage 2

Five shots (6 to 10). One scripted run (`Content/Data/tests/stage2.txt`) solves all of them with the normal movement code, with one failed take per reason, one catch and one escape.

| Step | Log line | Capture |
|---|---|---|
| Entering stage 2 plays the diver record as Fathom released it: 8.5 s of 17 s. Mask frame, bubbles, torch, the big creature rushing in, signal lost. | `CLIP start "diver" version=official ... plays 8.5s of 17.0s (2 cuts)` | `t1a_official_calm`, `t1b_official_creature_comes`, `t1c_official_creature_close`, `t1d_official_signal_lost`, `t1e_official_end_banner` |
| Shot 6 THE TRENCH: the part. `FEATURE CUT -> BUILT: GRIPPER`, `E grab` joins the controls line, the feature list shows GRIPPER. | `PART picked up in shot 6: GRIPPER built` | `t1g_gripper_built_restore_frame` |
| Failed take: restore not lined up | `RESTORE NG "NOT LINED UP"` | `t1h_FAIL_restore_not_lined_up` |
| Restore-by-cut on the frame where the predator first shows. The whole record plays: the thin dark one hits the diver twice, the big one comes after and pulls the diver away. One line after: "Fathom cut it out." Gap 2 of the reel is filled. | `RESTORE OK`, `CLIP start "diver" version=restored ... plays 17.0s`, `FLASHBACK end: gap 2 filled` | `t1i_restore_lined_up`, `t2a` .. `t2g`, `t2h_scene_restored_fathom_cut_it_out` |
| Failed take: next camera is dark, lamp off | `CUT 6->7 NG "TOO DARK - F: LAMP"` | `t2j_FAIL_too_dark` |
| Shot 7 THE LEDGE (dark): a heavy lid. Grabbing it shows two marks. | `PROP lid grabbed`, `MARKS revealed (gate "grab"): 2 marks` | `t3b_at_heavy_lid`, `t3c_lid_held_two_marks` |
| Failed take: not on a mark | `CUT 7->8 NG "NOT ON A MARK"` | `t3d_FAIL_not_on_mark` |
| Wrong mark: the lid comes but lands away from the vent. The current pushes Unit 0 back. R goes back one shot. | `CUT 7->8 OK mark=1 carried=[ lid ]`, `RETAKE from shot 8 back to shot 7` | `t3e_high_mark_lid_comes`, `t4a_current_blocks`, `t4b_pushed_back_by_current`, `t3f_back_one_shot` |
| Right mark: the lid lands on the VENT frame of the next shot, the current stops. | `CUT 7->8 OK mark=0 carried=[ lid(socket) ]`, `SOCKET vent2 filled by lid` | `t3g_low_mark_lid_fits`, `t4c_lid_on_vent_current_off` |
| Shot 8 THE CURRENT: a crew unit that cannot swim. Grabbing it shows the mark. | `PROP crew2 grabbed` | `t4d_at_crew_unit`, `t4e_crew_held_mark_shows` |
| The predator: sound, red bar and brackets at the frame edge, eyes, then it comes. Waiting = caught: static, SIGNAL LOST, clapper `TAKE 2` with the reason. | `PREDATOR warning`, `PREDATOR attacks`, `PREDATOR caught Unit 0` | `t4f_predator_warning`, `t4g_predator_eyes_at_edge`, `t4h_predator_comes`, `t4i_predator_close`, `t4j_CAUGHT_signal_lost`, `t4j2_CAUGHT_take_2`, `t4k_take_again` |
| Second try: on the mark with the crew unit, cut when it comes. | `PREDATOR escaped by the cut in shot 8` , `CUT 8->9 OK carried=[ crew2 ]` | `t4l_on_mark_with_crew`, `t4m_predator_comes_cut_now`, `t5a_escaped_drift` |
| Shot 9 THE DRIFT: the mark holds only while the landing place in the next shot is clear. The big creature of the next shot is seen through the frame. The predator comes here too, faster. | `PREDATOR escaped by the cut in shot 9`, `CUT 9->10 OK carried=[ crew2 ]` | `t5b_on_mark_place_not_clear`, `t5c_on_mark_place_clear` |
| Shot 10 THE LIFT: crew unit dropped into the lift. End card for stage 2. | `RESCUE crew robot crew2 is in the lift: CREW 2/7`, `END CARD stage=2 scenes=2/3 crew=2/7` | `t6b_crew_over_lift`, `t6c_crew_safe`, `t6d_end_card_stage2` |
| Stage 1, then C on its end card, then stage 2, in one run | `STAGE 2 starts at shot 6` | `s5e_end_card_c_next_stage` |

## The puzzles and how a newcomer learns each one

1. **THE TRENCH - restore (known from stage 1).** The official record has just ended with "Some seconds are missing". The hint points at the part. Picking it up shows the amber MATCH ring with a still of the missing frame inside it; the hint line is the one from stage 1.
2. **THE TRENCH to THE LEDGE - lamp shows the mark (known).** The next camera is dark, so the mark only counts with the lamp on. The solved hint says so. Fail reason: `TOO DARK - F: LAMP`.
3. **THE LEDGE - carry.** The lid has the label `HEAVY - E`. Pushing does not move it. The banner at the part said "E: grab / drop. Now you can carry heavy things." Marks appear when it is grabbed.
4. **THE LEDGE to THE CURRENT - place it in the next frame.** Two marks. The VENT frame of the next shot is drawn in this frame. The lid's ring says COMES at the high mark and FITS at the low one. The wrong one is not a dead end: the current pushes back, the hint says "No lid? R: back one shot."
5. **THE CURRENT - carry a crew unit, first predator.** Grabbing the unit gives "CREW FOUND / It cannot swim. Carry it: E." The predator comes only after that. Numbers below. The hint line turns red: "Something is coming. You cannot fight it. Cut away: C."
6. **THE DRIFT - cut on the moment.** The reason is in the picture: the big creature of the next shot swims through the landing place, and the label "IN THE NEXT SHOT" shows while it is in the way. The ring says `NOT YET - WAIT`. The second predator makes waiting cost something.
7. **THE LIFT - put it in.** The LIFT frame was already drawn in the frame of THE DRIFT. Dropping the unit over it ends the stage.

## Predator timing

It is active only while the shot's marks are showing. Speed of Unit 0: 430 cm/s. Catch distance: about 110 cm.

| Shot | After the marks show | Warning (eyes at the edge, it does not move in) | Chase speed | Measured in the script |
|---|---|---|---|---|
| 8 THE CURRENT (first time) | 5.0 s | 6.0 s | 110 cm/s + 18 cm/s per second | standing still: caught 17.3 s after the marks, 6.3 s of chase from 1119 cm. Escape: cut 7.4 s after the warning began, 774 cm away |
| 9 THE DRIFT | 6.0 s | 4.0 s | 190 cm/s + 18 cm/s per second | cut 1.8 s after the warning began, 1004 cm away. Not measured to a catch; by the numbers about 4.5 s of chase from 1000 cm |

After a catch the take starts again where the shot was entered, what was held is let go where it was, and the predator's clock starts again from zero.

## New English lines (for approval)

HUD and banners:
- `END OF STAGE %d` (was `END OF STAGE 1`)
- `C: next stage     R: play again     ESC: quit`
- `FEATURE CUT -> BUILT: GRIPPER` (same line as HEADLAMP)
- `E: grab / drop. Now you can carry heavy things.`
- `HEAVY - E`
- `It cannot swim. Carry it: E.` (under `CREW FOUND`)
- `CREW SAFE`
- `Something is coming. You cannot fight it. Cut away: C.`
- `CAUGHT - CUT AWAY SOONER`
- `SIGNAL LOST`
- `IN THE NEXT SHOT`
- `FATHOM DYNAMICS  /  DIVE RECORD  /  OFFICIAL RELEASE`
- `OFFICIAL VERSION`
- `END OF RECORD`
- `That is the official record. Some seconds are missing.`
- `RESTORED SECONDS`
- `DIVER CAM  REC`
- Controls line, built from: `WASD swim   SPACE / SHIFT closer / farther`, `   E grab`, `   F lamp`, `   C cut   R back one shot   ESC quit`
- Mark label with two cameras: `CAM 06  THE DOCK`, `CAM 05  THE DARK`

Diver clip (film.json):
- Caption, both versions: `DIVE 12.  DIVER CAM.`
- Caption, official version only: `THE CREATURE ATTACKS THE DIVER.`
- After the restored clip: `Fathom cut it out.`

Shot names and hints (deepcut.json):
- Stage 1, changed: `Push the loose plate onto the VENT frame.` / `Near things come with you through the cut. Stay close to the plate. Then cut.` / `Yellow: the dock. Blue: a crew signal in the dark (F: lamp). Push the power cell along.` / `Dark. F: lamp. Push the cell onto the DOCK frame. The mark only holds while the warm glow is on it.` / `The dock needs the power cell. No cell? R: back one shot.`
- THE TRENCH: `The record is short. A cut part of you is here. Swim to it.` / `The next shot is dark. F: lamp shows its mark.`
- THE LEDGE: `Dark. F: lamp. Something heavy is here. E: grab.` / `Two marks. Hold the lid so it lands on the VENT frame (FITS). Then cut.`
- THE CURRENT: `A crew unit, behind the current. The vent needs the lid. No lid? R: back one shot.` / `Carry the crew unit to the mark. Then cut.`
- THE DRIFT: `The big one drifts over your landing place. Cut when the place is clear.`
- THE LIFT: `Put the crew unit in the lift: E. No unit? R: back one shot.` Socket label: `LIFT`.

## Decisions that differ from the letter of the brief

- The brief says the diver scene fills the "first gap". Stage 1 already fills gap 1 with the Unit 7 scene, so the diver scene fills gap 2 and the count reads `SCENE RESTORED 2/3`.
- The official version plays by itself when stage 2 starts. It is not skippable.
- The crew robot from stage 1 still follows Unit 0 in stage 2 (it can swim). The stage 2 crew unit cannot, and has to be carried.

## Audio

No new sound files. Stand-ins from the 13 sounds: predator warning = `reel_restore` at pitch 0.42; attack = `take_clapper` at pitch 0.55; catch = `cut_splice` at pitch 0.5; official record start = `cam_rec`; the join where seconds were taken out = `cut_splice`.

Request (not made): a predator sting (short, low, from one side) and a diver breathing loop for the head-cam clip. Both clips are silent apart from the stand-ins.

## Not verified

- No human has played it. Whether the first predator feels fair, whether the lid and FITS puzzle is understood without help, and whether THE DRIFT can be read at speed are all untested.
- Sound has not been heard by anyone; runs are made with `-nosound` and checked by log only.
- The diver clip was looked at as stills only, not in motion. The predator in the old-film look is dark on dark: it can be seen in the stills at its two close passes, less well when it is far.
- THE DRIFT predator was never run to a catch.
- Frame times were not measured for stage 2.
- Windows build: not made (not in this brief).

## What I would cut first

1. The second predator (THE DRIFT). The timed mark stands without it, and the brief allows "at most two".
2. The wrong mark in THE LEDGE (keep one mark that fits). It costs a retake and adds the most text.
3. The second pass of the predator in the diver clip (shorten the removed seconds from 5 s to 3 s).
