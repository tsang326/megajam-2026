"""Creates M_DC_Caustics: a light function for the key light of the 3D game (headless editor).

    UnrealEditor-Cmd MegaJam.uproject -run=pythonscript -script=Tools/make_caustics_material.py -nullrhi -unattended

The light is multiplied by a slow net of bright lines (two layers of cell noise in world space, drifting against
each other), the way sunlight lands on a sea floor. No texture: the pattern is computed.
Scalar params: Scale (cells per cm), Speed, Low (light between the lines), High (light on the lines), Sharp.
"""
import unreal

DIR = "/Game/DeepCut"
NAME = "M_DC_Caustics"
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

path = "%s/%s" % (DIR, NAME)
if unreal.EditorAssetLibrary.does_asset_exist(path):
    unreal.EditorAssetLibrary.delete_asset(path)
m = tools.create_asset(NAME, DIR, unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("material_domain", unreal.MaterialDomain.MD_LIGHT_FUNCTION)


def node(cls, x, y):
    return mel.create_material_expression(m, cls, x, y)


def scalar(name, value, y):
    e = node(unreal.MaterialExpressionScalarParameter, -1700, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", value)
    return e


def vec3(r, g, b, x, y):
    e = node(unreal.MaterialExpressionConstant3Vector, x, y)
    e.set_editor_property("constant", unreal.LinearColor(r, g, b, 1.0))
    return e


def binary(cls, a, b, x, y):
    e = node(cls, x, y)
    mel.connect_material_expressions(a, "", e, "A")
    mel.connect_material_expressions(b, "", e, "B")
    return e


scale = scalar("Scale", 0.0042, 0)
speed = scalar("Speed", 14.0, 100)
low = scalar("Low", 0.55, 200)
high = scalar("High", 1.9, 300)
sharp = scalar("Sharp", 3.0, 400)

pos = node(unreal.MaterialExpressionWorldPosition, -1700, -200)
time = node(unreal.MaterialExpressionTime, -1700, -100)
drift = binary(unreal.MaterialExpressionMultiply, time, speed, -1450, -100)


def layer(direction, rel_scale, y):
    move = binary(unreal.MaterialExpressionMultiply, drift, vec3(*direction, -1450, y + 60), -1250, y)
    at = binary(unreal.MaterialExpressionAdd, pos, move, -1050, y)
    at = binary(unreal.MaterialExpressionMultiply, at, scale, -850, y)
    k = node(unreal.MaterialExpressionConstant, -850, y + 60)
    k.set_editor_property("r", rel_scale)
    at = binary(unreal.MaterialExpressionMultiply, at, k, -650, y)
    n = node(unreal.MaterialExpressionNoise, -450, y)
    n.set_editor_property("noise_function", unreal.NoiseFunction.NOISEFUNCTION_VORONOI_ALU)
    n.set_editor_property("quality", 1)
    n.set_editor_property("levels", 1)
    n.set_editor_property("scale", 1.0)
    n.set_editor_property("output_min", 0.0)
    n.set_editor_property("output_max", 1.0)
    n.set_editor_property("turbulence", False)
    mel.connect_material_expressions(at, "", n, "Position")
    return n


a = layer((1.0, 0.35, 0.6), 1.0, -400)
b = layer((-0.5, 0.9, -0.4), 1.37, -700)
# cell noise is 0 in the middle of a cell and near 1 on its border: where both layers are near a border, a bright line
net = binary(unreal.MaterialExpressionMultiply, a, b, -200, -550)
lines = node(unreal.MaterialExpressionPower, 0, -550)
mel.connect_material_expressions(net, "", lines, "Base")
mel.connect_material_expressions(sharp, "", lines, "Exp")
sat = node(unreal.MaterialExpressionSaturate, 200, -550)
mel.connect_material_expressions(lines, "", sat, "")
out = node(unreal.MaterialExpressionLinearInterpolate, 400, -550)
mel.connect_material_expressions(low, "", out, "A")
mel.connect_material_expressions(high, "", out, "B")
mel.connect_material_expressions(sat, "", out, "Alpha")
mel.connect_material_property(out, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

mel.recompile_material(m)
ok = unreal.EditorAssetLibrary.save_loaded_asset(m)
unreal.log("DEEPCUT_MAT saved %s: %s" % (m.get_path_name(), ok))
