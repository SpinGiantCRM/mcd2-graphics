# AMD FG HDR benchmark

Development candidate `0.4.0-amd.dev.7`, using the complete PR54 native archive,
its exact separately selected framework and the original HDR addon. The private
shutdown-boundary guide was not installed. This is Linux/NVIDIA evidence;
Windows/Radeon and active Anti-Lag 2 remain unqualified.

## Method and results

FSR analytical 3.1.5 Quality: 2560×1440 source, 3840×2160 PQ/BT.2020 output.
VSync Off, frame cap None, Reflex On. The character remained stationary in the
hub; lighting, wind and NPCs continued to change. Startup warmed for 45 seconds
after route acknowledgement; menu changes settled for 15 seconds. Three captures
per phase each contained 1,199 application Present intervals. The same CPU
sampler was installed throughout the primary comparison.

The [numeric receipt](../qualification/providers/amd-pr54-hdr-benchmark-2026-10-13.json)
retains all fifteen captures and the thresholds registered before measurement.
Medians:

| Phase | Application presents/s | SDK real + generated/s | p95 (ms) | p99 (ms) |
| --- | ---: | ---: | ---: | ---: |
| Fresh Off startup | 137.84 | — | 8.27 | 8.78 |
| On A | 92.64 | 185.23 | 11.77 | 12.57 |
| Off A, presenter retained | 130.03 | — | 8.63 | 9.19 |
| On B | 92.37 | 184.73 | 11.85 | 12.67 |
| Off B, presenter retained | 131.96 | — | 8.56 | 9.06 |

On reduced rendered throughput by about 32.8–33.0%; median SDK-counted total
throughput rose 34.0–34.4%. Retained Off recovered to 94.3–95.7% of fresh Off.
Median memory increases were 714–862 MiB. The registered median throughput,
tail-time, memory, power and temperature gates passed. These are sequential
scene observations, not a frozen replay or a provider ranking. SDK totals do
not measure physical display FPS; GPU pass timings and input latency were not
measured.

## Partial generation is retained

One On A capture recorded 1,199 real presents and only 649 generated presents.
Its application rate was 99.51/s and SDK total 153.37/s. The guide log contains
intermediate inactive outcomes with no SDK faults/errors/warnings; generation
then resumed. The capture remains in the comparison. Passing median gates does
not make every interval equivalent to uninterrupted 2× generation.

The sampler observes requested activation at capture boundaries. It does not
prove continuous activation between them. The analyzer now reports generated,
prepared and image counts per real present, and explicitly identifies this
boundary-only activation scope. A regression fixture covers partial generation.
Existing rate calculations and the recorded measurements are unchanged.

A separate follow-up added foreground ownership and own FG-slot readiness/age
observations. Its three captures recorded one generated frame per real present
with valid foreground/readiness observations. It did not reproduce the earlier
pause. Those additional observers were not part of the primary benchmark, and
neither foreground loss nor a UI/readiness defect is established as the cause.

## Shutdown and remaining gates

Both held-process normal quits returned actual exit code 0: 3.35 seconds for
fresh Off and 4.07 seconds after the active-owner session. Scene and shared SDK
retirement returned 0. Original files, own mod settings, renderer configuration
and calibration were restored byte for byte; the temporary sampler was removed.
Player/account saves were not copied or edited. No raw traces or screenshots are
attached.

The earlier SDR shutdown failure remains recorded. Its cause, the HDR generation
pause, moving-image quality, physical pacing, current-artifact NVIDIA regressions
and native Windows/Radeon qualification remain open. Published releases are
unchanged.
