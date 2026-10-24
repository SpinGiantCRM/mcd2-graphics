#pragma once
#include "graphics_record.hpp"

namespace mcd2::providers {
// Restart-only preferences. These records never authorize rendering or change
// the existing graphics intent ABI. The loader remains the persistence owner.
using AdvancedRequestBytes=std::array<std::uint8_t,64>;
using AdvancedStateBytes=std::array<std::uint8_t,128>;
struct AdvancedRequest {
    std::uint32_t session=0,sequence=0;
    RecordStamp expected;
    std::uint32_t hint=0,migrate=0;
    bool operator==(const AdvancedRequest&)const=default;
};
enum class AdvancedStatus : std::uint32_t { Ready, Saved, Conflict, Invalid, WriteFailure, Busy, SequenceReuse, OutOfOrder };
struct AdvancedState {
    std::uint32_t session=0,sequence=0;
    AdvancedStatus status=AdvancedStatus::Ready;
    RecordStamp expected;
    std::uint32_t hint=0,startupHint=0,migrated=0;
    // V1 identities/readable bits 0..3/4..7 are unchanged. V2 adds DLSS FG
    // identity/readable bits 8/9 and packs its version in the two reserved words.
    std::uint32_t modules=0;
    std::array<std::uint32_t,20> versions{};
};
inline unsigned advancedLoadedBit(unsigned index){return index<4?1u<<index:index==4?256u:0u;}
inline unsigned advancedVersionBit(unsigned index){return index<4?1u<<(index+4):index==4?512u:0u;}
template<std::size_t N>inline void advancedWord(std::array<std::uint8_t,N>& b,std::size_t word,std::uint32_t value){
    for(unsigned n=0;n<4;++n)b[word*4+n]=std::uint8_t(value>>(8*n));
}
inline bool decodeAdvancedRequest(std::span<const std::uint8_t> b,AdvancedRequest& out){
    if(b.size()!=64 || recordWord(b,0)!=0x3244434d || recordWord(b,4)!=0x00315141 ||
       recordWord(b,8)!=1 || recordWord(b,12)!=64 || recordWord(b,16)!=recordChecksum(b))return false;
    AdvancedRequest r{recordWord(b,20),recordWord(b,24),{recordWord(b,28),recordWord(b,32)},recordWord(b,36),recordWord(b,40)};
    if(!validRevision(r.session)||!validRevision(r.sequence)||!validRevision(r.expected.revision)||r.hint>255||r.migrate>1)return false;
    if(std::any_of(b.begin()+44,b.end(),[](auto v){return v!=0;}))return false;
    out=r;return true;
}
inline bool encodeAdvancedRequest(const AdvancedRequest& r,AdvancedRequestBytes& out){
    AdvancedRequestBytes b{};
    advancedWord(b,0,0x3244434d);advancedWord(b,1,0x00315141);advancedWord(b,2,1);advancedWord(b,3,64);
    advancedWord(b,5,r.session);advancedWord(b,6,r.sequence);advancedWord(b,7,r.expected.revision);
    advancedWord(b,8,r.expected.checksum);advancedWord(b,9,r.hint);advancedWord(b,10,r.migrate);
    advancedWord(b,4,recordChecksum(b));AdvancedRequest checked;if(!decodeAdvancedRequest(b,checked))return false;
    out=b;return true;
}
inline bool encodeAdvancedState(const AdvancedState& a,AdvancedStateBytes& out){
    if(!validRevision(a.session)||unsigned(a.status)>7||a.hint>255||a.startupHint>255||a.migrated>1||a.modules>1023)return false;
    if(a.sequence && (!validRevision(a.sequence)||!validRevision(a.expected.revision)))return false;
    if(!a.sequence && (a.status!=AdvancedStatus::Ready||a.expected!=RecordStamp{}||a.migrated))return false;
    for(unsigned i=0;i<5;++i){
        const bool version=(a.modules&advancedVersionBit(i))!=0;
        if(version && !(a.modules&advancedLoadedBit(i)))return false;
        for(unsigned n=0;n<4;++n)if(a.versions[i*4+n]>65535||(!version&&a.versions[i*4+n]))return false;
    }
    AdvancedStateBytes b{};advancedWord(b,0,0x3244434d);advancedWord(b,1,0x00315241);advancedWord(b,2,2);advancedWord(b,3,128);
    advancedWord(b,5,a.session);advancedWord(b,6,a.sequence);advancedWord(b,7,unsigned(a.status));
    advancedWord(b,8,a.expected.revision);advancedWord(b,9,a.expected.checksum);
    advancedWord(b,10,a.hint);advancedWord(b,11,a.startupHint);advancedWord(b,12,a.migrated);advancedWord(b,13,a.modules);
    for(unsigned n=0;n<16;++n)advancedWord(b,14+n,a.versions[n]);
    advancedWord(b,30,a.versions[16]|(a.versions[17]<<16));
    advancedWord(b,31,a.versions[18]|(a.versions[19]<<16));
    advancedWord(b,4,recordChecksum(b));out=b;return true;
}

// One instance on the existing settings worker. Replays never repeat a write.
class AdvancedRequests {
    std::uint32_t session_;
    bool seen_=false;
    AdvancedRequest last_;
    AdvancedState state_;
public:
    AdvancedRequests(std::uint32_t session,std::uint32_t startup):session_(session){state_.session=session;state_.hint=state_.startupHint=startup;}
    const AdvancedState& state()const{return state_;}
    template<class Backend,class Recheck>
    void consume(const AdvancedRequest& r,RecordStamp current,Backend& backend,Recheck recheck){
        AdvancedRequestBytes checked;if(!encodeAdvancedRequest(r,checked)||r.session!=session_)return;
        if(seen_ && r.sequence==last_.sequence){
            if(r!=last_)state_.status=AdvancedStatus::SequenceReuse;
            return;
        }
        if(seen_ && r.sequence<last_.sequence)return;
        seen_=true;last_=r;state_.sequence=r.sequence;state_.expected=r.expected;state_.migrated=0;
        if(r.expected!=current || !recheck()){state_.status=AdvancedStatus::Conflict;return;}
        std::uint32_t old=0;
        if(!backend.read(old)||old>255){state_.status=AdvancedStatus::Invalid;return;}
        const auto wanted=r.migrate?old:r.hint;
        if(!recheck()){state_.status=AdvancedStatus::Conflict;return;}
        if(old!=wanted && !backend.write(wanted)){state_.status=AdvancedStatus::WriteFailure;return;}
        std::uint32_t confirmed=0;
        if(!backend.read(confirmed)||confirmed!=wanted){state_.status=AdvancedStatus::WriteFailure;return;}
        state_.hint=confirmed;state_.migrated=r.migrate;state_.status=AdvancedStatus::Saved;
    }
};
} // namespace mcd2::providers
