# Brief 07 report: the deep-sea documentary

Working notes, one section per checkpoint. The final summary is at the end once checkpoint 4 is done.

## Checkpoints 1 and 2 (2026-11-14, ~06:30)

What runs (editor build, offscreen script runs, captures looked at):

- Camera mode: hold right mouse (or C toggles). Viewfinder with thirds, a cross, lock brackets on the subject, focal length and focus distance, slower swimming, wheel (or 1 / 2) zooms. Depth of field while the camera is up.
- Recording: left mouse (or E) starts a 3 second take with a meter and a red frame.
- Scoring: each take gets 0 to 3 stars and one named reason (`Nothing in frame`, `Too far`, `Subject lost`, `Subject cut off`, `Missed the moment`, `Too dark. It needs a lamp.`, `Too shaky`, `Off centre`, `A perfect take`). One star or more crosses the shot off. Up to three takes are kept per shot.
- World, built in code from the `scene:*` models only: arch, wreck, station, vents, a roofed dark trench, a reef, a clearing with a light shaft, rim rocks. The old stylised props are not used.
- Six subjects, each a function of the clock so a take can be played again: manta circling over the wreck, a shoal of 130 instanced fish that splits when you swim through it, the anglerfish in the trench (lit only by the headlamp), seven jellyfish rising in the clearing, the giant turtle passing overhead every 120 s, the crewmate (a second diver) who waves when you are near.
- Headlamp lies on the wreck; without it the anglerfish take is refused as too dark. A long lens lies on the reef.
- Ambient life from the new creature models (squid, nautilus, crab, lantern fish, isopod, whale far out, shark, eel): 30 creatures on loops.

Test scripts: `Content/Data/tests/d1_camera.txt`, `d2_dbg.txt`, `d3_film.txt`.

Captures: `docs/captures/brief07/cp2_shooting.jpg`, `cp3_cutting_room_title.jpg`, `cp3_film.jpg`.

Known problems at this point:

- Frame time 48 ms average in the full run with the 30 ambient creatures (30 to 35 ms before them). To be cut down next.
- With frames that slow the test script ran ahead of the game: the jellyfish take ended after the script had moved on, so that run shows 5 of 6 shots and no rating card. The cutting room, the title card, film playback and the credits are in the captures.
- Hard rectangular shadows on the sea floor, weak light shafts, flat floor: checkpoint 4.
- Mouse and gamepad feel have not been tried by hand.
