#pragma once
#include <cstddef>
#include <dxgi.h>

// AMD's documented FSR 3.1.1+ Anti-Lag 2 swapchain contract. Use only inside
// Microsoft-ABI bridges; context remains owned by the Anti-Lag SDK bridge.
namespace mcd2::amd_fg {
inline constexpr GUID antiLagDataGuid={0x5083ae5b,0x8070,0x4fca,{0x8e,0xe5,0x35,0x82,0xdd,0x36,0x7d,0x13}};
struct AntiLagData { void* context; bool enabled; };
static_assert(sizeof(AntiLagData)==16 && offsetof(AntiLagData,enabled)==8);
inline HRESULT publish(IDXGIObject* swap,void* context,bool enabled){
    if(!swap || (enabled && !context))return E_INVALIDARG;
    AntiLagData data{};data.context=context;data.enabled=enabled;
    return swap->SetPrivateData(antiLagDataGuid,sizeof(data),&data);
}
}
