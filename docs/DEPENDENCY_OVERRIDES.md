# Trying other dependency versions

The candidate installer tests Blueprint Loader 2.2. Published releases retain
their own dependency pins. Shared dependencies are downloaded separately.

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
