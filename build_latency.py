"""Build independent display/Streamline Reflex payload from separately acquired SDKs."""
from pathlib import Path
import argparse,subprocess,re,json,hashlib
from build_toolchain import native_frame_guard,normalize_windows_header
from build_amd_latency import build as build_amd_bridge
r=Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--streamline-sdk',required=True,type=Path);p.add_argument('--windows-sysroot',required=True,type=Path)
p.add_argument('--reshade-include',required=True,type=Path);p.add_argument('--output',type=Path,default=r/'dist/latency')
p.add_argument('--clang-cl',default='clang-cl');p.add_argument('--linker',default='lld-link');p.add_argument('--mingw-cxx',default='x86_64-w64-mingw32-g++')
a=p.parse_args();o=a.output.resolve();o.mkdir(parents=True,exist_ok=True);src=r/'src/latency';inc=o/'include';inc.mkdir(exist_ok=True)
version=(a.streamline_sdk/'include/sl_version.h').read_text()
assert all(re.search(r'#define SL_VERSION_'+k+r'\s+'+v+r'\b',version)for k,v in [('MAJOR','2'),('MINOR','14'),('PATCH','1')]),'Expected Streamline 2.14.1'
assert '#define RESHADE_API_VERSION 20' in (a.reshade_include/'reshade.hpp').read_text(),'Expected ReShade API20'
for f in a.reshade_include.glob('reshade*.hpp'):(inc/f.name).write_bytes(normalize_windows_header(f.read_bytes()))
cmd=[a.clang_cl,'/nologo','/std:c++20','/O2','/MT','/EHsc','/c',str(src/'streamline_bridge.cpp'),'/Fo'+str(o/'streamline_bridge.obj'),'/I'+str(a.streamline_sdk.resolve()/'include'),'/clang:-fms-compatibility-version=19.44']
for i in ['crt/include','sdk/include/ucrt','sdk/include/shared','sdk/include/um','sdk/include/winrt']:cmd+=['/imsvc'+str(a.windows_sysroot.resolve()/i)]
exports=['verify','init','set_device','mode_limit','mode','begin','index','sleep','marker','state','report_range','pcl_message','shutdown']
link=[a.linker,'/dll','/out:'+str(o/'mcd2-streamline-bridge.dll'),str(o/'streamline_bridge.obj')]+['/export:mcd2_sl_'+x for x in exports]+['kernel32.lib','user32.lib','wintrust.lib','crypt32.lib','advapi32.lib','shell32.lib']
for l in ['crt/lib/x86_64','sdk/lib/ucrt/x86_64','sdk/lib/um/x86_64']:link+=['/libpath:'+str(a.windows_sysroot.resolve()/l)]
commands=[cmd,link,[a.mingw_cxx,'-std=c++20','-O2',*native_frame_guard(a.mingw_cxx),'-shared','-static','-s','-I'+str(inc),str(src/'latency_addon.cpp'),'-o',str(o/'mcd2-display-latency.addon64'),'-ld3d12','-ldxgi','-ld3dcompiler','-lole32','-luuid']]
with(o/'build.log').open('w')as log:
 for command in commands:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
amd_bridge=build_amd_bridge(a.windows_sysroot.resolve(),o/'amd-latency',a.clang_cl,a.linker)
sources=[*src.glob('*.cpp'),*src.glob('*.hpp'),*(r/'src/providers').glob('*.hpp'),*(r/'third-party/AntiLag2').glob('*')]
(o/'build-result.json').write_text(json.dumps({'sdk':'2.14.1','sourceSHA256':{f.relative_to(r).as_posix():hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(sources)},'files':{f.name:hashlib.sha256(f.read_bytes()).hexdigest()for f in [o/'mcd2-streamline-bridge.dll',o/'mcd2-display-latency.addon64',amd_bridge]},'runtimeQualified':False},indent=2)+'\n')
print('Display/latency addon and MSVC ABI bridge built. Runtime and Windows gates remain separate.')
