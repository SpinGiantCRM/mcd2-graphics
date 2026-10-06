// Optional read-only observer for the synthetic host, never installed in MCD2.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <reshade.hpp>
#include <cstdio>
#include <mutex>
namespace a=reshade::api;
namespace {
FILE* file=nullptr;std::mutex lock;
unsigned devices=0,presents=0,finishes=0,submissions=0,verified=0;
// Same pinned ReShade 6.8 command-list identity check used by the SR adapter.
// This is deliberately private-version-specific, not a portable SDK accessor.
struct Layout:ID3D12GraphicsCommandList,a::command_list{};
void init(a::device* d){std::lock_guard guard(lock);++devices;if(file){fprintf(file,"{\"stage\":\"device\",\"d3d12\":%u,\"nativeNonzero\":%u}\n",unsigned(d->get_api()==a::device_api::d3d12),unsigned(d->get_native()!=0));fflush(file);}}
void execute(a::command_queue*,a::command_list* c){
 if(c->get_device()->get_api()!=a::device_api::d3d12)return;
 constexpr GUID proxyId={0x479b29e3,0x9a2c,0x11d0,{0xb6,0x96,0x00,0xa0,0xc9,0x03,0x48,0x7a}};
 constexpr GUID unwrapId={0x7f2c9a11,0x3b4e,0x4d6a,{0x81,0x2f,0x5e,0x9c,0xd3,0x7a,0x1b,0x42}};
 auto candidate=static_cast<ID3D12GraphicsCommandList*>(static_cast<Layout*>(c));
 ID3D12GraphicsCommandList *proxy=nullptr,*native=nullptr;bool good=false;
 if(SUCCEEDED(candidate->QueryInterface(proxyId,reinterpret_cast<void**>(&proxy)))&&proxy)
  good=SUCCEEDED(proxy->QueryInterface(unwrapId,reinterpret_cast<void**>(&native)))&&native&&reinterpret_cast<uint64_t>(native)==c->get_native();
 if(native)native->Release();if(proxy)proxy->Release();
 std::lock_guard guard(lock);++submissions;if(good)++verified;
}
void present(a::command_queue*,a::swapchain*,const a::rect*,const a::rect*,unsigned,const a::rect*){std::lock_guard guard(lock);++presents;}
void finish(a::command_queue*,a::swapchain*){std::lock_guard guard(lock);++finishes;}
}
extern "C" __declspec(dllexport) const char* NAME="Isolated FG proxy-chain observer";
extern "C" __declspec(dllexport) const char* DESCRIPTION="Read-only synthetic host evidence; no game integration";
extern "C" __declspec(dllexport) bool AddonInit(HMODULE,HMODULE){fopen_s(&file,"chain-observer.jsonl","w");return file!=nullptr;}
extern "C" __declspec(dllexport) void AddonUninit(HMODULE,HMODULE){std::lock_guard guard(lock);if(file){fprintf(file,"{\"stage\":\"complete\",\"devices\":%u,\"presents\":%u,\"finishes\":%u,\"submissions\":%u,\"nativeIdentityMatches\":%u}\n",devices,presents,finishes,submissions,verified);fclose(file);file=nullptr;}}
BOOL APIENTRY DllMain(HMODULE module,DWORD reason,LPVOID){
 if(reason==DLL_PROCESS_ATTACH){if(!reshade::register_addon(module))return FALSE;
  reshade::register_event<reshade::addon_event::init_device>(init);
  reshade::register_event<reshade::addon_event::execute_command_list>(execute);
  reshade::register_event<reshade::addon_event::present>(present);
  reshade::register_event<reshade::addon_event::finish_present>(finish);
 }else if(reason==DLL_PROCESS_DETACH)reshade::unregister_addon(module);return TRUE;
}
