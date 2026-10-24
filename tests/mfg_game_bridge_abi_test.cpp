#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include "../experiments/providers/mfg_game_bridge.h"
#include "../experiments/providers/mfg_game_parameters.hpp"
int wmain(int argc,wchar_t** argv){
 MfgGameParameters p;float farValue{};unsigned integer{};p.Set("DLSSG.CameraFar",std::numeric_limits<float>::infinity());if(!p.healthy()||p.Get("DLSSG.CameraFar",&farValue)!=NVSDK_NGX_Result_Success||!std::isinf(farValue)||p.Get("DLSSG.CameraFar",&integer)==NVSDK_NGX_Result_Success)return 1;
 p.Set("DLSSG.CameraFar",1000.f);if(p.Get("DLSSG.CameraFar",&farValue)!=NVSDK_NGX_Result_Success||farValue!=1000.f)return 1;p.Set("Other",std::numeric_limits<float>::infinity());if(p.healthy())return 1;
 p.Reset();p.Set("DLSSG.CameraFar",-std::numeric_limits<float>::infinity());if(p.healthy())return 1;
 if(argc!=2)return 2;auto module=LoadLibraryW(argv[1]);if(!module){std::fprintf(stderr,"missing bridge\n");return 1;}
 const char* names[]={"mcd2_mfg_load_v1","mcd2_mfg_capabilities_v1","mcd2_mfg_configure_v1","mcd2_mfg_configure_v2","mcd2_mfg_state_v1","mcd2_mfg_timing_v1","mcd2_afg_support_v1","mcd2_afg_swap_v1","mcd2_afg_recording_contract_v2","mcd2_afg_output_v1","mcd2_afg_scene_v1","mcd2_afg_guides_v2","mcd2_afg_world_v2","mcd2_afg_present_v2","mcd2_afg_state_v1","mcd2_afg_diagnostic_bind_v1","mcd2_afg_diagnostics_v2","mcd2_afg_retire_v1","mcd2_afg_presenter_switch_state_v1","mcd2_afg_antilag_ready_v1","mcd2_afg_antilag_v1"};
 for(auto name:names)if(!GetProcAddress(module,name)){std::fprintf(stderr,"missing %s\n",name);return 1;}
 auto configure=reinterpret_cast<int(*)(unsigned,unsigned)>(GetProcAddress(module,"mcd2_mfg_configure_v1"));
 auto configure2=reinterpret_cast<int(*)(unsigned,unsigned,unsigned,unsigned)>(GetProcAddress(module,"mcd2_mfg_configure_v2"));
 auto capabilities=reinterpret_cast<int(*)(void*,MCD2MfgCapabilitiesV1*)>(GetProcAddress(module,"mcd2_mfg_capabilities_v1"));
 auto load=reinterpret_cast<int(*)(const wchar_t*,const wchar_t*)>(GetProcAddress(module,"mcd2_mfg_load_v1"));
 auto contract=reinterpret_cast<unsigned(*)()>(GetProcAddress(module,"mcd2_afg_recording_contract_v2"));
 MCD2MfgCapabilitiesV1 caps{};caps.size=sizeof(caps);
 if(contract()!=2||configure(3,6)==0||configure(2,4)==0||configure(99,6)==0||capabilities(nullptr,&caps)==0||load(L"relative.dll",nullptr)==0)return 1;
 if(configure(0,1)!=0||configure2(0,1,0,0)!=0||configure2(2,6,2,3)==0||configure2(3,6,0,5)==0||configure2(0,1,1,0)==0)return 1;
 FILE* receipt{};fopen_s(&receipt,"abi-passed.txt","w");if(!receipt)return 1;std::fputs("MFG bridge ABI and fail-closed startup passed\n",receipt);std::fclose(receipt);return 0;
}
