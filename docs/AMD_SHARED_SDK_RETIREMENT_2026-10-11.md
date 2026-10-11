# AMD shared SDK retirement

The AMD presenter and the shared Streamline Reflex/PCL context have separate
lifetimes. Retiring the AMD presenter alone does not shut down Streamline.
NVIDIA's [manual-hooking lifecycle guide](https://github.com/NVIDIA-RTX/Streamline/blob/v2.14.1/docs/ProgrammingGuideManualHooking.md)
requires `slShutdown` before destroying the graphics device and other engine
components.

The AMD closing-owner path now requests shared SDK cleanup after successful
presenter retirement, while the device is still available. It requires a native
NGX retirement observation, a bound AMD-only Streamline context, ordinary
shutdown rather than terminal process teardown, and a one-shot admission gate.
An FSR menu selection is insufficient: a retained or failed NGX generation
refuses admission. Missing exports and busy native state also refuse admission.
The native retirement notification permits either order of owner retirement.
NVIDIA's existing shared-NGX retirement path remains in place.

Streamline readiness is published atomically only after successful device and
feature binding, and revoked before SDK shutdown. Ownership queries and cleanup
run outside the guide mutex. Resize callbacks do not admit shared SDK shutdown.
The new plain-C exports and admission header are covered by source receipts.

## Evidence and limits

An exact MSVC artifact of PR51 commit
`d42f6aef217b67b22f66aaaf592f4fef06442eb6` passed FSR Quality with AMD FG
On → Off → On, then FSR Native AA with FG On at 3840×2160. All checkpoints
reported successful evaluation and zero SDK faults, errors and warnings. Normal
quit returned code 0, approximately 4.36 seconds after confirmation. This artifact
preceded the shared SDK cleanup change.

A separate private trial moved shared SDK cleanup earlier. It completed the
same transitions and returned code 0 approximately 3.59 seconds after quit
confirmation. Exit-only instrumentation observed AMD retirement, `slShutdown`,
bridge unloading and bootstrap detachment all returning successfully. Original
files and preferences were restored byte for byte. A prior diagnostic control
exited without entering final SDK shutdown, so process exit alone cannot prove
the SDK retirement path ran.

The private trial established an ordering candidate, not a proven root cause or
qualification of the changed production source. The earlier intermittent hang
remains a failed result. The changed source requires matching-artifact runtime
repeats, including actual SDK-retirement receipts, and Windows/Radeon lifecycle
qualification. No raw runtime logs, account data or third-party binaries are
included. Published releases remain unchanged.

The vendor-free admission fixture covers all 64 input combinations, including
terminal shutdown, resize/non-closing ownership, retained NGX, unbound or NVIDIA
SDK ownership and duplicate cleanup. Source contracts and the fixture run in
both Linux and Windows CI.
