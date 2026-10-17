# AMD output lifecycle candidate

Development candidate `0.4.0-amd.dev.7`, not a published release.
Native inputs: [run 38313187332](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/38313187332),
artifact 11775477368. The [artifact receipt](../qualification/providers/windows-candidate-pr54-2026-10-13.json)
pins archive, source tree and all 44 members. The source-matched UI was rebuilt
with NeoRune 0.1.2 and the existing Blueprint Loader 2.0 metadata API.

The installer workflow selects a repository qualification receipt, defaulting
to `windows-candidate-current.json`. It does not silently reuse the historical
PR52 archive. Arbitrary receipt paths are rejected; archive, source, member and
SR/bridge bindings remain mandatory. Vendor runtimes and the separately selected
ReShade framework are not included in the own payload.

The new artifact's framework hash differs from the earlier qualified binary.
Its runtime checks must use its exact dependency lock; earlier framework results
are not qualification of this build. Published installers and pins are unchanged.

## Owned-host controls

The [numeric results](../qualification/providers/amd-owned-output-lifecycle-2026-10-13.json)
record two private Microsoft-ABI probes against the unchanged source-built
generation bridge and the pinned analytical AMD runtime:

- SDR → PQ → SDR, with Off / On / Off at each output encoding.
- 1280×720 SDR → 960×540 PQ → 1280×720 SDR, with the same mode sequence.

Each case completed nine phases: enabled phases recorded 60 real and 60 generated
present callbacks; disabled phases recorded 30 real and zero generated callbacks.
All SDK faults, errors and warnings were zero. Each recreated the generation
context twice; the second case also resized the same presenter twice. Both
retired normally, replayed an independently retained host command list after SDK
retirement and exited 0 without a timeout.

These controlled images are not gameplay, native Windows/Radeon qualification,
physical display FPS or measured input latency. The game observer's recording
leases, original rendering and real output transitions require separate checks.

## Complete-artifact Linux SDR control

The [bounded game receipt](../qualification/providers/amd-sdr-pr54-game-2026-10-13.json)
uses the fresh artifact, including its exact separately selected framework.
FSR Quality + AMD FG ran in 3840×2160 SDR, resized through the native Video menu
to a 3200×1800 window and returned to 4K borderless. Generation resumed with
current 2560×1440 / 2134×1200 / 2560×1440 guides; consecutive admitted intervals
recorded 60 real and 60 generated callbacks without SDK faults/errors/warnings.
The rebuilt FG control was available without HDR. Pause/menu generation stopped.

The actual shipping process exited 0 after a normal menu quit, with scene leases
retired and shared SDK retirement returning 0. Original mod files, own settings,
renderer configuration and calibration INI were restored. This is one bounded
complete-artifact SDR control, not a performance, long-session or Windows pass.

## Complete-artifact Linux HDR control

The [HDR game receipt](../qualification/providers/amd-hdr-pr54-game-2026-10-13.json)
uses the same full artifact with the existing RenoDX HDR addon present. Startup
reported 3840×2160 PQ/BT.2020. AMD generation remained active through FSR Quality,
Native at 100%, and DLSS Quality selected through the native Video page. Fresh
full-resolution or 1440p guides matched each route; the NVIDIA feature was created
and reached the successful-evaluation acknowledgement. SDK faults/errors/warnings
were zero. Normal quit exited 0, with scene and shared SDK retirement completed.
Original files and configuration were restored after the control.

A [third complete-artifact control](../qualification/providers/amd-hdr-pr54-repeat-2026-10-13.json)
repeated HDR FSR Quality + AMD FG and exited 0 with completed retirement and
restoration. The three held-process normal quits took about 3.56–4.38 seconds
from confirmation to process disappearance. This bounded series retains the
prior historical failures; it does not establish their cause or prove every
shutdown/lifecycle case is fixed.

The [installer verification](../qualification/providers/amd-pr54-installers-2026-10-13.json)
checks the downloaded Windows-built installer archive and both visible platform
folders. All 17 own payload hashes and the captured manifests/dependency locks
match. This does not qualify Windows GUI execution or active Radeon features.

## Native Windows/Radeon qualification

**Latest development input:** `windows-candidate-current.json` now selects
`0.4.0-amd.dev.8`, run 38333151314 / artifact 11783652643. The
[rebuilt benchmark](AMD_REBUILT_BENCHMARK_2026-10-13.md) retains all thirty SDR/HDR
captures, the failed sampler-assisted SDR exit and separate no-sampler controls.
Neither Linux-wide nor Windows/Radeon qualification is marked passed. The
[complete NVIDIA regression](AMD_NVIDIA_REGRESSION_2026-10-13.md#complete-dev8-regression)
passed bounded preset/status checks but hung on shutdown without a sampler. Installer
generation uses this newer receipt; historical dev.7 results remain below.

The pinned `0.4.0-amd.dev.7` artifact predates the later permanent NVIDIA FG
restart-status correction. Keep it as the reference for the results above;
do not use it to qualify current corrected source. See the
[NVIDIA regression and correction](AMD_NVIDIA_REGRESSION_2026-10-13.md) for
the failed gate, guide-only retests and separately staged complete rebuild.
The [HDR benchmark](AMD_HDR_BENCHMARK_2026-10-13.md) retains its transient
generation pause and distinguishes SDK totals from physical display FPS.

The later [SDR benchmark](AMD_SDR_BENCHMARK_2026-10-13.md) passed its registered
numeric gates but hung after the final retained-Off menu quit. Candidate
qualification is therefore incomplete despite the three earlier normal exits.
The failure and authorized forced termination are retained separately; the
temporary sampler's involvement is under isolation, not established causation.

Use this candidate's installer and dependency lock. Follow the preparation and
required runs in [the Windows checklist](AMD_WINDOWS_CANDIDATE_2026-10-12.md),
using this artifact's pins rather than its historical PR52 pins. Additionally:

1. Run SDR first; require actual sRGB output and active AMD generation without
   the NVIDIA-only HDR requirement. Check HUD edges, foliage and moving objects.
2. Select native HDR only on a supporting output and restart as prompted.
   Require matching PQ output, current guides and no colour corruption. Live
   transfer-function changes are isolated-host evidence, not a game UI claim.
3. Change output resolution/window mode, then restore it. Require generation to
   suspend during invalid output and resume only with matching current resources.
4. Repeat normal quits with the actual shipping process handle held through exit.
   Record a failure even if the window closes or SDK retirement reports success.
5. Test supported Radeon Anti-Lag 2 separately from unsupported-adapter refusal.
   Record actual driver activation; do not infer a latency benefit from a row.

Active Windows/Radeon, wider lifecycle and performance qualification remain open.


## Exact compiled installer engine

The [installer engine receipt](../qualification/providers/amd-pr54-exact-installer-engine-2026-10-13.json)
uses the downloaded Linux installer core assembly, current dev.8 manifest and
visible payload directly. In an isolated fixture with the verified executable
copy and complete recognized external dependency sets (including Loader 2.3),
fresh install, repair, modified-file uninstall refusal, uninstall and reinstall
passed. All 17 own payload hashes matched; refusal changed no fixture files.
External framework/loader/provider files and synthetic preservation witnesses
remained intact. Uninstall removed the mod-owned DLSS runtime as documented and
restored the fixture renderer configuration exactly. Overrides started Off.

The fixture used synthetic save/account witnesses; real saves were not read or
copied. The shipped GUI, native Windows filesystem behavior and active Radeon
features remain separate qualification gates.


The [Linux GUI startup receipt](../qualification/providers/amd-pr54-linux-installer-startup-2026-10-13.json)
records a successful self-contained launch and rendered game-selection page,
including detection of the supported game. No GUI installation was performed.
The executable needed its execute permission restored after artifact extraction;
installation instructions now include that step. Windows GUI execution remains
unqualified.
