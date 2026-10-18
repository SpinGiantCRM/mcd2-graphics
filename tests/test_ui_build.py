"""Offline build-contract checks; fake compiler outputs never install a mod."""
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

import build_ui as ui


class UIBuildTests(unittest.TestCase):
    def test_named_dlss_presets_keep_auto_quality_dependent(self):
        actor = (ui.ROOT / 'src/ui/ModActor.cs').read_text()
        self.assertIn('ModSetting.Widget("advanced_graphics", typeof(AdvancedSettingsPanel))', actor)
        panel = (ui.ROOT / "src/ui/AdvancedSettingsPanel.cs").read_text()
        self.assertNotIn('ModSetting.Slider("dlss_sr_model"', actor)
        self.assertIn('"Auto"', panel)
        choices = panel.split('if(setting==0)return new List<FText>', 1)[1].split(';', 1)[0]
        labels = ['"Auto"', 'E - CNN', 'F - CNN', 'J - Transformer',
                  'K - Transformer', 'L - Transformer 2nd Gen', 'M - Transformer 2nd Gen']
        self.assertEqual(sorted(choices.index(label) for label in labels), [choices.index(label) for label in labels])
        self.assertIn('return "Show In-Game Diagnostics"', panel)
        self.assertIn('C - Performance', panel)
        self.assertNotIn('Custom (manual override)', panel)
        self.assertIn('4.1 - INT8 (Experimental)', panel)
        self.assertIn('4.0 - ML (Experimental)', panel)
        self.assertIn('OnFocusReceived', panel)
        self.assertNotIn('SetToolTip', panel)
        # The renderer must allocate fresh NGX parameters and supply the quality
        # mode while leaving every model-preset override unset for Auto (zero).
        renderer = (ui.ROOT / 'src/native/ngx_live_fixture.hpp').read_text()
        create = renderer.split('result=allocate(&c.params);', 1)[1].split('result=create(cmd,', 1)[0]
        self.assertIn('NVSDK_NGX_Parameter_PerfQualityValue', create)
        for mode in ('DLAA', 'UltraPerformance', 'MaxPerf', 'Balanced', 'MaxQuality'):
            self.assertIn('NVSDK_NGX_PerfQuality_Value_' + mode, create)
        override = create.split('if(model){', 1)[1].split('})ui(c.params,key,model);}', 1)[0]
        for mode in ('DLAA', 'Quality', 'Balanced', 'Performance', 'UltraPerformance', 'UltraQuality'):
            key = 'NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_' + mode
            self.assertIn(key, override)
            self.assertEqual(create.count(key), 1)

    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.sdk = self.root / 'sdk'
        self.dotnet = self.root / 'dotnet/dotnet'
        self.tools = self.root / 'pack tools'
        pins = json.loads((ui.ROOT / 'dependencies.lock.json').read_text())['buildDependencies']
        self.version = pins['dotnetSDK']
        self.dotnet.parent.mkdir()
        self.dotnet.write_bytes(b'fake executable')
        self.refs = self.dotnet.parent / 'packs/Microsoft.NETCore.App.Ref' / pins['dotnetReferencePack'] / 'ref/net10.0'
        self.refs.mkdir(parents=True)
        (self.refs / 'System.Runtime.dll').write_bytes(b'reference')
        for folder in ('src', 'ref', 'tools'):
            (self.sdk / folder).mkdir(parents=True)
        (self.sdk / 'NeoRune.Sdk.nuspec').write_text('<package><metadata><version>' + pins['NeoRuneSDK'] + '</version></metadata></package>')
        for name in ('ModSettings.cs', 'Log.cs', 'Trail.cs'):
            (self.sdk / 'src' / name).write_text('// helper')
        for name in ('NeoRune.Abstractions.dll', 'NeoRune.Game.dll'):
            (self.sdk / 'ref' / name).write_bytes(b'binding')
        (self.sdk / 'tools/neorune.dll').write_bytes(b'compiler')
        self.tools.mkdir()
        for name in ('repak.exe', 'retoc.exe'):
            (self.tools / name).write_bytes(b'packer')
        self.output = self.root / 'build output'

    def compile(self, *, diagnostic='', returncode=0, missing=None, **kwargs):
        def compiler(command, **options):
            self.assertEqual(command[:2], [str(self.dotnet.resolve()), str(self.sdk.resolve() / 'tools/neorune.dll')])
            args = Path(command[-1][1:]).read_text().splitlines()
            self.assertIn('--source=' + str(self.sdk.resolve() / 'src/ModSettings.cs'), args)
            self.assertIn('--source=' + str(self.sdk.resolve() / 'src/Trail.cs'), args)
            self.assertTrue(any(arg.startswith('--description=') for arg in args))
            self.assertNotIn('install', args)
            for name in ('ModInfo.uasset', 'ModInfo.uexp'):
                (self.output / 'Assets').mkdir(exist_ok=True)
                (self.output / 'Assets' / name).write_bytes(b'metadata')
            (self.output / 'Pak').mkdir()
            for name in ui.PAYLOAD_NAMES:
                if name != missing:
                    (self.output / 'Pak' / name).write_bytes(b'payload')
            version = json.loads((ui.ROOT / 'manifest.json').read_text())['version']
            (self.output / 'Pak/MCD2Graphics_P.neorune.json').write_text(json.dumps({'mod': 'MCD2Graphics', 'neoRune': '1.0.0', 'version': version}))
            return subprocess.CompletedProcess(command, returncode, diagnostic, '')
        with patch.object(ui.subprocess, 'check_output', return_value=self.version + '\n'), patch.object(ui.subprocess, 'run', side_effect=compiler) as run:
            receipt = ui.build_ui(self.dotnet, self.sdk, self.tools, self.output, **kwargs)
            self.assertEqual(run.call_count, 1)  # no legacy metadata rewrite or install
            return receipt

    def test_build_generates_metadata_and_complete_source_receipt(self):
        receipt = self.compile()
        self.assertEqual(set(receipt['sourceFileSHA256']), set(ui.SOURCE_INPUTS))
        self.assertFalse(receipt['WindowsQualified'])
        self.assertFalse(receipt['LinuxRuntimeQualified'])
        self.assertNotIn('trialOnly', receipt)
        self.assertNotIn('tools/ModInfoBuilder/Program.cs', receipt['sourceFileSHA256'])
        self.assertEqual(set(receipt['payloadSHA256']), set(ui.PAYLOAD_NAMES))

    def test_probe_overlay_remains_explicitly_a_trial(self):
        actor = self.root / 'ModActor.cs'
        probe = self.root / 'Probe.cs'
        actor.write_text('// actor overlay')
        probe.write_text('// probe')
        receipt = self.compile(actor=actor, extra_sources=(probe,))
        self.assertTrue(receipt['trialOnly'])
        self.assertEqual(receipt['trialSourceSHA256']['Probe.cs'], ui.sha(probe))

    def test_existing_output_is_not_reused(self):
        self.output.mkdir()
        (self.output / 'old').write_bytes(b'do not overwrite')
        with self.assertRaisesRegex(ValueError, 'empty'):
            ui.build_ui(self.dotnet, self.sdk, self.tools, self.output)

    def test_wrong_package_is_refused(self):
        (self.sdk / 'NeoRune.Sdk.nuspec').write_text('<package><version>0.1.2</version></package>')
        with self.assertRaisesRegex(ValueError, 'pin'):
            ui.build_ui(self.dotnet, self.sdk, self.tools, self.output)

    def test_wrong_dotnet_is_refused(self):
        with patch.object(ui.subprocess, 'check_output', return_value='10.0.401\n'), self.assertRaisesRegex(ValueError, 'Expected .NET'):
            ui.build_ui(self.dotnet, self.sdk, self.tools, self.output)

    def test_zero_exit_with_compiler_error_is_refused(self):
        with self.assertRaisesRegex(RuntimeError, 'compilation failed'):
            self.compile(diagnostic='error CS1000: bad input')

    def test_incomplete_package_is_refused(self):
        with self.assertRaisesRegex(RuntimeError, 'complete UI package'):
            self.compile(missing='MCD2Graphics_P.utoc')

    def test_response_file_refuses_newline_injection(self):
        with self.assertRaises(ValueError):
            ui.response_file(['--description=hello\ninstall'])
        self.assertEqual(ui.response_file(['--out=C:/folder with spaces']), '--out=C:/folder with spaces\n')

    def test_source_receipt_normalizes_windows_line_endings(self):
        a, b = self.root / 'a', self.root / 'b'
        a.write_bytes(b'hello\nworld\n')
        b.write_bytes(b'hello\r\nworld\r\n')
        self.assertEqual(ui.source_sha(a), ui.source_sha(b))

    def test_ci_sdk_versions_match_dependency_lock(self):
        for name in ('portable-checks.yml', 'amd-installer-candidate.yml', 'update2-candidate.yml'):
            self.assertIn("dotnet-version: '" + self.version + "'", (ui.ROOT / '.github/workflows' / name).read_text())


if __name__ == '__main__':
    unittest.main()
