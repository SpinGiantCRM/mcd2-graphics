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
