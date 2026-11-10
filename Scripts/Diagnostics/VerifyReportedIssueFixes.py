"""Run via Unreal MCP execute_python in dedicated-server + client PIE.
Creates transient test actors; writes evidence under Saved/Diagnostics/ReportedIssues.
The caller stops PIE after completion. No assets are saved by this fixture.
"""
import unreal, json, time, pathlib, traceback, builtins

root=pathlib.Path(unreal.Paths.project_saved_dir())/'Diagnostics/ReportedIssues'
root.mkdir(parents=True,exist_ok=True)
cw=unreal.EditorLevelLibrary.get_pie_worlds(False)[0]
sw=next(w for w in unreal.EditorLevelLibrary.get_pie_worlds(True) if w!=cw)
assert 'Lvl_PortalMap' in sw.get_path_name(), 'Run this fixture in Lvl_PortalMap dedicated-server/client PIE'
lib=unreal.get_default_object(unreal.GameplayStatics.static_class())
state={'start':time.monotonic(),'phase':'init','buff_trials':[],'done':False}
def component(p,name):return next(c for c in p.get_components_by_class(unreal.ActorComponent) if c.get_name()==name)
def save():
 (root/'fix-regression.json').write_text(json.dumps({k:v for k,v in state.items() if k not in ['handle','item','pawns','buffs','spc','cpc']},ensure_ascii=False,indent=2,default=str),encoding='utf-8')
def stop():
 unreal.unregister_slate_post_tick_callback(state['handle']);state['done']=True;save()
def rows(gs):
 gs.call_method('ShowResultWidget')
 widget=gs.get_editor_property('ResultWidgetRef');scroll=widget.get_editor_property('ScrollBox_Results')
 return [{k:str(scroll.get_child_at(i).get_editor_property(k).get_text()) for k in ['Text_Name','Text_Rank','Text_Kill','Text_Death']} for i in range(scroll.get_children_count())]
def tick(dt):
 try:
  now=time.monotonic()
  if now-state['start']>60:raise AssertionError('Fixture timed out')
  if state['phase']=='init':
   if now-state['start']<4:return
   spc=unreal.GameplayStatics.get_player_controller(sw,0);cpc=unreal.GameplayStatics.get_player_controller(cw,0)
   p1=spc.get_controlled_pawn()
   ai_pawns=[c.get_controlled_pawn() for c in unreal.GameplayStatics.get_all_actors_of_class(sw,unreal.AIController) if c.get_controlled_pawn()]
   if not p1 or not ai_pawns:return
   p2=ai_pawns[0]
   for ai in unreal.GameplayStatics.get_all_actors_of_class(sw,unreal.AIController):
    ai.stop_movement()
    if ai.brain_component:ai.brain_component.stop_logic('Reported issue regression')
   state.update(pawns=[p1,p2],buffs=[component(p,'BPC_Buff') for p in [p1,p2]],spc=spc,cpc=cpc,phase='spawn')
  elif state['phase']=='spawn':
   for b in state['buffs']:b.call_method('OnBuffTimerFinished')
   for i,p in enumerate(state['pawns']):
    p.set_editor_property('can_be_damaged',False);p.get_movement_component().disable_movement();p.set_actor_location(unreal.Vector(i*90,0,50000),False,False)
   cp=state['cpc'].get_controlled_pawn();cp.get_movement_component().disable_movement();cp.set_actor_location(unreal.Vector(0,0,50000),False,False)
   cls=unreal.load_class(None,'/Game/QuakeLike_1_0/DropItem/BP_BuffItem.BP_BuffItem_C')
   tf=unreal.Transform(location=unreal.Vector(2000,0,50000))
   item=lib.call_method('BeginDeferredActorSpawnFromClass',(sw,cls,tf,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None))
   table=unreal.load_object(None,'/Game/QuakeLike_1_0/Data/Item/DropTable/DT_BuffTable.DT_BuffTable')
   item.set_editor_property('DataTable Handle',unreal.DataTableRowHandle(data_table=table,row_name='Buff.Multi.Atk'))
   lib.call_method('FinishSpawningActor',(item,tf))
   state.update(item=item,ready=now+1,phase='overlap')
  elif state['phase']=='overlap' and now>=state['ready']:
   item=state['item'];trial={'item':item.get_path_name(),'distance':90}
   if not state['buff_trials']:
    dummy=lib.call_method('BeginDeferredActorSpawnFromClass',(sw,unreal.Actor.static_class(),unreal.Transform(),unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None))
    lib.call_method('FinishSpawningActor',(dummy,unreal.Transform()))
    sphere=next(c for c in item.get_components_by_class(unreal.SphereComponent))
    sphere.on_component_begin_overlap.broadcast(sphere,dummy,None,0,False,unreal.HitResult())
    trial['failure_released_lock']=not item.get_editor_property('bPickupInProgress') and not item.get_editor_property('IsPickedUp')
    assert trial['failure_released_lock'],'Failed pickup retained lock'
    dummy.destroy_actor()
   for i,p in enumerate(state['pawns']):p.set_actor_location(unreal.Vector(i*90,0,50000),False,False)
   item.set_actor_location(unreal.Vector(45,0,50000),False,False)
   ends=[b.get_editor_property('buffEndServerTime') for b in state['buffs']]
   trial.update(ends=ends,picked=item.get_editor_property('IsPickedUp'),locked=item.get_editor_property('bPickupInProgress'))
   assert sum(e>0 for e in ends)==1,trial
   assert trial['picked'] and not trial['locked'],trial
   item.set_actor_location(unreal.Vector(2000,0,50000),False,False);item.set_actor_location(unreal.Vector(45,0,50000),False,False)
   trial['reentry_ignored']=[b.get_editor_property('buffEndServerTime') for b in state['buffs']]==ends
   assert trial['reentry_ignored'],trial
   state['buff_trials'].append(trial);item.destroy_actor();save()
   if len(state['buff_trials'])<6:state['phase']='spawn'
   else:
    unreal.GameplayStatics.get_game_mode(sw).call_method('EndMatch');state.update(phase='result',ready=now+1)
  elif state['phase']=='result' and now>=state['ready']:
   gs=unreal.GameplayStatics.get_game_state(cw)
   if not gs.get_editor_property('bIsMatchEnded'):return
   state['result_original']=rows(gs)
   sgs=unreal.GameplayStatics.get_game_state(sw);sgs.get_editor_property('FinalRankingList').reverse();sgs.force_net_update()
   state.update(phase='reversed',ready=now+1)
  elif state['phase']=='reversed' and now>=state['ready']:
   gs=unreal.GameplayStatics.get_game_state(cw);state['result_reversed']=rows(gs)
   name=state['cpc'].player_state.get_player_name()
   a=[r['Text_Name'] for r in state['result_original']];b=[r['Text_Name'] for r in state['result_reversed']]
   assert a.count(name)==1 and b.count(name)==1,(a,b,name)
   assert b==list(reversed(a)),(a,b)
   assert a.index(name)!=b.index(name),(a,b)
   assert all(n and n not in ['None','C01','C02','C03','C04','C05','C06'] for n in a),a
   state['player_name']=name;state['passed']=True;stop()
 except Exception:
  state['error']=traceback.format_exc();state['passed']=False;stop()

state['handle']=unreal.register_slate_post_tick_callback(tick)
builtins.reported_issue_regression=state
print(json.dumps({'regression':'started'}))
