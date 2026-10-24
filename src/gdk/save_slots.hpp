#pragma once
#include "../native/settings_bridge.hpp"
#include "../latency/display_protocol.hpp"
#include "../../experiments/fg-streamline/fg_ui_protocol.hpp"
#include "../providers/menu_save_transport.hpp"
#include "../providers/sr_runtime_protocol.hpp"
#include "../providers/advanced_settings.hpp"
#include <array>
#include <string_view>

namespace mcd2::gdk {
struct Slot { const char* name; const char* saveClass; bool nativeWrites; };
inline constexpr std::array<Slot,20> slots{{
    {"MCD2GraphicsSettings", "GraphicsSettingsSave", false},
    {"MCD2GraphicsDisplaySettings", "DisplaySettingsSave", false},
    {"MCD2GraphicsFGSettings", "FGSettingsSave", false},
    {"MCD2GraphicsRuntime", "GraphicsRuntimeStateSave", true},
    {"MCD2GraphicsDisplayRuntime", "DisplayRuntimeSave", true},
    {"MCD2GraphicsFGRuntime", "FGRuntimeSave", true},
    {"MCD2GraphicsProviderRequest", "ProviderRequestSave", false},
    {"MCD2GraphicsProviderAuthority", "ProviderAuthoritySave", true},
    {"MCD2GraphicsProviderSrContext", "ProviderSrContextSave", false},
    {"MCD2GraphicsProviderSrRuntime", "ProviderSrRuntimeSave", true},
    {"MCD2GraphicsAdvancedRequest", "AdvancedRequestSave", false},
    {"MCD2GraphicsAdvancedState", "AdvancedStateSave", true},
    {"MCD2GraphicsAdvancedFGRequest", "AdvancedRequestSave", false},
    {"MCD2GraphicsAdvancedFGState", "AdvancedStateSave", true},
    {"MCD2GraphicsAdvancedFSRRequest", "AdvancedRequestSave", false},
    {"MCD2GraphicsAdvancedFSRState", "AdvancedStateSave", true},
    {"MCD2GraphicsAdvancedFSRFGRequest", "AdvancedRequestSave", false},
    {"MCD2GraphicsAdvancedFSRFGState", "AdvancedStateSave", true},
    {"MCD2GraphicsDiagnosticsRequest", "DiagnosticsRequestSave", false},
    {"MCD2GraphicsDiagnosticsState", "DiagnosticsStateSave", true},
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
template<std::size_t N,class Decode>
inline bool wordPayload(const Slot& slot,const std::vector<std::uint8_t>& bytes,bool allowSeed,Decode decode){
    std::array<std::uint8_t,N> words{};
    if(!providers::menu_save_detail::parse(bytes,slot.saveClass,words))return false;
    // UE creates empty serializer seeds before native can publish its first response.
    // Seeds cannot authorize rendering and are never published back as active state.
    if(allowSeed&&slot.nativeWrites&&std::all_of(words.begin(),words.end(),[](auto v){return v==0;}))return true;
    return decode(words);
}
inline bool valid(const Slot& slot, const std::vector<std::uint8_t>& bytes,bool allowSeed=true) {
    const bool known=std::any_of(slots.begin(),slots.end(),[&](const auto& s){return std::string_view(s.name)==slot.name && std::string_view(s.saveClass)==slot.saveClass && s.nativeWrites==slot.nativeWrites;});
    if(!known)return false;
    const auto canonical=nativePayload(bytes);if(canonical.empty())return false;
    const auto name=std::string_view(slot.name);
    using namespace providers;
    const auto className=std::string_view(slot.saveClass);
    if(className=="ProviderRequestSave")return wordPayload<168>(slot,canonical,allowSeed,[](const auto& b){MenuRequest r;return decodeMenuRequest(b,r);});
    if(className=="ProviderAuthoritySave")return wordPayload<176>(slot,canonical,allowSeed,[](const auto& b){MenuAuthority r;return decodeMenuAuthority(b,r);});
    if(className=="ProviderSrContextSave")return wordPayload<128>(slot,canonical,allowSeed,[](const auto& b){SrContext r;return decodeSrContext(b,r);});
    if(className=="ProviderSrRuntimeSave")return wordPayload<128>(slot,canonical,allowSeed,[](const auto& b){SrRuntimeState r;return decodeSrRuntimeState(b,r);});
    if(className=="AdvancedRequestSave")return wordPayload<64>(slot,canonical,allowSeed,[](const auto& b){AdvancedRequest r;return decodeAdvancedRequest(b,r);});
    if(className=="AdvancedStateSave")return wordPayload<128>(slot,canonical,allowSeed,[](const auto& b){AdvancedState r;return decodeAdvancedState(b,r);});
    if(name=="MCD2GraphicsSettings") { mcd2ui::Settings value; return mcd2ui::decode(canonical,value); }
    if(name=="MCD2GraphicsDisplaySettings") { display::Intent value; return display::decode(canonical,value); }
    if(name=="MCD2GraphicsFGSettings") { fgui::Intent value; return fgui::decode(canonical,value); }
    if(name=="MCD2GraphicsRuntime") { mcd2ui::RuntimeState value; return mcd2ui::decode_runtime(canonical,value); }
    if(name!="MCD2GraphicsDisplayRuntime" && name!="MCD2GraphicsFGRuntime" && name!="MCD2GraphicsDiagnosticsRequest" && name!="MCD2GraphicsDiagnosticsState")return false;
    std::map<std::string,unsigned> fields;std::size_t header=0;
    if(!display::parse(canonical,slot.saveClass,fields,header))return false;
    if(name=="MCD2GraphicsDiagnosticsRequest"){
        for(const auto& [key,value]:fields)if(key!="SchemaVersion"&&key!="SessionId"&&key!="Enabled")return false;
        return fields["Enabled"]<=1;
    }
    if(name=="MCD2GraphicsDiagnosticsState"){
        const std::set<std::string> allowed={"SchemaVersion","SessionId","Revision","Sample","WorldGeneration","SrProvider","SrFrames","FsrSrVersion","FsrHardware","FgProvider","FgActive","FgFault","FgRendered","FgGenerated","FsrFgVersion","FgPresetStatus","ReflexMode","ReflexFrames","AmdMode","AmdFrames"};
        for(const auto& [key,value]:fields)if(!allowed.contains(key))return false;
        return fields["SrProvider"]<=3&&fields["FgProvider"]<=2&&fields["FgActive"]<=1&&fields["ReflexMode"]<=2&&fields["AmdMode"]<=1;
    }
    const std::set<std::string> allowed=name=="MCD2GraphicsDisplayRuntime"?
        std::set<std::string>{"SchemaVersion","Revision","SessionId","ReflexAvailable","ReflexMode","ReflexFault","HDRRevision","HDRRestartRequired","RenderingVendorId","AmdAntiLagAvailable","AmdAntiLagMode","AmdAntiLagFault","AmdAntiLagRevision","AmdAntiLagFgCompatible"}:
        std::set<std::string>{"SchemaVersion","Revision","SessionId","Available","Active","Phase","RestartRequired","AvailableProviders","StartupOwner","PresenterRoute","PresenterCanSwitch","PresenterSettingsRevision","NativeGuidesRetired","MaximumGeneratedFrames","DynamicSupported","ActiveFrameMode"};
    for(const auto& [key,value]:fields)if(!allowed.contains(key))return false;
    if(name=="MCD2GraphicsDisplayRuntime")return fields["ReflexAvailable"]<=1 && fields["ReflexMode"]<=2 && fields["ReflexFault"]<=1 && fields["HDRRestartRequired"]<=1 && fields["RenderingVendorId"]<=65535 && fields["AmdAntiLagAvailable"]<=1 && fields["AmdAntiLagMode"]<=1 && fields["AmdAntiLagFault"]<=1 && fields["AmdAntiLagFgCompatible"]<=1;
    return fields["Available"]<=1 && fields["Active"]<=1 && fields["Phase"]<=6 && fields["RestartRequired"]<=1 && fields["AvailableProviders"]<=3 && fields["StartupOwner"]<=2 && fields["PresenterRoute"]<=2 && fields["PresenterCanSwitch"]<=1 && fields["NativeGuidesRetired"]<=1 && fields["MaximumGeneratedFrames"]<=5 && fields["ActiveFrameMode"]<=4 && fields["DynamicSupported"]<=1;
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
