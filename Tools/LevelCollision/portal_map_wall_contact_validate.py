"""Actual PIE character contact/drop regression checks for non-walkable walls."""

import traceback
import unreal


class WallContactValidation:
    def __init__(self, movement, cases):
        self.world,self.pawn,self.move=movement.world,movement.pawn,movement.move
        self.radius,self.height=movement.radius,movement.height
        self.pawns=movement.pawns
        self.cases=cases
        self.results=[]
        self.index=0
        self.current=None
        self.handle=None
        self.done=False
        self.error=None

    @staticmethod
    def vector(p):
        return [round(p.x,3),round(p.y,3),round(p.z,3)]

    def begin(self):
        self.handle=unreal.register_slate_post_tick_callback(self.tick)

    def stop(self):
        if self.handle is not None:
            unreal.unregister_slate_post_tick_callback(self.handle)
            self.handle=None
        self.move.stop_movement_immediately()
        self.done=True

    def start(self,case,now):
        self.move.stop_movement_immediately()
        self.pawn.consume_movement_input_vector()
        self.pawn.set_actor_location(unreal.Vector(*case['start']),False,True)
        self.move.set_movement_mode(unreal.MovementMode.MOVE_FALLING if case['kind']=='wall_drop' or case.get('airborne')
                                    else unreal.MovementMode.MOVE_WALKING)
        self.move.set_editor_property('max_walk_speed',case.get('speed',800.))
        self.current={**case,'began':now,'samples':[],'worst_penetration':0.}

    def finish(self,passed,reason,now):
        c=self.current
        p=self.pawn.get_actor_location()
        self.results.append({k:v for k,v in c.items() if k!='began'}|{
            'passed':passed,'reason':reason,'seconds':round(now-c['began'],3),
            'end':self.vector(p),'mode':str(self.move.movement_mode)})
        self.index+=1
        self.current=None

    def tick(self,_delta):
        try:
            now=unreal.GameplayStatics.get_time_seconds(self.world)
            if self.current is None:
                if self.index==len(self.cases):
                    self.stop();return
                self.start(self.cases[self.index],now)
            c=self.current;p=self.pawn.get_actor_location();elapsed=now-c['began']
            if not c['samples'] or elapsed-c['samples'][-1][0]>.2:
                c['samples'].append([round(elapsed,3),self.vector(p)])
            if c['kind']=='wall_push':
                n=unreal.Vector(*c['normal']);plane=unreal.Vector(*c['point'])
                gap=(p-plane).dot(n)
                c['worst_penetration']=max(c['worst_penetration'],self.radius-gap)
                if elapsed>.15:
                    self.pawn.add_movement_input(n*-1,1.,True)
                if elapsed>1.2:
                    drop=c['start'][2]-p.z
                    self.finish(c['worst_penetration']<5 and (not c.get('airborne') or drop>80),
                        'max capsule penetration %.2f cm; downward displacement %.1f cm'%(c['worst_penetration'],drop),now)
            elif elapsed>1.7:
                hit=unreal.SystemLibrary.line_trace_single_by_profile(
                    self.world,p,p-unreal.Vector(0,0,self.height+15),'Pawn',False,
                    self.pawns,unreal.DrawDebugTrace.NONE)
                owner=hit.to_tuple()[9] if hit else None
                on_wall=bool(owner and str(owner.get_folder_path()).endswith('Wall'))
                grounded=self.move.is_moving_on_ground()
                fall=c['start'][2]-p.z
                c['ground_actor']=owner.get_actor_label() if owner else None
                self.finish(not(grounded and on_wall) and (grounded or fall>20),
                    'grounded=%s wall_base=%s downward displacement=%.1f cm'%(grounded,on_wall,fall),now)
        except Exception:
            self.error=traceback.format_exc();self.stop()

    def status(self):
        return {'done':self.done,'completed':len(self.results),'total':len(self.cases),
                'passed':sum(r['passed'] for r in self.results),'error':self.error,
                'current':self.current['name'] if self.current else None,
                'failures':[r for r in self.results if not r['passed']]}
