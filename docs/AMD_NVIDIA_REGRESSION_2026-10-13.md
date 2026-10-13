# PR54 NVIDIA regression and restart status

The complete `0.4.0-amd.dev.7` archive, exact selected framework and original
HDR addon were exercised on Linux / RTX 4080 SUPER. No private boundary guide
or CPU benchmark sampler was installed. Output reached 3840×2160 PQ/BT.2020.
The [numeric receipt](../qualification/providers/amd-pr54-nvidia-regression-2026-10-13.json)
identifies the tested artifact and retains the failed gate.

FSR with NVIDIA FG passed bounded runtime checks for Native AA, Quality,
Balanced, Performance, Ultra Performance and custom 78%. Each produced matching
guide sizes, frame-token identity, valid alpha and successful sampled two-present
outcomes. Pause disabled generation; resume restored it. Switching to DLSS
Quality and then DLAA created the expected NGX source/output dimensions and
kept FG active. These observations do not qualify moving-image quality,
physical display pacing, latency or Windows/Radeon execution.

## Failed gate and correction

Selecting NVIDIA FG Off permanently released its feature successfully. Selecting
On afterward retained the same startup presenter, but the session was retired
and activation correctly remained refused. The shared controller incorrectly
reported `RestartRequired=0`: its ownership-only calculation omitted retirement.
This was not a failed SDK dispatch or an AMD live-toggle failure.

The shared restart predicate now includes permanent retirement while preserving
ownership-change and unavailable-provider behavior. Actual activation retains
its refusal; no attempt is made to revive a retired presenter. Temporary menus
do not retire it, and AMD live Off/On keeps its existing behavior. A 96-case
portable projection fixture runs on both platforms, with a source check tying
the controller to that predicate. Corrected binaries need a fresh runtime check.

The original artifact's normal quit returned held-process exit code 0 in 3.97
seconds. Closing-owner cleanup first deferred while NGX was retained, then
shared SDK retirement succeeded after native queue notification. Original files,
own mod settings and renderer configuration were restored byte for byte.
Player/account saves were not copied or edited; raw traces remain private.

The original candidate is not cleared by this report. Its earlier SDR quit
failure and transient HDR generation pause remain recorded. Published releases
and selected artifact pins are unchanged.
