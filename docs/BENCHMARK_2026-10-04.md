# Preview 1: local 4K presentation benchmark

Measured 4 October 2026 on the unchanged `0.1.0-preview.1` payload. This is one stationary solo hub scene, with the same camera and settings across modes.

| Mode | Mean presentation FPS | Mean interval (ms) | Mean run p95 (ms) | Mean run p99 (ms) |
| --- | ---: | ---: | ---: | ---: |
| Native | 102.54 | 9.75 | 11.11 | 11.62 |
| DLAA | 89.90 | 11.12 | 12.69 | 13.40 |
| Quality | 129.33 | 7.73 | 8.94 | 9.51 |
| Balanced | 139.70 | 7.16 | 8.25 | 8.57 |
| Performance | 139.29 | 7.18 | 8.35 | 8.92 |

Quality was approximately 26% faster than Native in this scene. Balanced and Performance were approximately 36% faster. Performance did not outperform Balanced here. DLAA increased rendering cost. These results do not predict every dungeon or hardware configuration.

## Hardware and settings

- RTX 4080 SUPER, NVIDIA driver 615.71.09; Ryzen 7 7800X3D.
- CachyOS KDE Wayland, Proton Experimental, Gamescope HDR output; inverse tone mapping disabled.
- Steam game build 25647713; 3840×2160 output, VSync off, frame limit off, windowed fullscreen.
- Custom graphics: quality level 3 except Shadows and Effects at 2. Existing short-range Lumen AO override disabled that pass. This is **not stock Ultra**. The same graphics/HDR settings were retained for every mode.
- Actual DLSS inputs: DLAA 3840×2160; Quality 2560×1440; Balanced 2228×1256; Performance 1920×1080.

## Method

A separate temporary sampler recorded CPU high-resolution timestamps at ReShade `finish_present`. Each run recorded 1,200 timestamps (1,199 intervals). Three runs per DLSS mode; Native was measured in three runs before and three after the sequence. Native means differed by approximately 0.071%. Five seconds of warm-up followed menu changes; the mod's applied mode and feature dimensions were checked.

The sampler used no GPU queries, pixel readbacks or resource writes. Video recording and the Reflex/FG observer were absent during timing. Lightweight GPU utilisation/clock/power sampling ran every 500 ms. The mod's released files matched the frozen manifest; no production rendering changes were made for this benchmark.

FPS is the arithmetic mean of per-run `interval count / elapsed time`. Mean/p95/p99 columns average the corresponding run statistics; p99 is **not labelled a 1% low**. CPU presentation intervals are not isolated GPU timings, physical display refresh, click-to-photon latency or proof of Reflex. The sampler's overhead was not independently calibrated.

NPCs, fire and day/night lighting continued to animate. This is a short scene sample, not a fixed replay, quality comparison, long-session qualification or Windows benchmark. Raw HDR brightness cannot be judged from the SDR demonstration media.

## Reproduction and data

Use the same build, dependencies, output size, scene/camera and graphics settings; warm up each applied mode; record multiple runs and bracket the sequence with Native. Repeat in a dungeon before generalising the result. Do not test online enforcement.

[Sanitised run statistics and configuration](benchmark-2026-10-04.json) contain sample counts, dimensions and full graphics overrides without account data or private paths. `LastUserConfirmedResolution` in the configuration is a stale preference; active feature dimensions and output were 4K.
