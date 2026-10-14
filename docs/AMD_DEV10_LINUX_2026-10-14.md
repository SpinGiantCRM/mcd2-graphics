# dev.10 Linux checks

The complete **0.4.0-amd.dev.10** candidate includes the Anti-Lag shutdown,
FSR resource-ownership and shared gameplay-readiness corrections. All 44 native
artifact receipt members were verified, and all 17 own payload files were installed
with 15 verified external dependency files. No private addon substitution or
benchmark sampler was installed. See the [numeric receipt](../qualification/providers/amd-dev10-linux-2026-10-14.json).

## Results

On Linux/NVIDIA with Proton SLR and native Wayland HDR:

- All six FSR presets activated with FG Off, AMD FG On and NVIDIA FG On.
- AMD FG survived Native fallback, DLSS Quality, DLAA and return to FSR.
- AMD FG paused generation in menus and resumed fresh output, including live
  Off/On and a menu hold beyond the unchanged FSR startup timeout.
- AMD FG/FSR recreated output at 1080p, 1440p and 4K.
- NVIDIA FG worked with DLSS Quality and DLAA, paused/resumed, and correctly
  required restart after permanent Off followed by an On request.

| Normal quit | Shipping process exit | Time until process disappeared |
| --- | --- | --- |
| Fresh FG Off / FSR presets | 0 | 3.77 s |
| AMD FG transitions and resize | 0 | 5.22 s |
| NVIDIA FG transitions and permanent retirement | `0xc0000409` | 2.33 s |

The third window closed, but its held process handle recorded a fatal exit.
Alpha retirement and shared SDK shutdown reported success beforehand; those
results do not clear the process failure or establish its cause. The required
three clean exits have **not** passed. Every run restored the original setup.

These are bounded operational checks, not complete lifecycle, image-quality,
performance, physical display pacing or native Windows/Radeon qualification.
Anti-Lag 2 driver activation remains untested. Earlier candidate benchmarks do
not qualify these rebuilt binaries. Published files and tags are unchanged.
