# Independent Anti-Lag 2 integration — 8 October 2026

This is an opt-in development path. FSR SR and committed menu transactions are
integrated; AMD FG and hardware qualification remain in progress. No AMD support
is released.

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

The ordinary Video page now offers **AMD Anti-Lag 2: Off / On** only when the
current settings session matches a driver-confirmed runtime response and its
timing owner is installed. Requests use the committed authority. Missing, stale,
faulted or unsupported responses hide the row; no Boost mode is invented. The
active-mode message requires the driver update's matching committed revision.
The worker publishes these fields through the existing display runtime slot,
without changing its class path or schema. A saved request is not SDK activation.

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

## Bounded Linux/NVIDIA game check

The final source-matched latency addon SHA-256 is
`f8bb76ca4301721827fa9e45e8dc3cda96fabae9b2c07e1c2820351366b7bf16`.
The local SDK bridge SHA-256 is
`9da5e85b5198d08dd11a0a8866a38fd34320f3757f36cde6ee648850b0304230`.
Only the latency addon and opt-in bootstrap policy were temporarily installed;
the AMD bridge was deliberately absent from the game directory.

On Steam build 25754144 / CachyOS Proton SLR / RTX 4080 SUPER, the game reached
the main menu and world. Process mappings confirmed the exact latency addon and
neither an AMD bridge nor driver. AMD eligibility was rejected; availability,
active state, input calls and render calls remained zero. Reflex On completed
24,142 real frames by the sampled checkpoint with zero coordinator, marker or
Reflex faults. This establishes bounded fallback and continued NVIDIA timing,
not performance parity, latency reduction or active AMD support.

After normal shutdown, all twelve released payload hashes and the six mod-owned
settings/runtime slots matched their backups byte for byte. No player/account
save, raw log or screenshot is included in this report. Both portable platform
CI jobs and the full native Windows FG/Anti-Lag build passed at implementation
commit `ff88e33b`, including execution of the synthetic SDK-interface fixture.

## Ordinary-menu fallback check — 9 October 2026

The source-matched menu candidate reached the main-menu Video page and hub
with `AmdAntiLag2=1`, Native rendering and FG Off. The AMD row was absent;
NVIDIA Reflex remained visible and On. The actual driver eligibility gate
rejected this Linux/NVIDIA session. AMD availability, activation, input and
render calls remained zero, while Reflex completed 125,034 real frames at the
last checkpoint with zero coordinator, SDK-marker or Reflex faults.

The shipping process disappeared after normal Save and quit without forced
termination. Its exit code was not captured, so this is not an exit-code pass
or a repair of the earlier FSR menu trial's delayed shutdown. All twelve original
payloads and all mod-owned settings/authority files were restored byte for byte.
The native addon, latency adapter and own UI archive hashes are recorded in
`qualification/providers/amd-latency-menu-2026-10-09.json`. No vendor runtime,
raw log, screenshot or player/account data is included. Active Windows/AMD
menu operation and latency benefit remain unqualified.

## Remaining qualification

Native Windows/AMD must verify the actual adapter/driver, Off/On through real
menu transactions, the driver latency monitor, gameplay/travel/exit and preserved
native limiter. Independent latency and Off/On frame-time measurements precede
any benefit claim. NVIDIA/Proton can establish unsupported fallback only.
Active Anti-Lag, AMD FG coexistence, new menu controls and packaging remain
unqualified. No released tag, installer or dependency is changed.
