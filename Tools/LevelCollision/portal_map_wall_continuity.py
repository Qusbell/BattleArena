"""Continuous, non-walkable wall bodies for PortalMap (editor instances only).

Unlike missing-face patches, these bodies replace the full collision of plain
wall panels. The full boundary panel is included even beyond NavBounds. Real
portal apertures are subtracted; the Corridor kit's roof lip retains native support.
plan(), apply(), and save() are deliberately separate operations.
"""

import collections
import hashlib
import importlib.util
import json
import math
import os

import unreal

_spec = importlib.util.spec_from_file_location('portal_simplify_continuity',
    os.path.join(os.path.dirname(__file__), 'portal_map_simplify.py'))
simple = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(simple)
V, dot, unit, xyz = simple.V, simple.dot, simple.unit, simple.xyz


def corners(lo, hi):
    return [V(x,y,z) for x in (lo.x,hi.x) for y in (lo.y,hi.y) for z in (lo.z,hi.z)]


def subtract(rect, hole):
    a,b,c,d = rect
    e,f,g,h = hole
    e,f,g,h = max(a,e),min(b,f),max(c,g),min(d,h)
    if f <= e or h <= g:
        return [rect]
    return [r for r in ((a,e,c,d),(f,b,c,d),(e,f,c,g),(e,f,h,d))
            if r[1]-r[0] > .1 and r[3]-r[2] > .1]


def rect_near(a, b):
    sw = min(a[1],b[1])-max(a[0],b[0])
    zh = min(a[3],b[3])-max(a[2],b[2])
    # Close construction seams, not a character-sized doorway.
    return (sw >= -6 and zh > 100) or (sw > 70 and zh >= -80)


class WallContinuity(simple.Simplifier):
    def __init__(self, report_dir):
        super().__init__(report_dir)
        self.flat_sources = []
        self.corridor_sources = []
        self.wall_bodies = []
        self.retained = []
        self.clusters = []
        self.seam_boxes = []
        self.coverage_samples = []

    def in_map_boundary(self, c):
        p,e,_ = unreal.SystemLibrary.get_component_bounds(c)
        return all(getattr(p+e,k) >= getattr(self.lo,k)-(100 if k=='z' else 400)
                   and getattr(p-e,k) <= getattr(self.hi,k)+(100 if k=='z' else 400)
                   for k in ('x','y','z'))

    @staticmethod
    def no_wall_base(c):
        c.set_walkable_slope_override(unreal.WalkableSlopeOverride(
            walkable_slope_behavior=unreal.WalkableSlopeBehavior.WALKABLE_SLOPE_UNWALKABLE,
            walkable_slope_angle=0.))
        c.set_editor_property('can_character_step_up_on', unreal.CanBeCharacterBase.ECB_NO)

    def panel(self, actor):
        c=actor.static_mesh_component
        lo,hi=c.get_local_bounds()
        t=c.get_world_transform()
        normal=unreal.MathLibrary.transform_direction(t,V(0,1,0));normal.z=0
        tangent=normal.cross(V(0,0,1))
        tangent=unit(tangent)
        if tangent.x < -.0001 or abs(tangent.x)<.0001 and tangent.y<0:
            tangent=tangent*-1
        normal=V(0,0,1).cross(tangent)
        points=[unreal.MathLibrary.transform_location(t,p) for p in corners(lo,hi)]
        ss,dd,zz=([dot(p,tangent) for p in points],[dot(p,normal) for p in points],[p.z for p in points])
        return {'source':actor,'u':tangent,'n':normal,
                'rect':(min(ss)-2,max(ss)+2,min(zz)-2,max(zz)+2),
                'depth':(min(dd),max(dd))}

    def plan(self):
        self.plans=[];self.wall_bodies=[];self.clusters=[];self.seam_boxes=[]
        self.corridor_sources=[];self.coverage_samples=[]
        self.quality={}
        panels=[]
        for actor in self.sources:
            c=actor.static_mesh_component
            if not c.static_mesh or not self.in_map_boundary(c):
                continue
            name=c.static_mesh.get_name()
            if name=='SM_BigWall_Office':
                normal=unit(unreal.MathLibrary.transform_direction(c.get_world_transform(),V(0,1,0)))
                if abs(normal.z)>.15:
                    self.quality.setdefault('non_vertical_wall_assets_retained',[]).append(actor.get_actor_label())
                    continue
                panels.append(self.panel(actor))
            elif name=='SM_Corridor':
                self.corridor_sources.append(actor)
        self.flat_sources=[p['source'] for p in panels]
        # Connected coplanar solids share ONE front/back depth envelope. Thus
        # stacked or side-by-side panels cannot leave recessed stepping ledges.
        for panel in panels:
            matches=[]
            for cluster in self.clusters:
                if dot(panel['u'],cluster[0]['u']) < math.cos(math.radians(.01)):
                    continue
                if any(min(panel['depth'][1],q['depth'][1])-max(panel['depth'][0],q['depth'][0])>=-2
                       and rect_near(panel['rect'],q['rect']) for q in cluster):
                    matches.append(cluster)
            if not matches:
                self.clusters.append([panel])
            else:
                matches[0].append(panel)
                for other in matches[1:]:
                    matches[0].extend(other);self.clusters.remove(other)
        for cluster in self.clusters:
            ref=cluster[0];u,n=ref['u'],ref['n']
            rects=[p['rect'] for p in cluster]
            # Explicitly bridge only a small vertical/horizontal construction
            # seam between overlapping panels. No bounding-box fill of an L.
            for i,a in enumerate(rects[:]):
                for b in rects[:i]:
                    sw0,sw1=max(a[0],b[0]),min(a[1],b[1])
                    z0,z1=min(a[3],b[3]),max(a[2],b[2])
                    if sw1-sw0>70 and 0<z1-z0<=80:
                        rects.append((sw0,sw1,z0,z1))
            d0=min(p['depth'][0] for p in cluster)-.5
            d1=max(p['depth'][1] for p in cluster)+.5
            merged=self._rectangles(rects,grid=.1)
            for plo,phi in self.protected:
                pp=corners(plo,phi)
                pd=[dot(p,n) for p in pp]
                if max(pd)<d0-20 or min(pd)>d1+20:
                    continue
                hole=(min(dot(p,u) for p in pp),max(dot(p,u) for p in pp),plo.z,phi.z)
                merged=[r for rect in merged for r in subtract(rect,hole)]
            for s0,s1,z0,z1 in merged:
                p=self.spec('Wall',ref['source'],u*((s0+s1)/2)+n*((d0+d1)/2)+V(0,0,(z0+z1)/2),
                    unreal.MathLibrary.make_rot_from_xz(u,V(0,0,1)),
                    V((s1-s0)/100,(d1-d0)/100,(z1-z0)/100),
                    {'continuous_wall':True,'sources':[q['source'].get_path_name() for q in cluster],
                     'tangent':xyz(u),'normal':xyz(n),'rect':[s0,s1,z0,z1],'depth':[d0,d1]})
                self.wall_bodies.append(p)
                self.sample_body(p,u,n,(s0,s1,z0,z1),(d0,d1))
        for actor in self.corridor_sources:
            self.plan_corridor(actor)
        self.quality['continuous_walls']={'panels':len(panels),'clusters':len(self.clusters),
            'corridors':len(self.corridor_sources),'boxes':len(self.wall_bodies),
            'coverage_samples':len(self.coverage_samples),'boundary_margin_xy_cm':400,
            'construction_seam_max_height_cm':80}
        return self.quality['continuous_walls']

    def sample_body(self,plan,u,n,rect,depth):
        s0,s1,z0,z1=rect;d0,d1=depth
        for s in range(math.ceil((s0+10)/35),math.floor((s1-10)/35)+1):
            for z in range(math.ceil((z0+10)/50),math.floor((z1-10)/50)+1):
                for d,sign in ((d0,-1),(d1,1)):
                    self.coverage_samples.append({'point':xyz(u*(s*35)+n*d+V(0,0,z*50)),
                        'normal':xyz(n*sign),'source':plan['source']})

    def plan_corridor(self,actor):
        c=actor.static_mesh_component;lo,hi=c.get_local_bounds();t=c.get_world_transform()
        # This kit is a solid-backed wall, not a hollow tunnel. Its normal is
        # local X, and its local Y/Z projection is fully closed. Flatten the
        # high wall trim while leaving its long upper roof overhang in place.
        vertices,_=self.geometry(c.static_mesh)
        front=max(v.x for v in vertices if lo.z+.05*(hi.z-lo.z)<v.z<lo.z+.85*(hi.z-lo.z))+3
        local=V((lo.x+front)/2,(lo.y+hi.y)/2,(lo.z+hi.z)/2)
        size=V(front-lo.x+1,hi.y-lo.y+4,hi.z-lo.z+4)
        p=self.spec('Wall',actor,unreal.MathLibrary.transform_location(t,local),t.rotation.rotator(),
            V(abs(t.scale3d.x)*size.x/100,abs(t.scale3d.y)*size.y/100,abs(t.scale3d.z)*size.z/100),
            {'corridor_wall':True,'front_local_x':front,'roof_overhang_preserved':True})
        self.wall_bodies.append(p)
        n=unit(unreal.MathLibrary.transform_direction(t,V(1,0,0)))
        for iy in range(12):
            for iz in range(14):
                for x,sign in ((lo.x-.5,-1),(front+.5,1)):
                    point=unreal.MathLibrary.transform_location(t,V(x,
                        lo.y+(hi.y-lo.y)*(iy+.5)/12,lo.z+(hi.z-lo.z)*(iz+.5)/14))
                    self.coverage_samples.append({'point':xyz(point),'normal':xyz(n*sign),'source':actor.get_path_name()})
        self.quality.setdefault('corridor_profiles',[]).append({'source':actor.get_path_name(),'front_local_x':front})

    def apply(self):
        self.retained=[]
        groups=collections.defaultdict(list)
        for p in self.wall_bodies:groups[p['source']].append(p)
        flat_paths={a.get_path_name() for a in self.flat_sources}
        targeted=flat_paths|{a.get_path_name() for a in self.corridor_sources}
        active=set()
        with unreal.ScopedEditorTransaction('Close PortalMap boundary gaps and remove wall stepping ledges'):
            for path,plans in groups.items():
                proxy=self.upsert_group('Wall',self.by_path[path],plans)
                self.no_wall_base(proxy.get_component_by_class(unreal.InstancedStaticMeshComponent))
                active.add(proxy.get_path_name())
            for actor in self.flat_sources:
                c=actor.static_mesh_component
                self._remember(actor,c,'replace detailed wall ledges with continuous non-walkable wall body')
                c.set_collision_profile_name('NoCollision',True)
                c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
            self.retired=[]
            for proxy in self.old_walls:
                source=self.source_for(proxy)
                if source and source.get_path_name() in targeted and proxy.get_path_name() not in active:
                    self.retire(proxy)
                elif proxy.get_path_name() not in active:
                    c=proxy.get_component_by_class(unreal.InstancedStaticMeshComponent)
                    proxy.modify();c.modify();self.no_wall_base(c)
                    if proxy.get_package() not in self.packages:self.packages.append(proxy.get_package())
                    self.retained.append(proxy.get_path_name())
        self.apply_native_wall_base_rules()
        return {'continuous_proxies':len(active),'retired':len(self.retired),'retained_wall_proxies':len(self.retained)}

    def apply_native_wall_base_rules(self):
        affected=[]
        for proxy in self.old_walls:
            source=self.source_for(proxy)
            if not source or source.get_path_name() in affected:continue
            c=source.static_mesh_component
            if not c.static_mesh or c.static_mesh.get_name() not in (
                    'SM_BigColumn01','Boolean_DB6EECED','Boolean_5D47BE0C'):continue
            # These are wall/column-only shapes, with no traversable floor.
            # Preserve their exact collision and only forbid using them as a base.
            slope=c.get_walkable_slope_override()
            before={'behavior':slope.walkable_slope_behavior.name,'angle':slope.walkable_slope_angle,
                    'step':c.get_editor_property('can_character_step_up_on').name}
            self._remember(source,c,'wall-only native shape may not be a character base')
            record=self.baseline[source.get_path_name()+':'+c.get_name()]
            record.setdefault('walkability_before',before)
            record.update(expected_enabled=c.get_collision_enabled().value,
                expected_profile=str(c.get_collision_profile_name()),expected_no_character_base=True)
            self.no_wall_base(c)
            affected.append(source.get_path_name())
        self.validation['native_wall_no_character_base']={'components':affected}
        return affected

    def final_instance_report(self):
        plans=[]
        for actor in self.eas.get_all_level_actors():
            tags=[str(t) for t in actor.tags]
            if simple.base.TAG not in tags:continue
            c=actor.get_component_by_class(unreal.InstancedStaticMeshComponent)
            if not c or c.get_collision_enabled()==unreal.CollisionEnabled.NO_COLLISION:continue
            kind='Stair' if str(actor.get_folder_path()).endswith('Stair') else 'Wall'
            source=self.source_for(actor)
            for i in range(c.get_instance_count()):
                p=simple.serialize_transform(c.get_instance_transform(i,True))
                p.update(id=hashlib.sha1((actor.get_path_name()+':'+str(i)).encode()).hexdigest()[:16],
                    kind=kind,source=source.get_path_name() if source else '',label=actor.get_actor_label(),
                    proxy_actor=actor.get_path_name(),instance_index=i,evidence={})
                plans.append(p)
        self.plans=plans
        return dict(collections.Counter(p['kind'] for p in plans))

    def verify_no_wall_bases(self):
        failures=[];count=0
        for actor in self.eas.get_all_level_actors():
            if not str(actor.get_folder_path()).endswith('Wall'):continue
            if simple.base.TAG not in [str(t) for t in actor.tags]:continue
            c=actor.get_component_by_class(unreal.InstancedStaticMeshComponent)
            if not c or not c.get_instance_count():continue
            count+=1
            if c.get_walkable_slope_override().walkable_slope_behavior != unreal.WalkableSlopeBehavior.WALKABLE_SLOPE_UNWALKABLE \
                    or c.get_editor_property('can_character_step_up_on') != unreal.CanBeCharacterBase.ECB_NO:
                failures.append(actor.get_path_name())
        self.validation['wall_no_character_base']={'components':count,'failures':failures}
        return failures
