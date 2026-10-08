#pragma once
// Private, bounded output trial. This does not select an installed provider.
template<class Capture>
static bool fsr_probe_native_reset(Capture &c,a::command_list *cmd,const Cmd &saved,
 const Slot &pass,const std::array<float,84> &data,uint32_t x,uint32_t y,uint32_t z){
 if(!c.history_dirty)return false;
 auto &cache=c.borrows;
 if(!c.resource_device || cache.blocked || cache.entries.size()>=256
  ||saved.cp.find(pass.param)==saved.cp.end())return false;
 if(!cache.fence && FAILED(c.resource_device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&cache.fence))))return false;
 EvalOwned owned;auto *buffer=eval_buffer(c.resource_device,owned,512,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
 if(!buffer)return false;
 void *mapped=nullptr;D3D12_RANGE empty{0,0};
 if(FAILED(buffer->Map(0,&empty,&mapped))){buffer->Release();owned.resources.clear();return false;}
 memset(mapped,0,512);memcpy(mapped,data.data(),sizeof(data));uint32_t reset=1;
 memcpy(static_cast<char*>(mapped)+48,&reset,sizeof(reset));buffer->Unmap(0,nullptr);
 LeanBorrowKey key{reinterpret_cast<uint64_t>(buffer),0,UINT_MAX,0,0,0,0,0};
 LeanBorrowEntry entry;entry.resources[0]=buffer;cache.entries.emplace(key,entry);owned.resources.clear();
 cache.record(cmd,key,frame);cache.peak_entries=std::max<uint64_t>(cache.peak_entries,cache.entries.size());
 a::buffer_range range{{reinterpret_cast<uint64_t>(buffer)},0,512};a::descriptor_table_update update{};
 update.binding=pass.rangebinding;update.count=1;update.type=a::descriptor_type::constant_buffer;update.descriptors=&range;
 cmd->push_descriptors(a::shader_stage::all_compute,{saved.compute_layout},pass.param,update);
 reinterpret_cast<ID3D12GraphicsCommandList*>(cmd->get_native())->Dispatch(x,y,z);
 restore_root(cmd,saved,false,true);restore_root(cmd,saved,true,true);
 cmd->bind_pipeline(a::pipeline_stage::all_compute,{saved.pipeline});commands[cmd]=saved;
 c.history_dirty=false;
 c.log<<"{\"stage\":\"fsr_native_history_reset\",\"presentSequence\":"<<frame
  <<",\"passByteOffset\":48,\"gameBufferModified\":false,\"NGXContextPresent\":false}\n";c.log.flush();
 return true;
}
template<class Capture>
static bool fsr_probe_output(Capture &c,ID3D12GraphicsCommandList *native,ID3D12Resource *output){
 const auto source=c.fsr.output->GetDesc(),target=output->GetDesc();
 if(source.Width!=target.Width||source.Height!=target.Height||source.Format!=target.Format
  ||source.DepthOrArraySize!=target.DepthOrArraySize||source.MipLevels!=target.MipLevels
  ||source.SampleDesc.Count!=target.SampleDesc.Count||source.SampleDesc.Quality!=target.SampleDesc.Quality
  ||source.Dimension!=target.Dimension||c.fsr.output==output)return false;
 eval_transition(native,c.fsr.output,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
 eval_transition(native,output,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_DEST);
 native->CopyResource(output,c.fsr.output);
 eval_transition(native,output,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 eval_transition(native,c.fsr.output,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 c.history_dirty=true;return true;
}
