# FG candidate v31 — source investigation

v31 corrects shared NGX ownership during SR preset retirement. It supersedes
v27's invalid post-transition FG measurements.

[Diagnosis and corrected measurements](../../experiments/fg-streamline/LINUX_FG_NGX_LIFECYCLE_2026-10-06.md).

No Windows archive is cleared yet: DLAA with FG Off still has a small measured
cost relative to the release. The branch contains the source correction;
Windows NVIDIA runtime and performance qualification remain outstanding. Published
releases and the historical v27 archive are unchanged.

The separate [Windows / AMD trial](../../docs/FG_WINDOWS_AMD_VALIDATION_2026-10-06.md)
passed reversible installation/restoration, bounded startup, unsupported fallback
with FG Off and persisted On, and two normal process exits. Its
[exact-binary aggregate](windows-amd-checks.json) does not qualify active NVIDIA
FG/HDR or shared NGX teardown and does not clear an archive for release.
