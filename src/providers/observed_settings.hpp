#pragma once
#include "graphics_store.hpp"
#include "legacy_graphics.hpp"
#include <vector>

namespace mcd2::providers {

// Experimental observation bridge. The legacy UI remains authoritative in
// this phase; this is NOT the final one-writer migration or a vendor router.
using SlotBytes = std::vector<std::uint8_t>;
struct LegacySnapshot {
    SlotBytes sr, display, fg;
    bool operator==(const LegacySnapshot&) const = default;
};

inline bool readObservedSlot(const std::filesystem::path& path, SlotBytes& out) {
    using namespace store_detail;
#ifdef _WIN32
    File file(CreateFileW(path.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    LARGE_INTEGER length{};
    if (file.handle == badHandle || !regularHandle(file.handle, false) ||
        !GetFileSizeEx(file.handle, &length) || length.QuadPart < 64 || length.QuadPart > 8192) return false;
    SlotBytes bytes(std::size_t(length.QuadPart));
    DWORD read = 0;
    if (!ReadFile(file.handle, bytes.data(), DWORD(bytes.size()), &read, nullptr) || read != bytes.size())
        return false;
#else
    File file(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    struct stat info{};
    if (file.handle == badHandle || !regularHandle(file.handle, false) || fstat(file.handle, &info) ||
        info.st_size < 64 || info.st_size > 8192) return false;
    SlotBytes bytes(std::size_t(info.st_size));
    std::size_t at = 0;
    while (at != bytes.size()) {
        const auto read = ::read(file.handle, bytes.data()+at, bytes.size()-at);
        if (read < 0 && errno == EINTR) continue;
        if (read <= 0) return false;
        at += std::size_t(read);
    }
#endif
    out = std::move(bytes);
    return true;
}

inline bool readLegacySnapshot(const std::filesystem::path& saves, LegacySnapshot& out) {
    if (!store_detail::directoryReady(saves)) return false;
    auto read = [&](LegacySnapshot& s) {
        return readObservedSlot(saves / "MCD2GraphicsSettings.sav", s.sr) &&
               readObservedSlot(saves / "MCD2GraphicsDisplaySettings.sav", s.display) &&
               readObservedSlot(saves / "MCD2GraphicsFGSettings.sav", s.fg);
    };
    LegacySnapshot first, second;
    if (!read(first) || !read(second) || first != second) return false;
    out = std::move(first);
    return true;
}

inline bool decodeObserved(const LegacySnapshot& bytes, GraphicsIntent& out) {
    return decodeLegacyGraphics(bytes.sr, bytes.display, bytes.fg, out);
}
// Runtime context/source acknowledgements and their revision increments must
// not trigger a settings transaction or invalidate the startup owner.
inline GraphicsIntent preferencesOnly(GraphicsIntent intent) {
    intent.revision = 1;
    intent.migratedFrom = {};
    return intent;
}
inline bool sameObservedPreferences(const GraphicsIntent& a, const GraphicsIntent& b) {
    return preferencesOnly(a) == preferencesOnly(b);
}
inline bool legacyRepresentable(const GraphicsIntent& intent) {
    return validIntent(intent) && unsigned(intent.sr) <= unsigned(SrProvider::NvidiaDlss) &&
        intent.fg == FgProvider::Nvidia && intent.fgStrategy == FgStrategy::Single &&
        intent.requestedMultiplier == 2 && intent.latency == LatencyProvider::NvidiaReflex &&
        intent.srPreferences[1] == SrPreference{} && intent.srPreferences[2] == SrPreference{};
}

enum class ObservationStatus { Waiting, Unchanged, Published, Rejected, StoreFailure };
struct ObservationResult {
    ObservationStatus status = ObservationStatus::Waiting;
    StoreStatus store = StoreStatus::Ok;
    RecordStamp stamp;
};
class ObservedSettings {
    LegacySnapshot previous_;
    bool seen_ = false;
public:
    // Call only on the existing settings worker, never on render/present.
    // Two byte-identical polls debounce independently saved UI slots. This is
    // not proof of an atomic multi-slot UI transaction; authoritative cutover
    // requires a single consolidated UI request and separate qualification.
    ObservationResult observe(const LegacySnapshot& snapshot, const GraphicsStore& store) {
        GraphicsIntent incoming;
        if (!decodeObserved(snapshot, incoming)) { seen_ = false; return {ObservationStatus::Rejected, StoreStatus::Invalid, {}}; }
        if (!seen_ || previous_ != snapshot) {
            previous_ = snapshot; seen_ = true; return {};
        }
        DecodedGraphicsRecord current;
        const auto loaded = store.load(current);
        if (loaded.status != StoreStatus::Ok && loaded.status != StoreStatus::Missing)
            return {ObservationStatus::StoreFailure, loaded.status, {}};
        if (loaded.status == StoreStatus::Ok) {
            // Never downgrade a future provider request through an old UI.
            if (!legacyRepresentable(current.intent)) return {ObservationStatus::Rejected, StoreStatus::Invalid, loaded.stamp};
            if (sameObservedPreferences(current.intent, incoming))
                return {ObservationStatus::Unchanged, StoreStatus::Ok, loaded.stamp};
            if (current.intent.revision == 0x7fffffffu) return {ObservationStatus::Rejected, StoreStatus::Invalid, loaded.stamp};
            incoming.revision = std::max(incoming.revision, current.intent.revision+1);
            incoming.migratedFrom = current.intent.migratedFrom; // Original provenance, not latest ACKs.
        }
        const auto committed = store.publish(incoming, loaded.stamp);
        if (committed.status != StoreStatus::Ok)
            return {ObservationStatus::StoreFailure, committed.status, committed.stamp};
        return {ObservationStatus::Published, StoreStatus::Ok, committed.stamp};
    }
    void invalidate() { seen_ = false; }
};

// Read once before the first game factory. The mirror is valid only if it
// still agrees with a freshly validated UI snapshot. Missing, corrupt, stale
// or future-provider records fail closed to Native FG ownership; they do not
// fall back to a separate FG slot. Capability checks still belong to DXGI.
inline StartupSelection observedStartup(const GraphicsStore& store, const LegacySnapshot& snapshot,
                                        std::uint64_t session) {
    GraphicsIntent ui;
    DecodedGraphicsRecord record;
    if (!session || !decodeObserved(snapshot, ui) || store.load(record).status != StoreStatus::Ok ||
        !legacyRepresentable(record.intent) || !sameObservedPreferences(record.intent, ui)) return {};
    return {session, record.bootstrap, true};
}

} // namespace mcd2::providers
