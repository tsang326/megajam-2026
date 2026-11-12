"""Creates the empty level the match-cut prototype runs in (headless editor).

    UnrealEditor-Cmd MegaJam.uproject -run=pythonscript -script=Tools/make_matchcut_map.py -nullrhi -unattended

The level holds nothing: AMatchCutGameMode spawns the shots from Content/MatchCut/shots.json.
"""
import unreal

PATH = "/Game/MatchCut/Lvl_MatchCut"

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if unreal.EditorAssetLibrary.does_asset_exist(PATH):
    ok = les.load_level(PATH)
else:
    ok = les.new_level(PATH, False)
unreal.log("MATCHCUT_MAP level ready: %s" % ok)

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
gm = unreal.load_class(None, "/Script/MegaJam.MatchCutGameMode")
unreal.log("MATCHCUT_MAP game mode class: %s" % gm)
world.get_world_settings().set_editor_property("default_game_mode", gm)
saved = les.save_current_level()
unreal.log("MATCHCUT_MAP saved: %s world=%s" % (saved, world.get_path_name()))
