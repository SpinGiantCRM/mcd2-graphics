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

## Native Windows/Radeon qualification

Use this candidate's installer and dependency lock. Follow the preparation and
required runs in [the Windows checklist](AMD_WINDOWS_CANDIDATE_2026-10-12.md),
using this artifact's pins rather than its historical PR52 pins. Additionally:

1. Run SDR first; require actual sRGB output and active AMD generation without
   the NVIDIA-only HDR requirement. Check HUD edges, foliage and moving objects.
2. Toggle native HDR only on a supporting output. Require matching PQ output,
   current guides, generation-context recreation and no colour corruption.
3. Change output resolution/window mode, then restore it. Require generation to
   suspend during invalid output and resume only with matching current resources.
4. Repeat normal quits with the actual shipping process handle held through exit.
   Record a failure even if the window closes or SDK retirement reports success.
5. Test supported Radeon Anti-Lag 2 separately from unsupported-adapter refusal.
   Record actual driver activation; do not infer a latency benefit from a row.

Active Windows/Radeon, wider lifecycle and performance qualification remain open.
