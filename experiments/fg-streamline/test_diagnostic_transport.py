"""Execute the actual transport getter/cache block with SDK-free getter doubles."""
from pathlib import Path
import os,shutil,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[2]
class DiagnosticTransportTests(unittest.TestCase):
 def test_actual_transport_busy_bounds_invalidation_and_metadata(self):
  source=(ROOT/'src/latency/runtime_diagnostics_transport.hpp').read_text();begin=source.index(' MCD2SrDiagnosticsV1 sr{');end=source.index(' // The V1 getter',begin)
  fixture=(ROOT/'experiments/fg-streamline/diagnostic_transport_fixture.cpp').read_text().replace('// ACTUAL_GETTERS',source[begin:end])
  compiler=shutil.which('g++') or shutil.which('clang++');self.assertIsNotNone(compiler)
  with tempfile.TemporaryDirectory() as temporary:
   folder=Path(temporary);cpp=folder/'transport.cpp';exe=folder/('transport.exe'if os.name=='nt'else'transport');cpp.write_text(fixture)
   args=[compiler,'-std=c++20','-pthread','-Wall','-Wextra','-Werror','-I'+str(ROOT/'src/providers'),str(cpp),'-o',str(exe)]
   if os.name=='nt':args.insert(1,'-static')
   r=subprocess.run(args,capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stdout+r.stderr)
   r=subprocess.run([str(exe)],capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stdout+r.stderr)
if __name__=='__main__':unittest.main()
