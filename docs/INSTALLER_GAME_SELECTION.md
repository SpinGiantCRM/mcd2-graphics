# Game selection in the integration installer

This describes the installer under development in PR #60. It does not change the
published dev.25 installer or establish Game Pass runtime support.

Choose **Browse folder…** or **Choose game executable…**. Supported selections are:

- The installation root, or its single `Content` wrapper.
- `Dungeons`, `Dungeons/Binaries`, or the edition's `Win64` / `WinGDK` directory.
- `Dungeons-Win64-Shipping.exe` or `Dungeons-WinGDK-Shipping.exe` in its expected binary directory.

The installer checks only these known locations. It does not search arbitrary
parents or crawl a drive. Selecting an executable or binary directory identifies
that edition; a root containing both editions requires an explicit choice.
Unrelated executables, symbolic links and linked path components are refused.

On Linux, selecting Game Pass also asks for that installation's existing
Xodus-managed Wine prefix. Cancelling this second picker leaves no game selected.
Cancelling the initial folder/executable picker keeps the current selection.
A failed replacement selection clears the previous game and dependency choices,
so installation cannot accidentally continue against the previous location.

Path recognition does not authorize installation. The edition's executable must
still match the package's SHA-256 pin, dependencies must pass their own checks,
and the game must be closed. Game Pass installation remains blocked until its
exact executable pin is supplied. No changes to WindowsApps permissions, Gaming
Services, save data or the managed Xodus runtime are made by selection.

Core filesystem cases and GUI compilation are checked on Linux and Windows.
Native interactive picker/cancel checks remain part of integration qualification.
