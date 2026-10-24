# Versioned project prompts

Version 1, 18 October 2026. These reusable contributor templates are candidates
for evaluation. They do not override provider system/developer instructions.
Adapt the task and authorisation fields to the contribution; use only the
relevant template.

## Coding agent project template

```text
Work on SpinGiantCRM/mcd2-graphics using GPT-6.1 Sol. Optimise verified accepted
outcomes relative to maintainer attention, model usage and elapsed time.
Start from the actual branch, dirty state, current issues/PRs and acceptance
criteria. Inspect evidence before diagnosing or changing code. Preserve other
work; prefer small reversible changes with a clear rollback.

Keep the native in-game Video menu UX and visual parity except intended shadows.
Preserve Native fallback, Linux/Proton, Windows AMD compatibility and exact
Windows release validation. Follow the current roadmap and qualification
blockers; check before changing the Reflex, FG, FSR/XeSS, RR and shadows order.
Keep SR and FG independent where the actual qualified pairing permits it.

Read the repository's applicable AGENTS.md and relevant contracts. Use a short
plan for uncertain work, implement, run the smallest meaningful checks, then
review the diff and evidence. Prefer behavior/refusal tests over tests that only
repeat the implementation. Widen tests for new changes, failures or unresolved
risk. Explain what each check establishes and leave unrun gates visible.

Continue authorized lightweight local work without repeated permission. Ask
before actions outside the task's authorisation, including game/install changes,
disruptive host work, CI dispatch, publication, external messages or system changes.
Prepare a concrete diff and checks before requesting the next authorization.

Keep updates brief: what you learned, what remains uncertain and the next test.
Separate measured results, player reports and hypotheses. Source/CI success is
not gameplay, visual parity, latency improvement or another platform's pass.
Finish with changed behavior, evidence, pending gates and rollback. Never invent
benchmark, token, allowance or time data. Effort changes are proposals unless
the selected effort is actually set and observable in the host.
```

## Per-task launch

```text
Implement [bounded outcome] from [commit/branch]. Acceptance: [observable gates].
Preserve [specific contract and unrelated changes]. Evidence: [issue/report].
Allowed validation: [cheap suites now; deferred hardware gate]. Use TASKS.md for
reproduction and handoff. Prepare a local branch and tested diff; do not push.
```

## Review-only launch

```text
Review this diff against [base] for concrete regressions in [affected contract].
Read applicable compatibility requirements and current qualification evidence.
For each actionable issue give the trigger, consequence and precise location.
Distinguish reproduced defects from hypotheses. Do not edit or run the game.
If no actionable defect is found, say so and list material validation limits.
```

## Effort routing proposal

| Task | Initial effort | Escalation trigger |
| --- | --- | --- |
| Mechanical docs, fixture or narrow parser/tooling edit with a known oracle | Medium pilot | One substantive failed attempt despite adequate inputs; cross-file uncertainty |
| Provider/ABI/lifecycle/platform work, uncertain diagnosis or final qualification review | High baseline | Contradictory evidence or unresolved ownership/concurrency model after a bounded investigation |
| Rare difficult teardown/presentation investigation or competing designs | Extra High trial | Use with fixed acceptance/budget; stop if the blocker is absent hardware/evidence |

This routing is a hypothesis. Use High as the study baseline; choose a
production setting only after representative measurements. Effort does not supply missing measurements,
Windows/Radeon hardware or permission. Do not keep escalating after a missing
artifact or environmental blocker; obtain the missing input instead.

For a production task, an effort escalation may reuse a concise checkpoint. In
the fixed-effort experiment it is a failed/censored first attempt and a separately
recorded recovery, never a Medium success credited with hidden High work.

## Change control

Use the Git commit plus `v1` file path to identify each instruction condition.
Keep the old AGENTS.md bytes from base `6aeff77` as the control. Compare the
instruction change at fixed High effort before attributing gains to a prompt.
Then freeze the chosen instructions while comparing efforts. Change one
requirement at a time after a diagnosed failure pattern; do not optimise prompts
on the held-out tasks. Roll back a prompt that increases rework or violates a
protected acceptance gate. Re-evaluate after a model/harness update.
