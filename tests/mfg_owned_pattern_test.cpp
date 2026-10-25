#include "../experiments/providers/mfg_owned_pattern.hpp"
#include <array>
#include <cassert>
int main() {
    std::array<float,1280> row{};
    // Previous real frame at shift 80, current at 88: quarter phase is 82.
    for(unsigned x=0;x<row.size();++x) row[x]=(x%48>=10 && x%48<34) ? .75f : .25f;
    auto quarter=mcd2::mfg::fit_phase(row.data(),row.size(),80);
    assert(quarter.offset==2 && quarter.meanAbsoluteError<0.00001);
    for(unsigned x=0;x<row.size();++x) row[x]=(x%48>=14 && x%48<38) ? .75f : .25f;
    auto threeQuarter=mcd2::mfg::fit_phase(row.data(),row.size(),80);
    assert(threeQuarter.offset==6 && threeQuarter.meanAbsoluteError<0.00001);
    row.fill(0.f); auto black=mcd2::mfg::fit_phase(row.data(),row.size(),80);
    assert(black.meanAbsoluteError>0.4);
    assert(mcd2::mfg::stripe_red(82,82)==.25f);
    assert(mcd2::mfg::stripe_red(106,82)==.75f);
}
