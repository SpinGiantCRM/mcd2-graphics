#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cassert>
#include <iostream>
extern "C" unsigned mcd2_al2_abi();
extern "C" HRESULT mcd2_al2_init(void*);
extern "C" HRESULT mcd2_al2_update(unsigned);
extern "C" HRESULT mcd2_al2_end_rendering();
extern "C" HRESULT mcd2_al2_real_frame();
extern "C" void mcd2_al2_shutdown();
int main(){
    assert(mcd2_al2_abi()==1&&mcd2_al2_init(nullptr)==E_INVALIDARG);
    assert(mcd2_al2_init(reinterpret_cast<void*>(123))==E_HANDLE); // No driver loaded.
    const auto fake=LoadLibraryExW(L"amdxc64.dll",nullptr,LOAD_LIBRARY_SEARCH_APPLICATION_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    assert(fake);
    auto count=reinterpret_cast<unsigned(*)(unsigned)>(GetProcAddress(fake,"test_count"));
    auto fail=reinterpret_cast<void(*)(unsigned)>(GetProcAddress(fake,"test_fail_delay"));assert(count&&fail);
    assert(mcd2_al2_init(reinterpret_cast<void*>(123))==S_OK&&count(0)==1&&count(2)==1);
    assert(mcd2_al2_init(reinterpret_cast<void*>(123))==E_INVALIDARG&&count(0)==1);
    assert(mcd2_al2_update(1)==S_OK&&count(1)==1&&count(3)==1); // S_FALSE delay normalized by SDK.
    assert(mcd2_al2_update(1)==S_OK&&count(1)==1&&count(3)==2); // No repeated state change.
    assert(mcd2_al2_update(0)==S_OK&&count(2)==2&&count(3)==3);
    assert(mcd2_al2_update(2)==E_INVALIDARG&&count(3)==3); // No invented Boost mode.
    assert(mcd2_al2_end_rendering()==S_OK&&count(4)==1);
    assert(mcd2_al2_real_frame()==S_OK&&count(5)==1);
    fail(1);assert(mcd2_al2_update(0)==E_FAIL);
    mcd2_al2_shutdown();assert(count(6)==1&&mcd2_al2_update(0)==E_NOINTERFACE);
    mcd2_al2_shutdown();assert(count(6)==1);
    fail(0);assert(mcd2_al2_init(reinterpret_cast<void*>(123))==S_OK&&count(0)==2);
    mcd2_al2_shutdown();assert(count(6)==2);FreeLibrary(fake);
    std::cout<<"Official Anti-Lag SDK ABI fixture passed; synthetic driver, no hardware qualification\n";
}
