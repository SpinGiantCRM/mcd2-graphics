"""Run filesystem installation gates against an isolated fixture supplied by developer."""
from pathlib import Path
import argparse, tempfile, json, shutil, importlib.util
p=argparse.ArgumentParser();p.add_argument('--reference-game',required=True,type=Path);p.add_argument('--dlss-runtime',required=True,type=Path);p.add_argument('--output',type=Path,default=Path(__file__).parent/'docs/install-validation.json');a=p.parse_args()
spec=importlib.util.spec_from_file_location('installer',Path(__file__).with_name('install.py'));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
lock=json.loads((m.HERE/'dependencies.lock.json').read_text());baseline={'Dungeons/Binaries/Win64/Dungeons-Win64-Shipping.exe':lock['game']['exeSHA256']}
for k in ('ReShade','RenoDXUEExtended'):
 d=lock['dependencies'][k];baseline[d['file']]=d['sha256']
baseline.update(lock['dependencies']['BlueprintLoader']['files']);checks=[]
with tempfile.TemporaryDirectory(prefix='mcd2-install-gate-') as td:
 root=Path(td).resolve()
 try:m.install(root,a.dlss_runtime);raise AssertionError('Missing dependency accepted')
 except ValueError:checks.append('missing dependency refused before writes')
 for n,sha in baseline.items():
  dest=root/n;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(a.reference_game/n,dest)
 protected=root/'Dungeons/Content/Paks/~mods/Unrelated/readme.txt';protected.parent.mkdir(parents=True);protected.write_text('unrelated mod')
 m.install(root,a.dlss_runtime);checks.append('fresh install and copied hashes verified')
 try:m.install(root,a.dlss_runtime);raise AssertionError('Duplicate install accepted')
 except ValueError:checks.append('duplicate install refused')
 addon=root/'Dungeons/Binaries/Win64/mcd2-graphics.addon64';data=addon.read_bytes();addon.write_bytes(data+b'x')
 try:m.uninstall(root);raise AssertionError('Modified addon removed')
 except ValueError:checks.append('modified owned file retained')
 addon.write_bytes(data);m.uninstall(root)
 assert protected.read_text()=='unrelated mod'
 assert all(m.digest(root/n)==sha for n,sha in baseline.items())
 assert not addon.exists();checks.append('uninstall preserved every dependency and unrelated mod')
 m.install(root,a.dlss_runtime);m.uninstall(root);checks.append('reinstall after uninstall passed')
 for name in ('../outside','/outside'):
  try:m.target(root,name);raise AssertionError('Unsafe path accepted')
  except ValueError:pass
 checks.append('unsafe paths rejected')
a.output.write_text(json.dumps({'isolatedFixture':True,'checks':checks},indent=2)+'\n')
print('All '+str(len(checks))+' filesystem gates passed')
