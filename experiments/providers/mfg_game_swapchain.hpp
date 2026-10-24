#pragma once
#include <dxgi1_6.h>
#include <atomic>
#include <mutex>
// One final DXGI owner. All swapchain interfaces preserve this COM identity.
class MCD2MfgSwapchain final : public IDXGISwapChain4 {
 IDXGISwapChain4* raw_;
 std::recursive_mutex& gate_;
 bool (*live_)(IDXGISwapChain4*);
 bool (*terminal_)();
 std::atomic<ULONG> references_{1};
 HRESULT(*present_)(UINT,UINT,const DXGI_PRESENT_PARAMETERS*);
 HRESULT(*buffer_)(UINT,REFIID,void**);
 UINT(*index_)();
 HRESULT(*resize_)(UINT,UINT,UINT,DXGI_FORMAT,UINT,const UINT*,IUnknown*const*);
 bool(*retire_)();
 void(*changed_)();
 void(*colour_)(DXGI_COLOR_SPACE_TYPE);
 HANDLE(*latency_)();HRESULT(*maxLatency_)(UINT);HRESULT(*getLatency_)(UINT*);UINT(*flags_)();bool(*synchronize_)();
public:
 MCD2MfgSwapchain(IDXGISwapChain4* raw,std::recursive_mutex& gate,
     bool(*live)(IDXGISwapChain4*),bool(*terminal)(),
     HRESULT(*present)(UINT,UINT,const DXGI_PRESENT_PARAMETERS*),HRESULT(*buffer)(UINT,REFIID,void**),UINT(*index)(),
     HRESULT(*resize)(UINT,UINT,UINT,DXGI_FORMAT,UINT,const UINT*,IUnknown*const*),bool(*retire)(),void(*changed)(),void(*colour)(DXGI_COLOR_SPACE_TYPE),HANDLE(*latency)(),HRESULT(*maxLatency)(UINT),HRESULT(*getLatency)(UINT*),UINT(*flags)(),bool(*synchronize)())
     :raw_(raw),gate_(gate),live_(live),terminal_(terminal),present_(present),buffer_(buffer),index_(index),resize_(resize),retire_(retire),changed_(changed),colour_(colour),latency_(latency),maxLatency_(maxLatency),getLatency_(getLatency),flags_(flags),synchronize_(synchronize){raw_->AddRef();}
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out) override {
  if(!out)return E_POINTER;*out=nullptr;
  if(id!=IID_IUnknown&&id!=IID_IDXGIObject&&id!=IID_IDXGIDeviceSubObject&&
     id!=IID_IDXGISwapChain&&id!=IID_IDXGISwapChain1&&id!=IID_IDXGISwapChain2&&
     id!=IID_IDXGISwapChain3&&id!=IID_IDXGISwapChain4)return E_NOINTERFACE;
  *out=static_cast<IDXGISwapChain4*>(this);AddRef();return S_OK;
 }
 ULONG STDMETHODCALLTYPE AddRef() override {return references_.fetch_add(1)+1;}
 ULONG STDMETHODCALLTYPE Release() override {
  // Only terminal process exit defers vendor code destruction to the OS.
  if(terminal_())return references_.load();
  const auto left=references_.fetch_sub(1)-1;if(left)return left;
  std::lock_guard lock(gate_);if(!retire_()){references_=1;return 1;}if(raw_)raw_->Release();delete this;return 0;
 }
 HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID Name, UINT DataSize, const void *pData) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;auto result=raw_->SetPrivateData(Name,DataSize,pData);return result;}
 HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID Name, const IUnknown *pUnknown) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;auto result=raw_->SetPrivateDataInterface(Name,pUnknown);return result;}
 HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID Name, UINT *pDataSize, void *pData) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetPrivateData(Name, pDataSize, pData);}
 HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void **ppParent) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetParent(riid, ppParent);}
 HRESULT STDMETHODCALLTYPE GetDevice(REFIID riid, void **ppDevice) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetDevice(riid, ppDevice);}
 HRESULT STDMETHODCALLTYPE Present(UINT SyncInterval, UINT Flags) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return present_(SyncInterval,Flags,nullptr);}
 HRESULT STDMETHODCALLTYPE GetBuffer(UINT Buffer, REFIID riid, void **ppSurface) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return buffer_(Buffer,riid,ppSurface);}
 HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL Fullscreen, IDXGIOutput *pTarget) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;if(!synchronize_())return DXGI_ERROR_WAS_STILL_DRAWING;auto result=raw_->SetFullscreenState(Fullscreen,pTarget);if(result==S_OK)changed_();return result;}
 HRESULT STDMETHODCALLTYPE GetFullscreenState(BOOL *pFullscreen, IDXGIOutput **ppTarget) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetFullscreenState(pFullscreen, ppTarget);}
 HRESULT STDMETHODCALLTYPE GetDesc(DXGI_SWAP_CHAIN_DESC *pDesc) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;auto result=raw_->GetDesc(pDesc);if(SUCCEEDED(result))pDesc->Flags=flags_();return result;}
 HRESULT STDMETHODCALLTYPE ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return resize_(BufferCount,Width,Height,NewFormat,SwapChainFlags,nullptr,nullptr);}
 HRESULT STDMETHODCALLTYPE ResizeTarget(const DXGI_MODE_DESC *pNewTargetParameters) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;if(!synchronize_())return DXGI_ERROR_WAS_STILL_DRAWING;auto result=raw_->ResizeTarget(pNewTargetParameters);if(result==S_OK)changed_();return result;}
 HRESULT STDMETHODCALLTYPE GetContainingOutput(IDXGIOutput **ppOutput) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetContainingOutput(ppOutput);}
 HRESULT STDMETHODCALLTYPE GetFrameStatistics(DXGI_FRAME_STATISTICS *pStats) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetFrameStatistics(pStats);}
 HRESULT STDMETHODCALLTYPE GetLastPresentCount(UINT *pLastPresentCount) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetLastPresentCount(pLastPresentCount);}
 HRESULT STDMETHODCALLTYPE GetDesc1(DXGI_SWAP_CHAIN_DESC1 *pDesc) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;auto result=raw_->GetDesc1(pDesc);if(SUCCEEDED(result))pDesc->Flags=flags_();return result;}
 HRESULT STDMETHODCALLTYPE GetFullscreenDesc(DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetFullscreenDesc(pDesc);}
 HRESULT STDMETHODCALLTYPE GetHwnd(HWND *pHwnd) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetHwnd(pHwnd);}
 HRESULT STDMETHODCALLTYPE GetCoreWindow(REFIID refiid, void **ppUnk) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetCoreWindow(refiid, ppUnk);}
 HRESULT STDMETHODCALLTYPE Present1(UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS *pPresentParameters) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return present_(SyncInterval,PresentFlags,pPresentParameters);}
 BOOL STDMETHODCALLTYPE IsTemporaryMonoSupported() override {if(terminal_())return 0;std::lock_guard lock(gate_);if(!live_(raw_))return 0;return raw_->IsTemporaryMonoSupported();}
 HRESULT STDMETHODCALLTYPE GetRestrictToOutput(IDXGIOutput **ppRestrictToOutput) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetRestrictToOutput(ppRestrictToOutput);}
 HRESULT STDMETHODCALLTYPE SetBackgroundColor(const DXGI_RGBA *pColor) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->SetBackgroundColor(pColor);}
 HRESULT STDMETHODCALLTYPE GetBackgroundColor(DXGI_RGBA *pColor) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetBackgroundColor(pColor);}
 HRESULT STDMETHODCALLTYPE SetRotation(DXGI_MODE_ROTATION Rotation) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->SetRotation(Rotation);}
 HRESULT STDMETHODCALLTYPE GetRotation(DXGI_MODE_ROTATION *pRotation) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetRotation(pRotation);}
 HRESULT STDMETHODCALLTYPE SetSourceSize(UINT Width, UINT Height) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->SetSourceSize(Width, Height);}
 HRESULT STDMETHODCALLTYPE GetSourceSize(UINT *pWidth, UINT *pHeight) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetSourceSize(pWidth, pHeight);}
 HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency(UINT MaxLatency) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return maxLatency_(MaxLatency);}
 HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency(UINT *pMaxLatency) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return getLatency_(pMaxLatency);}
 HANDLE STDMETHODCALLTYPE GetFrameLatencyWaitableObject() override {if(terminal_())return nullptr;std::lock_guard lock(gate_);if(!live_(raw_))return nullptr;return latency_();}
 HRESULT STDMETHODCALLTYPE SetMatrixTransform(const DXGI_MATRIX_3X2_F *pMatrix) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->SetMatrixTransform(pMatrix);}
 HRESULT STDMETHODCALLTYPE GetMatrixTransform(DXGI_MATRIX_3X2_F *pMatrix) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetMatrixTransform(pMatrix);}
 UINT STDMETHODCALLTYPE GetCurrentBackBufferIndex() override {if(terminal_())return 0;std::lock_guard lock(gate_);if(!live_(raw_))return 0;return index_();}
 HRESULT STDMETHODCALLTYPE CheckColorSpaceSupport(DXGI_COLOR_SPACE_TYPE ColorSpace, UINT *pColorSpaceSupport) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->CheckColorSpaceSupport(ColorSpace, pColorSpaceSupport);}
 HRESULT STDMETHODCALLTYPE SetColorSpace1(DXGI_COLOR_SPACE_TYPE ColorSpace) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;if(!synchronize_())return DXGI_ERROR_WAS_STILL_DRAWING;auto result=raw_->SetColorSpace1(ColorSpace);if(result==S_OK)colour_(ColorSpace);return result;}
 HRESULT STDMETHODCALLTYPE ResizeBuffers1(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT Format, UINT SwapChainFlags, const UINT *pCreationNodeMask, IUnknown *const *ppPresentQueue) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return resize_(BufferCount,Width,Height,Format,SwapChainFlags,pCreationNodeMask,ppPresentQueue);}
 HRESULT STDMETHODCALLTYPE SetHDRMetaData(DXGI_HDR_METADATA_TYPE Type, UINT Size, void *pMetaData) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;if(!synchronize_())return DXGI_ERROR_WAS_STILL_DRAWING;auto result=raw_->SetHDRMetaData(Type,Size,pMetaData);if(result==S_OK)changed_();return result;}
};
