#pragma once
#include "menu_requests.hpp"
#include "observed_settings.hpp"

namespace mcd2::providers {
// The UE save serializer omits default-zero IntProperty fields. Word slots
// preserve signed int bits, including checksum words with their high bit set.
// This parser is deliberately separate from the nonnegative legacy decoder.
namespace menu_save_detail {
inline constexpr std::string_view requestClass = "ProviderRequestSave";
inline constexpr std::string_view authorityClass = "ProviderAuthoritySave";
template<std::size_t N>
bool parse(std::span<const std::uint8_t> bytes, std::string_view className,
           std::array<std::uint8_t,N>& out, std::size_t* prefixEnd = nullptr) {
    static_assert(N%4 == 0);
    if(bytes.size()<64 || bytes.size()>8192)return false;
    legacy_detail::Reader r{bytes};
    if(r.number(4)!=0x53415647 || r.number(4)!=3 || r.number(4)!=522 || r.number(4)!=1017 ||
       r.number(2)!=5 || r.number(2)!=6 || r.number(2)!=1 || r.number(4)!=0 ||
       r.string()!="UE5" || r.number(4)!=3 || r.number(4)!=93 || !r.ok ||
       r.at>bytes.size() || 93*20>bytes.size()-r.at)return false;
    r.at+=93*20;
    const auto expected=std::string("/Game/Mods/MCD2Graphics/")+std::string(className)+"."+
                        std::string(className)+"_C";
    if(r.string()!=expected || r.number(1)!=0 || !r.ok)return false;
    const auto prefix=r.at;
    std::array<std::uint8_t,N> next{};
    std::array<bool,N/4> seen{};
    for(std::size_t count=0;count<=N/4;++count){
        const auto name=r.string();if(!r.ok)return false;
        if(name=="None"){
            if(r.number(4)!=0 || !r.ok || r.at!=bytes.size())return false;
            out=next;if(prefixEnd)*prefixEnd=prefix;return true;
        }
        if(name.size()<2 || name.size()>3 || name[0]!='W')return false;
        unsigned index=0;
        for(std::size_t n=1;n<name.size();++n){
            if(name[n]<'0'||name[n]>'9')return false;
            index=index*10+unsigned(name[n]-'0');
        }
        if(name!="W"+std::to_string(index) || index>=seen.size() || seen[index])return false;
        seen[index]=true;
        if(r.string()!="IntProperty" || r.number(4)!=0 || r.number(4)!=4 || r.number(1)!=0)return false;
        const auto value=r.number(4);if(!r.ok)return false;
        for(unsigned n=0;n<4;++n)next[index*4+n]=std::uint8_t(value>>(8*n));
    }
    return false;
}
inline void number(SlotBytes& b,std::uint32_t value,unsigned width=4){
    for(unsigned n=0;n<width;++n)b.push_back(std::uint8_t(value>>(8*n)));
}
inline void string(SlotBytes& b,const std::string& value){
    number(b,unsigned(value.size()+1));b.insert(b.end(),value.begin(),value.end());b.push_back(0);
}
template<std::size_t N>
bool encode(std::span<const std::uint8_t> seed,std::string_view className,
            const std::array<std::uint8_t,N>& words,SlotBytes& out){
    std::array<std::uint8_t,N> old{};std::size_t prefix=0;
    if(!parse(seed,className,old,&prefix))return false;
    SlotBytes next(seed.begin(),seed.begin()+prefix);
    for(std::size_t at=0;at<N;at+=4){
        const auto value=recordWord(words,at);if(!value)continue;
        string(next,"W"+std::to_string(at/4));string(next,"IntProperty");
        number(next,0);number(next,4);number(next,0,1);number(next,value);
    }
    string(next,"None");number(next,0);
    out=std::move(next);return true;
}
} // namespace menu_save_detail

// Persistence receipt only. Capability, runtime context, history reset and
// active feature acknowledgements use a different channel.
struct MenuAuthority {
    std::uint32_t session=0;
    StoreStatus load=StoreStatus::Missing;
    DecodedGraphicsRecord current;
    MenuCommitReceipt receipt;
    bool operator==(const MenuAuthority&)const=default;
};
using MenuAuthorityBytes=std::array<std::uint8_t,176>;
inline bool validAuthority(const MenuAuthority& a){
    if(!validRevision(a.session) || unsigned(a.load)>unsigned(StoreStatus::IoError))return false;
    const auto& r=a.receipt;
    if(r.sequence==0)return r.session==0 && r.status==MenuCommitStatus::Invalid &&
        r.store==StoreStatus::Invalid && r.stamp==RecordStamp{};
    bool storeMatches=false;
    switch(r.status){
        case MenuCommitStatus::Committed:case MenuCommitStatus::Unchanged:storeMatches=r.store==StoreStatus::Ok;break;
        case MenuCommitStatus::DurabilityUncertain:storeMatches=r.store==StoreStatus::PublishedSyncUncertain;break;
        case MenuCommitStatus::NeedsInitialization:storeMatches=r.store==StoreStatus::Missing;break;
        case MenuCommitStatus::Conflict:case MenuCommitStatus::SourceChanged:case MenuCommitStatus::Superseded:
            storeMatches=r.store==StoreStatus::Conflict;break;
        case MenuCommitStatus::Busy:storeMatches=r.store==StoreStatus::Busy;break;
        case MenuCommitStatus::StoreFailure:storeMatches=r.store==StoreStatus::Missing ||
            r.store==StoreStatus::Invalid || r.store==StoreStatus::IoError;break;
        case MenuCommitStatus::Invalid:case MenuCommitStatus::WrongSession:case MenuCommitStatus::OutOfOrder:
        case MenuCommitStatus::SequenceReuse:storeMatches=r.store==StoreStatus::Invalid;break;
    }
    return storeMatches && r.session==a.session && validRevision(r.sequence) &&
        unsigned(r.status)<=unsigned(MenuCommitStatus::Superseded) &&
        unsigned(r.store)<=unsigned(StoreStatus::PublishedSyncUncertain) &&
        (!r.stamp.revision || validRevision(r.stamp.revision)) &&
        (r.stamp.revision || !r.stamp.checksum) &&
        ((r.status!=MenuCommitStatus::Committed && r.status!=MenuCommitStatus::Unchanged &&
          r.status!=MenuCommitStatus::DurabilityUncertain) || validRevision(r.stamp.revision));
}
inline bool encodeMenuAuthority(const MenuAuthority& a,MenuAuthorityBytes& out){
    if(!validAuthority(a))return false;
    MenuAuthorityBytes b{};
    auto put=[&](std::size_t word,std::uint32_t v){for(unsigned n=0;n<4;++n)b[word*4+n]=std::uint8_t(v>>(8*n));};
    put(0,0x3244434d);put(1,0x00315341);put(2,1);put(3,unsigned(b.size()));
    put(5,a.session);put(6,a.receipt.sequence);put(7,unsigned(a.receipt.status));
    put(8,unsigned(a.load));put(9,a.receipt.stamp.revision);put(10,a.receipt.stamp.checksum);
    put(11,unsigned(a.receipt.store));
    if(a.load==StoreStatus::Ok){
        GraphicsRecord record;if(!encodeGraphicsRecord(a.current.intent,record) ||
            a.current.bootstrap.stamp!=RecordStamp{a.current.intent.revision,recordWord(record,16)} ||
            a.current.bootstrap.requested!=requestedOwner(a.current.intent) ||
            a.current.bootstrap.fgEnabled!=a.current.intent.fgEnabled)return false;
        std::copy(record.begin(),record.end(),b.begin()+48);
    }
    put(4,recordChecksum(b));out=b;return true;
}
inline bool decodeMenuAuthority(std::span<const std::uint8_t> b,MenuAuthority& out){
    if(b.size()!=176 || recordWord(b,0)!=0x3244434d || recordWord(b,4)!=0x00315341 ||
       recordWord(b,8)!=1 || recordWord(b,12)!=176 || recordWord(b,16)!=recordChecksum(b))return false;
    MenuAuthority a;a.session=recordWord(b,20);a.load=StoreStatus(recordWord(b,32));
    a.receipt={recordWord(b,24)?a.session:0,recordWord(b,24),MenuCommitStatus(recordWord(b,28)),
        StoreStatus(recordWord(b,44)),{recordWord(b,36),recordWord(b,40)}};
    if(!validAuthority(a))return false;
    if(a.load==StoreStatus::Ok){if(!decodeGraphicsRecord(b.subspan(48),a.current))return false;}
    else if(std::any_of(b.begin()+48,b.end(),[](auto v){return v!=0;}))return false;
    out=a;return true;
}
inline bool decodeMenuRequestSave(std::span<const std::uint8_t> save,MenuRequestBytes& out){
    MenuRequestBytes b{};MenuRequest r;
    if(!menu_save_detail::parse(save,menu_save_detail::requestClass,b) || !decodeMenuRequest(b,r))return false;
    out=b;return true;
}

// One worker-owned writer. Initialization migrates a stable legacy snapshot
// only while the authority is missing; after that it accepts consolidated
// requests only and never overwrites them through legacy preferences.
class MenuSaveTransport {
    std::filesystem::path saves_;
    GraphicsStore store_;
    MenuRequestWriter<> writer_;
    ObservedSettings migration_;
    std::uint32_t session_;
    MenuCommitReceipt receipt_;
public:
    MenuSaveTransport(std::filesystem::path saves,std::filesystem::path authority,std::uint32_t session)
        :saves_(std::move(saves)),store_(std::move(authority)),writer_(session,store_),session_(session){}
    MenuAuthority poll(){
        DecodedGraphicsRecord current;auto loaded=store_.load(current);
        if(loaded.status==StoreStatus::Missing){
            LegacySnapshot legacy;
            if(readLegacySnapshot(saves_,legacy))migration_.observe(legacy,store_,[&](const auto& expected){
                LegacySnapshot fresh;return readLegacySnapshot(saves_,fresh)&&fresh==expected;
            },false);else migration_.invalidate();
            loaded=store_.load(current);
        }
        SlotBytes request;MenuRequestBytes words;
        if(loaded.status==StoreStatus::Ok && readObservedSlot(saves_/"MCD2GraphicsProviderRequest.sav",request) &&
           decodeMenuRequestSave(request,words)){
            auto next=writer_.consume(words,[&](auto){
                SlotBytes fresh;return readObservedSlot(saves_/"MCD2GraphicsProviderRequest.sav",fresh)&&fresh==request;
            });
            // Wrong-session requests cannot replace the current session's ACK.
            if(next.session==session_)receipt_=next;
            loaded=store_.load(current);
        }
        const auto visible=unsigned(loaded.status)<=unsigned(StoreStatus::IoError)?loaded.status:StoreStatus::IoError;
        MenuAuthority state{session_,visible,current,receipt_};
        MenuAuthorityBytes wordsOut;SlotBytes seed,encoded;
        const auto path=saves_/"MCD2GraphicsProviderAuthority.sav";
        if(encodeMenuAuthority(state,wordsOut) && readObservedSlot(path,seed) &&
           menu_save_detail::encode(seed,menu_save_detail::authorityClass,wordsOut,encoded) && seed!=encoded){
            // A persistent lock serializes only this mod-owned response slot.
            store_detail::WriterLock lock(saves_/"MCD2GraphicsProviderAuthority.lock");
            SlotBytes fresh;
            if(lock.status==StoreStatus::Ok && readObservedSlot(path,fresh) && fresh==seed){
                const auto pending=saves_/"MCD2GraphicsProviderAuthority.pending";
                if(store_detail::discardPending(pending) && store_detail::writeFlushed(pending,encoded)){
                    SlotBytes staged;
                    if(readObservedSlot(pending,staged)&&staged==encoded&&store_detail::replace(pending,path))
                        store_detail::syncDirectory(saves_);
                }
            }
        }
        return state;
    }
};
} // namespace mcd2::providers
