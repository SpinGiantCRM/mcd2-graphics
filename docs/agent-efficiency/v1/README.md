# Agent efficiency work

Version 1, 18 October 2026. The goal is accepted MCD2 Graphics work per unit of
maintainer attention, model usage and elapsed time. High is the study baseline;
no local effort comparison has been run and no winning effort is established.

| Need | Document |
| --- | --- |
| Find relevant code, evidence and cheap checks | [Context map](CONTEXT.md) |
| Define a task, reproduce a bug and hand off verification | [Task templates](TASKS.md) |
| Review the user-owned project instruction candidate | [Prompt version 1](PROMPTS.md) |
| Compare prompts and reasoning efforts without conflating them | [Experiment protocol](EXPERIMENT.md) |
| Understand research, tradeoffs and transfer limits | [Research synthesis](RESEARCH.md) |
| Prepare independent task graders | [Pinned candidate tasks](tasks.json) |
| Record one attempt with unknowns kept explicit | [Result template](result-template.json) |

The companion CI changes retain both platforms and existing required check
names. Missing/invalid native scope now fails closed, and Windows portable
commands stop on their first failure. Local regression checks exercise the
embedded gate and the recipe's actual command block. Hosted Windows execution
is a separate gate; a local pass does not qualify Windows/Radeon gameplay.

Keep prompt and effort interventions separate. First compare the old root
AGENTS.md from `6aeff77` and v1 at fixed High; then freeze the instruction bundle
for the effort comparison. The six task definitions are candidates awaiting
independent base-fail/control-pass validation, not completed benchmark cases.

No runtime source, menu, payload, dependency pin, release asset or system setting
is changed by this instruction package. Keep existing ROADMAP.md and exact
candidate qualification requirements authoritative for feature readiness.
