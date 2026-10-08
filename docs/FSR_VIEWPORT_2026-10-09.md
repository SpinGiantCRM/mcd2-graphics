# FSR active viewport correction — 9 October 2026

## Cause and correction

Balanced requested the SDK recommendation of 2258×1270 for 3840×2160 output. The game rounded its visible render area to 2259×1271 and allocated a padded 2260×1272 texture. The previous full-texture rectangle check rejected that layout before SDK dispatch.

The controller now validates the zero-origin inclusive active rectangle separately from allocation dimensions. After the exact process/world/source-scale acknowledgement, it accepts at most one pixel above the SDK recommendation on either axis. Allocation padding is limited to seven pixels per axis; malformed, shifted or undersized rectangles still fail closed.

FSR converts depth and motion vectors using the active dimensions. When allocation padding exists, an owned colour texture receives only the active rectangle. The original resource state is restored, and the cropped resource remains under the generation's existing GPU retirement rules. Full-sized inputs add no colour crop.

The default Native/DLSS conversion shader compiled byte for byte identically to the preceding candidate. The FSR converter is a separate shader asset. Released installers and dependency pins are unchanged.

## Gameplay evidence

A stationary Linux/NVIDIA 4K gameplay trial accepted Native AA, then Balanced, then Quality, then returned to Native at an acknowledged 100% source scale. The [sanitized receipt](../qualification/providers/fsr-viewport-2026-10-09.json) records exact dimensions, evaluation counts and artifact hashes. No SDK error was reported in those observations.

These counts establish sustained execution and output substitution, not image-quality scores or frame-rate improvements. The private chooser used the shared production settings client; ordinary Video-menu FSR admission is still pending.

The process lingered after confirmed menu quit, as in the preceding trial. Shutdown remains unqualified and the cause has not been isolated. Windows/AMD execution, wider lifecycle transitions, Custom scales, image quality and performance remain separate qualification gates.

## Checks

Portable viewport tests cover malformed rectangles, size bounds, limited padding and the SDK floor/game ceiling difference. Runtime source guards cover exact source acknowledgement, crop bounds and provider isolation. Native compilation retained the 16 KiB frame guard. The default converter's identical binary hash is recorded in the receipt.
