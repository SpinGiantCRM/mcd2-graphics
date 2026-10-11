#pragma once
#include <windows.h>
#include <memory>
#include <string>

namespace mcd2::providers {
// The system DXGI and our application-local bootstrap can both be loaded.
// Basename lookup has unspecified identity in that case. Cache the path only,
// never a module handle: missing/unloaded bootstrap must remain unavailable.
inline HMODULE bootstrapModule() {
    static const std::wstring path = [] {
        auto buffer = std::make_unique<wchar_t[]>(32768);
        const auto length = GetModuleFileNameW(nullptr, buffer.get(), 32768);
        if (!length || length >= 32768) return std::wstring{};
        std::wstring result(buffer.get(), length);
        for (auto& c : result) if (c == L'/') c = L'\\';
        const auto separator = result.find_last_of(L'\\');
        if (separator == std::wstring::npos) return std::wstring{};
        result.resize(separator + 1);
        result += L"dxgi.dll";
        return result;
    }();
    return path.empty() ? nullptr : GetModuleHandleW(path.c_str());
}
}
