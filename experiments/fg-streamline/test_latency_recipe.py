"""Exercise the actual shared-adapter generator without invoking a compiler."""
import json
from pathlib import Path
import runpy
import sys
import tempfile
import subprocess
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]

class SharedLatencyRecipe(unittest.TestCase):
    def test_current_source_adopts_shared_sdk_and_links_version_metadata(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            probe, headers, output = root/'probe', root/'headers', root/'output'
            probe.mkdir(); headers.mkdir()
            (probe/'build-receipt.json').write_text(json.dumps({'earlyBootstrap': True, 'ownBinariesSHA256': {}}))
            def compile_fixture(command, **kwargs):
                if '--version' in command:return subprocess.CompletedProcess(command,0,stdout="g++ 15")
                self.assertIn('-lversion', command)
                Path(command[command.index('-o')+1]).write_bytes(b'compiler fixture')
            args = ['build_game_candidate.py', '--probe-output', str(probe), '--reshade-headers', str(headers), '--output', str(output)]
            with patch.object(sys, 'argv', args), patch('subprocess.run', side_effect=compile_fixture):
                runpy.run_path(str(ROOT/'experiments/fg-streamline/build_game_candidate.py'), run_name='__main__')
            source = (output/'latency_candidate.cpp').read_text()
            self.assertIn('auto folder=advancedRoot.parent_path()', source)
            self.assertIn('folder/L"fg-sdk-bridge.dll"', source)
            self.assertIn('ready&&ready()?0:-100', source)
            self.assertIn('mcd2_fg_visit_frame', source)
            self.assertIn('runtime_diagnostics_transport.hpp', source)
            self.assertIn('diagnosticRuntimeResult=runtime?runtime(&response):-3', source)
            self.assertIn('diagnosticSrCache.observe(diagnosticIdentity,diagnosticRequested', source)
            self.assertIn('publishIdentity,diagnosticSrSample,applied', source)
            self.assertEqual(source.count('runtime(&response)'), 1)
            self.assertNotIn('folder/L"mcd2-streamline-bridge.dll"', source)
            self.assertNotIn('register_event<reshade::addon_event::init_device>(init)', source)

if __name__ == '__main__':
    unittest.main()
