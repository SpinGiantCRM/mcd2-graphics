# AMD Frame Generation: SDR scene capture and output contract

Development implementation and bounded Linux/NVIDIA evidence. This does not
qualify native Windows/Radeon, wider output lifecycle or a release.
The [numeric receipt](../qualification/providers/amd-sdr-implementation-2026-10-12.json)
retains the failed exit and later controls separately.

## Scene boundary

The pinned Steam build's SDR tone-map draw (`0x088c6818`) writes RGB to its primary
backbuffer and also binds a second render target. Requiring a single bound target
incorrectly rejected this draw. The original indexed draw executes exactly once,
with all arguments preserved. Its primary backbuffer is then copied into owned
staging storage; an owned shader preserves RGB and supplies opaque alpha. The
second target, backbuffer content and render state remain intact.

Full output coverage, known blend/write masks, the pinned shader, primary
backbuffer, actual presentation encoding and a validated View uniform are
required. Native render passes defer the copy until a stored ending boundary;
any intervening draw cancels that deferred snapshot. This alternate pass case
has not been exercised in gameplay: the observed draw used no native render pass.

The View buffer is pixel constant buffer `b1`. Its frame stamp at byte 2644 is
also present in the independently recorded temporal inputs. The scene draw can
be recorded before the temporal callback, so a timing marker or immediate lookup
cannot safely associate it. The snapshot is retained by stamped frame and joined
at Present only after submission, matching frame ID, camera vectors, unjittered
projection, previous transform and appropriately scaled jitter. SDK scene staging
checks actual device and texture identity. No nearest-frame approximation is used.

## Output and ownership

The bridge creates its generation context from actual swapchain dimensions,
format and ReShade colour space. SDR uses the sRGB transfer contract; PQ uses the
HDR contract. A 10-bit backbuffer alone does not enable HDR. Unqualified scRGB,
HLG, formats or scene stages are refused.

Changing output disables/drains SDK callbacks, obtains fresh queue completion,
invalidates owned private recordings, releases their borrows and recreates the
generation context while retaining the same presenter. Game-owned lists are
never reset for retirement. Live output changes still need qualification.

Scene snapshots own their staging texture, opaque-alpha image, descriptors and
pipeline. Host recording leases must retire independently of GPU completion.
The framework's reset event occurs before native Reset: its successful-reset
private-data stamp is now required before retiring a scene lease. Destruction
uses a separate callback. Present holds a strong scene-resource borrow across
concurrent output retirement. Unproven ordinary-unload retirement retains and
pins implementing modules rather than permitting hot reload.

## Evidence and limits

- Owned GPU readback: 153 pixels in each R10, RGBA8 and BGRA8 case; zero RGB-code
  differences, zero alpha errors, confirmed cleanup. The source denied shader
  resource access, exercising the separate owned staging path.
- Isolated sRGB SDK host: Off / On / Off, expected generated counts, no SDK
  faults/errors/warnings, normal exit. This is not game qualification.
- In-game 3840×2160 SDR with FSR Quality at 2560×1440: consecutive admitted
  intervals recorded 60 real, 60 generated, 60 prepared and 60 image inputs,
  with zero SDK faults/errors/warnings.
- Pause stopped generation while real presents continued; gameplay resumed it.
- The first active capture exited with `0xc0000409`, despite retirement receipts.
  After the reset-lease correction, two bounded launches exited 0 and recorded
  scene GPU retirement with zero remaining leases and successful shared SDK
  retirement. This does not establish the earlier failure's cause or erase it.

These used private Microsoft-ABI guide/bridge builds with the unchanged pinned
framework and earlier qualified SR/UI payload. The latest guide includes the
strong Present borrow; the game trials' bridge came from the earlier ABI-matching
build, so they are not qualification of a complete freshly rebuilt artifact.
The UI source removes the NVIDIA-only HDR requirement for AMD FG; a rebuilt UI
must still be checked. No benchmark or physical scanout/latency claim is made.

Original mod files, mod preferences, renderer INIs and external calibration INI
were restored and verified. Only mod-owned save slots were backed up. No player
or account saves, raw logs, screenshots, shader extracts or vendor runtimes are
included here. Published releases are unchanged.
