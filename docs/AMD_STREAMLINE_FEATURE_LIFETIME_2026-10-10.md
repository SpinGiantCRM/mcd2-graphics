# AMD presenter / Streamline feature lifetime

## Observed failure

A Linux/NVIDIA development run completed FSR → Native with AMD FG, then FG Off.
The window closed on normal quit but the shipping process remained alive.
AMD retirement reported result 0. A bounded read-only stack/module inspection
located the blocked call inside the bridge's real Streamline shutdown, with
NGX modules on the saved Windows stack. This is evidence for the blocked
integration boundary, not a complete symbolic vendor stack or a vendor-root-cause
claim. The stalled process required a recorded SIGTERM and is **not** a passing
normal exit. Historical failures remain recorded.

## Change

An AMD startup owner now initializes Streamline Reflex/PCL without DLSS-G.
Before binding any device, token or presentation proxy, one bounded discovery
session queries the SDK's actual DLSS-G support for up to 16 physical DXGI
adapter LUIDs, shuts down successfully, and unloads the discovery interposer.
The Reflex/PCL session retains those exact-adapter capability results for the
Video menu. Missing/untested adapters remain unsupported; support is never
inferred from a GPU vendor name. Switching presentation owner requires restart.

NVIDIA and legacy/unowned startup keep the existing full feature set. The
versioned NGX-owner query still rejects device mismatch; the AMD session owns
no DLSS-G NGX instance, so NVIDIA SR performs its own device-scoped retirement.
There is no forced global-shutdown bypass, vendor binary edit, per-frame adapter
scan, rendering shader change or preset change. Duplicate or invalid owner
initialization is rejected before changing the active capability cache.

## Checks and limits

The portable configuration fixture checks startup policy and exact-LUID cache
lookup, replacement, negative results, capacity, overflow and clearing. It runs
in both Linux and Windows CI. The isolated `feature-lifetime` probe exercises
real SDK initialization, invalid/duplicate rejection, actual-adapter support,
D3D12 device binding, NGX ownership, Reflex Off and normal SDK shutdown for
AMD, NVIDIA and unowned starts. It is never a game payload.

The [numeric receipt](../qualification/providers/amd-streamline-feature-lifetime-2026-10-10.json) records the source/build hashes and bounded gameplay
checks. The earlier feature-selection pilot completed FSR → DLSS → Native, FG Off and
normal quit with process disappearance in 3 seconds and shared SDK shutdown 0.
The exact final rebuild repeated FSR → DLSS → Native with AMD FG still selected:
1440p and full-resolution guides were active, retained NVIDIA FG capability was
available, and SDK errors/warnings stayed zero. AMD retirement and shared SDK
shutdown both returned 0. The process exceeded the 30-second quit observation,
then disappeared without a forced signal. Its exit code was not captured.
This delayed exit is **not** qualified and is a separate remaining boundary;
the feature-selection change does not claim to fix all shutdown failures.
Both reversible transactions restored the original mod/settings hashes.

Linux/NVIDIA execution cannot establish native Windows/Radeon Anti-Lag,
Windows provider lifecycle, latency benefit, physical scanout FPS or ordinary
installer qualification. The independently observed FSR borrowed-recording
queue-destruction retirement gate remains open. Published tags and files are
unchanged.
