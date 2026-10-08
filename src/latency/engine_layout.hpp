#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace mcd2::engine {
// Explicit, inspected layouts; this is not an address-delta scanner or a game
// version override. Actual registry pointers are checked again before use.
struct Layout {
    std::uint32_t steamBuild, timestamp, imageSize;
    std::uint32_t simStart, simStartDispatch, simEndDispatch, pacingDispatch;
    std::uint32_t registryGet, registry, count, add, remove, nameConstructor;
    std::uint32_t simulationCounter, renderCounter;
};
inline constexpr Layout released{
    25647713, 0, 0,
    0x45546f0, 0x4554769, 0x4554679, 0x4557b8a,
    0x12a64d0, 0xba78270, 0x12a7c80, 0x12b4eb0, 0x12bb990, 0x1461040,
    0xbe45f10, 0xbe45f18};
inline constexpr Layout updated{
    25754144, 0xc7fda2fb, 0xce03000,
    0x4555980, 0x45559f9, 0x4555909, 0x4558e1a,
    0x12a6670, 0xba98270, 0x12a7e20, 0x12b5050, 0x12bbb30, 0x1461190,
    0xbe65f90, 0xbe65f98};

template<class Match>
bool guards(const Layout& l, Match match) {
    constexpr std::array<std::uint8_t,15> simulation{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x30};
    constexpr std::array<std::uint8_t,3> start{0xff,0x50,0x38}, end{0xff,0x50,0x40}, pacing{0xff,0x50,0x30};
    constexpr std::array<std::uint8_t,6> get{0x48,0x83,0xec,0x28,0x8b,0x0d};
    return match(l.simStart,std::span(simulation)) && match(l.simStartDispatch,std::span(start)) &&
        match(l.simEndDispatch,std::span(end)) && match(l.pacingDispatch,std::span(pacing)) &&
        match(l.registryGet,std::span(get));
}
template<class Match>
const Layout* select(std::uint32_t timestamp, std::uint32_t imageSize, Match match) {
    // Preserve the original five signature gates for the historical build.
    if (guards(released,match)) return &released;
    if (timestamp!=updated.timestamp || imageSize!=updated.imageSize || !guards(updated,match)) return nullptr;
    // The new build also guards the registry/name entry points before calling
    // them. PE metadata is not authentication and cannot replace these checks.
    constexpr std::array<std::uint8_t,16> count{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x18,0x48,0x89,0x54,0x24,0x10,0x57};
    constexpr std::array<std::uint8_t,16> add{0x48,0x89,0x5c,0x24,0x08,0x4c,0x89,0x44,0x24,0x18,0x48,0x89,0x54,0x24,0x10,0x57};
    constexpr std::array<std::uint8_t,16> remove{0x48,0x89,0x54,0x24,0x10,0x57,0x48,0x83,0xec,0x30,0x48,0x89,0x5c,0x24,0x40,0x48};
    constexpr std::array<std::uint8_t,16> name{0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x30,0x48,0x8b,0xd9,0x41,0x8b,0xf8};
    return match(updated.count,std::span(count)) && match(updated.add,std::span(add)) &&
        match(updated.remove,std::span(remove)) && match(updated.nameConstructor,std::span(name)) ? &updated : nullptr;
}
} // namespace mcd2::engine
