# FSR in the native Video menu — 9 October 2026

Development integration, enabled by the consolidated-menu startup policy and an
exact FSR bridge pin. Published installers, tags and manifest hashes are unchanged.

## Controls and authority

The ordinary Upscaler row now offers Native, NVIDIA DLSS and AMD FSR. Each
provider retains its own preset and custom scale. FSR uses Native AA at 100%;
its named resolutions come from the active SDK query, with Auto displayed until
that result is available. NVIDIA retains its existing DLAA and percentage aliases.
The slider uses the current provider's bounds rather than borrowing NVIDIA ratios.

Mouse and keyboard callbacks submit one versioned, complete request. The settings
worker remains the only preference authority. HDR, Reflex and FG edits use that
same transaction; old save slots are compatibility projections and source ACKs.
The NVIDIA renderer consumes the committed authority with its existing history
reset, adapter eligibility, 15-second timeout and independent GPU retirement.

Current-process and actor-world acknowledgements must agree before admitting the
shared controls. A stopped, corrupt or stale authority cannot reactivate old
preferences. Previously admitted source rollback can still restore 100% while
settings are unavailable, for both NVIDIA and FSR. Other callbacks remain inert.

The normal menu currently admits NVIDIA FG only with NVIDIA SR. Choosing Native
or FSR disables FG in the same transaction and hides that row. This is a temporary
qualification boundary; the underlying independent SR/FG preferences are retained.
AMD low latency and AMD FG remain separate implementation/qualification work.

## Bounded runtime evidence

Steam build 25754144, game 1.1.2.0, CachyOS Proton SLR, native WineWayland HDR and
RTX 4080 SUPER. The trial uses the real Video rows, without the diagnostic chooser.
Its structured, sanitized receipt is
[fsr-video-menu-2026-10-09.json](../qualification/providers/fsr-video-menu-2026-10-09.json).

- NVIDIA Quality and DLAA reached matching legacy renderer Active ACKs through
  the committed shared settings projection.
- FSR Quality evaluated at 2560×1440 → 3840×2160.
- FSR Balanced evaluated at active 2259×1271 → 3840×2160, preserving the separately
  handled padded allocation fixed by PR29.
- Switching FSR → Native restored measured screen percentage to 100%, acknowledged
  the restoration and left both reconstruction controllers inactive.
- The final source-matched build saved FSR from the main-menu Video page, remained
  at 100% there, then applied it on actor travel into gameplay. Native AA also
  evaluated at 3840×2160 through the same ordinary controls.

These are control/SDK execution checks, not image-quality, FPS or physical-latency
measurements. No player/account save, raw log or screenshot is included.

## Checks and remaining scope

The exact C# clients pass signed CRC, real request coalescing, independent provider
preferences, immutable retries, stale actor/session rejection and Native rollback
fixtures. Portable C++ checks cover canonical NVIDIA projection, source ACK,
transport, retirement and existing timing ownership. The native addon and complete
NeoRune UI rebuild pass locally; the current UI qualification archive contains
only our source-matched menu payload. Windows CI must build the changed sources.

The first ordinary-menu trial remained alive after normal quit exceeded 120
seconds. It was terminated before exact restoration. This does not isolate the
cause; shutdown remains unqualified and older control observations are retained.
Windows/AMD execution, physical-controller input, missing-runtime/authority live
faults, extended travel, image comparisons, performance and FG coexistence need
separate checks before release. No broad platform or latency benefit is claimed.
