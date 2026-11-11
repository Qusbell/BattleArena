"""Run through Unreal MCP in Lvl_PortalMap server/client PIE; stop PIE afterward.

Exercises the real personal-message widgets in both event orders and after respawn.
Only transient PIE state changes; evidence is written under Saved/Diagnostics.
"""
import builtins
import json
from pathlib import Path
import time
import traceback
import unreal


world = unreal.EditorLevelLibrary.get_pie_worlds(False)[0]
assert 'Lvl_PortalMap' in world.get_path_name()
pc = unreal.GameplayStatics.get_player_controller(world, 0)
hud = pc.get_editor_property('LifeAndWeapon')
messages = pc.get_hud().get_editor_property('WB_ShooterHUD').get_editor_property('WB_KillMessage')
content = messages.get_editor_property('OV_Content')
assert hud.get_owning_player() == pc
state = {'done': False, 'cases': [], 'next': 0, 'ready': 0}
output = Path(unreal.Paths.project_saved_dir()) / 'Diagnostics/PersonalMessagePriority.json'

# Keep combat from injecting unrelated UI events into the presentation checks.
for w in unreal.EditorLevelLibrary.get_pie_worlds(True):
    for pawn in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Pawn):
        pawn.set_editor_property('can_be_damaged', False)
    for ai in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.AIController):
        ai.stop_movement()
        if ai.brain_component:
            ai.brain_component.stop_logic('Personal message priority test')


def kill():
    pc.call_method('Client_ShowKillMessage', (unreal.Text('PriorityVictim'),))


def death():
    pc.call_method('Toggle Death UI', (True,))
    hud.call_method('ShowCompetitiveDeathMessage', (unreal.Text('PriorityKiller'), 3.0))


cases = [
    ('kill_only', [kill], 'PriorityVictim'),
    ('kill_then_death', [kill, death], 'PriorityKiller'),
    ('death_then_kill', [death, kill], 'PriorityKiller'),
    ('death_then_multiple_kills', [death, kill, kill], 'PriorityKiller'),
    ('after_respawn', [death, lambda: pc.call_method('Toggle Death UI', (False,)),
                       lambda: messages.call_method('RemoveKillMessages'), kill], 'PriorityVictim'),
]


def finish():
    unreal.unregister_slate_post_tick_callback(state['handle'])
    state['done'] = True
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps({k: v for k, v in state.items() if k != 'handle'}, indent=2), encoding='utf-8')


def tick(delta):
    try:
        now = time.monotonic()
        if now < state['ready']:
            return
        if 'pending' in state:
            name, expected = state.pop('pending')
            rows = []
            for i in range(content.get_children_count()):
                item = content.get_child_at(i)
                rows.append({key: str(item.get_editor_property(key).get_text())
                             for key in ['Head1', 'Head2', 'Child1', 'Child2']})
            state['cases'].append({'name': name, 'rows': rows})
            assert len(rows) == 1, (name, rows)
            texts = list(rows[0].values())
            assert expected in texts, (name, expected, rows)
            rejected = 'PriorityVictim' if expected == 'PriorityKiller' else 'PriorityKiller'
            assert rejected not in texts, (name, rows)
        if state['next'] == len(cases):
            state['passed'] = True
            finish()
            return
        name, actions, expected = cases[state['next']]
        messages.call_method('RemoveKillMessages')
        pc.call_method('Toggle Death UI', (False,))
        for action in actions:
            action()
        state.update(pending=(name, expected), ready=now + 0.6)
        state['next'] += 1
    except Exception:
        state.update(passed=False, error=traceback.format_exc())
        finish()


state['handle'] = unreal.register_slate_post_tick_callback(tick)
builtins.personal_message_priority = state
