#include "../src/providers/menu_save_transport.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <random>
using namespace mcd2::providers;
using namespace menu_save_detail;
static SlotBytes seed(const std::string& name){
 SlotBytes b;for(auto v:{0x53415647u,3u,522u,1017u})number(b,v);
 for(auto v:{5u,6u,1u})number(b,v,2);
 number(b,0);string(b,"UE5");number(b,3);number(b,93);b.resize(b.size()+93*20);
 string(b,"/Game/Mods/MCD2Graphics/"+name+"."+name+"_C");number(b,0,1);
 string(b,"None");number(b,0);return b;
}
static void write(const std::filesystem::path& p,const SlotBytes& b){
 std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());assert(f.good());
}
static DecodedGraphicsRecord decoded(GraphicsIntent i){
 GraphicsRecord b;DecodedGraphicsRecord r;assert(encodeGraphicsRecord(i,b)&&decodeGraphicsRecord(b,r));return r;
}
int main(){
 GraphicsIntent i; i.revision=1;
 auto base=decoded(i);auto desired=i;desired.revision=2;desired.sr=SrProvider::AmdFsr;
 desired.fg=FgProvider::Amd;desired.fgEnabled=true;desired.latency=LatencyProvider::RadeonAntiLag2;
 const auto request=MenuRequest{123,1,base.bootstrap.stamp,desired};
 MenuRequestBytes original;assert(encodeMenuRequest(request,original));
 auto requestSeed=seed(std::string(requestClass));SlotBytes save;assert(encode(requestSeed,requestClass,original,save));
 MenuRequestBytes out{};assert(decodeMenuRequestSave(save,out)&&out==original);
 // All bit patterns are transported, not constrained to nonnegative values.
 std::array<std::uint8_t,168> bits;bits.fill(255);SlotBytes signedSave;
 assert(encode(requestSeed,requestClass,bits,signedSave));
 assert(parse(signedSave,requestClass,out)&&out==bits);
 for(std::size_t n=0;n<save.size();++n){
  auto sentinel=out;assert(!decodeMenuRequestSave(std::span(save).first(n),out)&&out==sentinel);
 }
 auto trailing=save;trailing.push_back(0);assert(!decodeMenuRequestSave(trailing,out));
 auto wrong=save;std::size_t prefix=0;assert(parse(save,requestClass,out,&prefix));
 // Duplicate and unknown/noncanonical names, wrong type/flags/size/array.
 SlotBytes property;
 string(property,"W0");string(property,"IntProperty");number(property,0);number(property,4);number(property,0,1);number(property,0x3244434d);
 auto duplicate=save;duplicate.insert(duplicate.begin()+prefix,property.begin(),property.end());
 assert(!decodeMenuRequestSave(duplicate,out));
 for(const auto& name:{"W00","W42","W-1","SchemaVersion","W0x"}){
  auto broken=requestSeed;broken.resize(broken.size()-13); // FString None (9), terminator word (4).
  SlotBytes prop;string(prop,name);string(prop,"IntProperty");number(prop,0);number(prop,4);number(prop,0,1);number(prop,123);
  broken.insert(broken.end(),prop.begin(),prop.end());string(broken,"None");number(broken,0);
  assert(!parse(broken,requestClass,out));
 }
 for(auto offset:{prefix+7+16,prefix+7+16+4,prefix+7+16+8}){ // array, size, flags of W0.
  auto broken=save;broken[offset]^=1;assert(!decodeMenuRequestSave(broken,out));
 }
 MenuAuthority a{123,StoreStatus::Ok,base,{}};MenuAuthorityBytes b;
 assert(encodeMenuAuthority(a,b));MenuAuthority read;assert(decodeMenuAuthority(b,read)&&read==a);
 unsigned damaged=0;
 for(std::size_t n=0;n<b.size();++n)for(unsigned bit=0;bit<8;++bit){
  auto bad=b;bad[n]^=1<<bit;const auto held=read;assert(!decodeMenuAuthority(bad,read)&&read==held);++damaged;
 }
 for(auto load:{StoreStatus::Missing,StoreStatus::Invalid,StoreStatus::IoError}){
  a.load=load;assert(encodeMenuAuthority(a,b)&&decodeMenuAuthority(b,read));
  assert(std::all_of(b.begin()+48,b.end(),[](auto v){return !v;}));
 }
 a.load=StoreStatus::Busy;assert(!encodeMenuAuthority(a,b));a.load=StoreStatus::Ok;
 a.receipt={123,1,MenuCommitStatus::Committed,StoreStatus::Ok,base.bootstrap.stamp};
 assert(encodeMenuAuthority(a,b)&&decodeMenuAuthority(b,read)&&read==a);
 a.receipt.session=124;assert(!encodeMenuAuthority(a,b));a.receipt.session=123;
 a.receipt.store=StoreStatus::IoError;assert(!encodeMenuAuthority(a,b));a.receipt.store=StoreStatus::Ok;
 a.receipt.stamp={};assert(!encodeMenuAuthority(a,b));
 a.receipt={};auto authoritySeed=seed(std::string(authorityClass));
 assert(encodeMenuAuthority(a,b));SlotBytes snapshot;
 assert(encode(authoritySeed,authorityClass,b,snapshot));MenuAuthorityBytes loaded;
 assert(parse(snapshot,authorityClass,loaded)&&loaded==b&&decodeMenuAuthority(loaded,read));
 auto root=std::filesystem::temp_directory_path()/("mcd2-menu-transport-"+std::to_string(std::random_device{}()));
 std::filesystem::create_directories(root/"saves");std::filesystem::create_directory(root/"authority");
 auto saves=root/"saves";GraphicsStore store(root/"authority");assert(store.publish(i,{}).status==StoreStatus::Ok);
 write(saves/"MCD2GraphicsProviderAuthority.sav",authoritySeed);write(saves/"MCD2GraphicsProviderRequest.sav",save);
 MenuSaveTransport worker(saves,root/"authority",123);auto committed=worker.poll();
 assert(committed.current.intent==desired&&committed.receipt.status==MenuCommitStatus::Committed);
 SlotBytes actual;assert(readObservedSlot(saves/"MCD2GraphicsProviderAuthority.sav",actual));
 assert(parse(actual,authorityClass,loaded)&&decodeMenuAuthority(loaded,read)&&read==committed);
 const auto modified=std::filesystem::last_write_time(saves/"MCD2GraphicsProviderAuthority.sav");
 assert(worker.poll()==committed);
 assert(std::filesystem::last_write_time(saves/"MCD2GraphicsProviderAuthority.sav")==modified); // No per-poll publication/revision churn.
 // Legacy files are neither needed nor rewritten after migration.
 assert(!std::filesystem::exists(saves/"MCD2GraphicsSettings.sav"));
 auto restart=MenuSaveTransport(saves,root/"authority",124);auto after=restart.poll();
 assert(after.current.intent==desired&&after.receipt.sequence==0);
 assert(readObservedSlot(saves/"MCD2GraphicsProviderAuthority.sav",actual));
 assert(parse(actual,authorityClass,loaded)&&decodeMenuAuthority(loaded,read)&&read.session==124);
 // Corrupt authority remains corrupt, never recreated from a request/legacy fallback.
 write(store.committedPath(),SlotBytes(128,0));auto corrupt=worker.poll();assert(corrupt.load==StoreStatus::Invalid);
 assert(std::filesystem::file_size(store.committedPath())==128);
 std::filesystem::remove_all(root);
 std::cout<<"Menu save transport: signed words, strict framing, "<<damaged<<" damaged responses and real worker round trip pass\n";
}
