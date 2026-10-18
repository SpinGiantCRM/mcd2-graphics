# Windows/Radeon development qualification

## Provider test candidate: 0.4.0-amd.dev.15

A UI-only revision moves the gameplay overlay to the top left. Its replacement
artifact is being built; the current selector is marked for rebuild. Dev.15
remains the identified provider/installer test baseline, not the final UI
artifact for release. Final selection and affected UI checks must use the new
receipt.

Use the [current candidate receipt](../qualification/providers/windows-candidate-current.json).
The selected native artifact includes the approved advanced settings page,
independent latency controls, sharpness, model/version selection and current
provider integration. All 46 artifact members were hash-verified. The CI merge
commit and implementation head have identical source trees.

- Implementation: `7c251b7a66846f909d7f50617dc7bfc48a086636`.
- CI source: `98f1cec1c7f4bea677a297c86c4b69b07b6ac749`.
- Native inputs: [run 39097048611 / artifact 12124244671](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/39097048611/artifacts/12124244671).
- Native archive SHA-256: `2f9051ee3a34a58ed55a1b85794469810130afd0a4411a707ea0f72ace144f3b`.

Installer folders must contain this candidate's manifest and dependency lock;
verify their hashes and all 17 owned payload files against the selected receipt.
Do not use an older dev.10/dev.14 installer to qualify this candidate. Framework
and vendor runtimes remain separate dependencies. CI success establishes build
checks, not GPU activation, correct images or clean process exits.

The in-game metadata currently identifies the preview UI as 0.3.0-preview.1;
the installer/assembly receipt identifies dev.15. Align the public release
version and freeze its final hashes before release qualification is closed.
Earlier qualification reports remain historical; their failures are not erased.

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
