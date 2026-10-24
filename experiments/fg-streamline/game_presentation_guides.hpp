#pragma once
#include <dxgi1_6.h>
// The SL public backbuffers and render queue differ from its physical presenter.
extern "C" __declspec(dllexport) int mcd2_fg_game_present_v1(uint64_t owner,uint64_t nativeQueue,IDXGISwapChain4*swap){
 if(mcd2::process_exit::terminating())return 0;
 if(!owner||!swap)return 0;
 HWND window=nullptr;DXGI_SWAP_CHAIN_DESC1 desc{};
 if(FAILED(swap->GetHwnd(&window))||FAILED(swap->GetDesc1(&desc))||!desc.BufferCount||desc.BufferCount>16)return 0;
 DWORD pid=0;wchar_t windowClass[256]{};GetWindowThreadProcessId(window,&pid);GetClassNameW(window,windowClass,256);
 if(pid!=GetCurrentProcessId()||wcscmp(windowClass,L"UnrealWindow")!=0)return 0;
 std::set<uint64_t> buffers;
 for(unsigned i=0;i<desc.BufferCount;++i){
  ID3D12Resource*resource=nullptr;if(FAILED(swap->GetBuffer(i,IID_PPV_ARGS(&resource))))return 0;
  buffers.insert(reinterpret_cast<uint64_t>(resource));resource->Release();
 }
 a::command_queue*queue=nullptr;a::swapchain*physical=nullptr;
 {
  std::lock_guard lock(mutex);auto q=gameQueues.find(nativeQueue);if(q==gameQueues.end())return 0;queue=q->second;
  if(queue->get_device()->get_api()!=a::device_api::d3d12)return 0;
  for(const auto&[candidate,hwnd]:gameSwaps)if(hwnd==window&&candidate->get_device()==queue->get_device()){
   if(physical)return 0;physical=candidate;
  }
  if(!physical)return 0;
  if(gamePresentationOwner!=owner){privateLog<<"{\"kind\":\"game_presentation_owner\",\"publicBuffers\":"<<buffers.size()<<",\"applicationQueue\":true}\n";privateLog.flush();}
  gameBackbuffers=std::move(buffers);gamePresentationOwner=owner;
 }
 const bool previous=gamePresentationCallback;gamePresentationCallback=true;
 present(queue,physical,nullptr,nullptr,0,nullptr);gamePresentationCallback=previous;return 1;
}
extern "C" __declspec(dllexport) int mcd2_fg_game_present_done_v1(uint64_t owner,uint64_t nativeQueue,IDXGISwapChain4*){
 if(mcd2::process_exit::terminating())return 0;
 a::command_queue*queue=nullptr;
 {std::lock_guard lock(mutex);if(gamePresentationOwner!=owner)return 0;auto q=gameQueues.find(nativeQueue);if(q==gameQueues.end())return 0;queue=q->second;}
 const bool previous=gamePresentationCallback;gamePresentationCallback=true;
 finish(queue,nullptr);gamePresentationCallback=previous;return 1;
}
extern "C" __declspec(dllexport) int mcd2_fg_game_present_end_v1(uint64_t owner,uint64_t,IDXGISwapChain4*){
 if(mcd2::process_exit::terminating())return 0;
 std::lock_guard lock(mutex);if(gamePresentationOwner==owner){gamePresentationOwner=0;gameBackbuffers.clear();}return 1;
}
