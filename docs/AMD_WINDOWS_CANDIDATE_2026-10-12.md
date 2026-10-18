# Windows/Radeon development qualification

## Provider test candidate: 0.4.0-amd.dev.16

Dev.16 is ready for Windows qualification. It includes the confirmed bootstrap
trace write/close race correction and the gameplay diagnostics overlay at the
top left. No completed dev.15 check qualifies these changed candidate bytes.

Use the [current candidate receipt](../qualification/providers/windows-candidate-current.json).
All 46 recorded native-artifact files were hash-verified, including the separate
framework. The CI merge and implementation head have identical source trees;
62 UI/bootstrap consumed source files also match the current implementation,
allowing only checkout line-ending differences. Subsequent changes record dev.15
qualification results and do not change runtime code.

- Implementation: `9fb44f41160bc51cee202a0106fedf6eeccd5fe4`.
- CI source: `a140afb6e04913b039b5113a97c12f11661166d7`.
- Native inputs and required framework: [run 39109623985 / artifact 12130026833](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/39109623985/artifacts/12130026833).
- Native archive SHA-256: `6228b83788991b5a3b6f38e3f67ca730d81b62ee620798660117f6440f9fc56f`.

Use the **dev.16** visible installer folder. Verify its manifest, dependency lock
and all 17 owned payload files against its assembly receipt. Older dev.15 or
released installers do not qualify dev.16. Framework and vendor runtimes remain
separate dependencies; extract the native archive above and select
`experimental-reshade/ReShade64.dll` when the installer asks for ReShade.
Do not mix a different candidate's framework with this lock. CI success is build
and packaging evidence; actual Radeon activation, image quality and clean exits
still require the checklist below.

The game's mod metadata still identifies the preview UI as 0.3.0-preview.1;
the installer and assembly receipt identify dev.16. Record the manifest and
payload hashes rather than relying on that in-game label. Align the public
release version and freeze its final hashes before release qualification closes.

Linux release clearance remains pending. Earlier uninstrumented dev.15 quit
with `0xC0000409`; two later unchanged Steam controls exited 0, including a longer
FG-active run. The source correction fixes a reproduced logging lifetime defect,
but is not established as the original game's fatal-exit cause. The complete
dev.16 artifact has not yet run on Linux. See the
[bounded runtime record](../qualification/providers/amd-dev15-packaging-2026-10-18.json).
Windows qualification can proceed independently; these Linux results do not
establish a Windows/Radeon result.

## Preparation

1. Close the game. Use Steam build 25754144, shipping SHA-256
   `3a8703406fd50520f83c4f70a0212c000cb3b584ef28eb38032902230c01ebdd`.
2. Back up replaced mod files and only mod settings/runtime slots and authority.
   Preserve other mods and player/account saves. Extract the complete visible
   installer folder; do not run the EXE out of the ZIP.
3. Select external dependencies matching the accompanying candidate lock.
   For ReShade, use `experimental-reshade/ReShade64.dll` from the exact native
   artifact; the installer places it as `d3d12.asi`. Stock ReShade is not a
   substitute for the required interfaces. Do not place it over the mod's
   `dxgi.dll` bootstrap.
4. Use a complete recognized Blueprint Loader set. The 2.0+ API minimum remains;
   no popup API is required. Record the actual Loader release and all files.
5. Record installed payload/dependency hashes, GPU, driver, OS and game build.
   Executable identity and runtime/hardware admission checks remain mandatory.

## Pre-release checks

Record Pass, Fail or Unavailable for each check. Include the exact candidate
and effective settings. Stop on a new unexplained crash, hang or image corruption.

- **Native control:** Native, 100% scale, FG Off, vendor low latency Off. Verify
  ordinary temporal output and stable limiting.
- **FSR SR:** Native AA, Quality, Balanced, Performance, Ultra Performance and
  Custom. Confirm successful evaluations and actual render/output dimensions.
  Exercise analytical 3.1 and official 4.1 on a compatible Radeon/driver. A saved
  version or loaded DLL does not establish the selected implementation.
- **AMD FG:** Native and FSR inputs, On/Off, pause/resume and travel. Require
  advancing successful generation counters, fresh guides and no SDK errors.
  Check analytical 3.1; test ML 4.0 only where the device actually supports it.
- **FG Off recovery:** compare warmed fresh Off with returned Off in the same
  scene and settings, repeat the cycle, and confirm generation stops. Retained
  memory is not proof of continuing work. Investigate persistent throughput or
  frame-time loss; this short regression check remains before release.
- **Anti-Lag 2:** prove actual Radeon driver/API eligibility and active timing
  counters. Test independently with FG Off, then with AMD FG. A visible toggle
  is not activation proof. Unsupported hardware must not report it active.
- **NVIDIA routes:** on suitable NVIDIA hardware, check DLSS/DLAA, independent
  Reflex and both SR/FG pairings. An AMD laptop qualifies refusal/fallback only.
  Native Windows preset C/profile restoration requires a Windows NVIDIA system;
  never claim that an AMD or Proton run qualified that driver-profile path.
- **Advanced Mods page:** all three tabs and exclusive glow, mouse/keyboard
  navigation, saved overrides, Auto/reset, relaunch, and independent FSR SR/FG
  choices. Record physical-controller checks separately if unavailable.
- **Diagnostics:** details stay within Diagnostics; gameplay overlay follows
  fresh successful activity and clears on pause, disabled features or stale data.
  Separate requested presets, implementation versions and actual activity.
- **Sharpness:** 0–100% below Render Scale, DLSS and FSR, reset/relaunch,
  Native fallback and preservation of a later external value change.
- **Lifecycle/output:** hub/level travel, pause/resume, source/output resolution,
  window changes and SDR/HDR. Require actual swapchain colour space/format;
  preferences alone do not qualify HDR or SDR. Preserve display confirmation.
- **Shutdown:** hold the shipping process handle through three ordinary quits.
  Require exit code 0, completed presenter/shared-runtime retirement, and no
  unexplained delayed exit. A closed window is insufficient.
- **Installer:** fresh install, repair, upgrade, verify, modified-file refusal,
  uninstall/reinstall and preserved external dependencies/player/other-mod files.

## Release and measurements

Complete the functional/platform and installer checks before releasing. The full
performance and image benchmark suite runs **after the mod release**, with
results published separately. Do not delay release for the extensive matrix or
publish old measurements as results for this candidate.

Do not infer display FPS by multiplying an overlay number. SDK generation counts,
rendered FPS and physical presentation/latency are different measurements.
Keep raw logs and captures private; share sanitized results and reproducible
failures. Preserve earlier failed evidence and its affected scope.
