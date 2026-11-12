"""Creates the four small materials the Deep Cut slice uses (headless editor).

    UnrealEditor-Cmd MegaJam.uproject -run=pythonscript -script=Tools/make_deepcut_materials.py -nullrhi -unattended

  M_DC_Lit    lit, vector params Color + Emissive
  M_DC_Unlit  unlit, vector param Color
  M_DC_Toon   lit base colour + two-band emissive shading from the vertex normal; params Color, Emissive, Glow
  M_DC_Add    unlit additive translucent, vector param Color (light shafts, beacons)
"""
import unreal

DIR = "/Game/DeepCut"
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def new_material(name):
    path = "%s/%s" % (DIR, name)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    return tools.create_asset(name, DIR, unreal.Material, unreal.MaterialFactoryNew())


def vec_param(mat, name, default, y):
    e = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -600, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", unreal.LinearColor(*default))
    return e


def const(mat, value, y):
    e = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -600, y)
    e.set_editor_property("r", value)
    return e


def finish(mat):
    mel.recompile_material(mat)
    ok = unreal.EditorAssetLibrary.save_loaded_asset(mat)
    unreal.log("DEEPCUT_MAT saved %s: %s" % (mat.get_path_name(), ok))


# lit
m = new_material("M_DC_Lit")
c = vec_param(m, "Color", (0.3, 0.3, 0.3, 1.0), 0)
e = vec_param(m, "Emissive", (0.0, 0.0, 0.0, 1.0), 250)
mel.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(e, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(const(m, 0.9, 500), "", unreal.MaterialProperty.MP_ROUGHNESS)
mel.connect_material_property(const(m, 0.0, 600), "", unreal.MaterialProperty.MP_SPECULAR)
finish(m)

# unlit
m = new_material("M_DC_Unlit")
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
c = vec_param(m, "Color", (1.0, 1.0, 1.0, 1.0), 0)
mel.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
finish(m)

# toon: lit base colour (so a base-colour capture still sees it) plus a banded emissive that does the shading.
# band = floor((dot(N, L) * 0.5 + 0.5) * 3) / 2, emissive = Color * lerp(0.35, 1.0, band) * Glow + Emissive
m = new_material("M_DC_Toon")
c = vec_param(m, "Color", (1.0, 1.0, 1.0, 1.0), 0)
em = vec_param(m, "Emissive", (0.0, 0.0, 0.0, 1.0), 700)
glow = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -600, 550)
glow.set_editor_property("parameter_name", "Glow")
glow.set_editor_property("default_value", 1.0)
n = mel.create_material_expression(m, unreal.MaterialExpressionVertexNormalWS, -1400, 300)
l = mel.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -1400, 450)
l.set_editor_property("constant", unreal.LinearColor(0.25, -0.35, 0.90, 0.0))
d = mel.create_material_expression(m, unreal.MaterialExpressionDotProduct, -1200, 350)
mel.connect_material_expressions(n, "", d, "A")
mel.connect_material_expressions(l, "", d, "B")
half = mel.create_material_expression(m, unreal.MaterialExpressionConstantBiasScale, -1050, 350)
half.set_editor_property("bias", 1.0)
half.set_editor_property("scale", 0.5)
mel.connect_material_expressions(d, "", half, "")
mul3 = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -900, 350)
mul3.set_editor_property("const_b", 2.999)
mel.connect_material_expressions(half, "", mul3, "A")
fl = mel.create_material_expression(m, unreal.MaterialExpressionFloor, -750, 350)
mel.connect_material_expressions(mul3, "", fl, "")
mulh = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -600, 350)
mulh.set_editor_property("const_b", 0.5)
mel.connect_material_expressions(fl, "", mulh, "A")
lerp = mel.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -450, 350)
lerp.set_editor_property("const_a", 0.35)
lerp.set_editor_property("const_b", 1.0)
mel.connect_material_expressions(mulh, "", lerp, "Alpha")
shade = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -250, 100)
mel.connect_material_expressions(c, "", shade, "A")
mel.connect_material_expressions(lerp, "", shade, "B")
dim = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -100, 250)
mel.connect_material_expressions(shade, "", dim, "A")
mel.connect_material_expressions(glow, "", dim, "B")
out = mel.create_material_expression(m, unreal.MaterialExpressionAdd, 50, 400)
mel.connect_material_expressions(dim, "", out, "A")
mel.connect_material_expressions(em, "", out, "B")
mel.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(out, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(const(m, 1.0, 900), "", unreal.MaterialProperty.MP_ROUGHNESS)
mel.connect_material_property(const(m, 0.0, 1000), "", unreal.MaterialProperty.MP_SPECULAR)
finish(m)

# additive
m = new_material("M_DC_Add")
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
m.set_editor_property("two_sided", True)
c = vec_param(m, "Color", (0.1, 0.1, 0.1, 1.0), 0)
mel.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
finish(m)

unreal.log("DEEPCUT_MAT done")
