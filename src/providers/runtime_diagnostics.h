#pragma once
#include <cstdint>
// Read-only snapshots. A loaded module or requested mode is never an activity
// claim. Callers sample monotonic counters and expire stale/session-mismatched data.
struct MCD2SrDiagnosticsV1 {
 std::uint32_t size,provider,revision,world,implementation,requestedPreset;
 std::uint64_t evaluations;
};
struct MCD2FgDiagnosticsV1 {
 std::uint32_t size,provider,active,fault,implementation,reserved;
 std::uint64_t rendered,generated;
};
static_assert(sizeof(MCD2SrDiagnosticsV1)==32);
static_assert(sizeof(MCD2FgDiagnosticsV1)==40);
