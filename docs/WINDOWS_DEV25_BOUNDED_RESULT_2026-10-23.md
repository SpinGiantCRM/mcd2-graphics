# Windows Radeon qualification result — 23 October

**Windows remains UNQUALIFIED.** One ordinary full-candidate run reproduced the intermittent shutdown access violation. Six isolation attempts did not identify its source owner; no crash repair or release approval is claimed. This closes the bounded investigation and recovery work while preserving the failed and untested gates below.

## Exact inputs

Candidate `0.4.0-amd.dev.25`: requested source `e704a17c59e69ad4ec04191f43b0928ee0ad1097`, native artifact source `b5719f95820acb97dcd5fcdfa60bdde291b3379a`, identical tree `775604d9599c7ae1862eaeeb878f31c611cdcc65`. Installer logic is source `97c92569169ec5efdb5c4ac7bf54b0460f8db6fe`; no production source changed this execution. [Native CI](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/39774607102) and [portable Windows/Linux CI](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/39774607284) remain the source-bound build evidence.

Current-dependency installer SHA-256: `4b441e9d819ccdd1b132cb3edc396e8c9c05ff239665ebacefb7d14e67d331c2`. Its [candidate-specific lock](../qualification/providers/windows-dev25/dependencies-2026-10-23.lock.json) SHA-256 is `0cebd7a90ef5338c1d0dcd32e71aece15456ee78199f08b20692d3601e202979`. The released lock and frozen preview.1 assets were not changed. The framework remains separate from the installer.

Publisher inputs checked before comparisons: Loader **2.6/main file 506**, Streamline **2.14.16**, DLSS **310.9.16**, FSR SDK **2.3.0**, and newly current RenoDX **0.2026.1023.0826**, SHA-256 `572124413ff392c3741f730f45896f4fc99753de9805d04887c484d4085f7c3a`. RenoDX's asset update was 23 October 10:31:35 UTC. All matched arms used it; earlier observations retain their older dependency scope. Old bytes are retained privately for recovery.

Host: Windows 11 build 26200, Radeon 860M, driver **32.0.32015.7029**, Steam build **25910487**, shipping SHA-256 `3842102bb05fc532b22e97992d61c2248d115be80ff752ff9b6843bf4acd7005`, 1920x1200 SDR Windowed Full Screen.

## Gate results

| Gate | Result | Evidence and limit |
|---|---|---|
| Input identity and recovery baseline | PASS | Executable, payload, dependencies, receipt and owned transactions checked. |
| Current-dependency GUI install / Verify | PASS | 18 owned / 15 dependency hashes match; overrides empty. Final reinstall, startup and main-menu quit completed. |
| Installer guards, repair and overrides | PASS, reused | Earlier dev.25 GUI observations use unchanged logic/native payload. Current Core run passed 147 checks; all seven dependency cards recognized. No unknown-runtime compatibility claim. |
| Main-menu metadata and Video mouse controls | PASS, reused | Shared named preset, numeric percentage, custom bounds and unavailable-DLSS skipping in the [detailed record](WINDOWS_DEV25_UPDATE_2026-10-22.md). Keyboard coverage remains separate. |
| FSR Quality with current dependencies | PASS | 1280x800 to 1920x1200; current-world SR +53 / FG +26, no provider error. |
| Native → FSR → Native | PASS, bounded | Native target and observed source scale 100%; SR inactive. FSR live re-entry SR +37 / FG +51. Zero inactive-SR dimensions are not interpreted as a measured Native render size. |
| Native with AMD FG | PASS, bounded | Settled current-world rendered/generated+32, zero SR evaluations, no FG fault. |
| FSR + FG + Anti-Lag | PASS, bounded | Settled current-world SR +13 / rendered+25 / generated+25 / Anti-Lag +25; mode On and compatibility acknowledged. Reset/stale/transition intervals rejected. |
| Other presets and unsupported advanced fallback | PASS, reused | Same native/FSR bytes; earlier updated-game dimensions and explicit INT8/ML refusal retained. No optional ML activation claim. RenoDX HDR rendering was disabled during the new activity checks. |
| Advanced independence / persistence | PASS, bounded | Actual separate SR/FG selection and restart observations retained; all advanced choices restored to Auto. No NVIDIA model/DRS activation claim. |
| Diagnostics and restart clearing | PASS | Independent details/overlay, both Off persisted, publication stopped at Settings close, opted-out data refused. Fresh menu session contains no prior-world activity. Constructed Advanced panel deliberately retains an observer. |
| Active resize | PASS, reused SDR scope | Earlier dev.25 windowed/maximize/borderless measurements; unchanged native/framework rendering bytes. New RenoDX presence does not qualify active HDR resize. |
| Sharpness | PASS, historical bounded endpoints only | dev.23 correction/counter sequence retained. Live external-value preservation/restoration is NOT RUN for this final candidate. |
| Keyboard slider, physical pause/resume, five minutes active play and dungeon/hub travel | NOT RUN | One supported Escape attempt ignored. Mouse settings work; stationary hub activity is not active gameplay. No new input framework. |
| In-game Save and quit | NOT RUN | Current runs cover main-menu Quit and window close. dev.23 Save-and-quit evidence is not transferred to this candidate. |
| Ordinary shutdown regression | FAIL | Full candidate C0000005 unresolved. Later clean exits do not repair it. Run 8 exited 0 with a clock gap, excluding comparable timing; recovery run 9 exited 0 without a gap. |
| Normal GUI uninstall | PASS | 17 owned paths absent, private DLSS removed, original proxy restored, 14 shared files unchanged, receipt/HDR lease removed, exact ReShade configuration restored. |
| Final preferences / installation | PASS, restoration scope | Original provider preferences restored by production CAS at revision 353; exact lease still owned after recovery startup. Loader shortcut blank/blank, display unchanged, Advanced Auto, diagnostics Off. 18/15 hashes match after final exit. |
| Physical HDR / controller / Windows NVIDIA activation | NOT AVAILABLE | Matching hardware absent. No physical FPS, frame-time-tail, latency-benefit or image-quality qualification. |

The [functional/recovery aggregate](../qualification/providers/windows-dev25/bounded-functional-2026-10-23.json) records measurements and limits. Its unchanged read-only SR observer and compiled dependencies still match the retained receipt. It validates a historical Apply-acknowledgement relationship, not a direct transition-history witness; the production transition predicate was not weakened.

First startup after reinstall reported HDR restart required while creating its owned RenoDX-disable lease. The source deliberately fails closed while RenoDX is already resident. HDR was requested Off and physical output remained SDR; no HDR activation/calibration pass is claimed. Normal exit leaves the disable token for the next launch.

Original preference provenance is the exclusive-create retained capture, exact store binding, revision/CRC and preserved timestamps. SHA was pinned at restoration, not independently recorded at historical capture. Exact preference bytes were restored while preserving current migration metadata. No player/account saves were read, restored or rolled back.

## Attribution and actual reporting

The [six-attempt ledger](../qualification/providers/windows-dev25/bounded-isolation-2026-10-23.json) includes one excluded Steam0042 startup. Direct candidate framework and bootstrap-plus-mandatory-bridges controls exited 0. Full candidate failed once; UI removal and add-back each exited 0. These samples establish neither ownership nor rates. The new null-target fault shares the earlier worker/unwind family; an identical freed allocation is not established.

The reduced failing proxy independently matches official ReShade 6.8.0. That reproduction contains neither our modified framework nor our payload. It is an unresolved Loader/ReShade compatibility finding. Already corrected descriptor corruption, long successful waits and tool/input failures remain separate.

[Blueprint Loader issue 1156615](https://www.nexusmods.com/minecraftdungeons2/mods/2?tab=bugs) was submitted and its saved body verified. The [ReShade report](WINDOWS_DEPENDENCY_SHUTDOWN_REPORT_2026-10-23.md) is ready but requires forum sign-in; no submission is claimed. Mojang reporting remains on hold: no ordinary unmodified-game crash or complete independent game-ownership proof. Raw dumps and proprietary disassembly stay private.

The next discriminating investigation is the worker's source owner and stop/join order in the failing reduced stack. Further clean repetitions alone do not establish that boundary. No speculative source patch, proprietary-game edit, exception suppression or forced successful exit was added.

## Handoff

Candidate installed, game closed, test holds and image-specific diagnostic settings cleared, temporary power request released before handoff. The PC remains on because manual/qualification work is pending, as required by the accepted plan.

1. Physical keyboard: edit render scale at 100%, Quality, Custom 77% and minimum; confirm number/preset/fill agree, preset-only changes keep Native selected, and unavailable DLSS is skipped.
2. Open pause, inspect Video/Mods, play actively for five minutes, travel to a dungeon and back, and validate recovered current-world SR/FG telemetry.
3. With a held process monitor, use in-game Save and quit. Any AV retains FAIL; one clean manual exit does not repair an intermittent fault.

Live external-sharpness ownership and a cause-directed exit regression remain required. ReShade forum sign-in is the only pending reporting-account step. No administrator installation or broader permission is currently needed. Do not merge or label Windows qualified from this record.
