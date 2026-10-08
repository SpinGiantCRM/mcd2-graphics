// Game-specific, requested-pass observer. No shader, descriptor or image replacements.
// ReShade SDK v6.8.0; native D3D12 readbacks retain the original source state.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <reshade.hpp>
#include "ngx-research/nvsdk_ngx.h"
#include <algorithm>
#include <atomic>
#include <array>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include "dlss_model_override.hpp"

namespace a = reshade::api;
namespace fs = std::filesystem;
static std::recursive_mutex lock;
static fs::path root; // Per-user cache/diagnostics, initialized by the UI bridge.
static fs::path asset_root; // Installed beside this addon; no experiment-directory dependency.
#ifndef MCD2_ENABLE_DIAGNOSTICS
#define MCD2_ENABLE_DIAGNOSTICS 0
#endif
static constexpr bool developer_controls = MCD2_ENABLE_DIAGNOSTICS != 0;
static bool developer_file_exists(const fs::path &path){
 if constexpr(!developer_controls)return false;
 std::error_code error;return fs::exists(path,error) && !error;
}
static uint64_t frame = 0;
static std::string label;
static uint32_t remaining = 0;
static bool metadata_only = true;
static bool sequence_capture = false;
static bool copy_sequence = false;
static bool state_sequence = false;
static bool proxy_audit = false;
static bool live_saved_mode = false;
static bool live_current_mode = false;
static bool live_continuous_mode = false;
static bool live_display_mode = false;
static std::unordered_map<a::command_list*,std::vector<std::pair<uint64_t,unsigned>>> current_submissions;
static uint64_t current_submission_queue=0;
static bool current_queue_mismatch=false;
static thread_local bool internal_evaluation = false;
static void timer_begin(a::command_list*);
static void timer_reset(a::command_list*);
static void timer_close(a::command_list*);
static void timer_execute(a::command_queue*,a::command_list*);
static void timer_present(a::command_queue*);
static unsigned lean_width=2560,lean_height=1440,lean_outwidth=3840,lean_outheight=2160;
static bool lean_gate(a::command_list*,uint32_t,uint32_t,uint32_t,uint32_t);
static void lean_submission(a::command_queue*,a::command_list*);
static void lean_forget_recording(a::command_list*);
static void lean_reset_recording(a::command_list*);
static void lean_begin_recording(a::command_list*);
static void guide_probe_record(a::command_list*);
static void guide_probe_present(a::command_queue*,std::unique_lock<std::recursive_mutex>&);
static void guide_probe_submit(a::command_queue*,a::command_list*);
static void guide_probe_begin(a::command_list*);
static void guide_probe_reset(a::command_list*);
static void guide_probe_forget(a::command_list*);
static void guide_probe_destroy_queue(a::command_queue*);
static bool fsr_runtime_record(a::command_list*,uint32_t,uint32_t,uint32_t);
static void fsr_runtime_present(a::command_queue*,std::unique_lock<std::recursive_mutex>&);
static bool fsr_runtime_owns_source();
static void fsr_runtime_begin(a::command_list*);
static void fsr_runtime_reset(a::command_list*);
static void fsr_runtime_forget(a::command_list*);
static void fsr_runtime_submit(a::command_queue*,a::command_list*);
static void fsr_runtime_destroy_queue(a::command_queue*);
static bool live_saved_gate(a::command_list*,uint32_t,uint32_t,uint32_t,uint32_t);
static bool live_current_gate(a::command_list*,uint32_t,uint32_t,uint32_t,uint32_t);
static bool init_live_saved(a::command_queue*,std::unique_lock<std::recursive_mutex>* = nullptr);
static thread_local bool capture_probe_queue=false;
static a::command_queue *probe_queue_api=nullptr;
static void cleanup_live_saved(a::command_queue*);
static bool roi_sequence = false;
static ULONGLONG previous_present_ms=0;
static double frame_delta_ms=0;
static constexpr uint32_t TAA=0x03645DC9, AO=0x3143A9B9, UI=0x378DF900, TAA_SR=0xF56E10F9;
static bool is_taa(uint32_t shader){return shader==TAA || shader==TAA_SR;}

#include "reshade_public_bridge.hpp"
static bool public_bridge_layout_matches(){
 const uint32_t expected[]={sizeof(a::resource),alignof(a::resource),sizeof(a::resource_view),alignof(a::resource_view),sizeof(a::resource_view_desc),alignof(a::resource_view_desc),sizeof(a::resource_desc),alignof(a::resource_desc)};
 uint32_t actual[8]{};mcd2_reshade_bridge_layout(actual);
 return std::equal(std::begin(expected),std::end(expected),std::begin(actual));
}
static a::resource from_view(a::device*d,a::resource_view v){a::resource r{};mcd2_resource_from_view(d,v.handle,&r);return r;}
static a::resource_view_desc view_desc(a::device*d,a::resource_view v){a::resource_view_desc r{};mcd2_view_desc(d,v.handle,&r);return r;}
static a::resource_desc resource_desc(a::device*d,a::resource v){a::resource_desc r{};mcd2_resource_desc(d,v.handle,&r);return r;}
static a::resource back_buffer(a::swapchain*s){a::resource r{};mcd2_back_buffer(s,s->get_current_back_buffer_index(),&r);return r;}

struct Binding { a::descriptor_type type{}; a::resource_view view{}; a::buffer_range cb{}; };
struct Range { a::descriptor_range desc{}; };
struct Param { std::vector<Range> ranges; a::constant_range constants{}; };
struct Cmd {
 uint32_t cs=0, ps=0, vs=0;
 uint64_t pipeline=0;
 bool dynamic_offsets=false;
 bool active_compute=false;
 uint64_t compute_layout=0, graphics_layout=0;
 std::map<uint32_t,uint64_t> ct,gt;
 std::map<uint32_t,std::map<uint32_t,Binding>> cp,gp;
 std::map<uint32_t,std::vector<uint32_t>> cc,gc;
 std::unordered_map<uint64_t,a::resource_usage> states;
};
struct Pipe { uint32_t cs=0,ps=0,vs=0; };
struct HeapKey { uint64_t heap; uint32_t offset; bool operator<(const HeapKey &o) const {return heap<o.heap || (heap==o.heap && offset<o.offset);} };
static std::unordered_map<uint64_t,Pipe> pipelines;
static std::unordered_map<uint64_t,std::vector<Param>> layouts;
static std::unordered_map<a::command_list*,Cmd> commands;
static std::map<HeapKey,Binding> descriptors;
static std::unordered_map<uint64_t,a::resource_usage> initial_states;
static std::unordered_map<uint64_t,a::resource_desc> live_resources;
static std::unordered_map<uint64_t,a::resource_usage> submitted_states;
struct Copy {
 ID3D12Resource* readback=nullptr;
 a::command_list* cmd=nullptr;
 a::command_queue* queue=nullptr;
 uint64_t bytes=0;
 uint32_t rows=1, pitch=0, row_bytes=0;
 fs::path path;
};
static std::vector<Copy> copies;
static std::vector<std::string> records;
static uint64_t capture_frame=0;
static bool seen_taa=false,seen_ui=false,seen_ao=false;
static std::map<uint32_t,uint32_t> frame_compute_counts,frame_pixel_counts;
static std::map<std::tuple<bool,uint32_t,uint32_t>,std::pair<uint64_t,uint64_t>> frame_indirect_counts;
static std::map<std::string,uint64_t> frame_api_counts;
static std::map<uint64_t,uint64_t> frame_queue_lists;
static uint64_t rt_builds=0,rt_dispatches=0,rt_descriptor_updates=0,rt_pipeline_initializations=0;
static std::map<a::device*,ID3D12Resource*> roundtrip_textures;

static uint32_t crc(const void *data,size_t size) {
 static const auto table=[](){std::array<uint32_t,256> t{};for(uint32_t i=0;i<256;i++){uint32_t c=i;for(int k=0;k<8;k++)c=(c>>1)^(0xEDB88320u & (0u-(c&1u)));t[i]=c;}return t;}();
 uint32_t c=~0u;auto *bytes=static_cast<const unsigned char*>(data);
 for(size_t i=0;i<size;i++)c=(c>>8)^table[(c^bytes[i])&255u];
 return ~c;
}
static bool compute(a::shader_stage s) {return static_cast<uint32_t>(s&a::shader_stage::compute)!=0;}
static Binding unpack(const a::descriptor_table_update &u,uint32_t i) {
 Binding b;b.type=u.type;
 if(u.type==a::descriptor_type::constant_buffer) b.cb=static_cast<const a::buffer_range*>(u.descriptors)[i];
 else if(u.type==a::descriptor_type::shader_resource_view || u.type==a::descriptor_type::buffer_shader_resource_view || u.type==a::descriptor_type::unordered_access_view || u.type==a::descriptor_type::buffer_unordered_access_view) b.view=static_cast<const a::resource_view*>(u.descriptors)[i];
 return b;
}
static HeapKey heap_key(a::device *dev,a::descriptor_table t,uint32_t off) {
 a::descriptor_heap h{};uint32_t index=0;dev->get_descriptor_heap_offset(t,off,0,&h,&index);return {h.handle,index};
}
static void init_layout(a::device*,uint32_t n,const a::pipeline_layout_param *p,a::pipeline_layout l) {
 std::lock_guard guard(lock);auto &out=layouts[l.handle];out.clear();out.resize(n);
 for(uint32_t i=0;i<n;i++) {
  if(p[i].type==a::pipeline_layout_param_type::push_constants) out[i].constants=p[i].push_constants;
  else if(p[i].type==a::pipeline_layout_param_type::push_descriptors) out[i].ranges.push_back({p[i].push_descriptors});
  else if(p[i].type==a::pipeline_layout_param_type::descriptor_table || p[i].type==a::pipeline_layout_param_type::push_descriptors_with_ranges)
   for(uint32_t j=0;j<p[i].descriptor_table.count;j++) out[i].ranges.push_back({p[i].descriptor_table.ranges[j]});
  else if(p[i].type==a::pipeline_layout_param_type::descriptor_table_with_flags || p[i].type==a::pipeline_layout_param_type::push_descriptors_with_ranges_and_flags)
   for(uint32_t j=0;j<p[i].descriptor_table_with_flags.count;j++) out[i].ranges.push_back({p[i].descriptor_table_with_flags.ranges[j]});
 }
}
static void init_pipe(a::device*,a::pipeline_layout,uint32_t n,const a::pipeline_subobject *s,a::pipeline p) {
 Pipe v;
 for(uint32_t i=0;i<n;i++)if(s[i].type==a::pipeline_subobject_type::compute_shader || s[i].type==a::pipeline_subobject_type::pixel_shader || s[i].type==a::pipeline_subobject_type::vertex_shader){
  const auto &d=*static_cast<const a::shader_desc*>(s[i].data);auto h=crc(d.code,d.code_size);
  if(s[i].type==a::pipeline_subobject_type::compute_shader)v.cs=h;else if(s[i].type==a::pipeline_subobject_type::pixel_shader)v.ps=h;else v.vs=h;
 }
 std::lock_guard guard(lock);pipelines[p.handle]=v;
}
static void bind_pipe(a::command_list *cmd,a::pipeline_stage s,a::pipeline p) {
 std::lock_guard guard(lock);lean_begin_recording(cmd);timer_begin(cmd);auto it=pipelines.find(p.handle);if(it==pipelines.end())return;
 auto &c=commands[cmd];c.pipeline=p.handle;if(static_cast<uint32_t>(s&a::pipeline_stage::compute_shader)!=0)c.cs=it->second.cs;
 if(static_cast<uint32_t>(s&a::pipeline_stage::pixel_shader)!=0)c.ps=it->second.ps;
 // D3D12 SetPipelineState emits an all-stages mask even for a compute-only PSO.
 // Derive its actual type from the initialized shader subobjects, not that mask.
 if(static_cast<uint32_t>(s&a::pipeline_stage::vertex_shader)!=0)c.vs=it->second.vs;
 // A depth-only graphics PSO may have no pixel shader. Do not inherit compute state.
 c.active_compute=(it->second.cs!=0);
}
static void set_layout(Cmd &c,bool cs,uint64_t layout) {
 auto &active=cs?c.compute_layout:c.graphics_layout;
 if(active!=layout) { (cs?c.ct:c.gt).clear();(cs?c.cp:c.gp).clear();(cs?c.cc:c.gc).clear();active=layout; }
}
static void push_desc(a::command_list *cmd,a::shader_stage s,a::pipeline_layout l,uint32_t param,const a::descriptor_table_update &u) {
 std::lock_guard guard(lock);lean_begin_recording(cmd);auto &c=commands[cmd];bool cs=compute(s);set_layout(c,cs,l.handle);auto &m=cs?c.cp:c.gp;
 if(u.type==a::descriptor_type::acceleration_structure){rt_descriptor_updates+=u.count;if(remaining)frame_api_counts["AS_descriptors_pushed"]+=u.count;}
 (cs?c.ct:c.gt).erase(param);
 for(uint32_t i=0;i<u.count;i++)m[param][u.binding+i]=unpack(u,i);
}
static void bind_tables(a::command_list *cmd,a::shader_stage s,a::pipeline_layout l,uint32_t first,uint32_t n,const a::descriptor_table *t,uint32_t dynamic_count,const uint32_t*) {
 std::lock_guard guard(lock);lean_begin_recording(cmd);auto &c=commands[cmd];bool cs=compute(s);set_layout(c,cs,l.handle);auto &m=cs?c.ct:c.gt;
 if(dynamic_count)c.dynamic_offsets=true;
 for(uint32_t i=0;i<n;i++){m[first+i]=t[i].handle;(cs?c.cp:c.gp).erase(first+i);}
}
static void push_constants(a::command_list *cmd,a::shader_stage s,a::pipeline_layout l,uint32_t p,uint32_t first,uint32_t n,const void *values) {
 std::lock_guard guard(lock);lean_begin_recording(cmd);auto &c=commands[cmd];bool cs=compute(s);set_layout(c,cs,l.handle);auto &v=(cs?c.cc:c.gc)[p];
 if(first+n>4096)return;v.resize(std::max<size_t>(v.size(),first+n));std::memcpy(v.data()+first,values,n*4);
}
static bool update_desc(a::device *d,uint32_t n,const a::descriptor_table_update *u) {
 std::lock_guard guard(lock);
 for(uint32_t j=0;j<n;j++)if(u[j].type==a::descriptor_type::acceleration_structure){rt_descriptor_updates+=u[j].count;if(remaining)frame_api_counts["AS_descriptors_updated"]+=u[j].count;}
 if(remaining){++frame_api_counts["descriptor_update_calls"];for(uint32_t j=0;j<n;j++)frame_api_counts["descriptors_updated"]+=u[j].count;}
 for(uint32_t j=0;j<n;j++) for(uint32_t i=0;i<u[j].count;i++)descriptors[heap_key(d,u[j].table,u[j].binding+i)]=unpack(u[j],i);
 return false;
}
static bool copy_desc(a::device *d,uint32_t n,const a::descriptor_table_copy *x) {
 std::lock_guard guard(lock);
 if(remaining){++frame_api_counts["descriptor_copy_calls"];for(uint32_t j=0;j<n;j++)frame_api_counts["descriptors_copied"]+=x[j].count;}
 for(uint32_t j=0;j<n;j++) {
  std::vector<Binding> tmp(x[j].count);
  for(uint32_t i=0;i<x[j].count;i++){auto it=descriptors.find(heap_key(d,x[j].source_table,x[j].source_binding+i));if(it!=descriptors.end())tmp[i]=it->second;}
  for(uint32_t i=0;i<x[j].count;i++)descriptors[heap_key(d,x[j].dest_table,x[j].dest_binding+i)]=tmp[i];
 }
 return false;
}
static void init_resource(a::device*,const a::resource_desc &d,const a::subresource_data*,a::resource_usage s,a::resource r) {std::lock_guard guard(lock);initial_states[r.handle]=s;live_resources[r.handle]=d;}
static void destroy_resource(a::device*,a::resource r) {std::lock_guard guard(lock);initial_states.erase(r.handle);live_resources.erase(r.handle);submitted_states.erase(r.handle);for(auto &[cmd,c]:commands)c.states.erase(r.handle);}
static void barrier(a::command_list *cmd,uint32_t n,const a::resource *r,const a::resource_usage*,const a::resource_usage *s) {std::lock_guard guard(lock);lean_begin_recording(cmd);timer_begin(cmd);if(remaining){++frame_api_counts["barrier_event_calls"];frame_api_counts["barrier_event_resources"]+=n;}for(uint32_t i=0;i<n;i++)commands[cmd].states[r[i].handle]=s[i];}
static void reset_cmd(a::command_list *cmd) {std::lock_guard guard(lock);timer_reset(cmd);lean_reset_recording(cmd);commands.erase(cmd);}
struct Slot {uint32_t reg,space;Binding binding;uint32_t param,rangebinding;};
static std::vector<Slot> resolve(a::command_list *cmd,bool cs) {
 auto &c=commands[cmd];uint64_t l=cs?c.compute_layout:c.graphics_layout;auto li=layouts.find(l);std::vector<Slot> out;if(li==layouts.end())return out;
 auto &tables=cs?c.ct:c.gt;auto &push=cs?c.cp:c.gp;
 for(uint32_t p=0;p<li->second.size();p++)for(auto &rr:li->second[p].ranges) {
  auto &r=rr.desc;if(r.type==a::descriptor_type::sampler || static_cast<uint32_t>(r.visibility & (cs?a::shader_stage::compute:a::shader_stage::pixel))==0)continue;
  for(uint32_t i=0;i<std::min(r.count,128u);i++) {
   Binding b;bool found=false;auto pi=push.find(p);
   if(pi!=push.end()){auto bi=pi->second.find(r.binding+i);if(bi!=pi->second.end()){b=bi->second;found=true;}}
   auto ti=tables.find(p);
   if(!found && ti!=tables.end()) {auto bi=descriptors.find(heap_key(cmd->get_device(),{ti->second},r.binding+i));if(bi!=descriptors.end()){b=bi->second;found=true;}}
   if(found) {
    bool same=(b.type==r.type) || ((r.type==a::descriptor_type::shader_resource_view || r.type==a::descriptor_type::buffer_shader_resource_view) && (b.type==a::descriptor_type::shader_resource_view || b.type==a::descriptor_type::buffer_shader_resource_view)) || ((r.type==a::descriptor_type::unordered_access_view || r.type==a::descriptor_type::buffer_unordered_access_view) && (b.type==a::descriptor_type::unordered_access_view || b.type==a::descriptor_type::buffer_unordered_access_view));
    if(same)out.push_back({r.dx_register_index+i,r.dx_register_space,b,p,r.binding});
   }
  }
 }
 return out;
}
static std::string basename(uint32_t shader,uint32_t reg,a::descriptor_type type) {
 std::ostringstream s;s<<std::hex<<shader<<'-'<<((type==a::descriptor_type::constant_buffer)?'b':((type==a::descriptor_type::unordered_access_view || type==a::descriptor_type::buffer_unordered_access_view)?'u':'t'))<<std::dec<<reg;return s.str();
}
static D3D12_RESOURCE_STATES native_state(a::resource_usage state) {
 uint32_t s=static_cast<uint32_t>(state),out=s & 0x3fff;
 if(s&0x8000)out|=D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
 return static_cast<D3D12_RESOURCE_STATES>(out);
}
static void snapshot(a::command_list *cmd,const Slot &slot,uint32_t shader,bool take_bytes,const char *output_phase=nullptr) {
 auto *dev=cmd->get_device();auto &b=slot.binding;
 a::resource r=b.type==a::descriptor_type::constant_buffer?b.cb.buffer:from_view(dev,b.view);
 if(!r.handle)return;
 if(live_resources.find(r.handle)==live_resources.end()) {records.push_back("{\"kind\":\"error\",\"message\":\"resource_not_in_live_cache\",\"shader\":"+std::to_string(shader)+",\"slot\":"+std::to_string(slot.reg)+",\"type\":"+std::to_string(static_cast<uint32_t>(b.type))+"}");return;}
 {std::ofstream trace(root/"inspection-trace.txt",std::ios::app);trace<<frame<<" shader "<<std::hex<<shader<<std::dec<<" slot "<<slot.reg<<" type "<<static_cast<uint32_t>(b.type)<<" resource "<<std::hex<<r.handle<<std::dec<<'\n';}
 auto *native=reinterpret_cast<ID3D12Resource*>(r.handle);auto desc=native->GetDesc();
 const std::string name=basename(shader,slot.reg,b.type)+(output_phase?std::string("-")+output_phase:std::string());
 std::ostringstream meta;meta<<"{\"kind\":\"binding\",\"frame\":"<<frame<<",\"shader\":"<<shader<<",\"slot\":"<<slot.reg<<",\"param\":"<<slot.param<<",\"rangeBinding\":"<<slot.rangebinding<<",\"space\":"<<slot.space<<",\"type\":"<<static_cast<uint32_t>(b.type)<<",\"resource\":"<<r.handle<<",\"width\":"<<desc.Width<<",\"height\":"<<desc.Height<<",\"format\":"<<desc.Format<<",\"mips\":"<<desc.MipLevels<<",\"samples\":"<<desc.SampleDesc.Count;
 if(b.type==a::descriptor_type::constant_buffer)meta<<",\"sourceOffset\":"<<b.cb.offset;
 else {auto descriptor=view_desc(dev,b.view);meta<<",\"viewFormat\":"<<static_cast<uint32_t>(descriptor.format)<<",\"firstViewLevel\":"<<descriptor.texture.first_level<<",\"firstViewLayer\":"<<descriptor.texture.first_layer;}
 const auto emit=[&](const char *status){meta<<",\"status\":\""<<status<<"\"}";records.push_back(meta.str());};
 if(!take_bytes || metadata_only){emit("metadata_only");return;}
 if(desc.SampleDesc.Count!=1 || desc.DepthOrArraySize!=1){emit("unsupported_array_or_msaa");return;}
 if(!output_phase && (b.type==a::descriptor_type::unordered_access_view || b.type==a::descriptor_type::buffer_unordered_access_view)){emit("output_dimensions_only");return;}
 auto si=commands[cmd].states.find(r.handle);a::resource_usage state;
 if(si!=commands[cmd].states.end())state=si->second;
 else {auto it=submitted_states.find(r.handle);if(it!=submitted_states.end())state=it->second;else {auto first=initial_states.find(r.handle);if(first==initial_states.end()){emit("unknown_source_state");return;}state=first->second;}}
 auto vd=b.type==a::descriptor_type::constant_buffer?a::resource_view_desc{}:view_desc(dev,b.view);
 uint64_t offset=b.type==a::descriptor_type::constant_buffer?b.cb.offset:(desc.Dimension==D3D12_RESOURCE_DIMENSION_BUFFER?vd.buffer.offset:0),bytes=0;D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};uint32_t rows=1;uint64_t row_bytes=0;
 auto *ndev=reinterpret_cast<ID3D12Device*>(dev->get_native());
 D3D12_HEAP_PROPERTIES source_heap{};D3D12_HEAP_FLAGS source_flags{};native->GetHeapProperties(&source_heap,&source_flags);
 if(source_heap.Type!=D3D12_HEAP_TYPE_UPLOAD && si==commands[cmd].states.end() && submitted_states.find(r.handle)==submitted_states.end()){emit("unknown_current_command_list_state");return;}
 if(desc.Dimension==D3D12_RESOURCE_DIMENSION_BUFFER) {
  if(offset>=desc.Width){emit("invalid_buffer_offset");return;}
  bytes=std::min<uint64_t>(desc.Width-offset,b.type==a::descriptor_type::constant_buffer?(is_taa(shader)?(slot.reg==0?336:4096):256):64);
  if(b.type==a::descriptor_type::constant_buffer && b.cb.size!=UINT64_MAX)bytes=std::min(bytes,b.cb.size);
  row_bytes=bytes;
 } else {
  if(vd.texture.first_level!=0 || vd.texture.first_layer!=0){emit("nonzero_view_subresource");return;}
  // Depth/stencil copies must cover their complete subresource.
  if(roi_sequence && shader==TAA && slot.reg!=2) {desc.Width=768;desc.Height=768;meta<<",\"capturedRegion\":[448,384,1216,1152]";}
  ndev->GetCopyableFootprints(&desc,0,1,0,&footprint,&rows,&row_bytes,&bytes);
 }
 if(!bytes || bytes>128ull*1024*1024){emit("size_limit");return;}
 D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_READBACK;
 D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=bytes;rd.Height=1;rd.DepthOrArraySize=1;rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
 ID3D12Resource *dest=nullptr;HRESULT hr=ndev->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,__uuidof(ID3D12Resource),reinterpret_cast<void**>(&dest));
 if(FAILED(hr)){meta<<",\"hresult\":"<<hr;emit("allocation_failed");return;}
 auto *list=reinterpret_cast<ID3D12GraphicsCommandList*>(cmd->get_native());
 D3D12_RESOURCE_BARRIER br{};br.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;br.Transition.pResource=native;br.Transition.Subresource=0;br.Transition.StateBefore=native_state(state);br.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;
 bool change=source_heap.Type!=D3D12_HEAP_TYPE_UPLOAD && !(native_state(state)&D3D12_RESOURCE_STATE_COPY_SOURCE);
 if(change)list->ResourceBarrier(1,&br);
 if(desc.Dimension==D3D12_RESOURCE_DIMENSION_BUFFER)list->CopyBufferRegion(dest,0,native,offset,bytes);
 else {D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=native;src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;src.SubresourceIndex=0;dst.pResource=dest;dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=footprint;D3D12_BOX box{448,384,0,1216,1152,1};list->CopyTextureRegion(&dst,0,0,0,&src,roi_sequence && shader==TAA && slot.reg!=2?&box:nullptr);}
 if(change){std::swap(br.Transition.StateBefore,br.Transition.StateAfter);list->ResourceBarrier(1,&br);}
 auto filename=std::to_string(frame)+"-"+name+".bin";
 copies.push_back({dest,cmd,nullptr,bytes,rows,desc.Dimension==D3D12_RESOURCE_DIMENSION_BUFFER?static_cast<uint32_t>(bytes):footprint.Footprint.RowPitch,static_cast<uint32_t>(row_bytes),root/label/filename});
 meta<<",\"copiedOffset\":"<<offset<<",\"stateOrigin\":\""<<(si!=commands[cmd].states.end()?"current_list":(source_heap.Type==D3D12_HEAP_TYPE_UPLOAD?"upload_heap":"submitted_predecessor"))<<"\",\"file\":\""<<filename<<"\",\"rowPitch\":"<<copies.back().pitch<<",\"rowBytes\":"<<row_bytes<<",\"rows\":"<<rows<<",\"bytes\":"<<bytes<<",\"footprintFormat\":"<<footprint.Footprint.Format<<",\"sourceState\":"<<static_cast<uint32_t>(state);emit("copy_recorded_not_yet_submitted");
}
static void observe(a::command_list *cmd,uint32_t shader,bool cs) {
 if(!remaining)return;
 bool &seen=is_taa(shader)?seen_taa:(shader==AO?seen_ao:seen_ui);if(seen)return;seen=true;
 auto slots=resolve(cmd,cs);if(slots.empty())records.push_back("{\"kind\":\"error\",\"message\":\"no_resolved_bindings\",\"shader\":"+std::to_string(shader)+"}");
 for(auto &s:slots) {
  bool iscb=s.binding.type==a::descriptor_type::constant_buffer;
  bool issrv=s.binding.type==a::descriptor_type::shader_resource_view || s.binding.type==a::descriptor_type::buffer_shader_resource_view;
  bool isuav=s.binding.type==a::descriptor_type::unordered_access_view || s.binding.type==a::descriptor_type::buffer_unordered_access_view;
  bool used=s.space==0 && ((is_taa(shader) && ((iscb && s.reg<2)||(issrv && s.reg<6)||(isuav && s.reg==0))) || (shader==AO && ((iscb && s.reg<3)||(issrv && s.reg<6)||(isuav && s.reg<3))) || (shader==UI && ((iscb && s.reg==0)||(issrv && s.reg<2))));
  if(!used)continue;
  bool bytes=s.space==0 && ((is_taa(shader) && ((iscb && s.reg<2)||(issrv && (s.reg<4 || (sequence_capture && s.reg==5))))) || (shader==UI && !sequence_capture && capture_frame==frame && issrv && s.reg<2));
  if(roi_sequence && shader==TAA && issrv && s.reg==2)bytes=(frame==capture_frame || frame==capture_frame+8);
  snapshot(cmd,s,shader,bytes && !state_sequence && (!live_saved_mode || live_current_mode));
 }
 auto &c=commands[cmd];auto &consts=cs?c.cc:c.gc;
 for(auto &[param,v]:consts) {std::ostringstream m;m<<"{\"kind\":\"root_constants\",\"frame\":"<<frame<<",\"shader\":"<<shader<<",\"param\":"<<param<<",\"words\":[";for(size_t i=0;i<v.size();i++){if(i)m<<',';m<<v[i];}m<<"]}";records.push_back(m.str());}
}
// Bounded native-AA state-restoration gate. Both native dispatches use identical
// read-only inputs and the same output UAV. No NGX result is displayed.
static bool root_push_supported(const std::map<uint32_t,std::map<uint32_t,Binding>> &push) {
 for(const auto &[param,bindings]:push)for(const auto &[reg,b]:bindings)
  if(b.type!=a::descriptor_type::constant_buffer && b.type!=a::descriptor_type::buffer_shader_resource_view && b.type!=a::descriptor_type::buffer_unordered_access_view)return false;
 return true;
}
static void restore_root(a::command_list *cmd,const Cmd &state,bool cs,bool force_layout) {
 a::shader_stage stages=cs?a::shader_stage::all_compute:a::shader_stage::all_graphics;
 a::pipeline_layout layout{cs?state.compute_layout:state.graphics_layout};
 if(!layout.handle)return;
 if(force_layout)cmd->bind_descriptor_tables(stages,layout,0,0,nullptr);
 for(const auto &[param,table]:(cs?state.ct:state.gt)){a::descriptor_table value{table};cmd->bind_descriptor_tables(stages,layout,param,1,&value);}
 for(const auto &[param,bindings]:(cs?state.cp:state.gp))for(const auto &[reg,b]:bindings){
  a::descriptor_table_update update{};update.binding=reg;update.count=1;update.type=b.type;
  update.descriptors=b.type==a::descriptor_type::constant_buffer?static_cast<const void*>(&b.cb):static_cast<const void*>(&b.view);
  cmd->push_descriptors(stages,layout,param,update);
 }
 for(const auto &[param,values]:(cs?state.cc:state.gc))if(!values.empty())cmd->push_constants(stages,layout,param,0,static_cast<uint32_t>(values.size()),values.data());
}
// Private, version-pinned ABI diagnostic. ReShade 6.8.0's game command-list
// object inherits the COM interface first and the API command_list second.
// This does not apply to its standalone/immediate command_list_impl objects.
// The downcast is used only at a verified game AA dispatch, and its result
// must round-trip through both public COM GUIDs to the exact original object.
// It is not a portable production SDK accessor.
struct ObserverProxyLayout : ID3D12GraphicsCommandList, a::command_list {};
static ID3D12GraphicsCommandList *validated_game_proxy(a::command_list *cmd) {
 static constexpr GUID proxy_id={0x479b29e3,0x9a2c,0x11d0,{0xb6,0x96,0x00,0xa0,0xc9,0x03,0x48,0x7a}};
 static constexpr GUID unwrap_id={0x7f2c9a11,0x3b4e,0x4d6a,{0x81,0x2f,0x5e,0x9c,0xd3,0x7a,0x1b,0x42}};
 auto *candidate=static_cast<ID3D12GraphicsCommandList*>(static_cast<ObserverProxyLayout*>(cmd));
 ID3D12GraphicsCommandList *proxy=nullptr,*original=nullptr;
 if(FAILED(candidate->QueryInterface(proxy_id,reinterpret_cast<void**>(&proxy))) || !proxy)return nullptr;
 bool valid=SUCCEEDED(proxy->QueryInterface(unwrap_id,reinterpret_cast<void**>(&original))) && original && reinterpret_cast<uint64_t>(original)==cmd->get_native();
 if(original)original->Release();
 if(!valid){proxy->Release();return nullptr;}
 return proxy;
}
static void audit_proxy(a::command_list *cmd) {
 auto *proxy=validated_game_proxy(cmd);bool device_matches=false,wrapped=false;
 if(proxy){
  ID3D12Device *device=nullptr,*raw=nullptr;
  if(SUCCEEDED(proxy->GetDevice(IID_PPV_ARGS(&device))) && device){
   static constexpr GUID unwrap_id={0x7f2c9a11,0x3b4e,0x4d6a,{0x81,0x2f,0x5e,0x9c,0xd3,0x7a,0x1b,0x42}};
   wrapped=SUCCEEDED(device->QueryInterface(unwrap_id,reinterpret_cast<void**>(&raw)));
   device_matches=raw && reinterpret_cast<uint64_t>(raw)==cmd->get_device()->get_native();
   if(raw)raw->Release();device->Release();
  }
  proxy->Release();
 }
 records.push_back("{\"kind\":\"proxy_audit\",\"frame\":"+std::to_string(frame)+",\"proxyRecoveredAndNativeIdentityMatched\":"+(proxy?"true":"false")+",\"getDeviceReturnsWrapped\":"+(wrapped?"true":"false")+",\"deviceNativeIdentityMatched\":"+(device_matches?"true":"false")+",\"method\":\"private_reshade_6_8_game_object_layout\",\"GPUCommandsChanged\":false}");
}
static bool state_gate(a::command_list *cmd,uint32_t shader,uint32_t x,uint32_t y,uint32_t z) {
 const Cmd saved=commands[cmd];
 if(!saved.pipeline || !saved.compute_layout || saved.ct.empty() || saved.dynamic_offsets || !root_push_supported(saved.cp) || !root_push_supported(saved.gp)){
  records.push_back("{\"kind\":\"error\",\"message\":\"state_gate_restore_contract_incomplete\"}");return false;
 }
 auto slots=resolve(cmd,true);auto out=std::find_if(slots.begin(),slots.end(),[](const Slot &s){return s.space==0 && s.reg==0 && s.binding.type==a::descriptor_type::unordered_access_view;});
 if(out==slots.end())return false;
 auto resource=from_view(cmd->get_device(),out->binding.view);auto *source=reinterpret_cast<ID3D12Resource*>(resource.handle);auto desc=source->GetDesc();
 if(desc.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT || desc.Width!=3840 || desc.Height!=2160)return false;
 for(const auto &s:slots)if(s.space==0 && s.reg<6 && (s.binding.type==a::descriptor_type::shader_resource_view || s.binding.type==a::descriptor_type::buffer_shader_resource_view))
  if(from_view(cmd->get_device(),s.binding.view).handle==resource.handle){records.push_back("{\"kind\":\"error\",\"message\":\"state_gate_input_output_alias\"}");return false;}
 auto *native=reinterpret_cast<ID3D12GraphicsCommandList*>(cmd->get_native());
 native->Dispatch(x,y,z);commands[cmd].states[resource.handle]=a::resource_usage::unordered_access;
 D3D12_RESOURCE_BARRIER order{};order.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;order.UAV.pResource=source;native->ResourceBarrier(1,&order);
 snapshot(cmd,*out,shader,true,"before-state");
 native->SetComputeRootSignature(nullptr);native->SetDescriptorHeaps(0,nullptr);
 // count=0 explicitly forces the cached ReShade root signature and heap pair
 // back onto the native list. A normal table bind can incorrectly skip this
 // when an injected native call bypassed the ReShade state cache.
 restore_root(cmd,saved,true,true);restore_root(cmd,saved,false,false);
 cmd->bind_pipeline(a::pipeline_stage::all_compute,{saved.pipeline});
 native->Dispatch(x,y,z);native->ResourceBarrier(1,&order);
 snapshot(cmd,*out,shader,true,"after-state");
 records.push_back("{\"kind\":\"state_roundtrip\",\"frame\":"+std::to_string(frame)+",\"nativeDispatches\":2,\"computeRootCleared\":true,\"descriptorHeapsCleared\":true,\"computeTables\":"+std::to_string(saved.ct.size())+",\"graphicsTables\":"+std::to_string(saved.gt.size())+",\"rootPushParameters\":"+std::to_string(saved.cp.size())+",\"NGXEvaluated\":false}");
 return true;
}
static bool dispatch(a::command_list *cmd,uint32_t x,uint32_t y,uint32_t z) {
 if(internal_evaluation)return false;
 std::lock_guard guard(lock);lean_begin_recording(cmd);auto shader=commands[cmd].cs;bool first_taa=!seen_taa;
 if(is_taa(shader) && first_taa){seen_taa=true;if(fsr_runtime_record(cmd,x,y,z))return true;guide_probe_record(cmd);return lean_gate(cmd,shader,x,y,z);}
 return false;
 // Historical diagnostic dispatch routes below are inactive in the lean build.
 if(remaining)++frame_compute_counts[shader];if(is_taa(shader) || shader==AO)observe(cmd,shader,true);
 if(remaining && shader==TAA_SR)records.push_back("{\"kind\":\"sr_dispatch\",\"frame\":"+std::to_string(frame)+",\"groups\":["+std::to_string(x)+","+std::to_string(y)+","+std::to_string(z)+"]}");
 if(remaining && proxy_audit && is_taa(shader) && first_taa)audit_proxy(cmd);
 if(remaining && live_current_mode && is_taa(shader) && first_taa)return live_current_gate(cmd,shader,x,y,z);
 if(remaining && live_saved_mode && is_taa(shader) && first_taa)return live_saved_gate(cmd,shader,x,y,z);
 if(remaining && state_sequence && is_taa(shader) && first_taa)return state_gate(cmd,shader,x,y,z);
 if(!remaining || !copy_sequence || shader!=TAA || !first_taa)return false;
 // The original dispatch is forwarded once. Copies/barriers do not alter pipeline/root state.
 auto slots=resolve(cmd,true);auto found=std::find_if(slots.begin(),slots.end(),[](const Slot &s){return s.space==0 && s.reg==0 && s.binding.type==a::descriptor_type::unordered_access_view;});
 if(found==slots.end()){records.push_back("{\"kind\":\"error\",\"message\":\"roundtrip_output_missing\"}");return false;}
 auto resource=from_view(cmd->get_device(),found->binding.view);auto *source=reinterpret_cast<ID3D12Resource*>(resource.handle);auto desc=source->GetDesc();
 auto state=commands[cmd].states.find(resource.handle);
 auto predecessor=submitted_states.find(resource.handle);auto initial=initial_states.find(resource.handle);
 a::resource_usage source_state=state!=commands[cmd].states.end()?state->second:(predecessor!=submitted_states.end()?predecessor->second:a::resource_usage::undefined);
 if(desc.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT || desc.Width!=3840 || desc.Height!=2160 || !(desc.Flags&D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)){records.push_back("{\"kind\":\"error\",\"message\":\"roundtrip_precondition_failed\"}");return false;}
 auto *device=cmd->get_device();auto &owned=roundtrip_textures[device];
 if(!owned) {
  D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;desc.Flags=D3D12_RESOURCE_FLAG_NONE;
  auto *native_device=reinterpret_cast<ID3D12Device*>(device->get_native());
  if(FAILED(native_device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&owned)))){records.push_back("{\"kind\":\"error\",\"message\":\"roundtrip_allocation_failed\"}");return false;}
 }
 auto *list=reinterpret_cast<ID3D12GraphicsCommandList*>(cmd->get_native());list->Dispatch(x,y,z);
 // A successfully executed original TAA dispatch requires its bound output in UAV state.
 // A predecessor transition can live in another list of the same not-yet-submitted batch;
 // the previous submitted state alone cannot describe the post-dispatch output.
 commands[cmd].states[resource.handle]=a::resource_usage::unordered_access;
 D3D12_RESOURCE_BARRIER order{};order.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;order.UAV.pResource=source;list->ResourceBarrier(1,&order);
 snapshot(cmd,*found,TAA,true,"before-copy");
 auto transition=[&](ID3D12Resource *r,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};list->ResourceBarrier(1,&b);};
 transition(source,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);list->CopyResource(owned,source);
 transition(owned,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COPY_SOURCE);transition(source,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COPY_DEST);list->CopyResource(source,owned);
 transition(owned,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COPY_DEST);transition(source,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 snapshot(cmd,*found,TAA,true,"after-copy");
 records.push_back("{\"kind\":\"roundtrip\",\"frame\":"+std::to_string(frame)+",\"originalDispatches\":1,\"pipelineStateChanged\":false,\"sourceStateRestored\":true,\"stateAuthority\":\"original_dispatch_uav_output_contract\",\"priorTrackedState\":"+std::to_string(static_cast<uint32_t>(source_state))+"}");
 return true;
}
static bool indirect(a::command_list *cmd,a::indirect_command type,a::resource,uint64_t,uint32_t count,uint32_t) {
 std::lock_guard guard(lock);if(!remaining)return false;
 // ReShade 6.8.0 reports D3D12 ExecuteIndirect as unknown: classify by the bound PSO stage.
 bool cs=type==a::indirect_command::dispatch || (type==a::indirect_command::unknown && commands[cmd].active_compute);
 auto &census=frame_indirect_counts[{cs,cs?commands[cmd].cs:commands[cmd].ps,cs?0:commands[cmd].vs}];++census.first;census.second+=count;
 if(type==a::indirect_command::dispatch || (type==a::indirect_command::unknown && commands[cmd].active_compute)){frame_compute_counts[commands[cmd].cs]+=count;if(commands[cmd].cs==AO)observe(cmd,AO,true);}
 else frame_pixel_counts[commands[cmd].ps]+=count;
 return false;
}
static bool draw(a::command_list *cmd,uint32_t,uint32_t,uint32_t,uint32_t) {std::lock_guard guard(lock);lean_begin_recording(cmd);auto shader=commands[cmd].ps;if(remaining)++frame_pixel_counts[shader];if(shader==UI)observe(cmd,UI,false);return false;}
static bool draw_indexed(a::command_list *cmd,uint32_t,uint32_t,uint32_t,int32_t,uint32_t) {return draw(cmd,0,0,0,0);}
static void execute(a::command_queue *q,a::command_list *cmd) {std::lock_guard guard(lock);
 timer_execute(q,cmd);guide_probe_submit(q,cmd);fsr_runtime_submit(q,cmd);lean_submission(q,cmd);
 auto pending=current_submissions.find(cmd);if(pending!=current_submissions.end()){
  auto queue=q->get_native();if(!current_submission_queue)current_submission_queue=queue;bool same=queue==current_submission_queue;if(!same)current_queue_mismatch=true;
  for(auto [submitted_frame,index]:pending->second)records.push_back("{\"kind\":\"live_queue_submission\",\"frame\":"+std::to_string(submitted_frame)+",\"index\":"+std::to_string(index)+",\"queue\":"+std::to_string(queue)+",\"sameIntegrationQueue\":"+std::string(same?"true":"false")+"}");current_submissions.erase(pending);
 }
if(remaining)++frame_queue_lists[q->get_native()];for(auto &[resource,state]:commands[cmd].states)submitted_states[resource]=state;for(auto &c:copies)if(c.cmd==cmd)c.queue=q;}
static bool observe_as_build(a::command_list*,a::acceleration_structure_type type,a::acceleration_structure_build_flags flags,uint32_t count,const a::acceleration_structure_build_input*,a::resource,uint64_t,a::resource_view,a::resource_view,a::acceleration_structure_build_mode mode){
 std::lock_guard guard(lock);++rt_builds;if(remaining)++frame_api_counts["AS_build_commands_type_"+std::to_string(static_cast<uint32_t>(type))+"_mode_"+std::to_string(static_cast<uint32_t>(mode))];
 std::ofstream(root/"rt-lifecycle.jsonl",std::ios::app)<<"{\"kind\":\"AS_build\",\"frame\":"<<frame<<",\"type\":"<<static_cast<uint32_t>(type)<<",\"mode\":"<<static_cast<uint32_t>(mode)<<",\"flags\":"<<static_cast<uint32_t>(flags)<<",\"inputCount\":"<<count<<"}\n";return false;
}
static bool observe_rays(a::command_list*,a::resource,uint64_t,uint64_t,a::resource,uint64_t,uint64_t,uint64_t,a::resource,uint64_t,uint64_t,uint64_t,a::resource,uint64_t,uint64_t,uint64_t,uint32_t width,uint32_t height,uint32_t depth){
 std::lock_guard guard(lock);++rt_dispatches;if(remaining)++frame_api_counts["DispatchRays_commands"];
 std::ofstream(root/"rt-lifecycle.jsonl",std::ios::app)<<"{\"kind\":\"DispatchRays\",\"frame\":"<<frame<<",\"dimensions\":["<<width<<','<<height<<','<<depth<<"]}\n";return false;
}
static bool begin_query(a::command_list*,a::query_heap,a::query_type type,uint32_t) {std::lock_guard guard(lock);if(remaining)++frame_api_counts["query_begin_type_"+std::to_string(static_cast<uint32_t>(type))];return false;}
static bool end_query(a::command_list*,a::query_heap,a::query_type type,uint32_t) {std::lock_guard guard(lock);if(remaining)++frame_api_counts["query_end_type_"+std::to_string(static_cast<uint32_t>(type))];return false;}
static bool copy_query_results(a::command_list*,a::query_heap,a::query_type type,uint32_t,uint32_t count,a::resource,uint64_t,uint32_t) {std::lock_guard guard(lock);if(remaining){++frame_api_counts["query_resolve_calls_type_"+std::to_string(static_cast<uint32_t>(type))];frame_api_counts["queries_resolved_type_"+std::to_string(static_cast<uint32_t>(type))]+=count;}return false;}
static bool get_query_results(a::device*,a::query_heap,a::query_type type,uint32_t,uint32_t count,void*,uint32_t) {std::lock_guard guard(lock);if(remaining){++frame_api_counts["query_cpu_read_calls_type_"+std::to_string(static_cast<uint32_t>(type))];frame_api_counts["queries_cpu_requested_type_"+std::to_string(static_cast<uint32_t>(type))]+=count;}return false;}
static bool copy_buffer(a::command_list*,a::resource,uint64_t,a::resource,uint64_t,uint64_t bytes) {std::lock_guard guard(lock);if(remaining){++frame_api_counts["buffer_copy_calls"];if(bytes!=UINT64_MAX)frame_api_counts["buffer_copy_explicit_bytes"]+=bytes;else ++frame_api_counts["buffer_copy_unspecified_size_calls"];}return false;}
static bool copy_texture(a::command_list*,a::resource,uint32_t,const a::subresource_box*,a::resource,uint32_t,const a::subresource_box*,a::filter_mode) {std::lock_guard guard(lock);if(remaining)++frame_api_counts["texture_region_copy_calls"];return false;}

// Public proxy-unwrapping GUID and device proxy private-data GUID pinned to
// ReShade v6.8.0 sources. CUDA descriptor hooks need wrapped heap creation.
static constexpr GUID observer_unwrapped={0x7f2c9a11,0x3b4e,0x4d6a,{0x81,0x2f,0x5e,0x9c,0xd3,0x7a,0x1b,0x42}};
static constexpr GUID observer_device_proxy={0x2523aff4,0x978b,0x4939,{0xba,0x16,0x8e,0xe8,0x76,0xa4,0xcb,0x2a}};
static void submit_probe(a::command_queue *queue,ID3D12GraphicsCommandList *cmd) {
 ID3D12CommandList *unwrapped=nullptr;
 if(FAILED(cmd->QueryInterface(observer_unwrapped,reinterpret_cast<void**>(&unwrapped)))){unwrapped=cmd;unwrapped->AddRef();}
 auto *q=reinterpret_cast<ID3D12CommandQueue*>(queue->get_native());q->ExecuteCommandLists(1,&unwrapped);queue->wait_idle();unwrapped->Release();
}
#include "ngx_saved_eval.hpp"
// Allocation-only DLAA feature test on an owned command list. No evaluation,
// no game resources supplied, and no output enters the game's render graph.
static void probe_feature(HMODULE module,ID3D12Device *native,a::command_queue *queue,std::ofstream &log,bool run_eval,bool gpu_motion=false) {
 using Allocate=NVSDK_NGX_Result(*)(NVSDK_NGX_Parameter**);
 using Destroy=NVSDK_NGX_Result(*)(NVSDK_NGX_Parameter*);
 using Create=NVSDK_NGX_Result(*)(ID3D12GraphicsCommandList*,NVSDK_NGX_Feature,NVSDK_NGX_Parameter*,NVSDK_NGX_Handle**);
 using Release=NVSDK_NGX_Result(*)(NVSDK_NGX_Handle*);
 auto allocate=reinterpret_cast<Allocate>(GetProcAddress(module,"NVSDK_NGX_D3D12_AllocateParameters"));
 auto destroy=reinterpret_cast<Destroy>(GetProcAddress(module,"NVSDK_NGX_D3D12_DestroyParameters"));
 auto create=reinterpret_cast<Create>(GetProcAddress(module,"NVSDK_NGX_D3D12_CreateFeature"));
 auto release=reinterpret_cast<Release>(GetProcAddress(module,"NVSDK_NGX_D3D12_ReleaseFeature"));
 if(!allocate || !destroy || !create || !release){log<<"{\"stage\":\"feature_exports\",\"status\":\"missing\"}\n";return;}
 auto *q=reinterpret_cast<ID3D12CommandQueue*>(queue->get_native());
 if(q->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT){log<<"{\"stage\":\"feature_queue\",\"status\":\"not_direct\"}\n";return;}
 NVSDK_NGX_Parameter *parameters=nullptr;auto allocated=allocate(&parameters);
 log<<"{\"stage\":\"feature_parameters\",\"result\":"<<static_cast<uint32_t>(allocated)<<"}\n";
 if(NVSDK_NGX_FAILED(allocated) || !parameters)return;
 auto table=*reinterpret_cast<void***>(parameters);
 using SetUI=void(*)(NVSDK_NGX_Parameter*,const char*,unsigned int);using SetI=void(*)(NVSDK_NGX_Parameter*,const char*,int);
 auto ui=reinterpret_cast<SetUI>(table[4]);auto integer=reinterpret_cast<SetI>(table[3]);
 ui(parameters,NVSDK_NGX_Parameter_CreationNodeMask,1);ui(parameters,NVSDK_NGX_Parameter_VisibilityNodeMask,1);
 auto [input_width,input_height]=saved_input_size();
 ui(parameters,NVSDK_NGX_Parameter_Width,input_width);ui(parameters,NVSDK_NGX_Parameter_Height,input_height);
 ui(parameters,NVSDK_NGX_Parameter_OutWidth,3840);ui(parameters,NVSDK_NGX_Parameter_OutHeight,2160);
 integer(parameters,NVSDK_NGX_Parameter_PerfQualityValue,input_width==3840?NVSDK_NGX_PerfQuality_Value_DLAA:NVSDK_NGX_PerfQuality_Value_MaxQuality);
 int feature_flags=NVSDK_NGX_DLSS_Feature_Flags_IsHDR|NVSDK_NGX_DLSS_Feature_Flags_MVLowRes|NVSDK_NGX_DLSS_Feature_Flags_DepthInverted;
 if(saved_exposure_mode()=="auto")feature_flags|=NVSDK_NGX_DLSS_Feature_Flags_AutoExposure;
 integer(parameters,NVSDK_NGX_Parameter_DLSS_Feature_Create_Flags,feature_flags);
 log<<"{\"stage\":\"exposure_mode\",\"mode\":\""<<saved_exposure_mode()<<"\",\"featureFlags\":"<<feature_flags<<"}\n";
 integer(parameters,NVSDK_NGX_Parameter_DLSS_Enable_Output_Subrects,0);
 ID3D12CommandAllocator *allocator=nullptr;ID3D12GraphicsCommandList *cmd=nullptr;
 auto hr=native->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator));
 if(SUCCEEDED(hr))hr=native->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator,nullptr,IID_PPV_ARGS(&cmd));
 log<<"{\"stage\":\"feature_command_list\",\"hresult\":"<<static_cast<uint32_t>(hr)<<"}\n";
 NVSDK_NGX_Handle *feature=nullptr;
 if(SUCCEEDED(hr)) {
  log<<"{\"stage\":\"create_super_sampling_begin\",\"inputWidth\":"<<input_width<<",\"inputHeight\":"<<input_height<<",\"outputWidth\":3840,\"outputHeight\":2160,\"evaluated\":false}\n";
  log<<"{\"stage\":\"feature_dimensions\",\"inputWidth\":"<<input_width<<",\"inputHeight\":"<<input_height<<",\"outputWidth\":3840,\"outputHeight\":2160,\"dlaa\":"<<(input_width==3840?"true":"false")<<"}\n";
  auto created=create(cmd,NVSDK_NGX_Feature_SuperSampling,parameters,&feature);
  log<<"{\"stage\":\"create_super_sampling\",\"result\":"<<static_cast<uint32_t>(created)<<",\"handleReturned\":"<<(feature?"true":"false")<<"}\n";
  hr=cmd->Close();
  if(SUCCEEDED(hr))submit_probe(queue,cmd);
  log<<"{\"stage\":\"feature_submission\",\"hresult\":"<<static_cast<uint32_t>(hr)<<"}\n";
  if(feature && NVSDK_NGX_SUCCEED(created) && SUCCEEDED(hr) && run_eval)evaluate_saved(module,native,queue,parameters,feature,log,gpu_motion);
  if(feature){auto freed=release(feature);log<<"{\"stage\":\"release_feature\",\"result\":"<<static_cast<uint32_t>(freed)<<"}\n";}
 }
 if(cmd)cmd->Release();if(allocator)allocator->Release();
 auto freed=destroy(parameters);log<<"{\"stage\":\"feature_destroy_parameters\",\"result\":"<<static_cast<uint32_t>(freed)<<"}\n";
}
static std::mutex ngx_log_lock;
static void NVSDK_CONV ngx_log_callback(const char *message,NVSDK_NGX_Logging_Level level,NVSDK_NGX_Feature component) {
 std::lock_guard guard(ngx_log_lock);std::ofstream out(root/"ngx-runtime-private.log",std::ios::app);
 out<<"level="<<static_cast<int>(level)<<" component="<<static_cast<int>(component)<<" "<<(message?message:"")<<'\n';
}
// One-shot driver capability probe. No feature is created or evaluated.
static void probe_ngx(a::command_queue *queue,const std::string &mode) {
 auto *device=queue->get_device();bool query=mode!="ngx-initonly";
 fs::create_directories(root/"ngx-cache");std::ofstream log(root/"ngx-probe.jsonl",std::ios::app);log.setf(std::ios::unitbuf);
 HMODULE module=LoadLibraryW(L"nvngx.dll");
 if(!module)module=LoadLibraryExW(L"Z:\\run\\host\\usr\\lib\\nvidia\\wine\\nvngx.dll",nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
 if(!module){log<<"{\"stage\":\"load_driver_ngx\",\"win32Error\":"<<GetLastError()<<"}\n";return;}
 // Driver export order differs from the linked SDK wrapper: version precedes info.
 // Signature confirmed against OptiScaler's NVNGX_Proxy.h implementation.
 using Init=NVSDK_NGX_Result(*)(const char*,NVSDK_NGX_EngineType,const char*,const wchar_t*,ID3D12Device*,NVSDK_NGX_Version,const NVSDK_NGX_FeatureCommonInfo*);
 using Caps=NVSDK_NGX_Result(*)(NVSDK_NGX_Parameter**);
 using Destroy=NVSDK_NGX_Result(*)(NVSDK_NGX_Parameter*);
 using Shutdown=NVSDK_NGX_Result(*)();
 auto init=reinterpret_cast<Init>(GetProcAddress(module,"NVSDK_NGX_D3D12_Init_ProjectID"));
 auto caps=reinterpret_cast<Caps>(GetProcAddress(module,"NVSDK_NGX_D3D12_GetCapabilityParameters"));
 auto destroy=reinterpret_cast<Destroy>(GetProcAddress(module,"NVSDK_NGX_D3D12_DestroyParameters"));
 auto shutdown=reinterpret_cast<Shutdown>(GetProcAddress(module,"NVSDK_NGX_D3D12_Shutdown"));
 if(!init || !caps || !destroy || !shutdown){log<<"{\"stage\":\"resolve_exports\",\"status\":\"missing_export\"}\n";FreeLibrary(module);return;}
 auto *native=reinterpret_cast<ID3D12Device*>(device->get_native());
 ID3D12Device *proxy=nullptr;UINT proxy_size=sizeof(proxy);bool retain_proxy=false;
 if(SUCCEEDED(native->GetPrivateData(observer_device_proxy,&proxy_size,&proxy)) && proxy && proxy_size==sizeof(proxy)){proxy->AddRef();native=proxy;retain_proxy=true;}
 log<<"{\"stage\":\"device_route\",\"wrapped\":"<<(retain_proxy?"true":"false")<<"}\n";
 const wchar_t *paths[]={L"C:\\renodx-observer\\ngx-runtime",L"Z:\\run\\host\\usr\\lib\\nvidia\\wine"};NVSDK_NGX_FeatureCommonInfo info{};info.PathListInfo.Path=paths;info.PathListInfo.Length=2;info.LoggingInfo.LoggingCallback=ngx_log_callback;info.LoggingInfo.MinimumLoggingLevel=NVSDK_NGX_LOGGING_LEVEL_ON;info.LoggingInfo.DisableOtherLoggingSinks=true;
 auto result=init("27fe0b1b-1112-4466-b36e-dd330014e2ad",NVSDK_NGX_ENGINE_TYPE_CUSTOM,"mcd2-observer-0.1",L"C:\\renodx-observer\\ngx-cache",native,NVSDK_NGX_Version_API,&info);
 log<<"{\"stage\":\"init\",\"result\":"<<static_cast<uint32_t>(result)<<",\"sdkVersion\":"<<static_cast<uint32_t>(NVSDK_NGX_Version_API)<<"}\n";log.flush();
 if(NVSDK_NGX_SUCCEED(result)) {
  if(query) {
  NVSDK_NGX_Parameter *parameters=nullptr;auto capresult=caps(&parameters);log<<"{\"stage\":\"capabilities\",\"result\":"<<static_cast<uint32_t>(capresult)<<"}\n";
  if(NVSDK_NGX_SUCCEED(capresult) && parameters) {
   // MSVC reverses each virtual overload group. An owned clang-cl COFF test
   // verified that ordering; SDK Get(int*) occupies index 11, Get(uint*) 12.
   using GetInt=NVSDK_NGX_Result(*)(NVSDK_NGX_Parameter*,const char*,int*);auto table=*reinterpret_cast<void***>(parameters);auto get=reinterpret_cast<GetInt>(table[11]);
   for(const char *key:{NVSDK_NGX_Parameter_SuperSampling_Available,NVSDK_NGX_Parameter_SuperSampling_NeedsUpdatedDriver,NVSDK_NGX_Parameter_SuperSampling_FeatureInitResult}) {int value=-1;auto got=get(parameters,key,&value);log<<"{\"stage\":\"parameter\",\"key\":\""<<key<<"\",\"result\":"<<static_cast<uint32_t>(got)<<",\"value\":"<<value<<"}\n";}
   log<<"{\"stage\":\"destroy_begin\"}\n";
   auto freed=destroy(parameters);log<<"{\"stage\":\"destroy_parameters\",\"result\":"<<static_cast<uint32_t>(freed)<<"}\n";
  }
  }
  if(mode=="ngx-create" || mode=="ngx-eval" || mode=="ngx-gpu-eval"){Sleep(500);probe_feature(module,native,queue,log,mode!="ngx-create",mode=="ngx-gpu-eval");}
  log<<"{\"stage\":\"shutdown_begin\",\"scope\":\"legacy_global\",\"capabilityQuery\":"<<(query?"true":"false")<<"}\n";
  auto ended=shutdown();log<<"{\"stage\":\"shutdown\",\"result\":"<<static_cast<uint32_t>(ended)<<"}\n";
 }
 FreeLibrary(module);if(retain_proxy)proxy->Release();
}
#include "ngx_live_fixture.hpp"
#include "ngx_live_continuous.hpp"
#include "ngx_live_current.hpp"
#include "ngx_lean.hpp"
#include "sr_guide_probe.hpp"
#include "fsr_runtime.hpp"
#include "render_timing.hpp"
static void finish_present(a::command_queue *q,a::swapchain *sc) {
 std::unique_lock guard(lock);
 if(q->get_device()->get_api()!=a::device_api::d3d12)return;
 timer_present_interval();auto present_ms=GetTickCount64();frame_delta_ms=previous_present_ms?double(present_ms-previous_present_ms):0.;previous_present_ms=present_ms;
 fsr_runtime_present(q,guard);guide_probe_present(q,guard);lean_present(q,guard);timer_present(q);++frame;seen_taa=seen_ui=seen_ao=false;
 auto release_batch=std::move(lean.borrow.ready_to_release);lean.borrow.ready_to_release.clear();
 for(auto &entry:native_reset_borrows.ready_to_release)release_batch.push_back(entry);native_reset_borrows.ready_to_release.clear();
 guard.unlock();for(auto &entry:release_batch)LeanBorrowCache::release(entry);return;
 if(remaining) {
  bool drain=(!roi_sequence && !live_continuous_mode) || remaining==1;
  std::vector<a::command_queue*> queues;if(drain)for(auto &c:copies)if(c.queue && std::find(queues.begin(),queues.end(),c.queue)==queues.end())queues.push_back(c.queue);
  for(auto *queue:queues)queue->wait_idle();
  fs::create_directories(root/label);std::ofstream log(root/label/"metadata.jsonl",std::ios::app);
  log<<"{\"kind\":\"frame\",\"frame\":"<<frame<<",\"frameDeltaMs\":"<<frame_delta_ms<<",\"taa\":"<<(seen_taa?"true":"false")<<",\"ui\":"<<(seen_ui?"true":"false")<<",\"ao\":"<<(seen_ao?"true":"false")<<",\"metadataOnly\":"<<(metadata_only?"true":"false")<<"}\n";
  log<<"{\"kind\":\"rt_lifetime_counts\",\"frame\":"<<frame<<",\"ASBuildCommands\":"<<rt_builds<<",\"DispatchRaysCommands\":"<<rt_dispatches<<",\"ASDescriptorUpdates\":"<<rt_descriptor_updates<<",\"RTPipelineShaderSubobjects\":"<<rt_pipeline_initializations<<"}\n";
  for(auto &[hash,count]:frame_compute_counts)log<<"{\"kind\":\"operation_count\",\"frame\":"<<frame<<",\"stage\":\"compute\",\"shader\":"<<hash<<",\"count\":"<<count<<"}\n";
  for(auto &[hash,count]:frame_pixel_counts)log<<"{\"kind\":\"operation_count\",\"frame\":"<<frame<<",\"stage\":\"pixel\",\"shader\":"<<hash<<",\"count\":"<<count<<"}\n";
  for(auto &[key,value]:frame_indirect_counts)log<<"{\"kind\":\"indirect_census\",\"frame\":"<<frame<<",\"stage\":\""<<(std::get<0>(key)?"compute":"graphics")<<"\",\"shader\":"<<std::get<1>(key)<<",\"vertexShader\":"<<std::get<2>(key)<<",\"apiCalls\":"<<value.first<<",\"requestedCommandMaximumSum\":"<<value.second<<",\"actualGPUCommandCountKnown\":false,\"commandSignatureKnown\":false,\"countBufferKnown\":false}\n";
  for(auto &[name,value]:frame_api_counts)log<<"{\"kind\":\"api_census\",\"frame\":"<<frame<<",\"name\":\""<<name<<"\",\"count\":"<<value<<"}\n";
  for(auto &[queue,value]:frame_queue_lists)log<<"{\"kind\":\"queue_list_census\",\"frame\":"<<frame<<",\"queue\":"<<queue<<",\"commandListsObserved\":"<<value<<"}\n";
  auto back=resource_desc(q->get_device(),back_buffer(sc));log<<"{\"kind\":\"output\",\"width\":"<<back.texture.width<<",\"height\":"<<back.texture.height<<",\"format\":"<<static_cast<uint32_t>(back.texture.format)<<"}\n";
  for(auto &r:records)log<<r<<'\n';records.clear();
  if(drain)for(auto &c:copies) {
   if(c.queue){void *data=nullptr;D3D12_RANGE range{0,static_cast<SIZE_T>(c.bytes)};auto hr=c.readback->Map(0,&range,&data);if(SUCCEEDED(hr)) {std::ofstream out(c.path,std::ios::binary);out.write(static_cast<const char*>(data),c.bytes);D3D12_RANGE empty{0,0};c.readback->Unmap(0,&empty);log<<"{\"kind\":\"readback\",\"file\":\""<<c.path.filename().string()<<"\",\"status\":\"gpu_queue_completed\"}\n";}else log<<"{\"kind\":\"error\",\"message\":\"map_failed\"}\n";
    c.readback->Release();
   } else {log<<"{\"kind\":\"error\",\"message\":\"copy_list_not_submitted_no_readback\"}\n";/* Retain unsubmitted resources until shutdown; never free a GPU reference early. */}
  }
  if(drain)copies.erase(std::remove_if(copies.begin(),copies.end(),[](const Copy &c){return c.queue!=nullptr;}),copies.end());
  if(seen_taa)--remaining;
  else {--remaining;if(live_fixture)live_fixture->reset_pending=true;log<<"{\"kind\":\"error\",\"message\":\"target_taa_not_seen_in_frame\"}\n";}
  if(!remaining){if(live_saved_mode)cleanup_live_saved(q);std::ofstream(root/label/"complete.txt")<<"Capture attempt completed; inspect metadata for validity.\n";reshade::log::message(reshade::log::level::info,"MCD2 observer capture attempt completed");}
 }
 ++frame;seen_taa=seen_ui=seen_ao=false;frame_compute_counts.clear();frame_pixel_counts.clear();frame_indirect_counts.clear();frame_api_counts.clear();frame_queue_lists.clear();
 static ULONGLONG last=0;auto now=GetTickCount64();if(now-last<100)return;last=now;
 if(!remaining && developer_file_exists(root/"request.txt")) {
  std::ifstream in(root/"request.txt");std::string mode,next;in>>mode>>next;in.close();fs::remove(root/"request.txt");
  if(mode=="probe" && (next=="ngx" || next=="ngx-initonly" || next=="ngx-create" || next=="ngx-eval" || next=="ngx-gpu-eval")){q->wait_idle();probe_ngx(q,next);return;}
  if(!next.empty() && next.size()<50 && next.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-")==std::string::npos && (mode=="metadata" || mode=="census" || mode=="capture" || mode=="sequence" || mode=="copysequence" || mode=="roisequence" || mode=="staterestoresequence" || mode=="proxyaudit" || mode=="livefixture" || mode=="livecurrent" || mode=="livecontinuous" || mode=="livedisplay")) {
   label=next;current_submissions.clear();current_submission_queue=0;current_queue_mismatch=false;proxy_audit=(mode=="proxyaudit");live_continuous_mode=(mode=="livecontinuous" || mode=="livedisplay");live_display_mode=(mode=="livedisplay");live_current_mode=(mode=="livecurrent" || live_continuous_mode);live_saved_mode=(mode=="livefixture" || live_current_mode);metadata_only=(mode=="metadata" || mode=="census" || proxy_audit || live_continuous_mode);copy_sequence=(mode=="copysequence");state_sequence=(mode=="staterestoresequence");roi_sequence=(mode=="roisequence");sequence_capture=(mode=="sequence" || copy_sequence || roi_sequence || state_sequence);remaining=live_continuous_mode?(live_display_mode?600:120):(mode=="census"?240:(roi_sequence?65:(sequence_capture?33:2)));capture_frame=frame;fs::create_directories(root/label);
   if(live_saved_mode && !init_live_saved(q)){remaining=0;std::ofstream(root/label/"complete.txt")<<"Live fixture initialization failed; inspect its private API log.\n";return;}
   reshade::log::message(reshade::log::level::info,"MCD2 observer capture attempt armed");
  }
 }
}
static void destroy_cmd(a::command_list *cmd){std::lock_guard guard(lock);timer_reset(cmd);lean_forget_recording(cmd);commands.erase(cmd);}
static void init_device(a::device *d) {if(d->get_api()==a::device_api::d3d12)reshade::log::message(reshade::log::level::info,"MCD2 observer D3D12 initialized; captures only on explicit request file");}
static void init_queue(a::command_queue *q){std::lock_guard guard(lock);if(capture_probe_queue)probe_queue_api=q;}
static void destroy_queue(a::command_queue *q) {
 std::unique_lock guard(lock);guide_probe_destroy_queue(q);fsr_runtime_destroy_queue(q);
 if(native_reset_queue==q){native_reset_borrows.blocked=true;native_reset_queue=nullptr;}
 if(!live_fixture || live_fixture->integration_queue!=q)return;
 lean.terminal=true;lean.wanted=false;
 lean.log<<"{\"kind\":\"integration_queue_destruction\",\"recordedLists\":"<<lean.borrow.recordings.size()<<",\"bundles\":"<<lean.borrow.entries.size()<<",\"historyDirty\":"<<(lean.history_dirty?"true":"false")<<"}\n";lean.log.flush();
 const bool success=lean_cleanup(q,guard,true);
 if(!success){lean.failed=true;lean.borrow.blocked=true;if(live_fixture){live_fixture->ready=false;live_fixture->integration_queue=nullptr;}}
 std::ofstream(root/label/"queue-teardown.json")<<"{\"cleanupCompleted\":"<<(success?"true":"false")<<",\"generationRetained\":"<<(live_fixture?"true":"false")<<"}\n";
}
static void destroy_device(a::device *dev) {std::lock_guard guard(lock);pipelines.clear();layouts.clear();commands.clear();descriptors.clear();initial_states.clear();live_resources.clear();submitted_states.clear();remaining=0;records.clear();for(auto &c:copies)if(c.readback)c.readback->Release();copies.clear();auto it=roundtrip_textures.find(dev);if(it!=roundtrip_textures.end()){if(it->second)it->second->Release();roundtrip_textures.erase(it);}}

extern "C" __declspec(dllexport) const char *NAME="MCD2 Graphics Reconstruction Candidate";
extern "C" __declspec(dllexport) const char *DESCRIPTION="Native graphics-menu reconstruction; version-pinned interop";
#define EVENT(ev,fn) reshade::register_event<reshade::addon_event::ev>(fn)
#define REMOVE(ev,fn) reshade::unregister_event<reshade::addon_event::ev>(fn)
BOOL WINAPI DllMain(HMODULE module,DWORD reason,LPVOID) {
 if(reason==DLL_PROCESS_ATTACH) {
  // Keep this 64 KiB Windows path buffer off the DLL-loading thread's stack.
  // See docs/WINDOWS_COMPATIBILITY.md before replacing it with a fixed array.
  std::vector<wchar_t> module_path(32768);auto path_size=GetModuleFileNameW(module,module_path.data(),DWORD(module_path.size()));
  if(!path_size || path_size>=module_path.size())return FALSE;
  asset_root=fs::path(module_path.data()).parent_path()/L"MCD2Graphics";
  if(!public_bridge_layout_matches())return FALSE;
  if(!reshade::register_addon(module))return FALSE;
  EVENT(init_device,init_device);EVENT(destroy_device,destroy_device);EVENT(init_command_queue,init_queue);EVENT(destroy_command_queue,destroy_queue);EVENT(init_pipeline_layout,init_layout);EVENT(init_pipeline,init_pipe);EVENT(bind_pipeline,bind_pipe);
  EVENT(push_descriptors,push_desc);EVENT(bind_descriptor_tables,bind_tables);EVENT(push_constants,push_constants);EVENT(update_descriptor_tables,update_desc);EVENT(copy_descriptor_tables,copy_desc);
  EVENT(init_resource,init_resource);EVENT(destroy_resource,destroy_resource);EVENT(barrier,barrier);EVENT(reset_command_list,reset_cmd);EVENT(close_command_list,timer_close);EVENT(destroy_command_list,destroy_cmd);EVENT(dispatch,dispatch);EVENT(execute_command_list,execute);EVENT(finish_present,finish_present);
  
  
 } else if(reason==DLL_PROCESS_DETACH) {
  REMOVE(init_device,init_device);REMOVE(destroy_device,destroy_device);REMOVE(init_command_queue,init_queue);REMOVE(destroy_command_queue,destroy_queue);REMOVE(init_pipeline_layout,init_layout);REMOVE(init_pipeline,init_pipe);REMOVE(bind_pipeline,bind_pipe);
  REMOVE(push_descriptors,push_desc);REMOVE(bind_descriptor_tables,bind_tables);REMOVE(push_constants,push_constants);REMOVE(update_descriptor_tables,update_desc);REMOVE(copy_descriptor_tables,copy_desc);
  REMOVE(init_resource,init_resource);REMOVE(destroy_resource,destroy_resource);REMOVE(barrier,barrier);REMOVE(reset_command_list,reset_cmd);REMOVE(close_command_list,timer_close);REMOVE(destroy_command_list,destroy_cmd);REMOVE(dispatch,dispatch);REMOVE(execute_command_list,execute);REMOVE(finish_present,finish_present);
  
  
  reshade::unregister_addon(module);
 }
 return TRUE;
}
