# Windows fixes: reasons and preservation requirements

The opt-in [continuous FSR controller](FSR_CONTINUOUS_2026-10-09.md) preserves the
MSVC bridge, native stack guard and independent recording/fence ownership. Its
context and runtime-response ABI sizes are 288 and 136 bytes. Retain codec,
source-acknowledgement, actor-travel and corruption checks on both CI platforms.
Dependency hashes are checked before loading; unsupported or invalid inputs
cannot lower source resolution without successful preflight. Linux/NVIDIA FSR
execution does not qualify Windows/AMD runtime, UI parity or AMD FG.

The [shared SR guide producer](SR_GUIDES_2026-10-08.md) preserves the existing
conversion and NGX parameter/output path. Each consumer owns independent
descriptor/recording leases and retirement fences; one consumer's reset or
release must not invalidate another. Keep the two-consumer fixture on both CI
platforms, the native stack guard and diagnostics disabled in ordinary builds.
The bounded Native/Quality/DLAA checks are Linux evidence; Windows runtime checks
remain required before including this changed native binary in a release.

The opt-in [Anti-Lag integration](AMD_ANTILAG2_2026-10-08.md) uses the official SDK
through a Microsoft ABI bridge. Preserve actual adapter selection, native
Windows/loaded-driver gates, exclusive timing ownership, source pins, frame guard
and callback shutdown barrier. Synthetic SDK tests and Linux/NVIDIA fallback are
not active Windows/AMD qualification. Never copy the fake amdxc64.dll into an
artifact or game. Existing NVIDIA checks and both CI platforms remain required.

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

## Shutdown isolation follow-up — 7 October 2026

The [shutdown isolation report](WINDOWS_SHUTDOWN_ISOLATION_2026-10-07.md)
reproduced the same access violation with the exact pinned framework alone,
without MCD2 addons, FG/Reflex plugins, RenoDX or Loader. The unmodified control
exited cleanly but also showed a delayed process exit. This narrows the failure;
it does not prove a precise framework defect or establish a fix. Keep this
control evidence separate from full installer and mod/framework qualification.

Do not remove valid AMD PCL markers merely because Reflex low latency is
unavailable: statistics and low-latency support have different vendor gates.
The same-source MSVC baseline exited cleanly without the investigated upstream
descriptor-heap lifetime patch, so a clean patched build alone cannot prove a
causal repair. Keep exact binary, toolchain and source provenance, successful
Reset metadata, shipping process-handle measurements, and both CI platforms.
Exclude sleep/hibernation-interrupted wall-clock timings from normal shutdown
comparisons. The original public qualification remains FAIL.

The full-memory follow-up in that report preserves the original exception and
confirms the same fault in a sixth event, with GameThread waiting in the game's
C runtime exit callbacks. An attached debugger instead caused a distinct exit
code 1000 before normal close. Keep these observations separate: the diagnostic
capture addon does not repair the fault, and neither the nearest exported
symbol nor an explicitly anchored unwind names its cause. Windows dump-request
structures use the SDK's four-byte packing; verify exception-context preservation
in a harness if replacing this evidence method. Restore the hash-guarded test
installation after the actual shipping exit and keep raw memory private.

The separate full-package MSVC A follow-up in the same report records two clean
normal Alt+F4 exits after more than five minutes of observed hub gameplay each,
with Loader 2.3 and matching AMD Quality/FG On/Reflex Boost fallback. It retains
the successful-Reset patch without B's heap fix. Preserve exact loaded module
hashes, closed-game fixtures and the distinction between private runtime staging
and public installer qualification. Compiler, recipe and flags changed together;
these clean intermittent observations do not establish a causal crash repair or
replace the original FAIL. Snapshot all staging inputs before removing older
receipt-owned files, which can also include shared runtime sources.

## MSVC framework mitigation candidate — 7 October 2026

The maintainer resumed testing after the historical investigation stop. The
[new mitigation record](WINDOWS_MSVC_MITIGATION_2026-10-07.md) preserves that
history and qualifies changed inputs separately. Another matched full-package
cross-build control reproduced the same shutdown AV; the same-source MSVC A
comparison exited 0, with the earlier clean A observations retained. This
supports selecting the tested build as a bounded mitigation, not naming a
compiler defect or retroactively changing the original FAIL.

| Change | Why | Windows requirement to preserve |
| --- | --- | --- |
| Pin MSVC A's DLL and a separately named dependency ZIP in `dependencies.lock.json`. | The prior exact cross-built framework repeatedly reproduced the shutdown AV, including without our mod. | Framework source/version alone does not identify the runtime build; verify actual loaded hashes and held shipping exits. Keep PR435 and successful-Reset metadata. |
| Publish sanitized MSVC toolchain, eleven submodule pins and the exact approved source diff beside the separate dependency. | A clean different build must be independently reproducible and reviewable. | Compiler, recipe and flags changed together; do not attribute the result to the investigated heap patch, which A does not contain. Rebuilds need new hashes and qualification. |
| Package the new ZIP with stored members and explicit LF byte inputs. | Deflate implementations produced different compressed bytes for identical historical inputs; Windows CRLF checkouts also change text bytes. | Both platforms must reproduce the new archive hash without normalizing runtime/source evidence or replacing the frozen historical archive. |
| Add explicit lock/readme/layout inputs and a pinned extended build receipt to the dependency packager. | Current and historical framework candidates must keep distinct provenance. | Do not silently substitute framework binaries or accept unpinned extra receipt data. Historical compressed reproduction still requires its original compressor; preserve the original ZIP directly. |
| Add archive integrity/provenance/reproduction tests and actual installer-core selection/refusal checks. | A default dependency switch must install the exact tested framework without an override and reject the prior ZIP. | Keep hash checks, process guards, old proxy recovery, shared dependencies and both Windows/Linux CI jobs. |
| Update candidate instructions and retain a separate mitigation validation record. | The install link must match the new pin, and prior failures must remain visible. | Distinguish shipped-core API checks from GUI checks, requested AMD fallback from activation, and bounded clean exits from a universal crash repair. |

No mod native, UI, shader or NVIDIA runtime binary changed for this mitigation.
Player/account saves and raw diagnostics remain private. The separate framework
dependency follows the maintainer's 6 October exception and is never embedded in
the mod installer. Final Linux runtime and Windows NVIDIA/HDR checks remain
separate; do not erase these limits during subsequent Linux work.

## Full GUI qualification of the MSVC candidate - 7 October 2026

The [full Windows AMD report](WINDOWS_FULL_QUALIFICATION_2026-10-07.md) records
the downloaded `e421e57` CI artifact, GUI upgrade/fresh install, repair and
ownership refusals, uninstall/reinstall, both menus, AMD SR/FG/Reflex fallback,
active play and three held shipping-process exits. All exited 0; Save and quit
took 85.3 seconds and one window close took 86.45 seconds. Keep these delays
visible even though both completed within the 120-second ceiling.

This qualification changed evidence and documentation only. It does not replace
the historical framework FAIL or establish a causal crash repair. Preserve the
exact MSVC framework pin, existing Windows fixes and both CI platforms. The
report distinguishes maintainer-reported dungeon travel and source/CI foliage
eligibility evidence from directly captured runtime observations. Windows
NVIDIA/HDR/controller and Linux gameplay remain separate hardware checks.

## Experimental provider store / SDK-header compatibility — 8 October 2026

The provider mirror uses the documented `FileRenameInfoEx` information class
(value 22) with replacement and POSIX semantics. LLVM MinGW's default target
headers hide the enum name; the Windows candidate build exposed that difference
after the portable g++ fixtures passed. The store now spells the documented
class value explicitly, preserving the same API and old-reader behavior.

Keep the SDK enum-value assertion when available and the Windows held-reader
fixtures under both default and older target-header macros. Unsupported runtime
rename still fails closed; do not substitute delete/copy or legacy replacement.
The observer remains opt-in and does not modify released payloads.

## Steam build 25754144 layout trial — 8 October 2026

A new Steam executable moves the inspected modular latency registry, marker
wrappers, FName constructor and simulation/render frame counters. The historical
layout correctly rejects it. `engine_layout.hpp` retains the original map and
five signature gates, and adds an explicit new-build map gated by PE metadata,
all five dispatch guards, four registry/name entry guards, and the live registry
object/vtable identity. Do not calculate one address delta for the entire image.

Both the pacing callback and the FG exported render-frame accessor must use the
selected map; mixing old and new frame counters can break token identity even
when registration succeeds. The FG recipe records the map header hash. Keep
the portable corruption/missing/mixed-map fixtures, frame guard, actual-adapter
policy, shared SDK shutdown and both CI platforms. These native changes require
new Windows runtime checks; earlier AMD qualification does not cover them.
The game installer pin and released assets remain unchanged by this trial.

## Consolidated menu transaction processor

`menu_requests.hpp` is a portable worker component, not a new installed UI or
transport. Preserve its session/sequence identity, immutable request IDs,
revision/checksum CAS, migration provenance and uncertainty handling when wiring
NeoRune. Its Windows fixtures must execute the real filesystem publication race
and corrupt-record preservation cases as well as mock-store results. Do not
reinterpret a commit receipt as GPU capability or successful SDK activation.
Link the Windows filesystem fixture statically, as with the observation/store
fixtures. The hosted runner otherwise exits with code 127 before entering the
test because the compiler's dependent runtime DLLs are not on its launch path.
The consolidated transport and authority-reader cutover still require rebuilt
Windows runtime checks; these fixtures do not establish that qualification.


## Consolidated menu save transport

The opt-in transport runs on the existing settings worker, including when the
NVIDIA provider is unavailable. Preserve the worker's startup/join lifecycle;
transport failures must not initialize a feature or change rendering settings.
Its CRC word slots accept the full signed IntProperty bit pattern without
weakening the legacy nonnegative parser. The authority response preserves the
verified UE seed and uses flushed, atomic replacement; missing/corrupt authority,
wrong sessions and stale CAS stamps fail closed. Keep response publication off
render/present callbacks and avoid per-poll writes of identical snapshots.

Portable native tests exercise real request/response files, sessions, no-op
retries, corrupt authority preservation and unchanged-response timestamps.
The exact C# client source runs with test-only UE substitutes on both CI
platforms; 4,000 CRC comparisons do not qualify UE serialization. The separately
compiled NeoRune overlay has bounded Linux runtime evidence. Its timer is driven
by the game actor, not a detached SaveGame object's world context. The frozen
Windows candidate UI remains reused only under its existing source/hash checks;
the experimental overlay is not silently substituted into that artifact.
The recipe records all provider header hashes in the native candidate receipt.
New Windows UE/native runtime checks are still required for this transport and
for the later real-control/runtime-reader cutover.

## Experimental FSR game bridge

Keep AMD API descriptors and Microsoft COM calls inside the Microsoft-ABI bridge;
expose only fixed-width plain-C structs to the native observer. Match explicit
resource formats/dimensions and the verified game device/command proxy before
recording. Query returned analytical provider IDs and required inputs instead
of assuming ML support. Failed create may return an owned session; retain or
explicitly destroy it, never silently abandon it. Destroying a recorded context
requires invalidated recordings and a subsequent completed fence; failed proof
retains its context and loaded modules.

The real-game two-frame Linux gate is an isolated developer probe, not a new
released rendering path. Its successful execution and readbacks do not qualify
Windows/AMD, reduced-resolution reconstruction, output substitution or FG. The
Windows candidate recipe compiles the bridge using the already pinned external
MIT headers and includes it only as a separate experiment, with no AMD runtime
DLL. Keep both platform source/generation checks and the Windows native build.
See [the gate record](FSR_GAME_EVALUATION_2026-10-08.md) for its limits.

The [sustained output trial](FSR_SUSTAINED_OUTPUT_2026-10-08.md) retains that ABI
and adds a separate developer build. Preserve the output-format/copy guards,
immutable Native reset upload, dirty-history retirement barrier, frame-stamp
reset logic, bounded first/last readbacks and both platform generation checks.
Its Linux/NVIDIA Native-AA output evidence does not establish Windows/AMD,
reduced-resolution or induced-failure fallback qualification. Keep this addon
separate from the ordinary Windows installer and preserve the 16 KiB stack guard.

The [Quality/rejection gate](FSR_QUALITY_FALLBACK_2026-10-08.md) adds an active-
context SDK preset query and immediate independent Native history reset on all
failed frames. Keep dirty generations retained when reset cannot be recorded;
never let a failing SDK frame run ordinary Native with FSR-derived history.
The Windows experimental artifact now compiles Quality plus a deterministic
plain-C bridge rejection after 128 frames. No invalid input reaches the vendor
SDK. This variant stays separate from installers; Linux execution and both
platform generation guards do not qualify Windows/AMD execution, device removal
or source-scale rollback. The later UI transaction must restore source scale.

The [shared menu client](PROVIDER_MENU_CONTROLS.md) preserves the word-slot class
paths and signed checksum bits. In consolidated mode, display and AMD latency
must consume the same authority; a legacy save must not overwrite it. Invalid
authority disables latency without applying a zero HDR calibration. Snapshot
readers must use the exact 144-byte plain-C ABI, handle a busy worker without
blocking the render thread, and refuse a stopped settings worker. Preserve both
platform client/projection checks and include the `.h` ABI source in build
provenance. The native SR controller and normal Video rows have not switched to
this client yet; no new Windows runtime qualification is implied.
