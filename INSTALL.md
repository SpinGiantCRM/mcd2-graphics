# Guided installation — Update 2 candidate

Update 2 (`0.2.0-rc.1`) is a test candidate. Published preview.1 and preview.2 remain unchanged. Do not publish this candidate until [Windows qualification](docs/UPDATE_2_RELEASE_GATE.md) passes.

## Install

1. Use a legally obtained game that already launches and signs in normally. Launch it through Steam once, then close it.
2. Run **MCD2-Graphics-Installer.exe** on Windows or **mcd2-graphics-installer** on Linux. No Python or separate .NET installation is needed.
3. **Game:** select the detected Steam installation or use Browse. The supported executable and build are verified.
4. **Requirements:** open each official download and select the downloaded file/archive. The installer verifies the pinned hashes and places Blueprint Loader, RenoDX and NVIDIA runtimes correctly. They are not bundled. A known Blueprint Loader 1.1 installation can upgrade to 2.0; its original bytes are retained in a recovery folder. Unknown or modified dependencies are retained.
5. **ReShade:** run the official full addon-support installer for `Dungeons/Binaries/Win64/Dungeons-Win64-Shipping.exe`, selecting DirectX 10/11/12. Complete its own workflow and any agreement yourself, then Check again. A normal or unsupported build is refused.
6. **Install:** install the mod. Launch normally through Steam and open **Settings → Video**. The Mods page identifies the mod and links to help.

The installer contains only this project's seven payload files plus its own application/runtime. Vendor graphics runtimes, game assets, authentication replacements and player data are excluded. Downloads and license acceptance remain under the player's control. Required versions and official links are in [dependencies.lock.json](dependencies.lock.json).

On Linux the official ReShade setup is launched using the game's detected Proton prefix/tool. This requires an existing Steam prefix. If that cannot be detected, use the official setup in that game prefix and Check again. The installer does not change Steam launch options or solve platform sign-in.

## Update, repair and verify

Close the game. For a hash-matching preview.2 installation (including its recorded Linux-regression receipt alias), choose **Repair / Verify** to migrate its receipt and owned payload. If files differ, back them up and resolve the conflict first. Other versions require their original uninstall workflow before installing this candidate.

Repair checks ownership before replacing missing files. Modified files are retained. Verify checks the payload, receipt, dependencies, supported executable and owned HDR startup setting. It reports missing or unsupported versions instead of quietly accepting them.

## HDR

The installer owns only `[SystemSettings] r.AllowHDR=1` and `r.AntiAliasingMethod=2` (the temporal-AA boundary required by SR) in Dungeons/Config/UserEngine.ini and records both previous values. It preserves other configuration. Enable system HDR yourself, then use **HDR Output**, **HDR Peak Brightness**, **HDR Paper White** and **HDR UI Brightness** in Video. Calibration changes require a game restart with the pinned RenoDX addon; the help panel says so. See [HDR](docs/HDR.md).

## Uninstall

Close the game and choose **Uninstall**. Only hash-matching files recorded by the installer are removed. Modified payload blocks removal before mutation. Shared dependencies, other mods, saves and preferences remain. The mod’s private DLSS runtime copy is removed with its owned payload. Each owned startup key is restored to its previous value only if it remains unchanged; player edits are retained.

## Support

**Support → Create diagnostic report** exports a small allowlisted report: mod/game version, platform, GPU/driver and file checks/hashes. It excludes usernames, private paths, account data, tokens, party codes, player saves, raw logs and unrelated ReShade configuration. Review it before sharing it in an issue.

Use offline. Online/co-op compatibility is unqualified; see [online use](docs/ONLINE_USE.md). The legacy Python CLI is retained for historical preview checks and refuses this Update 2 payload. The qualification branch includes only the hash-pinned own payload under `qualification/update2`; it excludes vendor runtimes. Use the CI candidate installer artifact for testing, never treat it as a published release.
