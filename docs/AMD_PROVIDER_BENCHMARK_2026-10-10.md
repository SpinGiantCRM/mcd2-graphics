# AMD Frame Generation pairing measurements

Development evidence for AMD FG with NVIDIA DLSS and AMD FSR, using the same
source-matched candidate and ordinary Video controls. Published payloads are
unchanged. The numeric receipt is
[amd-provider-benchmark-2026-10-10.json](../qualification/providers/amd-provider-benchmark-2026-10-10.json).

## Method

CachyOS, RTX 4080 SUPER, driver 615.78.08, CachyOS Proton SLR 11.0-20261005,
Steam build 25754144 / game 1.1.2.0. Output is 3840×2160 native WineWayland HDR;
Quality source is 2560×1440 for both upscalers. VSync is Off, FPS Limit None,
and Reflex On. Anti-Lag is unavailable on this Linux/NVIDIA machine.
The character remains stationary at the hub fountain. Lighting, NPCs and
foliage continue animating; these are sequential observations, not a frozen replay.

The temporary [CPU sampler](../experiments/providers/present_sampler.cpp) records
1,200 QPC timestamps from ReShade `finish_present` per capture: 1,199 intervals.
It queries AMD SDK counters only at the boundaries and writes after capture.
It submits no GPU commands, GPU queries, copies or pixel readbacks. The same
sampler remains installed for Off and On. Low-rate GPU clock/utilization/power
observations are kept locally. No other benchmark input is sent during a batch.

Before and after each admitted capture, the current gameplay context, committed
provider revision, source scale, render/output dimensions and FG mode agree.
NVIDIA requires its matching Active acknowledgement; FSR requires phase 5,
successful evaluations and zero runtime error. Menus and loading are excluded.

The [analyzer](../experiments/providers/analyze_present_samples.py) separates
application present intervals from real/generated SDK counts. Active captures
require both boundary states to be ready, fault-free and advancing, with SDK
real presents matching application intervals. Off requires no generated-count
advance. A loaded bridge with no owned presenter has no usable SDK FPS counter.
The sampler has no swapchain filter; this single-window test's active SDK count
match is required. Do not generalize it to multiple swapchains.

## Quality results

| Pairing / state | Median application presents/s | Median SDK real + generated/s |
| --- | ---: | ---: |
| dlss-quality-steady-clean-off | 124.85 | — |
| dlss-quality-active-amd-on | 86.68 | 173.35 |
| fsr-quality-steady-clean-off | 120.64 | — |
| fsr-quality-active-amd-on | 82.25 | 164.49 |
| fsr-quality-amd-retained-off | 125.3 | — |
| fsr-quality-repeat-amd-on | 89.49 | 178.98 |

## Interpretation and remaining checks

AMD FG produced one generated present per real present with both Quality
upscalers. Switching NVIDIA SR → FSR through Video preserved AMD FG and current
1440p → 4K guides. The native temporal shader remains bypassed only after
successful SR output replacement. SDK errors, warnings and faults stayed zero
at measured boundaries.

The repeat FSR run's median was 89.49 application presents/s and 178.98 SDK
real-plus-generated/s, versus 120.64 application presents/s after a clean Off
startup. That is a 25.82% reduction in real rendering rate and a 48.36% increase
in SDK count-equivalent total. DLSS's corresponding medians were 86.68 / 173.35
versus 124.85: a 30.58% real-rate reduction and 38.85% SDK total increase.
These are bounded scene-specific observations, not provider-wide benefit claims.

FSR → Native through Video retired the active FSR generation, reached phase 8
with error 0 and zero FSR evaluations, and confirmed actual 100% source scale.
AMD FG resumed from independently produced 3840×2160 guides without recreating
the presenter. Its counters advanced with no SDK errors/warnings. The transient
restoration phase was excluded from measurement.

One subsequent full-resolution Native check observed 63.91 application and
127.82 SDK real-plus-generated presents/s, with 1,199 real and 1,199 generated
presents. Turning FG Off observed 94.72 application presents/s and zero generated
count advance. These single captures check independent activation/deactivation;
they are not a repeatable Native performance comparison or a clean-start Off
baseline.

Clean-start Off and Off with a retained AMD presenter are separate cases. Turning
FG Off does not recreate the swapchain; a restart removes that owner. These
measurements do not isolate the retained-presenter cost from changing scene and
host load, nor compare candidate Off against a matched released-build control.

FSR's first active batch varied appreciably. Do not rank FSR against DLSS from
these observations. Repeat controlled runs and moving scenes before performance
or artifact claims. Generated presents are SDK accounting, **not measured physical
scanout FPS**, unique visible images or latency. Present intervals are not GPU
execution times. Frame Generation lowers the real rendering rate here while
increasing the SDK real-plus-generated rate; it does not double simulation FPS.

Two later FSR Quality sessions closed through normal Save and quit without a
forced signal. Process disappearance was observed; exit codes were not captured.
This does not erase earlier shutdown failures or qualify FSR producer retirement
on queue destruction. Native Windows/Radeon, active Anti-Lag, broader lifecycle,
visual quality and an ordinary candidate installer remain separate gates.
Raw captures/logs, account data, player saves and vendor binaries are excluded.
