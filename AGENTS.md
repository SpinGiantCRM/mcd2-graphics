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

In-game diagnostics must be based on advancing provider counters, with bounded
freshness and current session/revision/world validation. Keep the opt-in request
separate from advanced preset persistence. Versions and requested overrides are
not active-model readbacks. The FG override uses the documented Streamline UI
recomposition setting; preserve Auto and driver-override caveats. Its separate
advanced slots use the same replay/source validation and may change only the FG
INI key. Use the shared-runtime latency recipe for the FG candidate, including
its version-library link; the standalone latency build is not a substitute.
Upscaling Sharpness uses one post-upscale game filter for both providers. Restore
its prior value only while still owned, yield to external changes, and retain
mouse, keyboard and controller paths with a 0-100 percent bound.

The advanced page has separate Upscaling, Frame Generation and Diagnostics tabs
with persistent side help. Preserve Loader-owned keys and the independent FSR
SR/FG slots. Explicit portable FSR 4.1 INT8 and ML FG selections must never be
reported as analytical fallback, and ML FG still needs actual device approval.
DLSS FG C uses driver DRS; never map it to A/B recomposition or edit a global/
shared NVIDIA profile. Preserve original-setting receipts, ownership checks and
Auto restoration, plus the distinction between an accepted request and a model
readback. Native Windows qualification is required for the Windows DRS path.
