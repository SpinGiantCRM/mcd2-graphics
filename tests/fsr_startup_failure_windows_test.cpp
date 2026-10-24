// Exercise the actual startup entrypoint with a controlled SDK and real DXGI ownership.
#include "../experiments/providers/fsr_fg_game_bridge.cpp"
#include <cstdio>
#include <cstdlib>

namespace {
unsigned liveContexts=0;
bool queryFailure=false,createFailure=false,destroyFailure=false;
bool automaticSelection=false,providerAvailable=true;
uint64_t reportedVersion=1;
struct TestContext { IDXGISwapChain4* swap; };
// Only IUnknown and GetDevice are reachable on this rejected input queue.
struct RejectedQueue {
 void** vtable; ULONG references=1;
 static ULONG STDMETHODCALLTYPE addRef(RejectedQueue* self){return ++self->references;}
 static ULONG STDMETHODCALLTYPE releaseRef(RejectedQueue* self){return --self->references;}
 static HRESULT STDMETHODCALLTYPE getDevice(RejectedQueue*,REFIID,void** out){*out=nullptr;return E_FAIL;}
};
void require(bool ok,const char* message){if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
ffxReturnCode_t testCreate(ffxContext* context,ffxCreateContextDescHeader* header,const ffxAllocationCallbacks*) {
 require(providerAvailable,"Unavailable verified provider must be rejected before SDK creation");
 auto& p=*reinterpret_cast<const ffxCreateContextDescFrameGenerationSwapChainForHwndDX12*>(header);
 if(automaticSelection){
  reportedVersion=17752306902731063296ull;
  for(auto* next=header->pNext;next;next=next->pNext)if(next->type==FFX_API_DESC_TYPE_OVERRIDE_VERSION)reportedVersion=reinterpret_cast<ffxOverrideVersion*>(next)->versionId;
 }
 IDXGIFactory2* factory=nullptr;
 if(FAILED(p.dxgiFactory->QueryInterface(IID_PPV_ARGS(&factory))))return FFX_API_RETURN_ERROR;
 IDXGISwapChain1* raw=nullptr;
 auto hr=factory->CreateSwapChainForHwnd(p.gameQueue,p.hwnd,p.desc,p.fullscreenDesc,nullptr,&raw);factory->Release();
 if(FAILED(hr))return FFX_API_RETURN_ERROR;
 IDXGISwapChain4* swap=nullptr;hr=raw->QueryInterface(IID_PPV_ARGS(&swap));raw->Release();
 if(FAILED(hr))return FFX_API_RETURN_ERROR;
 *p.swapchain=swap;swap->AddRef();*context=new TestContext{swap};++liveContexts;
 return createFailure?FFX_API_RETURN_ERROR:FFX_API_RETURN_OK;
}
ffxReturnCode_t testDestroy(ffxContext* context,const ffxAllocationCallbacks*) {
 if(destroyFailure)return FFX_API_RETURN_ERROR;
 auto* p=static_cast<TestContext*>(*context);p->swap->Release();delete p;*context=nullptr;--liveContexts;
 return FFX_API_RETURN_OK;
}
ffxReturnCode_t testQuery(ffxContext*,ffxQueryDescHeader* header) {
 if(header->type==FFX_API_QUERY_DESC_TYPE_GET_VERSIONS){
  auto& p=*reinterpret_cast<ffxQueryDescGetVersions*>(header);
  require(p.createDescType==FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_FOR_HWND_DX12&&p.device,"Enumerate presenter versions for the actual device");
  const uint64_t count=providerAvailable?2:1;
  if(p.versionIds&&p.versionNames){require(*p.outputCount>=count,"Version query capacity");p.versionIds[0]=17752306902731063296ull;p.versionNames[0]="4.0.0 *";if(providerAvailable){p.versionIds[1]=17752306900579389447ull;p.versionNames[1]="3.1.7";}}
  *p.outputCount=count;return FFX_API_RETURN_OK;
 }
 auto& p=*reinterpret_cast<ffxQueryGetProviderVersion*>(header);p.versionId=reportedVersion;
 return queryFailure?FFX_API_RETURN_ERROR:FFX_API_RETURN_OK;
}
ffxReturnCode_t testDispatch(ffxContext*,const ffxDispatchDescHeader*) {return FFX_API_RETURN_OK;}
void prepare(){
 require(!session,"Prior failed startup must not retain a session");
 session=new Session;session->create=testCreate;session->destroy=testDestroy;session->query=testQuery;session->dispatch=testDispatch;
 queryFailure=createFailure=destroyFailure=automaticSelection=false;providerAvailable=true;reportedVersion=1;
}
}
int main(){
 IDXGIFactory4* factory=nullptr;require(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))),"Create factory");
 IDXGIAdapter* warp=nullptr;require(SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp))),"Find WARP");
 ID3D12Device* device=nullptr;require(SUCCEEDED(D3D12CreateDevice(warp,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))),"Create owned device");warp->Release();
 ID3D12CommandQueue* queue=nullptr;D3D12_COMMAND_QUEUE_DESC q{};
 require(SUCCEEDED(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue))),"Create queue");
 WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"MCD2OwnedStartupFailure";
 require(RegisterClassW(&wc)!=0,"Register owned window");
 auto window=CreateWindowW(wc.lpszClassName,L"Owned startup failure",WS_POPUP,0,0,64,64,nullptr,nullptr,wc.hInstance,nullptr);
 require(window!=nullptr,"Create owned window");
 MCD2AmdFgSwapV1 p{sizeof(p),64,64,DXGI_FORMAT_R8G8B8A8_UNORM,1,0,DXGI_USAGE_RENDER_TARGET_OUTPUT,2,DXGI_SCALING_STRETCH,DXGI_SWAP_EFFECT_FLIP_DISCARD,DXGI_ALPHA_MODE_IGNORE,0,0,1,0,1,0,0};
 void* rejectedMethods[8]{};rejectedMethods[1]=reinterpret_cast<void*>(&RejectedQueue::addRef);rejectedMethods[2]=reinterpret_cast<void*>(&RejectedQueue::releaseRef);rejectedMethods[7]=reinterpret_cast<void*>(&RejectedQueue::getDevice);
 RejectedQueue rejected{rejectedMethods};prepare();void* rejectedResult=nullptr;
 require(mcd2_afg_swap_v1(factory,&rejected,window,&p,&rejectedResult)==int(E_FAIL)&&!rejectedResult,"Failed GetDevice must remain a handled startup failure");
 require(rejected.references==1&&!liveContexts&&session&&!session->queue&&!session->device,"Failed GetDevice must not publish or retain queue ownership");
 delete session;session=nullptr;
 for(unsigned mode=0;mode<4;++mode){
  prepare();queryFailure=mode==1;createFailure=mode==2;
  p.format=mode==3?DXGI_FORMAT_UNKNOWN:DXGI_FORMAT_R8G8B8A8_UNORM;
  void* result=nullptr;auto status=mcd2_afg_swap_v1(factory,queue,window,&p,&result);
  require(status!=0&&!result,"Reject failed or unverified presenter");
  require(!liveContexts&&(!session||(!session->presenter&&!session->swap&&!session->queue&&!session->device)),"Failed startup must release presenter and HWND ownership before fallback");
  IDXGISwapChain1* fallback=nullptr;DXGI_SWAP_CHAIN_DESC1 desc{};desc.Width=64;desc.Height=64;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.BufferCount=2;desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
  require(SUCCEEDED(factory->CreateSwapChainForHwnd(queue,window,&desc,nullptr,nullptr,&fallback)),"Native fallback must reuse the exact HWND");fallback->Release();
 }
 p.format=DXGI_FORMAT_R8G8B8A8_UNORM;
 prepare();automaticSelection=true;void* selectedResult=nullptr;
 require(mcd2_afg_swap_v1(factory,queue,window,&p,&selectedResult)==0&&selectedResult&&reportedVersion==17752306900579389447ull,"Explicitly select the verified presenter when the runtime default changes");
 require(mcd2_afg_retire_v1(nullptr,0)==0,"Selected presenter retires normally");static_cast<IDXGISwapChain1*>(selectedResult)->Release();
 prepare();providerAvailable=false;selectedResult=nullptr;
 require(mcd2_afg_swap_v1(factory,queue,window,&p,&selectedResult)!=0&&!selectedResult&&!liveContexts,"Unavailable verified presenter must fail before SDK creation");
 require(!session||(!session->presenter&&!session->swap&&!session->queue),"Unavailable presenter releases pre-create ownership");
 prepare();reportedVersion=17752306900579389447ull;
 void* result=nullptr;require(mcd2_afg_swap_v1(factory,queue,window,&p,&result)==0&&result,"Verified presenter still succeeds");
 void* duplicate=nullptr;
 require(mcd2_afg_swap_v1(factory,queue,window,&p,&duplicate)==MCD2_AFG_SWAP_RETAINED&&!duplicate&&liveContexts==1,"Duplicate startup cannot retire the published presenter");
 require(mcd2_afg_retire_v1(nullptr,0)==0,"Successful startup retires normally");
 static_cast<IDXGISwapChain1*>(result)->Release();require(!liveContexts,"No successful-control context leak");
 prepare();destroyFailure=true;result=nullptr;
 require(mcd2_afg_swap_v1(factory,queue,window,&p,&result)==MCD2_AFG_SWAP_RETAINED&&!result,"Failed retirement explicitly forbids native fallback");
 require(liveContexts==1&&session&&session->presenter,"Unretired context stays owned");
 destroyFailure=false;require(mcd2_afg_retire_v1(nullptr,0)==0&&!liveContexts,"Retained startup can retire on a later safe retry");
 queue->Release();device->Release();factory->Release();DestroyWindow(window);UnregisterClassW(wc.lpszClassName,wc.hInstance);
 std::puts("PASS: rejected providers release real HWND ownership; successful startup preserved");
}
