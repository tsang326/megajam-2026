# Dev log – 2026 Epic AI MegaJam ("Director's Cut")

Solo developer + AI agents (Claude Code). Times are Vietnam time (UTC+7).

## 2026-11-12

- **02:10** Theme revealed: *Director's Cut*.
- **12:00** Jam building window opens. Morning and early afternoon spent on the concept, no code yet.
  - Many AI brainstorming runs on the theme email alone kept landing on the same few ideas (timeline-cutting, "cut content", directing AI actors). Used that as a map of what *not* to make by default.
  - Candidates weighed: a screen-position "match cut" platformer, a horror film whose script you rewrite, and my own reading of the theme: an unfinished character design, shelved before it was ever finished, on its way to becoming a real character in another game.
  - Decision: try the hybrid first – the unfinished character travels into the other game *by match cuts*. If the match-cut prototype does not hold up by the first play-test, fall back to a keep/cut design mechanic for the same character.
- **~15:20** New Unreal Engine 5.8 project created from Epic's Third Person C++ template (renamed module `MegaJam`); git repo initialised.
- **~15:40** Editor target compiles from the command line (headless, ~9 min). Template Blueprints failed to find their renamed C++ parents; fixed with `[CoreRedirects]` class redirects. Headless Python check now loads the game mode, character and level with 0 load errors.
- **~16:15–18:50** Match-cut movement prototype (greybox, brief 01). Five shots with fixed cameras are spawned from a JSON layout; CUT projects the character's feet to a fixed 16:9 frame and deprojects the same point from the next camera onto a tagged landing surface; the next shot is shown as a faint ghost.
  - Checked with scripted offscreen runs only: every shot pair has a logged failed and successful cut, the whole sequence completes by walking, and the cut lands the same at 1920x1080, 1280x720, 2560x1080 and 1024x768 (letterbox/pillarbox bars confirmed in captures).
  - Found and fixed: a moving platform could slide out from under the character on a long frame right after a cut; falling respawned past the obstacle.
  - Found, not fixed: the ghost capture roughly triples frame time in the test runs; designing shots by eye was not workable, so a small offline solver (`Tools/cutmap.py`) was written; a packaged build would not ship the JSON layout.
  - Not done: no human has played it. Keyboard input is untested. Report: `docs/prototype-report.md`.
  - The independent review of the report was started but cut off by a usage limit; the report's numbers were re-checked against the logs by the same session that wrote them.
- **Evening** Played the match-cut prototype myself. Movement felt fine, but I could not get past the first cut: four presses, all refused, and nothing on screen said why. The faint ghost alone does not teach the rule.
- **~20:20** Direction settled: *Deep Cut*. A diving robot swims through fixed survey-camera shots on the sea floor and moves between them with match cuts; a company film with scenes stamped [SCENE REMOVED] gets its missing scenes cut back in. The earlier "unfinished character" framing is dropped.
- **~20:20-23:00** Underwater vertical slice (brief 02), greybox, engine shapes only.
  - Built: an in-engine opening film in a film-player frame with three removed gaps, a reel strip in the letterbox bar, six shots, a 2D float with hand-written circle-against-rectangle collision, a landing ring that is green or red with the reason before the press, clapper cards for failed takes, a headlamp part, restoring one removed scene by lining up with its frame (12 s flashback), one crew rescue in a dark shot, an end card.
  - Checked with scripted offscreen runs only. One run goes from the film to the end card with a logged failed take for each kind of cut; every successful cut lands on the same screen position (0.00 px).
  - Found and fixed: the old 35 % lit ghost turns into an even green haze in fog, so the ghost is now a flat base-colour capture with the valid area outlined. The engine cube's physics body is hollow to overlap tests, so the validity mask showed water inside rocks; collision is now computed in the camera plane. The robot coasted about 180 units after a key was released; it now brakes harder with no input. The toon look was silhouettes until its emissive bands were scaled to the fixed exposure.
  - Found, not fixed: frame-time numbers from background-priority runs on a busy machine swing more between two runs of the same look than between looks, so the three looks cannot be ranked yet. The ghost of a lit shot is too bright over a dark shot. Large text is a blurry bitmap font. No audio.
  - Packaging (Mac, Development) took three tries. The first archive was a bare executable with no data or libraries (the script had no `-package` step) and died at launch on a missing library. The second quit at start: this Mac falls back to Metal SM5 and only SM6 shaders had been cooked. The third starts offscreen, loads the layout from the package, plays the film and reaches shot 1. Nothing past shot 1 was checked in the package, and nobody has seen its screen.
  - Not done: no human has played this build and the keyboard path is untested. Report: `docs/slice-report.md`.

## 2026-11-13

- **Night to ~14:30** Stage 1 (brief 03), greybox, engine shapes only.
  - Built: tape marks instead of the green area; a shot shows its marks only when its puzzle is solved; six wider shots with depth (Space / Shift); things near the robot come through a cut and can land in a frame of the next shot; R goes back one shot; one camera rail; two cameras with two tape colours in one shot; sound (13 files on four buses); two OFL fonts drawn at run time; shots, film and mixer as JSON; the game mode split into files by job.
  - Morning change: Unit 0 is a shelved prototype from the first second. The opening is a design-file card with three struck-out features, then a shorter company film (14.9 s together). The feature list stays in the top bar and the headlamp line is rebuilt on pickup. Valves were removed; puzzles are carry across the cut, lamp-reveal, one mark that comes and goes, and one camera choice.
  - Checked with scripted offscreen runs only. One run goes from the card to the end card and logs a failed take of each kind (not on a mark, too close, restore not lined up, too dark, not yet); every successful cut lands on the same screen position (0.00 px).
  - Found and fixed: text drawn from a font file alone showed nothing until it was given a run-time font object; a held plate swung with the robot's heading and missed the vent, so it now hangs straight under the robot; a mark outside the rail's middle zone could only be reached at the end of the rail; the opening's "seen before" flag made a scripted run depend on the run before it; an orphaned crash reporter from a morning crash used a third of a core for three hours and slowed every run.
  - Found, not fixed: the power cell lies inside the carry radius of the yellow mark, so it comes through that cut without being picked up. The mark that comes and goes has no reason in the picture yet. The dark shot's HUD is crowded when the cell is held on the mark. No focus pull on reveal.
  - Not done: no human has played this build and nobody has heard it; sound is checked by log only. Report: `docs/stage1-report.md`.

## Declared before the jam
- Tool setup only: an empty UE smoke-test project and an Unreal MCP test project (no game content), a tools/perks list, and a shelved concept note (*Lost Property*) that this game does not use.
- No models, images, music, dialogue or prompts for game assets were made before 12/11 12:00.

### 2026-11-13, afternoon: packaged build runs stage 1

- Packaged the Mac Development build and ran the `stage1` script in it, offscreen: card, film, all five cuts, one failed take per cut type, end card `scenes=1/3 crew=1/7`. 13 of 13 sounds and both fonts load from the pak. 50 captures.
- The first packaged runs wrote no log and no captures. Cause: the packaged Mac app is sandboxed and cannot write outside its container, and it fails silently. `Tools/run_packaged.sh` now lets the captures go to the container and copies them out, and takes the log from stdout.
- Frame times from these runs are 29-81 ms per shot, but they were taken at background priority on a busy machine and two runs differ by a factor of two. A normal-priority measurement is still missing.

### 2026-11-13, evening: stage 1 without the gripper

- Stage 1 no longer has `E: grab / drop`. Loose things are pushed by swimming into them and come through a cut when they are inside the dashed radius (STAYS / COMES / FITS labels kept). The plate and the power cell puzzles are solved by nudging. `F: lamp` shows only after HEADLAMP is built.
- THE WRECK: the two marks are labelled with the camera they cut to (`CAM 06 THE DOCK`, `CAM 05 THE DARK`), the blue one is the crew signal, and the hint says the same.
- THE DARK: the timed mark now has a reason in the picture. A warm glow drifts through the shot and the mark only holds while the glow is on it.
- Checked offscreen with the editor build: `stage1` (49 captures, end card `scenes=1/3 crew=1/7`) and `route_yellow` (end card `scenes=1/3 crew=0/7`) both run to the end.
- Found, not fixed yet: in THE DARK the body of the creature that carries the glow can be seen clearly; it should be only a faint light in this stage. Not checked in the packaged build.

### 2026-11-13, evening: stage 2 runs to its end card

- Stage 2 has five shots (THE TRENCH, THE LEDGE, THE CURRENT, THE DRIFT, THE LIFT). C on the stage 1 end card goes on to it; `-MCShot=6` starts there with everything stage 1 gives.
- The diver record is one clip in `film.json` with the seconds Fathom removed marked as cuts. The official version plays it without them (8.5 s), the restored version plays all 17 s. Both creatures are built from engine shapes. No image or sound was generated.
- GRIPPER: `E` grabs and drops. A heavy lid and a crew unit that cannot swim only come through a cut while they are held.
- The predator comes only when a shot's marks are showing: warning at the frame edge, then a chase. The only way out is a cut. Caught = static, SIGNAL LOST, the clapper with `TAKE n`. Two shots have it.
- First version of the diver clip had the predator swimming straight at the camera; in the captures it was a dot. The path now crosses in front of the camera so the long body is seen, and its skin is a little lighter on the old film.
- THE DRIFT: the first timing had the mark open while the big creature was still over the landing place. The sweep is now wide enough for the body to leave the place before the mark opens.
- Stage 1: the body that carries the warm glow in THE DARK is no longer shown, only its light.
- Checked offscreen with the editor build: `stage2` alone and `full12` (stage 1, C, stage 2) both reach `END CARD stage=2 scenes=2/3 crew=2/7`. 99 captures in `docs/captures/stage2/`. Report: `docs/stage2-report.md`.
- Not done: no human has played stage 2; the clip was looked at as stills, not in motion; sound checked by log only. Missing sounds (a predator sting, diver breathing) use stand-ins from the 13 existing sounds.
- Packaged Mac Development build made again; `full12` runs in it from the design-file card to the stage 2 end card (99 captures, 13 of 13 sounds load). `route_yellow` still passes in the editor build. `-MCShot=6` starts at stage 2 and plays the official record first.

### 2026-11-13, night: fixes after the first hand playtest (brief 05, part A)

The first person to play stage 1 by hand got stuck in THE SHELF: the plate was under the bottom bar, pushing needed floor height and the right depth, and the screen only said "No tape mark yet".

- Every floor is 65 cm higher, so things that lie on it are inside the picture. The plate is safety orange with a thin ring until the marks show. The VENT frame is 400 px away from where Unit 0 arrives.
- Pushing works from beside a thing at any height up to 150 cm above it and at any depth within 300 cm; Unit 0 lines up in depth by itself while it swims at the thing. A `PUSH` label and arrow show on contact.
- Depth is on the mouse wheel and Q / Z (SPACE and SHIFT are gone). Depth alone keeps Unit 0's place on screen, so only its ring changes size. A shadow ring on the floor under Unit 0 shows where it is.
- Controls are taught one at a time: a boxed prompt the first time the level needs a key, and only taught keys are listed in the bottom line. `-MCShot=N` counts what earlier shots teach as taught.
- A press of C before the marks show answers with the shot's own instruction. The clapper of a failed take has a second line that says what to do. After 20 s without progress the thing to use gets an arrow and a pulse, and an arrow to where it goes.
- THE VENT lost its camera rail; the way on is now behind the current, at a mark with a depth.
- Checked offscreen with the editor build: `stage1`, `route_yellow` and `stage2` reach their end cards. Captures: `docs/captures/brief05/partA/`. Not yet run after this change: `full12`, the packaged build. No human has played it.

### 2026-11-13, late: models from the art drop can replace the code-built shapes (brief 05, part C1-C3)

- `Tools/import_art.sh` imports every entry of `ArtDrop/manifest.json` into `Content/DeepCut/Art/<id>/` with a headless editor and writes `Content/Data/art.json`. The game reads that list at start: a thing with a listed id uses the model, everything else keeps its shapes. Sizes for the rules do not change; a model is scaled evenly, centred and stood on the bottom of the box its shapes used. Rock boxes pick one of several models by a hash of their place.
- Tested first with a rounded box written by `Tools/make_test_glb.py` (`-MCArt=art_test.json` shows it as the plate), then with the hero model. Capture: `docs/captures/brief05/art/`.
- Hero: one mesh without a rig, so it swims by code (bob, lean into the move, turn through facing the camera, small bank). First capture showed it facing backwards: fixed with a -90 degree turn in the list. It came out black because the game draws no reflections: models are now drawn non-metal, and Unit 0 has a soft light only it takes. The model has no separate parts, so the headlamp, gripper and pods cannot be shown or hidden by stage.
- Not done yet: the eleven prop and rock models (imported after the puzzle work). The hero's colours look washed out in the green fog; the emissive map is wired in but does not show in captures.

### 2026-11-13, night: three puzzles that need a thought (brief 05, part B)

All three use rules the game already had (near things come through the cut, a thing that lands in a frame of the next shot fits, R goes back one shot). Verified by `stage1` and `stage2` offscreen: `END CARD stage=1 scenes=1/3 crew=1/7` and `END CARD stage=2 scenes=2/3 crew=2/7`. Captures: `docs/captures/brief05/partB/`.

- **Graphic match, THE SHELF (shot 1).** Two loose things, a round one and a long one. The next shot's crack is drawn here as an outline with its real shape. Only the long plate on the outline shows the tape mark. The round one on the outline gets a red WRONG SHAPE label and no mark. The vent in the next shot is now a crack across the picture, so the outline and the crack are the same shape.
- **The predator as a tool, THE CURRENT (shot 8).** The crew unit is behind a shoal that Unit 0 cannot swim through. The predator now comes as soon as the vent is covered. Cut away while it is outside the carry circle: the log says `PREDATOR stayed behind`, the next shot says IT STAYED BEHIND and tells the player to press R. Back in the shot the shoal flies apart and the crew unit is free. Cut while it is inside the circle and it comes along: IT CAME WITH YOU, take again. The shoal stays gone after a retake.
- **Shuttle, THE DRIFT and THE LIFT (shots 9 and 10).** The lift needs the crew unit and a power cell, and the gripper holds one thing. The cell lies in THE DRIFT, too heavy to drift along. A white mark in THE LIFT cuts back to THE DRIFT. Either order works. The second predator of THE DRIFT is gone: the timed mark is enough there.
- A pushed thing can no longer be pushed so far to the side that Unit 0 cannot get behind it.
- `Tools/make_full12.py` writes `full12.txt` from the two stage scripts.
- Found on the way: the stage 1 script lost the power cell when Unit 0 bumped it early in THE WRECK. The script now swims to the cell wherever it is.

### 2026-11-13, ~23:30: the water, and all twelve models in the game (brief 05, C4 and the art drop)

- Engine-only visual pass (`DCVisual.cpp`): 660 flecks of marine snow sink and 92 bubbles rise in front of the live camera (two instanced meshes, no particle system); the slats that cut the key light into shafts now sway, so the light bars on the floor move slowly like caustics; colour grading per look (saturation, contrast, tint in `deepcut.json` `looks`); each rock block gets its own shade. The rim light on Unit 0 came with the hero model.
- All twelve models of `ArtDrop/` are imported (`Tools/import_art.sh`, 125 MB under `Content/DeepCut/Art/`). Rock boxes take the rock model nearest their own proportions. No model for `solid:dark` and `socket:LIFT`: code shapes stay. The two pieces of the graphic match keep their plain orange shapes, because their outline and contrast are the puzzle. The crack in THE VENT keeps its code shape for the same reason.
- First capture with the models showed every model glowing pale: the code raised the emissive of all of them. Now only the hero's is raised (it is the only one with an emissive map).
- `full12` offscreen after all of this: `END CARD stage=1 scenes=1/3 crew=1/7`, then `END CARD stage=2 scenes=2/3 crew=2/7`.
- Frame times in that run (editor, offscreen, background priority while the Mac was in use, so an upper bound): 28-37 ms in shots 0-3 and 6-7, 47-57 ms in shots 8-9, and 89-250 ms in shots 4, 5 and 10. Not measured at normal priority or in the packaged build yet.

### 2026-11-14, ~00:45: packaged build, yellow route mark, report (brief 05)

- Packaged Mac build runs `full12` to both end cards with the models, the water and the three puzzles, and `route_yellow` to its end card. Captures: `docs/captures/brief05/packaged/`.
- Frame times in the packaged `full12`: 25-34 ms in every shot in the first run; 26-34 ms and 54-110 ms (shots 3-7) in the second. Both at background priority with the Mac in use. A run at normal priority is still missing.
- `route_yellow` failed once tonight: the power cell sat on the DOCK frame and still stayed behind. Cause: the frame's centre was 294 cm from THE WRECK's yellow mark, outside the 280 cm carry circle; it only worked when the cell lay on the right part of the frame. The mark moved from x 400 to 380. From its start place the cell still stays (292 cm), from the frame it comes and fits (275 cm).
- `docs/brief05-report.md` written: what changed, how a newcomer learns each puzzle, frame times, every new English line, what is unverified, what to cut first, play commands.

### 2026-11-14, ~03:45: the game goes 3D; movement and camera first (brief 06, M1)

- The side-view game is kept as the git tag `v2d-fallback` (09033c8) and behind `-MC2D`. An unfinished change that removed depth movement from it was dropped.
- New `DC3D.cpp`: free swimming with momentum (push towards a speed, slow glide when the keys are let go, slide along rock), collision by engine sweeps against the models' triangles, a camera behind Unit 0 that a swept ball keeps out of rock.
- One test area built in code from the ArtDrop models at uniform scale (arch, wreck, vents, a roofed gully, a spire), six rings to swim through.
- The hero model had looked pale since it was imported. Cause: the glTF material adds its emissive colour to the whole surface unless the static switch `bHasEmissiveTexture` is on; the import set the colour to white without the switch. Fixed in `Tools/import_assets.py`, hero imported again; its emissive map now lights only eyes, lamp and chest.
- Lumen does not run on this Mac: the engine falls back to SM5 (its SM6 check fails on this macOS). A run with Lumen asked for looked and timed the same as one without.
- Film look without Lumen: volumetric fog, dappled moving light from unseen pieces high in the water, blue water, warm headlamp, bloom, chromatic aberration, grain.
- `m1_swim` offscreen, alone: 29.9 ms average over 3037 frames (editor build, background priority, Mac in use).
- 03:40: the hero is now a diver in a hard suit (new `ArtDrop/hero.glb`); imported with `DC_ART_ONLY=hero Tools/import_art.sh`. The robot model is no longer used.
- Six creature models from the art drop imported (manta, anglerfish, jellyfish, turtle, eel, fish) for brief 07; not used yet.

### 2026-11-14, ~05:15: the documentary: camera, takes, creatures, a new world (brief 07, checkpoints 1 and 2)

- New `DCDoc.cpp`: camera mode (viewfinder, zoom, focus, depth of field), 3 second takes, a score of 0 to 3 stars with one named reason per take, six shots on a list.
- The world is rebuilt in code from eight realistic scene models (boulder, arch, ledge, wreck, debris, vent, station, coral). The stylised props of the 2D game are no longer placed.
- Six subjects driven by one clock (manta, shoal of 130 instanced fish, anglerfish, jellyfish, turtle, crewmate), so a take is stored as camera transforms plus a clock value and played back by drawing the world again.
- The anglerfish sits on its own lighting channel in a roofed trench and is lit only by the headlamp, which is found on the wreck.
- Lights: unitless inverse-square lights were far too weak at 10 m and more. The headlamp and the light shaft now use the plain falloff exponent.
- Eight more creature models imported and set swimming or crawling as ambient life (30 creatures). Frame time went from about 30-35 ms to 48 ms with them; not acceptable yet.
- Cutting room, title card, film playback and credits run in the `d3_film` script. The rating card was not reached in that run because the script outran the slow frames.
- Rule for this repo from now on: scripts move files to `Saved/_trash/` and never delete.

### 2026-11-14, ~06:40: shot list to finished film in one scripted run (brief 07, checkpoint 3)

- `d3_film` offscreen, editor build: start, two takes of the crewmate, headlamp from the wreck, manta, jellyfish, anglerfish, shoal, turtle, cutting room, title card, six takes played back, credits, rating card (`FESTIVAL OPENER`, 17 / 18 stars), back to the cutting room, back to shooting. All six shots registered.
- The earlier run lost the jellyfish take and never reached the rating card. Cause: the test script counted its waits in real time while the game steps at most 0.05 s a frame, so on slow frames the script ran ahead. Script waits now use the game's step.
- Jellyfish: the column was 650 cm tall, so a normal lens rarely held three at once. Now 440 cm and two are enough. A take with only part of a group in frame says `Too few of them in frame`.
- Light in the water: faint additive columns along the key light, and one upright in the clearing. They replace the volumetric shaft that never showed. The first try came out as solid black columns: parts made with `AddPart` get the look's material again later, so the columns are made without it.
- The unseen shadow casters that dappled the floor are off by default. Bars drew hard rectangles, round blots drew polka dots, 9000 small pieces drew camouflage; all of them fought the creatures for the eye.
- Seven more creature models imported (vampire squid, hatchetfish, frilled shark, sea pig, coelacanth, oarfish, stingray): 52 ambient creatures, none casting shadows.
- Frame time report now carries median and 90th percentile, because the average of a test run includes the hitch of every screenshot. `d3_film` alone, editor build at background priority: median 28 ms, p90 66 ms, average 42 ms over 3062 frames.

### 2026-11-14, ~10:15: a fuller sea, and why the first package failed (brief 07, checkpoint 4 in progress)

- The packaged build did not compile: `DC3D.cpp` read a material's static switch with `GetStaticSwitchParameterValue`, which exists only in editor builds. The call is now inside `WITH_EDITOR`; outside the editor the hero's emissive map is taken as present, which is how the importer saves it.
- Imported: brittle star, comb jelly, sea angel; kelp, tube worms, clam, sponge, anemone, spire, cliff, whale fall, anchor, bell, crashed submersible. Each is placed by a seeded scatter that drops it onto whatever lies below. 67 ambient creatures.
- Caustics as a light function (`M_DC_Caustics`, `Tools/make_caustics_material.py`: two layers of computed cell noise in world space) took the median frame from 28 ms to 152 ms in its first run and did not show on the floor. It stays in the project but off (`-MCCaustics=1`). Not investigated further.
- The clearing's shaft is seven thin additive shells instead of three, so its edge is a gradient. The jellyfish get a stronger soft light of their own and now read pink against the rock.
- The film music plays under the finished film and the rating card.
- `d3_film` alone, editor build, background priority: 6 of 6 shots, 18 of 18 stars, median 25 ms, p90 48 ms, average 33.8 ms over 3398 frames.

### 2026-11-14, ~10:50: the packaged build plays the whole documentary (brief 07, checkpoint 3 closed)

- `Tools/package_mac.sh` succeeded after the compile fix. `Tools/run_packaged.sh d3_film Saved/DeepCut/d3_pkg`: 6 of 6 shots, 18 of 18 stars, cutting room, film, rating card, back to the cutting room, back to shooting.
- Packaged frame time in that run: median 23 ms, p90 47 ms, average 31.6 ms over 3570 frames (background priority, Mac in use).
- `docs/brief07-report.md` now has the play commands, every new English line, the frame times, what is not verified and what to cut first.

### 2026-11-14, ~11:20: the packaged app crashed on a normal launch; the release is now a Shipping build

- Launching the Development package with `open` (or a double-click) crashed within seconds: the engine's memory tracker (LLM) was being torn down on the game thread (`FLLMGlobals::ProcessCommandLineInner` -> `Clear`) while AppKit allocated on the main thread and ran into the half-freed tracker. The scripted runs start the executable directly and never met it.
- The engine is an installed build, so the tracker cannot be compiled out of a Development game target. The fix is to ship the Shipping configuration, which has no memory tracker at all (`nm` finds 0 `FLLMTracker` symbols in the new binary, 167 in the old). `Tools/package_mac.sh` now packages Shipping; the app is `Saved/Package/Mac/MegaJam-Mac-Shipping.app`.
- Shipping builds compile `UE_LOG` out. The test scripts read the game's lines, so in Shipping those lines are printed to stdout, and only when a test script runs (`-MCScript=`). `Tools/run_packaged.sh` finds the newest app and its executable by itself.
- Bundle id is `com.tsangfx.DeepCut` (was `com.YourCompany.MegaJam`); the project's name is Deep Cut.
- Verified: three launches through `open -g -n` (offscreen), no crash report. The first ran `d3_film`: 6 of 6 shots, 18 of 18 stars, median 21 ms, p90 26 ms over 4617 frames (the editor was compiling at the same time). A launch with a visible window was not made by the agent.
- Models imported: buoy, container, pipe, sonar dish, habitat module, ruins, chimney; octopus, barreleye, goblin shark, gulper eel, sunfish, sea dragon, seahorse, lobster. 68 ids in `art.json`.
- The sea floor is a sand material without a texture (`M_DC_Sand`, `Tools/make_sand_material.py`) with up to 70 low mounds; vents and chimneys have a rising plume of specks and haze.

### 2026-11-14, ~11:25: the shoot is guided (first playtest: "I don't know how to play")

- One shot is the current one, in a fixed order (crewmate, shoal, manta, jellyfish, giant, anglerfish). A card names it and says in one line what to film and where. Any shot can still be filmed out of order.
- A marker leads to it: a ring with the distance in metres while its place is in the picture, an arrow at the edge of the screen otherwise. It fades when the lens has the subject and pulses after a minute without a new shot. For the anglerfish it points to the headlamp first.
- The first shot is taught one prompt at a time: swim up, raise the camera, frame, record.
- The director types one line between shots, and a take of one star or none comes with a tip for its reason.
- Test: `Content/Data/tests/d6_guide.txt`.

### 2026-11-14, ~11:50: every creature swims (first playtest: "no creature has any swimming motion")

- The models have no bones. One material, `M_DC_Creature` (`Tools/make_creature_material.py`), moves the body in the vertex shader from the vertex's place in the mesh: a wave along the body that is zero at the head and largest at the tail, a wave that grows away from the middle line (wings, flippers, legs), and a pulse round the axis (bells, mantles). The model's own textures are copied into it by name.
- Thirteen kinds with their own numbers (`DCSwim::Kinds` in `DCDoc.cpp`): fish, sharks, eels, whale (fluke beats up and down), rays, cuttlefish, sea angel, jellyfish (the wave runs down the tentacles), squid and octopus, crabs and their like (legs), sea pig and nudibranch, turtle (flippers). Every animal has its own phase; the fish of the shoal use the per-instance random number; the beat follows the animal's speed for its length.
- Ambient swimmers bank into their turn; jet swimmers (squid, octopus, nautilus, jellyfish) move in surges.
- Facing: `d5_facing.txt` lines every creature model up in side view. Octopus, squid, vampire squid, nautilus and stingray came with the nose at the other end and are turned round (`DCSwim::FrontFix`).
- `d3_film`, editor, alone: 6 of 6, 18 of 18 stars, median 29 ms, p90 54 ms (25 and 48 before the swim).

### 2026-11-14, ~15:45: the wider sea (playtest: "I need a wider map, and far away the scenery has to be different")

- The swimmable area goes from a 55 m radius to 250 m. The first site stays in the middle; six zones lie around it, each built from its own models and with its own water: The Kelp Forest, The Wreck Graveyard, The Drowned City, The Whale Fall, The Black Smokers (with a research outpost on the way), The Abyss.
- Fog colour and density, key and fill light and the colour of the flecks blend from the first site's look into each zone's (`FDC3D::SetupZones` / `TickZones`). The depth readout adds the zone's depth; a title card names the zone when you swim in, and "THE STATION" when you come back.
- Trails of beacons lead out from the first site to each zone; an outer ring of cliffs closes the sea. Small things far from the middle get a cull distance; ambient creatures more than about 110 m away are hidden and not moved.
- About 40 new ambient creatures per zone, from the batch 3 to 5 models (48 new models this session, logged in `ASSETS.md`). The Abyss is lit only by blue and violet points of life.
- Tests: `d8_zones.txt` (teleports to each zone, snaps); `d3_film` still 6 of 6, 17 stars; `d6_guide` passes.

### 2026-11-14, ~17:00: easier shoal, sharper picture, Unit 0 kicks (playtest: "blurry, fish flicker and spin when close, the hero is frozen")

- The shoal parts as soon as you are within 10 m, at any speed, and stays open for a few seconds after you leave; the point it parts from follows you softly. Card and tip say so.
- Fish spun because the flee turn was a straight lerp between unwound angles; it now turns the short way, at most 70 % of the way. The fast 9° yaw wobble is gone (the tail beat is the material's), the tail beats slower (4.5 Hz), rings are wider.
- The shoal's instances carry last frame's transforms and vertex deformation writes velocity (`r.Velocity.EnableVertexDeformation=1`), so TSR no longer smears or flickers them.
- Picture: Retina rendering in game (`bAllowHighDPIInGameMode`) at 70 % screen percentage upscaled by TSR, `r.Tonemapper.Sharpen=0.8`, chromatic fringe 0.9 → 0.2, grain 0.18 → 0.06, camera depth of field f/2.2 → f/4.5.
- Unit 0 uses the swimming material too (kind `hero`: axis head to feet, swing forward and back): a fin kick in the legs and a paddle in the arms, scaled by speed every frame so the beat never jumps. `M_DC_Creature` gained an emissive map so the eyes and lamp still glow.
- Tests: `d9_look.txt` (hero side views, the shoal close); `d3_film` 6 of 6, 18 of 18 stars, median 25 ms, p90 69 ms.

### 2026-11-16, ~01:40: the base from outside, creatures that act their kind (playtest: "the base outside does not match the inside, no door; the worm is a statue")

- The station hull is cut down to the room it holds (it was twice as wide); under the moon pool hangs a short entrance trunk with a yellow rim and a warm light, so the way in reads from the sand. Scatter keeps clear of the base footprint; the station model that sat under it is gone.
- Worms (maw worm, bobbit, polychaete) get their own swim kind (an S-wave through the whole body) and a new habit flag `Burrow`: they lie buried at rest, come out and slither in S-curves to the next spot, and lunge, rearing up, at a diver who comes within about two body lengths.
- The beat never stops (crawlers and worms keep moving at rest, slower). The career subjects have their own motion: the Ribcage Ray flaps like a manta, the Hollow Diver drifts upright and kicks, the Drowner perches and jets like an octopus, the Lamp Widow hangs by its lure and darts, the Warden cruises as a hunter.
- Tests: `d32_base.txt` (outside and deck views), `d33_worm.txt` / `d35_wormfar.txt` (worm near and far), `d36_uniques.txt` (the six subjects).

### 2026-11-16, ~02:30: fixes from the night's playtest ("the hatch gets stuck", "sometimes ESC does nothing", "the brown rim is in the way")

- Moon pool: going down drops the diver through the middle of the hatch; swimming up near it pulls gently to the centre and climbs out as soon as the head is over the opening; a short cooldown stops the drop and climb from fighting each other (`d27_hatch.txt`).
- The hatch opens straight onto the real sea: the brown ring, trunk and fake water plane are gone, the hull material is one-sided so the opening shows the sea floor, and a steel lip with a yellow rail marks the edge (`d31_pool.txt`).
- ESC: the pause menu no longer depends on not standing at a shot spot, so it always opens.
- The water moves: marine snow drifts with a slow side current, the diver breathes out small bubbles, kelp, grass, fans and anemones sway from the root in a swell that rolls across the site (`d29_look.txt`, `d37_sway.txt`).
- Three creature loops in The Abyss cut through the cliff wall; the loop search now also tries shifted centres before giving up, and every loop in the log is clear.
- The take result says only "Saved."; the line about getting more views is gone.
