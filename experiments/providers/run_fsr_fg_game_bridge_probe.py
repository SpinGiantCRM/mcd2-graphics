"""Run the exact AMD game-bridge ABI in an isolated owned-scene Proton host."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import signal
import subprocess
import time

RUNTIME = {'amd_fidelityfx_framegeneration_dx12.dll': '02297beedd285e822d3a64f314cf00faf378dcec0edc47ff0c4dd71b3a8c2f18'}
def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()

def validate(output, prefix):
    if 'steamapps' in prefix.parts or '1912410' in prefix.parts or not (prefix/'pfx').is_dir():
        raise ValueError('Existing isolated prefix required; game prefixes forbidden')
    receipt=json.loads((output/'build-receipt.json').read_text())
    for name,key in [('mcd2-fsr-fg-game-bridge.dll','binarySHA256'),('fsr-fg-game-bridge-probe.exe','probeSHA256')]:
        if digest(output/name)!=receipt.get(key): raise ValueError('Build receipt mismatch: '+name)
    if receipt.get('runtimeBundled') is not False or receipt.get('gameIntegrationQualified') is not False:
        raise ValueError('This host cannot qualify game integration')
    for name,expected in RUNTIME.items():
        if digest(output/name)!=expected: raise ValueError('Pinned external runtime required')
    for name in ['dxgi.dll','d3d12.dll','d3d12core.dll','amdxc64.dll','sl.interposer.dll']:
        if (output/name).exists(): raise ValueError('Unexpected proxy/driver: '+name)

def gate(rows, code, timeout):
    if code!=0 or timeout: return False
    phases=[r for r in rows if r.get('stage')=='phase']
    if len(phases)!=3: return False
    for index,row in enumerate(phases):
        count=60 if index==1 else 30
        if row.get('phase')!=index or row.get('enabled') is not (index==1): return False
        if row.get('real')!=count or row.get('generated')!=(count if index==1 else 0): return False
        if any(row.get(key)!=0 for key in ['fault','errors','warnings']): return False
    counts={'load-verified-runtime':1,'create-bridge-swapchain':1,'set-PQ-output':1,
            'owned-presenter-antilag-ready':1,'clear-antilag-before-presentation':1,'clear-antilag-after-presentation':1,
            'prepare-game-bridge-inputs':60,'copy-hudless-world':60,'configure-bridge-present':120,
            'retain-live-recording':1,'retain-after-failed-Reset':1,
            'retire-after-successful-Reset-and-own-fence':1,'complete':1}
    for name,count in counts.items():
        events=[r for r in rows if r.get('stage')==name]
        expected=-61 if name.startswith('retain-') else 0
        if len(events)!=count or any(r.get('result')!=expected for r in events): return False
    ordered=['retain-live-recording','retain-after-failed-Reset','retire-after-successful-Reset-and-own-fence','complete']
    indices=[next(i for i,r in enumerate(rows) if r.get('stage')==name) for name in ordered]
    return indices==sorted(indices)

def run(proton, prefix, steam, output):
    validate(output,prefix)
    env=os.environ.copy()
    for key in ['LD_PRELOAD','VKD3D_CONFIG','VKD3D_FEATURE_LEVEL','VKD3D_SHADER_DEBUG','PROTON_ENABLE_HDR',
                'DXVK_HDR','PROTON_LOG','MANGOHUD','MANGOHUD_CONFIG']: env.pop(key,None)
    env.update(STEAM_COMPAT_DATA_PATH=str(prefix),STEAM_COMPAT_CLIENT_INSTALL_PATH=str(steam),
               STEAM_COMPAT_INSTALL_PATH=str(output),STEAM_COMPAT_APP_ID='0',SteamAppId='0',SteamGameId='0',
               WINEDEBUG='-all',PROTON_LOG='0',PROTON_ENABLE_WAYLAND='1',PROTON_FSR4_UPGRADE='0',
               WINEDLLOVERRIDES='d3d12,d3d12core,dxgi=n')
    application=output/'fsr-fg-game-bridge-probe.jsonl';application.unlink(missing_ok=True)
    timeout=False;start=time.monotonic()
    with (output/'run-private.log').open('w') as log:
        process=subprocess.Popen([str(proton),'runinprefix',str(output/'fsr-fg-game-bridge-probe.exe')],
                                 cwd=output,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
        try:code=process.wait(timeout=50)
        except subprocess.TimeoutExpired:
            timeout=True;os.killpg(process.pid,signal.SIGTERM)
            try:code=process.wait(timeout=5)
            except subprocess.TimeoutExpired:os.killpg(process.pid,signal.SIGKILL);code=process.wait(timeout=5)
    rows=[json.loads(line) for line in application.read_text().splitlines()] if application.exists() else []
    result=dict(exitCode=code,timeout=timeout,seconds=round(time.monotonic()-start,3),
                passed=gate(rows,code,timeout),applicationRows=rows,gameIntegrationQualified=False,
                runtimeBundled=False,bridgeSHA256=digest(output/'mcd2-fsr-fg-game-bridge.dll'),
                probeSHA256=digest(output/'fsr-fg-game-bridge-probe.exe'),runtimeSHA256=RUNTIME)
    (output/'run-result.json').write_text(json.dumps(result,indent=2)+'\n')
    return result

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ['proton','prefix','steam','output']:parser.add_argument('--'+name,required=True,type=Path)
    args=parser.parse_args();result=run(*(getattr(args,n).resolve() for n in ['proton','prefix','steam','output']))
    print(json.dumps({k:v for k,v in result.items() if k!='applicationRows'}))
    raise SystemExit(0 if result['passed'] else 1)
