#pragma once
#include "../../src/latency/display_protocol.hpp"
namespace mcd2::fgui {
struct Intent {unsigned revision=0,mode=0,ready=0,session=0,provider=0,settingsRevision=0;};
inline bool decode(const std::vector<uint8_t>&bytes,Intent&out){
 std::map<std::string,unsigned> v;size_t h=0;if(!display::parse(bytes,"FGSettingsSave",v,h))return false;
 // Unreal FName preserves the first interned spelling, with case-insensitive
 // identity. Both the current FGProvider and legacy Provider name can vary.
 // Reject duplicates, including differently cased and current/legacy aliases.
 unsigned provider=0;bool haveProvider=false;
 const std::set<std::string> allowed={"SchemaVersion","Revision","Mode","ContextReady","SessionId","SettingsRevision"};
 for(const auto&kv:v){auto name=kv.first;for(char&c:name)if(c>='A'&&c<='Z')c=char(c-'A'+'a');
  if(name=="provider"||name=="fgprovider"){if(haveProvider)return false;haveProvider=true;provider=kv.second;}
  else if(!allowed.count(kv.first))return false;
 }
 Intent next{v["Revision"],v["Mode"],v["ContextReady"],v["SessionId"],provider,v["SettingsRevision"]};if(v["SchemaVersion"]!=1||!next.revision||next.mode>1||next.ready>1||next.provider>1)return false;out=next;return true;
}
inline std::vector<uint8_t> encode(const std::vector<uint8_t>&seed,const std::map<std::string,unsigned>&fields){
 std::map<std::string,unsigned> old;size_t h=0;if(!display::parse(seed,"FGRuntimeSave",old,h))return {};
 const std::set<std::string> allowed={"SchemaVersion","Revision","SessionId","Available","Active","Phase","RestartRequired","AvailableProviders","StartupOwner"};for(auto&kv:fields)if(!allowed.count(kv.first)||kv.second>0x7fffffff)return {};
 std::vector<uint8_t> out(seed.begin(),seed.begin()+h);auto u32=[&](unsigned x){for(unsigned i=0;i<4;++i)out.push_back(uint8_t(x>>(8*i)));};auto str=[&](const std::string&s){u32(unsigned(s.size()+1));out.insert(out.end(),s.begin(),s.end());out.push_back(0);};
 for(auto&f:fields){if(!f.second)continue;str(f.first);str("IntProperty");u32(0);u32(4);out.push_back(0);u32(f.second);}str("None");u32(0);return out;
}
}
