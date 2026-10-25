# Standalone and Vortex installation

These changes are in the **PR60 development installer**, not the published 0.4.0 installer. The new packages need Windows and Linux installation/game checks before release. Get public downloads from [Nexus Files](https://www.nexusmods.com/minecraftdungeons2/mods/87?tab=files); do not substitute source archives or files awaiting moderation.

## Standalone

Extract the whole installer folder and open the Windows or Linux application. No separate .NET or Python installation is needed.

1. Confirm the game. A single detected Steam installation is selected automatically; otherwise choose its folder or shipping executable. Game Pass on Linux uses the existing Xodus prefix.
2. Choose **Get missing requirements** if needed. Recognised installed files are reused. Approved publisher downloads are verified in a local cache; Nexus/login/licence prompts open in the browser. Return to the installer after downloading. **Import downloads** accepts multiple files and also works offline.
3. Choose **Install**, then start the game from its usual launcher.

**Update / Repair**, **Uninstall** and **Prepare for Vortex** stay on the installation page. Unsupported versions and individual requirement controls are under Advanced; file hashes are in the support report. Installation verifies files and startup settings, not rendering activity or performance.

## Vortex

The development companion works alongside [ewanhowell5195's game extension](https://www.nexusmods.com/site/mods/2402). It does not replace or redistribute that extension.

1. Manage Minecraft Dungeons II and select **Hardlink Deployment** in Vortex Settings → Mods. Keep staging on the same filesystem as the game. Symlinks and copy deployment are refused for this package.
2. Install the MCD2 Graphics companion as a Vortex extension, then import the matching **Vortex payload package** as a game mod. The standalone application archive is not a Vortex mod.
3. Import the compatible requirement downloads and enable the payload and requirements. The companion recognises whole pinned releases and maps their files to the selected edition. A Collection will supply these selections once final Nexus file IDs are available; no Collection is published yet.
4. Deploy, then launch normally. Only one MCD2 Graphics candidate can be enabled per profile.

Vortex owns hardlinks and deployment manifests. The companion checks the game, staged files, requirements, process state and ownership at stock hardlink transaction boundaries. It records and restores only its two project startup keys and the owned RenoDX activation lease. It never runs a second installer behind Vortex or edits staging hardlinks in place.

Disable MCD2 Graphics and **Deploy** to remove it, or use **Purge**. Keep the companion enabled until removal finishes. Shared requirement packages remain under Vortex control: don't disable a requirement another enabled mod needs. Profile changes, interrupted deployments, Windows paths and real Vortex behaviour remain part of qualification.

Official Vortex distribution currently targets Windows. Linux users have the independent installer; Vortex under a compatibility layer and Game Pass through Xodus need their own checks.

## Switch managers without losing settings

| Current installation | Next action |
| --- | --- |
| Old or current standalone → Vortex | Close the game. Use **Prepare for Vortex** in the new installer, or **Uninstall** in the original installer. Keep shared requirements and downloads. Then import/enable the matching Vortex package and Deploy. |
| Vortex → standalone | Close the game. In Vortex, disable MCD2 Graphics and Deploy, or Purge. Keep the companion enabled until this finishes. Open standalone, choose the same game and **Check again**, then Install. |
| Interrupted removal | Retry removal through the recorded manager. The ownership journal is retained until configuration and deployment cleanup complete. |
| Unknown or edited receipt/payload | Keep the files and restore a trusted backup or use the original installer. Neither manager adopts unknown files. Do not delete DLLs or edit receipts to force a takeover. |

Preferences and saves are not installer-owned. Removal restores startup values only while still owned; unrelated configuration and player edits are retained. Standalone removes its private DLSS copy, while shared dependencies remain available. Vortex removes only files owned by its deployed packages. The receiving manager rechecks all requirements before writing.

The synthetic cross-adapter test exercises legacy standalone → Vortex → standalone using the real managed engine and JavaScript adapter. It is filesystem evidence; it does not establish live Vortex UI, Game Pass or graphics compatibility.

## Package identities

`mcd2-install.json` is generated from the selected manifest and platform lock. Standalone embeds the same contract as the distributed sidecar; the Vortex ZIP contains the contract and `payload/` only. Both routes use identical graphics bytes. Frameworks and vendor runtimes stay separate.

An `-install.N` suffix identifies an installer-only revision. The manifest separately records the native/UI candidate version; graphics binaries are not rebuilt or relabelled merely to change installation. Published 0.4.0 archives remain unchanged.
