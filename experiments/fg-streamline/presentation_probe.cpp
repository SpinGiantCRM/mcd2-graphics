#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdio>
#include <cwchar>
#include <memory>
extern "C" {
long mcd2_sl_verify(const wchar_t*);int mcd2_sl_init(const wchar_t*,const wchar_t*,const wchar_t*);int mcd2_sl_set_device(void*);int mcd2_sl_mode(unsigned);int mcd2_sl_begin(unsigned,void**);unsigned mcd2_sl_index(void*);int mcd2_sl_sleep(void*);int mcd2_sl_marker(void*,unsigned);int mcd2_sl_state(unsigned*,unsigned*,unsigned*);int mcd2_sl_shutdown();void mcd2_fg_unload();int mcd2_fg_support(void*,unsigned);int mcd2_fg_upgrade(void**);int mcd2_fg_state(unsigned*,unsigned*,unsigned*,unsigned*);
}
struct Session {
 IDXGIFactory4* factory=nullptr; IDXGIFactory4* proxyFactory=nullptr; ID3D12Device* proxyDevice=nullptr; ID3D12Device* device=nullptr; ID3D12CommandQueue* queue=nullptr; IDXGISwapChain1* swap=nullptr; ID3D12CommandAllocator* allocator=nullptr; ID3D12GraphicsCommandList* cmd=nullptr; ID3D12Fence* fence=nullptr; HWND window=nullptr; HANDLE done=nullptr;
 ~Session(){mcd2_sl_mode(0);mcd2_sl_shutdown();if(fence)fence->Release();if(cmd)cmd->Release();if(allocator)allocator->Release();if(swap)swap->Release();if(queue)queue->Release();if(proxyDevice && proxyDevice!=device)proxyDevice->Release();if(proxyFactory && proxyFactory!=factory)proxyFactory->Release();if(device)device->Release();if(factory)factory->Release();if(done)CloseHandle(done);if(window)DestroyWindow(window);mcd2_fg_unload();}
};
int main(){
 FILE*log=nullptr;fopen_s(&log,"fg-probe.jsonl","w");if(!log)return 1;auto emit=[&](const char*stage,long result){fprintf(log,"{\"stage\":\"%s\",\"result\":%ld}\n",stage,result);fflush(log);};
 auto folder=std::make_unique<wchar_t[]>(32768),dll=std::make_unique<wchar_t[]>(32768);auto length=GetCurrentDirectoryW(32768,folder.get());if(!length||length>=32768)return 1;if(swprintf_s(dll.get(),32768,L"%s\\sl.interposer.dll",folder.get())<0)return 1;
 auto trust=mcd2_sl_verify(dll.get());emit("authenticode",trust);if(trust)return 2;
 auto init=mcd2_sl_init(dll.get(),folder.get(),folder.get());emit("slInit",init);if(init){mcd2_sl_shutdown();mcd2_fg_unload();return 3;}
 Session c;
 auto& device=c.device;auto hr=D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&device));emit("D3D12CreateDevice",hr);if(FAILED(hr))return 4;
 auto luid=device->GetAdapterLuid();auto support=mcd2_fg_support(&luid,sizeof(luid));emit("DLSSG-support-rendering-adapter",support);if(support)return 40;
 auto set=mcd2_sl_set_device(device);emit("slSetD3DDevice",set);if(set)return 5;
 unsigned available=0,valid=0,complete=0;auto state=mcd2_sl_state(&available,&valid,&complete);fprintf(log,"{\"stage\":\"capability\",\"result\":%d,\"lowLatencyAvailable\":%u}\n",state,available);fflush(log);if(state||!available)return 6;
 // Real isolated presents exercise reporting; this is not an MCD2 performance benchmark.
 WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"MCD2StreamlineIsolatedProbe";RegisterClassW(&wc);
 c.window=CreateWindowW(wc.lpszClassName,L"Isolated Streamline provider check",WS_OVERLAPPEDWINDOW,0,0,320,180,nullptr,nullptr,wc.hInstance,nullptr);if(!c.window)return 11;ShowWindow(c.window,SW_SHOWNOACTIVATE);
 if(FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&c.factory))))return 12;
 auto& proxyDevice=c.proxyDevice;proxyDevice=device;auto upgraded=mcd2_fg_upgrade(reinterpret_cast<void**>(&proxyDevice));emit("upgrade-device",upgraded);if(upgraded)return 41;
 auto& proxyFactory=c.proxyFactory;proxyFactory=c.factory;upgraded=mcd2_fg_upgrade(reinterpret_cast<void**>(&proxyFactory));emit("upgrade-factory",upgraded);if(upgraded)return 42;
 D3D12_COMMAND_QUEUE_DESC qd{};qd.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;if(FAILED(proxyDevice->CreateCommandQueue(&qd,IID_PPV_ARGS(&c.queue))))return 13;
 DXGI_SWAP_CHAIN_DESC1 sd{};sd.Width=320;sd.Height=180;sd.Format=DXGI_FORMAT_R8G8B8A8_UNORM;sd.SampleDesc.Count=1;sd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;sd.BufferCount=3;sd.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
 if(FAILED(proxyFactory->CreateSwapChainForHwnd(c.queue,c.window,&sd,nullptr,nullptr,&c.swap)))return 14;
 unsigned maximum=0,fgStatus=0,minimum=0,presented=0;auto fg=mcd2_fg_state(&maximum,&fgStatus,&minimum,&presented);fprintf(log,"{\"stage\":\"DLSSG-state-off\",\"result\":%d,\"maximumGeneratedFrames\":%u,\"minimumDimension\":%u,\"status\":%u}\n",fg,maximum,minimum,fgStatus);fflush(log);if(fg)return 43;
 if(FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&c.allocator))))return 15;
 if(FAILED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,c.allocator,nullptr,IID_PPV_ARGS(&c.cmd))))return 16;
 if(FAILED(c.cmd->Close()))return 17;
 if(FAILED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&c.fence))))return 18;c.done=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!c.done)return 19;
 for(unsigned mode=0;mode<3;++mode){auto r=mcd2_sl_mode(mode);emit("slReflexSetOptions",r);if(r)return 7;for(unsigned n=0;n<60;++n){unsigned index=mode*60+n+1;void*token=nullptr;r=mcd2_sl_begin(index,&token);if(r||!token||mcd2_sl_index(token)!=index)return 8;r=mcd2_sl_sleep(token);if(r)return 9;if(mcd2_sl_marker(token,0))return 10;
 MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}if(mcd2_sl_marker(token,1)||mcd2_sl_marker(token,2))return 10;
 if(FAILED(c.allocator->Reset())||FAILED(c.cmd->Reset(c.allocator,nullptr))||FAILED(c.cmd->Close()))return 20;ID3D12CommandList*lists[]={c.cmd};c.queue->ExecuteCommandLists(1,lists);
 if(mcd2_sl_marker(token,3)||mcd2_sl_marker(token,4))return 10;hr=c.swap->Present(0,0);if(FAILED(hr))return 21;if(mcd2_sl_marker(token,5))return 10;
 if(FAILED(c.queue->Signal(c.fence,index))||FAILED(c.fence->SetEventOnCompletion(index,c.done))||WaitForSingleObject(c.done,5000)!=WAIT_OBJECT_0)return 22;}state=mcd2_sl_state(&available,&valid,&complete);fprintf(log,"{\"stage\":\"mode\",\"mode\":%u,\"result\":%d,\"calls\":60,\"reportAvailable\":%u,\"completeReports\":%u}\n",mode,state,valid,complete);fflush(log);}
 mcd2_sl_mode(0);emit("slShutdown",mcd2_sl_shutdown());fclose(log);return 0;
}
