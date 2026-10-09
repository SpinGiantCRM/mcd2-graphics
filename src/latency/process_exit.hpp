#pragma once
#include <windows.h>
#include <atomic>
#include <bit>
#include "process_lifetime.hpp"

namespace mcd2::process_exit {
using ShutdownQuery = BOOLEAN (WINAPI*)();
inline ShutdownQuery query = nullptr;
inline std::atomic<bool> detachTermination = false;
// Resolve before device callbacks, outside DllMain. Device destruction can be
// triggered by a vendor DLL's detach before this addon's own detach arrives.
inline void initialize() {
    auto module = GetModuleHandleW(L"ntdll.dll");
    query = module ? std::bit_cast<ShutdownQuery>(
        GetProcAddress(module, "RtlDllShutdownInProgress")) : nullptr;
}
inline bool terminating() {
    return detachTermination.load(std::memory_order_relaxed) || (query && query());
}
inline void mark_terminating() { detachTermination.store(true, std::memory_order_relaxed); }
}
