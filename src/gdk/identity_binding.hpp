#pragma once
#include "save_slots.hpp"
#include <filesystem>
namespace mcd2::gdk {
enum class IdentityStatus { Ready,Missing,Ambiguous,Filesystem };
struct BoundIdentity {Identity identity;std::uintptr_t handle=0;};
// Directory names nominate candidates; only the title's public user API can
// admit one. No store/provider initialization or blob reads occur here.
template<class Find,class Close>
IdentityStatus selectIdentity(const std::filesystem::path& wgs,Find find,Close close,BoundIdentity& out){
    std::error_code error;BoundIdentity selected;
    for(std::filesystem::directory_iterator it(wgs,error),end;!error&&it!=end;it.increment(error)){
        const auto status=it->symlink_status(error);if(error)break;
        if(!std::filesystem::is_directory(status))continue;
        Identity identity;if(!mcd2::gdk::identity(it->path().filename().string(),identity))continue;
        const auto handle=find(identity.user);if(!handle)continue;
        if(selected.handle){close(handle);close(selected.handle);return IdentityStatus::Ambiguous;}
        selected={std::move(identity),handle};
    }
    if(error){if(selected.handle)close(selected.handle);return IdentityStatus::Filesystem;}
    if(!selected.handle)return IdentityStatus::Missing;
    out=std::move(selected);return IdentityStatus::Ready;
}
}
