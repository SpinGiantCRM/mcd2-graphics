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

Three subsequent [short isolation controls](../qualification/providers/amd-dev10-exit-controls-2026-10-14.json)
used unchanged dev.10 files with FSR Quality and a NVIDIA FG startup owner:
active quit, permanent Off quit, and Off/On refusal quit all returned 0.
The captures validate mod-owned SR record framing, checksums and field semantics.
These results show that Off/On alone did not reproduce the longer mixed-provider
failure. They do not clear it or establish its cause.

A subsequent [exception-logged replay](../qualification/providers/amd-dev10-seh-replay-2026-10-14.json)
repeated the six FSR presets, DLSS Quality/DLAA, menu suspension, FSR return and
permanent Off/On refusal. It exited 0 after a 22-minute diagnostic session.
This used the same payload through a direct Proton launch with standard Wine
exception logging and bounded private storage. The altered launch/logging timing
prevents treating it as an equivalent Steam qualification or performance run.
The earlier fatal exit remains unresolved; no source fix is inferred from this
non-reproduction, and raw logs are not published.

These are bounded operational checks, not complete lifecycle, image-quality,
performance, physical display pacing or native Windows/Radeon qualification.
Anti-Lag 2 driver activation remains untested. Earlier candidate benchmarks do
not qualify these rebuilt binaries. Published files and tags are unchanged.
