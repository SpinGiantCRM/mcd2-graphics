# Windows qualification: held 0.3.0-preview.1 candidate

Follow this order. Record **PASS / FAIL / NOT RUN / NOT AVAILABLE** for every
check below, with a short observation. This is a test procedure, not a claim
that qualification has passed. Do not publish, merge, replace release files or
change rendering code as part of this run. Report failures to the maintainer.

## 1. Freeze the inputs before installing

| Input | Required value |
| --- | --- |
| Repository / PR | `SpinGiantCRM/mcd2-graphics`, PR #10 |
| Installer source | `38c348b953641946483fc28ec5722eb10e5dbd60` |
| Installer workflow | [37554384729](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37554384729) |
| Artifact | `update2-candidate-installers`, ID `11454227703` |
| Downloaded artifact ZIP SHA-256 | `397363ffb383a45dc1230f927542168c7a890ca5277b9955038b25190304db48` |
| Installer | Extract the entire artifact; use the `windows` folder and `MCD2-Graphics-Installer.exe` |
| Own payload | All 12 hashes in that package's `manifest.json`, version `0.3.0-preview.1` |
| Separate ReShade dependency ZIP | `ReShade-PR435-4eb9056-reset-epoch-x64.zip` |
| ReShade ZIP SHA-256 | `38b1affa05445a0801372f1f17be76f87e0ab5777c769a09031cd4e23dee7cb2` |
| ReShade64.dll / installed d3d12.asi SHA-256 | `16c05f65b47b1a1f7b21eaa9a89cfbb7868b72894aab58610763c0e4014656b1` |
| Loader for the main run | Official Blueprint Loader **2.3**, Nexus file **289** |
| Loader 2.3 ZIP SHA-256 | `982ccc00b25a83da7fb3081739651b0e1a16d62ca990696ba64d89e8617c3374` |
| Other dependencies | Exact recognized sets from the package's `dependencies.lock.json`; overrides Off for the main run |

The separate framework archive must be supplied by the maintainer while the
release is held. If it is unavailable, report **BLOCKED: missing pinned
framework**. Do not substitute the Windows developer artifact or bypass its
hash check to obtain an apparent pass. Dependency licenses remain applicable.

Use these PowerShell commands to verify each downloaded ZIP and the expanded
Windows folder. Run them before opening the installer; use your own local paths.

```powershell
Get-FileHash -LiteralPath '<downloaded artifact.zip>' -Algorithm SHA256
Get-FileHash -LiteralPath '<separate ReShade dependency.zip>' -Algorithm SHA256
Get-FileHash -LiteralPath '<official Loader 2.3.zip>' -Algorithm SHA256
$package = (Resolve-Path '<extracted windows folder>').Path
$receipt = Get-Content -LiteralPath (Join-Path $package 'installer-build.json') -Raw | ConvertFrom-Json
foreach ($entry in $receipt.files.PSObject.Properties) {
    $path = Join-Path $package $entry.Name
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing: $($entry.Name)" }
    $actual = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $entry.Value) { throw "Hash mismatch: $($entry.Name)" }
}
```

Also compare the package's manifest and dependency lock byte for byte with the
repository at the pinned installer source. A receipt alone is not an independent
source check. Record the downloaded archive digest, installer executable digest
and actual dependency versions in the result. Do not rebuild or mix native DLLs
from `fg-windows-candidate` into this installer.

The [previous Windows AMD run](FG_WINDOWS_AMD_VALIDATION_2026-10-07.md) tested
different native/framework hashes through a developer transaction. It does not
qualify this public installer package. A later documentation-only commit may
contain this checklist; it does not change the pinned test inputs above.

## 2. Establish a recoverable baseline

- **B1:** Record Windows version, actual rendering GPU and driver, game store,
  game build, shipping executable SHA-256, output resolution and HDR state.
- **B2:** Close the game normally and verify its shipping process has exited.
  Keep local recovery copies and hashes of the existing mod/dependency files,
  install receipt and mod/video preferences. Keep saves/account data private.
- **B3:** Use the previous version's normal uninstall if necessary to obtain a
  fresh mod install. Preserve shared dependencies and saves. Do not delete
  unknown files, rewrite receipts or manually bypass ownership refusals.
- **B4:** Run from an ordinary Windows path containing a space. The full folder
  must work without installing Python or .NET. Record any SmartScreen/antivirus
  warning; do not disable protection or claim an unsigned build is certified.

## 3. Public installer and dependency checks

Use the GUI for these checks, not `game_trial.py` or a manually copied payload.

| ID | Action | Required result |
| --- | --- | --- |
| I1 | Select the supported Steam game, then Check requirements. | Exact game identity accepted; all five dependency cards visible. Wrong folder/unsupported executable refused without writes. This does not qualify Xbox PC. |
| I2 | Select official Loader 2.3 and the pinned external downloads with every override Off; install. | Complete sets recognized, installation succeeds, receipt says `0.3.0-preview.1`; all owned files match the manifest. Loader 2.3 is not downgraded to 2.2. |
| I3 | Launch the game; attempt Repair / Verify and Uninstall while it is running. | Writes refused; no files/configuration changed. Close the game normally before continuing. |
| I4 | Repair / Verify an intact install. | Verified without changing correct owned/shared files. |
| I5 | While closed, back up then remove only the owned `MCD2Graphics/live_dense.cso`; Repair / Verify. | Missing file restored to its manifest hash; no unrelated change. |
| I6 | While closed, back up then change one byte of that owned shader; attempt Repair and Uninstall. | Both refuse the modified file without mutation. Restore its exact bytes from the verified backup, then Verify. Never launch with the corrupt fixture. |
| I7 | Inspect each of Loader, ReShade, RenoDX, DLSS and Streamline override controls. Toggle one at a time; use an already installed complete dependency set via Use installed version. | Explicit acknowledgement is required; selection affects only that dependency. Close/reopen installer: all five override checkboxes default Off. |
| I8 | Use disposable dependency fixtures in the installer core checks for unknown complete sets, mixed releases, incomplete archives and unsafe paths. | Unknown complete sets need explicit override; mixed sets are not recognized as an official release; missing files and unsafe extraction fail even with override. No fake DLL is installed into the real game. Retain Windows CI results for these fixtures. |
| I9 | Uninstall the intact candidate after runtime checks. | Unchanged owned files/private DLSS copy removed; shared dependencies and saves retained; startup configuration restored only where still owned/unchanged. Compare baseline hashes. |
| I10 | Fresh install again, then Verify and launch. | Same exact hashes and successful startup. Leave the candidate installed only if the maintainer requests it. |

For I7, using a recognized installed set checks the GUI acknowledgement path;
it does not establish compatibility with an unknown future release. Unknown-set
behavior is separately checked by I8. Never modify vendor binaries to simulate
a future version in the real game. See [dependency policy](DEPENDENCY_OVERRIDES.md).

## 4. Loader, menus and gameplay

Run the installed candidate with Loader 2.3. Capture evidence privately, then
record only the useful results.

| ID | Action | Required result |
| --- | --- | --- |
| U1 | Open Settings → Mods in the main menu and pause menu. | Loader reports 2.3; MCD2 Graphics has a name, author and description. Record the displayed mod version exactly. |
| U2 | Open Settings → Video from both menus; navigate with mouse/keyboard. | Available mod rows, concise help and separate recommendation/default footer render correctly without overlap. Unsupported FG/Reflex rows may be hidden. |
| U3 | Change a render-scale preset and the slider, including 100%, 67%, a custom value and the current minimum. | Preset/slider/fill stay synchronized; 100% maps to DLAA; custom values identify Custom; source-size minimum is explained rather than silently misrepresented. |
| U4 | Restart after changing settings. | Saved preferences return; current-session runtime acknowledgements agree with the requested/current state. A saved preference alone is not an activation pass. |
| U5 | Play for five minutes; enter a dungeon and return to the hub if available. | Responsive gameplay, intact menus and no new crash/hang. Record actual transitions; unavailable progression is NOT RUN. |
| U6 | Physical controller, if available. | Navigation and changes work in both menus. No controller available means NOT AVAILABLE, not PASS. |

**Known metadata gap:** the current pinned UI can display `0.2.0-rc.1` in
ModInfo despite the installed manifest being `0.3.0-preview.1`. Record it as an
open metadata defect, not a wrong-native-build diagnosis. A subsequent metadata
package needs its new UI hashes and U1/U2 checked again before release.

## 5. Hardware-specific runtime checks

### Required on the AMD laptop

- **A1:** Start with Native, FG Off and Reflex Off. Verify the loaded addon chain
  and compare actual game-directory hashes to the package/lock, including
  `d3d12.asi`. Do not count a system DXGI module as the mod bootstrap.
- **A2:** Request Quality 67%, DLAA 100% and Custom 77% in separate gameplay
  checks. Unsupported DLSS must be declined **before source-resolution
  reduction**, with Native fallback acknowledged in the current session. Each
  request must reach a resolved state; do not count an indefinitely Applying
  state as a pass.
- **A3:** With the game closed, use the existing mod-slot fixture tooling to
  seed FG On and Reflex On/Boost preferences for a second launch. Record the
  fixture method. Do not touch character/account saves. Unsupported hardware
  must keep FG/Reflex unavailable/inactive without faults, FG swapchain routing
  or a foliage velocity lease. Hidden rows are not evidence of activation.
- **A4:** Confirm native Windows never applies the Wine-only Reflex pacing
  setting. Preserve the existing native-Windows guard checks and examine only
  the relevant sanitized status event, not a public raw log dump.
- **A5:** Leave the optional model hint absent/default. An explicit hint may be
  checked on AMD only for safe unsupported fallback, never for model quality or
  actual NGX model selection.

The opt-in module probe requires PowerShell 7 and the repository script:

```powershell
pwsh -NoProfile -File tests/windows_runtime_probe.ps1 -Seconds 60 -RequireDisplayLatency -RequireFG -Output '<private evidence folder>/startup.json'
```

It checks survival/loading; independently verify its six hashes and menu,
gameplay, source-resolution and current-session acknowledgements. Retain those
bounded observations, not merely a screenshot of the saved settings.

### Only on supported Windows NVIDIA hardware

If unavailable, mark this entire subsection NOT AVAILABLE and qualify only
Windows/AMD. Do not block the AMD report by inventing NVIDIA results.

- **N1:** Native ↔ Quality ↔ DLAA, including repeated changes and hub/dungeon
  transitions: safe history reset, completed acknowledgements, no internal NGX
  evaluation errors or stuck resource retirement.
- **N2:** FG Off/On at DLAA 100% and Quality 67%, restarting where requested:
  actual generated presents, correct UI composition, stable foliage and no
  faults. Measure rendered/base FPS separately from displayed total FPS.
- **N3:** Reflex Off/On/On + Boost with FG and DLSS Off, same scene, frame limit
  and VSync state: compare frame-time distribution and base FPS. Then repeat
  with FG On. Do not call a driver invocation an input-latency measurement.
- **N4:** For performance, warm up each mode, record at least three 30-second
  samples in a matched stationary scene and an Off → On → Off repeat. Report
  medians/low-percentile FPS and frame-time tails. FG On must increase total FPS
  with a reasonable disclosed base-FPS cost; Off must remain comparable to the
  qualified no-FG baseline. Ask the maintainer to assess a material regression;
  do not hide it through a different scene, cap or render scale.
- **N5:** Verify native HDR output and calibration only with an HDR-capable
  display. SDR screenshots and an enabled checkbox are insufficient proof.

## 6. Shutdown, restore and report

- **E1:** Perform at least two normal exits, including Save and quit and normal
  window close. Hold the actual process handle before requesting exit; record
  shipping exit code and elapsed time, separately from the launcher. A closed
  window or disappearing task-list entry does not prove exit code 0.
- **E2:** Observe delayed exit for up to 120 seconds without force killing. If
  still running, stop further install/repair/uninstall and report a blocker.
  A delayed clean exit is an observation needing maintainer review, not an
  unexplained-crash fix. Preserve the older delayed-exit/crash history.
- **E3:** Complete I9/I10 and restore the agreed baseline or final candidate.
  Verify files and preferences; remove test fixtures. Never roll back progress
  or overwrite player/account saves from a test backup.
- **E4:** Write a short report and sanitized aggregate in the repository using
  the structure below. Record every check ID, measured timings, exact hashes,
  hardware limits and remaining defects. Keep private paths, usernames,
  account/party identifiers, raw logs and game executables out of the repo.

```text
Test date:
Installer source / workflow / artifact / ZIP SHA-256:
Installer executable SHA-256:
Manifest version / owned-file verification:
Dependency versions and actual framework SHA-256:
Windows / rendering GPU / driver / game store / build / executable SHA-256:
Resolution / HDR / VSync / frame limit:
Checks: B1–B4, I1–I10, U1–U6, A1–A5, N1–N5, E1–E4
  [one PASS / FAIL / NOT RUN / NOT AVAILABLE and observation for each]
Normal exits: route, elapsed seconds, shipping code, launcher code:
Baseline/restoration verification:
Open defects / unavailable hardware / maintainer decision required:
Decision: PASS for stated scope / FAIL / BLOCKED
```

Do not mark overall PASS with an unexplained failed required check. Distinguish
a known metadata defect, an unavailable hardware test and a runtime failure.
After any change, list exactly which inputs changed and which affected checks
were repeated; previous results apply only to unchanged, verified inputs.
Send the result to the maintainer and stop. Release approval remains separate.
