"""Run only through Unreal MCP execute_python after building ShootingArenaEditor."""
import json
from pathlib import Path
import unreal


def configure():
    targets = []
    def root(path, section):
        targets.append((path, None, section))

    vendor = '/Game/CompShooterHUD/UI/Content/'
    for path in ['HealthBar/WB_HB_Container', 'ArmorBar/WB_AB_Container',
                 'WeaponBar/WB_WeaponBar_Container', 'ActionBar/WB_ActionBar_Base']:
        root(vendor + path, 'Combat')
    for path, section in [
        ('Timer/WB_Timer', 'Timer'), ('TeamKillCount/WB_TKC_Base', 'Ranking'),
        ('PersonaKillCount/WB_PersonaKillCount', 'Ranking'), ('Kill_Message/WB_KillMessage', 'KillFeed'),
        ('Medals/WB_Medals_Base', 'Medals'), ('AchievementMessage/WB_Achievement_Base', 'Medals'),
        ('IntermediatePoints/WB_IntPoints_Base', 'Medals'), ('Crosshairs/WB_CrosshairBase', 'Crosshair'),
        ('DamageFeedback/WB_DamageFeedback', 'DamageEffects'),
        ('DamageIndicator/WB_DamageIndicator_Base', 'DamageEffects'),
        ('MainTutorialMessage/WB_MainTutorialMessage', 'Menus')]:
        root(vendor + path, section)
    base = '/Game/QuakeLike_Base/SDM/UI/Combat_UI/'
    for name, section in [('WBP_ScoreBoard', 'Scoreboard'), ('WBP_KillLog', 'KillFeed'),
                          ('WBP_PlayerKIll', 'KillFeed'), ('WBP_PlayerHit', 'DamageEffects'), ('WBP_Playtime', 'Timer')]:
        root(base + name, section)
    for name, section in [('VerticalBox_73', 'Combat'), ('InactiveVeritcalBox', 'Combat'),
                          ('BuffStatusColumn', 'Buffs'), ('HitZone', 'Crosshair')]:
        targets.append((base + 'WBP_PlayerHUD_sdm', name, section))
    for name in ['WB_FX_Damage', 'WB_FX_Highlight']:
        targets.append(('/Game/CompShooterHUD/UI/WB_ShooterHUD', name, 'DamageEffects'))
    game = '/Game/QuakeLike_1_0/'
    for path, section in [
        ('UI/DeathUI/WBP_DeathScreen', 'DeathScreen'), ('UI/PlayerUI/WBP_PlayerHUD', 'Combat'),
        ('UI/WBP_Crosshair', 'Crosshair'), ('UI/WBP_RailGunZoomOverlay', 'Crosshair'),
        ('UI/WBP_ItemRespawnBar', 'Combat'), ('GameFlow/UI/WBP_Result', 'Results'),
        ('GameFlow/UI/WBP_Cursor', 'Cursor')]:
        root(game + path, section)
    for path in [
        'GameFlow/UI/03_Widget/WBP_RootLayout', 'GameFlow/UI/03_Widget/WBP_MapPickPop',
        'GameFlow/UI/03_Widget/WBP_QuitDialog', 'GameProgressFlow/WBP_PauseMenu',
        'GameProgressFlow/WBP_SettingsMenu', 'GameFlow/UI/WBP_IPInput',
        'GameFlow/UI/WBP_MainMenu', 'GameFlow/UI/WBP_MapSelect', 'GameFlow/UI/WBP_MapSelect1',
        'GameFlow/UI/WBP_DifficultySelect', 'GameFlow/UI/WBP_DifficultySelect_SDM',
        'GameFlow/UI/WBP_MainMenu_SDM', 'GameFlow/UI/WBP_MapSelect_SDM',
        'GameFlow/Lobby/WBP_MultiLobby', 'GameFlow/Lobby/WBP_MultiConnect',
        'GameFlow/Lobby/WBP_MapSelectPopup', 'GameFlow/Lobby/WBP_BotMenu',
        'GameFlow/Lobby/WBP_BotPicker', 'GameFlow/Lobby/WBP_ConfirmModal',
        'UI/MapTransitionUI/WBP_MapTransitionUI', 'UI/ServerUI/WBP_ServerConnectUI',
        'UI/ServerUI/WBP_ServerMapChangeTestUI', 'Camera/WBP_FOVTest',
        'Test/UI/WBP_DebugAI', 'Player/WBP_PlayerOption']:
        root(game + path, 'Loading' if path in ['UI/MapTransitionUI/WBP_MapTransitionUI', 'UI/ServerUI/WBP_ServerConnectUI'] else 'Menus')

    result = []
    for path, widget, section in targets:
        bp = unreal.load_asset(path)
        assert bp, path
        info = json.loads(unreal.MCPythonHelper.umg_get_widget_info(bp))
        assert info.get('success'), info
        widget = widget or info['root_widget']
        wrapper = 'RecordingUI_' + section + '_' + (widget if not widget.startswith('RecordingUI_') else 'Root')
        # A repeated run keeps the already-installed root mask.
        if widget.startswith('RecordingUI_'):
            wrapper = widget
        elif not any(x['name'] == wrapper for x in info['widgets']):
            assert unreal.EditorAssetLibrary.save_loaded_asset(bp), path
            wrapped = json.loads(unreal.MCPythonHelper.umg_wrap_widget(bp, widget, '/Script/ShootingArena.RecordingUIMask', wrapper))
            assert wrapped.get('success'), wrapped
        prop = json.loads(unreal.MCPythonHelper.umg_set_widget_property(bp, wrapper, 'Section', section))
        assert prop.get('success'), prop
        compiled = json.loads(unreal.MCPythonHelper.compile_blueprint(bp))
        assert compiled.get('success'), compiled
        assert unreal.EditorAssetLibrary.save_loaded_asset(bp), path
        content = next((x['name'] for x in info['widgets'] if x.get('parent') == wrapper), widget)
        result.append({'asset': bp.get_path_name(), 'content': content, 'mask': wrapper, 'section': section})
    output = Path(unreal.Paths.project_dir()) / 'docs/repository-audit/systems/evidence/ui-recording.json'
    output.write_text(json.dumps({'bindings': result}, ensure_ascii=False, indent=2), encoding='utf-8')
    return {'success': True, 'bindings': len(result), 'evidence': str(output)}
