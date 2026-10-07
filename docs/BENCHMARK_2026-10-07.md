# Final Linux FG benchmark — 7 October 2026

Exact release installer input: `e421e57e7667f0e9ec71e1940c6daa352a39f0c7`, CI [37602995068](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37602995068). No runtime rebuild for publication.

RTX 4080 SUPER / driver 615.71.09, CachyOS KDE Wayland, CachyOS Proton SLR, 3840×2160 native HDR, VSync Off, no FPS limit. Custom graphics, 420-nit peak and 203-nit paper/UI white held fixed. Same stationary hub position near fire/fountain.

| Mode | FG | Rendered FPS | SDK-reported presentation FPS |
| --- | --- | ---: | ---: |
| DLAA | Off | 81.80 | 81.80 |
| DLAA | On | 61.06 | 122.11 |
| Quality | Off | 122.74 | 122.74 |
| Quality | On | 83.76 | 167.52 |

Quality uses 2560×1440 input; DLAA uses 3840×2160. Each entry averages three 30-second runs. On generated exactly one additional frame per rendered frame in every retained interval, with successful SDK status and continuous cached-outcome sequence. SR/FG request and runtime sessions/revisions were checked before and after each sample.

- **DLAA:** FG changes rendered FPS by -25.4%, with 49.3% more SDK-reported presentations. Fresh rc.1 control: 83.43 FPS; final FG Off: 81.80 FPS (-1.9%).
- **Quality:** FG changes rendered FPS by -31.8%, with 36.5% more SDK-reported presentations. Fresh rc.1 control: 125.15 FPS; final FG Off: 122.74 FPS (-1.9%).

## Method and limits

A temporary CPU-only addon sampled `finish_present` intervals and cached FG outcomes. It added no GPU queries, readbacks, validation dispatches or resource writes and was removed afterward. GPU utilization/clocks/power were sampled separately at 500 ms. No recording/encoding or browser interaction occurred during retained samples. A focus-interrupted cohort was discarded.

The rc.1 control used its exact seven owned files and original public ReShade framework; the new FG-only files/framework/receipt were reversibly removed. Current shared Loader 2.3, RenoDX and official NVIDIA dependencies were retained, so this isolates the mod/framework change rather than recreating every historical dependency. Final files and ownership receipt were restored byte for byte.

The scene contains changing fire, weather/lighting and NPCs. These are bounded samples, not identical-frame captures, stock Ultra, long-session qualification or a latency test. SDK presentation FPS does not prove physical panel pacing. MangoHud can report application frames instead of all generated frames. Earlier tables remain historical.

[Sanitized per-run numbers](../qualification/fg-release/benchmark-2026-10-07.json) · [Linux qualification](LINUX_FINAL_QUALIFICATION_2026-10-07.md)
