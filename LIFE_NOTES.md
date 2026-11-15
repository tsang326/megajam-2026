# How the animals live (Deep Cut)

What each species does in the game and what that is based on. Code: `Source/MegaJam/MatchCut/DCDoc.cpp`,
`DCLife::Habits` (how it moves) and the table in `FDCDoc::SpawnAmbient` (where, how high over the floor, how fast).
Sources were looked up on 2026-11-15. Where a line says "not looked up", the behaviour is common knowledge that I did
not check against a source that day.

The game is not a simulation. The sea here is one place with every depth zone in it, so animals that live 800 m apart
in life are neighbours. What the notes fix is how each one holds itself and moves.

## Ways of living (habits)

| Habit | What it looks like |
|---|---|
| Cruise | a wide loop that is not a circle, never at one speed, banks into turns, tips with climbs |
| School | one path, each fish its own place in the group; the group opens when the diver or a hunter comes close |
| Hover | hangs in the water at one of six places, never quite still, then darts to the next |
| Perch | sits on the floor or on top of a rock, then jets in an arc to the next seat |
| Glide | lies on the sand, lifts off, glides low, settles again |
| Crawl | walks the floor, stops, walks; legs stop when it stops |
| Drift | carried slowly, turning about itself, rising a little with each pulse of its bell |
| Hang | upright, head up, in open water; sinks and rises slowly |
| Far | passes far off in the haze |

Hunters (shark, hammerhead, barracuda school, dolphin pod): each one's loop is laid at the start so that it touches the
nearest school of prey fish at their height, and it is fastest there. Prey (lanternfish, hatchetfish, parrotfish, krill,
vent shrimp, squid) move away from a hunter and from the diver.

## Species

| Species | In the game | Based on | Source |
|---|---|---|---|
| Oarfish | Hang: upright, head up, body straight, the fin along its back ripples; about 9-11 m over the floor | Seen from ROVs in a vertical ("columnar") posture, tail down, moving by waves of the dorsal fin with the body held straight (amiiform); it backed away tail first | [Wikipedia: Giant oarfish](https://en.wikipedia.org/wiki/Giant_oarfish), [Practical Fishkeeping on the 2010 ROV film](https://www.practicalfishkeeping.co.uk/fishkeeping-news/oarfish-captured-on-film-for-first-time/) |
| Squid | Hover, 0.7-2 m over the bottom, three loose animals, short jets | Longfin squid school by day and are also known to rest on the seabed; catches change between day and night because they move up at night. Many other squid do school in midwater: this is the inshore, daytime picture | [NOAA FishWatch, longfin squid (archived)](https://webarchive.library.unt.edu/eot2008/20090116031857/http://www.nmfs.noaa.gov/fishwatch/species/longfin_squid.htm) |
| Cuttlefish | Hover, 0.5-1.7 m over the bottom, backs away from the diver | not looked up |  |
| Octopus | Perch: sits on the floor or on a rock, jets to the next seat now and then | Octopuses are bottom animals (benthic) | [Frontiers in Marine Science 2022 review](https://www.frontiersin.org/journals/marine-science/articles/10.3389/fmars.2022.909192/pdf) |
| Nautilus | Hover along the reef, 1.5-5 m up, slow | Moves by jet, mostly shell first; described as living up and down the reef face rather than on the bottom | [Nautilus magazine](https://nautil.us/the-deep-secrets-of-the-nautilus-1279692), [Nature 1993 letter](https://www.nature.com/articles/362626a0.pdf) |
| Vampire squid | Drift in midwater | not looked up (my search returned nothing on it) |  |
| Giant squid | Cruise, deep, slow, in surges | not looked up |  |
| Stingray | Glide: lies on the sand, lifts off, glides low, settles | Bottom-living rays wait in or on sand and mud and swim close over the bottom | [private-scuba.com stingrays](https://www.private-scuba.com/sea-life/marine/vertebrates/rays/stingrays/index.html), [Mental Floss](https://www.mentalfloss.com/animals/fish/10-stunning-facts-about-stingrays) |
| Moray | Perch: keeps to a small patch, long rests | not looked up |  |
| Shark | Cruise low (2.6-4.6 m), slow, wide; rises through the hatchetfish school | not looked up |  |
| Hammerhead | Cruise, 4.5-8 m | not looked up |  |
| Goblin shark | Cruise low and slow | Body of a slow, sluggish swimmer; feeds near the floor and above it | [US Fish & Wildlife Service](https://fws.gov/story/goblin-sharks) |
| Frilled shark | Cruise close over the bottom, slow | not confirmed: my search found nothing on how it swims |  |
| Barracuda | School of four, slow | not confirmed: my search found nothing |  |
| Dolphin | School (a pod of three), high, fast, rising and diving | not looked up |  |
| Sea turtle | Cruise, midwater | not looked up |  |
| Whale, sunfish (far) | Far: pass in the haze | unchanged |  |
| Grouper | Hover near the bottom by the wreck, long rests | not confirmed: my search found nothing |  |
| Coelacanth | Hover near the bottom; some rests it tips nose down and hangs ("headstand") | "Headstand drift hunting", snout down for up to two minutes; body rigid, fins sculling, very slow | [Field & Stream](https://www.fieldandstream.com/conservation/coelacanth-hunt-upside-down-on-heads/), [Animal Diversity Web](https://animaldiversity.org/accounts/Latimeria_chalumnae) |
| Barreleye | Hover in midwater, long rests | Spends most of its time drifting in place, fins holding it steady | [MBARI barreleye](https://www.mbari.org/animal/barreleye/) |
| Gulper, dragonfish, viperfish | Hover in midwater, short darts | not looked up |  |
| Seahorse, sea dragon | Hover, upright, almost on the spot, low in the weed | Poor swimmers; hold on with the tail; the dorsal fin does the swimming | [PADI](https://blog.padi.com/spectacular-seahorses-10-things-to-know-about-these-tiny-undersea-wonders/), [AIP Scilight](https://www.aip.org/scilights/exploring-the-unique-swimming-dynamics-of-seahorses) |
| Lanternfish | School of twelve in the dark trench | Deep by day, up at night; some species school | [Wikipedia: Lanternfish](https://en.wikipedia.org/wiki/Lanternfish) |
| Hatchetfish | School of ten in midwater | Mesopelagic; no source found for schooling |  |
| Parrotfish | School of five, low over the reef | not looked up |  |
| Krill, vent shrimp | School (a swarm); the shrimp close to the vents | not looked up |  |
| Jellies, siphonophore, sea angel | Drift | unchanged in kind; now rise with each pulse |  |
| Crab, lobster, isopod, yeti crab | Crawl with stops | not looked up |  |
| Sea pig, nudibranch, brittle star, sea toad | Crawl, slower, fewer stops | not looked up |  |

The six shot-list animals (manta, shoal, anglerfish, jellyfish bloom, the giant, the crewmate) and the legend of the
abyss move as before.
