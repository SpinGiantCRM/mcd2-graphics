# Anti-Lag 2 shutdown pacing correction

The project bridge called `AMD::AntiLag2DX12::Update(context, false, 0)` from
shutdown. In the pinned official header, `Update` invokes the driver's pacing
operation even when disabling the mode. Teardown is outside the pre-input stage.

Shutdown now clears and drains asynchronous FSR presentation readers, then calls
`DeInitialize`. It performs no mode update or pacing call. Failed drains still
retain the context and bridge module; successful retirement still releases them.
The ABI, normal pre-input updates, render-end markers and frame-type handoff are
unchanged. This follows [AMD's integration guide](https://github.com/GPUOpen-LibrariesAndSDKs/AntiLag2-SDK)
and its [pinned SDK header](https://github.com/GPUOpen-LibrariesAndSDKs/AntiLag2-SDK/blob/390aa4a8c8655d0ae6e90079db2c85e103a96da3/ffx_antilag2_dx12.h).

## Verification

The same synthetic driver/presenter fixture was built against the old and new
bridges. Starting with Anti-Lag enabled, it checks that teardown makes no pacing
or mode-update calls, including failed drain, queued callback, successful retry,
API-error cleanup, repeated shutdown and independent no-FG cleanup.

- Old bridge: failed the expected no-shutdown-update assertion, exit 9.
- Corrected bridge: passed, exit 0, using the real Microsoft ABI bridge under Wine.
- Existing controller ordering, unsupported fallback and threaded teardown fixture passed.
- Three build-recipe tests passed; official SDK sources and pins remain unchanged.

The [component receipt](../qualification/providers/amd-antilag-shutdown-2026-10-14.json)
records exact local build/source hashes. Synthetic DLLs stayed outside the game
and packages. This is not native Windows/Radeon or active driver qualification.

The correction does not explain the intermittent Linux/NVIDIA quit failure:
Anti-Lag 2 was unavailable there. The earlier dev.8 game results apply to their
original files, not this changed bridge. A new complete candidate must be rebuilt
and verified before selecting it for Windows testing. The AMD laptop is currently
unavailable; releases remain unchanged.
