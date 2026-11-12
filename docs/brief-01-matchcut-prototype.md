# Brief 01 – Match-cut movement prototype (for a Claude Fable session)

## Why this exists
I'm a solo developer in the 2026 Epic AI MegaJam (theme "Director's Cut", Unreal Engine required, deadline 16 Nov 08:59 Vietnam time / 15 Nov 17:59 PST). Epic judges score 7 criteria: ambition/innovation, use of theme, fun, gameplay, visuals, level design, audio.

The game: an unfinished character design – shelved before it was ever finished – travels into a real game to become one of its characters. Its way of moving between places is the **match cut** from film editing. Before I commit to that, I need to know whether the core movement rule is solid. **This brief is only the prototype that answers that question.** If it fails, I switch to a different mechanic, so do not build anything beyond the test.

## The rule to prototype
- Each "shot" is a small greybox location seen through one **fixed camera** (no player camera control).
- The player moves the character (template third-person movement, adapted to a fixed camera: input relative to the camera).
- Pressing **CUT** (keyboard C, gamepad face button) switches to the next shot's camera. The character keeps its **exact position on screen**: project the character's position to screen space from camera A, deproject that same screen point from camera B, trace into shot B and place the character on the first **tagged landing surface** hit (tag/collision channel "Cuttable"). Keep the velocity direction sensible.
- A **ghost overlay** of the next shot is visible at low opacity while in the current shot, so the player can line up. Must not show an explicit landing marker.
- If the trace misses a tagged surface (void, wall, untagged), the take fails: brief "NG" feedback, restart the shot.
- Lock the viewport to 16:9 with letterbox, so alignment is identical at any window size or DPI. This matters: judges run on unknown Windows machines.

## Content for the test (greybox only, no art)
- 3–4 shot pairs, each teaching one thing: (1) cut across a gap onto a ledge, (2) cut that only works from one spot, (3) a timing pair (a moving platform in shot B), (4) optional: a pair where cutting at the wrong spot lands you somewhere useless but safe.
- A simple test level sequence: shot 1 → 2 → 3 → 4 → "THAT'S A WRAP" text.
- Use the template mannequin and LevelPrototyping meshes already in the project.

## Project facts
- Project: `/Users/os/Documents/side-gigs/itch/megajam-2026/MegaJam` (UE 5.8.4, C++ module `MegaJam`, made from Epic's Third Person C++ template). Git repo, remote `origin` = private GitHub `tsang326/megajam-2026`, branch `main`.
- Engine: `/Users/Shared/Epic Games/UE_5.8/`. Editor target already compiles: `Engine/Build/BatchFiles/Mac/Build.sh MegaJamEditor Mac Development -project=<uproject>` (~9 min clean).
- Python editor scripting works headless: `Engine/Binaries/Mac/UnrealEditor-Cmd <uproject> -run=pythonscript -script=<file> -nullrhi -unattended` (PythonScriptPlugin enabled). Use it to build levels/actors.
- **The user is working on this Mac. Never open a visible Unreal window or steal focus.** Run every UE process with `taskpolicy -b` (background priority). For visual checks, run the game offscreen: `-game -RenderOffscreen -ForceRes -ResX=1920 -ResY=1080` plus screenshots/high-res captures, and look at the images yourself.
- Unreal MCP is installed elsewhere but untested headless; you don't need it.
- Machine: M1 Pro, 16 GB. Shader compiles are slow; plan for them.

## How to work
- Simplest thing that answers the question. No menus, no story, no audio, no art, no refactors of the template beyond what the test needs.
- Commit small and often (every 30–60 min or each working step) and `git push` to `main`. Never commit `CLAUDE.md`, `.claude/`, `.mcp.json`, secrets. Commit messages under the user's name only, no AI attribution lines.
- Append a short dated entry to `DEVLOG.md` at each milestone (English, factual, no hype). Add any new asset to `ASSETS.md`.
- Verify on a cadence with a fresh-context subagent against this brief (does the cut land where the screen position says? does letterbox hold at 1280x720, 2560x1080 and 1024x768?). Ground every claim in a tool result; say plainly what is unverified.
- Keep a notes file `docs/notes-prototype.md` for lessons and gotchas.
- I'm not watching. Don't stop to ask unless an action is destructive/irreversible, the scope would change, or you need something only I have. Don't end on "I'll…" – keep going until done or blocked. Time cap: about 3 hours of work.

## Done means
1. A Mac Development game build (or `-game` run) where all shot pairs are playable with keyboard, verified by your own offscreen captures showing at least one successful and one failed cut per pair.
2. A short report in `docs/prototype-report.md`: what works, landing edge cases found, letterbox results, frame time, and your honest verdict on whether the rule holds up for a full game, plus what would break first.
3. Everything committed and pushed.
4. Tell me the exact command to launch the build so I can play-test it for 10–15 minutes.
