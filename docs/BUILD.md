# Source build

The native source is C++20. Its ReShade calls use a separate MSVC-ABI adapter; the main Windows addon uses MinGW. Both adapters are intentionally retained from the tested path. Do not replace them with PR435 or an upscaler-provider patch without the separate regression matrix.

Pinned dependencies:

- NeoRune SDK and tool **0.1.2**, obtained from NuGet. The SDK provides the game bindings; these generated bindings are not copied into this repository.
- .NET SDK **10.0.401** (reference runtime **10.0.12**) for the tested UI compiler.
- ReShade SDK **v6.8.0**, addon API **20**; obtain headers from `crosire/reshade`.
- NVIDIA DLSS SDK commit **374959484e79a640feaba44c93ac8cfb0a03f5b5**, obtained from NVIDIA under its own license.
- LLVM/Clang, MinGW-w64 C++20 and Microsoft's DirectX Shader Compiler for the native/conversion shader build.

The SDK headers and vendor runtime DLLs are not relicensed or bundled. The parameterized `build.py` expects separately obtained SDK directories and tool paths. It does not download SDKs or accept licenses. Supply `--dotnet`, `--neorune-sdk`, `--pack-tools`, `--reshade-include`, `--ngx-include` and `--dxc`. The native toolchain recipe targets a Linux build host with MinGW and Clang. Windows source builds are not qualified. This parameterized recipe was executed successfully on the development Linux host. NeoRune package tools run through a separately supplied Wine wrapper on that host; Windows build execution remains unqualified. The local build uses NeoRune's command-line package compiler and verifies diagnostics even when its exit code is zero.

Runtime output goes to the game's local `Saved/MCD2Graphics` directory. The installed own conversion shader is `MCD2Graphics/live_dense.cso`; the externally supplied official DLSS runtime is `MCD2Graphics/ngx-runtime/nvngx_dlss.dll` beside the addon. Normal builds disable developer file controls.

The shipped converter is the originally qualified bytecode. Rebuilding with a different compiler can produce different bytecode; build output must be requalified and is not automatically packaged.
