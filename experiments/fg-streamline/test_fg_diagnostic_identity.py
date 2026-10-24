"""Compile the actual identity export reader against nonblocking getter doubles."""
from pathlib import Path
import os,shutil,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[2]
class DiagnosticIdentityTests(unittest.TestCase):
 def test_actual_identity_reader_busy_invalid_and_changed_epoch(self):
  source=(ROOT/'experiments/fg-streamline/guide_recon.cpp').read_text();start=source.index('mcd2::diagnostics::IdentityRead diagnostic_identity(');end=source.index('\n}\n',start)+2
  fixture=(ROOT/'experiments/fg-streamline/fg_diagnostic_identity_fixture.cpp').read_text().replace('// ACTUAL_IDENTITY_READER',source[start:end])
  compiler=shutil.which('g++') or shutil.which('clang++');self.assertIsNotNone(compiler)
  with tempfile.TemporaryDirectory() as temporary:
   folder=Path(temporary);cpp=folder/'identity.cpp';exe=folder/('identity.exe'if os.name=='nt'else'identity');cpp.write_text(fixture)
   args=[compiler,'-std=c++20','-pthread','-Wall','-Wextra','-Werror','-I'+str(ROOT/'src/providers'),str(cpp),'-o',str(exe)]
   if os.name=='nt':args.insert(1,'-static')
   r=subprocess.run(args,capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stdout+r.stderr)
   r=subprocess.run([str(exe)],capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stdout+r.stderr)
if __name__=='__main__':unittest.main()
