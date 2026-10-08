#include "../src/providers/menu_requests.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <random>

using namespace mcd2::providers;
static DecodedGraphicsRecord decoded(const GraphicsIntent& intent) {
    GraphicsRecord b;DecodedGraphicsRecord out;
    assert(encodeGraphicsRecord(intent,b)&&decodeGraphicsRecord(b,out));return out;
}
static MenuRequest request(const DecodedGraphicsRecord& base,unsigned sequence=1) {
    auto desired=base.intent;desired.revision++;
    return {123,sequence,base.bootstrap.stamp,desired};
}
static MenuRequestBytes wire(const MenuRequest& r) {
    MenuRequestBytes bytes;assert(encodeMenuRequest(r,bytes));return bytes;
}
struct FakeStore {
    mutable DecodedGraphicsRecord current;
    mutable unsigned writes=0;
    StoreStatus readStatus=StoreStatus::Ok,publishStatus=StoreStatus::Ok;
    StoreResult load(DecodedGraphicsRecord& out)const {
        if(readStatus==StoreStatus::Ok)out=current;
        return {readStatus,readStatus==StoreStatus::Ok?current.bootstrap.stamp:RecordStamp{}};
    }
    StoreResult publish(const GraphicsIntent& i,RecordStamp expected)const {
        ++writes;assert(expected==current.bootstrap.stamp);
        if(publishStatus==StoreStatus::Ok||publishStatus==StoreStatus::PublishedSyncUncertain)
            current=decoded(i);
        return {publishStatus,current.bootstrap.stamp};
    }
};
int main() {
    LegacyV4 legacy;legacy.srRevision=22;legacy.displayRevision=3;legacy.fgRevision=31;
    GraphicsIntent initial;assert(migrateV4(legacy,initial));const auto baseline=decoded(initial);
    auto r=request(baseline);r.desired.sr=SrProvider::AmdFsr;
    r.desired.srPreferences[1]={Quality::Custom,7300,7300};
    r.desired.fgEnabled=true;r.desired.fg=FgProvider::Amd;
    r.desired.latency=LatencyProvider::RadeonAntiLag2;r.desired.latencyMode=LatencyMode::On;
    const auto original=wire(r);MenuRequest read;
    assert(decodeMenuRequest(original,read)&&read==r);
    unsigned rejected=0;
    for(std::size_t n=0;n<original.size();++n)for(unsigned bit=0;bit<8;++bit) {
        auto broken=original;broken[n]^=1u<<bit;auto held=read;
        assert(!decodeMenuRequest(broken,read)&&read==held);++rejected;
    }
    for(std::size_t n=0;n<original.size();++n) {
        assert(!decodeMenuRequest(std::span(original).first(n),read));++rejected;
    }
    auto invalid=original;setRequestWord(invalid,36,1);setRequestWord(invalid,16,recordChecksum(invalid));
    assert(!decodeMenuRequest(invalid,read));
    // Recomputed outer checksums must not excuse bad inner semantics/framing.
    for(const auto offset:{8u,12u,20u,24u,28u,40u+32u}) {
        auto forged=original;setRequestWord(forged,offset,0xffffffffu);
        if(offset>=40) {
            GraphicsRecord inner;std::copy(forged.begin()+40,forged.end(),inner.begin());
            setRequestWord(forged,40+16,recordChecksum(inner));
        }
        setRequestWord(forged,16,recordChecksum(forged));
        assert(!decodeMenuRequest(forged,read));
    }
    for(const auto field:{0u,1u,2u,3u,4u}) {
        auto bad=r;
        if(field==0)bad.session=0;
        if(field==1)bad.sequence=0;
        if(field==2)bad.expected.revision=0;
        if(field==3)bad.desired.revision++;
        if(field==4)bad.expected.revision=0x7fffffff;
        auto held=original;assert(!encodeMenuRequest(bad,held)&&held==original);
    }
    auto stable=[](auto){return true;};
    FakeStore fake{baseline};MenuRequestWriter writer(123,fake);
    auto wrong=r;wrong.session=124;
    assert(writer.consume(wire(wrong),stable).status==MenuCommitStatus::WrongSession&&!fake.writes);
    auto receipt=writer.consume(original,stable);
    assert(receipt.status==MenuCommitStatus::Committed&&fake.writes==1);
    assert(fake.current.intent==r.desired&&fake.current.bootstrap.requested==PresentationOwner::AmdFidelityFX);
    assert(fake.current.intent.srPreferences[0]==initial.srPreferences[0]);
    assert(writer.consume(original,stable)==receipt&&fake.writes==1); // Lost acknowledgement.
    auto reused=r;reused.desired.hdr.uiNits++;
    assert(writer.consume(wire(reused),stable).status==MenuCommitStatus::SequenceReuse&&fake.writes==1);
    auto noOp=request(fake.current,2);
    auto unchanged=writer.consume(wire(noOp),stable);
    assert(unchanged.status==MenuCommitStatus::Unchanged&&fake.writes==1);
    assert(unchanged.stamp==receipt.stamp&&writer.consume(wire(noOp),stable)==unchanged);
    assert(writer.consume(original,stable).status==MenuCommitStatus::OutOfOrder);
    auto next=request(fake.current,3);next.desired.fg=FgProvider::Nvidia;
    assert(writer.consume(wire(next),stable).status==MenuCommitStatus::Committed);
    assert(fake.current.intent.sr==SrProvider::AmdFsr&&fake.current.bootstrap.requested==PresentationOwner::NvidiaStreamline);
    auto external=fake.current.intent;external.revision++;external.hdr.uiNits++;
    fake.current=decoded(external);
    assert(writer.consume(wire(next),stable).status==MenuCommitStatus::Superseded);
    auto stale=request(baseline,4);
    assert(writer.consume(wire(stale),stable).status==MenuCommitStatus::Conflict);
    auto provenance=request(fake.current,5);provenance.desired.migratedFrom={};
    assert(writer.consume(wire(provenance),stable).status==MenuCommitStatus::Invalid);
    auto interrupted=request(fake.current,6);interrupted.desired.hdr.uiNits++;
    assert(writer.consume(wire(interrupted),[](auto){return false;}).status==MenuCommitStatus::SourceChanged);
    const auto writes=fake.writes;
    assert(writer.consume(wire(interrupted),stable).status==MenuCommitStatus::SourceChanged&&fake.writes==writes);
    for(const auto error:{StoreStatus::Missing,StoreStatus::Invalid,StoreStatus::IoError}) {
        FakeStore failed{baseline};failed.readStatus=error;MenuRequestWriter w(123,failed);
        const auto status=w.consume(original,stable).status;
        assert(status==(error==StoreStatus::Missing?MenuCommitStatus::NeedsInitialization:MenuCommitStatus::StoreFailure));
        assert(!failed.writes);
    }
    for(const auto error:{StoreStatus::Busy,StoreStatus::Conflict,StoreStatus::IoError,StoreStatus::PublishedSyncUncertain}) {
        FakeStore failed{baseline};failed.publishStatus=error;MenuRequestWriter w(123,failed);
        auto outcome=w.consume(original,stable);
        const auto expected=error==StoreStatus::Busy?MenuCommitStatus::Busy:
            error==StoreStatus::Conflict?MenuCommitStatus::Conflict:
            error==StoreStatus::PublishedSyncUncertain?MenuCommitStatus::DurabilityUncertain:MenuCommitStatus::StoreFailure;
        assert(outcome.status==expected&&failed.writes==1);
        if(error==StoreStatus::Busy) {
            assert(w.consume(wire(reused),stable).status==MenuCommitStatus::SequenceReuse);
            failed.publishStatus=StoreStatus::Ok;
            assert(w.consume(original,stable).status==MenuCommitStatus::Committed&&failed.writes==2);
        } else assert(w.consume(original,stable)==outcome&&failed.writes==1);
    }
    // Real on-disk publication and a competing writer between source recheck
    // and commit. No legacy file or game installation is opened.
    namespace fs=std::filesystem;std::random_device random;fs::path root;
    for(unsigned n=0;n<10;++n){root=fs::temp_directory_path()/("mcd2-menu-"+std::to_string(random()));if(fs::create_directory(root))break;root.clear();}
    assert(!root.empty());
    struct Cleanup {fs::path root;~Cleanup(){std::error_code e;fs::remove_all(root,e);}} cleanup{root};
    GraphicsStore store(root);assert(store.publish(initial,{}).status==StoreStatus::Ok);
    MenuRequestWriter real(123,store);
    {
        store_detail::WriterLock held(store.lockPath());assert(held.status==StoreStatus::Ok);
        assert(real.consume(original,stable).status==MenuCommitStatus::Busy);
    }
    assert(real.consume(original,stable).status==MenuCommitStatus::Committed);
    DecodedGraphicsRecord loaded;assert(store.load(loaded).status==StoreStatus::Ok&&loaded.intent==r.desired);
    auto raced=request(loaded,2);raced.desired.hdr.uiNits++;
    auto other=loaded.intent;other.revision++;other.hdr.paperWhiteNits++;
    assert(real.consume(wire(raced),[&](auto){return store.publish(other,loaded.bootstrap.stamp).status==StoreStatus::Ok;}).status==MenuCommitStatus::Conflict);
    assert(store.load(loaded).status==StoreStatus::Ok&&loaded.intent==other);
    auto corruptRequest=request(loaded,3);corruptRequest.desired.hdr.uiNits++;
    {std::ofstream file(store.committedPath(),std::ios::binary);file<<"invalid";}
    assert(real.consume(wire(corruptRequest),stable).status==MenuCommitStatus::StoreFailure);
    {std::ifstream file(store.committedPath(),std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(file)),{});assert(bytes=="invalid");}
    std::cout<<rejected<<" damaged/truncated requests rejected; session, idempotence, CAS, provenance, AMD/mixed intent and store failure gates passed.\n";
}
