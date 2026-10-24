#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <reshade.hpp>
#include "game_save_api.hpp"
#include "cache_time.hpp"
#include "save_lifecycle.hpp"
#include "../latency/process_exit.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <memory>
#include <mutex>
#include <thread>
#include <bcrypt.h>

namespace {
using namespace mcd2::gdk;
struct Worker {
    std::atomic<bool> stop=false;
    std::mutex mutex;
    std::condition_variable wake;
    std::thread thread;
    WorkerCompletion completion;
};
Worker* worker=nullptr;
Store* preparedStore=nullptr;
bool addonRegistered=false,closing=false;
std::mutex lifecycleMutex;
std::mutex pathMutex;
std::filesystem::path boundSaveRoot;
void clearRoot(){std::lock_guard lock(pathMutex);boundSaveRoot.clear();}
bool privateDirectory(const std::filesystem::path& path){
    for(auto cursor=path;!cursor.empty();cursor=cursor.parent_path()){
        const auto attributes=GetFileAttributesW(cursor.c_str());
        if(attributes!=INVALID_FILE_ATTRIBUTES&&(attributes&FILE_ATTRIBUTE_REPARSE_POINT))return false;
        if(cursor==cursor.root_path())break;
    }
    std::filesystem::create_directories(path);return true;
}
std::string bindingHash(const std::string& binding){
    auto module=LoadLibraryExW(L"bcrypt.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);if(!module)return {};
    auto open=reinterpret_cast<decltype(&BCryptOpenAlgorithmProvider)>(GetProcAddress(module,"BCryptOpenAlgorithmProvider"));
    auto hash=reinterpret_cast<decltype(&BCryptHash)>(GetProcAddress(module,"BCryptHash"));
    auto close=reinterpret_cast<decltype(&BCryptCloseAlgorithmProvider)>(GetProcAddress(module,"BCryptCloseAlgorithmProvider"));
    BCRYPT_ALG_HANDLE algorithm=nullptr;std::array<UCHAR,32> digest{};std::string result;
    if(open&&hash&&close&&open(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0){
        if(hash(algorithm,nullptr,0,reinterpret_cast<PUCHAR>(const_cast<char*>(binding.data())),ULONG(binding.size()),digest.data(),ULONG(digest.size()))>=0)
            for(auto b:digest){result+="0123456789abcdef"[b>>4];result+="0123456789abcdef"[b&15];}
        close(algorithm,0);
    }
    FreeLibrary(module);return result;
}
std::uintptr_t saveOwner=0;
void stopWorker() {
    std::lock_guard lock(lifecycleMutex);
    closing=true;clearRoot();
    if(!worker){delete preparedStore;preparedStore=nullptr;return;}
    worker->stop=true;worker->wake.notify_all();
    clearRoot();
    // Sync XGameSave calls cannot be cancelled. The addon is process-pinned;
    // unfinished work retains its state, handles and binding until it returns.
    if(!worker->completion.wait(std::chrono::milliseconds(250)))return;
    if(worker->thread.joinable())worker->thread.join();
    delete worker;worker=nullptr;
}
void swap_init(reshade::api::swapchain* swap,bool) {
    if(mcd2::process_exit::terminating())return;
    const auto window=static_cast<HWND>(swap->get_hwnd());DWORD pid=0;wchar_t name[256]{};
    GetWindowThreadProcessId(window,&pid);GetClassNameW(window,name,256);
    if(pid!=GetCurrentProcessId()||std::wcscmp(name,L"UnrealWindow"))return;
    std::lock_guard lock(lifecycleMutex);
    if(worker&&!saveOwner)saveOwner=reinterpret_cast<std::uintptr_t>(swap);
}
void swap_destroy(reshade::api::swapchain* swap,bool resize) {
    if(mcd2::process_exit::terminating())return;
    bool close=false;
    {std::lock_guard lock(lifecycleMutex);close=closesSaveOwner(false,resize,saveOwner,reinterpret_cast<std::uintptr_t>(swap));}
    // Release save handles and our middleware binding before final DLL teardown.
    if(close)stopWorker();
}
std::atomic<uint32_t> cacheClockResolution{0};
bool readCache(const std::filesystem::path& path,const Slot& slot,std::vector<std::uint8_t>& bytes) {
    const auto attributes=GetFileAttributesW(path.c_str());
    if(attributes==INVALID_FILE_ATTRIBUTES || (attributes&FILE_ATTRIBUTE_REPARSE_POINT))return false;
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    const auto size=file.tellg();if(size<64||size>8192)return false;
    std::vector<std::uint8_t> next(static_cast<std::size_t>(size));file.seekg(0);
    if(!file.read(reinterpret_cast<char*>(next.data()),next.size())||!valid(slot,next,false))return false;
    bytes=std::move(next);return true;
}
HRESULT writeCache(const std::filesystem::path& path,const std::vector<std::uint8_t>& bytes,const FILETIME* timestamp) {
    auto temporary=path;temporary+=L".gdk.tmp";
    const auto attributes=GetFileAttributesW(path.c_str());
    if(attributes!=INVALID_FILE_ATTRIBUTES && (attributes&FILE_ATTRIBUTE_REPARSE_POINT))return E_ACCESSDENIED;
    const auto file=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return HRESULT_FROM_WIN32(GetLastError());
    DWORD count=0;const auto written=WriteFile(file,bytes.data(),static_cast<DWORD>(bytes.size()),&count,nullptr);
    auto result=written&&count==bytes.size()?S_OK:HRESULT_FROM_WIN32(written?ERROR_WRITE_FAULT:GetLastError());
    if(SUCCEEDED(result)&&timestamp&&!SetFileTime(file,nullptr,nullptr,timestamp))result=HRESULT_FROM_WIN32(GetLastError());
    CloseHandle(file);
    if(SUCCEEDED(result)&&!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))result=HRESULT_FROM_WIN32(GetLastError());
    if(FAILED(result))DeleteFileW(temporary.c_str());return result;
}
HRESULT cacheClock(const std::filesystem::path& path,const FILETIME& timestamp){
    const auto file=CreateFileW(path.c_str(),FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    if(file==INVALID_HANDLE_VALUE)return HRESULT_FROM_WIN32(GetLastError());
    BY_HANDLE_FILE_INFORMATION info{};HRESULT result=S_OK;
    if(!GetFileInformationByHandle(file,&info))result=HRESULT_FROM_WIN32(GetLastError());
    else if(info.dwFileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY))result=E_ACCESSDENIED;
    else if(CompareFileTime(&info.ftLastWriteTime,&timestamp)!=0&&!SetFileTime(file,nullptr,nullptr,&timestamp))result=HRESULT_FROM_WIN32(GetLastError());
    CloseHandle(file);return result;
}
HRESULT prepareBinding(HMODULE module){
    if(closing)return E_ABORT;if(preparedStore)return S_OK;
    std::array<wchar_t,256> family{};UINT32 length=static_cast<UINT32>(family.size());
    if(GetCurrentPackageFamilyName(&length,family.data())!=ERROR_SUCCESS||std::wstring_view(family.data()).find(L"Microsoft.MinecraftDungeons2_")!=0)return E_ACCESSDENIED;
    std::vector<wchar_t> local(32768),filename(32768);
    const auto count=GetEnvironmentVariableW(L"LOCALAPPDATA",local.data(),DWORD(local.size()));
    const auto n=GetModuleFileNameW(module,filename.data(),DWORD(filename.size()));
    if(!count||count>=local.size()||!n||n>=filename.size())return E_INVALIDARG;
    const auto binding=std::filesystem::path(filename.data()).parent_path()/L"MCD2Graphics"/L"gdk"/L"xgameruntime.thunks.dll";
    const auto root=std::filesystem::path(local.data());
    auto candidate=std::make_unique<Store>();
    const auto result=candidate->bindUser(root/L"Packages"/family.data()/L"SystemAppData"/L"wgs",binding);
    if(FAILED(result))return result;
    const auto token=bindingHash(candidate->accountBinding);if(token.size()!=64)return E_FAIL;
    const auto cache=root/L"Dungeons2"/L"Saved"/L"MCD2Graphics"/L"WinGDK"/token/L"SaveGames";
    if(!privateDirectory(cache))return E_ACCESSDENIED;
    HMODULE pinned=nullptr;if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&prepareBinding),&pinned))return E_FAIL;
    preparedStore=candidate.release();{std::lock_guard lock(pathMutex);boundSaveRoot=cache;}return S_OK;
}
void run(Worker& state,std::filesystem::path wgs,std::filesystem::path cacheBase,std::filesystem::path receipt,std::filesystem::path binding,std::unique_ptr<Store> initial) {
    std::ofstream log(receipt,std::ios::trunc);
    if(!log)return;
    auto report=[&](const char* operation,const char* slot,HRESULT result){
        log<<operation<<" slot="<<slot<<" result=0x"<<std::hex<<static_cast<std::uint32_t>(result)<<std::dec<<'\n';log.flush();
    };
    try {
        std::unique_ptr<Store> store;
        std::array<bool,slots.size()> seeded{};
        std::array<std::vector<std::uint8_t>,slots.size()> published;
        std::array<HRESULT,slots.size()> errors{};
        HRESULT previous=E_PENDING;
        std::filesystem::path cache;
        while(!state.stop && !mcd2::process_exit::terminating()){
            if(!store){
                auto candidate=initial?std::move(initial):std::make_unique<Store>();const auto result=candidate->user?candidate->openProvider():candidate->open(wgs,binding);
                if(result!=previous){report("provider","none",result);previous=result;}
                if(SUCCEEDED(result)&&!state.stop){
                    const auto token=bindingHash(candidate->accountBinding);
                    if(token.size()!=64){report("identity-binding","none",E_FAIL);return;}
                    cache=cacheBase/token/L"SaveGames";
                    if(!privateDirectory(cache)){report("cache-ownership","none",E_ACCESSDENIED);return;}
                    // Refuse linked cache components, including pre-existing parents.
                    for(auto cursor=cache;cursor!=cacheBase.parent_path();cursor=cursor.parent_path())
                        if(GetFileAttributesW(cursor.c_str())&FILE_ATTRIBUTE_REPARSE_POINT){report("cache-ownership","none",E_ACCESSDENIED);return;}
                    store=std::move(candidate);
                    {std::lock_guard lock(pathMutex);if(!state.stop)boundSaveRoot=cache;}
                }
            }
            if(store){
                for(std::size_t index=0;index<slots.size()&&!state.stop;++index){
                    const auto& slot=slots[index];const auto path=cache/(std::string(slot.name)+".sav");
                    std::int64_t modified=0;std::vector<std::uint8_t> remote,local;auto result=store->readSlot(slot,remote,modified);
                    if(state.stop)break;
                    FILETIME sourceTime{};const FILETIME* timestamp=nullptr;
                    if(SUCCEEDED(result)&&!slot.nativeWrites){
                        FILETIME now{};GetSystemTimeAsFileTime(&now);ULARGE_INTEGER current{},source{};current.LowPart=now.dwLowDateTime;current.HighPart=now.dwHighDateTime;
                        if(!cacheFileTime(modified,current.QuadPart,source.QuadPart))result=E_INVALIDARG;
                        else{sourceTime.dwLowDateTime=source.LowPart;sourceTime.dwHighDateTime=source.HighPart;timestamp=&sourceTime;}
                    }
                    if(SUCCEEDED(result)){
                        const auto canonical=nativePayload(remote);
                        const bool hasLocal=readCache(path,slot,local);
                        if(!slot.nativeWrites || !seeded[index]){
                            if(!hasLocal||local!=canonical){result=writeCache(path,canonical,timestamp);if(SUCCEEDED(result))report("cache",slot.name,result);}
                            else if(timestamp)result=cacheClock(path,*timestamp);
                            if(SUCCEEDED(result)&&timestamp&&std::strcmp(slot.name,"MCD2GraphicsFGSettings")==0)cacheClockResolution=1000;
                            if(SUCCEEDED(result)){seeded[index]=true;published[index]=canonical;}
                        }else if(hasLocal&&(local!=published[index]||local!=canonical)){
                            const auto payload=storePayload(local,remote);
                            result=store->writeSlot(slot,payload);
                            if(SUCCEEDED(result)){
                                std::int64_t verifiedModified=0;std::vector<std::uint8_t> verified;result=store->readSlot(slot,verified,verifiedModified);
                                if(SUCCEEDED(result)&&verified!=payload)result=HRESULT_FROM_WIN32(ERROR_RETRY);
                                if(SUCCEEDED(result)){published[index]=local;report("publish",slot.name,result);}
                            }
                        }
                    }
                    if(result!=errors[index]){
                        report("slot",slot.name,result);errors[index]=result;
                        if(FAILED(result)){log<<"read stage="<<store->readStage<<" blobs="<<store->blobCount<<" bytes="<<store->dataSize<<'\n';log.flush();}
                    }
                }
            }
            std::unique_lock lock(state.mutex);state.wake.wait_for(lock,std::chrono::milliseconds(store?200:1000),[&]{return state.stop.load();});
        }
    }catch(const std::filesystem::filesystem_error& error){report("filesystem","none",HRESULT_FROM_WIN32(error.code().value()));}
    catch(const std::exception&){report("exception","none",E_FAIL);}
    clearRoot();cacheClockResolution=0;report("stopped","none",S_OK);
}
}
extern "C" __declspec(dllexport) const char* NAME="MCD2 Graphics GDK save transport";
extern "C" __declspec(dllexport) uint32_t mcd2_gdk_cache_clock_resolution_v1(){return cacheClockResolution.load();}
extern "C" __declspec(dllexport) int mcd2_gdk_save_root_v1(wchar_t* path,uint32_t capacity){
    std::unique_lock lock(pathMutex,std::try_to_lock);if(!lock.owns_lock())return -2;
    if(!path||boundSaveRoot.empty()||capacity<=boundSaveRoot.native().size())return -3;
    const auto& value=boundSaveRoot.native();std::copy(value.begin(),value.end(),path);path[value.size()]=0;return 0;
}
extern "C" __declspec(dllexport) const char* DESCRIPTION="Identity-bound Xbox Game Save transport for MCD2 Graphics";
BOOL APIENTRY DllMain(HMODULE module,DWORD reason,LPVOID reserved){
    if(reason==DLL_PROCESS_ATTACH)return TRUE;
    if(reason==DLL_PROCESS_DETACH){if(reserved)mcd2::process_exit::mark_terminating();else if(addonRegistered)reshade::unregister_addon(module);}
    return TRUE;
}
extern "C" __declspec(dllexport) int mcd2_gdk_prepare_v1(HMODULE module){
    mcd2::process_exit::initialize();std::lock_guard lock(lifecycleMutex);
    try{return int(prepareBinding(module));}catch(...){return int(E_FAIL);}
}
extern "C" __declspec(dllexport) bool AddonInit(HMODULE module,HMODULE){
    mcd2::process_exit::initialize();std::lock_guard lock(lifecycleMutex);if(worker)return true;
    if(!addonRegistered){if(!reshade::register_addon(module))return false;addonRegistered=true;}
    if(closing)return false;
    std::array<wchar_t,256> family{};UINT32 length=static_cast<UINT32>(family.size());
    if(GetCurrentPackageFamilyName(&length,family.data())!=ERROR_SUCCESS || std::wstring_view(family.data()).find(L"Microsoft.MinecraftDungeons2_")!=0)return true;
    try {
        auto local=std::make_unique<wchar_t[]>(32768);const auto count=GetEnvironmentVariableW(L"LOCALAPPDATA",local.get(),32768);
        if(!count||count>=32768)return false;
        auto filename=std::make_unique<wchar_t[]>(32768);const auto moduleLength=GetModuleFileNameW(module,filename.get(),32768);
        if(!moduleLength||moduleLength>=32768)return false;
        const auto binding=std::filesystem::path(filename.get()).parent_path()/L"MCD2Graphics"/L"gdk"/L"xgameruntime.thunks.dll";
        const auto root=std::filesystem::path(local.get());const auto saved=root/L"Dungeons2"/L"Saved";
        const auto cache=saved/L"MCD2Graphics"/L"WinGDK",logs=saved/L"MCD2Graphics"/L"WinGDKLogs";
        if(!privateDirectory(logs))return false;
        if(GetFileAttributesW(logs.c_str())&FILE_ATTRIBUTE_REPARSE_POINT)return false;
        // Unload cannot invalidate an in-flight synchronous GDK call or its return address.
        HMODULE pinned=nullptr;if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&mcd2_gdk_save_root_v1),&pinned))return false;
        auto state=std::make_unique<Worker>();
        // Bind identity before thread startup when available. Provider/cloud initialization stays on the worker.
        prepareBinding(module);
        std::unique_ptr<Store> initial(preparedStore);preparedStore=nullptr;
        auto* work=state.get();
        state->thread=std::thread([work,wgs=root/L"Packages"/family.data()/L"SystemAppData"/L"wgs",cache,receipt=logs/L"gdk-save-bridge.log",binding,initial=std::move(initial)]()mutable{run(*work,wgs,cache,receipt,binding,std::move(initial));work->completion.finish();});
        worker=state.release();saveOwner=0;
        reshade::register_event<reshade::addon_event::init_swapchain>(swap_init);
        reshade::register_event<reshade::addon_event::destroy_swapchain>(swap_destroy);
        return true;
    }catch(const std::exception&){return false;}
}
extern "C" __declspec(dllexport) void AddonUninit(HMODULE module,HMODULE){
    if(mcd2::process_exit::terminating())return;
    reshade::unregister_event<reshade::addon_event::init_swapchain>(swap_init);
    reshade::unregister_event<reshade::addon_event::destroy_swapchain>(swap_destroy);
    stopWorker();
    if(addonRegistered){reshade::unregister_addon(module);addonRegistered=false;}
}
