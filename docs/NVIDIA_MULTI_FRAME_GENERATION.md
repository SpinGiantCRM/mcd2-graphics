# NVIDIA fixed Multi Frame Generation integration

This candidate integrates the Xbox and MFG contributions from
[PR55](https://github.com/SpinGiantCRM/mcd2-graphics/pull/55) and
[PR57](https://github.com/SpinGiantCRM/mcd2-graphics/pull/57) with the released FSR
architecture. It is not a qualified release.

**Mods → MCD2 Graphics → Frame Generation → DLSS FG Multiplier** selects fixed
2x–6x output. 2x requests one generated frame per rendered frame; 6x requests
five. Selection and SDK configuration require the actual device's reported
`numFramesToGenerateMax`, not a GPU-model whitelist. The initial default stays
2x. Unsupported saved modes remain visible and can be changed to a supported
mode or disabled. Multiplier selection is independent of DLSS FG preset A/B/C.

The same NVIDIA path can consume DLSS or FSR upscaling. The existing AMD FG path,
its capability cache, temporal-guide ownership and native-Off presenter recovery
remain separate. This is single-provider MFG. Combining two FG providers needs
another presentation/timeline backend and remains unavailable.

The transport keeps the 48-byte plain-C configuration and schema-5 settings.
Legacy FrameMode 0–4 maps to 2x–6x; omitted means 2x. Case-insensitive duplicate
properties are refused. Dynamic mode is not enabled and strategy value 3 is
rejected, preserving older schema-5 readers and rollback behavior.

Fresh startup with FG Off keeps the native presentation route. NVIDIA Off
releases its feature and retains the existing restart requirement for re-enable.
The contributor's retained-proxy live-toggle policy is not enabled by default.
An active configuration is not a measured output multiplier: actual SDK
presentation observations and diagnostics remain separate evidence.

## Existing evidence and remaining checks

The contributor's fixed higher-mode evidence used RTX 4080 with a separately
installed Ada unlocker. This does not establish ordinary RTX 40 support or
native RTX 50 qualification. No unlocker is installed or bundled here. Historical
contributor evidence is preserved in [NVIDIA_MFG_TESTING.md](NVIDIA_MFG_TESTING.md).
It predates this FSR integration and does not qualify the combined candidate.

Functional qualification must cover Steam and Game Pass on native Windows,
Steam on Linux, and Game Pass through Xodus independently. Dynamic adaptation,
combined-provider generation and performance claims remain deferred. Benchmarks
are on hold.

## Required provider-neutral follow-on work

The complete MFG goal includes both Radeon and NVIDIA, single-provider software
generation on compatible older hardware, supported native-provider paths, and
two providers contributing frames to the same timeline. Native support must
not remove supported software or mixed-provider choices on newer hardware.

[DLSS Enabler's author description](https://www.nexusmods.com/site/articles/237)
is the mixed-provider behavior reference: it assigns suitable intermediate
positions to separate generators and presents them on one timeline. This
candidate has not implemented that backend or qualified those combinations.

The next integration milestone must establish an arbitrary-phase software
backend and its supported interface, qualify it on both hardware tracks, then
coordinate compatible contributors through one presentation and latency owner.
Current two-frame analytical FSR support is not proof of software MFG. Keep the
existing refusal of unsupported combinations until their rendering backend is
implemented. See the [roadmap](../ROADMAP.md).
