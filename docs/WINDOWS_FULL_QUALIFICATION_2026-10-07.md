# Full Windows AMD qualification - 7 October 2026

Decision: **PASS for bounded Windows/AMD GUI installation and unsupported fallback**, with material delayed clean exits requiring maintainer review. No new crash was observed. This does not establish a universal/causal crash repair, NVIDIA/HDR compatibility or release approval. The historical cross-built-framework FAIL remains unchanged.

## Frozen inputs and machine

PR #10 installer source `e421e57e7667f0e9ec71e1940c6daa352a39f0c7`; downloaded workflow [37602995068](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37602995068), artifact `11473108471`. The full 235-file receipt was verified; manifest/lock compared byte for byte with source. No locally rebuilt installer or older native DLL substituted.

| Input | SHA-256 |
| --- | --- |
| Downloaded artifact ZIP | `4e001d34b009661068d691933336937fd97e5c1d7c84b141268f8e32e6f25b10` |
| Installer executable | `ad3bc856d06258abd62745fd5aaff02046c7be3c4b1be74c70391f3bf676eb44` |
| Installer core | `51fefeafeea0ffa682da846eac465fa4c091bd48443bd5a4f278fc58c94f5c37` |
| Separate MSVC framework ZIP | `610314d160a28f743cd2cfdc593c83b818bb2ead8576671d870c8f8652e95168` |
| Actual loaded d3d12.asi | `ce808bb1494415586cfb2ec5638a3ab0f0cea3879e45146a34976740b59fcf48` |
| Steam shipping executable | `231147bd0c655a4ae73f90873675d42917f2bfb3a9ee164fc64f217d6d6bd4ef` |

Manifest and visible Mods version: **0.3.0-preview.1**. Twelve owned hashes are in the [sanitized aggregate](../qualification/fg-release/windows-full-qualification-2026-10-07.json). Dependencies: official Loader 2.3, PR435/MSVC A successful-Reset framework, RenoDX UE Extended nightly-20260928, DLSS 310.9.1, Streamline 2.14.1; all overrides Off.

Windows 11 Pro 26200.9457, Radeon 860M driver 32.0.31041.1004, Steam app 1912410/build 25647713, game 1.1.1.0_52099637, 1920x1200 windowed full screen, SDR/HDR Off, VSync Off, no frame limit, AC power.

## Checklist results

The original [procedure](WINDOWS_QUALIFICATION_CHECKLIST.md) applies to the separately pinned [MSVC mitigation inputs](WINDOWS_MSVC_MITIGATION_2026-10-07.md). Its historical frozen input table is not rewritten.

| ID | Status | Observation |
| --- | --- | --- |
| B1 | PASS | Windows 11 Pro 26200.9457; Radeon 860M driver 32.0.31041.1004; Steam build 25647713; 1920x1200 SDR, VSync Off, no FPS limit. |
| B2 | PASS | Actual shipping process closed before recovery snapshot; 41 known mod/dependency/preferences/input entries and existing recovery backups verified. Player/account saves excluded. |
| B3 | PASS | Intact GUI uninstall obtained a fresh mod installation, retaining shared dependencies and restoring the original graphics proxy. |
| B4 | PASS | Full 235-file CI artifact verified; ordinary path containing spaces; self-contained GUI loaded its bundled runtime without SDK installation or installer Python invocation. No SmartScreen/antivirus warning observed. This is not unsigned certification or a pristine no-Python host claim. |
| I1 | PASS | GUI accepted supported Steam identity and five dependency cards. Disposable unsupported executable refused without writes. Xbox PC not tested. |
| I2 | PASS | GUI Repair upgraded original 0.2.0-rc.1 receipt; subsequent fresh GUI Install succeeded. Twelve own files/five complete dependency sets exact, 13 receipt files, version 0.3.0-preview.1, Loader 2.3 retained, overrides Off. |
| I3 | PASS | Running-game intact Repair / Verify completed read-only; all 41 entries unchanged. Confirmed GUI Uninstall refused Close Minecraft Dungeons II before changing files; fresh 41-entry snapshot unchanged. Missing-file repair writes while running not attempted; retained core regressions cover write refusal. |
| I4 | PASS | Intact GUI Verify completed; all 41 known entries unchanged. |
| I5 | PASS | Closed-game owned shader removed; GUI Repair restored exact manifest bytes and all 41 entries matched intact snapshot. |
| I6 | PASS | Closed-game one-byte shader change: GUI Repair and confirmed Uninstall refused without changing any of 41 known entries. Exact backup restored and GUI Verify passed before launch. |
| I7 | PASS | Each of five override acknowledgements and Use installed version checked individually; selection affected only that dependency. All five defaulted Off after closing/reopening. Recognized-set GUI checks do not qualify future unknown binaries. |
| I8 | PASS | Exact-source CI installer-core safety fixtures: 118 Windows / 119 Ubuntu checks. Unknown complete sets, mixed/incomplete releases and unsafe extraction covered. Local 17 installer Python regressions passed. No fake vendor DLL installed in game. |
| I9 | PASS | Intact GUI uninstall verified: 13 owned entries removed or original proxy restored; shared dependencies and known preferences unchanged; owned startup restoration validated. Player/account saves never read/restored. |
| I10 | PASS | Fresh GUI Install and Verify, exact 12 own/five dependency sets, successful startup/gameplay with six matched loaded module hashes plus RenoDX. Final GUI uninstall completed before baseline restoration. |
| U1 | PASS | Loader 2.3; both main/pause Mods pages showed MCD2 Graphics, SpinGiantCRM, description and exact 0.3.0-preview.1 version. |
| U2 | PASS | Main/pause Video pages mouse/keyboard navigation; separate help/recommendation footer without overlap. SDR HDR disabled; unsupported AMD FG/Reflex rows hidden as expected. |
| U3 | PASS | 100 DLAA, 67 Quality, 77 Custom and minimum 33 Ultra Performance: slider/preset/fill synchronized; help explains current minimum. |
| U4 | PASS | Custom 77 persisted across restart and matched current-session runtime acknowledgement. Seeded FG On/Reflex On and Boost returned with resolved current acknowledgements; preference alone not counted. |
| U5 | PASS | 326.4 conservative seconds active play after deducting 120 seconds for foreground installer testing. Movement, stairs, bridge and camera responsive; autonomous Brave Haven to Honeycomb Farm to Howling Woods/Little Howl Hamlet. Maintainer reported requested dungeon entry/return to Brave Haven complete; name/route not autonomously captured. Post-hand-off current fallback still matched. No new crash/hang. |
| U6 | NOT AVAILABLE | No physical game controller available; mouse/keyboard checks do not imply controller qualification. |
| A1 | PASS | Native/FG Off/Reflex Off startup; all six loaded addon/bootstrap/framework/bridge hashes plus RenoDX independently matched frozen manifest/lock. |
| A2 | PASS | Separate gameplay Quality 67, DLAA 100 and Custom 77 resolved current-session Native fallback. AMD vendor 4098; native receipt denies source reduction before feature creation. Quality is 6667 basis points displayed rounded 67. |
| A3 | PASS | Closed-game existing fixture tooling seeded only mod slots: FG On/Reflex On run 2 and Boost run 3. Current revisions/sessions matched; FG available/active 0, resolved phase 2, restart 0; Reflex available/active 0, fault 0. Closed logs show no FG swapchain routing. Current SR phase 3/error 1 is ineligible for foliage lease under retained source guard and 31 CI lease checks. Direct live CVar/lease capture unavailable; no direct measurement claimed. |
| A4 | PASS | All three closed-run bootstrap logs contain no wine-reflex-pacing; native-Windows guard/CI retained. Unsupported SDK cubin probe did not apply Wine pacing. |
| A5 | PASS | Optional model hint absent/default. No AMD model-selection/image-quality claim. |
| N1 | NOT AVAILABLE | No supported Windows NVIDIA GPU; active NGX/FG/Reflex, generated presents, NVIDIA performance/latency not qualified. |
| N2 | NOT AVAILABLE | No supported Windows NVIDIA GPU; active NGX/FG/Reflex, generated presents, NVIDIA performance/latency not qualified. |
| N3 | NOT AVAILABLE | No supported Windows NVIDIA GPU; active NGX/FG/Reflex, generated presents, NVIDIA performance/latency not qualified. |
| N4 | NOT AVAILABLE | No supported Windows NVIDIA GPU; active NGX/FG/Reflex, generated presents, NVIDIA performance/latency not qualified. |
| N5 | NOT AVAILABLE | SDR output; native HDR/calibration not qualified on this host. |
| E1 | PASS | Three normal exits: Save and quit and two window closes. Actual shipping process handles held before requests. No force kill/debugger; launcher codes not measured. |
| E2 | PASS | All actual process exits within 120 seconds. The 85.3-second Save and quit and 86.45-second window-close delays are material and retained for maintainer review. Window disappearance was not treated as process exit. |
| E3 | PASS | Final intact GUI uninstall, then guarded restoration of 41 baseline entries, original mod/video/input preferences and existing recovery backups. Known test-created files removed; player/account saves never restored. |
| E4 | PASS | Separate sanitized per-ID report/aggregate with frozen input hashes, timings and hardware/evidence limits. Historical FAIL remains unchanged; no release publication/merge. |

## Runtime and normal shutdown

Three startup and three gameplay probes loaded all six expected modules plus RenoDX; actual module hashes independently matched the frozen package/lock. Separate Quality, DLAA and Custom receipts identify AMD and deny source-resolution reduction before feature creation. Current FG and Reflex On/Boost requests resolve unavailable/inactive without faults. Closed native-Windows logs contain no Wine pacing or FG swapchain routing.

Active play included at least 326.4 conservative seconds of movement, camera following, stairs and bridges plus observed Fields to Woods travel. The maintainer completed the requested dungeon round trip; its name/route was not autonomously captured. Post-hand-off acknowledgements still matched. This evidence distinction is retained.

| Run / normal route | Shipping exit code | Request to process exit | Process lifetime |
| --- | --- | --- | --- |
| 1 / Save and quit | 0 | 85.3 s | 1863.2 s |
| 2 / Alt+F4 | 0 | 4.9 s | 1747.02 s |
| 3 / Alt+F4 | 0 | 86.45 s | 373.21 s |

Actual shipping process handles were held before each request. No force kill/debugger; launcher codes not measured. The 85.3-second Save and quit and 86.45-second window-close delays remain material although both returned code 0 within 120 seconds. Earlier delayed-exit and framework AV history remains in the mitigation record.

## Changes, reasons and Windows requirements

No native addon, UI, shader, build recipe, installer or dependency pin changed during this qualification. This commit adds evidence and links only. The selected MSVC framework mitigation remains exactly pinned.

| Qualification change | Why | Windows quirk / requirement preserved |
| --- | --- | --- |
| Add this report and sanitized aggregate separately from historical records. | Core API passes did not establish the public GUI package. | Verify self-contained downloaded artifact, paths with spaces, actual loaded hashes, Loader 2.3 and held shipping exits. Different inputs must not replace an older FAIL. |
| Link completed evidence from compatibility/mitigation notes. | Keep Linux refactors aware of qualified boundaries and remaining delay. | Preserve small native stack frames, case-insensitive header handling, CSV process guards, ownership/refusal, AMD decline before source reduction, current-session acknowledgements and Wine-only pacing guards. Both CI platforms stay enabled. |

## Restoration and limitations

Final intact GUI uninstall preceded guarded restoration of all 41 baseline entries, original mod/video/input settings and existing recovery backups. Temporary controls were restored; only known test-created files/backups removed. Player/account saves were not read, copied or rolled back. Raw logs/private paths remain local.

Exact-source [Windows/Ubuntu CI](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37602995042) passed: installer-core 118 Windows / 119 Ubuntu; 31 foliage lease checks per platform. [Windows FG build](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37602994984) and installer packaging passed; local 17 installer Python regressions passed. GUI required no SDK installation.

Supported Windows NVIDIA, native HDR, physical controller and Linux gameplay runtime unavailable here. Direct foliage CVar capture unavailable: matching unsupported SR state fails the retained lease eligibility guard; source/CI evidence is disclosed. Generated presents, active Reflex latency and NVIDIA performance are not claimed. Unsigned installer certification, release publication/merge and preview.1 asset replacement remain outside this qualification.
