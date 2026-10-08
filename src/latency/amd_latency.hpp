#pragma once
#include "../providers/graphics_record.hpp"
#include <cstdint>
#include <mutex>

namespace mcd2::amd_latency {

// SDK calls stay behind the Microsoft ABI bridge. No vendor interfaces are
// copied into the MinGW addon and no GPU or driver capability is inferred
// from a saved request.
struct Provider {
    virtual ~Provider() = default;
    virtual bool initialize(std::uintptr_t device) = 0;
    virtual bool update(bool enabled) = 0;
    virtual bool end_rendering() = 0;
    virtual bool real_frame() = 0;
    virtual void shutdown() = 0;
};

struct Eligibility {
    bool optedIn = false, nativeWindows = false, driverLoaded = false;
    std::uint32_t renderingVendor = 0;
    bool candidate() const {
        return optedIn && nativeWindows && driverLoaded && renderingVendor == 0x1002;
    }
};

struct Request {
    bool valid = false, enabled = false;
    std::uint32_t revision = 0, checksum = 0;
};

// This first integration supports independent, non-FG Anti-Lag only. Boost
// is not an Anti-Lag mode; FG needs the owned FSR swapchain handshake first.
inline Request resolve(const providers::DecodedGraphicsRecord& record) {
    const auto& i = record.intent;
    providers::GraphicsRecord bytes;
    providers::DecodedGraphicsRecord canonical;
    if (!providers::encodeGraphicsRecord(i, bytes) ||
        !providers::decodeGraphicsRecord(bytes, canonical) || canonical != record) return {};
    const bool selected = i.latency == providers::LatencyProvider::Automatic ||
                          i.latency == providers::LatencyProvider::RadeonAntiLag2;
    return {true, selected && i.latencyMode == providers::LatencyMode::On && !i.fgEnabled,
            i.revision, record.bootstrap.stamp.checksum};
}

struct State {
    bool available = false, enabled = false, fault = false;
    std::uint64_t inputFrames = 0, renderedFrames = 0;
    std::uint32_t revision = 0, checksum = 0;
};

// Serializes SDK access and teardown against callbacks on both game and
// render threads. Holding a shared Controller keeps it alive; after shutdown
// every remaining callback is inert. No SDK-owned device pointer escapes.
class Controller {
    Provider& provider_;
    mutable std::mutex mutex_;
    State state_;
    bool closed_ = false;
    std::uint64_t lastInput_ = 0, lastPresent_ = 0;
    void fault() {
        state_.fault = true;
        state_.enabled = false;
        state_.available = false;
        provider_.shutdown();
    }
public:
    explicit Controller(Provider& provider) : provider_(provider) {}
    ~Controller() { shutdown(); }
    bool attach(const Eligibility& eligibility, std::uintptr_t device) {
        std::lock_guard lock(mutex_);
        if (closed_ || state_.available || state_.fault || !device || !eligibility.candidate()) return false;
        if (!provider_.initialize(device)) { provider_.shutdown(); return false; }
        state_.available = true;
        return true;
    }
    void pre_input(std::uint64_t frame, Request request) {
        std::lock_guard lock(mutex_);
        if (!state_.available || !frame) return;
        if (frame == lastInput_) return; // A repeated pacing query is not another simulation.
        if (frame < lastInput_) { fault(); return; }
        const bool enabled = request.valid && request.enabled;
        if (!provider_.update(enabled)) { fault(); return; }
        lastInput_ = frame;
        ++state_.inputFrames;
        state_.enabled = enabled;
        state_.revision = request.valid ? request.revision : 0;
        state_.checksum = request.valid ? request.checksum : 0;
    }
    void pre_present(std::uint64_t frame) {
        std::lock_guard lock(mutex_);
        if (!state_.available || !state_.enabled || !lastInput_ || !frame || frame == lastPresent_) return;
        // Rendered frames may lag simulation. Future/unordered identities are
        // not fabricated as SDK frame indices or generated frames.
        if (frame > lastInput_ || frame < lastPresent_) { fault(); return; }
        if (!provider_.end_rendering() || !provider_.real_frame()) { fault(); return; }
        lastPresent_ = frame;
        ++state_.renderedFrames;
    }
    State state() const { std::lock_guard lock(mutex_); return state_; }
    void shutdown() {
        std::lock_guard lock(mutex_);
        if (state_.available) provider_.shutdown();
        closed_ = true;
        state_.available = false;
        state_.enabled = false;
    }
};
}
