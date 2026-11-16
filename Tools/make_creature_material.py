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
# an emissive map as well (Unit 0's eyes and lamp), off unless EmissiveStrength is set
emap = texture("EmissiveTexture", "texture_diffuse", -900, 750)
emul = mul(emap, scalar("EmissiveStrength", 0.0, -900, 950), -600, 750)
eadd = node(unreal.MaterialExpressionAdd, -400, 600)
mel.connect_material_expressions(mul(base, glow, -600, 550), "", eadd, "A")
mel.connect_material_expressions(emul, "", eadd, "B")
mel.connect_material_property(eadd, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

# ---- the swim
custom = node(unreal.MaterialExpressionCustom, -300, 1100)
names = ["P", "T", "R", "Axis", "Side", "Lat", "Up", "Centre", "Tail", "Wing", "Pulse", "Drive", "Fin", "Leg", "Leg2", "Shape", "Pec"]
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
// Tail: amount, power, waves, beats.  Wing: amount, waves, rate, half the height.  Pulse: amount, waves, rate.
// Drive (set every frame by the game for an animal whose beat follows its speed): x is added to the beat's phase,
// y z w are added to 1 to scale the tail, the wing and the pulse.  With Tail.w = 0 the phase is Drive.x alone.
// Fin: a ripple along the body in what stands off its middle line (a ribbon fin): amount, waves, rate, where it starts.
// Leg: legs per side (0 = none), swing (radians), lift (radians), hip radius as a fraction of the half span.
// Leg2: phase step from one leg to the next (radians; 3.1416 = alternating, 2pi/N = a metachronal wave), knee as a
//       fraction of the leg, only below this height (-1 = everywhere; 0 = the lower half), leg beat rate.
// Shape: x waves along the wing from its leading edge to its trailing edge (rays), y the share of the cycle a pulse
//        spends contracting (0 = a plain sine; 0.3 = a quick squeeze and a slow refill, as jellyfish and squid do).
float u = saturate((dot(P, Axis.xyz) - Axis.w) / max(Side.w, 0.001));
float ph = Centre.w + R * 6.2832;
float w = T * Tail.w + Drive.x;
float tail = Tail.x * (1.0 + Drive.y) * pow(u, max(Tail.y, 0.01)) * sin(Tail.z * u * 6.2832 - w + ph);
float lat = saturate(abs(dot(P, Lat.xyz) - Lat.w) / max(Up.w, 0.001));
float wing = Wing.x * (1.0 + Drive.z) * lat * lat * sin(w * Wing.z - lat * Wing.y * 6.2832 - u * Shape.x * 6.2832 + ph);
float3 d = P - Centre.xyz;
float3 rad = d - Axis.xyz * dot(d, Axis.xyz);
float pp = w * Pulse.z - u * Pulse.y * 6.2832 + ph;
float pwave = sin(pp);
if (Shape.y > 0.0)
{
    // a squeeze in Shape.y of the cycle, then a slow refill: -1 contracted, +1 open
    float x = frac(pp / 6.2832);
    float c = x < Shape.y ? sin(1.5708 * x / Shape.y) : cos(1.5708 * (x - Shape.y) / (1.0 - Shape.y));
    pwave = 1.0 - 2.0 * c;
}
float pulse = Pulse.x * (1.0 + Drive.w) * pwave;
float hgt = abs(dot(d, Up.xyz)) / max(Wing.w, 0.001);
float fin = Fin.x * saturate((hgt - Fin.w) / max(1.0 - Fin.w, 0.01)) * sin(Fin.y * u * 6.2832 - w * Fin.z + ph * 1.7);
float3 off = Side.xyz * (tail * Side.w) + Up.xyz * (wing * Up.w) + rad * pulse + Lat.xyz * (fin * Side.w);

// Pec: the paired fins behind the head (pectorals). amount (of the half span), where across the body they start
// (0..1 of the half span), rate (times the beat), how far back along the body they reach (0..1). Both beat together:
// down and back is the power stroke, up and forward the recovery (labriform rowing; a hovering barreleye or a
// coelacanth sculls the same way).
if (Pec.x > 0.0)
{
    float fl = saturate((lat - Pec.y) / max(1.0 - Pec.y, 0.01));
    float fa = 1.0 - smoothstep(Pec.w * 0.7, Pec.w, u);
    float pph = w * Pec.z + ph * 0.5;
    float k = Pec.x * (1.0 + Drive.z) * fl * fl * fa * Up.w;
    off += Up.xyz * (k * sin(pph)) + Axis.xyz * (0.5 * k * cos(pph));
}

// legs: each leg is a stiff lever on a hip, with a knee. It swings forward and back about the hip, lifts while it
// swings forward, and the part past the knee folds back down so the foot points at the ground.
if (Leg.x > 0.5)
{
    float3 fwd = -Axis.xyz;
    float3 up = Up.xyz;
    float3 across = Lat.xyz;
    float a = dot(d, fwd);
    float l = dot(P, across) - Lat.w;
    float span = max(Up.w, 0.001);
    float hip = Leg.w * span;
    float r = length(float2(a, l));
    float below = Leg2.z < -0.5 ? 1.0 : saturate((-dot(d, up) / max(Wing.w, 0.001) - Leg2.z) * 6.0);
    float on = smoothstep(hip, hip + 0.12 * span, r) * below;
    if (on > 0.0)
    {
        float side = l >= 0.0 ? 1.0 : -1.0;
        float ang = atan2(a, abs(l));                     // -pi/2 at the back .. +pi/2 at the front
        float n = Leg.x;
        float idx = clamp(floor((ang / 3.1416 + 0.5) * n), 0.0, n - 1.0);
        float mid = ((idx + 0.5) / n - 0.5) * 3.1416;
        float3 radial = normalize(fwd * sin(mid) + across * side * cos(mid));
        float3 hp = Centre.xyz + radial * hip;
        float lph = w * Leg2.w - idx * Leg2.x + (side > 0.0 ? 0.0 : 3.1416) + ph;
        float swing = Leg.y * (1.0 + Drive.z) * sin(lph);
        float lift = Leg.z * (1.0 + Drive.z) * max(0.0, cos(lph));
        float3 v = P - hp;
        // swing about the vertical through the hip
        float cs = cos(swing), sn = sin(swing);
        v = v * cs + cross(up, v) * sn + up * dot(up, v) * (1.0 - cs);
        float3 rd = normalize(v - up * dot(v, up) + radial * 0.001);
        float3 tng = normalize(cross(rd, up));            // turning about this raises the outer part
        cs = cos(lift); sn = sin(lift);
        float3 v1 = v * cs + cross(tng, v) * sn + tng * dot(tng, v) * (1.0 - cs);
        // the knee folds the outer part back down
        float kneeR = Leg2.y * (span - hip);
        float3 kp = rd * kneeR;
        float3 kp1 = kp * cs + cross(tng, kp) * sn + tng * dot(tng, kp) * (1.0 - cs);
        float3 o = v1 - kp1;
        float fold = -lift * 1.4;
        float cf = cos(fold), sf = sin(fold);
        float3 v2 = kp1 + o * cf + cross(tng, o) * sf + tng * dot(tng, o) * (1.0 - cf);
        float past = smoothstep(kneeR * 0.85, kneeR * 1.15, dot(v, rd));
        float3 moved = hp + lerp(v1, v2, past);
        off += (moved - P) * on;
    }
}
return off;
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
