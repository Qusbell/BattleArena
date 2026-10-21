"""Execute via Unreal MCP in PortalMap PIE; exercises real debug Blueprint and tile churn.

No editor assets are changed. The selected PIE pawn is restored after the Blueprint fixture.
Results are written after 20 seconds to Saved/PathLinkBench/RuntimeReferenceRegression.json.
"""
import json
import time
from pathlib import Path
import unreal

_ref_sub = next(s for s in unreal.ObjectIterator(unreal.PathLinkSubsystem)
                if '/UEDPIE_0_Lvl_PortalMap.' in s.get_path_name())
_ref_world = _ref_sub.get_world()
_ref_cases = [
    ((1185.449396119161, -4228.476337821451, 2085.926174342145),
     (659.3372697826575, -2541.497452245849, 2418.7875044217776)),
    ((1185.449396119161, -4228.476337821451, 2085.926174342145),
     (676.6743601742373, -3020.73987340316, 2281.1507568359375)),
    ((1185.449396119161, -4228.476337821451, 2085.926174342145),
     (690.7673012793729, -3225.598325405726, 2162.660903930664)),
    ((1156.2872312427694, -4212.330329620268, 1898.5077705551648),
     (676.6743601742373, -3020.73987340316, 2281.1507568359375)),
]
_ref_rows = []
_ref_started = time.monotonic()
_ref_last = 0.0

# Invoke the actual Blueprint graph at the previously failing start location.
_ref_cls = unreal.load_class(None, '/Game/QuakeLike_1_0/Test/'
                           'BPC_DebugAI_AttachToPlayerController.BPC_DebugAI_AttachToPlayerController_C')
_ref_debug = next(c for c in unreal.ObjectIterator(_ref_cls) if c.get_world() == _ref_world)
_ref_valid, _ref_controller, _ref_pawn = _ref_debug.call_method('Get Valid Debug Target')
if not _ref_valid:
    _ref_debug.call_method('Call Debug AI')
    _ref_valid, _ref_controller, _ref_pawn = _ref_debug.call_method('Get Valid Debug Target')
assert _ref_valid, 'No debug pawn; spawn bots before this test'
_ref_old_location = _ref_pawn.get_actor_location()
try:
    _ref_pawn.set_actor_location(unreal.Vector(*_ref_cases[0][0]), False, True)
    _ref_debug.call_method('ReceiveTick', (0.0,))
finally:
    _ref_pawn.set_actor_location(_ref_old_location, False, True)


def _ref_tick(delta):
    global _ref_last
    now = time.monotonic()
    if now - _ref_started >= 20:
        unreal.unregister_slate_post_tick_callback(_ref_handle)
        values = sorted(r['ms'] for r in _ref_rows)
        summary = dict(queries=len(_ref_rows), failures=sum(r['distance_cm'] is None for r in _ref_rows),
                       synchronous_queries=sum(r['sync_queries'] for r in _ref_rows),
                       mean_ms=sum(values)/len(values), max_ms=max(values),
                       p95_ms=values[int((len(values)-1)*0.95)],
                       cache_status=_ref_sub.get_routing_cache_status(),
                       cache_mib=_ref_sub.get_runtime_cache_mi_b())
        folder = Path(unreal.Paths.project_saved_dir()) / 'PathLinkBench'
        folder.mkdir(parents=True, exist_ok=True)
        (folder / 'RuntimeReferenceRegression.json').write_text(
            json.dumps(dict(summary=summary, cases=_ref_rows), indent=2), encoding='utf-8')
        print('RuntimeReferenceRegression:', json.dumps(summary))
        assert summary['failures'] == 0, 'Previously failing coordinates must survive runtime tile churn'
        assert summary['synchronous_queries'] == 0, 'Estimate must not fall back to detailed navigation'
        return
    if now - _ref_last < 0.25:
        return
    _ref_last = now
    for index, (a, b) in enumerate(_ref_cases):
        distance = _ref_sub.get_estimated_route_distance(unreal.Vector(*a), unreal.Vector(*b), None)
        stats = _ref_sub.get_last_estimate_stats()
        _ref_rows.append(dict(sample=index, seconds=now-_ref_started, distance_cm=distance,
                              status=str(_ref_sub.get_last_estimate_status()),
                              ms=stats.total_milliseconds, sync_queries=stats.synchronous_nav_queries))


_ref_handle = unreal.register_slate_post_tick_callback(_ref_tick)
print('RuntimeReferenceRegression: sampling actual PIE NavMesh for 20 seconds')
