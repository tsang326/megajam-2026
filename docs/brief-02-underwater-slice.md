# Brief 02 – Deep Cut: underwater vertical slice (for a Claude Fable session)

> APPROVED by the user on 2026-11-12 ~20:20 VN. `docs/prototype-report.md` (brief 01) exists. Commit this brief with your first commit.

## Why this exists
Solo entry in the 2026 Epic AI MegaJam (theme "Director's Cut", Unreal Engine, deadline 16 Nov 08:59 Vietnam time). Judged on 7 criteria × 5 points: ambition/innovation, use of theme, fun, gameplay, visuals, level design, audio. Judges get a packaged Windows build, a 30–60 s video and the itch page; nobody knows how long they play, so **the theme must be readable in the first minute of play**.

Brief 01 proved the match-cut rule on land (greybox, walking). This brief turns it into **the first playable minute of the real game**, underwater, end to end, so we can judge feel and look before building the rest.

## The game (decided by the user)
**Deep Cut.** Fathom Dynamics sent a crew of diving robots to the sea floor. Its official film says: "Exploration mission successful. Unit 7 defective, decommissioned." – but the film has visible holes: black gaps stamped **[SCENE REMOVED]**. The player is a small, battered diving robot (in-game it is **Unit 0**; outside the game call it only "a diving robot" – its identity is the final twist). It dives through fixed survey-camera shots, moving between them with **match cuts**. Each stage holds one of Unit 7's parts: it grants an ability needed to go on, and lets the player **cut a removed scene back into the reel**. The player rescues the lost crew on the way. At the end the restored film is the Director's Cut, and the player chooses what the final cut shows.

Full design log: `../IDEAS.md` (Vietnamese, Decisions 1–8). Story reveals (B → A → C) are **not** in this slice; keep them out of all text.

## Scope of this slice (only this)
1. **Opening film (~20–30 s, in-engine):** Fathom's official film playing in a simple film-player UI: a timeline with a playhead, timecode, 3–4 short shots of the crew launching (greybox robots fine), the line "EXPLORATION MISSION SUCCESSFUL. UNIT 7 – DEFECTIVE, DECOMMISSIONED." and **two or three black gaps** on the timeline stamped "[SCENE REMOVED] – REMOVED BY: ________". The film then "drops" the player into the first shot. Skippable with any key after first view.
2. **Reel bar HUD during play:** a thin film strip at the top showing the same timeline; gaps visible; filled gaps show a thumbnail. Must not cover the play area (lives in the letterbox bar if possible).
3. **Survey-camera framing:** every gameplay shot is framed as Fathom footage: small "FATHOM CAM 0X ● REC" tag and running timecode in a corner. It must never cover the ghost overlay or the robot.
4. **Underwater match cut** (see rule below), with a failed take shown as a clapper card "TAKE 2", "TAKE 3"… (replaces "NG") and an instant retake.
5. **Stage 1 (5–7 shots):** teaches (a) a plain cut, (b) a cut that only works from one spot because rock blocks the rest, (c) a timing cut (something moving in shot B, e.g. a drifting wreck plate or a current). Contains **one part** and **one rescue**.
6. **The part:** picking it up gives the **lamp** ability (toggle a headlamp beam; in a dark shot only lit areas are valid cut targets – define the rule simply and tell me what you chose). Then the **restore-by-cut**: a faded frame of the removed scene appears as a ghost (same overlay tech); the player lines Unit 0 up with the robot's position in that frame and presses CUT; on success the scene (a 10–15 s in-engine flashback, old-film look: grain, gate weave, timecode) plays and fills the first gap in the reel. On failure: TAKE 2, try again.
7. **One rescue:** a crew robot stuck (e.g. wedged under a plate in a dark shot). Solving it uses the cut + the lamp. Rescued robot follows (simple follow, can teleport on cut). HUD "CREW 1/7" (number configurable – the crew count is not final).
8. **Look-dev:** the same stage-1 shot rendered in **three looks** (offscreen captures, 1920×1080), e.g. (A) murky green-blue with volumetric fog and god rays, (B) clear deep blue, low fog, strong headlamp, (C) stylised flat-shaded/toon. Report frame time for each on this Mac. The user picks; build the rest in look A until then.
9. **Packaging check:** make a **Windows-targeted cook path work in principle**: move the layout data out of `Content/MatchCut/shots.json` into something the packager ships (a cooked DataAsset/DataTable, or a dedicated folder staged as UFS – see `docs/notes-prototype.md`, Packaging), add the map to the cook list, and do a **Mac** package (Development) to prove the packaged build loads the shots. Windows packaging itself happens on the user's Windows PC later; leave a one-page `docs/packaging-windows.md` with the exact steps.

## Play-test finding from brief 01 (must fix in this slice)
The user played the brief-01 prototype on 2026-11-12. Movement felt smooth, but they **could not pass the first cut**: four presses of C, all NG "nothing there", and they did not understand what to do (log: `~/Library/Logs/MegaJam/MegaJam.log`). The faint 35 % ghost with no marker does not teach the rule. A judge will hit the same wall in the first minute. So in this slice:
- **Show the landing before the cut:** a silhouette of Unit 0 where it would land in shot B, green when C would succeed, red (with the reason) when it would fail. The player should never have to guess.
- **Highlight the valid landing area** inside the ghost (outline or tint), so "where can I cut to" is visible at a glance.
- **First cut cannot be failed by a newcomer:** a large target and one short line of instruction on screen (e.g. "Line up with the ghost. Press C to cut."), shown until the first successful cut. Difficulty rises from the second cut on.
- Make the ghost read as a hint (stronger or pulsing until the first cut), then settle.
- Report how a first-time player would get through shot 1 without reading anything outside the game.

## The underwater cut rule (decide and report)
- Robot keeps its **screen position and its distance to the camera** from shot A when placed in shot B (keeps on-screen size – a true match cut). Clamp that distance to the shot's allowed swim range.
- Each shot has a **swim volume** (a thin slab facing the camera, ~2.5D) so depth is readable with a fixed camera; soft walls at the frame edges.
- The cut fails (TAKE 2) if the placed point is inside rock/geometry, outside the swim volume, or off-screen. Alternative from review: pull the robot forward in front of the blocking rock instead of failing. Prototype the fail version first; if it feels unfair, try the alternative and tell me which you kept.
- Use UE's Swimming movement mode (water/physics volume) if it works headless; otherwise a simple custom 3D float. Velocity is re-expressed in the new camera's frame including pitch, not only yaw.
- **Test early:** the ghost overlay is additive at 35% and assumed a black void; underwater the void is fog-coloured and may wash out the frame. First capture: two fogged shots with the ghost on. If it washes out, change the ghost (e.g. edge/outline pass, or subtract fog colour) before building more.

## Text (English, draft – the user approves all player-facing text)
Keep every line short (B1-level English). Placeholder lines are fine; list them all in the report so the user can review.

## Project facts and rules
- Project: `/Users/os/Documents/side-gigs/itch/megajam-2026/MegaJam` (UE 5.8.4, C++ module `MegaJam`). Repo `tsang326/megajam-2026`, branch `main`. Read `docs/prototype-report.md` and `docs/notes-prototype.md` first; reuse the brief-01 code (`Source/MegaJam/MatchCut`, `Tools/cutmap.py`, script runner, `Tools/run_offscreen.sh`).
- **The user is working on this Mac. Never open a visible Unreal window or steal focus.** Every UE process with `taskpolicy -b`; visual checks offscreen (`-game -RenderOffscreen -ForceRes -ResX=1920 -ResY=1080`) and look at the captures yourself.
- Machine: M1 Pro 16 GB; shader compiles are slow. Judges' PCs are unknown; one reference Windows PC has Intel Iris Xe – keep the default look cheap enough to run there (no heavy Lumen-only dependence; measure).
- Assets: template mannequin/LevelPrototyping are fine for greybox. Any new mesh/texture/sound must be made during the jam and logged in `ASSETS.md` with tool + licence. Do **not** use Meta Muse images in the build. Simple engine-made shapes are preferred over generated assets for this slice.
- Commit small and often, push to `main`. Never commit `CLAUDE.md`, `.claude/`, `.mcp.json`, secrets. Commit messages under the user's name only, no AI attribution lines. Append factual dated entries to `DEVLOG.md` (English, never mention any review council).
- Verify against this brief with a fresh-context subagent at milestones; ground every claim in a tool result; say plainly what is unverified.
- Don't stop to ask unless destructive/irreversible, scope would change, or you need something only the user has. Time cap: about 4 hours.

## Done means
1. `-game` run (and the Mac package) where the opening film → stage 1 → part → restore-by-cut → rescue → end-of-slice card plays through with keyboard; your offscreen captures show each step, plus one failed take per cut type.
2. Three look-dev captures with frame times.
3. `docs/slice-report.md`: what works, the cut-rule choices you made and why, ghost-in-fog result, frame times, packaging result, every placeholder English line, and your honest verdict on what will break first when scaling to 4 stages.
4. Everything committed and pushed; the exact command for the user to play the slice.
