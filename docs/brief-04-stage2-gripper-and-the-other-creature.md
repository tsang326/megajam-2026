# Brief 04 – Deep Cut stage 2: GRIPPER, the framed creature, the predator (for a Claude Fable session)

> APPROVED by the user 2026-11-13 afternoon ("cứ làm 4 chặng như outline"). The user has not played stage 1 yet (tonight). Do not change the core feel of movement and cutting until their feedback comes in.

## Why
Solo entry, 2026 Epic AI MegaJam, theme "Director's Cut", deadline **16 Nov 08:59 Vietnam time**. Judged 1–5 on ambition, theme, fun, gameplay, visuals, level design, audio. The game is four stages (user decision). Stage 1 is done (`docs/stage1-report.md`). This brief builds stage 2 and fixes what stage 1 got wrong against the outline.

Read first: `docs/OUTLINE.md` (the whole game, user-approved), `docs/stage1-report.md`, `docs/brief-03-marks-and-puzzles.md` (rules and conventions you already used), `../IDEAS.md` Decisions 11–12 (Vietnamese).

## Theme rule for everything you add
The theme's main line is "the idea you shelved… finally build it". Every new thing must be an act of editing: what goes into the frame, what gets cut, which version is true. No valves, levers, doors, combat or hiding-behind-rocks.

## Stage 1 fixes (do first, small)
1. **Gripper is still cut in stage 1, so no grabbing there.** Remove `E: GRAB/DROP` from stage 1. Loose things near Unit 0 simply come with it through the cut (the dashed radius, STAYS/COMES/FITS stay). Re-tune the plate and cell puzzles so they work by nudging/swimming the object (push by contact) or by lining up Unit 0 next to it. Keep `F: lamp` after the HEADLAMP is built.
2. **Check the camera-choice labels in THE WRECK** (`s3l_blue_mark_lamp_on_cell_comes`): the hint says "Yellow: the dock. Blue: a crew signal", but the yellow X looks labelled "CREW SIGNAL". Make label, colour and hint agree.
3. **Give the timed mark in THE DARK a story reason**: the mark is lit when the gentle creature's slow glow passes over it (seen as a faint warm light drifting through the shot). This also plants the gentle creature before stage 2.
4. Keep both scripts (`stage1`, `route_yellow`) passing.

## Stage 2 (new): 5–7 shots, rising difficulty
**Story beat (twist B):** the opening film's first gap becomes, in stage 2, a diver head-cam clip from Fathom's own record. Official version: a first-person diver view, mask frame, bubbles, torch, a big glowing shape (the gentle creature) rushing in, signal lost. Unit 0 restores the cut seconds: a dark, thin, fast **predator** hits the diver first; the gentle creature arrives after and pulls the diver away. Fathom cut the predator out to blame the gentle creature and justify capturing it. One short English line on screen after the restore, e.g. "Fathom cut it out." (list all lines for approval).

**Feature rebuilt:** `FEATURE CUT -> BUILT: GRIPPER`. From then on E grabs/drops: carry heavy things and a rescued crew robot across the cut (B). Puzzles use carry, placing in the next frame (sockets), cut-out (choose the shot where an obstacle is not), lamp-reveal, and cut-on-the-moment with a reason in the world.

**Predator in play (user decision):** appears only in shots whose puzzle is already solved (marks visible): a shadow with eyes and a sound sting, coming from off-frame. The only escape is to cut away before it reaches Unit 0. Caught = static/noise → `TAKE n` of that shot (no blood, no combat, no health). First appearance must be readable and fair (long warning, slow), at most two predator shots in stage 2.

**Crew:** rescue 1–2 more units (CREW x/7), at least one carried across a cut with the gripper.

**Creatures:** build both from engine shapes/materials in the look of the game (no AI images). Gentle creature: big, slow, warm glow. Predator: thin, dark, fast, mostly silhouette and eyes. They must not be confusable at a glance.

**Diver head-cam clip:** built in engine like the existing flashbacks (camera, mask-frame overlay, bubbles, torch, timecode, film damage), film data not C++. Both versions (official, restored) from the same data with the cut seconds removed/added.

**Audio:** reuse the 13 sounds. If something essential is missing (predator sting, diver breathing), note it in the report as a request; do not call any paid API.

## Not in this brief
Stages 3–4, MULTI-CAM, twist A and C, the ending timeline, Windows build, video.

## Rules (same as brief 03)
- The user works on this Mac: never open a visible Unreal window or steal focus; every UE process with `taskpolicy -b`; visual checks offscreen; look at the captures yourself.
- Commit small and often, push to `main`; `git diff --cached | ~/bin/ioc-scan.sh` before each push; commits under the user's git identity only, no AI attribution lines; never commit CLAUDE.md, .claude/, .mcp.json, secrets. Factual dated DEVLOG.md entries (English, never mention any review council). Log new assets in ASSETS.md.
- No paid API calls. At most one fresh-context verifier subagent at the end. Ground every claim in a tool result; say plainly what is unverified. If the usage limit hits, commit what runs first. Time cap about 5 hours of work.

## Done means
1. Offscreen and packaged scripted runs through stage 1 **and** stage 2 to a stage-2 end card, with captures of: the official and restored diver clip, GRIPPER built, each puzzle before/after, one predator escape and one predator catch (TAKE n), each fail reason.
2. `docs/stage2-report.md`: what works, each puzzle and how a newcomer learns it, predator timing numbers, new English lines (all of them), what is unverified, what you'd cut first.
3. Everything committed and pushed; exact play commands (including `-MCShot=` to start at stage 2).
