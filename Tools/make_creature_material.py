"""Creates M_DC_Creature: the material every sea creature is drawn with, so that it swims (headless editor).

    UnrealEditor-Cmd MegaJam.uproject -run=pythonscript -script=Tools/make_creature_material.py -nullrhi -unattended

The models have no bones. The body moves by a world position offset computed from the vertex's place in the mesh:
  tail   a wave along the long axis, nothing at the head, most at the tail (fish, sharks, eels; a whale's fluke when
         the side vector points up; a jellyfish's tentacles when the axis points down)
  wing   a wave that grows away from the body's middle line (rays, a turtle's flippers, a crab's legs)
  pulse  the body swells and narrows round its axis (a bell, a mantle)
The game sets the vectors per creature (DCDoc.cpp); with the defaults nothing moves.
Textures use the names of the glTF importer's material, so they are copied over by name.
"""
import unreal

DIR = "/Game/DeepCut"
NAME = "M_DC_Creature"
SAMPLE = "/Game/DeepCut/Art/creature_fish/creature_fish/Textures/"
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

path = "%s/%s" % (DIR, NAME)
if eal.does_asset_exist(path):
    eal.delete_asset(path)
m = tools.create_asset(NAME, DIR, unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("used_with_instanced_static_meshes", True)


def node(cls, x, y):
    return mel.create_material_expression(m, cls, x, y)


def scalar(name, value, x, y):
    e = node(unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", value)
    return e


def vec4(name, x, y):
    """A vector parameter as four numbers."""
    p = node(unreal.MaterialExpressionVectorParameter, x, y)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, 0.0, 0.0))
    a = node(unreal.MaterialExpressionAppendVector, x + 220, y)
    mel.connect_material_expressions(p, "", a, "A")
    mel.connect_material_expressions(p, "A", a, "B")
    return a


def texture(name, file, x, y):
    t = unreal.load_asset(SAMPLE + file)
    e = node(unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("texture", t)
    if t.get_editor_property("compression_settings") == unreal.TextureCompressionSettings.TC_NORMALMAP:
        st = unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
    elif t.get_editor_property("srgb"):
        st = unreal.MaterialSamplerType.SAMPLERTYPE_COLOR
    else:
        st = unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
    e.set_editor_property("sampler_type", st)
    unreal.log("DEEPCUT_MAT %s: %s sampler %s" % (name, t.get_name(), st))
    return e


def mul(a, b, x, y):
    e = node(unreal.MaterialExpressionMultiply, x, y)
    mel.connect_material_expressions(a, "", e, "A")
    mel.connect_material_expressions(b, "", e, "B")
    return e


# ---- surface
base = texture("BaseColorTexture", "texture_diffuse", -900, -500)
tint = node(unreal.MaterialExpressionVectorParameter, -900, -250)
tint.set_editor_property("parameter_name", "Tint")
tint.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
mel.connect_material_property(mul(base, tint, -600, -400), "", unreal.MaterialProperty.MP_BASE_COLOR)
mr = texture("MetallicRoughnessTexture", "texture_metallic-texture_roughness", -900, -50)
rough = node(unreal.MaterialExpressionSaturate, -400, -50)
rm = node(unreal.MaterialExpressionMultiply, -600, -50)
mel.connect_material_expressions(mr, "G", rm, "A")
mel.connect_material_expressions(scalar("RoughnessFactor", 1.0, -900, 150), "", rm, "B")
mel.connect_material_expressions(rm, "", rough, "")
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
normal = texture("NormalTexture", "texture_normal", -900, 300)
mel.connect_material_property(normal, "", unreal.MaterialProperty.MP_NORMAL)
glow = node(unreal.MaterialExpressionVectorParameter, -900, 550)
glow.set_editor_property("parameter_name", "Glow")
glow.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, 0.0, 1.0))
mel.connect_material_property(mul(base, glow, -600, 550), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

# ---- the swim
custom = node(unreal.MaterialExpressionCustom, -300, 1100)
names = ["P", "T", "R", "Axis", "Side", "Lat", "Up", "Centre", "Tail", "Wing", "Pulse"]
ins = []
for n in names:
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", n)
    ins.append(ci)
custom.set_editor_property("inputs", ins)
custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
custom.set_editor_property("code", """
// Axis: xyz from head to tail, w the head's place along it.  Side: xyz the tail's swing, w the body's length.
// Lat: xyz across the body, w the middle.  Up: xyz the wing's swing, w half the width.  Centre: xyz, w phase.
// Tail: amount, power, waves, beats.  Wing: amount, waves, rate.  Pulse: amount, waves, rate.
float u = saturate((dot(P, Axis.xyz) - Axis.w) / max(Side.w, 0.001));
float ph = Centre.w + R * 6.2832;
float w = T * Tail.w;
float tail = Tail.x * pow(u, max(Tail.y, 0.01)) * sin(Tail.z * u * 6.2832 - w + ph);
float lat = saturate(abs(dot(P, Lat.xyz) - Lat.w) / max(Up.w, 0.001));
float wing = Wing.x * lat * lat * sin(w * Wing.z - lat * Wing.y * 6.2832 + ph);
float3 d = P - Centre.xyz;
float3 rad = d - Axis.xyz * dot(d, Axis.xyz);
float pulse = Pulse.x * sin(w * Pulse.z - u * Pulse.y * 6.2832 + ph);
return Side.xyz * (tail * Side.w) + Up.xyz * (wing * Up.w) + rad * pulse;
""")
srcs = {
    "P": node(unreal.MaterialExpressionPreSkinnedPosition, -900, 900),
    "T": node(unreal.MaterialExpressionTime, -900, 980),
    "R": node(unreal.MaterialExpressionPerInstanceRandom, -900, 1060),
}
y = 1150
for n in names[3:]:
    srcs[n] = vec4("Swim" + n, -900, y)
    y += 160
for n in names:
    ok = mel.connect_material_expressions(srcs[n], "", custom, n)
    unreal.log("DEEPCUT_MAT input %s connected: %s" % (n, ok))
tr = node(unreal.MaterialExpressionTransform, 0, 1100)
tr.set_editor_property("transform_source_type", unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL)
tr.set_editor_property("transform_type", unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
mel.connect_material_expressions(custom, "", tr, "")
unreal.log("DEEPCUT_MAT wpo connected: %s" % mel.connect_material_property(tr, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET))

mel.recompile_material(m)
ok = eal.save_loaded_asset(m)
unreal.log("DEEPCUT_MAT saved %s: %s" % (m.get_path_name(), ok))
