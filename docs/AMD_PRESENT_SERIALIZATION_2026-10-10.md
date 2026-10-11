# AMD FG presentation synchronization

The AMD bridge serialized its own SDK calls, but returned a raw SDK swapchain to
the game. `Present` and `Present1` therefore did not share that mutex. AMD's
[SDK 2.3 thread-safety contract](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/v2.3.0/Kits/FidelityFX/docs/techniques/frame-interpolation-api.md#thread-safety)
requires whole-call presentation synchronization when the configured callback
dispatches FG, alongside Prepare and context destruction. This is a source-level
contract gap; it has not been established as the cause of the intermittent quit.

The application now receives an owned `IDXGISwapChain4` forwarding object. Its
methods use the bridge's existing SDK gate. Both Present methods hold the gate
through the complete vendor call. The bridge keeps the raw swapchain for vendor
configuration; ordinary COM queries retain the outer identity and unsupported
interfaces are refused rather than exposing the unguarded raw object. Final raw
reference release also uses the gate. A retired presenter refuses later calls;
terminal process teardown skips vendor calls and locking.

A vendor-free Windows-ABI fixture verifies standard COM identity, forwarding,
Present/Present1 exclusion of concurrent SDK work, closed/terminal refusal and
actual final raw-object destruction while concurrent SDK work is excluded.
It passed under Proton. The existing Windows
CI job runs the same fixture on Windows. Source contract checks remain on both
platforms. Both process-guard headers and the forwarding header are hashed in the
bridge build receipt.

The pinned-runtime isolated bridge probe passed its Off → On → Off accounting,
owned-recording replay and normal retirement gates with code 0 in 3.62 seconds.
This synthetic owned scene does not qualify game rendering or Windows/Radeon.
See the [numeric receipt](../qualification/providers/amd-present-serialization-2026-10-10.json).
## Game controls

The game variant retains the PR47 packaged candidate, its exact ReShade framework
and external runtime hashes. Only the bridge and the PR50 Windows/MSVC guide are
replaced; a CPU-only interval/counter sampler is added. FSR Quality + AMD FG was
exercised through FG Off, FG On and FSR Native AA at 3840×2160, followed by the
normal quit menu. Runtime records establish successful FSR evaluation and matching
FG activation at each checkpoint, with no SDK faults, errors or warnings.

Diagnostic stationary-camera samples measured 88.33 and 88.47 rendered frames/s
with FG On, and 124.53 frames/s with FG Off while retaining the AMD presenter.
The On samples counted one generated frame per rendered frame (176.66 and 176.94
SDK-counted frames/s). These are CPU intervals and SDK counters, not physical
screen presentation, GPU timestamps or input latency. The scene is animated;
these short samples do not establish published-baseline equivalence or a release
performance result.

The first combined control exited normally with code 0. An identical repeat
hung on the black quit screen after SDK retirement returned 0. The process
remained alive after a 179,994 ms observation timeout; Windows code 259 here
means STILL_ACTIVE, not a terminal exit code. Read-only thread enumeration found
GameThread still present with RenderThread and RHIThread absent. Only the hung
game process was ended after the observation timeout. Original files and preferences
were restored exactly. Forced termination is not a passing shutdown.

Moving-image, resize/travel, longer-session and Windows/Radeon checks remain
required. Shutdown is not qualified, and the earlier fatal-exit cause is still
unidentified. No release assets or external runtimes are changed.

## Exit phase diagnostics

Two runs of a separate diagnostic build completed the combined control and
exited normally with code 0 in 3.67 and 4.59 seconds after confirmation. Its
exit-only phase markers show both SDK context retirement and final raw swapchain
release returning. No per-frame work was added. These results do not qualify the
uninstrumented candidate or locate the intermittent hang: exit instrumentation
changes timing. The diagnostic code and raw records are kept outside the repo.
