"""Build isolated FG probes from separately acquired pinned SDK/runtime files."""
from pathlib import Path
import argparse, hashlib, json, re, subprocess, zipfile

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--sdk',required=True,type=Path)
p.add_argument('--sysroot',required=True,type=Path)
p.add_argument('--runtime-archive',required=True,type=Path)
p.add_argument('--output',required=True,type=Path)
p.add_argument('--probe',choices=['owned','presentation'],default='owned')
p.add_argument('--reshade-headers',type=Path,help='Optionally build the read-only synthetic proxy-chain observer')
p.add_argument('--early-bootstrap',action='store_true',help='Build the separate experimental DXGI startup route')
p.add_argument('--guide-recon',action='store_true',help='Build a separate bounded FG opt-in game observer with owned UI-alpha conversion')
p.add_argument('--dxc',type=Path,help='Offline compiler for the owned UI-alpha shader')
p.add_argument('--clang-cl',default='clang-cl')
p.add_argument('--lld-link',default='lld-link')
a=p.parse_args();src=Path(__file__).resolve().parent;repo=src.parents[1]
if a.guide_recon and not a.reshade_headers:p.error('--guide-recon requires --reshade-headers')
if a.guide_recon and not a.dxc:p.error('--guide-recon requires --dxc')
lock=json.loads((repo/'dependencies.lock.json').read_text())['dependencies']['Streamline']
with a.runtime_archive.open('rb') as f:archive_hash=hashlib.file_digest(f,'sha256').hexdigest()
assert archive_hash==lock['archiveSHA256'],'Unpinned Streamline archive'
version=(a.sdk/'include/sl_version.h').read_text()
for key,value in [('MAJOR',2),('MINOR',14),('PATCH',1)]:
    assert re.search(r'#define\s+SL_VERSION_'+key+r'\s+'+str(value)+r'\b',version),'Expected SDK 2.14.1'
out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
host='owned_inputs_probe.cpp' if a.probe=='owned' else 'presentation_probe.cpp'
commands=[];objects=[]
if a.guide_recon:commands.append([str(a.dxc.resolve()),'-T','cs_6_0','-E','main',str(src/'fg_ui_alpha.hlsl'),'-Fo',str(out/'FG_UI_ALPHA.cso')])
for name in ['streamline_probe_bridge.cpp',host]:
    obj=out/(Path(name).stem+'.obj');objects.append(str(obj))
    cmd=['clang-cl','/nologo','/std:c++20','/O2','/MT','/EHsc','/c',str(src/name),'/Fo'+str(obj),'/I'+str(a.sdk.resolve()/'include'),'/clang:-fms-compatibility-version=19.44','/clang:-Wframe-larger-than=16384','/clang:-Werror=frame-larger-than']
    for inc in ['crt/include','sdk/include/ucrt','sdk/include/shared','sdk/include/um']:
        cmd+=['/imsvc'+str(a.sysroot.resolve()/inc)]
    commands.append(cmd)
link=['lld-link','/out:'+str(out/'fg-probe.exe'),*objects,'kernel32.lib','user32.lib','wintrust.lib','crypt32.lib','d3d12.lib','dxgi.lib','dxguid.lib','advapi32.lib','shell32.lib']
for folder in ['crt/lib/x86_64','sdk/lib/ucrt/x86_64','sdk/lib/um/x86_64']:
    link+=['/libpath:'+str(a.sysroot.resolve()/folder)]
commands.append(link)
if a.early_bootstrap:
    assert a.probe=='owned','Early bootstrap requires the owned-input host'
    exports=['mcd2_sl_'+x for x in ['verify','init','set_device','mode_limit','mode','begin','index','sleep','marker','state','report_range','pcl_message','shutdown']]+['mcd2_fg_'+x for x in ['initialized','support','upgrade','state','unload','api_error','mode','inputs','configure','configured','last_config','release','game_guides','game_images','ngx_owner_v1']]
    bridge=['lld-link','/dll','/out:'+str(out/'fg-sdk-bridge.dll'),'/implib:'+str(out/'fg-sdk-bridge.lib'),objects[0],*['/export:'+x for x in exports],*[x for x in link[3:] if not x.endswith('.obj')]]
    shimobj=out/'bootstrap_dxgi.obj'
    compile_shim=[*commands[1 if a.guide_recon else 0], '/I'+str(a.sdk.resolve()/'external/nvapi')]
    compile_shim=[str(src/'bootstrap_dxgi.cpp') if x==str(src/'streamline_probe_bridge.cpp') else '/Fo'+str(shimobj) if x.startswith('/Fo') else x for x in compile_shim]
    shim=['lld-link','/dll','/out:'+str(out/'dxgi.dll'),'/implib:'+str(out/'bootstrap-dxgi.lib'),str(shimobj),str(out/'fg-sdk-bridge.lib'),*['/export:'+x for x in ['CreateDXGIFactory','CreateDXGIFactory1','CreateDXGIFactory2','DXGIDeclareAdapterRemovalSupport','DXGIGetDebugInterface1']],*[x for x in link[3:] if not x.endswith('.obj')]]
    hostlink=[x for x in link if x!=objects[0] and x!='dxgi.lib']+[str(out/'fg-sdk-bridge.lib'),str(out/'bootstrap-dxgi.lib')]
    commands[-1:]=[bridge,compile_shim,shim,hostlink]
if a.reshade_headers:
    obj=out/'chain_observer.obj'
    cmd=['clang-cl','/nologo','/std:c++20','/O2','/MT','/EHsc','/c',str(src/'chain_observer.cpp'),'/Fo'+str(obj),'/I'+str(a.reshade_headers.resolve()),'/clang:-fms-compatibility-version=19.44','/clang:-Wframe-larger-than=16384','/clang:-Werror=frame-larger-than']
    for inc in ['crt/include','sdk/include/ucrt','sdk/include/shared','sdk/include/um']:cmd+=['/imsvc'+str(a.sysroot.resolve()/inc)]
    commands.append(cmd)
    commands.append(['lld-link','/dll','/out:'+str(out/'fg-chain-observer.addon64'),str(obj),'kernel32.lib','user32.lib',*[item for item in link if item.startswith('/libpath:')]])
if a.guide_recon:
    obj=out/'guide_recon.obj'
    cmd=[str(src/'guide_recon.cpp') if x==str(src/'chain_observer.cpp') else '/Fo'+str(obj) if x.startswith('/Fo') else x for x in cmd]
    commands.append(cmd)
    commands.append(['lld-link','/dll','/out:'+str(out/'mcd2-fg-guide-recon.addon64'),str(obj),'kernel32.lib','user32.lib','dxguid.lib','d3d12.lib','d3dcompiler.lib',*[item for item in link if item.startswith('/libpath:')]])
with (out/'build-private.log').open('w') as log:
    for cmd in commands:
        if cmd[0]=='clang-cl':cmd[0]=a.clang_cl
        elif cmd[0]=='lld-link':cmd[0]=a.lld_link
        subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True)
files={}
with zipfile.ZipFile(a.runtime_archive) as archive:
    for name in ['sl.interposer.dll','sl.common.dll','sl.pcl.dll','sl.reflex.dll','sl.dlss_g.dll','nvngx_dlssg.dll','NvLowLatencyVk.dll']:
        data=archive.read('bin/x64/'+name);(out/name).write_bytes(data);files[name]=hashlib.sha256(data).hexdigest()
sources=['streamline_probe_bridge.cpp','fg_bridge_contract.h','fg_camera_contract.h','fg_configuration.hpp',host]+(['factory_route.hpp'] if a.probe=='owned' else [])+(['chain_observer.cpp'] if a.reshade_headers else [])+(['bootstrap_dxgi.cpp','fg_ui_protocol.hpp','wine_reflex_pacing.hpp'] if a.early_bootstrap else [])
own=['fg-probe.exe']+(['fg-chain-observer.addon64'] if a.reshade_headers else [])+(['fg-sdk-bridge.dll','dxgi.dll'] if a.early_bootstrap else [])
if a.guide_recon:sources+=['guide_recon.cpp','fg_alpha_copy.h','fg_ui_alpha.hlsl','fg_controls.hpp','fg_ui_protocol.hpp'];own+=['mcd2-fg-guide-recon.addon64','FG_UI_ALPHA.cso']
(out/'build-receipt.json').write_text(json.dumps({'sdk':'2.14.1','archiveSHA256':archive_hash,'probe':a.probe,'earlyBootstrap':a.early_bootstrap,'files':files,'dxcSHA256':hashlib.sha256(a.dxc.read_bytes()).hexdigest() if a.guide_recon else None,'sourceSHA256':{name:hashlib.sha256((src/name).read_bytes()).hexdigest() for name in sources},'ownBinariesSHA256':{name:hashlib.sha256((out/name).read_bytes()).hexdigest() for name in own},'executableSHA256':hashlib.sha256((out/'fg-probe.exe').read_bytes()).hexdigest()},indent=2))
print('Isolated '+a.probe+' FG probe built; production runtime archive verified. No game files changed.')
