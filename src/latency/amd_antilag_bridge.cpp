#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include "../../third-party/AntiLag2/ffx_antilag2_dx12.h"

// Compile against the official, unmodified SDK with Microsoft's COM ABI.
// Controller serializes calls, including release before device destruction.
namespace {
AMD::AntiLag2DX12::Context context{};
void* boundDevice=nullptr;bool fgBound=false;
using Ready=int(*)(void*);
using Publish=int(*)(void*,void*,unsigned,unsigned);
Ready ready=nullptr;Publish publish=nullptr;
void findPresenter(){
 auto module=GetModuleHandleW(L"mcd2-fsr-fg-game-bridge.dll");
 ready=module?reinterpret_cast<Ready>(GetProcAddress(module,"mcd2_afg_antilag_ready_v1")):nullptr;
 publish=module?reinterpret_cast<Publish>(GetProcAddress(module,"mcd2_afg_antilag_v1")):nullptr;
}
bool clearAndDrain(){
 if(!fgBound)return true;
 // Failure keeps both the SDK context and this module mapped: presentation
 // workers may still be reading the persistent context pointer.
 if(!publish||publish(boundDevice,nullptr,0,1)!=0)return false;
 fgBound=false;return true;
}
}
extern "C" __declspec(dllexport) unsigned mcd2_al2_abi() { return 2; }
extern "C" __declspec(dllexport) HRESULT mcd2_al2_init(void* device) {
    if (context.m_pAntiLagAPI || !device) return E_INVALIDARG;
    const auto result = AMD::AntiLag2DX12::Initialize(&context, static_cast<ID3D12Device*>(device));
    if (result != S_OK) return result;
    if(!context.m_pAntiLagAPI)return E_NOINTERFACE;
    boundDevice=device;return S_OK;
}
extern "C" __declspec(dllexport) unsigned mcd2_al2_fg_supported(){
    if(!context.m_pAntiLagAPI)return 0;
    findPresenter();return ready&&publish&&ready(boundDevice)==0?1u:0u;
}
extern "C" __declspec(dllexport) HRESULT mcd2_al2_frame_generation(unsigned requested,unsigned enabled){
    if(requested>1||enabled>1||!context.m_pAntiLagAPI)return E_INVALIDARG;
    if(!requested)return clearAndDrain()?S_OK:E_FAIL;
    if(!mcd2_al2_fg_supported())return E_NOINTERFACE;
    const int result=publish(boundDevice,&context,enabled,0);
    if(result==0)fgBound=true;
    return result==0?S_OK:E_FAIL;
}
extern "C" __declspec(dllexport) HRESULT mcd2_al2_update(unsigned enabled) {
    if (enabled > 1) return E_INVALIDARG;
    // Leave the game's existing limiter in charge. Never add another FPS cap.
    return AMD::AntiLag2DX12::Update(&context, enabled != 0, 0);
}
extern "C" __declspec(dllexport) HRESULT mcd2_al2_end_rendering() {
    return AMD::AntiLag2DX12::MarkEndOfFrameRendering(&context);
}
extern "C" __declspec(dllexport) HRESULT mcd2_al2_real_frame() {
    return AMD::AntiLag2DX12::SetFrameGenFrameType(&context, false);
}
extern "C" __declspec(dllexport) void mcd2_al2_shutdown() {
    if(context.m_pAntiLagAPI)AMD::AntiLag2DX12::Update(&context,false,0);
    if(!clearAndDrain())return;
    AMD::AntiLag2DX12::DeInitialize(&context);
    boundDevice=nullptr;
}
extern "C" __declspec(dllexport) unsigned mcd2_al2_can_unload(){return context.m_pAntiLagAPI?0u:1u;}
