#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

namespace mcd2::providers {

// Requested settings only. These values are neither vendor SDK enums nor a
// persisted binary ABI. No capability, device, runtime token or GPU object is
// inferred from them. Wire-format/persistence and activation are later slices.
enum class SrProvider : std::uint32_t { Native, NvidiaDlss, AmdFsr, IntelXeSs };
enum class Quality : std::uint32_t { NativeAA, Quality, Balanced, Performance, UltraPerformance, Custom };
enum class FgProvider : std::uint32_t { Nvidia, Amd, Intel };
enum class FgStrategy : std::uint32_t { Single, NativeMfg, Hybrid };
enum class LatencyProvider : std::uint32_t { Automatic, NvidiaReflex, RadeonAntiLag2, IntelXeLL };
enum class LatencyMode : std::uint32_t { Off, On, OnBoost };
enum class PresentationOwner : std::uint32_t { Native, NvidiaStreamline, AmdFidelityFX, IntelXeSs };

struct SrPreference {
    Quality quality = Quality::Quality;
    std::uint32_t customScaleBasisPoints = 6700;
    std::uint32_t lastCustomScaleBasisPoints = 7700;
    bool operator==(const SrPreference&) const = default;
};

struct HdrPreference {
    bool enabled = false;
    std::uint32_t peakNits = 1000, paperWhiteNits = 203, uiNits = 203;
    bool operator==(const HdrPreference&) const = default;
};

struct LegacyRevisions {
    std::uint32_t sr = 0, display = 0, fg = 0;
    bool operator==(const LegacyRevisions&) const = default;
};

struct GraphicsIntent {
    std::uint32_t schema = 5, revision = 1;
    SrProvider sr = SrProvider::Native;
    // Indexed by non-Native SrProvider minus one. Switching provider must not
    // silently relabel the previous provider's named preset or custom value.
    std::array<SrPreference, 3> srPreferences{};
    bool nativeFallbackPreference = true;
    bool fgEnabled = false;
    FgProvider fg = FgProvider::Nvidia;
    FgStrategy fgStrategy = FgStrategy::Single;
    std::uint32_t requestedMultiplier = 2;
    LatencyProvider latency = LatencyProvider::Automatic;
    LatencyMode latencyMode = LatencyMode::On;
    HdrPreference hdr;
    LegacyRevisions migratedFrom;
    bool operator==(const GraphicsIntent&) const = default;
};

constexpr bool validRevision(std::uint32_t n) { return n && n <= 0x7fffffffu; }
constexpr bool scalarScale(std::uint32_t n) { return n >= 100 && n <= 10000 && n % 100 == 0; }
constexpr bool validHdr(const HdrPreference& h) {
    return h.peakNits >= 100 && h.peakNits <= 10000 && h.peakNits % 10 == 0 &&
           h.paperWhiteNits >= 48 && h.paperWhiteNits <= 500 && h.uiNits >= 48 && h.uiNits <= 500;
}

// Structural limits are format bounds, not SDK eligibility or a supported
// render ratio/multiplier. A separate compatibility resolver must validate the
// actual device, selected SDK, dimensions, frame inputs and pacing owner.
constexpr bool validIntent(const GraphicsIntent& i) {
    if (i.schema != 5 || !validRevision(i.revision) || unsigned(i.sr) > 3 || unsigned(i.fg) > 2 ||
        unsigned(i.fgStrategy) > 2 || unsigned(i.latency) > 3 || unsigned(i.latencyMode) > 2 ||
        !validHdr(i.hdr) || i.requestedMultiplier < 2 || i.requestedMultiplier > 16 ||
        (i.fgStrategy == FgStrategy::Single && i.requestedMultiplier != 2)) return false;
    for (const auto& p : i.srPreferences)
        if (unsigned(p.quality) > 5 || !scalarScale(p.customScaleBasisPoints) ||
            !scalarScale(p.lastCustomScaleBasisPoints)) return false;
    return true;
}

struct RenderRatio {
    std::uint32_t numerator, denominator;
    bool operator==(const RenderRatio&) const = default;
};

// Exact legacy NVIDIA ratios only. FSR/XeSS adapters must query/validate their
// own policy; this is not a universal named-preset lookup.
constexpr RenderRatio nvidiaRatio(const SrPreference& p) {
    switch (p.quality) {
        case Quality::NativeAA: return {1, 1};
        case Quality::Quality: return {2, 3};
        case Quality::Balanced: return {58, 100};
        case Quality::Performance: return {1, 2};
        case Quality::UltraPerformance: return {1, 3};
        case Quality::Custom: return {p.customScaleBasisPoints, 10000};
    }
    return {0, 0};
}

// All three legacy saves must be framing/semantically validated before a
// caller forms this value. LastCustom must be read explicitly, not inferred
// from the currently selected preset. No runtime acknowledgement is migrated.
struct LegacyV4 {
    std::uint32_t schema = 4, srRevision = 1, mode = 0, preset = 0;
    std::uint32_t renderScaleBasisPoints = 10000, customScaleBasisPoints = 6700;
    std::uint32_t lastCustomScaleBasisPoints = 7700;
    bool nativeFallback = true;
    std::uint32_t fgSchema = 1, fgRevision = 1, fgMode = 0;
    std::uint32_t displaySchema = 1, displayRevision = 1, reflexMode = 1;
    HdrPreference hdr;
};

constexpr bool migrateV4(const LegacyV4& v, GraphicsIntent& out) {
    if (v.schema != 4 || v.fgSchema != 1 || v.displaySchema != 1 ||
        !validRevision(v.srRevision) || !validRevision(v.fgRevision) || !validRevision(v.displayRevision) ||
        v.mode > 2 || v.preset > 5 || v.fgMode > 1 || v.reflexMode > 2 ||
        !scalarScale(v.lastCustomScaleBasisPoints) || !scalarScale(v.customScaleBasisPoints) || !validHdr(v.hdr) ||
        (v.mode == 1 && v.preset != 5) || (v.mode == 2 && v.preset == 5)) return false;
    constexpr std::array<std::uint32_t, 6> displayed{6700, 5800, 5000, 3300, 0, 10000};
    constexpr std::array<std::uint32_t, 6> source{6667, 5800, 5000, 3333, 0, 10000};
    if (v.preset != 4 && v.customScaleBasisPoints != displayed[v.preset]) return false;
    if (v.preset == 4)
        for (auto named : displayed) if (v.customScaleBasisPoints == named) return false;
    const auto scale = v.mode != 2 ? 10000 : (v.preset == 4 ? v.customScaleBasisPoints : source[v.preset]);
    if (v.renderScaleBasisPoints != scale) return false;

    GraphicsIntent next;
    next.revision = std::max({v.srRevision, v.fgRevision, v.displayRevision});
    next.sr = v.mode == 0 ? SrProvider::Native : SrProvider::NvidiaDlss;
    constexpr std::array<Quality, 6> quality{Quality::Quality, Quality::Balanced, Quality::Performance,
        Quality::UltraPerformance, Quality::Custom, Quality::NativeAA};
    next.srPreferences[0] = {quality[v.preset], v.customScaleBasisPoints, v.lastCustomScaleBasisPoints};
    next.nativeFallbackPreference = v.nativeFallback;
    next.fgEnabled = v.fgMode == 1;
    next.latency = LatencyProvider::NvidiaReflex;
    next.latencyMode = static_cast<LatencyMode>(v.reflexMode);
    next.hdr = v.hdr;
    next.migratedFrom = {v.srRevision, v.displayRevision, v.fgRevision};
    if (!validIntent(next)) return false;
    out = next; // Atomic in-memory publication only; no file or SDK write.
    return true;
}

// This is the requested startup route, not permission to wrap a swapchain.
// Hardware/capability checks and a matching validated startup revision must
// still approve it before the bootstrap creates any vendor owner.
constexpr PresentationOwner requestedOwner(const GraphicsIntent& i) {
    if (!validIntent(i) || !i.fgEnabled) return PresentationOwner::Native;
    switch (i.fg) {
        case FgProvider::Nvidia: return PresentationOwner::NvidiaStreamline;
        case FgProvider::Amd: return PresentationOwner::AmdFidelityFX;
        case FgProvider::Intel: return PresentationOwner::IntelXeSs;
    }
    return PresentationOwner::Native;
}

} // namespace mcd2::providers
