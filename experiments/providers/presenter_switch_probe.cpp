// Exercises the exact AMD bridge and native DXGI in an owned scene.
// Explicit reset boundaries release cached buffers and frame-latency handles.
// Does not open game files, trigger an engine reset or qualify game performance.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdio>
#include <memory>
#include "fsr_fg_game_bridge.h"
#include "../../src/native/reset_epoch_contract.hpp"
#include "owned_presenter_switch_probe.hpp"
#ifndef MCD2_PROBE_WIDTH
#define MCD2_PROBE_WIDTH 1280
#define MCD2_PROBE_HEIGHT 720
#endif
constexpr unsigned W=MCD2_PROBE_WIDTH,H=MCD2_PROBE_HEIGHT;
constexpr auto Read=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
struct ConcurrentObserver {
 OwnedPresenterSwitchProbe* chain;
 IUnknown* identity;
 std::atomic<bool> stop{false};
 std::atomic<unsigned> failures{0},samples{0};
 HANDLE thread=nullptr;
 ~ConcurrentObserver(){
  stop=true;
  if(thread){if(WaitForSingleObject(thread,5000)!=WAIT_OBJECT_0)ExitProcess(48);CloseHandle(thread);}
 }
};
DWORD WINAPI observe(void* data){
 auto& observer=*static_cast<ConcurrentObserver*>(data);
 while(!observer.stop){
  DXGI_SWAP_CHAIN_DESC1 description{};IUnknown* identity=nullptr;
  const auto queried=observer.chain->QueryInterface(IID_PPV_ARGS(&identity));
  if(FAILED(queried)||identity!=observer.identity||FAILED(observer.chain->GetDesc1(&description))||
     description.Width!=W||description.Height!=H||description.BufferCount!=3)++observer.failures;
  if(identity)identity->Release();++observer.samples;Sleep(1);
 }
 return 0;
}
void transition(ID3D12GraphicsCommandList* cmd,ID3D12Resource* image,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to){
 D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={image,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,from,to};cmd->ResourceBarrier(1,&b);
}
int main(int argc,char**argv){
 const bool sdr=argc==2&&!strcmp(argv[1],"sdr");
 FILE* log=nullptr;fopen_s(&log,"presenter-switch-probe.jsonl","w");if(!log)return 1;
 auto result=[&](const char* stage,long code){fprintf(log,"{\"stage\":\"%s\",\"result\":%ld}\n",stage,code);fflush(log);return code;};
 auto path=std::make_unique<wchar_t[]>(32768);auto length=GetCurrentDirectoryW(32768,path.get());if(!length||length>32600)return 2;
 auto bridge=LoadLibraryW(L"mcd2-fsr-fg-game-bridge.dll");if(!bridge)return 3;
#define FN(name) auto name=reinterpret_cast<decltype(&mcd2_afg_##name##_v1)>(GetProcAddress(bridge,"mcd2_afg_" #name "_v1"));if(!name)return 4;
 FN(load) FN(swap) FN(output) FN(state) FN(retire) FN(antilag_ready) FN(antilag)
#undef FN
#define FN(name) auto name=reinterpret_cast<decltype(&mcd2_afg_##name##_v2)>(GetProcAddress(bridge,"mcd2_afg_" #name "_v2"));if(!name)return 4;
 FN(recording_contract) FN(guides) FN(world) FN(present)
#undef FN
 if(result("owned-recording-contract",recording_contract()==2?0:-1))return 35;
 wcscat_s(path.get(),32768,L"\\amd_fidelityfx_framegeneration_dx12.dll");
 if(result("load-verified-runtime",load(path.get())))return 5;
 IDXGIFactory4* factory=nullptr;ID3D12Device* device=nullptr;ID3D12CommandQueue* queue=nullptr;
 ID3D12CommandAllocator* allocator=nullptr;ID3D12GraphicsCommandList* cmd=nullptr;ID3D12Fence* fence=nullptr;
 if(FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))||FAILED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&device))))return 6;
 D3D12_COMMAND_QUEUE_DESC q{};
 if(FAILED(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)))||FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)))||
 FAILED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator,nullptr,IID_PPV_ARGS(&cmd)))||FAILED(cmd->Close())||FAILED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence))))return 7;
 const auto event=CreateEventW(nullptr,FALSE,FALSE,nullptr);uint64_t fenceValue=0;
 auto wait=[&](){auto value=++fenceValue;return event&&SUCCEEDED(queue->Signal(fence,value))&&SUCCEEDED(fence->SetEventOnCompletion(value,event))&&WaitForSingleObject(event,10000)==WAIT_OBJECT_0;};
 // This owned host increments metadata only after the actual Reset succeeds.
 // The game integration separately requires its recording-epoch contract.
 uint64_t epoch=0;
 auto reset=[&](){if(FAILED(allocator->Reset())||FAILED(cmd->Reset(allocator,nullptr)))return false;++epoch;return SUCCEEDED(cmd->SetPrivateData(mcd2_reset_epoch_guid,sizeof(epoch),&epoch));};
 WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"MCD2OwnedPresenterSwitchProbe";RegisterClassW(&wc);
 HWND window=CreateWindowW(wc.lpszClassName,L"Owned presenter handoff test",WS_OVERLAPPEDWINDOW,0,0,W,H,nullptr,nullptr,wc.hInstance,nullptr);if(!window)return 8;ShowWindow(window,SW_SHOW);
 MCD2AmdFgSwapV1 desc{sizeof(desc),W,H,unsigned(DXGI_FORMAT_R10G10B10A2_UNORM),1,0,DXGI_USAGE_RENDER_TARGET_OUTPUT,3,unsigned(DXGI_SCALING_STRETCH),unsigned(DXGI_SWAP_EFFECT_FLIP_DISCARD),unsigned(DXGI_ALPHA_MODE_UNSPECIFIED),DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT,0,1,0,1,0,0};
 void* created=nullptr;if(result("create-bridge-swapchain",swap(factory,queue,window,&desc,&created))||!created)return 9;
 IDXGISwapChain4* first=nullptr;auto* initial=static_cast<IDXGISwapChain1*>(created);auto hr=initial->QueryInterface(IID_PPV_ARGS(&first));initial->Release();if(FAILED(hr))return 10;
 auto* empty=new OwnedPresenterSwitchProbe;
 if(empty->rebuild(true,0,true,true,[](IDXGISwapChain4*&){return S_OK;})!=E_UNEXPECTED||empty->generation||empty->Present(0,0)!=E_ABORT)return 45;
 empty->Release();result("reject-empty-target",0);
 auto* chain=new OwnedPresenterSwitchProbe;
 if(FAILED(chain->rebuild(true,0,true,true,[&](IDXGISwapChain4*& raw){raw=first;return S_OK;})))return 40;
 IUnknown* stableIdentity=nullptr;if(FAILED(chain->QueryInterface(IID_PPV_ARGS(&stableIdentity))))return 41;
 bool sdkOwner=true;unsigned cachedCount=0;ID3D12Resource* cached[3]{};
 auto acquire=[&](){for(unsigned n=0;n<3;++n){if(FAILED(chain->GetBuffer(n,IID_PPV_ARGS(&cached[n]))))return false;++cachedCount;}return true;};
 auto drop=[&](){for(auto*& image:cached){if(image){image->Release();image=nullptr;--cachedCount;}}};

 if(result("owned-presenter-antilag-ready",antilag_ready(device))||antilag_ready(nullptr)==0)return 31;
 if(antilag(device,nullptr,1,0)!=E_INVALIDARG||antilag(device,nullptr,0,2)!=E_INVALIDARG)return 32;
 if(result("clear-antilag-before-presentation",antilag(device,nullptr,0,1)))return 33;
 ID3D12CommandQueue* unrelated=nullptr;if(FAILED(device->CreateCommandQueue(&q,IID_PPV_ARGS(&unrelated))))return 36;
 const auto refused=present(unrelated,100,1);unrelated->Release();
 if(result("reject-unrelated-submission-queue",refused==-41?0:refused?refused:-1))return 37;
 HANDLE waitable=chain->GetFrameLatencyWaitableObject();if(!waitable)return 11;
 if(result("set-presentation-colour",chain->SetColorSpace1(sdr?DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)))return 38;
 MCD2AmdFgOutputV1 contract{sizeof(contract),W,H,24,24,sdr?1u:3u,sdr?2u:1u,0};
 if(result("configure-output-contract",output(&contract)))return 39;
 ID3D12DescriptorHeap* views=nullptr;D3D12_DESCRIPTOR_HEAP_DESC vh{};vh.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;vh.NumDescriptors=3;
 if(FAILED(device->CreateDescriptorHeap(&vh,IID_PPV_ARGS(&views))))return 12;
 ID3D12Resource* images[3]{};DXGI_FORMAT formats[]={DXGI_FORMAT_R10G10B10A2_UNORM,DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R16G16_FLOAT};
 auto handle=views->GetCPUDescriptorHandleForHeapStart();auto stride=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
 for(unsigned i=0;i<3;++i){D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=W;d.Height=H;d.MipLevels=d.DepthOrArraySize=1;d.Format=formats[i];d.SampleDesc.Count=1;d.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
  if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&images[i]))))return 13;device->CreateRenderTargetView(images[i],nullptr,handle);handle.ptr+=stride;
 }

 unsigned identity=0;unsigned switchCount=0;
 auto rebuild=[&](bool nextSdk){
  const auto switchStart=GetTickCount64();
  // Fresh submission fence and successful owned command reset invalidate replay.
  if(!wait()||!reset()||FAILED(cmd->Close()))return false;
  // Reject switching before the host releases its cached backbuffers.
  unsigned refusedCallbacks=0;
  for(unsigned reason=0;reason<4;++reason){
   auto refused=chain->rebuild(reason!=0,reason==1?cachedCount:0,reason!=2,reason!=3,[&](IDXGISwapChain4*&){++refusedCallbacks;return S_OK;});
   if(refused!=DXGI_ERROR_INVALID_CALL||refusedCallbacks)return false;
  }
  fprintf(log,"{\"stage\":\"reject-unsafe-switch\",\"cases\":4}\n");fflush(log);
  drop();if(waitable){if(!CloseHandle(waitable))return false;waitable=nullptr;}
  auto code=chain->rebuild(true,cachedCount,true,true,[&](IDXGISwapChain4*& raw)->HRESULT{
   if(sdkOwner){
    if(present(queue,identity+100,0)||retire(nullptr,0))return E_FAIL;
   }
   if(raw){raw->Release();raw=nullptr;}
   if(nextSdk){
    if(load(path.get()))return E_FAIL;
    void* output=nullptr;if(swap(factory,queue,window,&desc,&output)||!output)return E_FAIL;
    auto* inner=static_cast<IDXGISwapChain1*>(output);
    auto code=inner->QueryInterface(IID_PPV_ARGS(&raw));inner->Release();if(FAILED(code))return code;
   }else{
    DXGI_SWAP_CHAIN_DESC1 d{};d.Width=W;d.Height=H;d.Format=DXGI_FORMAT_R10G10B10A2_UNORM;
    d.SampleDesc.Count=1;d.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;d.BufferCount=3;
    d.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;d.Flags=DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    IDXGISwapChain1* inner=nullptr;
    auto code=factory->CreateSwapChainForHwnd(queue,window,&d,nullptr,nullptr,&inner);if(FAILED(code))return code;
    code=inner->QueryInterface(IID_PPV_ARGS(&raw));inner->Release();if(FAILED(code))return code;
   }
   return raw->SetColorSpace1(sdr?DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
  });
  if(FAILED(code))return false;sdkOwner=nextSdk;
  if(sdkOwner&&output(&contract))return false;
  waitable=chain->GetFrameLatencyWaitableObject();if(!waitable||!acquire())return false;
  IUnknown* same=nullptr;if(FAILED(chain->QueryInterface(IID_PPV_ARGS(&same))))return false;
  bool stable=same==stableIdentity;same->Release();
  fprintf(log,"{\"stage\":\"switch\",\"generation\":%u,\"sdk\":%s,\"identityStable\":%s,\"milliseconds\":%llu}\n",chain->generation,sdkOwner?"true":"false",stable?"true":"false",GetTickCount64()-switchStart);fflush(log);
  ++switchCount;return stable;
 };
 if(!acquire())return 42;
 ConcurrentObserver observer{chain,stableIdentity};
 observer.thread=CreateThread(nullptr,0,observe,&observer,0,nullptr);if(!observer.thread)return 46;
 for(unsigned phase=0;phase<6;++phase){
  if(phase&&!rebuild(phase%2==0))return 43;
  const bool enabled=sdkOwner&&phase>0;const unsigned frames=enabled?60:30;
  MCD2AmdFgStateV1 before{};before.size=sizeof(before);if(sdkOwner&&state(&before))return 14;
  for(unsigned n=0;n<frames;++n,++identity){MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}
   if(WaitForSingleObject(waitable,10000)!=WAIT_OBJECT_0||!reset())return 15;
   handle=views->GetCPUDescriptorHandleForHeapStart();const float clear[3][4]={{.5f,.55f,.6f,1},{.5f,0,0,0},{0,0,0,0}};
   for(unsigned i=0;i<3;++i){if(identity)transition(cmd,images[i],i?D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:Read,D3D12_RESOURCE_STATE_RENDER_TARGET);cmd->ClearRenderTargetView(handle,clear[i],0,nullptr);transition(cmd,images[i],D3D12_RESOURCE_STATE_RENDER_TARGET,i?D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:Read);handle.ptr+=stride;}
   if(enabled){MCD2AmdFgGuidesV1 p{};p.size=sizeof(p);p.frameTimeMs=16.667f;p.worldToMeters=1;p.camera.size=sizeof(p.camera);p.camera.frame=identity+100;p.camera.width=W;p.camera.height=H;p.camera.reset=n==0;p.camera.nearPlane=.1f;p.camera.farPlane=1000;p.camera.verticalFOV=1.04719755f;p.camera.up[1]=1;p.camera.right[0]=1;p.camera.forward[2]=1;
    if(result("prepare-game-bridge-inputs",guides(cmd,images[1],images[2],&p)))return 16;
    if(result("copy-hudless-world",world(cmd,images[0],p.camera.frame)))return 17;
   }
   const auto index=chain->GetCurrentBackBufferIndex();if(index>=cachedCount)return 20;auto* back=cached[index];
   transition(cmd,images[0],Read,D3D12_RESOURCE_STATE_COPY_SOURCE);transition(cmd,back,D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_COPY_DEST);cmd->CopyResource(back,images[0]);transition(cmd,back,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PRESENT);transition(cmd,images[0],D3D12_RESOURCE_STATE_COPY_SOURCE,Read);
   if(FAILED(cmd->Close()))return 21;ID3D12CommandList* commands[]={cmd};queue->ExecuteCommandLists(1,commands);
   if(sdkOwner&&result("configure-bridge-present",present(queue,identity+100,enabled)))return 19;
   if(FAILED(chain->Present(0,0))||!wait()||FAILED(device->GetDeviceRemovedReason()))return 22;Sleep(16);
  }
  MCD2AmdFgStateV1 after{};after.size=sizeof(after);auto deadline=GetTickCount64()+5000;
  if(sdkOwner){do{if(state(&after))return 23;if(after.realPresents-before.realPresents>=frames)break;Sleep(10);}while(GetTickCount64()<deadline);}
  auto real=sdkOwner?after.realPresents-before.realPresents:frames,generated=after.generatedPresents-before.generatedPresents;
  fprintf(log,"{\"stage\":\"phase\",\"phase\":%u,\"enabled\":%s,\"sdkOwner\":%s,\"real\":%llu,\"generated\":%llu,\"fault\":%u,\"errors\":%u,\"warnings\":%u}\n",phase,enabled?"true":"false",sdkOwner?"true":"false",real,generated,after.fault,after.errors,after.warnings);fflush(log);
  if(real!=frames||after.fault||after.errors||(!enabled&&generated)||(enabled&&generated!=frames))return 24;
 }
 observer.stop=true;
 if(WaitForSingleObject(observer.thread,5000)!=WAIT_OBJECT_0||observer.failures||!observer.samples)return 47;
 CloseHandle(observer.thread);observer.thread=nullptr;
 fprintf(log,"{\"stage\":\"concurrent-observer\",\"samples\":%u,\"failures\":%u}\n",observer.samples.load(),observer.failures.load());fflush(log);
 if(sdkOwner||switchCount!=5||!wait()||!reset()||FAILED(cmd->Close()))return 44;
 drop();CloseHandle(waitable);waitable=nullptr;stableIdentity->Release();
 chain->Release();chain=nullptr;
 for(auto* image:images)image->Release();views->Release();cmd->Release();allocator->Release();fence->Release();CloseHandle(event);queue->Release();device->Release();factory->Release();DestroyWindow(window);FreeLibrary(bridge);
 result("complete",0);fclose(log);return 0;
}
