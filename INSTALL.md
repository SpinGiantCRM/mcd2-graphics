# Guided installation — 0.4.0-amd.dev.25

Download the compiled **Windows** or **Linux** installer and separate **MCD2-Graphics-dev25-ReShade-dependency.zip** from the [release Assets](https://github.com/SpinGiantCRM/mcd2-graphics/releases/tag/v0.4.0-amd.dev.25). Extract the **entire installer folder** before running it. GitHub's automatic source archives are for developers. No separate Python or .NET installation is needed.

The exact [published hashes and signing/scan status](qualification/providers/dev25-release-artifacts-2026-10-24.json) identify the packages. [Release scope and advanced controls](docs/DEV25_RELEASE_CANDIDATE.md).

## Install

1. Use a legally obtained Steam game that already launches and signs in normally. Launch once, then close it. Xbox app / Microsoft Store PC compatibility remains unverified.
2. Run **MCD2-Graphics-Installer.exe** on Windows or **mcd2-graphics-installer** on Linux. If Linux reports “Permission denied,” enable **Allow executing file as program** in the file properties, or run `chmod +x mcd2-graphics-installer` from the extracted folder.
3. **Game:** choose the detected Steam installation or Browse. The supported executable and build are verified.
4. **Requirements:** select the pinned Blueprint Loader, RenoDX and graphics runtime downloads. Select the separate **MCD2-Graphics-dev25-ReShade-dependency.zip** from this release under ReShade. FSR runtimes remain external dependencies. The installer verifies and places each dependency; downloads and license acceptance stay under your control.
5. **Install:** install, then launch normally through Steam. Ordinary controls are in **Settings → Video**; **Mods → MCD2 Graphics** contains advanced Upscaling, Frame Generation and Diagnostics pages.

The patched framework is a separate credited download, not an official ReShade release. Its native device/queue access and successful-reset interfaces are runtime requirements. A newer stock ReShade build may lack them. [Exact dependencies and explanation](docs/DEV25_RELEASE_CANDIDATE.md#exact-packages-and-dependencies).

The ordinary installer ZIP contains a visible self-contained application folder and 17 owned payload files under `payload/`. No embedded payload archive, self-extraction or background downloader. Vendor graphics DLLs, game assets, authentication replacements and player data are excluded. Required hashes and sources are in the [Windows lock](qualification/providers/windows-dev25/dependencies-2026-10-23.lock.json) and [Linux lock](qualification/providers/linux-dev25/dependencies-2026-10-24.lock.json).

## Versions and overrides

The exact completed runs used **Loader 2.6 on Windows/Radeon** and **2.7 on Linux/NVIDIA**, with Streamline 2.14.16, DLSS 310.9.16, FSR SDK 2.3.0 and RenoDX 0.2026.1023.0826. Use the complete recognized dependency set. A minimum API requirement does not qualify every newer release. Default-Off untested-version overrides remain explicit experiments.
Overrides allow an experiment; they do not add missing APIs or unsupported hardware. Game identity, own-file hashes, complete dependency sets, extraction safety and ownership checks remain enforced. See [override policy and optional DLSS model hint](docs/DEPENDENCY_OVERRIDES.md).

## Update, repair and verify

Close the game. For unchanged recognized **0.3.0-preview.1** or **0.2.0-rc.1** installations, use **Repair / Verify** to migrate the recorded payload. Unknown/modified files are retained and reported. Other versions require their original uninstall workflow before fresh installation. Private test candidates with the same version but different hashes are not interchangeable; do not edit receipts.

Repair checks ownership before replacing missing files. Verify checks payload/receipt, dependencies, game identity and the owned startup settings. Keep shared dependencies in place unless deliberately updating them through Requirements.

## Frame Generation and low latency

SR and FG providers can be chosen independently where supported, including FSR SR + NVIDIA FG and DLSS SR + AMD FG. This is one FG provider at a time, not native MFG or combined FG providers. Follow the game's availability and restart notices. A completely native startup requires a restart to enable FG; a session started with AMD FG can switch On → Off → On through native presenter recovery.

Reflex and supported Windows/Radeon Anti-Lag 2 remain independently selectable with FG Off. Anti-Lag 2 does not activate on NVIDIA. FG adds presentation frames; it does not increase simulation rate. Full benchmarks follow release, and no physical latency benefit is inferred from activity/FPS counters.

## HDR

Enable HDR in the OS/compositor and display, then use **HDR Output**, **Peak Brightness**, **Paper White** and **UI Brightness** in Video. Follow the restart message after calibration. Peak brightness is display-specific. Avoid simultaneous SDR conversion or another tone mapper. [HDR guide](docs/HDR.md).

The installer records only its owned `r.AllowHDR=1` and temporal-AA base `r.AntiAliasingMethod=2` startup keys. Other configuration is preserved.

## Uninstall

Close the game and choose **Uninstall**. Only unchanged recorded own files are removed; the original proxy/startup values are restored where still owned. Modified payload blocks removal before mutation. Shared dependencies, other mods, saves and preferences remain. The private DLSS runtime copy is removed with the owned installation.

## Support and qualification

**Support → Create diagnostic report** exports allowlisted versions, platform/GPU and file checks/hashes. It excludes private paths, accounts, tokens, party codes, saves and raw logs. Review it before sharing.

For crashes, black lines or shutdown faults, use the [bug report form](https://github.com/SpinGiantCRM/mcd2-graphics/issues/new?template=bug.yml) and [reporting guide](docs/REPORTING_GRAPHICS_PROBLEMS.md). Include FG/HDR state and the exact trigger. Intermittent failures are valid reports; keep raw dumps and full log folders private.

Bounded [Linux/NVIDIA](docs/LINUX_DEV25_ACCEPTANCE_2026-10-24.md) and [Windows/Radeon](docs/WINDOWS_DEV25_ACCEPTANCE_2026-10-24.md) acceptance applies to the exact [published inputs](qualification/providers/dev25-release-artifacts-2026-10-24.json), with known issues. Intermittent shutdown failures remain recorded; Windows reproduced its worker fault without this mod's graphics payload, and identical Linux ownership is unproven. Some successful Windows exits took about 89 seconds. Native Windows NVIDIA features/profile overrides, Windows HDR, physical controllers, wider hardware, long sessions and device loss remain outside this scope.
The mod does not force offline mode. Online/co-op permission and compatibility remain unverified; offline play is safer. [Online use](docs/ONLINE_USE.md). Packages are unsigned; transparent packaging does not guarantee Nexus clearance. Await Nexus staff clearance for quarantined files; do not use external links to bypass that review or disable security protection.
