# -----------------------------------------------------------------------------
# Lobby text styles (CommonTextStyle) - 9 base + 13 derived
# Source: "Arena FPS Lobby UI - CommonUI style spec"
#
# Safety:
#   - Creates assets ONLY in TARGET_FOLDER.
#   - If an asset with the same name already exists, it is skipped (never modified).
#   - Nothing is deleted.
#
# Project Font Resolution = 96 DPI, so Size = px * 0.75 (spec values in brackets).
# Run once from the Unreal Editor (Tools > Execute Python Script...).
# -----------------------------------------------------------------------------
import unreal

TARGET_FOLDER = "/Game/QuakeLike_1_0/GameFlow/UI/02_Styles/01_TextStyles/02_TS_Lobby"
FONT_PATH = "/Game/QuakeLike_1_0/GameFlow/UI/01_Fonts/F_Pretendard"


def srgb_to_linear(c):
    c = c / 255.0
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def hex_color(hex_value, alpha=1.0):
    h = hex_value.lstrip("#")
    r, g, b = (int(h[i:i + 2], 16) for i in (0, 2, 4))
    return unreal.LinearColor(srgb_to_linear(r), srgb_to_linear(g), srgb_to_linear(b), alpha)


# Palette tokens used by text (Hex sRGB, alpha)
TEXT_1 = ("F4F6FA", 1.00)
TEXT_2 = ("F4F6FA", 0.70)
ON_ACCENT = ("121317", 1.00)
ON_ACCENT_DIM = ("121317", 0.60)
ENEMY = ("FF4D5A", 1.00)
DIFF_EASY = ("3FC6E8", 1.00)
DIFF_NORMAL = ("F4F6FA", 1.00)
DIFF_HARD = ("FF4D5A", 1.00)

SHADOW_OFFSET = (0.0, 2.0)
SHADOW_COLOR = ("000000", 0.65)

# name, typeface, size(pt @96DPI), letter spacing, color, line height %, drop shadow
BASE_STYLES = [
    ("TS_Display",        "Black",    33.0, -10, TEXT_1, 1.00, True),
    ("TS_HeadingL",       "Black",    24.0, -10, TEXT_1, 1.00, False),
    ("TS_Heading",        "Bold",     18.0,   0, TEXT_1, 1.00, True),
    ("TS_Heading_Black",  "Black",    18.0,   0, TEXT_1, 1.00, False),
    ("TS_BodyStrong",     "Bold",     13.5,   0, TEXT_1, 1.00, False),
    ("TS_Body",           "Regular",  13.5,   0, TEXT_2, 1.25, False),
    ("TS_Body_Input",     "SemiBold", 13.5,  20, TEXT_1, 1.00, False),
    ("TS_CaptionStrong",  "SemiBold", 10.5,   0, TEXT_1, 1.00, False),
    ("TS_Caption",        "Regular",  10.5,   0, TEXT_2, 1.20, True),
]

# name, parent, color, typeface override (or None), drop shadow override (or None)
DERIVED_STYLES = [
    ("TS_HeadingL_OnAccent",      "TS_HeadingL",      ON_ACCENT,     None,    None),
    ("TS_HeadingL_OnAccentDim",   "TS_HeadingL",      ON_ACCENT_DIM, None,    None),
    ("TS_BodyBlack_OnAccent",     "TS_BodyStrong",    ON_ACCENT,     "Black", None),
    ("TS_BodyStrong_Easy",        "TS_BodyStrong",    DIFF_EASY,     None,    None),
    ("TS_BodyStrong_Normal",      "TS_BodyStrong",    DIFF_NORMAL,   None,    None),
    ("TS_BodyStrong_Hard",        "TS_BodyStrong",    DIFF_HARD,     None,    None),
    ("TS_BodyStrong_Muted",       "TS_BodyStrong",    TEXT_2,        None,    None),
    ("TS_Heading_Easy",           "TS_Heading",       DIFF_EASY,     None,    False),
    ("TS_Heading_Normal",         "TS_Heading",       DIFF_NORMAL,   None,    False),
    ("TS_Heading_Hard",           "TS_Heading",       DIFF_HARD,     None,    False),
    ("TS_Heading_Muted",          "TS_Heading",       TEXT_2,        None,    False),
    ("TS_CaptionStrong_OnAccent", "TS_CaptionStrong", ON_ACCENT,     "Bold",  None),
    ("TS_Caption_Error",          "TS_Caption",       ENEMY,         None,    True),
]

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
bel = unreal.BlueprintEditorLibrary

created, skipped, failed = [], [], []


def asset_path(name):
    return "{}/{}".format(TARGET_FOLDER, name)


def create_blueprint(name, parent_class):
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    bp = asset_tools.create_asset(name, TARGET_FOLDER, unreal.Blueprint, factory)
    if bp is None:
        raise RuntimeError("create_asset returned None")
    bel.compile_blueprint(bp)
    return bp


def default_object(bp):
    return unreal.get_default_object(bel.generated_class(bp))


def apply_font(cdo, font_obj=None, typeface=None, size=None, spacing=None):
    font = cdo.get_editor_property("font")
    if font_obj is not None:
        font.set_editor_property("font_object", font_obj)
    if typeface is not None:
        font.set_editor_property("typeface_font_name", typeface)
    if size is not None:
        try:
            font.set_editor_property("size", float(size))
        except Exception:
            font.set_editor_property("size", int(round(size)))
            unreal.log_warning("Font size is integer in this engine build; {} rounded to {}".format(size, int(round(size))))
    if spacing is not None:
        font.set_editor_property("letter_spacing", int(spacing))
    cdo.set_editor_property("font", font)


def save(bp):
    eal.save_loaded_asset(bp, only_if_is_dirty=False)


def main():
    if not eal.does_directory_exist(TARGET_FOLDER):
        unreal.log_error("Target folder not found: " + TARGET_FOLDER)
        return
    font_obj = eal.load_asset(FONT_PATH)
    if font_obj is None:
        unreal.log_error("Font not found: " + FONT_PATH)
        return

    # 1) Base styles
    for name, typeface, size, spacing, color, line_height, shadow in BASE_STYLES:
        if eal.does_asset_exist(asset_path(name)):
            skipped.append(name)
            continue
        try:
            bp = create_blueprint(name, unreal.CommonTextStyle)
            cdo = default_object(bp)
            apply_font(cdo, font_obj, typeface, size, spacing)
            cdo.set_editor_property("color", hex_color(*color))
            cdo.set_editor_property("uses_drop_shadow", shadow)
            cdo.set_editor_property("shadow_offset", unreal.Vector2D(*SHADOW_OFFSET))
            cdo.set_editor_property("shadow_color", hex_color(*SHADOW_COLOR))
            cdo.set_editor_property("margin", unreal.Margin(0.0, 0.0, 0.0, 0.0))
            cdo.set_editor_property("line_height_percentage", line_height)
            save(bp)
            created.append(name)
        except Exception as e:
            failed.append("{} ({})".format(name, e))

    # 2) Derived styles (child blueprints of the base styles)
    for name, parent_name, color, typeface, shadow in DERIVED_STYLES:
        if eal.does_asset_exist(asset_path(name)):
            skipped.append(name)
            continue
        try:
            parent_bp = eal.load_asset(asset_path(parent_name))
            if parent_bp is None:
                raise RuntimeError("parent not found: " + parent_name)
            bp = create_blueprint(name, bel.generated_class(parent_bp))
            cdo = default_object(bp)
            cdo.set_editor_property("color", hex_color(*color))
            if typeface is not None:
                apply_font(cdo, typeface=typeface)
            if shadow is not None:
                cdo.set_editor_property("uses_drop_shadow", shadow)
            save(bp)
            created.append(name)
        except Exception as e:
            failed.append("{} ({})".format(name, e))

    unreal.log("========== Lobby text styles ==========")
    unreal.log("Created ({}): {}".format(len(created), ", ".join(created)))
    unreal.log("Skipped, already exists ({}): {}".format(len(skipped), ", ".join(skipped)))
    if failed:
        unreal.log_error("Failed ({}): {}".format(len(failed), " | ".join(failed)))
    unreal.log("=======================================")


main()
