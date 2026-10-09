// Experimental early DXGI entry point. Not part of an installer or release.
// Own DllMain does no SDK work; first-factory caller context still needs review.
// The selected ReShade binary is staged as d3d12.asi. Its normal
// factory wrapper remains outside the Streamline presentation proxy.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <nvapi.h>
#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <filesystem>
#include <cstdio>
#include <cstring>
#include <intrin.h>
#include "fg_bridge_contract.h"
#include "../providers/fsr_fg_game_bridge.h"
#include "fg_ui_protocol.hpp"
#include "wine_reflex_pacing.hpp"
#include "../../src/providers/observed_settings.hpp"
#include "../../src/providers/fg_menu_projection.hpp"
#include <fstream>
extern "C" __declspec(dllimport) int mcd2_sl_init(const wchar_t*,const wchar_t*,const wchar_t*);
extern "C" __declspec(dllimport) int mcd2_sl_init_for_owner_v1(const wchar_t*,const wchar_t*,const wchar_t*,unsigned);
extern "C" __declspec(dllimport) int mcd2_fg_initialized();
extern "C" __declspec(dllimport) int mcd2_fg_upgrade(void**);
extern "C" __declspec(dllimport) int mcd2_sl_set_device(void*);
extern "C" __declspec(dllimport) int mcd2_fg_support(void*,unsigned);
extern "C" __declspec(dllimport) int mcd2_fg_configure(const MCD2FGConfig*);
extern "C" __declspec(dllimport) int mcd2_fg_configured();
namespace {
HMODULE self=nullptr,systemDxgi=nullptr,reshade=nullptr;
INIT_ONCE once=INIT_ONCE_STATIC_INIT,systemOnce=INIT_ONCE_STATIC_INIT;
thread_local bool initializing=false;
bool sdkReady=false,routeEnabled=true;FILE* receipt=nullptr;
unsigned fgSessionMode=1;
bool amdFgSession=false;HMODULE amdFgBridge=nullptr;
std::atomic<unsigned> actualFgOwner{0};
int(*amdLoad)(const wchar_t*)=nullptr;
int(*amdSwap)(void*,void*,void*,const MCD2AmdFgSwapV1*,void**)=nullptr;
void event(const char* stage,long result);
unsigned shared_startup_owner(){
 auto local=std::make_unique<wchar_t[]>(32768);auto n=GetEnvironmentVariableW(L"LOCALAPPDATA",local.get(),32768);if(!n||n>=32768)return 0;
 const auto path=std::filesystem::path(local.get())/L"Dungeons2"/L"Saved"/L"MCD2Graphics"/L"ProviderSettings";
 mcd2::providers::GraphicsStore store(path);mcd2::providers::DecodedGraphicsRecord record;
 mcd2::providers::FgMenuProjection projection;
 if(!mcd2::providers::store_detail::directoryReady(path)||store.load(record).status!=mcd2::providers::StoreStatus::Ok||
    !mcd2::providers::projectFgMenu(record,projection)||!projection.enabled||!projection.pairEligible)return 0;
 event("shared-FG-startup-revision",projection.revision);return projection.provider+1;
}
unsigned saved_fg_mode(){
 auto local=std::make_unique<wchar_t[]>(32768);auto n=GetEnvironmentVariableW(L"LOCALAPPDATA",local.get(),32768);if(!n||n>=32768)return 0;
 auto path=std::filesystem::path(local.get())/L"Dungeons2"/L"Saved"/L"SaveGames"/L"MCD2GraphicsFGSettings.sav";
 std::ifstream file(path,std::ios::binary|std::ios::ate);if(!file)return 0;auto size=file.tellg();if(size<64||size>8192)return 0;
 std::vector<uint8_t> bytes(static_cast<size_t>(size));file.seekg(0);if(!file.read(reinterpret_cast<char*>(bytes.data()),bytes.size()))return 0;
 mcd2::fgui::Intent intent{};return mcd2::fgui::decode(bytes,intent)?intent.mode:0;
}
unsigned observed_fg_mode(){
 auto local=std::make_unique<wchar_t[]>(32768);auto n=GetEnvironmentVariableW(L"LOCALAPPDATA",local.get(),32768);if(!n||n>=32768)return 0;
 const auto saved=std::filesystem::path(local.get())/L"Dungeons2"/L"Saved";
 mcd2::providers::LegacySnapshot snapshot;
 if(!mcd2::providers::readLegacySnapshot(saved/L"SaveGames",snapshot))return 0;
 mcd2::providers::GraphicsStore store(saved/L"MCD2Graphics"/L"ProviderMirror");
 auto session=(GetTickCount64()^(uint64_t(GetCurrentProcessId())<<32));if(!session)session=1;
 const auto selection=mcd2::providers::observedStartup(store,snapshot,session);
 event("observed-settings-valid",selection.valid?1:0);
 event("observed-settings-revision",selection.bootstrap.stamp.revision);
 // Only the already implemented NVIDIA owner can request this bootstrap.
 // Actual rendering-adapter/SDK approval still occurs in create_swap.
 return selection.valid&&selection.bootstrap.requested==mcd2::providers::PresentationOwner::NvidiaStreamline?1u:0u;
}
void event(const char* stage,long result){if(receipt){fprintf(receipt,"{\"stage\":\"%s\",\"result\":%ld}\n",stage,result);fflush(receipt);}}
BOOL CALLBACK system_init(PINIT_ONCE,void*,void**){
 auto path=std::make_unique<wchar_t[]>(32768);auto n=GetSystemDirectoryW(path.get(),32768);
 if(!n||n>=32750)return FALSE;wcscat_s(path.get(),32768,L"\\dxgi.dll");
 systemDxgi=LoadLibraryExW(path.get(),nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);return systemDxgi!=nullptr;
}
FARPROC native(const char* name){if(!InitOnceExecuteOnce(&systemOnce,system_init,nullptr,nullptr))return nullptr;return GetProcAddress(systemDxgi,name);}
void configure_wine_reflex_pacing(const wchar_t* policy){
 // Windows never enters this path. NVAPI has already been initialized by SL;
 // adapter enumeration does not create a D3D12 device. Apply before the CuBIN
 // capability probe or game device creation, when VKD3D reads its extensions.
 if(!GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"wine_get_version")||
    !GetPrivateProfileIntW(L"Compatibility",L"WineReflexPacing",1,policy))return;
 auto module=GetModuleHandleW(L"nvapi64.dll");if(!module)return;
 auto query=reinterpret_cast<void*(__cdecl*)(unsigned)>(GetProcAddress(module,"nvapi_QueryInterface"));if(!query)return;
 auto enumerate=reinterpret_cast<decltype(&NvAPI_EnumPhysicalGPUs)>(query(0xe5ac921f));
 NvPhysicalGpuHandle adapters[NVAPI_MAX_PHYSICAL_GPUS]{};NvU32 count=0;
 if(!enumerate||enumerate(adapters,&count)!=NVAPI_OK||!count)return;
 constexpr DWORD capacity=8192;
 auto existing=std::make_unique<wchar_t[]>(capacity);
 auto length=GetEnvironmentVariableW(L"VKD3D_DISABLE_EXTENSIONS",existing.get(),capacity);
 if(length>=capacity){event("wine-reflex-pacing",ERROR_INSUFFICIENT_BUFFER);return;}
 auto extensions=mcd2::compat::reflex_disabled_extensions({existing.get(),length});
 // Avoid the FIFO-created/dynamically-unlocked path that destabilizes Reflex
 // on the tested NVIDIA Wine driver. VSync still selects FIFO normally;
 // neither the present mode nor the user's frame limit is forced here.
 event("wine-reflex-pacing",SetEnvironmentVariableW(L"VKD3D_DISABLE_EXTENSIONS",extensions.c_str())?0:GetLastError());
}
long prime_wine_cubin_capability(){
 // DXVK-NVAPI discovers 64-bit CuBIN support by creating a temporary device.
 // Run the real query before ReShade's non-recursive D3D12 creation lock exists.
 // No shader/device is supplied and no capability result is overridden.
 if(!GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"wine_get_version"))return -100;
 auto module=GetModuleHandleW(L"nvapi64.dll");if(!module)return -101;
 auto query=reinterpret_cast<void*(__cdecl*)(unsigned)>(GetProcAddress(module,"nvapi_QueryInterface"));if(!query)return -102;
 auto version=reinterpret_cast<decltype(&NvAPI_GetInterfaceVersionString)>(query(0x01053fa5));
 NvAPI_ShortString text{};if(!version||version(text)!=NVAPI_OK||!std::strstr(text,"DXVK-NVAPI"))return -103;
 auto probe=reinterpret_cast<decltype(&NvAPI_D3D12_CreateCubinComputeShaderExV2)>(query(0x299f5fdc));if(!probe)return -104;
 NVAPI_D3D12_CREATE_CUBIN_SHADER_PARAMS params{};params.structSizeIn=sizeof(params);
 return probe(&params); // Expected invalid argument or unsupported; never success.
}
// Bounded multi-factory counterpart to the qualified single-factory probe.
// Routes hold COM references until explicit quiescent experiment shutdown.
// Device-loss/concurrent shutdown and unlimited factory churn are not qualified.
struct Route {
 using Create=HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory2*,IUnknown*,HWND,const DXGI_SWAP_CHAIN_DESC1*,const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*,IDXGIOutput*,IDXGISwapChain1**);
 IDXGIFactory7* base=nullptr;IDXGIFactory2* proxy=nullptr;void** original=nullptr;std::array<void*,32> table{};
 void detach(){if(original&&base){InterlockedExchangePointer(reinterpret_cast<void* volatile*>(base),original);original=nullptr;}if(proxy){proxy->Release();proxy=nullptr;}if(base){base->Release();base=nullptr;}}
};
std::mutex routesMutex;std::array<Route,16> routes{};thread_local Route* nested=nullptr;
HRESULT STDMETHODCALLTYPE create_swap(IDXGIFactory2* factory,IUnknown* queue,HWND hwnd,const DXGI_SWAP_CHAIN_DESC1* desc,const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreen,IDXGIOutput* output,IDXGISwapChain1** result){
 Route* route=nullptr;{std::lock_guard guard(routesMutex);for(auto& r:routes)if(r.base==factory){route=&r;break;}}
 if(!route)return E_UNEXPECTED;
 auto original=reinterpret_cast<Route::Create>(route->original[15]);
 if(nested==route)return original(factory,queue,hwnd,desc,fullscreen,output,result);
 if(amdFgSession){
  wchar_t ownerClass[256]{};DWORD pid=0;GetClassNameW(hwnd,ownerClass,256);GetWindowThreadProcessId(hwnd,&pid);
  if(!amdSwap||!desc||!result||output||pid!=GetCurrentProcessId()||wcscmp(ownerClass,L"UnrealWindow"))return original(factory,queue,hwnd,desc,fullscreen,output,result);
  MCD2AmdFgSwapV1 p{sizeof(p),desc->Width,desc->Height,unsigned(desc->Format),desc->SampleDesc.Count,desc->SampleDesc.Quality,desc->BufferUsage,desc->BufferCount,unsigned(desc->Scaling),unsigned(desc->SwapEffect),unsigned(desc->AlphaMode),desc->Flags,
   fullscreen?1u:0u,fullscreen?unsigned(fullscreen->Windowed):1u,fullscreen?fullscreen->RefreshRate.Numerator:0u,fullscreen?fullscreen->RefreshRate.Denominator:1u,fullscreen?unsigned(fullscreen->ScanlineOrdering):0u,fullscreen?unsigned(fullscreen->Scaling):0u};
  RECT client{};if((!p.width||!p.height)&&GetClientRect(hwnd,&client)){
   if(!p.width&&client.right>client.left)p.width=unsigned(client.right-client.left);
   if(!p.height&&client.bottom>client.top)p.height=unsigned(client.bottom-client.top);
  }
  if(receipt){fprintf(receipt,"{\"stage\":\"AMD-swap-description\",\"width\":%u,\"height\":%u,\"format\":%u,\"samples\":%u,\"buffers\":%u}\n",p.width,p.height,p.format,p.samples,p.buffers);fflush(receipt);}
  ID3D12CommandQueue* nativeQueue=nullptr;if(!queue||FAILED(queue->QueryInterface(IID_PPV_ARGS(&nativeQueue))))return E_INVALIDARG;
  auto previous=nested;nested=route;
  void* out=nullptr;const auto status=amdSwap(factory,nativeQueue,hwnd,&p,&out);nativeQueue->Release();nested=previous;
  event("AMD-owned-swapchain",status);
  if(!status&&out){*result=static_cast<IDXGISwapChain1*>(out);actualFgOwner=2;return S_OK;}
  // A failed SDK construction never makes a later NVIDIA owner silently active.
  return original(factory,queue,hwnd,desc,fullscreen,output,result);
 }
 // The actual queue identifies the rendering adapter. Bind before the SDK's
 // swapchain hooks run, rather than waiting for ReShade's post-create event.
 ID3D12CommandQueue* commandQueue=nullptr;ID3D12Device* device=nullptr;
 auto deviceResult=queue?queue->QueryInterface(IID_PPV_ARGS(&commandQueue)):E_INVALIDARG;
 if(SUCCEEDED(deviceResult))deviceResult=commandQueue->GetDevice(IID_PPV_ARGS(&device));
 if(commandQueue)commandQueue->Release();
 int support=-1,binding=-1;
 if(device){auto luid=device->GetAdapterLuid();support=mcd2_fg_support(&luid,sizeof(luid));if(!support)binding=mcd2_sl_set_device(device);device->Release();}
 event("swapchain-rendering-adapter-support",support);event("pre-swapchain-device-binding",binding);
 if(FAILED(deviceResult)||support||binding)return original(factory,queue,hwnd,desc,fullscreen,output,result);
 if(!desc)return E_INVALIDARG;
 const MCD2FGConfig config{sizeof(MCD2FGConfig),0,desc->Width,desc->Height,desc->Width,desc->Height,unsigned(desc->Format),10,41,34,desc->BufferCount,1};
 // Only the initial default. A later swapchain recreation must preserve the
 // experiment owner's explicitly requested mode instead of resetting it.
 auto configured=mcd2_fg_configured()?0:mcd2_fg_configure(&config);event("initial-FG-configuration",configured);
 if(configured)return original(factory,queue,hwnd,desc,fullscreen,output,result);
 auto previous=nested;nested=route;
 auto hr=route->proxy->CreateSwapChainForHwnd(queue,hwnd,desc,fullscreen,output,result);
 nested=previous;if(SUCCEEDED(hr)&&result&&*result)actualFgOwner=1;event("routed-hwnd-swapchain",hr);return hr;
}
bool attach(IUnknown* outer){
 constexpr GUID unwrap={0x7f2c9a11,0x3b4e,0x4d6a,{0x81,0x2f,0x5e,0x9c,0xd3,0x7a,0x1b,0x42}};
 IDXGIFactory* raw=nullptr;IDXGIFactory7* base=nullptr;
 if(FAILED(outer->QueryInterface(unwrap,reinterpret_cast<void**>(&raw)))||!raw)return false;
 auto hr=raw->QueryInterface(IID_PPV_ARGS(&base));raw->Release();if(FAILED(hr)||!base)return false;
 std::lock_guard guard(routesMutex);Route* target=nullptr;
 for(auto& r:routes){if(r.base==base){base->Release();return true;}if(!r.base&&!target)target=&r;}
 if(!target){base->Release();return false;}target->base=base;
 if(!amdFgSession&&(FAILED(base->QueryInterface(IID_PPV_ARGS(&target->proxy)))||mcd2_fg_upgrade(reinterpret_cast<void**>(&target->proxy)))){target->detach();return false;}
 target->original=*reinterpret_cast<void***>(base);std::memcpy(target->table.data(),target->original,sizeof(target->table));target->table[15]=reinterpret_cast<void*>(create_swap);
 InterlockedExchangePointer(reinterpret_cast<void* volatile*>(base),target->table.data());return true;
}
BOOL CALLBACK initialize(PINIT_ONCE,void*,void**){
 initializing=true;
 // All paths are derived from this module, not the process working directory.
 auto filename=std::make_unique<wchar_t[]>(32768);auto n=GetModuleFileNameW(self,filename.get(),32768);
 if(!n||n>=32768){initializing=false;return FALSE;}
 auto folder=std::filesystem::path(filename.get()).parent_path();
 fopen_s(&receipt,(folder/"bootstrap.jsonl").string().c_str(),"w");
 // Local isolation controls are intentionally absent from the released UI.
 auto policy=mcd2::providers::observedPolicyPath(filename.get()).wstring();
 if(GetPrivateProfileIntW(L"Experiment",L"TraceNvapi",0,policy.c_str())){
  SetEnvironmentVariableW(L"DXVK_NVAPI_LOG_LEVEL",L"trace");
  SetEnvironmentVariableW(L"DXVK_NVAPI_LOG_PATH",folder.c_str());
 }
 routeEnabled=GetPrivateProfileIntW(L"Experiment",L"FactoryRouting",1,policy.c_str())!=0;
 const bool factoryRoutingRequested=routeEnabled;
 // An Off startup must create the normal swapchain. A retained FG proxy still
 // copies/paces frames while generation is Off. Until engine-owned recreation
 // is qualified, the native toggle saves the next-launch presentation mode.
 auto uiPolicy=(folder/L"FGGuideCapture.ini").wstring();
 const bool sharedMenu=GetPrivateProfileIntW(L"Providers",L"ConsolidatedMenuTransport",0,policy.c_str())==1;
 unsigned sharedOwner=0;
 if(sharedMenu){sharedOwner=shared_startup_owner();fgSessionMode=sharedOwner?1:0;routeEnabled=routeEnabled&&fgSessionMode==1;}
 else if(GetPrivateProfileIntW(L"Providers",L"ObservedStartup",0,policy.c_str())==1){
  // An invalid/missing mirror must not fall back to the legacy FG-only slot.
  fgSessionMode=observed_fg_mode();routeEnabled=routeEnabled&&fgSessionMode==1;
 }else if(GetPrivateProfileIntW(L"Capture",L"NativeUIToggle",0,uiPolicy.c_str())==1){fgSessionMode=saved_fg_mode();routeEnabled=routeEnabled&&fgSessionMode==1;}
 // Explicit private trial only. Ordinary startup/migration remains unchanged.
 const bool privateAmdTrial=GetPrivateProfileIntW(L"Providers",L"ExperimentalAmdFG",0,policy.c_str())==1;
 amdFgSession=privateAmdTrial||(sharedMenu&&sharedOwner==2);
 if(amdFgSession||sharedMenu){
  amdFgBridge=LoadLibraryExW((folder/L"mcd2-fsr-fg-game-bridge.dll").c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
  amdLoad=amdFgBridge?reinterpret_cast<decltype(amdLoad)>(GetProcAddress(amdFgBridge,"mcd2_afg_load_v1")):nullptr;
  amdSwap=amdFgBridge?reinterpret_cast<decltype(amdSwap)>(GetProcAddress(amdFgBridge,"mcd2_afg_swap_v1")):nullptr;
  auto contract=amdFgBridge?reinterpret_cast<unsigned(*)()>(GetProcAddress(amdFgBridge,"mcd2_afg_recording_contract_v2")):nullptr;
  const bool ownRecordings=contract&&contract()==2&&GetProcAddress(amdFgBridge,"mcd2_afg_guides_v2")&&GetProcAddress(amdFgBridge,"mcd2_afg_world_v2")&&GetProcAddress(amdFgBridge,"mcd2_afg_present_v2");
  const auto loaded=ownRecordings&&amdLoad&&amdSwap?amdLoad((folder/L"amd_fidelityfx_framegeneration_dx12.dll").c_str()):-1;
  event("AMD-pinned-runtime",loaded);
  if(amdFgSession){routeEnabled=factoryRoutingRequested&&!loaded&&(privateAmdTrial||sharedOwner==2);fgSessionMode=routeEnabled?1:0;}
 }
 event("FG-session-mode",fgSessionMode);
 auto enableSdk=GetPrivateProfileIntW(L"Experiment",L"EnableSDK",1,policy.c_str())!=0;
 auto result=!enableSdk?-100:(mcd2_fg_initialized()?0:mcd2_sl_init_for_owner_v1((folder/L"sl.interposer.dll").c_str(),folder.c_str(),folder.c_str(),sharedOwner));
 event("Streamline-NVIDIA-FG-plugin-requested",sharedOwner==2?0:1);
 sdkReady=result==0;event("early-SDK-init",result);
 if(sdkReady)configure_wine_reflex_pacing(policy.c_str());
 if(sdkReady&&GetPrivateProfileIntW(L"Experiment",L"PrimeWineCubin",1,policy.c_str()))event("wine-cubin-capability-prime",prime_wine_cubin_capability());
 // Resolve system DXGI before ReShade registers its hooks, including when the
 // SDK is disabled. The Direct3D stem avoids implicit OpenGL hooks.
 native("CreateDXGIFactory1");
 reshade=LoadLibraryExW((folder/L"d3d12.asi").c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
 event("qualified-ReShade-load",reshade?0:GetLastError());initializing=false;
 return TRUE; // Failure is stable for this session; no repeated SDK loading.
}
FARPROC factory(const char* name){
 if(initializing)return native(name);
 if(!InitOnceExecuteOnce(&once,initialize,nullptr,nullptr))return native(name);
 return reshade?GetProcAddress(reshade,name):native(name);
}
bool host_caller(void* caller){
 auto image=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(image);auto nt=reinterpret_cast<IMAGE_NT_HEADERS*>(image+dos->e_lfanew);
 auto address=reinterpret_cast<uintptr_t>(caller);return address>=image&&address<image+nt->OptionalHeader.SizeOfImage;
}
void route_result(HRESULT hr,void** out,void* caller){
 // Do not retain or route factories created internally by the SDK, overlays
 // or other DLLs. They are not this experiment's world presentation owner.
 if(SUCCEEDED(hr)&&out&&*out&&(amdFgSession||sdkReady)&&routeEnabled&&host_caller(caller)){auto ok=attach(static_cast<IUnknown*>(*out));event("factory-route",ok?0:-1);}
}
}
extern "C" HRESULT WINAPI CreateDXGIFactory(REFIID iid,void** out){auto f=reinterpret_cast<HRESULT(WINAPI*)(REFIID,void**)>(factory("CreateDXGIFactory"));if(!f)return E_NOINTERFACE;auto hr=f(iid,out);if(!initializing)route_result(hr,out,_ReturnAddress());return hr;}
extern "C" HRESULT WINAPI CreateDXGIFactory1(REFIID iid,void** out){auto f=reinterpret_cast<HRESULT(WINAPI*)(REFIID,void**)>(factory("CreateDXGIFactory1"));if(!f)return E_NOINTERFACE;auto hr=f(iid,out);if(!initializing)route_result(hr,out,_ReturnAddress());return hr;}
extern "C" HRESULT WINAPI CreateDXGIFactory2(UINT flags,REFIID iid,void** out){auto f=reinterpret_cast<HRESULT(WINAPI*)(UINT,REFIID,void**)>(factory("CreateDXGIFactory2"));if(!f)return E_NOINTERFACE;auto hr=f(flags,iid,out);if(!initializing)route_result(hr,out,_ReturnAddress());return hr;}
extern "C" HRESULT WINAPI DXGIDeclareAdapterRemovalSupport(){auto f=reinterpret_cast<HRESULT(WINAPI*)()>(native("DXGIDeclareAdapterRemovalSupport"));return f?f():E_NOINTERFACE;}
extern "C" HRESULT WINAPI DXGIGetDebugInterface1(UINT flags,REFIID iid,void** out){auto f=reinterpret_cast<HRESULT(WINAPI*)(UINT,REFIID,void**)>(native("DXGIGetDebugInterface1"));return f?f(flags,iid,out):E_NOINTERFACE;}
extern "C" __declspec(dllexport) void mcd2_bootstrap_detach(){std::lock_guard guard(routesMutex);for(auto& r:routes)r.detach();event("factory-routes-detached",0);if(receipt){fclose(receipt);receipt=nullptr;}}
extern "C" __declspec(dllexport) unsigned mcd2_bootstrap_fg_session_mode(){return fgSessionMode;}
extern "C" __declspec(dllexport) unsigned mcd2_bootstrap_amd_fg_session(){return amdFgSession&&routeEnabled?1:0;}
extern "C" __declspec(dllexport) unsigned mcd2_bootstrap_fg_owner(){return actualFgOwner.load();}
BOOL APIENTRY DllMain(HMODULE module,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH)self=module;return TRUE;}
