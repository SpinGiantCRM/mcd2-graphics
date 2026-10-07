# MCD2 Graphics — DLSS, DLAA, HDR and Reflex

[Download](https://github.com/SpinGiantCRM/mcd2-graphics/releases) · [Nexus page](https://www.nexusmods.com/minecraftdungeons2/mods/87) · [Roadmap](ROADMAP.md) · [Contributing](CONTRIBUTING.md) · [Validation](docs/VALIDATION.md)

DLAA, DLSS Super Resolution, native HDR controls and supported NVIDIA Reflex in Minecraft Dungeons II's Video menu, with a guided installer.

**Experimental prerelease: [`0.2.0-rc.1`](https://github.com/SpinGiantCRM/mcd2-graphics/releases/tag/v0.2.0-rc.1). Guided Windows/Linux installation, native HDR controls and supported Reflex. Windows/AMD qualification is bounded; Windows NVIDIA and Windows HDR output remain untested. [Release notes](docs/RELEASE_NOTES.md). Earlier preview tags and assets remain frozen.**

**Development branch:** `0.3.0-preview.1` adds Frame Generation and dependency overrides. It is held for qualification, not released. See the [candidate guide](docs/FG_CANDIDATE.md) and [Windows checklist](docs/WINDOWS_QUALIFICATION_CHECKLIST.md). The controls and release results below describe published `0.2.0-rc.1`.

The mod does not force offline mode. Online/co-op compatibility is unverified; offline play is safer. Follow the publisher's terms. See [online use](docs/ONLINE_USE.md).

**NOT AN OFFICIAL MINECRAFT PRODUCT. NOT APPROVED BY OR ASSOCIATED WITH MOJANG OR MICROSOFT.**

## Controls

**Settings → Video → Reconstruction** is available from the main menu and pause menu.

- **Upscaler:** Native or NVIDIA DLSS.
- **DLSS Preset:** DLAA, Quality, Balanced, Performance, Ultra Performance, Custom.
- **Render Scale:** 1% steps up to 100%; the minimum follows output resolution. At 4K it is 17%. 100% selects DLAA.

Presets and scale stay synchronized. Quality displays 67% and uses exactly two-thirds internally; Ultra Performance displays 33% and uses one-third. Other scales select Custom. Changes are saved immediately and applied after a safe rendering-history reset. The main menu remains Native; saved DLSS selections activate in gameplay. Resume gameplay if a change is pending. Unsupported presets are skipped. Native retains your selected DLSS preset for later.

HDR Output and its three calibration controls appear directly after Brightness. Calibration stays visible but disabled while Off. Native HDR settings and RenoDX processing apply together after the restart requested by the help panel.

NVIDIA Reflex appears beside FPS Limit only when the integration reports low-latency support. It offers Off, On and On + Boost; measurement markers and pre-input pacing continue in Off. This release has no FG or RR controls and does not add ray tracing.

## Requirements and installation

The pinned game build, full-addon ReShade, Blueprint Loader 2.0, RenoDX UE Extended and the pinned official NVIDIA runtimes. DLSS and Reflex require supported NVIDIA hardware; AMD uses Native fallback and hides unavailable Reflex. Dependencies are external downloads and are checked by hash. See [INSTALL.md](INSTALL.md) and [dependencies.lock.json](dependencies.lock.json). HDR requires an HDR-capable display and a working OS HDR output path; this mod does not convert SDR to HDR.

The payload includes only this mod's UI/metadata, native graphics and display/latency addons, own ABI bridge and motion/exposure conversion shader. The standalone installer also includes its permitted application framework/runtime. It contains no game assets, extracted game shader binaries, authentication replacements or third-party runtime DLLs. It does not solve platform sign-in or Gaming Services problems.

## Scope and limitations

This is a preview, not an official RenoDX, NVIDIA, Mojang or Microsoft release. The renderer uses a version-pinned ReShade adapter and game-specific resource layouts. Output is limited to 640×360 through 3840×2160. Split-screen, arbitrary aspect ratios, device recreation and long sessions are not qualified. The SR addon and conversion shader are unchanged from preview.2. The complete Update 2 payload has bounded Linux/NVIDIA and Windows/AMD qualification. Windows AMD passed installer/core removal checks, level travel, unsupported-feature fallback and three clean exits. Windows NVIDIA execution, Windows HDR output, physical controller, device recreation and long sessions remain unqualified. Earlier unexplained candidate shutdown exceptions remain documented. See the [Linux report](docs/UPDATE_2_LINUX_VALIDATION_2026-10-05.md), [Windows report](docs/UPDATE_2_WINDOWS_VALIDATION_2026-10-05.md) and [qualification decision](docs/UPDATE_2_RELEASE_GATE.md). Resolution changes can fall back to Native; select DLSS again after the new resolution settles.

Mouse and keyboard controls have been exercised. Controller input follows the game's normal focus/navigation and D-pad actions; a physical controller qualification remains outstanding. Changing input devices can move selection back to the first native setting. Native graphics-reset integration currently recognizes the English confirmation text.

HDR uses the separately maintained UE Extended addon. Peak brightness must be calibrated to your display. The supplied guidance does not claim an official HDR mastering reference or physical panel-luminance measurements.

See [Update 2 Linux evidence](docs/UPDATE_2_LINUX_VALIDATION_2026-10-05.md) and [qualification inputs](qualification/update2/README.md). See [validation](docs/VALIDATION.md) for release gates and [build notes](docs/BUILD.md) for source/dependency details. Report game build, GPU/driver, output resolution, preset and reproduction steps. Remove account information, party codes, local paths and authentication data from reports.

## Cross-platform maintenance

Before changing the installer, native addon or build recipe, read the
[Windows fix rationale and preservation requirements](docs/WINDOWS_COMPATIBILITY.md).
It maps every change to its cause, relevant Windows behavior and protecting checks.

## Local benchmark

This frozen preview.1 benchmark was not rerun for 0.2.0-rc.1 and does not measure Reflex. At 4K output on an RTX 4080 SUPER, the short stationary hub sample averaged 102.54 presentation FPS Native and 129.33 Quality. Custom graphics were held fixed; this is not stock Ultra or a latency benchmark. [Method, all presets and sanitised data](docs/BENCHMARK_2026-10-04.md).

## Credits

ReShade, RenoDX/UE Extended, NeoRune and Blueprint Loader contributors, and NVIDIA. See [third-party notices](THIRD_PARTY_NOTICES.md). Development used AI assistance with local build and runtime checks.
