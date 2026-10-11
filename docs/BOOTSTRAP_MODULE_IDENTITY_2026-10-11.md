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
Source/include-receipt checks run on both platforms. Updated game artifacts need
their own transition and shutdown qualification; this fixture alone does not
qualify gameplay or Radeon runtime.
