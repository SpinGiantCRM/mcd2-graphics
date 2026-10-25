"""Package verified own files for the package-specific Vortex companion."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import zipfile

spec=importlib.util.spec_from_file_location('mcd2_contract_builder',Path(__file__).with_name('build_install_contract.py'))
contract_builder=importlib.util.module_from_spec(spec);spec.loader.exec_module(contract_builder)

def build_package(manifest,dependency_lock,payload,output):
    contract=contract_builder.build_contract(manifest,dependency_lock)
    payload=Path(payload).resolve();output=Path(output)
    verified=[]
    for relative,expected in contract['payload'].items():
        target=payload/relative
        if target.is_symlink() or not target.resolve().is_relative_to(payload) or not target.is_file():raise ValueError('Unsafe/missing payload')
        with target.open('rb') as f:actual=hashlib.file_digest(f,'sha256').hexdigest()
        if actual!=expected:raise ValueError('Payload hash mismatch: '+relative)
        verified.append((relative,target))
    if output.exists():raise ValueError('Output already exists')
    output.parent.mkdir(parents=True,exist_ok=True)
    try:
        with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED) as z:
            z.writestr('mcd2-install.json',json.dumps(contract,indent=2)+'\n')
            for relative,target in verified:
                # Recheck the exact bytes being archived, not just an earlier stat/hash.
                data=target.read_bytes()
                if hashlib.sha256(data).hexdigest()!=contract['payload'][relative]:raise ValueError('Payload changed while packaging')
                z.writestr('payload/'+relative,data)
    except BaseException:
        output.unlink(missing_ok=True);raise
    return contract

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('manifest','dependency-lock','payload','output'):p.add_argument('--'+name,required=True,type=Path)
    a=p.parse_args();build_package(json.loads(a.manifest.read_bytes()),json.loads(a.dependency_lock.read_bytes()),a.payload,a.output)
if __name__=='__main__':main()
