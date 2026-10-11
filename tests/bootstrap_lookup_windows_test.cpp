#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../src/providers/bootstrap_module.hpp"
#include <cassert>
#include <bit>
#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
    auto buffer = std::make_unique<wchar_t[]>(32768);
    auto length = GetModuleFileNameW(nullptr, buffer.get(), 32768);
    assert(length && length < 32768);
    const auto folder = std::filesystem::path(buffer.get()).parent_path();
    const auto otherPath = folder / L"other" / L"dxgi.dll";
    const auto ownPath = folder / L"dxgi.dll";
    assert(!mcd2::providers::bootstrapModule());
    HMODULE own = nullptr, other = nullptr;
    if (argc > 1 && std::string(argv[1]) == "own-first") {
        own = LoadLibraryW(ownPath.c_str());
        other = LoadLibraryW(otherPath.c_str());
    } else {
        other = LoadLibraryW(otherPath.c_str());
        assert(other);
        assert(!mcd2::providers::bootstrapModule());
        own = LoadLibraryW(ownPath.c_str());
    }
    assert(own && other && own != other);
    assert(mcd2::providers::bootstrapModule() == own);
    using Identity = int(*)();
    auto id = std::bit_cast<Identity>(GetProcAddress(mcd2::providers::bootstrapModule(), "fixture_identity"));
    assert(id && id() == 1);
    auto otherId = std::bit_cast<Identity>(GetProcAddress(other, "fixture_identity"));
    assert(otherId && otherId() == 2);
    // CWD cannot select another DLL, and the helper does not retain stale handles.
    auto previous = std::filesystem::current_path();
    std::filesystem::current_path(folder / L"other");
    assert(mcd2::providers::bootstrapModule() == own);
    std::filesystem::current_path(previous);
    assert(FreeLibrary(own));
    assert(!mcd2::providers::bootstrapModule());
    assert(FreeLibrary(other));
    std::cout << "Exact bootstrap identity passed with duplicate DLL basenames\n";
}
