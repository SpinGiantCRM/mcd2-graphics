# Windows/Radeon development qualification

**Selection update, 14 October:** the [Anti-Lag shutdown correction](AMD_ANTILAG_SHUTDOWN_2026-10-14.md)
changes the project bridge after dev.8. The dev.8 hashes below identify the tested
baseline; they do not contain that correction. Select a new complete rebuilt
artifact before starting new Windows qualification. The AMD laptop is currently
unavailable, so active driver testing remains outstanding.

Use [the current candidate receipt](../qualification/providers/windows-candidate-current.json)
and [the exact installer verification](../qualification/providers/amd-pr54-rebuilt-installers-2026-10-13.json).
The selected candidate is **0.4.0-amd.dev.8**. Its native inputs are
[run 38333151314 / artifact 11783652643](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/38333151314/artifacts/11783652643),
archive SHA-256 `9574ec28b096f35e484af61e066835a1815178070d96e390c4c24ddd599c9e64`.
Its source receipt is test merge `818033960e3347718a1c7f7ef0ca4e169f0b99d5`,
matching implementation `c9f011a5bdc8dd6fd50885eeb8c2986764183755`.

The verified visible installer folders are from
[run 38340596287](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/38340596287),
installer archive SHA-256 `2605460e66a8fb4336a75e9c22da8899a873dd1d22ce5471b9f52d6d6beeabc3`.
Use the accompanying dependency lock. The required source-built framework is
separate, SHA-256 `5ab60fb92385c774bb5f61d668676556c32fc91d989a18679129cdb32031bb84`;
do not substitute an earlier artifact's framework and inherit its results.

**Qualification remains incomplete.** The complete dev.8 Linux run passed
bounded NVIDIA preset/status checks but hung on shutdown. A short private
instrumented control and a later unmodified 13-minute production control exited
0; neither clears the earlier production failure or establishes its cause.
Preserve that distinction when reporting Windows results. See
[the regression report](AMD_NVIDIA_REGRESSION_2026-10-13.md).

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
