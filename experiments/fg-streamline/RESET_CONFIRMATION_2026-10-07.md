# Command-list Reset confirmation

## Reproduction and scope

The exact Windows v31 artifact reproduced an indefinitely pending Quality-to-DLAA
request on Linux with FG Off. Seven alternating transitions succeeded; the next
remained pending. Gameplay/presentation and the menus remained responsive. The
old generation held one recording, one reset-pending list and one immutable heap,
with no NGX evaluation failures. This differs from the previously captured
render-thread hang; a common cause has not been established.

ReShade notifies `reset_command_list` before calling the underlying Reset. The
old recording was deliberately retained until a later recording operation proved
its replacement. A successfully reset list left dormant never supplied that proof.

## Fix and safety requirements

- The addon arms a private uint64 epoch only on command lists holding its leases.
- The pinned PR435 framework advances that epoch only after native Reset succeeds.
- At finish-present, the addon observes the changed epoch and invalidates the
  old CPU recording reference. A subsequent queue signal and completed GPU fence
  are still required before resources can be released.
- A failed Reset, unavailable/malformed metadata or unchanged epoch retains the
  recording. Existing subsequent-recording proof remains available.
- Repeated pre-call notifications preserve the earliest observed epoch. The
  framework's public pre-call notification ordering is unchanged.

Microsoft documents that [command-list Reset may succeed while previous GPU
execution remains in flight](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-reset).
Reset success therefore proves recording invalidation, not GPU completion.

The framework base is `4eb9056c76016aad6f98495d3bbda2d721106104` plus
[`reshade-reset-epoch.patch`](../../qualification/fg-release/reshade-reset-epoch.patch).
Build receipts record the patch hash and binary hash; they do not claim an
unmodified checkout. No framework or vendor runtime binary is committed here.

## Qualification

The recording-lifetime unit check covers failed/successful resets, dormant lists,
repeated notifications, missing metadata, double invalidation and fence gating.
Framework provenance checks accept only the pinned clean base or that base with
the exact approved patch and explicit provenance fields.

The first Linux experiment passed ten FG-Off DLAA/Quality transitions and a
normal in-game Save and quit. Its proof-of-concept binaries were then replaced
with the opt-in implementation. Results for that implementation are recorded
below when complete. The earlier v31 benchmark is historical evidence and does
not qualify this rebuilt pair.

### Windows follow-up

Use the new `FG Windows candidate` artifact and its receipts, not the previous
download. Keep all six loaded-chain module hashes and the exact archive SHA-256.

1. Repeat startup, saved FG Off/On and unsupported-GPU fallback on Windows/AMD.
   No source-resolution reduction or active FG is expected on that adapter.
2. On supported NVIDIA hardware, repeat at least five DLAA-to-Quality-to-DLAA
   cycles with FG Off and then On, checking current requested/source/runtime
   revisions, zero errors and continuous active FG outcomes.
3. Check Native return/history reset, pause/resume, window minimize/restore,
   normal in-game shutdown and a longer gameplay session.
4. Report any pending request, render-thread hang, device error or exit crash.
   Clean runs do not establish the cause of the prior hang.

AMD fallback cannot qualify active NVIDIA SR/FG resource retirement. Installer
qualification is separate; this artifact is an isolated runtime trial, not a
published installer. No release/tag/assets are replaced by this experiment.
