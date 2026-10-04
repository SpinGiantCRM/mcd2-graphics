# MCD2 Graphics — experimental preview

[Download](https://github.com/SpinGiantCRM/mcd2-graphics/releases) · [Nexus page](https://www.nexusmods.com/minecraftdungeons2/mods/87) · [Roadmap](ROADMAP.md) · [Contributing](CONTRIBUTING.md) · [Validation](docs/VALIDATION.md)

DLAA and DLSS Super Resolution in Minecraft Dungeons II's own Video menu, alongside a documented RenoDX UE Extended HDR setup.

**Offline single-player only for this preview. Online/co-op permission and anti-cheat compatibility are unverified. This restriction does not guarantee compliance or account safety.** See [publisher rules and online use](docs/ONLINE_USE.md).

**NOT AN OFFICIAL MINECRAFT PRODUCT. NOT APPROVED BY OR ASSOCIATED WITH MOJANG OR MICROSOFT.**

## Controls

**Settings → Video → Reconstruction** is available from the main menu and pause menu.

- **Upscaler:** Native or NVIDIA DLSS.
- **DLSS Preset:** DLAA, Quality, Balanced, Performance, Ultra Performance, Custom.
- **Render Scale:** 1% steps up to 100%; the minimum follows output resolution. At 4K it is 17%. 100% selects DLAA.

Presets and scale stay synchronized. Quality displays 67% and uses exactly two-thirds internally; Ultra Performance displays 33% and uses one-third. Other scales select Custom. Changes are saved immediately and applied after a safe rendering-history reset. The main menu remains Native; saved DLSS selections activate in gameplay. Resume gameplay if a change is pending. Unsupported presets are skipped. Native retains your selected DLSS preset for later.

Ray Reconstruction, Frame Generation and Reflex are hidden until implemented. This release does not add ray tracing.

## Requirements and installation

An NVIDIA RTX GPU, the pinned game build, full-addon ReShade, Blueprint Loader and RenoDX UE Extended. Dependencies are external downloads and are checked by hash. See [INSTALL.md](INSTALL.md) and [dependencies.lock.json](dependencies.lock.json). HDR requires an HDR-capable display and a working OS HDR output path; this mod does not convert SDR to HDR.

The package includes only this mod's UI, native addon and motion/exposure conversion shader. It contains no game assets, extracted game shader binaries, authentication replacements or third-party runtime DLLs. It does not solve platform sign-in or Gaming Services problems.

## Scope and limitations

This is a preview, not an official RenoDX, NVIDIA, Mojang or Microsoft release. The renderer uses a version-pinned ReShade adapter and game-specific resource layouts. Output is limited to 640×360 through 3840×2160. Split-screen, arbitrary aspect ratios, device recreation and long sessions are not qualified. The Windows fix candidate passed bounded startup and AMD Radeon 860M native fallback checks; see [Windows validation](docs/WINDOWS_VALIDATION_2026-10-05.md). NVIDIA DLSS execution with the rebuilt addon still needs qualification. Original development tests used Proton Experimental on an RTX 4080 SUPER. Resolution changes can fall back to Native; select DLSS again after the new resolution settles.

Mouse and keyboard controls have been exercised. Controller input follows the game's normal focus/navigation and D-pad actions; a physical controller qualification remains outstanding. Changing input devices can move selection back to the first native setting. Native graphics-reset integration currently recognizes the English confirmation text.

HDR uses the separately maintained UE Extended addon. Peak brightness must be calibrated to your display. The supplied guidance does not claim an official HDR mastering reference or physical panel-luminance measurements.

See [validation](docs/VALIDATION.md) for release gates and [build notes](docs/BUILD.md) for source/dependency details. Report game build, GPU/driver, output resolution, preset and reproduction steps. Remove account information, party codes, local paths and authentication data from reports.

## Local benchmark

At 4K output on an RTX 4080 SUPER, the short stationary hub sample averaged 102.54 presentation FPS Native and 129.33 Quality. Custom graphics were held fixed; this is not stock Ultra or a latency benchmark. [Method, all presets and sanitised data](docs/BENCHMARK_2026-10-04.md).

## Credits

ReShade, RenoDX/UE Extended, NeoRune and Blueprint Loader contributors, and NVIDIA. See [third-party notices](THIRD_PARTY_NOTICES.md). Development used AI assistance with local build and runtime checks.
