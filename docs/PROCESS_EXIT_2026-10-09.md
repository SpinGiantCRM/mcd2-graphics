# Final process teardown — 9 October 2026

## Observed failure

Two Linux/NVIDIA developer runs completed FSR Quality → Native with source
resolution restored to 100%, but remained alive after Save and quit. Disabling
Streamline factory routing did not prevent the second failure.

The recovered original exception was a read access violation in `nvngx_dlss.dll`.
The call chain passed through Streamline shutdown, the timing addon's device
cleanup and ReShade's device destruction, while `LdrShutdownProcess` was already
detaching DLLs. The subsequent exception-reporting path waited for Streamline's
log worker. This identifies reentrant SDK cleanup during final process teardown;
it does not establish the cause of every historical Windows exit failure.

## Change

Resolve `RtlDllShutdownInProgress` during addon initialization. Device cleanup
and addon uninitialization decline SDK shutdown, locking and worker joins only
when final process termination is in progress. The DLL detach reserved pointer
provides a secondary signal. Coordinator and thread holders abandon destruction
only on this terminal path; the operating system reclaims their process memory.

Ordinary device destruction and dynamic addon unloading retain worker joins,
SDK shutdown, callback drainage and module unloading. No rendering, frame timing,
menu request, authentication or player-save behavior is changed. This follows
[Microsoft's DLL teardown guidance](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices).

## Checks and limits

- Portable fixtures distinguish ordinary destruction from terminal abandonment,
  including an active frame-token coordinator and its SDK release callback.
- A separate SDK-free DLL fixture verifies the actual NT query: false on
  `FreeLibrary`, true during `ExitProcess`, before its own detach fallback.
  It passes under the current Proton environment and runs in Windows CI.
- The generated FG timing candidate retains the guards and records both new
  header hashes in its build receipt.
- The first guarded Linux run completed 10,011 FSR evaluations, returned to Native
  at 100%, and disappeared after normal quit in 5.133 seconds without a signal.
  Its binary predates a function-pointer cast cleanup. The final binary completed
  at least 4,436 FSR evaluations and normal quit in 5.134 seconds with Streamline
  factory routing enabled. AMD FG was selected but not activated in that launch;
  it is not counted as an FG execution pass. See the [artifact receipt](../qualification/providers/process-exit-2026-10-09.json).

This is bounded Linux evidence. Native Windows gameplay/shutdown qualification
remains separate. AMD FG's independently retained command-recording generation
and `-61` retirement result are not repaired by this change. Raw process memory,
logs and screenshots remain private; release assets are unchanged.
