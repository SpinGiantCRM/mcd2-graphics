"""Recreate the separate PR435 framework ZIP from pinned candidate inputs."""
from pathlib import Path
import argparse,json,hashlib,zipfile,shutil
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--runtime',required=True,type=Path);p.add_argument('--receipt',required=True,type=Path);p.add_argument('--license',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args()
r=Path(__file__).resolve().parent;lock=json.loads((r.parents[1]/'dependencies.lock.json').read_text())['dependencies']['ReShade'];receipt=json.loads(a.receipt.read_text());sha=lambda b:hashlib.sha256(b).hexdigest()
import sys
sys.path.insert(0,str(r.parents[1]/'experiments/fg-streamline'))
from framework_provenance import source_matches
patch_sha=sha((r/'reshade-reset-epoch.patch').read_bytes())
assert patch_sha==lock['patchSHA256'] and source_matches(receipt,patch_sha)
assert receipt['sourceClean'] is False and receipt['patchSHA256']==patch_sha
assert sha(a.runtime.read_bytes())==receipt['binarySHA256']==lock['sha256'];assert not a.output.exists()
assert set(receipt)=={'commit','sourceClean','binarySHA256','configuration','WindowsQualified','baseCleanBeforePatch','approvedPatchOnly','patchSHA256'},'Unexpected receipt fields; sanitize before packaging'
inputs={'LICENSE.md':a.license,'README.md':r/'ReShade-dependency-README.md','ReShade64.dll':a.runtime,'build-receipt.json':a.receipt}
layout=json.loads((r/'framework-archive-layout.json').read_text())
with zipfile.ZipFile(a.output,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
 for name,spec in layout.items():
  info=zipfile.ZipInfo(name,tuple(spec['date_time']));info.external_attr=spec['external_attr'];info.create_system=spec['create_system'];info.compress_type=spec['compress_type'];
  with inputs[name].open("rb") as source,z.open(info,"w") as dest:shutil.copyfileobj(source,dest,8192)
assert sha(a.output.read_bytes())==lock['archiveSHA256'],'Archive differs from pinned dependency'
print('Separate candidate framework dependency reproduced with pinned SHA-256')
