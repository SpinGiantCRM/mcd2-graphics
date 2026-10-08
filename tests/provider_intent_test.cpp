#include "../src/providers/graphics_intent.hpp"
#include <cassert>
#include <iostream>

using namespace mcd2::providers;

int main() {
    unsigned migrated = 0, rejected = 0;
    constexpr std::array<unsigned, 6> displayed{6700, 5800, 5000, 3300, 7100, 10000};
    constexpr std::array<unsigned, 6> source{6667, 5800, 5000, 3333, 7100, 10000};
    constexpr std::array<RenderRatio, 6> ratios{{{2,3}, {58,100}, {1,2}, {1,3}, {7100,10000}, {1,1}}};
    for (unsigned mode = 0; mode <= 2; ++mode)
        for (unsigned preset = 0; preset <= 5; ++preset)
            for (unsigned fg = 0; fg <= 1; ++fg)
                for (unsigned reflex = 0; reflex <= 2; ++reflex) {
                    LegacyV4 old;
                    old.srRevision = 22; old.displayRevision = 5; old.fgRevision = 31;
                    old.mode = mode; old.preset = preset; old.fgMode = fg; old.reflexMode = reflex;
                    old.customScaleBasisPoints = displayed[preset];
                    old.lastCustomScaleBasisPoints = 7300;
                    old.renderScaleBasisPoints = mode == 2 ? source[preset] : 10000;
                    old.hdr = {true, 410, 167, 203};
                    old.nativeFallback = false;
                    GraphicsIntent result;
                    const bool expected = !(mode == 1 && preset != 5) && !(mode == 2 && preset == 5);
                    assert(migrateV4(old, result) == expected);
                    if (!expected) { ++rejected; continue; }
                    ++migrated;
                    assert(validIntent(result));
                    assert(result.revision == 31);
                    assert(result.migratedFrom == (LegacyRevisions{22,5,31}));
                    assert(result.sr == (mode == 0 ? SrProvider::Native : SrProvider::NvidiaDlss));
                    assert(result.srPreferences[0].customScaleBasisPoints == displayed[preset]);
                    assert(result.srPreferences[0].lastCustomScaleBasisPoints == 7300);
                    assert(nvidiaRatio(result.srPreferences[0]) == ratios[preset]);
                    assert(result.fgEnabled == bool(fg));
                    assert(result.fg == FgProvider::Nvidia && result.fgStrategy == FgStrategy::Single);
                    assert(result.requestedMultiplier == 2);
                    assert(result.latency == LatencyProvider::NvidiaReflex);
                    assert(unsigned(result.latencyMode) == reflex);
                    assert(result.hdr == old.hdr && !result.nativeFallbackPreference);
                    assert(requestedOwner(result) == (fg ? PresentationOwner::NvidiaStreamline : PresentationOwner::Native));
                }
    assert(migrated == 72 && rejected == 36);

    auto refuses = [&](const LegacyV4& old) {
        GraphicsIntent untouched; untouched.revision = 123;
        const auto before = untouched;
        assert(!migrateV4(old, untouched));
        assert(untouched == before);
        ++rejected;
    };
    for (unsigned field = 0; field < 17; ++field) {
        LegacyV4 invalid;
        switch (field) {
            case 0: invalid.schema = 3; break;
            case 1: invalid.fgSchema = 2; break;
            case 2: invalid.displaySchema = 2; break;
            case 3: invalid.srRevision = 0; break;
            case 4: invalid.fgRevision = 0x80000000u; break;
            case 5: invalid.displayRevision = 0; break;
            case 6: invalid.mode = 3; break;
            case 7: invalid.preset = 6; break;
            case 8: invalid.fgMode = 2; break;
            case 9: invalid.reflexMode = 3; break;
            case 10: invalid.customScaleBasisPoints = 6600; break;
            case 11: invalid.lastCustomScaleBasisPoints = 0; break;
            case 12: invalid.lastCustomScaleBasisPoints = 7701; break;
            case 13: invalid.renderScaleBasisPoints = 6667; break;
            case 14: invalid.hdr.peakNits = 411; break;
            case 15: invalid.hdr.paperWhiteNits = 47; break;
            case 16: invalid.hdr.uiNits = 501; break;
        }
        refuses(invalid);
    }
    for (auto named : std::array<unsigned,5>{6700,5800,5000,3300,10000}) {
        LegacyV4 old; old.mode = 2; old.preset = 4;
        old.customScaleBasisPoints = old.renderScaleBasisPoints = named;
        refuses(old);
    }

    // Saved intent remains independent: selecting SR never selects FG.
    // This tests routing requests only, not SDK compatibility/availability.
    GraphicsIntent request;
    request.fgEnabled = true;
    for (unsigned sr = 0; sr <= 3; ++sr) {
        request.sr = static_cast<SrProvider>(sr);
        for (unsigned fg = 0; fg <= 2; ++fg) {
            request.fg = static_cast<FgProvider>(fg);
            assert(validIntent(request));
            assert(unsigned(requestedOwner(request)) == fg + 1);
        }
    }
    request.fgEnabled = false;
    assert(requestedOwner(request) == PresentationOwner::Native);
    request.fgEnabled = true;
    request.fgStrategy = FgStrategy::NativeMfg; request.requestedMultiplier = 4;
    assert(validIntent(request)); // An SDK/device resolver must still decline unsupported MFG.
    request.fgStrategy = FgStrategy::Hybrid;
    assert(validIntent(request)); // A research request does not grant presentation ownership.
    for (unsigned field = 0; field < 9; ++field) {
        auto invalid = request;
        switch (field) {
            case 0: invalid.schema = 6; break;
            case 1: invalid.revision = 0; break;
            case 2: invalid.sr = static_cast<SrProvider>(4); break;
            case 3: invalid.fg = static_cast<FgProvider>(3); break;
            case 4: invalid.fgStrategy = static_cast<FgStrategy>(3); break;
            case 5: invalid.latency = static_cast<LatencyProvider>(4); break;
            case 6: invalid.latencyMode = static_cast<LatencyMode>(3); break;
            case 7: invalid.requestedMultiplier = 17; break;
            case 8: invalid.srPreferences[2].quality = static_cast<Quality>(6); break;
        }
        assert(!validIntent(invalid));
        assert(requestedOwner(invalid) == PresentationOwner::Native);
    }
    request.fgStrategy = FgStrategy::Single;
    assert(!validIntent(request)); // Single interpolation cannot request a 4x timeline.
    std::cout << "Migrated " << migrated << " legacy combinations; rejected " << rejected
              << " malformed/mismatched combinations; request routing checks passed.\n";
}
