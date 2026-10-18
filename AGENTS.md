# MCD2 Graphics working agreements

Preserve the native in-game Video menu, visual parity except intended shadows,
safe Native fallback, and Linux/Proton and Windows/AMD compatibility. Check the
current roadmap and overlapping PRs before changing priorities or support claims.

Preserve dirty work. Use [the check map](docs/agent-efficiency/v1/CONTEXT.md) and
[task templates](docs/agent-efficiency/v1/TASKS.md) for scoped validation.
Game launches, installed-mod changes, disruptive local builds, Actions, pushes,
merges, releases and system-wide changes need authorization for that action.
Continue already-authorized work without asking again.

# Cross-platform changes

Read [docs/WINDOWS_COMPATIBILITY.md](docs/WINDOWS_COMPATIBILITY.md) before changing
the native addon, build recipe, installer or their tests. It records why the
Windows fixes exist and the behavior that must survive Linux changes.

Preserve the fixes and their regression checks when refactoring. A Linux build
or NVIDIA runtime pass does not by itself establish Windows / AMD compatibility.
Run the available relevant checks, retain both Windows and Linux CI jobs, and
record runtime checks that could not be run. If an alternative replaces a fix,
update the compatibility note to explain how it preserves the same requirement.

Keep the original preview.1 tag/assets frozen. Rebuilt payload hashes belong to
their own candidate manifest and validation record. Never include player saves,
account data, private runtime logs or third-party runtime DLLs in releases.

The maintainer authorized one exception on 6 October 2026: the exact tested,
source-built PR435 ReShade framework may be offered as a separate dependency
download, with its source, build provenance and license. Keep it outside the
mod installer archives. NVIDIA runtimes remain external dependencies.

## Evidence

Separate source/fixture checks, exact-package runtime evidence and player
observations. Saved choices and SDK/CI success do not prove active rendering,
visual parity, clean process exit or another platform's qualification. Record
unrun gates and exact artifact identities; retain failed controls. Keep public
support replies short and private diagnostics out of public artifacts.
