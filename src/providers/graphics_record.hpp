#pragma once
#include "graphics_intent.hpp"
#include <span>

namespace mcd2::providers {

// An explicit LE wire format, not a dump of a compiler-dependent C++ struct.
// The one committed record supplies both menu intent and the early startup
// route; publishing two separate files would allow a torn provider selection.
using GraphicsRecord = std::array<std::uint8_t, 128>;
inline constexpr std::array<std::uint8_t, 8> recordMagic{'M','C','D','2','G','I','5',0};
inline constexpr unsigned recordVersion = 1;
inline constexpr std::size_t checksumOffset = 16;

struct RecordStamp {
    std::uint32_t revision = 0, checksum = 0;
    bool operator==(const RecordStamp&) const = default;
};
struct BootstrapGraphicsIntent {
    RecordStamp stamp;
    PresentationOwner requested = PresentationOwner::Native;
    bool fgEnabled = false;
    bool operator==(const BootstrapGraphicsIntent&) const = default;
};
struct DecodedGraphicsRecord {
    GraphicsIntent intent;
    BootstrapGraphicsIntent bootstrap;
    bool operator==(const DecodedGraphicsRecord&) const = default;
};

inline std::uint32_t recordWord(std::span<const std::uint8_t> bytes, std::size_t at) {
    return std::uint32_t(bytes[at]) | (std::uint32_t(bytes[at+1]) << 8) |
           (std::uint32_t(bytes[at+2]) << 16) | (std::uint32_t(bytes[at+3]) << 24);
}
inline void setRecordWord(GraphicsRecord& bytes, std::size_t at, std::uint32_t value) {
    for (unsigned n = 0; n != 4; ++n) bytes[at+n] = std::uint8_t(value >> (n*8));
}
// CRC-32/ISO-HDLC, checksum word treated as zero. Accidental corruption
// detection only: a local user can edit and recalculate it. This is not a
// signature, a trust boundary, a GPU capability or authorization to activate FG.
inline std::uint32_t recordChecksum(std::span<const std::uint8_t> bytes) {
    std::uint32_t crc = 0xffffffffu;
    for (std::size_t at = 0; at != bytes.size(); ++at) {
        crc ^= at >= checksumOffset && at < checksumOffset+4 ? 0 : bytes[at];
        for (unsigned bit = 0; bit != 8; ++bit)
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}
inline bool validMigrationStamp(const GraphicsIntent& i) {
    const auto& m = i.migratedFrom;
    if (!m.sr && !m.display && !m.fg) return true;
    return validRevision(m.sr) && validRevision(m.display) && validRevision(m.fg) &&
           std::max({m.sr, m.display, m.fg}) <= i.revision;
}

inline bool encodeGraphicsRecord(const GraphicsIntent& i, GraphicsRecord& out) {
    if (!validIntent(i) || !validMigrationStamp(i)) return false;
    GraphicsRecord next{};
    std::copy(recordMagic.begin(), recordMagic.end(), next.begin());
    setRecordWord(next, 8, recordVersion);
    setRecordWord(next, 12, unsigned(next.size()));
    setRecordWord(next, 20, unsigned(requestedOwner(i)));
    std::size_t at = 24;
    auto put = [&](std::uint32_t v) { setRecordWord(next, at, v); at += 4; };
    put(i.schema); put(i.revision); put(unsigned(i.sr));
    for (const auto& p : i.srPreferences) {
        put(unsigned(p.quality)); put(p.customScaleBasisPoints); put(p.lastCustomScaleBasisPoints);
    }
    put(i.nativeFallbackPreference); put(i.fgEnabled); put(unsigned(i.fg));
    put(unsigned(i.fgStrategy)); put(i.requestedMultiplier);
    put(unsigned(i.latency)); put(unsigned(i.latencyMode));
    put(i.hdr.enabled); put(i.hdr.peakNits); put(i.hdr.paperWhiteNits); put(i.hdr.uiNits);
    put(i.migratedFrom.sr); put(i.migratedFrom.display); put(i.migratedFrom.fg);
    if (at != next.size()) return false;
    setRecordWord(next, checksumOffset, recordChecksum(next));
    out = next;
    return true;
}

inline bool decodeGraphicsRecord(std::span<const std::uint8_t> bytes, DecodedGraphicsRecord& out) {
    if (bytes.size() != GraphicsRecord{}.size() ||
        !std::equal(recordMagic.begin(), recordMagic.end(), bytes.begin()) ||
        recordWord(bytes, 8) != recordVersion || recordWord(bytes, 12) != bytes.size() ||
        recordWord(bytes, checksumOffset) != recordChecksum(bytes)) return false;
    DecodedGraphicsRecord next;
    auto& i = next.intent;
    std::size_t at = 24;
    auto get = [&]() { const auto value = recordWord(bytes, at); at += 4; return value; };
    bool validBools = true;
    auto boolean = [&]() { const auto value = get(); validBools &= value <= 1; return value == 1; };
    i.schema = get(); i.revision = get(); i.sr = SrProvider(get());
    for (auto& p : i.srPreferences) {
        p.quality = Quality(get()); p.customScaleBasisPoints = get(); p.lastCustomScaleBasisPoints = get();
    }
    i.nativeFallbackPreference = boolean(); i.fgEnabled = boolean(); i.fg = FgProvider(get());
    i.fgStrategy = FgStrategy(get()); i.requestedMultiplier = get();
    i.latency = LatencyProvider(get()); i.latencyMode = LatencyMode(get());
    i.hdr.enabled = boolean(); i.hdr.peakNits = get(); i.hdr.paperWhiteNits = get(); i.hdr.uiNits = get();
    i.migratedFrom.sr = get(); i.migratedFrom.display = get(); i.migratedFrom.fg = get();
    if (!validBools || !validIntent(i) || !validMigrationStamp(i) || at != bytes.size() ||
        recordWord(bytes, 20) != unsigned(requestedOwner(i))) return false;
    next.bootstrap = {{i.revision, recordWord(bytes, checksumOffset)}, requestedOwner(i), i.fgEnabled};
    out = next;
    return true;
}

// Session identity is generated by the host and stays out of persistent intent.
// Failure yields a Native request. Success still does not approve a vendor SDK,
// rendering adapter or swapchain: that belongs to the later capability router.
struct StartupSelection {
    std::uint64_t session = 0;
    BootstrapGraphicsIntent bootstrap;
    bool valid = false;
};
inline StartupSelection selectStartup(std::span<const std::uint8_t> bytes, std::uint64_t session) {
    DecodedGraphicsRecord record;
    if (!session || !decodeGraphicsRecord(bytes, record)) return {};
    return {session, record.bootstrap, true};
}
inline bool matchesStartup(const StartupSelection& startup, std::span<const std::uint8_t> menuRecord,
                           std::uint64_t session) {
    DecodedGraphicsRecord record;
    return startup.valid && session && session == startup.session &&
           decodeGraphicsRecord(menuRecord, record) && record.bootstrap == startup.bootstrap;
}

} // namespace mcd2::providers
