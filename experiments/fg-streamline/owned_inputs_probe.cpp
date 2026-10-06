// Isolated synthetic FG gate. No game hooks, game data, saves or installations.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdio>
#include <cmath>
#include <memory>
extern "C" {
long mcd2_sl_verify(const wchar_t*);int mcd2_sl_init(const wchar_t*,const wchar_t*,const wchar_t*);int mcd2_sl_set_device(void*);int mcd2_sl_mode(unsigned);int mcd2_sl_mode_limit(unsigned,unsigned);int mcd2_sl_begin(unsigned,void**);unsigned mcd2_sl_index(void*);int mcd2_sl_sleep(void*);int mcd2_sl_marker(void*,unsigned);int mcd2_sl_state(unsigned*,unsigned*,unsigned*);int mcd2_sl_shutdown();void mcd2_fg_unload();int mcd2_fg_support(void*,unsigned);int mcd2_fg_upgrade(void**);int mcd2_fg_state(unsigned*,unsigned*,unsigned*,unsigned*);int mcd2_fg_mode(unsigned,unsigned,unsigned);long mcd2_fg_api_error();int mcd2_fg_inputs(void*,void*,void*,void*,void*,void*,unsigned,unsigned,unsigned);
int mcd2_fg_initialized();
}
#include "factory_route.hpp"
constexpr unsigned Width=1280,Height=720;
constexpr auto Read=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
template<class T>void release(T*& p){if(p){p->Release();p=nullptr;}}
struct Session {
 IDXGIFactory4 *factory=nullptr,*proxyFactory=nullptr;ID3D12Device *device=nullptr,*proxyDevice=nullptr;ID3D12CommandQueue* queue=nullptr;IDXGISwapChain3* swap=nullptr;
 ID3D12CommandAllocator* allocator=nullptr;ID3D12GraphicsCommandList* cmd=nullptr;ID3D12Fence* fence=nullptr;ID3D12DescriptorHeap* views=nullptr;
 ID3D12Resource* inputs[4]{};HWND window=nullptr;HANDLE done=nullptr;UINT64 fenceValue=0;
 FactoryRoute route;
 bool wait(){auto n=++fenceValue;return SUCCEEDED(queue->Signal(fence,n))&&SUCCEEDED(fence->SetEventOnCompletion(n,done))&&WaitForSingleObject(done,5000)==WAIT_OBJECT_0;}
 ~Session(){
  mcd2_fg_mode(0,Width,Height);if(queue&&fence&&done)wait();mcd2_sl_mode(0);mcd2_sl_shutdown();
  release(swap);for(auto& p:inputs)release(p);release(views);release(cmd);release(allocator);release(fence);release(queue);
  route.detach();if(proxyDevice!=device)release(proxyDevice);if(proxyFactory!=factory)release(proxyFactory);release(device);release(factory);
  if(done)CloseHandle(done);if(window)DestroyWindow(window);
  auto shim=GetModuleHandleW(L"dxgi.dll");auto detach=shim?reinterpret_cast<void(*)()>(GetProcAddress(shim,"mcd2_bootstrap_detach")):nullptr;if(detach)detach();mcd2_fg_unload();
 }
};
void barrier(ID3D12GraphicsCommandList* cmd,ID3D12Resource* p,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to){D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={p,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,from,to};cmd->ResourceBarrier(1,&b);}
struct Log {FILE* file=nullptr;~Log(){if(file)fclose(file);}void emit(const char* stage,long result){fprintf(file,"{\"stage\":\"%s\",\"result\":%ld}\n",stage,result);fflush(file);}};
int main(){
 Log log;fopen_s(&log.file,"owned-inputs.jsonl","w");if(!log.file)return 1;Session c;
 auto folder=std::make_unique<wchar_t[]>(32768),dll=std::make_unique<wchar_t[]>(32768);auto length=GetCurrentDirectoryW(32768,folder.get());if(!length||length>=32768)return 2;
 if(swprintf_s(dll.get(),32768,L"%s\\sl.interposer.dll",folder.get())<0)return 2;
 auto trust=mcd2_sl_verify(dll.get());log.emit("authenticode",trust);if(trust)return 3;
 const bool bootstrap=GetEnvironmentVariableW(L"MCD2_FG_EARLY_BOOTSTRAP",nullptr,0)!=0;
 auto r=bootstrap?0:mcd2_sl_init(dll.get(),folder.get(),folder.get());if(!bootstrap){log.emit("slInit",r);if(r)return 4;}
 auto factoryHr=CreateDXGIFactory1(IID_PPV_ARGS(&c.factory));log.emit("CreateDXGIFactory1-before-device",factoryHr);if(FAILED(factoryHr))return 10;
 if(bootstrap){auto ready=mcd2_fg_initialized();log.emit("automatic-SDK-init-before-device",ready?0:-1);if(!ready)return 43;}
 auto hr=D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&c.device));log.emit("D3D12CreateDevice",hr);if(FAILED(hr))return 5;
 auto luid=c.device->GetAdapterLuid();r=mcd2_fg_support(&luid,sizeof(luid));log.emit("DLSSG-support-rendering-adapter",r);if(r)return 6;
 r=mcd2_sl_set_device(c.device);log.emit("slSetD3DDevice",r);if(r)return 7;
 unsigned available=0,valid=0,complete=0;r=mcd2_sl_state(&available,&valid,&complete);log.emit("Reflex-capability",r);if(r||!available)return 8;
 c.proxyDevice=c.device;
 if(GetEnvironmentVariableW(L"MCD2_FG_NATIVE_QUEUE",nullptr,0))log.emit("native-device-queue-control",0);
 else {r=mcd2_fg_upgrade(reinterpret_cast<void**>(&c.proxyDevice));log.emit("upgrade-device",r);if(r)return 9;}
 c.proxyFactory=c.factory;
 if(bootstrap)log.emit("factory-via-early-bootstrap",0);
 else if(GetEnvironmentVariableW(L"MCD2_FG_FACTORY_ROUTE",nullptr,0)){
  bool attached=c.route.attach(c.factory);log.emit("factory-inner-route",attached?0:-1);if(!attached)return 42;
 }else if(GetEnvironmentVariableW(L"MCD2_FG_RESHADEROUTE",nullptr,0)){
  // ReShade's documented proxy-library route wraps the SL factory. Retain
  // that outer wrapper; upgrading it again would reverse the chain.
  log.emit("factory-via-reshade-proxy-library",0);
 }else{r=mcd2_fg_upgrade(reinterpret_cast<void**>(&c.proxyFactory));log.emit("upgrade-factory",r);if(r)return 11;}
 D3D12_COMMAND_QUEUE_DESC q{};if(FAILED(c.proxyDevice->CreateCommandQueue(&q,IID_PPV_ARGS(&c.queue))))return 12;
 if(FAILED(c.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&c.allocator)))||FAILED(c.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,c.allocator,nullptr,IID_PPV_ARGS(&c.cmd))))return 13;
 if(FAILED(c.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&c.fence))))return 14;c.done=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!c.done)return 15;
 D3D12_DESCRIPTOR_HEAP_DESC vh{};vh.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;vh.NumDescriptors=4;if(FAILED(c.device->CreateDescriptorHeap(&vh,IID_PPV_ARGS(&c.views))))return 16;
 const bool hdr=GetEnvironmentVariableW(L"MCD2_FG_HDR",nullptr,0)!=0;
 const DXGI_FORMAT formats[]={hdr?DXGI_FORMAT_R10G10B10A2_UNORM:DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R16G16_FLOAT,hdr?DXGI_FORMAT_R16G16B16A16_FLOAT:DXGI_FORMAT_R8G8B8A8_UNORM};
 auto pq=[](float nits){auto x=std::pow(nits/10000.f,2610.f/16384.f);return std::pow((3424.f/4096.f+(2413.f/128.f)*x)/(1+(2392.f/128.f)*x),2523.f/32.f);};
 const float colours[4][4]={{hdr?pq(10):.12f,hdr?pq(20):.25f,hdr?pq(35):.55f,1},{.5f,0,0,0},{0,0,0,0},{0,0,0,0}};
 D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;auto handle=c.views->GetCPUDescriptorHandleForHeapStart();auto stride=c.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
 for(unsigned i=0;i<4;++i){D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=Width;d.Height=Height;d.DepthOrArraySize=1;d.MipLevels=1;d.Format=formats[i];d.SampleDesc.Count=1;d.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
  hr=c.device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&c.inputs[i]));log.emit("owned-resource",hr);if(FAILED(hr))return 17;
  c.device->CreateRenderTargetView(c.inputs[i],nullptr,handle);c.cmd->ClearRenderTargetView(handle,colours[i],0,nullptr);
  if(i==0){D3D12_RECT rect{Width/4,Height/4,3*Width/4,3*Height/4};const float block[]={hdr?pq(120):.8f,hdr?pq(50):.3f,hdr?pq(10):.1f,1};c.cmd->ClearRenderTargetView(handle,block,1,&rect);}
  barrier(c.cmd,c.inputs[i],D3D12_RESOURCE_STATE_RENDER_TARGET,Read);handle.ptr+=stride;
 }
 if(FAILED(c.cmd->Close()))return 18;ID3D12CommandList* initialize[]={c.cmd};c.queue->ExecuteCommandLists(1,initialize);if(!c.wait())return 19;
 WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"MCD2OwnedInputsFGGate";RegisterClassW(&wc);c.window=CreateWindowW(wc.lpszClassName,L"Isolated FG owned-input test",WS_OVERLAPPEDWINDOW,32,32,640,360,nullptr,nullptr,wc.hInstance,nullptr);if(!c.window)return 20;
 const bool background=GetEnvironmentVariableW(L"MCD2_FG_BACKGROUND_CONTROL",nullptr,0)!=0;
 log.emit("winewayland-driver-loaded",GetModuleHandleW(L"winewayland.drv")?1:0);
 if(!background){ShowWindow(c.window,SW_SHOW);SetForegroundWindow(c.window);SetActiveWindow(c.window);}
 r=mcd2_sl_mode_limit(1,16667);log.emit("Reflex-On-60fps-limit",r);if(r)return 21;
 unsigned maximum=0,status=0,minimum=0,presented=0;r=mcd2_fg_state(&maximum,&status,&minimum,&presented);log.emit("DLSSG-state",r);if(r||maximum<1||Width<minimum||Height<minimum)return 22;
 unsigned identity=0,onInterpolatedSamples=0,onSteadySamples=0,onFocusedSamples=0;
 for(unsigned phase=0;phase<3;++phase){unsigned enabled=phase==1?1:0;r=mcd2_fg_mode(enabled,Width,Height);log.emit(enabled?"FG-enable":"FG-disable",r);if(r)return 23;
  release(c.swap);DXGI_SWAP_CHAIN_DESC1 sd{};sd.Width=Width;sd.Height=Height;sd.Format=formats[0];sd.SampleDesc.Count=1;sd.BufferCount=3;sd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;sd.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
  IDXGISwapChain1* swap=nullptr;hr=c.proxyFactory->CreateSwapChainForHwnd(c.queue,c.window,&sd,nullptr,nullptr,&swap);log.emit("swapchain-recreate",hr);if(FAILED(hr))return 24;hr=swap->QueryInterface(IID_PPV_ARGS(&c.swap));swap->Release();if(FAILED(hr))return 25;
 if(hdr){IDXGISwapChain3* color=c.swap;UINT support=0;hr=color->CheckColorSpaceSupport(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020,&support);log.emit("HDR-PQ-support",hr);log.emit("HDR-PQ-support-flags",support);if(FAILED(hr)||!(support&DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))return 39;hr=color->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);log.emit("HDR-PQ-set",hr);if(FAILED(hr))return 40;}
 unsigned count=enabled?90:30,actual=0,steady=0,interpolated=0,focused=0;
  for(unsigned n=0;n<count;++n){void* token=nullptr;r=mcd2_sl_begin(++identity,&token);if(r||!token||mcd2_sl_index(token)!=identity){log.emit("frame-token",r);return 26;}
   r=mcd2_sl_sleep(token);if(r){log.emit("Reflex-sleep",r);return 27;}if(mcd2_sl_marker(token,0))return 28;
   MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}if(mcd2_sl_marker(token,1)||mcd2_sl_marker(token,2))return 28;
   if(FAILED(c.allocator->Reset())||FAILED(c.cmd->Reset(c.allocator,nullptr)))return 29;
   ID3D12Resource* back=nullptr;hr=c.swap->GetBuffer(c.swap->GetCurrentBackBufferIndex(),IID_PPV_ARGS(&back));if(FAILED(hr))return 30;
   barrier(c.cmd,c.inputs[0],Read,D3D12_RESOURCE_STATE_COPY_SOURCE);barrier(c.cmd,back,D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_COPY_DEST);c.cmd->CopyResource(back,c.inputs[0]);barrier(c.cmd,back,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PRESENT);barrier(c.cmd,c.inputs[0],D3D12_RESOURCE_STATE_COPY_SOURCE,Read);back->Release();
   r=mcd2_fg_inputs(token,c.inputs[0],c.inputs[1],c.inputs[2],c.inputs[3],c.cmd,Width,Height,n==0);if(r){log.emit("frame-inputs",r);return 31;}
   if(FAILED(c.cmd->Close()))return 32;ID3D12CommandList* lists[]={c.cmd};c.queue->ExecuteCommandLists(1,lists);
   if(mcd2_sl_marker(token,3)||mcd2_sl_marker(token,4))return 28;hr=c.swap->Present(0,0);if(FAILED(hr)){log.emit("Present",hr);return 33;}if(mcd2_sl_marker(token,5))return 28;if(!c.wait())return 34;
   r=mcd2_fg_state(&maximum,&status,&minimum,&presented);actual+=presented;
   const bool hasFocus=GetForegroundWindow()==c.window;
   if(n>=10){++steady;if(presented>1)++interpolated;if(hasFocus)++focused;}
   if(n%10==0||r||status||mcd2_fg_api_error()){fprintf(log.file,"{\"stage\":\"frame\",\"phase\":%u,\"index\":%u,\"result\":%d,\"status\":%u,\"presented\":%u,\"focused\":%u,\"apiError\":%ld}\n",phase,identity,r,status,presented,unsigned(hasFocus),mcd2_fg_api_error());fflush(log.file);}
   if(r||mcd2_fg_api_error()||(n>10&&status))return 35;
  }
  mcd2_sl_state(&available,&valid,&complete);
  fprintf(log.file,"{\"stage\":\"phase\",\"phase\":%u,\"FG\":%u,\"realFrames\":%u,\"actualPresents\":%u,\"steadySamples\":%u,\"interpolatedSamples\":%u,\"focusedSamples\":%u,\"status\":%u,\"completeReflexReports\":%u}\n",phase,enabled,count,actual,steady,interpolated,focused,status,complete);fflush(log.file);
  if(enabled){onSteadySamples=steady;onInterpolatedSamples=interpolated;onFocusedSamples=focused;}
  else if(interpolated){log.emit("unexpected-generated-presents-while-off",interpolated);return 41;}
 }
 // A repeated trailing state query can report a stale single presentation. It
 // must never turn N real frames into a false N+1 generated-frame pass.
 if(background){log.emit("background-control-interpolated-samples",onInterpolatedSamples);return onInterpolatedSamples?38:0;}
 if(onFocusedSamples<onSteadySamples/2){log.emit("foreground-not-established",onFocusedSamples);return 37;}
 if(onInterpolatedSamples<onSteadySamples/2){log.emit("insufficient-generated-presents",onInterpolatedSamples);return 36;}
 log.emit("gate-complete",0);return 0;
}
