#include "../src/gdk/save_slots.hpp"
#include "../src/providers/menu_save_transport.hpp"
#include "../src/providers/advanced_settings.hpp"
#include "../src/providers/sr_runtime_protocol.hpp"
#include <cassert>
#include <iostream>
using namespace mcd2::providers;
static SlotBytes seed(std::string name) {
    using namespace menu_save_detail;
    SlotBytes b;for(auto n:{0x53415647u,3u,522u,1017u})number(b,n);
    for(auto n:{5u,6u,1u})number(b,n,2);
    number(b,0);string(b,"UE5");number(b,3);number(b,93);b.resize(b.size()+93*20);
    string(b,"/Game/Mods/MCD2Graphics/"+name+"."+name+"_C");number(b,0,1);
    string(b,"None");number(b,0);return b;
}
int main() {
    using namespace mcd2::gdk;
    const auto slot=[](std::string_view name)->const Slot& {
        for(const auto& s:slots)if(s.name==name)return s;
        std::cerr<<"missing transport slot: "<<name<<'\n';std::abort();
    };
    AdvancedRequestBytes request;assert(encodeAdvancedRequest({123,1,{7,0xfedcba98},20,0},request));
    for(auto name:{"MCD2GraphicsAdvancedRequest","MCD2GraphicsAdvancedFGRequest","MCD2GraphicsAdvancedFSRRequest","MCD2GraphicsAdvancedFSRFGRequest"}) {
        const auto& s=slot(name);assert(!s.nativeWrites);
        SlotBytes bytes;assert(menu_save_detail::encode(seed("AdvancedRequestSave"),"AdvancedRequestSave",request,bytes));
        assert(valid(s,bytes));
        std::size_t at=0,end=0;unsigned count=0;assert(versionTable(bytes,at,end,count));
        auto gdk=bytes;gdk.erase(gdk.begin()+end-60,gdk.begin()+end);gdk[at]=90;
        assert(valid(s,gdk)&&storePayload(nativePayload(gdk),gdk)==gdk);
        for(std::size_t n=0;n<bytes.size();++n)assert(!valid(s,{bytes.begin(),bytes.begin()+n}));
        auto bad=request;bad[35]^=128;assert(menu_save_detail::encode(seed("AdvancedRequestSave"),"AdvancedRequestSave",bad,bytes));assert(!valid(s,bytes));
    }
    AdvancedState state;state.session=123;state.modules=768;state.versions[16]=310;state.versions[17]=9;state.versions[18]=65535;state.versions[19]=65535;
    AdvancedStateBytes words;assert(encodeAdvancedState(state,words));assert(recordWord(words,124)==0xffffffffu);
    for(auto name:{"MCD2GraphicsAdvancedState","MCD2GraphicsAdvancedFGState","MCD2GraphicsAdvancedFSRState","MCD2GraphicsAdvancedFSRFGState"}) {
        SlotBytes bytes;assert(menu_save_detail::encode(seed("AdvancedStateSave"),"AdvancedStateSave",words,bytes));
        assert(slot(name).nativeWrites&&valid(slot(name),bytes));
    }
    SrRuntimeBytes context;assert(encodeSrContext({123,1,{7,0xdeadbeef},2,true,100000,66666667,0,0,0},context));
    SlotBytes bytes;assert(menu_save_detail::encode(seed("ProviderSrContextSave"),"ProviderSrContextSave",context,bytes));
    assert(valid(slot("MCD2GraphicsProviderSrContext"),bytes));
    auto wrong=context;wrong[127]=1;assert(menu_save_detail::encode(seed("ProviderSrContextSave"),"ProviderSrContextSave",wrong,bytes));assert(!valid(slot("MCD2GraphicsProviderSrContext"),bytes));
    MenuRequestBytes requestWords;GraphicsIntent desired;desired.revision=8;
    assert(encodeMenuRequest({123,1,{7,0xfedcba98},desired},requestWords));
    assert(menu_save_detail::encode(seed("ProviderRequestSave"),"ProviderRequestSave",requestWords,bytes));
    assert(valid(slot("MCD2GraphicsProviderRequest"),bytes));
    MenuAuthority authority;authority.session=123;
    MenuAuthorityBytes authorityWords;assert(encodeMenuAuthority(authority,authorityWords));
    assert(menu_save_detail::encode(seed("ProviderAuthoritySave"),"ProviderAuthoritySave",authorityWords,bytes));
    assert(valid(slot("MCD2GraphicsProviderAuthority"),bytes));
    // Empty response seeds permit native bootstrap, but are never native publications.
    assert(valid(slot("MCD2GraphicsProviderAuthority"),seed("ProviderAuthoritySave")));
    assert(!valid(slot("MCD2GraphicsProviderAuthority"),seed("ProviderAuthoritySave"),false));
    SrRuntimeState runtime{123,1,{7,0xdeadbeef},2,SrPhase::Native};
    assert(encodeSrRuntimeState(runtime,context));
    assert(menu_save_detail::encode(seed("ProviderSrRuntimeSave"),"ProviderSrRuntimeSave",context,bytes));
    assert(valid(slot("MCD2GraphicsProviderSrRuntime"),bytes));
    const auto namedSeed=[](const char* cls){auto bytes=seed(cls);return mcd2::display::encode_runtime(bytes,{{"SchemaVersion",1}},cls);};
    // Named slots require schema 1 even for their empty serializer seeds.
    const auto named=[](const char* cls,const std::map<std::string,unsigned>& fields){
        auto bytes=seed(cls);auto prefix=bytes.size()-13;bytes.resize(prefix);
        using namespace menu_save_detail;
        for(const auto& [key,value]:fields){string(bytes,key);string(bytes,"IntProperty");number(bytes,0);number(bytes,4);number(bytes,0,1);number(bytes,value);}
        string(bytes,"None");number(bytes,0);return bytes;
    };
    (void)namedSeed;
    auto display=named("DisplayRuntimeSave",{{"SchemaVersion",1},{"SessionId",123},{"AmdAntiLagAvailable",1},{"AmdAntiLagMode",1},{"AmdAntiLagFgCompatible",1},{"RenderingVendorId",4098}});
    assert(valid(slot("MCD2GraphicsDisplayRuntime"),display));
    auto invalidDisplay=named("DisplayRuntimeSave",{{"SchemaVersion",1},{"AmdAntiLagMode",2}});assert(!valid(slot("MCD2GraphicsDisplayRuntime"),invalidDisplay));
    auto fg=named("FGRuntimeSave",{{"SchemaVersion",1},{"SessionId",123},{"PresenterRoute",1},{"PresenterCanSwitch",1},{"NativeGuidesRetired",1},{"MaximumGeneratedFrames",5},{"ActiveFrameMode",4}});
    assert(valid(slot("MCD2GraphicsFGRuntime"),fg));
    auto diagnostics=named("DiagnosticsRequestSave",{{"SchemaVersion",1},{"SessionId",123},{"Enabled",1}});assert(valid(slot("MCD2GraphicsDiagnosticsRequest"),diagnostics));
    auto diagnosticState=named("DiagnosticsStateSave",{{"SchemaVersion",1},{"SessionId",123},{"Sample",1},{"SrProvider",2},{"FgProvider",2},{"AmdMode",1}});assert(valid(slot("MCD2GraphicsDiagnosticsState"),diagnosticState));
    auto wrongDiagnostics=named("DiagnosticsStateSave",{{"SchemaVersion",1},{"AmdMode",2}});assert(!valid(slot("MCD2GraphicsDiagnosticsState"),wrongDiagnostics));
    assert(!valid({"PlayerSave","AdvancedRequestSave",false},seed("AdvancedRequestSave")));
    assert(!valid({"MCD2GraphicsAdvancedRequest","PlayerSave",false},seed("PlayerSave")));
    assert(slots.size()==20);
    std::cout<<"GDK provider transport: signed words, typed checksums, exact slots, truncation and GDK header roundtrip passed\n";
}
