# -----------------------------------------------------------------------------
# Fix: rebuilds the node graphs of M_UI_Cut, M_UI_TabGlow, M_UI_MapFade only
# (they were created by create_ui_materials.py with an alpha-channel bug).
# Then creates any instances that are still missing. Existing instances keep
# their parameter values (parameter names are unchanged). Nothing is deleted
# except the nodes inside those 3 materials.
# Run: Tools > Execute Python Script...
# -----------------------------------------------------------------------------
import os
import unreal

try:
    here = os.path.dirname(os.path.abspath(__file__))
except NameError:
    here = os.path.join(unreal.Paths.project_content_dir(), "QuakeLike_1_0", "GameFlow", "UI", "_Scripts")
script = os.path.join(here, "create_ui_materials.py")
with open(script, encoding="utf-8") as f:
    code = f.read()
exec(compile(code, script, "exec"), {"__name__": "__main__", "REBUILD_MATERIALS": True})
