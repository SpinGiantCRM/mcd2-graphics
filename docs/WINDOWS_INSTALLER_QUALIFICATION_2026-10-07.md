# Public installer Windows / AMD qualification — 7 October 2026

**Decision: FAIL.** Exact installation, startup and bounded AMD fallback checks passed, but two original normal window closes returned `0xc0000005`. Three later clean closes do not establish a fix. Several required menu/transition checks remain incomplete. No rendering or installer behavior changed in this qualification.

This supersedes the earlier [dependency-blocked attempt](WINDOWS_INSTALLER_PREREQUISITES_2026-10-07.md), which remains historical. Followed the [pinned checklist](WINDOWS_QUALIFICATION_CHECKLIST.md); all gate observations and full manifest/dependency hashes are in the [sanitized aggregate](../qualification/fg-release/windows-installer-qualification-2026-10-07.json).

## Exact inputs

PR [#10](https://github.com/SpinGiantCRM/mcd2-graphics/pull/10), remote head refreshed and checked at `205776d`. The checklist fixes the original package; the newer metadata package was installed only after original uninstall. Later `9514b24` documentation and `205776d` regression-test changes do not change shipped payloads.

- **original pinned**: source `38c348b953641946483fc28ec5722eb10e5dbd60`; [workflow 37554384729](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37554384729), artifact `11454227703`. ZIP `397363ffb383a45dc1230f927542168c7a890ca5277b9955038b25190304db48`; installer EXE `12485112d875a8c5f3472204fe2651cc8e9be14a770edf47af28cab6a73086eb`.
- **metadata follow-up**: source `3d97982edcbf56ff9cec7ad501f9132f5bb671f0`; [workflow 37556991330](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37556991330), artifact `11455501936`. ZIP `2d4e5668b1cbf08c6be7ec5b4a4cbf9148910f922137856eb280b823be8dec5c`; installer EXE `2febe682427ba5bf6e85eece970639c63af93eb86784f880fcff6dd051115d92`.

Both complete packages: all 235 receipt files checked plus independent canonical source-byte comparison for manifest/lock. Each GUI install verified all 12 manifest files and 13 owned receipt files. Every dependency override Off for real installs and runtime. Blueprint Loader 2.3 was recognized and retained; ReShade PR435 with successful-Reset patch, RenoDX nightly-20260928, DLSS 310.9.1 and Streamline 2.14.1 matched complete recognized sets.

Separate framework ZIP `38b1affa05445a0801372f1f17be76f87e0ab5777c769a09031cd4e23dee7cb2`; actual `d3d12.asi` `16c05f65b47b1a1f7b21eaa9a89cfbb7868b72894aab58610763c0e4014656b1`. No substitute Windows developer framework. Loader 2.3 ZIP `982ccc00b25a83da7fb3081739651b0e1a16d62ca990696ba64d89e8617c3374`.

Windows 11 Pro 10.0.26200.9457, actual Radeon 860M driver 32.0.31041.1004. Steam build 25647713; shipping SHA-256 `231147bd0c655a4ae73f90873675d42917f2bfb3a9ee164fc64f217d6d6bd4ef`. 1920×1200 Windowed Full Screen; HDR Off, VSync Off, FPS limit None.

## Checklist results

A composite check is NOT RUN where required subchecks are incomplete, even when its recorded subchecks passed.

| ID | Status | Observation |
| --- | --- | --- |
| B1 | PASS | Windows 11 Pro 10.0.26200.9457; Radeon 860M driver 32.0.31041.1004; Steam build 25647713. 1920x1200 Windowed Full Screen, HDR Off, VSync Off, FPS limit None. |
| B2 | PASS | Closed-game baseline captured 16 existing game files and seven mod/video preferences. Original private bridge omitted from initial inventory was recovered from the prior local baseline and verified against the original receipt before restoration. No player/account saves read. |
| B3 | PASS | Previous rc.1 normal GUI uninstall completed; shared dependencies retained. No ownership receipt rewritten to bypass a refusal. |
| B4 | PASS | Full self-contained package ran from a normal path containing spaces without installing Python/.NET. No SmartScreen/antivirus warning observed; unsigned build is not certified. |
| I1 | NOT RUN | Supported Steam identity and five cards passed; wrong folder refused without writes. Unsupported-executable GUI fixture not run; core fixtures cover rejection. Xbox PC not qualified. |
| I2 | PASS | Fresh original GUI install, every override Off: all 12 manifest and 13 receipt-owned files verified; complete recognized Loader 2.3 retained. Exact separate framework used. |
| I3 | NOT RUN | While running, intact Repair / Verify completed read-only with unchanged hashes; Uninstall refused and files/config remained unchanged. Missing-file write-producing Repair while running not exercised. |
| I4 | PASS | Closed intact GUI Repair / Verify succeeded without changing correct owned/shared files. |
| I5 | PASS | Closed, backed up and removed only owned live_dense.cso. GUI Repair restored exact manifest hash; unrelated files unchanged. |
| I6 | PASS | Closed one-byte shader fixture: GUI Repair and Uninstall both refused without mutation. Exact backup restored and Verify passed. Corrupt fixture never launched. |
| I7 | PASS | All five override controls separately checked: checkbox plus explicit acknowledgement text and Use installed version selected only that complete dependency set and displayed Untested override/hashes. Reopening reset all five Off. No vendor bytes changed. |
| I8 | PASS | 116 installer-core checks passed locally and Windows CI, including unknown complete sets, mixed releases, incomplete sets and extraction safety. No fake vendor DLL entered the real installation. |
| I9 | PASS | Normal original GUI uninstall removed 13 candidate-owned/private files, removed receipt and restored previous DXGI. Twelve shared paths stayed hash-identical. Only owned startup values restored; no player/account save comparison claimed. |
| I10 | PASS | Fresh original GUI reinstall and Verify: same 12/13 hashes and five strict recognized sets. Relaunch survived 67.5-second module probe and gameplay; then normal original uninstall before metadata package. |
| U1 | FAIL | Original main-menu Loader 2.3/name/author/description present, but displayed version 0.2.0-rc.1 despite installed manifest 0.3.0-preview.1. Known metadata defect. Pause-menu check not run; later metadata main-menu version fixed separately. |
| U2 | NOT RUN | Main-menu Video mouse checks passed: help and separate recommendation footer, no overlap. Unsupported FG/Reflex rows hidden. Pause-menu and keyboard navigation incomplete: injected gameplay I/Escape input did not open menus; cause not established. |
| U3 | PASS | Main-menu preset/slider/fill matched DLAA 100%, Quality 67%, Custom 77%, and clamped Ultra Performance 33%. Help explicitly showed current minimum 33%. |
| U4 | PASS | Native and Quality returned on original restarts; Custom 77% returned across fresh metadata installation with current-session menu acknowledgement. Original gameplay Quality/Custom fallback acknowledged. No saved preference counted as activation. |
| U5 | NOT RUN | Original second run observed 305.53 seconds of responsive hub gameplay; movement verified. No startup/gameplay crash in that interval. Dungeon entry/return and pause menus not completed because automated gameplay-menu input failed. |
| U6 | NOT AVAILABLE | No physical controller available for both-menu navigation. |
| A1 | PASS | Original Native/FG Off/Reflex Off survived 72.5-second probe; six actual game-directory modules and RenoDX loaded. Six module hashes independently matched package/lock, including d3d12.asi and bootstrap DXGI. |
| A2 | NOT RUN | Original Quality 67% and Custom 77% gameplay requests resolved phase 3/error 1 unsupported fallback at current request/session, before any new gameplay source acknowledgement. Original DLAA gameplay not run; DLAA completed separately with metadata package and identical native hashes. |
| A3 | NOT RUN | Closed strict mod-slot fixtures seeded FG On plus Reflex On and Boost in separate launches; only mod preference classes changed. Current-session gameplay status unavailable/inactive, no Reflex fault or restart required. Bootstrap declined actual adapter before binding/configuration/swapchain upgrade. No direct runtime foliage CVar/lease measurement obtained; source guard and 31 foliage tests pass, which does not replace that measurement. |
| A4 | PASS | Relevant native Windows bootstrap/display status contained no Wine-only Reflex pacing event; Wine-only prime path skipped. Existing native-Windows guard tests passed. No public raw log dump. |
| A5 | PASS | ModelPreset absent from installed INIs; no explicit model-hint fixture or NGX model-quality claim. |
| N1 | NOT AVAILABLE | AMD laptop; supported Windows NVIDIA activation/performance/shared-NGX teardown unavailable. |
| N2 | NOT AVAILABLE | AMD laptop; supported Windows NVIDIA activation/performance/shared-NGX teardown unavailable. |
| N3 | NOT AVAILABLE | AMD laptop; supported Windows NVIDIA activation/performance/shared-NGX teardown unavailable. |
| N4 | NOT AVAILABLE | AMD laptop; supported Windows NVIDIA activation/performance/shared-NGX teardown unavailable. |
| N5 | NOT AVAILABLE | SDR panel; native HDR output/calibration not qualified. |
| E1 | FAIL | Actual shipping handles held before all exits. First two original normal window closes returned 0xc0000005; third original and both metadata closes returned 0. Save and quit route not run; launcher exit codes not recorded. |
| E2 | PASS | All five normal closes ended within the 120-second observation bound, without force killing. Clean closes do not erase crashes or establish a fix. |
| E3 | PASS | I9/I10 complete; final metadata GUI uninstall then closed-game baseline recovery. 39 baseline entries verified, including 17 existing game files and seven existing preferences; all eight rc.1 owned hashes match original receipt. Test dependency presence restored; EnhancedInput unchanged. No player/account save rollback. |
| E4 | PASS | Sanitized report/aggregate written with all 34 checklist IDs, exact inputs, timings and limits. Raw evidence, runtime DLLs, game executable and player/account data excluded. |

## Runtime evidence and shutdown

Bounded six-module probes survived 72.5 s (original-off), 72.4 s (original-boost), 67.5 s (original-reinstall), 67.5 s (metadata). Actual game-directory SR addon, FG bridge, guide, latency addon, framework and bootstrap hashes matched; RenoDX also loaded. Original second run included a 305.53-second hub interval with observed movement. No dungeon round trip is claimed.

Quality request 105 (67%) and Custom request 121 (77%) resolved unsupported in their current gameplay sessions: phase 3/error 1, matching runtime revisions, prior source revisions 103/119 retained. Private receipt events place decline before a new gameplay source acknowledgement. A prior menu source acknowledgement at Custom 77% is not evidence of reduced gameplay rendering. FG On remained unavailable/inactive for both Reflex On and Boost; adapter support was declined before FG binding/configuration/swapchain upgrade. Foliage eligibility requires current error-free runtime; unsupported phase/error cannot acquire it. This is source/test-backed inference because a direct runtime CVar/lease measurement was not collected.

| Run | Route | Lifetime (s) | Request to process exit (s) | Shipping code | Launcher code |
| --- | --- | --- | --- | --- | --- |
| original-off | normal window close Alt+F4 | 517.74 | 9.11 | `0xc0000005` | NOT RECORDED |
| original-boost | normal window close Alt+F4 | 741.62 | 8.71 | `0xc0000005` | NOT RECORDED |
| original-reinstall | normal window close Alt+F4 | 260.14 | 6.37 | `0x0` | NOT RECORDED |
| metadata | normal window close Alt+F4 | 485.94 | 5.69 | `0x0` | NOT RECORDED |
| metadata-restart | normal window close Alt+F4 | 117.4 | 7.03 | `0x0` | NOT RECORDED |

All shipping handles were held before exit. Timings begin with a UTC marker immediately before the UI action and include tool handoff; all ended inside 120 seconds without force kill. Windows Application Error confirmed an access violation for the first failed run but identified the faulting module as unknown. No dump-derived cause is established. Save and quit was not completed: injected gameplay menu keys remained ineffective, including after helper recovery, while main-menu mouse controls and gameplay movement worked. This does not establish ReShade or the mod as the input cause.

Preserve the [PR6 shutdown crash/delay](WINDOWS_REGRESSION_PR6_2026-10-05.md), [PR7 clean closes](UPDATE_2_WINDOWS_VALIDATION_2026-10-05.md) and [different Windows developer pair](FG_WINDOWS_AMD_VALIDATION_2026-10-07.md) as separate exact-build results. None erases these two failed exits.

## Metadata follow-up

Only three UI containers differ; native addons/DLLs, shaders, settings logic and INIs are unchanged and independently hash-verified. Correct version/name/author/help and online wording observed in the main menu.

| Check | Status | Observation |
| --- | --- | --- |
| I2 | PASS | Fresh GUI install after original normal uninstall; 235 package receipt files and canonical manifest/lock source bytes verified, then 12 manifest/13 owned installed hashes and five strict recognized sets. |
| U1 | NOT RUN | Main-menu metadata PASS: Loader 2.3; MCD2 Graphics, SpinGiantCRM, version 0.3.0-preview.1, concise feature/help and online wording. Pause-menu metadata not run. |
| U2 | NOT RUN | Main-menu mouse layout/help/footer PASS. Pause-menu and keyboard navigation not completed; injected I again ignored in gameplay. |
| U3 | PASS | Repeated Custom 77%, DLAA 100%, Quality 67%, minimum 33% with synchronized thumb/fill/preset and explicit minimum help. |
| U4 | PASS | Custom 77% returned on first launch; changed DLAA 100% returned after restart with current-session menu revision/source/runtime 135, contextReady 0, phase 2/error 0. Earlier DLAA gameplay revision 132 resolved unsupported phase 3/error 1; not an activation claim. |
| A2_DLAA | PASS | Separate metadata gameplay DLAA 100% revision/runtime 132, phase 3/error 1, current sessions matched; source revision remained prior menu 130. No new gameplay source ACK preceded decline. |
| normal_exit | PASS | Gameplay normal window close 0 in 5.69 seconds; restarted main-menu close 0 in 7.03 seconds. Save and quit not run. |

DLAA gameplay request 132 at 100% resolved phase 3/error 1 in the current session; source remained prior menu revision 130. This adds a separate exact-metadata-package DLAA observation, not a retroactive original-package pass. After restart, Video showed DLAA 100%; request/runtime/source revision 135 matched the current menu session. This acknowledges inactive menu state, not active NVIDIA reconstruction.

## Restoration, changes and remaining decision

After the original uninstall/reinstall gates and the separate metadata checks, normal metadata GUI uninstall succeeded. Closed-game recovery restored and verified 39 recorded entries: 17 existing game files, seven existing mod/video preferences and expected absent test paths. All eight original rc.1 owned hashes match its unchanged recovery receipt. Initial inventory missed its private bridge; an earlier local baseline copy recovered it at the original receipt hash. EnhancedInput remained unchanged. Only known mod/dependency/configuration paths were restored, with private recovery copies and hash guards; player/account saves were not read or rolled back.

| Change | Why | Windows quirk / preservation rule |
| --- | --- | --- |
| `205776d`: provenance regression mock and compatibility note | Original mock compared a CRLF working-tree source with a canonical Git receipt and falsely failed Windows CI. Regression now executes the production canonical Git read, while stale-source rejection remains. | Windows checkout conversion; retain canonical blob comparison and both OS jobs. Production provenance checks and payloads unchanged. |
| Qualification report/aggregate and compatibility/candidate notes | Record public package evidence now the exact dependency is downloadable; retain failed shutdowns and incomplete checks. | Hold real Windows shipping handles: a closed window or vanished task-list entry cannot prove code 0. Keep this requirement across Linux changes. No speculative rendering fix. |

Local checks: installer Python 13, build recipe 6, FG Python 16, installer core 116, foliage policy 31 and Wine pacing guard passed. [CI 37557512321](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37557512321) passed all four Windows/Linux portable/installer jobs; [installer](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37557512324) and [Windows FG build](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37557512353) passed. CI does not diagnose the runtime shutdown failures.

Release remains held. Maintainer must assess the unexplained shutdown exceptions and complete the omitted routes/menus/fixtures. Supported Windows NVIDIA, HDR and controller gates require suitable hardware. No merge, release replacement or publication performed; frozen preview.1 assets remain unchanged. Raw logs, private identifiers, player data, game executable and third-party runtime DLLs are excluded.
