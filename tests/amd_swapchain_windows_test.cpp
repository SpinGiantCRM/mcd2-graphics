#include "../experiments/providers/synchronized_swapchain.hpp"
#include <thread>
#include <chrono>
#include <cassert>
#include <iostream>
std::atomic<bool> entered{false},allow{false},live{true},terminal{false};
std::atomic<unsigned> calls{0},destroyed{0};
bool isLive(IDXGISwapChain4*){return live.load();}
bool isTerminal(){return terminal.load();}
void until(std::atomic<bool>& flag){auto end=std::chrono::steady_clock::now()+std::chrono::seconds(2);while(!flag.load()){assert(std::chrono::steady_clock::now()<end);std::this_thread::yield();}}
class Fake final : public IDXGISwapChain4 {
 std::atomic<ULONG> references_{1};
public:
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID,void** out) override {*out=this;AddRef();return S_OK;}
 ULONG STDMETHODCALLTYPE AddRef() override {return references_.fetch_add(1)+1;}
 ULONG STDMETHODCALLTYPE Release() override {auto n=references_.fetch_sub(1)-1;if(!n){++destroyed;delete this;}return n;}
 HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID Name, UINT DataSize, const void *pData) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID Name, const IUnknown *pUnknown) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID Name, UINT *pDataSize, void *pData) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void **ppParent) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE GetDevice(REFIID riid, void **ppDevice) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE Present(UINT SyncInterval, UINT Flags) override {++calls;entered=true;until(allow);return S_FALSE;}
 HRESULT STDMETHODCALLTYPE GetBuffer(UINT Buffer, REFIID riid, void **ppSurface) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL Fullscreen, IDXGIOutput *pTarget) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE GetFullscreenState(BOOL *pFullscreen, IDXGIOutput **ppTarget) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE GetDesc(DXGI_SWAP_CHAIN_DESC *pDesc) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE ResizeTarget(const DXGI_MODE_DESC *pNewTargetParameters) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE GetContainingOutput(IDXGIOutput **ppOutput) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE GetFrameStatistics(DXGI_FRAME_STATISTICS *pStats) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE GetLastPresentCount(UINT *pLastPresentCount) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE GetDesc1(DXGI_SWAP_CHAIN_DESC1 *pDesc) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE GetFullscreenDesc(DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE GetHwnd(HWND *pHwnd) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE GetCoreWindow(REFIID refiid, void **ppUnk) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE Present1(UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS *pPresentParameters) override {++calls;entered=true;until(allow);return S_FALSE;}
 BOOL STDMETHODCALLTYPE IsTemporaryMonoSupported() override {return FALSE;}
 HRESULT STDMETHODCALLTYPE GetRestrictToOutput(IDXGIOutput **ppRestrictToOutput) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE SetBackgroundColor(const DXGI_RGBA *pColor) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE GetBackgroundColor(DXGI_RGBA *pColor) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE SetRotation(DXGI_MODE_ROTATION Rotation) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE GetRotation(DXGI_MODE_ROTATION *pRotation) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE SetSourceSize(UINT Width, UINT Height) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE GetSourceSize(UINT *pWidth, UINT *pHeight) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency(UINT MaxLatency) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency(UINT *pMaxLatency) override {return S_OK;}
 HANDLE STDMETHODCALLTYPE GetFrameLatencyWaitableObject() override {return reinterpret_cast<HANDLE>(123);}
 HRESULT STDMETHODCALLTYPE SetMatrixTransform(const DXGI_MATRIX_3X2_F *pMatrix) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE GetMatrixTransform(DXGI_MATRIX_3X2_F *pMatrix) override {return S_OK;}
 UINT STDMETHODCALLTYPE GetCurrentBackBufferIndex() override {return 7;}
 HRESULT STDMETHODCALLTYPE CheckColorSpaceSupport(DXGI_COLOR_SPACE_TYPE ColorSpace, UINT *pColorSpaceSupport) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE SetColorSpace1(DXGI_COLOR_SPACE_TYPE ColorSpace) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE ResizeBuffers1(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT Format, UINT SwapChainFlags, const UINT *pCreationNodeMask, IUnknown *const *ppPresentQueue) override {return S_OK;}
 HRESULT STDMETHODCALLTYPE SetHDRMetaData(DXGI_HDR_METADATA_TYPE Type, UINT Size, void *pMetaData) override {assert(Type==DXGI_HDR_METADATA_TYPE_HDR10&&Size==9&&pMetaData==reinterpret_cast<void*>(42));return HRESULT(321);}
};
int main(){
 std::mutex gate;auto raw=new Fake;auto outer=new MCD2SynchronizedSwapchain(raw,gate,isLive,isTerminal);
 for(const auto& iid:{IID_IUnknown,IID_IDXGIObject,IID_IDXGIDeviceSubObject,IID_IDXGISwapChain,IID_IDXGISwapChain1,IID_IDXGISwapChain2,IID_IDXGISwapChain3,IID_IDXGISwapChain4}){void* p=nullptr;assert(outer->QueryInterface(iid,&p)==S_OK&&p==outer);static_cast<IUnknown*>(p)->Release();}
 const GUID unknown={1,2,3,{4,5,6,7,8,9,10,11}};void* unknownOut=reinterpret_cast<void*>(1);assert(outer->QueryInterface(unknown,&unknownOut)==E_NOINTERFACE&&!unknownOut);assert(outer->QueryInterface(IID_IUnknown,nullptr)==E_POINTER);
 assert(outer->GetCurrentBackBufferIndex()==7);assert(outer->GetFrameLatencyWaitableObject()==reinterpret_cast<HANDLE>(123));assert(outer->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10,9,reinterpret_cast<void*>(42))==HRESULT(321));
 for(bool present1:{false,true}){
  entered=false;allow=false;std::atomic<bool> attempted{false},prepared{false};
  std::thread a([&]{auto r=present1?outer->Present1(0,0,nullptr):outer->Present(0,0);assert(r==S_FALSE);});until(entered);
  assert(!gate.try_lock());
  std::thread b([&]{attempted=true;std::lock_guard lock(gate);prepared=true;});until(attempted);assert(!prepared.load());allow=true;a.join();b.join();assert(prepared);
 }
 live=false;assert(outer->Present(0,0)==E_ABORT&&outer->Present1(0,0,nullptr)==E_ABORT);assert(calls==2);assert(outer->GetCurrentBackBufferIndex()==0&&!outer->GetFrameLatencyWaitableObject());
 {std::lock_guard lock(gate);terminal=true;assert(outer->Release()==1);assert(outer->Present(0,0)==E_ABORT);terminal=false;}
 assert(outer->Release()==0&&destroyed==0);assert(raw->Release()==0&&destroyed==1);
 std::cout<<"COM identity, forwarding, both Present gates, concurrent SDK exclusion, closed/terminal refusal and balanced destruction passed\n";
}
