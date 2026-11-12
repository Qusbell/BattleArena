"""Applies the collection-version-1 extension (extension_v1.sql + extension_views_v1.sql) to the committed template
database, rebuilds v_event_details / v_record_issues so the new tables are checked, then regenerates the C++ schema.
Run from the repository root:  python Tools/ArenaMatchLog/apply_extension.py
It refuses to run twice (the template already has ammo_change)."""
import re, sqlite3, subprocess, sys

DB = 'Content/Data/ShootingArenaLog.db'
con = sqlite3.connect(DB)
if con.execute("select 1 from sqlite_master where name='ammo_change'").fetchone():
    sys.exit('template already extended')

con.executescript(open('Tools/ArenaMatchLog/extension_v1.sql', encoding='utf-8').read())

# New event kinds join the one-detail-per-event check.
details = con.execute("select sql from sqlite_master where name='v_event_details'").fetchone()[0]
extra = (" UNION ALL SELECT MatchId,EventSequence,'AmmoChanged' FROM ammo_change"
         " UNION ALL SELECT MatchId,EventSequence,'ExposureChanged' FROM exposure_change"
         " UNION ALL SELECT MatchId,EventSequence,'AttackInput' FROM attack_input"
         " UNION ALL SELECT MatchId,EventSequence,'FireAvailabilityChanged' FROM fire_availability"
         " UNION ALL SELECT MatchId,EventSequence,'JumpObserved' FROM jump_observation")
con.execute('drop view v_event_details')
con.execute(details.rstrip() + extra)

views = open('Tools/ArenaMatchLog/extension_views_v1.sql', encoding='utf-8').read()
for part in re.split(r'^-- @@\s*$', views, flags=re.M):
    part = part.strip()
    if part:
        con.executescript(part if part.endswith(';') else part + ';')

issues = con.execute("select sql from sqlite_master where name='v_record_issues'").fetchone()[0].rstrip()
con.execute('drop view v_record_issues')
con.execute(issues + "\nUNION ALL\nSELECT MatchId,Code,Severity,EventSequence,Detail FROM v_extension_issues")

con.execute('pragma user_version=2')
con.commit()
con.close()
subprocess.check_call([sys.executable, 'Tools/ArenaMatchLog/GenerateSchema.py'])
