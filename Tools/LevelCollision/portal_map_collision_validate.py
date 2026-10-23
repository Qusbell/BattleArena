"""Temporary SIE checks using an actual spawned AI character.

Only operates on the PIE world. Stopping SIE discards all pawn/controller edits.
The editor level and gameplay Blueprints are never changed by this validator.
"""

import math
import traceback

import unreal


class MovementValidation:
    def __init__(self, repair):
        self.world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        if self.world is None or "UEDPIE_" not in self.world.get_path_name():
            raise RuntimeError("Start editor simulation first")
        self.pawns = list(unreal.GameplayStatics.get_all_actors_of_class(self.world, unreal.Character))
        if not self.pawns:
            raise RuntimeError("The map has not spawned its AI characters")
        # PathFollowing can continue issuing requested velocity after BT stop
        # and unpossession. Stop its ticks as well as the character's AI ticks.
        for controller in unreal.GameplayStatics.get_all_actors_of_class(self.world, unreal.AIController):
            controller.stop_movement()
            controller.set_actor_tick_enabled(False)
            for component in controller.get_components_by_class(unreal.ActorComponent):
                component.set_component_tick_enabled(False)
        for pawn in self.pawns:
            controller = pawn.get_controller()
            if controller:
                brain = controller.get_component_by_class(unreal.BrainComponent)
                if brain:
                    brain.stop_logic("Temporary level collision validation")
                controller.stop_movement()
            pawn.set_editor_property("can_be_damaged", False)
        self.pawn = self.pawns[0]
        self.cap = self.pawn.get_component_by_class(unreal.CapsuleComponent)
        self.move = self.pawn.get_component_by_class(unreal.CharacterMovementComponent)
        # The project also steers AI from actor/component ticks outside its BT.
        # Isolate the character movement simulation from those route commands.
        controller = self.pawn.get_controller()
        if controller:
            controller.un_possess()
        self.pawn.set_actor_tick_enabled(False)
        for component in self.pawn.get_components_by_class(unreal.ActorComponent):
            if component != self.move:
                component.set_component_tick_enabled(False)
        self.cap.set_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN,
                                                   unreal.CollisionResponseType.ECR_IGNORE)
        self.radius = self.cap.get_scaled_capsule_radius()
        self.height = self.cap.get_scaled_capsule_half_height()
        self.move.set_editor_property("run_physics_with_no_controller", True)
        self.speed = 800.0
        self.move.set_editor_property("max_walk_speed", self.speed)
        self.results, self.cases = [], []
        for group in repair.stair_groups:
            for reverse in (False, True):
                self.cases.append({"kind": "stair", "name": group["sources"][0].get_actor_label(),
                                   "direction": "up" if reverse else "down",
                                   "start": group["lower"] if reverse else group["upper"],
                                   "end": group["upper"] if reverse else group["lower"]})
        for plan in repair.decor_plans:
            start, end = unreal.Vector(*plan["start"]), unreal.Vector(*plan["end"])
            start.z -= 90
            end.z -= 90
            for reverse in (False, True):
                self.cases.append({"kind": "decor", "name": plan["before"]["label"],
                                   "direction": "reverse" if reverse else "forward",
                                   "start": end if reverse else start,
                                   "end": start if reverse else end})
        for a in repair.actors:
            if "Direction_JumpPad" not in a.get_class().get_name():
                continue
            c = next(c for c in a.get_components_by_class(unreal.StaticMeshComponent)
                     if c.get_name() == "JumpPad")
            center, _, _ = unreal.SystemLibrary.get_component_bounds(c)
            center.z -= 15
            self.cases.append({"kind": "jump_pad", "name": a.get_actor_label(),
                               "direction": "launch", "start": center, "end": center})
        self.index, self.current, self.handle = 0, None, None
        self.error = None
        self.done = False

    @staticmethod
    def vector(v):
        return [round(v.x, 2), round(v.y, 2), round(v.z, 2)]

    def floor(self, point):
        return unreal.SystemLibrary.line_trace_single_by_profile(
            self.world, point + unreal.Vector(0, 0, 65), point - unreal.Vector(0, 0, 180),
            "Pawn", False, self.pawns, unreal.DrawDebugTrace.NONE)

    def begin(self):
        self.handle = unreal.register_slate_post_tick_callback(self.tick)

    def start_case(self, case, now):
        self.move.stop_movement_immediately()
        self.pawn.consume_movement_input_vector()
        ground = case["start"].copy()
        hit = self.floor(ground)
        nz = 1.0
        if hit and hit.to_tuple()[7].z > .7:
            ground = hit.to_tuple()[5]
            nz = hit.to_tuple()[7].z
        ground = ground + unreal.Vector(0, 0, self.height + self.radius * (1 / nz - 1) + 3)
        self.move.set_movement_mode(unreal.MovementMode.MOVE_WALKING)
        self.pawn.set_actor_location(ground, False, True)
        delta = case["end"] - case["start"]
        delta.z = 0
        length = delta.length()
        direction = delta / length if length else unreal.Vector()
        self.current = {**case, "began": now, "start_location": ground, "direction_vector": direction,
                        "timeout": max(3, length / max(self.speed, 1) * 4 + 3), "samples": []}

    def finish_case(self, passed, reason, now):
        c = self.current
        position = self.pawn.get_actor_location()
        self.results.append({"kind": c["kind"], "name": c["name"], "direction": c["direction"],
                             "passed": passed, "reason": reason, "seconds": round(now - c["began"], 2),
                             "start": self.vector(c["start_location"]), "end": self.vector(position),
                             "target": self.vector(c["end"]), "samples": c["samples"]})
        self.current = None
        self.index += 1

    def stop(self):
        if self.handle is not None:
            unreal.unregister_slate_post_tick_callback(self.handle)
            self.handle = None
        if self.pawn:
            self.move.stop_movement_immediately()
        self.done = True

    def tick(self, _delta):
        try:
            now = unreal.GameplayStatics.get_time_seconds(self.world)
            if self.current is None:
                if self.index == len(self.cases):
                    self.stop()
                    return
                self.start_case(self.cases[self.index], now)
            c = self.current
            position = self.pawn.get_actor_location()
            elapsed = now - c["began"]
            if not c["samples"] or elapsed - c["samples"][-1][0] > .25:
                c["samples"].append([round(elapsed, 2), self.vector(position)])
            if c["kind"] == "jump_pad":
                velocity = self.pawn.get_velocity()
                if velocity.z > 200 or position.z - c["start_location"].z > 100:
                    self.finish_case(True, "trigger launched the actual AI character", now)
                elif elapsed > 3:
                    self.finish_case(False, "launch not observed", now)
                return
            distance = math.hypot(position.x - c["end"].x, position.y - c["end"].y)
            delta = position - c["end"]
            progress = delta.x * c["direction_vector"].x + delta.y * c["direction_vector"].y
            lateral = abs(delta.x * c["direction_vector"].y - delta.y * c["direction_vector"].x)
            if elapsed > .35 and (distance <= 55 or progress >= 0 and lateral <= 55):
                height_error = abs((position.z - self.height) - c["end"].z)
                self.finish_case(height_error < 95, "reached opposite end; foot height error %.1f cm" % height_error, now)
                return
            if elapsed > c["timeout"]:
                self.finish_case(False, "movement timed out; remaining %.1f cm" % distance, now)
                return
            if elapsed > .35:
                self.pawn.add_movement_input(c["direction_vector"], 1.0, True)
        except Exception:
            self.error = traceback.format_exc()
            self.stop()

    def status(self):
        return {"done": self.done, "completed": len(self.results), "total": len(self.cases),
                "passed": sum(r["passed"] for r in self.results),
                "capsule_radius": self.radius, "capsule_half_height": self.height,
                "speed": self.speed, "error": self.error,
                "current": {k: self.current[k] for k in ("kind", "name", "direction")} if self.current else None,
                "failures": [r for r in self.results if not r["passed"]]}
