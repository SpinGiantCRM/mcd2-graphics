# AMD FG continuous movement capture

The [numeric receipt](../qualification/providers/amd-pr54-continuous-images-2026-10-14.json)
records **600 consecutive real/generated pairs** from FSR Quality + AMD FG
on Linux / RTX 4080 SUPER. This extends the earlier
[burst image checks](AMD_LABELLED_IMAGES_2026-10-14.md).

The base remains `0.4.0-amd.dev.8`, with a private capture bridge substituted.
Production source and released files were unchanged.

## Results

- All 1,200 images mapped after successful SDK GPU completion; no capture refusals.
- SDK frame identities were consecutive. Engine identities and source timing
  matched within every pair. All RGB images were distinct.
- The 640×360 world crop includes a bounded character movement and moving
  nearby entities. Sampled pairs retained coherent character and world geometry.
- Both frame types used PQ/BT.2020. Their maximum decoded channel signal was
  416.403 nits; this is not measured panel luminance.
- A private replay preserves all captured images across 7.191 seconds of source
  timing. Its encoded endpoint repeats the last image. The first export was
  rejected because the encoder's default timebase dropped frames; the corrected
  export retained them and decoded successfully.
- Normal menu quit returned shipping-process code 0 in 3.877 seconds. Original
  mod files, mod preferences and renderer configuration were restored byte for byte.

## Scope

The replay reconstructs generated/real ordering using half the corresponding
source interval for each image. It is **not a recording of physical presentation**.
Its SDR preview decodes PQ, converts BT.2020 to Rec.709 and clips highlights above
203 nits. Raw images, logs and footage remain private.

This short diagnostic capture is not a performance benchmark or comprehensive
temporal-quality pass. It does not clear the earlier production shutdown failure,
qualify native Windows/Radeon or prove active Anti-Lag 2 or reduced input latency.
