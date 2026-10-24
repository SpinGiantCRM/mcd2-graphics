#include "../src/providers/fg_menu_projection.hpp"
#include "../src/providers/menu_requests.hpp"
#include <cassert>
#include <random>
using namespace mcd2::providers;
static DecodedGraphicsRecord decode(const GraphicsIntent& i){
 GraphicsRecord bytes;DecodedGraphicsRecord r;assert(encodeGraphicsRecord(i,bytes)&&decodeGraphicsRecord(bytes,r));return r;
}
int main(){
 GraphicsIntent i;assert(i.schema==6);
 GraphicsRecord bytes;assert(encodeGraphicsRecord(i,bytes));
 assert(bytes[6]=='6'&&recordWord(bytes,24)==6&&bytes.size()==128);
 static_assert(sizeof(MenuRequestBytes)==168);
 // Every allowed additive allocation survives persistence, transport and source
 // projection without enabling the saved master switch.
 for(unsigned d=0;d<=5;++d)for(unsigned f=0;f<=5;++f){
  auto intent=i;const auto before=intent;
  const bool valid=d+f>=1&&d+f<=5;
  assert(setFgAllocation(intent,d,f)==valid);
  if(!valid){assert(intent==before);continue;}
  assert(!intent.fgEnabled&&intent.schema==6&&intent.requestedMultiplier==1+d+f);
  assert(fgAllocation(intent)==(FgAllocation{d,f}));
  intent.sr=SrProvider::AmdFsr;intent.revision=8;
  auto r=decode(intent);assert(r.intent==intent);
  assert(encodeGraphicsRecord(intent,bytes));
  assert(recordWord(bytes,84)==(unsigned(intent.fgStrategy)|(d<<8)|(f<<16)));
  FgMenuProjection p;assert(projectFgMenu(r,p)&&p.pairEligible);
  if(d&&f){
   assert(fgMenuAllocationSupported(p,124,5));
   assert(!fgMenuAllocationSupported(p,0,5));
   assert(!fgMenuAllocationSupported(p,124,0));
  }
  assert(p.dlssGenerated==d&&p.fsrGenerated==f&&p.stamp==r.bootstrap.stamp);
  assert(fgMenuSourceMatches(p,8,unsigned(intent.fg),0,d+f-1,unsigned(intent.fgStrategy),d,f));
  assert(!fgMenuSourceMatches(p,8,unsigned(intent.fg),0,d+f-1,unsigned(intent.fgStrategy),d+1,f));

  MenuRequest req{123,1,{7,0x12345678},intent};MenuRequestBytes wire;MenuRequest parsed;
  assert(encodeMenuRequest(req,wire)&&decodeMenuRequest(wire,parsed)&&parsed==req);
  for(unsigned offset:{24u,80u,84u,88u}){
   auto broken=bytes;setRecordWord(broken,offset,0xffffffffu);setRecordWord(broken,16,recordChecksum(broken));
   DecodedGraphicsRecord held=r;assert(!decodeGraphicsRecord(broken,held)&&held==r);
  }
  for(unsigned packed:{recordWord(bytes,84)|0x01000000u,(recordWord(bytes,84)&0xffff00ffu)|0x00000600u,(recordWord(bytes,84)&0xff00ffffu)|0x00060000u}){
   auto broken=bytes;setRecordWord(broken,84,packed);setRecordWord(broken,16,recordChecksum(broken));
   DecodedGraphicsRecord held=r;assert(!decodeGraphicsRecord(broken,held)&&held==r);
  }
  auto wrong=intent;wrong.fg=FgProvider::Intel;assert(!validIntent(wrong));
 }
 auto exact=i;exact.sr=SrProvider::AmdFsr;assert(setFgAllocation(exact,2,3));
 FgMenuProjection exactProjection;assert(projectFgMenu(decode(exact),exactProjection));
 assert(fgMenuAllocationSupported(exactProjection,124,2));
 assert(!fgMenuAllocationSupported(exactProjection,124,1));
 // Schema 5 source identity remains byte exact until an explicit UI edit.
 for(auto strategy:{FgStrategy::Single,FgStrategy::NativeMfg,FgStrategy::Hybrid,FgStrategy::SoftwareMfg}){
  GraphicsIntent old;old.schema=5;old.revision=42;old.fgStrategy=strategy;
  old.fg=strategy==FgStrategy::SoftwareMfg?FgProvider::Amd:FgProvider::Nvidia;
  old.requestedMultiplier=strategy==FgStrategy::Single?2:4;old.fgEnabled=true;
  const auto r=decode(old);GraphicsRecord first,again;
  assert(encodeGraphicsRecord(old,first)&&first[6]=='5'&&recordWord(first,24)==5);
  assert(encodeGraphicsRecord(r.intent,again)&&again==first);
  auto upgraded=old;assert(normalizeFgAllocation(upgraded)&&upgraded.schema==6);
  assert(fgAllocation(upgraded)==fgAllocation(old)&&upgraded.fgEnabled);
  assert(upgraded.revision==old.revision&&upgraded.srPreferences==old.srPreferences);
  assert(decode(upgraded).bootstrap.stamp!=r.bootstrap.stamp);
  auto impossible=old;impossible.dlssGenerated=1;assert(!validIntent(impossible));
  for(unsigned offset:{6u,24u,84u}){
   auto bad=first;
   if(offset==6)bad[6]='6';else setRecordWord(bad,offset,offset==24?6u:0x100u);
   setRecordWord(bad,16,recordChecksum(bad));DecodedGraphicsRecord held;
   assert(!decodeGraphicsRecord(bad,held));
  }
 }
 // Reading a legacy store must retain its checksum for running-session identity
 // and compare/exchange. An unrelated edit upgrades only its format and field.
 namespace fs=std::filesystem;std::random_device random;fs::path root;
 for(unsigned n=0;n<10;++n){root=fs::temp_directory_path()/("mcd2-allocation-"+std::to_string(random()));if(fs::create_directory(root))break;root.clear();}
 assert(!root.empty());
 struct Cleanup {fs::path path;~Cleanup(){std::error_code error;fs::remove_all(path,error);}} cleanup{root};
 GraphicsIntent legacy;legacy.schema=5;legacy.revision=17;legacy.sr=SrProvider::AmdFsr;
 legacy.fgStrategy=FgStrategy::Hybrid;legacy.requestedMultiplier=4;
 GraphicsStore store(root);const auto stored=store.publish(legacy,{});assert(stored.status==StoreStatus::Ok);
 DecodedGraphicsRecord loaded;assert(store.load(loaded).status==StoreStatus::Ok&&loaded.intent==legacy);
 const auto before=loaded;GraphicsRecord original;assert(encodeGraphicsRecord(legacy,original));
 const auto startup=selectStartup(original,123);assert(matchesStartup(startup,original,123));
 FgMenuProjection oldProjection;assert(projectFgMenu(loaded,oldProjection)&&oldProjection.stamp==stored.stamp);
 assert(store.load(loaded).stamp==stored.stamp&&loaded==before);
 auto changed=legacy;changed.schema=6;changed.revision++;changed.hdr.uiNits++;
 MenuRequest request{123,1,stored.stamp,changed};MenuRequestBytes requestBytes;assert(encodeMenuRequest(request,requestBytes));
 MenuRequestWriter writer(123,store);const auto receipt=writer.consume(requestBytes,[](auto){return true;});
 assert(receipt.status==MenuCommitStatus::Committed&&receipt.stamp!=stored.stamp);
 assert(store.load(loaded).status==StoreStatus::Ok&&loaded.intent==changed);
 assert(fgAllocation(loaded.intent)==fgAllocation(legacy)&&!loaded.intent.dlssGenerated&&!loaded.intent.fsrGenerated);
 assert(store.publish(changed,stored.stamp).status==StoreStatus::Conflict);
 GraphicsRecord upgraded;assert(encodeGraphicsRecord(loaded.intent,upgraded)&&upgraded[6]=='6');
 assert(!matchesStartup(startup,upgraded,123));

}
