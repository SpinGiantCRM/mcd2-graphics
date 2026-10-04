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
