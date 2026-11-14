"""Creates M_DC_Sand: the sea floor of the 3D game (headless editor).

    UnrealEditor-Cmd MegaJam.uproject -run=pythonscript -script=Tools/make_sand_material.py -nullrhi -unattended

Sand and silt without a texture: the colour comes from two layers of gradient noise in world space (broad patches of
silt, fine grain on top), so it never repeats however large the floor is. Vector param Color multiplies the result.
"""
import unreal

DIR = "/Game/DeepCut"
NAME = "M_DC_Sand"
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

path = "%s/%s" % (DIR, NAME)
if unreal.EditorAssetLibrary.does_asset_exist(path):
    unreal.EditorAssetLibrary.delete_asset(path)
m = tools.create_asset(NAME, DIR, unreal.Material, unreal.MaterialFactoryNew())


def node(cls, x, y):
    return mel.create_material_expression(m, cls, x, y)


def vec3(r, g, b, x, y):
    e = node(unreal.MaterialExpressionConstant3Vector, x, y)
    e.set_editor_property("constant", unreal.LinearColor(r, g, b, 1.0))
    return e


def const(v, x, y):
    e = node(unreal.MaterialExpressionConstant, x, y)
    e.set_editor_property("r", v)
    return e


def noise(scale, levels, y):
    # with nothing wired to Position the node uses the world position
    n = node(unreal.MaterialExpressionNoise, -900, y)
    n.set_editor_property("noise_function", unreal.NoiseFunction.NOISEFUNCTION_GRADIENT_TEX)
    n.set_editor_property("quality", 1)
    n.set_editor_property("levels", levels)
    n.set_editor_property("scale", scale)
    n.set_editor_property("output_min", 0.0)
    n.set_editor_property("output_max", 1.0)
    n.set_editor_property("turbulence", False)
    return n


def binary(cls, a, b, x, y):
    e = node(cls, x, y)
    mel.connect_material_expressions(a, "", e, "A")
    mel.connect_material_expressions(b, "", e, "B")
    return e


broad = noise(0.0011, 4, -300)
fine = noise(0.035, 2, 0)
mix = binary(unreal.MaterialExpressionAdd,
             binary(unreal.MaterialExpressionMultiply, broad, const(0.72, -900, -200), -650, -300),
             binary(unreal.MaterialExpressionMultiply, fine, const(0.28, -900, 100), -650, 0), -450, -150)
tone = node(unreal.MaterialExpressionLinearInterpolate, -200, -150)
mel.connect_material_expressions(vec3(0.105, 0.10, 0.088, -450, -350), "", tone, "A")
mel.connect_material_expressions(vec3(0.30, 0.285, 0.235, -450, 50), "", tone, "B")
mel.connect_material_expressions(mix, "", tone, "Alpha")
tint = node(unreal.MaterialExpressionVectorParameter, -200, 150)
tint.set_editor_property("parameter_name", "Color")
tint.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
out = binary(unreal.MaterialExpressionMultiply, tone, tint, 50, -150)
mel.connect_material_property(out, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(const(1.0, 50, 100), "", unreal.MaterialProperty.MP_ROUGHNESS)
mel.connect_material_property(const(0.05, 50, 200), "", unreal.MaterialProperty.MP_SPECULAR)

mel.recompile_material(m)
ok = unreal.EditorAssetLibrary.save_loaded_asset(m)
unreal.log("DEEPCUT_MAT saved %s: %s" % (m.get_path_name(), ok))
