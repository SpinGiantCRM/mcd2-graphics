"""Run the synthetic probe in an existing isolated Proton prefix only."""
from pathlib import Path
import argparse,hashlib,json,os,signal,subprocess,time,shutil

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--proton',required=True,type=Path)
p.add_argument('--prefix',required=True,type=Path)
p.add_argument('--steam',required=True,type=Path)
p.add_argument('--output',required=True,type=Path)
p.add_argument('--hdr',action='store_true')
p.add_argument('--wayland',action='store_true')
p.add_argument('--background',action='store_true')
p.add_argument('--reshade',type=Path)
p.add_argument('--experimental-reshade-receipt',type=Path,help='Isolated source-build experiment only; never qualifies a game/release dependency')
p.add_argument('--reshade-proxy-route',action='store_true')
p.add_argument('--factory-inner-route',action='store_true')
p.add_argument('--native-queue',action='store_true',help='Diagnostic native queue path; not a qualified production integration')
p.add_argument('--early-bootstrap',action='store_true')
a=p.parse_args();out=a.output.resolve();prefix=a.prefix.resolve()
if 'steamapps' in prefix.parts or '1912410' in prefix.parts:
    p.error('Refusing a Steam/game prefix: provide a separately prepared experiment prefix')
if not (prefix/'pfx').is_dir() or not (out/'fg-probe.exe').is_file():
    p.error('Existing isolated prefix and built probe required; this runner does not set up prefixes')
if a.reshade:
    lock=json.loads((Path(__file__).resolve().parents[2]/'dependencies.lock.json').read_text())
    digest=hashlib.sha256(a.reshade.read_bytes()).hexdigest()
    if a.experimental_reshade_receipt:
        receipt=json.loads(a.experimental_reshade_receipt.read_text())
        if digest!=receipt.get('binarySHA256') or not receipt.get('sourceClean') or not receipt.get('commit'):
            p.error('Experimental source-build receipt does not match the DLL')
    elif digest!=lock['dependencies']['ReShade']['sha256']:p.error('Unqualified ReShade DLL')
    if a.early_bootstrap and not json.loads((out/'build-receipt.json').read_text()).get('earlyBootstrap'):p.error('Early bootstrap binary was not built')
    shutil.copyfile(a.reshade,out/('d3d12.asi' if a.early_bootstrap else 'dxgi.dll'))
    config='[GENERAL]\nNoReloadOnInit=1\nSkipLoadingDisabledEffects=1\n[OVERLAY]\nTutorialProgress=4\n'
    if a.reshade_proxy_route:config+='[PROXY]\nEnableProxyLibrary=1\nProxyLibrary=sl.interposer.dll\n'
    (out/'ReShade.ini').write_text(config)
elif (out/'dxgi.dll').exists():p.error('Unexpected local DXGI DLL; use a clean output directory or --reshade')
if a.reshade_proxy_route and not a.reshade:p.error('--reshade-proxy-route requires --reshade')
if a.factory_inner_route and (not a.reshade or a.reshade_proxy_route):p.error('--factory-inner-route requires ReShade and excludes the proxy-library experiment')
if a.early_bootstrap and (not a.reshade or a.factory_inner_route or a.reshade_proxy_route):p.error('--early-bootstrap requires ReShade and excludes other factory routes')
env=os.environ.copy()
for key in ['MCD2_FG_HDR','MCD2_FG_BACKGROUND_CONTROL','MCD2_FG_RESHADEROUTE','MCD2_FG_FACTORY_ROUTE','MCD2_FG_NATIVE_QUEUE','MCD2_FG_EARLY_BOOTSTRAP','PROTON_ENABLE_WAYLAND','PROTON_ENABLE_HDR','DXVK_HDR']:
    env.pop(key,None)
overrides='d3d12,d3d12core,dxgi=n;nvapi64,nvofapi64=n;nvcuda=b'
if a.wayland:
    if not env.get('WAYLAND_DISPLAY'):p.error('No Wayland session available')
    overrides+=';winex11.drv=d;winewayland.drv=b'
    env.update(PROTON_ENABLE_WAYLAND='1')
if a.hdr:env.update(PROTON_ENABLE_HDR='1',DXVK_HDR='1',MCD2_FG_HDR='1')
if a.background:env['MCD2_FG_BACKGROUND_CONTROL']='1'
if a.reshade_proxy_route:env['MCD2_FG_RESHADEROUTE']='1'
if a.factory_inner_route:env['MCD2_FG_FACTORY_ROUTE']='1'
if a.native_queue:env['MCD2_FG_NATIVE_QUEUE']='1'
if a.early_bootstrap:env['MCD2_FG_EARLY_BOOTSTRAP']='1'
env.update(STEAM_COMPAT_DATA_PATH=str(prefix),STEAM_COMPAT_CLIENT_INSTALL_PATH=str(a.steam.resolve()),STEAM_COMPAT_INSTALL_PATH=str(out),SteamAppId='0',SteamGameId='0',STEAM_COMPAT_APP_ID='0',WINEDEBUG='-all',PROTON_LOG='0',WINEDLLOVERRIDES=overrides)
start=time.monotonic();timed_out=False
with (out/'run-private.log').open('w') as log:
    process=subprocess.Popen([str(a.proton.resolve()),'runinprefix',str(out/'fg-probe.exe')],cwd=out,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
    try:code=process.wait(timeout=45)
    except subprocess.TimeoutExpired:
        timed_out=True;os.killpg(process.pid,signal.SIGKILL);process.wait();code=None
result={'exitCode':code,'timeout':timed_out,'seconds':round(time.monotonic()-start,3),'isolatedPrefix':True,'gameFilesChanged':False,'HDR':a.hdr,'WaylandRequested':a.wayland,'backgroundControl':a.background,'ReShadeRequested':bool(a.reshade),'ReShadeProxyRoute':a.reshade_proxy_route,'FactoryInnerRoute':a.factory_inner_route,'nativeQueueControl':a.native_queue,'earlyBootstrap':a.early_bootstrap}
if a.reshade:result['ReShadeSHA256']=digest
if a.experimental_reshade_receipt:result.update(experimentalReShade=True,ReShadeCommit=receipt['commit'],gameOrReleaseQualified=False)
(out/'run-result.json').write_text(json.dumps(result,indent=2))
print(json.dumps(result))
for name in ['owned-inputs.jsonl','fg-probe.jsonl','chain-observer.jsonl','bootstrap.jsonl']:
    if (out/name).exists():
        for line in (out/name).read_text().splitlines():
            row=json.loads(line)
            if row.get('stage') in ['phase','complete','gate-complete','foreground-not-established','insufficient-generated-presents','background-control-interpolated-samples','slShutdown','early-SDK-init','wine-cubin-capability-prime','qualified-ReShade-load','factory-route','routed-hwnd-swapchain','factory-routes-detached']:print(json.dumps(row))
raise SystemExit(code if code is not None else 124)
