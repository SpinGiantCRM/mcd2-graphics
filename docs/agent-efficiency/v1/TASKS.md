# Task and verification templates

Version 1. Fill only the relevant fields. A one-line doc correction does not
need a lifecycle checklist. Preserve existing acceptance gates for runtime work.

## Bounded implementation

```text
Goal and user-visible outcome:
Starting repository commit and branch; dirty work to preserve:
Issue/PR and overlapping work:
Reproduction or existing evidence:
Acceptance criteria, including one failure/fallback case:
Constraints and allowed files:
Validation budget: lightweight now; hardware/Actions only when authorized.

Inspect the affected contracts and tests. Briefly state the approach and the
smallest check that can reject it. Implement one reversible change, run the
relevant checks, and review the final diff. Correct failures before expanding.
Finish with the result, evidence, remaining gates and rollback. Ask only for
information that changes acceptance or for a required authorization.
```

## Bug reproduction

```text
Expected behavior:
Observed behavior and first failure:
Frequency; last known good and first known bad source/artifact if known:
Store/game build, OS/Proton, GPU/driver, dependency and loaded module hashes:
Minimal steps; settings; scene/lifecycle boundary:
Control differing in one input:
Acceptance: reproduce before, pass after, preserve negative/fallback behavior.
Public evidence reference (redacted; no private storage paths):
```

An unavailable environment is an unrun gate. An intermittent failure followed by
one clean run is not a causal fix. Keep all attempts and interruption reasons;
do not silently discard slow exits, failed launches or counterexamples.

## Plan execute test review

1. Plan: identify the contract, current evidence, uncertainty and smallest
   reversible change. Define acceptance before implementation.
2. Execute: edit only the task's scope, preserve dirty files and exact candidate
   identities, and keep public support statements bounded by evidence.
3. Test: reproduce the failure, exercise behavior and refusal/fallback, then run
   relevant regression checks. Use source-string checks only for the structural
   contract they actually protect. Keep real Windows checks where applicable.
4. Review: inspect the diff for lost ownership/retirement, incorrect frame or
   adapter identity, changed menu/visual behavior and exaggerated support claims.
   Use a fresh review context if a large patch would otherwise hide assumptions.
   A separate agent is optional and needs authorization under the active rules.
5. Handoff: record completed and pending criteria, what the user must inspect,
   and rollback. Do not label an entire feature verified when a required runtime
   or release gate is still pending.

## Verification record

```text
Candidate source commit; payload and dependency hashes:
Acceptance criterion | command or observation | result | platform | evidence
Reproduction before / behavior after / negative control:
Regressions; rework and corrective user messages:
Human active minutes / agent elapsed / test time / queue wait:
Input, cached, output, reasoning tokens if exposed; otherwise unknown:
Unrun gates and reason:
Rollback and restored-state evidence:
```

## Reviewable PR description

```text
Concrete problem and resulting behavior:
Why this bounded change preserves the relevant contract:
Checks run and what they establish:
Runtime/platform gates still pending:
Release impact and rollback:
```

Keep protected check names, existing Windows/Linux coverage and frozen release
assets. Before proposing a push, compare against the latest base and overlapping
PRs; re-run only checks affected by integration changes. Review approval is
separate from approval to publish a release or alter installed files.

## Diagnose before spending another attempt

For an unresolved failure, record the observed evidence, its category and the
next discriminating check. Distinguish an ambiguous requirement, missing input,
localisation error, implementation error, tool/build failure, inadequate test,
platform/runtime issue and integration problem. Retrying the same full build
without a changed input or new observation is not a useful experiment.

Use bounded search results and relevant source slices. After an edit, check
syntax or the narrow behavioral oracle where it can catch the specific error.
Keep acceptance and the next action in a short checkpoint when context resets.
The [research synthesis](RESEARCH.md) explains the evidence and its limits;
these practices remain subject to the existing required checks.
