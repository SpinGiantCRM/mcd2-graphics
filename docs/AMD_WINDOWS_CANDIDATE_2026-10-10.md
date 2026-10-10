# Windows/Radeon candidate handoff

This is a development qualification artifact, not a released installer.
Use [PR45's Windows build](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/38009520993/artifacts/11652882639)
and download `fg-windows-candidate`, artifact 11652882639.
It contains the [AMD Streamline feature lifetime correction](AMD_STREAMLINE_FEATURE_LIFETIME_2026-10-10.md)
and the [retained-generation unload correction](NGX_DEVICE_RETIREMENT_2026-10-10.md).

Archive SHA-256:
`b0e4d1ba4acb8d52e6063d67d41419e0446b9d425dd582846a8f76d6b10c7f8b`.
The embedded source commit `826f18177418044c68910f9e85e21f0dfbbb00f2` is GitHub's
test merge of PR45 head `ea6e651431245752944502c56bba713174033f1f`. Its tree
`5af0e85bc4c71e13eb12adadec7599af617c540e` matches the merged implementation.
All 42 manifest file hashes and the continuous SR addon’s binding to this
Windows FSR bridge were verified. See [the recorded hashes](../qualification/providers/windows-candidate-pr45-2026-10-10.json).
Build success does not establish Windows GPU execution. The bounded mixed-provider
Linux trial exited normally, but Windows active-FSR/FG/Anti-Lag and shutdown
remain qualification gates. Do not substitute Linux-built binaries or old hashes.

The [PR44 receipt](../qualification/providers/windows-candidate-pr44-2026-10-10.json)
and [PR42 receipt](../qualification/providers/windows-candidate-pr42-2026-10-10.json)
remain historical evidence and do not qualify this rebuilt candidate.

## Prepare a reversible trial

Close the game and use the verified Steam Win64 build. Keep a backup of every
replaced mod file, `MCD2Graphics*.sav` and the mod's `ProviderSettings` folder.
Do not copy/edit account or character saves. Preserve the working Blueprint
Loader and RenoDX files. Never install isolated probe executables/addons or
synthetic driver fixtures into the game. A Windows operator must verify exact
game executable, baseline dependency hashes and a rollback plan before writing.

Map these artifact files relative to `Dungeons/Binaries/Win64`:

| Artifact | Destination |
| --- | --- |
| `probe/dxgi.dll`, `probe/fg-sdk-bridge.dll`, `probe/fg-chain-observer.addon64`, `probe/mcd2-fg-guide-recon.addon64` | Same basenames |
| `latency/mcd2-display-latency.addon64`, `amd-latency/mcd2-antilag2-bridge.dll` | Same basenames |
| `sr/mcd2-graphics.addon64` | `mcd2-graphics.addon64` |
| `experimental-reshade/ReShade64.dll` | `d3d12.asi`, preserving its separate build/license provenance |
| `isolated-fsr-fg-game-bridge/mcd2-fsr-fg-game-bridge.dll` | `mcd2-fsr-fg-game-bridge.dll` |
| `isolated-fsr-game-bridge/mcd2-fsr-game-bridge.dll` | `MCD2Graphics/fsr/mcd2-fsr-game-bridge.dll` |
| `sr/live_dense.cso`, `sr/fsr_dense.cso` | `MCD2Graphics/live_dense.cso`, `MCD2Graphics/fsr_dense.cso` |

The three `ui/Pak/MCD2Graphics_P.*` files go in
`Dungeons/Content/Paks/~mods/MCD2Graphics`. Validate every copied hash.
The Windows SR bridge hash must equal the SR receipt's `fsrBridgeSHA256`:
`2e9b74f5d165a7e6b8191e3fc982636c7ada884430b4ff9035db030d8a977fca`.
Do not substitute a Linux-built bridge or the bounded sustained-probe addon.

Acquire vendor runtimes separately from their official pinned SDK sources;
they are absent from this archive. Existing Streamline/NGX dependencies retain
their lock-file hashes. Analytical FSR SDK 2.3 commit
`60f4ea81909200d8542eca14dccb2628b763a9a3` requires:

- `amd_fidelityfx_upscaler_dx12.dll` under `MCD2Graphics/fsr`, SHA-256
  `d0dcccc74a43c44ba435b7a369b456e0970d8a4464e4bd683119b374f2c9fb46`.
- `amd_fidelityfx_framegeneration_dx12.dll` beside the FG bridge, SHA-256
  `02297beedd285e822d3a64f314cf00faf378dcec0edc47ff0c4dd71b3a8c2f18`.

Use these trial policies in the existing configuration files, preserving a backup:

```ini
; FGBootstrap.ini
[Experiment]
EnableSDK=1
FactoryRouting=1
TraceNvapi=0
[Providers]
ConsolidatedMenuTransport=1
ObserveLegacySettings=0
AmdAntiLag2=1
```

```ini
; FGGuideCapture.ini
[Capture]
Enabled=0
GuideTags=1
ImageTags=1
EnableFG=0
TrialRevision=0
NativeUIToggle=1
```

## Required runtime evidence

1. Record Windows build, actual rendering GPU/LUID, driver, game build and all
   artifact/dependency hashes. Do not infer the rendering GPU from the display GPU.
2. Native + FG Off: startup, gameplay, menu access, source 100%, no accidental
   reconstruction, zero generated frames. NVIDIA-only choices must fail safely
   on Radeon before lowering source scale.
3. FSR Native AA, Quality, Balanced, Performance, Ultra Performance and Custom:
   verify current revision/world/source ACK, active dimensions, successful
   evaluations and zero errors. Invalid/missing dependencies must return to
   Native with confirmed full scale. Check slider/preset persistence after restart.
4. Choose AMD FG, enable and restart. Check Native AA + AMD FG and FSR Quality +
   AMD FG separately: current guides, real/generated counters, zero SDK faults,
   one presenter, pause stops generation and resume resets history correctly.
5. Anti-Lag requires actual native Radeon driver initialization. Verify its row
   only appears when available, Off → On → Off changes the matching committed
   runtime state, and the native limiter remains usable. Repeat with FG Off and
   On, including presenter metadata clearing/drain. Use AMD's latency monitor;
   a selected setting or synthetic fixture is insufficient.
6. Travel hub → dungeon → hub, menus, resolution/window changes, provider changes
   and a longer gameplay session. Capture real frame intervals and SDK generated
   counts independently; distinguish physical presentation from either counter.
7. Quit normally twice. Record elapsed time and actual process exit code, not
   only window disappearance. Restore the exact baseline and all mod preferences;
   confirm dependency and save preservation. Retained work/failed drain is a
   failure to investigate, not permission to free replayable GPU resources.

Return sanitized numeric results and exact errors. Raw private logs and captures
stay local. Active Radeon Anti-Lag, Windows FSR/FG and installer qualification
remain open until these checks execute on the actual hardware.
