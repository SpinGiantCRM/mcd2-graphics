# dev.10 AMD FG performance check

Exact development artifact **0.4.0-amd.dev.10**, native run 38497329826 /
artifact 11863879034, with its verified source-built framework. No production
binary was substituted. A separate temporary CPU sampler was used throughout;
these are instrumented measurements, not a complete candidate qualification.
See the [numeric receipt](../qualification/providers/amd-dev10-benchmark-2026-10-14.json)
for hashes, all captures, registered thresholds, telemetry and exits.

## HDR method and results

Linux/Proton SLR, RTX 4080 SUPER, driver 615.78.08; native Wayland HDR.
FSR analytical 3.1.5 Quality rendered 2560×1440 into 3840×2160, with analytical
3.1.6 AMD FG. VSync Off, frame cap None, Reflex On. Anti-Lag 2 is unavailable
on this host. The stationary hub's lighting, wind and NPCs continued to vary.

Three captures per phase recorded 1,199 application Present intervals each.
Startup warmed for 45 seconds after acknowledgement; menu changes settled for
15 seconds. SR framing, checksums, revision, world, quality and resolution were
verified at both capture boundaries. SDK counters must match the application
interval count and advance without provider errors; Off must generate no frames.

| Phase | Application presents/s | SDK real + generated/s | p95 ms | p99 ms |
| --- | ---: | ---: | ---: | ---: |
| clean-off | 131.29 | — | 8.87 | 9.39 |
| on-a | 89.03 | 178.06 | 12.42 | 13.15 |
| retained-off-a | 125.77 | — | 9.19 | 9.65 |
| on-b | 89.34 | 178.68 | 12.42 | 12.88 |
| retained-off-b | 125.39 | — | 9.18 | 9.65 |

AMD FG On reduced rendered throughput by **31.95–32.19%**, while SDK-counted
real-plus-generated throughput increased by **35.62–36.09%**. After disabling
FG, throughput returned to **95.51–95.79%** of fresh Off. This is a measurable
residual cost, not identical Off performance. Median memory increases were
663–696 MiB; repeating the cycle changed the retained-Off median by 1 MiB.
That bounded observation is not a leak or long-session qualification.

All registered HDR median gates passed: retained Off ≥90% of fresh-Off rate
and ≤1.15× p99; On ≥65% rendered rate, ≥1.2× SDK total and ≤1.65× p99; memory
increase ≤2048 MiB, power increase ≤60 W and temperature increase ≤8°C.
Physical display pacing, input latency and GPU pass execution times were not
measured. SDK frame counts do not prove physical display FPS. No image-quality
or performance improvement over earlier binaries is inferred from a different
sequential scene.

## SDR attempt and shutdown

The SDR attempt exposed an output-state mismatch before FG-On measurement:
HDR Off was saved in the mod, but native HDR remained enabled. With RenoDX
removed, the display code reported HDRRevision 0 / restart required and refused
native output changes. AMD's output metadata remained PQ/HDR. The three fresh
Off samples are retained in the receipt but rejected as SDR evidence.

Three benchmark processes exited normally (0, about 3.46, 3.78 and 4.08 seconds).
The fourth diagnostic process, with AMD FG and the unexpected HDR output,
returned **0xc0000409** after an ordinary quit. AMD/shared SDK retirement returned
0 beforehand. The cause is not established; the sampler's presence does not
prove causation. Earlier failures remain recorded.

All four transactions restored the original mod files, mod-owned preferences,
renderer INIs and HDR addon. Raw logs, screenshots, account/player saves and
private paths are not published. The [HDR acknowledgement correction](HDR_OFF_ACKNOWLEDGEMENT_2026-10-14.md)
requires a new artifact and runtime check; these numbers do not qualify it.
Native Windows/Radeon, actual Anti-Lag activation and broader lifecycle checks
remain outstanding. Published releases are unchanged.
