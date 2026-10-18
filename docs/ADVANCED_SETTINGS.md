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

Blueprint Loader saves the choice. On first use, an existing valid
`[DLSS] ModelPreset` in `MCD2Graphics/DependencyOverrides.ini` is adopted only
when the Loader has no saved value. Subsequent changes update that single INI
key through the native settings worker and preserve other overrides.

Existing numeric choices retain their meaning. **Custom (manual override)**
represents an existing value outside the named presets; the menu does not create
new numeric presets. Select a named preset or Auto to replace it.

**Reset mod** restores Auto. The page reports whether the native worker saved
the selection and whether a restart is needed.

## Runtime information

The page displays version metadata from the exact loaded DLSS SR, Streamline,
FSR SR and FSR FG modules. A component that has not loaded is labelled
**Not loaded**. A loaded component without readable version metadata is labelled
**Version unavailable**. These values do not establish a selected model or
hardware support.

No FG model selector is exposed: the pinned public DLSS-G interface does not
provide a verified selector for that control.
