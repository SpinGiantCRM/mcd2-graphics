# Separate candidate installer inputs

## Complete development candidate

`tools/assemble_amd_candidate.py` now assembles 16 own files from the independently
verified [PR45 Windows artifact](AMD_WINDOWS_CANDIDATE_2026-10-10.md). It checks
the archive, every recorded member, source commit, SR/FSR bridge binding and UI
receipt before writing. Diagnostic observers and isolated probe binaries are
excluded. Neither vendor runtime DLLs nor ReShade enter the own payload.

The candidate has installer version `0.4.0-amd.dev.1`, its own manifest and
dependency lock. The existing UI artifact still reports `0.3.0-preview.1`; this
is recorded explicitly and requires alignment before a public release. The
installer describes the AMD development scope and pending Radeon qualification.
Default published metadata and files are unchanged.

The candidate game pin is Steam build `25754144`, executable SHA-256
`3a8703406fd50520f83c4f70a0212c000cb3b584ef28eb38032902230c01ebdd`.
The installed executable's PE metadata matches the explicit updated map. Native
runtime signature/registry gates remain mandatory. This does not support unknown
builds or the Xbox PC executable. The released installer still uses its older pin.

FSR SR/FG vendor files are separately selected official analytical SDK 2.3
dependencies. Their native API and runtime hashes are not weakened by an installer
override. ReShade uses the exact artifact framework hash; the known released
framework can be upgraded through the existing backup/ownership checks. AMD SDK
header and Anti-Lag licenses accompany both installer folders.

### Reproduce the inputs

```sh
python tools/assemble_amd_candidate.py \
  --artifact fg-windows-candidate-pr45.zip \
  --archive-sha256 b0e4d1ba4acb8d52e6063d67d41419e0446b9d425dd582846a8f76d6b10c7f8b \
  --source-commit 826f18177418044c68910f9e85e21f0dfbbb00f2 \
  --artifact-url https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/38009520993/artifacts/11652882639 \
  --version 0.4.0-amd.dev.1 --output candidate-inputs
```

Then build each empty RID output using the command below with
`candidate-inputs/payload`, `candidate-inputs/manifest.json` and
`candidate-inputs/dependencies.lock.json`. The **AMD development installer
candidate** workflow performs the same pinned assembly on Windows and uploads
`amd-development-installers`. It is a development artifact, not a release; expired
or changed upstream artifacts fail closed and need a newly reviewed receipt.

### Package evidence

[The candidate receipt](../qualification/providers/amd-installer-candidate-2026-10-10.json)
records 30 Python installer checks, 133 managed safety checks and actual Linux/
Windows-target folder builds. Every folder hash and both embedded resource hashes
were verified. Each actual payload passed 59 transaction checks in an isolated
temporary game tree: fresh install, missing-file repair, modified-file refusal,
uninstall, reinstall, synthetic save preservation and external dependency retention.

These transactions ran on Linux. They did not execute the Windows GUI or any GPU
feature, and did not write to or launch the installed game. Actual Radeon FSR/FG/
Anti-Lag, Windows installer UI and wider lifecycle/performance qualification remain
required. No new latency or physical-display performance claim is established.

## Installer input snapshots

`build_installer.py` accepts `--manifest` and `--dependency-lock` for a candidate
with its own payload and dependency hashes. Omitting them keeps the repository's
released inputs. Do not rewrite those released pins to package an AMD trial.

```sh
python build_installer.py --dotnet /path/to/dotnet \
  --payload /path/to/verified-own-payload --rid win-x64 \
  --manifest /path/to/candidate/manifest.json \
  --dependency-lock /path/to/candidate/dependencies.lock.json \
  --output /path/to/empty-output
```

The builder snapshots both JSON inputs before compilation. The managed installer
embeds those exact snapshots, and the same bytes accompany the visible `payload/`
folder. Their hashes are recorded in `installer-build.json`. A changed snapshot
or payload prevents a successful receipt. Full folder deployment, integrity
checks and existing install/repair/removal behavior remain required.

AMD/NVIDIA vendor runtimes and the separately credited ReShade framework cannot
enter the own-payload manifest. Keep them as external dependencies. Selecting a
candidate lock does not relax native hardware, API, ABI or runtime hash checks.

## Evidence and remaining work

[The numeric receipt](../qualification/providers/installer-resource-inputs-2026-10-10.json)
records nine package fixtures and real self-contained Linux/Windows-target
builds. Reflection read each assembly's two embedded resources; their hashes
matched the accompanying files. All folder file hashes were verified. These
trials used the existing released own payload and were never installed.

This prepares separate candidate packaging; it does not qualify an AMD installer
or Windows execution. The [current Windows artifact](AMD_WINDOWS_CANDIDATE_2026-10-10.md)
still needs active Radeon FSR/FG/Anti-Lag and lifecycle checks. A complete AMD
package then needs its own fresh install, repair, removal and fallback checks.
Published installers, tags and dependency pins remain unchanged.
