"""Exercise actual FG producer decisions independently of SDK/GPU objects."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]

class FgDiagnosticEpoch(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory()
        folder = Path(cls.temporary.name)
        guide = (ROOT/'experiments/fg-streamline/guide_recon.cpp').read_text()
        bridge = (ROOT/'experiments/providers/fsr_fg_game_bridge.cpp').read_text()
        begin = guide.index('MCD2FgDiagnosticBindingV1 binding{')
        binding = guide[begin:guide.index(';if(bind)', begin)+1]
        identity = next(line.strip() for line in guide.splitlines() if 'const auto identity=wanted?diagnostic_identity()' in line)
        begin = bridge.index(' std::lock_guard lock(guard);if(!session||session->closing||wanted>1)')
        present = bridge[begin:bridge.index(' auto* supplied=', begin)]
        begin = bridge.index('extern "C" int mcd2_afg_diagnostic_bind_v1(')
        bind = bridge[begin:bridge.index('\nextern "C" int mcd2_afg_diagnostics_v2(', begin)]
        begin = bridge.index(' if(!configured&&s.diagnosticBinding.engineFrame==id)')
        admit = bridge[begin:bridge.index(' return configured;', begin)]
        begin = guide.index(' nvidiaDiagnosticFrames.select(')
        nvidia = guide[begin:guide.index('\n}', begin)]
        fixture = (ROOT/'experiments/fg-streamline/fg_diagnostic_epoch_fixture.cpp').read_text()
        for key, value in {'GUIDE_IDENTITY':identity,'BINDING':binding,'PRESENT_PREFIX':present,'BIND_EXPORT':bind,'FINAL_ADMISSION':admit,'NVIDIA':nvidia}.items():
            if fixture.count('// ACTUAL_'+key)!=1:raise RuntimeError('Missing unique fixture marker '+key)
            fixture = fixture.replace('// ACTUAL_'+key, value)
        compiler = shutil.which('g++') or shutil.which('clang++')
        if not compiler:raise RuntimeError('Portable C++ compiler required for FG epoch regression')
        cpp=folder/'epoch.cpp';cpp.write_text(fixture)
        cls.exe=folder/('epoch.exe' if os.name=='nt' else 'epoch')
        command=[compiler,'-std=c++20','-O2','-pthread','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-I'+str(ROOT/'src/providers'),str(cpp),'-o',str(cls.exe)]
        if os.name=='nt':command.insert(1,'-static')
        result=subprocess.run(command,capture_output=True,text=True)
        if result.returncode:raise RuntimeError(result.stdout+result.stderr)
    @classmethod
    def tearDownClass(cls):cls.temporary.cleanup()
    def run_case(self, name):
        result=subprocess.run([str(self.exe),name],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
    def test_amd_epoch_survives_unadmitted_frames(self):self.run_case('amd')
    def test_nvidia_epoch_survives_unadmitted_frames(self):self.run_case('nvidia')

if __name__=='__main__':unittest.main()
