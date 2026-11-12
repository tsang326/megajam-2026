# Assets log

One line per asset: file · tool · prompt / source · created (VN time) · job/request id or history link.

| File | Tool | Prompt / source | Created | Job / history |
|---|---|---|---|---|
| Content/Characters, Content/LevelPrototyping, Content/Input, Content/ThirdPerson, Content/Variant_* | Epic Third Person C++ template (UE 5.8) | Sponsor starter template | 2026-11-12 ~15:20 | – |
| Content/MatchCut/Lvl_MatchCut.umap | Unreal headless Python (`Tools/make_matchcut_map.py`) | Empty level that only selects the prototype game mode | 2026-11-12 ~16:25 | – |
| Content/MatchCut/shots.json | Hand-written with Claude Code, checked with `Tools/cutmap.py` | Greybox shot layout for the match-cut prototype | 2026-11-12 ~16:00 | – |
| docs/captures/*.png | Offscreen game captures (`Tools/run_offscreen.sh`), downscaled | Evidence for the prototype report, not game content | 2026-11-12 ~18:35 | – |
| Content/DeepCut/M_DC_Lit, M_DC_Unlit, M_DC_Toon, M_DC_Add (.uasset) | Unreal headless Python (`Tools/make_deepcut_materials.py`), written with Claude Code | Four small materials built from engine nodes (colour and emissive parameters, toon bands). No textures. Own work, no third-party licence | 2026-11-12 ~20:25 | – |
| Content/Data/deepcut.json | Hand-written with Claude Code, checked with `Tools/cutmap.py` | Shot layout, looks and tuning for the Deep Cut slice. Replaces Content/MatchCut/shots.json (deleted). Own work | 2026-11-12 ~20:30 | – |
| Robots, rocks, plates, light shafts in the slice | Unreal Engine BasicShapes (cube, sphere, cylinder, cone), spawned and scaled in C++ | Engine content, Unreal Engine EULA. No models imported | 2026-11-12 ~20:30 | – |
| HUD, film player, reel strip, clapper card, old-film overlay | Drawn in code on the HUD canvas with the engine's built-in font | Own work plus engine font (Unreal Engine EULA) | 2026-11-12 ~20:40 | – |
| docs/captures/slice/*.jpg | Offscreen game captures (`Tools/run_offscreen.sh`), downscaled with `sips` | Evidence for the slice report, not game content | 2026-11-12 ~21:35 | – |
