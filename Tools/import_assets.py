"""Imports the jam's sounds into Content/DeepCut (headless editor). Fonts are not imported: the .ttf files in
Content/Data/Fonts are staged as they are and loaded at run time.

    UnrealEditor-Cmd MegaJam.uproject -run=pythonscript -script=Tools/import_assets.py -nullrhi -unattended

  SourceAssets/audio/*.wav  -> /Game/DeepCut/Audio/<name>   (SoundWave; amb_*, projector and mus_* loop)

Models (brief 05): every entry of a manifest.json is imported to /Game/DeepCut/Art/<id>/ and listed in a small JSON
the game reads at start (id -> meshes). The game uses a mesh when its id is listed and its own shapes when not.

    Tools/import_art.sh                      ArtDrop/manifest.json            -> Content/Data/art.json
    Tools/import_art.sh SourceAssets/art_test art_test.json                   -> Content/Data/art_test.json (pipeline test)

  manifest entry: { "id": "hero" | "plate" | "solid:rock" | ..., "file": "x.glb", "yaw": degrees (optional) }
  Several entries with one id make a set (rocks). "<file stem>_emissive.png" next to the GLB is imported too and
  wired into the material's emissive slot when the material has one.
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


# turn about the up axis so the model faces +X (forward for robots); used when the manifest gives no "yaw"
YAW = {"hero": -90, "crew": -90}


def allow_instancing():
    """The water flecks are instanced meshes: the two plain materials must be compiled for that (brief 05, C4)."""
    mel = unreal.MaterialEditingLibrary
    for name in ("M_DC_Unlit", "M_DC_Lit"):
        mat = unreal.EditorAssetLibrary.load_asset("/Game/DeepCut/" + name)
        if not mat:
            continue
        try:
            mat.set_editor_property("used_with_instanced_static_meshes", True)
        except Exception as err:  # the property name is the engine's; say so if it ever changes
            unreal.log_warning("DEEPCUT_MAT %s: %s" % (name, err))
            continue
        mel.recompile_material(mat)
        unreal.log("DEEPCUT_MAT %s instanced meshes on, saved: %s" % (name, unreal.EditorAssetLibrary.save_loaded_asset(mat)))


def import_art(src_dir, out_name):
    import json
    eal, mel = unreal.EditorAssetLibrary, unreal.MaterialEditingLibrary
    manifest = json.load(open(os.path.join(src_dir, "manifest.json")))
    out_path = os.path.join(ROOT, "Content", "Data", out_name)
    art = {}
    count = {}
    for entry in manifest:
        aid, src = entry["id"], os.path.join(src_dir, entry["file"])
        if not os.path.isfile(src):
            unreal.log_warning("DEEPCUT_ART %s: file missing: %s" % (aid, src))
            continue
        count[aid] = count.get(aid, 0) + 1
        safe = "".join(c if c.isalnum() else "_" for c in aid) + ("_%d" % count[aid] if count[aid] > 1 else "")
        dest = os.environ.get("DC_ART_DEST", "/Game/DeepCut/Art") + "/" + safe
        t = unreal.AssetImportTask()
        t.filename, t.destination_path, t.automated, t.replace_existing, t.save = src, dest, True, True, True
        tools.import_asset_tasks([t])
        assets = [a.split(".")[0] for a in eal.list_assets(dest, recursive=True, include_folder=False)]
        meshes = [a for a in assets if isinstance(unreal.load_asset(a), unreal.StaticMesh)]
        if not meshes:
            unreal.log_warning("DEEPCUT_ART %s: no static mesh came out of %s (assets: %s)" % (aid, entry["file"], assets))
            continue
        # one mesh per entry: the one with the most triangles
        meshes.sort(key=lambda a: -unreal.load_asset(a).get_num_triangles(0))
        mesh = unreal.load_asset(meshes[0])
        box = mesh.get_bounding_box()
        size = box.max - box.min
        emissive = os.path.splitext(src)[0] + "_emissive.png"
        wired = "none"
        if os.path.isfile(emissive):
            et = unreal.AssetImportTask()
            et.filename, et.destination_path, et.automated, et.replace_existing, et.save = emissive, dest, True, True, True
            tools.import_asset_tasks([et])
            tex = unreal.load_asset(dest + "/" + os.path.splitext(os.path.basename(emissive))[0])
            wired = "texture imported, no emissive slot found"
            for a in assets:
                mi = unreal.load_asset(a)
                if isinstance(mi, unreal.MaterialInstanceConstant) and tex:
                    names = [str(n) for n in mel.get_texture_parameter_names(mi)]
                    slot = next((n for n in names if "emissive" in n.lower()), None)
                    unreal.log("DEEPCUT_ART %s material %s texture params %s" % (aid, mi.get_name(), names))
                    if slot:
                        mel.set_material_instance_texture_parameter_value(mi, slot, tex)
                        for v in [str(n) for n in mel.get_vector_parameter_names(mi)]:
                            if "emissive" in v.lower():
                                mel.set_material_instance_vector_parameter_value(mi, v, unreal.LinearColor(1, 1, 1, 1))
                        for v in [str(n) for n in mel.get_scalar_parameter_names(mi)]:
                            if "emissive" in v.lower():
                                mel.set_material_instance_scalar_parameter_value(mi, v, 1.0)
                        mel.update_material_instance(mi)
                        eal.save_loaded_asset(mi)
                        wired = "wired into %s.%s" % (mi.get_name(), slot)
        art.setdefault(aid, []).append({"mesh": mesh.get_path_name(), "yaw": entry.get("yaw", YAW.get(aid, 0)), "file": entry["file"], "triangles": mesh.get_num_triangles(0),
                                        "sizeCm": [round(size.x), round(size.y), round(size.z)], "emissive": wired})
        unreal.log("DEEPCUT_ART %s <- %s: %s, %d triangles, %d material slots, size %.0f x %.0f x %.0f cm, emissive: %s" % (
            aid, entry["file"], mesh.get_path_name(), mesh.get_num_triangles(0), len(mesh.static_materials), size.x, size.y, size.z, wired))
    json.dump(art, open(out_path, "w"), indent=1)
    unreal.log("DEEPCUT_ART wrote %s: %d ids" % (out_path, len(art)))


ART_DIR = os.environ.get("DC_ART_DIR", "")
if ART_DIR:
    allow_instancing()
    import_art(ART_DIR if os.path.isabs(ART_DIR) else os.path.join(ROOT, ART_DIR), os.environ.get("DC_ART_OUT", "art.json"))

for t in ([] if ART_DIR else run(os.path.join(ROOT, "SourceAssets/audio"), "/Game/DeepCut/Audio")):
    for p in t.imported_object_paths:
        w = unreal.load_asset(p)
        if isinstance(w, unreal.SoundWave):
            name = w.get_name()
            w.set_editor_property("looping", name.startswith(LOOPS))
            ok = unreal.EditorAssetLibrary.save_loaded_asset(w)
            unreal.log("DEEPCUT_SOUND %s loop=%s duration=%.2f saved=%s" % (name, name.startswith(LOOPS), w.get_editor_property("duration"), ok))

unreal.log("DEEPCUT_IMPORT done")
