# Advanced settings

Open **Mods → MCD2 Graphics → Settings** for model overrides and runtime
information. The page has **Upscaling**, **Frame Generation** and **Diagnostics**
tabs, using native top-page buttons and the initialized Settings strip’s
gradient/border brush. The mod owns the exclusive selection group; it does not
instantiate a screen-specific tab controller or bind the main Q/E actions.
The mod title, description, master toggle and rows occupy the left two thirds;
the native framed help pane occupies the right third from the top of the page.
Category selection uses a single native button group and clears on page exit.
The frame and background come from newly created native widget shells.

Blueprint Loader still owns the header and toggle. This mod temporarily narrows
only its own active page slot and mounts its help pane alongside it. It validates
the page structure first, adapts to the page geometry, and restores the original
padding on exit or unloading only while still owned. An unexpected Loader layout
or external padding change leaves the embedded layout available without
rewriting another mod’s page. Loader’s code and saved preferences are unchanged. Mouse, keyboard arrows
and controller directions change values. Popup tooltips are not used. Everyday upscaler, quality, render scale, sharpness, frame generation,
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
- **C — Performance** requests NVIDIA's lighter optical-flow path through the
  driver preset override. It is not an alias for A or B.

Restart after changes. The setting uses the documented
[`enableUserInterfaceRecomposition` option](https://github.com/NVIDIA-RTX/Streamline/blob/v2.14.1/include/sl_dlss_g.h).
[UI recomposition](https://github.com/NVIDIA-RTX/Streamline/blob/v2.14.1/docs/ProgrammingGuideDLSS_G.md#66-enabling-user-interface-recomposition)
requires both HUDless colour and UI buffers and can have a performance/memory
cost. NVIDIA App overrides can supersede the request. The menu labels it as a
**requested** preset because the pinned runtime does not report the final active
model. The corresponding DLL version appears in Diagnostics.

On Linux, explicit A/B/C choices are process-local DXVK-NVAPI DRS overrides
applied before Streamline initialization. Auto preserves external driver settings.
On Windows, C requires an existing NVIDIA profile dedicated to this executable;
base/global and shared profiles are refused. The prior two profile settings are
saved before any write. Returning to Auto/A/B and restarting restores settings
still owned by the mod; external changes are preserved. Select Auto and restart
before removing a Windows installation that used C. Diagnostics reports request
acceptance or the need for a dedicated profile. Acceptance is not active-model
readback. Windows profile behaviour still needs native Windows qualification.
See [NVIDIA's driver preset definitions](https://github.com/NVIDIA/nvapi/blob/main/NvApiDriverSettings.h).

## FSR versions

The Upscaling tab offers **Auto**, **3.1 — Analytical** and **4.1**.
On recognized Radeon RX 7000/9000 desktop GPUs, Auto and explicit 4.1 select
AMD's official 4.1.1 implementation from the official runtime. A compatible
AMD driver is still required; hardware recognition does not prove activation.
Other GPUs show **4.1 — INT8 (Experimental)**, and Auto uses 3.1.5. Their explicit
4.1 choice uses the separately acquired
[portable 4.1.1r11 build](https://github.com/daniel-h-0/bc250-fsr4-fork/tree/v4/dll).
This community implementation is not bundled in installer archives. Keep its
source, notices and exact external runtime identity. Unknown devices are treated
conservatively. Explicit implementation choices never silently substitute 3.1.
Restart after changing versions. See
[AMD's hardware requirements](https://gpuopen.com/learn/amd-fsr-sdk-2-3-blog/).

The Frame Generation tab independently offers **Auto**, **3.1 — Analytical**
and **4.0 — ML**, labelled unsupported on GPUs outside the recognized Radeon RX 9000 desktop families. Auto uses 3.1.6. ML FG requires a compatible GPU,
driver and device-reported 4.0.1 provider. Portable FSR 4 upscaling does not make
ML FG portable. Unsupported explicit selections are refused.

Diagnostics separates the implementation selected and validated by a context
from the loaded library's file version. The last verified implementation remains
readable in menus; it is not inferred from a saved provider choice. This page
does not invent DLSS-style letter presets or CNN/Transformer labels for FSR.
Selection uses the [FidelityFX API version queries](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/main/Kits/FidelityFX/api/include/ffx_api.h).
The experimental choices still require game, quality, performance and native
Windows/Radeon qualification before a support claim.

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

**Reset mod** restores all model/version overrides to Auto and turns diagnostics off.
**Reset graphics** resets the sharpness slider along with the Video settings.
Reflex and Anti-Lag 2 remain independently selectable with FG Off on supported
hardware; unsupported driver APIs are not represented as active features.

## Reusing the native menu widgets

Blueprint Loader and NeoRune remain unmodified. The reusable MIT-licensed
[`NativeSettingsWidgets.cs`](../src/ui/NativeSettingsWidgets.cs) helper creates
the game's settings tabs and detail card through Loader's existing custom-widget
API. It references installed game widgets and redistributes no game assets. Other
mod authors can reuse it; no Loader fork or Loader API extension is required.
