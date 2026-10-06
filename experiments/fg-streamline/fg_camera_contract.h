#pragma once
#include <stdint.h>
// Plain C ABI. Matrices are row major, without temporal jitter.
typedef struct MCD2FGCamera {
 uint32_t size,frame,width,height,reset;
 float viewToClip[16],clipToView[16],clipToPrevious[16],previousToClip[16];
 float position[3],up[3],right[3],forward[3],jitter[2];
 float nearPlane,farPlane,verticalFOV,aspectRatio;
} MCD2FGCamera;
#ifdef __cplusplus
static_assert(sizeof(MCD2FGCamera)==348);
#endif
