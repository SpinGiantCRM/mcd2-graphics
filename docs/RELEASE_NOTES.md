# 0.2.0-rc.1 — guided installation, native HDR and Reflex

Experimental prerelease, published after the maintainer accepted the available Windows/AMD qualification on 5 October 2026. Linux/NVIDIA and Windows/AMD results are separate; Windows NVIDIA Reflex and Windows HDR output remain untested.

- Standalone Windows and Linux guided installers: Steam detection, official dependency selection and verification, repair, verify, safe uninstall and sanitized support reports. No Python or separate .NET installation needed.
- Native Video-menu HDR Output, Peak Brightness, Paper White and UI Brightness controls, coordinated with the pinned RenoDX UE Extended addon. Calibration requires the restart indicated by the help panel. System HDR must already work.
- NVIDIA Reflex Off / On / On + Boost beside FPS Limit when low-latency support is reported. Streamline frame tokens, pre-input sleep and continuous measurement markers prepare the integration for later FG work. No latency reduction or performance percentage is claimed.
- Blueprint Loader 2.0 metadata: name, description, version, author and help. Controls remain in the game's native Video menu.
- Existing DLSS SR and DLAA renderer/shader binaries remain byte-identical to preview.2. Frame Generation, other upscalers and Ray Reconstruction are not included.

## Download and update

Download the compiled **Windows** or **Linux** installer archive from Assets; GitHub's automatic source archives are for developers. Extract it, close the game, run the installer and follow [INSTALL.md](../INSTALL.md). Use Repair / Verify to migrate a hash-matching preview.2 installation. Unknown/modified files are retained and reported. Earlier versions need their original uninstall workflow.

Official game/platform sign-in must already work. Vendor dependencies are separate official downloads, verified by hash; their licenses are accepted by the player. No game assets, authentication workarounds, player data or vendor graphics DLLs are redistributed.

## Qualification and limits

Linux / RTX 4080 SUPER: guided install/repair/uninstall, native HDR persistence/restart, supported Reflex modes and current reports, Native/Quality/DLAA coexistence and normal relaunch/exit checks. Windows 11 / Radeon 860M: official dependency file selection, repair, real fresh install/uninstall through the unchanged core, sanitized diagnostics, Blueprint metadata, native fallback before source reduction, unsupported HDR/Reflex behavior, resolution/level travel and three clean exits (21m37s, 4m23s, 4m38s).

Windows NVIDIA execution, HDR on a Windows HDR display, physical controllers, complete Windows keyboard/combat qualification, fresh GUI Install/Uninstall/Browse and a fresh full-ReShade setup remain unqualified. Device loss, long sessions and wider configurations also remain unqualified. Earlier unexplained Windows PR6 and Linux candidate shutdown exceptions are retained in the evidence; these bounded clean runs do not establish their cause or guarantee a fix.

Output support remains 640×360–3840×2160. Main-menu rendering remains Native, with saved DLSS selection activated in gameplay. Resolution changes may fall back to Native; reselect DLSS once stable. Offline single-player only; online/co-op permission and anti-cheat compatibility are unverified.

[Linux report](UPDATE_2_LINUX_VALIDATION_2026-10-05.md) · [Windows report](UPDATE_2_WINDOWS_VALIDATION_2026-10-05.md) · [artifact provenance](../qualification/update2/release-artifacts.json) · [remaining qualification](UPDATE_2_RELEASE_GATE.md).

## Exact published artifacts

Payload/installer source: `f032d00b23669720ee58f6d130f41cc6ea6586bb`; subsequent PR7 changes record evidence and publication documentation only. No runtime rebuild for publication.

- Windows installer from [tested CI run 37284816608](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37284816608): SHA-256 `19f520a6e5c76bb53d1f8affe77a84b4b1880c3363127e2136409153542f2a1d`.
- Linux installer from the qualified local GUI build: SHA-256 `30dabd440783032b6a1cc58d7c957676dc8ca72a23fe086944409107d7f34fdc`.

Both embed the same seven owned payload files. Different ZIP-container hashes are recorded rather than treated as different payloads. Preview.1 and preview.2 tags/assets remain frozen. The published preview.1 benchmark has not been rerun for this release and does not measure Reflex.

# 0.1.0-preview.2 — startup and compatibility fixes

- Fix the Windows render-thread stack overflow caused by large path buffers.
- Detect unsupported rendering adapters before reducing source resolution. AMD/Intel remain on native anti-aliasing with a clear status message; DLSS still requires a supported NVIDIA RTX GPU.
- Bound missing DLSS source acknowledgements to 15 seconds.
- Fix Windows running-game detection, installer portability and build recipe checks.
- Use the exact GCC addon qualified on Linux NVIDIA and Windows AMD; the UI and shader payloads are unchanged from preview.1.
- Clarify dependency installation, updating and the distinction between the compiled release ZIP and GitHub's source archive.

Linux: Quality / DLAA / Native transitions, menu travel, saved Quality after reinstall and two normal exits passed on CachyOS Proton SLR with native WineWayland. The exact addon also passed 1,027.5 seconds of Windows AMD observation and unsupported-GPU fallback checks.

Known qualification limit: one Windows candidate exit crashed with `0xc0000005`; two others exited cleanly. The cause is unknown. Windows NVIDIA execution, physical controller, long sessions and device recreation remain unqualified. Further Windows checks are planned with the separate Reflex candidate. This release does not claim to fix every startup report; please report your platform and dependency versions if a crash remains.

[Linux evidence](LINUX_CACHYOS_REGRESSION_2026-10-05.md) · [Windows evidence](WINDOWS_REGRESSION_PR6_2026-10-05.md) · [Windows isolation plan](WINDOWS_SHUTDOWN_ISOLATION_2026-10-05.md).

The release reuses addon SHA-256 `611b49abee32db2705d47e5a9e08dd676cd7e892cb7da7e563eab4c18e733240` from the PR6 test candidate. Only package version and documentation changed for publication; no native rebuild was performed. Preview.1 remains frozen. Reflex, FG and RR remain hidden and absent from this release.

# 0.1.0-preview.1-windows-fix.1 — Windows candidate

- Fix Windows startup stack overflow by moving two 64 KiB path buffers to heap storage.
- Detect unsupported rendering adapters before reducing source resolution; AMD falls back to native anti-aliasing with a visible status message.
- Bound pending DLSS source requests to 15 seconds.
- Fix Windows running-game detection and make the installer test harness portable.
- Add a reproducible Windows native build with a 16 KiB stack-frame limit.

Real Windows installation/removal, bounded startup, AMD Quality/DLAA fallback,
return to Native and normal shutdown passed. NVIDIA DLSS execution with this
rebuilt addon remains unqualified. See [Windows validation](WINDOWS_VALIDATION_2026-10-05.md).
This candidate is a separate GitHub prerelease. The original preview.1 tag and
assets remain unchanged; Nexus has not been updated. Linux / NVIDIA regression
testing is still required before replacing the original preview.

# 0.1.0-preview.1

First experimental public preview.

- Native / NVIDIA DLSS in the native Video menu; DLAA, named SR presets and Custom scale.
- Linked presets and render scale, 100% DLAA, working slider fill and output-dependent minimum.
- Main-menu selection persistence with gameplay-only activation and current-process context authorization.
- Game-thread source-resolution application, safe native-history reset, native fallback and renderer acknowledgement.
- Exact dependency checks, refusal to overwrite, owned-file uninstall, external official DLSS runtime.
- Separate manual RenoDX UE Extended HDR guidance.

Bounded checks cover a fresh profile, Quality/Native/DLAA transitions, 4K Custom17%, main-menu travel and real install/removal. This remains a preview; Windows, physical controller, device failures and long sessions are not qualified. See [validation](VALIDATION.md).
