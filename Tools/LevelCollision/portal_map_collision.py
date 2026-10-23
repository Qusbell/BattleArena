"""Editor-only, instance-scoped collision repair for Lvl_PortalMap.

Load with importlib inside the Unreal Editor. The audit_* methods are read-only;
apply_surfaces(), apply_walls() and save() are explicit. Original mesh assets,
gameplay triggers and global presets are never edited. Reports in
Saved/CollisionAudit retain the original settings.
"""

import collections
import hashlib
import json
import math
import os

import unreal


MAP = "/Game/QuakeLike_Base/Lvl_PortalMap.Lvl_PortalMap"
TAG = "PortalCollisionFix"
RADIUS, HALF_HEIGHT, STEP = 34.0, 88.0, 45.0
GRID = 40.0
DECOR_PROBES = [
    {"label": "Loigh02_Clean343", "nav_a": [259.19, -2438.75, 691.61],
     "nav_b": [259.19, -2638.75, 685.57]},
    {"label": "Loigh02_Clean342", "nav_a": [95.06, -2090.29, 663.0],
     "nav_b": [95.06, -2290.29, 663.0]},
]


def vec(v):
    return [round(v.x, 4), round(v.y, 4), round(v.z, 4)]


def dot(a, b):
    return a.x * b.x + a.y * b.y + a.z * b.z


def unit(v):
    return v / v.length()


def rotator(values):
    # Unreal's reflected positional constructor is roll, pitch, yaw.
    return unreal.Rotator(pitch=values[0], yaw=values[1], roll=values[2])


def hit_info(hit):
    if hit is None:
        return None
    h = hit.to_tuple()
    return {"actor": h[9].get_path_name() if h[9] else None,
            "component": h[10].get_name() if h[10] else None,
            "point": vec(h[5]), "normal": vec(h[7]),
            "penetrating": h[1]}


def state(actor, component):
    return {"actor": actor.get_path_name(), "label": actor.get_actor_label(),
            "component": component.get_name(),
            "profile": str(component.get_collision_profile_name()),
            "enabled": component.get_collision_enabled().value,
            "overlap": component.get_editor_property("generate_overlap_events"),
            "mesh": component.static_mesh.get_path_name(),
            "package": actor.get_package().get_name()}


class PortalCollisionRepair:
    def __init__(self):
        self.eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        self.world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        if self.world.get_path_name() != MAP:
            raise RuntimeError("Open Lvl_PortalMap before running this repair")
        self.actors = list(self.eas.get_all_level_actors())
        self.nav = next(a for a in self.actors if isinstance(a, unreal.RecastNavMesh))
        self.bounds_actor = next(a for a in self.actors if isinstance(a, unreal.NavMeshBoundsVolume))
        center, extent = self.bounds_actor.get_actor_bounds(False)
        self.lo, self.hi = center - extent, center + extent
        self.sources = [a for a in self.actors if isinstance(a, unreal.StaticMeshActor)
                        and TAG not in [str(t) for t in a.tags]]
        self.sms = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
        self.plans, self.packages = [], []
        self.report_dir = os.path.abspath(os.path.join(unreal.Paths.project_saved_dir(),
                                                     "CollisionAudit", "Lvl_PortalMap"))
        self.ignored_decor = []
        self.baseline = {}
        self.validation = {}

    def inside(self, point):
        return all(getattr(self.lo, k) <= getattr(point, k) <= getattr(self.hi, k)
                   for k in ("x", "y", "z"))

    def overlaps(self, component):
        p, e, _ = unreal.SystemLibrary.get_component_bounds(component)
        return all(getattr(p + e, k) >= getattr(self.lo, k)
                   and getattr(p - e, k) <= getattr(self.hi, k) for k in ("x", "y", "z"))

    def line(self, start, end, complex=False, ignore=(), profile="Pawn"):
        return unreal.SystemLibrary.line_trace_single_by_profile(
            self.world, start, end, profile, complex, list(ignore), unreal.DrawDebugTrace.NONE)

    def capsule(self, start, end, ignore=(), complex=False):
        return unreal.SystemLibrary.capsule_trace_single_by_profile(
            self.world, start, end, RADIUS, HALF_HEIGHT, "Pawn", complex,
            list(ignore), unreal.DrawDebugTrace.NONE)

    def nav_at(self, point, extent=(80, 80, 140)):
        return unreal.NavigationSystemV1.project_point_to_navigation(
            self.world, point, self.nav, None, unreal.Vector(*extent))

    def floor(self, point, ignore=(), above=30, below=120):
        return self.line(point + unreal.Vector(0, 0, above),
                         point - unreal.Vector(0, 0, below), ignore=ignore)

    def spec(self, kind, source, center, rotation, scale, evidence=None):
        key = json.dumps([kind, source.get_path_name(), vec(center),
                          [round(rotation.pitch, 4), round(rotation.yaw, 4), round(rotation.roll, 4)],
                          vec(scale)], sort_keys=True)
        identifier = hashlib.sha1(key.encode()).hexdigest()[:16]
        plan = {"id": identifier, "kind": kind, "source": source.get_path_name(),
                "label": source.get_actor_label(), "location": vec(center),
                "rotation": [rotation.pitch, rotation.yaw, rotation.roll],
                "scale": vec(scale), "evidence": evidence or {}}
        self.plans.append(plan)
        return plan

    def audit_stairs(self):
        stairs = []
        for a in self.sources:
            c = a.static_mesh_component
            if not c.static_mesh or c.static_mesh.get_name() != "SM_RampSteps":
                continue
            center, _, _ = unreal.SystemLibrary.get_component_bounds(c)
            if not self.inside(center):
                continue
            t = c.get_world_transform()
            lo, hi = c.get_local_bounds()
            origin = unreal.MathLibrary.transform_location(t, unreal.Vector(0, 0, 0))
            d = unit(unreal.MathLibrary.transform_location(t, unreal.Vector(1, 0, 0)) - origin)
            lateral = unreal.Vector(-d.y, d.x, 0)
            if abs(d.z) > .001:
                raise RuntimeError("Unexpected tilted stair transform: " + a.get_actor_label())
            corners = [unreal.MathLibrary.transform_location(t, unreal.Vector(x, y, 0))
                       for x in (lo.x, hi.x) for y in (lo.y, hi.y)]
            stairs.append({"actor": a, "component": c, "transform": t, "lo": lo, "hi": hi,
                           "d": d, "l": lateral,
                           "s0": min(dot(p, d) for p in corners),
                           "s1": max(dot(p, d) for p in corners),
                           "w0": min(dot(p, lateral) for p in corners),
                           "w1": max(dot(p, lateral) for p in corners)})
        groups = []
        for s in stairs:
            match = None
            for g in groups:
                ref = g[0]
                if dot(s["d"], ref["d"]) > .99999 and abs(s["w0"] - ref["w0"]) < 3 \
                        and abs(s["w1"] - ref["w1"]) < 3 \
                        and s["s0"] <= max(q["s1"] for q in g) + 3 \
                        and s["s1"] >= min(q["s0"] for q in g) - 3:
                    match = g
                    break
            if match is None:
                groups.append([s])
            else:
                match.append(s)
        all_stairs = [s["actor"] for s in stairs]
        self.stair_groups = []
        for g in groups:
            d, lateral = g[0]["d"], g[0]["l"]
            s0, s1 = min(s["s0"] for s in g), max(s["s1"] for s in g)
            w0, w1 = min(s["w0"] for s in g), max(s["w1"] for s in g)
            w = (w0 + w1) / 2
            samples = []
            for i in range(61):
                distance = s0 + (s1 - s0) * (i + .5) / 61
                p = d * distance + lateral * w
                heights = []
                for s in g:
                    h = s["component"].line_trace_component(
                        p + unreal.Vector(0, 0, 5000), p - unreal.Vector(0, 0, 500), True, False, False)
                    if h:
                        heights.append(h[0].z)
                if heights:
                    samples.append((distance, max(heights)))
            if len(samples) < 50:
                raise RuntimeError("Incomplete stair surface sampling")
            high = max(z for s, z in samples[:4])
            low = min(z for s, z in samples[-4:])
            for top in (True, False):
                distance = s0 - 10 if top else s1 + 10
                z = high if top else low
                p = d * distance + lateral * w + unreal.Vector(0, 0, z)
                h = self.floor(p, all_stairs + self.ignored_decor, above=40, below=60)
                if h and h.to_tuple()[7].z > .7 and abs(h.to_tuple()[5].z - z) <= STEP:
                    if top:
                        high = h.to_tuple()[5].z
                    else:
                        low = h.to_tuple()[5].z
            angle = math.degrees(math.atan2(high - low, s1 - s0))
            if not 0 < angle <= 44:
                raise RuntimeError("Stair replacement exceeds unchanged navigation slope")
            # Follow the upper envelope of tread noses. Several meshes have a
            # long top landing: one plane through their AABB would put feet up
            # to 42 cm inside visible treads. Broad joined inclines eliminate
            # that penetration without reintroducing individual risers.
            points = [(s0 - 20, high)] + [(s, z + 1) for s, z in samples] + [(s1 + 40, low)]
            hull = []
            for p in points:
                while len(hull) > 1:
                    a, b = hull[-2:]
                    cross = (b[0] - a[0]) * (p[1] - b[1]) - (b[1] - a[1]) * (p[0] - b[0])
                    if cross < 0:
                        break
                    hull.pop()
                hull.append(p)

            def simplify(part):
                a, b = part[0], part[-1]
                errors = [p[1] - (a[1] + (b[1] - a[1]) * (p[0] - a[0]) / (b[0] - a[0]))
                          for p in part]
                index = max(range(len(part)), key=lambda i: errors[i])
                if errors[index] <= 2 or index in (0, len(part) - 1):
                    return [a, b]
                return simplify(part[:index + 1])[:-1] + simplify(part[index:])

            path = [d * s + lateral * w + unreal.Vector(0, 0, z) for s, z in simplify(hull)]
            plans = []
            for upper, lower in zip(path, path[1:]):
                tangent = unit(lower - upper)
                # Unreal's Python Vector unary minus mutates the input struct.
                normal = unit((lateral * -1).cross(tangent))
                slope = math.degrees(math.acos(normal.z))
                if slope > 44:
                    raise RuntimeError("Stair envelope exceeds existing walkable slope: "
                                       + str((g[0]["actor"].get_actor_label(), vec(upper), vec(lower), slope)))
                rotation = unreal.MathLibrary.make_rot_from_yz(tangent, normal)
                # Thin slabs preserve the passages below the stacked stairs.
                thickness = 12.0
                center = (upper + lower) / 2 - normal * (thickness / 2)
                scale = unreal.Vector((w1 - w0) / 100, (lower - upper).length() / 100, thickness / 100)
                plans.append(self.spec("Stair", g[0]["actor"], center, rotation, scale,
                                       {"sources": [q["actor"].get_path_name() for q in g],
                                        "upper": vec(upper), "lower": vec(lower), "angle": slope}))
            self.stair_groups.append({"plans": plans, "sources": [s["actor"] for s in g],
                                      "path": path, "upper": path[0], "lower": path[-1],
                                      "lateral": lateral, "width": w1 - w0})
        return {"source_stairs": len(stairs), "straight_flights": len(groups),
                "flights": [{"sources": [a.get_actor_label() for a in g["sources"]],
                             "path": [vec(p) for p in g["path"]], "slabs": len(g["plans"])}
                            for g in self.stair_groups]}

    def audit_decor(self, seeds=None):
        if seeds is None:
            seeds = DECOR_PROBES
        self.decor_plans = []
        seen = set()
        for item in seeds:
            label = item["label"]
            if label in seen:
                continue
            seen.add(label)
            a = next((a for a in self.sources if a.get_actor_label() == label), None)
            if not a:
                continue
            c = a.static_mesh_component
            if not c.static_mesh or not c.static_mesh.get_name().startswith(("SM_Light", "SM_Prop04")):
                continue
            if c.get_collision_enabled() == unreal.CollisionEnabled.NO_COLLISION:
                continue
            start, end = unreal.Vector(*item["nav_a"]), unreal.Vector(*item["nav_b"])
            z = max(start.z, end.z) + HALF_HEIGHT + 2
            start.z = end.z = z
            before = self.capsule(start, end)
            after = self.capsule(start, end, [a])
            if not before or before.to_tuple()[9] != a or before.to_tuple()[1] or after:
                continue
            # Check ground on both sides and directly below the ornament.
            ground = []
            for p in (start, (start + end) / 2, end):
                foot = p - unreal.Vector(0, 0, HALF_HEIGHT + 2)
                h = self.floor(foot, [a], above=35, below=60)
                if h and h.to_tuple()[7].z > .7:
                    ground.append(hit_info(h))
            if len(ground) != 3:
                continue
            self.decor_plans.append({"actor": a, "component": c, "before": state(a, c),
                                     "start": vec(start), "end": vec(end), "ground": ground,
                                     "blocking_hit": hit_info(before)})
        self.ignored_decor = [p["actor"] for p in self.decor_plans]
        return [{k: v for k, v in p.items() if k not in ("actor", "component")}
                for p in self.decor_plans]

    def audit_walls(self):
        """Tile real vertical faces, then merge coplanar missing collision cells.

        Nine complex traces must agree across each cell. Openings and bevel
        boundaries fail that test and are left open; no actor AABB is filled.
        """
        self.wall_plans = []
        protected = []
        for actor in self.actors:
            if "Portal" not in actor.get_class().get_name():
                continue
            for c in actor.get_components_by_class(unreal.PrimitiveComponent):
                if c.get_name() in ("EntryCollision", "PortalScreen", "entryCollision"):
                    p, e, _ = unreal.SystemLibrary.get_component_bounds(c)
                    protected.append((p - e - unreal.Vector(20, 20, 20),
                                      p + e + unreal.Vector(20, 20, 20)))

        def portal_overlap(points):
            for lo, hi in protected:
                if all(max(getattr(p, k) for p in points) >= getattr(lo, k)
                       and min(getattr(p, k) for p in points) <= getattr(hi, k)
                       for k in ("x", "y", "z")):
                    return True
            return False

        scanned, missing_cells, unmerged = 0, 0, 0
        for actor in self.sources:
            c = actor.static_mesh_component
            sm = c.static_mesh
            if not sm or not self.overlaps(c):
                continue
            name = sm.get_name()
            if not any(k in name for k in ("Wall", "Corridor", "Boolean", "04map", "Deck")):
                continue
            if self.sms.get_collision_complexity(sm) == unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE:
                continue
            _, extent, _ = unreal.SystemLibrary.get_component_bounds(c)
            if extent.z < 88:
                continue
            scanned += 1
            t = c.get_world_transform()
            lo, hi = c.get_local_bounds()
            lov, hiv = vec(lo), vec(hi)
            base = unreal.MathLibrary.transform_location(t, unreal.Vector())
            axes = [unreal.MathLibrary.transform_location(t, unreal.Vector(*(1 if i == j else 0
                                                                          for i in range(3)))) - base
                    for j in range(3)]
            groups = {}
            reachable = False
            for axis in range(3):
                if abs(unit(axes[axis]).z) > .35:
                    continue
                u, v = [i for i in range(3) if i != axis]
                widths = ((hiv[u] - lov[u]) * axes[u].length(),
                          (hiv[v] - lov[v]) * axes[v].length())
                if min(widths) < 5:
                    continue
                nu, nv = [max(1, math.ceil(w / GRID)) for w in widths]
                du, dv = (hiv[u] - lov[u]) / nu, (hiv[v] - lov[v]) / nv

                def cast(uu, vv, sign):
                    a = [0.0, 0.0, 0.0]
                    b = [0.0, 0.0, 0.0]
                    a[u] = b[u] = uu
                    a[v] = b[v] = vv
                    a[axis] = hiv[axis] + 50 / axes[axis].length() if sign > 0 else lov[axis] - 50 / axes[axis].length()
                    b[axis] = lov[axis] - 50 / axes[axis].length() if sign > 0 else hiv[axis] + 50 / axes[axis].length()
                    return c.line_trace_component(unreal.MathLibrary.transform_location(t, unreal.Vector(*a)),
                                                  unreal.MathLibrary.transform_location(t, unreal.Vector(*b)),
                                                  True, False, False)

                for sign in (-1, 1):
                    for iu in range(nu):
                        for iv in range(nv):
                            uu, vv = lov[u] + (iu + .5) * du, lov[v] + (iv + .5) * dv
                            h = cast(uu, vv, sign)
                            if not h or abs(h[1].z) > .35 or not self.inside(h[0]):
                                continue
                            point, normal = h[0], unit(h[1])
                            if self.line(point + normal * 6, point - normal * 6):
                                continue
                            hits = []
                            valid = True
                            for fu, fv in ((0, 0), (0, .5), (0, 1), (.5, 0), (.5, .5),
                                           (.5, 1), (1, 0), (1, .5), (1, 1)):
                                corner = cast(lov[u] + (iu + .002 + .996 * fu) * du,
                                              lov[v] + (iv + .002 + .996 * fv) * dv, sign)
                                if not corner or dot(unit(corner[1]), normal) < .995 \
                                        or abs(dot(corner[0] - point, normal)) > .5 \
                                        or not self.inside(corner[0]):
                                    valid = False
                                    break
                                hits.append(corner[0])
                            if not valid or portal_overlap(hits):
                                continue
                            if not reachable:
                                probe = point + normal * 70 - unreal.Vector(0, 0, 88)
                                nav = self.nav_at(probe, (90, 90, 180))
                                reachable = nav is not None and abs(nav.z - probe.z) <= 180
                            key = (axis, sign, *(round(k, 3) for k in vec(normal)), round(dot(point, normal)))
                            if key not in groups:
                                groups[key] = {"axis": axis, "u": u, "v": v, "du": du, "dv": dv,
                                               "point": point, "normal": normal, "cells": set()}
                            groups[key]["cells"].add((iu, iv))
            if not reachable:
                continue
            for group in groups.values():
                cells = group["cells"]
                missing_cells += len(cells)
                u, v, axis = group["u"], group["v"], group["axis"]
                normal, point = group["normal"], group["point"]

                def plane_point(uu, vv):
                    local = [0.0, 0.0, 0.0]
                    local[u], local[v] = uu, vv
                    world = unreal.MathLibrary.transform_location(t, unreal.Vector(*local))
                    local[axis] = dot(point - world, normal) / dot(axes[axis], normal)
                    return unreal.MathLibrary.transform_location(t, unreal.Vector(*local))

                while cells:
                    iu, iv = min(cells)
                    end_u = iu + 1
                    while (end_u, iv) in cells:
                        end_u += 1
                    end_v = iv + 1
                    while all((i, end_v) in cells for i in range(iu, end_u)):
                        end_v += 1
                    count = (end_u - iu) * (end_v - iv)
                    cells.difference_update((i, j) for i in range(iu, end_u) for j in range(iv, end_v))
                    a = plane_point(lov[u] + iu * group["du"], lov[v] + iv * group["dv"])
                    b = plane_point(lov[u] + end_u * group["du"], lov[v] + iv * group["dv"])
                    d = plane_point(lov[u] + iu * group["du"], lov[v] + end_v * group["dv"])
                    e = b + d - a
                    if portal_overlap([a, b, d, e]) or abs(dot(unit(b - a), unit(d - a))) > .02:
                        unmerged += count
                        continue
                    center = (b + d) / 2 - normal
                    rotation = unreal.MathLibrary.make_rot_from_xz(unit(b - a), normal)
                    scale = unreal.Vector((b - a).length() / 100, (d - a).length() / 100, .02)
                    plan = self.spec("Wall", actor, center, rotation, scale,
                                     {"corners": [vec(p) for p in (a, b, e, d)],
                                      "normal": vec(normal), "cells": count})
                    self.wall_plans.append(plan)
        return {"scanned_sources": scanned, "missing_cells": missing_cells,
                "patches": len(self.wall_plans), "source_actors": len(set(p["source"] for p in self.wall_plans)),
                "nonrectangular_cells_skipped": unmerged,
                "per_source": collections.Counter(p["label"] for p in self.wall_plans)}

    def apply_walls(self):
        with unreal.ScopedEditorTransaction("Fill Portal Map missing wall collision"):
            grouped = collections.defaultdict(list)
            for plan in self.wall_plans:
                grouped[plan["source"]].append(plan)
            by_path = {a.get_path_name(): a for a in self.sources}
            for path, plans in grouped.items():
                self.upsert_group("Wall", by_path[path], plans)

    def repair_verified_edge(self, source, point, normal):
        """Resolve a proven residual ray hole at a bevel narrower than the grid."""
        component = source.static_mesh_component
        hit = component.line_trace_component(point + normal * 10, point - normal * 10,
                                             True, False, False)
        if not hit or not self.inside(hit[0]):
            return None
        point, normal = hit[0], unit(hit[1])
        u = unit(unreal.Vector(0, 0, 1).cross(normal))
        v = unit(normal.cross(u))
        for half in (40, 20, 10, 5, 2.5, 1, .5):
            valid = True
            for x in (-half, 0, half):
                for y in (-half, 0, half):
                    p = point + u * x + v * y
                    h = component.line_trace_component(p + normal * 3, p - normal * 3,
                                                       True, False, False)
                    if not h or dot(unit(h[1]), normal) < .995 \
                            or abs(dot(h[0] - point, normal)) > .25 or not self.inside(h[0]):
                        valid = False
                        break
                if not valid:
                    break
            if valid:
                plan = self.spec("Wall", source, point - normal,
                                 unreal.MathLibrary.make_rot_from_xz(u, normal),
                                 unreal.Vector(half * 2 / 100, half * 2 / 100, .02),
                                 {"verified_edge": vec(point), "normal": vec(normal),
                                  "half_size": half})
                self.wall_plans.append(plan)
                matching = [p for p in self.wall_plans if p["source"] == source.get_path_name()]
                self.upsert_group("Wall", source, matching)
                return plan
        return None

    def _remember(self, actor, component, reason):
        key = actor.get_path_name() + ":" + component.get_name()
        if key not in self.baseline:
            self.baseline[key] = {**state(actor, component), "reason": reason}
        actor.modify()
        component.modify()
        package = actor.get_package()
        if package not in self.packages:
            self.packages.append(package)

    def apply_surfaces(self):
        with unreal.ScopedEditorTransaction("Smooth Portal Map stairs and floor ornaments"):
            for g in self.stair_groups:
                self.upsert_group("Stair", g["sources"][0], g["plans"])
                for a in g["sources"]:
                    c = a.static_mesh_component
                    self._remember(a, c, "replace stair treads with continuous slope")
                    c.set_collision_profile_name("NoCollision", True)
                    c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
            for p in self.decor_plans:
                a, c = p["actor"], p["component"]
                self._remember(a, c, "floor ornament obstructs a supported clear route")
                c.set_collision_profile_name("NoCollision", True)
                c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)

    def upsert_group(self, kind, source, plans):
        """One persistent actor/ISM component per source, not one actor per box."""
        identifier = hashlib.sha1((kind + source.get_path_name()).encode()).hexdigest()[:16]
        id_tag = "PCFId=" + identifier
        actor = next((a for a in self.eas.get_all_level_actors()
                      if id_tag in [str(t) for t in a.tags]), None)
        if actor is None:
            actor = self.eas.spawn_actor_from_class(unreal.StaticMeshActor, source.get_actor_location())
        actor.modify()
        actor.set_editor_property("tags", [unreal.Name(TAG), unreal.Name(id_tag),
                                            unreal.Name("PCFSource=" + source.get_name())])
        root = actor.static_mesh_component
        root.modify()
        root.set_collision_profile_name("NoCollision", True)
        root.set_visibility(False)
        component = actor.get_component_by_class(unreal.InstancedStaticMeshComponent)
        if component is None:
            subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
            handles = subsystem.k2_gather_subobject_data_for_instance(actor)
            params = unreal.AddNewSubobjectParams(parent_handle=handles[1],
                                                  new_class=unreal.InstancedStaticMeshComponent)
            handle, reason = subsystem.add_new_subobject(params)
            library = unreal.SubobjectDataBlueprintFunctionLibrary
            component = library.get_object(library.get_data(handle))
            if not isinstance(component, unreal.InstancedStaticMeshComponent):
                raise RuntimeError("Cannot create persistent collision instances: " + str(reason))
        component.modify()
        component.set_mobility(unreal.ComponentMobility.STATIC)
        component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
        component.set_visibility(False)
        component.set_hidden_in_game(True)
        component.set_collision_profile_name("BlockAll", True)
        component.set_editor_property("can_ever_affect_navigation", True)
        component.clear_instances()
        for plan in plans:
            transform = unreal.Transform(location=unreal.Vector(*plan["location"]),
                                         rotation=rotator(plan["rotation"]),
                                         scale=unreal.Vector(*plan["scale"]))
            index = component.add_instance(transform, world_space=True)
            plan["proxy_actor"], plan["instance_index"] = actor.get_path_name(), index
        actor.set_actor_label("PCF_" + kind + "_" + source.get_actor_label())
        actor.set_folder_path("CollisionFix/" + kind)
        actor.set_editor_property("tags", [unreal.Name(TAG), unreal.Name(id_tag),
                                            unreal.Name("PCFSource=" + source.get_name())])
        actor.set_actor_hidden_in_game(True)
        if actor.get_package() not in self.packages:
            self.packages.append(actor.get_package())
        return actor

    def report(self, name="repair_report"):
        os.makedirs(self.report_dir, exist_ok=True)
        path = os.path.join(self.report_dir, name + ".json")
        with open(path, "w", encoding="utf-8") as f:
            json.dump({"map": MAP, "bounds": [vec(self.lo), vec(self.hi)],
                       "baseline": self.baseline, "proxies": self.plans,
                       "decor": [{k: v for k, v in p.items() if k not in ("actor", "component")}
                                 for p in getattr(self, "decor_plans", [])],
                       "validation": self.validation,
                       "packages": [p.get_name() for p in self.packages]},
                      f, ensure_ascii=False, indent=2)
        return path

    def save(self):
        # RebuildNavigation dirties its own external actor; actor folder records
        # are separate external objects. Save these dependencies explicitly.
        nav_package = self.nav.get_package()
        dirty = unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
        for package in dirty:
            if package == nav_package or package.get_name().startswith(
                    "/Game/__ExternalObjects__/QuakeLike_Base/Lvl_PortalMap/"):
                if package not in self.packages:
                    self.packages.append(package)
        if not unreal.EditorLoadingAndSavingUtils.save_packages(self.packages, True):
            raise RuntimeError("Failed to save collision repair actor packages")
        return self.report()
