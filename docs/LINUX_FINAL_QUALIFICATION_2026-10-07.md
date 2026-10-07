# Final Linux qualification — 7 October 2026

Release input: **0.3.0-preview.1**, installer source `e421e57e7667f0e9ec71e1940c6daa352a39f0c7`, [CI run 37602995068](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/37602995068). The final UI/native/shader/installer files were not rebuilt or changed during qualification. The separately pinned MSVC ReShade framework was used.

Decision: **PASS for the bounded Linux/NVIDIA release scope below.**

## Scope

CachyOS KDE Wayland, kernel 7.2.9-1-cachyos-bore, RTX 4080 SUPER / driver 615.71.09, CachyOS Proton SLR, native WineWayland HDR. Steam build 25647713; verified shipping SHA-256 `231147bd0c655a4ae73f90873675d42917f2bfb3a9ee164fc64f217d6d6bd4ef`. 3840×2160, VSync Off, no FPS limit. Loader 2.3, RenoDX UE Extended nightly-20260928, DLSS 310.9.1 and Streamline 2.14.1; dependency overrides Off.

## Completed checks

- All 234 Linux deployed receipt files verified, with manifest/lock matching source and twelve own payload hashes matching the Windows-qualified artifact.
- Actual downloaded Linux GUI fresh Install, Repair/Verify, confirmed Uninstall and fresh reinstall passed. Uninstall removed unchanged owned files/restored the original proxy, preserved shared dependencies/ReShade configuration and preserved the mod's preference slots. Its private DLSS copy was removed as intended and subsequently restored from the verified official dependency. Player/account saves were excluded.
- An obsolete private prototype receipt was refused before mutation. It was backed up and removed through a reversible maintenance transaction; no receipt edit or ownership bypass was used.
- 17 installer regressions, 119 guided-installer core checks and 31 foliage-ownership checks passed locally. Exact-head Windows/Linux hosted checks also passed before documentation updates.
- Final native Video menu exposes concise help and separate recommendation footer. Quality and DLAA resolve matching request/source/runtime revisions in gameplay. FG Off/On has separate current-session acknowledgement.
- FG On remained active in all retained Quality/DLAA benchmark intervals, with successful SDK status, continuous cached outcomes and two reported presentations per rendered frame. Internal feature-evaluation/NGX errors were checked separately and absent in the sampled final On log.
- Quality↔DLAA changes completed without indefinite Applying. Saved DLAA/FG Off settings survived normal shutdown/relaunch, with fresh process context and acknowledgement.
- Three normal shipping-process window closes returned exit code **0**, measured by handles held before the request: **4.633 s**, **4.102 s** and **4.485 s**. No force kill or launcher-exit proxy was used.

- Final clean launch after removing the temporary sampler passed over one minute of gameplay observation, including bounded movement/jump input, with matching Quality/FG On acknowledgement at the start and end. The game remains open for the maintainer’s demo. This is a smoke check, not full combat/lifecycle qualification.
- Main-menu Mods metadata showed the correct author, feature description and 0.3.0-preview.1 version with Loader 2.3. Native fallback and DLAA/Quality selections returned matching current-process acknowledgements; the native FG On toggle saved and activated after restart.

## Benchmarks and limitations

[Fresh FG Off/On results and previous-release control](BENCHMARK_2026-10-07.md) replace stale performance claims for this release. SDK-reported presentations are not physical panel-pacing or latency measurements. Native Windows AMD qualification is [separate](WINDOWS_FULL_QUALIFICATION_2026-10-07.md); its clean but delayed exits and historical cross-built-framework failure remain documented.

Known Proton backend synchronization warnings remain; successful frame generation does not establish Reflex latency benefit. VSync On, Windows NVIDIA active features/HDR, physical controllers, long sessions, device loss and Xbox/Microsoft Store PC compatibility remain unqualified. No raw logs, player/account data, party codes, screenshots or extracted game shaders are published.

[Exact release hashes](../qualification/fg-release/release-artifacts-2026-10-07.json) · [Packaging/review policy](NEXUS_RELEASE_GATE.md)
