# Multi Frame Generation integration

**Released 0.4.0-amd.dev.25 uses one FG provider at 2x.** MFG and Game Pass are
unreleased PR60 work based on the FSR build, integrating contributions from
[PR55](https://github.com/SpinGiantCRM/mcd2-graphics/pull/55) and
[PR57](https://github.com/SpinGiantCRM/mcd2-graphics/pull/57).

## Candidate modes

**Mods → MCD2 Graphics → Frame Generation → Mode / Multiplier** selects:

- **Standard:** one generated frame per real frame (2x), using the supported
  NVIDIA or AMD provider selected in Video.
- **Native MFG:** fixed NVIDIA output counts 2x–6x, bounded by the actual
  Streamline device capability. GPU model names never grant a count.
- **Software MFG:** external FSR-based headless generation at supported 2x–6x
  counts, independently admitted on the actual device.
- **Mixed MFG:** one caller-owned NVIDIA midpoint plus software intermediate
  frames at 4x or 6x, from the same pair of real frames.

Software and mixed modes require DLSS or FSR SR guide producers in this
candidate. Their capability masks do not authorize higher native counts. Preset
A/B/C remains independent of multiplier. Dynamic adaptation is not enabled.
One final presenter and one compatible latency owner coordinate each batch.
Software/mixed presentation fits the multiplier to the output refresh through
bounded backpressure. Higher counts can therefore lower the real rendering rate;
Diagnostics shows this scheduled limit separately from measured FPS. Native Off
removes this custom pacing. The game's saved FPS/VSync settings are preserved.

The schema-5 transport assigns strategy 3 to Software MFG, retaining values
0 Standard, 1 Native MFG and 2 Mixed MFG. Old readers that do not understand the
new strategy refuse it. Legacy FrameMode 0–4 still maps to 2x–6x. Checksums,
session, authority revision, world, output and real-frame identities remain
required. See [compute/presenter work and remaining gates](MFG_COMPUTE_TESTING.md).

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
