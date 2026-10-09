# Native anti-aliasing with AMD Frame Generation candidate

This experimental path adds a continuous guide consumer independent of any
upscaler. It converts the verified game's depth and packed motion at its
pre-temporal boundary and leaves the native temporal shader and output intact.
It does not create/evaluate NGX or FSR SR, lower source scale, copy the temporal
output, take pixel readbacks, wait for the GPU, or impose a trial frame limit.
The ordinary build keeps it disabled; the tracked FG candidate recipe enables
it explicitly and records the source and camera hashes.

## Admission and ownership

The implemented pairing is Native AA / AMD FG / single generated frame.
NVIDIA FG with Native AA remains unqualified and excluded. FG Off avoids guide
allocation and conversion. Successful SDK/device capability checks remain
separate from the saved provider preference.

Conversion requires a matching current shared authority/session/world, a ready
context observed within three seconds, full source scale, known world units,
validated temporal bindings/formats, a full-resolution active viewport and the
same proxy device. Stale or invalid input skips generation without replacing
native output. A frame-stamp gap, camera reset, world change or menu interruption
resets FG history. Overlapping SR ownership is rejected.

Its generation and immutable descriptor/recording cache are separate from both
SR providers. Successful command-list Reset/replacement or destruction must
invalidate every recording before a subsequent graphics-queue fence allows
borrow/resource retirement. Wrong queue, removed device or failed signal retains
resources. The producer never resets a game-owned list to force cleanup. AMD
FG's SDK work retains the private recording contract introduced in PR40.

The Video menu preserves AMD FG selection when choosing Native, while excluding
unimplemented NVIDIA pairings. Source-matched UI/metadata is included in the
experimental build receipt. Anti-Lag still needs actual Windows/Radeon driver
support and the owned-presenter handshake; selecting this pair cannot supply
those capabilities.

## Checks

The local 16 KiB guarded native candidate compiles. The 59 focused source/build
checks, 21 FG recipe checks, provider menu/runtime fixtures, standalone admission
and projection checks, Anti-Lag controller checks, and 41 foliage lease checks
pass. Native admission includes mismatched session/checksum/world, future/stale
observations, wrong source scale/units, unsupported providers and FG Off.
Both Windows and Linux CI include the admission executable and existing SR/FG
checks. The historical rejection of Native/AMD is updated with an additional
missing-presenter-handshake test.

## Bounded Linux runtime

CachyOS / RTX 4080 SUPER / driver 615.78.08 / CachyOS Proton SLR
11.0-20261005 ran game 1.1.2.0 (Steam build 25754144) at 3840 × 2160.
The transaction replaced only mod files and backed up mod settings. Account and
player saves were not edited. Raw captures and logs remain private.

The first candidate addon SHA-256 was
`a210cfa7726fb32034d3dfaa255ee33b053948db965a8011df578156c95025a7`.
Native / AMD FG Off entered gameplay at full source scale with zero SR
evaluations. Enabling FG through Video requested a restart; it did not claim
activation before that restart. The ordinary menu quit completed in 3.785 s.

After restart, Native / AMD FG On used presentation owner 2, current ready
full-resolution world context and 3840 × 2160 guides. Prepared/images/generated
counts reached 4890/4890/4890 with SDK result 0, no errors and no warnings.
FSR evaluations remained zero. During a 48.0144 s pause, generated frames stayed
at 2726 while real presents advanced from 31321 to 37741. After resume,
generated frames advanced to 4809 and activation resumed. The normal menu quit
completed in 3.935 s, with AMD SDK retirement result 0. Process disappearance
was observed; the process exit code was not captured and no signal was sent.

The Windows build exposed duplicate camera header copies accepted by GCC but
redefined by Clang. The recipe now references one canonical camera header path.
This changes source closure, not the intended rendering algorithm. The rebuilt
addon has its own SHA-256 and is checked separately; the first run's binary
qualification is not transferred automatically.

The canonical-header rebuild SHA-256 is
`1f2d4b0fa3b190baef721b415e4da60acf1be2278a807893cee78b515bf4f12d`.
A separate launch entered Native / AMD FG gameplay with full-resolution current
guides: generated counts advanced from 681 to 1581 in 21.5232 s, while FSR
evaluations remained zero and SDK errors/warnings remained zero. Final
prepared/images/generated counts were 1662/1662/1662. Normal menu shutdown
completed in 2.501 s with SDK retirement result 0; again no exit code was
captured or signal sent. Original mod files and preferences were restored byte
for byte after this run. The measurements establish activation and bounded
lifecycle behavior, not delivered FPS or visual quality.

No native Windows/Radeon runtime, long-session, performance, latency, artifact
quality, arbitrary world-unit, resize or multi-frame claim is established here.
