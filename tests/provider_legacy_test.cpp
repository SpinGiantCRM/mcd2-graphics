#include "../src/providers/legacy_graphics.hpp"
#include "../src/providers/graphics_record.hpp"
#include "../src/providers/graphics_store.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

using namespace mcd2::providers;
using Bytes = std::vector<std::uint8_t>;
using Fields = std::map<std::string, std::uint32_t>;

static void number(Bytes& bytes, std::uint32_t value, unsigned width = 4) {
    for (unsigned n = 0; n < width; ++n) bytes.push_back(std::uint8_t(value >> (n*8)));
}
static void string(Bytes& bytes, const std::string& value) {
    number(bytes, unsigned(value.size()+1));
    bytes.insert(bytes.end(), value.begin(), value.end()); bytes.push_back(0);
}
static Bytes save(const std::string& className, const Fields& fields, int fallback = -1,
                  bool duplicateLastCustom = false) {
    Bytes bytes;
    for (auto value : {0x53415647u, 3u, 522u, 1017u}) number(bytes, value);
    for (auto value : {5u, 6u, 1u}) number(bytes, value, 2);
    number(bytes, 0); string(bytes, "UE5"); number(bytes, 3); number(bytes, 93);
    bytes.resize(bytes.size()+93*20);
    string(bytes, "/Game/Mods/MCD2Graphics/"+className+"."+className+"_C"); number(bytes, 0, 1);
    auto scalar = [&](const std::string& name, unsigned value) {
        string(bytes, name); string(bytes, "IntProperty");
        number(bytes, 0); number(bytes, 4); number(bytes, 0, 1); number(bytes, value);
    };
    for (const auto& [name, value] : fields) scalar(name, value);
    if (duplicateLastCustom) scalar("LastCustomScaleBasisPoints", 7300);
    if (fallback >= 0) {
        string(bytes, "NativeFallback"); string(bytes, "BoolProperty");
        number(bytes, 0); number(bytes, 0); number(bytes, unsigned(fallback), 1);
    }
    string(bytes, "None"); number(bytes, 0);
    return bytes;
}

int main() {
    Fields s{{"SchemaVersion",4},{"Revision",22},{"ReconstructionMode",2},
        {"RenderScaleBasisPoints",6667},{"SRPreset",0},{"CustomScaleBasisPoints",6700},
        {"LastCustomScaleBasisPoints",7300},{"AppliedSourceRevision",21},
        {"AppliedSourceSessionId",345},{"RenderContextReady",1},{"RenderContextSessionId",345}};
    Fields d{{"SchemaVersion",1},{"Revision",3},{"HDROutput",1},{"PeakNits",410},
        {"PaperWhiteNits",167},{"UINits",203},{"ReflexMode",2}};
    Fields f{{"SchemaVersion",1},{"Revision",31},{"Mode",1},{"ContextReady",1},{"SessionId",345}};
    auto sr = save("GraphicsSettingsSave", s, 0x10);
    auto display = save("DisplaySettingsSave", d);
    auto fg = save("FGSettingsSave", f);
    GraphicsIntent migrated;
    assert(decodeLegacyGraphics(sr, display, fg, migrated));
    assert(migrated.revision == 31 && migrated.migratedFrom == (LegacyRevisions{22,3,31}));
    assert(migrated.srPreferences[0].lastCustomScaleBasisPoints == 7300);
    assert(migrated.nativeFallbackPreference && migrated.fgEnabled);
    assert(migrated.latencyMode == LatencyMode::OnBoost && migrated.hdr == (HdrPreference{true,410,167,203}));
    GraphicsRecord record;
    assert(encodeGraphicsRecord(migrated, record));
    DecodedGraphicsRecord loaded;
    assert(decodeGraphicsRecord(record, loaded) && loaded.intent == migrated);
    // Disk upgrade uses synthetic mod-owned saves only, in a new private test
    // directory. A bad subsequent upgrade must preserve both versions.
    namespace fs = std::filesystem;
    std::random_device random;
    fs::path root;
    for (unsigned attempt = 0; attempt != 10; ++attempt) {
        root = fs::temp_directory_path() / ("mcd2-legacy-"+std::to_string(random()));
        if (fs::create_directory(root)) break;
        root.clear();
    }
    assert(!root.empty());
    struct Cleanup { fs::path root; ~Cleanup() { std::error_code error; fs::remove_all(root, error); } } cleanup{root};
    for (const auto& [name, bytes] : std::array<std::pair<const char*,Bytes>,3>{{
        {"MCD2GraphicsSettings.sav",sr}, {"MCD2GraphicsDisplaySettings.sav",display}, {"MCD2GraphicsFGSettings.sav",fg}}}) {
        std::ofstream file(root / name, std::ios::binary);
        file.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
        assert(file.good());
    }
    fs::create_directory(root / "providers");
    GraphicsStore store(root / "providers");
    const auto committed = store.publish(migrated, {});
    assert(committed.status == StoreStatus::Ok);
    assert(store.load(loaded).status == StoreStatus::Ok && loaded.intent == migrated);
    auto badUpgrade = migrated; badUpgrade.schema = 6;
    assert(store.publish(badUpgrade, committed.stamp).status == StoreStatus::Invalid);
    assert(store.load(loaded).status == StoreStatus::Ok && loaded.intent == migrated);
    for (const auto& [name, bytes] : std::array<std::pair<const char*,Bytes>,3>{{
        {"MCD2GraphicsSettings.sav",sr}, {"MCD2GraphicsDisplaySettings.sav",display}, {"MCD2GraphicsFGSettings.sav",fg}}}) {
        std::ifstream file(root / name, std::ios::binary);
        const Bytes remaining{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        assert(remaining == bytes);
    }
    unsigned rejects = 0, combinations = 0;
    auto refuse = [&](std::span<const std::uint8_t> a, std::span<const std::uint8_t> b,
                      std::span<const std::uint8_t> c) {
        const auto before = migrated;
        assert(!decodeLegacyGraphics(a, b, c, migrated) && migrated == before); ++rejects;
    };
    for (std::size_t n = 0; n < sr.size(); ++n) refuse(std::span(sr).first(n), display, fg);
    for (std::size_t n = 0; n < display.size(); ++n) refuse(sr, std::span(display).first(n), fg);
    for (std::size_t n = 0; n < fg.size(); ++n) refuse(sr, display, std::span(fg).first(n));
    auto appended = sr; appended.push_back(0); refuse(appended, display, fg);
    refuse(save("GraphicsSettingsSave", s, 0x10, true), display, fg);
    auto bad = s; bad.erase("LastCustomScaleBasisPoints");
    refuse(save("GraphicsSettingsSave", bad), display, fg);
    bad = s; bad["LastCustomScaleBasisPoints"] = 7301;
    refuse(save("GraphicsSettingsSave", bad), display, fg);
    bad = s; bad["AppliedSourceRevision"] = 23;
    refuse(save("GraphicsSettingsSave", bad), display, fg);
    bad = s; bad["NewUnknownPreference"] = 1;
    refuse(save("GraphicsSettingsSave", bad), display, fg);
    bad = s; bad["NativeFallback"] = 1; // Must be a BoolProperty.
    refuse(save("GraphicsSettingsSave", bad), display, fg);
    refuse(save("GraphicsSettingsSave", s, 1), display, fg);
    refuse(save("GraphicsRuntimeStateSave", s), display, fg);
    auto wrongDisplay = d; wrongDisplay["HDROutput"] = 2;
    refuse(sr, save("DisplaySettingsSave", wrongDisplay), fg);
    auto wrongFg = f; wrongFg["ContextReady"] = 2;
    refuse(sr, display, save("FGSettingsSave", wrongFg));

    constexpr std::array<unsigned,6> displayed{6700,5800,5000,3300,7100,10000};
    constexpr std::array<unsigned,6> exact{6667,5800,5000,3333,7100,10000};
    for (unsigned mode = 0; mode <= 2; ++mode)
        for (unsigned preset = 0; preset <= 5; ++preset)
            for (unsigned fgMode = 0; fgMode <= 1; ++fgMode)
                for (unsigned reflex = 0; reflex <= 2; ++reflex) {
                    if ((mode == 1 && preset != 5) || (mode == 2 && preset == 5)) continue;
                    auto a = s, b = d, c = f;
                    a["ReconstructionMode"] = mode; a["SRPreset"] = preset;
                    a["CustomScaleBasisPoints"] = displayed[preset];
                    a["RenderScaleBasisPoints"] = mode == 2 ? exact[preset] : 10000;
                    b["ReflexMode"] = reflex; c["Mode"] = fgMode;
                    // Unreal may omit zero-valued scalar properties.
                    for (auto* fields : {&a,&b,&c})
                        for (auto it = fields->begin(); it != fields->end(); )
                            if (!it->second) it = fields->erase(it); else ++it;
                    assert(decodeLegacyGraphics(save("GraphicsSettingsSave", a, 0),
                        save("DisplaySettingsSave", b), save("FGSettingsSave", c), migrated));
                    assert(!migrated.nativeFallbackPreference);
                    assert(migrated.srPreferences[0].lastCustomScaleBasisPoints == 7300);
                    assert(migrated.fgEnabled == bool(fgMode));
                    assert(migrated.latencyMode == LatencyMode(reflex));
                    assert(encodeGraphicsRecord(migrated, record));
                    ++combinations;
                }
    assert(combinations == 72);
    std::cout << combinations << " legacy save combinations migrated; " << rejects
              << " framing/type/truncation cases rejected without changing output.\n";
}
