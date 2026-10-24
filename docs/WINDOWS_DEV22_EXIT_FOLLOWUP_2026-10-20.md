# Dev.22 exit investigation follow-up

Windows qualification remains **FAIL**. This investigation supplements the [incremental checkpoint](WINDOWS_DEV22_QUALIFICATION_2026-10-20.md); it does not replace its failed ordinary exits or qualify a private diagnostic build.

## What the retained dump establishes

The three recent ordinary access violations occur at different instructions within the same worker function and have the same independently recovered caller chain and GameThread CRT-exit path. This supports treating them as one failure family for investigation. It does not prove a single root cause.

Offline inspection of the retained full dump now reports both exact head-like allocations as **LFH;free**. A separate read-only decode of the LFH busy bitmap agrees for both targets; neighboring control allocations are busy. This is stronger lifetime evidence than the earlier observation of a zero next link. It does not identify the freeing caller or establish that the free preceded the original exception: the fault thread waited in the private capture handler while other threads could continue before/during the external dump. Whole-heap integrity triage was stopped after a bounded analysis period and supplies no integrity pass. The dump has no handle-data stream or retained allocation/free call stacks.

The separate descriptor-heap corruption was reproduced and patched previously. Neither that patch nor this new free-state observation explains the ordinary worker AV yet. Clean delayed exits have different sampled waits and are not automatically the same defect.

## Comparison with working source

The qualified Windows baseline used source `e421e57e7667f0e9ec71e1940c6daa352a39f0c7`, Steam build `25647713`, Loader 2.3, Streamline 2.14.1, DLSS 310.9.1 and a September RenoDX nightly. Current dev.22 uses source `fe8b2b49c528c3f0bbe8393cab3e69a257406cee`, Steam build `25754144` and the current locked dependencies already recorded in the checkpoint. The historical pass is not an unchanged-input control.

From the first current-dependency failure to dev.22, the bootstrap, process-lifetime/exit guards, shared SDK shutdown header and both framework patches have unchanged Git blobs. The settings worker existed in the qualified baseline; its ordinary unload remains stop, wake, join, cleanup and module unload. Recent latency changes did not change that join. No individual source delta is established as the cause.

The working framework and current framework share the same base and successful-Reset patch. Current also includes the descriptor-heap lifetime fix. Both are MSVC builds; PE metadata records a compiler-build difference, which is not evidence of a compiler defect. Two previously retained framework-plus-Loader-2.6-only controls exited cleanly in approximately 6.47 and 5.28 seconds. Those controls used the same approved framework patch set but a different framework binary from exact dev.22; they do not establish current full-package stability.

## Bounded settings-worker comparison

Two private latency binaries were prepared from the verified generated dev.22 source with the same compiler, flags, headers, input filename and private read-only query export. The sole behavioral difference omits settings-worker construction. Three source-transform regressions and six owned-host lifecycle checks passed. Both builds retain the same ten existing export-redeclaration warnings.

The runtime control requests Native SR, FG Off and Anti-Lag Off. Each arm must reach the same starting menu, dwell at least 60 seconds and use the same ordinary close route, with loaded hashes and held-handle kernel exit timing. Worker-disabled status/UI functionality cannot be counted as a feature pass. A clean disabled arm alone cannot establish a fix; a failing disabled arm would show that this worker is not necessary for that failure in this control.

Both arms reached the normal starting menu and used Alt+F4 after at least 60 seconds of measured stable-menu dwell. Held process handles recorded kernel exit code 0 in both cases. Loaded game-directory module hashes differed only for the intended private latency addon. The common Native/FG-Off/Anti-Lag-Off intent remained byte-identical. Neither arm used a debugger, injected capture addon or forced termination.

| Diagnostic arm | Stable-menu dwell | Request to kernel exit | Result |
|---|---:|---:|---|
| Worker enabled, run 14 | 69.140 s | 93.612 s | Code 0; substantial unexplained delay |
| Worker disabled, run 15 | 66.875 s | 11.594 s | Code 0 |

The initial enabled run also started Steam, whereas Steam was already running for the disabled arm. The different delays therefore cannot be attributed to the worker from this single pair. Neither reproduced the intermittent AV: **the crash comparison is inconclusive**. No production crash fix, new release candidate or qualification pass is claimed. Both are private diagnostic binaries, so their exits do not count toward final ordinary-release exit qualification.

After both processes exited, the exact original latency DLL was restored. A fresh verification matched all 18 owned files and all 15 dependencies, overrides `{}`, and the unchanged installation receipt. The obsolete observer remains quarantined outside the game. Test preferences remain separately owned for continued work; final preference restoration is pending.

## Publisher listing refresh

A fresh publisher-page check still lists [Blueprint Loader 2.6 as the main file, ID 506](https://www.nexusmods.com/minecraftdungeons2/mods/2?tab=files), [Streamline 2.14.16](https://github.com/NVIDIA-RTX/Streamline/releases/tag/v2.14.16), [DLSS 310.9.16](https://github.com/NVIDIA/DLSS/releases/tag/v310.9.16), and [FSR SDK 2.3.0 at the already pinned commit](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/releases/tag/v2.3.0). The [RenoDX snapshot asset](https://github.com/marat569/renodx/releases/tag/snapshot) API digest still matches the installed UE Extended DLL. These listing checks do not establish feature activation or qualify a dependency update. No dependency was changed during the matched pair.

## Exact free captured in a failing run

After administrator access became available, a bounded Windows heap trace reproduced the recurring exit AV on the restored exact candidate. The ordinary close request ended `C0000005` after **14.082399 seconds**. The instruction and independently recovered caller chain match the earlier worker failure family. This diagnostic run is excluded from final ordinary-release exit qualification.

The reader was first verified against an owned program: both exact target frees matched distinct known caller frames, and wrong target addresses were rejected. The game trace reports zero lost events. It records one exact Free event for the initially self-linked second head, from GameThread through a **game CRT exit callback**. The first target has no Free event in this recording window; this is a partial two-target capture, not a complete two-target pass.

The captured return address identifies the pinned game's list-destruction callback. Its readable on-disk code clears the previous node's next link, releases the non-head chain, then releases the same head pointer. For the observed empty head, that cleanup produces next=0. The fault context retains that same target pointer. This identifies the concrete freeing path matching the earlier cleanup hypothesis. The callback's code page is absent from the minidump; its instruction sequence was inspected in the independently hash-verified disk image. No source-class name or safe production fix is established yet.

A separate latency-addon thread remains asleep in a condition-variable wait in the dump; it is a different thread from the faulting worker. No direct native-addon frame appears in the captured target-free stack. Indirect timing effects remain possible. The other target was already not self-linked at the initial observation, so it must not automatically be treated as a second circular list head.

No diagnostic addon or debugger was installed for this run. Windows recording stopped, its temporary image configuration was restored, and exact 18-owned/15-dependency verification passed again. Test preference restoration and further gates remain pending. Windows remains **FAIL**; this is new causal evidence, not a claimed repair.

## Clean current-game trace comparison

Normal GUI uninstall removed the candidate. An exact-hash transaction then temporarily held the remaining fourteen external dependency files and original proxy outside the game. Loaded modules confirmed system DXGI and no mod entrypoints. The clean game reached its normal starting menu and received Alt+F4; its held process handle recorded code **0** after **99.283774 seconds** from the pre-input marker, or **90.669146 seconds** after input returned. Input dispatch spans the difference. The menu dwell was not matched to the preceding run, so this is a bounded structural comparison, not a controlled timing benchmark or an ordinary candidate exit pass.

The clean trace contains 277,291 game heap frees and 490,431 game stack records, with zero lost events. Both target frees have captured stacks: the second uses the same `8ca7b00 → 8e4caae` game return path as the failing run; the first uses `8ca7b00 → 8e4ca6d`. The read-only timeline also observes the initially self-linked second head become zero-next. That cleanup shape therefore occurs in a successful clean-game exit too.

Thread rundown identifies one clean-game thread with the same generic entry RVA `8af026c` as the faulting thread. Its Stop event follows the second-target Free event by **75.1562 ms**. The generic entry alone does not identify its proprietary owning class, task state or stop cause. The failing run's faulting thread also remains present after the target Free event, but its eventual Stop follows Windows fault handling; the trace does not establish the original exception-dispatch timestamp. These events must not be presented as proof of a safe stop/join or a measured fault latency.

Recording stopped and the temporary image tracing key was restored to its originally absent state; fresh Windows queries report tracing disabled and the named session not recording. All fifteen held files were restored exactly. Normal GUI reinstall reselected the verified current DLSS download after uninstall removed its private copy. Fresh verification again matches all 18 owned files, 15 dependencies, empty overrides and the original installed receipt. No player/account saves were read or restored. The exit defect and remaining qualification gates stay open.
