// Bounded current-input test. Original AA remains displayed; GPU readbacks are
// diagnostic copies and are never inputs to NGX evaluation.
static bool live_current_gate(a::command_list *cmd,uint32_t shader,uint32_t x,uint32_t y,uint32_t z) {
 if(!live_fixture || !live_fixture->ready)return false;
 auto &c=*live_fixture;c.log<<"{\"stage\":\"current_gate_begin\",\"frame\":"<<frame<<"}\n";const Cmd saved=commands[cmd];auto slots=resolve(cmd,true);
 auto fail=[&](const char *reason){c.ready=false;c.log<<"{\"stage\":\"current_gate_failure\",\"reason\":\""<<reason<<"\"}\n";records.push_back("{\"kind\":\"error\",\"message\":\""+std::string(reason)+"\"}");return false;};
 if(current_queue_mismatch)return fail("current_integration_queue_changed");
 if(shader!=TAA_SR || c.width!=2560 || c.height!=1440 || !saved.pipeline || !saved.compute_layout || saved.dynamic_offsets || !root_push_supported(saved.cp) || !root_push_supported(saved.gp))return fail("current_restore_contract");
 const Slot *colour=nullptr,*depth=nullptr,*packed=nullptr,*exposure=nullptr,*pass=nullptr,*view=nullptr,*out=nullptr;
 for(const auto &s:slots)if(s.space==0){
  if(s.binding.type==a::descriptor_type::shader_resource_view){if(s.reg==1)colour=&s;if(s.reg==2)depth=&s;if(s.reg==3)packed=&s;}
  if(s.binding.type==a::descriptor_type::buffer_shader_resource_view && s.reg==0)exposure=&s;
  if(s.binding.type==a::descriptor_type::constant_buffer){if(s.reg==0)pass=&s;if(s.reg==1)view=&s;}
  if(s.binding.type==a::descriptor_type::unordered_access_view && s.reg==0)out=&s;
 }
 if(!colour || !depth || !packed || !exposure || !pass || !view || !out)return fail("current_required_binding_missing");
 auto resource=[&](const Slot *s){return reinterpret_cast<ID3D12Resource*>(s->binding.type==a::descriptor_type::constant_buffer?s->binding.cb.buffer.handle:from_view(cmd->get_device(),s->binding.view).handle);};
 auto *game_colour=resource(colour),*game_depth=resource(depth),*game_packed=resource(packed),*game_exp=resource(exposure),*game_pass=resource(pass),*game_view=resource(view),*game_out=resource(out);
 c.log<<"{\"stage\":\"current_bindings_resolved\"}\n";
 for(auto *r:{game_colour,game_depth,game_packed,game_exp,game_pass,game_view,game_out})if(!r || live_resources.find(reinterpret_cast<uint64_t>(r))==live_resources.end())return fail("current_resource_missing_from_live_cache");
 D3D12_RESOURCE_STATES states[3]{};const Slot *input_slots[]={colour,depth,packed};ID3D12Resource *input_resources[]={game_colour,game_depth,game_packed};
 for(unsigned i=0;i<3;i++){
  auto d=input_resources[i]->GetDesc();auto vd=view_desc(cmd->get_device(),input_slots[i]->binding.view);
  if(d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D || d.Width!=c.width || d.Height!=c.height || d.SampleDesc.Count!=1 || d.DepthOrArraySize!=1 || vd.texture.first_level || vd.texture.first_layer)return fail("current_texture_contract");
  auto it=saved.states.find(reinterpret_cast<uint64_t>(input_resources[i]));
  if(it!=saved.states.end())states[i]=native_state(it->second);else{
   auto prior=submitted_states.find(reinterpret_cast<uint64_t>(input_resources[i]));if(prior==submitted_states.end())return fail("current_source_state_unknown");states[i]=native_state(prior->second);
  }
 }
 auto od=game_out->GetDesc();if(od.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT || od.Width!=3840 || od.Height!=2160 || od.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D || od.MipLevels!=1 || od.DepthOrArraySize!=1 || od.SampleDesc.Count!=1 || game_out==game_colour || game_out==game_depth || game_out==game_packed)return fail("current_output_contract");
 c.log<<"{\"stage\":\"current_texture_states_checked\"}\n";
 if(game_colour->GetDesc().Format!=DXGI_FORMAT_R11G11B10_FLOAT || static_cast<uint32_t>(view_desc(cmd->get_device(),packed->binding.view).format)!=11 || !(states[1]&D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE))return fail("current_input_format_or_depth_state");
 const Slot *cb_slots[]={pass,view};ID3D12Resource *cb_resources[]={game_pass,game_view};std::array<std::vector<float>,2> cb;
 for(unsigned i=0;i<2;i++){
  const uint64_t bytes=i?2528:336,offset=cb_slots[i]->binding.cb.offset;D3D12_HEAP_PROPERTIES hp{};D3D12_HEAP_FLAGS flags{};
  if(FAILED(cb_resources[i]->GetHeapProperties(&hp,&flags)) || hp.Type!=D3D12_HEAP_TYPE_UPLOAD || offset+bytes>cb_resources[i]->GetDesc().Width || (cb_slots[i]->binding.cb.size!=UINT64_MAX && cb_slots[i]->binding.cb.size<bytes))return fail("current_upload_constants_contract");
  void *data=nullptr;D3D12_RANGE range{static_cast<SIZE_T>(offset),static_cast<SIZE_T>(offset+bytes)};if(FAILED(cb_resources[i]->Map(0,&range,&data)))return fail("current_upload_constants_map");
  cb[i].resize(bytes/4);std::memcpy(cb[i].data(),static_cast<char*>(data)+offset,bytes);D3D12_RANGE empty{0,0};cb_resources[i]->Unmap(0,&empty);
 }
 c.log<<"{\"stage\":\"current_constants_read\"}\n";
 if(cb[0][36]!=c.width || cb[0][37]!=c.height || cb[0][44]!=3840 || cb[0][45]!=2160 || !std::isfinite(cb[1][626]) || cb[1][626]<=0 || !std::isfinite(cb[1][576]) || !std::isfinite(cb[1][577]))return fail("current_dimensions_or_constants");
 auto exp_view=view_desc(cmd->get_device(),exposure->binding.view);if(exp_view.buffer.offset+16>game_exp->GetDesc().Width)return fail("current_exposure_range");
 auto *proxy=validated_game_proxy(cmd);if(!proxy)return fail("current_proxy_identity");
 ID3D12Device *proxy_device=nullptr;const bool device_matches=SUCCEEDED(proxy->GetDevice(IID_PPV_ARGS(&proxy_device))) && proxy_device==c.proxy_device;if(proxy_device)proxy_device->Release();
 if(!device_matches || proxy->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT){proxy->Release();return fail("current_proxy_device");}
 c.log<<"{\"stage\":\"current_proxy_checked\"}\n";
 if(!c.current_signature){
  c.current_packed=eval_texture(c.resource_device,c.owned,c.width,c.height,DXGI_FORMAT_R16G16B16A16_UNORM,false);c.current_depth=eval_texture(c.resource_device,c.owned,c.width,c.height,DXGI_FORMAT_R32_FLOAT,true);
  std::ifstream shaderfile(root/"live_dense.cso",std::ios::binary|std::ios::ate);if(!c.current_packed || !c.current_depth || !shaderfile){proxy->Release();return fail("current_converter_resources");}
  std::vector<char> code(static_cast<size_t>(shaderfile.tellg()));shaderfile.seekg(0);shaderfile.read(code.data(),code.size());
  D3D12_DESCRIPTOR_RANGE ranges[2]{{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,2,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,3,0,0,0}};
  D3D12_ROOT_PARAMETER rp[5]{};rp[0].ParameterType=rp[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;rp[0].Descriptor={0,0};rp[1].Descriptor={1,0};rp[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;rp[2].Descriptor={2,0};rp[3].ParameterType=rp[4].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;rp[3].DescriptorTable={1,&ranges[0]};rp[4].DescriptorTable={1,&ranges[1]};
  D3D12_ROOT_SIGNATURE_DESC rd{5,rp,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ID3DBlob *blob=nullptr,*errors=nullptr;auto hr=D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&errors);c.owned.keep(blob);c.owned.keep(errors);
  if(SUCCEEDED(hr))hr=c.proxy_device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&c.current_signature));c.owned.keep(c.current_signature);
  D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=c.current_signature;pd.CS={code.data(),code.size()};if(SUCCEEDED(hr))hr=c.proxy_device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&c.current_pipeline));c.owned.keep(c.current_pipeline);
  if(FAILED(hr)){proxy->Release();return fail("current_converter_pipeline");}
 }
 c.log<<"{\"stage\":\"current_converter_ready\"}\n";
 if(live_continuous_mode && !init_current_diagnostics(c)){proxy->Release();return fail("current_diagnostics_initialization");}
 auto stamp=[&](unsigned offset){if(live_continuous_mode)reinterpret_cast<ID3D12GraphicsCommandList*>(cmd->get_native())->EndQuery(c.timestamps,D3D12_QUERY_TYPE_TIMESTAMP,c.evaluated*10+offset);};
 // A distinct heap per recorded frame avoids rewriting descriptors still in flight.
 D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=5;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;ID3D12DescriptorHeap *heap=nullptr;
 if(FAILED(c.proxy_device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap)))){proxy->Release();return fail("current_converter_heap");}c.owned.keep(heap);
 auto cpu=heap->GetCPUDescriptorHandleForHeapStart();auto gpu=heap->GetGPUDescriptorHandleForHeapStart();const auto increment=c.proxy_device->GetDescriptorHandleIncrementSize(hd.Type);
 D3D12_SHADER_RESOURCE_VIEW_DESC sd{};sd.Format=DXGI_FORMAT_R16G16B16A16_UNORM;sd.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sd.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sd.Texture2D.MipLevels=1;c.proxy_device->CreateShaderResourceView(c.current_packed,&sd,cpu);cpu.ptr+=increment;
 sd.Format=static_cast<DXGI_FORMAT>(view_desc(cmd->get_device(),depth->binding.view).format);c.proxy_device->CreateShaderResourceView(game_depth,&sd,cpu);cpu.ptr+=increment;
 c.log<<"{\"stage\":\"current_converter_SRVs_ready\"}\n";
 D3D12_UNORDERED_ACCESS_VIEW_DESC ud{};ud.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;ud.Format=DXGI_FORMAT_R16G16_FLOAT;c.proxy_device->CreateUnorderedAccessView(c.motion,nullptr,&ud,cpu);cpu.ptr+=increment;ud.Format=DXGI_FORMAT_R32_FLOAT;c.proxy_device->CreateUnorderedAccessView(c.exposure,nullptr,&ud,cpu);cpu.ptr+=increment;c.proxy_device->CreateUnorderedAccessView(c.current_depth,nullptr,&ud,cpu);
 auto *native=reinterpret_cast<ID3D12GraphicsCommandList*>(cmd->get_native());stamp(0);native->Dispatch(x,y,z);stamp(1);commands[cmd].states[reinterpret_cast<uint64_t>(game_out)]=a::resource_usage::unordered_access;
 D3D12_RESOURCE_BARRIER order{};order.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;order.UAV.pResource=game_out;native->ResourceBarrier(1,&order);if(!live_continuous_mode)snapshot(cmd,*out,shader,true,"before-current");stamp(2);
 ID3D12Resource *sources[]={game_colour,game_packed},*destinations[]={c.colour,c.current_packed};
 for(unsigned i=0;i<2;i++){
  const auto source_state=states[i?2:0];if(!(source_state&D3D12_RESOURCE_STATE_COPY_SOURCE))eval_transition(native,sources[i],source_state,D3D12_RESOURCE_STATE_COPY_SOURCE);
  if(i==0 || c.current_resources_initialized)eval_transition(native,destinations[i],D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
  D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=sources[i];src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;dst.pResource=destinations[i];dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;native->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
  eval_transition(native,destinations[i],D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);if(!(source_state&D3D12_RESOURCE_STATE_COPY_SOURCE))eval_transition(native,sources[i],D3D12_RESOURCE_STATE_COPY_SOURCE,source_state);
 }
 stamp(3);stamp(4);eval_transition(native,c.motion,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);eval_transition(native,c.exposure,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);if(c.current_resources_initialized)eval_transition(native,c.current_depth,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 // Heap objects and GPU descriptor handles are proxied by ReShade; route
 // their binds through the wrapper so they are translated before native use.
 internal_evaluation=true;proxy->SetComputeRootSignature(c.current_signature);proxy->SetPipelineState(c.current_pipeline);proxy->SetDescriptorHeaps(1,&heap);proxy->SetComputeRootConstantBufferView(0,game_pass->GetGPUVirtualAddress()+pass->binding.cb.offset);proxy->SetComputeRootConstantBufferView(1,game_view->GetGPUVirtualAddress()+view->binding.cb.offset);proxy->SetComputeRootShaderResourceView(2,game_exp->GetGPUVirtualAddress()+exp_view.buffer.offset);proxy->SetComputeRootDescriptorTable(3,gpu);gpu.ptr+=2*increment;proxy->SetComputeRootDescriptorTable(4,gpu);proxy->Dispatch((c.width+7)/8,(c.height+7)/8,1);internal_evaluation=false;
 for(auto *r:{c.motion,c.exposure,c.current_depth}){order.UAV.pResource=r;native->ResourceBarrier(1,&order);eval_transition(native,r,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);}c.current_resources_initialized=true;stamp(5);
 c.log<<"{\"stage\":\"current_conversion_recorded\"}\n";
 // Restore the cache before calling the wrapped NGX entry point.
 restore_root(cmd,saved,false,true);restore_root(cmd,saved,true,true);cmd->bind_pipeline(a::pipeline_stage::all_compute,{saved.pipeline});commands[cmd]=saved;
 auto table=*reinterpret_cast<void***>(c.params);auto resource_param=reinterpret_cast<void(*)(NVSDK_NGX_Parameter*,const char*,ID3D12Resource*)>(table[1]);auto fp=reinterpret_cast<void(*)(NVSDK_NGX_Parameter*,const char*,float)>(table[6]);auto ip=reinterpret_cast<void(*)(NVSDK_NGX_Parameter*,const char*,int)>(table[3]);resource_param(c.params,NVSDK_NGX_Parameter_Depth,c.current_depth);
 fp(c.params,NVSDK_NGX_Parameter_Jitter_Offset_X,cb[1][576]*c.width*.5f);fp(c.params,NVSDK_NGX_Parameter_Jitter_Offset_Y,-cb[1][577]*c.height*.5f);fp(c.params,NVSDK_NGX_Parameter_DLSS_Pre_Exposure,cb[1][626]);fp(c.params,NVSDK_NGX_Parameter_FrameTimeDeltaInMsec,static_cast<float>(frame_delta_ms));bool explicit_reset=developer_file_exists(root/"reset-current.txt");if(explicit_reset)fs::remove(root/"reset-current.txt");bool reset=!live_continuous_mode || c.evaluated==0 || c.reset_pending || explicit_reset;ip(c.params,NVSDK_NGX_Parameter_Reset,reset?1:0);
 eval_transition(native,c.output,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_DEST);native->CopyResource(c.output,c.sentinel);eval_transition(native,c.output,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 using Evaluate=NVSDK_NGX_Result(*)(ID3D12GraphicsCommandList*,const NVSDK_NGX_Handle*,const NVSDK_NGX_Parameter*,void*);auto evaluate=live_export<Evaluate>("NVSDK_NGX_D3D12_EvaluateFeature");
 c.log<<"{\"stage\":\"current_evaluate_begin\",\"frame\":"<<frame<<",\"index\":"<<c.evaluated<<",\"reset\":"<<(reset?"true":"false")<<",\"explicitReset\":"<<(explicit_reset?"true":"false")<<",\"preExposure\":"<<cb[1][626]<<",\"jitterPixels\":["<<cb[1][576]*c.width*.5f<<','<<-cb[1][577]*c.height*.5f<<"],\"frameDeltaMs\":"<<frame_delta_ms<<"}\n";
 stamp(6);internal_evaluation=true;auto result=evaluate(proxy,c.feature,c.params,nullptr);internal_evaluation=false;stamp(7);
 c.log<<"{\"stage\":\"current_evaluate\",\"frame\":"<<frame<<",\"result\":"<<static_cast<uint32_t>(result)<<"}\n";
 restore_root(cmd,saved,false,true);restore_root(cmd,saved,true,true);cmd->bind_pipeline(a::pipeline_stage::all_compute,{saved.pipeline});commands[cmd]=saved;
 if(!live_continuous_mode){native->Dispatch(x,y,z);order.UAV.pResource=game_out;native->ResourceBarrier(1,&order);snapshot(cmd,*out,shader,true,"after-current");}
 bool display=live_display_mode && c.evaluated>=8 && !developer_file_exists(root/"native-fallback.txt") && !NVSDK_NGX_FAILED(result);
 stamp(8);if(display){eval_transition(native,c.output,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);eval_transition(native,game_out,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_DEST);native->CopyResource(game_out,c.output);eval_transition(native,game_out,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);eval_transition(native,c.output,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);}stamp(9);
 if(live_continuous_mode)record_current_timestamps(cmd,c);
 order.UAV.pResource=c.output;native->ResourceBarrier(1,&order);eval_transition(native,c.output,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 if(!live_continuous_mode)for(auto pair:{std::pair{c.colour,"colour"},std::pair{c.current_depth,"depth"},std::pair{c.motion,"motion"},std::pair{c.exposure,"exposure"},std::pair{c.output,"output"}}){
  const auto path=root/label/(std::to_string(frame)+"-current-"+pair.second+".bin");auto read=dense_readback(c.resource_device,c.owned,native,pair.first,path);if(read.buffer){read.buffer->AddRef();copies.push_back({read.buffer,cmd,nullptr,read.total,read.rows,read.fp.Footprint.RowPitch,static_cast<uint32_t>(read.rowbytes),path});}
 }
 if(live_continuous_mode){if(!record_current_validation(cmd,c,proxy))records.push_back("{\"kind\":\"error\",\"message\":\"current_GPU_validation_recording_failed\"}");restore_root(cmd,saved,false,true);restore_root(cmd,saved,true,true);cmd->bind_pipeline(a::pipeline_stage::all_compute,{saved.pipeline});commands[cmd]=saved;}
 proxy->Release();eval_transition(native,c.output,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 records.push_back("{\"kind\":\"live_current\",\"frame\":"+std::to_string(frame)+",\"result\":"+std::to_string(static_cast<uint32_t>(result))+",\"liveGameInputs\":true,\"displaySubstitution\":"+std::string(display?"true":"false")+",\"index\":"+std::to_string(c.evaluated)+",\"timestampFrequency\":"+std::to_string(c.timestamp_frequency)+",\"continuousHistory\":"+std::string(live_continuous_mode?"true":"false")+",\"reset\":"+std::string(reset?"true":"false")+",\"textureCPUTransferUsedAsInput\":false}");if(live_continuous_mode)current_submissions[cmd].push_back({frame,c.evaluated});++c.evaluated;c.reset_pending=false;if(NVSDK_NGX_FAILED(result))c.ready=false;return true;
}
