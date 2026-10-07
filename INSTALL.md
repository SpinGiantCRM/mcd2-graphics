# Guided installation — 0.3.0-preview.1

Download the compiled **Windows** or **Linux** installer from the [release Assets](https://github.com/SpinGiantCRM/mcd2-graphics/releases/tag/v0.3.0-preview.1). Extract the **entire folder** before running it. GitHub's automatic source archives are for developers. No separate Python or .NET installation is needed.

## Install

1. Use a legally obtained Steam game that already launches and signs in normally. Launch once, then close it. Xbox app / Microsoft Store PC compatibility remains unverified.
2. Run **MCD2-Graphics-Installer.exe** on Windows or **mcd2-graphics-installer** on Linux.
3. **Game:** choose the detected Steam installation or Browse. The supported executable and build are verified.
4. **Requirements:** select official Blueprint Loader, RenoDX and NVIDIA dependency downloads. Select the separate **ReShade-PR435-4eb9056-reset-epoch-msvc-x64.zip** from the same release under ReShade. The installer verifies and places each dependency; downloads and license acceptance stay under your control.
5. **Install:** install, then launch normally through Steam. Controls are in **Settings → Video**; the Mods page shows version and help.

The patched framework is a separate credited download, not an official ReShade release. Its native device/queue access and successful-reset interfaces are runtime requirements. A newer stock ReShade build may lack them. [Explanation and direct download](docs/FG_CANDIDATE.md#why-this-reshade-build-is-required).

The ordinary installer ZIP contains a visible self-contained application folder and twelve own files under `payload/`. No embedded payload archive, self-extraction or background downloader. Vendor graphics DLLs, game assets, authentication replacements and player data are excluded. Required files/hashes and official sources are in [dependencies.lock.json](dependencies.lock.json).

## Versions and overrides

Blueprint Loader **2.0+** is the API minimum; complete official 2.0, 2.2 and 2.3 sets are recognized. The 2.2 popup API is not used. Loader, ReShade, RenoDX, DLSS and Streamline each offer an explicit **Try an untested dependency version** override, disabled by default. You can select a new official download or explicitly keep an installed version.

Overrides allow an experiment; they do not add missing APIs or unsupported hardware. Game identity, own-file hashes, complete dependency sets, extraction safety and ownership checks remain enforced. See [override policy and optional DLSS model hint](docs/DEPENDENCY_OVERRIDES.md).

## Update, repair and verify

Close the game. For an unchanged published **0.2.0-rc.1** installation, use **Repair / Verify** to migrate its recorded payload. Unknown/modified files are retained and reported. Other versions require their original uninstall workflow before fresh installation. Private test candidates with the same version but different hashes are not interchangeable; do not edit receipts.

Repair checks ownership before replacing missing files. Verify checks payload/receipt, dependencies, game identity and the owned startup settings. Keep shared dependencies in place unless deliberately updating them through Requirements.

## Frame Generation and Reflex

**Frame Generation → On** adds one generated frame between rendered frames and enables Reflex. This preview requires supported NVIDIA hardware, active **NVIDIA DLSS or DLAA**, **native HDR** and the patched framework. Restart when the help panel requests it. Unsupported hardware stays on the native path.

FG adds presentation frames while reducing rendered/base FPS; it does not increase simulation rate. [Fresh benchmark and measurement limits](docs/BENCHMARK_2026-10-07.md). NVIDIA Reflex offers Off / On / On + Boost when supported; no input-latency benefit is claimed from FPS alone.

## HDR

Enable HDR in the OS/compositor and display, then use **HDR Output**, **Peak Brightness**, **Paper White** and **UI Brightness** in Video. Follow the restart message after calibration. Peak brightness is display-specific. Avoid simultaneous SDR conversion or another tone mapper. [HDR guide](docs/HDR.md).

The installer records only its owned `r.AllowHDR=1` and temporal-AA base `r.AntiAliasingMethod=2` startup keys. Other configuration is preserved.

## Uninstall

Close the game and choose **Uninstall**. Only unchanged recorded own files are removed; the original proxy/startup values are restored where still owned. Modified payload blocks removal before mutation. Shared dependencies, other mods, saves and preferences remain. The private DLSS runtime copy is removed with the owned installation.

## Support and qualification

**Support → Create diagnostic report** exports allowlisted versions, platform/GPU and file checks/hashes. It excludes private paths, accounts, tokens, party codes, saves and raw logs. Review it before sharing.

Bounded [Linux/NVIDIA](docs/LINUX_FINAL_QUALIFICATION_2026-10-07.md) and [Windows/AMD installer/fallback](docs/WINDOWS_FULL_QUALIFICATION_2026-10-07.md) results apply to the exact [released hashes](qualification/fg-release/release-artifacts-2026-10-07.json). Windows NVIDIA active features/HDR, physical controllers, long sessions and device loss remain unqualified. Windows clean exits sometimes took 85–86 seconds. Historical failures remain documented.

The mod does not force offline mode. Online/co-op permission and compatibility remain unverified; offline play is safer. [Online use](docs/ONLINE_USE.md). Packages are unsigned; transparent packaging does not guarantee Nexus clearance. Await Nexus staff clearance for quarantined files; do not use external links to bypass that review or disable security protection.
