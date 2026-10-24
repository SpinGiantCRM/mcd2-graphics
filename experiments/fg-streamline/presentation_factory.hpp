#pragma once
#include <dxgi1_6.h>
#include <atomic>
namespace mcd2::fg {
// Keep Windows' native DXGI factory vtable intact; SL wraps the ReShade factory.
class PresentationFactory final : public IDXGIFactory7 {
public:
 using Create=HRESULT(*)(IDXGIFactory2*,IDXGIFactory2*,IUnknown*,HWND,const DXGI_SWAP_CHAIN_DESC1*,const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*,IDXGIOutput*,IDXGISwapChain1**);
private:
 IDXGIFactory7*base_;IDXGIFactory7*proxy_;Create create_;std::atomic<ULONG> refs_{1};
 ~PresentationFactory(){proxy_->Release();base_->Release();}
public:
 PresentationFactory(IDXGIFactory7*base,IDXGIFactory7*proxy,Create create):base_(base),proxy_(proxy),create_(create){}
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void**out) override {
  if(!out)return E_POINTER;
  if(id==__uuidof(IUnknown)||id==__uuidof(IDXGIObject)||id==__uuidof(IDXGIFactory)||id==__uuidof(IDXGIFactory1)||id==__uuidof(IDXGIFactory2)||id==__uuidof(IDXGIFactory3)||id==__uuidof(IDXGIFactory4)||id==__uuidof(IDXGIFactory5)||id==__uuidof(IDXGIFactory6)||id==__uuidof(IDXGIFactory7)){*out=static_cast<IDXGIFactory7*>(this);AddRef();return S_OK;}
  return base_->QueryInterface(id,out);
 }
 ULONG STDMETHODCALLTYPE AddRef() override{return ++refs_;}
 ULONG STDMETHODCALLTYPE Release() override{const auto left=--refs_;if(!left)delete this;return left;}
 HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID a,UINT b,const void*c) override{return base_->SetPrivateData(a,b,c);}
 HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID a,const IUnknown*b) override{return base_->SetPrivateDataInterface(a,b);}
 HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID a,UINT*b,void*c) override{return base_->GetPrivateData(a,b,c);}
 HRESULT STDMETHODCALLTYPE GetParent(REFIID a,void**b) override{return base_->GetParent(a,b);}
 HRESULT STDMETHODCALLTYPE EnumAdapters(UINT a,IDXGIAdapter**b) override{return base_->EnumAdapters(a,b);}
 HRESULT STDMETHODCALLTYPE MakeWindowAssociation(HWND a,UINT b) override{return base_->MakeWindowAssociation(a,b);}
 HRESULT STDMETHODCALLTYPE GetWindowAssociation(HWND*a) override{return base_->GetWindowAssociation(a);}
 HRESULT STDMETHODCALLTYPE CreateSwapChain(IUnknown*a,DXGI_SWAP_CHAIN_DESC*b,IDXGISwapChain**c) override{return base_->CreateSwapChain(a,b,c);}
 HRESULT STDMETHODCALLTYPE CreateSoftwareAdapter(HMODULE a,IDXGIAdapter**b) override{return base_->CreateSoftwareAdapter(a,b);}
 HRESULT STDMETHODCALLTYPE EnumAdapters1(UINT a,IDXGIAdapter1**b) override{return base_->EnumAdapters1(a,b);}
 BOOL STDMETHODCALLTYPE IsCurrent() override{return base_->IsCurrent();}
 BOOL STDMETHODCALLTYPE IsWindowedStereoEnabled() override{return base_->IsWindowedStereoEnabled();}
 HRESULT STDMETHODCALLTYPE CreateSwapChainForHwnd(IUnknown*a,HWND b,const DXGI_SWAP_CHAIN_DESC1*c,const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*d,IDXGIOutput*e,IDXGISwapChain1**f) override{return create_(base_,proxy_,a,b,c,d,e,f);}
 HRESULT STDMETHODCALLTYPE CreateSwapChainForCoreWindow(IUnknown*a,IUnknown*b,const DXGI_SWAP_CHAIN_DESC1*c,IDXGIOutput*d,IDXGISwapChain1**e) override{return base_->CreateSwapChainForCoreWindow(a,b,c,d,e);}
 HRESULT STDMETHODCALLTYPE GetSharedResourceAdapterLuid(HANDLE a,LUID*b) override{return base_->GetSharedResourceAdapterLuid(a,b);}
 HRESULT STDMETHODCALLTYPE RegisterStereoStatusWindow(HWND a,UINT b,DWORD*c) override{return base_->RegisterStereoStatusWindow(a,b,c);}
 HRESULT STDMETHODCALLTYPE RegisterStereoStatusEvent(HANDLE a,DWORD*b) override{return base_->RegisterStereoStatusEvent(a,b);}
 void STDMETHODCALLTYPE UnregisterStereoStatus(DWORD a) override{base_->UnregisterStereoStatus(a);}
 HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusWindow(HWND a,UINT b,DWORD*c) override{return base_->RegisterOcclusionStatusWindow(a,b,c);}
 HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusEvent(HANDLE a,DWORD*b) override{return base_->RegisterOcclusionStatusEvent(a,b);}
 void STDMETHODCALLTYPE UnregisterOcclusionStatus(DWORD a) override{base_->UnregisterOcclusionStatus(a);}
 HRESULT STDMETHODCALLTYPE CreateSwapChainForComposition(IUnknown*a,const DXGI_SWAP_CHAIN_DESC1*b,IDXGIOutput*c,IDXGISwapChain1**d) override{return base_->CreateSwapChainForComposition(a,b,c,d);}
 UINT STDMETHODCALLTYPE GetCreationFlags() override{return base_->GetCreationFlags();}
 HRESULT STDMETHODCALLTYPE EnumAdapterByLuid(LUID a,REFIID b,void**c) override{return base_->EnumAdapterByLuid(a,b,c);}
 HRESULT STDMETHODCALLTYPE EnumWarpAdapter(REFIID a,void**b) override{return base_->EnumWarpAdapter(a,b);}
 HRESULT STDMETHODCALLTYPE CheckFeatureSupport(DXGI_FEATURE a,void*b,UINT c) override{return base_->CheckFeatureSupport(a,b,c);}
 HRESULT STDMETHODCALLTYPE EnumAdapterByGpuPreference(UINT a,DXGI_GPU_PREFERENCE b,REFIID c,void**d) override{return base_->EnumAdapterByGpuPreference(a,b,c,d);}
 HRESULT STDMETHODCALLTYPE RegisterAdaptersChangedEvent(HANDLE a,DWORD*b) override{return base_->RegisterAdaptersChangedEvent(a,b);}
 HRESULT STDMETHODCALLTYPE UnregisterAdaptersChangedEvent(DWORD a) override{return base_->UnregisterAdaptersChangedEvent(a);}
};
}
