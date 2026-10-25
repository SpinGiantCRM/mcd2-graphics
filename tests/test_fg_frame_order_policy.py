"""The exact managed menu and native scheduler must agree on every order/cap."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class OrderPolicy(unittest.TestCase):
 def test_managed_native_all_splits_orders_and_caps(self):
  compiler=shutil.which('g++');dotnet=shutil.which('dotnet')
  if not compiler or not dotnet:self.skipTest('g++ and .NET SDK required')
  with tempfile.TemporaryDirectory(prefix='mcd2-order-policy-') as d:
   p=Path(d);managed=p/'managed.csv';cpp=p/'oracle.cpp';binary=p/('oracle.exe' if os.name=='nt' else 'oracle')
   cpp.write_text('''#include "src/providers/fg_frame_order.hpp"
#include <iostream>
int main(){for(unsigned d=0;d<=5;++d)for(unsigned f=0;f<=5;++f)if(d+f>=1&&d+f<=5)for(unsigned code=0;code<64;++code)for(unsigned cap=0;cap<=5;++cap){auto mask=mcd2::mfg::resolve_order_mask(d,f,code,cap);std::cout<<d<<','<<f<<','<<code<<','<<cap<<','<<(mask?int(*mask):-1)<<'\\n';}}
''')
   built=subprocess.run([compiler,'-std=c++20','-Wall','-Wextra','-Werror','-I'+str(ROOT),str(cpp),'-o',str(binary)],capture_output=True,text=True,timeout=60)
   self.assertEqual(built.returncode,0,built.stdout+built.stderr)
   native=subprocess.run([str(binary)],capture_output=True,text=True,timeout=30);self.assertEqual(native.returncode,0,native.stderr)
   result=subprocess.run([dotnet,'run','--project',str(ROOT/'tests/provider-menu-client/ProviderMenuClient.Tests.csproj'),'-c','Release','--','--order-table',str(managed)],capture_output=True,text=True,timeout=120)
   self.assertEqual(result.returncode,0,result.stdout+result.stderr)
   expected=native.stdout.splitlines();actual=managed.read_text().splitlines();self.assertEqual(len(actual),7680);self.assertEqual(actual,expected)
if __name__=='__main__':unittest.main()
