# Rebuilt AMD candidate benchmark

**Qualification remains incomplete.** The fresh SDR FG-Off session returned
`0xc0000409` on quit. Its cause is not established. A completed window close
and successful benchmark do not clear that failed process exit.

Candidate `0.4.0-amd.dev.8` uses the complete source-matched Windows artifact
from run 38333151314 / artifact 11783652643, including the corrected NVIDIA
permanent-retirement restart status. All 44 receipt members, 41 FG source hashes
and six UI source hashes were checked. The separately selected rebuilt ReShade
binary was tested here; the previous framework's runtime evidence was not reused.

## Method and measurements

Linux / RTX 4080 SUPER / Proton SLR. FSR analytical 3.1.5 Quality at 2560×1440
source and 3840×2160 output; analytical 3.1.6 AMD FG. VSync Off, frame cap None,
Reflex On. The stationary hub remained live: lighting, wind and NPCs varied.
Startup warmed for 45 seconds after route acknowledgement, and actual Video-menu
changes settled for 15 seconds. Three captures per phase contained 1,199
application Present intervals each. The same temporary CPU sampler remained
installed throughout both primary comparisons.

The [numeric receipt](../qualification/providers/amd-pr54-rebuilt-benchmark-2026-10-13.json)
retains all thirty captures and the thresholds registered before measurement.
Medians:

| Output | Phase | Application presents/s | SDK real + generated/s | p99 ms |
| --- | --- | ---: | ---: | ---: |
| HDR | Fresh Off | 137.34 | — | 8.79 |
| HDR | On A | 92.04 | 184.08 | 12.72 |
| HDR | Retained Off A | 131.10 | — | 9.11 |
| HDR | On B | 92.04 | 184.08 | 12.72 |
| HDR | Retained Off B | 131.64 | — | 9.14 |
| SDR | Fresh Off | 141.27 | — | 8.67 |
| SDR | On A | 94.42 | 188.84 | 12.35 |
| SDR | Retained Off A | 134.75 | — | 8.91 |
| SDR | On B | 94.26 | 188.53 | 12.07 |
| SDR | Retained Off B | 135.21 | — | 8.91 |

The registered median throughput, tail-time, memory, power and temperature gates
passed in both modes. SDK-counted total throughput rose about 33.4–34.0%, while
rendered throughput fell about 33.0–33.3%. Retained Off recovered to 95.4–95.9%
of fresh Off; it is not cost-free. Median VRAM increases were 682–1,002 MiB.

All twelve On captures counted one generated frame per real present. Activation
was observed at capture boundaries, not continuously. These are CPU application
intervals and SDK counts, not physical display FPS or input latency. GPU pass
timings and moving-image quality were not measured. This is not a deterministic
replay or a provider ranking.

## Exit results and isolation

| Session | Actual held-process exit code | Confirmation to disappearance |
| --- | ---: | ---: |
| HDR fresh Off | 0 | 4.08 s |
| HDR repeated On/Off | 0 | 4.68 s |
| SDR fresh Off | `0xc0000409` | 3.30 s |
| SDR repeated On/Off | 0 | 3.66 s |

The SDR failure occurred with startup owner 0, so it does not establish an
active AMD presenter as its cause. The sampler was installed; that fact is not
proof of sampler causation. No-sampler controls are recorded separately rather
than replacing or discarding the failed run.

The [separate no-sampler controls](../qualification/providers/amd-pr54-rebuilt-no-sampler-2026-10-13.json)
used the same complete payload and framework in SDR. Fresh Off remained alive
for at least 300 seconds, then exited 0 in 3.25 seconds. An owned AMD session
passed On/Off/On/Off, with generation stopped while Off and resumed while On;
it exited 0 in 3.87 seconds. Scene GPU completion and shared SDK retirement
passed, and original files/configuration were restored. These controls narrow
the investigation; they do not establish sampler causation or erase the failure.

Each primary comparison restored original files, own mod settings, renderer
configuration and calibration byte for byte, removing the sampler. The HDR
addon was held outside the game for SDR and returned before restoration.
Player/account saves were not copied or edited. Raw logs and screenshots remain
private. Native Windows/Radeon, active Anti-Lag 2 and broader qualification remain
open; published releases are unchanged.
