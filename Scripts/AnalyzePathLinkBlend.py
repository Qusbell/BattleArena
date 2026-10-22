"""Analyze PathLink.BenchmarkEstimateBlend CSV. Distances are cm; relative errors exclude zero-length routes."""
import csv
import json
import statistics as st
from collections import defaultdict
from pathlib import Path

root = Path(__file__).resolve().parents[1]
rows = list(csv.DictReader((root / "Saved/PathLinkBench/Blend200.csv").open(encoding="utf-8-sig")))
groups = defaultdict(list)
for row in rows:
    groups[(float(row["strength"]), float(row["range_cm"]))].append(row)

def metrics(data):
    valid = [r for r in data if int(r["exact_success"]) and int(r["estimate_success"]) and float(r["exact_cm"]) > 1]
    errors = [(float(r["estimate_cm"]) - float(r["exact_cm"])) / float(r["exact_cm"]) for r in valid]
    absolute = sorted(abs(e) for e in errors)
    return dict(samples=len(data), compared=len(valid),
                mean_absolute_m=st.mean(abs(float(r["estimate_cm"])-float(r["exact_cm"]))/100 for r in valid) if valid else 0,
                mean_absolute_pct=st.mean(absolute)*100 if absolute else 0,
                p95_absolute_pct=absolute[max(0, int(len(absolute)*.95)-1)]*100 if absolute else 0,
                max_over_pct=max([0]+errors)*100, max_under_pct=-min([0]+errors)*100,
                under_10pct=sum(e < -.1 for e in errors), over_10pct=sum(e > .1 for e in errors),
                false_positive=sum(not int(r["exact_success"]) and int(r["estimate_success"]) for r in data),
                false_negative=sum(int(r["exact_success"]) and not int(r["estimate_success"]) for r in data),
                mean_us=st.mean(float(r["estimate_us"]) for r in data),
                path_queries=sum(int(r["path_queries"]) for r in data))

report=[]
for (strength, distance), data in groups.items():
    result=dict(strength=strength, range_m=distance/100, **metrics(data))
    result["groups"]={name: metrics([r for r in data if r["group"]==name]) for name in sorted({r["group"] for r in data})}
    result["distance_buckets"]={name: metrics([r for r in data if lo <= float(r["exact_cm"]) < hi])
                               for name,lo,hi in [("under5m",1,500),("5to20m",500,2000),("over20m",2000,float("inf"))]}
    prior=data[0]
    result["prior_case"]={"exact_m":float(prior["exact_cm"])/100,"estimate_m":float(prior["estimate_cm"])/100}
    report.append(result)
(root/"Saved/PathLinkBench/Blend200Summary.json").write_text(json.dumps(report,indent=2),encoding="utf-8")
for r in report:
    print(f"S={r['strength']:.2f} R={r['range_m']:g}m n={r['compared']} MAE={r['mean_absolute_pct']:.2f}% "
          f"P95={r['p95_absolute_pct']:.2f}% over={r['max_over_pct']:.2f}% under={r['max_under_pct']:.2f}% "
          f"under10={r['under_10pct']} prior={r['prior_case']['estimate_m']:.3f}m us={r['mean_us']:.2f}")
