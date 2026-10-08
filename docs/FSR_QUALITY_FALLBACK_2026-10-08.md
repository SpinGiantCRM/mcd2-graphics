# FSR Quality output and rejection fallback

Analytical FSR 3.1.5 reconstructed **2560×1440 to 3840×2160** for 256 gameplay
frames on CachyOS/Proton with an RTX 4080 SUPER. This extends the
[full-resolution output gate](FSR_SUSTAINED_OUTPUT_2026-10-08.md); the ordinary
mod and published installers still have no supported FSR control.

## SDK preset dimensions

The Microsoft-ABI bridge now exposes `mcd2_fsr_quality_v1`, a fixed-width
40-byte plain-C result. Both ratio and render dimensions are queried through
the **selected context**, preserving the verified analytical provider identity.
Invalid modes, dimensions or returned values fail without publishing a result.
The sustained recipe can require an exact named mode before creating its output.

The runtime returned these dimensions for a 3840×2160 display:

| FSR mode | Ratio | Render dimensions |
| --- | ---: | --- |
| Native AA | 1 | 3840×2160 |
| Quality | 1.5 | 2560×1440 |
| Balanced | 1.7 | 2258×1270 |
| Performance | 2 | 1920×1080 |
| Ultra Performance | 3 | 1280×720 |

These are measured SDK results, not a universal lookup shared with NVIDIA.
In particular, Balanced differs from the existing DLSS 58% mapping. AMD's
[integration guide](https://gpuopen.com/manuals/fsr_sdk/techniques/super-resolution-upscaler/)
documents provider-specific scaling modes. Game rounding, allocated texture
sizes and active subrects still need handling before broader preset support.

## Rendering and fallback results

| Trial | Successful evaluations / output replacements | Result |
| --- | ---: | --- |
| Quality gameplay | 256 / 256 | One FSR reset, one Native reset; both output readbacks finite and nonblack |
| Mid-run bridge rejection | 128 / 128 | Attempt 129 rejected with `-1011`; Native history reset before returning from that callback |
| Full-resolution input with Quality required | 0 / 0 | Preset mismatch rejected with `-1104`; no evaluation or output replacement |
| Request accidentally armed in main menu | 0 / 0 | Input-format guard rejected it before SDK context creation |

All four contexts/generations retired and SDK destruction returned zero. The
three created SDK contexts reported zero errors/warnings. NGX was absent and
NVIDIA FG was Off. Quality's 256 UE view stamps were consecutive; the rejection
trial's 129 observed stamps were also consecutive. In the rejection trial, the
Native reset occurred at the next presentation sequence after replacement 128.

Previously, an unsuccessful frame could return to ordinary Native dispatch
before resetting FSR-derived history. Every failure now routes through the
independent reset immediately. This includes early source/format/clock errors,
conversion, dispatch and output-copy rejection. A valid Native pass can reset
even when an FSR-only view binding fails. If reset bindings/resources are not
available, the trial retains dirty history and suppresses that Native dispatch
until a reset can be recorded; it does not release the generation prematurely.

The rejection is deterministic **bridge validation**, not an induced failure
inside AMD's GPU kernels: the private recipe invalidates its own plain-C size
field after 128 successes, and the bridge refuses it before calling the SDK.
Device removal, an SDK-internal partial failure and physically destroyed bindings
remain unqualified. Early-binding routes have generation guards, not a claim of
runtime device-loss coverage.

## Source scale and limits

A temporary NeoRune game-thread applier acknowledged 66.666664% and verified
`WorldToMeters=100`. After the trial, an explicit test-controller request restored
100% and its acknowledgement was read back. **Automatic source-scale rollback is
not implemented by this trial.** That belongs to the shared menu/runtime
transaction, alongside truthful requested/effective status.

Both 4K output readbacks held finite RGB for all 8,294,400 pixels. Their maxima
were 404 and 336.75 in scene-linear units; these are not nits or quality scores.
No performance or visual-quality claim follows from these bounded diagnostic
runs. The earlier main-menu attempt is retained as a refusal result, not counted
as gameplay evidence. Gameplay arming subsequently required a current scale
acknowledgement.

The exact binaries and sanitized measurements are in the
[qualification receipt](../qualification/providers/fsr-quality-2026-10-08.json).
After the final normal close request, the game process exited. Twelve original
payloads and six mod-owned state slots were restored and verified; temporary
SDK files, UI and the private control slot were removed. Raw logs, screenshots,
textures and saves are excluded.

## Build and next gate

The separate developer recipe accepts `--quality-mode 1` for Quality and
`--fail-after 128` for the rejection test. Its matching bridge is required.
Zero disables rejection; the ordinary native build never consumes the private
request file. Eight generation/source guards run on both CI platforms, and the
Windows recipe compiles the separate Quality/rejection variant with the existing
16 KiB native stack guard. Compilation is not active Windows/AMD qualification.

Next is the shared settings/UI and runtime-reader cutover, including automatic
source rollback, stable provider switching and normal FSR activation. Preset
rounding, resize/travel, longer sessions, particles/foliage and comparative
quality/performance remain open. AMD low-latency hardware qualification and AMD
FG presentation integration are separate unfinished gates.
