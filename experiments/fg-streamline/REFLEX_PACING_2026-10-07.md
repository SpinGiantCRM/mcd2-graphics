# Wine Reflex pacing and dependency compatibility

Held candidate; no published release is replaced by this result.

## Reflex

3840 × 2160 native HDR, RTX 4080 SUPER / driver 615.71.09, CachyOS Proton
SLR `cachyos-11.0-20261005-slr`, WineWayland. DLSS and FG Off; VSync Off,
frame limit disabled. Same stationary offline tutorial location. Each sample
contains 1,200 CPU finish-present timestamps (1,199 intervals), without GPU
queries/readbacks. These are rendered frame intervals, not panel scanout or
input-latency measurements. Raw logs and captures remain private.

| Run | FPS | P99 interval (ms) | Interval standard deviation (ms) |
| --- | ---: | ---: | ---: |
| reversal-off-a | 105.55 | 10.73 | 0.39 |
| reversal-on-a | 102.85 | 20.68 | 4.34 |
| clean-off-a | 106.47 | 10.05 | 0.26 |
| clean-on-a | 106.34 | 10.77 | 0.42 |
| clean-on-b | 106.16 | 11.14 | 0.40 |
| clean-boost-a | 106.22 | 10.82 | 0.40 |

The clean patched runs use the original SDK bridge and latency addon, with only
the new bootstrap presentation compatibility setting active. They contain 64
complete SDK latency reports and no coordinator, marker or Reflex faults.
Reflex On averaged about 0.2% below Off across the two On runs. The user's full
100→80 FPS reduction was not reproduced in this scene; severe interval variance
was reproducible with the old path and returned when the fix was removed.

The fix disables KHR/EXT `swapchain_maintenance1` in the NVIDIA Wine process
before any graphics-device creation. Existing extension exclusions are retained.
It avoids dynamically unlocking a FIFO-created swapchain. It does not force an
IMMEDIATE present mode, replace Reflex sleep, change marker identity, override
VSync, or change the game's frame limit. Windows exits this compatibility path.
The setting can be disabled with `WineReflexPacing=0` in `[Compatibility]` in
`FGBootstrap.ini`. VSync On still produced greater interval variance (P99 17.57 ms)
on this host; it is not included in the fixed result.

Reference: [installed vkd3d-proton swapchain implementation](https://github.com/HansKristian-Work/vkd3d-proton/blob/44cf7c2042168f3/libs/vkd3d/swapchain.c).

## SR / FG and Loader 2.2

With runtime-default DLSS model selection and the clean bridge, DLAA + FG
measured 69.70 rendered / 139.40 SDK-reported presentations per second; Quality
measured 94.61 / 189.22. Input/output extents and current-process SR/FG revisions
were checked before and after each sample. Internal NGX error count was zero.
Quality→DLAA transitions and saved native-menu choices worked.

Official Blueprint Loader 2.2 passed repeated startup, offline gameplay, Video
settings, Mods metadata and preference persistence checks on this Linux host.
Windows runtime and a physical controller remain unqualified.

## Overrides and packaging

All five external dependencies have an explicit, default-Off **Try an untested
dependency version** control. Installed files can be explicitly acknowledged.
Actual selected hashes are recorded. Archive completeness, traversal/symlink
checks, file ownership, game identity and mod payload hashes remain enforced.
The optional restart-only `ModelPreset` hint leaves NVIDIA defaults unchanged at
zero; model 11 also passed DLAA/Quality + FG checks. See
[dependency overrides](../../docs/DEPENDENCY_OVERRIDES.md).

Installer tests: 86 core safety checks, 14 FG Python checks, 6 build-toolchain
checks and 13 legacy installer checks passed. Native model-hint and Wine extension
helpers passed. The GUI compiled without warnings or errors.

The expanded installer contains 12 project-owned files. Vendor runtimes are
external; the exact tested patched ReShade is a separate credited dependency.
Its lock now identifies the successful-reset lifetime patch and the tested DLL,
rather than the earlier unpatched Windows artifact. The cross-built framework
and modified own binaries need Windows requalification. This result does not
clear the separate render-hang hold or establish long-session compatibility.
