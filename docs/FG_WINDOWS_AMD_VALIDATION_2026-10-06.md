# PR10 FG Windows / AMD regression — 6 October 2026

The current v31 source passed the reversible trial installation, the 10-second
startup gate, unsupported AMD fallback and two normal Windows close requests.
The shipping process exited with code 0 both times. No rendering change was
needed to make the tested AMD path decline FG and preserve Native rendering.
This qualifies the exact trial binaries below for these bounded AMD checks.
It does not qualify active NVIDIA FG, shared NGX teardown or a public installer.

## Tested artifact

- [Windows candidate build 37454118859](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37454118859), artifact `fg-windows-candidate`.
- Artifact checkout: PR merge commit `6d5077177c26bfd1423f7b84736576319f45d2ee`, with branch parent `1648708880cc47ad0352176dd109bdb3a34c48bf` and base parent `7f0863b5d186ac855b3640ee7537402194521dc0`.
- Current bridge, bootstrap, guide observer, latency and SR sources were checked against their receipts. The historical UI is reused only because its current git source blob matches its receipt.
- All 18 artifact files matched the build receipt before installation. Six independently loaded modules matched the candidate hashes in both real processes: game-directory DXGI bootstrap, PR435 framework, FG bridge, FG guide observer, SR and display/latency addons.
- ReShade PR435 source `4eb9056c76016aad6f98495d3bbda2d721106104`, built clean with full addon support. Its Windows hash is `3b7e8647f3ccebc6b4b177a7d18f226ed8c956e56179b897eaefa4eac8be6704`; this is a separate binary from the Linux qualification.
- Streamline production runtime 2.14.1 was acquired separately from the pinned archive, SHA-256 `92c4d954631a1710da86ca3fa8d5034f2b9503838c95fc4ae977ae149319781b`. Runtime DLLs are not part of the test artifact or this record.
- Microsoft CRT `14.44.35207`, Windows SDK `10.0.26100.0`.

[The aggregate record](../qualification/fg-v31/windows-amd-checks.json) contains
exact candidate hashes and sanitized observations. Qualification does not
transfer to a later rebuild, even if source code is unchanged.

## Environment and installation

Windows 11 Pro `10.0.26200`, AMD Radeon 860M, driver `32.0.31041.1004`,
1920 × 1200. Steam app `1912410`, build `25647713`, shipping executable SHA-256
`231147bd0c655a4ae73f90873675d42917f2bfb3a9ee164fc64f217d6d6bd4ef`.
Native HDR was Off; this laptop did not provide the required supported NVIDIA/HDR
environment. RenoDX and Blueprint Loader remained present.

`game_trial.py install` verified prerequisites and staged the full experimental
chain, temporary SR and matching UI with a recovery transaction. After both
processes closed, `game_trial.py restore` succeeded. All 26 baseline game files
were independently hash-verified afterward, including dependencies, UI,
configuration and the original addons. Trial DLLs were removed. Only the test's
mod preferences, video preferences and temporary control binding were restored;
player and account slots were not rolled back. Private runtime evidence remains
local. No published assets or frozen preview.1 files changed.

This was a real FG trial transaction, not a run of the expanded public GUI
installer. The ordinary candidate installer still packages rc.1 rather than FG;
its fresh-install/repair/removal gates are separate.

## Runtime observations

| Check | FG Off launch | Persisted FG On launch |
| --- | --- | --- |
| Local launch → exit | 21:40:27 → 21:54:54 (867 s) | 21:57:50 → 22:05:48 (478 s) |
| Initial bounded observation | 37.8 s, survived | 30.0 s, survived |
| Full loaded candidate chain | Hash matched | Hash matched |
| Main menu and gameplay | Reached; mouse movement observed | Reached; mouse movement observed |
| FG acknowledgement | Revision 5, current session, Available 0, Active 0, Phase 0, RestartRequired 0 | Revision 12, current session, Available 0, Active 0, Phase 2, RestartRequired 0 |
| DLSS Quality request | Revision 100 declined, vendor 4098 (AMD), phase 3/error 1 | Revision 105 declined, vendor 4098 (AMD), phase 3/error 1 |
| Shipping / launcher exit | 0 / 0 | 0 / 0 |

The FG On fixture changed only the mod-owned FG preference while the game was
closed, modelling a profile brought from supported hardware. Its fresh runtime
revision and session matched in gameplay. The actual rendering adapter support
query returned 6 (unsupported); device binding remained -1, and neither initial
FG configuration nor a routed FG swapchain was recorded. The game kept its
original swapchain path. The Video screen did not offer an available FG control.

Both DLSS Quality requests were declined before a source-scale acknowledgement
for the requested revision. Run 1's own UI log explicitly recorded Native
fallback restoration, no Quality source-scale application and no foliage
override. Run 2 independently repeated adapter rejection and the unchanged
source revision. The saved 67% Quality value describes requested intent, not
actual reduced rendering resolution. AMD never established active DLSS, so the
DLSS foliage lease must remain unapplied.

The guide observer recorded zero command leases and verified alpha retirement
completion on this no-FG-allocation path. This cannot establish active NVIDIA
resource retirement. No SR queue-teardown receipt was produced for an allocated
NGX feature; do not infer shared NGX cleanup from an AMD exit. There were no
matching Application Error / Windows Error Reporting events in the trial window.

Close requests used normal Windows Alt+F4, with actual shipping and launcher
exit codes checked in Steam's process log. Injected gameplay I/Escape and a
temporary middle-mouse inventory binding did not reliably open the gameplay
menu. Main-menu mouse controls worked. This run therefore does not claim the
in-game Save and quit route or all gameplay preset transitions. A temporary
Windows on-screen keyboard was also unable to inject gameplay keys and could
not be closed by the automation at its higher integrity level; it did not block
the recorded fallback or process checks.

## Changes and the Windows requirement each preserves

| Change | Why it was needed | Windows quirk / preservation requirement |
| --- | --- | --- |
| Dedicated Windows FG build job and private compiler sysroot | Build the current experiment instead of treating a Linux archive as Windows evidence | The Streamline bridge requires Microsoft C++ ABI and CRT/Windows SDK headers and libraries. LLVM MinGW alone builds SR but cannot substitute for that bridge toolchain. Private junctions adapt VS layout without modifying installed tools. Preserve both Linux and Windows jobs. |
| Build pinned ReShade through `ReShade.sln` | The first Windows candidate failed to find dependency headers | Upstream include paths depend on MSBuild `SolutionDir`; invoking the project directly leaves them empty. Keep the solution build and clean-source receipt. |
| Select complete numeric SDK versions, with two regression fixtures | The second Windows build failed parsing `wdf` as a version | Windows Kits mixes versioned SDK folders with unversioned components and potentially incomplete versions. Ignore nonnumeric/incomplete entries; choose the newest complete numeric version. |
| Compare historical UI receipt to the git source blob | Preserve source verification while reusing the exact unchanged UI | Windows checkout CRLF can differ from Linux working bytes. Git blob identity is the comparison; do not weaken source provenance or rebuild frozen archives. |
| Opt-in `-RequireFG` runtime probe, loaded-module hashes | Prove the complete experimental chain actually loaded | A live Windows process or system DXGI module does not prove the game-directory FG bootstrap and addons loaded. Hash all six actual game-directory modules at the first and last samples; fail if the chain is incomplete. Keep legacy SR/latency probes usable. |
| Candidate-specific aggregate and compatibility notes | Keep AMD evidence distinct from Linux/NVIDIA and future builds | Clean exits and unsupported fallback do not establish active FG, HDR, shared NGX teardown or the cause of the historical shutdown exception. |

The first three implementation commits are `5226e77`, `b6e83fa`, and `1648708`.
The follow-up runtime probe/report changes do not modify native rendering,
adapter policy, resource lifetime, stack guards or the installer payload.

## Checks and outstanding gates

Local checks passed: 11 FG Python tests (including both Windows SDK fixtures),
6 native recipe tests, 13 installer Python tests, 49 GUI core checks, 31 foliage
ownership checks, and Windows camera/configuration, unsupported-adapter and
borrow-lifetime executables. [Portable CI 37454118855](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37454118855)
passed its Linux and Windows jobs; the Windows FG artifact build passed separately.

Active NVIDIA FG/HDR, Quality↔DLAA shared NGX transitions, internal NGX evaluation
errors, long sessions, device loss, active-resource teardown and the expanded
GUI install/repair/remove remain unqualified by this AMD trial. The historical
PR6 `0xc0000005` exit remains disclosed; these two clean exits do not identify
its cause. FG is still experimental and no release publication is authorized.
