# Update 2 qualification and release gate

Update 2 is the upcoming guided-installer / native HDR / Blueprint Loader 2.0 /
Streamline Reflex release. It is distinct from published `v0.1.0-preview.2`.
The working candidate uses `0.2.0-rc.1`; this is not a published release.

**Do not publish Update 2 until required Windows parity and regression checks pass
against the exact final candidate.** Missing, unrun or failing checks block release.
The preview.2 decision accepting known Windows limitations does not carry over.
A Linux pass or portable CI pass is not Windows runtime qualification.

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

Released preview.1 and preview.2 tags/assets remain unchanged. No new publication
is authorized merely by this checklist or by building a candidate.
