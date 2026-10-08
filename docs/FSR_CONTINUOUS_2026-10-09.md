# Continuous FSR controller

The experimental controller replaces the bounded FSR probe with continuous
analytical FSR 3.1.5 through the shared settings authority. Native AA, Quality,
Performance and Ultra Performance produced sustained gameplay output on
CachyOS/Proton with an RTX 4080 SUPER. Balanced failed validation and returned
to Native at 100% source scale. It is not qualified yet.

## Integration

- The game thread publishes actual world units, current world generation and
  observed `r.ScreenPercentage`. A saved preference is not an activation ACK.
- The settings worker transports explicit, CRC-checked 128-byte messages in two
  mod-owned slots. Render-side getters use nonblocking snapshots with bounded
  freshness; no file IO runs in those getters.
- Preflight verifies the exact bridge/framework bytes, creates an unrecorded
  SDK context and queries that provider's dimensions and ratio before lowering
  source scale. Activation requires an exact process/intent/world/source ACK and
  the matching render dimensions. Pending phases have a 15-second deadline.
- Evaluation uses the shared current-frame depth, backward motion and exposure
  producer, an independent context and recording leases, and an owned RGBA16F
  output. The original temporal shader is skipped only after successful output
  replacement. There is no trial frame cap or output readback in this path.
- Transient texture-size or timing mismatches skip SDK evaluation, reset Native
  history independently, and reset FSR history on the next valid input. A busy
  snapshot read cannot renew the cached sample's age.
- Provider changes retire recorded GPU work before destroying the context and
  restoring 100% source scale. The game thread confirms that restoration. FSR
  retirement does not shut down NGX.

## Recorded checks

The corrected controller completed at least 1,049 Native AA frames, 5,142
Quality frames, 2,304 Performance frames and 4,357 Ultra Performance frames.
The first version stopped Native AA at 303 frames on a transient size mismatch;
Quality completed 6,750 frames in that earlier version. The correction recovered
and continued Native AA rather than permanently rejecting the request.

Quality used 2560×1440, Performance 1920×1080 and Ultra Performance 1280×720,
all into 3840×2160. The measured source values acknowledged the SDK-derived
commands, including noninteger percentages. Balanced requested 2258×1270 but
failed the frame/camera validation gate with error 4; automatic fallback
restored and acknowledged 100%. Selecting Native after Ultra Performance also
retired FSR and restored 100% without manual console changes.

These are execution and recovery checks, not benchmark or image-quality scores.
The exact hashes and restoration result are in the
[qualification receipt](../qualification/providers/fsr-continuous-2026-10-09.json).
The confirmed menu quit closed the window, but the process lingered and needed
guarded termination before restoration. Shutdown is not qualified; its cause
has not been isolated. All 12 original payload hashes and mod-owned slots were
restored. Private captures, account data, saves, raw logs and vendor binaries
are excluded.

## Build and remaining gates

FSR stays inactive in an ordinary build. An experimental native build requires
`build.py --native-only --fsr-bridge-sha256 HASH`, the separately built MSVC
bridge and official AMD framework under `MCD2Graphics/fsr`, and
`ConsolidatedMenuTransport=1` in the existing provider policy. The controller
checks the bridge build pin and the tested framework hash before loading either.
The same native stack guard and separate SDK acquisition requirements apply.

Windows qualification artifacts use `qualification/providers/current-ui-payload.zip`,
compiled from the normal UI without the private chooser. Its receipt checks all
five UI source files and the metadata builder against canonical Git blobs,
plus the archive and all three payload hashes. Stale source is refused before
writing. The released UI bundle and installer manifest remain unchanged. This
candidate bundle is build evidence and needs its own runtime qualification.

The trial sent choices through the real game-thread menu client using a
temporary mod-owned chooser. The normal Video rows remain on the existing
NVIDIA path. Balanced, Custom, resize/travel, longer sessions, comparative
quality/performance and Windows/AMD runtime need further qualification before
normal FSR menu admission. AMD low-latency hardware qualification and FSR Frame
Generation remain separate unfinished gates. Published installers are unchanged.
