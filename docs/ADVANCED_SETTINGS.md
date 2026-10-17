# Advanced settings

Open **Mods → MCD2 Graphics → Settings** for the advanced page. Everyday
upscaler, quality, render-scale, frame-generation, HDR and latency controls
remain under **Settings → Video**.

## DLSS SR model hint

**0** uses the NVIDIA runtime default. Values **1–255** are experimental numeric
hints, independent of the Quality/Balanced/Performance render-resolution presets.
A runtime can ignore or reject a hint. The displayed request is not proof of the
model actually selected. Restart after changing it; use 0 to return to defaults.

Blueprint Loader saves the choice. On first use, an existing valid
`[DLSS] ModelPreset` in `MCD2Graphics/DependencyOverrides.ini` is adopted only
when the Loader has no saved value. Subsequent changes update that single INI
key through the native settings worker and preserve other overrides.

**Reset mod** resets the hint to 0. The page reports whether the native worker
acknowledged the request and whether it differs from the startup hint.

## Runtime information

The page displays version metadata from the exact loaded DLSS SR, Streamline,
FSR SR and FSR FG modules. A component that has not loaded is labelled
**Not loaded**. A loaded component without readable version metadata is labelled
**Version unavailable**. These values do not establish a selected model or
hardware support.

No FG model selector is exposed: the pinned public DLSS-G interface does not
provide a verified selector for that control.
