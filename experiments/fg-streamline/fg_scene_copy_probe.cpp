// Owned synthetic SDR pixels only: no game resources, authentication or saves.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <reshade.hpp>
#include <map>
#include <mutex>
#include <memory>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cstdio>
#include "../../src/latency/process_exit.hpp"
namespace a=reshade::api;
#include "fg_alpha_copy.h"
#include "fg_scene_copy.h"
template<class T>void drop(T*&p){if(p){p->Release();p=nullptr;}}
int main(){
 mcd2::process_exit::initialize();FILE*log=nullptr;fopen_s(&log,"sdr-copy-probe.jsonl","w");if(!log)return 1;
 ID3D12Device*device=nullptr;if(FAILED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&device))))return 2;
 ID3D12CommandQueue*queue=nullptr;D3D12_COMMAND_QUEUE_DESC q{};if(FAILED(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue))))return 3;
 ID3D12Fence*fence=nullptr;if(FAILED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence))))return 4;
 HANDLE done=CreateEventW(nullptr,FALSE,FALSE,nullptr);uint64_t value=0;
 auto wait=[&](){return done&&SUCCEEDED(queue->Signal(fence,++value))&&SUCCEEDED(fence->SetEventOnCompletion(value,done))&&WaitForSingleObject(done,10000)==WAIT_OBJECT_0&&fence->GetCompletedValue()!=UINT64_MAX;};
 for(auto format:{DXGI_FORMAT_R10G10B10A2_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_B8G8R8A8_UNORM}){
  constexpr unsigned W=17,H=9;const unsigned maximum=format==DXGI_FORMAT_R10G10B10A2_UNORM?1023:255;
  const unsigned rgb[3]{maximum/3,maximum/2,maximum};
  ID3D12Resource*source=nullptr,*readback=nullptr;ID3D12DescriptorHeap*rtv=nullptr;ID3D12CommandAllocator*allocator=nullptr;ID3D12GraphicsCommandList*cmd=nullptr;
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=W;d.Height=H;d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Format=format;d.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET|D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
  if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&source))))return 5;
  D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;hd.NumDescriptors=1;if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&rtv))))return 6;
  device->CreateRenderTargetView(source,nullptr,rtv->GetCPUDescriptorHandleForHeapStart());
  if(FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)))||FAILED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator,nullptr,IID_PPV_ARGS(&cmd))))return 7;
  float colour[4]{float(rgb[0])/maximum,float(rgb[1])/maximum,1,0};cmd->ClearRenderTargetView(rtv->GetCPUDescriptorHandleForHeapStart(),colour,0,nullptr);
  auto image=fg_scene::copy(cmd,source,W,H);if(!image)return 8;auto copied=image->GetDesc();D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout{};UINT64 total=0;device->GetCopyableFootprints(&copied,0,1,0,&layout,nullptr,nullptr,&total);
  hp.Type=D3D12_HEAP_TYPE_READBACK;D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=total;buffer.Height=buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback))))return 9;
  constexpr auto read=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  fg_alpha::transition(cmd,image,read,D3D12_RESOURCE_STATE_COPY_SOURCE);
  D3D12_TEXTURE_COPY_LOCATION from{};from.pResource=image;from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;D3D12_TEXTURE_COPY_LOCATION to{};to.pResource=readback;to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=layout;cmd->CopyTextureRegion(&to,0,0,0,&from,nullptr);fg_alpha::transition(cmd,image,D3D12_RESOURCE_STATE_COPY_SOURCE,read);
  if(FAILED(cmd->Close()))return 10;ID3D12CommandList*lists[]{cmd};queue->ExecuteCommandLists(1,lists);if(!fg_scene::submission_queue_native(queue)||!wait())return 11;
  unsigned differences=0,alphaErrors=0;void*bytes=nullptr;D3D12_RANGE range{0,size_t(total)};if(FAILED(readback->Map(0,&range,&bytes)))return 12;
  for(unsigned y=0;y<H;++y)for(unsigned x=0;x<W;++x){auto pixel=*reinterpret_cast<const uint32_t*>(static_cast<const unsigned char*>(bytes)+layout.Offset+y*layout.Footprint.RowPitch+x*4);unsigned actual[4]{};
   if(format==DXGI_FORMAT_R10G10B10A2_UNORM){actual[0]=pixel&1023;actual[1]=(pixel>>10)&1023;actual[2]=(pixel>>20)&1023;actual[3]=pixel>>30;alphaErrors+=actual[3]!=3;}else{actual[0]=pixel&255;actual[1]=(pixel>>8)&255;actual[2]=(pixel>>16)&255;actual[3]=pixel>>24;alphaErrors+=actual[3]!=255;}
   for(unsigned c=0;c<3;++c)differences+=actual[c]!=rgb[c];
  }
  D3D12_RANGE none{};readback->Unmap(0,&none);
  // No replay owner remains before the helper obtains fresh GPU completion.
  drop(cmd);drop(allocator);const bool retired=fg_scene::cleanup();
  fprintf(log,"{\"format\":%u,\"pixels\":%u,\"RGBCodeDifferences\":%u,\"alphaErrors\":%u,\"retired\":%s}\n",unsigned(format),W*H,differences,alphaErrors,retired?"true":"false");fflush(log);
  drop(readback);drop(rtv);drop(source);if(differences||alphaErrors||!retired)return 13;
 }
 CloseHandle(done);drop(fence);drop(queue);drop(device);fclose(log);return 0;
}
