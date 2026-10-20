# Windows AMD qualification: dev.22 incremental review

The [sanitized aggregate](../qualification/providers/windows-dev22/checkpoint-2026-10-20.json) records the same checkpoint. The [earlier dev.17 record](WINDOWS_DEV17_QUALIFICATION_2026-10-19.md) retains the preceding failed history. This documentation update does not change the pinned runtime inputs.


**Overall Windows status: FAIL. Windows and release qualification are not granted.** This sanitized incremental checkpoint is dated 20 October 2026 and records the evidence available at this checkpoint. The dev.22 ordinary startup exit access violation remains unresolved. Further checks and final restoration are pending.

The current candidate guide still describes dev.16. Its pre-release checks are applied here to separately pinned dev.22 bytes. The original Windows qualification checklist supplies the B/I/U/A/N/E gate identifiers; its historical versions, Loader requirement and unsupported-AMD assumptions are not substituted for this candidate's current dependency lock or AMD provider support. Earlier dev.21 failures remain retained.

## Exact inputs and verification

| Input | Verified value |
|---|---|
| Candidate / UI version | `0.4.0-amd.dev.22` |
| Requested source / reviewed HEAD | `fe8b2b49c528c3f0bbe8393cab3e69a257406cee` |
| Actual native-artifact source | `8444aadc28d7c3327a12caef4aa7c1aeed672499` |
| Identical complete source tree | `bfba19b2a41d762338842032c0619225061b4b32` |
| Successful native run / artifact | `39294350560` / `12206479195`, attempt 1 |
| Native archive SHA-256 | `af94e6be41bca8fe3dac55bfa5d01d56210711af97cbe4c844a3d24260d00e67` |
| Successful portable checks run | `39294350569`, attempt 1 |
| Installer EXE SHA-256 | `e591098e537e9ffdd37cd75a45b4e1d83f449bc259cc9855dccd337a42c274ec` |
| Managed GUI SHA-256 | `a6617e0c99adc8a8776746c1863fbf7745c769d9509ddba53528d67fec0aca15` |
| Installer Core SHA-256 | `22853b1bffde16234caa203224d28efb7ff30933a57e0add1b4a11c0b4551ca9` |
| Manifest SHA-256 | `3fba83676f17a4aef055f103f7af0e5ce67b83f540f4f1da7fa9769c2dc72e93` |
| Dependency lock SHA-256 | `d0d2d41c98f446d18895ea47b251079566c48c02c03816af1b03a12585727140` |
| Installer assembly receipt SHA-256 | `2a5d04beb9f619819354fa8d66dbc13dd32f3492a5996748b154498b59a30c7b` |
| Separately installed framework SHA-256 | `6b94ab48306b23da702c386abf853147218f5b6d6708a1ac8621aea414b7d676` |
| Assembly verification | 242 installer-folder entries, 47 native-artifact entries and 143 source pins |
| Installed verification checkpoint | All 18 owned files, including the private DLSS copy, and all 15 dependency files match; overrides `{}` |

The independent CI receipt agrees on source, successful native and portable conclusions, artifact identity and archive digest. The post-diagnostic installed receipt independently preserves the exact installed hashes. The aggregate includes the complete sanitized owned/dependency maps. This is verification of receipts and their agreement, not a new game-directory scan or a claim that CI qualifies runtime behavior.

Later read-only workflow metadata checks also confirmed [run 39294350565](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/39294350565) and [run 39294350619](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/39294350619) completed successfully at the same requested source commit. These links corroborate workflow metadata only; they do not replace the exact native-artifact attestation above or establish runtime qualification.

Fresh dev.22 installation was performed through the normal GUI, as directly observed by the parent qualification agent, and is corroborated by the final fresh-install receipt and exact 18-owned/15-dependency hashes with overrides `{}`. The report author did not independently repeat that interaction. A later fresh GUI follow-up exercised the separate FSR SR and FG override acknowledgement/warning/Use installed paths; each marked only its own card Untested, then returned to recognized with the override Off. SR remained Off while FG was tested. Closing/reopening the exact installer and checking requirements showed all seven cards recognized, with no override warnings or Use installed controls. Both installed hash maps and the install receipt remained unchanged. The other five override toggle paths were not freshly repeated, so full I7 remains incomplete; no unknown-future-dependency compatibility is claimed.

Fresh normal GUI intact Verify reported **“Installation and dependencies verified.”** For missing-file repair, only the verified owned `live_dense.cso` shader was backed up and removed; normal GUI Repair reported **“Repair complete”** and restored its original hash. For the modified-file gate, only the first byte of that shader was changed. Repair refused with **“Existing or modified file retained”**; confirmed normal GUI Uninstall then refused with **“Modified mod file retained.”** Uninstall stopped before removal: after each refusal, the fixture remained modified, the other 17 owned files and all 15 dependencies were unchanged. Ownership/hash checks restored the exact original backup. Final intact GUI Verify again reported **“Installation and dependencies verified.”** All 18 owned and 15 dependency hashes, overrides `{}` and the original installed receipt were then reverified. These GUI observations are directly attested by the parent qualification agent and corroborated by the private fixture ledger and installed receipts; the report author did not repeat the interactions. The game is closed, the shader fixture is restored and no temporary diagnostic addon remains installed at this checkpoint. This refusal test does not complete normal candidate uninstall I9.

The exact-source Windows installer Core fixture suite reports **147 checks passed**, using a private isolated metadata location. It includes unknown complete sets, individually recognized files mixed across releases, incomplete/missing members, ambiguous members, traversal/unsafe paths, symlink safety, unsupported targets, running-state refusal and explicit acknowledgement. This is a source-built Core test result at the pinned requested source, supported by the 143 source pins; it is not a claim that 147 tests executed against the exact shipped Core binary. Separately, constructor/metadata verification of the shipped Core confirms candidate/Core version dev.22, all seven dependency defaults Off and byte-matching embedded inputs. These offline fixtures do not qualify an unknown future dependency or game runtime behavior.

Dependencies are Blueprint Loader 2.6, RenoDX UE Extended `0.2026.1008.1941`, DLSS runtime `310.9.16`, Streamline `2.14.16`, official FSR upscaling runtime `3.1.5 / official 4.1.1`, and official FSR frame-generation runtime `3.1.6 / ML 4.0.1`. Installed versions and loaded libraries do not establish active model selection or ML device approval. Framework and vendor runtimes remain external dependencies.

The recorded platform is Windows 11, AMD Radeon 860M, driver `32.0.31041.1004`, Steam build `25754144`, shipping executable SHA-256 `3a8703406fd50520f83c4f70a0212c000cb3b584ef28eb38032902230c01ebdd`. These platform facts carry from the established machine baseline; this report does not claim a new hardware scan. Physical HDR is unavailable on this display. Other AMD devices, NVIDIA and Xbox PC remain outside this scope.

## Dev.22 game outcomes

| Trial | Bounded observation | Exit classification |
|---|---|---|
| Run 1 | Ordinary main-menu trial; Native SR, FG requested On at startup. No gameplay, successful SR evaluations or generated frames established. | Ordinary exit failed: `C0000005` after **6.905158 s**. This unresolved failure blocks qualification. |
| Run 2 | Interrupted session. | Excluded: no complete exit receipt. |
| Run 3 | Native FG and Anti-Lag activity; FSR six-preset checks; matched FSR Quality FG Off → On → Off observations. | Later live CDB attachment failed; no hardware watch was armed or debug registers written. Process then exited zero without a normal-close request. This diagnostic termination is **excluded from ordinary-exit qualification**; cause was not established. Earlier ordinary runtime evidence is retained up to attachment. |
| Run 4 | Startup owner Native (owner 0), FG Off and Anti-Lag On. Ordinary UI window/resolution transitions, FSR Quality evaluation and movement recovery. No capture, debugger or extra addon in this run. | Ordinary Alt+F4 from hub gameplay: held process handle, final kernel code `00000000`, **6.8994144 s** from immediate request marker to kernel exit, calculated with timezone-aware timestamps. One bounded clean close. |
| Diagnostic run 5 | Temporary hardware-write-watch addon; manual arming found the watched head readable and initially self-linked. All 108 armed threads were cleaned. Capture stopped on a new-thread condition with zero write-watch events. | Process exited zero without a normal-close request about seven seconds after arming. **Excluded from ordinary-exit qualification.** No causal writer was identified; instrumentation compatibility is suspected but unproven. The temporary addon was removed and exact 18-owned/15-dependency hashes restored. |

Run 4 changed through ordinary UI to Windowed 1680×1050. FSR Quality in the matching current request context rendered 1120×700 to 1680×1050, with error zero and **+557 evaluations / 10.135149 s**. A fullscreen transition to 1920-wide output resumed SR. A second toggle returned to 1680×1050; SR recovered automatically with **+432 / 10.117893 s**. Movement retained successful evaluation with **+485 / 10.114613 s**. A third toggle restored 1920-wide output, but restoration of the original borderless mode remains pending. The final FG On request is for next startup; owner 0 in this run is not active AMD FG evidence.

These dev.22 game resize observations apply **only to the Native presenter owner, FG Off**. They do not qualify an active AMD FG owner resize, pause/travel lifecycle, or the earlier dev.21 `ResizeBuffers` / `DXGI_ERROR_INVALID_CALL` failure. The earlier failure is retained as historical failed evidence, not erased by this bounded recovery.

## Provider and diagnostics observations

Run 3 Native FG On had validated advances of **+35 rendered / +35 generated**, with Anti-Lag +35. Native FG Off / Anti-Lag On had a validated Anti-Lag +31 observation. Native SR stayed inactive. With FSR Quality, the FG On observation advanced **+60 rendered / +60 generated**, and a warmed observation advanced **+57 / +57**; both also had advancing Anti-Lag. Returned FG Off had zero rendered/generated FG delta and Anti-Lag +62. These are independent provider counters, not physical FPS or latency-reduction measurements.

| FSR request, FG Off | Render → output | Native SDK evaluation delta | Interval |
|---|---|---:|---:|
| Balanced | 1130×706 → 1920×1200 | +352 | 10.126562 s |
| Performance | 960×600 → 1920×1200 | +491 | 10.144618 s |
| Ultra Performance | 640×400 → 1920×1200 | +535 | 10.100732 s |
| Native AA | 1920×1200 → 1920×1200 | +239 | 10.140612 s |
| Custom 77% | 1479×924 → 1920×1200 | +334 | 10.130806 s |
| Quality, run 4 | 1120×700 → 1680×1050 | +557 | 10.135149 s |

Quality at 1920×1200 was separately exercised in run 3. All six requested presets have bounded native SR evidence; this does not establish current official FSR 4.1 selection, a quality assessment, or a complete strict diagnostics pass.

Error zero, matching full request context, current session/world and observed source scale support the native SR observations. The strict collector still reports **`srAdvancingValidated: 0`**: it requires an ApplySource acknowledgement sequence equal to the later Active sequence. ApplySource acknowledgement is a phase-specific transactional gate. Its mismatch must not be hidden, weakened or described as a strict validation pass. No raw session/world identifiers or sequence values are published here.

## Presentation timing and independent bridge probe

Matched approximately 30-second settled FSR Quality / Anti-Lag On hub samples at 1920-wide output yielded ETW observed display-timestamp cadence of **38.953/s Off**, **57.598/s On**, and **38.169/s returned Off**. Provider generated counters are independent evidence. These are ETW-observed cadence values only: they do not identify physical panel FPS, distinguish base from generated presents, or measure input latency. The cold/ApplySource capture is excluded. This bounded cycle does not complete the full Off-recovery regression or post-release benchmark suite.

The parser was corrected to use the actual `CPUStartQPCTime` column with explicit PresentMon `--qpc_time_ms` attestation; six parser checks pass. Prior inferred-unit/schema results are superseded. No physical panel measurement is claimed.

The separate owned-device bridge probe passed with no game files changed. Two actual On intervals each produced **60 real + 60 generated** presents. Repeated 1904×1161 / 1920×1200 resizes and Native handoffs returned success. A deliberately held buffer refused resize with `887A0001`; releasing it recovered with result zero. This proves the owned bridge contract only. It is not game integration, active game AMD-owner resize qualification, a repair of the historical game resize failure, or an ordinary game exit.

## Original checklist gate accounting

Status applies to the **complete stated gate on current dev.22**. A bounded observation in a NOT RUN row is retained without promoting historical or incomplete work to a fresh pass.

| ID | Status | Current evidence / remaining scope |
|---|---|---|
| B1 | NOT RUN | Established Windows/GPU/driver/game baseline retained; no fresh complete HDR/VSync/limit/output baseline. |
| B2 | NOT RUN | Run 4 ordinary exit has held-handle/kernel evidence; the full current backup/recovery baseline is not reverified in this report. Saves are outside this report. |
| B3 | NOT RUN | No complete dev.22 normal uninstall/fresh-baseline restoration gate. Earlier dev.21 normal uninstall remains historical. |
| B4 | NOT RUN | Candidate folder contains a space and runtime launched; full fresh installer prerequisites/protection-warning gate not established here. |
| I1 | NOT RUN | Fresh unsupported-target Install refusal with unchanged files is retained; full supported identity/cards/wrong-folder gate needs completion. |
| I2 | PASS | Parent directly observed normal dev.22 GUI install with current locked dependencies; final fresh-install receipt corroborates version, 18 owned/15 dependency hashes and overrides {}. Report author did not repeat the interaction. |
| I3 | PASS | Fresh GUI Verify remained read-only during play; Install and confirmed Uninstall refused changes. All hashes and receipt retained. |
| I4 | PASS | Parent directly observed normal intact GUI Verify; all 18 owned/15 dependency hashes, overrides {} and installed receipt unchanged. |
| I5 | PASS | Only the backed-up owned shader was removed; normal GUI Repair restored its original hash. Exact 18+15 hashes and receipt unchanged. |
| I6 | PASS | Modified owned shader caused Repair and confirmed Uninstall refusals before removal; other 17 owned/15 dependencies unchanged. Exact backup restored and final intact GUI Verify passed with original 18+15 hashes, overrides {} and receipt. |
| I7 | NOT RUN | Fresh dev.22 FSR SR/FG acknowledgement, warning, Use installed, isolation and Off restoration passed; reopen showed all seven recognized/default Off. Other five toggle paths remain historical; full current seven-control gate incomplete. |
| I8 | PASS | Pinned-source Windows installer Core suite passed 147 fixtures covering unknown/mixed/incomplete/unsafe/acknowledgement cases. Separate exact-shipped-Core constructor check verified seven defaults Off and byte-matching embedded inputs; 147 tests are not claimed against that shipped binary. |
| I9 | PASS | Normal GUI uninstall removed all 17 removable owned paths, preserved 14 shared dependencies, restored original proxy and exact owned startup configuration. |
| I10 | NOT RUN | Normal post-I9 GUI reinstall and Verify completed with exact hashes; title launch succeeded, but sign-in failure prevented full startup/gameplay verification. |
| U1 | NOT RUN | Fresh visible version dev.22, Loader 2.6, correct apostrophe/no mojibake. Full main/pause metadata gate incomplete; font arrow is visually blank. |
| U2 | NOT RUN | Ordinary UI changes worked in bounded run 4; complete both-menu mouse/keyboard/layout gate pending. |
| U3 | NOT RUN | Six FSR presets and dimensions observed; full preset/slider/fill/minimum interaction gate pending. |
| U4 | NOT RUN | Final FG On is a next-startup request; relaunch/activity/persistence agreement pending. |
| U5 | NOT RUN | Bounded hub gameplay/movement observed; required five minutes and dungeon/hub travel not completed. |
| U6 | NOT AVAILABLE | No physical-controller result available. |
| A1 | NOT RUN | Exact installed hashes and Native/FG-Off observations available; full current loaded-chain/control gate incomplete. |
| A2 | NOT RUN | Legacy unsupported-DLSS before-scale-reduction fallback requests not repeated on current dev.22. FSR success does not satisfy this gate. |
| A3 | NOT RUN | Legacy unsupported NVIDIA FG/Reflex seeded preference fallback not repeated. Current supported AMD FG activity is a separate scope. |
| A4 | NOT RUN | Native-Windows Wine-only Reflex guard check not freshly evidenced. |
| A5 | NOT RUN | Optional-model-hint fallback gate not freshly evidenced. |
| N1 | NOT AVAILABLE | Requires suitable Windows NVIDIA hardware. |
| N2 | NOT AVAILABLE | Requires suitable Windows NVIDIA hardware. |
| N3 | NOT AVAILABLE | Requires suitable Windows NVIDIA hardware; AMD Anti-Lag counters do not substitute. |
| N4 | NOT AVAILABLE | NVIDIA performance matrix unavailable; AMD ETW observations are separately bounded. |
| N5 | NOT AVAILABLE | Native HDR display unavailable; no swapchain colour-space/format HDR pass. |
| E1 | FAIL | Ordinary run 1 access violation; only one dev.22 ordinary clean exit. Diagnostic zero exit and interrupted session excluded. |
| E2 | NOT RUN | Bounded exit timings retained; full prescribed delayed-exit observation sequence not complete. |
| E3 | NOT RUN | I9/I10 and original borderless-mode restoration remain pending. |
| E4 | NOT RUN | This sanitized mid-work report/aggregate exist; final report/restoration/release decision is not complete. |

## Current AMD pre-release scope

| Current candidate check | Status | Bounded evidence and missing gate |
|---|---|---|
| Native control | NOT RUN | Native SR inactive and FG-Off startup observed; full scale/low-latency-Off/limiter control pending. |
| FSR SR | NOT RUN | Six presets, actual dimensions and advancing SDK evaluations observed; explicit analytical/official implementation selection and strict diagnostics remain pending. |
| AMD FG | NOT RUN | Valid Native and FSR counter advances; pause/resume, travel, active AMD-owner output/window lifecycle and any supported ML selection pending. |
| FG Off recovery | NOT RUN | Matched Off → On → Off counters/cadence retained; complete warmed repeated recovery regression pending. |
| Anti-Lag 2 | NOT RUN | Valid independent FG-Off and FG-On counter advances retained; full eligibility/unsupported refusal and timing scope pending. |
| NVIDIA routes / preset C | NOT AVAILABLE | No suitable Windows NVIDIA hardware; no native DRS/profile-restoration claim. |
| Advanced Mods page | NOT RUN | Full three-tab/exclusive-glow navigation, independent official FSR SR/FG selections, Auto/reset and relaunch not completed. |
| Diagnostics | NOT RUN | Busy correction has bounded observations; strict SR validation is zero. Pause clearing, stale/disabled behavior and full recovery pending. |
| Sharpness | NOT RUN | Complete 0–100%, reset/relaunch/provider/external-change gate pending. |
| Lifecycle/output | NOT RUN | Native-owner game resize/SR recovery observed. Active AMD owner, pause/travel, original borderless restoration and actual colour-space/format checks pending. |
| Shutdown | FAIL | Startup ordinary-exit access violation unresolved. Three ordinary code-zero quits with retirement and delay review are not completed. |
| Installer | NOT RUN | I2-I6, I8 and I9 complete. Reinstall, Verify and title launch passed; full I1/I7 scope and I10 gameplay remain incomplete. |

Evidence preparation changes no runtime source or payload. The runtime observations and GUI maintenance above are attributed to the qualification agent; compiling this checkpoint used bounded offline receipt/report checks. Final settings/restoration, remaining fresh GUI scope, official FSR selection, pause diagnostics, travel and three ordinary clean quits remain outstanding. The extensive performance/image benchmark remains a separate post-release task; these observations do not grant release approval.


## Later ordinary exit and running-game installer follow-up

Fresh exact dev.22 GUI Verify succeeded while the game was running. Install and confirmed Uninstall each refused with **Close Minecraft Dungeons II before changing files**. Independent post-action checks preserved all 18 owned and 15 dependency hashes, overrides `{}` and the unchanged installed receipt. This completes I3.

Run 7 used ordinary mouse **Quit to Confirm** from the starting menu and completed with held-handle kernel exit code 0 in **5.813616 seconds**. The original Windowed Full Screen mode was restored through the ordinary Video UI at 1920x1200. An external read-only observer verified the exact game image and watched only two known pointer slots and their first two words; it used no debugger, injected code, register changes, memory writes or protection changes. A null next-link value persisted for about **3.30 seconds** during this successful exit. Consequently, that value alone does not establish corruption or identify the earlier failure's cause. Nominal polling was 2 ms, with a maximum observed gap of 24.12 ms; snapshots are not atomic.

Diagnostic runs 6 and 8 used a separate temporary one-shot page observer and are excluded from ordinary-exit qualification. Each exited 0 after an ordinary close request and captured an unrelated write elsewhere on the same page, with the watched node still self-linked at capture. Both restored the original page protection. The temporary addon was removed and exact 18-owned/15-dependency hashes reverified. Neither identified the target writer or repaired the recurring crash. The observer itself creates a thread before protection is applied; its startup work is a possible source of the unrelated write, which the retained packet cannot resolve.

The advanced Upscaling tab displayed Auto, analytical 3.1 and experimental INT8 choices on this GPU. A mouse round trip returned the selection to Auto. This establishes menu interaction only; it does not qualify optional INT8 activation, official ML support, keyboard navigation or controller input. Windows remains **FAIL** and further runtime/restoration checks remain pending.

Run 9 reproduced an ordinary exit failure after startup sign-in errors 0020 and 0064. Held-handle kernel exit was `C0000005` after **5.810680 seconds** from Alt+F4. Independent offline analysis recovered the same worker function, caller chain and GameThread CRT-exit path as run 1, but a different null-derived byte read at `0x3dac`. The dump lacks the relevant heap; it does not establish a zero next link or identify its writer. No diagnostic addon or debugger was active. This fresh failure retains the shutdown blocker.

Normal GUI uninstall then completed with the game closed. Independent before/after verification confirmed all 17 removable owned paths absent, including private DLSS; all 14 shared dependencies unchanged; the original proxy restored; both owned install/HDR receipts absent; and the exact ownership-safe startup configuration restored. Player saves were not read. This completes I9; the post-uninstall reinstall/Verify/launch gate is still being completed.


## Further reinstall and ordinary shutdown observations

After I9, normal GUI reinstall and Verify completed. All 18 owned and 15 dependency hashes and overrides `{}` matched the pinned candidate. Run 10 reached the title, but a startup sign-in error prevented gameplay. Its ordinary Alt+F4 completed with held-handle kernel exit 0 in **6.026806 seconds**. The full I10 gate remains incomplete.

Run 11 used a temporary Steam offline comparison and again failed to reach gameplay. Its ordinary Alt+F4 ended `C0000005` in **6.183625 seconds**, without a debugger or extra diagnostic addon. Offline dump analysis and independent CDB unwind recovered the same worker function, caller chain and CRT-exit path, now at a different null-pointer read. The relevant heap is absent, so this does not identify a writer or justify a source patch. Steam was returned online.

Run 12 was an interrupted diagnostic attempt with no arming or complete exit receipt. It is excluded. The temporary addon was removed and the exact 18-owned/15-dependency hashes reverified afterward.

A separate private read-only Active SR observer now has 32 isolated synthetic checks. It preserves the production transition predicate and the original collector, and distinguishes historical ApplySource acknowledgement consistency from exact transition acknowledgement. No fresh live observer result is claimed. Report exit 0 alone does not qualify SR; advancing, current-context validation is required. This preparation changes no runtime source or payload. Windows remains **FAIL**.
