#pragma once
// Provider-independent current-frame producer. No SDK creation/evaluation here.
// Every consumer owns its generation and command-recording lease cache.
#include "lean_borrow_lifetime.hpp"
struct SrGuideInput {
 ID3D12Resource *colour=nullptr,*depth=nullptr,*packed=nullptr,*exposure=nullptr,*output=nullptr;
 Slot pass{},view{}; a::resource_view_desc exposure_view{}; unsigned depth_format=0;
};
struct SrGuideGeneration {
 EvalOwned owned;
 ID3D12Device *proxy_device=nullptr,*resource_device=nullptr;
 ID3D12Resource *motion=nullptr,*exposure=nullptr,*current_depth=nullptr;
 ID3D12RootSignature *current_signature=nullptr;
 ID3D12PipelineState *current_pipeline=nullptr;
 bool current_resources_initialized=false;
 unsigned width=0,height=0;
};
template<class Generation>
static const char *record_sr_guides(Generation &c,LeanBorrowCache &cache,a::command_list *cmd,
 ID3D12GraphicsCommandList *proxy,const Cmd &saved,const SrGuideInput &input) {
 auto *gc=input.colour,*gd=input.depth,*gv=input.packed,*ge=input.exposure,*go=input.output;
 const auto *pass=&input.pass,*view=&input.view;const auto ev=input.exposure_view;
 if(!c.current_signature){
  c.current_depth=eval_texture(c.resource_device,c.owned,c.width,c.height,DXGI_FORMAT_R32_FLOAT,true);
  std::ifstream f(asset_root/"live_dense.cso",std::ios::binary|std::ios::ate);if(!c.current_depth || !f){return ("converter_resources");}
  std::vector<char> code(size_t(f.tellg()));f.seekg(0);f.read(code.data(),code.size());
  D3D12_DESCRIPTOR_RANGE ranges[2]{{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,2,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,3,0,0,0}};
  D3D12_ROOT_PARAMETER rp[5]{};rp[0].ParameterType=rp[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;rp[0].Descriptor={0,0};rp[1].Descriptor={1,0};rp[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;rp[2].Descriptor={2,0};rp[3].ParameterType=rp[4].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;rp[3].DescriptorTable={1,&ranges[0]};rp[4].DescriptorTable={1,&ranges[1]};
  D3D12_ROOT_SIGNATURE_DESC rd{5,rp,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ID3DBlob *blob=nullptr,*error=nullptr;auto hr=D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error);c.owned.keep(blob);c.owned.keep(error);
  if(SUCCEEDED(hr))hr=c.proxy_device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&c.current_signature));c.owned.keep(c.current_signature);
  D3D12_COMPUTE_PIPELINE_STATE_DESC p{};p.pRootSignature=c.current_signature;p.CS={code.data(),code.size()};if(SUCCEEDED(hr))hr=c.proxy_device->CreateComputePipelineState(&p,IID_PPV_ARGS(&c.current_pipeline));c.owned.keep(c.current_pipeline);
  if(FAILED(hr)){return ("converter_pipeline");}
 }
 if(!cache.fence && FAILED(c.resource_device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&cache.fence)))){return ("retirement_fence_creation");}
 LeanBorrowKey key{reinterpret_cast<uint64_t>(gv),reinterpret_cast<uint64_t>(gd),input.depth_format,reinterpret_cast<uint64_t>(gc),reinterpret_cast<uint64_t>(ge),reinterpret_cast<uint64_t>(go),pass->binding.cb.buffer.handle,view->binding.cb.buffer.handle};
 ID3D12DescriptorHeap *heap=nullptr;auto hit=cache.entries.find(key);
 if(hit!=cache.entries.end())heap=hit->second.heap;else{
  if(cache.entries.size()>=256){return ("immutable_heap_cache_limit");}
  D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=5;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  if(FAILED(c.proxy_device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap)))){return ("converter_heap");}LeanBorrowEntry entry;entry.heap=heap;entry.resources={gc,gd,gv,ge,go,reinterpret_cast<ID3D12Resource*>(pass->binding.cb.buffer.handle),reinterpret_cast<ID3D12Resource*>(view->binding.cb.buffer.handle)};
  for(auto *r:entry.resources)r->AddRef();cache.entries.emplace(key,entry);
  cache.peak_entries=std::max<uint64_t>(cache.peak_entries,cache.entries.size());
  // Cache immutable descriptors and retain their exact resources. No descriptor
  // rewriting or per-frame heap allocation while previous frames are in flight.
  // The bundle now owns every borrowed resource used by conversion/NGX.
  auto cpu=heap->GetCPUDescriptorHandleForHeapStart();auto inc=c.proxy_device->GetDescriptorHandleIncrementSize(hd.Type);
  D3D12_SHADER_RESOURCE_VIEW_DESC sd{};sd.Format=DXGI_FORMAT_R16G16B16A16_UNORM;sd.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sd.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sd.Texture2D.MipLevels=1;c.proxy_device->CreateShaderResourceView(gv,&sd,cpu);cpu.ptr+=inc;
  sd.Format=static_cast<DXGI_FORMAT>(std::get<2>(key));c.proxy_device->CreateShaderResourceView(gd,&sd,cpu);cpu.ptr+=inc;
  D3D12_UNORDERED_ACCESS_VIEW_DESC ud{};ud.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;ud.Format=DXGI_FORMAT_R16G16_FLOAT;c.proxy_device->CreateUnorderedAccessView(c.motion,nullptr,&ud,cpu);cpu.ptr+=inc;ud.Format=DXGI_FORMAT_R32_FLOAT;c.proxy_device->CreateUnorderedAccessView(c.exposure,nullptr,&ud,cpu);cpu.ptr+=inc;c.proxy_device->CreateUnorderedAccessView(c.current_depth,nullptr,&ud,cpu);
 }
 cache.record(cmd,key,frame);
 auto *native=reinterpret_cast<ID3D12GraphicsCommandList*>(cmd->get_native());
 if(c.current_resources_initialized)for(auto *r:{c.motion,c.exposure,c.current_depth})eval_transition(native,r,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 auto gpu=heap->GetGPUDescriptorHandleForHeapStart();auto inc=c.proxy_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
 internal_evaluation=true;proxy->SetComputeRootSignature(c.current_signature);proxy->SetPipelineState(c.current_pipeline);proxy->SetDescriptorHeaps(1,&heap);proxy->SetComputeRootConstantBufferView(0,pass->binding.cb.buffer.handle?reinterpret_cast<ID3D12Resource*>(pass->binding.cb.buffer.handle)->GetGPUVirtualAddress()+pass->binding.cb.offset:0);proxy->SetComputeRootConstantBufferView(1,reinterpret_cast<ID3D12Resource*>(view->binding.cb.buffer.handle)->GetGPUVirtualAddress()+view->binding.cb.offset);proxy->SetComputeRootShaderResourceView(2,ge->GetGPUVirtualAddress()+ev.buffer.offset);proxy->SetComputeRootDescriptorTable(3,gpu);gpu.ptr+=2*inc;proxy->SetComputeRootDescriptorTable(4,gpu);proxy->Dispatch((c.width+7)/8,(c.height+7)/8,1);internal_evaluation=false;
 D3D12_RESOURCE_BARRIER order{};order.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;
 for(auto *r:{c.motion,c.exposure,c.current_depth}){order.UAV.pResource=r;native->ResourceBarrier(1,&order);eval_transition(native,r,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);}c.current_resources_initialized=true;
 restore_root(cmd,saved,false,true);restore_root(cmd,saved,true,true);cmd->bind_pipeline(a::pipeline_stage::all_compute,{saved.pipeline});commands[cmd]=saved;
 return nullptr;
}
