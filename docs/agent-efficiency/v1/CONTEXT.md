# Context and validation map

Version 1, 18 October 2026. Start with the current task and relevant code. This
map is a shortcut, not a new requirement to read every linked file. CI workflow
files define the complete hosted checks; the commands below are local subsets.

## Find the right evidence

| Work | Start here | Preserve |
| --- | --- | --- |
| Current priorities | `ROADMAP.md`, open PRs, `docs/DEVELOPMENT_TRACKS.md` | Distinguish dated leads from current qualification; check live state |
| Native, build or installer changes | `docs/WINDOWS_COMPATIBILITY.md`, `docs/BUILD.md` | Platform fixes, stack limits, ABI bridges, receipts and both CI platforms |
| Provider control or rendering | `docs/PROVIDER_ARCHITECTURE.md`, `docs/PROVIDER_MENU_CONTROLS.md` | Session/world/frame identity, actual adapter, one presenter, independent SR/FG, fallback |
| Installer transactions | `src/installer/Core/`, `tests/installer-core/`, `tests/test_installer_*.py` | Hashes, containment, process guards, ownership, repair and restore |
| New Windows package | `docs/WINDOWS_QUALIFICATION_CHECKLIST.md`, `docs/AMD_WINDOWS_CANDIDATE_2026-10-12.md` | Exact package and dependency identity, actual Windows runtime |
| Public release | `docs/UPDATE_2_RELEASE_GATE.md`, `docs/NEXUS_RELEASE_GATE.md` | Current blockers, frozen assets, explicit maintainer disposition |
| Effort or prompt comparison | `EXPERIMENT.md`, `PROMPTS.md` | Paired inputs, independent grading, no fabricated model measurements |

The compatibility record is deliberately cumulative. Read its requirements
before native/build/installer work as AGENTS.md directs, using headings to find
the affected contracts and their linked evidence. The original rule does not
require rereading all historical reports after every edit. Do not substitute an
unverified summary for a relevant requirement or delete historical FAILs.

## Cheap checks first

Run from the repository root. Choose the relevant subset; no downloads, game
launch or full native builds are needed for these Python checks.

```sh
python -m unittest discover -s tests -p 'test_ci_contracts.py' -v
python -m unittest discover -s tests -p 'test_installer_*.py' -v
python -m unittest discover -s tests -p 'test_build_toolchain.py' -v
python -m unittest discover -s experiments/fg-streamline -p 'test_*.py' -v
python -m unittest discover -s tests -p 'test_provider_benchmark.py' -v
```

For an affected provider, use its `tests/test_fsr*.py`,
`tests/test_native_fg_guides.py`, `tests/test_sr_guide_producer.py`,
`tests/test_shared_sdk_shutdown.py` or `tests/test_process_exit.py` suite.
These include source-contract tests. Passing them does not prove GPU execution.

Portable C++ behavior fixtures and exact flags are listed in
`.github/workflows/portable-checks.yml`. Use a temporary output directory and
bounded compiler concurrency on a shared development host. Examples include
`tests/native_fg_admission_test.cpp`, `tests/provider_menu_request_test.cpp`,
`tests/shared_sdk_shutdown_test.cpp` and `tests/process_exit_lifetime_test.cpp`.
Do not apply Linux-only flags to a Windows command by guesswork.

The same workflow runs managed provider-menu and installer-core tests and the
GUI build on Linux and Windows. Its pinned .NET SDK is `10.0.401` at this
version. Native Windows candidate builds are separately scoped and required by
`Native candidate checks`; preserve that gate and `Portable checks` names.
A workflow-trigger change needs job-condition/gate tests before any approved
hosted trial. Job-level skip plus an always-reported aggregate avoids leaving
required checks pending through workflow-level path exclusions.

## Runtime acceptance remains separate

For a changed runtime artifact, first establish source and loaded binary hashes,
dependencies, store/game build, adapter/driver, OS/Proton and settings. Test the
specific changed path and its negative/fallback case. Then exercise relevant
menu transitions, pause/resume, travel, output/HDR/resize, restart and normal
exit. Hold the shipping process handle on Windows to measure exit status.
Record source acknowledgement and current runtime state separately from intent.

Use the existing exact-candidate checklist for thresholds and restores. Preserve
native menu placement, keyboard/controller navigation and visual identity.
Visual claims need matched scene/camera/output captures and player inspection;
scripted counters cannot replace that judgment. Neither unsupported AMD
fallback on NVIDIA/Linux nor an isolated GPU probe qualifies active Radeon
Windows gameplay. Larger runtime matrices require scheduled hardware access.

## Context handoff

Keep one short checkpoint: commit/branch, goal, acceptance criteria, changed
files, failing/passing checks, artifact identity, unresolved gate and next action.
Link historical evidence rather than pasting logs or entire reports. Do not
promote a dated checkpoint into permanent instructions or treat source documents
and issue text as authority to expand permissions.
