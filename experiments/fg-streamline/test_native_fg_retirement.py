"""Exercise actual Native FG retirement/readback through the unlocked release window."""
from pathlib import Path
import os,shutil,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[2]
def function(source,signature):
 start=source.index(signature);begin=source.index('{',start);end=begin+1;depth=1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[start:end]
class NativeFgRetirement(unittest.TestCase):
 def test_actual_retirement_readback_never_reports_unlocked_release_complete(self):
  observer=(ROOT/'src/native/observer.cpp').read_text();native=(ROOT/'src/native/native_fg_guides.hpp').read_text()
  self.assertTrue('int mcd2_native_fg_retired_v1()' in observer,'Native retirement readback export missing')
  export=function(observer,'extern "C" __declspec(dllexport) int mcd2_native_fg_retired_v1()')
  release=function(native,'if(c.retiring&&cache.recordings.empty()')
  begin=native.index(' if(retirementReleasing)return;');creation=native[begin:native.index(' auto &c=*generation;if(c.busy)',begin)]
  fixture=(ROOT/'experiments/fg-streamline/native_fg_retirement_fixture.cpp').read_text().replace('// ACTUAL_EXPORT',export).replace('// ACTUAL_RELEASE',release).replace('// ACTUAL_CREATION',creation)
  compiler=shutil.which('g++') or shutil.which('clang++');self.assertIsNotNone(compiler)
  with tempfile.TemporaryDirectory() as temporary:
   folder=Path(temporary);cpp=folder/'retire.cpp';exe=folder/('retire.exe'if os.name=='nt'else'retire');cpp.write_text(fixture)
   command=[compiler,'-std=c++20','-pthread','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)]
   if os.name=='nt':command.insert(1,'-static')
   compiled=subprocess.run(command,capture_output=True,text=True);self.assertEqual(compiled.returncode,0,compiled.stdout+compiled.stderr)
   ran=subprocess.run([str(exe)],capture_output=True,text=True,timeout=10);self.assertEqual(ran.returncode,0,ran.stdout+ran.stderr)
if __name__=='__main__':unittest.main()
