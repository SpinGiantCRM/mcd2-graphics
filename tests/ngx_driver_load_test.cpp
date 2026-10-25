// Windows hardware smoke test; exit 77 means NGX is absent on this machine.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../src/native/ngx_driver.hpp"
#include <cassert>
#include <iostream>
int main(){
 const auto module=mcd2::ngx::load_driver();
 if(!module){std::cout<<"NGX driver unavailable\n";return 77;}
 for(auto name:{"NVSDK_NGX_D3D12_Init_ProjectID","NVSDK_NGX_D3D12_GetCapabilityParameters",
                "NVSDK_NGX_D3D12_CreateFeature","NVSDK_NGX_D3D12_EvaluateFeature"})
  assert(GetProcAddress(module,name));
 assert(FreeLibrary(module));std::cout<<"Registered NGX driver load and exports passed\n";
}
