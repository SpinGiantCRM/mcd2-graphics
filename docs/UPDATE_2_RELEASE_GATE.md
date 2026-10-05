# Update 2 qualification and release gate

Update 2 is the guided-installer / native HDR / Blueprint Loader 2.0 /
Streamline Reflex release. It is distinct from published `v0.1.0-preview.2`.
The release uses `0.2.0-rc.1`, an experimental prerelease.

## Publication decision — 5 October 2026

The maintainer completed the available AMD Windows testing and explicitly requested GitHub/Nexus publication. This supersedes the earlier instruction to wait for complete Windows parity **for this experimental prerelease only**. The exact qualified Windows CI and Linux local GUI installers are reused without a runtime rebuild. Missing checks remain unqualified and are disclosed in the release notes; no AMD result is treated as Windows NVIDIA/HDR qualification. Preview.1 and preview.2 remain frozen.

## Remaining qualification before a broader/stable claim

The original checklist below remains the qualification target. A Linux/CI pass is not Windows runtime qualification. Preserve the original partial evidence and unknown shutdown causes.

PR7's bounded Windows/AMD installation, fallback and shutdown results are in
[the Windows report](UPDATE_2_WINDOWS_VALIDATION_2026-10-05.md). Several required
checks remain unqualified; that report does not qualify the missing paths. Use
`-RequireDisplayLatency` with the Windows runtime probe for Update 2.

Required Windows evidence, tied to commit and every candidate/installer hash:

- Fresh install, repair / verify, safe uninstall and sanitised diagnostics.
- Automatic Steam-library detection, browse, official dependency selection and
  verified placement, normal/full-addon ReShade distinction.
- Native Video stock ordering preserved; HDR directly after Brightness; Reflex
  next to FPS Limit when genuinely available. Mouse, keyboard and controller;
  main-menu and pause-menu access, reset confirmation, persistence and restart.
- HDR On/Off plus peak, paper-white and UI brightness; disabled calibration when
  Off; system-HDR unavailable case; reboot/relaunch and native/RenoDX agreement.
- Blueprint Loader 2.0 metadata shows name, description, version and author.
- Supported **Windows NVIDIA** actual Streamline `sl.reflex` execution: Off/On/
  On + Boost, shared frame identities, complete current reports, sleep before
  input even when Off, PCL ping attribution and failure/lifecycle handling.
- Unsupported Windows AMD/Intel rendering-adapter behavior: Reflex unavailable,
  saved choices retained, Native fallback before render scale reduction.
- Existing Native / Quality / DLAA / Custom transitions, resolution changes,
  menu/level travel, gameplay, responsiveness and repeated normal shutdown.
  Investigate any exception including the prior unexplained `0xc0000005`.

Record environment (OS, GPU, driver, game/dependency versions), exact hashes,
procedure and outcome for each check. Do not infer NVIDIA support from an AMD
fallback pass. Do not weaken SDK OS/security/signature checks to obtain a pass.

Before requesting Windows qualification, finish Linux functionality/regression,
native HDR/layout/persistence/restart, Blueprint 2.0 integration and non-Windows
installer checks. Notify the user when those are complete and Windows is the
sole remaining gate. This scope ends there; FG/providers/RR/shadows are later work.

Released preview.1 and preview.2 tags/assets remain unchanged. Publication was separately authorized by the maintainer; this checklist and future builds alone do not authorize another release.
