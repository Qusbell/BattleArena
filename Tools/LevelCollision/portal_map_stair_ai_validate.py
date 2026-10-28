"""Real AIController/PathFollowing checks, confined to the authoritative PIE world."""

import math
import traceback
import unreal


class StairAIValidation:
    def __init__(self,repair,cases):
        worlds=unreal.EditorLevelLibrary.get_pie_worlds(True)
        self.world=next((w for w in worlds if unreal.GameplayStatics.get_game_mode(w)),None)
        if self.world is None:raise RuntimeError('An authoritative PIE world is required')
        self.world_path=self.world.get_path_name()
        pawns=list(unreal.GameplayStatics.get_all_actors_of_class(self.world,unreal.Character))
        if not pawns:raise RuntimeError('No spawned AI character')
        for controller in unreal.GameplayStatics.get_all_actors_of_class(self.world,unreal.AIController):
            brain=controller.get_component_by_class(unreal.BrainComponent)
            if brain:brain.stop_logic('Temporary stair navigation verification')
            controller.stop_movement();controller.set_actor_tick_enabled(False)
            for c in controller.get_components_by_class(unreal.ActorComponent):c.set_component_tick_enabled(False)
        self.pawns=pawns;self.pawn=pawns[0]
        for pawn in pawns:
            pawn.set_actor_tick_enabled(False);pawn.set_editor_property('can_be_damaged',False)
            for c in pawn.get_components_by_class(unreal.ActorComponent):
                if not isinstance(c,unreal.CharacterMovementComponent):c.set_component_tick_enabled(False)
        old=self.pawn.get_controller()
        if old:old.un_possess()
        self.pawn.set_editor_property('ai_controller_class',unreal.AIController)
        self.pawn.spawn_default_controller()
        self.controller=self.pawn.get_controller()
        if not self.controller:raise RuntimeError('Could not create the temporary native AIController')
        self.controller.set_actor_tick_enabled(True)
        for c in self.controller.get_components_by_class(unreal.ActorComponent):c.set_component_tick_enabled(True)
        self.move=self.pawn.get_component_by_class(unreal.CharacterMovementComponent)
        self.move.set_component_tick_enabled(True)
        self.move.set_editor_property('max_walk_speed',600.)
        self.cap=self.pawn.get_component_by_class(unreal.CapsuleComponent)
        self.cap.set_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN,unreal.CollisionResponseType.ECR_IGNORE)
        self.radius=self.cap.get_scaled_capsule_radius();self.height=self.cap.get_scaled_capsule_half_height()
        self.repair=repair;self.cases=cases;self.results=[];self.current=None;self.index=0
        self.done=False;self.error=None;self.handle=None

    @staticmethod
    def vec(v):return [round(v.x,3),round(v.y,3),round(v.z,3)]

    def begin(self):self.handle=unreal.register_slate_post_tick_callback(self.tick)

    def stop(self):
        if self.handle is not None:unreal.unregister_slate_post_tick_callback(self.handle);self.handle=None
        self.controller.stop_movement();self.done=True

    def start(self,case,now):
        self.controller.stop_movement();self.move.stop_movement_immediately()
        start=case['start'].copy()
        hit=unreal.SystemLibrary.line_trace_single_by_profile(self.world,start+unreal.Vector(0,0,65),
            start-unreal.Vector(0,0,180),'Pawn',False,self.pawns,unreal.DrawDebugTrace.NONE)
        nz=1.
        if hit and hit.to_tuple()[7].z>.7:start=hit.to_tuple()[5];nz=hit.to_tuple()[7].z
        start+=unreal.Vector(0,0,self.height+3+self.radius*(1/nz-1))
        self.pawn.set_actor_location(start,False,True)
        self.move.set_movement_mode(unreal.MovementMode.MOVE_WALKING)
        request=self.controller.move_to_location(case['end'],acceptance_radius=20,stop_on_overlap=False,
            use_pathfinding=True,project_destination_to_navigation=True,can_strafe=True,allow_partial_path=False)
        self.current={**case,'began':now,'request':str(request),'samples':[],'used_stair':False}

    def on_stair(self,position):
        d=unreal.Vector(*self.repair.plan_data['direction']);l=unreal.Vector(*self.repair.plan_data['lateral'])
        s=position.dot(d)
        source=self.repair.source;lo,hi=source.static_mesh_component.get_local_bounds();t=source.get_actor_transform()
        a=unreal.MathLibrary.transform_location(t,unreal.Vector(lo.x,(lo.y+hi.y)/2,0))
        b=unreal.MathLibrary.transform_location(t,unreal.Vector(hi.x,(lo.y+hi.y)/2,0))
        spans=[(unreal.Vector(*p['evidence']['upper']).dot(d),
                unreal.Vector(*p['evidence']['lower']).dot(d)) for p in self.repair.plans]
        first=min(min(span) for span in spans);last=max(max(span) for span in spans)
        progress=(s-first)/(last-first)
        if not .05<progress<.95 or abs((position-a).dot(l))>self.repair.plan_data['width']/2-self.radius/2:return False
        heights=[]
        for plan in self.repair.plans:
            aa,bb=(unreal.Vector(*plan['evidence'][k]) for k in ('upper','lower'))
            sa,sb=aa.dot(d),bb.dot(d)
            if min(sa,sb)-1<=s<=max(sa,sb)+1:
                f=(s-sa)/(sb-sa)
                heights.append(aa.z+(bb.z-aa.z)*f)
        if not heights or abs(position.z-self.height-max(heights))>=65:return False
        # Short ramps can be crossed in one low-frame-rate tick. Use the
        # fitted collision footprint and verify its actual support actor,
        # rather than miss it with a window based on the visual source bounds.
        support=unreal.SystemLibrary.line_trace_single_by_profile(self.world,position,
            position-unreal.Vector(0,0,self.height+90),'Pawn',False,self.pawns,unreal.DrawDebugTrace.NONE)
        if not support:return False
        proxy_names={p['proxy_actor'].rsplit('.',1)[-1] for p in self.repair.plans}
        return support.to_tuple()[9].get_name() in proxy_names

    def finish(self,passed,reason,now):
        c=self.current;p=self.pawn.get_actor_location()
        self.results.append({'name':c['name'],'direction':c['direction'],'passed':passed,'reason':reason,
            'request':c['request'],'used_stair':c['used_stair'],'seconds':round(now-c['began'],2),
            'start':self.vec(c['start']),'target':self.vec(c['end']),'end':self.vec(p),'samples':c['samples']})
        self.current=None;self.index+=1

    def tick(self,_delta):
        try:
            now=unreal.GameplayStatics.get_time_seconds(self.world)
            if self.current is None:
                if self.index==len(self.cases):self.stop();return
                self.start(self.cases[self.index],now)
            c=self.current;p=self.pawn.get_actor_location();elapsed=now-c['began']
            c['used_stair']|=self.on_stair(p)
            if not c['samples'] or elapsed-c['samples'][-1][0]>.2:c['samples'].append([round(elapsed,2),self.vec(p)])
            distance=math.hypot(p.x-c['end'].x,p.y-c['end'].y)
            status=self.controller.get_move_status()
            if elapsed>.35 and status==unreal.PathFollowingStatus.IDLE:
                self.finish(distance<65 and c['used_stair'],
                    'PathFollowing finished; remaining %.1f cm; used stair=%s'%(distance,c['used_stair']),now)
            elif elapsed>15:
                self.finish(False,'AI path following timed out; remaining %.1f cm'%distance,now)
        except Exception:self.error=traceback.format_exc();self.stop()

    def status(self):
        return {'done':self.done,'completed':len(self.results),'total':len(self.cases),
            'passed':sum(r['passed'] for r in self.results),'error':self.error,
            'radius':self.radius,'half_height':self.height,'world':getattr(self,'world_path','completed PIE session'),
            'failures':[r for r in self.results if not r['passed']]}
