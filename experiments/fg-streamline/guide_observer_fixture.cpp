// Portable test doubles around the extracted production observe() body. No game/GPU access.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
using SIZE_T=decltype(sizeof(0));
struct D3D12_HEAP_PROPERTIES{unsigned Type=0;};using D3D12_HEAP_FLAGS=unsigned;
constexpr unsigned D3D12_HEAP_TYPE_UPLOAD=1;
struct D3D12_RANGE{SIZE_T Begin,End;};struct D3D12_RESOURCE_DESC{unsigned long long Width=0;};
inline bool SUCCEEDED(int result){return result>=0;}
struct ID3D12Resource{static inline unsigned calls=0;int GetHeapProperties(D3D12_HEAP_PROPERTIES* heap,D3D12_HEAP_FLAGS*){++calls;heap->Type=0;return 0;}D3D12_RESOURCE_DESC GetDesc(){++calls;return {};}int Map(unsigned,D3D12_RANGE*,void**){throw 1;}void Unmap(unsigned,D3D12_RANGE*){throw 2;}};
#include <map>
#include <vector>
#include <array>
#include <memory>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <iostream>
namespace a {
enum class descriptor_type {sampler,constant_buffer,shader_resource_view,buffer_shader_resource_view,unordered_access_view,buffer_unordered_access_view};
enum class shader_stage:unsigned {pixel=1,compute=2};
inline shader_stage operator&(shader_stage x,shader_stage y){return shader_stage(unsigned(x)&unsigned(y));}
enum class resource_type {buffer,texture};
struct resource {uint64_t handle=0;};struct resource_view {uint64_t handle=0;};struct descriptor_table{uint64_t handle=0;};
struct buffer_range {resource buffer;uint64_t offset=0,size=0;};
struct resource_desc {resource_type type=resource_type::texture;struct{uint64_t size=0;}buffer;struct{unsigned width=16,height=16,format=1;}texture;};
struct descriptor_range {descriptor_type type=descriptor_type::shader_resource_view;unsigned count=1,dx_register_space=0;shader_stage visibility=shader_stage::pixel;unsigned binding=0,dx_register_index=0;};
struct device{unsigned descriptionCalls=0;uint64_t live=0;resource get_resource_from_view(resource_view view){return {view.handle};}resource_desc get_resource_desc(resource r){++descriptionCalls;if(r.handle!=live)throw std::runtime_error("native dereference of destroyed/unknown resource");return {};}};
struct command_list {device* owner;device*get_device(){return owner;}};
}
struct Binding {a::descriptor_type type{};a::resource_view view{};a::buffer_range cb{};};
struct Root {uint64_t layout=1;std::map<unsigned,uint64_t> tables;std::map<unsigned,std::map<unsigned,Binding>> pushed;};
struct State {Root graphics,compute;unsigned ps=1,cs=0;uint64_t graphicsPipeline=1;bool backbuffer=false,passActive=false,passKnown=false,diagnosticViewportKnown=false,diagnosticScissorKnown=false;struct{float x=0,y=0,width=1,height=1;}diagnosticViewport;struct{int left=0,top=0,right=1,bottom=1;}diagnosticScissor;std::map<uint64_t,unsigned> states;};
struct Pipe {bool blendKnown=false,blendEnabled=false;unsigned writeMask=0;};
State state;unsigned frame=240,samples=0;std::ostringstream privateLog;std::map<uint64_t,Pipe>pipelines;
std::map<uint64_t,std::vector<std::vector<a::descriptor_range>>> layouts;
std::map<std::pair<uint64_t,unsigned>,Binding> descriptors;std::map<uint64_t,unsigned>initial,submitted;
State&recording(a::command_list*){return state;}bool capturing(){return true;}void identity_receipt(const char*){}
std::pair<uint64_t,unsigned>key(a::device*,a::descriptor_table t,unsigned b){return {t.handle,b};}
uint64_t(*renderFrame)()=nullptr;

// Match the public-resource adapter boundary without replacing observe().
namespace fg_resources {
auto from_view(a::device* device,a::resource_view view){return device->get_resource_from_view(view);}
auto resource_desc(a::device* device,a::resource resource){return device->get_resource_desc(resource);}
}
// ACTUAL_OBSERVE

void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
int main(){try{
 a::device device;a::command_list command{&device};layouts[1]={{{}}};state.graphics.tables[0]=7;state.compute.tables[0]=7;
 Binding b;b.type=a::descriptor_type::shader_resource_view;b.view={42};descriptors[{7,0}]=b;
 device.live=42;initial[42]=1;observe(&command,false);require(device.descriptionCalls==1,"live view not observed");
 // Retain exactly the old descriptor while the resource's destroy callback erases liveness.
 initial.erase(42);device.live=0;observe(&command,false);require(device.descriptionCalls==1,"destroyed view queried");
 descriptors[{7,0}].view={99};observe(&command,false);require(device.descriptionCalls==1,"unknown view queried");
 descriptors[{7,0}].view={0};observe(&command,false);require(device.descriptionCalls==1,"null view queried");
 descriptors[{7,0}].view={1};observe(&command,false);require(device.descriptionCalls==1,"sentinel view queried");
 device.live=42;initial[42]=1;descriptors[{7,0}].view={42};observe(&command,false);require(device.descriptionCalls==2,"new live view not observed");
 // Exercise the distinct constant-buffer path and ensure rejection precedes all native calls.
 ID3D12Resource native;auto handle=reinterpret_cast<uint64_t>(&native);b.type=a::descriptor_type::constant_buffer;b.cb.buffer={handle};b.cb.size=UINT64_MAX;
 auto& range=layouts[1][0][0];range.type=a::descriptor_type::constant_buffer;range.visibility=a::shader_stage::compute;range.dx_register_index=1;descriptors[{7,0}]=b;
 device.live=0;observe(&command,true);require(device.descriptionCalls==2&&ID3D12Resource::calls==0,"unknown CB queried");
 device.live=handle;initial[handle]=1;observe(&command,true);require(device.descriptionCalls==3&&ID3D12Resource::calls>0,"live CB not observed");
 initial.erase(handle);device.live=0;auto nativeCalls=ID3D12Resource::calls;observe(&command,true);require(device.descriptionCalls==3&&ID3D12Resource::calls==nativeCalls,"destroyed CB queried");
 std::cout<<"Actual observe: live view/CB allowed, destroyed/unknown/null/sentinel bindings skipped before native access.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
