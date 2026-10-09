#pragma once
#include <cstdint>

// Plain-C snapshot exported by the settings worker's addon. No filesystem IO,
// SDK pointers, capability flags, or GPU/UE objects cross the addon boundary.
// The 128 bytes are the existing validated LE graphics record, not a C++ dump.
struct MCD2MenuSnapshotV1 {
    std::uint32_t size,session,load,reserved;
    std::uint8_t record[128];
};
static_assert(sizeof(MCD2MenuSnapshotV1)==144);
extern "C" int mcd2_menu_snapshot_v1(MCD2MenuSnapshotV1* snapshot);
// Startup policy only, never capability or SDK activation.
extern "C" int mcd2_menu_enabled_v1();
