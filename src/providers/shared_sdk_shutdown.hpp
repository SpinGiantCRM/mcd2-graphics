#pragma once

namespace mcd2::providers {
// These inputs are observations of completed retirement, never user settings.
// An unavailable ownership query must be treated as false.
struct SharedSdkShutdownAdmission {
    bool terminal = false;
    bool ownerClosing = false;
    bool amdPresenterRetired = false;
    bool nativeNgxRetired = false;
    bool sdkHasDeviceWithoutNvidiaFg = false;
    bool alreadyIssued = false;
};
constexpr bool admitSharedSdkShutdown(const SharedSdkShutdownAdmission& a) {
    return !a.terminal && a.ownerClosing && a.amdPresenterRetired &&
        a.nativeNgxRetired && a.sdkHasDeviceWithoutNvidiaFg && !a.alreadyIssued;
}
struct NvidiaSdkShutdownAdmission {
    bool terminal = false;
    bool ownerClosing = false;
    bool nvidiaOwnerStopped = false;
    bool alphaGpuRetired = false;
    bool nativeNgxRetired = false;
    bool alreadyIssued = false;
};
constexpr bool admitNvidiaSdkShutdown(const NvidiaSdkShutdownAdmission& a) {
    return !a.terminal && a.ownerClosing && a.nvidiaOwnerStopped &&
        a.alphaGpuRetired && a.nativeNgxRetired && !a.alreadyIssued;
}
}
