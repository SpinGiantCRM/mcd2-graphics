# Windows AMD regression results for PR6

The exact GCC candidate supplied with PR6 passed Windows installation, startup,
AMD fallback, settings controls, menu travel and reinstallation on 5 October
2026. An extended probe observed 1,027.5 seconds of survival with both addons
loaded and all 901 samples responsive. The full Windows regression is not yet
qualified: the first shutdown eventually returned exit code 0, but the second
shutdown crashed with `0xc0000005` after Native mode was restored.
The third PR6 run exited normally. This clean repeat does not erase the crash.

No renderer, NGX lifecycle, UI, shader or installer changes were made during
this regression test. The original Windows preservation requirements remain in
[WINDOWS_COMPATIBILITY.md](WINDOWS_COMPATIBILITY.md). Completed checks apply only
to the exact candidate below; they do not qualify shutdown or NVIDIA execution
on this AMD laptop.

## Exact artifact and environment

- Candidate: `0.1.0-preview.1-linux-regression.1`.
- PR source tested: `eed6924b6b1a27835f7cf4df6c79fae295ec3ccc`.
- ZIP SHA-256: `6ec967e049341daaf94b3e70b47deec27ed1cf2d75a9164f0749d057fb66542b`.
- Addon SHA-256: `611b49abee32db2705d47e5a9e08dd676cd7e892cb7da7e563eab4c18e733240`.
- Main compiler: MinGW GCC 16.2.0; MSVC ABI bridges: Clang 23.1.1.
- Windows 11 Pro 10.0.26200 x64; AMD Radeon 860M driver 32.0.31041.1004.
- Steam build 25647713; output 1920 by 1200.
- Existing pinned ReShade, RenoDX, Blueprint Loader and external DLSS runtime
  dependencies were verified and retained.

The ZIP matches the handoff hash. Its public source files match the PR source
tree after newline normalization, and the installed addon hash was independently
verified. Native renderer, NGX, UI and shader source match merged PR5. The
original preview.1 tag and assets remain frozen; the separate LLVM build below
was never substituted for this GCC runtime payload.

## Installation and preservation

Replacement of the previous Windows candidate preserved all 31 files in the
preinstallation save/config/dependency inventory byte for byte. All six
installed mod/runtime hashes and the candidate marker version matched.
Uninstalling while the actual game was running was refused, and the six hashes
still matched afterward.

After gameplay and eventual exit, a fresh inventory contained 32 protected
files. Duplicate installation was refused; uninstall removed all six owned
files and the marker; reinstall and check passed. Every protected file remained
byte for byte identical throughout this cycle. The fresh inventory matters
because gameplay legitimately updates saves. The repository's ignored
`package/` payload now matches the supplied candidate manifest.

Seven isolated filesystem gates using the supplied ZIP passed: missing
dependency refusal before writes, fresh hashes, duplicate refusal, modified
owned-file retention, dependency/unrelated-mod preservation, reinstall and
unsafe-path rejection. Modified-file retention was tested in the fixture, not
by deliberately corrupting the live installation.

## Startup and AMD fallback

The startup probe observed 138.2 seconds without an exit. The extended probe
then observed 1,027.5 seconds with MCD2 and RenoDX loaded and all 901 samples
responsive. The session included hub gameplay and mouse movement, settings
changes, and world → main menu → world. It was not 17 minutes of continuous
active dungeon play or a completed dungeon.

| Transition | Current request evidence |
| --- | --- |
| Native to Quality | Revision 38, mode 2, exact two-thirds requested scale; Phase 3 / Error 1; AMD vendor 4098 declined and acknowledged in the same recorded frame |
| Quality to DLAA | Revision 39, mode 1, 100% scale; Phase 3 / Error 1; visible native fallback message |
| DLAA to Quality | Revision 40, mode 2, two-thirds scale; Phase 3 / Error 1 |
| Slider to Custom | Revision 41, 77% scale, Custom preset; Phase 3 / Error 1; UI and decoded intent agree |
| Quality to Native | Revision 45, mode 0, 100% effective scale; Phase 2 / Error 0; source and runtime revisions match |
| Quality in main menu | Revision 51, saved Quality, context not ready, Phase 2 / Error 0; UI says the saved selection activates in gameplay |
| Reentered world | Revision 53, saved Quality, context ready, Phase 3 / Error 1; current sessions match and AMD is declined |
| Restart after reinstall | Revision 58, saved Quality, context ready, Phase 3 / Error 1; current sessions match; source revision remains the preceding Native acknowledgement, 56 |

Quality displayed 67% while requesting exactly two-thirds. DLAA selected 100%.
Slider clicks selected Custom at 77% and 98%; cycling presets restored
DLAA/100% and Quality/67%. The slider's own 100% endpoint was not independently
qualified. Both pause and main-menu Video pages remained usable. Changing from
mouse to injected keyboard input moved focus to the first native row, consistent
with the documented input-device caveat. Opening the gameplay menu required
human keyboard input because the game's injected I/Escape input was ignored.

The Video page visibly reported **DLSS unavailable; native anti-aliasing is
active.** No NGX feature was created on AMD. Rejected requests did not receive a
reduced-source acknowledgement; the source revision stayed at the preceding
Native acknowledgement. The unchanged UI fallback handler requests
`r.ScreenPercentage 100`. GPU source-resource dimensions were not independently
measured, so this establishes fallback control behavior, not a pixel-dimension
measurement or working DLSS.

Reinstall preserved the Quality preference. The restarted process survived
71.1 seconds with both addons loaded and all samples responsive, then reached
gameplay and current Quality fallback again. All six installed hashes matched.

## Shutdown observation

Save and quit closed the first game window, but Windows `tasklist` continued to
report the Shipping process. The installer correctly refused changes during
that interval. A `Get-Process` check briefly failed to find it while `tasklist`
and the process inventory still did; absence from that one check was not proof
of completed shutdown.

At 11:21:57 Adelaide time, Steam recorded exit code 0 for both Shipping and its
launcher, and `tasklist` then confirmed absence. Neither process was forcibly
terminated. The delay was roughly two minutes; the first quit was not timed
precisely enough for a stronger latency claim. Its cause and whether PR6 changes
the latency were not established.

After reinstall, Quality fallback applied again. Restoring Native reached
revision 59, Phase 2 / Error 0, with matching current source/runtime revisions
and sessions. Save and quit was initiated at 11:28:14.341 Adelaide time. Windows
Application Error recorded `0xc0000005` at 11:28:18; Steam recorded exit code
`-1073741819` at 11:28:24. Process absence therefore did not constitute a normal
exit pass. The private dump shows a read from address `0x10` in an executable
code region outside the dump's named module ranges; it does not establish a
fault in the addon or a compiler-specific cause.

The prior Windows addon, SHA-256
`645de6fcba098eeaaef4b571323f8fbd63a0c5a5b355288fac642c5b3028b112`,
was installed temporarily as a control. Replacement preserved 33 protected
files. It survived 69.8 seconds with both addons loaded, reached gameplay,
acknowledged Quality fallback at revision 65 and Native at revision 66, then
completed Save and quit with exit code 0. The bounded exit observation confirmed
absence at 20.4 seconds after confirmation; this is an observation bound, not an
exact shutdown duration. The control started with Native saved, whereas the
crashing PR6 run started with Quality saved. One clean control does not prove a
compiler cause or isolate the difference in initial state.

PR6 was restored with its matching installer and all six hashes verified;
34 protected files remained byte for byte identical. No native source was
changed on the basis of this inconclusive crash evidence.

The restored PR6 candidate was then repeated from a saved Native preference,
matching the control's initial preference. It survived a 71.4-second probe with
both addons loaded. Quality reached current fallback at revision 72; Native
reached revision 73, Phase 2 / Error 0, with matching source/runtime revisions
and sessions. Save and quit was initiated at 11:56:04.829 Adelaide time. Steam
recorded exit code 0; process absence was confirmed within a 17.8-second
observation bound, without forced termination. The exact PR6 candidate remains
installed, Native is saved, and the game is closed.

PR6 therefore produced two normal exits and one access-violation exit across
three runs. The previous Windows candidate produced one normal control exit.
These small, partly different sessions do not establish a regression rate or
identify the cause. Windows shutdown remains unqualified.

Linux's separately recorded lingering-process exit remains unqualified. Neither
candidate's Windows shutdown observations resolve its cause.

## Build checks and preserved Windows requirements

Thirteen installer/build-recipe unit tests passed locally on Windows. Settings
framing rejected all 2,502 truncations and passed the mailbox check; 2,312
settings/runtime semantic checks and eight adapter/deadline assertions passed.
Both Windows and Linux CI jobs passed for the tested PR source in
[run 37247159009](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37247159009).

The Windows native-only recipe passed with LLVM MinGW 20260922 and its 16 KiB
frame guard. That separate, uninstalled addon has SHA-256
`4a4033d03fda10a7e2013c6e2916fab20976fcdc16197d7c25db0c3c8cf3f1b5`.
This verifies the recipe, not NVIDIA runtime compatibility of that rebuild.

| PR6 change | Reason and Windows requirement retained | Regression evidence |
| --- | --- | --- |
| Compiler-specific frame-error flags in `build_toolchain.py` / `build.py` | GCC rejects Clang's flag spelling. Both compilers must still reject frames above 16 KiB so the original Windows render-thread stack overflow cannot return. | Six recipe tests, Windows native-only build, bounded startup with the exact GCC candidate |
| Lowercase Windows include in copied ReShade headers | Linux case sensitivity blocked the rebuild. Normalize only the copy and retain direct platform inclusion; a `Windows.h` alias would recurse on Windows' case-insensitive filesystem. | Normalization tests and successful Windows compilation; no shim restored |
| Separate GCC candidate manifest | The Linux NGX switch regression passed with a different compiler artifact. Same source does not imply same runtime behavior. Preserve both MSVC ABI bridges, `_fltused`, heap buffers and adapter/deadline policy. | Exact ZIP/addon hashes, source comparison, Windows AMD runtime and existing Linux NVIDIA evidence |
| Recipe checks in both CI jobs | Linux checks miss Windows process-output and path behavior; Windows AMD checks cannot exercise NVIDIA lifecycle. | Both CI jobs retained and passed; 13 local unit tests and seven filesystem gates |
| This report and updated compatibility/handoff notes | Record exact artifact and platform evidence without overwriting historical qualification or claiming an untested GPU passed. | Separate sanitized record; Linux exit limitation and Windows NVIDIA qualification remain explicit |

## Remaining qualification

Windows NVIDIA feature creation, evaluation and NGX teardown require an RTX
host. Repeat Quality ↔ DLAA, DLSS ↔ Native, travel and exit with this exact GCC
payload. An LLVM rebuild requires independent qualification. No physical
controller, HDR recalibration, complete dungeon, long session, device recreation
or deliberately induced pending-source timeout was qualified here.

This test does not merge PR6, promote a release or update Nexus. Private
snapshots, raw receipts and screenshots remain under ignored
`dist/validation/pr6`. The public
[sanitized record](windows-regression-pr6-2026-10-05.json) contains no saves,
account data, private file inventory or third-party runtime DLLs.
