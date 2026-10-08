#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include "../../third-party/AntiLag2/ffx_antilag2_dx12.h"

// Compile against the official, unmodified SDK with Microsoft's COM ABI.
// Controller serializes calls, including release before device destruction.
namespace { AMD::AntiLag2DX12::Context context{}; }
extern "C" __declspec(dllexport) unsigned mcd2_al2_abi() { return 1; }
extern "C" __declspec(dllexport) HRESULT mcd2_al2_init(void* device) {
    if (context.m_pAntiLagAPI || !device) return E_INVALIDARG;
    const auto result = AMD::AntiLag2DX12::Initialize(&context, static_cast<ID3D12Device*>(device));
    if (result != S_OK) return result;
    return context.m_pAntiLagAPI ? S_OK : E_NOINTERFACE;
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
    AMD::AntiLag2DX12::DeInitialize(&context);
}
