#pragma once
#include <dxgi1_6.h>
#include <atomic>
#include <mutex>
#include <new>
// Owned-host experiment only. This is not a game presenter or an engine reset API.
// All ordinary DXGI interfaces retain one identity across destructive rebuilds.
class OwnedPresenterSwitchProbe final : public IDXGISwapChain4 {
 IDXGISwapChain4* raw_=nullptr;
 std::mutex gate_;
 std::atomic<ULONG> references_{1};
public:
 unsigned generation=0;
 OwnedPresenterSwitchProbe()=default;
 // Only the owned host calls this, after an actual fresh queue completion,
 // releasing every cached resource/view and closing the cached latency handle.
 // These arguments are host attestations, not a substitute for engine admission.
 // The callback owns old-target retirement and new-target creation. A destructive
 // failure may leave no target; rendering must not resume until recovery succeeds.
 template<class Rebuild> HRESULT rebuild(bool gpuComplete,unsigned cachedBuffers,
       bool latencyHandleClosed,bool recordingReset,Rebuild&& create){
  std::lock_guard lock(gate_);
  if(!gpuComplete||cachedBuffers||!latencyHandleClosed||!recordingReset)return DXGI_ERROR_INVALID_CALL;
  auto hr=create(raw_);if(SUCCEEDED(hr)){if(!raw_)return E_UNEXPECTED;++generation;}return hr;
 }
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out) override {
  if(!out)return E_POINTER;*out=nullptr;
  if(id!=IID_IUnknown&&id!=IID_IDXGIObject&&id!=IID_IDXGIDeviceSubObject&&
     id!=IID_IDXGISwapChain&&id!=IID_IDXGISwapChain1&&id!=IID_IDXGISwapChain2&&
     id!=IID_IDXGISwapChain3&&id!=IID_IDXGISwapChain4)return E_NOINTERFACE;
  *out=static_cast<IDXGISwapChain4*>(this);AddRef();return S_OK;
 }
 ULONG STDMETHODCALLTYPE AddRef() override {return references_.fetch_add(1)+1;}
 ULONG STDMETHODCALLTYPE Release() override {
  auto left=references_.fetch_sub(1)-1;if(left)return left;
  {std::lock_guard lock(gate_);if(raw_)raw_->Release();raw_=nullptr;}
  delete this;return 0;
 }
 HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID Name, UINT DataSize, const void *pData) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->SetPrivateData(Name, DataSize, pData);}
 HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID Name, const IUnknown *pUnknown) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->SetPrivateDataInterface(Name, pUnknown);}
 HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID Name, UINT *pDataSize, void *pData) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->GetPrivateData(Name, pDataSize, pData);}
 HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void **ppParent) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->GetParent(riid, ppParent);}
 HRESULT STDMETHODCALLTYPE GetDevice(REFIID riid, void **ppDevice) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->GetDevice(riid, ppDevice);}
 HRESULT STDMETHODCALLTYPE Present(UINT SyncInterval, UINT Flags) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->Present(SyncInterval, Flags);}
 HRESULT STDMETHODCALLTYPE GetBuffer(UINT Buffer, REFIID riid, void **ppSurface) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->GetBuffer(Buffer, riid, ppSurface);}
 HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL Fullscreen, IDXGIOutput *pTarget) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->SetFullscreenState(Fullscreen, pTarget);}
 HRESULT STDMETHODCALLTYPE GetFullscreenState(BOOL *pFullscreen, IDXGIOutput **ppTarget) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->GetFullscreenState(pFullscreen, ppTarget);}
 HRESULT STDMETHODCALLTYPE GetDesc(DXGI_SWAP_CHAIN_DESC *pDesc) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->GetDesc(pDesc);}
 HRESULT STDMETHODCALLTYPE ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->ResizeBuffers(BufferCount, Width, Height, NewFormat, SwapChainFlags);}
 HRESULT STDMETHODCALLTYPE ResizeTarget(const DXGI_MODE_DESC *pNewTargetParameters) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->ResizeTarget(pNewTargetParameters);}
 HRESULT STDMETHODCALLTYPE GetContainingOutput(IDXGIOutput **ppOutput) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->GetContainingOutput(ppOutput);}
 HRESULT STDMETHODCALLTYPE GetFrameStatistics(DXGI_FRAME_STATISTICS *pStats) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->GetFrameStatistics(pStats);}
 HRESULT STDMETHODCALLTYPE GetLastPresentCount(UINT *pLastPresentCount) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->GetLastPresentCount(pLastPresentCount);}
 HRESULT STDMETHODCALLTYPE GetDesc1(DXGI_SWAP_CHAIN_DESC1 *pDesc) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->GetDesc1(pDesc);}
 HRESULT STDMETHODCALLTYPE GetFullscreenDesc(DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->GetFullscreenDesc(pDesc);}
 HRESULT STDMETHODCALLTYPE GetHwnd(HWND *pHwnd) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->GetHwnd(pHwnd);}
 HRESULT STDMETHODCALLTYPE GetCoreWindow(REFIID refiid, void **ppUnk) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->GetCoreWindow(refiid, ppUnk);}
 HRESULT STDMETHODCALLTYPE Present1(UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS *pPresentParameters) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->Present1(SyncInterval, PresentFlags, pPresentParameters);}
 BOOL STDMETHODCALLTYPE IsTemporaryMonoSupported() override {std::lock_guard lock(gate_);if(!raw_)return 0;return raw_->IsTemporaryMonoSupported();}
 HRESULT STDMETHODCALLTYPE GetRestrictToOutput(IDXGIOutput **ppRestrictToOutput) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->GetRestrictToOutput(ppRestrictToOutput);}
 HRESULT STDMETHODCALLTYPE SetBackgroundColor(const DXGI_RGBA *pColor) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->SetBackgroundColor(pColor);}
 HRESULT STDMETHODCALLTYPE GetBackgroundColor(DXGI_RGBA *pColor) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->GetBackgroundColor(pColor);}
 HRESULT STDMETHODCALLTYPE SetRotation(DXGI_MODE_ROTATION Rotation) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->SetRotation(Rotation);}
 HRESULT STDMETHODCALLTYPE GetRotation(DXGI_MODE_ROTATION *pRotation) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->GetRotation(pRotation);}
 HRESULT STDMETHODCALLTYPE SetSourceSize(UINT Width, UINT Height) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->SetSourceSize(Width, Height);}
 HRESULT STDMETHODCALLTYPE GetSourceSize(UINT *pWidth, UINT *pHeight) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->GetSourceSize(pWidth, pHeight);}
 HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency(UINT MaxLatency) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->SetMaximumFrameLatency(MaxLatency);}
 HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency(UINT *pMaxLatency) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->GetMaximumFrameLatency(pMaxLatency);}
 HANDLE STDMETHODCALLTYPE GetFrameLatencyWaitableObject() override {std::lock_guard lock(gate_);if(!raw_)return nullptr;return raw_->GetFrameLatencyWaitableObject();}
 HRESULT STDMETHODCALLTYPE SetMatrixTransform(const DXGI_MATRIX_3X2_F *pMatrix) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->SetMatrixTransform(pMatrix);}
 HRESULT STDMETHODCALLTYPE GetMatrixTransform(DXGI_MATRIX_3X2_F *pMatrix) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->GetMatrixTransform(pMatrix);}
 UINT STDMETHODCALLTYPE GetCurrentBackBufferIndex() override {std::lock_guard lock(gate_);if(!raw_)return 0;return raw_->GetCurrentBackBufferIndex();}
 HRESULT STDMETHODCALLTYPE CheckColorSpaceSupport(DXGI_COLOR_SPACE_TYPE ColorSpace, UINT *pColorSpaceSupport) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->CheckColorSpaceSupport(ColorSpace, pColorSpaceSupport);}
 HRESULT STDMETHODCALLTYPE SetColorSpace1(DXGI_COLOR_SPACE_TYPE ColorSpace) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->SetColorSpace1(ColorSpace);}
 HRESULT STDMETHODCALLTYPE ResizeBuffers1(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT Format, UINT SwapChainFlags, const UINT *pCreationNodeMask, IUnknown *const *ppPresentQueue) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->ResizeBuffers1(BufferCount, Width, Height, Format, SwapChainFlags, pCreationNodeMask, ppPresentQueue);}
 HRESULT STDMETHODCALLTYPE SetHDRMetaData(DXGI_HDR_METADATA_TYPE Type, UINT Size, void *pMetaData) override {std::lock_guard lock(gate_);if(!raw_)return E_ABORT;return raw_->SetHDRMetaData(Type, Size, pMetaData);}
};
