# FSR Frame Generation: first game presentation

An opt-in developer route now generates frames from the game's real camera,
depth, motion vectors and HDR world image. This is an integration checkpoint;
it is not yet an ordinary menu-selectable AMD FG release.

## Evidence

Two Linux/NVIDIA game runs used the analytical FSR 3.1.6 interpolator and
3.1.7 presenter from the pinned FidelityFX SDK 2.3.0 runtime. Both reached the
world, generated 600 frames, reported no SDK errors/warnings, disabled generation
at the trial limit and retired the AMD contexts successfully on normal close.
The second run used the final bridge's exact world-copy dimension guard.
The original mod files and mod-owned settings were restored byte for byte.

The final bridge also passes an isolated Off → On → Off host: 30 real / 0
generated, 60 / 60, then 30 / 0. Retirement is refused while recorded commands
remain live and after a failed Reset; a successful Reset followed by a fresh
completed queue fence permits destruction. The host exits with code zero.

The game trial uses 3840×2160 PQ output and 2560×1440 DLSS Quality guides.
It proves AMD presentation with real game inputs, not FSR-upscaling/AMD-FG parity,
Windows/AMD execution, latency reduction, visual quality or a performance gain.
The sanitized [receipt](../qualification/providers/fsr-fg-game-2026-10-09.json)
records exact binaries; no screenshots, raw logs or game assets are included.

## Integration and fixes

- One AMD presenter owns the game swapchain. No NVIDIA presenter is chained
  into it. Missing/failed construction falls back to the game's native factory.
- D3D12 API descriptors and COM calls stay inside a Microsoft-ABI bridge with
  a fixed-width C interface and the existing 16 KiB stack guard.
- The analytical provider and presenter identities are queried explicitly.
  The external runtime's SHA-256 is verified before loading it.
- Prepare uses current camera parameters, game-unit conversion, measured frame
  cadence and converted depth/motion. The world image is copied before UI
  composition. Matching engine identity is required before generation.
- ReShade device identity is compared through canonical COM identity; wrapped
  and native interface pointer addresses are not assumed equal.
- Zero-sized factory descriptors use the actual Unreal window's client size.
  Temporary startup-device cleanup cannot destroy an empty pending session.
- The current timing addon recognizes Steam build 25754144. The previously
  installed binary rejected this build, leaving all frame identities unavailable.
- Recorded command lists retain their borrowed resources. Successful Reset
  epochs and a subsequent fresh queue fence are required before context release.
  A failed proof retains the context. The vendor module remains loaded while
  external game swapchain references can still execute its code.

## Remaining work

Connect provider selection and capability acknowledgements to the normal menu;
produce guides with FSR SR and Native without relying on NGX; validate AMD
timing/Anti-Lag coexistence; then qualify transitions, resize, shutdown, artifacts
and Off/On performance on Linux and Windows/AMD. The isolated Windows candidate
build compiles this bridge and host separately from the installer. Vendor runtime
DLLs remain external; existing release assets are unchanged.
