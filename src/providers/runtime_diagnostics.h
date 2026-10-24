#pragma once
#include <cstdint>
// Read-only snapshots. A loaded module or requested mode is never an activity
// claim. Callers sample monotonic counters and expire stale/session-mismatched data.
struct MCD2SrDiagnosticsV1 {
 std::uint32_t size,provider,revision,world,implementation,capabilities;
 std::uint64_t evaluations;
};
struct MCD2FgDiagnosticsV1 {
 std::uint32_t size,provider,active,fault,implementation,reserved;
 std::uint64_t rendered,generated;
};
// V1 remains unchanged. V2 counters belong to this exact admitted producer
// epoch, not SDK lifetime totals relabelled by a diagnostic consumer.
struct MCD2DiagnosticIdentityV1 {std::uint32_t session,revision,checksum,world;};
struct MCD2FgDiagnosticBindingV1 {
 std::uint32_t size,reserved;std::uint64_t engineFrame;
 MCD2DiagnosticIdentityV1 identity;
};
struct MCD2FgDiagnosticsV2 {
 std::uint32_t size,provider,active,fault,implementation;
 std::uint32_t session,revision,checksum,world,reserved;
 std::uint64_t observedAtMs,rendered,generated;
};
static_assert(sizeof(MCD2SrDiagnosticsV1)==32);
static_assert(sizeof(MCD2FgDiagnosticsV1)==40);
static_assert(sizeof(MCD2FgDiagnosticBindingV1)==32);
static_assert(sizeof(MCD2FgDiagnosticsV2)==64);
