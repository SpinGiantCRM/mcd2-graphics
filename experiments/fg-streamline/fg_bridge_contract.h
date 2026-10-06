#pragma once
#include <stdint.h>
// Plain C, fixed-width boundary. Formats are actual DXGI_FORMAT values.
typedef struct MCD2FGConfig {
 uint32_t size,mode,width,height,motionWidth,motionHeight;
 uint32_t colorFormat,uiFormat,depthFormat,motionFormat,backBuffers,generatedFrames;
} MCD2FGConfig;
#ifdef __cplusplus
static_assert(sizeof(MCD2FGConfig)==48);
#endif
