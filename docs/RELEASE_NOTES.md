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
