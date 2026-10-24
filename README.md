# MCD2 Graphics — DLSS, FSR, Frame Generation, HDR and low latency

[Download 0.4.0-amd.dev.25](https://github.com/SpinGiantCRM/mcd2-graphics/releases/tag/v0.4.0-amd.dev.25) · [Nexus page](https://www.nexusmods.com/minecraftdungeons2/mods/87) · [Roadmap](ROADMAP.md) · [Contributing](CONTRIBUTING.md) · [Validation](docs/VALIDATION.md) · [Documentation](docs/README.md)

DLSS/DLAA, FSR upscaling, independent Frame Generation, HDR and supported low-latency controls in Minecraft Dungeons II's menus, with a guided Windows/Linux installer.

**Experimental prerelease: 0.4.0-amd.dev.25.** Adds analytical FSR SR/FG, independent provider choices, sharpness, advanced model settings and top-left gameplay diagnostics. The exact tested payload is retained. [Changelog](docs/RELEASE_NOTES.md) · [Installation](INSTALL.md) · [Hashes and scan/signing status](qualification/providers/dev25-release-artifacts-2026-10-24.json). Earlier tags/assets remain frozen.

**NOT AN OFFICIAL MINECRAFT PRODUCT. NOT APPROVED BY OR ASSOCIATED WITH MOJANG OR MICROSOFT.**

## Controls

Use **Settings → Video** for:

- **Upscaling:** Native, NVIDIA DLSS/DLAA or AMD FSR, with quality presets and custom render scale. Availability and the minimum scale follow the provider, output size and supported hardware.
- **Upscaling Sharpness:** one post-upscale game filter for either provider. Native fallback restores the prior value while still owned; external changes take precedence.
- **Frame Generation:** select the supported FG provider independently of the upscaler. Tested combinations include FSR SR + NVIDIA FG, DLSS SR + AMD FG and Native + AMD FG. This release uses one FG provider at a time.
- **Low latency:** NVIDIA Reflex Off / On / On + Boost, or Anti-Lag 2 on compatible Windows/Radeon systems. It remains independently selectable with FG Off. Anti-Lag 2 is not supported on NVIDIA.
- **HDR:** HDR Output, Peak Brightness, Paper White and UI Brightness through the separate RenoDX dependency.

The main menu remains Native; saved reconstruction choices activate in gameplay. Follow Applying and restart notices. AMD FG can return to native presentation on Off and generate again in a session started with AMD FG; enabling FG from a completely native startup requires a restart.

Use **Mods → MCD2 Graphics** for **Upscaling**, **Frame Generation** and **Diagnostics** tabs with persistent framed side help. DLSS SR lists Auto, E, F, J, K, L and M; DLSS FG lists Auto, A, B and C. Auto preserves quality-dependent SR selection. Explicit presets are requests, not final model readbacks. FSR SR/FG implementation choices are independent. See [Advanced settings](docs/ADVANCED_SETTINGS.md).

The optional top-left gameplay overlay reports fresh, matching provider activity. Menu details, loaded DLL versions, requested overrides and actual execution remain distinct; a temporarily unavailable sample is not proof that rendering stopped.

## Requirements and installation

Use supported **Steam build 25910487**, the separate pinned candidate ReShade framework, Blueprint Loader, RenoDX and the required official graphics runtimes. Windows/Radeon used **Loader 2.6**; Linux/NVIDIA used **2.7**. The installer verifies the complete platform-specific dependency set. Overrides do not provide missing APIs or unsupported hardware.

Extract the entire Windows/Linux installer ZIP. No separate .NET or Python installation is needed. Its 17 owned payload files are visible under `payload/`; installation additionally owns its verified private DLSS copy. Framework/vendor runtimes remain external, and no player/account data or game assets are included. [Install, update, repair and uninstall](INSTALL.md).

The separately downloaded ReShade build supplies required device/queue and reset interfaces. A stock DLL is not interchangeable with the pinned framework. Its archive includes source references, exact patches, build receipt and license.

## Tested scope and known issues

Bounded [Windows/Radeon](docs/WINDOWS_DEV25_ACCEPTANCE_2026-10-24.md) and [Linux/NVIDIA](docs/LINUX_DEV25_ACCEPTANCE_2026-10-24.md) qualification is **PASS WITH KNOWN ISSUES** for the exact inputs recorded there. Analytical FSR SR **3.1.5** and FG **3.1.6** have runtime coverage. Official FSR 4.1, optional community INT8 and ML FG activation remain outside these completed checks and require their own hardware/runtime approval. Portable upscaling does not unlock ML FG.

Intermittent shutdown failures remain disclosed. Windows reproduced its worker fault with official ReShade + Loader and no graphics payload from this mod; the Linux fault is not proven identical. Successful Windows exits can take about **89 seconds**. Historical focus-related GPU crashes/black lines, a Linux click-capture recovery and intermittent unavailable diagnostic samples remain visible. No universal crash repair is claimed.

Native Windows NVIDIA activation/profile overrides, Windows HDR, physical controllers, Linux Radeon, long sessions, device loss and wider lifecycle/hardware coverage remain unqualified. Xbox app / Microsoft Store PC, native MFG, combined FG providers, XeSS and ray reconstruction are not supported by this release.

Online/co-op compatibility and permission remain unverified; offline play is safer. The mod does not force offline mode. Follow the publisher's terms. [Online use](docs/ONLINE_USE.md).

## Benchmarks and support

Full FPS, frame-time, image-quality and physical latency benchmarks follow release. The [0.3.0-preview.1 benchmark](docs/BENCHMARK_2026-10-07.md) is historical and does not measure this release. Activity counters and generated frames are not physical display FPS or measured input-latency improvement.

Use the [bug form](https://github.com/SpinGiantCRM/mcd2-graphics/issues/new?template=bug.yml) and [reporting guide](docs/REPORTING_GRAPHICS_PROBLEMS.md). Include store, game/mod version, platform, GPU/driver, settings and reproduction steps. Keep raw logs/dumps, accounts, private paths, game executables and saves out of public reports.

## Cross-platform maintenance and credits

Read [Windows compatibility requirements](docs/WINDOWS_COMPATIBILITY.md) before changing the native addon, installer or build recipe. Source builders should use the explicit dev.25 selection and platform locks; the root preview.1 manifests remain historical baseline inputs.

Thanks to ReShade, RenoDX/UE Extended, NeoRune, Blueprint Loader, NVIDIA and AMD/GPUOpen contributors. [Third-party notices](THIRD_PARTY_NOTICES.md). Development used AI assistance with local build and runtime checks. Packages are unsigned; no current local antivirus clean-scan result is claimed.
