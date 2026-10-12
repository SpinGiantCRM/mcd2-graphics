# AMD FG repeated Off/On measurements

Bounded development evidence for FSR Quality with AMD analytical Frame Generation.
The production native files are the exact PR52 implementation at
`4d5a5ebd993c0e59bacd0833788128f7d0565c3e`, archive SHA-256
`71fc9e563a31de90878e35b66c23f841e39ea9ce9284a066a8aa1a6266639622`.
The [numeric receipt](../qualification/providers/amd-provider-benchmark-2026-10-12.json)
records all captures, dependency hashes, thresholds and exit controls. Published
releases and installer payloads are unchanged.

## Method

RTX 4080 SUPER, driver 615.78.08, CachyOS KDE/Wayland and CachyOS Proton SLR
11.0-20261005; Steam build 25754144 / game 1.1.2.0. Output is 3840×2160 native
Wayland HDR, source 2560×1440, FSR analytical SR 3.1.5 and FG 3.1.6. VSync is
Off, FPS Limit None and Reflex On. Anti-Lag 2 is unavailable on this host.

The character stayed at the hub fountain. Lighting, wind and NPCs remained
animated; this is a sequential scene comparison, not a frozen replay or a
provider ranking. Three captures per phase recorded 1,200 QPC timestamps each
(1,199 application-present intervals). Startup warmup was 45 seconds; menu
changes settled for 15 seconds. The same temporary sampler remained installed
through the complete comparison. Current SR output, source dimensions, gameplay
context and FG acknowledgement were checked before and after each capture.

The sampler submits no GPU work or pixel readbacks. SDK real/generated counters
are read at capture boundaries. Active counts must advance, match application
intervals and report no fault/error/warning. Off must not generate frames.
Low-rate GPU telemetry is a separate observation, not GPU pass timing.

## Results

Values are medians of three captures per phase. SDK totals are count-equivalent
rates, **not measured physical display FPS**.

| Phase | Rendered presents/s | SDK real + generated/s | p95 interval (ms) | p99 interval (ms) |
| --- | ---: | ---: | ---: | ---: |
| Fresh Off startup | 130.59 | — | 8.81 | 9.52 |
| On A | 89.60 | 179.20 | 12.43 | 12.93 |
| Off A, presenter retained | 124.91 | — | 9.13 | 9.65 |
| On B | 89.79 | 179.57 | 12.30 | 12.97 |
| Off B, presenter retained | 125.10 | — | 9.16 | 9.73 |

On reduced rendered throughput by 31.2–31.4% and increased SDK-counted total
throughput by 37.2–37.5% relative to fresh Off. Retained Off recovered to within
4.2–4.3% of fresh Off. Its p99 increased 1.4–2.2%; On p99 increased 35.8–36.2%.
Median GPU memory rose by 565–657 MiB while On and 689 MiB in retained Off.
Observed median power and temperature did not increase; changing scene and
thermal conditions prevent attributing that observation to an efficiency gain.

Thresholds were recorded before measurement: retained Off at least 90% of
baseline throughput and p99 no more than 1.15×; On at least 65% rendered
throughput, 1.20× SDK total and p99 no more than 1.65×. Memory, power and
temperature increases were limited to 2,048 MiB, 60 W and 8°C. All phases passed
these numerical gates. All individual captures, including the first baseline's
higher p99, remain in the receipt; no runs were trimmed.

## Lifecycle and diagnostic shutdown

Pause stopped generated counts across an eight-second observation while 6,420
real presents continued. Resume restored active FG. Returning to the main menu
stopped generation and restored 100% source scale; re-entering the hub created
a new world context and restored current 1440p FSR output and active AMD FG.
SDK errors/warnings remained zero at the observed checkpoints.

The original diagnostic sampler's fresh-Off quit returned `0xc0000409` despite
successful shared SDK cleanup. That failure and its three captures are retained,
but those captures are excluded from this comparison. Removing only the sampler
produced exit code 0 in the same Off → saved-On → quit control. This narrows the
investigation; one control does not prove the crash's cause.

The sampler now checks the existing NT termination guard before its callback
mutex and avoids ReShade unregister during final process teardown. Ordinary
unload still unregisters; NT-query initialization occurs outside DllMain.
Production renderer/provider binaries did not change. The guarded sampler's
fresh-Off quit returned 0 in 3.57 seconds. The full Off/On/menu-travel session
returned 0 in 3.36 seconds, with AMD retirement and shared SDK cleanup result 0.
These are bounded passes, not a universal shutdown repair. Original mod files,
mod preferences and renderer configuration were restored exactly, and the
temporary sampler was removed. Player/account saves were not copied or edited.

## Qualification scope

This remains **experimental**. Application intervals and SDK counts establish
neither physical scanout cadence nor input latency. GPU execution times,
classified moving real/generated images, HDR/UI image parity, wider lifecycle
and native Windows/Radeon active Anti-Lag 2 still require their own checks.
The [Windows candidate checklist](AMD_WINDOWS_CANDIDATE_2026-10-12.md) pins the
unchanged production files. Raw logs, screenshots and runtime binaries are not
attached.
