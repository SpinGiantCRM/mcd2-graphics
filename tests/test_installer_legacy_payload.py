"""Default legacy installers retain frozen labels independently of candidate UI metadata."""
from pathlib import Path
import hashlib,json,runpy,shutil,subprocess,sys,tempfile,unittest,zipfile
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
sha=lambda data:hashlib.sha256(data).hexdigest()

class LegacyPayloadTests(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup);self.repo=Path(self.tmp.name)
  self.manifest=json.loads((ROOT/'manifest.json').read_text())
  for path in ['qualification/update2/prepare_payload.py','qualification/update2/own-payload.zip','qualification/fg-release/own-payload.zip','manifest.json']:
   dest=self.repo/path;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/path,dest)
  self.output=self.repo/'dist/qualified-own'
 def extract(self):return subprocess.run([sys.executable,str(self.repo/'qualification/update2/prepare_payload.py')],cwd=self.repo,capture_output=True,text=True)
 def verify(self):
  self.assertEqual({p.relative_to(self.output).as_posix() for p in self.output.rglob('*') if p.is_file()},set(self.manifest['files']))
  for name,h in self.manifest['files'].items():self.assertEqual(sha((self.output/name).read_bytes()),h)
 def test_default_legacy_extract_and_gui_manifest_keep_frozen_preview_version(self):
  self.assertEqual(self.manifest['version'],'0.3.0-preview.1');self.assertEqual(len(self.manifest['files']),12)
  r=self.extract();self.assertEqual(r.returncode,0,r.stdout+r.stderr);self.verify()
  project=(ROOT/'src/installer/MCD2.Installer.csproj').read_text();self.assertIn('>../../manifest.json</InstallerManifestPath>',project)
  self.assertIn('frozen 0.3.0-preview.1',(ROOT/'.github/workflows/update2-candidate.yml').read_text())
 def test_ui_candidate_version_does_not_relabel_legacy_payload(self):
  self.manifest['uiCandidateVersion']='0.4.0-amd.dev.99';(self.repo/'manifest.json').write_text(json.dumps(self.manifest));r=self.extract();self.assertEqual(r.returncode,0,r.stdout+r.stderr);self.verify();self.assertEqual(self.manifest['version'],'0.3.0-preview.1')
 def test_unsupported_legacy_package_version_is_refused_without_output(self):
  self.manifest['version']='0.4.0-amd.dev.21';(self.repo/'manifest.json').write_text(json.dumps(self.manifest));r=self.extract();self.assertNotEqual(r.returncode,0);self.assertIn('Unsupported legacy qualification version',r.stderr);self.assertFalse(self.output.exists())
 def test_wrong_archive_members_and_duplicates_are_refused_before_output(self):
  archive=self.repo/'qualification/fg-release/own-payload.zip'
  for duplicate in [False,True]:
   with zipfile.ZipFile(archive,'w') as z:
    if duplicate:
     with zipfile.ZipFile(ROOT/'qualification/fg-release/own-payload.zip') as original:
      for name in original.namelist():z.writestr(name,original.read(name))
      with self.assertWarns(UserWarning):z.writestr(original.namelist()[0],original.read(original.namelist()[0]))
    else:z.writestr('wrong.txt',b'wrong')
   r=self.extract();self.assertNotEqual(r.returncode,0);self.assertIn('archive members differ',r.stderr);self.assertFalse(self.output.exists())
 def test_corrupt_member_hash_is_refused_before_output(self):
  archive=self.repo/'qualification/fg-release/own-payload.zip'
  with zipfile.ZipFile(ROOT/'qualification/fg-release/own-payload.zip') as original,zipfile.ZipFile(archive,'w') as z:
   for n,name in enumerate(original.namelist()):z.writestr(name,b'corrupt' if n==0 else original.read(name))
  r=self.extract();self.assertNotEqual(r.returncode,0);self.assertIn('Qualification hash mismatch',r.stderr);self.assertFalse(self.output.exists())
 def test_default_installer_receipt_and_embedded_version_bind_to_frozen_payload(self):
  r=self.extract();self.assertEqual(r.returncode,0,r.stdout+r.stderr);output=self.repo/'installer';commands=[]
  def publish(command,**kwargs):
   commands.append(command);self.assertIn('-p:Version=0.3.0-preview.1',command)
   snapshot=next(x.split('=',1)[1] for x in command if x.startswith('-p:InstallerManifestPath='));self.assertEqual(Path(snapshot).read_bytes(),(ROOT/'manifest.json').read_bytes())
   (output/'mcd2-graphics-installer.exe').write_bytes(b'compiler-double');return subprocess.CompletedProcess(command,0)
  argv=['build_installer.py','--dotnet',str(self.repo/'dotnet.exe'),'--payload',str(self.output),'--rid','win-x64','--output',str(output)]
  with patch.object(sys,'argv',argv),patch('subprocess.run',side_effect=publish):runpy.run_path(str(ROOT/'build_installer.py'),run_name='__main__')
  receipt=json.loads((output/'installer-build.json').read_text());self.assertEqual(len(commands),1);self.assertEqual(receipt['candidateVersion'],'0.3.0-preview.1');self.assertEqual(receipt['manifestSHA256'],sha((ROOT/'manifest.json').read_bytes()));self.assertEqual(receipt['dependencyLockSHA256'],sha((ROOT/'dependencies.lock.json').read_bytes()));self.assertEqual(receipt['payload'],self.manifest['files']);self.assertFalse(receipt['windowsRuntimeQualified'])
  for name,h in receipt['payload'].items():self.assertEqual(sha((output/'payload'/name).read_bytes()),h)

if __name__=='__main__':unittest.main()
