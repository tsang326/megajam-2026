# The base's hull (scene:ahull) wraps the room and runs under its floor. Its material is two-sided, so from inside the
# room the moon pool showed the hull's inner bottom. One-sided, its inside is not drawn and the hole looks onto the sea.
import unreal
P = "/Game/DeepCut/Art/scene_ahull/scene_ahull/Materials/model"
mi = unreal.load_asset(P)
unreal.log("HULLMAT class %s" % mi.get_class().get_name())
ov = mi.get_editor_property("base_property_overrides")
unreal.log("HULLMAT before override=%s two_sided=%s" % (ov.get_editor_property("override_two_sided"), ov.get_editor_property("two_sided")))
ov.set_editor_property("override_two_sided", True)
ov.set_editor_property("two_sided", False)
mi.set_editor_property("base_property_overrides", ov)
unreal.MaterialEditingLibrary.update_material_instance(mi)
unreal.EditorAssetLibrary.save_asset(P, only_if_is_dirty=False)
unreal.log("HULLMAT saved one-sided")
