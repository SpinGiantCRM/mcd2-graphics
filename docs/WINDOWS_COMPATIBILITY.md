# Windows fixes: reasons and preservation requirements

For the held FG candidate, follow the pinned
[Windows qualification checklist](WINDOWS_QUALIFICATION_CHECKLIST.md) before
claiming public-installer or Loader 2.3 qualification. It separates the exact
installer payload from the separately compiled developer trial.

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

## FG candidate and transparent installer follow-up — 6 October 2026

Command-list Reset is a pre-call notification and may fail. The FG candidate
retains old recording references until a later recording command establishes a
replacement, including barrier-only and root-state lists. A cancelled, pending
SR evaluation clears its CPU sentinel only at that proven replacement. GPU
retirement still requires the existing post-invalidation signal and completed
fence. The stack guard and adapter/deadline policy remain unchanged.

The guided installer now reads a visible, expanded `payload/` folder, verifies
all expected hashes before any game write, and preserves the existing ownership,
repair, rollback and dependency rules. Both platforms use self-contained folder
deployment. Extract the entire package; launching a lone copied executable is
unsupported. Legacy stream callers remain available for compatibility tests,
but the GUI has no embedded payload archive. Windows CI and real Windows
fresh-install, repair and removal are required for this new package layout.
Neither Linux measurements nor older installer qualification qualify it.

## Shared NGX ownership — FG candidate v31

SR preset retirement releases only its own feature and parameters when the
versioned FG bridge verifies that Streamline owns the same wrapper device's NGX
instance. Streamline performs final NGX shutdown after SR retirement. Missing
or mismatched ownership confirmation retains the generation rather than shutting
down a potentially shared instance. Without an initialized FG bridge, SR keeps
its existing device shutdown path.

Preserve this distinction across preset changes, Native transitions, temporary
menus and normal exit. FG SDK status 0 and two reported presents are insufficient
qualification: also inspect Streamline's internal NGX evaluation errors. Windows
NVIDIA must verify this shared path; AMD/Intel fallback exercises the unshared
path. Linux runtime measurements do not qualify either Windows path.

### Building the FG trial on Windows

The `FG Windows candidate` CI job builds the current bridge, bootstrap, guide
observer, latency addon and SR addon on Windows. Windows FG needs the Microsoft
C++ ABI and CRT headers/libraries; the LLVM MinGW SR toolchain alone cannot build
the Streamline bridge. A private junction-based sysroot adapts Visual Studio's
installed layout to the existing recipe without changing the installed tools.
The pinned ReShade PR435 source is built separately with full addon support.

The test artifact contains binary hashes and source provenance, with runtime
qualification explicitly false. Streamline runtime DLLs are acquired and checked
separately; they are excluded from the artifact. The historical UI is reused
only when its receipt matches the current git source blob (Windows checkout
line endings must not invalidate that comparison). Do not replace this check
with a Linux build pass or apply the result to the historical v27 archive.
Both existing Windows and Linux CI jobs remain required.

The Windows runner builds the upstream ReShade solution because its includes
depend on `SolutionDir`. SDK discovery must ignore unversioned Windows Kits
components such as `wdf` and incomplete version folders; retain the two
`test_windows_build.py` fixtures. Microsoft ABI/sysroot adaptation must remain
isolated from installed tools and from the Linux build path.

### Windows / AMD FG fallback check — 6 October 2026

The [exact-binary Windows report](FG_WINDOWS_AMD_VALIDATION_2026-10-06.md) records
two clean exits and unsupported fallback with FG Off and a persisted FG On
preference. Preserve the opt-in runtime probe's six loaded game-directory module
hashes: a live process or system DXGI alone cannot establish FG-chain startup.
The actual rendering adapter must decline FG before device binding/swapchain
routing and decline DLSS before source reduction; unsupported fallback must not
apply the foliage lease. No native rendering fix was needed in this trial.

The reversible transaction restored the prior installation and preferences.
These results qualify only the recorded Windows/AMD binaries and checks, not
NVIDIA HDR/active FG, shared NGX teardown, a rebuilt artifact or the expanded GUI
installer. The historical shutdown exception remains unresolved.

### Successful Reset confirmation — 7 October 2026

A Linux FG-Off Quality-to-DLAA transition reproduced a stuck request with a
dormant reset command list retaining one old SR recording. The framework's Reset
event precedes the native call, so the event alone cannot release that recording.
The opt-in patch now advances a private command-list epoch only after native
Reset succeeds; SR confirms the changed epoch before invalidating the old
recording. Resource release still waits for a subsequent signal and completed
GPU fence. Failed Reset, unknown metadata and missing proof retain the lease.

Keep the existing pre-call event contract, recording-operation alternative,
unsupported-adapter policy and frame guards. The Windows recipe builds the same
pinned PR435 base plus the recorded patch, verifies no other source modification,
and reports the checkout as patched rather than clean. This is a new framework
and SR binary pair: older Windows results do not qualify it. The separate
render-thread hang has not been established as the same defect. See the
[qualification handoff](../experiments/fg-streamline/RESET_CONFIRMATION_2026-10-07.md).

## Dependency overrides and Loader 2.2 candidate

Overrides relax only external dependency version pins after explicit selection.
Keep supported executable checks, own-payload hashes, required archive members,
link/traversal/size rejection, changed-file refusal and receipt ownership checks.
The private DLSS DLL's actual selected hash is recorded for safe removal; shared
dependencies remain shared. An override receipt must never authorize an arbitrary
project file or game executable. Overrides start disabled on every GUI launch.

The optional DLSS model hint reads a Unicode-path INI only when creating a
feature; absent, invalid or out-of-range values preserve the runtime default.
No extra per-frame work is added. This native change and the new installer UI
require Windows regression. Prior AMD qualification does not qualify them or
Blueprint Loader 2.2.

### Held dependency and Reflex candidate — 7 October 2026

The candidate pins Blueprint Loader 2.2 and adds explicit per-dependency
experimentation for Loader, ReShade, RenoDX, DLSS and Streamline. Preserve
default strict hashes, game validation, private payload integrity, extraction
bounds, required-file completeness, closed-game checks and ownership checks.
An acknowledgement permits only the selected external dependency hashes.

A restart-only DLSS model hint defaults to zero; absent/invalid input leaves
NGX preset hints unset. Test the default and an explicit supported hint on
Windows NVIDIA, plus unsupported-adapter fallback on AMD/Intel.

The NVIDIA Wine bootstrap disables dynamic Vulkan swapchain-mode switching
before device creation to stabilize the tested Reflex/VSync-Off combination.
Windows must skip this path before querying NVAPI or setting environment
variables. Existing extension overrides are preserved; an explicit opt-out is
available. Recheck Windows startup/exit and normal Reflex modes with the rebuilt
binaries. The prior Windows qualification does not cover these new artifacts
or Loader 2.2. Release remains held.

### Exact rebuilt Windows / AMD check — 7 October 2026

The [new exact-binary report](FG_WINDOWS_AMD_VALIDATION_2026-10-07.md) qualifies
the Windows artifact built from `1389ceb` for the reversible FG trial and
unsupported AMD fallback with Loader 2.2. Six actual loaded module hashes matched
in both gameplay runs. Saved Reflex On/Boost and FG On stayed inactive without
faults; DLSS Quality/Custom requests were declined before source reduction.
Native Windows recorded no Wine Reflex pacing event. Both processes exited 0,
although FG-Off Save and quit had an unexplained delayed process exit.

Keep the report's change/quirk mapping, source and artifact receipts, and both CI
platforms when refactoring. This fresh Windows framework/SR pair is separate from
the older Windows and cross-built release pairs. AMD did not allocate an active
SR/FG feature, so the test does not qualify GPU retirement or active NVIDIA Reflex.
The public GUI runtime flows and historical hang/shutdown exception remain open.
Unsupported FG/Reflex rows currently disappear while reconstruction stays visible;
the observed UI inconsistency is recorded, not changed by this qualification.

## Dependency policy follow-up — 7 October 2026

The installer now separates minimum APIs, complete recognized release sets and
explicit untested-version overrides. Blueprint Loader requires 2.0+; the mod
uses no 2.2 popup API. Official 2.0, 2.2 and 2.3 archives and installed sets are
recognized without forced upgrades. DLSS and Streamline use the current tested
runtime/API baselines; ReShade and RenoDX use interface/configuration requirements.
All five dependencies retain an explicit override. A higher version does not
prove the required patched ReShade interfaces exist.

Preserve whole-set matching, strict own-payload/game validation, dependency
recovery, archive safety and receipt ownership. Regression fixtures exercise the
shared policy on every dependency, including alternate private DLSS removal.
These installer and documentation changes do not change native graphics or the
UI payload. The previous Windows gameplay result still used Loader 2.2; it does
not qualify Loader 2.3 or this rebuilt public GUI installer. Release remains held.

The mod has no runtime restriction forcing offline mode. The installer now says
online compatibility is unverified rather than directing offline-only use.

The subsequent [pinned Windows installer attempt](WINDOWS_INSTALLER_PREREQUISITES_2026-10-07.md)
verified the specified installer, all 12 payload hashes and Loader 2.3 archive,
but stopped before installation because the exact separate framework archive
was unavailable. This is a distribution blocker, not a runtime pass or a Windows
rendering fix. Preserve the pinned dependency requirement: the differently hashed
Windows developer framework cannot qualify the public installer by substitution.
The report records every checklist gate and leaves prior runtime results separate.

The exact separate framework was subsequently made available and the
[public-installer qualification](WINDOWS_INSTALLER_QUALIFICATION_2026-10-07.md)
supersedes that prerequisite blocker without deleting its history. The original
pinned package passed installation, repair/removal/reinstall, six-module startup
and bounded AMD fallback checks with Loader 2.3. Two normal window closes
returned `0xc0000005`; one later original and two metadata-package closes
returned 0. Overall qualification is **FAIL**, with omitted menu/transition and
fixture checks recorded. No rendering fix or causal diagnosis was made.

Preserve the process-handle exit measurement: on Windows, a disappearing game
window or task-list entry cannot establish shipping exit code 0. Hold the actual
shipping handle before normal close and keep launcher results separate. Clean
Linux/NVIDIA exits or a differently hashed framework/native pair do not erase
these failed public-package exits. The metadata package corrected its displayed
version and passed bounded main-menu scale/persistence checks; its three changed
UI hashes and repeated checks are recorded separately. Raw logs, player/account
data and third-party runtime DLLs remain private; the original installation and
mod/video preferences were restored and hash-verified after normal GUI removal.

The metadata-provenance regression must read canonical `git show HEAD:` source
bytes, as `copy_current_ui` does. Its original test mock read working-tree files;
Windows CRLF checkout conversion falsely rejected the unchanged UI receipt.
The regression now exercises the real Git read while retaining stale-source
refusal. Do not normalize or weaken the production receipt check to satisfy a
checkout-dependent test. This changes test evidence only, not shipped payloads.

A Linux Loader 2.3 smoke check reached the main-menu Mods page (reported version
2.3), mod metadata, Video controls and offline gameplay. Current-session DLAA at
100% and FG On acknowledged successfully; SR error, Reflex fault and coordinator
error were zero. Native graphics/UI payload hashes were unchanged. This was a
short compatibility smoke check, not a new benchmark or Windows 2.3 test.
