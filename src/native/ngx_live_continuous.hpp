// Compact all-pixel diagnostics and timestamps. Neither readback feeds NGX.
static bool init_current_diagnostics(LiveFixture &c) {
 if(c.timestamps)return c.validation_pipeline && c.timestamp_frequency;
 auto *queue=reinterpret_cast<ID3D12CommandQueue*>(c.integration_queue->get_native());
 if(FAILED(queue->GetTimestampFrequency(&c.timestamp_frequency)) || !c.timestamp_frequency)return false;
 D3D12_QUERY_HEAP_DESC qd{};qd.Type=D3D12_QUERY_HEAP_TYPE_TIMESTAMP;qd.Count=6000;
 if(FAILED(c.resource_device->CreateQueryHeap(&qd,IID_PPV_ARGS(&c.timestamps))))return false;c.owned.keep(c.timestamps);
 c.validation=eval_texture(c.resource_device,c.owned,10,1,DXGI_FORMAT_R32_UINT,true);c.validation_zero=eval_texture(c.resource_device,c.owned,10,1,DXGI_FORMAT_R32_UINT,false);
 std::ifstream f(root/"live_validate.cso",std::ios::binary|std::ios::ate);if(!f || !c.validation || !c.validation_zero)return false;
 std::vector<char> code(static_cast<size_t>(f.tellg()));f.seekg(0);f.read(code.data(),code.size());
 D3D12_DESCRIPTOR_RANGE ranges[2]{{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,4,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,0}};
 D3D12_ROOT_PARAMETER rp[2]{};for(unsigned i=0;i<2;i++){rp[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;rp[i].DescriptorTable={1,&ranges[i]};}
 D3D12_ROOT_SIGNATURE_DESC desc{2,rp,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ID3DBlob *blob=nullptr,*error=nullptr;
 auto hr=D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error);c.owned.keep(blob);c.owned.keep(error);
 if(SUCCEEDED(hr))hr=c.proxy_device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&c.validation_signature));c.owned.keep(c.validation_signature);
 D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=c.validation_signature;pd.CS={code.data(),code.size()};if(SUCCEEDED(hr))hr=c.proxy_device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&c.validation_pipeline));c.owned.keep(c.validation_pipeline);
 return SUCCEEDED(hr);
}
static bool record_current_validation(a::command_list *cmd,LiveFixture &c,ID3D12GraphicsCommandList *proxy) {
 auto *native=reinterpret_cast<ID3D12GraphicsCommandList*>(cmd->get_native());
 if(!c.validation_initialized){if(!eval_upload(c.resource_device,c.owned,native,c.validation_zero,root/"validation-zero.bin"))return false;eval_transition(native,c.validation_zero,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);}
 else eval_transition(native,c.validation,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 eval_transition(native,c.validation,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_DEST);native->CopyResource(c.validation,c.validation_zero);eval_transition(native,c.validation,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=5;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;ID3D12DescriptorHeap *heap=nullptr;
 if(FAILED(c.proxy_device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap))))return false;c.owned.keep(heap);
 auto cpu=heap->GetCPUDescriptorHandleForHeapStart();auto gpu=heap->GetGPUDescriptorHandleForHeapStart();auto increment=c.proxy_device->GetDescriptorHandleIncrementSize(hd.Type);
 ID3D12Resource *resources[]={c.output,c.current_depth,c.motion,c.exposure};DXGI_FORMAT formats[]={DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R16G16_FLOAT,DXGI_FORMAT_R32_FLOAT};
 for(unsigned i=0;i<4;i++){D3D12_SHADER_RESOURCE_VIEW_DESC sd{};sd.Format=formats[i];sd.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sd.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sd.Texture2D.MipLevels=1;c.proxy_device->CreateShaderResourceView(resources[i],&sd,cpu);cpu.ptr+=increment;}
 D3D12_UNORDERED_ACCESS_VIEW_DESC ud{};ud.Format=DXGI_FORMAT_R32_UINT;ud.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;c.proxy_device->CreateUnorderedAccessView(c.validation,nullptr,&ud,cpu);
 internal_evaluation=true;proxy->SetComputeRootSignature(c.validation_signature);proxy->SetPipelineState(c.validation_pipeline);proxy->SetDescriptorHeaps(1,&heap);proxy->SetComputeRootDescriptorTable(0,gpu);gpu.ptr+=4*increment;proxy->SetComputeRootDescriptorTable(1,gpu);proxy->Dispatch(240,135,1);internal_evaluation=false;
 D3D12_RESOURCE_BARRIER order{};order.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;order.UAV.pResource=c.validation;native->ResourceBarrier(1,&order);eval_transition(native,c.validation,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);c.validation_initialized=true;
 auto path=root/label/(std::to_string(frame)+"-validation.bin");auto read=dense_readback(c.resource_device,c.owned,native,c.validation,path);if(!read.buffer)return false;read.buffer->AddRef();copies.push_back({read.buffer,cmd,nullptr,read.total,read.rows,read.fp.Footprint.RowPitch,static_cast<uint32_t>(read.rowbytes),path});
 return true;
}
static void record_current_timestamps(a::command_list *cmd,LiveFixture &c) {
 auto *native=reinterpret_cast<ID3D12GraphicsCommandList*>(cmd->get_native());auto *read=eval_buffer(c.resource_device,c.owned,80,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);if(!read)return;
 native->ResolveQueryData(c.timestamps,D3D12_QUERY_TYPE_TIMESTAMP,c.evaluated*10,10,read,0);read->AddRef();copies.push_back({read,cmd,nullptr,80,1,80,80,root/label/(std::to_string(frame)+"-timestamps.bin")});
}
