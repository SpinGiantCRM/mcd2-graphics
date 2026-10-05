# Linux regression candidate and Windows qualification

Status: PR6 testing candidate, not a public release. Exact-artifact Windows AMD
checks are recorded in [Windows regression results](WINDOWS_REGRESSION_PR6_2026-10-05.md)
and its [sanitized record](windows-regression-pr6-2026-10-05.json). Installation,
startup, fallback, travel and reinstall passed. The first Windows exit returned
0 after a roughly two-minute delay; the second quit crashed with `0xc0000005`,
so Windows shutdown is not qualified. A third PR6 run and one previous-candidate
control exited 0 without establishing the crash's cause. The exact PR6 payload
remains installed with Native saved. Windows NVIDIA teardown remains untested,
and the Linux exit limitation below is unchanged. Do not merge or promote this
candidate without the user's release decision. Original `0.1.0-preview.1`
remains frozen.

## What was repaired

The PR5 Windows-produced addon loaded and evaluated Quality DLSS on the Linux
RTX 4080 SUPER, but crashed inside NGX's device shutdown on the first
Quality-to-DLAA transition. GPU completion, feature release and parameter
destruction succeeded; device shutdown did not return. The frozen original
completed both directions of that transition on the same host.

Rebuilding the merged PR5 native source with the Linux GCC toolchain restored
the switch. No NGX device argument, teardown ordering, renderer source, UI or
conversion shader was changed. This is a qualified replacement artifact for
the observed Linux regression, not a proven explanation of the LLVM MinGW
artifact's failure. Do not infer that PR5's heap-buffer or AMD fixes caused it.

Two build-recipe defects blocked that rebuild: GCC rejected the Clang-specific
frame-error flag, and the upstream ReShade include still used uppercase
Windows.h on a case-sensitive host. The recipe now handles both compilers
while enforcing the same 16 KiB limit, and normalizes only the copied header.
Six build-recipe unit tests cover the selection, refusal and normalization.

## Artifact and scope

- Base source: merged PR5, `2cc97532c1cf5f359b6d803e7e7ef1949b3aad2d`.
- Candidate: `0.1.0-preview.1-linux-regression.1`.
- Addon SHA-256: `611b49abee32db2705d47e5a9e08dd676cd7e892cb7da7e563eab4c18e733240`.
- Main compiler: MinGW GCC 16.2.0; MSVC ABI bridges: Clang 23.1.1.
- SDK pins and dependencies remain in `dependencies.lock.json` / `BUILD.md`.
- UI and conversion shader hashes match the frozen preview.1 payloads.
- Windows heap buffers, `_fltused`, both ABI bridges, adapter LUID policy,
  pending timeout, portable installer and both CI jobs are retained.

## Linux runtime observations

The first control session survived more than 13 minutes including title/login,
Quality gameplay, Quality → DLAA → Quality → Native → Quality, world → main
menu → world, and mouse movement. Each tested mode reached Applied with the
current revision and matching session. Four completed feature generations
returned successful GPU completion, NGX release/destruction/shutdown and owned
resource release. The reentered world created and evaluated Quality again.
The exact binary above was installed; this was not a DLL-absent survival test.

Normal exit is **not qualified**: Save and quit closed the window, but the
Shipping process lingered and required termination. The frozen original also
showed a lingering process on this host. The control's final active generation
did not emit its cleanup completion. A private debugger snapshot showed the
game thread yielding but did not establish the cause. Do not call this a
clean shutdown pass or blame the addon/Proton without further isolation.

This is a smoke test, not a long-session, complete dungeon/lifecycle, Windows
or physical-controller qualification. Existing HDR dependencies were retained;
no fresh HDR calibration or benchmark was performed.

## Windows checklist and remaining NVIDIA checks

The AMD run exercised the checklist below with the supplied payload; the report
distinguishes completed checks from unmeasured source dimensions, continuous
active play, controller use and prompt shutdown. Keep these requirements for
future candidates.

Test the supplied exact binary before rebuilding it:

1. Close the game and remove the previous mod using its matching installer.
   Install this candidate with existing pinned dependencies and external DLSS
   runtime. Check the marker records the candidate version and addon hash.
2. Confirm the mod is loaded and survive startup beyond the original 29-second
   stack-overflow window. Play for at least 15 minutes.
3. On AMD, select DLAA and Quality: confirm a bounded Unsupported fallback,
   native source resolution, current request acknowledgement, and usable UI.
   Native should apply normally. Do not mistake fallback for working DLSS.
4. Exercise pause/main-menu Video pages, linked preset/render-scale controls,
   world → menu → world, and physical controller if available.
5. Save and quit; confirm the Shipping process actually exits normally.
6. Uninstall/reinstall; preserve character saves, config, HDR/loader/ReShade
   dependencies and unrelated mods. Refuse uninstall while running and retain
   modified owned files.
7. Record OS/GPU/driver, exact package/addon hashes, current revisions and
   results. No raw account data, saves or authentication logs in public reports.

AMD results cannot qualify NVIDIA NGX teardown. A Windows RTX host should also
repeat Quality ↔ DLAA, DLSS ↔ Native, travel and exit. An LLVM MinGW rebuild
needs those checks independently; do not replace the tested payload by name.

All private saves, SDK headers, vendor runtime DLLs, screenshots, debugger
output, logs and authentication material are excluded from this handoff.

## Updated Steam / native Wayland follow-up

The unchanged candidate passed Quality/DLAA/Native transitions and normal
shutdown on CachyOS Proton SLR using the updated game's Steam sign-in path.
See [Linux follow-up](LINUX_CACHYOS_REGRESSION_2026-10-05.md). The Windows
access violation remains a release blocker; use the
[isolation plan](WINDOWS_SHUTDOWN_ISOLATION_2026-10-05.md) on the exact
existing artifact before promotion.
