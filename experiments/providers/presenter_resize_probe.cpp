// Exercises the real ResizeBuffers presenter handoff with owned resources only.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdio>
#include <memory>
#include "../../src/providers/graphics_store.hpp"
#include "fsr_fg_game_bridge.h"
#include "../../src/native/reset_epoch_contract.hpp"
constexpr unsigned W=3840,H=2160;
constexpr auto Read=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
void transition(ID3D12GraphicsCommandList* cmd,ID3D12Resource* image,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to){
 D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={image,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,from,to};cmd->ResourceBarrier(1,&b);
}
int main(int argc,char**argv){
 bool sdr=false,endOn=false;for(int n=1;n<argc;++n){if(!strcmp(argv[n],"sdr"))sdr=true;else if(!strcmp(argv[n],"on-exit"))endOn=true;else return 56;}
 FILE* log=nullptr;fopen_s(&log,"presenter-resize-probe.jsonl","w");if(!log)return 1;
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
 // The qualified ReShade framework provides the equivalent update in the game.
 uint64_t epoch=0;
 auto reset=[&](){if(FAILED(allocator->Reset())||FAILED(cmd->Reset(allocator,nullptr)))return false;++epoch;return SUCCEEDED(cmd->SetPrivateData(mcd2_reset_epoch_guid,sizeof(epoch),&epoch));};
 WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"MCD2AmdGameBridgeProbe";RegisterClassW(&wc);
 HWND window=CreateWindowW(wc.lpszClassName,L"Owned AMD FG bridge test",WS_OVERLAPPEDWINDOW,0,0,W,H,nullptr,nullptr,wc.hInstance,nullptr);if(!window)return 8;ShowWindow(window,SW_SHOW);
 MCD2AmdFgSwapV1 desc{sizeof(desc),W,H,unsigned(DXGI_FORMAT_R10G10B10A2_UNORM),1,0,0x30,3,unsigned(DXGI_SCALING_STRETCH),unsigned(DXGI_SWAP_EFFECT_FLIP_DISCARD),unsigned(DXGI_ALPHA_MODE_UNSPECIFIED),0x802,0,1,0,1,0,0};
 if(!WritePrivateProfileStringW(L"Providers",L"AmdPresenterSwitch",L"1",(std::filesystem::current_path()/L"FGBootstrap.ini").c_str()))return 54;
 void* created=nullptr;if(result("create-bridge-swapchain",swap(factory,queue,window,&desc,&created))||!created)return 9;
 IDXGISwapChain4* chain=nullptr;auto* initial=static_cast<IDXGISwapChain1*>(created);auto hr=initial->QueryInterface(IID_PPV_ARGS(&chain));initial->Release();if(FAILED(hr))return 10;
 if(result("owned-presenter-antilag-ready",antilag_ready(device))||antilag_ready(nullptr)==0)return 31;
 if(antilag(device,nullptr,1,0)!=E_INVALIDARG||antilag(device,nullptr,0,2)!=E_INVALIDARG)return 32;
 if(result("clear-antilag-before-presentation",antilag(device,nullptr,0,1)))return 33;
 ID3D12CommandQueue* unrelated=nullptr;if(FAILED(device->CreateCommandQueue(&q,IID_PPV_ARGS(&unrelated))))return 36;
 const auto refused=present(unrelated,100,1);unrelated->Release();
 if(result("reject-unrelated-submission-queue",refused==-41?0:refused?refused:-1))return 37;
 HANDLE waitable=chain->GetFrameLatencyWaitableObject();if(waitable)return 11;
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
 unsigned identity=0;

 const GUID binaryTag={0x18b57e4,0x1493,0x4953,{0xad,0xf2,0xde,0x6d,0x99,0xcc,0x5,0xe5}};
 const GUID interfaceTag={0x194e5001,0x1493,0x4953,{0xad,0xf2,0xde,0x6d,0x99,0xcc,0x5,0xe5}};
 unsigned colourTag=sdr?0:12;if(FAILED(chain->SetPrivateData(binaryTag,sizeof(colourTag),&colourTag))||FAILED(chain->SetPrivateDataInterface(interfaceTag,device)))return 51;
 auto query=reinterpret_cast<decltype(&mcd2_afg_presenter_switch_state_v1)>(GetProcAddress(bridge,"mcd2_afg_presenter_switch_state_v1"));
 auto route=[&](){unsigned actual=0,eligible=0;return query&&query(&actual,&eligible)==0&&eligible?actual:UINT_MAX;};if(route()!=2)return 40;
 auto* stableIdentity=chain;
 for(unsigned phase=0;phase<(endOn?6u:5u);++phase){
  if(phase>=2){
   if(present(queue,identity+100,0)||!wait()||!reset()||FAILED(cmd->Close()))return 41;
   auto local=std::make_unique<wchar_t[]>(32768);auto n=GetEnvironmentVariableW(L"LOCALAPPDATA",local.get(),32768);if(!n||n>=32768)return 42;
   using namespace mcd2::providers;auto folder=std::filesystem::path(local.get())/L"Dungeons2"/L"Saved"/L"MCD2Graphics"/L"ProviderSettings";std::filesystem::create_directories(folder);GraphicsStore store(folder);DecodedGraphicsRecord old;auto loaded=store.load(old);auto status=loaded.status;
   if(status!=StoreStatus::Ok&&status!=StoreStatus::Missing)return 43;
   GraphicsIntent intent;if(status==StoreStatus::Ok){intent=old.intent;intent.revision++;}intent.sr=SrProvider::AmdFsr;intent.fg=FgProvider::Amd;intent.fgEnabled=(phase==3||phase==5);
   if(store.publish(intent,status==StoreStatus::Ok?loaded.stamp:RecordStamp{}).status!=StoreStatus::Ok)return 44;
   if(!WritePrivateProfileStringW(L"Providers",L"AmdPresenterSwitch",L"1",(std::filesystem::current_path()/L"FGBootstrap.ini").c_str()))return 45;
   if(FAILED(chain->ResizeBuffers(3,W,H,DXGI_FORMAT_R10G10B10A2_UNORM,0x802))||route()!=((phase==3||phase==5)?2u:1u))return 46;
   IDXGISwapChain4* same=nullptr;if(FAILED(chain->QueryInterface(IID_PPV_ARGS(&same)))||same!=stableIdentity)return 55;same->Release();
   if(output(&contract))return 50;
   unsigned readTag=UINT_MAX,tagSize=sizeof(readTag);IUnknown* held=nullptr;UINT heldSize=sizeof(held);
   if(FAILED(chain->GetPrivateData(binaryTag,&tagSize,&readTag))||tagSize!=sizeof(readTag)||readTag!=colourTag||FAILED(chain->GetPrivateData(interfaceTag,&heldSize,&held))||!held)return 52;
   bool identity=held==static_cast<IUnknown*>(device);held->Release();if(!identity)return 53;
   fprintf(log,"{\"stage\":\"presenter-handoff\",\"route\":%u,\"identityStable\":true,\"privateDataPreserved\":true}\n",route());fflush(log);
  }
  const bool enabled=phase==1||(phase==3||phase==5);const unsigned frames=enabled?60:30;
  MCD2AmdFgStateV1 before{};before.size=sizeof(before);if(state(&before))return 14;
  for(unsigned n=0;n<frames;++n,++identity){MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}
   if(waitable||!reset())return 15;
   handle=views->GetCPUDescriptorHandleForHeapStart();const float clear[3][4]={{.5f,.55f,.6f,1},{.5f,0,0,0},{0,0,0,0}};
   for(unsigned i=0;i<3;++i){if(identity)transition(cmd,images[i],i?D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:Read,D3D12_RESOURCE_STATE_RENDER_TARGET);cmd->ClearRenderTargetView(handle,clear[i],0,nullptr);transition(cmd,images[i],D3D12_RESOURCE_STATE_RENDER_TARGET,i?D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:Read);handle.ptr+=stride;}
   if(enabled){MCD2AmdFgGuidesV1 p{};p.size=sizeof(p);p.frameTimeMs=16.667f;p.worldToMeters=1;p.camera.size=sizeof(p.camera);p.camera.frame=identity+100;p.camera.width=W;p.camera.height=H;p.camera.reset=n==0;p.camera.nearPlane=.1f;p.camera.farPlane=1000;p.camera.verticalFOV=1.04719755f;p.camera.up[1]=1;p.camera.right[0]=1;p.camera.forward[2]=1;
    if(result("prepare-game-bridge-inputs",guides(cmd,images[1],images[2],&p)))return 16;
    if(result("copy-hudless-world",world(cmd,images[0],p.camera.frame)))return 17;
   }
   ID3D12Resource* back=nullptr;if(FAILED(chain->GetBuffer(chain->GetCurrentBackBufferIndex(),IID_PPV_ARGS(&back))))return 20;
   transition(cmd,images[0],Read,D3D12_RESOURCE_STATE_COPY_SOURCE);transition(cmd,back,D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_COPY_DEST);cmd->CopyResource(back,images[0]);transition(cmd,back,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PRESENT);transition(cmd,images[0],D3D12_RESOURCE_STATE_COPY_SOURCE,Read);back->Release();
   if(FAILED(cmd->Close()))return 21;ID3D12CommandList* commands[]={cmd};queue->ExecuteCommandLists(1,commands);
   if(result("configure-bridge-present",present(queue,identity+100,enabled)))return 19;
   if(FAILED(chain->Present(0,0))||!wait()||FAILED(device->GetDeviceRemovedReason()))return 22;Sleep(16);
  }
  MCD2AmdFgStateV1 after{};after.size=sizeof(after);auto deadline=GetTickCount64()+5000;
  do{if(state(&after))return 23;if(after.realPresents-before.realPresents>=frames)break;Sleep(10);}while(GetTickCount64()<deadline);
  const auto real=after.realPresents-before.realPresents,generated=after.generatedPresents-before.generatedPresents;
  fprintf(log,"{\"stage\":\"phase\",\"phase\":%u,\"enabled\":%s,\"real\":%llu,\"generated\":%llu,\"fault\":%u,\"errors\":%u,\"warnings\":%u}\n",phase,enabled?"true":"false",real,generated,after.fault,after.errors,after.warnings);fflush(log);
  if(real!=frames||after.fault||after.errors||(!enabled&&generated)||(enabled&&generated<frames/2))return 24;
 }

 MCD2AmdFgStateV1 finalState{};finalState.size=sizeof(finalState);if(state(&finalState)||result("retire-request-state",finalState.active==(endOn?1u:0u)?0:-1))return 57;
 if(!wait()||!reset())return 47;
 transition(cmd,images[0],Read,D3D12_RESOURCE_STATE_COPY_SOURCE);transition(cmd,images[0],D3D12_RESOURCE_STATE_COPY_SOURCE,Read);
 if(FAILED(cmd->Close())||retire(nullptr,0))return 48;
 ID3D12CommandList* replay[]={cmd};queue->ExecuteCommandLists(1,replay);if(!wait())return 49;
 chain->Release();for(auto* image:images)image->Release();views->Release();cmd->Release();allocator->Release();fence->Release();CloseHandle(event);queue->Release();device->Release();factory->Release();DestroyWindow(window);FreeLibrary(bridge);
 result("complete",0);fclose(log);return 0;
}
