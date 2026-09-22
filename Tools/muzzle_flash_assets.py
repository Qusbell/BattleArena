"""Create only the self-contained muzzle-flash textures and materials in UE 5.6.

Run with UnrealEditor-Cmd.exe -ExecutePythonScript=<this file>. Existing assets are
never replaced; a failed partial run can be inspected before retrying.
"""

from pathlib import Path
import unreal


ROOT = "/Game/QuakeLike_1_0/HeldItem/NiagaraSystem/MuzzleFlashMG"
SRC = Path(unreal.Paths.project_content_dir()) / "QuakeLike_1_0/HeldItem/NiagaraSystem/MuzzleFlashMG/SourceTextures"
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
LIB = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary


def output(message):
    unreal.log(f"MFA {message}")


def import_texture(name):
    path = f"{ROOT}/Textures/{name}"
    if EAL.does_asset_exist(path):
        output(f"existing texture {path}")
        return EAL.load_asset(path)
    task = unreal.AssetImportTask()
    task.filename = str(SRC / f"{name}.png")
    task.destination_path = f"{ROOT}/Textures"
    task.destination_name = name
    task.automated = True
    task.replace_existing = False
    task.save = True
    task.factory = unreal.TextureFactory()
    TOOLS.import_asset_tasks([task])
    assert task.imported_object_paths, f"Import failed: {name}"
    texture = EAL.load_asset(path)
    texture.set_editor_property("max_texture_size", 512)
    EAL.save_loaded_asset(texture)
    output(f"imported {path}")
    return texture


def expression(material, klass, x, y, **properties):
    node = LIB.create_material_expression(material, klass, x, y)
    assert node, f"Cannot create {klass.__name__}"
    for name, value in properties.items():
        node.set_editor_property(name, value)
    return node


def material(name, blend_mode, emissive, opacity):
    path = f"{ROOT}/Materials/{name}"
    if EAL.does_asset_exist(path):
        output(f"existing material {path}")
        return EAL.load_asset(path)
    result = TOOLS.create_asset(name, f"{ROOT}/Materials", unreal.Material, unreal.MaterialFactoryNew())
    assert result, f"Cannot create {name}"
    result.set_editor_property("blend_mode", blend_mode)
    result.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    result.set_editor_property("two_sided", True)
    tex = expression(result, unreal.MaterialExpressionTextureSampleParameter2D, -800, -180,
                     parameter_name=unreal.Name("ShapeTexture"))
    color = expression(result, unreal.MaterialExpressionParticleColor, -800, 120)
    tint = expression(result, unreal.MaterialExpressionMultiply, -550, -150)
    strength = expression(result, unreal.MaterialExpressionScalarParameter, -570, -380,
                          parameter_name=unreal.Name("EmissiveStrength"), default_value=emissive)
    light = expression(result, unreal.MaterialExpressionMultiply, -300, -180)
    alpha = expression(result, unreal.MaterialExpressionMultiply, -550, 140)
    opacity_strength = expression(result, unreal.MaterialExpressionScalarParameter, -550, 350,
                                  parameter_name=unreal.Name("OpacityStrength"), default_value=opacity)
    faded = expression(result, unreal.MaterialExpressionMultiply, -300, 140)
    for source, source_pin, target, target_pin in (
        (tex, "RGB", tint, "A"), (color, "RGB", tint, "B"),
        (tint, "", light, "A"), (strength, "", light, "B"),
        (tex, "A", alpha, "A"), (color, "A", alpha, "B"),
        (alpha, "", faded, "A"), (opacity_strength, "", faded, "B"),
    ):
        assert LIB.connect_material_expressions(source, source_pin, target, target_pin)
    assert LIB.connect_material_property(light, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    assert LIB.connect_material_property(faded, "", unreal.MaterialProperty.MP_OPACITY)
    LIB.recompile_material(result)
    EAL.save_loaded_asset(result)
    output(f"created {path}")
    return result


def instance(name, parent, texture, emissive=None, opacity=None):
    path = f"{ROOT}/Materials/{name}"
    if EAL.does_asset_exist(path):
        output(f"existing instance {path}")
        return EAL.load_asset(path)
    mi = TOOLS.create_asset(name, f"{ROOT}/Materials", unreal.MaterialInstanceConstant,
                            unreal.MaterialInstanceConstantFactoryNew())
    assert mi, f"Cannot create {name}"
    LIB.set_material_instance_parent(mi, parent)
    LIB.set_material_instance_texture_parameter_value(mi, "ShapeTexture", texture)
    if emissive is not None:
        LIB.set_material_instance_scalar_parameter_value(mi, "EmissiveStrength", emissive)
    if opacity is not None:
        LIB.set_material_instance_scalar_parameter_value(mi, "OpacityStrength", opacity)
    LIB.update_material_instance(mi)
    EAL.save_loaded_asset(mi)
    output(f"created {path}")
    return mi


for folder in (ROOT, f"{ROOT}/Textures", f"{ROOT}/Materials"):
    EAL.make_directory(folder)

textures = {name: import_texture(name) for name in (
    "T_Muzzle_WhiteFlame_01", "T_Muzzle_WhiteFlame_02",
    "T_Muzzle_OrangeBlob_01", "T_Muzzle_YellowPuff_01",
)}
flame = material("M_Muzzle_Flame", unreal.BlendMode.BLEND_ADDITIVE, 8.0, 1.0)
puff = material("M_Muzzle_Puff", unreal.BlendMode.BLEND_TRANSLUCENT, 0.6, 0.4)
instance("MI_Muzzle_White_01", flame, textures["T_Muzzle_WhiteFlame_01"])
instance("MI_Muzzle_White_02", flame, textures["T_Muzzle_WhiteFlame_02"])
instance("MI_Muzzle_Orange", flame, textures["T_Muzzle_OrangeBlob_01"], emissive=4.0)
instance("MI_Muzzle_YellowPuff", puff, textures["T_Muzzle_YellowPuff_01"])
output("texture/material pass complete")
