// Real Windows driver smoke test; no game hooks or frame generation.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <d3d12.h>
#include <nvsdk_ngx.h>
#include "../../src/native/ngx_driver.hpp"
#include <cassert>
#include <iostream>
int wmain(int argc,wchar_t** argv){
 if(argc!=3){std::cerr<<"Expected runtime directory and private cache directory\n";return 2;}
 auto module=mcd2::ngx::load_driver();if(!module){std::cerr<<"Driver unavailable\n";return 77;}
 ID3D12Device* device=nullptr;
 auto hr=D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,__uuidof(ID3D12Device),reinterpret_cast<void**>(&device));
 if(FAILED(hr)){FreeLibrary(module);std::cerr<<"D3D12 device unavailable\n";return 77;}
 using Init=NVSDK_NGX_Result(*)(const char*,NVSDK_NGX_EngineType,const char*,const wchar_t*,ID3D12Device*,NVSDK_NGX_Version,const NVSDK_NGX_FeatureCommonInfo*);
 using Shutdown=NVSDK_NGX_Result(*)(ID3D12Device*,unsigned&);
 auto init=reinterpret_cast<Init>(GetProcAddress(module,"NVSDK_NGX_D3D12_Init_ProjectID"));
 auto shutdown=reinterpret_cast<Shutdown>(GetProcAddress(module,"NVSDK_NGX_D3D12_Shutdown1"));
 assert(init&&shutdown);
 const wchar_t* paths[]={argv[1]};NVSDK_NGX_FeatureCommonInfo info{};
 info.PathListInfo.Path=paths;info.PathListInfo.Length=1;
 std::filesystem::create_directories(argv[2]);
 for(unsigned cycle=0;cycle<3;++cycle){
  auto result=init("27fe0b1b-1112-4466-b36e-dd330014e2ad",NVSDK_NGX_ENGINE_TYPE_CUSTOM,"mcd2-graphics-0.1",argv[2],device,NVSDK_NGX_Version_API,&info);
  std::cout<<"cycle="<<cycle<<" init="<<std::hex<<unsigned(result)<<std::endl;
  if(NVSDK_NGX_FAILED(result)){device->Release();FreeLibrary(module);return 3;}
  unsigned remaining=~0u;result=shutdown(device,remaining);
  std::cout<<"shutdown="<<std::hex<<unsigned(result)<<" remaining="<<remaining<<std::endl;
  assert(NVSDK_NGX_SUCCEED(result)&&remaining==0);
 }
 device->Release();assert(FreeLibrary(module));
}
