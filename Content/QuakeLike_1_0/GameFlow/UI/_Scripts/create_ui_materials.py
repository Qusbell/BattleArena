# -----------------------------------------------------------------------------
# UI materials for the lobby style spec
#   - M_UI_Cut      : 45-degree corner cut (top-left / bottom-right), border, dashed border
#   - M_UI_TabGlow  : difficulty tab glow (bottom 32% gradient)
#   - M_UI_MapFade  : transparent -> color vertical fade
#   - MI_UI_Cut_*   : material instances (colors read from PDA_UIPalette defaults)
#
# Safety:
#   - Creates assets ONLY in MAT_FOLDER and INST_FOLDER.
#   - Existing assets with the same name are skipped (never modified). Nothing is deleted.
#   - PDA_UIPalette is only READ.
#
# Pixel size: the widget size is measured in the shader (ddx/ddy of the UV), so the cut
# stays the same number of pixels whatever the widget size. No Size parameter is needed.
# PixelScale (default 1.0) multiplies Cut / BorderWidth / dash length for non-1080p DPI.
#
# Run: Tools > Execute Python Script...
# -----------------------------------------------------------------------------
import unreal

UI_ROOT = "/Game/QuakeLike_1_0/GameFlow/UI"
MAT_FOLDER = UI_ROOT + "/02_Styles/04_Materials"
INST_FOLDER = MAT_FOLDER + "/Instance"
PDA_PATH = UI_ROOT + "/02_Styles/03_Palette/PDA_UIPalette"
WHITE_TEX = "/Engine/EngineResources/WhiteSquareTexture"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
bel = unreal.BlueprintEditorLibrary

created, skipped, failed = [], [], []

# Set to True only by fix_ui_materials.py: rebuilds the node graphs of the 3 materials
# this script owns (M_UI_Cut, M_UI_TabGlow, M_UI_MapFade). Nothing else is touched.
REBUILD_MATERIALS = globals().get("REBUILD_MATERIALS", False)
OWN_MATERIALS = ("M_UI_Cut", "M_UI_TabGlow", "M_UI_MapFade")

CUT_HLSL = r"""
float2 uv = UV;
float W = 1.0 / max(length(float2(ddx(uv.x), ddy(uv.x))), 1e-6);
float H = 1.0 / max(length(float2(ddx(uv.y), ddy(uv.y))), 1e-6);
float2 p = uv * float2(W, H);
float s  = max(PixelScale, 0.001);
float c  = Cut * s;
float bw = BorderWidth * s;

float dEdge = min(min(p.x, p.y), min(W - p.x, H - p.y));
float dTL = (p.x + p.y - c) * 0.70710678;
float dBR = ((W - p.x) + (H - p.y) - c) * 0.70710678;
float d = min(dEdge, min(dTL, dBR));

float aOuter = saturate(d + 0.5);
float aInner = saturate(d - bw + 0.5);
float borderMask = (bw > 0.001) ? saturate(aOuter - aInner) : 0.0;

if (Dashed > 0.5 && bw > 0.001)
{
    float dx = min(p.x, W - p.x);
    float dy = min(p.y, H - p.y);
    float t = (dx < dy) ? p.y : p.x;
    float period = 10.0 * s;
    float dashLen = 6.0 * s;
    borderMask *= step(fmod(t, period), dashLen);
}

float3 fillRGB = FillRGB.rgb * TexRGB.rgb;
float  fillA   = FillA * TexA * aInner;
float  bA      = BorderA * borderMask;
float  outA    = bA + fillA * (1.0 - bA);
float3 outRGB  = (BorderRGB.rgb * bA + fillRGB * fillA * (1.0 - bA)) / max(outA, 1e-5);
return float4(outRGB * VCRGB.rgb, outA * VCA);
"""


def lc(r, g, b, a=1.0):
    return unreal.LinearColor(r, g, b, a)


def srgb_to_linear(c):
    c = c / 255.0
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def hex_color(h, a=1.0):
    h = h.lstrip("#")
    r, g, b = (int(h[i:i + 2], 16) for i in (0, 2, 4))
    return lc(srgb_to_linear(r), srgb_to_linear(g), srgb_to_linear(b), a)


def new_material(name):
    path = "{}/{}".format(MAT_FOLDER, name)
    if eal.does_asset_exist(path):
        if REBUILD_MATERIALS and name in OWN_MATERIALS:
            mat = eal.load_asset(path)
            mel.delete_all_material_expressions(mat)
            mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
            mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
            return mat, mat
        skipped.append(name)
        return None, eal.load_asset(path)
    mat = asset_tools.create_asset(name, MAT_FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    return mat, mat


def node(mat, cls, x, y, **props):
    n = mel.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        n.set_editor_property(k, v)
    return n


def vparam(mat, name, default, x, y):
    return node(mat, unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name, default_value=default)


def sparam(mat, name, default, x, y):
    return node(mat, unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=default)


def finish(mat, name):
    mel.layout_material_expressions(mat)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat, only_if_is_dirty=False)
    created.append(name)


# ----------------------------------------------------------------------------- M_UI_Cut
def build_cut():
    name = "M_UI_Cut"
    mat, existing = new_material(name)
    if mat is None:
        return existing

    uv = node(mat, unreal.MaterialExpressionTextureCoordinate, -1200, 0)
    fill = vparam(mat, "FillColor", hex_color("22252D"), -1200, 150)
    border = vparam(mat, "BorderColor", hex_color("6B7180"), -1200, 350)
    bw = sparam(mat, "BorderWidth", 0.0, -1200, 550)
    cut = sparam(mat, "Cut", 8.0, -1200, 650)
    scale = sparam(mat, "PixelScale", 1.0, -1200, 750)
    dashed = sparam(mat, "Dashed", 0.0, -1200, 850)
    tex = node(mat, unreal.MaterialExpressionTextureSampleParameter2D, -1200, 1000,
               parameter_name="FillTexture", texture=eal.load_asset(WHITE_TEX))
    vc = node(mat, unreal.MaterialExpressionVertexColor, -1200, 1300)

    custom = node(mat, unreal.MaterialExpressionCustom, -600, 400)
    custom.set_editor_property("code", CUT_HLSL)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    custom.set_editor_property("description", "UI Cut")
    input_names = ["UV", "FillRGB", "FillA", "BorderRGB", "BorderA", "BorderWidth", "Cut",
                   "PixelScale", "Dashed", "TexRGB", "TexA", "VCRGB", "VCA"]
    inputs = []
    for n_ in input_names:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n_)
        inputs.append(ci)
    custom.set_editor_property("inputs", inputs)

    links = [
        (uv, "", "UV"), (fill, "", "FillRGB"), (fill, "A", "FillA"),
        (border, "", "BorderRGB"), (border, "A", "BorderA"),
        (bw, "", "BorderWidth"), (cut, "", "Cut"), (scale, "", "PixelScale"),
        (dashed, "", "Dashed"), (tex, "", "TexRGB"), (tex, "A", "TexA"),
        (vc, "", "VCRGB"), (vc, "A", "VCA"),
    ]
    for src, out_name, in_name in links:
        if not mel.connect_material_expressions(src, out_name, custom, in_name):
            raise RuntimeError("connect failed: " + in_name)
    mel.connect_material_expressions(uv, "", tex, "UVs")

    rgb = node(mat, unreal.MaterialExpressionComponentMask, -250, 300, r=True, g=True, b=True, a=False)
    alpha = node(mat, unreal.MaterialExpressionComponentMask, -250, 500, r=False, g=False, b=False, a=True)
    mel.connect_material_expressions(custom, "", rgb, "")
    mel.connect_material_expressions(custom, "", alpha, "")
    mel.connect_material_property(rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(alpha, "", unreal.MaterialProperty.MP_OPACITY)

    finish(mat, name)
    return mat


# ----------------------------------------------------------------------------- M_UI_TabGlow
def build_tab_glow():
    name = "M_UI_TabGlow"
    mat, existing = new_material(name)
    if mat is None:
        return existing
    uv = node(mat, unreal.MaterialExpressionTextureCoordinate, -900, 0)
    v = node(mat, unreal.MaterialExpressionComponentMask, -700, 0, r=False, g=True, b=False, a=False)
    mel.connect_material_expressions(uv, "", v, "")
    height = sparam(mat, "GlowHeight", 0.32, -900, 200)
    one_minus = node(mat, unreal.MaterialExpressionOneMinus, -700, 200)
    mel.connect_material_expressions(height, "", one_minus, "")
    sub = node(mat, unreal.MaterialExpressionSubtract, -500, 0)
    mel.connect_material_expressions(v, "", sub, "A")
    mel.connect_material_expressions(one_minus, "", sub, "B")
    div = node(mat, unreal.MaterialExpressionDivide, -350, 0)
    mel.connect_material_expressions(sub, "", div, "A")
    mel.connect_material_expressions(height, "", div, "B")
    sat = node(mat, unreal.MaterialExpressionSaturate, -200, 0)
    mel.connect_material_expressions(div, "", sat, "")
    glow = vparam(mat, "GlowColor", lc(1, 1, 1, 0.05), -900, 400)
    vc = node(mat, unreal.MaterialExpressionVertexColor, -900, 600)
    m1 = node(mat, unreal.MaterialExpressionMultiply, -100, 200)
    mel.connect_material_expressions(sat, "", m1, "A")
    mel.connect_material_expressions(glow, "A", m1, "B")
    m2 = node(mat, unreal.MaterialExpressionMultiply, 50, 200)
    mel.connect_material_expressions(m1, "", m2, "A")
    mel.connect_material_expressions(vc, "A", m2, "B")
    rgb = node(mat, unreal.MaterialExpressionMultiply, 50, 0)
    mel.connect_material_expressions(glow, "", rgb, "A")
    mel.connect_material_expressions(vc, "", rgb, "B")
    mel.connect_material_property(rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(m2, "", unreal.MaterialProperty.MP_OPACITY)
    finish(mat, name)
    return mat


# ----------------------------------------------------------------------------- M_UI_MapFade
def build_map_fade(fade_color):
    name = "M_UI_MapFade"
    mat, existing = new_material(name)
    if mat is None:
        return existing
    uv = node(mat, unreal.MaterialExpressionTextureCoordinate, -700, 0)
    v = node(mat, unreal.MaterialExpressionComponentMask, -500, 0, r=False, g=True, b=False, a=False)
    mel.connect_material_expressions(uv, "", v, "")
    col = vparam(mat, "FadeColor", fade_color, -700, 200)
    vc = node(mat, unreal.MaterialExpressionVertexColor, -700, 400)
    m1 = node(mat, unreal.MaterialExpressionMultiply, -300, 100)
    mel.connect_material_expressions(v, "", m1, "A")
    mel.connect_material_expressions(col, "A", m1, "B")
    m2 = node(mat, unreal.MaterialExpressionMultiply, -150, 100)
    mel.connect_material_expressions(m1, "", m2, "A")
    mel.connect_material_expressions(vc, "A", m2, "B")
    rgb = node(mat, unreal.MaterialExpressionMultiply, -150, -100)
    mel.connect_material_expressions(col, "", rgb, "A")
    mel.connect_material_expressions(vc, "", rgb, "B")
    mel.connect_material_property(rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(m2, "", unreal.MaterialProperty.MP_OPACITY)
    finish(mat, name)
    return mat


# ----------------------------------------------------------------------------- Instances
def make_instance(name, parent, vectors=None, scalars=None, textures=None):
    path = "{}/{}".format(INST_FOLDER, name)
    if eal.does_asset_exist(path):
        skipped.append(name)
        return
    try:
        mi = asset_tools.create_asset(name, INST_FOLDER, unreal.MaterialInstanceConstant,
                                      unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, parent)
        for k, v in (vectors or {}).items():
            mel.set_material_instance_vector_parameter_value(mi, k, v)
        for k, v in (scalars or {}).items():
            mel.set_material_instance_scalar_parameter_value(mi, k, float(v))
        for k, v in (textures or {}).items():
            mel.set_material_instance_texture_parameter_value(mi, k, v)
        mel.update_material_instance(mi)
        eal.save_loaded_asset(mi, only_if_is_dirty=False)
        created.append(name)
    except Exception as e:
        failed.append("{} ({})".format(name, e))


def main():
    for folder in (MAT_FOLDER, INST_FOLDER):
        if not eal.does_directory_exist(folder):
            unreal.log_error("Folder not found: " + folder)
            return

    pda = eal.load_asset(PDA_PATH)
    if pda is None:
        unreal.log_error("PDA_UIPalette not found")
        return
    pal = unreal.get_default_object(bel.generated_class(pda))
    P = lambda n: pal.get_editor_property(n)
    CLEAR = lc(0, 0, 0, 0)

    cut = glow = fade = None
    try:
        cut = build_cut()
    except Exception as e:
        failed.append("M_UI_Cut ({})".format(e))
    try:
        glow = build_tab_glow()
    except Exception as e:
        failed.append("M_UI_TabGlow ({})".format(e))
    try:
        fade = build_map_fade(P("MapFade"))
    except Exception as e:
        failed.append("M_UI_MapFade ({})".format(e))

    if cut is not None:
        # name, Cut, BorderWidth, Fill, Border, extra scalars
        rows = [
            ("MI_UI_Cut_Key",           4, 1, P("Chip"),        P("Line")),
            ("MI_UI_Cut_Chip",          4, 0, P("Chip"),        CLEAR),
            ("MI_UI_Cut_TagHost",       4, 0, P("White"),       CLEAR),
            ("MI_UI_Cut_TagOutline",    4, 1, CLEAR,            P("LineStrong")),
            ("MI_UI_Cut_Btn_Normal",    8, 1, P("Bg700"),       P("LineStrong")),
            ("MI_UI_Cut_Btn_Hovered",   8, 1, P("Bg600"),       P("LineHover")),
            ("MI_UI_Cut_Btn_Pressed",   8, 1, P("Bg800"),       P("LineHover")),
            ("MI_UI_Cut_Confirm_Normal",  8, 0, P("White"),       CLEAR),
            ("MI_UI_Cut_Confirm_Hovered", 8, 0, hex_color("D7D8DC"), CLEAR),
            ("MI_UI_Cut_Thumb_Normal",  8, 2, P("Bg700"),       P("LineSoft")),
            ("MI_UI_Cut_Thumb_Hovered", 8, 2, P("Bg700"),       P("LineStrong")),
            ("MI_UI_Cut_Thumb_Selected",8, 2, P("Bg700"),       P("White")),
            ("MI_UI_Cut_Slot",          8, 1, P("Bg700"),       P("Line")),
            ("MI_UI_Cut_SlotReady",     8, 0, P("Accent"),      CLEAR),
            ("MI_UI_Cut_Banner",        8, 1, P("Chip"),        P("Line")),
            ("MI_UI_Cut_Menu",          8, 1, P("Bg800"),       P("Line")),
            ("MI_UI_Cut_Primary_Normal",   24, 0, P("Accent"),        CLEAR),
            ("MI_UI_Cut_Primary_Hovered",  24, 0, hex_color("FFD800"), CLEAR),
            ("MI_UI_Cut_Primary_Pressed",  24, 0, hex_color("E0AC00"), CLEAR),
            ("MI_UI_Cut_Primary_Selected", 24, 0, P("White"),         CLEAR),
            ("MI_UI_Cut_Primary_Disabled", 24, 0, P("AccentDim"),     CLEAR),
            ("MI_UI_Cut_Launch",        24, 1, P("Panel94"),    P("LineStrong")),
            ("MI_UI_Cut_Modal",         28, 1, P("Bg800"),      P("Line")),
        ]
        for name, c, bw, fill, border in rows:
            make_instance(name, cut, {"FillColor": fill, "BorderColor": border}, {"Cut": c, "BorderWidth": bw})
        make_instance("MI_UI_Cut_SlotEmpty", cut, {"FillColor": P("Bg800"), "BorderColor": P("LineStrong")},
                      {"Cut": 8, "BorderWidth": 1, "Dashed": 1})
        make_instance("MI_UI_Cut_Map", cut, {"FillColor": lc(1, 1, 1, 1), "BorderColor": P("Line")},
                      {"Cut": 16, "BorderWidth": 1})

    if glow is not None:
        make_instance("MI_UI_TabGlow_Hover", glow, {"GlowColor": lc(1, 1, 1, 0.05)})
        make_instance("MI_UI_TabGlow_Easy", glow, {"GlowColor": P("DiffEasyTint")})
        make_instance("MI_UI_TabGlow_Normal", glow, {"GlowColor": P("DiffNormalTint")})
        make_instance("MI_UI_TabGlow_Hard", glow, {"GlowColor": P("DiffHardTint")})

    unreal.log("========== UI materials ==========")
    unreal.log("Created ({}): {}".format(len(created), ", ".join(created)))
    unreal.log("Skipped, already exists ({}): {}".format(len(skipped), ", ".join(skipped)))
    if failed:
        unreal.log_error("Failed ({}): {}".format(len(failed), " | ".join(failed)))
    unreal.log("==================================")


main()
