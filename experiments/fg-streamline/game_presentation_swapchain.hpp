#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <atomic>
#include <new>
#include <cstdint>
namespace mcd2::fg {
// Advance SR on the application's rendering queue before SL presents internal frames.
class GamePresentationSwapchain final : public IDXGISwapChain4 {
 IDXGISwapChain4* base_;IUnknown* queue_;std::atomic<ULONG> refs_{1};
 uint64_t nativeQueue_;
 void notify(const wchar_t*moduleName,const char*name){
 HMODULE module=nullptr;if(!GetModuleHandleExW(0,moduleName,&module))return;
  using Callback=int(*)(uint64_t,uint64_t);auto callback=reinterpret_cast<Callback>(GetProcAddress(module,name));
  if(callback)callback(reinterpret_cast<uint64_t>(this),nativeQueue_);FreeLibrary(module);
}
 void notify_fg(const char*name){
  HMODULE module=nullptr;if(!GetModuleHandleExW(0,L"mcd2-fg-guide-recon.addon64",&module))return;
  using Callback=int(*)(uint64_t,uint64_t,IDXGISwapChain4*);auto callback=reinterpret_cast<Callback>(GetProcAddress(module,name));
  if(callback)callback(reinterpret_cast<uint64_t>(this),nativeQueue_,base_);FreeLibrary(module);
 }
 ~GamePresentationSwapchain(){notify_fg("mcd2_fg_game_present_end_v1");notify(L"mcd2-display-latency.addon64","mcd2_fg_game_cpu_present_end_v1");notify(L"mcd2-graphics.addon64","mcd2_graphics_game_present_end_v1");base_->Release();queue_->Release();}
public:
 GamePresentationSwapchain(IDXGISwapChain4*base,IUnknown*queue,uint64_t nativeQueue):base_(base),queue_(queue),nativeQueue_(nativeQueue){}
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void**out) override {
  if(!out)return E_POINTER;
  if(id==__uuidof(IUnknown)||id==__uuidof(IDXGIObject)||id==__uuidof(IDXGIDeviceSubObject)||id==__uuidof(IDXGISwapChain)||id==__uuidof(IDXGISwapChain1)||id==__uuidof(IDXGISwapChain2)||id==__uuidof(IDXGISwapChain3)||id==__uuidof(IDXGISwapChain4)){*out=static_cast<IDXGISwapChain4*>(this);AddRef();return S_OK;}
  return base_->QueryInterface(id,out);
 }
 ULONG STDMETHODCALLTYPE AddRef() override{return ++refs_;}
 ULONG STDMETHODCALLTYPE Release() override{const auto left=--refs_;if(!left)delete this;return left;}
 HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID a,UINT b,const void*c) override{return base_->SetPrivateData(a,b,c);}
 HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID a,const IUnknown*b) override{return base_->SetPrivateDataInterface(a,b);}
 HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID a,UINT*b,void*c) override{return base_->GetPrivateData(a,b,c);}
 HRESULT STDMETHODCALLTYPE GetParent(REFIID a,void**b) override{return base_->GetParent(a,b);}
 HRESULT STDMETHODCALLTYPE GetDevice(REFIID a,void**b) override{return base_->GetDevice(a,b);}
 HRESULT STDMETHODCALLTYPE Present(UINT a,UINT b) override{if(b&DXGI_PRESENT_TEST)return base_->Present(a,b);notify_fg("mcd2_fg_game_present_v1");notify(L"mcd2-graphics.addon64","mcd2_graphics_game_present_v1");notify(L"mcd2-display-latency.addon64","mcd2_fg_game_cpu_present_v1");auto result=base_->Present(a,b);if(SUCCEEDED(result))notify(L"mcd2-display-latency.addon64","mcd2_fg_game_cpu_present_done_v1");notify_fg("mcd2_fg_game_present_done_v1");return result;}
 HRESULT STDMETHODCALLTYPE GetBuffer(UINT a,REFIID b,void**c) override{return base_->GetBuffer(a,b,c);}
 HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL a,IDXGIOutput*b) override{return base_->SetFullscreenState(a,b);}
 HRESULT STDMETHODCALLTYPE GetFullscreenState(BOOL*a,IDXGIOutput**b) override{return base_->GetFullscreenState(a,b);}
 HRESULT STDMETHODCALLTYPE GetDesc(DXGI_SWAP_CHAIN_DESC*a) override{return base_->GetDesc(a);}
 HRESULT STDMETHODCALLTYPE ResizeBuffers(UINT a,UINT b,UINT c,DXGI_FORMAT d,UINT e) override{return base_->ResizeBuffers(a,b,c,d,e);}
 HRESULT STDMETHODCALLTYPE ResizeTarget(const DXGI_MODE_DESC*a) override{return base_->ResizeTarget(a);}
 HRESULT STDMETHODCALLTYPE GetContainingOutput(IDXGIOutput**a) override{return base_->GetContainingOutput(a);}
 HRESULT STDMETHODCALLTYPE GetFrameStatistics(DXGI_FRAME_STATISTICS*a) override{return base_->GetFrameStatistics(a);}
 HRESULT STDMETHODCALLTYPE GetLastPresentCount(UINT*a) override{return base_->GetLastPresentCount(a);}
 HRESULT STDMETHODCALLTYPE GetDesc1(DXGI_SWAP_CHAIN_DESC1*a) override{return base_->GetDesc1(a);}
 HRESULT STDMETHODCALLTYPE GetFullscreenDesc(DXGI_SWAP_CHAIN_FULLSCREEN_DESC*a) override{return base_->GetFullscreenDesc(a);}
 HRESULT STDMETHODCALLTYPE GetHwnd(HWND*a) override{return base_->GetHwnd(a);}
 HRESULT STDMETHODCALLTYPE GetCoreWindow(REFIID a,void**b) override{return base_->GetCoreWindow(a,b);}
 HRESULT STDMETHODCALLTYPE Present1(UINT a,UINT b,const DXGI_PRESENT_PARAMETERS*c) override{if(b&DXGI_PRESENT_TEST)return base_->Present1(a,b,c);notify_fg("mcd2_fg_game_present_v1");notify(L"mcd2-graphics.addon64","mcd2_graphics_game_present_v1");notify(L"mcd2-display-latency.addon64","mcd2_fg_game_cpu_present_v1");auto result=base_->Present1(a,b,c);if(SUCCEEDED(result))notify(L"mcd2-display-latency.addon64","mcd2_fg_game_cpu_present_done_v1");notify_fg("mcd2_fg_game_present_done_v1");return result;}
 BOOL STDMETHODCALLTYPE IsTemporaryMonoSupported() override{return base_->IsTemporaryMonoSupported();}
 HRESULT STDMETHODCALLTYPE GetRestrictToOutput(IDXGIOutput**a) override{return base_->GetRestrictToOutput(a);}
 HRESULT STDMETHODCALLTYPE SetBackgroundColor(const DXGI_RGBA*a) override{return base_->SetBackgroundColor(a);}
 HRESULT STDMETHODCALLTYPE GetBackgroundColor(DXGI_RGBA*a) override{return base_->GetBackgroundColor(a);}
 HRESULT STDMETHODCALLTYPE SetRotation(DXGI_MODE_ROTATION a) override{return base_->SetRotation(a);}
 HRESULT STDMETHODCALLTYPE GetRotation(DXGI_MODE_ROTATION*a) override{return base_->GetRotation(a);}
 HRESULT STDMETHODCALLTYPE SetSourceSize(UINT a,UINT b) override{return base_->SetSourceSize(a,b);}
 HRESULT STDMETHODCALLTYPE GetSourceSize(UINT*a,UINT*b) override{return base_->GetSourceSize(a,b);}
 HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency(UINT a) override{return base_->SetMaximumFrameLatency(a);}
 HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency(UINT*a) override{return base_->GetMaximumFrameLatency(a);}
 HANDLE STDMETHODCALLTYPE GetFrameLatencyWaitableObject() override{return base_->GetFrameLatencyWaitableObject();}
 HRESULT STDMETHODCALLTYPE SetMatrixTransform(const DXGI_MATRIX_3X2_F*a) override{return base_->SetMatrixTransform(a);}
 HRESULT STDMETHODCALLTYPE GetMatrixTransform(DXGI_MATRIX_3X2_F*a) override{return base_->GetMatrixTransform(a);}
 UINT STDMETHODCALLTYPE GetCurrentBackBufferIndex() override{return base_->GetCurrentBackBufferIndex();}
 HRESULT STDMETHODCALLTYPE CheckColorSpaceSupport(DXGI_COLOR_SPACE_TYPE a,UINT*b) override{return base_->CheckColorSpaceSupport(a,b);}
 HRESULT STDMETHODCALLTYPE SetColorSpace1(DXGI_COLOR_SPACE_TYPE a) override{return base_->SetColorSpace1(a);}
 HRESULT STDMETHODCALLTYPE ResizeBuffers1(UINT a,UINT b,UINT c,DXGI_FORMAT d,UINT e,const UINT*f,IUnknown*const*g) override{return base_->ResizeBuffers1(a,b,c,d,e,f,g);}
 HRESULT STDMETHODCALLTYPE SetHDRMetaData(DXGI_HDR_METADATA_TYPE a,UINT b,void*c) override{return base_->SetHDRMetaData(a,b,c);}
};
inline bool wrap_game_presentation_swapchain(IDXGISwapChain1**swap,IUnknown*queue) {
 if(!swap||!*swap||!queue)return false;
 constexpr GUID unwrap={0x7f2c9a11,0x3b4e,0x4d6a,{0x81,0x2f,0x5e,0x9c,0xd3,0x7a,0x1b,0x42}};
 ID3D12CommandQueue*native=nullptr;if(FAILED(queue->QueryInterface(unwrap,reinterpret_cast<void**>(&native))))return false;
 const auto nativeId=reinterpret_cast<uint64_t>(native);native->Release();
 IDXGISwapChain4*base=nullptr;if(FAILED((*swap)->QueryInterface(IID_PPV_ARGS(&base))))return false;
 IUnknown*identity=nullptr;if(FAILED(queue->QueryInterface(IID_PPV_ARGS(&identity)))){base->Release();return false;}
 auto wrapped=new(std::nothrow) GamePresentationSwapchain(base,identity,nativeId);if(!wrapped){base->Release();identity->Release();return false;}
 (*swap)->Release();*swap=wrapped;return true;
}
}
