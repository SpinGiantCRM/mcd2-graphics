# Windows AMD qualification: updated-game dev.25 progress

**Qualification is incomplete. No exit-crash repair or release approval is claimed.**
The fifth ordinary process reproduced `C0000005` on normal close. Shutdown
qualification therefore remains **FAIL**, despite three earlier clean exits.
Steam build **25910487**, shipping SHA-256
`3842102bb05fc532b22e97992d61c2248d115be80ff752ff9b6843bf4acd7005`,
is a separate baseline from the previous 25754144 results. Old address-pinned
crash probes remain retired. This is an interim record, not the final checklist.

The exact candidate is `0.4.0-amd.dev.25`, requested source
`e704a17c59e69ad4ec04191f43b0928ee0ad1097`, native artifact source
`b5719f95820acb97dcd5fcdfa60bdde291b3379a`, identical complete source tree
`775604d9599c7ae1862eaeeb878f31c611cdcc65`. Native workflow
[39774607102](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/39774607102)
and portable Windows/Linux workflow
[39774607284](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/39774607284)
passed. Artifact `12424387023` has archive SHA-256
`49ae3e0eb63992eb8d161648623d24536ad1c2f52ca0cf72eded432450ee4476`.
The freshly assembled installer SHA-256 is
`d1772ed3ce8cb8f141bd6cf9a1bf7734da11482eb58f915f3a32d37542bf9774`.

The fresh GUI install recognizes the updated executable, uses current publisher
dependency defaults with overrides off, and verifies all **18 owned / 15
dependency files**. Inputs include Loader 2.6/file 506, Streamline 2.14.16,
DLSS 310.9.16, the pinned FSR SDK 2.3.0 runtimes and RenoDX snapshot
`0.2026.1021.0108` / SHA-256
`48fa2f37d7ccfda97e3e69d5e7c1e4df272fd4aaf9978d98540416c18ce93aa4`.
The framework remains a separately selected dependency, SHA-256
`9fdac099b390ff2d88d4cf6d51685f9991ea7484e6a70fe0b092019d7adddba2`.
Loaded libraries do not establish ML-model or HDR activation.

## Completed bounded checks

| Gate | Result | Observation |
|---|---|---|
| Fresh install | PASS | Normal dev.23 uninstall, then exact dev.25 GUI install; overrides empty. |
| Intact verification | PASS | GUI verifies correct files without mutation. |
| Missing owned shader repair | PASS | Exact manifest bytes restored. |
| Modified owned shader protection | PASS | Repair and confirmed uninstall refuse without mutation; exact original restored before launch. |
| Running-game writes | PASS | Install and confirmed uninstall refuse; intact Repair performs only its read-only scan. All files and receipt unchanged. |
| Dependency override controls | PASS | All seven acknowledgement / Use installed paths tested individually; the other six cards stay recognized. Each returned Off; normal reopening defaults all seven Off. No override installation performed. |
| Main-menu Loader metadata | PASS | Loader 2.6 and mod version dev.25, name, author and description observed. Loader-owned update-status text still clips. |
| Main-menu Video layout | PASS, mouse scope | Numeric render scale, preset/custom synchronization, shared preset and unavailable-DLSS skipping observed; keyboard coverage remains incomplete. |
| Updated engine layout | PASS | Actual registry validation and marker registration accepted build 25910487. |
| FSR / FG / Anti-Lag activity | PASS | Two ordinary pre-capture/current-process intervals validate same-session/revision/world advancing counters, with no provider errors. |
| Ordinary full-candidate exits | FAIL | The fifth ordinary process returned C0000005. Earlier clean exits took about 7.49, 9.48 and 7.60 seconds, and a later clean exit took about 88.00 seconds. Those observations do not establish a repair or pass the final exit series. |
| Active SR/FG resize recovery | PASS, bounded | Windowed 1680x1050, maximized 1920x1177, then restored borderless 1920x1200; fresh current-context SR and generated-frame counters resumed with no provider error. |
| Physical HDR | NOT AVAILABLE | Current Windows display diagnostic explicitly reports HDR not supported / HdrSupported 0 on the internal 1920x1200 60 Hz panel. Wide-color capability is separate and does not establish HDR support. |
| Physical controller | NOT AVAILABLE | No connected controller in any of four XInput slots; the Windows input report lists mouse, keyboard and two HIDI2C devices. |

## Functional follow-up — 23 October

The Native-started third ordinary process reached the hub with FG and Anti-Lag
Off. In Video, changing Quality to Balanced kept Native selected and displayed
59%. The upscaler arrows cycled Native → FSR → Native, skipping DLSS on this
Radeon and retaining Balanced. Mouse edits to Custom 77%, the displayed minimum
33%, and Native AA 100% synchronized the preset, numeric value and fill without
selecting an upscaler. Help explained the minimum. The first keyboard-arrow
input moved focus to Display rather than editing the slider; keyboard slider
coverage remains incomplete.

The real UI selected Custom 77% and FSR, then the production configuration store
requested the other named presets with full-record ownership checks. Strict
current-session/revision/world observations validated advancing SR evaluations
at Custom 77%, Native AA 100%, Quality, Balanced, Performance and Ultra
Performance. Measured dimensions included 1479x924, 1280x800, 1130x706, 960x600
and 640x400 for 1920x1200 output. Transient Applying or mismatched transition
samples were excluded until validated Active. These are activity and resolution
checks, not performance measurements.

All three Advanced categories displayed exclusive selection and their side
help. Changing FSR FG from analytical to explicit unsupported ML left the SR
choice unchanged; changing SR to experimental INT8 left the FG choice unchanged.
The separate choices survived menu changes. An ordinary restart confirmed the
explicit INT8 SR preference persisted. Its separately acquired community DLL
was absent, as permitted by the [external-runtime procedure](ADVANCED_SETTINGS.md#fsr-versions).
The CRC-valid runtime record resolved to Fallback/error 23, zero evaluations and
100% source scale, with a matching current-session context. It did not substitute
analytical SR or reduce source resolution. INT8 activation is **NOT AVAILABLE**
for this installed dependency set. Stable fallback records do not advance their
file timestamp; the strict activity observer therefore refused them as stale,
and was not weakened to claim activation.

The third and fourth processes exited normally after about 9.48 and 7.60
seconds. Their full installed-file hashes and receipt remained unchanged.
No new Application crash event was found for the third process. Ordinary exit
observations remain separate from the diagnostic failures below.

The fifth ordinary process verified Auto selecting analytical SR 3.1.5, with
61 advancing evaluations at Quality, while explicit unsupported ML FG produced
zero generated frames. Its held process handle returned `C0000005` on normal
Alt+F4, without a debugger, verifier or clock gap. The AMD presenter route had
been created although FG was unavailable. This narrows a useful next control;
it does not establish that the advanced choice caused the fault. All installed
hashes still matched afterward. The close marker predates the input by a
commentary interval, so it supplies only an upper timing bound for this run.

The Loader Open Mods mouse shortcut opens responsive Settings. Gameplay Map
inputs still failed through the control helper, including an admitted temporary
mouse binding. Original Interact and Map bindings were restored and verified
together. This does not qualify pause-menu entry, travel or Save and quit. No
physical controller was detected. A briefly retained Video recommendation on
the Loader list was observed during navigation; it disappeared on subsequent
entry and is retained as a cosmetic observation.

Machine: Windows 11 Pro build 26200, Radeon 860M, driver 32.0.32015.7029,
1920x1200 SDR output. The first strict active interval advanced SR/FG-generated/
Anti-Lag by **77/30/47**; the next ordinary process advanced **39/40/39**.
Analytical SR 3.1.5 and FG 3.1.6 were reported. These counters are not physical
display FPS, frame-time tails, image-quality or input-latency measurements.
FG correctly suspends while the game is not foreground.

## Crash investigation remains separate

An image-specific Heaps run with verified verifier modules reached the hub and
returned `C0000005` on normal close. ProcDump reported no access-violation event;
that recorder log alone did not identify a faulting instruction. Its exact IFEO
absence was restored, and all installed hashes were rechecked. A later, non-verifier run
returned `1000` shortly after debugger attachment, before a close request.
Neither diagnostic result is substituted for an ordinary-release exit result.

Windows Error Reporting independently retained minidumps for both the Heaps run
and the fifth ordinary run. Offline examination recovered their actual fault
instructions and independently anchored the return chain using each captured
instruction's direct jump and the updated executable's unwind metadata. Both
reach the same worker functions; one reads through a null-derived address and
the other through an unavailable heap address. Their main threads are in the
same CRT-exit wait path. This establishes a shared worker family, not an owning
component, allocation/free history or a source-level repair. No old live probe
was retargeted and no game debugger was attached for this analysis.

The sixth ordinary process used analytical SR/FG and exercised display changes.
Windowed output was 1680x1050 with 1120x700 rendering; maximizing produced
1920x1177 with 1280x785 rendering. Validated SR/FG deltas were 83/32 and 42/41.
Restoring the original borderless mode produced 1280x800 to 1920x1200 and
advancing SR/FG again. Anti-Lag remained Off and is not counted as active.
An initial single click on the title-bar maximize button did not resize the
window; only the subsequent normal title-bar double-click is counted.
The normal hub Alt+F4 returned 0, with about 88.00 seconds from the immediate
input marker to the held monitor's completed exit observation, without a clock
gap. The slow clean exit does not repair the preceding ordinary crash.

Afterward, normal GUI uninstall removed the owned payload and private DLSS
copy, restored the previous framework proxy and retained the current shared
dependencies. A hash-guarded private transaction temporarily held all remaining
Loader containers and shared native runtime entry points outside game load
paths. The first clean updated-game control reached the hub; its actual loaded
modules contained system DXGI and no mod proxy, addon, Loader or SDK modules.
The helper's Escape input was also ignored there. This supports a control-helper
coverage limitation independent of the mod. Its normal close returned 0 after
at least 90 seconds observed in the hub. A second clean control likewise used
system DXGI without mod modules, remained in the hub for at least 90 seconds
and exited 0. Close-to-observed-exit intervals were about 6.30 and 4.83 seconds.
These controls do not establish that the base game can never crash. All 15
temporarily held shared dependency/entrypoint files were restored to their exact
hashes before reinstalling the candidate.
The normal GUI reinstall and intact Verify succeeded; all 18 owned and 15
dependency files, empty override map and original installed receipt matched.
Final preference restoration and the end-of-qualification uninstall remain open.

A smaller updated-game comparison then retained only publisher Loader 2.6 and
the pre-existing ReShade 6.8.0.2155 proxy (SHA-256
`0cee63f9c9f13f3ac909c5b4903f4dbb4b719a7ab3b4f13b0deaf83c814b94f7`).
The normal GUI uninstall removed our owned payload, and all remaining native
graphics dependencies, including the candidate framework, RenoDX and FSR /
Streamline, were held outside load paths. Actual loaded modules confirmed that
reduced state. Both ordinary runs reached the same hub for at least 90 seconds.
The first normal window close returned 0; the second returned `C0000005`.
Its WER dump independently anchors the same updated-image worker and CRT-exit
wait path as the full candidate failure. Our UI, native code and graphics
providers are therefore not necessary for this crash. This does not yet identify
the responsible remaining component or exonerate the base game. No debugger or
verifier was attached; a read-only Windows display report completed before the
second close. The temporarily held files were restored before preparing the
next, separately recorded Loader-only control.

The completed eight-run [component matrix](../qualification/providers/windows-dev25/component-controls-2026-10-23.json)
keeps the current game, warm Steam, observed hub dwell of at least 90 seconds and
normal window-close route. All runs used held process handles without a debugger
or verifier. Timings below end at completed exit observation and include input
dispatch and monitor polling; they are not intrinsic shutdown-duration samples.

| Configuration | Held exit codes | Close to observed exit, seconds |
|---|---|---|
| Clean game | 0, 0 | 6.30, 4.83 |
| Original framework plus Loader 2.6 | 0, C0000005 | 7.44, 9.28 |
| Loader 2.6 only, system DXGI | 0, 0 | 5.80, 6.12 |
| Original framework only, no Loader | 0, 0 | 8.64, 6.02 |

The failure with the combination is a bounded observation, not proof of an
interaction defect. Neither clean pair exonerates its components or the game.
The useful conclusion is that the matching worker failure occurs without our
graphics payload. All four file transactions were exactly restored. Normal GUI
reinstall and Verify then succeeded, with the original receipt, all 18 owned /
15 dependency hashes and empty overrides matching again. Provider preferences
and remaining test bindings still await final restoration; shutdown remains
FAIL. No further unchanged component repeats are planned for this matrix.

Microsoft's signed TTD 1.11.617.0 recorder passed owned clean and freed-read
program controls, plus attach/stop followed by natural process completion.
Offline CDB replay recovered the owned allocation, successful free and subsequent
read of the same address in order, using the documented
[memory](https://learn.microsoft.com/en-us/windows-hardware/drivers/debuggercmds/time-travel-debugging-memory-objects)
and [heap](https://learn.microsoft.com/en-us/windows-hardware/drivers/debuggercmds/time-travel-debugging-heap-objects)
queries. The exact bounded capture wrapper also passed owned clean and explicit
failure-status controls with initialization confirmed, trace files retained,
correct target exit codes and drained recorders. This qualifies that diagnostic
method for a targeted experiment, not the game or candidate.

The subsequent bounded game recording used Native with FG and Anti-Lag Off and
returned 0 on normal window close. It initialized and drained successfully, and
offline indexing succeeded. However, the recorder reported 705 unsupported
instructions in game code. Saved exception contexts independently match the
updated worker family, while the relevant memory-history and termination-call
queries do not expose the required shutdown ordering. Successful fixture replay
therefore does not establish equivalent coverage of this game. The recording
does not identify the owner or demonstrate a crash repair, and its clean exit
is excluded from ordinary qualification. No identical recapture is planned.

The subsequent [full-memory heap diagnostic](../qualification/providers/windows-dev25/heap-exit-2026-10-23.json)
reproduced `C0000005` on normal starting-menu window close, with Native, FG Off
and Anti-Lag Off. Image-specific Application Verifier Heaps and Windows Error
Reporting produced a complete 7,964,025,703-byte dump without an attached
debugger. The actual invalid read targets a block independently classified as
freed. Its retained freeing stack is a game-owned CRT exit callback; that
callback's game static slot still contains the exact pointer consumed by the
worker. This confirms a use-after-free for this diagnostic instance. Earlier
ordinary faults grouped by worker family are consistent with this boundary,
but their grouping alone does not prove an identical target in every case.

The owned capture controls had recovered faults and freeing stacks, but failed
their original requirement to retain both allocation and free history. Only
that narrower demonstrated diagnostic capability was accepted for this one
game run. The actual game dump likewise lacks the freed target's allocation
history. Source ownership of the protected worker and complete stop/join and
callback-registration ordering remain unresolved; distant export labels and
display-enumeration allocation frames do not identify a class or component.
This evidence does not justify a patch in our addon. No identical capture is
planned. All temporary per-game diagnostic settings were restored, and exact
18-owned / 15-dependency hashes and the receipt matched afterward. The run is
excluded from ordinary qualification; Windows remains **UNQUALIFIED**.

The private capture wrapper could cancel its recorder immediately when the
target signaled exit, racing a termination dump. It now allows bounded debugger
drain first. The exact corrected wrapper passes owned clean and explicit
`ExitProcess(C0000005)` controls: one termination dump each, preserved exit
codes and completed debugger drain. This fixes capture tooling; it is not a
game-crash repair. Dumps and proprietary runtime details remain private.

Remaining checks include keyboard and remaining advanced/diagnostics paths,
gameplay and travel, resize/ownership restoration,
final uninstall/reinstall, and ordinary shutdown regression. NVIDIA activation
is unavailable on this Radeon-only machine. Physical controller/HDR coverage
must be reported independently. Historical shutdown failures remain disclosed;
one clean current-build exit does not establish their cause or repair.
