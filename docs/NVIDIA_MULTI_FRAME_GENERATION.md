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

Every generated image comes from the same two rendered frames. A uniform final
timeline assigns DLSS positions across that interval and fills other positions
with FSR. The native runtime must support the exact interpolation grid, which
can exceed the number of DLSS images selected for display. For 2 DLSS + 3 FSR,
DLSS supplies positions 2/6 and 4/6; FSR supplies 1/6, 3/6 and 5/6.

Preset A/B/C remains independent of the counts. One final presenter and compatible
latency owner coordinate the batch. Generated images never become rendered-frame
history. Output refresh and bounded backpressure can lower the real rendering
rate at higher counts; Diagnostics distinguishes the scheduled limit from a
measured FPS. The saved FPS/VSync settings are preserved. Turn FG Off in Video.

Schema6 keeps the 128-byte record and encodes the internal strategy and separate
DLSS/FSR counts atomically. Schema5 settings retain their identity until edited;
new edits preserve unrelated settings and use schema6. Old readers refuse the
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
