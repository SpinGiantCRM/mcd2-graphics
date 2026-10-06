# Exact Windows-artifact Linux benchmark — 7 October 2026

Historical v31 qualification record. The current held candidate uses the reset-epoch
framework patch and the Reflex pacing changes documented in
[the later result](REFLEX_PACING_2026-10-07.md). These numbers do not qualify it.

RTX 4080 SUPER, driver 615.71.09; 3840 × 2160 native HDR, CachyOS Proton SLR / WineWayland. Windows artifact from [run 37454118859](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37454118859), including its exact PR435 framework. The original 0.2.0-rc.1 payload is the baseline.

| Preset | Baseline | FG Off | FG On rendered | FG On SDK presentations |
| --- | ---: | ---: | ---: | ---: |
| DLAA | 86.6 | 86.3 | 66.4 | 132.9 |
| Quality | 128.7 | 129.5 | 88.9 | 177.7 |

Medians of three consecutive 1,200-callback samples per condition. The camera and character stayed still; lighting and NPCs continued to animate. Samples require matching current-process SR/source acknowledgements. Accepted FG On samples had no internal NGX evaluation failures and uninterrupted successful cached SDK outcomes. One transient sampler-sequence failure was rejected and retained locally.

FG Off differed from baseline by −0.34% in DLAA and +0.58% in Quality. FG On reduced rendered FPS by 23.0% and 31.4%; SDK presentation rates rose by 53.9% and 37.3% versus FG Off. These counters do not establish physical panel scanout, latency or image quality. No GPU readbacks or GPU timing queries were added. [Numeric record](LINUX_WINDOWS_BINARY_BENCHMARK_2026-10-07.json).

These are historical v31 measurements, not qualification of the subsequent reset-confirmation change. One render-thread hang and a reproducible pending preset request prevent treating the measured artifact as release-qualified. No public release was replaced.
