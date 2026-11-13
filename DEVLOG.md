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
