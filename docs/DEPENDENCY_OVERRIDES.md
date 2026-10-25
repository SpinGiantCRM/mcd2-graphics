# Trying other dependency versions

The released **0.4.0-amd.dev.25** installer uses separate, exact
[Windows and Linux dependency locks](DEV25_RELEASE_CANDIDATE.md#exact-packages-and-dependencies).
The root preview.1 lock is a historical build input, not this release's
installation baseline. All graphics dependencies are acquired separately.

| Dependency | Tested dev.25 input | Override limits |
| --- | --- | --- |
| Blueprint Loader | 2.6 Windows/Radeon; 2.7 Linux/NVIDIA | Required ModInfo/settings APIs remain mandatory |
| NVIDIA DLSS | 310.9.16 | Unsupported hardware and missing SDK entry points remain refused |
| Streamline | 2.14.16 complete module set | No mixed module versions or fabricated capability |
| ReShade | Separate dev.25 patched framework | A newer stock build can lack the required device/queue/reset interfaces |
| RenoDX UE Extended | 0.2026.1023.0826 | Preserve native HDR configuration and shader contracts |
| AMD FSR | SDK 2.3.0; analytical SR 3.1.5 and FG 3.1.6 exercised | ML/INT8 choices have separate runtime and hardware gates |

A minimum states the API baseline, not a guarantee about every future release.
The default installer recognizes complete, hash-verified release sets. It never
infers compatibility from an archive filename or treats a minimum API version as
qualification of a newer release. Untested-version overrides start disabled each
time the installer opens. Select the override beside the applicable requirement,
then choose an official download or **Use installed version** where available.
Keep required dependency files complete and mutually compatible.

The installer records the selected hashes and labels the version untested.
It still checks the supported game executable, this mod's payload, required
archive files, extraction limits and file ownership. Existing unknown files
need explicit acknowledgement before replacement. Shared dependency recovery
copies are retained; uninstall removes only recorded, unchanged private files.

An override permits an experiment; it does not establish compatibility. In
particular, Frame Generation needs the patched ReShade API. A newer standard
ReShade build may not provide it. Streamline still needs its required modules;
NVIDIA signature and hardware checks remain active. Revert to the tested
versions if startup or graphics fail.

See the [dev.25 dependency section](DEV25_RELEASE_CANDIDATE.md#exact-packages-and-dependencies) for the current framework download and provenance. The old preview.1 framework is not a substitute.

## DLSS model preset

Use **Mods → MCD2 Graphics → Upscaling → DLSS Model Preset** for normal changes.
Auto follows the quality mode; E/F/J/K/L/M are explicit overrides. The menu
preserves unrelated INI keys and adopts a valid existing override on first use.
See [advanced settings](ADVANCED_SETTINGS.md). The manual interface below is
retained for existing configurations; prefer the menu.

To request a model from the installed NVIDIA DLSS runtime, create
`Dungeons/Binaries/Win64/MCD2Graphics/DependencyOverrides.ini`:

```ini
[DLSS]
ModelPreset=0
```

Zero (or an absent key) uses the runtime's quality-dependent selection. A nonzero integer
from 1 to 255 sends that model hint for every DLSS/DLAA render-scale preset.
Use only values documented by NVIDIA for the DLL you installed. An unknown
hint may be ignored or rejected by that runtime. Restart after editing.
This setting does not change the in-game Quality/Balanced choices or render
scale. Return the menu to Auto to restore default selection; do not delete a file containing unrelated overrides.

The file is a user preference and is not installed or removed by the installer.
It cannot enable an unsupported GPU or an unimplemented feature.

## Wine Reflex pacing

This release applies an NVIDIA Wine presentation compatibility setting before
graphics device creation. It disables Vulkan's dynamic swapchain-mode switching
without forcing a present mode or changing the game's VSync/frame limit.
Windows and adapters without an initialized NVIDIA NVAPI are excluded.

To opt out for comparison, add this to `Dungeons/Binaries/Win64/FGBootstrap.ini`
and restart:

```ini
[Compatibility]
WineReflexPacing=0
```

The default is 1. Existing `VKD3D_DISABLE_EXTENSIONS` selections are preserved.
The tested improvement is with VSync Off; VSync On still showed greater interval
variation on this Proton/driver combination. This is not a measured input-latency
claim.
