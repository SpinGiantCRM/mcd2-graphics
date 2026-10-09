#pragma once
#include <stdint.h>
#include "../fg-streamline/fg_camera_contract.h"
// Plain C boundary. The caller supplies native COM objects, never vendor structs.
struct MCD2AmdFgSwapV1 {
 uint32_t size,width,height,format,samples,sampleQuality,usage,buffers,scaling,effect,alpha,flags;
 uint32_t fullscreenProvided,windowed,refreshNumerator,refreshDenominator,scanline,modeScaling;
};
struct MCD2AmdFgGuidesV1 {
 uint32_t size,reserved;float frameTimeMs,worldToMeters;
 MCD2FGCamera camera;
};
struct MCD2AmdFgStateV1 {
 uint32_t size,ready,active,fault,errors,warnings;
 uint64_t engineFrame,providerFrame,prepared,images,realPresents,generatedPresents;
};
static_assert(sizeof(MCD2AmdFgSwapV1)==72);
static_assert(sizeof(MCD2AmdFgGuidesV1)==364);
static_assert(sizeof(MCD2AmdFgStateV1)==72);
extern "C" {
 int mcd2_afg_load_v1(const wchar_t *absoluteVerifiedRuntime);
 int mcd2_afg_swap_v1(void *factory,void *nativeQueue,void *hwnd,const MCD2AmdFgSwapV1*,void **swap);
 int mcd2_afg_guides_v1(void *nativeCommand,void *depth,void *motion,const MCD2AmdFgGuidesV1*);
 int mcd2_afg_world_v1(void *nativeCommand,void *world,uint64_t engineFrame);
 int mcd2_afg_present_v1(uint64_t engineFrame,uint32_t wanted);
 int mcd2_afg_state_v1(MCD2AmdFgStateV1*);
 // Confirms successful native Reset epochs, then signals/waits a fresh own fence.
 // Missing invalidation proof retains the session and callbacks. Optional caller
 // fence evidence supplements, and never replaces, the internal queue proof.
 int mcd2_afg_retire_v1(void *postInvalidationFence,uint64_t completedValue);
}
