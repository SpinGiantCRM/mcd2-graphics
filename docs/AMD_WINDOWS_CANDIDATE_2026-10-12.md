# Windows/Radeon development qualification

**Selection update, 14 October:** use the complete **0.4.0-amd.dev.10**
[candidate receipt](../qualification/providers/windows-candidate-current.json).
It includes the Anti-Lag shutdown, FSR owned-resource and shared gameplay-gate
corrections, including the real rebuilt UI. Earlier dev.8/dev.9 artifacts do not.

Native inputs:
[run 38497329826 / artifact 11863879034](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/38497329826/artifacts/11863879034).
Archive SHA-256 `0146aa19f2e71c87dd2f303f7b0526a188325f6c8a8a66fe6aa24d61d5936751`.
Test merge `87078dbab5f3658267300796046f709ede96d3ef` matches implementation
`9483952ec9896633c58685493e0c101757c004a2` by tree. All 44 artifact members were
verified. The separate framework SHA-256 is
`1b5d71669299aa06dae2b27bf51f554fb59e2cd2f6a406a7447d59caf385c71b`.
Use its accompanying dependency lock; do not inherit earlier binary results.
Visible installer folders now match this selection:
[run 38502849822 / artifact 11865709322](https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/38502849822/artifacts/11865709322),
archive SHA-256 `d6e50cd6058759fcb07db7883d3bd5bb3a5b9c1224314cc34b2f7fb967d0f51f`.
The [file verification receipt](../qualification/providers/amd-dev10-installers-2026-10-14.json)
records both visible folders, their identical manifest/dependency-lock bytes and
all 17 own payload files matching the game-tested selection. They use no embedded
payload archive or self-extraction; framework/vendor runtimes remain separate.
This verifies packaging, not installer GUI transactions or Windows execution.

**Not cleared for qualification or release:** [dev.10 Linux checks](AMD_DEV10_LINUX_2026-10-14.md)
passed all six FSR presets with each FG route, AMD transitions/resize and NVIDIA
DLSS/DLAA checks. Two normal quits returned 0; the third returned `0xc0000409`
after NVIDIA retirement/re-enable refusal. Successful SDK cleanup does not clear
that process failure. Its cause remains unresolved. Earlier failure records are
preserved. Native Windows/Radeon and actual Anti-Lag driver results also remain
outstanding; the AMD laptop is currently unavailable.

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
