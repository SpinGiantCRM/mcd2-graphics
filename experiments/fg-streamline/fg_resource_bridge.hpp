#pragma once
#include <algorithm>
#if defined(__MINGW32__)
#include "../../src/native/reshade_public_bridge.hpp"
#endif
namespace fg_resources {
namespace a=reshade::api;
#if defined(__MINGW32__)
// ReShade's aggregate return values need the Microsoft ABI used by SR's bridge.
inline a::resource from_view(a::device*d,a::resource_view v){a::resource r{};mcd2_resource_from_view(d,v.handle,&r);return r;}
inline a::resource_view_desc view_desc(a::device*d,a::resource_view v){a::resource_view_desc r{};mcd2_view_desc(d,v.handle,&r);return r;}
inline a::resource_desc resource_desc(a::device*d,a::resource v){a::resource_desc r{};mcd2_resource_desc(d,v.handle,&r);return r;}
inline a::resource back_buffer(a::swapchain*s,unsigned i){a::resource r{};mcd2_back_buffer(s,i,&r);return r;}
inline bool layout_matches(){
 const uint32_t expected[]={sizeof(a::resource),alignof(a::resource),sizeof(a::resource_view),alignof(a::resource_view),sizeof(a::resource_view_desc),alignof(a::resource_view_desc),sizeof(a::resource_desc),alignof(a::resource_desc)};
 uint32_t actual[8]{};mcd2_reshade_bridge_layout(actual);return std::equal(std::begin(expected),std::end(expected),std::begin(actual));
}
#else
inline a::resource from_view(a::device*d,a::resource_view v){return d->get_resource_from_view(v);}
inline a::resource_view_desc view_desc(a::device*d,a::resource_view v){return d->get_resource_view_desc(v);}
inline a::resource_desc resource_desc(a::device*d,a::resource v){return d->get_resource_desc(v);}
inline a::resource back_buffer(a::swapchain*s,unsigned i){return s->get_back_buffer(i);}
inline bool layout_matches(){return true;}
#endif
inline a::resource current_back_buffer(a::swapchain*s){return back_buffer(s,s->get_current_back_buffer_index());}
}
