"""Folder deployment/package preflight; publisher/compiler replaced by a fixture."""
import hashlib
import json
from pathlib import Path
import runpy
import shutil
import sys
import tempfile
import unittest
from unittest.mock import patch
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[1]

class FolderPackageTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name);self.source=self.root/'source';self.output=self.root/'output'
        self.relative='Dungeons/Binaries/Win64/own.addon64';self.data=b'fixture only'
        p=self.source/self.relative;p.parent.mkdir(parents=True);p.write_bytes(self.data)
        self.manifest={'version':'0.0.0-test','files':{self.relative:hashlib.sha256(self.data).hexdigest()}}
        (self.root/'manifest.json').write_text(json.dumps(self.manifest));(self.root/'dependencies.lock.json').write_text('{}')
        (self.root/'third-party').mkdir();(self.root/'third-party/INSTALLER_NOTICES.txt').write_text('fixture notices')
        shutil.copyfile(ROOT/'build_installer.py',self.root/'build_installer.py')

    def build(self, nested=False, extra=(), during_build=None):
        def compiler(args,check):
            self.assertTrue(check);self.assertIn('-p:PublishSingleFile=false',args)
            self.assertNotIn('-p:PublishSingleFile=true',args)
            for prop,name in [('InstallerManifestPath','manifest.json'),('InstallerDependencyLockPath','dependencies.lock.json')]:
                self.assertIn('-p:'+prop+'='+str((self.output/name).resolve()),args)
            (self.output/'mcd2-graphics-installer.exe').write_bytes(b'fixture host')
            (self.output/'runtime.dll').write_bytes(b'fixture runtime')
            if nested:(self.output/'unexpected.zip').write_bytes(b'fixture')
            if during_build:during_build()
        with patch.object(sys,'argv',['build_installer.py','--dotnet',str(self.root/'dotnet'),'--payload',str(self.source),'--rid','win-x64','--output',str(self.output),*extra]),patch('subprocess.run',side_effect=compiler):
            runpy.run_path(str(self.root/'build_installer.py'),run_name='__main__')

    def test_candidate_pin_files_are_embedded_and_copied_without_changing_defaults(self):
        original_manifest=(self.root/'manifest.json').read_bytes()
        candidate=dict(self.manifest,version='0.0.0-candidate')
        pins=self.root/'candidate';pins.mkdir()
        (pins/'manifest.json').write_text(json.dumps(candidate))
        (pins/'dependencies.lock.json').write_text('{"candidateOnly":true}')
        self.build(extra=('--manifest',str(pins/'manifest.json'),'--dependency-lock',str(pins/'dependencies.lock.json')))
        receipt=json.loads((self.output/'installer-build.json').read_text())
        self.assertEqual(receipt['candidateVersion'],candidate['version'])
        for name,key in [('manifest.json','manifestSHA256'),('dependencies.lock.json','dependencyLockSHA256')]:
            self.assertEqual((self.output/name).read_bytes(),(pins/name).read_bytes())
            self.assertEqual(receipt[key],hashlib.sha256((pins/name).read_bytes()).hexdigest())
        self.assertEqual((self.root/'manifest.json').read_bytes(),original_manifest)
        self.assertEqual((self.root/'dependencies.lock.json').read_text(),'{}')

    def test_original_pin_edits_during_publish_do_not_change_embedded_snapshot(self):
        original=(self.root/'manifest.json').read_bytes()
        self.build(during_build=lambda:(self.root/'manifest.json').write_text('{}'))
        self.assertEqual((self.output/'manifest.json').read_bytes(),original)

    def test_compiler_cannot_replace_snapshot_and_still_emit_receipt(self):
        with self.assertRaisesRegex(AssertionError,'snapshot changed'):
            self.build(during_build=lambda:(self.output/'dependencies.lock.json').write_text('{}\n'))
        self.assertFalse((self.output/'installer-build.json').exists())

    def test_external_runtimes_cannot_enter_owned_payload(self):
        for name in ('sl.reflex.dll','nvngx_dlss.dll','nvngx_dlssg.dll',
                     'amd_fidelityfx_upscaler_dx12.dll','amd_fidelityfx_framegeneration_dx12.dll',
                     'NvLowLatencyVk.dll','ReShade64.dll','d3d12.asi'):
            with self.subTest(name=name):
                relative='Dungeons/Binaries/Win64/'+name
                (self.source/relative).write_bytes(self.data)
                manifest=dict(self.manifest,files={relative:hashlib.sha256(self.data).hexdigest()})
                (self.root/'manifest.json').write_text(json.dumps(manifest))
                with self.assertRaisesRegex(AssertionError,'runtime is not owned payload'):self.build()
                self.assertFalse(self.output.exists())

    def test_visible_payload_and_complete_receipt(self):
        self.build();r=json.loads((self.output/'installer-build.json').read_text())
        self.assertEqual((self.output/'payload'/self.relative).read_bytes(),self.data)
        self.assertEqual(r['deployment'],'self-contained-folder');self.assertFalse(r['selfExtraction']);self.assertFalse(r['embeddedPayloadArchive'])
        self.assertIn('runtime.dll',r['files']);self.assertIn('MCD2-Graphics-Installer.exe',r['files'])
        self.assertIn('payload/'+self.relative,r['files']);self.assertFalse(r['signedReleaseQualified'])
        for name,h in r['files'].items():self.assertEqual(hashlib.sha256((self.output/name).read_bytes()).hexdigest(),h)

    def test_modified_source_prevents_publish(self):
        (self.source/self.relative).write_bytes(b'wrong')
        with self.assertRaises(AssertionError):self.build()
        self.assertFalse(self.output.exists())

    def test_nonempty_output_rejected(self):
        self.output.mkdir();(self.output/'keep.txt').write_text('retain')
        with self.assertRaises(ValueError):self.build()
        self.assertEqual((self.output/'keep.txt').read_text(),'retain')

    def test_nested_archive_rejected(self):
        with self.assertRaises(AssertionError):self.build(nested=True)
        self.assertFalse((self.output/'installer-build.json').exists())

    def test_gui_has_no_embedded_payload_or_self_extraction(self):
        project=ET.parse(ROOT/'src/installer/MCD2.Installer.csproj').getroot()
        self.assertEqual(project.findtext('.//PublishSingleFile'),'false')
        self.assertIsNone(project.find('.//IncludeNativeLibrariesForSelfExtract'))
        self.assertFalse(any(x.attrib.get('LogicalName')=='payload.zip' for x in project.findall('.//EmbeddedResource')))
        self.assertIn('Path.Combine(AppContext.BaseDirectory,"payload")',(ROOT/'src/installer/Program.cs').read_text())

if __name__=='__main__':unittest.main()
