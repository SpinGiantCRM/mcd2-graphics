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
Rebuilt source needs its own runtime, transition and shutdown qualification;
the earlier package's inactive pairing is retained as a failed result.

Published releases and dependency pins are unchanged.
