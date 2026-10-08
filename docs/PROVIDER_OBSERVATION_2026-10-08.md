# Experimental settings handoff — 8 October 2026

## Scope

PR #17 adds an opt-in legacy-settings observer and an early startup reader for
its separate provider mirror. The existing menu remains the writer. This is
not the authoritative settings migration or new FSR support.

Code tested: `4d029f9cdef8658f8396c271e149d2199766e43b` (tree
`b8ae5daa345003a093357a4275eae2393cd53037`). Linux/CachyOS, KDE Wayland,
CachyOS Proton SLR, RTX 4080 SUPER, driver 615.71.09. Windows evidence is CI
compilation and fixtures only, not gameplay.

| Candidate component | SHA-256 |
| --- | --- |
| Early DXGI bootstrap | `9493f213b02febebaa2b78cf6c5154ed9c8c5083f62ffa5225677111893de353` |
| FG-chain latency addon | `cd8a20f1e65c92898a2e8319d0c49b70d44cb69777c7aaa063189b7f1b1bda9a` |

The latency trial used `build_game_candidate.py`, adopting the existing shared
FG SDK and engine-token exports. The installed SR, guide addon, UI, shader,
framework and vendor runtimes were retained. Native outputs came from the
recorded source tree; private build receipts retain source hashes.

## Results

| Check | Result and limit |
| --- | --- |
| Real saved preferences → mirror | Valid committed record, revision 2346, matching the three mod-owned legacy slots. Legacy UI remained authoritative. |
| Runtime ACK changes | SR runtime/save revisions advanced; mirror stayed at 2346 while requested preferences remained unchanged. |
| Matching mirror at restart | `observed-settings-valid=1`, revision 2346, requested NVIDIA FG owner; rendering-adapter approval and game HWND swapchain routing succeeded. This does not establish active interpolation. |
| Well-formed stale mirror at restart | A checksummed mirror with differing FG preference was published with the game closed. Legacy saves remained unchanged. `observed-settings-valid=0`, FG session mode 0; no factory/swapchain FG route recorded. Observer was disabled for this isolation launch. |
| Restoration | Game closed normally before writes. All 12 released payload hashes and six backed-up mod-only settings/runtime slots matched after restoration. The newly created experimental mirror was removed. |

The Linux store fixtures passed five independent-process checks, 144 wire
round trips and 1166 rejection cases. Observation fixtures passed. An old-target
MinGW compile passed. Hosted Linux/Windows portable and guided-installer checks
and the complete Windows FG candidate build passed for the code commit above.
The Windows store fixtures also exercise old SDK target macros; the documented
rename information class remains unchanged.

## Game-build blocker

The installed Steam build is **25754144**, executable SHA-256
`3a8703406fd50520f83c4f70a0212c000cb3b584ef28eb38032902230c01ebdd`.
The released integration pins build **25647713**, executable SHA-256
`231147bd0c655a4ae73f90873675d42917f2bfb3a9ee164fc64f217d6d6bd4ef`.
All five guarded latency hook locations fail comparison against the new
executable. The live addon reports `engine signature mismatch` and declines
installation; active FG/Reflex cannot be qualified in this environment.
The guarded engine-hook source is unchanged by this experiment. Do not remove
its checks or infer new offsets from a constant address delta.

Required follow-up: establish a separately verified new-build hook map, registry
ABI and frame-identity boundary, then repeat activation, transitions, reset,
shutdown and performance checks. Keep the historical baseline separate.

## Remaining gates

- Actual menu preference edits and persistence, including linked SR presets and
  HDR/Reflex settings, still need the complete runtime matrix.
- A valid FG-Off request, missing/corrupt records and unsupported requests have
  portable coverage; only matching FG-On and stale-request startup were exercised
  in the game during this trial.
- Windows AMD fallback, Windows NVIDIA activation, active Linux FG/Reflex and
  long-session/lifecycle regression remain unqualified for these rebuilt files.
- One consolidated UI request/writer and explicit effective-state receipts are
  still required before retiring the legacy writer.

No release tag, published asset, installer payload or vendor dependency was
replaced. Raw logs, screenshots, account data, player saves and game binaries are
not included in this record.
