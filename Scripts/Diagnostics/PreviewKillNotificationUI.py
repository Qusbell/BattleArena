"""Preview the saved medal and You Killed settings in a combat PIE session.

In the editor Output Log: py "<project>/Scripts/Diagnostics/PreviewKillNotificationUI.py" 3
The preview resets the local player's PIE streak and runs the real HUD notification.
"""
import sys
import time

import unreal


def preview(count=3):
    if count < 1 or count > 100:
        raise ValueError('Choose a medal count from 1 to 100.')
    clients = unreal.EditorLevelLibrary.get_pie_worlds(False)
    if not clients:
        raise RuntimeError('Start combat PIE before previewing the HUD.')
    player = unreal.GameplayStatics.get_player_controller(clients[0], 0)
    if not player or not player.get_controlled_pawn() or not player.get_hud():
        raise RuntimeError('Wait until the local player has spawned in the match.')
    death_screen = player.get_editor_property('deathScreen')
    if death_screen and death_screen.is_visible():
        raise RuntimeError('Wait until the local player has respawned.')

    player_id = player.player_state.call_method('GetPlayerId')
    server = next(w for w in unreal.EditorLevelLibrary.get_pie_worlds(True)
                  if unreal.GameplayStatics.get_game_mode(w))
    authority = next(pc for pc in unreal.GameplayStatics.get_all_actors_of_class(server, unreal.PlayerController)
                     if pc.player_state and pc.player_state.call_method('GetPlayerId') == player_id)
    local_voice = player.player_state.get_component_by_class(unreal.KillStreakVoiceComponent)
    voice = authority.player_state.get_component_by_class(unreal.KillStreakVoiceComponent)
    messages = player.get_hud().get_editor_property('WB_ShooterHUD').get_editor_property('WB_KillMessage')
    voice.notify_owner_death()
    authority.player_state.force_net_update()
    ready = time.monotonic() + 0.2

    def show(_delta):
        try:
            now = time.monotonic()
            if now > ready + 3.0:
                raise RuntimeError('The preview streak reset did not reach the client.')
            if now < ready or local_voice.get_current_streak_count() != 0:
                return
            for _ in range(count):
                voice.queue_kill_event(False)
            authority.player_state.force_net_update()
            messages.call_method('RemoveKillMessages')
            player.call_method('Client_ShowKillMessage', (unreal.Text('Preview Target'),))
        except Exception as error:
            unreal.log_error(str(error))
        unreal.unregister_slate_post_tick_callback(handle)

    handle = unreal.register_slate_post_tick_callback(show)


if __name__ == '__main__':
    preview(int(sys.argv[1]) if len(sys.argv) > 1 else 3)
