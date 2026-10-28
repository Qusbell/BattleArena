"""Fit collision to one existing PortalMap stair WITHOUT changing its geometry.

The source actor and shared mesh are read-only. Only the collision proxy,
local navigation modifier and generated navigation are saved.
"""

import importlib.util
import json
import math
import os
import shutil

import unreal

_spec=importlib.util.spec_from_file_location('portal_stair_simple',
    os.path.join(os.path.dirname(__file__),'portal_map_simplify.py'))
simple=importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(simple)
V,dot,unit,xyz=simple.V,simple.dot,simple.unit,simple.xyz


class StairConnector(simple.Simplifier):
    def __init__(self,label,report_dir):
        super().__init__(report_dir)
        self.proxy=next(a for a in self.actors if a.get_actor_label()==label)
        self.source=self.source_for(self.proxy)
        if not self.source:raise RuntimeError('Stair proxy source is missing')
        self.c=self.source.static_mesh_component
        if not self.c.static_mesh or self.c.static_mesh.get_name()!='SM_RampSteps':
            raise RuntimeError('Expected the existing SM_RampSteps instance')
        self.before={}
        self.plan_data={}
        self.dependencies=[]

    def backup(self):
        for a in [self.source,self.proxy]+self.dependencies:
            c=a.get_component_by_class(unreal.InstancedStaticMeshComponent) if a==self.proxy else a.static_mesh_component
            data={'actor':a.get_path_name(),'label':a.get_actor_label(),'package':a.get_package().get_name(),
                'transform':simple.serialize_transform(a.get_actor_transform()),
                'collision_profile':str(c.get_collision_profile_name()),'collision_enabled':c.get_collision_enabled().value}
            if isinstance(c,unreal.InstancedStaticMeshComponent):
                data['instances']=[simple.serialize_transform(c.get_instance_transform(i,True)) for i in range(c.get_instance_count())]
            self.before[a.get_path_name()]=data
            rel=a.get_package().get_name().replace('/Game/','')+'.uasset'
            src=os.path.join(unreal.Paths.project_content_dir(),rel)
            dst=os.path.join(self.report_dir,'package_backup',rel)
            if os.path.isfile(src) and not os.path.isfile(dst):
                os.makedirs(os.path.dirname(dst),exist_ok=True);shutil.copy2(src,dst)
        self.write_report('before.json')

    def tread_noses(self):
        vertices,indices=self.geometry(self.c.static_mesh)
        verts=[xyz(v) for v in vertices]
        lo,hi=self.c.get_local_bounds();y=(lo.y+hi.y)/2
        triangles=[]
        for i in range(0,len(indices),3):
            a,b,c=(verts[indices[i+k]] for k in range(3))
            den=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
            if abs(den)>1e-8 and min(a[1],b[1],c[1])-.01<=y<=max(a[1],b[1],c[1])+.01:
                triangles.append((a,b,c,den))
        samples=[]
        for j in range(1024):
            x=lo.x+(hi.x-lo.x)*(j+.5)/1024
            heights=[]
            for a,b,c,den in triangles:
                u=((b[1]-c[1])*(x-c[0])+(c[0]-b[0])*(y-c[1]))/den
                v=((c[1]-a[1])*(x-c[0])+(a[0]-c[0])*(y-c[1]))/den
                if u>=-1e-6 and v>=-1e-6 and u+v<=1.000001:
                    heights.append(u*a[2]+v*b[2]+(1-u-v)*c[2])
            if heights:samples.append((x,max(heights)))
        groups=[]
        for p in samples:
            if not groups or groups[-1][-1][1]-p[1]>10:
                groups.append([p])
            else:groups[-1].append(p)
        groups=[g for g in groups if g[-1][0]-g[0][0]>12]
        if len(groups)<4:raise RuntimeError('Could not identify stair treads')
        noses=[(g[-1][0],max(z for _,z in g)) for g in groups]
        return noses,samples

    def plan(self,upper_floor_height,frame_exit_station):
        noses,samples=self.tread_noses()
        original=self.source.get_actor_transform()
        direction=unit(unreal.MathLibrary.transform_direction(original,V(1,0,0)))
        lateral=V(-direction.y,direction.x,0)
        up=unit(unreal.MathLibrary.transform_direction(original,V(0,0,1)))
        if up.z<.999:raise RuntimeError('This connector expects horizontal original treads')
        scale=original.scale3d
        lo,hi=self.c.get_local_bounds();ymid=(lo.y+hi.y)/2
        width=(hi.y-lo.y)*scale.y
        self.plans=[]
        def world(x,z):
            return unreal.MathLibrary.transform_location(original,V(x,ymid,z))
        start=world(lo.x,noses[0][1]);end=world(hi.x,noses[-1][1])
        source_start_s=dot(start,direction)
        exit_x=max(lo.x,min(noses[1][0]-1,lo.x+(frame_exit_station-source_start_s)/scale.x))
        frame_end=world(exit_x,noses[0][1]);frame_end.z=upper_floor_height
        top=start.copy();top.z=upper_floor_height
        second=world(*noses[1]);second.z+=.2
        def slab(a,b,index):
            tangent=unit(b-a);normal=unit((lateral*-1).cross(tangent))
            angle=math.degrees(math.acos(max(-1,min(1,normal.z))))
            if angle>44.:raise RuntimeError('Connector slope exceeds the unchanged NavMesh slope limit')
            aa,bb=a-tangent*.35,b+tangent*.35
            self.spec('Stair',self.source,(aa+bb)/2-normal*6,
                unreal.MathLibrary.make_rot_from_yz(tangent,normal),
                V(width/100,(bb-aa).length()/100,.12),
                {'upper':xyz(a),'lower':xyz(b),'angle':angle,'connector_part':index})
        # Native doorway support extends over the first tread. This short
        # connector removes its excessive first drop, inside the stair bounds.
        slab(top,frame_end,'threshold')
        slab(frame_end,second,'entry_blend')
        path=[top,frame_end,second]
        finish=world(*noses[-1]);finish.z+=.2
        run=dot(finish-second,direction);rise=second.z-finish.z
        grade=rise/run
        count=4
        ramp_grade=math.tan(math.radians(43.))
        joint=(rise-ramp_grade*run)/(count-1)
        if not 0<=joint<=10:raise RuntimeError('Unexpected stair proportions; remeasure collision')
        for i in range(count):
            a=second+direction*(run*i/count)-V(0,0,i*(ramp_grade*run/count+joint))
            b=a+direction*(run/count)-V(0,0,ramp_grade*run/count)
            slab(a,b,'grade_'+str(i));path.extend((a,b))
        self.stair_groups=[{'sources':[self.source],'plans':self.plans,'path':path,
            'upper':top-direction*75,'lower':end+direction*80,'width':width,'lateral':lateral}]
        self.plan_data={'scope':'collision_only','noses_local':noses,'source_transform':simple.serialize_transform(original),
            'path':[xyz(p) for p in path],'width':width,'angles':[p['evidence']['angle'] for p in self.plans],
            'original_length_cm':(hi.x-lo.x)*scale.x,
            'max_tread_riser_cm':max((a[1]-b[1])*scale.z for a,b in zip(noses,noses[1:])),
            'collision_joint_step_cm':joint,
            'navigation_resolution':{'preset':'High','cell_size_cm':13.,'cell_height_cm':5.,
                'agent_step_height_cm':35.,'default_cell_size_cm':19.,'default_cell_height_cm':10.},
            'direction':xyz(direction),'lateral':xyz(lateral)}
        return self.plan_data

    def apply(self):
        if simple.serialize_transform(self.source.get_actor_transform())!=self.before[self.source.get_path_name()]['transform']:
            raise RuntimeError('Source geometry changed; remeasure without modifying it')
        self.packages=[];self.baseline={};self.dependencies=[]
        with unreal.ScopedEditorTransaction('Fit collision to unchanged ramp Clean51'):
            self.proxy.modify()
            self.proxy.set_actor_rotation(unreal.Rotator(pitch=0,yaw=0,roll=0),False)
            self.proxy.set_actor_scale3d(V(1,1,1))
            self.upsert_group('Stair',self.source,self.plans)
            c=self.proxy.get_component_by_class(unreal.InstancedStaticMeshComponent)
            c.set_walkable_slope_override(unreal.WalkableSlopeOverride(
                walkable_slope_behavior=unreal.WalkableSlopeBehavior.WALKABLE_SLOPE_DEFAULT))
            c.set_editor_property('can_character_step_up_on',unreal.CanBeCharacterBase.ECB_YES)
            self.configure_navigation_resolution()

    def configure_navigation_resolution(self):
        # The default 19x10 cm voxel grid leaves real holes on these steep,
        # short runs. Only tiles touched by this proxy request the High preset.
        # Keep the default grid, agent dimensions, climb and slope unchanged.
        tag='PCFNavSource='+self.source.get_name()
        self.navigation_actor=next((a for a in self.eas.get_all_level_actors()
            if tag in [str(t) for t in a.tags]),None)
        center,_,_=unreal.SystemLibrary.get_component_bounds(
            self.proxy.get_component_by_class(unreal.InstancedStaticMeshComponent))
        if self.navigation_actor is None:
            self.navigation_actor=self.eas.spawn_actor_from_class(unreal.StaticMeshActor,center)
            self.navigation_actor.set_actor_label('PCF_Nav_'+self.source.get_actor_label())
            self.navigation_actor.set_folder_path('CollisionFix/Stair')
            self.navigation_actor.set_editor_property('tags',[unreal.Name(tag)])
        actor=self.navigation_actor;actor.modify()
        actor.static_mesh_component.set_collision_profile_name('NoCollision')
        actor.static_mesh_component.set_editor_property('can_ever_affect_navigation',False)
        actor.set_actor_hidden_in_game(True)
        # A separate empty actor uses FailsafeExtent. On an ISM, the modifier
        # derives areas from the uninstanced cube BodySetup, missing the run.
        modifier=actor.get_component_by_class(unreal.NavModifierComponent)
        if modifier is None:
            subsystem=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
            handles=subsystem.k2_gather_subobject_data_for_instance(actor)
            handle,reason=subsystem.add_new_subobject(unreal.AddNewSubobjectParams(
                parent_handle=handles[0],new_class=unreal.NavModifierComponent))
            library=unreal.SubobjectDataBlueprintFunctionLibrary
            modifier=library.get_object(library.get_data(handle))
            if not modifier:raise RuntimeError('Could not create local navigation modifier: '+str(reason))
        modifier.modify()
        modifier.set_editor_property('failsafe_extent',V(400,400,400))
        modifier.set_editor_property('nav_mesh_resolution',unreal.NavigationDataResolution.HIGH)
        modifier.set_area_class_to_replace(unreal.NavArea_Default)
        modifier.set_area_class(unreal.NavArea_Default)
        params=self.nav.get_editor_property('nav_mesh_resolution_params')
        high=params[2]
        # TileSizeUU=1000, default size=19 => actual tile width=988.
        # 13 divides 988 and yields the same truncated tile width; 9.5 would
        # yield 997.5 and disconnect tiles in this engine configuration.
        high.set_editor_property('cell_size',13.)
        high.set_editor_property('cell_height',5.)
        # Array/struct getters return copies; assign the edited struct back.
        params[2]=high
        self.nav.modify();self.nav.set_editor_property('nav_mesh_resolution_params',params)
        actor.set_actor_location(center+V(0,0,.1),False,False)
        actor.set_actor_location(center,False,False)
        if actor.get_package() not in self.packages:self.packages.append(actor.get_package())
        if self.nav.get_package() not in self.packages:self.packages.append(self.nav.get_package())

    def write_report(self,filename='stair51_report.json'):
        data={'map':self.world.get_path_name(),'before':self.before,'plan':self.plan_data,
              'proxies':self.plans,'baseline_components':self.baseline,'validation':self.validation,
              'retired':[],
              'preserved_sources':[{'actor':self.source.get_path_name(),
                  'transform':self.before.get(self.source.get_path_name(),{}).get('transform',simple.serialize_transform(self.source.get_actor_transform())),
                  'mesh':self.c.static_mesh.get_path_name(),'collision_enabled':self.c.get_collision_enabled().value}],
              'packages':[p.get_name() for p in self.packages],
              'dirty_maps_before':sorted(self.dirty_before),'dirty_content_before':sorted(self.dirty_contents_before)}
        path=os.path.join(self.report_dir,filename)
        with open(path,'w',encoding='utf-8') as f:json.dump(data,f,ensure_ascii=False,indent=2)
        return path

    def save(self):
        if simple.serialize_transform(self.source.get_actor_transform())!=self.before[self.source.get_path_name()]['transform']:
            raise RuntimeError('Do not save collision for changed source geometry')
        self.packages=[self.proxy.get_package(),self.nav.get_package()]
        if getattr(self,'navigation_actor',None):self.packages.append(self.navigation_actor.get_package())
        if not unreal.EditorLoadingAndSavingUtils.save_packages(self.packages,True):
            raise RuntimeError('Could not save stair collision and navigation')
        return self.write_report()
