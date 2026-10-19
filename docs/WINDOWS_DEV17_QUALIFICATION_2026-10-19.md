# Windows dev.17 qualification with current dependencies

**FAIL: the full candidate reproduced the intermittent normal-exit access
violation. Wider active-runtime qualification stopped under the checklist crash
rule; this is not release approval.**
The [machine-readable record](../qualification/providers/windows-dev17/qualification.json)
contains the exact installer, manifest, dependency-lock hashes and completed
component controls. Older successful component tests retain their original scope.

The candidate uses native source `cbc34b8d78ba77933ebc491da851acc615b8af55`,
Windows workflow `39191593403`, artifact `12160128654`, and installer source
`e1fa173`. All 46 artifact entries, 242 final installer-folder entries and 17
owned payload files were verified. Normal installation records 18 owned files,
including its private DLSS copy. The self-contained Windows installer was built
in a path containing spaces with .NET SDK 10.0.402. No third-party runtime or
framework binary is included in the committed inputs or bundled payload.

## Current publisher inputs

The publisher audit was performed on 19 October 2026. Archive/member hashes and
primary-source URLs are retained in
[publisher-inputs.json](../qualification/providers/windows-dev17/publisher-inputs.json)
and the complete
[candidate dependency lock](../qualification/providers/windows-dev17/dependencies.lock.json).
These are frozen candidate inputs, rather than permission to accept arbitrary
future files through an override.

| Dependency | Candidate input |
| --- | --- |
| Blueprint Loader | Current Nexus main file 506, version 2.6; official archive and all three members verified |
| NVIDIA Streamline | 2.14.16, complete production x64 set; publisher archive digest and signatures verified |
| NVIDIA DLSS SR | 310.9.16, publisher commit `9981c6350dc58c0ce21f024fd65e31e851c31689`; blob identity and signature verified |
| RenoDX UE Extended | Current snapshot, 0.2026.1008.1941; official asset digest verified |
| AMD FSR | Existing current publisher inputs retained: analytical SR 3.1.5 / FG 3.1.6 in the pinned 4.1.1 package |
| AMD Anti-Lag 2 | Current 2.0.5 headers retained in the source-built bridge |
| ReShade | Exact source-built candidate `47d415dd919854b143e942b203e2b595a932a00d543bdb25c971609f4e17427c`, including successful-Reset and descriptor-heap lifetime patches |

Stock ReShade 6.8.0 was checked as the current public release. It does not replace
the framework interfaces required by this candidate. Its availability is separate
from compatibility with the custom integration. No NVIDIA feature activation is
claimed from an AMD run or from a runtime's version/signature.

The [assembly receipt](../qualification/providers/windows-dev17/assembly-receipt.json)
retains the native artifact provenance. The separate
[dependency update receipt](../qualification/providers/windows-dev17/dependency-update-receipt.json)
records the old/new lock digests while keeping the owned payload unchanged.
The candidate is `0.4.0-amd.dev.17`; historical dev.16 and preview.1 inputs remain
frozen. The production UI still displays `0.3.0-preview.1`, and its description
contains encoding artifacts. Those are open metadata defects.

## Installer and runtime observations

Windows 11 / Radeon 860M driver `32.0.31041.1004`, Steam build `25754144`, shipping
SHA-256 `3a8703406fd50520f83c4f70a0212c000cb3b584ef28eb38032902230c01ebdd`.
The observed output was 1920x1200. Native, FG Off and Anti-Lag Off were used in
the first full-package run. Fourteen loaded game-directory native component
hashes matched the frozen inputs. A menu checkbox does not establish SDR/HDR
output or feature activity.

Normal GUI operations passed: fresh install, duplicate-install refusal, intact
verification, missing-owned-shader repair, modified-owned-shader repair and
uninstall refusal, exact fixture restoration, running-process uninstall write
refusal, and intact uninstall after the shipping process exited. Uninstall
removed all 18 owned/private-DLSS files, restored the original framework proxy,
and retained the 14 verified external files. The disposable Windows installer-core
suite passed all 132 checks. Correct files were restored before every launch.

The five checklist dependency override paths also passed their GUI checks:
ReShade, RenoDX, Loader, DLSS and Streamline each required the explicit checkbox,
accepted the recognized installed set, and changed only that dependency's status.
Clearing each override restored normal recognition. Closing with the final
temporary override selected and reopening reset all overrides to Off. These
checks do not establish unknown future-runtime compatibility. No vendor binary
was modified, and the final installation has an empty override receipt.

The automation helper rejected the native dependency picker as a different
target window. That picker path is **NOT RUN**. Official, hash-verified external
files were prepared separately; normal GUI installation then recorded private
DLSS ownership. This is a disclosed limitation, not a GUI-picker pass. Other
unfinished GUI checks and runtime gates remain listed in the aggregate.

Loader 2.6 reported its version in game. The three advanced tabs and side help
rendered. Mouse changes worked after reopening the page following master
Off/On. The SR selector skipped DLSS on the AMD device and retained the shared
Quality/67% selection across Native/FSR menu changes. These observations do not
establish all keyboard/controller paths or active FSR evaluations in this package.

## Exit failure and component isolation

The first full-package ordinary Quit returned `0xC0000005`, approximately 9.99
seconds from confirmation to the two-second process observer. There was no
observed clock gap, debugger or capture addon. The original exception context
shows a null-adjacent read on an unidentified game worker while GameThread is
in CRT exit. Explicit-anchor offline unwinding recovered the same game return
chain as the earlier fault: `8bf8e9a`, `8b10e87`, `8aeda25`, `8af02cf`. The
nearest exported symbol does not identify the failing function.

Two clean Steam controls exited 0: normal window close from the hub and ordinary
main-menu Quit. A Loader 2.6 plus production-UI-only control also exited 0. A
further control reproduced `0xC0000005` with the original ReShade proxy and
current RenoDX, with all our native addons, bootstrap and UI payload absent.
Its null read was a different instruction in the same function, and the
explicit-anchor return chain matched the full-package fault. The candidate
`d3d12.asi`, Streamline and FSR bridges were not loaded in that control.

These small, differently timed samples exclude our native/UI payload as a
necessary condition for that reproduction. They do not prove a RenoDX,
framework, game or computer-control defect. A further original-framework plus
Loader control, with RenoDX removed, also reproduced the same read from `0x10`
and explicit-anchor return chain. RenoDX is therefore not required for that
reproduction. The exact patched candidate framework by itself, with Loader,
RenoDX and all our payload absent, exited 0 after 86.70 seconds. That different
framework/layout result does not explain the full candidate failure, and its
unexplained delay remains material. Faulting workers had no recorded thread name.
The separate descriptor-heap regression remains fixed; this recurring game AV
must not be described as repaired by that change.

Injected gameplay Escape was also ignored by the completely unmodified game.
Physical Escape worked in an earlier run. Record this automation limitation
separately from mod behavior; do not change gameplay code to accommodate it.
Raw dumps, logs, account/party identifiers and recovery copies remain private.

All diagnostic file moves were restored exactly. The full supported candidate
was then reinstalled through its normal GUI and passed Repair/Verify. All 18
owned files and 15 dependency files matched, with overrides empty. Its normal
Steam launch reached the main menu with Loader 2.6 and all 14 expected loaded
native component hashes. No player/account saves were read or rolled back.

The first post-reinstall startup run's exit code is unavailable: a sample-file
replacement error terminated its observer before the normal Quit request. The
later absence of the process is not counted as exit-code evidence. The private
collector was corrected to retain its held process handle when sample publication
fails and to publish an immutable final exit receipt. A locked/unlocked-file
fixture exercised the failure and recovery paths before an unchanged-candidate
repeat. This collection correction changes no game, addon or vendor bytes.
The repeat's immutable held-process receipt recorded exit code 0, 5.29 seconds
from main-menu Quit confirmation to the two-second observer, without a clock
gap. This was a startup/main-menu check with no gameplay; it does not replace
the earlier failed full-package gameplay exit or establish a shutdown fix.

The remaining active-FSR preset matrix, AMD FG/Anti-Lag advancing counters and
Off recovery, output lifecycle and repeated full-package clean exits are NOT RUN
or FAIL. The [current procedure](AMD_WINDOWS_CANDIDATE_2026-10-12.md) says to stop
on a new unexplained crash, hang or image corruption. Component isolation and
installer restoration do not turn those runtime gates into passes. No overall
Windows pass or exit-crash repair is claimed.

## Follow-up without launching the game

The [offline record](../qualification/providers/windows-dev17/offline-checks.json)
adds nine passing native Windows owned-host probes on the Radeon 860M. These use
the exact candidate bridge, pinned current AMD runtimes and, where specified,
the exact patched framework. Build receipts and checked source hashes connect
the probes to the retained native artifact. Minecraft and Steam were not started
during this pass, and no production source or installed payload was changed.

| Isolated check | Result and scope |
| --- | --- |
| FSR SR, native D3D12 and framework-wrapped device | Both passed all five SDK quality modes with SDR and HDR synthetic inputs: 160 dispatches/readbacks each, correct region values, ten context retirements, no SDK errors/warnings |
| AMD FG bridge, sRGB and PQ contracts | Both passed Off/On/Off, 60 generated frames for 60 enabled real frames, host-recording replay after SDK retirement, and idempotent retirement |
| Presenter switching, sRGB and PQ contracts | Both passed five handoffs, stable COM identity, unsafe-switch refusals, concurrent observation and exact generated counts |
| Resize, sRGB and PQ contracts | Both passed four handoffs, private data preservation, exact generated counts and retirement with FG still enabled; repeated with disposable `LOCALAPPDATA` |
| Framework-wrapped resize diagnostic | Passed all four handoffs and FG-active retirement using unchanged production DLLs; a private probe variation explicitly loads the framework as `d3d12.asi` and verifies device wrapping |

An initial framework resize comparison put ReShade at `dxgi.dll`, a path reserved
for the mod bootstrap in the supported installation. The bridge then required a
live worker settings snapshot that the isolated host did not supply, refused the
handoff, and the probe returned 46. This was not an access violation. The corrected
explicit-load diagnostic passed; the initial comparison is retained as a test
layout limitation, not a production regression or a full-bootstrap pass.

The original resize hosts also used the real mod-only provider-store directory.
Their synthetic selections were restored to the sole record matching the prior
recorded revision/checksum, preserving every field the host did not overwrite.
Subsequent resize and framework diagnostics redirected `LOCALAPPDATA` to their
own disposable directories and preserved the restored record's hash. Player and
account saves were neither read nor restored. Future native runs of these hosts
must redirect `LOCALAPPDATA` before execution.

Fresh CPU checks passed 32 Python tests, 37 actual-source NGX shutdown ABI and
ownership checks, 14 FSR ownership/destructor cases, all 128 shared SDK retirement
combinations, real NT DLL-unload/process-termination detection, concurrent
bootstrap trace teardown, process-lifetime/borrow fixtures, and the FG capability
cache/protocol fixture. The disposable installer-core suite passed all 132 checks.
Initial command/setup mistakes and their corrected successful invocations are
disclosed in the offline record. All 18 installed owned files, 15 dependencies,
242 installer entries and 17 payload hashes were reverified; overrides remain
empty.

Offline dump analysis recovered the game function ranges around the existing
anchored return chain, but protected/dynamic code and unavailable private symbols
still prevent a defensible worker identity. No additional shutdown patch is
justified by this evidence. The prior failed game-exit gate remains unchanged.
These probes establish synthetic SDK/component behavior, not active game SR/FG,
Anti-Lag cadence, physical HDR output, visual correctness, performance or clean
game shutdown. Runtime qualification remains failed/incomplete.
