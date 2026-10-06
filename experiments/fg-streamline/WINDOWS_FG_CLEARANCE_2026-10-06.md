# Windows candidate verification: FG and DLSS foliage

This is a test candidate for PR10, not a published release. Check the supplied
candidate manifest and payload hashes before installing. Use the Steam build
identified in `dependencies.lock.json`; Xbox / Microsoft Store support is not
established by this candidate.

## Install and recover

1. Close the game. Start from the working mod and its pinned dependencies.
2. Extract the candidate. Acquire the separately required Streamline 2.14.1
   production runtime and matching ReShade PR435 binary under their terms.
   `hydrate_runtime.py` verifies the runtime hashes; vendor DLLs are not bundled.
3. On supported NVIDIA hardware, run the isolated owned-input probe described
   in README before the game trial. Then run the reversible `game_trial.py install` command in README, passing the
   candidate’s `probe`, `latency`, `sr` and `ui/Pak` folders. Use a new backup.
4. Set `FGGuideCapture.ini` as shown in README, including `NativeUIToggle=1`.
5. After testing, close the game and run `game_trial.py restore` with that backup.
   Confirm original binaries and UI are restored. Do not remove player saves.

## Required checks

| Environment | Checks |
| --- | --- |
| Windows / AMD or Intel | Startup, menus, gameplay and normal exit; DLSS fallback at native resolution; FG unavailable; no foliage override while Native/fallback is active. |
| Windows / supported NVIDIA | Quality and DLAA, Native switching, active foliage motion correction with FG Off and HDR Off; FG On with HDR; menus, level travel, resolution changes and normal exit. |
| FG lifecycle | Start Off; request On and restart. Enter gameplay; visit menus and return twice. Switch Off, resume, then request On without restarting: it must remain Off and show restart required. Restart and verify On resumes. |
| Performance | Same scene, output resolution, SR preset and settings: released baseline, candidate fresh-start FG Off, candidate fresh-start FG On. Warm up; collect at least three samples each. Record rendered FPS and total presentation FPS separately. |
| Quality | Stationary and moving foliage, disocclusion, particles and HUD; inspect generated and rendered frames independently where possible. |

The new foliage correction is scoped to current acknowledged DLSS rendering.
It preserves a pre-existing enabled setting, restores owned changes on Native or
fallback, and stops overriding a later console/mod change. No persistent engine
configuration or forced-velocity-output setting is modified.

Record OS, GPU/driver, game build, candidate archive/payload hashes and results.
Report crashes/hangs, incorrect fallback, visual corruption or material FG-Off
cost before accepting the candidate. SDK presentation counts alone do not prove
physical monitor cadence. Windows/AMD results cannot qualify NVIDIA FG.

## v27 follow-up

Use the v27 candidate, not the older v23 archive. Its SR addon also handles
replacement command recordings which never bind a pipeline and cancelled
pending evaluations. GPU references still require proven fence completion.

Repeat Quality → DLAA → Quality → DLAA while FG stays On. Confirm the active
source dimensions and generated-frame counts after every return to gameplay.
Start a separate process directly in DLAA: the Linux fresh-start case costs
more than returning to DLAA from Quality. Record these cases separately. See
[the Linux comparison](LINUX_FG_PERFORMANCE_2026-10-06.md).

The separate guided-installer layout candidate deploys the existing rc.1 payload,
not FG. Extract its entire folder, then check fresh installation, repair,
modified-file refusal, uninstall/dependency retention and running-game refusal.
Its Windows compilation is not a Windows execution result. The candidate is
unsigned and is not a release; signing/final-scan/Nexus gates remain outstanding.
