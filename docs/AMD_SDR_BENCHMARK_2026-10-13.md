# AMD FG SDR benchmark and shutdown failure

Development candidate `0.4.0-amd.dev.7`, using the complete PR54 artifact pinned
in [output qualification](AMD_OUTPUT_QUALIFICATION_2026-10-13.md).
This is a Linux/NVIDIA measurement, not Windows/Radeon qualification.

## Method

FSR analytical 3.1.5 Quality rendered at 2560×1440 into 3840×2160 SDR.
The existing HDR addon was temporarily removed. VSync and the frame cap were
off; Reflex was On. Anti-Lag 2 is unavailable on this host.
The character remained stationary in the hub, but lighting, wind and NPCs
continued to change. The comparison is sequential, not an identical-frame replay.

Three captures per phase each contained 1,199 application Present intervals.
Startup warmed for 45 seconds after route acknowledgement; menu changes settled
for 15 seconds. The same temporary CPU sampler was installed throughout.
All captures and the thresholds registered before measurement are retained in
the [numeric receipt](../qualification/providers/amd-pr54-sdr-benchmark-2026-10-13.json).
The sampler submits no GPU work or pixel readbacks. GPU telemetry is separate
from pass timing.

## Results

Medians of three captures:

| Phase | Application presents/s | SDK real + generated/s | p95 (ms) | p99 (ms) |
| --- | ---: | ---: | ---: | ---: |
| Fresh Off startup | 142.56 | — | 7.99 | 8.59 |
| On A | 94.43 | 188.87 | 11.54 | 11.97 |
| Off A, presenter retained | 134.29 | — | 8.45 | 8.97 |
| On B | 94.53 | 189.06 | 11.63 | 12.17 |
| Off B, presenter retained | 135.41 | — | 8.40 | 8.90 |

On reduced application throughput by about 33.7–33.8%, while SDK-counted total
throughput increased by about 32.5–32.6%. Retained Off recovered to 94.2–95.0%
of fresh Off; it is not identical to a clean startup without the presenter.
Median memory increases were 822–964 MiB. All registered throughput, tail,
memory, power and temperature gates passed. No captures were trimmed.

SDK totals count composition callbacks; **physical display FPS, image quality,
GPU pass timings and input latency were not measured**. These numbers do not
establish a display pacing or latency benefit.

## Shutdown failure

The fresh-Off process exited normally with code 0. After the final retained-Off
phase, Save and quit to desktop left the game on a black screen with its process
still alive. The preserved task names include `RenderThread 0` and `RHIThread`.
An earlier exact-name check incorrectly reported the render thread absent;
neither the corrected names nor their wait metadata identify the blocked call.
The guide log contained no closing retirement event.
The user authorized terminating only that verified hung shipping process.
Forced termination is not a normal exit pass.

The earlier complete-artifact normal exits remain valid bounded controls, but
this new hang blocks candidate qualification. The temporary sampler has not
been established as the cause. An unchanged full-artifact control without that
sampler repeated On → Off → On → Off through the Video menu and exited normally
with code 0 in 3.56 seconds. Scene recording leases retired and shared SDK cleanup
returned 0. This shorter control is not a duration-matched reproduction and does
not identify the cause. Original files, own mod settings and renderer
configuration were restored byte for byte after each session.
Player/account saves were not copied or edited. Private traces and screenshots
are not attached.

Native Windows/Radeon, active Anti-Lag 2, wider lifecycle and moving-image checks
remain open. Published release payloads and tags are unchanged.

## Longer diagnostic reproduction

The [separate diagnostic receipt](../qualification/providers/amd-pr54-long-diagnostic-2026-10-13.json)
records an On → Off → On → Off run with the same CPU sampler and a private
guide rebuild that logs shutdown boundaries. It adds no GPU commands, but it
is a different guide binary and cannot qualify the unchanged production archive.

The same shipping process remained alive for at least 30 minutes. Generation
prepared 61,766 frames with zero SDK faults/errors/warnings, exceeding the active
work count in the earlier hang evidence. All twelve diagnostic captures are
retained. Final Off stopped generated counts while ordinary FSR rendering
continued; a stale FG-only frame stamp while Off does not prove rendering stopped.

The recorded menu confirmation was followed by actual exit code 0 in 3.36 seconds.
The trace reached AMD presenter retirement, scene GPU cleanup, shared SDK shutdown,
queue destruction and addon cleanup. Recorded retirement results were 0, with
zero retained scene recording leases. Original files and own settings were
restored byte for byte. This longer run did not reproduce the hang and does not
identify its cause or clear the candidate's outstanding qualification gates.
