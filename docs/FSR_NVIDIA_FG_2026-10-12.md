# FSR upscaling with NVIDIA Frame Generation

The exact PR51 installer inputs were exercised with their selected CI ReShade
dependency. FSR Quality was active, but a saved NVIDIA FG request started without
a presentation owner and remained inactive after entering gameplay. The
`projectFgMenu` route policy admitted NVIDIA FG only with NVIDIA upscaling; both
early startup and live control used that policy.

The route projection now admits FSR SR/Native AA with NVIDIA single-frame
generation, using the existing shared SR guide producer. SDK support for the
actual rendering adapter, one startup presentation owner, current request stamp,
foreground/world readiness, matching guides and fault checks remain required.
This admission does not imply NVIDIA FG works on a Radeon GPU. Native rendering
with NVIDIA FG, XeSS sources, native MFG and hybrid routes remain excluded.

The portable projection fixture covers all SR/FG selections, all six FSR
qualities with FG Off/On, exact requested startup ownership, corrupt stamps and
unimplemented strategy/source refusal. Both Windows and Linux CI retain it.

The first rebuilt MSVC candidate acquired the NVIDIA owner and activated FSR
Quality plus NVIDIA FG, with successful guide tags and two SDK-reported presents
per rendered frame. A preset change then saved FG Off: the menu retained a second
NVIDIA-only SR restriction in `SelectMenuSr` and its availability check. That
failed transition is retained; startup activation alone does not qualify the
pairing.

The menu now shares a pairing predicate between availability and SR-selection
transactions. FSR preset/scale edits retain independent NVIDIA FG intent; an
unimplemented Native/NVIDIA route still turns FG Off atomically. The managed
fixtures cover every SR/FG route and all six preset edits, including the saved
startup owner. The source-matched NeoRune UI is rebuilt alongside its receipt.
The combined correction at `a2f62c617e338467e86897990f0a477efb143e13` then passed
all six FSR choices with NVIDIA FG active: Quality, Balanced, Performance, Ultra
Performance, Custom 77% and Native AA. Current guide dimensions matched SR input
dimensions, token/tag status was zero and the SDK reported two presents per
rendered frame. Normal quit nevertheless remained alive at the 180-second
watcher deadline, after render/RHI threads had exited. No shared SDK retirement
receipt appeared. The test process was ended and the original setup restored;
this is a failed shutdown, not a clean exit or a qualified package.

The native queue teardown returned before its shared-SDK notification when no
NGX SR feature existed. FSR never creates that feature. The generated adapter now
also notifies for the no-NGX path, outside the native observer mutex. An owned
NVIDIA presenter additionally requires stopped generation, verified alpha GPU
retirement, no retained NGX feature, ordinary closing ownership and one-shot
admission before shared SDK cleanup. Resize does not satisfy closing ownership.
The AMD path retains its existing bound-device and presenter-retirement guards.
The portable fixture now covers 128 AMD/NVIDIA admission combinations. Exit-only
private numeric receipts distinguish admission from successful cleanup.

The matching MSVC archive at commit
`858bda0aa721d32809d976867fb3af718817f5a2`, SHA-256
`2226ded9f8820862284d218505673e5b5347bae1d88a8d31ffd2abdcaf48a1c2`,
repeated all six FSR choices with active NVIDIA FG and zero tag/present status.
Normal quit returned code 0 approximately 5.33 seconds after confirmation.
The new notification reached shared SDK shutdown exactly once, but that call
returned 24 (`eErrorExceptionHandler`), with the SDK log locating the exception
in its FG plugin shutdown. A clean process exit does not qualify SDK cleanup.
Both standalone installer folders passed 59 real core transaction checks each,
including repair, ownership refusal, uninstall and reinstall; these ran on
Linux with synthetic fixtures and do not qualify the Windows GUI. Original
runtime files and mod preferences were restored exactly.

The closing-owner correction attempts NVIDIA cleanup at the swapchain boundary,
after stopped generation and completed alpha GPU retirement. It still requires
explicit native NGX retirement, ordinary closing ownership, an actual NVIDIA
presenter and one-shot admission, outside observer mutexes. A retained NGX
feature continues to defer to the native queue notification. Resize and AMD
presentation retain their existing paths. This follows the SDK's requirement
to shut down before graphics components are destroyed. Matching runtime results
are recorded below.

## Matching candidate qualification

The implementation at `4d5a5ebd993c0e59bacd0833788128f7d0565c3e` passed
three bounded Linux/NVIDIA runs using the exact MSVC artifact, archive SHA-256
`71fc9e563a31de90878e35b66c23f841e39ea9ce9284a066a8aa1a6266639622`:

| Route / transitions | Normal exit | Shared SDK cleanup |
| --- | --- | --- |
| FSR six presets + NVIDIA FG | Code 0; 6.87 s | One receipt; result 0 |
| DLSS Quality → DLAA + NVIDIA FG | Code 0; 3.26 s | Early refusal while NGX retained, then result 0 |
| FSR + AMD FG Off → On; DLSS → Native | Code 0; 4.90 s | AMD presenter and shared SDK result 0 |

All six FSR choices retained active NVIDIA FG, matching guide dimensions,
successful tokens/tags and two SDK-reported presents per rendered frame.
AMD generated counts stopped during a retained-owner Off interval while real
presentation continued; On and both SR-provider transitions resumed generation.
Native restored 100% source scale and independent full-resolution guides.
Original mod files and mod preferences were restored after every run. No forced
termination or exit-phase patch was used in these three tests.

[The numeric receipt](../qualification/providers/fsr-nvidia-fg-2026-10-12.json)
keeps the earlier hang and SDK error separate from these passes. The exact
framework remains an external dependency. Both installer folders passed their
59 core transaction checks; the [current packaging record](AMD_CANDIDATE_PACKAGING_2026-10-12.md)
and [Windows checklist](AMD_WINDOWS_CANDIDATE_2026-10-12.md) pin the same native
inputs. Windows/Radeon, active Anti-Lag/latency, wider lifecycle, performance
and physical pacing remain separate gates. Three clean exits do not establish
a universal shutdown repair or qualify a different rebuilt framework.

Published releases and dependency pins are unchanged.
