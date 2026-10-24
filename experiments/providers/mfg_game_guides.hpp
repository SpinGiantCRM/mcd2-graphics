#pragma once
#include <d3dcompiler.h>
#include <algorithm>
#include <initializer_list>
// Convert render-pixel guides to the caller-owned SDK's output-pixel lattice.
// Motion values are current-to-previous render pixels. Colour is never sampled
// or transformed here. Point sampling preserves the source depth convention.
struct MfgGameGuides {
 ID3D12Resource* depth{},*motion{};ID3D12RootSignature* root{};ID3D12PipelineState* pipeline{};ID3D12DescriptorHeap* heap{};UINT stride{},width{},height{};
 template<class T> static void release(T*& p){if(p){p->Release();p=nullptr;}}
 void clear(){release(depth);release(motion);release(root);release(pipeline);release(heap);}
 bool init(ID3D12Device* device,UINT w,UINT h){
  if(root)return width==w&&height==h;width=w;height=h;
  constexpr char shader[]=R"(
Texture2D<float> sourceDepth:register(t0);
Texture2D<float2> sourceMotion:register(t1);
RWTexture2D<float> outputDepth:register(u0);
RWTexture2D<float2> outputMotion:register(u1);
cbuffer Dimensions:register(b0){uint2 sourceSize;uint2 outputSize;};
[numthreads(8,8,1)]void main(uint3 id:SV_DispatchThreadID){
 if(any(id.xy>=outputSize))return;
 uint2 source=min(uint2((float2(id.xy)+0.5)*float2(sourceSize)/float2(outputSize)),sourceSize-1);
 outputDepth[id.xy]=sourceDepth.Load(int3(source,0));
 outputMotion[id.xy]=sourceMotion.Load(int3(source,0))*float2(outputSize)/float2(sourceSize);
})";
  ID3DBlob* code{},*errors{},*serialized{};
  auto hr=D3DCompile(shader,sizeof(shader)-1,"mfg_game_guides",nullptr,nullptr,"main","cs_5_0",D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&errors);release(errors);if(FAILED(hr))return false;
  D3D12_DESCRIPTOR_RANGE ranges[2]{};ranges[0]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,2,0,0,0};ranges[1]={D3D12_DESCRIPTOR_RANGE_TYPE_UAV,2,0,0,2};
  D3D12_ROOT_PARAMETER parameters[2]{};parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[0].DescriptorTable={2,ranges};parameters[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;parameters[1].Constants={0,0,4};parameters[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
  D3D12_ROOT_SIGNATURE_DESC desc{2,parameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
  hr=D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&serialized,&errors);release(errors);if(FAILED(hr)){release(code);return false;}
  hr=device->CreateRootSignature(0,serialized->GetBufferPointer(),serialized->GetBufferSize(),IID_PPV_ARGS(&root));release(serialized);if(FAILED(hr)){release(code);return false;}
  D3D12_COMPUTE_PIPELINE_STATE_DESC compute{};compute.pRootSignature=root;compute.CS={code->GetBufferPointer(),code->GetBufferSize()};hr=device->CreateComputePipelineState(&compute,IID_PPV_ARGS(&pipeline));release(code);if(FAILED(hr))return false;
  D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,4,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap))))return false;stride=device->GetDescriptorHandleIncrementSize(hd.Type);
  for(unsigned i=0;i<2;++i){D3D12_HEAP_PROPERTIES h{};h.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=w;d.Height=this->height;d.DepthOrArraySize=d.MipLevels=1;d.Format=i?DXGI_FORMAT_R16G16_FLOAT:DXGI_FORMAT_R32_FLOAT;d.SampleDesc.Count=1;d.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
   auto** target=i?&motion:&depth;
   if(FAILED(device->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(target))))return false;}
  return true;
 }
 void record(ID3D12Device* device,ID3D12GraphicsCommandList* cmd,ID3D12Resource* sourceDepth,ID3D12Resource* sourceMotion,UINT sourceWidth,UINT sourceHeight){
  auto cpu=heap->GetCPUDescriptorHandleForHeapStart();
  for(auto* source:{sourceDepth,sourceMotion}){D3D12_SHADER_RESOURCE_VIEW_DESC d{};d.Format=source->GetDesc().Format;d.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;d.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;d.Texture2D.MipLevels=1;device->CreateShaderResourceView(source,&d,cpu);cpu.ptr+=stride;}
  D3D12_RESOURCE_BARRIER barriers[2]{};unsigned at=0;
  for(auto* output:{depth,motion}){D3D12_UNORDERED_ACCESS_VIEW_DESC d{};d.Format=output->GetDesc().Format;d.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;device->CreateUnorderedAccessView(output,nullptr,&d,cpu);cpu.ptr+=stride;auto& b=barriers[at++];b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={output,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS};}
  cmd->ResourceBarrier(2,barriers);cmd->SetDescriptorHeaps(1,&heap);cmd->SetComputeRootSignature(root);cmd->SetPipelineState(pipeline);cmd->SetComputeRootDescriptorTable(0,heap->GetGPUDescriptorHandleForHeapStart());UINT constants[]={sourceWidth,sourceHeight,width,height};cmd->SetComputeRoot32BitConstants(1,4,constants,0);cmd->Dispatch((width+7)/8,(height+7)/8,1);
  for(auto& b:barriers)std::swap(b.Transition.StateBefore,b.Transition.StateAfter);cmd->ResourceBarrier(2,barriers);
 }
};
