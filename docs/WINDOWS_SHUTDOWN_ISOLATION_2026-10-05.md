# Remaining Windows shutdown isolation

Release blocker: one normal Save and Quit on the exact PR6 candidate ended in
`0xc0000005`. Two other PR6 exits and one previous-addon control exited normally.
The existing observations do not establish a compiler, native-addon, UI,
dependency or game cause. Do not replace the artifact to test this matrix.

## Keep fixed

Use the PR6 test ZIP already attached to the PR; addon SHA-256
`611b49abee32db2705d47e5a9e08dd676cd7e892cb7da7e563eab4c18e733240`.
Keep game build, driver and dependencies fixed. Do not rebuild with LLVM, add
Reflex, remove loader dependencies, or change rendering settings between
controls. No changes to authentication or character saves.

## Short first matrix

Run each row twice initially, alternating candidate/control where practical.
Use normal UI choices and wait for current source/runtime acknowledgements.
If a failure reproduces, stop broad runs and capture its private diagnostic
evidence; expand that exact scenario rather than treating repetitions as a
statistical compatibility guarantee.

| Row | Installed configuration | Initial saved preference | Actions before normal Save and Quit |
| --- | --- | --- | --- |
| A | Exact PR6, complete mod | Quality | Reach AMD Unsupported fallback; select Native and wait for Applied / Error 0. |
| B | Exact PR6, complete mod | Native | Reach gameplay; Quality fallback, then Native Applied / Error 0. |
| C | PR6 native addon held outside the game's scanned addon folders; UI payload and other dependencies retained | Native | Reach gameplay and quit without requesting DLSS. Confirm MCD2 native addon absent and RenoDX present. |
| D, if needed | All MCD2-owned UI/native/conversion payloads removed with its matching installer; external dependencies retained | Native | Reach gameplay and quit. This distinguishes the complete mod from a native-addon-only control. |

Close the Shipping process before every file change. Back up the exact addon
before holding it, then restore it and verify its hash. Never leave a second
`.addon64` copy in a scanned folder. Keep configuration/character saves and
unrelated mods intact. Restoring the Native preference uses the game UI, not
editing a character or authentication save.

## Evidence required per run

1. Record exact installed hashes and which addons are actually loaded.
2. Record initial preference and current requested/source/runtime revision,
   matching session checks, phase/error and confirmation that gameplay was reached.
3. Obtain a live `System.Diagnostics.Process` handle before quitting. Retain it
   through shutdown and use `WaitForExit(timeout)` and `ExitCode`. A missing
   `Get-Process` query or closed window is insufficient. Time from confirmed quit;
   otherwise explicitly label the elapsed interval as an observation bound.
4. Allow a bounded 180 seconds. Record a timeout as a failure/limitation before
   any separately authorized stale-process termination. Do not count a killed
   process as a normal exit.
5. On failure, retain crash event, dump, loaded-module ranges and relevant
   addon/loader logs privately. Inspect the faulting instruction and executable
   region owner; the old dump's unnamed executable region is not proof that the
   fault occurred in MCD2 or NGX.
6. Publish only a compact sanitized result: scenario, OS/GPU/driver, binary hash,
   addon presence, acknowledgement result, exit code, bounded elapsed time and
   module/offset if actually established. No raw dumps, logs, saves or account data.

## Interpretation

- Failure in A but not B suggests a scenario difference worth isolating; it
  does not by itself establish a stale-preference or compiler cause.
- Failure in C means the native addon is not required for that reproduced fault.
- Failure in D means MCD2's complete payload is not required for that fault;
  investigate the retained dependency/game stack with further controls.
- Failure only with the complete candidate requires a targeted correction and
  fresh Windows/Linux regression. More successful runs alone do not explain
  the earlier access violation.
- AMD fallback tests do not qualify Windows NVIDIA execution. Linux NVIDIA
  results should remain explicitly scoped to the tested CachyOS stack.

Publication stays on hold until the access violation is corrected or supported
isolation establishes a separate cause and documents the remaining limitation.
