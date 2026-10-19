FSR output allocation may be padded beyond the logical viewport after a Windows
resize. A captured 1680x1050 SDK output targeted a 1680x1052 game texture; rejecting
that pair produced error 17. Preserve the exact-size `CopyResource` fast path.
For a larger target, permit only the known single-mip/array/sample 2D RGBA16F
output whose source dimensions equal the logical output, and copy subresource 0
through a bounded logical `CopyTextureRegion` box. Leave padding untouched;
reject smaller targets, aliasing and other descriptor mismatches before recording
transitions. The actual-function portable fixture and owned AMD texture readback
cover copied pixels, preserved padding and rejection. A fresh game resize run is
still required; this correction does not establish a fix for the separate
presenter resize failure.

The [diagnostics attribution correction](RUNTIME_DIAGNOSTICS_ATTRIBUTION.md)
separates current ready game context from SR activity. Native keeps zero SR
activity while independent provider diagnostics require the current full
authority stamp and world. FG V2 attributes asynchronous SDK outcomes to their
admitted frame epochs; V1 remains unchanged. AMD timing requires its applied
revision and checksum. Preserve the all-channel UI identity/expiry checks,
bounded callback retention and anti-replay tests on both CI platforms. FSR
activity reuses the worker's decoded native runtime snapshot; Busy is unknown,
not a fresh zero/reset. Preserve the 1500 ms original-observation bound, exact
identity/Active/error gates, immediate successful inactive/error invalidation
and independent per-channel UI recovery baselines. No extra render lock or SDK work is
permitted for sampling. Offline
checks and code review do not qualify a new artifact or repair the exit fault;
the changed complete package still requires fresh Windows runtime testing.

The [19 October dev.17 qualification](WINDOWS_DEV17_QUALIFICATION_2026-10-19.md)
pins current publisher Loader 2.6, Streamline 2.14.16, DLSS 310.9.16 and RenoDX
snapshot inputs separately from the frozen older packages. Its first full-package
ordinary quit reproduced the intermittent game-worker AV. A control also
reproduced the matching return chain with the original framework and current
RenoDX, while all our native/UI payload was absent. Two clean-game controls and
one UI-only control exited 0. Removing RenoDX from the original-framework/Loader
control still reproduced the same fault chain. The exact candidate framework
alone exited 0 after 86.70 seconds. These bounded controls do not identify the
precise cause or qualify Windows shutdown. Preserve the failed qualification,
exact loaded hashes, private recovery data and remaining runtime gates.

The same report's follow-up adds nine passing native owned-host probes and two
bounded active FSR Quality hub observations with fresh matching state. Three
ordinary hub closes returned 0, but the FSR closes took 87.465 and 24.480 seconds.
A delayed-exit snapshot identifies a real swapchain Present waiting in the AMD
driver through the framework/Steam-overlay/DXGI call chain, before the CRT stage
of the earlier AV. This is not a root-cause attribution or shutdown repair. Keep
the failed game-exit evidence and incomplete runtime gates; the renewed user
launch hold left the game closed with Native/FG Off preferences restored.

After launches resumed, a per-game Steam-overlay comparison exited 0 with the
setting Off after 185.572 seconds and with the original On setting after 6.332
seconds. Both had active FSR Quality and verified loaded candidate backing files.
The overlay DLL remained resident in both; hook absence is unproven. The delayed
run sampled a GameThread event wait through Microsoft xgameruntime, distinct
from the earlier sampled Present wait. No shared root cause or overlay workaround
is established. The overlay and Native/FG Off settings were restored. Keep the
failed qualification and unresolved AV; these observations are not a repair.

The Windows candidate framework also applies upstream ReShade
[`64017f98`](https://github.com/crosire/reshade/commit/64017f98cc1393db51d9f5b1307c64350680f41c)
for descriptor heaps that outlive their device. Registration follows proxy
construction/destruction rather than a deferred raw-heap destruction callback
holding the device pointer. Preserve the existing successful-Reset patch, exact
additional-patch hashes, clean-base/diff checks and both CI platforms. Candidate
trials require both patches; the historical separately pinned release remains
unchanged. This corrects a dependency lifetime defect, not a proven cause of the
intermittent game exit failure. The 19 October original-context capture still
showed an AV on an unidentified game worker with GameThread in CRT exit; its
explicit-anchor return chain matches the old build after the build's RVA shift.
An isolated Windows WARP reproduction released the proxy heap/device while a
raw heap survived: the old framework failed with heap corruption, and the fresh
patched framework exited 0. This proves the dependency defect, not the game AV's
cause. One framework-only control, one retained-MSVC full-mod control and two
fresh exact-source full-mod runs exited 0. The latter included advancing FSR
evaluations and Native/source/AA-quality restoration, without a debugger or
capture addon. A sleep/resume-interrupted run is excluded. Keep the
[bounded follow-up](../qualification/providers/windows-exit-heap-fix-2026-10-19.json);
the small sample and remaining exit delay do not establish complete shutdown or
installer qualification.

The SR menu shares its quality, custom percentage and last-Custom preference
across DLSS/FSR and Native. Preset/percentage edits never implicitly select an
upscaler. Keep the underlying SR/FG transport records independent and preserve
FG pairing validation. Skip DLSS on a known incompatible rendering vendor, and
gate its legacy source projection with current-session device metadata. Query
the actual D3D12 device LUID once at initialization, independently of latency
opt-in; never infer SR support from FG/Reflex or poll SDK capabilities each frame.
Unknown/stale metadata cannot authorize reduced DLSS source resolution or erase
a saved choice. NVIDIA vendor identity alone is not active DLSS/RTX capability;
the existing backend capability/source/resource gates remain mandatory.
The bounded source-built menu run recorded Balanced with both Native and FSR,
a preset edit that kept Native selected, synchronized SR preference records,
advancing FSR evaluations (46-1150), and exit code 0. It used the retained CI
renderers/framework with the changed latency addon and production UI rebuilt
locally. The complete fresh CI artifact was hash-verified and staged afterward;
its standalone WARP heap-lifetime probe passed, but that complete artifact has
not yet had a game runtime test. Keep the
[menu follow-up](../qualification/providers/windows-sr-menu-fix-2026-10-19.json)
and distinguish source-equivalent component tests from full artifact qualification.

FSR preflight requires the validated single-output TAA permutation. On Steam
build 25754144, `r.TemporalAA.Quality=1` used shader `62de03a8`, whose otherwise
matching constant-buffer and input layout includes a second UAV output. Merely
allowlisting it would skip an output the current replacement does not populate.
The bounded quality-2 control reached Active with advancing SDK evaluations.
Acquire quality 2 only for matching current-world FSR preflight; retain it until
native retirement/source rollback and across actor level transitions, then
restore only the still-owned value. The volatile receipt must reject a previous
renderer session, yield to external edits and bound restoration retries. Native
with FG Off never acquires the lease. Native + AMD FG may acquire only through
the separate current-world/100%-source/FG-owner acknowledgement below. Preserve all shader/resource/SDK/source-ACK
gates, managed ownership checks, both CI platforms and a real UI/ModInfo rebuild.
The source-built lease passed a bounded Windows Native → FSR Quality → Native
check: matching SDK counts advanced, full source scale returned, and the original
quality was restored on the first attempt. Keep the
[runtime record](../qualification/providers/windows-fsr-quality-fix-2026-10-19.json):
the same process exited with an access violation, so wider lifecycle, all
presets, the complete installer and shutdown remain unqualified.

AMD FG provider enumeration must not run repeatedly on the render thread. On
Windows/Radeon 860M the unchanged SDK support query took 127-278 ms per call,
and its once-per-second polling stalled Native menus with FG Off. Cache the
result by framework device and loaded bridge, including unsupported results;
retry the bridge's not-initialized response and forget on device init/destruction.
The cache owns no COM references and does not replace the SDK activation,
current intent/session/world, input, presenter or retirement gates. Preserve the
portable cache fixture in both CI jobs and the complete guide/header build receipt.
The polling-disabled control removed the observed stutter. The private rebuilt
guide with full controls restored then removed the periodic timing gaps, and
the maintainer confirmed smooth Native and FSR selections in menus/gameplay.
Keep the [bounded Windows record](../qualification/providers/windows-amd-stutter-fix-2026-10-19.json):
FSR still recorded timeout 22 with zero evaluations, and that run exited with
an access violation. Smooth output does not prove provider activation or a
shutdown repair. A complete rebuilt installer still needs qualification.

The render-scale slider always displays a numeric selected percentage, including
FSR named presets before gameplay and during menu suspension/fallback. The display
uses FSR preset ratios, or the matching error-free SDK plan while applying or
evaluating; it never authorizes source scaling. Preserve the native preflight,
source acknowledgement and current-session/world checks. Keep keyboard/controller
edits relative to the displayed value and rebuild the real UI/ModInfo after edits.
The 19 October private rebuilt UI loaded during the bounded stutter controls;
the numeric-slider source/build checks do not establish all control/input paths
or repair the separately observed Windows FSR timeout or access violation.

The bootstrap trace uses one mutex for opening, event/AMD-description writes,
flushing and closing. Factory creation and route teardown can run on different
threads; stdio's internal locking does not own a handle passed by another caller.
Preserve late-event refusal and idempotent close. The actual-source concurrency
fixture runs on both CI platforms and reproduces a closed-handle write with the
old source. Linux and an isolated Windows executable under Proton pass with the
correction; the Microsoft-ABI bootstrap also compiles. This establishes a trace
lifetime defect, not the cause of the game's intermittent `0xC0000409` quit.
The [offline receipt](../qualification/providers/bootstrap-shutdown-trace-2026-10-18.json)
keeps that distinction. Select a complete rebuilt artifact and repeat native
Windows/Radeon and Linux shutdown qualification before release.

The advanced page embeds a tabbed custom widget through Blueprint Loader 2.0,
with native stepper shells, native category buttons and the native bordered detail
card on hover/focus. Loader and SDK sources remain unmodified. Preserve Loader
save keys, independent SR/FG choices, keyboard/controller navigation and reset.
The advanced page leases only its own Loader page padding to include the header
and master toggle in the left column. Restore padding only while owned, retire
the sibling help pane on exit, and preserve the structure/active-page checks.
Its category bar reuses native buttons for font, hover/focus feedback and amber
glow, with its own exclusive button group and a simple mod-owned divider.
Retire click handlers on teardown and clear selection on page exit. Do not
instantiate the native strip controller or call visibility methods that assume
optional child bindings; standalone creation left its background null and caused
a GameThread access violation. No native strip lookup or controller is needed.
Leave Q/E with the main game tabs. This UI path still requires native Windows
menu qualification.
FSR SR create_v2 explicitly distinguishes analytical 3.1.5 from the separately
pinned portable INT8 4.1.1r11 DLL and official 4.1.1. Auto uses official 4.1.1
only on recognized Radeon RX 7000/9000 desktop hardware; other devices retain
analytical Auto. Explicit 4.1 chooses official or portable according to that
hardware classification. Recognition is not driver/provider activation proof.
create_v1 retains its analytical contract.
Preserve exact provider-name matching, active-ID/resource checks, failed-create
ownership and returned implementation metadata. FSR FG 4.0.1 is an independent
experimental device-approved selection; portable SR is not proof of ML FG support.
Keep the native SR bridge hash binding and all header/build source receipts.
Preset C uses DRS rather than the Streamline A/B UI-recomposition flag. Proton
sets process-local overrides before SDK initialization. Windows permits only an
existing one-application game profile, saves original settings first, restores
only values still owned, and refuses base/shared profiles or external edits.
Record driver request acceptance separately from active model identification.
These changed paths require new native Windows/Radeon and runtime qualification.

The AMD presenter status clears a stale restart requirement only after a
current, available, fault-free, non-retired AMD Off request has reached the native
presenter and the bridge confirms switching remains available. Preserve the
existing On admission and NVIDIA permanent-retirement rules. The shared
projection fixture and source checks cover this distinction on both CI platforms;
the status correction passed bounded Linux/NVIDIA runtime checks in a rebuilt
native candidate; Windows/Radeon coverage remains required.

The advanced Mods-tab page uses Blueprint Loader's existing 2.0 settings API.
Keep the separate versioned advanced request/state slots, core session/stamp
validation, replay refusal, loader-owned persistence, one-key native INI mirror
and restart-only model-override semantics. The named Mods-page selector has its
own Loader key; preserve the previous numeric key and unknown manual values
without reinterpreting NGX preset numbers as choice indices. Auto leaves all
NGX model-preset overrides unset and follows the selected DLSS quality mode.
Preserve migration from the earlier, differently ordered choice-index key.
Advanced state v2 keeps the 128-byte layout and original module bits, adding
the fifth (DLSS FG) module in bits 8/9 and its packed version in words 30/31.
The UI accepts v1 states without inventing that missing version. Signed packed
words must retain all bits, with version metadata requiring a loaded identity.
The visible Custom entry is removed; unsupported legacy values normalize to
Auto. Popup tooltips are removed and details stay in Diagnostics. Diagnostics
are opt-in and separate selected presets from unavailable active-model
readbacks. Do not label a framework/DLL version as an SDK-selected model.
Runtime versions come from held, exact-path
loaded modules and their in-memory version resources; inspecting them must not
load a provider or authorize a feature. Preserve both-platform protocol/client
checks, version-library linking and source-matched UI rebuild receipts. Native
Windows/Radeon and in-game page qualification remain separate from those checks.

The FG preset request has separate AdvancedFG slots, preserving SR replay/CAS
and migration behavior. It writes only the FrameGenerationPreset INI key;
Auto retains the Streamline default, A/B request UI recomposition Off/On, and
no active-model readback is inferred. The gameplay diagnostics transport is
opt-in, uses bounded mod-owned files and read-only snapshots, and requires
advancing counters with current session/revision/world checks. FSR implementation
versions retained from validated contexts must be labelled as last verified.
Use the shared FG latency recipe and keep its root-path rewrite and version
library link intact. Substituting the standalone adapter drops access to the
already-initialized shared Streamline owner and hides independent Reflex.
Preserve independent latency with FG Off. The sharpness slider uses one existing
post-upscale game filter and yields/restores only while it still owns that value.
New UI/native artifacts require fresh runtime qualification on each platform.

The UI build uses pinned NeoRune 1.0.0 and .NET SDK 10.0.402/reference pack
10.0.13. Preserve SDK-generated versioned ModInfo and its discovery header;
regenerating it with the older manual builder would erase Mods-tab settings.
Keep the isolated CLI build, complete source/payload receipts, CRLF-normalized
source hashes, explicit trial receipts and both-platform build-contract checks.
The dev.12 UI passed bounded Linux/NVIDIA startup, Video-menu and gameplay checks
with Blueprint Loader 2.3 and 2.5. All six FSR quality selections activated with
AMD FG; live Off/On and pause suspension also worked. These checks do not qualify
Windows/Radeon, physical controllers or long sessions. The complete dev.14
candidate also passed bounded Loader 2.5 startup, all six FSR presets with AMD FG,
pause/resume, display-confirmation preservation and normal exits with native and
AMD presenters on Linux/NVIDIA. Its stationary Off-recovery comparison met the
1% throughput and frame-time limits across five switching cycles. This does not
complete FG-On collector calibration or the wider benchmark qualification.
Historical release receipts and binaries remain unchanged; select a new complete
native artifact before installer qualification after further native changes.

The [native HDR Off correction](HDR_OFF_ACKNOWLEDGEMENT_2026-10-14.md) allows
the UI to disable native HDR without a loaded RenoDX addon. Preserve missing-addon
On refusal, loaded-addon calibration/restart admission and one wake per Off
revision. Keep the actual-source fixture in both portable CI jobs. Require a new
artifact and actual output-colour checks; saved HDR Off alone is insufficient.
Candidate installer assembly must skip stale source/artifact selections. Linux
source tests and HDR throughput measurements do not qualify Windows/Radeon.

The [shared gameplay readiness gate](FSR_MENU_READINESS_2026-10-14.md) applies
the existing pause/widget exclusions to SR as well as FG. Preserve HUD/activity/
notification allowances, source rollback outside gameplay and the acyclic actor
gate. Keep acquisition gameplay-only and retain existing matching foliage leases
through error-free temporary-menu retirement. The native timeout is unchanged.
Require source guards, managed cases and a real source-matched UI/ModInfo rebuild;
the bounded Linux control does not qualify native Windows/Radeon.

The [FSR retirement correction](FSR_OWNED_RETIREMENT_2026-10-14.md) clears the
owned resource list after manual reverse releases, before bridge unloading and
generation destruction. Otherwise `EvalOwned` releases those references twice.
Preserve SDK destruction, GPU/recording admission and failed-drain retention.
Keep the actual-source reference-count fixture and order guard on both platforms.
Earlier dev.8/dev.9 artifacts do not include this correction and cannot qualify it.

The [Anti-Lag shutdown correction](AMD_ANTILAG_SHUTDOWN_2026-10-14.md) removes
`Update` from teardown: the official SDK inserts its pacing operation there.
Preserve clear/drain before `DeInitialize`, failed-drain context/module retention,
ordinary pre-input updates, the unchanged ABI and frame-type handoff. The real
Microsoft-ABI fixture starts enabled and checks zero teardown mode/delay calls
on failed drain, successful retry, API error, repeated and independent cleanup.
Keep it in native Windows CI and preserve the controller's threaded teardown
checks on both platforms. A Wine synthetic pass does not qualify Radeon hardware.
Earlier dev.8 results do not qualify the changed bridge; select a new complete
source-matched artifact before Windows testing or publishing.

The shared FG restart status must include permanent session retirement, even
when the startup presenter still matches the requested provider. Preserve the
retired admission refusal, ownership changes, unsupported-provider behavior,
temporary menu suspension and AMD live Off/On. The portable projection fixture
checks all 96 availability/mode/owner/provider/retirement combinations on both
platforms; the source check verifies the real controller uses that predicate.
The original PR54 NVIDIA regression exposed the missing status after successful
permanent release. Corrected binaries need their own runtime checks.

The portable Windows checks use Bash with fail-fast step execution, so an earlier
Python failure cannot be masked by a later successful command. Keep the SDR
output checks on both platforms. UI source changes require a real NeoRune and
ModInfo rebuild with matching archive, member and source receipts; updating
hashes alone is not a rebuild. Candidate payloads do not replace release files.

The [SDR scene implementation](AMD_SDR_IMPLEMENTATION_2026-10-12.md) adds an
owned RGB-preserving, opaque-alpha snapshot of the primary backbuffer after the
original tone-map draw. Preserve complete original draw arguments, full coverage,
actual colour/format/device gates, submitted-frame and camera parity, independent
host recording leases, successful-reset stamps and strong Present borrows.
Keep output context recreation separate from presenter ownership and preserve
existing NVIDIA/HDR behavior. Include the new owned shader and all source headers
in build/package receipts, with both platform checks. Two private Linux controls
exited 0 after an earlier active SDR run failed with `0xc0000409`; these do not
establish causation or qualify a complete rebuilt artifact or Windows/Radeon.

The [FSR SR / NVIDIA FG route](FSR_NVIDIA_FG_2026-10-12.md) admits FSR through the
shared guide producer without equating SR vendor with FG vendor. Preserve exact
record and startup-owner matching, actual-adapter SDK checks, input readiness,
one presenter and compatible latency ownership. Native/NVIDIA FG, XeSS and MFG
remain excluded. Both-platform projection fixtures cover all six FSR qualities
and Off/On startup ownership. The menu uses one matching pairing predicate for
availability and atomic SR selection; preserve FG intent through FSR preset and
scale edits, and Native/NVIDIA refusal. Retain managed transaction checks and a
source-matched rebuilt UI archive/receipt. These do not qualify gameplay or
Windows/Radeon.

Queue teardown must notify the shared SDK owner even when no NGX SR feature was
created. Preserve callbacks outside the native observer mutex, NVIDIA stopped
generation and alpha GPU completion, native NGX retirement, terminal/resize
refusal and one-shot cleanup. Keep the AMD bound-device gate and both-platform
128-combination fixture. Queue notification restored a normal Linux process
exit, but shared SDK shutdown returned error 24. NVIDIA closing-owner cleanup
now attempts the same admission after alpha retirement, before graphics teardown;
a retained NGX feature still defers to its queue notification. Preserve both
notification orders, callbacks outside locks and resize/unsupported-owner refusal.
The matching corrected artifact passed three bounded Linux/NVIDIA transition
and exit runs, including explicit retained-NGX refusal before later successful
SDK cleanup. Keep those numeric receipts separate from the earlier hang and
error 24. They do not qualify Windows/Radeon or broader lifecycle.

The [bootstrap module identity](BOOTSTRAP_MODULE_IDENTITY_2026-10-11.md) uses the
full executable-directory path when local and system DXGI are both loaded.
Preserve missing/unloaded refusal, checked heap-backed path storage, no library
loading, all ownership/UI/detachment consumers and source receipts. Retain the
real duplicate-basename fixture on Windows and the source checks on both CI
platforms. Native Windows CI and two matching-artifact Linux transition/exit
runs passed; neither qualifies Windows/Radeon gameplay or wider lifecycle.

The [AMD shared SDK retirement](AMD_SHARED_SDK_RETIREMENT_2026-10-11.md) requests
Streamline cleanup before device destruction only after both AMD presentation
and native NGX retirement are observed. Preserve atomic AMD-only bound-device
readiness, missing/busy/retained refusal, one-shot admission, callbacks outside
the guide mutex, resize exclusion, existing NVIDIA retirement, source receipts
and both platform fixtures. A private early-cleanup trial is ordering evidence;
it does not qualify the changed production source or Windows/Radeon shutdown.

Native SR resource destruction and command-list reset/destruction use the NT
terminal guard before the state mutex, alongside existing queue/device guards.
Preserve ordinary resource and recording retirement and the terminal-lifetime
fixtures on both CI platforms. This does not identify the intermittent quit
cause; rebuilt native artifacts need their own runtime qualification.

The [AMD FG presentation gate](AMD_PRESENT_SERIALIZATION_2026-10-10.md) keeps one
outer COM identity and serializes both Present methods with Prepare/Configure/
Destroy. Preserve standard interface identity, unknown-interface refusal,
terminal refusal before locks, normal final Release, exact raw vendor config
identity and all source/header hashes. Retain the real Windows COM fixture and
both platform checks. A synthetic Proton probe does not qualify Windows/Radeon
or establish the cause of the intermittent game shutdown failure. One combined
game control exited with code 0; a repeat hung after SDK retirement returned 0.
Subsequent shared SDK ordering and bootstrap-identity changes passed two bounded
Linux transition/exit repeats, as recorded in their reports. Native
Windows/Radeon shutdown and longer lifecycle coverage remain unqualified.
The fixture exercises actual final raw destruction, not merely an intermediate
reference release.

The FG guide addon now uses the same terminal process guard as SR and latency.
Resolve the NT query outside DllMain; skip callback state locks and SDK retirement
only during final process teardown. Ordinary unload must still request retirement.
Preserve both header hashes in its build receipt and both platform checks. This
closes a source-level teardown gap; it does not establish the cause of the
packaged candidate's intermittent fatal exit or qualify Windows/Radeon runtime.

The [FSR foliage lease](FSR_FOLIAGE_2026-10-09.md) uses actual current-world SR
output before changing the material velocity CVar. Preserve matching session,
revision/checksum, scale and successful-evaluation admission, acquisition only
in gameplay, external ownership and restoration readback. Keep existing NVIDIA
checks and both platform fixtures. The bounded Linux pass does not qualify
Windows/AMD or the separately observed shutdown failure.

# Windows fixes: reasons and preservation requirements

The [SDR boundary diagnostic](AMD_SDR_FG_2026-10-12.md) records actual ReShade
presentation colour space, indexed/non-indexed draw arguments and explicit
viewport/scissor/blend/write-mask metadata. Preserve unknown coverage defaults,
capture-disabled and terminal returns before locks, and the original rendering
callbacks. A 10-bit format does not prove HDR, and an RGB-only tone-map draw does
not establish alpha validity. Keep both-platform source checks. The private
Microsoft-ABI diagnostic's Linux metadata/exit pass does not qualify SDR FG,
output resize, production performance or native Windows/Radeon execution.

The temporary [provider present sampler](AMD_PROVIDER_BENCHMARK_2026-10-12.md)
uses the same NT process-termination guard before its callback mutex and skips
unregister only during terminal DllMain detach. Resolve the query in AddonInit;
preserve ordinary unload registration cleanup and both-platform source checks.
Its original instrumented quit failed with `0xc0000409`; a no-sampler control
and two guarded-sampler sessions exited 0. This does not establish causation or
native Windows qualification. Production renderer/provider files are unchanged.

The [AMD development installer assembly](AMD_CANDIDATE_PACKAGING_2026-10-10.md)
uses the pinned PR45 Windows native artifact and the explicit Steam 25754144 game
pin. Preserve complete archive/member/source verification, exact bridge binding,
external runtime separation, known-framework upgrade backups and all existing
ownership checks. Candidate descriptions may replace the NVIDIA-only summary;
the default release text remains unchanged. Installer overrides do not relax
native FSR/Anti-Lag capability or runtime checks. Both platform fixtures remain
required. Temporary-tree package transactions on Linux do not qualify Windows
GUI execution or actual Radeon GPU features; existing published pins are intact.

The [separate candidate installer inputs](AMD_CANDIDATE_PACKAGING_2026-10-10.md)
embed and distribute the same captured manifest/dependency-lock bytes. Preserve
the default released inputs, resource snapshots, payload checks, external-runtime
exclusion and full folder deployment. Resource-selection builds do not establish
AMD installer or Windows GPU runtime qualification; both platform jobs remain
required.

The [NGX device retirement correction](NGX_DEVICE_RETIREMENT_2026-10-10.md)
also applies the existing terminal-lifetime guard to Native FG, FSR and NGX
generations and skips terminal queue/device callbacks before locks. Ordinary
unload and gameplay retirement remain required; replayable borrows are not
freed by this guard. Native guide manual release clears destructor ownership. On ordinary ReShade
unload with any retained generation, pin the exact addon/framework modules
outside DllMain, unregister callbacks and refuse reuse until process restart.
This is retained ownership, not completed retirement or hot-reload support.
It uses the exact held proxy device supplied to NGX initialization for unshared
`Shutdown1`. The direct driver export takes an additional writable remaining-device
count; preserve that two-argument ABI and its actual-source fixture in both
portable jobs. Preserve the non-null guard, verified shared ownership, fresh GPU
completion and feature/parameter release before owned device/resource release.
Retain the source checks on both CI platforms; Linux exit controls do not
qualify Windows/NVIDIA NGX lifecycle or identify the mixed-provider exit delay.


The [AMD Streamline feature lifetime](AMD_STREAMLINE_FEATURE_LIFETIME_2026-10-10.md)
loads Reflex/PCL without NVIDIA DLSS-G for AMD startup ownership. Preserve
startup-only SDK discovery before any device/token/proxy binding, exact actual
adapter-LUID capability lookup, duplicate/invalid-init rejection, device-matched
NGX ownership and real shutdown calls. Legacy/NVIDIA ownership keeps its full
feature set. Retain the configuration fixture on both platforms. Linux exits
and isolated SDK probes do not qualify native Windows/Radeon shutdown.

The [Native-AA AMD FG guide consumer](NATIVE_AA_AMD_FG_2026-10-09.md)
leaves native temporal output intact and performs no conversion when FG is Off.
Preserve authority/session/world/freshness, full-scale/device/viewport admission,
reset-on-gap and independent recording/fence ownership. Keep Native/NVIDIA FG
excluded until qualified, actual Anti-Lag presenter/driver gates and the camera
include closure on both platform builds. Bounded Linux/NVIDIA execution cannot
qualify native Windows/Radeon or a performance/latency benefit.

The [AMD FG private recording contract](AMD_FG_OWNED_RECORDINGS_2026-10-09.md)
stages inputs and submits only owned SDK command lists after host work on the
exact graphics queue. Preserve v2 export admission, nonblocking ring reuse,
completion plus successful Reset before releasing borrows, failed-signal
retention and fresh queue completion before private-list destruction. Never
reset a game-owned list to obtain retirement. Keep the isolated queue/replay
fixture and both CI platforms. Linux activation and SDK retirement do not
qualify Windows/Radeon or independent FSR SR teardown.

The [FG provider FName decoder](FG_PROVIDER_NAME_2026-10-09.md) accepts case
variants of the current and legacy provider property names. Preserve rejection
of duplicates across both aliases, invalid values, unknown properties, stale
sessions and mismatched authority. Keep the 1,280-spelling fixture on both CI
platforms; serialization spelling must not imply GPU capability or activation.

The [final process teardown guard](PROCESS_EXIT_2026-10-09.md) prevents SDK calls,
worker joins and coordinator destruction after NT process teardown has begun.
Preserve the real-query Windows DLL fixture and ordinary-unload cleanup checks.
Device/lifecycle cleanup outside process termination must still drain providers.
The bounded Proton exit pass does not qualify native Windows gameplay or repair
the separate FSR SR lifetime path. AMD FG private-recording retirement is
covered by its own candidate report.

The opt-in [continuous FSR controller](FSR_CONTINUOUS_2026-10-09.md) preserves the
MSVC bridge, native stack guard and independent recording/fence ownership. Its
context and runtime-response ABI sizes are 288 and 136 bytes. Retain codec,
source-acknowledgement, actor-travel and corruption checks on both CI platforms.
Dependency hashes are checked before loading; unsupported or invalid inputs
cannot lower source resolution without successful preflight. Linux/NVIDIA FSR
execution does not qualify Windows/AMD runtime, UI parity or AMD FG.

The isolated FG/SR build copies both native and provider headers plus the FSR
plain C ABI header. Its relative-include closure test must pass before compiling;
vendor headers and DLLs remain separately acquired. Provider UI source and
qualification receipts also trigger the Windows candidate build.

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
and callback shutdown barrier. The owned FSR presenter uses the documented private-data handshake, with
clear/drain before Anti-Lag release and retention when drain fails. Preserve
those lifetime checks and actual-device gates. Synthetic SDK tests and
Linux/NVIDIA fallback are not active Windows/AMD qualification. Never copy the fake amdxc64.dll into an
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

The current AMD development handoff is
[Windows/Radeon candidate qualification](AMD_WINDOWS_CANDIDATE_2026-10-10.md).
Use its verified PR45 artifact and per-file hashes; the old released installer
and bounded isolated probes do not install the continuous FSR candidate.

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
provenance. The later [native Video-menu cutover](FSR_VIDEO_MENU_2026-10-09.md) projects
committed shared settings into the existing NVIDIA controller and adds FSR
controls. Keep current-session/world admission, independent preset ratios,
source-only rollback on authority loss and source-matched UI receipts. New
Windows runtime qualification is still required.

The ordinary Anti-Lag row requires a matching current-session runtime capability
from the actual driver and installed timing owner. Preserve its Off/On-only
request, actual owned AMD FG handshake and matching-revision active acknowledgement. Missing new
fields in the existing schema-1 display slot mean unavailable. Both platforms
run the synthetic display codec and exact C# admission/request fixtures; those
checks do not establish active Windows/AMD driver support or latency reduction.

## Isolated AMD Frame Generation probes

The [FSR FG probes](FSR_FRAME_GENERATION_2026-10-09.md) compile with the Microsoft
ABI, pinned external MIT headers, static CRT and 16 KiB stack guard in the full
Windows candidate job. Keep both executables separate from the installed addon
and exclude the AMD runtime DLL. Source-build success is not GPU execution.

Preserve actual selected-provider checks, owned recording invalidation followed
by a completed fence, SDK presentation drainage before callback/context release,
and independent real/generated callback plus DXGI presentation counts. A failed
retirement retains the generation. The SDK-owned waitable handle is closed once;
no Streamline vendor presenter is chained into the AMD presenter. Linux/NVIDIA
isolated execution cannot qualify Windows/AMD, in-game AMD FG, Anti-Lag coexistence
or a latency/performance benefit. Both existing CI platforms remain required.

### Native AA / AMD FG candidate qualification

Use the exact artifact from PR41 and separately verified dependencies. Start
with Native / FG Off, enter gameplay and verify full source resolution and
zero SR evaluations. Select AMD FG / On through Video, restart when requested,
and re-enter the same scene. Require matching current-session/world/authority,
full-resolution current guides, advancing real/generated presenter counters,
SDK status 0 and no SDK errors. Inspect motion and foliage rather than treating
a successful dispatch as artifact-quality qualification.

Pause for two independent observation intervals: generated counts must stop,
while the real presentation path remains responsive. Resume and require fresh
history reset followed by renewed generation. Exercise Native → FSR → Native,
world travel and resolution changes, then normal exit and AMD SDK retirement.
Record artifact/dependency hashes, adapter/driver, counts and exact failures.
Do not treat the separate Linux/NVIDIA pass or unsupported Anti-Lag fallback
as active native Windows/Radeon qualification. Anti-Lag coexistence additionally
needs the actual owned-presenter metadata handshake and native driver support.

### Continuous SR Windows build closure

The Windows candidate must pass the SHA-256 of its own source-built
`mcd2-fsr-game-bridge.dll` to `build_sr_candidate.py`. An empty pin deliberately
keeps FSR inactive; a Linux bridge hash cannot authorize a Windows-built binary.
Refuse missing/empty bridges or a mismatched generated SR receipt.

The native-only addon recipe does not compile shader assets. Compile and include
both `live_dense.cso` and `fsr_dense.cso` with the pinned Windows DXC, and record
their source, compiler and output hashes. `fsr_dense.hlsl` includes the shared
`live_dense.hlsl`; both source hashes belong in that receipt. Missing/empty
compiler outputs fail the build. Keep these owned assets in the isolated SR
candidate, with vendor runtimes external. Both CI platforms check the recipe;
Windows native build success still requires separate Windows GPU qualification.


### Experimental AMD presenter reset

Development candidates can opt in with `AmdPresenterSwitch=1` under
`[Providers]` in `FGBootstrap.ini`. The option is read at swapchain creation.
The disabled path retains the existing forwarding behaviour.

The Video menu waits for a committed, current-session FG selection. A bounded
engine resolution change supplies the actual `ResizeBuffers` boundary, then
restores the previous resolution and window mode. Preserve the exact prior
last-confirmed resolution and fullscreen mode as well: the temporary mode must
not become the saved confirmation. Restore only confirmation still owned by
that handoff; subsequent user display changes take precedence. Finish deferred
confirmation ownership before starting another handoff and on actor teardown.
The actual-source confirmation fixture runs on both CI platforms. Bounded
Linux/NVIDIA main-menu and gameplay checks do not qualify Windows/Radeon.
No presenter is replaced inside `Present`. Keep the shared SDK gate, fresh queue
completion, callback drainage, one public COM identity, colour/HDR/private-data restoration and
recovery to the previous presenter on target-creation failure. Issued waitable
handles, exclusive fullscreen and incomplete metadata decline the handoff.

`presenter-resize-probe.exe` exercises the real bridge with owned resources;
`on-exit` additionally retires an active SDK presenter. Neither an isolated
probe nor Linux/NVIDIA execution qualifies Windows/Radeon behaviour. Candidate
qualification must compare warmed fresh Off against On → Off in the same scene,
verify HDR/SR, keyboard focus, display restoration, pause/resume, travel and
repeated normal exits. SDK frame counts do not establish physical display FPS.

### Bounded guide observer resource lifetime

The optional guide capture must check the existing init/destroy resource registry
before querying native metadata or mapping a cached binding. A nonzero view lookup
can still resolve an expired resource; cached descriptors do not own its lifetime.
The observer already holds its mutex for this check and inspection. Unknown or
destroyed resources are skipped. The actual observer regression covers retained
view/constant-buffer bindings after destruction, unknown/null bindings and live
recovery in both portable CI jobs. This source correction requires a fresh full
Windows candidate and runtime retest; handled Unreal crashes can exit with code
zero and must never count as successful gameplay or normal shutdown.

### HDR addon activation ownership

RenoDX UE Extended's SDR tone mapper does not disable its separate SDR-to-HDR
swapchain upgrade. HDR Off therefore owns only the exact ReShade
`[ADDON] DisabledAddons` token `RenoDX@renodx-ue-extended.addon64`, with a bounded
write-ahead `MCD2Graphics/HdrAddonActivation.bin` receipt beside the addon. A
resident RenoDX keeps HDR output acknowledgement blocked until restart. A fresh
Off startup without it can acknowledge native SDR; On restores only a token
inserted by this lease and needs restart before calibration acknowledgement.
Preexisting user disables are never removed. No live unloading or `Set_Path`
override is used. Other disabled addons and calibration values are preserved.

The lease retains original/last key values and missing-versus-empty state,
survives ReShade's delayed config save, and permanently yields on observed
external edits. Corrupt ownership or ambiguous/redirected configuration fails closed.
Management supports the normal game-bin ReShade.ini only; BasePath/environment
redirection is declined. Guided uninstall validates and restores the exact
owned key before deleting any mod payload; conflicts stop removal with an
actionable message. Preserve native/managed receipt compatibility, crash-window
and actual backend tests on both platforms. Public config APIs provide verified
readback, not atomic CAS against arbitrary concurrent external writers.
This correction does not qualify HDR on an unsupported/SDR Windows display or
repair the independently unresolved game shutdown defect.

### Native AA frame-generation temporal preflight

Native + AMD FG Single/2x needs the same validated single-output TAA producer.
Its separate lease admission requires current matching authority, settled Native
(or completed Native fallback), observed and requested 100% source, the implemented
world scale, and a current matching FG controller acknowledgement of the actual
AMD owner and availability. Prior generated frames are not required for producer
preflight. Native source resolution and SR SDK evaluation remain unchanged.

Retain a Native-guide lease across pause, missing context and actor travel. Off
restoration requires a current matching FG acknowledgement and the optional
NativeGuidesRetired readback equal to one. Unknown/BUSY and old runtimes retain.
The native export takes a try-lock and reports retired only after the generation
and its outside-lock COM release have both finished. It makes no SDK/GPU calls.
External console edits, original-value receipts and bounded restore retries still
apply. Actual-source client/native release-window fixtures cover this contract;
a fresh complete UI/ModInfo/native artifact and Windows runtime checks are required.

### Nonblocking diagnostic observations

A Busy (`-2`) identity getter is Unknown, not proof that the selected FG
request/world ended. Skip epoch binding and new diagnostic frame admission on
Unknown; retain original attributed outcome timestamps. Successful Off, changed
session/stamp/world, not-ready/invalid context, hard errors and expiry still
invalidate immediately, including when the other getter is Busy. SDK readiness
and generation admission remain unchanged.

Worker FG snapshots may survive Busy for at most 1500 ms from the original
successful read, with exact full identity and the existing producer-outcome
freshness bound. Explicit inactive/fault/malformed/error observations must not
revive cached activity. The same bounded V1 metadata reuse preserves the FSR
implementation label through Busy without supplying SR evaluation activity.
Actual identity/producer/transport fixtures cover these cases on both portable
CI platforms. Source and build checks pass; fresh Windows runtime qualification
of this correction remains required.
