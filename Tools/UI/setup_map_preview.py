"""Run in Unreal Editor after compiling MapPreviewMedia.cpp.

Connects the existing map/difficulty graphs without replacing their navigation,
map selection, server startup, text, or style logic. Safe to run again.
"""
import json
import unreal

ROOT = '/Game/QuakeLike_1_0/GameFlow/UI'
MEDIA_ROOT = ROOT + '/MapPreviewMedia'
TABLE_PATH = MEDIA_ROOT + '/DT_MapPreviewMedia'
MATERIAL_PATH = MEDIA_ROOT + '/M_MapPreviewVideo'


def checked(result):
    result = json.loads(result)
    if not result.get('success'):
        raise RuntimeError(result)
    return result


def graph(bp):
    return checked(unreal.MCPythonHelper.get_blueprint_graph_info(bp, 'EventGraph'))


def add(bp, name, **spec):
    existing = next((n for n in graph(bp)['nodes'] if n['node_name'] == name), None)
    if existing:
        return name
    result = checked(unreal.MCPythonHelper.add_blueprint_node(bp, 'EventGraph', json.dumps(spec)))
    # Stable names make rerunning this setup deterministic.
    node = unreal.find_object(None, bp.get_path_name() + ':EventGraph.' + result['node_name'])
    if node is None:
        raise RuntimeError('Cannot find newly created node: ' + str(result))
    node.rename(name)
    return name


def connect(bp, source, source_pin, target, target_pin):
    checked(unreal.MCPythonHelper.connect_blueprint_pins(bp, 'EventGraph', source, source_pin, target, target_pin))


def default(bp, node, pin, value):
    checked(unreal.MCPythonHelper.set_blueprint_node_pin_default(bp, 'EventGraph', node, pin, value))


def configure_screen(name, tail, image_node, image_pin, map_node, map_pin, always_play):
    bp = unreal.load_asset(ROOT + '/' + name)
    tail_info = next(n for n in graph(bp)['nodes'] if n['node_name'] == tail)
    setup_x, setup_y = tail_info['pos_x'] + 480, tail_info['pos_y']
    self_node = add(bp, 'K2Node_MapPreviewSelf', type='Self', pos_x=setup_x - 240, pos_y=setup_y + 320)
    setup = add(bp, 'K2Node_ConfigureMapPreview', type='CallFunction',
        target='MapPreviewLibrary', function_name='ConfigureMapPreview', pos_x=setup_x, pos_y=setup_y)
    default(bp, setup, 'PreviewMedia', TABLE_PATH)
    default(bp, setup, 'VideoMaterial', MATERIAL_PATH)
    default(bp, setup, 'bAlwaysPlay', 'true' if always_play else 'false')
    connect(bp, self_node, 'self', setup, 'Screen')
    connect(bp, image_node, image_pin, setup, 'Image')
    connect(bp, map_node, map_pin, setup, 'MapId')
    connect(bp, tail, 'then', setup, 'execute')

    destruct_events = [n for n in graph(bp)['nodes']
        if n['node_class'] == 'K2Node_Event' and any(s in n['node_title'] for s in ['Destruct', '디스트럭트'])]
    if destruct_events:
        destruct = destruct_events[0]['node_name']
        output = next(p for p in destruct_events[0]['pins'] if p['pin_name'] == 'then')
        if any(link['node_name'] != 'K2Node_ReleaseMapPreviews' for link in output.get('linked_to', [])):
            raise RuntimeError('Existing Destruct logic needs an explicit insertion: ' + name)
    else:
        destruct = add(bp, 'K2Node_MapPreviewDestruct', type='Event', event_name='Destruct', pos_x=1800, pos_y=4400)
    release = add(bp, 'K2Node_ReleaseMapPreviews', type='CallFunction',
        target='MapPreviewLibrary', function_name='ReleaseMapPreviews', pos_x=2200, pos_y=4400)
    connect(bp, self_node, 'self', release, 'Screen')
    connect(bp, destruct, 'then', release, 'execute')
    result = checked(unreal.MCPythonHelper.compile_blueprint(bp))
    if not unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False):
        raise RuntimeError('Save failed: ' + name)
    return {'asset': bp.get_path_name(), 'compile': result}


def run():
    assert unreal.load_asset(MATERIAL_PATH), 'Create the preview UI material first.'
    if not unreal.EditorAssetLibrary.does_asset_exist(TABLE_PATH):
        factory = unreal.DataTableFactory()
        factory.set_editor_property('struct', unreal.MapPreviewMediaRow.static_struct())
        table = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            'DT_MapPreviewMedia', MEDIA_ROOT, unreal.DataTable, factory)
        maps = unreal.load_asset('/Game/QuakeLike_1_0/GameFlow/Table/DT/DT_MapData')
        rows = [{'Name': str(name), 'StillImage': 'None', 'PreviewVideo': 'None'}
            for name in unreal.DataTableFunctionLibrary.get_data_table_row_names(maps)]
        assert unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows))
        unreal.EditorAssetLibrary.save_loaded_asset(table)

    return [
        configure_screen('WBP_MapSelect', 'K2Node_MacroInstance_2',
            'K2Node_Knot_35', 'OutputPin', 'K2Node_DisplayedRowName', 'Item', False),
        configure_screen('WBP_DifficultySelect', 'K2Node_VariableSet_8',
            'K2Node_VariableGet_17', 'Lvl_Image', 'K2Node_VariableGet_16', 'Selected_Map_ID', True),
    ]


if __name__ == '__main__':
    print(json.dumps(run(), ensure_ascii=False))
