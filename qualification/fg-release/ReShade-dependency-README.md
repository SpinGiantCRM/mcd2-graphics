# ReShade PR435 dependency for MCD2 Graphics

ReShade by crosire and contributors; PR435 implementation by JoeyDelp.
This separate source-built framework is not an official ReShade release.

Source: https://github.com/JoeyDelp/reshade/tree/4eb9056c76016aad6f98495d3bbda2d721106104
Build recipe: https://github.com/SpinGiantCRM/mcd2-graphics/blob/development/fg-bounded-windows/experiments/fg-streamline/build_windows_candidate.py
Patch: https://github.com/SpinGiantCRM/mcd2-graphics/blob/development/fg-bounded-windows/qualification/fg-release/reshade-reset-epoch.patch
Patch SHA-256: 109e99a0b160e2d2b1baf260b51bff461333aaf328554838ce994dac9abcfbc7

This candidate adds successful-command-list-reset lifetime confirmation. It is
not the unpatched framework from build 37454118859. This separate candidate uses
the upstream MSVC solution instead of the failing Clang-cl/CMake cross-build.
The enclosed receipt pins the binary, toolchain, approved source diff and eleven
submodules. It does not include the separately investigated descriptor-heap fix.

To rebuild, check out the source commit above with its pinned submodules, verify a
clean tree, apply reshade-reset-epoch.patch with git apply --index, then build:

    msbuild ReShade.sln /t:ReShade /p:Configuration=Release /p:Platform=64-bit /m:2

The tested binary used MSVC 14.44.35207, MSBuild 17.14.51.32402 and Windows SDK
10.0.26100.0. Compiler, recipe and flags changed together. This is a Windows
shutdown mitigation candidate; clean bounded observations do not establish the
precise cause or guarantee all machines. Each rebuild requires its own binary
hash and runtime qualification. The build-time WindowsQualified field is false;
runtime evidence is recorded separately in docs/WINDOWS_MSVC_MITIGATION_2026-10-07.md.

Select this ZIP in the MCD2 Graphics installer under ReShade. It verifies the
archive and binary hashes and stages the framework as d3d12.asi. Do not rename
or manually replace other graphics proxies. The mod installer records the
original supported proxy for uninstall recovery. See LICENSE.md for licensing.
This is a held test candidate, not a published dependency download.

No NVIDIA runtimes, game files or private data are included.
