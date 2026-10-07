# MCD2 Graphics — DLSS, DLAA, Frame Generation, HDR and Reflex

[Download](https://github.com/SpinGiantCRM/mcd2-graphics/releases) · [Nexus page](https://www.nexusmods.com/minecraftdungeons2/mods/87) · [Roadmap](ROADMAP.md) · [Contributing](CONTRIBUTING.md) · [Validation](docs/VALIDATION.md)

DLAA, DLSS Super Resolution, Frame Generation, native HDR controls and supported NVIDIA Reflex in Minecraft Dungeons II's Video menu, with a guided installer.

**Experimental prerelease: [`0.3.0-preview.1`](https://github.com/SpinGiantCRM/mcd2-graphics/releases/tag/v0.3.0-preview.1). Adds Frame Generation, DLSS foliage improvements, Linux Reflex pacing fixes and dependency overrides. [Release notes](docs/RELEASE_NOTES.md). Earlier preview tags and assets remain frozen.**

The mod does not force offline mode. Online/co-op compatibility is unverified; offline play is safer. Follow the publisher's terms. See [online use](docs/ONLINE_USE.md).

**NOT AN OFFICIAL MINECRAFT PRODUCT. NOT APPROVED BY OR ASSOCIATED WITH MOJANG OR MICROSOFT.**

## Controls

**Settings → Video → Reconstruction** is available from the main menu and pause menu.

- **Upscaler:** Native or NVIDIA DLSS.
- **DLSS Preset:** DLAA, Quality, Balanced, Performance, Ultra Performance, Custom.
- **Render Scale:** 1% steps up to 100%; the minimum follows output resolution. At 4K it is 17%. 100% selects DLAA.

Presets and scale stay synchronized. Quality displays 67% and uses exactly two-thirds internally; Ultra Performance displays 33% and uses one-third. Other scales select Custom. Changes are saved immediately and applied after a safe rendering-history reset. The main menu remains Native; saved DLSS selections activate in gameplay. Resume gameplay if a change is pending. Unsupported presets are skipped. Native retains your selected DLSS preset for later.

HDR Output and its three calibration controls appear directly after Brightness. Calibration stays visible but disabled while Off. Native HDR settings and RenoDX processing apply together after the restart requested by the help panel.

NVIDIA Reflex appears beside FPS Limit only when the integration reports low-latency support. It offers Off, On and On + Boost; measurement markers and pre-input pacing continue in Off. Frame Generation offers Off / On and inserts one generated frame per rendered frame. It currently requires active NVIDIA DLSS/DLAA, native HDR and the separate patched ReShade framework; restart when requested. Ray Reconstruction and alternative upscalers are not included.

## Requirements and installation

The supported Steam build, the separate patched ReShade framework, Blueprint Loader **2.0+**, RenoDX UE Extended and official NVIDIA runtimes. Complete Loader 2.0, 2.2 and 2.3 sets are recognized. Each dependency has an explicit, default-Off untested-version override; it cannot supply missing APIs or unsupported hardware. See [installation](INSTALL.md), [the ReShade download and explanation](docs/FG_CANDIDATE.md#why-this-reshade-build-is-required) and [dependency overrides](docs/DEPENDENCY_OVERRIDES.md).

Extract the entire Windows/Linux installer ZIP and run the included application; no separate Python or .NET installation is needed. The twelve owned files are visible under `payload/`. There is no nested payload archive, self-extraction or background downloader. The credited ReShade dependency is a separate download; NVIDIA runtimes remain external. Game assets, authentication replacements and player data are excluded.

## Scope and limitations

Bounded [Linux/NVIDIA qualification](docs/LINUX_FINAL_QUALIFICATION_2026-10-07.md) and [Windows/AMD installer and fallback qualification](docs/WINDOWS_FULL_QUALIFICATION_2026-10-07.md) use the exact released files. Native Windows NVIDIA active DLSS/FG/Reflex, Windows HDR output, physical controllers, long sessions and device loss remain unqualified. Windows clean exits sometimes took about 85–86 seconds; historical shutdown failures remain documented. This release does not claim a universal crash repair or measured Reflex latency benefit.

Steam is the verified edition. Xbox app / Microsoft Store PC compatibility remains unverified. Output is limited to 640×360–3840×2160. Main-menu rendering stays Native; saved DLSS activates in gameplay. Resolution changes may fall back to Native; reselect DLSS after the new size settles. Split-screen and unusual aspect ratios remain unqualified.

Some foliage/interpolation artifacts remain possible. HDR uses the separately maintained UE Extended addon; calibrate peak brightness to your display. No official HDR master or physical panel-luminance measurement is claimed. Native graphics reset currently recognizes the English confirmation text.

Report your store, game/mod version, GPU/driver, preset and reproduction steps. Remove personal details, party codes, private paths and authentication data; do not upload game executables or saves.

## Cross-platform maintenance

Before changing the installer, native addon or build recipe, read the
[Windows fix rationale and preservation requirements](docs/WINDOWS_COMPATIBILITY.md).
It maps every change to its cause, relevant Windows behavior and protecting checks.

## Local benchmark

At 4K on an RTX 4080 SUPER, three 30-second stationary hub samples per mode gave:

| Mode | FG Off: rendered FPS | FG On: rendered FPS | FG On: SDK presentation FPS |
| --- | ---: | ---: | ---: |
| DLAA | 81.80 | 61.06 | 122.11 |
| Quality | 122.74 | 83.76 | 167.52 |

FG Off was about 2% below a fresh previous-release control in this bounded scene. Generated frames add presentation rate while reducing base FPS; they do not increase simulation rate. SDK counts are not physical display-pacing or latency measurements. [Method and all runs](docs/BENCHMARK_2026-10-07.md). Earlier benchmark tables remain historical.

## Credits

ReShade, RenoDX/UE Extended, NeoRune and Blueprint Loader contributors, and NVIDIA. See [third-party notices](THIRD_PARTY_NOTICES.md). Development used AI assistance with local build and runtime checks.
