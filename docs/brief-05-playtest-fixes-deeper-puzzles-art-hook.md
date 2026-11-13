# Brief 05 – Deep Cut: playtest fixes, deeper puzzles, art hook and visual pass (for a Claude Fable session)

> APPROVED by the user 2026-11-13 ~20:15 VN: "cần đầu tư các trò giải đố, hiện tại thấy nhạt quá" and "bạn cho fable chạy rồi bạn làm đồ họa cho các vật phẩm, cảnh,... bản sau tôi test tôi muốn có đồ họa luôn, có gì chỉnh còn kịp". Earlier tonight: "tập trung vào làm gameplay trước", keep the current shot-based map (no open world), no new video clips.

## Why
Deadline **16 Nov 08:59 Vietnam time**. The user played stage 1 by hand for the first time tonight and got stuck in shot 2 (THE SHELF) for a long time: the screen never said what to do, the plate could not be pushed, SPACE/SHIFT did nothing visible. The user also finds the puzzles bland. The next build the user tests must (1) never leave a newcomer stuck, (2) have puzzles with a real "aha", and (3) show real art wherever it already exists. Stages 3–4 and the ending come in brief 06, not here.

Read first: `docs/playtest-notes.md` (the user's feedback, with causes), `docs/OUTLINE.md`, `docs/stage1-report.md`, `docs/stage2-report.md`, `docs/brief-04-stage2-gripper-and-the-other-creature.md` (conventions), `ASSETS.md`, `Tools/import_assets.py`.

## Theme rule (unchanged)
Every puzzle is an act of editing: what goes into the frame, what gets cut, which version is true. No valves, levers, doors, combat or hiding.

## Part A – playtest fixes (do first, commit and push as soon as stage 1 passes)
1. **Nobody gets stuck without being told what to do.** The shot hint stays visible until the shot is solved; a failed cut shows the reason *and* the next action (e.g. "Push the plate onto the VENT frame first."). After ~20 s with no progress in a shot, show one stronger hint (an arrow or pulse on the thing to use).
2. **Every interactive thing starts fully in frame, above the HUD bars, and reads at a glance**: contrasting colour or a thin outline/ring, never brown-on-brown. Check every shot of stages 1–2 with a capture at shot entry. The plate in THE SHELF is the known case (it sits under the bottom HUD bar).
3. **Pushing is forgiving.** Contact works at any height Unit 0 can swim to next to the thing, and depth snaps when Unit 0 is near it. Show a small "PUSH" cue on contact. Same for the power cell.
4. **Depth control is visible and makes sense.** Rebind closer/farther to the mouse wheel and Q/Z (E is grab). Add a soft shadow/ring on the floor under Unit 0 so depth can be read. Manual depth only matters at depth marks; the first one says so on screen.
5. **The VENT frame and Unit 0's spawn point do not overlap**, so the robot is never read as the vent.
6. **Controls are taught when they are first needed, like most games (user, 2026-11-13 ~20:20).** The bottom controls line no longer lists every key from the start. Each control appears the first time the level needs it, with a short one-time prompt at that moment: WASD at the start; C at the first mark; R the first time a take fails; push cue at the first loose thing; depth (wheel / Q/Z) at the first depth mark; F when HEADLAMP is built; E when GRIPPER is built. After it has been taught, the key stays in the controls line. Starting with `-MCShot=` counts earlier controls as taught.
7. Keep `stage1`, `route_yellow`, `stage2`, `full12` scripts passing (update them for the new controls).

## Part B – deeper puzzles (stages 1–2, keep the shot count about the same)
Make 3 puzzles that need a moment of thought. Each must be learnable without text walls, have no dead end (R always recovers), and get a capture before/after.
1. **Graphic match (stage 1).** A cut only "takes" when a shape in this shot lines up with a matching shape in the next shot's frame (round thing over a round hole, a long thing along a crack): the next shot's shape is drawn as a ghost outline in this shot, the player chooses which loose thing to push where. Wrong shape = it comes but doesn't fit (existing STAYS/COMES/FITS).
2. **The predator as a tool (stage 2).** Lead the predator into a shot and cut away while it is *outside* the carry radius: it stays in the old shot and does something there (scatters a shoal blocking the way, breaks a brittle ledge). Coming back with R shows the changed shot and opens the route. This is the same carry rule stage 4 will use, so it also teaches the finale.
3. **Shuttle (stage 2).** A goal needs two things from two different shots: carry one across, go back (R), bring the second, combine (e.g. lid + cell, or two plates side by side to make a ledge the current can't push you off). The player plans the order of cuts.
Retune or replace the weakest existing puzzles rather than adding shots. Report how a newcomer learns each one.

## Part C – art hook and in-engine visual pass
1. **Art drop folder.** I (the main session) will put AI-generated models in `MegaJam/ArtDrop/` (GLB, plus textures) with `ArtDrop/manifest.json`: `[{ "id": "hero" | "crew" | "plate" | "cell" | "weight" | "socket:VENT" | "socket:DOCK" | "socket:LIFT" | "solid:rock" | "solid:dark" | "solid:metal" | "solid:hull" | ..., "file": "...glb", "yUp": bool, "notes": "..." }]`. Extend `Tools/import_assets.py` (headless editor, `taskpolicy -b`) to import them into `Content/DeepCut/Art/`. In the game, use the mesh for that id when present and keep today's shapes as the fallback. **Collision and gameplay sizes stay the current boxes**: the mesh is fitted (uniform scale, centred, grounded) inside the existing bounds. For rock solids, pick from a small set of rock meshes by a stable hash so shots don't look tiled.
2. **Hero:** if `hero` exists, it replaces the robot body. No rig: procedural swim motion (bob, tilt into the move direction, small roll on turns), lights (eyes, headlamp) re-attached at sockets you place by hand from the model's bounds. Missing parts by stage (headlamp, gripper, camera pods) are toggled by hiding/showing sub-meshes if the GLB has them, otherwise skip that for now and say so.
3. Test the pipeline now with one simple GLB you make yourself (e.g. a rounded box exported by a script), so it works before my models arrive. Then import whatever is in `ArtDrop/` when you get to this step; I may also message you when new files land.
4. **Visual pass without any AI images** (engine only): underwater depth fog and colour grading per look, light shafts, slow caustics on the floor, marine snow and small bubble particles, a soft rim light so Unit 0 always reads against dark rock, variety in rock shapes/colours. Keep 60 fps-ish on this Mac; report frame times per shot.
5. Log every imported asset in `ASSETS.md` (I will write the tool/prompt/job id lines for my models; you add the import line).

## Order
A (push) → C1–C3 (push) → B (push after each puzzle works) → C4 (push) → import what is in ArtDrop.

## Not in this brief
Stages 3–4, MULTI-CAM, the ending, new film clips, Windows build, the submission video.

## Rules (same as brief 04)
- The user works on this Mac: never open a visible Unreal window or steal focus; every UE process with `taskpolicy -b`; visual checks offscreen; look at the captures yourself.
- Commit small and often, push to `main`; `git diff --cached | ~/bin/ioc-scan.sh` before each push; commits under the user's git identity only, no AI attribution lines; never commit CLAUDE.md, .claude/, .mcp.json, secrets. Factual dated DEVLOG.md entries (English, never mention any review council).
- No paid API calls. At most one fresh-context verifier subagent at the end. Ground every claim in a tool result; say plainly what is unverified. If the usage limit hits, commit what runs first. Time cap about 6 hours of work.

## Done means
1. Offscreen and packaged scripted runs through stages 1 and 2, with captures of: every shot at entry (things readable, above the HUD), each Part A fix, each new puzzle before/after, the visual pass per look, and the hero mesh in game if one was in ArtDrop.
2. `docs/brief05-report.md`: what changed, how a newcomer learns each puzzle, frame times, new English lines (all of them, for approval), what is unverified, what you'd cut first.
3. Everything committed and pushed; exact play commands, including `-MCShot=` starts for each new puzzle.
