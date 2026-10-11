#include <windows.h>
#ifndef FIXTURE_ID
#define FIXTURE_ID 1
#endif
extern "C" __declspec(dllexport) int fixture_identity() { return FIXTURE_ID; }
BOOL WINAPI DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }
