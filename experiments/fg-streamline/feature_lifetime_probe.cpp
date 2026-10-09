// Isolated startup/teardown probe. Never copy this executable into the game.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <cstdio>
#include <filesystem>
#include <memory>
extern "C" {
int mcd2_sl_init_for_owner_v1(const wchar_t*,const wchar_t*,const wchar_t*,unsigned);
int mcd2_sl_set_device(void*);int mcd2_sl_mode(unsigned);
int mcd2_fg_support(void*,unsigned);int mcd2_fg_ngx_owner_v1(void*);
int mcd2_sl_shutdown();void mcd2_fg_unload();
}
int main(){
 auto name=std::make_unique<wchar_t[]>(32768);
 auto n=GetModuleFileNameW(nullptr,name.get(),32768);if(!n||n>=32768)return 2;
 const auto folder=std::filesystem::path(name.get()).parent_path();
 unsigned owner=2;wchar_t input[8]{};
 if(GetEnvironmentVariableW(L"MCD2_FEATURE_OWNER",input,8))owner=unsigned(input[0]-L'0');
 if(owner>2)return 3;
 FILE* log=nullptr;fopen_s(&log,(folder/L"feature-lifetime.jsonl").string().c_str(),"w");if(!log)return 4;
 auto emit=[&](const char*stage,int result){fprintf(log,"{\"stage\":\"%s\",\"result\":%d,\"owner\":%u}\n",stage,result,owner);fflush(log);};
 auto invalid=mcd2_sl_init_for_owner_v1((folder/L"sl.interposer.dll").c_str(),folder.c_str(),folder.c_str(),3);
 emit("invalid-owner",invalid);if(invalid!=-30)return 5;
 auto result=mcd2_sl_init_for_owner_v1((folder/L"sl.interposer.dll").c_str(),folder.c_str(),folder.c_str(),owner);
 emit("owner-init",result);if(result)return 10;
 invalid=mcd2_sl_init_for_owner_v1((folder/L"sl.interposer.dll").c_str(),folder.c_str(),folder.c_str(),owner);
 emit("duplicate-init",invalid);if(invalid!=-30)return 6;
 auto loaded=GetModuleHandleW(L"sl.dlss_g.dll")!=nullptr;emit("NVIDIA-FG-plugin-loaded",loaded?1:0);
 if(owner==2&&loaded)return 11;
 IDXGIFactory1* factory=nullptr;IDXGIAdapter1* adapter=nullptr;ID3D12Device* device=nullptr;
 if(FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))return 12;
 for(UINT index=0;index<16;++index){
  IDXGIAdapter1* next=nullptr;if(FAILED(factory->EnumAdapters1(index,&next)))break;
  DXGI_ADAPTER_DESC1 desc{};
  if(SUCCEEDED(next->GetDesc1(&desc))&&!(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)&&
     SUCCEEDED(D3D12CreateDevice(next,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)))){adapter=next;break;}
  next->Release();
 }
 factory->Release();if(!adapter||!device)return 13;
 DXGI_ADAPTER_DESC1 desc{};if(FAILED(adapter->GetDesc1(&desc)))return 14;
 emit("real-adapter-support",mcd2_fg_support(&desc.AdapterLuid,sizeof(desc.AdapterLuid)));
 result=mcd2_sl_set_device(device);emit("set-device",result);if(result)return 15;
 result=mcd2_fg_ngx_owner_v1(device);emit("NGX-owner",result);if(owner==2&&result!=0)return 16;
 result=mcd2_sl_mode(0);emit("Reflex-Off",result);if(result)return 17;
 result=mcd2_sl_shutdown();emit("shutdown",result);if(result)return 18;
 device->Release();adapter->Release();mcd2_fg_unload();fclose(log);return 0;
}
