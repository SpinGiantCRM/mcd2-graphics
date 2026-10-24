"""Run owned-input MFG qualification explicitly; never benchmark or launch a game."""
import argparse
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import time

RUNTIME_SHA256='f131734e9bf1fd191a9f60fad54231cd88e45ada3c957bd02c8763f8e9ad4440'
BINARY='mfg-compute-probe.exe'
RUNTIME='dlss-enabler-headless.dll'


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def validate_build(build):
    build=Path(build).resolve()
    receipt=json.loads((build/'build-receipt.json').read_text())
    if (receipt.get('probeKind')!='mfg-owned-inputs' or receipt.get('runtimeBundled') is not False
            or receipt.get('gameIntegrationQualified') is not False):
        raise ValueError('Expected isolated owned-input probe receipt')
    if sha(build/BINARY)!=receipt.get('binarySHA256'):
        raise ValueError('Probe differs from its build receipt')
    return receipt


def validate(build,runtime):
    receipt=validate_build(build)
    if sha(runtime)!=RUNTIME_SHA256:
        raise ValueError('Expected exact externally acquired Enabler 4.9.0 module')
    return receipt


def validate_prefix(prefix):
    prefix=Path(prefix).resolve()
    for path in (prefix,(prefix/'pfx').resolve()):
        if any(part.lower() in ('steamapps','1912410','wingdk','dungeons2') for part in path.parts):
            raise ValueError('A game/Steam prefix is forbidden')
    if not (prefix/'pfx').is_dir():
        raise ValueError('Explicit existing isolated Proton prefix required')
    return prefix


def game_running():
    needles=('dungeons-win64-shipping','dungeons-wingdk-shipping')
    if os.name=='nt':
        rows=subprocess.check_output(['tasklist','/FO','CSV','/NH'],text=True)
        return any(any(needle in row[0].lower() for needle in needles) for row in csv.reader(rows.splitlines()) if row)
    for process in Path('/proc').iterdir():
        if not process.name.isdigit(): continue
        try: command=(process/'cmdline').read_bytes().lower()
        except (OSError,PermissionError): continue
        if any(needle.encode() in command for needle in needles): return True
    return False


def image_gate(rows,exit_code,timed_out,multiplier):
    if timed_out or exit_code!=0 or multiplier not in range(2,7): return False
    for stage in ('adapter','capability','init','populate','scratch','create','release','shutdown','complete'):
        selected=[r for r in rows if r.get('stage')==stage]
        if len(selected)!=1: return False
        row=selected[0]
        if stage in ('init','populate','scratch','create','release','shutdown') and row.get('result')!=1: return False
        if stage=='adapter' and (row.get('software') is not False or not row.get('vendor') or not row.get('device')): return False
        if stage=='capability' and not multiplier-1<=row.get('generatedMax',0)<=5: return False
        if stage=='complete' and (row.get('images')!=10*(multiplier-1) or row.get('swapchain') is not False
                                 or row.get('gameIntegrationQualified') is not False or row.get('moduleRetainedUntilExit') is not True): return False
    if any(r.get('stage')=='failure' or ('result' in r and r['result']!=1) for r in rows): return False
    if len([r for r in rows if r.get('stage')=='evaluate'])!=12*(multiplier-1): return False
    images=[r for r in rows if r.get('stage')=='image']
    if len(images)!=10*(multiplier-1): return False
    for frame in range(2,12):
        batch=images[(frame-2)*(multiplier-1):(frame-1)*(multiplier-1)]
        if len({r.get('hash') for r in batch})!=multiplier-1: return False
        prior=-1
        for slot,row in enumerate(batch,1):
            if (row.get('epoch')!=1 or row.get('previous')!=frame or row.get('current')!=frame+1
                    or row.get('slot')!=slot or row.get('denominator')!=multiplier
                    or row.get('result')!=1 or row.get('finite') is not True or row.get('nonzero') is not True
                    or not isinstance(row.get('hash'),str) or not re.fullmatch('[0-9a-f]{16}',row['hash'])): return False
            offset,error=row.get('bestOffset'),row.get('meanAbsoluteError')
            if (not isinstance(offset,int) or offset<=prior or not 0<=offset<=8 or abs(offset-8*slot/multiplier)>1
                    or not isinstance(error,(int,float)) or not math.isfinite(error) or not 0<=error<=.12): return False
            prior=offset
    return True


def run(build,runtime,run_dir,multiplier,proton=None,prefix=None,steam=None):
    receipt=validate(build,runtime)
    if multiplier not in range(2,7): raise ValueError('Supported qualification counts are 2x-6x')
    build,runtime,run_dir=Path(build).resolve(),Path(runtime).resolve(),Path(run_dir).resolve()
    if any(part.lower() in ('steamapps','wingdk','dungeons2','1912410') for part in run_dir.parts):
        raise ValueError('A run directory inside a game/Steam tree is forbidden')
    if run_dir.exists() or run_dir.is_symlink(): raise ValueError('A fresh run directory is required')
    if game_running(): raise ValueError('Close MCD2 before the isolated GPU qualification')
    environment=os.environ.copy()
    if os.name=='nt':
        if proton or prefix or steam: raise ValueError('Proton arguments are not used on native Windows')
    else:
        if not proton or not prefix or not steam: raise ValueError('Explicit Proton, Steam directory and isolated prefix required')
        prefix=validate_prefix(prefix)
        if not Path(proton).is_file(): raise ValueError('Missing Proton launcher')
        environment['STEAM_COMPAT_DATA_PATH']=str(prefix)
        environment['WINEPREFIX']=str(prefix/'pfx')
        if not Path(steam).is_dir(): raise ValueError('Missing Steam client directory')
        environment['STEAM_COMPAT_CLIENT_INSTALL_PATH']=str(Path(steam).resolve())
    run_dir.mkdir(parents=True,exist_ok=False)
    shutil.copyfile(build/BINARY,run_dir/BINARY)
    shutil.copyfile(runtime,run_dir/RUNTIME)
    # Copy only the own probe and one exact external DLL; no game's proxy,
    # settings, runtime tree or launcher configuration is copied.
    command=[str(run_dir/BINARY),str(run_dir/RUNTIME),str(multiplier)]
    if os.name!='nt': command=[str(Path(proton).resolve()),'run',*command]
    started=time.monotonic(); timed_out=False
    with (run_dir/'process-private.log').open('w') as output:
        process=subprocess.Popen(command,cwd=run_dir,env=environment,stdout=output,stderr=subprocess.STDOUT,start_new_session=os.name!='nt')
        try: code=process.wait(timeout=180)
        except subprocess.TimeoutExpired:
            timed_out=True
            if os.name=='nt': subprocess.run(['taskkill','/PID',str(process.pid),'/T','/F'],check=False,stdout=output,stderr=subprocess.STDOUT)
            else: os.killpg(process.pid,signal.SIGKILL)
            try: code=process.wait(timeout=10)
            except subprocess.TimeoutExpired: code=None
    evidence=run_dir/'mfg-evidence.jsonl'
    try: rows=[json.loads(line) for line in evidence.read_text().splitlines()] if evidence.exists() else []
    except (ValueError,OSError): rows=[]
    result={'schemaVersion':1,'kind':'owned-mfg-compute','multiplier':multiplier,
            'runtimeSHA256':RUNTIME_SHA256,'binarySHA256':receipt['binarySHA256'],
            'exitCode':code,'timedOut':timed_out,'elapsedSeconds':round(time.monotonic()-started,2),
            'functionalImageChecksPassed':image_gate(rows,code,timed_out,multiplier),
            'visualReviewRequired':True,'gameIntegrationQualified':False,'benchmark':False}
    (run_dir/'run-result.json').write_text(json.dumps(result,indent=2)+'\n')
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('build','runtime','run-dir'): parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--multiplier',type=int,choices=range(2,7),required=True)
    parser.add_argument('--proton',type=Path)
    parser.add_argument('--prefix',type=Path)
    parser.add_argument('--steam',type=Path)
    args=parser.parse_args()
    result=run(args.build,args.runtime,args.run_dir,args.multiplier,args.proton,args.prefix,args.steam)
    print(json.dumps(result,indent=2))
    raise SystemExit(0 if result['functionalImageChecksPassed'] else 1)


if __name__=='__main__': main()
