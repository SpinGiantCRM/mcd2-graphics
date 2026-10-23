# Windows crash attribution and qualification plan

**Execution outcome, 23 October:** The bounded run began 11:12:49 UTC. The six-attempt isolation cap was respected; one full-candidate ordinary AV remains unresolved. Loader issue 1156615 and [ReShade topic 10366/post 48618](https://reshade.me/forum/troubleshooting/10366-mcd2-intermittent-exit-av-with-reshade-6-8-0-blueprint-loader-2-6#48618) were submitted; Mojang attribution is not established. Current-dependency functional checks, normal GUI uninstall/reinstall, original preference restoration and cleanup completed within the disclosed scope. [Results and remaining gates](../../WINDOWS_DEV25_BOUNDED_RESULT_2026-10-23.md) supersede the planning-time state below. Windows is UNQUALIFIED; this execution does not complete every qualification gate.

**Resumed outcome, 24 October local time:** At the maintainer's explicit instruction, remaining autonomous work continued without waiting for ReShade. Bounded keyboard editing and separate live-engine sharpness ownership passed; ordinary functional run 10 reproduced the AV. No new isolation matrix or production fix was added. Input workarounds did not permit active play/travel/Save and quit. Original provider preferences were restored at revision 360 and the exact 18/15 installation verified after removing the private fixture. The maintainer's newer instruction makes normal shutdown after the evidence push the final handoff action; it supersedes earlier leave-on wording. Physical power-off is not attested by the evidence record.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans inline, task by task. This replaces the previous interpretation that a matching crash without our payload should simply be sent to Mojang. Planning does not start live testing.

**Goal:** Isolate each consequential Windows failure far enough to fix our defects or report a minimal reproduction to the relevant dependency maintainer, then finish available qualification and restoration.

**Architecture:** Separate fault families, review available source first, and run only comparisons that close an identified gap. Treat our bootstrap and our framework modifications as part of our integration. A faulting game instruction does not establish Mojang ownership; a failure without our addon does not exonerate the remaining dependency stack.

**Tech Stack:** Existing exact dev.25 candidate, Windows/Radeon host, verified publisher dependencies, ordinary exit monitor, retained WER evidence, available mod/bootstrap/framework source and existing owned lifecycle regressions.

**Spec:** Maintainer's 23 October correction: isolate and route findings appropriately, fix our mod where responsible, do not attribute all crashes to Mojang, and replace the prolonged investigation with an actionable bounded plan. Preserve earlier rules: block hidden consequential effects, allow understood minor settings defects, and finish autonomous checks before handing over user-testable gaps.

## Constraints and success criteria

- Qualification remains incomplete. No report has been submitted. The maintainer has created a Mojira account; authenticated browser access has not been rechecked.
- Our payload is unnecessary for one ordinary matching worker fault with original ReShade plus Loader 2.6. The current record does not identify the responsible component or compare failure rates.
- Ordinary tests and diagnostic captures remain separate. Two clean runs are not exoneration, and a faulting instruction/module is not necessarily the source of corruption.
- No proprietary-game patches, exception suppression, forced success exits, blanket verifier activation, new input framework or general debugger tooling project.
- Fixes require a demonstrated source defect and an appropriate failing regression. Native changes require a new traceable build and affected Windows/Linux checks. Preserve frozen preview.1 assets.
- No raw dumps, proprietary disassembly, account/player saves or third-party binaries in Git or public reports. Keep exact private receipts and safe restoration.
- PASS / FAIL / NOT RUN / NOT AVAILABLE are independent per gate. An unresolved relevant mod/integration blocker stays blocked at the time limit; it is not waived or relabeled external.

## Review focus

1. Distinct faults must not be merged merely because they happen during shutdown or share C0000005.
2. The original proxy and candidate framework differ; direct loading and bootstrap loading also differ. Every comparison must name these changes.
3. ReShade modifications authored for our integration are our responsibility until their provenance/contract establishes otherwise.
4. Removing a module must actually remove its execution, confirmed by loaded hashes, rather than just turn off a UI option.
5. Manual coverage and uncertain restoration must stay visible instead of being converted to passes.

## Task 1: Evidence census, source review and frozen baseline — 25 minutes

**Files:** `qualification/providers/windows-dev25/component-controls-2026-10-23.json`, `heap-exit-2026-10-23.json`, `qualification/providers/windows-exit-heap-fix-2026-10-19.json`, `docs/WINDOWS_DEV22_EXIT_FOLLOWUP_2026-10-20.md`, exact dev.25 build receipts, current private hold/restore receipts.

- [ ] Create one compact failure ledger. For each instance record executable/dependency hashes, ordinary versus instrumented, route/dwell, exception operation and original instruction/caller family, target classification, exit status/delay and available limitations. Mark absent information unknown.
- [ ] Keep these independent: (a) fixed descriptor-heap corruption, (b) current worker use-after-free/AV family, (c) long successful shutdown/Present waits, and (d) debugger/input/timeout failures. The inconclusive clean heap attempt is not a crash or a successful exit.
- [ ] Compare the actual candidate framework receipt with its pinned base `4eb9056c76016aad6f98495d3bbda2d721106104` and the two recorded patches: successful Reset epoch and upstream descriptor-heap fix `64017f98cc1393db51d9f5b1307c64350680f41c`. Reuse the existing failing-old/passing-fixed WARP result for the separate descriptor defect; do not report it as a new unresolved bug.
- [ ] Review mod changes against the retained working source `e421e57e7667f0e9ec71e1940c6daa352a39f0c7`, focusing on `experiments/fg-streamline/bootstrap_dxgi.cpp`, provider retirement/worker ownership and `src/providers/shared_sdk_shutdown.hpp`. The working run used different game/dependency versions, so a source delta is a hypothesis, not attribution.
- [ ] Re-identify actual game/build, diagnostic state, preferences and current hold transaction. Last known state: mod uninstalled, 15 dependency/entrypoint files held. Verify current official dependencies once and pin the chosen set for the comparison; never update midway. Reuse safe existing transactions to stage each arm.

**Deliverable:** A small failure ledger, source-risk list and exact first A/B comparison. If source review already proves a defect, proceed to its regression instead of collecting more generic crashes.

## Task 2: Close the missing integration comparisons — 65 minutes, at most six new ordinary runs

**Files:** Existing hash-owned isolation helpers/receipts, bootstrap/framework source and a new run ledger. Use existing ordinary exit/Windows crash evidence; no new capture framework.

**Known comparison gap:** The updated-game failing reduced control used original ReShade SHA `0cee63f9...`, while the candidate uses framework SHA `9fdac099...` loaded as `d3d12.asi` through our `dxgi.dll` bootstrap SHA `0e758adc...`. That matrix does not isolate our framework/bootstrap integration from our native/UI payload.

- [ ] Prepare baseline A: exact candidate framework and same third-party dependency set, with our bootstrap, addons and UI absent. Use the previously supported standalone framework route only after verifying its exports/loading contract and preserving exact file receipts. Confirm actual loaded modules. Do not describe this as identical to the bootstrap route.
- [ ] Prepare B: same state plus our exact bootstrap, framework again loaded as `d3d12.asi`, no own addon/UI execution, Native/FG Off/Anti-Lag Off. A versus B tests the bootstrap/chaining integration; the explicit route change is the intervention.
- [ ] Start with one ordinary run per arm, same game/dependencies, warm Steam, settings, menu or hub, at least 90 seconds observed dwell, same normal window close and held process monitor. Include the third-party addons actually present in both arms; do not silently remove RenoDX from only one side.
- [ ] If B introduces the fault while A does not, use the remaining runs for a reversed-order remove/add-back comparison and source-guided bootstrap investigation. Clean A alone cannot prove the bootstrap is responsible.
- [ ] If A also reproduces the relevant fault, our own binary code is unnecessary in that trial. Choose one source/evidence-led dependency comparison: candidate versus verified unmodified framework, Loader removal, or another actually loaded addon removal. Change only that named boundary, preserve route, and use a remove/add-back check where possible. The old original-proxy result is retained evidence, not proof of stock/upstream ownership.
- [ ] If A/B do not reproduce but the full candidate does, keep B's third-party configuration fixed and add our own layers in the smallest viable groups: UI containers, native addons/bootstrap features, then active provider state. Select one group based on the source review and compare it with its absence; do not attempt every permutation within the budget. Validate actual loads and activity.
- [ ] Classify every new fault against the ledger. A new signature is separate until evidence links it. A delayed exit without an exception is a separate wait problem. Do not infer frequency changes from these small samples.

**Decision at the bound:** Proven source defect; smallest failing dependency/integration combination; or unresolved intermittent failure with an exact next discriminating test. Stop live repetitions at six runs or 65 minutes, whichever comes first. No silent extension or automatic diagnostic recapture. If starting Steam, loading or shutdown consumes the allowance, report that and reduce the run count.

## Task 3: Fix or route the demonstrated finding — 75 minutes, including build allowance

**Files:** Only the implicated source and existing owning tests, or a sanitized upstream report. The current Mojang report draft is on hold until this routing decision.

| Evidence | Required action |
|---|---|
| Defect in our addon, UI, bootstrap or integration-specific patch | Add the failing regression, make one narrow correction, run affected tests and Windows/Linux checks, build an exact candidate, then repeat the original trigger. |
| Defect isolated in unmodified ReShade source/behavior | Report to the maintainer of the implicated upstream/fork with exact base, minimal configuration, source finding if established, and reproduction limitations. Assess a supported workaround separately. |
| Loader-dependent failure | Report the minimal Loader/framework/game combination to Blueprint Loader's author; involve ReShade too if the boundary is an interaction. Do not claim a single owner without evidence. |
| Matching fault in genuinely unmodified game, or independently established game-owned lifetime defect | Report that specific evidence to Mojang via the new account. A game-owned free/fault alone does not satisfy the latter criterion. |
| Interaction localized but owner unresolved | Report the integration finding to the relevant maintainers with shared factual wording and links, explicitly stating unresolved ownership; do not blame Mojang by default. |
| Still unresolved at the bound | Preserve the relevant blocked gate and next discriminating test. Do not ship an invented repair or an unsupported attribution. |

- [ ] Check official destination and existing reports before posting. Use the maintainer's authorization to report findings, but do not claim an account is authenticated until checked. Keep factual code/configuration summaries public; sensitive raw evidence remains private.
- [ ] Confirm an actual ticket/comment URL for each submission. If authentication or tracker scope prevents submission, save a complete ready-to-send report and record the specific pending action; continue other work.
- [ ] If a source fix/build needs longer than this allowance, give the concrete defect, regression result and actual remaining build/test estimate at the checkpoint. Do not consume the remaining qualification time without reporting the changed scope.

**Acceptance:** Each demonstrated issue is fixed with evidence, routed to the supported destination, or explicitly unresolved. A clean retest without a causal correction is not a claimed fix.

## Task 4: Finish remaining available gates — 40 minutes

**Files:** `docs/WINDOWS_DEV25_UPDATE_2026-10-22.md`, existing strict runtime readers, settings/UI checklists and production preference receipts.

- [ ] Reuse unchanged installer/fallback/Native-FSR/shared-preset/numeric-scale/resize passes. If a code/dependency change affects a pass, rerun only those affected gates.
- [ ] Complete remaining mouse-accessible Advanced/Diagnostics paths: independent selections and persistence; opt-in/out and stale-context clearing; active versus requested provider/resolution truth. Validate effects with fresh session/revision/world data.
- [ ] Exercise Native -> FSR Quality -> Native and active supported FG during necessary functional runs; confirm Native returns to full resolution and unsupported choices do not silently lower resolution or misreport activity. Reuse these runs for ordinary exit evidence.
- [ ] Make one supported attempt at any still-untestable keyboard/gameplay interaction. If input remains unavailable, stop and give the maintainer exact steps for keyboard slider, pause/travel and in-game quit checks after autonomous work. Do not spend this block repairing synthetic input.
- [ ] Block on crashes linked to our integration, incorrect rendering/activation, hidden state changes, lost settings or unsafe writes. Defer small understood presentation defects with explicit impact. HDR/controller/NVIDIA hardware absent on this host stays NOT AVAILABLE.

**Acceptance:** Every gate has an evidenced result or a specific manual action; relevant unresolved crash gates are not waived.

## Task 5: Restore and publish evidence — 35 minutes

**Files:** Existing provider restoration client/receipts, Loader binding receipt, held-file transaction, final installation verifier, failure ledger and sanitized final report.

- [ ] Restore authenticated original provider/display/input/diagnostic settings through the existing ownership checks; yield to external changes. Restore held files and verify the final declared installation/recovery state.
- [ ] Confirm temporary diagnostic configuration, capture processes and power requests are cleared. Do not touch player/account saves.
- [ ] Validate the final evidence JSON and links, review the exact diff, commit only intended files and push to `development/amd-colour-resize`.
- [ ] Deliver completed gates, separate unresolved failures, actual report URLs, deferred minor issues and the short manual checklist. No full qualification claim while a required manual or consequential gate remains unresolved.

## Deadline and checkpoints

- Total execution budget: **four hours**, allocated 25 + 65 + 75 + 40 + 35 minutes. This is a decision-and-deliverables deadline, not a promise that every intermittent third-party defect is repairable in that time.
- First decisive checkpoint: **90 minutes**. State which hypothesis the evidence supports/rejects, exact runs completed and the chosen fix/report destination or unresolved boundary.
- Report a blocked run promptly; never wait for the full block to expire before saying it cannot proceed. No unchanged experiment after two attempts add no information.
- If source changes require more build time, communicate the measured state and revised estimate before extending. No another-day investigation without an explicit scope decision.
- No game launch, installation mutation, administrator change or external report was performed to write this plan. Plan review is the current checkpoint; the prior broad autonomous investigation remains paused.
