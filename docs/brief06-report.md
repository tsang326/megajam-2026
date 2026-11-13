# Brief 06, checkpoint M1: 3D swimming and camera (2026-11-14, ~03:45 VN)

Scope after the coordinator's messages of 01:40 and 02:05: only M1 of brief 06. The cut-to-rig mechanic, the carry rule and the old puzzles are cancelled; the gameplay that follows is brief 07 (`docs/brief-07-deep-documentary.md`, notes in `docs/brief07-report.md`).

## Try it

    cd /Users/os/Documents/side-gigs/itch/megajam-2026/MegaJam
    Tools/play.sh                 # opens a 1600x900 window, 3D game
    Tools/play.sh 1920 1080
    Tools/play.sh 1600 900 -MCSens=0.08     # slower mouse (default 0.11 degrees per count)
    Tools/play.sh 1600 900 -MC2D            # the old side-view game; it is also the git tag v2d-fallback (09033c8)

Keys: mouse or arrow keys look, W A S D swim where the camera looks, SPACE up, X or Q down, SHIFT boost, `[` and `]` mouse slower and faster, I inverts the mouse's up and down, ESC quits. Gamepad: left stick swim, right stick look, triggers up and down.

## What is there

- Swimming with momentum: the keys push Unit 0 towards 5.6 m/s (9.8 m/s with boost) and it takes about a second to get there; when the keys are let go the water takes the speed away slowly (measured in the test run: 5.0 m/s falls to 1.1 m/s in 2 s). It slides along rock instead of stopping dead.
- Collision is against the models' own triangles, not boxes. A probe at start logs whether the models answer (`collision probe on the arch: HIT`).
- Camera 4.4 m behind and a little above. A ball swept from Unit 0 to the camera pulls it in at once when rock is in the way and lets it out again slowly; when it has to come closer than 95 cm the robot is hidden so its back does not fill the picture. Field of view opens from 72 to 81 degrees with boost.
- The body turns to where it is pushed, lies forward into its speed, banks in turns, bobs when it floats.
- One test area, a bowl about 110 m across made only of the ArtDrop models at uniform scale: an arch, the wreck with debris, dock, cell and a crew robot, a vent field with warm lights, a roofed gully (the tight spot for the camera), a 20 m spire, loose boulders, a rim of big rock. Six rings to swim through, as something to aim at; they are not the game.
- A current turns Unit 0 back at the edge of the bowl and 29 m up.

## The hero looked pale: cause found

The imported glTF material reads its emissive map only when the static switch `bHasEmissiveTexture` is on. The import had set the emissive colour to white without that switch, so the material added plain white to the whole body. `Tools/import_assets.py` now sets the switch; the hero was imported again (`DC_ART_ONLY=hero Tools/import_art.sh` imports one id and keeps the rest). Eyes, headlamp and chest light now glow from `hero_emissive.png` and the body shows its teal and white texture. Two lights that sat inside the (double-sided) mesh were also moved outside it. Captures (the diver): `docs/captures/brief06/f01_hero_close.png`, `f02_hero_front.png`.

At 03:40 the hero changed to the diver in a hard suit (new `ArtDrop/hero.glb`, imported the same way; the robot is not used any more). The captures show the diver; where this report says Unit 0 or robot, read: the diver. The suit needed a far stronger rim light than first set (24000, unitless) to read as white and teal in dark water.

The hero code is one mesh fitted in a box, a lamp in front of it and a rim light on the camera side, so another model (the hard suit) can replace `hero.glb` without code changes, as long as its manifest id stays `hero`.

## Film look (art direction of 02:20)

- Lumen cannot run on this Mac. The log of every run says `To use SM6 on this system, please ensure you are running Mac OS 15. Falling back to SM5` (this Mac runs macOS 27, the engine's check fails), and Lumen needs SM6. A run with Lumen asked for (`-MCLumen`) gave the same picture and the same frame time as one without. So: no Lumen here; unverified on other machines. The flag stays for a later check on Windows.
- In its place: volumetric fog lit by the key light, 150 unseen flat pieces high in the water whose swaying shadows dapple the floor and cut the light, blue water that has lost its red, a warm headlamp, bloom, a little chromatic aberration at the edges, film grain, vignette, a slightly lifted film toe.
- Depth of field belongs to the camera mode of brief 07.
- Not good yet: the shafts of light are hardly visible in the water itself, and the flat sea floor has no texture. Both are on the brief 07 look pass.

## Numbers

Editor build, offscreen 1920x1080, background priority, Mac in use (so an upper bound): `m1_swim` alone, 2959 frames, 35.6 ms average (an earlier run of the same script: 29.9 ms). Two runs at once gave 88 ms each, so only runs made alone count.

## Test

`Tools/run_offscreen.sh m1_swim Saved/DeepCut/m1` swims the area with scripted keys and writes captures. In the last run: every scripted swim target reached, 5 of 6 rings (the sixth is not on the script's path), a dive onto the arch stopped on the slab, a swim into a pillar stopped at its face, boost reached 9.3 m/s, the edge current stopped Unit 0 5 m past the edge.

## New English lines

- `MOUSE look     W A S D swim     SPACE up     X down     SHIFT boost` (shown for the first 14 s and after 10 s without input)
- `DEPTH n m   HDG n`
- `RINGS n / n`

## Unverified

- Nobody has played it by hand: mouse feel, sensitivity and whether the window takes the mouse correctly on this Mac are untested. The scripted run drives the same movement code but not the mouse.
- Gamepad input is untested.
- The packaged build has not been made with the 3D game yet.
