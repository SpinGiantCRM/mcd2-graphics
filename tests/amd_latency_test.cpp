#include "../src/latency/amd_latency.hpp"
#include <cassert>
#include <condition_variable>
#include <iostream>
#include <thread>
#include <vector>
using namespace mcd2;
using namespace amd_latency;
struct Fake : Provider {
    bool initOk=true, updateOk=true, endOk=true, typeOk=true;
    unsigned inits=0, stops=0;
    std::vector<int> calls;
    bool waiting=false, resume=false, block=false;
    std::mutex mutex;std::condition_variable cv;
    bool initialize(std::uintptr_t device) override { assert(device==123);++inits;calls.push_back(0);return initOk; }
    bool update(bool enabled) override {
        calls.push_back(enabled?2:1);
        std::unique_lock lock(mutex);if(block){waiting=true;cv.notify_all();cv.wait(lock,[&]{return resume;});}
        return updateOk;
    }
    bool end_rendering() override {calls.push_back(3);return endOk;}
    bool real_frame() override {calls.push_back(4);return typeOk;}
    void shutdown() override {++stops;calls.push_back(5);}
};
static providers::DecodedGraphicsRecord decoded(providers::GraphicsIntent intent) {
    providers::GraphicsRecord bytes;providers::DecodedGraphicsRecord r;
    assert(providers::encodeGraphicsRecord(intent,bytes)&&providers::decodeGraphicsRecord(bytes,r));return r;
}
int main() {
    const Eligibility yes{true,true,true,0x1002};
    for(unsigned bits=0;bits<8;++bits)for(auto vendor:{0u,0x10deu,0x8086u,0x1002u}){
        Eligibility e{bool(bits&1),bool(bits&2),bool(bits&4),vendor};
        Fake p;Controller c(p);const bool expected=bits==7&&vendor==0x1002;
        assert(c.attach(e,123)==expected);assert(p.inits==(expected?1u:0u));
        c.pre_input(1,{true,true,1,1});c.pre_present(1);
        if(!expected)assert(p.calls.empty());
    }
    providers::GraphicsIntent i;i.latency=providers::LatencyProvider::RadeonAntiLag2;i.revision=8;
    auto r=decoded(i);auto requested=resolve(r);
    assert(requested.valid&&requested.enabled&&requested.revision==8&&requested.checksum==r.bootstrap.stamp.checksum);
    for(auto provider:{providers::LatencyProvider::Automatic,providers::LatencyProvider::NvidiaReflex,providers::LatencyProvider::RadeonAntiLag2,providers::LatencyProvider::IntelXeLL})
    for(auto mode:{providers::LatencyMode::Off,providers::LatencyMode::On,providers::LatencyMode::OnBoost})
    for(bool fg:{false,true}){
        i.latency=provider;i.latencyMode=mode;i.fgEnabled=fg;
        const auto q=resolve(decoded(i));assert(q.valid);
        assert(q.enabled==((provider==providers::LatencyProvider::Automatic||provider==providers::LatencyProvider::RadeonAntiLag2)&&mode==providers::LatencyMode::On&&!fg));
    }
    auto invalid=r;invalid.bootstrap.stamp.checksum^=1;assert(!resolve(invalid).valid);
    invalid=r;invalid.bootstrap.stamp.revision++;assert(!resolve(invalid).valid);
    invalid=r;invalid.intent.schema=6;assert(!resolve(invalid).valid);
    {Fake p;Controller c(p);assert(c.attach(yes,123));assert(!c.attach(yes,123));
     c.pre_input(0,requested);c.pre_present(1);assert(p.calls==std::vector<int>{0});
     c.pre_input(10,requested);c.pre_input(10,requested);c.pre_input(11,requested);
     c.pre_present(10);c.pre_present(10);c.pre_present(11);
     assert(p.calls==std::vector<int>({0,2,2,3,4,3,4}));
     auto s=c.state();assert(s.available&&s.enabled&&!s.fault&&s.inputFrames==2&&s.renderedFrames==2&&s.revision==8);
     c.pre_input(12,{});c.pre_present(12);assert(p.calls.back()==1&&!c.state().enabled&&c.state().revision==0);
     c.shutdown();c.pre_input(13,requested);c.pre_present(13);assert(p.stops==1&&!c.attach(yes,123));}
    for(int failure=0;failure<5;++failure){
        Fake p;Controller c(p);assert(c.attach(yes,123));
        if(failure==0)p.updateOk=false;
        c.pre_input(10,requested);
        if(failure==1)p.endOk=false;
        if(failure==2)p.typeOk=false;
        if(failure==3)c.pre_input(9,requested);
        c.pre_present(failure==4?11:10);
        auto s=c.state();assert(s.fault&&!s.available&&!s.enabled&&p.stops==1);
        auto count=p.calls.size();c.pre_input(12,requested);c.pre_present(12);c.shutdown();assert(p.calls.size()==count);
    }
    {Fake p;p.initOk=false;Controller c(p);assert(!c.attach(yes,123)&&p.stops==1);c.pre_input(1,requested);assert(p.calls==std::vector<int>({0,5}));}
    // Device destruction waits for an in-flight delay; retained callbacks cannot
    // call unloaded code after the shutdown barrier has completed.
    {Fake p;p.block=true;Controller c(p);assert(c.attach(yes,123));
     std::thread input([&]{c.pre_input(1,requested);});
     {std::unique_lock lock(p.mutex);p.cv.wait(lock,[&]{return p.waiting;});}
     std::thread destroy([&]{c.shutdown();});
     {std::lock_guard lock(p.mutex);p.resume=true;}p.cv.notify_all();input.join();destroy.join();
     c.pre_input(2,requested);c.pre_present(2);assert(p.calls==std::vector<int>({0,2,5})&&p.stops==1);}
    std::cout<<"Anti-Lag policy, ABI-call ordering, Off/fallback, identity and threaded teardown checks pass\n";
}
