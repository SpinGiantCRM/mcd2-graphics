# Advanced settings

Open **Mods → MCD2 Graphics → Settings** for model overrides and runtime
information. Everyday upscaler, quality, render scale, sharpness, frame generation,
HDR and independent low-latency controls remain under **Settings → Video**.

## DLSS Model Preset

**Auto** lets NVIDIA select a model for the current DLSS quality mode. It does
not force one model across DLAA, Quality, Balanced, Performance and Ultra
Performance. Current defaults use K for DLAA/Quality/Balanced, M for Performance
and L for Ultra Performance; runtime updates can change these defaults.
Custom render scales use the renderer's corresponding DLSS quality mode.
See [NVIDIA's preset definitions](https://github.com/NVIDIA/DLSS/blob/main/include/nvsdk_ngx_defs.h).

The selector lists **Auto**, then **E, F, J, K, L, M**.
Letters come first: E/F are legacy CNN, J/K are Transformer and L/M are
second-generation Transformer. Native Video controls show help on hover or focus. Popup tooltips are not used
on the advanced page.

A named override requests that model across quality modes. Newer runtimes may
ignore legacy CNN presets. Restart after changing a model override. Return to
Auto to restore NVIDIA's quality-dependent selection. The saved choice is a
request; it cannot prove which model the driver ultimately selected.

Blueprint Loader saves the choice. On first use, an existing valid
`[DLSS] ModelPreset` in `MCD2Graphics/DependencyOverrides.ini` is adopted only
when the Loader has no saved value. Changes update that single INI key through
the native worker, preserving other overrides and unrelated settings. Unsupported legacy numeric model selections are reset
to Auto rather than exposed as a nonfunctional Custom choice.

## DLSS FG Preset

- **Auto** keeps Streamline's default UI recomposition setting.
- **A — Standard** requests the standard composition path.
- **B — UI Recomposition** requests separate interpolation of the scene and UI.

Restart after changes. The setting uses the documented
[`enableUserInterfaceRecomposition` option](https://github.com/NVIDIA-RTX/Streamline/blob/v2.14.1/include/sl_dlss_g.h).
[UI recomposition](https://github.com/NVIDIA-RTX/Streamline/blob/v2.14.1/docs/ProgrammingGuideDLSS_G.md#66-enabling-user-interface-recomposition)
requires both HUDless colour and UI buffers and can have a performance/memory
cost. NVIDIA App overrides can supersede the request. The menu labels it as a
**requested** preset because the pinned runtime does not report the final active
model. The corresponding DLL version appears in Diagnostics.

## FSR versions

Diagnostics separates **implementation version** from **library version**.
Implementation versions come from the context validated by the bridge: currently
FSR SR 3.1.5 and FSR FG 3.1.6. The last successfully evaluated/validated context version is retained for the
current process so it remains readable in menus; it is not inferred from a
saved provider choice. The loaded DLL may have a different
version. A component that is not loaded is labelled accordingly.

This page does not invent DLSS-style letter presets or CNN/Transformer labels
for FSR. Device-specific implementation selection must use the
[FidelityFX API's version queries](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/main/Kits/FidelityFX/api/include/ffx_api.h).

## Diagnostics

**Show Runtime Details** expands versions and preset/status information inside
the Diagnostics section. It is off by default so model controls remain compact.
The independent overlay control below does not expand the page.


**Show In-Game Diagnostics** enables a small, noninteractive overlay during
unpaused gameplay. It is off by default and saved by Blueprint Loader.

The overlay samples successful SR evaluations, provider-generated frame counters
and low-latency frame counters. It requires matching process/session, settings
revision and, for SR, the current world. The first sample does not establish
activity; stopped/reset counters and expired data cannot keep a feature marked
Active. Menus hide the overlay. Turning it off stops diagnostic-state publication
when no runtime-information page is open.

Model requests, verified activity, library versions and FSR implementation
versions remain distinct. The current APIs do not provide verified final DLSS
SR/FG model readback. Generated-frame counters also do not measure whether every
frame was scanned out by the monitor.

## Upscaling Sharpness

The **0–100%** slider sits directly below **Render Scale** in Video settings.
It controls the game's post-upscale sharpening for both DLSS and FSR; 0% disables
additional sharpening. Increasing it can emphasize halos or shimmering.
It does not change model selection or recover missing detail.

Only this sharpening stage is adjusted. The prior game value is restored when
returning to Native, leaving gameplay or unloading the mod. If another tool
changes the value while the mod owns it, the mod yields instead of overwriting
that change. Selecting a new sharpness value permits it to take control again.

**Reset mod** restores both model overrides to Auto and turns diagnostics off.
**Reset graphics** resets the sharpness slider along with the Video settings.
Reflex and Anti-Lag 2 remain independently selectable with FG Off on supported
hardware; unsupported driver APIs are not represented as active features.
