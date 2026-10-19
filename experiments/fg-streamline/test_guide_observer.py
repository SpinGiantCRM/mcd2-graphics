"""Run the actual bounded observer against resources outlived by cached descriptors."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent

class GuideObserverLifetime(unittest.TestCase):
    def test_actual_observe_rejects_dead_resources_before_native_access(self):
        source = (ROOT / 'guide_recon.cpp').read_text()
        start = source.index('void observe(a::command_list*')
        body = source[start:source.index('\nBinding binding(', start)]
        fixture = (ROOT / 'guide_observer_fixture.cpp').read_text()
        self.assertEqual(fixture.count('// ACTUAL_OBSERVE'), 1)
        compiler = shutil.which('g++') or shutil.which('clang++')
        self.assertIsNotNone(compiler, 'Portable C++ compiler is required for the observer regression')
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            cpp, exe = folder / 'observer.cpp', folder / ('observer.exe' if os.name == 'nt' else 'observer')
            cpp.write_text(fixture.replace('// ACTUAL_OBSERVE', body))
            command = [compiler, '-std=c++20', '-O2', '-Wall', '-Wextra', '-Werror', '-Wno-misleading-indentation', str(cpp), '-o', str(exe)]
            if os.name == 'nt':command.insert(1, '-static')
            compiled = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(compiled.returncode, 0, compiled.stdout + compiled.stderr)
            ran = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(ran.returncode, 0, ran.stdout + ran.stderr)

if __name__ == '__main__':
    unittest.main()
