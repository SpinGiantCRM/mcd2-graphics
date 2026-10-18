#pragma once
#include "../providers/advanced_settings.hpp"
#include "../providers/menu_save_transport.hpp"
#include <winver.h>

namespace mcd2::advanced {
namespace p=mcd2::providers;
struct IniHint {
    std::filesystem::path path;
    bool read(std::uint32_t& value)const{
        wchar_t text[64]{};
        const auto n=GetPrivateProfileStringW(L"DLSS",L"ModelPreset",L"0",text,64,path.c_str());
        if(!n||n>=63)return false;
        std::uint32_t next=0;
        for(unsigned i=0;i<n;++i){if(text[i]<L'0'||text[i]>L'9')return false;next=next*10+unsigned(text[i]-L'0');if(next>255)return false;}
        value=next;return true;
    }
    bool write(std::uint32_t value)const{
        if(value>255)return false;
        // Update one key through the same profile API used by the renderer.
        // Unrelated sections/overrides are retained. Never called on a frame callback.
        const auto text=std::to_wstring(value);
        if(!WritePrivateProfileStringW(L"DLSS",L"ModelPreset",text.c_str(),path.c_str()))return false;
        // The profile API returns zero for a cache flush, including success.
        // Success requires the actual key write and independent readback.
        WritePrivateProfileStringW(nullptr,nullptr,nullptr,path.c_str());
        std::uint32_t readback=0;return read(readback)&&readback==value;
    }
};
inline void moduleVersion(const std::filesystem::path& path,unsigned index,p::AdvancedState& state){
    if(index>=5)return;
    HMODULE held=nullptr;
    // GetModuleHandleEx never loads a provider. Hold only an already loaded,
    // exact-path module while inspecting its in-memory version resource.
    if(!GetModuleHandleExW(0,path.c_str(),&held))return;
    struct Release{HMODULE h;~Release(){FreeLibrary(h);}} release{held};
    auto text=std::make_unique<wchar_t[]>(32768);
    const auto length=GetModuleFileNameW(held,text.get(),32768);
    if(!length||length>=32768||CompareStringOrdinal(text.get(),int(length),path.c_str(),int(path.native().size()),TRUE)!=CSTR_EQUAL)return;
    state.modules|=p::advancedLoadedBit(index);
    auto resource=FindResourceW(held,MAKEINTRESOURCEW(VS_VERSION_INFO),MAKEINTRESOURCEW(16));
    if(!resource)return;
    const auto bytes=SizeofResource(held,resource);auto loaded=LoadResource(held,resource);auto data=loaded?LockResource(loaded):nullptr;
    VS_FIXEDFILEINFO* info=nullptr;UINT size=0;
    if(bytes<sizeof(VS_FIXEDFILEINFO)||!data||!VerQueryValueW(data,L"\\",reinterpret_cast<void**>(&info),&size)||
       !info||size<sizeof(VS_FIXEDFILEINFO)||info->dwSignature!=0xfeef04bd)return;
    state.modules|=p::advancedVersionBit(index);
    state.versions[index*4]=HIWORD(info->dwFileVersionMS);state.versions[index*4+1]=LOWORD(info->dwFileVersionMS);
    state.versions[index*4+2]=HIWORD(info->dwFileVersionLS);state.versions[index*4+3]=LOWORD(info->dwFileVersionLS);
}
class SaveTransport {
    std::filesystem::path saves_,root_;
    p::GraphicsStore store_;
    IniHint ini_;
    p::AdvancedRequests requests_;
    std::uint64_t versionAt_=0;
    p::AdvancedState versions_;
    static unsigned startup(const IniHint& ini){std::uint32_t hint=0;return ini.read(hint)?hint:0;}
public:
    SaveTransport(std::filesystem::path saves,std::filesystem::path authority,std::filesystem::path root,unsigned session)
        :saves_(std::move(saves)),root_(std::move(root)),store_(std::move(authority)),
         ini_{root_/L"DependencyOverrides.ini"},requests_(session,startup(ini_)){}
    void poll(const p::MenuAuthority& core){
        if(core.load!=p::StoreStatus::Ok)return;
        p::SlotBytes save;p::AdvancedRequestBytes words;p::AdvancedRequest request;
        const auto requestPath=saves_/L"MCD2GraphicsAdvancedRequest.sav";
        if(p::readObservedSlot(requestPath,save)&&p::menu_save_detail::parse(save,"AdvancedRequestSave",words)&&p::decodeAdvancedRequest(words,request)){
            p::store_detail::WriterLock lock(store_.lockPath());
            p::DecodedGraphicsRecord current;auto loaded=store_.load(current);
            if(lock.status==p::StoreStatus::Ok && loaded.status==p::StoreStatus::Ok){
                requests_.consume(request,loaded.stamp,ini_,[&]{
                    p::SlotBytes fresh;p::DecodedGraphicsRecord check;
                    return p::readObservedSlot(requestPath,fresh)&&fresh==save&&store_.load(check).status==p::StoreStatus::Ok&&check.bootstrap.stamp==request.expected;
                });
            }
        }
        const auto now=GetTickCount64();
        if(!versionAt_||now-versionAt_>=1000){
            versions_={};
            moduleVersion(root_/L"ngx-runtime"/L"nvngx_dlss.dll",0,versions_);
            moduleVersion(root_.parent_path()/L"sl.interposer.dll",1,versions_);
            if(!(versions_.modules&2))moduleVersion(root_/L"streamline"/L"sl.interposer.dll",1,versions_);
            moduleVersion(root_/L"fsr"/L"amd_fidelityfx_upscaler_dx12.dll",2,versions_);
            moduleVersion(root_.parent_path()/L"amd_fidelityfx_framegeneration_dx12.dll",3,versions_);
            moduleVersion(root_.parent_path()/L"nvngx_dlssg.dll",4,versions_);
            if(!(versions_.modules&p::advancedLoadedBit(4)))moduleVersion(root_/L"streamline"/L"nvngx_dlssg.dll",4,versions_);
            versionAt_=now;
        }
        auto state=requests_.state();state.modules=versions_.modules;state.versions=versions_.versions;
        p::AdvancedStateBytes bytes;p::SlotBytes seed,encoded;
        const auto path=saves_/L"MCD2GraphicsAdvancedState.sav";
        if(!p::encodeAdvancedState(state,bytes)||!p::readObservedSlot(path,seed)||
           !p::menu_save_detail::encode(seed,"AdvancedStateSave",bytes,encoded)||seed==encoded)return;
        p::store_detail::WriterLock lock(saves_/L"MCD2GraphicsAdvancedState.lock");p::SlotBytes fresh;
        if(lock.status!=p::StoreStatus::Ok||!p::readObservedSlot(path,fresh)||fresh!=seed)return;
        const auto pending=saves_/L"MCD2GraphicsAdvancedState.pending";
        if(!p::store_detail::discardPending(pending)||!p::store_detail::writeFlushed(pending,encoded))return;
        p::SlotBytes staged;
        if(p::readObservedSlot(pending,staged)&&staged==encoded&&p::store_detail::replace(pending,path))p::store_detail::syncDirectory(saves_);
    }
};
} // namespace mcd2::advanced
