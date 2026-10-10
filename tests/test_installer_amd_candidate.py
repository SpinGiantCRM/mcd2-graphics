"""Candidate assembly is offline, pinned, diagnostic-free and reversible."""
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import zipfile

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('assembler',ROOT/'tools/assemble_amd_candidate.py')
assembler=importlib.util.module_from_spec(spec);spec.loader.exec_module(assembler)
sha=lambda b:hashlib.sha256(b).hexdigest()

class CandidateAssemblyTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name);self.out=self.root/'candidate';self.archive=self.root/'artifact.zip'
        self.source='a'*40;self.url='https://github.com/SpinGiantCRM/mcd2-graphics/actions/runs/1/artifacts/2'
        self.content={n:('own fixture '+n).encode() for n in assembler.OWN}
        self.content['experimental-reshade/ReShade64.dll']=b'framework fixture'
        self.content['probe/fg-chain-observer.addon64']=b'diagnostic fixture'
        self.content['isolated-fsr-probe/fsr-owned-inputs.exe']=b'probe fixture'
        self.content['sr/build-receipt.json']=json.dumps({'fsrBridgeSHA256':sha(self.content['isolated-fsr-game-bridge/mcd2-fsr-game-bridge.dll'])}).encode()
        self.content['ui/build-receipt.json']=json.dumps({'version':'0.3.0-preview.1','payloadSHA256':{Path(n).name:sha(b) for n,b in self.content.items() if n.startswith('ui/Pak/')}}).encode()
        self.baseline={'version':'0.3.0-preview.1','files':{'Dungeons/Binaries/Win64/old.addon64':'c'*64},'upgradeFrom':{}}
        self.lock={'game':{'exeSHA256':'e'*64,'steamBuildID':'1'},'dependencies':{'ReShade':{'file':assembler.BIN+'d3d12.asi','sha256':'f'*64,'download':'old','archiveSHA256':'f'*64,'archiveMembers':{},'buildReceiptSHA256':'f'*64},'DLSSRuntime':{'file':assembler.BIN+'MCD2Graphics/ngx-runtime/nvngx_dlss.dll','sha256':'b'*64}}}
        (self.root/'manifest.json').write_text(json.dumps(self.baseline));(self.root/'dependencies.lock.json').write_text(json.dumps(self.lock))
        for name in ('third-party/FSR-SDK-HEADERS-LICENSE.txt','third-party/AntiLag2/LICENSE.txt'):
            p=self.root/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text('Fixture license')
    def write(self, mutate=None, extra=None):
        pins={n:sha(b) for n,b in self.content.items()}
        r={'sourceCommit':self.source,'files':pins}
        if mutate:mutate(r)
        with zipfile.ZipFile(self.archive,'w') as z:
            for n,b in self.content.items():z.writestr(n,b)
            z.writestr('windows-build-receipt.json',json.dumps(r))
            if extra:z.writestr(*extra)
        return sha(self.archive.read_bytes())
    def build(self, pin=None, source=None, version='0.4.0-amd.dev.1', url=None):
        if pin is None:pin=self.write()
        with patch.object(assembler,'ROOT',self.root):
            return assembler.assemble(self.archive,pin,source or self.source,self.out,version,url or self.url)
    def test_exact_mapping_external_dependencies_and_release_preservation(self):
        before={n:(self.root/n).read_bytes() for n in ['manifest.json','dependencies.lock.json']}
        r=self.build();m=json.loads((self.out/'manifest.json').read_text());lock=json.loads((self.out/'dependencies.lock.json').read_text())
        self.assertEqual(set(m['files']),set(assembler.OWN.values())|set(assembler.CONFIG))
        for n,pin in m['files'].items():self.assertEqual(sha((self.out/'payload'/n).read_bytes()),pin)
        self.assertFalse(r['windowsRuntimeQualified']);self.assertFalse(r['frameworkBundled']);self.assertTrue(m['candidateOnly'])
        self.assertEqual(m['uiMetadataVersion'],'0.3.0-preview.1')
        prior=m['upgradeFrom'][self.baseline['version']]
        self.assertEqual(prior[self.lock['dependencies']['DLSSRuntime']['file']],'b'*64)
        self.assertEqual(lock['dependencies']['ReShade']['file'],assembler.BIN+'d3d12.asi')
        self.assertEqual(lock['game'],assembler.GAME)
        self.assertNotIn('download',lock['dependencies']['ReShade']);self.assertNotIn('archiveSHA256',lock['dependencies']['ReShade'])
        self.assertEqual(lock['dependencies']['ReShade']['upgradeFromFiles'],{assembler.BIN+'d3d12.asi':'f'*64})
        self.assertIn('Fixture license',(self.out/'AMD_BRIDGE_NOTICES.txt').read_text())
        for ident,(n,pin) in assembler.FSR_RUNTIME.items():self.assertEqual(lock['dependencies'][ident]['sha256'],pin)
        self.assertFalse(any(p.name.startswith(('amd_fidelityfx_','nvngx_','sl.')) or p.name in ['ReShade64.dll','fg-chain-observer.addon64'] or p.suffix=='.exe' for p in (self.out/'payload').rglob('*')))
        for n,b in before.items():self.assertEqual((self.root/n).read_bytes(),b)
    def test_archive_and_source_mismatch_before_output(self):
        pin=self.write()
        for kwargs in [{'pin':'0'*64},{'pin':pin,'source':'0'*40}]:
            with self.assertRaises(ValueError):self.build(**kwargs)
            self.assertFalse(self.out.exists())
    def test_member_pin_mismatch(self):
        pin=self.write(lambda r:r['files'].update({'sr/live_dense.cso':'0'*64}))
        with self.assertRaisesRegex(ValueError,'member hash'):self.build(pin=pin)
        self.assertFalse(self.out.exists())
    def test_unrecorded_and_unsafe_member_refused(self):
        for n in ['extra.dll','../escape.dll','C:/escape.dll','escape\\file.dll']:
            with self.subTest(n=n):
                pin=self.write(extra=(n,b'unknown'))
                with self.assertRaises(ValueError):self.build(pin=pin)
                self.assertFalse(self.out.exists())
    def test_duplicate_member_refused(self):
        pin=self.write(extra=('sr/live_dense.cso',b'duplicate'))
        with self.assertRaisesRegex(ValueError,'Duplicate'):self.build(pin=pin)
        self.assertFalse(self.out.exists())
    def test_wrong_bridge_binding(self):
        self.content['sr/build-receipt.json']=b'{"fsrBridgeSHA256":"bad"}'
        with self.assertRaisesRegex(ValueError,'binding'):self.build()
        self.assertFalse(self.out.exists())
    def test_missing_license_prevents_output(self):
        (self.root/'third-party/AntiLag2/LICENSE.txt').unlink()
        with self.assertRaises(FileNotFoundError):self.build()
        self.assertFalse(self.out.exists())
    def test_ui_mismatch(self):
        self.content['ui/build-receipt.json']=b'{"version":"old","payloadSHA256":{"MCD2Graphics_P.pak":"bad"}}'
        with self.assertRaisesRegex(ValueError,'UI payload'):self.build()
    def test_output_release_version_and_signed_url_refused(self):
        with self.assertRaises(ValueError):self.build(version='0.3.0-preview.1')
        with self.assertRaises(ValueError):self.build(url=self.url+'?sig=secret')
        self.out.mkdir();(self.out/'keep.txt').write_text('keep')
        with self.assertRaises(ValueError):self.build()
        self.assertEqual((self.out/'keep.txt').read_text(),'keep')

if __name__=='__main__':unittest.main()
