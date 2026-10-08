#pragma once
#include <array>
#include <cstdint>

namespace mcd2::providers {
struct SrViewport {
    std::uint32_t width=0,height=0,allocationWidth=0,allocationHeight=0;
};
// The observed temporal pass uses inclusive integer rectangle endpoints.
// Accept only its known zero-origin layout and bounded allocation padding.
inline bool srViewport(std::uint32_t aw,std::uint32_t ah,
                       const std::array<std::uint32_t,4>& rect,SrViewport& out){
    if(!aw||!ah||aw>7680||ah>4320||rect[0]||rect[1]||rect[2]>=aw||rect[3]>=ah)return false;
    const auto w=rect[2]+1,h=rect[3]+1;
    if(w<640||h<360||aw-w>7||ah-h>7)return false;
    out={w,h,aw,ah};return true;
}
// UE rounds the active viewport up; the SDK's named-mode recommendation
// rounds down. A one-pixel difference is permitted after the exact scale ACK.
inline bool srViewportMatchesPlan(const SrViewport& v,std::uint32_t w,std::uint32_t h){
    return v.width>=w&&v.width-w<=1&&v.height>=h&&v.height-h<=1;
}
}
