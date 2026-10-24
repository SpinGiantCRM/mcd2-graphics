#pragma once
#include <windows.h>
#include <appmodel.h>
#include <XGameSave.h>
#include <XGameRuntimeInit.h>
#include <bit>
#include <filesystem>
#include <vector>
#include "save_slots.hpp"
#include "../latency/process_exit.hpp"

namespace mcd2::gdk {
static_assert(sizeof(void*)==8,"The GDK transport requires an x64 build");
using BlobInfo=XGameSaveBlobInfo;
using Blob=XGameSaveBlob;
struct Api {
    HMODULE module=nullptr;bool initialized=false;
    decltype(&XGameRuntimeInitialize) start=nullptr;
    decltype(&XGameRuntimeUninitialize) stop=nullptr;
    decltype(&XUserFindUserById) findUser=nullptr;
    decltype(&XUserCloseHandle) closeUser=nullptr;
    decltype(&XGameSaveInitializeProvider) initialize=nullptr;
    decltype(&XGameSaveCloseProvider) closeProvider=nullptr;
    decltype(&XGameSaveCreateContainer) container=nullptr;
    decltype(&XGameSaveCloseContainer) closeContainer=nullptr;
    decltype(&XGameSaveEnumerateBlobInfo) enumerate=nullptr;
    decltype(&XGameSaveEnumerateContainerInfoByName) containerInfo=nullptr;
    decltype(&XGameSaveReadBlobData) read=nullptr;
    decltype(&XGameSaveCreateUpdate) update=nullptr;
    decltype(&XGameSaveSubmitBlobWrite) write=nullptr;
    decltype(&XGameSaveSubmitUpdate) submit=nullptr;
    decltype(&XGameSaveCloseUpdate) closeUpdate=nullptr;
    ~Api(){if(mcd2::process_exit::terminating())return;if(initialized)stop();if(module)FreeLibrary(module);}
    template<class T> bool bind(HMODULE module,T& target,const char* name) {
        target=std::bit_cast<T>(GetProcAddress(module,name));return target!=nullptr;
    }
    HRESULT load(const std::filesystem::path& binding) {
        if(!GetModuleHandleW(L"xgameruntime.dll"))return E_PENDING;
        module=LoadLibraryExW(binding.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
        if(!module)return HRESULT_FROM_WIN32(GetLastError());
        const bool bound=bind(module,start,"XGameRuntimeInitialize") && bind(module,stop,"XGameRuntimeUninitialize") &&
            bind(module,findUser,"XUserFindUserById") && bind(module,closeUser,"XUserCloseHandle") &&
            bind(module,initialize,"XGameSaveInitializeProvider") && bind(module,closeProvider,"XGameSaveCloseProvider") &&
            bind(module,container,"XGameSaveCreateContainer") && bind(module,closeContainer,"XGameSaveCloseContainer") &&
            bind(module,enumerate,"XGameSaveEnumerateBlobInfo") && bind(module,containerInfo,"XGameSaveEnumerateContainerInfoByName") && bind(module,read,"XGameSaveReadBlobData") &&
            bind(module,update,"XGameSaveCreateUpdate") && bind(module,write,"XGameSaveSubmitBlobWrite") &&
            bind(module,submit,"XGameSaveSubmitUpdate") && bind(module,closeUpdate,"XGameSaveCloseUpdate");
        if(!bound)return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
        const auto result=start();initialized=SUCCEEDED(result);return result;
    }
};
struct Store {
    Api api;XUserHandle user=nullptr;XGameSaveProviderHandle provider=nullptr;
    const char* readStage="none";unsigned blobCount=0;std::uint32_t dataSize=0;
    Store()=default;Store(const Store&)=delete;Store& operator=(const Store&)=delete;
    ~Store(){if(mcd2::process_exit::terminating())return;if(provider)api.closeProvider(provider);if(user)api.closeUser(user);}
    HRESULT open(const std::filesystem::path& wgs,const std::filesystem::path& binding) {
        const auto result=api.load(binding);if(FAILED(result))return result;
        std::error_code error;Identity selected;
        for(std::filesystem::directory_iterator it(wgs,error),end;!error && it!=end;it.increment(error)){
            const auto status=it->symlink_status(error);if(error)return HRESULT_FROM_WIN32(error.value());
            if(!std::filesystem::is_directory(status))continue;
            Identity candidate;if(!identity(it->path().filename().string(),candidate))continue;
            XUserHandle handle=nullptr;if(FAILED(api.findUser(candidate.user,&handle)))continue;
            if(user){api.closeUser(handle);return E_UNEXPECTED;}
            user=handle;selected=std::move(candidate);
        }
        if(error)return HRESULT_FROM_WIN32(error.value());
        if(!user)return HRESULT_FROM_WIN32(ERROR_NO_SUCH_LOGON_SESSION);
        return api.initialize(user,selected.scid.c_str(),false,&provider);
    }
    HRESULT readSlot(const Slot& slot,std::vector<std::uint8_t>& bytes,std::int64_t& modified) {
        readStage="container";blobCount=0;dataSize=0;
        modified=0;
        if(!slot.nativeWrites){
            struct Metadata {const char* name;unsigned count=0;std::int64_t modified=0;bool valid=false;} metadata{slot.name};
            readStage="container-clock";
            auto result=api.containerInfo(provider,slot.name,&metadata,[](const XGameSaveContainerInfo* info,void* context)->bool{
                auto& metadata=*static_cast<Metadata*>(context);
                if(info->name&&std::strcmp(info->name,metadata.name)==0){++metadata.count;metadata.modified=info->lastModifiedTime;metadata.valid=info->blobCount==1&&info->totalSize>=64&&info->totalSize<=8192;}
                return true;
            });
            if(FAILED(result))return result;
            if(metadata.count!=1||!metadata.valid||metadata.modified<=0)return E_INVALIDARG;
            modified=metadata.modified;
        }
        XGameSaveContainerHandle handle=nullptr;auto result=api.container(provider,slot.name,&handle);if(FAILED(result))return result;
        struct Size {std::uint32_t bytes=0;unsigned count=0;} size;
        readStage="enumerate";
        result=api.enumerate(handle,&size,[](const BlobInfo* info,void* context)->bool {
            auto& size=*static_cast<Size*>(context);++size.count;
            if(info->name && std::strcmp(info->name,"Data")==0)size.bytes=info->size;return true;
        });
        blobCount=size.count;dataSize=size.bytes;
        if(SUCCEEDED(result)){readStage="metadata";if(size.count!=1 || size.bytes<64 || size.bytes>8192)result=E_INVALIDARG;}
        if(SUCCEEDED(result)){
            std::vector<std::uint64_t> storage((sizeof(Blob)+5+size.bytes+7)/8);
            auto* blob=reinterpret_cast<Blob*>(storage.data());const char* names[]={"Data"};std::uint32_t count=1;
            readStage="read";result=api.read(handle,names,&count,storage.size()*8,blob);
            if(SUCCEEDED(result)){
                const auto first=reinterpret_cast<std::uintptr_t>(storage.data()),last=first+storage.size()*8;
                const auto data=reinterpret_cast<std::uintptr_t>(blob->data);
                readStage="buffer";
                if(count!=1 || blob->info.size!=size.bytes || !blob->info.name || std::strcmp(blob->info.name,"Data") || data<first || data>last || blob->info.size>last-data)result=E_INVALIDARG;
                else {readStage="framing";std::vector<std::uint8_t> next(blob->data,blob->data+blob->info.size);if(!valid(slot,next))result=E_INVALIDARG;else bytes=std::move(next);}
            }
        }
        api.closeContainer(handle);return result;
    }
    HRESULT writeSlot(const Slot& slot,const std::vector<std::uint8_t>& bytes) {
        if(!slot.nativeWrites || !valid(slot,bytes))return E_INVALIDARG;
        XGameSaveContainerHandle handle=nullptr;auto result=api.container(provider,slot.name,&handle);if(FAILED(result))return result;
        XGameSaveUpdateHandle change=nullptr;result=api.update(handle,slot.name,&change);
        if(SUCCEEDED(result))result=api.write(change,"Data",bytes.data(),bytes.size());
        if(SUCCEEDED(result))result=api.submit(change);
        if(change)api.closeUpdate(change);api.closeContainer(handle);return result;
    }
};
}
