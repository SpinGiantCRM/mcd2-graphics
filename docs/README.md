# Documentation

## Released 0.4.0-amd.dev.25

- [Features, exact dependencies and known issues](DEV25_RELEASE_CANDIDATE.md)
- [Install, repair, verify and uninstall](../INSTALL.md)
- [Advanced settings](ADVANCED_SETTINGS.md), [HDR](HDR.md) and [dependency overrides](DEPENDENCY_OVERRIDES.md)
- [Changelog](RELEASE_NOTES.md) and [validation index](VALIDATION.md)
- [Windows/Radeon acceptance](WINDOWS_DEV25_ACCEPTANCE_2026-10-24.md) and [Linux/NVIDIA acceptance](LINUX_DEV25_ACCEPTANCE_2026-10-24.md)
- [Graphics problem reports](REPORTING_GRAPHICS_PROBLEMS.md) and [online-use scope](ONLINE_USE.md)
- [Published hashes and signing/scan status](../qualification/providers/dev25-release-artifacts-2026-10-24.json)

The released build targets Steam. It uses one FG provider at a time; Game Pass,
software/native MFG and mixed-provider MFG are not in its downloads. Performance
benchmarks remain on hold. Qualification applies to the exact recorded packages
and test scope, with known issues.

## Unreleased PR60 integration

- [Multi Frame Generation](NVIDIA_MULTI_FRAME_GENERATION.md)
- [Owned compute/presenter checks](MFG_COMPUTE_TESTING.md)
- [Edition-aware installer selection](INSTALLER_GAME_SELECTION.md)
- [Game Pass save transport](GDK_SAVE_TRANSPORT.md)

Game Pass 1.1.3.0 has a contributor-reported executable hash for candidate
installation. Native Game Pass and Linux/Xodus runtime checks remain pending.
The contributor testing guides preserve their separate historical candidates.

## Development and historical evidence

[Source build](BUILD.md) · [Compatibility contracts](WINDOWS_COMPATIBILITY.md) ·
[Provider architecture](PROVIDER_ARCHITECTURE.md) · [Roadmap](../ROADMAP.md)

Dated qualification reports and earlier release sections retain their original
versions, hashes, failures and limitations. They are evidence for those inputs;
they are not instructions to install older dependencies in the current release.
