#pragma once
#include "menu_save_transport.hpp"
#include "sr_runtime_protocol.hpp"

namespace mcd2::providers {
inline bool readSrContextSave(const std::filesystem::path& path,SrRuntimeBytes& out){
    SlotBytes save;SrContext context;SrRuntimeBytes bytes;
    if(!readObservedSlot(path,save)||!menu_save_detail::parse(save,"ProviderSrContextSave",bytes)||
       !decodeSrContext(bytes,context))return false;
    out=bytes;return true;
}
inline bool publishSrRuntimeSave(const std::filesystem::path& saves,const SrRuntimeBytes& bytes){
    SrRuntimeState state;if(!decodeSrRuntimeState(bytes,state))return false;
    const auto path=saves/"MCD2GraphicsProviderSrRuntime.sav";
    SlotBytes seed,encoded;
    if(!readObservedSlot(path,seed)||!menu_save_detail::encode(seed,"ProviderSrRuntimeSave",bytes,encoded))return false;
    if(seed==encoded)return true;
    store_detail::WriterLock lock(saves/"MCD2GraphicsProviderSrRuntime.lock");SlotBytes fresh;
    if(lock.status!=StoreStatus::Ok||!readObservedSlot(path,fresh)||fresh!=seed)return false;
    const auto pending=saves/"MCD2GraphicsProviderSrRuntime.pending";
    if(!store_detail::discardPending(pending)||!store_detail::writeFlushed(pending,encoded))return false;
    SlotBytes staged;
    if(!readObservedSlot(pending,staged)||staged!=encoded||!store_detail::replace(pending,path))return false;
    return store_detail::syncDirectory(saves);
}
} // namespace mcd2::providers
