#pragma once
#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace mcd2::hdr_activation {
inline constexpr const char* token="RenoDX@renodx-ue-extended.addon64";
inline constexpr const char* receiptName="HdrAddonActivation.bin";
struct Key {bool present=false;std::vector<std::string> entries;bool operator==(const Key&)const=default;};
enum class Phase:std::uint32_t {Prepared=1,Owned=2,Restoring=3,Yielded=4};
struct Receipt {Phase phase=Phase::Prepared;Key original,last;bool operator==(const Receipt&)const=default;};
inline bool utf8(const std::string&s){
 for(std::size_t i=0;i<s.size();){auto c=static_cast<unsigned char>(s[i++]);if(c<128)continue;unsigned n=c>=194&&c<=223?1:c>=224&&c<=239?2:c>=240&&c<=244?3:0;if(!n||i+n>s.size())return false;
  auto first=static_cast<unsigned char>(s[i]);if((c==224&&first<160)||(c==237&&first>=160)||(c==240&&first<144)||(c==244&&first>=144))return false;
  while(n--){auto next=static_cast<unsigned char>(s[i++]);if(next<128||next>191)return false;}}
 return true;
}
inline bool valid(const Key& k){
 if(!k.present&&!k.entries.empty())return false;if(k.entries.size()>128)return false;
 std::size_t bytes=0;for(const auto&s:k.entries){if(s.size()>1024||!utf8(s))return false;bytes+=s.size();for(unsigned char c:s)if(c<32||c==127)return false;}
 return bytes<=12000;
}
inline bool disabled(const Key&k){for(const auto&s:k.entries){auto at=s.find('@');if(at==std::string::npos?s=="RenoDX":s.substr(at+1)=="renodx-ue-extended.addon64")return true;}return false;}
inline bool valid(const Receipt&r){auto expected=r.original;expected.present=true;expected.entries.push_back(token);return unsigned(r.phase)>=1&&unsigned(r.phase)<=4&&valid(r.original)&&valid(r.last)&&!disabled(r.original)&&r.last==expected;}
inline std::uint32_t checksum(const std::vector<std::uint8_t>& b,std::size_t n){std::uint32_t h=2166136261u;for(std::size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
inline std::vector<std::uint8_t> encode(const Receipt&r){
 if(!valid(r))return {};std::vector<std::uint8_t>b{'M','C','D','2','H','D','R','1'};
 auto u32=[&](std::uint32_t v){for(unsigned i=0;i<4;++i)b.push_back(std::uint8_t(v>>(8*i)));};u32(unsigned(r.phase));
 for(const auto*k:{&r.original,&r.last}){u32(k->present);u32(std::uint32_t(k->entries.size()));for(const auto&s:k->entries){u32(std::uint32_t(s.size()));b.insert(b.end(),s.begin(),s.end());}}
 u32(checksum(b,b.size()));return b;
}
inline bool decode(const std::vector<std::uint8_t>&b,Receipt&r){
 if(b.size()<32||b.size()>32768||std::string(b.begin(),b.begin()+8)!="MCD2HDR1")return false;std::size_t at=8;
 auto u32=[&](std::uint32_t&v){if(at+4>b.size())return false;v=0;for(unsigned i=0;i<4;++i)v|=std::uint32_t(b[at++])<<(8*i);return true;};
 Receipt next;std::uint32_t phase=0;if(!u32(phase))return false;next.phase=Phase(phase);
 for(auto*k:{&next.original,&next.last}){std::uint32_t present=0,count=0;if(!u32(present)||present>1||!u32(count)||count>128)return false;k->present=present!=0;for(unsigned i=0;i<count;++i){std::uint32_t n=0;if(!u32(n)||n>1024||at+n>b.size())return false;k->entries.emplace_back(b.begin()+at,b.begin()+at+n);at+=n;}}
 const auto end=at;std::uint32_t hash=0;if(!u32(hash)||at!=b.size()||hash!=checksum(b,end)||!valid(next))return false;r=std::move(next);return true;
}
struct Result {bool ready=false,conflict=false,pendingRestart=false;};
// Backend operations must reread the current key before writing, verify the
// public config API readback, and persist the write-ahead receipt first.
class Activation {
 bool changed_=false;
public:
 template<class Backend>Result apply(bool hdr,bool resident,Backend&b){
  Key current;std::optional<Receipt>r;if(!b.read(current)||!valid(current)||!b.load(r))return {false,true};
  auto conflict=[&](){if(r){r->phase=Phase::Yielded;b.save(r);}return Result{false,true};};
  if(r){
   if(!valid(*r)||r->phase==Phase::Yielded)return {false,true};
   if(r->phase==Phase::Restoring){
    if(current==r->last){if(!b.write(current,r->original))return conflict();changed_=true;current=r->original;}
    else if(!(current==r->original))return conflict();
    if(!b.save(std::nullopt))return {false,true};r.reset();
   }else{
    if(r->phase==Phase::Prepared&&current==r->original){if(!b.write(current,r->last))return conflict();changed_=true;current=r->last;}
    if(!(current==r->last))return conflict();
    if(r->phase==Phase::Prepared){r->phase=Phase::Owned;if(!b.save(r))return {false,true};}
   }
  }
  if(!hdr){
   if(!disabled(current)){
    Receipt next;next.original=current;next.last=current;next.last.present=true;next.last.entries.push_back(token);
    r=next;if(!valid(next)||!b.save(r))return {false,true};
    if(!b.write(current,next.last))return conflict();changed_=true;r->phase=Phase::Owned;if(!b.save(r))return {false,true};
   }
   return {!resident,false,changed_};
  }
  if(r){r->phase=Phase::Restoring;if(!b.save(r))return {false,true};if(!b.write(current,r->original))return conflict();changed_=true;current=r->original;if(!b.save(std::nullopt))return {false,true};}
  if(disabled(current))return {false,true}; // A preexisting user disable is never removed.
  return {resident&&!changed_,false,changed_};
 }
};
} // namespace mcd2::hdr_activation
