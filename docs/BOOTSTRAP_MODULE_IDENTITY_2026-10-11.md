# Bootstrap module identity

An AMD startup run entered the NVIDIA FG configuration path after Off → On.
FSR remained active, but FG latched a failure after SDK result 31. The process
had both the application-local and system `dxgi.dll` loaded.

The guide, controls and latency adapter previously resolved bootstrap ownership
by `GetModuleHandleW(L"dxgi.dll")`. Microsoft's
[API contract](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandlew)
states that basename lookup is unpredictable when multiple matching modules are
loaded. The failure is consistent with that ambiguity; the selected handle was
not captured, so its cause is not conclusively established.

Bootstrap consumers now resolve the full executable-directory path. The helper
caches only the checked path, uses a heap buffer, loads no library and does not
retain a stale module handle. Missing local bootstrap cannot fall back to system
DXGI. Guide ownership, UI capability reporting and shutdown detachment use the
same helper. Normal recording resets also avoid querying shared SDK cleanup
unless the presentation owner is closing and already retired.

A real Windows-ABI fixture loads two different DLLs named `dxgi.dll` in both
orders. It checks exact identity, absence before local loading, a directory
containing spaces, independence from current working directory and refusal after
local unloading. The fixture passed under Proton; Windows CI runs it natively.
Source/include-receipt checks run on both platforms. The fixture also passed
natively in Windows CI.

## Matching-artifact runtime checks

Commit `e7dc811f97079060029f6824345c1164af6ce76d` was tested with its exact
MSVC artifact, archive SHA-256
`bf801b2ca928a375489f468608750692c114ad80e23d345eff7756131ab38b61`.
Two fresh Linux/NVIDIA launches completed FSR Quality + AMD FG On → Off → On,
then FSR Native AA + AMD FG On. Both sequences reported successful SR evaluation,
the expected FG activation state and zero SDK errors/warnings. Neither entered
the NVIDIA FG configuration path. Each normal quit returned code 0, with one
successful shared SDK retirement receipt, in approximately 4.20 and 4.70 seconds
after confirmation. Original mod files, preferences and renderer configuration
were restored exactly after each run.

See the [sanitized receipt](../qualification/providers/bootstrap-identity-linux-2026-10-12.json)
for provenance and scope. These bounded stationary checks do not establish a
performance/latency benefit, native Windows/Radeon support or a conclusive cause
of the earlier intermittent failures. Longer lifecycle checks remain required.
