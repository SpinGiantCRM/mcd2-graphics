# NGX device identity during owned retirement

NGX initialization receives the game's proxy device. The unshared retirement
path previously passed its unwrapped resource device to `Shutdown1`. The
[NVIDIA API contract](https://github.com/NVIDIA/DLSS/blob/374959484e79a640feaba44c93ac8cfb0a03f5b5/include/nvsdk_ngx.h#L248-L273)
selects an SDK instance by device. The correction passes the same held proxy
used at initialization and rejects a missing pointer. It never passes null,
which would request shutdown of every device's instance.

Shared Streamline ownership remains device-verified and defers NGX shutdown to
Streamline. Unverified ownership retains the generation. Feature/parameter
release, fresh GPU completion, recording invalidation and reverse-order owned
resource release are unchanged. This adds no per-frame work.

## Evidence and limits

The [numeric receipt](../qualification/providers/ngx-device-retirement-2026-10-10.json)
records exact source/build provenance and separate controls. Before this
correction, Native + AMD FG, FSR Quality + AMD FG and DLSS Quality + AMD FG
quit in 3.286, 4.085 and 3.688 seconds respectively. Each process disappeared
without debugger attachment or a forced signal. Process exit codes were not
captured. Successful SDK activation and generated-frame counters were checked
before each quit. Original mod payload/settings hashes were restored.

These controls do not reproduce or erase the previous mixed-provider delay,
and the device correction is not claimed as its established cause or cure.
The device-identity-only mixed regression still stalled beyond 40 seconds and
returned process exit code 1. NGX release, parameter destruction and device
shutdown all returned success first. The private saved Windows stack contained
a return into the exact addon's Native FG generation static destructor; binary
disassembly identifies the preceding call as `IUnknown::Release`. This is a
stack-candidate correlation, not a complete Windows unwind. Raw stacks are
not published. The source
checks run in both Linux and Windows CI, complementing actual compilation and
gameplay. Native Windows/Radeon Anti-Lag and FSR/FG, Windows/NVIDIA NGX lifecycle,
wider lifecycle, borrowed-recording retirement and ordinary installer
qualification remain required. Published tags and downloads are unchanged.


## Terminal generation cleanup correction

Native FG, FSR and NGX generations now use the existing process-lifetime guard.
The NT shutdown query is resolved at device initialization. During final process
termination, the generation is retained for OS reclamation rather than calling
COM/SDK code through renderer DLLs that are unloading. Terminal queue/device
callbacks return before acquiring the observer mutex, and terminal DLL detach
marks shutdown without unregistering callbacks. Ordinary detach still unregisters;
gameplay retirement, submission/reset proofs and fresh GPU completion remain.
This does not permit freeing replayable command-list borrows.

Native guide retirement also clears its ownership vector after manual reverse
release, preventing its destructor from releasing the same resources twice.
Portable behavioral checks cover nullable generation reset, ordinary destruction
and terminal retention. Both Windows and Linux CI retain these checks and the
existing real Windows NT-query DLL fixture. The isolated SR recipe copies and
hashes both shared terminal-guard headers.

The NT-guard-only mixed trial also exceeded 40 seconds and returned exit code 1.
Its saved stack again correlated with the Native FG generation destructor.
ReShade invokes `AddonUninit` and then `FreeLibrary` while ordinarily unloading
addons, before the process necessarily enters NT termination. The NT guard alone
therefore does not resolve this unload path.

## Retained-generation ordinary unload

`AddonUninit` now checks whether any NGX, FSR or Native FG generation remains.
An empty-generation unload retains its existing behavior. A retained generation
pins the addon and the exact ReShade module by their addresses outside DllMain,
then unregisters callbacks. A later initialization is refused in that process;
it cannot reinterpret this retained state as a fresh renderer. The runtime
receipt reports successful pins separately from retirement completion.
[Microsoft documents](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandleexw)
that pinned modules remain loaded until process termination. The generation lifetime guard covers both permanent retained-unload state and
NT termination, so a failed pin cannot trigger resource destructors either.
Pin success is still required before admitting this path. This is deliberately retention, not a GPU
completion or replay-invalidation claim; memory remains held until process exit.
No host command list is reset, no fence is fabricated, and no shutdown is forced.
