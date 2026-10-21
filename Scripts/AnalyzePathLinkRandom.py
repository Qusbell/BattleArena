"""Aggregate native random-route benchmark CSVs. No Unreal dependencies."""
import csv
import argparse
import json
import math
import statistics as st
from pathlib import Path

root = Path(__file__).resolve().parents[1]

def metrics(rows):
    valid = [r for r in rows if r["exact_success"] == "1" and r["estimate_success"] == "1" and float(r["exact_cm"]) > 1]
    signed = [(float(r["estimate_cm"])-float(r["exact_cm"])) / float(r["exact_cm"]) for r in valid]
    absolute = sorted(abs(e)*100 for e in signed)
    return dict(pairs=len(rows), exact_success=sum(r["exact_success"]=="1" for r in rows), compared=len(valid),
                estimate_success=sum(r["estimate_success"]=="1" for r in rows),
                false_positive=sum(r["exact_success"]=="0" and r["estimate_success"]=="1" for r in rows),
                false_negative=sum(r["exact_success"]=="1" and r["estimate_success"]=="0" for r in rows),
                mean_absolute_m=st.mean(abs(float(r["estimate_cm"])-float(r["exact_cm"]))/100 for r in valid) if valid else None,
                mean_absolute_pct=st.mean(absolute) if absolute else None,
                median_absolute_pct=st.median(absolute) if absolute else None,
                p95_absolute_pct=absolute[math.ceil(.95*len(absolute))-1] if absolute else None,
                max_over_pct=max([0]+signed)*100, max_under_pct=-min([0]+signed)*100,
                over10=sum(e>.1 for e in signed), under10=sum(e<-.1 for e in signed),
                mean_exact_m=st.mean(float(r["exact_cm"])/100 for r in valid) if valid else None,
                mean_estimate_us=st.mean(float(r["estimate_us"]) for r in rows),
                max_estimate_us=max(float(r["estimate_us"]) for r in rows),
                mean_detail_us=st.mean(float(r["exact_us"]) for r in rows),
                path_queries=sum(int(r["path_queries"]) for r in rows),
                paths_using_links=sum(r["exact_success"]=="1" and int(r["used_links"])>0 for r in rows))

parser=argparse.ArgumentParser()
parser.add_argument("--maps",nargs="+",default=["Lvl_PortalMap","Lvl_02Map"])
parser.add_argument("--kind",choices=["Random","Playable"],default="Random")
args=parser.parse_args()
selected=set(args.maps)
report={}
for map_name, seed in [("Lvl_PortalMap",20261022),("Lvl_02Map",20261023)]:
    if map_name not in selected: continue
    if args.kind=="Playable": seed=20261024
    path=root/f"Saved/PathLinkBench/{map_name}_{args.kind}Routes200_Seed{seed}.csv"
    if not path.exists(): continue
    data=list(csv.DictReader(path.open(encoding="utf-8-sig")))
    result={"file":str(path),"seed":seed}
    for name, strength in [("baseline",0),("current",1)]:
        rows=[r for r in data if float(r["strength"])==strength]
        assert len(rows)==200, (map_name,name,len(rows))
        m=metrics(rows)
        m["groups"]={g:metrics([r for r in rows if r["group"]==g]) for g in sorted({r["group"] for r in rows})}
        m["distance_buckets"]={g:metrics([r for r in rows if lo<=float(r["exact_cm"])<hi])
                               for g,lo,hi in [("under5m",1,500),("5to20m",500,2000),("over20m",2000,float("inf"))]
                               if any(lo<=float(r["exact_cm"])<hi for r in rows)}
        m["worst_samples"]=sorted([r for r in rows if r["exact_success"]=="1" and r["estimate_success"]=="1" and float(r["exact_cm"])>1],
                                  key=lambda r:abs(float(r["estimate_cm"])-float(r["exact_cm"]))/float(r["exact_cm"]),reverse=True)[:5]
        result[name]=m
    report[map_name]=result
(root/"Saved/PathLinkBench"/("PlayableSummary.json" if args.kind=="Playable" else "Random400Summary.json")).write_text(json.dumps(report,indent=2),encoding="utf-8")
for name,m in report.items():
    print(name)
    for setting in ("baseline","current"):
        print(setting,json.dumps({k:v for k,v in m[setting].items() if k not in ("groups","worst_samples","distance_buckets")}))
    print("groups",json.dumps(m["current"]["groups"]))
