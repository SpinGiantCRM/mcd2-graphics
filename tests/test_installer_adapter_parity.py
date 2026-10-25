"""Run the real managed engine and JS adapter on one synthetic game directory."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from tools.build_install_contract import build_contract

ROOT=Path(__file__).resolve().parents[1]
SDK=os.environ.get('MCD2_TEST_DOTNET')
@unittest.skipUnless(SDK and shutil.which('node'),'Set MCD2_TEST_DOTNET for cross-adapter fixtures')
class AdapterParityTests(unittest.TestCase):
 def test_legacy_standalone_vortex_standalone_preserves_preferences(self):
  with tempfile.TemporaryDirectory() as temp:
   base=Path(temp);root=base/'game';payload=base/'payload';root.mkdir();(root/'.synthetic-install-fixture').touch()
   own='Dungeons/Binaries/Win64/mcd2-graphics.addon64';runtime='Dungeons/Binaries/Win64/MCD2Graphics/ngx-runtime/nvngx_dlss.dll';exe='Dungeons/Binaries/Win64/Dungeons-Win64-Shipping.exe'
   def put(p,data):p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
   h=lambda b:hashlib.sha256(b).hexdigest()
   put(root/exe,b'game');put(payload/own,b'payload');put(payload/runtime,b'external runtime');put(root/'Dungeons2/Saved/preferences.json',b'{"preset":"keep"}')
   m={'version':'fixture.test','description':'Parity – preserved preferences','files':{own:h(b'payload')}}
   l={'game':{'steamBuildID':'1','exeSHA256':h(b'game')},'dependencies':{'DLSSRuntime':{'version':'1','file':runtime,'sha256':h(b'external runtime'),'url':'https://example.com/dlss'}}}
   c=build_contract(m,l)
   for name,value in [('manifest.json',m),('lock.json',l),('mcd2-install.json',c)]:put(base/name,(json.dumps(value,ensure_ascii=False)+'\n').encode())
   dll=ROOT/'tests/installer-core/bin/Release/net10.0/InstallerCore.Tests.dll'
   def managed(action,ok=True):
    result=subprocess.run([SDK,str(dll),'--transition-fixture',action,str(root),str(payload),str(base/'manifest.json'),str(base/'lock.json'),str(base/'mcd2-install.json')],text=True,capture_output=True)
    if ok:self.assertEqual(result.returncode,0,result.stderr)
    else:self.assertNotEqual(result.returncode,0)
    return result
   self.assertEqual(json.loads(managed('install').stdout)['owner'],'Standalone')
   # Remove new fields to exercise the recognised historical standalone shape.
   marker=root/'Dungeons/Binaries/Win64/MCD2Graphics/install-manifest.json';r=json.loads(marker.read_bytes());r.pop('owner');r.pop('schemaVersion');marker.write_text(json.dumps(r))
   script=r'''const fs=require('node:fs/promises'),path=require('node:path');const home=process.argv[1],root=process.argv[2],payload=process.argv[3],phase=process.argv[4];const c=JSON.parse(require('node:fs').readFileSync(path.join(home,'mcd2-install.json')));const adapter=require(process.argv[5]+'/integrations/vortex/src/deployment');const ownership=require(process.argv[5]+'/integrations/vortex/src/ownership');const ctx={root,contract:c,edition:'Steam',profileId:'fixture',requireClosed:async()=>{},stagedFiles:new Map(Object.keys(c.payload).concat(c.dependencies.flatMap(d=>Object.keys(d.files))).map(p=>[p,path.join(payload,p)]))};(async()=>{if(phase==='status'){console.log(JSON.stringify(await ownership.checkOwnership(ctx)));return;}if(phase==='install'){await adapter.beforeDeploy(ctx);for(const [p,source] of ctx.stagedFiles){await fs.mkdir(path.dirname(path.join(root,p)),{recursive:true});await fs.link(source,path.join(root,p));}await adapter.verifyDeployed(ctx);}if(phase==='remove'){await adapter.beforeRemove(ctx);for(const p of ctx.stagedFiles.keys())await fs.rm(path.join(root,p),{force:true});await adapter.afterRemove(ctx);}console.log(JSON.stringify(await ownership.checkOwnership(ctx)));})().catch(e=>{console.error(e.message);process.exit(1)});'''
   def vortex(phase,ok=True):
    r=subprocess.run(['node','-e',script,str(base),str(root),str(payload),phase,str(ROOT)],text=True,capture_output=True)
    if ok:self.assertEqual(r.returncode,0,r.stderr)
    else:self.assertNotEqual(r.returncode,0)
    return r
   self.assertEqual(json.loads(vortex('status').stdout)['owner'],'standalone');vortex('install',False)
   managed('remove');self.assertEqual(json.loads(vortex('install').stdout)['owner'],'vortex')
   managed('install',False);managed('remove',False)
   self.assertEqual(json.loads(vortex('remove').stdout)['owner'],'unmanaged')
   managed('install');managed('remove')
   self.assertEqual((root/'Dungeons2/Saved/preferences.json').read_bytes(),b'{"preset":"keep"}')
   self.assertEqual((payload/own).read_bytes(),b'payload')
