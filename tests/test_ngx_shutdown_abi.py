"""Compile actual NGX cleanup against the direct-driver shutdown contract.

Synthetic objects only: no GPU, game, driver DLL or user files are accessed.
"""
from pathlib import Path
import argparse
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='g++')
    parser.add_argument('--source-root', type=Path, default=ROOT)
    args = parser.parse_args()
    source = (args.source_root / 'src/native/ngx_live_fixture.hpp').read_text()
    body = source.split('static bool cleanup_detached_live(LiveFixture &c) {', 1)[1].split(
        '\nstatic void cleanup_live_saved(', 1)[0]
    needle = 'auto fn=reinterpret_cast<F>(GetProcAddress(c.module,"NVSDK_NGX_D3D12_Shutdown1"));'
    assert body.count(needle) == 1
    # Type-check the real local alias before invoking a typed mock. An old
    # one-argument alias is rejected at compile time instead of invoking UB.
    body = body.replace(needle, 'static_assert(std::is_same_v<F,DriverShutdown>);' + needle)
    harness = r'''
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <type_traits>
#include <vector>
using NVSDK_NGX_Result=unsigned;
using HMODULE=void*;
#define NVSDK_NGX_SUCCEED(value) ((value)==1u)
struct Object {unsigned releases=0;void Release(){++releases;}};
struct ID3D12Device:Object {};
struct NVSDK_NGX_Handle {};
struct NVSDK_NGX_Parameter {};
using DriverShutdown=NVSDK_NGX_Result(*)(ID3D12Device*,unsigned&);
struct LiveFixture {
 HMODULE module=nullptr;bool initialized=false;
 ID3D12Device* proxy_device=nullptr;
 NVSDK_NGX_Handle* feature=nullptr;NVSDK_NGX_Parameter* params=nullptr;
 struct {std::vector<Object*> resources;} owned;
 std::ostringstream log;
};
unsigned checks=0,shutdownCalls=0,releaseCalls=0,destroyCalls=0,freeCalls=0;
unsigned shutdownResult=1,releaseResult=1,destroyResult=1;
bool bridge=false,shared=false,missingShutdown=false;
int owner=1;
ID3D12Device* expectedDevice=nullptr;
Object resource;ID3D12Device device,otherDevice;
std::vector<unsigned> events;
void check(bool ok,const char* message){++checks;if(!ok){std::cerr<<message<<'\n';std::exit(2);}}
int initialized(){return shared?1:0;}
int ownership(void* value){check(value==expectedDevice,"Shared ownership must use the initialization device");return owner;}
unsigned release(NVSDK_NGX_Handle*){++releaseCalls;events.push_back(1);return releaseResult;}
unsigned destroy(NVSDK_NGX_Parameter*){++destroyCalls;events.push_back(2);return destroyResult;}
unsigned shutdown(ID3D12Device* value,unsigned& remaining){
 ++shutdownCalls;events.push_back(3);
 check(value&&value==expectedDevice,"Direct shutdown must use the non-null initialization device");
 check(resource.releases==0&&device.releases==0,"GPU-owned references must survive driver shutdown");
 remaining=7;return shutdownResult;
}
void* GetModuleHandleW(const wchar_t*){return bridge?reinterpret_cast<void*>(1):nullptr;}
void* GetProcAddress(void*,const char* name){
 if(!std::strcmp(name,"NVSDK_NGX_D3D12_ReleaseFeature"))return reinterpret_cast<void*>(&release);
 if(!std::strcmp(name,"NVSDK_NGX_D3D12_DestroyParameters"))return reinterpret_cast<void*>(&destroy);
 if(!std::strcmp(name,"NVSDK_NGX_D3D12_Shutdown1"))return missingShutdown?nullptr:reinterpret_cast<void*>(&shutdown);
 if(!std::strcmp(name,"mcd2_fg_initialized"))return reinterpret_cast<void*>(&initialized);
 if(!std::strcmp(name,"mcd2_fg_ngx_owner_v1"))return reinterpret_cast<void*>(&ownership);
 return nullptr;
}
bool FreeLibrary(void*){
 ++freeCalls;check(resource.releases==1&&device.releases==1,"Module must outlive owned references");return true;
}
static bool cleanup_detached_live(LiveFixture &c) {__BODY__
void setup(LiveFixture& c){
 shutdownCalls=releaseCalls=destroyCalls=freeCalls=0;
 shutdownResult=releaseResult=destroyResult=1;
 bridge=shared=missingShutdown=false;owner=1;events.clear();
 resource.releases=device.releases=otherDevice.releases=0;expectedDevice=&device;
 c.module=reinterpret_cast<void*>(2);c.initialized=true;c.proxy_device=&device;
 c.feature=reinterpret_cast<NVSDK_NGX_Handle*>(3);c.params=reinterpret_cast<NVSDK_NGX_Parameter*>(4);
 c.owned.resources={&device,&resource};
}
int main(){
 {LiveFixture c;setup(c);check(cleanup_detached_live(c),"Unshared cleanup must succeed");
  check(shutdownCalls==1&&events==std::vector<unsigned>{1,2,3},"Feature and parameters must retire before shutdown");
  check(!c.initialized&&!c.module&&c.owned.resources.empty(),"Successful cleanup must clear ownership");
  check(cleanup_detached_live(c)&&shutdownCalls==1&&freeCalls==1,"Repeated cleanup must be harmless");}
 {LiveFixture c;setup(c);bridge=shared=true;check(cleanup_detached_live(c),"Verified shared owner must succeed");
  check(shutdownCalls==0&&freeCalls==1,"Streamline must own shared NGX shutdown");}
 {LiveFixture c;setup(c);bridge=shared=true;owner=-1;
  check(!cleanup_detached_live(c),"Unknown shared owner must retain");
  check(c.initialized&&resource.releases==0&&freeCalls==0&&shutdownCalls==0,"Unverified ownership must not release resources");}
 {LiveFixture c;setup(c);missingShutdown=true;check(!cleanup_detached_live(c),"Missing shutdown export must retain");
  check(c.initialized&&freeCalls==0&&resource.releases==0,"Missing export cannot prove retirement");}
 {LiveFixture c;setup(c);c.proxy_device=nullptr;check(!cleanup_detached_live(c),"Null initialization device must be refused");
  check(shutdownCalls==0&&freeCalls==0,"Null must never reach global driver shutdown");}
 {LiveFixture c;setup(c);shutdownResult=0xbad00001u;check(!cleanup_detached_live(c),"Driver failure must retain");
  check(c.initialized&&freeCalls==0&&resource.releases==0,"Failed shutdown cannot release ownership");}
 {LiveFixture c;setup(c);releaseResult=0xbad00001u;check(!cleanup_detached_live(c),"Feature release failure must retain");
  check(destroyCalls==0&&shutdownCalls==0&&freeCalls==0,"Failed feature release must stop cleanup");}
 {LiveFixture c;setup(c);destroyResult=0xbad00001u;check(!cleanup_detached_live(c),"Parameter release failure must retain");
  check(shutdownCalls==0&&freeCalls==0,"Failed parameter release must stop cleanup");}
 {LiveFixture c;setup(c);bridge=true;shared=false;check(cleanup_detached_live(c)&&shutdownCalls==1,"Inactive bridge must use direct shutdown");}
 {LiveFixture c;setup(c);bridge=shared=true;owner=0;check(cleanup_detached_live(c)&&shutdownCalls==1,"Verified non-owner must use direct shutdown");}
 {LiveFixture c;setup(c);c.initialized=false;check(cleanup_detached_live(c)&&shutdownCalls==0,"Never initialized generation must not shut down driver");}
 std::cout<<checks<<" actual-source NGX shutdown ABI and ownership checks passed\n";
}
'''.replace('__BODY__', body)
    with tempfile.TemporaryDirectory(prefix='mcd2-ngx-shutdown-') as folder:
        folder = Path(folder)
        cpp, exe = folder / 'shutdown.cpp', folder / 'shutdown.exe'
        cpp.write_text(harness)
        subprocess.run([args.compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror',
                        '-Wno-misleading-indentation', str(cpp), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == '__main__':
    main()
