# FSR / AMD FG toggle isolation

The [numeric receipt](../qualification/providers/amd-pr54-toggle-isolation-2026-10-14.json)
records an unchanged dev.8 production run on Linux / RTX 4080 SUPER with FSR
Quality, AMD FG and native HDR. No private sampler or modified provider was used.

## Observations

- An Off/On change in one menu visit resumed FSR and FG without a resolution change.
- Windowed output changes recovered at 3200×1800, 1920×1080 and 2560×1440;
  returning to borderless recovered 3840×2160.
- After these changes, resuming gameplay with FG Off kept FSR active and stopped
  generated frames. A separate On request and menu wait resumed both features.
  Five subsequent snapshots showed fresh FSR evaluations and generated frames.
- In the menu, the SR context reported ready while the FG context reported not
  ready. This mismatch alone did not reproduce timeout 22. No readiness or timeout
  code was changed on that hypothesis.

The earlier [timeout failure](AMD_LIFECYCLE_2026-10-14.md) remains unresolved.
These controls are evidence of successful repeats, not a verified fix.

## Shutdown failure

Normal menu quit ended the shipping process with **0xc0000409**. The input helper
returned 0 and the process disappeared, but the held shipping-process handle
reported the failure. Those first two observations must not count as a clean exit.
The last own AMD presentation-retirement and shared-SDK-retirement records both
returned 0; they do not establish successful process shutdown or its cause.

This uninstrumented failure means a private sampler cannot be the sole cause of
the earlier shutdown failures. The original mod files, own settings and renderer
configuration were restored byte for byte. Raw captures and diagnostics remain
private. Full production and native Windows/Radeon qualification remain incomplete.
