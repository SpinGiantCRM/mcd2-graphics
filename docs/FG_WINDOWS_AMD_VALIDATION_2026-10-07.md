# PR10 FG / Reflex Windows AMD regression — 7 October 2026

The rebuilt candidate at `1389ceb21c6348920b7071215724d824f37a347b` passed
the reversible FG trial installation, the 10-second startup gate, and unsupported
AMD fallback with Blueprint Loader 2.2. Two gameplay runs exited with code 0.
The FG-Off Save and quit exit was delayed; its cause is unresolved. No native
source change was needed for the tested fallback. Active NVIDIA operation and
the public installer remain separate qualification gates.

## Exact build, independently verified

- [Windows FG build 37547389139](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37547389139), artifact `fg-windows-candidate`.
- Tested branch head: `1389ceb21c6348920b7071215724d824f37a347b` (`Stabilize Wine Reflex pacing and add explicit dependency overrides`). The remote PR10 head was rechecked during the trial and matched.
- GitHub checked out merge `5c7c8ac745656a26b49e1dc2fa84db897946a7a9`: branch parent `1389ceb21c6348920b7071215724d824f37a347b`, base parent `7f0863b5d186ac855b3640ee7537402194521dc0`. The different merge SHA does not identify an older build.
- Downloaded artifact ZIP SHA-256 `d8ce7e51e7257f2d8c5c8d62dedaf43f0c2c5b2ef369acf4f5f56930f06a791c`, matching GitHub's artifact digest. All 18 artifact files matched their Windows receipt.
- All 14 probe source hashes, four SR source/generator hashes, the latency source and the current UI git blob matched the receipts. UI payload reuse is supported by unchanged source, not by an old runtime result.
- Both real shipping processes independently loaded six game-directory modules matching this candidate: DXGI bootstrap, patched PR435 framework, FG bridge, guide addon, SR addon and display/latency addon.
- Framework base `4eb9056c76016aad6f98495d3bbda2d721106104`, approved successful-Reset patch SHA-256 `109e99a0b160e2d2b1baf260b51bff461333aaf328554838ce994dac9abcfbc7`. The checkout was clean before patching, and only the approved patch was applied; it is a patched build.
- Exact Windows framework hash `f205cee9c19d85100631ac2cf122769c710cd8f552827dad10625c00579ab351`; SR hash `7267a9a6db3ee13e8de23c94219132e11904cd3777758a73b1ae132277dab0b9`. Neither the previous Windows pair nor the separately cross-built release pair qualifies these files.
- CRT `14.44.35207`, Windows SDK `10.0.26100.0`. Streamline production 2.14.1 was hydrated separately from the pinned SDK archive; vendor runtimes were not bundled in the artifact or published with this report.

The [sanitized aggregate](../qualification/fg-release/windows-amd-checks-2026-10-07.json)
records all candidate hashes, current acknowledgements, checks and restoration.
Build-time receipt qualification flags were not rewritten. This record qualifies
only the exact tested binaries for its stated AMD scope, not a future rebuild.

## Environment and installation

Windows 11 Pro `10.0.26200`, AMD Radeon 860M, driver `32.0.31041.1004`,
1920 × 1200. Steam app `1912410`, build `25647713`; shipping executable
SHA-256 `231147bd0c655a4ae73f90873675d42917f2bfb3a9ee164fc64f217d6d6bd4ef`.
Native HDR was Off. RenoDX remained installed.

The maintainer downloaded Blueprint Loader 2.2. Its archive matched the new lock:
`cfadee0563c17684c1a3055c41bab7d5ca12f772a856bca6cb6bbea44744dc28`.
All three installed containers were independently verified; the title screen
showed v2.2. The preliminary Loader 2.0 title-screen launch was a startup control
only and does not stand in for the two Loader 2.2 gameplay runs below.

`game_trial.py install` staged the full FG chain, temporary SR and matching UI
with a recovery transaction. Loader 2.2 had a separate verified transaction.
After both processes closed, both transactions were restored. All 26 baseline
game files matched their original hashes. Six original mod/video preference
files were restored, two new FG slots removed, and the test model-hint INI
removed. Input preferences were unchanged. Player and account slots were not
rolled back; private logs remain local. Frozen preview.1 assets were untouched.

This was the real developer FG trial transaction. It did not execute the expanded
public GUI's fresh-install, repair or removal flows.

## Runtime observations

| Check | FG Off / saved Reflex On | Saved FG On / Reflex Boost |
| --- | --- | --- |
| Local launch → exit | 10:32:37 → 10:43:03 | 10:43:25 → 10:50:27 |
| Initial observation | 40.7 s, survived | 36.8 s, survived |
| Six loaded candidate hashes | Matched | Matched |
| Main menu / gameplay | Reached | Reached |
| FG acknowledgement | Current revision/session, unavailable, inactive | Revision 16 matched; current gameplay session, unavailable, inactive, Phase 2, no restart |
| SR gameplay request | Quality 67%, revision 102 | Custom 77%, revision 111 |
| Adapter rejection | AMD vendor 4098, Phase 3 / error 1 | AMD vendor 4098, Phase 3 / error 1 |
| Source revision | 100, no acknowledgement of rejected revision 102 | 109, no acknowledgement of rejected revision 111 |
| Reflex acknowledgement | Request On, revision 2 matched; unavailable, active Off, fault 0 | Request Boost, revision 3 matched; unavailable, active Off, fault 0 |
| Shipping / launcher exit | 0 / 0 | 0 / 0 |
| Close route | System → Save and quit to desktop | Normal Windows Alt+F4 |

The second run seeded only mod-owned FG/Reflex preferences while closed, modelling
a profile brought from NVIDIA hardware. Its bootstrap queried the actual rendering
adapter: support result 6 (unsupported), device binding -1, no routed FG swapchain
and no initial FG configuration. It preserved the original swapchain path.

Both current-session UI logs recorded Native fallback restoration, no source-scale
acknowledgement for the rejected gameplay request, and no foliage velocity lease.
Requested percentages are saved intent, not proof of reduced source resolution.
The first run additionally returned Quality → Native through Video controls:
revision 103 matched, scale 10000, Phase 2 / error 0, current source/session.
The second run used VSync Off and a restart-only `ModelPreset=11` fixture;
unsupported AMD rejection happened before NGX feature creation. This does not
qualify model-hint selection on NVIDIA. DLAA was selected in the main menu but
changed to Custom before gameplay; no DLAA gameplay pass is claimed.

The Wine Reflex pacing event was absent in both native Windows bootstrap logs.
Reflex status reported coordinator/marker errors 0 and remained inactive.
FG guide cleanup recorded zero command leases and completed alpha retirement on
this no-FG-allocation path. It cannot prove retirement of allocated NVIDIA resources.

The FG-Off window disappeared before the process finished. At 10:41:50 the process
still existed with no window and unchanged CPU time; Steam recorded exit 0 at
10:43:03, at least 73 seconds later. A read-only debugger attachment was denied;
no stack or root cause was obtained. The FG-On normal close finished promptly.
No process was killed, and no matching Application Error / WER / hang event was
returned for the trial window. The delayed exit is retained as an unresolved
observation, not declared fixed or attributed to the new Reset/Reflex change.

Gameplay keyboard input was intermittent under automation; one run's System and
Video controls were operated without maintainer assistance. Player movement was
attempted but displacement was not independently verified. The UI also hides
unsupported FG/Reflex rows while leaving reconstruction visible. That is current
UI support gating, not evidence of an older binary; its inconsistency is unchanged.

## New changes and Windows requirements to preserve

| Change since the previous Windows record | Why it exists | Windows / AMD requirement and evidence |
| --- | --- | --- |
| Confirm Reset success with a private epoch | ReShade emits its Reset event before the native call; dormant old recordings otherwise retain SR resources. | Preserve pre-call event ordering; invalidate only after successful native Reset, then require signal/completed fence. Failed/missing proof retains resources. New patched framework/SR pair loaded on Windows; retirement unit checks passed. AMD cannot exercise an allocated DLSS feature. |
| Stable patch bytes and recorded patched provenance | Windows checkout line endings changed the approved patch's hash. | Keep LF patch bytes, clean-base/approved-patch checks and patched receipt status. Windows build and all artifact/source hashes passed. |
| Wine-only Reflex pacing override | NVIDIA Wine's dynamic swapchain-mode switching disturbed the tested Reflex/VSync-Off combination. | Native Windows must return before NVAPI/environment modification. Wine override/opt-out checks passed; native Windows logs contained no pacing event, startup passed, AMD Reflex On/Boost both stayed inactive without faults. Active Windows NVIDIA Reflex remains untested. |
| Optional restart-only DLSS model hint | Enables explicit NVIDIA model experiments without per-frame work; invalid/absent values preserve defaults. | Unicode INI path, bounds/default checks and unsupported-adapter rejection must remain. Windows model tests and default/explicit-hint AMD fallback passed; actual NVIDIA feature creation was unavailable. |
| Loader 2.2 and per-dependency overrides | Pins the newer loader and permits acknowledged external-version experiments. | Own payload/game identity, archive bounds, selected-file completeness, closed-game refusal and receipt ownership remain strict; overrides default Off. Exact Loader 2.2 trial install/restore passed; installer/core checks passed. Expanded public GUI runtime flows remain untested here. |

## Checks and limits

Local checks passed: 14 FG Python tests, six native build-recipe tests, 13 installer
Python tests, 85 GUI core checks, 31 foliage policy checks, and Windows-compiled
Wine-pacing, borrow/Reset retirement, model, adapter, camera and FG configuration
executables. The Windows Reflex token executable passed 31 checks. Native test
executables used static runtime linkage and their exit codes were checked.

[CI 37547389174](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37547389174)
passed portable Linux, Windows installer and both GUI-core jobs for the tested
source. Windows FG build 37547389139 and ordinary installer build 37547389227
also passed. Both Windows and Linux jobs are retained.

Active NVIDIA FG/Reflex, HDR, model-preset effects, allocated SR/NGX retirement,
the expanded GUI runtime flows, and the historical render-thread hang/shutdown
exception remain unqualified. Release remains held. No speculative rendering
fix or UI policy change was made for this test.
