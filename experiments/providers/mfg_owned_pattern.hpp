#pragma once
#include <cmath>
#include <limits>

namespace mcd2::mfg {
inline float stripe_red(unsigned x,unsigned shift) {
    return ((x+1280-shift)%1280/24)%2 ? .75f : .25f;
}
struct PhaseFit { unsigned offset{}; double meanAbsoluteError{std::numeric_limits<double>::infinity()}; };
inline PhaseFit fit_phase(const float* row,unsigned width,unsigned previousShift) {
    PhaseFit fit;
    if(!row || width!=1280 || previousShift>88) return fit;
    for(unsigned offset=0;offset<=8;++offset) {
        double error=0;
        // Exclude entering/exiting image edges where disocclusion is expected.
        for(unsigned x=128;x<1152;++x) {
            if(!std::isfinite(row[x])) return {};
            error+=std::fabs(row[x]-stripe_red(x,previousShift+offset));
        }
        error/=1024;
        if(error<fit.meanAbsoluteError) fit={offset,error};
    }
    return fit;
}
}
