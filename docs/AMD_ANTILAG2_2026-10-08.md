# Independent Anti-Lag 2 integration — 8 October 2026

This is an opt-in development path. FSR SR/FG, real menu transactions and the
full settings-reader cutover remain in progress; no AMD support is released.

## Contract and ownership

The official [SDK documentation](https://github.com/GPUOpen-LibrariesAndSDKs/AntiLag2-SDK/blob/390aa4a8c8655d0ae6e90079db2c85e103a96da3/README.md)
requires Update before input, render-end notification before FG presentation,
and context release before device destruction. It lists native Windows, RDNA
hardware and supporting Adrenalin DX12 drivers. Proton support is not claimed.

- The unchanged MIT SDK header compiles with the Microsoft COM ABI behind a
  versioned C bridge; the MinGW addon does not duplicate vendor interfaces.
- Actual D3D12 device LUID, non-software AMD adapter, native Windows and an
  already-loaded `amdxc64.dll` are required before loading the project bridge.
  SDK initialization must succeed with a non-null interface.
- The existing inspected game-layout and modular-feature ownership guards remain
  required. AMD and NVIDIA are never both bound as timing owners. AMD does not
  install the NVIDIA PCL message hook.
- `Update(enabled, 0)` runs once per real simulation identity in the verified
  pre-input pacing hook. Repeated pacing queries do not add delays. Returning
  false preserves the game's native FPS limiter; no additional cap is set.
- Enabled Anti-Lag receives render-end and real-frame notifications at the
  matching device's pre-present boundary. No generated identity is invented.
- SDK calls and context release are serialized against game/render callbacks.
  After shutdown, retained callbacks are inert. The bridge stays mapped until
  addon cleanup has drained callbacks and joined the settings worker.

## Requested settings

`[Providers] AmdAntiLag2=1` opts in through the development bootstrap policy.
Default zero adds no AMD authority reads, adapter checks or bridge loads.
The existing settings worker reads the committed ProviderSettings/intent-v5.bin
record, including its revision/checksum. Automatic or RadeonAntiLag2 with mode
On requests enabled Anti-Lag. Off, corrupt/missing authority, another latency
provider, Boost or any FG request resolves to Off. No legacy Reflex preference
is reinterpreted as AMD intent, and the reader never writes the authority.

The current transport probe still sends no-op requests only. Real menu controls
are a subsequent cutover step; a saved request is not feature activation.

All FG combinations remain disabled here. FSR FG needs the owned swapchain's
documented Anti-Lag context/enabled private-data handshake and generated-frame
notifications before admitting coexistence. No nested presentation owner is added.

## Component evidence

`amd_latency_test.cpp` covers 32 eligibility combinations, the latency/mode/FG
matrix, exact record stamps, Off, initialization/API failures, repeated/reordered
identities and in-flight update versus teardown. Unsupported candidates make
zero AMD SDK calls. GCC execution and MinGW execution in an isolated Wine
prefix pass. Both platform CI jobs retain this fixture.

`build_amd_latency.py --with-tests` builds the real bridge and a synthetic
SDK-interface fixture. It verifies the GUID/COM ABI, disabled initialization,
maxFPS zero, cached mode changes, S_FALSE delay normalization, render-end and
real-frame flags, errors, idempotent shutdown and reinitialization. It passes
in the isolated Wine prefix. This is not a real driver or latency measurement.

Windows CI builds and runs that fixture. Only the project bridge, receipt and
license are copied into its artifact; fake driver/test binaries are excluded.
Local bridge and actual FG latency-candidate builds pass frame guards. Existing
Reflex token, engine-layout, build-recipe and FG regressions pass.

## Remaining qualification

Native Windows/AMD must verify the actual adapter/driver, Off/On through real
menu transactions, the driver latency monitor, gameplay/travel/exit and preserved
native limiter. Independent latency and Off/On frame-time measurements precede
any benefit claim. NVIDIA/Proton can establish unsupported fallback only.
Active Anti-Lag, AMD FG coexistence, new menu controls and packaging remain
unqualified. No released tag, installer or dependency is changed.
