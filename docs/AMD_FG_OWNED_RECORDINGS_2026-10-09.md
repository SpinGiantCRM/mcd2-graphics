# AMD FG private command recordings — 9 October 2026

Developer candidate; released binaries are unchanged.

## Problem and change

The previous AMD bridge recorded SDK Prepare and the HUD-less world copy in
game-owned command lists. A dormant final recording remained legally replayable
without another successful Reset. Retirement correctly refused with `-61`, even
after queue completion; completing a submission does not invalidate its recording.

Recording contract 2 stages strong references to the current depth, motion and
world resources. After host submissions, the pre-Present callback validates the
exact swapchain graphics queue and records Prepare plus the world copy in a
private three-slot command-list ring. Busy slots skip generation without a CPU
frame-time wait. Slot reuse requires completed submission and successful own
Reset before old borrowed resources are released.

Retirement disables callbacks, obtains fresh queue completion, releases only
private recordings and their borrowed references, drains presentation, then
destroys SDK contexts. Failed completion retains ownership. No game command list
is reset or destroyed. The runtime module remains loaded for external swapchain
COM interfaces. Bootstrap refuses bridges without the version 2 contract and
exports; mixing older bridge/observer binaries is unsupported.

## Evidence

- Isolated owned-scene test: Off 30 rendered / 0 generated; On 60 / 60;
  Off 30 / 0. No faults, SDK errors or warnings.
- An unrelated graphics queue is refused. SDK retirement succeeds while a host
  barrier recording remains closed and replayable; replay after SDK retirement
  and repeated retirement succeed.
- Normal Steam launch, native menu selection: FSR Quality at 2560×1440,
  3840×2160 output, AMD FG On and actual AMD startup presenter ownership.
- Bounded Linux gameplay: 6,470 FSR evaluations and 6,442 generated frames;
  no AMD SDK errors, warnings or faults. Pause counts stayed at 3,863 generated
  frames; resume reactivated generation.
- Normal Save and quit: process gone in 2.933 seconds, no signal sent;
  exit code was not captured. AMD context retirement returned 0.
- Original mod files and preferences restored byte for byte. Player/account
  saves were not edited. Raw logs and captures remain private.

The isolated host was rebuilt to add queue/refusal and barrier-replay checks;
its bridge binary differs from the installed gameplay binary while bridge source
is unchanged. Exact binary/source hashes are in the
[qualification receipt](../qualification/providers/amd-fg-owned-recordings-2026-10-09.json).

Local checks: 85 portable Python checks passed (5 skipped), plus 21 FG recipe
checks. They protect export admission, staging without host GPU commands,
queue identity, fence/Reset ordering, failed-signal retention and retirement.

## Limits and next gates

These are Linux/NVIDIA execution results, not Windows/Radeon qualification,
performance benchmarks, latency measurements or artifact-quality certification.
Resolution changes, device loss and long sessions still need coverage.
FSR SR separately reported phase 6/error 27 on queue destruction; this change
repairs AMD FG retirement, not the independent SR lifetime path.

Preserve both Windows and Linux CI, the 16 KiB native stack guard, source pins,
external runtime acquisition, Anti-Lag clear/drain handshake and ordinary
process-exit guard. Native Windows/Radeon must exercise the exact rebuilt bridge,
activation/pause/resume and shutdown before release admission.
