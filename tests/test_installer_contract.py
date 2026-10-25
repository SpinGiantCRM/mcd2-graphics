"""Exercise generated packages: no vendor payload, unsafe paths or release mixing."""
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
def load(name):
    spec=importlib.util.spec_from_file_location(name, ROOT/'tools'/f'{name}.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module

class ContractTests(unittest.TestCase):
    def setUp(self):
        self.hash=hashlib.sha256(b'payload').hexdigest()
        self.manifest={'version':'candidate.test','files':{'Dungeons/Binaries/Win64/mcd2-graphics.addon64':self.hash}}
        self.lock={'game':{'steamBuildID':'123','exeSHA256':self.hash,'winGDK':{'version':'1.1.3.0','exeSHA256':self.hash}},'dependencies':{'BlueprintLoader':{'version':'2.7','file':'Dungeons/Content/Paks/~mods/BlueprintLoader/BlueprintLoader_P.pak','sha256':self.hash,'download':'https://www.nexusmods.com/minecraftdungeons2/mods/2?tab=files'}}}
    def test_generation_maps_both_editions_and_conservatively_retains_requirements(self):
        c=load('build_install_contract').build_contract(self.manifest,self.lock)
        self.assertEqual(c['schemaVersion'],1)
        self.assertEqual(c['editionProfiles']['WinGDK']['binaryDirectory'],'Dungeons/Binaries/WinGDK')
        self.assertEqual(c['dependencies'][0]['role'],'common')
        self.assertEqual(c['dependencies'][0]['acquisition']['method'],'browser')
        self.assertEqual(c['configuration']['startup']['keys'],{'r.AllowHDR':'1','r.AntiAliasingMethod':'2'})
    def test_refuses_unsafe_paths_bad_pins_and_external_payload(self):
        for path in ['../bad','Dungeons/Binaries/Win64/NVNGX_DLSS.DLL','Dungeons/Binaries/Win64/dlss-enabler-headless.dll','/absolute','a//b','a\\b']:
            m=copy.deepcopy(self.manifest);m['files']={path:self.hash}
            with self.subTest(path=path), self.assertRaises(ValueError):load('build_install_contract').build_contract(m,self.lock)
        m=copy.deepcopy(self.manifest);m['files'][next(iter(m['files']))]='0'
        with self.assertRaises(ValueError):load('build_install_contract').build_contract(m,self.lock)
    def test_vortex_archive_contains_only_verified_payload_and_contract(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);payload=root/'payload';target=payload/next(iter(self.manifest['files']));target.parent.mkdir(parents=True);target.write_bytes(b'payload')
            output=root/'vortex.zip'
            load('build_vortex_package').build_package(self.manifest,self.lock,payload,output)
            with zipfile.ZipFile(output) as z:
                self.assertEqual(sorted(z.namelist()),['mcd2-install.json','payload/Dungeons/Binaries/Win64/mcd2-graphics.addon64'])
                self.assertEqual(z.read('payload/'+next(iter(self.manifest['files']))),b'payload')
                self.assertEqual(json.loads(z.read('mcd2-install.json'))['packageVersion'],'candidate.test')
            target.write_bytes(b'wrong')
            with self.assertRaises(ValueError):load('build_vortex_package').build_package(self.manifest,self.lock,payload,root/'bad.zip')
            self.assertFalse((root/'bad.zip').exists())
    def test_refuses_duplicate_case_destinations_and_mixed_release_members(self):
        m=copy.deepcopy(self.manifest);m['files']['Dungeons/Binaries/Win64/MCD2-GRAPHICS.addon64']=self.hash
        with self.assertRaises(ValueError):load('build_install_contract').build_contract(m,self.lock)
        l=copy.deepcopy(self.lock);l['dependencies']['BlueprintLoader']['compatibleReleases']=[{'version':'bad','files':{'elsewhere':self.hash}}]
        with self.assertRaises(ValueError):load('build_install_contract').build_contract(self.manifest,l)

if __name__=='__main__':unittest.main()

class InstallerRevisionTests(unittest.TestCase):
    def test_installer_revision_preserves_native_version_and_previous_receipt_pins(self):
        h='a'*64;runtime='Dungeons/Binaries/Win64/MCD2Graphics/ngx-runtime/nvngx_dlss.dll'
        m={'version':'0.5.0-integration.dev.4','uiMetadataVersion':'0.5.0-integration.dev.4','files':{'Dungeons/Binaries/Win64/own.addon64':h},'upgradeFrom':{}}
        l={'dependencies':{'DLSSRuntime':{'file':runtime,'sha256':'b'*64}}}
        result=load('build_install_contract').installer_revision(m,l,'0.5.0-integration.dev.4-install.1')
        self.assertEqual(result['uiMetadataVersion'],'0.5.0-integration.dev.4')
        self.assertEqual(result['upgradeFrom']['0.5.0-integration.dev.4'][runtime],'b'*64)
        self.assertEqual(result['files'],m['files']);self.assertEqual(m['version'],'0.5.0-integration.dev.4')
        with self.assertRaises(ValueError):load('build_install_contract').installer_revision(m,l,'0.6.0')
