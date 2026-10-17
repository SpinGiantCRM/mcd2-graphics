# FSR owned-resource retirement correction

FSR generation retirement manually released `EvalOwned.resources` in reverse
order, then destroyed the generation without clearing that list. `EvalOwned`'s
destructor released the same references again. This is an ownership bug regardless
of whether a particular device/resource survived the extra release.

The correction clears the list immediately after the manual releases, before
unloading the bridge and destroying the generation. SDK destruction, completed
GPU work, recording leases, failure retention and native-history reset gates are
unchanged. The existing native FG guide path uses the same clear-after-release
rule.

## Evidence and limits

The [numeric receipt](../qualification/providers/fsr-owned-retirement-2026-10-14.json)
records the old/new source and fixture hashes and exact test outcomes.

`tests/test_sr_owned_retirement.py` compiles the actual `EvalOwned` definition and
the actual FSR retirement release block against synthetic reference counters.
The pre-correction source compiles and fails the ownership check. The corrected
source passes 14 cases covering empty/populated lists, null entries, distinct and
duplicate owned references, external references, bridge unloading and release
order. The executable check is required in Linux and Windows portable CI.
The source-order guard and existing lifetime/FSR/guide checks also pass locally.

This proves the double release and its correction. It does not establish that
every earlier timeout, hang or `0xc0000409` was caused by it. Corrected game
artifacts require fresh lifecycle/exit checks; native Windows/Radeon qualification
remains pending. No published release file or tag is replaced.

A private corrected-native control retired FSR to Native at 100% with AMD FG
continuing, and exited normally. Reactivation with the previous UI reproduced
the separate [menu readiness failure](FSR_MENU_READINESS_2026-10-14.md). All three
CI jobs for the ownership correction passed; these do not qualify a new complete
runtime artifact.
