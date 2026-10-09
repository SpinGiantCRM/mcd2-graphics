// Own fixture only: checks the real NT termination signal without any SDK.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../src/latency/process_exit.hpp"
#ifdef MCD2_EXIT_PROBE_DLL
static wchar_t output[MAX_PATH]{};
extern "C" __declspec(dllexport) void setup(const wchar_t* path) {
    lstrcpynW(output, path, MAX_PATH);
    mcd2::process_exit::initialize();
}
BOOL WINAPI DllMain(HMODULE, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_DETACH && output[0]) {
        // Read the real query before the reserved-pointer fallback is marked.
        DWORD values[2] = {reserved ? 1u : 0u, mcd2::process_exit::terminating() ? 1u : 0u};
        auto file = CreateFileW(output, FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                               OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file != INVALID_HANDLE_VALUE) {
            DWORD written = 0;
            WriteFile(file, values, sizeof(values), &written, nullptr);
            CloseHandle(file);
        }
    }
    return TRUE;
}
#else
int wmain(int argc, wchar_t** argv) {
    if (argc != 3) return 1;
    wchar_t output[MAX_PATH]{};
    if (!GetFullPathNameW(argv[2], MAX_PATH, output, nullptr)) return 2;
    auto library = LoadLibraryW(argv[1]);
    if (!library) return 3;
    auto setup = std::bit_cast<void (*)(const wchar_t*)>(GetProcAddress(library, "setup"));
    if (!setup) return 4;
    setup(output);
    if (!FreeLibrary(library)) return 5;
    library = LoadLibraryW(argv[1]);
    if (!library) return 6;
    setup = std::bit_cast<void (*)(const wchar_t*)>(GetProcAddress(library, "setup"));
    if (!setup) return 7;
    setup(output);
    ExitProcess(0);
}
#endif
