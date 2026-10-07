#pragma once
#include <string>
#include <string_view>

namespace mcd2::compat {
// VKD3D accepts comma/semicolon-separated extension names. Keep every existing
// choice and append exact names once; do not replace a user's launch overrides.
inline std::wstring reflex_disabled_extensions(std::wstring_view existing) {
    std::wstring result(existing);
    for (auto name : {L"VK_KHR_swapchain_maintenance1", L"VK_EXT_swapchain_maintenance1"}) {
        bool found = false;
        for (std::size_t begin = 0; begin < result.size();) {
            const auto end = result.find_first_of(L",;", begin);
            const auto length = end == std::wstring::npos ? result.size() - begin : end - begin;
            if (std::wstring_view(result).substr(begin, length) == name) found = true;
            if (end == std::wstring::npos) break;
            begin = end + 1;
        }
        if (!found) {
            if (!result.empty() && result.back() != L',' && result.back() != L';') result += L',';
            result += name;
        }
    }
    return result;
}
}
