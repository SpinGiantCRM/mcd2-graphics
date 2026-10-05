# Windows fixes: reasons and preservation requirements

These changes were tested on Windows 11 / Radeon 860M on 5 October 2026.
The original addon crashed at approximately 29 seconds; the fixed candidate
completed gameplay and normal shutdown after nearly 13 minutes. Exact evidence
and limits are in [Windows validation](WINDOWS_VALIDATION_2026-10-05.md).

Each row identifies what changed, why, the relevant Windows behavior, and what
must survive a Linux refactor. Some changes address general compatibility or
release provenance rather than a defect proven exclusive to Windows.

| Change / files | Why and relevant Windows behavior | Preserve / regression evidence |
| --- | --- | --- |
| Heap path buffer in `src/native/ui_control_present.hpp::ui_poll_intent` | The 32,768-element Windows `wchar_t` array reserves 64 KiB on every callback entry, even if its initialization branch is skipped. The original frame was 65,960 bytes; Windows faulted in `___chkstk_ms` with `0xc00000fd`. The game's render-thread stack could not accommodate it. | Keep large buffers off this stack and retain failed/truncated-path checks. Native compilation rejects frames above 16 KiB; original source fails the guard. Bounded Windows startup is the runtime check. |
| Heap module-path buffer in `src/native/observer.cpp::DllMain` | A second 64 KiB path buffer occupied the Windows DLL-loading thread's stack. Original compilation reports a 65,736-byte frame here. The observed fault identifies the stack probe, not an independently proven second `DllMain` crash. | Keep this buffer on the heap with path-length checks. Preserve the native stack-frame guard. |
| Lowercase `<windows.h>` / removal of generated `Windows.h` shim in `observer.cpp` and `build.py` | The shim includes `windows.h`; Windows' case-insensitive filesystem treats these names as the same file, causing recursive inclusion. Linux's case-sensitive filesystem concealed it. | Include the platform header directly; do not restore a case-only alias shim. Qualified Windows native compilation must still pass. |
| `_fltused` in `src/native/ngx_parameter_bridge.cpp` | MSVC-target Clang emits this floating-point CRT marker for the NGX float setter. The Windows LLVM MinGW static-CRT link needed its definition. This is a toolchain/ABI combination issue, not a claim that every Windows compiler needs the workaround. | Keep the qualified Windows link working and retain both MSVC ABI bridges. A Linux linker succeeding is insufficient reason to remove the marker; alternatives must pass both toolchains. |
| `--native-only`, explicit compiler paths and frame guard in `build.py`; recipe in `docs/BUILD.md` | The Windows host needed explicit LLVM MinGW executable paths and an addon-only build retaining qualified UI/shader payloads. Linux compiler commands and mandatory UI tooling prevented that workflow. The 16 KiB limit prevents large stack frames returning. | Preserve full Linux builds and the Windows native-only path. Native-only must not claim it rebuilt/qualified the UI or shader. Retain the frame guard on the main addon. |
| CSV process guard in `install.py::require_closed`; `tests/test_installer_process.py` | Windows `tasklist` truncates the full executable name in its default table, allowing the original installer to miss a running game. | Retain `/FO CSV /NH`, exact case-insensitive image-field comparison and failure when the query fails. Four tests protect these rules; a real running-game CLI attempt was refused. Keep the Linux `/proc` path. |
| `shutil.copyfile` and optional result output in `test_install.py` | Linux `cp --reflink=auto` is absent on Windows and caused `WinError 2`. The output option preserves archived Linux qualification records when collecting Windows results. | Keep fixture copying portable, with no Unix command dependency. Seven real-reference filesystem gates pass. Do not overwrite historical Linux evidence with Windows results. |
| Resolved fixture roots in `test_install.py` and `tests/test_installer_files.py` | GitHub's Windows runner uses shortened 8.3 temporary paths. Comparing an unresolved alias with resolved full paths falsely reports an unsafe package path. The actual CLI already resolves its root. | Match the CLI's resolved-root contract; do not weaken containment checks to pass tests. Windows CI exercises the runner's path variant. |
| Manifest-derived install-marker version in `install.py`; transaction tests in `tests/test_installer_files.py` | The hard-coded preview.1 marker misidentified the rebuilt Windows candidate as the original binary. This is provenance, not an OS quirk. | Record the manifest version. Tests use a deliberately different version; real installation matched the candidate. Preserve duplicate refusal, modified-file retention and dependency preservation tests. |
| Rendering-adapter check in `ui_control_present.hpp`; policy in `src/native/upscaler_support.hpp` | AMD Quality remained Applying for over a minute with reduced source scale because matching render observations never triggered NGX/fallback. D3D12's device LUID identifies the actual rendering adapter; a laptop's display/default GPU is not a reliable substitute. The defect was observed on Windows/AMD and is not proven Windows-only. | Decline AMD, Intel, software and unidentified adapters BEFORE reducing source resolution. NVIDIA identity only permits proceeding to existing NGX initialization/capability checks. Vendor tests and Windows Quality/DLAA fallback UI/runtime checks protect this behavior. |
| 15-second pending-source deadline in `ui_control_present.hpp` / `upscaler_support.hpp`; `tests/upscaler_support_test.cpp` | Missing source acknowledgements or render observations could leave a request pending indefinitely. This general asynchronous-renderer safeguard was motivated by the Windows failure. | Keep a bounded fallback path for DLSS waits. Tests cover 15,000 / 15,001 ms boundaries. This timeout has not been deliberately induced in NVIDIA runtime; Linux testing should exercise it where possible. |
| Windows installer CI alongside Linux portable CI in `.github/workflows/portable-checks.yml` | Linux-only tests miss Windows process-output/path behavior; Windows tests do not establish Linux/NVIDIA runtime compatibility. | Retain both jobs and installer, settings/runtime and adapter/deadline checks. Both jobs passed for this candidate. CI does not substitute for gameplay/DLSS/shutdown qualification. |
| `tests/windows_runtime_probe.ps1` / `tests/windows_mod_state.cpp` | Process survival could pass with the addon absent. Windows probes verify addon loading, survival/responsiveness and requested/current acknowledgement revisions. | Tie evidence to loaded MCD2/RenoDX and the current request. Read only the mod's two state slots; keep account/character saves private. These evidence tools do not change rendering behavior. |
| Candidate `manifest.json` and package; `README.md`, `INSTALL.md`, `docs/VALIDATION.md`, `docs/RELEASE_NOTES.md`, Windows report | The addon changed while original UI/shader hashes stayed frozen. Installation must identify the exact tested binary. Users also need the unsupported-GPU message, ReShade first-run keyboard caveat and outstanding NVIDIA qualification. This is traceability and documentation, not a separate OS defect. | Keep the original preview.1 tag/assets frozen. Publish the candidate separately, verify payload hashes, preserve external dependencies, and record platform/artifact-specific qualification rather than implying NVIDIA success from AMD fallback. |

## Linux regression handoff

Use this candidate source and its matching release ZIP. GitHub's automatic
source archive omits the ignored `package/` payload. Check install/removal and
save preservation, startup beyond the original crash window, gameplay, NVIDIA
Quality/DLAA and Native transitions, main-menu travel and normal shutdown.
Verify the LUID check admits the actual NVIDIA rendering adapter and NGX still
decides feature support. Preserve the Windows requirements above while fixing
Linux regressions. A rebuilt artifact needs its own hash and qualification;
Windows results for this binary do not qualify a different Linux-hosted build.

### Build recipe follow-up

The local Linux regression candidate selects compiler-specific frame-error
flags and normalizes the Windows include in generated ReShade header copies.
This retains the Clang guard and avoids restoring the recursive Windows.h
shim. Both Windows and Linux CI retain installer checks and now include the
build-recipe unit checks. Both CI jobs passed for PR6's tested source.
The GCC-produced addon preserves all native PR5 source fixes. Its
[Windows AMD regression](WINDOWS_REGRESSION_PR6_2026-10-05.md) passed installation,
1,027.5 seconds of survival, fallback, menu travel and reinstall.
The first quit eventually returned exit code 0 after a roughly two-minute delay;
the second quit crashed with `0xc0000005`. Windows shutdown is not qualified.
A third PR6 run and one previous-candidate control exited with code 0; these
clean runs do not erase the crash or establish its cause.
Windows NVIDIA runtime and the separately recorded Linux exit limitation remain
outstanding. The prior LLVM candidate's
results and this AMD result cannot qualify NVIDIA teardown in another binary.

## Preview.2 publication decision — 5 October 2026

The later clean Steam / CachyOS Proton SLR / native WineWayland run passed two normal shutdowns; see [the latest Linux report](LINUX_CACHYOS_REGRESSION_2026-10-05.md). This qualifies that Linux environment, rather than erasing the older environment's lingering-process observation.

Preview.2 retains the exact tested GCC addon, UI and shader. The maintainer elected to publish with the single unexplained Windows `0xc0000005` exit disclosed and further Windows isolation deferred to Reflex qualification. Two clean candidate exits do not establish the crash's cause. Windows NVIDIA remains untested. No native changes were made to guess at a fix.

## Update 2 / PR7 Windows follow-up

The [PR7 Windows report](UPDATE_2_WINDOWS_VALIDATION_2026-10-05.md) records the exact downloaded installer, real repair/removal/fresh install, AMD Quality/DLAA/Custom fallback before source reduction, native HDR Off/restart and three clean exits. It retains the historical exit exception and outstanding NVIDIA/HDR/input gates. No rendering or installer behavior changed during this follow-up.

The runtime probe now records the independently loaded display/latency addon and accepts `-RequireDisplayLatency` for Update 2. Keep this opt-in check: on Windows, a live process with only the SR addon loaded cannot establish HDR/Reflex startup. Legacy SR-only checks remain usable. This is an evidence correction, not an OS-specific rendering fix. Preserve the candidate-specific aggregate record and both Linux/Windows CI jobs; do not apply these results to a rebuilt installer.
