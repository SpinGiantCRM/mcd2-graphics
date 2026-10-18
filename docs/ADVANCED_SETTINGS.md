# Advanced settings

Open **Mods → MCD2 Graphics → Settings** for the advanced page. Everyday
upscaler, quality, render-scale, frame-generation, HDR and latency controls
remain under **Settings → Video**.

## DLSS Model Preset

**Auto** is the default. It lets NVIDIA choose the model for the DLSS quality
mode selected under **Video**. It does not force one model across DLAA, Quality,
Balanced, Performance and Ultra Performance. Current NVIDIA defaults use K for
DLAA/Quality/Balanced, M for Performance and L for Ultra Performance; these can
change with runtime updates. Custom render scales use the renderer's corresponding
DLSS quality mode. See [NVIDIA's preset definitions](https://github.com/NVIDIA/DLSS/blob/main/include/nvsdk_ngx_defs.h).

The named **Transformer** presets J/K and **Transformer 2nd Gen** presets L/M are
explicit overrides, independent of render scale. Choosing one requests that preset
across the DLSS quality modes. **CNN** presets E/F are legacy options deprecated
by NVIDIA and may be ignored by newer runtimes. The selected preset is a request,
not proof of the model actually running. Restart after changing it; return to
**Auto** to restore NVIDIA's quality-dependent selection.

The selector lists **Auto**, then **E, F, J, K, L, M**, followed by Custom.
Changing the display order preserves previously saved preset choices.

Blueprint Loader saves the choice. On first use, an existing valid
`[DLSS] ModelPreset` in `MCD2Graphics/DependencyOverrides.ini` is adopted only
when the Loader has no saved value. Subsequent changes update that single INI
key through the native settings worker and preserve other overrides.

Existing numeric choices retain their meaning. **Custom (manual override)**
represents an existing value outside the named presets; the menu does not create
new numeric presets. Select a named preset or Auto to replace it.

**Reset mod** restores Auto. The page reports whether the native worker saved
the selection and whether a restart is needed.

## Diagnostics

Enable **Show Runtime Diagnostics** to display loaded versions, the selected
upscaler/render scale and the existing frame-generation runtime status. It is
off by default and is saved by Blueprint Loader. Reset mod turns it off.

Version metadata comes from the exact loaded DLSS Super Resolution, DLSS Frame
Generation, Streamline, FSR Upscaling and FSR Frame Generation modules.
A component that has not loaded is labelled
**Not loaded**. A loaded component without readable version metadata is labelled
**Version unavailable**. These values do not establish a selected model or
hardware support.

The selected DLSS preset is kept separate from the active preset. Active DLSS
SR and FG presets display **Not reported** because the current integration has
no verified readback for them. A saved request, successful evaluation or DLL
version is not an active-model readback. FG activation status is also distinct
from measuring frames actually displayed by the monitor.

## Provider-specific controls

The same naming rules apply to each provider: quality modes, model presets,
runtime versions and frame multipliers are separate concepts. Auto retains the
provider default; named model choices are explicit overrides.

DLSS FG does not expose an SR-style letter-preset setter in the pinned
[Streamline interface](https://github.com/NVIDIA-RTX/Streamline/blob/v2.14.1/include/sl_dlss_g.h).
No nonfunctional FG preset selector is shown.

AMD FSR quality modes use AMD's names. Its implementation versions are distinct
from DLSS letter presets and the loaded DLL's version. The
[FidelityFX API](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/main/Kits/FidelityFX/api/include/ffx_api.h)
can enumerate implementation IDs/names for a device and query the implementation
actually used by a context. Such a selector must use those device-reported
choices; this page currently reports loaded FSR DLL versions, not a context's
implementation ID or a CNN/Transformer classification.
