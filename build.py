"""Build using separately acquired, pinned SDKs. No vendor downloads or license acceptance."""
from pathlib import Path
import argparse, subprocess, re, json, hashlib, sys
from build_toolchain import native_frame_guard, normalize_windows_header
r=Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--dotnet',type=Path);p.add_argument('--neorune-sdk',type=Path);p.add_argument('--pack-tools',type=Path)
p.add_argument('--reshade-include',required=True,type=Path);p.add_argument('--ngx-include',required=True,type=Path)
p.add_argument('--dxc',type=Path);p.add_argument('--output',type=Path,default=r/'dist/build')
p.add_argument('--developer-controls',action='store_true',help='Enable bounded private controls for native-only experiments; never a release build.')
p.add_argument('--fsr-bridge-sha256',default='',help='Exact source-built FSR bridge pin for an opt-in continuous candidate; empty keeps FSR inactive.')
p.add_argument('--native-only',action='store_true',help='Rebuild only the addon; retain separately qualified UI and shader payloads.')
p.add_argument('--clang-cxx',default='clang++',help='Clang executable for the MSVC ABI bridges.')
p.add_argument('--mingw-cxx',default='x86_64-w64-mingw32-g++',help='MinGW C++ compiler; Windows LLVM MinGW clang++ is also supported.')
a=p.parse_args()
if a.developer_controls and not a.native_only:p.error('Developer controls require an isolated native-only experiment.')
if a.fsr_bridge_sha256 and not re.fullmatch('[0-9a-f]{64}',a.fsr_bridge_sha256):p.error('FSR bridge pin must be a lowercase SHA-256')
o=a.output.resolve();o.mkdir(parents=True,exist_ok=True);native=r/'src/native'
if not a.native_only and any(x is None for x in (a.dotnet,a.neorune_sdk,a.pack_tools,a.dxc)):
 p.error('Full builds require --dotnet, --neorune-sdk, --pack-tools and --dxc.')
assert '#define RESHADE_API_VERSION 20' in (a.reshade_include/'reshade.hpp').read_text(),'Expected ReShade API 20'
# SDK include files stay outside the public source tree. Use the platform's
# windows.h directly: a case-only shim recursively includes itself on Windows.
inc=o/'include';inc.mkdir(exist_ok=True)
(inc/'Windows.h').unlink(missing_ok=True)
for f in a.reshade_include.glob('reshade*.hpp'):
 # Upstream ReShade also spells this Windows.h. Normalize the generated
 # include copy instead of adding a case-only shim that recurses on Windows.
 (inc/f.name).write_bytes(normalize_windows_header(f.read_bytes()))
ngx=o/'ngx-research';ngx.mkdir(exist_ok=True)
for f in a.ngx_include.glob('nvsdk_ngx*.h'):(ngx/f.name).write_bytes(f.read_bytes())
commands=[
 [a.clang_cxx,'--target=x86_64-pc-windows-msvc','-std=c++20','-O2','-fno-exceptions','-fno-rtti','-I'+str(ngx),'-c',str(native/'ngx_parameter_bridge.cpp'),'-o',str(o/'ngx_parameter_bridge.obj')],
 [a.clang_cxx,'--target=x86_64-pc-windows-msvc','-ffreestanding','-std=c++20','-O2','-fno-exceptions','-fno-rtti','-nostdinc++','-I'+str(native/'bridge-freestanding'),'-I'+str(inc),'-c',str(native/'reshade_public_bridge.cpp'),'-o',str(o/'reshade_public_bridge.obj')],
 [a.mingw_cxx,'-DMCD2_FSR_BRIDGE_SHA256=\"'+a.fsr_bridge_sha256+'\"','-DMCD2_ENABLE_DIAGNOSTICS='+('1' if a.developer_controls else '0'),'-std=c++20','-O2',*native_frame_guard(a.mingw_cxx),'-shared','-static','-I'+str(inc),'-I'+str(o),str(native/'observer.cpp'),str(o/'ngx_parameter_bridge.obj'),str(o/'reshade_public_bridge.obj'),'-o',str(o/'mcd2-graphics.addon64'),'-ld3d12','-ldxgi','-ld3dcompiler','-lole32','-luuid','-lbcrypt']
]
for cmd in commands:subprocess.run(cmd,check=True)
if a.native_only:
 print('Native addon built. Requalify this artifact before packaging; UI and shader were not rebuilt.')
 sys.exit(0)
subprocess.run([str(a.dxc),'-T','cs_6_0','-E','main',str(r/'src/shaders/live_dense.hlsl'),'-Fo',str(o/'live_dense.cso')],check=True)
subprocess.run([str(a.dxc),'-T','cs_6_0','-E','main',str(r/'src/shaders/fsr_dense.hlsl'),'-Fo',str(o/'fsr_dense.cso')],check=True)
assert '<version>0.1.2</version>' in (a.neorune_sdk/'NeoRune.Sdk.nuspec').read_text(encoding='utf-8-sig'),'Expected NeoRune 0.1.2'
dotnet_root=a.dotnet.resolve().parent
refs=dotnet_root/'packs/Microsoft.NETCore.App.Ref/10.0.12/ref/net10.0'
assert refs.is_dir(),'Expected .NET 10.0.12 reference pack'
args=['build','--mod=MCD2Graphics','--out='+str(o/'ui'),'--tools='+str(a.pack_tools.resolve())]
args += ['--source='+str(a.neorune_sdk/'src'/f)for f in ('Log.cs','Timer.cs','World.cs')]
args += ['--ref='+str(f)for f in sorted(refs.glob('*.dll'))]
args += ['--ref='+str(a.neorune_sdk/'ref'/f)for f in ('NeoRune.Abstractions.dll','NeoRune.Game.dll')]
args += ['--source='+str(r/'src/ui'/f) for f in ('ModActor.cs','ProviderSaveWords.cs','ProviderMenuClient.cs','ProviderRuntimeWords.cs','ProviderRuntimeClient.cs')]
rsp=o/'ui-build.rsp';rsp.write_text('\n'.join(args)+'\n')
c=subprocess.run([str(a.dotnet),str(a.neorune_sdk/'tools/neorune.dll'),'@'+str(rsp)],capture_output=True,text=True)
(o/'ui-build.log').write_text(c.stdout+c.stderr)
assert c.returncode==0 and not re.search(r'\berror\s+\w+\d+:',c.stdout+c.stderr,re.I),'NeoRune build diagnostic failure'
subprocess.run([str(a.dotnet),'build',str(r/'tools/ModInfoBuilder/ModInfoBuilder.csproj'),'-p:NeoRuneSdk='+str(a.neorune_sdk.resolve()),'-c','Release','--nologo'],check=True)
subprocess.run([str(a.dotnet),str(r/'tools/ModInfoBuilder/bin/Release/net10.0/ModInfoBuilder.dll'),str(o/'ui/Assets'),str(a.pack_tools.resolve()),str(o/'ui/Pak'),str(r/'manifest.json')],check=True)
print('Build completed. Rebuilt artifacts require gameplay qualification before replacing the published payload.')
