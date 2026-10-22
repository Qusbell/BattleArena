"""Execute only through Unreal MCP execute_python. Inline directional LTG checks in Filtering STG."""
import json
from pathlib import Path
import unreal

asset = "/Game/QuakeLike_1_0/AI/AIC/BP_QuakeAIController2"
graph = "Filtering STG"
bp = unreal.load_asset(asset)
helper = unreal.MCPythonHelper

def checked(raw):
    value = json.loads(raw)
    if not value.get("success"):
        raise RuntimeError(value)
    return value

before = checked(helper.get_blueprint_graph_info(bp, graph))
names = {n["node_name"] for n in before["nodes"]}
assert {"K2Node_CallFunction_7", "K2Node_CallFunction_12", "K2Node_VariableSet_0"} <= names
folder = Path(unreal.Paths.project_saved_dir()) / "PathLinkBench"
folder.mkdir(parents=True, exist_ok=True)
backup = folder / "FilteringSTG_BeforeInlineLTG.json"
if backup.exists():
    assert json.loads(backup.read_text(encoding="utf-8")) == before, "Current graph differs from the migration snapshot"
else:
    backup.write_text(json.dumps(before, ensure_ascii=False, indent=2), encoding="utf-8")

created = {}
def add(key, kind, x, y, **values):
    data = dict(type=kind, pos_x=x, pos_y=y, **values)
    created[key] = checked(helper.add_blueprint_node(bp, graph, json.dumps(data)))["node_name"]
def call(key, target, function, x, y):
    add(key, "CallFunction", x, y, target=target, function_name=function)
def link(source, pin, target, target_pin):
    return checked(helper.connect_blueprint_pins(bp, graph, created.get(source,source), pin,
                                                created.get(target,target), target_pin))

with unreal.ScopedEditorTransaction("Inline estimated STG to LTG route comparisons"):
    # Place new distance queries after the existing arrival-time and actor-validity gates.
    add("ltg", "VariableGet", 2640, 272, variable_name="LTG")
    add("pawn", "VariableGet", 2640, 448, variable_name="MyCharacter")
    call("valid_ltg", "KismetSystemLibrary", "IsValid", 2880, 272)
    call("valid_pawn", "KismetSystemLibrary", "IsValid", 2880, 448)
    call("actors_valid", "KismetMathLibrary", "BooleanAND", 3168, 288)
    call("ltg_location", "Actor", "K2_GetActorLocation", 3536, 496)
    call("bot_ltg", "PathLinkSubsystem", "GetEstimatedRouteDistance", 3568, -32)
    call("stg_ltg", "PathLinkSubsystem", "GetEstimatedRouteDistance", 4208, -32)
    call("stg_closer", "KismetMathLibrary", "Less_DoubleDouble", 4864, 176)
    call("progress", "KismetMathLibrary", "Less_DoubleDouble", 4864, 336)
    call("both", "KismetMathLibrary", "BooleanAND", 5136, 192)
    call("queries_ok", "KismetMathLibrary", "BooleanAND", 4864, 528)
    call("decision", "KismetMathLibrary", "BooleanAND", 5408, 224)

    link("ltg", "LTG", "valid_ltg", "Object")
    link("pawn", "MyCharacter", "valid_pawn", "Object")
    link("valid_ltg", "ReturnValue", "actors_valid", "A")
    link("valid_pawn", "ReturnValue", "actors_valid", "B")
    link("ltg", "LTG", "ltg_location", "self")
    for query in ("bot_ltg", "stg_ltg"):
        link("K2Node_GetSubsystem_0", "ReturnValue", query, "self")
        link("ltg_location", "ReturnValue", query, "TargetLocation")
    link("K2Node_CallFunction_0", "location", "bot_ltg", "StartLocation")
    link("K2Node_CallFunction_1", "ReturnValue", "stg_ltg", "StartLocation")
    # Const native functions default to pure nodes. Both query outputs feed only one impure
    # comparison Branch, so the compiler shares each pure query within that data-gather phase.
    link("bot_ltg", "ReturnValue", "queries_ok", "A")
    link("stg_ltg", "ReturnValue", "queries_ok", "B")

    # Reuse the distance already calculated by the user's arrival-time section.
    link("K2Node_VariableSet_0", "Output_Get", "stg_closer", "A")
    link("bot_ltg", "OutDistance", "stg_closer", "B")
    link("stg_ltg", "OutDistance", "progress", "A")
    link("bot_ltg", "OutDistance", "progress", "B")
    link("stg_closer", "ReturnValue", "both", "A")
    link("progress", "ReturnValue", "both", "B")
    link("both", "ReturnValue", "decision", "A")
    link("queries_ok", "ReturnValue", "decision", "B")

    # Replace only this call site; keep the original pure function available to other callers.
    checked(helper.remove_blueprint_node(bp, graph, "K2Node_CallFunction_7"))
    link("actors_valid", "ReturnValue", "K2Node_IfThenElse_3", "Condition")
    link("decision", "ReturnValue", "K2Node_Knot_7", "InputPin")

    for name, x, y in (("K2Node_IfThenElse_3",3312,-16), ("K2Node_IfThenElse_1",5808,-16),
                       ("K2Node_CallArrayFunction_0",6192,0), ("K2Node_VariableGet_0",5984,64),
                       ("K2Node_Knot_4",6080,704), ("K2Node_Knot_5",2496,704),
                       ("K2Node_Knot_7",5728,240)):
        checked(helper.set_blueprint_node_position(bp,graph,name,x,y))

    system = unreal.get_default_object(unreal.SystemLibrary.static_class())
    for key, note in {
        "bot_ltg": "Bot → LTG 추정 거리(cm). 아래 비교 Branch에서만 사용.",
        "stg_ltg": "STG → LTG 추정 거리(cm). 역방향이 아님.",
        "stg_closer": "앞에서 계산한 Bot → STG 거리 재사용: Bot→STG < Bot→LTG",
        "progress": "STG→LTG < Bot→LTG",
        "queries_ok": "두 추정 조회가 모두 성공해야 통과",
        "decision": "조회 성공 AND 기존 두 거리 비교 조건",
    }.items():
        node = unreal.find_object(None,bp.get_path_name()+":"+graph+"."+created[key])
        system.call_method("SetStringPropertyByName",(node,"NodeComment",note))

after = checked(helper.get_blueprint_graph_info(bp,graph))
(folder / "FilteringSTG_InlineLTGNodes.json").write_text(json.dumps(created,indent=2),encoding="utf-8")
(folder / "FilteringSTG_AfterInlineLTG.json").write_text(json.dumps(after,ensure_ascii=False,indent=2),encoding="utf-8")
print(json.dumps(dict(created=created, nodes_before=len(before["nodes"]),nodes_after=len(after["nodes"]))))
