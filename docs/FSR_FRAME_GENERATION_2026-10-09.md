# Isolated FSR Frame Generation execution

The pinned AMD SDK executes frame interpolation and owns a working Off → On → Off
presenter on Linux/NVIDIA, directly and through the qualified ReShade wrapper.
This is groundwork for game integration. It does not enable AMD FG in the game,
qualify Windows/AMD, establish image quality or measure gameplay performance.

## Results

| Gate | Direct D3D12 | Qualified ReShade device |
| --- | --- | --- |
| Two 16-frame owned-input trials, SDR-like and scRGB HDR | Pass | Pass |
| All 32 generated outputs finite and correctly scaled | Pass | Pass |
| History resets at frames 0 and 8 in each trial | Pass | Pass |
| Off: 30 real submissions / 30 presentations | Pass | Pass |
| On: 60 real submissions / 120 presentations | Pass | Pass |
| Off again: 30 real submissions / 30 presentations | Pass | Pass |
| SDK warnings/errors, presentation callback errors | 0 | 0 |
| Disable, recording invalidation, completed fences and context retirement | Pass | Pass |

The generated callback count and DXGI presentation delta are checked independently.
These are actual presentations from the SDK-owned swapchain, rather than an FPS
estimate from a rendered-frame overlay. The scene contains static owned colour,
depth and zero-motion guides; it does not test gameplay motion or disocclusion.
The constant frame interval and camera definition belong to this test scene and
must not be reused as substitutes for game measurements.

## Versions and ownership

- FidelityFX SDK 2.3.0, commit `60f4ea81909200d8542eca14dccb2628b763a9a3`.
- Actual selected interpolation provider: analytical 3.1.6.
- Actual swapchain provider: 3.1.7, supplied by the same external frame-generation DLL.
- ReShade wrapper: exact qualified dependency hash in the runner and qualification receipt.
- One SDK-owned presenter. No Streamline vendor presenter is chained into it.

Both executables use the Microsoft C++ ABI and the existing 16 KiB stack guard.
They own their device, queue, command recording, textures and fence. Retirement
waits for submitted work and SDK presentations, releases the owned recording,
then proves a subsequent fence before disabling/destroying the context. A failed
retirement proof retains the generation until process exit. The SDK debug callback
uses atomic counters; callback state is retained until presents finish.

## Reproduction

Build with `experiments/providers/build_fsr_fg_probe.py`, supplying the separately
acquired pinned SDK headers and Microsoft sysroot. Add `--presentation` for the
presenter. The existing Windows candidate workflow builds both executables into
separate `isolated-fsr-fg-*` artifact directories, including receipts and license.
It does not bundle an AMD runtime or qualify GPU execution.

Acquire the pinned frame-generation runtime independently; place it beside the
probe. The Linux runner `experiments/providers/run_fsr_fg_probe.py` accepts only
an existing isolated Proton prefix, verifies the executable/runtime hashes and
rejects game prefixes and unexpected proxies. Add `--presentation` for the
presenter; add `--qualified-reshade` only with the exact checked framework. The
runner records results privately and bounds only its own new process group.
Neither probe reads or modifies the game, mod preferences, player saves or accounts.

The sanitized result and source/binary hashes are in
[the qualification receipt](../qualification/providers/fsr-fg-sdk-2026-10-09.json).
No external runtime, vendor source or raw private log is committed.

## Next game gate

Select exactly one final presentation owner before swapchain creation. Feed the
AMD presenter verified post-tone-map world colour, depth, motion, camera and
frame identity; add HUD composition separately. Preserve independent SR-provider
choice, native fallback and lifecycle retirement. Verify actual real/generated
presentations in game before exposing AMD FG as available. Anti-Lag 2 coexistence
requires its SDK swapchain handshake and active Windows/AMD evidence.
