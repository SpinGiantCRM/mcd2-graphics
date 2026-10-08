# Consolidated menu transport trial — 8 October 2026

This is bounded serialization/transaction evidence for an opt-in development
path. It is not FSR support, a completed settings cutover, Windows qualification
or a new release. Existing controls and renderer readers retain their legacy
path; the qualification client sends unchanged-preference transactions only.

## Inputs

- Installed Steam build 25754144; executable SHA-256
  `3a8703406fd50520f83c4f70a0212c000cb3b584ef28eb38032902230c01ebdd`.
- Existing Linux/NVIDIA, CachyOS Proton SLR and native HDR setup.
- Native addon built with the actual FG candidate recipe, preserving the heap
  paths, frame-size guard, SDK owner lifecycle and validated engine hook map.
- UI rebuilt separately with NeoRune 0.1.2 and the existing .NET reference pack,
  using the guarded actor overlay and pinned metadata builder.
- Only the latency addon, three UI containers and the opt-in bootstrap policy
  were temporarily replaced. Other release payloads and dependencies retained
  their original hashes. Twelve release files and six mod-owned state slots were
  checked/backed up; no account or character save was inspected or modified.

Final native addon SHA-256:
`b1d48f84806ff7a5962ada5ebf176f008c0bc5ac7c392c1f9205510215a2107f`.

Client source SHA-256:
`799e1c0b8aaaf653905653f537d803cc86b9c0f4a2085d25dc623bf044151712`.

Generated actor overlay SHA-256:
`14fd2daa2d7f5e8f4bb78dfd0e9f02dc63632ca7f4c98266fb0ab40256b79596`.

| Trial UI container | SHA-256 |
| --- | --- |
| pak | `7d1fd6761be398e1024562ed0eac28a6124ae90b8d34fbb6644e46d10d6cd3e6` |
| ucas | `bb80862b37ceb3a0b79b464f9866feaf7f2d1d542600ec9229d81e39ba3d6daa` |
| utoc | `36e49da5428002dfeafc6ee62acaf26966b10d9cb5a8b238bcfdc67d14b8c076` |

## Observations

1. A stable legacy snapshot initialized the experimental authority. The worker
   returned a valid 176-byte snapshot through the real UE save serializer.
2. The exact NeoRune client read that snapshot, preserved all 32 graphics-record
   words and generated valid inner and outer CRCs. A real authority response
   with CRC bit 31 set was successfully read and validated by the client,
   confirming signed-field preservation through UE serialization. Its no-op request used the
   expected revision/checksum and next desired revision.
3. Native acknowledged `Unchanged`, keyed to the request's current session and
   sequence. The authoritative preference revision/checksum did not advance.
4. Entering gameplay created a new menu actor instance. Its next sequence was
   acknowledged without a duplicate preference publication or legacy resync.
5. A normal close and restart changed the native session. The stale saved request
   could not claim the new session's acknowledgement; the client issued a new
   matching-session request and received `Unchanged` against the same authority.

The initial probe attached a timer to a detached SaveGame object and produced
no request. Moving polling to the existing game actor produced the validated
round trip. Keep that actor-owned timer in the eventual settings controller.
NeoRune also rejected right-shift operations at compile time. The client instead
uses integer division, masks and explicit high-bit terms; the exact source passes
4,000 independent CRC comparisons under .NET with test-only UE substitutes.

## Checks and limits

Native fixtures cover strict save framing, full signed word patterns, all
truncated request saves, 1,408 damaged authority responses, invalid status/store
combinations, request/response files, wrong/restarted sessions, idempotence,
no per-poll response rewrite and corrupt-authority preservation. Two competing-initializer cases ensure an
already-created or recheck-time authority cannot be overwritten by legacy
migration. Linux execution,
MinGW compilation, store-process races, observation/transaction regressions and
FG recipe checks pass. Both platform CI jobs retain these checks.

These observations establish the no-op save transport only. Real-control edits,
preference-write ownership, separate runtime contexts and source ACKs, bootstrap
and render-reader cutover, full lifecycle/performance coverage and Windows UE
runtime checks remain required. No FSR/Anti-Lag/FG feature was enabled by this
transport. Raw logs, saves, screenshots, account information and vendor binaries
are not attached to this report.
