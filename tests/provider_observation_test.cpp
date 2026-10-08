#include "../src/providers/observed_settings.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <random>

using namespace mcd2::providers;
using Fields = std::map<std::string, unsigned>;
static void number(SlotBytes& b, unsigned v, unsigned width = 4) {
    for (unsigned n=0; n<width; ++n) b.push_back(std::uint8_t(v>>(8*n)));
}
static void string(SlotBytes& b, const std::string& s) {
    number(b, unsigned(s.size()+1)); b.insert(b.end(),s.begin(),s.end()); b.push_back(0);
}
static SlotBytes slot(const std::string& cls, const Fields& fields) {
    SlotBytes b;
    for (auto v : {0x53415647u,3u,522u,1017u}) number(b,v);
    for (auto v : {5u,6u,1u}) number(b,v,2);
    number(b,0); string(b,"UE5"); number(b,3); number(b,93); b.resize(b.size()+93*20);
    string(b,"/Game/Mods/MCD2Graphics/"+cls+"."+cls+"_C"); number(b,0,1);
    for (const auto& [name,value] : fields) {
        string(b,name); string(b,"IntProperty"); number(b,0); number(b,4); number(b,0,1); number(b,value);
    }
    string(b,"None"); number(b,0); return b;
}
static void write(const std::filesystem::path& path, std::span<const std::uint8_t> bytes) {
    std::ofstream f(path,std::ios::binary|std::ios::trunc);
    f.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size())); assert(f.good());
}
int main() {
    namespace fs=std::filesystem;
    std::random_device random;
    fs::path root;
    for (unsigned n=0; n!=10; ++n) {
        root=fs::temp_directory_path()/("mcd2-observation-"+std::to_string(random()));
        if (fs::create_directory(root)) break;
        root.clear();
    }
    assert(!root.empty());
    struct Cleanup { fs::path p; ~Cleanup(){std::error_code e;fs::remove_all(p,e);} } cleanup{root};
    root/=fs::path(u8"settings-測試"); fs::create_directory(root);
    auto saves=root/"SaveGames", mirror=root/"ProviderMirror";
    fs::create_directory(saves); fs::create_directory(mirror);
    GraphicsStore store(mirror);
    Fields s{{"SchemaVersion",4},{"Revision",22},{"ReconstructionMode",2},{"SRPreset",0},
        {"RenderScaleBasisPoints",6667},{"CustomScaleBasisPoints",6700},{"LastCustomScaleBasisPoints",7300},
        {"AppliedSourceRevision",21},{"AppliedSourceSessionId",345},{"RenderContextReady",1},{"RenderContextSessionId",345}};
    Fields d{{"SchemaVersion",1},{"Revision",3},{"HDROutput",1},{"PeakNits",410},
        {"PaperWhiteNits",167},{"UINits",203},{"ReflexMode",2}};
    Fields f{{"SchemaVersion",1},{"Revision",31},{"Mode",1},{"ContextReady",1},{"SessionId",345}};
    auto snapshot=[&]{return LegacySnapshot{slot("GraphicsSettingsSave",s),slot("DisplaySettingsSave",d),slot("FGSettingsSave",f)};};
    auto original=snapshot();
    auto save=[&](const LegacySnapshot& b){
        write(saves/"MCD2GraphicsSettings.sav",b.sr);
        write(saves/"MCD2GraphicsDisplaySettings.sav",b.display);
        write(saves/"MCD2GraphicsFGSettings.sav",b.fg);
    };
    save(original);
    LegacySnapshot disk;
    assert(readLegacySnapshot(saves,disk)&&disk==original);
    assert(!observedStartup(store,disk,123).valid); // No legacy-only fallback.
    ObservedSettings observer;
    assert(observer.observe(disk,store).status==ObservationStatus::Waiting);
    assert(observer.observe(disk,store).status==ObservationStatus::Published);
    DecodedGraphicsRecord current;
    assert(store.load(current).status==StoreStatus::Ok&&current.intent.revision==31);
    const auto first=current;
    const auto startup=observedStartup(store,disk,123);
    assert(startup.valid&&startup.session==123&&startup.bootstrap.stamp==first.bootstrap.stamp);
    assert(startup.bootstrap.requested==PresentationOwner::NvidiaStreamline);
    assert(!observedStartup(store,disk,0).valid);
    // Rendering/source/context ACKs neither republish nor change startup match.
    s["Revision"]=50; s["AppliedSourceRevision"]=50; s["AppliedSourceSessionId"]=678;
    s["RenderContextSessionId"]=678; f["Revision"]=48; f["SessionId"]=678;
    disk=snapshot();
    assert(observer.observe(disk,store).status==ObservationStatus::Waiting);
    assert(observer.observe(disk,store).status==ObservationStatus::Unchanged);
    assert(store.load(current).status==StoreStatus::Ok&&current==first);
    assert(observedStartup(store,disk,123).valid);
    // Even a lower independent display revision produces a newer global one.
    d["Revision"]=4; d["PeakNits"]=420; disk=snapshot();
    assert(!observedStartup(store,disk,123).valid); // Pending menu preference is stale.
    assert(observer.observe(disk,store).status==ObservationStatus::Waiting);
    assert(observer.observe(disk,store).status==ObservationStatus::Published);
    assert(store.load(current).status==StoreStatus::Ok&&current.intent.revision==50);
    assert(current.intent.hdr.peakNits==420&&current.intent.migratedFrom==first.intent.migratedFrom);
    // Changed bytes between polls are never committed from the old observation.
    f["Mode"]=0; f["Revision"]=49; auto off=snapshot();
    assert(observer.observe(off,store).status==ObservationStatus::Waiting);
    assert(observer.observe(disk,store).status==ObservationStatus::Waiting);
    observer.invalidate();
    assert(observer.observe(off,store).status==ObservationStatus::Waiting);
    assert(observer.observe(off,store).status==ObservationStatus::Published);
    assert(observedStartup(store,off,123).valid);
    assert(observedStartup(store,off,123).bootstrap.requested==PresentationOwner::Native);
    // Invalid input interrupts the stability window and cannot change output.
    auto bad=off; bad.sr.resize(30);
    assert(observer.observe(bad,store).status==ObservationStatus::Rejected);
    assert(observer.observe(off,store).status==ObservationStatus::Waiting);
    assert(observer.observe(off,store).status==ObservationStatus::Unchanged);
    // Writes only touch the reserved mirror paths, never any legacy slot.
    assert(readLegacySnapshot(saves,disk)&&disk==original);
    fs::remove(saves/"MCD2GraphicsDisplaySettings.sav"); auto held=disk;
    assert(!readLegacySnapshot(saves,disk)&&disk==held);
    save(original);
    SlotBytes malformed{1,2,3};write(store.committedPath(),malformed);
    assert(!observedStartup(store,original,123).valid);
    assert(observer.observe(original,store).status==ObservationStatus::Waiting);
    auto refused=observer.observe(original,store);
    assert(refused.status==ObservationStatus::StoreFailure&&refused.store==StoreStatus::Invalid);
    SlotBytes remaining; // The corrupt file has not been repaired from legacy.
    {std::ifstream file(store.committedPath(),std::ios::binary); remaining.assign(std::istreambuf_iterator<char>(file),{});}
    assert(remaining==malformed);
    fs::remove(store.committedPath());
    GraphicsIntent future; future.sr=SrProvider::AmdFsr;future.fg=FgProvider::Amd;
    assert(store.publish(future,{}).status==StoreStatus::Ok);
    assert(observer.observe(original,store).status==ObservationStatus::Rejected);
    assert(store.load(current).status==StoreStatus::Ok&&current.intent==future);
    assert(!observedStartup(store,original,123).valid);
    // Saturated revision cannot wrap and overwrite a committed request.
    fs::remove(store.committedPath()); GraphicsIntent saturated;
    assert(decodeObserved(original,saturated)); saturated.revision=0x7fffffff;
    assert(store.publish(saturated,{}).status==StoreStatus::Ok);
    assert(observer.observe(off,store).status==ObservationStatus::Waiting);
    assert(observer.observe(off,store).status==ObservationStatus::Rejected);
    assert(store.load(current).status==StoreStatus::Ok&&current.intent==saturated);
#ifndef _WIN32
    fs::remove(saves/"MCD2GraphicsSettings.sav");
    fs::create_symlink(store.committedPath(),saves/"MCD2GraphicsSettings.sav");
    assert(!readLegacySnapshot(saves,disk));
#endif
    std::cout<<"Observation, startup, acknowledgement, corruption, future-provider and legacy-preservation gates passed.\n";
}
