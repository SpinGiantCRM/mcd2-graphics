# FSR / AMD FG lifecycle checks

The [numeric receipt](../qualification/providers/amd-pr54-lifecycle-2026-10-14.json)
records an unmodified `0.4.0-amd.dev.8` run on Linux / RTX 4080 SUPER with FSR
Quality, AMD FG and native HDR. No private provider or sampler was installed.
The separately corrected Anti-Lag bridge was not used in this game run.

## Passed observations

- Saving and returning to the main menu stopped generation. The generated counter
  remained unchanged across two observations about 33 seconds apart.
- Re-entering the hub produced a new world epoch and resumed FSR and AMD FG with
  fresh guides and no reported SDK fault, error or warning.
- Windowed output changes resumed both features at 3200×1800, 1920×1080 and
  2560×1440. Returning to borderless restored 3840×2160. FSR Quality source sizes
  were 2134×1200, 1280×720, 1707×960 and 2560×1440 respectively.

## Failed observation

After those transitions, retained AMD FG Off → On did **not** resume the route.
FSR reported timeout **22**, source scale returned to 100%, and FG remained
inactive despite its saved On request. The AMD SDK reported no fault. This is a
failed reactivation gate; its cause is not established. It must not be treated as
a successful live-toggle check or fixed merely by extending a timeout.

Normal menu quit returned shipping-process code 0 in 5.58 seconds. Original mod
files, mod settings and renderer configuration were restored byte for byte.
Raw screenshots, own runtime slots and diagnostics remain private.

This run establishes bounded main-menu world recreation and operational output
resize behavior. It does not establish dungeon travel, comprehensive image quality,
physical latency, native Windows/Radeon support or a clean full qualification.
