# Windows shutdown isolation — 7 October 2026

This is a separate diagnosis phase after the failed
[public-installer qualification](WINDOWS_INSTALLER_QUALIFICATION_2026-10-07.md).
It does not convert that qualification to PASS or qualify another framework by
substitution. Repository source was refreshed and verified at
`9201e27fea2a2c8ab80ba1b898e0ed0a4c611e83` before these controls.

**Status: crash fix unresolved; diagnostic controls and restoration complete.**
The [sanitized aggregate](../qualification/fg-release/windows-shutdown-isolation-2026-10-07.json)
keeps the matched controls, power-interrupted run and supplemental repeat
separate. No production fix is established by this report.

Windows 11 Pro 10.0.26200.9457, Radeon 860M driver 32.0.31041.1004, Steam build
25647713 and shipping SHA-256
`231147bd0c655a4ae73f90873675d42917f2bfb3a9ee164fc64f217d6d6bd4ef`
were retained. Each gameplay control launched through Steam. Shipping process
handles were held before the normal Alt+F4 close; no game was force-killed.

## Controls

Known mod-owned files, receipt, graphics proxy, startup configuration and both
UI/Loader container sets were moved outside game load paths using private,
hash-guarded recovery transactions. Framework-only controls additionally moved
RenoDX outside the load path. Actual loaded modules were inspected: no MCD2
addon, FG bridge, RenoDX or private Streamline runtime was loaded. The unmodified
control had only system DXGI. Framework controls used a temporary standalone
DXGI proxy, without the FG bootstrap. This is a private isolation experiment,
not a supported installer layout or installation recipe.

| Control | Observed hub interval | Process lifetime | Close to exit | Shipping result |
| --- | --- | --- | --- | --- |
| Unmodified game | 316.98 s | 676.73 s | 91.48 s | `0x0` |
| Exact pinned PR435 framework alone | 332.71 s | 511.67 s | 9.43 s | `0xc0000005` |
| Same-source MSVC baseline A alone | 311.88 s | 528.23 s | 10.00 s | `0x0` |

The supplemental B repeat exited from the main menu after previously observed
gameplay: process lifetime 1854.41 s, normal-close interval 85.93 s, shipping
result `0x0`. Its continuous hub interval is unknown because utility focus
interrupted observation; it is excluded from the matched gameplay table above.

The framework-alone binary is SHA-256
`16c05f65b47b1a1f7b21eaa9a89cfbb7868b72894aab58610763c0e4014656b1`,
the exact separately supplied dependency from the public qualification. Its
D3D12 runtime initialization was independently confirmed. Mouse movement and
continued rendering were observed. The unmodified game also ignored injected
gameplay I input, before the on-screen keyboard experiment. That experiment did
not provide a usable menu route; Save and quit remains untested here.

Two initial direct-executable attempts could not connect to Steam and never
entered gameplay. Their clean exits are startup-only observations and are
excluded from the gameplay comparison.

## Crash fingerprint and limits

The two public-package crashes, two historical 5 October access violations and
the framework-alone crash have the same fingerprint:

- Exception `0xc0000005`, read from `0x10`.
- Fault bytes `4d3927`: `cmp qword ptr [r15], r12`, with `r15 = 0x10`.
- Instruction address outside the dump's named module ranges.
- Following jump returns to shipping image RVA `0x8bdcb86`.
- Raw stack memory contains common shipping RVAs `0x8cb73ec` and `0x8b56dd0`.

Microsoft's debugger independently confirmed the fault instruction and register
state. In the first public crash the faulting thread was unnamed; GameThread
was waiting. The debugger could not unwind through the unnamed instruction
region. Raw stack pointers are not a proven call chain, and the nearest exported
symbol does not name the actual failing function. Raw dumps and debugger logs
remain private.

The shutdown delay occurs without the mod. The matching access violation can
occur with ReShade alone: MCD2 native/UI code, FG/Reflex plugins, RenoDX and Loader
are not required for that reproduction. One clean unmodified exit does not
establish that the base game can never crash, or prove the precise framework
defect. The framework log's inconsistent swapchain reference-count warning is
an observation, not an established cause.

Reflex/PCL engine registration on AMD is not itself proof of faulty fallback.
NVIDIA documents PCL statistics across GPU vendors and distinguishes them from
low-latency availability. Do not disable valid marker integration merely because
Reflex low latency is unavailable; see the
[official Reflex guide](https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideReflex.md).

## Candidate fix experiment

Upstream ReShade commit
[`64017f98cc1393db51d9f5b1307c64350680f41c`](https://github.com/crosire/reshade/commit/64017f98cc1393db51d9f5b1307c64350680f41c)
fixes descriptor-heap cleanup when the original heap outlives the device proxy.
It unregisters at proxy destruction instead of retaining a callback to a device
that may already be destroyed. This is a D3D12 ownership requirement; it is not
proven exclusive to Windows or established as this crash's cause.

Two private source-built frameworks use the same MSVC 14.44.35207,
MSBuild 17.14.51.32402 and Windows SDK 10.0.26100.0, Release x64 with full addon
support. Both retain base `4eb9056c76016aad6f98495d3bbda2d721106104`, pinned
submodules and the existing successful-reset patch. B additionally applies the
exact upstream descriptor-heap fix. No mod/native/UI/shader/settings source was
changed for this comparison.

| Build | Additional heap fix | DLL SHA-256 | Runtime result |
| --- | --- | --- | --- |
| A | No | `ce808bb1494415586cfb2ec5638a3ab0f0cea3879e45146a34976740b59fcf48` | One clean normal close after 311.88 s observed gameplay |
| B | Yes | `14e5ba131d32ee1a1902d20c3ce73484377fb60994d5a3976c3c0d9d78abb10d` | First exit 0, timing interrupted by battery sleep; supplemental main-menu close 0 after 85.93 s |

These are separate candidates from the pinned cross-built framework. Build
success does not establish a shutdown fix. No changed binary has been published
or substituted into the frozen installer assets.

Baseline A already exited cleanly without the proposed heap patch. Consequently,
a clean B exit alone cannot establish that patch as the cause of improvement.
The cross-build/MSVC difference is another variable; these observations do not
prove a compiler defect or justify marking either candidate qualified.

The first B exit measurement was interrupted by a critical-battery sleep.
Windows recorded the critical battery trigger and battery sleep at 16:35:29–30,
followed by a later power-source change and Modern Standby exit at 17:09:20.
The held shipping handle ultimately returned 0, but its 2035.43 s wall-clock
close interval includes those power transitions and is excluded from normal
shutdown comparisons. The unchanged B binary was repeated while charging.

The repeat passed its exact-framework module probe and reached the hub. An
attempt to close the temporary On-Screen Keyboard through Task Manager opened
Task Manager at higher Windows integrity than the computer-use helper. Game
activation failed twice with `failed to activate captured window`; the utility's
direct close control also had no effect. After the maintainer closed the utilities,
the recovered game was at the main menu. A continuous hub interval and the
intervening gameplay-to-menu route cannot be established. Its eventual normal
Alt+F4 close from that menu is a supplemental exit observation, not a matched
five-minute A/B gameplay shutdown comparison. No game was force-killed. A hidden
recovery helper holds this exact shipping process and waits for it to close, then
runs the hash-guarded framework removal, original-file restoration and 39-entry
verification. The final exit and recovery results are recorded separately below.

## Preservation requirements

| Work | Why | Windows behavior to preserve |
| --- | --- | --- |
| Framework-only isolation and dump comparison | Determine whether addon/UI/SDK code is necessary for the recorded failure. | Check actual loaded modules and normalized fault fingerprints; a live process or closed window alone is insufficient evidence. |
| Same-compiler A/B candidate experiment | Test a concrete upstream lifetime repair without confusing source changes with compiler changes. | Real D3D12 objects and wrapper objects can have different final-release lifetimes; retain the successful-reset contract while testing cleanup changes. |
| Separate report and recovery transactions | Preserve the original failures and exact-build evidence while exploring a fix. | Require the shipping process to close before restoring files, verify recovery hashes, and retain Windows and Linux CI. Do not roll back player/account saves or publish raw dumps/runtime DLLs. |

After the first two gameplay controls, all 39 baseline entries were verified,
including the original ownership receipt and private bridge. Only the two known
video configuration files needed restoration; EnhancedInput was unchanged.
Player/account saves were not read or rolled back. The A/B experiment uses its
own recovery transaction and requires a final closed-game verification.

All 39 entries were also verified after the battery-interrupted B run, before
staging the repeat. The repeat temporarily quarantined the same 14 known
original files and RenoDX. Its restored recovery records remain private.

After the repeat's actual shipping exit, the guarded recovery helper removed
the temporary framework and restored the original files. All 39 baseline
entries were independently reverified, including the original receipt and
private bridge. Both video configuration files were restored; EnhancedInput
remained unchanged. Player/account saves were not read or rolled back. Task
Manager and the temporary keyboard were closed by the maintainer.

Checks: the sanitized JSON was parsed and checked against the completed private
exit/module records; local documentation links and `git diff --check` passed.
Production source, dependency pins, the successful-Reset patch, frozen assets,
and Windows/Linux CI definitions were unchanged. No new Linux runtime check
was run for these Windows framework controls.

The original public qualification remains FAIL. Supported NVIDIA, HDR,
controller and Linux runtime checks are not provided by these AMD controls.

## Follow-up: caller reconstruction and full-memory diagnostics

Source was fetched again and matched remote
`369865490a586692c2329228a11771b62d3d7380` before this follow-up.
The five original access-violation dumps remain unchanged. Using Microsoft's
debugger with the original exception stack pointer and an explicit instruction
anchor at the decoded following-jump target, shipping RVA `0x8bdcb86`, produces
the same normalized return chain in all five:

`0x8bdcf5a` → `0x8af4f47` → `0x8ad1ae5` → `0x8ad438f` →
`kernel32!BaseThreadInitThunk` → `ntdll!RtlUserThreadStart`.

This is a reconstruction using shipping unwind metadata at the jump target,
not an original automatic unwind through the unnamed fault region. The fault
instruction and following jump do not change the stack pointer; using that
anchor assumes the stack state is compatible with its unwind metadata. The
matching results strengthen the fingerprint but do not identify a source
function or prove the corrupting caller. Stop at the OS thread root; apparent
frames beyond it and the nearest exported `AK` name are not evidence of an
audio failure. One preliminary analysis-only copy changed its context for
inspection; the independently repeated explicit-anchor commands use the
unmodified originals. Neither operation changes a live game or a saved dump.

A signed Microsoft ProcDump 12.01 process-snapshot capture succeeded for the
exact pinned framework alone. Its 4,748,453,489-byte full-memory dump remains
private. It was captured while entering the world; the hub was subsequently
observed. It has no fault context and does not identify the failed object.
Runtime code inspection still lacks usable game symbols and a validated
instruction sequence for the reconstructed callers.

Attaching ProcDump's unhandled-exception monitor to that same live process
produced two reported illegal-instruction notifications followed by shipping
exit code `1000`, four seconds after attachment. No unhandled-exception dump
was written, and no normal close had been requested. This is a separate
instrumentation failure, not the recorded shutdown access violation, a clean
exit, or evidence of a particular protection mechanism. Do not use it as a
qualification result or attach a debugger again just to repeat this observation.

Full Windows Error Reporting dumps require an administrator-configured
per-application registry setting; this session has read-only access to that
location. No registry setting, security permission or elevation was changed.
The original 39-entry baseline was restored and independently verified after
the instrumentation exit before the next experiment was staged.

A private diagnostic addon and external memory-copy helper were then built.
The addon selects only the known read-from-`0x10` exception, `r15 = 0x10`, exact
fault bytes and following jump to the version-pinned shipping RVA. It waits
briefly for the helper to record the original remote exception context, then
returns `EXCEPTION_CONTINUE_SEARCH`; it does not change registers, bypass the
fault or terminate the game. A standalone harness verified full-memory capture
and exception-context preservation. Selection checks verified unrelated
exceptions are ignored and capture is one-shot. The helper's dump request uses
the Windows SDK's four-byte structure packing; default x64 pointer alignment
otherwise produced an invalid-memory error in the harness.

This instrumented framework run adds a variable and is diagnostic only.
Source, binaries, raw memory and runtime logs are kept outside release payloads.
It must be removed by its recorded hash after the shipping process closes,
before original-file restoration. No production fix is established by building
this capture tool. The final run and restoration result is recorded below.

| Diagnostic change | Why | Windows behavior to preserve |
| --- | --- | --- |
| Explicit jump-target unwind anchor on original dumps | The unnamed execution region prevents automatic unwinding; raw stack values alone were inconclusive. | Keep original exception context and stack pointer, disclose the reconstruction assumption, and never label a nearby exported symbol as the failing function. |
| Process snapshot, then narrow exception capture without debugger attachment | The ordinary attached monitor caused a distinct exit before normal shutdown; administrator crash-recorder configuration was unavailable. | Separate instrumented evidence from qualification, preserve the original exception, and keep all dumps private. |
| Four-byte packed dump request and harness | Windows SDK dump structures differ from default x64 pointer alignment. | Retain the SDK ABI layout and verify captured exception registers, rather than trusting a successful file write alone. |

Production addon, build recipe, installer, dependency pins, frozen assets and
both Windows/Linux CI jobs are unchanged. These diagnostics provide no new
Linux, supported-NVIDIA, HDR or controller qualification.

The instrumented run completed 520.73 seconds of observed hub gameplay before
normal Alt+F4. The held shipping handle returned `0xc0000005` after 18.07 seconds;
that interval includes memory capture and must not be compared as ordinary
shutdown latency. The external helper wrote a 2,544,704,879-byte full dump.
Independent parsing confirmed that its original exception context and the
subsequent ordinary Windows minidump match on thread, exception, access address,
`r15`, fault bytes and decoded jump target. The same return reconstruction was
also obtained from this sixth crash event. These are two captures of one event,
not two independent reproductions.

The full dump shows GameThread waiting from the game's own frames inside
`ucrtbase!execute_onexit_table` / `common_exit`. The worker's nearby data and
saved caller pointers can now be read, but their source type and the operation
that invalidated the pointer remain unidentified. This does not establish a
ReShade descriptor-heap destructor, AMD Reflex marker or audio function as the
cause. No register edits or fault suppression were made in the live process.

After actual process exit, the diagnostic addon was moved out of the game load
path only after its SHA-256 matched the recorded test binary. Framework and
original-file recovery then completed; all 39 baseline entries were independently
reverified. Both known video configurations were restored, EnhancedInput was
unchanged, and no player/account saves were read or rolled back. No capture
helper or instrumented game remains running. Raw dumps, logs and diagnostic
binaries remain private. The crash remains unresolved and public qualification
remains FAIL.

Primary tool references:
[Microsoft debugger explicit stack/instruction anchors](https://learn.microsoft.com/en-us/windows-hardware/drivers/debuggercmds/k--kb--kc--kd--kp--kv--display-stack-backtrace-),
[ProcDump capture modes](https://learn.microsoft.com/en-us/sysinternals/downloads/procdump),
[Windows local full-dump configuration](https://learn.microsoft.com/en-us/windows/win32/wer/collecting-user-mode-dumps).
