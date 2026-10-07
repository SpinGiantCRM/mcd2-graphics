# ReShade PR435 dependency for MCD2 Graphics

ReShade by crosire and contributors; PR435 implementation by JoeyDelp.
This separate source-built framework is not an official ReShade release.

Source: https://github.com/JoeyDelp/reshade/tree/4eb9056c76016aad6f98495d3bbda2d721106104
Build recipe: https://github.com/SpinGiantCRM/mcd2-graphics/blob/development/fg-bounded-windows/experiments/fg-streamline/build_windows_candidate.py
Patch: https://github.com/SpinGiantCRM/mcd2-graphics/blob/development/fg-bounded-windows/qualification/fg-release/reshade-reset-epoch.patch
Patch SHA-256: 109e99a0b160e2d2b1baf260b51bff461333aaf328554838ce994dac9abcfbc7

This candidate adds successful-command-list-reset lifetime confirmation. It is
not the unpatched framework from build 37454118859. The enclosed receipt identifies
the exact binary and toolchain: Release x64 full addon support, cross-built using
Clang-cl, Microsoft C++/Windows SDK libraries and the CMake harness in
qualification/fg-release/cross-build. Windows runtime qualification is pending.
The Windows build recipe linked above applies the same source patch using MSBuild;
its output has a different hash and requires its own qualification.

Select this ZIP in the MCD2 Graphics installer under ReShade. It verifies the
archive and binary hashes and stages the framework as d3d12.asi. Do not rename
or manually replace other graphics proxies. The mod installer records the
original supported proxy for uninstall recovery. See LICENSE.md for licensing.
This is a held test candidate, not a published dependency download.

No NVIDIA runtimes, game files or private data are included.
