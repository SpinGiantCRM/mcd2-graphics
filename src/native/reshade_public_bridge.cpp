// Compile standard ReShade calls with Microsoft C++ ABI. No slot arithmetic.
#include "reshade_api_device.hpp"
namespace a=reshade::api;
extern "C" {
void mcd2_resource_from_view(a::device*d,uint64_t h,a::resource*out){*out=d->get_resource_from_view(a::resource_view{h});}
void mcd2_view_desc(a::device*d,uint64_t h,a::resource_view_desc*out){*out=d->get_resource_view_desc(a::resource_view{h});}
void mcd2_resource_desc(a::device*d,uint64_t h,a::resource_desc*out){*out=d->get_resource_desc(a::resource{h});}
void mcd2_back_buffer(a::swapchain*s,uint32_t i,a::resource*out){*out=s->get_back_buffer(i);}
}

extern "C" void mcd2_reshade_bridge_layout(uint32_t*out){
 const uint32_t v[]={sizeof(a::resource),alignof(a::resource),sizeof(a::resource_view),alignof(a::resource_view),sizeof(a::resource_view_desc),alignof(a::resource_view_desc),sizeof(a::resource_desc),alignof(a::resource_desc)};
 for(unsigned i=0;i<8;++i)out[i]=v[i];
}
