"""Build the caller-owned MFG bridge and isolated ABI host; acquire no runtimes."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[2]
_spec=importlib.util.spec_from_file_location('_mfg_probe',Path(__file__).with_name('build_mfg_compute_probe.py'))
_probe=importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_probe)
EXPORTS=['mcd2_mfg_'+name+'_v1' for name in ('load','capabilities','configure','state','timing')]
EXPORTS+=['mcd2_mfg_configure_v2','mcd2_mfg_configure_v3']
EXPORTS+=['mcd2_afg_'+name+'_v1' for name in ('support','swap','output','scene','state','diagnostic_bind','retire','presenter_switch_state','antilag_ready','antilag')]
EXPORTS+=['mcd2_afg_'+name+'_v2' for name in ('recording_contract','guides','world','present','diagnostics')]

def build(headers,output,sysroot=None,compiler='clang-cl',linker='lld-link'):
    headers,output=Path(headers).resolve(),Path(output).resolve()
    _probe.verify_headers(headers)
    output.mkdir(parents=True,exist_ok=True)
    includes=['/I'+str(headers)]
    libraries=[]
    if sysroot:
        for p in ('crt/include','sdk/include/ucrt','sdk/include/shared','sdk/include/um'):
            includes.append('/imsvc'+str(Path(sysroot)/p))
        for p in ('crt/lib/x86_64','sdk/lib/ucrt/x86_64','sdk/lib/um/x86_64'):
            libraries.append('/libpath:'+str(Path(sysroot)/p))
    outputs={}
    sources=['experiments/providers/mfg_game_bridge.cpp','tests/mfg_game_bridge_abi_test.cpp','tests/mfg_game_allocation_test.cpp']
    if (ROOT/'tests/mfg_game_bridge_host.cpp').exists(): sources.append('tests/mfg_game_bridge_host.cpp')
    with (output/'build-private.log').open('w') as log:
        for source in sources:
            name='mcd2-mfg-game-bridge' if source.endswith('mfg_game_bridge.cpp') else Path(source).stem
            dll=source.endswith('mfg_game_bridge.cpp')
            obj=output/(name+'.obj');binary=output/(name+('.dll' if dll else '.exe'))
            command=[compiler,'/nologo','/std:c++20','/O2','/MT','/EHsc','/c',str(ROOT/source),'/Fo'+str(obj),
                     '/clang:-fms-compatibility-version=19.44','/clang:-Wframe-larger-than=16384','/clang:-Werror=frame-larger-than',*includes]
            link=[linker,'/out:'+str(binary),str(obj),'kernel32.lib','user32.lib','d3d12.lib','dxgi.lib','dxguid.lib','bcrypt.lib','d3dcompiler.lib',*libraries]
            if dll:link+=['/dll',*['/export:'+name for name in EXPORTS]]
            for step in (command,link):
                try:subprocess.run(step,stdout=log,stderr=subprocess.STDOUT,check=True)
                except subprocess.CalledProcessError:
                    log.flush();print((output/'build-private.log').read_text(errors='replace')[-12000:]);raise
            outputs[binary.name]=_probe.sha(binary)
    sources+=['experiments/providers/mfg_game_bridge.h','experiments/providers/mfg_game_swapchain.hpp',
              'experiments/providers/mfg_headless.hpp','experiments/providers/mfg_ngx_parameters.hpp','experiments/providers/mfg_game_parameters.hpp','experiments/providers/mfg_game_guides.hpp','experiments/providers/mfg_game_timeline.hpp','tests/mfg_game_timeline_test.cpp',
              'experiments/providers/fsr_fg_game_bridge.h','experiments/providers/build_mfg_game_bridge.py',
              'src/providers/mfg_game_transaction.hpp','src/providers/fg_frame_order.hpp','src/providers/mfg_phase_plan.hpp','src/providers/fg_output_contract.hpp',
              'src/providers/runtime_diagnostics.h','src/native/mfg_parameters.hpp','experiments/fg-streamline/fg_camera_contract.h',
              'src/latency/process_exit.hpp','src/latency/process_lifetime.hpp','third-party/AntiLag2/ffx_antilag2_dx12.h','experiments/providers/build_mfg_compute_probe.py','experiments/providers/mfg_owned_pattern.hpp']
    receipt=dict(schemaVersion=1,kind='mfg-game-bridge',abi='Microsoft C++ x64',
                 sourceSHA256={p:_probe.sha(ROOT/p) for p in sources},binarySHA256=outputs,
                 SDKheadersSHA256=_probe.HEADERS,sdkCommit=_probe.SDK_COMMIT,
                 runtimeBundled=False,runtimeQualified=False,gameIntegrationQualified=False)
    (output/'build-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    (output/'NVIDIA-SDK-LICENSE.txt').write_bytes((ROOT/'third-party/NVIDIA-SDK-LICENSE.txt').read_bytes())
    return output/'mcd2-mfg-game-bridge.dll'

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ngx-headers',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--windows-sysroot',type=Path)
    parser.add_argument('--clang-cl',default='clang-cl')
    parser.add_argument('--linker',default='lld-link')
    args=parser.parse_args()
    build(args.ngx_headers,args.output,args.windows_sysroot,args.clang_cl,args.linker)
    print('MFG game bridge source build completed; runtime and game qualification remain separate.')
