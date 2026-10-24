# Intermittent Windows shutdown access violation — third-party modules present

**Submission status:** DRAFT ON HOLD FOR ATTRIBUTION, not submitted. The official project portal denied access at preflight despite the maintainer having created a Mojira account. The finding was instead submitted as unresolved compatibility evidence in [Blueprint Loader issue 1156615](https://www.nexusmods.com/minecraftdungeons2/mods/2?tab=bugs); the same unresolved finding was submitted as [ReShade topic 10366/post 48618](https://reshade.me/forum/troubleshooting/10366-mcd2-intermittent-exit-av-with-reshade-6-8-0-blueprint-loader-2-6#48618). No ordinary unmodified-game crash or complete independent game-ownership proof is established. Do not submit this draft to Mojang automatically.

## Report text

During Windows graphics-mod qualification, an intermittent access violation occurred while closing Minecraft Dungeons II. The matching fault also reproduced after removing our graphics mod, while retaining Blueprint Loader and the original ReShade proxy. We cannot currently attribute it to the game, either dependency, or their interaction.

Environment: Steam build 25910487; Windows 11 Pro build 26200; AMD Radeon 860M; driver 32.0.32015.7029; 1920x1200 SDR. The shipping executable SHA-256 is `3842102bb05fc532b22e97992d61c2248d115be80ff752ff9b6843bf4acd7005`.

The smallest configuration in which we observed the matching failure contained Blueprint Loader 2.6 (Nexus main file 506) and the pre-existing ReShade 6.8.0.2155 proxy. Our UI, native graphics addon, FSR, Streamline and RenoDX payloads were absent. Loaded-module checks confirmed that reduced configuration.

Reproduction attempted:

1. Keep Steam running and launch the game normally with the components above.
2. Reach the hub and leave it observed for at least 90 seconds.
3. Close the game normally using Alt+F4.
4. Observe the process exit status and Windows crash reporting.

Expected: process completes shutdown without an access violation.

Observed: one of two ordinary runs in that reduced configuration returned `0xC0000005`; the other returned 0. The failing close reached completed exit observation after approximately 9.28 seconds. This interval includes input dispatch and monitor polling, so it is not a precise intrinsic shutdown duration. Neither a debugger nor Application Verifier was attached. A read-only Windows display report completed before closing the failing run.

The same updated game, warm Steam state, at least 90 seconds of observed hub dwell and normal close route were used for this small matrix:

| Configuration | Ordinary exit codes |
|---|---|
| Unmodified game, system DXGI, no Loader or graphics payload | 0, 0 |
| Original ReShade plus Blueprint Loader 2.6 | 0, C0000005 |
| Blueprint Loader 2.6 only, system DXGI | 0, 0 |
| Original ReShade only, no Loader | 0, 0 |

These two-run samples do not establish an interaction defect, failure rates, or that any component is safe. In particular, we have not reproduced this fault in an ordinary unmodified-game run.

A separate diagnostic run with the full graphics candidate, Native rendering, frame generation Off and Anti-Lag Off used image-specific Application Verifier Heaps and a WER full-memory dump, with no attached debugger. The original invalid read targeted an allocation independently classified as freed. Its retained free stack passed through a game-owned CRT exit callback, while a game static slot still held the exact pointer used by the worker. This establishes a use-after-free in that diagnostic instance; it does not establish the owning source component, complete thread shutdown order, or that every ordinary fault has the identical cause. Instrumentation changes heap behavior and is excluded from ordinary test results.

A subsequent instrumented unmodified-game attempt was inconclusive: the observation deadline elapsed without a requested normal close or a conclusive exit receipt. It supplies no additional crash or clean-exit result.

No save corruption, blue screen, device reset or system crash has been demonstrated for this captured fault. We have not verified save integrity. No proprietary game patch, exception suppression or forced successful exit has been applied.

Sanitized evidence:

- [Ordinary component matrix](https://github.com/SpinGiantCRM/mcd2-graphics/blob/97c92569169ec5efdb5c4ac7bf54b0460f8db6fe/qualification/providers/windows-dev25/component-controls-2026-10-23.json)
- [Separate full-memory diagnostic summary](https://github.com/SpinGiantCRM/mcd2-graphics/blob/97c92569169ec5efdb5c4ac7bf54b0460f8db6fe/qualification/providers/windows-dev25/heap-exit-2026-10-23.json)

Raw memory dumps and proprietary runtime disassembly are retained privately and are not attached or published. If this modified-environment finding is outside the tracker scope, please direct us to the appropriate technical reporting channel.

## Routing notes — not part of the report

- Official tracker: https://bugs.mojang.com/browse/MCD2
- Inspected potentially related issue: https://bugs.mojang.com/browse/MCD2/issues/MCD2-546 . It describes online-loading error 0056 and a system freeze on exit; those symptoms are not established as our fault, so it is not a confirmed duplicate.
- [Mojang reporting guidelines](https://help.minecraft.net/hc/en-us/articles/4408887473421-Mojang-Bug-Tracker-Guidelines) require checking an unmodified environment and direct mod-only failures to the mod author. Our report must disclose the clean controls and third-party reproduction rather than imply a clean-game reproduction.
- Do not claim submission without an actual ticket/comment receipt. Do not upload full dumps as ordinary public attachments.
