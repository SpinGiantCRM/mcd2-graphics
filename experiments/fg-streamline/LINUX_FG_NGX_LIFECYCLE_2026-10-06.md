# Shared NGX lifecycle diagnosis — 6 October 2026

SR preset retirement called device-wide NGX shutdown while Streamline retained its FG feature. The internal evaluation failed with `0xbad00004` (`FeatureNotFound`), but the public SDK status remained 0 and reported two presents. The apparent increase from about 65 to 76 rendered FPS was therefore invalid.

Removing only the shutdown call eliminated the discrepancy. The corrected implementation verifies Streamline ownership of the same wrapper device, releases the SR feature/parameters after the existing GPU retirement fence, and defers device-wide NGX shutdown to Streamline. Missing/mismatched shared ownership retains the generation. Unshared SR cleanup remains unchanged.

NVIDIA documents [device-wide shutdown semantics](https://github.com/NVIDIA/DLSS/blob/main/include/nvsdk_ngx.h) and the [FeatureNotFound result](https://github.com/NVIDIA/DLSS/blob/main/include/nvsdk_ngx_defs.h).

## Corrected measurements

RTX 4080 SUPER / 615.71.09; CachyOS Proton SLR; native WineWayland HDR; 3840×2160 output; stationary outdoor hub.

| Condition | Rendered FPS | SDK total FPS | Samples |
| --- | ---: | ---: | ---: |
| Release DLAA | 84.8 | 84.8 | 2 |
| Release Quality | 126.0 | 126.0 | 2 |
| Candidate DLAA Off / fresh | 82.7 | 82.7 | 4 |
| Candidate DLAA Off / returned | 83.4 | 83.4 | 2 |
| Candidate DLAA On / fresh | 64.9 | 129.9 | 2 |
| Candidate DLAA On / returned | 65.6 | 131.2 | 2 |
| Candidate Quality Off | 125.7 | 125.7 | 2 |
| Candidate Quality On | 87.2 | 174.4 | 2 |

Each sample contains 1,199 intervals. Current source dimensions, settings acknowledgements and continuous SDK outcome sequences were verified. The corrected FG On session had zero internal NGX evaluation failures through DLAA → Quality → DLAA and exited normally; SR retired before Streamline performed final NGX shutdown. The sampler performs no GPU queries or extra SDK state queries.

**Performance qualification remains incomplete:** DLAA with FG Off remains roughly 2% below the exact release recheck. Quality Off is essentially unchanged. FG On adds about 3.5 ms per rendered frame in this scene. The historical higher returned-DLAA and Quality FG rates are withdrawn.

These are short Linux measurements. SDK counts do not measure physical monitor cadence or input latency. Animated scene content prevents identical-frame comparisons. Windows, broad transitions, image quality and long-session coverage remain unqualified. No release was changed.

[Numeric record and exact component hashes](LINUX_FG_NGX_LIFECYCLE_2026-10-06.json).
