"""Compile an isolated owned-input MFG gate. No provider runtime is downloaded."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[2]
SDK_COMMIT='374959484e79a640feaba44c93ac8cfb0a03f5b5'
HEADERS={'nvsdk_ngx_params.h':'943bc8cc5cdae03b6303016fbad3183636f2335ae27a2d18776798c3b4efabbc',
         'nvsdk_ngx_defs.h':'ea23f33497cd274860d1c25a97644fce807dcb0037c594547203343103fad03e'}
SOURCES=('experiments/providers/mfg_compute_probe.cpp','experiments/providers/mfg_headless.hpp',
         'experiments/providers/mfg_owned_pattern.hpp',
         'experiments/providers/mfg_ngx_parameters.hpp','src/native/mfg_parameters.hpp',
         'src/providers/mfg_phase_plan.hpp','experiments/providers/build_mfg_compute_probe.py',
         'experiments/providers/run_mfg_compute_probe.py','tests/mfg_ngx_parameters_abi_test.cpp')


def sha(path):
    with Path(path).open('rb') as stream: return hashlib.file_digest(stream,'sha256').hexdigest()


def verify_headers(sdk):
    for name,expected in HEADERS.items():
        if sha(sdk/name)!=expected: raise ValueError('Pinned NGX declaration changed: '+name)


def build(sdk,output,sysroot=None,compiler='clang-cl',linker='lld-link'):
    sdk,output=Path(sdk).resolve(),Path(output).resolve()
    verify_headers(sdk); output.mkdir(parents=True,exist_ok=True)
    libraries=['kernel32.lib','user32.lib','d3d12.lib','dxgi.lib','dxguid.lib','bcrypt.lib']
    with (output/'build-private.log').open('w') as log:
        for source,name in [('experiments/providers/mfg_compute_probe.cpp','mfg-compute-probe'),
                            ('tests/mfg_ngx_parameters_abi_test.cpp','mfg-parameters-abi-test')]:
            obj=output/(name+'.obj'); binary=output/(name+'.exe')
            compile_step=[str(compiler),'/nologo','/std:c++20','/O2','/MT','/EHsc','/c',str(ROOT/source),
                          '/Fo'+str(obj),'/I'+str(sdk),'/clang:-fms-compatibility-version=19.44',
                          '/clang:-Wframe-larger-than=16384','/clang:-Werror=frame-larger-than']
            link_step=[str(linker),'/out:'+str(binary),str(obj),*libraries]
            if sysroot:
                for folder in ('crt/include','sdk/include/ucrt','sdk/include/shared','sdk/include/um'):
                    compile_step.append('/imsvc'+str(Path(sysroot)/folder))
                for folder in ('crt/lib/x86_64','sdk/lib/ucrt/x86_64','sdk/lib/um/x86_64'):
                    link_step.append('/libpath:'+str(Path(sysroot)/folder))
            for command in (compile_step,link_step):
                try: subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
                except subprocess.CalledProcessError:
                    log.flush(); print((output/'build-private.log').read_text(errors='replace')[-8000:]); raise
    receipt={'schemaVersion':1,'probeKind':'mfg-owned-inputs','sdkCommit':SDK_COMMIT,'SDKheadersSHA256':HEADERS,
             'sourceSHA256':{name:sha(ROOT/name) for name in SOURCES},
             'binarySHA256':sha(output/'mfg-compute-probe.exe'),
             'abiTestSHA256':sha(output/'mfg-parameters-abi-test.exe'),
             'runtimeBundled':False,'gameIntegrationQualified':False,'runtimeQualified':False,
             'compilerSHA256':sha(Path(compiler)) if Path(compiler).is_file() else None,
             'abi':'Microsoft C++ x64','runtimeModuleLifetime':'process'}
    (output/'build-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    (output/'NVIDIA-SDK-LICENSE.txt').write_bytes((ROOT/'third-party/NVIDIA-SDK-LICENSE.txt').read_bytes())
    return output/'mfg-compute-probe.exe'


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ngx-headers',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--windows-sysroot',type=Path)
    parser.add_argument('--clang-cl',default='clang-cl')
    parser.add_argument('--linker',default='lld-link')
    args=parser.parse_args()
    build(args.ngx_headers,args.output,args.windows_sysroot,args.clang_cl,args.linker)
    print('Owned-input MFG probe compiled; GPU and game qualification remain unrun.')


if __name__=='__main__': main()
