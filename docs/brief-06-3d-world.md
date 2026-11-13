# Brief 06 – Deep Cut goes 3D: one connected underwater world (for a Claude Fable session)

> APPROVED by the user 2026-11-14 ~01:30 VN. User, verbatim: "gameplay hiện tại tệ quá. không làm thế giới mở thì bỏ di chuyển gần xa đi, vừa phức tạp vừa không có tác dụng giải trí gì", then "thay vì di chuyển ngang như map 2D hiện tại, thì có thể xoay nó lại làm map 3D không? … không gian thì cách di chuyển cũng khá giống đại dương", and chose **option A: one connected 3D world, camera behind the robot, free swimming in every direction**, after being told it is the riskiest option (est. 1–1.5 days of work, deadline 16 Nov 08:59 VN).

## Why
The user finds the side-view game dull: you read text, swim to a mark, press C. They want the feeling of floating through a 3D space (like zero-g), with free 3D swimming. Deadline **16 Nov 08:59 VN**; Windows packaging, video and submission still need ~half a day after this, so the 3D game must be playable end to end by **15 Nov ~12:00 VN** at the latest.

Read first: `docs/OUTLINE.md`, `docs/brief05-report.md`, `docs/playtest-notes.md`, `ASSETS.md`, `ArtDrop/manifest.json`.

## Step 0 – safety
- Stop the depth-removal task: commit it only if it already passes the four scripts, otherwise drop your uncommitted changes from it (they are your own, nothing else).
- `git tag v2d-fallback 09033c8` (or the depth-removal commit if you committed it) and push the tag. The 2D game stays our fallback submission if 3D is not ready.

## The design (keep the theme)
Theme rule unchanged: every puzzle is an act of editing (what goes in the frame, what gets cut, which version is true). No combat, no valves/levers.

1. **One connected 3D underwater world** (a sunken site: shelf, vent field, wreck, dark trench, dock, lift shaft), built from the existing ArtDrop models + engine shapes. Not huge: about 6–8 areas you can swim between in 20–40 s each.
2. **Movement:** third-person, camera behind and slightly above Unit 0, mouse look (and arrow keys), WASD swim relative to the camera, SPACE up, CTRL or C… pick keys that don't clash; momentum and drift like zero-g/water (accelerate, glide, soft stop), small boost key optional. It must *feel good*: this is what the user asked for. Camera collision so it never sits inside rock.
3. **Cuts stay the core mechanic, in 3D:** tape marks become floating **camera rigs / slates** in the world. At one, C *cuts* to the linked rig's shot elsewhere (the screen does a hard cut, the camera shows that place, Unit 0 is there). The carry rule stays: what is within the carry radius comes with you; what is outside stays. That keeps the predator-as-a-tool and shuttle ideas, and the cut is a shortcut/teleport you plan, not the only way to move.
4. **Puzzles:** rebuild 4–6 puzzles for 3D from the existing ones (plate on vent, lid on vent, power cell to dock, crew units, predator left behind, shuttle two things to the lift). Each must be readable by looking, with one short line of text at most. Remove the 2D-only graphic-match outline unless it has a natural 3D version.
5. **Progress:** keep the crew counter, headlamp (F) and gripper (E) upgrades, and the HUD look (REC, timecode, film strip) but much less text. Controls taught when first needed (same rule as brief 05).
6. **Look:** reuse the visual pass (fog, marine snow, light shafts, caustics, rim light). Fix the pale hero: its teal must read in the green water and its emissive map (`ArtDrop/hero_emissive.png`) should light the eyes/headlamp/chest. Don't stretch models non-uniformly; place several instances instead.

## Order and checkpoints (push at each)
1. **M1 (~3 h): movement + camera in one test area** with the hero and a few rocks. Push and write 5 lines in `docs/brief06-report.md` so the user can try the feel early: `Tools/play.sh` command included.
2. **M2: the whole world blocked out** with all areas, rigs linked, cut + carry rule working in 3D.
3. **M3: puzzles + upgrades + end card**, scripted run from start to end card offscreen and packaged (update the test runner for 3D: scripted inputs or a goto/teleport path is fine).
4. **M4: look pass + frame times.**

If at any checkpoint you think the 3D game cannot be playable end to end by 15 Nov ~12:00 VN, say so in the report immediately with what would be cut.

## Not in this brief
New film clips, Windows build, the submission video, a story ending beyond an end card.

## Rules (same as brief 05)
- The user works on this Mac: never open a visible Unreal window or steal focus; every UE process with `taskpolicy -b`; visual checks offscreen; look at the captures yourself.
- Commit small and often, push to `main`; `git diff --cached | ~/bin/ioc-scan.sh` before each push; commits under the user's git identity only, no AI attribution lines; never commit CLAUDE.md, .claude/, .mcp.json, secrets. `ArtDrop/` stays uncommitted. Factual dated DEVLOG.md entries (English, never mention any review council).
- No paid API calls. Ground every claim in a tool result; say plainly what is unverified. If the usage limit hits, commit what runs first.

## Done means
Scripted offscreen + packaged run start to end card; captures of each area, each puzzle before/after, the camera in tight spots; `docs/brief06-report.md` with play commands, all new English lines, frame times, unverified items, what you'd cut first.
