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

The next correction attempts NVIDIA cleanup at the closing swapchain boundary,
after stopped generation and completed alpha GPU retirement. It still requires
explicit native NGX retirement, ordinary closing ownership, an actual NVIDIA
presenter and one-shot admission, outside observer mutexes. A retained NGX
feature continues to defer to the native queue notification. Resize and AMD
presentation retain their existing paths. This follows the SDK's requirement
to shut down before graphics components are destroyed; it is not yet a proven
repair of error 24. Windows/Radeon, wider lifecycle, performance and physical
pacing remain separate gates.

Published releases and dependency pins are unchanged.
