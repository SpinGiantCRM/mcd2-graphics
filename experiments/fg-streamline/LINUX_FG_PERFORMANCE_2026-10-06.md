# Historical v27 Linux measurements — 6 October 2026

**Superseded:** post-transition FG performance claims in this record are invalid.
SR cleanup shut down the shared NGX device instance while Streamline still held
its FG feature. Internal FG evaluations then failed with `0xbad00004`
(`FeatureNotFound`), despite public SDK status 0 and two reported presents.

The apparent DLAA speedup from about 65 to 76 rendered FPS was a failure state.
The prior Quality FG result is also withdrawn. Use the
[corrected lifecycle measurements](LINUX_FG_NGX_LIFECYCLE_2026-10-06.md).
The [historical numeric record](LINUX_FG_PERFORMANCE_2026-10-06.json) is retained
for reproducibility, with the affected rows marked invalid. No release changed.
