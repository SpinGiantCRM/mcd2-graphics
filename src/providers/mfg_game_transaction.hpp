#pragma once
#include "mfg_phase_plan.hpp"
#include <type_traits>
namespace mcd2::mfg {
struct Identity {
    std::uint32_t session{},revision{},checksum{},world{};
    std::uint64_t epoch{};
    bool operator==(const Identity&) const = default;
    explicit operator bool()const{return session&&revision&&world&&epoch;}
};
// The host issues this ticket only after submission on its exact owning queue.
// Values are opaque queue/fence identities, never GPU-model or SDK eligibility.
struct Completion {
    std::uint64_t queue{},fence{},value{};
    bool operator==(const Completion&) const = default;
    explicit operator bool()const{return queue&&fence&&value&&value!=UINT64_MAX;}
};
// One render/present owner keeps actual COM leases until a subsequent retirement
// fence. This transaction is not itself a GPU resource owner or completion query.
class GameTransaction {
    Batch batch_;
    Identity identity_{};RealPair pair_{};Plan plan_{};Completion work_{};
    std::array<std::uint64_t,5> outputs_{};
    unsigned consumed_{};
    bool active_{},valid_{},ready_{},real_{};
    std::uint64_t rendered_{},native_{},software_{};
    bool matches(Identity id,RealPair pair)const{return active_&&valid_&&id==identity_&&pair==pair_;}
    bool reject(){valid_=ready_=false;return false;}
public:
    GameTransaction()=default;
    GameTransaction(const GameTransaction&)=delete;GameTransaction& operator=(const GameTransaction&)=delete;
    GameTransaction(GameTransaction&&)=delete;GameTransaction& operator=(GameTransaction&&)=delete;
    void invalidate(){batch_.invalidate();active_=valid_=ready_=real_=false;outputs_={};consumed_=0;work_={};}
    bool begin(Identity id,RealPair pair,const Plan& plan){
        if(active_)return reject();
        if(!id||id.epoch!=pair.epoch||!batch_.begin(plan,pair))return false;
        identity_=id;pair_=pair;plan_=plan;active_=valid_=true;return true;
    }
    bool record(RealPair pair,unsigned slot,Provider provider,std::uint64_t output,bool success){
        if(!active_||!valid_||work_)return false;
        if(!batch_.record(pair,slot,provider,output,success))return reject();
        outputs_[slot-1]=output;return true;
    }
    bool submitted(Completion work){
        if(!active_||!valid_||work_||!work||!batch_.complete())return reject();
        work_=work;return true;
    }
    bool completed(Completion work,std::uint64_t completedValue){
        if(!active_||!valid_||!work_||work!=work_||completedValue==UINT64_MAX||completedValue<work_.value)return false;
        ready_=true;return true;
    }
    std::uint64_t output(Identity id,RealPair pair,unsigned slot)const{
        return matches(id,pair)&&ready_&&!real_&&slot>consumed_&&slot<=plan_.count?outputs_[slot-1]:0;
    }
    bool presented(Identity id,RealPair pair,unsigned slot,bool success){
        if(!matches(id,pair)||!ready_||real_||!success||slot!=consumed_+1||slot>plan_.count)return reject();
        ++consumed_;if(plan_.phases[slot-1].provider==Provider::NvidiaNative)++native_;else ++software_;return true;
    }
    bool realPresented(Identity id,RealPair pair,bool success){
        if(!matches(id,pair)||!ready_||real_||consumed_!=plan_.count||!success)return reject();
        real_=true;++rendered_;return true;
    }
    bool retire(Completion retirement,std::uint64_t completedValue){
        if(!active_||!work_||!retirement||retirement.queue!=work_.queue||retirement.fence!=work_.fence||
           retirement.value<=work_.value||completedValue==UINT64_MAX||completedValue<retirement.value)return false;
        invalidate();return true;
    }
    std::uint64_t rendered()const{return rendered_;}
    std::uint64_t nativeGenerated()const{return native_;}
    std::uint64_t softwareGenerated()const{return software_;}
    std::uint64_t generated()const{return native_+software_;}
};
}
