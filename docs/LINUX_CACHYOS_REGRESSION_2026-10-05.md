# Linux NVIDIA regression on the updated Steam launch path

Status: Linux gameplay, reconstruction transitions and two bounded normal
shutdowns passed. Publication remains blocked by the separately recorded
Windows shutdown access violation. No native source or release payload changed
for this run.

## Exact artifact and environment

- Candidate: `0.1.0-preview.1-linux-regression.1` from PR6.
- Addon SHA-256: `611b49abee32db2705d47e5a9e08dd676cd7e892cb7da7e563eab4c18e733240`.
- Test ZIP SHA-256: `6ec967e049341daaf94b3e70b47deec27ed1cf2d75a9164f0749d057fb66542b`.
- Game Steam BuildID: `25647713`; files verified through Steam before launch.
- CachyOS Proton SLR: `1791146054 cachyos-11.0-20261005-slr`.
- NVIDIA RTX 4080 SUPER; KDE Wayland; 3840 x 2160 output.
- Pinned ReShade full addon build, RenoDX UE Extended, Blueprint Loader and
  externally supplied DLSS runtime retained.

Legacy local compatibility files were backed up privately and removed before
Steam verification. The test used the updated game's normal Steam sign-in
path. No custom authentication runtime was loaded and no Gamescope wrapper
was used. Authentication files, credentials and character saves were not
edited for the migration.

The saved options were `PROTON_ENABLE_WAYLAND=1 PROTON_ENABLE_HDR=1 %command%`.
WineWayland was actually loaded. CachyOS has retired `PROTON_ENABLE_HDR`, so
that variable did not enable HDR; this run used automatic HDR detection.
The current explicit equivalent is `PROTON_ENABLE_WAYLAND=1 DXVK_HDR=1
%command%`, per the [CachyOS documentation](https://github.com/CachyOS/proton-cachyos/blob/cachyos_main/README.md).
That exact replacement line was not the saved line tested in this session.

## Runtime results

| Check | Evidence / result |
| --- | --- |
| Startup and sign-in | Reached main menu, then hub gameplay without the old local compatibility runtime. MCD2 and RenoDX addons both loaded. |
| Quality | Current intent/source/runtime revision 1516, Applied / Error 0; NGX input 2560 x 1440, output 3840 x 2160. |
| Quality to DLAA | Revision 1517, Applied / Error 0; input/output both 3840 x 2160. |
| DLAA to Quality | Revision 1518, Applied / Error 0. |
| Quality to Native | Revision 1519, matching current source/runtime acknowledgements, Applied / Error 0. |
| Native to Quality | Revision 1520, Applied / Error 0. |
| World to main menu to world | Frontend remained Native while retaining Quality preference; returning world revision 1527 applied Quality again. |
| Feature retirement | Five generations recorded completed GPU fences, successful ReleaseFeature, DestroyParameters and Shutdown1, then owned-object release. Final queue teardown recorded completed cleanup without retaining the generation. |
| Normal Save and Quit | Process absent within a 6.6-second observation bound after sending the actual Space confirmation; no forced termination. ReShade logged Finished exiting. Steam recorded launcher exit 0, but Shipping's numeric exit code was unavailable (-1 tracking sentinel); do not describe that sentinel as a crash code or a proven Shipping exit 0. |
| Native HDR output path | R10G10B10A2 swapchain and DXGI color space 12 (HDR10 PQ / BT.2020), with the WineWayland driver loaded. This establishes the requested HDR output path, not physical luminance, calibration or content fidelity. |

The first session lasted approximately 23 minutes including frontend, loading,
menus and stationary gameplay. It was not 23 minutes of continuous active play.
All five sampled NGX generations recorded zero evaluation/contract failures;
transient input fallback recovered. This does not establish uninterrupted DLSS
on every frame, all dungeon transitions or long-session stability.

Initial Enter-only quit attempts left the confirmation open and are discarded
as shutdown timing evidence. No process was killed in this session. Desktop
screenshots are ordinary tonemapped previews and were not decoded as PQ values.
ReShade's device-reference warning remains a diagnostic observation; this run
does not establish its cause or harmlessness on other platforms.

## Installer and portable checks

- Seven installer unit tests and six build-recipe tests passed.
- Framing rejected 2,502 truncations; semantic/runtime checks passed 2,312 cases.
- Unsupported-adapter and pending-source-deadline checks passed.
- Seven filesystem fixture gates passed, including dependency refusal,
  duplicate refusal, modified-file retention, uninstall/reinstall and containment.
- Actual uninstall/reinstall preserved 77 protected files byte for byte,
  including saves, configuration, dependencies and unrelated mods.
- All six installed mod/runtime hashes matched the exact candidate afterward.

After the actual reinstall, a second Steam launch reached the main menu and
hub again. The retained Quality preference applied at revision 1532 with
matching current source/runtime acknowledgements. Save and Quit completed
within an 8.9-second observation bound without forced termination; that
generation also recorded completed GPU/NGX cleanup and final queue teardown.
ReShade logged Finished exiting. This was a short restart/exit check, not a
second long-session test.

The original preview.1 tag and archive remain unchanged. No raw logs, saves,
account information, personal paths, debugger output or vendor binaries are
included in these results.

## Remaining release gate

The Windows report contains two normal PR6 exits and one `0xc0000005` exit.
Linux success does not erase that result. The fault was not attributed to the
addon, and AMD did not create an NGX feature. Do not describe it as an NGX
shutdown defect without additional evidence. Use the
[Windows shutdown isolation plan](WINDOWS_SHUTDOWN_ISOLATION_2026-10-05.md)
before merging/promoting this candidate.
