# FSR gameplay readiness

A private run with the corrected FSR resource ownership and the previous UI
reproduced timeout 22 without leaving Video settings. After a Native → FSR request,
FSR entered Preflight with zero evaluations while the SR context reported ready
and the FG context reported not ready. It subsequently entered RestoreSource and
Fallback with error 22. This distinguishes the menu-admission failure from the
separately corrected resource double release.

SR and FG now use one actor gameplay gate: paused games and activated settings,
system, inventory, map or unknown widgets refuse rendering admission. Existing
HUD/navigation, activity-container and notification allowances are preserved.
The gate does not depend on the projected saved ready flag, avoiding a circular
latch. FSR's native 15-second failure timeout is unchanged.

An existing FSR foliage velocity lease may survive a matching temporary-menu
retirement with no error. New acquisition still requires gameplay and active
evaluated output. Changed intent, world/session, source plan, corruption or errors
still refuse the lease; source rollback remains allowed outside gameplay.

The managed client checks widget classification, paused lease retention and
error refusal, alongside existing source-ACK/travel/rollback checks. Source guards
verify both real actor callers. A real NeoRune/ModInfo rebuild is required; managed
stubs alone do not qualify the actor's runtime behavior. Live checks and complete
candidate qualification remain required, including native Windows/Radeon.

## Bounded live check

The [numeric receipt](../qualification/providers/fsr-menu-readiness-2026-10-14.json)
records the real rebuilt UI with the privately rebuilt ownership-corrected native
addon and the remaining dev.8 baseline files. A new FSR request stayed inactive
through a menu hold beyond the unchanged timeout, then activated after resume.
An already-active FSR session entered error-free retirement during a second menu
hold; generation stopped, then fresh evaluations and generated frames resumed.
Both SR and FG contexts reported not ready in the menus. Normal quit returned
shipping-process code 0 in 4.19 seconds. The original setup was restored exactly.

These checks qualify the stated local control, not a future complete CI artifact,
native Windows/Radeon, or every earlier intermittent shutdown failure.
