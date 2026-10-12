# Windows/Radeon development qualification

Use the AMD development installer artifact built from the pinned native archive.
The native inputs are run 38185912901, artifact 11719049992, implementation
4d5a5ebd993c0e59bacd0833788128f7d0565c3e. Archive SHA-256:
`71fc9e563a31de90878e35b66c23f841e39ea9ce9284a066a8aa1a6266639622`.
Its receipt source is GitHub test merge
`79538debe04b9b49d3360509a1fded2f8883e936`; its runtime source tree matches
that implementation. Keep these pins distinct from later documentation commits.

## Preparation

1. Close the game. Use verified Steam build 25754144, shipping SHA-256
   `3a8703406fd50520f83c4f70a0212c000cb3b584ef28eb38032902230c01ebdd`.
2. Back up replaced mod files and only mod settings/runtime slots and authority.
   Preserve other mods and player/account saves. Use the visible installer folder.
3. Select external dependencies matching the accompanying candidate lock.
   The exact artifact ReShade remains separate; no vendor DLL or probe is payload.
   Loader 2.3 is recognised under the existing 2.0+ API policy. No popup API is used.
4. Record exact installed hashes, actual rendering GPU/LUID, driver and OS.
   Unknown executables and SDK/runtime versions must fail their normal gates.

## Required runs

- **Native control:** 100% Native, FG Off, both vendor low-latency modes Off.
  Verify ordinary temporal output, source scale and stable native limiting.
- **FSR:** Native AA, Quality, Balanced, Performance, Ultra Performance, Custom.
  Require actual reconstruction, correct scale and no stale-frame/colour damage.
- **AMD FG:** independent provider selection plus restart, Off/On, active real
  and generated counts, no errors, paused/loading generation stopped, resumed
  fresh guides. Repeat with FSR, Native and NVIDIA SR where supported.
- **Anti-Lag 2:** require actual Radeon driver/API eligibility. Off/On in the
  Video menu with FG Off and AMD FG On, matching revision, correct pre-input
  timing and FG metadata. Measure latency independently; a toggled row is not
  proof of activation or benefit. On an unsupported adapter it must stay hidden.
- **NVIDIA routes:** test only on a supporting NVIDIA GPU. FSR + NVIDIA FG
  must retain FG through all FSR presets; DLSS/DLAA + NVIDIA FG must remain
  functional. AMD laptops qualify refusal/fallback, not active NVIDIA features.
- **Lifecycle:** main/pause menus, hub/level travel, cuts/death/respawn,
  pause/resume, source/output resolution and window changes. Provider owner
  changes require restart; no two presenter proxies may be active.
- **Shutdown:** hold the shipping process handle through three normal quits.
  Require code 0 and completed owning presenter/shared SDK retirement, not
  merely a closed window. Preserve SDK errors and delayed exits as failures.
- **Installer:** fresh install, repair, modified-file refusal, uninstall/reinstall,
  external-dependency retention, and unchanged player/account/other-mod files.

Report requested and effective routes separately. Keep SDK real/generated
counts separate from displayed FPS and input latency. Preserve raw diagnostics
locally; share only sanitised numeric results and failures. UI metadata currently
reports 0.3.0-preview.1 despite the development installer version; align it before
any public release. Windows active-provider/driver results are still required.
