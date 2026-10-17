#pragma once
#include <stdint.h>
// Plain C, fixed-width boundary. Formats are actual DXGI_FORMAT values.
// generatedFrames: 1..5 for fixed 2x..6x, 0 for native Dynamic MFG.
typedef struct MCD2FGConfig {
 uint32_t size,mode,width,height,motionWidth,motionHeight;
 uint32_t colorFormat,uiFormat,depthFormat,motionFormat,backBuffers,generatedFrames;
} MCD2FGConfig;
#ifdef __cplusplus
static_assert(sizeof(MCD2FGConfig)==48);
#endif
