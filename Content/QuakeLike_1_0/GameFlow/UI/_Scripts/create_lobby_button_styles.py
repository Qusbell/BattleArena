# -----------------------------------------------------------------------------
# Lobby button styles (CommonButtonStyle) - 10 assets  +  arrow icon textures
#
# Safety:
#   - Button styles are created ONLY in BS_FOLDER, icons ONLY in ICON_FOLDER.
#   - Existing assets with the same name are skipped (never modified). Nothing is deleted.
#   - Text styles, materials and PDA_UIPalette are only READ.
#
# Not included (by design): sounds (no sound assets yet), focus border, disabled
# opacity 0.35, press scale 0.98 -> these belong to the button widget (W_ButtonBase).
#
# Run: Tools > Execute Python Script...
# -----------------------------------------------------------------------------
import os
import unreal

UI_ROOT = "/Game/QuakeLike_1_0/GameFlow/UI"
BS_FOLDER = UI_ROOT + "/02_Styles/02_ButtonStyles/02_BS_Lobby"
TS_FOLDER = UI_ROOT + "/02_Styles/01_TextStyles/02_TS_Lobby"
MI_FOLDER = UI_ROOT + "/02_Styles/04_Materials/Instance"
PDA_PATH = UI_ROOT + "/02_Styles/03_Palette/PDA_UIPalette"
ICON_FOLDER = UI_ROOT + "/UI_Image/Icon"
ICON_FILES = [
    "T_UI_Icon_ArrowHead_Left", "T_UI_Icon_ArrowHead_Right",
    "T_UI_Icon_ArrowHeadSlim_Left", "T_UI_Icon_ArrowHeadSlim_Right",
]

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
bel = unreal.BlueprintEditorLibrary
created, skipped, failed, warnings = [], [], [], []

try:
    SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
except NameError:
    SCRIPT_DIR = os.path.join(unreal.Paths.project_content_dir(), "QuakeLike_1_0", "GameFlow", "UI", "_Scripts")


# ----------------------------------------------------------------------------- helpers
def load(path):
    a = eal.load_asset(path)
    if a is None:
        raise RuntimeError("missing asset: " + path)
    return a


def ts(name):
    """Text style class (TSubclassOf<UCommonTextStyle>)."""
    return bel.generated_class(load("{}/{}".format(TS_FOLDER, name)))


def mi(name):
    return load("{}/{}".format(MI_FOLDER, name))


def brush_mi(name):
    b = unreal.SlateBrush()
    b.set_editor_property("resource_object", mi(name))
    b.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
    b.set_editor_property("tint_color", unreal.SlateColor(unreal.LinearColor(1, 1, 1, 1)))
    return b


def brush_color(color):
    b = unreal.SlateBrush()
    b.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
    b.set_editor_property("tint_color", unreal.SlateColor(color))
    return b


def brush_none():
    b = unreal.SlateBrush()
    b.set_editor_property("draw_as", unreal.SlateBrushDrawType.NO_DRAW_TYPE)
    return b


def setp(obj, prop, value, name):
    try:
        obj.set_editor_property(prop, value)
    except Exception as e:
        warnings.append("{}.{} not set ({})".format(name, prop, e))


def make_style(name, min_w, min_h, padding, brushes, texts):
    path = "{}/{}".format(BS_FOLDER, name)
    if eal.does_asset_exist(path):
        skipped.append(name)
        return
    try:
        f = unreal.BlueprintFactory()
        f.set_editor_property("parent_class", unreal.CommonButtonStyle)
        bp = asset_tools.create_asset(name, BS_FOLDER, unreal.Blueprint, f)
        bel.compile_blueprint(bp)
        cdo = unreal.get_default_object(bel.generated_class(bp))

        setp(cdo, "single_material", False, name)
        for prop, br in brushes.items():
            setp(cdo, prop, br, name)
        setp(cdo, "min_width", int(min_w), name)
        setp(cdo, "min_height", int(min_h), name)
        l, t, r, b = padding
        setp(cdo, "button_padding", unreal.Margin(l, t, r, b), name)
        for prop, style in texts.items():
            if style is not None:
                setp(cdo, prop, style, name)

        eal.save_loaded_asset(bp, only_if_is_dirty=False)
        created.append(name)
    except Exception as e:
        failed.append("{} ({})".format(name, e))


def states(normal, hovered, pressed, disabled=None, selected=None, selected_hovered=None, selected_pressed=None):
    d = {
        "normal_base": normal,
        "normal_hovered": hovered,
        "normal_pressed": pressed,
        "disabled": disabled if disabled is not None else normal,
        "selected_base": selected if selected is not None else normal,
        "selected_hovered": selected_hovered if selected_hovered is not None else (selected if selected is not None else hovered),
        "selected_pressed": selected_pressed if selected_pressed is not None else (selected if selected is not None else pressed),
    }
    return d


def texts(normal, hovered=None, selected=None, selected_hovered=None, disabled=None):
    return {
        "normal_text_style": normal,
        "normal_hovered_text_style": hovered if hovered is not None else normal,
        "selected_text_style": selected if selected is not None else normal,
        "selected_hovered_text_style": selected_hovered if selected_hovered is not None else (selected if selected is not None else normal),
        "disabled_text_style": disabled if disabled is not None else normal,
    }


# ----------------------------------------------------------------------------- icons
def import_icons():
    src_dir = os.path.join(SCRIPT_DIR, "Icons")
    for name in ICON_FILES:
        dest = "{}/{}".format(ICON_FOLDER, name)
        if eal.does_asset_exist(dest):
            skipped.append(name)
            continue
        src = os.path.join(src_dir, name + ".png")
        if not os.path.exists(src):
            failed.append("{} (source png not found: {})".format(name, src))
            continue
        try:
            task = unreal.AssetImportTask()
            task.set_editor_property("filename", src)
            task.set_editor_property("destination_path", ICON_FOLDER)
            task.set_editor_property("destination_name", name)
            task.set_editor_property("automated", True)
            task.set_editor_property("replace_existing", False)
            task.set_editor_property("save", False)
            asset_tools.import_asset_tasks([task])
            tex = eal.load_asset(dest)
            if tex is None:
                raise RuntimeError("import failed")
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
            tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
            tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
            tex.set_editor_property("srgb", True)
            eal.save_loaded_asset(tex, only_if_is_dirty=False)
            created.append(name)
        except Exception as e:
            failed.append("{} ({})".format(name, e))


# ----------------------------------------------------------------------------- main
def main():
    for folder in (BS_FOLDER, TS_FOLDER, MI_FOLDER, ICON_FOLDER):
        if not eal.does_directory_exist(folder):
            unreal.log_error("Folder not found: " + folder)
            return

    import_icons()

    pal = unreal.get_default_object(bel.generated_class(load(PDA_PATH)))
    P = lambda n: pal.get_editor_property(n)

    try:
        # 1. BS_Primary - start / ready (selected = ready complete) / connect
        make_style("BS_Primary", 380, 92, (0, 0, 0, 0),
                   states(brush_mi("MI_UI_Cut_Primary_Normal"), brush_mi("MI_UI_Cut_Primary_Hovered"),
                          brush_mi("MI_UI_Cut_Primary_Pressed"), disabled=brush_mi("MI_UI_Cut_Primary_Disabled"),
                          selected=brush_mi("MI_UI_Cut_Primary_Selected")),
                   texts(ts("TS_HeadingL_OnAccent"), disabled=ts("TS_HeadingL_OnAccentDim")))

        # 2. BS_Secondary - cancel, add bot
        make_style("BS_Secondary", 44, 44, (16, 0, 16, 0),
                   states(brush_mi("MI_UI_Cut_Btn_Normal"), brush_mi("MI_UI_Cut_Btn_Hovered"),
                          brush_mi("MI_UI_Cut_Btn_Pressed")),
                   texts(ts("TS_BodyStrong")))

        # 3. BS_SecondaryIcon - back, prev/next, arrows (icon 20, no text)
        make_style("BS_SecondaryIcon", 44, 44, (0, 0, 0, 0),
                   states(brush_mi("MI_UI_Cut_Btn_Normal"), brush_mi("MI_UI_Cut_Btn_Hovered"),
                          brush_mi("MI_UI_Cut_Btn_Pressed")),
                   texts(None))

        # 4. BS_Confirm - apply, leave, kick
        make_style("BS_Confirm", 140, 44, (16, 0, 16, 0),
                   states(brush_mi("MI_UI_Cut_Confirm_Normal"), brush_mi("MI_UI_Cut_Confirm_Hovered"),
                          brush_mi("MI_UI_Cut_Confirm_Hovered")),
                   texts(ts("TS_BodyBlack_OnAccent")))

        # 5. BS_MenuItem - bot menu items (no cut)
        make_style("BS_MenuItem", 220, 44, (12, 0, 12, 0),
                   states(brush_none(), brush_color(P("Bg600")), brush_color(P("Bg800")),
                          selected=brush_color(P("Bg700")), selected_hovered=brush_color(P("Bg600")),
                          selected_pressed=brush_color(P("Bg800"))),
                   texts(ts("TS_BodyStrong")))

        # 6. BS_Thumb - map / bot thumbnails
        make_style("BS_Thumb", 136, 166, (2, 2, 2, 2),
                   states(brush_mi("MI_UI_Cut_Thumb_Normal"), brush_mi("MI_UI_Cut_Thumb_Hovered"),
                          brush_mi("MI_UI_Cut_Thumb_Hovered"), selected=brush_mi("MI_UI_Cut_Thumb_Selected")),
                   texts(ts("TS_BodyStrong_Muted"), selected=ts("TS_BodyStrong"),
                         disabled=ts("TS_BodyStrong_Muted")))

        # 7-9. BS_DiffTab_* - single difficulty tabs (indicator bar 3px is widget content)
        for key in ("Easy", "Normal", "Hard"):
            make_style("BS_DiffTab_" + key, 200, 76, (0, 0, 0, 16),
                       states(brush_none(), brush_mi("MI_UI_TabGlow_Hover"), brush_mi("MI_UI_TabGlow_Hover"),
                              selected=brush_mi("MI_UI_TabGlow_" + key)),
                       texts(ts("TS_Heading_Muted"), selected=ts("TS_Heading_" + key)))

        # 10. BS_SlotBot - multi bot card (click -> bot menu)
        make_style("BS_SlotBot", 398, 88, (12, 0, 24, 0),
                   states(brush_mi("MI_UI_Cut_Slot"), brush_mi("MI_UI_Cut_Btn_Hovered"),
                          brush_mi("MI_UI_Cut_Btn_Pressed"), selected=brush_mi("MI_UI_Cut_Btn_Hovered")),
                   texts(ts("TS_BodyStrong")))
    except Exception as e:
        failed.append("setup ({})".format(e))

    unreal.log("========== Lobby button styles + icons ==========")
    unreal.log("Created ({}): {}".format(len(created), ", ".join(created)))
    unreal.log("Skipped, already exists ({}): {}".format(len(skipped), ", ".join(skipped)))
    for w in warnings:
        unreal.log_warning(w)
    if failed:
        unreal.log_error("Failed ({}): {}".format(len(failed), " | ".join(failed)))
    unreal.log("=================================================")


main()
