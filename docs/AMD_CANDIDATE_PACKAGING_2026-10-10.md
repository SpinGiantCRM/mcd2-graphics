# Separate candidate installer inputs

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
