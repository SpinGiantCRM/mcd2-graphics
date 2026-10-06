# Linux cross-build recipe for the held framework candidate

Uses Clang-cl/LLD and separately obtained, licensed Microsoft C++ and Windows SDK
files arranged as `crt/include`, `crt/lib/x86_64`, `sdk/include/{ucrt,shared,um,winrt}`
and `sdk/lib/{ucrt,um}/x86_64`. SDK files are not distributed here.

Start with a clean checkout of JoeyDelp/ReShade commit
`4eb9056c76016aad6f98495d3bbda2d721106104`, with its pinned submodules initialized.
Apply `qualification/fg-release/reshade-reset-epoch.patch` and verify the only
source difference is that patch before and after building.

```sh
git -C "$RESH_SOURCE" apply "$MOD_SOURCE/qualification/fg-release/reshade-reset-epoch.patch"
cmake -S "$MOD_SOURCE/qualification/fg-release/cross-build" -B "$BUILD_DIR" \
  -DCMAKE_TOOLCHAIN_FILE="$MOD_SOURCE/qualification/fg-release/cross-build/msvc-toolchain.cmake" \
  -DWINDOWS_SYSROOT="$SDK_FILES" -DFRAMEWORK_SOURCE="$RESH_SOURCE" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --target ReShade -j 4
```

The harness adds host-compatible resource paths, compiler options and linkage
aliases. It does not replace framework functions. `build-receipt.json` records
the base commit, patch hash, resulting DLL hash and qualification status. Output
hashes can differ between toolchain versions; this recipe does not promise a
byte-identical build on every host.

For Windows/MSBuild, use `experiments/fg-streamline/build_windows_candidate.py`.
That output is a separate candidate and must be tested with its exact framework.
