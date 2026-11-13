"""Imports the jam's sounds into Content/DeepCut (headless editor). Fonts are not imported: the .ttf files in
Content/Data/Fonts are staged as they are and loaded at run time.

    UnrealEditor-Cmd MegaJam.uproject -run=pythonscript -script=Tools/import_assets.py -nullrhi -unattended

  SourceAssets/audio/*.wav  -> /Game/DeepCut/Audio/<name>   (SoundWave; amb_*, projector and mus_* loop)
"""
import os
import unreal

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
tools = unreal.AssetToolsHelpers.get_asset_tools()
LOOPS = ("amb_", "projector", "mus_")


def run(src_dir, dest):
    tasks = []
    for name in sorted(os.listdir(src_dir)):
        if name.startswith(".") or name.endswith(".txt"):
            continue
        t = unreal.AssetImportTask()
        t.filename = os.path.join(src_dir, name)
        t.destination_path = dest
        t.destination_name = os.path.splitext(name)[0].replace("-", "_")
        t.automated = True
        t.replace_existing = True
        t.save = True
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    for t in tasks:
        unreal.log("DEEPCUT_IMPORT %s -> %s" % (os.path.basename(t.filename), list(t.imported_object_paths)))
    return tasks


for t in run(os.path.join(ROOT, "SourceAssets/audio"), "/Game/DeepCut/Audio"):
    for p in t.imported_object_paths:
        w = unreal.load_asset(p)
        if isinstance(w, unreal.SoundWave):
            name = w.get_name()
            w.set_editor_property("looping", name.startswith(LOOPS))
            ok = unreal.EditorAssetLibrary.save_loaded_asset(w)
            unreal.log("DEEPCUT_SOUND %s loop=%s duration=%.2f saved=%s" % (name, name.startswith(LOOPS), w.get_editor_property("duration"), ok))

unreal.log("DEEPCUT_IMPORT done")
