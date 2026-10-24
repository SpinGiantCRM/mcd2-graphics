#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <numeric>

// Compute scheduling only. No vendor/model whitelist, presenter or runtime
// qualification claim lives here. Capabilities are supplied by an admitted host.
namespace mcd2::mfg {
enum class Mode : unsigned { Software, Native, Hybrid };
enum class Provider : unsigned { Software, NvidiaNative };
struct Capabilities {
    unsigned softwareMaxMultiplier{}, nativeMaxMultiplier{};
    bool nativeCallerOwnedOutput{};
};
struct Phase { unsigned numerator{},denominator{}; Provider provider{}; };
struct Plan {
    Mode mode{}; unsigned multiplier{},count{}; std::array<Phase,5> phases{};
    unsigned dlssGenerated{},fsrGenerated{};
};
// Native evaluations may occupy a coarser uniform grid than the total output
// grid. Its SDK count/index describes that native grid, never the number of
// native outputs selected from it (for example 2 DLSS +3 FSR uses thirds).
inline unsigned native_grid_divisor(const Plan& plan) {
    unsigned divisor=plan.multiplier;
    for(unsigned i=0;i<plan.count && i<plan.phases.size();++i)
        if(plan.phases[i].provider==Provider::NvidiaNative)
            divisor=std::gcd(divisor,plan.phases[i].numerator);
    return divisor;
}
inline unsigned native_generated_count(const Plan& plan) {
    if(!plan.dlssGenerated || !plan.multiplier) return 0;
    const auto divisor=native_grid_divisor(plan);
    return divisor?plan.multiplier/divisor-1:0;
}
inline unsigned native_phase_index(const Plan& plan,unsigned slot) {
    if(!slot || slot>plan.count || slot>plan.phases.size() ||
       plan.phases[slot-1].provider!=Provider::NvidiaNative) return 0;
    const auto divisor=native_grid_divisor(plan);
    return divisor?plan.phases[slot-1].numerator/divisor:0;
}
inline std::optional<Plan> make_allocation_plan(unsigned dlss,unsigned fsr,Capabilities caps) {
    if(dlss>5 || fsr>5 || dlss+fsr<1 || dlss+fsr>5) return {};
    const unsigned total=1+dlss+fsr;
    Plan plan{dlss?(fsr?Mode::Hybrid:Mode::Native):Mode::Software,total,total-1,{},dlss,fsr};
    for(unsigned slot=1;slot<total;++slot) plan.phases[slot-1]={slot,total,Provider::Software};
    for(unsigned j=1;j<=dlss;++j) {
        // Positive-integer round-to-nearest, with a half rounded upward.
        const unsigned slot=(2*j*total+dlss+1)/(2*(dlss+1));
        plan.phases[slot-1].provider=Provider::NvidiaNative;
    }
    if(fsr && caps.softwareMaxMultiplier<total) return {};
    if(dlss && (!caps.nativeCallerOwnedOutput ||
                caps.nativeMaxMultiplier<native_generated_count(plan)+1)) return {};
    return plan;
}
inline std::optional<Plan> make_plan(Mode mode,unsigned multiplier,Capabilities caps) {
    if(multiplier<2 || multiplier>6) return {};
    if(mode==Mode::Software) return make_allocation_plan(0,multiplier-1,caps);
    if(mode==Mode::Native) return make_allocation_plan(multiplier-1,0,caps);
    if(mode==Mode::Hybrid && (multiplier==4 || multiplier==6))
        return make_allocation_plan(1,multiplier-2,caps);
    return {};
}
enum class Origin : unsigned { Real, Generated };
struct RealPair {
    uint64_t epoch{},previous{},current{};
    Origin previousOrigin{Origin::Real}, currentOrigin{Origin::Real};
    bool operator==(const RealPair&) const = default;
};
class Batch {
    Plan plan_{};
    RealPair pair_{};
    std::array<uint64_t,5> outputs_{};
    unsigned recorded_{};
    bool active_{},valid_{};
public:
    void invalidate() { active_=valid_=false; recorded_=0; outputs_={}; }
    bool begin(const Plan& plan,RealPair pair) {
        // A caller that ignores a rejected replacement must not publish or
        // continue the old transaction. Keep it occupied until invalidate().
        if(active_) { valid_=false; return false; }
        if(!pair.epoch || !pair.previous || pair.current<=pair.previous ||
           pair.previousOrigin!=Origin::Real || pair.currentOrigin!=Origin::Real) return false;
        const auto expected=make_allocation_plan(plan.dlssGenerated,plan.fsrGenerated,{6,6,true});
        if(!expected || plan.mode!=expected->mode || plan.multiplier!=expected->multiplier ||
           plan.count!=expected->count) return false;
        for(unsigned i=0;i<plan.count;++i) {
            const auto& a=plan.phases[i]; const auto& b=expected->phases[i];
            if(a.numerator!=b.numerator || a.denominator!=b.denominator || a.provider!=b.provider) return false;
        }
        plan_=plan; pair_=pair; outputs_={}; recorded_=0; active_=valid_=true; return true;
    }
    bool record(RealPair pair,unsigned slot,Provider provider,uint64_t output,bool evaluationSucceeded) {
        if(!active_ || !valid_) return false;
        if(pair!=pair_ || !evaluationSucceeded || !output || slot!=recorded_+1 ||
           slot>plan_.count || provider!=plan_.phases[slot-1].provider) {
            valid_=false; return false;
        }
        for(unsigned i=0;i<recorded_;++i) if(outputs_[i]==output) { valid_=false; return false; }
        outputs_[recorded_++]=output; return true;
    }
    // The owner must separately prove queue completion before consuming images.
    bool complete() const { return active_ && valid_ && recorded_==plan_.count; }
};
}
