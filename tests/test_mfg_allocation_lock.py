"""CPU-only check of the production configuration entry points' gate ownership."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class AllocationLock(unittest.TestCase):
 def test_legacy_entry_releases_gate_for_worker_retirement(self):
  compiler=shutil.which('g++')
  if not compiler:self.skipTest('g++ unavailable')
  source=(ROOT/'experiments/providers/mfg_game_bridge.cpp').read_text()
  entry=source[source.index('extern "C" int mcd2_mfg_configure_v3'):source.index('extern "C" int mcd2_afg_swap_v1')]
  fixture=r'''
#include <mutex>
#include <thread>
#include <cstdio>
#include "src/providers/mfg_phase_plan.hpp"
using namespace mcd2::mfg;
constexpr int MCD2_AFG_BUSY_V2=-2;
std::recursive_mutex& gate(){static std::recursive_mutex g;return g;}
struct Runtime{bool softwareReady=true,nativeReady=true;unsigned softwareMax=5,nativeMax=5;};
Runtime& runtime(){static Runtime r;return r;}
struct Session{bool closing=false,nativeRoute=false;unsigned strategy=3,multiplier=6,dlssGenerated=0,fsrGenerated=5,order=0;struct{unsigned active=1,restartRequired=0;}state;};
Session*& current(){static Session s;static Session* p=&s;return p;}
struct SessionLease{explicit SessionLease(Session&) {}};
bool workerAcquired=false;
bool retireFeatures(Session&){gate().unlock();std::thread worker([]{if(gate().try_lock()){workerAcquired=true;gate().unlock();}});worker.join();gate().lock();return workerAcquired;}
unsigned selectedStrategy=3,selectedMultiplier=6,selectedDlss=0,selectedFsr=5,selectedOrder=0;
ENTRIES
int main(){if(mcd2_mfg_configure_v1(0,1)!=0||!workerAcquired){std::puts("Legacy config retained an outer gate lock during retirement");return 1;}workerAcquired=false;current()->strategy=3;current()->multiplier=6;current()->fsrGenerated=5;if(mcd2_mfg_configure_v1(2,4)!=0||!workerAcquired)return 2;workerAcquired=false;if(mcd2_mfg_configure_v3(2,4,1,2,33)!=0||!workerAcquired||current()->order!=33)return 3;workerAcquired=false;if(mcd2_mfg_configure_v2(2,4,1,2)!=0||!workerAcquired||current()->order!=0)return 4;return 0;}
'''.replace('ENTRIES',entry)
  with tempfile.TemporaryDirectory(prefix='mcd2-allocation-lock-') as d:
   cpp=Path(d)/'test.cpp';binary=Path(d)/'test';cpp.write_text(fixture)
   built=subprocess.run([compiler,'-std=c++20','-pthread','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-I'+str(ROOT),str(cpp),'-o',str(binary)],capture_output=True,text=True)
   self.assertEqual(built.returncode,0,built.stdout+built.stderr)
   run=subprocess.run([str(binary)],capture_output=True,text=True,timeout=10)
   self.assertEqual(run.returncode,0,run.stdout+run.stderr)
if __name__=='__main__':unittest.main()
