#pragma once
#include "fsr_fg_game_bridge.h"
// C calling convention, Microsoft x64 ABI. Capability is not activation.
struct MCD2MfgCapabilitiesV1 {
 uint32_t size,softwareReady,nativeReady,softwareGeneratedMax,nativeGeneratedMax;
 uint32_t softwareMultiplierMask,mixedMultiplierMask,reserved;
};
struct MCD2MfgStateV1 {
 uint32_t size,ready,active,fault,strategy,multiplier,restartRequired,reserved;
 MCD2DiagnosticIdentityV1 identity;
 uint64_t epoch,previousReal,currentReal,observedAtMs;
 uint64_t softwareGenerated,nativeGenerated,softwarePresented,nativePresented,realPresented;
};
struct MCD2MfgTimingV1 {
 uint32_t size,status,refreshMilliHz,phaseSync; // status: 0 unarmed, 1 paced, 2 refresh limiter, 3 unknown output, 4 failed, 5 missed cadence admission
 uint64_t observedRealPeriodNs,scheduledRealPeriodNs,lastActualPresentNs,acceptedRealFrames,completedRealFrames;
};
static_assert(sizeof(MCD2MfgTimingV1)==56);
static_assert(sizeof(MCD2MfgCapabilitiesV1)==32);
static_assert(sizeof(MCD2MfgStateV1)==120);
extern "C" {
 int mcd2_mfg_load_v1(const wchar_t* absoluteSoftwarePath,const wchar_t* absoluteNativePathOrNull);
 int mcd2_mfg_capabilities_v1(void* actualDevice,MCD2MfgCapabilitiesV1*);
 // Off=0/1; Hybrid=2 with 4 or 6; SoftwareMfg=3 with 2..6. No forced cap.
 int mcd2_mfg_configure_v1(unsigned strategy,unsigned multiplier);
 int mcd2_mfg_state_v1(MCD2MfgStateV1*);
 int mcd2_mfg_timing_v1(MCD2MfgTimingV1*);
}
