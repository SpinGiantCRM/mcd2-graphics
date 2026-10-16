#pragma once
#include <dxgi1_6.h>
#include <atomic>
#include <mutex>
#include <new>
#include <vector>
#include <memory>
struct MCD2SwapchainPrivateData {GUID id{};bool interfaceValue=false;std::vector<unsigned char> bytes;std::shared_ptr<IUnknown> object;};
struct MCD2PresentationState {bool privateComplete=true;std::vector<MCD2SwapchainPrivateData> privateData;DXGI_COLOR_SPACE_TYPE colour=DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;bool colourSet=false,handleIssued=false,latencySet=false;UINT latency=0;DXGI_HDR_METADATA_TYPE hdrType=DXGI_HDR_METADATA_TYPE_NONE;std::vector<unsigned char> hdr;};
// One outer COM identity. Raw SDK interfaces never escape QueryInterface.
// The same gate protects SDK Prepare/Configure/Destroy and the entire Present.
class MCD2SynchronizedSwapchain final : public IDXGISwapChain4 {
 IDXGISwapChain4* raw_;
 std::mutex& gate_;
 bool (*live_)(IDXGISwapChain4*);
 bool (*terminal_)();
 std::atomic<ULONG> references_{1};
 MCD2PresentationState presentation_;
 HRESULT(*resize_)(IDXGISwapChain4*&,UINT,UINT,UINT,DXGI_FORMAT,UINT,const MCD2PresentationState&);
 void(*presented_)(IDXGISwapChain4*,HRESULT);
void capturePrivate(REFGUID id,UINT size,const void* data,const IUnknown* object,bool isInterface){
  try{
   auto& values=presentation_.privateData;size_t at=0;while(at<values.size()&&!IsEqualGUID(values[at].id,id))++at;
   if(!size&&!object){if(at<values.size())values.erase(values.begin()+at);return;}
   if(size>65536||(at==values.size()&&values.size()>=64)){presentation_.privateComplete=false;return;}
   MCD2SwapchainPrivateData item;item.id=id;item.interfaceValue=isInterface;
   if(isInterface){auto* hold=const_cast<IUnknown*>(object);hold->AddRef();item.object=std::shared_ptr<IUnknown>(hold,[](IUnknown* p){p->Release();});}
   else {if(!data){presentation_.privateComplete=false;return;}auto* bytes=static_cast<const unsigned char*>(data);item.bytes.assign(bytes,bytes+size);}
   if(at==values.size())values.push_back(std::move(item));else values[at]=std::move(item);
  }catch(...){presentation_.privateComplete=false;}
 }
public:
 MCD2SynchronizedSwapchain(IDXGISwapChain4* raw,std::mutex& gate,
     bool(*live)(IDXGISwapChain4*),bool(*terminal)(),
     HRESULT(*resize)(IDXGISwapChain4*&,UINT,UINT,UINT,DXGI_FORMAT,UINT,const MCD2PresentationState&)=nullptr,void(*presented)(IDXGISwapChain4*,HRESULT)=nullptr)
     :raw_(raw),gate_(gate),live_(live),terminal_(terminal),resize_(resize),presented_(presented){raw_->AddRef();}
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
  std::lock_guard lock(gate_);if(raw_)raw_->Release();delete this;return 0;
 }
 HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID Name, UINT DataSize, const void *pData) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;auto result=raw_->SetPrivateData(Name,DataSize,pData);if(resize_&&result==S_OK)capturePrivate(Name,DataSize,pData,nullptr,false);return result;}
 HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID Name, const IUnknown *pUnknown) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;auto result=raw_->SetPrivateDataInterface(Name,pUnknown);if(resize_&&result==S_OK)capturePrivate(Name,0,nullptr,pUnknown,true);return result;}
 HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID Name, UINT *pDataSize, void *pData) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetPrivateData(Name, pDataSize, pData);}
 HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void **ppParent) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetParent(riid, ppParent);}
 HRESULT STDMETHODCALLTYPE GetDevice(REFIID riid, void **ppDevice) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetDevice(riid, ppDevice);}
 HRESULT STDMETHODCALLTYPE Present(UINT SyncInterval, UINT Flags) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;auto result=raw_->Present(SyncInterval, Flags);if(presented_&&!(Flags&DXGI_PRESENT_TEST))presented_(raw_,result);return result;}
 HRESULT STDMETHODCALLTYPE GetBuffer(UINT Buffer, REFIID riid, void **ppSurface) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetBuffer(Buffer, riid, ppSurface);}
 HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL Fullscreen, IDXGIOutput *pTarget) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->SetFullscreenState(Fullscreen, pTarget);}
 HRESULT STDMETHODCALLTYPE GetFullscreenState(BOOL *pFullscreen, IDXGIOutput **ppTarget) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetFullscreenState(pFullscreen, ppTarget);}
 HRESULT STDMETHODCALLTYPE GetDesc(DXGI_SWAP_CHAIN_DESC *pDesc) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetDesc(pDesc);}
 HRESULT STDMETHODCALLTYPE ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return resize_?resize_(raw_,BufferCount,Width,Height,NewFormat,SwapChainFlags,presentation_):raw_->ResizeBuffers(BufferCount,Width,Height,NewFormat,SwapChainFlags);}
 HRESULT STDMETHODCALLTYPE ResizeTarget(const DXGI_MODE_DESC *pNewTargetParameters) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->ResizeTarget(pNewTargetParameters);}
 HRESULT STDMETHODCALLTYPE GetContainingOutput(IDXGIOutput **ppOutput) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetContainingOutput(ppOutput);}
 HRESULT STDMETHODCALLTYPE GetFrameStatistics(DXGI_FRAME_STATISTICS *pStats) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetFrameStatistics(pStats);}
 HRESULT STDMETHODCALLTYPE GetLastPresentCount(UINT *pLastPresentCount) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetLastPresentCount(pLastPresentCount);}
 HRESULT STDMETHODCALLTYPE GetDesc1(DXGI_SWAP_CHAIN_DESC1 *pDesc) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetDesc1(pDesc);}
 HRESULT STDMETHODCALLTYPE GetFullscreenDesc(DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetFullscreenDesc(pDesc);}
 HRESULT STDMETHODCALLTYPE GetHwnd(HWND *pHwnd) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetHwnd(pHwnd);}
 HRESULT STDMETHODCALLTYPE GetCoreWindow(REFIID refiid, void **ppUnk) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetCoreWindow(refiid, ppUnk);}
 HRESULT STDMETHODCALLTYPE Present1(UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS *pPresentParameters) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;auto result=raw_->Present1(SyncInterval, PresentFlags, pPresentParameters);if(presented_&&!(PresentFlags&DXGI_PRESENT_TEST))presented_(raw_,result);return result;}
 BOOL STDMETHODCALLTYPE IsTemporaryMonoSupported() override {if(terminal_())return 0;std::lock_guard lock(gate_);if(!live_(raw_))return 0;return raw_->IsTemporaryMonoSupported();}
 HRESULT STDMETHODCALLTYPE GetRestrictToOutput(IDXGIOutput **ppRestrictToOutput) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetRestrictToOutput(ppRestrictToOutput);}
 HRESULT STDMETHODCALLTYPE SetBackgroundColor(const DXGI_RGBA *pColor) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->SetBackgroundColor(pColor);}
 HRESULT STDMETHODCALLTYPE GetBackgroundColor(DXGI_RGBA *pColor) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetBackgroundColor(pColor);}
 HRESULT STDMETHODCALLTYPE SetRotation(DXGI_MODE_ROTATION Rotation) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->SetRotation(Rotation);}
 HRESULT STDMETHODCALLTYPE GetRotation(DXGI_MODE_ROTATION *pRotation) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetRotation(pRotation);}
 HRESULT STDMETHODCALLTYPE SetSourceSize(UINT Width, UINT Height) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->SetSourceSize(Width, Height);}
 HRESULT STDMETHODCALLTYPE GetSourceSize(UINT *pWidth, UINT *pHeight) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetSourceSize(pWidth, pHeight);}
 HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency(UINT MaxLatency) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;auto result=raw_->SetMaximumFrameLatency(MaxLatency);if(resize_&&result==S_OK){presentation_.latency=MaxLatency;presentation_.latencySet=true;}return result;}
 HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency(UINT *pMaxLatency) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetMaximumFrameLatency(pMaxLatency);}
 HANDLE STDMETHODCALLTYPE GetFrameLatencyWaitableObject() override {if(terminal_())return nullptr;std::lock_guard lock(gate_);if(!live_(raw_))return nullptr;auto result=raw_->GetFrameLatencyWaitableObject();if(result)presentation_.handleIssued=true;return result;}
 HRESULT STDMETHODCALLTYPE SetMatrixTransform(const DXGI_MATRIX_3X2_F *pMatrix) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->SetMatrixTransform(pMatrix);}
 HRESULT STDMETHODCALLTYPE GetMatrixTransform(DXGI_MATRIX_3X2_F *pMatrix) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->GetMatrixTransform(pMatrix);}
 UINT STDMETHODCALLTYPE GetCurrentBackBufferIndex() override {if(terminal_())return 0;std::lock_guard lock(gate_);if(!live_(raw_))return 0;return raw_->GetCurrentBackBufferIndex();}
 HRESULT STDMETHODCALLTYPE CheckColorSpaceSupport(DXGI_COLOR_SPACE_TYPE ColorSpace, UINT *pColorSpaceSupport) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->CheckColorSpaceSupport(ColorSpace, pColorSpaceSupport);}
 HRESULT STDMETHODCALLTYPE SetColorSpace1(DXGI_COLOR_SPACE_TYPE ColorSpace) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;auto result=raw_->SetColorSpace1(ColorSpace);if(resize_&&result==S_OK){presentation_.colour=ColorSpace;presentation_.colourSet=true;}return result;}
 HRESULT STDMETHODCALLTYPE ResizeBuffers1(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT Format, UINT SwapChainFlags, const UINT *pCreationNodeMask, IUnknown *const *ppPresentQueue) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;return raw_->ResizeBuffers1(BufferCount, Width, Height, Format, SwapChainFlags, pCreationNodeMask, ppPresentQueue);}
 HRESULT STDMETHODCALLTYPE SetHDRMetaData(DXGI_HDR_METADATA_TYPE Type, UINT Size, void *pMetaData) override {if(terminal_())return E_ABORT;std::lock_guard lock(gate_);if(!live_(raw_))return E_ABORT;auto result=raw_->SetHDRMetaData(Type,Size,pMetaData);if(resize_&&result==S_OK){try{presentation_.hdrType=Type;presentation_.hdr.clear();if(Size&&pMetaData&&Size<=1024){auto p=static_cast<const unsigned char*>(pMetaData);presentation_.hdr.assign(p,p+Size);}else if(Size)presentation_.privateComplete=false;}catch(...){presentation_.privateComplete=false;}}return result;}
};
