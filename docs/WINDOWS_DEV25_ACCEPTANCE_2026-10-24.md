# Windows/Radeon dev.25 acceptance — 24 October 2026

**PASS WITH KNOWN ISSUES for the tested scope, under the maintainer’s acceptance rule.** This closes this qualification task. It does not establish a universal crash repair, eliminate earlier failures, or publish/merge a release. Historical records that say UNQUALIFIED retain their original observations and stricter decision rule; this record supersedes their acceptance decision.

The maintainer directed that clean tests should not be repeated indefinitely: faults that cannot be reproduced in the bounded follow-up may pass, while plausible recurring issues must have a usable reporting route. The later ordinary focus test passed. The intermittent shutdown fault remains disclosed and reported; independent reduced reproduction shows our graphics payload is unnecessary for that fault, without identifying the ultimate owner.

## Exact tested inputs

- Candidate **0.4.0-amd.dev.25**, requested source `e704a17c59e69ad4ec04191f43b0928ee0ad1097`; native artifact source `b5719f95820acb97dcd5fcdfa60bdde291b3379a`, identical tree `775604d9599c7ae1862eaeeb878f31c611cdcc65`.
- Steam build **25910487**, shipping SHA-256 `3842102bb05fc532b22e97992d61c2248d115be80ff752ff9b6843bf4acd7005`.
- Windows 11 build 26200, Radeon **860M**, driver **32.0.32015.7029**, 1920×1200 SDR borderless output.
- [Exact dependency lock](../qualification/providers/windows-dev25/dependencies-2026-10-23.lock.json), SHA-256 `0cebd7a90ef5338c1d0dcd32e71aece15456ee78199f08b20692d3601e202979`: Loader **2.6/main file 506**, candidate ReShade, Streamline **2.14.16**, DLSS **310.9.16**, FSR SDK **2.3.0**, RenoDX **0.2026.1023.0826**. Publisher inputs were checked on 23 October; RenoDX’s current asset digest was rechecked on 24 October and still matched `572124413ff392c3741f730f45896f4fc99753de9805d04887c484d4085f7c3a`.
- Current-dependency installer SHA-256 `4b441e9d819ccdd1b132cb3edc396e8c9c05ff239665ebacefb7d14e67d331c2`; logic source `97c92569169ec5efdb5c4ac7bf54b0460f8db6fe`. No production rendering code or binary was changed by this closing pass.

## Completed checks and limits

| Check | Result | Evidence and scope |
|---|---|---|
| Install, Verify, repair guards, dependency controls, uninstall/reinstall | PASS | Existing exact-candidate GUI and 147-check core evidence retained; final 18 owned / 15 dependency hashes match, overrides empty. |
| Native/FSR preset, numeric scale, unsupported DLSS skipping, persistence | PASS | Shared Quality 67%, custom 77%, minimum 33%, upper clamp 100%; Native retained. Mouse and bounded synthesized-key observations; no physical controller claim. |
| SR, FG and Anti-Lag activity / transitions | PASS, bounded | Existing current-session/revision/world advancing counters and explicit fallback results retained. Activity is not a physical FPS or latency measurement. |
| Advanced independence, diagnostics opt-out, sharpness ownership, SDR resize | PASS, bounded | Existing exact-source fixtures and ordinary results retain their separate diagnostic/runtime limits. |
| Gameplay, pause/system menu and travel | PASS observed functionality | Maintainer reported normal dungeon/hub travel and gameplay. Supported held-mouse menu-wheel input autonomously reached the normal System page. Independent five-minute physical active-play timing is not claimed. |
| Five-minute ordinary FSR rendering and focus switches | PASS, bounded | 301.547 seconds, +8,386 evaluations, no timer reset/provider error, then three inspected Alt-Tab out/back pairs without lines or crash. [Receipt](../qualification/providers/windows-dev25/ordinary-timed-focus-2026-10-24.json). Character stationary. |
| FSR Save and exit to main menu → Quit | PASS | Normal System action returned to the main menu; Quit and Confirm completed with held process exit **0**, no clock gap. Request marker to exit observation **88.774 seconds**. [Receipt](../qualification/providers/windows-dev25/fsr-save-quit-2026-10-24.json). |
| Installation, preferences and input restoration | PASS | All 18/15 files verify after final exit; exact revision-393 preference hash unchanged. Interact Left Mouse/None and Menu Wheel S/None visually restored before saving/quitting. No player/account saves inspected, copied or directly edited by tools. |
| Physical HDR, controller, Windows NVIDIA active features | NOT AVAILABLE | Matching hardware absent; not included in this acceptance. |

For the FSR exit route, only the ordinary control mappings were temporarily changed. A held left-mouse drag opened Menu Wheel; its gear led to the normal System page. Row-specific actions restored both original bindings before Save and exit. No diagnostic UI replacement, debugger, tracing request, game-code patch or forced termination was used. A current-world FSR Active snapshot is retained; a following Applying sample is excluded rather than presented as an advancing pair or source-acknowledgement proof.

## Known issues retained

1. **Intermittent focus-related GPU crash / Native black lines:** observed earlier, not reproduced in the later ordinary full-candidate focus follow-up. Passed under the maintainer’s bounded non-reproduction rule, with [the original incident and comparisons](../qualification/providers/windows-dev25/alt-tab-gpu-crash-2026-10-24.json) retained. No causal owner or source repair is claimed.
2. **Intermittent shutdown access violation:** real historical failures remain recorded. The [official ReShade + Loader control](../qualification/providers/windows-dev25/component-controls-2026-10-23.json) reproduced the worker-family fault without our UI, bootstrap, modified framework or graphics providers; today’s reduced candidate-framework/Loader control also exited C0000005. This establishes that our graphics payload is unnecessary, not which dependency or game component is responsible. Already reported to [Blueprint Loader](https://www.nexusmods.com/minecraftdungeons2/mods/2?tab=bugs) and [ReShade](https://reshade.me/forum/troubleshooting/10366-mcd2-intermittent-exit-av-with-reshade-6-8-0-blueprint-loader-2-6#48618). No reply is required for this acceptance.
3. **Long successful shutdown:** the final FSR route exited 0 after about 89 seconds. Keep this material delay visible even though it stayed within the 120-second observation window.
4. **Unexpected online main-menu return:** suspected online timeout, not independently attributed. It is not recorded as a GPU crash or substituted for the subsequently observed Save and exit action.

The [reporting guide](REPORTING_GRAPHICS_PROBLEMS.md) and bug form cover rendering settings, FG/HDR, display mode, overlays, exact focus/exit trigger, timing/frequency, visible versus shutdown-only faults, and sanitized error text. The existing public form is usable now; the additional dedicated questions take effect after branch integration. Raw dumps, account identifiers and full log folders stay private. No speculative source fix or unchanged clean-test matrix is scheduled.

The [machine-readable acceptance record](../qualification/providers/windows-dev25/acceptance-2026-10-24.json) links the evidence. Final state: game closed, candidate installed, exact preferences and controls restored, no outstanding user action. Direct in-game **Save and quit to desktop**, physical input/hardware coverage, independently timed active play, performance benefits, long-session reliability and universal crash repair remain outside the completed scope.
