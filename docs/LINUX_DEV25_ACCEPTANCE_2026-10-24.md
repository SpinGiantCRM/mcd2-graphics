# Linux/NVIDIA dev.25 qualification — 24 October 2026

**PASS WITH KNOWN ISSUES for the tested scope, under the maintainer's bounded acceptance rule.** Linux qualification is complete. This records working features and real failures; it does not claim a shutdown repair or publish a release. The separate [Windows/Radeon acceptance](WINDOWS_DEV25_ACCEPTANCE_2026-10-24.md) remains valid for its own inputs.

## Exact inputs

- Candidate **0.4.0-amd.dev.25**, requested source `e704a17c59e69ad4ec04191f43b0928ee0ad1097`; native artifact source `b5719f95820acb97dcd5fcdfa60bdde291b3379a`, identical tree `775604d9599c7ae1862eaeeb878f31c611cdcc65`. All 46 recorded artifact members and 45 consumed source pins matched. No production rendering, UI or installer source changed for this qualification.
- [Native CI artifact](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/39774607102/artifacts/12424387023), archive SHA-256 `49ae3e0eb63992eb8d161648623d24536ad1c2f52ca0cf72eded432450ee4476`.
- CachyOS/KDE Wayland, **RTX 4080 SUPER**, driver **615.78.08**, Proton **CachyOS-11.0-100**, 3840×2160 output. Steam build **25910487**, executable SHA-256 `3842102bb05fc532b22e97992d61c2248d115be80ff752ff9b6843bf4acd7005`.
- [Linux dependency lock](../qualification/providers/linux-dev25/dependencies-2026-10-24.lock.json), SHA-256 `8cc2d271748a5c4534f62b362f63edfa2d5ecf12c4df2a236ce169803ace4bbd`. It differs from the Windows lock only in the Loader entry: **Blueprint Loader 2.7 / file 540**, as requested by the maintainer. The publisher archive hash matched `100347ddf924686f12b0a042ba5cd47d487c30020bdd8b7eab05a7156022429d`. Windows used Loader 2.6; this does not retroactively qualify 2.7 on Windows.
- Shared inputs include Streamline **2.14.16**, DLSS **310.9.16**, FSR SDK **2.3.0**, RenoDX **0.2026.1023.0826**, and the separately acquired candidate framework SHA-256 `9fdac099b390ff2d88d4cf6d51685f9991ea7484e6a70fe0b092019d7adddba2`. Vendor runtimes and the framework are outside the installer payload.

## Completed checks

| Check | Result | Evidence and limit |
| --- | --- | --- |
| Packaging and installation | PASS | Self-contained Linux folder: 241 file pins, 17 packaged payload files. Live install through the unchanged production installer core verified 18 owned files, including the private DLSS copy, and 15 dependencies; no overrides. No fresh installer GUI interaction claim. |
| Install/repair/removal safety | PASS, isolated | 148 core checks; 15 exact-candidate fixture checks cover fresh install, Verify, missing-file repair, modified-file refusal, uninstall, dependency/unrelated-file preservation and reinstall. The live candidate remains installed. |
| Native, DLSS/DLAA and analytical FSR SR | PASS, bounded | All six modes for each provider produced advancing current-context evaluations without a reported SR error. FSR reported implementation **3.1.5**. Quality produced 2560×1440 → 3840×2160; full-resolution AA used 3840×2160; custom 77% was also exercised. All dimensions are in the [numeric record](../qualification/providers/linux-dev25/acceptance-2026-10-24.json). |
| Independent low latency with FG Off | PASS, bounded | Reflex On/Boost/Off with FSR, plus On with Native. Unsupported Anti-Lag 2 on NVIDIA was refused. Markers can advance while Reflex is Off; those markers alone are not activation or latency-benefit evidence. |
| NVIDIA FG combinations | PASS, bounded activity | DLSS Quality, DLAA and FSR Quality each advanced generated-frame counters with fresh successful two-present SDK outcomes and no reported SDK fault. Actual HDR colour-space identity was present. These are activity checks, not display FPS or physical HDR calibration. |
| Resolution and sharpness | PASS, bounded | Maintainer exercised resolution changes and sharpness 0/100, then restored the preferred value. After returning to 4K, 15 fresh successful two-present samples confirmed FG recovery. No independent engine readback of the sharpness endpoints is claimed. |
| AMD FG on NVIDIA, SDR | PASS, bounded | Analytical FG **3.1.6** generated with FSR, DLSS and Native. SDR colour-space identity was verified with RenoDX absent. ML FG is a separate hardware-dependent path. |
| AMD FG On → Off → On | PASS, bounded recovery | Off returned to the native presentation route without a restart; On generated again. A completely native startup correctly reported that enabling FG requires a restart. Short provider evaluation-rate samples did not demonstrate a persistent Off loss; they are not physical/display FPS or full benchmarks. |
| Loader 2.7 advanced menu | PASS, bounded manual check | The maintainer completed the requested tabs, side-help and top-left gameplay-diagnostics inspection, with no visual problem reported. This is not exhaustive keyboard/controller or menu-layout coverage. |
| Restoration | PASS | Final live verification matched all 18 owned / 15 dependency files. Production compare-and-swap restored settings while preserving the maintainer's intervening menu changes. Temporary Proton recorder settings were removed; game closed. No player/account saves were inspected or edited. |

Sampling required current session/revision/world identity, freshness and advancing counters. NVIDIA FG outcomes were also checked against independent SDK presentation receipts. A saved selection, library version or requested model override was not substituted for active execution.

## Shutdown observations and known issues

Four ordinary sessions used the normal **Save and quit to desktop** route. Native/SR/latency, fresh-Off SDR, and AMD-FG switching sessions returned held process exit **0**. The NVIDIA-FG/resize session returned **C0000409**. A separate replay with a private terminal-signal recorder, unchanged FSR Quality/NVIDIA FG/Reflex/HDR settings, also returned **C0000409**. It is diagnostic evidence, not another ordinary control. Whole-session elapsed times are not shutdown latencies.

The replay captured an earlier **C0000005 read at address 0x10** before the terminal failure during exception dispatch. A defect in this mod's payload was not demonstrated. This does **not** prove the Linux fault is identical to the Windows failure or identify the responsible game/dependency component.

Windows already reproduced its matching worker fault with **official ReShade + Loader and no graphics payload from this mod**. Its [component controls](../qualification/providers/windows-dev25/component-controls-2026-10-23.json), [heap diagnostic](../qualification/providers/windows-dev25/heap-exit-2026-10-23.json) and [existing upstream reports](../qualification/providers/windows-dev25/reporting-2026-10-23.json) are reused. No duplicate component matrix or upstream report was created. Under the maintainer's acceptance rule, these disclosed failures do not expand this qualification into repairing the game or require waiting for an upstream reply. No exception suppression, forced successful exit or crash-repair claim is included.

The diagnostic replay observed 180.27 seconds with 164 active samples out of 181, +5,196 generated frames and 224 successful two-present samples, with no sampled SDK failures. Brief inactive intervals are retained; continuous activity across every frame is not claimed.

Two other observations remain visible:

- One main-menu startup lost mouse clicks while cursor movement continued. Alt-Tab out and back restored clicks, before any automated presenter switches. The cause is unknown.
- Diagnostics sometimes showed unavailable activity or implementation/version information while independent SDK outcomes advanced. Missing or stale samples were excluded, and a zero version was not treated as proof that a library was unloaded.

## Scope outside this acceptance

Full FPS, frame-time, image-quality and physical latency benchmarks are **deferred until after release at the maintainer's request**. Earlier release benchmarks do not measure dev.25. No new performance-benefit claim is made here.

Optional community FSR 4.1 INT8 activation, official FSR 4.1 and ML FG on supported Radeon hardware, Linux Radeon, native Windows NVIDIA model/profile overrides, physical controllers, long sessions, device loss and comprehensive travel/respawn coverage remain outside this run. Xbox PC/Microsoft Store support, native MFG and combined FG providers are future work.

The [machine-readable acceptance](../qualification/providers/linux-dev25/acceptance-2026-10-24.json) preserves the exact scope. Use the [release-candidate guide](DEV25_RELEASE_CANDIDATE.md) and [reporting guide](REPORTING_GRAPHICS_PROBLEMS.md) for installation and issue reports. Raw runtime logs, dumps, private paths and game data remain private.
