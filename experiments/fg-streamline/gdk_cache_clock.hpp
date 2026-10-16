#pragma once
#include "../../src/gdk/cache_time.hpp"
inline uint64_t fg_cache_fresh_budget(){
 HMODULE module=nullptr;if(!GetModuleHandleExW(0,L"mcd2-gdk-save-bridge.addon64",&module))return mcd2::gdk::freshBudgetTicks(0);
 using Clock=uint32_t(*)();auto clock=reinterpret_cast<Clock>(GetProcAddress(module,"mcd2_gdk_cache_clock_resolution_v1"));
 const auto resolution=clock?clock():0;FreeLibrary(module);return mcd2::gdk::freshBudgetTicks(resolution);
}
