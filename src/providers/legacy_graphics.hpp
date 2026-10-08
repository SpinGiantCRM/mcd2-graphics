#pragma once
#include "graphics_intent.hpp"
#include <map>
#include <span>
#include <string>
#include <string_view>

namespace mcd2::providers {
namespace legacy_detail {
// Strict reader for the three mod-owned legacy slots only. It never discovers
// or opens files and does not change the decoder used by the shipping runtime.
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at = 0;
    bool ok = true;
    std::uint32_t number(unsigned width) {
        if (at > bytes.size() || width > bytes.size()-at) { ok = false; return 0; }
        std::uint32_t value = 0;
        for (unsigned n = 0; n < width; ++n) value |= std::uint32_t(bytes[at++]) << (n*8);
        return value;
    }
    std::string string() {
        const auto length = number(4);
        if (!ok || !length || length > 256 || at > bytes.size() || length > bytes.size()-at ||
            bytes[at+length-1] != 0) { ok = false; return {}; }
        std::string value(reinterpret_cast<const char*>(bytes.data()+at), length-1);
        at += length;
        if (value.find('\0') != std::string::npos) ok = false;
        return value;
    }
};
struct Scalar { std::uint32_t value = 0; bool boolean = false; };
using Fields = std::map<std::string, Scalar, std::less<>>;
inline bool parse(std::span<const std::uint8_t> bytes, std::string_view className,
                  std::span<const std::string_view> integerNames, Fields& out,
                  bool allowFallback = false) {
    if (bytes.size() < 64 || bytes.size() > 8192) return false;
    Reader r{bytes};
    if (r.number(4) != 0x53415647 || r.number(4) != 3 || r.number(4) != 522 || r.number(4) != 1017 ||
        r.number(2) != 5 || r.number(2) != 6 || r.number(2) != 1 || r.number(4) != 0 ||
        r.string() != "UE5" || r.number(4) != 3 || r.number(4) != 93 || !r.ok ||
        r.at > bytes.size() || 93*20 > bytes.size()-r.at) return false;
    r.at += 93*20;
    const auto expected = std::string("/Game/Mods/MCD2Graphics/")+std::string(className)+"."+
                          std::string(className)+"_C";
    if (r.string() != expected || r.number(1) != 0 || !r.ok) return false;
    Fields fields;
    for (unsigned n = 0; n < 32; ++n) {
        const auto name = r.string();
        if (!r.ok) return false;
        if (name == "None") {
            if (r.number(4) != 0 || !r.ok || r.at != bytes.size()) return false;
            out = std::move(fields); return true;
        }
        if (fields.contains(name)) return false;
        const auto type = r.string();
        const auto array = r.number(4), size = r.number(4), flags = r.number(1);
        if (!r.ok || array) return false;
        if (allowFallback && name == "NativeFallback") {
            if (type != "BoolProperty" || size || (flags != 0 && flags != 0x10)) return false;
            fields.emplace(name, Scalar{flags == 0x10 ? 1u : 0u, true});
        } else {
            if (std::find(integerNames.begin(), integerNames.end(), name) == integerNames.end() ||
                type != "IntProperty" || size != 4 || flags) return false;
            const auto value = r.number(4);
            if (!r.ok || value > 0x7fffffffu) return false;
            fields.emplace(name, Scalar{value, false});
        }
    }
    return false;
}
inline std::uint32_t value(const Fields& fields, std::string_view key) {
    const auto found = fields.find(key);
    return found == fields.end() ? 0 : found->second.value;
}
} // namespace legacy_detail

// The caller must provide a stable snapshot of these three mod-owned saves.
// A future live reader must recheck all bytes before commit; separate legacy
// revisions alone do not prove that three filesystem reads were simultaneous.
// Missing LastCustom is refused rather than guessing and losing a preference.
// Context/session/source acknowledgements are validated then not migrated.
inline bool decodeLegacyGraphics(std::span<const std::uint8_t> sr,
                                 std::span<const std::uint8_t> display,
                                 std::span<const std::uint8_t> fg, GraphicsIntent& out) {
    using namespace legacy_detail;
    constexpr std::array<std::string_view, 11> srNames{
        "SchemaVersion", "Revision", "ReconstructionMode", "RenderScaleBasisPoints", "SRPreset",
        "CustomScaleBasisPoints", "LastCustomScaleBasisPoints", "AppliedSourceRevision",
        "AppliedSourceSessionId", "RenderContextReady", "RenderContextSessionId"};
    // The arrays intentionally describe only the shipped properties. No future
    // scalar is silently promoted into a persisted preference.
    constexpr std::array<std::string_view, 7> displayNames{
        "SchemaVersion", "Revision", "HDROutput", "PeakNits", "PaperWhiteNits", "UINits", "ReflexMode"};
    constexpr std::array<std::string_view, 5> fgNames{
        "SchemaVersion", "Revision", "Mode", "ContextReady", "SessionId"};
    Fields s, d, f;
    if (!parse(sr, "GraphicsSettingsSave", srNames, s, true) ||
        !parse(display, "DisplaySettingsSave", displayNames, d) ||
        !parse(fg, "FGSettingsSave", fgNames, f) || !s.contains("LastCustomScaleBasisPoints")) return false;
    LegacyV4 old;
    old.schema = value(s, "SchemaVersion"); old.srRevision = value(s, "Revision");
    old.mode = value(s, "ReconstructionMode"); old.preset = value(s, "SRPreset");
    old.renderScaleBasisPoints = value(s, "RenderScaleBasisPoints");
    old.customScaleBasisPoints = value(s, "CustomScaleBasisPoints");
    old.lastCustomScaleBasisPoints = value(s, "LastCustomScaleBasisPoints");
    old.nativeFallback = value(s, "NativeFallback") == 1;
    old.fgSchema = value(f, "SchemaVersion"); old.fgRevision = value(f, "Revision");
    old.fgMode = value(f, "Mode");
    old.displaySchema = value(d, "SchemaVersion"); old.displayRevision = value(d, "Revision");
    old.reflexMode = value(d, "ReflexMode");
    if (value(d, "HDROutput") > 1 || value(f, "ContextReady") > 1 ||
        value(s, "RenderContextReady") > 1 || value(s, "AppliedSourceRevision") > old.srRevision) return false;
    old.hdr = {value(d, "HDROutput") == 1, value(d, "PeakNits"), value(d, "PaperWhiteNits"), value(d, "UINits")};
    return migrateV4(old, out);
}
} // namespace mcd2::providers
