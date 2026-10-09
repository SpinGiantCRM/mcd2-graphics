// Private replacement prototype. No pixel readbacks or validation work in the
// normal path. Object interop still uses the previously tested pinned adapter.
#include "sr_guide_producer.hpp"
struct LeanState {
 bool wanted=false,history_dirty=false,last_wanted=false,stop=false,failed=false,cleanup_in_progress=false,terminal=false;
 uint64_t evaluations=0,native_dispatches=0,native_resets=0,failures=0,missing=0;
 std::array<uint32_t,4> active_rect{};
 uint64_t last_frame=0,queue=0; a::command_list *pending=nullptr; bool direct_output=true,capture_requested=false,reset_pair_requested=false;uint64_t transient_fallbacks=0;bool rebuild=false,inject_failure=false;unsigned next_width=0,next_height=0,next_outwidth=0,next_outheight=0; ID3D12Resource *native_reset_buffer=nullptr;
 LeanBorrowCache borrow;
 std::ofstream log;
} lean;
#include "native_reset_gate.hpp"
static bool read_upload(const Slot &slot,size_t bytes,void *dest);
#include "ui_control_state.hpp"
// Native reset uploads contain only game-owned shader constants. They do not
// reference NGX objects and must survive feature-generation retirement.
static LeanBorrowCache native_reset_borrows;
static a::command_queue *native_reset_queue=nullptr;
#include "reset_epoch_contract.hpp"
static bool lean_reset_epoch(a::command_list *cmd,uint64_t &epoch,bool arm=false){
 auto *native=reinterpret_cast<ID3D12GraphicsCommandList*>(cmd->get_native());
 if(!native)return false;UINT size=sizeof(epoch);epoch=0;
 const auto result=native->GetPrivateData(mcd2_reset_epoch_guid,&size,&epoch);
 if(result==DXGI_ERROR_NOT_FOUND){epoch=0;return arm&&SUCCEEDED(native->SetPrivateData(mcd2_reset_epoch_guid,sizeof(epoch),&epoch));}
 return SUCCEEDED(result)&&size==sizeof(epoch);
}
static void lean_reset_recording(a::command_list *cmd){
 guide_probe_reset(cmd);native_fg_reset(cmd);fsr_runtime_reset(cmd);
 if(!lean.borrow.recordings.contains(cmd)&&!native_reset_borrows.recordings.contains(cmd))return;
 uint64_t epoch=0;bool known=lean_reset_epoch(cmd,epoch,true);
 if(lean.borrow.recordings.contains(cmd))lean.borrow.note_reset(cmd,known,epoch);
 if(native_reset_borrows.recordings.contains(cmd))native_reset_borrows.note_reset(cmd,known,epoch);
}
static void lean_confirm_resets(){
 auto confirm=[](LeanBorrowCache &cache,bool feature){
  const auto pending=cache.reset_pending;
  for(auto *cmd:pending){uint64_t epoch=0;if(lean_reset_epoch(cmd,epoch)&&cache.confirm_reset(cmd,epoch)){
   if(feature&&lean.pending==cmd)lean.pending=nullptr;
   if(lean.log.is_open())lean.log<<"{\"kind\":\"successful_reset_confirmed\",\"featureRecording\":"<<(feature?"true":"false")<<",\"frame\":"<<frame<<"}\n";
  }}
 };
 confirm(lean.borrow,true);confirm(native_reset_borrows,false);
}
static void lean_begin_recording(a::command_list *cmd){
 guide_probe_begin(cmd);native_fg_begin(cmd);fsr_runtime_begin(cmd);
 // Reset can cancel an evaluation before submission. Clear its CPU sentinel
 // only after replacement recording is observed; references still retire by fence.
 if(lean.borrow.begin_recording(cmd) && lean.pending==cmd)lean.pending=nullptr;
 native_reset_borrows.begin_recording(cmd);
}
static void lean_forget_recording(a::command_list *cmd){guide_probe_forget(cmd);native_fg_forget(cmd);fsr_runtime_forget(cmd);lean.borrow.reset_pending.erase(cmd);lean.borrow.reset_epochs.erase(cmd);lean.borrow.forget(cmd);native_reset_borrows.reset_pending.erase(cmd);native_reset_borrows.reset_epochs.erase(cmd);native_reset_borrows.forget(cmd);if(lean.pending==cmd)lean.pending=nullptr;}
static bool lean_cleanup(a::command_queue *q,std::unique_lock<std::recursive_mutex> &guard,bool terminal=false){
 if(!live_fixture || lean.cleanup_in_progress || (!terminal && lean.history_dirty) || !lean.borrow.recordings.empty() || lean.borrow.blocked)return false;
 lean.cleanup_in_progress=true;
 auto context=std::move(live_fixture);auto borrows=std::move(lean.borrow);lean.borrow=LeanBorrowCache{};
 guard.unlock();
 bool success=live_wait_generation(q,*context);
 if(success){
  borrows.clear_after_idle();for(auto &entry:borrows.ready_to_release)LeanBorrowCache::release(entry);borrows.ready_to_release.clear();
  success=cleanup_detached_live(*context);
 }
 guard.lock();lean.borrow=std::move(borrows);lean.cleanup_in_progress=false;
 if(!success){context->ready=false;live_fixture=std::move(context);lean.borrow.blocked=true;lean.failed=true;lean.wanted=false;++lean.failures;lean.log<<"{\"kind\":\"generation_cleanup_blocked\",\"retained\":true}\n";lean.log.flush();return false;}
 lean.native_reset_buffer=nullptr;return true;
}
static void lean_retire_cache(LeanBorrowCache &cache,a::command_queue *q){
 if(!cache.fence || cache.blocked)return;
 bool needs_signal=false;for(const auto &[key,e]:cache.entries)needs_signal|=e.awaiting_signal;
 if(needs_signal){
  uint64_t value=cache.next_fence+1;
  auto *queue=reinterpret_cast<ID3D12CommandQueue*>(q->get_native());
  auto hr=queue->Signal(cache.fence,value);
  if(FAILED(hr)){cache.blocked=true;lean.wanted=false;lean.failed=true;++lean.failures;lean.log<<"{\"kind\":\"retirement_signal_failure\",\"HRESULT\":"<<unsigned(hr)<<"}\n";return;}
  cache.next_fence=value;++cache.signals;
  for(auto &[key,e]:cache.entries)if(e.awaiting_signal){e.awaiting_signal=false;e.retirement_fence=value;}
 }
 uint64_t completed=cache.fence->GetCompletedValue();
 if(completed==UINT64_MAX){cache.blocked=true;lean.wanted=false;lean.failed=true;++lean.failures;lean.log<<"{\"kind\":\"retirement_device_removed\"}\n";return;}
 cache.completed_fence=completed;
 cache.retire_ready(completed,frame);
}
static void lean_retire_borrows(a::command_queue *q){
 if(live_fixture && q==live_fixture->integration_queue)lean_retire_cache(lean.borrow,q);
 if(native_reset_queue==q)lean_retire_cache(native_reset_borrows,q);
}
static bool read_upload(const Slot &slot,size_t bytes,void *dest) {
 auto *r=reinterpret_cast<ID3D12Resource*>(slot.binding.cb.buffer.handle);
 D3D12_HEAP_PROPERTIES hp{};D3D12_HEAP_FLAGS flags{};
 if(!r || FAILED(r->GetHeapProperties(&hp,&flags)) || hp.Type!=D3D12_HEAP_TYPE_UPLOAD || slot.binding.cb.offset+bytes>r->GetDesc().Width || (slot.binding.cb.size!=UINT64_MAX && slot.binding.cb.size<bytes))return false;
 void *mapped=nullptr;D3D12_RANGE range{size_t(slot.binding.cb.offset),size_t(slot.binding.cb.offset+bytes)};
 if(FAILED(r->Map(0,&range,&mapped)))return false;
 memcpy(dest,static_cast<char*>(mapped)+slot.binding.cb.offset,bytes);D3D12_RANGE empty{0,0};r->Unmap(0,&empty);return true;
}
static bool lean_native(a::command_list *cmd,const Cmd &saved,const Slot &pass,const std::array<float,84> &data,uint32_t x,uint32_t y,uint32_t z) {
 auto *native=reinterpret_cast<ID3D12GraphicsCommandList*>(cmd->get_native());
 if(lean.history_dirty){
  if(!live_fixture || saved.cp.find(pass.param)==saved.cp.end())return false;
  auto &c=*live_fixture;
  // Each reset gets a fresh immutable upload. Recorded lists can replay after
  // submission, so GPU idle alone is insufficient permission to overwrite it.
  
  if(native_reset_borrows.blocked || native_reset_borrows.entries.size()>=256 || (native_reset_queue && native_reset_queue!=c.integration_queue))return false;
  if(!native_reset_borrows.fence){if(FAILED(c.resource_device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&native_reset_borrows.fence))))return false;native_reset_queue=c.integration_queue;}
  EvalOwned reset_owned;auto *buffer=eval_buffer(c.resource_device,reset_owned,512,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
  if(!buffer)return false;
  void *mapped=nullptr;D3D12_RANGE empty{0,0};if(FAILED(buffer->Map(0,&empty,&mapped))){buffer->Release();reset_owned.resources.clear();return false;}
  memset(mapped,0,512);memcpy(mapped,data.data(),336);uint32_t reset=1;memcpy(static_cast<char*>(mapped)+48,&reset,4);buffer->Unmap(0,nullptr);
  LeanBorrowKey reset_key{reinterpret_cast<uint64_t>(buffer),0,UINT_MAX,0,0,0,0,0};
  LeanBorrowEntry reset_entry;reset_entry.resources[0]=buffer;
  native_reset_borrows.entries.emplace(reset_key,reset_entry);reset_owned.resources.clear();
  native_reset_borrows.record(cmd,reset_key,frame);lean.native_reset_buffer=buffer;
  native_reset_borrows.peak_entries=std::max<uint64_t>(native_reset_borrows.peak_entries,native_reset_borrows.entries.size());
  a::buffer_range range{{reinterpret_cast<uint64_t>(buffer)},0,512};a::descriptor_table_update update{};update.binding=pass.rangebinding;update.count=1;update.type=a::descriptor_type::constant_buffer;update.descriptors=&range;
  cmd->push_descriptors(a::shader_stage::all_compute,{saved.compute_layout},pass.param,update);
  if(!native_reset_pair(cmd,saved,x,y,z))native->Dispatch(x,y,z);
  restore_root(cmd,saved,false,true);restore_root(cmd,saved,true,true);cmd->bind_pipeline(a::pipeline_stage::all_compute,{saved.pipeline});commands[cmd]=saved;
  lean.history_dirty=false;++lean.native_resets;
  lean.log<<"{\"kind\":\"native_history_reset\",\"frame\":"<<frame<<",\"passByteOffset\":48,\"gameBufferModified\":false}\n";
 }else native->Dispatch(x,y,z);
 ++lean.native_dispatches;return true;
}
static bool lean_gate(a::command_list *cmd,uint32_t shader,uint32_t x,uint32_t y,uint32_t z) {
 if(lean.cleanup_in_progress || live_cleanup_busy || lean.terminal)return false;
 ui_observe_source(cmd);
 if(!live_fixture || !live_fixture->ready)return false;
 auto &c=*live_fixture;const Cmd saved=commands[cmd];auto slots=resolve(cmd,true);
 const Slot *colour=nullptr,*depth=nullptr,*packed=nullptr,*exposure=nullptr,*pass=nullptr,*view=nullptr,*out=nullptr;
 for(const auto &s:slots)if(s.space==0){
  if(s.binding.type==a::descriptor_type::shader_resource_view){if(s.reg==1)colour=&s;if(s.reg==2)depth=&s;if(s.reg==3)packed=&s;}
  if(s.binding.type==a::descriptor_type::buffer_shader_resource_view && s.reg==0)exposure=&s;
  if(s.binding.type==a::descriptor_type::constant_buffer){if(s.reg==0)pass=&s;if(s.reg==1)view=&s;}
  if(s.binding.type==a::descriptor_type::unordered_access_view && s.reg==0)out=&s;
 }
 std::array<float,84> pc{};std::array<float,632> vc{};
 if(!is_taa(shader) || !pass || !view || !out || !saved.pipeline || !saved.compute_layout || saved.dynamic_offsets || !root_push_supported(saved.cp) || !root_push_supported(saved.gp) || !read_upload(*pass,sizeof(pc),pc.data()) || !read_upload(*view,sizeof(vc),vc.data())){
  if(lean.wanted){lean.wanted=false;lean.failed=true;++lean.failures;lean.log<<"{\"kind\":\"contract_failure\",\"frame\":"<<frame<<",\"reason\":\"pass_or_shader_contract\"}\n";}return false;
 }
 if(!lean.wanted || lean.failed)return lean_native(cmd,saved,*pass,pc,x,y,z);
 auto fail=[&](const char *reason){lean.wanted=false;lean.failed=true;++lean.failures;c.reset_pending=true;lean.log<<"{\"kind\":\"contract_failure\",\"frame\":"<<frame<<",\"reason\":\""<<reason<<"\"}\n";return lean_native(cmd,saved,*pass,pc,x,y,z);};
 if(!colour || !depth || !packed || !exposure)return fail("inputs_missing");
 auto res=[&](const Slot *s){return reinterpret_cast<ID3D12Resource*>(from_view(cmd->get_device(),s->binding.view).handle);};
 auto *gc=res(colour),*gd=res(depth),*gv=res(packed),*ge=res(exposure),*go=res(out);
 for(auto *r:{gc,gd,gv,ge,go})if(!r || !live_resources.contains(reinterpret_cast<uint64_t>(r)))return fail("resource_lifetime");
 const auto od=go->GetDesc();
 if(pc[36]!=c.width || pc[37]!=c.height || pc[44]!=c.outwidth || pc[45]!=c.outheight){
  // Hot output resize timed out after a nominally successful NGX rebuild in this
  // environment. Retire/reset and fall back instead; a new UI revision can
  // explicitly create a fresh feature at the new output size.
  if(ui_control.phase==2 && (pc[44]!=c.outwidth || pc[45]!=c.outheight)){
   lean.wanted=false;lean.failed=true;lean.rebuild=false;
   lean.log<<"{\"kind\":\"output_resize_native_fallback\",\"frame\":"<<frame<<",\"output\":["<<pc[44]<<","<<pc[45]<<"]}\n";lean.log.flush();
   return lean_native(cmd,saved,*pass,pc,x,y,z);
  }
  if(!std::isfinite(pc[36]) || !std::isfinite(pc[37]) || !std::isfinite(pc[44]) || !std::isfinite(pc[45]) || pc[36]<640 || pc[37]<360 || pc[44]<pc[36] || pc[45]<pc[37] || pc[44]>3840 || pc[45]>2160)return fail("unsupported_dimensions");
  lean.next_width=unsigned(pc[36]);lean.next_height=unsigned(pc[37]);lean.next_outwidth=unsigned(pc[44]);lean.next_outheight=unsigned(pc[45]);lean.rebuild=true;lean.wanted=false;
  lean.log<<"{\"kind\":\"dimensions_rebuild_requested\",\"frame\":"<<frame<<",\"input\":["<<lean.next_width<<","<<lean.next_height<<"],\"output\":["<<lean.next_outwidth<<","<<lean.next_outheight<<"]}\n";
  return lean_native(cmd,saved,*pass,pc,x,y,z);
 }
 // The inclusive native input viewport is distinct from padded allocation.
 // Validate it against the UV scale/bias used by the original temporal shader.
 std::array<uint32_t,4> rect{};memcpy(rect.data(),pc.data()+40,sizeof(rect));
 if(rect[2]<rect[0] || rect[3]<rect[1] || rect[2]>=c.width || rect[3]>=c.height)return fail("invalid_active_rectangle");
 unsigned aw=rect[2]-rect[0]+1,ah=rect[3]-rect[1]+1;
 if(aw<640 || ah<360 || !std::isfinite(pc[0]) || !std::isfinite(pc[1]) || !std::isfinite(pc[2]) || !std::isfinite(pc[3]) || std::abs(pc[0]*c.width-aw)>.01f || std::abs(pc[1]*c.height-ah)>.01f || std::abs(pc[2]*c.width-rect[0])>.01f || std::abs(pc[3]*c.height-rect[1])>.01f)return fail("active_rectangle_uv_contract");
 if(rect!=lean.active_rect){
  lean.active_rect=rect;c.reset_pending=true;
  lean.log<<"{\"kind\":\"active_rectangle\",\"frame\":"<<frame<<",\"allocation\":["<<c.width<<","<<c.height<<"],\"origin\":["<<rect[0]<<","<<rect[1]<<"],\"render\":["<<aw<<","<<ah<<"]}\n";
 }
 if(lean.inject_failure){
  lean.inject_failure=false;++lean.transient_fallbacks;c.reset_pending=true;
  lean.log<<"{\"kind\":\"injected_pre_evaluation_fallback\",\"frame\":"<<frame<<",\"automaticResume\":true}\n";
  return lean_native(cmd,saved,*pass,pc,x,y,z);
 }
 if(!std::isfinite(vc[626]) || vc[626]<=0 || !std::isfinite(vc[576]) || !std::isfinite(vc[577]) || od.Width!=c.outwidth || od.Height!=c.outheight || od.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT || od.DepthOrArraySize!=1 || od.MipLevels!=1 || od.SampleDesc.Count!=1)return fail("dimensions_or_output");
 for(auto *r:{gc,gd,gv}){const auto d=r->GetDesc();if(d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D || d.Width!=c.width || d.Height!=c.height || d.DepthOrArraySize!=1 || d.SampleDesc.Count!=1){
   ++lean.transient_fallbacks;c.reset_pending=true;
   if(lean.transient_fallbacks<=16)lean.log<<"{\"kind\":\"transient_input_fallback\",\"frame\":"<<frame<<",\"actualWidth\":"<<d.Width<<",\"actualHeight\":"<<d.Height<<",\"expectedWidth\":"<<c.width<<",\"expectedHeight\":"<<c.height<<"}\n";
   return lean_native(cmd,saved,*pass,pc,x,y,z);
 }}
 if(gc->GetDesc().Format!=DXGI_FORMAT_R11G11B10_FLOAT || static_cast<uint32_t>(view_desc(cmd->get_device(),packed->binding.view).format)!=11 || go==gc || go==gd || go==gv)return fail("formats_or_alias");
 auto state=[&](ID3D12Resource *r){auto k=reinterpret_cast<uint64_t>(r);auto it=saved.states.find(k);if(it!=saved.states.end())return native_state(it->second);auto prior=submitted_states.find(k);return prior==submitted_states.end()?D3D12_RESOURCE_STATE_COMMON:native_state(prior->second);};
 // The original compute pass already requires these bound SRVs readable.
 // Resource transitions can be in another not-yet-submitted command list;
 // prior-submission states cannot override this dispatch contract. Borrow
 // read-only inputs without changing their existing states.
 if(!lean.evaluations)lean.log<<"{\"kind\":\"source_state_scope\",\"colourTracked\":"<<state(gc)<<",\"depthTracked\":"<<state(gd)<<",\"velocityTracked\":"<<state(gv)<<",\"authority\":\"original_compute_SRV_contract\"}\n";
 auto ev=view_desc(cmd->get_device(),exposure->binding.view);if(ev.buffer.offset+16>ge->GetDesc().Width)return fail("exposure_range");
 auto *proxy=validated_game_proxy(cmd);if(!proxy)return fail("proxy_identity");
 ID3D12Device *pd=nullptr;bool match=SUCCEEDED(proxy->GetDevice(IID_PPV_ARGS(&pd))) && pd==c.proxy_device;if(pd)pd->Release();
 if(!match){proxy->Release();return fail("proxy_device");}
 SrGuideInput guide_input{gc,gd,gv,ge,go,*pass,*view,ev,static_cast<unsigned>(view_desc(cmd->get_device(),depth->binding.view).format)};
 if(const auto *reason=record_sr_guides(c,lean.borrow,cmd,proxy,saved,guide_input)){proxy->Release();return fail(reason);}
 auto *native=reinterpret_cast<ID3D12GraphicsCommandList*>(cmd->get_native());
 D3D12_RESOURCE_BARRIER order{};order.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;
 auto resource=mcd2_ngx_set_resource;auto fp=mcd2_ngx_set_f;auto ip=mcd2_ngx_set_i;
 resource(c.params,NVSDK_NGX_Parameter_Output,lean.direct_output?go:c.output);resource(c.params,NVSDK_NGX_Parameter_Color,gc);resource(c.params,NVSDK_NGX_Parameter_Depth,c.current_depth);
 auto ui=mcd2_ngx_set_ui;
 ui(c.params,NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Width,aw);ui(c.params,NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Height,ah);
 ui(c.params,NVSDK_NGX_Parameter_DLSS_Input_Color_Subrect_Base_X,rect[0]);ui(c.params,NVSDK_NGX_Parameter_DLSS_Input_Color_Subrect_Base_Y,rect[1]);
 ui(c.params,NVSDK_NGX_Parameter_DLSS_Input_Depth_Subrect_Base_X,rect[0]);ui(c.params,NVSDK_NGX_Parameter_DLSS_Input_Depth_Subrect_Base_Y,rect[1]);
 ui(c.params,NVSDK_NGX_Parameter_DLSS_Input_MV_SubrectBase_X,rect[0]);ui(c.params,NVSDK_NGX_Parameter_DLSS_Input_MV_SubrectBase_Y,rect[1]);
 fp(c.params,NVSDK_NGX_Parameter_Jitter_Offset_X,vc[576]*aw*.5f);fp(c.params,NVSDK_NGX_Parameter_Jitter_Offset_Y,-vc[577]*ah*.5f);fp(c.params,NVSDK_NGX_Parameter_DLSS_Pre_Exposure,vc[626]);fp(c.params,NVSDK_NGX_Parameter_FrameTimeDeltaInMsec,float(frame_delta_ms));
 uint32_t native_reset=0;memcpy(&native_reset,pc.data()+12,4);
 bool reset=c.evaluated==0 || c.reset_pending || native_reset || (lean.last_frame && frame>lean.last_frame+1);ip(c.params,NVSDK_NGX_Parameter_Reset,reset?1:0);
 using Evaluate=NVSDK_NGX_Result(*)(ID3D12GraphicsCommandList*,const NVSDK_NGX_Handle*,const NVSDK_NGX_Parameter*,void*);
 internal_evaluation=true;auto result=live_export<Evaluate>("NVSDK_NGX_D3D12_EvaluateFeature")(proxy,c.feature,c.params,nullptr);internal_evaluation=false;
 restore_root(cmd,saved,false,true);restore_root(cmd,saved,true,true);cmd->bind_pipeline(a::pipeline_stage::all_compute,{saved.pipeline});commands[cmd]=saved;
 if(NVSDK_NGX_FAILED(result)){proxy->Release();return fail("NGX_evaluate");}
 // First lean gate retains only this output copy. Direct output aliasing is a
 // subsequent controlled gate; colour/packed input copies already disappear.
 if(!lean.direct_output){ order.UAV.pResource=c.output;native->ResourceBarrier(1,&order);eval_transition(native,c.output,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);eval_transition(native,go,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_DEST);native->CopyResource(go,c.output);eval_transition(native,go,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);eval_transition(native,c.output,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 }else{order.UAV.pResource=go;native->ResourceBarrier(1,&order);}
 if(lean.capture_requested){
  std::ofstream(root/label/(std::to_string(frame)+"-pass.bin"),std::ios::binary).write(reinterpret_cast<const char*>(pc.data()),sizeof(pc));
  std::ofstream(root/label/(std::to_string(frame)+"-view.bin"),std::ios::binary).write(reinterpret_cast<const char*>(vc.data()),sizeof(vc));
  lean.capture_requested=false;auto *capture=lean.direct_output?go:c.output;eval_transition(native,capture,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  auto read=dense_readback(c.resource_device,c.owned,native,capture,root/label/(std::to_string(frame)+"-lean-output.bin"));
  if(read.buffer){read.buffer->AddRef();copies.push_back({read.buffer,cmd,nullptr,read.total,read.rows,read.fp.Footprint.RowPitch,static_cast<uint32_t>(read.rowbytes),root/label/(std::to_string(frame)+"-lean-output.bin")});}
  eval_transition(native,capture,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 }
 proxy->Release();commands[cmd].states[reinterpret_cast<uint64_t>(go)]=a::resource_usage::unordered_access;
 lean.pending=cmd;++c.evaluated;++lean.evaluations;lean.history_dirty=true;lean.last_frame=frame;c.reset_pending=false;
 if(reset)lean.log<<"{\"kind\":\"NGX_reset\",\"frame\":"<<frame<<",\"nativeReset\":"<<native_reset<<"}\n";
 return true;
}
static void lean_submission(a::command_queue *q,a::command_list *cmd){
 if(native_reset_borrows.recordings.contains(cmd) && q!=native_reset_queue){native_reset_borrows.blocked=true;lean.failed=true;lean.wanted=false;++lean.failures;}
 if(!live_fixture || !lean.borrow.recordings.contains(cmd))return;if(lean.pending==cmd)lean.pending=nullptr;auto queue=q->get_native();
 if(!lean.queue){lean.queue=queue;lean.log<<"{\"kind\":\"submission_queue\",\"sameAsFeatureCreation\":"<<(queue==live_fixture->integration_queue->get_native()?"true":"false")<<"}\n";}
 if(lean.queue!=queue || queue!=live_fixture->integration_queue->get_native()){lean.borrow.blocked=true;lean.failed=true;lean.wanted=false;++lean.failures;lean.log<<"{\"kind\":\"queue_mismatch\",\"frame\":"<<frame<<"}\n";}
}
// Diagnostic-only feature creation on a separate owned queue. No game command
// lists, colour inputs, evaluation, output substitution or host queue reset.
static void lean_queue_probe(a::command_queue *host,std::unique_lock<std::recursive_mutex> &guard) {
 std::ifstream request(root/"lean-queue-probe.txt");std::string next;request>>next;request.close();fs::remove(root/"lean-queue-probe.txt");
 if(next.empty() || next.size()>=50 || next.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-")!=std::string::npos)return;
 label=next;fs::create_directories(root/label);lean=LeanState{};lean.log.open(root/label/"lean-events.jsonl");
 ID3D12Device *proxy=nullptr;UINT bytes=sizeof(proxy);auto *native=reinterpret_cast<ID3D12Device*>(host->get_device()->get_native());
 if(FAILED(native->GetPrivateData(observer_device_proxy,&bytes,&proxy)) || !proxy || bytes!=sizeof(proxy))return;
 proxy->AddRef();D3D12_COMMAND_QUEUE_DESC desc{};desc.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;
 ID3D12CommandQueue *owned_queue=nullptr;probe_queue_api=nullptr;capture_probe_queue=true;
 auto hr=proxy->CreateCommandQueue(&desc,IID_PPV_ARGS(&owned_queue));capture_probe_queue=false;
 const bool created=SUCCEEDED(hr) && owned_queue && probe_queue_api;
 const bool ready=created && init_live_saved(probe_queue_api,&guard);
 // Queue release can invoke our destruction callback; never hold this lock.
 guard.unlock();ULONG refs=owned_queue?owned_queue->Release():0;proxy->Release();guard.lock();
 const bool callback=developer_file_exists(root/label/"queue-teardown.json");lean.terminal=false;probe_queue_api=nullptr;
 std::ofstream(root/label/"probe-result.json")<<"{\"created\":"<<(created?"true":"false")<<",\"featureReady\":"<<(ready?"true":"false")<<",\"queueReleaseRefs\":"<<refs<<",\"destructionCallbackObserved\":"<<(callback?"true":"false")<<",\"generationRemaining\":"<<(live_fixture?"true":"false")<<",\"evaluations\":0}\n";
}
// Terminal receipts must reflect completed cleanup, not pre-cleanup ownership.
static void lean_write_status(){
 std::ofstream(root/label/"lean-status.json")<<"{\"evaluations\":"<<lean.evaluations<<",\"transientFallbacks\":"<<lean.transient_fallbacks<<",\"nativeDispatches\":"<<lean.native_dispatches<<",\"nativeResets\":"<<lean.native_resets<<",\"failures\":"<<lean.failures<<",\"missingTAAFrames\":"<<lean.missing<<",\"immutableHeaps\":"<<lean.borrow.entries.size()<<",\"recordedLists\":"<<lean.borrow.recordings.size()<<",\"resetPendingLists\":"<<lean.borrow.reset_pending.size()<<",\"retiredBundles\":"<<lean.borrow.retired<<",\"peakBundles\":"<<lean.borrow.peak_entries<<",\"fenceSignals\":"<<lean.borrow.signals<<",\"fenceCompleted\":"<<lean.borrow.completed_fence<<",\"retirementBlocked\":"<<(lean.borrow.blocked?"true":"false")<<",\"featurePresent\":"<<(live_fixture?"true":"false")<<",\"nativeResetBuffers\":"<<native_reset_borrows.entries.size()<<",\"nativeResetRecordedLists\":"<<native_reset_borrows.recordings.size()<<",\"nativeResetRetired\":"<<native_reset_borrows.retired<<",\"nativeResetBlocked\":"<<(native_reset_borrows.blocked?"true":"false")<<",\"on\":"<<(lean.wanted?"true":"false")<<",\"historyDirty\":"<<(lean.history_dirty?"true":"false")<<"}\n";lean.log.flush();
}
#include "ui_control_present.hpp"
static void lean_present(a::command_queue *q,std::unique_lock<std::recursive_mutex> &guard) {
 if(lean.cleanup_in_progress || live_cleanup_busy || lean.terminal)return;
 lean_confirm_resets();
 lean_retire_borrows(q);

 if(!copies.empty()){
  for(auto &copy:copies)if(copy.queue){copy.queue->wait_idle();void *mapped=nullptr;D3D12_RANGE range{0,size_t(copy.bytes)};
   if(SUCCEEDED(copy.readback->Map(0,&range,&mapped))){std::ofstream out(copy.path,std::ios::binary);out.write(static_cast<char*>(mapped),copy.bytes);D3D12_RANGE empty{0,0};copy.readback->Unmap(0,&empty);}copy.readback->Release();
  }
  copies.erase(std::remove_if(copies.begin(),copies.end(),[](const Copy &c){return c.queue!=nullptr;}),copies.end());
 }
 if(live_fixture && !seen_taa){live_fixture->reset_pending=true;++lean.missing;}
 static ULONGLONG poll=0;auto now=GetTickCount64();if(now-poll<100)return;poll=now;
 if(!fsr_runtime_owns_source() && ui_poll_intent())ui_apply_queued(q,guard);
 if(!live_fixture && developer_file_exists(root/"lean-queue-probe.txt")){lean_queue_probe(q,guard);return;}
 if(!live_fixture && developer_file_exists(root/"lean-start.txt")){
  std::ifstream f(root/"lean-start.txt");std::string next;f>>next;f.close();fs::remove(root/"lean-start.txt");
  if(next.empty() || next.size()>=50 || next.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-")!=std::string::npos)return;
  std::ifstream profile(root/"lean-profile.cfg");if(profile){unsigned w,h,ow,oh;if(profile>>w>>h>>ow>>oh && w>=640 && h>=360 && ow>=w && oh>=h && ow<=3840 && oh<=2160){lean_width=w;lean_height=h;lean_outwidth=ow;lean_outheight=oh;}}
  label=next;fs::create_directories(root/label);lean=LeanState{};lean.log.open(root/label/"lean-events.jsonl");
  if(init_live_saved(q,&guard)){lean.wanted=true;lean.last_wanted=true;lean.log<<"{\"kind\":\"started\",\"nativeAASkipped\":true,\"inputTextureCopies\":0,\"outputCopies\":0,\"sentinel\":false,\"validationDispatches\":0,\"fixtureInputs\":false}\n";}else{lean.failed=true;++lean.failures;lean.log<<"{\"kind\":\"initialization_declined\"}\n";lean.log.flush();}
 }
 if(!live_fixture)return;
 if(lean.rebuild && !lean.pending && !lean.history_dirty && lean.borrow.recordings.empty() && !lean.borrow.blocked){
  if(!lean_cleanup(q,guard))return;
  lean_width=lean.next_width;lean_height=lean.next_height;lean_outwidth=lean.next_outwidth;lean_outheight=lean.next_outheight;lean.rebuild=false;
  bool success=init_live_saved(q,&guard);lean.wanted=success;lean.failed=!success;if(!success)++lean.failures;
  lean.log<<"{\"kind\":\"dimensions_rebuild_completed\",\"frame\":"<<frame<<",\"success\":"<<(success?"true":"false")<<"}\n";
 }
 if(!live_fixture)return;
 if(developer_file_exists(root/"lean-inject-failure.txt")){fs::remove(root/"lean-inject-failure.txt");lean.inject_failure=true;}
 if(developer_file_exists(root/"lean-capture.txt")){fs::remove(root/"lean-capture.txt");lean.capture_requested=true;}
 if(developer_file_exists(root/"lean-reset-pair.txt")){fs::remove(root/"lean-reset-pair.txt");lean.reset_pair_requested=true;}
 if(developer_file_exists(root/"lean-direct-output.txt")){fs::remove(root/"lean-direct-output.txt");lean.direct_output=true;live_fixture->reset_pending=true;lean.log<<"{\"kind\":\"direct_output_enabled\",\"frame\":"<<frame<<"}\n";}
 if(developer_file_exists(root/"lean-toggle.txt")){std::ifstream f(root/"lean-toggle.txt");std::string mode;f>>mode;f.close();fs::remove(root/"lean-toggle.txt");if(mode=="off" || mode=="on"){lean.wanted=mode=="on";if(lean.wanted){lean.failed=false;live_fixture->reset_pending=true;}lean.log<<"{\"kind\":\"toggle\",\"frame\":"<<frame<<",\"featurePresent\":"<<(live_fixture?"true":"false")<<",\"nativeResetBuffers\":"<<native_reset_borrows.entries.size()<<",\"nativeResetRecordedLists\":"<<native_reset_borrows.recordings.size()<<",\"nativeResetRetired\":"<<native_reset_borrows.retired<<",\"nativeResetBlocked\":"<<(native_reset_borrows.blocked?"true":"false")<<",\"on\":"<<(lean.wanted?"true":"false")<<"}\n";}}
 if(developer_file_exists(root/"lean-stop.txt")){fs::remove(root/"lean-stop.txt");lean.wanted=false;lean.stop=true;}
 static ULONGLONG status=0;
 if(now-status>(developer_controls?1000:10000) || lean.stop){status=now;lean_write_status();}
 if(lean.stop && !lean.history_dirty && lean.borrow.recordings.empty() && !lean.borrow.blocked){if(!lean_cleanup(q,guard))return;lean_write_status();lean.log<<"{\"kind\":\"stopped\",\"frame\":"<<frame<<"}\n";lean.log.flush();std::ofstream(root/label/"complete.txt")<<"Lean run stopped after native history reset and verified GPU completion.\n";}
}
