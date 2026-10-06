# Linux FG performance — 6 October 2026

RTX 4080 SUPER / driver 615.71.09; CachyOS Proton SLR; native WineWayland HDR; 3840×2160 output. Stationary outdoor hub with the same camera and character position.

The baseline is the exact seven-file `v0.2.0-rc.1` payload. The candidate uses the matching FG, latency, SR and UI hashes in [the measurement record](LINUX_FG_PERFORMANCE_2026-10-06.json).

| Mode | Rendered FPS | SDK presentation FPS | Change from release |
| --- | ---: | ---: | ---: |
| Released DLAA | 84.6 | 84.6 | reference |
| Candidate DLAA / FG Off | 83.8 | 83.8 | -1.0% rendered |
| Candidate DLAA / FG On / fresh launch | 65.2 | 130.5 | +54.2% including FG |
| Candidate DLAA / FG On / return from Quality | 76.1 | 152.1 | +79.8% including FG |
| Released Quality | 126.5 | 126.5 | reference |
| Candidate Quality / FG Off | 125.2 | 125.2 | -1.0% rendered |
| Candidate Quality / FG On | 106.3 | 212.5 | +68.0% including FG |

Each row is a median of three samples of 1,199 present intervals, except final Quality Off, which includes all six samples from two batches. Current-process settings, render dimensions and acknowledgements were checked before and after each sample. All FG samples reported SDK status 0 and a continuous cached outcome sequence. The sampler makes no GPU queries, resource copies or additional SDK status calls.

Fresh DLAA with FG has a 22.9% rendered-FPS cost; Quality has a 16.0% cost. DLAA after returning from Quality has a lower cost (10.1%). That startup-dependent difference is recorded separately; its cause is not established. Five Quality/DLAA transitions resumed FG with matching source dimensions. Both candidate launches exited normally and completed generation cleanup.

FG Off is within 1% of this release baseline in the measured scene. FG On increases SDK-reported total FPS by 54% in fresh DLAA and 68% in Quality. These counts are not a measurement of physical monitor cadence. See [NVIDIA’s reporting guidance](https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS_G.md#9-dlss-g-state).

This is a short Linux performance check. Animated NPCs and lighting prevent identical-frame comparisons. It does not qualify Windows, physical latency, generated-frame image quality, every game area or long sessions. No published release was modified.
