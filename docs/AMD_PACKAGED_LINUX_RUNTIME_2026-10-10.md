# Packaged AMD candidate: Linux runtime and presentation counts

**Development evidence only. Normal quit failed after the mixed-provider sequence.**
This candidate is not runtime-qualified for release or native Windows/Radeon use.

The [PR47 installer artifact](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/38011982509/artifacts/11653422430)
was verified against its archive and both complete folder receipts. Its Windows
payload equals the pinned PR45 native artifact. Those exact 16 own files, the
matching separately sourced ReShade framework and FSR SDK runtimes were installed
reversibly under Proton. No substitute Linux native binaries were used.
The [numeric receipt](../qualification/providers/amd-packaged-linux-runtime-2026-10-10.json)
records hashes, admitted captures, counter deltas, source dimensions and exits.

## Short stationary scene

CachyOS/KDE Wayland, RTX 4080 SUPER, driver 615.78.08, CachyOS Proton SLR
11.0-20261005, Steam build 25754144 / game 1.1.2.0. Output was 3840×2160;
FSR Quality source was 2560×1440. Native HDR was requested with 420-nit peak
and 203-nit paper/UI white. Character/camera stayed still; lighting and NPCs
continued to animate. This is not a matched immutable frame or a Radeon result.

A CPU-only diagnostic addon recorded 1,200 finish-present callbacks per capture,
without GPU commands, queries or pixel readbacks. Three consecutive admitted
captures per Quality condition used matching current-world/source acknowledgements.
All active FG capture boundaries had zero SDK faults, errors and warnings, with
real/generated deltas matching the 1,199 application intervals. Eight analyzer
checks passed. One Off capture overlapped a screenshot and was excluded; its raw
record remains private. No screenshots or input occurred during admitted captures.

| FSR Quality state | Application presents/s | Generated SDK presents/s | Combined SDK count/s | Mean per-run p95 / p99 (ms) |
| --- | ---: | ---: | ---: | ---: |
| Clean FG Off; Native startup owner | 133.06 | 0 | — | 8.51 / 9.16 |
| AMD FG On | 90.03 | 90.03 | 180.07 | 12.24 / 12.86 |
| FG Off after On; AMD presenter retained | 127.29 | 0 | — | 8.87 / 9.34 |

Values are arithmetic means of three per-run statistics, not pooled percentiles.
FG On reduced the base rate by 32.33% and increased the combined SDK count by
35.33% relative to this candidate's clean Off launch. Turning Off stopped
generation immediately but retained about 4.33% base-rate overhead in this scene;
the menu's restart-required state was asserted. Clean Off did not own an AMD
swapchain, so its SDK real-present rate is unavailable, not zero FPS.
These are application intervals and SDK counts, **not physical scanout, unique
visible frames or latency**. There is no comparison against the published payload
and no claim about performance without the diagnostic sampler.

Single additional captures established successful generation with FSR Native AA
(62.20 base / 124.40 combined), DLSS Quality (87.36 / 174.73), and Native rendering
(67.39 / 134.79). These are transition checks, not comparative quality benchmarks.
Menu pause kept the generated count unchanged across about 22 seconds. Native
Radeon Anti-Lag was unavailable and hidden on this NVIDIA host; Reflex remained
available. This does not qualify Anti-Lag or measure a latency benefit.

## Shutdown result and next gate

The clean FSR Quality / FG Off launch quit normally: actual game exit code 0,
3.40 seconds from starting the confirmation helper to process disappearance.
After Quality FG On → retained Off → Native AA FG On → DLSS Quality FG On →
Native FG On, normal quit returned **0xc0000409**, despite window/process closure
in 2.66 seconds. The watcher helper itself succeeded; its success must not be
mistaken for the game's exit code.

The SR unload receipt reported retained ownership and successful module pinning,
not completed GPU retirement. AMD retirement and shared SDK shutdown each reported
result 0. Those results did not prevent the fatal process exit. The root cause is
not established. Microsoft documents [0xc0000409 as a fast-fail exception](https://learn.microsoft.com/en-us/cpp/intrinsics/fastfail);
the code alone does not establish a stack overwrite or identify the failing module.

Three subsequent controls exited with actual code 0: menu-only (5.58 s),
fresh FSR Quality + AMD FG without provider/preset switching (6.27 s), and
fresh DLSS Quality + AMD FG followed by Native 100% + AMD FG (4.65 s).
The shorter DLSS-to-Native transition alone did not reproduce the failure.
These controls do not establish its cause or qualify the longer mixed sequence.
A separate debugger-attachment trial altered execution and ended before normal
quit; it was excluded and supplied no fast-fail exception evidence.

A smaller FSR-only sequence also reproduced the fatal exit twice: Quality +
AMD FG → FG Off → FG On → FSR Native AA → normal quit. Neither launch selected
DLSS. One lost FG activation after Native AA and recorded NVIDIA configuration
result 31 despite AMD startup ownership; the repeat kept AMD FG active and still
failed quit. Thus the intermittent activation fault is not required for the exit
failure. The repeat included an extra bounded CPU module inspector, whose samples
ended before the Native AA switch; it did not establish the routing cause.

A further isolation launch pinned the already-loaded FSR upscaler SDK DLL
until process exit, without changing SDK context retirement. The pin succeeded;
Native AA and AMD FG both remained active, but normal quit still returned
0xc0000409 (2.67 s). Pinning that DLL alone is insufficient and was not adopted
as a shipping change. This does not establish the failing module.

The clean-Off control changed FSR Quality → Native AA with Native presentation
ownership and no FG activation. It exited with actual code 0 (5.17 s). The FSR
preset change alone therefore did not reproduce this failure; the AMD presenter
or its interaction with the transition remains a target, not an established cause.

The original mod files, mod preferences and renderer settings were restored byte
for byte after the measurement sequence. Account/character saves were never
copied or edited. Raw logs, screenshots and account information remain private.

Fix and recheck the shutdown failure before admitting this artifact. Actual
Windows/Radeon FSR, FG and Anti-Lag; Windows installer UI; moving image quality;
travel, resize and long-session checks; old NVIDIA equivalence; and release
metadata alignment remain required. Published downloads and tags are unchanged.
