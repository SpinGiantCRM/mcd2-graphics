# 0.3.0-preview.1 candidate

Held for qualification. Published `0.2.0-rc.1` and earlier releases are unchanged.
The [release notes](RELEASE_NOTES.md) describe the additions; the
[Windows checklist](WINDOWS_QUALIFICATION_CHECKLIST.md) identifies test inputs.

## Install

1. Use the qualified candidate installer archive when supplied. Extract the
   entire folder and run the Windows/Linux installer; no separate .NET or Python
   installation is needed. Do not use the automatic GitHub source ZIP.
2. Close the game normally. Select the supported Steam installation. Xbox app /
   Microsoft Store PC remains unverified; a version override cannot bypass the
   supported executable check.
3. Under Requirements, choose official dependencies and the separate credited
   `ReShade-PR435-4eb9056-reset-epoch-x64.zip`. The framework is staged as
   `d3d12.asi`; the project's `dxgi.dll` supplies the FG bootstrap. Do not manually
   replace graphics proxies or substitute a stock ReShade version.
4. Select Install, then launch normally through Steam. Settings are in Video;
   the Mods page shows the mod's version, author and help.

Dependencies remain separate. Blueprint Loader **2.0+** is the API minimum;
complete official 2.0, 2.2 and 2.3 sets are recognized. Each dependency has an
explicit default-Off untested-version override. Newer ReShade must provide the
required D3D12 and successful-Reset interfaces; a higher version number alone
does not establish compatibility. See [overrides and model hints](DEPENDENCY_OVERRIDES.md).

## Settings

- **Upscaler / DLSS Preset / Render Scale:** Native or NVIDIA DLSS; 100% selects
  DLAA. Named presets and scale stay linked. The minimum follows output size.
- **Frame Generation:** Off / On. On inserts one generated frame per rendered
  frame and enables Reflex. This preview needs active DLSS/DLAA and native HDR;
  restart when requested. Unsupported hardware keeps it inactive.
- **NVIDIA Reflex:** Off / On / On + Boost when supported. FG and Reflex have
  separate runtime acknowledgements; saving On does not prove activation.
- **HDR:** Enable system HDR, then use HDR Output, Peak Brightness, Paper White
  and UI Brightness. Follow the restart message after calibration changes.

The mod does not force offline mode. Online compatibility is unverified; offline
play is safer. Follow the publisher's terms. Advanced RenoDX processing remains
in its ReShade panel.

## Metadata-only update during qualification

The installer from commit `38c348b` remains the pinned input for the Windows
checks already in progress. This follow-up changes only ModInfo and its three
UI containers: correct `0.3.0-preview.1` version, concise feature/help text and
online wording. Settings logic, all native DLLs/addons, shaders and INIs are
unchanged. The [UI receipt](../qualification/fg-release/ui-build-receipt.json)
records source and old/new container hashes. Neither platform's metadata
runtime check has passed yet.

For the final metadata follow-up, finish the original package's tests and
uninstall with **that original installer while the game is closed**. Then fresh
install the new metadata package, verify its hashes and repeat Windows I2,
U1–U4 and a normal exit from the checklist. This is a bounded follow-up; do not
repeat native tests merely because metadata changed. If new failures appear,
investigate and extend the relevant checks.

Both held packages use the same candidate version with different UI hashes;
Repair does not migrate between their receipts. Do not edit receipts or bypass
ownership checks. Published rc.1 migration remains a separate supported path.

## Before publication

- Complete public-installer Windows checks and the metadata follow-up; retain
  unavailable NVIDIA/HDR/controller limits rather than implying a pass.
- Repeat Linux installer/runtime checks against the final package.
- Measure matched FG Off/On DLAA and Quality results, reporting rendered/base
  FPS separately from SDK-reported presentations. Keep old benchmarks historical.
- Freeze final archives/hashes, record signing/scan results and follow the
  [Nexus review checks](NEXUS_RELEASE_GATE.md). No release is authorized by this guide.

The framework has a [direct ZIP download](https://raw.githubusercontent.com/SpinGiantCRM/mcd2-graphics/1200608712d05bf487dbc9e381969909f63908bc/qualification/fg-release/dependencies/ReShade-PR435-4eb9056-reset-epoch-x64.zip)
that needs no sign-in. The installer link now resolves to the separate archive
in the qualification branch. Its pinned hash remains unchanged; the old test
installer accepts this same ZIP. This fixes distribution, not a rendering defect.
