"""Aggregate the fixed 204 before/after samples and independent exact-reference results."""
import json
import math
import statistics as st
from pathlib import Path

root = Path(__file__).resolve().parents[1]
folder = root / "Saved/PathLinkBench"
before = json.loads((folder / "LazySearch_before.json").read_text(encoding="utf-8"))["cases"]
after = json.loads((folder / "LazySearch_after.json").read_text(encoding="utf-8"))["cases"]
reference = json.loads((folder / "LazySearch_ExactReference.json").read_text(encoding="utf-8"))
assert [(r["group"], r["sample"], r["a"], r["b"]) for r in before] == [(r["group"], r["sample"], r["a"], r["b"]) for r in after]

def percentile(values, fraction): return sorted(values)[math.ceil(len(values)*fraction)-1]
def metrics(rows):
    errors = [(r["estimate_cm"]-r["distance_cm"])/r["distance_cm"]*100 for r in rows]
    timings = [s for r in rows for s in r["stats"]]
    return dict(pairs=len(rows), mean_us=st.mean(s["us"] for s in timings),
                p95_us=percentile([s["us"] for s in timings], .95), max_us=max(s["us"] for s in timings),
                mean_queries=st.mean(s["queries"] for s in timings), max_queries=max(s["queries"] for s in timings),
                mean_solves=st.mean(s["solves"] for s in timings),
                mean_candidate_us=st.mean(s["candidate_us"] for s in timings),
                mean_solve_us=st.mean(s["solve_us"] for s in timings),
                mean_points_us=st.mean(s["points_us"] for s in timings),
                mean_abs_m=st.mean(abs(r["estimate_cm"]-r["distance_cm"])/100 for r in rows),
                mean_abs_pct=st.mean(abs(e) for e in errors), p95_abs_pct=percentile([abs(e) for e in errors], .95),
                max_over_pct=max([0]+errors), max_under_pct=-min([0]+errors),
                over10=sum(e>10 for e in errors), under10=sum(e< -10 for e in errors),
                estimate_mean_us=st.mean(r["estimate_us"] for r in rows),
                estimate_queries=sum(r["estimate_queries"] for r in rows),
                mean_distance_only_us=st.mean(r["distance_only_us"] for r in rows),
                paths_using_links=sum(bool(r["links"]) for r in rows),
                teleport_paths=sum(any("TELEPORT" in t.upper() for t in r["link_types"]) for r in rows),
                reverse_paths=sum(any(r["reverse"]) for r in rows),
                multiple_link_paths=sum(len(r["links"])>1 for r in rows))

fresh_before = [r for r in before if r["group"]=="fresh"]
fresh_after = [r for r in after if r["group"]=="fresh"]
changes = [dict(sample=a["sample"],before_m=b["distance_cm"]/100,after_m=a["distance_cm"]/100,
                shorter_m=(b["distance_cm"]-a["distance_cm"])/100,before_links=b["links"],after_links=a["links"])
           for b,a in zip(fresh_before,fresh_after) if abs(b["distance_cm"]-a["distance_cm"])>.01]
outliers = [dict(sample=a["sample"],before_m=b["distance_cm"]/100,after_m=a["distance_cm"]/100,
                 estimate_m=a["estimate_cm"]/100,error_pct=(a["estimate_cm"]/a["distance_cm"]-1)*100,
                 links=a["links"]) for b,a in zip(before[:4],after[:4])]
summary = dict(seed=20261026,before=metrics(fresh_before),after=metrics(fresh_after),outliers=outliers,
               changed_fresh_routes=changes,unchanged_estimates=all(a["estimate_cm"]==b["estimate_cm"] for a,b in zip(after,before)),
               exact_reference={k:v for k,v in reference.items() if k!="cases"})
(folder / "LazySearch_Summary.json").write_text(json.dumps(summary,indent=2),encoding="utf-8")
print(json.dumps(summary,indent=2))
