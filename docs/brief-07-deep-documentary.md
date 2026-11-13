# Brief 07 – Deep Cut: the deep-sea documentary (core gameplay for the 3D world)

> APPROVED by the user 2026-11-14 ~02:05 VN: chose option 1 ("Robot quay phim tài liệu đáy biển") after "gameplay như hiện tại … chẳng đi được đến đâu". Builds on brief 06 M1 (3D swimming + camera). Brief 06 items 3–5 and M2–M4 are replaced by this brief.

## The game in one line
Unit 0 is a robot camera-diver sent to film a nature documentary in the deep. The director hands you a **shot list**; you swim the 3D world, find the creatures, frame them at the right moment and record. At the end you **cut your film** from your best takes and watch it: that is your Director's Cut.

## Deadline
Playable end to end, packaged, by **15 Nov ~12:00 VN** (submission 16 Nov 08:59). Keep it small and polished rather than big.

## Core loop
1. **Shot list** (HUD card, top-left, short): e.g. "1. The manta glides over the wreck", "2. The shoal splits around you", "3. The anglerfish's light in the trench", "4. Jellyfish rise in the light shaft", "5. The giant passes overhead", "6. Your crewmate waves" (6 shots, final count yours).
2. **Swim** freely (M1 controls). Creatures live in the world with simple, readable behaviours on loops or triggers.
3. **Camera mode:** hold right mouse = raise the camera: viewfinder overlay (frame lines, REC dot, rule-of-thirds guides, focus/zoom readout), slower movement, mouse aims. Mouse wheel = zoom (field of view) – this is the only "near/far", and it is a camera zoom, not movement.
4. **Record:** left mouse = record a take (3 s). Live meter while recording.
5. **Take score** (1–3 stars, shown right after): subject in frame and big enough, close to centre/thirds, the *action* happening during the take (the manta over the wreck, not just the manta), steady (little camera shake), in focus (distance in range). Name the reason when it is low ("Too far", "Missed the moment", "Subject cut off"). One star or more = the shot is crossed off; you may retake for more stars.
6. **Final cut:** when all shots have a take (or at an "end the shoot" option after at least 4), a simple edit screen: shots in order on a film strip, pick one take per shot (thumbnails + stars), then **play the film**: your takes played back in sequence (re-render from recorded camera transforms + creature state, or saved frames – your choice, cheapest that looks good), with title card "DEEP CUT – a film by Unit 0" and credits. Total stars → a rating card.

## Creatures and behaviours (models coming in ArtDrop, procedural motion, no rigs)
- **Manta ray:** glides a big loop that passes over the wreck every ~40 s.
- **Shoal of small fish (instanced):** schools around a reef; when Unit 0 swims into it fast, it splits and re-forms.
- **Anglerfish:** in the dark trench; only visible with the headlamp (F) – its lure glows.
- **Jellyfish (several):** slowly rise and fall in a light shaft.
- **Giant (whale or huge turtle):** passes overhead on a long timed route; a moment you must be ready for.
- **Crew robot (existing crew.glb):** waves when you are close and in front.
- Optional: the existing predator as an eel that darts out of a hole when you come near.
Motion: swim cycles via sine-wave bending (material world-position offset or bone-less mesh bending), banking on turns. Behaviours readable at a glance.

## Progression (light)
- Start with the basic camera. The **headlamp** (F) is found in the wreck – needed for the trench shot. A **zoom lens** upgrade could gate the giant shot. Keep at most 2 upgrades.
- Teach each control when first needed, one short line, as in brief 05.

## World
One connected 3D site (brief 06 idea): reef/shelf, wreck, light-shaft clearing, dark trench, open water above for the giant, the dock with the crew robot. Swim time between areas 15–30 s. Use ArtDrop models (rocks, hull, debris, vent, dock, plate, lid, cell as set dressing), uniform scale only, many instances. Fog, marine snow, caustics, light shafts from brief 05.

## HUD
Keep the film look (REC, timecode, film strip) but quiet: shot list card, small stars, viewfinder only in camera mode. Almost no text otherwise.

## Order and checkpoints (push at each, short notes in docs/brief07-report.md)
1. After M1: camera mode + record + take scoring on one placeholder creature (a sphere that swims a loop). Push.
2. World layout + creatures with behaviours (placeholders until models land).
3. Shot list complete + upgrades + final cut screen + film playback + rating card. Scripted offscreen run start → film playback; packaged run.
4. Models from ArtDrop as they land; look pass; frame times.
If any checkpoint shows the 15 Nov noon target is at risk, say so at once with what to cut (first cuts: zoom upgrade, eel, film playback → replaced by a still-frame slideshow).

## Rules
Same as briefs 05/06 (offscreen only, `taskpolicy -b`, commit/push to main with ioc-scan, user's identity, no AI attribution, no Claude files, ArtDrop uncommitted, DEVLOG factual, no paid API calls, say what is unverified). Report: play commands, all new English lines, frame times, unverified items, what you'd cut first.
