# Windows shutdown compatibility report

Both reports were submitted on 23 October: [Blueprint Loader issue **1156615**](https://www.nexusmods.com/minecraftdungeons2/mods/2?tab=bugs) and [ReShade topic **10366**, post **48618**](https://reshade.me/forum/troubleshooting/10366-mcd2-intermittent-exit-av-with-reshade-6-8-0-blueprint-loader-2-6#48618). Saved bodies were verified. ReShade displayed “Your message has been successfully posted.” The finding and source ownership remain unresolved; neither submission establishes a repair. The [reporting receipt](../qualification/providers/windows-dev25/reporting-2026-10-23.json) records actual delivery without publishing private screenshots or dumps.

## Evidence basis for the submitted ReShade report

**Intermittent shutdown AV with official ReShade 6.8.0 and Blueprint Loader 2.6; ownership unresolved**

During graphics-mod qualification, an intermittent shutdown access violation reproduced with our mod removed, leaving only official ReShade and Blueprint Loader. This is a compatibility finding, not attribution to either component.

Environment: Steam build 25910487, Windows 11 build 26200, Radeon 860M, driver 32.0.32015.7029, 1920x1200 SDR. The game executable SHA-256 is `3842102bb05fc532b22e97992d61c2248d115be80ff752ff9b6843bf4acd7005`. Loader is 2.6, Nexus main file 506. The full add-on ReShade proxy reports file version 6.8.0.2155, SHA-256 `0cee63f9c9f13f3ac909c5b4903f4dbb4b719a7ab3b4f13b0deaf83c814b94f7`.

The proxy exactly matches ReShade64.dll extracted from the [official 6.8.0 add-on setup](https://reshade.me/downloads/ReShade_Setup_6.8.0_Addon.exe), downloaded normally on 23 October. Setup SHA-256: `afe4c8f13048306307983b8b3d41d5bf00a86820440b0e57dea10950e1176445`. The setup was inspected as an archive; it was not executed for this provenance check.

1. Keep Steam running and launch the game normally with those two components.
2. Enter the hub and remain observed for at least 90 seconds.
3. Close normally with Alt+F4.
4. Inspect the held process exit status and Windows Error Reporting.

Expected: shutdown without an access violation. Observed: one of two ordinary runs returned `C0000005`, with a WER minidump; the other returned 0. No debugger or Application Verifier was attached. A read-only Windows display report completed before the failing close.

| Reduced configuration | Ordinary exit codes |
|---|---|
| Unmodified game, system DXGI, no Loader | 0, 0 |
| Official ReShade plus Loader 2.6 | 0, C0000005 |
| Loader only, system DXGI | 0, 0 |
| Official ReShade only, no Loader | 0, 0 |

Loaded modules and file hashes confirmed the layouts. Our UI, native addons, modified framework, FSR/Streamline and RenoDX were absent from the failing reduced control. Two runs per configuration do not establish rates, a causal interaction, or component exoneration.

Offline analysis places the fault in the game-worker/CRT-exit wait family also seen with the full candidate. A separate instrumented full-candidate capture establishes a freed-block read, but does not identify the owning source component or complete worker-stop order. It is excluded from ordinary reproduction counts. A later ordinary full-candidate failure targets null in the same unwind/caller family; an identical allocation cause is not claimed.

Our candidate framework includes a successful-Reset epoch patch and upstream descriptor-heap lifetime fix `64017f98cc1393db51d9f5b1307c64350680f41c`. That demonstrated descriptor defect is a separate, already corrected failure family. The official-proxy reproduction above contains neither patch.

- [Ordinary reduced matrix](../qualification/providers/windows-dev25/component-controls-2026-10-23.json)
- [Separate diagnostic summary](../qualification/providers/windows-dev25/heap-exit-2026-10-23.json)
- [Six-attempt candidate integration comparison](../qualification/providers/windows-dev25/bounded-isolation-2026-10-23.json)
- [Related Loader report 1156615](https://www.nexusmods.com/minecraftdungeons2/mods/2?tab=bugs)

Raw dumps and proprietary disassembly remain private. No base-game-only ordinary crash, save corruption, device reset or system crash is established. Is there a known shutdown interaction, or a supported source-level boundary that would discriminate Loader and ReShade ownership?

## Routing decision

The [Mojang draft](MOJANG_WINDOWS_SHUTDOWN_REPORT_2026-10-23.md) remains on hold. A game faulting instruction, game-owned free or matching stack does not independently prove Mojang ownership. Further live isolation is outside the completed six-attempt budget. The next investigation should identify the worker's source owner and stop/join order; repeating clean controls alone cannot answer that question.
