#pragma once
#include <cstdint>
namespace mcd2ui {
// NVIDIA hardware is only a candidate; NGX still checks actual DLSS support.
constexpr bool dlss_adapter_candidate(uint32_t vendor){return vendor==0x10de;}
constexpr bool source_wait_expired(uint64_t elapsed){return elapsed>15000;}
}
