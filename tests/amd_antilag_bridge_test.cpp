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
extern "C" unsigned mcd2_al2_fg_supported();
extern "C" HRESULT mcd2_al2_frame_generation(unsigned,unsigned);
extern "C" unsigned mcd2_al2_can_unload();
int main(){
    assert(mcd2_al2_abi()==2&&mcd2_al2_init(nullptr)==E_INVALIDARG);
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
    assert(mcd2_al2_fg_supported()==0&&mcd2_al2_frame_generation(1,1)==E_NOINTERFACE);
    auto presenter=LoadLibraryExW(L"mcd2-fsr-fg-game-bridge.dll",nullptr,LOAD_LIBRARY_SEARCH_APPLICATION_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    assert(presenter);
    auto presented=reinterpret_cast<int(*)(unsigned)>(GetProcAddress(presenter,"test_fg_present"));
    auto drainFail=reinterpret_cast<void(*)(unsigned)>(GetProcAddress(presenter,"test_fg_drain_failure"));
    auto fgCount=reinterpret_cast<unsigned(*)(unsigned)>(GetProcAddress(presenter,"test_fg_count"));assert(presented&&drainFail&&fgCount);
    assert(mcd2_al2_fg_supported()==1&&mcd2_al2_frame_generation(2,1)==E_INVALIDARG);
    assert(mcd2_al2_frame_generation(1,1)==S_OK&&fgCount(0)==1&&fgCount(2)==1);
    assert(presented(0)==S_OK&&count(5)==2&&presented(1)==S_OK&&count(7)==1);
    assert(mcd2_al2_frame_generation(1,0)==S_OK&&fgCount(2)==0);
    assert(mcd2_al2_frame_generation(0,0)==S_OK&&fgCount(1)==1&&presented(0)==E_NOINTERFACE);
    assert(mcd2_al2_frame_generation(1,1)==S_OK);drainFail(1);
    mcd2_al2_shutdown();assert(count(6)==0&&mcd2_al2_can_unload()==0);
    // A failed drain keeps the context valid even for an already queued frame.
    assert(presented(1)==S_OK&&count(7)==2);
    drainFail(0);mcd2_al2_shutdown();assert(count(6)==1&&mcd2_al2_can_unload()==1&&presented(0)==E_NOINTERFACE);
    assert(mcd2_al2_init(reinterpret_cast<void*>(123))==S_OK);
    fail(1);assert(mcd2_al2_update(0)==E_FAIL);
    mcd2_al2_shutdown();assert(count(6)==2&&mcd2_al2_update(0)==E_NOINTERFACE);
    mcd2_al2_shutdown();assert(count(6)==2);
    fail(0);assert(mcd2_al2_init(reinterpret_cast<void*>(123))==S_OK&&count(0)==3);
    mcd2_al2_shutdown();assert(count(6)==3);FreeLibrary(presenter);FreeLibrary(fake);
    std::cout<<"Official Anti-Lag SDK ABI fixture passed; synthetic driver, no hardware qualification\n";
}
