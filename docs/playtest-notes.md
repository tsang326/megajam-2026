# Playtest notes (user, human play)

## 2026-11-13 evening, stage 1
- Shot 2 THE SHELF: user stuck, did not know what to do. Pressed C and only saw "No tape mark yet. Solve this shot first." Fixes:
  1. The fail line must say what to do (e.g. "Push the plate onto the VENT frame first."), not only that it is unsolved; the shot hint should stay visible.
  2. The plate starts mostly off-screen (bottom-right edge) and is brown on brown: start it fully in frame, give it a contrasting colour / outline / ring.
  3. The VENT frame brackets + label sit on Unit 0's spawn point, so the robot reads as the vent: move the frame or spawn apart.
- Shot 2 again: user could not interact with the plate at all. Cause: plate at y=-262 on the floor, its screen position is under the bottom HUD bar (only an edge shows); pushing needs Unit 0 at floor height (test script swims to y=-244 first) and about the same depth. Fixes: move the plate up/into clear view above the HUD; make push contact forgiving (taller push volume or auto-align height/depth when near); show a "PUSH" prompt/arrow when Unit 0 touches it.
- User did not understand what SHIFT does. Depth (closer/farther) is invisible from the side camera and SPACE/SHIFT read as up/down. Fixes: rebind depth to mouse wheel + Q/Z (E is already grab in stage 2), add a floor shadow/depth indicator under Unit 0, auto-align depth to a nearby pushable thing; require manual depth only at depth marks (with an explicit on-screen cue the first time).
