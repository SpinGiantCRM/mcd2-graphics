# Bounded DLSS Frame Generation experiment

This is an opt-in development experiment, **not a release or normal installer**.
FG has an Off / On control in Settings → Video and requires a restart after
changing modes. The UI path has no activation timer. The legacy command-driven
trial remains capped at 600 source frames. This candidate requires the Steam
build, supported NVIDIA FG hardware, NVIDIA DLSS SR/DLAA and native HDR output.
Do not replace a published release with it.

DLSS SR and DLAA enable vertex-deformation motion vectors in verified gameplay,
independently of FG and HDR. Native selection, unsupported-GPU fallback and level
teardown restore the original value. Existing external overrides are respected.
Only velocity output pass 2 is qualified for this correction.

## Architecture

The early DXGI bootstrap initializes Streamline before the game creates its
presentation swapchain. A source-built ReShade with PR435 remains the outer
wrapper. Streamline and SR bind the same existing device wrapper; manufacturing
a second wrapper or initializing NGX on a different device is unsafe.

The existing Reflex coordinator supplies one engine frame token for simulation,
render submission, constants, resource tags and Present. The measured View frame
stamp selects matching depth/motion guides. The final HDR compositor supplies
the world image before UI composition. An owned compute shader copies UI alpha
to R8; game resources are not modified. Streamline copies transient inputs at
their tagging command. FG declines frames without matching fresh inputs or
foreground ownership. Guides while Off request a history reset for resuming FG.

Retirement stops new tags, waits for command recordings to retire and verifies
a fresh GPU fence before releasing alpha resources. SR releases its NGX feature,
parameters and device ownership before notifying the shared Streamline owner
outside the SR mutex. Streamline shutdown results are recorded. A failed shutdown
retains provider ownership and does not detach the factory routes as if it passed.

Temporary menus retain FG resources and their accepted guide dimensions. A full
FG release retires that process’s FG session; re-enabling requires a restart.
An Off request may retire resources only after the UI binds to the current
process; default or previous-session startup state does not retire an On route.

## Required separate prerequisites

| Component | Pin |
| --- | --- |
| Steam shipping executable | `dependencies.lock.json` game hash; other builds declined |
| Streamline SDK and production archive | 2.14.1; archive hash in `dependencies.lock.json` |
| ReShade + PR435 | source commit `4eb9056c76016aad6f98495d3bbda2d721106104` |
| Tested PR435 binary | SHA-256 `35f8bb47eb54311a30a790ba0360125be4b70c1fe18acdfa91446b1f2cbb49d2` |
| ReShade headers | matching PR435 checkout |
| NGX headers | acquired official SDK, same prerequisite as native SR build |
| Tools | MSVC ABI Clang/LLD, Microsoft C++/Windows SDK sysroot, LLVM MinGW or MinGW GCC, DXC |

Dependencies must be acquired separately under their terms. The probe builder
verifies the Streamline archive before extracting signed production runtimes
into a local output folder. ReShade needs a clean-source build receipt containing
its commit and `binarySHA256`; a rebuild has its own hash and qualification.
This source directory does not contain vendor runtimes, game shaders or assets.

## Build

Run from the repository root. The variables below refer to separately acquired
local prerequisites and fresh output folders. Explicit compiler paths work on
Windows; retain the 16 KiB stack-frame guard and MSVC ABI bridges.

```text
python experiments/fg-streamline/build_probe.py --sdk SDK --sysroot SYSROOT --runtime-archive SDK_ARCHIVE --output PROBE --reshade-headers RESHADE_INCLUDE --early-bootstrap --guide-recon --dxc DXC --clang-cl CLANG_CL --lld-link LLD_LINK
python experiments/fg-streamline/build_game_candidate.py --probe-output PROBE --reshade-headers RESHADE_INCLUDE --output LATENCY --mingw-cxx MINGW_CXX
python experiments/fg-streamline/build_sr_candidate.py --output SR --reshade-headers RESHADE_INCLUDE --ngx-headers NGX_INCLUDE --clang-cxx CLANG_CXX --mingw-cxx MINGW_CXX
```

The SR/latency builders generate isolated sources and hash them. They do not edit
the released sources. Keep all build receipts with the candidate.

The synthetic `fg-probe.exe` has separate static owned inputs. On Windows run it
from its output folder with `MCD2_FG_HDR=1`, `MCD2_FG_EARLY_BOOTSTRAP=1` and
`MCD2_FG_NATIVE_QUEUE=1`, placing the qualified ReShade binary as `d3d12.asi`.
It tests Off → On → Off and normal retirement. It must pass before a game trial.
`run_probe.py` is the Linux-only isolated Proton runner; never use a game prefix.

## Reversible game trial

Close the game. Start from the matching working release/dependencies. Installation
refuses a different shipping executable, framework or pre-existing trial files.
The backup folder must be new. It retains original files and exact hashes.

```text
python experiments/fg-streamline/game_trial.py install --game GAME_WIN64 --backup NEW_BACKUP --probe PROBE --candidate LATENCY --sr-candidate SR --ui-candidate UI/Pak --experimental-reshade PR435_DLL --experimental-reshade-receipt PR435_RECEIPT
```

Create `FGGuideCapture.ini` beside the game executable, then launch normally:

```ini
[Capture]
Enabled=0
GuideTags=1
ImageTags=1
EnableFG=0
TrialRevision=1
NativeUIToggle=1
```

Enter gameplay with DLSS Quality and HDR On. Use the Video menu to select FG;
restart after changing modes. Follow [the Windows checklist](WINDOWS_FG_CLEARANCE_2026-10-06.md).

For the separate legacy command-driven trial, set `NativeUIToggle=0` before
launching. Stay in solo gameplay. Use the bounded trial below with a **new revision each time**; move normally during the
second run. Its output omits identifiers, paths and addresses. Raw game/SDK logs
and screenshots stay private. The Python controller forces Off in its finally
handler; the addon independently enforces the 600-frame cap.

```text
python experiments/fg-streamline/bounded_trial.py --policy GAME_WIN64/FGGuideCapture.ini --log LOCALAPPDATA/Dungeons2/Saved/MCD2Graphics/FGGuideRecon-private.jsonl --revision 2 --output stationary.json
python experiments/fg-streamline/bounded_trial.py --policy GAME_WIN64/FGGuideCapture.ini --log LOCALAPPDATA/Dungeons2/Saved/MCD2Graphics/FGGuideRecon-private.jsonl --revision 3 --output movement.json
```

Close normally before restoring. **Always restore, even after a failed test.**

```text
python experiments/fg-streamline/game_trial.py restore --game GAME_WIN64 --backup NEW_BACKUP
```

Do not manually rename or overwrite files to bypass a failed restoration check.
Retain the backup and diagnose the mismatch. Player saves are not transaction
targets. Return any settings changed through the game's menu yourself.

## Windows qualification gates

**AMD/unsupported GPU:** startup, normal gameplay, existing Native/DLSS fallback,
main/pause menus and repeated clean shutdown. FG must remain unavailable/Off;
the actual rendering adapter is selected by device LUID. A running process alone
is insufficient: verify the trial's bootstrap and addons were loaded.

**Supported NVIDIA GPU:** native Windows 10 20H1+ or Windows 11, current driver,
RTX 40/50-class FG-capable GPU, HAGS enabled and SDK support check successful.
Run the synthetic gate first, then stationary and movement game gates, Off/On transitions with their required restarts, existing SR/HDR/Reflex checks and three normal shutdowns. Record
Windows/driver/GPU, game build and executable hash, every own binary hash and
framework hash. AMD fallback cannot qualify active NVIDIA FG.

For each game exit require SR `cleanupCompleted=true`,
`generationRetained=false`, alpha `GPUCompletionVerified=true` with zero recorded
leases, and shared SDK shutdown `SDKResult=0`. Also require the real process to
exit without a crash reporter or new application fault. Receipt success alone
does not prove that all native COM references retired.

## Outstanding release gates

- Native menu mouse/keyboard/controller qualification and persistence across restart.
- Independent Native-upscaler guides and SDR path.
- Explicit menu/pause/loading suppression and travel/cutscene reset coverage.
- Resolution/window/swapchain/device recreation and repeated long sessions.
- Command replay/cancellation, complete native COM retirement and concurrency.
- Moving-object/disocclusion/particles/UI quality, latency and frame pacing.
- Wine backend RSYNC/Reflex warnings separated from native Windows behavior.

Linux gameplay can establish a bounded implementation milestone. It cannot
qualify these Windows or release gates. See
[NVIDIA FG guidance](https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS_G.md)
and [Reflex guidance](https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideReflex.md).

## Current Windows candidate

The current source is [v31](../../qualification/fg-v31/README.md). The separate
[Windows / AMD trial report](../../docs/FG_WINDOWS_AMD_VALIDATION_2026-10-06.md)
records exact Windows build hashes, reversible installation, two clean exits and
unsupported fallback with FG Off and persisted On. Active NVIDIA FG/HDR and
shared NGX teardown remain outstanding. Historical v23 assets in
[`qualification/fg-v23`](../../qualification/fg-v23/README.md) remain frozen.
The historical bounded Linux measurements are in
[`LINUX_DLSS_FOLIAGE_FG_CANDIDATE_2026-10-06.json`](LINUX_DLSS_FOLIAGE_FG_CANDIDATE_2026-10-06.json).
These measurements do not qualify Windows execution or a release.

## v31 shared NGX lifecycle

[Candidate v31](../../qualification/fg-v31/README.md) replaces v27 for testing.
SR preset changes preserve Streamline's device-level NGX instance; SR releases
its own feature and Streamline shuts NGX down at final retirement. The
[Linux diagnosis and corrected measurements](LINUX_FG_NGX_LIFECYCLE_2026-10-06.md)
explain why the earlier returned-DLAA performance result was invalid.

Verify internal FG evaluation errors as well as input dimensions and SDK counts
through Quality/DLAA transitions. Windows and long-session qualification remain
required. No published release changed.
