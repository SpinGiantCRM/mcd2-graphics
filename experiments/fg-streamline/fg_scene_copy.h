#pragma once
// The source backbuffer is borrowed only for this recording. An owned copy
// supplies the shader SRV, so DXGI_SHADER_INPUT is not required of the game.
namespace fg_scene {
struct Entry {ID3D12Resource*stage=nullptr,*image=nullptr;ID3D12DescriptorHeap*heap=nullptr;bool initialized=false;};
inline std::mutex mutex;
inline std::map<uint64_t,Entry> entries;
inline std::vector<IUnknown*> owned;
inline ID3D12Device*device=nullptr;
inline ID3D12RootSignature*signature=nullptr;
inline ID3D12PipelineState*pipeline=nullptr;
inline ID3D12CommandQueue*queue=nullptr;
inline ID3D12Fence*fence=nullptr;
inline uint64_t fenceValue=0;
inline HRESULT error=S_OK;
inline bool recorded=false;
template<class T>inline T*keep(T*p){if(p)owned.push_back(p);return p;}
inline bool initialize(ID3D12GraphicsCommandList*cmd,DXGI_FORMAT inputFormat,DXGI_FORMAT outputFormat){
 ID3D12Device*current=nullptr;if(FAILED(cmd->GetDevice(IID_PPV_ARGS(&current))))return false;
 if(device){const bool same=current==device;current->Release();return same&&pipeline;}
 device=current;device->Release(); // Borrow the framework wrapper; never retain it.
 D3D12_FEATURE_DATA_FORMAT_SUPPORT input{inputFormat},output{outputFormat};
 if(FAILED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,&input,sizeof(input)))||
    FAILED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,&output,sizeof(output)))||
    !(input.Support1&D3D12_FORMAT_SUPPORT1_SHADER_LOAD)||!(output.Support2&D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE))return false;
 auto exe=std::make_unique<wchar_t[]>(32768);auto n=GetModuleFileNameW(nullptr,exe.get(),32768);if(!n||n>=32768)return false;
 std::ifstream file(std::filesystem::path(exe.get()).parent_path()/L"FG_SCENE_COPY.cso",std::ios::binary|std::ios::ate);
 if(!file||file.tellg()<=0||file.tellg()>1024*1024)return false;
 std::vector<char>code(size_t(file.tellg()));file.seekg(0);if(!file.read(code.data(),code.size()))return false;
 D3D12_DESCRIPTOR_RANGE ranges[2]{{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,0}};
 D3D12_ROOT_PARAMETER params[2]{};for(unsigned i=0;i<2;++i){params[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[i].DescriptorTable={1,&ranges[i]};}
 D3D12_ROOT_SIGNATURE_DESC desc{2,params,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ID3DBlob*blob=nullptr,*messages=nullptr;
 error=D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&messages);keep(blob);keep(messages);if(FAILED(error))return false;
 error=device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&signature));keep(signature);if(FAILED(error))return false;
 D3D12_COMPUTE_PIPELINE_STATE_DESC ps{};ps.pRootSignature=signature;ps.CS={code.data(),code.size()};
 error=device->CreateComputePipelineState(&ps,IID_PPV_ARGS(&pipeline));keep(pipeline);return SUCCEEDED(error);
}
inline ID3D12Resource*copy(ID3D12GraphicsCommandList*cmd,ID3D12Resource*backbuffer,unsigned width,unsigned height){
 if(mcd2::process_exit::terminating())return nullptr;
 std::lock_guard lock(mutex);if(!cmd||!backbuffer)return nullptr;auto d=backbuffer->GetDesc();
 if(d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||d.Width!=width||d.Height!=height||d.DepthOrArraySize!=1||d.MipLevels!=1||d.SampleDesc.Count!=1)return nullptr;
 const auto format=d.Format;const auto target=format==DXGI_FORMAT_B8G8R8A8_UNORM?DXGI_FORMAT_R8G8B8A8_UNORM:format;
 if((format!=DXGI_FORMAT_R10G10B10A2_UNORM&&format!=DXGI_FORMAT_R8G8B8A8_UNORM&&format!=DXGI_FORMAT_B8G8R8A8_UNORM)||!initialize(cmd,format,target))return nullptr;
 const auto key=reinterpret_cast<uint64_t>(backbuffer);auto it=entries.find(key);
 if(it==entries.end()){
  if(entries.size()>=16)return nullptr;Entry e{};D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;rd.Width=width;rd.Height=height;rd.DepthOrArraySize=rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Format=format;
  error=device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&e.stage));keep(e.stage);if(FAILED(error))return nullptr;
  rd.Format=target;rd.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  error=device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&e.image));keep(e.image);if(FAILED(error))return nullptr;
  D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,2,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
  error=device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&e.heap));keep(e.heap);if(FAILED(error))return nullptr;
  auto cpu=e.heap->GetCPUDescriptorHandleForHeapStart();const auto increment=device->GetDescriptorHandleIncrementSize(hd.Type);
  D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=format;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;device->CreateShaderResourceView(e.stage,&srv,cpu);cpu.ptr+=increment;
  D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=target;uav.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;device->CreateUnorderedAccessView(e.image,nullptr,&uav,cpu);it=entries.emplace(key,e).first;
 }
 auto&e=it->second;const auto staged=e.stage->GetDesc();if(staged.Width!=width||staged.Height!=height||staged.Format!=format)return nullptr;
 recorded=true;
 constexpr auto read=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
 if(e.initialized){fg_alpha::transition(cmd,e.stage,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);fg_alpha::transition(cmd,e.image,read,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);}
 fg_alpha::transition(cmd,backbuffer,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);cmd->CopyResource(e.stage,backbuffer);fg_alpha::transition(cmd,backbuffer,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
 fg_alpha::transition(cmd,e.stage,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 auto gpu=e.heap->GetGPUDescriptorHandleForHeapStart();auto increment=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
 cmd->SetComputeRootSignature(signature);cmd->SetPipelineState(pipeline);cmd->SetDescriptorHeaps(1,&e.heap);cmd->SetComputeRootDescriptorTable(0,gpu);gpu.ptr+=increment;cmd->SetComputeRootDescriptorTable(1,gpu);cmd->Dispatch((width+7)/8,(height+7)/8,1);
 D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;b.UAV.pResource=e.image;cmd->ResourceBarrier(1,&b);fg_alpha::transition(cmd,e.image,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,read);e.initialized=true;return e.image;
}
inline bool submission_queue_native(ID3D12CommandQueue*actual){
 if(mcd2::process_exit::terminating())return false;std::lock_guard lock(mutex);
 if(!actual)return false;if(queue&&queue!=actual)return false;if(!queue){queue=actual;queue->AddRef();keep(queue);}return true;
}
inline bool submission_queue(a::command_queue*q){return q&&submission_queue_native(reinterpret_cast<ID3D12CommandQueue*>(q->get_native()));}
inline bool cleanup(){
 if(mcd2::process_exit::terminating())return false;std::lock_guard lock(mutex);
 if(owned.empty()){device=nullptr;return true;}
 if(recorded){
  if(!queue)return false;
  if(!fence){ID3D12Device*raw=nullptr;auto hr=queue->GetDevice(IID_PPV_ARGS(&raw));if(SUCCEEDED(hr))hr=raw->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence));if(raw)raw->Release();keep(fence);if(FAILED(hr))return false;}
  const auto value=++fenceValue;if(FAILED(queue->Signal(fence,value)))return false;auto event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event)return false;
  auto hr=fence->SetEventOnCompletion(value,event);const bool complete=SUCCEEDED(hr)&&WaitForSingleObject(event,5000)==WAIT_OBJECT_0&&fence->GetCompletedValue()!=UINT64_MAX;CloseHandle(event);if(!complete)return false;
 }
 entries.clear();for(auto i=owned.rbegin();i!=owned.rend();++i)(*i)->Release();owned.clear();device=nullptr;signature=nullptr;pipeline=nullptr;queue=nullptr;fence=nullptr;error=S_OK;recorded=false;return true;
}
} // namespace fg_scene
