# Steam build 25754144 compatibility trial

8 October 2026. Experimental source change; not a release qualification.

## Cause and scope

The installed Steam executable changed from the released build 25647713 to
25754144 (SHA-256
`3a8703406fd50520f83c4f70a0212c000cb3b584ef28eb38032902230c01ebdd`).
The released latency addon rejects all five historical hook guards on the new
build. A separate launch with the original released payload reproduced the
rejection, independently of the provider-settings experiment.

The new explicit layout maps the modular registry, simulation/render/present
wrappers, name constructor and frame counters. These were inspected in the
normally running executable, without modifying game code. Matching used
multiple independent instruction sequences and registry/counter references;
it did not apply one address delta to the whole image.

The historical map and its guards remain. The new map additionally requires
its PE timestamp/image size, registry/name entry guards and actual registry
object/vtable identity. Unsupported signatures leave the hooks absent. Both
the pacing callback and FG's render-frame export read the selected counters.
Installer executable admission remains unchanged.

## Bounded Linux result

CachyOS, KDE Wayland, CachyOS Proton SLR, RTX 4080 SUPER, driver 615.71.09.
Only the actual FG-chain latency addon was replaced temporarily. Released
bootstrap, SR, guide addon, UI, shaders, framework and vendor runtimes were
retained. Experimental provider observation was disabled.

| Check | Result |
| --- | --- |
| Selected layout | Build 25754144; registry installation accepted. |
| Reflex/timing | Available, On; completed frame counts advanced, with zero coordinator, marker and SDK errors in the captured samples. |
| Gameplay SR/FG | SR active without an error; FG available and active. |
| FG presentation | 60 sampled outcomes spanning frame IDs 32575–36115 each reported two actual presents, SDK result 0 and status 0. This is activation evidence, not an FPS benchmark. |
| Exit | Normal window close; process exited and shared SDK shutdown returned 0. |
| Restoration | All 12 released payload hashes and six backed-up mod-owned settings/runtime slots matched. No experimental mirror remained. |

Candidate latency SHA-256:
`8c7404094967b727775d67c298959c7411428fe023f701216ed13edf04d03620`.
Engine-map header SHA-256:
`7902c52d17c742ec108409735a45d18801f82923e979e4d7352685c4250373e4`.
Private build receipts retain the generated source and toolchain provenance.

Portable fixtures passed 138 missing/corrupt guard cases, both layout
selections, metadata mismatch and mixed/empty-map rejection. GCC and MinGW
compilation passed, as did the actual FG-chain build and its 16 KiB native
stack-frame guard. Relevant FG recipe/provenance fixtures also passed.

## Remaining qualification

- Rebuild from the exact candidate source. Confirm the receipt's map hash and
  generated latency addon; a standalone latency build lacks FG token exports.
- Windows AMD: startup, unavailable NVIDIA controls/native fallback, and normal
  exit without an access violation. Earlier Windows qualification does not cover
  this native change.
- Windows NVIDIA and Linux: Native/Reflex Off–On–Boost; DLAA and Quality with
  FG Off/On; verify frame IDs, SDK errors and real/generated presentation counts.
- Exercise menu/gameplay, hub/level, resolution changes and normal shutdown.
  Repeat matched Off/On performance checks and a longer session before release.
- Test the historical build separately when available; it has portable guard
  coverage here, not a fresh runtime pass.

No published tag or asset was replaced. This report excludes raw logs,
screenshots, account data, player saves and game binaries/code captures.
