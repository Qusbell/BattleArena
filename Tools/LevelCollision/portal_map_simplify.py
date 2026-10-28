"""Movement-first, level-local simplification of PortalMap collision.

Plans are computed in memory. apply() is explicit; save() saves only touched
packages. Source render assets are not edited. All coordinates in reports are
world coordinates, so moved/scaled actors are measured again on each run.
"""

import collections
import importlib.util
import json
import math
import os

import unreal


_path = os.path.join(os.path.dirname(__file__), "portal_map_collision.py")
_spec = importlib.util.spec_from_file_location("portal_collision_base", _path)
base = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(base)

V = unreal.Vector


def dot(a, b):
    return a.x * b.x + a.y * b.y + a.z * b.z


def unit(v):
    return v / v.length()


def segment_distance(a, b, c, d):
    """Squared distance between closed line segments (including endpoints)."""
    u, v, w = b-a, d-c, a-c
    aa, bb, cc = dot(u,u), dot(u,v), dot(v,v)
    dd, ee = dot(u,w), dot(v,w)
    candidates = []
    def clamp(x):
        return max(0., min(1., x))
    for s in (0., 1.):
        t = clamp((ee+bb*s)/cc) if cc else 0.
        candidates.append((w+u*s-v*t).length_squared())
    for t in (0., 1.):
        s = clamp((bb*t-dd)/aa) if aa else 0.
        candidates.append((w+u*s-v*t).length_squared())
    den = aa*cc-bb*bb
    if den > 1e-12:
        s, t = (bb*ee-cc*dd)/den, (aa*ee-bb*dd)/den
        if 0 <= s <= 1 and 0 <= t <= 1:
            candidates.append((w+u*s-v*t).length_squared())
    return min(candidates)


def segment_triangle_distance(a, b, p, q, r):
    """Distance to actual render triangles; avoids triangle initial-overlap gaps."""
    normal = (q-p).cross(r-p)
    nn = dot(normal, normal)
    if nn < 1e-12:
        return math.sqrt(min(segment_distance(a,b,p,q),segment_distance(a,b,q,r),segment_distance(a,b,r,p)))
    def inside(x):
        return all(dot((y-x0).cross(x-x0),normal) >= -1e-6
                   for x0,y in ((p,q),(q,r),(r,p)))
    da, db = dot(a-p,normal), dot(b-p,normal)
    if da*db <= 0 and abs(da-db) > 1e-12:
        x = a+(b-a)*(da/(da-db))
        if inside(x):
            return 0.
    distances = [segment_distance(a,b,p,q),segment_distance(a,b,q,r),segment_distance(a,b,r,p)]
    for x, signed in ((a,da),(b,db)):
        projection = x-normal*(signed/nn)
        if inside(projection):
            distances.append(signed*signed/nn)
    return math.sqrt(min(distances))


def xyz(v):
    return [v.x, v.y, v.z]


def serialize_transform(t):
    r = t.rotation.rotator()
    return {"location": xyz(t.translation), "rotation": [r.pitch, r.yaw, r.roll],
            "scale": xyz(t.scale3d)}


def transform(spec):
    return unreal.Transform(location=V(*spec["location"]), rotation=base.rotator(spec["rotation"]),
                            scale=V(*spec["scale"]))


def face(spec):
    t = transform(spec)
    center = t.translation
    x = unit(unreal.MathLibrary.transform_direction(t, V(1, 0, 0)))
    y = unit(unreal.MathLibrary.transform_direction(t, V(0, 1, 0)))
    n = unit(unreal.MathLibrary.transform_direction(t, V(0, 0, 1)))
    hx, hy, hz = [abs(v) * 50 for v in spec["scale"]]
    corners = [center + x * sx * hx + y * sy * hy
               for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
    return center, n, corners, hz


class Simplifier(base.PortalCollisionRepair):
    def __init__(self, report_dir):
        super().__init__()
        self.report_dir = report_dir
        os.makedirs(report_dir, exist_ok=True)
        self.old_proxies = [a for a in self.actors if base.TAG in [str(t) for t in a.tags]]
        self.old_walls = [a for a in self.old_proxies if str(a.get_folder_path()).endswith("Wall")]
        self.old_stairs = [a for a in self.old_proxies if str(a.get_folder_path()).endswith("Stair")]
        self.by_name = {a.get_name(): a for a in self.actors}
        self.by_path = {a.get_path_name(): a for a in self.actors}
        self.dirty_before = {p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()}
        self.dirty_contents_before = {p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()}
        self.raw_wall_specs = []
        self.wall_plans = []
        self.stair_groups = []
        self.decor_plans = []
        self.keep_wall_actors = set()
        self.retired = []
        self.quality = {}
        self._geometry = {}
        self._top_profiles = {}
        self.source_pose = {}
        self.strict_sources = set()
        self.inset_wall_planes = {}
        self.stair_lift_caps = {}
        self._wall_inputs_cached = None
        self.protected = []
        for actor in self.actors:
            if "Portal" not in actor.get_class().get_name():
                continue
            for c in actor.get_components_by_class(unreal.PrimitiveComponent):
                if c.get_name().lower() in ("entrycollision", "portalscreen"):
                    p, e, _ = unreal.SystemLibrary.get_component_bounds(c)
                    self.protected.append((p - e - V(12, 12, 12), p + e + V(12, 12, 12)))

    def protected_point(self, p, margin=0):
        return any(all(getattr(lo, k) - margin <= getattr(p, k) <= getattr(hi, k) + margin
                       for k in ("x", "y", "z")) for lo, hi in self.protected)

    def source_for(self, proxy):
        name = next((str(t).split("=", 1)[1] for t in proxy.tags if str(t).startswith("PCFSource=")), None)
        return self.by_name.get(name)

    def audit_fresh_walls(self):
        """Measure current source geometry without legacy proxy occlusion.

        Run before plan_walls/capture_clearance/apply. This only changes a
        Python auditor's trace function, not component collision in the level.
        """
        auditor = base.PortalCollisionRepair()
        original_line = auditor.line
        def native_line(start, end, complex=False, ignore=(), profile='Pawn'):
            return original_line(start,end,complex,list(ignore)+self.old_walls,profile)
        auditor.line = native_line
        auditor.audit_walls()
        return auditor.wall_plans

    def geometry_backing(self, spec):
        p, n, _, _ = face(spec)
        h = self.line(p + n * 16, p - n * 16, complex=True, ignore=self.old_walls)
        if not h:
            return None
        t = h.to_tuple()
        return t[9] if abs(dot(unit(t[7]), n)) > .8 else None

    def wall_inputs(self, fresh_specs):
        by_source = collections.defaultdict(list)
        for p in fresh_specs:
            by_source[p["source"]].append(p)
        orphan_stats = []
        for actor in self.old_walls:
            component = actor.get_component_by_class(unreal.InstancedStaticMeshComponent)
            source = self.source_for(actor)
            if source and source.get_path_name() in by_source:
                continue
            backed = 0
            redundant = 0
            for index in range(component.get_instance_count()):
                p = serialize_transform(component.get_instance_transform(index, True))
                actual = self.geometry_backing(p)
                if actual and isinstance(actual, unreal.StaticMeshActor):
                    center,n,corners,_ = face(p)
                    samples=[center]+corners+[(corners[i]+corners[(i+1)%4])/2 for i in range(4)]
                    if all(self.line(q+n*6,q-n*6,ignore=self.old_walls) for q in samples):
                        redundant += 1
                        continue
                    p.update(kind="Wall", source=actual.get_path_name(), label=actual.get_actor_label(),
                             evidence={"old_proxy": actor.get_path_name(), "old_instance": index})
                    by_source[p["source"]].append(p)
                    backed += 1
            orphan_stats.append({"proxy": actor.get_actor_label(), "source_missing": source is None,
                                 "old_shapes": component.get_instance_count(), "backed_shapes": backed})
            orphan_stats[-1]["already_blocked_by_native"] = redundant
        self.raw_wall_specs = [p for values in by_source.values() for p in values]
        self.quality["legacy_reconciliation"] = orphan_stats
        return by_source

    @staticmethod
    def _rectangles(rects, grid=5, fill=None):
        # Coordinate-compressed union. Snap outward by <=5 cm to remove seams;
        # empty cells remain empty, so an L/U-shaped wall cannot become a slab
        # across a doorway. Never merge merely by the union's bounding box.
        snapped = [(math.floor(a / grid) * grid, math.ceil(b / grid) * grid,
                    math.floor(c / grid) * grid, math.ceil(d / grid) * grid)
                   for a, b, c, d in rects]
        xs = sorted({v for r in snapped for v in r[:2]})
        ys = sorted({v for r in snapped for v in r[2:]})
        cells = set()
        required = set()
        for ix in range(len(xs) - 1):
            x = (xs[ix] + xs[ix + 1]) / 2
            for iy in range(len(ys) - 1):
                y = (ys[iy] + ys[iy + 1]) / 2
                needed = any(a <= x <= b and c <= y <= d for a, b, c, d in snapped)
                if needed:
                    required.add((ix,iy))
                if needed or fill and fill(xs[ix], xs[ix + 1], ys[iy], ys[iy + 1]):
                    cells.add((ix, iy))
        # Cover with maximal rectangles, allowing overlaps. Removing occupied
        # cells after each rectangle creates many unnecessary thin fragments.
        candidates = []
        for y0 in range(len(ys)-1):
            columns = set(range(len(xs)-1))
            for y1 in range(y0+1, len(ys)):
                columns.intersection_update(x for x in range(len(xs)-1) if (x,y1-1) in cells)
                if not columns:
                    break
                runs = []
                for x in sorted(columns):
                    if not runs or x != runs[-1][-1] + 1:
                        runs.append([x])
                    else:
                        runs[-1].append(x)
                for run in runs:
                    x0,x1 = run[0],run[-1]+1
                    covered = frozenset((x,y) for x in run for y in range(y0,y1))
                    candidates.append(((xs[x0],xs[x1],ys[y0],ys[y1]),covered))
        candidates.sort(key=lambda q:-((q[0][1]-q[0][0])*(q[0][3]-q[0][2])))
        maximal=[]
        for rect,covered in candidates:
            if not any(covered.issubset(c) for _,c in maximal):
                maximal.append((rect,covered))
        # Geometry-confirmed bridge cells permit a large rectangle, but are
        # already solid and do not themselves require a new collision shape.
        remaining=set(required)
        result=[]
        weights={(x,y):(xs[x+1]-xs[x])*(ys[y+1]-ys[y]) for x,y in cells}
        while remaining:
            rect,covered=max(maximal,key=lambda q:sum(weights[c] for c in q[1] if c in remaining))
            result.append(rect)
            remaining.difference_update(covered)
        return result

    def plan_walls(self, fresh_specs):
        inputs = self._wall_inputs_cached
        if inputs is None:
            inputs = self.wall_inputs(fresh_specs)
            self._wall_inputs_cached = inputs
        self.plans = [p for p in self.plans if p["kind"] != "Wall"]
        self.wall_plans = []
        groups_total = 0
        for path, specs in inputs.items():
            source = self.by_path.get(path)
            if source is None:
                continue
            self.source_pose[path] = serialize_transform(source.get_actor_transform())
            groups = []
            angle = .5 if path in self.strict_sources else 12
            distance = .25 if path in self.strict_sources else 12
            # Seed the planes from broad faces, not a tiny bevel whose tilted
            # normal would prevent the main wall and its trim from joining.
            for spec in sorted(specs, key=lambda p: -abs(p["scale"][0]*p["scale"][1])):
                p, n, corners, half_depth = face(spec)
                match = None
                for group in groups:
                    if abs(dot(group["n"], n)) >= math.cos(math.radians(angle)) \
                            and all(abs(dot(c - group["p"], group["n"])) <= distance for c in corners):
                        match = group
                        break
                if match is None:
                    match = {"p": p, "n": n, "faces": []}
                    groups.append(match)
                match["faces"].append((p, n, corners, half_depth))
            groups_total += len(groups)
            for group in groups:
                origin, n = group["p"], group["n"]
                u = unit(V(0, 0, 1).cross(n))
                v = unit(n.cross(u))
                rects, depths = [], []
                for p, _, corners, half_depth in group["faces"]:
                    us = [dot(c - origin, u) for c in corners]
                    vs = [dot(c - origin, v) for c in corners]
                    rects.append((min(us), max(us), min(vs), max(vs)))
                    depths.extend(dot(c - origin, n) + sign * half_depth
                                  for c in corners for sign in (-1, 1))
                d0, d1 = min(depths) - .1, max(depths) + .1
                def supported_gap(u0, u1, v0, v1):
                    # The old patches omit areas already covered by native
                    # collision. Bridge those areas only where the CURRENT
                    # render mesh is solid, sampled at <=25 cm spacing.
                    nu, nv = max(1, math.ceil((u1-u0)/25)), max(1, math.ceil((v1-v0)/25))
                    for i in range(nu + 1):
                        for j in range(nv + 1):
                            p = origin + u * (u0 + (u1-u0)*i/nu) + v * (v0 + (v1-v0)*j/nv)
                            if self.protected_point(p) or not self.inside(p):
                                return False
                            h = source.static_mesh_component.line_trace_component(
                                p+n*8, p-n*8, True, False, False)
                            if not h or abs(dot(unit(h[1]), n)) < .94:
                                return False
                    return True

                grid = .01 if path in self.strict_sources else 5
                for u0, u1, v0, v1 in self._rectangles(rects, grid=grid, fill=supported_gap):
                    corners = [origin + u * x + v * y + n * ((d0 + d1) / 2)
                               for x, y in ((u0, v0), (u1, v0), (u1, v1), (u0, v1))]
                    # Keep the portal clearance even when adjacent old cells
                    # inflate slightly during snapping. Fallback to its original
                    # small patches rather than fill the protected opening.
                    if any(self.protected_point(c) for c in corners) or self.protected_point(sum(corners, V()) / 4):
                        for spec in specs:
                            p, sn, sc, sh = face(spec)
                            if abs(dot(sn, n)) > .998 and abs(dot(p - origin, n)) <= 5 \
                                    and u0 <= dot(p - origin, u) <= u1 and v0 <= dot(p - origin, v) <= v1:
                                self.wall_plans.append(self.spec("Wall", source, p,
                                    base.rotator(spec["rotation"]), V(*spec["scale"]),
                                    {"portal_edge_preserved": True}))
                        continue
                    center = sum(corners, V()) / 4
                    correction = next((q for q in self.inset_wall_planes.get(path, [])
                                       if dot(n, V(*q['normal'])) > .999), {})
                    inset = correction.get('distance', 0)
                    thickness = correction.get('thickness_cm', d1-d0)
                    center += n * inset
                    self.wall_plans.append(self.spec("Wall", source, center,
                        unreal.MathLibrary.make_rot_from_xz(u, n),
                        V((u1 - u0) / 100, (v1 - v0) / 100, thickness / 100),
                        {"normal": xyz(n), "corners": [xyz(c+n*inset) for c in corners],
                         "inset_cm": inset,
                         "input_patches": len(group["faces"])}))
        self.quality["wall_plan"] = {"input_patches": len(self.raw_wall_specs),
                                    "plane_groups": groups_total, "output_boxes": len(self.wall_plans),
                                    "sources": len(inputs)}
        return self.quality["wall_plan"]

    def capture_clearance(self):
        anchors, seen = [], set()
        for spec in self.raw_wall_specs:
            p,n,_,_=face(spec)
            for radius,height in ((34,88),(51,132)):
                for sign in (-1,1):
                    for extra in (4,18):
                        probe=p+n*sign*(radius+extra)-V(0,0,height)
                        nav=self.nav_at(probe,(25,25,160))
                        if nav is None or not self.inside(nav):
                            continue
                        ground=self.floor(nav,self.old_walls,above=20,below=85)
                        if ground is None or ground.to_tuple()[7].z < .7:
                            continue
                        point=ground.to_tuple()[5]
                        normal=ground.to_tuple()[7]
                        center=point+V(0,0,height+3+radius*(1/normal.z-1))
                        key=(radius,*(round(v/10) for v in xyz(center)))
                        if key in seen:
                            continue
                        seen.add(key)
                        blocked=False
                        for complex in (False,True):
                            ignored=self.old_walls if complex else []
                            hit=unreal.SystemLibrary.capsule_trace_single_by_profile(
                                self.world,center,center+V(.1,0,0),radius,height,
                                "Pawn",complex,ignored,unreal.DrawDebugTrace.NONE)
                            if hit:
                                blocked=True
                                break
                        if not blocked:
                            anchors.append({"center":xyz(center),"radius":radius,"height":height})
        self.clearance_anchors=anchors
        path=os.path.join(self.report_dir,"clearance_anchors.json")
        with open(path,"w",encoding="utf-8") as f:json.dump(anchors,f,indent=2)
        return len(anchors)

    def check_clearance(self):
        failures=[]
        repaired_visible_walls=[]
        triangles={}
        adjusted=0
        for anchor in self.clearance_anchors:
            center=V(*anchor["center"])
            hit=unreal.SystemLibrary.capsule_trace_single_by_profile(
                self.world,center,center+V(.1,0,0),anchor["radius"],anchor["height"],
                "Pawn",False,[],unreal.DrawDebugTrace.NONE)
            if hit:
                info=base.hit_info(hit)
                owner=hit.to_tuple()[9]
                if str(owner.get_folder_path()).endswith("Stair"):
                    foot=center-V(0,0,anchor["height"]+3)
                    ground=self.floor(foot,above=45,below=65)
                    if ground and ground.to_tuple()[7].z > .7 and abs(ground.to_tuple()[5].z-foot.z) <= 45:
                        nz=ground.to_tuple()[7].z
                        rebased=ground.to_tuple()[5]+V(0,0,anchor["height"]+3+anchor["radius"]*(1/nz-1))
                        h=unreal.SystemLibrary.capsule_trace_single_by_profile(
                            self.world,rebased,rebased+V(.1,0,0),anchor["radius"],anchor["height"],
                            "Pawn",False,[],unreal.DrawDebugTrace.NONE)
                        if not h:
                            adjusted+=1
                            continue
                result={**anchor,"hit":info,"tags":[str(t) for t in owner.tags]}
                source=self.source_for(owner)
                if source and str(owner.get_folder_path()).endswith('Wall'):
                    key=source.get_path_name()
                    if key not in triangles:
                        c=source.static_mesh_component
                        vertices,indices=self.geometry(c.static_mesh)
                        vertices=[unreal.MathLibrary.transform_location(c.get_world_transform(),v) for v in vertices]
                        triangles[key]=[tuple(vertices[indices[i+k]] for k in range(3))
                                        for i in range(0,len(indices),3)]
                    offset=V(0,0,anchor['height']-anchor['radius'])
                    distance=min(segment_triangle_distance(center-offset,center+offset,*tri)
                                 for tri in triangles[key])
                    result['render_distance_cm']=distance
                    # A missing collision on an existing visible wall was a
                    # valid baseline capsule overlap in Chaos. Closing that
                    # hole is the requested repair, not a lost open passage.
                    if distance <= anchor['radius']+.01:
                        repaired_visible_walls.append(result)
                        continue
                failures.append(result)
        self.validation["standing_clearance"]={"tested":len(self.clearance_anchors),
            "capsules_cm":[[34,88],[51,132]],"rebased_onto_new_stairs":adjusted,
            "repaired_visible_walls":repaired_visible_walls,"failures":failures}
        return failures

    def geometry(self, mesh):
        key = mesh.get_path_name()
        if key not in self._geometry:
            verts, tris = [], []
            for section in range(mesh.get_num_sections(0)):
                v, t, *_ = unreal.ProceduralMeshLibrary.get_section_from_static_mesh(mesh, 0, section)
                offset = len(verts)
                verts.extend(v)
                tris.extend(i + offset for i in t)
            self._geometry[key] = (verts, tris)
        return self._geometry[key]

    def top_profile(self, component):
        mesh = component.static_mesh
        if mesh.get_path_name() not in self._top_profiles:
            lo, hi = component.get_local_bounds()
            verts, tris = self.geometry(mesh)
            values = []
            for j in range(61):
                x, y = lo.x + (hi.x - lo.x) * (j + .5) / 61, (lo.y + hi.y) / 2
                heights = []
                for i in range(0, len(tris), 3):
                    a, b, c = [verts[tris[i + k]] for k in range(3)]
                    den = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y)
                    if abs(den) < .00001:
                        continue
                    u = ((b.y - c.y) * (x - c.x) + (c.x - b.x) * (y - c.y)) / den
                    v = ((c.y - a.y) * (x - c.x) + (a.x - c.x) * (y - c.y)) / den
                    if min(u, v, 1 - u - v) >= -.00001:
                        heights.append(u * a.z + v * b.z + (1 - u - v) * c.z)
                if heights:
                    values.append(V(x, y, max(heights)))
            self._top_profiles[mesh.get_path_name()] = values
        return [unreal.MathLibrary.transform_location(component.get_world_transform(), p)
                for p in self._top_profiles[mesh.get_path_name()]]

    def plan_stairs(self):
        self.plans = [p for p in self.plans if p['kind'] != 'Stair']
        sources = []
        for actor in self.sources:
            c = actor.static_mesh_component
            if not c.static_mesh or c.static_mesh.get_name() != "SM_RampSteps":
                continue
            center, _, _ = unreal.SystemLibrary.get_component_bounds(c)
            if not self.inside(center):
                continue
            t = c.get_world_transform()
            d = unit(unreal.MathLibrary.transform_direction(t, V(1, 0, 0)))
            lateral = V(-d.y, d.x, 0)
            lo, hi = c.get_local_bounds()
            corners = [unreal.MathLibrary.transform_location(t, V(x, y, 0))
                       for x in (lo.x, hi.x) for y in (lo.y, hi.y)]
            sources.append({"actor": actor, "c": c, "d": d, "l": lateral,
                "s0": min(dot(p, d) for p in corners), "s1": max(dot(p, d) for p in corners),
                "w0": min(dot(p, lateral) for p in corners), "w1": max(dot(p, lateral) for p in corners),
                "points": self.top_profile(c)})
        groups = []
        for source in sources:
            matched = None
            for group in groups:
                ref = group[0]
                if dot(source["d"], ref["d"]) > .99999 \
                        and abs(source["w0"] - ref["w0"]) < 3 and abs(source["w1"] - ref["w1"]) < 3 \
                        and source["s0"] <= max(q["s1"] for q in group) + 3 \
                        and source["s1"] >= min(q["s0"] for q in group) - 3:
                    matched = group
                    break
            if matched is None:
                groups.append([source])
            else:
                matched.append(source)
        self.stair_groups = []
        for group in groups:
            d, lateral = group[0]["d"], group[0]["l"]
            s0, s1 = min(q["s0"] for q in group), max(q["s1"] for q in group)
            w0, w1 = min(q["w0"] for q in group), max(q["w1"] for q in group)
            points = sorted((dot(p, d), p.z) for q in group for p in q["points"])
            high = max(z for s, z in points[:5])
            low = min(z for s, z in points[-5:])
            for upper in (True, False):
                p = d * (s0 - 15 if upper else s1 + 15) + lateral * ((w0 + w1) / 2)
                p.z = high if upper else low
                h = self.floor(p, [q["actor"] for q in sources] + self.old_stairs, above=45, below=65)
                if h and h.to_tuple()[7].z > .7 and abs(h.to_tuple()[5].z - p.z) <= 45:
                    if upper:
                        high = h.to_tuple()[5].z
                    else:
                        low = h.to_tuple()[5].z
            # One long slope, plus only the landing joins required by its
            # envelope. Raise the central line above visible noses, rather than
            # reproduce every tread or create deep invisible solid wedges.
            slope = (low - high) / (s1 - s0)
            lift = max(0, max(z - (high + slope * (s - s0)) for s, z in points) + 1)
            lift = min(lift, 42)
            lift = min(lift, self.stair_lift_caps.get(group[0]['actor'].get_path_name(), lift))
            width_center = lateral * ((w0 + w1) / 2)
            inner_top = d * s0 + width_center + V(0, 0, high + lift)
            inner_bottom = d * s1 + width_center + V(0, 0, low + lift)
            path = [inner_top, inner_bottom]
            if lift > 3:
                extension = max(45, lift * 1.5)
                path = [d * (s0 - extension) + width_center + V(0, 0, high),
                        inner_top, inner_bottom,
                        d * (s1 + extension) + width_center + V(0, 0, low)]
            plans = []
            for a, b in zip(path, path[1:]):
                tangent = unit(b - a)
                normal = unit((lateral * -1).cross(tangent))
                angle = math.degrees(math.acos(max(-1, min(1, normal.z))))
                if angle > 44:
                    raise RuntimeError("Stair exceeds unchanged walkable slope: " + group[0]["actor"].get_actor_label())
                plan = self.spec("Stair", group[0]["actor"], (a + b) / 2 - normal * 6,
                    unreal.MathLibrary.make_rot_from_yz(tangent, normal),
                    V((w1 - w0) / 100, (b - a).length() / 100, .12),
                    {"upper": xyz(a), "lower": xyz(b), "angle": angle,
                     "sources": [q["actor"].get_path_name() for q in group]})
                plans.append(plan)
            self.stair_groups.append({"plans": plans, "sources": [q["actor"] for q in group],
                                      "path": path, "upper": path[0], "lower": path[-1],
                                      "width": w1 - w0, "lateral": lateral})
        self.quality["stairs"] = {"sources": len(sources), "flights": len(groups),
                                  "boxes": sum(len(g["plans"]) for g in self.stair_groups)}
        return self.quality["stairs"]

    def retire(self, actor):
        actor.modify()
        c = actor.get_component_by_class(unreal.InstancedStaticMeshComponent)
        if c:
            c.modify()
            c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
            c.clear_instances()
        actor.set_folder_path("CollisionFix/Retired")
        actor.set_editor_property("tags", list(dict.fromkeys(list(actor.tags) + [unreal.Name("PCFRetired")])))
        if actor.get_package() not in self.packages:
            self.packages.append(actor.get_package())
        self.retired.append(actor.get_path_name())

    def apply(self):
        self.retired=[]
        with unreal.ScopedEditorTransaction("Simplify Portal Map collision envelopes"):
            grouped = collections.defaultdict(list)
            for p in self.wall_plans:
                grouped[p["source"]].append(p)
            active = set()
            for path, specs in grouped.items():
                actor = self.upsert_group("Wall", self.by_path[path], specs)
                active.add(actor.get_path_name())
            for group in self.stair_groups:
                actor = self.upsert_group("Stair", group["sources"][0], group["plans"])
                active.add(actor.get_path_name())
                for source in group["sources"]:
                    c = source.static_mesh_component
                    self._remember(source, c, "continuous movement-first stair support")
                    c.set_collision_profile_name("NoCollision", True)
                    c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
            for actor in self.old_proxies:
                if actor.get_path_name() not in active:
                    self.retire(actor)
        return {"active_proxies": len(active), "retired_proxies": len(self.retired)}

    def write_report(self, filename="simplification_report.json"):
        data = {"map": self.world.get_path_name(), "bounds": [xyz(self.lo), xyz(self.hi)],
                "quality": self.quality, "proxies": self.plans, "baseline_components": self.baseline,
                "retired": self.retired, "validation": self.validation,
                "dirty_before": sorted(self.dirty_before),
                "dirty_contents_before": sorted(self.dirty_contents_before),
                "tuning": {"strict_sources": sorted(self.strict_sources),
                           "inset_wall_planes": self.inset_wall_planes,
                           "stair_lift_caps": self.stair_lift_caps},
                "packages": [p.get_name() for p in self.packages]}
        path = os.path.join(self.report_dir, filename)
        with open(path, "w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False, indent=2)
        return path
