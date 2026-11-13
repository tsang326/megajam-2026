# Brief 03 – Deep Cut: tape marks, puzzles that earn the cut, wider shots (for a Claude Fable session)

> APPROVED by the user 2026-11-12 ~23:50: build **both A and B, B first**.

## Why this exists
Solo entry in the 2026 Epic AI MegaJam (theme "Director's Cut", Unreal Engine, deadline **16 Nov 08:59 Vietnam time**). Judged on 7 criteria × 5: ambition, theme, fun, gameplay, visuals, level design, audio. Judges get a packaged Windows build and a 30–60 s video.

Brief 02 delivered a slice that reads: the user played it on 2026-11-12 and passed shot 1 in 54 s with no help, then every other shot in 3–13 s (log `~/Library/Logs/MegaJam/MegaJam.log`, 16:04–16:06 UTC). Their verdict: **"you just keep pressing C"**. The cut is a single button and the shots are thin 2D slabs, so there is nothing to think about between cuts. This brief makes the cut something the player *earns and chooses*, in shots with room to explore.

Read first: `docs/slice-report.md` (what exists, cut rule, what breaks first), `docs/brief-02-underwater-slice.md`, `../IDEAS.md` Decision 10 (Vietnamese).

## Decisions by the user (2026-11-12 ~23:30)
1. **Tape marks replace the green area.** Valid landing spots in the next shot are shown as film-set **tape marks** (an X or T of coloured tape on the floor/rock, like an actor's mark – "hit your mark") inside the ghost of the next shot. The green/red Unit 0 landing silhouette stays (it tells the player before pressing C whether the cut works and why).
2. **Marks appear only after the player solves something in the current shot.** No solved puzzle, no mark, no cut. The first shot stays a gentle tutorial (solving it is trivial and taught by one on-screen line).
3. **Wider shots.** The current shot is a thin slab with rectangles; the user finds it cramped. Shots must have room to explore.
4. The restore-by-cut keeps its own visual (the faded old-film frame with the robot's silhouette); do not use tape marks there.

## Change from the user, 2026-11-13 morning (overrides the puzzle list below)
Read `docs/OUTLINE.md`. Unit 0 is a **shelved prototype shown from the first second** (not a late twist). For stage 1:
- Opening ≤ 15 s: design-file card "UNIT 0 – PROTOTYPE · SHELVED 02:00 AM" with struck-out features, then the Fathom film with the `[SCENE REMOVED]` gaps. Keep the struck-out feature list visible in the HUD.
- Part pickup = "FEATURE CUT → BUILT: HEADLAMP" (strike-through removed), then restore-by-cut as before.
- **Puzzles: edit puzzles only.** Drop switch/valve and crew-holds-door. Use carry across the cut (B), lamp-reveal (the lamp is the rebuilt feature), cut on the right moment, and one camera choice (A).
- First cut of the game must be doable in under 20 s by a newcomer.

## User's choice: A + B, build B first (A after B works end to end)
- **A. Choose the camera:** some shots have 2–3 next cameras; each camera's marks use its own tape colour; the cut goes to the camera whose mark Unit 0 lines up with. Different cameras lead to different routes (one leads to the crew robot).
- **B. Carry across the cut:** whatever is in frame with Unit 0 inside a small radius (a loose plate, a rock, a rescued crew robot) is placed in the next shot at the same screen position and distance. Puzzles: put the object where it is needed in the *next* shot's frame (block a current, make a step, cover a vent, light a dark spot).
- Stage 1 must contain at least two B puzzles and at least one A choice (two cameras, two routes, one of them leading to the crew robot).
- Priority if time runs short: B + tape marks + puzzle gating + wider shots + audio first; then A; camera rails and the code split last.

## Scope (only this)
1. **Tape marks.** Replace the green valid-area tint/outline with tape marks drawn in the ghost (decal or screen-space sprite at the mark's 3D spot, readable at 1080p and 720p). A mark has a small capture radius in screen space (start generous; shrink after the first stage). Remove the green tint; keep the silhouette ring. Report what radius you chose and why.
2. **Puzzle-gated marks.** A shot's marks stay hidden until its puzzle is solved; then they appear with a short sound and a camera "focus pull" or similar. Puzzle building blocks for stage 1 (use what exists, keep each one obvious):
   - lamp: a mark painted in a dark corner only shows when the headlamp beam is on it;
   - switch/valve: swim to it and press E to open a hatch or stop a current, revealing the marks behind;
   - object: push or carry a plate/rock off the spot that hides the mark (or, with choice B, carry it through);
   - crew: a freed crew robot holds a door/lever (it follows; keep it simple).
   Stage 1 = 5–7 shots using at least three of these, rising in difficulty, still containing the part (lamp) → restore-by-cut → one rescue from brief 02.
3. **Wider shots, real depth.** Give each shot a proper 3D swim volume: the robot moves left/right, up/down **and toward/away from the camera** (on-screen size changes, so the match cut's "keep screen position and distance" now matters – a mark can need the right depth). Shots can be several times wider than now. Keep the fixed survey camera; where a space is wider than one frame, the camera may **pan or dolly along a rail** to follow the robot (survey-cam motion, slow, clamped). The cut uses the camera pose at the moment of the cut. Report how you did collision (brief-02 used hand-made rectangles because the engine cube's physics body was hollow to overlap tests – fix that properly now: use real collision on meshes or simple convex volumes, so art and collision cannot drift apart).
4. **Level authoring that scales** (slice report "what breaks first" #1–3): shots defined as data or placed actors in the map (not rectangles typed in JSON), a way to preview a shot quickly, and split the 1,700-line game mode (cut rule, film/flashback, puzzles, HUD state) so adding abilities is cheap. Flashbacks/film as data (key poses), not C++.
5. **Audio (new – all files ready).** Wire the user's chosen sounds from `../audio/picked/` (13 files: `amb_underwater` loop, `cam_rec`, `cut_splice`, `take_clapper`, `lamp_on`, `part_pickup`, `projector` loop, `reel_restore`, `thruster`, `plate_lift`, `mus_opening`, `mus_explore`, `mus_flashback`). Simple mixer: music / ambience / SFX buses, music ducks under the flashback narration-less film, thruster plays on swim bursts (pitch-randomised ±5 %), cut_splice on a good cut, take_clapper on TAKE n, mark-reveal reuses `cam_rec` or `reel_restore` (say which). Copy them into Content and log every file in `ASSETS.md` (tool: fal – ElevenLabs Sound Effects v2 / Lyria 2; prompts and request ids are in `../audio/takes/log.jsonl`; the chosen take per id is in `../audio/picks.json`). Loudness: level-match so no SFX is more than ~6 dB above the ambience bed by ear-proxy (measure with ffmpeg loudnorm or volumedetect, report numbers).
6. **Known slice fixes:** dark shot ghost too bright over black; THE CHIMNEY reads as a black wall; blurry large text (import a proper font made/licensed for the jam – log it in ASSETS.md; an open-licence font such as one under SIL OFL is fine).
7. **Packaging:** keep the Mac package working (`Tools/package_mac.sh`, now with `-package` and SM5+SM6). Make the test-script runner work in the packaged build (it failed with `Cannot read script`) so the packaged game can be driven offscreen through stage 1 with captures. Keep `docs/packaging-windows.md` current.

## Not in this brief
Stages 2–4, the other abilities, story reveals B → A → C, the ending. Look choice: keep look A until the user picks (captures from brief 02 are in `Saved/DeepCut/lookdev/`).

## Text
English, B1-level, short. List every new player-facing line in the report; the user approves all text.

## Project facts and rules (unchanged from brief 02)
- Project `/Users/os/Documents/side-gigs/itch/megajam-2026/MegaJam` (UE 5.8.4, module `MegaJam`), repo `tsang326/megajam-2026`, branch `main`.
- **The user works on this Mac: never open a visible Unreal window or steal focus.** Every UE process with `taskpolicy -b`; visual checks offscreen (`-game -RenderOffscreen -ForceRes -ResX=1920 -ResY=1080`); look at the captures yourself.
- M1 Pro 16 GB; one reference Windows PC has Intel Iris Xe – keep the default look cheap; measure.
- New assets made during the jam only, each logged in `ASSETS.md` (file, tool, prompt, time, request id). No Meta Muse images in the build. Engine-made shapes preferred for greybox.
- Commit small and often, push to `main`, run `~/bin/ioc-scan.sh` on the staged diff before each push. Commits under the user's name only, no AI attribution lines. Never commit `CLAUDE.md`, `.claude/`, `.mcp.json`, secrets. Append factual dated entries to `DEVLOG.md` (English, never mention any review council).
- At most one fresh-context verifier subagent, at the end. Ground every claim in a tool result; say plainly what is unverified.
- Don't stop to ask unless destructive/irreversible, scope would change, or you need something only the user has. If the usage limit hits, commit what runs first. Time cap: about 5 hours.

## Done means
1. Offscreen `-game` run **and** packaged-build run through stage 1 (film → shots with puzzles → part → restore-by-cut → rescue → end card), with captures of every puzzle before and after solving, and one failed take per cut type.
2. Audio audible in the captures' run (log each sound event) with the loudness numbers.
3. `docs/stage1-report.md`: what works, mark radius and depth rule, collision approach, camera rail rule, each puzzle and how a newcomer learns it, frame times (packaged), every new English line, and what you'd cut first if time runs short.
4. Everything committed and pushed; the exact command for the user to play.
