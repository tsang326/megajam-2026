# Brief 07 report: the deep-sea documentary

Working notes, one section per checkpoint. The final summary is at the end once checkpoint 4 is done.

## Checkpoints 1 and 2 (2026-11-14, ~05:15)

What runs (editor build, offscreen script runs, captures looked at):

- Camera mode: hold right mouse (or C toggles). Viewfinder with thirds, a cross, lock brackets on the subject, focal length and focus distance, slower swimming, wheel (or 1 / 2) zooms. Depth of field while the camera is up.
- Recording: left mouse (or E) starts a 3 second take with a meter and a red frame.
- Scoring: each take gets 0 to 3 stars and one named reason (`Nothing in frame`, `Too far`, `Subject lost`, `Subject cut off`, `Missed the moment`, `Too dark. It needs a lamp.`, `Too shaky`, `Off centre`, `A perfect take`). One star or more crosses the shot off. Up to three takes are kept per shot.
- World, built in code from the `scene:*` models only: arch, wreck, station, vents, a roofed dark trench, a reef, a clearing with a light shaft, rim rocks. The old stylised props are not used.
- Six subjects, each a function of the clock so a take can be played again: manta circling over the wreck, a shoal of 130 instanced fish that splits when you swim through it, the anglerfish in the trench (lit only by the headlamp), seven jellyfish rising in the clearing, the giant turtle passing overhead every 120 s, the crewmate (a second diver) who waves when you are near.
- Headlamp lies on the wreck; without it the anglerfish take is refused as too dark. A long lens lies on the reef.
- Ambient life from the new creature models (squid, nautilus, crab, lantern fish, isopod, whale far out, shark, eel): 30 creatures on loops.

Test scripts: `Content/Data/tests/d1_camera.txt`, `d2_dbg.txt`, `d3_film.txt`.

Captures: `docs/captures/brief07/cp2_shooting.jpg`, `cp3_cutting_room_title.jpg`, `cp3_film.jpg`.

Known problems at this point:

- Frame time 48 ms average in the full run with the 30 ambient creatures (30 to 35 ms before them). To be cut down next.
- With frames that slow the test script ran ahead of the game: the jellyfish take ended after the script had moved on, so that run shows 5 of 6 shots and no rating card. The cutting room, the title card, film playback and the credits are in the captures.
- Hard rectangular shadows on the sea floor, weak light shafts, flat floor: checkpoint 4.
- Mouse and gamepad feel have not been tried by hand.

## Checkpoint 3 (2026-11-14, ~06:40)

`Content/Data/tests/d3_film.txt` runs the whole game offscreen in the editor build: start, two takes of the crewmate, headlamp picked up on the wreck, manta, jellyfish, anglerfish, shoal, turtle, ENTER to the cutting room, choosing takes, ENTER to play the film (title card, six takes played back from the stored camera path, credits), rating card, back. Log of the last run: 6 of 6 shots, 17 of 18 stars, states 1 (cutting room), 2 (film), 3 (rating), 1, 0 reached in order.

- The two problems of the earlier run (lost jellyfish take, no rating card) were the test script running ahead of slow frames. Fixed in the script clock, not by loosening the game.
- New line: `Too few of them in frame` (a take that holds only part of the shoal or the jellyfish).
- Jellyfish shot needs two in frame (was three); their column is shorter.
- Visible light columns in the water (additive), one upright in the clearing.
- Floor dapple is off by default (`-MCDapple=9000` shows the last version).
- Frame time, `d3_film` alone, editor build, background priority: median 28 ms, 90th percentile 66 ms, average 42 ms (the average includes 21 screenshots). With the dapple on the median was 35 ms.
- 52 ambient creatures from 15 models.

Captures: `docs/captures/brief07/cp3_shooting.jpg`, `cp3_cut_film_rating.jpg`.

Still open: packaged build and a packaged run of `d3_film`; jellyfish read dark and glossy; the columns of the clearing show their edges from close by; the sea floor is flat and plain; the long lens and the vent plume have not been looked at; nothing has been played by hand.

## Checkpoint 4, first part (2026-11-14, ~10:15)

- The first packaged build failed to compile (an editor-only call in `DC3D.cpp`). Fixed; the package is being made again.
- World dressing and ambient life from all models that have arrived: 53 art ids in `Content/Data/art.json`, 67 ambient creatures. `d4_look.txt` takes ten views of the world.
- Caustics light function made and switched off: far too slow on this Mac (median 152 ms against 28 ms) and not visible.
- Softer shaft in the clearing, lit jellyfish, film music under the finished film.
- `d3_film` alone in the editor build: 6 of 6 shots, 18 of 18 stars, median 25 ms, p90 48 ms.

Captures: `docs/captures/brief07/cp4_look_a.jpg`, `cp4_look_b.jpg`.

## Summary so far (2026-11-14, ~10:50)

The game is playable from the start of the dive to the rating card, and the packaged Mac build runs the same scripted run: `Tools/run_packaged.sh d3_film Saved/DeepCut/d3_pkg` gave 6 of 6 shots, 18 of 18 stars, cutting room, film, rating card, back; median frame 23 ms, 90th percentile 47 ms, average 31.6 ms over 3570 frames (background priority, the Mac in use). Capture: `docs/captures/brief07/cp4_packaged.jpg`. The packaged app is `Saved/Package/Mac/MegaJam.app` (1.2 GB).

### Play

    cd /Users/os/Documents/side-gigs/itch/megajam-2026/MegaJam
    Tools/play.sh                 # opens a 1600x900 window
    Tools/play.sh 1920 1080
    Tools/play.sh 1600 900 -MCLife=0.5      # half the ambient life
    Tools/play.sh 1600 900 -MCRays=0.6      # fainter light columns

Keys: mouse look, W A S D swim, SPACE up, X or Q down, SHIFT boost. Hold RIGHT MOUSE (or press C) to raise the camera; WHEEL (or 1 / 2) zooms; LEFT MOUSE (or E) records a 3 second take; F switches the headlamp once it is found on top of the wreck; ENTER ends the shoot when four shots have a take; in the cutting room A / D choose the shot, W / S the take, ENTER plays the film, BACKSPACE goes back to shooting. Gamepad: LB camera, RB record.

### Every new English line

Shot list: `The manta glides over the wreck`, `The shoal splits around you`, `The anglerfish's light in the trench`, `Jellyfish rise in the light shaft`, `The giant passes overhead`, `Your crewmate waves`.
Film captions: `THE MANTA`, `THE SHOAL`, `THE ANGLERFISH`, `THE JELLYFISH`, `THE GIANT`, `THE CREW`.
HUD: `SHOT LIST`, `STBY`, `REC`, `DEPTH`, `HDG`, `FOCUS`, `TAKE n / m`, `n / m STARS`, `NO TAKE`, `NO GOOD`.
Teaching lines: `Hold RIGHT MOUSE to raise the camera     (or press C)`, `LEFT MOUSE record a take     WHEEL zoom     (or E, and 1 / 2)`, `ENTER  end the shoot and cut your film`, `Headlamp fitted.  F switches it.`, `Long lens fitted.  The zoom goes further.`, `Something big is passing overhead. Look up.`, `That is every shot.  ENTER: cut your film.`
Verdicts: `Nothing in frame`, `Too few of them in frame`, `Too far`, `Subject lost`, `Subject cut off`, `Missed the moment`, `Too dark. It needs a lamp.`, `Too shaky`, `Off centre`, `A perfect take`.
Cutting room and film: `THE CUTTING ROOM`, `A / D  shot     W / S  take     ENTER  play the film     BACKSPACE  keep shooting`, `DEEP CUT`, `a documentary from 212 metres down`, `Camera, direction and cut:  you`, `Filmed on location.  No animal was asked to do it twice.`, `ENTER  back to the cutting room     ESC  quit`.
Rating: `STRAIGHT TO THE ARCHIVE`, `A LATE-NIGHT SLOT`, `PRIME TIME`, `FESTIVAL OPENER`.

### Frame times (all at background priority with the Mac in use, so upper bounds)

| run | median | p90 | average |
| --- | --- | --- | --- |
| `d3_film`, editor, 67 ambient creatures, full dressing | 25 ms | 48 ms | 33.8 ms |
| `d3_film`, packaged, same content | 23 ms | 47 ms | 31.6 ms |
| `d4_look`, editor, caustics light function on (first run with it) | 152 ms | 200 ms | 186.7 ms |
| `d3_film`, editor, 9000-piece floor dapple on | 35 ms | not kept | not kept |

Lumen does not run on this Mac (the engine falls back to SM5), so there is no with-Lumen number; the game is lit without it.

### Not verified

- Nothing has been played by hand: mouse feel in camera mode, the wheel, the gamepad buttons, how hard the shots are for a person. The scripted run places the camera with test commands (`near`, `track`), so it proves the systems, not the difficulty.
- The long lens pickup on the reef: placed and coded, never picked up in a test.
- "End the shoot" with four or five shots (a film with a `NO TAKE` gap): the code path exists, not run.
- The film music under the finished film: wired, not listened to (test runs are without sound).
- The creature models' facing: checked for the turtle only; `-MCFront=` turns all of them.
- The caustics slowdown was measured in the first run after the material was made, so part of it may have been shader compiling. It was switched off without a second measurement.
- Whether any scattered piece (a cliff stands near the vents) blocks a route a player would take. The six subjects are reachable in the test.
- No Windows build.

### Known weak points

- The sea floor is one flat plain colour.
- The vents have no plume.
- The jellyfish bells are glossy close up.
- `m1_swim.txt` still describes the brief 06 ring layout and will not pass.
- Models still to come (per the coordinator's list) join in on their own once imported: hatchet shoals and the rest are already in the tables.

### What to cut first if time runs short

1. The long lens (it is optional and untested).
2. The eel and the far whale and oarfish (ambient only).
3. Film playback as moving takes: fall back to a still frame per take.

## Crash on launch, guidance (2026-11-14, ~11:25)

### The crash
- Cause: a race in the engine's memory tracker (LLM) at start-up in Development builds; it only shows when the app is launched the normal way (`open`, double-click).
- Fix: the release is a Shipping build, which has no memory tracker. App: `Saved/Package/Mac/MegaJam-Mac-Shipping.app`, bundle id `com.tsangfx.DeepCut`.
- Verified by three `open` launches offscreen (one ran the whole `d3_film`: 6/6 shots, 18/18 stars, median 21 ms, p90 26 ms). Not verified: a launch with a visible window, by hand.

### Guidance: every new English line
- Card: "CURRENT SHOT  n / 6", the shot's caption, and one of:
  - "Find the manta. It circles over the wreck."
  - "Find the shoal over the reef. Swim into it fast, then film it opening."
  - "Film the anglerfish at the end of the trench, with your lamp on it."
  - "Film the jellyfish rising in the shaft of light."
  - "The giant crosses high overhead every two minutes. Get under it and film upwards."
  - "Film your crewmate waving. Swim up in front of them."
  - "The trench is dark. First fetch the headlamp: it lies on the wreck."
- Marker labels: the caption or "HEADLAMP", then the distance, e.g. "THE SHOAL  40 m".
- Director: "DIRECTOR:  Rolling. Start easy: film your crewmate, just ahead." / "DIRECTOR:  Good take. Next: the manta, over the wreck." / "... Next: the shoal over the reef." / "... Last one: the anglerfish, down in the trench." / "... Next: the jellyfish in the light shaft." / "... Next: the giant. Watch the water above you." / "... Next: your crewmate." / "DIRECTOR:  We have that one. A better take replaces it." / "DIRECTOR:  That's a wrap. Press ENTER to cut your film."
- First shot: "Swim up to your crewmate     W A S D swim     MOUSE look" / "Hold RIGHT MOUSE to raise the camera     (or press C)" / "Frame your crewmate" / "LEFT MOUSE: record     Hold the frame for three seconds     (or E)" / "WHEEL zooms the lens     (or 1 / 2)".
- Tips: "TIP: swim closer, or zoom in with the WHEEL." / "TIP: back off or zoom out until all of it fits." / "TIP: wait until the manta is over the wreck." / "TIP: swim into the shoal fast, then film it opening." / "TIP: fetch the headlamp from the wreck, then switch it on with F." / "TIP: film them while they rise in the light." / "TIP: get under the giant and tilt the camera up." / "TIP: come close, in front of them, until they wave." / "TIP: move the camera slowly while it records." / "TIP: keep the subject in the middle, or on a third line." / "TIP: keep it in the frame for the whole three seconds." / "TIP: the brackets show what the lens has found." / "TIP: get more of them into the picture."
- The ending keeps the ENTER key (there is no swim back to the dock): the wrap line stays on screen until ENTER.
