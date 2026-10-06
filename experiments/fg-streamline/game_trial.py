"""Reversible local bootstrap trial. Not an installer or public package."""
from pathlib import Path
import argparse, hashlib, json, shutil, sys
sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
from install import require_closed

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('action',choices=['install','restore'])
p.add_argument('--game',required=True,type=Path)
p.add_argument('--backup',required=True,type=Path)
p.add_argument('--probe',type=Path)
p.add_argument('--candidate',type=Path)
p.add_argument('--sr-candidate',type=Path,help='Optional temporary SR guide-notification build; backed up and restored separately')
p.add_argument('--experimental-reshade',type=Path)
p.add_argument('--experimental-reshade-receipt',type=Path)
a=p.parse_args()
game=a.game.resolve();backup=a.backup.resolve();record=backup/'transaction-private.json'
def sha(path):
    with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
require_closed()
for proc in Path('/proc').glob('[0-9]*'):
    try:
        if (proc/'comm').read_text().strip()=='GameThread':raise RuntimeError('Close the game before the trial transaction')
    except (FileNotFoundError,PermissionError,ProcessLookupError):pass
assert (game/'Dungeons-Win64-Shipping.exe').is_file()
if a.action=='install':
    assert not record.exists(),'A trial transaction already exists'
    lock=json.loads((Path(__file__).resolve().parents[2]/'dependencies.lock.json').read_text())
    assert sha(game/'Dungeons-Win64-Shipping.exe')==lock['game']['exeSHA256']
    assert sha(game/'dxgi.dll')==lock['dependencies']['ReShade']['sha256']
    assert bool(a.experimental_reshade)==bool(a.experimental_reshade_receipt)
    framework=game/'dxgi.dll';framework_receipt=None
    if a.experimental_reshade:
        framework_receipt=json.loads(a.experimental_reshade_receipt.read_text())
        assert framework_receipt.get('sourceClean') and framework_receipt.get('commit')
        assert sha(a.experimental_reshade)==framework_receipt['binarySHA256']
        framework=a.experimental_reshade
    pr=json.loads((a.probe/'build-receipt.json').read_text())
    cr=json.loads((a.candidate/'build-receipt.json').read_text())
    assert pr['earlyBootstrap'] and not cr['FGEnabled']
    assert sha(a.candidate/'mcd2-display-latency.addon64')==cr['addonSHA256']
    sources={name:a.probe/name for name in pr['files']}
    sources.update({'dxgi.dll':a.probe/'dxgi.dll','fg-sdk-bridge.dll':a.probe/'fg-sdk-bridge.dll',
                    'd3d12.asi':framework,'mcd2-display-latency.addon64':a.candidate/'mcd2-display-latency.addon64',
                    'FGBootstrap.ini':a.candidate/'FGBootstrap.ini'})
    if a.sr_candidate:
        sr=json.loads((a.sr_candidate/'build-receipt.json').read_text())
        assert sr['releasedSourcesChanged'] is False and sr['WindowsQualified'] is False
        assert sha(a.sr_candidate/'mcd2-graphics.addon64')==sr['addonSHA256']
        sources['mcd2-graphics.addon64']=a.sr_candidate/'mcd2-graphics.addon64'
    if 'mcd2-fg-guide-recon.addon64' in pr['ownBinariesSHA256']:
        sources['mcd2-fg-guide-recon.addon64']=a.probe/'mcd2-fg-guide-recon.addon64'
        assert sha(sources['mcd2-fg-guide-recon.addon64'])==pr['ownBinariesSHA256']['mcd2-fg-guide-recon.addon64']
        if 'FG_UI_ALPHA.cso' in pr['ownBinariesSHA256']:
            sources['FG_UI_ALPHA.cso']=a.probe/'FG_UI_ALPHA.cso'
            assert sha(sources['FG_UI_ALPHA.cso'])==pr['ownBinariesSHA256']['FG_UI_ALPHA.cso']
    for name in pr['files']:assert sha(sources[name])==pr['files'][name]
    for name in ['dxgi.dll','fg-sdk-bridge.dll']:assert sha(sources[name])==pr['ownBinariesSHA256'][name]
    for name in sources:
        if name not in ['dxgi.dll','mcd2-display-latency.addon64','mcd2-graphics.addon64']:assert not (game/name).exists(),f'Existing file: {name}'
    backup.mkdir(parents=True)
    entries={}
    for name in list(sources)+['ReShade.ini','bootstrap.jsonl','sl.log','nvapi64.log','nvapi.log','FGGuideCapture.ini']:
        old=game/name
        entries[name]={'before':sha(old) if old.exists() else None,'installed':sha(sources[name]) if name in sources else None}
        if old.exists():shutil.copy2(old,backup/name)
    protected={name:sha(game/name) for name in ['mcd2-graphics.addon64','renodx-ue-extended.addon64'] if name not in sources}
    state={'game':str(game),'entries':entries,'protected':protected,'restored':False,'experimentalReShade':framework_receipt,'releaseQualified':False}
    record.write_text(json.dumps(state,indent=2)+'\n')
    # Copy from backups when the original path is itself a replacement target.
    for name,source in sources.items():shutil.copyfile(backup/'dxgi.dll' if name=='d3d12.asi' and not a.experimental_reshade else source,game/name)
    print('FG-Off trial staged with a complete recovery transaction; RenoDX unchanged.'+(' Temporary SR observer installed.' if a.sr_candidate else ' SR unchanged.'))
else:
    state=json.loads(record.read_text());assert state['game']==str(game) and not state['restored']
    for name,entry in state['entries'].items():
        target=game/name
        if entry['installed'] is not None:assert target.exists() and sha(target)==entry['installed'],f'Trial file was changed: {name}'
        if target.exists() and name in ['ReShade.ini','bootstrap.jsonl','sl.log','nvapi64.log','nvapi.log','FGGuideCapture.ini']:shutil.copy2(target,backup/('trial-'+name))
    for name,entry in state['entries'].items():
        target=game/name
        if entry['before'] is not None:shutil.copyfile(backup/name,target);assert sha(target)==entry['before']
        elif target.exists():target.unlink()
    for name,expected in state['protected'].items():assert sha(game/name)==expected
    state['restored']=True;record.write_text(json.dumps(state,indent=2)+'\n')
    print('Working bootstrap/latency/config restored byte for byte; trial DLLs removed.')
