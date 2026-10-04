#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
namespace mcd2ui {
struct Reader { const std::vector<uint8_t>& b; size_t at=0; bool ok=true;
 uint32_t u32(){ if(at+4>b.size()){ok=false;return 0;}uint32_t v=0;std::memcpy(&v,b.data()+at,4);at+=4;return v; }
 uint16_t u16(){if(at+2>b.size()){ok=false;return 0;}uint16_t v=0;std::memcpy(&v,b.data()+at,2);at+=2;return v;}
 uint8_t u8(){if(at>=b.size()){ok=false;return 0;}return b[at++];}
 std::string str(){uint32_t n=u32();if(!ok||n<1||n>256||at+n>b.size()||b[at+n-1]!=0){ok=false;return {};}
  std::string s(reinterpret_cast<const char*>(b.data()+at),n-1);if(s.find('\0')!=std::string::npos){ok=false;return {}; }at+=n;return s;}
};
struct Settings { uint32_t schema=0, revision=0, mode=0, scale=0, sourceRevision=0, sourceSession=0, preset=0, custom=6700, contextReady=0, contextSession=0; bool fallback=false; };
inline bool decode(const std::vector<uint8_t>& bytes, Settings &out){
 if(bytes.size()<64||bytes.size()>8192)return false;
 Reader r{bytes};if(r.u32()!=0x53415647||r.u32()!=3||r.u32()!=522||r.u32()!=1017)return false;
 if(r.u16()!=5||r.u16()!=6||r.u16()!=1||r.u32()!=0||r.str()!="UE5"||r.u32()!=3)return false;
 auto n=r.u32();if(n!=93||r.at+size_t(n)*20>bytes.size())return false;r.at+=size_t(n)*20;
 if(r.str()!="/Game/Mods/MCD2Graphics/GraphicsSettingsSave.GraphicsSettingsSave_C"||r.u8()!=0)return false;
 Settings s; std::vector<std::string> seen;
 for(unsigned count=0; count<32; ++count){
  auto name=r.str();if(!r.ok)return false;
  if(name=="None"){
   if(r.u32()!=0||!r.ok||r.at!=bytes.size())return false;
   if((s.schema!=1 && s.schema!=2 && s.schema!=3 && s.schema!=4)||s.revision<1||s.revision>0x7fffffff||s.mode>2||s.sourceRevision>s.revision||s.contextReady>1)return false;
   if(s.preset>(s.schema==4?5u:4u) || s.custom%100)return false;
   if(s.schema>=3 ? (s.custom<100 || s.custom>(s.schema==4?10000u:9900u)) : (s.custom<5000 || s.custom>10000))return false;
   if(s.schema==4){
    if((s.mode==1 && s.preset!=5)||(s.mode==2 && s.preset==5))return false;
    const uint32_t display=s.preset==5?10000u:(s.preset==0?6700u:(s.preset==1?5800u:(s.preset==2?5000u:(s.preset==3?3300u:s.custom))));
    if(s.custom!=display || (s.preset==4 && (s.custom==10000 || s.custom==6700 || s.custom==5800 || s.custom==5000 || s.custom==3300)))return false;
   }
   if(s.schema==1 && (s.preset || s.custom!=6700))return false;
   const uint32_t expected=s.mode!=2?10000u:(s.preset==0?6667u:(s.preset==1?5800u:(s.preset==2?5000u:(s.preset==3?3333u:s.custom))));
   if(s.scale!=expected)return false;
   out=s;return true;
  }
  for(const auto &prior:seen){if(prior==name)return false;}
  seen.push_back(name);
  auto type=r.str();auto array=r.u32();auto size=r.u32();auto flags=r.u8();
  if(!r.ok||array!=0)return false;
  if(type=="IntProperty" && size==4 && flags==0){
   auto value=r.u32();if(!r.ok||value>0x7fffffff)return false;
   if(name=="SchemaVersion")s.schema=value;else if(name=="Revision")s.revision=value;
   else if(name=="ReconstructionMode")s.mode=value;else if(name=="RenderScaleBasisPoints")s.scale=value;
   else if(name=="SRPreset")s.preset=value;else if(name=="CustomScaleBasisPoints")s.custom=value;
   else if(name=="AppliedSourceRevision")s.sourceRevision=value;else if(name=="AppliedSourceSessionId")s.sourceSession=value;
   else if(name=="RenderContextReady")s.contextReady=value;else if(name=="RenderContextSessionId")s.contextSession=value;
   // Future scalar fields can be skipped after their framing is checked.
   else if(name=="NativeFallback")return false;
  }else if(type=="BoolProperty" && size==0 && (flags==0 || flags==0x10)){
   if(name=="NativeFallback")s.fallback=flags==0x10;
   else if(name=="SchemaVersion"||name=="Revision"||name=="ReconstructionMode"||name=="RenderScaleBasisPoints"||name=="SRPreset"||name=="CustomScaleBasisPoints"||name=="AppliedSourceRevision"||name=="AppliedSourceSessionId"||name=="RenderContextReady"||name=="RenderContextSessionId")return false;
  }else return false;
 }
 return false;
}
inline bool source_ratio(const Settings &s,unsigned width,unsigned height,unsigned outwidth,unsigned outheight){
 if(!width || !height || !outwidth || !outheight)return false;
 if(s.mode!=2)return width==outwidth && height==outheight;
 double scale=double(s.scale)/10000.;if(s.preset==0)scale=2./3.;if(s.preset==3)scale=1./3.;
 return std::abs(double(width)-double(outwidth)*scale)<=8 && std::abs(double(height)-double(outheight)*scale)<=8;
}
struct RuntimeState { uint32_t schema=0, revision=0, session=0, phase=0, error=0; size_t headerBytes=0; };
inline bool decode_runtime(const std::vector<uint8_t>& bytes, RuntimeState &out){
 if(bytes.size()<64||bytes.size()>8192)return false;
 Reader r{bytes};if(r.u32()!=0x53415647||r.u32()!=3||r.u32()!=522||r.u32()!=1017)return false;
 if(r.u16()!=5||r.u16()!=6||r.u16()!=1||r.u32()!=0||r.str()!="UE5"||r.u32()!=3||r.u32()!=93)return false;
 if(r.at+93*20>bytes.size())return false;r.at+=93*20;
 if(r.str()!="/Game/Mods/MCD2Graphics/GraphicsRuntimeStateSave.GraphicsRuntimeStateSave_C"||r.u8()!=0)return false;
 RuntimeState s;s.headerBytes=r.at;std::vector<std::string> seen;
 for(unsigned count=0;count<32;++count){
  auto name=r.str();if(!r.ok)return false;
  if(name=="None"){
   if(r.u32()!=0||!r.ok||r.at!=bytes.size()||s.schema!=1||s.phase>5||s.error>2)return false;
   if(s.phase && (!s.revision || !s.session))return false;
   out=s;return true;
  }
  for(const auto &prior:seen)if(prior==name)return false;seen.push_back(name);
  auto type=r.str();auto array=r.u32();auto size=r.u32();auto flags=r.u8();
  if(!r.ok||array!=0||type!="IntProperty"||size!=4||flags!=0)return false;
  auto value=r.u32();if(!r.ok||value>0x7fffffff)return false;
  if(name=="SchemaVersion")s.schema=value;else if(name=="RequestedRevision")s.revision=value;
  else if(name=="SessionId")s.session=value;else if(name=="Phase")s.phase=value;else if(name=="ErrorCode")s.error=value;
 }
 return false;
}
// Last valid intent wins; malformed/older input never configures the GPU.
struct Mailbox {
 Settings latest; bool pending=false;
 bool offer(const Settings &s){if(s.revision<=latest.revision)return false;latest=s;pending=true;return true;}
 bool take(Settings &s){if(!pending)return false;s=latest;pending=false;return true;}
};
}
