"""The legacy CLI must not bypass guided HDR/dependency ownership."""
import subprocess,sys,unittest
from pathlib import Path
class CandidateCliTests(unittest.TestCase):
    def test_update2_cli_refuses_before_mutation(self):
        root=Path(__file__).resolve().parents[1]
        r=subprocess.run([sys.executable,str(root/'install.py'),'install','--game',str(root/'unused-test-destination')],capture_output=True,text=True)
        self.assertNotEqual(r.returncode,0)
        self.assertIn('standalone guided installer',r.stderr)
        self.assertFalse((root/'unused-test-destination').exists())
if __name__=='__main__':unittest.main()
