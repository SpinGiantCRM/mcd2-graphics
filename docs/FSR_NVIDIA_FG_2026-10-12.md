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
The combined correction needs matching-artifact runtime, transition and shutdown
qualification. Windows/Radeon and physical pacing remain separate gates.

Published releases and dependency pins are unchanged.
