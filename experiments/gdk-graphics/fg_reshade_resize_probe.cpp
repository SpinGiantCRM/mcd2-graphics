#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdio>
#include <memory>
#include <cstring>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
static bool result(const char*,HRESULT);
static bool wait_queue(ID3D12Device*device,ID3D12CommandQueue*queue){
 ComPtr<ID3D12Fence>fence;if(!result("CreateFence",device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence))))return false;
 if(!result("Signal",queue->Signal(fence.Get(),1)))return false;
 if(fence->GetCompletedValue()>=1)return true;auto event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event)return false;
 const auto status=fence->SetEventOnCompletion(1,event);const bool done=SUCCEEDED(status)&&WaitForSingleObject(event,5000)==WAIT_OBJECT_0;CloseHandle(event);return done;
}
static bool draw_red(ID3D12Device*device,ID3D12CommandQueue*queue,IDXGISwapChain1*swap){
 ComPtr<IDXGISwapChain3>swap3;if(!result("SwapChain3",swap->QueryInterface(IID_PPV_ARGS(&swap3))))return false;
 ComPtr<ID3D12Resource>buffer;if(!result("GetRenderBuffer",swap->GetBuffer(swap3->GetCurrentBackBufferIndex(),IID_PPV_ARGS(&buffer))))return false;
 D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};heapDesc.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;heapDesc.NumDescriptors=1;
 ComPtr<ID3D12DescriptorHeap>heap;if(!result("CreateRTVHeap",device->CreateDescriptorHeap(&heapDesc,IID_PPV_ARGS(&heap))))return false;
 const auto view=heap->GetCPUDescriptorHandleForHeapStart();device->CreateRenderTargetView(buffer.Get(),nullptr,view);
 ComPtr<ID3D12CommandAllocator>allocator;if(!result("CreateAllocator",device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))))return false;
 ComPtr<ID3D12GraphicsCommandList>list;if(!result("CreateList",device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list))))return false;
 D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition={buffer.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_RENDER_TARGET};list->ResourceBarrier(1,&barrier);
 const float red[]={1,0,0,1};list->ClearRenderTargetView(view,red,0,nullptr);barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_RENDER_TARGET;barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_PRESENT;list->ResourceBarrier(1,&barrier);
 if(!result("CloseList",list->Close()))return false;ID3D12CommandList*lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);return wait_queue(device,queue);
}
static bool read_red(ID3D12Device*device,ID3D12CommandQueue*queue,ID3D12Resource*source,const char*label){
 const auto sourceDesc=source->GetDesc();if(sourceDesc.Format!=DXGI_FORMAT_R10G10B10A2_UNORM||!sourceDesc.Width||!sourceDesc.Height){printf("%s unsupportedFormat=%u\n",label,unsigned(sourceDesc.Format));return false;}
 D3D12_HEAP_PROPERTIES properties{};properties.Type=D3D12_HEAP_TYPE_READBACK;properties.CreationNodeMask=properties.VisibleNodeMask=1;
 D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=256;desc.Height=1;desc.DepthOrArraySize=1;desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
 ComPtr<ID3D12Resource>readback;if(!result("CreateReadback",device->CreateCommittedResource(&properties,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback))))return false;
 ComPtr<ID3D12CommandAllocator>allocator;if(!result("ReadbackAllocator",device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))))return false;
 ComPtr<ID3D12GraphicsCommandList>list;if(!result("ReadbackList",device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list))))return false;
 D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition={source,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&barrier);
 D3D12_TEXTURE_COPY_LOCATION from{};from.pResource=source;from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
 D3D12_TEXTURE_COPY_LOCATION to{};to.pResource=readback.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint.Footprint={sourceDesc.Format,1,1,1,256};
 const auto x=UINT(sourceDesc.Width/2),y=sourceDesc.Height/2;const D3D12_BOX box{x,y,0,x+1,y+1,1};list->CopyTextureRegion(&to,0,0,0,&from,&box);
 barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_SOURCE;barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_PRESENT;list->ResourceBarrier(1,&barrier);if(!result("CloseReadback",list->Close()))return false;
 ID3D12CommandList*lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);if(!wait_queue(device,queue))return false;
 void*data=nullptr;const D3D12_RANGE read{0,4};if(!result("MapReadback",readback->Map(0,&read,&data)))return false;unsigned pixel=0;memcpy(&pixel,data,4);const D3D12_RANGE written{0,0};readback->Unmap(0,&written);
 printf("%s pixel=0x%08x\n",label,pixel);return (pixel&1023)>900&&((pixel>>10)&1023)<100&&((pixel>>20)&1023)<100;
}
static bool result(const char*stage,HRESULT value){printf("%s=0x%08lx\n",stage,static_cast<unsigned long>(value));fflush(stdout);return SUCCEEDED(value);}
int main(int argc,char**argv){
 const bool noResize=argc==2&&!strcmp(argv[1],"no-resize");if(argc>2||(argc==2&&!noResize))return 64;
 auto path=std::make_unique<wchar_t[]>(32768);auto length=GetFullPathNameW(L"dxgi.dll",32768,path.get(),nullptr);if(!length||length>=32768)return 1;
 auto bootstrap=LoadLibraryExW(path.get(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
 if(!bootstrap){printf("LoadLibrary=%lu\n",GetLastError());return 1;}
 using Factory=HRESULT(WINAPI*)(UINT,REFIID,void**);
 auto createFactory=reinterpret_cast<Factory>(GetProcAddress(bootstrap,"CreateDXGIFactory2"));if(!createFactory)return 2;
 IDXGIFactory2*factory=nullptr;if(!result("CreateDXGIFactory2",createFactory(0,IID_PPV_ARGS(&factory))))return 3;
 auto reshade=GetModuleHandleW(L"d3d12.asi");if(!reshade)return 4;
 using Device=HRESULT(WINAPI*)(IUnknown*,D3D_FEATURE_LEVEL,REFIID,void**);
 auto createDevice=reinterpret_cast<Device>(GetProcAddress(reshade,"D3D12CreateDevice"));if(!createDevice)return 5;
 ID3D12Device*device=nullptr;if(!result("ReShade-D3D12CreateDevice",createDevice(nullptr,D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&device))))return 6;
 D3D12_COMMAND_QUEUE_DESC queueDesc{};queueDesc.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;
 ID3D12CommandQueue*queue=nullptr;if(!result("CreateCommandQueue",device->CreateCommandQueue(&queueDesc,IID_PPV_ARGS(&queue))))return 7;
 WNDCLASSW windowClass{};windowClass.lpfnWndProc=DefWindowProcW;windowClass.hInstance=GetModuleHandleW(nullptr);windowClass.lpszClassName=L"MCD2ReShadeResizeProbe";RegisterClassW(&windowClass);
 auto window=CreateWindowW(windowClass.lpszClassName,L"MCD2 isolated resize check",WS_OVERLAPPEDWINDOW,0,0,320,180,nullptr,nullptr,windowClass.hInstance,nullptr);if(!window)return 8;ShowWindow(window,SW_SHOWNOACTIVATE);
 DXGI_SWAP_CHAIN_DESC1 desc{};desc.Width=3840;desc.Height=2160;desc.Format=DXGI_FORMAT_R10G10B10A2_UNORM;desc.SampleDesc.Count=1;desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.BufferCount=3;desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;desc.Flags=DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH|DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
 IDXGISwapChain1*swap=nullptr;if(!result("CreateSwapChainForHwnd",factory->CreateSwapChainForHwnd(queue,window,&desc,nullptr,nullptr,&swap)))return 9;
 if(!noResize&&!result("ResizeBuffers",swap->ResizeBuffers(3,desc.Width,desc.Height,desc.Format,desc.Flags)))return 10;
 using Native=int(*)(void*,void**);auto interposer=GetModuleHandleW(L"sl.interposer.dll");auto getNative=interposer?reinterpret_cast<Native>(GetProcAddress(interposer,"slGetNativeInterface")):nullptr;if(!getNative)return 13;
 ComPtr<IDXGISwapChain>inner;if(getNative(swap,reinterpret_cast<void**>(inner.GetAddressOf())))return 14;
 constexpr GUID unwrap={0x7f2c9a11,0x3b4e,0x4d6a,{0x81,0x2f,0x5e,0x9c,0xd3,0x7a,0x1b,0x42}};
 ComPtr<IDXGISwapChain3>physical;if(!result("PhysicalSwapchain",inner->QueryInterface(unwrap,reinterpret_cast<void**>(physical.GetAddressOf()))))return 15;
 ComPtr<IDXGISwapChain3>publicSwap;if(!result("PublicSwapchain",swap->QueryInterface(IID_PPV_ARGS(&publicSwap))))return 16;unsigned redInput=0,redOutput=0;
 for(unsigned i=0;i<10;++i){MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}
  ComPtr<ID3D12Resource>input,output;if(!result("InputBuffer",swap->GetBuffer(publicSwap->GetCurrentBackBufferIndex(),IID_PPV_ARGS(&input)))||!result("OutputBuffer",physical->GetBuffer(physical->GetCurrentBackBufferIndex(),IID_PPV_ARGS(&output))))return 17;
  if(!draw_red(device,queue,swap))return 12;redInput+=read_red(device,queue,input.Get(),"Input");
  if(!result("Present",swap->Present(0,DXGI_PRESENT_ALLOW_TEARING))){result("GetDeviceRemovedReason",device->GetDeviceRemovedReason());return 11;}redOutput+=read_red(device,queue,output.Get(),"Output");
 }
 printf("redInput=%u redOutput=%u\n",redInput,redOutput);if(redInput!=10||!redOutput)return 18;
 physical.Reset();publicSwap.Reset();inner.Reset();
 swap->Release();auto bridge=GetModuleHandleW(L"fg-sdk-bridge.dll");using Shutdown=int(*)();auto shutdown=bridge?reinterpret_cast<Shutdown>(GetProcAddress(bridge,"mcd2_sl_shutdown")):nullptr;if(shutdown)printf("slShutdown=%d\n",shutdown());
 queue->Release();device->Release();using Detach=void(*)();auto detach=reinterpret_cast<Detach>(GetProcAddress(bootstrap,"mcd2_bootstrap_detach"));if(detach)detach();factory->Release();DestroyWindow(window);return 0;
}
