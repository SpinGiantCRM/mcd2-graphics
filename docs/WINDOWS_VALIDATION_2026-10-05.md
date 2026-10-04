# Windows / AMD validation — 5 October 2026

## Environment and scope

- Windows 11 Pro 10.0.26200, x64.
- AMD Radeon 860M, driver 32.0.31041.1004.
- Steam game build 25647713; executable matches the dependency lock.
- Source baseline a16df07; original payload v0.1.0-preview.1.
- Fixed candidate: `0.1.0-preview.1-windows-fix.1`.
- Pinned ReShade, RenoDX, Blueprint Loader and official DLSS runtime hashes verified.

This qualifies installation, bounded startup and native fallback on this AMD
laptop. It does not qualify NVIDIA DLSS execution with the rebuilt addon, long
sessions, HDR, split-screen or device recreation. Original Proton / RTX results
remain archived in [VALIDATION.md](VALIDATION.md).

## Findings and fixes

### Installer

The original Windows running-game guard fails because the default `tasklist`
table truncates the image name to `Dungeons-Win64-Shipping.e`. The original
guard returned successfully while the actual game was running. The fix requests
CSV and compares the complete image-name field, case-insensitively. The corrected
CLI refused an actual install attempt before changing files.

The original filesystem test harness fails on Windows with `WinError 2`:
it invokes Linux's `cp --reflink=auto`. It now uses `shutil.copyfile` and accepts
an output path, preserving archived Linux results. Install markers now record
the package manifest's version rather than a hard-coded preview version.

### Startup stack overflow

The original full mod crashed approximately 29 seconds after launch. Windows
Application Error reported `0xc00000fd` (stack overflow), fault module
`mcd2-graphics.addon64`, offset `0x2caf6`. Steam reported exit code -1073741571.
Both MCD2 and RenoDX were loaded in the process probe.

That offset is inside `___chkstk_ms`. Disassembly shows `ui_poll_intent`
reserving `0x101a8` bytes (65,960) on entry. Its 32,768-element `wchar_t` path
buffer alone occupies 64 KiB, even though initialization uses it only once.
`DllMain` has another 64 KiB path buffer. Both buffers now use heap storage.

The Windows build also removes a case-only `Windows.h` shim that recursively
includes itself on a case-insensitive filesystem. Explicit compiler paths and
`--native-only` support make the Windows native build reproducible. Both MSVC
ABI bridges are retained; the NGX bridge defines the CRT float marker needed by
LLVM MinGW. The main addon build rejects stack frames above 16 KiB.
Compiling original source with that limit fails at 65,736-byte and 65,912-byte
frames; fixed source passes.

### Unsupported GPU fallback

After fixing startup, requesting DLSS Quality in gameplay on AMD remained at
Phase 1 / Applying for over a minute. The source-scale request was acknowledged,
but no feature creation or native fallback acknowledgement arrived. The game
remained alive while rendering at reduced source scale. The original path waited
for matching render-pass observations before trying NGX.

The fix identifies the actual D3D12 rendering adapter through its LUID and
declines non-NVIDIA or unidentifiable adapters before requesting reduced source
scale. NVIDIA adapters still require existing NGX initialization and capability
checks; vendor identity alone does not establish DLSS support. A 15-second
source-wait deadline prevents indefinite pending requests when source
acknowledgements or observations do not arrive.

The final candidate preserved saved DLSS Quality intent across restart. The
main menu acknowledged effective Native. Entering gameplay then recorded
`adapter_declined` with vendor 4098 (`0x1002`, AMD), followed in the same recorded
frame by current-revision Phase 3 / Error 1 (DLSS unavailable, native fallback).
No NGX feature was created. The game-thread fallback handler requests
`r.ScreenPercentage 100`; gameplay rendered normally and mouse movement worked.
The source dimensions were not independently measured after fallback.

Video settings visibly reported **DLSS unavailable; native anti-aliasing is
active.** The saved Quality / 67% preference remained visible. Selecting DLAA /
100% also reached current-revision Phase 3 / Error 1. Returning to Quality
reported the same fallback status.

Returning to Native reached current-revision Phase 2 / Error 0, with source and
runtime revisions matching the requested revision. The final preferences were
left at Native, remembering Quality. A separate 66.8-second gameplay observation
found both addons loaded and the game responding throughout. The final session
ran from 09:15:22 to 09:28:18 and exited normally through Save and quit to desktop
with Steam exit code 0 (approximately 12 minutes 56 seconds).

## Recorded checks

| Check | Result |
| --- | --- |
| Unmodified baseline | Animated title/menu; alive at 242.4 seconds |
| ReShade + RenoDX only | Alive throughout 78.2-second observation; title, menu and hub rendered |
| Original full mod | Failed: stack overflow at approximately 29 seconds |
| Stack fix, before adapter fix | Startup passed for 138.9 seconds; gameplay worked; normal save/quit exited with code 0 after over 16 minutes |
| Final candidate startup | MCD2 and RenoDX loaded; alive throughout 139.6-second observation beginning 0.5 seconds after launch |
| Final AMD Quality and DLAA fallback | Current-revision Phase 3 / Error 1; UI fallback message confirmed; gameplay and mouse movement worked |
| Final fallback gameplay observation | Alive and responding throughout 66.8 seconds; both addons loaded |
| Return to Native | Current-revision Phase 2 / Error 0; source acknowledgement matched; Quality preference retained |
| Final normal save/quit | Exit code 0 after approximately 12 minutes 56 seconds |
| Original payload real install/check/uninstall/reinstall/check | Passed; ten existing configuration/save files preserved byte for byte, dependencies preserved |
| Final candidate real check/uninstall/reinstall/check | Passed; all six files removed and reinstalled; sixteen current configuration/save files preserved byte for byte; game/dependencies preserved; installed version matches manifest |
| Final candidate isolated real-reference filesystem gates | All seven passed |
| Extracted candidate archive | All seven real-reference filesystem gates passed; extracted installer verified the live six-file installation; archive payload hashes match manifest |
| Portable Windows installer regressions | Seven tests passed |
| Settings/runtime semantic and truncation checks | 2,312 passed on Windows |
| Adapter eligibility / timeout boundary checks | AMD, Intel, software, unknown and NVIDIA candidate vendors, plus deadline boundaries passed |

Raw local process probes, build logs, UI screenshot and mod-state snapshots are
under `dist/validation/` and excluded from Git. Account/character saves and raw
private logs are not included in the candidate archive. Remote CI results are
available in the GitHub pull request. The Windows CI fixture resolves temporary
paths like the real installer CLI, including the runner's Windows 8.3 aliases.

The fixed addon SHA-256 is
`645de6fcba098eeaaef4b571323f8fbd63a0c5a5b355288fac642c5b3028b112`.
The candidate includes public source, documentation and this mod's five payload
files. External runtime DLLs and private validation data are excluded. The local
game is left closed with the fixed candidate installed. The candidate is
published as a separate GitHub prerelease; the original preview.1 tag/assets
and Nexus download remain unchanged.

One dependency-only run exited with an access violation after Alt+F4; MCD2 was
absent in that run. A later normal save/quit of the stack-fixed full mod exited
with code 0. This does not establish the cause of the earlier shutdown failure.

## Reproduce

```text
python -m unittest discover -s tests -p "test_installer_*.py" -v
python test_install.py --reference-game "PATH TO GAME" --dlss-runtime "PATH TO nvngx_dlss.dll" --output "PATH TO RESULTS.json"
pwsh -NoProfile -File tests/windows_runtime_probe.ps1 -Seconds 120 -Output "PATH TO STARTUP RESULTS.json"
```

Start the process probe before launching through Steam. It requires the MCD2
addon to load and fails if the game exits during observation. Survival alone
does not qualify gameplay or fallback. `tests/windows_mod_state.cpp` reads only
the two MCD2 preference/runtime slots to expose requested mode, current revision
and acknowledgement phase without inspecting player saves.
See [BUILD.md](BUILD.md) for the native-only Windows build command.
