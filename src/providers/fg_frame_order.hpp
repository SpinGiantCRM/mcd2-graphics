#pragma once
#include <algorithm>
#include <bit>
#include <limits>
#include <numeric>
#include <optional>
#include <tuple>

namespace mcd2::mfg {
// Code zero is Automatic. Manual codes have exactly the marker plus the
// generated-slot DLSS mask; all other slots belong to FSR.
inline constexpr bool valid_order(unsigned dlss,unsigned fsr,unsigned code) {
    if(dlss>5 || fsr>5 || dlss+fsr<1 || dlss+fsr>5) return false;
    if(!code) return true;
    const unsigned mask=code&31u;
    return !(code&~63u) && (code&32u) && mask<(1u<<(dlss+fsr)) &&
           std::popcount(mask)==int(dlss);
}
inline unsigned native_count_for_mask(unsigned total,unsigned mask) {
    if(total<2 || total>6 || mask>=(1u<<(total-1)))
        return std::numeric_limits<unsigned>::max();
    if(!mask) return 0;
    unsigned divisor=total;
    for(unsigned slot=1;slot<total;++slot)
        if(mask&(1u<<(slot-1))) divisor=std::gcd(divisor,slot);
    return total/divisor-1;
}
inline auto order_score(unsigned total,unsigned mask) {
    unsigned largest=0,squares=0;
    for(unsigned provider=0;provider<2;++provider) {
        unsigned previous=0;bool present=false;
        for(unsigned slot=1;slot<total;++slot) {
            if(((mask>>(slot-1))&1u)!=provider) continue;
            const unsigned interval=slot-previous;
            largest=std::max(largest,interval);squares+=interval*interval;
            previous=slot;present=true;
        }
        if(present) {
            const unsigned interval=total-previous;
            largest=std::max(largest,interval);squares+=interval*interval;
        }
    }
    return std::tuple{largest,squares,native_count_for_mask(total,mask),31u-mask};
}
inline std::optional<unsigned> resolve_order_mask(unsigned dlss,unsigned fsr,
                                                 unsigned code,unsigned nativeGeneratedMaximum) {
    if(!valid_order(dlss,fsr,code)) return {};
    const unsigned total=1+dlss+fsr;
    if(code) {
        const unsigned mask=code&31u;
        return native_count_for_mask(total,mask)<=nativeGeneratedMaximum?
               std::optional<unsigned>{mask}:std::nullopt;
    }
    std::optional<unsigned> best;
    for(unsigned mask=0;mask<(1u<<(total-1));++mask) {
        if(std::popcount(mask)!=int(dlss) ||
           native_count_for_mask(total,mask)>nativeGeneratedMaximum) continue;
        if(!best || order_score(total,mask)<order_score(total,*best)) best=mask;
    }
    return best;
}
}
