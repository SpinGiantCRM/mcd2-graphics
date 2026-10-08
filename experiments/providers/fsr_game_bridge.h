#pragma once
#include <stdint.h>
// Plain C ABI only: SDK descriptors and Microsoft COM calls stay in the bridge.
// Every texture and command recording remains owned by the calling generation.
struct MCD2FsrDispatchV1 {
 uint32_t size,renderWidth,renderHeight,outputWidth,outputHeight,reset;
 float jitterX,jitterY,motionScaleX,motionScaleY,frameTimeMs,preExposure;
 float cameraNear,cameraFar,verticalFov,viewSpaceToMeters;
};
struct MCD2FsrInfoV1 {
 uint32_t size,errors,warnings,reserved;
 uint64_t providerId,requiredResources,optionalResources,dispatches;
};
static_assert(sizeof(MCD2FsrDispatchV1)==64);
static_assert(sizeof(MCD2FsrInfoV1)==48);
extern "C" {
 int mcd2_fsr_create_v1(void *device,const wchar_t *verifiedRuntime,
  uint32_t renderWidth,uint32_t renderHeight,uint32_t outputWidth,uint32_t outputHeight,void **session);
 int mcd2_fsr_dispatch_v1(void *session,void *commandList,void *colour,void *depth,
  void *motion,void *exposure,void *output,const MCD2FsrDispatchV1 *parameters);
 int mcd2_fsr_info_v1(void *session,MCD2FsrInfoV1 *info);
 // Caller must first invalidate all recordings and signal a fresh queue fence.
 // Failure preserves the context/module. Successful destroy consumes the session.
 int mcd2_fsr_destroy_v1(void *session,void *postInvalidationFence,uint64_t requiredValue);
}
