# Arena Analyzer v2

## Identity and firing units

- The recorder exists only in authority Game/PIE worlds, never in client or editor worlds.
- `SessionId` is a GUID. `ShotId` is monotonically allocated within that session, including misses.
- `ControllerId` identifies a controller object in that session. Reconnection creates a new ID; optional `PlayerId` is descriptive metadata, not an identity key.
- `TrackId` is local to a controller and advances whenever a new Pawn is possessed. Pair `(ControllerId, TrackId)` identifies a firing life. Possession changes intentionally start a new track.
- Each accepted WeaponBase server `StartFire` invocation is one shot: one pistol/machinegun bullet, one complete shotgun pellet group, one penetrating rail shot, or one plasma/rocket projectile. Rail beam effects do not create shots. The six current weapons do not implement a continuous damaging beam.

## JSON schema

Existing map, interval, duration, movement and damage fields remain. Missing schema version means v1; new recordings explicitly write v2. Missing new arrays/fields receive fresh defaults; loading uses a temporary session so previous contents cannot leak into legacy loads.

- `Participants`: ControllerId, human `bPlayer`, display name, optional network PlayerId.
- `ShotSamples`: ShotId, time, ControllerId, TrackId, WeaponId (data asset path, class path fallback), location, normalized direction.
- `HitSamples`: time, ShotId, target ControllerId/TrackId/name, contact location/normal, `bDamageContact`. Trace/impact contacts and damage overlap contacts are distinguishable; do not count both as unique bullets.
- `DamageSamples`: existing fields plus ShotId, immutable WeaponId, explicit instigator-location availability. Damage is the value returned by Unreal ApplyDamage, not post-armor health loss.
- `DeathSamples`: confirmed death time, victim/killer IDs and tracks, victim/killer positions and availability, direct Cause, ShotId/WeaponId, separately CreditedControllerId/CreditSource.
- Unknown IDs are -1. Unavailable locations are accompanied by false availability flags and must not be treated as world origin measurements.
- Environmental/unknown deaths never inherit the last weapon's ShotId. An explicitly associated self-inflicted shot may retain its actual ShotId. Rule-based recent-attacker credit never replaces the direct cause.

## Event hooks

MCP invokes the narrow editor migration library; ordinary file tools do not inspect or modify Blueprint assets.

1. WeaponBase: BeginWeaponShot immediately before authoritative StartFire, after ammo consumption/approval.
2. Pistol, machinegun, shotgun and rail StartFire: trace result hook, including trace metadata on misses.
3. Plasma/rocket pool activation: copies the firing context into the projectile and overwrites it on every reuse; pool return clears it.
4. Plasma/rocket impacts and explosion/direct TakeDamage calls: record impacts and push/pop the projectile's immutable context around synchronous weapon damage dispatch.
5. WeaponBase ApplyDamage: native wrapper snapshots identity before health/death callbacks, preserves the original controller, records applied damage, and suppresses the recorder's duplicate AnyDamage callback for that victim.
6. BPC_Life OnRep_LifeInfo: records only at the actual OnDeath dispatch, before downstream callbacks can unpossess the victim. Client callbacks have no recording subsystem. A victim is recorded once.
7. RecentAttackerComponent ResolveDeathCredit: copies the actual game-rule result into separate credit fields, whether it runs before or after the death hook.

## Query and visualization

`FilterSession` filters combat by exact WeaponId, human/AI kind, controller/track and time. Hits join shots independently of the selected time range, so delayed hits can refer to earlier shots. Movement has no weapon identity and is not weapon-filtered.

EUW_ArenaAnalyzer now has one unified control panel inside a vertical ScrollBox with a visible scrollbar. File selection, Refresh files, Load, actor/weapon/event filters, Draw and Clear are all owned by the native panel; the old duplicate widgets and their EventGraph nodes were removed. Combo buttons and menus use explicit light text, dark backgrounds and distinct hover/selection colors. The added controls use 13px text and compact rows.

The panel populates its file list on initialization and preserves the current selection when refreshing. Load populates weapon options; Draw also loads a changed file automatically. Draw supports shots, contacts, deaths, movement and damage. It resolves the **same BP_ArenaAnalyzeVisualizer actor through the existing Get Visualizer function** and uses its sphere/cube meshes and material templates. Movement and damage reuse the original named ISM components. Extra event layers are transient ISM components on that actor. Player and AI use the same layer/material for each event category. Movement points and links share the white Color parameter of M_ArenaAnalyze_AIMovement. Actor type is distinguished by the filter, not by color. Link transforms match Set Link ISM: midpoint, X-axis look rotation, length/100, Y/Z scale .02.

Each Draw replaces the actor's previous instances and calls AddInstances once per layer, with collision/navigation disabled. It does not emit persistent Debug Draw primitives. Clear combat invokes the utility's original Clear function, so the same actor and all its layers are removed by either clear route. The status reports CPU draw time and instance count; it is not a viewport FPS measurement. The former Debug Draw implementation was removed after user feedback about draw stalls and poor visibility.

## Storage and cost

Movement sampling is checked in live PIE (including the real loading/game-flow subsystems). The native identity fixture does not start unrelated game-flow subsystems.

The 0.5-second recorder timer discovers controllers idempotently, samples movement and checks a 50,000-total-event buffer threshold. On successful save it releases event buffers and increments ChunkIndex; on failure it retains data and logs an error. Files are independently valid JSON named `ArenaAnalyze_<SessionId>_<ChunkIndex>.json`. Loading any chunk joins all adjacent chunks of that session in order and rejects a missing/corrupt intermediate chunk. Keep all chunks together when copying recordings.

Shot lookup on active weapons/projectiles retains only their current firing context, not a second full session history. JSON serialization temporarily copies the current buffer. Offline visualization intentionally loads the complete selected session, so viewer memory still scales with session length. File I/O is synchronous on the sampling timer; large sessions should be profiled before increasing the threshold.

## Validation

See `Saved/Automation/ArenaAnalyzer` and the build/test logs under `Saved/Logs`. Native tests exercise human/AI identity and registration, duplicate registration, misses, original weapon and firing-life retention across respawn/refire, multiple damage records per shot, projectile context reset, confirmed-death deduplication, unknown-death attribution, legacy JSON, multi-chunk loading, weapon/actor/time filters and delayed-hit joins. Blueprint integration and live PIE results are reported separately from these native fixtures.


## Verified results (2026-10-28)

- Development Editor build succeeded. Live Coding was not used.
- `ShootingArena.Analyzer.Identity` and `ShootingArena.Analyzer.JsonAndFilters`: both Success, zero test warnings/errors. Report: `Saved/Automation/ArenaAnalyzer/index.json`.
- All nine modified Blueprint assets compiled UpToDate through MCP. The existing BP_ArenaAnalyzeVisualizer also compiled UpToDate and required no asset changes.
- Dedicated-server PIE world had one recorder; its client PIE world had none. A recorded session contained 3,789 movement samples, 416 unique shots and 8 confirmed deaths. Player and six AI participants were identified separately.
- Following the explicit-firing-weapon correction, a fresh PIE snapshot had 578 damage records and all 578 referred to a ShotId. The correction matters because the original weapon Blueprint uses the shooter Pawn as DamageCauser.
- The normal inventory/pickup path was used to prepare all six weapons. One server fire per weapon created exactly one shot with its data asset identity. Hitscan central aim and projectile muzzle transforms were nonzero and matched their respective implementations. A shotgun invocation produced 16 damage contacts under one ShotId.
- Naturally recorded multi-target examples: rocket shots 425 and 544 each damaged two tracked targets; shotgun shot 1167 damaged three tracked targets. These are from verification session `6D9CA0DE4C3CBABD68EBBF800209FE17`.
- A controlled delayed-damage callback fixture used an actual spawned rocket, fired a different weapon, confirmed the original shooter's death, then delivered two authoritative damage callbacks with the projectile context. Both damage records and both resulting confirmed deaths retained ShotId 288, RocketLauncher data and firing TrackId 2. This checks the recording boundary; it is distinct from a complete natural-flight/physics replay after shooter death.
- Confirmed direct kill, suicide and environment death were exercised through the real health/death pipeline. Environment death kept ShotId -1 while separately storing the recent attacker as credited controller 6 / RecentAttacker.
- Legacy file `ArenaAnalyze_2026.09.18-22.33.32.json` and v2 files loaded and drew through the new panel. Player/weapon/death filters and clear were invoked successfully. Internal widget-tree and desired-size checks passed; Windows screenshot capture timed out, so screenshot-based visual QA was unavailable.
- The 50,000-event rollover design was reviewed, and multi-chunk loading was tested with two fixture chunks. A long-duration performance benchmark was not run. Offline viewer memory grows with the selected session.
- PIE was stopped and the editor was responding at completion.

## Changed sources and assets

Runtime source:

- `Source/ShootingArena/Public/Analyzer/ArenaAnalyzeSubsystem.h`
- `Source/ShootingArena/Private/Analyzer/ArenaAnalyzeSubsystem.cpp`
- `Source/ShootingArena/Public/Analyzer/ArenaAnalyzeLibrary.h`
- `Source/ShootingArena/Private/Analyzer/ArenaAnalyzeLibrary.cpp`
- New `ArenaCombatTypes.h`, `ArenaCombatLibrary.h/.cpp`, `ArenaAnalyzeControls.h/.cpp` under the matching Analyzer directories.
- `Source/ShootingArena/Private/Weapon/ProjectilePoolComponent.cpp`
- `Source/ShootingArena/Private/Combat/RecentAttackerComponent.cpp`
- `Source/ShootingArena/ShootingArena.Build.cs` (CoreOnline for network identity formatting).
- `Source/ShootingArena/Private/Tests/ArenaAnalyzerTests.cpp`

Editor source:

- `Source/ShootingArenaEditorTools/Private/ArenaAnalyzerMigrationLibrary.h/.cpp`: idempotent MCP-invoked graph authoring plus a PIE-only inventory weapon fixture.

Blueprint assets under `/Game/QuakeLike_1_0`:

- `EUW_ArenaAnalyzer`
- `HeldItem/Gun/BP_WeaponBase`
- `HeldItem/Gun/BP_Weapon_Pistol`, `BP_Weapon_MachineGun`, `BP_Weapon_ShotGun`, `BP_Weapon_RailGun`
- `HeldItem/Projectile/BP_PlasmaProjectile`, `BP_RocketLauncherProjectile`
- `Life/BPC_Life`

The original visualizer asset is unchanged. `Content/QuakeLike_Base/Lvl_PortalMap.umap` also appears in the working changes because dirty editor packages were saved before restarting; no intentional analyzer edits were made to that level, and its saved contents were preserved.


## Follow-up: UI and Draw correction

- The first combat renderer used separate persistent Debug Draw primitives and accumulated them on repeated draws. This was different from the original visualizer and increased sustained viewport work, particularly the per-contact wire spheres.
- It has been replaced by the original visualizer's ISM approach with batched uploads. This also avoids one Blueprint/AddInstance/render update per record.
- FilterSession no longer copies all event arrays before clearing them; it constructs only the selected result arrays.
- No weapon or projectile Blueprint assets are changed by this follow-up. Only the utility Blueprint receives the scroll wrapper and file-selector styling. The existing visualizer asset remains the template and is not rewritten.
- Weapon instrumentation from the original task remains: WeaponBase BeginWeaponShot / ApplyAnalyzedDamage; pistol/machinegun/shotgun RecordTrace; railgun RecordTraces; plasma/rocket projectile impact and damage-context hooks. PlasmaGun and RocketLauncher weapon Blueprints themselves were not changed. BPC_Life contains the confirmed-death hook.


Follow-up validation:

- Development Editor build succeeded without Live Coding. All three Analyzer automation tests passed, including InstancedRendering.
- The ISM fixture checks world-space transforms with an offset actor, the original link scale/midpoint, separate Player/AI layers, no cross-life segment, no unknown-killer origin line, event replacement and clear. 10,000 movement samples produced 19,999 instances in 6.10 ms of CPU fixture time.
- Actual recording `64B2D4B24BA8672E33B04D8B23309917` (1,133 shots, 2,731 contacts, 1,369 damage events, 31 deaths) produced 15,724 ISM instances. Consecutive Draw calls took 8.7 and 5.6 ms of CPU filter/render preparation time, with the same instance count on both calls. A shots-only draw produced exactly 2,266 instances and removed the movement instances; the original Clear removed the entire visualizer actor. A fresh All-events draw took 6.4 ms.
- Root ScrollBox overflow and scroll-to-end were verified with temporary runtime overflow content: maximum/current offset both 1934.26. The test spacer was removed. File selector foreground is explicit white, background linear RGB .035/.045/.065, font 13; menus also use explicit row and selected text colors.
- These are CPU timings, not an FPS/GPU benchmark. The live editor remains open with the selected real recording and the improved ISM visualization. The two temporary Analyze recordings made by native test worlds were removed; the user's real recording was preserved.
- Follow-up changed sources: ArenaAnalyzeControls.h/.cpp, ArenaAnalyzeLibrary.cpp, new ArenaAnalyzeRenderLibrary.h/.cpp, ArenaAnalyzerTests.cpp and the UI branch of ArenaAnalyzerMigrationLibrary.cpp. The only changed Blueprint asset in this follow-up is EUW_ArenaAnalyzer.


## Follow-up: unified controls and shared actor colors

The duplicate legacy UI has been removed, including its 32 obsolete EventGraph nodes. The original Get Visualizer and Clear backend functions are retained for the unified panel. A single high-contrast file selector and Refresh files/Load/Draw/Clear controls now cover the complete workflow, with the existing scroll wrapper retained.

Player/AI movement points and links share MovementISM/MovementLinkISM and a white movement material. Shots likewise share ShotISM/ShotLinkISM instead of actor-type color layers. Damage, hits and deaths already used shared event-role materials. The Players/AI filter remains unchanged. No recording schema or weapon Blueprint changes are involved.


Unified-control validation (2026-10-28): Development Editor build and all three Analyzer tests passed. EUW_ArenaAnalyzer compiled UpToDate. MCP verified that the utility asset now contains only AnalyzerScroll -> StackBox_0 -> CombatRecords and has zero legacy EventGraph nodes. The unified file selector populated automatically and preserved its selected recording on refresh. Load, Draw and Clear were exercised. Movement filtering returned 1,961 total points, split into 303 Player and 1,658 AI points; all modes used the same MovementISM/MovementLinkISM material, whose Color is (1,1,1,1). No extra Player movement layer was created. The editor was left responsive with the unified panel in Movement mode. Changed assets: EUW_ArenaAnalyzer and M_ArenaAnalyze_AIMovement; weapon Blueprint assets are unchanged.
