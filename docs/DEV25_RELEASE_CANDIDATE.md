# FSR prerelease — 0.4.0-amd.dev.25

This release has bounded **PASS WITH KNOWN ISSUES** results on [Windows/Radeon](WINDOWS_DEV25_ACCEPTANCE_2026-10-24.md) and [Linux/NVIDIA](LINUX_DEV25_ACCEPTANCE_2026-10-24.md). The [release downloads](https://github.com/SpinGiantCRM/mcd2-graphics/releases/tag/v0.4.0-amd.dev.25) retain the accepted Windows installer and exact verified Linux folder. Earlier downloads remain unchanged.

## Features and controls

Use **Settings → Video** for Native, DLSS/DLAA or FSR upscaling, quality/custom render scale, Upscaling Sharpness, independent low latency, Frame Generation and HDR. Low latency can be used with FG Off: NVIDIA uses Reflex; compatible Windows/Radeon systems use Anti-Lag 2. Anti-Lag 2 is not supported on NVIDIA.

SR and FG providers are independent where supported: tested combinations include FSR SR + NVIDIA FG and DLSS SR + AMD FG. This is one FG provider at a time. Native multipliers and combining two FG providers are future work. Follow restart notices, particularly when starting FG from a completely native startup or changing HDR/advanced implementation settings.

Use **Mods → MCD2 Graphics** for the **Upscaling**, **Frame Generation** and **Diagnostics** tabs. Model letters come first in the DLSS selectors. SR lists Auto, E, F, J, K, L and M; FG lists Auto, A, B and C. Auto preserves quality-dependent SR selection. Explicit presets are requests, not proof of the final driver-selected model. Windows C/profile behaviour is not qualified on the Radeon test host.

FSR SR and FG implementation choices are separate. Analytical **SR 3.1.5 / FG 3.1.6** have bounded runtime coverage. Official 4.1 SR and ML FG require compatible hardware/driver approval. The optional community 4.1 INT8 upscaler is acquired separately and was not activated in this Linux qualification. Selecting portable upscaling does not enable ML FG. See [Advanced settings](ADVANCED_SETTINGS.md) for exact paths, pins and refusal behaviour.

Runtime details stay in Diagnostics. The independent gameplay-overlay toggle shows current verified activity at the top left. Requested settings, loaded DLL versions and active execution are distinct. An unavailable diagnostic sample does not establish that rendering stopped.

## Exact packages and dependencies

The [candidate selection receipt](../qualification/providers/windows-candidate-current.json) pins dev.25's native CI archive and separate installer dependency locks:

- **Windows:** [Loader 2.6 baseline](../qualification/providers/windows-dev25/dependencies-2026-10-23.lock.json), as qualified on Radeon.
- **Linux:** [Loader 2.7 baseline](../qualification/providers/linux-dev25/dependencies-2026-10-24.lock.json), as qualified on NVIDIA. Obtain the complete official [Loader 2.7 file 540](https://www.nexusmods.com/minecraftdungeons2/mods/2?tab=files&file_id=540).

Both use the same dev.25 graphics payload, supported Steam build **25910487**, separate candidate ReShade framework, Streamline 2.14.16, DLSS 310.9.16, FSR SDK 2.3.0 and RenoDX 0.2026.1023.0826. The ReShade download must match the pinned candidate hash; a stock DLL is not a substitute for the required interfaces. Keep its source/build/license provenance with the separate dependency download. Vendor runtimes are not bundled.

Build receipts and dependency notes retain their original pending-qualification wording and hashes. The linked acceptance records describe the later runtime results; those historical inputs are not rewritten to manufacture a qualified build receipt.

Extract the complete self-contained installer folder. No separate .NET or Python installation is required. The 17 payload files are visible under `payload/`; installation additionally owns its verified private DLSS copy. Close the game before Install, Repair or Uninstall. Keep original uninstall tools for older private candidates; modified or unrecognized files must be preserved rather than force-overwritten. Repair/Verify and dependency overrides retain their ownership and hash guards. A newly built archive must be checked before it is substituted for a tested installer.

## Known issues and limits

- Intermittent shutdown crashes remain disclosed. Windows reproduced its shutdown fault without this mod's graphics payload and reported it upstream. Linux's underlying access violation has not been proven identical. No universal crash repair is claimed.
- Some successful Windows exits took about **89 seconds**. Wait for the process to finish before changing installed files.
- Historical Windows focus-loss GPU crashes/black lines were not reproduced in the accepted bounded follow-up. A Linux main-menu click-capture issue recovered after Alt-Tab. Neither has a demonstrated universal repair.
- Diagnostics may briefly report activity/version information unavailable. Saved choices and override acceptance are not active-model readbacks.
- Xbox PC/Microsoft Store, physical controllers, wider hardware, long sessions and device loss remain unqualified. Online/co-op compatibility remains unverified.

Full performance, image-quality and latency benchmarks follow release. The old preview.1 benchmark numbers apply only to that release. Report problems through the [graphics reporting guide](REPORTING_GRAPHICS_PROBLEMS.md); keep raw crash folders, dumps, account data and game files private.

## Package provenance and scan status

Runtime qualification is complete within the linked scope. Final archives passed content/hash checks; see [published artifact hashes and provenance](../qualification/providers/dev25-release-artifacts-2026-10-24.json). The accepted Windows installer and verified Linux folder were retained without production rebuilds. The [package-review checks](NEXUS_RELEASE_GATE.md) still apply to future replacements. Packages are unsigned; no new antivirus clean result is claimed by these runtime reports. Rebuilding or renaming a version inside the UI/native payload creates a different candidate and requires the affected checks again. Keep existing published tags/assets frozen.
