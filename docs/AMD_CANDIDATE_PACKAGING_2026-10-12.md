# Current AMD development installer candidate

`0.4.0-amd.dev.6` uses the verified PR52 native artifact and source-matched UI.
It remains a development candidate. Windows/Radeon qualification and broader
runtime coverage are required; published releases and dependency pins are unchanged.

[Exact build and test pins](../qualification/providers/amd-installer-candidate-2026-10-12.json)
record 16 owned payload files, 15 separately selected dependency files and both
standalone installer folders. All folder hashes and embedded manifest/lock
resources matched. Each package passed 59 real core transaction checks on Linux:
fresh install, repair, modified-file refusal, uninstall, reinstall, synthetic save
preservation and external dependency retention. This does not qualify Windows GUI
or GPU execution. The current [Windows handoff](AMD_WINDOWS_CANDIDATE_2026-10-12.md)
is the required runtime checklist.

## Reproduce

```sh
python tools/assemble_amd_candidate.py \
  --artifact fg-windows-candidate.zip \
  --archive-sha256 71fc9e563a31de90878e35b66c23f841e39ea9ce9284a066a8aa1a6266639622 \
  --source-commit 79538debe04b9b49d3360509a1fded2f8883e936 \
  --artifact-url https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/38185912901/artifacts/11719049992 \
  --version 0.4.0-amd.dev.6 --output candidate-inputs
```

Use `build_installer.py` with those payload/manifest/dependency-lock inputs and
separate empty `linux-x64` and `win-x64` output folders. The AMD development
installer workflow downloads this exact native archive; it does not substitute a
newer framework/native build. Expired artifacts fail closed and require a fresh
reviewed pin. Folder builds are visible and self-contained, with no self-extraction.

ReShade and AMD/NVIDIA runtimes remain external. Select the exact artifact's
framework as `d3d12.asi`, with its source/license attribution. Loader 2.3 is
recognised under the existing 2.0+ API policy; no popup API is introduced.
Installer overrides do not bypass native SDK/API, device or shader contracts.
The candidate pins Steam build 25754144 and the verified executable. It does not
admit an unknown Microsoft Store/Xbox build.

The rebuilt UI still reports metadata version `0.3.0-preview.1`, explicitly
recorded apart from the development installer version. Align it before any
public release. Diagnostic addons, probe/test executables, raw logs, player
saves, authentication files and vendor/runtime binaries are absent from the
owned payload. Historical candidate receipts remain intact.
