# Brief 05 report: playtest fixes, deeper puzzles, art hook, visual pass

Written 2026-11-13 night. Everything below was run offscreen in the editor (`Tools/run_offscreen.sh`) and then in the packaged Mac build (`Tools/run_packaged.sh`, last section).

## What changed

### Part A: playtest fixes (commit 616a08d)
1. **Never stuck without being told.** The shot hint stays until the shot is solved. A failed take shows the reason and the next action on the clapper card. A C press before any mark shows "No tape mark yet." plus the shot's own hint. After 20 s without progress the thing to use pulses, with an arrow to where it goes.
2. **Things in frame and readable.** Floors raised so nothing sits under the bottom HUD bar. Loose things are safety orange with a white ring.
3. **Forgiving push.** A thing on the floor is pushed from beside it up to 150 cm above it and up to 300 cm off in depth; a PUSH label with an arrow shows on contact.
4. **Depth.** Mouse wheel and Q / Z (SPACE / SHIFT removed). A shadow ring on the floor under Unit 0. Depth only matters at depth marks; the first one says so.
5. **VENT frame and spawn point apart** in THE SHELF and THE VENT.
6. **Controls taught when first needed.** One prompt each, then the key stays in the controls line. `-MCShot=N` counts what earlier shots teach as taught.
7. Scripts `stage1`, `route_yellow`, `stage2`, `full12` updated.

Captures: `docs/captures/brief05/partA/`.

### Part B: three puzzles (commit 2b2a459)
Shot count is unchanged (11). One predator encounter was removed (THE DRIFT) to make room.

**B1. Graphic match, THE SHELF (shot 1).** Two loose things, a round one and a long one. The next shot's crack is drawn in this shot as an outline of its real shape. Only the long plate on the outline shows the tape mark.
- How a newcomer learns it: the hint says "Two loose things. One has the shape of the outline." The outline is a long rectangle, one thing is long, one is round. Pushing the round one on gives a red WRONG SHAPE label under the outline and no mark. After 20 s the long plate pulses with an arrow to the outline.
- No dead end: a thing cannot be pushed into a corner any more (it stops where Unit 0 can still get behind it), and R restores both things.
- Captures: `docs/captures/brief05/partB/B1_*.jpg` (before, stuck hint, wrong shape, right shape, FITS on the mark).

**B2. The predator as a tool, THE CURRENT (shot 8).** The crew unit is behind a shoal Unit 0 cannot swim through. The predator comes once the vent is covered. Cut away while it is outside the carry circle and it stays in the old shot, where it goes through the shoal.
- How a newcomer learns it: the hint says "A shoal hides a crew unit. Only something big can scatter it. Wait on the mark." When the predator comes the hint turns red: "Cut away while it is OUTSIDE your circle. Then it stays in this shot." After the cut the next shot says IT STAYED BEHIND and "R: back one shot. See what it did." Back in THE CURRENT the shoal flies apart (THE SHOAL IS GONE) and the hint says to carry the crew unit.
- The other half of the rule: cut while it is inside the circle and the take fails with IT CAME WITH YOU. This is the carry rule stage 4 will use.
- No dead end: the shoal stays gone after a retake; being caught only restarts the take.
- Captures: `docs/captures/brief05/partB/B2_*.jpg`.

**B3. Shuttle, THE DRIFT and THE LIFT (shots 9 and 10).** The lift needs the crew unit and a power cell. The gripper holds one thing. The cell lies in THE DRIFT and is too heavy to drift along.
- How a newcomer learns it: THE DRIFT's hint says "The lift needs the crew unit and this power cell. You hold one thing at a time." THE LIFT shows two frames (LIFT and POWER). After the first thing is in, the banner says "The lift needs one more thing." A white mark labelled BACK TO THE DRIFT is the way back.
- **Differs from the brief:** the brief says to go back with R. R is an undo (it restores the earlier shot, so the thing just delivered would go back too). The trip back is a white mark that cuts back instead. R still works as undo.
- Either order works; the script takes the crew unit first.
- Captures: `docs/captures/brief05/partB/B3_*.jpg`.

### Part C: art hook and visual pass (commits df829b8, 2b2a459)
- `Tools/import_art.sh` imports every entry of `ArtDrop/manifest.json` headless into `Content/DeepCut/Art/<id>/` and writes `Content/Data/art.json`. The game uses the model for an id when it is listed, else the code shapes. Rule sizes are unchanged; a model is scaled evenly, centred and grounded in its box. Rock, hull and debris boxes are filled by the model (stretched per axis), and a rock box takes the rock model nearest its own proportions, so rock2 (the pillar) only lands on tall boxes.
- Pipeline test: `Tools/make_test_glb.py` writes a rounded box; `-MCArt=art_test.json` shows it in game.
- All 12 ArtDrop models are imported (125 MB of assets). Code shapes stay for `solid:dark`, `socket:LIFT`, the crack in THE VENT and the two orange puzzle pieces of THE SHELF (their outline and contrast are the puzzle).
- **Hero:** replaces the robot body. Swims by code: bob, lean into the move, turn through facing the camera, bank on turns. Headlamp light re-attached by hand. **The GLB has no sub-meshes, so parts cannot be shown or hidden by stage; skipped.**
- **Visual pass, engine only** (`DCVisual.cpp`): marine snow (660 flecks) and bubbles (92) as two instanced meshes that follow the live camera; the light-shaft slats sway so the light bars on the floor move like slow caustics; colour grading per look; a shade per rock block; a rim light only Unit 0 takes. Depth fog and shafts existed before and are kept.
- Captures: `docs/captures/brief05/art/`, `docs/captures/brief05/visual/`.

## Frame times
From `FRAMETIME` log lines, editor build, offscreen, **background priority (`taskpolicy -b`) while the Mac was in use**. These are upper bounds, not what a player gets.

| Run | Shots | Average per frame |
|---|---|---|
| `full12`, look A, all models | 0, 1, 2, 3 | 28-35 ms |
| | 6, 7 | 34-37 ms (67 ms on a second visit to 7) |
| | 8, 9 | 47-57 ms |
| | 4, 5, 10 | 58-250 ms |
| `look_check`, look A | 1, 3, 7, 8, 10 | 44, 27, 35, 57, 33 ms |
| `look_check`, look C | 1, 3, 7, 8, 10 | 22, 23, 33, 29, 26 ms |
| `look_check`, look B (ran at the same time as another editor) | 1, 3, 7, 8, 10 | 115, 233, 97, 70, 52 ms |

Before the models and the water, the same kind of run gave about 26-30 ms in shots 0-3. So the pass costs a few ms where it could be compared; the large numbers in shots 4, 5 and 10 are not explained. The same shot 10 gave 33 ms in `look_check` and 250 ms in `full12`, which points at machine load rather than the scene, but that is a guess. The packaged build (last section) gave 25-34 ms in every shot in its first run and 28-110 ms in its second run of the same script, so the large numbers move with machine load, not with the shot. **60 fps is still not shown**: every run here was at background priority. It needs one run at normal priority in a window.

## New English lines (all of them, for approval)

Controls line (each appears once taught): `WASD swim` · `C cut` · `R back one shot` · `swim into things: push` · `WHEEL or Q / Z farther / closer` · `F lamp` · `E grab` · `ESC quit`

One-time prompts:
- `W A S D  or arrow keys: swim`
- `On the tape mark:  C  cuts to the next shot`
- `R: go back one shot`
- `Swim into a loose thing to push it`
- `Mouse wheel or  Q / Z: farther / closer`
- `F: lamp on / off`
- `E: grab / drop. Now you can carry heavy things`

Cues and labels: `PUSH` · `WRONG SHAPE` · `BACK TO` (followed by the shot name) · `POWER` (frame label) · `TOO CLOSE - Q` · `TOO FAR - Z`

Banners: `IT STAYED BEHIND` · `THE SHOAL IS GONE` · `IT CAME WITH YOU`

After a failed take (second line of the clapper card):
- `Swim onto the tape mark. Then press C.`
- `Go farther: wheel up or Q, until your ring fits the white ring.`
- `Come closer: wheel down or Z, until your ring fits the white ring.`
- `Wait until the mark is bright. Then press C.`
- `Press F for the lamp. Then press C.`
- `Swim into the MATCH ring. Then press C.`
- `Rock is in the way in the next shot. Move a little on the mark.`
- `Get to the tape mark and press C before it reaches you.`
- `Cut while it is outside your circle. Then it stays behind.`

Other lines:
- `Not here. Swim onto the tape mark. Then press C.`
- `No tape mark yet.` (followed by the shot hint)
- `Depth only matters at a mark with a white ring.`
- `Nothing more to do here. R: back one shot.`
- `Cut away while it is OUTSIDE your circle. Then it stays in this shot.`
- `It is still in the last shot. R: back one shot. See what it did.`
- `The lift needs one more thing.`

Shot hints:
- THE SHELF: `Two loose things. One has the shape of the outline. Push it onto the outline.` / solved: `Near things come with you through the cut. Stay close to it. Swim onto the tape mark.`
- THE VENT: `The current is too strong. The crack needs the long plate. R: back one shot.` / solved: `This mark has a depth. Make your ring the same size as the white ring.`
- THE WRECK, solved: `Two cameras. Yellow: the dock. Blue: a crew signal in the dark (F: lamp). Push the power cell along.`
- THE LEDGE: `Dark. F: lamp. Something heavy is here. Swim to it. E: grab.`
- THE CURRENT: `The vent needs the lid. No lid? R: back one shot.` / solved: `A shoal hides a crew unit. Only something big can scatter it. Wait on the mark.` / after: `The shoal is gone. Carry the crew unit to the mark: E. Then cut.`
- THE DRIFT: `The lift needs the crew unit and this power cell. You hold one thing at a time. Cut when the place is clear.`
- THE LIFT: `The lift needs a crew unit and a power cell: E drops one in. White mark: back for the other.`

## What is verified
- `stage1` offscreen: `END CARD stage=1 scenes=1/3 crew=1/7`.
- `stage2` offscreen: `END CARD stage=2 scenes=2/3 crew=2/7`, with `PREDATOR stayed behind`, `SHOAL gone`, `PREDATOR inside the carry radius ... caught`, both lift frames filled.
- `full12` offscreen after the models and the water: both end cards.
- `route_yellow` offscreen: `END CARD stage=1 scenes=1/3 crew=0/7`.
- Packaged Mac build: `full12` and `route_yellow`, same end cards (last section).
- I looked at the captures listed above myself.

## What is unverified
- **No human has played this build.** Whether a newcomer finds the three puzzles clear is my reading of the captures.
- **Frame rate at normal priority** (see above).
- THE WRECK's yellow mark moved 20 cm (x 400 to 380) late in the session: a power cell on the centre of the DOCK frame was 294 cm from the old mark, outside the 280 cm carry circle, so a player who centred it saw it stay. `route_yellow` passes with the new mark (cell stays from its start place, comes and fits from the frame). Not tried by a human.
- The hero's colours look pale in the green look and its emissive map does not show in captures. Facing was checked in captures going right and left.
- Stretched models: the debris model on long thin boxes (40 x 40 x 400 and 260 x 1150 x 50) is stretched a lot; I did not inspect each one.
- Skipping the crew unit in THE CURRENT and coming back from THE LIFT by pressing R several times was not scripted. R keeps a stack of earlier shots, so it should work.
- Stage 3 and later were not touched or run.
- `ArtDrop/` (source GLBs, 120 MB) is not committed; only the imported assets are.

## What I would cut first
1. The stretched debris and hull fills (set those ids back to code shapes by deleting their entries from `Content/Data/art.json`).
2. Bubbles (keep the snow).
3. The white back mark of the shuttle, if it confuses: put the power cell in THE LIFT's own shot and drop the second trip.

## Play commands
Human play (opens a window): `Tools/play.sh`, or with a start shot `Tools/play.sh 1600 900 -MCShot=N` (the size comes first).
- Graphic match: `-MCShot=1`
- Predator as a tool: `-MCShot=7` (grab the lid in THE LEDGE, land it on the vent, then THE CURRENT). Starting at `-MCShot=8` does not work for this puzzle: the lid is in the shot before and R has nothing to go back to.
- Shuttle: `-MCShot=9` starts in THE DRIFT with the power cell but without the crew unit, so only the cell half can be tried; for the whole puzzle start at `-MCShot=7`.
- Looks: `Tools/play.sh 1600 900 -MCLook=B` (A, B or C; one extra argument besides the size, or two).

Scripted, no window:
- `Tools/run_offscreen.sh stage1 Saved/DeepCut/stage1`
- `Tools/run_offscreen.sh stage2 Saved/DeepCut/stage2`
- `Tools/run_offscreen.sh full12 Saved/DeepCut/full12`
- `MC_EXTRA=-MCLook=B Tools/run_offscreen.sh look_check Saved/DeepCut/look_B`
- Packaged: `Tools/package_mac.sh`, then `Tools/run_packaged.sh full12 Saved/DeepCut/full12_pkg`

## Packaged run
`Tools/package_mac.sh` built twice after the last code change (BUILD SUCCESSFUL both times; the second holds the final data file). Run with `Tools/run_packaged.sh`, offscreen, background priority:

- `full12`: `END CARD stage=1 scenes=1/3 crew=1/7`, then `END CARD stage=2 scenes=2/3 crew=2/7`, 112 captures. The log shows all twelve models loaded, `PREDATOR stayed behind`, `PREDATOR inside the carry radius`, and both lift frames filled.
- `route_yellow`: `END CARD stage=1 scenes=1/3 crew=0/7`.
- Frame times, first packaged `full12` run: 25-34 ms in every shot (shots 0-10). Second run (same script, final data): 26-34 ms in shots 0-2 and 8-10, 54-110 ms in shots 3-7. The Mac was in use both times.
- Captures of the first packaged run: `docs/captures/brief05/packaged/` (19 files). They show the models, the water flecks and the three puzzles the same as in the editor.
- The package itself is at `Saved/Package/Mac/MegaJam.app` (not in git).
