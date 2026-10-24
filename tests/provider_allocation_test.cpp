#include "../src/providers/fg_menu_projection.hpp"
#include "../src/providers/menu_requests.hpp"
#include <cassert>
#include <random>
using namespace mcd2::providers;
static DecodedGraphicsRecord decode(const GraphicsIntent& i){
 GraphicsRecord bytes;DecodedGraphicsRecord r;assert(encodeGraphicsRecord(i,bytes)&&decodeGraphicsRecord(bytes,r));return r;
}
int main(){
 GraphicsIntent i;assert(i.schema==7);
 GraphicsRecord bytes;assert(encodeGraphicsRecord(i,bytes));
 assert(bytes[6]=='7'&&recordWord(bytes,24)==7&&bytes.size()==128);
 static_assert(sizeof(MenuRequestBytes)==168);
 // Every allowed additive allocation survives persistence, transport and source
 // projection without enabling the saved master switch.
 for(unsigned d=0;d<=5;++d)for(unsigned f=0;f<=5;++f){
  auto intent=i;const auto before=intent;
  const bool valid=d+f>=1&&d+f<=5;
  assert(setFgAllocation(intent,d,f)==valid);
  if(!valid){assert(intent==before);continue;}
  assert(!intent.fgEnabled&&intent.schema==7&&intent.requestedMultiplier==1+d+f);
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
  for(unsigned packed:{recordWord(bytes,84)|0x01000000u,recordWord(bytes,84)|0x40000000u,recordWord(bytes,84)|0x80000000u,(recordWord(bytes,84)&0xffff00ffu)|0x00000600u,(recordWord(bytes,84)&0xff00ffffu)|0x00060000u}){
   auto broken=bytes;setRecordWord(broken,84,packed);setRecordWord(broken,16,recordChecksum(broken));
   DecodedGraphicsRecord held=r;assert(!decodeGraphicsRecord(broken,held)&&held==r);
  }
  auto wrong=intent;wrong.fg=FgProvider::Intel;assert(!validIntent(wrong));
 }
 auto exact=i;exact.sr=SrProvider::AmdFsr;assert(setFgAllocation(exact,2,3));
 FgMenuProjection exactProjection;assert(projectFgMenu(decode(exact),exactProjection));
 assert(fgMenuAllocationSupported(exactProjection,124,2));
 assert(!fgMenuAllocationSupported(exactProjection,124,1));
 exactProjection.frameOrder=42; // DLSS at 2/6 and 4/6 needs native count2.
 assert(fgMenuAllocationSupported(exactProjection,124,2));
 exactProjection.frameOrder=37; // DLSS at 1/6 and 3/6 needs native count5.
 assert(!fgMenuAllocationSupported(exactProjection,124,2));
 assert(fgMenuAllocationSupported(exactProjection,124,5));
 auto manualA=exact;manualA.frameOrder=42;auto manualB=manualA;manualB.frameOrder=37;
 GraphicsRecord manualABytes,manualBBytes;
 assert(encodeGraphicsRecord(manualA,manualABytes)&&encodeGraphicsRecord(manualB,manualBBytes));
 assert(decode(manualA).bootstrap.stamp!=decode(manualB).bootstrap.stamp);
 assert(!matchesStartup(selectStartup(manualABytes,123),manualBBytes,123));
 // Schema7 order codes carry every provider permutation without changing counts,
 // master enable, or unrelated preferences. Equal counts with a different order
 // still have distinct checksums and source identities.
 unsigned permutations=0;
 for(unsigned d=0;d<=5;++d)for(unsigned f=0;f<=5;++f){
  if((!d&&!f)||d+f>5)continue;
  auto ordered=i;ordered.sr=SrProvider::AmdFsr;ordered.revision=8;
  assert(setFgAllocation(ordered,d,f));
  const auto automatic=decode(ordered);
  for(unsigned mask=0;mask<32;++mask){
   const auto code=32u|mask;auto custom=ordered;custom.frameOrder=code;
   unsigned bits=0;for(unsigned n=mask;n;n>>=1)bits+=n&1;
   const bool allowed=bits==d&&mask<(1u<<(d+f));
   assert(validIntent(custom)==allowed);
   if(!allowed){
    GraphicsRecord broken;assert(encodeGraphicsRecord(ordered,broken));
    setRecordWord(broken,84,recordWord(broken,84)|(code<<24));
    setRecordWord(broken,16,recordChecksum(broken));DecodedGraphicsRecord held=automatic;
    assert(!decodeGraphicsRecord(broken,held)&&held==automatic);continue;
   }
   ++permutations;
   auto r=decode(custom);assert(r.intent==custom&&r.bootstrap.stamp!=automatic.bootstrap.stamp);
   GraphicsRecord automaticWire;assert(encodeGraphicsRecord(ordered,automaticWire));
   GraphicsRecord customWire;assert(encodeGraphicsRecord(custom,customWire));
   assert(!matchesStartup(selectStartup(automaticWire,123),customWire,123));
   FgMenuProjection projection;assert(projectFgMenu(r,projection)&&projection.frameOrder==code);
   assert(fgMenuSourceMatches(projection,8,unsigned(custom.fg),0,d+f-1,unsigned(custom.fgStrategy),d,f,code));
   assert(!fgMenuSourceMatches(projection,8,unsigned(custom.fg),0,d+f-1,unsigned(custom.fgStrategy),d,f,0));
   assert(!fgMenuSourceMatches(projection,8,unsigned(custom.fg),0,d+f-1,unsigned(custom.fgStrategy),d,f,code^1));
   for(unsigned cap=0;cap<=5;++cap){
    const bool supported=mcd2::mfg::native_count_for_mask(d+f+1,mask)<=cap;
    assert(fgMenuAllocationSupported(projection,124,cap)==supported);
    assert(fgMenuAllocationSupported(projection,0,cap)==(supported&&!f));
   }
   GraphicsRecord wire;assert(encodeGraphicsRecord(custom,wire));
   assert((recordWord(wire,84)>>24)==code);
   MenuRequest req{123,1,{7,0x12345678},custom};MenuRequestBytes requestWire;MenuRequest parsed;
   assert(encodeMenuRequest(req,requestWire)&&decodeMenuRequest(requestWire,parsed)&&parsed==req);
   auto normalized=custom;assert(normalizeFgAllocation(normalized)&&normalized==custom);
   auto toggled=custom;toggled.fgEnabled=true;assert(normalizeFgAllocation(toggled)&&toggled.frameOrder==code);
   auto countEdit=custom;assert(setFgAllocation(countEdit,d,f)&&countEdit.frameOrder==0);
   auto legacy=custom;legacy.schema=5;assert(!validIntent(legacy));
   legacy.schema=6;assert(!validIntent(legacy));
   for(unsigned bit:{30u,31u}){
    auto broken=wire;setRecordWord(broken,84,recordWord(broken,84)|(1u<<bit));
    setRecordWord(broken,16,recordChecksum(broken));DecodedGraphicsRecord held=r;
    assert(!decodeGraphicsRecord(broken,held)&&held==r);
   }
  }
  for(unsigned code:{1u,31u,64u,255u,0xffffffffu}){ordered.frameOrder=code;assert(!validIntent(ordered));}
 }
 assert(permutations==62);
 // A custom mask uses the inferred legacy allocation when explicit counts are absent.
 GraphicsIntent implicit;implicit.fgStrategy=FgStrategy::Hybrid;implicit.requestedMultiplier=4;
 implicit.frameOrder=34;assert(validIntent(implicit)&&decode(implicit).intent==implicit);
 implicit.frameOrder=35;assert(!validIntent(implicit));
 // Schema 5/6 source identity remains byte exact until an explicit UI edit.
 for(unsigned schema:{5u,6u})
 for(auto strategy:{FgStrategy::Single,FgStrategy::NativeMfg,FgStrategy::Hybrid,FgStrategy::SoftwareMfg}){
  GraphicsIntent old;old.schema=schema;old.revision=42;old.fgStrategy=strategy;
  old.fg=strategy==FgStrategy::SoftwareMfg?FgProvider::Amd:FgProvider::Nvidia;
  old.requestedMultiplier=strategy==FgStrategy::Single?2:4;old.fgEnabled=true;
  const auto r=decode(old);GraphicsRecord first,again;
  assert(encodeGraphicsRecord(old,first)&&first[6]==('0'+schema)&&recordWord(first,24)==schema);
  assert(encodeGraphicsRecord(r.intent,again)&&again==first);
  auto upgraded=old;assert(normalizeFgAllocation(upgraded)&&upgraded.schema==7);
  assert(fgAllocation(upgraded)==fgAllocation(old)&&upgraded.fgEnabled);
  assert(upgraded.revision==old.revision&&upgraded.srPreferences==old.srPreferences);
  assert(decode(upgraded).bootstrap.stamp!=r.bootstrap.stamp);
  auto impossible=old;impossible.frameOrder=34;assert(!validIntent(impossible));
  for(unsigned offset:{6u,24u,84u}){
   auto bad=first;
   if(offset==6)bad[6]='7';else setRecordWord(bad,offset,offset==24?7u:0x10000000u);
   setRecordWord(bad,16,recordChecksum(bad));DecodedGraphicsRecord held;
   assert(!decodeGraphicsRecord(bad,held));
  }
 }
 // A schema6 record with explicit counts is preserved byte for byte too.
 auto schema6=i;assert(setFgAllocation(schema6,2,3));schema6.schema=6;
 GraphicsRecord schema6Bytes,schema6Again;assert(encodeGraphicsRecord(schema6,schema6Bytes));
 assert(encodeGraphicsRecord(decode(schema6).intent,schema6Again)&&schema6Bytes==schema6Again&&schema6Bytes[6]=='6');
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
 auto changed=legacy;changed.schema=7;changed.revision++;changed.hdr.uiNits++;
 MenuRequest request{123,1,stored.stamp,changed};MenuRequestBytes requestBytes;assert(encodeMenuRequest(request,requestBytes));
 MenuRequestWriter writer(123,store);const auto receipt=writer.consume(requestBytes,[](auto){return true;});
 assert(receipt.status==MenuCommitStatus::Committed&&receipt.stamp!=stored.stamp);
 assert(store.load(loaded).status==StoreStatus::Ok&&loaded.intent==changed);
 assert(fgAllocation(loaded.intent)==fgAllocation(legacy)&&!loaded.intent.dlssGenerated&&!loaded.intent.fsrGenerated);
 assert(store.publish(changed,stored.stamp).status==StoreStatus::Conflict);
 GraphicsRecord upgraded;assert(encodeGraphicsRecord(loaded.intent,upgraded)&&upgraded[6]=='7');
 assert(!matchesStartup(startup,upgraded,123));
 // Order-only edits use the same CAS authority. A stale checksum is refused even
 // when revision, provider, total and counts all match the newly saved record.
 auto orderA=changed;assert(setFgAllocation(orderA,2,3));orderA.revision++;orderA.frameOrder=42;
 const auto firstOrder=store.publish(orderA,receipt.stamp);assert(firstOrder.status==StoreStatus::Ok);
 auto orderB=orderA;orderB.revision++;orderB.frameOrder=37;
 const auto secondOrder=store.publish(orderB,firstOrder.stamp);assert(secondOrder.status==StoreStatus::Ok);
 assert(firstOrder.stamp.checksum!=secondOrder.stamp.checksum);
 auto staleOrder=orderB;staleOrder.revision++;
 auto staleStamp=secondOrder.stamp;staleStamp.checksum=firstOrder.stamp.checksum;
 assert(store.publish(staleOrder,staleStamp).status==StoreStatus::Conflict);
 assert(store.load(loaded).status==StoreStatus::Ok&&loaded.intent==orderB);
}
