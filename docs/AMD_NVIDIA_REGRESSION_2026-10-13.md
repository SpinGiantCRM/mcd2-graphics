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

## Corrected-guide runtime checks

The [correction receipt](../qualification/providers/amd-pr54-restart-correction-2026-10-13.json)
records two subsequent trials using a production-source corrected guide over
the original complete candidate, with the same framework. This was a deliberate
guide-only substitution, not qualification of a complete rebuilt archive.

NVIDIA startup and temporary pause/resume passed. After permanent Off then On,
the runtime correctly reported `RestartRequired=1` while refusing activation.
AMD generation stopped while Off, real presents continued, and generation resumed
when On without a restart requirement. One resumed interval advanced both real
and generated counters by 3,600, with no SDK faults/errors/warnings. This is not
a physical display measurement.

Both held-process normal quits returned exit code 0, in 4.49 seconds for NVIDIA
and 3.87 seconds for AMD. Both shared SDK retirements returned 0. Each original
setup was restored byte for byte.

The source-matched complete rebuild from run 38333151314 / artifact 11783652643
passed Windows build and both platform checks. Its archive digest is
`9574ec28b096f35e484af61e066835a1815178070d96e390c4c24ddd599c9e64`;
all 44 receipt members and 41 FG source hashes were verified. Isolated
`0.4.0-amd.dev.8` installer inputs contain 17 own files. Their new framework
binary is a separate dependency and has not inherited the earlier framework's
runtime evidence. Complete rebuilt-candidate and Windows/Radeon checks remain.

The original artifact's normal quit returned held-process exit code 0 in 3.97
seconds. Closing-owner cleanup first deferred while NGX was retained, then
shared SDK retirement succeeded after native queue notification. Original files,
own mod settings and renderer configuration were restored byte for byte.
Player/account saves were not copied or edited; raw traces remain private.

The original candidate is not cleared by this report. Its earlier SDR quit
failure and transient HDR generation pause remain recorded. Published releases
and selected artifact pins are unchanged.


## Complete dev.8 regression

The [rebuilt NVIDIA receipt](../qualification/providers/amd-pr54-rebuilt-nvidia-2026-10-13.json)
uses the complete dev.8 payload and its newly selected framework, without a
sampler or private guide substitution. All six FSR qualities, a repeated Quality
selection, DLSS Quality and DLAA produced matching current guides and successful
sampled two-present outcomes. FSR image tags had matching frame identity and
valid stage-6 alpha. Pause suspended generation without requiring restart;
resume restored it. Permanent NVIDIA Off then On correctly refused activation
and reported `RestartRequired=1`.

Shutdown did not pass. The process remained alive after the menu quit and
required an explicitly approved termination of that process alone. The retained
trace contained no closing-swapchain callback. OS metadata still showed render
and RHI threads; their presence or wait state does not identify the blocked call.
No cause is established. Original mod files, own preferences and renderer
configuration were restored byte for byte. This failure remains recorded despite
the successful gameplay checkpoints and earlier normal exits.

The [rebuilt installer receipt](../qualification/providers/amd-pr54-rebuilt-installers-2026-10-13.json)
verifies both platform folders from run 38340596287 against the selected dev.8
inputs: all 17 own payloads match, with semantically matching manifests and
locks and no bundled vendor runtimes. This is packaging evidence, not Windows
GUI, Radeon gameplay or complete lifecycle qualification.


## Private shutdown tracing control

A [short diagnostic control](../qualification/providers/amd-pr54-nvidia-shutdown-control-2026-10-13.json)
repeated DLSS Quality → DLAA → permanent FG Off → requested On. It substituted
one private guide built from current production source, adding only callback
entry/return timestamps and no GPU commands. The same selected framework and
other dev.8 payloads remained installed. Normal quit returned actual process
exit code 0 in 3.56 seconds. Closing-swapchain and native SR retirement callbacks
were observed, and shared NVIDIA SDK quiescence returned in 13 ms. Original
files/settings were restored afterward.

This shorter instrumented run did not reproduce the earlier production hang.
It neither clears that failure nor proves the trace fixed it. The failing run
stopped before any closing-swapchain observation; a longer matched control and
broader lifecycle coverage remain necessary to identify its cause.


## Longer native callback tracing control

The [13-minute control](../qualification/providers/amd-pr54-native-shutdown-trace-2026-10-13.json)
added a low-frequency native Present heartbeat and generation cleanup entry/return
markers, alongside the private guide trace. It exercised all six FSR qualities,
then DLSS Quality, DLAA and permanent FG Off → requested On. The native callbacks
continued returning during the session. Normal menu quit returned actual process
exit code 0 in 4.79 seconds; GPU completion, native feature cleanup and shared
NVIDIA SDK quiescence all returned. The latter took 14 ms. Original files and
settings were restored.

These private native/guide substitutions add no GPU commands but can affect CPU
timing. This run does not establish the production hang's cause, qualify the
production archive or prove continuous FG activation throughout thirteen minutes.
The earlier retained native status showed many missing temporal passes and an
unreset history; that observation alone does not establish a deadlock in SR.

## Unmodified production control, 14 October

The [matched production control](../qualification/providers/amd-pr54-production-shutdown-control-2026-10-14.json)
used the exact selected dev.8 payload and dependencies, with no private native,
guide or AMD bridge substitutions and no performance sampler. Installed hashes
were checked before testing. The same shipping process remained alive for the
planned 780-second world-session interval, including the settings checks.

All six FSR presets with NVIDIA FG passed two sampled activation checks each.
DLSS Quality and DLAA also reported successful two-present samples. Permanent
FG Off followed by requested On correctly remained inactive with restart required;
DLAA continued evaluating independently.

Normal menu quit returned shipping-process code 0 in 3.88 seconds. Queue teardown
reported native generation cleanup complete with no retained generation. Shared
NVIDIA SDK shutdown returned 0 after native NGX retirement. Original files,
mod preferences and renderer configuration were restored byte for byte.

This establishes a clean exit without private tracing; it does not reproduce or
explain the earlier failure. That failure remains open. Periodic `lean-status`
snapshots can precede terminal cleanup: use queue teardown and API completion
receipts to assess retirement, rather than treating the last periodic snapshot
as final ownership. No runtime source changed, and Windows/Radeon and active
Anti-Lag 2 qualification remain outstanding.
