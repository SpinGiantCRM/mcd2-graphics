#pragma once
#include <cstdint>
#include <mutex>

namespace mcd2::providers {
// Provider enumeration is a device capability query, not a frame operation.
// The bridge's -1 means not initialized yet and must remain retryable. Other
// results describe this device/runtime pair; FG creation still checks admission.
class FgCapabilityCache {
    std::mutex mutex_;
    std::uintptr_t device_=0,module_=0;
    int result_=-1;
public:
    template<class Probe> int query(std::uintptr_t device,std::uintptr_t module,Probe&& probe){
        if(!device||!module)return -1;
        std::lock_guard lock(mutex_);
        if(device_==device&&module_==module)return result_;
        const int result=probe();
        if(result!=-1){device_=device;module_=module;result_=result;}
        return result;
    }
    void forget(std::uintptr_t device){
        std::lock_guard lock(mutex_);
        if(device_==device){device_=module_=0;result_=-1;}
    }
};
}
