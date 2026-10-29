"""Instance-only ramp collision with ordinary bidirectional walking NavLinks.

The visual mesh and its current user placement are read-only. Navigation face
generation remains incomplete in this location; links explicitly bridge that
gap and must be validated with actual AI path following before saving.
"""
import importlib.util
import json
import math
import os
import shutil

import unreal

_spec=importlib.util.spec_from_file_location('ramp_patch_simple',
    os.path.join(os.path.dirname(__file__),'portal_map_simplify.py'))
simple=importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(simple)
V,xyz=simple.V,simple.xyz


class RampPatch(simple.Simplifier):
    def __init__(self,label,report_dir):
        super().__init__(report_dir)
        self.source=next(a for a in self.actors if a.get_actor_label()==label)
        self.c=self.source.static_mesh_component
        if self.c.static_mesh.get_name()!='SM_Ramp01':raise RuntimeError('Expected SM_Ramp01')
        self.source_pose=simple.serialize_transform(self.source.get_actor_transform())
        self.source_mesh=self.c.static_mesh.get_path_name()
        self.proxy=next((a for a in self.old_proxies if self.source_for(a)==self.source),None)
        tag='PCFNavLinkSource='+self.source.get_name()
        self.link_actor=next((a for a in self.actors if tag in [str(t) for t in a.tags]),None)
        self.before={'transform':self.source_pose,'mesh':self.source_mesh,
            'collision_profile':str(self.c.get_collision_profile_name()),
            'collision_enabled':self.c.get_collision_enabled().value,
            'navigation_relevant':self.c.get_editor_property('can_ever_affect_navigation')}

    def backup(self):
        for actor in (self.source,self.proxy,self.link_actor,self.nav):
            if not actor:continue
            rel=actor.get_package().get_name().replace('/Game/','')+'.uasset'
            src=os.path.join(unreal.Paths.project_content_dir(),rel)
            dst=os.path.join(self.report_dir,'package_backup',rel)
            if os.path.isfile(src) and not os.path.isfile(dst):
                os.makedirs(os.path.dirname(dst),exist_ok=True);shutil.copy2(src,dst)
        filename=os.path.join(self.report_dir,'implementation_before.json')
        if not os.path.isfile(filename):
            with open(filename,'w',encoding='utf-8') as f:json.dump(self.before,f,indent=2)

    def plan(self):
        transform=self.source.get_actor_transform()
        direction=simple.unit(unreal.MathLibrary.transform_direction(transform,V(1,0,0)))
        lateral=V(-direction.y,direction.x,0)
        lo,hi=self.c.get_local_bounds()
        origin=unreal.MathLibrary.transform_location(transform,V(lo.x,(lo.y+hi.y)/2,0))
        length=(hi.x-lo.x)*transform.scale3d.x
        width=(hi.y-lo.y)*transform.scale3d.y
        ignored=[self.source]+([self.proxy] if self.proxy else [])
        floors=[]
        for station in range(-30,int(math.ceil(length))+40,2):
            point=origin+direction*station
            hit=self.line(point+V(0,0,190),point-V(0,0,80),ignore=ignored)
            if hit:
                values=hit.to_tuple()
                if values[7].z>.9:floors.append((station,values[5],values[9].get_actor_label()))
        frame=max((r for r in floors if r[2]=='SM_EntranceDoorFrame4'),key=lambda r:r[0])
        lower=min((r for r in floors if r[0]>=length),key=lambda r:r[0])
        a=origin+direction*(frame[0]-4);a.z=frame[1].z+.2
        b=origin+direction*length;b.z=lower[1].z+.2
        tangent=simple.unit(b-a);normal=simple.unit((lateral*-1).cross(tangent))
        angle=math.degrees(math.acos(normal.z))
        if not 0<angle<35:raise RuntimeError('Unexpected connection slope; remeasure')
        self.plans=[]
        self.spec('Stair',self.source,(a+b)/2-normal*6,
            unreal.MathLibrary.make_rot_from_yz(tangent,normal),
            V(width/100,((b-a).length()+1)/100,.12),
            {'upper':xyz(a),'lower':xyz(b),'angle':angle,'collision_only':True})
        self.stair_groups=[{'sources':[self.source],'upper':a-direction*90,'lower':b+direction*90,
            'width':width,'lateral':lateral,'plans':self.plans,'path':[a,b]}]
        self.plan_data={'source_transform':self.source_pose,'direction':xyz(direction),
            'lateral':xyz(lateral),'width':width,'original_length_cm':length,
            'angle':angle,'navigation_mode':'ordinary_bidirectional_walking_links',
            'surface_generation_gap_resolved':False}
        return self.plan_data

    def apply(self):
        self.guard_source()
        with unreal.ScopedEditorTransaction('Fit ramp128 collision and walking navigation links'):
            self.c.modify();self.c.set_collision_profile_name('NoCollision',True)
            self.c.set_editor_property('can_ever_affect_navigation',False)
            self.proxy=self.upsert_group('Stair',self.source,self.plans)
            component=self.proxy.get_component_by_class(unreal.InstancedStaticMeshComponent)
            component.set_editor_property('can_character_step_up_on',unreal.CanBeCharacterBase.ECB_YES)
            component.set_editor_property('fill_collision_underneath_for_navmesh',False)
            component.set_walkable_slope_override(unreal.WalkableSlopeOverride(
                walkable_slope_behavior=unreal.WalkableSlopeBehavior.WALKABLE_SLOPE_DEFAULT))
            if not self.link_actor:
                center=V(*self.plans[0]['location'])
                self.link_actor=self.eas.spawn_actor_from_class(unreal.NavLinkProxy,center)
            actor=self.link_actor;actor.modify()
            actor.set_actor_label('PCF_NavLink_'+self.source.get_actor_label())
            actor.set_folder_path('CollisionFix/Stair')
            actor.set_editor_property('tags',[unreal.Name(simple.base.TAG),
                unreal.Name('PCFNavLinkSource='+self.source.get_name())])
            actor.set_editor_property('smart_link_is_relevant',False)
            actor.set_smart_link_enabled(False)
            links=[]
            group=self.stair_groups[0]
            for lane in (-120,0,120):
                endpoints=[group[k]+group['lateral']*lane for k in ('upper','lower')]
                projected=[self.nav_at(p,(35,35,100)) for p in endpoints]
                if any(p is None for p in projected):raise RuntimeError('Link endpoint misses Navigation')
                link=unreal.NavigationLink()
                for name,p in zip(('left','right'),projected):
                    local=unreal.MathLibrary.inverse_transform_location(actor.get_actor_transform(),p)
                    link.set_editor_property(name,local)
                link.set_editor_property('direction',unreal.NavLinkDirection.BOTH_WAYS)
                link.set_editor_property('area_class',unreal.NavArea_Default)
                link.set_editor_property('snap_radius',50.)
                link.set_editor_property('snap_height',60.)
                link.set_editor_property('use_snap_height',True)
                link.set_editor_property('max_fall_down_length',0.)
                link.set_editor_property('description','Walk across fitted ramp128 collision; lane '+str(lane))
                links.append(link)
            actor.set_editor_property('point_links',links)

    def guard_source(self):
        if simple.serialize_transform(self.source.get_actor_transform())!=self.source_pose \
                or self.c.static_mesh.get_path_name()!=self.source_mesh:
            raise RuntimeError('Visual source changed; remeasure without modifying it')

    def report(self,filename='ramp128_patch_report.json'):
        self.guard_source()
        data={'map':self.world.get_path_name(),'before':self.before,'plan':self.plan_data,
            'proxies':self.plans,'baseline_components':{},'retired':[],
            'preserved_sources':[{'actor':self.source.get_path_name(),'transform':self.source_pose,
                'mesh':self.source_mesh,'collision_enabled':0}],
            'validation':self.validation,'packages':[p.get_name() for p in self.packages],
            'dirty_maps_before':sorted(self.dirty_before),'dirty_content_before':sorted(self.dirty_contents_before)}
        path=os.path.join(self.report_dir,filename)
        with open(path,'w',encoding='utf-8') as f:json.dump(data,f,ensure_ascii=False,indent=2)
        return path

    def save(self):
        self.guard_source()
        for key in ('runtime_direct_movement','runtime_ai_path_following'):
            status=self.validation.get(key,{}).get('status',{})
            if not status.get('done') or status.get('error') or status.get('passed')!=6:
                raise RuntimeError('Six validated traversals required before saving: '+key)
        if not all(p.get('valid') for p in self.validation.get('navigation_paths',[])) \
                or len(self.validation.get('navigation_paths',[]))!=3:
            raise RuntimeError('Three complete navigation paths required before saving')
        self.packages=[a.get_package() for a in (self.source,self.proxy,self.link_actor,self.nav)]
        if not unreal.EditorLoadingAndSavingUtils.save_packages(self.packages,True):
            raise RuntimeError('Could not save fitted collision and walking navigation')
        return self.report()
