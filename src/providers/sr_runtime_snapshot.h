#pragma once
#include "menu_snapshot.h"
struct MCD2SrContextSnapshotV1 {
    std::uint32_t size,reserved;
    std::uint64_t observedAtMs;
    MCD2MenuSnapshotV1 authority;
    std::uint8_t context[128];
};
struct MCD2SrRuntimeSnapshotV1 { std::uint32_t size,reserved;std::uint8_t state[128]; };
static_assert(sizeof(MCD2SrContextSnapshotV1)==288);
static_assert(sizeof(MCD2SrRuntimeSnapshotV1)==136);
extern "C" int mcd2_sr_context_v1(MCD2SrContextSnapshotV1*);
extern "C" int mcd2_sr_runtime_v1(MCD2SrRuntimeSnapshotV1*);
