# Installer edition and selection correction

Related: [issue #8](https://github.com/SpinGiantCRM/mcd2-graphics/issues/8).

The report's screenshot shows the Xbox/Game Pass layout under
`Content/Dungeons/Binaries/WinGDK`. The released installer expects
`Dungeons/Binaries/Win64/Dungeons-Win64-Shipping.exe` and the executable hash
pinned in `dependencies.lock.json`. A folder-selection change cannot qualify
different game code, shader/resource layouts or Reflex hooks.

## Proposed correction

- State the Steam Win64 requirement on the first page and in installation docs.
- Recognize the WinGDK directory and explain that this edition is unsupported.
- Add an executable picker alongside the folder picker.
- Normalize the supported installation root, known project/binary folders and
  an optional `Content` wrapper. Do not search drives or arbitrary parent games.
- Clear the previous game selection when a new selection fails.
- Retain executable fingerprints, dependency hashes, ownership, path containment
  and linked-file rejection. Recognition grants no WinGDK installation support.

This changes installer selection/messages and documentation only. There are no
native addon, game offset, renderer, payload or pinned dependency changes.
The existing `0.2.0-rc.1` tag and downloads remain unchanged.

## Qualification

Local Linux: all 56 .NET 10.0.401 core checks pass, including root/project/binary/
executable selection, Content-wrapper selection, unchanged hash refusal, all
reported WinGDK selection forms, no writes for unsupported layouts, missing/
unrelated/file selection and symbolic-link refusal. Existing install, repair,
rollback, uninstall, dependency ownership and HDR-bootstrap tests also pass.
The Avalonia GUI compiles with locked dependencies and zero warnings/errors.

The existing Windows and Linux guided-installer CI jobs are retained. Windows
runtime folder/executable picker checks remain necessary before publishing a
rebuilt installer; local Linux success does not qualify Windows UI behavior.
Linux link tests are skipped on Windows to avoid requiring symlink privileges.

## Xbox follow-up

The reporter was asked for the shipping executable filename, SHA-256 and game
version, excluding private paths, executable uploads, saves and raw logs. The
Xbox compatibility report remains open pending that evidence and qualification.
Do not describe this correction as Xbox support or close that report as fixed.
