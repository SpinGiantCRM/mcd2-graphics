#include "../src/providers/hdr_addon_activation.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace mcd2::hdr_activation;
struct Backend {
 Key key;std::optional<Receipt> receipt;unsigned writes=0,saves=0;bool failSave=false,failWrite=false;
 bool read(Key&k){k=key;return true;}bool load(std::optional<Receipt>&r){r=receipt;return true;}
 bool save(std::optional<Receipt>r){if(failSave)return false;receipt=r;++saves;return true;}
 bool write(const Key&expected,const Key&next){if(failWrite||key!=expected)return false;key=next;++writes;return true;}
};
int main(int argc,char**argv){
 Backend b;Activation a;auto r=a.apply(false,true,b);assert(!r.ready&&!r.conflict&&r.pendingRestart&&b.writes==1&&b.receipt&&b.receipt->phase==Phase::Owned);
 const auto receipt=*b.receipt;auto bytes=encode(receipt);Receipt decoded;assert(decode(bytes,decoded)&&decoded==receipt);
 if(argc==3&&std::string(argv[1])=="--receipt"){std::ofstream f(argv[2],std::ios::binary);f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());assert(f.good());}else assert(argc==1);
 for(unsigned i=0;i<bytes.size();++i){auto bad=bytes;bad[i]^=1;assert(!decode(bad,decoded));}auto extra=bytes;extra.push_back(0);assert(!decode(extra,decoded));
 for(const auto&s:{std::string("\xff"),std::string("\xc0\x80"),std::string("\xed\xa0\x80"),std::string("\xf4\x90\x80\x80")})assert(!valid(Key{true,{s}}));assert(valid(Key{true,{"\xce\xa9@addon64"}}));
 r=a.apply(false,true,b);assert(!r.ready&&b.writes==1);Activation fresh;r=fresh.apply(false,false,b);assert(r.ready&&!r.conflict&&b.writes==1);
 r=fresh.apply(true,false,b);assert(!r.ready&&r.pendingRestart&&!b.key.present&&!b.receipt&&b.writes==2);r=fresh.apply(true,true,b);assert(!r.ready&&r.pendingRestart);Activation next;r=next.apply(true,true,b);assert(r.ready);
 Backend unrelated;unrelated.key={true,{"Other@other.addon64","Name,with,commas@addon64"}};auto original=unrelated.key;Activation other;other.apply(false,true,unrelated);other.apply(true,true,unrelated);assert(unrelated.key==original&&!unrelated.receipt);
 Backend manual;manual.key={true,{token,"Other"}};Activation user;r=user.apply(true,false,manual);assert(r.conflict&&manual.writes==0&&!manual.receipt);r=user.apply(false,false,manual);assert(r.ready&&manual.writes==0);
 for(auto s:{"RenoDX","@renodx-ue-extended.addon64","Different@renodx-ue-extended.addon64"}){manual.key={true,{s}};r=user.apply(true,false,manual);assert(r.conflict&&manual.writes==0);}
 Backend external;Activation e;e.apply(false,true,external);external.key.entries.push_back("UserEdit");auto edited=external.key;r=e.apply(true,true,external);assert(r.conflict&&external.key==edited&&external.receipt->phase==Phase::Yielded);external.key=receipt.last;r=e.apply(true,true,external);assert(r.conflict&&external.writes==1);
 Backend pending;pending.receipt=receipt;pending.receipt->phase=Phase::Prepared;Activation recover;r=recover.apply(false,false,pending);assert(r.ready&&pending.key==receipt.last&&pending.receipt->phase==Phase::Owned);
 pending.receipt->phase=Phase::Restoring;Activation restore;r=restore.apply(true,true,pending);assert(!r.ready&&r.pendingRestart&&!pending.key.present&&!pending.receipt);
 pending.receipt=receipt;pending.receipt->phase=Phase::Restoring;Activation already;r=already.apply(true,true,pending);assert(r.ready&&!pending.receipt&&pending.writes==2);
 Backend failed;failed.failSave=true;Activation f;r=f.apply(false,true,failed);assert(r.conflict&&!failed.key.present&&failed.writes==0);
 failed.failSave=false;failed.failWrite=true;r=f.apply(false,true,failed);assert(r.conflict&&!failed.key.present&&failed.receipt->phase==Phase::Yielded);
 std::cout<<"HDR activation: ownership, restart, external edits, WAL recovery, byte validation and restoration passed\n";
}
