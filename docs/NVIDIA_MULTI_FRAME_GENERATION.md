# Multi Frame Generation integration

**Released 0.4.0-amd.dev.25 uses one FG provider at 2x.** MFG and Game Pass are
unreleased PR60 work based on the FSR build, integrating contributions from
[PR55](https://github.com/SpinGiantCRM/mcd2-graphics/pull/55) and
[PR57](https://github.com/SpinGiantCRM/mcd2-graphics/pull/57).

## Candidate controls

**Mods → MCD2 Graphics → Frame Generation** has separate **DLSS Generated Frames**
and **FSR Generated Frames** rows. The total includes one rendered frame:
**2 DLSS + 3 FSR + 1 rendered = 6x**. Between one and five generated frames can
be configured across the providers. Ordinary FG remains 2x by default through
the Video menu; the extra page has no Standard/Native/Software/Mixed selector.

DLSS requires a supported NVIDIA device; FSR supports compatible AMD and NVIDIA
devices. Pure DLSS uses Streamline's reported maximum. Higher FSR counts and mixed
DLSS/FSR use the separately acquired headless runtime and require DLSS or FSR
upscaling guides. Both runtimes must approve a mixed split. GPU names, an FSR
capability or an optional helper alone never authorize higher DLSS counts.
Unsupported choices are skipped without silently reducing a saved request.

## Frame Order

**Automatic** is the default in the extra settings. The **Frame Order** row also
lists every supported manual DLSS/FSR sequence for the selected counts. The side
pane shows the sequence between two rendered frames. Counts and the FG On/Off
switch stay unchanged when selecting an order. Changing a count restores Automatic.

Automatic keeps equally spaced presentation slots and balances the gaps between
both providers' frames, subject to actual SDK limits. Among equally balanced
orders, it prefers the smaller native interpolation grid and a stable tie-break.
For **2 DLSS + 3 FSR**, it chooses:

**Real → FSR → DLSS → FSR → DLSS → FSR → Real**

Custom order chooses which provider generates each temporal slot. It does not
shuffle already generated images, which would show motion out of sequence.
Every image uses the same two rendered frames and its own time fraction. Some
orders require a larger native interpolation grid; unsupported orders are
unavailable, without silently substituting a different requested order.

This is a pacing-based default, not a measured image-quality winner. The equal
presentation-spacing goal follows [AMD's frame-pacing guidance](https://gpuopen.com/manuals/fidelityfx_sdk/techniques/frame-interpolation-swap-chain/#frame-pacing-and-presentation).
The requirement to respect reported native counts follows [NVIDIA's DLSS-G guide](https://github.com/NVIDIA-RTX/Streamline/blob/v2.14.1/docs/ProgrammingGuideDLSS_G.md).
The balanced mixed-provider order is this mod's policy; neither source prescribes
a universal mixed-provider sequence. Scene-dependent quality comparison remains
part of later image analysis.

Preset A/B/C remains independent of the counts. One final presenter and compatible
latency owner coordinate the batch. Generated images never become rendered-frame
history. Output refresh and bounded backpressure can lower the real rendering
rate at higher counts; Diagnostics distinguishes the scheduled limit from a
measured FPS. The saved FPS/VSync settings are preserved. Turn FG Off in Video.

Schema7 keeps the 128-byte record and encodes the internal strategy, separate
DLSS/FSR counts and requested order atomically. Schema5/6 settings retain their
identity until edited; new edits preserve unrelated settings and use schema7. Old readers refuse the
new format. Checksums, session/revision/world, source pair and queue completion
remain required. See [compute/presenter checks](MFG_COMPUTE_TESTING.md).

## Qualification and external inputs

After custom startup, cached NVIDIA support permits a baseline 2x native
request for the next restart. The native maximum remains unknown until
Streamline reports it; software capabilities never authorize higher native
counts. The menu shows this restart requirement separately from active support.

The native path retains its restart requirement after releasing the NVIDIA
feature. Custom Off recovery uses engine-confirmed presenter switching. An
accepted setting or successful Present call is not a measured display multiplier.

The contributor tested higher native counts on an RTX 4080 with an external Ada
helper. Those [historical results](NVIDIA_MFG_TESTING.md) do not qualify this
integrated binary, ordinary RTX 40 support or native RTX 50 hardware. Optional
helpers stay external; production never forces counts or edits a global profile.
The external software runtime is also acquired separately, hash-checked and
credited, not bundled as an owned mod binary.

The qualification sequence is complete Linux/NVIDIA testing, then the available
Windows/Radeon laptop startup check with MFG Off. That laptop cannot qualify MFG.
Native Windows Game Pass and Linux/Xodus require separate tests. Display pacing,
HDR, focus, travel, resize, Off recovery and normal exit must be checked against
the same complete package before support claims. **Benchmarks remain on hold.**
