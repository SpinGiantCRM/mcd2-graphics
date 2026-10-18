# Reasoning effort comparison protocol

Version 1, 18 October 2026. Status: designed, not run. No repository-specific
Medium, High or Extra High outcome/cost ranking is established. High is the
comparison baseline for the proposed pilot.

## Decision and objectives

Hold the model at GPT-6.1 Sol. Find the least expensive setting that reliably
passes the task's acceptance gates, including review and relevant regressions.
Report accepted tasks per active human hour, per model usage unit and per
elapsed hour separately. Do not combine them into an arbitrary weighted score.
Select a Pareto-efficient policy subject to the maintainer's quality threshold.

Primary outcome: independently accepted first attempt, with no substantive
human correction. Secondary outcomes: final acceptance after recovery, human
active minutes, end-to-end elapsed time, tool/test time, corrections/rework,
critical regressions, and input/cache/output/reasoning tokens when exposed. Separate the
maintainer's active time from evaluator time, even when the same person fills
both roles; report each and their combined human cost.
Include failed, timed-out and abandoned attempts in costs and denominators.
Task throughput is total accepted / total cost, not the average of successful
runs' costs. A timeout is not a free failure.

## Separate the interventions

First compare the original AGENTS.md at `6aeff77` with v1 guidance at fixed High
on separate development tasks excluded from the main effort evaluation. A tiny
instruction-only screen is diagnostic, not enough
to declare improvement. Freeze the selected prompt bundle, task launch text,
tool access, harness version and validation budget before comparing effort.
Do not simultaneously change prompts, effort, tools, CI and task selection and
then attribute the combined result to reasoning effort.

`tasks.json` defines six candidate tasks: three routine and three hard. It pins
historical source revisions but is not yet a validated benchmark. A preparer
must export each base without later Git history, remove answer-bearing reports,
and freeze a task description plus public and hidden behavior graders. Confirm
the grader fails on the base and passes a known correct control, and inspect
that failure for relevance. Record hashes and platform requirements before any
model run. Reject unsuitable tasks before seeing effort outcomes and replace
within the same stratum with a documented reason. Historical solutions can be
memorized; include at least two fresh prospective held-out tasks before adopting
a routing policy for production.

## Bounded run design

Screen: one routine and one hard task × three efforts × two repetitions = 12
attempts. This tests the harness and metrics; it is not a conclusive ranking.
Main comparison: six tasks × three efforts × three repetitions = 54 attempts,
including compatible screen attempts only if the protocol did not change.
The nine attempts per task are repeated observations, not nine independent
tasks. Add prospective tasks to improve generalisation rather than merely
repeating a single familiar bug.

Use Latin-square order across Medium/High/Extra High within task and repeat;
counterbalance task order using a recorded seed. Start every attempt in a new
context and isolated export at the identical pinned revision, with identical
cold/warm cache policy and no access to another effort's patch or history.
Keep network policy fixed; for offline historical coding tasks disable browsing
and remote Git so later public fixes cannot leak in. Give all efforts the same
required evidence and tool results. Do not ask an agent already familiar with
the solution to replay it as an independent sample.

Set and verify actual host/API effort (`medium`, `high`, `xhigh`); a prompt that
says “use Extra High” does not set the parameter. Pin model snapshot if the host
exposes one; otherwise record alias, UTC date and host release and keep blocks
close in time. Fix mode, speed/service tier, tool schemas, context budget,
compaction policy and agent count. Use one agent per attempt. Ultra, parallel
subagents, Pro mode and Extra High are not interchangeable treatments.

Proposed caps before starting: routine 15 elapsed minutes and 30,000 total
reported tokens; hard 30 minutes and 60,000. Use the same cap within each
stratum, record the actual enforced mechanism and retain censored outcomes.
If subscription tokens are unavailable, enforce elapsed/tool-call caps (80 and
150 tool calls) and mark token cap unenforceable; do not pretend it was met.
Stop the screen at 12 attempts, 4.5 agent hours, or a separately approved
allowance/spend cap, whichever comes first. Missing spend telemetry requires
an agreed subscription allowance budget before launch. No automatic model
calls, Actions, game launches or heavy builds are configured here.

Record both agent completion latency and time-to-accepted outcome, including
queue time and required human review. Time spent by the user reading, answering,
reviewing or correcting counts as active time; unrelated activity during agent
waiting does not. Capture start, finish and acceptance timestamps independently.
Blind the reviewer to effort labels and randomise diff order. Use the same
rubric and required platform gates for every condition. A patch needing a real
Windows/Radeon run remains runtime-unverified until that run happens, however
strong its fixture results. Do not average fixture success into a runtime pass.

## Grading and analysis

Accept only when the requested behavior, negative/fallback cases and scoped
regressions pass, there is no forbidden scope change, and required review gates
pass. Record each gate individually. Never let a candidate change the held-out
grader or weaken tests to pass. The preparer and judge need separate workspaces
and the answer/control patch must be unavailable to the solver.

Report per-stratum first-attempt success and final success, all-attempt costs,
median and tail elapsed/human time, corrective turns, touched files and verified
regressions. Show success counts per task and paired differences between
efforts. Repeated attempts share a task and are not independent Bernoulli
observations: do not use a naive Wilson interval over all attempts as evidence
for generalisation to new tasks. With a larger task set, resample paired task
clusters for success, cost and time differences, retaining each task's repeats.
With only three tasks per stratum, show every outcome and sensitivity to each
task; neither a small bootstrap nor repeated trials establishes noninferiority.

Proposed adoption rule: no critical invariant violations; no extra unverified
runtime claims; cheaper effort within a predeclared five-percentage-point
success noninferiority margin and at least 15% lower all-attempt human time or
usage, without a material regression on the other dimensions. These are proposed
maintainer utility thresholds, not values derived from the baseline. The pilot
cannot establish a 5-point margin. If uncertainty exceeds it, keep High, collect
more fresh tasks, and report the observed tradeoff without a winner.

Record infrastructure/tool failures separately and report both intention-to-run
and evaluable-task results. Retain the original cost and outcome when retrying;
use a predeclared replacement rule uniformly. Do not call a failure flaky unless
the same source/environment/check produces contradictory outcomes on repeats.
Review 7-day or next-release follow-up defects separately after initial acceptance.

## Adaptive policy evaluation

Only after fixed-effort measurement, compare static High against the proposal
in PROMPTS.md on fresh prospective tasks. Prelabel task difficulty before seeing
results. Medium starts routine tasks; High starts ownership/ABI/platform tasks;
Extra High is reserved for unresolved hard analysis. Escalate once after a
substantive failure with adequate inputs, not after a missing hardware gate.
Count all lower-effort work and context-transfer overhead in the routed cost.
Require the same final acceptance; do not relabel a recovered run first-attempt
success. If human time rises, keep the static policy even if tokens fall.

## Evidence and instrumentation

Use `result-template.json` for every attempt; null means unobserved, not zero.
Store raw transcripts/usage privately and publish only sanitized summaries.
Subscription allowance percentages are account-wide and can be confounded by
other chats or reset boundaries. They cannot be converted to API dollars or
attributed per task without isolated measurement. API pricing is a different
cost basis; record actual usage and tier before applying it. Never double-count
reasoning tokens if they are included in output tokens. A self-reported “token
estimate” is not usage telemetry.

The [OpenAI effort specification](https://developers.openai.com/api/docs/models/gpt-6.1-sol)
identifies supported settings, not a repo-specific optimum. The
[evaluation guide](https://developers.openai.com/api/docs/guides/evaluation-best-practices)
recommends task-specific metrics and human calibration. The
[SWE-bench harness](https://www.swebench.com/SWE-bench/guides/evaluation/)
provides a useful separation of patch/test outcomes from infrastructure errors;
its test resolution metric is narrower than this project's release acceptance.

## Review amendments

The [research synthesis](RESEARCH.md) separates evidence about model effort from
interfaces, context, validation, retries and coordination. Keep those dimensions
fixed in the effort study, then test them independently if traces reveal a
bottleneck. Log a failure category before escalating effort: ambiguous acceptance,
missing evidence, localisation, implementation, tool/build, verifier/test gap,
platform/runtime or review/integration. A longer reasoning budget is a candidate
intervention for only some of those failures.

Never treat pass@k (at least one selected success) as the same metric as reliable
success on every repeat. Report both best-of-budget discovery and consistency
when relevant, including selector cost. For native rendering changes, add a
follow-up maintenance task or regression review: immediate patch acceptance
alone does not measure how difficult the next change becomes.
