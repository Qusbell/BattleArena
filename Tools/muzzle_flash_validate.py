"""Read-only validation of the isolated MG muzzle-flash asset and dependencies.

Run through UnrealEditor-Cmd.exe -ExecutePythonScript=<absolute path>.
"""

import unreal


ROOT = "/Game/QuakeLike_1_0/HeldItem/NiagaraSystem/MuzzleFlashMG"
LIB = unreal.EditorAssetLibrary
MATERIALS = unreal.MaterialEditingLibrary


def asset(relative):
    path = f"{ROOT}/{relative}"
    assert LIB.does_asset_exist(path), f"Missing asset: {path}"
    value = LIB.load_asset(path)
    assert value is not None, f"Failed loading: {path}"
    unreal.log(f"MFV loaded {path}: {value.get_class().get_name()}")
    return value


system = asset("NS_MuzzleFlash_MG_Stylized")
assert system.get_class().get_name() == "NiagaraSystem"

for name in (
    "T_Muzzle_WhiteFlame_01", "T_Muzzle_WhiteFlame_02",
    "T_Muzzle_OrangeBlob_01", "T_Muzzle_YellowPuff_01",
):
    asset(f"Textures/{name}")

for name in ("M_Muzzle_Flame", "M_Muzzle_Puff"):
    asset(f"Materials/{name}")

for name in (
    "MI_Muzzle_White_01", "MI_Muzzle_White_02",
    "MI_Muzzle_Orange", "MI_Muzzle_YellowPuff",
):
    instance = asset(f"Materials/{name}")
    texture = MATERIALS.get_material_instance_texture_parameter_value(
        instance, "ShapeTexture"
    )
    assert texture is not None, f"No ShapeTexture: {name}"
    assert texture.get_path_name().startswith(ROOT + "/Textures/"), name
    unreal.log(f"MFV material {name} -> {texture.get_path_name()}")

unreal.log("MFV all 11 isolated assets load; four material textures resolve")
