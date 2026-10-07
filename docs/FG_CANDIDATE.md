# 0.3.0-preview.1 — Frame Generation preview

[Download the release](https://github.com/SpinGiantCRM/mcd2-graphics/releases/tag/v0.3.0-preview.1) · [Installation](../INSTALL.md) · [Release notes](RELEASE_NOTES.md) · [Fresh benchmarks](BENCHMARK_2026-10-07.md)

Extract the complete Windows/Linux installer folder, close the game and select the supported Steam installation. Under Requirements choose the official dependencies and the separate credited **ReShade-PR435-4eb9056-reset-epoch-msvc-x64.zip** from the release. Install, then launch normally through Steam. Settings remain in Video; the Mods page shows version, author and help.

Blueprint Loader **2.0+** is the API minimum; complete 2.0, 2.2 and 2.3 sets are recognized. The popup API is not used. Each dependency has an explicit, default-Off untested-version override. [Version policy and model hints](DEPENDENCY_OVERRIDES.md).

## Why this ReShade build is required

This separately distributed source-built framework supplies two runtime interfaces:

- **Native Direct3D device/queue access (PR435):** passes the game's actual device and queue to the FG runtime.
- **Successful command-list reset confirmation:** identifies when an old recording is actually replaced. DLSS resource retirement still waits for GPU completion; reset success alone is insufficient.

These interfaces support rendering and resource lifetime, including DLSS/DLAA preset changes with FG Off. They are not just benchmark instrumentation. A newer stock ReShade version may lack them; an override cannot add missing interfaces.

[Download the exact ZIP](https://github.com/SpinGiantCRM/mcd2-graphics/releases/download/v0.3.0-preview.1/ReShade-PR435-4eb9056-reset-epoch-msvc-x64.zip), then select it under ReShade in the installer. No account or compilation is needed. Its archive SHA-256 is `610314d160a28f743cd2cfdc593c83b818bb2ead8576671d870c8f8652e95168`; the installed `d3d12.asi` SHA-256 is `ce808bb1494415586cfb2ec5638a3ab0f0cea3879e45146a34976740b59fcf48`.

ReShade is by crosire and contributors; PR435 is by JoeyDelp. The archive includes license, source/build references, successful-reset patch provenance and a receipt. It is separate from both mod installer archives and is **not an official ReShade release**. NVIDIA runtimes remain external official dependencies.

The MSVC build replaces an earlier cross-built framework that reproduced Windows shutdown access violations even when isolated. [The mitigation record](WINDOWS_MSVC_MITIGATION_2026-10-07.md) and historical FAIL remain intact. The final [Windows AMD GUI/fallback qualification](WINDOWS_FULL_QUALIFICATION_2026-10-07.md) passed bounded checks, including three clean but sometimes delayed exits. [Final Linux qualification](LINUX_FINAL_QUALIFICATION_2026-10-07.md) uses the same framework. No universal/causal shutdown repair is claimed.

## Controls

- **Upscaler / Preset / Scale:** Native or NVIDIA DLSS; 100% selects DLAA. Named presets and scale stay synchronized; the minimum follows output size.
- **Frame Generation:** Off / On. On inserts one generated frame per rendered frame and enables Reflex. This preview requires supported NVIDIA hardware, active DLSS/DLAA and native HDR. Restart when requested.
- **NVIDIA Reflex:** Off / On / On + Boost where supported. Saved selection and actual activation have separate acknowledgements.
- **HDR:** enable system HDR first, then use Output, Peak Brightness, Paper White and UI Brightness. Calibration follows the stated restart requirement.

The mod does not force offline mode. Online/co-op compatibility is unverified; offline play is safer. Xbox app / Microsoft Store PC remains unverified. Follow publisher terms. Advanced RenoDX processing remains in ReShade.

## Qualification and package identity

Final installers come from [CI run 37602995068](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37602995068), source `e421e57e7667f0e9ec71e1940c6daa352a39f0c7`, with final Mods version 0.3.0-preview.1. Publication changes docs and ZIP containers only. [Frozen hashes](../qualification/fg-release/release-artifacts-2026-10-07.json).

Linux/NVIDIA active FG/SR/HDR and Windows/AMD installer/fallback checks have separate scopes. Native Windows NVIDIA active features/HDR, physical controllers, device loss and long sessions remain unqualified. Packages are unsigned; a Nexus review may delay availability. GitHub provides the exact files while review is pending.

Private earlier candidates can share the version number but have different UI/dependency hashes. Use their original installer to uninstall before fresh installation; do not edit receipts. Published rc.1 migration is supported by Repair / Verify. Historical tags and dependency archives remain unchanged.
