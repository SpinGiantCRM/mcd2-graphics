#pragma once
#include "graphics_store.hpp"

namespace mcd2::providers {

// One complete menu transaction. Session/sequence describe transport, never
// provider capability, a frame token or successful rendering activation.
struct MenuRequest {
    std::uint32_t session = 0, sequence = 0;
    RecordStamp expected;
    GraphicsIntent desired;
    bool operator==(const MenuRequest&) const = default;
};
using MenuRequestBytes = std::array<std::uint8_t,168>;
inline constexpr std::array<std::uint8_t,8> menuRequestMagic{'M','C','D','2','R','Q','1',0};
inline bool validMenuRequest(const MenuRequest& r) {
    return validRevision(r.session) && validRevision(r.sequence) &&
        validRevision(r.expected.revision) && r.expected.revision < 0x7fffffffu &&
        r.desired.revision == r.expected.revision+1 &&
        validIntent(r.desired) && validMigrationStamp(r.desired);
}
inline void setRequestWord(MenuRequestBytes& b,std::size_t at,std::uint32_t value) {
    for(unsigned n=0;n!=4;++n)b[at+n]=std::uint8_t(value>>(8*n));
}
inline bool encodeMenuRequest(const MenuRequest& r,MenuRequestBytes& out) {
    GraphicsRecord record;
    if(!validMenuRequest(r)||!encodeGraphicsRecord(r.desired,record))return false;
    MenuRequestBytes b{};
    std::copy(menuRequestMagic.begin(),menuRequestMagic.end(),b.begin());
    setRequestWord(b,8,1);setRequestWord(b,12,unsigned(b.size()));
    setRequestWord(b,20,r.session);setRequestWord(b,24,r.sequence);
    setRequestWord(b,28,r.expected.revision);setRequestWord(b,32,r.expected.checksum);
    // Word 36 is reserved zero. The inner record retains its own checksum.
    std::copy(record.begin(),record.end(),b.begin()+40);
    setRequestWord(b,16,recordChecksum(b));out=b;return true;
}
inline bool decodeMenuRequest(std::span<const std::uint8_t> b,MenuRequest& out) {
    if(b.size()!=MenuRequestBytes{}.size()||
        !std::equal(menuRequestMagic.begin(),menuRequestMagic.end(),b.begin())||
        recordWord(b,8)!=1||recordWord(b,12)!=b.size()||recordWord(b,36)||
        recordWord(b,16)!=recordChecksum(b))return false;
    DecodedGraphicsRecord inner;
    if(!decodeGraphicsRecord(b.subspan(40),inner))return false;
    MenuRequest r{recordWord(b,20),recordWord(b,24),
        {recordWord(b,28),recordWord(b,32)},inner.intent};
    if(!validMenuRequest(r))return false;
    out=r;return true;
}

enum class MenuCommitStatus {
    Invalid, WrongSession, OutOfOrder, SequenceReuse, NeedsInitialization,
    Conflict, SourceChanged, Busy, StoreFailure, Committed, Unchanged,
    DurabilityUncertain, Superseded
};
struct MenuCommitReceipt {
    std::uint32_t session = 0, sequence = 0;
    MenuCommitStatus status = MenuCommitStatus::Invalid;
    StoreStatus store = StoreStatus::Invalid;
    RecordStamp stamp;
    bool operator==(const MenuCommitReceipt&) const = default;
};
inline bool sameMenuPreferences(GraphicsIntent a,GraphicsIntent b) {
    a.revision=b.revision=1;a.migratedFrom=b.migratedFrom={};return a==b;
}

// Own exactly one instance on the settings worker. The UI sends requests; it
// must not write the committed record. Legacy migration/explicit corrupt-file
// recovery are separate operations performed before this writer is admitted.
// No SDK calls, filesystem discovery or capability resolution occur here.
template<class Store = GraphicsStore>
class MenuRequestWriter {
    std::uint32_t session_;
    const Store& store_;
    bool seen_ = false;
    MenuRequest last_;
    MenuCommitReceipt receipt_;
    GraphicsIntent accepted_;
    MenuCommitReceipt result(const MenuRequest& r,MenuCommitStatus status,
                             StoreStatus store,RecordStamp stamp={})const {
        return {r.session,r.sequence,status,store,stamp};
    }
public:
    MenuRequestWriter(std::uint32_t session,const Store& store):session_(session),store_(store){}
    MenuRequestWriter(const MenuRequestWriter&)=delete;
    MenuRequestWriter& operator=(const MenuRequestWriter&)=delete;
    template<class Recheck>
    MenuCommitReceipt consume(std::span<const std::uint8_t> bytes,Recheck sourceStillCurrent) {
        MenuRequest r;
        if(!decodeMenuRequest(bytes,r))return {};
        if(!validRevision(session_)||r.session!=session_)
            return result(r,MenuCommitStatus::WrongSession,StoreStatus::Invalid);
        if(seen_ && r.sequence<last_.sequence)
            return result(r,MenuCommitStatus::OutOfOrder,StoreStatus::Invalid);
        if(seen_ && r.sequence==last_.sequence) {
            if(r!=last_)return result(r,MenuCommitStatus::SequenceReuse,StoreStatus::Invalid);
            if(receipt_.status!=MenuCommitStatus::Busy) {
                if(receipt_.status==MenuCommitStatus::Committed ||
                   receipt_.status==MenuCommitStatus::Unchanged ||
                   receipt_.status==MenuCommitStatus::DurabilityUncertain) {
                    DecodedGraphicsRecord now;
                    const auto loaded=store_.load(now);
                    if(loaded.status!=StoreStatus::Ok)
                        return result(r,MenuCommitStatus::StoreFailure,loaded.status);
                    if(loaded.stamp!=receipt_.stamp || now.intent!=accepted_)
                        return result(r,MenuCommitStatus::Superseded,StoreStatus::Conflict,loaded.stamp);
                }
                return receipt_; // Lost ACK: never republish a committed request.
            }
        } else {last_=r;seen_=true;}
        DecodedGraphicsRecord current;
        const auto loaded=store_.load(current);
        auto finish=[&](MenuCommitStatus status,StoreStatus store,RecordStamp stamp={}) {
            receipt_=result(r,status,store,stamp);return receipt_;
        };
        if(loaded.status==StoreStatus::Missing)
            return finish(MenuCommitStatus::NeedsInitialization,loaded.status);
        if(loaded.status!=StoreStatus::Ok)
            return finish(MenuCommitStatus::StoreFailure,loaded.status);
        if(loaded.stamp!=r.expected)
            return finish(MenuCommitStatus::Conflict,StoreStatus::Conflict,loaded.stamp);
        if(current.intent.migratedFrom!=r.desired.migratedFrom)
            return finish(MenuCommitStatus::Invalid,StoreStatus::Invalid,loaded.stamp);
        // A consolidated source can still change between read and commit.
        // Check all request bytes here; the store then enforces the base stamp
        // under its lock. A stale/busy UI must reload instead of editing an ID.
        if(!sourceStillCurrent(bytes))
            return finish(MenuCommitStatus::SourceChanged,StoreStatus::Conflict,loaded.stamp);
        if(sameMenuPreferences(current.intent,r.desired)) {
            accepted_=current.intent;
            return finish(MenuCommitStatus::Unchanged,StoreStatus::Ok,loaded.stamp);
        }
        const auto committed=store_.publish(r.desired,r.expected);
        switch(committed.status) {
            case StoreStatus::Ok:
                accepted_=r.desired;return finish(MenuCommitStatus::Committed,committed.status,committed.stamp);
            case StoreStatus::PublishedSyncUncertain:
                accepted_=r.desired;return finish(MenuCommitStatus::DurabilityUncertain,committed.status,committed.stamp);
            case StoreStatus::Busy:return finish(MenuCommitStatus::Busy,committed.status,committed.stamp);
            case StoreStatus::Conflict:return finish(MenuCommitStatus::Conflict,committed.status,committed.stamp);
            default:return finish(MenuCommitStatus::StoreFailure,committed.status,committed.stamp);
        }
    }
};
} // namespace mcd2::providers
