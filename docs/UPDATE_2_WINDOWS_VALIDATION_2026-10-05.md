# PR7 Windows / AMD regression — 5 October 2026

The exact `0.2.0-rc.1` candidate passed installation, bounded startup, AMD fallback and three normal process exits on this laptop. No candidate crash was observed. This is partial Windows qualification: the original [release gate](UPDATE_2_RELEASE_GATE.md) had outstanding checks below. The later publication decision is recorded at the end of this report. Clean exits do not identify or erase the earlier PR6 `0xc0000005` shutdown.

## Artifact and environment

- Tested payload source: `f032d00b23669720ee58f6d130f41cc6ea6586bb`, PR7.
- Downloaded Windows installer: [candidate CI run 37284816608](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37284816608).
- Installer SHA-256: `19f520a6e5c76bb53d1f8affe77a84b4b1880c3363127e2136409153542f2a1d`.
- Embedded payload archive SHA-256: `978c2609d6e63ecab4ed79ae4caa0390efb340756f46b9b65c7ecaf33ce0f85d`.
- Qualification input archive SHA-256: `62b9e42f162f13423858cdfa19d3d90173666f45ba0b53c8249474f94af27116`.
- Windows 11 Pro `10.0.26200`; AMD Radeon 860M, driver `32.0.31041.1004`; display `1920×1200`, system HDR unavailable during the test.
- Steam app `1912410`, build `25647713`; executable SHA-256 `231147bd0c655a4ae73f90873675d42917f2bfb3a9ee164fc64f217d6d6bd4ef`.
- ReShade full addon support `6.8.0.2155`; RenoDX UE Extended `nightly-20260928`; Blueprint Loader `2.0`; Streamline SDK `2.14.1`; pinned private DLSS runtime.

All seven project-owned payload hashes matched the manifest before and after installation. [windows-checks.json](../qualification/update2/windows-checks.json) records exact tested hashes and aggregate results. This follow-up changes evidence tooling/documentation only; it does not rebuild the payload. Original preview.1/preview.2 assets and qualification inputs remain frozen.

## Installation and preservation

The standalone GUI autodetected Steam and checked the game fingerprint/build. It correctly identified older Blueprint Loader and missing Reflex dependencies. Official Blueprint Loader 2.0 and Streamline 2.14.1 archives were selected through the Windows file picker and hash-verified before placement. GUI Repair / Verify migrated the known previous receipt. Previous Blueprint files were retained as hash-named recovery copies. Already-correct ReShade, RenoDX, SR/shader and private DLSS files were preserved. Launch through Steam succeeded.

The receipt identifies `0.2.0-rc.1` and eight owned files: seven project files plus the private DLSS runtime. Game-scoped `Dungeons/Config/UserEngine.ini` owns only `r.AllowHDR=1` and `r.AntiAliasingMethod=2`. Fresh pre-install copies established that installation did not change account/character saves or Saved configuration. Subsequent legitimate gameplay writes are not counted as installer changes.

Real removal and fresh installation used the unmodified installer core compiled from the tested source, including its real Windows closed-game guard. These two operations used the core API, not the GUI buttons. Removal deleted all eight owned files, receipt and newly created startup configuration. All 42 retained inventoried files, including saves and shared dependencies, remained byte-identical. Fresh installation restored the entire prior inventory. Sanitized diagnostics reported game/dependency fingerprints, OS and GPU without private paths, saves or raw logs.

GUI Repair restored a deliberately missing `live_dense.cso` to its manifest hash. A one-byte modification was then refused with “Existing or modified file retained”; the altered hash remained unchanged. The original was restored from the verified backup and repair passed. No player file was modified for these tests.

## Native Video and unsupported hardware

Main-menu and gameplay settings worked with the mouse. Stock Display Mode, Resolution and Brightness retained their order; HDR and its three calibration controls appeared directly after Brightness. Graphics controls continued below. Full Screen `1920×1200` to Windowed `1680×1050` succeeded.

Requesting HDR On with system HDR unavailable retained Off and showed an unavailable message. Calibration remained visibly disabled; clicking disabled controls did not change values. Defaults were 1000/203/203 nits. Canceling Reset preserved settings; confirming restored Native and HDR Off. Both startup keys survived shutdown unchanged. The initial HDR restart flag cleared after relaunch: intent/runtime HDR revision 2 matched, `hdrRestart=0`, with no Reflex coordinator or SDK marker error. This establishes the Off/restart path, not HDR output on capable hardware.

Reflex was absent beside FPS Limit on AMD. Saved `ReflexMode=1` remained intact; current reports said `reflexAvailable=0`, effective Off and `sdkReportsAvailable=0`. The bridge and four pinned Streamline modules loaded; bootstrap initialized successfully with result 0. Coordinator completed-frame counts on AMD do **not** establish NVIDIA `sl.reflex` execution.

Blueprint Loader 2.0 displayed MCD2 Graphics name, description, candidate version and author. Settings remained in native Video.

## SR fallback, travel and shutdown

| Request | Current acknowledgement / observed result |
| --- | --- |
| Frontend Quality | Revision 82, mode 2, scale 6667; context not ready, effective Native. Saved selection activates in gameplay. |
| Gameplay Quality | Revision 84, phase 3, error 1; AMD vendor 4098 declined in the same frame as the acknowledgement. Source revision remained 82; reduced source was not acknowledged. UI: “DLSS unavailable; native anti-aliasing is active.” |
| DLAA | Revision 85, mode 1, scale 10000; matching current fallback acknowledgement and same-frame adapter decline. |
| Custom 85% | Revision 86, mode 2, scale 8500; matching current fallback; source revision still 82. |
| Reset to Native | Revision 87, scale 10000; source/runtime revision 87, phase 2, error 0. |
| Restart Native | Revision 92, scale 10000; context/source sessions current and matching; source/runtime revision 92, phase 2, error 0. |

Run 1 entered the world, exercised pause Video/reset and fast traveled from the hub to Honeycomb Fields. Run 2 loaded that world after restart. Menus, rendering and transitions remained responsive. Startup probes verified SR/RenoDX; extended probes additionally required the display/latency addon. Run 1's 70.5-second extended observation covered launch ages 798.0–868.5 seconds, with all samples responding.

| Run | Local launch / exit time | Duration | Exit request | Steam shipping-process exit code |
| --- | --- | --- | --- | --- |
| 1 | 19:43:21 / 20:04:58 | 21m 37s | Gameplay Save and quit; mouse confirmation | 0 |
| 2 | 20:05:47 / 20:10:10 | 4m 23s | Normal Windows Alt+F4 from gameplay | 0 |
| 3 | 20:11:36 / 20:16:14 | 4m 38s | Main-menu Quit; mouse confirmation | 0 |

Times are Australia/Adelaide, UTC+10:30; durations use Steam's second-resolution records. Window disappearance was followed by actual process exit records. No force termination was used. These bounded checks pass the 10-second crash gate for this artifact, not long-session stability or proof that the historical exit exception is fixed.

Injected gameplay keys were inconsistent, including stock I/Tab and a temporary mouse binding; title/menu input and key registration worked. A temporary Inventory-to-middle-mouse binding allowed autonomous pause access during run 1. Inventory was restored to I/Tab, and Guidance Trail to middle mouse through the game UI. Stock video preferences were restored from the verified pre-install copy after shutdown. The verified PR7 installation remains installed, with Native and HDR Off. No account/character save was restored or published. Input automation trouble is a qualification limitation, not diagnosed as a new mod defect or bypassed by changing ReShade input policy.

## Checks, changes and remaining gates

Local Windows checks passed: 39 guided installer checks, eight legacy installer tests, six build-recipe tests; native framing (2502), settings semantics (2312), adapter/deadline (8), Reflex-token (31) and display-protocol (14) checks. Both Linux and Windows jobs passed for the tested source in [portable CI run 37284816601](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37284816601). Both candidate targets built in run 37284816608. [Linux runtime evidence](UPDATE_2_LINUX_VALIDATION_2026-10-05.md) remains separately scoped.

| Change in this Windows follow-up | Why | Windows quirk / preservation rule |
| --- | --- | --- |
| `tests/windows_runtime_probe.ps1`: record `displayLatencyAddonLoaded`; opt-in `-RequireDisplayLatency` fails when absent | PR7 adds a second independent addon; SR-only survival could miss its failure to load | Windows survival and SR module presence do not prove HDR/Reflex addon loading. Keep the Update 2 check and legacy SR-only support. This changes evidence, not rendering. |
| This report, aggregate record, compatibility note and gate/index links | Tie claims to the downloaded installer; preserve partial outcomes and limits across Linux refactors | Linux/NVIDIA success cannot qualify this Windows/AMD artifact. Keep exact hashes, current acknowledgements, actual exits and historical records. Do not transfer qualification to rebuilt binaries. |

No native addon, installer behavior, manifest, runtime or build recipe was changed to guess at a defect. Required checks still **unrun/unqualified**: Windows NVIDIA actual Reflex Off/On/Boost, PCL/timing/performance and failure lifecycle; HDR On/calibration with an HDR-capable display; physical controller and complete keyboard navigation; independently confirmed movement/combat; GUI Browse folder workflow, fresh Install/Uninstall button flows and a fresh official full-ReShade installer run (existing full ReShade was used; normal/full distinction passed file-safety fixtures). Intel runtime, broad device-loss and long-session shutdown coverage are also unrun. These limits blocked the original complete-qualification gate; AMD fallback does not qualify those paths.

## Publication decision — 5 October 2026

After this report, the maintainer stated that AMD Windows testing was complete to the available extent and explicitly requested GitHub/Nexus publication. `0.2.0-rc.1` is therefore published as an experimental prerelease using the exact qualified installers, with the outstanding checks above disclosed. This changes publication authorization, not qualification outcomes; Windows NVIDIA and HDR output are not marked passed. See [artifact provenance](../qualification/update2/release-artifacts.json).
