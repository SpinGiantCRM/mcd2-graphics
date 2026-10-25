# Research and optimisation decisions

Review revision, 18 October 2026. This is a selective synthesis of primary
research about coding agents and developer productivity, not a ranking of
reasoning settings. The studies below motivate interventions to test in MCD2;
they do not establish a GPT-6.1 Sol Medium/High/xhigh optimum. Keep the research
out of the always-loaded instruction file and consult the relevant section.

## Optimise the workflow as well as effort

A coding outcome depends on task specification, useful context, tools,
implementation, validation and integration. Reasoning effort changes only one
part. A missing Windows fixture, an inadequate oracle or a broken source-build
transformation will not be fixed by giving the same diagnosis more tokens.
Classify the observed failure first, then choose the intervention whose effect
can be measured. This is an engineering inference from the evidence below.

| Evidence | Study design and finding | MCD2 decision and limit |
| --- | --- | --- |
| [SWE-agent](https://arxiv.org/abs/2405.15793v3), 2024 | Software-repair benchmark with interface ablations. Editing guardrails, search presentation, file windows and context management changed resolution rates. | Use bounded search output and immediate syntax feedback where available. The studied GPT-4-era interface effects are not Sol effect sizes, and a universal 100-line window is not established. |
| [Agentless](https://arxiv.org/abs/2407.01489v2), 2024 | A staged localisation, repair and validation pipeline competed strongly on SWE-bench Lite at comparatively low cost. Reproduction and regression tests filter candidate patches. | Start a bounded bug fix with localisation and a reproduction oracle. Do not add orchestration simply because it is available; its benefit needs a matched baseline. This does not show fixed pipelines win on every task. |
| [ReAct](https://arxiv.org/abs/2210.03629v3), 2023 | Interleaved reasoning/action evaluated on QA and interactive environments; environment feedback helps ground decisions. | Prefer a discriminating tool observation over repeated unsupported speculation. It is not a coding-effort trial or a reason to request private chain-of-thought. |
| [Reflexion](https://arxiv.org/abs/2303.11366), 2023 | Feedback and episodic verbal memory improve subsequent trials on coding and interactive benchmarks. | Retain a concise observed failure and next check in the checkpoint. Do not turn one self-diagnosis into permanent policy or an unbounded reflection loop; feedback quality and retry cost matter. |

The direct repo example is #54's source-boundary failure after a long native
build. A generation-only failing fixture would add information earlier than
repeating that build. The next change should preserve the generator's refusal
contract and add an early oracle; it should not remove the assertion to make CI
green. That change belongs with the active generator work, not this guidance PR.

## Keep context relevant and test instruction changes

| Evidence | Study design and finding | MCD2 decision and limit |
| --- | --- | --- |
| [AGENTS.md efficiency study](https://arxiv.org/abs/2601.20404v2), 2026 | Paired configurations over 124 PRs in 10 repositories; reports lower median runtime and output tokens with context files. | Measure attention and usage as well as correctness. The association under this harness is not a universal prescription to add instructions. |
| [Evaluating AGENTS.md](https://arxiv.org/abs/2602.11988v3), September 2026 revision | Coding-agent evaluation across SWE-bench and developer-instruction tasks; context did not generally improve success and increased inference cost. | Keep unusual project constraints in AGENTS.md and detailed task guidance on demand. Evaluate changes rather than filling a repository overview automatically. The studies use different settings and outcomes. |
| [Lost in the Middle](https://arxiv.org/abs/2307.03172v3), 2023 | Position-controlled retrieval and multi-document QA show sensitivity to where relevant evidence occurs in long inputs. | Put the active acceptance criteria and checkpoint where they are easy to find, and link history. Older models and retrieval tasks do not establish Sol's context-length failure threshold. |
| [NoLiMa](https://arxiv.org/abs/2502.05167v3), 2025 | Long-context retrieval removes literal overlap, revealing degradation missed by simpler needle tests. | Large advertised context is not proof that indirect ownership/lifecycle evidence will be used correctly. Test task-relevant context, not only exact-string retrieval. Synthetic retrieval is not native code repair. |

The initial 429-word AGENTS candidate repeated generic process guidance. The
review reduces it to 298 words while preserving the original 171-word platform
and release contract. It also removes an invented requirement to reread all
historical compatibility material after each edit. This reduces instruction
volume; whether it reduces rework or model usage remains unmeasured.

## Separate useful compute from expensive repetition

| Evidence | Study design and finding | MCD2 decision and limit |
| --- | --- | --- |
| [OpenAI Sol maintenance case study](https://developers.openai.com/blog/maintaining-software-with-the-gpt-6-family), October 2026 | Author-reviewed maintenance findings compare Sol Medium and Ultra, including follow-ups and research subagents. More findings and lower cost per finding favour different configurations. | Measure both completeness and cost. Ultra is not xhigh; this is not a repeated three-effort MCD2 experiment. |
| [Scaling Test-time Compute for LLM Agents](https://arxiv.org/abs/2506.12928), 2025 | Ablates parallel sampling, sequential revision, verification/merging and rollout diversity. Scaling can improve performance, with strategy-dependent effects. | Fix sampling, retry and selection policy in the effort study. A selected best patch's success must include the cost of discarded candidates and the selector. |
| [Cost-effective Agent Test-Time Scaling](https://research.google/pubs/cost-effective-agent-test-time-scaling-2/), 2026 | Budget-aware search-agent experiments account for both token and tool-call costs. Sequential and parallel expansion have different failure/cost patterns. | Record model, tool, build and queue costs separately. Its search benchmarks do not validate the CATS algorithm or claimed savings for this repository. |
| [Towards a Science of Scaling Agent Systems](https://arxiv.org/abs/2512.08296), revised study | Controlled tools/prompts/compute across architectures and task types show that coordination benefits depend on decomposability; sequential tasks can degrade. | Keep the one-agent baseline. Test delegated, non-overlapping work separately only when authorised. Ownership debugging with tightly shared state is not automatically parallelisable. |
| [Why Do Multi-Agent LLM Systems Fail](https://arxiv.org/abs/2503.13657v3), 2025 | Annotated traces and a failure taxonomy identify specification, coordination and verification failures. | If delegation is later tested, require bounded roles, explicit handoff evidence and one integration owner. More agents do not themselves provide independent verification. |

Fixed-effort trials must exclude hidden higher-effort recovery. Adaptive routing
is a separate policy experiment whose costs include escalation, handoff and
verification. Extra effort is plausible for unresolved analysis with adequate
inputs; it is an untested remedy for absent hardware, stale receipts or a broken
build environment. Keep the comparison baseline fixed until local evidence supports a change.

## Verify the tests as well as the patch

| Evidence | Study design and finding | MCD2 decision and limit |
| --- | --- | --- |
| [SWT-Bench](https://arxiv.org/abs/2406.12952), 2024 | Real issue reproduction tests are checked against pre-fix and fixed code; generated tests can filter proposed repairs. | Require base-fail/control-pass for each study grader and inspect the actual failure. A compile/import failure is not automatically reproduction of the reported bug. |
| [EvalPlus](https://arxiv.org/abs/2305.01210), 2023 | Expanded inputs, including mutation-driven generation, expose wrong code missed by smaller HumanEval suites and can change rankings. | Add discriminating negative/boundary cases where a demonstrated oracle gap exists. Do not equate test count or source-string matching with behavioral coverage. Function synthesis is narrower than a GPU lifecycle. |
| [SWE-rebench](https://arxiv.org/abs/2505.20411), 2025 | Continuously collected interactive Python repair tasks address stale benchmark/contamination problems. | Include fresh prospective work and remove later Git history from historical exports. Public historical tasks cannot prove absence of memorisation. |
| [SWE-Bench Pro Verified](https://arxiv.org/abs/2609.08149v3), October 2026 revision | Audits task defects and leakage channels and refines instances and evaluation boundaries. | Check grader scope, hidden-answer access and prompt/test agreement independently. A newer benchmark title is not a guarantee of complete validity. |
| [tau-bench](https://arxiv.org/abs/2406.12045), 2024 | Tool/user simulations use final-state grading and repeated-trial reliability, including pass^k. | Report repeat consistency separately from at-least-one success. Its policy-driven retail/airline tasks are not an estimate of MCD2 coding reliability. |

The CI tests in this PR execute the actual embedded native gate and inject an
early failure into the real Windows recipe block. They establish those
contracts, not GPU execution or gameplay. Both-platform hosted checks and exact
Windows/Radeon qualification remain separate. Do not add dozens of tests that
only restate source strings to make a numerical coverage total look stronger.

Repeated runs on the same task form a cluster. The revised study reports
per-task success and paired differences; it does not treat 54 attempts as 54
independent tasks. Generalisation uncertainty requires more distinct tasks,
not a naive binomial interval over correlated repeats.

## Measure maintenance and human work

| Evidence | Study design and finding | MCD2 decision and limit |
| --- | --- | --- |
| [SlopCodeBench](https://arxiv.org/abs/2603.24755v2), May 2026 revision | Iteratively extended solutions are assessed for functionality, verbosity and structural erosion. Passing checkpoints can coexist with accumulating design problems. | Include next-change difficulty and regression follow-up, not only the first green patch. The study's Python metrics do not mechanically score C++ ownership design or prove this repo is degrading. |
| [SWE-EVO](https://arxiv.org/abs/2512.18470v6), May 2026 revision | Release-note tasks require coordinated changes across files and preserve existing functionality. | Evaluate a small multi-step maintenance task separately from routine fixes. Do not transfer cross-benchmark percentages, especially when model/harness conditions differ. |
| [METR developer RCT](https://metr.org/blog/2025-07-10-early-2025-ai-experienced-os-dev-study/), 2025 | Randomised AI access for experienced open-source maintainers measured task completion time and found a slowdown in that setting. | Record actual review/correction time; perceived speed is insufficient. It is early-2025 tooling on a specific experienced cohort, not a Sol forecast. |
| [METR follow-up](https://metr.org/blog/2026-02-24-uplift-update/), February 2026 | Later estimates are complicated by recruitment/task selection and time-accounting problems. | Preselect tasks and retain abandoned/censored trials; separate waiting from active work. Do not use a selected success-only sample to claim productivity. |
| [Three developer field experiments](https://pubsonline.informs.org/doi/10.1287/mnsc.2025.00535), [author manuscript](https://economics.mit.edu/sites/default/files/inline-files/draft_copilot_experiments.pdf) | Randomised coding-assistant access across three companies and 4,867 developers reports higher completed-task throughput when pooled, with noisy site estimates. | Positive workplace results and METR's slowdown can coexist: tool, users, task mix and outcome differ. Throughput is not this user's personal-time saving or proof of identical quality. |

Track maintainer-active seconds separately from evaluator-active seconds and
report the combined human cost. A shorter model run can be worse if it produces
more review, integration or regression work. Conversely, more elapsed compute
may be acceptable when it saves attention and runs in an authorised quiet window.

## Apply the evidence in stages

1. Preserve a single-agent High baseline and the protected graphics contracts.
2. Use concise context and reproduction-driven checks; log which failure each
   change addresses. The root instructions link optional detail rather than
   loading this research on every turn.
3. Confirm the CI changes on Windows. Keep findings about build checks separate
   from native runtime qualification and the active #54 generator work.
4. Validate task graders and screen instructions at fixed effort on separate
   development tasks. Freeze that bundle before comparing reasoning effort.
5. Run the capped effort screen only under its own agreed budget. Record all
   attempts and defer any winner until uncertainty and acceptance justify it.
6. Study context, retries, tool presentation or delegation as separate changes
   only when local traces identify them as a bottleneck. Retain rollback.

The bibliography is selective, spans older foundational work and 2026 revisions,
and contains differing evidence levels. Tool-interface and Agentless method/
ablation sections were inspected in full-text form; the remaining papers are
used only for the design/results stated in their primary abstracts or relevant
primary source sections. No claim of exhaustive systematic review, replication
or Sol-specific effect size is made. The model's supported settings remain
specified by [official OpenAI documentation](https://developers.openai.com/api/docs/models/gpt-6.1-sol).
