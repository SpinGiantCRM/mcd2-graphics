# Trying other dependency versions

The candidate has a minimum API policy and an explicit override for every
external dependency. Published releases retain their own requirements.
Shared dependencies are downloaded separately.

| Dependency | Requirement | Newer versions |
| --- | --- | --- |
| Blueprint Loader | **2.0+**; ModInfo/settings API, no popup API | Official 2.0, 2.2 and 2.3 sets recognized; other sets use the override |
| NVIDIA DLSS runtime | **310.9.1+** NGX SR baseline | Use the override for an untested DLL; optional model hint below |
| Streamline | **2.14.1+** production x64 module baseline | Use the override for an untested complete module set |
| ReShade | Native D3D12 PR435 access and successful-Reset lifetime API | Use the override; a newer stock release may lack required interfaces |
| RenoDX UE Extended | Required shader/configuration contract | Use the override for an untested nightly |

A minimum states the API baseline, not a guarantee about every future release.
The default installer recognizes complete, hash-verified release sets. It never
infers compatibility from an archive filename, accepts mixed versions, or forces
a recognized installed version to upgrade. All five dependencies support an
explicit untested-version override. Loader 1.x is below the supported API
baseline; its known files can be backed up during an upgrade. The generic
recognized-release mechanism also works for single-file dependencies such as DLSS.

If popups are adopted later, raise the Loader minimum to **2.2+**. Loader 2.3
adds immediate settings-page refresh and text/widget indentation according to
[its official changelog](https://www.nexusmods.com/minecraftdungeons2/mods/2?tab=files).
These additions do not require a new graphics UI build. The existing Windows
qualification used Loader 2.2; recognizing 2.3 is not Windows runtime qualification.

For Blueprint Loader, ReShade, RenoDX, DLSS or Streamline, select **Try an
untested dependency version** beside that requirement. Select an official
download, or explicitly select **Use installed version** to keep the files
already installed. Overrides start disabled each time the installer opens.

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

See [why the patched ReShade framework is required](FG_CANDIDATE.md#why-this-reshade-build-is-required)
for its device/queue access, reset-lifetime protection and direct ZIP download.

## DLSS model hint

To request a model from the installed NVIDIA DLSS runtime, create
`Dungeons/Binaries/Win64/MCD2Graphics/DependencyOverrides.ini`:

```ini
[DLSS]
ModelPreset=0
```

Zero (or an absent file) uses the runtime's default model. A nonzero integer
from 1 to 255 sends that model hint for every DLSS/DLAA render-scale preset.
Use only values documented by NVIDIA for the DLL you installed. An unknown
hint may be ignored or rejected by that runtime. Restart after editing.
This setting does not change the in-game Quality/Balanced choices or render
scale. Delete the file to restore default model selection.

The file is a user preference and is not installed or removed by the installer.
It cannot enable an unsupported GPU or an unimplemented feature.

## Wine Reflex pacing

The candidate applies an NVIDIA Wine presentation compatibility setting before
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
