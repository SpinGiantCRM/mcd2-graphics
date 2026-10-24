#pragma once
#include "../native/settings_bridge.hpp"
#include "../latency/display_protocol.hpp"
#include "../../experiments/fg-streamline/fg_ui_protocol.hpp"
#include <array>
#include <string_view>

namespace mcd2::gdk {
struct Slot { const char* name; const char* saveClass; bool nativeWrites; };
inline constexpr std::array<Slot,6> slots{{
    {"MCD2GraphicsSettings", "GraphicsSettingsSave", false},
    {"MCD2GraphicsDisplaySettings", "DisplaySettingsSave", false},
    {"MCD2GraphicsFGSettings", "FGSettingsSave", false},
    {"MCD2GraphicsRuntime", "GraphicsRuntimeStateSave", true},
    {"MCD2GraphicsDisplayRuntime", "DisplayRuntimeSave", true},
    {"MCD2GraphicsFGRuntime", "FGRuntimeSave", true},
}};
inline bool versionTable(const std::vector<std::uint8_t>& bytes,std::size_t& countAt,std::size_t& end,unsigned& count){
    mcd2ui::Reader reader{bytes};
    if(reader.u32()!=0x53415647||reader.u32()!=3||reader.u32()!=522||reader.u32()!=1017||
       reader.u16()!=5||reader.u16()!=6||reader.u16()!=1||reader.u32()!=0||reader.str()!="UE5"||reader.u32()!=3)return false;
    countAt=reader.at;count=reader.u32();
    if(!reader.ok||(count!=90&&count!=93)||reader.at+count*20>bytes.size())return false;
    end=reader.at+count*20;return true;
}
inline std::vector<std::uint8_t> nativePayload(const std::vector<std::uint8_t>& bytes){
    if(bytes.size()<64||bytes.size()>8192)return {};
    std::size_t at=0,end=0;unsigned count=0;if(!versionTable(bytes,at,end,count))return {};
    if(count==93)return bytes;
    if(bytes.size()+60>8192)return {};
    // Released codecs skip exactly 93 entries. Padding is cache-only, never sent to Unreal.
    auto next=bytes;next.insert(next.begin()+end,60,0);next[at]=93;return next;
}
inline std::vector<std::uint8_t> storePayload(const std::vector<std::uint8_t>& native,const std::vector<std::uint8_t>& seed){
    std::size_t nativeAt=0,nativeEnd=0,seedAt=0,seedEnd=0;unsigned nativeCount=0,seedCount=0;
    if(!versionTable(native,nativeAt,nativeEnd,nativeCount)||nativeCount!=93||!versionTable(seed,seedAt,seedEnd,seedCount))return {};
    const auto expected=nativePayload(seed);
    if(expected.size()<nativeEnd||!std::equal(expected.begin(),expected.begin()+nativeEnd,native.begin()))return {};
    std::vector<std::uint8_t> next(seed.begin(),seed.begin()+seedEnd);
    next.insert(next.end(),native.begin()+nativeEnd,native.end());return next;
}
inline bool valid(const Slot& slot, const std::vector<std::uint8_t>& bytes) {
    const auto canonical=nativePayload(bytes);if(canonical.empty())return false;
    const auto name=std::string_view(slot.name);
    if(name=="MCD2GraphicsSettings") { mcd2ui::Settings value; return mcd2ui::decode(canonical,value); }
    if(name=="MCD2GraphicsDisplaySettings") { display::Intent value; return display::decode(canonical,value); }
    if(name=="MCD2GraphicsFGSettings") { fgui::Intent value; return fgui::decode(canonical,value); }
    if(name=="MCD2GraphicsRuntime") { mcd2ui::RuntimeState value; return mcd2ui::decode_runtime(canonical,value); }
    if(name!="MCD2GraphicsDisplayRuntime" && name!="MCD2GraphicsFGRuntime")return false;
    std::map<std::string,unsigned> fields;std::size_t header=0;
    if(!display::parse(canonical,slot.saveClass,fields,header))return false;
    const std::set<std::string> allowed=name=="MCD2GraphicsDisplayRuntime"?
        std::set<std::string>{"SchemaVersion","Revision","SessionId","ReflexAvailable","ReflexMode","ReflexFault","HDRRevision","HDRRestartRequired"}:
        std::set<std::string>{"SchemaVersion","Revision","SessionId","Available","Active","Phase","RestartRequired","AvailableProviders","StartupOwner"};
    for(const auto& [key,value]:fields)if(!allowed.contains(key))return false;
    if(name=="MCD2GraphicsDisplayRuntime")return fields["ReflexAvailable"]<=1 && fields["ReflexMode"]<=2 && fields["ReflexFault"]<=1 && fields["HDRRestartRequired"]<=1;
    return fields["Available"]<=1 && fields["Active"]<=1 && fields["Phase"]<=6 && fields["RestartRequired"]<=1 && fields["AvailableProviders"]<=3 && fields["StartupOwner"]<=2;
}
struct Identity { std::uint64_t user=0; std::string scid; };
inline bool identity(std::string_view folder, Identity& out) {
    if(folder.size()!=49 || folder[16]!='_')return false;
    auto hex=[](char c)->int { if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;if(c>='a'&&c<='f')return c-'a'+10;return -1; };
    Identity next;
    for(std::size_t at=0;at<folder.size();++at){if(at==16)continue;const auto digit=hex(folder[at]);if(digit<0)return false;if(at<16)next.user=(next.user<<4)|unsigned(digit);}
    if(!next.user)return false;
    const auto id=folder.substr(17);
    next.scid=std::string(id.substr(0,8))+"-"+std::string(id.substr(8,4))+"-"+std::string(id.substr(12,4))+"-"+std::string(id.substr(16,4))+"-"+std::string(id.substr(20));
    out=std::move(next);return true;
}
}
