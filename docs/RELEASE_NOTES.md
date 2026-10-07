# 0.3.0-preview.1 — Frame Generation preview

Experimental prerelease. Uses the exact Windows-qualified installer source and payload; Linux qualification and fresh benchmarks passed without runtime or installer code changes for publication.

## Frame Generation

- Adds **Off / On** to the native Video menu. On generates one extra frame
  between rendered frames and enables Reflex on supported NVIDIA hardware.
  This preview requires active NVIDIA DLSS/DLAA, native HDR and the patched
  ReShade framework; restart when requested.
- Supports full-resolution DLAA and reduced-resolution DLSS inputs. Foliage
  motion-vector changes address the severe shimmering reported during testing;
  some interpolation artifacts remain possible.
- Coordinates shared NVIDIA runtime ownership so changing SR presets does not
  shut down a runtime still used by FG. Internal evaluation failures are checked
  separately from reported presentation counts.

## Reflex and DLSS fixes

- **Linux/Proton Reflex pacing:** stabilizes the tested VSync-Off presentation
  path by disabling dynamic Vulkan swapchain-mode switching before graphics
  startup. Existing extension exclusions, VSync choice and frame limit are
  preserved. Native Windows skips this compatibility setting.
- In the recorded stationary 4K Linux check with DLSS and FG Off, Reflex On
  averaged about **0.2% below Off**, with substantially steadier frame intervals
  than the old path. The full reported 100→80 FPS drop was not reproduced in
  that scene. VSync On remains a separate limitation; these measurements do not
  establish input-latency reduction. See the [measured result](../experiments/fg-streamline/REFLEX_PACING_2026-10-07.md).
- **DLSS SR / DLAA foliage:** enables vertex-deformation motion vectors during
  acknowledged gameplay, including FG Off. This gives reconstruction information
  about animated foliage instead of treating its deformation as stationary.
  Native fallback restores the previous value where the mod still owns it;
  later user/mod changes take precedence. It does not promise to remove every
  foliage or shadow artifact.
- **Preset transitions:** adds successful-reset confirmation for dormant command
  lists, addressing the reproduced Quality→DLAA request that stayed Applying.
  Resource release still requires completed GPU work. This is a specific
  lifetime fix, not a claim that every historical hang or exit issue is resolved.

## Settings and dependencies

- Shortens setting descriptions, adds bullets where useful and places
  recommendations in the separate native footer. FG help describes generated
  frames directly; the old 15-minute preview wording is removed.
- Updates Mods-page version, feature description, author/help information and
  online wording. Settings remain in **Settings → Video**.
- Supports the Blueprint Loader **2.0+** metadata/settings API without using the
  2.2 popup API. Complete official 2.0, 2.2 and 2.3 sets are recognized without
  forced upgrades.
- Adds explicit, default-Off untested-version overrides for **Loader, ReShade,
  RenoDX, DLSS and Streamline**. A minimum API/version is not a guarantee about
  every future release. Overrides preserve game identity, own-payload hashes,
  required-file completeness, archive safety and file ownership checks.
- Adds an optional restart-only DLSS model hint. Default/absent configuration
  keeps NVIDIA's model selection; it does not change render-scale presets.
  See [dependency overrides](DEPENDENCY_OVERRIDES.md).

## Installation and availability

- Uses a normal ZIP with self-contained installer files and an expanded visible
  payload. No embedded payload archive, self-extraction or background downloader.
  Repair/uninstall preserve shared dependencies, saves and modified files.
- Provides the exact credited ReShade framework as a separate direct download,
  fixing the missing download during Windows qualification. Its device/queue
  access and successful-reset interfaces are runtime requirements; a newer stock
  build may lack them. See [the explanation and download](FG_CANDIDATE.md#why-this-reshade-build-is-required).
- The mod does not force offline mode. Online compatibility is unverified;
  offline play is safer. Xbox app / Microsoft Store PC compatibility remains
  unverified. NVIDIA runtime DLLs remain external official dependencies.

At 4K on RTX 4080 SUPER, DLAA measured **81.80 FPS Off → 61.06 rendered / 122.11 SDK presentation FPS On**; Quality measured **122.74 Off → 83.76 / 167.52 On**. Three 30-second samples per mode; fresh rc.1 controls were within about 2%. [Method and per-run numbers](BENCHMARK_2026-10-07.md). Earlier tables remain historical.

[Final Linux qualification](LINUX_FINAL_QUALIFICATION_2026-10-07.md) and [Windows AMD GUI/fallback qualification](WINDOWS_FULL_QUALIFICATION_2026-10-07.md) cover the exact shipping files. Windows clean exits sometimes took 85–86 seconds; historical framework failures remain documented. No universal shutdown repair is claimed. Windows
AMD fallback results do not qualify active Windows NVIDIA FG/Reflex or HDR.
Packages are unsigned; transparent packaging does not guarantee Nexus clearance.

See [candidate installation and limits](FG_CANDIDATE.md),
[Windows qualification](WINDOWS_FULL_QUALIFICATION_2026-10-07.md) and
[packaging/review checks](NEXUS_RELEASE_GATE.md). Published tags remain unchanged.

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
