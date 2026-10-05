# Source build

The native source is C++20. Its ReShade calls use a separate MSVC-ABI adapter; the main Windows addon uses MinGW. Both adapters are intentionally retained from the tested path. Do not replace them with PR435 or an upscaler-provider patch without the separate regression matrix.

Pinned dependencies:

- NeoRune SDK and tool **0.1.2**, obtained from NuGet. The SDK provides the game bindings; these generated bindings are not copied into this repository.
- .NET SDK **10.0.401** (reference runtime **10.0.12**) for the tested UI compiler.
- ReShade SDK **v6.8.0**, addon API **20**; obtain headers from `crosire/reshade`.
- NVIDIA DLSS SDK commit **374959484e79a640feaba44c93ac8cfb0a03f5b5**, obtained from NVIDIA under its own license.
- LLVM/Clang, MinGW-w64 C++20 and Microsoft's DirectX Shader Compiler for the native/conversion shader build.

The SDK headers and vendor runtime DLLs are not relicensed or bundled. The parameterized `build.py` expects separately obtained SDK directories and tool paths. It does not download SDKs or accept licenses. Supply `--dotnet`, `--neorune-sdk`, `--pack-tools`, `--reshade-include`, `--ngx-include` and `--dxc` for a full build. The complete toolchain recipe targets a Linux build host with MinGW and Clang and was executed successfully there. NeoRune package tools run through a separately supplied Wine wrapper on that host; the complete UI/shader build remains unqualified on Windows. The native-only Windows recipe is qualified below. The local build uses NeoRune's command-line package compiler and verifies diagnostics even when its exit code is zero.

Runtime output goes to the game's local `Saved/MCD2Graphics` directory. The installed own conversion shader is `MCD2Graphics/live_dense.cso`; the externally supplied official DLSS runtime is `MCD2Graphics/ngx-runtime/nvngx_dlss.dll` beside the addon. Normal builds disable developer file controls.

The shipped converter is the originally qualified bytecode. Rebuilding with a different compiler can produce different bytecode; build output must be requalified and is not automatically packaged.

## Windows native addon build

Preserve the [Windows compatibility requirements](WINDOWS_COMPATIBILITY.md)
when changing either platform's build or native code.

The native-only build was executed on Windows with LLVM MinGW
20260922 (UCRT x64). Supply the pinned headers and explicit compiler paths:

```text
python build.py --native-only --reshade-include "PATH TO RESHADE HEADERS" --ngx-include "PATH TO NGX HEADERS" --clang-cxx "PATH TO LLVM MINGW/bin/clang++.exe" --mingw-cxx "PATH TO LLVM MINGW/bin/x86_64-w64-mingw32-clang++.exe" --output "PATH TO BUILD OUTPUT"
```

This keeps both MSVC ABI bridges and disables developer controls. The build
rejects stack frames above 16 KiB: the original 64 KiB path buffers fail this
check. Headers use the platform's `windows.h` directly; a case-only include
shim recursively includes itself on Windows. The NGX bridge defines the
MSVC floating-point CRT marker required by LLVM MinGW.

`--native-only` does not rebuild the UI or conversion shader. The Windows
candidate reuses their hash-verified preview.1 payloads. The complete UI/shader
source build remains unqualified on Windows. See the
[Windows validation report](WINDOWS_VALIDATION_2026-10-05.md) for runtime scope.

## Linux NVIDIA regression candidate (not published)

The Windows-produced PR5 addon failed Quality-to-DLAA teardown on the Linux
NVIDIA host. The same native source built with MinGW GCC 16.2.0 and MSVC-target
Clang 23.1.1 bridges passed that transition and subsequent Native/Quality
switches. This isolates a build/artifact difference, not a proven compiler or
driver defect. LLVM MinGW NVIDIA runtime compatibility remains unqualified.

`build.py` now selects the 16 KiB frame-error spelling for GCC or Clang and
refuses an unidentified compiler. GCC needs `-Werror=frame-larger-than=16384`;
Clang retains `-Wframe-larger-than=16384 -Werror=frame-larger-than`. Both guards
were checked with small and deliberately oversized functions. The copied
ReShade headers normalize `<Windows.h>` to `<windows.h>` without a case-only
alias or modifying the separately acquired SDK.

The local candidate retains PR5's native source, Windows heap path buffers,
ABI bridges, adapter policy, deadline, installer fixes and frozen UI/shader
payloads. The changed binary needs fresh Windows qualification. Test the exact
packaged addon first; rebuilding with LLVM MinGW produces a different artifact
and requires its own NVIDIA regression checks. See
[regression handoff](LINUX_REGRESSION_HANDOFF_2026-10-05.md).

## Update 2 display addon and guided installer

Build the UI using `build.py` and the separately acquired NeoRune SDK/tool and package tools. The build includes `tools/ModInfoBuilder`, which packages the Blueprint Loader 2.0 ModInfo using the loader's public template class. No extracted game assets or vendor runtime files are included.

Build the independent display/latency addon with `build_latency.py --streamline-sdk PATH --windows-sysroot PATH --reshade-include PATH --output PATH`. Streamline SDK **2.14.1** and the Microsoft C++/Windows SDK sysroot are acquired separately under their own terms. It builds the MSVC ABI bridge and MinGW addon with the preserved frame-size guard; it does not rebuild or alter the released SR renderer. Compiler paths can be supplied explicitly. This Linux-hosted recipe is not a Windows source-build qualification.

Stage the seven own payload files with exactly the hashes in manifest.json. Build each standalone installer using .NET SDK **10.0.401**, Avalonia **11.3.22** and its committed package lock:

```text
python build_installer.py --dotnet PATH --payload PATH --rid linux-x64 --output PATH
python build_installer.py --dotnet PATH --payload PATH --rid win-x64 --output PATH
```

The script packages only manifest-owned files and embeds them in a self-contained executable. It excludes NVIDIA runtime DLLs and records the installer/payload hashes. Windows cross-compilation establishes a build, not Windows runtime behavior. Developer builds without the qualified payload explicitly refuse installation. A build receipt never grants publication approval.

Run `dotnet run --project tests/installer-core/InstallerCore.Tests.csproj -c Release`, the existing Python portable tests and the C++ display/token tests. Linux and Windows CI remain in place. Exact final candidate runtime evidence must satisfy [the Update 2 gate](UPDATE_2_RELEASE_GATE.md). FG needs additional early device/interposer integration and is not implemented by this Reflex bootstrap.

The Linux installer uses GTK file pickers directly rather than depending on a desktop portal. See [Avalonia file picker documentation](https://docs.avaloniaui.net/docs/services/storage/file-picker-options). Linux still needs its normal graphical desktop libraries; self-contained refers to the .NET application/runtime.

The qualification workflow packages the checked-in own payload ZIP after verifying every manifest hash and refusing vendor DLLs. Its output is a test artifact, not a GitHub release. Qualify the exact installer hash downloaded from that run on Windows; rebuilding changes the installer identity and requires recording the new hash.

Do not launch helper tools with `proton run` against a running game prefix: Proton setup can remove Steam's `s:` drive when game-library environment variables are absent. The PCL helper must use the same Proton's Wine binary directly and retain the mapping. The guided ReShade launch is allowed only with the game closed and supplies the selected game/library variables.
