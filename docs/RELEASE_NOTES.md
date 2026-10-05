# 0.1.0-preview.2 — startup and compatibility fixes

- Fix the Windows render-thread stack overflow caused by large path buffers.
- Detect unsupported rendering adapters before reducing source resolution. AMD/Intel remain on native anti-aliasing with a clear status message; DLSS still requires a supported NVIDIA RTX GPU.
- Bound missing DLSS source acknowledgements to 15 seconds.
- Fix Windows running-game detection, installer portability and build recipe checks.
- Use the exact GCC addon qualified on Linux NVIDIA and Windows AMD; the UI and shader payloads are unchanged from preview.1.
- Clarify dependency installation, updating and the distinction between the compiled release ZIP and GitHub's source archive.

Linux: Quality / DLAA / Native transitions, menu travel, saved Quality after reinstall and two normal exits passed on CachyOS Proton SLR with native WineWayland. The exact addon also passed 1,027.5 seconds of Windows AMD observation and unsupported-GPU fallback checks.

Known qualification limit: one Windows candidate exit crashed with `0xc0000005`; two others exited cleanly. The cause is unknown. Windows NVIDIA execution, physical controller, long sessions and device recreation remain unqualified. Further Windows checks are planned with the separate Reflex candidate. This release does not claim to fix every startup report; please report your platform and dependency versions if a crash remains.

[Linux evidence](LINUX_CACHYOS_REGRESSION_2026-10-05.md) · [Windows evidence](WINDOWS_REGRESSION_PR6_2026-10-05.md) · [Windows isolation plan](WINDOWS_SHUTDOWN_ISOLATION_2026-10-05.md).

The release reuses addon SHA-256 `611b49abee32db2705d47e5a9e08dd676cd7e892cb7da7e563eab4c18e733240` from the PR6 test candidate. Only package version and documentation changed for publication; no native rebuild was performed. Preview.1 remains frozen. Reflex, FG and RR remain hidden and absent from this release.

# 0.1.0-preview.1-windows-fix.1 — Windows candidate

- Fix Windows startup stack overflow by moving two 64 KiB path buffers to heap storage.
- Detect unsupported rendering adapters before reducing source resolution; AMD falls back to native anti-aliasing with a visible status message.
- Bound pending DLSS source requests to 15 seconds.
- Fix Windows running-game detection and make the installer test harness portable.
- Add a reproducible Windows native build with a 16 KiB stack-frame limit.

Real Windows installation/removal, bounded startup, AMD Quality/DLAA fallback,
return to Native and normal shutdown passed. NVIDIA DLSS execution with this
rebuilt addon remains unqualified. See [Windows validation](WINDOWS_VALIDATION_2026-10-05.md).
This candidate is a separate GitHub prerelease. The original preview.1 tag and
assets remain unchanged; Nexus has not been updated. Linux / NVIDIA regression
testing is still required before replacing the original preview.

# 0.1.0-preview.1

First experimental public preview.

- Native / NVIDIA DLSS in the native Video menu; DLAA, named SR presets and Custom scale.
- Linked presets and render scale, 100% DLAA, working slider fill and output-dependent minimum.
- Main-menu selection persistence with gameplay-only activation and current-process context authorization.
- Game-thread source-resolution application, safe native-history reset, native fallback and renderer acknowledgement.
- Exact dependency checks, refusal to overwrite, owned-file uninstall, external official DLSS runtime.
- Separate manual RenoDX UE Extended HDR guidance.

Bounded checks cover a fresh profile, Quality/Native/DLAA transitions, 4K Custom17%, main-menu travel and real install/removal. This remains a preview; Windows, physical controller, device failures and long sessions are not qualified. See [validation](VALIDATION.md).
