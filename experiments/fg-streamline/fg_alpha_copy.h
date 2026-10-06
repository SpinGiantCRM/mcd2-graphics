#pragma once
#include <d3dcompiler.h>
#include <vector>
// Private bounded experiment. No game's input texture or final image is written.
namespace fg_alpha {
struct ProxyLayout : ID3D12GraphicsCommandList, a::command_list {};
inline ID3D12GraphicsCommandList* proxy(a::command_list*cmd){
 constexpr GUID proxyId={0x479b29e3,0x9a2c,0x11d0,{0xb6,0x96,0x00,0xa0,0xc9,0x03,0x48,0x7a}},unwrapId={0x7f2c9a11,0x3b4e,0x4d6a,{0x81,0x2f,0x5e,0x9c,0xd3,0x7a,0x1b,0x42}};
 auto candidate=static_cast<ID3D12GraphicsCommandList*>(static_cast<ProxyLayout*>(cmd));ID3D12GraphicsCommandList*p=nullptr,*original=nullptr;
 if(FAILED(candidate->QueryInterface(proxyId,reinterpret_cast<void**>(&p)))||!p)return nullptr;
 bool valid=SUCCEEDED(p->QueryInterface(unwrapId,reinterpret_cast<void**>(&original)))&&original&&reinterpret_cast<uint64_t>(original)==cmd->get_native();if(original)original->Release();if(!valid){p->Release();return nullptr;}return p;
}
struct Entry{ID3D12Resource*input=nullptr,*alpha=nullptr;ID3D12DescriptorHeap*heap=nullptr;bool initialized=false;};
inline std::mutex guard;
inline std::map<uint64_t,Entry> entries;
inline std::vector<IUnknown*> owned;
inline ID3D12Device*device=nullptr;
inline ID3D12RootSignature*signature=nullptr;
inline ID3D12PipelineState*pipeline=nullptr;
inline ID3D12CommandQueue*queue=nullptr;
inline ID3D12Fence*fence=nullptr;
inline uint64_t fenceValue=0;
inline bool failed=false;
inline std::atomic<unsigned> stage{0};inline std::atomic<HRESULT> lastHR{S_OK};
template<class T>inline T*keep(T*p){if(p)owned.push_back(p);return p;}
inline void transition(ID3D12GraphicsCommandList*cmd,ID3D12Resource*r,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};cmd->ResourceBarrier(1,&b);}
inline bool initialize(ID3D12GraphicsCommandList*cmd){
 if(device){ID3D12Device*other=nullptr;bool same=SUCCEEDED(cmd->GetDevice(IID_PPV_ARGS(&other)))&&other==device;if(other)other->Release();if(!same){stage=2;lastHR=E_INVALIDARG;}return same&&!failed;}
 // The framework owns this wrapper for the lifetime of command callbacks.
 // Retaining it would prevent destroy_device / AddonUninit from ever running.
 stage=1;lastHR=cmd->GetDevice(IID_PPV_ARGS(&device));if(FAILED(lastHR))return false;device->Release();
 stage=3;auto exe=std::make_unique<wchar_t[]>(32768);auto length=GetModuleFileNameW(nullptr,exe.get(),32768);if(!length||length>=32768){lastHR=E_FAIL;return false;}
 std::ifstream file(std::filesystem::path(exe.get()).parent_path()/L"FG_UI_ALPHA.cso",std::ios::binary|std::ios::ate);if(!file||file.tellg()<=0||file.tellg()>1024*1024){lastHR=E_FAIL;return false;}
 std::vector<char>shader(size_t(file.tellg()));file.seekg(0);file.read(shader.data(),shader.size());if(!file){lastHR=E_FAIL;return false;}
 D3D12_DESCRIPTOR_RANGE ranges[2]{{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,0}};
 D3D12_ROOT_PARAMETER params[2]{};for(unsigned i=0;i<2;++i){params[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[i].DescriptorTable={1,&ranges[i]};}
 D3D12_ROOT_SIGNATURE_DESC desc{2,params,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ID3DBlob*blob=nullptr,*error=nullptr;auto hr=D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error);keep(blob);keep(error);
 stage=4;if(SUCCEEDED(hr))hr=device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&signature));keep(signature);lastHR=hr;if(FAILED(hr))return false;
 stage=5;D3D12_COMPUTE_PIPELINE_STATE_DESC ps{};ps.pRootSignature=signature;ps.CS={shader.data(),shader.size()};hr=device->CreateComputePipelineState(&ps,IID_PPV_ARGS(&pipeline));keep(pipeline);lastHR=hr;return SUCCEEDED(hr);
}
inline ID3D12Resource*copy(ID3D12GraphicsCommandList*cmd,ID3D12Resource*ui,DXGI_FORMAT viewFormat,unsigned width,unsigned height){
 std::lock_guard lock(guard);if(failed||!ui||!initialize(cmd)){failed=true;return nullptr;}
 stage=6;auto d=ui->GetDesc();if(d.Width!=width||d.Height!=height||d.DepthOrArraySize!=1||d.SampleDesc.Count!=1||(viewFormat!=DXGI_FORMAT_B8G8R8A8_UNORM&&viewFormat!=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)){lastHR=E_INVALIDARG;return nullptr;}
 const auto key=reinterpret_cast<uint64_t>(ui);auto hit=entries.find(key);
 if(hit==entries.end()){
  if(entries.size()>=16)return nullptr;Entry e{};e.input=ui;ui->AddRef();keep(ui);
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;rd.Width=width;rd.Height=height;rd.DepthOrArraySize=rd.MipLevels=1;rd.Format=DXGI_FORMAT_R8_UNORM;rd.SampleDesc.Count=1;rd.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  stage=7;auto hr=device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&e.alpha));keep(e.alpha);lastHR=hr;if(FAILED(hr))return nullptr;
  stage=8;D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,2,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};hr=device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&e.heap));keep(e.heap);lastHR=hr;if(FAILED(hr))return nullptr;
  auto cpu=e.heap->GetCPUDescriptorHandleForHeapStart();auto inc=device->GetDescriptorHandleIncrementSize(hd.Type);D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=viewFormat;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;device->CreateShaderResourceView(ui,&srv,cpu);cpu.ptr+=inc;
  D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=DXGI_FORMAT_R8_UNORM;uav.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;device->CreateUnorderedAccessView(e.alpha,nullptr,&uav,cpu);hit=entries.emplace(key,e).first;
 }
 auto&e=hit->second;if(e.initialized)transition(cmd,e.alpha,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 auto gpu=e.heap->GetGPUDescriptorHandleForHeapStart();auto inc=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
 cmd->SetComputeRootSignature(signature);cmd->SetPipelineState(pipeline);cmd->SetDescriptorHeaps(1,&e.heap);cmd->SetComputeRootDescriptorTable(0,gpu);gpu.ptr+=inc;cmd->SetComputeRootDescriptorTable(1,gpu);cmd->Dispatch((width+7)/8,(height+7)/8,1);
 D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;b.UAV.pResource=e.alpha;cmd->ResourceBarrier(1,&b);transition(cmd,e.alpha,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);e.initialized=true;return e.alpha;
}
inline bool submission_queue(a::command_queue*q){
 std::lock_guard lock(guard);auto actual=reinterpret_cast<ID3D12CommandQueue*>(q->get_native());if(queue&&queue!=actual){failed=true;return false;}
 if(!queue){queue=actual;queue->AddRef();keep(queue);}return !failed;
}
inline bool cleanup(){
 std::lock_guard lock(guard);if(owned.empty())return true;
 // Teardown is bounded and retains references if GPU completion is unproven.
 if(!queue)return false;
 if(!fence){ID3D12Device*raw=nullptr;auto hr=queue->GetDevice(IID_PPV_ARGS(&raw));if(SUCCEEDED(hr))hr=raw->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence));if(raw)raw->Release();keep(fence);if(FAILED(hr))return false;}
 auto value=++fenceValue;if(FAILED(queue->Signal(fence,value)))return false;auto event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event)return false;
 auto hr=fence->SetEventOnCompletion(value,event);bool complete=SUCCEEDED(hr)&&WaitForSingleObject(event,5000)==WAIT_OBJECT_0&&fence->GetCompletedValue()!=UINT64_MAX;CloseHandle(event);if(!complete)return false;
 entries.clear();for(auto it=owned.rbegin();it!=owned.rend();++it)(*it)->Release();owned.clear();device=nullptr;signature=nullptr;pipeline=nullptr;queue=nullptr;fence=nullptr;failed=false;return true;
}
}
