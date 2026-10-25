"""Generate the common install contract from selected manifest/lock snapshots.

This file contains packaging policy, not executable download or game mutation.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
from urllib.parse import urlparse

BIN='Dungeons/Binaries/Win64/'
PAK='Dungeons/Content/Paks/~mods/'

def safe_path(path):
    if not isinstance(path,str) or not path or ':' in path or '\\' in path or any(p in ('','.','..') for p in path.split('/')):
        raise ValueError('Unsafe installation path')
    if not path.startswith((BIN,PAK)):
        raise ValueError('Destination outside mod directories')
    return path

def external_runtime(path):
    name=Path(path).name.lower()
    return name.startswith(('sl.','nvngx_','amd_fidelityfx_')) or name in ('nvlowlatencyvk.dll','reshade64.dll','d3d12.asi','dlss-enabler-headless.dll','xgameruntime.thunks.dll')

def hashes(files,owned=False):
    if not isinstance(files,dict) or not files:raise ValueError('Empty file set')
    seen=set()
    for path,pin in files.items():
        safe_path(path)
        if path.casefold() in seen:raise ValueError('Duplicate destination')
        seen.add(path.casefold())
        if not isinstance(pin,str) or not re.fullmatch('[a-f0-9]{64}',pin):raise ValueError('Invalid SHA-256')
        if owned and external_runtime(path):raise ValueError('External framework/vendor runtime is not owned payload')
    return copy.deepcopy(files)

def snapshot_hash(value):
    return hashlib.sha256(json.dumps(value,sort_keys=True,separators=(',',':'),ensure_ascii=False).encode()).hexdigest()

def acquisition_route(name,definition,url):
    """Reviewed publisher routes; all bytes still need the selected lock's hash."""
    explicit=definition.get('acquisition')
    if explicit is not None:
        return explicit['method'],explicit.get('url',url)
    if name=='ReShade' and definition.get('file','').endswith('/d3d12.asi'):
        return 'browser','https://www.nexusmods.com/minecraftdungeons2/mods/87?tab=files'
    if name=='DLSSRuntime' and re.fullmatch(r'https://raw\.githubusercontent\.com/NVIDIA/DLSS/[a-f0-9]{40}/lib/Windows_x86_64/rel/nvngx_dlss\.dll',url):
        return 'direct',url
    if name=='Streamline' and definition.get('archiveSHA256') and url.startswith('https://github.com/NVIDIA-RTX/Streamline/releases/download/'):
        return 'direct',url
    if name=='RenoDXUEExtended' and url.startswith('https://github.com/marat569/renodx/releases/download/'):
        return 'direct',url
    if name in ('FSRUpscaler','FSRFrameGeneration'):
        match=re.fullmatch(r'https://github\.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/tree/([a-f0-9]{40})',url)
        if match:
            filename='amd_fidelityfx_'+('upscaler' if name=='FSRUpscaler' else 'framegeneration')+'_dx12.dll'
            return 'direct','https://raw.githubusercontent.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/'+match[1]+'/Kits/FidelityFX/signedbin/'+filename
    return 'browser',url

def build_contract(manifest,dependency_lock,profiles=None):
    version=manifest['version']
    if not isinstance(version,str) or not re.fullmatch('[a-zA-Z0-9.+_-]{1,100}',version):raise ValueError('Invalid package version')
    payload=hashes(manifest['files'],owned=True)
    game=dependency_lock['game']
    editions={'Steam':{'binaryDirectory':BIN.rstrip('/'),'shipping':BIN+'Dungeons-Win64-Shipping.exe','exeSHA256':game['exeSHA256'],'build':str(game['steamBuildID'])}}
    if 'winGDK' in game:
        gdk=game['winGDK'];editions['WinGDK']={'binaryDirectory':'Dungeons/Binaries/WinGDK','shipping':'Dungeons/Binaries/WinGDK/Dungeons-WinGDK-Shipping.exe','exeSHA256':gdk['exeSHA256'],'build':gdk['version']}
    for edition in editions.values():
        if not re.fullmatch('[a-f0-9]{64}',edition['exeSHA256']):raise ValueError('Invalid executable hash')
    # Profile mapping is derived from the same canonical layout as GameInstallationProfile.
    if profiles is not None and profiles!=editions:raise ValueError('Profile mapping differs from supported layouts')
    dependencies=[]
    definitions=list(dependency_lock['dependencies'].items())+list(dependency_lock.get('editionDependencies',{}).get('WinGDK',{}).items())
    seen=set()
    destinations=set(p.casefold() for p in payload)
    for name,definition in definitions:
        if name in seen:raise ValueError('Duplicate requirement')
        seen.add(name)
        files=hashes(definition.get('files') or {definition['file']:definition['sha256']})
        if any(p.casefold() in destinations for p in files):raise ValueError('Overlapping payload/dependency destinations')
        destinations.update(p.casefold() for p in files)
        url=next((definition[k] for k in ('download','source','url') if k in definition),None)
        if not url or urlparse(url).scheme!='https' or not urlparse(url).hostname:raise ValueError('Requirement needs an HTTPS publisher route')
        releases=[{'version':str(definition.get('version',definition.get('releaseTag','Pinned'))),'files':files,'archiveSHA256':definition.get('archiveSHA256')}]
        for release in definition.get('compatibleReleases',[]):
            f=hashes(release['files'])
            if f.keys()!=files.keys():raise ValueError('Mixed compatible release members')
            releases.append({'version':release['version'],'files':f,'archiveSHA256':release.get('archiveSHA256')})
        for release in releases:
            if release['archiveSHA256'] is not None and not re.fullmatch('[a-f0-9]{64}',release['archiveSHA256']):raise ValueError('Invalid archive hash')
        # Direct downloads are explicit policy, never inferred just from a URL suffix.
        method,url=acquisition_route(name,definition,url)
        if method not in ('browser','direct'):raise ValueError('Unknown acquisition method')
        dependencies.append({'id':name,'role':'edition' if definition.get('gamePassOnly') or name=='GDKRuntime' else 'common','editions':['WinGDK'] if definition.get('gamePassOnly') or name=='GDKRuntime' else list(editions),'reason':'Required by the current complete graphics candidate; feature admission stays in the game.','files':files,'releases':releases,'archiveMembers':definition.get('archiveMembers'),'acquisition':{'method':method,'url':url},'upgradeFromFileSets':definition.get('upgradeFromFileSets',[])+([definition['upgradeFromFiles']] if 'upgradeFromFiles' in definition else [])})
    return {'schemaVersion':1,'kind':'mcd2-graphics','packageVersion':version,'manifestIdentity':snapshot_hash(manifest),'dependencyIdentity':snapshot_hash(dependency_lock),'editionProfiles':editions,'payload':payload,'dependencies':dependencies,'configuration':{'startup':{'path':'Dungeons/Config/UserEngine.ini','section':'SystemSettings','keys':{'r.AllowHDR':'1','r.AntiAliasingMethod':'2'}},'hdrActivation':{'path':BIN+'ReShade.ini','lease':BIN+'MCD2Graphics/HdrAddonActivation.bin'},'frameworkPreviousSHA256':dependency_lock['dependencies'].get('ReShade',{}).get('bootstrapPreviousSHA256')},'migration':{'receiptSchemaVersion':1,'owners':['standalone','vortex'],'legacyOwner':'standalone','previousInstalls':manifest.get('upgradeFrom',{})}}

def installer_revision(manifest,dependency_lock,version):
    """New installer identity without relabelling/rebuilding native or UI bytes."""
    if not re.fullmatch(re.escape(manifest['version'])+r'-install\.[1-9][0-9]*',version):
        raise ValueError('Installer revision must extend the selected native candidate version')
    result=copy.deepcopy(manifest)
    prior=dict(manifest['files'])
    runtime=dependency_lock['dependencies']['DLSSRuntime']
    prior.update(runtime.get('files') or {runtime['file']:runtime['sha256']})
    result.setdefault('upgradeFrom',{})[manifest['version']]=prior
    result['nativeCandidateVersion']=manifest['version']
    result['version']=version
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--manifest',type=Path,required=True);p.add_argument('--dependency-lock',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    result=build_contract(json.loads(a.manifest.read_bytes()),json.loads(a.dependency_lock.read_bytes()))
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
