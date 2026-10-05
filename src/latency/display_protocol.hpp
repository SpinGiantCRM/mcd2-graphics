#pragma once
#include "../native/settings_bridge.hpp"
#include <map>
#include <set>
namespace mcd2::display {
struct Intent {unsigned revision=0,hdr=0,peak=0,paper=0,ui=0,reflex=0;};
inline bool parse(const std::vector<uint8_t>&bytes,const char*className,std::map<std::string,unsigned>&values,size_t&header){
 if(bytes.size()<64||bytes.size()>8192)return false;mcd2ui::Reader r{bytes};
 if(r.u32()!=0x53415647||r.u32()!=3||r.u32()!=522||r.u32()!=1017||r.u16()!=5||r.u16()!=6||r.u16()!=1||r.u32()!=0||r.str()!="UE5"||r.u32()!=3||r.u32()!=93)return false;
 if(r.at+93*20>bytes.size())return false;r.at+=93*20;
 if(r.str()!=std::string("/Game/Mods/MCD2Graphics/")+className+"."+className+"_C"||r.u8()!=0)return false;header=r.at;
 for(unsigned n=0;n<24;++n){auto name=r.str();if(!r.ok)return false;if(name=="None")return r.u32()==0&&r.ok&&r.at==bytes.size()&&values["SchemaVersion"]==1;
  if(values.count(name))return false;auto type=r.str();auto array=r.u32(),size=r.u32();auto flags=r.u8();if(!r.ok||type!="IntProperty"||array||size!=4||flags)return false;
  auto value=r.u32();if(!r.ok||value>0x7fffffff)return false;values[name]=value;
 }return false;
}
inline bool decode(const std::vector<uint8_t>&bytes,Intent&intent){std::map<std::string,unsigned> v;size_t h=0;if(!parse(bytes,"DisplaySettingsSave",v,h))return false;
 static const std::set<std::string> allowed={"SchemaVersion","Revision","HDROutput","PeakNits","PaperWhiteNits","UINits","ReflexMode"};for(auto&kv:v)if(!allowed.count(kv.first))return false;
 Intent s{v["Revision"],v["HDROutput"],v["PeakNits"],v["PaperWhiteNits"],v["UINits"],v["ReflexMode"]};
 if(!s.revision||s.hdr>1||s.reflex>2||s.peak<100||s.peak>10000||s.peak%10||s.paper<48||s.paper>500||s.ui<48||s.ui>500)return false;intent=s;return true;
}
inline std::vector<uint8_t> encode_runtime(const std::vector<uint8_t>&seed,const std::map<std::string,unsigned>&fields){
 std::map<std::string,unsigned> old;size_t h=0;if(!parse(seed,"DisplayRuntimeSave",old,h))return {};
 std::vector<uint8_t> out(seed.begin(),seed.begin()+h);auto u32=[&](unsigned x){for(unsigned i=0;i<4;++i)out.push_back(uint8_t(x>>(8*i)));};auto str=[&](const std::string&s){u32(unsigned(s.size()+1));out.insert(out.end(),s.begin(),s.end());out.push_back(0);};
 for(auto&f:fields){if(!f.second)continue;str(f.first);str("IntProperty");u32(0);u32(4);out.push_back(0);u32(f.second);}str("None");u32(0);return out;
}
}
