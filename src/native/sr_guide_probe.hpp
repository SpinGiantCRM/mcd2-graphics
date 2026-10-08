#pragma once
// Explicit, bounded developer gate for the independent pre-temporal producer.
// The game keeps its current AA/output. No source-scale or SDK request is made.
struct GuideProbeSample {
 a::command_list *recording=nullptr; bool submitted=false;
 std::array<DenseReadback,3> readbacks{};
};
struct GuideProbe : SrGuideGeneration {
 LeanBorrowCache borrows;
 a::command_queue *queue=nullptr;
 std::vector<GuideProbeSample> samples;
 ID3D12Fence *capture_fence=nullptr;
 bool signed_capture=false,saved=false,failed=false,busy=false;
 ULONGLONG started=0;
 fs::path directory;
 std::ofstream log;
};
static std::unique_ptr<GuideProbe> guide_probe;
static void guide_probe_failure(const char *reason){
 if(!guide_probe || guide_probe->failed)return;
 guide_probe->failed=true;
 guide_probe->log<<"{\"stage\":\"failure\",\"reason\":\""<<reason<<"\"}\n";
 guide_probe->log.flush();
}
static void guide_probe_begin(a::command_list *cmd){
 if constexpr(!developer_controls)return;
 if(guide_probe)guide_probe->borrows.begin_recording(cmd);
}
static void guide_probe_reset(a::command_list *cmd){
 if constexpr(!developer_controls)return;
 if(!guide_probe || !guide_probe->borrows.recordings.contains(cmd))return;
 uint64_t epoch=0;bool known=lean_reset_epoch(cmd,epoch,true);
 guide_probe->borrows.note_reset(cmd,known,epoch);
 // An unsubmitted captured recording can be cancelled. Never count its data
 // as completed, and never release its resources before proven invalidation.
 for(const auto &sample:guide_probe->samples)
  if(sample.recording==cmd && !sample.submitted)guide_probe_failure("capture_recording_cancelled");
}
static void guide_probe_forget(a::command_list *cmd){
 if constexpr(!developer_controls)return;
 if(!guide_probe)return;
 guide_probe->borrows.reset_pending.erase(cmd);guide_probe->borrows.reset_epochs.erase(cmd);
 guide_probe->borrows.forget(cmd);
}
static void guide_probe_submit(a::command_queue *q,a::command_list *cmd){
 if constexpr(!developer_controls)return;
 if(!guide_probe || !guide_probe->borrows.recordings.contains(cmd))return;
 if(guide_probe->queue!=q){guide_probe->borrows.blocked=true;guide_probe_failure("queue_mismatch");return;}
 for(auto &sample:guide_probe->samples)if(sample.recording==cmd)sample.submitted=true;
}
static void guide_probe_destroy_queue(a::command_queue *q){
 if constexpr(!developer_controls)return;
 if(guide_probe && guide_probe->queue==q){
  guide_probe->borrows.blocked=true;guide_probe->queue=nullptr;guide_probe_failure("queue_destroyed_before_retirement");
 }
}
static void guide_probe_record(a::command_list *cmd){
 if constexpr(!developer_controls)return;
 if(!guide_probe || guide_probe->failed || guide_probe->samples.size()>=2)return;
 auto &c=*guide_probe;const Cmd saved=commands[cmd];const auto slots=resolve(cmd,true);
 const Slot *colour=nullptr,*depth=nullptr,*packed=nullptr,*exposure=nullptr,*pass=nullptr,*view=nullptr,*out=nullptr;
 for(const auto &s:slots)if(s.space==0){
  if(s.binding.type==a::descriptor_type::shader_resource_view){if(s.reg==1)colour=&s;if(s.reg==2)depth=&s;if(s.reg==3)packed=&s;}
  if(s.binding.type==a::descriptor_type::buffer_shader_resource_view && s.reg==0)exposure=&s;
  if(s.binding.type==a::descriptor_type::constant_buffer){if(s.reg==0)pass=&s;if(s.reg==1)view=&s;}
  if(s.binding.type==a::descriptor_type::unordered_access_view && s.reg==0)out=&s;
 }
 std::array<float,84> pc{};std::array<float,632> vc{};
 if(!colour||!depth||!packed||!exposure||!pass||!view||!out||!saved.pipeline||!saved.compute_layout||saved.dynamic_offsets
  ||!root_push_supported(saved.cp)||!root_push_supported(saved.gp)||!read_upload(*pass,sizeof(pc),pc.data())
  ||!read_upload(*view,sizeof(vc),vc.data())){guide_probe_failure("source_binding_contract");return;}
 for(unsigned i:{36u,37u,44u,45u})if(!std::isfinite(pc[i])||pc[i]<1||pc[i]>3840||pc[i]!=std::floor(pc[i])){
  guide_probe_failure("dimensions");return;
 }
 if(!std::isfinite(vc[626])||vc[626]<=0||!std::isfinite(vc[576])||!std::isfinite(vc[577])){guide_probe_failure("exposure_or_jitter");return;}
 auto res=[&](const Slot *s){return reinterpret_cast<ID3D12Resource*>(from_view(cmd->get_device(),s->binding.view).handle);};
 auto *gc=res(colour),*gd=res(depth),*gv=res(packed),*ge=res(exposure),*go=res(out);
 for(auto *r:{gc,gd,gv,ge,go})if(!r||!live_resources.contains(reinterpret_cast<uint64_t>(r))){guide_probe_failure("source_lifetime");return;}
 for(auto *r:{gc,gd,gv}){auto d=r->GetDesc();if(d.Width!=pc[36]||d.Height!=pc[37]||d.DepthOrArraySize!=1||d.SampleDesc.Count!=1){guide_probe_failure("input_dimensions");return;}}
 const auto ev=view_desc(cmd->get_device(),exposure->binding.view);
 if(ev.buffer.offset+16>ge->GetDesc().Width||gc->GetDesc().Format!=DXGI_FORMAT_R11G11B10_FLOAT
  ||static_cast<unsigned>(view_desc(cmd->get_device(),packed->binding.view).format)!=11){guide_probe_failure("input_format");return;}
 if(!c.proxy_device){
  auto *native_device=reinterpret_cast<ID3D12Device*>(cmd->get_device()->get_native());UINT bytes=sizeof(c.proxy_device);
  if(FAILED(native_device->GetPrivateData(observer_device_proxy,&bytes,&c.proxy_device))||!c.proxy_device||bytes!=sizeof(c.proxy_device)){
   c.proxy_device=nullptr;guide_probe_failure("device_proxy");return;
  }
  c.proxy_device->AddRef();c.owned.keep(c.proxy_device);
  if(FAILED(c.proxy_device->QueryInterface(observer_unwrapped,reinterpret_cast<void**>(&c.resource_device)))){guide_probe_failure("resource_device");return;}
  c.owned.keep(c.resource_device);c.width=unsigned(pc[36]);c.height=unsigned(pc[37]);
  c.motion=eval_texture(c.resource_device,c.owned,c.width,c.height,DXGI_FORMAT_R16G16_FLOAT,true);
  c.exposure=eval_texture(c.resource_device,c.owned,1,1,DXGI_FORMAT_R32_FLOAT,true);
  if(!c.motion||!c.exposure||FAILED(c.resource_device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&c.capture_fence)))){
   guide_probe_failure("owned_resources");return;
  }
  c.owned.keep(c.capture_fence);
 }
 if(c.width!=pc[36]||c.height!=pc[37]){guide_probe_failure("capture_resize");return;}
 auto *proxy=validated_game_proxy(cmd);if(!proxy){guide_probe_failure("command_proxy");return;}
 ID3D12Device *device=nullptr;const bool matched=SUCCEEDED(proxy->GetDevice(IID_PPV_ARGS(&device)))&&device==c.proxy_device;
 if(device)device->Release();if(!matched){proxy->Release();guide_probe_failure("command_device");return;}
 SrGuideInput input{gc,gd,gv,ge,go,*pass,*view,ev,static_cast<unsigned>(view_desc(cmd->get_device(),depth->binding.view).format)};
 const auto *error=record_sr_guides(c,c.borrows,cmd,proxy,saved,input);proxy->Release();
 if(error){guide_probe_failure(error);return;}
 GuideProbeSample sample;sample.recording=cmd;unsigned index=unsigned(c.samples.size());
 auto *native=reinterpret_cast<ID3D12GraphicsCommandList*>(cmd->get_native());
 std::array<ID3D12Resource*,3> textures{c.motion,c.current_depth,c.exposure};
 const char *names[]={"motion","depth","exposure"};
 for(unsigned i=0;i<3;++i){
  sample.readbacks[i]=dense_readback(c.resource_device,c.owned,native,textures[i],c.directory/(std::to_string(index)+"-"+names[i]+".bin"));
  if(!sample.readbacks[i].buffer){guide_probe_failure("readback_allocation");return;}
 }
 std::ofstream(c.directory/(std::to_string(index)+"-pass.bin"),std::ios::binary).write(reinterpret_cast<const char*>(pc.data()),sizeof(pc));
 std::ofstream(c.directory/(std::to_string(index)+"-view.bin"),std::ios::binary).write(reinterpret_cast<const char*>(vc.data()),sizeof(vc));
 c.samples.push_back(sample);
 c.log<<"{\"stage\":\"pre_temporal_guides\",\"sample\":"<<index<<",\"presentSequence\":"<<frame
  <<",\"shader\":"<<saved.cs<<",\"render\":["<<c.width<<","<<c.height<<"],\"output\":["<<pc[44]<<","<<pc[45]
  <<"],\"preExposure\":"<<vc[626]<<",\"frameDeltaMs\":"<<frame_delta_ms<<",\"NGXContextPresent\":"<<(live_fixture?"true":"false")
  <<",\"SDKCalls\":0,\"outputReplaced\":false,\"sourceScaleChanged\":false}\n";c.log.flush();
}
static void guide_probe_present(a::command_queue *q,std::unique_lock<std::recursive_mutex> &guard){
 if constexpr(!developer_controls)return;
 if(!guide_probe){
  if(!developer_file_exists(root/"provider-guide-start.txt"))return;
  std::ifstream request(root/"provider-guide-start.txt");std::string name;request>>name;request.close();fs::remove(root/"provider-guide-start.txt");
  if(name.empty()||name.size()>49||name.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-")!=std::string::npos)return;
  guide_probe=std::make_unique<GuideProbe>();auto &c=*guide_probe;c.queue=q;c.started=GetTickCount64();
  c.directory=root/name;fs::create_directories(c.directory);c.log.open(c.directory/"guides.jsonl");return;
 }
 auto &c=*guide_probe;if(c.queue!=q||c.busy)return;auto &cache=c.borrows;
 const auto pending=cache.reset_pending;for(auto *cmd:pending){uint64_t epoch=0;if(lean_reset_epoch(cmd,epoch))cache.confirm_reset(cmd,epoch);}
 if(!c.failed && c.samples.size()<2 && GetTickCount64()-c.started>15000)guide_probe_failure("capture_timeout");
 auto *queue=reinterpret_cast<ID3D12CommandQueue*>(q->get_native());
 if(!c.failed && c.samples.size()==2 && !c.signed_capture && std::all_of(c.samples.begin(),c.samples.end(),[](const auto &s){return s.submitted;})){
  if(FAILED(queue->Signal(c.capture_fence,1)))guide_probe_failure("capture_signal");else c.signed_capture=true;
 }
 if(c.signed_capture && !c.saved){
  const auto completed=c.capture_fence->GetCompletedValue();
  if(completed==UINT64_MAX)guide_probe_failure("device_removed");
  else if(completed>=1){
   c.busy=true;guard.unlock();bool written=true;
   for(const auto &sample:c.samples)for(const auto &readback:sample.readbacks){dense_save(readback);std::error_code error;
    const auto bytes=fs::file_size(readback.path,error);written&=!error && bytes==readback.rowbytes*readback.rows;
   }
   guard.lock();c.busy=false;if(!written)guide_probe_failure("readback_save_failed");
   c.saved=true;c.log<<"{\"stage\":\"readbacks_saved\",\"afterGPUFence\":true,\"files\":6}\n";c.log.flush();
  }
 }
 if(cache.fence && !cache.blocked){
  bool signal=false;for(const auto &[key,entry]:cache.entries)signal|=entry.awaiting_signal;
  if(signal){auto value=cache.next_fence+1;if(FAILED(queue->Signal(cache.fence,value))){cache.blocked=true;guide_probe_failure("retirement_signal");}
   else{cache.next_fence=value;++cache.signals;for(auto &[key,entry]:cache.entries)if(entry.awaiting_signal){entry.awaiting_signal=false;entry.retirement_fence=value;}}
  }
  auto done=cache.fence->GetCompletedValue();if(done==UINT64_MAX){cache.blocked=true;guide_probe_failure("retirement_device_removed");}
  else{cache.completed_fence=done;cache.retire_ready(done,frame);}
 }
 auto release=std::move(cache.ready_to_release);cache.ready_to_release.clear();c.busy=true;guard.unlock();for(auto &entry:release)LeanBorrowCache::release(entry);guard.lock();c.busy=false;
 if((c.saved||c.failed) && cache.recordings.empty() && cache.entries.empty() && !cache.blocked){
  c.log<<"{\"stage\":\"retired\",\"complete\":"<<(c.saved&&!c.failed?"true":"false")<<",\"bundles\":"<<cache.retired<<",\"SDKCalls\":0}\n";c.log.flush();
  auto retired=std::move(guide_probe);guard.unlock();retired->borrows.clear_after_idle();
  for(auto it=retired->owned.resources.rbegin();it!=retired->owned.resources.rend();++it)if(*it)(*it)->Release();retired->owned.resources.clear();retired.reset();guard.lock();
 }
}
