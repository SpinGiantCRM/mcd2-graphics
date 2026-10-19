#pragma once
#include <stdint.h>
#include "../fg-streamline/fg_camera_contract.h"
#include "../../src/providers/runtime_diagnostics.h"
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
struct MCD2AmdFgOutputV1 {
 uint32_t size,width,height,backbufferFormat,sceneFormat,colourSpace,sceneStage,reserved;
};
static_assert(sizeof(MCD2AmdFgSwapV1)==72);
static_assert(sizeof(MCD2AmdFgGuidesV1)==364);
static_assert(sizeof(MCD2AmdFgStateV1)==72);
static_assert(sizeof(MCD2AmdFgOutputV1)==32);
inline constexpr int MCD2_AFG_BUSY_V2=1000001;
inline constexpr int MCD2_AFG_OUTPUT_UNSUPPORTED_V3=1000002;
extern "C" {
 int mcd2_afg_diagnostic_bind_v1(const MCD2FgDiagnosticBindingV1*);
 int mcd2_afg_diagnostics_v2(MCD2FgDiagnosticsV2*);
 int mcd2_afg_load_v1(const wchar_t *absoluteVerifiedRuntime);
 // Eligibility query only; does not create contexts, wrap or activate FG.
 int mcd2_afg_support_v1(void *nativeDevice);
 int mcd2_afg_swap_v1(void *factory,void *nativeQueue,void *hwnd,const MCD2AmdFgSwapV1*,void **swap);
 // Actual output contract; context changes drain old SDK and owned GPU work.
 // A busy retirement declines generation and retains its resources for retry.
 int mcd2_afg_scene_v1(void* device,void* world,uint64_t engineFrame);
 int mcd2_afg_output_v1(const MCD2AmdFgOutputV1*);
 // Recording contract 2 stages inputs without touching the host list. Present
 // must run after host submissions on the exact swapchain graphics queue.
 unsigned mcd2_afg_recording_contract_v2();
 int mcd2_afg_guides_v2(void *nativeCommand,void *depth,void *motion,const MCD2AmdFgGuidesV1*);
 int mcd2_afg_world_v2(void *nativeCommand,void *world,uint64_t engineFrame);
 int mcd2_afg_present_v2(void *nativeSubmittedQueue,uint64_t engineFrame,uint32_t wanted);
 int mcd2_afg_state_v1(MCD2AmdFgStateV1*);
 // Actual presenter, independent of startup policy: 0 unavailable, 1 native,
 // 2 AMD. Eligibility is not a completed switch or an activation receipt.
 int mcd2_afg_presenter_switch_state_v1(uint32_t *route,uint32_t *eligible);
 // Actual owned FSR presenter only. Clear metadata and drain presentation
 // before the caller releases its persistent Anti-Lag context.
 int mcd2_afg_antilag_ready_v1(void *nativeDevice);
 int mcd2_afg_antilag_v1(void *nativeDevice,void *antiLagContext,uint32_t enabled,uint32_t drain);
 // Signals/waits a fresh own queue fence, then releases only own recordings.
 // Failed completion proof retains the session and callbacks. Optional caller
 // fence evidence supplements, and never replaces, the internal queue proof.
 int mcd2_afg_retire_v1(void *postInvalidationFence,uint64_t completedValue);
}
