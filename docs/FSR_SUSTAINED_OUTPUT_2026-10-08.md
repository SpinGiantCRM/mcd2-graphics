# Bounded FSR output and Native fallback gate

The subsequent [Quality/rejection gate](FSR_QUALITY_FALLBACK_2026-10-08.md)
adds reduced-resolution output and immediate reset on rejected frames. This
record retains the earlier full-resolution trial and its original limits.

Analytical FSR 3.1.5 replaced 768 gameplay frames across three separate
256-frame runs on CachyOS/Proton and an RTX 4080 SUPER. Every run returned to
Native AA, reset its history and retired its independent recording leases.
The SDK reported zero errors and warnings. This is a Native-AA output trial;
reduced-resolution presets and a supported FSR menu option remain unfinished.

## Change and inputs

The developer recipe extends the [first real-game input gate](FSR_GAME_EVALUATION_2026-10-08.md).
It uses the same analytical provider, external runtime and Microsoft-ABI bridge.
NGX was absent and NVIDIA FG was Off throughout the actual FSR runs.

- Current scene-linear colour, converted depth, backward pixel motion and
  exposure come from the independent pre-temporal producer.
- FSR writes an owned RGBA16F texture. Before replacing the Native temporal
  output, the trial checks its format, dimensions, mip count, sample count,
  dimension and nonaliasing. Copy barriers restore both resources' prior states.
- The original temporal dispatch is skipped only after successful evaluation
  and output replacement. FSR output passes through the existing downstream
  world/UI/HDR rendering path.
- Returning to Native uses a fresh immutable 512-byte upload containing the
  original pass constants with the history-reset word at byte 48 set to one.
  The original game constant buffer is never modified. This reset resource has
  its own recording lease and is not overwritten while in flight.
- The generation cannot retire with dirty Native history or live recordings.
  SDK destruction follows command invalidation and a completed subsequent fence;
  failed retirement retains the context and modules.

Each run captures only its first and last inputs/results, bounding readback
memory independently of the evaluation count. The recipe permits 2–512 frames;
normal addon builds do not consume its private request file.

## Camera and clock verification

A separately compiled NeoRune actor queried `AWorldSettings.WorldToMeters` via
`UGameplayStatics.GetAllActorsOfClass`/the SDK's `World.FindAll` helper and wrote
the result to a mod-owned temporary SaveGame. The observed value was **100**;
the corresponding FSR `viewSpaceToMeters` factor is **0.01**. The temporary UI
and record were removed after verification. The supplied scale is explicit in
the sustained build command, and must be rechecked for a different world/build.

The sustained trial samples QPC at the observed pre-temporal boundary, before
SDK context creation or evaluation, and reads the actual UE View frame stamp
from word 661. All three runs had 256 consecutive view stamps and exactly one
FSR history reset. Native reset flags or stamp gaps request an FSR reset.
Intervals outside 0–250 ms are not evaluated. This is measured rendered-frame
timing; it does not establish simulation/input/present identity or physical
latency. SDK creation and diagnostic readbacks make it unsuitable as a benchmark.

## Results

| Run | Successful FSR evaluations / output replacements | FSR resets | Native resets | SDK errors / warnings | Retired bundles |
| --- | ---: | ---: | ---: | ---: | ---: |
| A | 256 / 256 | 1 | 1 | 0 / 0 | 257 |
| B | 256 / 256 | 1 | 1 | 0 / 0 | 257 |
| Moving camera | 256 / 256 | 1 | 1 | 0 / 0 | 257 |

The third request was armed while a guarded three-second movement input was
held. Its first-to-last captured camera position changed by about 1,396 world
units. It establishes execution during camera movement, not visual-quality or
disocclusion qualification. The earlier B movement attempt occurred after the
bounded run and is therefore not counted as a moving-camera check.

Every run saved ten texture readbacks after GPU completion, then completed SDK
destruction with return code zero. All captured FSR RGB samples were finite;
the structured receipt records their signal ranges. Signal values are scene
linear, not nits or accuracy scores. Raw buffers, images, logs, saves and vendor
DLLs remain private.

The exact binary/runtime pins and sanitized results are in
[the qualification receipt](../qualification/providers/fsr-sustained-2026-10-08.json).
After normal game shutdown, twelve original mod payloads and six mod-owned state
slots were restored and verified, and both temporary SDK files were removed.
Published release files and installers are unchanged.

## Reproduction and remaining gates

Build the bridge with the existing pinned SDK/header recipe. With separately
acquired ReShade/NGX headers and a measured world scale:

```sh
python experiments/providers/build_fsr_sustained_probe.py \
  --output <private-trial-folder> --frames 256 --replace-output \
  --world-to-meters 100 \
  --reshade-include <ReShade-API-20-headers> --ngx-include <pinned-NGX-headers>
```

An explicitly installed developer trial consumes
`provider-fsr-sustained-start.txt` in the existing mod cache, containing a unique
capture label. Back up the exact addon and mod state, require the game closed
for file changes, and restore after the test. The recipe does not acquire or
bundle AMD's runtime. Without `--replace-output`, Native continues to display.

Five source/generation guards run on both CI platforms. Native compilation
retains the 16 KiB stack limit. The Windows candidate recipe separately compiles
the sustained developer addon; it does not substitute it into the installer.
Those checks are not active Windows/AMD runtime qualification.

Next gates are reduced-resolution reconstruction, the shared settings/UI path,
induced failure fallback, resize/travel/device lifecycle, longer sessions,
moving-scene quality and performance. Native fallback here is the normal bounded
stop with valid bindings; a failed binding/reset path is not qualified. Active
Windows/AMD FSR and Anti-Lag 2 remain separate hardware gates. AMD FG swapchain,
world/UI composition and presentation integration are still unfinished.
