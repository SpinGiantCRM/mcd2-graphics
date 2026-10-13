# AMD generated-frame image checks

The [numeric receipt](../qualification/providers/amd-pr54-labelled-images-2026-10-14.json)
records 192 cropped output images: 96 SDK-labelled real/generated pairs on
Linux / RTX 4080 SUPER. The base was `0.4.0-amd.dev.8`, with one private AMD
bridge substitution. Production runtime source and released files were unchanged.

| Case | Images | Complete pairs | Consecutive triplets |
|---|---:|---:|---:|
| FSR Quality + AMD FG, world | 48 | 24 | 18 |
| FSR Quality + AMD FG, HUD | 48 | 24 | 18 |
| FSR Quality + AMD FG, movement window | 48 | 24 | 18 |
| DLSS Quality + AMD FG, world | 48 | 24 | 18 |

## Method and observations

Labels came directly from AMD's presentation callback `frameID` and
`isGeneratedFrame`. Copies captured the composed output. Generation and UI
composition were disabled, and SDK completion succeeded before CPU mapping,
following [AMD's pinned integration guide](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/60f4ea81909200d8542eca14dccb2628b763a9a3/Kits/FidelityFX/docs/techniques/frame-interpolation-api.md).
The ordinary Off callback was restored afterward.

Every image mapped successfully; every pair matched its SDK and engine frame
identities. Captures reported PQ/BT.2020 with no capture refusals. Sampled world
and HUD triplets showed coherent geometry, colour and legible HUD elements.
The HUD crop's maximum channel signal was 202.915 nits in both frame types.
The highest world channel signal was 416.403 nits in both frame types. These
are decoded signal values, not measured panel luminance or an HDR accuracy score.

Screenshots confirm character displacement during the pointer test. Collection
used short consecutive bursts separated by gaps; it is not continuous footage
or comprehensive temporal-quality qualification. Raw images and logs remain private.

## Limits

The held game process exited with code 0 after the interrupted run. Its initiating
input and confirmation timing were not recorded. Original files and own settings
were restored byte for byte. This control does not clear the earlier production
shutdown hang or qualify the production archive, physical FPS/latency,
Windows/Radeon execution or active Anti-Lag 2.

The standalone probe needed explicit native Proton DXGI/D3D12/D3D12Core DLL
selection. Earlier device-creation failures were retained. An initial diagnostic
capture also omitted one Off callback count; restoring the callback after the
GPU drain passed the full synthetic Off/On/Off and retirement checks. That
correction affected only the private capture build.
