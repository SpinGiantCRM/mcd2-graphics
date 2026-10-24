"""Exercise the addon's actual initialization guard after terminal teardown."""
from pathlib import Path
import shutil, subprocess, tempfile, unittest

ROOT=Path(__file__).resolve().parents[1]

class Reinitialize(unittest.TestCase):
    def test_retained_stopped_worker_cannot_report_success(self):
        compiler=shutil.which('g++')
        if not compiler:self.skipTest('Portable C++ compiler unavailable')
        source=(ROOT/'src/gdk/save_bridge_addon.cpp').read_text()
        prefix=source.split('bool AddonInit(HMODULE module,HMODULE){',1)[1].split('std::array<wchar_t,256> family',1)[0]
        harness='''
#include <mutex>
#include <cassert>
#include <atomic>
using HMODULE=int;
namespace mcd2::process_exit { void initialize(){} }
struct Worker { std::atomic<bool> stop=false; }; Worker* worker=nullptr;
bool closing=false,addonRegistered=false;int registrations=0;
std::mutex lifecycleMutex;
namespace reshade { bool register_addon(int){++registrations;return true;} }
bool initialize(int module,int){BODY return true;}
int main(){
 Worker retained;retained.stop=true;worker=&retained;closing=true;
 assert(!initialize(1,0));assert(registrations==0);
 worker=nullptr;assert(!initialize(1,0));assert(registrations==0);
 closing=false;addonRegistered=true;retained.stop=false;worker=&retained;
 assert(initialize(1,0));
 retained.stop=true;assert(!initialize(1,0));
 retained.stop=false;addonRegistered=false;assert(!initialize(1,0));
}
'''.replace('BODY',prefix)
        with tempfile.TemporaryDirectory() as tmp:
            cpp=Path(tmp)/'reinit.cpp';exe=Path(tmp)/'reinit.exe';cpp.write_text(harness)
            subprocess.run([compiler,'-std=c++20','-pthread',str(cpp),'-o',str(exe)],check=True,capture_output=True,text=True)
            result=subprocess.run([str(exe)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)

if __name__=='__main__':unittest.main()
