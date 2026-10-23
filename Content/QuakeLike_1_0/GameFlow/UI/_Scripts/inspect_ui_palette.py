# -----------------------------------------------------------------------------
# READ-ONLY palette inspection. Changes nothing, saves nothing.
# Finds every Data Asset made from PDA_UIPalette (anywhere in /Game),
# plus the PDA_UIPalette class defaults, and compares each color to the spec.
# Run: Tools > Execute Python Script...  then read the Output Log.
# -----------------------------------------------------------------------------
import unreal

PDA_PATH = "/Game/QuakeLike_1_0/GameFlow/UI/02_Styles/03_Palette/PDA_UIPalette"

SPEC = [
    ("Bg900", "08090B", 1.00), ("Bg800", "15171C", 1.00), ("Bg700", "22252D", 1.00), ("Bg600", "323641", 1.00),
    ("LineSoft", "2A2D35", 1.00), ("Line", "464B57", 1.00), ("LineStrong", "6B7180", 1.00),
    ("LineHover", "A3AAB8", 1.00), ("LineFaint", "F4F6FA", 0.12),
    ("Text1", "F4F6FA", 1.00), ("Text2", "F4F6FA", 0.70), ("Text3", "F4F6FA", 0.45), ("White", "F4F6FA", 1.00),
    ("Accent", "FFC400", 1.00), ("OnAccent", "121317", 1.00), ("AccentDim", "FFC400", 0.28),
    ("OnAccentDim", "121317", 0.60), ("Enemy", "FF4D5A", 1.00),
    ("DiffEasy", "3FC6E8", 1.00), ("DiffNormal", "F4F6FA", 1.00), ("DiffHard", "FF4D5A", 1.00),
    ("DiffEasyTint", "3FC6E8", 0.16), ("DiffNormalTint", "F4F6FA", 0.10), ("DiffHardTint", "FF4D5A", 0.16),
    ("Panel94", "15171C", 0.94), ("Chip", "08090B", 0.72), ("Scrim", "08090B", 0.82), ("MapFade", "08090B", 0.92),
]


def linear_to_srgb8(c):
    c = max(0.0, min(1.0, c))
    s = c * 12.92 if c <= 0.0031308 else 1.055 * (c ** (1.0 / 2.4)) - 0.055
    return int(round(s * 255))


def to_hex(lc):
    return "{:02X}{:02X}{:02X}".format(linear_to_srgb8(lc.r), linear_to_srgb8(lc.g), linear_to_srgb8(lc.b))


def hex_dist(a, b):
    return max(abs(int(a[i:i + 2], 16) - int(b[i:i + 2], 16)) for i in (0, 2, 4))


def report(title, obj):
    unreal.log("---------- {} ----------".format(title))
    same, diff, missing = 0, [], []
    for name, spec_hex, spec_a in SPEC:
        try:
            lc = obj.get_editor_property(name)
        except Exception:
            missing.append(name)
            continue
        cur_hex, cur_a = to_hex(lc), round(lc.a, 2)
        if hex_dist(cur_hex, spec_hex) <= 1 and abs(cur_a - spec_a) < 0.011:
            same += 1
        else:
            diff.append("{:<15} now {} A{:.2f}   spec {} A{:.2f}".format(name, cur_hex, cur_a, spec_hex, spec_a))
    unreal.log("Matches spec: {} / {}".format(same, len(SPEC)))
    for line in diff:
        unreal.log_warning("DIFF  " + line)
    if missing:
        unreal.log_error("MISSING variables: " + ", ".join(missing))


def main():
    unreal.log("========== UI palette inspection (read-only) ==========")
    pda = unreal.EditorAssetLibrary.load_asset(PDA_PATH)
    if pda is None:
        unreal.log_error("PDA_UIPalette not found: " + PDA_PATH)
        return
    gen = unreal.BlueprintEditorLibrary.generated_class(pda)
    report("PDA_UIPalette class defaults", unreal.get_default_object(gen))

    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    found = 0
    for data in registry.get_assets_by_path("/Game", recursive=True):
        if str(data.asset_class_path.asset_name) != gen.get_name():
            continue
        found += 1
        asset = data.get_asset()
        report("Data Asset {}".format(data.package_name), asset)
    if found == 0:
        unreal.log_warning("No Data Asset of PDA_UIPalette found under /Game (DA_UI_Palette not created or not saved yet).")
    unreal.log("========================================================")


main()
